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

#include "qfu-reseter.h"
#include "qfu-at-device.h"

G_DEFINE_TYPE (QfuReseter, qfu_reseter, G_TYPE_OBJECT)

struct _QfuReseterPrivate {
    QfuDeviceSelection *device_selection;
};

/******************************************************************************/
/* Run */

#define MAX_RETRIES 2

typedef struct {
    /* List of AT devices */
    GList *at_devices;
    GList *current;
    /* Retries */
    guint retries;
} RunContext;

static void
run_context_free (RunContext *ctx)
{
    g_list_free_full (ctx->at_devices, (GDestroyNotify) g_object_unref);
    g_slice_free (RunContext, ctx);
}

gboolean
qfu_reseter_run_finish (QfuReseter    *self,
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
    ctx->current = g_list_next (ctx->current);

    if (!ctx->current) {
        if (!ctx->retries) {
            g_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_FAILED,
                                     "couldn't run reseter operation");
            g_object_unref (task);
            return;
        }

        /* Launch retry with the first device */
        ctx->retries--;
        ctx->current = ctx->at_devices;
    }

    /* Schedule next step in an idle */
    g_idle_add ((GSourceFunc) run_context_step_cb, task);
}

static void
run_context_step (GTask *task)
{
    RunContext  *ctx;
    QfuAtDevice *at_device;
    GError      *error = NULL;

    ctx = (RunContext *) g_task_get_task_data (task);
    g_assert (ctx->current);

    at_device = QFU_AT_DEVICE (ctx->current->data);
    if (!qfu_at_device_boothold (at_device, g_task_get_cancellable (task), &error)) {
        g_debug ("error: %s", error->message);
        g_error_free (error);
        run_context_step_next (task);
        return;
    }

    /* Success! */
    g_task_return_boolean (task, TRUE);
    g_object_unref (task);
}

static gint
device_sort_by_name_reversed (QfuAtDevice *a, QfuAtDevice *b)
{
    return strcmp (qfu_at_device_get_name (b), qfu_at_device_get_name (a));
}

void
qfu_reseter_run (QfuReseter          *self,
                 GCancellable        *cancellable,
                 GAsyncReadyCallback  callback,
                 gpointer             user_data)
{
    RunContext *ctx;
    GTask      *task;
    GList      *l, *ttys;

    ctx = g_slice_new0 (RunContext);
    ctx->retries = MAX_RETRIES;

    task = g_task_new (self, cancellable, callback, user_data);
    g_task_set_task_data (task, ctx, (GDestroyNotify) run_context_free);

    /* List TTYs we may use for the reset operation */
    ttys = qfu_device_selection_get_multiple_ttys (self->priv->device_selection);
    if (!ttys) {
        g_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT,
                                 "No TTYs found to run reset operation");
        g_object_unref (task);
        return;
    }

    /* Build QfuAtDevice objects for each TTY given */
    for (l = ttys; l; l = g_list_next (l)) {
        GError      *error = NULL;
        QfuAtDevice *at_device;

        at_device = qfu_at_device_new (G_FILE (l->data), cancellable, &error);
        if (!at_device) {
            g_task_return_error (task, error);
            g_object_unref (task);
            return;
        }
        ctx->at_devices = g_list_append (ctx->at_devices, at_device);
    }
    g_list_free_full (ttys, (GDestroyNotify) g_object_unref);

    /* Sort by filename reversed; usually the TTY with biggest number is a
     * good AT port */
    ctx->at_devices = g_list_sort (ctx->at_devices, (GCompareFunc) device_sort_by_name_reversed);

    /* Select first TTY and start */
    ctx->current = ctx->at_devices;
    run_context_step (task);
}

/******************************************************************************/

QfuReseter *
qfu_reseter_new (QfuDeviceSelection *device_selection)
{
    QfuReseter *self;

    self = g_object_new (QFU_TYPE_RESETER, NULL);
    self->priv->device_selection = g_object_ref (device_selection);

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

    G_OBJECT_CLASS (qfu_reseter_parent_class)->dispose (object);
}

static void
qfu_reseter_class_init (QfuReseterClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (object_class, sizeof (QfuReseterPrivate));

    object_class->dispose = dispose;
}
