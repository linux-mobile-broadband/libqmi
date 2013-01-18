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

#include "mbim-message.h"

static void
test_message_open (void)
{
    MbimMessage *message;

    message = mbim_message_open_new (12345, 4096);
    g_assert (message != NULL);

    g_assert_cmpuint (mbim_message_get_transaction_id            (message), ==, 12345);
    g_assert_cmpuint (mbim_message_get_message_type              (message), ==, MBIM_MESSAGE_TYPE_OPEN);
    g_assert_cmpuint (mbim_message_get_message_length            (message), ==, 16);
    g_assert_cmpuint (mbim_message_open_get_max_control_transfer (message), ==, 4096);

    mbim_message_unref (message);
}

static void
test_message_open_done (void)
{
    MbimMessage *message;
    const guint8 buffer [] =  { 0x01, 0x00, 0x00, 0x80,
                                0x10, 0x00, 0x00, 0x00,
                                0x01, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00 };

    message = mbim_message_new (buffer, sizeof (buffer));

    g_assert_cmpuint (mbim_message_get_transaction_id        (message), ==, 1);
    g_assert_cmpuint (mbim_message_get_message_type          (message), ==, MBIM_MESSAGE_TYPE_OPEN_DONE);
    g_assert_cmpuint (mbim_message_get_message_length        (message), ==, 16);
    g_assert_cmpuint (mbim_message_open_done_get_status_code (message), ==, MBIM_STATUS_ERROR_NONE);

    mbim_message_unref (message);
}

int main (int argc, char **argv)
{
    g_test_init (&argc, &argv, NULL);

    g_test_add_func ("/libmbim-glib/message/open",       test_message_open);
    g_test_add_func ("/libmbim-glib/message/open-done",  test_message_open_done);

    return g_test_run ();
}
