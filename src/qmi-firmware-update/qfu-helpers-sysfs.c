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
 * Copyright (C) 2022 VMware, Inc.
 */

#include "config.h"
#include <stdlib.h>
#include <gio/gio.h>

#if defined WITH_UDEV
# error udev found
#endif

#include "qfu-helpers.h"
#include "qfu-helpers-sysfs.h"

/******************************************************************************/

static const gchar *tty_subsys_list[]     = { "tty", NULL };
static const gchar *cdc_wdm_subsys_list[] = { "usbmisc", "usb", NULL };

static gboolean
has_sysfs_attribute (const gchar *path,
                     const gchar *attribute)
{
    g_autofree gchar *aux_filepath = NULL;

    aux_filepath = g_strdup_printf ("%s/%s", path, attribute);
    return g_file_test (aux_filepath, G_FILE_TEST_EXISTS);
}

static gchar *
read_sysfs_attribute_as_string (const gchar *path,
                                const gchar *attribute)
{
    g_autofree gchar *aux = NULL;
    gchar            *contents = NULL;

    aux = g_strdup_printf ("%s/%s", path, attribute);
    if (g_file_get_contents (aux, &contents, NULL, NULL)) {
        g_strdelimit (contents, "\r\n", ' ');
        g_strstrip (contents);
    }
    return contents;
}

static gulong
read_sysfs_attribute_as_num (const gchar *path,
                             const gchar *attribute,
                             guint        base)
{
    g_autofree gchar *contents = NULL;
    gulong            val = 0;

    contents = read_sysfs_attribute_as_string (path, attribute);
    if (contents)
        val = strtoul (contents, NULL, base);
    return val;
}

static gchar *
read_sysfs_attribute_link_basename (const gchar *path,
                                    const gchar *attribute)
{
    g_autofree gchar *aux_filepath = NULL;
    g_autofree gchar *canonicalized_path = NULL;

    aux_filepath = g_strdup_printf ("%s/%s", path, attribute);
    if (!g_file_test (aux_filepath, G_FILE_TEST_EXISTS))
        return NULL;

    canonicalized_path = realpath (aux_filepath, NULL);
    return g_path_get_basename (canonicalized_path);
}

static gboolean
get_device_details (const gchar  *port_sysfs_path,
                    gchar       **out_sysfs_path,
                    guint16      *out_vid,
                    guint16      *out_pid,
                    guint        *out_busnum,
                    guint        *out_devnum,
                    GError      **error)
{
    g_autofree gchar *iter = NULL;
    g_autofree gchar *physdev_sysfs_path = NULL;
    gulong            aux;

    if (out_vid)
        *out_vid = 0;
    if (out_pid)
        *out_pid = 0;
    if (out_busnum)
        *out_busnum = 0;
    if (out_devnum)
        *out_devnum = 0;
    if (out_sysfs_path)
        *out_sysfs_path = NULL;

    iter = realpath (port_sysfs_path, NULL);

    while (iter && (g_strcmp0 (iter, "/") != 0)) {
        gchar *parent;

        /* is this the USB physdev? */
        if (has_sysfs_attribute (iter, "idVendor")) {
            /* stop traversing as soon as the physical device is found */
            physdev_sysfs_path = g_steal_pointer (&iter);
            break;
        }

        parent = g_path_get_dirname (iter);
        g_clear_pointer (&iter, g_free);
        iter = parent;
    }

    if (!physdev_sysfs_path) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "couldn't find parent physical USB device");
        return FALSE;
    }

    if (out_vid) {
        aux = read_sysfs_attribute_as_num (physdev_sysfs_path, "idVendor", 16);
        if (aux <= G_MAXUINT16)
            *out_vid = (guint16) aux;
    }

    if (out_pid) {
        aux = read_sysfs_attribute_as_num (physdev_sysfs_path, "idProduct", 16);
        if (aux <= G_MAXUINT16)
            *out_pid = (guint16) aux;
    }

    if (out_busnum) {
        aux = read_sysfs_attribute_as_num (physdev_sysfs_path, "busnum", 10);
        if (aux <= G_MAXUINT)
            *out_busnum = (guint16) aux;
    }

    if (out_devnum) {
        aux = read_sysfs_attribute_as_num (physdev_sysfs_path, "devnum", 10);
        if (aux <= G_MAXUINT)
            *out_devnum = (guint16) aux;
    }

    if (out_sysfs_path)
        *out_sysfs_path = g_steal_pointer (&physdev_sysfs_path);

    return TRUE;
}

