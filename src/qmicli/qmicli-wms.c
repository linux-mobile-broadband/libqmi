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
 * Copyright (C) 2015-2017 Aleksander Morgado <aleksander@aleksander.es>
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

#if defined HAVE_QMI_SERVICE_WMS

#define VALIDATE_UNKNOWN(str) (str ? str : "unknown")

/* Context */
typedef struct {
    QmiDevice *device;
    QmiClientWms *client;
    GCancellable *cancellable;
#if defined HAVE_QMI_INDICATION_WMS_EVENT_REPORT
    guint event_report_indication_id;
#endif
} Context;
static Context *ctx;

/* Options */
static gboolean get_supported_messages_flag;
static gboolean get_routes_flag;
static gchar *set_routes_str;
static gboolean reset_flag;
static gboolean noop_flag;
static gchar *set_broadcast_config_str;
static gboolean get_broadcast_config_flag;
static gboolean set_broadcast_activation_flag;
static gboolean monitor_flag;

static GOptionEntry entries[] = {
#if defined HAVE_QMI_MESSAGE_WMS_GET_SUPPORTED_MESSAGES
    { "wms-get-supported-messages", 0, 0, G_OPTION_ARG_NONE, &get_supported_messages_flag,
      "Get supported messages",
      NULL
    },
#endif
#if defined HAVE_QMI_MESSAGE_WMS_GET_ROUTES
    { "wms-get-routes", 0, 0, G_OPTION_ARG_NONE, &get_routes_flag,
      "Get SMS route information",
      NULL
    },
#endif
#if defined HAVE_QMI_MESSAGE_WMS_SET_ROUTES
    { "wms-set-routes", 0, 0, G_OPTION_ARG_STRING, &set_routes_str,
      "Set SMS route information (keys: type, class, storage, receipt-action)",
      "[\"key=value,...\"]"
    },
#endif
#if defined HAVE_QMI_MESSAGE_WMS_SET_BROADCAST_CONFIG
    { "wms-set-cbs-channels", 0, 0, G_OPTION_ARG_STRING, &set_broadcast_config_str,
      "Set CBS channels (e.g. 4371-4372,4370,4373-4380",
      "[start-end,start-end]",
    },
#endif
#if defined HAVE_QMI_MESSAGE_WMS_GET_BROADCAST_CONFIG
    { "wms-get-cbs-channels", 0, 0, G_OPTION_ARG_NONE, &get_broadcast_config_flag,
      "Get CBS channels",
      NULL,
    },
#endif
#if defined HAVE_QMI_INDICATION_WMS_EVENT_REPORT
    { "wms-monitor", 0, 0, G_OPTION_ARG_NONE, &monitor_flag,
      "Monitor for incoming messages including Cell Broadcast (CBS/ETWS/CMAS). Use Ctrl+C to stop.",
      NULL
    },
#endif
#if defined HAVE_QMI_MESSAGE_WMS_SET_BROADCAST_ACTIVATION
    { "wms-set-broadcast-activation", 0, 0, G_OPTION_ARG_NONE, &set_broadcast_activation_flag,
      "Activate Cell Broadcast message reception on the modem",
      NULL
    },
#endif
#if defined HAVE_QMI_MESSAGE_WMS_RESET
    { "wms-reset", 0, 0, G_OPTION_ARG_NONE, &reset_flag,
      "Reset the service state",
      NULL
    },
#endif
    { "wms-noop", 0, 0, G_OPTION_ARG_NONE, &noop_flag,
      "Just allocate or release a WMS client. Use with `--client-no-release-cid' and/or `--client-cid'",
      NULL
    },
    { NULL, 0, 0, 0, NULL, NULL, NULL }
};

GOptionGroup *
qmicli_wms_get_option_group (void)
{
    GOptionGroup *group;

    group = g_option_group_new ("wms",
                                "WMS options:",
                                "Show Wireless Messaging Service options",
                                NULL,
                                NULL);
    g_option_group_add_entries (group, entries);

    return group;
}

