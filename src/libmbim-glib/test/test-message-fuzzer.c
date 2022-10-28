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
    g_autoptr(MbimMessage) message = NULL;
    g_autoptr(GError)      error = NULL;

    if (!size)
      return 0;

    message = mbim_message_new (data, size);
    mbim_message_validate (message, &error);
    return 0;
}
