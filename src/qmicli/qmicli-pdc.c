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
 * Copyright (C) 2013-2015 Aleksander Morgado <aleksander@aleksander.es>
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <string.h>

#include <glib.h>
#include <glib/gprintf.h>
#include <gio/gio.h>

#include <libqmi-glib.h>

#include "qmicli.h"
#include "qmicli-helpers.h"

#define LOAD_CONFIG_CHUNK_SIZE 0x400

/* Info about config */
typedef struct {
    GArray *id;
    QmiPdcConfigurationType config_type;
    guint32 token;
    guint32 version;
    gchar *description;
    guint32 total_size;
} ConfigInfo;

/* Info about loading config */
typedef struct {
    GMappedFile *mapped_file;
    GArray *checksum;
    gsize offset;
} LoadConfigFileData;

/* Context */
typedef struct {
    QmiDevice *device;
    QmiClientPdc *client;
    GCancellable *cancellable;

    /* local data */
    GArray *config_list;
    gint configs_loaded;
    GArray *active_config_id;
    GArray *pending_config_id;
    gboolean ids_loaded;
    guint list_configs_indication_id;
    guint get_selected_config_indication_id;

    LoadConfigFileData *load_config_file_data;
    guint load_config_indication_id;
    guint get_config_info_indication_id;

    guint set_selected_config_indication_id;
    guint activate_config_indication_id;
    guint deactivate_config_indication_id;

    guint token;
} Context;
static Context *ctx;

/* Options */
static gchar *list_configs_str;
static gchar *delete_config_str;
static gchar *activate_config_str;
static gchar *deactivate_config_str;
static gchar *load_config_str;
static gboolean noop_flag;

static GOptionEntry entries[] = {
    {"pdc-list-configs", 0, 0, G_OPTION_ARG_STRING, &list_configs_str,
     "List all configs",
     "[(platform|software)]"},
    {"pdc-delete-config", 0, 0, G_OPTION_ARG_STRING, &delete_config_str,
     "Delete config",
     "[(platform|software),ConfigId]"},
    {"pdc-activate-config", 0, 0, G_OPTION_ARG_STRING, &activate_config_str,
     "Activate config",
     "[(platform|software),ConfigId]"},
    {"pdc-deactivate-config", 0, 0, G_OPTION_ARG_STRING, &deactivate_config_str,
     "Deactivate config",
     "[(platform|software),ConfigId]"},
    {"pdc-load-config", 0, 0, G_OPTION_ARG_STRING, &load_config_str,
     "Load config to device",
     "[Path to config]"},
    {"pdc-noop", 0, 0, G_OPTION_ARG_NONE, &noop_flag,
     "Just allocate or release a PDC client. Use with `--client-no-release-cid' and/or `--client-cid'",
     NULL},
    {NULL}
};

GOptionGroup *
qmicli_pdc_get_option_group (
    void)
{
    GOptionGroup *group;

    group = g_option_group_new ("pdc",
                                "PDC options",
                                "Show platform device configurations options", NULL, NULL);
    g_option_group_add_entries (group, entries);

    return group;
}

gboolean
qmicli_pdc_options_enabled (
    void)
{
    static guint n_actions = 0;
    static gboolean checked = FALSE;

    if (checked)
        return !!n_actions;

    n_actions = !!list_configs_str +
        !!delete_config_str +
        !!activate_config_str + !!deactivate_config_str + !!load_config_str + noop_flag;

    if (n_actions > 1) {
        g_printerr ("error: too many PDC actions requested\n");
        exit (EXIT_FAILURE);
    }

    checked = TRUE;
    return !!n_actions;
}

static Context *
context_new (
    QmiDevice * device,
    QmiClientPdc * client,
    GCancellable * cancellable)
{
    Context *context;

    context = g_slice_new (Context);
    context->device = g_object_ref (device);
    context->client = g_object_ref (client);
    context->cancellable = g_object_ref (cancellable);

    context->config_list = NULL;
    context->configs_loaded = 0;
    context->active_config_id = NULL;
    context->pending_config_id = NULL;
    context->ids_loaded = FALSE;
    context->list_configs_indication_id = 0;
    context->get_selected_config_indication_id = 0;

    context->load_config_file_data = NULL;
    context->load_config_indication_id = 0;
    context->get_config_info_indication_id = 0;

    context->set_selected_config_indication_id = 0;
    context->activate_config_indication_id = 0;
    context->deactivate_config_indication_id = 0;

    context->token = 0;
    return context;
}

