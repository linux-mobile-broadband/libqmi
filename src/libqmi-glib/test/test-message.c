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
 * Copyright (C) 2012-2014 Aleksander Morgado <aleksander@aleksander.es>
 */

#include <config.h>
#include <glib-object.h>
#include <string.h>
#include <stdio.h>

#include <libqmi-glib.h>

/*****************************************************************************/

static gchar *
__str_hex (gconstpointer mem,
           gsize         size,
           gchar         delimiter)
{
    const guint8 *data = mem;
    gsize i;
    gsize j;
    gsize new_str_length;
    gchar *new_str;

    /* Get new string length. If input string has N bytes, we need:
     * - 1 byte for last NUL char
     * - 2N bytes for hexadecimal char representation of each byte...
     * - N-1 bytes for the separator ':'
     * So... a total of (1+2N+N-1) = 3N bytes are needed... */
    new_str_length =  3 * size;

    /* Allocate memory for new array and initialize contents to NUL */
    new_str = g_malloc0 (new_str_length);

    /* Print hexadecimal representation of each byte... */
    for (i = 0, j = 0; i < size; i++, j += 3) {
        /* Print character in output string... */
        snprintf (&new_str[j], 3, "%02X", data[i]);
        /* And if needed, add separator */
        if (i != (size - 1) )
            new_str[j + 2] = delimiter;
    }

    /* Set output string */
    return new_str;
}

static void
_g_assert_cmpmem (gconstpointer mem1,
                  gsize         size1,
                  gconstpointer mem2,
                  gsize         size2)
{
    g_autofree gchar *str1 = NULL;
    g_autofree gchar *str2 = NULL;

    str1 = __str_hex (mem1, size1, ':');
    str2 = __str_hex (mem2, size2, ':');
    g_assert_cmpstr (str1, ==, str2);
}

/*****************************************************************************/

static void
test_message_parse_common (const guint8 *buffer,
                           guint buffer_len,
                           guint n_expected_messages)
{
    g_autoptr(GByteArray) array = NULL;
    guint                 n_messages = 0;

    array = g_byte_array_sized_new (buffer_len);
    g_byte_array_append (array, buffer, buffer_len);

    do {
        g_autoptr(GError)      error = NULL;
        g_autoptr(QmiMessage)  message = NULL;
        g_autofree gchar      *printable = NULL;

        message = qmi_message_new_from_raw (array, &error);
        if (!message) {
            if (error && (n_messages < n_expected_messages))
                g_printerr ("error creating message from raw data: '%s'\n", error->message);
            break;
        }

        printable = qmi_message_get_printable_full (message, NULL, "");
        g_debug ("%s", printable);

        n_messages++;
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

static void
test_message_printable_common (const guint8 *buffer,
                               guint         buffer_len,
                               guint16       vendor_id,
                               const gchar  *expected_in_printable)
{
    g_autoptr(QmiMessageContext)  context = NULL;
    g_autoptr(QmiMessage)         message = NULL;
    g_autoptr(GByteArray)         array = NULL;
    g_autoptr(GError)             error = NULL;
    g_autofree gchar             *printable = NULL;

    if (vendor_id != QMI_MESSAGE_VENDOR_GENERIC) {
        context = qmi_message_context_new ();
        qmi_message_context_set_vendor_id (context, vendor_id);
    }

    array = g_byte_array_sized_new (buffer_len);
    g_byte_array_append (array, buffer, buffer_len);
    message = qmi_message_new_from_raw (array, &error);
    g_assert_no_error (error);
    g_assert (message);

    printable = qmi_message_get_printable_full (message, context, "");
    g_debug ("%s", printable);
    g_assert (strstr (printable, expected_in_printable));
}

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

    test_message_printable_common (buffer, G_N_ELEMENTS (buffer), QMI_MESSAGE_VENDOR_GENERIC, "ERROR: Reading TLV would overflow");
}

#if defined HAVE_QMI_INDICATION_PDS_EVENT_REPORT

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

    test_message_printable_common (buffer, G_N_ELEMENTS (buffer), QMI_MESSAGE_VENDOR_GENERIC, "ERROR: Reading TLV would overflow");
}

#endif

#if defined HAVE_QMI_INDICATION_LOC_NMEA

static void
test_message_parse_string_with_crlf (void)
{
    /* LOC indication: NMEA
     * The NMEA trace comes suffixed with \r\n, this test makes sure the proper
     * parsing is done on the string.
     */
    const guint8 buffer[] = {
        0x01, 0x2D, 0x00, 0x80, 0x10, 0x01, 0x04, 0xB4, 0x00, 0x26, 0x00, 0x21,
        0x00, 0x01, 0x1E, 0x00, 0x24, 0x47, 0x50, 0x47, 0x53, 0x41, 0x2C, 0x41,
        0x2C, 0x31, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C,
        0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2A, 0x31, 0x45, 0x0D, 0x0A
    };

    test_message_printable_common (buffer, sizeof (buffer), QMI_MESSAGE_VENDOR_GENERIC, "$GPGSA,A,1,,,,,,,,,,,,,,,*1E");
}

#endif

#if defined HAVE_QMI_MESSAGE_DMS_SWI_GET_CURRENT_FIRMWARE

static void
test_message_parse_string_with_trailing_nul (void)
{
    /* The Sierra Wireless (vid 0x1199) specific method to get current firmware
     * info uses NUL-terminated strings.
     */
    const guint8 buffer[] = {
        0x01, 0x82, 0x00, 0x80, 0x02, 0x03, 0x02, 0x01, 0x00, 0x56, 0x55, 0x76,
        0x00, 0x02, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x0C, 0x00, 0x30,
        0x30, 0x32, 0x2E, 0x30, 0x37, 0x32, 0x5F, 0x30, 0x30, 0x30, 0x00, 0x17,
        0x08, 0x00, 0x47, 0x45, 0x4E, 0x45, 0x52, 0x49, 0x43, 0x00, 0x16, 0x08,
        0x00, 0x30, 0x30, 0x30, 0x2E, 0x30, 0x30, 0x35, 0x00, 0x15, 0x02, 0x00,
        0x31, 0x00, 0x13, 0x08, 0x00, 0x31, 0x31, 0x30, 0x32, 0x34, 0x37, 0x36,
        0x00, 0x12, 0x15, 0x00, 0x53, 0x57, 0x49, 0x39, 0x58, 0x33, 0x30, 0x43,
        0x5F, 0x30, 0x32, 0x2E, 0x33, 0x33, 0x2E, 0x30, 0x33, 0x2E, 0x30, 0x30,
        0x00, 0x11, 0x15, 0x00, 0x53, 0x57, 0x49, 0x39, 0x58, 0x33, 0x30, 0x43,
        0x5F, 0x30, 0x32, 0x2E, 0x33, 0x33, 0x2E, 0x30, 0x33, 0x2E, 0x30, 0x30,
        0x00, 0x10, 0x07, 0x00, 0x4D, 0x43, 0x37, 0x34, 0x35, 0x35, 0x00
    };

    test_message_printable_common (buffer, sizeof (buffer), 0x1199, "SWI9X30C_02.33.03.00");
}

#endif

#if defined HAVE_QMI_MESSAGE_DMS_GET_MODEL

static void
test_message_parse_string_with_trailing_tab (void)
{
    /* Quectel EM12-AW model strint has a trailing TAB character (ASCII 0x09) */
    const guint8 buffer[] = {
        0x01, 0x1E, 0x00, 0x80, 0x02, 0x05, 0x02, 0x01, 0x00, 0x22, 0x00, 0x12,
        0x00, 0x02, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x08, 0x00, 0x45,
        0x4D, 0x31, 0x32, 0x2D, 0x41, 0x57, 0x09
    };

    test_message_printable_common (buffer, sizeof (buffer), QMI_MESSAGE_VENDOR_GENERIC, "EM12-AW");
}

#endif

#if defined HAVE_QMI_MESSAGE_NAS_SWI_GET_STATUS

