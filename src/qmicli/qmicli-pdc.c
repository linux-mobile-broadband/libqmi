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
 * Copyright (C) 2013-2020 Aleksander Morgado <aleksander@aleksander.es>
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

#if defined HAVE_QMI_SERVICE_PDC

#define LIST_CONFIGS_TIMEOUT_SECS 2
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
    gboolean skip_cid_release;

    /* local data */
    guint timeout_id;
    GArray *config_list;
    guint configs_loaded;
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
    guint device_removed_indication_id;

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

#if defined HAVE_QMI_MESSAGE_PDC_LIST_CONFIGS && \
    defined HAVE_QMI_INDICATION_PDC_LIST_CONFIGS && \
    defined HAVE_QMI_MESSAGE_PDC_GET_SELECTED_CONFIG && \
    defined HAVE_QMI_INDICATION_PDC_GET_SELECTED_CONFIG && \
    defined HAVE_QMI_MESSAGE_PDC_GET_CONFIG_INFO && \
    defined HAVE_QMI_INDICATION_PDC_GET_CONFIG_INFO
# define HAVE_QMI_ACTION_PDC_LIST_CONFIGS
#endif

#if defined HAVE_QMI_MESSAGE_PDC_ACTIVATE_CONFIG && \
    defined HAVE_QMI_INDICATION_PDC_ACTIVATE_CONFIG && \
    defined HAVE_QMI_MESSAGE_PDC_SET_SELECTED_CONFIG && \
    defined HAVE_QMI_INDICATION_PDC_SET_SELECTED_CONFIG
# define HAVE_QMI_ACTION_PDC_ACTIVATE_CONFIG
#endif

#if defined HAVE_QMI_MESSAGE_PDC_DEACTIVATE_CONFIG && \
    defined HAVE_QMI_INDICATION_PDC_DEACTIVATE_CONFIG
# define HAVE_QMI_ACTION_PDC_DEACTIVATE_CONFIG
#endif

#if defined HAVE_QMI_MESSAGE_PDC_LOAD_CONFIG && \
    defined HAVE_QMI_INDICATION_PDC_LOAD_CONFIG
# define HAVE_QMI_ACTION_PDC_LOAD_CONFIG
#endif

static GOptionEntry entries[] = {
#if defined HAVE_QMI_ACTION_PDC_LIST_CONFIGS
    {
        "pdc-list-configs", 0, 0, G_OPTION_ARG_STRING, &list_configs_str,
        "List all configs",
        "[(platform|software)]"
    },
#endif
#if defined HAVE_QMI_MESSAGE_PDC_DELETE_CONFIG
    {
        "pdc-delete-config", 0, 0, G_OPTION_ARG_STRING, &delete_config_str,
        "Delete config",
        "[(platform|software),ConfigId]"
    },
#endif
#if defined HAVE_QMI_ACTION_PDC_ACTIVATE_CONFIG
    {
        "pdc-activate-config", 0, 0, G_OPTION_ARG_STRING, &activate_config_str,
        "Activate config",
        "[(platform|software),ConfigId]"
    },
#endif
#if defined HAVE_QMI_ACTION_PDC_DEACTIVATE_CONFIG
    {
        "pdc-deactivate-config", 0, 0, G_OPTION_ARG_STRING, &deactivate_config_str,
        "Deactivate config",
        "[(platform|software),ConfigId]"
    },
#endif
#if defined HAVE_QMI_ACTION_PDC_LOAD_CONFIG
    {
        "pdc-load-config", 0, 0, G_OPTION_ARG_STRING, &load_config_str,
        "Load config to device",
        "[Path to config]"
    },
#endif
    {
        "pdc-noop", 0, 0, G_OPTION_ARG_NONE, &noop_flag,
        "Just allocate or release a PDC client. Use with `--client-no-release-cid' and/or `--client-cid'",
        NULL
    },
    { NULL }
};

GOptionGroup *
qmicli_pdc_get_option_group (void)
{
    GOptionGroup *group;

    group = g_option_group_new ("pdc",
                                "PDC options:",
                                "Show platform device configurations options", NULL, NULL);
    g_option_group_add_entries (group, entries);

    return group;
}

