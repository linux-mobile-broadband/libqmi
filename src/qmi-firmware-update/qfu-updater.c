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
 * Copyright (C) 2016 Aleksander Morgado <aleksander@aleksander.es>
 */

#include <config.h>
#include <string.h>

#include <gio/gio.h>
#include <gudev/gudev.h>

#include <libqmi-glib.h>

#include "qfu-log.h"
#include "qfu-image-factory.h"
#include "qfu-updater.h"
#include "qfu-reseter.h"
#include "qfu-utils.h"
#include "qfu-udev-helpers.h"
#include "qfu-qdl-device.h"
#include "qfu-enum-types.h"

G_DEFINE_TYPE (QfuUpdater, qfu_updater, G_TYPE_OBJECT)

#define CLEAR_LINE "\33[2K\r"

typedef enum {
    UPDATER_TYPE_UNKNOWN,
    UPDATER_TYPE_GENERIC,
    UPDATER_TYPE_QDL,
} UpdaterType;

struct _QfuUpdaterPrivate {
    UpdaterType         type;
    QfuDeviceSelection *device_selection;
    gchar              *firmware_version;
    gchar              *config_version;
    gchar              *carrier;
    gboolean            device_open_proxy;
    gboolean            device_open_mbim;
    gboolean            force;
};

/******************************************************************************/
/* Run */

typedef enum {
    RUN_CONTEXT_STEP_QMI_CLIENT,
    RUN_CONTEXT_STEP_GET_FIRMWARE_PREFERENCE,
    RUN_CONTEXT_STEP_SET_FIRMWARE_PREFERENCE,
    RUN_CONTEXT_STEP_OFFLINE,
    RUN_CONTEXT_STEP_RESET,
    RUN_CONTEXT_STEP_CLEANUP_QMI_DEVICE,
    RUN_CONTEXT_STEP_WAIT_FOR_TTY,
    RUN_CONTEXT_STEP_QDL_DEVICE,
    RUN_CONTEXT_STEP_SELECT_IMAGE,
    RUN_CONTEXT_STEP_DOWNLOAD_IMAGE,
    RUN_CONTEXT_STEP_CLEANUP_IMAGE,
    RUN_CONTEXT_STEP_CLEANUP_QDL_DEVICE,
    RUN_CONTEXT_STEP_WAIT_FOR_CDC_WDM,
    RUN_CONTEXT_STEP_LAST
} RunContextStep;

typedef struct {
    /* Device selection */
    GFile *cdc_wdm_file;
    GFile *serial_file;

    /* Context step */
    RunContextStep step;

    /* List of pending QfuImages to download, and the current one being
     * processed. */
    GList    *pending_images;
    QfuImage *current_image;

    /* QMI device and client */
    QmiDevice    *qmi_device;
    QmiClientDms *qmi_client;

    /* QDL device */
    QfuQdlDevice *qdl_device;

    /* Reset configuration */
    gboolean boothold_reset;

    /* Information gathered from the firmware images themselves */
    gchar *firmware_version;
    gchar *config_version;
    gchar *carrier;
} RunContext;

static void
run_context_free (RunContext *ctx)
{
    g_free (ctx->firmware_version);
    g_free (ctx->config_version);
    g_free (ctx->carrier);
    if (ctx->qdl_device)
        g_object_unref (&ctx->qdl_device);
    if (ctx->qmi_client) {
        g_assert (ctx->qmi_device);
        qmi_device_release_client (ctx->qmi_device,
                                   QMI_CLIENT (ctx->qmi_client),
                                   QMI_DEVICE_RELEASE_CLIENT_FLAGS_RELEASE_CID,
                                   10, NULL, NULL, NULL);
        g_object_unref (ctx->qmi_client);
    }
    if (ctx->qmi_device) {
        qmi_device_close (ctx->qmi_device, NULL);
        g_object_unref (ctx->qmi_device);
    }
    if (ctx->current_image)
        g_object_unref (ctx->current_image);
    g_list_free_full (ctx->pending_images, (GDestroyNotify) g_object_unref);
    if (ctx->serial_file)
        g_object_unref (ctx->serial_file);
    if (ctx->cdc_wdm_file)
        g_object_unref (ctx->cdc_wdm_file);
    g_slice_free (RunContext, ctx);
}

gboolean
qfu_updater_run_finish (QfuUpdater    *self,
                        GAsyncResult  *res,
                        GError       **error)
{
    return g_task_propagate_boolean (G_TASK (res), error);
}

static void run_context_step (GTask *task);

static gboolean
run_context_step_cb (GTask *task)
{
    run_context_step (task);
    return FALSE;
}

static void
run_context_step_next (GTask *task, RunContextStep next)
{
    RunContext *ctx;

    ctx = (RunContext *) g_task_get_task_data (task);
    ctx->step = next;

    /* Schedule next step in an idle */
    g_idle_add ((GSourceFunc) run_context_step_cb, task);
}

