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

#ifndef QFU_UDEV_HELPERS_H
#define QFU_UDEV_HELPERS_H

#include <gio/gio.h>

G_BEGIN_DECLS

gchar *qfu_udev_helper_get_udev_device_sysfs_path (GUdevDevice         *device,
                                                   GError             **error);
gchar *qfu_udev_helper_get_sysfs_path             (GFile               *file,
                                                   const gchar *const  *subsys,
                                                   GError             **error);

G_END_DECLS

#endif /* QFU_UDEV_HELPERS_H */
