/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * mbimcli -- Command line interface to control MBIM devices
 *
 * Copyright (C) 2022 Google, Inc.
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
static gboolean query_uicc_application_list_flag;

static GOptionEntry entries[] = {
    { "ms-query-uicc-application-list", 0, 0, G_OPTION_ARG_NONE, &query_uicc_application_list_flag,
      "Query UICC application list",
      NULL
    },
    { NULL }
};

GOptionGroup *
mbimcli_ms_uicc_low_level_access_get_option_group (void)
{
   GOptionGroup *group;

   group = g_option_group_new ("ms-uicc-low-level-access",
                               "Microsoft UICC Low Level Access Service options:",
                               "Show Microsoft UICC Low Level Access Service options",
                               NULL,
                               NULL);
   g_option_group_add_entries (group, entries);

   return group;
}

gboolean
mbimcli_ms_uicc_low_level_access_options_enabled (void)
{
    static guint n_actions = 0;
    static gboolean checked = FALSE;

    if (checked)
        return !!n_actions;

    n_actions = query_uicc_application_list_flag;

    if (n_actions > 1) {
        g_printerr ("error: too many Microsoft UICC Low Level Access Service actions requested\n");
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
application_list_query_ready (MbimDevice   *device,
                              GAsyncResult *res)
{
    g_autoptr(MbimMessage)              response = NULL;
    g_autoptr(GError)                   error = NULL;
    guint32                             application_count;
    guint32                             active_application_index;
    g_autoptr(MbimUiccApplicationArray) applications = NULL;
    guint32                             i;

    response = mbim_device_command_finish (device, res, &error);
    if (!response || !mbim_message_response_get_result (response, MBIM_MESSAGE_TYPE_COMMAND_DONE, &error)) {
        g_printerr ("error: operation failed: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    if (!mbim_message_ms_uicc_low_level_access_application_list_response_parse (
            response,
            NULL, /* version */
            &application_count,
            &active_application_index,
            NULL, /* application_list_size_bytes */
            &applications,
            &error)) {
        g_printerr ("error: couldn't parse response message: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    g_print ("[%s] UICC applications: (%u)\n",
             mbim_device_get_path_display (device),
             application_count);

    for (i = 0; i < application_count; i++) {
        g_autofree gchar *application_id_str = NULL;
        g_autofree gchar *pin_key_references_str = NULL;

        application_id_str = mbim_common_str_hex (applications[i]->application_id, applications[i]->application_id_size, ':');
        pin_key_references_str = mbim_common_str_hex (applications[i]->pin_key_references, applications[i]->pin_key_references_size, ':');

        g_print ("Application %u:%s\n",             i, (i == active_application_index) ? " (active)" : "");
        g_print ("\tApplication type:        %s\n", mbim_uicc_application_type_get_string (applications[i]->application_type));
        g_print ("\tApplication ID:          %s\n", application_id_str);
        g_print ("\tApplication name:        %s\n", applications[i]->application_name);
        g_print ("\tPIN key reference count: %u\n", applications[i]->pin_key_reference_count);
        g_print ("\tPIN key references:      %s\n", pin_key_references_str);
    }

    shutdown (TRUE);
}

void
mbimcli_ms_uicc_low_level_access_run (MbimDevice   *device,
                                      GCancellable *cancellable)
{
    g_autoptr(MbimMessage) request = NULL;

    /* Initialize context */
    ctx = g_slice_new (Context);
    ctx->device = g_object_ref (device);
    ctx->cancellable = cancellable ? g_object_ref (cancellable) : NULL;

    /* Request to query UICC application list? */
    if (query_uicc_application_list_flag) {
        g_debug ("Asynchronously querying UICC application list...");
        request = mbim_message_ms_uicc_low_level_access_application_list_query_new (NULL);
        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)application_list_query_ready,
                             NULL);
        return;
    }

    g_warn_if_reached ();
}
