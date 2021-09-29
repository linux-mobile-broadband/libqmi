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
static gboolean  query_provisioned_contexts_flag;
static gchar    *set_provisioned_contexts_str;
static gboolean  query_base_stations_flag;
static gchar    *query_version_str;
static gboolean  query_registration_parameters_flag;
static gchar    *set_registration_parameters_str;
static gboolean  query_modem_configuration_flag;

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
    { "ms-set-provisioned-contexts", 0, 0, G_OPTION_ARG_STRING, &set_provisioned_contexts_str,
      "Set provisioned contexts (allowed keys: operation, context-type, ip-type, state, roaming-control, media-type, source, auth, compression, username, password, access-string)",
      "[\"key=value,...\"]"
    },
    { "ms-query-provisioned-contexts", 0, 0, G_OPTION_ARG_NONE, &query_provisioned_contexts_flag,
      "Query provisioned contexts",
      NULL
    },
    { "ms-query-base-stations-info", 0, 0, G_OPTION_ARG_NONE, &query_base_stations_flag,
      "Query base stations info",
      NULL
    },
    { "ms-query-version", 0, 0,G_OPTION_ARG_STRING , &query_version_str,
      "Exchange supported version information. Since MBIMEx v2.0.",
      "[(MBIM version),(MBIM extended version)]"
    },
    { "ms-query-registration-parameters", 0, 0, G_OPTION_ARG_NONE, &query_registration_parameters_flag,
      "Query registration parameters. Since MBIMEx v3.0.",
      NULL
    },
    { "ms-set-registration-parameters", 0, 0,G_OPTION_ARG_STRING , &set_registration_parameters_str,
      "Set registration parameters (required keys: mico-mode, drx-cycle, ladn-info, default-pdu-activation-hint, re-register-if-needed). Since MBIMEx v3.0.",
      "[\"key=value,...\"]"
    },
    { "ms-query-modem-configuration", 0, 0, G_OPTION_ARG_NONE, &query_modem_configuration_flag,
      "Query modem configuration. Since MBIMEx v3.0.",
      NULL
    },
    { NULL }
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
                 query_provisioned_contexts_flag +
                 !!set_provisioned_contexts_str +
                 query_base_stations_flag +
                 !!query_version_str +
                 query_registration_parameters_flag +
                 !!set_registration_parameters_str +
                 query_modem_configuration_flag);

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
    MbimNwError             nw_error = 0;

    response = mbim_device_command_finish (device, res, &error);
    if (!response || !mbim_message_response_get_result (response, MBIM_MESSAGE_TYPE_COMMAND_DONE, &error)) {
        g_printerr ("error: operation failed: %s\n", error->message);
        shutdown (FALSE);
        return;
    }


    /* MBIMEx 3.0 support */
    if (mbim_device_check_ms_mbimex_version (device, 3, 0)) {
        if (!mbim_message_ms_basic_connect_extensions_v3_lte_attach_info_response_parse (
                response,
                &lte_attach_state,
                &nw_error,
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
        g_print ("[%s] Successfully received v3.0 LTE attach info\n",
                 mbim_device_get_path_display (device));
    }
    /* MBIM 1.0 support */
    else {
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
        g_print ("[%s] Successfully received v1.0 LTE attach info\n",
                 mbim_device_get_path_display (device));
    }

#define VALIDATE_NA(str) (str ? str : "n/a")
    g_print ("  Attach state:  %s\n", mbim_lte_attach_state_get_string (lte_attach_state));
    g_print ("  IP type:       %s\n", mbim_context_ip_type_get_string (ip_type));
    g_print ("  Access string: %s\n", VALIDATE_NA (access_string));
    g_print ("  Username:      %s\n", VALIDATE_NA (user_name));
    g_print ("  Password:      %s\n", VALIDATE_NA (password));
    g_print ("  Compression:   %s\n", mbim_compression_get_string (compression));
    g_print ("  Auth protocol: %s\n", mbim_auth_protocol_get_string (auth_protocol));
    if (mbim_device_check_ms_mbimex_version (device, 3, 0)) {
        if (nw_error == 0)
            g_print ("  Network error: none\n");
        else if (nw_error == 0xFFFFFFFF)
            g_print ("  Network error: unknown\n");
        else {
            const gchar *nw_error_str;

            nw_error_str = mbim_nw_error_get_string (nw_error);
            if (nw_error_str)
                g_print ("  Network error: %s\n", nw_error_str);
            else
                g_print ("  Network error: unknown (0x%08x)\n", nw_error);
        }
    }
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
    MbimDataClass           data_class = 0;
    MbimDataClassV3         data_class_v3 = 0;
    g_autofree gchar       *data_class_str = NULL;
    MbimDataSubclass        data_subclass;
    MbimSmsCaps             sms_caps;
    g_autofree gchar       *sms_caps_str = NULL;
    MbimCtrlCaps            ctrl_caps;
    g_autofree gchar       *ctrl_caps_str = NULL;
    guint32                 max_sessions;
    guint32                 wcdma_band_class = 0;
    guint32                 lte_band_class_array_size = 0;
    g_autofree guint16     *lte_band_class_array = NULL;
    guint32                 nr_band_class_array_size = 0;
    g_autofree guint16     *nr_band_class_array = NULL;
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

    if (mbim_device_check_ms_mbimex_version (device, 3, 0)) {
        if (!mbim_message_ms_basic_connect_extensions_v3_device_caps_response_parse (
                response,
                &device_type,
                &cellular_class,
                &voice_class,
                &sim_class,
                &data_class_v3,
                &sms_caps,
                &ctrl_caps,
                &data_subclass,
                &max_sessions,
                &executor_index,
                &wcdma_band_class,
                &lte_band_class_array_size,
                &lte_band_class_array,
                &nr_band_class_array_size,
                &nr_band_class_array,
                &custom_data_class,
                &device_id,
                &firmware_info,
                &hardware_info,
                &error)) {
            g_printerr ("error: couldn't parse response message: %s\n", error->message);
            shutdown (FALSE);
            return;
        }
        g_debug ("Successfully parsed response as MBIMEx 3.0 Device Caps");
    } else {
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
        g_debug ("Successfully parsed response as MBIMEx 1.0 Device Caps");
    }

    if (mbim_device_check_ms_mbimex_version (device, 3, 0))
        data_class_str = mbim_data_class_v3_build_string_from_mask (data_class_v3);
    else
        data_class_str = mbim_data_class_build_string_from_mask (data_class);

    device_type_str    = mbim_device_type_get_string (device_type);
    cellular_class_str = mbim_cellular_class_build_string_from_mask (cellular_class);
    voice_class_str    = mbim_voice_class_get_string (voice_class);
    sim_class_str      = mbim_sim_class_build_string_from_mask (sim_class);
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

    if (mbim_device_check_ms_mbimex_version (device, 3, 0)) {
        g_autofree gchar *data_subclass_str = NULL;
        guint             i;
        gboolean          n_printed;

        data_subclass_str = mbim_data_subclass_build_string_from_mask (data_subclass);
        g_print ("\t    Data subclass: '%s'\n", data_subclass_str);

        g_print ("\t WCDMA band class: '");
        for (n_printed = 0, i = 0; i < 31; i++) {
            if (wcdma_band_class & (1 << i)) {
                g_print ("%s%u", n_printed > 0 ? ", " : "", i + 1);
                n_printed++;
            }
        }
        g_print ("'\n");

        g_print ("\t   LTE band class: '");
        for (i = 0; i < lte_band_class_array_size; i++)
            g_print ("%s%" G_GUINT16_FORMAT, i > 0 ? ", " : "", lte_band_class_array[i]);
        g_print ("'\n");

        g_print ("\t    NR band class: '");
        for (i = 0; i < nr_band_class_array_size; i++)
            g_print ("%s%" G_GUINT16_FORMAT, i > 0 ? ", " : "", nr_band_class_array[i]);
        g_print ("'\n");
    }

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

typedef struct {
    MbimContextOperation       operation;
    MbimContextIpType          ip_type;
    MbimContextState           state;
    MbimContextRoamingControl  roaming_control;
    MbimContextMediaType       media_type;
    MbimContextSource          source;
    gchar                     *access_string;
    gchar                     *username;
    gchar                     *password;
    MbimCompression            compression;
    MbimAuthProtocol           auth_protocol;
    MbimContextType            context_type;
} ProvisionedContextProperties;

static void
provisioned_context_properties_clear (ProvisionedContextProperties *props)
{
    g_free (props->access_string);
    g_free (props->username);
    g_free (props->password);
}

G_DEFINE_AUTO_CLEANUP_CLEAR_FUNC(ProvisionedContextProperties, provisioned_context_properties_clear)

static gboolean
set_provisioned_contexts_foreach_cb (const gchar                   *key,
                                     const gchar                   *value,
                                     GError                       **error,
                                     ProvisionedContextProperties  *props)
{
    if (g_ascii_strcasecmp (key, "operation") == 0) {
        if (!mbimcli_read_context_operation_from_string (value, &props->operation)) {
            g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_INVALID_ARGS,
                         "unknown operation: '%s'", value);
            return FALSE;
        }
    } else if (g_ascii_strcasecmp (key, "context-type") == 0) {
        if (!mbimcli_read_context_type_from_string (value, &props->context_type)) {
            g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_INVALID_ARGS,
                         "unknown context-type: '%s'", value);
            return FALSE;
        }
    } else if (g_ascii_strcasecmp (key, "ip-type") == 0) {
        if (!mbimcli_read_context_ip_type_from_string (value, &props->ip_type)) {
            g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_INVALID_ARGS,
                         "unknown ip-type: '%s'", value);
            return FALSE;
        }
    } else if (g_ascii_strcasecmp (key, "state") == 0) {
        if (!mbimcli_read_context_state_from_string (value, &props->state)) {
            g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_INVALID_ARGS,
                         "unknown state: '%s'", value);
            return FALSE;
        }
    } else if (g_ascii_strcasecmp (key, "roaming-control") == 0) {
        if (!mbimcli_read_context_roaming_control_from_string (value, &props->roaming_control)) {
            g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_INVALID_ARGS,
                         "unknown roaming-control: '%s'", value);
            return FALSE;
        }
    } else if (g_ascii_strcasecmp (key, "media-type") == 0) {
        if (!mbimcli_read_context_media_type_from_string (value, &props->media_type)) {
            g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_INVALID_ARGS,
                         "unknown media-type: '%s'", value);
            return FALSE;
        }
    } else if (g_ascii_strcasecmp (key, "source") == 0) {
        if (!mbimcli_read_context_source_from_string (value, &props->source)) {
            g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_INVALID_ARGS,
                         "unknown source: '%s'", value);
            return FALSE;
        }
    } else if (g_ascii_strcasecmp (key, "auth") == 0) {
        if (!mbimcli_read_auth_protocol_from_string (value, &props->auth_protocol)) {
            g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_INVALID_ARGS,
                         "unknown auth: '%s'", value);
            return FALSE;
        }
    } else if (g_ascii_strcasecmp (key, "compression") == 0) {
        if (!mbimcli_read_compression_from_string (value, &props->compression)) {
            g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_INVALID_ARGS,
                         "unknown compression: '%s'", value);
            return FALSE;
        }
    } else if (g_ascii_strcasecmp (key, "username") == 0) {
        g_free (props->username);
        props->username = g_strdup (value);
    } else if (g_ascii_strcasecmp (key, "password") == 0) {
        g_free (props->password);
        props->password = g_strdup (value);
    } else if (g_ascii_strcasecmp (key, "access-string") == 0) {
        g_free (props->access_string);
        props->access_string = g_strdup (value);
    } else {
        g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_FAILED,
                     "unrecognized option '%s'", key);
        return FALSE;
    }

    return TRUE;
}

