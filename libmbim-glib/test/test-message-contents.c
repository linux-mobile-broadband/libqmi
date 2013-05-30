/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details:
 *
 * Copyright (C) 2012 Aleksander Morgado <aleksander@gnu.org>
 */

#include <config.h>
#include <string.h>

#include "mbim-basic-connect.h"
#include "mbim-message.h"
#include "mbim-cid.h"

static void
test_message_contents_basic_connect_subscriber_ready_status (void)
{
    MbimSubscriberReadyState ready_state;
    gchar *subscriber_id;
    gchar *sim_iccid;
    MbimReadyInfoFlag ready_info;
    guint32 telephone_numbers_count;
    gchar **telephone_numbers;
    MbimMessage *response;
    GError *error = NULL;
    const guint8 buffer [] =  {
        /* header */
        0x03, 0x00, 0x00, 0x80, /* type */
        0xB4, 0x00, 0x00, 0x00, /* length */
        0x02, 0x00, 0x00, 0x00, /* transaction id */
        /* fragment header */
        0x01, 0x00, 0x00, 0x00, /* total */
        0x00, 0x00, 0x00, 0x00, /* current */
        /* command_message */
        0xA2, 0x89, 0xCC, 0x33, /* service id */
        0xBC, 0xBB, 0x8B, 0x4F,
        0xB6, 0xB0, 0x13, 0x3E,
        0xC2, 0xAA, 0xE6, 0xDF,
        0x02, 0x00, 0x00, 0x00, /* command id */
        0x00, 0x00, 0x00, 0x00, /* status code */
        0x84, 0x00, 0x00, 0x00, /* buffer_length */
        /* information buffer */
        0x01, 0x00, 0x00, 0x00, /* 0x00 ready state */
        0x5C, 0x00, 0x00, 0x00, /* 0x04 subscriber id (offset) */
        0x1E, 0x00, 0x00, 0x00, /* 0x08 subscriber id (size) */
        0x7C, 0x00, 0x00, 0x00, /* 0x0C sim iccid (offset) */
        0x28, 0x00, 0x00, 0x00, /* 0x10 sim iccid (size) */
        0x00, 0x00, 0x00, 0x00, /* 0x14 ready info */
        0x02, 0x00, 0x00, 0x00, /* 0x18 telephone numbers count */
        0x2C, 0x00, 0x00, 0x00, /* 0x1C telephone number #1 (offset) */
        0x16, 0x00, 0x00, 0x00, /* 0x20 telephone number #1 (size) */
        0x44, 0x00, 0x00, 0x00, /* 0x24 telephone number #2 (offset) */
        0x16, 0x00, 0x00, 0x00, /* 0x28 telephone number #2 (size) */
        /* data buffer */
        0x31, 0x00, 0x31, 0x00, /* 0x2C telephone number #1 (data) */
        0x31, 0x00, 0x31, 0x00,
        0x31, 0x00, 0x31, 0x00,
        0x31, 0x00, 0x31, 0x00,
        0x31, 0x00, 0x31, 0x00,
        0x31, 0x00, 0x00, 0x00, /* last 2 bytes are padding */
        0x30, 0x00, 0x30, 0x00, /* 0x44 telephone number #2 (data) */
        0x30, 0x00, 0x30, 0x00,
        0x30, 0x00, 0x30, 0x00,
        0x30, 0x00, 0x30, 0x00,
        0x30, 0x00, 0x30, 0x00,
        0x30, 0x00, 0x00, 0x00, /* last 2 bytes are padding */
        0x33, 0x00, 0x31, 0x00, /* 0x5C subscriber id (data) */
        0x30, 0x00, 0x34, 0x00,
        0x31, 0x00, 0x30, 0x00,
        0x30, 0x00, 0x30, 0x00,
        0x30, 0x00, 0x31, 0x00,
        0x31, 0x00, 0x30, 0x00,
        0x37, 0x00, 0x36, 0x00,
        0x31, 0x00, 0x00, 0x00, /* last 2 bytes are padding */
        0x38, 0x00, 0x39, 0x00, /* 0x7C sim iccid (data) */
        0x30, 0x00, 0x31, 0x00,
        0x30, 0x00, 0x31, 0x00,
        0x30, 0x00, 0x34, 0x00,
        0x30, 0x00, 0x35, 0x00,
        0x34, 0x00, 0x36, 0x00,
        0x30, 0x00, 0x31, 0x00,
        0x31, 0x00, 0x30, 0x00,
        0x30, 0x00, 0x36, 0x00,
        0x31, 0x00, 0x32, 0x00 };

    response = mbim_message_new (buffer, sizeof (buffer));

    g_assert (mbim_message_subscriber_ready_status_response_parse (
                  response,
                  &ready_state,
                  &subscriber_id,
                  &sim_iccid,
                  &ready_info,
                  &telephone_numbers_count,
                  &telephone_numbers,
                  &error));

    g_assert_no_error (error);

    g_assert_cmpuint (ready_state, ==, MBIM_SUBSCRIBER_READY_STATE_INITIALIZED);
    g_assert_cmpstr (subscriber_id, ==, "310410000110761");
    g_assert_cmpstr (sim_iccid, ==, "89010104054601100612");
    g_assert_cmpuint (ready_info, ==, 0);
    g_assert_cmpuint (telephone_numbers_count, ==, 2);
    g_assert_cmpstr (telephone_numbers[0], ==, "11111111111");
    g_assert_cmpstr (telephone_numbers[1], ==, "00000000000");
    g_assert (telephone_numbers[2] == NULL);

    g_free (subscriber_id);
    g_free (sim_iccid);
    g_strfreev (telephone_numbers);

    mbim_message_unref (response);
}

