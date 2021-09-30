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

gboolean mbimcli_read_uint_from_string (const gchar *str,
                                        guint *out);

gboolean mbimcli_read_uint8_from_bcd_string (const gchar *str,
                                             guint8      *out);

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


/* Common helpers to read enums from strings */
#define MBIMCLI_ENUM_LIST                                                                                  \
    MBIMCLI_ENUM_LIST_ITEM (MbimPinType,               pin_type,                "pin type")                \
    MBIMCLI_ENUM_LIST_ITEM (MbimContextType,           context_type,            "context type")            \
    MBIMCLI_ENUM_LIST_ITEM (MbimContextIpType,         context_ip_type,         "context ip type")         \
    MBIMCLI_ENUM_LIST_ITEM (MbimContextState,          context_state,           "context state")           \
    MBIMCLI_ENUM_LIST_ITEM (MbimContextRoamingControl, context_roaming_control, "context roaming control") \
    MBIMCLI_ENUM_LIST_ITEM (MbimContextMediaType,      context_media_type,      "context media type")      \
    MBIMCLI_ENUM_LIST_ITEM (MbimContextSource,         context_source,          "context source")          \
    MBIMCLI_ENUM_LIST_ITEM (MbimContextOperation,      context_operation,       "context operation")       \
    MBIMCLI_ENUM_LIST_ITEM (MbimAuthProtocol,          auth_protocol,           "auth protocol")           \
    MBIMCLI_ENUM_LIST_ITEM (MbimCompression,           compression,             "compression")             \
    MBIMCLI_ENUM_LIST_ITEM (MbimSarControlMode,        sar_control_mode,        "sar control mode")        \
    MBIMCLI_ENUM_LIST_ITEM (MbimSarBackoffState,       sar_backoff_state,       "sar backoff state")

#define MBIMCLI_ENUM_LIST_ITEM(TYPE,TYPE_UNDERSCORE,DESCR)        \
    gboolean mbimcli_read_## TYPE_UNDERSCORE ##_from_string (const gchar *str, TYPE *out);
MBIMCLI_ENUM_LIST
#undef MBIMCLI_ENUM_LIST_ITEM

#endif /* __MBIMCLI_H__ */
