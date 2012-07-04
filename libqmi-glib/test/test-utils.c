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

#include <glib-object.h>
#include <string.h>
#include "qmi-utils.h"

static void
test_utils_uint8 (void)
{
    static guint8 in_buffer[8] = {
        0x0F, 0x50, 0xEB, 0xE2, 0xB6, 0x00, 0x00, 0x00
    };
    guint8 out_buffer[8] = { 0 };

    guint16 in_buffer_size;
    guint8 *in_buffer_walker;
    guint16 out_buffer_size;
    guint8 *out_buffer_walker;

    in_buffer_size = sizeof (in_buffer);
    in_buffer_walker = &in_buffer[0];
    out_buffer_size = sizeof (out_buffer);
    out_buffer_walker = &out_buffer[0];

    while (in_buffer_size) {
        guint8 tmp;

        qmi_utils_read_guint8_from_buffer (&in_buffer_walker, &in_buffer_size, &tmp);
        qmi_utils_write_guint8_to_buffer (&out_buffer_walker, &out_buffer_size, &tmp);
    }

    g_assert_cmpuint (out_buffer_size, ==, 0);
    g_assert (memcmp (in_buffer, out_buffer, sizeof (in_buffer)) == 0);
}

static void
test_utils_int8 (void)
{
    static guint8 in_buffer[8] = {
        0x0F, 0x50, 0xEB, 0xE2, 0xB6, 0x00, 0x00, 0x00
    };
    guint8 out_buffer[8] = { 0 };

    guint16 in_buffer_size;
    guint8 *in_buffer_walker;
    guint16 out_buffer_size;
    guint8 *out_buffer_walker;

    in_buffer_size = sizeof (in_buffer);
    in_buffer_walker = &in_buffer[0];
    out_buffer_size = sizeof (out_buffer);
    out_buffer_walker = &out_buffer[0];

    while (in_buffer_size) {
        gint8 tmp;

        qmi_utils_read_gint8_from_buffer (&in_buffer_walker, &in_buffer_size, &tmp);
        qmi_utils_write_gint8_to_buffer (&out_buffer_walker, &out_buffer_size, &tmp);
    }

    g_assert_cmpuint (out_buffer_size, ==, 0);
    g_assert (memcmp (in_buffer, out_buffer, sizeof (in_buffer)) == 0);
}

static void
test_utils_uint16 (void)
{
    static guint8 in_buffer[8] = {
        0x0F, 0x50, 0xEB, 0xE2, 0xB6, 0x00, 0x00, 0x00
    };
    static guint16 values[4] = {
        0x500F, 0xE2EB, 0x00B6, 0x0000
    };
    guint8 out_buffer[8] = { 0 };

    guint i;
    guint16 in_buffer_size;
    guint8 *in_buffer_walker;
    guint16 out_buffer_size;
    guint8 *out_buffer_walker;

    in_buffer_size = sizeof (in_buffer);
    in_buffer_walker = &in_buffer[0];
    out_buffer_size = sizeof (out_buffer);
    out_buffer_walker = &out_buffer[0];
    i = 0;

    while (in_buffer_size) {
        guint16 tmp;

        qmi_utils_read_guint16_from_buffer (&in_buffer_walker, &in_buffer_size, &tmp);

        g_assert (tmp == values[i++]);

        qmi_utils_write_guint16_to_buffer (&out_buffer_walker, &out_buffer_size, &tmp);
    }

    g_assert_cmpuint (out_buffer_size, ==, 0);
    g_assert (memcmp (in_buffer, out_buffer, sizeof (in_buffer)) == 0);
}

static void
test_utils_int16 (void)
{
    static guint8 in_buffer[8] = {
        0x0F, 0x50, 0xEB, 0xE2, 0xB6, 0x00, 0x00, 0x00
    };
    static gint16 values[4] = {
        0x500F, 0xE2EB, 0x00B6, 0x0000
    };
    guint8 out_buffer[8] = { 0 };

    guint i;
    guint16 in_buffer_size;
    guint8 *in_buffer_walker;
    guint16 out_buffer_size;
    guint8 *out_buffer_walker;

    in_buffer_size = sizeof (in_buffer);
    in_buffer_walker = &in_buffer[0];
    out_buffer_size = sizeof (out_buffer);
    out_buffer_walker = &out_buffer[0];
    i = 0;

    while (in_buffer_size) {
        gint16 tmp;

        qmi_utils_read_gint16_from_buffer (&in_buffer_walker, &in_buffer_size, &tmp);

        g_assert (tmp == values[i++]);

        qmi_utils_write_gint16_to_buffer (&out_buffer_walker, &out_buffer_size, &tmp);
    }

    g_assert_cmpuint (out_buffer_size, ==, 0);
    g_assert (memcmp (in_buffer, out_buffer, sizeof (in_buffer)) == 0);
}

