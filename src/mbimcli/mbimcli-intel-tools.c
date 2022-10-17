/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * mbimcli -- Command line interface to control MBIM devices
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
 * Copyright (C) 2022 Intel Corporation
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <string.h>

#include <glib.h>
#include <gio/gio.h>
#include <glib/gprintf.h>
#include <libmbim-glib.h>

#include "mbim-common.h"
#include "mbimcli.h"
#include "mbimcli-helpers.h"

/* Context */
typedef struct {
    MbimDevice   *device;
    GCancellable *cancellable;
} Context;
static Context *ctx;

/* Options */
static char *set_trace_config_str;
static char *query_trace_config_str;

static GOptionEntry entries[] = {
    { "set-trace-config", 0, 0, G_OPTION_ARG_STRING, &set_trace_config_str,
      "Set trace configuration",
      "[(TraceCmd)|(TraceValue)]"
    },
    { "query-trace-config", 0, 0, G_OPTION_ARG_STRING, &query_trace_config_str,
      "Query trace configuration",
      "[(TraceCmd)]"
    },
    { NULL }
};

GOptionGroup *
mbimcli_intel_tools_get_option_group (void)
{
   GOptionGroup *group;
   group = g_option_group_new ("intel-tools",
                               "Intel 5G tools options",
                               "Show Intel 5G tools options",
                               NULL,
                               NULL);
   g_option_group_add_entries (group, entries);

   return group;
}

gboolean
mbimcli_intel_tools_options_enabled (void)
{
    static guint n_actions = 0;
    static gboolean checked = FALSE;

    if (checked)
        return !!n_actions;

    n_actions =(!!set_trace_config_str +
                !!query_trace_config_str);

    if (n_actions > 1) {
        g_printerr ("error: too many intel tools actions requested\n");
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
query_trace_config_ready (MbimDevice   *device,
                          GAsyncResult *res)
{
    g_autoptr(MbimMessage)  response = NULL;
    g_autoptr(GError)       error = NULL;
    MbimTraceCommand        trace_cmd;
    const gchar            *trace_cmd_str;
    guint32                 trace_result;

    response = mbim_device_command_finish (device, res, &error);
    if (!response || !mbim_message_response_get_result (response, MBIM_MESSAGE_TYPE_COMMAND_DONE, &error)) {
        g_printerr ("error: operation failed: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    g_print ("[%s] Successfully retrieved trace configuration\n",
             mbim_device_get_path_display (device));
    if (!mbim_message_intel_tools_trace_config_response_parse (
            response,
            &trace_cmd,
            &trace_result,
            &error)) {
        g_printerr ("error: couldn't parse response messages: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    trace_cmd_str = mbim_trace_command_get_string (trace_cmd);

    g_print ("[%s] Trace configuration retrieved:\n"
             "\t Trace Command: '%s'\n"
             "\t  Trace Result: '%u'\n",
             mbim_device_get_path_display (device),
             VALIDATE_UNKNOWN(trace_cmd_str),
             trace_result);

    shutdown (TRUE);
}

static void
set_trace_config_ready (MbimDevice   *device,
                        GAsyncResult *res)
{
    g_autoptr(MbimMessage)  response = NULL;
    g_autoptr(GError)       error = NULL;

    response = mbim_device_command_finish (device, res, &error);
    if (!response || !mbim_message_response_get_result (response, MBIM_MESSAGE_TYPE_COMMAND_DONE, &error)) {
        g_printerr ("error: operation failed: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    g_print ("[%s] Successfully retrieved trace configuration\n",
             mbim_device_get_path_display (device));

    shutdown (TRUE);
}

void
mbimcli_intel_tools_run (MbimDevice   *device,
                         GCancellable *cancellable)
{
    g_autoptr(MbimMessage) request = NULL;
    g_autoptr(GError)      error = NULL;

    /* Initialize context */
    ctx = g_slice_new (Context);
    ctx->device = g_object_ref (device);
    ctx->cancellable = cancellable ? g_object_ref (cancellable) : NULL;

    /* Request to set trace config? */
    if (set_trace_config_str) {
        g_auto(GStrv)    split = NULL;
        MbimTraceCommand trace_command;
        guint32          trace_value = 0;

        split = g_strsplit (set_trace_config_str, ",", -1);

        if (g_strv_length (split) > 2) {
            g_printerr ("error: couldn't parse input string, too many arguments\n");
            return;
        }

        if (g_strv_length (split) < 2) {
            g_printerr ("error: couldn't parse input string, missing arguments\n");
            return;
        }

        if (split[0]) {
            if (!mbimcli_read_trace_command_from_string (split[0], &trace_command)) {
                g_printerr ("error: couldn't parse input string, invalid trace command '%s'\n", split[0]);
                return;
            }
        }

        if (split[1]) {
            if (!mbimcli_read_uint_from_string (split[1], &trace_value)) {
                g_printerr ("error: couldn't parse input string, invalid trace value '%s'\n", split[1]);
                return;
            }
        }

        g_debug ("Asynchronously setting trace info...");
        request = mbim_message_intel_tools_trace_config_set_new (trace_command, trace_value, NULL);
        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)set_trace_config_ready,
                             NULL);
        return;
    }

    /* Request to get trace config? */
    if (query_trace_config_str) {
        MbimTraceCommand trace_command;

        if (query_trace_config_str) {
            if (!mbimcli_read_trace_command_from_string (query_trace_config_str, &trace_command)) {
                g_printerr ("error: couldn't parse input string, invalid trace command '%d'\n", trace_command);
                return;
            }
        }

        g_debug ("Asynchronously querying trace info...");
        request = mbim_message_intel_tools_trace_config_query_new (trace_command, NULL);
        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)query_trace_config_ready,
                             NULL);
        return;
    }

    g_warn_if_reached ();
}
