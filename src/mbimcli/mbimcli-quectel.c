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
static gchar    *set_command_str;

static GOptionEntry entries[] = {
    { "quectel-query-radio-state", 0, 0, G_OPTION_ARG_NONE, &query_radio_state_flag,
      "Query radio state",
      NULL
    },
    { "quectel-set-radio-state", 0, 0, G_OPTION_ARG_STRING, &set_radio_state_str,
      "Set radio state",
      "[(on)]"
    },
    { "quectel-set-command", 0, 0, G_OPTION_ARG_STRING, &set_command_str,
      "Send command to module (Command type is optional, defaults to AT, allowed options: "
      "(at, system)",
      "[(Command type),(\"Command\")]"
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
                 !!set_command_str);

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
qdu_command_ready (MbimDevice   *device,
                      GAsyncResult *res)
{
    g_autoptr(GError)       error      = NULL;
    guint32                 ret_status = 0;
    guint32                 ret_size   = 0;
    const guint8           *ret_data   = NULL;
    g_autoptr(MbimMessage)  response   = NULL;

    response = mbim_device_command_finish (device, res, &error);

    if (!response || !mbim_message_response_get_result (response, MBIM_MESSAGE_TYPE_COMMAND_DONE, &error)) {
        g_printerr ("error: operation failed: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    if (!mbim_message_qdu_command_response_parse (
            response,
            &ret_status,
            &ret_size,
            &ret_data,
            &error)) {
        g_printerr ("error: couldn't parse response message: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    if (ret_status == MBIM_QUECTEL_COMMAND_RESPONSE_STATUS_OK)
        g_print ("%.*s\n", ret_size , ret_data);
    else
        g_print ("error: Command returns error!!\n");

    shutdown (TRUE);
}

static gboolean
set_command_input_parse (const gchar             *str,
                         gchar                  **command_str,
                         MbimQuectelCommandType  *command_type)
{
    g_auto(GStrv)          split            = NULL;
    guint                  num_parts        = 0;
    g_autofree gchar      *command_type_str = NULL;
    MbimQuectelCommandType new_command_type;
    g_autoptr(GError)      error            = NULL;

    g_assert (command_str != NULL);

    /* Format of the string is:
     *    "[\"Command\"]"
     * or:
     *    "[(Command type),(\"Command\")]"
     */
    split = g_strsplit (str, ",", -1);
    num_parts = g_strv_length (split);

    /* The at command may have multiple commas, like:at+qcfg="usbcfg",0x2C7C,0x6008,0x00FF ,
     * so we need to take the first split to see if it is the command type.
     * If it is, then combine the remaining splits into a string.
     * If not, combine the splits into a string. */
    if (num_parts > 0 && split[0] != NULL) {
        command_type_str = g_ascii_strdown (split[0], -1);

        if (!g_strcmp0 (command_type_str, "at") ||
            !g_strcmp0 (command_type_str, "system")) {
            if (!mbimcli_read_quectel_command_type_from_string (command_type_str, &new_command_type, &error)) {
                g_printerr ("error: couldn't parse input command-type: %s\n", error->message);
                return FALSE;
            }

            if ((new_command_type != MBIM_QUECTEL_COMMAND_TYPE_AT) &&
                (new_command_type != MBIM_QUECTEL_COMMAND_TYPE_SYSTEM)) {
                g_printerr ("error: couldn't parse input string, invalid command type.\n");
                return FALSE;
            }

            *command_type = new_command_type;

            if (num_parts > 1)
                *command_str = g_strjoinv (",", split + 1);
            else {
                g_printerr ("error: couldn't parse input string, missing arguments.\n");
                return FALSE;
            }
        } else if (num_parts > 1) {
            *command_str = g_strjoinv (",", split);
        } else {
            *command_str = g_strdup (split[0]);
        }

        if (g_str_has_prefix (*command_str, "AT") || g_str_has_prefix (*command_str, "at"))
            return TRUE;
        else {
            g_printerr ("error: Wrong AT command , command must start with \"AT\".\n");
            return FALSE;
        }
    } else {
        g_printerr ("error: The input string is empty, please re-enter.\n");
        return FALSE;
    }
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
    if (set_command_str) {
        g_autofree gchar      *req_str = NULL;
        guint32                req_size = 0;
        MbimQuectelCommandType command_type = MBIM_QUECTEL_COMMAND_TYPE_AT;

        if (!set_command_input_parse (set_command_str, &req_str, &command_type)) {
            g_printerr ("error: parse input string failed!\n");
            shutdown (FALSE);
            return;
        }

        req_size = strlen (req_str);

        request = mbim_message_qdu_command_set_new (command_type,
                                                    req_size,
                                                    (const guint8 *) req_str,
                                                    NULL);
        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)qdu_command_ready,
                             NULL);
        return;
    }

    g_warn_if_reached ();
}
