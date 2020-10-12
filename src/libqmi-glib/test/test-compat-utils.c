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
#include "qmi-compat.h"

#ifndef QMI_DISABLE_DEPRECATED

static void
test_utils_uint8 (void)
{
    static const guint8 in_buffer[8] = {
        0x0F, 0x50, 0xEB, 0xE2, 0xB6, 0x00, 0x00, 0x00
    };
    guint8 out_buffer[8] = { 0 };

    guint16 in_buffer_size;
    const guint8 *in_buffer_walker;
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
    static const guint8 in_buffer[8] = {
        0x0F, 0x50, 0xEB, 0xE2, 0xB6, 0x00, 0x00, 0x00
    };
    guint8 out_buffer[8] = { 0 };

    guint16 in_buffer_size;
    const guint8 *in_buffer_walker;
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
test_utils_uint16_le (void)
{
    static const guint8 in_buffer[8] = {
        0x0F, 0x50, 0xEB, 0xE2, 0xB6, 0x00, 0x00, 0x00
    };
    static const guint16 values[4] = {
        0x500F, 0xE2EB, 0x00B6, 0x0000
    };
    guint8 out_buffer[8] = { 0 };

    guint i;
    guint16 in_buffer_size;
    const guint8 *in_buffer_walker;
    guint16 out_buffer_size;
    guint8 *out_buffer_walker;

    in_buffer_size = sizeof (in_buffer);
    in_buffer_walker = &in_buffer[0];
    out_buffer_size = sizeof (out_buffer);
    out_buffer_walker = &out_buffer[0];
    i = 0;

    while (in_buffer_size) {
        guint16 tmp;

        qmi_utils_read_guint16_from_buffer (&in_buffer_walker, &in_buffer_size, QMI_ENDIAN_LITTLE, &tmp);
        g_assert_cmpuint (tmp, ==, values[i++]);
        qmi_utils_write_guint16_to_buffer (&out_buffer_walker, &out_buffer_size, QMI_ENDIAN_LITTLE, &tmp);
    }

    g_assert_cmpuint (out_buffer_size, ==, 0);
    g_assert (memcmp (in_buffer, out_buffer, sizeof (in_buffer)) == 0);
}

static void
test_utils_uint16_be (void)
{
    static const guint8 in_buffer[8] = {
        0x50, 0x0F, 0xE2, 0xEB, 0x00, 0xB6, 0x00, 0x00
    };
    static const guint16 values[4] = {
        0x500F, 0xE2EB, 0x00B6, 0x0000
    };
    guint8 out_buffer[8] = { 0 };

    guint i;
    guint16 in_buffer_size;
    const guint8 *in_buffer_walker;
    guint16 out_buffer_size;
    guint8 *out_buffer_walker;

    in_buffer_size = sizeof (in_buffer);
    in_buffer_walker = &in_buffer[0];
    out_buffer_size = sizeof (out_buffer);
    out_buffer_walker = &out_buffer[0];
    i = 0;

    while (in_buffer_size) {
        guint16 tmp;

        qmi_utils_read_guint16_from_buffer (&in_buffer_walker, &in_buffer_size, QMI_ENDIAN_BIG, &tmp);
        g_assert_cmpuint (tmp, ==, values[i++]);
        qmi_utils_write_guint16_to_buffer (&out_buffer_walker, &out_buffer_size, QMI_ENDIAN_BIG, &tmp);
    }

    g_assert_cmpuint (out_buffer_size, ==, 0);
    g_assert (memcmp (in_buffer, out_buffer, sizeof (in_buffer)) == 0);
}

static void
test_utils_int16_le (void)
{
    static const guint8 in_buffer[8] = {
        0x0F, 0x50, 0xEB, 0xE2, 0xB6, 0x00, 0x00, 0x00
    };
    static const gint16 values[4] = {
        0x500F, 0xE2EB, 0x00B6, 0x0000
    };
    guint8 out_buffer[8] = { 0 };

    guint i;
    guint16 in_buffer_size;
    const guint8 *in_buffer_walker;
    guint16 out_buffer_size;
    guint8 *out_buffer_walker;

    in_buffer_size = sizeof (in_buffer);
    in_buffer_walker = &in_buffer[0];
    out_buffer_size = sizeof (out_buffer);
    out_buffer_walker = &out_buffer[0];
    i = 0;

    while (in_buffer_size) {
        gint16 tmp;

        qmi_utils_read_gint16_from_buffer (&in_buffer_walker, &in_buffer_size, QMI_ENDIAN_LITTLE, &tmp);
        g_assert_cmpint (tmp, ==, values[i++]);
        qmi_utils_write_gint16_to_buffer (&out_buffer_walker, &out_buffer_size, QMI_ENDIAN_LITTLE, &tmp);
    }

    g_assert_cmpuint (out_buffer_size, ==, 0);
    g_assert (memcmp (in_buffer, out_buffer, sizeof (in_buffer)) == 0);
}

static void
test_utils_int16_be (void)
{
    static const guint8 in_buffer[8] = {
        0x50, 0x0F, 0xE2, 0xEB, 0x00, 0xB6, 0x00, 0x00
    };
    static const gint16 values[4] = {
        0x500F, 0xE2EB, 0x00B6, 0x0000
    };
    guint8 out_buffer[8] = { 0 };

    guint i;
    guint16 in_buffer_size;
    const guint8 *in_buffer_walker;
    guint16 out_buffer_size;
    guint8 *out_buffer_walker;

    in_buffer_size = sizeof (in_buffer);
    in_buffer_walker = &in_buffer[0];
    out_buffer_size = sizeof (out_buffer);
    out_buffer_walker = &out_buffer[0];
    i = 0;

    while (in_buffer_size) {
        gint16 tmp;

        qmi_utils_read_gint16_from_buffer (&in_buffer_walker, &in_buffer_size, QMI_ENDIAN_BIG, &tmp);
        g_assert_cmpint (tmp, ==, values[i++]);
        qmi_utils_write_gint16_to_buffer (&out_buffer_walker, &out_buffer_size, QMI_ENDIAN_BIG, &tmp);
    }

    g_assert_cmpuint (out_buffer_size, ==, 0);
    g_assert (memcmp (in_buffer, out_buffer, sizeof (in_buffer)) == 0);
}

static void
test_utils_uint16_unaligned_le (void)
{
    static const guint8 in_buffer[9] = {
        0x00, 0x0F, 0x50, 0xEB, 0xE2, 0xB6, 0x00, 0x00, 0x00
    };
    static guint16 values[4] = {
        0x500F, 0xE2EB, 0x00B6, 0x0000
    };
    guint8 out_buffer[8] = { 0 };

    guint i;
    guint16 in_buffer_size;
    const guint8 *in_buffer_walker;
    guint16 out_buffer_size;
    guint8 *out_buffer_walker;

    in_buffer_size = sizeof (in_buffer) - 1;
    in_buffer_walker = &in_buffer[1];
    out_buffer_size = sizeof (out_buffer);
    out_buffer_walker = &out_buffer[0];
    i = 0;

    while (in_buffer_size) {
        guint16 tmp;

        qmi_utils_read_guint16_from_buffer (&in_buffer_walker, &in_buffer_size, QMI_ENDIAN_LITTLE, &tmp);
        g_assert_cmpuint (tmp, ==, values[i++]);
        qmi_utils_write_guint16_to_buffer (&out_buffer_walker, &out_buffer_size, QMI_ENDIAN_LITTLE, &tmp);
    }

    g_assert_cmpuint (out_buffer_size, ==, 0);
    g_assert (memcmp (&in_buffer[1], out_buffer, sizeof (in_buffer) - 1) == 0);
}

static void
test_utils_uint16_unaligned_be (void)
{
    static const guint8 in_buffer[9] = {
        0x00, 0x50, 0x0F, 0xE2, 0xEB, 0x00, 0xB6, 0x00, 0x00
    };
    static guint16 values[4] = {
        0x500F, 0xE2EB, 0x00B6, 0x0000
    };
    guint8 out_buffer[8] = { 0 };

    guint i;
    guint16 in_buffer_size;
    const guint8 *in_buffer_walker;
    guint16 out_buffer_size;
    guint8 *out_buffer_walker;

    in_buffer_size = sizeof (in_buffer) - 1;
    in_buffer_walker = &in_buffer[1];
    out_buffer_size = sizeof (out_buffer);
    out_buffer_walker = &out_buffer[0];
    i = 0;

    while (in_buffer_size) {
        guint16 tmp;

        qmi_utils_read_guint16_from_buffer (&in_buffer_walker, &in_buffer_size, QMI_ENDIAN_BIG, &tmp);
        g_assert_cmpuint (tmp, ==, values[i++]);
        qmi_utils_write_guint16_to_buffer (&out_buffer_walker, &out_buffer_size, QMI_ENDIAN_BIG, &tmp);
    }

    g_assert_cmpuint (out_buffer_size, ==, 0);
    g_assert (memcmp (&in_buffer[1], out_buffer, sizeof (in_buffer) - 1) == 0);
}

static void
test_utils_int16_unaligned_le (void)
{
    static const guint8 in_buffer[9] = {
        0x00, 0x0F, 0x50, 0xEB, 0xE2, 0xB6, 0x00, 0x00, 0x00
    };
    static gint16 values[4] = {
        0x500F, 0xE2EB, 0x00B6, 0x0000
    };
    guint8 out_buffer[8] = { 0 };

    guint i;
    guint16 in_buffer_size;
    const guint8 *in_buffer_walker;
    guint16 out_buffer_size;
    guint8 *out_buffer_walker;

    in_buffer_size = sizeof (in_buffer) - 1;
    in_buffer_walker = &in_buffer[1];
    out_buffer_size = sizeof (out_buffer);
    out_buffer_walker = &out_buffer[0];
    i = 0;

    while (in_buffer_size) {
        gint16 tmp;

        qmi_utils_read_gint16_from_buffer (&in_buffer_walker, &in_buffer_size, QMI_ENDIAN_LITTLE, &tmp);
        g_assert_cmpint (tmp, ==, values[i++]);
        qmi_utils_write_gint16_to_buffer (&out_buffer_walker, &out_buffer_size, QMI_ENDIAN_LITTLE, &tmp);
    }

    g_assert_cmpuint (out_buffer_size, ==, 0);
    g_assert (memcmp (&in_buffer[1], out_buffer, sizeof (in_buffer) - 1) == 0);
}

static void
test_utils_int16_unaligned_be (void)
{
    static const guint8 in_buffer[9] = {
        0x00, 0x50, 0x0F, 0xE2, 0xEB, 0x00, 0xB6, 0x00, 0x00
    };
    static gint16 values[4] = {
        0x500F, 0xE2EB, 0x00B6, 0x0000
    };
    guint8 out_buffer[8] = { 0 };

    guint i;
    guint16 in_buffer_size;
    const guint8 *in_buffer_walker;
    guint16 out_buffer_size;
    guint8 *out_buffer_walker;

    in_buffer_size = sizeof (in_buffer) - 1;
    in_buffer_walker = &in_buffer[1];
    out_buffer_size = sizeof (out_buffer);
    out_buffer_walker = &out_buffer[0];
    i = 0;

    while (in_buffer_size) {
        gint16 tmp;

        qmi_utils_read_gint16_from_buffer (&in_buffer_walker, &in_buffer_size, QMI_ENDIAN_BIG, &tmp);
        g_assert_cmpint (tmp, ==, values[i++]);
        qmi_utils_write_gint16_to_buffer (&out_buffer_walker, &out_buffer_size, QMI_ENDIAN_BIG, &tmp);
    }

    g_assert_cmpuint (out_buffer_size, ==, 0);
    g_assert (memcmp (&in_buffer[1], out_buffer, sizeof (in_buffer) - 1) == 0);
}

static void
test_utils_uint32_le (void)
{
    static const guint8 in_buffer[8] = {
        0x0F, 0x50, 0xEB, 0xE2, 0xB6, 0x00, 0x00, 0x00
    };
    static const guint32 values[2] = {
        0xE2EB500F, 0x000000B6
    };
    guint8 out_buffer[8] = { 0 };

    guint i;
    guint16 in_buffer_size;
    const guint8 *in_buffer_walker;
    guint16 out_buffer_size;
    guint8 *out_buffer_walker;

    in_buffer_size = sizeof (in_buffer);
    in_buffer_walker = &in_buffer[0];
    out_buffer_size = sizeof (out_buffer);
    out_buffer_walker = &out_buffer[0];
    i = 0;

    while (in_buffer_size) {
        guint32 tmp;

        qmi_utils_read_guint32_from_buffer (&in_buffer_walker, &in_buffer_size, QMI_ENDIAN_LITTLE, &tmp);
        g_assert_cmpuint (tmp, ==, values[i++]);
        qmi_utils_write_guint32_to_buffer (&out_buffer_walker, &out_buffer_size, QMI_ENDIAN_LITTLE, &tmp);
    }

    g_assert_cmpuint (out_buffer_size, ==, 0);
    g_assert (memcmp (in_buffer, out_buffer, sizeof (in_buffer)) == 0);
}

static void
test_utils_uint32_be (void)
{
    static const guint8 in_buffer[8] = {
        0xE2, 0xEB, 0x50, 0x0F, 0x00, 0x00, 0x00, 0xB6
    };
    static const guint32 values[2] = {
        0xE2EB500F, 0x000000B6
    };
    guint8 out_buffer[8] = { 0 };

    guint i;
    guint16 in_buffer_size;
    const guint8 *in_buffer_walker;
    guint16 out_buffer_size;
    guint8 *out_buffer_walker;

    in_buffer_size = sizeof (in_buffer);
    in_buffer_walker = &in_buffer[0];
    out_buffer_size = sizeof (out_buffer);
    out_buffer_walker = &out_buffer[0];
    i = 0;

    while (in_buffer_size) {
        guint32 tmp;

        qmi_utils_read_guint32_from_buffer (&in_buffer_walker, &in_buffer_size, QMI_ENDIAN_BIG, &tmp);
        g_assert_cmpuint (tmp, ==, values[i++]);
        qmi_utils_write_guint32_to_buffer (&out_buffer_walker, &out_buffer_size, QMI_ENDIAN_BIG, &tmp);
    }

    g_assert_cmpuint (out_buffer_size, ==, 0);
    g_assert (memcmp (in_buffer, out_buffer, sizeof (in_buffer)) == 0);
}

static void
test_utils_int32_le (void)
{
    static const guint8 in_buffer[8] = {
        0x0F, 0x50, 0xEB, 0xE2, 0xB6, 0x00, 0x00, 0x00
    };
    static const gint32 values[2] = {
        0xE2EB500F, 0x000000B6
    };
    guint8 out_buffer[8] = { 0 };

    guint i;
    guint16 in_buffer_size;
    const guint8 *in_buffer_walker;
    guint16 out_buffer_size;
    guint8 *out_buffer_walker;

    in_buffer_size = sizeof (in_buffer);
    in_buffer_walker = &in_buffer[0];
    out_buffer_size = sizeof (out_buffer);
    out_buffer_walker = &out_buffer[0];
    i = 0;

    while (in_buffer_size) {
        gint32 tmp;

        qmi_utils_read_gint32_from_buffer (&in_buffer_walker, &in_buffer_size, QMI_ENDIAN_LITTLE, &tmp);
        g_assert_cmpint (tmp, ==, values[i++]);
        qmi_utils_write_gint32_to_buffer (&out_buffer_walker, &out_buffer_size, QMI_ENDIAN_LITTLE, &tmp);
    }

    g_assert_cmpuint (out_buffer_size, ==, 0);
    g_assert (memcmp (in_buffer, out_buffer, sizeof (in_buffer)) == 0);
}

static void
test_utils_int32_be (void)
{
    static const guint8 in_buffer[8] = {
        0xE2, 0xEB, 0x50, 0x0F, 0x00, 0x00, 0x00, 0xB6
    };
    static const gint32 values[2] = {
        0xE2EB500F, 0x000000B6
    };
    guint8 out_buffer[8] = { 0 };

    guint i;
    guint16 in_buffer_size;
    const guint8 *in_buffer_walker;
    guint16 out_buffer_size;
    guint8 *out_buffer_walker;

    in_buffer_size = sizeof (in_buffer);
    in_buffer_walker = &in_buffer[0];
    out_buffer_size = sizeof (out_buffer);
    out_buffer_walker = &out_buffer[0];
    i = 0;

    while (in_buffer_size) {
        gint32 tmp;

        qmi_utils_read_gint32_from_buffer (&in_buffer_walker, &in_buffer_size, QMI_ENDIAN_BIG, &tmp);
        g_assert_cmpint (tmp, ==, values[i++]);
        qmi_utils_write_gint32_to_buffer (&out_buffer_walker, &out_buffer_size, QMI_ENDIAN_BIG, &tmp);
    }

    g_assert_cmpuint (out_buffer_size, ==, 0);
    g_assert (memcmp (in_buffer, out_buffer, sizeof (in_buffer)) == 0);
}

static void
test_utils_uint32_unaligned_le (void)
{
    static const guint8 in_buffer[9] = {
        0x00, 0x0F, 0x50, 0xEB, 0xE2, 0xB6, 0x00, 0x00, 0x00
    };
    static guint32 values[2] = {
        0xE2EB500F, 0x000000B6
    };
    guint8 out_buffer[8] = { 0 };

    guint i;
    guint16 in_buffer_size;
    const guint8 *in_buffer_walker;
    guint16 out_buffer_size;
    guint8 *out_buffer_walker;

    in_buffer_size = sizeof (in_buffer) - 1;
    in_buffer_walker = &in_buffer[1];
    out_buffer_size = sizeof (out_buffer);
    out_buffer_walker = &out_buffer[0];
    i = 0;

    while (in_buffer_size) {
        guint32 tmp;

        qmi_utils_read_guint32_from_buffer (&in_buffer_walker, &in_buffer_size, QMI_ENDIAN_LITTLE, &tmp);
        g_assert_cmpuint (tmp, ==, values[i++]);
        qmi_utils_write_guint32_to_buffer (&out_buffer_walker, &out_buffer_size, QMI_ENDIAN_LITTLE, &tmp);
    }

    g_assert_cmpuint (out_buffer_size, ==, 0);
    g_assert (memcmp (&in_buffer[1], out_buffer, sizeof (in_buffer) - 1) == 0);
}

static void
test_utils_uint32_unaligned_be (void)
{
    static const guint8 in_buffer[9] = {
        0x00, 0xE2, 0xEB, 0x50, 0x0F, 0x00, 0x00, 0x00, 0xB6
    };
    static guint32 values[2] = {
        0xE2EB500F, 0x000000B6
    };
    guint8 out_buffer[8] = { 0 };

    guint i;
    guint16 in_buffer_size;
    const guint8 *in_buffer_walker;
    guint16 out_buffer_size;
    guint8 *out_buffer_walker;

    in_buffer_size = sizeof (in_buffer) - 1;
    in_buffer_walker = &in_buffer[1];
    out_buffer_size = sizeof (out_buffer);
    out_buffer_walker = &out_buffer[0];
    i = 0;

    while (in_buffer_size) {
        guint32 tmp;

        qmi_utils_read_guint32_from_buffer (&in_buffer_walker, &in_buffer_size, QMI_ENDIAN_BIG, &tmp);
        g_assert_cmpuint (tmp, ==, values[i++]);
        qmi_utils_write_guint32_to_buffer (&out_buffer_walker, &out_buffer_size, QMI_ENDIAN_BIG, &tmp);
    }

    g_assert_cmpuint (out_buffer_size, ==, 0);
    g_assert (memcmp (&in_buffer[1], out_buffer, sizeof (in_buffer) - 1) == 0);
}

static void
test_utils_int32_unaligned_le (void)
{
    static const guint8 in_buffer[9] = {
        0x00, 0x0F, 0x50, 0xEB, 0xE2, 0xB6, 0x00, 0x00, 0x00
    };
    static gint32 values[2] = {
        0xE2EB500F, 0x000000B6
    };
    guint8 out_buffer[8] = { 0 };

    guint i;
    guint16 in_buffer_size;
    const guint8 *in_buffer_walker;
    guint16 out_buffer_size;
    guint8 *out_buffer_walker;

    in_buffer_size = sizeof (in_buffer) - 1;
    in_buffer_walker = &in_buffer[1];
    out_buffer_size = sizeof (out_buffer);
    out_buffer_walker = &out_buffer[0];
    i = 0;

    while (in_buffer_size) {
        gint32 tmp;

        qmi_utils_read_gint32_from_buffer (&in_buffer_walker, &in_buffer_size, QMI_ENDIAN_LITTLE, &tmp);
        g_assert_cmpint (tmp, ==, values[i++]);
        qmi_utils_write_gint32_to_buffer (&out_buffer_walker, &out_buffer_size, QMI_ENDIAN_LITTLE, &tmp);
    }

    g_assert_cmpuint (out_buffer_size, ==, 0);
    g_assert (memcmp (&in_buffer[1], out_buffer, sizeof (in_buffer) - 1) == 0);
}

static void
test_utils_int32_unaligned_be (void)
{
    static const guint8 in_buffer[9] = {
        0x00, 0xE2, 0xEB, 0x50, 0x0F, 0x00, 0x00, 0x00, 0xB6
    };
    static gint32 values[2] = {
        0xE2EB500F, 0x000000B6
    };
    guint8 out_buffer[8] = { 0 };

    guint i;
    guint16 in_buffer_size;
    const guint8 *in_buffer_walker;
    guint16 out_buffer_size;
    guint8 *out_buffer_walker;

    in_buffer_size = sizeof (in_buffer) - 1;
    in_buffer_walker = &in_buffer[1];
    out_buffer_size = sizeof (out_buffer);
    out_buffer_walker = &out_buffer[0];
    i = 0;

    while (in_buffer_size) {
        gint32 tmp;

        qmi_utils_read_gint32_from_buffer (&in_buffer_walker, &in_buffer_size, QMI_ENDIAN_BIG, &tmp);
        g_assert_cmpint (tmp, ==, values[i++]);
        qmi_utils_write_gint32_to_buffer (&out_buffer_walker, &out_buffer_size, QMI_ENDIAN_BIG, &tmp);
    }

    g_assert_cmpuint (out_buffer_size, ==, 0);
    g_assert (memcmp (&in_buffer[1], out_buffer, sizeof (in_buffer) - 1) == 0);
}

static void
test_utils_uint64_le (void)
{
    static const guint8 in_buffer[8] = {
        0x0F, 0x50, 0xEB, 0xE2, 0xB6, 0x00, 0x00, 0x00
    };
    static const guint64 values[1] = {
        0x000000B6E2EB500FULL
    };
    guint8 out_buffer[8] = { 0 };

    guint i;
    guint16 in_buffer_size;
    const guint8 *in_buffer_walker;
    guint16 out_buffer_size;
    guint8 *out_buffer_walker;

    in_buffer_size = sizeof (in_buffer);
    in_buffer_walker = &in_buffer[0];
    out_buffer_size = sizeof (out_buffer);
    out_buffer_walker = &out_buffer[0];
    i = 0;

    while (in_buffer_size) {
        guint64 tmp;

        qmi_utils_read_guint64_from_buffer (&in_buffer_walker, &in_buffer_size, QMI_ENDIAN_LITTLE, &tmp);
        g_assert_cmpuint (tmp, ==, values[i++]);
        qmi_utils_write_guint64_to_buffer (&out_buffer_walker, &out_buffer_size, QMI_ENDIAN_LITTLE, &tmp);
    }

    g_assert_cmpuint (out_buffer_size, ==, 0);
    g_assert (memcmp (in_buffer, out_buffer, sizeof (in_buffer)) == 0);
}

static void
test_utils_uint64_be (void)
{
    static const guint8 in_buffer[8] = {
        0x00, 0x00, 0x00, 0xB6, 0xE2, 0xEB, 0x50, 0x0F
    };
    static const guint64 values[1] = {
        0x000000B6E2EB500FULL
    };
    guint8 out_buffer[8] = { 0 };

    guint i;
    guint16 in_buffer_size;
    const guint8 *in_buffer_walker;
    guint16 out_buffer_size;
    guint8 *out_buffer_walker;

    in_buffer_size = sizeof (in_buffer);
    in_buffer_walker = &in_buffer[0];
    out_buffer_size = sizeof (out_buffer);
    out_buffer_walker = &out_buffer[0];
    i = 0;

    while (in_buffer_size) {
        guint64 tmp;

        qmi_utils_read_guint64_from_buffer (&in_buffer_walker, &in_buffer_size, QMI_ENDIAN_BIG, &tmp);
        g_assert_cmpuint (tmp, ==, values[i++]);
        qmi_utils_write_guint64_to_buffer (&out_buffer_walker, &out_buffer_size, QMI_ENDIAN_BIG, &tmp);
    }

    g_assert_cmpuint (out_buffer_size, ==, 0);
    g_assert (memcmp (in_buffer, out_buffer, sizeof (in_buffer)) == 0);
}

static void
test_utils_int64_le (void)
{
    static const guint8 in_buffer[8] = {
        0x0F, 0x50, 0xEB, 0xE2, 0xB6, 0x00, 0x00, 0x00
    };
    static const gint64 values[1] = {
        0x000000B6E2EB500FLL
    };
    guint8 out_buffer[8] = { 0 };

    guint i;
    guint16 in_buffer_size;
    const guint8 *in_buffer_walker;
    guint16 out_buffer_size;
    guint8 *out_buffer_walker;

    in_buffer_size = sizeof (in_buffer);
    in_buffer_walker = &in_buffer[0];
    out_buffer_size = sizeof (out_buffer);
    out_buffer_walker = &out_buffer[0];
    i = 0;

    while (in_buffer_size) {
        gint64 tmp;

        qmi_utils_read_gint64_from_buffer (&in_buffer_walker, &in_buffer_size, QMI_ENDIAN_LITTLE, &tmp);
        g_assert_cmpint (tmp, ==, values[i++]);
        qmi_utils_write_gint64_to_buffer (&out_buffer_walker, &out_buffer_size, QMI_ENDIAN_LITTLE, &tmp);
    }

    g_assert_cmpuint (out_buffer_size, ==, 0);
    g_assert (memcmp (in_buffer, out_buffer, sizeof (in_buffer)) == 0);
}

static void
test_utils_int64_be (void)
{
    static const guint8 in_buffer[8] = {
        0x00, 0x00, 0x00, 0xB6, 0xE2, 0xEB, 0x50, 0x0F
    };
    static const gint64 values[1] = {
        0x000000B6E2EB500FLL
    };
    guint8 out_buffer[8] = { 0 };

    guint i;
    guint16 in_buffer_size;
    const guint8 *in_buffer_walker;
    guint16 out_buffer_size;
    guint8 *out_buffer_walker;

    in_buffer_size = sizeof (in_buffer);
    in_buffer_walker = &in_buffer[0];
    out_buffer_size = sizeof (out_buffer);
    out_buffer_walker = &out_buffer[0];
    i = 0;

    while (in_buffer_size) {
        gint64 tmp;

        qmi_utils_read_gint64_from_buffer (&in_buffer_walker, &in_buffer_size, QMI_ENDIAN_BIG, &tmp);
        g_assert_cmpint (tmp, ==, values[i++]);
        qmi_utils_write_gint64_to_buffer (&out_buffer_walker, &out_buffer_size, QMI_ENDIAN_BIG, &tmp);
    }

    g_assert_cmpuint (out_buffer_size, ==, 0);
    g_assert (memcmp (in_buffer, out_buffer, sizeof (in_buffer)) == 0);
}

static void
test_utils_uint64_unaligned_le (void)
{
    static const guint8 in_buffer[9] = {
        0x00, 0x0F, 0x50, 0xEB, 0xE2, 0xB6, 0x00, 0x00, 0x00
    };
    static guint64 values[1] = {
        0x000000B6E2EB500FULL
    };
    guint8 out_buffer[8] = { 0 };

    guint i;
    guint16 in_buffer_size;
    const guint8 *in_buffer_walker;
    guint16 out_buffer_size;
    guint8 *out_buffer_walker;

    in_buffer_size = sizeof (in_buffer) - 1;
    in_buffer_walker = &in_buffer[1];
    out_buffer_size = sizeof (out_buffer);
    out_buffer_walker = &out_buffer[0];
    i = 0;

    while (in_buffer_size) {
        guint64 tmp;

        qmi_utils_read_guint64_from_buffer (&in_buffer_walker, &in_buffer_size, QMI_ENDIAN_LITTLE, &tmp);
        g_assert_cmpuint (tmp, ==, values[i++]);
        qmi_utils_write_guint64_to_buffer (&out_buffer_walker, &out_buffer_size, QMI_ENDIAN_LITTLE, &tmp);
    }

    g_assert_cmpuint (out_buffer_size, ==, 0);
    g_assert (memcmp (&in_buffer[1], out_buffer, sizeof (in_buffer) - 1) == 0);
}

static void
test_utils_uint64_unaligned_be (void)
{
    static const guint8 in_buffer[9] = {
        0x00, 0x00, 0x00, 0x00, 0xB6, 0xE2, 0xEB, 0x50, 0x0F
    };
    static guint64 values[1] = {
        0x000000B6E2EB500FULL
    };
    guint8 out_buffer[8] = { 0 };

    guint i;
    guint16 in_buffer_size;
    const guint8 *in_buffer_walker;
    guint16 out_buffer_size;
    guint8 *out_buffer_walker;

    in_buffer_size = sizeof (in_buffer) - 1;
    in_buffer_walker = &in_buffer[1];
    out_buffer_size = sizeof (out_buffer);
    out_buffer_walker = &out_buffer[0];
    i = 0;

    while (in_buffer_size) {
        guint64 tmp;

        qmi_utils_read_guint64_from_buffer (&in_buffer_walker, &in_buffer_size, QMI_ENDIAN_BIG, &tmp);
        g_assert_cmpuint (tmp, ==, values[i++]);
        qmi_utils_write_guint64_to_buffer (&out_buffer_walker, &out_buffer_size, QMI_ENDIAN_BIG, &tmp);
    }

    g_assert_cmpuint (out_buffer_size, ==, 0);
    g_assert (memcmp (&in_buffer[1], out_buffer, sizeof (in_buffer) - 1) == 0);
}

static void
test_utils_int64_unaligned_le (void)
{
    static const guint8 in_buffer[9] = {
        0x00, 0x0F, 0x50, 0xEB, 0xE2, 0xB6, 0x00, 0x00, 0x00
    };
    static gint64 values[1] = {
        0x000000B6E2EB500FLL
    };
    guint8 out_buffer[8] = { 0 };

    guint i;
    guint16 in_buffer_size;
    const guint8 *in_buffer_walker;
    guint16 out_buffer_size;
    guint8 *out_buffer_walker;

    in_buffer_size = sizeof (in_buffer) - 1;
    in_buffer_walker = &in_buffer[1];
    out_buffer_size = sizeof (out_buffer);
    out_buffer_walker = &out_buffer[0];
    i = 0;

    while (in_buffer_size) {
        gint64 tmp;

        qmi_utils_read_gint64_from_buffer (&in_buffer_walker, &in_buffer_size, QMI_ENDIAN_LITTLE, &tmp);
        g_assert_cmpint (tmp, ==, values[i++]);
        qmi_utils_write_gint64_to_buffer (&out_buffer_walker, &out_buffer_size, QMI_ENDIAN_LITTLE, &tmp);
    }

    g_assert_cmpuint (out_buffer_size, ==, 0);
    g_assert (memcmp (&in_buffer[1], out_buffer, sizeof (in_buffer) - 1) == 0);
}

static void
test_utils_int64_unaligned_be (void)
{
    static const guint8 in_buffer[9] = {
        0x00, 0x00, 0x00, 0x00, 0xB6, 0xE2, 0xEB, 0x50, 0x0F
    };
    static gint64 values[1] = {
        0x000000B6E2EB500FLL
    };
    guint8 out_buffer[8] = { 0 };

    guint i;
    guint16 in_buffer_size;
    const guint8 *in_buffer_walker;
    guint16 out_buffer_size;
    guint8 *out_buffer_walker;

    in_buffer_size = sizeof (in_buffer) - 1;
    in_buffer_walker = &in_buffer[1];
    out_buffer_size = sizeof (out_buffer);
    out_buffer_walker = &out_buffer[0];
    i = 0;

    while (in_buffer_size) {
        gint64 tmp;

        qmi_utils_read_gint64_from_buffer (&in_buffer_walker, &in_buffer_size, QMI_ENDIAN_BIG, &tmp);
        g_assert_cmpint (tmp, ==, values[i++]);
        qmi_utils_write_gint64_to_buffer (&out_buffer_walker, &out_buffer_size, QMI_ENDIAN_BIG, &tmp);
    }

    g_assert_cmpuint (out_buffer_size, ==, 0);
    g_assert (memcmp (&in_buffer[1], out_buffer, sizeof (in_buffer) - 1) == 0);
}

static void
common_test_utils_uint_sized_le (guint n_bytes)
{
    static const guint8 in_buffer[8] = {
        0x0F, 0x50, 0xEB, 0xE2, 0xB6, 0x00, 0x00, 0x00
    };
    guint64 value = 0x000000B6E2EB500FULL;
    guint8 expected_out_buffer[8] = { 0 };
    guint8 out_buffer[8] = { 0 };

    guint64 tmp;
    guint i;
    guint16 in_buffer_size;
    const guint8 *in_buffer_walker;
    guint16 out_buffer_size;
    guint8 *out_buffer_walker;

    /* Build expected intermediate value */
    tmp = 0xFF;
    for (i = 1; i < n_bytes; i++) {
        tmp <<= 8;
        tmp |= 0xFF;
    }
    value &= tmp;

    /* Build expected output buffer */
    memcpy (expected_out_buffer, in_buffer, n_bytes);

    in_buffer_size = sizeof (in_buffer);
    in_buffer_walker = &in_buffer[0];
    out_buffer_size = sizeof (out_buffer);
    out_buffer_walker = &out_buffer[0];

    qmi_utils_read_sized_guint_from_buffer (&in_buffer_walker, &in_buffer_size, n_bytes, QMI_ENDIAN_LITTLE, &tmp);
    g_assert_cmpuint (tmp, ==, value);
    qmi_utils_write_sized_guint_to_buffer (&out_buffer_walker, &out_buffer_size, n_bytes, QMI_ENDIAN_LITTLE, &tmp);

    g_assert_cmpuint (out_buffer_size, ==, 8 - n_bytes);
    g_assert (memcmp (expected_out_buffer, out_buffer, sizeof (expected_out_buffer)) == 0);
}

static void
test_utils_uint_sized_1_le (void)
{
    common_test_utils_uint_sized_le (1);
}

static void
test_utils_uint_sized_2_le (void)
{
    common_test_utils_uint_sized_le (2);
}

static void
test_utils_uint_sized_4_le (void)
{
    common_test_utils_uint_sized_le (4);
}

static void
test_utils_uint_sized_8_le (void)
{
    common_test_utils_uint_sized_le (8);
}

static void
common_test_utils_uint_sized_be (guint n_bytes)
{
    static const guint8 in_buffer[8] = {
        0x00, 0x00, 0x00, 0xB6, 0xE2, 0xEB, 0x50, 0x0F
    };
    guint64 value = 0x000000B6E2EB500FULL;
    guint8 expected_out_buffer[8] = { 0 };
    guint8 out_buffer[8] = { 0 };
    guint8 in_buffer_aux[8] = { 0 };

    guint64 tmp;
    guint i;
    guint16 in_buffer_size;
    const guint8 *in_buffer_walker;
    guint16 out_buffer_size;
    guint8 *out_buffer_walker;

    /* Build expected intermediate value */
    tmp = 0xFF;
    for (i = 1; i < n_bytes; i++) {
        tmp <<= 8;
        tmp |= 0xFF;
    }
    value &= tmp;

    /* In BIG ENDIAN buffers, let's read only the bytes we want, starting from
     * the byte we want, not from the beginning of the input buffer. But we do
     * want to be aligned while reading, so we copy the bytes to read to the
     * beginning of an aux buffer */
    g_assert (n_bytes <= sizeof (in_buffer));
    memcpy (&in_buffer_aux[0], &in_buffer[sizeof (in_buffer) - n_bytes], n_bytes);

    /* Build expected output buffer */
    memcpy (&expected_out_buffer[0], &in_buffer_aux[0], n_bytes);

    in_buffer_size = sizeof (in_buffer);
    in_buffer_walker = &in_buffer_aux[0];
    out_buffer_size = sizeof (out_buffer);
    out_buffer_walker = &out_buffer[0];

    qmi_utils_read_sized_guint_from_buffer (&in_buffer_walker, &in_buffer_size, n_bytes, QMI_ENDIAN_BIG, &tmp);
    g_assert_cmpuint (tmp, ==, value);
    qmi_utils_write_sized_guint_to_buffer (&out_buffer_walker, &out_buffer_size, n_bytes, QMI_ENDIAN_BIG, &tmp);

    g_assert_cmpuint (out_buffer_size, ==, 8 - n_bytes);
    if (memcmp (expected_out_buffer, out_buffer, sizeof (expected_out_buffer)) != 0) {
        g_print ("OUTPUT: %x, %x, %x, %x, %x, %x, %x, %x\n",
                 out_buffer[0], out_buffer[1], out_buffer[2], out_buffer[3],
                 out_buffer[4], out_buffer[5], out_buffer[6], out_buffer[7]);
        g_print ("EXPECTED: %x, %x, %x, %x, %x, %x, %x, %x\n",
                 expected_out_buffer[0], expected_out_buffer[1], expected_out_buffer[2], expected_out_buffer[3],
                 expected_out_buffer[4], expected_out_buffer[5], expected_out_buffer[6], expected_out_buffer[7]);
    }
    g_assert (memcmp (expected_out_buffer, out_buffer, sizeof (expected_out_buffer)) == 0);
}

static void
test_utils_uint_sized_1_be (void)
{
    common_test_utils_uint_sized_be (1);
}

static void
test_utils_uint_sized_2_be (void)
{
    common_test_utils_uint_sized_be (2);
}

static void
test_utils_uint_sized_4_be (void)
{
    common_test_utils_uint_sized_be (4);
}

static void
test_utils_uint_sized_8_be (void)
{
    common_test_utils_uint_sized_be (8);
}

static void
common_test_utils_uint_sized_unaligned_le (guint n_bytes)
{
    static const guint8 in_buffer[9] = {
        0x00, 0x0F, 0x50, 0xEB, 0xE2, 0xB6, 0x00, 0x00, 0x00
    };
    guint64 value = 0x000000B6E2EB500FULL;
    guint8 expected_out_buffer[8] = { 0 };
    guint8 out_buffer[8] = { 0 };

    guint64 tmp;
    guint i;
    guint16 in_buffer_size;
    const guint8 *in_buffer_walker;
    guint16 out_buffer_size;
    guint8 *out_buffer_walker;

    /* Build expected intermediate value */
    tmp = 0xFF;
    for (i = 1; i < n_bytes; i++) {
        tmp <<= 8;
        tmp |= 0xFF;
    }
    value &= tmp;

    /* Build expected output buffer */
    memcpy (expected_out_buffer, &in_buffer[1], n_bytes);

    in_buffer_size = sizeof (in_buffer) - 1;
    in_buffer_walker = &in_buffer[1];
    out_buffer_size = sizeof (out_buffer);
    out_buffer_walker = &out_buffer[0];

    qmi_utils_read_sized_guint_from_buffer (&in_buffer_walker, &in_buffer_size, n_bytes, QMI_ENDIAN_LITTLE, &tmp);
    g_assert_cmpuint (tmp, ==, value);
    qmi_utils_write_sized_guint_to_buffer (&out_buffer_walker, &out_buffer_size, n_bytes, QMI_ENDIAN_LITTLE, &tmp);

    g_assert_cmpuint (out_buffer_size, ==, 8 - n_bytes);
    g_assert (memcmp (expected_out_buffer, out_buffer, sizeof (expected_out_buffer)) == 0);
}

static void
test_utils_uint_sized_1_unaligned_le (void)
{
    common_test_utils_uint_sized_unaligned_le (1);
}

static void
test_utils_uint_sized_2_unaligned_le (void)
{
    common_test_utils_uint_sized_unaligned_le (2);
}

static void
test_utils_uint_sized_4_unaligned_le (void)
{
    common_test_utils_uint_sized_unaligned_le (4);
}

static void
test_utils_uint_sized_8_unaligned_le (void)
{
    common_test_utils_uint_sized_unaligned_le (8);
}

static void
common_test_utils_uint_sized_unaligned_be (guint n_bytes)
{
    static const guint8 in_buffer[8] = {
        0x00, 0x00, 0x00, 0xB6, 0xE2, 0xEB, 0x50, 0x0F
    };
    guint64 value = 0x000000B6E2EB500FULL;
    guint8 expected_out_buffer[8] = { 0 };
    guint8 out_buffer[8] = { 0 };
    guint8 in_buffer_aux[9] = { 0 };

    guint64 tmp;
    guint i;
    guint16 in_buffer_size;
    const guint8 *in_buffer_walker;
    guint16 out_buffer_size;
    guint8 *out_buffer_walker;

    /* Build expected intermediate value */
    tmp = 0xFF;
    for (i = 1; i < n_bytes; i++) {
        tmp <<= 8;
        tmp |= 0xFF;
    }
    value &= tmp;

    /* In BIG ENDIAN buffers, let's read only the bytes we want, starting from
     * the byte we want, not from the beginning of the input buffer. But we do
     * not want to be aligned while reading, so we copy the bytes to read to
     * almost the beginning of an aux buffer */
    g_assert (n_bytes <= (sizeof (in_buffer)));
    memcpy (&in_buffer_aux[1], &in_buffer[sizeof (in_buffer) - n_bytes], n_bytes);

    /* Build expected output buffer */
    memcpy (expected_out_buffer, &in_buffer_aux[1], n_bytes);

    in_buffer_size = sizeof (in_buffer_aux) - 1;
    in_buffer_walker = &in_buffer_aux[1];
    out_buffer_size = sizeof (out_buffer);
    out_buffer_walker = &out_buffer[0];

    qmi_utils_read_sized_guint_from_buffer (&in_buffer_walker, &in_buffer_size, n_bytes, QMI_ENDIAN_BIG, &tmp);
    g_assert_cmpuint (tmp, ==, value);
    qmi_utils_write_sized_guint_to_buffer (&out_buffer_walker, &out_buffer_size, n_bytes, QMI_ENDIAN_BIG, &tmp);

    g_assert_cmpuint (out_buffer_size, ==, 8 - n_bytes);
    if (memcmp (expected_out_buffer, out_buffer, sizeof (expected_out_buffer)) != 0) {
        g_print ("OUTPUT: %x, %x, %x, %x, %x, %x, %x, %x\n",
                 out_buffer[0], out_buffer[1], out_buffer[2], out_buffer[3],
                 out_buffer[4], out_buffer[5], out_buffer[6], out_buffer[7]);
        g_print ("EXPECTED: %x, %x, %x, %x, %x, %x, %x, %x\n",
                 expected_out_buffer[0], expected_out_buffer[1], expected_out_buffer[2], expected_out_buffer[3],
                 expected_out_buffer[4], expected_out_buffer[5], expected_out_buffer[6], expected_out_buffer[7]);
    }
    g_assert (memcmp (expected_out_buffer, out_buffer, sizeof (expected_out_buffer)) == 0);
}

static void
test_utils_uint_sized_1_unaligned_be (void)
{
    common_test_utils_uint_sized_unaligned_be (1);
}

static void
test_utils_uint_sized_2_unaligned_be (void)
{
    common_test_utils_uint_sized_unaligned_be (2);
}

static void
test_utils_uint_sized_4_unaligned_be (void)
{
    common_test_utils_uint_sized_unaligned_be (4);
}

static void
test_utils_uint_sized_8_unaligned_be (void)
{
    common_test_utils_uint_sized_unaligned_be (8);
}

int main (int argc, char **argv)
{
    g_test_init (&argc, &argv, NULL);

    g_test_add_func ("/libqmi-glib/compat/utils/uint8",  test_utils_uint8);
    g_test_add_func ("/libqmi-glib/compat/utils/int8",   test_utils_int8);

    g_test_add_func ("/libqmi-glib/compat/utils/uint16-LE",           test_utils_uint16_le);
    g_test_add_func ("/libqmi-glib/compat/utils/uint16-BE",           test_utils_uint16_be);
    g_test_add_func ("/libqmi-glib/compat/utils/int16-LE",            test_utils_int16_le);
    g_test_add_func ("/libqmi-glib/compat/utils/int16-BE",            test_utils_int16_be);
    g_test_add_func ("/libqmi-glib/compat/utils/uint16-unaligned-LE", test_utils_uint16_unaligned_le);
    g_test_add_func ("/libqmi-glib/compat/utils/uint16-unaligned-Be", test_utils_uint16_unaligned_be);
    g_test_add_func ("/libqmi-glib/compat/utils/int16-unaligned-LE",  test_utils_int16_unaligned_le);
    g_test_add_func ("/libqmi-glib/compat/utils/int16-unaligned-BE",  test_utils_int16_unaligned_be);

    g_test_add_func ("/libqmi-glib/compat/utils/uint32-LE",           test_utils_uint32_le);
    g_test_add_func ("/libqmi-glib/compat/utils/uint32-BE",           test_utils_uint32_be);
    g_test_add_func ("/libqmi-glib/compat/utils/int32-LE",            test_utils_int32_le);
    g_test_add_func ("/libqmi-glib/compat/utils/int32-BE",            test_utils_int32_be);
    g_test_add_func ("/libqmi-glib/compat/utils/uint32/unaligned-LE", test_utils_uint32_unaligned_le);
    g_test_add_func ("/libqmi-glib/compat/utils/uint32/unaligned-BE", test_utils_uint32_unaligned_be);
    g_test_add_func ("/libqmi-glib/compat/utils/int32/unaligned-LE",  test_utils_int32_unaligned_le);
    g_test_add_func ("/libqmi-glib/compat/utils/int32/unaligned-BE",  test_utils_int32_unaligned_be);

    g_test_add_func ("/libqmi-glib/compat/utils/uint64-LE",           test_utils_uint64_le);
    g_test_add_func ("/libqmi-glib/compat/utils/uint64-BE",           test_utils_uint64_be);
    g_test_add_func ("/libqmi-glib/compat/utils/int64-LE",            test_utils_int64_le);
    g_test_add_func ("/libqmi-glib/compat/utils/int64-BE",            test_utils_int64_be);
    g_test_add_func ("/libqmi-glib/compat/utils/uint64/unaligned-LE", test_utils_uint64_unaligned_le);
    g_test_add_func ("/libqmi-glib/compat/utils/uint64/unaligned-BE", test_utils_uint64_unaligned_be);
    g_test_add_func ("/libqmi-glib/compat/utils/int64/unaligned-LE",  test_utils_int64_unaligned_le);
    g_test_add_func ("/libqmi-glib/compat/utils/int64/unaligned-BE",  test_utils_int64_unaligned_be);

    g_test_add_func ("/libqmi-glib/compat/utils/uint-sized-1-LE",           test_utils_uint_sized_1_le);
    g_test_add_func ("/libqmi-glib/compat/utils/uint-sized-2-LE",           test_utils_uint_sized_2_le);
    g_test_add_func ("/libqmi-glib/compat/utils/uint-sized-4-LE",           test_utils_uint_sized_4_le);
    g_test_add_func ("/libqmi-glib/compat/utils/uint-sized-8-LE",           test_utils_uint_sized_8_le);
    g_test_add_func ("/libqmi-glib/compat/utils/uint-sized-1-BE",           test_utils_uint_sized_1_be);
    g_test_add_func ("/libqmi-glib/compat/utils/uint-sized-2-BE",           test_utils_uint_sized_2_be);
    g_test_add_func ("/libqmi-glib/compat/utils/uint-sized-4-BE",           test_utils_uint_sized_4_be);
    g_test_add_func ("/libqmi-glib/compat/utils/uint-sized-8-BE",           test_utils_uint_sized_8_be);
    g_test_add_func ("/libqmi-glib/compat/utils/uint-sized-1-unaligned-LE", test_utils_uint_sized_1_unaligned_le);
    g_test_add_func ("/libqmi-glib/compat/utils/uint-sized-2-unaligned-LE", test_utils_uint_sized_2_unaligned_le);
    g_test_add_func ("/libqmi-glib/compat/utils/uint-sized-4-unaligned-LE", test_utils_uint_sized_4_unaligned_le);
    g_test_add_func ("/libqmi-glib/compat/utils/uint-sized-8-unaligned-LE", test_utils_uint_sized_8_unaligned_le);
    g_test_add_func ("/libqmi-glib/compat/utils/uint-sized-1-unaligned-BE", test_utils_uint_sized_1_unaligned_be);
    g_test_add_func ("/libqmi-glib/compat/utils/uint-sized-2-unaligned-BE", test_utils_uint_sized_2_unaligned_be);
    g_test_add_func ("/libqmi-glib/compat/utils/uint-sized-4-unaligned-BE", test_utils_uint_sized_4_unaligned_be);
    g_test_add_func ("/libqmi-glib/compat/utils/uint-sized-8-unaligned-BE", test_utils_uint_sized_8_unaligned_be);

    return g_test_run ();
}

#else

int main (int argc, char **argv)
{
    return 0;
}

#endif /* QMI_DISABLE_DEPRECATED */
