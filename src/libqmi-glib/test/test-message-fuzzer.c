/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2022 Google, Inc.
 */

#include <config.h>
#include <string.h>
#include <stdint.h>
#include <glib.h>

#include "qmi-message.h"

int
LLVMFuzzerTestOneInput (const uint8_t *data,
                        size_t         size)
{
    g_autoptr(GByteArray) bytearray = NULL;
    g_autoptr(QmiMessage) message = NULL;
    g_autoptr(GError)     error = NULL;

    if (!size)
      return 0;

    bytearray = g_byte_array_append (g_byte_array_sized_new (size), data, size);
    message = qmi_message_new_from_raw (bytearray, &error);
    return 0;
}
