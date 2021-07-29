/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2017 Google Inc.
 */

#include <config.h>

#include "mbim-common.h"

static void
test_common_str_hex (void)
{
    static const guint8 buffer [] = { 0x00, 0xDE, 0xAD, 0xC0, 0xDE };
    gchar *str;

    str = mbim_common_str_hex (NULL, 0, ':');
    g_assert (str == NULL);

    str = mbim_common_str_hex (buffer, 0, ':');
    g_assert (str == NULL);

    str = mbim_common_str_hex (buffer, 1, ':');
    g_assert_cmpstr (str, ==, "00");
    g_free (str);

    str = mbim_common_str_hex (buffer, 2, '-');
    g_assert_cmpstr (str, ==, "00-DE");
    g_free (str);

    str = mbim_common_str_hex (buffer, 5, '.');
    g_assert_cmpstr (str, ==, "00.DE.AD.C0.DE");
    g_free (str);
}

/*****************************************************************************/

int main (int argc, char **argv)
{
    g_test_init (&argc, &argv, NULL);

    g_test_add_func ("/common/str_hex", test_common_str_hex);

    return g_test_run ();
}