static void
context_free (
    Context * context)
{
    int i;
    ConfigInfo *current_config;

    if (!context)
        return;

    if (context->config_list) {
        for (i = 0; i < context->config_list->len; i++) {
            current_config = &g_array_index (context->config_list, ConfigInfo, i);
            if (current_config->description)
                g_free (current_config->description);
            if (current_config->id)
                g_array_unref (current_config->id);
        }
        g_array_unref (context->config_list);
        g_signal_handler_disconnect (context->client, context->list_configs_indication_id);
        g_signal_handler_disconnect (context->client, context->get_config_info_indication_id);
        g_signal_handler_disconnect (context->client, context->get_selected_config_indication_id);
    }
    if (context->load_config_file_data) {
        g_array_unref (context->load_config_file_data->checksum);
        g_mapped_file_unref (context->load_config_file_data->mapped_file);
        g_slice_free (LoadConfigFileData, context->load_config_file_data);
        g_signal_handler_disconnect (context->client, context->load_config_indication_id);
    }
    if (context->client)
        g_object_unref (context->client);
    if (context->set_selected_config_indication_id) {
        g_signal_handler_disconnect (context->client, context->set_selected_config_indication_id);
    }
    if (context->activate_config_indication_id) {
        g_signal_handler_disconnect (context->client, context->activate_config_indication_id);
    }
    if (context->deactivate_config_indication_id) {
        g_signal_handler_disconnect (context->client, context->deactivate_config_indication_id);
    }
    g_object_unref (context->cancellable);
    g_object_unref (context->device);

    g_slice_free (Context, context);
}

static void
operation_shutdown (
    gboolean operation_status)
{
    /* Cleanup context and finish async operation */
    context_free (ctx);
    ctx = NULL;
    qmicli_async_operation_done (operation_status);
}

/****************************************************************************************
 * List configs
 ****************************************************************************************/
static const char *
printable_config_type (
    QmiPdcConfigurationType type)
{
    if (type == QMI_PDC_CONFIGURATION_TYPE_PLATFORM) {
        return "Platform";
    } else if (type == QMI_PDC_CONFIGURATION_TYPE_SOFTWARE) {
        return "Software";
    } else {
        return "Unknown";
    }
}

static char *
printable_id (
    GArray * id)
{
    if (!id) {
        return g_strdup ("Id is NULL?");
    } else if (id->len == 0) {
        return g_strdup ("Id is empty");
    } else {
        GString *printable;
        int i;

        printable = g_string_new ("");
        for (i = 0; i < id->len; i++) {
            guint8 ch = g_array_index (id, guint8, i);

            g_string_append_printf (printable, "%02hhX", ch);
        }
        return g_string_free (printable, FALSE);
    }
}

static gboolean
is_array_equals (
    GArray * a,
    GArray * b)
{
    int i;

    if (a == NULL && b == NULL) {
        return TRUE;
    } else if (a == NULL || b == NULL) {
        return FALSE;
    } else if (a->len != b->len) {
        return FALSE;
    }
    for (i = 0; i < a->len; i++) {
        if (g_array_index (a, guint8, i) != g_array_index (b, guint8, i)) {
            return FALSE;
        }
    }

    return TRUE;
}

static const
char *status_string (
    GArray * id)
{
    if (!id) {
        return "UNKNOWN";
    } else if (is_array_equals (id, ctx->active_config_id)) {
        return "Active";
    } else if (is_array_equals (id, ctx->pending_config_id)) {
        return "Pending";
    }
    return "Inactive";
}

static void
print_configs (
    GArray * configs)
{
    ConfigInfo *current_config = NULL;
    int i;

    g_printf ("CONFIGS: %d\n", ctx->config_list->len);
    for (i = 0; i < ctx->config_list->len; i++) {
        char *id_str;

        current_config = &g_array_index (ctx->config_list, ConfigInfo, i);

        g_printf ("START_CONFIG\n");
        g_printf ("Config Index : %i\n", i);
        g_printf ("Description  : %s\n", current_config->description);
        g_printf ("Type         : %s\n", printable_config_type (current_config->config_type));
        g_printf ("Size         : %u\n", current_config->total_size);
        g_printf ("Status       : %s\n", status_string (current_config->id));
        g_printf ("Version      : 0x%x\n", current_config->version);
        id_str = printable_id (current_config->id);
        g_printf ("ID           : %s\n", id_str);
        g_free (id_str);
        g_printf ("END_CONFIG\n\n");

    }
}

static void
get_config_info_ready (
    QmiClientPdc * client,
    GAsyncResult * res)
{
    GError *error = NULL;
    QmiMessagePdcGetConfigInfoOutput *output;

    output = qmi_client_pdc_get_config_info_finish (client, res, &error);

    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_pdc_get_config_info_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get config info: %s\n", error->message);
        g_error_free (error);
        qmi_message_pdc_get_config_info_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    qmi_message_pdc_get_config_info_output_unref (output);
}

