/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * mbimcli -- Command line interface to control MBIM devices
 *
 * Copyright (C) 2014 Aleksander Morgado <aleksander@aleksander.es>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "mbimcli-helpers.h"

gboolean
mbimcli_read_uint_from_string (const gchar *str,
                               guint *out)
{
    gulong num;

    if (!str || !str[0])
        return FALSE;

    for (num = 0; str[num]; num++) {
        if (!g_ascii_isdigit (str[num]))
            return FALSE;
    }

    errno = 0;
    num = strtoul (str, NULL, 10);
    if (!errno && num <= G_MAXUINT) {
        *out = (guint)num;
        return TRUE;
    }
    return FALSE;
}

gboolean
mbimcli_read_uint_from_bcd_string (const gchar *str,
                                   guint       *out)
{
    gulong num;

    if (!str || !str[0])
        return FALSE;

    /* in bcd, only numeric values (0-9) */
    for (num = 0; str[num]; num++) {
        if (!g_ascii_isdigit (str[num]))
            return FALSE;
    }

    /* for the numeric values of str, we can just read the string as hex
     * (base 16) and it will be valid bcd */
    errno = 0;
    num = strtoul (str, NULL, 16);
    if (!errno && num <= G_MAXUINT) {
        *out = (guint)num;
        return TRUE;
    }
    return FALSE;
}

gboolean
mbimcli_read_uint8_from_bcd_string (const gchar *str,
                                    guint8      *out)
{
    guint num;

    if (!mbimcli_read_uint_from_bcd_string (str, &num) || (num > G_MAXUINT8))
        return FALSE;

    *out = (guint8)num;
    return TRUE;
}

gboolean
mbimcli_read_boolean_from_string (const gchar *value,
                                  gboolean    *out)
{
    if (!g_ascii_strcasecmp (value, "true") || g_str_equal (value, "1") || !g_ascii_strcasecmp (value, "yes")) {
        *out = TRUE;
        return TRUE;
    }

    if (!g_ascii_strcasecmp (value, "false") || g_str_equal (value, "0") || !g_ascii_strcasecmp (value, "no")) {
        *out = FALSE;
        return TRUE;
    }

    return FALSE;
}

/* Based on ModemManager's mm_utils_hexstr2bin() */

static gint
hex2num (gchar c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}

static gint
hex2byte (const gchar *hex)
{
    gint a, b;

    a = hex2num (*hex++);
    if (a < 0)
        return -1;
    b = hex2num (*hex++);
    if (b < 0)
        return -1;
    return (a << 4) | b;
}

guint8 *
mbimcli_read_buffer_from_string (const gchar  *hex,
                                 gssize        len,
                                 gsize        *out_len,
                                 GError      **error)
{
    const gchar *ipos = hex;
    g_autofree guint8 *buf = NULL;
    gssize i;
    gint a;
    guint8 *opos;

    if (len < 0)
        len = strlen (hex);

    if (len == 0) {
        g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_FAILED,
                     "Hex conversion failed: empty string");
        return NULL;
    }

    /* Length must be a multiple of 2 */
    if ((len % 2) != 0) {
        g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_FAILED,
                     "Hex conversion failed: invalid input length");
        return NULL;
    }

    opos = buf = g_malloc0 (len / 2);
    for (i = 0; i < len; i += 2) {
        a = hex2byte (ipos);
        if (a < 0) {
            g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_FAILED,
                         "Hex byte conversion from '%c%c' failed",
                         ipos[0], ipos[1]);
            return NULL;
        }
        *opos++ = (guint8)a;
        ipos += 2;
    }
    *out_len = len / 2;
    return g_steal_pointer (&buf);
}

