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
    MbimDevice *device;
    GCancellable *cancellable;
} Context;
static Context *ctx;

/* Options */
static gboolean query_fcc_lock_flag;
static gchar    *set_fcc_lock_str;

static GOptionEntry entries[] = {
    { "query-fcc-lock", 0, 0, G_OPTION_ARG_NONE, &query_fcc_lock_flag,
      "Query FCC lock information",
      NULL
    },
    { "set-fcc-lock", 0, 0, G_OPTION_ARG_STRING, &set_fcc_lock_str,
      "Set FCC lock information",
      "[(ResponsePresent),(Response)]"
    },
    { NULL, 0, 0, 0, NULL, NULL, NULL }
};

GOptionGroup *
mbimcli_intel_mutual_authentication_get_option_group (void)
{
   GOptionGroup *group;

   group = g_option_group_new ("intel-mutual-authentication",
                               "Intel mutual authentication Service options:",
                               "Show Intel mutual authentication Service options",
                               NULL,
                               NULL);
   g_option_group_add_entries (group, entries);

   return group;
}

gboolean
mbimcli_intel_mutual_authentication_options_enabled (void)
{
    static guint n_actions = 0;
    static gboolean checked = FALSE;

    if (checked)
        return !!n_actions;

    n_actions = (query_fcc_lock_flag +
                 !!set_fcc_lock_str);

    if (n_actions > 1) {
        g_printerr ("error: too many Intel mutual Authentication actions requested\n");
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
query_fcc_lock_ready (MbimDevice *device,
                      GAsyncResult *res,
                      gpointer     user_data)
{
    MbimMessage  *response          = NULL;
    GError       *error             = NULL;
    gboolean      challenge_present = FALSE;
    guint32       challenge         = 0;

    response = mbim_device_command_finish (device, res, &error);
    if (!response || !mbim_message_response_get_result (response, MBIM_MESSAGE_TYPE_COMMAND_DONE, &error)) {
        g_printerr ("error: operation failed: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    if (!mbim_message_intel_mutual_authentication_fcc_lock_response_parse (response,
                                                                           &challenge_present,
                                                                           &challenge,
                                                                           &error)) {
        g_printerr ("error: couldn't parse response message: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    g_print ("FCC lock status: %s\n", challenge_present ? "locked" : "unlocked");
    if (challenge_present)
        g_print ("\tChallenge: %u\n", challenge);

    shutdown (TRUE);
}

void
mbimcli_intel_mutual_authentication_run (MbimDevice   *device,
                                         GCancellable *cancellable)
{
    g_autoptr(MbimMessage) request = NULL;

    /* Initialize context */
    ctx = g_slice_new (Context);
    ctx->device = g_object_ref (device);
    ctx->cancellable = cancellable ? g_object_ref (cancellable) : NULL;

    /* Query FCC lock information */
    if (query_fcc_lock_flag) {
        g_debug ("Asynchronously querying FCC lock information...");

        request = mbim_message_intel_mutual_authentication_fcc_lock_query_new (NULL);
        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)query_fcc_lock_ready,
                             NULL);
        return;
    }

    /* Set FCC lock information */
    if (set_fcc_lock_str) {
        gboolean       response_present = FALSE;
        guint32        response = 0;
        g_auto(GStrv)  split = NULL;

        split = g_strsplit (set_fcc_lock_str, ",", -1);

        if (g_strv_length (split) < 2) {
            g_printerr ("error: couldn't parse input arguments, missing arguments\n");
            shutdown (FALSE);
            return;
        }

        if (g_strv_length (split) > 2) {
            g_printerr ("error: couldn't parse input arguments, too many arguments\n");
            shutdown (FALSE);
            return;
        }

        if (!mbimcli_read_boolean_from_string (split[0], &response_present)) {
            g_printerr ("error: couldn't parse input, wrong value given\n");
            shutdown (FALSE);
            return;
        }

        if (!mbimcli_read_uint_from_string (split[1], &response)) {
            g_printerr ("error: couldn't parse input, wrong value given\n");
            shutdown (FALSE);
            return;
        }

        request = mbim_message_intel_mutual_authentication_fcc_lock_set_new (response_present, response, NULL);
        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)query_fcc_lock_ready,
                             NULL);
        return;
    }
    g_warn_if_reached ();
}
