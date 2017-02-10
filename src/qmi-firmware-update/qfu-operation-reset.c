/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * qmi-firmware-reseter -- Command line tool to reseter firmware in QMI devices
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
#include "qfu-reseter.h"

typedef struct {
    GMainLoop    *loop;
    GCancellable *cancellable;
    gboolean      result;
} ReseterOperation;

static gboolean
signal_handler (ReseterOperation *operation)
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
run_ready (QfuReseter       *reseter,
           GAsyncResult     *res,
           ReseterOperation *operation)
{
    GError *error = NULL;

    if (!qfu_reseter_run_finish (reseter, res, &error)) {
        g_printerr ("error: reseter operation finished: %s\n", error->message);
        g_error_free (error);
    } else {
        g_print ("reseter operation finished successfully\n");
        operation->result = TRUE;
    }

    g_idle_add ((GSourceFunc) g_main_loop_quit, operation->loop);
}

static gboolean
operation_reseter_run (QfuReseter *reseter)
{
    ReseterOperation operation = {
        .loop        = NULL,
        .cancellable = NULL,
        .result      = FALSE,
    };

    g_assert (QFU_IS_RESETER (reseter));

    /* Create runtime context */
    operation.loop        = g_main_loop_new (NULL, FALSE);
    operation.cancellable = g_cancellable_new ();

    /* Setup signals */
    g_unix_signal_add (SIGINT,  (GSourceFunc) signal_handler, &operation);
    g_unix_signal_add (SIGHUP,  (GSourceFunc) signal_handler, &operation);
    g_unix_signal_add (SIGTERM, (GSourceFunc) signal_handler, &operation);

    /* Run! */
    qfu_reseter_run (reseter, operation.cancellable, (GAsyncReadyCallback) run_ready, &operation);
    g_main_loop_run (operation.loop);

    if (operation.cancellable)
        g_object_unref (operation.cancellable);
    if (operation.loop)
        g_main_loop_unref (operation.loop);

    return operation.result;
}

gboolean
qfu_operation_reset_run (QfuDeviceSelection *device_selection,
                         QmiDeviceOpenFlags  device_open_flags)
{
    QfuReseter *reseter = NULL;
    gboolean    result;

    reseter = qfu_reseter_new (device_selection, NULL, device_open_flags);
    result = operation_reseter_run (reseter);
    g_object_unref (reseter);
    return result;
}
