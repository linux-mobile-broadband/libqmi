/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2013 - 2014 Aleksander Morgado <aleksander@aleksander.es>
 */

#include <config.h>
#include <string.h>

#include "mbim-basic-connect.h"
#include "mbim-sms.h"
#include "mbim-ussd.h"
#include "mbim-auth.h"
#include "mbim-stk.h"
#include "mbim-ms-firmware-id.h"
#include "mbim-ms-basic-connect-extensions.h"
#include "mbim-ms-uicc-low-level-access.h"
#include "mbim-message.h"
#include "mbim-tlv.h"
#include "mbim-cid.h"
#include "mbim-common.h"
#include "mbim-error-types.h"

static void
test_message_trace (const guint8 *computed,
                    guint32       computed_size,
                    const guint8 *expected,
                    guint32       expected_size)
{
    g_autofree gchar *message_str = NULL;
    g_autofree gchar *expected_str = NULL;

    message_str = mbim_common_str_hex (computed, computed_size, ':');
    expected_str = mbim_common_str_hex (expected, expected_size, ':');

    /* Dump all message contents */
    g_print ("\n"
             "Message str:\n"
             "'%s'\n"
             "Expected str:\n"
             "'%s'\n",
             message_str,
             expected_str);

    /* If they are different, tell which are the different bytes */
    if (computed_size != expected_size ||
        memcmp (computed, expected, expected_size)) {
        guint32 i;

        for (i = 0; i < MIN (computed_size, expected_size); i++) {
            if (computed[i] != expected[i])
                g_print ("Byte [%u] is different (computed: 0x%02X vs expected: 0x%02x)\n", i, computed[i], expected[i]);
        }
    }
}

static void
test_message_printable (MbimMessage *message,
                        guint8       mbimex_version_major,
                        guint8       mbimex_version_minor)
{
    g_autofree gchar *printable = NULL;

    printable = mbim_message_get_printable_full (message,
                                                 mbimex_version_major,
                                                 mbimex_version_minor,
                                                 "---- ",
                                                 FALSE,
                                                 NULL);
    g_print ("\n"
             "Message printable:\n"
             "%s\n",
             printable);
}

static void
test_basic_connect_visible_providers (void)
{
    guint32 n_providers;
    g_autoptr(GError) error = NULL;
    g_autoptr(MbimProviderArray) providers = NULL;
    g_autoptr(MbimMessage) response = NULL;

    const guint8 buffer [] =  {
        /* header */
        0x03, 0x00, 0x00, 0x80, /* type */
        0xB4, 0x00, 0x00, 0x00, /* length */
        0x02, 0x00, 0x00, 0x00, /* transaction id */
        /* fragment header */
        0x01, 0x00, 0x00, 0x00, /* total */
        0x00, 0x00, 0x00, 0x00, /* current */
        /* command_done_message */
        0xA2, 0x89, 0xCC, 0x33, /* service id */
        0xBC, 0xBB, 0x8B, 0x4F,
        0xB6, 0xB0, 0x13, 0x3E,
        0xC2, 0xAA, 0xE6, 0xDF,
        0x08, 0x00, 0x00, 0x00, /* command id */
        0x00, 0x00, 0x00, 0x00, /* status code */
        0x84, 0x00, 0x00, 0x00, /* buffer length */
        /* information buffer */
        0x02, 0x00, 0x00, 0x00, /* 0x00 providers count */
        0x14, 0x00, 0x00, 0x00, /* 0x04 provider 0 offset */
        0x38, 0x00, 0x00, 0x00, /* 0x08 provider 0 length */
        0x4C, 0x00, 0x00, 0x00, /* 0x0C provider 1 offset */
        0x38, 0x00, 0x00, 0x00, /* 0x10 provider 1 length */
        /* data buffer... struct provider 0 */
        0x20, 0x00, 0x00, 0x00, /* 0x14 [0x00] id offset */
        0x0A, 0x00, 0x00, 0x00, /* 0x18 [0x04] id length */
        0x08, 0x00, 0x00, 0x00, /* 0x1C [0x08] state */
        0x2C, 0x00, 0x00, 0x00, /* 0x20 [0x0C] name offset */
        0x0C, 0x00, 0x00, 0x00, /* 0x24 [0x10] name length */
        0x01, 0x00, 0x00, 0x00, /* 0x28 [0x14] cellular class */
        0x0B, 0x00, 0x00, 0x00, /* 0x2C [0x18] rssi */
        0x00, 0x00, 0x00, 0x00, /* 0x30 [0x1C] error rate */
        0x32, 0x00, 0x31, 0x00, /* 0x34 [0x20] id string (10 bytes) */
        0x34, 0x00, 0x30, 0x00,
        0x33, 0x00, 0x00, 0x00,
        0x4F, 0x00, 0x72, 0x00, /* 0x40 [0x2C] name string (12 bytes) */
        0x61, 0x00, 0x6E, 0x00,
        0x67, 0x00, 0x65, 0x00,
        /* data buffer... struct provider 1 */
        0x20, 0x00, 0x00, 0x00, /* 0x4C [0x00] id offset */
        0x0A, 0x00, 0x00, 0x00, /* 0x50 [0x04] id length */
        0x19, 0x00, 0x00, 0x00, /* 0x51 [0x08] state */
        0x2C, 0x00, 0x00, 0x00, /* 0x54 [0x0C] name offset */
        0x0C, 0x00, 0x00, 0x00, /* 0x58 [0x10] name length */
        0x01, 0x00, 0x00, 0x00, /* 0x5C [0x14] cellular class */
        0x0B, 0x00, 0x00, 0x00, /* 0x60 [0x18] rssi */
        0x00, 0x00, 0x00, 0x00, /* 0x64 [0x1C] error rate */
        0x32, 0x00, 0x31, 0x00, /* 0x68 [0x20] id string (10 bytes) */
        0x34, 0x00, 0x30, 0x00,
        0x33, 0x00, 0x00, 0x00,
        0x4F, 0x00, 0x72, 0x00, /* 0x74 [0x2C] name string (12 bytes) */
        0x61, 0x00, 0x6E, 0x00,
        0x67, 0x00, 0x65, 0x00 };

    response = mbim_message_new (buffer, sizeof (buffer));
    test_message_printable (response, 1, 0);

    g_assert (mbim_message_visible_providers_response_parse (
                  response,
                  &n_providers,
                  &providers,
                  &error));

    g_assert_no_error (error);

    g_assert_cmpuint (n_providers, ==, 2);

    /* Provider [0]
     * Provider ID: '21403'
     * Provider Name: 'Orange'
     * State: 'visible'
     * Cellular class: 'gsm'
     * RSSI: '11'
     * Error rate: '0'
     */
    g_assert_cmpstr (providers[0]->provider_id, ==, "21403");
    g_assert_cmpstr (providers[0]->provider_name, ==, "Orange");
    g_assert_cmpuint (providers[0]->provider_state, ==, MBIM_PROVIDER_STATE_VISIBLE);
    g_assert_cmpuint (providers[0]->cellular_class, ==, MBIM_CELLULAR_CLASS_GSM);
    g_assert_cmpuint (providers[0]->rssi, ==, 11);
    g_assert_cmpuint (providers[0]->error_rate, ==, 0);

    /* Provider [1]:
     * Provider ID: '21403'
     * Provider Name: 'Orange'
     * State: 'home, visible, registered'
     * Cellular class: 'gsm'
     * RSSI: '11'
     * Error rate: '0'
     */
    g_assert_cmpstr (providers[1]->provider_id, ==, "21403");
    g_assert_cmpstr (providers[1]->provider_name, ==, "Orange");
    g_assert_cmpuint (providers[1]->provider_state, ==, (MBIM_PROVIDER_STATE_HOME |
                                                         MBIM_PROVIDER_STATE_VISIBLE |
                                                         MBIM_PROVIDER_STATE_REGISTERED));
    g_assert_cmpuint (providers[1]->cellular_class, ==, MBIM_CELLULAR_CLASS_GSM);
    g_assert_cmpuint (providers[1]->rssi, ==, 11);
    g_assert_cmpuint (providers[1]->error_rate, ==, 0);
}

static void
test_basic_connect_subscriber_ready_status (void)
{
    MbimSubscriberReadyState ready_state;
    MbimReadyInfoFlag ready_info;
    guint32 telephone_numbers_count;
    g_autofree gchar *subscriber_id = NULL;
    g_autofree gchar *sim_iccid = NULL;
    g_auto(GStrv) telephone_numbers = NULL;
    g_autoptr(GError) error = NULL;
    g_autoptr(MbimMessage) response = NULL;

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
    test_message_printable (response, 1, 0);

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
}

static void
test_basic_connect_device_caps (void)
{
    MbimDeviceType device_type;
    MbimCellularClass cellular_class;
    MbimVoiceClass voice_class;
    MbimSimClass sim_class;
    MbimDataClass data_class;
    MbimSmsCaps sms_caps;
    MbimCtrlCaps ctrl_caps;
    guint32 max_sessions;
    g_autofree gchar *custom_data_class = NULL;
    g_autofree gchar *device_id = NULL;
    g_autofree gchar *firmware_info = NULL;
    g_autofree gchar *hardware_info = NULL;
    g_autoptr(GError) error = NULL;
    g_autoptr(MbimMessage) response = NULL;

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
    test_message_printable (response, 1, 0);

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
}

static void
test_basic_connect_ip_configuration (void)
{
    guint32 session_id;
    MbimIPConfigurationAvailableFlag ipv4configurationavailable;
    MbimIPConfigurationAvailableFlag ipv6configurationavailable;
    guint32 ipv4addresscount;
    guint32 ipv6addresscount;
    const MbimIPv4 *ipv4gateway;
    const MbimIPv6 *ipv6gateway;
    guint32 ipv4dnsservercount;
    guint32 ipv6dnsservercount;
    guint32 ipv4mtu;
    guint32 ipv6mtu;
    g_autofree MbimIPv4 *ipv4dnsserver = NULL;
    g_autofree MbimIPv6 *ipv6dnsserver = NULL;
    g_autoptr(MbimIPv4ElementArray) ipv4address = NULL;
    g_autoptr(MbimIPv6ElementArray) ipv6address = NULL;
    g_autoptr(GError) error = NULL;
    g_autoptr(MbimMessage) response = NULL;

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
    test_message_printable (response, 1, 0);

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
}


static void
test_basic_connect_ip_configuration_2 (void)
{
    guint32 session_id;
    MbimIPConfigurationAvailableFlag ipv4configurationavailable;
    MbimIPConfigurationAvailableFlag ipv6configurationavailable;
    guint32 ipv4addresscount;
    guint32 ipv6addresscount;
    const MbimIPv4 *ipv4gateway;
    const MbimIPv6 *ipv6gateway;
    guint32 ipv4dnsservercount;
    guint32 ipv6dnsservercount;
    guint32 ipv4mtu;
    guint32 ipv6mtu;
    g_autofree MbimIPv4 *ipv4dnsserver = NULL;
    g_autofree MbimIPv6 *ipv6dnsserver = NULL;
    g_autoptr(MbimIPv4ElementArray) ipv4address = NULL;
    g_autoptr(MbimIPv6ElementArray) ipv6address = NULL;
    g_autoptr(GError) error = NULL;
    g_autoptr(MbimMessage) response = NULL;

    const guint8 buffer [] =  {
        /* header */
        0x03, 0x00, 0x00, 0x80, /* type */
        0xC4, 0x00, 0x00, 0x00, /* length */
        0x24, 0x00, 0x00, 0x00, /* transaction id */
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
        0x94, 0x00, 0x00, 0x00, /* buffer length */
        /* information buffer */
        0x00, 0x00, 0x00, 0x00, /* session id */
        0x0F, 0x00, 0x00, 0x00, /* IPv4ConfigurationAvailable */
        0x0F, 0x00, 0x00, 0x00, /* IPv6ConfigurationAvailable */
        0x01, 0x00, 0x00, 0x00, /* IPv4 element count */
        0x3C, 0x00, 0x00, 0x00, /* IPv4 element offset */
        0x01, 0x00, 0x00, 0x00, /* IPv6 element count */
        0x50, 0x00, 0x00, 0x00, /* IPv6 element offset */
        0x44, 0x00, 0x00, 0x00, /* IPv4 gateway offset */
        0x64, 0x00, 0x00, 0x00, /* IPv6 gateway offset */
        0x02, 0x00, 0x00, 0x00, /* IPv4 DNS count */
        0x48, 0x00, 0x00, 0x00, /* IPv4 DNS offset */
        0x02, 0x00, 0x00, 0x00, /* IPv6 DNS count */
        0x74, 0x00, 0x00, 0x00, /* IPv6 DNS offset */
        0xDC, 0x05, 0x00, 0x00, /* IPv4 MTU */
        0xDC, 0x05, 0x00, 0x00, /* IPv6 MTU */
        /* data buffer */
        0x1D, 0x00, 0x00, 0x00, /* IPv4 element (netmask) */
        0x1C, 0xF6, 0xC9, 0xDB, /* IPv4 element (address) */
        0x1C, 0xF6, 0xC9, 0xDC, /* IPv4 gateway */
        0x0A, 0xB1, 0x00, 0x22, /* IPv4 DNS1 */
        0x0A, 0xB1, 0x00, 0xD2, /* IPv4 DNS2 */
        0x40, 0x00, 0x00, 0x00, /* IPv6 element (netmask) */
        0x26, 0x07, 0xFB, 0x90, /* IPv6 element (address) */
        0x64, 0x3B, 0x28, 0x1F,
        0x1D, 0xFF, 0xBF, 0x3D,
        0xC5, 0xC8, 0x48, 0xAD,
        0x26, 0x07, 0xFB, 0x90, /* IPv6 gateway */
        0x64, 0x3B, 0x28, 0x1F,
        0xFD, 0xF7, 0x80, 0xF4,
        0xE3, 0x99, 0x98, 0x4A,
        0xFD, 0x00, 0x97, 0x6A, /* IPv6 DNS1 */
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x09,
        0xFD, 0x00, 0x97, 0x6A, /* IPv6 DNS2 */
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x10
    };

    response = mbim_message_new (buffer, sizeof (buffer));
    test_message_printable (response, 1, 0);

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
     *       IP [0]: '28.246.201.219/29'
     *     gateway: '28.246.201.220'
     *     DNS addresses (2)
     *       DNS [0]: '10.177.0.34'
     *       DNS [1]: '10.177.0.210'
     *     MTU: '1500'
     *   IPv6 configuration available: 'address, gateway, dns, mtu'
     *     IP addresses (1)
     *       IP [0]: '2607:fb90:643b:281f:1dff:bf3d:c5c8:48ad/64'
     *     gateway: '2607:fb90:643b:281f:fdf7:80f4:e399:984a'
     *     DNS addresses (2)
     *       DNS [0]: 'fd00:976a::9'
     */

    g_assert_cmpuint (session_id, ==, 0);
    g_assert_cmpuint (ipv4configurationavailable, ==, (MBIM_IP_CONFIGURATION_AVAILABLE_FLAG_ADDRESS |
                                                       MBIM_IP_CONFIGURATION_AVAILABLE_FLAG_GATEWAY |
                                                       MBIM_IP_CONFIGURATION_AVAILABLE_FLAG_DNS |
                                                       MBIM_IP_CONFIGURATION_AVAILABLE_FLAG_MTU));
    g_assert_cmpuint (ipv6configurationavailable, ==, MBIM_IP_CONFIGURATION_AVAILABLE_FLAG_ADDRESS |
                                                       MBIM_IP_CONFIGURATION_AVAILABLE_FLAG_GATEWAY |
                                                       MBIM_IP_CONFIGURATION_AVAILABLE_FLAG_DNS |
                                                       MBIM_IP_CONFIGURATION_AVAILABLE_FLAG_MTU);

    {
        MbimIPv4 addr = { .addr = { 0x1C, 0xF6, 0xC9, 0xDB } };

        g_assert_cmpuint (ipv4addresscount, ==, 1);
        g_assert_cmpuint (ipv4address[0]->on_link_prefix_length, ==, 29);
        g_assert (memcmp (&addr, &(ipv4address[0]->ipv4_address), 4) == 0);
    }

    {
        MbimIPv4 gateway_addr = { .addr = { 0x1C, 0xF6, 0xC9, 0xDC } };

        g_assert (memcmp (&gateway_addr, ipv4gateway, 4) == 0);
    }

    {
        MbimIPv4 dns_addr_1 = { .addr = { 0x0A, 0xB1, 0x00, 0x22 } };
        MbimIPv4 dns_addr_2 = { .addr = { 0x0A, 0xB1, 0x00, 0xD2 } };

        g_assert_cmpuint (ipv4dnsservercount, ==, 2);
        g_assert (memcmp (&dns_addr_1, &ipv4dnsserver[0], 4) == 0);
        g_assert (memcmp (&dns_addr_2, &ipv4dnsserver[1], 4) == 0);
    }

    g_assert_cmpuint (ipv4mtu, ==, 1500);

    {
        MbimIPv6 addr = { .addr = { 0x26, 0x07, 0xFB, 0x90,
                                    0x64, 0x3B, 0x28, 0x1F,
                                    0x1D, 0xFF, 0xBF, 0x3D,
                                    0xC5, 0xC8, 0x48, 0xAD } };

        g_assert_cmpuint (ipv6addresscount, ==, 1);
        g_assert_cmpuint (ipv6address[0]->on_link_prefix_length, ==, 64);
        g_assert (memcmp (&addr, &(ipv6address[0]->ipv6_address), 16) == 0);
    }

    {
        MbimIPv6 dns_addr_1 = { .addr = { 0xFD, 0x00, 0x97, 0x6A,
                                          0x00, 0x00, 0x00, 0x00,
                                          0x00, 0x00, 0x00, 0x00,
                                          0x00, 0x00, 0x00, 0x09 } };
        MbimIPv6 dns_addr_2 = { .addr = { 0xFD, 0x00, 0x97, 0x6A,
                                          0x00, 0x00, 0x00, 0x00,
                                          0x00, 0x00, 0x00, 0x00,
                                          0x00, 0x00, 0x00, 0x10 } };

        g_assert_cmpuint (ipv6dnsservercount, ==, 2);
        g_assert (memcmp (&dns_addr_1, &ipv6dnsserver[0], 16) == 0);
        g_assert (memcmp (&dns_addr_2, &ipv6dnsserver[1], 16) == 0);
    }
}

