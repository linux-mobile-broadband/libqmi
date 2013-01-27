/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * mbimcli -- Command line interface to control MBIM devices
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
 * Copyright (C) 2013 Aleksander Morgado <aleksander@gnu.org>
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <string.h>

#include <glib.h>
#include <gio/gio.h>

#include <libmbim-glib.h>

#include "mbimcli.h"

/* Context */
typedef struct {
    MbimDevice *device;
    GCancellable *cancellable;
} Context;
static Context *ctx;

/* Options */
static gboolean query_device_caps_flag;
static gboolean query_subscriber_ready_status_flag;
static gboolean query_device_services_flag;

static GOptionEntry entries[] = {
    { "basic-connect-query-device-caps", 0, 0, G_OPTION_ARG_NONE, &query_device_caps_flag,
      "Query device capabilities",
      NULL
    },
    { "basic-connect-query-subscriber-ready-status", 0, 0, G_OPTION_ARG_NONE, &query_subscriber_ready_status_flag,
      "Query subscriber ready status",
      NULL
    },
    { "basic-connect-query-device-services", 0, 0, G_OPTION_ARG_NONE, &query_device_services_flag,
      "Query device services",
      NULL
    },
    { NULL }
};

GOptionGroup *
mbimcli_basic_connect_get_option_group (void)
{
	GOptionGroup *group;

	group = g_option_group_new ("basic-connect",
	                            "Basic Connect options",
	                            "Show Basic Connect Service options",
	                            NULL,
	                            NULL);
	g_option_group_add_entries (group, entries);

	return group;
}

gboolean
mbimcli_basic_connect_options_enabled (void)
{
    static guint n_actions = 0;
    static gboolean checked = FALSE;

    if (checked)
        return !!n_actions;

    n_actions = (query_device_caps_flag +
                 query_subscriber_ready_status_flag +
                 query_device_services_flag);

    if (n_actions > 1) {
        g_printerr ("error: too many Basic Connect actions requested\n");
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
    g_slice_free (Context, context);
}

static void
shutdown (gboolean operation_status)
{
    /* Cleanup context and finish async operation */
    context_free (ctx);
    mbimcli_async_operation_done (operation_status);
}

static void
query_device_caps_ready (MbimDevice   *device,
                         GAsyncResult *res)
{
    MbimMessage *response;
    GError *error = NULL;
    gchar *cellular_class;
    gchar *sim_class;
    gchar *data_class;
    gchar *sms_caps;
    gchar *ctrl_caps;
    gchar *custom_data_class;
    gchar *device_id;
    gchar *firmware_info;
    gchar *hardware_info;

    response = mbim_device_command_finish (device, res, &error);
    if (!response) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        shutdown (FALSE);
        return;
    }

    cellular_class = (mbim_cellular_class_build_string_from_mask (
                          mbim_message_basic_connect_device_caps_query_response_get_cellular_class (response)));
    sim_class = (mbim_sim_class_build_string_from_mask (
                     mbim_message_basic_connect_device_caps_query_response_get_sim_class (response)));
    data_class = (mbim_data_class_build_string_from_mask (
                      mbim_message_basic_connect_device_caps_query_response_get_data_class (response)));
    sms_caps = (mbim_sms_caps_build_string_from_mask (
                      mbim_message_basic_connect_device_caps_query_response_get_sms_caps (response)));
    ctrl_caps = (mbim_ctrl_caps_build_string_from_mask (
                     mbim_message_basic_connect_device_caps_query_response_get_ctrl_caps (response)));

    custom_data_class = mbim_message_basic_connect_device_caps_query_response_get_custom_data_class (response);
    device_id = mbim_message_basic_connect_device_caps_query_response_get_device_id (response);
    firmware_info = mbim_message_basic_connect_device_caps_query_response_get_firmware_info (response);
    hardware_info = mbim_message_basic_connect_device_caps_query_response_get_hardware_info (response);

#undef VALIDATE_UNKNOWN
#define VALIDATE_UNKNOWN(str) (str ? str : "unknown")

    g_print ("[%s] Device capabilities retrieved:\n"
             "\t      Device type: '%s'\n"
             "\t   Cellular class: '%s'\n"
             "\t      Voice class: '%s'\n"
             "\t        Sim class: '%s'\n"
             "\t       Data class: '%s'\n"
             "\t         SMS caps: '%s'\n"
             "\t        Ctrl caps: '%s'\n"
             "\t     Max sessions: '%u'\n"
             "\tCustom data class: '%s'\n"
             "\t        Device ID: '%s'\n"
             "\t    Firmware info: '%s'\n"
             "\t    Hardware info: '%s'\n",
             mbim_device_get_path_display (device),
             VALIDATE_UNKNOWN (mbim_device_type_get_string (
                                   mbim_message_basic_connect_device_caps_query_response_get_device_type (response))),
             VALIDATE_UNKNOWN (cellular_class),
             VALIDATE_UNKNOWN (mbim_device_type_get_string (
                                   mbim_message_basic_connect_device_caps_query_response_get_voice_class (response))),
             VALIDATE_UNKNOWN (sim_class),
             VALIDATE_UNKNOWN (data_class),
             VALIDATE_UNKNOWN (sms_caps),
             VALIDATE_UNKNOWN (ctrl_caps),
             mbim_message_basic_connect_device_caps_query_response_get_max_sessions (response),
             VALIDATE_UNKNOWN (custom_data_class),
             VALIDATE_UNKNOWN (device_id),
             VALIDATE_UNKNOWN (firmware_info),
             VALIDATE_UNKNOWN (hardware_info));

    g_free (cellular_class);
    g_free (sim_class);
    g_free (data_class);
    g_free (sms_caps);
    g_free (ctrl_caps);
    g_free (custom_data_class);
    g_free (device_id);
    g_free (firmware_info);
    g_free (hardware_info);

    mbim_message_unref (response);
    shutdown (TRUE);
}

