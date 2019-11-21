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

#include "qfu-log.h"
#include "qfu-image-factory.h"
#include "qfu-updater.h"
#include "qfu-reseter.h"
#include "qfu-utils.h"
#include "qfu-udev-helpers.h"
#include "qfu-qdl-device.h"
#include "qfu-sahara-device.h"
#include "qfu-enum-types.h"

G_DEFINE_TYPE (QfuUpdater, qfu_updater, G_TYPE_OBJECT)

#define CLEAR_LINE "\33[2K\r"

typedef enum {
    UPDATER_TYPE_UNKNOWN,
#if defined WITH_UDEV
    UPDATER_TYPE_GENERIC,
#endif
    UPDATER_TYPE_DOWNLOAD,
} UpdaterType;

struct _QfuUpdaterPrivate {
    UpdaterType         type;
    QfuDeviceSelection *device_selection;
#if defined WITH_UDEV
    gchar              *firmware_version;
    gchar              *config_version;
    gchar              *carrier;
    QmiDeviceOpenFlags  device_open_flags;
    gboolean            ignore_version_errors;
    gboolean            override_download;
    guint8              modem_storage_index;
    gboolean            skip_validation;
#endif
};

/******************************************************************************/

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

/******************************************************************************/
/* Run */

/* Wait time after the upgrade has been done, before using the cdc-wdm port */
#define WAIT_FOR_BOOT_TIMEOUT_SECS 5
#define WAIT_FOR_BOOT_RETRIES      12

typedef enum {
#if defined WITH_UDEV
    RUN_CONTEXT_STEP_QMI_CLIENT,
    RUN_CONTEXT_STEP_GET_FIRMWARE_PREFERENCE,
    RUN_CONTEXT_STEP_SET_FIRMWARE_PREFERENCE,
    RUN_CONTEXT_STEP_POWER_CYCLE,
    RUN_CONTEXT_STEP_CLEANUP_QMI_DEVICE,
    RUN_CONTEXT_STEP_WAIT_FOR_TTY,
#endif
    RUN_CONTEXT_STEP_SELECT_DEVICE,
    RUN_CONTEXT_STEP_SELECT_IMAGE,
    RUN_CONTEXT_STEP_DOWNLOAD_IMAGE,
    RUN_CONTEXT_STEP_CLEANUP_IMAGE,
    RUN_CONTEXT_STEP_CLEANUP_DEVICE,
#if defined WITH_UDEV
    RUN_CONTEXT_STEP_WAIT_FOR_CDC_WDM,
    RUN_CONTEXT_STEP_WAIT_FOR_BOOT,
    RUN_CONTEXT_STEP_QMI_CLIENT_AFTER,
    RUN_CONTEXT_STEP_CLEANUP_QMI_DEVICE_FULL,
#endif
    RUN_CONTEXT_STEP_LAST
} RunContextStep;

typedef struct {
    /* Device selection */
#if defined WITH_UDEV
    GFile *cdc_wdm_file;
#endif
    GFile *serial_file;

    /* Context step */
    RunContextStep step;

#if defined WITH_UDEV
    /* Old/New info and capabilities */
    gchar                                    *revision;
    gboolean                                  supports_stored_image_management;
    guint8                                    max_modem_storage_index;
    gboolean                                  supports_firmware_preference_management;
    QmiMessageDmsGetFirmwarePreferenceOutput *firmware_preference;
    QmiMessageDmsSwiGetCurrentFirmwareOutput *current_firmware;
    gchar                                    *new_revision;
    gboolean                                  new_supports_stored_image_management;
    gboolean                                  new_supports_firmware_preference_management;
    QmiMessageDmsGetFirmwarePreferenceOutput *new_firmware_preference;
    QmiMessageDmsSwiGetCurrentFirmwareOutput *new_current_firmware;
#endif

    /* List of pending QfuImages to download, and the current one being
     * processed. */
    GList    *pending_images;
    QfuImage *current_image;

#if defined WITH_UDEV
    /* QMI device and client */
    QmiDevice    *qmi_device;
    QmiClientDms *qmi_client;

    /* Reset configuration */
    gboolean boothold_reset;

    /* Information gathered from the firmware images themselves */
    gchar *firmware_version;
    gchar *config_version;
    gchar *carrier;

    /* Waiting for boot */
    guint wait_for_boot_seconds_elapsed;
    guint wait_for_boot_retries;
#endif

    /* Device to use while already in download mode */
    QfuQdlDevice    *qdl_device;
    QfuSaharaDevice *sahara_device;
} RunContext;

