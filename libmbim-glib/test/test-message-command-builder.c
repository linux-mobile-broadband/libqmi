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
 * Copyright (C) 2013 Aleksander Morgado <aleksander@gnu.org>
 */

#include <config.h>
#include <string.h>

#include "mbim-message.h"
#include "mbim-cid.h"
#include "mbim-enums.h"
#include "mbim-utils.h"
#include "mbim-basic-connect.h"


#if defined ENABLE_TEST_MESSAGE_TRACES
static void
test_message_trace (const guint8 *computed,
                    guint32       computed_size,
                    const guint8 *expected,
                    guint32       expected_size)
{
    gchar *message_str;
    gchar *expected_str;

    message_str = __mbim_utils_str_hex (computed, computed_size, ':');
    expected_str = __mbim_utils_str_hex (expected, expected_size, ':');

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

    g_free (message_str);
    g_free (expected_str);
}
#else
#define test_message_trace(...)
#endif

static void
test_message_command_builder_raw_set_pin (void)
{
    MbimMessage *message;
    MbimMessageCommandBuilder *builder;
    const guint8 expected_message [] = {
        /* header */
        0x03, 0x00, 0x00, 0x00, /* type */
        0x50, 0x00, 0x00, 0x00, /* length */
        0x01, 0x00, 0x00, 0x00, /* transaction id */
        /* fragment header */
        0x01, 0x00, 0x00, 0x00, /* total */
        0x00, 0x00, 0x00, 0x00, /* current */
        /* command_message */
        0xa2, 0x89, 0xcc, 0x33, /* service id */
        0xbc, 0xbb, 0x8b, 0x4f,
        0xb6, 0xb0, 0x13, 0x3e,
        0xc2, 0xaa, 0xe6, 0xdf,
        0x04, 0x00, 0x00, 0x00, /* command id */
        0x01, 0x00, 0x00, 0x00, /* command_type */
        0x20, 0x00, 0x00, 0x00, /* buffer_length */
        /* information buffer */
        0x02, 0x00, 0x00, 0x00, /* pin type */
        0x00, 0x00, 0x00, 0x00, /* pin operation */
        0x18, 0x00, 0x00, 0x00, /* pin offset */
        0x08, 0x00, 0x00, 0x00, /* pin size */
        0x00, 0x00, 0x00, 0x00, /* new pin offset */
        0x00, 0x00, 0x00, 0x00, /* new pin size */
        0x31, 0x00, 0x31, 0x00, /* pin string */
        0x31, 0x00, 0x31, 0x00
    };

    /* PIN set message */
    builder = _mbim_message_command_builder_new (1,
                                                 MBIM_SERVICE_BASIC_CONNECT,
                                                 MBIM_CID_BASIC_CONNECT_PIN,
                                                 MBIM_MESSAGE_COMMAND_TYPE_SET);
    _mbim_message_command_builder_append_guint32 (builder, (guint32)MBIM_PIN_TYPE_PIN1);
    _mbim_message_command_builder_append_guint32 (builder, (guint32)MBIM_PIN_OPERATION_ENTER);
    _mbim_message_command_builder_append_string  (builder, "1111");
    _mbim_message_command_builder_append_string  (builder, "");
    message = _mbim_message_command_builder_complete (builder);

    g_assert (message != NULL);

    test_message_trace ((const guint8 *)((GByteArray *)message)->data,
                        ((GByteArray *)message)->len,
                        expected_message,
                        sizeof (expected_message));

    g_assert_cmpuint (mbim_message_get_transaction_id (message), ==, 1);
    g_assert_cmpuint (mbim_message_get_message_type   (message), ==, MBIM_MESSAGE_TYPE_COMMAND);
    g_assert_cmpuint (mbim_message_get_message_length (message), ==, sizeof (expected_message));

    g_assert_cmpuint (mbim_message_command_get_service      (message), ==, MBIM_SERVICE_BASIC_CONNECT);
    g_assert_cmpuint (mbim_message_command_get_cid          (message), ==, MBIM_CID_BASIC_CONNECT_PIN);
    g_assert_cmpuint (mbim_message_command_get_command_type (message), ==, MBIM_MESSAGE_COMMAND_TYPE_SET);

    g_assert_cmpuint (((GByteArray *)message)->len, ==, sizeof (expected_message));
    g_assert (memcmp (((GByteArray *)message)->data, expected_message, sizeof (expected_message)) == 0);

    mbim_message_unref (message);
}