static void
query_subscriber_ready_status_ready (MbimDevice   *device,
                                     GAsyncResult *res)
{
    MbimMessage *response;
    GError *error = NULL;
    gchar *subscriber_id;
    gchar *sim_iccid;
    gchar *ready_info;
    gchar **telephone_numbers;
    gchar *telephone_numbers_str;

    response = mbim_device_command_finish (device, res, &error);
    if (!response) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        shutdown (FALSE);
        return;
    }

    ready_info = (mbim_ready_info_flag_build_string_from_mask (
                      mbim_message_basic_connect_subscriber_ready_status_query_response_get_ready_info (response)));

    subscriber_id = mbim_message_basic_connect_subscriber_ready_status_query_response_get_subscriber_id (response);
    sim_iccid = mbim_message_basic_connect_subscriber_ready_status_query_response_get_sim_iccid (response);

    telephone_numbers = mbim_message_basic_connect_subscriber_ready_status_query_response_get_telephone_numbers (response, NULL);
    telephone_numbers_str = (telephone_numbers ? g_strjoinv (", ", telephone_numbers) : NULL);

#undef VALIDATE_UNKNOWN
#define VALIDATE_UNKNOWN(str) (str ? str : "unknown")

    g_print ("[%s] Subscriber ready status retrieved:\n"
             "\t      Ready state: '%s'\n"
             "\t    Subscriber ID: '%s'\n"
             "\t        SIM ICCID: '%s'\n"
             "\t       Ready info: '%s'\n"
             "\tTelephone numbers: '%s'\n",
             mbim_device_get_path_display (device),
             VALIDATE_UNKNOWN (mbim_subscriber_ready_state_get_string (
                                   mbim_message_basic_connect_subscriber_ready_status_query_response_get_ready_state (response))),
             VALIDATE_UNKNOWN (subscriber_id),
             VALIDATE_UNKNOWN (sim_iccid),
             VALIDATE_UNKNOWN (ready_info),
             VALIDATE_UNKNOWN (telephone_numbers_str));

    g_free (subscriber_id);
    g_free (sim_iccid);
    g_free (ready_info);
    g_strfreev (telephone_numbers);
    g_free (telephone_numbers_str);

    mbim_message_unref (response);
    shutdown (TRUE);
}