static void
run_context_free (RunContext *ctx)
{
#if defined WITH_UDEV
    if (ctx->current_firmware)
        qmi_message_dms_swi_get_current_firmware_output_unref (ctx->current_firmware);
    if (ctx->new_current_firmware)
        qmi_message_dms_swi_get_current_firmware_output_unref (ctx->new_current_firmware);
    if (ctx->firmware_preference)
        qmi_message_dms_get_firmware_preference_output_unref (ctx->firmware_preference);
    if (ctx->new_firmware_preference)
        qmi_message_dms_get_firmware_preference_output_unref (ctx->new_firmware_preference);
    g_free (ctx->new_revision);
    g_free (ctx->revision);
    g_free (ctx->firmware_version);
    g_free (ctx->config_version);
    g_free (ctx->carrier);

    if (ctx->qmi_client) {
        g_assert (ctx->qmi_device);
        /* This release only happens when cleaning up from an error,
         * therefore always release CID */
        qmi_device_release_client (ctx->qmi_device,
                                   QMI_CLIENT (ctx->qmi_client),
                                   QMI_DEVICE_RELEASE_CLIENT_FLAGS_RELEASE_CID,
                                   10, NULL, NULL, NULL);
        g_object_unref (ctx->qmi_client);
    }
    if (ctx->qmi_device) {
        qmi_device_close_async (ctx->qmi_device, 10, NULL, NULL, NULL);
        g_object_unref (ctx->qmi_device);
    }
    if (ctx->cdc_wdm_file)
        g_object_unref (ctx->cdc_wdm_file);
#endif

    g_clear_object (&ctx->qdl_device);
    g_clear_object (&ctx->sahara_device);
    g_clear_object (&ctx->serial_file);
    g_clear_object (&ctx->current_image);
    g_list_free_full (ctx->pending_images, g_object_unref);
    g_slice_free (RunContext, ctx);
}

gboolean
qfu_updater_run_finish (QfuUpdater    *self,
                        GAsyncResult  *res,
                        GError       **error)
{
    return g_task_propagate_boolean (G_TASK (res), error);
}

#if defined WITH_UDEV

static void
print_firmware_preference (QmiMessageDmsGetFirmwarePreferenceOutput *firmware_preference,
                           const gchar                              *prefix)
{
    GArray *array;
    guint   i;

    qmi_message_dms_get_firmware_preference_output_get_list (firmware_preference, &array, NULL);

    if (array->len > 0) {
        for (i = 0; i < array->len; i++) {
            QmiMessageDmsGetFirmwarePreferenceOutputListImage *image;
            gchar                                             *unique_id_str;

            image = &g_array_index (array, QmiMessageDmsGetFirmwarePreferenceOutputListImage, i);
            unique_id_str = qfu_utils_get_firmware_image_unique_id_printable (image->unique_id);
            g_print ("%simage '%s': unique id '%s', build id '%s'\n",
                     prefix,
                     qmi_dms_firmware_image_type_get_string (image->type),
                     unique_id_str,
                     image->build_id);
            g_free (unique_id_str);
        }
    } else
        g_debug ("%sno preference specified\n", prefix);
}

static void
print_current_firmware (QmiMessageDmsSwiGetCurrentFirmwareOutput *current_firmware,
                        const gchar                              *prefix)
{
    const gchar *model = NULL;
    const gchar *boot_version = NULL;
    const gchar *amss_version = NULL;
    const gchar *sku_id = NULL;
    const gchar *package_id = NULL;
    const gchar *carrier_id = NULL;
    const gchar *pri_version = NULL;
    const gchar *carrier = NULL;
    const gchar *config_version = NULL;

    qmi_message_dms_swi_get_current_firmware_output_get_model          (current_firmware, &model,          NULL);
    qmi_message_dms_swi_get_current_firmware_output_get_boot_version   (current_firmware, &boot_version,   NULL);
    qmi_message_dms_swi_get_current_firmware_output_get_amss_version   (current_firmware, &amss_version,   NULL);
    qmi_message_dms_swi_get_current_firmware_output_get_sku_id         (current_firmware, &sku_id,         NULL);
    qmi_message_dms_swi_get_current_firmware_output_get_package_id     (current_firmware, &package_id,     NULL);
    qmi_message_dms_swi_get_current_firmware_output_get_carrier_id     (current_firmware, &carrier_id,     NULL);
    qmi_message_dms_swi_get_current_firmware_output_get_pri_version    (current_firmware, &pri_version,    NULL);
    qmi_message_dms_swi_get_current_firmware_output_get_carrier        (current_firmware, &carrier,        NULL);
    qmi_message_dms_swi_get_current_firmware_output_get_config_version (current_firmware, &config_version, NULL);

    if (model)
        g_print ("%sModel: %s\n", prefix, model);
    if (boot_version)
        g_print ("%sBoot version: %s\n", prefix, boot_version);
    if (amss_version)
        g_print ("%sAMSS version: %s\n", prefix, amss_version);
    if (sku_id)
        g_print ("%sSKU ID: %s\n", prefix, sku_id);
    if (package_id)
        g_print ("%sPackage ID: %s\n", prefix, package_id);
    if (carrier_id)
        g_print ("%sCarrier ID: %s\n", prefix, carrier_id);
    if (config_version)
        g_print ("%sConfig version: %s\n", prefix, config_version);
}

#endif /* WITH_UDEV */

