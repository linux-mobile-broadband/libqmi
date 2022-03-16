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

#ifndef QFU_HELPERS_UDEV_H
#define QFU_HELPERS_UDEV_H

#include "config.h"
#include <gio/gio.h>

#if !defined WITH_UDEV
# error udev not found
#endif

G_BEGIN_DECLS

gchar *qfu_helpers_udev_find_by_file          (GFile        *file,
                                               GError      **error);
gchar *qfu_helpers_udev_find_by_file_path     (const gchar  *path,
                                               GError      **error);
gchar *qfu_helpers_udev_find_by_device_info   (guint16       vid,
                                               guint16       pid,
                                               guint         busnum,
                                               guint         devnum,
                                               GError      **error);

GList *qfu_helpers_udev_list_devices           (QfuHelpersDeviceType   device_type,
                                                QfuHelpersDeviceMode   device_mode,
                                                const gchar           *sysfs_path);

void   qfu_helpers_udev_wait_for_device        (QfuHelpersDeviceType   device_type,
                                                QfuHelpersDeviceMode   device_mode,
                                                const gchar           *sysfs_path,
                                                const gchar           *peer_port,
                                                GCancellable          *cancellable,
                                                GAsyncReadyCallback    callback,
                                                gpointer               user_data);
GFile *qfu_helpers_udev_wait_for_device_finish (GAsyncResult          *res,
                                                GError               **error);

typedef struct _QfuHelpersUdevGenericMonitor QfuHelpersUdevGenericMonitor;
QfuHelpersUdevGenericMonitor *qfu_helpers_udev_generic_monitor_new  (const gchar *sysfs_path);
void                          qfu_helpers_udev_generic_monitor_free (QfuHelpersUdevGenericMonitor *self);

G_END_DECLS

#endif /* QFU_HELPERS_UDEV_H */