static gboolean
get_interface_details (const gchar  *port_sysfs_path,
                       gchar       **out_driver,
                       GError      **error)
{
    g_autofree gchar *iter = NULL;
    g_autofree gchar *interface_sysfs_path = NULL;

    if (out_driver)
        *out_driver = NULL;

    iter = realpath (port_sysfs_path, NULL);

    while (iter && (g_strcmp0 (iter, "/") != 0)) {
        gchar *parent;

        /* is this the USB interface? */
        if (has_sysfs_attribute (iter, "bInterfaceClass")) {
            /* stop traversing as soon as the physical device is found */
            interface_sysfs_path = g_steal_pointer (&iter);
            break;
        }

        parent = g_path_get_dirname (iter);
        g_clear_pointer (&iter, g_free);
        iter = parent;
    }

    if (!interface_sysfs_path) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "couldn't find parent interface USB device");
        return FALSE;
    }

    if (out_driver)
        *out_driver = read_sysfs_attribute_link_basename (interface_sysfs_path, "driver");

    return TRUE;
}

/******************************************************************************/

gchar *
qfu_helpers_sysfs_find_by_file (GFile   *file,
                                GError **error)
{
    const gchar      **subsys_list = NULL;
    g_autofree gchar  *physdev_sysfs_path = NULL;
    g_autofree gchar  *found_port_sysfs_path = NULL;
    g_autofree gchar  *basename = NULL;
    guint              i;

    basename = g_file_get_basename (file);
    if (!basename) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "couldn't get filename");
        return NULL;
    }

    if (g_str_has_prefix (basename, "tty"))
        subsys_list = tty_subsys_list;
    else if  (g_str_has_prefix (basename, "cdc-wdm"))
        subsys_list = cdc_wdm_subsys_list;
    else {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "unknown device file type");
        return NULL;
    }

    for (i = 0; !found_port_sysfs_path && subsys_list[i]; i++) {
        g_autofree gchar *tmp = NULL;
        g_autofree gchar *port_sysfs_path = NULL;

        tmp = g_strdup_printf ("/sys/class/%s/%s", subsys_list[i], basename);

        port_sysfs_path = realpath (tmp, NULL);
        if (port_sysfs_path && g_file_test (port_sysfs_path, G_FILE_TEST_EXISTS))
            found_port_sysfs_path = g_steal_pointer (&port_sysfs_path);
    }

    if (!found_port_sysfs_path) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "device not found");
        return NULL;
    }

    if (!get_device_details (found_port_sysfs_path,
                             &physdev_sysfs_path, NULL, NULL, NULL, NULL,
                             error))
        return NULL;

    g_debug ("[qfu-sysfs] sysfs path for '%s' found: %s", basename, physdev_sysfs_path);
    return g_steal_pointer (&physdev_sysfs_path);
}

/******************************************************************************/

