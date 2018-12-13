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

#include "qmi-message.h"
#include "qmi-errors.h"
#include "qmi-error-types.h"
#include "qmi-utils.h"

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
    gchar *str1;
    gchar *str2;

    str1 = __str_hex (mem1, size1, ':');
    str2 = __str_hex (mem2, size2, ':');
    g_assert_cmpstr (str1, ==, str2);
    g_free (str1);
    g_free (str2);
}

/*****************************************************************************/

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

        printable = qmi_message_get_printable_full (message, NULL, "");
#ifdef TEST_PRINT_MESSAGE
        g_print ("\n%s\n", printable);
#endif
        g_free (printable);

        n_messages++;
        qmi_message_unref (message);
    } while (array->len > 0);

    g_assert_cmpuint (n_messages, ==, n_expected_messages);

    g_byte_array_unref (array);
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
test_message_overflow_common (const guint8 *buffer,
                              guint buffer_len)
{
    QmiMessage *message;
    GByteArray *array;
    GError *error = NULL;
    gchar *printable;

    array = g_byte_array_sized_new (buffer_len);
    g_byte_array_append (array, buffer, buffer_len);
    message = qmi_message_new_from_raw (array, &error);
    g_assert_no_error (error);
    g_assert (message);

    printable = qmi_message_get_printable_full (message, NULL, "");
    g_print ("\n%s\n", printable);
    g_assert (strstr (printable, "ERROR: Reading TLV would overflow"));
    g_free (printable);

    g_byte_array_unref (array);
    qmi_message_unref (message);
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

    test_message_overflow_common (buffer, G_N_ELEMENTS (buffer));
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

    test_message_overflow_common (buffer, G_N_ELEMENTS (buffer));
}

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

    QmiMessage *self;
    GError *error = NULL;
    const guint8 *buffer;
    gsize buffer_length = 0;

    self = qmi_message_new (QMI_SERVICE_DMS, 0x01, 0x02, 0xFFFF);
    g_assert (self);

    buffer = qmi_message_get_raw (self, &buffer_length, &error);
    g_assert_no_error (error);
    g_assert (buffer != NULL);
    g_assert_cmpuint (buffer_length, >, 0);

    _g_assert_cmpmem (buffer, buffer_length, expected_buffer, sizeof (expected_buffer));

    qmi_message_unref (self);
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

    QmiMessage *request;
    QmiMessage *response;
    GError *error = NULL;
    const guint8 *buffer;
    gsize buffer_length = 0;

    request = qmi_message_new (QMI_SERVICE_DMS, 0x01, 0x02, 0xFFFF);
    g_assert (request);

    response = qmi_message_response_new (request, QMI_PROTOCOL_ERROR_NONE);
    g_assert (response);

    buffer = qmi_message_get_raw (response, &buffer_length, &error);
    g_assert_no_error (error);
    g_assert (buffer != NULL);
    g_assert_cmpuint (buffer_length, >, 0);

    _g_assert_cmpmem (buffer, buffer_length, expected_buffer, sizeof (expected_buffer));

    qmi_message_unref (request);
    qmi_message_unref (response);
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

    QmiMessage *request;
    QmiMessage *response;
    GError *error = NULL;
    const guint8 *buffer;
    gsize buffer_length = 0;

    request = qmi_message_new (QMI_SERVICE_DMS, 0x01, 0x02, 0xFFFF);
    g_assert (request);

    response = qmi_message_response_new (request, QMI_PROTOCOL_ERROR_INTERNAL);
    g_assert (response);

    buffer = qmi_message_get_raw (response, &buffer_length, &error);
    g_assert_no_error (error);
    g_assert (buffer != NULL);
    g_assert_cmpuint (buffer_length, >, 0);

    _g_assert_cmpmem (buffer, buffer_length, expected_buffer, sizeof (expected_buffer));

    qmi_message_unref (request);
    qmi_message_unref (response);
}

/*****************************************************************************/

static void
test_message_tlv_write_empty (void)
{
    QmiMessage *self;
    GError *error = NULL;
    gboolean ret;
    gsize init_offset;

    self = qmi_message_new (QMI_SERVICE_DMS, 0x01, 0x02, 0xFFFF);

    init_offset = qmi_message_tlv_write_init (self, 0x01, &error);
    g_assert_no_error (error);
    g_assert (init_offset > 0);

    ret = qmi_message_tlv_write_complete (self, init_offset, &error);
    g_assert_no_error (error);
    g_assert (ret);

    qmi_message_unref (self);
}

static void
test_message_tlv_write_reset (void)
{
    QmiMessage *self;
    GError *error = NULL;
    gboolean ret;
    gsize init_offset;
    gsize previous_size;

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

    qmi_message_unref (self);
}

static void
test_message_tlv_rw_8 (void)
{
    QmiMessage *self;
    GError *error = NULL;
    gboolean ret;
    gsize init_offset;
    gsize expected_tlv_payload_size = 0;
    guint16 tlv_length = 0;
    gsize offset;
    guint8 uint8;
    gint8 int8;

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

    qmi_message_unref (self);
}