static void
test_basic_connect_service_activation (void)
{
    guint32 nw_error;
    const guint8 *databuffer;
    guint32 databuffer_size;
    g_autoptr(GError) error = NULL;
    g_autoptr(MbimMessage) response = NULL;

    const guint8 expected_databuffer [] =  {
        0x01, 0x02, 0x03, 0x04,
        0x05, 0x06, 0x07, 0x08
    };
    const guint8 buffer [] =  {
        /* header */
        0x03, 0x00, 0x00, 0x80, /* type */
        0x3C, 0x00, 0x00, 0x00, /* length */
        0x02, 0x00, 0x00, 0x00, /* transaction id */
        /* fragment header */
        0x01, 0x00, 0x00, 0x00, /* total */
        0x00, 0x00, 0x00, 0x00, /* current */
        /* command_done_message */
        0xA2, 0x89, 0xCC, 0x33, /* service id */
        0xBC, 0xBB, 0x8B, 0x4F,
        0xB6, 0xB0, 0x13, 0x3E,
        0xC2, 0xAA, 0xE6, 0xDF,
        0x0E, 0x00, 0x00, 0x00, /* command id */
        0x00, 0x00, 0x00, 0x00, /* status code */
        0x0C, 0x00, 0x00, 0x00, /* buffer length */
        /* information buffer */
        0x06, 0x00, 0x00, 0x00, /* nw error */
        0x01, 0x02, 0x03, 0x04, /* buffer */
        0x05, 0x06, 0x07, 0x08  };

    response = mbim_message_new (buffer, sizeof (buffer));
    test_message_printable (response, 1, 0);

    g_assert (mbim_message_service_activation_response_parse (
                  response,
                  &nw_error,
                  &databuffer_size,
                  &databuffer,
                  &error));

    g_assert_no_error (error);

    g_assert_cmpuint (nw_error, ==, MBIM_NW_ERROR_ILLEGAL_ME);
    g_assert_cmpuint (databuffer_size, ==, sizeof (expected_databuffer));
    g_assert (memcmp (databuffer, expected_databuffer, databuffer_size) == 0);
}

static void
test_basic_connect_register_state (void)
{
    MbimNwError nw_error;
    MbimRegisterState register_state;
    MbimRegisterMode register_mode;
    MbimDataClass available_data_classes;
    MbimCellularClass current_cellular_class;
    MbimRegistrationFlag registration_flag;
    g_autofree gchar *provider_id;
    g_autofree gchar *provider_name;
    g_autofree gchar *roaming_text;
    g_autoptr(GError) error = NULL;
    g_autoptr(MbimMessage) response = NULL;

    const guint8 buffer [] =  {
        /* header */
        0x03, 0x00, 0x00, 0x80, /* type */
        0x6C, 0x00, 0x00, 0x00, /* length */
        0x12, 0x00, 0x00, 0x00, /* transaction id */
        /* fragment header */
        0x01, 0x00, 0x00, 0x00, /* total */
        0x00, 0x00, 0x00, 0x00, /* current */
        /* command_done message */
        0xA2, 0x89, 0xCC, 0x33, /* service id */
        0xBC, 0xBB, 0x8B, 0x4F,
        0xB6, 0xB0, 0x13, 0x3E,
        0xC2, 0xAA, 0xE6, 0xDF,
        0x09, 0x00, 0x00, 0x00, /* command id */
        0x00, 0x00, 0x00, 0x00, /* status code */
        0x3C, 0x00, 0x00, 0x00, /* buffer length */
        /* information buffer */
        0x00, 0x00, 0x00, 0x00, /* nw error */
        0x03, 0x00, 0x00, 0x00, /* register state */
        0x01, 0x00, 0x00, 0x00, /* register mode */
        0x1C, 0x00, 0x00, 0x00, /* available data classes */
        0x01, 0x00, 0x00, 0x00, /* current cellular class */
        0x30, 0x00, 0x00, 0x00, /* provider id offset */
        0x0A, 0x00, 0x00, 0x00, /* provider id size */
        0x00, 0x00, 0x00, 0x00, /* provider name offset */
        0x00, 0x00, 0x00, 0x00, /* provider name size */
        0x00, 0x00, 0x00, 0x00, /* roaming text offset */
        0x00, 0x00, 0x00, 0x00, /* roaming text size */
        0x02, 0x00, 0x00, 0x00, /* registration flag */
        /* data buffer */
        0x32, 0x00, 0x36, 0x00,
        0x30, 0x00, 0x30, 0x00,
        0x36, 0x00, 0x00, 0x00 };

    response = mbim_message_new (buffer, sizeof (buffer));
    test_message_printable (response, 1, 0);

    g_assert (mbim_message_register_state_response_parse (
                  response,
                  &nw_error,
                  &register_state,
                  &register_mode,
                  &available_data_classes,
                  &current_cellular_class,
                  &provider_id,
                  &provider_name,
                  &roaming_text,
                  &registration_flag,
                  &error));

    g_assert_no_error (error);

    g_assert_cmpuint (nw_error, ==, MBIM_NW_ERROR_NONE);
    g_assert_cmpuint (register_state, ==, MBIM_REGISTER_STATE_HOME);
    g_assert_cmpuint (register_mode, ==, MBIM_REGISTER_MODE_AUTOMATIC);
    g_assert_cmpuint (available_data_classes, ==, (MBIM_DATA_CLASS_UMTS |
                                                   MBIM_DATA_CLASS_HSDPA |
                                                   MBIM_DATA_CLASS_HSUPA));
    g_assert_cmpuint (current_cellular_class, ==, MBIM_CELLULAR_CLASS_GSM);
    g_assert_cmpstr (provider_id, ==, "26006");
    g_assert (provider_name == NULL);
    g_assert (roaming_text == NULL);
    g_assert_cmpuint (registration_flag, ==, MBIM_REGISTRATION_FLAG_PACKET_SERVICE_AUTOMATIC_ATTACH);
}

static void
test_provisioned_contexts (void)
{
    guint32 provisioned_contexts_count = 0;
    g_autoptr(MbimProvisionedContextElementArray) provisioned_contexts = NULL;
    g_autoptr(GError) error = NULL;
    g_autoptr(MbimMessage) response = NULL;

    const guint8 buffer [] = {
        /* header */
        0x03, 0x00, 0x00, 0x80, /* type */
        0x30, 0x00, 0x00, 0x00, /* length */
        0x1C, 0x00, 0x00, 0x00, /* transaction id */
        /* fragment header */
        0x01, 0x00, 0x00, 0x00, /* total */
        0x00, 0x00, 0x00, 0x00, /* length */
        /* command_done_message */
        0xA2, 0x89, 0xCC, 0x33, /* service id */
        0xBC, 0xBB, 0x8B, 0x4F,
        0xB6, 0xB0, 0x13, 0x3E,
        0xC2, 0xAA, 0xE6, 0xDF,
        0x0D, 0x00, 0x00, 0x00, /* command id */
        0x00, 0x00, 0x00, 0x00, /* status code */
        0x00, 0x00, 0x00, 0x00  /* buffer length */
    };

    response = mbim_message_new (buffer, sizeof (buffer));
    test_message_printable (response, 1, 0);

    g_assert (!mbim_message_provisioned_contexts_response_parse (
                  response,
                  &provisioned_contexts_count,
                  &provisioned_contexts,
                  &error));

    g_assert_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_INVALID_MESSAGE);
}

static void
test_sms_read_zero_pdu (void)
{
    MbimSmsFormat format;
    guint32 messages_count;
    g_autoptr(MbimSmsPduReadRecordArray) pdu_messages = NULL;
    g_autoptr(MbimSmsCdmaReadRecordArray) cdma_messages = NULL;
    g_autoptr(GError) error = NULL;
    g_autoptr(MbimMessage) response = NULL;

    const guint8 buffer [] =  {
        /* header */
        0x03, 0x00, 0x00, 0x80, /* type */
        0x38, 0x00, 0x00, 0x00, /* length */
        0x02, 0x00, 0x00, 0x00, /* transaction id */
        /* fragment header */
        0x01, 0x00, 0x00, 0x00, /* total */
        0x00, 0x00, 0x00, 0x00, /* current */
        /* command_done_message */
        0x53, 0x3F, 0xBE, 0xEB, /* service id */
        0x14, 0xFE, 0x44, 0x67,
        0x9F, 0x90, 0x33, 0xA2,
        0x23, 0xE5, 0x6C, 0x3F,
        0x02, 0x00, 0x00, 0x00, /* command id */
        0x00, 0x00, 0x00, 0x00, /* status code */
        0x08, 0x00, 0x00, 0x00, /* buffer length */
        /* information buffer */
        0x00, 0x00, 0x00, 0x00, /* 0x00 format */
        0x00, 0x00, 0x00, 0x00, /* 0x04 messages count */
    };

    response = mbim_message_new (buffer, sizeof (buffer));
    test_message_printable (response, 1, 0);

    g_assert (mbim_message_sms_read_response_parse (
                  response,
                  &format,
                  &messages_count,
                  &pdu_messages,
                  &cdma_messages,
                  &error));
    g_assert_no_error (error);

    g_assert_cmpuint (format, ==, MBIM_SMS_FORMAT_PDU);
    g_assert_cmpuint (messages_count, ==, 0);
    g_assert (pdu_messages == NULL);
    g_assert (cdma_messages == NULL);
}

static void
test_sms_read_single_pdu (void)
{
    MbimSmsFormat format;
    guint32 messages_count;
    g_autoptr(MbimSmsPduReadRecordArray) pdu_messages = NULL;
    g_autoptr(MbimSmsCdmaReadRecordArray) cdma_messages = NULL;
    g_autoptr(GError) error = NULL;
    g_autoptr(MbimMessage) response = NULL;

    const guint8 buffer [] =  {
        /* header */
        0x03, 0x00, 0x00, 0x80, /* type */
        0x60, 0x00, 0x00, 0x00, /* length */
        0x02, 0x00, 0x00, 0x00, /* transaction id */
        /* fragment header */
        0x01, 0x00, 0x00, 0x00, /* total */
        0x00, 0x00, 0x00, 0x00, /* current */
        /* command_done_message */
        0x53, 0x3F, 0xBE, 0xEB, /* service id */
        0x14, 0xFE, 0x44, 0x67,
        0x9F, 0x90, 0x33, 0xA2,
        0x23, 0xE5, 0x6C, 0x3F,
        0x02, 0x00, 0x00, 0x00, /* command id */
        0x00, 0x00, 0x00, 0x00, /* status code */
        0x30, 0x00, 0x00, 0x00, /* buffer length */
        /* information buffer */
        0x00, 0x00, 0x00, 0x00, /* 0x00 format */
        0x01, 0x00, 0x00, 0x00, /* 0x04 messages count */
        0x10, 0x00, 0x00, 0x00, /* 0x08 message 1 offset */
        0x20, 0x00, 0x00, 0x00, /* 0x0C message 1 length */
        /* data buffer... message 1 */
        0x07, 0x00, 0x00, 0x00, /* 0x10 0x00 message index */
        0x03, 0x00, 0x00, 0x00, /* 0x14 0x04 message status */
        0x10, 0x00, 0x00, 0x00, /* 0x18 0x08 pdu data offset (w.r.t. pdu start */
        0x10, 0x00, 0x00, 0x00, /* 0x1C 0x0C pdu data length */
        /*    pdu data... */
        0x01, 0x02, 0x03, 0x04, /* 0x20 0x10 */
        0x05, 0x06, 0x07, 0x08,
        0x09, 0x0A, 0x0B, 0x0C,
        0x0D, 0x0E, 0x0F, 0x00
    };

    const guint8 expected_pdu [] = {
        0x01, 0x02, 0x03, 0x04,
        0x05, 0x06, 0x07, 0x08,
        0x09, 0x0A, 0x0B, 0x0C,
        0x0D, 0x0E, 0x0F, 0x00
    };

    response = mbim_message_new (buffer, sizeof (buffer));
    test_message_printable (response, 1, 0);

    g_assert (mbim_message_sms_read_response_parse (
                  response,
                  &format,
                  &messages_count,
                  &pdu_messages,
                  &cdma_messages,
                  &error));
    g_assert_no_error (error);

    g_assert_cmpuint (format, ==, MBIM_SMS_FORMAT_PDU);
    g_assert_cmpuint (messages_count, ==, 1);
    g_assert (pdu_messages != NULL);
    g_assert (cdma_messages == NULL);

    g_assert_cmpuint (pdu_messages[0]->message_index, ==, 7);
    g_assert_cmpuint (pdu_messages[0]->message_status, ==, MBIM_SMS_STATUS_SENT);
    test_message_trace (pdu_messages[0]->pdu_data,
                        pdu_messages[0]->pdu_data_size,
                        expected_pdu,
                        sizeof (expected_pdu));
    g_assert_cmpuint (pdu_messages[0]->pdu_data_size, ==, sizeof (expected_pdu));
    g_assert (memcmp (pdu_messages[0]->pdu_data, expected_pdu, sizeof (expected_pdu)) == 0);
}

static void
test_sms_read_multiple_pdu (void)
{
    guint32 idx;
    MbimSmsFormat format;
    guint32 messages_count;
    g_autoptr(MbimSmsPduReadRecordArray) pdu_messages = NULL;
    g_autoptr(MbimSmsCdmaReadRecordArray) cdma_messages = NULL;
    g_autoptr(GError) error = NULL;
    g_autoptr(MbimMessage) response = NULL;

    const guint8 buffer [] =  {
        /* header */
        0x03, 0x00, 0x00, 0x80, /* type */
        0x88, 0x00, 0x00, 0x00, /* length */
        0x02, 0x00, 0x00, 0x00, /* transaction id */
        /* fragment header */
        0x01, 0x00, 0x00, 0x00, /* total */
        0x00, 0x00, 0x00, 0x00, /* current */
        /* command_done_message */
        0x53, 0x3F, 0xBE, 0xEB, /* service id */
        0x14, 0xFE, 0x44, 0x67,
        0x9F, 0x90, 0x33, 0xA2,
        0x23, 0xE5, 0x6C, 0x3F,
        0x02, 0x00, 0x00, 0x00, /* command id */
        0x00, 0x00, 0x00, 0x00, /* status code */
        0x58, 0x00, 0x00, 0x00, /* buffer length */
        /* information buffer */
        0x00, 0x00, 0x00, 0x00, /* 0x00 format */
        0x02, 0x00, 0x00, 0x00, /* 0x04 messages count */
        0x18, 0x00, 0x00, 0x00, /* 0x08 message 1 offset */
        0x20, 0x00, 0x00, 0x00, /* 0x0C message 1 length */
        0x38, 0x00, 0x00, 0x00, /* 0x10 message 2 offset */
        0x24, 0x00, 0x00, 0x00, /* 0x14 message 2 length */
        /* data buffer... message 1 */
        0x06, 0x00, 0x00, 0x00, /* 0x18 0x00 message index */
        0x03, 0x00, 0x00, 0x00, /* 0x1C 0x04 message status */
        0x10, 0x00, 0x00, 0x00, /* 0x20 0x08 pdu data offset (w.r.t. pdu start */
        0x10, 0x00, 0x00, 0x00, /* 0x24 0x0C pdu data length */
        /*    pdu data... */
        0x01, 0x02, 0x03, 0x04, /* 0x28 0x10 */
        0x05, 0x06, 0x07, 0x08,
        0x09, 0x0A, 0x0B, 0x0C,
        0x0D, 0x0E, 0x0F, 0x00,
        /* data buffer... message 2 */
        0x07, 0x00, 0x00, 0x00, /* 0x38 0x00 message index */
        0x03, 0x00, 0x00, 0x00, /* 0x3C 0x04 message status */
        0x10, 0x00, 0x00, 0x00, /* 0x40 0x08 pdu data offset (w.r.t. pdu start */
        0x10, 0x00, 0x00, 0x00, /* 0x44 0x0C pdu data length */
        /*    pdu data... */
        0x00, 0x01, 0x02, 0x03, /* 0x48 0x10 */
        0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B,
        0x0C, 0x0D, 0x0E, 0x0F
    };

    const guint8 expected_pdu_idx6 [] = {
        0x01, 0x02, 0x03, 0x04,
        0x05, 0x06, 0x07, 0x08,
        0x09, 0x0A, 0x0B, 0x0C,
        0x0D, 0x0E, 0x0F, 0x00
    };

    const guint8 expected_pdu_idx7 [] = {
        0x00, 0x01, 0x02, 0x03,
        0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B,
        0x0C, 0x0D, 0x0E, 0x0F
    };

    response = mbim_message_new (buffer, sizeof (buffer));
    test_message_printable (response, 1, 0);

    g_assert (mbim_message_sms_read_response_parse (
                  response,
                  &format,
                  &messages_count,
                  &pdu_messages,
                  &cdma_messages,
                  &error));
    g_assert_no_error (error);

    g_assert_cmpuint (format, ==, MBIM_SMS_FORMAT_PDU);
    g_assert_cmpuint (messages_count, ==, 2);
    g_assert (pdu_messages != NULL);
    g_assert (cdma_messages == NULL);

    /* First message with index 6 */
    if (pdu_messages[0]->message_index == 6)
        idx = 0;
    else if (pdu_messages[1]->message_index == 6)
        idx = 1;
    else
        g_assert_not_reached ();
    g_assert_cmpuint (pdu_messages[idx]->message_index, ==, 6);
    g_assert_cmpuint (pdu_messages[idx]->message_status, ==, MBIM_SMS_STATUS_SENT);
    test_message_trace (pdu_messages[idx]->pdu_data,
                        pdu_messages[idx]->pdu_data_size,
                        expected_pdu_idx6,
                        sizeof (expected_pdu_idx6));
    g_assert_cmpuint (pdu_messages[idx]->pdu_data_size, ==, sizeof (expected_pdu_idx6));
    g_assert (memcmp (pdu_messages[idx]->pdu_data, expected_pdu_idx6, sizeof (expected_pdu_idx6)) == 0);

    /* Second message with index 7 */
    if (pdu_messages[0]->message_index == 7)
        idx = 0;
    else if (pdu_messages[1]->message_index == 7)
        idx = 1;
    else
        g_assert_not_reached ();
    g_assert_cmpuint (pdu_messages[idx]->message_index, ==, 7);
    g_assert_cmpuint (pdu_messages[idx]->message_status, ==, MBIM_SMS_STATUS_SENT);
    test_message_trace (pdu_messages[idx]->pdu_data,
                        pdu_messages[idx]->pdu_data_size,
                        expected_pdu_idx7,
                        sizeof (expected_pdu_idx7));
    g_assert_cmpuint (pdu_messages[idx]->pdu_data_size, ==, sizeof (expected_pdu_idx7));
    g_assert (memcmp (pdu_messages[idx]->pdu_data, expected_pdu_idx7, sizeof (expected_pdu_idx7)) == 0);
}