static void
query_device_services_ready (MbimDevice   *device,
                             GAsyncResult *res)
{
    MbimMessage *response;
    GError *error = NULL;
    MbimDeviceServiceElement **device_services;
    guint32 n_device_services;

    response = mbim_device_command_finish (device, res, &error);
    if (!response) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        shutdown (FALSE);
        return;
    }

    device_services = mbim_message_basic_connect_device_services_query_response_get_device_services (response, &n_device_services);

    g_print ("[%s] Device services retrieved:\n"
             "\tMax DSS sessions: '%u'\n",
             mbim_device_get_path_display (device),
             mbim_message_basic_connect_device_services_query_response_get_max_dss_sessions (response));
    if (!device_services)
        g_print ("\t        Services: None\n");
    else {
        guint32 i;

        g_print ("\t        Services: (%u)\n", n_device_services);
        for (i = 0; i < n_device_services; i++) {
            MbimService service;
            gchar *uuid_str;
            GString *cids;
            guint32 j;

            service = mbim_uuid_to_service (&device_services[i]->device_service_id);
            uuid_str = mbim_uuid_get_printable (&device_services[i]->device_service_id);

            cids = g_string_new ("");
            for (j = 0; j < device_services[i]->cids_count; j++) {
                if (service == MBIM_SERVICE_INVALID) {
                    g_string_append_printf (cids, "%u", device_services[i]->cids[j]);
                    if (j < device_services[i]->cids_count - 1)
                        g_string_append (cids, ", ");
                } else {
                    g_string_append_printf (cids, "%s%s (%u)",
                                            j == 0 ? "" : "\t\t                   ",
                                            mbim_cid_get_printable (service, device_services[i]->cids[j]),
                                            device_services[i]->cids[j]);
                    if (j < device_services[i]->cids_count - 1)
                        g_string_append (cids, ",\n");
                }
            }

            g_print ("\n"
                     "\t\t          Service: '%s'\n"
                     "\t\t             UUID: [%s]:\n"
                     "\t\t      DSS payload: %u\n"
                     "\t\tMax DSS instances: %u\n"
                     "\t\t             CIDs: %s\n",
                     service == MBIM_SERVICE_INVALID ? "unknown" : mbim_service_get_string (service),
                     uuid_str,
                     device_services[i]->dss_payload,
                     device_services[i]->max_dss_instances,
                     cids->str);

            g_string_free (cids, TRUE);
            g_free (uuid_str);
        }
    }

    mbim_device_service_element_array_free (device_services);

    mbim_message_unref (response);
    shutdown (TRUE);
}

void
mbimcli_basic_connect_run (MbimDevice   *device,
                           GCancellable *cancellable)
{
    /* Initialize context */
    ctx = g_slice_new (Context);
    ctx->device = g_object_ref (device);
    if (cancellable)
        ctx->cancellable = g_object_ref (cancellable);

    /* Request to get capabilities? */
    if (query_device_caps_flag) {
        MbimMessage *request;

        g_debug ("Asynchronously querying device capabilities...");
        request = (mbim_message_basic_connect_device_caps_query_request_new (
                       mbim_device_get_next_transaction_id (ctx->device)));
        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)query_device_caps_ready,
                             NULL);
        mbim_message_unref (request);
        return;
    }

    /* Request to get subscriber ready status? */
    if (query_subscriber_ready_status_flag) {
        MbimMessage *request;

        g_debug ("Asynchronously querying subscriber ready status...");
        request = (mbim_message_basic_connect_subscriber_ready_status_query_request_new (
                       mbim_device_get_next_transaction_id (ctx->device)));
        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)query_subscriber_ready_status_ready,
                             NULL);
        mbim_message_unref (request);
        return;
    }

    /* Request to query device services? */
    if (query_device_services_flag) {
        MbimMessage *request;

        g_debug ("Asynchronously querying device services...");
        request = (mbim_message_basic_connect_device_services_query_request_new (
                       mbim_device_get_next_transaction_id (ctx->device)));
        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)query_device_services_ready,
                             NULL);
        mbim_message_unref (request);
        return;
    }

    g_warn_if_reached ();
}
