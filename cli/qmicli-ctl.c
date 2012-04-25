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
    QmiClientCtl *client;
    GCancellable *cancellable;
} Context;
static Context *ctx;

/* Options */
static gboolean get_version_info_flag;

static GOptionEntry entries[] = {
    { "ctl-get-version-info", 0, 0, G_OPTION_ARG_NONE, &get_version_info_flag,
      "Get QMI version info",
      NULL
    },
    { NULL }
};

GOptionGroup *
qmicli_ctl_get_option_group (void)
{
	GOptionGroup *group;

	group = g_option_group_new ("ctl",
	                            "CTL options",
	                            "Show Control options",
	                            NULL,
	                            NULL);
	g_option_group_add_entries (group, entries);

	return group;
}

gboolean
qmicli_ctl_options_enabled (void)
{
    static guint n_actions = 0;
    static gboolean checked = FALSE;

    if (checked)
        return !!n_actions;

    n_actions = (get_version_info_flag);

    if (n_actions > 1) {
        g_printerr ("error: too many CTL actions requested\n");
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

    if (ctx->cancellable)
        g_object_unref (ctx->cancellable);
    if (ctx->device)
        g_object_unref (ctx->device);
    if (ctx->client)
        g_object_unref (ctx->client);
    g_slice_free (Context, ctx);
}

static void
client_release_ready (QmiClient *client,
                      GAsyncResult *res)
{
    GError *error = NULL;

    if (!qmi_client_release_finish (client, res, &error)) {
        g_printerr ("error: couldn't release client CID: %s", error->message);
        exit (EXIT_FAILURE);
    }

    g_debug ("Client CID released");

    /* Cleanup context and finish async operation */
    context_free (ctx);
    qmicli_async_operation_done ();
}

static void
shutdown (void)
{
    /* NOTE: we don't really need to run explicit release for the CTL client */
    qmi_client_release (QMI_CLIENT (ctx->client),
                        10,
                        (GAsyncReadyCallback)client_release_ready,
                        NULL);
}

static void
get_version_info_ready (QmiClientCtl *client,
                        GAsyncResult *res)
{
    GError *error = NULL;
    GPtrArray *result;
    guint i;

    result = qmi_client_ctl_get_version_info_finish (client, res, &error);
    if (!result) {
        g_printerr ("error: couldn't get version info: %s\n",
                    error->message);
        exit (EXIT_FAILURE);
    }

    g_print ("[%s] Supported services:\n",
             qmi_device_get_path_display (ctx->device));
    for (i = 0; i < result->len; i++) {
        QmiCtlVersionInfo *info;

        info = g_ptr_array_index (result, i);
        g_print ("\t%s (%u.%u)\n",
                 qmi_service_get_string (qmi_ctl_version_info_get_service (info)),
                 qmi_ctl_version_info_get_major_version (info),
                 qmi_ctl_version_info_get_minor_version (info));
    }

    g_ptr_array_unref (result);

    shutdown ();
}

void
qmicli_ctl_run (QmiDevice *device,
                GCancellable *cancellable)
{
    /* Initialize context */
    ctx = g_slice_new (Context);
    ctx->device = g_object_ref (device);
    if (cancellable)
        ctx->cancellable = g_object_ref (cancellable);

    /* The CTL client should be directly retrievable from the QmiDevice */
    ctx->client = qmi_device_get_client_ctl (device);

    /* Request to get version info? */
    if (get_version_info_flag) {
        g_debug ("Asynchronously getting version info...");
        qmi_client_ctl_get_version_info (ctx->client,
                                         10,
                                         ctx->cancellable,
                                         (GAsyncReadyCallback)get_version_info_ready,
                                         NULL);
        return;
    }

    g_warn_if_reached ();
}