static void
test_ussd (void)
{
    MbimUssdResponse ussd_response;
    MbimUssdSessionState ussd_session_state;
    const guint8 *ussd_payload;
    guint32 ussd_payload_size;
    guint32 ussd_dcs;
    g_autoptr(GError) error = NULL;
    g_autoptr(MbimMessage) response = NULL;

    const guint8 buffer [] =  {
        /* header */
        0x03, 0x00, 0x00, 0x80, /* type */
        0x54, 0x00, 0x00, 0x00, /* length */
        0x02, 0x00, 0x00, 0x00, /* transaction id */
        /* fragment header */
        0x01, 0x00, 0x00, 0x00, /* total */
        0x00, 0x00, 0x00, 0x00, /* current */
        /* command_done_message */
        0xE5, 0x50, 0xA0, 0xC8, /* service id */
        0x5E, 0x82, 0x47, 0x9E,
        0x82, 0xF7, 0x10, 0xAB,
        0xF4, 0xC3, 0x35, 0x1F,
        0x01, 0x00, 0x00, 0x00, /* command id */
        0x00, 0x00, 0x00, 0x00, /* status code */
        0x24, 0x00, 0x00, 0x00, /* buffer length */
        /* information buffer */
        0x05, 0x00, 0x00, 0x00, /* 0x00 response */
        0x01, 0x00, 0x00, 0x00, /* 0x04 sesstion state */
        0x01, 0x00, 0x00, 0x00, /* 0x08 coding scheme */
        0x14, 0x00, 0x00, 0x00, /* 0x0C payload offset */
        0x10, 0x00, 0x00, 0x00, /* 0x10 payload length */
        /* data buffer... payload */
        0x01, 0x02, 0x03, 0x04, /* 0x14 payload */
        0x05, 0x06, 0x07, 0x08,
        0x09, 0x0A, 0x0B, 0x0C,
        0x0D, 0x0E, 0x0F, 0x00
    };

    const guint8 expected_payload [] = {
        0x01, 0x02, 0x03, 0x04,
        0x05, 0x06, 0x07, 0x08,
        0x09, 0x0A, 0x0B, 0x0C,
        0x0D, 0x0E, 0x0F, 0x00
    };

    response = mbim_message_new (buffer, sizeof (buffer));
    test_message_printable (response, 1, 0);

    g_assert (mbim_message_ussd_response_parse (
                  response,
                  &ussd_response,
                  &ussd_session_state,
                  &ussd_dcs,
                  &ussd_payload_size,
                  &ussd_payload,
                  &error));
    g_assert_no_error (error);

    g_assert_cmpuint (ussd_response, ==, MBIM_USSD_RESPONSE_NETWORK_TIMEOUT);
    g_assert_cmpuint (ussd_session_state, ==, MBIM_USSD_SESSION_STATE_EXISTING_SESSION);
    g_assert_cmpuint (ussd_dcs, ==, 0x01);

    test_message_trace (ussd_payload,
                        ussd_payload_size,
                        expected_payload,
                        sizeof (expected_payload));
    g_assert_cmpuint (ussd_payload_size, ==, sizeof (expected_payload));
    g_assert (memcmp (ussd_payload, expected_payload, sizeof (expected_payload)) == 0);
}

static void
test_auth_akap (void)
{
    const guint8 *res;
    guint32 res_len;
    const guint8 *ik;
    const guint8 *ck;
    const guint8 *auts;
    g_autoptr(GError) error = NULL;
    g_autoptr(MbimMessage) response = NULL;

    const guint8 buffer [] =  {
        /* header */
        0x03, 0x00, 0x00, 0x80, /* type */
        0x74, 0x00, 0x00, 0x00, /* length */
        0x02, 0x00, 0x00, 0x00, /* transaction id */
        /* fragment header */
        0x01, 0x00, 0x00, 0x00, /* total */
        0x00, 0x00, 0x00, 0x00, /* current */
        /* command_done_message */
        0x1D, 0x2B, 0x5F, 0xF7, /* service id */
        0x0A, 0xA1, 0x48, 0xB2,
        0xAA, 0x52, 0x50, 0xF1,
        0x57, 0x67, 0x17, 0x4E,
        0x02, 0x00, 0x00, 0x00, /* command id */
        0x00, 0x00, 0x00, 0x00, /* status code */
        0x44, 0x00, 0x00, 0x00, /* buffer length */
        /* information buffer */
        0x00, 0x01, 0x02, 0x03, /* 0x00 Res */
        0x04, 0x05, 0x06, 0x07, /* 0x04 */
        0x08, 0x09, 0x0A, 0x0B, /* 0x08 */
        0x0C, 0x0D, 0x0E, 0x0F, /* 0x0C */
        0x05, 0x00, 0x00, 0x00, /* 0x10 Reslen */
        0xFF, 0xFE, 0xFD, 0xFC, /* 0x14 IK */
        0xFB, 0xFA, 0xF9, 0xF8, /* 0x18 */
        0xF7, 0xF6, 0xF5, 0xF4, /* 0x1C */
        0xF3, 0xF2, 0xF1, 0xF0, /* 0x20 */
        0xAF, 0xAE, 0xAD, 0xAC, /* 0x24 CK */
        0xAB, 0xAA, 0xA9, 0xA8, /* 0x28 */
        0xA7, 0xA6, 0xA5, 0xA4, /* 0x2C */
        0xA3, 0xA2, 0xA1, 0xA0, /* 0x30 */
        0x7F, 0x7E, 0x7D, 0x7C, /* 0x34 Auts */
        0x7B, 0x7A, 0x79, 0x78, /* 0x38 */
        0x77, 0x76, 0x75, 0x74, /* 0x3C */
        0x73, 0x72, 0x00, 0x00, /* 0x40 */
    };

    const guint8 expected_res [] = {
        0x00, 0x01, 0x02, 0x03,
        0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B,
        0x0C, 0x0D, 0x0E, 0x0F,
    };
    const guint8 expected_ik [] = {
        0xFF, 0xFE, 0xFD, 0xFC,
        0xFB, 0xFA, 0xF9, 0xF8,
        0xF7, 0xF6, 0xF5, 0xF4,
        0xF3, 0xF2, 0xF1, 0xF0,
    };
    const guint8 expected_ck [] = {
        0xAF, 0xAE, 0xAD, 0xAC,
        0xAB, 0xAA, 0xA9, 0xA8,
        0xA7, 0xA6, 0xA5, 0xA4,
        0xA3, 0xA2, 0xA1, 0xA0,
    };
    const guint8 expected_auts [] = {
        0x7F, 0x7E, 0x7D, 0x7C,
        0x7B, 0x7A, 0x79, 0x78,
        0x77, 0x76, 0x75, 0x74,
        0x73, 0x72
    };

    response = mbim_message_new (buffer, sizeof (buffer));
    test_message_printable (response, 1, 0);

    g_assert (mbim_message_auth_akap_response_parse (
                  response,
                  &res,
                  &res_len,
                  &ik,
                  &ck,
                  &auts,
                  &error));
    g_assert_no_error (error);

    test_message_trace (res,
                        sizeof (expected_res),
                        expected_res,
                        sizeof (expected_res));
    g_assert (memcmp (res, expected_res, sizeof (expected_res)) == 0);

    g_assert_cmpuint (res_len, ==, 5);

    test_message_trace (ik,
                        sizeof (expected_ik),
                        expected_ik,
                        sizeof (expected_ik));
    g_assert (memcmp (ik, expected_ik, sizeof (expected_ik)) == 0);

    test_message_trace (ck,
                        sizeof (expected_ck),
                        expected_ck,
                        sizeof (expected_ck));
    g_assert (memcmp (ck, expected_ck, sizeof (expected_ck)) == 0);

    test_message_trace (auts,
                        sizeof (expected_auts),
                        expected_auts,
                        sizeof (expected_auts));
    g_assert (memcmp (auts, expected_auts, sizeof (expected_auts)) == 0);
}

static void
test_stk_pac_notification (void)
{
    const guint8 *databuffer;
    guint32 databuffer_len;
    guint32 pac_type;
    g_autoptr(GError) error = NULL;
    g_autoptr(MbimMessage) response = NULL;

    const guint8 buffer [] =  {
        /* header */
        0x07, 0x00, 0x00, 0x80, /* type */
        0x54, 0x00, 0x00, 0x00, /* length */
        0x02, 0x00, 0x00, 0x00, /* transaction id */
        /* fragment header */
        0x01, 0x00, 0x00, 0x00, /* total */
        0x00, 0x00, 0x00, 0x00, /* current */
        /* indicate_status_message */
        0xD8, 0xF2, 0x01, 0x31, /* service id */
        0xFC, 0xB5, 0x4E, 0x17,
        0x86, 0x02, 0xD6, 0xED,
        0x38, 0x16, 0x16, 0x4C,
        0x01, 0x00, 0x00, 0x00, /* command id */
        0x28, 0x00, 0x00, 0x00, /* buffer length */
        /* information buffer */
        0x01, 0x00, 0x00, 0x00, /* 0x00 Pac Type */
        0x04, 0x05, 0x06, 0x07, /* 0x04 Data buffer */
        0xAF, 0xAE, 0xAD, 0xAC,
        0xAB, 0xAA, 0xA9, 0xA8,
        0xA7, 0xA6, 0xA5, 0xA4,
        0xA3, 0xA2, 0xA1, 0xA0,
        0x7F, 0x7E, 0x7D, 0x7C,
        0x7B, 0x7A, 0x79, 0x78,
        0x77, 0x76, 0x75, 0x74,
        0x73, 0x72, 0x00, 0x00
    };

    const guint8 expected_databuffer [] = {
        0x04, 0x05, 0x06, 0x07,
        0xAF, 0xAE, 0xAD, 0xAC,
        0xAB, 0xAA, 0xA9, 0xA8,
        0xA7, 0xA6, 0xA5, 0xA4,
        0xA3, 0xA2, 0xA1, 0xA0,
        0x7F, 0x7E, 0x7D, 0x7C,
        0x7B, 0x7A, 0x79, 0x78,
        0x77, 0x76, 0x75, 0x74,
        0x73, 0x72, 0x00, 0x00
    };

    response = mbim_message_new (buffer, sizeof (buffer));
    test_message_printable (response, 1, 0);

    g_assert (mbim_message_stk_pac_notification_parse (
                  response,
                  &pac_type,
                  &databuffer_len,
                  &databuffer,
                  &error));
    g_assert_no_error (error);

    g_assert_cmpuint (pac_type, ==, MBIM_STK_PAC_TYPE_NOTIFICATION);

    test_message_trace (databuffer,
                        sizeof (databuffer_len),
                        expected_databuffer,
                        sizeof (expected_databuffer));
    g_assert_cmpuint (databuffer_len, ==, sizeof (expected_databuffer));
    g_assert (memcmp (databuffer, expected_databuffer, sizeof (expected_databuffer)) == 0);
}

static void
test_stk_pac_response (void)
{
    const guint8 *databuffer;
    g_autoptr(GError) error = NULL;
    g_autoptr(MbimMessage) response = NULL;

    const guint8 buffer [] =  {
        /* header */
        0x03, 0x00, 0x00, 0x80, /* type */
        0x30, 0x01, 0x00, 0x00, /* length */
        0x02, 0x00, 0x00, 0x00, /* transaction id */
        /* fragment header */
        0x01, 0x00, 0x00, 0x00, /* total */
        0x00, 0x00, 0x00, 0x00, /* current */
        /* indicate_status_message */
        0xD8, 0xF2, 0x01, 0x31, /* service id */
        0xFC, 0xB5, 0x4E, 0x17,
        0x86, 0x02, 0xD6, 0xED,
        0x38, 0x16, 0x16, 0x4C,
        0x01, 0x00, 0x00, 0x00, /* command id */
        0x00, 0x00, 0x00, 0x00, /* status code */
        0x00, 0x01, 0x00, 0x00, /* buffer length (256) */
        /* information buffer */
        0x04, 0x05, 0x06, 0x07,
        0xAF, 0xAE, 0xAD, 0xAC,
        0xAB, 0xAA, 0xA9, 0xA8,
        0xA7, 0xA6, 0xA5, 0xA4,
        0xA3, 0xA2, 0xA1, 0xA0,
        0x7F, 0x7E, 0x7D, 0x7C,
        0x7B, 0x7A, 0x79, 0x78,
        0x77, 0x76, 0x75, 0x74,
        0x73, 0x72, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00,
        0x04, 0x05, 0x06, 0x07,
        0xAF, 0xAE, 0xAD, 0xAC,
        0xAB, 0xAA, 0xA9, 0xA8,
        0xA7, 0xA6, 0xA5, 0xA4,
        0xA3, 0xA2, 0xA1, 0xA0,
        0x7F, 0x7E, 0x7D, 0x7C,
        0x7B, 0x7A, 0x79, 0x78,
        0x77, 0x76, 0x75, 0x74,
        0x73, 0x72, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00,
        0x04, 0x05, 0x06, 0x07,
        0xAF, 0xAE, 0xAD, 0xAC,
        0xAB, 0xAA, 0xA9, 0xA8,
        0xA7, 0xA6, 0xA5, 0xA4,
        0xA3, 0xA2, 0xA1, 0xA0,
        0x7F, 0x7E, 0x7D, 0x7C,
        0x7B, 0x7A, 0x79, 0x78,
        0x77, 0x76, 0x75, 0x74,
        0x73, 0x72, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00,
        0x04, 0x05, 0x06, 0x07,
        0xAF, 0xAE, 0xAD, 0xAC,
        0xAB, 0xAA, 0xA9, 0xA8,
        0xA7, 0xA6, 0xA5, 0xA4,
        0xA3, 0xA2, 0xA1, 0xA0,
        0x7F, 0x7E, 0x7D, 0x7C,
        0x7B, 0x7A, 0x79, 0x78,
        0x77, 0x76, 0x75, 0x74,
        0x73, 0x72, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00,
        0x04, 0x05, 0x06, 0x07,
        0xAF, 0xAE, 0xAD, 0xAC,
        0xAB, 0xAA, 0xA9, 0xA8,
        0xA7, 0xA6, 0xA5, 0xA4,
        0xA3, 0xA2, 0xA1, 0xA0,
        0x7F, 0x7E, 0x7D, 0x7C,
        0x7B, 0x7A, 0x79, 0x78,
        0x77, 0x76, 0x75, 0x74,
        0x73, 0x72, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00,
        0x04, 0x05, 0x06, 0x07,
        0xAF, 0xAE, 0xAD, 0xAC,
        0xAB, 0xAA, 0xA9, 0xA8,
        0xA7, 0xA6, 0xA5, 0xA4,
        0xA3, 0xA2, 0xA1, 0xA0,
        0x7F, 0x7E, 0x7D, 0x7C,
        0x7B, 0x7A, 0x79, 0x78,
        0x77, 0x76, 0x75, 0x74,
        0x73, 0x72, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00,
        0x04, 0x05, 0x06, 0x07,
        0xAF, 0xAE, 0xAD, 0xAC,
        0xAB, 0xAA, 0xA9, 0xA8,
        0xA7, 0xA6, 0xA5, 0xA4,
    };

    const guint8 expected_databuffer [256] = {
        0x04, 0x05, 0x06, 0x07,
        0xAF, 0xAE, 0xAD, 0xAC,
        0xAB, 0xAA, 0xA9, 0xA8,
        0xA7, 0xA6, 0xA5, 0xA4,
        0xA3, 0xA2, 0xA1, 0xA0,
        0x7F, 0x7E, 0x7D, 0x7C,
        0x7B, 0x7A, 0x79, 0x78,
        0x77, 0x76, 0x75, 0x74,
        0x73, 0x72, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00,
        0x04, 0x05, 0x06, 0x07,
        0xAF, 0xAE, 0xAD, 0xAC,
        0xAB, 0xAA, 0xA9, 0xA8,
        0xA7, 0xA6, 0xA5, 0xA4,
        0xA3, 0xA2, 0xA1, 0xA0,
        0x7F, 0x7E, 0x7D, 0x7C,
        0x7B, 0x7A, 0x79, 0x78,
        0x77, 0x76, 0x75, 0x74,
        0x73, 0x72, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00,
        0x04, 0x05, 0x06, 0x07,
        0xAF, 0xAE, 0xAD, 0xAC,
        0xAB, 0xAA, 0xA9, 0xA8,
        0xA7, 0xA6, 0xA5, 0xA4,
        0xA3, 0xA2, 0xA1, 0xA0,
        0x7F, 0x7E, 0x7D, 0x7C,
        0x7B, 0x7A, 0x79, 0x78,
        0x77, 0x76, 0x75, 0x74,
        0x73, 0x72, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00,
        0x04, 0x05, 0x06, 0x07,
        0xAF, 0xAE, 0xAD, 0xAC,
        0xAB, 0xAA, 0xA9, 0xA8,
        0xA7, 0xA6, 0xA5, 0xA4,
        0xA3, 0xA2, 0xA1, 0xA0,
        0x7F, 0x7E, 0x7D, 0x7C,
        0x7B, 0x7A, 0x79, 0x78,
        0x77, 0x76, 0x75, 0x74,
        0x73, 0x72, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00,
        0x04, 0x05, 0x06, 0x07,
        0xAF, 0xAE, 0xAD, 0xAC,
        0xAB, 0xAA, 0xA9, 0xA8,
        0xA7, 0xA6, 0xA5, 0xA4,
        0xA3, 0xA2, 0xA1, 0xA0,
        0x7F, 0x7E, 0x7D, 0x7C,
        0x7B, 0x7A, 0x79, 0x78,
        0x77, 0x76, 0x75, 0x74,
        0x73, 0x72, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00,
        0x04, 0x05, 0x06, 0x07,
        0xAF, 0xAE, 0xAD, 0xAC,
        0xAB, 0xAA, 0xA9, 0xA8,
        0xA7, 0xA6, 0xA5, 0xA4,
        0xA3, 0xA2, 0xA1, 0xA0,
        0x7F, 0x7E, 0x7D, 0x7C,
        0x7B, 0x7A, 0x79, 0x78,
        0x77, 0x76, 0x75, 0x74,
        0x73, 0x72, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00,
        0x04, 0x05, 0x06, 0x07,
        0xAF, 0xAE, 0xAD, 0xAC,
        0xAB, 0xAA, 0xA9, 0xA8,
        0xA7, 0xA6, 0xA5, 0xA4,
    };

    response = mbim_message_new (buffer, sizeof (buffer));
    test_message_printable (response, 1, 0);

    g_assert (mbim_message_stk_pac_response_parse (
                  response,
                  &databuffer,
                  &error));
    g_assert_no_error (error);

    test_message_trace (databuffer,
                        sizeof (expected_databuffer),
                        expected_databuffer,
                        sizeof (expected_databuffer));
    g_assert (memcmp (databuffer, expected_databuffer, sizeof (expected_databuffer)) == 0);
}

