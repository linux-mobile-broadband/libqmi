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
 * Copyright (C) 2020 Aleksander Morgado <aleksander@aleksander.es>
 */

#include <glib-object.h>
#include <string.h>
#include "qmi-utils.h"
#include "qmi-enums-nas.h"

/******************************************************************************/

static void
common_test_read_string_from_plmn_encoded_array (QmiNasPlmnEncodingScheme  encoding,
                                                 const guint8             *data,
                                                 gsize                     data_len,
                                                 const gchar              *expected_utf8)
{
    g_autofree gchar *utf8 = NULL;
    g_autoptr(GArray) array = NULL;

    array = g_array_append_vals (g_array_sized_new (FALSE, FALSE, sizeof (guint8), data_len),
                                 data, data_len);

    utf8 = qmi_nas_read_string_from_plmn_encoded_array (encoding, array);

    g_assert_nonnull (utf8);
    g_assert_cmpstr (utf8, ==, expected_utf8);
}

static void
test_read_string_from_plmn_encoded_array_gsm7_default_chars (void)
{
    const guint8 gsm[] = {
        0x80, 0x80, 0x60, 0x40, 0x28, 0x18, 0x0E, 0x88,
        0x84, 0x62, 0xC1, 0x68, 0x38, 0x1E, 0x90, 0x88,
        0x64, 0x42, 0xA9, 0x58, 0x2E, 0x98, 0x8C, 0x86,
        0xD3, 0xF1, 0x7C, 0x40, 0x21, 0xD1, 0x88, 0x54,
        0x32, 0x9D, 0x50, 0x29, 0xD5, 0x8A, 0xD5, 0x72,
        0xBD, 0x60, 0x31, 0xD9, 0x8C, 0x56, 0xB3, 0xDD,
        0x70, 0x39, 0xDD, 0x8E, 0xD7, 0xF3, 0xFD, 0x80,
        0x41, 0xE1, 0x90, 0x58, 0x34, 0x1E, 0x91, 0x49,
        0xE5, 0x92, 0xD9, 0x74, 0x3E, 0xA1, 0x51, 0xE9,
        0x94, 0x5A, 0xB5, 0x5E, 0xB1, 0x59, 0xED, 0x96,
        0xDB, 0xF5, 0x7E, 0xC1, 0x61, 0xF1, 0x98, 0x5C,
        0x36, 0x9F, 0xD1, 0x69, 0xF5, 0x9A, 0xDD, 0x76,
        0xBF, 0xE1, 0x71, 0xF9, 0x9C, 0x5E, 0xB7, 0xDF,
        0xF1, 0x79, 0xFD, 0x9E, 0xDF, 0xF7, 0xFF, 0x01,
    };
    const gchar *expected_utf8 = "@£$¥èéùìòÇ\nØø\rÅåΔ_ΦΓΛΩΠΨΣΘΞÆæßÉ !\"#¤%&'()*+,-./0123456789:;<=>?¡ABCDEFGHIJKLMNOPQRSTUVWXYZÄÖÑÜ§¿abcdefghijklmnopqrstuvwxyzäöñüà";

    common_test_read_string_from_plmn_encoded_array (QMI_NAS_PLMN_ENCODING_SCHEME_GSM, gsm, G_N_ELEMENTS (gsm), expected_utf8);
}

static void
test_read_string_from_plmn_encoded_array_gsm7_extended_chars (void)
{
    const guint8 gsm[] = {
        0x1B, 0xC5, 0x86, 0xB2, 0x41, 0x6D, 0x52, 0x9B,
        0xD7, 0x86, 0xB7, 0xE9, 0x6D, 0x7C, 0x1B, 0xE0,
        0xA6, 0x0C
    };
    const gchar *expected_utf8 = "\f^{}\\[~]|€";

    common_test_read_string_from_plmn_encoded_array (QMI_NAS_PLMN_ENCODING_SCHEME_GSM, gsm, G_N_ELEMENTS (gsm), expected_utf8);
}

