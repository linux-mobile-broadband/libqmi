/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * mbimcli -- Command line interface to control MBIM devices
 *
 * Copyright (C) 2014 Google, Inc.
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
static gboolean notify_host_shutdown_flag;

static GOptionEntry entries[] = {
    { "ms-notify-host-shutdown", 0, 0, G_OPTION_ARG_NONE, &notify_host_shutdown_flag,
      "Notify that host is shutting down",
      NULL
    },
    { NULL }
};

GOptionGroup *
mbimcli_ms_host_shutdown_get_option_group (void)
{
   GOptionGroup *group;

   group = g_option_group_new ("ms-host-shutdown",
                               "Microsoft Host Shutdown options:",
                               "Show Microsoft Host Shutdown Service options",
                               NULL,
                               NULL);
   g_option_group_add_entries (group, entries);

   return group;
}

gboolean
mbimcli_ms_host_shutdown_options_enabled (void)
{
    static guint n_actions = 0;
    static gboolean checked = FALSE;

    if (checked)
        return !!n_actions;

    n_actions = notify_host_shutdown_flag;

    if (n_actions > 1) {
        g_printerr ("error: too many Microsoft Host Shutdown actions requested\n");
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
ms_host_shutdown_ready (MbimDevice   *device,
                        GAsyncResult *res)
{
    g_autoptr(MbimMessage) response = NULL;
    g_autoptr(GError)      error = NULL;

    response = mbim_device_command_finish (device, res, &error);
    if (!response || !mbim_message_response_get_result (response, MBIM_MESSAGE_TYPE_COMMAND_DONE, &error)) {
        g_printerr ("error: operation failed: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    g_print ("[%s] Successfully notified that host is shutting down\n\n",
             mbim_device_get_path_display (device));

    shutdown (TRUE);
}

void
mbimcli_ms_host_shutdown_run (MbimDevice   *device,
                              GCancellable *cancellable)
{
    g_autoptr(MbimMessage) request = NULL;

    /* Initialize context */
    ctx = g_slice_new (Context);
    ctx->device = g_object_ref (device);
    ctx->cancellable = cancellable ? g_object_ref (cancellable) : NULL;

    /* Request to notify that host is shutting down */
    if (notify_host_shutdown_flag) {
        g_debug ("Asynchronously notifying host is shutting down...");
        request = (mbim_message_ms_host_shutdown_notify_set_new (NULL));
        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)ms_host_shutdown_ready,
                             NULL);
        return;
    }

    g_warn_if_reached ();
}
