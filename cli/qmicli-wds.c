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
    guint packet_status_timeout_id;
    guint32 packet_data_handle;
} Context;
static Context *ctx;

/* Options */
static gboolean start_network_flag;
static gboolean get_packet_service_status_flag;
static gboolean get_data_bearer_technology_flag;
static gboolean get_current_data_bearer_technology_flag;

static GOptionEntry entries[] = {
    { "wds-start-network", 0, 0, G_OPTION_ARG_NONE, &start_network_flag,
      "Start network",
      NULL
    },
    { "wds-get-packet-service-status", 0, 0, G_OPTION_ARG_NONE, &get_packet_service_status_flag,
      "Get packet service status",
      NULL
    },
    { "wds-get-data-bearer-technology", 0, 0, G_OPTION_ARG_NONE, &get_data_bearer_technology_flag,
      "Get data bearer technology",
      NULL
    },
    { "wds-get-current-data-bearer-technology", 0, 0, G_OPTION_ARG_NONE, &get_current_data_bearer_technology_flag,
      "Get current data bearer technology",
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

    n_actions = (start_network_flag +
                 get_packet_service_status_flag +
                 get_data_bearer_technology_flag +
                 get_current_data_bearer_technology_flag);

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
    if (ctx->packet_status_timeout_id)
        g_source_remove (ctx->packet_status_timeout_id);
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
                               QMI_DEVICE_RELEASE_CLIENT_FLAGS_RELEASE_CID,
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
    QmiWdsStopNetworkOutput *output;

    output = qmi_client_wds_stop_network_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n",
                    error->message);
        exit (EXIT_FAILURE);
    }

    if (!qmi_wds_stop_network_output_get_result (output, &error)) {
        g_printerr ("error: couldn't stop network: %s\n", error->message);
        exit (EXIT_FAILURE);
    }

#undef VALIDATE_UNKNOWN
#define VALIDATE_UNKNOWN(str) (str ? str : "unknown")

    g_print ("[%s] Network stopped\n",
             qmi_device_get_path_display (ctx->device));

    qmi_wds_stop_network_output_unref (output);
    shutdown ();
}

static void
network_cancelled (GCancellable *cancellable)
{
    QmiWdsStopNetworkInput *input;

    ctx->network_started_id = 0;

    /* Remove the timeout right away */
    if (ctx->packet_status_timeout_id) {
        g_source_remove (ctx->packet_status_timeout_id);
        ctx->packet_status_timeout_id = 0;
    }

    input = qmi_wds_stop_network_input_new ();
    qmi_wds_stop_network_input_set_packet_data_handle (input, ctx->packet_data_handle);

    g_print ("Network cancelled... releasing resources\n");
    qmi_client_wds_stop_network (ctx->client,
                                 input,
                                 10,
                                 ctx->cancellable,
                                 (GAsyncReadyCallback)stop_network_ready,
                                 NULL);
    qmi_wds_stop_network_input_unref (input);
}

static void
timeout_get_packet_service_status_ready (QmiClientWds *client,
                                         GAsyncResult *res)
{
    GError *error = NULL;
    QmiWdsGetPacketServiceStatusOutput *output;

    output = qmi_client_wds_get_packet_service_status_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n",
                    error->message);
        return;
    }

    if (!qmi_wds_get_packet_service_status_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get packet service status: %s\n", error->message);
        g_error_free (error);
    } else {
        g_print ("[%s] Connection status: '%s'\n",
                 qmi_device_get_path_display (ctx->device),
                 qmi_wds_connection_status_get_string (
                     qmi_wds_get_packet_service_status_output_get_connection_status (
                         output)));
    }

    qmi_wds_get_packet_service_status_output_unref (output);
}

static gboolean
packet_status_timeout (void)
{
    qmi_client_wds_get_packet_service_status (ctx->client,
                                              NULL,
                                              10,
                                              ctx->cancellable,
                                              (GAsyncReadyCallback)timeout_get_packet_service_status_ready,
                                              NULL);
    return TRUE;
}

static void
start_network_ready (QmiClientWds *client,
                     GAsyncResult *res)
{
    GError *error = NULL;
    QmiWdsStartNetworkOutput *output;

    output = qmi_client_wds_start_network_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n",
                    error->message);
        exit (EXIT_FAILURE);
    }

    if (!qmi_wds_start_network_output_get_result (output, &error)) {
        g_printerr ("error: couldn't start network: %s\n", error->message);
        if (g_error_matches (error,
                             QMI_PROTOCOL_ERROR,
                             QMI_PROTOCOL_ERROR_CALL_FAILED)) {
            guint16 cer;
            guint16 domain;

            if (qmi_wds_start_network_output_get_call_end_reason (output, &cer))
                g_printerr ("call end reason: %u\n", cer);
            if (qmi_wds_start_network_output_get_verbose_call_end_reason (output, &cer, &domain))
                g_printerr ("verbose call end reason: %u, %u\n", domain, cer);
        }

        exit (EXIT_FAILURE);
    }

    qmi_wds_start_network_output_get_packet_data_handle (output, &ctx->packet_data_handle);

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

    ctx->packet_status_timeout_id = g_timeout_add_seconds (20,
                                                           (GSourceFunc)packet_status_timeout,
                                                           NULL);

    qmi_wds_start_network_output_unref (output);
}

