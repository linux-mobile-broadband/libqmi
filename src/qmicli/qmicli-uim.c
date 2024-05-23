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
 * Copyright (C) 2012-2017 Aleksander Morgado <aleksander@aleksander.es>
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc.
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <string.h>

#include <glib.h>
#include <gio/gio.h>

#include <libqmi-glib.h>
#include <qmi-common.h>

#include "qmicli.h"
#include "qmicli-helpers.h"

#if defined HAVE_QMI_SERVICE_UIM

#undef VALIDATE_MASK_NONE
#define VALIDATE_MASK_NONE(str) (str ? str : "none")

/* Context */
typedef struct {
    QmiDevice *device;
    QmiClientUim *client;
    GCancellable *cancellable;

    /* For Slot Status indication */
    guint slot_status_indication_id;
    guint refresh_indication_id;
} Context;
static Context *ctx;

/* Options */
static gchar *read_transparent_str;
static gchar *read_record_str;
static gchar *set_pin_protection_str;
static gchar *verify_pin_str;
static gchar *unblock_pin_str;
static gchar *change_pin_str;
static gchar *get_file_attributes_str;
static gchar *sim_power_on_str;
static gchar *sim_power_off_str;
static gchar *change_provisioning_session_str;
static gchar *switch_slot_str;
static gchar *depersonalization_str;
static gchar *remote_unlock_str;
static gchar *open_logical_channel_str;
static gchar *close_logical_channel_str;
static gchar *send_apdu_str;
static gchar **monitor_refresh_file_array;
static gboolean get_card_status_flag;
static gboolean get_supported_messages_flag;
static gboolean get_slot_status_flag;
static gboolean monitor_slot_status_flag;
static gboolean reset_flag;
static gboolean monitor_refresh_all_flag;
static gboolean noop_flag;
static gboolean get_configuration_flag;

#undef VALIDATE_UNKNOWN
#define VALIDATE_UNKNOWN(str) (str ? str : "unknown")

static GOptionEntry entries[] = {
#if defined HAVE_QMI_MESSAGE_UIM_SET_PIN_PROTECTION
    { "uim-set-pin-protection", 0, 0, G_OPTION_ARG_STRING, &set_pin_protection_str,
      "Set PIN protection (allowed keys: session-type ((primary|secondary|tertiary|quarternary|quinary)-gw-provisioning|card-slot-[1-5]))",
      "[(PIN1|PIN2|UPIN),(disable|enable),(current PIN)[,\"key=value,...\"]]"
    },
#endif
#if defined HAVE_QMI_MESSAGE_UIM_VERIFY_PIN
    { "uim-verify-pin", 0, 0, G_OPTION_ARG_STRING, &verify_pin_str,
      "Verify PIN (allowed keys: session-type ((primary|secondary|tertiary|quarternary|quinary)-gw-provisioning|card-slot-[1-5]))",
      "[(PIN1|PIN2|UPIN),(current PIN)[,\"key=value,...\"]]",
    },
#endif
#if defined HAVE_QMI_MESSAGE_UIM_UNBLOCK_PIN
    { "uim-unblock-pin", 0, 0, G_OPTION_ARG_STRING, &unblock_pin_str,
      "Unblock PIN (allowed keys: session-type ((primary|secondary|tertiary|quarternary|quinary)-gw-provisioning|card-slot-[1-5]))",
      "[(PIN1|PIN2|UPIN),(PUK),(new PIN)[,\"key=value,...\"]]",
    },
#endif
#if defined HAVE_QMI_MESSAGE_UIM_CHANGE_PIN
    { "uim-change-pin", 0, 0, G_OPTION_ARG_STRING, &change_pin_str,
      "Change PIN (allowed keys: session-type ((primary|secondary|tertiary|quarternary|quinary)-gw-provisioning|card-slot-[1-5]))",
      "[(PIN1|PIN2|UPIN),(old PIN),(new PIN)[,\"key=value,...\"]]",
    },
#endif
#if defined HAVE_QMI_MESSAGE_UIM_READ_TRANSPARENT
    { "uim-read-transparent", 0, 0, G_OPTION_ARG_STRING, &read_transparent_str,
      "Read a transparent file given the file path",
      "[0xNNNN,0xNNNN,...]"
    },
#endif
#if defined HAVE_QMI_MESSAGE_UIM_GET_FILE_ATTRIBUTES
    { "uim-get-file-attributes", 0, 0, G_OPTION_ARG_STRING, &get_file_attributes_str,
      "Get the attributes of a given file",
      "[0xNNNN,0xNNNN,...]"
    },
#endif
#if defined HAVE_QMI_MESSAGE_UIM_READ_RECORD
    { "uim-read-record", 0, 0, G_OPTION_ARG_STRING, &read_record_str,
      "Read a record from given file (allowed keys: record-number, record-length, file ([0xNNNN-0xNNNN,...])",
      "[\"key=value,...\"]"
    },
#endif
#if defined HAVE_QMI_MESSAGE_UIM_GET_CARD_STATUS
    { "uim-get-card-status", 0, 0, G_OPTION_ARG_NONE, &get_card_status_flag,
      "Get card status",
      NULL
    },
#endif
#if defined HAVE_QMI_MESSAGE_UIM_GET_SUPPORTED_MESSAGES
    { "uim-get-supported-messages", 0, 0, G_OPTION_ARG_NONE, &get_supported_messages_flag,
      "Get supported messages",
      NULL
    },
#endif
#if defined HAVE_QMI_MESSAGE_UIM_POWER_ON_SIM
    { "uim-sim-power-on", 0, 0, G_OPTION_ARG_STRING, &sim_power_on_str,
      "Power on SIM card",
      "[(slot number)]"
    },
#endif
#if defined HAVE_QMI_MESSAGE_UIM_POWER_OFF_SIM
    { "uim-sim-power-off", 0, 0, G_OPTION_ARG_STRING, &sim_power_off_str,
      "Power off SIM card",
      "[(slot number)]"
    },
#endif
#if defined HAVE_QMI_MESSAGE_UIM_CHANGE_PROVISIONING_SESSION
    { "uim-change-provisioning-session", 0, 0, G_OPTION_ARG_STRING, &change_provisioning_session_str,
      "Change provisioning session (allowed keys: session-type ((primary|secondary|tertiary|quarternary|quinary)-gw-provisioning), activate (yes|no), slot, aid)",
      "[\"key=value,...\"]"
    },
#endif
#if defined HAVE_QMI_MESSAGE_UIM_GET_SLOT_STATUS
    { "uim-get-slot-status", 0, 0, G_OPTION_ARG_NONE, &get_slot_status_flag,
      "Get slot status",
      NULL
    },
#endif
#if defined HAVE_QMI_MESSAGE_UIM_SWITCH_SLOT && defined HAVE_QMI_MESSAGE_UIM_GET_SLOT_STATUS
    { "uim-switch-slot", 0, 0, G_OPTION_ARG_STRING, &switch_slot_str,
      "Switch active physical slot",
      "[(slot number)]"
    },
#endif
#if defined HAVE_QMI_INDICATION_UIM_SLOT_STATUS
    { "uim-monitor-slot-status", 0, 0, G_OPTION_ARG_NONE, &monitor_slot_status_flag,
      "Watch for slot status indications",
      NULL
    },
#endif
#if defined HAVE_QMI_MESSAGE_UIM_RESET
    { "uim-reset", 0, 0, G_OPTION_ARG_NONE, &reset_flag,
      "Reset the service state",
      NULL
    },
#endif
#if defined HAVE_QMI_MESSAGE_UIM_REFRESH_REGISTER
    { "uim-monitor-refresh-file", 0, 0, G_OPTION_ARG_STRING_ARRAY, &monitor_refresh_file_array,
      "Watch for REFRESH events for given file paths",
      "[0xNNNN,0xNNNN,...]"
    },
#endif
#if defined HAVE_QMI_MESSAGE_UIM_REFRESH_REGISTER_ALL
    { "uim-monitor-refresh-all", 0, 0, G_OPTION_ARG_NONE, &monitor_refresh_all_flag,
      "Watch for REFRESH events for any file",
      NULL
    },
#endif
#if defined HAVE_QMI_MESSAGE_UIM_GET_CONFIGURATION
    { "uim-get-configuration", 0, 0, G_OPTION_ARG_NONE, &get_configuration_flag,
      "Get personalization status of the modem",
      NULL
    },
#endif
#if defined HAVE_QMI_MESSAGE_UIM_DEPERSONALIZATION
    { "uim-depersonalization", 0, 0, G_OPTION_ARG_STRING, &depersonalization_str,
      "Deactivates or unblocks personalization feature",
      "[(feature),(operation),(control key)[,(slot number)]]"
    },
#endif
#if defined HAVE_QMI_MESSAGE_UIM_REMOTE_UNLOCK
    { "uim-remote-unlock", 0, 0, G_OPTION_ARG_STRING, &remote_unlock_str,
      "Updates the SimLock configuration data",
      "[XX:XX:...]"
    },
#endif
#if defined HAVE_QMI_MESSAGE_UIM_OPEN_LOGICAL_CHANNEL
    { "uim-open-logical-channel", 0, 0, G_OPTION_ARG_STRING, &open_logical_channel_str,
      "Open logical channel",
      "[(slot number),(aid)]"
    },
#endif
#if defined HAVE_QMI_MESSAGE_UIM_LOGICAL_CHANNEL
    { "uim-close-logical-channel", 0, 0, G_OPTION_ARG_STRING, &close_logical_channel_str,
      "Close logical channel",
      "[(slot number),(channel ID)]"
    },
#endif
#if defined HAVE_QMI_MESSAGE_UIM_SEND_APDU
    { "uim-send-apdu", 0, 0, G_OPTION_ARG_STRING, &send_apdu_str,
      "Send APDU",
      "[(slot number),(channel ID),(apdu)]"
    },
#endif
    { "uim-noop", 0, 0, G_OPTION_ARG_NONE, &noop_flag,
      "Just allocate or release a UIM client. Use with `--client-no-release-cid' and/or `--client-cid'",
      NULL
    },
    { NULL, 0, 0, 0, NULL, NULL, NULL }
};

GOptionGroup *
qmicli_uim_get_option_group (void)
{
    GOptionGroup *group;

    group = g_option_group_new ("uim",
                                "UIM options:",
                                "Show User Identity Module options",
                                NULL,
                                NULL);
    g_option_group_add_entries (group, entries);

    return group;
}

gboolean
qmicli_uim_options_enabled (void)
{
    static guint n_actions = 0;
    static gboolean checked = FALSE;

    if (checked)
        return !!n_actions;

    n_actions = (!!set_pin_protection_str +
                 !!verify_pin_str +
                 !!unblock_pin_str +
                 !!change_pin_str +
                 !!read_transparent_str +
                 !!read_record_str +
                 !!get_file_attributes_str +
                 !!sim_power_on_str +
                 !!sim_power_off_str +
                 !!change_provisioning_session_str +
                 !!switch_slot_str +
                 !!monitor_refresh_file_array +
                 !!depersonalization_str +
                 !!remote_unlock_str +
                 !!open_logical_channel_str +
                 !!close_logical_channel_str +
                 !!send_apdu_str +
                 get_card_status_flag +
                 get_supported_messages_flag +
                 get_slot_status_flag +
                 monitor_slot_status_flag +
                 reset_flag +
                 monitor_refresh_all_flag +
                 get_configuration_flag +
                 noop_flag);

    if (n_actions > 1) {
        g_printerr ("error: too many UIM actions requested\n");
        exit (EXIT_FAILURE);
    }

    if (monitor_slot_status_flag || monitor_refresh_file_array || monitor_refresh_all_flag)
        qmicli_expect_indications ();

    checked = TRUE;
    return !!n_actions;
}

static void
context_free (Context *context)
{
    if (!context)
        return;

    if (context->slot_status_indication_id)
        g_signal_handler_disconnect (context->client,
                                     context->slot_status_indication_id);
    if (context->refresh_indication_id)
        g_signal_handler_disconnect (context->client,
                                     context->refresh_indication_id);

    if (context->client)
        g_object_unref (context->client);
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

#if defined HAVE_QMI_MESSAGE_UIM_SET_PIN_PROTECTION || \
    defined HAVE_QMI_MESSAGE_UIM_VERIFY_PIN || \
    defined HAVE_QMI_MESSAGE_UIM_UNBLOCK_PIN ||\
    defined HAVE_QMI_MESSAGE_UIM_CHANGE_PIN

static gboolean
provisioning_session_type_handle (const gchar *key,
                                  const gchar *value,
                                  GError     **error,
                                  gpointer     user_data)
{
    QmiUimSessionType *session_type = (QmiUimSessionType *) user_data;

    if (!value || !value[0]) {
        g_set_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_FAILED,
                     "key '%s' requires a value", key);
        return FALSE;
    }

    if (g_ascii_strcasecmp (key, "session-type") == 0) {
        if (!qmicli_read_uim_session_type_from_string (value, session_type)) {
            g_set_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_FAILED,
                         "invalid session type value: %s (not a valid enum)", value);
            return FALSE;
        }
        return TRUE;
    }

    g_set_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_FAILED,
                 "Unrecognized option '%s'", key);
    return FALSE;
}

