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

#include "qfu-image-factory.h"
#include "qfu-updater.h"
#include "qfu-udev-helpers.h"
#include "qfu-qdl-device.h"
#include "qfu-enum-types.h"

G_DEFINE_TYPE (QfuUpdater, qfu_updater, G_TYPE_OBJECT)

typedef enum {
    UPDATER_TYPE_UNKNOWN,
    UPDATER_TYPE_GENERIC,
    UPDATER_TYPE_QDL,
} UpdaterType;

struct _QfuUpdaterPrivate {
    UpdaterType  type;
    GFile       *cdc_wdm_file;
    GFile       *serial_file;
    gchar       *firmware_version;
    gchar       *config_version;
    gchar       *carrier;
    gboolean     device_open_proxy;
    gboolean     device_open_mbim;
};

static const gchar *cdc_wdm_subsys[] = { "usbmisc", "usb", NULL };

/******************************************************************************/
/* Run */

#define QMI_CLIENT_RETRIES 3

typedef enum {
    RUN_CONTEXT_STEP_USB_INFO = 0,
    RUN_CONTEXT_STEP_QMI_DEVICE,
    RUN_CONTEXT_STEP_QMI_DEVICE_OPEN,
    RUN_CONTEXT_STEP_QMI_CLIENT,
    RUN_CONTEXT_STEP_FIRMWARE_PREFERENCE,
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
    /* Device files and common USB sysfs path*/
    GFile *cdc_wdm_file;
    GFile *serial_file;
    gchar *sysfs_path;

    /* Context step */
    RunContextStep step;

    /* List of pending QfuImages to download, and the current one being
     * processed. */
    GList    *pending_images;
    QfuImage *current_image;

    /* QMI device and client */
    QmiDevice    *qmi_device;
    QmiClientDms *qmi_client;
    gint          qmi_client_retries;

    /* QDL device */
    QfuQdlDevice *qdl_device;
} RunContext;

