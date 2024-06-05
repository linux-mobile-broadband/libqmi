/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * mbimcli -- Command line interface to control MBIM devices
 *
 * Copyright (C) 2021 Aleksander Morgado <aleksander@aleksander.es>
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <string.h>

#include <glib.h>
#include <gio/gio.h>

#include <libmbim-glib.h>

#include "mbimcli.h"
#include "mbimcli-helpers.h"

/* Context */
typedef struct {
    MbimDevice   *device;
    GCancellable *cancellable;
} Context;
static Context *ctx;

/* Options */
static gboolean  query_radio_state_flag;
static gchar    *set_radio_state_str;
static gchar    *set_at_command_str;

static GOptionEntry entries[] = {
    { "quectel-query-radio-state", 0, 0, G_OPTION_ARG_NONE, &query_radio_state_flag,
      "Query radio state",
      NULL
    },
    { "quectel-set-radio-state", 0, 0, G_OPTION_ARG_STRING, &set_radio_state_str,
      "Set radio state",
      "[(on)]"
    },
    { "quectel-set-at-command", 0, 0, G_OPTION_ARG_STRING, &set_at_command_str,
      "Send AT command to module, and receive AT response",
      "\"<AT Command>\""
    },
    { NULL, 0, 0, 0, NULL, NULL, NULL }
};

GOptionGroup *
mbimcli_quectel_get_option_group (void)
{
   GOptionGroup *group;

   group = g_option_group_new ("quectel",
                               "Quectel options:",
                               "Show Quectel Service options",
                               NULL,
                               NULL);
   g_option_group_add_entries (group, entries);

   return group;
}

gboolean
mbimcli_quectel_options_enabled (void)
{
    static guint n_actions = 0;
    static gboolean checked = FALSE;

    if (checked)
        return !!n_actions;

    n_actions = (query_radio_state_flag +
                 !!set_radio_state_str +
                 !!set_at_command_str);

    if (n_actions > 1) {
        g_printerr ("error: too many Quectel actions requested\n");
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
radio_state_ready (MbimDevice   *device,
                   GAsyncResult *res,
                   gpointer      user_data)
{
    g_autoptr(MbimMessage)       response = NULL;
    g_autoptr(GError)            error = NULL;
    MbimQuectelRadioSwitchState  radio_state;
    const gchar                 *radio_state_str;
    gboolean                     action_flag;

    action_flag = GPOINTER_TO_UINT (user_data);

    response = mbim_device_command_finish (device, res, &error);
    if (!response || !mbim_message_response_get_result (response, MBIM_MESSAGE_TYPE_COMMAND_DONE, &error)) {
        g_printerr ("error: operation failed: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    if (action_flag) {
        g_print ("[%s] Successfully requested to enable radio\n",
                 mbim_device_get_path_display (device));
        shutdown (TRUE);
        return;
    }

    /* The body is only included in the query response, not in the set response */
    if (!mbim_message_quectel_radio_state_response_parse (response, &radio_state, &error)) {
        g_printerr ("error: couldn't parse response message: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    radio_state_str = mbim_quectel_radio_switch_state_get_string (radio_state);

    g_print ("[%s] Radio state retrieved: '%s'\n",
             mbim_device_get_path_display (device),
             VALIDATE_UNKNOWN (radio_state_str));

    shutdown (TRUE);
}

static void
qdu_at_command_ready (MbimDevice   *device,
                      GAsyncResult *res)
{
    g_autoptr(GError)       error    = NULL;
    guint32                 ret_size = 0;
    const guint8           *ret_data = NULL;
    g_autoptr(MbimMessage)  response = NULL;

    response = mbim_device_command_finish (device, res, &error);

    if (!response || !mbim_message_response_get_result (response, MBIM_MESSAGE_TYPE_COMMAND_DONE, &error)) {
        g_printerr ("error: operation failed: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    if (!mbim_message_qdu_at_command_response_parse (
            response,
            &ret_size,
            &ret_data,
            &error)) {
        g_printerr ("error: couldn't parse response message: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    /* The first four data are the status of the at response */
    g_print ("%.*s\n", (int)(ret_size - sizeof (guint32)), ret_data + sizeof (guint32));

    shutdown (TRUE);
}

void
mbimcli_quectel_run (MbimDevice   *device,
                     GCancellable *cancellable)
{
    g_autoptr(MbimMessage) request = NULL;

    /* Initialize context */
    ctx = g_slice_new (Context);
    ctx->device = g_object_ref (device);
    ctx->cancellable = cancellable ? g_object_ref (cancellable) : NULL;

    /* Request to get radio state? */
    if (query_radio_state_flag) {
        g_debug ("Asynchronously querying radio state...");
        request = (mbim_message_quectel_radio_state_query_new (NULL));
        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)radio_state_ready,
                             GUINT_TO_POINTER (FALSE));
        return;
    }

    /* Request to set radio state? */
    if (set_radio_state_str) {
        MbimQuectelRadioSwitchState radio_state;

        if (g_ascii_strcasecmp (set_radio_state_str, "on") == 0)
            radio_state = MBIM_QUECTEL_RADIO_SWITCH_STATE_ON;
        else {
            g_printerr ("error: invalid radio state (only 'on' allowed): '%s'\n", set_radio_state_str);
            shutdown (FALSE);
            return;
        }

        g_debug ("Asynchronously setting radio state to on...");
        request = mbim_message_quectel_radio_state_set_new (radio_state, NULL);
        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)radio_state_ready,
                             GUINT_TO_POINTER (TRUE));
        return;
    }

    /* Request to send AT command */
    if (set_at_command_str) {
        g_autoptr(GByteArray) buffer   = NULL;
        gint32                cmdlen   = 0;
        gint32                cmd_type = 0;

        cmdlen = strlen (set_at_command_str);
        buffer = g_byte_array_sized_new (cmdlen + sizeof (gint32) + 2);

        /* Specify command type: AT -- 0 */
        g_byte_array_append (buffer, (const guint8 *) &cmd_type, sizeof (gint32));
        g_byte_array_append (buffer, (const guint8 *) set_at_command_str, cmdlen);

        request = mbim_message_qdu_at_command_set_new (buffer->len,
                                                       (const guint8 *) buffer->data,
                                                       NULL);
        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)qdu_at_command_ready,
                             NULL);
        return;
    }

    g_warn_if_reached ();
}
