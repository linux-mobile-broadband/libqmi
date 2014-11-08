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
#include <glib-object.h>
#include <string.h>
#include "qmi-message.h"

static void
test_message_parse_common (const guint8 *buffer,
                           guint buffer_len,
                           guint n_expected_messages)
{
    GError *error = NULL;
    GByteArray *array;
    guint n_messages = 0;

    array = g_byte_array_sized_new (buffer_len);
    g_byte_array_append (array, buffer, buffer_len);

    do {
        QmiMessage *message;
        gchar *printable;

        message = qmi_message_new_from_raw (array, &error);
        if (!message) {
            if (error) {
                if (n_messages < n_expected_messages)
                    g_printerr ("error creating message from raw data: '%s'\n", error->message);
                g_error_free (error);
            }
            break;
        }

        printable = qmi_message_get_printable (message, "");
        g_print ("\n%s\n", printable);
        g_free (printable);

        n_messages++;
        qmi_message_unref (message);
    } while (array->len > 0);

    g_assert_cmpuint (n_messages, ==, n_expected_messages);
}

static void
test_message_parse_short (void)
{
    const guint8 buffer[] = {
        0x01, 0x26, 0x00, 0x80, 0x03, 0x01, 0x02, 0x01, 0x00, 0x20, 0x00, 0x1a,
        0x00, 0x02, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x00, 0x9b,
        0x05, 0x11, 0x04, 0x00, 0x01, 0x00, 0x66, 0x05
    };

    test_message_parse_common (buffer, sizeof (buffer), 0);
}

static void
test_message_parse_complete (void)
{
    const guint8 buffer[] = {
        0x01, 0x26, 0x00, 0x80, 0x03, 0x01, 0x02, 0x01, 0x00, 0x20, 0x00, 0x1a,
        0x00, 0x02, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x00, 0x9b,
        0x05, 0x11, 0x04, 0x00, 0x01, 0x00, 0x65, 0x05, 0x12, 0x04, 0x00, 0x01,
        0x00, 0x11, 0x05
    };

    test_message_parse_common (buffer, sizeof (buffer), 1);
}

static void
test_message_parse_complete_and_short (void)
{
    const guint8 buffer[] = {
        0x01, 0x26, 0x00, 0x80, 0x03, 0x01, 0x02, 0x01, 0x00, 0x20, 0x00, 0x1a,
        0x00, 0x02, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x00, 0x9b,
        0x05, 0x11, 0x04, 0x00, 0x01, 0x00, 0x65, 0x05, 0x12, 0x04, 0x00, 0x01,
        0x00, 0x11, 0x05, 0x01, 0x26, 0x00, 0x80, 0x03, 0x01, 0x02, 0x01, 0x00,
        0x20, 0x00, 0x1a, 0x00, 0x02, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
        0x02, 0x00, 0x9b, 0x05, 0x11, 0x04, 0x00, 0x01, 0x00, 0x66, 0x05
    };

    test_message_parse_common (buffer, sizeof (buffer), 1);
}

static void
test_message_parse_complete_and_complete (void)
{
    const guint8 buffer[] = {
        0x01, 0x26, 0x00, 0x80, 0x03, 0x01, 0x02, 0x01, 0x00, 0x20, 0x00, 0x1a,
        0x00, 0x02, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x00, 0x9b,
        0x05, 0x11, 0x04, 0x00, 0x01, 0x00, 0x65, 0x05, 0x12, 0x04, 0x00, 0x01,
        0x00, 0x11, 0x05, 0x01, 0x26, 0x00, 0x80, 0x03, 0x01, 0x02, 0x01, 0x00,
        0x20, 0x00, 0x1a, 0x00, 0x02, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
        0x02, 0x00, 0x9b, 0x05, 0x11, 0x04, 0x00, 0x01, 0x00, 0x65, 0x05, 0x12,
        0x04, 0x00, 0x01, 0x00, 0x11, 0x05
    };

    test_message_parse_common (buffer, sizeof (buffer), 2);
}

#if GLIB_CHECK_VERSION (2,34,0)
static void
test_message_parse_wrong_tlv (void)
{
    const guint8 buffer[] = {
        0x01, 0x4F, 0x00, 0x80, 0x03, 0x03, 0x02, 0x01, 0x00, 0x24, 0x00, 0x43,
        0x00, 0x02, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x21, 0x04, 0x00, 0x02,
        0x03, 0x00, 0x00, 0x1D, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1C, 0x02,
        0x00, 0x00, 0x00, 0x15, 0x03, 0x00, 0x01, 0x05, 0x01, 0x12, 0x0E, 0x00,
        0x36, 0x01, 0x04, 0x01, 0x09, 0x20, 0x54, 0x2D, 0x4D, 0x6F, 0x62, 0x69,
        0x6C, 0x65, 0x11, 0x02, 0x00, 0x01, 0x05, 0x10, 0x01, 0x00, 0x01, 0x01,
        0x06, 0x00, 0x01, 0x01, 0x01, 0x02, 0x01, 0x05
    };

    g_test_expect_message ("Qmi",
                           G_LOG_LEVEL_WARNING,
                           "Cannot read the '*' TLV: expected '*' bytes, but only got '*' bytes");
    test_message_parse_common (buffer, sizeof (buffer), 1);
    g_test_assert_expected_messages ();
}

