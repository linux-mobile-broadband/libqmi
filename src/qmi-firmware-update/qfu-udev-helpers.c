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

#include "config.h"

#include <stdlib.h>

#include <gio/gio.h>

#if defined WITH_UDEV
# include <gudev/gudev.h>
#endif

#include "qfu-udev-helpers.h"

/******************************************************************************/

static const gchar *device_type_str[] = {
    [QFU_UDEV_HELPER_DEVICE_TYPE_TTY]     = "tty",
    [QFU_UDEV_HELPER_DEVICE_TYPE_CDC_WDM] = "cdc-wdm",
};

G_STATIC_ASSERT (G_N_ELEMENTS (device_type_str) == QFU_UDEV_HELPER_DEVICE_TYPE_LAST);

const gchar *
qfu_udev_helper_device_type_to_string (QfuUdevHelperDeviceType type)
{
    return device_type_str[type];
}

/******************************************************************************/

#if defined WITH_UDEV

static const gchar *tty_subsys_list[]     = { "tty", NULL };
static const gchar *cdc_wdm_subsys_list[] = { "usbmisc", "usb", NULL };

static gboolean
udev_helper_get_udev_device_details (GUdevDevice  *device,
                                     gchar       **out_sysfs_path,
                                     guint16      *out_vid,
                                     guint16      *out_pid,
                                     guint        *out_busnum,
                                     guint        *out_devnum,
                                     GError      **error)
{
    GUdevDevice *parent;
    gulong       aux;

    if (out_vid)
        *out_vid = 0;
    if (out_pid)
        *out_pid = 0;
    if (out_sysfs_path)
        *out_sysfs_path = NULL;
    if (out_busnum)
        *out_busnum = 0;
    if (out_devnum)
        *out_devnum = 0;

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
        return FALSE;
    }

    if (out_sysfs_path)
        *out_sysfs_path = g_strdup (g_udev_device_get_sysfs_path (parent));

    if (out_vid) {
        aux = strtoul (g_udev_device_get_property (parent, "ID_VENDOR_ID"), NULL, 16);
        if (aux <= G_MAXUINT16)
            *out_vid = (guint16) aux;
    }

    if (out_pid) {
        aux = strtoul (g_udev_device_get_property (parent, "ID_MODEL_ID"), NULL, 16);
        if (aux <= G_MAXUINT16)
            *out_pid = (guint16) aux;
    }

    if (out_busnum) {
        aux = strtoul (g_udev_device_get_property (parent, "BUSNUM"), NULL, 10);
        if (aux <= G_MAXUINT)
            *out_busnum = (guint16) aux;
    }

    if (out_devnum) {
        aux = strtoul (g_udev_device_get_property (parent, "DEVNUM"), NULL, 10);
        if (aux <= G_MAXUINT)
            *out_devnum = (guint16) aux;
    }

    g_object_unref (parent);
    return TRUE;
}

static gboolean
udev_helper_get_udev_interface_details (GUdevDevice  *device,
                                        gchar       **out_driver,
                                        GError      **error)
{
    GUdevDevice *parent;

    /* We need to look for the parent GUdevDevice which has a "usb_interface"
     * devtype. */

    parent = g_udev_device_get_parent (device);
    while (parent) {
        GUdevDevice *next;

        if (g_strcmp0 (g_udev_device_get_devtype (parent), "usb_interface") == 0)
            break;

        /* Check next parent */
        next = g_udev_device_get_parent (parent);
        g_object_unref (parent);
        parent = next;
    }

    if (!parent) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "couldn't find parent interface USB device");
        return FALSE;
    }

    if (out_driver)
        *out_driver = g_strdup (g_udev_device_get_driver (parent));

    g_object_unref (parent);
    return TRUE;
}

/******************************************************************************/

