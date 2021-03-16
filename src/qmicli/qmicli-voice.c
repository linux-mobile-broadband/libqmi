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
 * Copyright (C) 2014-2017 Aleksander Morgado <aleksander@aleksander.es>
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

#if defined HAVE_QMI_SERVICE_VOICE

#undef VALIDATE_MASK_NONE
#define VALIDATE_MASK_NONE(str) (str ? str : "none")

/* Context */
typedef struct {
    QmiDevice *device;
    QmiClientVoice *client;
    GCancellable *cancellable;
} Context;
static Context *ctx;

/* Options */
static gboolean get_config_flag;
static gboolean get_supported_messages_flag;
static gboolean noop_flag;

static GOptionEntry entries[] = {
#if defined HAVE_QMI_MESSAGE_VOICE_GET_CONFIG
    { "voice-get-config", 0, 0, G_OPTION_ARG_NONE, &get_config_flag,
      "Get Voice service configuration",
      NULL
    },
#endif
#if defined HAVE_QMI_MESSAGE_VOICE_GET_SUPPORTED_MESSAGES
    { "voice-get-supported-messages", 0, 0, G_OPTION_ARG_NONE, &get_supported_messages_flag,
      "Get supported messages",
      NULL
    },
#endif
    { "voice-noop", 0, 0, G_OPTION_ARG_NONE, &noop_flag,
      "Just allocate or release a VOICE client. Use with `--client-no-release-cid' and/or `--client-cid'",
      NULL
    },
    { NULL }
};

GOptionGroup *
qmicli_voice_get_option_group (void)
{
    GOptionGroup *group;

    group = g_option_group_new ("voice",
                                "VOICE options:",
                                "Show Voice Service options",
                                NULL,
                                NULL);
    g_option_group_add_entries (group, entries);

    return group;
}

gboolean
qmicli_voice_options_enabled (void)
{
    static guint n_actions = 0;
    static gboolean checked = FALSE;

    if (checked)
        return !!n_actions;

    n_actions = (get_config_flag +
                 get_supported_messages_flag +
                 noop_flag);

    if (n_actions > 1) {
        g_printerr ("error: too many VOICE actions requested\n");
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

#if defined HAVE_QMI_MESSAGE_VOICE_GET_CONFIG

static void
get_config_ready (QmiClientVoice *client,
                  GAsyncResult *res)
{
    QmiMessageVoiceGetConfigOutput *output;
    GError *error = NULL;
    QmiVoiceDomain current_voice_domain_preference;
    QmiVoicePrivacy current_voice_privacy_preference;
    gboolean current_amr_status_gsm;
    QmiVoiceWcdmaAmrStatus current_amr_status_wcdma;
    guint8 current_preferred_voice_so_nam_id;
    gboolean current_preferred_voice_so_evrc_capability;
    QmiVoiceServiceOption current_preferred_voice_so_home_page_voice_service_option;
    QmiVoiceServiceOption current_preferred_voice_so_home_origination_voice_service_option;
    QmiVoiceServiceOption current_preferred_voice_so_roaming_origination_voice_service_option;
    QmiVoiceTtyMode current_tty_mode;
    guint8 roam_timer_count_nam_id;
    guint32 roam_timer_count_roam_timer;
    guint8 air_timer_count_nam_id;
    guint32 air_timer_count_air_timer;
    gboolean auto_answer_status;

    output = qmi_client_voice_get_config_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_voice_get_config_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get Voice configuration: %s\n", error->message);
        g_error_free (error);
        qmi_message_voice_get_config_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] Successfully retrieved Voice configuration:\n",
             qmi_device_get_path_display (ctx->device));

    if (qmi_message_voice_get_config_output_get_auto_answer_status (
            output,
            &auto_answer_status,
            NULL))
        g_print ("Auto Answer Status: '%s'",
                 auto_answer_status ? "enabled" : "disabled");

    if (qmi_message_voice_get_config_output_get_air_timer_count (
            output,
            &air_timer_count_nam_id,
            &air_timer_count_air_timer,
            NULL))
        g_print ("Air Timer Count:\n"
                 "\tNAM ID: '%u'"
                 "\tTimer: '%u'",
                 air_timer_count_nam_id,
                 air_timer_count_air_timer);

    if (qmi_message_voice_get_config_output_get_roam_timer_count (
            output,
            &roam_timer_count_nam_id,
            &roam_timer_count_roam_timer,
            NULL))
        g_print ("Roam Timer Count:\n"
                 "\tNAM ID: '%u'"
                 "\tTimer: '%u'",
                 roam_timer_count_nam_id,
                 roam_timer_count_roam_timer);

    if (qmi_message_voice_get_config_output_get_current_tty_mode (
            output,
            &current_tty_mode,
            NULL))
        g_print ("Current TTY mode: '%s'",
                 qmi_voice_tty_mode_get_string (current_tty_mode));

    if (qmi_message_voice_get_config_output_get_current_preferred_voice_so (
            output,
            &current_preferred_voice_so_nam_id,
            &current_preferred_voice_so_evrc_capability,
            &current_preferred_voice_so_home_page_voice_service_option,
            &current_preferred_voice_so_home_origination_voice_service_option,
            &current_preferred_voice_so_roaming_origination_voice_service_option,
            NULL)) {
        g_print ("Current Preferred Voice SO:\n"
                 "\tNAM ID: '%u'\n"
                 "\tEVRC capability: '%s'\n"
                 "\tHome Page Voice SO: '%s'\n"
                 "\tHome Origination Voice SO: '%s'\n"
                 "\tRoaming Origination Voice SO: '%s'\n",
                 current_preferred_voice_so_nam_id,
                 current_preferred_voice_so_evrc_capability ? "enabled" : "disabled",
                 qmi_voice_service_option_get_string (current_preferred_voice_so_home_page_voice_service_option),
                 qmi_voice_service_option_get_string (current_preferred_voice_so_home_origination_voice_service_option),
                 qmi_voice_service_option_get_string (current_preferred_voice_so_roaming_origination_voice_service_option));
    }

    if (qmi_message_voice_get_config_output_get_current_amr_status (
            output,
            &current_amr_status_gsm, &current_amr_status_wcdma,
            NULL)) {
        gchar *current_amr_status_wcdma_str;

        current_amr_status_wcdma_str = qmi_voice_wcdma_amr_status_build_string_from_mask (current_amr_status_wcdma);
        g_print ("AMR Status:\n"
                 "\tGSM: '%s'\n"
                 "\tWCDMA: '%s' (0x%04X)\n",
                 current_amr_status_gsm ? "enabled" : "disabled",
                 VALIDATE_MASK_NONE (current_amr_status_wcdma_str),
                 current_amr_status_wcdma);
        g_free (current_amr_status_wcdma_str);
    }

    if (qmi_message_voice_get_config_output_get_current_voice_privacy_preference (
            output,
            &current_voice_privacy_preference,
            NULL))
        g_print ("Current Voice Privacy Preference: '%s'\n",
                 qmi_voice_privacy_get_string (current_voice_privacy_preference));

    if (qmi_message_voice_get_config_output_get_current_voice_domain_preference (
            output,
            &current_voice_domain_preference,
            NULL))
        g_print ("Current Voice Domain Preference: '%s'\n",
                 qmi_voice_domain_get_string (current_voice_domain_preference));

    qmi_message_voice_get_config_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_VOICE_GET_CONFIG */

#if defined HAVE_QMI_MESSAGE_VOICE_GET_SUPPORTED_MESSAGES

static void
get_supported_messages_ready (QmiClientVoice *client,
                              GAsyncResult *res)
{
    QmiMessageVoiceGetSupportedMessagesOutput *output;
    GError *error = NULL;
    GArray *bytearray = NULL;
    gchar *str;

    output = qmi_client_voice_get_supported_messages_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_voice_get_supported_messages_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get supported VOICE messages: %s\n", error->message);
        g_error_free (error);
        qmi_message_voice_get_supported_messages_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] Successfully got supported VOICE messages:\n",
             qmi_device_get_path_display (ctx->device));

    qmi_message_voice_get_supported_messages_output_get_list (output, &bytearray, NULL);
    str = qmicli_get_supported_messages_list (bytearray ? (const guint8 *)bytearray->data : NULL,
                                              bytearray ? bytearray->len : 0);
    g_print ("%s", str);
    g_free (str);

    qmi_message_voice_get_supported_messages_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_VOICE_GET_SUPPORTED_MESSAGES */