static void
test_stk_terminal_response (void)
{
    const guint8 *databuffer;
    guint32 databuffer_len;
    guint32 status_words;
    g_autoptr(GError) error = NULL;
    g_autoptr(MbimMessage) response = NULL;

    const guint8 buffer [] =  {
        /* header */
        0x03, 0x00, 0x00, 0x80, /* type */
        0x48, 0x00, 0x00, 0x00, /* length */
        0x02, 0x00, 0x00, 0x00, /* transaction id */
        /* fragment header */
        0x01, 0x00, 0x00, 0x00, /* total */
        0x00, 0x00, 0x00, 0x00, /* current */
        /* indicate_status_message */
        0xD8, 0xF2, 0x01, 0x31, /* service id */
        0xFC, 0xB5, 0x4E, 0x17,
        0x86, 0x02, 0xD6, 0xED,
        0x38, 0x16, 0x16, 0x4C,
        0x02, 0x00, 0x00, 0x00, /* command id */
        0x00, 0x00, 0x00, 0x00, /* status code */
        0x18, 0x00, 0x00, 0x00, /* buffer length */
        /* information buffer */
        0x0C, 0x00, 0x00, 0x00, /* 0x00 ResultData offset */
        0x0C, 0x00, 0x00, 0x00, /* 0x04 ResultData length */
        0xCC, 0x00, 0x00, 0x00, /* 0x08 StatusWords */
        /* databuffer */
        0x00, 0x00, 0x00, 0x00, /* 0x0C ResultData */
        0x04, 0x05, 0x06, 0x07,
        0xAF, 0xAE, 0xAD, 0xAC
    };

    const guint8 expected_databuffer [] = {
        0x00, 0x00, 0x00, 0x00,
        0x04, 0x05, 0x06, 0x07,
        0xAF, 0xAE, 0xAD, 0xAC
    };

    response = mbim_message_new (buffer, sizeof (buffer));
    test_message_printable (response, 1, 0);

    g_assert (mbim_message_stk_terminal_response_response_parse (
                  response,
                  &databuffer_len,
                  &databuffer,
                  &status_words,
                  &error));
    g_assert_no_error (error);

    g_assert_cmpuint (status_words, ==, 204);

    test_message_trace (databuffer,
                        databuffer_len,
                        expected_databuffer,
                        sizeof (expected_databuffer));
    g_assert_cmpuint (databuffer_len, ==, sizeof (expected_databuffer));
    g_assert (memcmp (databuffer, expected_databuffer, sizeof (expected_databuffer)) == 0);
}

static void
test_stk_envelope_response (void)
{
    const guint8 *databuffer;
    g_autoptr(GError) error = NULL;
    g_autoptr(MbimMessage) response = NULL;

    const guint8 buffer [] =  {
        /* header */
        0x03, 0x00, 0x00, 0x80, /* type */
        0x50, 0x00, 0x00, 0x00, /* length */
        0x02, 0x00, 0x00, 0x00, /* transaction id */
        /* fragment header */
        0x01, 0x00, 0x00, 0x00, /* total */
        0x00, 0x00, 0x00, 0x00, /* current */
        /* indicate_status_message */
        0xD8, 0xF2, 0x01, 0x31, /* service id */
        0xFC, 0xB5, 0x4E, 0x17,
        0x86, 0x02, 0xD6, 0xED,
        0x38, 0x16, 0x16, 0x4C,
        0x03, 0x00, 0x00, 0x00, /* command id */
        0x00, 0x00, 0x00, 0x00, /* status code */
        0x20, 0x00, 0x00, 0x00, /* buffer length */
        /* information buffer */
        0x0C, 0x00, 0x00, 0x00,
        0x0C, 0x00, 0x00, 0x00,
        0xCC, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x0C, 0x00, 0x00, 0x00,
        0x0C, 0x00, 0x00, 0x00,
        0xCC, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00
    };

    const guint8 expected_databuffer [] = {
        0x0C, 0x00, 0x00, 0x00,
        0x0C, 0x00, 0x00, 0x00,
        0xCC, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x0C, 0x00, 0x00, 0x00,
        0x0C, 0x00, 0x00, 0x00,
        0xCC, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00
    };

    response = mbim_message_new (buffer, sizeof (buffer));
    test_message_printable (response, 1, 0);

    g_assert (mbim_message_stk_envelope_response_parse (
                  response,
                  &databuffer,
                  &error));
    g_assert_no_error (error);

    test_message_trace (databuffer,
                        sizeof (expected_databuffer),
                        expected_databuffer,
                        sizeof (expected_databuffer));
    g_assert (memcmp (databuffer, expected_databuffer, sizeof (expected_databuffer)) == 0);
}

static void
test_basic_connect_ip_packet_filters_none (void)
{
    guint32 n_filters;
    guint32 session_id;
    g_autoptr(MbimPacketFilterArray) filters = NULL;
    g_autoptr(GError) error = NULL;
    g_autoptr(MbimMessage) response = NULL;

    const guint8 buffer [] =  {
        /* header */
        0x03, 0x00, 0x00, 0x80, /* type */
        0x38, 0x00, 0x00, 0x00, /* length */
        0x02, 0x00, 0x00, 0x00, /* transaction id */
        /* fragment header */
        0x01, 0x00, 0x00, 0x00, /* total */
        0x00, 0x00, 0x00, 0x00, /* current */
        /* indicate_status_message */
        0xA2, 0x89, 0xCC, 0x33, /* service id */
        0xBC, 0xBB, 0x8B, 0x4F,
        0xB6, 0xB0, 0x13, 0x3E,
        0xC2, 0xAA, 0xE6, 0xDF,
        0x17, 0x00, 0x00, 0x00, /* command id */
        0x00, 0x00, 0x00, 0x00, /* status code */
        0x08, 0x00, 0x00, 0x00, /* buffer length */
        /* information buffer */
        0x01, 0x00, 0x00, 0x00, /* session id */
        0x00, 0x00, 0x00, 0x00  /* packet filters count */
    };

    response = mbim_message_new (buffer, sizeof (buffer));
    test_message_printable (response, 1, 0);

    g_assert (mbim_message_ip_packet_filters_response_parse (
                  response,
                  &session_id,
                  &n_filters,
                  &filters,
                  &error));
    g_assert_no_error (error);

    g_assert_cmpuint (session_id, ==, 1);
    g_assert_cmpuint (n_filters, ==, 0);
    g_assert (filters == NULL);
}

static void
test_basic_connect_ip_packet_filters_one (void)
{
    guint32 n_filters;
    guint32 session_id;
    g_autoptr(MbimPacketFilterArray) filters = NULL;
    g_autoptr(GError) error = NULL;
    g_autoptr(MbimMessage) response = NULL;

    const guint8 buffer [] =  {
        /* header */
        0x03, 0x00, 0x00, 0x80, /* type */
        0x5C, 0x00, 0x00, 0x00, /* length */
        0x02, 0x00, 0x00, 0x00, /* transaction id */
        /* fragment header */
        0x01, 0x00, 0x00, 0x00, /* total */
        0x00, 0x00, 0x00, 0x00, /* current */
        /* indicate_status_message */
        0xA2, 0x89, 0xCC, 0x33, /* service id */
        0xBC, 0xBB, 0x8B, 0x4F,
        0xB6, 0xB0, 0x13, 0x3E,
        0xC2, 0xAA, 0xE6, 0xDF,
        0x17, 0x00, 0x00, 0x00, /* command id */
        0x00, 0x00, 0x00, 0x00, /* status code */
        0x2C, 0x00, 0x00, 0x00, /* buffer length */
        /* information buffer */
        0x01, 0x00, 0x00, 0x00, /* 0x00 session id */
        0x01, 0x00, 0x00, 0x00, /* 0x04 packet filters count */
        0x10, 0x00, 0x00, 0x00, /* 0x08 packet filter 1 offset */
        0x1C, 0x00, 0x00, 0x00, /* 0x0C packet filter 1 length */
        /* databuffer, packet filter 1 */
        0x08, 0x00, 0x00, 0x00, /* 0x10 0x00 filter size */
        0x0C, 0x00, 0x00, 0x00, /* 0x14 0x04 filter offset (from beginning of struct) */
        0x14, 0x00, 0x00, 0x00, /* 0x18 0x08 mask offset (from beginning of struct) */
        0x01, 0x02, 0x03, 0x04, /* 0x1C 0x0C filter */
        0x05, 0x06, 0x07, 0x08,
        0xF1, 0xF2, 0xF3, 0xF4, /* 0x24 0x14 mask */
        0xF5, 0xF6, 0xF7, 0xF8,
    };

    const guint8 expected_filter[] = {
        0x01, 0x02, 0x03, 0x04,
        0x05, 0x06, 0x07, 0x08,
    };
    const guint8 expected_mask[] = {
        0xF1, 0xF2, 0xF3, 0xF4,
        0xF5, 0xF6, 0xF7, 0xF8,
    };

    response = mbim_message_new (buffer, sizeof (buffer));
    test_message_printable (response, 1, 0);

    g_assert (mbim_message_ip_packet_filters_response_parse (
                  response,
                  &session_id,
                  &n_filters,
                  &filters,
                  &error));
    g_assert_no_error (error);

    g_assert_cmpuint (session_id, ==, 1);
    g_assert_cmpuint (n_filters, ==, 1);
    g_assert (filters != NULL);

    g_assert_cmpuint (filters[0]->filter_size, ==, 8);

    test_message_trace (filters[0]->packet_filter, 8,
                        expected_filter,
                        sizeof (expected_filter));
    g_assert (memcmp (filters[0]->packet_filter, expected_filter, sizeof (expected_filter)) == 0);

    test_message_trace (filters[0]->packet_mask, 8,
                        expected_mask,
                        sizeof (expected_mask));
    g_assert (memcmp (filters[0]->packet_mask, expected_mask, sizeof (expected_mask)) == 0);
}

static void
test_basic_connect_ip_packet_filters_two (void)
{
    guint32 n_filters;
    guint32 session_id;
    g_autoptr(MbimPacketFilterArray) filters = NULL;
    g_autoptr(GError) error = NULL;
    g_autoptr(MbimMessage) response = NULL;

    const guint8 buffer [] =  {
        /* header */
        0x03, 0x00, 0x00, 0x80, /* type */
        0x88, 0x00, 0x00, 0x00, /* length */
        0x02, 0x00, 0x00, 0x00, /* transaction id */
        /* fragment header */
        0x01, 0x00, 0x00, 0x00, /* total */
        0x00, 0x00, 0x00, 0x00, /* current */
        /* indicate_status_message */
        0xA2, 0x89, 0xCC, 0x33, /* service id */
        0xBC, 0xBB, 0x8B, 0x4F,
        0xB6, 0xB0, 0x13, 0x3E,
        0xC2, 0xAA, 0xE6, 0xDF,
        0x17, 0x00, 0x00, 0x00, /* command id */
        0x00, 0x00, 0x00, 0x00, /* status code */
        0x58, 0x00, 0x00, 0x00, /* buffer length */
        /* information buffer */
        0x01, 0x00, 0x00, 0x00, /* 0x00 session id */
        0x02, 0x00, 0x00, 0x00, /* 0x04 packet filters count */
        0x18, 0x00, 0x00, 0x00, /* 0x08 packet filter 1 offset */
        0x1C, 0x00, 0x00, 0x00, /* 0x0C packet filter 1 length */
        0x34, 0x00, 0x00, 0x00, /* 0x10 packet filter 2 offset */
        0x24, 0x00, 0x00, 0x00, /* 0x14 packet filter 2 length */
        /* databuffer, packet filter 2 */
        0x08, 0x00, 0x00, 0x00, /* 0x18 0x00 filter size */
        0x0C, 0x00, 0x00, 0x00, /* 0x1C 0x04 filter offset (from beginning of struct) */
        0x14, 0x00, 0x00, 0x00, /* 0x20 0x08 mask offset (from beginning of struct) */
        0x01, 0x02, 0x03, 0x04, /* 0x24 0x0C filter */
        0x05, 0x06, 0x07, 0x08,
        0xF1, 0xF2, 0xF3, 0xF4, /* 0x2C 0x14 mask */
        0xF5, 0xF6, 0xF7, 0xF8,
        /* databuffer, packet filter 2 */
        0x0C, 0x00, 0x00, 0x00, /* 0x34 0x00 filter size */
        0x0C, 0x00, 0x00, 0x00, /* 0x38 0x04 filter offset (from beginning of struct) */
        0x18, 0x00, 0x00, 0x00, /* 0x3C 0x08 mask offset (from beginning of struct) */
        0x01, 0x02, 0x03, 0x04, /* 0x40 0x0C filter */
        0x05, 0x06, 0x07, 0x08,
        0x05, 0x06, 0x07, 0x08,
        0xF1, 0xF2, 0xF3, 0xF4, /* 0x4C 0x18 mask */
        0xF5, 0xF6, 0xF7, 0xF8,
        0xF5, 0xF6, 0xF7, 0xF8,
    };

    const guint8 expected_filter1[] = {
        0x01, 0x02, 0x03, 0x04,
        0x05, 0x06, 0x07, 0x08,
    };
    const guint8 expected_mask1[] = {
        0xF1, 0xF2, 0xF3, 0xF4,
        0xF5, 0xF6, 0xF7, 0xF8,
    };
    const guint8 expected_filter2[] = {
        0x01, 0x02, 0x03, 0x04,
        0x05, 0x06, 0x07, 0x08,
        0x05, 0x06, 0x07, 0x08,
    };
    const guint8 expected_mask2[] = {
        0xF1, 0xF2, 0xF3, 0xF4,
        0xF5, 0xF6, 0xF7, 0xF8,
        0xF5, 0xF6, 0xF7, 0xF8,
    };

    response = mbim_message_new (buffer, sizeof (buffer));
    test_message_printable (response, 1, 0);

    g_assert (mbim_message_ip_packet_filters_response_parse (
                  response,
                  &session_id,
                  &n_filters,
                  &filters,
                  &error));
    g_assert_no_error (error);

    g_assert_cmpuint (session_id, ==, 1);
    g_assert_cmpuint (n_filters, ==, 2);
    g_assert (filters != NULL);

    g_assert_cmpuint (filters[0]->filter_size, ==, 8);
    test_message_trace (filters[0]->packet_filter, 8,
                        expected_filter1,
                        sizeof (expected_filter1));
    g_assert (memcmp (filters[0]->packet_filter, expected_filter1, sizeof (expected_filter1)) == 0);
    test_message_trace (filters[0]->packet_mask, 8,
                        expected_mask1,
                        sizeof (expected_mask1));
    g_assert (memcmp (filters[0]->packet_mask, expected_mask1, sizeof (expected_mask1)) == 0);

    g_assert_cmpuint (filters[1]->filter_size, ==, 12);
    test_message_trace (filters[1]->packet_filter, 12,
                        expected_filter2,
                        sizeof (expected_filter2));
    g_assert (memcmp (filters[1]->packet_filter, expected_filter2, sizeof (expected_filter2)) == 0);
    test_message_trace (filters[1]->packet_mask, 12,
                        expected_mask2,
                        sizeof (expected_mask2));
    g_assert (memcmp (filters[1]->packet_mask, expected_mask2, sizeof (expected_mask2)) == 0);
}

