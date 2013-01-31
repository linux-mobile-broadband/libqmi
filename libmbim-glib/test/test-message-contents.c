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
test_message_contents_basic_connect_device_caps (void)
{
    MbimMessage *response;
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
    gchar *str;

    response = mbim_message_new (buffer, sizeof (buffer));

    g_assert_cmpuint (mbim_message_basic_connect_device_caps_query_response_get_cellular_class (response), ==, MBIM_CELLULAR_CLASS_GSM);
    g_assert_cmpuint (mbim_message_basic_connect_device_caps_query_response_get_sim_class (response), ==, MBIM_SIM_CLASS_REMOVABLE);
    g_assert_cmpuint (mbim_message_basic_connect_device_caps_query_response_get_data_class (response), ==,
                      (MBIM_DATA_CLASS_GPRS |
                       MBIM_DATA_CLASS_EDGE |
                       MBIM_DATA_CLASS_UMTS |
                       MBIM_DATA_CLASS_HSDPA |
                       MBIM_DATA_CLASS_HSUPA |
                       MBIM_DATA_CLASS_CUSTOM));
    g_assert_cmpuint (mbim_message_basic_connect_device_caps_query_response_get_sms_caps (response), ==,
                      (MBIM_SMS_CAPS_PDU_RECEIVE | MBIM_SMS_CAPS_PDU_SEND));
    g_assert_cmpuint (mbim_message_basic_connect_device_caps_query_response_get_ctrl_caps (response), ==, MBIM_CTRL_CAPS_REG_MANUAL);
    g_assert_cmpuint (mbim_message_basic_connect_device_caps_query_response_get_max_sessions (response), ==, 1);

    str = mbim_message_basic_connect_device_caps_query_response_get_custom_data_class (response);
    g_assert_cmpstr (str, ==, "HSPA+");
    g_free (str);

    str = mbim_message_basic_connect_device_caps_query_response_get_device_id (response);
    g_assert_cmpstr (str, ==, "353613048804622");
    g_free (str);

    str = mbim_message_basic_connect_device_caps_query_response_get_firmware_info (response);
    g_assert_cmpstr (str, ==, "11.810.09.00.00");
    g_free (str);

    str = mbim_message_basic_connect_device_caps_query_response_get_hardware_info (response);
    g_assert_cmpstr (str, ==, "CP1E367UM");
    g_free (str);

    mbim_message_unref (response);
}

int main (int argc, char **argv)
{
    g_test_init (&argc, &argv, NULL);

    g_test_add_func ("/libmbim-glib/message-contents/basic-connect/device-caps", test_message_contents_basic_connect_device_caps);

    return g_test_run ();
}
