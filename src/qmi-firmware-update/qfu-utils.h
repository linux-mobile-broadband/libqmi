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
 * Copyright (C) 2016 Bjørn Mork <bjorn@mork.no>
 * Copyright (C) 2016 Zodiac Inflight Innovations
 * Copyright (C) 2016-2017 Aleksander Morgado <aleksander@aleksander.es>
 */

#ifndef QFU_UTILS_H
#define QFU_UTILS_H

#include "config.h"

#include <glib.h>
#include <gio/gio.h>
#include <libqmi-glib.h>

G_BEGIN_DECLS

gchar *qfu_utils_str_hex (gconstpointer mem,
                          gsize         size,
                          gchar         delimiter);

gchar *qfu_utils_get_firmware_image_unique_id_printable (const GArray *unique_id);

guint16 qfu_utils_crc16 (const guint8 *buffer,
                         gsize         len);

gboolean qfu_utils_parse_cwe_version_string (const gchar  *version,
                                             gchar       **firmware_version,
                                             gchar       **config_version,
                                             gchar       **carrier,
                                             GError      **error);

void     qfu_utils_new_client_dms        (GFile                *cdc_wdm_file,
                                          guint                 retries,
                                          QmiDeviceOpenFlags    device_open_flags,
                                          gboolean              load_capabilities,
                                          GCancellable         *cancellable,
                                          GAsyncReadyCallback   callback,
                                          gpointer              user_data);
gboolean qfu_utils_new_client_dms_finish (GAsyncResult         *res,
                                          QmiDevice           **qmi_device,
                                          QmiClientDms        **qmi_client,
                                          gchar               **revision,
                                          gboolean             *supports_stored_image_management,
                                          guint8               *max_storage_index,
                                          gboolean             *supports_firmware_preference_management,
                                          QmiMessageDmsGetFirmwarePreferenceOutput **firmware_preference,
                                          QmiMessageDmsSwiGetCurrentFirmwareOutput **current_firmware,
                                          GError              **error);

void     qfu_utils_power_cycle        (QmiClientDms         *qmi_client,
                                       GCancellable         *cancellable,
                                       GAsyncReadyCallback   callback,
                                       gpointer              user_data);
gboolean qfu_utils_power_cycle_finish (QmiClientDms         *qmi_client,
                                       GAsyncResult         *res,
                                       GError              **error);

#if defined MM_RUNTIME_CHECK_ENABLED

gboolean qfu_utils_modemmanager_running (gboolean  *mm_running,
                                         GError   **error);

#endif

G_END_DECLS

#endif /* QFU_UTILS_H */
