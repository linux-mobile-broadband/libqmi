/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 * libqmi-glib -- GLib/GIO based library to control QMI devices
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
 * Copyright (C) 2012-2015 Dan Williams <dcbw@redhat.com>
 * Copyright (C) 2012-2017 Aleksander Morgado <aleksander@aleksander.es>
 */

#include <config.h>

#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <pwd.h>

#include "qmi-utils.h"
#include "qmi-error-types.h"

/**
 * SECTION:qmi-utils
 * @title: Common utilities
 *
 * This section exposes a set of common utilities that may be used to work
 * with the QMI library.
 **/

/*****************************************************************************/

gchar *
__qmi_utils_str_hex (gconstpointer mem,
                     gsize size,
                     gchar delimiter)
{
    const guint8 *data = mem;
    gsize i;
    gsize j;
    gsize new_str_length;
    gchar *new_str;

    /* Get new string length. If input string has N bytes, we need:
     * - 1 byte for last NUL char
     * - 2N bytes for hexadecimal char representation of each byte...
     * - N-1 bytes for the separator ':'
     * So... a total of (1+2N+N-1) = 3N bytes are needed... */
    new_str_length =  3 * size;

    /* Allocate memory for new array and initialize contents to NUL */
    new_str = g_malloc0 (new_str_length);

    /* Print hexadecimal representation of each byte... */
    for (i = 0, j = 0; i < size; i++, j += 3) {
        /* Print character in output string... */
        snprintf (&new_str[j], 3, "%02X", data[i]);
        /* And if needed, add separator */
        if (i != (size - 1) )
            new_str[j + 2] = delimiter;
    }

    /* Set output string */
    return new_str;
}

/*****************************************************************************/

gboolean
__qmi_user_allowed (uid_t uid,
                    GError **error)
{
#ifndef QMI_USERNAME_ENABLED
    if (uid == 0)
        return TRUE;
#else
# ifndef QMI_USERNAME
#  error QMI username not defined
# endif

    struct passwd *expected_usr = NULL;

    /* Root user is always allowed, regardless of the specified QMI_USERNAME */
    if (uid == 0)
        return TRUE;

    expected_usr = getpwnam (QMI_USERNAME);
    if (!expected_usr) {
        g_set_error (error,
                     QMI_CORE_ERROR,
                     QMI_CORE_ERROR_FAILED,
                     "Not enough privileges (unknown username %s)", QMI_USERNAME);
        return FALSE;
    }

    if (uid == expected_usr->pw_uid)
        return TRUE;
#endif

    g_set_error (error,
                 QMI_CORE_ERROR,
                 QMI_CORE_ERROR_FAILED,
                 "Not enough privileges");
    return FALSE;
}

/*****************************************************************************/

gchar *
__qmi_utils_get_driver (const gchar *cdc_wdm_path)
{
    static const gchar *subsystems[] = { "usbmisc", "usb" };
    guint i;
    gchar *device_basename;
    gchar *driver = NULL;

    device_basename = g_path_get_basename (cdc_wdm_path);

    for (i = 0; !driver && i < G_N_ELEMENTS (subsystems); i++) {
        gchar *tmp;
        gchar *path;

        /* driver sysfs can be built directly using subsystem and name; e.g. for subsystem
         * usbmisc and name cdc-wdm0:
         *    $ realpath /sys/class/usbmisc/cdc-wdm0/device/driver
         *    /sys/bus/usb/drivers/qmi_wwan
         */
        tmp = g_strdup_printf ("/sys/class/%s/%s/device/driver", subsystems[i], device_basename);
        path = realpath (tmp, NULL);
        g_free (tmp);

        if (!path)
            continue;

        driver = g_path_get_basename (path);
        g_free (path);
    }

    g_free (device_basename);

    return driver;
}

/*****************************************************************************/

static volatile gint __traces_enabled = FALSE;

gboolean
qmi_utils_get_traces_enabled (void)
{
    return (gboolean) g_atomic_int_get (&__traces_enabled);
}

void
qmi_utils_set_traces_enabled (gboolean enabled)
{
    g_atomic_int_set (&__traces_enabled, enabled);
}
