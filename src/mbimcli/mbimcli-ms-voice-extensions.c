/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * mbimcli -- Command line interface to control MBIM devices
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
    MbimDevice   *device;
    GCancellable *cancellable;
} Context;
static Context *ctx;

/* Options */
static gboolean query_nitz_flag;

static GOptionEntry entries[] = {
    { "query-nitz", 0, 0, G_OPTION_ARG_NONE, &query_nitz_flag,
      "Query network identity and time zone",
      NULL
    },
    { NULL }
};

GOptionGroup *
mbimcli_ms_voice_extensions_get_option_group (void)
{
   GOptionGroup *group;

   group = g_option_group_new ("ms-voice-extensions",
                               "Microsoft Voice Extensions Service options:",
                               "Show Microsoft Voice Extensions Service options",
                               NULL,
                               NULL);
   g_option_group_add_entries (group, entries);

   return group;
}

gboolean
mbimcli_ms_voice_extensions_options_enabled (void)
{
    static guint n_actions = 0;
    static gboolean checked = FALSE;

    if (checked)
        return !!n_actions;

    n_actions = query_nitz_flag;

    if (n_actions > 1) {
        g_printerr ("error: too many Microsoft Voice Extensions Service actions requested\n");
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
query_nitz_ready (MbimDevice   *device,
                  GAsyncResult *res)
{
    g_autoptr(MbimMessage) response = NULL;
    g_autoptr(GError)      error = NULL;
    guint32 day = 0;
    guint32 month = 0;
    guint32 year = 0;
    guint32 hour = 0;
    guint32 minutes = 0;
    guint32 second = 0;
    guint32 time_zone_offset_minutes = 0;
    guint32 daylight_saving_time_offset_minutes = 0;
    MbimDataClass data_class;
    g_autofree gchar *data_class_str = NULL;

    response = mbim_device_command_finish (device, res, &error);
    if (!response || !mbim_message_response_get_result (response, MBIM_MESSAGE_TYPE_COMMAND_DONE, &error)) {
        g_printerr ("error: operation failed: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    if (!mbim_message_ms_voice_extensions_nitz_response_parse (
              response,
              &year,
              &month,
              &day,
              &hour,
              &minutes,
              &second,
              &time_zone_offset_minutes,
              &daylight_saving_time_offset_minutes,
              &data_class,
              &error)) {
        g_printerr ("error: couldn't parse response message: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    data_class_str = mbim_data_class_build_string_from_mask (data_class);

    g_print ("Successfully queried NITZ info from modem:\n"
             "\t                               Date: %02u/%02u/%u\n"
             "\t                               Time: %02u:%02u:%02u\n"
             "\t           Time zone offset minutes: %u\n"
             "\tDaylight saving time offset minutes: %u\n"
             "\t                         Data class: %s\n",
             day,
             month,
             year,
             hour,
             minutes,
             second,
             time_zone_offset_minutes,
             daylight_saving_time_offset_minutes,
             VALIDATE_UNKNOWN (data_class_str));

    shutdown (TRUE);
}

void
mbimcli_ms_voice_extensions_run (MbimDevice   *device,
                                 GCancellable *cancellable)
{
    g_autoptr(MbimMessage) request = NULL;

    /* Initialize context */
    ctx = g_slice_new (Context);
    ctx->device = g_object_ref (device);
    ctx->cancellable = cancellable ? g_object_ref (cancellable) : NULL;

    /* Request to reboot modem? */
    if (query_nitz_flag) {
        g_debug ("Asynchronously querying nitz info...");
        request = mbim_message_ms_voice_extensions_nitz_query_new (NULL);
        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)query_nitz_ready,
                             NULL);
        return;
    }

    g_warn_if_reached ();
}