gboolean
mbimcli_print_ip_config (MbimDevice   *device,
                         MbimMessage  *response,
                         GError      **error)
{
    MbimIPConfigurationAvailableFlag  ipv4configurationavailable;
    g_autofree gchar                 *ipv4configurationavailable_str = NULL;
    MbimIPConfigurationAvailableFlag  ipv6configurationavailable;
    g_autofree gchar                 *ipv6configurationavailable_str = NULL;
    guint32                           ipv4addresscount;
    g_autoptr(MbimIPv4ElementArray)   ipv4address = NULL;
    guint32                           ipv6addresscount;
    g_autoptr(MbimIPv6ElementArray)   ipv6address = NULL;
    const MbimIPv4                   *ipv4gateway;
    const MbimIPv6                   *ipv6gateway;
    guint32                           ipv4dnsservercount;
    g_autofree MbimIPv4              *ipv4dnsserver = NULL;
    guint32                           ipv6dnsservercount;
    g_autofree MbimIPv6              *ipv6dnsserver = NULL;
    guint32                           ipv4mtu;
    guint32                           ipv6mtu;

    if (!mbim_message_ip_configuration_response_parse (
            response,
            NULL, /* sessionid */
            &ipv4configurationavailable,
            &ipv6configurationavailable,
            &ipv4addresscount,
            &ipv4address,
            &ipv6addresscount,
            &ipv6address,
            &ipv4gateway,
            &ipv6gateway,
            &ipv4dnsservercount,
            &ipv4dnsserver,
            &ipv6dnsservercount,
            &ipv6dnsserver,
            &ipv4mtu,
            &ipv6mtu,
            error))
        return FALSE;

    /* IPv4 info */

    ipv4configurationavailable_str = mbim_ip_configuration_available_flag_build_string_from_mask (ipv4configurationavailable);
    g_print ("\n[%s] IPv4 configuration available: '%s'\n", mbim_device_get_path_display (device), ipv4configurationavailable_str);

    if (ipv4configurationavailable & MBIM_IP_CONFIGURATION_AVAILABLE_FLAG_ADDRESS) {
        guint i;

        for (i = 0; i < ipv4addresscount; i++) {
            g_autoptr(GInetAddress)  addr = NULL;
            g_autofree gchar        *addr_str = NULL;

            addr = g_inet_address_new_from_bytes ((guint8 *)&ipv4address[i]->ipv4_address, G_SOCKET_FAMILY_IPV4);
            addr_str = g_inet_address_to_string (addr);
            g_print ("     IP [%u]: '%s/%u'\n", i, addr_str, ipv4address[i]->on_link_prefix_length);
        }
    }

    if (ipv4configurationavailable & MBIM_IP_CONFIGURATION_AVAILABLE_FLAG_GATEWAY) {
        g_autoptr(GInetAddress)  addr = NULL;
        g_autofree gchar        *addr_str = NULL;

        addr = g_inet_address_new_from_bytes ((guint8 *)ipv4gateway, G_SOCKET_FAMILY_IPV4);
        addr_str = g_inet_address_to_string (addr);
        g_print ("    Gateway: '%s'\n", addr_str);
    }

    if (ipv4configurationavailable & MBIM_IP_CONFIGURATION_AVAILABLE_FLAG_DNS) {
        guint i;

        for (i = 0; i < ipv4dnsservercount; i++) {
            g_autoptr(GInetAddress) addr = NULL;

            addr = g_inet_address_new_from_bytes ((guint8 *)&ipv4dnsserver[i], G_SOCKET_FAMILY_IPV4);
            if (!g_inet_address_get_is_any (addr)) {
                g_autofree gchar *addr_str = NULL;

                addr_str = g_inet_address_to_string (addr);
                g_print ("    DNS [%u]: '%s'\n", i, addr_str);
            }
        }
    }

    if (ipv4configurationavailable & MBIM_IP_CONFIGURATION_AVAILABLE_FLAG_MTU)
        g_print ("        MTU: '%u'\n", ipv4mtu);

    /* IPv6 info */
    ipv6configurationavailable_str = mbim_ip_configuration_available_flag_build_string_from_mask (ipv6configurationavailable);
    g_print ("\n[%s] IPv6 configuration available: '%s'\n", mbim_device_get_path_display (device), ipv6configurationavailable_str);

    if (ipv6configurationavailable & MBIM_IP_CONFIGURATION_AVAILABLE_FLAG_ADDRESS) {
        guint i;

        for (i = 0; i < ipv6addresscount; i++) {
            g_autoptr(GInetAddress)  addr = NULL;
            g_autofree gchar        *addr_str = NULL;

            addr = g_inet_address_new_from_bytes ((guint8 *)&ipv6address[i]->ipv6_address, G_SOCKET_FAMILY_IPV6);
            addr_str = g_inet_address_to_string (addr);
            g_print ("     IP [%u]: '%s/%u'\n", i, addr_str, ipv6address[i]->on_link_prefix_length);
        }
    }

    if (ipv6configurationavailable & MBIM_IP_CONFIGURATION_AVAILABLE_FLAG_GATEWAY) {
        g_autoptr(GInetAddress)  addr = NULL;
        g_autofree gchar        *addr_str = NULL;

        addr = g_inet_address_new_from_bytes ((guint8 *)ipv6gateway, G_SOCKET_FAMILY_IPV6);
        addr_str = g_inet_address_to_string (addr);
        g_print ("    Gateway: '%s'\n", addr_str);
    }

    if (ipv6configurationavailable & MBIM_IP_CONFIGURATION_AVAILABLE_FLAG_DNS) {
        guint i;

        for (i = 0; i < ipv6dnsservercount; i++) {
            g_autoptr(GInetAddress) addr = NULL;

            addr = g_inet_address_new_from_bytes ((guint8 *)&ipv6dnsserver[i], G_SOCKET_FAMILY_IPV6);
            if (!g_inet_address_get_is_any (addr)) {
                g_autofree gchar *addr_str = NULL;

                addr_str = g_inet_address_to_string (addr);
                g_print ("    DNS [%u]: '%s'\n", i, addr_str);
            }
        }
    }

    if (ipv6configurationavailable & MBIM_IP_CONFIGURATION_AVAILABLE_FLAG_MTU)
        g_print ("        MTU: '%u'\n", ipv6mtu);

    return TRUE;
}

