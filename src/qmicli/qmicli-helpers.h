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
 * Copyright (C) 2012-2017 Aleksander Morgado <aleksander@aleksander.es>
 */

#include <glib.h>

#include <libqmi-glib.h>

#ifndef __QMICLI_HELPERS_H__
#define __QMICLI_HELPERS_H__

/* Common helpers to read enums from strings */
#define QMICLI_ENUM_LIST                                                                                                                               \
    QMICLI_ENUM_LIST_ITEM (QmiDmsOperatingMode,                         dms_operating_mode,                           "operating mode")                \
    QMICLI_ENUM_LIST_ITEM (QmiDmsUimFacility,                           dms_uim_facility,                             "facility")                      \
    QMICLI_ENUM_LIST_ITEM (QmiPdcConfigurationType,                     pdc_configuration_type,                       "configuration type")            \
    QMICLI_ENUM_LIST_ITEM (QmiNasRadioInterface,                        nas_radio_interface,                          "radio interface")               \
    QMICLI_ENUM_LIST_ITEM (QmiDeviceExpectedDataFormat,                 device_expected_data_format,                  "device expected data format")   \
    QMICLI_ENUM_LIST_ITEM (QmiWdaLinkLayerProtocol,                     wda_link_layer_protocol,                      "link layer protocol")           \
    QMICLI_ENUM_LIST_ITEM (QmiWdaDataAggregationProtocol,               wda_data_aggregation_protocol,                "data aggregation protocol")     \
    QMICLI_ENUM_LIST_ITEM (QmiDataEndpointType,                         data_endpoint_type,                           "data endpoint type")            \
    QMICLI_ENUM_LIST_ITEM (QmiWdsAutoconnectSetting,                    wds_autoconnect_setting,                      "autoconnect setting")           \
    QMICLI_ENUM_LIST_ITEM (QmiWdsAutoconnectSettingRoaming,             wds_autoconnect_setting_roaming,              "autoconnect setting roaming")   \
    QMICLI_ENUM_LIST_ITEM (QmiDmsBootImageDownloadMode,                 dms_boot_image_download_mode,                 "boot image download mode")      \
    QMICLI_ENUM_LIST_ITEM (QmiDmsHpDeviceMode,                          dms_hp_device_mode,                           "hp device mode")                \
    QMICLI_ENUM_LIST_ITEM (QmiDmsSwiUsbComposition,                     dms_swi_usb_composition,                      "swi usb composition")           \
    QMICLI_ENUM_LIST_ITEM (QmiDmsFoxconnDeviceMode,                     dms_foxconn_device_mode,                      "foxconn device mode")           \
    QMICLI_ENUM_LIST_ITEM (QmiDmsFoxconnFirmwareVersionType,            dms_foxconn_firmware_version_type,            "foxconn firmware version type") \
    QMICLI_ENUM_LIST_ITEM (QmiUimSessionType,                           uim_session_type,                             "session type")                  \
    QMICLI_ENUM_LIST_ITEM (QmiDsdApnType,                               dsd_apn_type,                                 "apn type")                      \
    QMICLI_ENUM_LIST_ITEM (QmiDmsMacType,                               dms_mac_type,                                 "mac address type")              \
    QMICLI_ENUM_LIST_ITEM (QmiSarRfState,                               sar_rf_state,                                 "sar rf state")                  \
    QMICLI_ENUM_LIST_ITEM (QmiSioPort,                                  sio_port,                                     "sio port")                      \
    QMICLI_ENUM_LIST_ITEM (QmiLocOperationMode,                         loc_operation_mode,                           "operation mode")                \
    QMICLI_ENUM_LIST_ITEM (QmiLocLockType,                              loc_lock_type,                                "lock type")                     \
    QMICLI_ENUM_LIST_ITEM (QmiUimCardApplicationPersonalizationFeature, uim_card_application_personalization_feature, "personalization feature" )      \
    QMICLI_ENUM_LIST_ITEM (QmiUimDepersonalizationOperation,            uim_depersonalization_operation,              "depersonalization operation" )

#define QMICLI_ENUM_LIST_ITEM(TYPE,TYPE_UNDERSCORE,DESCR)        \
    gboolean qmicli_read_## TYPE_UNDERSCORE ##_from_string (const gchar *str, TYPE *out);
QMICLI_ENUM_LIST
#undef QMICLI_ENUM_LIST_ITEM