static void
test_read_string_from_plmn_encoded_array_gsm7_mixed_chars (void)
{
    const guint8 gsm[] = {
        0x80, 0x80, 0x60, 0x40, 0x28, 0x18, 0x0E, 0x8C,
        0x8D, 0xA2, 0x62, 0xB9, 0x60, 0x32, 0x1B, 0x94,
        0x86, 0xD3, 0xF1, 0xA0, 0x36, 0xA9, 0xD4, 0x0D,
        0x97, 0xDB, 0xBC, 0x74, 0x3B, 0x5E, 0xCF, 0xB7,
        0xE1, 0xFD, 0x80, 0x51, 0xE9, 0x74, 0xE3, 0xA3,
        0x56, 0xB9, 0x1B, 0x60, 0xD7, 0xFB, 0x05, 0x87,
        0xC5, 0xF0, 0xB8, 0x7C, 0x4E, 0xAF, 0xDB, 0xF9,
        0x7D, 0xFF, 0x7F, 0x53, 0x06
    };
    const gchar *expected_utf8 = "@£$¥èéùìø\fΩΠΨΣΘ{ΞÆæß(})789\\:;<=>[?¡QRS]TUÖ|ÑÜ§¿abpqrstuvöñüà€";

    common_test_read_string_from_plmn_encoded_array (QMI_NAS_PLMN_ENCODING_SCHEME_GSM, gsm, G_N_ELEMENTS (gsm), expected_utf8);
}

static void
test_read_string_from_plmn_encoded_array_ucs2le (void)
{
    const guint8 ucs2le[] = {
        0x54, 0x00, 0x2d, 0x00, 0x4d, 0x00, 0x6f, 0x00,
        0x62, 0x00, 0x69, 0x00, 0x6c, 0x00, 0x65, 0x00
    };
    const gchar *expected_utf8 = "T-Mobile";

    common_test_read_string_from_plmn_encoded_array (QMI_NAS_PLMN_ENCODING_SCHEME_UCS2LE, ucs2le, G_N_ELEMENTS (ucs2le), expected_utf8);
}

/******************************************************************************/

static void
common_test_read_string_from_network_description_encoded_array (QmiNasNetworkDescriptionEncoding  encoding,
                                                                const guint8                     *data,
                                                                gsize                             data_len,
                                                                const gchar                      *expected_utf8)
{
    g_autofree gchar *utf8 = NULL;
    g_autoptr(GArray) array = NULL;

    array = g_array_append_vals (g_array_sized_new (FALSE, FALSE, sizeof (guint8), data_len),
                                 data, data_len);

    utf8 = qmi_nas_read_string_from_network_description_encoded_array (encoding, array);

    if (expected_utf8) {
        g_assert_nonnull (utf8);
        g_assert_cmpstr (utf8, ==, expected_utf8);
    } else
        g_assert_null (utf8);
}

static void
test_read_string_from_network_description_encoded_array_gsm7_default_chars (void)
{
    const guint8 gsm[] = {
        0x80, 0x80, 0x60, 0x40, 0x28, 0x18, 0x0E, 0x88,
        0x84, 0x62, 0xC1, 0x68, 0x38, 0x1E, 0x90, 0x88,
        0x64, 0x42, 0xA9, 0x58, 0x2E, 0x98, 0x8C, 0x86,
        0xD3, 0xF1, 0x7C, 0x40, 0x21, 0xD1, 0x88, 0x54,
        0x32, 0x9D, 0x50, 0x29, 0xD5, 0x8A, 0xD5, 0x72,
        0xBD, 0x60, 0x31, 0xD9, 0x8C, 0x56, 0xB3, 0xDD,
        0x70, 0x39, 0xDD, 0x8E, 0xD7, 0xF3, 0xFD, 0x80,
        0x41, 0xE1, 0x90, 0x58, 0x34, 0x1E, 0x91, 0x49,
        0xE5, 0x92, 0xD9, 0x74, 0x3E, 0xA1, 0x51, 0xE9,
        0x94, 0x5A, 0xB5, 0x5E, 0xB1, 0x59, 0xED, 0x96,
        0xDB, 0xF5, 0x7E, 0xC1, 0x61, 0xF1, 0x98, 0x5C,
        0x36, 0x9F, 0xD1, 0x69, 0xF5, 0x9A, 0xDD, 0x76,
        0xBF, 0xE1, 0x71, 0xF9, 0x9C, 0x5E, 0xB7, 0xDF,
        0xF1, 0x79, 0xFD, 0x9E, 0xDF, 0xF7, 0xFF, 0x01,
    };
    const gchar *expected_utf8 = "@£$¥èéùìòÇ\nØø\rÅåΔ_ΦΓΛΩΠΨΣΘΞÆæßÉ !\"#¤%&'()*+,-./0123456789:;<=>?¡ABCDEFGHIJKLMNOPQRSTUVWXYZÄÖÑÜ§¿abcdefghijklmnopqrstuvwxyzäöñüà";

    common_test_read_string_from_network_description_encoded_array (QMI_NAS_NETWORK_DESCRIPTION_ENCODING_GSM, gsm, G_N_ELEMENTS (gsm), expected_utf8);
}

