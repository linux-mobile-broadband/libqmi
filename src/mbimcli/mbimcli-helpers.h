/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * mbimcli -- Command line interface to control QMI devices
 *
 * Copyright (C) 2014 Aleksander Morgado <aleksander@aleksander.es>
 */

#include <glib.h>

#include <libmbim-glib.h>

#ifndef __MBIMCLI_HELPERS_H__
#define __MBIMCLI_HELPERS_H__

gboolean mbimcli_read_uint_from_string      (const gchar *str,
                                             guint       *out);
gboolean mbimcli_read_uint_from_bcd_string  (const gchar *str,
                                             guint       *out);
gboolean mbimcli_read_uint8_from_bcd_string (const gchar *str,
                                             guint8      *out);

gboolean mbimcli_read_boolean_from_string (const gchar *value,
                                           gboolean    *out);

guint8  *mbimcli_read_buffer_from_string (const gchar  *hex,
                                          gssize        len,
                                          gsize        *out_len,
                                          GError      **error);

gboolean mbimcli_print_ip_config (MbimDevice *device,
                                  MbimMessage *response,
                                  GError **error);

typedef gboolean (*MbimParseKeyValueForeachFn) (const gchar *key,
                                                const gchar *value,
                                                GError **error,
                                                gpointer user_data);

gboolean mbimcli_parse_key_value_string (const gchar *str,
                                         GError **error,
                                         MbimParseKeyValueForeachFn callback,
                                         gpointer user_data);

gboolean mbimcli_parse_sar_config_state_array (const gchar  *str,
                                               GPtrArray   **out);

/* Common helpers to read enums from strings */

#define MBIMCLI_ENUM_LIST                                                                                             \
    MBIMCLI_ENUM_LIST_ITEM (MbimPinType,                  pin_type,                    "pin type")                    \
    MBIMCLI_ENUM_LIST_ITEM (MbimContextType,              context_type,                "context type")                \
    MBIMCLI_ENUM_LIST_ITEM (MbimContextIpType,            context_ip_type,             "context ip type")             \
    MBIMCLI_ENUM_LIST_ITEM (MbimContextState,             context_state,               "context state")               \
    MBIMCLI_ENUM_LIST_ITEM (MbimContextRoamingControl,    context_roaming_control,     "context roaming control")     \
    MBIMCLI_ENUM_LIST_ITEM (MbimContextMediaType,         context_media_type,          "context media type")          \
    MBIMCLI_ENUM_LIST_ITEM (MbimContextSource,            context_source,              "context source")              \
    MBIMCLI_ENUM_LIST_ITEM (MbimContextOperation,         context_operation,           "context operation")           \
    MBIMCLI_ENUM_LIST_ITEM (MbimAuthProtocol,             auth_protocol,               "auth protocol")               \
    MBIMCLI_ENUM_LIST_ITEM (MbimCompression,              compression,                 "compression")                 \
    MBIMCLI_ENUM_LIST_ITEM (MbimSarControlMode,           sar_control_mode,            "sar control mode")            \
    MBIMCLI_ENUM_LIST_ITEM (MbimSarBackoffState,          sar_backoff_state,           "sar backoff state")           \
    MBIMCLI_ENUM_LIST_ITEM (MbimMicoMode,                 mico_mode,                   "mico mode")                   \
    MBIMCLI_ENUM_LIST_ITEM (MbimDrxCycle,                 drx_cycle,                   "drx cycle")                   \
    MBIMCLI_ENUM_LIST_ITEM (MbimLadnInfo,                 ladn_info,                   "ladn info")                   \
    MBIMCLI_ENUM_LIST_ITEM (MbimDefaultPduActivationHint, default_pdu_activation_hint, "default pdu activation hint") \
    MBIMCLI_ENUM_LIST_ITEM (MbimAccessMediaType,          access_media_type,           "access media type")           \
    MBIMCLI_ENUM_LIST_ITEM (MbimNetworkIdleHintState,     network_idle_hint_state,     "network idle hint state")     \
    MBIMCLI_ENUM_LIST_ITEM (MbimEmergencyModeState,       emergency_mode_state,        "emergency mode state")        \
    MBIMCLI_ENUM_LIST_ITEM (MbimUiccSecureMessaging,      uicc_secure_messaging,       "uicc secure messaging")       \
    MBIMCLI_ENUM_LIST_ITEM (MbimUiccClassByteType,        uicc_class_byte_type,        "uicc class byte type")        \
    MBIMCLI_ENUM_LIST_ITEM (MbimUiccPassThroughAction,    uicc_pass_through_action,    "uicc pass through action")    \
    MBIMCLI_ENUM_LIST_ITEM (MbimIntelBootMode,            intel_boot_mode,             "intel boot mode")             \
    MBIMCLI_ENUM_LIST_ITEM (MbimTraceCommand,             trace_command,               "trace command")               \
    MBIMCLI_ENUM_LIST_ITEM (MbimSmsFlag,                  sms_flag,                    "sms flag")                    \
    MBIMCLI_ENUM_LIST_ITEM (MbimQuectelCommandType,       quectel_command_type,        "quectel command type")

#define MBIMCLI_ENUM_LIST_ITEM(TYPE,TYPE_UNDERSCORE,DESCR)        \
    gboolean mbimcli_read_## TYPE_UNDERSCORE ##_from_string (const gchar *str, TYPE *out);
MBIMCLI_ENUM_LIST
#undef MBIMCLI_ENUM_LIST_ITEM

#endif /* __MBIMCLI_H__ */