static void
get_packet_service_status_ready (QmiClientWds *client,
                                 GAsyncResult *res)
{
    GError *error = NULL;
    QmiWdsGetPacketServiceStatusOutput *output;

    output = qmi_client_wds_get_packet_service_status_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n",
                    error->message);
        exit (EXIT_FAILURE);
    }

    if (!qmi_wds_get_packet_service_status_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get packet service status: %s\n", error->message);
        exit (EXIT_FAILURE);
    }

    g_print ("[%s] Connection status: '%s'\n",
             qmi_device_get_path_display (ctx->device),
             qmi_wds_connection_status_get_string (
                 qmi_wds_get_packet_service_status_output_get_connection_status (
                     output)));

    qmi_wds_get_packet_service_status_output_unref (output);
    shutdown ();
}

static void
get_data_bearer_technology_ready (QmiClientWds *client,
                                  GAsyncResult *res)
{
    GError *error = NULL;
    QmiWdsGetDataBearerTechnologyOutput *output;

    output = qmi_client_wds_get_data_bearer_technology_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n",
                    error->message);
        exit (EXIT_FAILURE);
    }

    if (!qmi_wds_get_data_bearer_technology_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get data bearer technology: %s\n", error->message);

        if (g_error_matches (error,
                             QMI_PROTOCOL_ERROR,
                             QMI_PROTOCOL_ERROR_OUT_OF_CALL)) {
            g_print ("[%s] Data bearer technology (last): '%s'\n",
                     qmi_device_get_path_display (ctx->device),
                     qmi_wds_data_bearer_technology_get_string (
                         qmi_wds_get_data_bearer_technology_output_get_last (
                             output)));
        }

        exit (EXIT_FAILURE);
    }

    g_print ("[%s] Data bearer technology (current): '%s'\n",
             qmi_device_get_path_display (ctx->device),
             qmi_wds_data_bearer_technology_get_string (
                 qmi_wds_get_data_bearer_technology_output_get_current (
                     output)));

    qmi_wds_get_data_bearer_technology_output_unref (output);
    shutdown ();
}

