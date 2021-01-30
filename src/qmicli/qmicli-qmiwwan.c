/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * qmicli -- Command line interface to control QMI devices
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright (C) 2021 Aleksander Morgado <aleksander@aleksander.es>
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <string.h>

#include <glib.h>
#include <glib/gprintf.h>
#include <gio/gio.h>

#include <libqmi-glib.h>

#include "qmicli.h"
#include "qmicli-helpers.h"

/* Options */
static gboolean  get_wwan_iface_flag;
static gboolean  get_expected_data_format_flag;
static gchar    *set_expected_data_format_str;

static GOptionEntry entries[] = {
    { "get-wwan-iface", 'w', 0, G_OPTION_ARG_NONE, &get_wwan_iface_flag,
      "Get the associated WWAN iface name",
      NULL
    },
    { "get-expected-data-format", 'e', 0, G_OPTION_ARG_NONE, &get_expected_data_format_flag,
      "Get the expected data format in the WWAN iface",
      NULL
    },
    { "set-expected-data-format", 'E', 0, G_OPTION_ARG_STRING, &set_expected_data_format_str,
      "Set the expected data format in the WWAN iface",
      "[802-3|raw-ip|qmap-pass-through]"
    },
    { NULL }
};

GOptionGroup *
qmicli_qmiwwan_get_option_group (void)
{
    GOptionGroup *group;

    group = g_option_group_new ("qmiwwan",
                                "qmi_wwan specific options:",
                                "Show qmi_wwan driver specific options", NULL, NULL);
    g_option_group_add_entries (group, entries);

    return group;
}

gboolean
qmicli_qmiwwan_options_enabled (void)
{
    static guint n_actions = 0;
    static gboolean checked = FALSE;

    if (checked)
        return !!n_actions;

    n_actions = (get_wwan_iface_flag +
                 get_expected_data_format_flag +
                 !!set_expected_data_format_str);

    if (n_actions > 1) {
        g_printerr ("error: too many qmi_wwan specific actions requested\n");
        exit (EXIT_FAILURE);
    }

    checked = TRUE;
    return !!n_actions;
}

/******************************************************************************/

static gboolean
device_set_expected_data_format_cb (QmiDevice *dev)
{
    QmiDeviceExpectedDataFormat expected;
    GError *error = NULL;

    if (!qmicli_read_device_expected_data_format_from_string (set_expected_data_format_str, &expected) ||
        expected == QMI_DEVICE_EXPECTED_DATA_FORMAT_UNKNOWN)
        g_printerr ("error: invalid requested data format: %s", set_expected_data_format_str);
    else if (!qmi_device_set_expected_data_format (dev, expected, &error)) {
        g_printerr ("error: cannot set expected data format: %s\n", error->message);
        g_error_free (error);
    } else
        g_print ("[%s] expected data format set to: %s\n",
                 qmi_device_get_path_display (dev),
                 qmi_device_expected_data_format_get_string (expected));

    /* We're done now */
    qmicli_async_operation_done (!error, FALSE);

    g_object_unref (dev);
    return FALSE;
}

static void
device_set_expected_data_format (QmiDevice *dev)
{
    g_debug ("Setting expected WWAN data format this control port...");
    g_idle_add ((GSourceFunc) device_set_expected_data_format_cb, g_object_ref (dev));
}

static gboolean
device_get_expected_data_format_cb (QmiDevice *dev)
{
    QmiDeviceExpectedDataFormat expected;
    GError *error = NULL;

    expected = qmi_device_get_expected_data_format (dev, &error);
    if (expected == QMI_DEVICE_EXPECTED_DATA_FORMAT_UNKNOWN) {
        g_printerr ("error: cannot get expected data format: %s\n", error->message);
        g_error_free (error);
    } else
        g_print ("%s\n", qmi_device_expected_data_format_get_string (expected));

    /* We're done now */
    qmicli_async_operation_done (!error, FALSE);

    g_object_unref (dev);
    return FALSE;
}

static void
device_get_expected_data_format (QmiDevice *dev)
{
    g_debug ("Getting expected WWAN data format this control port...");
    g_idle_add ((GSourceFunc) device_get_expected_data_format_cb, g_object_ref (dev));
}

static gboolean
device_get_wwan_iface_cb (QmiDevice *dev)
{
    const gchar *wwan_iface;

    wwan_iface = qmi_device_get_wwan_iface (dev);
    if (!wwan_iface)
        g_printerr ("error: cannot get WWAN interface name\n");
    else
        g_print ("%s\n", wwan_iface);

    /* We're done now */
    qmicli_async_operation_done (!!wwan_iface, FALSE);

    g_object_unref (dev);
    return FALSE;
}

static void
device_get_wwan_iface (QmiDevice *dev)
{
    g_debug ("Getting WWAN iface for this control port...");
    g_idle_add ((GSourceFunc) device_get_wwan_iface_cb, g_object_ref (dev));
}

/******************************************************************************/
/* Common */

void
qmicli_qmiwwan_run (QmiDevice    *device,
                    GCancellable *cancellable)
{
    if (get_wwan_iface_flag)
        device_get_wwan_iface (device);
    else if (get_expected_data_format_flag)
        device_get_expected_data_format (device);
    else if (set_expected_data_format_str)
        device_set_expected_data_format (device);
    else
      g_warn_if_reached ();
}
