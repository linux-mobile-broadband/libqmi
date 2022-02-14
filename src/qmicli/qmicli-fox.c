/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * qmicli -- Command line interface to control QMI devices
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
 * Copyright (C) 2022 Freedom Liu <lk@linuxdev.top>
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <string.h>
#include <assert.h>

#include <glib.h>
#include <gio/gio.h>

#include <libqmi-glib.h>

#include "qmicli.h"
#include "qmicli-helpers.h"

#if defined HAVE_QMI_SERVICE_FOX

#undef VALIDATE_UNKNOWN
#define VALIDATE_UNKNOWN(str) (str ? str : "unknown")

/* Context */
typedef struct {
    QmiDevice *device;
    QmiClientFox *client;
    GCancellable *cancellable;
} Context;
static Context *ctx;

/* Options */
static gchar *get_firmware_version_str;
static gboolean noop_flag;

static GOptionEntry entries[] = {
#if defined HAVE_QMI_MESSAGE_FOX_GET_FIRMWARE_VERSION
    { "fox-get-firmware-version", 0, 0, G_OPTION_ARG_STRING, &get_firmware_version_str,
      "Get firmware version",
      "[firmware-mcfg-apps|firmware-mcfg|apps]"
    },
#endif
    { "fox-noop", 0, 0, G_OPTION_ARG_NONE, &noop_flag,
      "Just allocate or release a FOX client. Use with `--client-no-release-cid' and/or `--client-cid'",
      NULL
    },
    { NULL }
};

GOptionGroup *
qmicli_fox_get_option_group (void)
{
    GOptionGroup *group;

    group = g_option_group_new ("fox",
                                "FOX options:",
                                "Show Foxconn Modem Service options",
                                NULL,
                                NULL);
    g_option_group_add_entries (group, entries);

    return group;
}

gboolean
qmicli_fox_options_enabled (void)
{
    static guint n_actions = 0;
    static gboolean checked = FALSE;

    if (checked)
        return !!n_actions;

    n_actions = (!!get_firmware_version_str +
                 noop_flag);

    if (n_actions > 1) {
        g_printerr ("error: too many FOX actions requested\n");
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
    if (context->client)
        g_object_unref (context->client);
    g_slice_free (Context, context);
}

static void
operation_shutdown (gboolean operation_status)
{
    /* Cleanup context and finish async operation */
    context_free (ctx);
    qmicli_async_operation_done (operation_status, FALSE);
}

#if defined HAVE_QMI_MESSAGE_FOX_GET_FIRMWARE_VERSION

static QmiMessageFoxGetFirmwareVersionInput *
get_firmware_version_input_create (const gchar *str)
{
    QmiMessageFoxGetFirmwareVersionInput *input = NULL;
    QmiFoxFirmwareVersionType type;
    GError *error = NULL;

    if (!qmicli_read_fox_firmware_version_type_from_string (str, &type)) {
        g_printerr ("error: couldn't parse input firmware version type : '%s'\n", str);
        return NULL;
    }

    input = qmi_message_fox_get_firmware_version_input_new ();
    if (!qmi_message_fox_get_firmware_version_input_set_version_type (input, type, &error)) {
        g_printerr ("error: couldn't create input data bundle: '%s'\n",
                    error->message);
        g_error_free (error);
        qmi_message_fox_get_firmware_version_input_unref (input);
        return NULL;
    }

    return input;
}

static void
get_firmware_version_ready (QmiClientFox *client,
                            GAsyncResult *res)
{
    const gchar *str = NULL;
    QmiMessageFoxGetFirmwareVersionOutput *output;
    GError *error = NULL;

    output = qmi_client_fox_get_firmware_version_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_fox_get_firmware_version_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get firmware version: %s\n", error->message);
        g_error_free (error);
        qmi_message_fox_get_firmware_version_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    qmi_message_fox_get_firmware_version_output_get_version (output, &str, NULL);

    g_print ("[%s] Firmware version retrieved:\n"
             "\tVersion: '%s'\n",
             qmi_device_get_path_display (ctx->device),
             VALIDATE_UNKNOWN (str));

    qmi_message_fox_get_firmware_version_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_FOX_GET_FIRMWARE_VERSION */

static gboolean
noop_cb (gpointer unused)
{
    operation_shutdown (TRUE);
    return FALSE;
}

void
qmicli_fox_run (QmiDevice *device,
                QmiClientFox *client,
                GCancellable *cancellable)
{
    /* Initialize context */
    ctx = g_slice_new (Context);
    ctx->device = g_object_ref (device);
    ctx->client = g_object_ref (client);
    ctx->cancellable = g_object_ref (cancellable);

#if defined HAVE_QMI_MESSAGE_FOX_GET_FIRMWARE_VERSION
    if (get_firmware_version_str) {
        QmiMessageFoxGetFirmwareVersionInput *input;

        g_debug ("Asynchronously getting firmware version...");

        input = get_firmware_version_input_create (get_firmware_version_str);
        if (!input) {
            operation_shutdown (FALSE);
            return;
        }

        qmi_client_fox_get_firmware_version (ctx->client,
                                             input,
                                             10,
                                             ctx->cancellable,
                                             (GAsyncReadyCallback)get_firmware_version_ready,
                                             NULL);
        qmi_message_fox_get_firmware_version_input_unref (input);
        return;
    }
#endif

    /* Just client allocate/release? */
    if (noop_flag) {
        g_idle_add (noop_cb, NULL);
        return;
    }

    g_warn_if_reached ();
}

#endif /* HAVE_QMI_SERVICE_FOX */

