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
 * Copyright (C) 2023 Daniele Palmas <dnlplm@gmail.com>
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <string.h>
#include <assert.h>

#include <glib.h>
#include <gio/gio.h>

#include <libqmi-glib.h>

#include "qmicli.h"
#include "qmicli-helpers.h"

#if defined HAVE_QMI_SERVICE_ATR

/* Context */
typedef struct {
    QmiDevice    *device;
    QmiClientAtr *client;
    GCancellable *cancellable;
    guint         timeout_id;
    guint         received_indication_id;
} Context;
static Context *ctx;

/* Options */
static gchar    *send_str;
static gchar    *send_only_str;
static gboolean  noop_flag;

static GOptionEntry entries[] = {
#if defined HAVE_QMI_MESSAGE_ATR_SEND && defined HAVE_QMI_INDICATION_ATR_RECEIVED
    { "atr-send", 0, 0, G_OPTION_ARG_STRING, &send_str,
      "Send an AT command and wait for the reply",
      "[AT command]"
    },
#endif
#if defined HAVE_QMI_MESSAGE_ATR_SEND
    { "atr-send-only", 0, 0, G_OPTION_ARG_STRING, &send_only_str,
      "Send an AT command without waiting for the reply",
      "[AT command]"
    },
#endif
    { "atr-noop", 0, 0, G_OPTION_ARG_NONE, &noop_flag,
      "Just allocate or release an ATR client. Use with `--client-no-release-cid' and/or `--client-cid'",
      NULL
    },
    { NULL, 0, 0, 0, NULL, NULL, NULL }
};

GOptionGroup *
qmicli_atr_get_option_group (void)
{
    GOptionGroup *group;

    group = g_option_group_new ("atr",
                                "ATR options:",
                                "Show AT Relay Service options",
                                NULL,
                                NULL);
    g_option_group_add_entries (group, entries);

    return group;
}

gboolean
qmicli_atr_options_enabled (void)
{
    static guint n_actions = 0;
    static gboolean checked = FALSE;

    if (checked)
        return !!n_actions;

    n_actions = (!!send_str +
                 !!send_only_str +
                 noop_flag);

    if (n_actions > 1) {
        g_printerr ("error: too many ATR actions requested\n");
        exit (EXIT_FAILURE);
    }

    /* Actions that require receiving QMI indication messages must specify that
     * indications are expected. */
    if (!!send_str)
        qmicli_expect_indications ();

    checked = TRUE;
    return !!n_actions;
}

static void
context_free (Context *context)
{
    if (!context)
        return;

    if (context->timeout_id)
        g_source_remove (context->timeout_id);

    if (context->received_indication_id)
        g_signal_handler_disconnect (context->client, context->received_indication_id);

    if (context->cancellable)
        g_object_unref (context->cancellable);
    if (context->device)
        g_object_unref (context->device);
    if (context->client)
        g_object_unref (context->client);
    g_slice_free (Context, context);
}

static void
operation_shutdown (gboolean operation_status)
{
    /* Cleanup context and finish async operation */
    context_free (ctx);
    qmicli_async_operation_done (operation_status, FALSE);
}

#if defined HAVE_QMI_MESSAGE_ATR_SEND && defined HAVE_QMI_INDICATION_ATR_RECEIVED

/******************************************************************************/
/* Send */

static QmiMessageAtrSendInput *
send_input_create (const gchar *str)
{
    QmiMessageAtrSendInput *input = NULL;

    input = qmi_message_atr_send_input_new ();
    qmi_message_atr_send_input_set_message (input, str, NULL);
    return input;
}

static gboolean
send_timed_out (void)
{
    ctx->timeout_id = 0;
    g_printerr ("error: operation failed: timeout\n");
    operation_shutdown (FALSE);
    return G_SOURCE_REMOVE;
}

static gboolean
is_final_response (const gchar *reply)
{
    /* The following regexes are taken from MM serial parser */
    GRegexCompileFlags flags = G_REGEX_DOLLAR_ENDONLY | G_REGEX_RAW | G_REGEX_OPTIMIZE;
    g_autoptr(GRegex)  regex_ok = NULL;
    g_autoptr(GRegex)  regex_connect = NULL;
    g_autoptr(GRegex)  regex_cme_error = NULL;
    g_autoptr(GRegex)  regex_cms_error = NULL;
    g_autoptr(GRegex)  regex_unknown_error = NULL;
    g_autoptr(GRegex)  regex_connect_failed = NULL;

    regex_ok = g_regex_new ("\\r\\nOK(\\r\\n)+", flags, 0, NULL);
    regex_connect = g_regex_new ("\\r\\nCONNECT.*\\r\\n", flags, 0, NULL);
    regex_cme_error = g_regex_new ("\\r\\n\\+CME ERROR*\\r\\n", flags, 0, NULL);
    regex_cms_error = g_regex_new ("\\r\\n\\+CMS ERROR*\\r\\n", flags, 0, NULL);
    regex_unknown_error = g_regex_new ("\\r\\n(ERROR)|(COMMAND NOT SUPPORT)\\r\\n", flags, 0, NULL);
    regex_connect_failed = g_regex_new ("\\r\\n(NO CARRIER)|(BUSY)|(NO ANSWER)|(NO DIALTONE)\\r\\n", flags, 0, NULL);

    if (g_regex_match_full (regex_ok,
                            reply, strlen (reply),
                            0, 0, NULL, NULL) ||
        g_regex_match_full (regex_unknown_error,
                            reply, strlen (reply),
                            0, 0, NULL, NULL) ||
        g_regex_match_full (regex_cme_error,
                            reply, strlen (reply),
                            0, 0, NULL, NULL) ||
        g_regex_match_full (regex_cms_error,
                            reply, strlen (reply),
                            0, 0, NULL, NULL) ||
        g_regex_match_full (regex_connect,
                            reply, strlen (reply),
                            0, 0, NULL, NULL) ||
        g_regex_match_full (regex_connect_failed,
                            reply, strlen (reply),
                            0, 0, NULL, NULL))
        return TRUE;

    return FALSE;
}

