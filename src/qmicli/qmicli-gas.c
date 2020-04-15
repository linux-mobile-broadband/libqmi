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
 * Copyright (C) 2019 Andreas Kling <awesomekling@gmail.com>
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <string.h>

#include <glib.h>
#include <gio/gio.h>

#include <libqmi-glib.h>

#include "qmicli.h"
#include "qmicli-helpers.h"

#if defined HAVE_QMI_SERVICE_GAS

/* Context */
typedef struct {
    QmiDevice *device;
    QmiClientGas *client;
    GCancellable *cancellable;
} Context;
static Context *ctx;

/* Options */
static gboolean get_firmware_list_flag;
static gboolean get_active_firmware_flag;
static gint     set_active_firmware_int = -1;
static gboolean noop_flag;

static GOptionEntry entries[] = {
#if defined HAVE_QMI_MESSAGE_GAS_DMS_GET_FIRMWARE_LIST
    { "gas-dms-get-firmware-list", 0, 0, G_OPTION_ARG_NONE, &get_firmware_list_flag,
      "Gets the list of stored firmware",
      NULL
    },
    { "gas-dms-get-active-firmware", 0, 0, G_OPTION_ARG_NONE, &get_active_firmware_flag,
      "Gets the currently active firmware",
      NULL
    },
#endif
#if defined HAVE_QMI_MESSAGE_GAS_DMS_SET_ACTIVE_FIRMWARE
    { "gas-dms-set-active-firmware", 0, 0, G_OPTION_ARG_INT, &set_active_firmware_int,
      "Sets the active firmware index",
      "[index]"
    },
#endif
    { "gas-noop", 0, 0, G_OPTION_ARG_NONE, &noop_flag,
      "Just allocate or release a GAS client. Use with `--client-no-release-cid' and/or `--client-cid'",
      NULL
    },
    { NULL }
};

GOptionGroup *
qmicli_gas_get_option_group (void)
{
    GOptionGroup *group;

    group = g_option_group_new ("gas",
                                "GAS options:",
                                "Show General Application Service options",
                                NULL,
                                NULL);
    g_option_group_add_entries (group, entries);

    return group;
}