static void
test_message_command_builder_set_pin (void)
{
    GError *error = NULL;
    MbimMessage *message;
    const guint8 expected_message [] = {
        /* header */
        0x03, 0x00, 0x00, 0x00, /* type */
        0x50, 0x00, 0x00, 0x00, /* length */
        0x01, 0x00, 0x00, 0x00, /* transaction id */
        /* fragment header */
        0x01, 0x00, 0x00, 0x00, /* total */
        0x00, 0x00, 0x00, 0x00, /* current */
        /* command_message */
        0xa2, 0x89, 0xcc, 0x33, /* service id */
        0xbc, 0xbb, 0x8b, 0x4f,
        0xb6, 0xb0, 0x13, 0x3e,
        0xc2, 0xaa, 0xe6, 0xdf,
        0x04, 0x00, 0x00, 0x00, /* command id */
        0x01, 0x00, 0x00, 0x00, /* command_type */
        0x20, 0x00, 0x00, 0x00, /* buffer_length */
        /* information buffer */
        0x02, 0x00, 0x00, 0x00, /* pin type */
        0x00, 0x00, 0x00, 0x00, /* pin operation */
        0x18, 0x00, 0x00, 0x00, /* pin offset */
        0x08, 0x00, 0x00, 0x00, /* pin size */
        0x00, 0x00, 0x00, 0x00, /* new pin offset */
        0x00, 0x00, 0x00, 0x00, /* new pin size */
        0x31, 0x00, 0x31, 0x00, /* pin string */
        0x31, 0x00, 0x31, 0x00
    };

    /* PIN set message */
    message = mbim_message_basic_connect_pin_set_request_new (1,
                                                              MBIM_PIN_TYPE_PIN1,
                                                              MBIM_PIN_OPERATION_ENTER,
                                                              "1111",
                                                              "",
                                                              &error);
    g_assert_no_error (error);
    g_assert (message != NULL);

    test_message_trace ((const guint8 *)((GByteArray *)message)->data,
                        ((GByteArray *)message)->len,
                        expected_message,
                        sizeof (expected_message));

    g_assert_cmpuint (mbim_message_get_transaction_id (message), ==, 1);
    g_assert_cmpuint (mbim_message_get_message_type   (message), ==, MBIM_MESSAGE_TYPE_COMMAND);
    g_assert_cmpuint (mbim_message_get_message_length (message), ==, sizeof (expected_message));

    g_assert_cmpuint (mbim_message_command_get_service      (message), ==, MBIM_SERVICE_BASIC_CONNECT);
    g_assert_cmpuint (mbim_message_command_get_cid          (message), ==, MBIM_CID_BASIC_CONNECT_PIN);
    g_assert_cmpuint (mbim_message_command_get_command_type (message), ==, MBIM_MESSAGE_COMMAND_TYPE_SET);

    g_assert_cmpuint (((GByteArray *)message)->len, ==, sizeof (expected_message));
    g_assert (memcmp (((GByteArray *)message)->data, expected_message, sizeof (expected_message)) == 0);

    mbim_message_unref (message);
}

int main (int argc, char **argv)
{
    g_test_init (&argc, &argv, NULL);

    g_test_add_func ("/libmbim-glib/message/command-builder/raw/set-pin", test_message_command_builder_raw_set_pin);
    g_test_add_func ("/libmbim-glib/message/command-builder/set-pin",     test_message_command_builder_set_pin);

    return g_test_run ();
}
