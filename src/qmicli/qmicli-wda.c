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
 * Copyright (C) 2014-2017 Aleksander Morgado <aleksander@aleksander.es>
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

#if defined HAVE_QMI_SERVICE_WDA

#define QMI_WDA_DL_AGGREGATION_PROTOCOL_MAX_DATAGRAMS_UNDEFINED 0xFFFFFFFF
#define QMI_WDA_DL_AGGREGATION_PROTOCOL_MAX_DATAGRAM_SIZE_UNDEFINED 0xFFFFFFFF
#define QMI_WDA_ENDPOINT_INTERFACE_NUMBER_UNDEFINED -1

/* Context */
typedef struct {
    QmiDevice *device;
    QmiClientWda *client;
    GCancellable *cancellable;
} Context;
static Context *ctx;

/* Options */
static gchar    *set_data_format_str;
static gchar    *get_data_format_str;
static gboolean  get_data_format_flag;
static gboolean  get_supported_messages_flag;
static gboolean  noop_flag;

#if defined HAVE_QMI_MESSAGE_WDA_GET_DATA_FORMAT

static gboolean
parse_get_data_format (const gchar  *option_name,
                       const gchar  *value,
                       gpointer      data,
                       GError      **error)
{
    get_data_format_flag = TRUE;
    if (value && value[0])
        get_data_format_str = g_strdup (value);
    return TRUE;
}

#endif

static GOptionEntry entries[] = {
#if defined HAVE_QMI_MESSAGE_WDA_SET_DATA_FORMAT
    { "wda-set-data-format", 0, 0, G_OPTION_ARG_STRING, &set_data_format_str,
      "Set data format (allowed keys: link-layer-protocol (802-3|raw-ip), ul-protocol (disabled|tlp|qc-ncm|mbim|rndis|qmap|qmapv5), dl-protocol (disabled|tlp|qc-ncm|mbim|rndis|qmap|qmapv5), dl-datagram-max-size, dl-max-datagrams, ep-type (undefined|hsusb|pcie|embedded), ep-iface-number)",
      "[\"key=value,...\"]"
    },
#endif
#if defined HAVE_QMI_MESSAGE_WDA_GET_DATA_FORMAT
    { "wda-get-data-format", 0, G_OPTION_FLAG_OPTIONAL_ARG, G_OPTION_ARG_CALLBACK, parse_get_data_format,
      "Get data format (allowed keys: ep-type (undefined|hsusb|pcie|embedded), ep-iface-number); also allows empty key list",
      "[\"key=value,...\"]"
    },
#endif
#if defined HAVE_QMI_MESSAGE_WDA_GET_SUPPORTED_MESSAGES
    { "wda-get-supported-messages", 0, 0, G_OPTION_ARG_NONE, &get_supported_messages_flag,
      "Get supported messages",
      NULL
    },
#endif
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
                                "WDA options:",
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
    qmicli_async_operation_done (operation_status, FALSE);
}

static gboolean
noop_cb (gpointer unused)
{
    operation_shutdown (TRUE);
    return FALSE;
}

#if defined HAVE_QMI_MESSAGE_WDA_GET_DATA_FORMAT

typedef struct {
    QmiDataEndpointType endpoint_type;
    gint                endpoint_iface_number;
} GetDataFormatProperties;

static gboolean
get_data_format_properties_handle (const gchar  *key,
                                   const gchar  *value,
                                   GError      **error,
                                   gpointer      user_data)
{
    GetDataFormatProperties *props = (GetDataFormatProperties *)user_data;

    if (!value || !value[0]) {
        g_set_error (error,
                     QMI_CORE_ERROR,
                     QMI_CORE_ERROR_FAILED,
                     "key '%s' requires a value",
                     key);
        return FALSE;
    }

    if (g_ascii_strcasecmp (key, "ep-type") == 0) {
        if (!qmicli_read_data_endpoint_type_from_string (value, &(props->endpoint_type))) {
            g_set_error (error,
                         QMI_CORE_ERROR,
                         QMI_CORE_ERROR_FAILED,
                         "Unrecognized Endpoint Type '%s'",
                         value);
            return FALSE;
        }
        return TRUE;
    }

    if (g_ascii_strcasecmp (key, "ep-iface-number") == 0) {
        props->endpoint_iface_number = atoi (value);
        return TRUE;
    }

    g_set_error (error,
                 QMI_CORE_ERROR,
                 QMI_CORE_ERROR_FAILED,
                 "Unrecognized option '%s'",
                 key);
    return FALSE;
}

