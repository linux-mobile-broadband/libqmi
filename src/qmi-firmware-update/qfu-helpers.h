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

#ifndef QFU_HELPERS_H
#define QFU_HELPERS_H

#include "config.h"

#include <gio/gio.h>

G_BEGIN_DECLS

typedef enum {
    QFU_HELPERS_DEVICE_TYPE_TTY,
    QFU_HELPERS_DEVICE_TYPE_CDC_WDM,
    QFU_HELPERS_DEVICE_TYPE_LAST
} QfuHelpersDeviceType;

const gchar *qfu_helpers_device_type_to_string (QfuHelpersDeviceType type);

/******************************************************************************/

gchar *qfu_helpers_find_peer_port (const gchar  *sysfs_path,
                                   GError      **error);

/******************************************************************************/

#if defined WITH_UDEV
# include "qfu-helpers-udev.h"
# define qfu_helpers_find_by_file           qfu_helpers_udev_find_by_file
# define qfu_helpers_find_by_file_path      qfu_helpers_udev_find_by_file_path
# define qfu_helpers_find_by_device_info    qfu_helpers_udev_find_by_device_info
# define qfu_helpers_list_devices           qfu_helpers_udev_list_devices
# define qfu_helpers_wait_for_device        qfu_helpers_udev_wait_for_device
# define qfu_helpers_wait_for_device_finish qfu_helpers_udev_wait_for_device_finish
#endif

G_END_DECLS

#endif /* QFU_HELPERS_H */
