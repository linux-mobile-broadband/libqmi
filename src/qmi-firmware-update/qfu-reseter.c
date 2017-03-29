/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * qmi-firmware-update -- Command line tool to update firmware in QMI devices
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright (C) 2016 Zodiac Inflight Innovations
 * Copyright (C) 2016-2017 Aleksander Morgado <aleksander@aleksander.es>
 */

#include <config.h>
#include <string.h>

#include <gio/gio.h>

#include <libqmi-glib.h>

#include "qfu-reseter.h"
#include "qfu-at-device.h"
#include "qfu-utils.h"

G_DEFINE_TYPE (QfuReseter, qfu_reseter, G_TYPE_OBJECT)

struct _QfuReseterPrivate {
    QfuDeviceSelection *device_selection;
    QmiClientDms       *qmi_client;
    QmiDeviceOpenFlags  device_open_flags;
};

/******************************************************************************/
/* Run */

#define MAX_RETRIES 2

typedef struct {
    /* Files to use */
    GList      *ttys;
    GFile      *cdc_wdm;
    /* QMI client amd device */
    QmiDevice    *qmi_device;
    QmiClientDms *qmi_client;
    gboolean      ignore_release_cid;
    /* List of AT devices */
    GList *at_devices;
    GList *current;
    /* Retries */
    guint retries;
} RunContext;

static void
run_context_free (RunContext *ctx)
{
    if (ctx->cdc_wdm)
        g_object_unref (ctx->cdc_wdm);
    if (ctx->qmi_client) {
        g_assert (ctx->qmi_device);
        qmi_device_release_client (ctx->qmi_device,
                                   QMI_CLIENT (ctx->qmi_client),
                                   (ctx->ignore_release_cid ?
                                    QMI_DEVICE_RELEASE_CLIENT_FLAGS_NONE :
                                    QMI_DEVICE_RELEASE_CLIENT_FLAGS_RELEASE_CID),
                                   10, NULL, NULL, NULL);
        g_object_unref (ctx->qmi_client);
    }
    if (ctx->qmi_device) {
        qmi_device_close_async (ctx->qmi_device, 10, NULL, NULL, NULL);
        g_object_unref (ctx->qmi_device);
    }
    g_list_free_full (ctx->ttys, g_object_unref);
    g_list_free_full (ctx->at_devices, g_object_unref);
    g_slice_free (RunContext, ctx);
}

gboolean
qfu_reseter_run_finish (QfuReseter    *self,
                         GAsyncResult  *res,
                         GError       **error)
{
    return g_task_propagate_boolean (G_TASK (res), error);
}

static void run_context_step_at (GTask *task);

static gboolean
run_context_step_at_cb (GTask *task)
{
    run_context_step_at (task);
    return FALSE;
}

static void
run_context_step_at_next (GTask *task)
{
    RunContext *ctx;

    ctx = (RunContext *) g_task_get_task_data (task);
    ctx->current = g_list_next (ctx->current);

    if (!ctx->current) {
        if (!ctx->retries) {
            g_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_FAILED,
                                     "couldn't run reset operation");
            g_object_unref (task);
            return;
        }

        /* Launch retry with the first device */
        ctx->retries--;
        ctx->current = ctx->at_devices;
    }

    /* Schedule next step in an idle */
    g_idle_add ((GSourceFunc) run_context_step_at_cb, task);
}

static gint
device_sort_by_name_reversed (QfuAtDevice *a, QfuAtDevice *b)
{
    return strcmp (qfu_at_device_get_name (b), qfu_at_device_get_name (a));
}

static void
run_context_step_at (GTask *task)
{
    RunContext  *ctx;
    QfuAtDevice *at_device;
    GError      *error = NULL;

    ctx = (RunContext *) g_task_get_task_data (task);

    /* If we get to AT reset after trying QMI, and we didn't find any port to
     * use, return error */
    if (!ctx->ttys) {
        g_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT,
                                 "No devices found to run reset operation");
        g_object_unref (task);
        return;
    }

    /* Initialize AT devices the first time we get here */
    if (!ctx->at_devices) {
        GList *l;

        for (l = ctx->ttys; l; l = g_list_next (l)) {
            at_device = qfu_at_device_new (G_FILE (l->data), g_task_get_cancellable (task), &error);
            if (!at_device) {
                g_task_return_error (task, error);
                g_object_unref (task);
                return;
            }
            ctx->at_devices = g_list_append (ctx->at_devices, at_device);
        }

        /* Sort by filename reversed; usually the TTY with biggest number is a
         * good AT port */
        ctx->at_devices = g_list_sort (ctx->at_devices, (GCompareFunc) device_sort_by_name_reversed);
        /* Select first TTY to start */
        ctx->current = ctx->at_devices;
    } else
        g_assert (ctx->current);

    at_device = QFU_AT_DEVICE (ctx->current->data);
    if (!qfu_at_device_boothold (at_device, g_task_get_cancellable (task), &error)) {
        g_debug ("[qfu-reseter] error: %s", error->message);
        g_error_free (error);
        run_context_step_at_next (task);
        return;
    }

    /* Success! */
    g_debug ("[qfu-reseter] successfully run 'at boothold' operation");
    ctx->ignore_release_cid = TRUE;
    g_task_return_boolean (task, TRUE);
    g_object_unref (task);
}