static void
test_read_string_from_network_description_encoded_array_gsm7_extended_chars (void)
{
    const guint8 gsm[] = {
        0x1B, 0xC5, 0x86, 0xB2, 0x41, 0x6D, 0x52, 0x9B,
        0xD7, 0x86, 0xB7, 0xE9, 0x6D, 0x7C, 0x1B, 0xE0,
        0xA6, 0x0C
    };
    const gchar *expected_utf8 = "\f^{}\\[~]|€";

    common_test_read_string_from_network_description_encoded_array (QMI_NAS_NETWORK_DESCRIPTION_ENCODING_GSM, gsm, G_N_ELEMENTS (gsm), expected_utf8);
}

static void
test_read_string_from_network_description_encoded_array_gsm7_mixed_chars (void)
{
    const guint8 gsm[] = {
        0x80, 0x80, 0x60, 0x40, 0x28, 0x18, 0x0E, 0x8C,
        0x8D, 0xA2, 0x62, 0xB9, 0x60, 0x32, 0x1B, 0x94,
        0x86, 0xD3, 0xF1, 0xA0, 0x36, 0xA9, 0xD4, 0x0D,
        0x97, 0xDB, 0xBC, 0x74, 0x3B, 0x5E, 0xCF, 0xB7,
        0xE1, 0xFD, 0x80, 0x51, 0xE9, 0x74, 0xE3, 0xA3,
        0x56, 0xB9, 0x1B, 0x60, 0xD7, 0xFB, 0x05, 0x87,
        0xC5, 0xF0, 0xB8, 0x7C, 0x4E, 0xAF, 0xDB, 0xF9,
        0x7D, 0xFF, 0x7F, 0x53, 0x06
    };
    const gchar *expected_utf8 = "@£$¥èéùìø\fΩΠΨΣΘ{ΞÆæß(})789\\:;<=>[?¡QRS]TUÖ|ÑÜ§¿abpqrstuvöñüà€";

    common_test_read_string_from_network_description_encoded_array (QMI_NAS_NETWORK_DESCRIPTION_ENCODING_GSM, gsm, G_N_ELEMENTS (gsm), expected_utf8);
}

static void
test_read_string_from_network_description_encoded_array_ucs2le (void)
{
    const guint8 ucs2le[] = {
        0x54, 0x00, 0x2d, 0x00, 0x4d, 0x00, 0x6f, 0x00,
        0x62, 0x00, 0x69, 0x00, 0x6c, 0x00, 0x65, 0x00
    };
    const gchar *expected_utf8 = "T-Mobile";

    common_test_read_string_from_network_description_encoded_array (QMI_NAS_NETWORK_DESCRIPTION_ENCODING_UNICODE, ucs2le, G_N_ELEMENTS (ucs2le), expected_utf8);
}

static void
test_read_string_from_network_description_encoded_array_ascii (void)
{
    const guint8 ascii[] = { 'T', '-', 'M', 'o', 'b', 'i', 'l', 'e' };
    const gchar *expected_utf8 = "T-Mobile";

    common_test_read_string_from_network_description_encoded_array (QMI_NAS_NETWORK_DESCRIPTION_ENCODING_ASCII7, ascii, G_N_ELEMENTS (ascii), expected_utf8);
}