static void
wait_for_cdc_wdm_ready (QfuDeviceSelection *device_selection,
                        GAsyncResult       *res,
                        GTask              *task)
{
    GError     *error = NULL;
    RunContext *ctx;
    gchar      *path;

    ctx = (RunContext *) g_task_get_task_data (task);

    g_assert (!ctx->cdc_wdm_file);
    ctx->cdc_wdm_file = qfu_device_selection_wait_for_cdc_wdm_finish (device_selection, res, &error);
    if (!ctx->cdc_wdm_file) {
        g_prefix_error (&error, "error waiting for cdc-wdm: ");
        g_task_return_error (task, error);
        g_object_unref (task);
        return;
    }

    path = g_file_get_path (ctx->cdc_wdm_file);
    g_debug ("[qfu-updater] cdc-wdm device found: %s", path);
    g_free (path);

    /* Go on */
    run_context_step_next (task, ctx->step + 1);
}

static void
run_context_step_wait_for_cdc_wdm (GTask *task)
{
    QfuUpdater *self;

    self = g_task_get_source_object (task);

    g_debug ("[qfu-updater] now waiting for cdc-wdm device...");

    qfu_device_selection_wait_for_cdc_wdm (self->priv->device_selection,
                                           g_task_get_cancellable (task),
                                           (GAsyncReadyCallback) wait_for_cdc_wdm_ready,
                                           task);
}

static void
run_context_step_cleanup_qdl_device (GTask *task)
{
    RunContext *ctx;
    QfuUpdater *self;

    ctx = (RunContext *) g_task_get_task_data (task);
    self = g_task_get_source_object (task);

    g_assert (ctx->qdl_device);
    g_assert (ctx->serial_file);

    g_debug ("[qfu-updater] QDL reset");
    qfu_qdl_device_reset (ctx->qdl_device, g_task_get_cancellable (task), NULL);
    g_clear_object (&ctx->qdl_device);
    g_clear_object (&ctx->serial_file);

    g_print ("rebooting in normal mode...\n");

    /* If we were running in QDL mode, we don't even wait for the reboot to finish */
    if (self->priv->type == UPDATER_TYPE_QDL) {
        run_context_step_next (task, RUN_CONTEXT_STEP_LAST);
        return;
    }

    run_context_step_next (task, ctx->step + 1);
}

static void
run_context_step_cleanup_image (GTask *task)
{
    RunContext *ctx;

    ctx = (RunContext *) g_task_get_task_data (task);

    g_assert (ctx->current_image);
    g_clear_object (&ctx->current_image);

    /* Select next image */
    if (ctx->pending_images) {
        run_context_step_next (task, RUN_CONTEXT_STEP_SELECT_IMAGE);
        return;
    }

    /* If no more files to download, we're done! */
    g_debug ("[qfu-updater] no more files to download");
    run_context_step_next (task, ctx->step + 1);
}

static const gchar *progress[] = {
    "(*-----)",
    "(-*----)",
    "(--*---)",
    "(---*--)",
    "(----*-)",
    "(-----*)",
    "(----*-)",
    "(---*--)",
    "(--*---)",
    "(-*----)"
};

static void
run_context_step_download_image (GTask *task)
{
    RunContext   *ctx;
    guint16       sequence;
    guint16       n_chunks;
    GError       *error = NULL;
    GCancellable *cancellable;
    GTimer       *timer;
    gdouble       elapsed;
    gchar        *aux;

    ctx = (RunContext *) g_task_get_task_data (task);
    cancellable = g_task_get_cancellable (task);

    timer = g_timer_new ();

    aux = g_format_size ((guint64) qfu_image_get_size (ctx->current_image));
    g_print ("downloading %s image: %s (%s)...\n",
             qfu_image_type_get_string (qfu_image_get_image_type (ctx->current_image)),
             qfu_image_get_display_name (ctx->current_image),
             aux);
    g_free (aux);

    if (!qfu_qdl_device_hello (ctx->qdl_device, cancellable, &error)) {
        g_prefix_error (&error, "couldn't send greetings to device: ");
        goto out;
    }

    if (!qfu_qdl_device_ufopen (ctx->qdl_device, ctx->current_image, cancellable, &error)) {
        g_prefix_error (&error, "couldn't open session: ");
        goto out;
    }

    n_chunks = qfu_image_get_n_data_chunks (ctx->current_image);
    for (sequence = 0; sequence < n_chunks; sequence++) {
        if (!qfu_log_get_verbose ()) {
            /* Use n-1 chunks for progress reporting; because the last one will take
             * a lot longer. */
            if (n_chunks > 1 && sequence < (n_chunks - 1))
                g_print (CLEAR_LINE "%s %04.1lf%%",
                         progress[sequence % G_N_ELEMENTS (progress)],
                         100.0 * ((gdouble) sequence / (gdouble) (n_chunks - 1)));
            else if (sequence == (n_chunks - 1))
                g_print (CLEAR_LINE "finalizing download...\n");
        }
        if (!qfu_qdl_device_ufwrite (ctx->qdl_device, ctx->current_image, sequence, cancellable, &error)) {
            g_prefix_error (&error, "couldn't write in session: ");
            goto out;
        }
    }

    g_debug ("[qfu-updater] all chunks ack-ed");

    if (!qfu_qdl_device_ufclose (ctx->qdl_device, cancellable, &error)) {
        g_prefix_error (&error, "couldn't close session: ");
        goto out;
    }

out:
    elapsed = g_timer_elapsed (timer, NULL);

    g_timer_destroy (timer);

    if (error) {
        g_prefix_error (&error, "error downloading image: ");
        g_task_return_error (task, error);
        return;
    }

    aux = g_format_size ((guint64) ((qfu_image_get_size (ctx->current_image)) / elapsed));
    g_print ("successfully downloaded in %.2lfs (%s/s)\n", elapsed, aux);
    g_free (aux);

    /* Go on */
    run_context_step_next (task, ctx->step + 1);
}

