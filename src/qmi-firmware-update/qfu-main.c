/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * qmi-firmware-update -- Command line tool to update firmware in QMI devices
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
 * Copyright (C) 2016 Zodiac Inflight Innovation
 * Copyright (C) 2016 Aleksander Morgado <aleksander@aleksander.es>
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

#include "qfu-operation.h"

#define PROGRAM_NAME    "qmi-firmware-update"
#define PROGRAM_VERSION PACKAGE_VERSION

/*****************************************************************************/
/* Options */

/* Download */
static gchar     *device_str;
static gchar    **image_strv;
static gchar     *firmware_version_str;
static gchar     *config_version_str;
static gchar     *carrier_str;
static gboolean   device_open_proxy_flag;
static gboolean   device_open_mbim_flag;

/* Verify */
static gchar *verify_image_str;

/* Main */
static gboolean verbose_flag;
static gboolean silent_flag;
static gboolean version_flag;
static gboolean help_flag;

static GOptionEntry context_download_entries[] = {
    { "device", 'd', 0, G_OPTION_ARG_FILENAME, &device_str,
      "Specify device path.",
      "[PATH]"
    },
    { "image", 'i', 0, G_OPTION_ARG_FILENAME_ARRAY, &image_strv,
      "Specify image to download to the device. May be given multiple times.",
      "[PATH]"
    },
    { "firmware-version", 'f', 0, G_OPTION_ARG_STRING, &firmware_version_str,
      "Firmware version (e.g. '05.05.58.00').",
      "[VERSION]",
    },
    { "config-version", 'c', 0, G_OPTION_ARG_STRING, &config_version_str,
      "Config version (e.g. '005.025_002').",
      "[VERSION]",
    },
    { "carrier", 'C', 0, G_OPTION_ARG_STRING, &carrier_str,
      "Carrier name (e.g. 'Generic')",
      "[CARRIER]",
    },
    { "device-open-proxy", 'p', 0, G_OPTION_ARG_NONE, &device_open_proxy_flag,
      "Request to use the 'qmi-proxy' proxy.",
      NULL
    },
    { "device-open-mbim", 0, 0, G_OPTION_ARG_NONE, &device_open_mbim_flag,
      "Open an MBIM device with EXT_QMUX support.",
      NULL
    },
    { NULL }
};

static GOptionEntry context_verify_entries[] = {
    { "verify-image", 'z', 0, G_OPTION_ARG_FILENAME, &verify_image_str,
      "Specify image to verify.",
      "[PATH]"
    },
    { NULL }
};

static GOptionEntry context_main_entries[] = {
    { "verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose_flag,
      "Run action with verbose logs, including the debug ones.",
      NULL
    },
    { "silent", 0, 0, G_OPTION_ARG_NONE, &silent_flag,
      "Run action with no logs; not even the error/warning ones.",
      NULL
    },
    { "version", 'V', 0, G_OPTION_ARG_NONE, &version_flag,
      "Print version.",
      NULL
    },
    { "help", 'h', 0, G_OPTION_ARG_NONE, &help_flag,
      "Show help",
      NULL
    },
    { NULL }
};

static const gchar *context_description =
    " E.g. a download operation:\n"
    " $ sudo " PROGRAM_NAME " \\\n"
    "       --verbose \\\n"
    "       --device /dev/cdc-wdm4 \\\n"
    "       --firmware-version 05.05.58.00 \\\n"
    "       --config-version 005.025_002 \\\n"
    "       --carrier Generic \\\n"
    "       --image SWI9X15C_05.05.58.00.cwe \\\n"
    "       --image SWI9X15C_05.05.58.00_Generic_005.025_002.nvu\n"
    "\n"
    " E.g. a verify operation:\n"
    " $ sudo " PROGRAM_NAME " \\\n"
    "       --verbose \\\n"
    "       --verify-image SWI9X15C_05.05.58.00.cwe\n"
    "\n";

/*****************************************************************************/
/* Logging output */

