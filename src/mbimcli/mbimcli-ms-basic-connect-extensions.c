/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * mbimcli -- Command line interface to control MBIM devices
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
#include "mbimcli-helpers.h"

/* Context */
typedef struct {
    MbimDevice *device;
    GCancellable *cancellable;
} Context;
static Context *ctx;

/* Options */
static gchar    *query_pco_str;
static gboolean  query_lte_attach_configuration_flag;
static gboolean  query_lte_attach_status_flag; /* support for the deprecated name */
static gboolean  query_lte_attach_info_flag;
static gboolean  query_sys_caps_flag;
static gboolean  query_device_caps_flag;
static gchar    *query_slot_info_status_str;
static gboolean  query_device_slot_mappings_flag;
static gchar    *set_device_slot_mappings_str;
static gboolean  query_location_info_status_flag;
static gchar    *query_version_str;
static gboolean  query_provisioned_contexts_v2_flag;
static gchar    *set_provisioned_contexts_v2_flag;


static gboolean query_pco_arg_parse (const gchar  *option_name,
                                     const gchar  *value,
                                     gpointer      user_data,
                                     GError      **error);

static GOptionEntry entries[] = {
    { "ms-query-pco", 0, G_OPTION_FLAG_OPTIONAL_ARG, G_OPTION_ARG_CALLBACK, G_CALLBACK (query_pco_arg_parse),
      "Query PCO value (SessionID is optional, defaults to 0)",
      "[SessionID]"
    },
    { "ms-query-lte-attach-configuration", 0, 0, G_OPTION_ARG_NONE, &query_lte_attach_configuration_flag,
      "Query LTE attach configuration",
      NULL
    },
    { "ms-query-lte-attach-status", 0, G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_NONE, &query_lte_attach_status_flag,
      NULL,
      NULL
    },
    { "ms-query-lte-attach-info", 0, 0, G_OPTION_ARG_NONE, &query_lte_attach_info_flag,
      "Query LTE attach status information",
      NULL
    },
    { "ms-query-sys-caps", 0, 0, G_OPTION_ARG_NONE, &query_sys_caps_flag,
      "Query system capabilities",
      NULL
    },
    { "ms-query-device-caps", 0,0, G_OPTION_ARG_NONE, &query_device_caps_flag,
      "Query device capabilities",
      NULL
    },
    { "ms-query-slot-info-status", 0, 0, G_OPTION_ARG_STRING, &query_slot_info_status_str,
      "Query slot information status",
      "[SlotIndex]"
    },
    { "ms-set-device-slot-mappings", 0, 0, G_OPTION_ARG_STRING, &set_device_slot_mappings_str,
      "Set device slot mappings for each executor",
      "[(SlotIndex)[,(SlotIndex)[,...]]]"

    },
    { "ms-query-device-slot-mappings", 0, 0, G_OPTION_ARG_NONE, &query_device_slot_mappings_flag,
      "Query device slot mappings",
      NULL
    },
    { "ms-query-location-info-status", 0, 0, G_OPTION_ARG_NONE, &query_location_info_status_flag,
      "Query location info status",
      NULL
    },
    { "ms-query-version", 0, 0,G_OPTION_ARG_STRING , &query_version_str,
      "Exchange supported version information",
      "[(MBIM version),(MBIM extended version)]"
    },
    { "ms-set-provisioned-contexts-v2", 0, 0, G_OPTION_ARG_STRING, &set_provisioned_contexts_v2_flag,
      "set provisioned contexts V2",
      "[(access_string),(user_name),(password)]"
    },
    { "ms-query-provisioned-contexts-v2", 0, 0, G_OPTION_ARG_NONE, &query_provisioned_contexts_v2_flag,
      "Query provisioned contexts V2",
      NULL
    },
    {NULL }
};

static gboolean
query_pco_arg_parse (const gchar *option_name,
                     const gchar *value,
                     gpointer     user_data,
                     GError      **error)
{
    query_pco_str = g_strdup (value ? value : "0");
    return TRUE;
}

