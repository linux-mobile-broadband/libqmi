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

#include <gio/gio.h>
#include <gudev/gudev.h>

#include "qfu-udev-helpers.h"

gchar *
qfu_udev_helper_get_udev_device_sysfs_path (GUdevDevice  *device,
                                            GError      **error)
{
    GUdevDevice *parent;
    gchar       *sysfs_path;

    /* We need to look for the parent GUdevDevice which has a "usb_device"
     * devtype. */

    parent = g_udev_device_get_parent (device);
    while (parent) {
        GUdevDevice *next;

        if (g_strcmp0 (g_udev_device_get_devtype (parent), "usb_device") == 0)
            break;

        /* Check next parent */
        next = g_udev_device_get_parent (parent);
        g_object_unref (parent);
        parent = next;
    }

    if (!parent) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "couldn't find parent physical USB device");
        return NULL;
    }

    sysfs_path = g_strdup (g_udev_device_get_sysfs_path (parent));
    g_object_unref (parent);
    return sysfs_path;
}

gchar *
qfu_udev_helper_get_sysfs_path (GFile               *file,
                                const gchar *const  *subsys,
                                GError             **error)
{
    guint        i;
    GUdevClient *udev;
    gchar       *sysfs_path = NULL;
    gchar       *basename;
    gboolean     matched = FALSE;

    /* Get the filename */
    basename = g_file_get_basename (file);
    if (!basename) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "couldn't get filename");
        return NULL;
    }

    udev = g_udev_client_new (NULL);

    /* Note: we'll only get devices reported in either one subsystem or the
     * other, never in both */
    for (i = 0; !matched && subsys[i]; i++) {
        GList *devices, *iter;

        devices = g_udev_client_query_by_subsystem (udev, subsys[i]);
        for (iter = devices; !matched && iter; iter = g_list_next (iter)) {
            const gchar *name;
            GUdevDevice *device;

            device = G_UDEV_DEVICE (iter->data);
            name = g_udev_device_get_name (device);
            if (g_strcmp0 (name, basename) != 0)
                continue;

            /* We'll halt the search once this has been processed */
            matched = TRUE;
            sysfs_path = qfu_udev_helper_get_udev_device_sysfs_path (device, error);
        }
        g_list_free_full (devices, (GDestroyNotify) g_object_unref);
    }

    if (!matched) {
        g_assert (!sysfs_path);
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "couldn't find device");
    }

    g_free (basename);
    g_object_unref (udev);
    return sysfs_path;
}

/******************************************************************************/

#define WAIT_FOR_TTY_TIMEOUT_SECS 15

typedef struct {
    GUdevClient *udev;
    gchar       *sysfs_path;
    guint        timeout_id;
    gulong       uevent_id;
    gulong       cancellable_id;
} WaitForTtyContext;

static void
wait_for_tty_context_free (WaitForTtyContext *ctx)
{
    g_assert (!ctx->timeout_id);
    g_assert (!ctx->uevent_id);
    g_assert (!ctx->cancellable_id);

    g_object_unref (ctx->udev);
    g_free (ctx->sysfs_path);
    g_slice_free (WaitForTtyContext, ctx);
}

GFile *
qfu_udev_helper_wait_for_tty_finish (GAsyncResult  *res,
                                     GError       **error)
{
    return G_FILE (g_task_propagate_pointer (G_TASK (res), error));
}