gboolean
qmicli_wms_options_enabled (void)
{
    static guint n_actions = 0;
    static gboolean checked = FALSE;

    if (checked)
        return !!n_actions;

    n_actions = (get_supported_messages_flag +
                 get_routes_flag +
                 !!set_routes_str +
                 !!set_broadcast_config_str +
                 get_broadcast_config_flag +
                 set_broadcast_activation_flag +
                 monitor_flag +
                 reset_flag +
                 noop_flag);

    if (n_actions > 1) {
        g_printerr ("error: too many WMS actions requested\n");
        exit (EXIT_FAILURE);
    }

    if (monitor_flag)
        qmicli_expect_indications ();

    checked = TRUE;
    return !!n_actions;
}

static void
context_free (Context *context)
{
    if (!context)
        return;

#if defined HAVE_QMI_INDICATION_WMS_EVENT_REPORT
    if (context->event_report_indication_id)
        g_signal_handler_disconnect (context->client,
                                     context->event_report_indication_id);
#endif

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
    qmicli_async_operation_done (operation_status, FALSE);
}

#if defined HAVE_QMI_MESSAGE_WMS_GET_SUPPORTED_MESSAGES

static void
get_supported_messages_ready (QmiClientWms *client,
                              GAsyncResult *res)
{
    QmiMessageWmsGetSupportedMessagesOutput *output;
    GError *error = NULL;
    GArray *bytearray = NULL;
    gchar *str;

    output = qmi_client_wms_get_supported_messages_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_wms_get_supported_messages_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get supported WMS messages: %s\n", error->message);
        g_error_free (error);
        qmi_message_wms_get_supported_messages_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] Successfully got supported WMS messages:\n",
             qmi_device_get_path_display (ctx->device));

    qmi_message_wms_get_supported_messages_output_get_list (output, &bytearray, NULL);
    str = qmicli_get_supported_messages_list (bytearray ? (const guint8 *)bytearray->data : NULL,
                                              bytearray ? bytearray->len : 0);
    g_print ("%s", str);
    g_free (str);

    qmi_message_wms_get_supported_messages_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_WMS_GET_SUPPORTED_MESSAGES */

#if defined HAVE_QMI_MESSAGE_WMS_GET_ROUTES