static void
test_message_parse_signed_int (void)
{
    /* Temperature given as a signed int */
    const guint8 buffer[] = {
        0x01, 0x27, 0x00, 0x80, 0x03, 0x05, 0x02, 0x01, 0x00, 0x56, 0x55, 0x1B,
        0x00, 0x02, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x09, 0x00, 0x08,
        0x01, 0xFC, 0x0D, 0xFF, 0xFF, 0x02, 0x00, 0x00, 0x01, 0x05, 0x00, 0xFB,
        0x05, 0x09, 0x00, 0x00
    };

    test_message_printable_common (buffer, sizeof (buffer), 0x1199, "temperature = '-5'");
}

#endif

/*****************************************************************************/

static void
test_message_new_request (void)
{
    static const guint8 expected_buffer [] = {
        0x01,       /* marker */
        0x0C, 0x00, /* qmux length */
        0x00,       /* qmux flags */
        0x02,       /* service: DMS */
        0x01,       /* client id */
        0x00,       /* service flags */
        0x02, 0x00, /* transaction */
        0xFF, 0xFF, /* message id */
        0x00, 0x00, /* all tlvs length */
    };

    g_autoptr(QmiMessage)  self = NULL;
    g_autoptr(GError)      error = NULL;
    const guint8          *buffer;
    gsize                  buffer_length = 0;

    self = qmi_message_new (QMI_SERVICE_DMS, 0x01, 0x02, 0xFFFF);
    g_assert (self);

    buffer = qmi_message_get_raw (self, &buffer_length, &error);
    g_assert_no_error (error);
    g_assert (buffer != NULL);
    g_assert_cmpuint (buffer_length, >, 0);

    _g_assert_cmpmem (buffer, buffer_length, expected_buffer, sizeof (expected_buffer));
}

static void
test_message_new_request_from_data (void)
{
    static const guint8 expected_buffer [] = {
        0x00,       /* service flags */
        0x02, 0x00, /* transaction */
        0xFF, 0xFF, /* message id */
        0x00, 0x00, /* all tlvs length */
    };

    g_autoptr(GByteArray)  qmi = NULL;
    g_autoptr(QmiMessage)  self = NULL;
    g_autoptr(GError)      error = NULL;
    const guint8          *buffer;
    gsize                  buffer_length = 0;

    /* set up service header */
    qmi = g_byte_array_new ();
    g_byte_array_append (qmi, expected_buffer, sizeof (expected_buffer));

    self = qmi_message_new_from_data (QMI_SERVICE_DMS, 0x01, qmi, &error);
    g_assert_no_error (error);
    g_assert (self);

    /* check that the QMUX header contains the right values*/
    g_assert_cmpuint (qmi_message_get_service (self), ==, QMI_SERVICE_DMS);
    g_assert_cmpuint (qmi_message_get_client_id (self), ==, 0x01);

    /* length (13) = qmux marker (1) + qmux header length (5) + qmi header length (7) */
    g_assert_cmpuint (qmi_message_get_length (self), ==, 13);

    /* check that transaction and message IDs line up */
    g_assert_cmpuint (qmi_message_get_transaction_id (self), ==, 0x02);
    g_assert_cmpuint (qmi_message_get_message_id (self), ==, 0xFFFF);

    buffer = qmi_message_get_raw (self, &buffer_length, &error);
    g_assert_no_error (error);
    g_assert (buffer != NULL);
    g_assert_cmpuint (buffer_length, >, 0);
    /* check that we have a qmux marker so we don't break framing */
    g_assert_cmpuint (buffer[0], ==, 0x01);

    /* make sure the qmi portion of the message is what we expect */
    buffer = qmi_message_get_data (self, &buffer_length, &error);
    g_assert_no_error (error);
    g_assert (buffer != NULL);
    g_assert_cmpuint (buffer_length, >, 0);
    _g_assert_cmpmem (buffer, buffer_length, expected_buffer, sizeof (expected_buffer));
}

static void
test_message_new_response_ok (void)
{
    static const guint8 expected_buffer [] = {
        0x01,       /* marker */
        0x13, 0x00, /* qmux length */
        0x80,       /* qmux flags */
        0x02,       /* service: DMS */
        0x01,       /* client id */
        0x02,       /* service flags */
        0x02, 0x00, /* transaction */
        0xFF, 0xFF, /* message id */
        0x07, 0x00, /* all tlvs length */
        /* TLV */
        0x02,       /* tlv type */
        0x04, 0x00, /* tlv size */
        0x00, 0x00, 0x00, 0x00
    };

    g_autoptr(QmiMessage)  request = NULL;
    g_autoptr(QmiMessage)  response = NULL;
    g_autoptr(GError)      error = NULL;
    const guint8          *buffer;
    gsize                  buffer_length = 0;

    request = qmi_message_new (QMI_SERVICE_DMS, 0x01, 0x02, 0xFFFF);
    g_assert (request);

    response = qmi_message_response_new (request, QMI_PROTOCOL_ERROR_NONE);
    g_assert (response);

    buffer = qmi_message_get_raw (response, &buffer_length, &error);
    g_assert_no_error (error);
    g_assert (buffer != NULL);
    g_assert_cmpuint (buffer_length, >, 0);

    _g_assert_cmpmem (buffer, buffer_length, expected_buffer, sizeof (expected_buffer));
}

static void
test_message_new_response_error (void)
{
    static const guint8 expected_buffer [] = {
        0x01,       /* marker */
        0x13, 0x00, /* qmux length */
        0x80,       /* qmux flags */
        0x02,       /* service: DMS */
        0x01,       /* client id */
        0x02,       /* service flags */
        0x02, 0x00, /* transaction */
        0xFF, 0xFF, /* message id */
        0x07, 0x00, /* all tlvs length */
        /* TLV */
        0x02,       /* tlv type */
        0x04, 0x00, /* tlv size */
        0x01, 0x00, 0x03, 0x00
    };

    g_autoptr(QmiMessage)  request = NULL;
    g_autoptr(QmiMessage)  response = NULL;
    g_autoptr(GError)      error = NULL;
    const guint8          *buffer;
    gsize                  buffer_length = 0;

    request = qmi_message_new (QMI_SERVICE_DMS, 0x01, 0x02, 0xFFFF);
    g_assert (request);

    response = qmi_message_response_new (request, QMI_PROTOCOL_ERROR_INTERNAL);
    g_assert (response);

    buffer = qmi_message_get_raw (response, &buffer_length, &error);
    g_assert_no_error (error);
    g_assert (buffer != NULL);
    g_assert_cmpuint (buffer_length, >, 0);

    _g_assert_cmpmem (buffer, buffer_length, expected_buffer, sizeof (expected_buffer));
}

/*****************************************************************************/

static void
test_message_tlv_write_empty (void)
{
    g_autoptr(QmiMessage) self = NULL;
    g_autoptr(GError)     error = NULL;
    gboolean              ret;
    gsize                 init_offset;

    self = qmi_message_new (QMI_SERVICE_DMS, 0x01, 0x02, 0xFFFF);

    init_offset = qmi_message_tlv_write_init (self, 0x01, &error);
    g_assert_no_error (error);
    g_assert (init_offset > 0);

    ret = qmi_message_tlv_write_complete (self, init_offset, &error);
    g_assert_no_error (error);
    g_assert (ret);
}

static void
test_message_tlv_write_reset (void)
{
    g_autoptr(QmiMessage) self = NULL;
    g_autoptr(GError)     error = NULL;
    gboolean              ret;
    gsize                 init_offset;
    gsize                 previous_size;

    self = qmi_message_new (QMI_SERVICE_DMS, 0x01, 0x02, 0xFFFF);
    previous_size = qmi_message_get_length (self);

    /* Test reset just after init */
    init_offset = qmi_message_tlv_write_init (self, 0x01, &error);
    g_assert_no_error (error);
    g_assert (init_offset > 0);

    qmi_message_tlv_write_reset (self, init_offset);
    g_assert_cmpuint (previous_size, ==, qmi_message_get_length (self));

    /* Test reset after adding variables */
    init_offset = qmi_message_tlv_write_init (self, 0x01, &error);
    g_assert_no_error (error);
    g_assert (init_offset > 0);

    ret = qmi_message_tlv_write_guint8 (self, 0x12, &error);
    g_assert_no_error (error);
    g_assert (ret);

    qmi_message_tlv_write_reset (self, init_offset);
    g_assert_cmpuint (previous_size, ==, qmi_message_get_length (self));
}

