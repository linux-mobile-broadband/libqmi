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
 * Copyright (C) 2021 Aleksander Morgado <aleksander@aleksander.es>
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

#if defined HAVE_QMI_SERVICE_DPM

#define QMI_ENDPOINT_INTERFACE_NUMBER_UNDEFINED -1

/* Context */
typedef struct {
    QmiDevice *device;
    QmiClientDpm *client;
    GCancellable *cancellable;
} Context;
static Context *ctx;

/* Options */
static gchar    *open_port_str;
static gboolean  close_port_flag;
static gboolean  noop_flag;

static GOptionEntry entries[] = {
#if defined HAVE_QMI_MESSAGE_DPM_OPEN_PORT
    { "dpm-open-port", 0, 0, G_OPTION_ARG_STRING, &open_port_str,
      "Open port (allowed-keys: ctrl-ep-type, ctrl-ep-iface-number, ctrl-port-name, hw-data-ep-type, hw-data-ep-iface-number, hw-data-rx-id, hw-data-tx-id, sw-data-ep-type, sw-data-ep-iface-number, sw-data-port-name)",
      "[\"key=value,...\"]"
    },
#endif
#if defined HAVE_QMI_MESSAGE_DPM_CLOSE_PORT
    { "dpm-close-port", 0, 0, G_OPTION_ARG_NONE, &close_port_flag,
      "Close port",
      NULL
    },
#endif
    { "dpm-noop", 0, 0, G_OPTION_ARG_NONE, &noop_flag,
      "Just allocate or release a DPM client. Use with `--client-no-release-cid' and/or `--client-cid'",
      NULL
    },
    { NULL }
};

GOptionGroup *
qmicli_dpm_get_option_group (void)
{
    GOptionGroup *group;

    group = g_option_group_new ("dpm",
                                "DPM options:",
                                "Show Data Port Mapper Service options",
                                NULL,
                                NULL);
    g_option_group_add_entries (group, entries);

    return group;
}

gboolean
qmicli_dpm_options_enabled (void)
{
    static guint n_actions = 0;
    static gboolean checked = FALSE;

    if (checked)
        return !!n_actions;

    n_actions = (!!open_port_str +
                 close_port_flag +
                 noop_flag);

    if (n_actions > 1) {
        g_printerr ("error: too many DPM actions requested\n");
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

    if (context->cancellable)
        g_object_unref (context->cancellable);
    if (context->device)
        g_object_unref (context->device);
    if (context->client)
        g_object_unref (context->client);
    g_slice_free (Context, context);
}

static void
operation_shutdown (gboolean operation_status)
{
    /* Cleanup context and finish async operation */
    context_free (ctx);
    qmicli_async_operation_done (operation_status, FALSE);
}

#if defined HAVE_QMI_MESSAGE_DPM_OPEN_PORT

static void
open_port_ready (QmiClientDpm *client,
                 GAsyncResult *res)
{
    g_autoptr(QmiMessageDpmOpenPortOutput) output = NULL;
    g_autoptr(GError)                      error = NULL;

    output = qmi_client_dpm_open_port_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_dpm_open_port_output_get_result (output, &error)) {
        g_printerr ("error: couldn't open port: %s\n", error->message);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("Successfully opened the port\n");
    operation_shutdown (TRUE);
}

typedef struct {
    /* control port item building */
    GArray              *ctrl_ports;
    QmiDataEndpointType  ctrl_ep_type;
    gint                 ctrl_ep_iface_number;
    gchar               *ctrl_port_name;

    /* hw data port item building */
    GArray              *hw_data_ports;
    QmiDataEndpointType  hw_data_ep_type;
    gint                 hw_data_ep_iface_number;
    guint                hw_data_rx_id;
    guint                hw_data_tx_id;

    /* sw data port item building */
    GArray              *sw_data_ports;
    QmiDataEndpointType  sw_data_ep_type;
    gint                 sw_data_ep_iface_number;
    gchar               *sw_data_port_name;
} OpenPortProperties;

static void
reset_ctrl_port_item (OpenPortProperties *props)
{
    g_free (props->ctrl_port_name);
    props->ctrl_port_name       = NULL;
    props->ctrl_ep_type         = QMI_DATA_ENDPOINT_TYPE_UNKNOWN;
    props->ctrl_ep_iface_number = QMI_ENDPOINT_INTERFACE_NUMBER_UNDEFINED;
}

static void
ctrl_port_details_clear (QmiMessageDpmOpenPortInputControlPortsElement *details)
{
    g_free (details->port_name);
}

static void
build_ctrl_port_item (OpenPortProperties *props)
{
    if ((props->ctrl_ep_type != QMI_DATA_ENDPOINT_TYPE_UNKNOWN) &&
        (props->ctrl_ep_iface_number != QMI_ENDPOINT_INTERFACE_NUMBER_UNDEFINED) &&
        (props->ctrl_port_name != NULL)) {
        QmiMessageDpmOpenPortInputControlPortsElement details;

        details.port_name = g_strdup (props->ctrl_port_name);
        details.endpoint_type = props->ctrl_ep_type;
        details.interface_number = props->ctrl_ep_iface_number;

        if (!props->ctrl_ports) {
            props->ctrl_ports = g_array_new (FALSE, FALSE, sizeof (QmiMessageDpmOpenPortInputControlPortsElement));
            g_array_set_clear_func (props->ctrl_ports, (GDestroyNotify)ctrl_port_details_clear);
        }

        g_array_append_val (props->ctrl_ports, details);
        reset_ctrl_port_item (props);
    }
}

static gboolean
check_unfinished_ctrl_port_item (OpenPortProperties  *props,
                                 GError             **error)
{
    if ((props->ctrl_ep_type != QMI_DATA_ENDPOINT_TYPE_UNKNOWN) ||
        (props->ctrl_ep_iface_number != QMI_ENDPOINT_INTERFACE_NUMBER_UNDEFINED) ||
        (props->ctrl_port_name != NULL)) {
        g_set_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_FAILED,
                     "error: unfinished ctrl port item");
        return FALSE;
    }
    return TRUE;
}

