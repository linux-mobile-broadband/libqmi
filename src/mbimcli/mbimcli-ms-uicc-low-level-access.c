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
static gchar    *query_uicc_read_binary_str;
static gchar    *query_uicc_read_record_str;
static gchar    *set_uicc_open_channel_str;
static gchar    *set_uicc_close_channel_str;

static GOptionEntry entries[] = {
    { "ms-query-uicc-application-list", 0, 0, G_OPTION_ARG_NONE, &query_uicc_application_list_flag,
      "Query UICC application list",
      NULL
    },
    { "ms-query-uicc-file-status", 0, 0, G_OPTION_ARG_STRING, &query_uicc_file_status_str,
      "Query UICC file status (allowed keys: application-id, file-path)",
      "[\"key=value,...\"]"
    },
    { "ms-query-uicc-read-binary", 0, 0, G_OPTION_ARG_STRING, &query_uicc_read_binary_str,
      "Read UICC binary file (allowed keys: application-id, file-path, read-offset, read-size, local-pin and data)",
      "[\"key=value,...\"]"
    },
    { "ms-query-uicc-read-record", 0, 0, G_OPTION_ARG_STRING, &query_uicc_read_record_str,
      "Read UICC record file (allowed keys: application-id, file-path, record-number, local-pin and data)",
      "[\"key=value,...\"]"
    },
    { "ms-set-uicc-open-channel", 0, 0, G_OPTION_ARG_STRING, &set_uicc_open_channel_str,
      "Set UICC open channel (allowed keys: application-id, selectp2arg, channel-group)",
      "[\"key=value,...\"]"
    },
    { "ms-set-uicc-close-channel", 0, 0, G_OPTION_ARG_STRING, &set_uicc_close_channel_str,
      "Set UICC close channel (allowed keys: channel, channel-group)",
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
                !!query_uicc_file_status_str +
                !!query_uicc_read_binary_str +
                !!query_uicc_read_record_str +
                !!set_uicc_open_channel_str +
                !!set_uicc_close_channel_str;

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
read_record_query_ready (MbimDevice   *device,
                         GAsyncResult *res)
{
    g_autoptr(MbimMessage)    response = NULL;
    g_autoptr(GError)         error = NULL;
    guint32                   status_word_1;
    guint32                   status_word_2;
    const guint8             *data;
    guint32                   data_size;
    g_autofree gchar         *data_str = NULL;

    response = mbim_device_command_finish (device, res, &error);
    if (!response || !mbim_message_response_get_result (response, MBIM_MESSAGE_TYPE_COMMAND_DONE, &error)) {
        g_printerr ("error: operation failed: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    if (!mbim_message_ms_uicc_low_level_access_read_record_response_parse (
            response,
            NULL, /* version */
            &status_word_1,
            &status_word_2,
            &data_size,
            &data,
            &error)) {
        g_printerr ("error: couldn't parse response message: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    data_str = mbim_common_str_hex (data, data_size, ':');

    g_print ("[%s] UICC file record read:\n"
             "\tStatus word 1: %u\n"
             "\tStatus word 2: %u\n"
             "\t         Data: %s\n",
             mbim_device_get_path_display (device),
             status_word_1,
             status_word_2,
             data_str);

    shutdown (TRUE);
}

typedef struct {
    gsize    application_id_size;
    guint8  *application_id;
    gsize    file_path_size;
    guint8  *file_path;
    guint32  record_number;
    gchar   *local_pin;
    gsize    data_size;
    guint8  *data;
} ReadRecordQueryProperties;

static void
read_record_query_properties_clear (ReadRecordQueryProperties *props)
{
    g_clear_pointer (&props->application_id, g_free);
    g_clear_pointer (&props->file_path, g_free);
    g_clear_pointer (&props->local_pin, g_free);
    g_clear_pointer (&props->data, g_free);
}

G_DEFINE_AUTO_CLEANUP_CLEAR_FUNC(ReadRecordQueryProperties, read_record_query_properties_clear);

static gboolean
read_record_query_properties_handle (const gchar  *key,
                                     const gchar  *value,
                                     GError      **error,
                                     gpointer      user_data)
{
    ReadRecordQueryProperties *props = user_data;

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
    } else if (g_ascii_strcasecmp (key, "record-number") == 0) {
        if (!mbimcli_read_uint_from_string (value, &props->record_number)) {
            g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_FAILED,
                         "Failed to parse field as an integer");
            return FALSE;
        }
    } else if (g_ascii_strcasecmp (key, "local-pin") == 0) {
        g_clear_pointer (&props->local_pin, g_free);
        props->local_pin = g_strdup (value);
    } else if (g_ascii_strcasecmp (key, "data") == 0) {
        g_clear_pointer (&props->data, g_free);
        props->data_size = 0;
        props->data = mbimcli_read_buffer_from_string (value, -1, &props->data_size, error);
        if (!props->data)
            return FALSE;
    } else {
        g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_FAILED,
                     "unrecognized option '%s'", key);
        return FALSE;
    }

    return TRUE;
}

static gboolean
read_record_query_input_parse (const gchar                *str,
                               ReadRecordQueryProperties  *props,
                               GError                    **error)
{

    if (!mbimcli_parse_key_value_string (str,
                                         error,
                                         read_record_query_properties_handle,
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

    /* all the other fields are optional */

    return TRUE;
}

static void
read_binary_query_ready (MbimDevice   *device,
                         GAsyncResult *res)
{
    g_autoptr(MbimMessage)    response = NULL;
    g_autoptr(GError)         error = NULL;
    guint32                   status_word_1;
    guint32                   status_word_2;
    const guint8             *data;
    guint32                   data_size;
    g_autofree gchar         *data_str = NULL;

    response = mbim_device_command_finish (device, res, &error);
    if (!response || !mbim_message_response_get_result (response, MBIM_MESSAGE_TYPE_COMMAND_DONE, &error)) {
        g_printerr ("error: operation failed: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    if (!mbim_message_ms_uicc_low_level_access_read_binary_response_parse (
            response,
            NULL, /* version */
            &status_word_1,
            &status_word_2,
            &data_size,
            &data,
            &error)) {
        g_printerr ("error: couldn't parse response message: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    data_str = mbim_common_str_hex (data, data_size, ':');

    g_print ("[%s] UICC file binary read:\n"
             "\tStatus word 1: %u\n"
             "\tStatus word 2: %u\n"
             "\t         Data: %s\n",
             mbim_device_get_path_display (device),
             status_word_1,
             status_word_2,
             data_str);

    shutdown (TRUE);
}

typedef struct {
    gsize    application_id_size;
    guint8  *application_id;
    gsize    file_path_size;
    guint8  *file_path;
    guint32  read_offset;
    guint32  read_size;
    gchar   *local_pin;
    gsize    data_size;
    guint8  *data;
} ReadBinaryQueryProperties;

static void
read_binary_query_properties_clear (ReadBinaryQueryProperties *props)
{
    g_clear_pointer (&props->application_id, g_free);
    g_clear_pointer (&props->file_path, g_free);
    g_clear_pointer (&props->local_pin, g_free);
    g_clear_pointer (&props->data, g_free);
}

G_DEFINE_AUTO_CLEANUP_CLEAR_FUNC(ReadBinaryQueryProperties, read_binary_query_properties_clear);

static gboolean
read_binary_query_properties_handle (const gchar  *key,
                                     const gchar  *value,
                                     GError      **error,
                                     gpointer      user_data)
{
    ReadBinaryQueryProperties *props = user_data;

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
    } else if (g_ascii_strcasecmp (key, "read-offset") == 0) {
        if (!mbimcli_read_uint_from_string (value, &props->read_offset)) {
            g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_FAILED,
                         "Failed to parse field as an integer");
            return FALSE;
        }
    } else if (g_ascii_strcasecmp (key, "read-size") == 0) {
        if (!mbimcli_read_uint_from_string (value, &props->read_size)) {
            g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_FAILED,
                         "Failed to parse field as an integer");
            return FALSE;
        }
    } else if (g_ascii_strcasecmp (key, "local-pin") == 0) {
        g_clear_pointer (&props->local_pin, g_free);
        props->local_pin = g_strdup (value);
    } else if (g_ascii_strcasecmp (key, "data") == 0) {
        g_clear_pointer (&props->data, g_free);
        props->data_size = 0;
        props->data = mbimcli_read_buffer_from_string (value, -1, &props->data_size, error);
        if (!props->data)
            return FALSE;
    } else {
        g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_FAILED,
                     "unrecognized option '%s'", key);
        return FALSE;
    }

    return TRUE;
}

static gboolean
read_binary_query_input_parse (const gchar                *str,
                               ReadBinaryQueryProperties  *props,
                               GError                    **error)
{

    if (!mbimcli_parse_key_value_string (str,
                                         error,
                                         read_binary_query_properties_handle,
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

    /* all the other fields are optional */

    return TRUE;
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

static void
open_channel_ready (MbimDevice   *device,
                    GAsyncResult *res)
{
    g_autoptr(MbimMessage)  response = NULL;
    g_autoptr(GError)       error = NULL;
    guint32                 status = 0;
    guint32                 channel = 0;
    const guint8           *open_channel_response = NULL;
    guint32                 open_channel_response_size = 0;
    g_autofree gchar       *open_channel_response_str = NULL;

    response = mbim_device_command_finish (device, res, &error);
    if (!response || !mbim_message_response_get_result (response, MBIM_MESSAGE_TYPE_COMMAND_DONE, &error)) {
        g_printerr ("error: operation failed: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    if (!mbim_message_ms_uicc_low_level_access_open_channel_response_parse (
            response,
            &status,
            &channel,
            &open_channel_response_size,
            &open_channel_response,
            &error)) {
        g_printerr ("error: couldn't parse response message: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    open_channel_response_str = mbim_common_str_hex (open_channel_response, open_channel_response_size, ':');
    g_print ("Succesfully retrieved open channel info:\n"
             "\t  status: %u\n"
             "\t channel: %u\n"
             "\tresponse: %s\n",
             status,
             channel,
             open_channel_response_str);

    shutdown (TRUE);
}

typedef struct {
    guint32  channel_group;
    guint32  selectprg;
    gsize    application_id_size;
    guint8  *application_id;
} OpenChannelProperties;

static void
open_channel_properties_clear (OpenChannelProperties *props)
{
    g_clear_pointer (&props->application_id, g_free);
}

G_DEFINE_AUTO_CLEANUP_CLEAR_FUNC(OpenChannelProperties, open_channel_properties_clear);

static gboolean
open_channel_properties_handle (const gchar  *key,
                                const gchar  *value,
                                GError      **error,
                                gpointer      user_data)
{
    OpenChannelProperties *props = user_data;

    if (g_ascii_strcasecmp (key, "application-id") == 0) {
        g_clear_pointer (&props->application_id, g_free);
        props->application_id_size = 0;
        props->application_id = mbimcli_read_buffer_from_string (value, -1, &props->application_id_size, error);
        if (!props->application_id)
            return FALSE;
    } else if (g_ascii_strcasecmp (key, "selectp2arg") == 0) {
        if (!mbimcli_read_uint_from_string (value, &props->selectprg)) {
            g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_FAILED,
                         "Failed to parse selectp2arg field as an integer");
            return FALSE;
        }
    } else if (g_ascii_strcasecmp (key, "channel-group") == 0) {
        if (!mbimcli_read_uint_from_string (value, &props->channel_group)) {
            g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_FAILED,
                         "Failed to parse channel-group field as an integer");
            return FALSE;
        }
    } else {
        g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_FAILED,
                     "unrecognized option '%s'", key);
        return FALSE;
    }

    return TRUE;
}

static gboolean
open_channel_input_parse (const gchar            *str,
                          OpenChannelProperties  *props,
                          GError                **error)
{
    if (!mbimcli_parse_key_value_string (str,
                                         error,
                                         open_channel_properties_handle,
                                         props))
        return FALSE;

    if (!props->application_id_size || !props->application_id) {
        g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_FAILED,
                     "Option 'application-id' is missing");
        return FALSE;
    }

    return TRUE;
}

static void
close_channel_ready (MbimDevice   *device,
                     GAsyncResult *res)
{
    g_autoptr(MbimMessage)  response = NULL;
    g_autoptr(GError)       error = NULL;
    guint32                 status = 0;

    response = mbim_device_command_finish (device, res, &error);
    if (!response || !mbim_message_response_get_result (response, MBIM_MESSAGE_TYPE_COMMAND_DONE, &error)) {
        g_printerr ("error: operation failed: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    if (!mbim_message_ms_uicc_low_level_access_close_channel_response_parse (
            response,
            &status,
            &error)) {
        g_printerr ("error: couldn't parse response message: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    g_print ("Succesfully retrieved close channel info:\n"
             "\tstatus: %u\n", status);

    shutdown (TRUE);
}

typedef struct {
    guint32  channel;
    guint32  channel_group;
} CloseChannelProperties;

static gboolean
close_channel_properties_handle (const gchar  *key,
                                 const gchar  *value,
                                 GError      **error,
                                 gpointer      user_data)
{
    CloseChannelProperties *props = user_data;

    if (g_ascii_strcasecmp (key, "channel") == 0) {
        if (!mbimcli_read_uint_from_string (value, &props->channel)) {
            g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_FAILED,
                         "Failed to parse channel field as an integer");
            return FALSE;
        }
    } else if (g_ascii_strcasecmp (key, "channel-group") == 0) {
        if (!mbimcli_read_uint_from_string (value, &props->channel_group)) {
            g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_FAILED,
                         "Failed to parse channel-group field as an integer");
            return FALSE;
        }
    } else {
        g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_FAILED,
                     "unrecognized option '%s'", key);
        return FALSE;
    }

    return TRUE;
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

    /* Request to UICC read binary? */
    if (query_uicc_read_binary_str) {
        g_auto(ReadBinaryQueryProperties) props = {
            .application_id_size = 0,
            .application_id      = NULL,
            .file_path_size      = 0,
            .file_path           = NULL,
            .read_offset         = 0,
            .read_size           = 0,
            .local_pin           = NULL,
            .data_size           = 0,
            .data                = NULL,
        };

        g_debug ("Asynchronously reading from UICC in binary...");

        if (!read_binary_query_input_parse (query_uicc_read_binary_str, &props, &error)) {
            g_printerr ("error: couldn't parse input arguments: %s\n", error->message);
            shutdown (FALSE);
            return;
        }

        request = mbim_message_ms_uicc_low_level_access_read_binary_query_new (1, /* version fixed */
                                                                               props.application_id_size,
                                                                               props.application_id,
                                                                               props.file_path_size,
                                                                               props.file_path,
                                                                               props.read_offset,
                                                                               props.read_size,
                                                                               props.local_pin,
                                                                               props.data_size,
                                                                               props.data,
                                                                               NULL);
        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)read_binary_query_ready,
                             NULL);
        return;
    }

    /* Request to UICC read record? */
    if (query_uicc_read_record_str) {
        g_auto(ReadRecordQueryProperties) props = {
            .application_id_size = 0,
            .application_id      = NULL,
            .file_path_size      = 0,
            .file_path           = NULL,
            .record_number       = 0,
            .local_pin           = NULL,
            .data_size           = 0,
            .data                = NULL,
        };

        g_debug ("Asynchronously reading from UICC record...");

        if (!read_record_query_input_parse (query_uicc_read_record_str, &props, &error)) {
            g_printerr ("error: couldn't parse input arguments: %s\n", error->message);
            shutdown (FALSE);
            return;
        }

        request = mbim_message_ms_uicc_low_level_access_read_record_query_new (1, /* version fixed */
                                                                               props.application_id_size,
                                                                               props.application_id,
                                                                               props.file_path_size,
                                                                               props.file_path,
                                                                               props.record_number,
                                                                               props.local_pin,
                                                                               props.data_size,
                                                                               props.data,
                                                                               NULL);
        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)read_record_query_ready,
                             NULL);
        return;
    }

    /* Request to Set UICC open channel */
    if (set_uicc_open_channel_str) {
        g_auto(OpenChannelProperties) props = {
            .application_id_size = 0,
            .application_id      = NULL,
            .selectprg           = 0,
            .channel_group       = 0,
        };

        if (!open_channel_input_parse (set_uicc_open_channel_str, &props, &error)) {
            g_printerr ("error: couldn't parse input arguments: %s\n", error->message);
            shutdown (FALSE);
            return;
        }

        g_debug ("Asynchronously setting UICC open channel.");
        request = mbim_message_ms_uicc_low_level_access_open_channel_set_new (props.application_id_size,
                                                                              props.application_id,
                                                                              props.selectprg,
                                                                              props.channel_group,
                                                                              &error);
        if (!request) {
            g_printerr ("error: couldn't create open channel Request %s\n", error->message);
            shutdown (FALSE);
            return;
        }

        mbim_device_command (ctx->device,
                             request,
                             30,
                             ctx->cancellable,
                             (GAsyncReadyCallback)open_channel_ready,
                             NULL);
        return;
    }

    /* Request to Set UICC close channel */
    if (set_uicc_close_channel_str) {
        CloseChannelProperties props = {
            .channel       = 0,
            .channel_group = 0,
        };

        if (!mbimcli_parse_key_value_string (set_uicc_close_channel_str,
                                             &error,
                                             close_channel_properties_handle,
                                             &props)) {
            g_printerr ("error: couldn't parse input arguments: %s\n", error->message);
            shutdown (FALSE);
            return;
        }

        g_debug ("Asynchronously setting UICC close channel...");
        request = mbim_message_ms_uicc_low_level_access_close_channel_set_new (props.channel,
                                                                               props.channel_group,
                                                                               &error);
        if (!request) {
            g_printerr ("error: couldn't create close channel request %s\n", error->message);
            shutdown (FALSE);
            return;
        }

        mbim_device_command (ctx->device,
                             request,
                             30,
                             ctx->cancellable,
                             (GAsyncReadyCallback)close_channel_ready,
                             NULL);
        return;
    }

    g_warn_if_reached ();
}