static void
run_context_step_last (GTask *task)
{
    RunContext *ctx;
    QfuUpdater *self;

    ctx = (RunContext *) g_task_get_task_data (task);
    g_assert (ctx);

    self = g_task_get_source_object (task);

    if (self->priv->type == UPDATER_TYPE_DOWNLOAD) {
        g_task_return_boolean (task, TRUE);
        g_object_unref (task);
        return;
    }

#if defined WITH_UDEV
    if (self->priv->type == UPDATER_TYPE_GENERIC) {
        g_print ("\n"
                 "------------------------------------------------------------------------\n");

        g_print ("\n"
                 "   original firmware revision was:\n"
                 "      %s\n", ctx->revision ? ctx->revision : "unknown");
        if (ctx->current_firmware) {
            g_print ("   original running firmware details:\n");
            print_current_firmware (ctx->current_firmware, "      ");
        }
        if (ctx->firmware_preference) {
            g_print ("   original firmware preference details:\n");
            print_firmware_preference (ctx->firmware_preference, "      ");
        }

        g_print ("\n"
                 "   new firmware revision is:\n"
                 "      %s\n", ctx->new_revision ? ctx->new_revision : "unknown");
        if (ctx->new_current_firmware) {
            g_print ("   new running firmware details:\n");
            print_current_firmware (ctx->new_current_firmware, "      ");
        }
        if (ctx->new_firmware_preference) {
            g_print ("   new firmware preference details:\n");
            print_firmware_preference (ctx->new_firmware_preference, "      ");
        }

        if (ctx->new_supports_stored_image_management)
            g_print ("\n"
                     "   NOTE: this device supports stored image management\n"
                     "   with qmicli operations:\n"
                     "      --dms-list-stored-images\n"
                     "      --dms-select-stored-image\n"
                     "      --dms-delete-stored-image\n");

        if (ctx->new_supports_firmware_preference_management)
            g_print ("\n"
                     "   NOTE: this device supports firmware preference management\n"
                     "   with qmicli operations:\n"
                     "      --dms-get-firmware-preference\n"
                     "      --dms-set-firmware-preference\n");

        g_print ("\n"
                 "------------------------------------------------------------------------\n"
                 "\n");

        g_task_return_boolean (task, TRUE);
        g_object_unref (task);
        return;
    }
#endif /* WITH_UDEV */

    g_assert_not_reached ();
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

#if defined WITH_UDEV

static void
run_context_step_next_no_idle (GTask *task, RunContextStep next)
{
    RunContext *ctx;

    ctx = (RunContext *) g_task_get_task_data (task);
    ctx->step = next;

    /* Run right away */
    run_context_step (task);
}

static void
close_ready (QmiDevice    *dev,
             GAsyncResult *res,
             GTask        *task)
{
    RunContext *ctx;
    GError     *error = NULL;

    ctx = (RunContext *) g_task_get_task_data (task);

    if (!qmi_device_close_finish (dev, res, &error)) {
        g_warning ("[qfu-updater] couldn't close device: %s", error->message);
        g_error_free (error);
    } else
        g_debug ("[qfu-updater] closed");

    /* Go on */
    run_context_step_next (task, ctx->step + 1);
}

static void
release_client_ready (QmiDevice    *dev,
                      GAsyncResult *res,
                      GTask        *task)
{
    RunContext *ctx;
    GError     *error = NULL;

    ctx = (RunContext *) g_task_get_task_data (task);

    if (!qmi_device_release_client_finish (dev, res, &error)) {
        g_warning ("[qfu-updater] couldn't release client: %s", error->message);
        g_error_free (error);
    } else
        g_debug ("[qfu-updater] client released");

    qmi_device_close_async (ctx->qmi_device,
                            10,
                            g_task_get_cancellable (task),
                            (GAsyncReadyCallback) close_ready,
                            task);
    g_clear_object (&ctx->qmi_device);
}

static void
run_context_step_cleanup_qmi_device_full (GTask *task)
{
    RunContext *ctx;

    ctx = (RunContext *) g_task_get_task_data (task);

    qmi_device_release_client (ctx->qmi_device,
                               QMI_CLIENT (ctx->qmi_client),
                               QMI_DEVICE_RELEASE_CLIENT_FLAGS_RELEASE_CID,
                               10,
                               g_task_get_cancellable (task),
                               (GAsyncReadyCallback) release_client_ready,
                               task);
    g_clear_object (&ctx->qmi_client);
}

static void
new_client_dms_after_ready (gpointer      unused,
                            GAsyncResult *res,
                            GTask        *task)
{
    RunContext *ctx;
    GError     *error = NULL;

    ctx = (RunContext *) g_task_get_task_data (task);

    g_assert (!ctx->qmi_device);
    g_assert (!ctx->qmi_client);

    /* After the upgrade, non-fatal error if DMS client couldn't be created */
    if (!qfu_utils_new_client_dms_finish (res,
                                          &ctx->qmi_device,
                                          &ctx->qmi_client,
                                          &ctx->new_revision,
                                          &ctx->new_supports_stored_image_management,
                                          NULL, /* we don't care about the max number of images */
                                          &ctx->new_supports_firmware_preference_management,
                                          &ctx->new_firmware_preference,
                                          &ctx->new_current_firmware,
                                          &error)) {
        if (ctx->wait_for_boot_retries == WAIT_FOR_BOOT_RETRIES) {
            g_warning ("couldn't create DMS client after upgrade: %s", error->message);
            run_context_step_next (task, ctx->step + 1);
        } else {
            g_debug ("couldn't create DMS client after upgrade: %s (will retry)", error->message);
            run_context_step_next (task, RUN_CONTEXT_STEP_WAIT_FOR_BOOT);
        }
        g_error_free (error);
        return;
    }

    /* Go on */
    run_context_step_next (task, ctx->step + 1);
}

static void
run_context_step_qmi_client_after (GTask *task)
{
    RunContext *ctx;
    QfuUpdater *self;

    ctx = (RunContext *) g_task_get_task_data (task);
    self = g_task_get_source_object (task);

    ctx->wait_for_boot_retries++;
    g_print ("loading device information after the update (%u/%u)...\n",
             ctx->wait_for_boot_retries, WAIT_FOR_BOOT_RETRIES);

    g_debug ("[qfu-updater] creating QMI DMS client after upgrade...");
    g_assert (ctx->cdc_wdm_file);
    qfu_utils_new_client_dms (ctx->cdc_wdm_file,
                              1, /* single try to allocate DMS client */
                              self->priv->device_open_flags,
                              TRUE,
                              g_task_get_cancellable (task),
                              (GAsyncReadyCallback) new_client_dms_after_ready,
                              task);
}

static gboolean
wait_for_boot_ready (GTask *task)
{
    RunContext *ctx;

    ctx = (RunContext *) g_task_get_task_data (task);
    ctx->wait_for_boot_seconds_elapsed++;

    if (ctx->wait_for_boot_seconds_elapsed < WAIT_FOR_BOOT_TIMEOUT_SECS) {
        if (!qfu_log_get_verbose_stdout ())
            g_print (CLEAR_LINE "%s %u",
                     progress[ctx->wait_for_boot_seconds_elapsed % G_N_ELEMENTS (progress)],
                     WAIT_FOR_BOOT_TIMEOUT_SECS - ctx->wait_for_boot_seconds_elapsed);
        return G_SOURCE_CONTINUE;
    }

    if (!qfu_log_get_verbose_stdout ())
        g_print (CLEAR_LINE);

    /* Go on */
    run_context_step_next (task, ctx->step + 1);
    return G_SOURCE_REMOVE;
}

static void
run_context_step_wait_for_boot (GTask *task)
{
    RunContext *ctx;

    ctx = (RunContext *) g_task_get_task_data (task);
    ctx->wait_for_boot_seconds_elapsed = 0;

    g_debug ("[qfu-updater] waiting some time (%us) before accessing the cdc-wdm device...",
             WAIT_FOR_BOOT_TIMEOUT_SECS);

    if (!qfu_log_get_verbose_stdout ()) {
        g_print ("waiting some time for the device to boot...\n");
        g_print ("%s %u", progress[0], WAIT_FOR_BOOT_TIMEOUT_SECS);
    }

    g_timeout_add_seconds (1, (GSourceFunc) wait_for_boot_ready, task);
}

static void
wait_for_cdc_wdm_ready (QfuDeviceSelection *device_selection,
                        GAsyncResult       *res,
                        GTask              *task)
{
    GError     *error = NULL;
    RunContext *ctx;
    QfuUpdater *self;
    gchar      *path;

    ctx = (RunContext *) g_task_get_task_data (task);
    self = g_task_get_source_object (task);

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

    g_print ("normal mode detected\n");

    /* If no need to validate, we're done */
    if (self->priv->skip_validation) {
        run_context_step_next (task, RUN_CONTEXT_STEP_LAST);
        return;
    }

    g_print ("\n"
             "------------------------------------------------------------------------\n"
             "    NOTE: in order to validate which is the firmware running in the\n"
             "    module, the program will wait for a complete boot; this process\n"
             "    may take some time and several retries.\n"
             "------------------------------------------------------------------------\n"
             "\n");

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

#endif /* WITH_UDEV */

static void
run_context_step_cleanup_device (GTask *task)
{
    RunContext *ctx;
    QfuUpdater *self;

    ctx = (RunContext *) g_task_get_task_data (task);
    self = g_task_get_source_object (task);

    g_assert (ctx->serial_file);

    if (ctx->qdl_device) {
        g_debug ("[qfu-updater] QDL reset");
        qfu_qdl_device_reset (ctx->qdl_device, g_task_get_cancellable (task), NULL);
        g_clear_object (&ctx->qdl_device);
    } else if (ctx->sahara_device) {
        g_debug ("[qfu-updater] firehose reset");
        qfu_sahara_device_firehose_reset (ctx->sahara_device, g_task_get_cancellable (task), NULL);
        g_clear_object (&ctx->sahara_device);
    } else
        g_assert_not_reached ();

    g_clear_object (&ctx->serial_file);

    g_print ("rebooting in normal mode...\n");

    /* If we were running in download mode, we don't even wait for the reboot to finish */
    if (self->priv->type == UPDATER_TYPE_DOWNLOAD) {
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

static gboolean
download_image_firehose (QfuSaharaDevice  *device,
                         QfuImage         *image,
                         GCancellable     *cancellable,
                         GError          **error)
{
    guint sequence;
    guint n_blocks;

    if (!qfu_sahara_device_firehose_setup_download (device, image, &n_blocks, cancellable, error)) {
        g_prefix_error (error, "couldn't prepare download: ");
        return FALSE;
    }

    for (sequence = 0; sequence < n_blocks; sequence++) {
        if (!qfu_log_get_verbose_stdout ()) {
            if (n_blocks > 1) {
                g_print (CLEAR_LINE "%s %04.1lf%%",
                         progress[sequence % G_N_ELEMENTS (progress)],
                         100.0 * ((gdouble) sequence / (gdouble) (n_blocks - 1)));
            }
        }
        if (!qfu_sahara_device_firehose_write_block (device, image, sequence, cancellable, error)) {
            g_prefix_error (error, "couldn't write in session: ");
            return FALSE;
        }
    }

    g_debug ("[qfu-updater] all blocks downloaded");

    if (!qfu_log_get_verbose_stdout ())
        g_print (CLEAR_LINE "finalizing download... (may take several minutes, be patient)\n");

    if (!qfu_sahara_device_firehose_teardown_download (device, image, cancellable, error)) {
        g_prefix_error (error, "couldn't teardown download: ");
        return FALSE;
    }

    if (!qfu_log_get_verbose_stdout ())
        g_print (CLEAR_LINE);

    g_debug ("[qfu-updater] sahara/firehose download finished");
    return TRUE;
}

static gboolean
download_image_qdl (QfuQdlDevice  *device,
                    QfuImage      *image,
                    GCancellable  *cancellable,
                    GError       **error)
{
    guint16 sequence;
    guint16 n_chunks;

    if (!qfu_qdl_device_hello (device, cancellable, error)) {
        g_prefix_error (error, "couldn't send greetings to device: ");
        return FALSE;
    }

    if (!qfu_qdl_device_ufopen (device, image, cancellable, error)) {
        g_prefix_error (error, "couldn't open session: ");
        return FALSE;
    }

    n_chunks = qfu_image_get_n_data_chunks (image);
    for (sequence = 0; sequence < n_chunks; sequence++) {
        if (!qfu_log_get_verbose_stdout ()) {
            /* Use n-1 chunks for progress reporting; because the last one will take
             * a lot longer. */
            if (n_chunks > 1 && sequence < (n_chunks - 1))
                g_print (CLEAR_LINE "%s %04.1lf%%",
                         progress[sequence % G_N_ELEMENTS (progress)],
                         100.0 * ((gdouble) sequence / (gdouble) (n_chunks - 1)));
            else if (sequence == (n_chunks - 1))
                g_print (CLEAR_LINE "finalizing download... (may take more than one minute, be patient)\n");
        }
        if (!qfu_qdl_device_ufwrite (device, image, sequence, cancellable, error)) {
            g_prefix_error (error, "couldn't write in session: ");
            return FALSE;
        }
    }

    g_debug ("[qfu-updater] all chunks ack-ed");

    if (!qfu_log_get_verbose_stdout ())
        g_print (CLEAR_LINE);

    if (!qfu_qdl_device_ufclose (device, cancellable, error)) {
        g_prefix_error (error, "couldn't close session: ");
        return FALSE;
    }

    g_debug ("[qfu-updater] qdl/sdp download finished");
    return TRUE;
}

static void
run_context_step_download_image (GTask *task)
{
    RunContext   *ctx;
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

    /* QDL/SDP based download */
    if (ctx->qdl_device)
        download_image_qdl (ctx->qdl_device,
                            ctx->current_image,
                            cancellable,
                            &error);
    /* Sahara based download */
    else if (ctx->sahara_device)
        download_image_firehose (ctx->sahara_device,
                                 ctx->current_image,
                                 cancellable,
                                 &error);
    else
        g_assert_not_reached ();

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
run_context_step_select_device (GTask *task)
{
    RunContext *ctx;
    GError     *error = NULL;

    ctx = (RunContext *) g_task_get_task_data (task);

    g_assert (ctx->serial_file);
    g_assert (!ctx->qdl_device);
    g_assert (!ctx->sahara_device);

    /* Check if we can setup a Sahara device. Always check this first, because
     * the sahara stack is very sensitive to any kind of data sent to the port. */
    ctx->sahara_device = qfu_sahara_device_new (ctx->serial_file, g_task_get_cancellable (task), &error);
    if (!ctx->sahara_device) {
        g_debug ("[qfu-updater] sahara device creation failed: %s", error->message);
        g_clear_error (&error);

        /* Check if we can setup a QDL device */
        ctx->qdl_device = qfu_qdl_device_new (ctx->serial_file, g_task_get_cancellable (task), &error);
        if (!ctx->qdl_device) {
            g_debug ("[qfu-updater] qdl device creation failed: %s", error->message);
            g_clear_error (&error);
        }
    }

    if (!ctx->qdl_device && !ctx->sahara_device) {
        g_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_FAILED, "unsupported download protocol");
        g_object_unref (task);
        return;
    }

    run_context_step_next (task, ctx->step + 1);
}

#if defined WITH_UDEV

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

    g_print ("download mode detected\n");

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
run_context_step_cleanup_qmi_device (GTask *task)
{
    RunContext *ctx;
    QmiDevice  *tmp;

    ctx = (RunContext *) g_task_get_task_data (task);

    g_debug ("[qfu-updater] cleaning up QMI device...");

    /* We don't release CID as we're going to reset anyway */
    qmi_device_release_client (ctx->qmi_device,
                               QMI_CLIENT (ctx->qmi_client),
                               QMI_DEVICE_RELEASE_CLIENT_FLAGS_NONE,
                               10, NULL, NULL, NULL);
    g_clear_object (&ctx->qmi_client);

    /* We want to close and unref the QmiDevice only AFTER having set the wait
     * tasks for cdc-wdm or tty devices. This is because the close operation may
     * take a long time if doing QMI over MBIM (as the MBIM close async
     * operation is run internally). If we don't do this in this sequence, we
     * may end up getting udev events reported before the wait have started. */
    tmp = g_object_ref (ctx->qmi_device);

    g_clear_object (&ctx->qmi_device);
    g_clear_object (&ctx->cdc_wdm_file);

    /* If nothing to download we don't need to wait for QDL download mode */
    if (!ctx->pending_images)
        run_context_step_next_no_idle (task, RUN_CONTEXT_STEP_WAIT_FOR_CDC_WDM);
    else
        /* Go on */
        run_context_step_next_no_idle (task, ctx->step + 1);

    /* After the wait operation has been started, we do run the close */
    qmi_device_close_async (tmp, 10, NULL, NULL, NULL);
    g_object_unref (tmp);
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
power_cycle_ready (QmiClientDms *qmi_client,
                   GAsyncResult *res,
                   GTask        *task)
{
    GError     *error = NULL;
    RunContext *ctx;

    ctx = (RunContext *) g_task_get_task_data (task);

    if (!qfu_utils_power_cycle_finish (qmi_client, res, &error)) {
        g_task_return_error (task, error);
        g_object_unref (task);
        return;
    }

    g_debug ("[qfu-updater] reset requested successfully...");

    /* Go on */
    run_context_step_next (task, ctx->step + 1);
}

static void
run_context_step_power_cycle (GTask *task)
{
    RunContext *ctx;
    QfuUpdater *self;
    QfuReseter *reseter;

    ctx = (RunContext *) g_task_get_task_data (task);
    self = g_task_get_source_object (task);

    g_debug ("[qfu-updater] power cycling...");
    if (!ctx->boothold_reset) {
        qfu_utils_power_cycle (ctx->qmi_client,
                               g_task_get_cancellable (task),
                               (GAsyncReadyCallback) power_cycle_ready,
                               task);
        return;
    }

    /* Boothold is required when firmware preference isn't supported; and if so,
     * there must always be images to download. The only case in which we don't
     * have images listed for download is when we're told that there is nothing
     * to download via firmware preference. */
    g_assert (ctx->pending_images != NULL);

    /* Boothold reset */
    reseter = qfu_reseter_new (self->priv->device_selection, ctx->qmi_client, self->priv->device_open_flags);
    qfu_reseter_run (reseter,
                     g_task_get_cancellable (task),
                     (GAsyncReadyCallback) reseter_run_ready,
                     task);
    g_object_unref (reseter);
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
            g_print ("device already contains the given firmware/config version: no download needed\n");
            g_print ("forcing the download may be requested with the --override-download option\n");
            g_print ("now power cycling to apply the new firmware preference...\n");
            g_list_free_full (ctx->pending_images, g_object_unref);
            ctx->pending_images = NULL;
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

    if (self->priv->override_download)
        qmi_message_dms_set_firmware_preference_input_set_download_override (input, TRUE, NULL);

    if (self->priv->modem_storage_index > 0)
        qmi_message_dms_set_firmware_preference_input_set_modem_storage_index (input, (guint8) self->priv->modem_storage_index, NULL);

    g_debug ("[qfu-updater] setting firmware preference...");
    g_debug ("[qfu-updater]   modem image: unique id '%.16s', build id '%s'",
             (gchar *) (modem_image_id.unique_id->data), modem_image_id.build_id);
    g_debug ("[qfu-updater]   pri image:   unique id '%.16s', build id '%s'",
             (gchar *) (pri_image_id.unique_id->data), pri_image_id.build_id);
    g_debug ("[qfu-updater]   override download: %s",
             self->priv->override_download ? "yes" : "no");

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

    qmi_message_dms_set_firmware_preference_input_unref (input);
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
                if (!self->priv->ignore_version_errors) {
                    g_set_error (error, G_IO_ERROR, G_IO_ERROR_PERMISSION_DENIED,
                                 "couldn't detect firmware version: "
                                 "firmware version strings don't match on specified images: "
                                 "'%s' != '%s'",
                                 firmware_version, ctx->firmware_version);
                    return FALSE;
                }
                g_warning ("firmware version strings don't match on specified images: "
                           "'%s' != '%s' (IGNORED with --ignore-version-errors)",
                           firmware_version, ctx->firmware_version);
            }
        }

        if (config_version) {
            if (!ctx->config_version)
                ctx->config_version = g_strdup (config_version);
            else if (!g_str_equal (config_version, ctx->config_version)) {
                if (!self->priv->ignore_version_errors) {
                    g_set_error (error, G_IO_ERROR, G_IO_ERROR_PERMISSION_DENIED,
                                 "couldn't detect config version: "
                                 "config version strings don't match on specified images: "
                                 "'%s' != '%s'",
                                 config_version, ctx->config_version);
                    return FALSE;
                }
                g_warning ("[qfu-updater] config version strings don't match on specified images: "
                           "'%s' != '%s' (IGNORED with --ignore-version-errors)",
                           config_version, ctx->config_version);
            }
        }

        if (carrier) {
            if (!ctx->carrier)
                ctx->carrier = g_strdup (carrier);
            else if (!g_str_equal (carrier, ctx->carrier)) {
                if (!self->priv->ignore_version_errors) {
                    g_set_error (error, G_IO_ERROR, G_IO_ERROR_PERMISSION_DENIED,
                                 "couldn't detect carrier: "
                                 "carrier strings don't match on specified images: "
                                 "'%s' != '%s'",
                                 carrier, ctx->carrier);
                    return FALSE;
                }
                g_warning ("[qfu-updater] carrier strings don't match on specified images: "
                           "'%s' != '%s' (IGNORED with --ignore-version-errors)",
                           carrier, ctx->carrier);
            }
        }
    }

    /* If given firmware version doesn't match the one in the image, error out */
    if (self->priv->firmware_version && (g_strcmp0 (self->priv->firmware_version, ctx->firmware_version) != 0)) {
        if (!self->priv->ignore_version_errors) {
            g_set_error (error, G_IO_ERROR, G_IO_ERROR_PERMISSION_DENIED,
                         "error validating firmware version: "
                         "user provided firmware version doesn't match the one in the specified images: "
                         "'%s' != '%s'",
                         self->priv->firmware_version, ctx->firmware_version);
            return FALSE;
        }
        g_warning ("[qfu-updater] user provided firmware version doesn't match the one in the specified images: "
                   "'%s' != '%s' (IGNORED with --ignore-version-errors)",
                   self->priv->firmware_version, ctx->firmware_version);
    }

    /* If given config version doesn't match the one in the image, error out */
    if (self->priv->config_version && (g_strcmp0 (self->priv->config_version, ctx->config_version) != 0)) {
        if (!self->priv->ignore_version_errors) {
            g_set_error (error, G_IO_ERROR, G_IO_ERROR_PERMISSION_DENIED,
                         "error validating firmware version: "
                         "user provided firmware version doesn't match the one in the specified images: "
                         "'%s' != '%s'",
                         self->priv->config_version, ctx->config_version);
            return FALSE;
        }
        g_warning ("[qfu-updater] user provided config version doesn't match the one in the specified images: "
                   "'%s' != '%s' (IGNORED with --ignore-version-errors)",
                   self->priv->firmware_version, ctx->firmware_version);
    }

    /* If given carrier doesn't match the one in the image, error out */
    if (self->priv->carrier && (g_strcmp0 (self->priv->carrier, ctx->carrier) != 0)) {
        if (!self->priv->ignore_version_errors) {
            g_set_error (error, G_IO_ERROR, G_IO_ERROR_PERMISSION_DENIED,
                         "error validating carrier: "
                         "user provided carrier doesn't match the one in the specified images: "
                         "'%s' != '%s'",
                         self->priv->carrier, ctx->carrier);
            return FALSE;
        }
        g_warning ("[qfu-updater] user provided carrier doesn't match the one in the specified images: "
                   "'%s' != '%s' (IGNORED with --ignore-version-errors)",
                   self->priv->carrier, ctx->carrier);
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
run_context_step_get_firmware_preference (GTask *task)
{
    RunContext *ctx;
    QfuUpdater *self;
    GError     *error = NULL;

    ctx = (RunContext *) g_task_get_task_data (task);
    self = g_task_get_source_object (task);

    if (!ctx->supports_firmware_preference_management) {
        /* Firmware preference setting not supported; fail if we got those settings explicitly */
        if (self->priv->firmware_version || self->priv->config_version || self->priv->carrier) {
            g_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_FAILED,
                                     "setting firmware/config/carrier is not supported by this device");
            g_object_unref (task);
            return;
        }

        /* Jump to the reset step and run boothold there */
        ctx->boothold_reset = TRUE;
        run_context_step_next (task, RUN_CONTEXT_STEP_POWER_CYCLE);
        return;
    }

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
new_client_dms_ready (gpointer      unused,
                      GAsyncResult *res,
                      GTask        *task)
{
    RunContext *ctx;
    QfuUpdater *self;
    GError     *error = NULL;

    ctx = (RunContext *) g_task_get_task_data (task);
    self = g_task_get_source_object (task);

    g_assert (!ctx->qmi_device);
    g_assert (!ctx->qmi_client);

    if (!qfu_utils_new_client_dms_finish (res,
                                          &ctx->qmi_device,
                                          &ctx->qmi_client,
                                          &ctx->revision,
                                          &ctx->supports_stored_image_management,
                                          &ctx->max_modem_storage_index,
                                          &ctx->supports_firmware_preference_management,
                                          &ctx->firmware_preference,
                                          &ctx->current_firmware,
                                          &error)) {
        g_task_return_error (task, error);
        g_object_unref (task);
        return;
    }

    /* Validate modem storage index, if one specified */
    if (self->priv->modem_storage_index > ctx->max_modem_storage_index) {
        g_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_FAILED,
                                 "modem storage index out of bounds (%u > %u)",
                                 self->priv->modem_storage_index,
                                 ctx->max_modem_storage_index);
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

    g_print ("loading device information before the update...\n");

    g_debug ("[qfu-updater] creating QMI DMS client...");
    g_assert (ctx->cdc_wdm_file);
    qfu_utils_new_client_dms (ctx->cdc_wdm_file,
                              3, /* initially, up to 3 tries to allocate DMS client */
                              self->priv->device_open_flags,
                              TRUE,
                              g_task_get_cancellable (task),
                              (GAsyncReadyCallback) new_client_dms_ready,
                              task);
}

#endif /* WITH_UDEV */

typedef void (* RunContextStepFunc) (GTask *task);
static const RunContextStepFunc run_context_step_func[] = {
#if defined WITH_UDEV
    [RUN_CONTEXT_STEP_QMI_CLIENT]              = run_context_step_qmi_client,
    [RUN_CONTEXT_STEP_GET_FIRMWARE_PREFERENCE] = run_context_step_get_firmware_preference,
    [RUN_CONTEXT_STEP_SET_FIRMWARE_PREFERENCE] = run_context_step_set_firmware_preference,
    [RUN_CONTEXT_STEP_POWER_CYCLE]             = run_context_step_power_cycle,
    [RUN_CONTEXT_STEP_CLEANUP_QMI_DEVICE]      = run_context_step_cleanup_qmi_device,
    [RUN_CONTEXT_STEP_WAIT_FOR_TTY]            = run_context_step_wait_for_tty,
#endif
    [RUN_CONTEXT_STEP_SELECT_DEVICE]           = run_context_step_select_device,
    [RUN_CONTEXT_STEP_SELECT_IMAGE]            = run_context_step_select_image,
    [RUN_CONTEXT_STEP_DOWNLOAD_IMAGE]          = run_context_step_download_image,
    [RUN_CONTEXT_STEP_CLEANUP_IMAGE]           = run_context_step_cleanup_image,
    [RUN_CONTEXT_STEP_CLEANUP_DEVICE]          = run_context_step_cleanup_device,
#if defined WITH_UDEV
    [RUN_CONTEXT_STEP_WAIT_FOR_CDC_WDM]        = run_context_step_wait_for_cdc_wdm,
    [RUN_CONTEXT_STEP_WAIT_FOR_BOOT]           = run_context_step_wait_for_boot,
    [RUN_CONTEXT_STEP_QMI_CLIENT_AFTER]        = run_context_step_qmi_client_after,
    [RUN_CONTEXT_STEP_CLEANUP_QMI_DEVICE_FULL] = run_context_step_cleanup_qmi_device_full,
#endif
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
    run_context_step_last (task);
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
#if defined WITH_UDEV
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
#endif
    case UPDATER_TYPE_DOWNLOAD:
        ctx->step = RUN_CONTEXT_STEP_SELECT_DEVICE;
        ctx->serial_file = qfu_device_selection_get_single_tty (self->priv->device_selection);
        if (!ctx->serial_file) {
            g_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT,
                                     "No serial device found to run QDL update operation");
            g_object_unref (task);
            return;
        }
        break;
    case UPDATER_TYPE_UNKNOWN:
    default:
        g_assert_not_reached ();
    }

    run_context_step (task);
}

/******************************************************************************/

#if defined WITH_UDEV

QfuUpdater *
qfu_updater_new (QfuDeviceSelection *device_selection,
                 const gchar        *firmware_version,
                 const gchar        *config_version,
                 const gchar        *carrier,
                 QmiDeviceOpenFlags  device_open_flags,
                 gboolean            ignore_version_errors,
                 gboolean            override_download,
                 guint8              modem_storage_index,
                 gboolean            skip_validation)
{
    QfuUpdater *self;

    g_assert (QFU_IS_DEVICE_SELECTION (device_selection));

    self = g_object_new (QFU_TYPE_UPDATER, NULL);
    self->priv->type = UPDATER_TYPE_GENERIC;
    self->priv->device_selection = g_object_ref (device_selection);
    self->priv->device_open_flags = device_open_flags;
    self->priv->firmware_version = (firmware_version ? g_strdup (firmware_version) : NULL);
    self->priv->config_version = (config_version ? g_strdup (config_version) : NULL);
    self->priv->carrier = (carrier ? g_strdup (carrier) : NULL);
    self->priv->ignore_version_errors = ignore_version_errors;
    self->priv->override_download = override_download;
    self->priv->modem_storage_index = modem_storage_index;
    self->priv->skip_validation = skip_validation;

    return self;
}

#endif

QfuUpdater *
qfu_updater_new_download (QfuDeviceSelection *device_selection)
{
    QfuUpdater *self;

    g_assert (QFU_IS_DEVICE_SELECTION (device_selection));

    self = g_object_new (QFU_TYPE_UPDATER, NULL);
    self->priv->type = UPDATER_TYPE_DOWNLOAD;
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
#if defined WITH_UDEV
    QfuUpdater *self = QFU_UPDATER (object);

    g_free (self->priv->firmware_version);
    g_free (self->priv->config_version);
    g_free (self->priv->carrier);
#endif

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