static void
test_message_tlv_rw_8 (void)
{
    g_autoptr(QmiMessage) self = NULL;
    g_autoptr(GError)     error = NULL;
    gboolean              ret;
    gsize                 init_offset;
    gsize                 expected_tlv_payload_size = 0;
    guint16               tlv_length = 0;
    gsize                 offset;
    guint8                uint8;
    gint8                 int8;

    self = qmi_message_new (QMI_SERVICE_DMS, 0x01, 0x02, 0xFFFF);

    init_offset = qmi_message_tlv_write_init (self, 0x01, &error);
    g_assert_no_error (error);
    g_assert (init_offset > 0);

    /* Add one of each */
    expected_tlv_payload_size += 1;
    ret = qmi_message_tlv_write_guint8 (self, 0x12, &error);
    g_assert_no_error (error);
    g_assert (ret);

    expected_tlv_payload_size += 1;
    ret = qmi_message_tlv_write_gint8 (self, 0 - 0x12, &error);
    g_assert_no_error (error);
    g_assert (ret);

    ret = qmi_message_tlv_write_complete (self, init_offset, &error);
    g_assert_no_error (error);
    g_assert (ret);

    /* Now read */
    init_offset = qmi_message_tlv_read_init (self, 0x01, &tlv_length, &error);
    g_assert_no_error (error);
    g_assert (init_offset > 0);
    g_assert_cmpuint (tlv_length, ==, expected_tlv_payload_size);

    offset = 0;

    ret = qmi_message_tlv_read_guint8 (self, init_offset, &offset, &uint8, &error);
    g_assert_no_error (error);
    g_assert (ret);
    g_assert_cmpuint (uint8, ==, 0x12);

    ret = qmi_message_tlv_read_gint8 (self, init_offset, &offset, &int8, &error);
    g_assert_no_error (error);
    g_assert (ret);
    g_assert_cmpuint (int8, ==, 0 - 0x12);
}

static void
test_message_tlv_rw_16 (void)
{
    guint n_bytes_prefixed;

    /* We'll add [0 or 1] bytes before the actual 16-bit values, so that we
     * check all possible memory alignments. */
    for (n_bytes_prefixed = 0; n_bytes_prefixed <= 1; n_bytes_prefixed++) {
        g_autoptr(QmiMessage) self = NULL;
        g_autoptr(GError)     error = NULL;
        gboolean              ret;
        gsize                 init_offset;
        guint16               tlv_length = 0;
        gsize                 offset;
        gsize                 expected_tlv_payload_size = 0;
        guint16               uint16;
        gint16                int16;
        guint                 i;

        self = qmi_message_new (QMI_SERVICE_DMS, 0x01, 0x02, 0xFFFF);

        init_offset = qmi_message_tlv_write_init (self, 0x01, &error);
        g_assert_no_error (error);
        g_assert (init_offset > 0);

        for (i = 0; i < n_bytes_prefixed; i++) {
            expected_tlv_payload_size += 1;
            ret = qmi_message_tlv_write_guint8 (self, 0xFF, &error);
            g_assert_no_error (error);
            g_assert (ret);
        }

        /* Add one of each */
        expected_tlv_payload_size += 2;
        ret = qmi_message_tlv_write_guint16 (self, QMI_ENDIAN_LITTLE, 0x1212, &error);
        g_assert_no_error (error);
        g_assert (ret);

        expected_tlv_payload_size += 2;
        ret = qmi_message_tlv_write_gint16 (self, QMI_ENDIAN_LITTLE, 0 - 0x1212, &error);
        g_assert_no_error (error);
        g_assert (ret);

        expected_tlv_payload_size += 2;
        ret = qmi_message_tlv_write_guint16 (self, QMI_ENDIAN_BIG, 0x1212, &error);
        g_assert_no_error (error);
        g_assert (ret);

        expected_tlv_payload_size += 2;
        ret = qmi_message_tlv_write_gint16 (self, QMI_ENDIAN_BIG, 0 - 0x1212, &error);
        g_assert_no_error (error);
        g_assert (ret);

        ret = qmi_message_tlv_write_complete (self, init_offset, &error);
        g_assert_no_error (error);
        g_assert (ret);

        /* Now read */
        init_offset = qmi_message_tlv_read_init (self, 0x01, &tlv_length, &error);
        g_assert_no_error (error);
        g_assert (init_offset > 0);
        g_assert_cmpuint (tlv_length, ==, expected_tlv_payload_size);

        offset = 0;

        for (i = 0; i < n_bytes_prefixed; i++) {
            guint8 aux;

            ret = qmi_message_tlv_read_guint8 (self, init_offset, &offset, &aux, &error);
            g_assert_no_error (error);
            g_assert (ret);
            g_assert_cmpuint (aux, ==, 0xFF);
        }

        ret = qmi_message_tlv_read_guint16 (self, init_offset, &offset, QMI_ENDIAN_LITTLE, &uint16, &error);
        g_assert_no_error (error);
        g_assert (ret);
        g_assert_cmpuint (uint16, ==, 0x1212);

        ret = qmi_message_tlv_read_gint16 (self, init_offset, &offset, QMI_ENDIAN_LITTLE, &int16, &error);
        g_assert_no_error (error);
        g_assert (ret);
        g_assert_cmpuint (int16, ==, 0 - 0x1212);

        ret = qmi_message_tlv_read_guint16 (self, init_offset, &offset, QMI_ENDIAN_BIG, &uint16, &error);
        g_assert_no_error (error);
        g_assert (ret);
        g_assert_cmpuint (uint16, ==, 0x1212);

        ret = qmi_message_tlv_read_gint16 (self, init_offset, &offset, QMI_ENDIAN_BIG, &int16, &error);
        g_assert_no_error (error);
        g_assert (ret);
        g_assert_cmpuint (int16, ==, 0 - 0x1212);
    }
}

