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

#include "qfu-updater.h"
#include "qfu-udev-helpers.h"

G_DEFINE_TYPE (QfuUpdater, qfu_updater, G_TYPE_OBJECT)

struct _QfuUpdaterPrivate {
    /* Inputs */
    GFile    *cdc_wdm_file;
    gchar    *firmware_version;
    gchar    *config_version;
    gchar    *carrier;
    GList    *image_file_list;
    gboolean  device_open_proxy;
    gboolean  device_open_mbim;
};

static const gchar *cdc_wdm_subsys[] = { "usbmisc", "usb", NULL };

/******************************************************************************/
/* Run */

typedef enum {
    RUN_CONTEXT_STEP_USB_INFO = 0,
    RUN_CONTEXT_STEP_SELECT_IMAGE,
    RUN_CONTEXT_STEP_QMI_DEVICE,
    RUN_CONTEXT_STEP_QMI_DEVICE_OPEN,
    RUN_CONTEXT_STEP_QMI_CLIENT,
    RUN_CONTEXT_STEP_FIRMWARE_PREFERENCE,
    RUN_CONTEXT_STEP_OFFLINE,
    RUN_CONTEXT_STEP_RESET,
    RUN_CONTEXT_STEP_CLEANUP_QMI_DEVICE,
    RUN_CONTEXT_STEP_WAIT_FOR_TTY,
    RUN_CONTEXT_STEP_CLEANUP_IMAGE,
    RUN_CONTEXT_STEP_LAST
} RunContextStep;

typedef struct {
    /* Context step */
    RunContextStep step;
    /* List of pending image files to download */
    GList *pending_images;
    /* Current image being downloaded */
    GFile     *current_image;
    GFileInfo *current_image_info;
    /* USB info */
    gchar *sysfs_path;
    /* QMI device and client */
    QmiDevice    *qmi_device;
    QmiClientDms *qmi_client;
    /* TTY file */
    GFile *tty;
} RunContext;

