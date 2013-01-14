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

#include "mbim-uuid.h"

static void
compare_uuid_strings (const MbimUuid *uuid,
                      const gchar *other_uuid_str)
{
    gchar *uuid_str;

    uuid_str = mbim_uuid_get_printable (uuid);
    g_assert_cmpstr (uuid_str, ==, other_uuid_str);
    g_free (uuid_str);
}

static void
test_uuid_basic_connect (void)
{
    compare_uuid_strings (MBIM_UUID_BASIC_CONNECT,
                          "a289cc33-bcbb-8b4f-b6b0-133ec2aae6df");
}

static void
test_uuid_sms (void)
{
    compare_uuid_strings (MBIM_UUID_SMS,
                          "533fbeeb-14fe-4467-9f90-33a223e56c3f");
}

static void
test_uuid_ussd (void)
{
    compare_uuid_strings (MBIM_UUID_USSD,
                          "e550a0c8-5e82-479e-82f7-10abf4c3351f");
}

static void
test_uuid_phonebook (void)
{
    compare_uuid_strings (MBIM_UUID_PHONEBOOK,
                          "4bf38476-1e6a-41db-b1d8-bed289c25bdb");
}

static void
test_uuid_stk (void)
{
    compare_uuid_strings (MBIM_UUID_STK,
                          "d8f20131-fcb5-4e17-8602-d6ed3816164c");
}

static void
test_uuid_auth (void)
{
    compare_uuid_strings (MBIM_UUID_AUTH,
                          "1d2b5ff7-0aa1-48b2-aa52-50f15767174e");
}

static void
test_uuid_dss (void)
{
    compare_uuid_strings (MBIM_UUID_DSS,
                          "c08a26dd-7718-4382-8482-6e0d583c4d0e");
}

int main (int argc, char **argv)
{
    g_test_init (&argc, &argv, NULL);

    g_test_add_func ("/libmbim-glib/uuid/basic-connect", test_uuid_basic_connect);
    g_test_add_func ("/libmbim-glib/uuid/sms",           test_uuid_sms);
    g_test_add_func ("/libmbim-glib/uuid/ussd",          test_uuid_ussd);
    g_test_add_func ("/libmbim-glib/uuid/phonebook",     test_uuid_phonebook);
    g_test_add_func ("/libmbim-glib/uuid/stk",           test_uuid_stk);
    g_test_add_func ("/libmbim-glib/uuid/auth",          test_uuid_auth);
    g_test_add_func ("/libmbim-glib/uuid/dss",           test_uuid_dss);

    return g_test_run ();
}
