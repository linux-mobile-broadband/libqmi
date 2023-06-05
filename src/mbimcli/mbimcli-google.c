/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * mbimcli -- Command line interface to control MBIM devices
 *
 * Copyright (C) 2023 Intel Corporation
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
static gchar    *set_carrier_lock_str;
static gboolean  query_carrier_lock_flag;

static GOptionEntry entries[] = {
    { "google-set-carrier-lock", 0, 0, G_OPTION_ARG_STRING, &set_carrier_lock_str,
      "Set Google Carrier Lock",
      "[(Data)]"
    },
    { "google-query-carrier-lock", 0, 0, G_OPTION_ARG_NONE, &query_carrier_lock_flag,
      "Query Google Carrier Lock",
      NULL
    },
    { NULL, 0, 0, 0, NULL, NULL, NULL }
};

GOptionGroup *
mbimcli_google_get_option_group (void)
{
    GOptionGroup *group;

    group = g_option_group_new ("google",
                                "Google options:",
                                "Show Google Service options",
                                NULL,
                                NULL);
    g_option_group_add_entries (group, entries);

    return group;
}

gboolean
mbimcli_google_options_enabled (void)
{
    static guint    n_actions = 0;
    static gboolean checked = FALSE;

    if (checked)
        return !!n_actions;

    n_actions = (!!set_carrier_lock_str +
                 query_carrier_lock_flag);

    if (n_actions > 1) {
        g_printerr ("error: too many google actions requested\n");
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
set_carrier_lock_ready (MbimDevice   *device,
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

    g_print ("[%s] Successfully set carrier lock: \n",
             mbim_device_get_path_display (device));

    shutdown (TRUE);
}

static void
query_carrier_lock_ready (MbimDevice   *device,
                          GAsyncResult *res)
{
    g_autoptr(MbimMessage)     response = NULL;
    g_autoptr(GError)          error = NULL;
    const gchar               *carrier_lock_status_str;
    const gchar               *carrier_lock_modem_state_str;
    const gchar               *carrier_lock_cause_str;
    MbimCarrierLockStatus      carrier_lock_status;
    MbimCarrierLockModemState  carrier_lock_modem_state;
    MbimCarrierLockCause       carrier_lock_cause;

    response = mbim_device_command_finish (device, res, &error);
    if (!response || !mbim_message_response_get_result (response, MBIM_MESSAGE_TYPE_COMMAND_DONE, &error)) {
        g_printerr ("error: operation failed: %s\n", error->message);
        shutdown (FALSE);
        return;
    }

    if (!mbim_message_google_carrier_lock_response_parse (
            response,
            &carrier_lock_status,
            &carrier_lock_modem_state,
            &carrier_lock_cause,
            &error)) {
        g_printerr ("error: couldn't parse response message: %s\n", error->message);
        return;
    }

    carrier_lock_status_str      = mbim_carrier_lock_status_get_string (carrier_lock_status);
    carrier_lock_modem_state_str = mbim_carrier_lock_modem_state_get_string (carrier_lock_modem_state);
    carrier_lock_cause_str       = mbim_carrier_lock_cause_get_string (carrier_lock_cause);

    g_print ("[%s] Successfully queried carrier lock: \n"
             "\t     Carrier lock status: '%s'\n"
             "\tCarrier lock modem state: '%s'\n"
             "\t      Carrier lock cause: '%s'\n",
             mbim_device_get_path_display (device),
             VALIDATE_UNKNOWN (carrier_lock_status_str),
             VALIDATE_UNKNOWN (carrier_lock_modem_state_str),
             VALIDATE_UNKNOWN (carrier_lock_cause_str));

    shutdown (TRUE);
}

void
mbimcli_google_run (MbimDevice   *device,
                    GCancellable *cancellable)
{
    g_autoptr(MbimMessage) request = NULL;
    g_autoptr(GError)      error = NULL;

    /* Initialize context */
    ctx = g_slice_new (Context);
    ctx->device = g_object_ref (device);
    ctx->cancellable = cancellable ? g_object_ref (cancellable) : NULL;

    /* Request to set carrier lock */
    if (set_carrier_lock_str) {
        gsize               data_size = 0;
        g_autofree guint8  *data      = NULL;

        data = mbimcli_read_buffer_from_string (set_carrier_lock_str, -1, &data_size, &error);
        if (!data) {
            g_printerr ("Failed to read data: %s\n", error->message);
            shutdown (FALSE);
            return;
        }

        g_debug ("Asynchronously setting carrier lock...");
        request = mbim_message_google_carrier_lock_set_new ((guint32)data_size, data, &error);
        if (!request) {
            g_printerr ("error: couldn't create request: %s\n", error->message);
            shutdown (FALSE);
            return;
        }

        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)set_carrier_lock_ready,
                             NULL);
        return;
    }

    /* Query carrier lock information */
    if (query_carrier_lock_flag) {
        request = mbim_message_google_carrier_lock_query_new (&error);
        if (!request) {
            g_printerr ("error: couldn't create request: %s\n", error->message);
            shutdown (FALSE);
            return;
        }

        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)query_carrier_lock_ready,
                             NULL);
        return;
    }

    g_warn_if_reached ();
}