gchar *
qfu_udev_helper_find_by_file (GFile   *file,
                              GError **error)
{
    GUdevClient  *client = NULL;
    GUdevDevice  *device = NULL;
    gchar        *basename = NULL;
    const gchar **subsys_list = NULL;
    gchar        *sysfs_path = NULL;
    guint         i;

    basename = g_file_get_basename (file);
    if (!basename) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "couldn't get filename");
        goto out;
    }

    client = g_udev_client_new (NULL);

    if (g_str_has_prefix (basename, "tty"))
        subsys_list = tty_subsys_list;
    else if  (g_str_has_prefix (basename, "cdc-wdm"))
        subsys_list = cdc_wdm_subsys_list;
    else {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "unknown device file type");
        goto out;
    }

    for (i = 0; !device && subsys_list[i]; i++)
        device = g_udev_client_query_by_subsystem_and_name (client, subsys_list[i], basename);

    if (!device) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "device not found");
        goto out;
    }

    if (!udev_helper_get_udev_device_details (device,
                                              &sysfs_path, NULL, NULL, NULL, NULL,
                                              error))
        goto out;

    g_debug ("[qfu-udev] sysfs path for '%s' found: %s", basename, sysfs_path);

out:
    g_free (basename);
    g_clear_object (&device);
    g_clear_object (&client);
    return sysfs_path;
}

gchar *
qfu_udev_helper_find_by_file_path (const gchar  *path,
                                   GError      **error)
{
    GFile *file;
    gchar *sysfs_path;

    file = g_file_new_for_path (path);
    sysfs_path = qfu_udev_helper_find_by_file (file, error);
    g_object_unref (file);
    return sysfs_path;
}

gchar *
qfu_udev_helper_find_peer_port (const gchar  *sysfs_path,
                                GError      **error)
{
    gchar *tmp, *path;

    tmp = g_build_filename (sysfs_path, "port", "peer", NULL);
    path = realpath (tmp, NULL);
    g_free (tmp);
    if (!path)
        return NULL;

    g_debug ("[qfu-udev] peer port for '%s' found: %s", sysfs_path, path);
    return path;
}

/******************************************************************************/

static gboolean
udev_helper_device_already_added (GPtrArray   *ptr,
                                  const gchar *sysfs_path)
{
    guint i;

    for (i = 0; i < ptr->len; i++) {
        if (g_strcmp0 (g_ptr_array_index (ptr, i), sysfs_path) == 0)
            return TRUE;
    }

    return FALSE;
}

static GPtrArray *
udev_helper_find_by_device_info_in_subsystem (GPtrArray    *sysfs_paths,
                                              GUdevClient  *udev,
                                              const gchar  *subsystem,
                                              guint16       vid,
                                              guint16       pid,
                                              guint         busnum,
                                              guint         devnum)
{
    GList     *devices;
    GList     *iter;

    devices = g_udev_client_query_by_subsystem (udev, subsystem);
    for (iter = devices; iter; iter = g_list_next (iter)) {
        GUdevDevice *device;
        guint16      device_vid = 0;
        guint16      device_pid = 0;
        guint        device_busnum = 0;
        guint        device_devnum = 0;
        gchar       *device_sysfs_path = NULL;

        device = G_UDEV_DEVICE (iter->data);

        if (udev_helper_get_udev_device_details (device,
                                                 &device_sysfs_path,
                                                 &device_vid,
                                                 &device_pid,
                                                 &device_busnum,
                                                 &device_devnum,
                                                 NULL)) {
            if ((vid == 0 || vid == device_vid) &&
                (pid == 0 || pid == device_pid) &&
                (busnum == 0 || busnum == device_busnum) &&
                (devnum == 0 || devnum == device_devnum) &&
                (!udev_helper_device_already_added (sysfs_paths, device_sysfs_path)))
                g_ptr_array_add (sysfs_paths, device_sysfs_path);
            else
                g_free (device_sysfs_path);
        }

        g_object_unref (device);
    }
    g_list_free (devices);
    return sysfs_paths;
}

