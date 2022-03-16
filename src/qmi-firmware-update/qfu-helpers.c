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
 * Copyright (C) 2016-2022 Aleksander Morgado <aleksander@aleksander.es>
 */

#include "config.h"
#include <stdlib.h>
#include "qfu-helpers.h"

/******************************************************************************/

static const gchar *device_type_str[] = {
    [QFU_HELPERS_DEVICE_TYPE_TTY]     = "tty",
    [QFU_HELPERS_DEVICE_TYPE_CDC_WDM] = "cdc-wdm",
};

G_STATIC_ASSERT (G_N_ELEMENTS (device_type_str) == QFU_HELPERS_DEVICE_TYPE_LAST);

const gchar *
qfu_helpers_device_type_to_string (QfuHelpersDeviceType type)
{
    return device_type_str[type];
}

/******************************************************************************/

static const gchar *device_mode_str[] = {
    [QFU_HELPERS_DEVICE_MODE_UNKNOWN]  = "unknown",
    [QFU_HELPERS_DEVICE_MODE_MODEM]    = "modem",
    [QFU_HELPERS_DEVICE_MODE_DOWNLOAD] = "download",
};

G_STATIC_ASSERT (G_N_ELEMENTS (device_mode_str) == QFU_HELPERS_DEVICE_MODE_LAST);

const gchar *
qfu_helpers_device_mode_to_string (QfuHelpersDeviceMode mode)
{
    return device_mode_str[mode];
}

/******************************************************************************/

gchar *
qfu_helpers_find_by_file_path (const gchar  *path,
                               GError      **error)
{
    GFile *file;
    gchar *sysfs_path;

    file = g_file_new_for_path (path);
    sysfs_path = qfu_helpers_find_by_file (file, error);
    g_object_unref (file);
    return sysfs_path;
}

gchar *
qfu_helpers_find_peer_port (const gchar  *sysfs_path,
                            GError      **error)
{
    gchar *tmp, *path;

    tmp = g_build_filename (sysfs_path, "port", "peer", NULL);
    path = realpath (tmp, NULL);
    g_free (tmp);
    if (!path)
        return NULL;

    g_debug ("[qfu-helpers] peer port for '%s' found: %s", sysfs_path, path);
    return path;
}