static void
get_routes_ready (QmiClientWms *client,
                  GAsyncResult *res)
{
    g_autoptr(QmiMessageWmsGetRoutesOutput) output = NULL;
    GError *error = NULL;
    GArray *route_list;
    guint i;

    output = qmi_client_wms_get_routes_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_wms_get_routes_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get SMS routes: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_wms_get_routes_output_get_route_list (output, &route_list, &error)) {
        g_printerr ("error: got invalid SMS routes: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] Got %u SMS routes:\n", qmi_device_get_path_display (ctx->device),
                                          route_list->len);

    for (i = 0; i < route_list->len; i++) {
        QmiMessageWmsGetRoutesOutputRouteListElement *route;

        route = &g_array_index (route_list, QmiMessageWmsGetRoutesOutputRouteListElement, i);
        g_print ("  Route #%u:\n", i + 1);
        g_print ("      Message Type: %s\n", VALIDATE_UNKNOWN (qmi_wms_message_type_get_string (route->message_type)));
        g_print ("     Message Class: %s\n", VALIDATE_UNKNOWN (qmi_wms_message_class_get_string (route->message_class)));
        g_print ("      Storage Type: %s\n", VALIDATE_UNKNOWN (qmi_wms_storage_type_get_string (route->storage)));
        g_print ("    Receipt Action: %s\n", VALIDATE_UNKNOWN (qmi_wms_receipt_action_get_string (route->receipt_action)));
    }

    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_WMS_GET_ROUTES */

#if defined HAVE_QMI_MESSAGE_WMS_SET_ROUTES

typedef struct {
    GArray *route_list;

    gboolean message_type_set;
    gboolean message_class_set;
    gboolean storage_set;
    gboolean receipt_action_set;
} SetRoutesContext;

static void
set_routes_context_init (SetRoutesContext *routes_ctx)
{
    memset (routes_ctx, 0, sizeof(SetRoutesContext));
    routes_ctx->route_list = g_array_new (FALSE, TRUE, sizeof (QmiMessageWmsSetRoutesInputRouteListElement));
}

static void
set_routes_context_destroy (SetRoutesContext *routes_ctx)
{
    g_array_unref (routes_ctx->route_list);
}

static gboolean
set_route_properties_handle (const gchar  *key,
                             const gchar  *value,
                             GError      **error,
                             gpointer      user_data)
{
    SetRoutesContext *routes_ctx = user_data;
    QmiMessageWmsSetRoutesInputRouteListElement *cur_route;
    gboolean ret = FALSE;

    if (!value || !value[0]) {
        g_set_error (error,
                     QMI_CORE_ERROR,
                     QMI_CORE_ERROR_FAILED,
                     "key '%s' required a value",
                     key);
        return FALSE;
    }

    if (!routes_ctx->message_type_set && !routes_ctx->message_class_set &&
        !routes_ctx->storage_set && !routes_ctx->receipt_action_set) {
        QmiMessageWmsSetRoutesInputRouteListElement new_elt;

        memset (&new_elt, 0, sizeof (QmiMessageWmsSetRoutesInputRouteListElement));
        g_array_append_val (routes_ctx->route_list, new_elt);
    }
    cur_route = &g_array_index (routes_ctx->route_list,
                                QmiMessageWmsSetRoutesInputRouteListElement,
                                routes_ctx->route_list->len - 1);

    if (g_ascii_strcasecmp (key, "type") == 0 && !routes_ctx->message_type_set) {
        if (!qmicli_read_wms_message_type_from_string (value, &cur_route->message_type)) {
            g_set_error (error,
                         QMI_CORE_ERROR,
                         QMI_CORE_ERROR_FAILED,
                         "unknown message type '%s'",
                         value);
            return FALSE;
        }
        routes_ctx->message_type_set = TRUE;
        ret = TRUE;
    } else if (g_ascii_strcasecmp (key, "class") == 0 && !routes_ctx->message_class_set) {
        if (!qmicli_read_wms_message_class_from_string (value, &cur_route->message_class)) {
            g_set_error (error,
                         QMI_CORE_ERROR,
                         QMI_CORE_ERROR_FAILED,
                         "unknown message class '%s'",
                         value);
            return FALSE;
        }
        routes_ctx->message_class_set = TRUE;
        ret = TRUE;
    } else if (g_ascii_strcasecmp (key, "storage") == 0 && !routes_ctx->storage_set) {
        if (!qmicli_read_wms_storage_type_from_string (value, &cur_route->storage)) {
            g_set_error (error,
                         QMI_CORE_ERROR,
                         QMI_CORE_ERROR_FAILED,
                         "unknown storage type '%s'",
                         value);
            return FALSE;
        }
        routes_ctx->storage_set = TRUE;
        ret = TRUE;
    } else if (g_ascii_strcasecmp (key, "receipt-action") == 0 && !routes_ctx->receipt_action_set) {
        if (!qmicli_read_wms_receipt_action_from_string (value, &cur_route->receipt_action)) {
            g_set_error (error,
                         QMI_CORE_ERROR,
                         QMI_CORE_ERROR_FAILED,
                         "unknown receipt action '%s'",
                         value);
            return FALSE;
        }
        routes_ctx->receipt_action_set = TRUE;
        ret = TRUE;
    }

    if (routes_ctx->message_type_set && routes_ctx->message_class_set &&
        routes_ctx->storage_set && routes_ctx->receipt_action_set) {
        /* We have a complete set of details for this route. Reset the context state. */
        routes_ctx->message_type_set = FALSE;
        routes_ctx->message_class_set = FALSE;
        routes_ctx->storage_set = FALSE;
        routes_ctx->receipt_action_set = FALSE;
    }

    if (!ret) {
        g_set_error (error,
                     QMI_CORE_ERROR,
                     QMI_CORE_ERROR_FAILED,
                     "unrecognized or duplicate option '%s'",
                     key);
    }
    return ret;
}

static QmiMessageWmsSetRoutesInput *
set_routes_input_create (const gchar  *str,
                         GError      **error)
{
    g_autoptr(QmiMessageWmsSetRoutesInput) input = NULL;
    SetRoutesContext routes_ctx;
    GError *inner_error = NULL;

    set_routes_context_init (&routes_ctx);

    if (!qmicli_parse_key_value_string (str,
                                        &inner_error,
                                        set_route_properties_handle,
                                        &routes_ctx)) {
        g_propagate_prefixed_error (error,
                                    inner_error,
                                    "couldn't parse input string: ");
        set_routes_context_destroy (&routes_ctx);
        return NULL;
    }

    if (routes_ctx.route_list->len == 0) {
        g_set_error_literal (error,
                             QMI_CORE_ERROR,
                             QMI_CORE_ERROR_FAILED,
                             "route list was empty");
        set_routes_context_destroy (&routes_ctx);
        return NULL;
    }

    if (routes_ctx.message_type_set || routes_ctx.message_class_set ||
        routes_ctx.storage_set || routes_ctx.receipt_action_set) {
        g_set_error_literal (error,
                             QMI_CORE_ERROR,
                             QMI_CORE_ERROR_FAILED,
                             "final route was missing one or more options");
        set_routes_context_destroy (&routes_ctx);
        return NULL;
    }

    /* Create input */
    input = qmi_message_wms_set_routes_input_new ();

    if (!qmi_message_wms_set_routes_input_set_route_list (input, routes_ctx.route_list, &inner_error)) {
        g_propagate_error (error, inner_error);
        set_routes_context_destroy (&routes_ctx);
        return NULL;
    }

    set_routes_context_destroy (&routes_ctx);
    return g_steal_pointer (&input);
}

static void
set_routes_ready (QmiClientWms *client,
                  GAsyncResult *res)
{
    g_autoptr(QmiMessageWmsSetRoutesOutput) output = NULL;
    GError *error = NULL;

    output = qmi_client_wms_set_routes_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_wms_set_routes_output_get_result (output, &error)) {
        g_printerr ("error: couldn't set SMS routes: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] Successfully set SMS routes\n",
             qmi_device_get_path_display (ctx->device));

    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_WMS_SET_ROUTES */

#if defined HAVE_QMI_MESSAGE_WMS_SET_BROADCAST_CONFIG

static QmiMessageWmsSetBroadcastConfigInput *
set_broadcast_config_input_create (const gchar  *str,
                                   GError      **error)
{
    g_autoptr(QmiMessageWmsSetBroadcastConfigInput) input = NULL;
    g_autoptr (GArray) channels_list = NULL;
    GError *inner_error = NULL;

    if (!qmicli_read_cbs_channels_from_string (str, &channels_list)) {
        return NULL;
    }

    if (channels_list->len == 0) {
        g_set_error_literal (error,
                             QMI_CORE_ERROR,
                             QMI_CORE_ERROR_FAILED,
                             "cbs channels list was empty");
        return NULL;
    }

    /* Create input */
    input = qmi_message_wms_set_broadcast_config_input_new ();

    if (!qmi_message_wms_set_broadcast_config_input_set_message_mode (input,
                                                                      QMI_WMS_MESSAGE_MODE_GSM_WCDMA,
                                                                      &inner_error)) {
        g_propagate_error (error, inner_error);
        return NULL;
    }

    if (!qmi_message_wms_set_broadcast_config_input_set_channels (input, channels_list, &inner_error)) {
        g_propagate_error (error, inner_error);
        return NULL;
    }

    return g_steal_pointer (&input);
}

static void
set_broadcast_config_ready (QmiClientWms *client,
                            GAsyncResult *res)
{
    g_autoptr(QmiMessageWmsSetBroadcastConfigOutput) output = NULL;
    g_autoptr(GError) error = NULL;

    output = qmi_client_wms_set_broadcast_config_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_wms_set_broadcast_config_output_get_result (output, &error)) {
        g_printerr ("error: couldn't set CBS channels: %s\n", error->message);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] Successfully set cbs channels\n",
             qmi_device_get_path_display (ctx->device));

    operation_shutdown (TRUE);
}