static void
test_message_parse_missing_size (void)
{
    /* PDS Event Report indication: NMEA position */
    const guint8 buffer[] = {
        0x01,       /* marker */
        0x10, 0x00, /* qmux length */
        0x80,       /* qmux flags */
        0x06,       /* service: PDS */
        0x03,       /* client */
        0x04,       /* service flags: Indication */
        0x01, 0x00, /* transaction */
        0x01, 0x00, /* message: Event Report */
        0x04, 0x00, /* all tlvs length: 4 bytes */
        /* TLV */
        0x11,       /* type: Extended NMEA Position (1 guint8 and one 16-bit-sized string) */
        0x01, 0x00, /* length: 1 byte (WE ONLY GIVE THE GUINT8!!!) */
        0x01
    };

    g_test_expect_message ("Qmi",
                           G_LOG_LEVEL_WARNING,
                           "Cannot read the string size: expected '*' bytes, but only got '*' bytes");
    test_message_parse_common (buffer, sizeof (buffer), 1);
    g_test_assert_expected_messages ();
}
#endif

static void
test_message_set_transaction_id_ctl (void)
{
    GByteArray *buffer;
    QmiMessage *message;
    GError     *error = NULL;
    guint8      ctl_message[] = {
        0x01, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xFF, /* TRID to update */
        0x22, 0x00, 0x04, 0x00, 0x01, 0x01, 0x00, 0x01
    };

    /* CTL message */
    buffer = g_byte_array_append (g_byte_array_sized_new (G_N_ELEMENTS (ctl_message)), ctl_message, G_N_ELEMENTS (ctl_message));
    message = qmi_message_new_from_raw (buffer, &error);
    g_assert_no_error (error);
    g_assert (message);

    qmi_message_set_transaction_id (message, 0x55);
    g_assert_cmpuint (qmi_message_get_transaction_id (message), ==, 0x55);

    qmi_message_unref (message);
    g_byte_array_unref (buffer);
}

static void
test_message_set_transaction_id_services (void)
{
    GByteArray *buffer;
    QmiMessage *message;
    GError     *error = NULL;
    guint8      dms_message[] = {
        0x01, 0x0C, 0x00, 0x00, 0x02, 0x01, 0x00,
        0xFF, 0xFF, /* TRID to update */
        0x25, 0x00, 0x00, 0x00
    };

    /* DMS message */
    buffer = g_byte_array_append (g_byte_array_sized_new (G_N_ELEMENTS (dms_message)), dms_message, G_N_ELEMENTS (dms_message));
    message = qmi_message_new_from_raw (buffer, &error);
    g_assert_no_error (error);
    g_assert (message);

    qmi_message_set_transaction_id (message, 0x5566);
    g_assert_cmpuint (qmi_message_get_transaction_id (message), ==, 0x5566);

    qmi_message_unref (message);
    g_byte_array_unref (buffer);
}

int main (int argc, char **argv)
{
    g_type_init ();
    g_test_init (&argc, &argv, NULL);

    g_test_add_func ("/libqmi-glib/message/parse/short",                 test_message_parse_short);
    g_test_add_func ("/libqmi-glib/message/parse/complete",              test_message_parse_complete);
    g_test_add_func ("/libqmi-glib/message/parse/complete-and-short",    test_message_parse_complete_and_short);
    g_test_add_func ("/libqmi-glib/message/parse/complete-and-complete", test_message_parse_complete_and_complete);
#if GLIB_CHECK_VERSION (2,34,0)
    g_test_add_func ("/libqmi-glib/message/parse/wrong-tlv",             test_message_parse_wrong_tlv);
    g_test_add_func ("/libqmi-glib/message/parse/missing-size",          test_message_parse_missing_size);
#endif
    g_test_add_func ("/libqmi-glib/message/set-transaction-id/ctl",      test_message_set_transaction_id_ctl);
    g_test_add_func ("/libqmi-glib/message/set-transaction-id/services", test_message_set_transaction_id_services);

    return g_test_run ();
}