/* Expecting input as:
 *   key1=string,key2=true,key3=false...
 * Strings may also be passed enclosed between double or single quotes, like:
 *   key1="this is a string", key2='and so is this'
 */
gboolean
mbimcli_parse_key_value_string (const gchar                 *str,
                                GError                     **error,
                                MbimParseKeyValueForeachFn   callback,
                                gpointer                     user_data)
{
    GError           *inner_error = NULL;
    g_autofree gchar *dupstr = NULL;
    gchar *p, *key, *key_end, *value, *value_end, quote;

    g_return_val_if_fail (callback != NULL, FALSE);
    g_return_val_if_fail (str != NULL, FALSE);

    /* Allow empty strings, we'll just return with success */
    while (g_ascii_isspace (*str))
        str++;
    if (!str[0])
        return TRUE;

    dupstr = g_strdup (str);
    p = dupstr;

    while (TRUE) {
        gboolean keep_iteration = FALSE;

        /* Skip leading spaces */
        while (g_ascii_isspace (*p))
            p++;

        /* Key start */
        key = p;
        if (!g_ascii_isalnum (*key)) {
            inner_error = g_error_new (MBIM_CORE_ERROR,
                                       MBIM_CORE_ERROR_FAILED,
                                       "Key must start with alpha/num, starts with '%c'",
                                       *key);
            break;
        }

        /* Key end */
        while (g_ascii_isalnum (*p) || (*p == '-') || (*p == '_'))
            p++;
        key_end = p;
        if (key_end == key) {
            inner_error = g_error_new (MBIM_CORE_ERROR,
                                       MBIM_CORE_ERROR_FAILED,
                                       "Couldn't find a proper key");
            break;
        }

        /* Skip whitespaces, if any */
        while (g_ascii_isspace (*p))
            p++;

        /* Equal sign must be here */
        if (*p != '=') {
            inner_error = g_error_new (MBIM_CORE_ERROR,
                                       MBIM_CORE_ERROR_FAILED,
                                       "Couldn't find equal sign separator");
            break;
        }
        /* Skip the equal */
        p++;

        /* Skip whitespaces, if any */
        while (g_ascii_isspace (*p))
            p++;

        /* Do we have a quote-enclosed string? */
        if (*p == '\"' || *p == '\'') {
            quote = *p;
            /* Skip the quote */
            p++;
            /* Value start */
            value = p;
            /* Find the closing quote */
            p = strchr (p, quote);
            if (!p) {
                inner_error = g_error_new (MBIM_CORE_ERROR,
                                           MBIM_CORE_ERROR_FAILED,
                                           "Unmatched quotes in string value");
                break;
            }

            /* Value end */
            value_end = p;
            /* Skip the quote */
            p++;
        } else {
            /* Value start */
            value = p;

            /* Value end */
            while ((*p != ',') && (*p != '\0') && !g_ascii_isspace (*p))
                p++;
            value_end = p;
        }

        /* Note that we allow value == value_end here */

        /* Skip whitespaces, if any */
        while (g_ascii_isspace (*p))
            p++;

        /* If a comma is found, we should keep the iteration */
        if (*p == ',') {
            /* skip the comma */
            p++;
            keep_iteration = TRUE;
        }

        /* Got key and value, prepare them and run the callback */
        *value_end = '\0';
        *key_end = '\0';
        if (!callback (key, value, &inner_error, user_data)) {
            /* We were told to abort */
            break;
        }
        g_assert (!inner_error);

        if (keep_iteration)
            continue;

        /* Check if no more key/value pairs expected */
        if (*p == '\0')
            break;

        inner_error = g_error_new (MBIM_CORE_ERROR,
                                   MBIM_CORE_ERROR_FAILED,
                                   "Unexpected content (%s) after value",
                                   p);
        break;
    }

    if (inner_error) {
        g_propagate_error (error, inner_error);
        return FALSE;
    }

    return TRUE;
}

