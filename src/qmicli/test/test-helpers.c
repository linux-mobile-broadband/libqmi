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
 * Copyright (C) 2012-2016 Aleksander Morgado <aleksander@aleksander.es>
 */

#include <string.h>
#include <glib.h>
#include "qmicli-helpers.h"

/******************************************************************************/

static void
test_helpers_raw_printable_1 (void)
{
    GArray *array;
    gchar *printable;
    static guint8 buffer[8] = {
        0x0F, 0x50, 0xEB, 0xE2, 0xB6, 0x00, 0x00, 0x00
    };
    static const gchar *expected =
        "0F:\n"
        "50:\n"
        "EB:\n"
        "E2:\n"
        "B6:\n"
        "00:\n"
        "00:\n"
        "00\n";

    array = g_array_sized_new (FALSE, FALSE, 1, 8);
    g_array_insert_vals (array, 0, buffer, 8);

    printable = qmicli_get_raw_data_printable (array, 3, "");

    g_assert_cmpstr (printable, ==, expected);
    g_free (printable);
    g_array_unref (array);
}

static void
test_helpers_raw_printable_2 (void)
{
    GArray *array;
    gchar *printable;
    static guint8 buffer[8] = {
        0x0F, 0x50, 0xEB, 0xE2, 0xB6, 0x00, 0x00, 0x00
    };
    static const gchar *expected =
        "\t0F:50:\n"
        "\tEB:E2:\n"
        "\tB6:00:\n"
        "\t00:00\n";

    array = g_array_sized_new (FALSE, FALSE, 1, 8);
    g_array_insert_vals (array, 0, buffer, 8);

    /* When passing 7, we'll be really getting 6 (the closest lower multiple of 3) */
    printable = qmicli_get_raw_data_printable (array, 7, "\t");

    g_assert_cmpstr (printable, ==, expected);
    g_free (printable);
    g_array_unref (array);
}

static void
test_helpers_raw_printable_3 (void)
{
    GArray *array;
    gchar *printable;
    static guint8 buffer[8] = {
        0x0F, 0x50, 0xEB, 0xE2, 0xB6, 0x00, 0x00, 0x00
    };
    static const gchar *expected =
        "\t\t\t0F:50:EB:E2:\n"
        "\t\t\tB6:00:00:00\n";

    array = g_array_sized_new (FALSE, FALSE, 1, 8);
    g_array_insert_vals (array, 0, buffer, 8);

    printable = qmicli_get_raw_data_printable (array, 12, "\t\t\t");

    g_assert_cmpstr (printable, ==, expected);
    g_free (printable);
    g_array_unref (array);
}

static void
test_helpers_raw_printable_4 (void)
{
    GArray *array;
    gchar *printable;
    static guint8 buffer[8] = {
        0x0F, 0x50, 0xEB, 0xE2, 0xB6, 0x00, 0x00, 0x00
    };
    static const gchar *expected =
        "\t0F:50:EB:E2:B6:00:00:00\n";

    array = g_array_sized_new (FALSE, FALSE, 1, 8);
    g_array_insert_vals (array, 0, buffer, 8);

    printable = qmicli_get_raw_data_printable (array, 24, "\t");

    g_assert_cmpstr (printable, ==, expected);
    g_free (printable);
    g_array_unref (array);
}

/******************************************************************************/

static void
test_helpers_binary_array_from_string_0 (void)
{
    const guint8 expected[] = {
        0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0, 0xAB, 0xCD, 0xEF
    };
    const gchar *str = "12:34:56:78:9A:BC:DE:F0:ab:cd:ef";
    GArray *out = NULL;

    g_assert (qmicli_read_binary_array_from_string (str, &out));
    g_assert (out != NULL);
    g_assert_cmpmem ((guint8 *)(out->data), out->len, expected, G_N_ELEMENTS (expected));
    g_array_unref (out);
}

static void
test_helpers_binary_array_from_string_1 (void)
{
    const guint8 expected[] = {
        0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0, 0xAB, 0xCD, 0xEF
    };
    const gchar *str = "123456789ABCDEF0abcdef";
    GArray *out = NULL;

    g_assert (qmicli_read_binary_array_from_string (str, &out));
    g_assert (out != NULL);
    g_assert_cmpmem ((guint8 *)(out->data), out->len, expected, G_N_ELEMENTS (expected));
    g_array_unref (out);
}

static void
test_helpers_binary_array_from_string_2 (void)
{
    const gchar *str = "";
    GArray *out = NULL;

    g_assert (qmicli_read_binary_array_from_string (str, &out));
    g_assert (out != NULL);
    g_assert_cmpuint (out->len, ==, 0);
    g_array_unref (out);
}

static void
test_helpers_binary_array_from_string_3 (void)
{
    const gchar *str = "hello";
    GArray *out = NULL;

    g_assert (qmicli_read_binary_array_from_string (str, &out) == FALSE);
}

static void
test_helpers_binary_array_from_string_4 (void)
{
    const gchar *str = "a";
    GArray *out = NULL;

    g_assert (qmicli_read_binary_array_from_string (str, &out) == FALSE);
}