static void
run_context_free (RunContext *ctx)
{
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
    g_free (ctx->sysfs_path);
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
wait_for_cdc_wdm_ready (gpointer      unused,
                        GAsyncResult *res,
                        GTask        *task)
{
    GError     *error = NULL;
    RunContext *ctx;
    gchar      *path;

    ctx = (RunContext *) g_task_get_task_data (task);

    g_assert (!ctx->cdc_wdm_file);
    ctx->cdc_wdm_file = qfu_udev_helper_wait_for_device_finish (res, &error);
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
    RunContext *ctx;

    ctx = (RunContext *) g_task_get_task_data (task);

    g_debug ("[qfu-updater] now waiting for cdc-wdm device...");
    qfu_udev_helper_wait_for_device (QFU_UDEV_HELPER_WAIT_FOR_DEVICE_TYPE_CDC_WDM,
                                     ctx->sysfs_path,
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
wait_for_tty_ready (gpointer      unused,
                    GAsyncResult *res,
                    GTask        *task)
{
    GError     *error = NULL;
    RunContext *ctx;
    gchar      *path;

    ctx = (RunContext *) g_task_get_task_data (task);

    g_assert (!ctx->serial_file);
    ctx->serial_file = qfu_udev_helper_wait_for_device_finish (res, &error);
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
    RunContext *ctx;

    ctx = (RunContext *) g_task_get_task_data (task);

    g_print ("rebooting in download mode...\n");

    g_debug ("[qfu-updater] reset requested, now waiting for TTY device...");
    qfu_udev_helper_wait_for_device (QFU_UDEV_HELPER_WAIT_FOR_DEVICE_TYPE_TTY,
                                     ctx->sysfs_path,
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
    g_clear_object (&ctx->cdc_wdm_file);

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
        /* If this was the last attempt, error out */
        if (!ctx->qmi_client_retries) {
            g_prefix_error (&error, "couldn't allocate DMS QMI client: ");
            g_task_return_error (task, error);
            g_object_unref (task);
            return;
        }

        /* Retry allocation */
        g_debug ("[qfu-updater: DMS QMI client allocation failed: %s", error->message);
        g_error_free (error);

        g_debug ("[qfu-updater: retrying...");
        run_context_step_next (task, ctx->step);
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

    /* Consume one retry attempt */
    ctx->qmi_client_retries--;
    g_assert (ctx->qmi_client_retries >= 0);

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
    g_assert (self->priv->cdc_wdm_file);
    qmi_device_new (self->priv->cdc_wdm_file,
                    g_task_get_cancellable (task),
                    (GAsyncReadyCallback) qmi_device_ready,
                    task);
}

static void
run_context_step_usb_info (GTask *task)
{
    QfuUpdater *self;
    RunContext *ctx;
    GError     *error = NULL;

    ctx = (RunContext *) g_task_get_task_data (task);
    self = g_task_get_source_object (task);

    g_assert (self->priv->cdc_wdm_file);

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
    [RUN_CONTEXT_STEP_QMI_DEVICE]          = run_context_step_qmi_device,
    [RUN_CONTEXT_STEP_QMI_DEVICE_OPEN]     = run_context_step_qmi_device_open,
    [RUN_CONTEXT_STEP_QMI_CLIENT]          = run_context_step_qmi_client,
    [RUN_CONTEXT_STEP_FIRMWARE_PREFERENCE] = run_context_step_firmware_preference,
    [RUN_CONTEXT_STEP_OFFLINE]             = run_context_step_offline,
    [RUN_CONTEXT_STEP_RESET]               = run_context_step_reset,
    [RUN_CONTEXT_STEP_CLEANUP_QMI_DEVICE]  = run_context_step_cleanup_qmi_device,
    [RUN_CONTEXT_STEP_WAIT_FOR_TTY]        = run_context_step_wait_for_tty,
    [RUN_CONTEXT_STEP_QDL_DEVICE]          = run_context_step_qdl_device,
    [RUN_CONTEXT_STEP_SELECT_IMAGE]        = run_context_step_select_image,
    [RUN_CONTEXT_STEP_DOWNLOAD_IMAGE]      = run_context_step_download_image,
    [RUN_CONTEXT_STEP_CLEANUP_IMAGE]       = run_context_step_cleanup_image,
    [RUN_CONTEXT_STEP_CLEANUP_QDL_DEVICE]  = run_context_step_cleanup_qdl_device,
    [RUN_CONTEXT_STEP_WAIT_FOR_CDC_WDM]    = run_context_step_wait_for_cdc_wdm,
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

void
qfu_updater_run (QfuUpdater          *self,
                 GList               *image_file_list,
                 GCancellable        *cancellable,
                 GAsyncReadyCallback  callback,
                 gpointer             user_data)
{
    RunContext  *ctx;
    GTask       *task;
    GList       *l;

    g_assert (image_file_list);

    ctx = g_slice_new0 (RunContext);

    task = g_task_new (self, cancellable, callback, user_data);
    g_task_set_task_data (task, ctx, (GDestroyNotify) run_context_free);

    /* Build QfuImage objects for each image file given */
    for (l = image_file_list; l; l = g_list_next (l)) {
        GError   *error = NULL;
        QfuImage *image;

        image = qfu_image_factory_build (G_FILE (l->data), cancellable, &error);
        if (!image) {
            g_task_return_error (task, error);
            g_object_unref (task);
            return;
        }
        ctx->pending_images = g_list_append (ctx->pending_images, image);
    }

    /* Sort by size, we want to download bigger images first, as that is usually
     * the use case anyway, first flash e.g. the .cwe file, then the .nvu one. */
    ctx->pending_images = g_list_sort (ctx->pending_images, (GCompareFunc) image_sort_by_size);

    switch (self->priv->type) {
    case UPDATER_TYPE_GENERIC:
        ctx->step = RUN_CONTEXT_STEP_USB_INFO;
        ctx->qmi_client_retries = QMI_CLIENT_RETRIES;
        ctx->cdc_wdm_file = g_object_ref (self->priv->cdc_wdm_file);
        break;
    case UPDATER_TYPE_QDL:
        ctx->step = RUN_CONTEXT_STEP_QDL_DEVICE;
        ctx->serial_file = g_object_ref (self->priv->serial_file);
        break;
    default:
        g_assert_not_reached ();
    }

    run_context_step (task);
}

/******************************************************************************/

QfuUpdater *
qfu_updater_new (GFile       *cdc_wdm_file,
                 const gchar *firmware_version,
                 const gchar *config_version,
                 const gchar *carrier,
                 gboolean     device_open_proxy,
                 gboolean     device_open_mbim)
{
    QfuUpdater *self;

    g_assert (G_IS_FILE (cdc_wdm_file));

    self = g_object_new (QFU_TYPE_UPDATER, NULL);
    self->priv->type = UPDATER_TYPE_GENERIC;
    self->priv->cdc_wdm_file = g_object_ref (cdc_wdm_file);
    self->priv->device_open_proxy = device_open_proxy;
    self->priv->device_open_mbim = device_open_mbim;
    self->priv->firmware_version = g_strdup (firmware_version);
    self->priv->config_version = g_strdup (config_version);
    self->priv->carrier = g_strdup (carrier);

    return self;
}

QfuUpdater *
qfu_updater_new_qdl (GFile *serial_file)
{
    QfuUpdater *self;

    g_assert (G_IS_FILE (serial_file));

    self = g_object_new (QFU_TYPE_UPDATER, NULL);
    self->priv->type = UPDATER_TYPE_QDL;
    self->priv->serial_file = g_object_ref (serial_file);

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

    g_clear_object (&self->priv->serial_file);
    g_clear_object (&self->priv->cdc_wdm_file);

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