static void
run_context_free (RunContext *ctx)
{
    if (ctx->tty)
        g_object_unref (ctx->tty);
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
    if (ctx->current_image_info)
        g_object_unref (ctx->current_image_info);
    if (ctx->current_image)
        g_object_unref (ctx->current_image);
    g_list_free_full (ctx->pending_images, (GDestroyNotify) g_object_unref);
    g_free (ctx->sysfs_path);
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
run_context_step_cleanup_image (GTask *task)
{
    RunContext *ctx;

    ctx = (RunContext *) g_task_get_task_data (task);

    g_assert (ctx->current_image);
    g_assert (ctx->current_image_info);

    g_clear_object (&ctx->current_image);
    g_clear_object (&ctx->current_image_info);

    /* Select next image */
    run_context_step_next (task, RUN_CONTEXT_STEP_SELECT_IMAGE);
}

static void
wait_for_tty_ready (gpointer      unused,
                    GAsyncResult *res,
                    GTask        *task)
{
    GError     *error = NULL;
    RunContext *ctx;
    gchar      *tty_path;

    ctx = (RunContext *) g_task_get_task_data (task);

    ctx->tty = qfu_udev_helper_wait_for_tty_finish (res, &error);
    if (!ctx->tty) {
        g_prefix_error (&error, "error waiting for TTY: ");
        g_task_return_error (task, error);
        g_object_unref (task);
        return;
    }

    tty_path = g_file_get_path (ctx->tty);
    g_debug ("[qfu-updater] TTY device found: %s", tty_path);
    g_free (tty_path);

    /* Go on */
    run_context_step_next (task, ctx->step + 1);
}

static void
run_context_step_wait_for_tty (GTask *task)
{
    RunContext *ctx;

    ctx = (RunContext *) g_task_get_task_data (task);

    g_debug ("[qfu-updater] reset requested, now waiting for TTY device...");
    qfu_udev_helper_wait_for_tty (ctx->sysfs_path,
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
        g_prefix_error (&error, "couldn't release DMS QMI client: ");
        g_task_return_error (task, error);
        g_object_unref (task);
        return;
    }

    g_debug ("[qfu-updater] DMS QMI client released");

    if (!qmi_device_close (ctx->qmi_device, &error)) {
        g_prefix_error (&error, "couldn't close QMI device: ");
        g_task_return_error (task, error);
        g_object_unref (task);
        return;
    }

    g_debug ("[qfu-updater] QMI device closed");

    g_clear_object (&ctx->qmi_device);

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
run_context_step_reset (GTask *task)
{
    RunContext                         *ctx;
    QmiMessageDmsSetOperatingModeInput *input;

    ctx = (RunContext *) g_task_get_task_data (task);

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

    /* list images we need to download? */
    if (qmi_message_dms_set_firmware_preference_output_get_image_download_list (output, &array, &error)) {
        if (!array->len) {
            g_debug ("[qfu-updater] no more images needed to download");
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
    run_context_step_next (task, ctx->step + 1);
}

static void
run_context_step_firmware_preference (GTask *task)
{
    QfuUpdater                                       *self;
    RunContext                                       *ctx;
    QmiMessageDmsSetFirmwarePreferenceInput          *input;
    GArray                                           *array;
    QmiMessageDmsSetFirmwarePreferenceInputListImage  modem_image_id;
    QmiMessageDmsSetFirmwarePreferenceInputListImage  pri_image_id;

    ctx = (RunContext *) g_task_get_task_data (task);
    self = g_task_get_source_object (task);

    /* Set modem image info */
    modem_image_id.type = QMI_DMS_FIRMWARE_IMAGE_TYPE_MODEM;
    modem_image_id.unique_id = g_array_sized_new (FALSE, TRUE, sizeof (gchar), 16);
    g_array_insert_vals (modem_image_id.unique_id, 0, "?_?", 3);
    g_array_set_size (modem_image_id.unique_id, 16);
    modem_image_id.build_id = g_strdup_printf ("%s_?", self->priv->firmware_version);

    /* Set pri image info */
    pri_image_id.type = QMI_DMS_FIRMWARE_IMAGE_TYPE_PRI;
    pri_image_id.unique_id = g_array_sized_new (FALSE, TRUE, sizeof (gchar), 16);
    g_array_insert_vals (pri_image_id.unique_id, 0, self->priv->config_version, strlen (self->priv->config_version));
    g_array_set_size (pri_image_id.unique_id, 16);
    pri_image_id.build_id = g_strdup_printf ("%s_%s", self->priv->firmware_version, self->priv->carrier);

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

static void
qmi_client_ready (QmiDevice    *device,
                  GAsyncResult *res,
                  GTask        *task)
{
    RunContext *ctx;
    GError     *error = NULL;

    ctx = (RunContext *) g_task_get_task_data (task);

    ctx->qmi_client = QMI_CLIENT_DMS (qmi_device_allocate_client_finish (device, res, &error));
    if (!ctx->qmi_client) {
        g_prefix_error (&error, "couldn't allocate DMS QMI client: ");
        g_task_return_error (task, error);
        g_object_unref (task);
        return;
    }

    g_debug ("[qfu-updater] DMS QMI client allocated");

    /* Go on */
    run_context_step_next (task, ctx->step + 1);
}

static void
run_context_step_qmi_client (GTask *task)
{
    RunContext *ctx;

    ctx = (RunContext *) g_task_get_task_data (task);

    g_debug ("[qfu-updater] allocating new DMS QMI client...");
    qmi_device_allocate_client (ctx->qmi_device,
                                QMI_SERVICE_DMS,
                                QMI_CID_NONE,
                                10,
                                g_task_get_cancellable (task),
                                (GAsyncReadyCallback) qmi_client_ready,
                                task);
}

static void
qmi_device_open_ready (QmiDevice    *device,
                       GAsyncResult *res,
                       GTask        *task)
{
    RunContext *ctx;
    GError     *error = NULL;

    ctx = (RunContext *) g_task_get_task_data (task);

    if (!qmi_device_open_finish (device, res, &error)) {
        g_prefix_error (&error, "couldn't open QMI device: ");
        g_task_return_error (task, error);
        g_object_unref (task);
        return;
    }

    g_debug ("[qfu-updater] QMI device open");

    /* Go on */
    run_context_step_next (task, ctx->step + 1);
}

static void
run_context_step_qmi_device_open (GTask *task)
{
    QfuUpdater         *self;
    RunContext         *ctx;
    QmiDeviceOpenFlags  flags = QMI_DEVICE_OPEN_FLAGS_NONE;

    ctx = (RunContext *) g_task_get_task_data (task);
    self = g_task_get_source_object (task);

    if (self->priv->device_open_proxy)
        flags |= QMI_DEVICE_OPEN_FLAGS_PROXY;

    if (self->priv->device_open_mbim)
        flags |= QMI_DEVICE_OPEN_FLAGS_MBIM;

    g_debug ("[qfu-updater] opening QMI device...");
    qmi_device_open (ctx->qmi_device,
                     flags,
                     20,
                     g_task_get_cancellable (task),
                     (GAsyncReadyCallback) qmi_device_open_ready,
                     task);
}

static void
qmi_device_ready (GObject      *source,
                  GAsyncResult *res,
                  GTask        *task)
{
    RunContext *ctx;
    GError     *error = NULL;

    ctx = (RunContext *) g_task_get_task_data (task);

    ctx->qmi_device = qmi_device_new_finish (res, &error);
    if (!ctx->qmi_device) {
        g_prefix_error (&error, "couldn't create QMI device: ");
        g_task_return_error (task, error);
        g_object_unref (task);
        return;
    }

    g_debug ("[qfu-updater] QMI device created");

    /* Go on */
    run_context_step_next (task, ctx->step + 1);
}

static void
run_context_step_qmi_device (GTask *task)
{
    QfuUpdater *self;

    self = g_task_get_source_object (task);

    g_debug ("[qfu-updater] creating QMI device...");
    qmi_device_new (self->priv->cdc_wdm_file,
                    g_task_get_cancellable (task),
                    (GAsyncReadyCallback) qmi_device_ready,
                    task);
}

static void
run_context_step_select_image (GTask *task)
{
    RunContext *ctx;
    GError     *error = NULL;

    ctx = (RunContext *) g_task_get_task_data (task);

    g_assert (!ctx->current_image);
    g_assert (!ctx->current_image_info);

    /* If no more files to download, we're done! */
    if (!ctx->pending_images) {
        g_debug ("[qfu-updater] no more files to download");
        run_context_step_next (task, RUN_CONTEXT_STEP_LAST);
        return;
    }

    /* Select new current image */
    ctx->current_image = G_FILE (ctx->pending_images->data);
    ctx->pending_images = g_list_delete_link (ctx->pending_images, ctx->pending_images);
    ctx->current_image_info = g_file_query_info (ctx->current_image,
                                                 G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME "," G_FILE_ATTRIBUTE_STANDARD_SIZE,
                                                 G_FILE_QUERY_INFO_NONE,
                                                 g_task_get_cancellable (task),
                                                 &error);
    if (!ctx->current_image_info) {
        g_prefix_error (&error, "couldn't get image file info: ");
        g_task_return_error (task, error);
        g_object_unref (task);
        return;
    }

    g_debug ("[qfu-updater] selected file '%s' (%" G_GOFFSET_FORMAT " bytes)",
             g_file_info_get_display_name (ctx->current_image_info),
             g_file_info_get_size (ctx->current_image_info));

    /* Go on */
    run_context_step_next (task, ctx->step + 1);
}

static void
run_context_step_usb_info (GTask *task)
{
    QfuUpdater *self;
    RunContext *ctx;
    GError     *error = NULL;

    ctx = (RunContext *) g_task_get_task_data (task);
    self = g_task_get_source_object (task);

    g_debug ("[qfu-updater] looking for device sysfs path...");
    ctx->sysfs_path = qfu_udev_helper_get_sysfs_path (self->priv->cdc_wdm_file, cdc_wdm_subsys, &error);
    if (!ctx->sysfs_path) {
        g_prefix_error (&error, "couldn't get cdc-wdm device sysfs path: ");
        g_task_return_error (task, error);
        g_object_unref (task);
        return;
    }
    g_debug ("[qfu-updater] device sysfs path: %s", ctx->sysfs_path);

    run_context_step_next (task, ctx->step + 1);
}

typedef void (* RunContextStepFunc) (GTask *task);
static const RunContextStepFunc run_context_step_func[] = {
    [RUN_CONTEXT_STEP_USB_INFO]            = run_context_step_usb_info,
    [RUN_CONTEXT_STEP_SELECT_IMAGE]        = run_context_step_select_image,
    [RUN_CONTEXT_STEP_QMI_DEVICE]          = run_context_step_qmi_device,
    [RUN_CONTEXT_STEP_QMI_DEVICE_OPEN]     = run_context_step_qmi_device_open,
    [RUN_CONTEXT_STEP_QMI_CLIENT]          = run_context_step_qmi_client,
    [RUN_CONTEXT_STEP_FIRMWARE_PREFERENCE] = run_context_step_firmware_preference,
    [RUN_CONTEXT_STEP_OFFLINE]             = run_context_step_offline,
    [RUN_CONTEXT_STEP_RESET]               = run_context_step_reset,
    [RUN_CONTEXT_STEP_CLEANUP_QMI_DEVICE]  = run_context_step_cleanup_qmi_device,
    [RUN_CONTEXT_STEP_WAIT_FOR_TTY]        = run_context_step_wait_for_tty,
    [RUN_CONTEXT_STEP_CLEANUP_IMAGE]       = run_context_step_cleanup_image,
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

void
qfu_updater_run (QfuUpdater          *self,
                 GCancellable        *cancellable,
                 GAsyncReadyCallback  callback,
                 gpointer             user_data)
{
    RunContext  *ctx;
    GTask       *task;

    ctx = g_slice_new0 (RunContext);
    ctx->pending_images = g_list_copy_deep (self->priv->image_file_list, (GCopyFunc) g_object_ref, NULL);

    task = g_task_new (self, cancellable, callback, user_data);
    g_task_set_task_data (task, ctx, (GDestroyNotify) run_context_free);

    run_context_step (task);
}

/******************************************************************************/

QfuUpdater *
qfu_updater_new (GFile       *cdc_wdm_file,
                 const gchar *firmware_version,
                 const gchar *config_version,
                 const gchar *carrier,
                 GList       *image_file_list,
                 gboolean     device_open_proxy,
                 gboolean     device_open_mbim)
{
    QfuUpdater *self;

    g_assert (G_IS_FILE (cdc_wdm_file));
    g_assert (image_file_list);

    self = g_object_new (QFU_TYPE_UPDATER, NULL);
    self->priv->cdc_wdm_file = g_object_ref (cdc_wdm_file);
    self->priv->device_open_proxy = device_open_proxy;
    self->priv->device_open_mbim = device_open_mbim;
    self->priv->firmware_version = g_strdup (firmware_version);
    self->priv->config_version = g_strdup (config_version);
    self->priv->carrier = g_strdup (carrier);
    self->priv->image_file_list = g_list_copy_deep (image_file_list, (GCopyFunc) g_object_ref, NULL);

    return self;
}

static void
qfu_updater_init (QfuUpdater *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, QFU_TYPE_UPDATER, QfuUpdaterPrivate);
}

static void
dispose (GObject *object)
{
    QfuUpdater *self = QFU_UPDATER (object);

    g_clear_object (&self->priv->cdc_wdm_file);
    if (self->priv->image_file_list) {
        g_list_free_full (self->priv->image_file_list, (GDestroyNotify) g_object_unref);
        self->priv->image_file_list = NULL;
    }

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
