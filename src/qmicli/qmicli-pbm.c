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
 * Copyright (C) 2013-2017 Aleksander Morgado <aleksander@aleksander.es>
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

#if defined HAVE_QMI_SERVICE_PBM

#undef VALIDATE_MASK_NONE
#define VALIDATE_MASK_NONE(str) (str ? str : "none")

/* Context */
typedef struct {
    QmiDevice *device;
    QmiClientPbm *client;
    GCancellable *cancellable;
} Context;
static Context *ctx;

/* Options */
static gboolean get_all_capabilities_flag;
static gboolean noop_flag;

static GOptionEntry entries[] = {
#if defined HAVE_QMI_MESSAGE_PBM_GET_ALL_CAPABILITIES
    { "pbm-get-all-capabilities", 0, 0, G_OPTION_ARG_NONE, &get_all_capabilities_flag,
      "Get all phonebook capabilities",
      NULL
    },
#endif
    { "pbm-noop", 0, 0, G_OPTION_ARG_NONE, &noop_flag,
      "Just allocate or release a PBM client. Use with `--client-no-release-cid' and/or `--client-cid'",
      NULL
    },
    { NULL }
};

GOptionGroup *
qmicli_pbm_get_option_group (void)
{
    GOptionGroup *group;

    group = g_option_group_new ("pbm",
                                "PBM options:",
                                "Show Phonebook Management options",
                                NULL,
                                NULL);
    g_option_group_add_entries (group, entries);

    return group;
}

