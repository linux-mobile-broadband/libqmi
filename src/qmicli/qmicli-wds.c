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
 * Copyright (C) 2015 Velocloud Inc.
 * Copyright (C) 2012-2017 Aleksander Morgado <aleksander@aleksander.es>
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <string.h>
#include <arpa/inet.h>

#include <glib.h>
#include <gio/gio.h>

#include <libqmi-glib.h>

#include "qmicli.h"
#include "qmicli-helpers.h"

#if defined HAVE_QMI_SERVICE_WDS

#undef VALIDATE_MASK_NONE
#define VALIDATE_MASK_NONE(str) (str ? str : "none")

#define QMI_WDS_MUX_ID_UNDEFINED 0xFF
#define QMI_WDS_ENDPOINT_INTERFACE_NUMBER_UNDEFINED -1

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
static gchar *start_network_str;
static gboolean follow_network_flag;
static gchar *stop_network_str;
static gboolean get_current_settings_flag;
static gboolean get_packet_service_status_flag;
static gboolean get_packet_statistics_flag;
static gboolean get_data_bearer_technology_flag;
static gboolean get_current_data_bearer_technology_flag;
static gboolean go_dormant_flag;
static gboolean go_active_flag;
static gboolean get_dormancy_status_flag;
static gchar *create_profile_str;
static gchar *swi_create_profile_indexed_str;
static gchar *modify_profile_str;
static gchar *delete_profile_str;
static gchar *get_profile_list_str;
static gchar *get_default_profile_num_str; /* deprecated */
static gchar *get_default_profile_number_str;
static gchar *set_default_profile_num_str; /* deprecated */
static gchar *set_default_profile_number_str;
static gchar *get_default_settings_str;
static gboolean get_autoconnect_settings_flag;
static gchar *set_autoconnect_settings_str;
static gboolean get_supported_messages_flag;
static gboolean reset_flag;
static gboolean noop_flag;
static gchar *bind_data_port_str;
static gchar *bind_mux_str;
static gchar *set_ip_family_str;
static gboolean get_channel_rates_flag;
static gboolean get_lte_attach_parameters_flag;
static gboolean get_max_lte_attach_pdn_number_flag;
static gboolean get_lte_attach_pdn_list_flag;
static gchar *set_lte_attach_pdn_list_str;

static GOptionEntry entries[] = {
#if defined HAVE_QMI_MESSAGE_WDS_START_NETWORK
    { "wds-start-network", 0, 0, G_OPTION_ARG_STRING, &start_network_str,
      "Start network (allowed keys: apn, 3gpp-profile, 3gpp2-profile, auth (PAP|CHAP|BOTH), username, password, autoconnect=yes, ip-type (4|6))",
      "[\"key=value,...\"]"
    },
# if defined HAVE_QMI_MESSAGE_WDS_STOP_NETWORK && defined HAVE_QMI_MESSAGE_WDS_GET_PACKET_SERVICE_STATUS
    { "wds-follow-network", 0, 0, G_OPTION_ARG_NONE, &follow_network_flag,
      "Follow the network status until disconnected. Use with `--wds-start-network'",
      NULL
    },
# endif
#endif
#if defined HAVE_QMI_MESSAGE_WDS_STOP_NETWORK
    { "wds-stop-network", 0, 0, G_OPTION_ARG_STRING, &stop_network_str,
      "Stop network",
      "[Packet data handle] OR [disable-autoconnect]",
    },
#endif
#if defined HAVE_QMI_MESSAGE_WDS_GET_CURRENT_SETTINGS
    { "wds-get-current-settings", 0, 0, G_OPTION_ARG_NONE, &get_current_settings_flag,
      "Get current settings",
      NULL
    },
#endif
#if defined HAVE_QMI_MESSAGE_WDS_GET_PACKET_SERVICE_STATUS
    { "wds-get-packet-service-status", 0, 0, G_OPTION_ARG_NONE, &get_packet_service_status_flag,
      "Get packet service status",
      NULL
    },
#endif
#if defined HAVE_QMI_MESSAGE_WDS_GET_PACKET_STATISTICS
    { "wds-get-packet-statistics", 0, 0, G_OPTION_ARG_NONE, &get_packet_statistics_flag,
      "Get packet statistics",
      NULL
    },
#endif
#if defined HAVE_QMI_MESSAGE_WDS_GET_DATA_BEARER_TECHNOLOGY
    { "wds-get-data-bearer-technology", 0, 0, G_OPTION_ARG_NONE, &get_data_bearer_technology_flag,
      "Get data bearer technology",
      NULL
    },
#endif
#if defined HAVE_QMI_MESSAGE_WDS_GET_CURRENT_DATA_BEARER_TECHNOLOGY
    { "wds-get-current-data-bearer-technology", 0, 0, G_OPTION_ARG_NONE, &get_current_data_bearer_technology_flag,
      "Get current data bearer technology",
      NULL
    },
#endif
#if defined HAVE_QMI_MESSAGE_WDS_GO_DORMANT
    { "wds-go-dormant", 0, 0, G_OPTION_ARG_NONE, &go_dormant_flag,
      "Make the active data connection go dormant",
      NULL
    },
#endif
#if defined HAVE_QMI_MESSAGE_WDS_GO_ACTIVE
    { "wds-go-active", 0, 0, G_OPTION_ARG_NONE, &go_active_flag,
      "Make the active data connection go active",
      NULL
    },
#endif
#if defined HAVE_QMI_MESSAGE_WDS_GET_DORMANCY_STATUS
    { "wds-get-dormancy-status", 0, 0, G_OPTION_ARG_NONE, &get_dormancy_status_flag,
      "Get the dormancy status of the active data connection",
      NULL
    },
#endif
#if defined HAVE_QMI_MESSAGE_WDS_CREATE_PROFILE
    { "wds-create-profile", 0, 0, G_OPTION_ARG_STRING, &create_profile_str,
      "Create new profile using first available profile index (optional keys: name, apn, pdp-type (IP|PPP|IPV6|IPV4V6), auth (NONE|PAP|CHAP|BOTH), username, password, context-num, no-roaming=yes, disabled=yes)",
      "[\"(3gpp|3gpp2)[,key=value,...]\"]"
    },
#endif
#if defined HAVE_QMI_MESSAGE_WDS_SWI_CREATE_PROFILE_INDEXED
    { "wds-swi-create-profile-indexed", 0, 0, G_OPTION_ARG_STRING, &swi_create_profile_indexed_str,
      "Create new profile at specified profile index [Sierra Wireless specific] (optional keys: name, apn, pdp-type (IP|PPP|IPV6|IPV4V6), auth (NONE|PAP|CHAP|BOTH), username, password, context-num, no-roaming=yes, disabled=yes)",
      "[\"(3gpp|3gpp2),#[,key=value,...]\"]"
    },
#endif
#if defined HAVE_QMI_MESSAGE_WDS_MODIFY_PROFILE
    { "wds-modify-profile", 0, 0, G_OPTION_ARG_STRING, &modify_profile_str,
      "Modify existing profile (optional keys: name, apn, pdp-type (IP|PPP|IPV6|IPV4V6), auth (NONE|PAP|CHAP|BOTH), username, password, context-num, no-roaming=yes, disabled=yes)",
      "[\"(3gpp|3gpp2),#,key=value,...\"]"
    },
#endif
#if defined HAVE_QMI_MESSAGE_WDS_DELETE_PROFILE
    { "wds-delete-profile", 0, 0, G_OPTION_ARG_STRING, &delete_profile_str,
      "Delete existing profile",
      "[(3gpp|3gpp2),#]"
    },
#endif
#if defined HAVE_QMI_MESSAGE_WDS_GET_PROFILE_LIST && defined HAVE_QMI_MESSAGE_WDS_GET_PROFILE_SETTINGS
    { "wds-get-profile-list", 0, 0, G_OPTION_ARG_STRING, &get_profile_list_str,
      "Get profile list",
      "[3gpp|3gpp2]"
    },
#endif
#if defined HAVE_QMI_MESSAGE_WDS_GET_DEFAULT_PROFILE_NUMBER
    { "wds-get-default-profile-number", 0, 0, G_OPTION_ARG_STRING, &get_default_profile_number_str,
      "Get default profile number",
      "[3gpp|3gpp2]"
    },
#endif
#if defined HAVE_QMI_MESSAGE_WDS_SET_DEFAULT_PROFILE_NUMBER
    { "wds-set-default-profile-number", 0, 0, G_OPTION_ARG_STRING, &set_default_profile_number_str,
      "Set default profile number",
      "[(3gpp|3gpp2),#]"
    },
#endif
#if defined HAVE_QMI_MESSAGE_WDS_GET_DEFAULT_SETTINGS
    { "wds-get-default-settings", 0, 0, G_OPTION_ARG_STRING, &get_default_settings_str,
      "Get default settings",
      "[3gpp|3gpp2]"
    },
#endif
#if defined HAVE_QMI_MESSAGE_WDS_GET_AUTOCONNECT_SETTINGS
    { "wds-get-autoconnect-settings", 0, 0, G_OPTION_ARG_NONE, &get_autoconnect_settings_flag,
      "Get autoconnect settings",
      NULL
    },
#endif
#if defined HAVE_QMI_MESSAGE_WDS_SET_AUTOCONNECT_SETTINGS
    { "wds-set-autoconnect-settings", 0, 0, G_OPTION_ARG_STRING, &set_autoconnect_settings_str,
      "Set autoconnect settings (roaming settings optional)",
      "[(enabled|disabled|paused)[,(roaming-allowed|home-only)]]"
    },
#endif
#if defined HAVE_QMI_MESSAGE_WDS_GET_SUPPORTED_MESSAGES
    { "wds-get-supported-messages", 0, 0, G_OPTION_ARG_NONE, &get_supported_messages_flag,
      "Get supported messages",
      NULL
    },
#endif
#if defined HAVE_QMI_MESSAGE_WDS_RESET
    { "wds-reset", 0, 0, G_OPTION_ARG_NONE, &reset_flag,
      "Reset the service state",
      NULL
    },
#endif
#if defined HAVE_QMI_MESSAGE_WDS_BIND_DATA_PORT
    { "wds-bind-data-port", 0, 0, G_OPTION_ARG_STRING, &bind_data_port_str,
      "Bind data port to controller device to be used with `--client-no-release-cid'",
      "[a2-mux-rmnet0-7|#]"
    },
#endif
#if defined HAVE_QMI_MESSAGE_WDS_BIND_MUX_DATA_PORT
    { "wds-bind-mux-data-port", 0, 0, G_OPTION_ARG_STRING, &bind_mux_str,
      "Bind qmux data port to controller device (allowed keys: mux-id, ep-type (undefined|hsusb|pcie|embedded), ep-iface-number) to be used with `--client-no-release-cid'",
      "[\"key=value,...\"]"
    },
#endif
#if defined HAVE_QMI_MESSAGE_WDS_SET_IP_FAMILY
    { "wds-set-ip-family", 0, 0, G_OPTION_ARG_STRING, &set_ip_family_str,
      "Set IP family",
      "[4|6]"
    },
#endif
#if defined HAVE_QMI_MESSAGE_WDS_GET_CHANNEL_RATES
    { "wds-get-channel-rates", 0, 0, G_OPTION_ARG_NONE, &get_channel_rates_flag,
      "Get channel data rates",
      NULL
    },
#endif
#if defined HAVE_QMI_MESSAGE_WDS_GET_LTE_ATTACH_PARAMETERS
    { "wds-get-lte-attach-parameters", 0, 0, G_OPTION_ARG_NONE, &get_lte_attach_parameters_flag,
      "Get LTE attach parameters",
      NULL
    },
#endif
#if defined HAVE_QMI_MESSAGE_WDS_GET_MAX_LTE_ATTACH_PDN_NUMBER
    { "wds-get-max-lte-attach-pdn-num", 0, 0, G_OPTION_ARG_NONE, &get_max_lte_attach_pdn_number_flag,
      "Get the maximum number of LTE attach PDN",
      NULL
    },
#endif
#if defined HAVE_QMI_MESSAGE_WDS_GET_LTE_ATTACH_PDN_LIST
    { "wds-get-lte-attach-pdn-list", 0, 0, G_OPTION_ARG_NONE, &get_lte_attach_pdn_list_flag,
      "Get the list of LTE attach PDN",
      NULL
    },
#endif
#if defined HAVE_QMI_MESSAGE_WDS_SET_LTE_ATTACH_PDN_LIST
    { "wds-set-lte-attach-pdn-list", 0, 0, G_OPTION_ARG_STRING, &set_lte_attach_pdn_list_str,
      "Set the list of LTE attach PDN",
      "[#,#,...]"
    },
#endif
    { "wds-noop", 0, 0, G_OPTION_ARG_NONE, &noop_flag,
      "Just allocate or release a WDS client. Use with `--client-no-release-cid' and/or `--client-cid'",
      NULL
    },
    /* deprecated entries (hidden in --help) */
#if defined HAVE_QMI_MESSAGE_WDS_GET_DEFAULT_PROFILE_NUMBER
    { "wds-get-default-profile-num", 0, G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_STRING, &get_default_profile_num_str,
      "Get default profile number",
      "[3gpp|3gpp2]"
    },
#endif
#if defined HAVE_QMI_MESSAGE_WDS_SET_DEFAULT_PROFILE_NUMBER
    { "wds-set-default-profile-num", 0, G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_STRING, &set_default_profile_num_str,
      "Set default profile number",
      "[(3gpp|3gpp2),#]"
    },
#endif
    { NULL }
};

