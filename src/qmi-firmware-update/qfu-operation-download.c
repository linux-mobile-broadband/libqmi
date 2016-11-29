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
 * Copyright (C) 2016 Zodiac Inflight Innovations
 * Copyright (C) 2016 Aleksander Morgado <aleksander@aleksander.es>
 */

#include <glib-object.h>
#include <gio/gio.h>
#include <glib-unix.h>

#include "qfu-operation.h"
#include "qfu-updater.h"

typedef struct {
    GMainLoop    *loop;
    GCancellable *cancellable;
    gboolean      result;
} DownloadOperation;

static gboolean
signal_handler (DownloadOperation *operation)
{
    operation->result = FALSE;

    /* Ignore consecutive requests of cancellation */
    if (!g_cancellable_is_cancelled (operation->cancellable)) {
        g_printerr ("cancelling the operation...\n");
        g_cancellable_cancel (operation->cancellable);
        /* Reset the signal handler to allow main loop cancellation on
         * second signal */
        return G_SOURCE_CONTINUE;
    }

    if (g_main_loop_is_running (operation->loop)) {
        g_printerr ("cancelling the main loop...\n");
        g_main_loop_quit (operation->loop);
    }

    return G_SOURCE_REMOVE;
}

static void
run_ready (QfuUpdater        *updater,
           GAsyncResult      *res,
           DownloadOperation *operation)
{
    GError *error = NULL;

    if (!qfu_updater_run_finish (updater, res, &error)) {
        g_printerr ("error: firmware update operation finished: %s\n", error->message);
        g_error_free (error);
    } else {
        g_print ("firmware update operation finished successfully\n");
        operation->result = TRUE;
    }

    g_idle_add ((GSourceFunc) g_main_loop_quit, operation->loop);
}

gboolean
qfu_operation_download_run (const gchar  *device,
                            const gchar  *firmware_version,
                            const gchar  *config_version,
                            const gchar  *carrier,
                            const gchar **images,
                            gboolean      device_open_proxy,
                            gboolean      device_open_mbim)
{
    DownloadOperation operation = {
        .loop        = NULL,
        .cancellable = NULL,
        .result      = FALSE,
    };
    QfuUpdater *updater = NULL;
    GFile      *device_file = NULL;
    GList      *image_file_list = NULL;
    guint       i;

    g_assert (images);

    /* No device path given? */
    if (!device) {
        g_printerr ("error: no device path specified\n");
        goto out;
    }

    /* No firmware version given? */
    if (!firmware_version) {
        g_printerr ("error: no firmware version specified\n");
        goto out;
    }

    /* No config version given? */
    if (!config_version) {
        g_printerr ("error: no config version specified\n");
        goto out;
    }

    /* No carrier given? */
    if (!carrier) {
        g_printerr ("error: no carrier specified\n");
        goto out;
    }

    /* Create runtime context */
    operation.loop        = g_main_loop_new (NULL, FALSE);
    operation.cancellable = g_cancellable_new ();

    /* Setup signals */
    g_unix_signal_add (SIGINT,  (GSourceFunc) signal_handler, &operation);
    g_unix_signal_add (SIGHUP,  (GSourceFunc) signal_handler, &operation);
    g_unix_signal_add (SIGTERM, (GSourceFunc) signal_handler, &operation);

    /* Create list of image files */
    for (i = 0; images[i]; i++)
        image_file_list = g_list_append (image_file_list, g_file_new_for_commandline_arg (images[i]));

    /* Create updater */
    device_file = g_file_new_for_commandline_arg (device);
    updater = qfu_updater_new (device_file,
                               firmware_version,
                               config_version,
                               carrier,
                               image_file_list,
                               device_open_proxy,
                               device_open_mbim);
    g_object_unref (device_file);
    g_list_free_full (image_file_list, (GDestroyNotify) g_object_unref);

    /* Run! */
    qfu_updater_run (updater, operation.cancellable, (GAsyncReadyCallback) run_ready, &operation);
    g_main_loop_run (operation.loop);

out:
    if (updater)
        g_object_unref (updater);
    if (operation.cancellable)
        g_object_unref (operation.cancellable);
    if (operation.loop)
        g_main_loop_unref (operation.loop);

    return operation.result;
}