gboolean
qmicli_gas_options_enabled (void)
{
    static guint n_actions = 0;
    static gboolean checked = FALSE;

    if (checked)
        return !!n_actions;

    n_actions = (get_firmware_list_flag +
                 get_active_firmware_flag +
                 (set_active_firmware_int >= 0) +
                 noop_flag);

    if (n_actions > 1) {
        g_printerr ("error: too many GAS actions requested\n");
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

#if defined HAVE_QMI_MESSAGE_GAS_DMS_GET_FIRMWARE_LIST

static void
print_firmware_listing (guint8       idx,
                        const gchar *name,
                        const gchar *version,
                        const gchar *pri_revision)
{
    g_print ("Firmware #%u:\n"
             "\tIndex:        %u\n"
             "\tName:         %s\n"
             "\tVersion:      %s\n"
             "\tPRI revision: %s\n",
             idx,
             idx,
             name,
             version,
             pri_revision);
}

static void
get_firmware_list_ready (QmiClientGas *client,
                         GAsyncResult *res)
{
    QmiMessageGasDmsGetFirmwareListOutput *output;
    GError *error = NULL;
    guint8 idx;
    const gchar *name;
    const gchar *version;
    const gchar *pri_revision;

    output = qmi_client_gas_dms_get_firmware_list_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_gas_dms_get_firmware_list_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get stored firmware list: %s\n", error->message);
        g_error_free (error);
        qmi_message_gas_dms_get_firmware_list_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    if (qmi_message_gas_dms_get_firmware_list_output_get_stored_firmware_1 (output, &idx, &name, &version, &pri_revision, NULL))
        print_firmware_listing (idx, name, version, pri_revision);

    if (qmi_message_gas_dms_get_firmware_list_output_get_stored_firmware_2 (output, &idx, &name, &version, &pri_revision, NULL))
        print_firmware_listing (idx, name, version, pri_revision);

    if (qmi_message_gas_dms_get_firmware_list_output_get_stored_firmware_3 (output, &idx, &name, &version, &pri_revision, NULL))
        print_firmware_listing (idx, name, version, pri_revision);

    if (qmi_message_gas_dms_get_firmware_list_output_get_stored_firmware_4 (output, &idx, &name, &version, &pri_revision, NULL))
        print_firmware_listing (idx, name, version, pri_revision);

    qmi_message_gas_dms_get_firmware_list_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_GAS_DMS_GET_FIRMWARE_LIST */

#if defined HAVE_QMI_MESSAGE_GAS_DMS_SET_ACTIVE_FIRMWARE

static void
set_active_firmware_ready (QmiClientGas *client,
                           GAsyncResult *res)
{
    QmiMessageGasDmsSetActiveFirmwareOutput *output;
    GError *error = NULL;

    output = qmi_client_gas_dms_set_active_firmware_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_gas_dms_set_active_firmware_output_get_result (output, &error)) {
        g_printerr ("error: couldn't set active firmware list: %s\n", error->message);
        g_error_free (error);
        qmi_message_gas_dms_set_active_firmware_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("Successfully set the active firmware.\n");

    qmi_message_gas_dms_set_active_firmware_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_GAS_DMS_SET_ACTIVE_FIRMWARE */

static gboolean
noop_cb (gpointer unused)
{
    operation_shutdown (TRUE);
    return FALSE;
}

void
qmicli_gas_run (QmiDevice *device,
                QmiClientGas *client,
                GCancellable *cancellable)
{
    /* Initialize context */
    ctx = g_slice_new (Context);
    ctx->device = g_object_ref (device);
    ctx->client = g_object_ref (client);
    ctx->cancellable = g_object_ref (cancellable);

#if defined HAVE_QMI_MESSAGE_GAS_DMS_GET_FIRMWARE_LIST
    if (get_firmware_list_flag || get_active_firmware_flag) {
        QmiMessageGasDmsGetFirmwareListInput *input;

        input = qmi_message_gas_dms_get_firmware_list_input_new ();
        if (get_firmware_list_flag) {
            g_debug ("Asynchronously getting full firmware list...");
            qmi_message_gas_dms_get_firmware_list_input_set_mode (input, QMI_GAS_FIRMWARE_LISTING_MODE_ALL_FIRMWARE, NULL);
        } else if (get_active_firmware_flag) {
            g_debug ("Asynchronously getting active firmware list...");
            qmi_message_gas_dms_get_firmware_list_input_set_mode (input, QMI_GAS_FIRMWARE_LISTING_MODE_ACTIVE_FIRMWARE, NULL);
        } else
            g_assert_not_reached ();

        qmi_client_gas_dms_get_firmware_list (ctx->client,
                                              input,
                                              10,
                                              ctx->cancellable,
                                              (GAsyncReadyCallback)get_firmware_list_ready,
                                              NULL);
        qmi_message_gas_dms_get_firmware_list_input_unref (input);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_GAS_DMS_SET_ACTIVE_FIRMWARE
    if (set_active_firmware_int >= 0) {
        QmiMessageGasDmsSetActiveFirmwareInput *input;

        input = qmi_message_gas_dms_set_active_firmware_input_new ();
        qmi_message_gas_dms_set_active_firmware_input_set_slot_index (input, set_active_firmware_int, NULL);
        g_debug ("Asynchronously setting the active firmware index...");
        qmi_client_gas_dms_set_active_firmware (ctx->client,
                                                input,
                                                10,
                                                ctx->cancellable,
                                                (GAsyncReadyCallback)set_active_firmware_ready,
                                                NULL);
        qmi_message_gas_dms_set_active_firmware_input_unref (input);
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

#endif /* HAVE_QMI_SERVICE_GAS */