static void
get_current_data_bearer_technology_ready (QmiClientWds *client,
                                          GAsyncResult *res)
{
    GError *error = NULL;
    QmiWdsGetCurrentDataBearerTechnologyOutput *output;
    gchar *rat_string = NULL;
    gchar *so_string = NULL;

    output = qmi_client_wds_get_current_data_bearer_technology_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n",
                    error->message);
        exit (EXIT_FAILURE);
    }

    if (!qmi_wds_get_current_data_bearer_technology_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get current data bearer technology: %s\n", error->message);

        if (g_error_matches (error,
                             QMI_PROTOCOL_ERROR,
                             QMI_PROTOCOL_ERROR_OUT_OF_CALL)) {
            /* Retrieve LAST */

            switch (qmi_wds_get_current_data_bearer_technology_output_get_last_network_type (output)) {
            case QMI_WDS_NETWORK_TYPE_UNKNOWN:
                break;
            case QMI_WDS_NETWORK_TYPE_3GPP:
                rat_string = (qmi_wds_rat_3gpp_build_string_from_mask (
                                  qmi_wds_get_current_data_bearer_technology_output_get_last_rat_3gpp (
                                      output)));
                break;
            case QMI_WDS_NETWORK_TYPE_3GPP2:
                rat_string = (qmi_wds_rat_3gpp2_build_string_from_mask (
                                  qmi_wds_get_current_data_bearer_technology_output_get_last_rat_3gpp2 (
                                      output)));
                if (qmi_wds_get_current_data_bearer_technology_output_get_last_rat_3gpp2 (output) &
                    QMI_WDS_RAT_3GPP2_CDMA1X) {
                    so_string = (qmi_wds_so_cdma1x_build_string_from_mask (
                                     qmi_wds_get_current_data_bearer_technology_output_get_last_so_cdma1x (
                                         output)));
                } else if (qmi_wds_get_current_data_bearer_technology_output_get_last_rat_3gpp2 (output) &
                           QMI_WDS_RAT_3GPP2_EVDO_REVA) {
                    so_string = (qmi_wds_so_evdo_reva_build_string_from_mask (
                                     qmi_wds_get_current_data_bearer_technology_output_get_last_so_evdo_reva (
                                         output)));
                }
                break;
            default:
                g_warn_if_reached ();
            }

            g_print ("[%s] Data bearer technology (last):\n"
                     "   Network type: '%s'\n",
                     qmi_device_get_path_display (ctx->device),
                     qmi_wds_network_type_get_string (
                         qmi_wds_get_current_data_bearer_technology_output_get_last_network_type (
                             output)));
            if (rat_string) {
                g_print ("   Radio Access Technology: '%s'\n",
                         rat_string);
                g_free (rat_string);
            }
            if (so_string) {
                g_print ("   Service Option: '%s'\n",
                         so_string);
                g_free (so_string);
            }
        }

        exit (EXIT_FAILURE);
    }

    /* Retrieve CURRENT */

    switch (qmi_wds_get_current_data_bearer_technology_output_get_current_network_type (output)) {
    case QMI_WDS_NETWORK_TYPE_UNKNOWN:
        break;
    case QMI_WDS_NETWORK_TYPE_3GPP:
        rat_string = (qmi_wds_rat_3gpp_build_string_from_mask (
                          qmi_wds_get_current_data_bearer_technology_output_get_current_rat_3gpp (
                              output)));
        break;
    case QMI_WDS_NETWORK_TYPE_3GPP2:
        rat_string = (qmi_wds_rat_3gpp2_build_string_from_mask (
                          qmi_wds_get_current_data_bearer_technology_output_get_current_rat_3gpp2 (
                              output)));
        if (qmi_wds_get_current_data_bearer_technology_output_get_current_rat_3gpp2 (output) &
            QMI_WDS_RAT_3GPP2_CDMA1X) {
            so_string = (qmi_wds_so_cdma1x_build_string_from_mask (
                          qmi_wds_get_current_data_bearer_technology_output_get_current_so_cdma1x (
                              output)));
        } else if (qmi_wds_get_current_data_bearer_technology_output_get_current_rat_3gpp2 (output) &
                   QMI_WDS_RAT_3GPP2_EVDO_REVA) {
            so_string = (qmi_wds_so_evdo_reva_build_string_from_mask (
                          qmi_wds_get_current_data_bearer_technology_output_get_current_so_evdo_reva (
                              output)));
        }
        break;
    default:
        g_warn_if_reached ();
    }

    g_print ("[%s] Data bearer technology (current):\n"
             "   Network type: '%s'\n",
             qmi_device_get_path_display (ctx->device),
             qmi_wds_network_type_get_string (
                 qmi_wds_get_current_data_bearer_technology_output_get_current_network_type (
                     output)));
    if (rat_string) {
        g_print ("   Radio Access Technology: '%s'\n",
                 rat_string);
        g_free (rat_string);
    }
    if (so_string) {
        g_print ("   Service Option: '%s'\n",
                 so_string);
        g_free (so_string);
    }

    qmi_wds_get_current_data_bearer_technology_output_unref (output);
    shutdown ();
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
                                      NULL, /* allow NULL input for now */
                                      10,
                                      ctx->cancellable,
                                      (GAsyncReadyCallback)start_network_ready,
                                      NULL);
        return;
    }

    /* Request to get packet service status? */
    if (get_packet_service_status_flag) {
        g_debug ("Asynchronously getting packet service status...");
        qmi_client_wds_get_packet_service_status (ctx->client,
                                                  NULL,
                                                  10,
                                                  ctx->cancellable,
                                                  (GAsyncReadyCallback)get_packet_service_status_ready,
                                                  NULL);
        return;
    }

    /* Request to get data bearer technology? */
    if (get_data_bearer_technology_flag) {
        g_debug ("Asynchronously getting data bearer technology...");
        qmi_client_wds_get_data_bearer_technology (ctx->client,
                                                   NULL,
                                                   10,
                                                   ctx->cancellable,
                                                   (GAsyncReadyCallback)get_data_bearer_technology_ready,
                                                   NULL);
        return;
    }

    /* Request to get current data bearer technology? */
    if (get_current_data_bearer_technology_flag) {
        g_debug ("Asynchronously getting current data bearer technology...");
        qmi_client_wds_get_current_data_bearer_technology (ctx->client,
                                                           NULL,
                                                           10,
                                                           ctx->cancellable,
                                                           (GAsyncReadyCallback)get_current_data_bearer_technology_ready,
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
    ctx->packet_status_timeout_id = 0;

    /* Create a new WDS client */
    qmi_device_allocate_client (device,
                                QMI_SERVICE_WDS,
                                QMI_CID_NONE,
                                10,
                                cancellable,
                                (GAsyncReadyCallback)allocate_client_ready,
                                NULL);
}
