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
 * Copyright (C) 2012 Aleksander Morgado <aleksander@lanedo.com>
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <string.h>

#include <glib.h>
#include <gio/gio.h>

#include <libqmi-glib.h>

#define PROGRAM_NAME    "qmicli"
#define PROGRAM_VERSION PACKAGE_VERSION

/* Globals */
static GMainLoop *loop;
static GCancellable *cancellable;

/* Main options */
static gchar *device_str;
static gboolean verbose_flag;
static gboolean version_flag;

static GOptionEntry main_entries[] = {
    { "device", 'd', 0, G_OPTION_ARG_STRING, &device_str,
      "Specify device path",
      "[PATH]"
    },
    { "verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose_flag,
      "Run action with verbose logs",
      NULL
    },
    { "version", 'V', 0, G_OPTION_ARG_NONE, &version_flag,
      "Print version",
      NULL
    },
    { NULL }
};

static void
signals_handler (int signum)
{
    if (cancellable) {
        /* Ignore consecutive requests of cancellation */
        if (!g_cancellable_is_cancelled (cancellable)) {
            g_printerr ("%s\n",
                        "cancelling the operation...\n");
            g_cancellable_cancel (cancellable);
        }
        return;
    }

    if (loop &&
        g_main_loop_is_running (loop)) {
        g_printerr ("%s\n",
                    "cancelling the main loop...\n");
        g_main_loop_quit (loop);
    }
}

static void
log_handler (const gchar *log_domain,
             GLogLevelFlags log_level,
             const gchar *message,
             gpointer user_data)
{
    const gchar *log_level_str;
	time_t now;
	gchar time_str[64];
	struct tm    *local_time;

	now = time ((time_t *) NULL);
	local_time = localtime (&now);
	strftime (time_str, 64, "%d %b %Y, %H:%M:%S", local_time);

	switch (log_level) {
	case G_LOG_LEVEL_WARNING:
		log_level_str = "-Warning **";
		break;

	case G_LOG_LEVEL_CRITICAL:
    case G_LOG_FLAG_FATAL:
	case G_LOG_LEVEL_ERROR:
		log_level_str = "-Error **";
		break;

	case G_LOG_LEVEL_DEBUG:
		log_level_str = "[Debug]";
		break;

    default:
		log_level_str = "";
		break;
    }

    g_print ("[%s] %s %s\n", time_str, log_level_str, message);
}

static void
print_version_and_exit (void)
{
    g_print ("\n"
             PROGRAM_NAME " " PROGRAM_VERSION "\n"
             "Copyright (2012) Aleksander Morgado\n"
             "License GPLv2+: GNU GPL version 2 or later <http://gnu.org/licenses/gpl-2.0.html>\n"
             "This is free software: you are free to change and redistribute it.\n"
             "There is NO WARRANTY, to the extent permitted by law.\n"
             "\n");
    exit (EXIT_SUCCESS);
}

/*****************************************************************************/
/* Running asynchronously */

static void
async_operation_done (void)
{
    if (cancellable) {
        g_object_unref (cancellable);
        cancellable = NULL;
    }

    g_main_loop_quit (loop);
}

typedef struct {
    GCancellable *cancellable;
    QmiDevice *device;
} AsyncRunContext;

static void
async_run_context_complete_and_free (AsyncRunContext *ctx)
{
    if (ctx->cancellable)
        g_object_unref (ctx->cancellable);
    if (ctx->device)
        g_object_unref (ctx->device);
    g_slice_free (AsyncRunContext, ctx);

    async_operation_done ();
}

static void
device_open_ready (QmiDevice *device,
                   GAsyncResult *res,
                   AsyncRunContext *ctx)
{
    GError *error = NULL;

    if (!qmi_device_open_finish (device, res, &error)) {
        g_printerr ("error: couldn't open the QmiDevice: %s\n",
                    error->message);
        exit (EXIT_FAILURE);
    }

    if (!qmi_device_close (ctx->device, &error)) {
        g_printerr ("error: couldn't close the QmiDevice: %s\n",
                    error->message);
        exit (EXIT_FAILURE);
    }

    /* All done */
    async_run_context_complete_and_free (ctx);
}

static void
device_new_ready (GObject *unused,
                  GAsyncResult *res,
                  AsyncRunContext *ctx)
{
    GError *error = NULL;

    ctx->device = qmi_device_new_finish (res, &error);
    if (!ctx->device) {
        g_printerr ("error: couldn't create QmiDevice: %s\n",
                    error->message);
        exit (EXIT_FAILURE);
    }

    /* Open the device */
    qmi_device_open (ctx->device,
                     ctx->cancellable,
                     (GAsyncReadyCallback)device_open_ready,
                     ctx);
}

static void
run (GFile *file,
     GCancellable *cancellable)
{
    AsyncRunContext *ctx;

    ctx = g_slice_new0 (AsyncRunContext);
    ctx->cancellable = cancellable ? g_object_ref (cancellable) : NULL;

    qmi_device_new (file,
                    cancellable,
                    (GAsyncReadyCallback)device_new_ready,
                    ctx);
}

/*****************************************************************************/

int main (int argc, char **argv)
{
    GFile *file;
    GOptionContext *context;
	GOptionGroup *group;

    setlocale (LC_ALL, "");

    g_type_init ();

    /* Setup option context, process it and destroy it */
    context = g_option_context_new ("- Control QMI devices");
    g_option_context_add_main_entries (context, main_entries, NULL);
    g_option_context_parse (context, &argc, &argv, NULL);
	g_option_context_free (context);

    if (version_flag)
        print_version_and_exit ();

    if (verbose_flag)
        g_log_set_handler (G_LOG_DOMAIN, G_LOG_LEVEL_MASK, log_handler, NULL);

    /* No device path given? */
    if (!device_str) {
        g_printerr ("error: no device path specified\n");
        exit (EXIT_FAILURE);
    }

    /* Build new GFile from the commandline arg */
    file = g_file_new_for_commandline_arg (device_str);

    /* Setup signals */
    signal (SIGINT, signals_handler);
    signal (SIGHUP, signals_handler);
    signal (SIGTERM, signals_handler);

    /* Create requirements for async options */
    cancellable = g_cancellable_new ();
    loop = g_main_loop_new (NULL, FALSE);

    run (file, cancellable);
    g_main_loop_run (loop);

    if (cancellable)
        g_object_unref (cancellable);
    g_main_loop_unref (loop);
    g_object_unref (file);

    return EXIT_SUCCESS;
}