/* Common helpers to read flags from strings */
#define QMICLI_FLAGS_LIST                                                                                                          \
    QMICLI_FLAGS_LIST_ITEM (QmiDeviceOpenFlags,                   device_open_flags,                     "device open flags")      \
    QMICLI_FLAGS_LIST_ITEM (QmiDeviceAddLinkFlags,                device_add_link_flags,                 "device add link flags")  \
    QMICLI_FLAGS_LIST_ITEM (QmiLocNmeaType,                       loc_nmea_type,                         "NMEA type")              \
    QMICLI_FLAGS_LIST_ITEM (QmiNasPlmnAccessTechnologyIdentifier, nas_plmn_access_technology_identifier, "PLMN access technology")

#define QMICLI_FLAGS_LIST_ITEM(TYPE,TYPE_UNDERSCORE,DESCR)        \
    gboolean qmicli_read_## TYPE_UNDERSCORE ##_from_string (const gchar *str, TYPE *out);
QMICLI_FLAGS_LIST
#undef QMICLI_FLAGS_LIST_ITEM

/* Common helpers to read 64bit flags from strings */
#define QMICLI_FLAGS64_LIST                                                                            \
    QMICLI_FLAGS64_LIST_ITEM (QmiDsdApnTypePreference, dsd_apn_type_preference, "apn type preference") \
    QMICLI_FLAGS64_LIST_ITEM (QmiWdsApnTypeMask,       wds_apn_type_mask,       "apn type mask")

#define QMICLI_FLAGS64_LIST_ITEM(TYPE,TYPE_UNDERSCORE,DESCR)        \
    gboolean qmicli_read_## TYPE_UNDERSCORE ##_from_string (const gchar *str, TYPE *out);
QMICLI_FLAGS64_LIST
#undef QMICLI_FLAGS64_LIST_ITEM

gchar *qmicli_get_raw_data_printable (const GArray *data,
                                      gsize max_line_length,
                                      const gchar *new_line_prefix);

gchar *qmicli_get_firmware_image_unique_id_printable (const GArray *unique_id);

gboolean qmicli_read_dms_uim_pin_id_from_string              (const gchar *str,
                                                              QmiDmsUimPinId *out);
gboolean qmicli_read_uim_pin_id_from_string                  (const gchar *str,
                                                              QmiUimPinId *out);
gboolean qmicli_read_ssp_rat_options_from_string             (const gchar              *str,
                                                              QmiNasRatModePreference  *out_mode_preference,
                                                              GArray                  **out_acquisition_order);
gboolean qmicli_read_ssp_net_options_from_string             (const gchar *str,
                                                              QmiNasNetworkSelectionPreference *out_network_preference,
                                                              guint16 *out_network_mcc,
                                                              guint16 *out_network_mnc);
gboolean qmicli_read_parse_3gpp_mcc_mnc                      (const gchar *str,
                                                              guint16     *out_mcc,
                                                              guint16     *out_mnc,
                                                              gboolean    *out_pcs_digit);
gboolean qmicli_read_enable_disable_from_string              (const gchar *str,
                                                              gboolean *out);
gboolean qmicli_read_firmware_id_from_string                 (const gchar *str,
                                                              QmiDmsFirmwareImageType *out_type,
                                                              guint *out_index);
gboolean qmicli_read_binary_array_from_string                (const gchar *str,
                                                              GArray **out);
gboolean qmicli_read_authentication_from_string              (const gchar *str,
                                                              QmiWdsAuthentication *out);
gboolean qmicli_read_pdp_type_from_string                    (const gchar *str,
                                                              QmiWdsPdpType *out);
gboolean qmicli_read_non_empty_string           (const gchar *str,
                                                 const gchar *description,
                                                 gchar **out);
gboolean qmicli_read_uint_from_string           (const gchar *str,
                                                 guint *out);
gboolean qmicli_read_raw_data_from_string       (const gchar  *str,
                                                 GArray **out);
gboolean qmicli_read_yes_no_from_string         (const gchar *str,
                                                 gboolean *out);

gchar *qmicli_get_supported_messages_list (const guint8 *data,
                                           gsize len);

const char *qmicli_earfcn_to_eutra_band_string (guint16 earfcn);

gboolean qmicli_validate_device_open_flags (QmiDeviceOpenFlags mask);

typedef gboolean (*QmiParseKeyValueForeachFn) (const gchar *key,
                                               const gchar *value,
                                               GError **error,
                                               gpointer user_data);

gboolean qmicli_parse_key_value_string (const gchar *str,
                                        GError **error,
                                        QmiParseKeyValueForeachFn callback,
                                        gpointer user_data);

#endif /* __QMICLI_H__ */