static void
test_ms_firmware_id_get (void)
{
    const MbimUuid *firmware_id;
    g_autoptr(GError) error = NULL;
    g_autoptr(MbimMessage) response = NULL;

    const guint8 buffer [] = {
        /* header */
        0x03, 0x00, 0x00, 0x80, /* type */
        0x40, 0x00, 0x00, 0x00, /* length */
        0x02, 0x00, 0x00, 0x00, /* transaction id */
        /* fragment header */
        0x01, 0x00, 0x00, 0x00, /* total */
        0x00, 0x00, 0x00, 0x00, /* current */
        /* command_done_message */
        0xE9, 0xF7, 0xDE, 0xA2, /* service id */
        0xFE, 0xAF, 0x40, 0x09,
        0x93, 0xCE, 0x90, 0xA3,
        0x69, 0x41, 0x03, 0xB6,
        0x01, 0x00, 0x00, 0x00, /* command id */
        0x00, 0x00, 0x00, 0x00, /* status code */
        0x10, 0x00, 0x00, 0x00, /* buffer length */
        /* information buffer */
        0x00, 0x11, 0x22, 0x33, /* firmware id */
        0x44, 0x55, 0x66, 0x77,
        0x88, 0x99, 0xAA, 0xBB,
        0xCC, 0xDD, 0xEE, 0xFF  };

    const MbimUuid expected_firmware_id = {
        .a = { 0x00, 0x11, 0x22, 0x33 },
        .b = { 0x44, 0x55 },
        .c = { 0x66, 0x77 },
        .d = { 0x88, 0x99 },
        .e = { 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF }
    };

    response = mbim_message_new (buffer, sizeof (buffer));
    test_message_printable (response, 1, 0);

    g_assert (mbim_message_ms_firmware_id_get_response_parse (
                  response,
                  &firmware_id,
                  &error));

    g_assert_no_error (error);

    g_assert (mbim_uuid_cmp (firmware_id, &expected_firmware_id));
}

static void
test_basic_connect_connect_short (void)
{
    guint32 session_id;
    MbimActivationState activation_state;
    MbimVoiceCallState voice_call_state;
    MbimContextIpType ip_type;
    const MbimUuid *context_type;
    guint32 nw_error;
    g_autoptr(GError) error = NULL;
    g_autoptr(MbimMessage) response = NULL;

    const guint8 buffer [] = {
        /* header */
        0x03, 0x00, 0x00, 0x80, /* type */
        0x30, 0x00, 0x00, 0x00, /* length */
        0x1A, 0x0D, 0x00, 0x00, /* transaction id */
        /* fragment header */
        0x01, 0x00, 0x00, 0x00, /* total */
        0x00, 0x00, 0x00, 0x00, /* current */
        /* command_done_message */
        0xA2, 0x89, 0xCC, 0x33, /* service id */
        0xBC, 0xBB, 0x8B, 0x4F,
        0xB6, 0xB0, 0x13, 0x3E,
        0xC2, 0xAA, 0xE6, 0xDF, /* command id */
        0x0C, 0x00, 0x00, 0x00, /* status code */
        0x02, 0x00, 0x00, 0x00, /* buffer length */
        /* information buffer */
        0x00, 0x00, 0x00, 0x00
    };

    response = mbim_message_new (buffer, sizeof (buffer));

    /* should fail! */
    g_assert (!mbim_message_connect_response_parse (
                  response,
                  &session_id,
                  &activation_state,
                  &voice_call_state,
                  &ip_type,
                  &context_type,
                  &nw_error,
                  &error));

    g_assert (error != NULL);
}

static void
test_basic_connect_visible_providers_overflow (void)
{
    guint32 n_providers;
    g_autoptr(GError) error = NULL;
    g_autoptr(MbimProviderArray) providers = NULL;
    g_autoptr(MbimMessage) response = NULL;
    gboolean result;

    const guint8 buffer [] =  {
        /* header */
        0x03, 0x00, 0x00, 0x80, /* type */
        0xB4, 0x00, 0x00, 0x00, /* length */
        0x02, 0x00, 0x00, 0x00, /* transaction id */
        /* fragment header */
        0x01, 0x00, 0x00, 0x00, /* total */
        0x00, 0x00, 0x00, 0x00, /* current */
        /* command_done_message */
        0xA2, 0x89, 0xCC, 0x33, /* service id */
        0xBC, 0xBB, 0x8B, 0x4F,
        0xB6, 0xB0, 0x13, 0x3E,
        0xC2, 0xAA, 0xE6, 0xDF,
        0x08, 0x00, 0x00, 0x00, /* command id */
        0x00, 0x00, 0x00, 0x00, /* status code */
        0x84, 0x00, 0x00, 0x00, /* buffer length */
        /* information buffer */
        0x02, 0x00, 0x00, 0x00, /* 0x00 providers count */
        0x14, 0x00, 0x00, 0x00, /* 0x04 provider 0 offset */
        0x38, 0x00, 0x00, 0x00, /* 0x08 provider 0 length */
        0x4C, 0x00, 0x00, 0x00, /* 0x0C provider 1 offset */
        0x38, 0x00, 0x00, 0x00, /* 0x10 provider 1 length */
        /* data buffer... struct provider 0 */
        0x20, 0x00, 0x00, 0x80, /* 0x14 [0x00] id offset */     /* OFFSET WRONG (0x80 instead of 0x00) */
        0x0A, 0x00, 0x00, 0x80, /* 0x18 [0x04] id length */     /* LENGTH WRONG (0x80 instead of 0x00) */
        0x08, 0x00, 0x00, 0x00, /* 0x1C [0x08] state */
        0x2C, 0x00, 0x00, 0x00, /* 0x20 [0x0C] name offset */
        0x0C, 0x00, 0x00, 0x00, /* 0x24 [0x10] name length */
        0x01, 0x00, 0x00, 0x00, /* 0x28 [0x14] cellular class */
        0x0B, 0x00, 0x00, 0x00, /* 0x2C [0x18] rssi */
        0x00, 0x00, 0x00, 0x00, /* 0x30 [0x1C] error rate */
        0x32, 0x00, 0x31, 0x00, /* 0x34 [0x20] id string (10 bytes) */
        0x34, 0x00, 0x30, 0x00,
        0x33, 0x00, 0x00, 0x00,
        0x4F, 0x00, 0x72, 0x00, /* 0x40 [0x2C] name string (12 bytes) */
        0x61, 0x00, 0x6E, 0x00,
        0x67, 0x00, 0x65, 0x00,
        /* data buffer... struct provider 1 */
        0x20, 0x00, 0x00, 0x00, /* 0x4C [0x00] id offset */
        0x0A, 0x00, 0x00, 0x00, /* 0x50 [0x04] id length */
        0x19, 0x00, 0x00, 0x00, /* 0x51 [0x08] state */
        0x2C, 0x00, 0x00, 0x00, /* 0x54 [0x0C] name offset */
        0x0C, 0x00, 0x00, 0x00, /* 0x58 [0x10] name length */
        0x01, 0x00, 0x00, 0x00, /* 0x5C [0x14] cellular class */
        0x0B, 0x00, 0x00, 0x00, /* 0x60 [0x18] rssi */
        0x00, 0x00, 0x00, 0x00, /* 0x64 [0x1C] error rate */
        0x32, 0x00, 0x31, 0x00, /* 0x68 [0x20] id string (10 bytes) */
        0x34, 0x00, 0x30, 0x00,
        0x33, 0x00, 0x00, 0x00,
        0x4F, 0x00, 0x72, 0x00, /* 0x74 [0x2C] name string (12 bytes) */
        0x61, 0x00, 0x6E, 0x00,
        0x67, 0x00, 0x65, 0x00 };

    response = mbim_message_new (buffer, sizeof (buffer));

    result = mbim_message_visible_providers_response_parse (response,
                                                            &n_providers,
                                                            &providers,
                                                            &error);

    g_assert (error != NULL);
    g_assert (!result);
}

static void
test_ms_basic_connect_extensions_base_stations (void)
{
    g_autoptr(GError)                              error = NULL;
    g_autoptr(MbimMessage)                         response = NULL;
    gboolean                                       result;
    MbimDataClass                                  system_type;
	g_autoptr(MbimCellInfoServingGsm)              gsm_serving_cell = NULL;
    g_autoptr(MbimCellInfoServingUmts)             umts_serving_cell = NULL;
    g_autoptr(MbimCellInfoServingTdscdma)          tdscdma_serving_cell = NULL;
    g_autoptr(MbimCellInfoServingLte)              lte_serving_cell = NULL;
    guint32                                        gsm_neighboring_cells_count;
    g_autoptr(MbimCellInfoNeighboringGsmArray)     gsm_neighboring_cells = NULL;
    guint32                                        umts_neighboring_cells_count;
    g_autoptr(MbimCellInfoNeighboringUmtsArray)    umts_neighboring_cells = NULL;
    guint32                                        tdscdma_neighboring_cells_count;
    g_autoptr(MbimCellInfoNeighboringTdscdmaArray) tdscdma_neighboring_cells = NULL;
    guint32                                        lte_neighboring_cells_count;
    g_autoptr(MbimCellInfoNeighboringLteArray)     lte_neighboring_cells = NULL;
    guint32                                        cdma_cells_count;
    g_autoptr(MbimCellInfoCdmaArray)               cdma_cells = NULL;

    const guint8 buffer [] =  {
        /* header */
        0x03, 0x00, 0x00, 0x80, /* type */
        0xD8, 0x00, 0x00, 0x00, /* length */
        0x03, 0x00, 0x00, 0x00, /* transaction id */
        /* fragment header */
        0x01, 0x00, 0x00, 0x00, /* total */
        0x00, 0x00, 0x00, 0x00, /* current */
        /* command_done_message */
        0x3D, 0x01, 0xDC, 0xC5, /* service id */
        0xFE, 0xF5, 0x4D, 0x05,
        0x0D, 0x3A, 0xBE, 0xF7,
        0x05, 0x8E, 0x9A, 0xAF,
        0x0B, 0x00, 0x00, 0x00, /* command id */
        0x00, 0x00, 0x00, 0x00, /* status code */
        0xA8, 0x00, 0x00, 0x00, /* buffer length */
        /* information buffer */
        0x60, 0x00, 0x00, 0x00, /* system type */
        0x00, 0x00, 0x00, 0x00, /* gsm serving cell offset */
        0x00, 0x00, 0x00, 0x00, /* gsm serving cell size */
        0x00, 0x00, 0x00, 0x00, /* umts serving cell offset */
        0x00, 0x00, 0x00, 0x00, /* umts serving cell size */
        0x00, 0x00, 0x00, 0x00, /* tdscdma serving cell offset */
        0x00, 0x00, 0x00, 0x00, /* tdscdma serving cell size */
        0x4C, 0x00, 0x00, 0x00, /* lte serving cell offset*/
        0x2E, 0x00, 0x00, 0x00, /* lte serving cell size*/
        0xA0, 0x00, 0x00, 0x00, /* gsm network measurement report offset */
        0x04, 0x00, 0x00, 0x00, /* gsm network measurement report size */
        0xA4, 0x00, 0x00, 0x00, /* umts network measurement report offset */
        0x04, 0x00, 0x00, 0x00, /* umts network measurement report size */
        0x00, 0x00, 0x00, 0x00, /* tdscdma network measurement report offset */
        0x00, 0x00, 0x00, 0x00, /* tdscdma network measurement report size */
        0x7C, 0x00, 0x00, 0x00, /* lte network measurement report offset */
        0x24, 0x00, 0x00, 0x00, /* lte network measurement report size */
        0x00, 0x00, 0x00, 0x00, /* cdma network measurement report offset */
        0x00, 0x00, 0x00, 0x00, /* cdma network measurement report size */
        /* lte serving cell */
/*4C*/  0x24, 0x00, 0x00, 0x00, /* provider id offset */
        0x0A, 0x00, 0x00, 0x00, /* provider id size */
        0x1F, 0xCD, 0x65, 0x04, /* cell id */
        0x00, 0x19, 0x00, 0x00, /* earfcn */
        0x36, 0x01, 0x00, 0x00, /* physical cell id */
        0xFE, 0x6F, 0x00, 0x00, /* tac */
        0x99, 0xFF, 0xFF, 0xFF, /* rsrp */
        0xF4, 0xFF, 0xFF, 0xFF, /* rsrq */
        0xFF, 0xFF, 0xFF, 0xFF, /* timing advance */
        0x32, 0x00, 0x31, 0x00, /* provider id string */
        0x34, 0x00, 0x30, 0x00,
        0x37, 0x00, 0x00, 0x00,
        /* lte network measurement report */
/*7C*/  0x01, 0x00, 0x00, 0x00, /* element count */
        0x00, 0x00, 0x00, 0x00, /* provider id offset */
        0x00, 0x00, 0x00, 0x00, /* provider id size */
        0xFF, 0xFF, 0xFF, 0xFF, /* cell id */
        0xFF, 0xFF, 0xFF, 0xFF, /* earfcn */
        0x36, 0x01, 0x00, 0x00, /* physical cell id */
        0xFF, 0xFF, 0xFF, 0xFF, /* tac */
        0x99, 0xFF, 0xFF, 0xFF, /* rsrp */
        0xF4, 0xFF, 0xFF, 0xFF, /* rsrq */
        /* gsm network measurement report */
/*A0*/  0x00, 0x00, 0x00, 0x00,
        /* umts network measurement report */
/*A4*/  0x00, 0x00, 0x00, 0x00
    };

    response = mbim_message_new (buffer, sizeof (buffer));
    test_message_printable (response, 1, 0);

    result = (mbim_message_ms_basic_connect_extensions_base_stations_info_response_parse (
                  response,
                  &system_type,
                  &gsm_serving_cell,
                  &umts_serving_cell,
                  &tdscdma_serving_cell,
                  &lte_serving_cell,
                  &gsm_neighboring_cells_count,
                  &gsm_neighboring_cells,
                  &umts_neighboring_cells_count,
                  &umts_neighboring_cells,
                  &tdscdma_neighboring_cells_count,
                  &tdscdma_neighboring_cells,
                  &lte_neighboring_cells_count,
                  &lte_neighboring_cells,
                  &cdma_cells_count,
                  &cdma_cells,
                  &error));

    g_assert_no_error (error);
    g_assert (result);

    g_assert_null (gsm_serving_cell);
    g_assert_null (umts_serving_cell);
    g_assert_null (tdscdma_serving_cell);
    g_assert_nonnull (lte_serving_cell);
    g_assert_cmpuint (gsm_neighboring_cells_count, ==, 0);
    g_assert_null (gsm_neighboring_cells);
    g_assert_cmpuint (umts_neighboring_cells_count, ==, 0);
    g_assert_null (umts_neighboring_cells);
    g_assert_cmpuint (tdscdma_neighboring_cells_count, ==, 0);
    g_assert_null (tdscdma_neighboring_cells);
    g_assert_cmpuint (lte_neighboring_cells_count, ==, 1);
    g_assert_nonnull (lte_neighboring_cells);
    g_assert_cmpuint (cdma_cells_count, ==, 0);
    g_assert_null (cdma_cells);
}

static void
test_ms_basic_connect_extensions_registration_parameters_0_unnamed_tlvs (void)
{
    g_autoptr(GError)             error = NULL;
    g_autoptr(MbimMessage)        response = NULL;
    gboolean                      result;
    MbimMicoMode                  mico_mode;
    MbimDrxCycle                  drx_cycle;
    MbimLadnInfo                  ladn_info;
    MbimDefaultPduActivationHint  pdu_hint;
    gboolean                      re_register_if_needed;
    GList                        *unnamed_ies = NULL;

    const guint8 buffer [] =  {
        /* header */
        0x03, 0x00, 0x00, 0x80, /* type */
        0x44, 0x00, 0x00, 0x00, /* length */
        0x04, 0x00, 0x00, 0x00, /* transaction id */
        /* fragment header */
        0x01, 0x00, 0x00, 0x00, /* total */
        0x00, 0x00, 0x00, 0x00, /* current */
        /* command_done_message */
        0x3D, 0x01, 0xDC, 0xC5, /* service id */
        0xFE, 0xF5, 0x4D, 0x05,
        0x0D, 0x3A, 0xBE, 0xF7,
        0x05, 0x8E, 0x9A, 0xAF,
        0x11, 0x00, 0x00, 0x00, /* command id */
        0x00, 0x00, 0x00, 0x00, /* status code */
        0x14, 0x00, 0x00, 0x00, /* buffer length */
        /* information buffer */
        0x00, 0x00, 0x00, 0x00, /* mico mode */
        0x00, 0x00, 0x00, 0x00, /* drx cycle */
        0x00, 0x00, 0x00, 0x00, /* ladn info */
        0x01, 0x00, 0x00, 0x00, /* pdu hint */
        0x01, 0x00, 0x00, 0x00, /* re register if needed */
        /* no unnamed TLVs */
    };

    response = mbim_message_new (buffer, sizeof (buffer));
    test_message_printable (response, 3, 0);

    result = (mbim_message_ms_basic_connect_extensions_v3_registration_parameters_response_parse (
                  response,
                  &mico_mode,
                  &drx_cycle,
                  &ladn_info,
                  &pdu_hint,
                  &re_register_if_needed,
                  &unnamed_ies,
                  &error));

    g_assert_no_error (error);
    g_assert (result);

    g_assert_cmpuint (mico_mode, ==, MBIM_MICO_MODE_DISABLED);
    g_assert_cmpuint (drx_cycle, ==, MBIM_DRX_CYCLE_NOT_SPECIFIED);
    g_assert_cmpuint (ladn_info, ==, MBIM_LADN_INFO_NOT_NEEDED);
    g_assert_cmpuint (pdu_hint, ==, MBIM_DEFAULT_PDU_ACTIVATION_HINT_LIKELY);
    g_assert_cmpuint (re_register_if_needed, ==, TRUE);
    g_assert_cmpuint (g_list_length (unnamed_ies), ==, 0);
}