gboolean
qmicli_pbm_options_enabled (void)
{
    static guint n_actions = 0;
    static gboolean checked = FALSE;

    if (checked)
        return !!n_actions;

    n_actions = (get_all_capabilities_flag +
                 noop_flag);

    if (n_actions > 1) {
        g_printerr ("error: too many PBM actions requested\n");
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

#if defined HAVE_QMI_MESSAGE_PBM_GET_ALL_CAPABILITIES

static void
get_all_capabilities_ready (QmiClientPbm *client,
                            GAsyncResult *res)
{
    GError *error = NULL;
    QmiMessagePbmGetAllCapabilitiesOutput *output;
    GArray *capability_basic_information = NULL;
    GArray *group_capability = NULL;
    GArray *additional_number_capability = NULL;
    GArray *email_capability = NULL;
    GArray *second_name_capability = NULL;
    GArray *hidden_records_capability = NULL;
    GArray *grouping_information_alpha_string_capability = NULL;
    GArray *additional_number_alpha_string_capability = NULL;
    guint i, j;

    output = qmi_client_pbm_get_all_capabilities_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n",
                    error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_pbm_get_all_capabilities_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get capabilities: %s\n", error->message);
        g_error_free (error);
        qmi_message_pbm_get_all_capabilities_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    qmi_message_pbm_get_all_capabilities_output_get_capability_basic_information (output, &capability_basic_information, NULL);
    qmi_message_pbm_get_all_capabilities_output_get_group_capability (output, &group_capability, NULL);
    qmi_message_pbm_get_all_capabilities_output_get_additional_number_capability (output, &additional_number_capability, NULL);
    qmi_message_pbm_get_all_capabilities_output_get_email_capability (output, &email_capability, NULL);
    qmi_message_pbm_get_all_capabilities_output_get_second_name_capability (output, &second_name_capability, NULL);
    qmi_message_pbm_get_all_capabilities_output_get_hidden_records_capability (output, &hidden_records_capability, NULL);
    qmi_message_pbm_get_all_capabilities_output_get_grouping_information_alpha_string_capability (output, &grouping_information_alpha_string_capability, NULL);
    qmi_message_pbm_get_all_capabilities_output_get_additional_number_alpha_string_capability (output, &additional_number_alpha_string_capability, NULL);

    g_print ("[%s] Phonebook capabilities:%s\n",
             qmi_device_get_path_display (ctx->device),
             (capability_basic_information ||
              group_capability ||
              additional_number_capability ||
              email_capability ||
              second_name_capability ||
              hidden_records_capability ||
              grouping_information_alpha_string_capability ||
              additional_number_alpha_string_capability) ? "" : " none");

    if (capability_basic_information) {
        g_print ("Capability basic information:\n");
        for (i = 0; i < capability_basic_information->len; i++) {
            QmiMessagePbmGetAllCapabilitiesOutputCapabilityBasicInformationElement *session;

            session = &g_array_index (capability_basic_information,
                                      QmiMessagePbmGetAllCapabilitiesOutputCapabilityBasicInformationElement,
                                      i);
            g_print ("\t[%s]:\n", qmi_pbm_session_type_get_string (session->session_type));
            for (j = 0; j < session->phonebooks->len; j++) {
                QmiMessagePbmGetAllCapabilitiesOutputCapabilityBasicInformationElementPhonebooksElement *phonebook;
                g_autofree gchar *phonebook_type_str = NULL;

                phonebook = &g_array_index (session->phonebooks,
                                            QmiMessagePbmGetAllCapabilitiesOutputCapabilityBasicInformationElementPhonebooksElement,
                                            j);
                phonebook_type_str = qmi_pbm_phonebook_type_build_string_from_mask (phonebook->phonebook_type);
                g_print ("\t\t[%s]:\n", VALIDATE_MASK_NONE (phonebook_type_str));
                g_print ("\t\t\tUsed records: %" G_GUINT16_FORMAT "\n", phonebook->used_records);
                g_print ("\t\t\tMaximum records: %" G_GUINT16_FORMAT "\n", phonebook->maximum_records);
                g_print ("\t\t\tMaximum number length: %u\n", phonebook->maximum_number_length);
                g_print ("\t\t\tMaximum name length: %u\n", phonebook->maximum_name_length);
            }
        }
    }

    if (group_capability) {
        g_print ("Group capability:\n");
        for (i = 0; i < group_capability->len; i++) {
            QmiMessagePbmGetAllCapabilitiesOutputGroupCapabilityElement *session;

            session = &g_array_index (group_capability,
                                      QmiMessagePbmGetAllCapabilitiesOutputGroupCapabilityElement,
                                      i);
            g_print ("\t[%s]:\n", qmi_pbm_session_type_get_string (session->session_type));
            g_print ("\t\tMaximum groups: %u\n", session->maximum_groups);
            g_print ("\t\tMaximum group tag length: %u\n", session->maximum_group_tag_length);
        }
    }

    if (additional_number_capability) {
        g_print ("Additional number capability:\n");
        for (i = 0; i < additional_number_capability->len; i++) {
            QmiMessagePbmGetAllCapabilitiesOutputAdditionalNumberCapabilityElement *session;

            session = &g_array_index (additional_number_capability,
                                      QmiMessagePbmGetAllCapabilitiesOutputAdditionalNumberCapabilityElement,
                                      i);
            g_print ("\t[%s]:\n", qmi_pbm_session_type_get_string (session->session_type));
            g_print ("\t\tMaximum additional numbers: %u\n", session->maximum_additional_numbers);
            g_print ("\t\tMaximum additional number length: %u\n", session->maximum_additional_number_length);
            g_print ("\t\tMaximum additional number tag length: %u\n", session->maximum_additional_number_tag_length);
        }
    }

    if (email_capability) {
        g_print ("Email capability:\n");
        for (i = 0; i < email_capability->len; i++) {
            QmiMessagePbmGetAllCapabilitiesOutputEmailCapabilityElement *session;

            session = &g_array_index (email_capability,
                                      QmiMessagePbmGetAllCapabilitiesOutputEmailCapabilityElement,
                                      i);
            g_print ("\t[%s]:\n", qmi_pbm_session_type_get_string (session->session_type));
            g_print ("\t\tMaximum emails: %u\n", session->maximum_emails);
            g_print ("\t\tMaximum email address length: %u\n", session->maximum_email_address_length);
        }
    }

    if (second_name_capability) {
        g_print ("Second name capability:\n");
        for (i = 0; i < second_name_capability->len; i++) {
            QmiMessagePbmGetAllCapabilitiesOutputSecondNameCapabilityElement *session;

            session = &g_array_index (second_name_capability,
                                      QmiMessagePbmGetAllCapabilitiesOutputSecondNameCapabilityElement,
                                      i);
            g_print ("\t[%s]:\n", qmi_pbm_session_type_get_string (session->session_type));
            g_print ("\t\tMaximum second name length: %u\n", session->maximum_second_name_length);
        }
    }

    if (hidden_records_capability) {
        g_print ("Hidden records capability:\n");
        for (i = 0; i < hidden_records_capability->len; i++) {
            QmiMessagePbmGetAllCapabilitiesOutputHiddenRecordsCapabilityElement *session;

            session = &g_array_index (hidden_records_capability,
                                      QmiMessagePbmGetAllCapabilitiesOutputHiddenRecordsCapabilityElement,
                                      i);
            g_print ("\t[%s]:\n", qmi_pbm_session_type_get_string (session->session_type));
            g_print ("\t\tSupported: %s\n", session->supported ? "yes" : "no");
        }
    }

    if (grouping_information_alpha_string_capability) {
        g_print ("Alpha string capability:\n");
        for (i = 0; i < grouping_information_alpha_string_capability->len; i++) {
            QmiMessagePbmGetAllCapabilitiesOutputGroupingInformationAlphaStringCapabilityElement *session;

            session = &g_array_index (grouping_information_alpha_string_capability,
                                      QmiMessagePbmGetAllCapabilitiesOutputGroupingInformationAlphaStringCapabilityElement,
                                      i);
            g_print ("\t[%s]:\n", qmi_pbm_session_type_get_string (session->session_type));
            g_print ("\t\tMaximum records: %u\n", session->maximum_records);
            g_print ("\t\tUsed records: %u\n", session->used_records);
            g_print ("\t\tMaximum string length: %u\n", session->maximum_string_length);
        }
    }

    if (additional_number_alpha_string_capability) {
        g_print ("Additional number alpha string capability:\n");
        for (i = 0; i < additional_number_alpha_string_capability->len; i++) {
            QmiMessagePbmGetAllCapabilitiesOutputAdditionalNumberAlphaStringCapabilityElement *session;

            session = &g_array_index (additional_number_alpha_string_capability,
                                      QmiMessagePbmGetAllCapabilitiesOutputAdditionalNumberAlphaStringCapabilityElement,
                                      i);
            g_print ("\t[%s]:\n", qmi_pbm_session_type_get_string (session->session_type));
            g_print ("\t\tMaximum records: %u\n", session->maximum_records);
            g_print ("\t\tUsed records: %u\n", session->used_records);
            g_print ("\t\tMaximum string length: %u\n", session->maximum_string_length);
        }
    }

    qmi_message_pbm_get_all_capabilities_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_PBM_GET_ALL_CAPABILITIES */

static gboolean
noop_cb (gpointer unused)
{
    operation_shutdown (TRUE);
    return FALSE;
}

void
qmicli_pbm_run (QmiDevice *device,
                QmiClientPbm *client,
                GCancellable *cancellable)
{
    /* Initialize context */
    ctx = g_slice_new (Context);
    ctx->device = g_object_ref (device);
    ctx->client = g_object_ref (client);
    ctx->cancellable = g_object_ref (cancellable);

#if defined HAVE_QMI_MESSAGE_PBM_GET_ALL_CAPABILITIES
    if (get_all_capabilities_flag) {
        g_debug ("Asynchronously getting phonebook capabilities...");
        qmi_client_pbm_get_all_capabilities (ctx->client,
                                             NULL,
                                             10,
                                             ctx->cancellable,
                                             (GAsyncReadyCallback)get_all_capabilities_ready,
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

#endif /* HAVE_QMI_SERVICE_PBM */
