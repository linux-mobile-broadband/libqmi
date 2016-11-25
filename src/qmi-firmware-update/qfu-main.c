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
#include <glib-unix.h>
#include <gio/gio.h>

#include <libqmi-glib.h>

#include "qfu-updater.h"

#define PROGRAM_NAME    "qmi-firmware-update"
#define PROGRAM_VERSION PACKAGE_VERSION

/*****************************************************************************/
/* Main options */

static gchar     *device_str;
static gchar    **image_strv;
static gchar     *firmware_version_str;
static gchar     *config_version_str;
static gchar     *carrier_str;
static gboolean   device_open_proxy_flag;
static gboolean   device_open_mbim_flag;
static gboolean   verbose_flag;
static gboolean   silent_flag;
static gboolean   version_flag;

static GOptionEntry context_main_entries[] = {
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
    { NULL }
};

static const gchar *context_description =
    " E.g.:\n"
    " $ sudo " PROGRAM_NAME " \\\n"
    "       --verbose \\\n"
    "       --device /dev/cdc-wdm4 \\\n"
    "       --firmware-version 05.05.58.00 \\\n"
    "       --config-version 005.025_002 \\\n"
    "       --carrier Generic \\\n"
    "       --image SWI9X15C_05.05.58.00.cwe \\\n"
    "       --image SWI9X15C_05.05.58.00_Generic_005.025_002.nvu\n"
    "\n";

/*****************************************************************************/
/* Runtime globals */

GMainLoop    *loop;
GCancellable *cancellable;
gint          exit_status = EXIT_FAILURE;

/*****************************************************************************/
/* Signal handlers */

static gboolean
signals_handler (gpointer psignum)
{
    /* Ignore consecutive requests of cancellation */
    if (!g_cancellable_is_cancelled (cancellable)) {
        g_printerr ("cancelling the operation...\n");
        g_cancellable_cancel (cancellable);
        /* Re-set the signal handler to allow main loop cancellation on
         * second signal */
        g_unix_signal_add (GPOINTER_TO_INT (psignum),  (GSourceFunc) signals_handler, psignum);
        return FALSE;
    }

    if (g_main_loop_is_running (loop)) {
        g_printerr ("cancelling the main loop...\n");
        g_main_loop_quit (loop);
    }

    return FALSE;
}

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
/* Version */

static void
print_version_and_exit (void)
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
    exit (EXIT_SUCCESS);
}

/*****************************************************************************/

static void
run_ready (QfuUpdater   *updater,
           GAsyncResult *res)
{
    GError *error = NULL;

    if (!qfu_updater_run_finish (updater, res, &error)) {
        g_printerr ("error: firmware update operation finished: %s\n", error->message);
        g_error_free (error);
    } else {
        g_print ("firmware update operation finished successfully\n");
        exit_status = EXIT_SUCCESS;
    }

    g_idle_add ((GSourceFunc) g_main_loop_quit, loop);
}

int main (int argc, char **argv)
{
    GError         *error = NULL;
    GOptionContext *context;
    QfuUpdater     *updater;
    GFile          *device_file;
    GList          *image_file_list = NULL;
    guint           i;

    setlocale (LC_ALL, "");

    g_type_init ();

    /* Setup option context, process it and destroy it */
    context = g_option_context_new ("- Update firmware in QMI devices");
    g_option_context_add_main_entries (context, context_main_entries, NULL);
    g_option_context_set_description (context, context_description);
    if (!g_option_context_parse (context, &argc, &argv, &error)) {
        g_printerr ("error: couldn't parse option context: %s\n", error->message);
        g_error_free (error);
        goto out;
    }

    if (version_flag)
        print_version_and_exit ();

    g_log_set_handler (NULL, G_LOG_LEVEL_MASK, log_handler, NULL);
    g_log_set_handler ("Qmi", G_LOG_LEVEL_MASK, log_handler, NULL);
    if (verbose_flag)
        qmi_utils_set_traces_enabled (TRUE);

    /* No device path given? */
    if (!device_str) {
        g_printerr ("error: no device path specified\n");
        goto out;
    }

    /* No firmware version given? */
    if (!firmware_version_str) {
        g_printerr ("error: no firmware version specified\n");
        goto out;
    }

    /* No config version given? */
    if (!config_version_str) {
        g_printerr ("error: no config version specified\n");
        goto out;
    }

    /* No carrier given? */
    if (!carrier_str) {
        g_printerr ("error: no carrier specified\n");
        goto out;
    }

    /* No images given? */
    if (!image_strv) {
        g_printerr ("error: no image path(s) specified\n");
        goto out;
    }

    /* Create runtime context */
    loop        = g_main_loop_new (NULL, FALSE);
    cancellable = g_cancellable_new ();
    exit_status = EXIT_SUCCESS;

    /* Setup signals */
    g_unix_signal_add (SIGINT,  (GSourceFunc)signals_handler, GINT_TO_POINTER (SIGINT));
    g_unix_signal_add (SIGHUP,  (GSourceFunc)signals_handler, GINT_TO_POINTER (SIGHUP));
    g_unix_signal_add (SIGTERM, (GSourceFunc)signals_handler, GINT_TO_POINTER (SIGTERM));

    /* Create list of image files */
    for (i = 0; image_strv[i]; i++)
        image_file_list = g_list_append (image_file_list, g_file_new_for_commandline_arg (image_strv[i]));

    /* Create updater */
    device_file = g_file_new_for_commandline_arg (device_str);
    updater = qfu_updater_new (device_file,
                               firmware_version_str,
                               config_version_str,
                               carrier_str,
                               image_file_list,
                               device_open_proxy_flag,
                               device_open_mbim_flag);
    g_object_unref (device_file);
    g_list_free_full (image_file_list, (GDestroyNotify) g_object_unref);

    /* Run! */
    qfu_updater_run (updater, cancellable, (GAsyncReadyCallback) run_ready, NULL);
    g_main_loop_run (loop);

out:
    /* Clean exit for a clean memleak report */
    if (context)
        g_option_context_free (context);
    if (updater)
        g_object_unref (updater);
    if (cancellable)
        g_object_unref (cancellable);
    if (loop)
        g_main_loop_unref (loop);

    return (exit_status);
}