static void
log_handler (const gchar    *log_domain,
             GLogLevelFlags  log_level,
             const gchar    *message,
             gpointer        user_data)
{
    const gchar *log_level_str;
    time_t       now;
    gchar        time_str[64];
    struct tm   *local_time;
    gboolean     err;

    /* Nothing to do if we're silent */
    if (silent_flag)
        return;

    now = time ((time_t *) NULL);
    local_time = localtime (&now);
    strftime (time_str, 64, "%d %b %Y, %H:%M:%S", local_time);
    err = FALSE;

    switch (log_level) {
    case G_LOG_LEVEL_WARNING:
        log_level_str = "-Warning **";
        err = TRUE;
        break;

    case G_LOG_LEVEL_CRITICAL:
    case G_LOG_FLAG_FATAL:
    case G_LOG_LEVEL_ERROR:
        log_level_str = "-Error **";
        err = TRUE;
        break;

    case G_LOG_LEVEL_DEBUG:
        log_level_str = "[Debug]";
        break;

    default:
        log_level_str = "";
        break;
    }

    if (!verbose_flag && !err)
        return;

    g_fprintf (err ? stderr : stdout,
               "[%s] %s %s\n",
               time_str,
               log_level_str,
               message);
}

/*****************************************************************************/

static void
print_version (void)
{
    g_print ("\n"
             PROGRAM_NAME " " PROGRAM_VERSION "\n"
             "Copyright (C) 2016 Bjørn Mork\n"
             "Copyright (C) 2016 Zodiac Inflight Innovations\n"
             "Copyright (C) 2016 Aleksander Morgado\n"
             "License GPLv2+: GNU GPL version 2 or later <http://gnu.org/licenses/gpl-2.0.html>\n"
             "This is free software: you are free to change and redistribute it.\n"
             "There is NO WARRANTY, to the extent permitted by law.\n"
             "\n");
}

static void
print_help (GOptionContext *context)
{
    gchar *str;

    /* Always print --help-all */
    str = g_option_context_get_help (context, FALSE, NULL);
    g_print ("%s", str);
    g_free (str);
}

int main (int argc, char **argv)
{
    GError         *error = NULL;
    GOptionContext *context;
    GOptionGroup   *group;
    guint           n_actions;
    gboolean        result = FALSE;

    setlocale (LC_ALL, "");

    g_type_init ();

    /* Setup option context, process it and destroy it */
    context = g_option_context_new ("- Update firmware in QMI devices");

    group = g_option_group_new ("download", "Download options", "", NULL, NULL);
    g_option_group_add_entries (group, context_download_entries);
    g_option_context_add_group (context, group);

    group = g_option_group_new ("verify", "Verify options", "", NULL, NULL);
    g_option_group_add_entries (group, context_verify_entries);
    g_option_context_add_group (context, group);

    g_option_context_add_main_entries (context, context_main_entries, NULL);
    g_option_context_set_description  (context, context_description);
    g_option_context_set_help_enabled (context, FALSE);

    if (!g_option_context_parse (context, &argc, &argv, &error)) {
        g_printerr ("error: couldn't parse option context: %s\n", error->message);
        g_error_free (error);
        goto out;
    }

    if (version_flag) {
        print_version ();
        result = TRUE;
        goto out;
    }

    if (help_flag) {
        print_help (context);
        result = TRUE;
        goto out;
    }

    g_log_set_handler (NULL, G_LOG_LEVEL_MASK, log_handler, NULL);
    g_log_set_handler ("Qmi", G_LOG_LEVEL_MASK, log_handler, NULL);
    if (verbose_flag)
        qmi_utils_set_traces_enabled (TRUE);

    /* We don't allow multiple actions at the same time */
    n_actions = (!!verify_image_str + !!image_strv);
    if (n_actions == 0) {
        g_printerr ("error: no actions specified\n");
        goto out;
    }
    if (n_actions > 1) {
        g_printerr ("error: too many actions specified\n");
        goto out;
    }

    /* Run operation */
    if (image_strv)
        result = qfu_operation_download_run (device_str,
                                             firmware_version_str,
                                             config_version_str,
                                             carrier_str,
                                             (const gchar **) image_strv,
                                             device_open_proxy_flag,
                                             device_open_mbim_flag);
    else if (verify_image_str)
        result = qfu_operation_verify_run (verify_image_str);
    else
        g_assert_not_reached ();

out:
    /* Clean exit for a clean memleak report */
    if (context)
        g_option_context_free (context);

    return (result ? EXIT_SUCCESS : EXIT_FAILURE);
}