#endif

#if defined HAVE_QMI_MESSAGE_UIM_SET_PIN_PROTECTION

static QmiMessageUimSetPinProtectionInput *
set_pin_protection_input_create (const gchar *str)
{
    QmiMessageUimSetPinProtectionInput *input = NULL;
    QmiUimSessionType session_type = QMI_UIM_SESSION_TYPE_CARD_SLOT_1;
    gchar **split;
    guint len_split;
    GError *error = NULL;
    QmiUimPinId pin_id;
    gboolean enable_disable;
    gchar *current_pin;

    /* Prepare inputs.
     * Format of the string is:
     *    "[(PIN1|PIN2|UPIN),(disable|enable),(current PIN)[,'key=value,...']]" with valid key = (session-type)
     */
    split = g_strsplit (str, ",", 4);
    len_split = g_strv_length (split);

    /* Parse optional kv-pairs */
    if (len_split >= 4) {
        if (!qmicli_parse_key_value_string (split[3], &error, provisioning_session_type_handle, &session_type)) {
            g_printerr ("error: could not parse input string '%s': %s\n", str, error->message);
        }
    }

    if (error == NULL &&
        qmicli_read_uim_pin_id_from_string (split[0], &pin_id) &&
        qmicli_read_enable_disable_from_string (split[1], &enable_disable) &&
        qmicli_read_non_empty_string (split[2], "current PIN", &current_pin)) {
        GArray *placeholder_aid;

        placeholder_aid = g_array_new (FALSE, FALSE, sizeof (guint8));

        input = qmi_message_uim_set_pin_protection_input_new ();
        if (!qmi_message_uim_set_pin_protection_input_set_info (
                input,
                pin_id,
                enable_disable,
                current_pin,
                &error) ||
            !qmi_message_uim_set_pin_protection_input_set_session (
                input,
                session_type,
                placeholder_aid, /* ignored */
                &error)) {
            g_printerr ("error: couldn't create input data bundle: '%s'\n",
                        error->message);
            qmi_message_uim_set_pin_protection_input_unref (input);
            input = NULL;
        }
        g_array_unref (placeholder_aid);
    }
    g_strfreev (split);
    g_clear_error (&error);

    return input;
}

static void
set_pin_protection_ready (QmiClientUim *client,
                          GAsyncResult *res)
{
    QmiMessageUimSetPinProtectionOutput *output;
    GError *error = NULL;

    output = qmi_client_uim_set_pin_protection_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_uim_set_pin_protection_output_get_result (output, &error)) {
        guint8 verify_retries_left;
        guint8 unblock_retries_left;

        g_printerr ("error: couldn't set PIN protection: %s\n", error->message);
        g_error_free (error);

        if (qmi_message_uim_set_pin_protection_output_get_retries_remaining (
                output,
                &verify_retries_left,
                &unblock_retries_left,
                NULL)) {
            g_printerr ("[%s] Retries left:\n"
                        "\tVerify: %u\n"
                        "\tUnblock: %u\n",
                        qmi_device_get_path_display (ctx->device),
                        verify_retries_left,
                        unblock_retries_left);
        }

        qmi_message_uim_set_pin_protection_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] PIN protection updated\n",
             qmi_device_get_path_display (ctx->device));

    qmi_message_uim_set_pin_protection_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_UIM_SET_PIN_PROTECTION */

#if defined HAVE_QMI_MESSAGE_UIM_VERIFY_PIN

static QmiMessageUimVerifyPinInput *
verify_pin_input_create (const gchar *str)
{
    QmiMessageUimVerifyPinInput *input = NULL;
    QmiUimSessionType session_type = QMI_UIM_SESSION_TYPE_CARD_SLOT_1;
    gchar **split;
    guint len_split;
    GError *error = NULL;
    QmiUimPinId pin_id;
    gchar *current_pin;

    /* Prepare inputs.
     * Format of the string is:
     *    "[(PIN1|PIN2),(current PIN)[,'key=value,...']]" with valid key = (session-type)
     */
    split = g_strsplit (str, ",", 3);
    len_split = g_strv_length (split);

    /* Parse optional kv-pairs */
    if (len_split >= 3) {
        if (!qmicli_parse_key_value_string (split[2], &error, provisioning_session_type_handle, &session_type)) {
            g_printerr ("error: could not parse input string '%s': %s\n", str, error->message);
        }
    }

    if (error == NULL &&
        qmicli_read_uim_pin_id_from_string (split[0], &pin_id) &&
        qmicli_read_non_empty_string (split[1], "current PIN", &current_pin)) {
        GArray *placeholder_aid;

        placeholder_aid = g_array_new (FALSE, FALSE, sizeof (guint8));

        input = qmi_message_uim_verify_pin_input_new ();
        if (!qmi_message_uim_verify_pin_input_set_info (
                input,
                pin_id,
                current_pin,
                &error) ||
            !qmi_message_uim_verify_pin_input_set_session (
                input,
                session_type,
                placeholder_aid, /* ignored */
                &error)) {
            g_printerr ("error: couldn't create input data bundle: '%s'\n",
                        error->message);
            qmi_message_uim_verify_pin_input_unref (input);
            input = NULL;
        }
        g_array_unref (placeholder_aid);
    }
    g_strfreev (split);
    g_clear_error (&error);

    return input;
}

static void
verify_pin_ready (QmiClientUim *client,
                  GAsyncResult *res)
{
    QmiMessageUimVerifyPinOutput *output;
    GError *error = NULL;

    output = qmi_client_uim_verify_pin_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_uim_verify_pin_output_get_result (output, &error)) {
        guint8 verify_retries_left;
        guint8 unblock_retries_left;

        g_printerr ("error: couldn't verify PIN: %s\n", error->message);
        g_error_free (error);

        if (qmi_message_uim_verify_pin_output_get_retries_remaining (
                output,
                &verify_retries_left,
                &unblock_retries_left,
                NULL)) {
            g_printerr ("[%s] Retries left:\n"
                        "\tVerify: %u\n"
                        "\tUnblock: %u\n",
                        qmi_device_get_path_display (ctx->device),
                        verify_retries_left,
                        unblock_retries_left);
        }

        qmi_message_uim_verify_pin_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] PIN verified successfully\n",
             qmi_device_get_path_display (ctx->device));

    qmi_message_uim_verify_pin_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_UIM_VERIFY_PIN */

#if defined HAVE_QMI_MESSAGE_UIM_UNBLOCK_PIN

static QmiMessageUimUnblockPinInput *
unblock_pin_input_create (const gchar *str)
{
    QmiMessageUimUnblockPinInput *input = NULL;
    QmiUimSessionType session_type = QMI_UIM_SESSION_TYPE_CARD_SLOT_1;
    gchar **split;
    guint len_split;
    GError *error = NULL;
    QmiUimPinId pin_id;
    gchar *puk;
    gchar *new_pin;

    /* Prepare inputs.
     * Format of the string is:
     *    "[(PIN|PIN2),(PUK),(new PIN)[,'key=value,...']]" with valid key = (session-type)
     */
    split = g_strsplit (str, ",", 4);
    len_split = g_strv_length (split);

    /* Parse optional kv-pairs */
    if (len_split >= 4) {
        if (!qmicli_parse_key_value_string (split[3], &error, provisioning_session_type_handle, &session_type)) {
            g_printerr ("error: could not parse input string '%s': %s\n", str, error->message);
        }
    }

    if (error == NULL &&
        qmicli_read_uim_pin_id_from_string (split[0], &pin_id) &&
        qmicli_read_non_empty_string (split[1], "PUK", &puk) &&
        qmicli_read_non_empty_string (split[2], "new PIN", &new_pin)) {
        GArray *placeholder_aid;

        placeholder_aid = g_array_new (FALSE, FALSE, sizeof (guint8));

        input = qmi_message_uim_unblock_pin_input_new ();
        if (!qmi_message_uim_unblock_pin_input_set_info (
                input,
                pin_id,
                puk,
                new_pin,
                &error) ||
            !qmi_message_uim_unblock_pin_input_set_session (
                input,
                session_type,
                placeholder_aid, /* ignored */
                &error)) {
            g_printerr ("error: couldn't create input data bundle: '%s'\n",
                        error->message);
            g_error_free (error);
            qmi_message_uim_unblock_pin_input_unref (input);
            input = NULL;
        }
        g_array_unref (placeholder_aid);
    }
    g_strfreev (split);
    g_clear_error (&error);

    return input;
}

static void
unblock_pin_ready (QmiClientUim *client,
                   GAsyncResult *res)
{
    QmiMessageUimUnblockPinOutput *output;
    GError *error = NULL;

    output = qmi_client_uim_unblock_pin_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_uim_unblock_pin_output_get_result (output, &error)) {
        guint8 verify_retries_left;
        guint8 unblock_retries_left;

        g_printerr ("error: couldn't unblock PIN: %s\n", error->message);
        g_error_free (error);

        if (qmi_message_uim_unblock_pin_output_get_retries_remaining (
                output,
                &verify_retries_left,
                &unblock_retries_left,
                NULL)) {
            g_printerr ("[%s] Retries left:\n"
                        "\tVerify: %u\n"
                        "\tUnblock: %u\n",
                        qmi_device_get_path_display (ctx->device),
                        verify_retries_left,
                        unblock_retries_left);
        }

        qmi_message_uim_unblock_pin_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] PIN unblocked successfully\n",
             qmi_device_get_path_display (ctx->device));

    qmi_message_uim_unblock_pin_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_UIM_UNBLOCK_PIN */

#if defined HAVE_QMI_MESSAGE_UIM_CHANGE_PIN

static QmiMessageUimChangePinInput *
change_pin_input_create (const gchar *str)
{
    QmiMessageUimChangePinInput *input = NULL;
    QmiUimSessionType session_type = QMI_UIM_SESSION_TYPE_CARD_SLOT_1;
    gchar **split;
    guint len_split;
    GError *error = NULL;
    QmiUimPinId pin_id;
    gchar *old_pin;
    gchar *new_pin;

    /* Prepare inputs.
     * Format of the string is:
     *    "[(PIN1|PIN2),(old PIN),(new PIN)[,'key=value,...']]" with valid key = (session-type)
     */
    split = g_strsplit (str, ",", 4);
    len_split = g_strv_length (split);

    /* Parse optional kv-pairs */
    if (len_split >= 4) {
        if (!qmicli_parse_key_value_string (split[3], &error, provisioning_session_type_handle, &session_type)) {
            g_printerr ("error: could not parse input string '%s': %s\n", str, error->message);
        }
    }

    if (error == NULL &&
        qmicli_read_uim_pin_id_from_string (split[0], &pin_id) &&
        qmicli_read_non_empty_string (split[1], "old PIN", &old_pin) &&
        qmicli_read_non_empty_string (split[2], "new PIN", &new_pin)) {
        GArray *placeholder_aid;

        placeholder_aid = g_array_new (FALSE, FALSE, sizeof (guint8));

        input = qmi_message_uim_change_pin_input_new ();
        if (!qmi_message_uim_change_pin_input_set_info (
                input,
                pin_id,
                old_pin,
                new_pin,
                &error) ||
            !qmi_message_uim_change_pin_input_set_session (
                input,
                session_type,
                placeholder_aid, /* ignored */
                &error)) {
            g_printerr ("error: couldn't create input data bundle: '%s'\n",
                        error->message);
            g_error_free (error);
            qmi_message_uim_change_pin_input_unref (input);
            input = NULL;
        }
        g_array_unref (placeholder_aid);
    }
    g_strfreev (split);
    g_clear_error (&error);

    return input;
}

static void
change_pin_ready (QmiClientUim *client,
                  GAsyncResult *res)
{
    QmiMessageUimChangePinOutput *output;
    GError *error = NULL;

    output = qmi_client_uim_change_pin_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_uim_change_pin_output_get_result (output, &error)) {
        guint8 verify_retries_left;
        guint8 unblock_retries_left;

        g_printerr ("error: couldn't change PIN: %s\n", error->message);
        g_error_free (error);

        if (qmi_message_uim_change_pin_output_get_retries_remaining (
                output,
                &verify_retries_left,
                &unblock_retries_left,
                NULL)) {
            g_printerr ("[%s] Retries left:\n"
                        "\tVerify: %u\n"
                        "\tUnblock: %u\n",
                        qmi_device_get_path_display (ctx->device),
                        verify_retries_left,
                        unblock_retries_left);
        }

        qmi_message_uim_change_pin_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] PIN changed successfully\n",
             qmi_device_get_path_display (ctx->device));

    qmi_message_uim_change_pin_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_UIM_CHANGE_PIN */

#if defined HAVE_QMI_MESSAGE_UIM_GET_SUPPORTED_MESSAGES