static void
test_message_tlv_rw_32 (void)
{
    guint n_bytes_prefixed;

    /* We'll add [0, 1, 2, 3] bytes before the actual 32-bit values, so that we
     * check all possible memory alignments. */
    for (n_bytes_prefixed = 0; n_bytes_prefixed <= 3; n_bytes_prefixed++) {
        g_autoptr(QmiMessage) self = NULL;
        g_autoptr(GError)     error = NULL;
        gboolean              ret;
        gsize                 init_offset;
        guint16               tlv_length = 0;
        gsize                 offset;
        gsize                 expected_tlv_payload_size = 0;
        guint32               uint32;
        gint32                int32;
        guint                 i;

        self = qmi_message_new (QMI_SERVICE_DMS, 0x01, 0x02, 0xFFFF);

        init_offset = qmi_message_tlv_write_init (self, 0x01, &error);
        g_assert_no_error (error);
        g_assert (init_offset > 0);

        for (i = 0; i < n_bytes_prefixed; i++) {
            expected_tlv_payload_size += 1;
            ret = qmi_message_tlv_write_guint8 (self, 0xFF, &error);
            g_assert_no_error (error);
            g_assert (ret);
        }

        /* Add one of each */
        expected_tlv_payload_size += 4;
        ret = qmi_message_tlv_write_guint32 (self, QMI_ENDIAN_LITTLE, 0x12121212, &error);
        g_assert_no_error (error);
        g_assert (ret);

        expected_tlv_payload_size += 4;
        ret = qmi_message_tlv_write_gint32 (self, QMI_ENDIAN_LITTLE, 0 - 0x12121212, &error);
        g_assert_no_error (error);
        g_assert (ret);

        expected_tlv_payload_size += 4;
        ret = qmi_message_tlv_write_guint32 (self, QMI_ENDIAN_BIG, 0x12121212, &error);
        g_assert_no_error (error);
        g_assert (ret);

        expected_tlv_payload_size += 4;
        ret = qmi_message_tlv_write_gint32 (self, QMI_ENDIAN_BIG, 0 - 0x12121212, &error);
        g_assert_no_error (error);
        g_assert (ret);

        ret = qmi_message_tlv_write_complete (self, init_offset, &error);
        g_assert_no_error (error);
        g_assert (ret);

        /* Now read */
        init_offset = qmi_message_tlv_read_init (self, 0x01, &tlv_length, &error);
        g_assert_no_error (error);
        g_assert (init_offset > 0);
        g_assert_cmpuint (tlv_length, ==, expected_tlv_payload_size);

        offset = 0;

        for (i = 0; i < n_bytes_prefixed; i++) {
            guint8 aux;

            ret = qmi_message_tlv_read_guint8 (self, init_offset, &offset, &aux, &error);
            g_assert_no_error (error);
            g_assert (ret);
            g_assert_cmpuint (aux, ==, 0xFF);
        }

        ret = qmi_message_tlv_read_guint32 (self, init_offset, &offset, QMI_ENDIAN_LITTLE, &uint32, &error);
        g_assert_no_error (error);
        g_assert (ret);
        g_assert_cmpuint (uint32, ==, 0x12121212);

        ret = qmi_message_tlv_read_gint32 (self, init_offset, &offset, QMI_ENDIAN_LITTLE, &int32, &error);
        g_assert_no_error (error);
        g_assert (ret);
        g_assert_cmpuint (int32, ==, 0 - 0x12121212);

        ret = qmi_message_tlv_read_guint32 (self, init_offset, &offset, QMI_ENDIAN_BIG, &uint32, &error);
        g_assert_no_error (error);
        g_assert (ret);
        g_assert_cmpuint (uint32, ==, 0x12121212);

        ret = qmi_message_tlv_read_gint32 (self, init_offset, &offset, QMI_ENDIAN_BIG, &int32, &error);
        g_assert_no_error (error);
        g_assert (ret);
        g_assert_cmpuint (int32, ==, 0 - 0x12121212);
    }
}

static void
test_message_tlv_rw_64 (void)
{
    guint n_bytes_prefixed;

    /* We'll add [0, 1, 2, 3, 4, 5, 6, 7] bytes before the actual 64-bit values,
     * so that we check all possible memory alignments. */
    for (n_bytes_prefixed = 0; n_bytes_prefixed <= 7; n_bytes_prefixed++) {
        g_autoptr(QmiMessage) self = NULL;
        g_autoptr(GError)     error = NULL;
        gboolean              ret;
        gsize                 init_offset;
        guint16               tlv_length = 0;
        gsize                 offset;
        gsize                 expected_tlv_payload_size = 0;
        guint64               uint64;
        gint64                int64;
        guint                 i;

        self = qmi_message_new (QMI_SERVICE_DMS, 0x01, 0x02, 0xFFFF);

        init_offset = qmi_message_tlv_write_init (self, 0x01, &error);
        g_assert_no_error (error);
        g_assert (init_offset > 0);

        for (i = 0; i < n_bytes_prefixed; i++) {
            expected_tlv_payload_size += 1;
            ret = qmi_message_tlv_write_guint8 (self, 0xFF, &error);
            g_assert_no_error (error);
            g_assert (ret);
        }

        /* Add one of each */
        expected_tlv_payload_size += 8;
        ret = qmi_message_tlv_write_guint64 (self, QMI_ENDIAN_LITTLE, 0x1212121212121212ULL, &error);
        g_assert_no_error (error);
        g_assert (ret);

        expected_tlv_payload_size += 8;
        ret = qmi_message_tlv_write_gint64 (self, QMI_ENDIAN_LITTLE, 0 - 0x1212121212121212LL, &error);
        g_assert_no_error (error);
        g_assert (ret);

        expected_tlv_payload_size += 8;
        ret = qmi_message_tlv_write_guint64 (self, QMI_ENDIAN_BIG, 0x1212121212121212ULL, &error);
        g_assert_no_error (error);
        g_assert (ret);

        expected_tlv_payload_size += 8;
        ret = qmi_message_tlv_write_gint64 (self, QMI_ENDIAN_BIG, 0 - 0x1212121212121212LL, &error);
        g_assert_no_error (error);
        g_assert (ret);

        ret = qmi_message_tlv_write_complete (self, init_offset, &error);
        g_assert_no_error (error);
        g_assert (ret);

        /* Now read */
        init_offset = qmi_message_tlv_read_init (self, 0x01, &tlv_length, &error);
        g_assert_no_error (error);
        g_assert (init_offset > 0);
        g_assert_cmpuint (tlv_length, ==, expected_tlv_payload_size);

        offset = 0;

        for (i = 0; i < n_bytes_prefixed; i++) {
            guint8 aux;

            ret = qmi_message_tlv_read_guint8 (self, init_offset, &offset, &aux, &error);
            g_assert_no_error (error);
            g_assert (ret);
            g_assert_cmpuint (aux, ==, 0xFF);
        }

        ret = qmi_message_tlv_read_guint64 (self, init_offset, &offset, QMI_ENDIAN_LITTLE, &uint64, &error);
        g_assert_no_error (error);
        g_assert (ret);
        g_assert_cmpuint (uint64, ==, 0x1212121212121212ULL);

        ret = qmi_message_tlv_read_gint64 (self, init_offset, &offset, QMI_ENDIAN_LITTLE, &int64, &error);
        g_assert_no_error (error);
        g_assert (ret);
        g_assert_cmpuint (int64, ==, 0 - 0x1212121212121212LL);

        ret = qmi_message_tlv_read_guint64 (self, init_offset, &offset, QMI_ENDIAN_BIG, &uint64, &error);
        g_assert_no_error (error);
        g_assert (ret);
        g_assert_cmpuint (uint64, ==, 0x1212121212121212ULL);

        ret = qmi_message_tlv_read_gint64 (self, init_offset, &offset, QMI_ENDIAN_BIG, &int64, &error);
        g_assert_no_error (error);
        g_assert (ret);
        g_assert_cmpuint (int64, ==, 0 - 0x1212121212121212LL);
    }
}

