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
#include <errno.h>

#include <glib.h>
#include <gio/gio.h>

#include <libmbim-glib.h>

#include "mbim-common.h"
#include "mbimcli.h"
#include "mbimcli-helpers.h"

/* Context */
typedef struct {
    MbimDevice *device;
    GCancellable *cancellable;
} Context;
static Context *ctx;

/* Options */
static gboolean query_rfim_flag;
static gchar    *set_rfim_str;

static GOptionEntry entries[] = {
    { "query-rfim", 0, 0, G_OPTION_ARG_NONE, &query_rfim_flag,
      "Query RFIM frequency information",
      NULL
    },
    { "set-rfim", 0, 0, G_OPTION_ARG_STRING, &set_rfim_str,
      "Enable or disable RFIM (disabled by default)",
      "[(on|off)]"
    },
    {NULL}
};

GOptionGroup *
mbimcli_intel_thermal_rf_get_option_group (void)
{
   GOptionGroup *group;

   group = g_option_group_new ("intel-thermal-rf",
                               "Intel Thermal RF Service options:",
                               "Show Intel Thermal RF Service options",
                               NULL,
                               NULL);
   g_option_group_add_entries (group, entries);

   return group;
}

gboolean
mbimcli_intel_thermal_rf_options_enabled (void)
{
    static guint n_actions = 0;
    static gboolean checked = FALSE;

    if (checked)
        return !!n_actions;

    n_actions = (query_rfim_flag +
                 !!set_rfim_str);

    if (n_actions > 1) {
        g_printerr ("error: too many Intel Thermal RF actions requested\n");
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
query_rfim_ready (MbimDevice   *device,
                  GAsyncResult *res,
                  gpointer      user_data)
{
    guint32                           i = 0;
    g_autoptr(MbimMessage)            response = NULL;
    g_autoptr(GError)                 error = NULL;
    guint32                           element_count;
    MbimIntelRfimFrequencyValueArray *rfim_frequency;
    g_autofree gchar                 *rssi_str = NULL;
    g_autofree gchar                 *sinr_str = NULL;
    g_autofree gchar                 *rsrq_str = NULL;
    g_autofree gchar                 *rsrp_str = NULL;

    response = mbim_device_command_finish (device, res, &error);
    if (!response || !mbim_message_response_get_result (response, MBIM_MESSAGE_TYPE_COMMAND_DONE, &error)) {
        g_printerr ("error: operation failed: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    if (!mbim_message_intel_thermal_rf_rfim_response_parse (response,
                                                            &element_count,
                                                            &rfim_frequency,
                                                            &error)) {
        g_printerr ("error: couldn't parse response message: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    g_print ("[%s] RFIM frequency values (%u):\n",
             mbim_device_get_path_display (device),
             element_count);

    for (i = 0; i < element_count; i++) {
        if (rfim_frequency[i]->rssi <= 31)
            rssi_str = g_strdup_printf ("%d dBm", -113 + (2 * rfim_frequency[i]->rssi));
        else
            rssi_str = g_strdup_printf ("n/a");

        if (rfim_frequency[i]->rsrq == 0)
            rsrq_str = g_strdup_printf ("< -19.5 dB");
        else if (rfim_frequency[i]->rsrq < 34)
            rsrq_str = g_strdup_printf ("%.2lf dB", -19.5 + ((gdouble)rfim_frequency[i]->rsrq / 2));
        else if (rfim_frequency[i]->rsrq == 34)
            rsrq_str = g_strdup_printf (">= -2.5 dB");
        else
            rsrq_str = g_strdup_printf ("n/a");

        if (rfim_frequency[i]->rsrp == 0)
            rsrp_str = g_strdup_printf ("< -140 dBm");
        else if (rfim_frequency[i]->rsrp < 97)
            rsrp_str = g_strdup_printf ("%d dBm", -140 + rfim_frequency[i]->rsrp);
        else if (rfim_frequency[i]->rsrp == 97)
            rsrp_str = g_strdup_printf (">= -43 dBm");
        else
            rsrp_str = g_strdup_printf ("n/a");

        if (rfim_frequency[i]->sinr == 0)
            sinr_str = g_strdup_printf ("< -23 dB");
        else if (rfim_frequency[i]->sinr < 97)
            sinr_str = g_strdup_printf ("%.21f dB", -23 + ((gdouble)rfim_frequency[i]->sinr / 2));
        else if (rfim_frequency[i]->sinr == 97)
            sinr_str = g_strdup_printf (">= 40 dB");
        else
            sinr_str = g_strdup_printf ("n/a");

        g_print ("\tElement Number: %u\n"
                 "\t Serving cell info: %s\n"
                 "\t  Center frequency: %" G_GUINT64_FORMAT " Hz\n"
                 "\t         Bandwidth: %u Hz\n"
                 "\t              RSRP: %s\n"
                 "\t              RSRQ: %s\n"
                 "\t              SINR: %s\n"
                 "\t              RSSI: %s\n"
                 "\t         Connected: %s\n",
                 i + 1,
                 mbim_intel_serving_cell_info_get_string (rfim_frequency[i]->serving_cell_info),
                 rfim_frequency[i]->center_frequency,
                 rfim_frequency[i]->bandwidth,
                 rsrp_str,
                 rsrq_str,
                 sinr_str,
                 rssi_str,
                 rfim_frequency[i]->connection_status ? "yes" : "no");
    }

    shutdown(TRUE);
}

static void
set_rfim_state_ready (MbimDevice   *device,
                      GAsyncResult *res,
                      gpointer      user_data)
{
    g_autoptr(MbimMessage) response = NULL;
    g_autoptr(GError)      error = NULL;

    response = mbim_device_command_finish (device, res, &error);
    if (!response) {
        g_printerr ("error: operation failed: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    g_print ("[%s] Successfully requested modem to set RFIM state\n",
             mbim_device_get_path_display (device));

    shutdown (TRUE);
}

void
mbimcli_intel_thermal_rf_run (MbimDevice   *device,
                              GCancellable *cancellable)
{
    g_autoptr(MbimMessage) request = NULL;
    g_autoptr(GError)      error = NULL;

    /* Initialize context */
    ctx = g_slice_new (Context);
    ctx->device = g_object_ref (device);
    ctx->cancellable = cancellable ? g_object_ref (cancellable) : NULL;

    /* Query RFIM Frequency information*/
    if (query_rfim_flag) {
        g_debug ("Asynchronously querying RFIM frequency information...");
        request = mbim_message_intel_thermal_rf_rfim_query_new (NULL);
        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)query_rfim_ready,
                             NULL);
        return;
    }

    /* Request to set rfim activation state */
    if (set_rfim_str) {
        gboolean activation_state;

        g_debug ("Asynchronously setting RFIM activation state...");

        if (g_ascii_strcasecmp (set_rfim_str, "on") == 0)
            activation_state = TRUE;
        else if (g_ascii_strcasecmp (set_rfim_str, "off") == 0)
            activation_state = FALSE;
        else {
            g_printerr ("error: invalid RFIM state: '%s'\n", set_rfim_str);
            shutdown (FALSE);
            return;
        }

        request = mbim_message_intel_thermal_rf_rfim_set_new (activation_state, NULL);
        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)set_rfim_state_ready,
                             NULL);
        return;
    }

    g_warn_if_reached ();
}