static void
get_supported_messages_ready (QmiClientUim *client,
                              GAsyncResult *res)
{
    QmiMessageUimGetSupportedMessagesOutput *output;
    GError *error = NULL;
    GArray *bytearray = NULL;
    gchar *str;

    output = qmi_client_uim_get_supported_messages_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_uim_get_supported_messages_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get supported UIM messages: %s\n", error->message);
        g_error_free (error);
        qmi_message_uim_get_supported_messages_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] Successfully got supported UIM messages:\n",
             qmi_device_get_path_display (ctx->device));

    qmi_message_uim_get_supported_messages_output_get_list (output, &bytearray, NULL);
    str = qmicli_get_supported_messages_list (bytearray ? (const guint8 *)bytearray->data : NULL,
                                              bytearray ? bytearray->len : 0);
    g_print ("%s", str);
    g_free (str);

    qmi_message_uim_get_supported_messages_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_UIM_GET_SUPPORTED_MESSAGES */

#if defined HAVE_QMI_MESSAGE_UIM_POWER_ON_SIM

static QmiMessageUimPowerOnSimInput *
power_on_sim_input_create (const gchar *slot_str)
{
    QmiMessageUimPowerOnSimInput *input;
    guint                         slot;
    GError                       *error = NULL;

    if (!qmicli_read_uint_from_string (slot_str, &slot) || (slot > G_MAXUINT8)) {
        g_printerr ("error: invalid slot number\n");
        return NULL;
    }

    input = qmi_message_uim_power_on_sim_input_new ();

    if (!qmi_message_uim_power_on_sim_input_set_slot (input, slot, &error)) {
        g_printerr ("error: could not create SIM power on input: %s\n", error->message);
        g_error_free (error);
        qmi_message_uim_power_on_sim_input_unref (input);
        input = NULL;
    }

    return input;
}

static void
power_on_sim_ready (QmiClientUim *client,
                    GAsyncResult *res)
{
    QmiMessageUimPowerOnSimOutput *output;
    GError *error = NULL;

    output = qmi_client_uim_power_on_sim_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_uim_power_on_sim_output_get_result (output, &error)) {
        g_printerr ("error: could not power on SIM: %s\n", error->message);
        g_error_free (error);
        qmi_message_uim_power_on_sim_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] Successfully performed SIM power on\n",
             qmi_device_get_path_display (ctx->device));

    qmi_message_uim_power_on_sim_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_UIM_POWER_ON_SIM */

#if defined HAVE_QMI_MESSAGE_UIM_POWER_OFF_SIM

static QmiMessageUimPowerOffSimInput *
power_off_sim_input_create (const gchar *slot_str)
{
    QmiMessageUimPowerOffSimInput *input;
    guint                         slot;
    GError                       *error = NULL;

    if (!qmicli_read_uint_from_string (slot_str, &slot) || (slot > G_MAXUINT8)) {
        g_printerr ("error: invalid slot number\n");
        return NULL;
    }

    input = qmi_message_uim_power_off_sim_input_new ();

    if (!qmi_message_uim_power_off_sim_input_set_slot (input, slot, &error)) {
        g_printerr ("error: could not create SIM power off input: %s\n", error->message);
        g_error_free (error);
        qmi_message_uim_power_off_sim_input_unref (input);
        input = NULL;
    }

    return input;
}

static void
power_off_sim_ready (QmiClientUim *client,
                     GAsyncResult *res)
{
    QmiMessageUimPowerOffSimOutput *output;
    GError *error = NULL;

    output = qmi_client_uim_power_off_sim_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_uim_power_off_sim_output_get_result (output, &error)) {
        g_printerr ("error: could not power off SIM: %s\n", error->message);
        g_error_free (error);
        qmi_message_uim_power_off_sim_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] Successfully performed SIM power off\n",
             qmi_device_get_path_display (ctx->device));

    qmi_message_uim_power_off_sim_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_UIM_POWER_OFF_SIM */

#if defined HAVE_QMI_MESSAGE_UIM_CHANGE_PROVISIONING_SESSION

typedef struct {
    QmiUimSessionType  session_type;
    gboolean           session_type_set;
    gboolean           activate;
    gboolean           activate_set;
    guint              slot;
    GArray            *aid;
} SetChangeProvisioningSessionProperties;

static gboolean
set_change_provisioning_session_properties_handle (const gchar *key,
                                                   const gchar *value,
                                                   GError     **error,
                                                   gpointer     user_data)
{
    SetChangeProvisioningSessionProperties *props = (SetChangeProvisioningSessionProperties *) user_data;

    if (!value || !value[0]) {
        g_set_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_FAILED,
                     "key '%s' requires a value", key);
        return FALSE;
    }

    if (g_ascii_strcasecmp (key, "session-type") == 0) {
        if (!qmicli_read_uim_session_type_from_string (value, &props->session_type)) {
            g_set_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_FAILED,
                         "invalid session type value: %s (not a valid enum)", value);
            return FALSE;
        }
        props->session_type_set = TRUE;
        return TRUE;
    }

    if (g_ascii_strcasecmp (key, "activate") == 0) {
        if (!qmicli_read_yes_no_from_string (value, &props->activate)) {
            g_set_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_FAILED,
                         "invalid activate value: %s (not a boolean)", value);
            return FALSE;
        }
        props->activate_set = TRUE;
        return TRUE;
    }

    if (g_ascii_strcasecmp (key, "slot") == 0) {
        if (!qmicli_read_uint_from_string (value, &props->slot)) {
            g_set_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_FAILED,
                         "invalid slot value: %s (not a number)", value);
            return FALSE;
        }
        if (props->slot > G_MAXUINT8) {
            g_set_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_FAILED,
                         "invalid slot value: %s (out of range)", value);
            return FALSE;
        }
        return TRUE;
    }

    if (g_ascii_strcasecmp (key, "aid") == 0) {
        if (!qmicli_read_raw_data_from_string (value, &props->aid)) {
            g_set_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_FAILED,
                         "invalid aid value: %s (not an hex string)", value);
            return FALSE;
        }
        return TRUE;
    }

    g_set_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_FAILED,
                 "Unrecognized option '%s'", key);
    return FALSE;
}

static QmiMessageUimChangeProvisioningSessionInput *
change_provisioning_session_input_create (const gchar *str)
{
    QmiMessageUimChangeProvisioningSessionInput *input;
    GError                                      *error = NULL;
    SetChangeProvisioningSessionProperties       props = { 0 };

    input = qmi_message_uim_change_provisioning_session_input_new ();

    if (!qmicli_parse_key_value_string (str,
                                        &error,
                                        set_change_provisioning_session_properties_handle,
                                        &props)) {
        g_printerr ("error: could not parse input string '%s': %s\n", str, error->message);
        g_error_free (error);
        g_clear_pointer (&input, qmi_message_uim_change_provisioning_session_input_unref);
        goto out;
    }

    if (!props.session_type_set || !props.activate_set) {
        g_printerr ("error: mandatory fields 'session-type' and 'activate' not given\n");
        g_clear_pointer (&input, qmi_message_uim_change_provisioning_session_input_unref);
        goto out;
    }

    qmi_message_uim_change_provisioning_session_input_set_session_change (
        input,
        props.session_type,
        props.activate,
        NULL);

    if (props.slot || props.aid) {
        GArray *aid = NULL;

        aid = props.aid ? g_array_ref (props.aid) : g_array_new (FALSE, FALSE, sizeof (guint8));
        qmi_message_uim_change_provisioning_session_input_set_application_information (
            input,
            props.slot,
            aid,
            NULL);
        g_array_unref (aid);
    }

out:
    return input;
}

static void
change_provisioning_session_ready (QmiClientUim *client,
                                   GAsyncResult *res)
{
    QmiMessageUimChangeProvisioningSessionOutput *output;
    GError *error = NULL;

    output = qmi_client_uim_change_provisioning_session_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_uim_change_provisioning_session_output_get_result (output, &error)) {
        g_printerr ("error: could not power off SIM: %s\n", error->message);
        g_error_free (error);
        qmi_message_uim_change_provisioning_session_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] Successfully changed provisioning session\n",
             qmi_device_get_path_display (ctx->device));

    qmi_message_uim_change_provisioning_session_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_UIM_CHANGE_PROVISIONING_SESSION */

#if (defined HAVE_QMI_MESSAGE_UIM_GET_SLOT_STATUS || \
     defined HAVE_QMI_INDICATION_UIM_SLOT_STATUS)

static const gchar bcd_chars[] = "0123456789\0\0\0\0\0\0";

static gchar *
decode_iccid (const gchar *bcd, gsize bcd_len)
{
    GString *str;
    gsize i;

    if (!bcd)
        return NULL;

    str = g_string_sized_new (bcd_len * 2 + 1);
    for (i = 0; i < bcd_len; i++) {
        str = g_string_append_c (str, bcd_chars[bcd[i] & 0xF]);
        str = g_string_append_c (str, bcd_chars[(bcd[i] >> 4) & 0xF]);
    }
    return g_string_free (str, FALSE);
}

#define EID_LENGTH 16

static gchar *
decode_eid (const gchar *eid, gsize eid_len)
{
    GString *str;
    gsize i;

    if (!eid)
        return NULL;
    if (eid_len != EID_LENGTH)
        return NULL;

    str = g_string_sized_new (eid_len * 2 + 1);
    for (i = 0; i < eid_len; i++) {
        str = g_string_append_c (str, bcd_chars[(eid[i] >> 4) & 0xF]);
        str = g_string_append_c (str, bcd_chars[eid[i] & 0xF]);
    }
    return g_string_free (str, FALSE);
}

static void
print_slot_status (GArray *physical_slots,
                   GArray *ext_information,
                   GArray *slot_eids)
{
    guint i;

    if (ext_information && physical_slots->len != ext_information->len) {
        g_print ("Malformed extended information data");
        ext_information = NULL;
    }

    if (slot_eids && physical_slots->len != slot_eids->len) {
        g_print ("Malformed slot EID data");
        slot_eids = NULL;
    }

    for (i = 0; i < physical_slots->len; i++) {
        QmiPhysicalSlotStatusSlot *slot_status;
        QmiPhysicalSlotInformationSlot *slot_info = NULL;
        QmiSlotEidElement *slot_eid = NULL;
        g_autofree gchar *iccid = NULL;
        g_autofree gchar *eid = NULL;

        slot_status = &g_array_index (physical_slots, QmiPhysicalSlotStatusSlot, i);

        g_print ("  Physical slot %u:\n", i + 1);
        g_print ("     Card status: %s\n",
                 qmi_uim_physical_card_state_get_string (slot_status->physical_card_status));
        g_print ("     Slot status: %s\n",
                 qmi_uim_slot_state_get_string (slot_status->physical_slot_status));

        if (slot_status->physical_slot_status == QMI_UIM_SLOT_STATE_ACTIVE)
            g_print ("    Logical slot: %u\n", slot_status->logical_slot);

        if (slot_status->physical_card_status != QMI_UIM_PHYSICAL_CARD_STATE_PRESENT)
            continue;

        if (slot_status->iccid->len)
            iccid = decode_iccid (slot_status->iccid->data, slot_status->iccid->len);
        g_print ("           ICCID: %s\n", VALIDATE_UNKNOWN (iccid));

        /* Extended information, if available */
        if (!ext_information)
            continue;

        slot_info = &g_array_index (ext_information, QmiPhysicalSlotInformationSlot, i);
        g_print ("        Protocol: %s\n",
                 qmi_uim_card_protocol_get_string (slot_info->card_protocol));
        g_print ("        Num apps: %u\n", slot_info->valid_applications);
        g_print ("        Is eUICC: %s\n", slot_info->is_euicc ? "yes" : "no");

        /* EID info, if available and this is an eUICC */
        if (!slot_info->is_euicc || !slot_eids)
            continue;

        slot_eid = &g_array_index (slot_eids, QmiSlotEidElement, i);
        if (slot_eid->eid)
            eid = decode_eid (slot_eid->eid->data, slot_eid->eid->len);
        g_print ("             EID: %s\n", VALIDATE_UNKNOWN (eid));
    }
}

#endif

#if defined HAVE_QMI_MESSAGE_UIM_GET_SLOT_STATUS

static void
get_slot_status_ready (QmiClientUim *client,
                       GAsyncResult *res)
{
    QmiMessageUimGetSlotStatusOutput *output;
    GArray *physical_slots;
    GArray *ext_information = NULL;
    GArray *slot_eids = NULL;
    GError *error = NULL;

    output = qmi_client_uim_get_slot_status_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_uim_get_slot_status_output_get_result (output, &error)) {
        g_printerr ("error: could not get slots status: %s\n", error->message);
        g_error_free (error);
        qmi_message_uim_get_slot_status_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] Successfully got slots status\n",
             qmi_device_get_path_display (ctx->device));

    if (!qmi_message_uim_get_slot_status_output_get_physical_slot_status (
            output, &physical_slots, &error)) {
        g_printerr ("error: could not parse slots status response: %s\n", error->message);
        g_error_free (error);
        qmi_message_uim_get_slot_status_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    /* Both of these are recoverable, just print less information per slot */
    qmi_message_uim_get_slot_status_output_get_physical_slot_information (output, &ext_information, NULL);
    qmi_message_uim_get_slot_status_output_get_slot_eid (output, &slot_eids, NULL);

    g_print ("[%s] %u physical slots found:\n",
             qmi_device_get_path_display (ctx->device), physical_slots->len);

    print_slot_status (physical_slots, ext_information, slot_eids);

    qmi_message_uim_get_slot_status_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_UIM_GET_SLOT_STATUS */

#if defined HAVE_QMI_MESSAGE_UIM_SWITCH_SLOT && defined HAVE_QMI_MESSAGE_UIM_GET_SLOT_STATUS

static void
switch_slot_ready (QmiClientUim *client,
                   GAsyncResult *res)
{
    QmiMessageUimSwitchSlotOutput *output;
    GError *error = NULL;

    output = qmi_client_uim_switch_slot_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_uim_switch_slot_output_get_result (output, &error)) {
        g_printerr ("error: couldn't switch slots: %s\n", error->message);
        g_error_free (error);
        qmi_message_uim_switch_slot_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] Successfully switched slots\n",
             qmi_device_get_path_display (ctx->device));

    qmi_message_uim_switch_slot_output_unref (output);
    operation_shutdown (TRUE);
}