gchar *
qfu_udev_helper_find_by_device_info (guint16   vid,
                                     guint16   pid,
                                     guint     busnum,
                                     guint     devnum,
                                     GError  **error)
{
    GUdevClient *udev;
    guint        i;
    GPtrArray   *sysfs_paths;
    GString     *match_str;
    gchar       *sysfs_path = NULL;

    sysfs_paths = g_ptr_array_new_with_free_func (g_free);
    udev        = g_udev_client_new (NULL);
    match_str   = g_string_new ("");

    if (vid != 0)
        g_string_append_printf (match_str, "vid 0x%04x", vid);
    if (pid != 0)
        g_string_append_printf (match_str, "%spid 0x%04x", match_str->len > 0 ? ", " : "", pid);
    if (busnum != 0)
        g_string_append_printf (match_str, "%sbus %03u", match_str->len > 0 ? ", " : "", busnum);
    if (devnum != 0)
        g_string_append_printf (match_str, "%sdev %03u", match_str->len > 0 ? ", " : "", devnum);
    g_assert (match_str->len > 0);

    for (i = 0; tty_subsys_list[i]; i++)
        sysfs_paths = udev_helper_find_by_device_info_in_subsystem (sysfs_paths,
                                                                    udev,
                                                                    tty_subsys_list[i],
                                                                    vid, pid, busnum, devnum);

    for (i = 0; cdc_wdm_subsys_list[i]; i++)
        sysfs_paths = udev_helper_find_by_device_info_in_subsystem (sysfs_paths,
                                                                    udev,
                                                                    cdc_wdm_subsys_list[i],
                                                                    vid, pid, busnum, devnum);

    for (i = 0; i < sysfs_paths->len; i++)
        g_debug ("[%s] sysfs path: %s", match_str->str, (gchar *) g_ptr_array_index (sysfs_paths, i));

    if (sysfs_paths->len == 0) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "no device found with matching criteria: %s",
                     match_str->str);
        goto out;
    }

    if (sysfs_paths->len > 1) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "multiple devices (%u) found with matching criteria: %s",
                     sysfs_paths->len, match_str->str);
        goto out;
    }

    sysfs_path = g_strdup (g_ptr_array_index (sysfs_paths, 0));

out:

    g_ptr_array_unref (sysfs_paths);
    g_string_free (match_str, TRUE);
    g_object_unref (udev);

    return sysfs_path;
}

/******************************************************************************/

static GFile *
device_matches_sysfs_and_type (GUdevDevice             *device,
                               const gchar             *sysfs_path,
                               QfuUdevHelperDeviceType  type)
{
    GFile *file = NULL;
    gchar *device_sysfs_path = NULL;
    gchar *device_driver = NULL;
    gchar *device_path = NULL;

    if (!udev_helper_get_udev_device_details (device,
                                              &device_sysfs_path, NULL, NULL, NULL, NULL,
                                              NULL))
        goto out;

    if (!device_sysfs_path)
        goto out;

    if (g_strcmp0 (device_sysfs_path, sysfs_path) != 0)
        goto out;

    if (!udev_helper_get_udev_interface_details (device,
                                                 &device_driver,
                                                 NULL))
        goto out;

    switch (type) {
    case QFU_UDEV_HELPER_DEVICE_TYPE_TTY:
        if (g_strcmp0 (device_driver, "qcserial") != 0)
            goto out;
        break;
    case QFU_UDEV_HELPER_DEVICE_TYPE_CDC_WDM:
        if (g_strcmp0 (device_driver, "qmi_wwan") != 0 && g_strcmp0 (device_driver, "cdc_mbim") != 0)
            goto out;
        break;
    case QFU_UDEV_HELPER_DEVICE_TYPE_LAST:
    default:
        g_assert_not_reached ();
    }

    device_path = g_strdup_printf ("/dev/%s", g_udev_device_get_name (device));
    file = g_file_new_for_path (device_path);
    g_free (device_path);

out:
    g_free (device_sysfs_path);
    g_free (device_driver);
    return file;
}

