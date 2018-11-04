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
 * Copyright (C) 2018 Google LLC
 * Copyright (C) 2018 Aleksander Morgado <aleksander@aleksander.es>
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <string.h>
#include <errno.h>

#include <glib.h>
#include <gio/gio.h>

#include <libmbim-glib.h>

#include "mbim-common.h"
#include "mbimcli.h"

/* Context */
typedef struct {
    MbimDevice *device;
    GCancellable *cancellable;
} Context;
static Context *ctx;

/* Options */
static gchar    *query_pco_str;
static gboolean  query_lte_attach_configuration_flag;
static gboolean  query_lte_attach_status_flag;

static gboolean query_pco_arg_parse (const char *option_name,
                                     const char *value,
                                     gpointer user_data,
                                     GError **error);

static GOptionEntry entries[] = {
    { "ms-query-pco", 0, G_OPTION_FLAG_OPTIONAL_ARG, G_OPTION_ARG_CALLBACK, G_CALLBACK (query_pco_arg_parse),
      "Query PCO value (SessionID is optional, defaults to 0)",
      "[SessionID]"
    },
    { "ms-query-lte-attach-configuration", 0, 0, G_OPTION_ARG_NONE, &query_lte_attach_configuration_flag,
      "Query LTE attach configuration",
      NULL
    },
    { "ms-query-lte-attach-status", 0, 0, G_OPTION_ARG_NONE, &query_lte_attach_status_flag,
      "Query LTE attach status",
      NULL
    },
    { NULL }
};

static gboolean
query_pco_arg_parse (const char *option_name,
                     const char *value,
                     gpointer user_data,
                     GError **error)
{
    query_pco_str = g_strdup (value ? value : "0");
    return TRUE;
}

GOptionGroup *
mbimcli_ms_basic_connect_extensions_get_option_group (void)
{
   GOptionGroup *group;

   group = g_option_group_new ("ms-basic-connect-extensions",
                               "Microsoft Basic Connect Extensions options",
                               "Show Microsoft Basic Connect Extensions Service options",
                               NULL,
                               NULL);
   g_option_group_add_entries (group, entries);

   return group;
}

static gboolean
session_id_parse (const gchar  *str,
                  guint32      *session_id,
                  GError      **error)
{
    gchar *endptr = NULL;
    gint64 n;

    g_assert (str != NULL);
    g_assert (session_id != NULL);

    if (!str[0]) {
        *session_id = 0;
        return TRUE;
    }

    errno = 0;
    n = g_ascii_strtoll (str, &endptr, 10);
    if (errno || n < 0 || n > 255 || ((endptr - str) < strlen (str))) {
        g_set_error (error,
                     MBIM_CORE_ERROR,
                     MBIM_CORE_ERROR_FAILED,
                     "couldn't parse session ID '%s' (must be 0 - 255)",
                     str);
        return FALSE;
    }
    *session_id = (guint32) n;

    return TRUE;
}

gboolean
mbimcli_ms_basic_connect_extensions_options_enabled (void)
{
    static guint n_actions = 0;
    static gboolean checked = FALSE;

    if (checked)
        return !!n_actions;

    n_actions = (!!query_pco_str +
                 query_lte_attach_configuration_flag +
                 query_lte_attach_status_flag);

    if (n_actions > 1) {
        g_printerr ("error: too many Microsoft Basic Connect Extensions Service actions requested\n");
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
query_pco_ready (MbimDevice   *device,
                 GAsyncResult *res)
{
    MbimMessage *response;
    GError *error = NULL;
    MbimPcoValue *pco_value;
    gchar *pco_data;

    response = mbim_device_command_finish (device, res, &error);
    if (!response || !mbim_message_response_get_result (response, MBIM_MESSAGE_TYPE_COMMAND_DONE, &error)) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        if (response)
            mbim_message_unref (response);
        shutdown (FALSE);
        return;
    }

    g_print ("[%s] Successfully queried PCO\n\n",
             mbim_device_get_path_display (device));
    if (!mbim_message_ms_basic_connect_extensions_pco_response_parse (
            response,
            &pco_value,
            &error)) {
        g_printerr ("error: couldn't parse response message: %s\n", error->message);
        g_error_free (error);
        mbim_message_unref (response);
        shutdown (FALSE);
        return;
    }

    pco_data = mbim_common_str_hex (pco_value->pco_data_buffer, pco_value->pco_data_size, ' ');
    g_print ("[%s] PCO:\n"
             "\t   Session ID: '%u'\n"
             "\tPCO data type: '%s'\n"
             "\tPCO data size: '%u'\n"
             "\t     PCO data: '%s'\n",
             mbim_device_get_path_display (device),
             pco_value->session_id,
             VALIDATE_UNKNOWN (mbim_pco_type_get_string (pco_value->pco_data_type)),
             pco_value->pco_data_size,
             pco_data);
    g_free (pco_data);
    mbim_pco_value_free (pco_value);

    mbim_message_unref (response);
    shutdown (TRUE);
}

static void
query_lte_attach_configuration_ready (MbimDevice   *device,
                                      GAsyncResult *res)
{
    MbimMessage                 *response;
    GError                      *error = NULL;
    guint32                      configuration_count = 0;
    MbimLteAttachConfiguration **configurations = NULL;
    guint                        i;

    response = mbim_device_command_finish (device, res, &error);
    if (!response || !mbim_message_response_get_result (response, MBIM_MESSAGE_TYPE_COMMAND_DONE, &error)) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        if (response)
            mbim_message_unref (response);
        shutdown (FALSE);
        return;
    }

    g_print ("[%s] Successfully queried LTE attach configuration\n",
             mbim_device_get_path_display (device));

    if (!mbim_message_ms_basic_connect_extensions_lte_attach_configuration_response_parse (
               response,
               &configuration_count,
               &configurations,
               &error)) {
        g_printerr ("error: couldn't parse response message: %s\n", error->message);
        g_error_free (error);
        mbim_message_unref (response);
        shutdown (FALSE);
        return;
    }

#define VALIDATE_NA(str) (str ? str : "n/a")
    for (i = 0; i < configuration_count; i++) {
        g_print ("Configuration %u:\n", i);
        g_print ("  IP type:       %s\n", mbim_context_ip_type_get_string (configurations[i]->ip_type));
        g_print ("  Roaming:       %s\n", mbim_lte_attach_context_roaming_control_get_string (configurations[i]->roaming));
        g_print ("  Source:        %s\n", mbim_context_source_get_string (configurations[i]->source));
        g_print ("  Access string: %s\n", VALIDATE_NA (configurations[i]->access_string));
        g_print ("  Username:      %s\n", VALIDATE_NA (configurations[i]->user_name));
        g_print ("  Password:      %s\n", VALIDATE_NA (configurations[i]->password));
        g_print ("  Compression:   %s\n", mbim_compression_get_string (configurations[i]->compression));
        g_print ("  Auth protocol: %s\n", mbim_auth_protocol_get_string (configurations[i]->auth_protocol));
    }
#undef VALIDATE_NA

    mbim_lte_attach_configuration_array_free (configurations);
    mbim_message_unref (response);
    shutdown (TRUE);
}