static void
provisioned_contexts_ready (MbimDevice   *device,
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
                 "\t        IP type: '%s'\n"
                 "\t          State: '%s'\n"
                 "\tRoaming control: '%s'\n"
                 "\t     Media type: '%s'\n"
                 "\t         Source: '%s'\n"
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

    shutdown (TRUE);
}

static void
query_base_stations_ready (MbimDevice   *device,
                           GAsyncResult *res)
{
    g_autoptr(MbimMessage)                         response = NULL;
    g_autoptr(GError)                              error = NULL;
    MbimDataClass                                  system_type;
    MbimDataClassV3                                system_type_v3;
    MbimDataSubclass                               system_subtype;
    g_autoptr(MbimCellInfoServingGsm)              gsm_serving_cell = NULL;
    g_autoptr(MbimCellInfoServingUmts)             umts_serving_cell = NULL;
    g_autoptr(MbimCellInfoServingTdscdma)          tdscdma_serving_cell = NULL;
    g_autoptr(MbimCellInfoServingLte)              lte_serving_cell = NULL;
    guint32                                        gsm_neighboring_cells_count;
    g_autoptr(MbimCellInfoNeighboringGsmArray)     gsm_neighboring_cells = NULL;
    guint32                                        umts_neighboring_cells_count;
    g_autoptr(MbimCellInfoNeighboringUmtsArray)    umts_neighboring_cells = NULL;
    guint32                                        tdscdma_neighboring_cells_count;
    g_autoptr(MbimCellInfoNeighboringTdscdmaArray) tdscdma_neighboring_cells = NULL;
    guint32                                        lte_neighboring_cells_count;
    g_autoptr(MbimCellInfoNeighboringLteArray)     lte_neighboring_cells = NULL;
    guint32                                        cdma_cells_count;
    g_autoptr(MbimCellInfoCdmaArray)               cdma_cells = NULL;
    guint32                                        nr_serving_cells_count;
    g_autoptr(MbimCellInfoServingNrArray)          nr_serving_cells = NULL;
    guint32                                        nr_neighboring_cells_count;
    g_autoptr(MbimCellInfoNeighboringNrArray)      nr_neighboring_cells = NULL;

    response = mbim_device_command_finish (device, res, &error);
    if (!response || !mbim_message_response_get_result (response, MBIM_MESSAGE_TYPE_COMMAND_DONE, &error)) {
        g_printerr ("error: operation failed: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    /* MBIMEx 3.0 support */
    if (mbim_device_check_ms_mbimex_version (device, 3, 0)) {
        if (!mbim_message_ms_basic_connect_extensions_v3_base_stations_info_response_parse (
                response,
                &system_type_v3,
                &system_subtype,
                &gsm_serving_cell,
                &umts_serving_cell,
                &tdscdma_serving_cell,
                &lte_serving_cell,
                &gsm_neighboring_cells_count,
                &gsm_neighboring_cells,
                &umts_neighboring_cells_count,
                &umts_neighboring_cells,
                &tdscdma_neighboring_cells_count,
                &tdscdma_neighboring_cells,
                &lte_neighboring_cells_count,
                &lte_neighboring_cells,
                &cdma_cells_count,
                &cdma_cells,
                &nr_serving_cells_count,
                &nr_serving_cells,
                &nr_neighboring_cells_count,
                &nr_neighboring_cells,
                &error)) {
            g_printerr ("error: couldn't parse response message: %s\n", error->message);
            shutdown (FALSE);
            return;
        }
        g_debug ("Successfully parsed response as MBIMEx 3.0 Base Stations Info");
    }
    /* MBIMEx 1.0 support */
    else {
        if (!mbim_message_ms_basic_connect_extensions_base_stations_info_response_parse (
                response,
                &system_type,
                &gsm_serving_cell,
                &umts_serving_cell,
                &tdscdma_serving_cell,
                &lte_serving_cell,
                &gsm_neighboring_cells_count,
                &gsm_neighboring_cells,
                &umts_neighboring_cells_count,
                &umts_neighboring_cells,
                &tdscdma_neighboring_cells_count,
                &tdscdma_neighboring_cells,
                &lte_neighboring_cells_count,
                &lte_neighboring_cells,
                &cdma_cells_count,
                &cdma_cells,
                &error)) {
            g_printerr ("error: couldn't parse response message: %s\n", error->message);
            shutdown (FALSE);
            return;
        }
        g_debug ("Successfully parsed response as MBIMEx 1.0 Base Stations Info");
    }

#define PRINT_VALIDATED_UINT(number, invalid, format, units) do {       \
        if (number == invalid)                                          \
            g_print ("%s: unknown\n", format);                          \
        else                                                            \
            g_print ("%s: %u%s\n", format, number, units ? units : ""); \
    } while (0)

#define PRINT_VALIDATED_UINT64(number, invalid, format, units) do {     \
        if (number == invalid)                                          \
            g_print ("%s: unknown\n", format);                          \
        else                                                            \
            g_print ("%s: %" G_GUINT64_FORMAT "%s\n", format, number, units ? units : ""); \
    } while (0)

#define PRINT_VALIDATED_INT(number, invalid, format, units) do {        \
        if (number == (gint32)invalid)                                  \
            g_print ("%s: unknown\n", format);                          \
        else                                                            \
            g_print ("%s: %d%s\n", format, number, units ? units : ""); \
    } while (0)

