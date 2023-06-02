/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2022 Google, Inc.
 */

#include <config.h>
#include <string.h>
#include <stdint.h>
#include <glib.h>

#include "mbim-message.h"

int
LLVMFuzzerTestOneInput (const uint8_t *data,
                        size_t         size)
{
    g_autofree gchar       *printable = NULL;
    g_autoptr(MbimMessage)  message = NULL;
    g_autoptr(GError)       error = NULL;

    if (!size)
      return 0;

    message = mbim_message_new (data, size);
    if (!mbim_message_validate (message, &error)) {
        g_printerr ("error: %s\n", error->message);
        return 0;
    }

    printable = mbim_message_get_printable_full (message, 1, 0, "---- ", FALSE, &error);
    if (!printable) {
        g_printerr ("error: %s\n", error->message);
        return 0;
    }

    g_print ("%s\n", printable);
    return 0;
}