static void
run_context_step_select_image (GTask *task)
{
    RunContext *ctx;

    ctx = (RunContext *) g_task_get_task_data (task);

    g_assert (!ctx->current_image);
    g_assert (ctx->pending_images);

    /* Select new current image */
    ctx->current_image = QFU_IMAGE (ctx->pending_images->data);
    ctx->pending_images = g_list_delete_link (ctx->pending_images, ctx->pending_images);

    g_debug ("[qfu-updater] selected file '%s' (%" G_GOFFSET_FORMAT " bytes)",
             qfu_image_get_display_name (ctx->current_image),
             qfu_image_get_size (ctx->current_image));

    /* Go on */
    run_context_step_next (task, ctx->step + 1);
}

static void
run_context_step_qdl_device (GTask *task)
{
    RunContext *ctx;
    GError     *error = NULL;

    ctx = (RunContext *) g_task_get_task_data (task);

    g_assert (ctx->serial_file);
    g_assert (!ctx->qdl_device);
    ctx->qdl_device = qfu_qdl_device_new (ctx->serial_file, g_task_get_cancellable (task), &error);
    if (!ctx->qdl_device) {
        g_prefix_error (&error, "error creating device: ");
        g_task_return_error (task, error);
        g_object_unref (task);
        return;
    }

    run_context_step_next (task, ctx->step + 1);
}

static void
wait_for_tty_ready (QfuDeviceSelection *device_selection,
                    GAsyncResult       *res,
                    GTask              *task)
{
    GError     *error = NULL;
    RunContext *ctx;
    gchar      *path;

    ctx = (RunContext *) g_task_get_task_data (task);

    g_assert (!ctx->serial_file);
    ctx->serial_file = qfu_device_selection_wait_for_tty_finish (device_selection, res, &error);
    if (!ctx->serial_file) {
        g_prefix_error (&error, "error waiting for TTY: ");
        g_task_return_error (task, error);
        g_object_unref (task);
        return;
    }

    path = g_file_get_path (ctx->serial_file);
    g_debug ("[qfu-updater] TTY device found: %s", path);
    g_free (path);

    /* Go on */
    run_context_step_next (task, ctx->step + 1);
}

static void
run_context_step_wait_for_tty (GTask *task)
{
    QfuUpdater *self;

    self = g_task_get_source_object (task);

    g_print ("rebooting in download mode...\n");

    g_debug ("[qfu-updater] reset requested, now waiting for TTY device...");
    qfu_device_selection_wait_for_tty (self->priv->device_selection,
                                       g_task_get_cancellable (task),
                                       (GAsyncReadyCallback) wait_for_tty_ready,
                                       task);
}

static void
qmi_client_release_ready (QmiDevice    *device,
                          GAsyncResult *res,
                          GTask        *task)
{
    RunContext *ctx;
    GError     *error = NULL;

    ctx = (RunContext *) g_task_get_task_data (task);

    if (!qmi_device_release_client_finish (device, res, &error)) {
        g_debug ("[qfu-updater] error (ignored): couldn't release DMS QMI client: %s", error->message);
        g_clear_error (&error);
    } else
        g_debug ("[qfu-updater] DMS QMI client released");

    if (!qmi_device_close (ctx->qmi_device, &error)) {
        g_debug ("[qfu-updater] error (ignored): couldn't close QMI device: %s", error->message);
        g_clear_error (&error);
    } else
        g_debug ("[qfu-updater] QMI device closed");

    g_clear_object (&ctx->qmi_device);
    g_clear_object (&ctx->cdc_wdm_file);

    /* If nothing to download, we're done */
    if (!ctx->pending_images) {
        run_context_step_next (task, RUN_CONTEXT_STEP_LAST);
        return;
    }

    /* Go on */
    run_context_step_next (task, ctx->step + 1);
}

static void
run_context_step_cleanup_qmi_device (GTask *task)
{
    RunContext *ctx;

    ctx = (RunContext *) g_task_get_task_data (task);

    g_debug ("[qfu-updater] releasing DMS QMI client...");
    qmi_device_release_client (ctx->qmi_device,
                               QMI_CLIENT (ctx->qmi_client),
                               QMI_DEVICE_RELEASE_CLIENT_FLAGS_RELEASE_CID,
                               10,
                               g_task_get_cancellable (task),
                               (GAsyncReadyCallback) qmi_client_release_ready,
                               task);

    g_clear_object (&ctx->qmi_client);
}