#define PRINT_VALIDATED_SCALED_UINT(number, invalid, scale, format, units) do {           \
        if (number == 0xFFFFFFFF)                                                         \
            g_print ("%s: unknown\n", format);                                            \
        else                                                                              \
            g_print ("%s: %d%s\n", format, ((gint32)number) + scale, units ? units : ""); \
    } while (0)

    if (mbim_device_check_ms_mbimex_version (device, 3, 0)) {
        g_autofree gchar *system_type_str = NULL;
        g_autofree gchar *system_subtype_str = NULL;

        system_type_str = mbim_data_class_v3_build_string_from_mask (system_type_v3);
        g_print ("System type: %s\n", system_type_str);
        system_subtype_str = mbim_data_subclass_build_string_from_mask (system_subtype);
        g_print ("System subtype: %s\n", VALIDATE_UNKNOWN (system_subtype_str));
    } else {
        g_autofree gchar *system_type_str = NULL;

        system_type_str = mbim_data_class_build_string_from_mask (system_type);
        g_print ("System type: %s\n", system_type_str);
    }

    if (gsm_serving_cell) {
        g_print ("GSM serving cell:\n"
                 "\t    Provider id: %s\n",
                 VALIDATE_UNKNOWN (gsm_serving_cell->provider_id));
        PRINT_VALIDATED_UINT        (gsm_serving_cell->location_area_code, 0xFFFFFFFF,       "\t            LAC", NULL);
        PRINT_VALIDATED_UINT        (gsm_serving_cell->cell_id,            0xFFFFFFFF,       "\t        Cell ID", NULL);
        PRINT_VALIDATED_UINT        (gsm_serving_cell->timing_advance,     0xFFFFFFFF,       "\t Timing advance", " bit periods");
        PRINT_VALIDATED_UINT        (gsm_serving_cell->arfcn,              0xFFFFFFFF,       "\t          ARFCN", NULL);
        PRINT_VALIDATED_UINT        (gsm_serving_cell->base_station_id,    0xFFFFFFFF,       "\tBase station ID", NULL);
        PRINT_VALIDATED_SCALED_UINT (gsm_serving_cell->rx_level,           0xFFFFFFFF, -110, "\t       Rx level", " dBm");
    } else
        g_print ("GSM serving cell: n/a\n");

    if (gsm_neighboring_cells_count && gsm_neighboring_cells) {
        guint i;

        g_print ("Neighboring GSM cells: %d\n", gsm_neighboring_cells_count);
        for (i = 0; i < gsm_neighboring_cells_count; i++) {
            g_print ("\tNeighboring cell [%u]:\n"
                     "\t\t    Provider id: %s\n",
                     i + 1,
                     VALIDATE_UNKNOWN (gsm_neighboring_cells[i]->provider_id));
            PRINT_VALIDATED_UINT        (gsm_neighboring_cells[i]->location_area_code, 0xFFFFFFFF,       "\t\t            LAC", NULL);
            PRINT_VALIDATED_UINT        (gsm_neighboring_cells[i]->cell_id,            0xFFFFFFFF,       "\t\t        Cell ID", NULL);
            PRINT_VALIDATED_UINT        (gsm_neighboring_cells[i]->arfcn,              0xFFFFFFFF,       "\t\t          ARFCN", NULL);
            PRINT_VALIDATED_UINT        (gsm_neighboring_cells[i]->base_station_id,    0xFFFFFFFF,       "\t\tBase station ID", NULL);
            PRINT_VALIDATED_SCALED_UINT (gsm_neighboring_cells[i]->rx_level,           0xFFFFFFFF, -110, "\t\t       Rx level", " dBm");
        }
    } else
        g_print ("Neighboring GSM cells: n/a\n");

    if (umts_serving_cell) {
        g_print ("UMTS Serving cell:\n"
                 "\t            Provider id: %s\n",
                 VALIDATE_UNKNOWN (umts_serving_cell->provider_id));
        PRINT_VALIDATED_UINT (umts_serving_cell->location_area_code,      0xFFFFFFFF, "\t                    LAC", NULL);
        PRINT_VALIDATED_UINT (umts_serving_cell->cell_id,                 0xFFFFFFFF, "\t                Cell id", NULL);
        PRINT_VALIDATED_UINT (umts_serving_cell->frequency_info_ul,       0xFFFFFFFF, "\t  Frequency info uplink", NULL);
        PRINT_VALIDATED_UINT (umts_serving_cell->frequency_info_dl,       0xFFFFFFFF, "\tFrequency info downlink", NULL);
        PRINT_VALIDATED_UINT (umts_serving_cell->frequency_info_nt,       0xFFFFFFFF, "\t     Frequency info TDD", NULL);
        PRINT_VALIDATED_UINT (umts_serving_cell->uarfcn,                  0xFFFFFFFF, "\t                 UARFCN", NULL);
        PRINT_VALIDATED_UINT (umts_serving_cell->primary_scrambling_code, 0xFFFFFFFF, "\tPrimary Scrambling Code", NULL);
        /* note: docs say unknown value in this case is 0
         * https://github.com/MicrosoftDocs/windows-driver-docs/issues/2932 */
        PRINT_VALIDATED_INT  (umts_serving_cell->rscp,                    0xFFFFFFFF, "\t                   RSCP", " dBm");
        PRINT_VALIDATED_INT  (umts_serving_cell->ecno,                    1,          "\t                   ECNO", " dBm");
        PRINT_VALIDATED_UINT (umts_serving_cell->path_loss,               0xFFFFFFFF, "\t              Path loss", NULL);
    } else
        g_print ("UMTS serving cell: n/a\n");

    if (umts_neighboring_cells_count && umts_neighboring_cells) {
        guint i;

        g_print ("Neighboring UMTS cells: %d\n", umts_neighboring_cells_count);
        for (i = 0; i < umts_neighboring_cells_count; i++) {
            g_print ("\tNeighboring cell [%u]:\n"
                     "\t\t            Provider id: %s\n",
                     i + 1,
                     VALIDATE_UNKNOWN (umts_neighboring_cells[i]->provider_id));
            PRINT_VALIDATED_UINT (umts_neighboring_cells[i]->location_area_code,      0xFFFFFFFF, "\t\t                    LAC", NULL);
            PRINT_VALIDATED_UINT (umts_neighboring_cells[i]->cell_id,                 0xFFFFFFFF, "\t\t                Cell id", NULL);
            PRINT_VALIDATED_UINT (umts_neighboring_cells[i]->uarfcn,                  0xFFFFFFFF, "\t\t                 UARFCN", NULL);
            PRINT_VALIDATED_UINT (umts_neighboring_cells[i]->primary_scrambling_code, 0xFFFFFFFF, "\t\tPrimary Scrambling Code", NULL);
            PRINT_VALIDATED_INT  (umts_neighboring_cells[i]->rscp,                    0xFFFFFFFF, "\t\t                   RSCP", " dBm");
            PRINT_VALIDATED_INT  (umts_neighboring_cells[i]->ecno,                    1,          "\t\t                   ECNO", " dBm");
            PRINT_VALIDATED_UINT (umts_neighboring_cells[i]->path_loss,               0xFFFFFFFF, "\t\t              Path loss", NULL);
        }
    } else
        g_print ("Neighboring UMTS cells: n/a\n");

    if (tdscdma_serving_cell) {
        g_print ("TDSCDMA Serving cell:\n"
                 "\t      Provider id: %s\n",
                 VALIDATE_UNKNOWN (tdscdma_serving_cell->provider_id));
        PRINT_VALIDATED_UINT (tdscdma_serving_cell->location_area_code, 0xFFFFFFFF, "\t              LAC", NULL);
        PRINT_VALIDATED_UINT (tdscdma_serving_cell->cell_id,            0xFFFFFFFF, "\t          Cell id", NULL);
        PRINT_VALIDATED_UINT (tdscdma_serving_cell->uarfcn,             0xFFFFFFFF, "\t           UARFCN", NULL);
        PRINT_VALIDATED_UINT (tdscdma_serving_cell->cell_parameter_id,  0xFFFFFFFF, "\tCell parameter id", NULL);
        PRINT_VALIDATED_UINT (tdscdma_serving_cell->timing_advance,     0xFFFFFFFF, "\t   Timing advance", NULL);
        PRINT_VALIDATED_INT  (tdscdma_serving_cell->rscp,               0xFFFFFFFF, "\t             RSCP", " dBm");
        PRINT_VALIDATED_UINT (tdscdma_serving_cell->path_loss,          0xFFFFFFFF, "\t        Path loss", NULL);
    } else
        g_print ("TDSCDMA serving cell: n/a\n");

    if (tdscdma_neighboring_cells_count && tdscdma_neighboring_cells) {
        guint i;

        g_print ("Neighboring TDSCDMA cells: %d\n", tdscdma_neighboring_cells_count);
        for (i = 0; i < tdscdma_neighboring_cells_count; i++) {
            g_print ("\tNeighboring cell [%u]:\n"
                     "\t\t      Provider id: %s\n",
                     i + 1,
                     VALIDATE_UNKNOWN (tdscdma_neighboring_cells[i]->provider_id));
            PRINT_VALIDATED_UINT (tdscdma_neighboring_cells[i]->location_area_code, 0xFFFFFFFF, "\t\t              LAC", NULL);
            PRINT_VALIDATED_UINT (tdscdma_neighboring_cells[i]->cell_id,            0xFFFFFFFF, "\t\t          Cell id", NULL);
            PRINT_VALIDATED_UINT (tdscdma_neighboring_cells[i]->uarfcn,             0xFFFFFFFF, "\t\t           UARFCN", NULL);
            PRINT_VALIDATED_UINT (tdscdma_neighboring_cells[i]->cell_parameter_id,  0xFFFFFFFF, "\t\tCell parameter id", NULL);
            PRINT_VALIDATED_UINT (tdscdma_neighboring_cells[i]->timing_advance,     0xFFFFFFFF, "\t\t   Timing advance", NULL);
            PRINT_VALIDATED_INT  (tdscdma_neighboring_cells[i]->rscp,               0xFFFFFFFF, "\t\t             RSCP", " dBm");
            PRINT_VALIDATED_UINT (tdscdma_neighboring_cells[i]->path_loss,          0xFFFFFFFF, "\t\t        Path Loss", NULL);
        }
    } else
        g_print ("Neighboring TDSCDMA cells: n/a\n");

    if (lte_serving_cell) {
        g_print ("LTE Serving cell:\n"
                 "\t      Provider id: %s\n",
                 VALIDATE_UNKNOWN (lte_serving_cell->provider_id));
        PRINT_VALIDATED_UINT (lte_serving_cell->cell_id,          0xFFFFFFFF, "\t          Cell id", NULL);
        PRINT_VALIDATED_UINT (lte_serving_cell->earfcn,           0xFFFFFFFF, "\t           EARFCN", NULL);
        PRINT_VALIDATED_UINT (lte_serving_cell->physical_cell_id, 0xFFFFFFFF, "\t Physical cell id", NULL);
        PRINT_VALIDATED_UINT (lte_serving_cell->tac,              0xFFFFFFFF, "\t              TAC", NULL);
        PRINT_VALIDATED_INT  (lte_serving_cell->rsrp,             0xFFFFFFFF, "\t             RSRP", " dBm");
        PRINT_VALIDATED_INT  (lte_serving_cell->rsrq,             0xFFFFFFFF, "\t             RSRQ", " dBm");
        PRINT_VALIDATED_UINT (lte_serving_cell->timing_advance,   0xFFFFFFFF, "\t   Timing advance", NULL);
    } else
        g_print ("LTE serving cell: n/a\n");

    if (lte_neighboring_cells_count && lte_neighboring_cells) {
        guint i;

        g_print ("Neighboring LTE cells: %d\n", lte_neighboring_cells_count);
        for (i = 0; i < lte_neighboring_cells_count; i++) {
            g_print ("\tNeighboring cell [%u]:\n"
                     "\t\t      Provider id: %s\n",
                     i + 1,
                     VALIDATE_UNKNOWN (lte_neighboring_cells[i]->provider_id));
            PRINT_VALIDATED_UINT (lte_neighboring_cells[i]->cell_id,          0xFFFFFFFF, "\t\t          Cell id", NULL);
            PRINT_VALIDATED_UINT (lte_neighboring_cells[i]->earfcn,           0xFFFFFFFF, "\t\t           EARFCN", NULL);
            PRINT_VALIDATED_UINT (lte_neighboring_cells[i]->physical_cell_id, 0xFFFFFFFF, "\t\t Physical cell id", NULL);
            PRINT_VALIDATED_UINT (lte_neighboring_cells[i]->tac,              0xFFFFFFFF, "\t\t              TAC", NULL);
            PRINT_VALIDATED_INT  (lte_neighboring_cells[i]->rsrp,             0xFFFFFFFF, "\t\t             RSRP", " dBm");
            PRINT_VALIDATED_INT  (lte_neighboring_cells[i]->rsrq,             0xFFFFFFFF, "\t\t             RSRQ", " dBm");
        }
    } else
        g_print ("Neighboring LTE cells: n/a\n");

    if (cdma_cells_count && cdma_cells) {
        guint i;

        g_print ("CDMA cells: %d\n", cdma_cells_count);
        for (i = 0; i < cdma_cells_count; i++)
            g_print ("Cell [%u]:\n"
                     "\t        Serving: %s\n",
                     i + 1,
                     cdma_cells[i]->serving_cell_flag ? "yes" : "no");
        PRINT_VALIDATED_UINT (cdma_cells[i]->nid,             0xFFFFFFFF, "\t            NID", NULL);
        PRINT_VALIDATED_UINT (cdma_cells[i]->sid,             0xFFFFFFFF, "\t            SID", NULL);
        PRINT_VALIDATED_UINT (cdma_cells[i]->base_station_id, 0xFFFFFFFF, "\tBase station id", NULL);
        /* TODO: The Base Station Latitude (0-4194303). This is encoded in units of 0.25 seconds, expressed
         * in two’s complement representation within the low 22 bits of the DWORD. As a signed value,
         * North latitudes are positive. Use 0xFFFFFFFF when this information is not available. */
        PRINT_VALIDATED_UINT (cdma_cells[i]->base_latitude,   0xFFFFFFFF, "\t  Base latitude", NULL);
        /* TODO: The Base Station Longitude (0-8388607). This is encoded in units of 0.25 seconds, expressed
         * in two’s complement representation within the low 23 bits of the DWORD. As a signed value, East
         * longitudes are positive. Use 0xFFFFFFFF when this information is not available. */
        PRINT_VALIDATED_UINT (cdma_cells[i]->base_longitude,  0xFFFFFFFF, "\t Base longitude", NULL);
        PRINT_VALIDATED_UINT (cdma_cells[i]->ref_pn,          0xFFFFFFFF, "\t          RefPN", NULL);
        PRINT_VALIDATED_UINT (cdma_cells[i]->gps_seconds,     0xFFFFFFFF, "\t    GPS seconds", " seconds");
        PRINT_VALIDATED_UINT (cdma_cells[i]->pilot_strength,  0xFFFFFFFF, "\t Pilot strength", NULL);
    } else
        g_print ("CDMA cells: n/a\n");

    if (nr_serving_cells_count && nr_serving_cells) {
        guint i;

        g_print ("Serving NR cells: %d\n", nr_serving_cells_count);
        for (i = 0; i < nr_serving_cells_count; i++) {
            g_print ("\tServing cell [%u]:\n"
                     "\t\t     Provider id: %s\n",
                     i + 1,
                     VALIDATE_UNKNOWN (nr_serving_cells[i]->provider_id));
            PRINT_VALIDATED_UINT64      (nr_serving_cells[i]->nci,              0xFFFFFFFFFFFFFFFF, "\t\t             NCI", NULL);
            PRINT_VALIDATED_UINT        (nr_serving_cells[i]->physical_cell_id, 0xFFFFFFFF,         "\t\tPhysical cell id", NULL);
            PRINT_VALIDATED_UINT        (nr_serving_cells[i]->nrarfcn,          0xFFFFFFFF,         "\t\t         NRARFCN", NULL);
            PRINT_VALIDATED_UINT        (nr_serving_cells[i]->tac,              0xFFFFFFFF,         "\t\t             TAC", NULL);
            PRINT_VALIDATED_SCALED_UINT (nr_serving_cells[i]->rsrp,             0xFFFFFFFF, -156,   "\t\t            RSRP", " dBm");
            PRINT_VALIDATED_SCALED_UINT (nr_serving_cells[i]->rsrq,             0xFFFFFFFF, -43,    "\t\t            RSRQ", " dB");
            PRINT_VALIDATED_SCALED_UINT (nr_serving_cells[i]->sinr,             0xFFFFFFFF, -23,    "\t\t            SINR", " dB");
            PRINT_VALIDATED_UINT64      (nr_serving_cells[i]->timing_advance,   0xFFFFFFFFFFFFFFFF, "\t\t  Timing advance", " us");
        }
    } else
        g_print ("Serving NR cells: n/a\n");

    if (nr_neighboring_cells_count && nr_neighboring_cells) {
        guint i;

        g_print ("Neighboring NR cells: %d\n", nr_neighboring_cells_count);
        for (i = 0; i < nr_neighboring_cells_count; i++) {
            g_autofree gchar *system_subtype_str = NULL;

            system_subtype_str = mbim_data_subclass_build_string_from_mask (nr_neighboring_cells[i]->system_sub_type);
            g_print ("\tNeighboring cell [%u]:\n"
                     "\t\t  System subtype: %s\n"
                     "\t\t     Provider id: %s\n"
                     "\t\t         Cell id: %s\n",
                     i + 1,
                     VALIDATE_UNKNOWN (system_subtype_str),
                     VALIDATE_UNKNOWN (nr_neighboring_cells[i]->provider_id),
                     VALIDATE_UNKNOWN (nr_neighboring_cells[i]->cell_id));
            PRINT_VALIDATED_UINT        (nr_neighboring_cells[i]->physical_cell_id, 0xFFFFFFFF,       "\t\tPhysical cell id", NULL);
            PRINT_VALIDATED_UINT        (nr_neighboring_cells[i]->tac,              0xFFFFFFFF,       "\t\t             TAC", NULL);
            PRINT_VALIDATED_SCALED_UINT (nr_neighboring_cells[i]->rsrp,             0xFFFFFFFF, -156, "\t\t            RSRP", " dBm");
            PRINT_VALIDATED_SCALED_UINT (nr_neighboring_cells[i]->rsrq,             0xFFFFFFFF, -43,  "\t\t            RSRQ", " dB");
            PRINT_VALIDATED_SCALED_UINT (nr_neighboring_cells[i]->sinr,             0xFFFFFFFF, -23,  "\t\t            SINR", " dB");
        }
    } else
        g_print ("Neighboring NR cells: n/a\n");