static void
test_message_tlv_rw_16 (void)
{
    guint n_bytes_prefixed;

    /* We'll add [0 or 1] bytes before the actual 16-bit values, so that we
     * check all possible memory alignments. */
    for (n_bytes_prefixed = 0; n_bytes_prefixed <= 1; n_bytes_prefixed++) {
        QmiMessage *self;
        GError *error = NULL;
        gboolean ret;
        gsize init_offset;
        guint16 tlv_length = 0;
        gsize offset;
        gsize expected_tlv_payload_size = 0;
        guint16 uint16;
        gint16 int16;
        guint i;

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

        qmi_message_unref (self);
    }
}

static void
test_message_tlv_rw_32 (void)
{
    guint n_bytes_prefixed;

    /* We'll add [0, 1, 2, 3] bytes before the actual 32-bit values, so that we
     * check all possible memory alignments. */
    for (n_bytes_prefixed = 0; n_bytes_prefixed <= 3; n_bytes_prefixed++) {
        QmiMessage *self;
        GError *error = NULL;
        gboolean ret;
        gsize init_offset;
        guint16 tlv_length = 0;
        gsize offset;
        gsize expected_tlv_payload_size = 0;
        guint32 uint32;
        gint32 int32;
        guint i;

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

        qmi_message_unref (self);
    }
}

static void
test_message_tlv_rw_64 (void)
{
    guint n_bytes_prefixed;

    /* We'll add [0, 1, 2, 3, 4, 5, 6, 7] bytes before the actual 64-bit values,
     * so that we check all possible memory alignments. */
    for (n_bytes_prefixed = 0; n_bytes_prefixed <= 7; n_bytes_prefixed++) {
        QmiMessage *self;
        GError *error = NULL;
        gboolean ret;
        gsize init_offset;
        guint16 tlv_length = 0;
        gsize offset;
        gsize expected_tlv_payload_size = 0;
        guint64 uint64;
        gint64 int64;
        guint i;

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

        qmi_message_unref (self);
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
            QmiMessage *self;
            GError *error = NULL;
            gboolean ret;
            gsize init_offset;
            guint16 tlv_length = 0;
            gsize offset;
            gsize expected_tlv_payload_size = 0;
            guint64 uint64;
            guint i;
            guint64 value;
            guint64 tmp;

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

            qmi_message_unref (self);
        }
    }
}

static void
test_message_tlv_rw_strings (void)
{
    QmiMessage *self;
    GError *error = NULL;
    gboolean ret;
    gsize init_offset;
    guint16 tlv_length = 0;
    gsize offset;
    gsize expected_tlv_payload_size = 0;
    gchar *str;
    gchar fixed_str[5];

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

    qmi_message_unref (self);
}

static void
test_message_tlv_rw_mixed (void)
{
    QmiMessage *self;
    GError *error = NULL;
    gboolean ret;
    gsize init_offset;
    guint16 tlv_length = 0;
    gsize offset;
    gsize expected_tlv_payload_size = 0;
    guint8 uint8;
    gint8  int8;
    guint16 uint16;
    gint16 int16;
    guint32 uint32;
    gint32 int32;
    guint64 uint64;
    gint64 int64;

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

    qmi_message_unref (self);
}

static void
test_message_tlv_write_overflow (void)
{
    QmiMessage *self;
    GError *error = NULL;
    gsize init_offset;
    gboolean ret;

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

    qmi_message_unref (self);
}

static void
test_message_tlv_read_overflow_message (void)
{
    QmiMessage *self;
    GError *error = NULL;
    gsize init_offset;
    guint16 tlv_length = 0;
    gsize offset;
    gboolean ret;
    gchar *str;
    guint8 uint8;
    gint8  int8;
    guint16 uint16;
    gint16 int16;
    guint32 uint32;
    gint32 int32;
    guint64 uint64;
    gint64 int64;

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
    qmi_message_unref (self);

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
    qmi_message_unref (self);
}

static void
test_message_tlv_read_overflow_tlv (void)
{
    QmiMessage *self;
    GError *error = NULL;
    gsize init_offset;
    guint16 tlv_length = 0;
    gsize offset;
    gboolean ret;
    gchar *str;
    guint8 uint8;
    gint8  int8;
    guint16 uint16;
    gint16 int16;
    guint32 uint32;
    gint32 int32;
    guint64 uint64;
    gint64 int64;

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
    qmi_message_unref (self);


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
    qmi_message_unref (self);
}

/*****************************************************************************/

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

/*****************************************************************************/

int main (int argc, char **argv)
{
    g_test_init (&argc, &argv, NULL);

    g_test_add_func ("/libqmi-glib/message/parse/short",                 test_message_parse_short);
    g_test_add_func ("/libqmi-glib/message/parse/complete",              test_message_parse_complete);
    g_test_add_func ("/libqmi-glib/message/parse/complete-and-short",    test_message_parse_complete_and_short);
    g_test_add_func ("/libqmi-glib/message/parse/complete-and-complete", test_message_parse_complete_and_complete);
    g_test_add_func ("/libqmi-glib/message/parse/wrong-tlv",             test_message_parse_wrong_tlv);
    g_test_add_func ("/libqmi-glib/message/parse/missing-size",          test_message_parse_missing_size);

    g_test_add_func ("/libqmi-glib/message/new/request",        test_message_new_request);
    g_test_add_func ("/libqmi-glib/message/new/response/ok",    test_message_new_response_ok);
    g_test_add_func ("/libqmi-glib/message/new/response/error", test_message_new_response_error);

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