static void
reset_hw_data_port_item (OpenPortProperties *props)
{
    props->hw_data_rx_id           = 0;
    props->hw_data_tx_id           = 0;
    props->hw_data_ep_type         = QMI_DATA_ENDPOINT_TYPE_UNKNOWN;
    props->hw_data_ep_iface_number = QMI_ENDPOINT_INTERFACE_NUMBER_UNDEFINED;
}

static void
build_hw_data_port_item (OpenPortProperties *props)
{
    if ((props->hw_data_ep_type != QMI_DATA_ENDPOINT_TYPE_UNKNOWN) &&
        (props->hw_data_ep_iface_number != QMI_ENDPOINT_INTERFACE_NUMBER_UNDEFINED) &&
        (props->hw_data_rx_id != 0) &&
        (props->hw_data_tx_id != 0)) {
        QmiMessageDpmOpenPortInputHardwareDataPortsElement details;

        details.rx_endpoint_number = props->hw_data_rx_id;
        details.tx_endpoint_number = props->hw_data_tx_id;
        details.endpoint_type = props->hw_data_ep_type;
        details.interface_number = props->hw_data_ep_iface_number;

        if (!props->hw_data_ports)
            props->hw_data_ports = g_array_new (FALSE, FALSE, sizeof (QmiMessageDpmOpenPortInputHardwareDataPortsElement));

        g_array_append_val (props->hw_data_ports, details);
        reset_hw_data_port_item (props);
    }
}

static gboolean
check_unfinished_hw_data_port_item (OpenPortProperties  *props,
                                    GError            **error)
{
    if ((props->hw_data_ep_type != QMI_DATA_ENDPOINT_TYPE_UNKNOWN) ||
        (props->hw_data_ep_iface_number != QMI_ENDPOINT_INTERFACE_NUMBER_UNDEFINED) ||
        (props->hw_data_rx_id != 0) ||
        (props->hw_data_tx_id != 0)) {
        g_set_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_FAILED,
                     "error: unfinished hw data port item");
        return FALSE;
    }
    return TRUE;
}

static void
reset_sw_data_port_item (OpenPortProperties *props)
{
    g_free (props->sw_data_port_name);
    props->sw_data_port_name       = NULL;
    props->sw_data_ep_type         = QMI_DATA_ENDPOINT_TYPE_UNKNOWN;
    props->sw_data_ep_iface_number = QMI_ENDPOINT_INTERFACE_NUMBER_UNDEFINED;
}