static void
test_message_contents_basic_connect_device_caps (void)
{
    MbimMessage *response;
    MbimDeviceType device_type;
    MbimCellularClass cellular_class;
    MbimVoiceClass voice_class;
    MbimSimClass sim_class;
    MbimDataClass data_class;
    MbimSmsCaps sms_caps;
    MbimCtrlCaps ctrl_caps;
    guint32 max_sessions;
    gchar *custom_data_class;
    gchar *device_id;
    gchar *firmware_info;
    gchar *hardware_info;
    GError *error = NULL;
    const guint8 buffer [] =  { 0x03, 0x00, 0x00, 0x80,
                                0xD0, 0x00, 0x00, 0x00,
                                0x02, 0x00, 0x00, 0x00,
                                0x01, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00,
                                0xA2, 0x89, 0xCC, 0x33,
                                0xBC, 0xBB, 0x8B, 0x4F,
                                0xB6, 0xB0, 0x13, 0x3E,
                                0xC2, 0xAA, 0xE6, 0xDF,
                                0x01, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00,
                                0xA0, 0x00, 0x00, 0x00,
                                0x02, 0x00, 0x00, 0x00,
                                0x01, 0x00, 0x00, 0x00,
                                0x01, 0x00, 0x00, 0x00,
                                0x02, 0x00, 0x00, 0x00,
                                0x1F, 0x00, 0x00, 0x80,
                                0x03, 0x00, 0x00, 0x00,
                                0x01, 0x00, 0x00, 0x00,
                                0x01, 0x00, 0x00, 0x00,
                                0x40, 0x00, 0x00, 0x00,
                                0x0A, 0x00, 0x00, 0x00,
                                0x4C, 0x00, 0x00, 0x00,
                                0x1E, 0x00, 0x00, 0x00,
                                0x6C, 0x00, 0x00, 0x00,
                                0x1E, 0x00, 0x00, 0x00,
                                0x8C, 0x00, 0x00, 0x00,
                                0x12, 0x00, 0x00, 0x00,
                                0x48, 0x00, 0x53, 0x00,
                                0x50, 0x00, 0x41, 0x00,
                                0x2B, 0x00, 0x00, 0x00,
                                0x33, 0x00, 0x35, 0x00,
                                0x33, 0x00, 0x36, 0x00,
                                0x31, 0x00, 0x33, 0x00,
                                0x30, 0x00, 0x34, 0x00,
                                0x38, 0x00, 0x38, 0x00,
                                0x30, 0x00, 0x34, 0x00,
                                0x36, 0x00, 0x32, 0x00,
                                0x32, 0x00, 0x00, 0x00,
                                0x31, 0x00, 0x31, 0x00,
                                0x2E, 0x00, 0x38, 0x00,
                                0x31, 0x00, 0x30, 0x00,
                                0x2E, 0x00, 0x30, 0x00,
                                0x39, 0x00, 0x2E, 0x00,
                                0x30, 0x00, 0x30, 0x00,
                                0x2E, 0x00, 0x30, 0x00,
                                0x30, 0x00, 0x00, 0x00,
                                0x43, 0x00, 0x50, 0x00,
                                0x31, 0x00, 0x45, 0x00,
                                0x33, 0x00, 0x36, 0x00,
                                0x37, 0x00, 0x55, 0x00,
                                0x4D, 0x00, 0x00, 0x00 };

    response = mbim_message_new (buffer, sizeof (buffer));

    g_assert (mbim_message_device_caps_response_parse (
                  response,
                  &device_type,
                  &cellular_class,
                  &voice_class,
                  &sim_class,
                  &data_class,
                  &sms_caps,
                  &ctrl_caps,
                  &max_sessions,
                  &custom_data_class,
                  &device_id,
                  &firmware_info,
                  &hardware_info,
                  &error));

    g_assert_no_error (error);

    g_assert_cmpuint (device_type, ==, MBIM_DEVICE_TYPE_REMOVABLE);
    g_assert_cmpuint (cellular_class, ==, MBIM_CELLULAR_CLASS_GSM);
    g_assert_cmpuint (sim_class, ==, MBIM_SIM_CLASS_REMOVABLE);
    g_assert_cmpuint (data_class, ==, (MBIM_DATA_CLASS_GPRS |
                                       MBIM_DATA_CLASS_EDGE |
                                       MBIM_DATA_CLASS_UMTS |
                                       MBIM_DATA_CLASS_HSDPA |
                                       MBIM_DATA_CLASS_HSUPA |
                                       MBIM_DATA_CLASS_CUSTOM));
    g_assert_cmpuint (sms_caps, ==, (MBIM_SMS_CAPS_PDU_RECEIVE | MBIM_SMS_CAPS_PDU_SEND));
    g_assert_cmpuint (ctrl_caps, ==, MBIM_CTRL_CAPS_REG_MANUAL);
    g_assert_cmpuint (max_sessions, ==, 1);
    g_assert_cmpstr (custom_data_class, ==, "HSPA+");
    g_assert_cmpstr (device_id, ==, "353613048804622");
    g_assert_cmpstr (firmware_info, ==, "11.810.09.00.00");
    g_assert_cmpstr (hardware_info, ==, "CP1E367UM");

    g_free (custom_data_class);
    g_free (device_id);
    g_free (firmware_info);
    g_free (hardware_info);

    mbim_message_unref (response);
}