#undef PRINT_VALIDATED_SCALED_UINT
#undef PRINT_VALIDATED_UINT64
#undef PRINT_VALIDATED_UINT
#undef PRINT_VALIDATED_INT

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
    if (!mbim_message_ms_basic_connect_extensions_v2_version_response_parse (
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

typedef struct {
    MbimMicoMode                 mico_mode;
    gboolean                     mico_mode_set;
    MbimDrxCycle                 drx_cycle;
    gboolean                     drx_cycle_set;
    MbimLadnInfo                 ladn_info;
    gboolean                     ladn_info_set;
    MbimDefaultPduActivationHint pdu_hint;
    gboolean                     pdu_hint_set;
    gboolean                     re_register_if_needed;
    gboolean                     re_register_if_needed_set;
} RegistrationParameters;

static gboolean
set_registration_parameters_foreach_cb (const gchar             *key,
                                        const gchar             *value,
                                        GError                 **error,
                                        RegistrationParameters  *params)
{
    if (g_ascii_strcasecmp (key, "mico-mode") == 0) {
        if (!mbimcli_read_mico_mode_from_string (value, &params->mico_mode)) {
            g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_INVALID_ARGS,
                         "unknown mico-mode: '%s'", value);
            return FALSE;
        }
        params->mico_mode_set = TRUE;
    } else if (g_ascii_strcasecmp (key, "drx-cycle") == 0) {
        if (!mbimcli_read_drx_cycle_from_string (value, &params->drx_cycle)) {
            g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_INVALID_ARGS,
                         "unknown drx-cycle: '%s'", value);
            return FALSE;
        }
        params->drx_cycle_set = TRUE;
    } else if (g_ascii_strcasecmp (key, "ladn-info") == 0) {
        if (!mbimcli_read_ladn_info_from_string (value, &params->ladn_info)) {
            g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_INVALID_ARGS,
                         "unknown ladn-info: '%s'", value);
            return FALSE;
        }
        params->ladn_info_set = TRUE;
    } else if (g_ascii_strcasecmp (key, "default-pdu-activation-hint") == 0) {
        if (!mbimcli_read_default_pdu_activation_hint_from_string (value, &params->pdu_hint)) {
            g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_INVALID_ARGS,
                         "unknown default-pdu-activation-hint: '%s'", value);
            return FALSE;
        }
        params->pdu_hint_set = TRUE;
    } else if (g_ascii_strcasecmp (key, "re-register-if-needed") == 0) {
        if (!mbimcli_read_boolean_from_string (value, &params->re_register_if_needed)) {
            g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_INVALID_ARGS,
                         "unknown re-register-if-needed: '%s'", value);
            return FALSE;
        }
        params->re_register_if_needed_set = TRUE;
    } else {
        g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_FAILED,
                     "unrecognized option '%s'", key);
        return FALSE;
    }

    return TRUE;
}

