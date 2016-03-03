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
 * Copyright (C) 2014-2015 Aleksander Morgado <aleksander@aleksander.es>
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
#include "qmicli-helpers.h"

/* Context */
typedef struct {
    QmiDevice *device;
    QmiClientWda *client;
    GCancellable *cancellable;
} Context;
static Context *ctx;

/* Options */
static gchar *set_data_format_str;
static gboolean get_data_format_flag;
static gboolean get_supported_messages_flag;
static gboolean noop_flag;

static GOptionEntry entries[] = {
    { "wda-set-data-format", 0, 0, G_OPTION_ARG_STRING, &set_data_format_str,
      "Set data format",
      "[raw-ip|802-3]"
    },
    { "wda-get-data-format", 0, 0, G_OPTION_ARG_NONE, &get_data_format_flag,
      "Get data format",
      NULL
    },
    { "wda-get-supported-messages", 0, 0, G_OPTION_ARG_NONE, &get_supported_messages_flag,
      "Get supported messages",
      NULL
    },
    { "wda-noop", 0, 0, G_OPTION_ARG_NONE, &noop_flag,
      "Just allocate or release a WDA client. Use with `--client-no-release-cid' and/or `--client-cid'",
      NULL
    },
    { NULL }
};

GOptionGroup *
qmicli_wda_get_option_group (void)
{
    GOptionGroup *group;

    group = g_option_group_new ("wda",
                                "WDA options",
                                "Show Wireless Data Administrative options",
                                NULL,
                                NULL);
    g_option_group_add_entries (group, entries);

    return group;
}

gboolean
qmicli_wda_options_enabled (void)
{
    static guint n_actions = 0;
    static gboolean checked = FALSE;

    if (checked)
        return !!n_actions;

    n_actions = (!!set_data_format_str +
                 get_data_format_flag +
                 get_supported_messages_flag +
                 noop_flag);

    if (n_actions > 1) {
        g_printerr ("error: too many WDA actions requested\n");
        exit (EXIT_FAILURE);
    }

    checked = TRUE;
    return !!n_actions;
}

static void
context_free (Context *context)
{
    if (!context)
        return;

    if (context->client)
        g_object_unref (context->client);
    g_object_unref (context->cancellable);
    g_object_unref (context->device);
    g_slice_free (Context, context);
}

static void
operation_shutdown (gboolean operation_status)
{
    /* Cleanup context and finish async operation */
    context_free (ctx);
    qmicli_async_operation_done (operation_status);
}

static gboolean
noop_cb (gpointer unused)
{
    operation_shutdown (TRUE);
    return FALSE;
}

static void
get_data_format_ready (QmiClientWda *client,
                       GAsyncResult *res)
{
    QmiMessageWdaGetDataFormatOutput *output;
    GError *error = NULL;
    gboolean qos_format;
    QmiWdaLinkLayerProtocol link_layer_protocol;
    QmiWdaDataAggregationProtocol data_aggregation_protocol;
    guint32 ndp_signature;
    guint32 data_aggregation_max_size;

    output = qmi_client_wda_get_data_format_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_wda_get_data_format_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get data format: %s\n", error->message);
        g_error_free (error);
        qmi_message_wda_get_data_format_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] Successfully got data format\n",
             qmi_device_get_path_display (ctx->device));

    if (qmi_message_wda_get_data_format_output_get_qos_format (
            output,
            &qos_format,
            NULL))
        g_print ("                   QoS flow header: %s\n", qos_format ? "yes" : "no");

    if (qmi_message_wda_get_data_format_output_get_link_layer_protocol (
            output,
            &link_layer_protocol,
            NULL))
        g_print ("               Link layer protocol: '%s'\n",
                 qmi_wda_link_layer_protocol_get_string (link_layer_protocol));

    if (qmi_message_wda_get_data_format_output_get_uplink_data_aggregation_protocol (
            output,
            &data_aggregation_protocol,
            NULL))
        g_print ("  Uplink data aggregation protocol: '%s'\n",
                 qmi_wda_data_aggregation_protocol_get_string (data_aggregation_protocol));

    if (qmi_message_wda_get_data_format_output_get_downlink_data_aggregation_protocol (
            output,
            &data_aggregation_protocol,
            NULL))
        g_print ("Downlink data aggregation protocol: '%s'\n",
                 qmi_wda_data_aggregation_protocol_get_string (data_aggregation_protocol));

    if (qmi_message_wda_get_data_format_output_get_ndp_signature (
            output,
            &ndp_signature,
            NULL))
        g_print ("                     NDP signature: '%u'\n", ndp_signature);

    if (qmi_message_wda_get_data_format_output_get_uplink_data_aggregation_max_size (
            output,
            &data_aggregation_max_size,
            NULL))
        g_print ("  Uplink data aggregation max size: '%u'\n", data_aggregation_max_size);

    if (qmi_message_wda_get_data_format_output_get_downlink_data_aggregation_max_size (
            output,
            &data_aggregation_max_size,
            NULL))
        g_print ("Downlink data aggregation max size: '%u'\n", data_aggregation_max_size);

    qmi_message_wda_get_data_format_output_unref (output);
    operation_shutdown (TRUE);
}

