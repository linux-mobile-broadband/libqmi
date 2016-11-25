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
    gboolean  device_open_proxy;
    gboolean  device_open_mbim;
};

static const gchar *cdc_wdm_subsys[] = { "usbmisc", "usb", NULL };

/******************************************************************************/
/* Run */

typedef enum {
    RUN_CONTEXT_STEP_USB_INFO = 0,
    RUN_CONTEXT_STEP_LAST
} RunContextStep;

typedef struct {
    /* Context step */
    RunContextStep step;
    /* USB info */
    gchar *sysfs_path;
} RunContext;

static void
run_context_free (RunContext *ctx)
{
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
run_context_step_next (GTask *task)
{
    RunContext *ctx;

    ctx = (RunContext *) g_task_get_task_data (task);
    ctx->step++;

    /* Schedule next step in an idle */
    g_idle_add ((GSourceFunc) run_context_step_cb, task);
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

    run_context_step_next (task);
}

typedef void (* RunContextStepFunc) (GTask *task);
static const RunContextStepFunc run_context_step_func[] = {
    [RUN_CONTEXT_STEP_USB_INFO] = run_context_step_usb_info,
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
        g_debug ("[qfu-updater] running step %u/%lu...", ctx->step + 1, G_N_ELEMENTS (run_context_step_func));
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

    task = g_task_new (self, cancellable, callback, user_data);
    g_task_set_task_data (task, ctx, (GDestroyNotify) run_context_free);

    run_context_step (task);
}

/******************************************************************************/

QfuUpdater *
qfu_updater_new (GFile    *cdc_wdm_file,
                 gboolean  device_open_proxy,
                 gboolean  device_open_mbim)
{
    QfuUpdater *self;

    g_assert (G_IS_FILE (cdc_wdm_file));

    self = g_object_new (QFU_TYPE_UPDATER, NULL);
    self->priv->cdc_wdm_file = g_object_ref (cdc_wdm_file);
    self->priv->device_open_proxy = device_open_proxy;
    self->priv->device_open_mbim = device_open_mbim;

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

    G_OBJECT_CLASS (qfu_updater_parent_class)->dispose (object);
}

static void
qfu_updater_class_init (QfuUpdaterClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (object_class, sizeof (QfuUpdaterPrivate));

    object_class->dispose = dispose;
}