static void
get_config_info_ready_indication (
    QmiClientPdc * client,
    QmiIndicationPdcGetConfigInfoOutput * output)
{
    GError *error = NULL;
    ConfigInfo *current_config = NULL;
    guint32 token;
    const gchar *description;
    int i;
    guint16 error_code;

    if (!qmi_indication_pdc_get_config_info_output_get_indication_result (output,
                                                                          &error_code, &error)) {
        g_printerr ("error: couldn't get config info: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    } else if (error_code != 0) {
        g_printerr ("error: couldn't get config info: %s\n",
                    qmi_protocol_error_get_string ((QmiProtocolError) error_code));
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_indication_pdc_get_config_info_output_get_token (output, &token, &error)) {
        g_printerr ("error: couldn't get config info: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    for (i = 0; i < ctx->config_list->len; i++) {
        current_config = &g_array_index (ctx->config_list, ConfigInfo, i);
        if (current_config->token == token)
            break;
    }

    if (!qmi_indication_pdc_get_config_info_output_get_total_size (output,
                                                                   &current_config->total_size,
                                                                   &error) ||
        !qmi_indication_pdc_get_config_info_output_get_version (output,
                                                                &current_config->version,
                                                                &error) ||
        !qmi_indication_pdc_get_config_info_output_get_description (output, &description, &error)) {
        g_printerr ("error: couldn't get config info: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }
    current_config->description = g_strdup (description);

    ctx->configs_loaded++;
    if (ctx->configs_loaded == ctx->config_list->len && ctx->ids_loaded) {
        print_configs (ctx->config_list);
        operation_shutdown (TRUE);
    }
}

static void
list_configs_ready (
    QmiClientPdc * client,
    GAsyncResult * res)
{
    GError *error = NULL;
    QmiMessagePdcListConfigsOutput *output;

    output = qmi_client_pdc_list_configs_finish (client, res, &error);

    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_pdc_list_configs_output_get_result (output, &error)) {
        g_printerr ("error: couldn't list configs: %s\n", error->message);
        g_error_free (error);
        qmi_message_pdc_list_configs_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    qmi_message_pdc_list_configs_output_unref (output);
}

static void
list_configs_ready_indication (
    QmiClientPdc * client,
    QmiIndicationPdcListConfigsOutput * output)
{
    GError *error = NULL;
    GArray *configs = NULL;
    int i;
    guint16 error_code;

    if (!qmi_indication_pdc_list_configs_output_get_indication_result (output, &error_code, &error)) {
        g_printerr ("error: couldn't list configs: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    } else if (error_code != 0) {
        g_printerr ("error: couldn't list config: %s\n",
                    qmi_protocol_error_get_string ((QmiProtocolError) error_code));
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_indication_pdc_list_configs_output_get_configs (output, &configs, &error)) {
        g_printerr ("error: couldn't list configs: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    ctx->config_list = g_array_sized_new (FALSE, TRUE, sizeof (ConfigInfo), configs->len);
    g_array_set_size (ctx->config_list, configs->len);
    for (i = 0; i < configs->len; i++) {
        ConfigInfo *current_info;
        QmiIndicationPdcListConfigsOutputConfigsElement *element;
        QmiConfigTypeAndId type_with_id;
        QmiMessagePdcGetConfigInfoInput *input;
        guint32 token = ctx->token++;

        element = &g_array_index (configs, QmiIndicationPdcListConfigsOutputConfigsElement, i);

        current_info = &g_array_index (ctx->config_list, ConfigInfo, i);

        input = qmi_message_pdc_get_config_info_input_new ();
        current_info->token = token;
        current_info->id = g_array_ref (element->id);
        current_info->config_type = element->config_type;
        type_with_id.config_type = element->config_type;
        type_with_id.id = current_info->id;
        g_printerr ("Fetching config type: %d\n", current_info->config_type);
        if (!qmi_message_pdc_get_config_info_input_set_type_with_id (input, &type_with_id, &error)) {
            g_printerr ("error: couldn't set type with id: %s\n", error->message);
            g_error_free (error);
            operation_shutdown (FALSE);
            return;
        }
        if (!qmi_message_pdc_get_config_info_input_set_token (input, token, &error)) {
            g_printerr ("error: couldn't set token: %s\n", error->message);
            g_error_free (error);
            operation_shutdown (FALSE);
            return;
        }

        qmi_client_pdc_get_config_info (ctx->client,
                                        input,
                                        10,
                                        ctx->cancellable,
                                        (GAsyncReadyCallback) get_config_info_ready, NULL);

    }

    g_print ("Loaded configs: %d\n", ctx->config_list->len);
    if (ctx->configs_loaded == ctx->config_list->len && ctx->ids_loaded) {
        print_configs (ctx->config_list);
        operation_shutdown (TRUE);
    }
}

static void
get_selected_config_ready (
    QmiClientPdc * client,
    GAsyncResult * res)
{
    GError *error = NULL;
    QmiMessagePdcGetSelectedConfigOutput *output;

    output = qmi_client_pdc_get_selected_config_finish (client, res, &error);

    if (!qmi_message_pdc_get_selected_config_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get selected config: %s\n", error->message);
        g_error_free (error);
        qmi_message_pdc_get_selected_config_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    qmi_message_pdc_get_selected_config_output_unref (output);
}

static void
get_selected_config_ready_indication (
    QmiClientPdc * client,
    QmiIndicationPdcGetSelectedConfigOutput * output)
{
    GArray *pending_id = NULL;
    GArray *active_id = NULL;
    GError *error = NULL;
    guint16 error_code;

    if (!qmi_indication_pdc_get_selected_config_output_get_indication_result (output,
                                                                              &error_code,
                                                                              &error)) {
        g_printerr ("error: couldn't get selected config: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    } else if (error_code != 0) {
        g_printerr ("error: couldn't get selected config: %s\n",
                    qmi_protocol_error_get_string ((QmiProtocolError) error_code));
    }

    qmi_indication_pdc_get_selected_config_output_get_pending_id (output, &pending_id, NULL);
    qmi_indication_pdc_get_selected_config_output_get_active_id (output, &active_id, NULL);
    if (active_id) {
        ctx->active_config_id = g_array_ref (active_id);
    }
    if (pending_id) {
        ctx->pending_config_id = g_array_ref (pending_id);
    }

    ctx->ids_loaded = TRUE;
    if (ctx->configs_loaded == ctx->config_list->len && ctx->ids_loaded) {
        print_configs (ctx->config_list);
        operation_shutdown (TRUE);
    }
}

static void
activate_config_ready_indication (
    QmiClientPdc * client,
    QmiIndicationPdcActivateConfigOutput * output)
{
    GError *error = NULL;
    guint16 error_code;

    if (!qmi_indication_pdc_activate_config_output_get_indication_result (output,
                                                                          &error_code, &error)) {
        g_printerr ("error: couldn't activate config: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    } else if (error_code != 0) {
        g_printerr ("error: couldn't activate config: %s\n",
                    qmi_protocol_error_get_string ((QmiProtocolError) error_code));
        operation_shutdown (FALSE);
    }

    operation_shutdown (TRUE);
}

static void
deactivate_config_ready_indication (
    QmiClientPdc * client,
    QmiIndicationPdcDeactivateConfigOutput * output)
{
    GError *error = NULL;
    guint16 error_code;

    if (!qmi_indication_pdc_deactivate_config_output_get_indication_result (output,
                                                                            &error_code, &error)) {
        g_printerr ("error: couldn't deactivate config: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    } else if (error_code != 0) {
        g_printerr ("error: couldn't deactivate config: %s\n",
                    qmi_protocol_error_get_string ((QmiProtocolError) error_code));
        operation_shutdown (FALSE);
    }

    operation_shutdown (TRUE);
}

static QmiMessagePdcListConfigsInput *
list_configs_input_create (
    const gchar * str)
{
    QmiMessagePdcListConfigsInput *input = NULL;
    QmiPdcConfigurationType configType;

    if (qmicli_read_pdc_configuration_type_from_string (str, &configType)) {
        GError *error = NULL;

        input = qmi_message_pdc_list_configs_input_new ();
        if (!qmi_message_pdc_list_configs_input_set_config_type (input, configType, &error)) {
            g_printerr ("error: couldn't create input data bundle: '%s'\n", error->message);
            g_error_free (error);
            qmi_message_pdc_list_configs_input_unref (input);
            input = NULL;
        }
        if (!qmi_message_pdc_list_configs_input_set_token (input, ctx->token++, &error)) {
            g_printerr ("error: couldn't create input data bundle: '%s'\n", error->message);
            g_error_free (error);
            qmi_message_pdc_list_configs_input_unref (input);
            input = NULL;
        }

    }

    return input;
}

/****************************************************************************************
 * Activate and deactivate configs
 ****************************************************************************************/
static QmiConfigTypeAndId *
parse_type_and_id (
    const gchar * str)
{
    GArray *id = NULL;
    int num_parts;
    gchar **substrings = g_strsplit (str, ",", -1);
    QmiPdcConfigurationType config_type;
    QmiConfigTypeAndId *result = NULL;

    for (num_parts = 0; substrings[num_parts]; num_parts++) ;
    if (num_parts != 2) {
        g_printerr ("Expected 2 parameters, but found: '%d'\n", num_parts);
        g_strfreev (substrings);
        return NULL;
    }

    if (!qmicli_read_pdc_configuration_type_from_string (substrings[0], &config_type)) {
        g_printerr ("Incorrect id specified: %s\n", substrings[0]);
        g_strfreev (substrings);
        return NULL;
    }

    if (!qmicli_read_binary_array_from_string (substrings[1], &id)) {
        g_printerr ("Incorrect config type specified: %s\n", substrings[1]);
        g_strfreev (substrings);
        return NULL;
    }

    result = g_slice_new0 (QmiConfigTypeAndId);
    if (!result) {
        g_printerr ("Error allocating QmiConfigTypeAndId\n");
        g_strfreev (substrings);
        g_array_unref (id);
        return NULL;
    }
    result->config_type = config_type;
    result->id = id;

    return result;
}

static QmiMessagePdcGetSelectedConfigInput *
get_selected_config_input_create (
    const gchar * str)
{
    QmiMessagePdcGetSelectedConfigInput *input = NULL;
    QmiPdcConfigurationType configType;

    if (qmicli_read_pdc_configuration_type_from_string (str, &configType)) {
        GError *error = NULL;

        input = qmi_message_pdc_get_selected_config_input_new ();
        if (!qmi_message_pdc_get_selected_config_input_set_config_type (input, configType, &error)) {
            g_printerr ("error: couldn't create input data bundle: '%s'\n", error->message);
            g_error_free (error);
            qmi_message_pdc_get_selected_config_input_unref (input);
            input = NULL;
        }
        if (!qmi_message_pdc_get_selected_config_input_set_token (input, ctx->token++, &error)) {
            g_printerr ("error: couldn't create input data bundle: '%s'\n", error->message);
            g_error_free (error);
            qmi_message_pdc_get_selected_config_input_unref (input);
            input = NULL;
        }

    }

    return input;
}

static QmiMessagePdcDeleteConfigInput *
delete_config_input_create (
    const gchar * str)
{
    QmiMessagePdcDeleteConfigInput *input = NULL;
    QmiConfigTypeAndId *type_and_id;

    type_and_id = parse_type_and_id (str);

    if (type_and_id) {
        GError *error = NULL;

        input = qmi_message_pdc_delete_config_input_new ();
        if (!qmi_message_pdc_delete_config_input_set_config_type (input,
                                                                  type_and_id->config_type,
                                                                  &error) ||
            !qmi_message_pdc_delete_config_input_set_token (input,
                                                            ctx->token++,
                                                            &error) ||
            !qmi_message_pdc_delete_config_input_set_id (input, type_and_id->id, &error)) {
            g_printerr ("error: couldn't create input data bundle: '%s'\n", error->message);
            g_error_free (error);
            g_free (type_and_id);
            qmi_message_pdc_delete_config_input_unref (input);
            input = NULL;
        }
        g_slice_free (QmiConfigTypeAndId, type_and_id);
    }

    return input;
}

static void
delete_config_ready (
    QmiClientPdc * client,
    GAsyncResult * res)
{
    GError *error = NULL;
    QmiMessagePdcDeleteConfigOutput *output;

    output = qmi_client_pdc_delete_config_finish (client, res, &error);

    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_pdc_delete_config_output_get_result (output, &error)) {
        g_printerr ("error: couldn't delete config: %s\n", error->message);
        g_error_free (error);
        qmi_message_pdc_delete_config_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    operation_shutdown (TRUE);
    qmi_message_pdc_delete_config_output_unref (output);
}

static QmiMessagePdcActivateConfigInput *
activate_config_input_create (
    const gchar * str)
{
    QmiMessagePdcActivateConfigInput *input = NULL;
    QmiConfigTypeAndId *type_and_id;

    type_and_id = parse_type_and_id (str);

    if (type_and_id) {
        GError *error = NULL;

        input = qmi_message_pdc_activate_config_input_new ();
        if (!qmi_message_pdc_activate_config_input_set_config_type (input,
                                                                    type_and_id->config_type,
                                                                    &error)) {
            g_printerr ("error: couldn't create input data bundle: '%s'\n", error->message);
            g_error_free (error);
            qmi_message_pdc_activate_config_input_unref (input);
            g_free (type_and_id);
            input = NULL;
        }
        if (!qmi_message_pdc_activate_config_input_set_token (input, ctx->token++, &error)) {
            g_printerr ("error: couldn't create input data bundle: '%s'\n", error->message);
            g_error_free (error);
            qmi_message_pdc_activate_config_input_unref (input);
            g_free (type_and_id);
            input = NULL;
        }
    }

    return input;
}

static void
activate_config_ready (
    QmiClientPdc * client,
    GAsyncResult * res)
{
    GError *error = NULL;
    QmiMessagePdcActivateConfigOutput *output;

    output = qmi_client_pdc_activate_config_finish (client, res, &error);

    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_pdc_activate_config_output_get_result (output, &error)) {
        g_printerr ("error: couldn't activate config: %s\n", error->message);
        g_error_free (error);
        qmi_message_pdc_activate_config_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    qmi_message_pdc_activate_config_output_unref (output);
}

static void
deactivate_config_ready (
    QmiClientPdc * client,
    GAsyncResult * res)
{
    GError *error = NULL;
    QmiMessagePdcDeactivateConfigOutput *output;

    output = qmi_client_pdc_deactivate_config_finish (client, res, &error);

    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_pdc_deactivate_config_output_get_result (output, &error)) {
        g_printerr ("error: couldn't activate config: %s\n", error->message);
        g_error_free (error);
        qmi_message_pdc_deactivate_config_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    qmi_message_pdc_deactivate_config_output_unref (output);
}

static void
set_selected_config_ready_indication_activation (
    QmiClientPdc * client,
    QmiIndicationPdcSetSelectedConfigOutput * output)
{
    GError *error = NULL;
    QmiMessagePdcActivateConfigInput *input;
    guint16 error_code;

    if (!qmi_indication_pdc_set_selected_config_output_get_indication_result (output,
                                                                              &error_code,
                                                                              &error)) {
        g_printerr ("error: couldn't set selected config: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    } else if (error_code != 0) {
        g_printerr ("error: couldn't set selected config: %s\n",
                    qmi_protocol_error_get_string ((QmiProtocolError) error_code));
        operation_shutdown (FALSE);
        return;
    }

    input = activate_config_input_create (activate_config_str);
    if (!input) {
        operation_shutdown (FALSE);
        return;
    }
    qmi_client_pdc_activate_config (ctx->client,
                                    input,
                                    10,
                                    ctx->cancellable,
                                    (GAsyncReadyCallback) activate_config_ready, NULL);
    qmi_message_pdc_activate_config_input_unref (input);
}

static void
set_selected_config_ready (
    QmiClientPdc * client,
    GAsyncResult * res)
{
    GError *error = NULL;
    QmiMessagePdcSetSelectedConfigOutput *output;

    output = qmi_client_pdc_set_selected_config_finish (client, res, &error);

    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    qmi_message_pdc_set_selected_config_output_unref (output);
}

static QmiMessagePdcDeactivateConfigInput *
deactivate_config_input_create (
    const gchar * str)
{
    QmiMessagePdcDeactivateConfigInput *input = NULL;
    QmiConfigTypeAndId *type_and_id;

    type_and_id = parse_type_and_id (str);

    if (type_and_id) {
        GError *error = NULL;

        input = qmi_message_pdc_deactivate_config_input_new ();
        if (!qmi_message_pdc_deactivate_config_input_set_config_type (input,
                                                                      type_and_id->config_type,
                                                                      &error)) {
            g_printerr ("error: couldn't create input data bundle: '%s'\n", error->message);
            g_error_free (error);
            qmi_message_pdc_deactivate_config_input_unref (input);
            g_free (type_and_id);
            input = NULL;
        }
        if (!qmi_message_pdc_deactivate_config_input_set_token (input, ctx->token++, &error)) {
            g_printerr ("error: couldn't create input data bundle: '%s'\n", error->message);
            g_error_free (error);
            qmi_message_pdc_deactivate_config_input_unref (input);
            g_free (type_and_id);
            input = NULL;
        }
    }

    return input;
}

static void
set_selected_config_ready_indication_deactivation (
    QmiClientPdc * client,
    QmiIndicationPdcSetSelectedConfigOutput * output)
{
    GError *error = NULL;
    QmiMessagePdcDeactivateConfigInput *input;
    guint16 error_code;

    if (!qmi_indication_pdc_set_selected_config_output_get_indication_result (output,
                                                                              &error_code,
                                                                              &error)) {
        g_printerr ("error: couldn't set selected config: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    } else if (error_code != 0) {
        g_printerr ("error: couldn't set selected config: %s\n",
                    qmi_protocol_error_get_string ((QmiProtocolError) error_code));
        operation_shutdown (FALSE);
        return;
    }

    input = deactivate_config_input_create (activate_config_str);
    if (!input) {
        operation_shutdown (FALSE);
        return;
    }
    qmi_client_pdc_deactivate_config (ctx->client,
                                      input,
                                      10,
                                      ctx->cancellable,
                                      (GAsyncReadyCallback) deactivate_config_ready, NULL);
    qmi_message_pdc_deactivate_config_input_unref (input);
}

static QmiMessagePdcSetSelectedConfigInput *
set_selected_config_input_create (
    const gchar * str)
{
    QmiMessagePdcSetSelectedConfigInput *input = NULL;
    QmiConfigTypeAndId *type_and_id;

    type_and_id = parse_type_and_id (str);

    if (type_and_id) {
        GError *error = NULL;

        input = qmi_message_pdc_set_selected_config_input_new ();
        if (!qmi_message_pdc_set_selected_config_input_set_type_with_id (input,
                                                                         type_and_id, &error)) {
            g_printerr ("error: couldn't create input data bundle: '%s'\n", error->message);
            g_error_free (error);
            g_free (type_and_id);
            qmi_message_pdc_set_selected_config_input_unref (input);
            input = NULL;
        }
        if (!qmi_message_pdc_set_selected_config_input_set_token (input, ctx->token++, &error)) {
            g_printerr ("error: couldn't create input data bundle: '%s'\n", error->message);
            g_error_free (error);
            g_free (type_and_id);
            qmi_message_pdc_set_selected_config_input_unref (input);
            input = NULL;
        }

    }

    return input;
}

/****************************************************************************************
 * Load config
 ****************************************************************************************/

static LoadConfigFileData *
load_config_file_from_string (
    const gchar * str)
{
    GError *error = NULL;
    GMappedFile *mapped_file;
    LoadConfigFileData *data;
    guchar *file_contents;
    GChecksum *checksum;
    gsize file_size;
    gsize hash_size;

    if (!(mapped_file = g_mapped_file_new (str, FALSE, &error))) {
        g_printerr ("error: couldn't map config file: '%s'\n", error->message);
        g_error_free (error);
        return NULL;
    }

    if (!(file_contents = (guchar *) g_mapped_file_get_contents (mapped_file))) {
        g_printerr ("error: couldn't get file content\n");
        g_mapped_file_unref (mapped_file);
        return NULL;
    }
    file_size = g_mapped_file_get_length (mapped_file);
    hash_size = g_checksum_type_get_length (G_CHECKSUM_SHA1);
    g_info ("File opened: %lu, %lu", file_size, hash_size);
    checksum = g_checksum_new (G_CHECKSUM_SHA1);
    g_checksum_update (checksum, file_contents, file_size);

    data = g_slice_new (LoadConfigFileData);
    data->mapped_file = mapped_file;
    data->checksum = g_array_sized_new (FALSE, FALSE, sizeof (guint8), hash_size);
    g_array_set_size (data->checksum, hash_size);
    data->offset = 0;
    g_checksum_get_digest (checksum, &g_array_index (data->checksum, guint8, 0), &hash_size);
    g_info ("Checksum: %lu, %lu", file_size, hash_size);

    return data;
}

static QmiMessagePdcLoadConfigInput *
load_config_input_create_chunk (
    LoadConfigFileData * config_file)
{
    QmiMessagePdcLoadConfigInput *input = NULL;

    if (config_file) {
        GError *error = NULL;
        GArray *chunk;
        gsize full_size;
        gsize chunk_size;
        guchar *file_content;

        input = qmi_message_pdc_load_config_input_new ();
        if (!qmi_message_pdc_load_config_input_set_token (input, ctx->token++, &error)) {
            g_printerr ("error: couldn't set token: '%s'\n", error->message);
            g_error_free (error);
            qmi_message_pdc_load_config_input_unref (input);
            input = NULL;
        }

        chunk = g_array_new (FALSE, FALSE, 1);
        full_size = g_mapped_file_get_length (config_file->mapped_file);
        chunk_size = config_file->offset + LOAD_CONFIG_CHUNK_SIZE > full_size ?
            full_size - config_file->offset : LOAD_CONFIG_CHUNK_SIZE;
        file_content = (guchar *) g_mapped_file_get_contents (config_file->mapped_file);
        g_array_append_vals (chunk, file_content + config_file->offset, chunk_size);
        g_print ("Uploaded %lu of %lu\n", config_file->offset, full_size);

        if (!qmi_message_pdc_load_config_input_set_config_chunk (input,
                                                                 QMI_PDC_CONFIGURATION_TYPE_SOFTWARE,
                                                                 config_file->checksum,
                                                                 g_mapped_file_get_length
                                                                 (config_file->mapped_file), chunk,
                                                                 &error)) {
            g_printerr ("error: couldn't set chunk: '%s'\n", error->message);
            g_error_free (error);
            qmi_message_pdc_load_config_input_unref (input);
            input = NULL;
        } else {
            config_file->offset += chunk_size;
        }
    }
    return input;
}

static QmiMessagePdcLoadConfigInput *
load_config_input_create (
    const gchar * str)
{
    LoadConfigFileData *config_file;
    QmiMessagePdcLoadConfigInput *input = NULL;

    if ((config_file = load_config_file_from_string (str))) {
        if ((input = load_config_input_create_chunk (config_file))) {
            ctx->load_config_file_data = config_file;
        }
    }

    return input;
}

static void
load_config_ready (
    QmiClientPdc * client,
    GAsyncResult * res)
{
    GError *error = NULL;
    QmiMessagePdcLoadConfigOutput *output;

    output = qmi_client_pdc_load_config_finish (client, res, &error);

    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_pdc_load_config_output_get_result (output, &error)) {
        g_printerr ("error: couldn't load config: %s\n", error->message);
        g_error_free (error);
        qmi_message_pdc_load_config_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    qmi_message_pdc_load_config_output_unref (output);
}

static void
load_config_ready_indication (
    QmiClientPdc * client,
    QmiIndicationPdcLoadConfigOutput * output)
{
    GError *error = NULL;
    QmiMessagePdcLoadConfigInput *input;
    gboolean frame_reset;
    guint32 remaining_size;
    guint16 error_code;

    if (!qmi_indication_pdc_load_config_output_get_indication_result (output, &error_code, &error)) {
        g_printerr ("error: couldn't load config: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    } else if (error_code != 0) {
        g_printerr ("error: couldn't load config: %s\n",
                    qmi_protocol_error_get_string ((QmiProtocolError) error_code));
        operation_shutdown (FALSE);
        return;
    }

    if (qmi_indication_pdc_load_config_output_get_frame_reset (output, &frame_reset, &error)) {
        if (frame_reset) {
            g_printerr ("error: frame reset requested\n");
            operation_shutdown (FALSE);
            return;
        }
    } else {
        g_error_free (error);
        error = NULL;
    }

    if (!qmi_indication_pdc_load_config_output_get_remaining_size (output, &remaining_size, &error)) {
        g_printerr ("error: couldn't load config: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("Remaining %d\n", remaining_size);
    if (remaining_size > 0) {
        g_print ("Loading next\n");
        input = load_config_input_create_chunk (ctx->load_config_file_data);
        if (!input) {
            g_printerr ("Input is null\n");
            operation_shutdown (FALSE);
            return;
        }
        qmi_client_pdc_load_config (ctx->client,
                                    input,
                                    10,
                                    ctx->cancellable,
                                    (GAsyncReadyCallback) load_config_ready, NULL);
        qmi_message_pdc_load_config_input_unref (input);
    } else {
        operation_shutdown (TRUE);
    }
}

/****************************************************************************************
 * Common logic
 ****************************************************************************************/

static gboolean
noop_cb (
    gpointer unused)
{
    operation_shutdown (TRUE);
    return FALSE;
}

void
qmicli_pdc_run (
    QmiDevice * device,
    QmiClientPdc * client,
    GCancellable * cancellable)
{
    /* Initialize context */
    ctx = context_new (device, client, cancellable);

    /* Request to get all configs */
    if (list_configs_str) {
        QmiMessagePdcListConfigsInput *input;
        QmiMessagePdcGetSelectedConfigInput *get_selected_config_input;

        g_debug ("Listing configs asynchroniously...");
        ctx->list_configs_indication_id = g_signal_connect (client,
                                                            "list-configs",
                                                            G_CALLBACK
                                                            (list_configs_ready_indication), NULL);
        ctx->get_selected_config_indication_id =
            g_signal_connect (client, "get-selected-config",
                              G_CALLBACK (get_selected_config_ready_indication), NULL);
        ctx->get_config_info_indication_id =
            g_signal_connect (client, "get-config-info",
                              G_CALLBACK (get_config_info_ready_indication), NULL);
        input = list_configs_input_create (list_configs_str);
        if (!input) {
            operation_shutdown (FALSE);
            return;
        }
        get_selected_config_input = get_selected_config_input_create (list_configs_str);
        if (!get_selected_config_input) {
            operation_shutdown (FALSE);
            return;
        }

        qmi_client_pdc_list_configs (ctx->client,
                                     input,
                                     10,
                                     ctx->cancellable,
                                     (GAsyncReadyCallback) list_configs_ready, NULL);
        qmi_message_pdc_list_configs_input_unref (input);

        qmi_client_pdc_get_selected_config (ctx->client,
                                            get_selected_config_input,
                                            10,
                                            ctx->cancellable,
                                            (GAsyncReadyCallback) get_selected_config_ready, NULL);
        qmi_message_pdc_get_selected_config_input_unref (get_selected_config_input);
        return;
    }

    /* Request to delete config */
    if (delete_config_str) {
        QmiMessagePdcDeleteConfigInput *input;

        g_debug ("Deleting config asynchroniously...");
        input = delete_config_input_create (delete_config_str);
        if (!input) {
            operation_shutdown (FALSE);
            return;
        }
        qmi_client_pdc_delete_config (ctx->client,
                                      input,
                                      10,
                                      ctx->cancellable,
                                      (GAsyncReadyCallback) delete_config_ready, NULL);
        qmi_message_pdc_delete_config_input_unref (input);
        return;
    }

    /* Request to activate config */
    if (activate_config_str) {
        QmiMessagePdcSetSelectedConfigInput *input;

        g_debug ("Activating config asynchroniously...");
        input = set_selected_config_input_create (activate_config_str);
        if (!input) {
            operation_shutdown (FALSE);
            return;
        }
        ctx->set_selected_config_indication_id =
            g_signal_connect (client,
                              "set-selected-config",
                              G_CALLBACK (set_selected_config_ready_indication_activation), NULL);
        ctx->activate_config_indication_id = g_signal_connect (client,
                                                               "activate-config",
                                                               G_CALLBACK
                                                               (activate_config_ready_indication),
                                                               NULL);
        qmi_client_pdc_set_selected_config (ctx->client, input, 10, ctx->cancellable,
                                            (GAsyncReadyCallback) set_selected_config_ready, NULL);
        qmi_message_pdc_set_selected_config_input_unref (input);
        return;
    }

    /* Request to deactivate config */
    if (deactivate_config_str) {
        QmiMessagePdcSetSelectedConfigInput *input;

        g_debug ("Deactivating config asynchroniously...");
        input = set_selected_config_input_create (activate_config_str);
        if (!input) {
            operation_shutdown (FALSE);
            return;
        }
        ctx->set_selected_config_indication_id =
            g_signal_connect (client,
                              "set-selected-config",
                              G_CALLBACK (set_selected_config_ready_indication_deactivation), NULL);
        ctx->deactivate_config_indication_id = g_signal_connect (client,
                                                                 "deactivate-config",
                                                                 G_CALLBACK
                                                                 (deactivate_config_ready_indication),
                                                                 NULL);
        qmi_client_pdc_set_selected_config (ctx->client, input, 10, ctx->cancellable,
                                            (GAsyncReadyCallback) set_selected_config_ready, NULL);
        qmi_message_pdc_set_selected_config_input_unref (input);
        return;
    }

    if (load_config_str) {
        QmiMessagePdcLoadConfigInput *input;

        g_debug ("Loading config asynchroniously...");
        input = load_config_input_create (load_config_str);
        if (!input) {
            operation_shutdown (FALSE);
            return;
        }
        ctx->load_config_indication_id = g_signal_connect (client,
                                                           "load-config",
                                                           G_CALLBACK
                                                           (load_config_ready_indication), NULL);
        qmi_client_pdc_load_config (ctx->client, input, 10, ctx->cancellable,
                                    (GAsyncReadyCallback) load_config_ready, NULL);
        qmi_message_pdc_load_config_input_unref (input);
        return;
    }

    /* Just client allocate/release? */
    if (noop_flag) {
        g_idle_add (noop_cb, NULL);
        return;
    }

    g_warn_if_reached ();
}