#endif

#if defined HAVE_QMI_MESSAGE_WMS_GET_BROADCAST_CONFIG

static void
get_broadcast_config_ready (QmiClientWms *client,
                            GAsyncResult *res)
{
    g_autoptr(QmiMessageWmsGetBroadcastConfigOutput) output = NULL;
    g_autoptr(GError) error = NULL;
    GArray *channels = NULL;
    gboolean active;
    guint i;

    output = qmi_client_wms_get_broadcast_config_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_wms_get_broadcast_config_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get CBS channels: %s\n", error->message);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_wms_get_broadcast_config_output_get_config (output, &active, &channels, &error)) {
        g_printerr ("error: couldn't get CBS channels: %s\n", error->message);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] CBS broadcast active: %s\n",
             qmi_device_get_path_display (ctx->device),
             active ? "yes" : "no");
    g_print ("[%s] CBS channels: ", qmi_device_get_path_display (ctx->device));

    for (i = 0; i < channels->len; i++) {
        QmiMessageWmsGetBroadcastConfigOutputConfigChannelsElement ch;

        ch = g_array_index (channels, QmiMessageWmsGetBroadcastConfigOutputConfigChannelsElement, i);
        if (!ch.selected)
            continue;

        if (i > 0)
            g_print (",");

        if (ch.start == ch.end)
            g_print ("%d", ch.start);
        else
            g_print ("%d-%d", ch.start, ch.end);
    }
    g_print ("\n");

    operation_shutdown (TRUE);
}