static gboolean
noop_cb (gpointer unused)
{
    operation_shutdown (TRUE);
    return FALSE;
}

void
qmicli_voice_run (QmiDevice *device,
                  QmiClientVoice *client,
                  GCancellable *cancellable)
{
    /* Initialize context */
    ctx = g_slice_new (Context);
    ctx->device = g_object_ref (device);
    ctx->client = g_object_ref (client);
    ctx->cancellable = g_object_ref (cancellable);

#if defined HAVE_QMI_MESSAGE_VOICE_GET_CONFIG
    if (get_config_flag) {
        QmiMessageVoiceGetConfigInput *input;

        input = qmi_message_voice_get_config_input_new ();
        qmi_message_voice_get_config_input_set_auto_answer (input, TRUE, NULL);
        qmi_message_voice_get_config_input_set_air_timer (input, TRUE, NULL);
        qmi_message_voice_get_config_input_set_roam_timer (input, TRUE, NULL);
        qmi_message_voice_get_config_input_set_tty_mode (input, TRUE, NULL);
        qmi_message_voice_get_config_input_set_preferred_voice_service_option (input, TRUE, NULL);
        qmi_message_voice_get_config_input_set_amr_status (input, TRUE, NULL);
        qmi_message_voice_get_config_input_set_preferred_voice_privacy (input, TRUE, NULL);
        if (qmi_client_check_version (QMI_CLIENT (ctx->client), 2, 3))
            qmi_message_voice_get_config_input_set_nam_index (input, TRUE, NULL);
        if (qmi_client_check_version (QMI_CLIENT (ctx->client), 2, 9))
            qmi_message_voice_get_config_input_set_voice_domain_preference (input, TRUE, NULL);

        g_debug ("Asynchronously getting voice configuration...");
        qmi_client_voice_get_config (ctx->client,
                                     input,
                                     10,
                                     ctx->cancellable,
                                     (GAsyncReadyCallback)get_config_ready,
                                     NULL);
        qmi_message_voice_get_config_input_unref (input);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_VOICE_GET_SUPPORTED_MESSAGES
    if (get_supported_messages_flag) {
        g_debug ("Asynchronously getting supported voice messages...");
        qmi_client_voice_get_supported_messages (ctx->client,
                                                 NULL,
                                                 10,
                                                 ctx->cancellable,
                                                 (GAsyncReadyCallback)get_supported_messages_ready,
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

#endif /* HAVE_QMI_SERVICE_VOICE */