static void
test_message_tlv_rw_sized (void)
{
    guint sized[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    guint sized_i;

    for (sized_i = 0; sized_i < G_N_ELEMENTS (sized); sized_i++) {
        guint n_bytes_prefixed;

        /* We'll add [0, 1, 2, 3, 4, 5, 6, 7] bytes before the actual N-bit values,
         * so that we check all possible memory alignments. */
        for (n_bytes_prefixed = 0; n_bytes_prefixed <= sized[sized_i] - 1; n_bytes_prefixed++) {
            g_autoptr(QmiMessage) self = NULL;
            g_autoptr(GError)     error = NULL;
            gboolean              ret;
            gsize                 init_offset;
            guint16               tlv_length = 0;
            gsize                 offset;
            gsize                 expected_tlv_payload_size = 0;
            guint64               uint64;
            guint                 i;
            guint64               value;
            guint64               tmp;

            value = 0x1212121212121212ULL;
            tmp = 0xFF;
            for (i = 1; i < sized[sized_i]; i++) {
                tmp <<= 8;
                tmp |= 0xFF;
            }
            value &= tmp;

            self = qmi_message_new (QMI_SERVICE_DMS, 0x01, 0x02, 0xFFFF);

            init_offset = qmi_message_tlv_write_init (self, 0x01, &error);
            g_assert_no_error (error);
            g_assert (init_offset > 0);

            for (i = 0; i < n_bytes_prefixed; i++) {
                expected_tlv_payload_size += 1;
                ret = qmi_message_tlv_write_guint8 (self, 0xFF, &error);
                g_assert_no_error (error);
                g_assert (ret);
            }

            /* Add one of each */
            expected_tlv_payload_size += sized[sized_i];
            ret = qmi_message_tlv_write_sized_guint (self, sized[sized_i], QMI_ENDIAN_LITTLE, value, &error);
            g_assert_no_error (error);
            g_assert (ret);

            expected_tlv_payload_size += sized[sized_i];
            ret = qmi_message_tlv_write_sized_guint (self, sized[sized_i], QMI_ENDIAN_BIG, value, &error);
            g_assert_no_error (error);
            g_assert (ret);

            ret = qmi_message_tlv_write_complete (self, init_offset, &error);
            g_assert_no_error (error);
            g_assert (ret);

            /* Now read */
            init_offset = qmi_message_tlv_read_init (self, 0x01, &tlv_length, &error);
            g_assert_no_error (error);
            g_assert (init_offset > 0);
            g_assert_cmpuint (tlv_length, ==, expected_tlv_payload_size);

            offset = 0;

            for (i = 0; i < n_bytes_prefixed; i++) {
                guint8 aux;

                ret = qmi_message_tlv_read_guint8 (self, init_offset, &offset, &aux, &error);
                g_assert_no_error (error);
                g_assert (ret);
                g_assert_cmpuint (aux, ==, 0xFF);
            }

            qmi_message_tlv_read_sized_guint (self, init_offset, &offset, sized[sized_i], QMI_ENDIAN_LITTLE, &uint64, &error);
            g_assert_cmpuint (uint64, ==, value);

            qmi_message_tlv_read_sized_guint (self, init_offset, &offset, sized[sized_i], QMI_ENDIAN_BIG, &uint64, &error);
            g_assert_cmpuint (uint64, ==, value);
        }
    }
}

static void
test_message_tlv_rw_strings (void)
{
    g_autoptr(QmiMessage)  self = NULL;
    g_autoptr(GError)      error = NULL;
    gboolean               ret;
    gsize                  init_offset;
    guint16                tlv_length = 0;
    gsize                  offset;
    gsize                  expected_tlv_payload_size = 0;
    gchar                 *str;
    gchar                  fixed_str[5];

    self = qmi_message_new (QMI_SERVICE_DMS, 0x01, 0x02, 0xFFFF);

    init_offset = qmi_message_tlv_write_init (self, 0x01, &error);
    g_assert_no_error (error);
    g_assert (init_offset > 0);

    /* FIXED SIZE */
    expected_tlv_payload_size += 4;
    ret = qmi_message_tlv_write_string (self, 0, "abcd", -1, &error);
    g_assert_no_error (error);
    g_assert (ret);

    /* 1-BYTE PREFIX */
    expected_tlv_payload_size += 5;
    ret = qmi_message_tlv_write_string (self, 1, "abcd", -1, &error);
    g_assert_no_error (error);
    g_assert (ret);

    /* 2-BYTE PREFIX */
    expected_tlv_payload_size += 6;
    ret = qmi_message_tlv_write_string (self, 2, "abcd", -1, &error);
    g_assert_no_error (error);
    g_assert (ret);

    /* FIXED SIZE */
    expected_tlv_payload_size += 0;
    ret = qmi_message_tlv_write_string (self, 0, "", -1, &error);
    g_assert_no_error (error);
    g_assert (ret);

    /* 1-BYTE PREFIX */
    expected_tlv_payload_size += 1;
    ret = qmi_message_tlv_write_string (self, 1, "", -1, &error);
    g_assert_no_error (error);
    g_assert (ret);

    /* 2-BYTE PREFIX */
    expected_tlv_payload_size += 2;
    ret = qmi_message_tlv_write_string (self, 2, "", -1, &error);
    g_assert_no_error (error);
    g_assert (ret);

    /* 0-BYTE PREFIX (same as fixed size, but LAST in TLV) */
    expected_tlv_payload_size += 4;
    ret = qmi_message_tlv_write_string (self, 0, "abcd", -1, &error);
    g_assert_no_error (error);
    g_assert (ret);

    ret = qmi_message_tlv_write_complete (self, init_offset, &error);
    g_assert_no_error (error);
    g_assert (ret);

    /* Now read */
    init_offset = qmi_message_tlv_read_init (self, 0x01, &tlv_length, &error);
    g_assert_no_error (error);
    g_assert (init_offset > 0);
    g_assert_cmpuint (tlv_length, ==, expected_tlv_payload_size);

    offset = 0;

    ret = qmi_message_tlv_read_fixed_size_string (self, init_offset, &offset, 4, fixed_str, &error);
    g_assert_no_error (error);
    g_assert (ret);
    fixed_str[4] = '\0';
    g_assert_cmpstr (fixed_str, ==, "abcd");

    ret = qmi_message_tlv_read_string (self, init_offset, &offset, 1, 0, &str, &error);
    g_assert_no_error (error);
    g_assert (ret);
    g_assert_cmpstr (str, ==, "abcd");
    g_free (str);

    ret = qmi_message_tlv_read_string (self, init_offset, &offset, 2, 0, &str, &error);
    g_assert_no_error (error);
    g_assert (ret);
    g_assert_cmpstr (str, ==, "abcd");
    g_free (str);

    fixed_str[0] = 'z';
    ret = qmi_message_tlv_read_fixed_size_string (self, init_offset, &offset, 0, fixed_str, &error);
    g_assert_no_error (error);
    g_assert (ret);
    g_assert_cmpuint (fixed_str[0], ==, 'z');

    ret = qmi_message_tlv_read_string (self, init_offset, &offset, 1, 0, &str, &error);
    g_assert_no_error (error);
    g_assert (ret);
    g_assert_cmpstr (str, ==, "");
    g_free (str);

    ret = qmi_message_tlv_read_string (self, init_offset, &offset, 2, 0, &str, &error);
    g_assert_no_error (error);
    g_assert (ret);
    g_assert_cmpstr (str, ==, "");
    g_free (str);

    ret = qmi_message_tlv_read_string (self, init_offset, &offset, 0, 0, &str, &error);
    g_assert_no_error (error);
    g_assert (ret);
    g_assert_cmpstr (str, ==, "abcd");
    g_free (str);

    /* There's no other string added, but we can try to read a new one without
     * size prefix, and we should get a 0-length string */
    ret = qmi_message_tlv_read_string (self, init_offset, &offset, 0, 0, &str, &error);
    g_assert_no_error (error);
    g_assert (ret);
    g_assert_cmpstr (str, ==, "");
    g_free (str);
}

static void
test_message_tlv_rw_mixed (void)
{
    g_autoptr(QmiMessage) self = NULL;
    g_autoptr(GError)     error = NULL;
    gboolean              ret;
    gsize                 init_offset;
    guint16               tlv_length = 0;
    gsize                 offset;
    gsize                 expected_tlv_payload_size = 0;
    guint8                uint8;
    gint8                 int8;
    guint16               uint16;
    gint16                int16;
    guint32               uint32;
    gint32                int32;
    guint64               uint64;
    gint64                int64;

    self = qmi_message_new (QMI_SERVICE_DMS, 0x01, 0x02, 0xFFFF);

    init_offset = qmi_message_tlv_write_init (self, 0x01, &error);
    g_assert_no_error (error);
    g_assert (init_offset > 0);

    /* Add one of each */
    expected_tlv_payload_size += 1;
    ret = qmi_message_tlv_write_guint8 (self, 0x12, &error);
    g_assert_no_error (error);
    g_assert (ret);

    expected_tlv_payload_size += 1;
    ret = qmi_message_tlv_write_gint8 (self, 0 - 0x12, &error);
    g_assert_no_error (error);
    g_assert (ret);

    expected_tlv_payload_size += 2;
    ret = qmi_message_tlv_write_guint16 (self, QMI_ENDIAN_LITTLE, 0x1212, &error);
    g_assert_no_error (error);
    g_assert (ret);

    expected_tlv_payload_size += 2;
    ret = qmi_message_tlv_write_gint16 (self, QMI_ENDIAN_LITTLE, 0 - 0x1212, &error);
    g_assert_no_error (error);
    g_assert (ret);

    expected_tlv_payload_size += 4;
    ret = qmi_message_tlv_write_guint32 (self, QMI_ENDIAN_LITTLE, 0x12121212, &error);
    g_assert_no_error (error);
    g_assert (ret);

    expected_tlv_payload_size += 4;
    ret = qmi_message_tlv_write_gint32 (self, QMI_ENDIAN_LITTLE, 0 - 0x12121212, &error);
    g_assert_no_error (error);
    g_assert (ret);

    expected_tlv_payload_size += 8;
    ret = qmi_message_tlv_write_guint64 (self, QMI_ENDIAN_LITTLE, 0x1212121212121212ULL, &error);
    g_assert_no_error (error);
    g_assert (ret);

    expected_tlv_payload_size += 8;
    ret = qmi_message_tlv_write_gint64 (self, QMI_ENDIAN_LITTLE, 0 - 0x1212121212121212LL, &error);
    g_assert_no_error (error);
    g_assert (ret);

    expected_tlv_payload_size += 4;
    ret = qmi_message_tlv_write_string (self, 0, "abcd", -1, &error);
    g_assert_no_error (error);
    g_assert (ret);

    expected_tlv_payload_size += 5;
    ret = qmi_message_tlv_write_string (self, 1, "abcd", -1, &error);
    g_assert_no_error (error);
    g_assert (ret);

    expected_tlv_payload_size += 6;
    ret = qmi_message_tlv_write_string (self, 2, "abcd", -1, &error);
    g_assert_no_error (error);
    g_assert (ret);

    ret = qmi_message_tlv_write_complete (self, init_offset, &error);
    g_assert_no_error (error);
    g_assert (ret);

    /* Now read */
    init_offset = qmi_message_tlv_read_init (self, 0x01, &tlv_length, &error);
    g_assert_no_error (error);
    g_assert (init_offset > 0);
    g_assert_cmpuint (tlv_length, ==, expected_tlv_payload_size);

    offset = 0;

    ret = qmi_message_tlv_read_guint8 (self, init_offset, &offset, &uint8, &error);
    g_assert_no_error (error);
    g_assert (ret);
    g_assert_cmpuint (uint8, ==, 0x12);

    ret = qmi_message_tlv_read_gint8 (self, init_offset, &offset, &int8, &error);
    g_assert_no_error (error);
    g_assert (ret);
    g_assert_cmpuint (int8, ==, 0 - 0x12);

    ret = qmi_message_tlv_read_guint16 (self, init_offset, &offset, QMI_ENDIAN_LITTLE, &uint16, &error);
    g_assert_no_error (error);
    g_assert (ret);
    g_assert_cmpuint (uint16, ==, 0x1212);

    ret = qmi_message_tlv_read_gint16 (self, init_offset, &offset, QMI_ENDIAN_LITTLE, &int16, &error);
    g_assert_no_error (error);
    g_assert (ret);
    g_assert_cmpuint (int16, ==, 0 - 0x1212);

    ret = qmi_message_tlv_read_guint32 (self, init_offset, &offset, QMI_ENDIAN_LITTLE, &uint32, &error);
    g_assert_no_error (error);
    g_assert (ret);
    g_assert_cmpuint (uint32, ==, 0x12121212);

    ret = qmi_message_tlv_read_gint32 (self, init_offset, &offset, QMI_ENDIAN_LITTLE, &int32, &error);
    g_assert_no_error (error);
    g_assert (ret);
    g_assert_cmpuint (int32, ==, 0 - 0x12121212);

    ret = qmi_message_tlv_read_guint64 (self, init_offset, &offset, QMI_ENDIAN_LITTLE, &uint64, &error);
    g_assert_no_error (error);
    g_assert (ret);
    g_assert_cmpuint (uint64, ==, 0x1212121212121212ULL);

    ret = qmi_message_tlv_read_gint64 (self, init_offset, &offset, QMI_ENDIAN_LITTLE, &int64, &error);
    g_assert_no_error (error);
    g_assert (ret);
    g_assert_cmpuint (int64, ==, 0 - 0x1212121212121212LL);
}

static void
test_message_tlv_write_overflow (void)
{
    g_autoptr(QmiMessage) self = NULL;
    g_autoptr(GError)     error = NULL;
    gboolean              ret;
    gsize                 init_offset;

    self = qmi_message_new (QMI_SERVICE_DMS, 0x01, 0x02, 0xFFFF);

    init_offset = qmi_message_tlv_write_init (self, 0x01, &error);
    g_assert_no_error (error);
    g_assert (init_offset > 0);

    /* Add guint8 values until we get to the maximum message size */
    while (qmi_message_get_length (self) < G_MAXUINT16) {
        ret = qmi_message_tlv_write_guint8 (self, 0x12, &error);
        g_assert_no_error (error);
        g_assert (ret);
    }

    /* Message is max size now, don't allow more variables */

    ret = qmi_message_tlv_write_guint8 (self, 0x12, &error);
    g_assert_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_TLV_TOO_LONG);
    g_assert (!ret);
    g_clear_error (&error);

    ret = qmi_message_tlv_write_gint8 (self, 0 - 0x12, &error);
    g_assert_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_TLV_TOO_LONG);
    g_assert (!ret);
    g_clear_error (&error);

    ret = qmi_message_tlv_write_guint16 (self, QMI_ENDIAN_LITTLE, 0x1212, &error);
    g_assert_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_TLV_TOO_LONG);
    g_assert (!ret);
    g_clear_error (&error);

    ret = qmi_message_tlv_write_gint16 (self, QMI_ENDIAN_LITTLE, 0 - 0x1212, &error);
    g_assert_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_TLV_TOO_LONG);
    g_assert (!ret);
    g_clear_error (&error);

    ret = qmi_message_tlv_write_guint32 (self, QMI_ENDIAN_LITTLE, 0x12121212, &error);
    g_assert_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_TLV_TOO_LONG);
    g_assert (!ret);
    g_clear_error (&error);

    ret = qmi_message_tlv_write_gint32 (self, QMI_ENDIAN_LITTLE, 0 - 0x12121212, &error);
    g_assert_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_TLV_TOO_LONG);
    g_assert (!ret);
    g_clear_error (&error);

    ret = qmi_message_tlv_write_guint64 (self, QMI_ENDIAN_LITTLE, 0x1212121212121212ULL, &error);
    g_assert_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_TLV_TOO_LONG);
    g_assert (!ret);
    g_clear_error (&error);

    ret = qmi_message_tlv_write_gint64 (self, QMI_ENDIAN_LITTLE, 0 - 0x1212121212121212LL, &error);
    g_assert_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_TLV_TOO_LONG);
    g_assert (!ret);
    g_clear_error (&error);

    ret = qmi_message_tlv_write_sized_guint (self, 1, QMI_ENDIAN_LITTLE, 0x12, &error);
    g_assert_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_TLV_TOO_LONG);
    g_assert (!ret);
    g_clear_error (&error);

    ret = qmi_message_tlv_write_string (self, 0, "a", -1, &error);
    g_assert_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_TLV_TOO_LONG);
    g_assert (!ret);
    g_clear_error (&error);

    ret = qmi_message_tlv_write_string (self, 1, "a", -1, &error);
    g_assert_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_TLV_TOO_LONG);
    g_assert (!ret);
    g_clear_error (&error);

    ret = qmi_message_tlv_write_string (self, 2, "a", -1, &error);
    g_assert_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_TLV_TOO_LONG);
    g_assert (!ret);
    g_clear_error (&error);

    /* Writing an empty string with a 0-size prefix is actually valid :) */
    ret = qmi_message_tlv_write_string (self, 0, "", -1, &error);
    g_assert_no_error (error);
    g_assert (ret);

    ret = qmi_message_tlv_write_complete (self, init_offset, &error);
    g_assert_no_error (error);
    g_assert (ret);

    /* Message is max size now, don't allow more TLVs */
    init_offset = qmi_message_tlv_write_init (self, 0x02, &error);
    g_assert_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_TLV_TOO_LONG);
    g_assert (init_offset == 0);
    g_clear_error (&error);
}