static QmiMessageUimSwitchSlotInput *
switch_slot_input_create (guint logical_slot,
                          guint physical_slot)
{
    QmiMessageUimSwitchSlotInput *input;
    GError                       *error = NULL;

    input = qmi_message_uim_switch_slot_input_new ();

    if (!qmi_message_uim_switch_slot_input_set_logical_slot (input, logical_slot, &error) ||
        !qmi_message_uim_switch_slot_input_set_physical_slot (input, physical_slot, &error)) {
        g_printerr ("error: could not create switch slot input: %s\n", error->message);
        g_error_free (error);
        g_clear_pointer (&input, qmi_message_uim_switch_slot_input_unref);
    }

    return input;
}

static void
get_active_logical_slot_ready (QmiClientUim *client,
                               GAsyncResult *res,
                               gpointer user_data)
{
    QmiMessageUimGetSlotStatusOutput *output;
    QmiMessageUimSwitchSlotInput *input;
    GArray *physical_slots;
    guint physical_slot_id;
    guint active_logical_slot_id = 0;
    guint i;
    GError *error = NULL;

    physical_slot_id = GPOINTER_TO_UINT (user_data);

    output = qmi_client_uim_get_slot_status_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_uim_get_slot_status_output_get_result (output, &error)) {
        g_printerr ("error: could not get slots status: %s\n", error->message);
        g_error_free (error);
        qmi_message_uim_get_slot_status_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_uim_get_slot_status_output_get_physical_slot_status (
          output, &physical_slots, &error)) {
        g_printerr ("error: could not parse slots status response: %s\n", error->message);
        g_error_free (error);
        qmi_message_uim_get_slot_status_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    /* Ensure the physical slot is available. */
    if (physical_slot_id > physical_slots->len) {
        g_printerr ("error: physical slot %u is unavailable\n", physical_slot_id);
        qmi_message_uim_get_slot_status_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    /* Find active logical slot */
    for (i = 0; i < physical_slots->len; i++) {
        QmiPhysicalSlotStatusSlot *slot_status;

        slot_status = &g_array_index (physical_slots, QmiPhysicalSlotStatusSlot, i);
        if (slot_status->physical_slot_status == QMI_UIM_SLOT_STATE_ACTIVE) {
            active_logical_slot_id = slot_status->logical_slot;
            break;
        }
    }
    qmi_message_uim_get_slot_status_output_unref (output);

    if (active_logical_slot_id == 0) {
        g_printerr ("error: no active logical slot\n");
        operation_shutdown (FALSE);
        return;
    }

    input = switch_slot_input_create (active_logical_slot_id, physical_slot_id);
    qmi_client_uim_switch_slot (ctx->client,
                                input,
                                10,
                                ctx->cancellable,
                                (GAsyncReadyCallback)switch_slot_ready,
                                ctx);
    qmi_message_uim_switch_slot_input_unref (input);
}

#endif /* HAVE_QMI_MESSAGE_UIM_SWITCH_SLOT && HAVE_QMI_MESSAGE_UIM_GET_SLOT_STATUS */

#if defined HAVE_QMI_INDICATION_UIM_SLOT_STATUS

static void
monitoring_cancelled (GCancellable *cancellable)
{
    operation_shutdown (TRUE);
}

static void
slot_status_received (QmiClientUim *client,
                      QmiIndicationUimSlotStatusOutput *output)
{
    GArray *physical_slots;
    GArray *ext_information = NULL;
    GArray *slot_eids = NULL;
    GError *error = NULL;

    g_print ("[%s] Received slot status indication:\n",
             qmi_device_get_path_display (ctx->device));

    if (!qmi_indication_uim_slot_status_output_get_physical_slot_status (
          output, &physical_slots, &error)) {
        g_printerr ("error: could not parse slots status: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    /* Both of these are recoverable, just print less information per slot */
    qmi_indication_uim_slot_status_output_get_physical_slot_information (output, &ext_information, NULL);
    qmi_indication_uim_slot_status_output_get_slot_eid (output, &slot_eids, NULL);

    print_slot_status (physical_slots, ext_information, slot_eids);
}

static void
register_physical_slot_status_events_ready (QmiClientUim *client,
                                            GAsyncResult *res)
{
    QmiMessageUimRegisterEventsOutput *output;
    GError                            *error = NULL;

    output = qmi_client_uim_register_events_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_uim_register_events_output_get_result (output, &error)) {
        g_printerr ("error: could not register slot status change events: %s\n", error->message);
        qmi_message_uim_register_events_output_unref (output);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    g_debug ("Registered physical slot status change events...");
    ctx->slot_status_indication_id =
        g_signal_connect (ctx->client,
                          "slot-status",
                          G_CALLBACK (slot_status_received),
                          NULL);

    /* User can use Ctrl+C to cancel the monitoring at any time */
    g_cancellable_connect (ctx->cancellable,
                           G_CALLBACK (monitoring_cancelled),
                           NULL,
                           NULL);
}

static void
register_physical_slot_status_events (void)
{
    QmiMessageUimRegisterEventsInput *re_input;

    re_input = qmi_message_uim_register_events_input_new ();
    qmi_message_uim_register_events_input_set_event_registration_mask (
        re_input, QMI_UIM_EVENT_REGISTRATION_FLAG_PHYSICAL_SLOT_STATUS, NULL);
    qmi_client_uim_register_events (
        ctx->client,
        re_input,
        10,
        ctx->cancellable,
        (GAsyncReadyCallback) register_physical_slot_status_events_ready,
        NULL);
    qmi_message_uim_register_events_input_unref (re_input);
}

#endif /* HAVE_QMI_INDICATION_UIM_SLOT_STATUS */

#if defined HAVE_QMI_MESSAGE_UIM_RESET

static void
reset_ready (QmiClientUim *client,
             GAsyncResult *res)
{
    QmiMessageUimResetOutput *output;
    GError *error = NULL;

    output = qmi_client_uim_reset_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_uim_reset_output_get_result (output, &error)) {
        g_printerr ("error: couldn't reset the UIM service: %s\n", error->message);
        g_error_free (error);
        qmi_message_uim_reset_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] Successfully performed UIM service reset\n",
             qmi_device_get_path_display (ctx->device));

    qmi_message_uim_reset_output_unref (output);
    operation_shutdown (TRUE);
}

#endif

static gboolean
noop_cb (gpointer unused)
{
    operation_shutdown (TRUE);
    return FALSE;
}

#if defined HAVE_QMI_MESSAGE_UIM_GET_CARD_STATUS

static void
get_card_status_ready (QmiClientUim *client,
                       GAsyncResult *res)
{
    QmiMessageUimGetCardStatusOutput *output;
    GError *error = NULL;
    guint16 index_gw_primary;
    guint16 index_1x_primary;
    guint16 index_gw_secondary;
    guint16 index_1x_secondary;
    GArray *cards;
    guint i;

    output = qmi_client_uim_get_card_status_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_uim_get_card_status_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get card status: %s\n", error->message);
        g_error_free (error);
        qmi_message_uim_get_card_status_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] Successfully got card status\n",
             qmi_device_get_path_display (ctx->device));

    qmi_message_uim_get_card_status_output_get_card_status (
        output,
        &index_gw_primary,
        &index_1x_primary,
        &index_gw_secondary,
        &index_1x_secondary,
        &cards,
        NULL);

    g_print ("Provisioning applications:\n");
    if (index_gw_primary == 0xFFFF)
        g_print ("\tPrimary GW:   session doesn't exist\n");
    else
        g_print ("\tPrimary GW:   slot '%u', application '%u'\n",
                 ((index_gw_primary & 0xFF00) >> 8) + 1,
                 ((index_gw_primary & 0x00FF)) + 1);

    if (index_1x_primary == 0xFFFF)
        g_print ("\tPrimary 1X:   session doesn't exist\n");
    else
        g_print ("\tPrimary 1X:   slot '%u', application '%u'\n",
                 ((index_1x_primary & 0xFF00) >> 8) + 1,
                 ((index_1x_primary & 0x00FF)) + 1);

    if (index_gw_secondary == 0xFFFF)
        g_print ("\tSecondary GW: session doesn't exist\n");
    else
        g_print ("\tSecondary GW: slot '%u', application '%u'\n",
                 ((index_gw_secondary & 0xFF00) >> 8) + 1,
                 ((index_gw_secondary & 0x00FF)) + 1);

    if (index_1x_secondary == 0xFFFF)
        g_print ("\tSecondary 1X: session doesn't exist\n");
    else
        g_print ("\tSecondary 1X: slot '%u', application '%u'\n",
                 ((index_1x_secondary & 0xFF00) >> 8) + 1,
                 ((index_1x_secondary & 0x00FF)) + 1);

    for (i = 0; i < cards->len; i++) {
        QmiMessageUimGetCardStatusOutputCardStatusCardsElement *card;
        guint j;

        card = &g_array_index (cards, QmiMessageUimGetCardStatusOutputCardStatusCardsElement, i);

        g_print ("Slot [%u]:\n", i + 1);

        if (card->card_state != QMI_UIM_CARD_STATE_ERROR)
            g_print ("\tCard state: '%s'\n",
                     qmi_uim_card_state_get_string (card->card_state));
        else
            g_print ("\tCard state: '%s: %s (%u)'\n",
                     qmi_uim_card_state_get_string (card->card_state),
                     VALIDATE_UNKNOWN (qmi_uim_card_error_get_string (card->error_code)),
                     card->error_code);
        g_print ("\tUPIN state: '%s'\n"
                 "\t\tUPIN retries: '%u'\n"
                 "\t\tUPUK retries: '%u'\n",
                 qmi_uim_pin_state_get_string (card->upin_state),
                 card->upin_retries,
                 card->upuk_retries);

        for (j = 0; j < card->applications->len; j++) {
            QmiMessageUimGetCardStatusOutputCardStatusCardsElementApplicationsElementV2 *app;
            gchar *str;

            app = &g_array_index (card->applications, QmiMessageUimGetCardStatusOutputCardStatusCardsElementApplicationsElementV2, j);

            str = qmicli_get_raw_data_printable (app->application_identifier_value, 80, "");

            g_print ("\tApplication [%u]:\n"
                     "\t\tApplication type:  '%s (%u)'\n"
                     "\t\tApplication state: '%s'\n"
                     "\t\tApplication ID:\n"
                     "\t\t\t%s",
                     j + 1,
                     VALIDATE_UNKNOWN (qmi_uim_card_application_type_get_string (app->type)), app->type,
                     qmi_uim_card_application_state_get_string (app->state),
                     str);

            if (app->personalization_state == QMI_UIM_CARD_APPLICATION_PERSONALIZATION_STATE_CODE_REQUIRED ||
                app->personalization_state == QMI_UIM_CARD_APPLICATION_PERSONALIZATION_STATE_PUK_CODE_REQUIRED)
                g_print ("\t\tPersonalization state: '%s' (feature: %s)\n"
                         "\t\t\tDisable retries:     '%u'\n"
                         "\t\t\tUnblock retries:     '%u'\n",
                         qmi_uim_card_application_personalization_state_get_string (app->personalization_state),
                         qmi_uim_card_application_personalization_feature_status_get_string (app->personalization_feature),
                         app->personalization_retries,
                         app->personalization_unblock_retries);
            else
                g_print ("\t\tPersonalization state: '%s'\n",
                         qmi_uim_card_application_personalization_state_get_string (app->personalization_state));

            g_print ("\t\tUPIN replaces PIN1: '%s'\n",
                     app->upin_replaces_pin1 ? "yes" : "no");

            g_print ("\t\tPIN1 state: '%s'\n"
                     "\t\t\tPIN1 retries: '%u'\n"
                     "\t\t\tPUK1 retries: '%u'\n"
                     "\t\tPIN2 state: '%s'\n"
                     "\t\t\tPIN2 retries: '%u'\n"
                     "\t\t\tPUK2 retries: '%u'\n",
                     qmi_uim_pin_state_get_string (app->pin1_state),
                     app->pin1_retries,
                     app->puk1_retries,
                     qmi_uim_pin_state_get_string (app->pin2_state),
                     app->pin2_retries,
                     app->puk2_retries);
            g_free (str);
        }
    }

    qmi_message_uim_get_card_status_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* defined HAVE_QMI_MESSAGE_UIM_GET_CARD_STATUS */

