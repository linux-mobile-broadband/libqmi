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
gchar *
qfu_helpers_sysfs_find_by_file (GFile   *file,
                                GError **error)
{
    g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "not implemented");
    return NULL;
}

gchar *
qfu_helpers_sysfs_find_by_file_path (const gchar  *path,
                                     GError      **error)
{
    g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "not implemented");
    return NULL;
}

gchar *
qfu_helpers_sysfs_find_by_device_info (guint16   vid,
                                       guint16   pid,
                                       guint     busnum,
                                       guint     devnum,
                                       GError  **error)
{
    g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "not implemented");
    return NULL;
}

/******************************************************************************/

GList *
qfu_helpers_sysfs_list_devices (QfuHelpersDeviceType  device_type,
                                const gchar          *sysfs_path)
{
    return NULL;
}

/******************************************************************************/

GFile *
qfu_helpers_sysfs_wait_for_device_finish (GAsyncResult  *res,
                                          GError       **error)
{
    return g_task_propagate_pointer (G_TASK (res), error);
}

void
qfu_helpers_sysfs_wait_for_device (QfuHelpersDeviceType   device_type,
                                   const gchar           *sysfs_path,
                                   const gchar           *peer_port,
                                   GCancellable          *cancellable,
                                   GAsyncReadyCallback    callback,
                                   gpointer               user_data)
{
    GTask *task;

    task = g_task_new (NULL, cancellable, callback, user_data);
    g_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_FAILED, "not implemented");
    g_object_unref (task);
}