static void
test_message_tlv_read_overflow_message (void)
{
    g_autoptr(QmiMessage)  self = NULL;
    g_autoptr(GError)      error = NULL;
    gboolean               ret;
    gsize                  init_offset;
    guint16                tlv_length = 0;
    gsize                  offset;
    gchar                 *str;
    guint8                 uint8;
    gint8                  int8;
    guint16                uint16;
    gint16                 int16;
    guint32                uint32;
    gint32                 int32;
    guint64                uint64;
    gint64                 int64;

    self = qmi_message_new (QMI_SERVICE_DMS, 0x01, 0x02, 0xFFFF);
    init_offset = qmi_message_tlv_write_init (self, 0x01, &error);
    g_assert_no_error (error);
    g_assert (init_offset > 0);
    /* Create a 1-byte size prefixed string manually, with a wrong length (out of message) */
    ret = qmi_message_tlv_write_guint8 (self, 5, &error);
    g_assert_no_error (error);
    g_assert (ret);
    ret = qmi_message_tlv_write_string (self, 0, "abcd", -1, &error);
    g_assert_no_error (error);
    g_assert (ret);
    ret = qmi_message_tlv_write_complete (self, init_offset, &error);
    g_assert_no_error (error);
    g_assert (ret);
    /* Now read */
    init_offset = qmi_message_tlv_read_init (self, 0x01, &tlv_length, &error);
    g_assert_no_error (error);
    g_assert (init_offset > 0);
    offset = 0;
    ret = qmi_message_tlv_read_string (self, init_offset, &offset, 1, 0, &str, &error);
    g_assert_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_TLV_TOO_LONG);
    g_assert (!ret);
    g_clear_error (&error);
    g_clear_pointer (&self, qmi_message_unref);

    self = qmi_message_new (QMI_SERVICE_DMS, 0x01, 0x02, 0xFFFF);
    init_offset = qmi_message_tlv_write_init (self, 0x01, &error);
    g_assert_no_error (error);
    g_assert (init_offset > 0);
    ret = qmi_message_tlv_write_string (self, 1, "abcd", -1, &error);
    g_assert_no_error (error);
    g_assert (ret);
    ret = qmi_message_tlv_write_complete (self, init_offset, &error);
    g_assert_no_error (error);
    g_assert (ret);
    /* Now read */
    init_offset = qmi_message_tlv_read_init (self, 0x01, &tlv_length, &error);
    g_assert_no_error (error);
    g_assert (init_offset > 0);
    offset = 0;
    ret = qmi_message_tlv_read_string (self, init_offset, &offset, 1, 0, &str, &error);
    g_assert_no_error (error);
    g_assert (ret);
    g_assert_cmpstr (str, ==, "abcd");
    g_free (str);
    /* Reading more will fail */
    ret = qmi_message_tlv_read_string (self, init_offset, &offset, 1, 0, &str, &error);
    g_assert_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_TLV_TOO_LONG);
    g_assert (!ret);
    g_clear_error (&error);
    ret = qmi_message_tlv_read_string (self, init_offset, &offset, 2, 0, &str, &error);
    g_assert_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_TLV_TOO_LONG);
    g_assert (!ret);
    g_clear_error (&error);
    ret = qmi_message_tlv_read_guint8 (self, init_offset, &offset, &uint8, &error);
    g_assert_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_TLV_TOO_LONG);
    g_assert (!ret);
    g_clear_error (&error);
    ret = qmi_message_tlv_read_gint8 (self, init_offset, &offset, &int8, &error);
    g_assert_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_TLV_TOO_LONG);
    g_assert (!ret);
    g_clear_error (&error);
    ret = qmi_message_tlv_read_guint16 (self, init_offset, &offset, QMI_ENDIAN_LITTLE, &uint16, &error);
    g_assert_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_TLV_TOO_LONG);
    g_assert (!ret);
    g_clear_error (&error);
    ret = qmi_message_tlv_read_gint16 (self, init_offset, &offset, QMI_ENDIAN_LITTLE, &int16, &error);
    g_assert_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_TLV_TOO_LONG);
    g_assert (!ret);
    g_clear_error (&error);
    ret = qmi_message_tlv_read_guint32 (self, init_offset, &offset, QMI_ENDIAN_LITTLE, &uint32, &error);
    g_assert_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_TLV_TOO_LONG);
    g_assert (!ret);
    g_clear_error (&error);
    ret = qmi_message_tlv_read_gint32 (self, init_offset, &offset, QMI_ENDIAN_LITTLE, &int32, &error);
    g_assert_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_TLV_TOO_LONG);
    g_assert (!ret);
    g_clear_error (&error);
    ret = qmi_message_tlv_read_guint64 (self, init_offset, &offset, QMI_ENDIAN_LITTLE, &uint64, &error);
    g_assert_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_TLV_TOO_LONG);
    g_assert (!ret);
    g_clear_error (&error);
    ret = qmi_message_tlv_read_gint64 (self, init_offset, &offset, QMI_ENDIAN_LITTLE, &int64, &error);
    g_assert_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_TLV_TOO_LONG);
    g_assert (!ret);
    g_clear_error (&error);
    g_clear_pointer (&self, qmi_message_unref);
}