static void
query_lte_attach_status_ready (MbimDevice   *device,
                               GAsyncResult *res)
{
    MbimMessage         *response;
    GError              *error = NULL;
    MbimLteAttachStatus *lte_attach_status = NULL;

    response = mbim_device_command_finish (device, res, &error);
    if (!response || !mbim_message_response_get_result (response, MBIM_MESSAGE_TYPE_COMMAND_DONE, &error)) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        if (response)
            mbim_message_unref (response);
        shutdown (FALSE);
        return;
    }

    g_print ("[%s] Successfully queried LTE attach status\n",
             mbim_device_get_path_display (device));

    if (!mbim_message_ms_basic_connect_extensions_lte_attach_status_response_parse (
            response,
            &lte_attach_status,
            &error)) {
        g_printerr ("error: couldn't parse response message: %s\n", error->message);
        g_error_free (error);
        mbim_message_unref (response);
        shutdown (FALSE);
        return;
    }

#define VALIDATE_NA(str) (str ? str : "n/a")
    g_print ("  Attach state:  %s\n", mbim_lte_attach_state_get_string (lte_attach_status->lte_attach_state));
    g_print ("  IP type:       %s\n", mbim_context_ip_type_get_string (lte_attach_status->ip_type));
    g_print ("  Access string: %s\n", VALIDATE_NA (lte_attach_status->access_string));
    g_print ("  Username:      %s\n", VALIDATE_NA (lte_attach_status->user_name));
    g_print ("  Password:      %s\n", VALIDATE_NA (lte_attach_status->password));
    g_print ("  Compression:   %s\n", mbim_compression_get_string (lte_attach_status->compression));
    g_print ("  Auth protocol: %s\n", mbim_auth_protocol_get_string (lte_attach_status->auth_protocol));
#undef VALIDATE_NA

    mbim_lte_attach_status_free (lte_attach_status);
    mbim_message_unref (response);
    shutdown (TRUE);
}

void
mbimcli_ms_basic_connect_extensions_run (MbimDevice   *device,
                                         GCancellable *cancellable)
{
    /* Initialize context */
    ctx = g_slice_new (Context);
    ctx->device = g_object_ref (device);
    ctx->cancellable = cancellable ? g_object_ref (cancellable) : NULL;

    /* Request to get PCO? */
    if (query_pco_str) {
        MbimMessage *request;
        MbimPcoValue pco_value;
        GError *error = NULL;

        if (!session_id_parse (query_pco_str, &pco_value.session_id, &error)) {
            g_printerr ("error: couldn't parse session ID: %s\n", error->message);
            g_error_free (error);
            shutdown (FALSE);
            return;
        }

        pco_value.pco_data_size = 0;
        pco_value.pco_data_type = MBIM_PCO_TYPE_COMPLETE;
        pco_value.pco_data_buffer = NULL;

        g_debug ("Asynchronously querying PCO...");
        request = mbim_message_ms_basic_connect_extensions_pco_query_new (&pco_value, NULL);
        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)query_pco_ready,
                             NULL);
        mbim_message_unref (request);
        return;
    }

    /* Request to query LTE attach configuration? */
    if (query_lte_attach_configuration_flag) {
        MbimMessage *request;

        g_debug ("Asynchronously querying LTE attach configuration...");
        request = mbim_message_ms_basic_connect_extensions_lte_attach_configuration_query_new (NULL);
        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)query_lte_attach_configuration_ready,
                             NULL);
        mbim_message_unref (request);
        return;
    }

    /* Request to query LTE attach status? */
    if (query_lte_attach_status_flag) {
        MbimMessage *request;

        g_debug ("Asynchronously querying LTE attach status...");
        request = mbim_message_ms_basic_connect_extensions_lte_attach_status_query_new (NULL);
        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)query_lte_attach_status_ready,
                             NULL);
        mbim_message_unref (request);
        return;
    }

    g_warn_if_reached ();
}