static void
registration_parameters_ready (MbimDevice   *device,
                               GAsyncResult *res)
{
    g_autoptr(MbimMessage)       response = NULL;
    g_autoptr(GError)            error = NULL;
    MbimMicoMode                 mico_mode;
    MbimDrxCycle                 drx_cycle;
    MbimLadnInfo                 ladn_info;
    MbimDefaultPduActivationHint pdu_hint;
    gboolean                     re_register_if_nedeed;

    response = mbim_device_command_finish (device, res, &error);
    if (!response || !mbim_message_response_get_result (response, MBIM_MESSAGE_TYPE_COMMAND_DONE, &error)) {
        g_printerr ("error: operation failed: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    g_print ("[%s] Successfully received registration parameters information\n",
             mbim_device_get_path_display (device));
    if (!mbim_message_ms_basic_connect_extensions_v3_registration_parameters_response_parse (
            response,
            &mico_mode,
            &drx_cycle,
            &ladn_info,
            &pdu_hint,
            &re_register_if_nedeed,
            NULL, /* ignore unnamed IEs for now */
            &error)) {
        g_printerr ("error: couldn't parse response message: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    g_print ("\t             MICO mode: %s\n", mbim_mico_mode_get_string (mico_mode));
    g_print ("\t             DRX cycle: %s\n", mbim_drx_cycle_get_string (drx_cycle));
    g_print ("\t      LADN information: %s\n", mbim_ladn_info_get_string (ladn_info));
    g_print ("\tDefault PDU activation: %s\n", mbim_default_pdu_activation_hint_get_string (pdu_hint));
    g_print ("\t Re-register if needed: %s\n", re_register_if_nedeed ? "yes" : "no");

    shutdown (TRUE);
}

static void
query_modem_configuration_ready (MbimDevice   *device,
                                 GAsyncResult *res)
{
    g_autoptr(MbimMessage)         response = NULL;
    g_autoptr(GError)              error = NULL;
    MbimModemConfigurationStatus   configuration_status;
    g_autofree gchar              *configuration_name = NULL;

    response = mbim_device_command_finish (device, res, &error);
    if (!response || !mbim_message_response_get_result (response, MBIM_MESSAGE_TYPE_COMMAND_DONE, &error)) {
        g_printerr ("error: operation failed: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    if (!mbim_message_ms_basic_connect_extensions_v3_modem_configuration_response_parse (
            response,
            &configuration_status,
            &configuration_name,
            NULL, /* ignore unnamed IEs for now */
            &error)) {
        g_printerr ("error: couldn't parse response message: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    g_print ("[%s] Modem configuration retrieved: \n",
             mbim_device_get_path_display (device));
    g_print ("\tStatus: '%s'\n"
             "\t  Name: '%s'\n",
             VALIDATE_UNKNOWN (mbim_modem_configuration_status_get_string (configuration_status)),
             VALIDATE_UNKNOWN (configuration_name));

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

    if (set_provisioned_contexts_str) {
        g_auto(ProvisionedContextProperties) props = {
            .access_string   = NULL,
            .operation       = MBIM_CONTEXT_OPERATION_DELETE,
            .auth_protocol   = MBIM_AUTH_PROTOCOL_NONE,
            .username        = NULL,
            .password        = NULL,
            .ip_type         = MBIM_CONTEXT_IP_TYPE_DEFAULT,
            .state           = MBIM_CONTEXT_STATE_DISABLED,
            .roaming_control = MBIM_CONTEXT_ROAMING_CONTROL_HOME_ONLY,
            .media_type      = MBIM_CONTEXT_MEDIA_TYPE_CELLULAR_ONLY,
            .source          = MBIM_CONTEXT_SOURCE_ADMIN,
            .compression     = MBIM_COMPRESSION_NONE,
            .context_type    = MBIM_CONTEXT_TYPE_INVALID
        };

        if (!mbimcli_parse_key_value_string (set_provisioned_contexts_str,
                                             &error,
                                             (MbimParseKeyValueForeachFn)set_provisioned_contexts_foreach_cb,
                                             &props)) {
            g_printerr ("error: couldn't parse input string: %s\n", error->message);
            shutdown (FALSE);
            return;
        }

        request = mbim_message_ms_basic_connect_extensions_provisioned_contexts_set_new (
                      props.operation,
                      mbim_uuid_from_context_type (props.context_type),
                      props.ip_type,
                      props.state,
                      props.roaming_control,
                      props.media_type,
                      props.source,
                      props.access_string,
                      props.username,
                      props.password,
                      props.compression,
                      props.auth_protocol,
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
                             (GAsyncReadyCallback)provisioned_contexts_ready,
                             NULL);
        return;
    }

    /* Request to query Provisioned contexts? */
    if (query_provisioned_contexts_flag) {
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
                             (GAsyncReadyCallback)provisioned_contexts_ready,
                             NULL);
        return;
    }

    if (query_base_stations_flag) {
        g_debug ("Asynchronously querying base stations...");
        /* default capacity is 15, so use that value when querying */
        request = mbim_message_ms_basic_connect_extensions_base_stations_info_query_new (15, 15, 15, 15, 15, NULL);
        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)query_base_stations_ready,
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
        request = mbim_message_ms_basic_connect_extensions_v2_version_query_new (bcd_mbim_version, bcd_mbim_extended_version, NULL);
        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)query_version_ready,
                             NULL);
        return;
    }

    if (query_registration_parameters_flag) {
        g_debug (" Asynchronously querying registration parameters...");
        request = mbim_message_ms_basic_connect_extensions_v3_registration_parameters_query_new (NULL);
        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)registration_parameters_ready,
                             NULL);
        return;
    }

    if (set_registration_parameters_str) {
        RegistrationParameters params = { 0 };

        if (!mbimcli_parse_key_value_string (set_registration_parameters_str,
                                             &error,
                                             (MbimParseKeyValueForeachFn)set_registration_parameters_foreach_cb,
                                             &params)) {
            g_printerr ("error: couldn't parse input string: %s\n", error->message);
            shutdown (FALSE);
            return;
        }

        if (!params.mico_mode_set ||
            !params.drx_cycle_set ||
            !params.ladn_info_set ||
            !params.pdu_hint_set  ||
            !params.re_register_if_needed_set) {
            g_printerr ("error: missing required keys\n");
            if (!params.mico_mode_set)
                g_printerr ("error: key 'mico-mode' is missing\n");
            if (!params.drx_cycle_set)
                g_printerr ("error: key 'drx-cycle' is missing\n");
            if (!params.ladn_info_set)
                g_printerr ("error: key 'ladn-info' is missing\n");
            if (!params.pdu_hint_set)
                g_printerr ("error: key 'default-pdu-activation-hint' is missing\n");
            if (!params.re_register_if_needed_set)
                g_printerr ("error: key 're-register-is-needed' is missing\n");
            shutdown (FALSE);
            return;
        }

        g_debug ("Asynchronously set registration parameters\n");
        request = (mbim_message_ms_basic_connect_extensions_v3_registration_parameters_set_new (
                       params.mico_mode,
                       params.drx_cycle,
                       params.ladn_info,
                       params.pdu_hint,
                       params.re_register_if_needed,
                       NULL, /* ignore unnamed IEs for now */
                       NULL));
        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)registration_parameters_ready,
                             NULL);
        return;
    }

    /* Request to query modem configuration? */
    if (query_modem_configuration_flag) {
        g_debug ("Asynchronously query modem configuration\n");
        request = mbim_message_ms_basic_connect_extensions_v3_modem_configuration_query_new (NULL);
        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)query_modem_configuration_ready,
                             NULL);
        return;
    }

    g_warn_if_reached ();
}