#if defined HAVE_QMI_MESSAGE_UIM_READ_TRANSPARENT || \
    defined HAVE_QMI_MESSAGE_UIM_READ_RECORD || \
    defined HAVE_QMI_MESSAGE_UIM_GET_FILE_ATTRIBUTES

static gboolean
get_sim_file_id_and_path_with_separator (const gchar *file_path_str,
                                         guint16 *file_id,
                                         GArray **file_path,
                                         const gchar *separator)
{
    guint i;
    gchar **split;

    split = g_strsplit (file_path_str, separator, -1);
    if (!split) {
        g_printerr ("error: invalid file path given: '%s'\n", file_path_str);
        return FALSE;
    }

    *file_path = g_array_sized_new (FALSE,
                                    FALSE,
                                    sizeof (guint8),
                                    g_strv_length (split) - 1);

    *file_id = 0;
    for (i = 0; split[i]; i++) {
        gulong path_item;

        path_item = (strtoul (split[i], NULL, 16)) & 0xFFFF;

        /* If there are more fields, this is part of the path; otherwise it's
         * the file id */
        if (split[i + 1]) {
            guint8 val;

            val = path_item & 0xFF;
            g_array_append_val (*file_path, val);
            val = (path_item >> 8) & 0xFF;
            g_array_append_val (*file_path, val);
        } else {
            *file_id = path_item;
        }
    }

    g_strfreev (split);

    if (*file_id == 0) {
        g_array_unref (*file_path);
        g_printerr ("error: invalid file path given: '%s'\n", file_path_str);
        return FALSE;
    }

    return TRUE;
}

#endif /* HAVE_QMI_MESSAGE_UIM_READ_TRANSPARENT
        * HAVE_QMI_MESSAGE_UIM_READ_RECORD
        * HAVE_QMI_MESSAGE_UIM_GET_FILE_ATTRIBUTES */

#if defined HAVE_QMI_MESSAGE_UIM_READ_TRANSPARENT || \
    defined HAVE_QMI_MESSAGE_UIM_GET_FILE_ATTRIBUTES || \
    defined HAVE_QMI_MESSAGE_UIM_REFRESH_REGISTER

static gboolean
get_sim_file_id_and_path (const gchar *file_path_str,
                          guint16 *file_id,
                          GArray **file_path)
{
    return get_sim_file_id_and_path_with_separator (file_path_str, file_id, file_path, ",");
}

#endif /* HAVE_QMI_MESSAGE_UIM_READ_TRANSPARENT
        * HAVE_QMI_MESSAGE_UIM_GET_FILE_ATTRIBUTES
        * HAVE_QMI_MESSAGE_UIM_REFRESH_REGISTER */

#if defined HAVE_QMI_MESSAGE_UIM_READ_TRANSPARENT

static void
read_transparent_ready (QmiClientUim *client,
                        GAsyncResult *res)
{
    QmiMessageUimReadTransparentOutput *output;
    GError *error = NULL;
    guint8 sw1 = 0;
    guint8 sw2 = 0;
    GArray *read_result = NULL;

    output = qmi_client_uim_read_transparent_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_uim_read_transparent_output_get_result (output, &error)) {
        g_printerr ("error: couldn't read transparent file from the UIM: %s\n", error->message);
        g_error_free (error);

        /* Card result */
        if (qmi_message_uim_read_transparent_output_get_card_result (
                output,
                &sw1,
                &sw2,
                NULL)) {
            g_print ("Card result:\n"
                     "\tSW1: '0x%02x'\n"
                     "\tSW2: '0x%02x'\n",
                     sw1, sw2);
        }

        qmi_message_uim_read_transparent_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] Successfully read information from the UIM:\n",
             qmi_device_get_path_display (ctx->device));

    /* Card result */
    if (qmi_message_uim_read_transparent_output_get_card_result (
            output,
            &sw1,
            &sw2,
            NULL)) {
        g_print ("Card result:\n"
                 "\tSW1: '0x%02x'\n"
                 "\tSW2: '0x%02x'\n",
                 sw1, sw2);
    }

    /* Read result */
    if (qmi_message_uim_read_transparent_output_get_read_result (
            output,
            &read_result,
            NULL)) {
        gchar *str;

        str = qmicli_get_raw_data_printable (read_result, 80, "\t");
        g_print ("Read result:\n"
                 "%s\n",
                 str);
        g_free (str);
    }

    qmi_message_uim_read_transparent_output_unref (output);
    operation_shutdown (TRUE);
}

static QmiMessageUimReadTransparentInput *
read_transparent_build_input (const gchar *file_path_str)
{
    QmiMessageUimReadTransparentInput *input;
    guint16 file_id = 0;
    GArray *file_path = NULL;
    GArray *placeholder_aid;

    if (!get_sim_file_id_and_path (file_path_str, &file_id, &file_path))
        return NULL;

    placeholder_aid = g_array_new (FALSE, FALSE, sizeof (guint8));

    input = qmi_message_uim_read_transparent_input_new ();
    qmi_message_uim_read_transparent_input_set_session (
        input,
        QMI_UIM_SESSION_TYPE_PRIMARY_GW_PROVISIONING,
        placeholder_aid, /* ignored */
        NULL);
    qmi_message_uim_read_transparent_input_set_file (
        input,
        file_id,
        file_path,
        NULL);
    qmi_message_uim_read_transparent_input_set_read_information (input, 0, 0, NULL);
    g_array_unref (file_path);
    g_array_unref (placeholder_aid);
    return input;
}

#endif /* HAVE_QMI_MESSAGE_UIM_READ_TRANSPARENT */

#if defined HAVE_QMI_MESSAGE_UIM_READ_RECORD

static void
read_record_ready (QmiClientUim *client,
                   GAsyncResult *res)
{
    QmiMessageUimReadRecordOutput *output;
    GError *error = NULL;
    guint8 sw1 = 0;
    guint8 sw2 = 0;
    GArray *read_result = NULL;

    output = qmi_client_uim_read_record_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_uim_read_record_output_get_result (output, &error)) {
        g_printerr ("error: couldn't read record file from the UIM: %s\n", error->message);
        g_error_free (error);

        /* Card result */
        if (qmi_message_uim_read_record_output_get_card_result (
                output,
                &sw1,
                &sw2,
                NULL)) {
            g_print ("Card result:\n"
                     "\tSW1: '0x%02x'\n"
                     "\tSW2: '0x%02x'\n",
                     sw1, sw2);
        }

        qmi_message_uim_read_record_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] Successfully read information from the UIM:\n",
             qmi_device_get_path_display (ctx->device));

    /* Card result */
    if (qmi_message_uim_read_record_output_get_card_result (
            output,
            &sw1,
            &sw2,
            NULL)) {
        g_print ("Card result:\n"
                 "\tSW1: '0x%02x'\n"
                 "\tSW2: '0x%02x'\n",
                 sw1, sw2);
    }

    /* Read result */
    if (qmi_message_uim_read_record_output_get_read_result (
            output,
            &read_result,
            NULL)) {
        gchar *str;

        str = qmicli_get_raw_data_printable (read_result, 80, "\t");
        g_print ("Read result:\n"
                 "%s\n",
                 str);
        g_free (str);
    }

    qmi_message_uim_read_record_output_unref (output);
    operation_shutdown (TRUE);
}

typedef struct {
    char *file;
    guint16 record_number;
    guint16 record_length;
} SetReadRecordProperties;

static gboolean
set_read_record_properties_handle (const gchar *key,
                                   const gchar *value,
                                   GError     **error,
                                   gpointer     user_data)
{
    SetReadRecordProperties *props = (SetReadRecordProperties *) user_data;

    if (!value || !value[0]) {
        g_set_error (error,
                     QMI_CORE_ERROR,
                     QMI_CORE_ERROR_FAILED,
                     "key '%s' requires a value",
                     key);
        return FALSE;
    }

    if (g_ascii_strcasecmp (key, "file") == 0) {
        props->file = g_strdup (value);
        return TRUE;
    }

    if (g_ascii_strcasecmp (key, "record-number") == 0) {
        guint aux;

        if (!qmicli_read_uint_from_string (value, &aux) || (aux > G_MAXUINT16)) {
            g_set_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_FAILED,
                         "failed reading key 'record-number' as 16bit value");
            return FALSE;
        }
        props->record_number = (guint16) aux;
        return TRUE;
    }

    if (g_ascii_strcasecmp (key, "record-length") == 0) {
        guint aux;

        if (!qmicli_read_uint_from_string (value, &aux) || (aux > G_MAXUINT16)) {
            g_set_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_FAILED,
                         "failed reading key 'record-length' as 16bit value");
            return FALSE;
        }
        props->record_length = (guint16) aux;
        return TRUE;
    }

    g_set_error (error,
                 QMI_CORE_ERROR,
                 QMI_CORE_ERROR_FAILED,
                 "Unrecognized option '%s'",
                 key);
    return FALSE;
}

static QmiMessageUimReadRecordInput *
read_record_input_create (const gchar *str)
{
    GError *error = NULL;
    QmiMessageUimReadRecordInput *input = NULL;
    SetReadRecordProperties props = {
        .file = NULL,
        .record_number = 0,
        .record_length = 0,
    };
    guint16 file_id = 0;
    GArray *file_path = NULL;
    GArray *placeholder_aid;

    if (!qmicli_parse_key_value_string (str,
                                        &error,
                                        set_read_record_properties_handle,
                                        &props)) {
        g_printerr ("error: could not parse input string '%s': %s\n",
                    str,
                    error->message);
        g_error_free (error);
        goto out;
    }

    if (!get_sim_file_id_and_path_with_separator (props.file, &file_id, &file_path, "-"))
        goto out;

    placeholder_aid = g_array_new (FALSE, FALSE, sizeof (guint8));

    input = qmi_message_uim_read_record_input_new ();

    qmi_message_uim_read_record_input_set_session (
        input,
        QMI_UIM_SESSION_TYPE_PRIMARY_GW_PROVISIONING,
        placeholder_aid, /* ignored */
        NULL);
    qmi_message_uim_read_record_input_set_file (
        input,
        file_id,
        file_path,
        NULL);
    qmi_message_uim_read_record_input_set_record (
        input,
        props.record_number,
        props.record_length,
        NULL);

    g_array_unref (placeholder_aid);

out:
    free (props.file);
    g_array_unref (file_path);
    return input;
}

#endif /* HAVE_QMI_MESSAGE_UIM_READ_RECORD */

#if defined HAVE_QMI_MESSAGE_UIM_GET_FILE_ATTRIBUTES