static gboolean
device_already_added (GPtrArray   *ptr,
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
find_by_device_info_in_subsystem (GPtrArray    *sysfs_paths,
                                  const gchar  *subsystem,
                                  guint16       vid,
                                  guint16       pid,
                                  guint         busnum,
                                  guint         devnum)
{
    g_autofree gchar           *subsys_sysfs_path = NULL;
    g_autoptr(GFile)            subsys_sysfs_file = NULL;
    g_autoptr(GFileEnumerator)  direnum = NULL;

    subsys_sysfs_path = g_strdup_printf ("/sys/class/%s", subsystem);
    subsys_sysfs_file = g_file_new_for_path (subsys_sysfs_path);
    direnum = g_file_enumerate_children (subsys_sysfs_file,
                                         G_FILE_ATTRIBUTE_STANDARD_NAME,
                                         G_FILE_QUERY_INFO_NONE,
                                         NULL,
                                         NULL);
    if (direnum) {
        while (TRUE) {
            GFileInfo        *info = NULL;
            g_autoptr(GFile)  child = NULL;
            g_autofree gchar *child_path = NULL;
            guint16           device_vid = 0;
            guint16           device_pid = 0;
            guint             device_busnum = 0;
            guint             device_devnum = 0;
            g_autofree gchar *device_sysfs_path = NULL;

            if (!g_file_enumerator_iterate (direnum, &info, NULL, NULL, NULL) || !info)
                break;

            child = g_file_enumerator_get_child (direnum, info);
            child_path = g_file_get_path (child);
            if (get_device_details (child_path,
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
                    (!device_already_added (sysfs_paths, device_sysfs_path)))
                    g_ptr_array_add (sysfs_paths, g_steal_pointer (&device_sysfs_path));
            }
        }
    }

    return sysfs_paths;
}

gchar *
qfu_helpers_sysfs_find_by_device_info (guint16   vid,
                                       guint16   pid,
                                       guint     busnum,
                                       guint     devnum,
                                       GError  **error)
{
    g_autoptr(GPtrArray) sysfs_paths = NULL;
    g_autoptr(GString)   match_str = NULL;
    guint                i;

    sysfs_paths = g_ptr_array_new_with_free_func (g_free);

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
        sysfs_paths = find_by_device_info_in_subsystem (sysfs_paths,
                                                        tty_subsys_list[i],
                                                        vid, pid, busnum, devnum);

    for (i = 0; cdc_wdm_subsys_list[i]; i++)
        sysfs_paths = find_by_device_info_in_subsystem (sysfs_paths,
                                                        cdc_wdm_subsys_list[i],
                                                        vid, pid, busnum, devnum);

    for (i = 0; i < sysfs_paths->len; i++)
        g_debug ("[%s] sysfs path: %s", match_str->str, (gchar *) g_ptr_array_index (sysfs_paths, i));

    if (sysfs_paths->len == 0) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "no device found with matching criteria: %s",
                     match_str->str);
        return NULL;
    }

    if (sysfs_paths->len > 1) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "multiple devices (%u) found with matching criteria: %s",
                     sysfs_paths->len, match_str->str);
        return NULL;
    }

    return g_strdup (g_ptr_array_index (sysfs_paths, 0));
}

/******************************************************************************/

static GFile *
device_matches_sysfs_and_type (GFile                *file,
                               const gchar          *sysfs_path,
                               QfuHelpersDeviceType  type)
{
    g_autofree gchar *port_sysfs_path = NULL;
    g_autofree gchar *device_sysfs_path = NULL;
    g_autofree gchar *device_driver = NULL;
    g_autofree gchar *device_path = NULL;

    port_sysfs_path = g_file_get_path (file);
    if (!get_device_details (port_sysfs_path,
                             &device_sysfs_path, NULL, NULL, NULL, NULL,
                             NULL))
        return NULL;

    if (!device_sysfs_path)
        return NULL;

    if (g_strcmp0 (device_sysfs_path, sysfs_path) != 0)
        return NULL;

    if (!get_interface_details (port_sysfs_path, &device_driver, NULL))
        return NULL;

    switch (type) {
    case QFU_HELPERS_DEVICE_TYPE_TTY:
        if (g_strcmp0 (device_driver, "qcserial") != 0)
            return NULL;
        break;
    case QFU_HELPERS_DEVICE_TYPE_CDC_WDM:
        if (g_strcmp0 (device_driver, "qmi_wwan") != 0 && g_strcmp0 (device_driver, "cdc_mbim") != 0)
            return NULL;
        break;
    case QFU_HELPERS_DEVICE_TYPE_LAST:
    default:
        g_assert_not_reached ();
    }

    device_path = g_strdup_printf ("/dev/%s", g_file_get_basename (file));
    return g_file_new_for_path (device_path);
}