static QmiMessageWdaGetDataFormatInput *
get_data_format_input_create (const gchar *str)
{
    g_autoptr(QmiMessageWdaGetDataFormatInput) input = NULL;
    g_autoptr(GError) error = NULL;
    GetDataFormatProperties props = {
        .endpoint_type         = QMI_DATA_ENDPOINT_TYPE_UNDEFINED,
        .endpoint_iface_number = QMI_WDA_ENDPOINT_INTERFACE_NUMBER_UNDEFINED,
    };

    input = qmi_message_wda_get_data_format_input_new ();

    if (!qmicli_parse_key_value_string (str,
                                        &error,
                                        get_data_format_properties_handle,
                                        &props)) {
        g_printerr ("error: could not parse input string '%s'\n", error->message);
        return NULL;
    }

    if ((props.endpoint_type == QMI_DATA_ENDPOINT_TYPE_UNDEFINED) ^
        (props.endpoint_iface_number == QMI_WDA_ENDPOINT_INTERFACE_NUMBER_UNDEFINED)) {
        g_printerr ("error: endpoint type and interface number must be both set or both unset\n");
        return NULL;
    }

    if ((props.endpoint_type != QMI_DATA_ENDPOINT_TYPE_UNDEFINED) &&
        (props.endpoint_iface_number != QMI_WDA_ENDPOINT_INTERFACE_NUMBER_UNDEFINED) &&
        !qmi_message_wda_get_data_format_input_set_endpoint_info (
            input,
            props.endpoint_type,
            props.endpoint_iface_number,
            &error)) {
        g_printerr ("error: could not set peripheral endpoint id: %s\n", error->message);
        return NULL;
    }

    return g_steal_pointer (&input);
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
    guint32 data_aggregation_max_datagrams;

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

    if (qmi_message_wda_get_data_format_output_get_downlink_data_aggregation_max_datagrams (
            output,
            &data_aggregation_max_datagrams,
            NULL))
        g_print ("Downlink data aggregation max datagrams: '%u'\n", data_aggregation_max_datagrams);

    if (qmi_message_wda_get_data_format_output_get_downlink_data_aggregation_max_size (
            output,
            &data_aggregation_max_size,
            NULL))
        g_print ("Downlink data aggregation max size: '%u'\n", data_aggregation_max_size);

    qmi_message_wda_get_data_format_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_WDA_GET_DATA_FORMAT */

#if defined HAVE_QMI_MESSAGE_WDA_SET_DATA_FORMAT

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

typedef struct {
    QmiWdaLinkLayerProtocol link_layer_protocol;
    QmiWdaDataAggregationProtocol ul_protocol;
    QmiWdaDataAggregationProtocol dl_protocol;
    guint32 dl_datagram_max_size;
    guint32 dl_max_datagrams;
    QmiDataEndpointType endpoint_type;
    gint endpoint_iface_number;
} SetDataFormatProperties;


static gboolean
set_data_format_properties_handle (const gchar  *key,
                                   const gchar  *value,
                                   GError      **error,
                                   gpointer      user_data)
{
    SetDataFormatProperties *props = (SetDataFormatProperties *)user_data;

    if (!value || !value[0]) {
        g_set_error (error,
                     QMI_CORE_ERROR,
                     QMI_CORE_ERROR_FAILED,
                     "key '%s' requires a value",
                     key);
        return FALSE;
    }

    if (g_ascii_strcasecmp (key, "link-layer-protocol") == 0) {
        if (!qmicli_read_wda_link_layer_protocol_from_string (value, &(props->link_layer_protocol))) {
            g_set_error (error,
                         QMI_CORE_ERROR,
                         QMI_CORE_ERROR_FAILED,
                         "Unrecognized Link Layer Protocol '%s'",
                         value);
            return FALSE;
        }
        return TRUE;
    }

    if (g_ascii_strcasecmp (key, "ul-protocol") == 0) {
        if (!qmicli_read_wda_data_aggregation_protocol_from_string (value, &(props->ul_protocol))) {
            g_set_error (error,
                         QMI_CORE_ERROR,
                         QMI_CORE_ERROR_FAILED,
                         "Unrecognized Data Aggregation Protocol '%s'",
                         value);
            return FALSE;
        }
        return TRUE;
    }

    if (g_ascii_strcasecmp (key, "dl-protocol") == 0) {
        if (!qmicli_read_wda_data_aggregation_protocol_from_string (value, &(props->dl_protocol))) {
            g_set_error (error,
                         QMI_CORE_ERROR,
                         QMI_CORE_ERROR_FAILED,
                         "Unrecognized Data Aggregation Protocol '%s'",
                         value);
            return FALSE;
        }
        return TRUE;
    }

    if (g_ascii_strcasecmp (key, "dl-datagram-max-size") == 0) {
        props->dl_datagram_max_size = atoi(value);
        return TRUE;
    }

    if (g_ascii_strcasecmp (key, "dl-max-datagrams") == 0) {
        props->dl_max_datagrams = atoi(value);
        return TRUE;
    }

    if (g_ascii_strcasecmp (key, "ep-type") == 0) {
        if (!qmicli_read_data_endpoint_type_from_string (value, &(props->endpoint_type))) {
            g_set_error (error,
                         QMI_CORE_ERROR,
                         QMI_CORE_ERROR_FAILED,
                         "Unrecognized Endpoint Type '%s'",
                         value);
            return FALSE;
        }
        return TRUE;
    }

    if (g_ascii_strcasecmp (key, "ep-iface-number") == 0) {
        props->endpoint_iface_number = atoi(value);
        return TRUE;
    }

    g_set_error (error,
                 QMI_CORE_ERROR,
                 QMI_CORE_ERROR_FAILED,
                 "Unrecognized option '%s'",
                 key);
    return FALSE;
}

static QmiMessageWdaSetDataFormatInput *
set_data_format_input_create (const gchar *str)
{
    QmiMessageWdaSetDataFormatInput *input = NULL;
    GError *error = NULL;
    SetDataFormatProperties props = {
        .link_layer_protocol   = QMI_WDA_LINK_LAYER_PROTOCOL_UNKNOWN,
        .ul_protocol           = QMI_WDA_DATA_AGGREGATION_PROTOCOL_DISABLED,
        .dl_protocol           = QMI_WDA_DATA_AGGREGATION_PROTOCOL_DISABLED,
        .dl_datagram_max_size  = QMI_WDA_DL_AGGREGATION_PROTOCOL_MAX_DATAGRAM_SIZE_UNDEFINED,
        .dl_max_datagrams      = QMI_WDA_DL_AGGREGATION_PROTOCOL_MAX_DATAGRAMS_UNDEFINED,
        .endpoint_type         = QMI_DATA_ENDPOINT_TYPE_UNDEFINED,
        .endpoint_iface_number = QMI_WDA_ENDPOINT_INTERFACE_NUMBER_UNDEFINED,
    };

    input = qmi_message_wda_set_data_format_input_new ();

    /* New key=value format */
    if (strchr (str, '=')) {
        if (!qmicli_parse_key_value_string (str,
                                            &error,
                                            set_data_format_properties_handle,
                                            &props)) {
            g_printerr ("error: could not parse input string '%s'\n", error->message);
            g_error_free (error);
            goto error_out;
        }

        if (!qmi_message_wda_set_data_format_input_set_uplink_data_aggregation_protocol (
                input,
                props.ul_protocol,
                &error)) {
            g_printerr ("error: could not set Upload data aggregation protocol '%d': %s\n",
                        props.ul_protocol, error->message);
            g_error_free (error);
            goto error_out;
        }

        if (!qmi_message_wda_set_data_format_input_set_downlink_data_aggregation_protocol (
                input,
                props.dl_protocol,
                &error)) {
            g_printerr ("error: could not set Download data aggregation protocol '%d': %s\n",
                        props.dl_protocol, error->message);
            g_error_free (error);
            goto error_out;
        }

        if (props.dl_datagram_max_size != QMI_WDA_DL_AGGREGATION_PROTOCOL_MAX_DATAGRAM_SIZE_UNDEFINED &&
            !qmi_message_wda_set_data_format_input_set_downlink_data_aggregation_max_size (
                input,
                props.dl_datagram_max_size,
                &error)) {
            g_printerr ("error: could not set Download data aggregation max size %d: %s\n",
                        props.dl_datagram_max_size, error->message);
            g_error_free (error);
            goto error_out;
        }

        if (props.dl_max_datagrams != QMI_WDA_DL_AGGREGATION_PROTOCOL_MAX_DATAGRAMS_UNDEFINED &&
            !qmi_message_wda_set_data_format_input_set_downlink_data_aggregation_max_datagrams (
                input,
                props.dl_max_datagrams,
                &error)) {
            g_printerr ("error: could not set Download data aggregation max datagrams %d: %s\n",
                        props.dl_max_datagrams, error->message);
            g_error_free (error);
            goto error_out;

        }

        if ((props.endpoint_type == QMI_DATA_ENDPOINT_TYPE_UNDEFINED) ^
            (props.endpoint_iface_number == QMI_WDA_ENDPOINT_INTERFACE_NUMBER_UNDEFINED)) {
            g_printerr ("error: endpoint type and interface number must be both set or both unset\n");
            goto error_out;
        }

        if ((props.endpoint_type != QMI_DATA_ENDPOINT_TYPE_UNDEFINED) &&
            (props.endpoint_iface_number != QMI_WDA_ENDPOINT_INTERFACE_NUMBER_UNDEFINED) &&
            !qmi_message_wda_set_data_format_input_set_endpoint_info (
                input,
                props.endpoint_type,
                props.endpoint_iface_number,
                &error)) {
            g_printerr ("error: could not set peripheral endpoint id: %s\n", error->message);
            g_error_free (error);
            goto error_out;
        }
    }
    /* Old non key=value format, like this:
     *    "[(raw-ip|802-3)]"
     */
    else {
        if (!qmicli_read_wda_link_layer_protocol_from_string (str, &(props.link_layer_protocol))) {
            g_printerr ("Unrecognized Link Layer Protocol '%s'\n", str);
            goto error_out;
        }
    }

    if (props.link_layer_protocol == QMI_WDA_LINK_LAYER_PROTOCOL_UNKNOWN) {
        g_printerr ("error: Link Layer Protocol value is missing\n");
        goto error_out;
    }

    if (!qmi_message_wda_set_data_format_input_set_link_layer_protocol (
            input,
            props.link_layer_protocol,
            &error)) {
        g_printerr ("error: couldn't create input data bundle: '%s'\n",
                    error->message);
        g_error_free (error);
        goto error_out;
    }

    return input;

error_out:
    qmi_message_wda_set_data_format_input_unref (input);
    return NULL;
}

#endif /* HAVE_QMI_MESSAGE_WDA_SET_DATA_FORMAT */

#if defined HAVE_QMI_MESSAGE_WDA_GET_SUPPORTED_MESSAGES

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

#endif /* HAVE_QMI_MESSAGE_WDA_GET_SUPPORTED_MESSAGES */

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

#if defined HAVE_QMI_MESSAGE_WDA_SET_DATA_FORMAT
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
#endif

#if defined HAVE_QMI_MESSAGE_WDA_GET_DATA_FORMAT
    if (get_data_format_flag) {
        g_autoptr(QmiMessageWdaGetDataFormatInput) input = NULL;

        if (get_data_format_str) {
            input = get_data_format_input_create (get_data_format_str);
            if (!input) {
                operation_shutdown (FALSE);
                return;
            }
        }

        g_debug ("Asynchronously getting data format...");
        qmi_client_wda_get_data_format (ctx->client,
                                        input,
                                        10,
                                        ctx->cancellable,
                                        (GAsyncReadyCallback)get_data_format_ready,
                                        NULL);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_WDA_GET_SUPPORTED_MESSAGES
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
#endif

    /* Just client allocate/release? */
    if (noop_flag) {
        g_idle_add (noop_cb, NULL);
        return;
    }

    g_warn_if_reached ();
}

#endif /* HAVE_QMI_SERVICE_WDA */