gboolean
qmicli_pdc_options_enabled (void)
{
    static guint n_actions = 0;
    static gboolean checked = FALSE;

    if (checked)
        return !!n_actions;

    n_actions = (!!list_configs_str +
                 !!delete_config_str +
                 !!activate_config_str +
                 !!deactivate_config_str +
                 !!load_config_str +
                 noop_flag);

    if (n_actions > 1) {
        g_printerr ("error: too many PDC actions requested\n");
        exit (EXIT_FAILURE);
    }

    /* Actions that require receiving QMI indication messages must specify that
     * indications are expected. */
    if (list_configs_str || activate_config_str || deactivate_config_str || load_config_str)
        qmicli_expect_indications ();

    checked = TRUE;
    return !!n_actions;
}

static Context *
context_new (QmiDevice *device,
             QmiClientPdc *client,
             GCancellable *cancellable)
{
    Context *context;

    context = g_slice_new0 (Context);
    context->device = g_object_ref (device);
    context->client = g_object_ref (client);
    context->cancellable = g_object_ref (cancellable);
    return context;
}

static void
context_free (Context *context)
{
    guint i;

    if (!context)
        return;

    if (context->config_list) {
        for (i = 0; i < context->config_list->len; i++) {
            ConfigInfo *current_config;

            current_config = &g_array_index (context->config_list, ConfigInfo, i);
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

    if (context->set_selected_config_indication_id)
        g_signal_handler_disconnect (context->client, context->set_selected_config_indication_id);

    if (context->activate_config_indication_id)
        g_signal_handler_disconnect (context->client, context->activate_config_indication_id);

    if (context->device_removed_indication_id)
        g_signal_handler_disconnect (context->device, context->device_removed_indication_id);

    if (context->deactivate_config_indication_id)
        g_signal_handler_disconnect (context->client, context->deactivate_config_indication_id);

    g_object_unref (context->cancellable);
    g_object_unref (context->client);
    g_object_unref (context->device);

    g_slice_free (Context, context);
}

static void
operation_shutdown (gboolean operation_status)
{
    /* Cleanup context and finish async operation */
    qmicli_async_operation_done (operation_status, ctx->skip_cid_release);
    context_free (ctx);
    ctx = NULL;
}

/******************************************************************************/
/* Common */

#if defined HAVE_QMI_ACTION_PDC_ACTIVATE_CONFIG || \
    defined HAVE_QMI_ACTION_PDC_DEACTIVATE_CONFIG || \
    defined HAVE_QMI_MESSAGE_PDC_DELETE_CONFIG

static QmiConfigTypeAndId *
parse_type_and_id (const gchar *str)
{
    GArray *id = NULL;
    guint num_parts;
    gchar **substrings;
    QmiPdcConfigurationType config_type;
    QmiConfigTypeAndId *result = NULL;

    substrings = g_strsplit (str, ",", -1);
    num_parts = g_strv_length (substrings);

    if (num_parts != 2) {
        g_printerr ("Expected 2 parameters, but found %u\n", num_parts);
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
    result->config_type = config_type;
    result->id = id;
    return result;
}

#endif /* HAVE_QMI_ACTION_PDC_ACTIVATE_CONFIG
        * HAVE_QMI_ACTION_PDC_DEACTIVATE_CONFIG
        * HAVE_QMI_MESSAGE_PDC_DELETE_CONFIG */

/******************************************************************************/
/* List configs and get selected config */

#if defined HAVE_QMI_ACTION_PDC_LIST_CONFIGS

static const char *
status_string (GArray *id)
{
    if (!id)
        return "Unknown";
    if (ctx->active_config_id &&
        id->len == ctx->active_config_id->len &&
        memcmp (id->data, ctx->active_config_id->data, id->len) == 0)
        return "Active";
    if (ctx->pending_config_id &&
        id->len == ctx->pending_config_id->len &&
        memcmp (id->data, ctx->pending_config_id->data, id->len) == 0)
        return "Pending";
    return "Inactive";
}

static void
print_configs (GArray *configs)
{
    guint i;

    g_printf ("Total configurations: %u\n", ctx->config_list->len);
    for (i = 0; i < ctx->config_list->len; i++) {
        ConfigInfo *current_config;
        char *id_str;

        current_config = &g_array_index (ctx->config_list, ConfigInfo, i);

        g_printf ("Configuration %u:\n", i + 1);
        g_printf ("\tDescription: %s\n", current_config->description);
        g_printf ("\tType:        %s\n", qmi_pdc_configuration_type_get_string (current_config->config_type));
        g_printf ("\tSize:        %u\n", current_config->total_size);
        g_printf ("\tStatus:      %s\n", status_string (current_config->id));
        g_printf ("\tVersion:     0x%X\n", current_config->version);
        id_str = qmicli_get_raw_data_printable (current_config->id, 80, "");
        g_printf ("\tID:          %s\n", id_str ? id_str : "none");
        g_free (id_str);
    }
}

static void
check_list_config_completed (void)
{
    if (ctx->configs_loaded == ctx->config_list->len &&
        ctx->ids_loaded) {
        print_configs (ctx->config_list);
        operation_shutdown (TRUE);
    }
}

static void
get_config_info_ready (QmiClientPdc *client,
                       GAsyncResult *res)
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
get_config_info_ready_indication (QmiClientPdc *client,
                                  QmiIndicationPdcGetConfigInfoOutput *output)
{
    GError *error = NULL;
    ConfigInfo *current_config = NULL;
    guint32 token;
    const gchar *description;
    guint i;
    guint16 error_code = 0;

    if (!qmi_indication_pdc_get_config_info_output_get_indication_result (output, &error_code, &error)) {
        g_printerr ("error: couldn't get config info: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (error_code != 0) {
        g_printerr ("error: couldn't get config info: %s\n",
                    qmi_protocol_error_get_string ((QmiProtocolError) error_code));
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_indication_pdc_get_config_info_output_get_token (output, &token, &error)) {
        g_printerr ("error: couldn't get config info token: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    /* Look for the current config in the list */
    for (i = 0; i < ctx->config_list->len; i++) {
        current_config = &g_array_index (ctx->config_list, ConfigInfo, i);
        if (current_config->token == token)
            break;
    }

    /* Store total size, version and description of the current config */
    if (!qmi_indication_pdc_get_config_info_output_get_total_size (output,
                                                                   &current_config->total_size,
                                                                   &error) ||
        !qmi_indication_pdc_get_config_info_output_get_version (output,
                                                                &current_config->version,
                                                                &error) ||
        !qmi_indication_pdc_get_config_info_output_get_description (output, &description, &error)) {
        g_printerr ("error: couldn't get config info details: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }
    current_config->description = g_strdup (description);

    ctx->configs_loaded++;

    check_list_config_completed ();
}

static gboolean
list_configs_timeout (void)
{
    /* No indication yet, cancelling */
    if (!ctx->config_list) {
        g_printf ("Total configurations: 0\n");
        operation_shutdown (TRUE);
    }

    return FALSE;
}

static void
list_configs_ready_indication (QmiClientPdc *client,
                               QmiIndicationPdcListConfigsOutput *output)
{
    GError *error = NULL;
    GArray *configs = NULL;
    guint i;
    guint16 error_code = 0;

    /* Remove timeout as soon as we get the indication */
    if (ctx->timeout_id) {
        g_source_remove (ctx->timeout_id);
        ctx->timeout_id = 0;
    }

    if (!qmi_indication_pdc_list_configs_output_get_indication_result (output, &error_code, &error)) {
        g_printerr ("error: couldn't list configs: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (error_code != 0) {
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

    /* Preallocate config list and request details for each */
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
        current_info->token = token;
        current_info->id = g_array_ref (element->id);
        current_info->config_type = element->config_type;

        input = qmi_message_pdc_get_config_info_input_new ();

        /* Add type with id */
        type_with_id.config_type = element->config_type;
        type_with_id.id = current_info->id;
        if (!qmi_message_pdc_get_config_info_input_set_type_with_id (input, &type_with_id, &error)) {
            g_printerr ("error: couldn't set type with id: %s\n", error->message);
            g_error_free (error);
            operation_shutdown (FALSE);
            return;
        }

        /* Add token */
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
        qmi_message_pdc_get_config_info_input_unref (input);
    }

    check_list_config_completed ();
}

static void
list_configs_ready (QmiClientPdc *client,
                    GAsyncResult *res)
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

static QmiMessagePdcListConfigsInput *
list_configs_input_create (const gchar *str)
{
    QmiMessagePdcListConfigsInput *input = NULL;
    QmiPdcConfigurationType config_type;
    GError *error = NULL;

    if (!qmicli_read_pdc_configuration_type_from_string (str, &config_type))
        return NULL;

    input = qmi_message_pdc_list_configs_input_new ();
    if (!qmi_message_pdc_list_configs_input_set_config_type (input, config_type, &error) ||
        !qmi_message_pdc_list_configs_input_set_token (input, ctx->token++, &error)) {
        g_printerr ("error: couldn't create input data bundle: '%s'\n", error->message);
        g_error_free (error);
        qmi_message_pdc_list_configs_input_unref (input);
        return NULL;
    }

    return input;
}

static void
get_selected_config_ready_indication (QmiClientPdc *client,
                                      QmiIndicationPdcGetSelectedConfigOutput *output)
{
    GArray *pending_id = NULL;
    GArray *active_id = NULL;
    GError *error = NULL;
    guint16 error_code = 0;

    if (!qmi_indication_pdc_get_selected_config_output_get_indication_result (output, &error_code, &error)) {
        g_printerr ("error: couldn't get selected config: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (error_code != 0 &&
        error_code != QMI_PROTOCOL_ERROR_NOT_PROVISIONED) { /* No configs active */
        g_printerr ("error: couldn't get selected config: %s\n",
                    qmi_protocol_error_get_string ((QmiProtocolError) error_code));
        operation_shutdown (FALSE);
        return;
    }

    qmi_indication_pdc_get_selected_config_output_get_pending_id (output, &pending_id, NULL);
    qmi_indication_pdc_get_selected_config_output_get_active_id (output, &active_id, NULL);
    if (active_id)
        ctx->active_config_id = g_array_ref (active_id);
    if (pending_id)
        ctx->pending_config_id = g_array_ref (pending_id);

    ctx->ids_loaded = TRUE;

    check_list_config_completed ();
}

static void
get_selected_config_ready (QmiClientPdc *client,
                           GAsyncResult *res)
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

static QmiMessagePdcGetSelectedConfigInput *
get_selected_config_input_create (const gchar *str)
{
    QmiMessagePdcGetSelectedConfigInput *input = NULL;
    QmiPdcConfigurationType config_type;
    GError *error = NULL;

    if (!qmicli_read_pdc_configuration_type_from_string (str, &config_type))
        return NULL;

    input = qmi_message_pdc_get_selected_config_input_new ();
    if (!qmi_message_pdc_get_selected_config_input_set_config_type (input, config_type, &error) ||
        !qmi_message_pdc_get_selected_config_input_set_token (input, ctx->token++, &error)) {
        g_printerr ("error: couldn't create input data bundle: '%s'\n", error->message);
        g_error_free (error);
        qmi_message_pdc_get_selected_config_input_unref (input);
        return NULL;
    }

    return input;
}

static void
run_list_configs (void)
{
    QmiMessagePdcListConfigsInput *input;
    QmiMessagePdcGetSelectedConfigInput *get_selected_config_input;

    g_debug ("Listing configs asynchronously...");

    /* Results are reported via indications */
    ctx->list_configs_indication_id =
        g_signal_connect (ctx->client,
                          "list-configs",
                          G_CALLBACK
                          (list_configs_ready_indication), NULL);
    ctx->get_selected_config_indication_id =
        g_signal_connect (ctx->client, "get-selected-config",
                          G_CALLBACK (get_selected_config_ready_indication), NULL);
    ctx->get_config_info_indication_id =
        g_signal_connect (ctx->client, "get-config-info",
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

    /* We need a timeout, because there will be no indications if no configs
     * are loaded */
    ctx->timeout_id = g_timeout_add_seconds (LIST_CONFIGS_TIMEOUT_SECS,
                                             (GSourceFunc) list_configs_timeout,
                                             NULL);

    qmi_client_pdc_list_configs (ctx->client,
                                 input,
                                 10,
                                 ctx->cancellable,
                                 (GAsyncReadyCallback) list_configs_ready,
                                 NULL);
    qmi_message_pdc_list_configs_input_unref (input);

    qmi_client_pdc_get_selected_config (ctx->client,
                                        get_selected_config_input,
                                        10,
                                        ctx->cancellable,
                                        (GAsyncReadyCallback) get_selected_config_ready,
                                        NULL);
    qmi_message_pdc_get_selected_config_input_unref (get_selected_config_input);
}

#endif /* HAVE_QMI_ACTION_PDC_LIST_CONFIGS */

/******************************************************************************/
/* Activate config */

#if defined HAVE_QMI_ACTION_PDC_ACTIVATE_CONFIG

static void
device_removed_indication (QmiDevice *device)
{
    g_print ("[%s] Successfully requested config activation\n",
             qmi_device_get_path_display (ctx->device));

    /* Device gone, don't attempt to release CIDs */
    ctx->skip_cid_release = TRUE;

    /* If device gets removed during an activate config operation,
     * it means the operation is successful */
    operation_shutdown (TRUE);
}

static void
activate_config_ready_indication (QmiClientPdc *client,
                                  QmiIndicationPdcActivateConfigOutput *output)
{
    GError *error = NULL;
    guint16 error_code = 0;

    if (!qmi_indication_pdc_activate_config_output_get_indication_result (output, &error_code, &error)) {
        g_printerr ("error: couldn't activate config: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (error_code != 0) {
        g_printerr ("error: couldn't activate config: %s\n",
                    qmi_protocol_error_get_string ((QmiProtocolError) error_code));
        operation_shutdown (FALSE);
        return;
    }

    /* NOTE: config activation is expected to reboot the device, so we may detect the
     * actual reboot before receiving this indication */

    g_print ("[%s] Successfully requested config activation\n",
             qmi_device_get_path_display (ctx->device));

    operation_shutdown (TRUE);
}

static void
activate_config_ready (QmiClientPdc *client,
                       GAsyncResult *res)
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

static QmiMessagePdcActivateConfigInput *
activate_config_input_create (const gchar *str)
{
    QmiMessagePdcActivateConfigInput *input = NULL;
    QmiConfigTypeAndId *type_and_id;
    GError *error = NULL;

    type_and_id = parse_type_and_id (str);
    if (!type_and_id)
        return NULL;

    input = qmi_message_pdc_activate_config_input_new ();
    if (!qmi_message_pdc_activate_config_input_set_config_type (input, type_and_id->config_type, &error) ||
        !qmi_message_pdc_activate_config_input_set_token (input, ctx->token++, &error)) {
        g_printerr ("error: couldn't create input data bundle: '%s'\n", error->message);
        g_error_free (error);
        qmi_message_pdc_activate_config_input_unref (input);
        input = NULL;
    }

    g_array_unref (type_and_id->id);
    g_slice_free (QmiConfigTypeAndId, type_and_id);
    return input;
}

static void
set_selected_config_ready_indication (QmiClientPdc *client,
                                      QmiIndicationPdcSetSelectedConfigOutput *output)
{
    GError *error = NULL;
    QmiMessagePdcActivateConfigInput *input;
    guint16 error_code = 0;

    if (!qmi_indication_pdc_set_selected_config_output_get_indication_result (output, &error_code, &error)) {
        g_printerr ("error: couldn't set selected config: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (error_code != 0) {
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

    ctx->activate_config_indication_id =
        g_signal_connect (ctx->client,
                          "activate-config",
                          G_CALLBACK (activate_config_ready_indication),
                          NULL);
    ctx->device_removed_indication_id =
        g_signal_connect (ctx->device,
                          QMI_DEVICE_SIGNAL_REMOVED,
                          G_CALLBACK (device_removed_indication),
                          NULL);
    qmi_client_pdc_activate_config (ctx->client,
                                    input,
                                    10,
                                    ctx->cancellable,
                                    (GAsyncReadyCallback) activate_config_ready, NULL);
    qmi_message_pdc_activate_config_input_unref (input);
}

static void
set_selected_config_ready (QmiClientPdc *client,
                           GAsyncResult *res)
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

static QmiMessagePdcSetSelectedConfigInput *
set_selected_config_input_create (const gchar *str)
{
    QmiMessagePdcSetSelectedConfigInput *input = NULL;
    QmiConfigTypeAndId *type_and_id;
    GError *error = NULL;

    type_and_id = parse_type_and_id (str);
    if (!type_and_id)
        return NULL;

    input = qmi_message_pdc_set_selected_config_input_new ();
    if (!qmi_message_pdc_set_selected_config_input_set_type_with_id (input, type_and_id, &error) ||
        !qmi_message_pdc_set_selected_config_input_set_token (input, ctx->token++, &error)) {
        g_printerr ("error: couldn't create input data bundle: '%s'\n", error->message);
        g_error_free (error);
        qmi_message_pdc_set_selected_config_input_unref (input);
        input = NULL;
    }

    g_array_unref (type_and_id->id);
    g_slice_free (QmiConfigTypeAndId, type_and_id);
    return input;
}

static void
run_activate_config (void)
{
    QmiMessagePdcSetSelectedConfigInput *input;

    g_debug ("Activating config asynchronously...");
    input = set_selected_config_input_create (activate_config_str);
    if (!input) {
        operation_shutdown (FALSE);
        return;
    }

    /* Results are reported via indications */
    ctx->set_selected_config_indication_id =
        g_signal_connect (ctx->client,
                          "set-selected-config",
                          G_CALLBACK (set_selected_config_ready_indication), NULL);
    qmi_client_pdc_set_selected_config (ctx->client,
                                        input,
                                        10,
                                        ctx->cancellable,
                                        (GAsyncReadyCallback) set_selected_config_ready,
                                        NULL);
    qmi_message_pdc_set_selected_config_input_unref (input);
}

#endif /* HAVE_QMI_ACTION_PDC_ACTIVATE_CONFIG */

/******************************************************************************/
/* Deactivate config */

#if defined HAVE_QMI_ACTION_PDC_DEACTIVATE_CONFIG

static void
deactivate_config_ready_indication (QmiClientPdc *client,
                                    QmiIndicationPdcDeactivateConfigOutput *output)
{
    GError *error = NULL;
    guint16 error_code = 0;

    if (!qmi_indication_pdc_deactivate_config_output_get_indication_result (output, &error_code, &error)) {
        g_printerr ("error: couldn't deactivate config: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (error_code != 0) {
        g_printerr ("error: couldn't deactivate config: %s\n",
                    qmi_protocol_error_get_string ((QmiProtocolError) error_code));
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] Successfully requested config deactivation\n",
             qmi_device_get_path_display (ctx->device));

    operation_shutdown (TRUE);
}

static void
deactivate_config_ready (QmiClientPdc *client,
                         GAsyncResult *res)
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
        g_printerr ("error: couldn't deactivate config: %s\n", error->message);
        g_error_free (error);
        qmi_message_pdc_deactivate_config_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    qmi_message_pdc_deactivate_config_output_unref (output);
}

static QmiMessagePdcDeactivateConfigInput *
deactivate_config_input_create (const gchar *str)
{
    QmiMessagePdcDeactivateConfigInput *input = NULL;
    QmiConfigTypeAndId *type_and_id;
    GError *error = NULL;

    type_and_id = parse_type_and_id (str);
    if (!type_and_id)
        return NULL;

    input = qmi_message_pdc_deactivate_config_input_new ();
    if (!qmi_message_pdc_deactivate_config_input_set_config_type (input, type_and_id->config_type, &error) ||
        !qmi_message_pdc_deactivate_config_input_set_token (input, ctx->token++, &error)) {
        g_printerr ("error: couldn't create input data bundle: '%s'\n", error->message);
        g_error_free (error);
        qmi_message_pdc_deactivate_config_input_unref (input);
        input = NULL;
    }

    g_array_unref (type_and_id->id);
    g_slice_free (QmiConfigTypeAndId, type_and_id);
    return input;
}

static void
run_deactivate_config (void)
{
    QmiMessagePdcDeactivateConfigInput *input;

    g_debug ("Deactivating config asynchronously...");
    input = deactivate_config_input_create (deactivate_config_str);
    if (!input) {
        operation_shutdown (FALSE);
        return;
    }

    /* Results are reported via indications */
    ctx->deactivate_config_indication_id =
        g_signal_connect (ctx->client,
                          "deactivate-config",
                          G_CALLBACK
                          (deactivate_config_ready_indication),
                          NULL);

    qmi_client_pdc_deactivate_config (ctx->client,
                                      input,
                                      10,
                                      ctx->cancellable,
                                      (GAsyncReadyCallback) deactivate_config_ready, NULL);
    qmi_message_pdc_deactivate_config_input_unref (input);
}

#endif /* HAVE_QMI_ACTION_PDC_DEACTIVATE_CONFIG */

/******************************************************************************/
/* Delete config */

#if defined HAVE_QMI_MESSAGE_PDC_DELETE_CONFIG

static void
delete_config_ready (QmiClientPdc *client,
                     GAsyncResult *res)
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

    g_print ("[%s] Successfully deleted config\n",
             qmi_device_get_path_display (ctx->device));

    qmi_message_pdc_delete_config_output_unref (output);
    operation_shutdown (TRUE);
}

static QmiMessagePdcDeleteConfigInput *
delete_config_input_create (const gchar *str)
{
    QmiMessagePdcDeleteConfigInput *input = NULL;
    QmiConfigTypeAndId *type_and_id;
    GError *error = NULL;

    type_and_id = parse_type_and_id (str);
    if (!type_and_id)
        return NULL;

    input = qmi_message_pdc_delete_config_input_new ();
    if (!qmi_message_pdc_delete_config_input_set_config_type (input, type_and_id->config_type, &error) ||
        !qmi_message_pdc_delete_config_input_set_token (input, ctx->token++, &error) ||
        !qmi_message_pdc_delete_config_input_set_id (input, type_and_id->id, &error)) {
        g_printerr ("error: couldn't create input data bundle: '%s'\n", error->message);
        g_error_free (error);
        qmi_message_pdc_delete_config_input_unref (input);
        input = NULL;
    }

    g_array_unref (type_and_id->id);
    g_slice_free (QmiConfigTypeAndId, type_and_id);
    return input;
}

static void
run_delete_config (void)
{
    QmiMessagePdcDeleteConfigInput *input;

    g_debug ("Deleting config asynchronously...");
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
}

#endif /* HAVE_QMI_MESSAGE_PDC_DELETE_CONFIG */

/******************************************************************************/
/* Load config */

#if defined HAVE_QMI_ACTION_PDC_LOAD_CONFIG

static LoadConfigFileData *
load_config_file_from_string (const gchar *str)
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

    /* Get checksum */
    file_size = g_mapped_file_get_length (mapped_file);
    hash_size = g_checksum_type_get_length (G_CHECKSUM_SHA1);
    checksum = g_checksum_new (G_CHECKSUM_SHA1);
    g_checksum_update (checksum, file_contents, file_size);

    data = g_slice_new (LoadConfigFileData);
    data->mapped_file = mapped_file;
    data->checksum = g_array_sized_new (FALSE, FALSE, sizeof (guint8), hash_size);
    g_array_set_size (data->checksum, hash_size);
    data->offset = 0;
    g_checksum_get_digest (checksum, &g_array_index (data->checksum, guint8, 0), &hash_size);

    return data;
}

static QmiMessagePdcLoadConfigInput *
load_config_input_create_chunk (LoadConfigFileData *config_file)
{
    QmiMessagePdcLoadConfigInput *input;
    GError *error = NULL;
    GArray *chunk;
    gsize full_size;
    gsize chunk_size;
    guint8 *file_content;

    if (!config_file)
        return NULL;

    input = qmi_message_pdc_load_config_input_new ();
    if (!qmi_message_pdc_load_config_input_set_token (input, ctx->token++, &error)) {
        g_printerr ("error: couldn't set token: '%s'\n", error->message);
        g_error_free (error);
        qmi_message_pdc_load_config_input_unref (input);
        return NULL;
    }

    chunk = g_array_new (FALSE, FALSE, sizeof (guint8));
    full_size = g_mapped_file_get_length (config_file->mapped_file);
    chunk_size = (((config_file->offset + LOAD_CONFIG_CHUNK_SIZE) > full_size) ?
                  (full_size - config_file->offset) :
                  LOAD_CONFIG_CHUNK_SIZE);

    file_content = (guint8 *) g_mapped_file_get_contents (config_file->mapped_file);
    g_array_append_vals (chunk, &file_content[config_file->offset], chunk_size);
    g_print ("Uploaded %" G_GSIZE_FORMAT "  of %" G_GSIZE_FORMAT "\n", config_file->offset, full_size);

    if (!qmi_message_pdc_load_config_input_set_config_chunk (input,
                                                             QMI_PDC_CONFIGURATION_TYPE_SOFTWARE,
                                                             config_file->checksum,
                                                             full_size,
                                                             chunk,
                                                             &error)) {
        g_printerr ("error: couldn't set chunk: '%s'\n", error->message);
        g_error_free (error);
        g_array_unref (chunk);
        qmi_message_pdc_load_config_input_unref (input);
        return NULL;
    }

    config_file->offset += chunk_size;

    g_array_unref (chunk);
    return input;
}

static void load_config_ready (QmiClientPdc *client,
                               GAsyncResult *res);

static void
load_config_ready_indication (QmiClientPdc *client,
                              QmiIndicationPdcLoadConfigOutput *output)
{
    GError *error = NULL;
    QmiMessagePdcLoadConfigInput *input;
    gboolean frame_reset;
    guint32 remaining_size;
    guint16 error_code = 0;

    if (!qmi_indication_pdc_load_config_output_get_indication_result (output, &error_code, &error)) {
        g_printerr ("error: couldn't load config: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (error_code != 0) {
        g_printerr ("error: couldn't load config: %s\n",
                    qmi_protocol_error_get_string ((QmiProtocolError) error_code));
        operation_shutdown (FALSE);
        return;
    }

    if (qmi_indication_pdc_load_config_output_get_frame_reset (output, &frame_reset, NULL) && frame_reset) {
        g_printerr ("error: frame reset requested\n");
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_indication_pdc_load_config_output_get_remaining_size (output, &remaining_size, &error)) {
        g_printerr ("error: couldn't load config: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (remaining_size == 0) {
        g_print ("Finished loading\n");
        operation_shutdown (TRUE);
        return;
    }

    g_print ("Loading next chunk (%u bytes remaining)\n", remaining_size);
    input = load_config_input_create_chunk (ctx->load_config_file_data);
    if (!input) {
        g_printerr ("error: couldn't create next chunk\n");
        operation_shutdown (FALSE);
        return;
    }

    qmi_client_pdc_load_config (ctx->client,
                                input,
                                10,
                                ctx->cancellable,
                                (GAsyncReadyCallback) load_config_ready, NULL);
    qmi_message_pdc_load_config_input_unref (input);
}

static void
load_config_ready (QmiClientPdc *client,
                   GAsyncResult *res)
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

static QmiMessagePdcLoadConfigInput *
load_config_input_create (const gchar *str)
{
    LoadConfigFileData *config_file;
    QmiMessagePdcLoadConfigInput *input = NULL;

    config_file = load_config_file_from_string (str);
    if (!config_file)
        return NULL;

    input = load_config_input_create_chunk (config_file);
    if (!input)
        return NULL;

    ctx->load_config_file_data = config_file;
    return input;
}

static void
run_load_config (void)
{
    QmiMessagePdcLoadConfigInput *input;

    g_debug ("Loading config asynchronously...");
    input = load_config_input_create (load_config_str);
    if (!input) {
        operation_shutdown (FALSE);
        return;
    }

    /* Results are reported via indications */
    ctx->load_config_indication_id =
        g_signal_connect (ctx->client,
                          "load-config",
                          G_CALLBACK (load_config_ready_indication),
                          NULL);

    qmi_client_pdc_load_config (ctx->client,
                                input,
                                10,
                                ctx->cancellable,
                                (GAsyncReadyCallback) load_config_ready,
                                NULL);
    qmi_message_pdc_load_config_input_unref (input);
}

#endif /* HAVE_QMI_ACTION_PDC_LOAD_CONFIG */

/******************************************************************************/
/* Common */

static gboolean
noop_cb (gpointer unused)
{
    operation_shutdown (TRUE);
    return FALSE;
}

void
qmicli_pdc_run (QmiDevice *device,
                QmiClientPdc *client,
                GCancellable *cancellable)
{
    /* Initialize context */
    ctx = context_new (device, client, cancellable);

#if defined HAVE_QMI_ACTION_PDC_LIST_CONFIGS
    if (list_configs_str) {
        run_list_configs ();
        return;
    }
#endif

#if defined HAVE_QMI_ACTION_PDC_ACTIVATE_CONFIG
    if (activate_config_str) {
        run_activate_config ();
        return;
    }
#endif

#if defined HAVE_QMI_ACTION_PDC_DEACTIVATE_CONFIG
    if (deactivate_config_str) {
        run_deactivate_config ();
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_PDC_DELETE_CONFIG
    if (delete_config_str) {
        run_delete_config ();
        return;
    }
#endif

#if defined HAVE_QMI_ACTION_PDC_LOAD_CONFIG
    if (load_config_str) {
        run_load_config ();
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

#endif /* HAVE_QMI_SERVICE_PDC */