static void
test_read_string_from_network_description_encoded_array_unspecified_ascii (void)
{
    const guint8 ascii[] = { 'T', '-', 'M', 'o', 'b', 'i', 'l', 'e' };
    const gchar *expected_utf8 = "T-Mobile";

    common_test_read_string_from_network_description_encoded_array (QMI_NAS_NETWORK_DESCRIPTION_ENCODING_UNSPECIFIED, ascii, G_N_ELEMENTS (ascii), expected_utf8);
}

static void
test_read_string_from_network_description_encoded_array_unspecified_other (void)
{
    const guint8 other[] = {
        0x54, 0x00, 0x2d, 0x00, 0x4d, 0x00, 0x6f, 0x00,
        0x62, 0x00, 0x69, 0x00, 0x6c, 0x00, 0x65, 0x00
    };

    common_test_read_string_from_network_description_encoded_array (QMI_NAS_NETWORK_DESCRIPTION_ENCODING_UNSPECIFIED, other, G_N_ELEMENTS (other), NULL);
}

static void
test_read_string_from_network_description_encoded_array_unknown (void)
{
    const guint8 other[] = {
        0x54, 0x00, 0x2d, 0x00, 0x4d, 0x00, 0x6f, 0x00,
        0x62, 0x00, 0x69, 0x00, 0x6c, 0x00, 0x65, 0x00
    };

    common_test_read_string_from_network_description_encoded_array (0xFF, other, G_N_ELEMENTS (other), NULL);
}

/******************************************************************************/

int main (int argc, char **argv)
{
    g_test_init (&argc, &argv, NULL);

    g_test_add_func ("/libqmi-glib/utils/read-string-from-plmn-encoded-array/gsm7-default-chars",  test_read_string_from_plmn_encoded_array_gsm7_default_chars);
    g_test_add_func ("/libqmi-glib/utils/read-string-from-plmn-encoded-array/gsm7-extended-chars", test_read_string_from_plmn_encoded_array_gsm7_extended_chars);
    g_test_add_func ("/libqmi-glib/utils/read-string-from-plmn-encoded-array/gsm7-mixed-chars",    test_read_string_from_plmn_encoded_array_gsm7_mixed_chars);
    g_test_add_func ("/libqmi-glib/utils/read-string-from-plmn-encoded-array/ucs2le",              test_read_string_from_plmn_encoded_array_ucs2le);

    g_test_add_func ("/libqmi-glib/utils/read-string-from-network-description-encoded-array/gsm7-default-chars",  test_read_string_from_network_description_encoded_array_gsm7_default_chars);
    g_test_add_func ("/libqmi-glib/utils/read-string-from-network-description-encoded-array/gsm7-extended-chars", test_read_string_from_network_description_encoded_array_gsm7_extended_chars);
    g_test_add_func ("/libqmi-glib/utils/read-string-from-network-description-encoded-array/gsm7-mixed-chars",    test_read_string_from_network_description_encoded_array_gsm7_mixed_chars);
    g_test_add_func ("/libqmi-glib/utils/read-string-from-network-description-encoded-array/ucs2le",              test_read_string_from_network_description_encoded_array_ucs2le);
    g_test_add_func ("/libqmi-glib/utils/read-string-from-network-description-encoded-array/ascii",               test_read_string_from_network_description_encoded_array_ascii);
    g_test_add_func ("/libqmi-glib/utils/read-string-from-network-description-encoded-array/unspecified-ascii",   test_read_string_from_network_description_encoded_array_unspecified_ascii);
    g_test_add_func ("/libqmi-glib/utils/read-string-from-network-description-encoded-array/unspecified-other",   test_read_string_from_network_description_encoded_array_unspecified_other);
    g_test_add_func ("/libqmi-glib/utils/read-string-from-network-description-encoded-array/unknown",             test_read_string_from_network_description_encoded_array_unknown);

    return g_test_run ();
}
