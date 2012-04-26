/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * qmicli -- Command line interface to control QMI devices
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
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
 * Copyright (C) 2012 Aleksander Morgado <aleksander@gnu.org>
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <string.h>

#include <glib.h>
#include <gio/gio.h>

#include <libqmi-glib.h>

#include "qmicli.h"

/* Context */
typedef struct {
    QmiDevice *device;
    QmiClientWds *client;
    GCancellable *cancellable;

    /* Helpers for the wds-start-network command */
    gulong network_started_id;
    guint32 packet_data_handle;
} Context;
static Context *ctx;

/* Options */
static gboolean start_network_flag;

static GOptionEntry entries[] = {
    { "wds-start-network", 0, 0, G_OPTION_ARG_NONE, &start_network_flag,
      "Start network",
      NULL
    },
    { NULL }
};

GOptionGroup *
qmicli_wds_get_option_group (void)
{
	GOptionGroup *group;

	group = g_option_group_new ("wds",
	                            "WDS options",
	                            "Show Wireless Data Service options",
	                            NULL,
	                            NULL);
	g_option_group_add_entries (group, entries);

	return group;
}

gboolean
qmicli_wds_options_enabled (void)
{
    static guint n_actions = 0;
    static gboolean checked = FALSE;

    if (checked)
        return !!n_actions;

    n_actions = (start_network_flag);

    if (n_actions > 1) {
        g_printerr ("error: too many WDS actions requested\n");
        exit (EXIT_FAILURE);
    }

    checked = TRUE;
    return !!n_actions;
}

static void
context_free (Context *ctx)
{
    if (!ctx)
        return;

    if (ctx->client)
        g_object_unref (ctx->client);
    if (ctx->network_started_id)
        g_cancellable_disconnect (ctx->cancellable, ctx->network_started_id);
    g_object_unref (ctx->cancellable);
    g_object_unref (ctx->device);
    g_slice_free (Context, ctx);
}

static void
release_client_ready (QmiDevice *device,
                      GAsyncResult *res)
{
    GError *error = NULL;

    if (!qmi_device_release_client_finish (device, res, &error)) {
        g_printerr ("error: couldn't release client: %s", error->message);
        exit (EXIT_FAILURE);
    }

    g_debug ("Client released");

    /* Cleanup context and finish async operation */
    context_free (ctx);
    qmicli_async_operation_done ();
}

static void
shutdown (void)
{
    qmi_device_release_client (ctx->device,
                               QMI_CLIENT (ctx->client),
                               10,
                               NULL,
                               (GAsyncReadyCallback)release_client_ready,
                               NULL);
}

static void
stop_network_ready (QmiClientWds *client,
                    GAsyncResult *res)
{
    GError *error = NULL;

    if (!qmi_client_wds_stop_network_finish (client, res, &error)) {
        g_printerr ("error: couldn't stop network: %s\n",
                    error->message);
        exit (EXIT_FAILURE);
    }

#undef VALIDATE_UNKNOWN
#define VALIDATE_UNKNOWN(str) (str ? str : "unknown")

    g_print ("[%s] Network stopped\n",
             qmi_device_get_path_display (ctx->device));

    shutdown ();
}

static void
network_cancelled (GCancellable *cancellable)
{
    ctx->network_started_id = 0;

    g_print ("Network cancelled... releasing resources\n");
    qmi_client_wds_stop_network (ctx->client,
                                 ctx->packet_data_handle,
                                 10,
                                 ctx->cancellable,
                                 (GAsyncReadyCallback)stop_network_ready,
                                 NULL);
}

static void
start_network_ready (QmiClientWds *client,
                     GAsyncResult *res)
{
    GError *error = NULL;

    ctx->packet_data_handle = qmi_client_wds_start_network_finish (client, res, &error);
    if (!ctx->packet_data_handle) {
        g_printerr ("error: couldn't start network: %s\n",
                    error->message);
        exit (EXIT_FAILURE);
    }

#undef VALIDATE_UNKNOWN
#define VALIDATE_UNKNOWN(str) (str ? str : "unknown")

    g_print ("[%s] Network started\n"
             "\tPacket data handle: %u\n",
             qmi_device_get_path_display (ctx->device),
             (guint)ctx->packet_data_handle);

    g_print ("\nCtrl+C will stop the network\n");
    ctx->network_started_id = g_cancellable_connect (ctx->cancellable,
                                                     G_CALLBACK (network_cancelled),
                                                     NULL,
                                                     NULL);
}

static void
allocate_client_ready (QmiDevice *device,
                       GAsyncResult *res)
{
    GError *error = NULL;

    ctx->client = (QmiClientWds *)qmi_device_allocate_client_finish (device, res, &error);
    if (!ctx->client) {
        g_printerr ("error: couldn't create WDS client: %s\n",
                    error->message);
        exit (EXIT_FAILURE);
    }

    /* Request to start network? */
    if (start_network_flag) {
        g_debug ("Asynchronously starting network...");
        qmi_client_wds_start_network (ctx->client,
                                      10,
                                      ctx->cancellable,
                                      (GAsyncReadyCallback)start_network_ready,
                                      NULL);
        return;
    }

    g_warn_if_reached ();
}

void
qmicli_wds_run (QmiDevice *device,
                GCancellable *cancellable)
{
    /* Initialize context */
    ctx = g_slice_new (Context);
    ctx->device = g_object_ref (device);
    ctx->cancellable = g_object_ref (cancellable);
    ctx->network_started_id = 0;

    /* Create a new WDS client */
    qmi_device_allocate_client (device,
                                QMI_SERVICE_WDS,
                                10,
                                cancellable,
                                (GAsyncReadyCallback)allocate_client_ready,
                                NULL);
}
