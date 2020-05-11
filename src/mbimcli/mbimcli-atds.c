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
 * Copyright (C) 2017 Red Hat, Inc.
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
static gboolean query_signal_flag;
static gboolean query_location_flag;

static GOptionEntry entries[] = {
    { "atds-query-signal", 0, 0, G_OPTION_ARG_NONE, &query_signal_flag,
      "Query signal info",
      NULL
    },
    { "atds-query-location", 0, 0, G_OPTION_ARG_NONE, &query_location_flag,
      "Query cell location",
      NULL
    },
    { NULL }
};

GOptionGroup *
mbimcli_atds_get_option_group (void)
{
   GOptionGroup *group;

   group = g_option_group_new ("atds",
                               "AT&T Device Service options:",
                               "Show AT&T Device Service options",
                               NULL,
                               NULL);
   g_option_group_add_entries (group, entries);

   return group;
}

gboolean
mbimcli_atds_options_enabled (void)
{
    static guint n_actions = 0;
    static gboolean checked = FALSE;

    if (checked)
        return !!n_actions;

    n_actions = (query_signal_flag +
                 query_location_flag);

    if (n_actions > 1) {
        g_printerr ("error: too many AT&T Device Service actions requested\n");
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
query_signal_ready (MbimDevice   *device,
                    GAsyncResult *res)
{
    g_autoptr(MbimMessage)  response = NULL;
    g_autoptr(GError)       error = NULL;
    g_autofree gchar       *rssi_str = NULL;
    g_autofree gchar       *error_rate_str = NULL;
    g_autofree gchar       *rscp_str = NULL;
    g_autofree gchar       *ecno_str = NULL;
    g_autofree gchar       *rsrq_str = NULL;
    g_autofree gchar       *rsrp_str = NULL;
    g_autofree gchar       *rssnr_str = NULL;
    guint32 rssi = 0, error_rate = 0, rscp = 0, ecno = 0, rsrq = 0, rsrp = 0, rssnr = 0;

    response = mbim_device_command_finish (device, res, &error);
    if (!response || !mbim_message_response_get_result (response, MBIM_MESSAGE_TYPE_COMMAND_DONE, &error)) {
        g_printerr ("error: operation failed: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    if (!mbim_message_atds_signal_response_parse (
            response,
            &rssi,
            &error_rate,
            &rscp,
            &ecno,
            &rsrq,
            &rsrp,
            &rssnr,
            &error)) {
        g_printerr ("error: couldn't parse response message: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    if (rssi <= 31)
        rssi_str = g_strdup_printf ("%d dBm", -113 + (2 * rssi));

    switch (error_rate) {
    case 0:
        error_rate_str = g_strdup_printf ("< 0.2%%");
        break;
    case 1:
        error_rate_str = g_strdup_printf ("0.2%% - 0.39%%");
        break;
    case 2:
        error_rate_str = g_strdup_printf ("0.4%% - 0.79%%");
        break;
    case 3:
        error_rate_str = g_strdup_printf ("0.8%% - 1.59%%");
        break;
    case 4:
        error_rate_str = g_strdup_printf ("1.6%% - 3.19%%");
        break;
    case 5:
        error_rate_str = g_strdup_printf ("3.2%% - 6.39%%");
        break;
    case 6:
        error_rate_str = g_strdup_printf ("6.4%% - 12.79%%");
        break;
    case 7:
        error_rate_str = g_strdup_printf ("> 12.8%%");
        break;
    default:
        error_rate_str = g_strdup_printf ("unknown (%u)", error_rate);
        break;
    }

    if (rscp == 0)
        rscp_str = g_strdup_printf ("< -120 dBm");
    else if (rscp < 96)
        rscp_str = g_strdup_printf ("%d dBm", -120 + rscp);
    else if (rscp == 96)
        rscp_str = g_strdup_printf (">= -24 dBm");

    if (ecno == 0)
        ecno_str = g_strdup_printf ("< -24 dBm");
    else if (ecno < 49)
        ecno_str = g_strdup_printf ("%.2lf dBm", -24.0 + ((gdouble)ecno / 2));
    else if (ecno == 49)
        ecno_str = g_strdup_printf (">= 0.5 dBm");

    if (rsrq == 0)
        rsrq_str = g_strdup_printf ("< -19.5 dBm");
    else if (rsrq < 34)
        rsrq_str = g_strdup_printf ("%.2lf dBm", -19.5 + ((gdouble)rsrq / 2));
    else if (rsrq == 34)
        rsrq_str = g_strdup_printf (">= -2.5 dBm");

    if (rsrp == 0)
        rsrp_str = g_strdup_printf ("< -140 dBm");
    else if (rsrp < 97)
        rsrp_str = g_strdup_printf ("%d dBm", -140 + rsrp);
    else if (rsrp == 97)
        rsrp_str = g_strdup_printf (">= -43 dBm");

    if (rssnr == 0)
        rssnr_str = g_strdup_printf ("< -5 dB");
    else if (rssnr < 35)
        rssnr_str = g_strdup_printf ("%d dB", -5 + rssnr);
    else if (rssnr == 35)
        rssnr_str = g_strdup_printf (">= 30 dB");

    g_print ("[%s] Signal info retrieved:\n"
             "\t      RSSI: %s\n"
             "\t       BER: %s\n"
             "\t      RSCP: %s\n"
             "\t     Ec/No: %s\n"
             "\t      RSRQ: %s\n"
             "\t      RSRP: %s\n"
             "\t     RSSNR: %s\n",
             mbim_device_get_path_display (device),
             VALIDATE_UNKNOWN (rssi_str),
             VALIDATE_UNKNOWN (error_rate_str),
             VALIDATE_UNKNOWN (rscp_str),
             VALIDATE_UNKNOWN (ecno_str),
             VALIDATE_UNKNOWN (rsrq_str),
             VALIDATE_UNKNOWN (rsrp_str),
             VALIDATE_UNKNOWN (rssnr_str));

    shutdown (TRUE);
}

static void
query_location_ready (MbimDevice   *device,
                      GAsyncResult *res)
{
    g_autoptr(MbimMessage) response = NULL;
    g_autoptr(GError)      error = NULL;
    guint32 lac = 0, tac = 0, cellid = 0;

    response = mbim_device_command_finish (device, res, &error);
    if (!response || !mbim_message_response_get_result (response, MBIM_MESSAGE_TYPE_COMMAND_DONE, &error)) {
        g_printerr ("error: operation failed: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    if (!mbim_message_atds_location_response_parse (
            response,
            &lac,
            &tac,
            &cellid,
            &error)) {
        g_printerr ("error: couldn't parse response message: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    g_print ("[%s] Cell location retrieved:\n"
             "\t       LAC: %04x\n"
             "\t       TAC: %04x\n"
             "\t   Cell ID: %04x\n",
             mbim_device_get_path_display (device),
             lac,
             tac,
             cellid);

    shutdown (TRUE);
}

void
mbimcli_atds_run (MbimDevice   *device,
                  GCancellable *cancellable)
{
    g_autoptr(MbimMessage) request = NULL;

    /* Initialize context */
    ctx = g_slice_new (Context);
    ctx->device = g_object_ref (device);
    ctx->cancellable = cancellable ? g_object_ref (cancellable) : NULL;

    /* Request to get signal info? */
    if (query_signal_flag) {
        g_debug ("Asynchronously querying signal info...");
        request = (mbim_message_atds_signal_query_new (NULL));
        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)query_signal_ready,
                             NULL);
        return;
    }

    /* Request to get cell location? */
    if (query_location_flag) {
        g_debug ("Asynchronously querying cell location...");
        request = (mbim_message_atds_location_query_new (NULL));
        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)query_location_ready,
                             NULL);
        return;
    }

    g_warn_if_reached ();
}