GList *
qfu_udev_helper_list_devices (QfuUdevHelperDeviceType  device_type,
                              const gchar             *sysfs_path)
{
    GUdevClient  *udev;
    const gchar **subsys_list = NULL;
    guint         i;
    GList        *files = NULL;

    udev = g_udev_client_new (NULL);

    switch (device_type) {
    case QFU_UDEV_HELPER_DEVICE_TYPE_TTY:
        subsys_list = tty_subsys_list;
        break;
    case QFU_UDEV_HELPER_DEVICE_TYPE_CDC_WDM:
        subsys_list = cdc_wdm_subsys_list;
        break;
    case QFU_UDEV_HELPER_DEVICE_TYPE_LAST:
    default:
        g_assert_not_reached ();
    }

    for (i = 0; subsys_list[i]; i++) {
        GList *devices, *iter;

        devices = g_udev_client_query_by_subsystem (udev, subsys_list[i]);
        for (iter = devices; iter; iter = g_list_next (iter)) {
            GFile *file;

            file = device_matches_sysfs_and_type (G_UDEV_DEVICE (iter->data), sysfs_path, device_type);
            if (file)
                files = g_list_prepend (files, file);
            g_object_unref (G_OBJECT (iter->data));
        }
        g_list_free (devices);
    }

    g_object_unref (udev);
    return files;
}

/******************************************************************************/

#define WAIT_FOR_DEVICE_TIMEOUT_SECS 120

typedef struct {
    QfuUdevHelperDeviceType  device_type;
    GUdevClient             *udev;
    gchar                   *sysfs_path;
    gchar                   *peer_port;
    guint                    timeout_id;
    gulong                   uevent_id;
    gulong                   cancellable_id;
} WaitForDeviceContext;

static void
wait_for_device_context_free (WaitForDeviceContext *ctx)
{
    g_assert (!ctx->timeout_id);
    g_assert (!ctx->uevent_id);
    g_assert (!ctx->cancellable_id);

    g_object_unref (ctx->udev);
    g_free (ctx->sysfs_path);
    g_free (ctx->peer_port);
    g_slice_free (WaitForDeviceContext, ctx);
}

GFile *
qfu_udev_helper_wait_for_device_finish (GAsyncResult  *res,
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
    WaitForDeviceContext *ctx;
    GFile                *file;

    ctx = (WaitForDeviceContext *) g_task_get_task_data (task);

    if (!g_str_equal (action, "add") && !g_str_equal (action, "move") && !g_str_equal (action, "change"))
        return;

    file = device_matches_sysfs_and_type (device, ctx->sysfs_path, ctx->device_type);
    if (!file && ctx->peer_port) {
        gchar *tmp, *path;

        tmp = g_build_filename (ctx->peer_port, "device", NULL);
        path = realpath (tmp, NULL);
        g_free (tmp);
        if (!path)
            return;

        file = device_matches_sysfs_and_type (device, path, ctx->device_type);
        g_debug ("[qfu-udev] peer lookup for %s: %s => %s",  g_udev_device_get_name (device), ctx->peer_port, path);
        g_free (path);
    }
    if (!file)
        return;

    g_debug ("[qfu-udev] waiting device (%s) matched: %s",
             qfu_udev_helper_device_type_to_string (ctx->device_type),
             g_udev_device_get_name (device));

    /* Disconnect this handler */
    g_signal_handler_disconnect (ctx->udev, ctx->uevent_id);
    ctx->uevent_id = 0;

    /* Disconnect the other handlers */
    g_cancellable_disconnect (g_task_get_cancellable (task), ctx->cancellable_id);
    ctx->cancellable_id = 0;
    g_source_remove (ctx->timeout_id);
    ctx->timeout_id = 0;

    g_task_return_pointer (task, file, g_object_unref);
    g_object_unref (task);
}

static gboolean
wait_for_device_timed_out (GTask *task)
{
    WaitForDeviceContext *ctx;

    ctx = (WaitForDeviceContext *) g_task_get_task_data (task);

    /* Disconnect this handler */
    ctx->timeout_id = 0;

    /* Disconnect the other handlers */
    g_cancellable_disconnect (g_task_get_cancellable (task), ctx->cancellable_id);
    ctx->cancellable_id = 0;
    g_signal_handler_disconnect (ctx->udev, ctx->uevent_id);
    ctx->uevent_id = 0;

    g_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_TIMED_OUT,
                             "waiting for device at '%s' timed out",
                             ctx->sysfs_path);
    g_object_unref (task);
    return FALSE;
}

