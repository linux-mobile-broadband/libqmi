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
