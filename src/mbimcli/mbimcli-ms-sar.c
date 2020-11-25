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
 * Copyright (C) 2020 Aleksander Morgado <aleksander@aleksander.es>
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

/* Context */
typedef struct {
    MbimDevice *device;
    GCancellable *cancellable;
} Context;
static Context *ctx;

/* Options */
static gboolean query_sar_config_flag;

static GOptionEntry entries[] = {
    { "ms-query-sar-config", 0, 0, G_OPTION_ARG_NONE, &query_sar_config_flag,
      "Query SAR config",
      NULL
    },
    { NULL }
};

GOptionGroup *
mbimcli_ms_sar_get_option_group (void)
{
   GOptionGroup *group;

   group = g_option_group_new ("ms-sar",
                               "Microsoft SAR options:",
                               "Show Microsoft SAR Service options",
                               NULL,
                               NULL);
   g_option_group_add_entries (group, entries);

   return group;
}

gboolean
mbimcli_ms_sar_options_enabled (void)
{
    static guint n_actions = 0;
    static gboolean checked = FALSE;

    if (checked)
        return !!n_actions;

    n_actions = query_sar_config_flag;

    if (n_actions > 1) {
        g_printerr ("error: too many Microsoft SAR actions requested\n");
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
ms_sar_ready (MbimDevice   *device,
              GAsyncResult *res)
{
    g_autoptr(MbimMessage)              response = NULL;
    g_autoptr(GError)                   error = NULL;
    MbimSarControlMode                  mode;
    const gchar                        *mode_str;
    MbimSarBackoffState                 backoff_state;
    const gchar                        *backoff_state_str;
    MbimSarWifiHardwareState            wifi_integration;
    const gchar                        *wifi_integration_str;
    guint32                             config_states_count;
    g_autoptr(MbimSarConfigStateArray)  config_states = NULL;
    guint32                             i;

    response = mbim_device_command_finish (device, res, &error);
    if (!response || !mbim_message_response_get_result (response, MBIM_MESSAGE_TYPE_COMMAND_DONE, &error)) {
        g_printerr ("error: operation failed: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    if (!mbim_message_ms_sar_config_response_parse (
            response,
            &mode,
            &backoff_state,
            &wifi_integration,
            &config_states_count,
            &config_states,
            &error)) {
        g_printerr ("error: couldn't parse response message: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    mode_str             = mbim_sar_control_mode_get_string (mode);
    backoff_state_str    = mbim_sar_backoff_state_get_string (backoff_state);
    wifi_integration_str = mbim_sar_wifi_hardware_state_get_string (wifi_integration);

    g_print ("[%s] SAR config:\n"
             "\t                Mode: %s\n"
             "\t       Backoff state: %s\n"
             "\tWi-Fi hardware state: %s\n"
             "\t       Config States: (%u)\n",
             mbim_device_get_path_display (device),
             VALIDATE_UNKNOWN (mode_str),
             VALIDATE_UNKNOWN (backoff_state_str),
             VALIDATE_UNKNOWN (wifi_integration_str),
             config_states_count);
    for (i = 0; i < config_states_count; i++) {
        g_print ("\t\t[%u] Antenna index: %u\n"
                 "\t\t     Backoff index: %u\n",
                 i,
                 config_states[i]->antenna_index,
                 config_states[i]->backoff_index);
    }

    shutdown (TRUE);
}

void
mbimcli_ms_sar_run (MbimDevice   *device,
		    GCancellable *cancellable)
{
    g_autoptr(MbimMessage) request = NULL;

    /* Initialize context */
    ctx = g_slice_new (Context);
    ctx->device = g_object_ref (device);
    ctx->cancellable = cancellable ? g_object_ref (cancellable) : NULL;

    /* Request to notify that host is shutting down */
    if (query_sar_config_flag) {
        g_debug ("Asynchronously querying SAR config...");
        request = (mbim_message_ms_sar_config_query_new (NULL));
        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)ms_sar_ready,
                             NULL);
        return;
    }

    g_warn_if_reached ();
}