static void
sw_data_port_details_clear (QmiMessageDpmOpenPortInputSoftwareDataPortsElement *details)
{
    g_free (details->port_name);
}

static void
build_sw_data_port_item (OpenPortProperties *props)
{
    if ((props->sw_data_ep_type != QMI_DATA_ENDPOINT_TYPE_UNKNOWN) &&
        (props->sw_data_ep_iface_number != QMI_ENDPOINT_INTERFACE_NUMBER_UNDEFINED) &&
        (props->sw_data_port_name != NULL)) {
        QmiMessageDpmOpenPortInputSoftwareDataPortsElement details;

        details.port_name = g_strdup (props->sw_data_port_name);
        details.endpoint_type = props->sw_data_ep_type;
        details.interface_number = props->sw_data_ep_iface_number;

        if (!props->sw_data_ports) {
            props->sw_data_ports = g_array_new (FALSE, FALSE, sizeof (QmiMessageDpmOpenPortInputSoftwareDataPortsElement));
            g_array_set_clear_func (props->sw_data_ports, (GDestroyNotify)sw_data_port_details_clear);
        }

        g_array_append_val (props->sw_data_ports, details);
        reset_sw_data_port_item (props);
    }
}

static gboolean
check_unfinished_sw_data_port_item (OpenPortProperties  *props,
                                    GError             **error)
{
    if ((props->sw_data_ep_type != QMI_DATA_ENDPOINT_TYPE_UNKNOWN) ||
        (props->sw_data_ep_iface_number != QMI_ENDPOINT_INTERFACE_NUMBER_UNDEFINED) ||
        (props->sw_data_port_name != NULL)) {
        g_set_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_FAILED,
                     "error: unfinished sw data port item");
        return FALSE;
    }
    return TRUE;
}

static gboolean
open_port_properties_handle (const gchar  *key,
                             const gchar  *value,
                             GError      **error,
                             gpointer      user_data)
{
    OpenPortProperties *props = (OpenPortProperties *)user_data;

    if (!value || !value[0]) {
        g_set_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_FAILED,
                     "key '%s' requires a value", key);
        return FALSE;
    }

    /* control port item */
    if (g_ascii_strcasecmp (key, "ctrl-ep-type") == 0) {
        if (!qmicli_read_data_endpoint_type_from_string (value, &(props->ctrl_ep_type))) {
            g_set_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_FAILED,
                         "Unrecognized Endpoint Type '%s'", value);
            return FALSE;
        }
        build_ctrl_port_item (props);
        return TRUE;
    }
    if (g_ascii_strcasecmp (key, "ctrl-ep-iface-number") == 0) {
        props->ctrl_ep_iface_number = atoi (value);
        build_ctrl_port_item (props);
        return TRUE;
    }
    if (g_ascii_strcasecmp (key, "ctrl-port-name") == 0) {
        g_clear_pointer (&props->ctrl_port_name, g_free);
        props->ctrl_port_name = g_strdup (value);
        build_ctrl_port_item (props);
        return TRUE;
    }

    /* hw data port item */
    if (g_ascii_strcasecmp (key, "hw-data-ep-type") == 0) {
        if (!qmicli_read_data_endpoint_type_from_string (value, &(props->hw_data_ep_type))) {
            g_set_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_FAILED,
                         "Unrecognized Endpoint Type '%s'", value);
            return FALSE;
        }
        build_hw_data_port_item (props);
        return TRUE;
    }
    if (g_ascii_strcasecmp (key, "hw-data-ep-iface-number") == 0) {
        props->hw_data_ep_iface_number = atoi (value);
        build_hw_data_port_item (props);
        return TRUE;
    }
    if (g_ascii_strcasecmp (key, "hw-data-rx-id") == 0) {
        props->hw_data_rx_id = atoi (value);
        build_hw_data_port_item (props);
        return TRUE;
    }
    if (g_ascii_strcasecmp (key, "hw-data-tx-id") == 0) {
        props->hw_data_tx_id = atoi (value);
        build_hw_data_port_item (props);
        return TRUE;
    }

    /* sw data port item */
    if (g_ascii_strcasecmp (key, "sw-data-ep-type") == 0) {
        if (!qmicli_read_data_endpoint_type_from_string (value, &(props->sw_data_ep_type))) {
            g_set_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_FAILED,
                         "Unrecognized Endpoint Type '%s'", value);
            return FALSE;
        }
        build_sw_data_port_item (props);
        return TRUE;
    }
    if (g_ascii_strcasecmp (key, "sw-data-ep-iface-number") == 0) {
        props->sw_data_ep_iface_number = atoi (value);
        build_sw_data_port_item (props);
        return TRUE;
    }
    if (g_ascii_strcasecmp (key, "sw-data-port-name") == 0) {
        g_clear_pointer (&props->sw_data_port_name, g_free);
        props->sw_data_port_name = g_strdup (value);
        build_sw_data_port_item (props);
        return TRUE;
    }

    g_set_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_FAILED,
                 "Unrecognized option '%s'", key);
    return FALSE;
}