static void
reset_or_offline_ready (QmiClientDms *client,
                        GAsyncResult *res,
                        GTask        *task)
{
    QmiMessageDmsSetOperatingModeOutput *output;
    GError                              *error = NULL;
    RunContext                          *ctx;

    ctx = (RunContext *) g_task_get_task_data (task);

    output = qmi_client_dms_set_operating_mode_finish (client, res, &error);
    if (!output) {
        g_prefix_error (&error, "QMI operation failed: couldn't set operating mode: ");
        g_task_return_error (task, error);
        g_object_unref (task);
        return;
    }

    if (!qmi_message_dms_set_operating_mode_output_get_result (output, &error)) {
        g_prefix_error (&error, "couldn't set operating mode: ");
        g_task_return_error (task, error);
        g_object_unref (task);
        qmi_message_dms_set_operating_mode_output_unref (output);
        return;
    }

    g_debug ("[qfu-updater] operating mode set successfully...");

    /* Go on */
    run_context_step_next (task, ctx->step + 1);
}

static void
reseter_run_ready (QfuReseter   *reseter,
                   GAsyncResult *res,
                   GTask        *task)
{
    GError     *error = NULL;
    RunContext *ctx;

    ctx = (RunContext *) g_task_get_task_data (task);

    if (!qfu_reseter_run_finish (reseter, res, &error)) {
        g_prefix_error (&error, "boothold reseter operation failed: ");
        g_task_return_error (task, error);
        g_object_unref (task);
        return;
    }

    g_debug ("[qfu-updater] boothold reset requested successfully...");

    /* Go on */
    run_context_step_next (task, ctx->step + 1);
}

static void
run_context_step_reset (GTask *task)
{
    QfuUpdater                         *self;
    RunContext                         *ctx;
    QmiMessageDmsSetOperatingModeInput *input;
    QfuReseter                         *reseter;

    ctx = (RunContext *) g_task_get_task_data (task);
    self = g_task_get_source_object (task);

    if (!ctx->boothold_reset) {
        g_debug ("[qfu-updater] setting operating mode 'reset'...");
        input = qmi_message_dms_set_operating_mode_input_new ();
        qmi_message_dms_set_operating_mode_input_set_mode (input, QMI_DMS_OPERATING_MODE_RESET, NULL);
        qmi_client_dms_set_operating_mode (ctx->qmi_client,
                                           input,
                                           10,
                                           g_task_get_cancellable (task),
                                           (GAsyncReadyCallback) reset_or_offline_ready,
                                           task);
        qmi_message_dms_set_operating_mode_input_unref (input);
        return;
    }

    /* Boothold reset */
    reseter = qfu_reseter_new (self->priv->device_selection, ctx->qmi_client);
    qfu_reseter_run (reseter,
                     g_task_get_cancellable (task),
                     (GAsyncReadyCallback) reseter_run_ready,
                     task);
    g_object_unref (reseter);
}


static void
run_context_step_offline (GTask *task)
{
    RunContext                         *ctx;
    QmiMessageDmsSetOperatingModeInput *input;

    ctx = (RunContext *) g_task_get_task_data (task);

    g_debug ("[qfu-updater] setting operating mode 'offline'...");
    input = qmi_message_dms_set_operating_mode_input_new ();
    qmi_message_dms_set_operating_mode_input_set_mode (input, QMI_DMS_OPERATING_MODE_OFFLINE, NULL);
    qmi_client_dms_set_operating_mode (ctx->qmi_client,
                                       input,
                                       10,
                                       g_task_get_cancellable (task),
                                       (GAsyncReadyCallback) reset_or_offline_ready,
                                       task);
    qmi_message_dms_set_operating_mode_input_unref (input);
}

static void
set_firmware_preference_ready (QmiClientDms *client,
                               GAsyncResult *res,
                               GTask        *task)
{
    RunContext                               *ctx;
    QmiMessageDmsSetFirmwarePreferenceOutput *output;
    GError                                   *error = NULL;
    GArray                                   *array = NULL;
    guint                                     next_step;

    ctx = (RunContext *) g_task_get_task_data (task);

    output = qmi_client_dms_set_firmware_preference_finish (client, res, &error);
    if (!output) {
        g_prefix_error (&error, "QMI operation failed, couldn't set firmware preference: ");
        g_task_return_error (task, error);
        g_object_unref (task);
        return;
    }

    if (!qmi_message_dms_set_firmware_preference_output_get_result (output, &error)) {
        g_prefix_error (&error, "couldn't set firmware preference: ");
        g_task_return_error (task, error);
        g_object_unref (task);
        qmi_message_dms_set_firmware_preference_output_unref (output);
        return;
    }

    /* We'll go to next step, unless told to finish early */
    next_step = ctx->step + 1;

    /* list images we need to download? */
    if (qmi_message_dms_set_firmware_preference_output_get_image_download_list (output, &array, &error)) {
        if (!array->len) {
            g_print ("device already reports the given firmware/config version: nothing to do\n");
            g_list_free_full (ctx->pending_images, (GDestroyNotify) g_object_unref);
            ctx->pending_images = NULL;
            next_step = RUN_CONTEXT_STEP_CLEANUP_QMI_DEVICE;
        } else {
            GString                 *images = NULL;
            QmiDmsFirmwareImageType  type;
            guint                    i;

            images = g_string_new ("");
            for (i = 0; i < array->len; i++) {
                type = g_array_index (array, QmiDmsFirmwareImageType, i);
                g_string_append (images, qmi_dms_firmware_image_type_get_string (type));
                if (i < array->len -1)
                    g_string_append (images, ", ");
            }

            g_debug ("[qfu-updater] need to download the following images: %s", images->str);
            g_string_free (images, TRUE);
        }
    }

    qmi_message_dms_set_firmware_preference_output_unref (output);

    /* Go on */
    run_context_step_next (task, next_step);
}