static void
power_cycle_ready (QmiClientDms *qmi_client,
                   GAsyncResult *res,
                   GTask        *task)
{
    GError     *error = NULL;
    RunContext *ctx;

    ctx = (RunContext *) g_task_get_task_data (task);

    if (!qfu_utils_power_cycle_finish (qmi_client, res, &error)) {
        g_debug ("[qfu-reseter] error: couldn't power cycle: %s", error->message);
        g_error_free (error);
        g_debug ("[qfu-reseter] skipping QMI-based boothold");
        run_context_step_at (task);
        return;
    }

    g_debug ("[qfu-reseter] reset requested successfully...");

    ctx->ignore_release_cid = TRUE;
    g_task_return_boolean (task, TRUE);
    g_object_unref (task);
}

static void
set_boot_image_download_mode_ready (QmiClientDms *client,
                                    GAsyncResult *res,
                                    GTask        *task)
{
    QmiMessageDmsSetBootImageDownloadModeOutput *output;
    GError                                      *error = NULL;
    RunContext                                  *ctx;

    ctx = (RunContext *) g_task_get_task_data (task);

    output = qmi_client_dms_set_boot_image_download_mode_finish (client, res, &error);
    if (!output || !qmi_message_dms_set_boot_image_download_mode_output_get_result (output, &error)) {
        g_debug ("[qfu-reseter] error: couldn't run 'set boot image download mode' operation: %s", error->message);
        g_error_free (error);
        if (output)
            qmi_message_dms_set_boot_image_download_mode_output_unref (output);
        g_debug ("[qfu-reseter] skipping QMI-based boothold");
        run_context_step_at (task);
        return;
    }

    qmi_message_dms_set_boot_image_download_mode_output_unref (output);

    g_debug ("[qfu-reseter] successfully run 'set boot image download mode' operation");

    qfu_utils_power_cycle (ctx->qmi_client,
                           g_task_get_cancellable (task),
                           (GAsyncReadyCallback) power_cycle_ready,
                           task);
}

static void
run_context_step_qmi_boot_image_download_mode (GTask *task)
{
    RunContext                                 *ctx;
    QfuReseter                                 *self;
    QmiMessageDmsSetBootImageDownloadModeInput *input;

    ctx = (RunContext *) g_task_get_task_data (task);
    self = g_task_get_source_object (task);

    g_assert (ctx->qmi_client || self->priv->qmi_client);

    /* Try DMS 0x0050 */
    input = qmi_message_dms_set_boot_image_download_mode_input_new ();
    qmi_message_dms_set_boot_image_download_mode_input_set_mode (input, QMI_DMS_BOOT_IMAGE_DOWNLOAD_MODE_BOOT_AND_RECOVERY, NULL);
    qmi_client_dms_set_boot_image_download_mode (self->priv->qmi_client ? self->priv->qmi_client : ctx->qmi_client,
                                                 input,
                                                 10,
                                                 g_task_get_cancellable (task),
                                                 (GAsyncReadyCallback) set_boot_image_download_mode_ready,
                                                 task);
    qmi_message_dms_set_boot_image_download_mode_input_unref (input);
}

static void
set_firmware_id_ready (QmiClientDms *client,
                       GAsyncResult *res,
                       GTask        *task)
{
    QmiMessageDmsSetFirmwareIdOutput *output;
    GError                           *error = NULL;
    RunContext                       *ctx;

    ctx = (RunContext *) g_task_get_task_data (task);

    output = qmi_client_dms_set_firmware_id_finish (client, res, &error);
    if (!output || !qmi_message_dms_set_firmware_id_output_get_result (output, &error)) {
        g_debug ("[qfu-reseter] error: couldn't run 'set firmware id' operation: %s", error->message);
        g_error_free (error);
        if (output)
            qmi_message_dms_set_firmware_id_output_unref (output);
        g_debug ("[qfu-reseter] trying boot image download mode...");
        run_context_step_qmi_boot_image_download_mode (task);
        return;
    }

    qmi_message_dms_set_firmware_id_output_unref (output);

    g_debug ("[qfu-reseter] successfully run 'set firmware id' operation");
    ctx->ignore_release_cid = TRUE;
    g_task_return_boolean (task, TRUE);
    g_object_unref (task);
}