GList *
qfu_helpers_sysfs_list_devices (QfuHelpersDeviceType  device_type,
                                const gchar          *sysfs_path)
{
    const gchar **subsys_list = NULL;
    guint         i;
    GList        *files = NULL;

    switch (device_type) {
    case QFU_HELPERS_DEVICE_TYPE_TTY:
        subsys_list = tty_subsys_list;
        break;
    case QFU_HELPERS_DEVICE_TYPE_CDC_WDM:
        subsys_list = cdc_wdm_subsys_list;
        break;
    case QFU_HELPERS_DEVICE_TYPE_LAST:
    default:
        g_assert_not_reached ();
    }

    for (i = 0; subsys_list[i]; i++) {
        g_autofree gchar           *subsys_sysfs_path = NULL;
        g_autoptr(GFile)            subsys_sysfs_file = NULL;
        g_autoptr(GFileEnumerator)  direnum = NULL;

        subsys_sysfs_path = g_strdup_printf ("/sys/class/%s", subsys_list[i]);
        subsys_sysfs_file = g_file_new_for_path (subsys_sysfs_path);
        direnum = g_file_enumerate_children (subsys_sysfs_file,
                                             G_FILE_ATTRIBUTE_STANDARD_NAME,
                                             G_FILE_QUERY_INFO_NONE,
                                             NULL,
                                             NULL);
        if (!direnum)
            continue;

        while (TRUE) {
            GFileInfo        *info = NULL;
            g_autoptr(GFile)  child = NULL;
            g_autoptr(GFile)  devfile = NULL;

            if (!g_file_enumerator_iterate (direnum, &info, NULL, NULL, NULL) || !info)
                break;

            child = g_file_enumerator_get_child (direnum, info);
            devfile = device_matches_sysfs_and_type (child, sysfs_path, device_type);
            if (devfile)
                files = g_list_prepend (files, g_steal_pointer (&devfile));
        }
    }

    return files;
}

/******************************************************************************/

/* Check for the new port addition every 10s */
#define WAIT_FOR_DEVICE_CHECK_SECS 10

/* And up to 12 attempts to check (so 120s in total) */
#define WAIT_FOR_DEVICE_CHECK_ATTEMPTS 12

typedef struct {
    QfuHelpersDeviceType  device_type;
    gchar                *sysfs_path;
    gchar                *peer_port;
    guint                 check_attempts;
    guint                 timeout_id;
    gulong                cancellable_id;
} WaitForDeviceContext;

static void
wait_for_device_context_free (WaitForDeviceContext *ctx)
{
    g_assert (!ctx->timeout_id);
    g_assert (!ctx->cancellable_id);

    g_free (ctx->sysfs_path);
    g_free (ctx->peer_port);
    g_slice_free (WaitForDeviceContext, ctx);
}

GFile *
qfu_helpers_sysfs_wait_for_device_finish (GAsyncResult  *res,
                                          GError       **error)
{
    return G_FILE (g_task_propagate_pointer (G_TASK (res), error));
}