static void
run_context_step_set_firmware_preference (GTask *task)
{
    QfuUpdater                                       *self;
    RunContext                                       *ctx;
    QmiMessageDmsSetFirmwarePreferenceInput          *input;
    GArray                                           *array;
    QmiMessageDmsSetFirmwarePreferenceInputListImage  modem_image_id;
    QmiMessageDmsSetFirmwarePreferenceInputListImage  pri_image_id;
    const gchar                                      *firmware_version;
    const gchar                                      *config_version;
    const gchar                                      *carrier;

    ctx = (RunContext *) g_task_get_task_data (task);
    self = g_task_get_source_object (task);

    firmware_version = self->priv->firmware_version ? self->priv->firmware_version : ctx->firmware_version;
    config_version   = self->priv->config_version   ? self->priv->config_version   : ctx->config_version;
    carrier          = self->priv->carrier          ? self->priv->carrier          : ctx->carrier;

    g_assert (firmware_version);
    g_assert (config_version);
    g_assert (carrier);

    g_print ("setting firmware preference:\n");
    g_print ("  firmware version: '%s'\n", firmware_version);
    g_print ("  config version:   '%s'\n", config_version);
    g_print ("  carrier:          '%s'\n", carrier);

    /* Set modem image info */
    modem_image_id.type = QMI_DMS_FIRMWARE_IMAGE_TYPE_MODEM;
    modem_image_id.unique_id = g_array_sized_new (FALSE, TRUE, sizeof (gchar), 16);
    g_array_insert_vals (modem_image_id.unique_id, 0, "?_?", 3);
    g_array_set_size (modem_image_id.unique_id, 16);
    modem_image_id.build_id = g_strdup_printf ("%s_?", firmware_version);

    /* Set pri image info */
    pri_image_id.type = QMI_DMS_FIRMWARE_IMAGE_TYPE_PRI;
    pri_image_id.unique_id = g_array_sized_new (FALSE, TRUE, sizeof (gchar), 16);
    g_array_insert_vals (pri_image_id.unique_id, 0, config_version, strlen (config_version));
    g_array_set_size (pri_image_id.unique_id, 16);
    pri_image_id.build_id = g_strdup_printf ("%s_%s", firmware_version, carrier);

    array = g_array_sized_new (FALSE, FALSE, sizeof (QmiMessageDmsSetFirmwarePreferenceInputListImage), 2);
    g_array_append_val (array, modem_image_id);
    g_array_append_val (array, pri_image_id);

    input = qmi_message_dms_set_firmware_preference_input_new ();
    qmi_message_dms_set_firmware_preference_input_set_list (input, array, NULL);
    g_array_unref (array);

    g_debug ("[qfu-updater] setting firmware preference...");
    g_debug ("[qfu-updater]   modem image: unique id '%.16s', build id '%s'",
             (gchar *) (modem_image_id.unique_id->data), modem_image_id.build_id);
    g_debug ("[qfu-updater]   pri image:   unique id '%.16s', build id '%s'",
             (gchar *) (pri_image_id.unique_id->data), pri_image_id.build_id);

    qmi_client_dms_set_firmware_preference (ctx->qmi_client,
                                            input,
                                            10,
                                            g_task_get_cancellable (task),
                                            (GAsyncReadyCallback) set_firmware_preference_ready,
                                            task);

    g_array_unref (modem_image_id.unique_id);
    g_free        (modem_image_id.build_id);
    g_array_unref (pri_image_id.unique_id);
    g_free        (pri_image_id.build_id);
}