static void
test_message_contents_basic_connect_ip_configuration (void)
{
    MbimMessage *response;
    guint32 session_id;
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
    GError *error = NULL;
    const guint8 buffer [] =  {
        /* header */
        0x03, 0x00, 0x00, 0x80, /* type */
        0x80, 0x00, 0x00, 0x00, /* length */
        0x1A, 0x00, 0x00, 0x00, /* transaction id */
        /* fragment header */
        0x01, 0x00, 0x00, 0x00, /* total */
        0x00, 0x00, 0x00, 0x00, /* current */
        /* command_done_message */
        0xA2, 0x89, 0xCC, 0x33, /* service id */
        0xBC, 0xBB, 0x8B, 0x4F,
        0xB6, 0xB0, 0x13, 0x3E,
        0xC2, 0xAA, 0xE6, 0xDF,
        0x0F, 0x00, 0x00, 0x00, /* command id */
        0x00, 0x00, 0x00, 0x00, /* status code */
        0x50, 0x00, 0x00, 0x00, /* buffer length */
        /* information buffer */
        0x00, 0x00, 0x00, 0x00, /* session id */
        0x0F, 0x00, 0x00, 0x00, /* IPv4ConfigurationAvailable */
        0x00, 0x00, 0x00, 0x00, /* IPv6ConfigurationAvailable */
        0x01, 0x00, 0x00, 0x00, /* IPv4 element count */
        0x3C, 0x00, 0x00, 0x00, /* IPv4 element offset */
        0x00, 0x00, 0x00, 0x00, /* IPv6 element count */
        0x00, 0x00, 0x00, 0x00, /* IPv6 element offset */
        0x44, 0x00, 0x00, 0x00, /* IPv4 gateway offset */
        0x00, 0x00, 0x00, 0x00, /* IPv6 gateway offset */
        0x02, 0x00, 0x00, 0x00, /* IPv4 DNS count */
        0x48, 0x00, 0x00, 0x00, /* IPv4 DNS offset */
        0x00, 0x00, 0x00, 0x00, /* IPv6 DNS count */
        0x00, 0x00, 0x00, 0x00, /* IPv6 DNS offset */
        0xDC, 0x05, 0x00, 0x00, /* IPv4 MTU */
        0x00, 0x00, 0x00, 0x00, /* IPv6 MTU */
        /* data buffer */
        0x1C, 0x00, 0x00, 0x00, /* IPv4 element (netmask) */
        0xD4, 0x49, 0x22, 0xF8, /* IPv4 element (address) */
        0xD4, 0x49, 0x22, 0xF1, /* IPv4 gateway */
        0xD4, 0xA6, 0xD2, 0x50, /* IPv4 DNS1 */
        0xD4, 0x49, 0x20, 0x43  /* IPv4 DNS2 */
    };

    response = mbim_message_new (buffer, sizeof (buffer));

    g_assert (mbim_message_ip_configuration_response_parse (
                  response,
                  &session_id,
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
                  &error));

    /*
     *   IPv4 configuration available: 'address, gateway, dns, mtu'
     *     IP addresses (1)
     *       IP [0]: '212.166.228.25/28'
     *     Gateway: '212.166.228.26'
     *     DNS addresses (2)
     *       DNS [0]: '212.166.210.80'
     *       DNS [1]: '212.73.32.67'
     *     MTU: '1500'
     */

    g_assert_cmpuint (session_id, ==, 0);
    g_assert_cmpuint (ipv4configurationavailable, ==, (MBIM_IP_CONFIGURATION_AVAILABLE_FLAG_ADDRESS |
                                                       MBIM_IP_CONFIGURATION_AVAILABLE_FLAG_GATEWAY |
                                                       MBIM_IP_CONFIGURATION_AVAILABLE_FLAG_DNS |
                                                       MBIM_IP_CONFIGURATION_AVAILABLE_FLAG_MTU));
    g_assert_cmpuint (ipv6configurationavailable, ==, MBIM_IP_CONFIGURATION_AVAILABLE_FLAG_NONE);

    {
        MbimIPv4 addr = { .addr = { 0xD4, 0x49, 0x22, 0xF8 } };

        g_assert_cmpuint (ipv4addresscount, ==, 1);
        g_assert_cmpuint (ipv4address[0]->on_link_prefix_length, ==, 28);
        g_assert (memcmp (&addr, &(ipv4address[0]->ipv4_address), 4) == 0);
    }

    {
        MbimIPv4 gateway_addr = { .addr = { 0xD4, 0x49, 0x22, 0xF1 } };

        g_assert (memcmp (&gateway_addr, ipv4gateway, 4) == 0);
    }

    {
        MbimIPv4 dns_addr_1 = { .addr = { 0xD4, 0xA6, 0xD2, 0x50 } };
        MbimIPv4 dns_addr_2 = { .addr = { 0xD4, 0x49, 0x20, 0x43 } };

        g_assert_cmpuint (ipv4dnsservercount, ==, 2);
        g_assert (memcmp (&dns_addr_1, &ipv4dnsserver[0], 4) == 0);
        g_assert (memcmp (&dns_addr_2, &ipv4dnsserver[1], 4) == 0);
    }

    g_assert_cmpuint (ipv4mtu, ==, 1500);

    g_assert_cmpuint (ipv6addresscount, ==, 0);
    g_assert (ipv6address == NULL);
    g_assert (ipv6gateway == NULL);
    g_assert_cmpuint (ipv6dnsservercount, ==, 0);
    g_assert (ipv6dnsserver == NULL);

    mbim_ipv4_element_array_free (ipv4address);
    mbim_ipv6_element_array_free (ipv6address);
    g_free (ipv4dnsserver);
    g_free (ipv6dnsserver);

    mbim_message_unref (response);
}

int main (int argc, char **argv)
{
    g_test_init (&argc, &argv, NULL);

    g_test_add_func ("/libmbim-glib/message-contents/basic-connect/subscriber-ready-status", test_message_contents_basic_connect_subscriber_ready_status);
    g_test_add_func ("/libmbim-glib/message-contents/basic-connect/device-caps", test_message_contents_basic_connect_device_caps);
    g_test_add_func ("/libmbim-glib/message-contents/basic-connect/ip-configuration", test_message_contents_basic_connect_ip_configuration);

    return g_test_run ();
}