static GFile *
wait_for_device_lookup (QfuHelpersDeviceType  device_type,
                        const gchar          *sysfs_path,
                        const gchar          *peer_port)
{
    GList *devices;
    GFile *file;

    devices = qfu_helpers_sysfs_list_devices (device_type, sysfs_path);
    if (!devices) {
        g_autofree gchar *tmp = NULL;
        g_autofree gchar *path = NULL;

        if (!peer_port)
            return NULL;

        /* Check with peer port */
        tmp = g_build_filename (peer_port, "device", NULL);
        path = realpath (tmp, NULL);
        if (path) {
            g_debug ("[qfu-sysfs] peer lookup: %s => %s", peer_port, path);
            devices = qfu_helpers_sysfs_list_devices (device_type, path);
            if (!devices)
                return NULL;
        }
    }

    if (g_list_length (devices) > 1)
        g_warning ("[qfu-sysfs] waiting device (%s) matched multiple times",
                   qfu_helpers_device_type_to_string (device_type));

    /* Take the first one from the list */
    file = G_FILE (g_object_ref (devices->data));
    g_list_free_full (devices, g_object_unref);

    return file;
}

static gboolean
wait_for_device_check (GTask *task)
{
    WaitForDeviceContext *ctx;
    g_autoptr(GFile)      device = NULL;
    g_autofree gchar     *device_name = NULL;

    ctx = (WaitForDeviceContext *) g_task_get_task_data (task);

    /* Check devices matching */
    device = wait_for_device_lookup (ctx->device_type, ctx->sysfs_path, ctx->peer_port);
    if (!device) {
        /* No devices found */
        if (ctx->check_attempts == WAIT_FOR_DEVICE_CHECK_ATTEMPTS) {
            /* Disconnect this handler */
            ctx->timeout_id = 0;

            /* Disconnect the other handlers */
            g_cancellable_disconnect (g_task_get_cancellable (task), ctx->cancellable_id);
            ctx->cancellable_id = 0;

            g_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_TIMED_OUT,
                                     "waiting for device at '%s' timed out",
                                     ctx->sysfs_path);
            g_object_unref (task);
            return G_SOURCE_REMOVE;
        }

        /* go on with next attempt */
        ctx->check_attempts++;
        return G_SOURCE_CONTINUE;
    }

    device_name = g_file_get_basename (device);
    g_debug ("[qfu-sysfs] waiting device (%s) matched: %s",
             qfu_helpers_device_type_to_string (ctx->device_type),
             device_name);

    /* Disconnect the other handlers */
    g_cancellable_disconnect (g_task_get_cancellable (task), ctx->cancellable_id);
    ctx->cancellable_id = 0;

    /* Disconnect this handler */
    ctx->timeout_id = 0;

    g_task_return_pointer (task, g_steal_pointer (&device), g_object_unref);
    g_object_unref (task);

    return G_SOURCE_REMOVE;
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

    g_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_CANCELLED,
                             "waiting for device at '%s' cancelled",
                             ctx->sysfs_path);
    g_object_unref (task);
}

void
qfu_helpers_sysfs_wait_for_device (QfuHelpersDeviceType   device_type,
                                   const gchar           *sysfs_path,
                                   const gchar           *peer_port,
                                   GCancellable          *cancellable,
                                   GAsyncReadyCallback    callback,
                                   gpointer               user_data)
{
    GTask                *task;
    WaitForDeviceContext *ctx;

    ctx = g_slice_new0 (WaitForDeviceContext);
    ctx->device_type = device_type;
    ctx->sysfs_path = g_strdup (sysfs_path);
    ctx->peer_port = g_strdup (peer_port);

    task = g_task_new (NULL, cancellable, callback, user_data);
    g_task_set_task_data (task, ctx, (GDestroyNotify) wait_for_device_context_free);

    /* Allow cancellation */
    ctx->cancellable_id = g_cancellable_connect (cancellable,
                                                 (GCallback) wait_for_device_cancelled,
                                                 task,
                                                 NULL);

    /* Schedule lookup of of port every once in a while */
    ctx->timeout_id = g_timeout_add_seconds (WAIT_FOR_DEVICE_CHECK_SECS,
                                             (GSourceFunc) wait_for_device_check,
                                             task);

    /* Note: task ownership is shared between the signals and the timeout */
}
