/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 * libmbim-glib -- GLib/GIO based library to control MBIM devices
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 * Copyright (C) 2013 - 2014 Aleksander Morgado <aleksander@aleksander.es>
 */

#include <config.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <pwd.h>

#include "mbim-utils.h"
#include "mbim-error-types.h"

/**
 * SECTION:mbim-utils
 * @title: Common utilities
 *
 * This section exposes a set of common utilities that may be used to work
 * with the MBIM library.
 **/

/*****************************************************************************/
gboolean
__mbim_user_allowed (uid_t uid,
                     GError **error)
{
#ifndef MBIM_USERNAME_ENABLED
    if (uid == 0)
        return TRUE;
#else
# ifndef MBIM_USERNAME
#  error MBIM username not defined
# endif

    struct passwd *expected_usr = NULL;

    /* Root user is always allowed, regardless of the specified MBIM_USERNAME */
    if (uid == 0)
        return TRUE;

    expected_usr = getpwnam (MBIM_USERNAME);
    if (!expected_usr) {
        g_set_error (error,
                     MBIM_CORE_ERROR,
                     MBIM_CORE_ERROR_FAILED,
                     "Not enough privileges (unknown username %s)", MBIM_USERNAME);
        return FALSE;
    }

    if (uid == expected_usr->pw_uid)
        return TRUE;
#endif

    g_set_error (error,
                 MBIM_CORE_ERROR,
                 MBIM_CORE_ERROR_FAILED,
                 "Not enough privileges");
    return FALSE;
}

/*****************************************************************************/

static volatile gint __traces_enabled = FALSE;

/**
 * mbim_utils_get_traces_enabled:
 *
 * Checks whether MBIM message traces are currently enabled.
 *
 * Returns: %TRUE if traces are enabled, %FALSE otherwise.
 */
gboolean
mbim_utils_get_traces_enabled (void)
{
    return (gboolean) g_atomic_int_get (&__traces_enabled);
}

/**
 * mbim_utils_set_traces_enabled:
 * @enabled: %TRUE to enable traces, %FALSE to disable them.
 *
 * Sets whether MBIM message traces are enabled or disabled.
 */
void
mbim_utils_set_traces_enabled (gboolean enabled)
{
    g_atomic_int_set (&__traces_enabled, enabled);
}