static gboolean
open_port_input_create (const gchar                 *str,
                        QmiMessageDpmOpenPortInput **out_input,
                        GError                     **error)
{
    QmiMessageDpmOpenPortInput *input;
    OpenPortProperties          props = { 0 };

    reset_ctrl_port_item    (&props);
    reset_hw_data_port_item (&props);
    reset_sw_data_port_item (&props);

    if (!qmicli_parse_key_value_string (str, error, open_port_properties_handle, &props))
        return FALSE;

    if (!check_unfinished_ctrl_port_item    (&props, error) ||
        !check_unfinished_hw_data_port_item (&props, error) ||
        !check_unfinished_sw_data_port_item (&props, error))
        return FALSE;

    input = qmi_message_dpm_open_port_input_new ();
    if (props.ctrl_ports) {
        qmi_message_dpm_open_port_input_set_control_ports (input, props.ctrl_ports, NULL);
        g_clear_pointer (&props.ctrl_ports, g_array_unref);
    }
    if (props.hw_data_ports) {
        qmi_message_dpm_open_port_input_set_hardware_data_ports (input, props.hw_data_ports, NULL);
        g_clear_pointer (&props.hw_data_ports, g_array_unref);
    }
    if (props.sw_data_ports) {
        qmi_message_dpm_open_port_input_set_software_data_ports (input, props.sw_data_ports, NULL);
        g_clear_pointer (&props.sw_data_ports, g_array_unref);
    }

    *out_input = input;
    return TRUE;
}

#endif /* HAVE_QMI_MESSAGE_DPM_OPEN_PORT */

#if defined HAVE_QMI_MESSAGE_DPM_CLOSE_PORT

static void
close_port_ready (QmiClientDpm *client,
                 GAsyncResult *res)
{
    g_autoptr(QmiMessageDpmClosePortOutput) output = NULL;
    g_autoptr(GError)                      error = NULL;

    output = qmi_client_dpm_close_port_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_dpm_close_port_output_get_result (output, &error)) {
        g_printerr ("error: couldn't close port: %s\n", error->message);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("Successfully closeed the port\n");
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_DPM_CLOSE_PORT */

static gboolean
noop_cb (gpointer unused)
{
    operation_shutdown (TRUE);
    return FALSE;
}

void
qmicli_dpm_run (QmiDevice    *device,
                QmiClientDpm *client,
                GCancellable *cancellable)
{
    /* Initialize context */
    ctx = g_slice_new (Context);
    ctx->device = g_object_ref (device);
    ctx->client = g_object_ref (client);
    ctx->cancellable = g_object_ref (cancellable);

#if defined HAVE_QMI_MESSAGE_DPM_OPEN_PORT
    if (open_port_str) {
        g_autoptr(QmiMessageDpmOpenPortInput) input = NULL;
        g_autoptr(GError)                     error = NULL;

        if (!open_port_input_create (open_port_str, &input, &error)) {
            g_printerr ("error: couldn't process input arguments: %s\n", error->message);
            operation_shutdown (FALSE);
            return;
        }
        qmi_client_dpm_open_port (ctx->client,
                                  input,
                                  10,
                                  ctx->cancellable,
                                  (GAsyncReadyCallback)open_port_ready,
                                  NULL);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_DPM_CLOSE_PORT
    if (close_port_flag) {
        qmi_client_dpm_close_port (ctx->client,
                                   NULL,
                                   10,
                                   ctx->cancellable,
                                   (GAsyncReadyCallback)close_port_ready,
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

#endif /* HAVE_QMI_SERVICE_DPM */