static void
set_data_format_ready (QmiClientWda *client,
                       GAsyncResult *res)
{
    QmiMessageWdaSetDataFormatOutput *output;
    GError *error = NULL;
    gboolean qos_format;
    QmiWdaLinkLayerProtocol link_layer_protocol;
    QmiWdaDataAggregationProtocol data_aggregation_protocol;
    guint32 ndp_signature;
    guint32 data_aggregation_max_datagrams;
    guint32 data_aggregation_max_size;

    output = qmi_client_wda_set_data_format_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_wda_set_data_format_output_get_result (output, &error)) {
        g_printerr ("error: couldn't set data format: %s\n", error->message);
        g_error_free (error);
        qmi_message_wda_set_data_format_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] Successfully set data format\n",
             qmi_device_get_path_display (ctx->device));

    if (qmi_message_wda_set_data_format_output_get_qos_format (
            output,
            &qos_format,
            NULL))
        g_print ("                        QoS flow header: %s\n", qos_format ? "yes" : "no");

    if (qmi_message_wda_set_data_format_output_get_link_layer_protocol (
            output,
            &link_layer_protocol,
            NULL))
        g_print ("                    Link layer protocol: '%s'\n",
                 qmi_wda_link_layer_protocol_get_string (link_layer_protocol));

    if (qmi_message_wda_set_data_format_output_get_uplink_data_aggregation_protocol (
            output,
            &data_aggregation_protocol,
            NULL))
        g_print ("       Uplink data aggregation protocol: '%s'\n",
                 qmi_wda_data_aggregation_protocol_get_string (data_aggregation_protocol));

    if (qmi_message_wda_set_data_format_output_get_downlink_data_aggregation_protocol (
            output,
            &data_aggregation_protocol,
            NULL))
        g_print ("     Downlink data aggregation protocol: '%s'\n",
                 qmi_wda_data_aggregation_protocol_get_string (data_aggregation_protocol));

    if (qmi_message_wda_set_data_format_output_get_ndp_signature (
            output,
            &ndp_signature,
            NULL))
        g_print ("                          NDP signature: '%u'\n", ndp_signature);

    if (qmi_message_wda_set_data_format_output_get_downlink_data_aggregation_max_datagrams (
            output,
            &data_aggregation_max_datagrams,
            NULL))
        g_print ("Downlink data aggregation max datagrams: '%u'\n", data_aggregation_max_datagrams);

    if (qmi_message_wda_set_data_format_output_get_downlink_data_aggregation_max_size (
            output,
            &data_aggregation_max_size,
            NULL))
        g_print ("     Downlink data aggregation max size: '%u'\n", data_aggregation_max_size);

    qmi_message_wda_set_data_format_output_unref (output);
    operation_shutdown (TRUE);
}

static QmiMessageWdaSetDataFormatInput *
set_data_format_input_create (const gchar *str)
{
    QmiMessageWdaSetDataFormatInput *input = NULL;
    QmiWdaLinkLayerProtocol link_layer_protocol;

    if (qmicli_read_link_layer_protocol_from_string (str, &link_layer_protocol)) {
        GError *error = NULL;

        input = qmi_message_wda_set_data_format_input_new ();
        if (!qmi_message_wda_set_data_format_input_set_link_layer_protocol (
                input,
                link_layer_protocol,
                &error)) {
            g_printerr ("error: couldn't create input data bundle: '%s'\n",
                        error->message);
            g_error_free (error);
            qmi_message_wda_set_data_format_input_unref (input);
            input = NULL;
        }
    }

    return input;
}

static void
get_supported_messages_ready (QmiClientWda *client,
                              GAsyncResult *res)
{
    QmiMessageWdaGetSupportedMessagesOutput *output;
    GError *error = NULL;
    GArray *bytearray = NULL;
    gchar *str;

    output = qmi_client_wda_get_supported_messages_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_wda_get_supported_messages_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get supported WDA messages: %s\n", error->message);
        g_error_free (error);
        qmi_message_wda_get_supported_messages_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] Successfully got supported WDA messages:\n",
             qmi_device_get_path_display (ctx->device));

    qmi_message_wda_get_supported_messages_output_get_list (output, &bytearray, NULL);
    str = qmicli_get_supported_messages_list (bytearray ? (const guint8 *)bytearray->data : NULL,
                                              bytearray ? bytearray->len : 0);
    g_print ("%s", str);
    g_free (str);

    qmi_message_wda_get_supported_messages_output_unref (output);
    operation_shutdown (TRUE);
}

void
qmicli_wda_run (QmiDevice *device,
                QmiClientWda *client,
                GCancellable *cancellable)
{
    /* Initialize context */
    ctx = g_slice_new (Context);
    ctx->device = g_object_ref (device);
    ctx->client = g_object_ref (client);
    ctx->cancellable = g_object_ref (cancellable);

    /* Request to set data format? */
    if (set_data_format_str) {
        QmiMessageWdaSetDataFormatInput *input;

        input = set_data_format_input_create (set_data_format_str);
        if (!input) {
            operation_shutdown (FALSE);
            return;
        }

        g_debug ("Asynchronously setting data format...");
        qmi_client_wda_set_data_format (ctx->client,
                                        input,
                                        10,
                                        ctx->cancellable,
                                        (GAsyncReadyCallback)set_data_format_ready,
                                        NULL);
        qmi_message_wda_set_data_format_input_unref (input);
        return;
    }

    /* Request to read data format? */
    if (get_data_format_flag) {
        g_debug ("Asynchronously getting data format...");
        qmi_client_wda_get_data_format (ctx->client,
                                        NULL,
                                        10,
                                        ctx->cancellable,
                                        (GAsyncReadyCallback)get_data_format_ready,
                                        NULL);
        return;
    }

    /* Request to list supported messages? */
    if (get_supported_messages_flag) {
        g_debug ("Asynchronously getting supported WDA messages...");
        qmi_client_wda_get_supported_messages (ctx->client,
                                               NULL,
                                               10,
                                               ctx->cancellable,
                                               (GAsyncReadyCallback)get_supported_messages_ready,
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
