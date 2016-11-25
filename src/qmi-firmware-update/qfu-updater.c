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

#include "qfu-updater.h"
#include "qfu-udev-helpers.h"

G_DEFINE_TYPE (QfuUpdater, qfu_updater, G_TYPE_OBJECT)

struct _QfuUpdaterPrivate {
    /* Inputs */
    GFile    *cdc_wdm_file;
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
} RunContext;

static void
run_context_free (RunContext *ctx)
{
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
    [RUN_CONTEXT_STEP_USB_INFO]      = run_context_step_usb_info,
    [RUN_CONTEXT_STEP_SELECT_IMAGE]  = run_context_step_select_image,
    [RUN_CONTEXT_STEP_CLEANUP_IMAGE] = run_context_step_cleanup_image,
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
qfu_updater_new (GFile    *cdc_wdm_file,
                 GList    *image_file_list,
                 gboolean  device_open_proxy,
                 gboolean  device_open_mbim)
{
    QfuUpdater *self;

    g_assert (G_IS_FILE (cdc_wdm_file));
    g_assert (image_file_list);

    self = g_object_new (QFU_TYPE_UPDATER, NULL);
    self->priv->cdc_wdm_file = g_object_ref (cdc_wdm_file);
    self->priv->device_open_proxy = device_open_proxy;
    self->priv->device_open_mbim = device_open_mbim;
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
qfu_updater_class_init (QfuUpdaterClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (object_class, sizeof (QfuUpdaterPrivate));

    object_class->dispose = dispose;
}