static void
wait_for_device_cancelled (GCancellable *cancellable,
                           GTask        *task)
{
    WaitForDeviceContext *ctx;

    ctx = (WaitForDeviceContext *) g_task_get_task_data (task);

    /* Disconnect this handler */
    g_cancellable_disconnect (g_task_get_cancellable (task), ctx->cancellable_id);
    ctx->cancellable_id = 0;

    /* Disconnect the other handlers */
    g_source_remove (ctx->timeout_id);
    ctx->timeout_id = 0;
    g_signal_handler_disconnect (ctx->udev, ctx->uevent_id);
    ctx->uevent_id = 0;

    g_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_CANCELLED,
                             "waiting for device at '%s' cancelled",
                             ctx->sysfs_path);
    g_object_unref (task);
}

void
qfu_udev_helper_wait_for_device (QfuUdevHelperDeviceType  device_type,
                                 const gchar             *sysfs_path,
                                 const gchar             *peer_port,
                                 GCancellable            *cancellable,
                                 GAsyncReadyCallback      callback,
                                 gpointer                 user_data)
{
    GTask                *task;
    WaitForDeviceContext *ctx;

    ctx = g_slice_new0 (WaitForDeviceContext);
    ctx->device_type = device_type;
    ctx->sysfs_path = g_strdup (sysfs_path);
    ctx->peer_port = g_strdup (peer_port);

    if (ctx->device_type == QFU_UDEV_HELPER_DEVICE_TYPE_TTY)
        ctx->udev = g_udev_client_new (tty_subsys_list);
    else if (ctx->device_type == QFU_UDEV_HELPER_DEVICE_TYPE_CDC_WDM)
        ctx->udev = g_udev_client_new (cdc_wdm_subsys_list);
    else
        g_assert_not_reached ();

    task = g_task_new (NULL, cancellable, callback, user_data);
    g_task_set_task_data (task, ctx, (GDestroyNotify) wait_for_device_context_free);

    /* Monitor for device additions. */
    ctx->uevent_id = g_signal_connect (ctx->udev,
                                       "uevent",
                                       G_CALLBACK (handle_uevent),
                                       task);

    /* Allow cancellation */
    ctx->cancellable_id = g_cancellable_connect (cancellable,
                                                 (GCallback) wait_for_device_cancelled,
                                                 task,
                                                 NULL);

    /* And also, setup a timeout to avoid waiting forever. */
    ctx->timeout_id = g_timeout_add_seconds (WAIT_FOR_DEVICE_TIMEOUT_SECS,
                                             (GSourceFunc) wait_for_device_timed_out,
                                             task);

    /* Note: task ownership is shared between the signals and the timeout */
}

/******************************************************************************/

struct _QfuUdevHelperGenericMonitor {
    GUdevClient *udev;
};

void
qfu_udev_helper_generic_monitor_free (QfuUdevHelperGenericMonitor *self)
{
    g_object_unref (self->udev);
    g_slice_free (QfuUdevHelperGenericMonitor, self);
}

static void
handle_uevent_generic (GUdevClient *client,
                       const char  *action,
                       GUdevDevice *device,
                       GTask       *task)
{
    g_debug ("[qfu-udev] event: %s %s", action, g_udev_device_get_name (device));
}

QfuUdevHelperGenericMonitor *
qfu_udev_helper_generic_monitor_new (const gchar *sysfs_path)
{
    static const gchar *all_list[] = {
        "usbmisc", "usb",
        "tty",
        "net",
        NULL };

    QfuUdevHelperGenericMonitor *self;

    self = g_slice_new0 (QfuUdevHelperGenericMonitor);
    self->udev = g_udev_client_new (all_list);

    /* Monitor for device events. */
    g_signal_connect (self->udev, "uevent", G_CALLBACK (handle_uevent_generic), NULL);
    return self;
}

#endif /* WITH_UDEV */
