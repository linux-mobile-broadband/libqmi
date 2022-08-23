/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * mbimcli -- Command line interface to control MBIM devices
 *
 * Copyright (C) 2022 Google, Inc.
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
#include "mbimcli-helpers.h"

/* Context */
typedef struct {
    MbimDevice   *device;
    GCancellable *cancellable;
} Context;
static Context *ctx;

/* Options */
static gboolean  query_uicc_application_list_flag;
static gchar    *query_uicc_file_status_str;

static GOptionEntry entries[] = {
    { "ms-query-uicc-application-list", 0, 0, G_OPTION_ARG_NONE, &query_uicc_application_list_flag,
      "Query UICC application list",
      NULL
    },
    { "ms-query-uicc-file-status", 0, 0, G_OPTION_ARG_STRING, &query_uicc_file_status_str,
      "Query UICC file status (allowed keys: application-id, file-path)",
      "[\"key=value,...\"]"
    },
    { NULL }
};

GOptionGroup *
mbimcli_ms_uicc_low_level_access_get_option_group (void)
{
   GOptionGroup *group;

   group = g_option_group_new ("ms-uicc-low-level-access",
                               "Microsoft UICC Low Level Access Service options:",
                               "Show Microsoft UICC Low Level Access Service options",
                               NULL,
                               NULL);
   g_option_group_add_entries (group, entries);

   return group;
}

gboolean
mbimcli_ms_uicc_low_level_access_options_enabled (void)
{
    static guint n_actions = 0;
    static gboolean checked = FALSE;

    if (checked)
        return !!n_actions;

    n_actions = query_uicc_application_list_flag +
                !!query_uicc_file_status_str;

    if (n_actions > 1) {
        g_printerr ("error: too many Microsoft UICC Low Level Access Service actions requested\n");
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
file_status_query_ready (MbimDevice   *device,
                         GAsyncResult *res)
{
    g_autoptr(MbimMessage)    response = NULL;
    g_autoptr(GError)         error = NULL;
    guint32                   status_word_1;
    guint32                   status_word_2;
    MbimUiccFileAccessibility file_accessibility;
    MbimUiccFileType          file_type;
    MbimUiccFileStructure     file_structure;
    guint32                   file_item_count;
    guint32                   file_item_size;
    MbimPinType               access_condition_read;
    MbimPinType               access_condition_update;
    MbimPinType               access_condition_activate;
    MbimPinType               access_condition_deactivate;

    response = mbim_device_command_finish (device, res, &error);
    if (!response || !mbim_message_response_get_result (response, MBIM_MESSAGE_TYPE_COMMAND_DONE, &error)) {
        g_printerr ("error: operation failed: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    if (!mbim_message_ms_uicc_low_level_access_file_status_response_parse (
            response,
            NULL, /* version */
            &status_word_1,
            &status_word_2,
            &file_accessibility,
            &file_type,
            &file_structure,
            &file_item_count,
            &file_item_size,
            &access_condition_read,
            &access_condition_update,
            &access_condition_activate,
            &access_condition_deactivate,
            &error)) {
        g_printerr ("error: couldn't parse response message: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    g_print ("[%s] UICC file status retrieved:\n"
             "\t    Status word 1: %u\n"
             "\t    Status word 2: %u\n"
             "\t    Accessibility: %s\n"
             "\t             Type: %s\n"
             "\t        Structure: %s\n"
             "\t       Item count: %u\n"
             "\t        Item size: %u\n"
             "\tAccess conditions:\n"
             "\t                 Read: %s\n"
             "\t               Update: %s\n"
             "\t             Activate: %s\n"
             "\t           Deactivate: %s\n",
             mbim_device_get_path_display (device),
             status_word_1,
             status_word_2,
             mbim_uicc_file_accessibility_get_string (file_accessibility),
             mbim_uicc_file_type_get_string (file_type),
             mbim_uicc_file_structure_get_string (file_structure),
             file_item_count,
             file_item_size,
             mbim_pin_type_get_string (access_condition_read),
             mbim_pin_type_get_string (access_condition_update),
             mbim_pin_type_get_string (access_condition_activate),
             mbim_pin_type_get_string (access_condition_deactivate));

    shutdown (TRUE);
}

typedef struct {
    gsize   application_id_size;
    guint8 *application_id;
    gsize   file_path_size;
    guint8 *file_path;
} FileStatusQueryProperties;

static void
file_status_query_properties_clear (FileStatusQueryProperties *props)
{
    g_clear_pointer (&props->application_id, g_free);
    g_clear_pointer (&props->file_path, g_free);
}

G_DEFINE_AUTO_CLEANUP_CLEAR_FUNC(FileStatusQueryProperties, file_status_query_properties_clear);

static gboolean
file_status_query_properties_handle (const gchar  *key,
                                     const gchar  *value,
                                     GError      **error,
                                     gpointer      user_data)
{
    FileStatusQueryProperties *props = user_data;

    if (g_ascii_strcasecmp (key, "application-id") == 0) {
        g_clear_pointer (&props->application_id, g_free);
        props->application_id_size = 0;
        props->application_id = mbimcli_read_buffer_from_string (value, -1, &props->application_id_size, error);
        if (!props->application_id)
            return FALSE;
    } else if (g_ascii_strcasecmp (key, "file-path") == 0) {
        g_clear_pointer (&props->file_path, g_free);
        props->file_path_size = 0;
        props->file_path = mbimcli_read_buffer_from_string (value, -1, &props->file_path_size, error);
        if (!props->file_path)
            return FALSE;
    } else {
        g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_FAILED,
                     "unrecognized option '%s'", key);
        return FALSE;
    }

    return TRUE;
}

static gboolean
file_status_query_input_parse (const gchar                *str,
                               FileStatusQueryProperties  *props,
                               GError                    **error)
{

    if (!mbimcli_parse_key_value_string (str,
                                         error,
                                         file_status_query_properties_handle,
                                         props))
        return FALSE;

    if (!props->application_id_size || !props->application_id) {
        g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_FAILED,
                     "Option 'application-id' is missing");
        return FALSE;
    }

    if (!props->file_path_size || !props->file_path) {
        g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_FAILED,
                     "Option 'file-path' is missing");
        return FALSE;
    }

    return TRUE;
}

