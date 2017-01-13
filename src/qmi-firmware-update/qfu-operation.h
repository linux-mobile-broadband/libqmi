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

#ifndef QFU_OPERATION_H
#define QFU_OPERATION_H

#include <glib.h>
#include "qfu-device-selection.h"

G_BEGIN_DECLS

gboolean qfu_operation_update_run     (const gchar        **images,
                                       QfuDeviceSelection  *device_selection,
                                       const gchar         *firmware_version,
                                       const gchar         *config_version,
                                       const gchar         *carrier,
                                       gboolean             device_open_proxy,
                                       gboolean             device_open_mbim,
                                       gboolean             force);
gboolean qfu_operation_update_qdl_run (const gchar        **images,
                                       QfuDeviceSelection  *device_selection);
gboolean qfu_operation_verify_run     (const gchar        **images);
gboolean qfu_operation_reset_run      (QfuDeviceSelection  *device_selection,
                                       gboolean             device_open_proxy,
                                       gboolean             device_open_mbim);

G_END_DECLS

#endif /* QFU_OPERATION_H */
