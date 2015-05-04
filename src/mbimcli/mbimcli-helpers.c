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
mbimcli_print_ip_config (MbimDevice *device,
                         MbimMessage *response,
                         GError **error)
{
    MbimIPConfigurationAvailableFlag ipv4configurationavailable;
    MbimIPConfigurationAvailableFlag ipv6configurationavailable;
    guint32 ipv4addresscount;
    MbimIPv4Element **ipv4address;
    guint32 ipv6addresscount;
    MbimIPv6Element **ipv6address;
    const MbimIPv4 *ipv4gateway;
    const MbimIPv6 *ipv6gateway;
    guint32 ipv4dnsservercount;
    MbimIPv4 *ipv4dnsserver;
    guint32 ipv6dnsservercount;
    MbimIPv6 *ipv6dnsserver;
    guint32 ipv4mtu;
    guint32 ipv6mtu;
    gchar *str;
    GInetAddress *addr;

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

    str = mbim_ip_configuration_available_flag_build_string_from_mask (ipv4configurationavailable);
    g_print ("\n[%s] IPv4 configuration available: '%s'\n", mbim_device_get_path_display (device), str);
    g_free (str);

    if (ipv4configurationavailable & MBIM_IP_CONFIGURATION_AVAILABLE_FLAG_ADDRESS) {
        guint i;

        for (i = 0; i < ipv4addresscount; i++) {
            addr = g_inet_address_new_from_bytes ((guint8 *)&ipv4address[i]->ipv4_address, G_SOCKET_FAMILY_IPV4);
            str = g_inet_address_to_string (addr);
            g_print ("     IP [%u]: '%s/%u'\n",
                     i,
                     str,
                     ipv4address[i]->on_link_prefix_length);
            g_free (str);
            g_object_unref (addr);
        }
    }

    if (ipv4configurationavailable & MBIM_IP_CONFIGURATION_AVAILABLE_FLAG_GATEWAY) {
        addr = g_inet_address_new_from_bytes ((guint8 *)ipv4gateway, G_SOCKET_FAMILY_IPV4);
        str = g_inet_address_to_string (addr);
        g_print ("    Gateway: '%s'\n", str);
        g_free (str);
        g_object_unref (addr);
    }

    if (ipv4configurationavailable & MBIM_IP_CONFIGURATION_AVAILABLE_FLAG_DNS) {
        guint i;

        for (i = 0; i < ipv4dnsservercount; i++) {
            addr = g_inet_address_new_from_bytes ((guint8 *)&ipv4dnsserver[i], G_SOCKET_FAMILY_IPV4);
            if (!g_inet_address_get_is_any (addr)) {
                str = g_inet_address_to_string (addr);
                g_print ("    DNS [%u]: '%s'\n", i, str);
                g_free (str);
            }
            g_object_unref (addr);
        }
    }

    if (ipv4configurationavailable & MBIM_IP_CONFIGURATION_AVAILABLE_FLAG_MTU)
        g_print ("        MTU: '%u'\n", ipv4mtu);

    /* IPv6 info */
    str = mbim_ip_configuration_available_flag_build_string_from_mask (ipv6configurationavailable);
    g_print ("\n[%s] IPv6 configuration available: '%s'\n", mbim_device_get_path_display (device), str);
    g_free (str);

    if (ipv6configurationavailable & MBIM_IP_CONFIGURATION_AVAILABLE_FLAG_ADDRESS) {
        guint i;

        for (i = 0; i < ipv6addresscount; i++) {
            addr = g_inet_address_new_from_bytes ((guint8 *)&ipv6address[i]->ipv6_address, G_SOCKET_FAMILY_IPV6);
            str = g_inet_address_to_string (addr);
            g_print ("     IP [%u]: '%s/%u'\n",
                     i,
                     str,
                     ipv6address[i]->on_link_prefix_length);
            g_free (str);
            g_object_unref (addr);
        }
    }

    if (ipv6configurationavailable & MBIM_IP_CONFIGURATION_AVAILABLE_FLAG_GATEWAY) {
        addr = g_inet_address_new_from_bytes ((guint8 *)ipv6gateway, G_SOCKET_FAMILY_IPV6);
        str = g_inet_address_to_string (addr);
        g_print ("    Gateway: '%s'\n", str);
        g_free (str);
        g_object_unref (addr);
    }

    if (ipv6configurationavailable & MBIM_IP_CONFIGURATION_AVAILABLE_FLAG_DNS) {
        guint i;

        for (i = 0; i < ipv6dnsservercount; i++) {
            addr = g_inet_address_new_from_bytes ((guint8 *)&ipv6dnsserver[i], G_SOCKET_FAMILY_IPV6);
            if (!g_inet_address_get_is_any (addr)) {
                str = g_inet_address_to_string (addr);
                g_print ("    DNS [%u]: '%s'\n", i, str);
                g_free (str);
            }
            g_object_unref (addr);
        }
    }

    if (ipv6configurationavailable & MBIM_IP_CONFIGURATION_AVAILABLE_FLAG_MTU)
        g_print ("        MTU: '%u'\n", ipv6mtu);

    mbim_ipv4_element_array_free (ipv4address);
    mbim_ipv6_element_array_free (ipv6address);
    g_free (ipv4dnsserver);
    g_free (ipv6dnsserver);
    return TRUE;
}