static void
application_list_query_ready (MbimDevice   *device,
                              GAsyncResult *res)
{
    g_autoptr(MbimMessage)              response = NULL;
    g_autoptr(GError)                   error = NULL;
    guint32                             application_count;
    guint32                             active_application_index;
    g_autoptr(MbimUiccApplicationArray) applications = NULL;
    guint32                             i;

    response = mbim_device_command_finish (device, res, &error);
    if (!response || !mbim_message_response_get_result (response, MBIM_MESSAGE_TYPE_COMMAND_DONE, &error)) {
        g_printerr ("error: operation failed: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    if (!mbim_message_ms_uicc_low_level_access_application_list_response_parse (
            response,
            NULL, /* version */
            &application_count,
            &active_application_index,
            NULL, /* application_list_size_bytes */
            &applications,
            &error)) {
        g_printerr ("error: couldn't parse response message: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    g_print ("[%s] UICC applications: (%u)\n",
             mbim_device_get_path_display (device),
             application_count);

    for (i = 0; i < application_count; i++) {
        g_autofree gchar *application_id_str = NULL;
        g_autofree gchar *pin_key_references_str = NULL;

        application_id_str = mbim_common_str_hex (applications[i]->application_id, applications[i]->application_id_size, ':');
        pin_key_references_str = mbim_common_str_hex (applications[i]->pin_key_references, applications[i]->pin_key_references_size, ':');

        g_print ("Application %u:%s\n",             i, (i == active_application_index) ? " (active)" : "");
        g_print ("\tApplication type:        %s\n", mbim_uicc_application_type_get_string (applications[i]->application_type));
        g_print ("\tApplication ID:          %s\n", application_id_str);
        g_print ("\tApplication name:        %s\n", applications[i]->application_name);
        g_print ("\tPIN key reference count: %u\n", applications[i]->pin_key_reference_count);
        g_print ("\tPIN key references:      %s\n", pin_key_references_str);
    }

    shutdown (TRUE);
}

void
mbimcli_ms_uicc_low_level_access_run (MbimDevice   *device,
                                      GCancellable *cancellable)
{
    g_autoptr(MbimMessage) request = NULL;
    g_autoptr(GError)      error = NULL;

    /* Initialize context */
    ctx = g_slice_new (Context);
    ctx->device = g_object_ref (device);
    ctx->cancellable = cancellable ? g_object_ref (cancellable) : NULL;

    /* Request to query UICC application list? */
    if (query_uicc_application_list_flag) {
        g_debug ("Asynchronously querying UICC application list...");
        request = mbim_message_ms_uicc_low_level_access_application_list_query_new (NULL);
        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)application_list_query_ready,
                             NULL);
        return;
    }

    /* Request to query UICC file status? */
    if (query_uicc_file_status_str) {
        g_auto(FileStatusQueryProperties) props = {
            .application_id_size = 0,
            .application_id      = NULL,
            .file_path_size      = 0,
            .file_path           = NULL,
        };

        g_debug ("Asynchronously querying UICC file status...");

        if (!file_status_query_input_parse (query_uicc_file_status_str, &props, &error)) {
            g_printerr ("error: couldn't parse input arguments: %s\n", error->message);
            shutdown (FALSE);
            return;
        }

        request = mbim_message_ms_uicc_low_level_access_file_status_query_new (1, /* version fixed */
                                                                               props.application_id_size,
                                                                               props.application_id,
                                                                               props.file_path_size,
                                                                               props.file_path,
                                                                               NULL);
        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)file_status_query_ready,
                             NULL);
        return;
    }

    g_warn_if_reached ();
}