static void
test_ms_basic_connect_extensions_registration_parameters_1_unnamed_tlv (void)
{
    g_autoptr(GError)             error = NULL;
    g_autoptr(MbimMessage)        response = NULL;
    gboolean                      result;
    MbimMicoMode                  mico_mode;
    MbimDrxCycle                  drx_cycle;
    MbimLadnInfo                  ladn_info;
    MbimDefaultPduActivationHint  pdu_hint;
    gboolean                      re_register_if_needed;
    GList                        *unnamed_ies = NULL;
    MbimTlv                      *tlv;
    g_autofree gchar             *tlv_str = NULL;

    const guint8 buffer [] =  {
        /* header */
        0x03, 0x00, 0x00, 0x80, /* type */
        0x58, 0x00, 0x00, 0x00, /* length */
        0x04, 0x00, 0x00, 0x00, /* transaction id */
        /* fragment header */
        0x01, 0x00, 0x00, 0x00, /* total */
        0x00, 0x00, 0x00, 0x00, /* current */
        /* command_done_message */
        0x3D, 0x01, 0xDC, 0xC5, /* service id */
        0xFE, 0xF5, 0x4D, 0x05,
        0x0D, 0x3A, 0xBE, 0xF7,
        0x05, 0x8E, 0x9A, 0xAF,
        0x11, 0x00, 0x00, 0x00, /* command id */
        0x00, 0x00, 0x00, 0x00, /* status code */
        0x28, 0x00, 0x00, 0x00, /* buffer length */
        /* information buffer */
        0x00, 0x00, 0x00, 0x00, /* mico mode */
        0x00, 0x00, 0x00, 0x00, /* drx cycle */
        0x00, 0x00, 0x00, 0x00, /* ladn info */
        0x01, 0x00, 0x00, 0x00, /* pdu hint */
        0x01, 0x00, 0x00, 0x00, /* re register if needed */
        /* First unnamed TLV */
        0x0A, 0x00, 0x00, 0x00, /* TLV type MBIM_TLV_TYPE_WCHAR_STR, no padding */
        0x0C, 0x00, 0x00, 0x00, /* TLV data length */
        0x4F, 0x00, 0x72, 0x00, /* TLV data string */
        0x61, 0x00, 0x6E, 0x00,
        0x67, 0x00, 0x65, 0x00,
    };

    response = mbim_message_new (buffer, sizeof (buffer));
    test_message_printable (response, 3, 0);

    result = (mbim_message_ms_basic_connect_extensions_v3_registration_parameters_response_parse (
                  response,
                  &mico_mode,
                  &drx_cycle,
                  &ladn_info,
                  &pdu_hint,
                  &re_register_if_needed,
                  &unnamed_ies,
                  &error));

    g_assert_no_error (error);
    g_assert (result);

    g_assert_cmpuint (mico_mode, ==, MBIM_MICO_MODE_DISABLED);
    g_assert_cmpuint (drx_cycle, ==, MBIM_DRX_CYCLE_NOT_SPECIFIED);
    g_assert_cmpuint (ladn_info, ==, MBIM_LADN_INFO_NOT_NEEDED);
    g_assert_cmpuint (pdu_hint, ==, MBIM_DEFAULT_PDU_ACTIVATION_HINT_LIKELY);
    g_assert_cmpuint (re_register_if_needed, ==, TRUE);
    g_assert_cmpuint (g_list_length (unnamed_ies), ==, 1);

    tlv = (MbimTlv *)(unnamed_ies->data);
    g_assert_cmpuint (mbim_tlv_get_tlv_type (tlv), ==, MBIM_TLV_TYPE_WCHAR_STR);

    tlv_str = mbim_tlv_string_get (tlv, &error);
    g_assert_no_error (error);
    g_assert_cmpstr (tlv_str, ==, "Orange");

    g_list_free_full (unnamed_ies, (GDestroyNotify)mbim_tlv_unref);
}

static void
test_ms_basic_connect_extensions_registration_parameters_3_unnamed_tlvs (void)
{
    g_autoptr(GError)             error = NULL;
    g_autoptr(MbimMessage)        response = NULL;
    gboolean                      result;
    MbimMicoMode                  mico_mode;
    MbimDrxCycle                  drx_cycle;
    MbimLadnInfo                  ladn_info;
    MbimDefaultPduActivationHint  pdu_hint;
    gboolean                      re_register_if_needed;
    GList                        *unnamed_ies = NULL;
    GList                        *iter;
    MbimTlv                      *tlv;
    g_autofree gchar             *tlv_str_1 = NULL;
    const gchar                  *expected_tlv_str_1 = "abcde";
    g_autofree gchar             *tlv_str_2 = NULL;
    const gchar                  *expected_tlv_str_2 = "Orange";
    const guint8                 *pco_3 = NULL;
    guint32                       pco_3_size = 0;
    const guint8                  expected_pco[] = { 0x01, 0x02, 0x03, 0x04,
                                                     0x05, 0x06, 0x07, 0x08,
                                                     0x09, 0x0A, 0x0B };

    const guint8 buffer [] =  {
        /* header */
        0x03, 0x00, 0x00, 0x80, /* type */
        0x80, 0x00, 0x00, 0x00, /* length */
        0x04, 0x00, 0x00, 0x00, /* transaction id */
        /* fragment header */
        0x01, 0x00, 0x00, 0x00, /* total */
        0x00, 0x00, 0x00, 0x00, /* current */
        /* command_done_message */
        0x3D, 0x01, 0xDC, 0xC5, /* service id */
        0xFE, 0xF5, 0x4D, 0x05,
        0x0D, 0x3A, 0xBE, 0xF7,
        0x05, 0x8E, 0x9A, 0xAF,
        0x11, 0x00, 0x00, 0x00, /* command id */
        0x00, 0x00, 0x00, 0x00, /* status code */
        0x50, 0x00, 0x00, 0x00, /* buffer length */
        /* information buffer */
        0x00, 0x00, 0x00, 0x00, /* mico mode */
        0x00, 0x00, 0x00, 0x00, /* drx cycle */
        0x00, 0x00, 0x00, 0x00, /* ladn info */
        0x01, 0x00, 0x00, 0x00, /* pdu hint */
        0x01, 0x00, 0x00, 0x00, /* re register if needed */
        /* First unnamed TLV */
        0x0A, 0x00, 0x00, 0x02, /* TLV type MBIM_TLV_TYPE_WCHAR_STR, padding 2 */
        0x0A, 0x00, 0x00, 0x00, /* TLV data length */
        0x61, 0x00, 0x62, 0x00, /* TLV data string */
        0x63, 0x00, 0x64, 0x00,
        0x65, 0x00, 0x00, 0x00,
        /* Second unnamed TLV */
        0x0A, 0x00, 0x00, 0x00, /* TLV type MBIM_TLV_TYPE_WCHAR_STR, no padding */
        0x0C, 0x00, 0x00, 0x00, /* TLV data length */
        0x4F, 0x00, 0x72, 0x00, /* TLV data string */
        0x61, 0x00, 0x6E, 0x00,
        0x67, 0x00, 0x65, 0x00,
        /* Third unnamed TLV */
        0x0D, 0x00, 0x00, 0x01, /* TLV type MBIM_TLV_TYPE_PCO, padding 1 */
        0x0B, 0x00, 0x00, 0x00, /* TLV data length */
        0x01, 0x02, 0x03, 0x04, /* TLV data bytes */
        0x05, 0x06, 0x07, 0x08,
        0x09, 0x0A, 0x0B, 0x00,
    };

    response = mbim_message_new (buffer, sizeof (buffer));
    test_message_printable (response, 3, 0);

    result = (mbim_message_ms_basic_connect_extensions_v3_registration_parameters_response_parse (
                  response,
                  &mico_mode,
                  &drx_cycle,
                  &ladn_info,
                  &pdu_hint,
                  &re_register_if_needed,
                  &unnamed_ies,
                  &error));

    g_assert_no_error (error);
    g_assert (result);

    g_assert_cmpuint (mico_mode, ==, MBIM_MICO_MODE_DISABLED);
    g_assert_cmpuint (drx_cycle, ==, MBIM_DRX_CYCLE_NOT_SPECIFIED);
    g_assert_cmpuint (ladn_info, ==, MBIM_LADN_INFO_NOT_NEEDED);
    g_assert_cmpuint (pdu_hint, ==, MBIM_DEFAULT_PDU_ACTIVATION_HINT_LIKELY);
    g_assert_cmpuint (re_register_if_needed, ==, TRUE);
    g_assert_cmpuint (g_list_length (unnamed_ies), ==, 3);

    iter = unnamed_ies;
    tlv = (MbimTlv *)(iter->data);
    g_assert_cmpuint (mbim_tlv_get_tlv_type (tlv), ==, MBIM_TLV_TYPE_WCHAR_STR);
    tlv_str_1 = mbim_tlv_string_get (tlv, &error);
    g_assert_no_error (error);
    g_assert_cmpstr (tlv_str_1, ==, expected_tlv_str_1);

    iter = g_list_next (iter);
    tlv = (MbimTlv *)(iter->data);
    g_assert_cmpuint (mbim_tlv_get_tlv_type (tlv), ==, MBIM_TLV_TYPE_WCHAR_STR);
    tlv_str_2 = mbim_tlv_string_get (tlv, &error);
    g_assert_no_error (error);
    g_assert_cmpstr (tlv_str_2, ==, expected_tlv_str_2);

    iter = g_list_next (iter);
    tlv = (MbimTlv *)(iter->data);
    g_assert_cmpuint (mbim_tlv_get_tlv_type (tlv), ==, MBIM_TLV_TYPE_PCO);
    pco_3 = mbim_tlv_get_tlv_data (tlv, &pco_3_size);
    g_assert_cmpuint (pco_3_size, ==, sizeof (expected_pco));
    g_assert (memcmp (pco_3, expected_pco, sizeof (expected_pco)) == 0);

    g_list_free_full (unnamed_ies, (GDestroyNotify)mbim_tlv_unref);
}

static void
test_ms_basic_connect_v3_connect_0_unnamed_tlvs (void)
{
    g_autoptr(GError)       error = NULL;
    g_autoptr(MbimMessage)  response = NULL;
    gboolean                result;
    GList                  *unnamed_ies = NULL;
    guint32                 session_id;
    MbimActivationState     activation_state;
    MbimVoiceCallState      voice_call_state;
    MbimContextIpType       ip_type;
    MbimAccessMediaType     media_type = MBIM_ACCESS_MEDIA_TYPE_UNKNOWN;
    g_autofree gchar       *access_string = NULL;
    const MbimUuid         *context_type;
    guint32                 nw_error;

    const guint8 buffer [] =  {
        /* header */
        0x03, 0x00, 0x00, 0x80, /* type */
        0x6C, 0x00, 0x00, 0x00, /* length */
        0x04, 0x00, 0x00, 0x00, /* transaction id */
        /* fragment header */
        0x01, 0x00, 0x00, 0x00, /* total */
        0x00, 0x00, 0x00, 0x00, /* current */
        /* command_done_message */
        0xA2, 0x89, 0xCC, 0x33, /* service id */
        0xBC, 0xBB, 0x8B, 0x4F,
        0xB6, 0xB0, 0x13, 0x3E,
        0xC2, 0xAA, 0xE6, 0xDF,
        0x0C, 0x00, 0x00, 0x00, /* command id */
        0x00, 0x00, 0x00, 0x00, /* status code */
        0x3C, 0x00, 0x00, 0x00, /* buffer_length */
        /* information buffer */
        0x01, 0x00, 0x00, 0x00, /* session id */
        0x01, 0x00, 0x00, 0x00, /* activation state */
        0x00, 0x00, 0x00, 0x00, /* voice call state */
        0x01, 0x00, 0x00, 0x00, /* ip type */
        0x7E, 0x5E, 0x2A, 0x7E, /* context type */
        0x4E, 0x6F, 0x72, 0x72,
        0x73, 0x6B, 0x65, 0x6E,
        0x7E, 0x5E, 0x2A, 0x7E,
        0x00, 0x00, 0x00, 0x00, /* nw error */
        0x01, 0x00, 0x00, 0x00, /* media type */
        0x0A, 0x00, 0x00, 0x00, /* access string */
        0x10, 0x00, 0x00, 0x00,
        0x69, 0x00, 0x6E, 0x00,
        0x74, 0x00, 0x65, 0x00,
        0x72, 0x00, 0x6E, 0x00,
        0x65, 0x00, 0x74, 0x00,
        /* no unnamed TLVs */
    };

    response = mbim_message_new (buffer, sizeof (buffer));
    test_message_printable (response, 3, 0);

    result = (mbim_message_ms_basic_connect_v3_connect_response_parse (
                  response,
                  &session_id,
                  &activation_state,
                  &voice_call_state,
                  &ip_type,
                  &context_type,
                  &nw_error,
                  &media_type,
                  &access_string,
                  &unnamed_ies,
                  &error));

    g_assert_no_error (error);
    g_assert (result);

    g_assert_cmpuint (session_id, ==, 1);
    g_assert_cmpuint (activation_state, ==, MBIM_ACTIVATION_STATE_ACTIVATED);
    g_assert_cmpuint (voice_call_state, ==, MBIM_VOICE_CALL_STATE_NONE);
    g_assert_cmpuint (ip_type, ==, MBIM_CONTEXT_IP_TYPE_IPV4);
    g_assert_cmpuint (mbim_uuid_to_context_type (context_type), ==, MBIM_CONTEXT_TYPE_INTERNET);
    g_assert_cmpuint (media_type, ==, MBIM_ACCESS_MEDIA_TYPE_3GPP);
    g_assert_cmpstr  (access_string, ==, "internet");
    g_assert_cmpuint (g_list_length (unnamed_ies), ==, 0);
}

static void
test_ms_basic_connect_v3_connect_1_unnamed_tlv (void)
{
    g_autoptr(GError)       error = NULL;
    g_autoptr(MbimMessage)  response = NULL;
    gboolean                result;
    GList                  *unnamed_ies = NULL;
    guint32                 session_id;
    MbimActivationState     activation_state;
    MbimVoiceCallState      voice_call_state;
    MbimContextIpType       ip_type;
    MbimAccessMediaType     media_type = MBIM_ACCESS_MEDIA_TYPE_UNKNOWN;
    g_autofree gchar       *access_string = NULL;
    const MbimUuid         *context_type;
    guint32                 nw_error;
    MbimTlv                *tlv;
    g_autofree gchar       *tlv_str = NULL;

    const guint8 buffer [] =  {
        /* header */
        0x03, 0x00, 0x00, 0x80, /* type */
        0x82, 0x00, 0x00, 0x00, /* length */
        0x04, 0x00, 0x00, 0x00, /* transaction id */
        /* fragment header */
        0x01, 0x00, 0x00, 0x00, /* total */
        0x00, 0x00, 0x00, 0x00, /* current */
        /* command_done_message */
        0xA2, 0x89, 0xCC, 0x33, /* service id */
        0xBC, 0xBB, 0x8B, 0x4F,
        0xB6, 0xB0, 0x13, 0x3E,
        0xC2, 0xAA, 0xE6, 0xDF,
        0x0C, 0x00, 0x00, 0x00, /* command id */
        0x00, 0x00, 0x00, 0x00, /* status code */
        0x52, 0x00, 0x00, 0x00, /* buffer_length */
        /* information buffer */
        0x01, 0x00, 0x00, 0x00, /* session id */
        0x01, 0x00, 0x00, 0x00, /* activation state */
        0x00, 0x00, 0x00, 0x00, /* voice call state */
        0x01, 0x00, 0x00, 0x00, /* ip type */
        0x7E, 0x5E, 0x2A, 0x7E, /* context type */
        0x4E, 0x6F, 0x72, 0x72,
        0x73, 0x6B, 0x65, 0x6E,
        0x7E, 0x5E, 0x2A, 0x7E,
        0x00, 0x00, 0x00, 0x00, /* nw error */
        0x01, 0x00, 0x00, 0x00, /* media type */
        0x0A, 0x00, 0x00, 0x00, /* access string */
        0x10, 0x00, 0x00, 0x00,
        0x69, 0x00, 0x6E, 0x00,
        0x74, 0x00, 0x65, 0x00,
        0x72, 0x00, 0x6E, 0x00,
        0x65, 0x00, 0x74, 0x00,
        /* First unnamed TLV */
        0x0A, 0x00, 0x00, 0x00, /* TLV type MBIM_TLV_TYPE_WCHAR_STR, no padding */
        0x0C, 0x00, 0x00, 0x00, /* TLV data length */
        0x4F, 0x00, 0x72, 0x00, /* TLV data string */
        0x61, 0x00, 0x6E, 0x00,
        0x67, 0x00, 0x65, 0x00,
    };

    response = mbim_message_new (buffer, sizeof (buffer));
    test_message_printable (response, 3, 0);

    result = (mbim_message_ms_basic_connect_v3_connect_response_parse (
                  response,
                  &session_id,
                  &activation_state,
                  &voice_call_state,
                  &ip_type,
                  &context_type,
                  &nw_error,
                  &media_type,
                  &access_string,
                  &unnamed_ies,
                  &error));

    g_assert_no_error (error);
    g_assert (result);

    g_assert_cmpuint (session_id, ==, 1);
    g_assert_cmpuint (activation_state, ==, MBIM_ACTIVATION_STATE_ACTIVATED);
    g_assert_cmpuint (voice_call_state, ==, MBIM_VOICE_CALL_STATE_NONE);
    g_assert_cmpuint (ip_type, ==, MBIM_CONTEXT_IP_TYPE_IPV4);
    g_assert_cmpuint (mbim_uuid_to_context_type (context_type), ==, MBIM_CONTEXT_TYPE_INTERNET);
    g_assert_cmpuint (media_type, ==, MBIM_ACCESS_MEDIA_TYPE_3GPP);
    g_assert_cmpstr  (access_string, ==, "internet");
    g_assert_cmpuint (g_list_length (unnamed_ies), ==, 1);

    tlv = (MbimTlv *)(unnamed_ies->data);
    g_assert_cmpuint (mbim_tlv_get_tlv_type (tlv), ==, MBIM_TLV_TYPE_WCHAR_STR);

    tlv_str = mbim_tlv_string_get (tlv, &error);
    g_assert_no_error (error);
    g_assert_cmpstr (tlv_str, ==, "Orange");

    g_list_free_full (unnamed_ies, (GDestroyNotify)mbim_tlv_unref);
}