#endif

#if defined HAVE_QMI_MESSAGE_WMS_SET_BROADCAST_ACTIVATION

static void
set_broadcast_activation_ready (QmiClientWms *client,
                                GAsyncResult *res)
{
    g_autoptr(QmiMessageWmsSetBroadcastActivationOutput) output = NULL;
    g_autoptr(GError) error = NULL;

    output = qmi_client_wms_set_broadcast_activation_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_wms_set_broadcast_activation_output_get_result (output, &error)) {
        g_printerr ("error: couldn't activate broadcast: %s\n", error->message);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] Successfully activated Cell Broadcast reception\n",
             qmi_device_get_path_display (ctx->device));

    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_WMS_SET_BROADCAST_ACTIVATION */

#if defined HAVE_QMI_INDICATION_WMS_EVENT_REPORT

static void
monitoring_cancelled (GCancellable *cancellable)
{
    operation_shutdown (TRUE);
}

static void
event_report_received (QmiClientWms *client,
                       QmiIndicationWmsEventReportOutput *output)
{
    g_print ("[%s] Received WMS event report indication:\n",
             qmi_device_get_path_display (ctx->device));

    /* Transfer Route MT Message (TLV 0x11) - carries CBS/SMS PDU data */
    {
        QmiWmsAckIndicator ack_indicator;
        guint32 transaction_id;
        QmiWmsMessageFormat format;
        GArray *raw_data = NULL;

        if (qmi_indication_wms_event_report_output_get_transfer_route_mt_message (
                output, &ack_indicator, &transaction_id, &format, &raw_data, NULL)) {
            guint i;

            g_print ("  Transfer Route MT Message:\n");
            g_print ("    Ack Indicator:  %s\n",
                     VALIDATE_UNKNOWN (qmi_wms_ack_indicator_get_string (ack_indicator)));
            g_print ("    Transaction ID: %u\n", transaction_id);
            g_print ("    Format:         %s\n",
                     VALIDATE_UNKNOWN (qmi_wms_message_format_get_string (format)));
            g_print ("    Raw Data (%u bytes):", raw_data->len);
            for (i = 0; i < raw_data->len; i++) {
                if (i % 16 == 0)
                    g_print ("\n      ");
                g_print ("%02x ", g_array_index (raw_data, guint8, i));
            }
            g_print ("\n");

            /* For GSM/WCDMA Cell Broadcast, decode the CBS page header */
            if (format == QMI_WMS_MESSAGE_FORMAT_GSM_WCDMA_BROADCAST &&
                raw_data->len >= 6) {
                guint16 serial_number;
                guint16 message_id;
                guint8  dcs;
                guint8  page_info;

                serial_number = ((guint16)g_array_index (raw_data, guint8, 0) << 8) |
                                 (guint16)g_array_index (raw_data, guint8, 1);
                message_id    = ((guint16)g_array_index (raw_data, guint8, 2) << 8) |
                                 (guint16)g_array_index (raw_data, guint8, 3);
                dcs           = g_array_index (raw_data, guint8, 4);
                page_info     = g_array_index (raw_data, guint8, 5);

                g_print ("    CBS Header:\n");
                g_print ("      Serial Number: 0x%04x (GS: %u, Message Code: %u, Update: %u)\n",
                         serial_number,
                         (serial_number >> 14) & 0x03,
                         (serial_number >> 4) & 0x03ff,
                         serial_number & 0x0f);
                g_print ("      Message ID:    %u (0x%04x)\n", message_id, message_id);
                g_print ("      DCS:           0x%02x\n", dcs);
                g_print ("      Page:          %u of %u\n",
                         (page_info >> 4) & 0x0f,
                         page_info & 0x0f);
            }
        }
    }

    /* ETWS Message (TLV 0x13) */
    {
        QmiWmsNotificationType notification_type;
        GArray *raw_data = NULL;

        if (qmi_indication_wms_event_report_output_get_etws_message (
                output, &notification_type, &raw_data, NULL)) {
            guint i;

            g_print ("  ETWS Message:\n");
            g_print ("    Notification Type: %s\n",
                     VALIDATE_UNKNOWN (qmi_wms_notification_type_get_string (notification_type)));
            g_print ("    Raw Data (%u bytes):", raw_data->len);
            for (i = 0; i < raw_data->len; i++) {
                if (i % 16 == 0)
                    g_print ("\n      ");
                g_print ("%02x ", g_array_index (raw_data, guint8, i));
            }
            g_print ("\n");
        }
    }

    /* ETWS PLMN Information (TLV 0x14) */
    {
        guint16 mcc;
        guint16 mnc;

        if (qmi_indication_wms_event_report_output_get_etws_plmn_information (
                output, &mcc, &mnc, NULL)) {
            g_print ("  ETWS PLMN:\n");
            g_print ("    MCC: %u\n", mcc);
            g_print ("    MNC: %u\n", mnc);
        }
    }

    /* MT Message (TLV 0x10) - stored message notification */
    {
        QmiWmsStorageType storage_type;
        guint32 memory_index;

        if (qmi_indication_wms_event_report_output_get_mt_message (
                output, &storage_type, &memory_index, NULL)) {
            g_print ("  MT Message (stored):\n");
            g_print ("    Storage Type: %s\n",
                     VALIDATE_UNKNOWN (qmi_wms_storage_type_get_string (storage_type)));
            g_print ("    Memory Index: %u\n", memory_index);
        }
    }

    /* Message Mode (TLV 0x12) */
    {
        QmiWmsMessageMode message_mode;

        if (qmi_indication_wms_event_report_output_get_message_mode (
                output, &message_mode, NULL)) {
            g_print ("  Message Mode: %s\n",
                     VALIDATE_UNKNOWN (qmi_wms_message_mode_get_string (message_mode)));
        }
    }

    g_print ("\n");
}