static void
get_file_attributes_ready (QmiClientUim *client,
                           GAsyncResult *res,
                           gchar *file_name)
{
    QmiMessageUimGetFileAttributesOutput *output;
    GError *error = NULL;
    guint8 sw1 = 0;
    guint8 sw2 = 0;
    guint16 file_size;
    guint16 file_id;
    QmiUimFileType file_type;
    guint16 record_size;
    guint16 record_count;
    QmiUimSecurityAttributeLogic read_security_attributes_logic;
    QmiUimSecurityAttribute read_security_attributes;
    QmiUimSecurityAttributeLogic write_security_attributes_logic;
    QmiUimSecurityAttribute write_security_attributes;
    QmiUimSecurityAttributeLogic increase_security_attributes_logic;
    QmiUimSecurityAttribute increase_security_attributes;
    QmiUimSecurityAttributeLogic deactivate_security_attributes_logic;
    QmiUimSecurityAttribute deactivate_security_attributes;
    QmiUimSecurityAttributeLogic activate_security_attributes_logic;
    QmiUimSecurityAttribute activate_security_attributes;
    GArray *raw = NULL;

    output = qmi_client_uim_get_file_attributes_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        g_free (file_name);
        return;
    }

    if (!qmi_message_uim_get_file_attributes_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get '%s' file attributes from the UIM: %s\n",
                    file_name,
                    error->message);
        g_error_free (error);

        /* Card result */
        if (qmi_message_uim_get_file_attributes_output_get_card_result (
                output,
                &sw1,
                &sw2,
                NULL)) {
            g_print ("Card result:\n"
                     "\tSW1: '0x%02x'\n"
                     "\tSW2: '0x%02x'\n",
                     sw1, sw2);
        }

        qmi_message_uim_get_file_attributes_output_unref (output);
        operation_shutdown (FALSE);
        g_free (file_name);
        return;
    }

    g_print ("[%s] Successfully got file '%s' attributes from the UIM:\n",
             file_name,
             qmi_device_get_path_display (ctx->device));

    /* Card result */
    if (qmi_message_uim_get_file_attributes_output_get_card_result (
            output,
            &sw1,
            &sw2,
            NULL)) {
        g_print ("Card result:\n"
                 "\tSW1: '0x%02x'\n"
                 "\tSW2: '0x%02x'\n",
                 sw1, sw2);
    }

    /* File attributes */
    if (qmi_message_uim_get_file_attributes_output_get_file_attributes (
            output,
            &file_size,
            &file_id,
            &file_type,
            &record_size,
            &record_count,
            &read_security_attributes_logic,
            &read_security_attributes,
            &write_security_attributes_logic,
            &write_security_attributes,
            &increase_security_attributes_logic,
            &increase_security_attributes,
            &deactivate_security_attributes_logic,
            &deactivate_security_attributes,
            &activate_security_attributes_logic,
            &activate_security_attributes,
            &raw,
            NULL)) {
        g_autofree gchar *read_security_attributes_str = NULL;
        g_autofree gchar *write_security_attributes_str = NULL;
        g_autofree gchar *increase_security_attributes_str = NULL;
        g_autofree gchar *deactivate_security_attributes_str = NULL;
        g_autofree gchar *activate_security_attributes_str = NULL;
        g_autofree gchar *raw_str = NULL;

        g_print ("File attributes:\n");
        g_print ("\tFile size: %u\n", (guint)file_size);
        g_print ("\tFile ID: %u\n", (guint)file_id);
        g_print ("\tFile type: %s\n", qmi_uim_file_type_get_string (file_type));
        g_print ("\tRecord size: %u\n", (guint)record_size);
        g_print ("\tRecord count: %u\n", (guint)record_count);

        read_security_attributes_str = qmi_uim_security_attribute_build_string_from_mask (read_security_attributes);
        g_print ("\tRead security attributes: (%s) %s\n",
                 qmi_uim_security_attribute_logic_get_string (read_security_attributes_logic),
                 VALIDATE_MASK_NONE (read_security_attributes_str));

        write_security_attributes_str = qmi_uim_security_attribute_build_string_from_mask (write_security_attributes);
        g_print ("\tWrite security attributes: (%s) %s\n",
                 qmi_uim_security_attribute_logic_get_string (write_security_attributes_logic),
                 VALIDATE_MASK_NONE (write_security_attributes_str));

        increase_security_attributes_str = qmi_uim_security_attribute_build_string_from_mask (increase_security_attributes);
        g_print ("\tIncrease security attributes: (%s) %s\n",
                 qmi_uim_security_attribute_logic_get_string (increase_security_attributes_logic),
                 VALIDATE_MASK_NONE (increase_security_attributes_str));

        deactivate_security_attributes_str = qmi_uim_security_attribute_build_string_from_mask (deactivate_security_attributes);
        g_print ("\tDeactivate security attributes: (%s) %s\n",
                 qmi_uim_security_attribute_logic_get_string (deactivate_security_attributes_logic),
                 VALIDATE_MASK_NONE (deactivate_security_attributes_str));

        activate_security_attributes_str = qmi_uim_security_attribute_build_string_from_mask (activate_security_attributes);
        g_print ("\tActivate security attributes: (%s) %s\n",
                 qmi_uim_security_attribute_logic_get_string (activate_security_attributes_logic),
                 VALIDATE_MASK_NONE (activate_security_attributes_str));

        raw_str = qmicli_get_raw_data_printable (raw, 80, "\t");
        g_print ("\tRaw: %s\n", raw_str);
    }

    qmi_message_uim_get_file_attributes_output_unref (output);
    operation_shutdown (TRUE);
}

static QmiMessageUimGetFileAttributesInput *
get_file_attributes_build_input (const gchar *file_path_str)
{
    QmiMessageUimGetFileAttributesInput *input;
    guint16 file_id = 0;
    GArray *file_path = NULL;
    GArray *placeholder_aid;

    if (!get_sim_file_id_and_path (file_path_str, &file_id, &file_path))
        return NULL;

    placeholder_aid = g_array_new (FALSE, FALSE, sizeof (guint8));

    input = qmi_message_uim_get_file_attributes_input_new ();
    qmi_message_uim_get_file_attributes_input_set_session (
        input,
        QMI_UIM_SESSION_TYPE_PRIMARY_GW_PROVISIONING,
        placeholder_aid, /* ignored */
        NULL);
    qmi_message_uim_get_file_attributes_input_set_file (
        input,
        file_id,
        file_path,
        NULL);
    g_array_unref (placeholder_aid);
    g_array_unref (file_path);
    return input;
}

#endif /* HAVE_QMI_MESSAGE_UIM_GET_FILE_ATTRIBUTES */

#if defined HAVE_QMI_MESSAGE_UIM_REFRESH_REGISTER || \
    defined HAVE_QMI_MESSAGE_UIM_REFRESH_REGISTER_ALL

static void
refresh_complete_ready (QmiClientUim *client,
                        GAsyncResult *res)
{
    QmiMessageUimRefreshCompleteOutput *output;
    GError *error = NULL;

    output = qmi_client_uim_refresh_complete_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: refresh complete failed: %s\n", error->message);
        g_error_free (error);
        return;
    }

    /* Ignore error, just log it as warning. In case we send complete message when
     * the modem does not expect it, we could get an error that is harmless.
     */
    if (!qmi_message_uim_refresh_complete_output_get_result (output, &error)) {
        g_warning ("refresh complete failed: %s\n", error->message);
        g_error_free (error);
    } else
        g_debug ("Refresh complete OK.");

    qmi_message_uim_refresh_complete_output_unref (output);
}

static void
refresh_complete (QmiClientUim *client,
                  gboolean success)
{
    QmiMessageUimRefreshCompleteInput *refresh_complete_input;
    GArray *placeholder_aid;

    placeholder_aid = g_array_new (FALSE, FALSE, sizeof (guint8));

    refresh_complete_input = qmi_message_uim_refresh_complete_input_new ();
    qmi_message_uim_refresh_complete_input_set_session (
        refresh_complete_input,
        QMI_UIM_SESSION_TYPE_CARD_SLOT_1,
        placeholder_aid, /* ignored */
        NULL);
    qmi_message_uim_refresh_complete_input_set_info (
        refresh_complete_input,
        success,
        NULL);

    qmi_client_uim_refresh_complete (
        ctx->client,
        refresh_complete_input,
        10,
        ctx->cancellable,
        (GAsyncReadyCallback) refresh_complete_ready,
        NULL);
    qmi_message_uim_refresh_complete_input_unref (refresh_complete_input);
    g_array_unref (placeholder_aid);
}

