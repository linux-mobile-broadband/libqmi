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
 * Copyright (C) 2020 Google Inc.
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

#if defined HAVE_QMI_SERVICE_SAR

/* Context */
typedef struct {
    QmiDevice *device;
    QmiClientSar *client;
    GCancellable *cancellable;
} Context;
static Context *ctx;

/* Options */
static gchar *rf_set_state_str;
static gboolean rf_get_state_flag;
static gboolean noop_flag;

#undef VALIDATE_UNKNOWN
#define VALIDATE_UNKNOWN(str) (str ? str : "unknown")

static GOptionEntry entries[] = {
#if defined HAVE_QMI_MESSAGE_SAR_RF_GET_STATE
    { "sar-rf-get-state", 0, 0, G_OPTION_ARG_NONE, &rf_get_state_flag,
      "Get RF state",
      NULL
    },
#endif
#if defined HAVE_QMI_MESSAGE_SAR_RF_SET_STATE
    { "sar-rf-set-state", 0, 0, G_OPTION_ARG_STRING, &rf_set_state_str,
      "Set RF state.",
      "[(state number)]"
    },
#endif
    { "sar-noop", 0, 0, G_OPTION_ARG_NONE, &noop_flag,
      "Just allocate or release a SAR client. Use with `--client-no-release-cid' and/or `--client-cid'",
      NULL
    },
    { NULL }
};

GOptionGroup *
qmicli_sar_get_option_group (void)
{
    GOptionGroup *group;

    group = g_option_group_new ("sar",
                                "SAR options:",
                                "Show Specific Absorption Rate options",
                                NULL,
                                NULL);
    g_option_group_add_entries (group, entries);

    return group;
}

gboolean
qmicli_sar_options_enabled (void)
{
    static guint n_actions = 0;
    static gboolean checked = FALSE;

    if (checked)
        return !!n_actions;

    n_actions = (!!rf_set_state_str +
                 rf_get_state_flag +
                 noop_flag);

    if (n_actions > 1) {
        g_printerr ("error: too many SAR actions requested\n");
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

#if defined HAVE_QMI_MESSAGE_SAR_RF_GET_STATE

static void
rf_get_state_ready (QmiClientSar *client,
                    GAsyncResult *res)
{
    QmiMessageSarRfGetStateOutput *output;
    GError *error = NULL;
    QmiSarRfState rf_state;

    output = qmi_client_sar_rf_get_state_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_sar_rf_get_state_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get SAR RF state: %s\n", error->message);
        g_error_free (error);
        qmi_message_sar_rf_get_state_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }


    qmi_message_sar_rf_get_state_output_get_state (output, &rf_state, NULL);
    g_print ("[%s] Successfully got SAR RF state: %s\n",
             qmi_device_get_path_display (ctx->device),
             qmi_sar_rf_state_get_string (rf_state));

    qmi_message_sar_rf_get_state_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_SAR_RF_GET_STATE */

#if defined HAVE_QMI_MESSAGE_SAR_RF_SET_STATE

static QmiMessageSarRfSetStateInput *
rf_set_state_input_create (const gchar *str)
{
    QmiMessageSarRfSetStateInput *input = NULL;
    QmiSarRfState rf_state;

    if (qmicli_read_sar_rf_state_from_string (str, &rf_state)) {
        GError *error = NULL;

        input = qmi_message_sar_rf_set_state_input_new ();
        if (!qmi_message_sar_rf_set_state_input_set_state (
                input,
                rf_state,
                &error)) {
            g_printerr ("error: couldn't create input data: '%s'\n",
                        error->message);
            g_error_free (error);
            qmi_message_sar_rf_set_state_input_unref (input);
            input = NULL;
        }
    }

    return input;
}

static void
rf_set_state_ready (QmiClientSar *client,
                    GAsyncResult *res)
{
    QmiMessageSarRfSetStateOutput *output;
    GError *error = NULL;

    output = qmi_client_sar_rf_set_state_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_sar_rf_set_state_output_get_result (output, &error)) {
        g_printerr ("error: couldn't set RF state: %s\n", error->message);
        g_error_free (error);

        qmi_message_sar_rf_set_state_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] RF state set successfully\n",
             qmi_device_get_path_display (ctx->device));

    qmi_message_sar_rf_set_state_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_SAR_RF_SET_STATE */

static gboolean
noop_cb (gpointer unused)
{
    operation_shutdown (TRUE);
    return FALSE;
}

void
qmicli_sar_run (QmiDevice *device,
                QmiClientSar *client,
                GCancellable *cancellable)
{
    /* Initialize context */
    ctx = g_slice_new0 (Context);
    ctx->device = g_object_ref (device);
    ctx->client = g_object_ref (client);
    ctx->cancellable = g_object_ref (cancellable);

#if defined HAVE_QMI_MESSAGE_SAR_RF_GET_STATE

    if (rf_get_state_flag) {
        g_debug ("Asynchronously getting RF power state...");
        qmi_client_sar_rf_get_state (ctx->client,
                                     NULL,
                                     10,
                                     ctx->cancellable,
                                     (GAsyncReadyCallback) rf_get_state_ready,
                                     NULL);
        return;
    }

#endif

#if defined HAVE_QMI_MESSAGE_SAR_RF_SET_STATE

    if (rf_set_state_str) {
        QmiMessageSarRfSetStateInput *input;

        g_debug ("Asynchronously setting RF power state...");
        input = rf_set_state_input_create (rf_set_state_str);
        if (!input) {
            operation_shutdown (FALSE);
            return;
        }
        qmi_client_sar_rf_set_state (ctx->client,
                                     input,
                                     10,
                                     ctx->cancellable,
                                     (GAsyncReadyCallback) rf_set_state_ready,
                                     NULL);
        qmi_message_sar_rf_set_state_input_unref (input);
        return;
    }

#endif /* HAVE_QMI_MESSAGE_SAR_RF_SET_STATE */

    /* Just client allocate/release? */
    if (noop_flag) {
        g_idle_add (noop_cb, NULL);
        return;
    }

    g_warn_if_reached ();
}

#endif /* HAVE_QMI_SERVICE_SAR */