GOptionGroup *
mbimcli_ms_basic_connect_extensions_get_option_group (void)
{
   GOptionGroup *group;

   group = g_option_group_new ("ms-basic-connect-extensions",
                               "Microsoft Basic Connect Extensions options:",
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
    if (errno || n < 0 || n > 255 || ((size_t)(endptr - str) < strlen (str))) {
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
                 (query_lte_attach_status_flag || query_lte_attach_info_flag) +
                 query_sys_caps_flag +
                 query_device_caps_flag +
                 !!query_slot_info_status_str +
                 !!set_device_slot_mappings_str +
                 query_device_slot_mappings_flag +
                 query_location_info_status_flag +
                 !!query_version_str +
                 query_provisioned_contexts_v2_flag +
                 !!set_provisioned_contexts_v2_flag);

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
    g_autoptr(MbimMessage)   response = NULL;
    g_autoptr(GError)        error = NULL;
    g_autoptr(MbimPcoValue)  pco_value = NULL;
    g_autofree gchar        *pco_data = NULL;

    response = mbim_device_command_finish (device, res, &error);
    if (!response || !mbim_message_response_get_result (response, MBIM_MESSAGE_TYPE_COMMAND_DONE, &error)) {
        g_printerr ("error: operation failed: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    g_print ("[%s] Successfully queried PCO\n", mbim_device_get_path_display (device));

    if (!mbim_message_ms_basic_connect_extensions_pco_response_parse (
            response,
            &pco_value,
            &error)) {
        g_printerr ("error: couldn't parse response message: %s\n", error->message);
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

    shutdown (TRUE);
}

static void
query_lte_attach_configuration_ready (MbimDevice   *device,
                                      GAsyncResult *res)
{
    g_autoptr(MbimMessage)                     response = NULL;
    g_autoptr(GError)                          error = NULL;
    g_autoptr(MbimLteAttachConfigurationArray) configurations = NULL;
    guint32                                    configuration_count = 0;
    guint                                      i;

    response = mbim_device_command_finish (device, res, &error);
    if (!response || !mbim_message_response_get_result (response, MBIM_MESSAGE_TYPE_COMMAND_DONE, &error)) {
        g_printerr ("error: operation failed: %s\n", error->message);
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

    shutdown (TRUE);
}

static void
query_lte_attach_info_ready (MbimDevice   *device,
                             GAsyncResult *res)
{
    g_autoptr(MbimMessage)  response = NULL;
    g_autoptr(GError)       error = NULL;
    guint32                 lte_attach_state;
    guint32                 ip_type;
    g_autofree gchar       *access_string = NULL;
    g_autofree gchar       *user_name = NULL;
    g_autofree gchar       *password = NULL;
    guint32                 compression;
    guint32                 auth_protocol;

    response = mbim_device_command_finish (device, res, &error);
    if (!response || !mbim_message_response_get_result (response, MBIM_MESSAGE_TYPE_COMMAND_DONE, &error)) {
        g_printerr ("error: operation failed: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    g_print ("[%s] Successfully queried LTE attach info\n",
             mbim_device_get_path_display (device));

    if (!mbim_message_ms_basic_connect_extensions_lte_attach_info_response_parse (
            response,
            &lte_attach_state,
            &ip_type,
            &access_string,
            &user_name,
            &password,
            &compression,
            &auth_protocol,
            &error)) {
        g_printerr ("error: couldn't parse response message: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

#define VALIDATE_NA(str) (str ? str : "n/a")
    g_print ("  Attach state:  %s\n", mbim_lte_attach_state_get_string (lte_attach_state));
    g_print ("  IP type:       %s\n", mbim_context_ip_type_get_string (ip_type));
    g_print ("  Access string: %s\n", VALIDATE_NA (access_string));
    g_print ("  Username:      %s\n", VALIDATE_NA (user_name));
    g_print ("  Password:      %s\n", VALIDATE_NA (password));
    g_print ("  Compression:   %s\n", mbim_compression_get_string (compression));
    g_print ("  Auth protocol: %s\n", mbim_auth_protocol_get_string (auth_protocol));
#undef VALIDATE_NA

    shutdown (TRUE);
}

static void
query_sys_caps_ready (MbimDevice   *device,
                      GAsyncResult *res)
{
    g_autoptr(MbimMessage)  response = NULL;
    g_autoptr(GError)       error = NULL;
    guint32                 number_executors;
    guint32                 number_slots;
    guint32                 concurrency;
    guint64                 modem_id;

    response = mbim_device_command_finish (device, res, &error);
    if (!response || !mbim_message_response_get_result (response, MBIM_MESSAGE_TYPE_COMMAND_DONE, &error)) {
        g_printerr ("error: operation failed: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    g_print ("[%s] Successfully queried sys caps\n",
             mbim_device_get_path_display (device));

    if (!mbim_message_ms_basic_connect_extensions_sys_caps_response_parse (
            response,
            &number_executors,
            &number_slots,
            &concurrency,
            &modem_id,
            &error)) {
        g_printerr ("error: couldn't parse response messages: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    g_print ("[%s] System capabilities retrieved:\n"
             "\t Number of executors: '%u'\n"
             "\t     Number of slots: '%u'\n"
             "\t         Concurrency: '%u'\n"
             "\t            Modem ID: '%" G_GUINT64_FORMAT "'\n",
             mbim_device_get_path_display (device),
             number_executors,
             number_slots,
             concurrency,
             modem_id);

    shutdown (TRUE);
}

static void
query_device_caps_ready (MbimDevice   *device,
                         GAsyncResult *res)
{
    g_autoptr(MbimMessage)  response = NULL;
    g_autoptr(GError)       error = NULL;
    MbimDeviceType          device_type;
    const gchar            *device_type_str;
    MbimVoiceClass          voice_class;
    const gchar            *voice_class_str;
    MbimCellularClass       cellular_class;
    g_autofree gchar       *cellular_class_str = NULL;
    MbimSimClass            sim_class;
    g_autofree gchar       *sim_class_str = NULL;
    MbimDataClass           data_class;
    g_autofree gchar       *data_class_str = NULL;
    MbimSmsCaps             sms_caps;
    g_autofree gchar       *sms_caps_str = NULL;
    MbimCtrlCaps            ctrl_caps;
    g_autofree gchar       *ctrl_caps_str = NULL;
    guint32                 max_sessions;
    g_autofree gchar       *custom_data_class = NULL;
    g_autofree gchar       *device_id = NULL;
    g_autofree gchar       *firmware_info = NULL;
    g_autofree gchar       *hardware_info = NULL;
    guint32                 executor_index;

    response = mbim_device_command_finish(device, res, &error);
    if (!response || !mbim_message_response_get_result (response, MBIM_MESSAGE_TYPE_COMMAND_DONE, &error)) {
        g_printerr ("error: operation failed: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    if (!mbim_message_ms_basic_connect_extensions_device_caps_response_parse (
            response,
            &device_type,
            &cellular_class,
            &voice_class,
            &sim_class,
            &data_class,
            &sms_caps,
            &ctrl_caps,
            &max_sessions,
            &custom_data_class,
            &device_id,
            &firmware_info,
            &hardware_info,
            &executor_index,
            &error)) {
        g_printerr ("error: couldn't parse response message: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    device_type_str    = mbim_device_type_get_string (device_type);
    cellular_class_str = mbim_cellular_class_build_string_from_mask (cellular_class);
    voice_class_str    = mbim_voice_class_get_string (voice_class);
    sim_class_str      = mbim_sim_class_build_string_from_mask (sim_class);
    data_class_str     = mbim_data_class_build_string_from_mask (data_class);
    sms_caps_str       = mbim_sms_caps_build_string_from_mask (sms_caps);
    ctrl_caps_str      = mbim_ctrl_caps_build_string_from_mask (ctrl_caps);

    g_print ("[%s] Device capabilities retrieved:\n"
             "\t      Device type: '%s'\n"
             "\t   Cellular class: '%s'\n"
             "\t      Voice class: '%s'\n"
             "\t        SIM class: '%s'\n"
             "\t       Data class: '%s'\n"
             "\t         SMS caps: '%s'\n"
             "\t        Ctrl caps: '%s'\n"
             "\t     Max sessions: '%u'\n"
             "\tCustom data class: '%s'\n"
             "\t        Device ID: '%s'\n"
             "\t    Firmware info: '%s'\n"
             "\t    Hardware info: '%s'\n"
             "\t   Executor Index: '%u'\n",
             mbim_device_get_path_display (device),
             VALIDATE_UNKNOWN (device_type_str),
             VALIDATE_UNKNOWN (cellular_class_str),
             VALIDATE_UNKNOWN (voice_class_str),
             VALIDATE_UNKNOWN (sim_class_str),
             VALIDATE_UNKNOWN (data_class_str),
             VALIDATE_UNKNOWN (sms_caps_str),
             VALIDATE_UNKNOWN (ctrl_caps_str),
             max_sessions,
             VALIDATE_UNKNOWN (custom_data_class),
             VALIDATE_UNKNOWN (device_id),
             VALIDATE_UNKNOWN (firmware_info),
             VALIDATE_UNKNOWN (hardware_info),
             executor_index);

    shutdown (TRUE);
}

static gboolean
query_slot_information_status_slot_index_parse (const gchar *str,
                                                guint32     *slot_index,
                                                GError      **error)
{
    gchar *endptr = NULL;
    gint64 n;

    g_assert (str != NULL);
    g_assert (slot_index != NULL);

    if (!str[0]) {
        g_set_error (error,
                     MBIM_CORE_ERROR,
                     MBIM_CORE_ERROR_FAILED,
                     "slot index not given");
        return FALSE;
    }

    errno = 0;
    n = g_ascii_strtoll (str, &endptr, 10);
    if (errno || ((size_t)(endptr - str) < strlen (str))) {
        g_set_error (error,
                     MBIM_CORE_ERROR,
                     MBIM_CORE_ERROR_FAILED,
                     "couldn't parse slot index '%s'",
                     str);
        return FALSE;
    }
    *slot_index = (guint32) n;

    return TRUE;
}

static void
query_slot_information_status_ready (MbimDevice   *device,
                                     GAsyncResult *res)
{
    g_autoptr(MbimMessage) response = NULL;
    g_autoptr(GError)      error = NULL;
    guint32                slot_index;
    MbimUiccSlotState      slot_state;
    const gchar           *slot_state_str;

    response = mbim_device_command_finish (device, res, &error);
    if (!response || !mbim_message_response_get_result (response, MBIM_MESSAGE_TYPE_COMMAND_DONE, &error)) {
        g_printerr ("error: operation failed: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    if (!mbim_message_ms_basic_connect_extensions_slot_info_status_response_parse (
                response,
                &slot_index,
                &slot_state,
                &error)) {
        g_printerr ("error: conldn't parse response message: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    slot_state_str = mbim_uicc_slot_state_get_string (slot_state);

    g_print ("[%s] Slot info status retrieved:\n"
             "\t        Slot '%u': '%s'\n",
             mbim_device_get_path_display (device),
             slot_index,
             VALIDATE_UNKNOWN (slot_state_str));
    shutdown (TRUE);
}

static gboolean
set_device_slot_mappings_input_parse (const gchar *str,
                                      GPtrArray   **slot_array,
                                      GError      **error)
{
    g_auto(GStrv) split = NULL;
    gchar        *endptr = NULL;
    gint64        n;
    MbimSlot     *slot_index;
    guint32       i = 0;

    g_assert (slot_array != NULL);

    split = g_strsplit (str, ",", 0);

    if (g_strv_length (split) < 1) {
        g_set_error (error,
                     MBIM_CORE_ERROR,
                     MBIM_CORE_ERROR_FAILED,
                     "missing arguments");
        return FALSE;
    }

    *slot_array = g_ptr_array_new_with_free_func (g_free);

    while (split[i] != NULL) {
        errno = 0;
        n = g_ascii_strtoll (split[i], &endptr, 10);
        if (errno || n < 0 || n > G_MAXUINT32 || ((size_t)(endptr - split[i]) < strlen (split[i]))) {
            g_set_error (error,
                         MBIM_CORE_ERROR,
                         MBIM_CORE_ERROR_FAILED,
                         "couldn't parse device slot index '%s'",
                         split[i]);
            return FALSE;
        }
        slot_index = g_new (MbimSlot, 1);
        slot_index->slot = (guint32) n;
        g_ptr_array_add (*slot_array, slot_index);
        i++;
    }

    return TRUE;
}

static void
query_device_slot_mappings_ready (MbimDevice   *device,
                                  GAsyncResult *res)
{
    g_autoptr(MbimMessage)   response = NULL;
    g_autoptr(GError)        error = NULL;
    guint32                  map_count = 0;
    g_autoptr(MbimSlotArray) slot_mappings = NULL;
    guint                    i;

    response = mbim_device_command_finish (device, res, &error);
    if (!response || !mbim_message_response_get_result (response, MBIM_MESSAGE_TYPE_COMMAND_DONE, &error)) {
        g_printerr ("error: operation failed: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    if (!mbim_message_ms_basic_connect_extensions_device_slot_mappings_response_parse (
            response,
            &map_count,
            &slot_mappings,
            &error)) {
        g_printerr ("error: couldn't parse response message: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    if (set_device_slot_mappings_str) {
        g_print ("[%s] Updated slot mappings retrieved:\n",
                 mbim_device_get_path_display (device));
    } else {
         g_print ("[%s] Slot mappings retrieved:\n",
                  mbim_device_get_path_display (device));
    }

    for (i = 0; i < map_count; i++) {
        g_print ("\t Executor '%u': slot '%u'\n",
                 i,
                 slot_mappings[i]->slot);
    }

    shutdown (TRUE);
}

static void
query_location_info_status_ready (MbimDevice   *device,
                                  GAsyncResult *res)
{
    g_autoptr(MbimMessage) response = NULL;
    g_autoptr(GError)      error = NULL;
    guint32                location_area_code = 0;
    guint32                tracking_area_code = 0;
    guint32                cell_id = 0;

    response = mbim_device_command_finish (device, res, &error);
    if (!response || !mbim_message_response_get_result (response, MBIM_MESSAGE_TYPE_COMMAND_DONE, &error)) {
        g_printerr ("error: operation failed: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    g_print ("[%s] Successfully queried location info status\n",
             mbim_device_get_path_display (device));

    if (!mbim_message_ms_basic_connect_extensions_location_info_status_response_parse (
            response,
            &location_area_code,
            &tracking_area_code,
            &cell_id,
            &error)) {
        g_printerr ("error: couldn't parse response message: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    g_print (" Location area code:  %04X\n", location_area_code);
    g_print (" Tracking area code:  %06X\n", tracking_area_code);
    g_print (" Cell ID:             %08X\n", cell_id);

    shutdown (TRUE);
}

static void
query_version_ready (MbimDevice   *device,
                     GAsyncResult *res)
{
    g_autoptr(MbimMessage) response = NULL;
    g_autoptr(GError)      error = NULL;
    guint16                mbim_version;
    guint16                mbim_ext_version;

    response = mbim_device_command_finish (device, res, &error);
    if (!response || !mbim_message_response_get_result (response, MBIM_MESSAGE_TYPE_COMMAND_DONE, &error)) {
        g_printerr ("error: operation failed: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    g_print ("[%s] Successfully exchanged version information\n",
             mbim_device_get_path_display (device));
    if (!mbim_message_ms_basic_connect_extensions_version_response_parse (
            response,
            &mbim_version,
            &mbim_ext_version,
            &error)) {
        g_printerr ("error: couldn't parse response message: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    g_print (" MBIM version          : %x.%02x\n", mbim_version >> 8, mbim_version & 0xFF);
    g_print (" MBIM extended version : %x.%02x\n", mbim_ext_version >> 8, mbim_ext_version & 0xFF);

    shutdown (TRUE);
    return;
}

static gboolean
mbim_auth_protocol_from_string (const gchar      *str,
                                MbimAuthProtocol *auth_protocol)
{
    if (g_ascii_strcasecmp (str, "0") == 0) {
        *auth_protocol = MBIM_AUTH_PROTOCOL_NONE;
        return TRUE;
    }
    if (g_ascii_strcasecmp (str, "1") == 0) {
        *auth_protocol = MBIM_AUTH_PROTOCOL_PAP;
        return TRUE;
    }
    if (g_ascii_strcasecmp (str, "2") == 0) {
        *auth_protocol = MBIM_AUTH_PROTOCOL_CHAP;
        return TRUE;
    }
    if (g_ascii_strcasecmp (str, "3") == 0) {
        *auth_protocol = MBIM_AUTH_PROTOCOL_MSCHAPV2;
        return TRUE;
    }

    return FALSE;
}
static gboolean
mbim_context_ip_type_from_string (const gchar       *str,
                                  MbimContextIpType *ip_type)
{
    if (g_ascii_strcasecmp (str, "0") == 0) {
        *ip_type = MBIM_CONTEXT_IP_TYPE_DEFAULT;
        return TRUE;
    }
    if (g_ascii_strcasecmp (str, "1") == 0) {
        *ip_type = MBIM_CONTEXT_IP_TYPE_IPV4;
        return TRUE;
    }
    if (g_ascii_strcasecmp (str, "2") == 0) {
        *ip_type = MBIM_CONTEXT_IP_TYPE_IPV6;
        return TRUE;
    }
    if (g_ascii_strcasecmp (str, "3") == 0) {
        *ip_type = MBIM_CONTEXT_IP_TYPE_IPV4V6;
        return TRUE;
    }
    if (g_ascii_strcasecmp (str, "4") == 0) {
        *ip_type = MBIM_CONTEXT_IP_TYPE_IPV4_AND_IPV6;
        return TRUE;
    }

    return FALSE;
}

static gboolean
mbim_context_operation_from_string (const gchar          *str,
                                    MbimContextOperation *operation)
{
    if (g_ascii_strcasecmp (str, "0") == 0) {
        *operation = MBIM_CONTEXT_OPERATION_DEFAULT;
        return TRUE;
    }
    if (g_ascii_strcasecmp (str, "1") == 0) {
        *operation = MBIM_CONTEXT_OPERATION_DELETE;
        return TRUE;
    }
    if (g_ascii_strcasecmp (str, "2") == 0) {
        *operation = MBIM_CONTEXT_OPERATION_RESTORE_FACTORY;
        return TRUE;
    }

    return FALSE;
}

static gboolean
mbim_context_state_from_string (const gchar      *str,
                                MbimContextState *enable)
{
    if (g_ascii_strcasecmp (str, "1") == 0) {
        *enable = MBIM_CONTEXT_STATE_ENABLED;
        return TRUE;
    }
    if (g_ascii_strcasecmp (str, "0") == 0) {
        *enable = MBIM_CONTEXT_STATE_DISABLED;
        return TRUE;
    }

    return FALSE;
}

static gboolean
mbim_compression_from_string (const gchar     *str,
                              MbimCompression *compression)
{
    if (g_ascii_strcasecmp (str, "1") == 0) {
        *compression = MBIM_COMPRESSION_ENABLE;
        return TRUE;
    }
        if (g_ascii_strcasecmp (str, "0") == 0) {
        *compression = MBIM_COMPRESSION_NONE;
        return TRUE;
    }
    return FALSE;
}
static gboolean
mbim_roaming_control_from_string (const gchar               *str,
                                  MbimContextRoamingControl *roaming)
{
    if (g_ascii_strcasecmp (str, "0") == 0) {
        *roaming = MBIM_CONTEXT_ROAMING_CONTROL_HOME_ONLY;
        return TRUE;
    }
    if (g_ascii_strcasecmp (str, "1") == 0) {
        *roaming = MBIM_CONTEXT_ROAMING_CONTROL_PARTNER_ONLY;
        return TRUE;
    }
    if (g_ascii_strcasecmp (str, "2") == 0) {
        *roaming = MBIM_CONTEXT_ROAMING_CONTROL_NON_PARTNER_ONLY;
        return TRUE;
    }
    if (g_ascii_strcasecmp (str, "4") == 0) {
        *roaming = MBIM_CONTEXT_ROAMING_CONTROL_HOME_AND_NON_PARTNER;
        return TRUE;
    }
    if (g_ascii_strcasecmp (str, "5") == 0) {
        *roaming = MBIM_CONTEXT_ROAMING_CONTROL_PARTNER_AND_NON_PARTNER;
        return TRUE;
    }
    if (g_ascii_strcasecmp (str, "3") == 0) {
        *roaming = MBIM_CONTEXT_ROAMING_CONTROL_HOME_AND_PARTNER;
        return TRUE;
    }
    if (g_ascii_strcasecmp (str, "6") == 0) {
        *roaming = MBIM_CONTEXT_ROAMING_CONTROL_ALLOW_ALL;
        return TRUE;
    }

    return FALSE;
}

static gboolean
mbim_context_media_from_string (const gchar          *str,
                                MbimContextMediaType *media_type)
{
    if (g_ascii_strcasecmp (str, "0") == 0) {
        *media_type = MBIM_CONTEXT_MEDIA_TYPE_CELLULAR_ONLY;
        return TRUE;
    }
    if (g_ascii_strcasecmp (str, "1") == 0) {
        *media_type = MBIM_CONTEXT_MEDIA_TYPE_WIFI_ONLY;
        return TRUE;
    }
    if (g_ascii_strcasecmp (str, "2") == 0) {
        *media_type = MBIM_CONTEXT_MEDIA_TYPE_ALL;
        return TRUE;
    }

    return FALSE;
}

static gboolean
mbim_context_source_from_string (const gchar       *str,
                                 MbimContextSource *source)
{
    if (g_ascii_strcasecmp (str, "0") == 0) {
        *source = MBIM_CONTEXT_SOURCE_ADMIN;
        return TRUE;
    }
    if (g_ascii_strcasecmp (str, "1") == 0) {
        *source = MBIM_CONTEXT_SOURCE_USER;
        return TRUE;
    }
    if (g_ascii_strcasecmp (str, "2") == 0) {
        *source = MBIM_CONTEXT_SOURCE_OPERATOR;
        return TRUE;
    }
    if (g_ascii_strcasecmp (str, "3") == 0) {
        *source = MBIM_CONTEXT_SOURCE_MODEM;
        return TRUE;
    }
    if (g_ascii_strcasecmp (str, "4") == 0) {
        *source = MBIM_CONTEXT_SOURCE_DEVICE;
        return TRUE;
    }

    return FALSE;
}

static gboolean
mbim_context_type_from_string (const gchar     *str,
                               MbimContextType *context_type)
{
    if (g_ascii_strcasecmp (str, "0") == 0) {
        *context_type = MBIM_CONTEXT_TYPE_INVALID;
        return TRUE;
    }
    if (g_ascii_strcasecmp (str, "b43f758c-a560-4b46-b35e-c5869641fb54") == 0) {
        *context_type = MBIM_CONTEXT_TYPE_NONE;
        return TRUE;
    }
    if (g_ascii_strcasecmp (str, "7e5e2a7e-4e6f-7272-736b-656e7e5e2a7e") == 0) {
        *context_type = MBIM_CONTEXT_TYPE_INTERNET ;
        return TRUE;
    }
    if (g_ascii_strcasecmp (str, "9b9f7bbe-8952-44b7-83ac-ca41318df7a0") == 0) {
        *context_type = MBIM_CONTEXT_TYPE_VPN;
        return TRUE;
    }
    if (g_ascii_strcasecmp (str, "b43f758c-a560-4b46-b35e-c5869641fb54") == 0) {
        *context_type = MBIM_CONTEXT_TYPE_VOICE;
        return TRUE;
    }
    if (g_ascii_strcasecmp (str, "05a2a716-7c34-4b4d-9a91-c5ef0c7aaacc") == 0) {
        *context_type = MBIM_CONTEXT_TYPE_VIDEO_SHARE;
        return TRUE;
    }
    if (g_ascii_strcasecmp (str, "b3272496-ac6c-422b-a8c0-acf687a27217") == 0) {
        *context_type = MBIM_CONTEXT_TYPE_PURCHASE;
        return TRUE;
    }
    if (g_ascii_strcasecmp (str, "21610d01-3074-4bce-9425-b53a07d697d6") == 0) {
        *context_type = MBIM_CONTEXT_TYPE_IMS;
        return TRUE;
    }
    if (g_ascii_strcasecmp (str, "46726664-7269-6bc6-9624-d1d35389aca9") == 0) {
        *context_type = MBIM_CONTEXT_TYPE_MMS;
        return TRUE;
    }
    if (g_ascii_strcasecmp (str, "a57a9afc-b09f-45d7-bb40-033c39f60db9") == 0) {
        *context_type = MBIM_CONTEXT_TYPE_LOCAL;
        return TRUE;
    }

    return FALSE;
}

typedef struct {
    MbimContextOperation       operation;
    MbimContextIpType          ip_type;
    MbimContextState           state;
    MbimContextRoamingControl  roaming;
    MbimContextMediaType       media_type;
    MbimContextSource          source;
    gchar                     *access_string;
    gchar                     *user_name;
    gchar                     *password;
    MbimCompression            compression;
    MbimAuthProtocol           auth_protocol;
    MbimContextType            context_type;
} provisioncontextv2;

static void
provision_context_clear (provisioncontextv2 *props)
{
    g_free (props->access_string);
    g_free (props->user_name);
    g_free (props->password);
}

G_DEFINE_AUTO_CLEANUP_CLEAR_FUNC(provisioncontextv2, provision_context_clear);

static gboolean
provision_context_parse (const gchar        *str,
                         MbimContextOperation *operation,
                         MbimContextIpType *ip_type,
                         MbimContextState *state,
                         MbimContextRoamingControl *roaming,
                         MbimContextMediaType *media_type,
                         MbimContextSource *source,
                         gchar **access_string,
                         gchar **user_name,
                         gchar **password,
                         MbimCompression *compression,
                         MbimAuthProtocol *auth_protocol,
                         MbimContextType *context_type)
{
    g_auto(provisioncontextv2) props = {
        .access_string = NULL,
        .operation     = MBIM_CONTEXT_OPERATION_DELETE,
        .auth_protocol = MBIM_AUTH_PROTOCOL_NONE,
        .user_name     = NULL,
        .password      = NULL,
        .ip_type       = MBIM_CONTEXT_IP_TYPE_DEFAULT,
        .state         = MBIM_CONTEXT_STATE_DISABLED,
        .roaming       = MBIM_CONTEXT_ROAMING_CONTROL_HOME_ONLY,
        .media_type    = MBIM_CONTEXT_MEDIA_TYPE_CELLULAR_ONLY,
        .source        = MBIM_CONTEXT_SOURCE_ADMIN,
        .compression   = MBIM_COMPRESSION_NONE,
        .context_type  = MBIM_CONTEXT_TYPE_INVALID
    };
    g_auto(GStrv)     split = NULL;
    g_autoptr(GError) error = NULL;

    g_assert (access_string != NULL);
    g_assert (operation != NULL);
    g_assert (auth_protocol != NULL);
    g_assert (user_name != NULL);
    g_assert (password != NULL);
    g_assert (ip_type != NULL);
    g_assert (state != NULL);
    g_assert (roaming != NULL);
    g_assert (media_type != NULL);
    g_assert (source != NULL);
    g_assert (compression != NULL);
    g_assert (context_type != NULL);
    split = g_strsplit (str, ",", -1);

    if (g_strv_length (split) > 12) {
        g_printerr ("error: couldn't parse input string, too many arguments\n");
        return FALSE;
    }

    if (g_strv_length (split) < 12) {
        g_printerr ("error: couldn't parse input string, too few arguments\n");
        return FALSE;
    }

    if (g_strv_length (split) > 0) {
        /* Use authentication method */
        if (split[0]) {
            if (!mbim_context_operation_from_string (split[0], &props.operation)) {
                g_printerr ("error: couldn't parse input string, operation '%s'\n", split[0]);
            }
        }/* Use authentication method */
        if (split[1]) {
            if (!mbim_context_type_from_string (split[1], &props.context_type)) {
                g_printerr ("error: couldn't parse input string, contexttype '%s'\n", split[1]);
            }
        }
        /* Use authentication method */
        if (split[2]) {
            if (!mbim_context_ip_type_from_string (split[2], &props.ip_type)) {
                g_printerr ("error: couldn't parse input string, ip_type '%s'\n", split[2]);
            }
        }
        /* Use authentication method */
        if (split[3]) {
            if (!mbim_context_state_from_string (split[3], &props.state)) {
                g_printerr ("error: couldn't parse input string, enable '%s'\n", split[3]);
            }
        }
        /* Use authentication method */
        if (split[4]) {
            if (!mbim_roaming_control_from_string (split[4], &props.roaming)) {
                g_printerr ("error: couldn't parse input string, roaming '%s'\n", split[4]);
            }
        }
        /* Use authentication method */
        if (split[5]) {
            if (!mbim_context_media_from_string (split[5], &props.media_type)) {
                g_printerr ("error: couldn't parse input string, media_type '%s'\n", split[5]);
            }
        }
        /* Use authentication method */
        if (split[6]) {
            if (!mbim_context_source_from_string (split[6], &props.source)) {
                g_printerr ("error: couldn't parse input string, source '%s'\n", split[6]);
            }
        }
        /* Use authentication method */
        if (split[11]) {
            if (!mbim_auth_protocol_from_string (split[11], &props.auth_protocol)) {
                g_printerr ("error: couldn't parse input string, unknown auth protocol '%s'\n", split[11]);
            }
        }
        /* Use authentication method */
        if (split[10]) {
            if (!mbim_compression_from_string (split[10], &props.compression)) {
                g_printerr ("error: couldn't parse input string, unknown compression '%s'\n", split[10]);
            }
        }
        if (split[7]){
            /* Username */
            props.user_name = g_strdup (split[7]);
            /* Password */
            props.password = g_strdup (split[8]);
            /* accessstring */
            props.access_string = g_strdup (split[9]);
        }
    }
    if (props.auth_protocol == MBIM_AUTH_PROTOCOL_NONE) {
        if (props.user_name || props.password) {
            g_printerr ("error: username or password requires an auth protocol\n");
            return FALSE;
        }
    } else {
        if (!props.user_name) {
            g_printerr ("error: auth protocol requires a username\n");
            return FALSE;
        }
    }

    *access_string = g_steal_pointer (&props.access_string);
    *auth_protocol = props.auth_protocol;
    *user_name     = g_steal_pointer (&props.user_name);
    *password      = g_steal_pointer (&props.password);
    *ip_type       = props.ip_type;
    *operation     = props.operation;
    *state         = props.state;
    *roaming       = props.roaming;
    *media_type    = props.media_type;
    *source        = props.source;
    *compression   = props.compression;
    *context_type  = props.context_type;

    return TRUE;
}

static void
provisioned_contexts_v2_ready (MbimDevice   *device,
                               GAsyncResult *res)
{
    g_autoptr(MbimMessage)                          response = NULL;
    g_autoptr(MbimProvisionedContextElementV2Array) provisioned_contexts = NULL;
    g_autoptr(GError)                               error = NULL;
    guint32 provisioned_contexts_count;
    guint32 i = 0;

    response = mbim_device_command_finish (device, res, &error);
    if (!response || !mbim_message_response_get_result (response, MBIM_MESSAGE_TYPE_COMMAND_DONE, &error)) {
        g_printerr ("error: operation failed: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    if (!mbim_message_ms_basic_connect_extensions_provisioned_contexts_response_parse (
            response,
            &provisioned_contexts_count,
            &provisioned_contexts,
            &error)) {
        g_printerr ("error: couldn't parse response message: %s\n", error->message);
        shutdown (FALSE);
        return;
    }
    g_print ("[%s] Provisioned contexts (%u):\n",
             mbim_device_get_path_display (device),
             provisioned_contexts_count);

    for (i = 0; i < provisioned_contexts_count; i++) {
        g_print ("\tContext ID %u:\n"
                 "\t   Context type: '%s'\n"
                 "\t        ip type: '%s'\n"
                 "\t        state:   '%s'\n"
                 "\t        roaming: '%s'\n"
                 "\t     media_type: '%s'\n"
                 "\t         source: '%s'\n"
                 "\t  Access string: '%s'\n"
                 "\t       Username: '%s'\n"
                 "\t       Password: '%s'\n"
                 "\t    Compression: '%s'\n"
                 "\t  Auth protocol: '%s'\n",
                 provisioned_contexts[i]->context_id,
                 VALIDATE_UNKNOWN (mbim_context_type_get_string (
                     mbim_uuid_to_context_type (&provisioned_contexts[i]->context_type))),
                 VALIDATE_UNKNOWN (mbim_context_ip_type_get_string (
                     provisioned_contexts[i]->ip_type)),
                 VALIDATE_UNKNOWN (mbim_context_state_get_string (
                     provisioned_contexts[i]->state)),
                 VALIDATE_UNKNOWN (mbim_context_roaming_control_get_string (
                     provisioned_contexts[i]->roaming)),
                 VALIDATE_UNKNOWN (mbim_context_media_type_get_string (
                     provisioned_contexts[i]->media_type)),
                 VALIDATE_UNKNOWN (mbim_context_source_get_string (
                     provisioned_contexts[i]->source)),
                 VALIDATE_UNKNOWN (provisioned_contexts[i]->access_string),
                 VALIDATE_UNKNOWN (provisioned_contexts[i]->user_name),
                 VALIDATE_UNKNOWN (provisioned_contexts[i]->password),
                 VALIDATE_UNKNOWN (mbim_compression_get_string (
                     provisioned_contexts[i]->compression)),
                 VALIDATE_UNKNOWN (mbim_auth_protocol_get_string (
                     provisioned_contexts[i]->auth_protocol)));
    }

    mbim_message_unref (response);
    shutdown (TRUE);
}

void
mbimcli_ms_basic_connect_extensions_run (MbimDevice   *device,
                                         GCancellable *cancellable)
{
    g_autoptr(MbimMessage) request = NULL;
    g_autoptr(GError)      error = NULL;

    /* Initialize context */
    ctx = g_slice_new (Context);
    ctx->device = g_object_ref (device);
    ctx->cancellable = cancellable ? g_object_ref (cancellable) : NULL;

    /* Request to get PCO? */
    if (query_pco_str) {
        MbimPcoValue pco_value;

        if (!session_id_parse (query_pco_str, &pco_value.session_id, &error)) {
            g_printerr ("error: couldn't parse session ID: %s\n", error->message);
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
        return;
    }

    if (query_lte_attach_configuration_flag) {
        g_debug ("Asynchronously querying LTE attach configuration...");
        request = mbim_message_ms_basic_connect_extensions_lte_attach_configuration_query_new (NULL);
        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)query_lte_attach_configuration_ready,
                             NULL);
        return;
    }

    if (query_lte_attach_status_flag || query_lte_attach_info_flag) {
        g_debug ("Asynchronously querying LTE attach info...");
        request = mbim_message_ms_basic_connect_extensions_lte_attach_info_query_new (NULL);
        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)query_lte_attach_info_ready,
                             NULL);
        return;
    }

    if (query_sys_caps_flag) {
        g_debug ("Asynchronously querying system capabilities...");
        request = mbim_message_ms_basic_connect_extensions_sys_caps_query_new (NULL);
        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)query_sys_caps_ready,
                             NULL);
        return;
    }

    if (query_device_caps_flag) {
        g_debug ("Asynchronously querying device capabilities...");
        request = mbim_message_ms_basic_connect_extensions_device_caps_query_new (NULL);
        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)query_device_caps_ready,
                             NULL);
        return;
    }

    if (query_slot_info_status_str) {
        guint32 slot_index;

        if (!query_slot_information_status_slot_index_parse (query_slot_info_status_str, &slot_index, &error)) {
            g_printerr ("error: couldn't parse slot index: %s\n", error->message);
            shutdown (FALSE);
            return;
        }

        g_debug ("Asynchronously querying slot information status...");
        request = mbim_message_ms_basic_connect_extensions_slot_info_status_query_new (slot_index, NULL);
        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)query_slot_information_status_ready,
                             NULL);
        return;
    }

    if (set_device_slot_mappings_str) {
        g_autoptr(GPtrArray) slot_array = NULL;

        g_print ("Asynchronously set device slot mappings\n");
        if (!set_device_slot_mappings_input_parse (set_device_slot_mappings_str, &slot_array, &error)) {
            g_printerr ("error: couldn't parse setting argument: %s\n", error->message);
            shutdown (FALSE);
            return;
        }

        request = mbim_message_ms_basic_connect_extensions_device_slot_mappings_set_new (slot_array->len,
                                                                                         (const MbimSlot **)slot_array->pdata,
                                                                                         NULL);
        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)query_device_slot_mappings_ready,
                             NULL);
        return;
    }

    if (query_device_slot_mappings_flag) {
        g_debug ("Asynchronously querying device slot mappings...");
        request = mbim_message_ms_basic_connect_extensions_device_slot_mappings_query_new (NULL);
        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)query_device_slot_mappings_ready,
                             NULL);
        return;
    }

    if (query_location_info_status_flag) {
        g_debug ("Asynchronously querying location info status...");
        request = mbim_message_ms_basic_connect_extensions_location_info_status_query_new (NULL);
        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)query_location_info_status_ready,
                             NULL);
        return;
    }

    if (query_version_str) {
        guint16       bcd_mbim_version = 0;
        guint16       bcd_mbim_extended_version = 0;
        guint8        mbim_version_major = 0;
        guint8        mbim_version_minor = 0;
        guint8        mbim_extended_version_major = 0;
        guint8        mbim_extended_version_minor = 0;
        g_auto(GStrv) split = NULL;
        g_auto(GStrv) mbim_version = NULL;
        g_auto(GStrv) mbim_extended_version = NULL;

        split = g_strsplit (query_version_str, ",", -1);

        if (g_strv_length (split) > 2) {
            g_printerr ("error: couldn't parse input string, too many arguments\n");
            return;
        }

        if (g_strv_length (split) < 2) {
            g_printerr ("error: couldn't parse input string, missing arguments\n");
            return;
        }

        mbim_version = g_strsplit (split[0], ".", -1);
        if (!mbimcli_read_uint8_from_bcd_string (mbim_version[0], &mbim_version_major) ||
            !mbimcli_read_uint8_from_bcd_string (mbim_version[1], &mbim_version_minor)) {
            g_printerr ("error: couldn't parse version string\n");
            return;
        }
        bcd_mbim_version = mbim_version_major << 8 | mbim_version_minor;
        g_debug ("BCD version built: 0x%x", bcd_mbim_version);

        mbim_extended_version = g_strsplit (split[1], ".", -1);
        if (!mbimcli_read_uint8_from_bcd_string (mbim_extended_version[0], &mbim_extended_version_major) ||
            !mbimcli_read_uint8_from_bcd_string (mbim_extended_version[1], &mbim_extended_version_minor)) {
            g_printerr ("error: couldn't parse extended version string\n");
            return;
        }
        bcd_mbim_extended_version = mbim_extended_version_major << 8 | mbim_extended_version_minor;
        g_debug ("BCD extended version built: 0x%x", bcd_mbim_extended_version);

        g_debug ("Asynchronously querying Version...");
        request = mbim_message_ms_basic_connect_extensions_version_query_new (bcd_mbim_version, bcd_mbim_extended_version, NULL);
        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)query_version_ready,
                             NULL);
        return;
    }
    if (set_provisioned_contexts_v2_flag) {
        MbimContextOperation operation;
        MbimContextType context_type;
        MbimContextIpType  ip_type = MBIM_CONTEXT_IP_TYPE_DEFAULT;
        MbimContextState state;
        MbimContextRoamingControl roaming;
        MbimContextMediaType media_type;
        MbimContextSource source;
        g_autofree gchar *access_string = NULL;
        g_autofree gchar *user_name = NULL;
        g_autofree gchar *password = NULL;
        MbimCompression compression;
        MbimAuthProtocol auth_protocol;

        if (!provision_context_parse (set_provisioned_contexts_v2_flag,
                                         &operation,
                                         &ip_type,
                                         &state,
                                         &roaming,
                                         &media_type,
                                         &source,
                                         &access_string,
                                         &user_name,
                                         &password,
                                         &compression,
                                         &auth_protocol,
                                         &context_type)) {
            shutdown (FALSE);
            return;
        }

        request = mbim_message_ms_basic_connect_extensions_provisioned_contexts_set_new (
                      operation,
                      mbim_uuid_from_context_type (context_type),
                      ip_type,
                      state,
                      roaming,
                      media_type,
                      source,
                      access_string,
                      user_name,
                      password,
                      compression,
                      auth_protocol,
                      &error);

        if (!request) {
            g_printerr ("error: couldn't create request: %s\n", error->message);
            shutdown (FALSE);
            return;
        }

        mbim_device_command (ctx->device,
                             request,
                             60,
                             ctx->cancellable,
                             (GAsyncReadyCallback)provisioned_contexts_v2_ready,
                             NULL);
        return;
    }

    /* Request to query Provisioned contexts? */
    if (query_provisioned_contexts_v2_flag) {
        g_debug ("Asynchronously query provisioned contexts...");

        request = mbim_message_ms_basic_connect_extensions_provisioned_contexts_query_new (NULL);
        if (!request) {
            g_printerr ("error: couldn't create request: %s\n", error->message);
            shutdown (FALSE);
            return;
        }
        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)provisioned_contexts_v2_ready,
                             NULL);
        return;
    }

    g_warn_if_reached ();
}
