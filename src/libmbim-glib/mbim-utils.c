/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * libmbim-glib -- GLib/GIO based library to control MBIM devices
 *
 * Copyright (C) 2013 - 2019 Aleksander Morgado <aleksander@aleksander.es>
 */

#include "mbim-utils.h"

/**
 * SECTION:mbim-utils
 * @title: Common utilities
 * @short_description: Common utilities in the libmbim-glib library.
 *
 * This section exposes a set of common utilities that may be used to work
 * with the libmbim-glib library.
 */

static volatile gint __traces_enabled = FALSE;

gboolean
mbim_utils_get_traces_enabled (void)
{
    return (gboolean) g_atomic_int_get (&__traces_enabled);
}

void
mbim_utils_set_traces_enabled (gboolean enabled)
{
    g_atomic_int_set (&__traces_enabled, enabled);
}
