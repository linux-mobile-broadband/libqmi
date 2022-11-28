/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * mbimcli -- Command line interface to control MBIM devices
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
#include "mbimcli-helpers.h"

/* Context */
typedef struct {
    MbimDevice   *device;
    GCancellable *cancellable;
} Context;
static Context *ctx;

/* Options */
static gboolean  modem_reboot_set;
static gchar    *modem_reboot_str;

static gboolean modem_reboot_arg_parse (const gchar  *option_name,
                                        const gchar  *value,
                                        gpointer      user_data,
                                        GError      **error);

static GOptionEntry entries[] = {
    { "intel-modem-reboot", 0, G_OPTION_FLAG_OPTIONAL_ARG, G_OPTION_ARG_CALLBACK, G_CALLBACK (modem_reboot_arg_parse),
      "Reboot modem. Boot mode and timeout arguments only required if MBIMEx >= 2.0.",
      "[(Boot Mode),(Timeout)]"
    },
    { NULL, 0, 0, 0, NULL, NULL, NULL }
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

static gboolean
modem_reboot_arg_parse (const gchar  *option_name,
                        const gchar  *value,
                        gpointer      user_data,
                        GError      **error)
{
    modem_reboot_set = TRUE;
    modem_reboot_str = g_strdup (value);
    return TRUE;
}

gboolean
mbimcli_intel_firmware_update_options_enabled (void)
{
    static guint    n_actions = 0;
    static gboolean checked = FALSE;

    if (checked)
        return !!n_actions;

    n_actions = modem_reboot_set;

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

static gboolean
modem_reboot_v2_input_parse (const gchar       *str,
                             MbimIntelBootMode *out_boot_mode,
                             guint32           *out_timeout)
{
    g_auto(GStrv) split = NULL;

    split = g_strsplit (modem_reboot_str, ",", -1);

    if (g_strv_length (split) > 2) {
        g_printerr ("error: couldn't parse input string, too many arguments\n");
        return FALSE;
    }

    if (g_strv_length (split) < 2) {
        g_printerr ("error: couldn't parse input string, missing arguments\n");
        return FALSE;
    }

    if (!mbimcli_read_intel_boot_mode_from_string (split[0], out_boot_mode)) {
        g_printerr ("error: couldn't read boot mode, wrong value given as input\n");
        return FALSE;
    }

    if (!mbimcli_read_uint_from_string (split[1], out_timeout)) {
        g_printerr ("error: couldn't read timeout value\n");
        return FALSE;
    }

    return TRUE;
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
    if (modem_reboot_set) {
        if (mbim_device_check_ms_mbimex_version (device, 2, 0)) {
            MbimIntelBootMode  boot_mode = MBIM_INTEL_BOOT_MODE_NORMAL_MODE;
            guint32            timeout = 0;

            if (!modem_reboot_v2_input_parse (modem_reboot_str, &boot_mode, &timeout)) {
                g_printerr ("error: couldn't parse input arguments\n");
                g_printerr ("error: device in MBIMEx >= 2.0 requires boot mode and timeout arguments.\n");
                shutdown (FALSE);
                return;
            }

            request = mbim_message_intel_firmware_update_v2_modem_reboot_set_new (boot_mode, timeout, NULL);
        } else {
            if (modem_reboot_str) {
                g_printerr ("error: arguments are not expected in MBIMEx < 2.0\n");
                shutdown (FALSE);
                return;
            }

            g_debug ("Asynchronously rebooting modem...");
            request = mbim_message_intel_firmware_update_modem_reboot_set_new (NULL);
        }

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
