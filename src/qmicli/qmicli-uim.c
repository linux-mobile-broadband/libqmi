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
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <string.h>

#include <glib.h>
#include <gio/gio.h>

#include <libqmi-glib.h>

#include "qmicli.h"
#include "qmicli-helpers.h"

/* Context */
typedef struct {
    QmiDevice *device;
    QmiClientUim *client;
    GCancellable *cancellable;
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
static gboolean get_card_status_flag;
static gboolean get_supported_messages_flag;
static gboolean reset_flag;
static gboolean noop_flag;

static GOptionEntry entries[] = {
    { "uim-set-pin-protection", 0, 0, G_OPTION_ARG_STRING, &set_pin_protection_str,
      "Set PIN protection",
      "[(PIN1|PIN2|UPIN),(disable|enable),(current PIN)]"
    },
    { "uim-verify-pin", 0, 0, G_OPTION_ARG_STRING, &verify_pin_str,
      "Verify PIN",
      "[(PIN1|PIN2|UPIN),(current PIN)]",
    },
    { "uim-unblock-pin", 0, 0, G_OPTION_ARG_STRING, &unblock_pin_str,
      "Unblock PIN",
      "[(PIN1|PIN2|UPIN),(PUK),(new PIN)]",
    },
    { "uim-change-pin", 0, 0, G_OPTION_ARG_STRING, &change_pin_str,
      "Change PIN",
      "[(PIN1|PIN2|UPIN),(old PIN),(new PIN)]",
    },
    { "uim-read-transparent", 0, 0, G_OPTION_ARG_STRING, &read_transparent_str,
      "Read a transparent file given the file path",
      "[0xNNNN,0xNNNN,...]"
    },
    { "uim-get-file-attributes", 0, 0, G_OPTION_ARG_STRING, &get_file_attributes_str,
      "Get the attributes of a given file",
      "[0xNNNN,0xNNNN,...]"
    },
    { "uim-read-record", 0, 0, G_OPTION_ARG_STRING, &read_record_str,
      "Read a record from given file (allowed keys: record-number, record-length, file ([0xNNNN-0xNNNN,...])",
      "[\"key=value,...\"]"
    },
    { "uim-get-card-status", 0, 0, G_OPTION_ARG_NONE, &get_card_status_flag,
      "Get card status",
      NULL
    },
    { "uim-get-supported-messages", 0, 0, G_OPTION_ARG_NONE, &get_supported_messages_flag,
      "Get supported messages",
      NULL
    },
    { "uim-sim-power-on", 0, 0, G_OPTION_ARG_STRING, &sim_power_on_str,
      "Power on SIM card",
      "[(slot number)]"
    },
    { "uim-sim-power-off", 0, 0, G_OPTION_ARG_STRING, &sim_power_off_str,
      "Power off SIM card",
      "[(slot number)]"
    },
    { "uim-reset", 0, 0, G_OPTION_ARG_NONE, &reset_flag,
      "Reset the service state",
      NULL
    },
    { "uim-noop", 0, 0, G_OPTION_ARG_NONE, &noop_flag,
      "Just allocate or release a UIM client. Use with `--client-no-release-cid' and/or `--client-cid'",
      NULL
    },
    { NULL }
};

GOptionGroup *
qmicli_uim_get_option_group (void)
{
    GOptionGroup *group;

    group = g_option_group_new ("uim",
                                "UIM options",
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
                 get_card_status_flag +
                 get_supported_messages_flag +
                 reset_flag +
                 noop_flag);

    if (n_actions > 1) {
        g_printerr ("error: too many UIM actions requested\n");
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

static QmiMessageUimSetPinProtectionInput *
set_pin_protection_input_create (const gchar *str)
{
    QmiMessageUimSetPinProtectionInput *input = NULL;
    gchar **split;
    QmiUimPinId pin_id;
    gboolean enable_disable;
    gchar *current_pin;

    /* Prepare inputs.
     * Format of the string is:
     *    "[(PIN1|PIN2|UPIN),(disable|enable),(current PIN)]"
     */
    split = g_strsplit (str, ",", -1);
    if (qmicli_read_uim_pin_id_from_string (split[0], &pin_id) &&
        qmicli_read_enable_disable_from_string (split[1], &enable_disable) &&
        qmicli_read_non_empty_string (split[2], "current PIN", &current_pin)) {
        GError *error = NULL;
        GArray *dummy_aid;

        dummy_aid = g_array_new (FALSE, FALSE, sizeof (guint8));

        input = qmi_message_uim_set_pin_protection_input_new ();
        if (!qmi_message_uim_set_pin_protection_input_set_info (
                input,
                pin_id,
                enable_disable,
                current_pin,
                &error) ||
            !qmi_message_uim_set_pin_protection_input_set_session (
                input,
                QMI_UIM_SESSION_TYPE_CARD_SLOT_1,
                dummy_aid, /* ignored */
                &error)) {
            g_printerr ("error: couldn't create input data bundle: '%s'\n",
                        error->message);
            g_error_free (error);
            qmi_message_uim_set_pin_protection_input_unref (input);
            input = NULL;
        }
        g_array_unref (dummy_aid);
    }
    g_strfreev (split);

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

static QmiMessageUimVerifyPinInput *
verify_pin_input_create (const gchar *str)
{
    QmiMessageUimVerifyPinInput *input = NULL;
    gchar **split;
    QmiUimPinId pin_id;
    gchar *current_pin;

    /* Prepare inputs.
     * Format of the string is:
     *    "[(PIN1|PIN2),(current PIN)]"
     */
    split = g_strsplit (str, ",", -1);
    if (qmicli_read_uim_pin_id_from_string (split[0], &pin_id) &&
        qmicli_read_non_empty_string (split[1], "current PIN", &current_pin)) {
        GError *error = NULL;
        GArray *dummy_aid;

        dummy_aid = g_array_new (FALSE, FALSE, sizeof (guint8));

        input = qmi_message_uim_verify_pin_input_new ();
        if (!qmi_message_uim_verify_pin_input_set_info (
                input,
                pin_id,
                current_pin,
                &error) ||
            !qmi_message_uim_verify_pin_input_set_session (
                input,
                QMI_UIM_SESSION_TYPE_CARD_SLOT_1,
                dummy_aid, /* ignored */
                &error)) {
            g_printerr ("error: couldn't create input data bundle: '%s'\n",
                        error->message);
            g_error_free (error);
            qmi_message_uim_verify_pin_input_unref (input);
            input = NULL;
        }
        g_array_unref (dummy_aid);
    }
    g_strfreev (split);

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

static QmiMessageUimUnblockPinInput *
unblock_pin_input_create (const gchar *str)
{
    QmiMessageUimUnblockPinInput *input = NULL;
    gchar **split;
    QmiUimPinId pin_id;
    gchar *puk;
    gchar *new_pin;

    /* Prepare inputs.
     * Format of the string is:
     *    "[(PIN|PIN2),(PUK),(new PIN)]"
     */
    split = g_strsplit (str, ",", -1);
    if (qmicli_read_uim_pin_id_from_string (split[0], &pin_id) &&
        qmicli_read_non_empty_string (split[1], "PUK", &puk) &&
        qmicli_read_non_empty_string (split[2], "new PIN", &new_pin)) {
        GError *error = NULL;
        GArray *dummy_aid;

        dummy_aid = g_array_new (FALSE, FALSE, sizeof (guint8));

        input = qmi_message_uim_unblock_pin_input_new ();
        if (!qmi_message_uim_unblock_pin_input_set_info (
                input,
                pin_id,
                puk,
                new_pin,
                &error) ||
            !qmi_message_uim_unblock_pin_input_set_session (
                input,
                QMI_UIM_SESSION_TYPE_CARD_SLOT_1,
                dummy_aid, /* ignored */
                &error)) {
            g_printerr ("error: couldn't create input data bundle: '%s'\n",
                        error->message);
            g_error_free (error);
            qmi_message_uim_unblock_pin_input_unref (input);
            input = NULL;
        }
        g_array_unref (dummy_aid);
    }
    g_strfreev (split);

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

static QmiMessageUimChangePinInput *
change_pin_input_create (const gchar *str)
{
    QmiMessageUimChangePinInput *input = NULL;
    gchar **split;
    QmiUimPinId pin_id;
    gchar *old_pin;
    gchar *new_pin;

    /* Prepare inputs.
     * Format of the string is:
     *    "[(PIN1|PIN2),(old PIN),(new PIN)]"
     */
    split = g_strsplit (str, ",", -1);
    if (qmicli_read_uim_pin_id_from_string (split[0], &pin_id) &&
        qmicli_read_non_empty_string (split[1], "old PIN", &old_pin) &&
        qmicli_read_non_empty_string (split[2], "new PIN", &new_pin)) {
        GError *error = NULL;
        GArray *dummy_aid;

        dummy_aid = g_array_new (FALSE, FALSE, sizeof (guint8));

        input = qmi_message_uim_change_pin_input_new ();
        if (!qmi_message_uim_change_pin_input_set_info (
                input,
                pin_id,
                old_pin,
                new_pin,
                &error) ||
            !qmi_message_uim_change_pin_input_set_session (
                input,
                QMI_UIM_SESSION_TYPE_CARD_SLOT_1,
                dummy_aid, /* ignored */
                &error)) {
            g_printerr ("error: couldn't create input data bundle: '%s'\n",
                        error->message);
            g_error_free (error);
            qmi_message_uim_change_pin_input_unref (input);
            input = NULL;
        }
        g_array_unref (dummy_aid);
    }
    g_strfreev (split);

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

static QmiMessageUimPowerOnSimInput *
power_on_sim_input_create (const gchar *slot_str)
{
    QmiMessageUimPowerOnSimInput *input;
    guint                         slot;
    GError                       *error = NULL;

    input = qmi_message_uim_power_on_sim_input_new ();

    if (!qmicli_read_uint_from_string (sim_power_on_str, &slot) || (slot > G_MAXUINT8)) {
        g_printerr ("error: invalid slot number\n");
        return NULL;
    }

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

static QmiMessageUimPowerOffSimInput *
power_off_sim_input_create (const gchar *slot_str)
{
    QmiMessageUimPowerOffSimInput *input;
    guint                         slot;
    GError                       *error = NULL;

    input = qmi_message_uim_power_off_sim_input_new ();

    if (!qmicli_read_uint_from_string (sim_power_off_str, &slot) || (slot > G_MAXUINT8)) {
        g_printerr ("error: invalid slot number\n");
        return NULL;
    }

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

static gboolean
noop_cb (gpointer unused)
{
    operation_shutdown (TRUE);
    return FALSE;
}

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

#undef VALIDATE_UNKNOWN
#define VALIDATE_UNKNOWN(str) (str ? str : "unknown")

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
            g_print ("\tCard state: '%s: %s (%u)\n",
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
            QmiMessageUimGetCardStatusOutputCardStatusCardsElementApplicationsElement *app;
            gchar *str;

            app = &g_array_index (card->applications, QmiMessageUimGetCardStatusOutputCardStatusCardsElementApplicationsElement, j);

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
                         qmi_uim_card_application_personalization_feature_get_string (app->personalization_feature),
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

static gboolean
get_sim_file_id_and_path (const gchar *file_path_str,
                          guint16 *file_id,
                          GArray **file_path)
{
    return get_sim_file_id_and_path_with_separator (file_path_str, file_id, file_path, ",");
}

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
    GArray *dummy_aid;

    if (!get_sim_file_id_and_path (file_path_str, &file_id, &file_path))
        return NULL;

    dummy_aid = g_array_new (FALSE, FALSE, sizeof (guint8));

    input = qmi_message_uim_read_transparent_input_new ();
    qmi_message_uim_read_transparent_input_set_session (
        input,
        QMI_UIM_SESSION_TYPE_PRIMARY_GW_PROVISIONING,
        dummy_aid, /* ignored */
        NULL);
    qmi_message_uim_read_transparent_input_set_file (
        input,
        file_id,
        file_path,
        NULL);
    qmi_message_uim_read_transparent_input_set_read_information (input, 0, 0, NULL);
    g_array_unref (file_path);
    g_array_unref (dummy_aid);
    return input;
}

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
        props->file = strdup(value);
        return TRUE;
    }

    if (g_ascii_strcasecmp (key, "record-number") == 0) {
        props->record_number = (guint16) atoi(value);
        return TRUE;
    }

    if (g_ascii_strcasecmp (key, "record-length") == 0) {
        props->record_length = (guint16) atoi(value);
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
    GArray *dummy_aid;

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

    dummy_aid = g_array_new (FALSE, FALSE, sizeof (guint8));

    input = qmi_message_uim_read_record_input_new ();

    qmi_message_uim_read_record_input_set_session (
        input,
        QMI_UIM_SESSION_TYPE_PRIMARY_GW_PROVISIONING,
        dummy_aid, /* ignored */
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

    g_array_unref (dummy_aid);

out:
    free (props.file);
    g_array_unref (file_path);
    return input;
}

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
        gchar *str;

        g_print ("File attributes:\n");
        g_print ("\tFile size: %u\n", (guint)file_size);
        g_print ("\tFile ID: %u\n", (guint)file_id);
        g_print ("\tFile type: %s\n", qmi_uim_file_type_get_string (file_type));
        g_print ("\tRecord size: %u\n", (guint)record_size);
        g_print ("\tRecord count: %u\n", (guint)record_count);

        str = qmi_uim_security_attribute_build_string_from_mask (read_security_attributes);
        g_print ("\tRead security attributes: (%s) %s\n",
                 qmi_uim_security_attribute_logic_get_string (read_security_attributes_logic),
                 str);
        g_free (str);

        str = qmi_uim_security_attribute_build_string_from_mask (write_security_attributes);
        g_print ("\tWrite security attributes: (%s) %s\n",
                 qmi_uim_security_attribute_logic_get_string (write_security_attributes_logic),
                 str);
        g_free (str);

        str = qmi_uim_security_attribute_build_string_from_mask (increase_security_attributes);
        g_print ("\tIncrease security attributes: (%s) %s\n",
                 qmi_uim_security_attribute_logic_get_string (increase_security_attributes_logic),
                 str);
        g_free (str);

        str = qmi_uim_security_attribute_build_string_from_mask (deactivate_security_attributes);
        g_print ("\tDeactivate security attributes: (%s) %s\n",
                 qmi_uim_security_attribute_logic_get_string (deactivate_security_attributes_logic),
                 str);
        g_free (str);

        str = qmi_uim_security_attribute_build_string_from_mask (activate_security_attributes);
        g_print ("\tActivate security attributes: (%s) %s\n",
                 qmi_uim_security_attribute_logic_get_string (activate_security_attributes_logic),
                 str);
        g_free (str);

        str = qmicli_get_raw_data_printable (raw, 80, "\t");
        g_print ("\tRaw: %s\n", str);
        g_free (str);
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
    GArray *dummy_aid;

    if (!get_sim_file_id_and_path (file_path_str, &file_id, &file_path))
        return NULL;

    dummy_aid = g_array_new (FALSE, FALSE, sizeof (guint8));

    input = qmi_message_uim_get_file_attributes_input_new ();
    qmi_message_uim_get_file_attributes_input_set_session (
        input,
        QMI_UIM_SESSION_TYPE_PRIMARY_GW_PROVISIONING,
        dummy_aid, /* ignored */
        NULL);
    qmi_message_uim_get_file_attributes_input_set_file (
        input,
        file_id,
        file_path,
        NULL);
    g_array_unref (dummy_aid);
    g_array_unref (file_path);
    return input;
}

void
qmicli_uim_run (QmiDevice *device,
                QmiClientUim *client,
                GCancellable *cancellable)
{
    /* Initialize context */
    ctx = g_slice_new (Context);
    ctx->device = g_object_ref (device);
    ctx->client = g_object_ref (client);
    ctx->cancellable = g_object_ref (cancellable);

    /* Set PIN protection */
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

    /* Request to verify PIN? */
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

    /* Request to unblock PIN? */
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

    /* Request to change the PIN? */
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

    /* Request to read a transparent file? */
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

    /* Request to read a transparent file? */
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

    /* Request to get file attributes? */
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

    /* Request to read card status? */
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

    /* Request to list supported messages? */
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

    /* Request to power on SIM card? */
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

    /* Request to power off SIM card? */
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

    /* Just client allocate/release? */
    if (noop_flag) {
        g_idle_add (noop_cb, NULL);
        return;
    }

    g_warn_if_reached ();
}