gboolean
mbimcli_parse_sar_config_state_array (const gchar  *str,
                                      GPtrArray   **out)
{
    g_autoptr(GPtrArray)  config_state_array = NULL;
    g_autoptr(GRegex)     regex = NULL;
    g_autoptr(GMatchInfo) match_info = NULL;
    g_autoptr(GError)     inner_error = NULL;

    config_state_array = g_ptr_array_new_with_free_func (g_free);

    if (!str || !str[0]) {
        *out = NULL;
        return TRUE;
    }

    regex = g_regex_new ("\\s*{\\s*(\\d+|all)\\s*,\\s*(\\d+)\\s*}(?:\\s*,)?", G_REGEX_RAW, 0, NULL);
    g_assert (regex);

    g_regex_match_full (regex, str, strlen (str), 0, 0, &match_info, &inner_error);
    while (!inner_error && g_match_info_matches (match_info)) {
        g_autofree MbimSarConfigState *config_state = NULL;
        g_autofree gchar              *antenna_index_str = NULL;
        g_autofree gchar              *backoff_index_str = NULL;

        config_state = g_new (MbimSarConfigState, 1);

        antenna_index_str = g_match_info_fetch (match_info, 1);
        backoff_index_str = g_match_info_fetch (match_info, 2);

        if (g_ascii_strcasecmp (antenna_index_str, "all") == 0)
            config_state->antenna_index = 0xFFFFFFFF;
        else if (!mbimcli_read_uint_from_string (antenna_index_str, &config_state->antenna_index)) {
            g_printerr ("error: invalid antenna index: '%s'\n", antenna_index_str);
            return FALSE;
        }
        if (!mbimcli_read_uint_from_string (backoff_index_str, &config_state->backoff_index)) {
            g_printerr ("error: invalid backoff index: '%s'\n", backoff_index_str);
            return FALSE;
        }

        g_ptr_array_add (config_state_array, g_steal_pointer (&config_state));
        g_match_info_next (match_info, &inner_error);
    }

    if (inner_error) {
        g_printerr ("error: couldn't match config state array: %s\n", inner_error->message);
        return FALSE;
    }

    if (config_state_array->len == 0) {
        g_printerr ("error: no elements found in the array\n");
        return FALSE;
    }

    *out = (config_state_array->len > 0) ? g_steal_pointer (&config_state_array) : NULL;
    return TRUE;
}

#define MBIMCLI_ENUM_LIST_ITEM(TYPE,TYPE_UNDERSCORE,DESCR)                    \
    gboolean                                                                  \
    mbimcli_read_## TYPE_UNDERSCORE ##_from_string (const gchar *str,         \
                                                    TYPE *out)                \
    {                                                                         \
        GType type;                                                           \
        GEnumClass *enum_class;                                               \
        GEnumValue *enum_value;                                               \
                                                                              \
        type = mbim_## TYPE_UNDERSCORE ##_get_type ();                         \
        enum_class = G_ENUM_CLASS (g_type_class_ref (type));                  \
        enum_value = g_enum_get_value_by_nick (enum_class, str);              \
                                                                              \
        if (enum_value)                                                       \
            *out = (TYPE)enum_value->value;                                   \
        else                                                                  \
            g_printerr ("error: invalid " DESCR " value given: '%s'\n", str); \
                                                                              \
        g_type_class_unref (enum_class);                                      \
        return !!enum_value;                                                  \
    }
MBIMCLI_ENUM_LIST
#undef MBIMCLI_ENUM_LIST_ITEM
