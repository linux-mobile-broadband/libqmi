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
 * Copyright 2018 Google LLC
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
static gboolean modem_reboot_flag;

static GOptionEntry entries[] = {
    { "intel-modem-reboot", 0, 0, G_OPTION_ARG_NONE, &modem_reboot_flag,
      "Reboot modem",
      NULL
    },
    { NULL }
};

GOptionGroup *
mbimcli_intel_firmware_update_get_option_group (void)
{
   GOptionGroup *group;

   group = g_option_group_new ("intel-firmware-update",
                               "Intel Firmware Update Service options:",
                               "Show Intel Firmware Update Service options",
                               NULL,
                               NULL);
   g_option_group_add_entries (group, entries);

   return group;
}

gboolean
mbimcli_intel_firmware_update_options_enabled (void)
{
    static guint n_actions = 0;
    static gboolean checked = FALSE;

    if (checked)
        return !!n_actions;

    n_actions = modem_reboot_flag;

    if (n_actions > 1) {
        g_printerr ("error: too many Intel Firmware Update Service actions requested\n");
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
modem_reboot_ready (MbimDevice   *device,
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

    g_print ("[%s] Successfully requested modem to reboot for firmware update\n\n",
             mbim_device_get_path_display (device));

    shutdown (TRUE);
}

void
mbimcli_intel_firmware_update_run (MbimDevice   *device,
                                   GCancellable *cancellable)
{
    g_autoptr(MbimMessage) request = NULL;

    /* Initialize context */
    ctx = g_slice_new (Context);
    ctx->device = g_object_ref (device);
    ctx->cancellable = cancellable ? g_object_ref (cancellable) : NULL;

    /* Request to reboot modem? */
    if (modem_reboot_flag) {
        g_debug ("Asynchronously rebooting modem...");
        request = (mbim_message_intel_firmware_update_modem_reboot_set_new (NULL));
        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)modem_reboot_ready,
                             NULL);
        return;
    }

    g_warn_if_reached ();
}