/******************************************************************************/

static void
test_helpers_supported_messages_list (void)
{
    const guint8 bytearray[] = {
        0x03, 0x00, 0x00, 0xC0
    };
    const gchar *expected_str =
        "\t0x0000\n"  /*  0 dec */
        "\t0x0001\n"  /*  1 dec */
        "\t0x001E\n"  /* 30 dec */
        "\t0x001F\n"; /* 31 dec */
    gchar *str;

    str = qmicli_get_supported_messages_list (bytearray, G_N_ELEMENTS (bytearray));
    g_assert (str);
    g_assert_cmpstr (str, ==, expected_str);
    g_free (str);
}

static void
test_helpers_supported_messages_list_none (void)
{
    const gchar *expected_str = "\tnone\n";
    gchar *str;

    str = qmicli_get_supported_messages_list (NULL, 0);
    g_assert (str);
    g_assert_cmpstr (str, ==, expected_str);
    g_free (str);
}

/******************************************************************************/

typedef struct {
    const gchar *key;
    const gchar *value;
    gboolean     found;
} KeyValue;

static const KeyValue test_key_values[] = {
    { "key1", "",          FALSE },
    { "key2", "value",     FALSE },
    { "key3", "1234",      FALSE },
    { "key4", "value1234", FALSE },
};

static gboolean
key_value_callback (const gchar   *key,
                    const gchar   *value,
                    GError       **error,
                    gboolean      *found)
{
    guint i;

    for (i = 0; i < G_N_ELEMENTS (test_key_values); i++) {
        if (!g_str_equal (test_key_values[i].key, key))
            continue;
        if (!g_str_equal (test_key_values[i].value, value))
            continue;

        /* Must not be found multiple times */
        g_assert (!found[i]);
        found[i] = TRUE;
        return TRUE;
    }

    g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                 "Key/value '%s/%s' pair not expected",
                 key, value);
    return FALSE;
}

static void
common_validate_key_value (const gchar *str)
{
    gboolean  found[G_N_ELEMENTS (test_key_values)] = { FALSE };
    gboolean  result;
    GError   *error = NULL;
    guint     i;

    result = qmicli_parse_key_value_string (str,
                                            &error,
                                            (QmiParseKeyValueForeachFn) key_value_callback,
                                            &found[0]);
    g_assert_no_error (error);
    g_assert (result);

    for (i = 0; i < G_N_ELEMENTS (test_key_values); i++)
        g_assert (found[i]);
}

static void
test_parse_key_value_string_no_quotes (void)
{
    common_validate_key_value ("key1=,key2=value,key3=1234,key4=value1234");
}

static void
test_parse_key_value_string_single_quotes (void)
{
    common_validate_key_value ("key1='',key2='value',key3='1234',key4='value1234'");
}

static void
test_parse_key_value_string_double_quotes (void)
{
    common_validate_key_value ("key1=\"\",key2=\"value\",key3=\"1234\",key4=\"value1234\"");
}

static void
test_parse_key_value_string_mixed_quotes (void)
{
    common_validate_key_value ("key1=\"\",key2='value',key3=1234,key4=\"value1234\"");
}

/******************************************************************************/

int main (int argc, char **argv)
{
    g_test_init (&argc, &argv, NULL);

    g_test_add_func ("/qmicli/helpers/raw-printable/1",  test_helpers_raw_printable_1);
    g_test_add_func ("/qmicli/helpers/raw-printable/2",  test_helpers_raw_printable_2);
    g_test_add_func ("/qmicli/helpers/raw-printable/3",  test_helpers_raw_printable_3);
    g_test_add_func ("/qmicli/helpers/raw-printable/4",  test_helpers_raw_printable_4);

    g_test_add_func ("/qmicli/helpers/binary-array-from-string/0", test_helpers_binary_array_from_string_0);
    g_test_add_func ("/qmicli/helpers/binary-array-from-string/1", test_helpers_binary_array_from_string_1);
    g_test_add_func ("/qmicli/helpers/binary-array-from-string/2", test_helpers_binary_array_from_string_2);
    g_test_add_func ("/qmicli/helpers/binary-array-from-string/3", test_helpers_binary_array_from_string_3);
    g_test_add_func ("/qmicli/helpers/binary-array-from-string/4", test_helpers_binary_array_from_string_4);

    g_test_add_func ("/qmicli/helpers/supported-message-list",      test_helpers_supported_messages_list);
    g_test_add_func ("/qmicli/helpers/supported-message-list/none", test_helpers_supported_messages_list_none);

    g_test_add_func ("/qmicli/helpers/key-value/no-quotes",     test_parse_key_value_string_no_quotes);
    g_test_add_func ("/qmicli/helpers/key-value/single-quotes", test_parse_key_value_string_single_quotes);
    g_test_add_func ("/qmicli/helpers/key-value/double-quotes", test_parse_key_value_string_double_quotes);
    g_test_add_func ("/qmicli/helpers/key-value/mixed-quotes",  test_parse_key_value_string_mixed_quotes);

    return g_test_run ();
}