GOptionGroup *
qmicli_wds_get_option_group (void)
{
    GOptionGroup *group;

    group = g_option_group_new ("wds",
                                "WDS options:",
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

    n_actions = (!!start_network_str +
                 !!stop_network_str +
                 !!bind_data_port_str +
                 !!bind_mux_str +
                 !!set_ip_family_str +
                 get_current_settings_flag +
                 get_packet_service_status_flag +
                 get_packet_statistics_flag +
                 get_data_bearer_technology_flag +
                 get_current_data_bearer_technology_flag +
                 go_dormant_flag +
                 go_active_flag +
                 get_dormancy_status_flag +
                 !!create_profile_str +
                 !!swi_create_profile_indexed_str +
                 !!modify_profile_str +
                 !!delete_profile_str +
                 !!get_profile_list_str +
                 !!get_default_profile_num_str +
                 !!get_default_profile_number_str +
                 !!set_default_profile_num_str +
                 !!set_default_profile_number_str +
                 !!get_default_settings_str +
                 get_autoconnect_settings_flag +
                 !!set_autoconnect_settings_str +
                 get_supported_messages_flag +
                 reset_flag +
                 !!get_channel_rates_flag +
                 get_lte_attach_parameters_flag +
                 get_max_lte_attach_pdn_number_flag +
                 get_lte_attach_pdn_list_flag +
                 !!set_lte_attach_pdn_list_str +
                 noop_flag);

    if (n_actions > 1) {
        g_printerr ("error: too many WDS actions requested\n");
        exit (EXIT_FAILURE);
    } else if (n_actions == 0 &&
               follow_network_flag) {
        g_printerr ("error: `--wds-follow-network' must be used with `--wds-start-network'\n");
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
    if (context->network_started_id)
        g_cancellable_disconnect (context->cancellable, context->network_started_id);
    if (context->packet_status_timeout_id)
        g_source_remove (context->packet_status_timeout_id);
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

#if defined HAVE_QMI_MESSAGE_WDS_START_NETWORK || defined HAVE_QMI_MESSAGE_WDS_STOP_NETWORK

static void
stop_network_ready (QmiClientWds *client,
                    GAsyncResult *res)
{
    GError *error = NULL;
    QmiMessageWdsStopNetworkOutput *output;

    output = qmi_client_wds_stop_network_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n",
                    error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_wds_stop_network_output_get_result (output, &error)) {
        g_printerr ("error: couldn't stop network: %s\n", error->message);
        g_error_free (error);
        qmi_message_wds_stop_network_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

#undef VALIDATE_UNKNOWN
#define VALIDATE_UNKNOWN(str) (str ? str : "unknown")

    g_print ("[%s] Network stopped\n",
             qmi_device_get_path_display (ctx->device));
    qmi_message_wds_stop_network_output_unref (output);
    operation_shutdown (TRUE);
}

static void
internal_stop_network (GCancellable *cancellable,
                       guint32 packet_data_handle,
                       gboolean disable_autoconnect)
{
    QmiMessageWdsStopNetworkInput *input;

    input = qmi_message_wds_stop_network_input_new ();
    qmi_message_wds_stop_network_input_set_packet_data_handle (input, packet_data_handle, NULL);
    if (disable_autoconnect)
        qmi_message_wds_stop_network_input_set_disable_autoconnect (input, TRUE, NULL);

    g_print ("Network cancelled... releasing resources\n");
    qmi_client_wds_stop_network (ctx->client,
                                 input,
                                 120,
                                 ctx->cancellable,
                                 (GAsyncReadyCallback)stop_network_ready,
                                 NULL);
    qmi_message_wds_stop_network_input_unref (input);
}

#endif /* HAVE_QMI_MESSAGE_WDS_START_NETWORK
        * HAVE_QMI_MESSAGE_WDS_STOP_NETWORK */

#if defined HAVE_QMI_MESSAGE_WDS_START_NETWORK

#if defined HAVE_QMI_MESSAGE_WDS_STOP_NETWORK && defined HAVE_QMI_MESSAGE_WDS_GET_PACKET_SERVICE_STATUS

static void
network_cancelled (GCancellable *cancellable)
{
    ctx->network_started_id = 0;

    /* Remove the timeout right away */
    if (ctx->packet_status_timeout_id) {
        g_source_remove (ctx->packet_status_timeout_id);
        ctx->packet_status_timeout_id = 0;
    }

    g_print ("Network cancelled... releasing resources\n");
    internal_stop_network (cancellable, ctx->packet_data_handle, FALSE);
}

static void
timeout_get_packet_service_status_ready (QmiClientWds *client,
                                         GAsyncResult *res)
{
    GError *error = NULL;
    QmiMessageWdsGetPacketServiceStatusOutput *output;
    QmiWdsConnectionStatus status;

    output = qmi_client_wds_get_packet_service_status_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n",
                    error->message);
        g_error_free (error);
        return;
    }

    if (!qmi_message_wds_get_packet_service_status_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get packet service status: %s\n", error->message);
        g_error_free (error);
        qmi_message_wds_get_packet_service_status_output_unref (output);
        return;
    }

    qmi_message_wds_get_packet_service_status_output_get_connection_status (
        output,
        &status,
        NULL);

    g_print ("[%s] Connection status: '%s'\n",
             qmi_device_get_path_display (ctx->device),
             qmi_wds_connection_status_get_string (status));
    qmi_message_wds_get_packet_service_status_output_unref (output);

    /* If packet service checks detect disconnection, halt --wds-follow-network */
    if (status != QMI_WDS_CONNECTION_STATUS_CONNECTED) {
        g_print ("[%s] Stopping after detecting disconnection\n",
                 qmi_device_get_path_display (ctx->device));
        internal_stop_network (NULL, ctx->packet_data_handle, FALSE);
    }
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

#endif /* HAVE_QMI_MESSAGE_WDS_STOP_NETWORK
        * HAVE_QMI_MESSAGE_WDS_GET_PACKET_SERVICE_STATUS */

typedef struct {
    gchar                *apn;
    guint8                profile_index_3gpp;
    guint8                profile_index_3gpp2;
    QmiWdsAuthentication  auth;
    gboolean              auth_set;
    QmiWdsIpFamily        ip_type;
    gchar                *username;
    gchar                *password;
    gboolean              autoconnect;
    gboolean              autoconnect_set;
} StartNetworkProperties;

static gboolean
start_network_properties_handle (const gchar  *key,
                                 const gchar  *value,
                                 GError      **error,
                                 gpointer      user_data)
{
    StartNetworkProperties *props = user_data;

    if (!value || !value[0]) {
        g_set_error (error,
                     QMI_CORE_ERROR,
                     QMI_CORE_ERROR_FAILED,
                     "key '%s' required a value",
                     key);
        return FALSE;
    }

    if (g_ascii_strcasecmp (key, "apn") == 0 && !props->apn) {
        props->apn = g_strdup (value);
        return TRUE;
    }

    if (g_ascii_strcasecmp (key, "3gpp-profile") == 0 && !props->profile_index_3gpp) {
        props->profile_index_3gpp = atoi (value);
        return TRUE;
    }

    if (g_ascii_strcasecmp (key, "3gpp2-profile") == 0 && !props->profile_index_3gpp2) {
        props->profile_index_3gpp2 = atoi (value);
        return TRUE;
    }

    if (g_ascii_strcasecmp (key, "auth") == 0 && !props->auth_set) {
        if (!qmicli_read_authentication_from_string (value, &(props->auth))) {
            g_set_error (error,
                         QMI_CORE_ERROR,
                         QMI_CORE_ERROR_FAILED,
                         "unknown auth protocol '%s'",
                         value);
            return FALSE;
        }
        props->auth_set = TRUE;
        return TRUE;
    }

    if (g_ascii_strcasecmp (key, "username") == 0 && !props->username) {
        props->username = g_strdup (value);
        return TRUE;
    }

    if (g_ascii_strcasecmp (key, "password") == 0 && !props->password) {
        props->password = g_strdup (value);
        return TRUE;
    }

    if (g_ascii_strcasecmp (key, "autoconnect") == 0 && !props->autoconnect_set) {
        if (!qmicli_read_yes_no_from_string (value, &(props->autoconnect))) {
            g_set_error (error,
                         QMI_CORE_ERROR,
                         QMI_CORE_ERROR_FAILED,
                         "unknown autoconnect setup '%s'",
                         value);
            return FALSE;
        }
        props->autoconnect_set = TRUE;
        return TRUE;
    }

    if (g_ascii_strcasecmp (key, "ip-type") == 0 && props->ip_type == QMI_WDS_IP_FAMILY_UNSPECIFIED) {
        switch (atoi (value)) {
        case 4:
            props->ip_type = QMI_WDS_IP_FAMILY_IPV4;
            break;
        case 6:
            props->ip_type = QMI_WDS_IP_FAMILY_IPV6;
            break;
        default:
            g_set_error (error,
                         QMI_CORE_ERROR,
                         QMI_CORE_ERROR_FAILED,
                         "unknown IP type '%s' (not 4 or 6)",
                         value);
            return FALSE;
        }
        return TRUE;
    }

    g_set_error (error,
                 QMI_CORE_ERROR,
                 QMI_CORE_ERROR_FAILED,
                 "unrecognized or duplicate option '%s'",
                 key);
    return FALSE;
}

static gboolean
start_network_input_create (const gchar                     *str,
                            QmiMessageWdsStartNetworkInput **input,
                            GError                         **error)
{
    g_autofree gchar *aux_auth_str = NULL;
    const gchar *ip_type_str = NULL;
    gchar **split = NULL;
    StartNetworkProperties props = {
        .auth    = QMI_WDS_AUTHENTICATION_NONE,
        .ip_type = QMI_WDS_IP_FAMILY_UNSPECIFIED,
    };
    gboolean success = FALSE;

    g_assert (input && !*input);

    /* An empty string is totally valid (i.e. no TLVs) */
    if (!str[0])
        return TRUE;

    /* New key=value format */
    if (strchr (str, '=')) {
        GError *parse_error = NULL;

        if (!qmicli_parse_key_value_string (str,
                                            &parse_error,
                                            start_network_properties_handle,
                                            &props)) {
            g_set_error (error,
                         QMI_CORE_ERROR,
                         QMI_CORE_ERROR_FAILED,
                         "couldn't parse input string: %s",
                         parse_error->message);
            g_error_free (parse_error);
            goto out;
        }
    }
    /* Old non key=value format, like this:
     *    "[(APN),(PAP|CHAP|BOTH),(Username),(Password)]"
     */
    else {
        /* Parse input string into the expected fields */
        split = g_strsplit (str, ",", 0);

        props.apn = g_strdup (split[0]);

        if (props.apn && split[1]) {
            if (!qmicli_read_authentication_from_string (split[1], &(props.auth))) {
                g_set_error (error,
                             QMI_CORE_ERROR,
                             QMI_CORE_ERROR_FAILED,
                             "unknown auth protocol '%s'",
                             split[1]);
                goto out;
            }
            props.auth_set = TRUE;
        }

        props.username = (props.auth_set ? g_strdup (split[2]) : NULL);
        props.password = (props.username ? g_strdup (split[3]) : NULL);
    }

    /* Create input bundle */
    *input = qmi_message_wds_start_network_input_new ();

    /* Set APN */
    if (props.apn)
        qmi_message_wds_start_network_input_set_apn (*input, props.apn, NULL);

    /* Set 3GPP profile */
    if (props.profile_index_3gpp > 0)
        qmi_message_wds_start_network_input_set_profile_index_3gpp (*input, props.profile_index_3gpp, NULL);

    /* Set 3GPP2 profile */
    if (props.profile_index_3gpp2 > 0)
        qmi_message_wds_start_network_input_set_profile_index_3gpp2 (*input, props.profile_index_3gpp2, NULL);

    /* Set IP Type */
    if (props.ip_type != 0) {
        qmi_message_wds_start_network_input_set_ip_family_preference (*input, props.ip_type, NULL);
        if (props.ip_type == QMI_WDS_IP_FAMILY_IPV4)
            ip_type_str = "4";
        else if (props.ip_type == QMI_WDS_IP_FAMILY_IPV6)
            ip_type_str = "6";
    }

    /* Set authentication method */
    if (props.auth_set) {
        aux_auth_str = qmi_wds_authentication_build_string_from_mask (props.auth);
        qmi_message_wds_start_network_input_set_authentication_preference (*input, props.auth, NULL);
    }

    /* Set username, avoid empty strings */
    if (props.username && props.username[0])
        qmi_message_wds_start_network_input_set_username (*input, props.username, NULL);

    /* Set password, avoid empty strings */
    if (props.password && props.password[0])
        qmi_message_wds_start_network_input_set_password (*input, props.password, NULL);

    /* Set autoconnect */
    if (props.autoconnect_set)
        qmi_message_wds_start_network_input_set_enable_autoconnect (*input, props.autoconnect, NULL);

    success = TRUE;

    g_debug ("Network start parameters set (apn: '%s', 3gpp_profile: '%u', 3gpp2_profile: '%u', auth: '%s', ip-type: '%s', username: '%s', password: '%s', autoconnect: '%s')",
             props.apn                             ? props.apn                          : "unspecified",
             props.profile_index_3gpp,
             props.profile_index_3gpp2,
             aux_auth_str                          ? aux_auth_str                       : "unspecified",
             ip_type_str                           ? ip_type_str                        : "unspecified",
             (props.username && props.username[0]) ? props.username                     : "unspecified",
             (props.password && props.password[0]) ? props.password                     : "unspecified",
             props.autoconnect_set                 ? (props.autoconnect ? "yes" : "no") : "unspecified");

out:
    g_strfreev (split);
    g_free     (props.apn);
    g_free     (props.username);
    g_free     (props.password);

    return success;
}

static void
start_network_ready (QmiClientWds *client,
                     GAsyncResult *res)
{
    GError *error = NULL;
    QmiMessageWdsStartNetworkOutput *output;

    output = qmi_client_wds_start_network_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n",
                    error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_wds_start_network_output_get_result (output, &error)) {
        g_printerr ("error: couldn't start network: %s\n", error->message);
        if (g_error_matches (error,
                             QMI_PROTOCOL_ERROR,
                             QMI_PROTOCOL_ERROR_CALL_FAILED)) {
            QmiWdsCallEndReason cer;
            QmiWdsVerboseCallEndReasonType verbose_cer_type;
            gint16 verbose_cer_reason;

            if (qmi_message_wds_start_network_output_get_call_end_reason (
                    output,
                    &cer,
                    NULL))
                g_printerr ("call end reason (%u): %s\n",
                            cer,
                            qmi_wds_call_end_reason_get_string (cer));

            if (qmi_message_wds_start_network_output_get_verbose_call_end_reason (
                    output,
                    &verbose_cer_type,
                    &verbose_cer_reason,
                    NULL))
                g_printerr ("verbose call end reason (%u,%d): [%s] %s\n",
                            verbose_cer_type,
                            verbose_cer_reason,
                            qmi_wds_verbose_call_end_reason_type_get_string (verbose_cer_type),
                            qmi_wds_verbose_call_end_reason_get_string (verbose_cer_type, verbose_cer_reason));
        }

        g_error_free (error);
        qmi_message_wds_start_network_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    qmi_message_wds_start_network_output_get_packet_data_handle (output, &ctx->packet_data_handle, NULL);
    qmi_message_wds_start_network_output_unref (output);

#undef VALIDATE_UNKNOWN
#define VALIDATE_UNKNOWN(str) (str ? str : "unknown")

    g_print ("[%s] Network started\n"
             "\tPacket data handle: '%u'\n",
             qmi_device_get_path_display (ctx->device),
             (guint)ctx->packet_data_handle);

#if defined HAVE_QMI_MESSAGE_WDS_STOP_NETWORK && defined HAVE_QMI_MESSAGE_WDS_GET_PACKET_SERVICE_STATUS
    if (follow_network_flag) {
        g_print ("\nCtrl+C will stop the network\n");
        ctx->network_started_id = g_cancellable_connect (ctx->cancellable,
                                                         G_CALLBACK (network_cancelled),
                                                         NULL,
                                                         NULL);

        ctx->packet_status_timeout_id = g_timeout_add_seconds (20,
                                                               (GSourceFunc)packet_status_timeout,
                                                               NULL);
        return;
    }
#endif

    /* Nothing else to do */
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_WDS_START_NETWORK */

#if defined HAVE_QMI_MESSAGE_WDS_GET_CURRENT_SETTINGS

static void
get_current_settings_ready (QmiClientWds *client,
                            GAsyncResult *res)
{
    GError *error = NULL;
    QmiMessageWdsGetCurrentSettingsOutput *output;
    QmiWdsIpFamily ip_family = QMI_WDS_IP_FAMILY_UNSPECIFIED;
    guint32 mtu = 0;
    GArray *array;
    guint32 addr = 0;
    struct in_addr in_addr_val;
    struct in6_addr in6_addr_val;
    gchar buf4[INET_ADDRSTRLEN];
    gchar buf6[INET6_ADDRSTRLEN];
    guint8 prefix = 0;
    guint i;

    output = qmi_client_wds_get_current_settings_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_wds_get_current_settings_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get current settings: %s\n", error->message);
        g_error_free (error);
        qmi_message_wds_get_current_settings_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] Current settings retrieved:\n",
             qmi_device_get_path_display (ctx->device));

    if (qmi_message_wds_get_current_settings_output_get_ip_family (output, &ip_family, NULL))
        g_print ("           IP Family: %s\n",
                 ((ip_family == QMI_WDS_IP_FAMILY_IPV4) ? "IPv4" :
                  ((ip_family == QMI_WDS_IP_FAMILY_IPV6) ? "IPv6" :
                   "unknown")));

    /* IPv4... */

    if (qmi_message_wds_get_current_settings_output_get_ipv4_address (output, &addr, NULL)) {
        in_addr_val.s_addr = GUINT32_TO_BE (addr);
        memset (buf4, 0, sizeof (buf4));
        inet_ntop (AF_INET, &in_addr_val, buf4, sizeof (buf4));
        g_print ("        IPv4 address: %s\n", buf4);
    }

    if (qmi_message_wds_get_current_settings_output_get_ipv4_gateway_subnet_mask (output, &addr, NULL)) {
        in_addr_val.s_addr = GUINT32_TO_BE (addr);
        memset (buf4, 0, sizeof (buf4));
        inet_ntop (AF_INET, &in_addr_val, buf4, sizeof (buf4));
        g_print ("    IPv4 subnet mask: %s\n", buf4);
    }

    if (qmi_message_wds_get_current_settings_output_get_ipv4_gateway_address (output, &addr, NULL)) {
        in_addr_val.s_addr = GUINT32_TO_BE (addr);
        memset (buf4, 0, sizeof (buf4));
        inet_ntop (AF_INET, &in_addr_val, buf4, sizeof (buf4));
        g_print ("IPv4 gateway address: %s\n", buf4);
    }

    if (qmi_message_wds_get_current_settings_output_get_primary_ipv4_dns_address (output, &addr, NULL)) {
        in_addr_val.s_addr = GUINT32_TO_BE (addr);
        memset (buf4, 0, sizeof (buf4));
        inet_ntop (AF_INET, &in_addr_val, buf4, sizeof (buf4));
        g_print ("    IPv4 primary DNS: %s\n", buf4);
    }

    if (qmi_message_wds_get_current_settings_output_get_secondary_ipv4_dns_address (output, &addr, NULL)) {
        in_addr_val.s_addr = GUINT32_TO_BE (addr);
        memset (buf4, 0, sizeof (buf4));
        inet_ntop (AF_INET, &in_addr_val, buf4, sizeof (buf4));
        g_print ("  IPv4 secondary DNS: %s\n", buf4);
    }

    /* IPv6... */

    if (qmi_message_wds_get_current_settings_output_get_ipv6_address (output, &array, &prefix, NULL)) {
        for (i = 0; i < array->len; i++)
            in6_addr_val.s6_addr16[i] = GUINT16_TO_BE (g_array_index (array, guint16, i));
        memset (buf6, 0, sizeof (buf6));
        inet_ntop (AF_INET6, &in6_addr_val, buf6, sizeof (buf6));
        g_print ("        IPv6 address: %s/%d\n", buf6, prefix);
    }

    if (qmi_message_wds_get_current_settings_output_get_ipv6_gateway_address (output, &array, &prefix, NULL)) {
        for (i = 0; i < array->len; i++)
            in6_addr_val.s6_addr16[i] = GUINT16_TO_BE (g_array_index (array, guint16, i));
        memset (buf6, 0, sizeof (buf6));
        inet_ntop (AF_INET6, &in6_addr_val, buf6, sizeof (buf6));
        g_print ("IPv6 gateway address: %s/%d\n", buf6, prefix);
    }

    if (qmi_message_wds_get_current_settings_output_get_ipv6_primary_dns_address (output, &array, NULL)) {
        for (i = 0; i < array->len; i++)
            in6_addr_val.s6_addr16[i] = GUINT16_TO_BE (g_array_index (array, guint16, i));
        memset (buf6, 0, sizeof (buf6));
        inet_ntop (AF_INET6, &in6_addr_val, buf6, sizeof (buf6));
        g_print ("    IPv6 primary DNS: %s\n", buf6);
    }

    if (qmi_message_wds_get_current_settings_output_get_ipv6_secondary_dns_address (output, &array, NULL)) {
        for (i = 0; i < array->len; i++)
            in6_addr_val.s6_addr16[i] = GUINT16_TO_BE (g_array_index (array, guint16, i));
        memset (buf6, 0, sizeof (buf6));
        inet_ntop (AF_INET6, &in6_addr_val, buf6, sizeof (buf6));
        g_print ("  IPv6 secondary DNS: %s\n", buf6);
    }

    /* Other... */

    if (qmi_message_wds_get_current_settings_output_get_mtu (output, &mtu, NULL))
        g_print ("                 MTU: %u\n", mtu);

    if (qmi_message_wds_get_current_settings_output_get_domain_name_list (output, &array, &error)) {
        GString *s = NULL;

        if (array) {
            for (i = 0; i < array->len; i++) {
                if (!s)
                    s = g_string_new ("");
                else
                    g_string_append (s, ", ");
                g_string_append (s, g_array_index (array, const gchar *, i));
            }
        }
        if (s) {
            g_print ("             Domains: %s\n", s->str);
            g_string_free (s, TRUE);
        } else
            g_print ("             Domains: none\n");
    }

    qmi_message_wds_get_current_settings_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_WDS_GET_CURRENT_SETTINGS */

#if defined HAVE_QMI_MESSAGE_WDS_GET_PACKET_SERVICE_STATUS

static void
get_packet_service_status_ready (QmiClientWds *client,
                                 GAsyncResult *res)
{
    GError *error = NULL;
    QmiMessageWdsGetPacketServiceStatusOutput *output;
    QmiWdsConnectionStatus status;

    output = qmi_client_wds_get_packet_service_status_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n",
                    error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_wds_get_packet_service_status_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get packet service status: %s\n", error->message);
        g_error_free (error);
        qmi_message_wds_get_packet_service_status_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    qmi_message_wds_get_packet_service_status_output_get_connection_status (
        output,
        &status,
        NULL);

    g_print ("[%s] Connection status: '%s'\n",
             qmi_device_get_path_display (ctx->device),
             qmi_wds_connection_status_get_string (status));

    qmi_message_wds_get_packet_service_status_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_WDS_GET_PACKET_SERVICE_STATUS */

#if defined HAVE_QMI_MESSAGE_WDS_GET_PACKET_STATISTICS

static void
get_packet_statistics_ready (QmiClientWds *client,
                             GAsyncResult *res)
{
    GError *error = NULL;
    QmiMessageWdsGetPacketStatisticsOutput *output;
    guint32 val32;
    guint64 val64;

    output = qmi_client_wds_get_packet_statistics_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n",
                    error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_wds_get_packet_statistics_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get packet statistics: %s\n", error->message);
        g_error_free (error);
        qmi_message_wds_get_packet_statistics_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] Connection statistics:\n",
             qmi_device_get_path_display (ctx->device));

    if (qmi_message_wds_get_packet_statistics_output_get_tx_packets_ok (output, &val32, NULL) &&
        val32 != 0xFFFFFFFF)
        g_print ("\tTX packets OK: %u\n", val32);
    if (qmi_message_wds_get_packet_statistics_output_get_rx_packets_ok (output, &val32, NULL) &&
        val32 != 0xFFFFFFFF)
        g_print ("\tRX packets OK: %u\n", val32);
    if (qmi_message_wds_get_packet_statistics_output_get_tx_packets_error (output, &val32, NULL) &&
        val32 != 0xFFFFFFFF)
        g_print ("\tTX packets error: %u\n", val32);
    if (qmi_message_wds_get_packet_statistics_output_get_rx_packets_error (output, &val32, NULL) &&
        val32 != 0xFFFFFFFF)
        g_print ("\tRX packets error: %u\n", val32);
    if (qmi_message_wds_get_packet_statistics_output_get_tx_overflows (output, &val32, NULL) &&
        val32 != 0xFFFFFFFF)
        g_print ("\tTX overflows: %u\n", val32);
    if (qmi_message_wds_get_packet_statistics_output_get_rx_overflows (output, &val32, NULL) &&
        val32 != 0xFFFFFFFF)
        g_print ("\tRX overflows: %u\n", val32);
    if (qmi_message_wds_get_packet_statistics_output_get_tx_packets_dropped (output, &val32, NULL) &&
        val32 != 0xFFFFFFFF)
        g_print ("\tTX packets dropped: %u\n", val32);
    if (qmi_message_wds_get_packet_statistics_output_get_rx_packets_dropped (output, &val32, NULL) &&
        val32 != 0xFFFFFFFF)
        g_print ("\tRX packets dropped: %u\n", val32);

    if (qmi_message_wds_get_packet_statistics_output_get_tx_bytes_ok (output, &val64, NULL))
        g_print ("\tTX bytes OK: %" G_GUINT64_FORMAT "\n", val64);
    if (qmi_message_wds_get_packet_statistics_output_get_rx_bytes_ok (output, &val64, NULL))
        g_print ("\tRX bytes OK: %" G_GUINT64_FORMAT "\n", val64);
    if (qmi_message_wds_get_packet_statistics_output_get_last_call_tx_bytes_ok (output, &val64, NULL))
        g_print ("\tTX bytes OK (last): %" G_GUINT64_FORMAT "\n", val64);
    if (qmi_message_wds_get_packet_statistics_output_get_last_call_rx_bytes_ok (output, &val64, NULL))
        g_print ("\tRX bytes OK (last): %" G_GUINT64_FORMAT "\n", val64);

    qmi_message_wds_get_packet_statistics_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_WDS_GET_PACKET_STATISTICS */

#if defined HAVE_QMI_MESSAGE_WDS_GET_DATA_BEARER_TECHNOLOGY

static void
get_data_bearer_technology_ready (QmiClientWds *client,
                                  GAsyncResult *res)
{
    GError *error = NULL;
    QmiMessageWdsGetDataBearerTechnologyOutput *output;
    QmiWdsDataBearerTechnology current;

    output = qmi_client_wds_get_data_bearer_technology_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n",
                    error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_wds_get_data_bearer_technology_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get data bearer technology: %s\n", error->message);

        if (g_error_matches (error,
                             QMI_PROTOCOL_ERROR,
                             QMI_PROTOCOL_ERROR_OUT_OF_CALL)) {
            QmiWdsDataBearerTechnology last = QMI_WDS_DATA_BEARER_TECHNOLOGY_UNKNOWN;

            if (qmi_message_wds_get_data_bearer_technology_output_get_last (
                    output,
                    &last,
                    NULL))
                g_print ("[%s] Data bearer technology (last): '%s'(%d)\n",
                         qmi_device_get_path_display (ctx->device),
                         qmi_wds_data_bearer_technology_get_string (last), last);
        }

        g_error_free (error);
        qmi_message_wds_get_data_bearer_technology_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    qmi_message_wds_get_data_bearer_technology_output_get_current (
        output,
        &current,
        NULL);
    g_print ("[%s] Data bearer technology (current): '%s'\n",
             qmi_device_get_path_display (ctx->device),
             qmi_wds_data_bearer_technology_get_string (current));
    qmi_message_wds_get_data_bearer_technology_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_WDS_GET_DATA_BEARER_TECHNOLOGY */

#if defined HAVE_QMI_MESSAGE_WDS_GET_CURRENT_DATA_BEARER_TECHNOLOGY

static void
print_current_data_bearer_technology_results (const gchar *which,
                                              QmiWdsNetworkType network_type,
                                              guint32 rat_mask,
                                              guint32 so_mask)
{
    g_autofree gchar *rat_string = NULL;
    g_autofree gchar *so_string = NULL;

    if (network_type == QMI_WDS_NETWORK_TYPE_3GPP2) {
        rat_string = qmi_wds_rat_3gpp2_build_string_from_mask (rat_mask);
        if (rat_mask & QMI_WDS_RAT_3GPP2_CDMA1X)
            so_string = qmi_wds_so_cdma1x_build_string_from_mask (so_mask);
        else if (rat_mask & QMI_WDS_RAT_3GPP2_EVDO_REVA)
            so_string = qmi_wds_so_evdo_reva_build_string_from_mask (so_mask);
    } else if (network_type == QMI_WDS_NETWORK_TYPE_3GPP) {
        rat_string = qmi_wds_rat_3gpp_build_string_from_mask (rat_mask);
    }

    g_print ("[%s] Data bearer technology (%s):\n"
             "              Network type: '%s'\n"
             "   Radio Access Technology: '%s'\n",
             qmi_device_get_path_display (ctx->device),
             which,
             qmi_wds_network_type_get_string (network_type),
             VALIDATE_MASK_NONE (rat_string));

    if (network_type == QMI_WDS_NETWORK_TYPE_3GPP2)
        g_print ("            Service Option: '%s'\n",
                 VALIDATE_MASK_NONE (so_string));
}

static void
get_current_data_bearer_technology_ready (QmiClientWds *client,
                                          GAsyncResult *res)
{
    GError *error = NULL;
    QmiMessageWdsGetCurrentDataBearerTechnologyOutput *output;
    QmiWdsNetworkType network_type;
    guint32 rat_mask;
    guint32 so_mask;

    output = qmi_client_wds_get_current_data_bearer_technology_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n",
                    error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

#undef VALIDATE_UNKNOWN
#define VALIDATE_UNKNOWN(str) (str ? str : "unknown")

    if (!qmi_message_wds_get_current_data_bearer_technology_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get current data bearer technology: %s\n", error->message);

        if (qmi_message_wds_get_current_data_bearer_technology_output_get_last (
                output,
                &network_type,
                &rat_mask,
                &so_mask,
                NULL)) {
            print_current_data_bearer_technology_results (
                "last",
                network_type,
                rat_mask,
                so_mask);
        }

        g_error_free (error);
        qmi_message_wds_get_current_data_bearer_technology_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    /* Retrieve CURRENT */
    if (qmi_message_wds_get_current_data_bearer_technology_output_get_current (
            output,
            &network_type,
            &rat_mask,
            &so_mask,
            NULL)) {
        print_current_data_bearer_technology_results (
            "current",
            network_type,
            rat_mask,
            so_mask);
    }

    qmi_message_wds_get_current_data_bearer_technology_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_WDS_GET_CURRENT_DATA_BEARER_TECHNOLOGY */

#if defined HAVE_QMI_MESSAGE_WDS_GO_DORMANT

static void
go_dormant_ready (QmiClientWds *client,
                  GAsyncResult *res)
{
    GError *error = NULL;
    QmiMessageWdsGoDormantOutput *output;

    output = qmi_client_wds_go_dormant_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n",
                    error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_wds_go_dormant_output_get_result (output, &error)) {
        g_printerr ("error: couldn't go dormant: %s\n", error->message);
        g_error_free (error);
        qmi_message_wds_go_dormant_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    qmi_message_wds_go_dormant_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_WDS_GO_DORMANT */

#if defined HAVE_QMI_MESSAGE_WDS_GO_ACTIVE

static void
go_active_ready (QmiClientWds *client,
                 GAsyncResult *res)
{
    GError *error = NULL;
    QmiMessageWdsGoActiveOutput *output;

    output = qmi_client_wds_go_active_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n",
                    error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_wds_go_active_output_get_result (output, &error)) {
        g_printerr ("error: couldn't go active: %s\n", error->message);
        g_error_free (error);
        qmi_message_wds_go_active_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    qmi_message_wds_go_active_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_WDS_GO_ACTIVE */

#if defined HAVE_QMI_MESSAGE_WDS_GET_DORMANCY_STATUS

static void
get_dormancy_status_ready (QmiClientWds *client,
                           GAsyncResult *res)
{
    GError *error = NULL;
    QmiMessageWdsGetDormancyStatusOutput *output;
    QmiWdsDormancyStatus status;

    output = qmi_client_wds_get_dormancy_status_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n",
                    error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_wds_get_dormancy_status_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get dormancy status: %s\n", error->message);
        g_error_free (error);
        qmi_message_wds_get_dormancy_status_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    if (qmi_message_wds_get_dormancy_status_output_get_dormancy_status (
            output,
            &status,
            NULL)) {
        g_print ("[%s] Dormancy Status: '%s'\n",
                 qmi_device_get_path_display (ctx->device),
                 qmi_wds_dormancy_status_get_string (status));
    }

    qmi_message_wds_get_dormancy_status_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_WDS_GET_DORMANCY_STATUS */

#if defined HAVE_QMI_MESSAGE_WDS_CREATE_PROFILE || \
    defined HAVE_QMI_MESSAGE_WDS_SWI_CREATE_PROFILE_INDEXED || \
    defined HAVE_QMI_MESSAGE_WDS_MODIFY_PROFILE

typedef struct {
    QmiWdsProfileType     profile_type;
    guint                 profile_index;
    guint                 context_number;
    gchar                *name;
    QmiWdsPdpType         pdp_type;
    gboolean              pdp_type_set;
    QmiWdsApnTypeMask     apn_type;
    gboolean              apn_type_set;
    gchar                *apn;
    gchar                *username;
    gchar                *password;
    QmiWdsAuthentication  auth;
    gboolean              auth_set;
    gboolean              no_roaming;
    gboolean              no_roaming_set;
    gboolean              disabled;
    gboolean              disabled_set;
} CreateModifyProfileProperties;

static gboolean
create_modify_profile_properties_handle (const gchar  *key,
                                         const gchar  *value,
                                         GError      **error,
                                         gpointer      user_data)
{
    CreateModifyProfileProperties *props = user_data;

    /* Allow empty values for string parameters */

    if (g_ascii_strcasecmp (key, "name") == 0 && !props->name) {
        props->name = g_strdup (value);
        return TRUE;
    }

    if (g_ascii_strcasecmp (key, "apn") == 0 && !props->apn) {
        props->apn = g_strdup (value);
        return TRUE;
    }

    if (g_ascii_strcasecmp (key, "username") == 0 && !props->username) {
        props->username = g_strdup (value);
        return TRUE;
    }

    if (g_ascii_strcasecmp (key, "password") == 0 && !props->password) {
        props->password = g_strdup (value);
        return TRUE;
    }

    /* all other TLVs do require a value... */
    if (!value || !value[0]) {
        g_set_error (error,
                     QMI_CORE_ERROR,
                     QMI_CORE_ERROR_FAILED,
                     "key '%s' required a value",
                     key);
        return FALSE;
    }

    if (g_ascii_strcasecmp (key, "context-num") == 0 && !props->context_number) {
        props->context_number = atoi (value);
        if (props->context_number <= 0 || props->context_number > G_MAXUINT8) {
            g_set_error (error,
                         QMI_CORE_ERROR,
                         QMI_CORE_ERROR_FAILED,
                         "invalid or out of range context number [1,%u]: '%s'",
                         G_MAXUINT8,
                         value);
            return FALSE;
        }
        return TRUE;
    }

    if (g_ascii_strcasecmp (key, "auth") == 0 && !props->auth_set) {
        if (!qmicli_read_authentication_from_string (value, &(props->auth))) {
            g_set_error (error,
                         QMI_CORE_ERROR,
                         QMI_CORE_ERROR_FAILED,
                         "unknown auth protocol '%s'",
                         value);
            return FALSE;
        }
        props->auth_set = TRUE;
        return TRUE;
    }

    if (g_ascii_strcasecmp (key, "pdp-type") == 0 && !props->pdp_type_set) {
        if (!qmicli_read_pdp_type_from_string (value, &(props->pdp_type))) {
            g_set_error (error,
                         QMI_CORE_ERROR,
                         QMI_CORE_ERROR_FAILED,
                         "unknown pdp type '%s'",
                         value);
            return FALSE;
        }
        props->pdp_type_set = TRUE;
        return TRUE;
    }

    if (g_ascii_strcasecmp (key, "apn-type-mask") == 0 && !props->apn_type_set) {
        if (!qmicli_read_wds_apn_type_mask_from_string (value, &(props->apn_type))) {
            g_set_error (error,
                         QMI_CORE_ERROR,
                         QMI_CORE_ERROR_FAILED,
                         "unknown apn type '%s'",
                         value);
            return FALSE;
        }
        props->apn_type_set = TRUE;
        return TRUE;
    }

    if (g_ascii_strcasecmp (key, "no-roaming") == 0 && !props->no_roaming_set) {
        if (!qmicli_read_yes_no_from_string (value, &(props->no_roaming))) {
            g_set_error (error,
                         QMI_CORE_ERROR,
                         QMI_CORE_ERROR_FAILED,
                         "unknown 'no-roaming' value '%s'",
                         value);
            return FALSE;
        }
        props->no_roaming_set = TRUE;
        return TRUE;
    }

    if (g_ascii_strcasecmp (key, "disabled") == 0 && !props->disabled_set) {
        if (!qmicli_read_yes_no_from_string (value, &(props->disabled))) {
            g_set_error (error,
                         QMI_CORE_ERROR,
                         QMI_CORE_ERROR_FAILED,
                         "unknown 'disabled' value '%s'",
                         value);
            return FALSE;
        }
        props->disabled_set = TRUE;
        return TRUE;
    }

    g_set_error (error,
                 QMI_CORE_ERROR,
                 QMI_CORE_ERROR_FAILED,
                 "unrecognized or duplicate option '%s'",
                 key);
    return FALSE;
}

#endif /* HAVE_QMI_MESSAGE_WDS_CREATE_PROFILE
        * HAVE_QMI_MESSAGE_WDS_SWI_CREATE_PROFILE_INDEXED
        * HAVE_QMI_MESSAGE_WDS_MODIFY_PROFILE */

#if defined HAVE_QMI_MESSAGE_WDS_CREATE_PROFILE

static gboolean
create_profile_input_create (const gchar                      *str,
                             QmiMessageWdsCreateProfileInput **input,
                             GError                          **error)
{
    GError *parse_error = NULL;
    CreateModifyProfileProperties props = {};
    gboolean success = FALSE;
    gchar **split;

    g_assert (input && !*input);

    split = g_strsplit (str, ",", 2);
    if (g_strv_length (split) < 1) {
        g_set_error (error,
                     QMI_CORE_ERROR,
                     QMI_CORE_ERROR_FAILED,
                     "expected at least 1 arguments for 'wds create profile' command");
        goto out;
    }

    g_strstrip (split[0]);
    if (g_str_equal (split[0], "3gpp"))
        props.profile_type = QMI_WDS_PROFILE_TYPE_3GPP;
    else if (g_str_equal (split[0], "3gpp2"))
        props.profile_type = QMI_WDS_PROFILE_TYPE_3GPP2;
    else {
        g_set_error (error,
                     QMI_CORE_ERROR,
                     QMI_CORE_ERROR_FAILED,
                     "invalid profile type. Expected '3gpp' or '3gpp2'.'");
        goto out;
    }

    if (split[1]) {
        if (!qmicli_parse_key_value_string (split[1],
                                            &parse_error,
                                            create_modify_profile_properties_handle,
                                            &props)) {
            g_set_error (error,
                         QMI_CORE_ERROR,
                         QMI_CORE_ERROR_FAILED,
                         "couldn't parse input string: %s",
                         parse_error->message);
            g_error_free (parse_error);
            goto out;
        }
    }

    /* Create input bundle */
    *input = qmi_message_wds_create_profile_input_new ();

    /* Profile type is required */
    qmi_message_wds_create_profile_input_set_profile_type (*input, props.profile_type, NULL);

    if (props.context_number)
        qmi_message_wds_create_profile_input_set_pdp_context_number (*input, props.context_number, NULL);

    if (props.pdp_type_set)
        qmi_message_wds_create_profile_input_set_pdp_type (*input, props.pdp_type, NULL);

    if (props.apn_type_set)
        qmi_message_wds_create_profile_input_set_apn_type_mask (*input, props.apn_type, NULL);

    if (props.name)
        qmi_message_wds_create_profile_input_set_profile_name (*input, props.name, NULL);

    if (props.apn)
        qmi_message_wds_create_profile_input_set_apn_name (*input, props.apn, NULL);

    if (props.auth_set)
        qmi_message_wds_create_profile_input_set_authentication (*input, props.auth, NULL);

    if (props.username)
        qmi_message_wds_create_profile_input_set_username (*input, props.username, NULL);

    if (props.password)
        qmi_message_wds_create_profile_input_set_password (*input, props.password, NULL);

    if (props.no_roaming_set)
        qmi_message_wds_create_profile_input_set_roaming_disallowed_flag (*input, props.no_roaming, NULL);

    if (props.disabled_set)
        qmi_message_wds_create_profile_input_set_apn_disabled_flag (*input, props.disabled, NULL);

    success = TRUE;

out:
    g_strfreev (split);
    g_free     (props.name);
    g_free     (props.apn);
    g_free     (props.username);
    g_free     (props.password);

    return success;
}

static void
create_profile_ready (QmiClientWds *client,
                      GAsyncResult *res)
{
    QmiMessageWdsCreateProfileOutput *output;
    GError *error = NULL;
    QmiWdsProfileType profile_type;
    guint8 profile_index;

    output = qmi_client_wds_create_profile_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_wds_create_profile_output_get_result (output, &error)) {
        QmiWdsDsProfileError ds_profile_error;

        if (g_error_matches (error,
                             QMI_PROTOCOL_ERROR,
                             QMI_PROTOCOL_ERROR_EXTENDED_INTERNAL) &&
            qmi_message_wds_create_profile_output_get_extended_error_code (
                output,
                &ds_profile_error,
                NULL)) {
            g_printerr ("error: couldn't create profile: ds profile error: %s\n",
                        qmi_wds_ds_profile_error_get_string (ds_profile_error));
        } else {
            g_printerr ("error: couldn't create profile: %s\n",
                        error->message);
        }
        g_error_free (error);
        qmi_message_wds_create_profile_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("New profile created:\n");
    if (qmi_message_wds_create_profile_output_get_profile_identifier (output,
                                                                      &profile_type,
                                                                      &profile_index,
                                                                      NULL)) {
        g_print ("\tProfile type: '%s'\n", qmi_wds_profile_type_get_string(profile_type));
        g_print ("\tProfile index: '%d'\n", profile_index);
    }
    qmi_message_wds_create_profile_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /*  HAVE_QMI_MESSAGE_WDS_CREATE_PROFILE */

#if defined HAVE_QMI_MESSAGE_WDS_SWI_CREATE_PROFILE_INDEXED

static gboolean
swi_create_profile_indexed_input_create (const gchar                                *str,
                                         QmiMessageWdsSwiCreateProfileIndexedInput **input,
                                         GError                                    **error)
{
    GError *parse_error = NULL;
    CreateModifyProfileProperties props = {};
    gchar **split;
    gboolean success = FALSE;

    g_assert (input && !*input);

    /* Expect max 3 tokens: the first two give us the mandatory parameters of the command,
     * the 3rd one will contain the key/value pair list */
    split = g_strsplit (str, ",", 3);
    if (g_strv_length (split) < 2) {
        g_set_error (error,
                     QMI_CORE_ERROR,
                     QMI_CORE_ERROR_FAILED,
                     "expected at least 2 arguments for 'wds swi create profile indexed' command");
        goto out;
    }

    g_strstrip (split[0]);
    if (g_str_equal (split[0], "3gpp"))
        props.profile_type = QMI_WDS_PROFILE_TYPE_3GPP;
    else if (g_str_equal (split[0], "3gpp2"))
        props.profile_type = QMI_WDS_PROFILE_TYPE_3GPP2;
    else {
        g_set_error (error,
                     QMI_CORE_ERROR,
                     QMI_CORE_ERROR_FAILED,
                     "invalid profile type '%s'. Expected '3gpp' or '3gpp2'.'",
                     split[0]);
        goto out;
    }

    g_strstrip (split[1]);
    props.profile_index = atoi (split[1]);
    if (props.profile_index <= 0 || props.profile_index > G_MAXUINT8) {
        g_set_error (error,
                     QMI_CORE_ERROR,
                     QMI_CORE_ERROR_FAILED,
                     "invalid or out of range profile index [1,%u]: '%s'\n",
                     G_MAXUINT8,
                     split[1]);
        goto out;
    }

    if (split[2]) {
        if (!qmicli_parse_key_value_string (split[2],
                                            &parse_error,
                                            create_modify_profile_properties_handle,
                                            &props)) {
            g_set_error (error,
                         QMI_CORE_ERROR,
                         QMI_CORE_ERROR_FAILED,
                         "couldn't parse input string: %s",
                         parse_error->message);
            g_error_free (parse_error);
            goto out;
        }
    }

    /* Create input bundle */
    *input = qmi_message_wds_swi_create_profile_indexed_input_new ();

    /* Profile identifier is required */
    qmi_message_wds_swi_create_profile_indexed_input_set_profile_identifier (*input, props.profile_type, props.profile_index, NULL);

    if (props.context_number)
        qmi_message_wds_swi_create_profile_indexed_input_set_pdp_context_number (*input, props.context_number, NULL);

    if (props.pdp_type_set)
        qmi_message_wds_swi_create_profile_indexed_input_set_pdp_type (*input, props.pdp_type, NULL);

    if (props.name)
        qmi_message_wds_swi_create_profile_indexed_input_set_profile_name (*input, props.name, NULL);

    if (props.apn)
        qmi_message_wds_swi_create_profile_indexed_input_set_apn_name (*input, props.apn, NULL);

    if (props.auth_set)
        qmi_message_wds_swi_create_profile_indexed_input_set_authentication (*input, props.auth, NULL);

    if (props.username)
        qmi_message_wds_swi_create_profile_indexed_input_set_username (*input, props.username, NULL);

    if (props.password)
        qmi_message_wds_swi_create_profile_indexed_input_set_password (*input, props.password, NULL);

    if (props.no_roaming_set)
        qmi_message_wds_swi_create_profile_indexed_input_set_roaming_disallowed_flag (*input, props.no_roaming, NULL);

    if (props.disabled_set)
        qmi_message_wds_swi_create_profile_indexed_input_set_apn_disabled_flag (*input, props.disabled, NULL);

    success = TRUE;

out:
    g_strfreev (split);
    g_free     (props.name);
    g_free     (props.apn);
    g_free     (props.username);
    g_free     (props.password);

    return success;
}

static void
swi_create_profile_indexed_ready (QmiClientWds *client,
                                  GAsyncResult *res)
{
    QmiMessageWdsSwiCreateProfileIndexedOutput *output;
    GError *error = NULL;
    QmiWdsProfileType profile_type;
    guint8 profile_index;

    output = qmi_client_wds_swi_create_profile_indexed_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_wds_swi_create_profile_indexed_output_get_result (output, &error)) {
        g_printerr ("error: couldn't create indexed profile: %s\n", error->message);
        g_error_free (error);
        qmi_message_wds_swi_create_profile_indexed_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("New profile created:\n");
    if (qmi_message_wds_swi_create_profile_indexed_output_get_profile_identifier (output,
                                                                                  &profile_type,
                                                                                  &profile_index,
                                                                                  NULL)) {
        g_print ("\tProfile type: '%s'\n", qmi_wds_profile_type_get_string(profile_type));
        g_print ("\tProfile index: '%d'\n", profile_index);
    }
    qmi_message_wds_swi_create_profile_indexed_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_WDS_SWI_CREATE_PROFILE_INDEXED */

#if defined HAVE_QMI_MESSAGE_WDS_MODIFY_PROFILE

static gboolean
modify_profile_input_create (const gchar                      *str,
                             QmiMessageWdsModifyProfileInput **input,
                             GError                          **error)
{
    GError *parse_error = NULL;
    CreateModifyProfileProperties props = {};
    gchar **split;
    gboolean success = FALSE;

    g_assert (input && !*input);

    /* Expect max 3 tokens: the first two give us the mandatory parameters of the command,
     * the 3rd one will contain the key/value pair list */
    split = g_strsplit (str, ",", 3);
    if (g_strv_length (split) < 3) {
        g_set_error (error,
                     QMI_CORE_ERROR,
                     QMI_CORE_ERROR_FAILED,
                     "expected at least 3 arguments for 'wds modify profile' command");
        goto out;
    }

    g_strstrip (split[0]);
    if (g_str_equal (split[0], "3gpp"))
        props.profile_type = QMI_WDS_PROFILE_TYPE_3GPP;
    else if (g_str_equal (split[0], "3gpp2"))
        props.profile_type = QMI_WDS_PROFILE_TYPE_3GPP2;
    else {
        g_set_error (error,
                     QMI_CORE_ERROR,
                     QMI_CORE_ERROR_FAILED,
                     "invalid profile type '%s'. Expected '3gpp' or '3gpp2'.'",
                     split[0]);
        goto out;
    }

    g_strstrip (split[1]);
    props.profile_index = atoi (split[1]);
    if (props.profile_index <= 0 || props.profile_index > G_MAXUINT8) {
        g_set_error (error,
                     QMI_CORE_ERROR,
                     QMI_CORE_ERROR_FAILED,
                     "invalid or out of range profile index [1,%u]: '%s'\n",
                     G_MAXUINT8,
                     split[1]);
        goto out;
    }

    /* advance to third token, that's where key/value pairs start */
    if (!qmicli_parse_key_value_string (split[2],
                                        &parse_error,
                                        create_modify_profile_properties_handle,
                                        &props)) {
        g_set_error (error,
                     QMI_CORE_ERROR,
                     QMI_CORE_ERROR_FAILED,
                     "couldn't parse input string: %s",
                    parse_error->message);
        g_error_free (parse_error);
        goto out;
    }

    /* Create input bundle */
    *input = qmi_message_wds_modify_profile_input_new ();

    /* Profile identifier is required */
    qmi_message_wds_modify_profile_input_set_profile_identifier (*input, props.profile_type, props.profile_index, NULL);

    if (props.context_number)
        qmi_message_wds_modify_profile_input_set_pdp_context_number (*input, props.context_number, NULL);

    if (props.pdp_type_set)
        qmi_message_wds_modify_profile_input_set_pdp_type (*input, props.pdp_type, NULL);

    if (props.apn_type_set)
        qmi_message_wds_modify_profile_input_set_apn_type_mask (*input, props.apn_type, NULL);

    if (props.name)
        qmi_message_wds_modify_profile_input_set_profile_name (*input, props.name, NULL);

    if (props.apn)
        qmi_message_wds_modify_profile_input_set_apn_name (*input, props.apn, NULL);

    if (props.auth_set)
        qmi_message_wds_modify_profile_input_set_authentication (*input, props.auth, NULL);

    if (props.username)
        qmi_message_wds_modify_profile_input_set_username (*input, props.username, NULL);

    if (props.password)
        qmi_message_wds_modify_profile_input_set_password (*input, props.password, NULL);

    if (props.no_roaming_set)
        qmi_message_wds_modify_profile_input_set_roaming_disallowed_flag (*input, props.no_roaming, NULL);

    if (props.disabled_set)
        qmi_message_wds_modify_profile_input_set_apn_disabled_flag (*input, props.disabled, NULL);

    success = TRUE;

out:
    g_strfreev (split);
    g_free     (props.name);
    g_free     (props.apn);
    g_free     (props.username);
    g_free     (props.password);

    return success;
}

static void
modify_profile_ready (QmiClientWds *client,
                      GAsyncResult *res)
{
    QmiMessageWdsModifyProfileOutput *output;
    GError *error = NULL;

    output = qmi_client_wds_modify_profile_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_wds_modify_profile_output_get_result (output, &error)) {
        QmiWdsDsProfileError ds_profile_error;

        if (g_error_matches (error,
                             QMI_PROTOCOL_ERROR,
                             QMI_PROTOCOL_ERROR_EXTENDED_INTERNAL) &&
            qmi_message_wds_modify_profile_output_get_extended_error_code (
                output,
                &ds_profile_error,
                NULL)) {
            g_printerr ("error: couldn't modify profile: ds profile error: %s\n",
                        qmi_wds_ds_profile_error_get_string (ds_profile_error));
        } else {
            g_printerr ("error: couldn't modify profile: %s\n",
                        error->message);
        }
        g_error_free (error);
        qmi_message_wds_modify_profile_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }
    qmi_message_wds_modify_profile_output_unref (output);
    g_print ("Profile successfully modified.\n");
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_WDS_MODIFY_PROFILE */

#if defined HAVE_QMI_MESSAGE_WDS_DELETE_PROFILE

static void
delete_profile_ready (QmiClientWds *client,
                      GAsyncResult *res)
{
    QmiMessageWdsDeleteProfileOutput *output;
    GError *error = NULL;

    output = qmi_client_wds_delete_profile_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_wds_delete_profile_output_get_result (output, &error)) {
        QmiWdsDsProfileError ds_profile_error;

        if (g_error_matches (error,
                             QMI_PROTOCOL_ERROR,
                             QMI_PROTOCOL_ERROR_EXTENDED_INTERNAL) &&
            qmi_message_wds_delete_profile_output_get_extended_error_code (
                output,
                &ds_profile_error,
                NULL)) {
            g_printerr ("error: couldn't delete profile: ds profile error: %s\n",
                        qmi_wds_ds_profile_error_get_string (ds_profile_error));
        } else {
            g_printerr ("error: couldn't delete profile: %s\n",
                        error->message);
        }
        g_error_free (error);
        qmi_message_wds_delete_profile_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }
    qmi_message_wds_delete_profile_output_unref (output);
    g_print ("Profile successfully deleted.\n");
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_WDS_DELETE_PROFILE */

#if defined HAVE_QMI_MESSAGE_WDS_GET_PROFILE_LIST && defined HAVE_QMI_MESSAGE_WDS_GET_PROFILE_SETTINGS

typedef struct {
    guint i;
    GArray *profile_list;
} GetProfileListContext;

static void get_next_profile_settings (GetProfileListContext *inner_ctx);

static void
get_profile_settings_ready (QmiClientWds *client,
                            GAsyncResult *res,
                            GetProfileListContext *inner_ctx)
{
    QmiMessageWdsGetProfileSettingsOutput *output;
    GError *error = NULL;

    output = qmi_client_wds_get_profile_settings_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
    } else if (!qmi_message_wds_get_profile_settings_output_get_result (output, &error)) {
        QmiWdsDsProfileError ds_profile_error;

        if (g_error_matches (error,
                             QMI_PROTOCOL_ERROR,
                             QMI_PROTOCOL_ERROR_EXTENDED_INTERNAL) &&
            qmi_message_wds_get_profile_settings_output_get_extended_error_code (
                output,
                &ds_profile_error,
                NULL)) {
            g_printerr ("error: couldn't get profile settings: ds profile error: %s\n",
                        qmi_wds_ds_profile_error_get_string (ds_profile_error));
        } else {
            g_printerr ("error: couldn't get profile settings: %s\n",
                        error->message);
        }
        g_error_free (error);
        qmi_message_wds_get_profile_settings_output_unref (output);
    } else {
        const gchar *str;
        guint8 context_number;
        QmiWdsPdpType pdp_type;
        QmiWdsAuthentication auth;
        QmiWdsApnTypeMask apn_type;
        gboolean flag;

        if (qmi_message_wds_get_profile_settings_output_get_apn_name (output, &str, NULL))
            g_print ("\t\tAPN: '%s'\n", str);
        if (qmi_message_wds_get_profile_settings_output_get_apn_type_mask (output, &apn_type, NULL)) {
            g_autofree gchar *aux = NULL;

            aux = qmi_wds_apn_type_mask_build_string_from_mask (apn_type);
            g_print ("\t\tAPN type: '%s'\n", VALIDATE_MASK_NONE (aux));
        }
        if (qmi_message_wds_get_profile_settings_output_get_pdp_type (output, &pdp_type, NULL))
            g_print ("\t\tPDP type: '%s'\n", qmi_wds_pdp_type_get_string (pdp_type));
        if (qmi_message_wds_get_profile_settings_output_get_pdp_context_number (output, &context_number, NULL))
            g_print ("\t\tPDP context number: '%d'\n", context_number);
        if (qmi_message_wds_get_profile_settings_output_get_username (output, &str, NULL))
            g_print ("\t\tUsername: '%s'\n", str);
        if (qmi_message_wds_get_profile_settings_output_get_password (output, &str, NULL))
            g_print ("\t\tPassword: '%s'\n", str);
        if (qmi_message_wds_get_profile_settings_output_get_authentication (output, &auth, NULL)) {
            g_autofree gchar *aux = NULL;

            aux = qmi_wds_authentication_build_string_from_mask (auth);
            g_print ("\t\tAuth: '%s'\n", VALIDATE_MASK_NONE (aux));
        }
        if (qmi_message_wds_get_profile_settings_output_get_roaming_disallowed_flag (output, &flag, NULL))
            g_print ("\t\tNo roaming: '%s'\n", flag ? "yes" : "no");
        if (qmi_message_wds_get_profile_settings_output_get_apn_disabled_flag (output, &flag, NULL))
            g_print ("\t\tAPN disabled: '%s'\n", flag ? "yes" : "no");
        qmi_message_wds_get_profile_settings_output_unref (output);
    }

    /* Keep on */
    inner_ctx->i++;
    get_next_profile_settings (inner_ctx);
}

static void
get_next_profile_settings (GetProfileListContext *inner_ctx)
{
    QmiMessageWdsGetProfileListOutputProfileListProfile *profile;
    QmiMessageWdsGetProfileSettingsInput *input;

    if (inner_ctx->i >= inner_ctx->profile_list->len) {
        /* All done */
        g_array_unref (inner_ctx->profile_list);
        g_slice_free (GetProfileListContext, inner_ctx);
        operation_shutdown (TRUE);
        return;
    }

    profile = &g_array_index (inner_ctx->profile_list, QmiMessageWdsGetProfileListOutputProfileListProfile, inner_ctx->i);
    g_print ("\t[%u] %s - %s\n",
             profile->profile_index,
             qmi_wds_profile_type_get_string (profile->profile_type),
             profile->profile_name);

    input = qmi_message_wds_get_profile_settings_input_new ();
    qmi_message_wds_get_profile_settings_input_set_profile_id (
        input,
        profile->profile_type,
        profile->profile_index,
        NULL);
    qmi_client_wds_get_profile_settings (ctx->client,
                                         input,
                                         3,
                                         NULL,
                                         (GAsyncReadyCallback)get_profile_settings_ready,
                                         inner_ctx);
    qmi_message_wds_get_profile_settings_input_unref (input);
}

static void
get_profile_list_ready (QmiClientWds *client,
                        GAsyncResult *res)
{
    GError *error = NULL;
    QmiMessageWdsGetProfileListOutput *output;
    GetProfileListContext *inner_ctx;
    GArray *profile_list = NULL;

    output = qmi_client_wds_get_profile_list_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n",
                    error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_wds_get_profile_list_output_get_result (output, &error)) {
        QmiWdsDsProfileError ds_profile_error;

        if (g_error_matches (error,
                             QMI_PROTOCOL_ERROR,
                             QMI_PROTOCOL_ERROR_EXTENDED_INTERNAL) &&
            qmi_message_wds_get_profile_list_output_get_extended_error_code (
                output,
                &ds_profile_error,
                NULL)) {
            g_printerr ("error: couldn't get profile list: ds profile error: %s\n",
                        qmi_wds_ds_profile_error_get_string (ds_profile_error));
        } else {
            g_printerr ("error: couldn't get profile list: %s\n",
                        error->message);
        }

        g_error_free (error);
        qmi_message_wds_get_profile_list_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    qmi_message_wds_get_profile_list_output_get_profile_list (output, &profile_list, NULL);

    if (!profile_list || !profile_list->len) {
        g_print ("Profile list empty\n");
        qmi_message_wds_get_profile_list_output_unref (output);
        operation_shutdown (TRUE);
        return;
    }

    g_print ("Profile list retrieved:\n");

    inner_ctx = g_slice_new (GetProfileListContext);
    inner_ctx->profile_list = g_array_ref (profile_list);
    inner_ctx->i = 0;
    get_next_profile_settings (inner_ctx);
}

#endif /* HAVE_QMI_MESSAGE_WDS_GET_PROFILE_LIST
        * HAVE_QMI_MESSAGE_WDS_GET_PROFILE_SETTINGS */

#if defined HAVE_QMI_MESSAGE_WDS_GET_DEFAULT_SETTINGS

static void
get_default_settings_ready (QmiClientWds *client,
                            GAsyncResult *res)
{
    QmiMessageWdsGetDefaultSettingsOutput *output;
    GError *error = NULL;
    const gchar *str;
    QmiWdsPdpType pdp_type;
    QmiWdsAuthentication auth;

    output = qmi_client_wds_get_default_settings_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_wds_get_default_settings_output_get_result (output, &error)) {
        QmiWdsDsProfileError ds_profile_error;

        if (g_error_matches (error,
                             QMI_PROTOCOL_ERROR,
                             QMI_PROTOCOL_ERROR_EXTENDED_INTERNAL) &&
            qmi_message_wds_get_default_settings_output_get_extended_error_code (
                output,
                &ds_profile_error,
                NULL)) {
            g_printerr ("error: couldn't get default settings: ds default error: %s\n",
                        qmi_wds_ds_profile_error_get_string (ds_profile_error));
        } else {
            g_printerr ("error: couldn't get default settings: %s\n",
                        error->message);
        }
        g_error_free (error);
        qmi_message_wds_get_default_settings_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("Default settings retrieved:\n");

    if (qmi_message_wds_get_default_settings_output_get_apn_name (output, &str, NULL))
        g_print ("\tAPN: '%s'\n", str);
    if (qmi_message_wds_get_default_settings_output_get_pdp_type (output, &pdp_type, NULL))
        g_print ("\tPDP type: '%s'\n", qmi_wds_pdp_type_get_string (pdp_type));
    if (qmi_message_wds_get_default_settings_output_get_username (output, &str, NULL))
        g_print ("\tUsername: '%s'\n", str);
    if (qmi_message_wds_get_default_settings_output_get_password (output, &str, NULL))
        g_print ("\tPassword: '%s'\n", str);
    if (qmi_message_wds_get_default_settings_output_get_authentication (output, &auth, NULL)) {
        g_autofree gchar *aux = NULL;

        aux = qmi_wds_authentication_build_string_from_mask (auth);
        g_print ("\tAuth: '%s'\n", VALIDATE_MASK_NONE (aux));
    }

    qmi_message_wds_get_default_settings_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_WDS_GET_DEFAULT_SETTINGS */

#if defined HAVE_QMI_MESSAGE_WDS_GET_DEFAULT_PROFILE_NUMBER

static void
get_default_profile_number_ready (QmiClientWds *client,
                                  GAsyncResult *res)
{
    g_autoptr(QmiMessageWdsGetDefaultProfileNumberOutput) output = NULL;
    g_autoptr(GError)                                     error = NULL;
    guint8                                                profile_num;

    output = qmi_client_wds_get_default_profile_number_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_wds_get_default_profile_number_output_get_result (output, &error)) {
        QmiWdsDsProfileError ds_profile_error;

        if (g_error_matches (error,
                             QMI_PROTOCOL_ERROR,
                             QMI_PROTOCOL_ERROR_EXTENDED_INTERNAL) &&
            qmi_message_wds_get_default_profile_number_output_get_extended_error_code (
                output,
                &ds_profile_error,
                NULL)) {
            g_printerr ("error: couldn't get default profile number: ds profile error: %s\n",
                        qmi_wds_ds_profile_error_get_string (ds_profile_error));
        } else {
            g_printerr ("error: couldn't get default profile number: %s\n",
                        error->message);
        }
        operation_shutdown (FALSE);
        return;
    }

    g_print ("Default profile number retrieved:\n");
    if (qmi_message_wds_get_default_profile_number_output_get_index (output, &profile_num, NULL))
        g_print ("\tDefault profile number: '%d'\n", profile_num);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_WDS_GET_DEFAULT_PROFILE_NUMBER */

#if defined HAVE_QMI_MESSAGE_WDS_SET_DEFAULT_PROFILE_NUMBER

static void
set_default_profile_number_ready (QmiClientWds *client,
                                  GAsyncResult *res)
{
    g_autoptr(QmiMessageWdsSetDefaultProfileNumberOutput) output = NULL;
    g_autoptr(GError)                                     error = NULL;

    output = qmi_client_wds_set_default_profile_number_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_wds_set_default_profile_number_output_get_result (output, &error)) {
        QmiWdsDsProfileError ds_profile_error;

        if (g_error_matches (error,
                             QMI_PROTOCOL_ERROR,
                             QMI_PROTOCOL_ERROR_EXTENDED_INTERNAL) &&
            qmi_message_wds_set_default_profile_number_output_get_extended_error_code (
                output,
                &ds_profile_error,
                NULL)) {
            g_printerr ("error: couldn't set default settings: ds profile error: %s\n",
                        qmi_wds_ds_profile_error_get_string (ds_profile_error));
        } else {
            g_printerr ("error: couldn't set default settings: %s\n",
                        error->message);
        }
        operation_shutdown (FALSE);
        return;
    }

    g_print ("Default profile number updated\n");
    operation_shutdown (TRUE);
}

static QmiMessageWdsSetDefaultProfileNumberInput *
set_default_profile_number_input_create (const gchar *str)
{
    g_autoptr(QmiMessageWdsSetDefaultProfileNumberInput) input = NULL;
    g_autoptr(GError)                                    error = NULL;
    g_auto(GStrv)                                        split = NULL;
    QmiWdsProfileType                                    profile_type;
    guint                                                profile_num;

    split = g_strsplit (str, ",", -1);
    input = qmi_message_wds_set_default_profile_number_input_new ();

    if (g_strv_length (split) != 2) {
        g_printerr ("error: expected 2 options in default profile number settings\n");
        return NULL;
    }

    g_strstrip (split[0]);
    if (g_str_equal (split[0], "3gpp"))
        profile_type = QMI_WDS_PROFILE_TYPE_3GPP;
    else if (g_str_equal (split[0], "3gpp2"))
        profile_type = QMI_WDS_PROFILE_TYPE_3GPP2;
    else {
        g_printerr ("error: invalid profile type '%s'. Expected '3gpp' or '3gpp2'.'\n", split[0]);
        return NULL;
    }

    g_strstrip (split[1]);
    profile_num = atoi (split[1]);
    if (profile_num <= 0 || profile_num > G_MAXUINT8) {
        g_printerr ("error: invalid or out of range profile number [1,%u]: '%s'\n",
                    G_MAXUINT8,
                    split[1]);
        return NULL;
    }

    if (!qmi_message_wds_set_default_profile_number_input_set_profile_identifier (
            input,
            profile_type,
            QMI_WDS_PROFILE_FAMILY_TETHERED,
            (guint8)profile_num,
            &error)) {
        g_printerr ("error: couldn't create input bundle: '%s'\n", error->message);
        return NULL;
    }

    return g_steal_pointer (&input);
}

#endif /* HAVE_QMI_MESSAGE_WDS_SET_DEFAULT_PROFILE_NUMBER */

#if defined HAVE_QMI_MESSAGE_WDS_GET_AUTOCONNECT_SETTINGS

static void
get_autoconnect_settings_ready (QmiClientWds *client,
                                GAsyncResult *res)
{
    QmiMessageWdsGetAutoconnectSettingsOutput *output;
    GError *error = NULL;
    QmiWdsAutoconnectSetting status;
    QmiWdsAutoconnectSettingRoaming roaming;

    output = qmi_client_wds_get_autoconnect_settings_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_wds_get_autoconnect_settings_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get autoconnect settings: %s\n",
                    error->message);
        g_error_free (error);
        qmi_message_wds_get_autoconnect_settings_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("Autoconnect settings retrieved:\n");

    qmi_message_wds_get_autoconnect_settings_output_get_status (output, &status, NULL);
    g_print ("\tStatus: '%s'\n", qmi_wds_autoconnect_setting_get_string (status));

    if (qmi_message_wds_get_autoconnect_settings_output_get_roaming (output, &roaming, NULL))
        g_print ("\tRoaming: '%s'\n", qmi_wds_autoconnect_setting_roaming_get_string (roaming));

    qmi_message_wds_get_autoconnect_settings_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_WDS_GET_AUTOCONNECT_SETTINGS */

#if defined HAVE_QMI_MESSAGE_WDS_SET_AUTOCONNECT_SETTINGS

static QmiMessageWdsSetAutoconnectSettingsInput *
set_autoconnect_settings_input_create (const gchar *str)
{
    QmiMessageWdsSetAutoconnectSettingsInput *input = NULL;
    gchar **split;
    QmiWdsAutoconnectSetting status;
    QmiWdsAutoconnectSettingRoaming roaming;
    GError *error = NULL;

    split = g_strsplit (str, ",", -1);
    input = qmi_message_wds_set_autoconnect_settings_input_new ();

    if (g_strv_length (split) != 2 && g_strv_length (split) != 1) {
        g_printerr ("error: expected 1 or 2 options in autoconnect settings\n");
        goto error_out;
    }

    g_strstrip (split[0]);
    if (!qmicli_read_wds_autoconnect_setting_from_string (split[0], &status)) {
        g_printerr ("error: failed to parse autoconnect setting\n");
        goto error_out;
    }
    if (!qmi_message_wds_set_autoconnect_settings_input_set_status (input, status, &error)) {
        g_printerr ("error: couldn't create input data bundle: '%s'\n",
                    error->message);
        goto error_out;
    }

    if (g_strv_length (split) == 2) {
        g_strstrip (split[1]);
        if (!qmicli_read_wds_autoconnect_setting_roaming_from_string (g_str_equal (split[1], "roaming-allowed") ? "allowed" : split[1], &roaming)) {
            g_printerr ("error: failed to parse autoconnect roaming setting\n");
            goto error_out;
        }
        if (!qmi_message_wds_set_autoconnect_settings_input_set_roaming (input, roaming, &error)) {
            g_printerr ("error: couldn't create input data bundle: '%s'\n",
                        error->message);
            goto error_out;
        }
    }

    g_strfreev (split);
    return input;

error_out:
    if (error)
        g_error_free (error);
    g_strfreev (split);
    qmi_message_wds_set_autoconnect_settings_input_unref (input);
    return NULL;
}

static void
set_autoconnect_settings_ready (QmiClientWds *client,
                                GAsyncResult *res)
{
    QmiMessageWdsSetAutoconnectSettingsOutput *output;
    GError *error = NULL;

    output = qmi_client_wds_set_autoconnect_settings_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_wds_set_autoconnect_settings_output_get_result (output, &error)) {
        g_printerr ("error: couldn't set autoconnect settings: %s\n",
                    error->message);
        g_error_free (error);
        qmi_message_wds_set_autoconnect_settings_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("Autoconnect settings updated\n");
    qmi_message_wds_set_autoconnect_settings_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_WDS_SET_AUTOCONNECT_SETTINGS */

#if defined HAVE_QMI_MESSAGE_WDS_GET_SUPPORTED_MESSAGES

static void
get_supported_messages_ready (QmiClientWds *client,
                              GAsyncResult *res)
{
    QmiMessageWdsGetSupportedMessagesOutput *output;
    GError *error = NULL;
    GArray *bytearray = NULL;
    gchar *str;

    output = qmi_client_wds_get_supported_messages_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_wds_get_supported_messages_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get supported WDS messages: %s\n", error->message);
        g_error_free (error);
        qmi_message_wds_get_supported_messages_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] Successfully got supported WDS messages:\n",
             qmi_device_get_path_display (ctx->device));

    qmi_message_wds_get_supported_messages_output_get_list (output, &bytearray, NULL);
    str = qmicli_get_supported_messages_list (bytearray ? (const guint8 *)bytearray->data : NULL,
                                              bytearray ? bytearray->len : 0);
    g_print ("%s", str);
    g_free (str);

    qmi_message_wds_get_supported_messages_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_WDS_GET_SUPPORTED_MESSAGES */

#if defined HAVE_QMI_MESSAGE_WDS_RESET

static void
reset_ready (QmiClientWds *client,
             GAsyncResult *res)
{
    QmiMessageWdsResetOutput *output;
    GError *error = NULL;

    output = qmi_client_wds_reset_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_wds_reset_output_get_result (output, &error)) {
        g_printerr ("error: couldn't reset the WDS service: %s\n", error->message);
        g_error_free (error);
        qmi_message_wds_reset_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] Successfully performed WDS service reset\n",
             qmi_device_get_path_display (ctx->device));

    qmi_message_wds_reset_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_WDS_RESET */

static gboolean
noop_cb (gpointer unused)
{
    operation_shutdown (TRUE);
    return FALSE;
}

#if defined HAVE_QMI_MESSAGE_WDS_BIND_DATA_PORT

static QmiMessageWdsBindDataPortInput *
bind_data_port_input_create (const gchar *str)
{
    g_autoptr(QmiMessageWdsBindDataPortInput) input = NULL;
    g_autoptr(GError)                         error = NULL;
    QmiSioPort                                sio_port;

    sio_port = strtoul(str, NULL, 0);
    if (!sio_port && !qmicli_read_sio_port_from_string (str, &sio_port))
        return NULL;

    input = qmi_message_wds_bind_data_port_input_new ();
    if (!qmi_message_wds_bind_data_port_input_set_data_port (input, sio_port, &error)) {
        g_printerr ("error: couldn't set data port: '%s'\n", error->message);
        return NULL;
    }

    return g_steal_pointer (&input);
}

static void
bind_data_port_ready (QmiClientWds *client,
                      GAsyncResult *res)
{
    g_autoptr(QmiMessageWdsBindDataPortOutput) output = NULL;
    g_autoptr(GError)                          error = NULL;

    output = qmi_client_wds_bind_data_port_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_wds_bind_data_port_output_get_result (output, &error)) {
        g_printerr ("error: couldn't bind data port: %s\n", error->message);
        operation_shutdown (FALSE);
        return;
    }

    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_WDS_BIND_DATA_PORT */

#if defined HAVE_QMI_MESSAGE_WDS_BIND_MUX_DATA_PORT

typedef struct {
    guint8 mux_id;
    QmiDataEndpointType ep_type;
    gint ep_iface_number;
    QmiWdsClientType client_type;
} BindMuxDataPortProperties;

static gboolean
bind_mux_data_port_properties_handle (const gchar *key,
                                      const gchar *value,
                                      GError     **error,
                                      gpointer     user_data)
{
    BindMuxDataPortProperties *props = user_data;

    if (!value || !value[0]) {
        g_set_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_FAILED,
                     "key '%s' requires a value", key);
        return FALSE;
    }

    if (g_ascii_strcasecmp (key, "mux-id") == 0) {
        guint aux;

        /* QMI_WDS_MUX_ID_UNDEFINED is G_MAXUINT8 (0xff) */
        if (!qmicli_read_uint_from_string (value, &aux) || (aux >= G_MAXUINT8)) {
            g_set_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_FAILED,
                         "failed reading key 'mux-id' value in range [0,254]: '%s'", value);
            return FALSE;
        }
        props->mux_id = (guint8) aux;
        return TRUE;
    }

    if (g_ascii_strcasecmp (key, "ep-type") == 0) {
        if (!qmicli_read_data_endpoint_type_from_string (value, &(props->ep_type))) {
            g_set_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_FAILED,
                         "failed reading key 'ep-type' value: '%s'", value);
            return FALSE;
        }
        return TRUE;
    }

    if (g_ascii_strcasecmp (key, "ep-iface-number") == 0) {
        guint aux;

        if (!qmicli_read_uint_from_string (value, &aux)) {
            g_set_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_FAILED,
                         "failed reading key 'ep-iface-number' value: '%s'", value);
            return FALSE;
        }

        props->ep_iface_number = (gint) aux;
        return TRUE;
    }

    g_set_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_FAILED,
                 "unrecognized key: '%s'", key);
    return FALSE;
}

static QmiMessageWdsBindMuxDataPortInput *
bind_mux_data_port_input_create (const gchar *str)
{
    QmiMessageWdsBindMuxDataPortInput *input = NULL;
    GError *error = NULL;
    BindMuxDataPortProperties props = {
        .mux_id = QMI_WDS_MUX_ID_UNDEFINED,
        .ep_type = QMI_DATA_ENDPOINT_TYPE_HSUSB,
        .ep_iface_number = QMI_WDS_ENDPOINT_INTERFACE_NUMBER_UNDEFINED,
        .client_type = QMI_WDS_CLIENT_TYPE_TETHERED,
    };

    if (!str[0])
        return NULL;

    if (strchr (str, '=')) {
        if (!qmicli_parse_key_value_string (str,
                                            &error,
                                            bind_mux_data_port_properties_handle,
                                            &props)) {
            g_printerr ("error: could not parse input string '%s'\n", error->message);
            g_error_free (error);
            return NULL;
        }
    } else {
        g_printerr ("error: malformed input string, key=value format expected.\n");
        goto error_out;
    }


    if ((props.mux_id == QMI_WDS_MUX_ID_UNDEFINED) ||
        (props.ep_iface_number == QMI_WDS_ENDPOINT_INTERFACE_NUMBER_UNDEFINED)) {
        g_printerr ("error: Mux ID and Endpoint Iface Number are both needed\n");
        return NULL;
    }

    input = qmi_message_wds_bind_mux_data_port_input_new ();

    if (!qmi_message_wds_bind_mux_data_port_input_set_endpoint_info (input, props.ep_type, props.ep_iface_number, &error)) {
        g_printerr ("error: couldn't set endpoint info: '%s'\n", error->message);
        goto error_out;
    }

    if (!qmi_message_wds_bind_mux_data_port_input_set_mux_id (input, props.mux_id, &error)) {
        g_printerr ("error: couldn't set mux ID %d: '%s'\n", props.mux_id, error->message);
        goto error_out;
    }

    if (!qmi_message_wds_bind_mux_data_port_input_set_client_type (input, props.client_type , &error)) {
        g_printerr ("error: couldn't set client type: '%s'\n", error->message);
        goto error_out;
    }

    return input;

error_out:
    if (error)
        g_error_free (error);
    qmi_message_wds_bind_mux_data_port_input_unref (input);
    return NULL;
}

static void
bind_mux_data_port_ready (QmiClientWds *client,
                          GAsyncResult *res) {
    QmiMessageWdsBindMuxDataPortOutput *output;
    GError *error = NULL;

    output = qmi_client_wds_bind_mux_data_port_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_wds_bind_mux_data_port_output_get_result (output, &error)) {
        g_printerr ("error: couldn't bind mux data port: %s\n", error->message);
        g_error_free (error);
        qmi_message_wds_bind_mux_data_port_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    qmi_message_wds_bind_mux_data_port_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_WDS_BIND_MUX_DATA_PORT */

#if defined HAVE_QMI_MESSAGE_WDS_SET_IP_FAMILY

static void
set_ip_family_ready (QmiClientWds *client,
                     GAsyncResult *res)
{
    QmiMessageWdsSetIpFamilyOutput *output;
    GError *error = NULL;

    output = qmi_client_wds_set_ip_family_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_wds_set_ip_family_output_get_result (output, &error)) {
        g_printerr ("error: couldn't set IP family: %s\n", error->message);
        g_error_free (error);
        qmi_message_wds_set_ip_family_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    qmi_message_wds_set_ip_family_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_WDS_SET_IP_FAMILY */

#if defined HAVE_QMI_MESSAGE_WDS_GET_CHANNEL_RATES

static void
get_channel_rates_ready (QmiClientWds *client,
                         GAsyncResult *res)
{
    QmiMessageWdsGetChannelRatesOutput *output;
    guint32 txrate = 0, rxrate = 0, maxtxrate = 0, maxrxrate = 0;
    GError *error = NULL;

    output = qmi_client_wds_get_channel_rates_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_wds_get_channel_rates_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get channel rates: %s\n",
                    error->message);
        g_error_free (error);
        qmi_message_wds_get_channel_rates_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("Channel data rates:\n");

    qmi_message_wds_get_channel_rates_output_get_channel_rates (output,
                                                                &txrate,
                                                                &rxrate,
                                                                &maxtxrate,
                                                                &maxrxrate,
                                                                NULL);

    /* Current TX/RX rates may not be available if device is disconnected */

    g_print ("\tCurrent TX rate: ");
    if (txrate != QMI_WDS_RATE_UNAVAILABLE)
        g_print ("%ubps\n", txrate);
    else
        g_print ("n/a\n");

    g_print ("\tCurrent RX rate: ");
    if (rxrate != QMI_WDS_RATE_UNAVAILABLE)
        g_print ("%ubps\n", rxrate);
    else
        g_print ("n/a\n");

    g_print ("\tMax TX rate:     %ubps\n"
             "\tMax RX rate:     %ubps\n",
             maxtxrate,
             maxrxrate);

    qmi_message_wds_get_channel_rates_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_WDS_GET_CHANNEL_RATES */

#if defined HAVE_QMI_MESSAGE_WDS_GET_LTE_ATTACH_PARAMETERS

static void
get_lte_attach_parameters_ready (QmiClientWds *client,
                                 GAsyncResult *res)
{
    g_autoptr(QmiMessageWdsGetLteAttachParametersOutput)  output = NULL;
    g_autoptr(GError)                                     error = NULL;
    const gchar                                          *apn;
    gboolean                                              ota_attach_performed;
    QmiWdsIpSupportType                                   ip_support_type;

    output = qmi_client_wds_get_lte_attach_parameters_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_wds_get_lte_attach_parameters_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get LTE attach parameters: %s\n", error->message);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("LTE attach parameters successfully retrieved:\n");
    if (qmi_message_wds_get_lte_attach_parameters_output_get_apn (output, &apn, NULL))
        g_print ("\tAPN: %s\n", apn);
    if (qmi_message_wds_get_lte_attach_parameters_output_get_ip_support_type (output, &ip_support_type, NULL))
        g_print ("\tIP support type: %s\n", qmi_wds_ip_support_type_get_string (ip_support_type));
    if (qmi_message_wds_get_lte_attach_parameters_output_get_ota_attach_performed (output, &ota_attach_performed, NULL))
        g_print ("\tOTA attach performed: %s\n", ota_attach_performed ? "yes" : "no");

    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_WDS_GET_LTE_ATTACH_PARAMETERS */

#if defined HAVE_QMI_MESSAGE_WDS_GET_MAX_LTE_ATTACH_PDN_NUMBER

static void
get_max_lte_attach_pdn_number_ready (QmiClientWds *client,
                                     GAsyncResult *res)
{
    g_autoptr(QmiMessageWdsGetMaxLteAttachPdnNumberOutput) output = NULL;
    g_autoptr(GError)                                      error = NULL;
    guint8                                                 maxnum = 0;

    output = qmi_client_wds_get_max_lte_attach_pdn_number_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_wds_get_max_lte_attach_pdn_number_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get maximum number of attach PDN: %s\n", error->message);
        operation_shutdown (FALSE);
        return;
    }

    qmi_message_wds_get_max_lte_attach_pdn_number_output_get_info (output, &maxnum, NULL);
    g_print ("Maximum number of LTE attach PDN: %u\n", maxnum);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_WDS_GET_MAX_LTE_ATTACH_PDN_NUMBER */

#if defined HAVE_QMI_MESSAGE_WDS_GET_LTE_ATTACH_PDN_LIST

static void
get_lte_attach_pdn_list_ready (QmiClientWds *client,
                               GAsyncResult *res)
{
    g_autoptr(QmiMessageWdsGetLteAttachPdnListOutput) output = NULL;
    g_autoptr(GError)                                 error = NULL;
    GArray                                           *current_list = NULL;
    GArray                                           *pending_list = NULL;
    guint                                             i;

    output = qmi_client_wds_get_lte_attach_pdn_list_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_wds_get_lte_attach_pdn_list_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get the list of LTE attach PDN: %s\n", error->message);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("Attach PDN list retrieved:\n");

    qmi_message_wds_get_lte_attach_pdn_list_output_get_current_list (output, &current_list, NULL);
    if (!current_list || !current_list->len) {
        g_print ("\tCurrent list: n/a\n");
    } else {
        g_print ("\tCurrent list: '");
        for (i = 0; i < current_list->len; i++)
            g_print ("%s%u", i > 0 ? ", " : "", g_array_index (current_list, guint16, i));
        g_print ("'\n");
    }

    qmi_message_wds_get_lte_attach_pdn_list_output_get_pending_list (output, &pending_list, NULL);
    if (!pending_list || !pending_list->len) {
        g_print ("\tPending list: n/a\n");
    } else {
        g_print ("\tPending list: '");
        for (i = 0; i < pending_list->len; i++)
            g_print ("%s%u", i > 0 ? ", " : "", g_array_index (pending_list, guint16, i));
        g_print ("'\n");
    }

    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_WDS_GET_LTE_ATTACH_PDN_LIST */

#if defined HAVE_QMI_MESSAGE_WDS_SET_LTE_ATTACH_PDN_LIST

static QmiMessageWdsSetLteAttachPdnListInput *
set_lte_attach_pdn_list_input_create (const gchar *str)
{
    g_autoptr(QmiMessageWdsSetLteAttachPdnListInput) input = NULL;
    g_autoptr(GError)                                error = NULL;
    g_auto(GStrv)                                    split = NULL;
    g_autoptr(GArray)                                pdn_list = NULL;
    guint                                            i;

    pdn_list = g_array_new (FALSE, FALSE, sizeof (guint16));

    split = g_strsplit (str, ",", -1);

    for (i = 0; i < g_strv_length (split); i++) {
        guint    val = 0;
        guint16  profile_index;
        gboolean success;

        g_strstrip (split[i]);
        success = qmicli_read_uint_from_string (split[i], &val);
        if (!success || val == 0 || val > G_MAXUINT16) {
            g_printerr ("error: invalid or out of range profile number [1,%u]: '%s'\n", G_MAXUINT16, split[i]);
            operation_shutdown (FALSE);
            return NULL;
        }
        profile_index = (guint16) val;
        g_array_append_val (pdn_list, profile_index);
    }

    input = qmi_message_wds_set_lte_attach_pdn_list_input_new ();
    if (!qmi_message_wds_set_lte_attach_pdn_list_input_set_list (input, pdn_list, &error)) {
        g_printerr ("error: couldn't set attach PDN list: '%s'\n", error->message);
        return NULL;
    }
    if (!qmi_message_wds_set_lte_attach_pdn_list_input_set_action (input, QMI_WDS_ATTACH_PDN_LIST_ACTION_DETACH_OR_PDN_DISCONNECT, &error)) {
        g_printerr ("error: couldn't set attach PDN list action: '%s'\n", error->message);
        return NULL;
    }

    return g_steal_pointer (&input);
}

static void
set_lte_attach_pdn_list_ready (QmiClientWds *client,
                               GAsyncResult *res)
{
    g_autoptr(QmiMessageWdsSetLteAttachPdnListOutput) output = NULL;
    g_autoptr(GError)                                 error = NULL;

    output = qmi_client_wds_set_lte_attach_pdn_list_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_wds_set_lte_attach_pdn_list_output_get_result (output, &error)) {
        g_printerr ("error: couldn't set attach PDN list: %s\n", error->message);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("Attach PDN list update successfully requested\n");
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_WDS_SET_LTE_ATTACH_PDN_LIST */

void
qmicli_wds_run (QmiDevice *device,
                QmiClientWds *client,
                GCancellable *cancellable)
{
    /* Initialize context */
    ctx = g_slice_new (Context);
    ctx->device = g_object_ref (device);
    ctx->client = g_object_ref (client);
    ctx->cancellable = g_object_ref (cancellable);
    ctx->network_started_id = 0;
    ctx->packet_status_timeout_id = 0;

#if defined HAVE_QMI_MESSAGE_WDS_START_NETWORK
    if (start_network_str) {
        QmiMessageWdsStartNetworkInput *input = NULL;
        GError *error = NULL;

        if (!start_network_input_create (start_network_str, &input, &error)) {
            g_printerr ("error: %s\n", error->message);
            g_error_free (error);
            return;
        }

        g_debug ("Asynchronously starting network...");
        qmi_client_wds_start_network (ctx->client,
                                      input,
                                      180,
                                      ctx->cancellable,
                                      (GAsyncReadyCallback)start_network_ready,
                                      NULL);
        if (input)
            qmi_message_wds_start_network_input_unref (input);
        return;
    }
#endif /* HAVE_QMI_MESSAGE_WDS_START_NETWORK */

#if defined HAVE_QMI_MESSAGE_WDS_STOP_NETWORK
    if (stop_network_str) {
        gulong packet_data_handle;
        gboolean disable_autoconnect;

        if (g_str_equal (stop_network_str, "disable-autoconnect")) {
            packet_data_handle  = 0xFFFFFFFF;
            disable_autoconnect = TRUE;
        } else {
            disable_autoconnect = FALSE;
            if (g_str_has_prefix (stop_network_str, "0x"))
                packet_data_handle = strtoul (stop_network_str, NULL, 16);
            else
                packet_data_handle = strtoul (stop_network_str, NULL, 10);
            if (!packet_data_handle || packet_data_handle > G_MAXUINT32) {
                g_printerr ("error: invalid packet data handle given '%s'\n",
                            stop_network_str);
                operation_shutdown (FALSE);
                return;
            }
        }

        g_debug ("Asynchronously stopping network (%lu)...", packet_data_handle);
        internal_stop_network (ctx->cancellable, (guint32)packet_data_handle, disable_autoconnect);
        return;
    }
#endif /* HAVE_QMI_MESSAGE_WDS_STOP_NETWORK */

#if defined HAVE_QMI_MESSAGE_WDS_BIND_DATA_PORT
    if (bind_data_port_str) {
        g_autoptr(QmiMessageWdsBindDataPortInput) input = NULL;

        g_debug ("Binding data port...");
        input = bind_data_port_input_create (bind_data_port_str);
        if (!input) {
            operation_shutdown (FALSE);
            return;
        }
        qmi_client_wds_bind_data_port (client,
                                       input,
                                       10,
                                       ctx->cancellable,
                                       (GAsyncReadyCallback) bind_data_port_ready,
                                       NULL);
        return;
    }
#endif /* HAVE_QMI_MESSAGE_WDS_BIND_MUX_DATA_PORT */

#if defined HAVE_QMI_MESSAGE_WDS_BIND_MUX_DATA_PORT
    if (bind_mux_str) {
        QmiMessageWdsBindMuxDataPortInput *input;

        g_debug ("Binding mux data port..");
        input = bind_mux_data_port_input_create (bind_mux_str);
        qmi_client_wds_bind_mux_data_port (client,
                                           input,
                                           10,
                                           ctx->cancellable,
                                           (GAsyncReadyCallback) bind_mux_data_port_ready,
                                           NULL);
        qmi_message_wds_bind_mux_data_port_input_unref (input);
        return;
    }
#endif /* HAVE_QMI_MESSAGE_WDS_BIND_MUX_DATA_PORT */

#if defined HAVE_QMI_MESSAGE_WDS_SET_IP_FAMILY
    if (set_ip_family_str) {
        QmiMessageWdsSetIpFamilyInput *input;

        input = qmi_message_wds_set_ip_family_input_new ();
        switch (atoi (set_ip_family_str)) {
        case 4:
            qmi_message_wds_set_ip_family_input_set_preference (input, QMI_WDS_IP_FAMILY_IPV4, NULL);
            break;
        case 6:
            qmi_message_wds_set_ip_family_input_set_preference (input, QMI_WDS_IP_FAMILY_IPV6, NULL);
            break;
        default:
            g_printerr ("error: unknown IP type '%s' (not 4 or 6)\n",
                        set_ip_family_str);
            operation_shutdown (FALSE);
            return;
        }
        g_debug ("Asynchronously set IP family...");
        qmi_client_wds_set_ip_family (client,
                                      input,
                                      10,
                                      ctx->cancellable,
                                      (GAsyncReadyCallback) set_ip_family_ready,
                                      NULL);
        qmi_message_wds_set_ip_family_input_unref (input);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_WDS_GET_CURRENT_SETTINGS
    if (get_current_settings_flag) {
        QmiMessageWdsGetCurrentSettingsInput *input;

        input = qmi_message_wds_get_current_settings_input_new ();
        qmi_message_wds_get_current_settings_input_set_requested_settings (
            input,
            (QMI_WDS_GET_CURRENT_SETTINGS_REQUESTED_SETTINGS_DNS_ADDRESS      |
             QMI_WDS_GET_CURRENT_SETTINGS_REQUESTED_SETTINGS_GRANTED_QOS      |
             QMI_WDS_GET_CURRENT_SETTINGS_REQUESTED_SETTINGS_IP_ADDRESS       |
             QMI_WDS_GET_CURRENT_SETTINGS_REQUESTED_SETTINGS_GATEWAY_INFO     |
             QMI_WDS_GET_CURRENT_SETTINGS_REQUESTED_SETTINGS_MTU              |
             QMI_WDS_GET_CURRENT_SETTINGS_REQUESTED_SETTINGS_DOMAIN_NAME_LIST |
             QMI_WDS_GET_CURRENT_SETTINGS_REQUESTED_SETTINGS_IP_FAMILY),
            NULL);

        g_debug ("Asynchronously getting current settings...");
        qmi_client_wds_get_current_settings (client,
                                             input,
                                             10,
                                             ctx->cancellable,
                                             (GAsyncReadyCallback)get_current_settings_ready,
                                             NULL);
        qmi_message_wds_get_current_settings_input_unref (input);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_WDS_GET_PACKET_SERVICE_STATUS
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
#endif

#if defined HAVE_QMI_MESSAGE_WDS_GET_PACKET_STATISTICS
    if (get_packet_statistics_flag) {
        QmiMessageWdsGetPacketStatisticsInput *input;

        input = qmi_message_wds_get_packet_statistics_input_new ();
        qmi_message_wds_get_packet_statistics_input_set_mask (
            input,
            (QMI_WDS_PACKET_STATISTICS_MASK_FLAG_TX_PACKETS_OK      |
             QMI_WDS_PACKET_STATISTICS_MASK_FLAG_RX_PACKETS_OK      |
             QMI_WDS_PACKET_STATISTICS_MASK_FLAG_TX_PACKETS_ERROR   |
             QMI_WDS_PACKET_STATISTICS_MASK_FLAG_RX_PACKETS_ERROR   |
             QMI_WDS_PACKET_STATISTICS_MASK_FLAG_TX_OVERFLOWS       |
             QMI_WDS_PACKET_STATISTICS_MASK_FLAG_RX_OVERFLOWS       |
             QMI_WDS_PACKET_STATISTICS_MASK_FLAG_TX_BYTES_OK        |
             QMI_WDS_PACKET_STATISTICS_MASK_FLAG_RX_BYTES_OK        |
             QMI_WDS_PACKET_STATISTICS_MASK_FLAG_TX_PACKETS_DROPPED |
             QMI_WDS_PACKET_STATISTICS_MASK_FLAG_RX_PACKETS_DROPPED),
            NULL);

        g_debug ("Asynchronously getting packet statistics...");
        qmi_client_wds_get_packet_statistics (ctx->client,
                                              input,
                                              10,
                                              ctx->cancellable,
                                              (GAsyncReadyCallback)get_packet_statistics_ready,
                                              NULL);
        qmi_message_wds_get_packet_statistics_input_unref (input);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_WDS_GET_DATA_BEARER_TECHNOLOGY
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
#endif

#if defined HAVE_QMI_MESSAGE_WDS_GET_CURRENT_DATA_BEARER_TECHNOLOGY
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
#endif

#if defined HAVE_QMI_MESSAGE_WDS_GO_DORMANT
    if (go_dormant_flag) {
        g_debug ("Asynchronously going dormant...");
        qmi_client_wds_go_dormant (ctx->client,
                                   NULL,
                                   10,
                                   ctx->cancellable,
                                   (GAsyncReadyCallback)go_dormant_ready,
                                   NULL);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_WDS_GO_ACTIVE
    if (go_active_flag) {
        g_debug ("Asynchronously going active...");
        qmi_client_wds_go_active (ctx->client,
                                   NULL,
                                   10,
                                   ctx->cancellable,
                                   (GAsyncReadyCallback)go_active_ready,
                                   NULL);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_WDS_GET_DORMANCY_STATUS
    if (get_dormancy_status_flag) {
        g_debug ("Asynchronously getting dormancy status...");
        qmi_client_wds_get_dormancy_status (ctx->client,
                                            NULL,
                                            10,
                                            ctx->cancellable,
                                            (GAsyncReadyCallback)get_dormancy_status_ready,
                                            NULL);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_WDS_CREATE_PROFILE
    if (create_profile_str) {
        QmiMessageWdsCreateProfileInput *input = NULL;
        GError *error = NULL;

        if (!create_profile_input_create (create_profile_str, &input, &error)) {
            g_printerr ("error: %s\n", error->message);
            g_error_free (error);
            operation_shutdown (FALSE);
            return;
        }

        g_debug ("Asynchronously creating new profile...");
        qmi_client_wds_create_profile (ctx->client,
                                       input,
                                       10,
                                       ctx->cancellable,
                                       (GAsyncReadyCallback)create_profile_ready,
                                       NULL);
       qmi_message_wds_create_profile_input_unref (input);
       return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_WDS_SWI_CREATE_PROFILE_INDEXED
    if (swi_create_profile_indexed_str) {
        QmiMessageWdsSwiCreateProfileIndexedInput *input = NULL;
        GError *error = NULL;

        if (!swi_create_profile_indexed_input_create (swi_create_profile_indexed_str, &input, &error)) {
            g_printerr ("error: %s\n", error->message);
            g_error_free (error);
            operation_shutdown (FALSE);
            return;
        }

        g_debug ("Asynchronously creating new indexed profile...");
        qmi_client_wds_swi_create_profile_indexed (ctx->client,
                                                   input,
                                                   10,
                                                   ctx->cancellable,
                                                   (GAsyncReadyCallback)swi_create_profile_indexed_ready,
                                                   NULL);
       qmi_message_wds_swi_create_profile_indexed_input_unref (input);
       return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_WDS_MODIFY_PROFILE
    if (modify_profile_str) {
        QmiMessageWdsModifyProfileInput *input = NULL;
        GError *error = NULL;

        if (!modify_profile_input_create (modify_profile_str, &input, &error)) {
            g_printerr ("error: %s\n", error->message);
            g_error_free (error);
            operation_shutdown (FALSE);
            return;
        }

        g_debug ("Asynchronously modifying profile...");
        qmi_client_wds_modify_profile (ctx->client,
                                       input,
                                       10,
                                       ctx->cancellable,
                                       (GAsyncReadyCallback)modify_profile_ready,
                                       NULL);
        qmi_message_wds_modify_profile_input_unref (input);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_WDS_DELETE_PROFILE
    if (delete_profile_str) {
        QmiMessageWdsDeleteProfileInput *input;
        gchar **split;
        QmiWdsProfileType profile_type;
        guint profile_index;

        split = g_strsplit (delete_profile_str, ",", -1);
        input = qmi_message_wds_delete_profile_input_new ();

        if (g_strv_length (split) != 2) {
            g_printerr ("error: expected 2 arguments for delete profile command\n");
            operation_shutdown (FALSE);
            return;
        }

        g_strstrip (split[0]);
        if (g_str_equal (split[0], "3gpp"))
            profile_type = QMI_WDS_PROFILE_TYPE_3GPP;
        else if (g_str_equal (split[0], "3gpp2"))
            profile_type = QMI_WDS_PROFILE_TYPE_3GPP2;
        else {
            g_printerr ("error: invalid profile type '%s'. Expected '3gpp' or '3gpp2'.'\n",
                        split[0]);
            operation_shutdown (FALSE);
            return;
        }

        g_strstrip (split[1]);
        profile_index = atoi (split[1]);
        if (profile_index <= 0 || profile_index > G_MAXUINT8) {
            g_printerr ("error: invalid or out of range profile number [1,%u]: '%s'\n",
                        G_MAXUINT8,
                        split[1]);
            operation_shutdown (FALSE);
            return;
        }

        qmi_message_wds_delete_profile_input_set_profile_identifier (input, profile_type, (guint8)profile_index, NULL);

        g_strfreev (split);

        g_debug ("Asynchronously deleting new profile...");
        qmi_client_wds_delete_profile (ctx->client,
                                       input,
                                       10,
                                       ctx->cancellable,
                                       (GAsyncReadyCallback)delete_profile_ready,
                                       NULL);
       qmi_message_wds_delete_profile_input_unref (input);
       return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_WDS_GET_PROFILE_LIST && defined HAVE_QMI_MESSAGE_WDS_GET_PROFILE_SETTINGS
    if (get_profile_list_str) {
        QmiMessageWdsGetProfileListInput *input;

        input = qmi_message_wds_get_profile_list_input_new ();
        if (g_str_equal (get_profile_list_str, "3gpp"))
            qmi_message_wds_get_profile_list_input_set_profile_type (input, QMI_WDS_PROFILE_TYPE_3GPP, NULL);
        else if (g_str_equal (get_profile_list_str, "3gpp2"))
            qmi_message_wds_get_profile_list_input_set_profile_type (input, QMI_WDS_PROFILE_TYPE_3GPP2, NULL);
        else {
            g_printerr ("error: invalid profile type '%s'. Expected '3gpp' or '3gpp2'.'\n",
                        get_profile_list_str);
            operation_shutdown (FALSE);
            return;
        }

        g_debug ("Asynchronously get profile list...");
        qmi_client_wds_get_profile_list (ctx->client,
                                         input,
                                         10,
                                         ctx->cancellable,
                                         (GAsyncReadyCallback)get_profile_list_ready,
                                         NULL);
        qmi_message_wds_get_profile_list_input_unref (input);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_WDS_GET_DEFAULT_PROFILE_NUMBER
    if (get_default_profile_number_str || get_default_profile_num_str) {
        g_autoptr(QmiMessageWdsGetDefaultProfileNumberInput)  input = NULL;
        QmiWdsProfileType                                     profile_type;
        const gchar                                          *str;

        str = get_default_profile_number_str ? get_default_profile_number_str : get_default_profile_num_str;
        if (g_str_equal (str, "3gpp"))
            profile_type = QMI_WDS_PROFILE_TYPE_3GPP;
        else if (g_str_equal (str, "3gpp2"))
            profile_type = QMI_WDS_PROFILE_TYPE_3GPP2;
        else {
            g_printerr ("error: invalid profile type '%s'. Expected '3gpp' or '3gpp2'.'\n", str);
            operation_shutdown (FALSE);
            return;
        }

        input = qmi_message_wds_get_default_profile_number_input_new ();
        /* always use profile family 'tethered', we don't really know what it means */
        qmi_message_wds_get_default_profile_number_input_set_profile_type (input,
                                                                           profile_type,
                                                                           QMI_WDS_PROFILE_FAMILY_TETHERED,
                                                                           NULL);

        g_debug ("Asynchronously getting default profile number...");
        qmi_client_wds_get_default_profile_number (ctx->client,
                                                   input,
                                                   10,
                                                   ctx->cancellable,
                                                   (GAsyncReadyCallback)get_default_profile_number_ready,
                                                   NULL);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_WDS_SET_DEFAULT_PROFILE_NUMBER
    if (set_default_profile_number_str || set_default_profile_num_str) {
        g_autoptr(QmiMessageWdsSetDefaultProfileNumberInput) input = NULL;

        input = set_default_profile_number_input_create (set_default_profile_number_str ? set_default_profile_number_str : set_default_profile_num_str);
        if (!input) {
            operation_shutdown (FALSE);
            return;
        }

        g_debug ("Asynchronously setting default profile number...");
        qmi_client_wds_set_default_profile_number (ctx->client,
                                                   input,
                                                   10,
                                                   ctx->cancellable,
                                                   (GAsyncReadyCallback)set_default_profile_number_ready,
                                                   NULL);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_WDS_GET_DEFAULT_SETTINGS
    if (get_default_settings_str) {
        QmiMessageWdsGetDefaultSettingsInput *input;

        input = qmi_message_wds_get_default_settings_input_new ();
        if (g_str_equal (get_default_settings_str, "3gpp"))
            qmi_message_wds_get_default_settings_input_set_profile_type (input, QMI_WDS_PROFILE_TYPE_3GPP, NULL);
        else if (g_str_equal (get_default_settings_str, "3gpp2"))
            qmi_message_wds_get_default_settings_input_set_profile_type (input, QMI_WDS_PROFILE_TYPE_3GPP2, NULL);
        else {
            g_printerr ("error: invalid default type '%s'. Expected '3gpp' or '3gpp2'.'\n",
                        get_default_settings_str);
            operation_shutdown (FALSE);
            return;
        }

        g_debug ("Asynchronously get default settings...");
        qmi_client_wds_get_default_settings (ctx->client,
                                             input,
                                             10,
                                             ctx->cancellable,
                                             (GAsyncReadyCallback)get_default_settings_ready,
                                             NULL);
        qmi_message_wds_get_default_settings_input_unref (input);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_WDS_GET_AUTOCONNECT_SETTINGS
    if (get_autoconnect_settings_flag) {
        g_debug ("Asynchronously getting autoconnect settings...");
        qmi_client_wds_get_autoconnect_settings (ctx->client,
                                                 NULL,
                                                 10,
                                                 ctx->cancellable,
                                                 (GAsyncReadyCallback)get_autoconnect_settings_ready,
                                                 NULL);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_WDS_SET_AUTOCONNECT_SETTINGS
    if (set_autoconnect_settings_str) {
        QmiMessageWdsSetAutoconnectSettingsInput *input;

        input = set_autoconnect_settings_input_create (set_autoconnect_settings_str);
        if (!input) {
            operation_shutdown (FALSE);
            return;
        }

        g_debug ("Asynchronously set autoconnect settings...");
        qmi_client_wds_set_autoconnect_settings (ctx->client,
                                                 input,
                                                 10,
                                                 ctx->cancellable,
                                                 (GAsyncReadyCallback)set_autoconnect_settings_ready,
                                                 NULL);
        qmi_message_wds_set_autoconnect_settings_input_unref (input);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_WDS_GET_SUPPORTED_MESSAGES
    if (get_supported_messages_flag) {
        g_debug ("Asynchronously getting supported WDS messages...");
        qmi_client_wds_get_supported_messages (ctx->client,
                                               NULL,
                                               10,
                                               ctx->cancellable,
                                               (GAsyncReadyCallback)get_supported_messages_ready,
                                               NULL);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_WDS_RESET
    if (reset_flag) {
        g_debug ("Asynchronously resetting WDS service...");
        qmi_client_wds_reset (ctx->client,
                              NULL,
                              10,
                              ctx->cancellable,
                              (GAsyncReadyCallback)reset_ready,
                              NULL);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_WDS_GET_CHANNEL_RATES
    if (get_channel_rates_flag) {
        g_debug ("Asynchronously getting channel data rates...");
        qmi_client_wds_get_channel_rates (client,
                                          NULL,
                                          10,
                                          ctx->cancellable,
                                          (GAsyncReadyCallback)get_channel_rates_ready,
                                          NULL);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_WDS_GET_LTE_ATTACH_PARAMETERS
    if (get_lte_attach_parameters_flag) {
        g_debug ("Asynchronously getting LTE attach parameters...");
        qmi_client_wds_get_lte_attach_parameters (client,
                                                  NULL,
                                                  10,
                                                  ctx->cancellable,
                                                  (GAsyncReadyCallback)get_lte_attach_parameters_ready,
                                                  NULL);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_WDS_GET_MAX_LTE_ATTACH_PDN_NUMBER
    if (get_max_lte_attach_pdn_number_flag) {
        g_debug ("Asynchronously getting max LTE attach PDN number...");
        qmi_client_wds_get_max_lte_attach_pdn_number (client,
                                                      NULL,
                                                      10,
                                                      ctx->cancellable,
                                                      (GAsyncReadyCallback)get_max_lte_attach_pdn_number_ready,
                                                      NULL);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_WDS_GET_LTE_ATTACH_PDN_LIST
    if (get_lte_attach_pdn_list_flag) {
        g_debug ("Asynchronously getting LTE Attach PDN list...");
        qmi_client_wds_get_lte_attach_pdn_list (client,
                                                NULL,
                                                10,
                                                ctx->cancellable,
                                                (GAsyncReadyCallback)get_lte_attach_pdn_list_ready,
                                                NULL);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_WDS_SET_LTE_ATTACH_PDN_LIST
    if (set_lte_attach_pdn_list_str) {
        g_autoptr(QmiMessageWdsSetLteAttachPdnListInput) input = NULL;

        input = set_lte_attach_pdn_list_input_create (set_lte_attach_pdn_list_str);
        g_debug ("Asynchronously setting LTE Attach PDN list...");
        qmi_client_wds_set_lte_attach_pdn_list (client,
                                                input,
                                                10,
                                                ctx->cancellable,
                                                (GAsyncReadyCallback) set_lte_attach_pdn_list_ready,
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

#endif /* HAVE_QMI_SERVICE_WDS */
