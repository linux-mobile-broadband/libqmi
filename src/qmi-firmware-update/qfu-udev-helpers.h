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

#ifndef QFU_UDEV_HELPERS_H
#define QFU_UDEV_HELPERS_H

#include "config.h"

#include <gio/gio.h>

G_BEGIN_DECLS

typedef enum {
    QFU_UDEV_HELPER_DEVICE_TYPE_TTY,
    QFU_UDEV_HELPER_DEVICE_TYPE_CDC_WDM,
    QFU_UDEV_HELPER_DEVICE_TYPE_LAST
} QfuUdevHelperDeviceType;

const gchar *qfu_udev_helper_device_type_to_string (QfuUdevHelperDeviceType type);

#if defined WITH_UDEV

gchar *qfu_udev_helper_find_by_file          (GFile        *file,
                                              GError      **error);
gchar *qfu_udev_helper_find_by_file_path     (const gchar  *path,
                                              GError      **error);
gchar *qfu_udev_helper_find_peer_port        (const gchar  *sysfs_path,
                                              GError      **error);
gchar *qfu_udev_helper_find_by_device_info   (guint16       vid,
                                              guint16       pid,
                                              guint         busnum,
                                              guint         devnum,
                                              GError      **error);

GList *qfu_udev_helper_list_devices           (QfuUdevHelperDeviceType   device_type,
                                               const gchar              *sysfs_path);

void   qfu_udev_helper_wait_for_device        (QfuUdevHelperDeviceType   device_type,
                                               const gchar              *sysfs_path,
                                               const gchar              *peer_port,
                                               GCancellable             *cancellable,
                                               GAsyncReadyCallback       callback,
                                               gpointer                  user_data);
GFile *qfu_udev_helper_wait_for_device_finish (GAsyncResult             *res,
                                               GError                  **error);

typedef struct _QfuUdevHelperGenericMonitor QfuUdevHelperGenericMonitor;
QfuUdevHelperGenericMonitor *qfu_udev_helper_generic_monitor_new  (const gchar *sysfs_path);
void                         qfu_udev_helper_generic_monitor_free (QfuUdevHelperGenericMonitor *self);

#endif

G_END_DECLS

#endif /* QFU_UDEV_HELPERS_H */