static void
test_ms_basic_connect_v3_connect_3_unnamed_tlvs (void)
{
    g_autoptr(GError)       error = NULL;
    g_autoptr(MbimMessage)  response = NULL;
    gboolean                result;
    GList                  *unnamed_ies = NULL;
    guint32                 session_id;
    MbimActivationState     activation_state;
    MbimVoiceCallState      voice_call_state;
    MbimContextIpType       ip_type;
    MbimAccessMediaType     media_type = MBIM_ACCESS_MEDIA_TYPE_UNKNOWN;
    g_autofree gchar       *access_string = NULL;
    const MbimUuid         *context_type;
    guint32                 nw_error;
    GList                  *iter;
    MbimTlv                *tlv;
    g_autofree gchar       *tlv_str_1 = NULL;
    const gchar            *expected_tlv_str_1 = "abcde";
    g_autofree gchar       *tlv_str_2 = NULL;
    const gchar            *expected_tlv_str_2 = "Orange";
    const guint8           *pco_3 = NULL;
    guint32                 pco_3_size = 0;
    const guint8            expected_pco[] = { 0x01, 0x02, 0x03, 0x04,
                                               0x05, 0x06, 0x07, 0x08,
                                               0x09, 0x0A, 0x0B };

    const guint8 buffer [] =  {
        /* header */
        0x03, 0x00, 0x00, 0x80, /* type */
        0xAA, 0x00, 0x00, 0x00, /* length */
        0x04, 0x00, 0x00, 0x00, /* transaction id */
        /* fragment header */
        0x01, 0x00, 0x00, 0x00, /* total */
        0x00, 0x00, 0x00, 0x00, /* current */
        /* command_done_message */
        0xA2, 0x89, 0xCC, 0x33, /* service id */
        0xBC, 0xBB, 0x8B, 0x4F,
        0xB6, 0xB0, 0x13, 0x3E,
        0xC2, 0xAA, 0xE6, 0xDF,
        0x0C, 0x00, 0x00, 0x00, /* command id */
        0x00, 0x00, 0x00, 0x00, /* status code */
        0x7A, 0x00, 0x00, 0x00, /* buffer_length */
        /* information buffer */
        0x01, 0x00, 0x00, 0x00, /* session id */
        0x01, 0x00, 0x00, 0x00, /* activation state */
        0x00, 0x00, 0x00, 0x00, /* voice call state */
        0x01, 0x00, 0x00, 0x00, /* ip type */
        0x7E, 0x5E, 0x2A, 0x7E, /* context type */
        0x4E, 0x6F, 0x72, 0x72,
        0x73, 0x6B, 0x65, 0x6E,
        0x7E, 0x5E, 0x2A, 0x7E,
        0x00, 0x00, 0x00, 0x00, /* nw error */
        0x01, 0x00, 0x00, 0x00, /* media type */
        0x0A, 0x00, 0x00, 0x00, /* access string */
        0x10, 0x00, 0x00, 0x00,
        0x69, 0x00, 0x6E, 0x00,
        0x74, 0x00, 0x65, 0x00,
        0x72, 0x00, 0x6E, 0x00,
        0x65, 0x00, 0x74, 0x00,
        /* First unnamed TLV */
        0x0A, 0x00, 0x00, 0x02, /* TLV type MBIM_TLV_TYPE_WCHAR_STR, padding 2 */
        0x0A, 0x00, 0x00, 0x00, /* TLV data length */
        0x61, 0x00, 0x62, 0x00, /* TLV data string */
        0x63, 0x00, 0x64, 0x00,
        0x65, 0x00, 0x00, 0x00,
        /* Second unnamed TLV */
        0x0A, 0x00, 0x00, 0x00, /* TLV type MBIM_TLV_TYPE_WCHAR_STR, no padding */
        0x0C, 0x00, 0x00, 0x00, /* TLV data length */
        0x4F, 0x00, 0x72, 0x00, /* TLV data string */
        0x61, 0x00, 0x6E, 0x00,
        0x67, 0x00, 0x65, 0x00,
        /* Third unnamed TLV */
        0x0D, 0x00, 0x00, 0x01, /* TLV type MBIM_TLV_TYPE_PCO, padding 1 */
        0x0B, 0x00, 0x00, 0x00, /* TLV data length */
        0x01, 0x02, 0x03, 0x04, /* TLV data bytes */
        0x05, 0x06, 0x07, 0x08,
        0x09, 0x0A, 0x0B, 0x00,
    };

    response = mbim_message_new (buffer, sizeof (buffer));
    test_message_printable (response, 3, 0);

    result = (mbim_message_ms_basic_connect_v3_connect_response_parse (
                  response,
                  &session_id,
                  &activation_state,
                  &voice_call_state,
                  &ip_type,
                  &context_type,
                  &nw_error,
                  &media_type,
                  &access_string,
                  &unnamed_ies,
                  &error));

    g_assert_no_error (error);
    g_assert (result);

    g_assert_cmpuint (session_id, ==, 1);
    g_assert_cmpuint (activation_state, ==, MBIM_ACTIVATION_STATE_ACTIVATED);
    g_assert_cmpuint (voice_call_state, ==, MBIM_VOICE_CALL_STATE_NONE);
    g_assert_cmpuint (ip_type, ==, MBIM_CONTEXT_IP_TYPE_IPV4);
    g_assert_cmpuint (mbim_uuid_to_context_type (context_type), ==, MBIM_CONTEXT_TYPE_INTERNET);
    g_assert_cmpuint (media_type, ==, MBIM_ACCESS_MEDIA_TYPE_3GPP);
    g_assert_cmpstr  (access_string, ==, "internet");
    g_assert_cmpuint (g_list_length (unnamed_ies), ==, 3);


    iter = unnamed_ies;
    tlv = (MbimTlv *)(iter->data);
    g_assert_cmpuint (mbim_tlv_get_tlv_type (tlv), ==, MBIM_TLV_TYPE_WCHAR_STR);
    tlv_str_1 = mbim_tlv_string_get (tlv, &error);
    g_assert_no_error (error);
    g_assert_cmpstr (tlv_str_1, ==, expected_tlv_str_1);

    iter = g_list_next (iter);
    tlv = (MbimTlv *)(iter->data);
    g_assert_cmpuint (mbim_tlv_get_tlv_type (tlv), ==, MBIM_TLV_TYPE_WCHAR_STR);
    tlv_str_2 = mbim_tlv_string_get (tlv, &error);
    g_assert_no_error (error);
    g_assert_cmpstr (tlv_str_2, ==, expected_tlv_str_2);

    iter = g_list_next (iter);
    tlv = (MbimTlv *)(iter->data);
    g_assert_cmpuint (mbim_tlv_get_tlv_type (tlv), ==, MBIM_TLV_TYPE_PCO);
    pco_3 = mbim_tlv_get_tlv_data (tlv, &pco_3_size);
    g_assert_cmpuint (pco_3_size, ==, sizeof (expected_pco));
    g_assert (memcmp (pco_3, expected_pco, sizeof (expected_pco)) == 0);

    g_list_free_full (unnamed_ies, (GDestroyNotify)mbim_tlv_unref);
}

static void
test_ms_basic_connect_extensions_device_caps_v3 (void)
{
    g_autoptr(GError)       error = NULL;
    g_autoptr(MbimMessage)  response = NULL;
    gboolean                result;
    MbimDeviceType          device_type;
    MbimVoiceClass          voice_class;
    MbimCellularClass       cellular_class;
    MbimSimClass            sim_class;
    MbimDataClassV3         data_class;
    MbimDataSubclass        data_subclass;
    MbimSmsCaps             sms_caps;
    MbimCtrlCaps            ctrl_caps;
    guint32                 max_sessions;
    guint32                 wcdma_band_class = 0;
    guint32                 lte_band_class_array_size = 0;
    g_autofree guint16     *lte_band_class_array = NULL;
    guint32                 nr_band_class_array_size = 0;
    g_autofree guint16     *nr_band_class_array = NULL;
    g_autofree gchar       *custom_data_class = NULL;
    g_autofree gchar       *device_id = NULL;
    g_autofree gchar       *firmware_info = NULL;
    g_autofree gchar       *hardware_info = NULL;
    guint32                 executor_index;
    static const guint16    expected_lte_band_class_array[] = {
        1, 2, 3, 4, 5, 7, 8, 12, 13, 14, 17, 18, 19, 20, 25, 26, 28, 29, 30, 32, 34, 38, 39, 40, 41, 42, 43, 46, 48
    };
    static const guint16    expected_nr_band_class_array[] = {
        1, 2, 3, 5, 7, 8, 20, 25, 28, 30, 38, 40, 41, 48, 66, 71, 77, 78, 79
    };

    const guint8 buffer [] =  {
        0x03, 0x00, 0x00, 0x80, 0x68, 0x01, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x3D, 0x01, 0xDC, 0xC5, 0xFE, 0xF5, 0x4D, 0x05, 0x0D, 0x3A, 0xBE, 0xF7,
        0x05, 0x8E, 0x9A, 0xAF, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x01, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
        0x7C, 0x00, 0x00, 0x80, 0x03, 0x00, 0x00, 0x00, 0xA3, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x9B, 0x00, 0x00, 0x00,
        0x0B, 0x00, 0x00, 0x02, 0x3A, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x03, 0x00, 0x04, 0x00,
        0x05, 0x00, 0x07, 0x00, 0x08, 0x00, 0x0C, 0x00, 0x0D, 0x00, 0x0E, 0x00, 0x11, 0x00, 0x12, 0x00,
        0x13, 0x00, 0x14, 0x00, 0x19, 0x00, 0x1A, 0x00, 0x1C, 0x00, 0x1D, 0x00, 0x1E, 0x00, 0x20, 0x00,
        0x22, 0x00, 0x26, 0x00, 0x27, 0x00, 0x28, 0x00, 0x29, 0x00, 0x2A, 0x00, 0x2B, 0x00, 0x2E, 0x00,
        0x30, 0x00, 0x00, 0x00, 0x0B, 0x00, 0x00, 0x02, 0x26, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02, 0x00,
        0x03, 0x00, 0x05, 0x00, 0x07, 0x00, 0x08, 0x00, 0x14, 0x00, 0x19, 0x00, 0x1C, 0x00, 0x1E, 0x00,
        0x26, 0x00, 0x28, 0x00, 0x29, 0x00, 0x30, 0x00, 0x42, 0x00, 0x47, 0x00, 0x4D, 0x00, 0x4E, 0x00,
        0x4F, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x00, 0x02, 0x0A, 0x00, 0x00, 0x00, 0x48, 0x00, 0x53, 0x00,
        0x50, 0x00, 0x41, 0x00, 0x2B, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x00, 0x02, 0x1E, 0x00, 0x00, 0x00,
        0x38, 0x00, 0x36, 0x00, 0x32, 0x00, 0x31, 0x00, 0x34, 0x00, 0x36, 0x00, 0x30, 0x00, 0x35, 0x00,
        0x30, 0x00, 0x30, 0x00, 0x38, 0x00, 0x34, 0x00, 0x35, 0x00, 0x35, 0x00, 0x35, 0x00, 0x00, 0x00,
        0x0A, 0x00, 0x00, 0x00, 0x3C, 0x00, 0x00, 0x00, 0x38, 0x00, 0x31, 0x00, 0x36, 0x00, 0x30, 0x00,
        0x30, 0x00, 0x2E, 0x00, 0x30, 0x00, 0x30, 0x00, 0x30, 0x00, 0x30, 0x00, 0x2E, 0x00, 0x39, 0x00,
        0x39, 0x00, 0x2E, 0x00, 0x32, 0x00, 0x39, 0x00, 0x2E, 0x00, 0x31, 0x00, 0x37, 0x00, 0x2E, 0x00,
        0x31, 0x00, 0x39, 0x00, 0x5F, 0x00, 0x47, 0x00, 0x43, 0x00, 0x0D, 0x00, 0x0A, 0x00, 0x42, 0x00,
        0x39, 0x00, 0x30, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x56, 0x00, 0x31, 0x00,
        0x2E, 0x00, 0x30, 0x00, 0x2E, 0x00, 0x36, 0x00
    };

    response = mbim_message_new (buffer, sizeof (buffer));
    test_message_printable (response, 3, 0);

    result = (mbim_message_ms_basic_connect_extensions_v3_device_caps_response_parse (
                response,
                &device_type,
                &cellular_class,
                &voice_class,
                &sim_class,
                &data_class,
                &sms_caps,
                &ctrl_caps,
                &data_subclass,
                &max_sessions,
                &executor_index,
                &wcdma_band_class,
                &lte_band_class_array_size,
                &lte_band_class_array,
                &nr_band_class_array_size,
                &nr_band_class_array,
                &custom_data_class,
                &device_id,
                &firmware_info,
                &hardware_info,
                &error));

    g_assert_no_error (error);
    g_assert (result);

    g_assert_cmpuint (device_type, ==, MBIM_DEVICE_TYPE_EMBEDDED);
    g_assert_cmpuint (cellular_class, ==, MBIM_CELLULAR_CLASS_GSM);
    g_assert_cmpuint (voice_class, ==, MBIM_VOICE_CLASS_NO_VOICE);
    g_assert_cmpuint (sim_class, ==, MBIM_SIM_CLASS_REMOVABLE);
    g_assert_cmpuint (data_class, ==, (MBIM_DATA_CLASS_V3_UMTS |
                                       MBIM_DATA_CLASS_V3_HSDPA |
                                       MBIM_DATA_CLASS_V3_HSUPA |
                                       MBIM_DATA_CLASS_V3_LTE |
                                       MBIM_DATA_CLASS_V3_5G |
                                       MBIM_DATA_CLASS_V3_CUSTOM));
    g_assert_cmpuint (sms_caps, ==, (MBIM_SMS_CAPS_PDU_RECEIVE |
                                     MBIM_SMS_CAPS_PDU_SEND));
    g_assert_cmpuint (ctrl_caps, ==, (MBIM_CTRL_CAPS_REG_MANUAL |
                                      MBIM_CTRL_CAPS_HW_RADIO_SWITCH |
                                      MBIM_CTRL_CAPS_ESIM |
                                      MBIM_CTRL_CAPS_SIM_HOT_SWAP_CAPABLE));
    g_assert_cmpuint (data_subclass, ==, (MBIM_DATA_SUBCLASS_5G_ENDC |
                                          MBIM_DATA_SUBCLASS_5G_NR));
    g_assert_cmpuint (max_sessions, ==, 2);
    g_assert_cmpuint (executor_index, ==, 0);
    g_assert_cmpuint (wcdma_band_class, ==, (1 << (1 - 1) |
                                             1 << (2 - 1) |
                                             1 << (4 - 1) |
                                             1 << (5 - 1) |
                                             1 << (8 - 1)));
    g_assert_cmpuint (G_N_ELEMENTS (expected_lte_band_class_array), ==, lte_band_class_array_size);
    g_assert (memcmp (lte_band_class_array, expected_lte_band_class_array, lte_band_class_array_size * sizeof (guint16)) == 0);
    g_assert_cmpuint (G_N_ELEMENTS (expected_nr_band_class_array), ==, nr_band_class_array_size);
    g_assert (memcmp (nr_band_class_array, expected_nr_band_class_array, nr_band_class_array_size * sizeof (guint16)) == 0);
    g_assert_cmpstr (custom_data_class, ==, "HSPA+");
    g_assert_cmpstr (device_id, ==, "862146050084555");
    g_assert_cmpstr (firmware_info, ==, "81600.0000.99.29.17.19_GC\r\nB90");
    g_assert_cmpstr (hardware_info, ==, "V1.0.6");
}

static void
test_ms_basic_connect_extensions_wake_reason_command (void)
{
    g_autoptr(GError)       error = NULL;
    g_autoptr(MbimMessage)  response = NULL;
    gboolean                result;
    MbimWakeType            wake_type;
    guint32                 session_id;
    g_autoptr(MbimTlv)      wake_tlv = NULL;
    const MbimUuid         *service = NULL;
    guint32                 cid = 0;
    guint32                 payload_size = 0;
    g_autofree guint8      *payload = NULL;

    const guint8 buffer [] =  {
        /* header */
        0x03, 0x00, 0x00, 0x80, /* type */
        0x5C, 0x00, 0x00, 0x00, /* length */
        0x04, 0x00, 0x00, 0x00, /* transaction id */
        /* fragment header */
        0x01, 0x00, 0x00, 0x00, /* total */
        0x00, 0x00, 0x00, 0x00, /* current */
        /* command_done_message */
        0x3D, 0x01, 0xDC, 0xC5, /* service id */
        0xFE, 0xF5, 0x4D, 0x05,
        0x0D, 0x3A, 0xBE, 0xF7,
        0x05, 0x8E, 0x9A, 0xAF,
        0x13, 0x00, 0x00, 0x00, /* command id */
        0x00, 0x00, 0x00, 0x00, /* status code */
        0x2C, 0x00, 0x00, 0x00, /* buffer_length */
        /* information buffer */
        0x01, 0x00, 0x00, 0x00, /* wake type: cid indication */
        0x02, 0x00, 0x00, 0x00, /* session id */
        /* TLV */
        0x10, 0x00, 0x00, 0x00, /* TLV type MBIM_TLV_TYPE_WAKE_COMMAND, padding 0 */
        0x1C, 0x00, 0x00, 0x00, /* TLV data length */
        0xA2, 0x89, 0xCC, 0x33, /* service id: basic connect */
        0xBC, 0xBB, 0x8B, 0x4F,
        0xB6, 0xB0, 0x13, 0x3E,
        0xC2, 0xAA, 0xE6, 0xDF,
        0x0B, 0x00, 0x00, 0x00, /* command id: signal state */
        0x00, 0x00, 0x00, 0x00, /* payload offset: none */
        0x00, 0x00, 0x00, 0x00, /* payload size: none */
    };

    response = mbim_message_new (buffer, sizeof (buffer));
    test_message_printable (response, 3, 0);

    result = (mbim_message_ms_basic_connect_extensions_v3_wake_reason_response_parse (
                  response,
                  &wake_type,
                  &session_id,
                  &wake_tlv,
                  &error));
    g_assert_no_error (error);
    g_assert (result);

    g_assert_cmpuint (wake_type, ==, MBIM_WAKE_TYPE_CID_INDICATION);
    g_assert_cmpuint (session_id, ==, 2);
    g_assert_nonnull (wake_tlv);
    g_assert_cmpuint (mbim_tlv_get_tlv_type (wake_tlv), ==, MBIM_TLV_TYPE_WAKE_COMMAND);

    result = (mbim_tlv_wake_command_get (wake_tlv,
                                         &service,
                                         &cid,
                                         &payload_size,
                                         &payload,
                                         &error));
    g_assert_no_error (error);
    g_assert (result);

    g_assert_cmpuint (mbim_uuid_to_service (service), ==, MBIM_SERVICE_BASIC_CONNECT);
    g_assert_cmpuint (cid, ==, MBIM_CID_BASIC_CONNECT_SIGNAL_STATE);
    g_assert_cmpuint (payload_size, ==, 0);
    g_assert_null (payload);
}