static void
test_utils_uint32 (void)
{
    static guint8 in_buffer[8] = {
        0x0F, 0x50, 0xEB, 0xE2, 0xB6, 0x00, 0x00, 0x00
    };
    static guint32 values[2] = {
        0xE2EB500F, 0x000000B6
    };
    guint8 out_buffer[8] = { 0 };

    guint i;
    guint16 in_buffer_size;
    guint8 *in_buffer_walker;
    guint16 out_buffer_size;
    guint8 *out_buffer_walker;

    in_buffer_size = sizeof (in_buffer);
    in_buffer_walker = &in_buffer[0];
    out_buffer_size = sizeof (out_buffer);
    out_buffer_walker = &out_buffer[0];
    i = 0;

    while (in_buffer_size) {
        guint32 tmp;

        qmi_utils_read_guint32_from_buffer (&in_buffer_walker, &in_buffer_size, &tmp);

        g_assert (tmp == values[i++]);

        qmi_utils_write_guint32_to_buffer (&out_buffer_walker, &out_buffer_size, &tmp);
    }

    g_assert_cmpuint (out_buffer_size, ==, 0);
    g_assert (memcmp (in_buffer, out_buffer, sizeof (in_buffer)) == 0);
}

static void
test_utils_int32 (void)
{
    static guint8 in_buffer[8] = {
        0x0F, 0x50, 0xEB, 0xE2, 0xB6, 0x00, 0x00, 0x00
    };
    static gint32 values[2] = {
        0xE2EB500F, 0x000000B6
    };
    guint8 out_buffer[8] = { 0 };

    guint i;
    guint16 in_buffer_size;
    guint8 *in_buffer_walker;
    guint16 out_buffer_size;
    guint8 *out_buffer_walker;

    in_buffer_size = sizeof (in_buffer);
    in_buffer_walker = &in_buffer[0];
    out_buffer_size = sizeof (out_buffer);
    out_buffer_walker = &out_buffer[0];
    i = 0;

    while (in_buffer_size) {
        gint32 tmp;

        qmi_utils_read_gint32_from_buffer (&in_buffer_walker, &in_buffer_size, &tmp);

        g_assert (tmp == values[i++]);

        qmi_utils_write_gint32_to_buffer (&out_buffer_walker, &out_buffer_size, &tmp);
    }

    g_assert_cmpuint (out_buffer_size, ==, 0);
    g_assert (memcmp (in_buffer, out_buffer, sizeof (in_buffer)) == 0);
}

static void
test_utils_uint64 (void)
{
    static guint8 in_buffer[8] = {
        0x0F, 0x50, 0xEB, 0xE2, 0xB6, 0x00, 0x00, 0x00
    };
    static guint64 values[1] = {
        0x000000B6E2EB500F
    };
    guint8 out_buffer[8] = { 0 };

    guint i;
    guint16 in_buffer_size;
    guint8 *in_buffer_walker;
    guint16 out_buffer_size;
    guint8 *out_buffer_walker;

    in_buffer_size = sizeof (in_buffer);
    in_buffer_walker = &in_buffer[0];
    out_buffer_size = sizeof (out_buffer);
    out_buffer_walker = &out_buffer[0];
    i = 0;

    while (in_buffer_size) {
        guint64 tmp;

        qmi_utils_read_guint64_from_buffer (&in_buffer_walker, &in_buffer_size, &tmp);

        g_assert (tmp == values[i++]);

        qmi_utils_write_guint64_to_buffer (&out_buffer_walker, &out_buffer_size, &tmp);
    }

    g_assert_cmpuint (out_buffer_size, ==, 0);
    g_assert (memcmp (in_buffer, out_buffer, sizeof (in_buffer)) == 0);
}

static void
test_utils_int64 (void)
{
    static guint8 in_buffer[8] = {
        0x0F, 0x50, 0xEB, 0xE2, 0xB6, 0x00, 0x00, 0x00
    };
    static gint64 values[1] = {
        0x000000B6E2EB500F
    };
    guint8 out_buffer[8] = { 0 };

    guint i;
    guint16 in_buffer_size;
    guint8 *in_buffer_walker;
    guint16 out_buffer_size;
    guint8 *out_buffer_walker;

    in_buffer_size = sizeof (in_buffer);
    in_buffer_walker = &in_buffer[0];
    out_buffer_size = sizeof (out_buffer);
    out_buffer_walker = &out_buffer[0];
    i = 0;

    while (in_buffer_size) {
        gint64 tmp;

        qmi_utils_read_gint64_from_buffer (&in_buffer_walker, &in_buffer_size, &tmp);

        g_assert (tmp == values[i++]);

        qmi_utils_write_gint64_to_buffer (&out_buffer_walker, &out_buffer_size, &tmp);
    }

    g_assert_cmpuint (out_buffer_size, ==, 0);
    g_assert (memcmp (in_buffer, out_buffer, sizeof (in_buffer)) == 0);
}

int main (int argc, char **argv)
{
    g_type_init ();
    g_test_init (&argc, &argv, NULL);

    g_test_add_func ("/libqmi-glib/utils/uint8",  test_utils_uint8);
    g_test_add_func ("/libqmi-glib/utils/int8",   test_utils_int8);
    g_test_add_func ("/libqmi-glib/utils/uint16", test_utils_uint16);
    g_test_add_func ("/libqmi-glib/utils/int16",  test_utils_int16);
    g_test_add_func ("/libqmi-glib/utils/uint32", test_utils_uint32);
    g_test_add_func ("/libqmi-glib/utils/int32",  test_utils_int32);
    g_test_add_func ("/libqmi-glib/utils/uint64", test_utils_uint64);
    g_test_add_func ("/libqmi-glib/utils/int64",  test_utils_int64);

    return g_test_run ();
}