static void
run_context_step_qmi_firmware_id (GTask *task)
{
    RunContext *ctx;
    QfuReseter *self;

    ctx = (RunContext *) g_task_get_task_data (task);
    self = g_task_get_source_object (task);

    g_assert (ctx->qmi_client || self->priv->qmi_client);

    /* Run DMS 0x003e to power cycle in boot & hold mode */
    qmi_client_dms_set_firmware_id (self->priv->qmi_client ? self->priv->qmi_client : ctx->qmi_client,
                                    NULL,
                                    10,
                                    g_task_get_cancellable (task),
                                    (GAsyncReadyCallback) set_firmware_id_ready,
                                    task);
}

static void
new_client_dms_ready (gpointer      unused,
                      GAsyncResult *res,
                      GTask        *task)
{
    RunContext *ctx;
    GError     *error = NULL;

    ctx = (RunContext *) g_task_get_task_data (task);

    if (!qfu_utils_new_client_dms_finish (res,
                                          &ctx->qmi_device,
                                          &ctx->qmi_client,
                                          NULL, NULL, NULL, NULL, NULL, NULL,
                                          &error)) {
        /* Jump to AT-based boothold */
        g_debug ("[qfu-reseter] error: couldn't allocate QMI client: %s", error->message);
        g_error_free (error);
        g_debug ("[qfu-reseter] skipping QMI-based boothold");
        run_context_step_at (task);
        return;
    }

    run_context_step_qmi_firmware_id (task);
}

void
qfu_reseter_run (QfuReseter          *self,
                 GCancellable        *cancellable,
                 GAsyncReadyCallback  callback,
                 gpointer             user_data)
{
    RunContext *ctx;
    GTask      *task;

    ctx = g_slice_new0 (RunContext);
    ctx->retries = MAX_RETRIES;

    task = g_task_new (self, cancellable, callback, user_data);
    g_task_set_task_data (task, ctx, (GDestroyNotify) run_context_free);

    /* List devices to use */
    if (!self->priv->qmi_client)
        ctx->cdc_wdm = qfu_device_selection_get_single_cdc_wdm (self->priv->device_selection);
    ctx->ttys = qfu_device_selection_get_multiple_ttys  (self->priv->device_selection);

    if (!ctx->ttys && !ctx->cdc_wdm && !self->priv->qmi_client) {
        g_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT,
                                 "No devices found to run reset operation");
        g_object_unref (task);
        return;
    }

    /* If no cdc-wdm file available, try AT directly */
    if (!ctx->cdc_wdm && !self->priv->qmi_client) {
        run_context_step_at (task);
        return;
    }

    /* If we already got a QMI client as input, try QMI directly */
    if (self->priv->qmi_client) {
        run_context_step_qmi_firmware_id (task);
        return;
    }

    /* Otherwise, try to allocate a QMI client */
    g_assert (ctx->cdc_wdm);
    qfu_utils_new_client_dms (ctx->cdc_wdm,
                              3,
                              self->priv->device_open_flags,
                              FALSE,
                              cancellable,
                              (GAsyncReadyCallback) new_client_dms_ready,
                              task);
}

/******************************************************************************/

QfuReseter *
qfu_reseter_new (QfuDeviceSelection *device_selection,
                 QmiClientDms       *qmi_client,
                 QmiDeviceOpenFlags  device_open_flags)
{
    QfuReseter *self;

    self = g_object_new (QFU_TYPE_RESETER, NULL);
    self->priv->device_selection = g_object_ref (device_selection);
    self->priv->qmi_client = qmi_client ? g_object_ref (qmi_client) : NULL;
    self->priv->device_open_flags = device_open_flags;

    return self;
}

static void
qfu_reseter_init (QfuReseter *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, QFU_TYPE_RESETER, QfuReseterPrivate);
}

static void
dispose (GObject *object)
{
    QfuReseter *self = QFU_RESETER (object);

    g_clear_object (&self->priv->device_selection);
    g_clear_object (&self->priv->qmi_client);

    G_OBJECT_CLASS (qfu_reseter_parent_class)->dispose (object);
}

static void
qfu_reseter_class_init (QfuReseterClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (object_class, sizeof (QfuReseterPrivate));

    object_class->dispose = dispose;
}
