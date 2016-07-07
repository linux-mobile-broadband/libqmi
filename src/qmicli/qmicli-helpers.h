/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * qmicli -- Command line interface to control QMI devices
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
 * Copyright (C) 2015 Velocloud Inc.
 * Copyright (C) 2012-2015 Aleksander Morgado <aleksander@aleksander.es>
 */

#include <glib.h>

#include <libqmi-glib.h>

#ifndef __QMICLI_HELPERS_H__
#define __QMICLI_HELPERS_H__

gchar *qmicli_get_raw_data_printable (const GArray *data,
                                      gsize max_line_length,
                                      const gchar *new_line_prefix);

gboolean qmicli_read_dms_uim_pin_id_from_string              (const gchar *str,
                                                              QmiDmsUimPinId *out);
gboolean qmicli_read_uim_pin_id_from_string                  (const gchar *str,
                                                              QmiUimPinId *out);
gboolean qmicli_read_operating_mode_from_string              (const gchar *str,
                                                              QmiDmsOperatingMode *out);
gboolean qmicli_read_rat_mode_pref_from_string               (const gchar *str,
                                                              QmiNasRatModePreference *out);
gboolean qmicli_read_facility_from_string                    (const gchar *str,
                                                              QmiDmsUimFacility *out);
gboolean qmicli_read_enable_disable_from_string              (const gchar *str,
                                                              gboolean *out);
gboolean qmicli_read_firmware_id_from_string                 (const gchar *str,
                                                              QmiDmsFirmwareImageType *out_type,
                                                              guint *out_index);
gboolean qmicli_read_binary_array_from_string                (const gchar *str,
                                                              GArray **out);
gboolean qmicli_read_pdc_configuration_type_from_string      (const gchar *str,
                                                              QmiPdcConfigurationType *out);
gboolean qmicli_read_radio_interface_from_string             (const gchar *str,
                                                              QmiNasRadioInterface *out);
gboolean qmicli_read_net_open_flags_from_string              (const gchar *str,
                                                              QmiDeviceOpenFlags *out);
gboolean qmicli_read_expected_data_format_from_string        (const gchar *str,
                                                              QmiDeviceExpectedDataFormat *out);
gboolean qmicli_read_link_layer_protocol_from_string         (const gchar *str,
                                                              QmiWdaLinkLayerProtocol *out);
gboolean qmicli_read_autoconnect_setting_from_string         (const gchar *str,
                                                              QmiWdsAutoconnectSetting *out);
gboolean qmicli_read_autoconnect_setting_roaming_from_string (const gchar *str,
                                                              QmiWdsAutoconnectSettingRoaming *out);
gboolean qmicli_read_authentication_from_string              (const gchar *str,
                                                              QmiWdsAuthentication *out);

gboolean qmicli_read_non_empty_string           (const gchar *str,
                                                 const gchar *description,
                                                 gchar **out);
gboolean qmicli_read_uint_from_string           (const gchar *str,
                                                 guint *out);
gboolean qmicli_read_uint16_from_string         (const gchar *str,
                                                 guint16 *out);
gboolean qmicli_read_uint8_from_string         (const gchar *str,
                                                guint8 *out);
gboolean qmicli_read_yes_no_from_string         (const gchar *str,
                                                 gboolean *out);

gchar *qmicli_get_supported_messages_list (const guint8 *data,
                                           gsize len);

const char *qmicli_earfcn_to_eutra_band_string (guint16 earfcn);

typedef gboolean (*QmiParseKeyValueForeachFn) (const gchar *key,
                                               const gchar *value,
                                               GError **error,
                                               gpointer user_data);

gboolean qmicli_parse_key_value_string (const gchar *str,
                                        GError **error,
                                        QmiParseKeyValueForeachFn callback,
                                        gpointer user_data);

#endif /* __QMICLI_H__ */