static gboolean
validate_firmware_config_carrier (QfuUpdater  *self,
                                  RunContext  *ctx,
                                  GError     **error)
{
    GList *l;

    /* Try to preload information like firmware/config/carrier from CWE images */
    for (l = ctx->pending_images; l; l = g_list_next (l)) {
        const gchar *firmware_version;
        const gchar *config_version;
        const gchar *carrier;
        QfuImageCwe *image;

        if (!QFU_IS_IMAGE_CWE (l->data))
            continue;

        image = QFU_IMAGE_CWE (l->data);
        firmware_version = qfu_image_cwe_get_parsed_firmware_version (image);
        config_version   = qfu_image_cwe_get_parsed_config_version   (image);
        carrier          = qfu_image_cwe_get_parsed_carrier          (image);

        if (firmware_version) {
            if (!ctx->firmware_version)
                ctx->firmware_version = g_strdup (firmware_version);
            else if (!g_str_equal (firmware_version, ctx->firmware_version)) {
                if (!self->priv->force) {
                    g_set_error (error, G_IO_ERROR, G_IO_ERROR_PERMISSION_DENIED,
                                 "couldn't detect firmware version: "
                                 "firmware version strings don't match on specified images: "
                                 "'%s' != '%s'",
                                 firmware_version, ctx->firmware_version);
                    return FALSE;
                }
                g_warning ("firmware version strings don't match on specified images: "
                           "'%s' != '%s' (IGNORED with --force)",
                           firmware_version, ctx->firmware_version);
            }
        }

        if (config_version) {
            if (!ctx->config_version)
                ctx->config_version = g_strdup (config_version);
            else if (!g_str_equal (config_version, ctx->config_version)) {
                if (!self->priv->force) {
                    g_set_error (error, G_IO_ERROR, G_IO_ERROR_PERMISSION_DENIED,
                                 "couldn't detect config version: "
                                 "config version strings don't match on specified images: "
                                 "'%s' != '%s'",
                                 config_version, ctx->config_version);
                    return FALSE;
                }
                g_warning ("[qfu-updater] config version strings don't match on specified images: "
                           "'%s' != '%s' (IGNORED with --force)",
                           config_version, ctx->config_version);
            }
        }

        if (carrier) {
            if (!ctx->carrier)
                ctx->carrier = g_strdup (carrier);
            else if (!g_str_equal (carrier, ctx->carrier)) {
                if (!self->priv->force) {
                    g_set_error (error, G_IO_ERROR, G_IO_ERROR_PERMISSION_DENIED,
                                 "couldn't detect carrier: "
                                 "carrier strings don't match on specified images: "
                                 "'%s' != '%s'",
                                 carrier, ctx->carrier);
                    return FALSE;
                }
                g_warning ("[qfu-updater] carrier strings don't match on specified images: "
                           "'%s' != '%s' (IGNORED with --force)",
                           carrier, ctx->carrier);
            }
        }
    }

    /* If given firmware version doesn't match the one in the image, error out */
    if (self->priv->firmware_version && (g_strcmp0 (self->priv->firmware_version, ctx->firmware_version) != 0)) {
        if (!self->priv->force) {
            g_set_error (error, G_IO_ERROR, G_IO_ERROR_PERMISSION_DENIED,
                         "error validating firmware version: "
                         "user provided firmware version doesn't match the one in the specified images: "
                         "'%s' != '%s'",
                         self->priv->firmware_version, ctx->firmware_version);
            return FALSE;
        }
        g_warning ("[qfu-updater] user provided firmware version doesn't match the one in the specified images: "
                   "'%s' != '%s' (IGNORED with --force)",
                   self->priv->firmware_version, ctx->firmware_version);
    }

    /* If given config version doesn't match the one in the image, error out */
    if (self->priv->config_version && (g_strcmp0 (self->priv->config_version, ctx->config_version) != 0)) {
        if (!self->priv->force) {
            g_set_error (error, G_IO_ERROR, G_IO_ERROR_PERMISSION_DENIED,
                         "error validating firmware version: "
                         "user provided firmware version doesn't match the one in the specified images: "
                         "'%s' != '%s'",
                         self->priv->config_version, ctx->config_version);
            return FALSE;
        }
        g_warning ("[qfu-updater] user provided config version doesn't match the one in the specified images: "
                   "'%s' != '%s' (IGNORED with --force)",
                   self->priv->firmware_version, ctx->firmware_version);
    }

    /* If given carrier doesn't match the one in the image, error out */
    if (self->priv->carrier && (g_strcmp0 (self->priv->carrier, ctx->carrier) != 0)) {
        if (!self->priv->force) {
            g_set_error (error, G_IO_ERROR, G_IO_ERROR_PERMISSION_DENIED,
                         "error validating carrier: "
                         "user provided carrier doesn't match the one in the specified images: "
                         "'%s' != '%s'",
                         self->priv->carrier, ctx->carrier);
        }
        g_warning ("[qfu-updater] user provided carrier doesn't match the one in the specified images: "
                   "'%s' != '%s' (IGNORED with --force)",
                   self->priv->carrier, ctx->carrier);
        return FALSE;
    }

    /* No firmware version? */
    if (!self->priv->firmware_version && !ctx->firmware_version) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "firmware version required");
        return FALSE;
    }

    /* No config version? */
    if (!self->priv->config_version && !ctx->config_version) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "config version required");
        return FALSE;
    }

    /* No carrier? */
    if (!self->priv->carrier && !ctx->carrier) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "carrier required");
        return FALSE;
    }

    return TRUE;
}