static void
set_event_report_for_monitor_ready (QmiClientWms *client,
                                    GAsyncResult *res)
{
    g_autoptr(QmiMessageWmsSetEventReportOutput) output = NULL;
    g_autoptr(GError) error = NULL;

    output = qmi_client_wms_set_event_report_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_wms_set_event_report_output_get_result (output, &error)) {
        g_printerr ("error: couldn't enable WMS event reporting: %s\n", error->message);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] WMS event reporting enabled, monitoring for messages...\n",
             qmi_device_get_path_display (ctx->device));

    /* Connect the indication signal */
    ctx->event_report_indication_id =
        g_signal_connect (ctx->client,
                          "event-report",
                          G_CALLBACK (event_report_received),
                          NULL);

    /* User can use Ctrl+C to cancel the monitoring at any time */
    g_cancellable_connect (ctx->cancellable,
                           G_CALLBACK (monitoring_cancelled),
                           NULL,
                           NULL);
}

static void
start_wms_monitoring (void)
{
    g_autoptr(QmiMessageWmsSetEventReportInput) input = NULL;

    input = qmi_message_wms_set_event_report_input_new ();
    qmi_message_wms_set_event_report_input_set_new_mt_message_indicator (input, TRUE, NULL);

    qmi_client_wms_set_event_report (ctx->client,
                                     input,
                                     10,
                                     ctx->cancellable,
                                     (GAsyncReadyCallback)set_event_report_for_monitor_ready,
                                     NULL);
}