static void
test_message_tlv_read_overflow_tlv (void)
{
    g_autoptr(QmiMessage)  self = NULL;
    g_autoptr(GError)      error = NULL;
    gboolean               ret;
    gsize                  init_offset;
    guint16                tlv_length = 0;
    gsize                  offset;
    gchar                 *str;
    guint8                 uint8;
    gint8                  int8;
    guint16                uint16;
    gint16                 int16;
    guint32                uint32;
    gint32                 int32;
    guint64                uint64;
    gint64                 int64;

    self = qmi_message_new (QMI_SERVICE_DMS, 0x01, 0x02, 0xFFFF);
    init_offset = qmi_message_tlv_write_init (self, 0x01, &error);
    g_assert_no_error (error);
    g_assert (init_offset > 0);
    /* Create a 1-byte size prefixed string manually, with a wrong length (out of tlv) */
    ret = qmi_message_tlv_write_guint8 (self, 5, &error);
    g_assert_no_error (error);
    g_assert (ret);
    ret = qmi_message_tlv_write_string (self, 0, "abcd", -1, &error);
    g_assert_no_error (error);
    g_assert (ret);
    ret = qmi_message_tlv_write_complete (self, init_offset, &error);
    g_assert_no_error (error);
    g_assert (ret);
    init_offset = qmi_message_tlv_write_init (self, 0x02, &error);
    g_assert_no_error (error);
    g_assert (init_offset > 0);
    ret = qmi_message_tlv_write_string (self, 1, "efgh", -1, &error);
    g_assert_no_error (error);
    g_assert (ret);
    ret = qmi_message_tlv_write_complete (self, init_offset, &error);
    g_assert_no_error (error);
    g_assert (ret);
    /* Now read */
    init_offset = qmi_message_tlv_read_init (self, 0x01, &tlv_length, &error);
    g_assert_no_error (error);
    g_assert (init_offset > 0);
    offset = 0;
    ret = qmi_message_tlv_read_string (self, init_offset, &offset, 1, 0, &str, &error);
    g_assert_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_TLV_TOO_LONG);
    g_assert (!ret);
    g_clear_error (&error);
    g_clear_pointer (&self, qmi_message_unref);

    self = qmi_message_new (QMI_SERVICE_DMS, 0x01, 0x02, 0xFFFF);
    init_offset = qmi_message_tlv_write_init (self, 0x01, &error);
    g_assert_no_error (error);
    g_assert (init_offset > 0);
    ret = qmi_message_tlv_write_string (self, 1, "abcd", -1, &error);
    g_assert_no_error (error);
    g_assert (ret);
    ret = qmi_message_tlv_write_complete (self, init_offset, &error);
    g_assert_no_error (error);
    g_assert (ret);
    init_offset = qmi_message_tlv_write_init (self, 0x02, &error);
    g_assert_no_error (error);
    g_assert (init_offset > 0);
    ret = qmi_message_tlv_write_string (self, 1, "efgh", -1, &error);
    g_assert_no_error (error);
    g_assert (ret);
    ret = qmi_message_tlv_write_complete (self, init_offset, &error);
    g_assert_no_error (error);
    g_assert (ret);
    /* Now read */
    init_offset = qmi_message_tlv_read_init (self, 0x01, &tlv_length, &error);
    g_assert_no_error (error);
    g_assert (init_offset > 0);
    offset = 0;
    ret = qmi_message_tlv_read_string (self, init_offset, &offset, 1, 0, &str, &error);
    g_assert_no_error (error);
    g_assert (ret);
    g_assert_cmpstr (str, ==, "abcd");
    g_free (str);
    /* Reading more will fail */
    ret = qmi_message_tlv_read_string (self, init_offset, &offset, 1, 0, &str, &error);
    g_assert_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_TLV_TOO_LONG);
    g_assert (!ret);
    g_clear_error (&error);
    ret = qmi_message_tlv_read_string (self, init_offset, &offset, 2, 0, &str, &error);
    g_assert_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_TLV_TOO_LONG);
    g_assert (!ret);
    g_clear_error (&error);
    ret = qmi_message_tlv_read_guint8 (self, init_offset, &offset, &uint8, &error);
    g_assert_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_TLV_TOO_LONG);
    g_assert (!ret);
    g_clear_error (&error);
    ret = qmi_message_tlv_read_gint8 (self, init_offset, &offset, &int8, &error);
    g_assert_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_TLV_TOO_LONG);
    g_assert (!ret);
    g_clear_error (&error);
    ret = qmi_message_tlv_read_guint16 (self, init_offset, &offset, QMI_ENDIAN_LITTLE, &uint16, &error);
    g_assert_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_TLV_TOO_LONG);
    g_assert (!ret);
    g_clear_error (&error);
    ret = qmi_message_tlv_read_gint16 (self, init_offset, &offset, QMI_ENDIAN_LITTLE, &int16, &error);
    g_assert_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_TLV_TOO_LONG);
    g_assert (!ret);
    g_clear_error (&error);
    ret = qmi_message_tlv_read_guint32 (self, init_offset, &offset, QMI_ENDIAN_LITTLE, &uint32, &error);
    g_assert_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_TLV_TOO_LONG);
    g_assert (!ret);
    g_clear_error (&error);
    ret = qmi_message_tlv_read_gint32 (self, init_offset, &offset, QMI_ENDIAN_LITTLE, &int32, &error);
    g_assert_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_TLV_TOO_LONG);
    g_assert (!ret);
    g_clear_error (&error);
    ret = qmi_message_tlv_read_guint64 (self, init_offset, &offset, QMI_ENDIAN_LITTLE, &uint64, &error);
    g_assert_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_TLV_TOO_LONG);
    g_assert (!ret);
    g_clear_error (&error);
    ret = qmi_message_tlv_read_gint64 (self, init_offset, &offset, QMI_ENDIAN_LITTLE, &int64, &error);
    g_assert_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_TLV_TOO_LONG);
    g_assert (!ret);
    g_clear_error (&error);
    g_clear_pointer (&self, qmi_message_unref);
}