static void
get_firmware_preference_ready (QmiClientDms *client,
                               GAsyncResult *res,
                               GTask        *task)
{
    QfuUpdater                               *self;
    RunContext                               *ctx;
    GError                                   *error = NULL;
    QmiMessageDmsGetFirmwarePreferenceOutput *output;
    GArray                                   *array;
    guint                                     i;

    ctx = (RunContext *) g_task_get_task_data (task);
    self = g_task_get_source_object (task);

    output = qmi_client_dms_get_firmware_preference_finish (client, res, &error);
    if (!output) {
        g_prefix_error (&error, "QMI operation failed: couldn't set operating mode: ");
        g_task_return_error (task, error);
        g_object_unref (task);
        return;
    }

    if (!qmi_message_dms_get_firmware_preference_output_get_result (output, NULL)) {
        qmi_message_dms_get_firmware_preference_output_unref (output);
        /* Firmware preference setting not supported; fail if we got those settings explicitly */
        if (self->priv->firmware_version || self->priv->config_version || self->priv->carrier) {
            g_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_FAILED,
                                     "setting firmware/config/carrier is not supported by this device");
            g_object_unref (task);
            return;
        }

        /* Jump to the reset step and run boothold there */
        ctx->boothold_reset = TRUE;
        run_context_step_next (task, RUN_CONTEXT_STEP_RESET);
        return;
    }

    qmi_message_dms_get_firmware_preference_output_get_list (output, &array, NULL);

    if (array->len > 0) {
        for (i = 0; i < array->len; i++) {
            QmiMessageDmsGetFirmwarePreferenceOutputListImage *image;
            gchar                                             *unique_id_str;

            image = &g_array_index (array, QmiMessageDmsGetFirmwarePreferenceOutputListImage, i);
            unique_id_str = qfu_utils_get_firmware_image_unique_id_printable (image->unique_id);

            g_debug ("[qfu-updater] [image %u]",         i);
            g_debug ("[qfu-updater] \tImage type: '%s'", qmi_dms_firmware_image_type_get_string (image->type));
            g_debug ("[qfu-updater] \tUnique ID:  '%s'", unique_id_str);
            g_debug ("[qfu-updater] \tBuild ID:   '%s'", image->build_id);

            g_free (unique_id_str);
        }
    } else
        g_debug ("[qfu-updater] no images specified");

    qmi_message_dms_get_firmware_preference_output_unref (output);

    /* Firmware preference setting is supported so we require firmware/config/carrier */
    if (!validate_firmware_config_carrier (self, ctx, &error)) {
        g_task_return_error (task, error);
        g_object_unref (task);
        return;
    }

    /* Go on */
    run_context_step_next (task, ctx->step + 1);
}

static void
run_context_step_get_firmware_preference (GTask *task)
{
    RunContext *ctx;

    ctx = (RunContext *) g_task_get_task_data (task);

    g_debug ("[qfu-updater] getting firmware preference...");
    qmi_client_dms_get_firmware_preference (ctx->qmi_client,
                                            NULL,
                                            10,
                                            g_task_get_cancellable (task),
                                            (GAsyncReadyCallback) get_firmware_preference_ready,
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

    g_assert (!ctx->qmi_device);
    g_assert (!ctx->qmi_client);

    if (!qfu_utils_new_client_dms_finish (res,
                                          &ctx->qmi_device,
                                          &ctx->qmi_client,
                                          &error)) {
        g_task_return_error (task, error);
        g_object_unref (task);
        return;
    }

    /* Go on */
    run_context_step_next (task, ctx->step + 1);
}

static void
run_context_step_qmi_client (GTask *task)
{
    RunContext *ctx;
    QfuUpdater *self;

    ctx = (RunContext *) g_task_get_task_data (task);
    self = g_task_get_source_object (task);

    g_debug ("[qfu-updater] creating QMI DMS client...");
    g_assert (ctx->cdc_wdm_file);
    qfu_utils_new_client_dms (ctx->cdc_wdm_file,
                              self->priv->device_open_proxy,
                              self->priv->device_open_mbim,
                              g_task_get_cancellable (task),
                              (GAsyncReadyCallback) new_client_dms_ready,
                              task);
}

typedef void (* RunContextStepFunc) (GTask *task);
static const RunContextStepFunc run_context_step_func[] = {
    [RUN_CONTEXT_STEP_QMI_CLIENT]              = run_context_step_qmi_client,
    [RUN_CONTEXT_STEP_GET_FIRMWARE_PREFERENCE] = run_context_step_get_firmware_preference,
    [RUN_CONTEXT_STEP_SET_FIRMWARE_PREFERENCE] = run_context_step_set_firmware_preference,
    [RUN_CONTEXT_STEP_OFFLINE]                 = run_context_step_offline,
    [RUN_CONTEXT_STEP_RESET]                   = run_context_step_reset,
    [RUN_CONTEXT_STEP_CLEANUP_QMI_DEVICE]      = run_context_step_cleanup_qmi_device,
    [RUN_CONTEXT_STEP_WAIT_FOR_TTY]            = run_context_step_wait_for_tty,
    [RUN_CONTEXT_STEP_QDL_DEVICE]              = run_context_step_qdl_device,
    [RUN_CONTEXT_STEP_SELECT_IMAGE]            = run_context_step_select_image,
    [RUN_CONTEXT_STEP_DOWNLOAD_IMAGE]          = run_context_step_download_image,
    [RUN_CONTEXT_STEP_CLEANUP_IMAGE]           = run_context_step_cleanup_image,
    [RUN_CONTEXT_STEP_CLEANUP_QDL_DEVICE]      = run_context_step_cleanup_qdl_device,
    [RUN_CONTEXT_STEP_WAIT_FOR_CDC_WDM]        = run_context_step_wait_for_cdc_wdm,
};

G_STATIC_ASSERT (G_N_ELEMENTS (run_context_step_func) == RUN_CONTEXT_STEP_LAST);

static void
run_context_step (GTask *task)
{
    RunContext *ctx;

    ctx = (RunContext *) g_task_get_task_data (task);

    /* Early halt operation if cancelled */
    if (g_task_return_error_if_cancelled (task)) {
        g_object_unref (task);
        return;
    }

    if (ctx->step < G_N_ELEMENTS (run_context_step_func)) {
        run_context_step_func [ctx->step] (task);
        return;
    }

    g_debug ("[qfu-updater] operation finished");
    g_task_return_boolean (task, TRUE);
    g_object_unref (task);
}

static gint
image_sort_by_size (QfuImage *a, QfuImage *b)
{
    return qfu_image_get_size (b) - qfu_image_get_size (a);
}

static gboolean
preload_images (RunContext    *ctx,
                GList         *image_file_list,
                GCancellable  *cancellable,
                GError       **error)
{
    GList *l;

    /* Build QfuImage objects for each image file given */
    for (l = image_file_list; l; l = g_list_next (l)) {
        QfuImage *image;

        image = qfu_image_factory_build (G_FILE (l->data), cancellable, error);
        if (!image)
            return FALSE;

        ctx->pending_images = g_list_append (ctx->pending_images, image);
    }

    /* Sort by size, we want to download bigger images first, as that is usually
     * the use case anyway, first flash e.g. the .cwe file, then the .nvu one. */
    ctx->pending_images = g_list_sort (ctx->pending_images, (GCompareFunc) image_sort_by_size);

    return TRUE;
}

void
qfu_updater_run (QfuUpdater          *self,
                 GList               *image_file_list,
                 GCancellable        *cancellable,
                 GAsyncReadyCallback  callback,
                 gpointer             user_data)
{
    RunContext *ctx;
    GTask      *task;
    GError     *error = NULL;

    g_assert (image_file_list);

    ctx = g_slice_new0 (RunContext);

    task = g_task_new (self, cancellable, callback, user_data);
    g_task_set_task_data (task, ctx, (GDestroyNotify) run_context_free);

    if (!preload_images (ctx, image_file_list, cancellable, &error)) {
        g_task_return_error (task, error);
        g_object_unref (task);
        return;
    }

    switch (self->priv->type) {
    case UPDATER_TYPE_GENERIC:
        ctx->step = RUN_CONTEXT_STEP_QMI_CLIENT;
        ctx->cdc_wdm_file = qfu_device_selection_get_single_cdc_wdm (self->priv->device_selection);
        if (!ctx->cdc_wdm_file) {
            g_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT,
                                     "No cdc-wdm device found to run update operation");
            g_object_unref (task);
            return;
        }
        break;
    case UPDATER_TYPE_QDL:
        ctx->step = RUN_CONTEXT_STEP_QDL_DEVICE;
        ctx->serial_file = qfu_device_selection_get_single_tty (self->priv->device_selection);
        if (!ctx->serial_file) {
            g_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT,
                                     "No serial device found to run QDL update operation");
            g_object_unref (task);
            return;
        }
        break;
    default:
        g_assert_not_reached ();
    }

    run_context_step (task);
}