#endif /* HAVE_QMI_INDICATION_WMS_EVENT_REPORT */

#if defined HAVE_QMI_MESSAGE_WMS_RESET

static void
reset_ready (QmiClientWms *client,
             GAsyncResult *res)
{
    QmiMessageWmsResetOutput *output;
    GError *error = NULL;

    output = qmi_client_wms_reset_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_wms_reset_output_get_result (output, &error)) {
        g_printerr ("error: couldn't reset the WMS service: %s\n", error->message);
        g_error_free (error);
        qmi_message_wms_reset_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] Successfully performed WMS service reset\n",
             qmi_device_get_path_display (ctx->device));

    qmi_message_wms_reset_output_unref (output);
    operation_shutdown (TRUE);
}

#endif

static gboolean
noop_cb (gpointer unused)
{
    operation_shutdown (TRUE);
    return FALSE;
}

void
qmicli_wms_run (QmiDevice *device,
                QmiClientWms *client,
                GCancellable *cancellable)
{
    /* Initialize context */
    ctx = g_slice_new0 (Context);
    ctx->device = g_object_ref (device);
    ctx->client = g_object_ref (client);
    ctx->cancellable = g_object_ref (cancellable);

#if defined HAVE_QMI_MESSAGE_WMS_GET_SUPPORTED_MESSAGES
    if (get_supported_messages_flag) {
        g_debug ("Asynchronously getting supported WMS messages...");
        qmi_client_wms_get_supported_messages (ctx->client,
                                               NULL,
                                               10,
                                               ctx->cancellable,
                                               (GAsyncReadyCallback)get_supported_messages_ready,
                                               NULL);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_WMS_GET_ROUTES
    if (get_routes_flag) {
        g_debug ("Asynchronously getting SMS routes...");
        qmi_client_wms_get_routes (ctx->client,
                                   NULL,
                                   10,
                                   ctx->cancellable,
                                   (GAsyncReadyCallback)get_routes_ready,
                                   NULL);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_WMS_SET_ROUTES
    if (set_routes_str) {
        g_autoptr(QmiMessageWmsSetRoutesInput) input = NULL;
        GError *error = NULL;

        input = set_routes_input_create (set_routes_str, &error);
        if (!input) {
            g_printerr ("Failed to set route: %s\n", error->message);
            g_error_free (error);
            operation_shutdown (FALSE);
            return;
        }
        g_debug ("Asynchronously setting SMS routes...");
        qmi_client_wms_set_routes (ctx->client,
                                   input,
                                   10,
                                   ctx->cancellable,
                                   (GAsyncReadyCallback)set_routes_ready,
                                   NULL);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_WMS_SET_BROADCAST_CONFIG
    if (set_broadcast_config_str) {
        g_autoptr(QmiMessageWmsSetBroadcastConfigInput) input = NULL;
        g_autoptr(GError) error = NULL;

        input = set_broadcast_config_input_create (set_broadcast_config_str, &error);
        if (!input) {
            g_printerr ("Failed to set cbs channels: %s\n", error->message);
            operation_shutdown (FALSE);
            return;
        }
        g_debug ("Asynchronously setting CBS channels...");
        qmi_client_wms_set_broadcast_config (ctx->client,
                                         input,
                                         10,
                                         ctx->cancellable,
                                         (GAsyncReadyCallback)set_broadcast_config_ready,
                                         NULL);
        return;
    }
#endif
#if defined HAVE_QMI_MESSAGE_WMS_GET_BROADCAST_CONFIG
    if (get_broadcast_config_flag) {
        g_autoptr(QmiMessageWmsGetBroadcastConfigInput) input = NULL;
        g_autoptr(GError) error = NULL;

        input = qmi_message_wms_get_broadcast_config_input_new ();
        if (!qmi_message_wms_get_broadcast_config_input_set_message_mode (input,
                                                                          QMI_WMS_MESSAGE_MODE_GSM_WCDMA,
                                                                          &error)) {
            g_printerr ("Failed to get cbs channels: %s\n", error->message);
            return;
        }

        g_debug ("Asynchronously getting CBS channels...");
        qmi_client_wms_get_broadcast_config (ctx->client,
                                             input,
                                             10,
                                             ctx->cancellable,
                                             (GAsyncReadyCallback)get_broadcast_config_ready,
                                             NULL);
        return;

    }
#endif

#if defined HAVE_QMI_INDICATION_WMS_EVENT_REPORT
    if (monitor_flag) {
        g_debug ("Starting WMS monitoring...");
        start_wms_monitoring ();
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_WMS_SET_BROADCAST_ACTIVATION
    if (set_broadcast_activation_flag) {
        g_autoptr(QmiMessageWmsSetBroadcastActivationInput) input = NULL;

        g_debug ("Asynchronously activating broadcast...");
        input = qmi_message_wms_set_broadcast_activation_input_new ();
        qmi_message_wms_set_broadcast_activation_input_set_activation (
            input,
            QMI_WMS_MESSAGE_MODE_GSM_WCDMA,
            TRUE,
            NULL);
        qmi_client_wms_set_broadcast_activation (ctx->client,
                                                 input,
                                                 10,
                                                 ctx->cancellable,
                                                 (GAsyncReadyCallback)set_broadcast_activation_ready,
                                                 NULL);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_WMS_RESET
    if (reset_flag) {
        g_debug ("Asynchronously resetting WMS service...");
        qmi_client_wms_reset (ctx->client,
                              NULL,
                              10,
                              ctx->cancellable,
                              (GAsyncReadyCallback)reset_ready,
                              NULL);
        return;
    }
#endif

    /* Just client allocate/release? */
    if (noop_flag) {
        g_idle_add (noop_cb, NULL);
        return;
    }

    g_warn_if_reached ();
}

#endif /* HAVE_QMI_SERVICE_WMS */
