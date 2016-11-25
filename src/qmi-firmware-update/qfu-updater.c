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
    RUN_CONTEXT_STEP_CLEANUP_QMI_DEVICE,
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
} RunContext;

static void
run_context_free (RunContext *ctx)
{
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
    [RUN_CONTEXT_STEP_USB_INFO]           = run_context_step_usb_info,
    [RUN_CONTEXT_STEP_SELECT_IMAGE]       = run_context_step_select_image,
    [RUN_CONTEXT_STEP_QMI_DEVICE]         = run_context_step_qmi_device,
    [RUN_CONTEXT_STEP_QMI_DEVICE_OPEN]    = run_context_step_qmi_device_open,
    [RUN_CONTEXT_STEP_QMI_CLIENT]         = run_context_step_qmi_client,
    [RUN_CONTEXT_STEP_CLEANUP_QMI_DEVICE] = run_context_step_cleanup_qmi_device,
    [RUN_CONTEXT_STEP_CLEANUP_IMAGE]      = run_context_step_cleanup_image,
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