/******************************************************************************/

QfuUpdater *
qfu_updater_new (QfuDeviceSelection *device_selection,
                 const gchar        *firmware_version,
                 const gchar        *config_version,
                 const gchar        *carrier,
                 gboolean            device_open_proxy,
                 gboolean            device_open_mbim,
                 gboolean            force)
{
    QfuUpdater *self;

    g_assert (QFU_IS_DEVICE_SELECTION (device_selection));

    self = g_object_new (QFU_TYPE_UPDATER, NULL);
    self->priv->type = UPDATER_TYPE_GENERIC;
    self->priv->device_selection = g_object_ref (device_selection);
    self->priv->device_open_proxy = device_open_proxy;
    self->priv->device_open_mbim = device_open_mbim;
    self->priv->firmware_version = (firmware_version ? g_strdup (firmware_version) : NULL);
    self->priv->config_version = (config_version ? g_strdup (config_version) : NULL);
    self->priv->carrier = (carrier ? g_strdup (carrier) : NULL);
    self->priv->force = force;

    return self;
}

QfuUpdater *
qfu_updater_new_qdl (QfuDeviceSelection *device_selection)
{
    QfuUpdater *self;

    g_assert (QFU_IS_DEVICE_SELECTION (device_selection));

    self = g_object_new (QFU_TYPE_UPDATER, NULL);
    self->priv->type = UPDATER_TYPE_QDL;
    self->priv->device_selection = g_object_ref (device_selection);

    return self;
}

static void
qfu_updater_init (QfuUpdater *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, QFU_TYPE_UPDATER, QfuUpdaterPrivate);
    self->priv->type = UPDATER_TYPE_UNKNOWN;
}

static void
dispose (GObject *object)
{
    QfuUpdater *self = QFU_UPDATER (object);

    g_clear_object (&self->priv->device_selection);

    G_OBJECT_CLASS (qfu_updater_parent_class)->dispose (object);
}

static void
finalize (GObject *object)
{
    QfuUpdater *self = QFU_UPDATER (object);

    g_free (self->priv->firmware_version);
    g_free (self->priv->config_version);
    g_free (self->priv->carrier);

    G_OBJECT_CLASS (qfu_updater_parent_class)->finalize (object);
}

static void
qfu_updater_class_init (QfuUpdaterClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (object_class, sizeof (QfuUpdaterPrivate));

    object_class->dispose = dispose;
    object_class->finalize = finalize;
}