/*****************************************************************************/

static void
test_message_set_transaction_id_ctl (void)
{
    g_autoptr(GByteArray) buffer = NULL;
    g_autoptr(QmiMessage) message = NULL;
    g_autoptr(GError)     error = NULL;

    guint8 ctl_message[] = {
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
}

static void
test_message_set_transaction_id_services (void)
{
    g_autoptr(GByteArray) buffer = NULL;
    g_autoptr(QmiMessage) message = NULL;
    g_autoptr(GError)     error = NULL;

    guint8 dms_message[] = {
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
}

/*****************************************************************************/

int main (int argc, char **argv)
{
    g_test_init (&argc, &argv, NULL);

    g_test_add_func ("/libqmi-glib/message/parse/short",                 test_message_parse_short);
    g_test_add_func ("/libqmi-glib/message/parse/complete",              test_message_parse_complete);
    g_test_add_func ("/libqmi-glib/message/parse/complete-and-short",    test_message_parse_complete_and_short);
    g_test_add_func ("/libqmi-glib/message/parse/complete-and-complete", test_message_parse_complete_and_complete);
    g_test_add_func ("/libqmi-glib/message/parse/wrong-tlv",             test_message_parse_wrong_tlv);
#if defined HAVE_QMI_INDICATION_PDS_EVENT_REPORT
    g_test_add_func ("/libqmi-glib/message/parse/missing-size",          test_message_parse_missing_size);
#endif
#if defined HAVE_QMI_INDICATION_LOC_NMEA
    g_test_add_func ("/libqmi-glib/message/parse/string-with-crlf",      test_message_parse_string_with_crlf);
#endif
#if defined HAVE_QMI_MESSAGE_DMS_SWI_GET_CURRENT_FIRMWARE
    g_test_add_func ("/libqmi-glib/message/parse/string-with-trailing-nul", test_message_parse_string_with_trailing_nul);
#endif
#if defined HAVE_QMI_MESSAGE_DMS_GET_MODEL
    g_test_add_func ("/libqmi-glib/message/parse/string-with-trailing-tab", test_message_parse_string_with_trailing_tab);
#endif
#if defined HAVE_QMI_MESSAGE_NAS_SWI_GET_STATUS
    g_test_add_func ("/libqmi-glib/message/parse/signed-int", test_message_parse_signed_int);
#endif

    g_test_add_func ("/libqmi-glib/message/new/request",           test_message_new_request);
    g_test_add_func ("/libqmi-glib/message/new/request-from-data", test_message_new_request_from_data);
    g_test_add_func ("/libqmi-glib/message/new/response/ok",       test_message_new_response_ok);
    g_test_add_func ("/libqmi-glib/message/new/response/error",    test_message_new_response_error);

    g_test_add_func ("/libqmi-glib/message/tlv-write/empty",           test_message_tlv_write_empty);
    g_test_add_func ("/libqmi-glib/message/tlv-write/reset",           test_message_tlv_write_reset);
    g_test_add_func ("/libqmi-glib/message/tlv-rw/8",                  test_message_tlv_rw_8);
    g_test_add_func ("/libqmi-glib/message/tlv-rw/16",                 test_message_tlv_rw_16);
    g_test_add_func ("/libqmi-glib/message/tlv-rw/32",                 test_message_tlv_rw_32);
    g_test_add_func ("/libqmi-glib/message/tlv-rw/64",                 test_message_tlv_rw_64);
    g_test_add_func ("/libqmi-glib/message/tlv-rw/sized",              test_message_tlv_rw_sized);
    g_test_add_func ("/libqmi-glib/message/tlv-rw/strings",            test_message_tlv_rw_strings);
    g_test_add_func ("/libqmi-glib/message/tlv-rw/mixed",              test_message_tlv_rw_mixed);
    g_test_add_func ("/libqmi-glib/message/tlv-write/overflow",        test_message_tlv_write_overflow);
    g_test_add_func ("/libqmi-glib/message/tlv-read/overflow-message", test_message_tlv_read_overflow_message);
    g_test_add_func ("/libqmi-glib/message/tlv-read/overflow-tlv",     test_message_tlv_read_overflow_tlv);

    g_test_add_func ("/libqmi-glib/message/set-transaction-id/ctl",      test_message_set_transaction_id_ctl);
    g_test_add_func ("/libqmi-glib/message/set-transaction-id/services", test_message_set_transaction_id_services);

    return g_test_run ();
}