static void
handle_uevent (GUdevClient *client,
               const char  *action,
               GUdevDevice *device,
               GTask       *task)
{
    gchar             *sysfs_path;
    WaitForTtyContext *ctx;

    ctx = (WaitForTtyContext *) g_task_get_task_data (task);

    if (!g_str_equal (action, "add") && !g_str_equal (action, "move") && !g_str_equal (action, "change"))
        return;

    sysfs_path = qfu_udev_helper_get_udev_device_sysfs_path (device, NULL);
    if (!sysfs_path)
        return;

    /* If sysfs_path matches and driver is 'qcserial', we're done */
    if ((g_strcmp0 (sysfs_path, ctx->sysfs_path) == 0) &&
        (g_strcmp0 (g_udev_device_get_property (device, "ID_USB_DRIVER"), "qcserial") == 0)) {
        gchar *path;

        /* Disconnect this handler */
        g_signal_handler_disconnect (ctx->udev, ctx->uevent_id);
        ctx->uevent_id = 0;

        /* Disconnect the other handlers */
        g_cancellable_disconnect (g_task_get_cancellable (task), ctx->cancellable_id);
        ctx->cancellable_id = 0;
        g_source_remove (ctx->timeout_id);
        ctx->timeout_id = 0;

        path = g_strdup_printf ("/dev/%s", g_udev_device_get_name (device));
        g_task_return_pointer (task, g_file_new_for_path (path), g_object_unref);
        g_object_unref (task);
        g_free (path);
    }

    g_free (sysfs_path);
}

static gboolean
wait_for_tty_timed_out (GTask *task)
{
    WaitForTtyContext *ctx;

    ctx = (WaitForTtyContext *) g_task_get_task_data (task);

    /* Disconnect this handler */
    ctx->timeout_id = 0;

    /* Disconnect the other handlers */
    g_cancellable_disconnect (g_task_get_cancellable (task), ctx->cancellable_id);
    ctx->cancellable_id = 0;
    g_signal_handler_disconnect (ctx->udev, ctx->uevent_id);
    ctx->uevent_id = 0;

    g_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_TIMED_OUT,
                             "waiting for TTY at '%s' timed out",
                             ctx->sysfs_path);
    g_object_unref (task);
    return FALSE;
}

static void
wait_for_tty_cancelled (GCancellable *cancellable,
                        GTask        *task)
{
    WaitForTtyContext *ctx;

    ctx = (WaitForTtyContext *) g_task_get_task_data (task);

    /* Disconnect this handler */
    g_cancellable_disconnect (g_task_get_cancellable (task), ctx->cancellable_id);
    ctx->cancellable_id = 0;

    /* Disconnect the other handlers */
    g_source_remove (ctx->timeout_id);
    ctx->timeout_id = 0;
    g_signal_handler_disconnect (ctx->udev, ctx->uevent_id);
    ctx->uevent_id = 0;

    g_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_CANCELLED,
                             "waiting for TTY at '%s' cancelled",
                             ctx->sysfs_path);
    g_object_unref (task);
}

void
qfu_udev_helper_wait_for_tty (const gchar         *sysfs_path,
                              GCancellable        *cancellable,
                              GAsyncReadyCallback  callback,
                              gpointer             user_data)
{
    static const gchar *subsys_monitor[] = { "tty", NULL };
    GTask              *task;
    WaitForTtyContext  *ctx;

    ctx = g_slice_new0 (WaitForTtyContext);
    ctx->sysfs_path = g_strdup (sysfs_path);
    ctx->udev = g_udev_client_new (subsys_monitor);

    task = g_task_new (NULL, cancellable, callback, user_data);
    g_task_set_task_data (task, ctx, (GDestroyNotify) wait_for_tty_context_free);

    /* Monitor for tty additions. */
    ctx->uevent_id = g_signal_connect (ctx->udev,
                                       "uevent",
                                       G_CALLBACK (handle_uevent),
                                       task);

    /* Allow cancellation */
    ctx->cancellable_id = g_cancellable_connect (cancellable,
                                                 (GCallback) wait_for_tty_cancelled,
                                                 task,
                                                 NULL);

    /* And also, setup a timeout to avoid waiting forever. */
    ctx->timeout_id = g_timeout_add_seconds (WAIT_FOR_TTY_TIMEOUT_SECS,
                                             (GSourceFunc) wait_for_tty_timed_out,
                                             task);

    /* Note: task ownership is shared between the signals and the timeout */
}
