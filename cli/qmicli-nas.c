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
    QmiClientNas *client;
    GCancellable *cancellable;
} Context;
static Context *ctx;

/* Options */
static gboolean network_scan_flag;
static gboolean reset_flag;
static gboolean noop_flag;

static GOptionEntry entries[] = {
    { "nas-network-scan", 0, 0, G_OPTION_ARG_NONE, &network_scan_flag,
      "Scan networks",
      NULL
    },
    { "nas-reset", 0, 0, G_OPTION_ARG_NONE, &reset_flag,
      "Reset the service state",
      NULL
    },
    { "nas-noop", 0, 0, G_OPTION_ARG_NONE, &noop_flag,
      "Just allocate or release a NAS client. Use with `--client-no-release-cid' and/or `--client-cid'",
      NULL
    },
    { NULL }
};

GOptionGroup *
qmicli_nas_get_option_group (void)
{
	GOptionGroup *group;

	group = g_option_group_new ("nas",
	                            "NAS options",
	                            "Show Network Access Service options",
	                            NULL,
	                            NULL);
	g_option_group_add_entries (group, entries);

	return group;
}

gboolean
qmicli_nas_options_enabled (void)
{
    static guint n_actions = 0;
    static gboolean checked = FALSE;

    if (checked)
        return !!n_actions;

    n_actions = (network_scan_flag +
                 reset_flag +
                 noop_flag);

    if (n_actions > 1) {
        g_printerr ("error: too many NAS actions requested\n");
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
shutdown (gboolean operation_status)
{
    /* Cleanup context and finish async operation */
    context_free (ctx);
    qmicli_async_operation_done (operation_status);
}

static void
network_scan_ready (QmiClientNas *client,
                    GAsyncResult *res)
{
    QmiMessageNasNetworkScanOutput *output;
    GError *error = NULL;
    GArray *array;

    output = qmi_client_nas_network_scan_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        shutdown (FALSE);
        return;
    }

    if (!qmi_message_nas_network_scan_output_get_result (output, &error)) {
        g_printerr ("error: couldn't scan networks: %s\n", error->message);
        g_error_free (error);
        qmi_message_nas_network_scan_output_unref (output);
        shutdown (FALSE);
        return;
    }

    g_print ("[%s] Successfully scanned networks\n",
             qmi_device_get_path_display (ctx->device));

    array = NULL;
    if (qmi_message_nas_network_scan_output_get_network_information (output, &array, NULL)) {
        guint i;

        for (i = 0; i < array->len; i++) {
            QmiMessageNasNetworkScanOutputNetworkInformationElement *element;
            gchar *status_str;

            element = &g_array_index (array, QmiMessageNasNetworkScanOutputNetworkInformationElement, i);
            status_str = qmi_nas_network_status_build_string_from_mask (element->network_status);
            g_print ("Network [%u]:\n"
                     "\tMCC: '%" G_GUINT16_FORMAT"'\n"
                     "\tMNC: '%" G_GUINT16_FORMAT"'\n"
                     "\tStatus: '%s'\n"
                     "\tDescription: '%s'\n",
                     i,
                     element->mcc,
                     element->mnc,
                     status_str,
                     element->description);
            g_free (status_str);
        }
    }

    array = NULL;
    if (qmi_message_nas_network_scan_output_get_radio_access_technology (output, &array, NULL)) {
        guint i;

        for (i = 0; i < array->len; i++) {
            QmiMessageNasNetworkScanOutputRadioAccessTechnologyElement *element;

            element = &g_array_index (array, QmiMessageNasNetworkScanOutputRadioAccessTechnologyElement, i);
            g_print ("Network [%u]:\n"
                     "\tMCC: '%" G_GUINT16_FORMAT"'\n"
                     "\tMNC: '%" G_GUINT16_FORMAT"'\n"
                     "\tRAT: '%s'\n",
                     i,
                     element->mcc,
                     element->mnc,
                     qmi_nas_radio_interface_get_string (element->rat));
        }
    }

    array = NULL;
    if (qmi_message_nas_network_scan_output_get_mnc_pds_digit_include_status (output, &array, NULL)) {
        guint i;

        for (i = 0; i < array->len; i++) {
            QmiMessageNasNetworkScanOutputMncPdsDigitIncludeStatusElement *element;

            element = &g_array_index (array, QmiMessageNasNetworkScanOutputMncPdsDigitIncludeStatusElement, i);
            g_print ("Network [%u]:\n"
                     "\tMCC: '%" G_GUINT16_FORMAT"'\n"
                     "\tMNC: '%" G_GUINT16_FORMAT"'\n"
                     "\tMCC with PCS digit: '%s'\n",
                     i,
                     element->mcc,
                     element->mnc,
                     element->includes_pcs_digit ? "yes" : "no");
        }
    }

    qmi_message_nas_network_scan_output_unref (output);
    shutdown (TRUE);
}

static void
reset_ready (QmiClientNas *client,
             GAsyncResult *res)
{
    QmiMessageNasResetOutput *output;
    GError *error = NULL;

    output = qmi_client_nas_reset_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        shutdown (FALSE);
        return;
    }

    if (!qmi_message_nas_reset_output_get_result (output, &error)) {
        g_printerr ("error: couldn't reset the NAS service: %s\n", error->message);
        g_error_free (error);
        qmi_message_nas_reset_output_unref (output);
        shutdown (FALSE);
        return;
    }

    g_print ("[%s] Successfully performed NAS service reset\n",
             qmi_device_get_path_display (ctx->device));

    qmi_message_nas_reset_output_unref (output);
    shutdown (TRUE);
}

static gboolean
noop_cb (gpointer unused)
{
    shutdown (TRUE);
    return FALSE;
}

void
qmicli_nas_run (QmiDevice *device,
                QmiClientNas *client,
                GCancellable *cancellable)
{
    /* Initialize context */
    ctx = g_slice_new (Context);
    ctx->device = g_object_ref (device);
    ctx->client = g_object_ref (client);
    if (cancellable)
        ctx->cancellable = g_object_ref (cancellable);

    /* Request to scan networks? */
    if (network_scan_flag) {
        g_debug ("Asynchronously scanning networks...");
        qmi_client_nas_network_scan (ctx->client,
                                     NULL,
                                     300, /* this operation takes a lot of time! */
                                     ctx->cancellable,
                                     (GAsyncReadyCallback)network_scan_ready,
                                     NULL);
        return;
    }

    /* Request to reset NAS service? */
    if (reset_flag) {
        g_debug ("Asynchronously resetting NAS service...");
        qmi_client_nas_reset (ctx->client,
                              NULL,
                              10,
                              ctx->cancellable,
                              (GAsyncReadyCallback)reset_ready,
                              NULL);
        return;
    }

    /* Just client allocate/release? */
    if (noop_flag) {
        g_idle_add (noop_cb, NULL);
        return;
    }

    g_warn_if_reached ();
}