static void
indication_received (QmiClientAtr                   *client,
                     QmiIndicationAtrReceivedOutput *output)
{
    const gchar       *received;
    g_autoptr(GError)  error = NULL;

    if (!qmi_indication_atr_received_output_get_message (output, &received, &error)) {
        g_printerr ("error: couldn't get indication message: %s\n", error->message);
        operation_shutdown (FALSE);
        return;
    }

    /* No need to print an additional '\n', since the indication already has '\r\n' */
    g_print ("%s", received);
    /* The reply can arrive with multiple indications, so we need to check if the
     * indication has the final response */
    if (is_final_response (received)) {
        g_print ("Successfully received final response\n");
        operation_shutdown (TRUE);
    }
}

static void
send_ready (QmiClientAtr *client,
            GAsyncResult *res)
{
    g_autoptr(QmiMessageAtrSendOutput) output = NULL;
    g_autoptr(GError)                  error = NULL;

    output = qmi_client_atr_send_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_atr_send_output_get_result (output, &error)) {
        g_printerr ("error: couldn't send AT command: %s\n", error->message);
        operation_shutdown (FALSE);
        return;
    }

    /* Wait for the response asynchronously: 120 seconds should be enough also
     * for long-lasting commands (e.g. AT+COPS=?) */
    ctx->timeout_id = g_timeout_add_seconds (120,
                                             (GSourceFunc) send_timed_out,
                                             NULL);

    ctx->received_indication_id = g_signal_connect (ctx->client,
                                                    "received",
                                                    G_CALLBACK (indication_received),
                                                    NULL);
}

#endif /* HAVE_QMI_MESSAGE_ATR_SEND
        * HAVE_QMI_INDICATION_ATR_RECEIVED */

#if defined HAVE_QMI_MESSAGE_ATR_SEND

/******************************************************************************/
/* Send and don't wait for the reply */

static void
send_only_ready (QmiClientAtr *client,
                 GAsyncResult *res)
{
    g_autoptr(QmiMessageAtrSendOutput) output = NULL;
    g_autoptr(GError)                  error = NULL;

    output = qmi_client_atr_send_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_atr_send_output_get_result (output, &error)) {
        g_printerr ("error: couldn't send AT command: %s\n", error->message);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("Successfully sent AT command\n");
    operation_shutdown (TRUE);
}

static void
generic_send (const gchar         *cmd,
              GAsyncReadyCallback  cb)
{
    g_autofree gchar                  *at_cmd = NULL;
    g_autoptr(QmiMessageAtrSendInput)  input = NULL;

    g_debug ("Asynchronously sending AT command...");

    at_cmd = g_strconcat (cmd, "\r", NULL);
    input = send_input_create (at_cmd);
    if (!input) {
        operation_shutdown (FALSE);
        return;
    }

    qmi_client_atr_send (ctx->client,
                         input,
                         10,
                         ctx->cancellable,
                         cb,
                         NULL);
}

#endif /* HAVE_QMI_MESSAGE_ATR_SEND */

/******************************************************************************/
/* Common */

static gboolean
noop_cb (gpointer unused)
{
    operation_shutdown (TRUE);
    return FALSE;
}

void
qmicli_atr_run (QmiDevice    *device,
                QmiClientAtr *client,
                GCancellable *cancellable)
{
    /* Initialize context */
    ctx = g_slice_new (Context);
    ctx->device = g_object_ref (device);
    ctx->client = g_object_ref (client);
    ctx->cancellable = g_object_ref (cancellable);

#if defined HAVE_QMI_MESSAGE_ATR_SEND && defined HAVE_QMI_INDICATION_ATR_RECEIVED
    if (send_str) {
        generic_send (send_str, (GAsyncReadyCallback)send_ready);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_ATR_SEND
    if (send_only_str) {
        generic_send (send_only_str, (GAsyncReadyCallback)send_only_ready);
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

#endif /* HAVE_QMI_SERVICE_ATR */