static void
refresh_received (QmiClientUim *client,
                  QmiIndicationUimRefreshOutput *output)
{
    QmiUimRefreshStage stage;
    QmiUimRefreshMode mode;
    GArray *files = NULL;
    GError *error = NULL;
    guint i, j;

    g_print ("[%s] Received refresh indication:\n",
             qmi_device_get_path_display (ctx->device));
    if (!qmi_indication_uim_refresh_output_get_event (
          output, &stage, &mode, NULL, NULL, &files, &error)) {
        g_printerr ("error: could not parse refresh ind: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }
    g_print ("  Refresh stage: %s\n",
             qmi_uim_refresh_stage_get_string (stage));
    g_print ("  Refresh mode: %s\n",
             qmi_uim_refresh_mode_get_string (mode));
    g_print ("  Files:\n");
    if (files && files->len > 0)
        for (i = 0; i < files->len; i++) {
            QmiIndicationUimRefreshOutputEventFilesElement *file;
            GArray *path;

            file = &g_array_index (files, QmiIndicationUimRefreshOutputEventFilesElement, i);
            g_print ("    0x%x; path:",
                     file->file_id);
            path = file->path;
            if (path && path->len >= 2)
                for (j = 0; j < path->len / 2; j++) {
                    guint16 path_component;
                    path_component = g_array_index (path, guint8, j * 2) |
                                    ((guint16)g_array_index (path, guint8, j * 2 + 1) << 8);
                    g_print (" 0x%x", path_component);
                }
            else
                g_print (" <none>");
            g_print ("\n");
        }
    else
        g_print ("    <none>\n");
    /* Send refresh complete message only in start stage and only if the
     * mode is something other than reset.
     */
    if (stage == QMI_UIM_REFRESH_STAGE_START && mode != QMI_UIM_REFRESH_MODE_RESET)
        refresh_complete (client, TRUE);
}

static void
refresh_cancelled (GCancellable *cancellable)
{
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_UIM_REFRESH_REGISTER
        * HAVE_QMI_MESSAGE_UIM_REFRESH_REGISTER_ALL */

#if defined HAVE_QMI_MESSAGE_UIM_REFRESH_REGISTER

static void
register_refresh_events_ready (QmiClientUim *client,
                               GAsyncResult *res)
{
    QmiMessageUimRefreshRegisterOutput *output;
    GError *error = NULL;

    output = qmi_client_uim_refresh_register_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_uim_refresh_register_output_get_result (output, &error)) {
        g_printerr ("error: could not register refresh file events: %s\n", error->message);
        qmi_message_uim_refresh_register_output_unref (output);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    g_debug ("Registered refresh file events...");
    ctx->refresh_indication_id =
        g_signal_connect (ctx->client,
                          "refresh",
                          G_CALLBACK (refresh_received),
                          NULL);

    /* User can use Ctrl+C to cancel the monitoring at any time */
    g_cancellable_connect (ctx->cancellable,
                           G_CALLBACK (refresh_cancelled),
                           NULL,
                           NULL);
    qmi_message_uim_refresh_register_output_unref (output);
}

static void
register_refresh_events (gchar **file_path_array)
{
    QmiMessageUimRefreshRegisterInput *refresh_input;
    GArray *placeholder_aid;
    GArray *file_list;
    QmiMessageUimRefreshRegisterInputInfoFilesElement file_element;
    guint i;

    file_list = g_array_new (FALSE, FALSE, sizeof (QmiMessageUimRefreshRegisterInputInfoFilesElement));
    while (*file_path_array) {
        memset (&file_element, 0, sizeof (file_element));
        if (!get_sim_file_id_and_path (*file_path_array, &file_element.file_id, &file_element.path))
            goto out;

        g_array_append_val (file_list, file_element);
        file_path_array++;
    }

    placeholder_aid = g_array_new (FALSE, FALSE, sizeof (guint8));
    refresh_input = qmi_message_uim_refresh_register_input_new ();
    qmi_message_uim_refresh_register_input_set_session (
        refresh_input,
        QMI_UIM_SESSION_TYPE_CARD_SLOT_1,
        placeholder_aid, /* ignored */
        NULL);
    qmi_message_uim_refresh_register_input_set_info (
        refresh_input,
        TRUE,
        FALSE,
        file_list,
        NULL);

    qmi_client_uim_refresh_register (
        ctx->client,
        refresh_input,
        10,
        ctx->cancellable,
        (GAsyncReadyCallback) register_refresh_events_ready,
        NULL);
    qmi_message_uim_refresh_register_input_unref (refresh_input);
    g_array_unref (placeholder_aid);

out:
    for (i = 0; i < file_list->len; i++) {
        QmiMessageUimRefreshRegisterInputInfoFilesElement *file;
        file = &g_array_index (file_list, QmiMessageUimRefreshRegisterInputInfoFilesElement, i);
        g_array_unref (file->path);
    }
    g_array_unref (file_list);
}

#endif /* HAVE_QMI_MESSAGE_UIM_REFRESH_REGISTER */

#if defined HAVE_QMI_MESSAGE_UIM_REFRESH_REGISTER_ALL

static void
register_refresh_all_events_ready (QmiClientUim *client,
                                   GAsyncResult *res)
{
    QmiMessageUimRefreshRegisterAllOutput *output;
    GError *error = NULL;

    output = qmi_client_uim_refresh_register_all_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_uim_refresh_register_all_output_get_result (output, &error)) {
        g_printerr ("error: could not register refresh file events: %s\n", error->message);
        qmi_message_uim_refresh_register_all_output_unref (output);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    g_debug ("Registered refresh all file events...");
    ctx->refresh_indication_id =
        g_signal_connect (ctx->client,
                          "refresh",
                          G_CALLBACK (refresh_received),
                          NULL);

    /* User can use Ctrl+C to cancel the monitoring at any time */
    g_cancellable_connect (ctx->cancellable,
                           G_CALLBACK (refresh_cancelled),
                           NULL,
                           NULL);
    qmi_message_uim_refresh_register_all_output_unref (output);
}

static void
register_refresh_all_events (void)
{
    QmiMessageUimRefreshRegisterAllInput *refresh_all_input;
    GArray *placeholder_aid;

    refresh_all_input = qmi_message_uim_refresh_register_all_input_new ();
    placeholder_aid = g_array_new (FALSE, FALSE, sizeof (guint8));
    qmi_message_uim_refresh_register_all_input_set_session (
        refresh_all_input,
        QMI_UIM_SESSION_TYPE_CARD_SLOT_1,
        placeholder_aid, /* ignored */
        NULL);

    qmi_message_uim_refresh_register_all_input_set_info (
        refresh_all_input,
        TRUE,
        NULL);
    qmi_client_uim_refresh_register_all (
        ctx->client,
        refresh_all_input,
        10,
        ctx->cancellable,
        (GAsyncReadyCallback) register_refresh_all_events_ready,
        NULL);
    qmi_message_uim_refresh_register_all_input_unref (refresh_all_input);
    g_array_unref (placeholder_aid);
}

#endif /* HAVE_QMI_MESSAGE_UIM_REFRESH_REGISTER_ALL */

#if defined HAVE_QMI_MESSAGE_UIM_GET_CONFIGURATION

static QmiMessageUimGetConfigurationInput *
get_configuration_input_create (void)
{
    QmiMessageUimGetConfigurationInput *input;

    input = qmi_message_uim_get_configuration_input_new ();

    qmi_message_uim_get_configuration_input_set_configuration_mask (
        input,
        QMI_UIM_CONFIGURATION_PERSONALIZATION_STATUS,
        NULL);

    return input;
}

static void
get_configuration_ready (QmiClientUim *client,
                         GAsyncResult *res)
{
    g_autoptr(QmiMessageUimGetConfigurationOutput) output = NULL;
    g_autoptr(GError)                              error = NULL;
    GArray                                        *elements = NULL;
    GArray                                        *other_slots = NULL;

    output = qmi_client_uim_get_configuration_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_uim_get_configuration_output_get_result (output, &error)) {
        g_printerr ("error: get configuration failed: %s\n", error->message);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("Configuration successfully retrieved\n");

    /* Other slots TLV contains info for slots > 1 */
    qmi_message_uim_get_configuration_output_get_personalization_status_other (output, &other_slots, NULL);

    if (qmi_message_uim_get_configuration_output_get_personalization_status (output, &elements, NULL)) {
        if (elements->len == 0)
            g_print ("Personalization features%s: all disabled\n",
                     other_slots ? " in slot 1" : "");
        else {
            QmiMessageUimGetConfigurationOutputPersonalizationStatusElement *element;
            guint i;

            g_print ("Personalization features%s:\n",
                     other_slots ? " in slot 1" : "");
            for (i = 0; i < elements->len; i++) {
                element = &g_array_index (elements,
                                          QmiMessageUimGetConfigurationOutputPersonalizationStatusElement,
                                          i);
                g_print ("\tPersonalization: %s\n"
                         "\t\tVerify left:  %u\n"
                         "\t\tUnblock left: %u\n",
                         qmi_uim_card_application_personalization_feature_get_string (element->feature),
                         element->verify_left,
                         element->unblock_left);
            }
        }
    }

    if (other_slots) {
        if (other_slots->len == 0)
            g_print ("Personalization features in other slots: all disabled\n");
        else {
            guint i_slot;

            for (i_slot = 0; i_slot < other_slots->len; i_slot++) {
                QmiMessageUimGetConfigurationOutputPersonalizationStatusOtherElement *slot_element;
                guint i;

                slot_element = &g_array_index (other_slots, QmiMessageUimGetConfigurationOutputPersonalizationStatusOtherElement, i_slot);
                if (!slot_element->slot)
                    continue;

                g_print ("Personalization features in slot %u:\n", i_slot + 2);
                for (i = 0; i < slot_element->slot->len; i++) {
                    QmiMessageUimGetConfigurationOutputPersonalizationStatusOtherElementSlotElement *element;

                    element = &g_array_index (slot_element->slot,
                                              QmiMessageUimGetConfigurationOutputPersonalizationStatusOtherElementSlotElement,
                                              i);
                    g_print ("\tPersonalization: %s\n"
                             "\t\tVerify left:  %u\n"
                             "\t\tUnblock left: %u\n",
                             qmi_uim_card_application_personalization_feature_get_string (element->feature),
                             element->verify_left,
                             element->unblock_left);
                }
            }
        }
    }

    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_UIM_GET_CONFIGURATION */

#if defined HAVE_QMI_MESSAGE_UIM_DEPERSONALIZATION

static QmiMessageUimDepersonalizationInput *
depersonalization_input_create (const gchar *str)
{
    g_auto(GStrv)                                split = NULL;
    QmiMessageUimDepersonalizationInput         *input = NULL;
    QmiUimCardApplicationPersonalizationFeature  feature;
    QmiUimDepersonalizationOperation             operation;
    const gchar                                 *control_key;
    guint                                        slot = 0;

    /* Prepare inputs.
     * Format of the string is:
     *    "[(feature),(operation),(control key)[,(slot number)]]"
     */
    split = g_strsplit (str, ",", -1);

    if (!split[0] || !qmicli_read_uim_card_application_personalization_feature_from_string (split[0], &feature)) {
        g_printerr ("error: invalid personalization feature\n");
        return NULL;
    }

    if (!split[1] || !qmicli_read_uim_depersonalization_operation_from_string (split[1], &operation)) {
        g_printerr ("error: invalid depersonalization operation\n");
        return NULL;
    }

    if (!split[2]) {
        g_printerr ("error: missing control key\n");
        return NULL;
    }
    control_key = split[2];

    if (g_strv_length (split) > 3) {
        if (!qmicli_read_uint_from_string (split[3], &slot) || (slot < 1) || (slot > 5)) {
            g_printerr ("error: invalid slot number\n");
            return NULL;
        }
    }

    input = qmi_message_uim_depersonalization_input_new ();
    qmi_message_uim_depersonalization_input_set_info (input, feature, operation, control_key, NULL);

    /* skip setting slot if not given by the user */
    if (slot > 0)
        qmi_message_uim_depersonalization_input_set_slot (input, slot, NULL);

    return input;
}

static void
depersonalization_ready (QmiClientUim *client,
                         GAsyncResult *res)
{
    g_autoptr(QmiMessageUimDepersonalizationOutput) output = NULL;
    g_autoptr(GError)                               error = NULL;
    guint8                                          unblock_left;
    guint8                                          verify_left;

    output = qmi_client_uim_depersonalization_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        operation_shutdown (FALSE);
        return;
    }

    if (qmi_message_uim_depersonalization_output_get_result (output, &error)) {
        g_print ("Modem was unlocked successfully\n");
        operation_shutdown (TRUE);
        return;
    }

    g_printerr ("error: depersonalization failed: %s\n", error->message);
    if (qmi_message_uim_depersonalization_output_get_retries_remaining (
            output,
            &verify_left,
            &unblock_left,
            NULL)) {
        g_printerr ("Retries left:\n"
                    "\tVerify: %u\n"
                    "\tUnblock: %u\n",
                    verify_left,
                    unblock_left);
    }

    operation_shutdown (FALSE);
}

#endif /* HAVE_QMI_MESSAGE_UIM_DEPERSONALIZATION */

#if defined HAVE_QMI_MESSAGE_UIM_REMOTE_UNLOCK

static QmiMessageUimRemoteUnlockInput *
remote_unlock_input_create (const gchar *simlock_data_str)
{
    QmiMessageUimRemoteUnlockInput *input;
    GArray                         *simlock_data = NULL;

    if (!qmicli_read_raw_data_from_string (simlock_data_str, &simlock_data))
        return NULL;

    input = qmi_message_uim_remote_unlock_input_new ();

    if (simlock_data->len <= 1024) {
        qmi_message_uim_remote_unlock_input_set_simlock_data (input,
                                                              simlock_data,
                                                              NULL);
    } else {
        qmi_message_uim_remote_unlock_input_set_simlock_extended_data (input,
                                                                       simlock_data,
                                                                       NULL);
    }

    g_array_unref (simlock_data);
    return input;
}

static void
remote_unlock_ready (QmiClientUim *client,
                     GAsyncResult *res)
{
    g_autoptr(QmiMessageUimRemoteUnlockOutput) output = NULL;
    g_autoptr(GError)                          error = NULL;

    output = qmi_client_uim_remote_unlock_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_uim_remote_unlock_output_get_result (output, &error)) {
        g_printerr ("error: remote unlock operation failed: %s\n", error->message);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("Remote unlock operation successfully completed\n");
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_UIM_REMOTE_UNLOCK */

#if defined HAVE_QMI_MESSAGE_UIM_OPEN_LOGICAL_CHANNEL

static QmiMessageUimOpenLogicalChannelInput *
open_logical_channel_input_create (const gchar *str)
{
    QmiMessageUimOpenLogicalChannelInput *input;
    g_auto(GStrv)                         split = NULL;
    guint                                 slot;
    g_autoptr(GArray)                     aid_data = NULL;

    /* Prepare inputs.
     * Format of the string is:
     *    "[(slot number),(aid)]"
     */
    split = g_strsplit (str, ",", -1);

    if (!split[0] || !qmicli_read_uint_from_string (split[0], &slot) || (slot > G_MAXUINT8)) {
        g_printerr ("error: invalid slot number\n");
        return NULL;
    }

    /* AID is optional */
    if (split[1]) {
        if (!qmicli_read_raw_data_from_string (split[1], &aid_data)) {
            g_printerr ("error: invalid AID data\n");
            return NULL;
        }
    }

    input = qmi_message_uim_open_logical_channel_input_new ();
    qmi_message_uim_open_logical_channel_input_set_slot (input, slot, NULL);
    if (aid_data)
        qmi_message_uim_open_logical_channel_input_set_aid (input, aid_data, NULL);

    return input;
}

static void
open_logical_channel_ready (QmiClientUim *client,
                            GAsyncResult *res)
{
    g_autoptr(QmiMessageUimOpenLogicalChannelOutput) output = NULL;
    g_autoptr(GError)                                error = NULL;
    guint8                                           channel_id;

    output = qmi_client_uim_open_logical_channel_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_uim_open_logical_channel_output_get_result (output, &error)) {
        g_printerr ("error: open logical channel operation failed: %s\n", error->message);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_uim_open_logical_channel_output_get_channel_id (output, &channel_id, &error)) {
        g_printerr ("error: get channel id operation failed: %s\n", error->message);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("Open Logical Channel operation successfully completed: %d\n", channel_id);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_UIM_OPEN_LOGICAL_CHANNEL */

#if defined HAVE_QMI_MESSAGE_UIM_LOGICAL_CHANNEL

static QmiMessageUimLogicalChannelInput *
close_logical_channel_input_create (const gchar *str)
{
    QmiMessageUimLogicalChannelInput *input;
    g_auto(GStrv)                     split = NULL;
    guint                             slot;
    guint                             channel_id;

    /* Prepare inputs.
     * Format of the string is:
     *    "[(slot number),(channel ID)]"
     */
    split = g_strsplit (str, ",", -1);

    if (!split[0] || !qmicli_read_uint_from_string (split[0], &slot) || (slot > G_MAXUINT8)) {
        g_printerr ("error: invalid slot number\n");
        return NULL;
    }

    if (!split[1] || !qmicli_read_uint_from_string (split[1], &channel_id) || (channel_id > G_MAXUINT8)) {
        g_printerr ("error: invalid channel ID\n");
        return NULL;
    }

    input = qmi_message_uim_logical_channel_input_new ();
    qmi_message_uim_logical_channel_input_set_slot (input, slot, NULL);
    qmi_message_uim_logical_channel_input_set_channel_id (input, channel_id, NULL);

    return input;
}

static void
close_logical_channel_ready (QmiClientUim *client,
                             GAsyncResult *res)
{
    g_autoptr(QmiMessageUimLogicalChannelOutput) output = NULL;
    g_autoptr(GError)                            error = NULL;

    output = qmi_client_uim_logical_channel_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_uim_logical_channel_output_get_result (output, &error)) {
        g_printerr ("error: close logical channel operation failed: %s\n", error->message);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("Close Logical Channel operation successfully completed\n");
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_UIM_LOGICAL_CHANNEL */

#if defined HAVE_QMI_MESSAGE_UIM_SEND_APDU

static QmiMessageUimSendApduInput *
send_apdu_input_create (const gchar *str)
{
    QmiMessageUimSendApduInput *input;
    g_auto(GStrv)               split = NULL;
    guint                       slot;
    guint                       channel_id;
    g_autoptr(GArray)           apdu_data = NULL;

    /* Prepare inputs.
     * Format of the string is:
     *    "[(slot number),(channel ID),(apdu)]"
     */
    split = g_strsplit (str, ",", -1);

    if (!split[0] || !qmicli_read_uint_from_string (split[0], &slot) || (slot > G_MAXUINT8)) {
        g_printerr ("error: invalid slot number\n");
        return NULL;
    }

    if (!split[1] || !qmicli_read_uint_from_string (split[1], &channel_id) || (channel_id > G_MAXUINT8)) {
        g_printerr ("error: invalid channel ID\n");
        return NULL;
    }

    if (!split[2] || !qmicli_read_raw_data_from_string (split[2], &apdu_data)) {
        g_printerr ("error: invalid APDU data\n");
        return NULL;
    }

    input = qmi_message_uim_send_apdu_input_new ();
    qmi_message_uim_send_apdu_input_set_slot (input, slot, NULL);
    qmi_message_uim_send_apdu_input_set_channel_id (input, channel_id, NULL);
    qmi_message_uim_send_apdu_input_set_apdu (input, apdu_data, NULL);

    return input;
}

static void
send_apdu_ready (QmiClientUim *client,
                 GAsyncResult *res)
{
    g_autoptr(QmiMessageUimSendApduOutput) output = NULL;
    g_autoptr(GError)                      error = NULL;
    GArray                                *apdu_res = NULL;
    gchar                                 *apdu_res_hex;

    output = qmi_client_uim_send_apdu_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_uim_send_apdu_output_get_result (output, &error)) {
        g_printerr ("error: send apdu operation failed: %s\n", error->message);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_uim_send_apdu_output_get_apdu_response (output, &apdu_res, &error)) {
        g_printerr ("error: get apdu response operation failed: %s\n", error->message);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("Send APDU operation successfully completed:");
    apdu_res_hex = qmi_common_str_hex (apdu_res->data, apdu_res->len, ':');
    g_print (" %s\n", apdu_res_hex);
    g_free (apdu_res_hex);

    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_UIM_SEND_APDU */

void
qmicli_uim_run (QmiDevice *device,
                QmiClientUim *client,
                GCancellable *cancellable)
{
    /* Initialize context */
    ctx = g_slice_new0 (Context);
    ctx->device = g_object_ref (device);
    ctx->client = g_object_ref (client);
    ctx->cancellable = g_object_ref (cancellable);

#if defined HAVE_QMI_MESSAGE_UIM_SET_PIN_PROTECTION
    if (set_pin_protection_str) {
        QmiMessageUimSetPinProtectionInput *input;

        g_debug ("Asynchronously setting PIN protection...");
        input = set_pin_protection_input_create (set_pin_protection_str);
        if (!input) {
            operation_shutdown (FALSE);
            return;
        }
        qmi_client_uim_set_pin_protection (ctx->client,
                                           input,
                                           10,
                                           ctx->cancellable,
                                           (GAsyncReadyCallback)set_pin_protection_ready,
                                           NULL);
        qmi_message_uim_set_pin_protection_input_unref (input);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_UIM_VERIFY_PIN
    if (verify_pin_str) {
        QmiMessageUimVerifyPinInput *input;

        g_debug ("Asynchronously verifying PIN...");
        input = verify_pin_input_create (verify_pin_str);
        if (!input) {
            operation_shutdown (FALSE);
            return;
        }
        qmi_client_uim_verify_pin (ctx->client,
                                   input,
                                   10,
                                   ctx->cancellable,
                                   (GAsyncReadyCallback)verify_pin_ready,
                                   NULL);
        qmi_message_uim_verify_pin_input_unref (input);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_UIM_UNBLOCK_PIN
    if (unblock_pin_str) {
        QmiMessageUimUnblockPinInput *input;

        g_debug ("Asynchronously unblocking PIN...");
        input = unblock_pin_input_create (unblock_pin_str);
        if (!input) {
            operation_shutdown (FALSE);
            return;
        }
        qmi_client_uim_unblock_pin (ctx->client,
                                    input,
                                    10,
                                    ctx->cancellable,
                                    (GAsyncReadyCallback)unblock_pin_ready,
                                    NULL);
        qmi_message_uim_unblock_pin_input_unref (input);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_UIM_CHANGE_PIN
    if (change_pin_str) {
        QmiMessageUimChangePinInput *input;

        g_debug ("Asynchronously changing PIN...");
        input = change_pin_input_create (change_pin_str);
        if (!input) {
            operation_shutdown (FALSE);
            return;
        }
        qmi_client_uim_change_pin (ctx->client,
                                   input,
                                   10,
                                   ctx->cancellable,
                                   (GAsyncReadyCallback)change_pin_ready,
                                   NULL);
        qmi_message_uim_change_pin_input_unref (input);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_UIM_READ_TRANSPARENT
    if (read_transparent_str) {
        QmiMessageUimReadTransparentInput *input;

        input = read_transparent_build_input (read_transparent_str);
        if (!input) {
            operation_shutdown (FALSE);
            return;
        }

        g_debug ("Asynchronously reading transparent file at '%s'...",
                 read_transparent_str);
        qmi_client_uim_read_transparent (ctx->client,
                                         input,
                                         10,
                                         ctx->cancellable,
                                         (GAsyncReadyCallback)read_transparent_ready,
                                         NULL);
        qmi_message_uim_read_transparent_input_unref (input);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_UIM_READ_RECORD
    if (read_record_str) {
        QmiMessageUimReadRecordInput *input;

        input = read_record_input_create (read_record_str);
        if (!input) {
            operation_shutdown (FALSE);
            return;
        }

        g_debug ("Asynchronously reading record file at '%s'...",
                 read_record_str);
        qmi_client_uim_read_record (ctx->client,
                                    input,
                                    10,
                                    ctx->cancellable,
                                    (GAsyncReadyCallback)read_record_ready,
                                    NULL);
        qmi_message_uim_read_record_input_unref (input);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_UIM_GET_FILE_ATTRIBUTES
    if (get_file_attributes_str) {
        QmiMessageUimGetFileAttributesInput *input;

        input = get_file_attributes_build_input (get_file_attributes_str);
        if (!input) {
            operation_shutdown (FALSE);
            return;
        }

        g_debug ("Asynchronously reading attributes of file '%s'...",
                 get_file_attributes_str);
        qmi_client_uim_get_file_attributes (ctx->client,
                                            input,
                                            10,
                                            ctx->cancellable,
                                            (GAsyncReadyCallback)get_file_attributes_ready,
                                            NULL);
        qmi_message_uim_get_file_attributes_input_unref (input);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_UIM_GET_CARD_STATUS
    if (get_card_status_flag) {
        g_debug ("Asynchronously getting card status...");
        qmi_client_uim_get_card_status (ctx->client,
                                        NULL,
                                        10,
                                        ctx->cancellable,
                                        (GAsyncReadyCallback)get_card_status_ready,
                                        NULL);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_UIM_GET_SUPPORTED_MESSAGES
    if (get_supported_messages_flag) {
        g_debug ("Asynchronously getting supported UIM messages...");
        qmi_client_uim_get_supported_messages (ctx->client,
                                               NULL,
                                               10,
                                               ctx->cancellable,
                                               (GAsyncReadyCallback)get_supported_messages_ready,
                                               NULL);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_UIM_POWER_ON_SIM
    if (sim_power_on_str) {
        QmiMessageUimPowerOnSimInput *input;

        g_debug ("Asynchronously power on SIM card");
        input = power_on_sim_input_create (sim_power_on_str);
        if (!input) {
            operation_shutdown (FALSE);
            return;
        }

        qmi_client_uim_power_on_sim (ctx->client,
                                     input,
                                     10,
                                     ctx->cancellable,
                                     (GAsyncReadyCallback)power_on_sim_ready,
                                     NULL);
        qmi_message_uim_power_on_sim_input_unref (input);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_UIM_POWER_OFF_SIM
    if (sim_power_off_str) {
        QmiMessageUimPowerOffSimInput *input;

        g_debug ("Asynchronously power off SIM card");
        input = power_off_sim_input_create (sim_power_off_str);
        if (!input) {
            operation_shutdown (FALSE);
            return;
        }

        qmi_client_uim_power_off_sim (ctx->client,
                                      input,
                                      10,
                                      ctx->cancellable,
                                      (GAsyncReadyCallback)power_off_sim_ready,
                                      NULL);
        qmi_message_uim_power_off_sim_input_unref (input);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_UIM_CHANGE_PROVISIONING_SESSION
    if (change_provisioning_session_str) {
        QmiMessageUimChangeProvisioningSessionInput *input;

        g_debug ("Asynchronously changing provisioning session");
        input = change_provisioning_session_input_create (change_provisioning_session_str);
        if (!input) {
            operation_shutdown (FALSE);
            return;
        }

        qmi_client_uim_change_provisioning_session (ctx->client,
                                                    input,
                                                    10,
                                                    ctx->cancellable,
                                                    (GAsyncReadyCallback)change_provisioning_session_ready,
                                                    NULL);
        qmi_message_uim_change_provisioning_session_input_unref (input);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_UIM_GET_SLOT_STATUS
    /* Request to get slot status? */
    if (get_slot_status_flag) {
        g_debug ("Asynchronously getting slot status...");
        qmi_client_uim_get_slot_status (ctx->client,
                                        NULL,
                                        10,
                                        ctx->cancellable,
                                        (GAsyncReadyCallback)get_slot_status_ready,
                                        NULL);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_UIM_SWITCH_SLOT && defined HAVE_QMI_MESSAGE_UIM_GET_SLOT_STATUS
    /* Request to change active slot? */
    if (switch_slot_str) {
        guint physical_slot;

        if (!qmicli_read_uint_from_string (switch_slot_str, &physical_slot) ||
            (physical_slot < 1) || (physical_slot > G_MAXUINT8)) {
            g_printerr ("error: invalid slot number\n");
            return;
        }

        g_debug ("Asynchronously switching active slot");

        qmi_client_uim_get_slot_status (ctx->client,
                                        NULL,
                                        10,
                                        ctx->cancellable,
                                        (GAsyncReadyCallback)get_active_logical_slot_ready,
                                        GUINT_TO_POINTER (physical_slot));
        return;
    }
#endif

#if defined HAVE_QMI_INDICATION_UIM_SLOT_STATUS
    /* Watch for slot status changes? */
    if (monitor_slot_status_flag) {
        g_debug ("Listening for slot status changes...");
        register_physical_slot_status_events ();
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_UIM_REFRESH_REGISTER
    if (monitor_refresh_file_array) {
        g_debug ("Listening for refresh events...");
        register_refresh_events (monitor_refresh_file_array);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_UIM_REFRESH_REGISTER_ALL
    if (monitor_refresh_all_flag) {
        g_debug ("Listening for all refresh events...");
        register_refresh_all_events ();
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_UIM_RESET
    /* Request to reset UIM service? */
    if (reset_flag) {
        g_debug ("Asynchronously resetting UIM service...");
        qmi_client_uim_reset (ctx->client,
                              NULL,
                              10,
                              ctx->cancellable,
                              (GAsyncReadyCallback)reset_ready,
                              NULL);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_UIM_GET_CONFIGURATION
    /* Request to get personalization status? */
    if (get_configuration_flag) {
        g_autoptr(QmiMessageUimGetConfigurationInput) input = NULL;

        g_debug ("Asynchronously getting UIM configuration...");
        input = get_configuration_input_create ();
        if (!input) {
            operation_shutdown (FALSE);
            return;
        }

        qmi_client_uim_get_configuration (ctx->client,
                                          input,
                                          10,
                                          ctx->cancellable,
                                          (GAsyncReadyCallback)get_configuration_ready,
                                          NULL);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_UIM_DEPERSONALIZATION
    /* Request to depersonalize the modem? */
    if (depersonalization_str) {
        g_autoptr(QmiMessageUimDepersonalizationInput) input = NULL;

        g_debug ("Asynchronously removing personalization...");
        input = depersonalization_input_create (depersonalization_str);
        if (!input) {
            operation_shutdown (FALSE);
            return;
        }

        qmi_client_uim_depersonalization (ctx->client,
                                          input,
                                          10,
                                          ctx->cancellable,
                                          (GAsyncReadyCallback)depersonalization_ready,
                                          NULL);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_UIM_REMOTE_UNLOCK
    /* Request to perform remote unlock operations? */
    if (remote_unlock_str) {
        g_autoptr(QmiMessageUimRemoteUnlockInput) input = NULL;

        g_debug ("Asynchronously updating SimLock data...");
        input = remote_unlock_input_create (remote_unlock_str);
        if (!input) {
            g_printerr ("error: couldn't parse the input string as a bytearray\n");
            operation_shutdown (FALSE);
            return;
        }

        qmi_client_uim_remote_unlock (ctx->client,
                                      input,
                                      10,
                                      ctx->cancellable,
                                      (GAsyncReadyCallback)remote_unlock_ready,
                                      NULL);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_UIM_OPEN_LOGICAL_CHANNEL
    /* Request to open logical channel? */
    if (open_logical_channel_str) {
        g_autoptr(QmiMessageUimOpenLogicalChannelInput) input = NULL;

        g_debug ("Asynchronously opening logical channel...");
        input = open_logical_channel_input_create (open_logical_channel_str);
        if (!input) {
            operation_shutdown (FALSE);
            return;
        }

        qmi_client_uim_open_logical_channel (ctx->client,
                                             input,
                                             10,
                                             ctx->cancellable,
                                             (GAsyncReadyCallback)open_logical_channel_ready,
                                             NULL);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_UIM_LOGICAL_CHANNEL
    /* Request to close logical channel? */
    if (close_logical_channel_str) {
        g_autoptr(QmiMessageUimLogicalChannelInput) input = NULL;

        g_debug ("Asynchronously closing logical channel...");
        input = close_logical_channel_input_create (close_logical_channel_str);
        if (!input) {
            operation_shutdown (FALSE);
            return;
        }

        qmi_client_uim_logical_channel (ctx->client,
                                        input,
                                        10,
                                        ctx->cancellable,
                                        (GAsyncReadyCallback)close_logical_channel_ready,
                                        NULL);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_UIM_SEND_APDU
    /* Request to send APDU? */
    if (send_apdu_str) {
        g_autoptr(QmiMessageUimSendApduInput) input = NULL;

        g_debug ("Asynchronously sending APDU...");
        input = send_apdu_input_create (send_apdu_str);
        if (!input) {
            operation_shutdown (FALSE);
            return;
        }

        qmi_client_uim_send_apdu (ctx->client,
                                  input,
                                  10,
                                  ctx->cancellable,
                                  (GAsyncReadyCallback)send_apdu_ready,
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

#endif /* HAVE_QMI_SERVICE_UIM */