static void
test_ms_basic_connect_extensions_wake_reason_command_payload (void)
{
    g_autoptr(GError)       error = NULL;
    g_autoptr(MbimMessage)  response = NULL;
    gboolean                result;
    MbimWakeType            wake_type;
    guint32                 session_id;
    g_autoptr(MbimTlv)      wake_tlv = NULL;
    const MbimUuid         *service = NULL;
    guint32                 cid = 0;
    guint32                 payload_size = 0;
    g_autofree guint8      *payload = NULL;
    guint32                 payload_uint;

    const guint8 buffer [] =  {
        /* header */
        0x03, 0x00, 0x00, 0x80, /* type */
        0x60, 0x00, 0x00, 0x00, /* length */
        0x04, 0x00, 0x00, 0x00, /* transaction id */
        /* fragment header */
        0x01, 0x00, 0x00, 0x00, /* total */
        0x00, 0x00, 0x00, 0x00, /* current */
        /* command_done_message */
        0x3D, 0x01, 0xDC, 0xC5, /* service id */
        0xFE, 0xF5, 0x4D, 0x05,
        0x0D, 0x3A, 0xBE, 0xF7,
        0x05, 0x8E, 0x9A, 0xAF,
        0x13, 0x00, 0x00, 0x00, /* command id */
        0x00, 0x00, 0x00, 0x00, /* status code */
        0x30, 0x00, 0x00, 0x00, /* buffer_length */
        /* information buffer */
        0x00, 0x00, 0x00, 0x00, /* wake type: cid response */
        0x02, 0x00, 0x00, 0x00, /* session id */
        /* TLV */
        0x10, 0x00, 0x00, 0x00, /* TLV type MBIM_TLV_TYPE_WAKE_COMMAND, padding 0 */
        0x20, 0x00, 0x00, 0x00, /* TLV data length */
        0xA2, 0x89, 0xCC, 0x33, /* service id: basic connect */
        0xBC, 0xBB, 0x8B, 0x4F,
        0xB6, 0xB0, 0x13, 0x3E,
        0xC2, 0xAA, 0xE6, 0xDF,
        0x0C, 0x00, 0x00, 0x00, /* command id: connect */
        0x1C, 0x00, 0x00, 0x00, /* payload offset: 28 */
        0x04, 0x00, 0x00, 0x00, /* payload size: 4 */
        0x01, 0x00, 0x00, 0x00, /* payload: a guint32 */
    };

    response = mbim_message_new (buffer, sizeof (buffer));
    test_message_printable (response, 3, 0);

    result = (mbim_message_ms_basic_connect_extensions_v3_wake_reason_response_parse (
                  response,
                  &wake_type,
                  &session_id,
                  &wake_tlv,
                  &error));
    g_assert_no_error (error);
    g_assert (result);

    g_assert_cmpuint (wake_type, ==, MBIM_WAKE_TYPE_CID_RESPONSE);
    g_assert_cmpuint (session_id, ==, 2);
    g_assert_nonnull (wake_tlv);
    g_assert_cmpuint (mbim_tlv_get_tlv_type (wake_tlv), ==, MBIM_TLV_TYPE_WAKE_COMMAND);

    result = (mbim_tlv_wake_command_get (wake_tlv,
                                         &service,
                                         &cid,
                                         &payload_size,
                                         &payload,
                                         &error));
    g_assert_no_error (error);
    g_assert (result);

    g_assert_cmpuint (mbim_uuid_to_service (service), ==, MBIM_SERVICE_BASIC_CONNECT);
    g_assert_cmpuint (cid, ==, MBIM_CID_BASIC_CONNECT_CONNECT);
    g_assert_cmpuint (payload_size, ==, 4);
    g_assert_nonnull (payload);

    memcpy (&payload_uint, payload, payload_size);
    payload_uint = GUINT32_FROM_LE (payload_uint);
    g_assert_cmpuint (payload_uint, ==, 1);
}

static void
test_ms_basic_connect_extensions_wake_reason_packet (void)
{
    g_autoptr(GError)       error = NULL;
    g_autoptr(MbimMessage)  response = NULL;
    gboolean                result;
    MbimWakeType            wake_type;
    guint32                 session_id;
    g_autoptr(MbimTlv)      wake_tlv = NULL;
    guint32                 filter_id = 0;
    guint32                 original_packet_size = 0;
    guint32                 packet_size = 0;
    g_autofree guint8      *packet = NULL;
    const guint8            expected_packet[] = { 0x01, 0x02, 0x03, 0x04,
                                                  0x05, 0x06, 0x07, 0x08,
                                                  0x09, 0x0A };

    const guint8 buffer [] =  {
        /* header */
        0x03, 0x00, 0x00, 0x80, /* type */
        0x5C, 0x00, 0x00, 0x00, /* length */
        0x04, 0x00, 0x00, 0x00, /* transaction id */
        /* fragment header */
        0x01, 0x00, 0x00, 0x00, /* total */
        0x00, 0x00, 0x00, 0x00, /* current */
        /* command_done_message */
        0x3D, 0x01, 0xDC, 0xC5, /* service id */
        0xFE, 0xF5, 0x4D, 0x05,
        0x0D, 0x3A, 0xBE, 0xF7,
        0x05, 0x8E, 0x9A, 0xAF,
        0x13, 0x00, 0x00, 0x00, /* command id */
        0x00, 0x00, 0x00, 0x00, /* status code */
        0x2C, 0x00, 0x00, 0x00, /* buffer_length */
        /* information buffer */
        0x02, 0x00, 0x00, 0x00, /* wake type: packet */
        0x02, 0x00, 0x00, 0x00, /* session id */
        /* TLV */
        0x11, 0x00, 0x00, 0x02, /* TLV type MBIM_TLV_TYPE_WAKE_PACKET, padding 2 */
        0x1A, 0x00, 0x00, 0x00, /* TLV data length */
        0x0B, 0x00, 0x00, 0x00, /* filter id */
        0x0C, 0x00, 0x00, 0x00, /* original packet size: 12 */
        0x10, 0x00, 0x00, 0x00, /* packet offset: 16 */
        0x0A, 0x00, 0x00, 0x00, /* packet size: 10 */
        0x01, 0x02, 0x03, 0x04,
        0x05, 0x06, 0x07, 0x08,
        0x09, 0x0A, 0x00, 0x00, /* last 2 bytes padding */
    };

    response = mbim_message_new (buffer, sizeof (buffer));
    test_message_printable (response, 3, 0);

    result = (mbim_message_ms_basic_connect_extensions_v3_wake_reason_response_parse (
                  response,
                  &wake_type,
                  &session_id,
                  &wake_tlv,
                  &error));
    g_assert_no_error (error);
    g_assert (result);

    g_assert_cmpuint (wake_type, ==, MBIM_WAKE_TYPE_PACKET);
    g_assert_cmpuint (session_id, ==, 2);
    g_assert_nonnull (wake_tlv);
    g_assert_cmpuint (mbim_tlv_get_tlv_type (wake_tlv), ==, MBIM_TLV_TYPE_WAKE_PACKET);

    result = (mbim_tlv_wake_packet_get (wake_tlv,
                                        &filter_id,
                                        &original_packet_size,
                                        &packet_size,
                                        &packet,
                                        &error));
    g_assert_no_error (error);
    g_assert (result);

    g_assert_cmpuint (filter_id, ==, 0x0B);
    g_assert_cmpuint (original_packet_size, ==, 12);
    g_assert_cmpuint (packet_size, ==, sizeof (expected_packet));
    g_assert_nonnull (packet);
    g_assert (memcmp (packet, expected_packet, sizeof (expected_packet)) == 0);
}

static void
test_ms_uicc_low_level_access_application_list (void)
{
    g_autoptr(GError)                   error = NULL;
    g_autoptr(MbimMessage)              response = NULL;
    gboolean                            result;
    guint32                             version;
    guint32                             application_count;
    guint32                             active_application_index;
    guint32                             application_list_size_bytes;
    g_autoptr(MbimUiccApplicationArray) applications = NULL;

    const guint8  expected_application_id[] = { 0xA0, 0x00, 0x00, 0x00,
                                                0x87, 0x10, 0x02, 0xFF,
                                                0x34, 0xFF, 0x07, 0x89,
                                                0x31, 0x2E, 0x30, 0xFF };
    const gchar  *expected_application_name = "Movistar";
    const guint8  expected_pin_key_references[] = { 0x01, 0x81 };

    const guint8 buffer [] =  {
        /* header */
        0x03, 0x00, 0x00, 0x80, /* type */
        0x84, 0x00, 0x00, 0x00, /* length */
        0x03, 0x00, 0x00, 0x00, /* transaction id */
        /* fragment header */
        0x01, 0x00, 0x00, 0x00, /* total */
        0x00, 0x00, 0x00, 0x00, /* current */
        /* command_done message */
        0xC2, 0xF6, 0x58, 0x8E, /* service id */
        0xF0, 0x37, 0x4B, 0xC9,
        0x86, 0x65, 0xF4, 0xD4,
        0x4B, 0xD0, 0x93, 0x67,
        0x07, 0x00, 0x00, 0x00, /* command id */
        0x00, 0x00, 0x00, 0x00, /* status code */
        0x54, 0x00, 0x00, 0x00, /* buffer_length */
        /* information buffer */
        0x01, 0x00, 0x00, 0x00, /* version: 1 */
        0x01, 0x00, 0x00, 0x00, /* app count: 1 */
        0x00, 0x00, 0x00, 0x00, /* active app index: 0 */
        0x3C, 0x00, 0x00, 0x00, /* app list size bytes: 60 */
        0x18, 0x00, 0x00, 0x00, /* application 0 offset: 24 bytes */
        0x3C, 0x00, 0x00, 0x00, /* application 0 length: 60 bytes */
        /* application 0 */
        0x04, 0x00, 0x00, 0x00, /* application type: usim */
        0x20, 0x00, 0x00, 0x00, /* application id offset: 32 bytes */
        0x10, 0x00, 0x00, 0x00, /* application id length: 16 bytes */
        0x30, 0x00, 0x00, 0x00, /* application name offset: 48 bytes */
        0x08, 0x00, 0x00, 0x00, /* application name length: 8 bytes */
        0x02, 0x00, 0x00, 0x00, /* num pin key refs: 2 */
        0x38, 0x00, 0x00, 0x00, /* pin key refs offset: 56 bytes */
        0x02, 0x00, 0x00, 0x00, /* pin key refs length: 2 bytes */
        /* application 0 databuffer */
        0xA0, 0x00, 0x00, 0x00, /* application id */
        0x87, 0x10, 0x02, 0xFF,
        0x34, 0xFF, 0x07, 0x89,
        0x31, 0x2E, 0x30, 0xFF,
        0x4D, 0x6F, 0x76, 0x69, /* application name */
        0x73, 0x74, 0x61, 0x72,
        0x01, 0x81, 0x00, 0x00, /* pin key refs plus 2 padding bytes */
    };

    response = mbim_message_new (buffer, sizeof (buffer));
    test_message_printable (response, 1, 0);

    result = (mbim_message_ms_uicc_low_level_access_application_list_response_parse (
                  response,
                  &version,
                  &application_count,
                  &active_application_index,
                  &application_list_size_bytes,
                  &applications,
                  &error));
    g_assert_no_error (error);
    g_assert (result);

    g_assert_cmpuint (version, ==, 1);
    g_assert_cmpuint (application_count, ==, 1);
    g_assert_cmpuint (active_application_index, ==, 0);
    g_assert_cmpuint (application_list_size_bytes, ==, 60);
    g_assert_cmpuint (applications[0]->application_id_size, ==, sizeof (expected_application_id));
    g_assert (memcmp (applications[0]->application_id, expected_application_id, sizeof (expected_application_id)) == 0);
    g_assert (g_strcmp0 (applications[0]->application_name, expected_application_name) == 0);
    g_assert_cmpuint (applications[0]->pin_key_reference_count, ==, 2);
    g_assert_cmpuint (applications[0]->pin_key_references_size, ==, sizeof (expected_pin_key_references));
    g_assert (memcmp (applications[0]->pin_key_references, expected_pin_key_references, sizeof (expected_pin_key_references)) == 0);
}

int main (int argc, char **argv)
{
    g_test_init (&argc, &argv, NULL);

#define PREFIX "/libmbim-glib/message/parser"

    g_test_add_func (PREFIX "/basic-connect/visible-providers", test_basic_connect_visible_providers);
    g_test_add_func (PREFIX "/basic-connect/subscriber-ready-status", test_basic_connect_subscriber_ready_status);
    g_test_add_func (PREFIX "/basic-connect/device-caps", test_basic_connect_device_caps);
    g_test_add_func (PREFIX "/basic-connect/ip-configuration/1", test_basic_connect_ip_configuration);
    g_test_add_func (PREFIX "/basic-connect/ip-configuration/2", test_basic_connect_ip_configuration_2);
    g_test_add_func (PREFIX "/basic-connect/service-activation", test_basic_connect_service_activation);
    g_test_add_func (PREFIX "/basic-connect/register-state", test_basic_connect_register_state);
    g_test_add_func (PREFIX "/basic-connect/provisioned-contexts", test_provisioned_contexts);
    g_test_add_func (PREFIX "/sms/read/zero-pdu", test_sms_read_zero_pdu);
    g_test_add_func (PREFIX "/sms/read/single-pdu", test_sms_read_single_pdu);
    g_test_add_func (PREFIX "/sms/read/multiple-pdu", test_sms_read_multiple_pdu);
    g_test_add_func (PREFIX "/ussd", test_ussd);
    g_test_add_func (PREFIX "/auth/akap", test_auth_akap);
    g_test_add_func (PREFIX "/stk/pac/notification", test_stk_pac_notification);
    g_test_add_func (PREFIX "/stk/pac/response", test_stk_pac_response);
    g_test_add_func (PREFIX "/stk/terminal/response", test_stk_terminal_response);
    g_test_add_func (PREFIX "/stk/envelope/response", test_stk_envelope_response);
    g_test_add_func (PREFIX "/basic-connect/ip-packet-filters/none", test_basic_connect_ip_packet_filters_none);
    g_test_add_func (PREFIX "/basic-connect/ip-packet-filters/one", test_basic_connect_ip_packet_filters_one);
    g_test_add_func (PREFIX "/basic-connect/ip-packet-filters/two", test_basic_connect_ip_packet_filters_two);
    g_test_add_func (PREFIX "/ms-firmware-id/get", test_ms_firmware_id_get);
    g_test_add_func (PREFIX "/basic-connect/connect/short", test_basic_connect_connect_short);
    g_test_add_func (PREFIX "/basic-connect/visible-providers/overflow", test_basic_connect_visible_providers_overflow);
    g_test_add_func (PREFIX "/basic-connect-extensions/base-stations", test_ms_basic_connect_extensions_base_stations);
    g_test_add_func (PREFIX "/basic-connect-extensions/registration-parameters/0-unnamed-tlvs", test_ms_basic_connect_extensions_registration_parameters_0_unnamed_tlvs);
    g_test_add_func (PREFIX "/basic-connect-extensions/registration-parameters/1-unnamed-tlv", test_ms_basic_connect_extensions_registration_parameters_1_unnamed_tlv);
    g_test_add_func (PREFIX "/basic-connect-extensions/registration-parameters/3-unnamed-tlvs", test_ms_basic_connect_extensions_registration_parameters_3_unnamed_tlvs);
    g_test_add_func (PREFIX "/basic-connect-v3/connect/0-unnamed-tlvs", test_ms_basic_connect_v3_connect_0_unnamed_tlvs);
    g_test_add_func (PREFIX "/basic-connect-v3/connect/1-unnamed-tlv", test_ms_basic_connect_v3_connect_1_unnamed_tlv);
    g_test_add_func (PREFIX "/basic-connect-v3/connect/3-unnamed-tlvs", test_ms_basic_connect_v3_connect_3_unnamed_tlvs);
    g_test_add_func (PREFIX "/basic-connect-extensions/device-caps-v3", test_ms_basic_connect_extensions_device_caps_v3);
    g_test_add_func (PREFIX "/basic-connect-extensions/wake-reason/command", test_ms_basic_connect_extensions_wake_reason_command);
    g_test_add_func (PREFIX "/basic-connect-extensions/wake-reason/command/payload", test_ms_basic_connect_extensions_wake_reason_command_payload);
    g_test_add_func (PREFIX "/basic-connect-extensions/wake-reason/packet", test_ms_basic_connect_extensions_wake_reason_packet);
    g_test_add_func (PREFIX "/ms-uicc-low-level-access/application-list", test_ms_uicc_low_level_access_application_list);

#undef PREFIX

    return g_test_run ();
}
