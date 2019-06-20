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
 * Copyright (C) 2016-2017 Aleksander Morgado <aleksander@aleksander.es>
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
} UpdateOperation;

static gboolean
signal_handler (UpdateOperation *operation)
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
           UpdateOperation *operation)
{
    GError *error = NULL;

    if (!qfu_updater_run_finish (updater, res, &error)) {
        g_printerr ("error: %s\n", error->message);
        if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_PERMISSION_DENIED))
            g_printerr ("note: you can ignore this error using --ignore-version-errors\n");
        g_error_free (error);
    } else {
        g_print ("firmware update operation finished successfully\n");
        operation->result = TRUE;
    }

    g_idle_add ((GSourceFunc) g_main_loop_quit, operation->loop);
}

static gboolean
operation_update_run (QfuUpdater   *updater,
                      const gchar **images)
{
    UpdateOperation operation = {
        .loop        = NULL,
        .cancellable = NULL,
        .result      = FALSE,
    };
    GList *image_file_list = NULL;
    guint  i;

    g_assert (images);
    g_assert (QFU_IS_UPDATER (updater));

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

    /* Run! */
    qfu_updater_run (updater, image_file_list, operation.cancellable, (GAsyncReadyCallback) run_ready, &operation);
    g_list_free_full (image_file_list, g_object_unref);
    g_main_loop_run (operation.loop);

    if (operation.cancellable)
        g_object_unref (operation.cancellable);
    if (operation.loop)
        g_main_loop_unref (operation.loop);

    return operation.result;
}

#if defined WITH_UDEV

gboolean
qfu_operation_update_run (const gchar        **images,
                          QfuDeviceSelection  *device_selection,
                          const gchar         *firmware_version,
                          const gchar         *config_version,
                          const gchar         *carrier,
                          QmiDeviceOpenFlags   device_open_flags,
                          gboolean             ignore_version_errors,
                          gboolean             override_download,
                          guint8               modem_storage_index,
                          gboolean             skip_validation)
{
    QfuUpdater *updater = NULL;
    gboolean    result;

    g_assert (images);

    updater = qfu_updater_new (device_selection,
                               firmware_version,
                               config_version,
                               carrier,
                               device_open_flags,
                               ignore_version_errors,
                               override_download,
                               modem_storage_index,
                               skip_validation);
    result = operation_update_run (updater, images);
    g_object_unref (updater);
    return result;
}

#endif

gboolean
qfu_operation_update_download_run (const gchar        **images,
                                   QfuDeviceSelection  *device_selection)
{
    QfuUpdater *updater = NULL;
    gboolean    result;

    g_assert (images);

    updater = qfu_updater_new_download (device_selection);
    result = operation_update_run (updater, images);
    g_object_unref (updater);
    return result;
}
