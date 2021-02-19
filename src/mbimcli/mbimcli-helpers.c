/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * mbimcli -- Command line interface to control MBIM devices
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

MbimPinType
mbimcli_read_pintype_from_string (const gchar *str)
{
    const gchar *feature;
    gint i;

    if (str == NULL)
        return MBIM_PIN_TYPE_UNKNOWN;

    /* Compare string to nicknames from mbim_pin_type_values */
    i = MBIM_PIN_TYPE_CUSTOM;
    while (NULL != (feature = mbim_pin_type_get_string (i))) {
        if (g_str_equal (feature, str))
            return i;
        i++;
    }

    return MBIM_PIN_TYPE_UNKNOWN;
}
