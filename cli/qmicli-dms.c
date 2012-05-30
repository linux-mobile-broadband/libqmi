/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * qmicli -- Command line interface to control QMI devices
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
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
 * Copyright (C) 2012 Aleksander Morgado <aleksander@gnu.org>
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

/* Context */
typedef struct {
    QmiDevice *device;
    QmiClientDms *client;
    GCancellable *cancellable;
} Context;
static Context *ctx;

/* Options */
static gboolean get_ids_flag;
static gboolean get_capabilities_flag;
static gboolean get_manufacturer_flag;
static gboolean get_model_flag;
static gboolean noop_flag;

static GOptionEntry entries[] = {
    { "dms-get-ids", 0, 0, G_OPTION_ARG_NONE, &get_ids_flag,
      "Get IDs",
      NULL
    },
    { "dms-get-capabilities", 0, 0, G_OPTION_ARG_NONE, &get_capabilities_flag,
      "Get capabilities",
      NULL
    },
    { "dms-get-manufacturer", 0, 0, G_OPTION_ARG_NONE, &get_manufacturer_flag,
      "Get manufacturer",
      NULL
    },
    { "dms-get-model", 0, 0, G_OPTION_ARG_NONE, &get_model_flag,
      "Get model",
      NULL
    },
    { "dms-noop", 0, 0, G_OPTION_ARG_NONE, &noop_flag,
      "Just allocate or release a DMS client. Use with `--client-no-release-cid' and/or `--client-cid'",
      NULL
    },
    { NULL }
};

GOptionGroup *
qmicli_dms_get_option_group (void)
{
	GOptionGroup *group;

	group = g_option_group_new ("dms",
	                            "DMS options",
	                            "Show Device Management Service options",
	                            NULL,
	                            NULL);
	g_option_group_add_entries (group, entries);

	return group;
}

gboolean
qmicli_dms_options_enabled (void)
{
    static guint n_actions = 0;
    static gboolean checked = FALSE;

    if (checked)
        return !!n_actions;

    n_actions = (get_ids_flag +
                 get_capabilities_flag +
                 get_manufacturer_flag +
                 get_model_flag +
                 noop_flag);

    if (n_actions > 1) {
        g_printerr ("error: too many DMS actions requested\n");
        exit (EXIT_FAILURE);
    }

    checked = TRUE;
    return !!n_actions;
}

static void
context_free (Context *ctx)
{
    if (!ctx)
        return;

    if (ctx->cancellable)
        g_object_unref (ctx->cancellable);
    if (ctx->device)
        g_object_unref (ctx->device);
    if (ctx->client)
        g_object_unref (ctx->client);
    g_slice_free (Context, ctx);
}

static void
shutdown (gboolean operation_status)
{
    /* Cleanup context and finish async operation */
    context_free (ctx);
    qmicli_async_operation_done (operation_status);
}

static void
get_ids_ready (QmiClientDms *client,
               GAsyncResult *res)
{
    const gchar *esn = NULL;
    const gchar *imei = NULL;
    const gchar *meid = NULL;
    QmiMessageDmsGetIdsOutput *output;
    GError *error = NULL;

    output = qmi_client_dms_get_ids_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_get_ids_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get IDs: %s\n", error->message);
        g_error_free (error);
        qmi_message_dms_get_ids_output_unref (output);
        shutdown (FALSE);
        return;
    }

#undef VALIDATE_UNKNOWN
#define VALIDATE_UNKNOWN(str) (str ? str : "unknown")

    qmi_message_dms_get_ids_output_get_esn  (output, &esn, NULL);
    qmi_message_dms_get_ids_output_get_imei (output, &imei, NULL);
    qmi_message_dms_get_ids_output_get_meid (output, &meid, NULL);

    g_print ("[%s] Device IDs retrieved:\n"
             "\t ESN: '%s'\n"
             "\tIMEI: '%s'\n"
             "\tMEID: '%s'\n",
             qmi_device_get_path_display (ctx->device),
             VALIDATE_UNKNOWN (esn),
             VALIDATE_UNKNOWN (imei),
             VALIDATE_UNKNOWN (meid));

    qmi_message_dms_get_ids_output_unref (output);
    shutdown (TRUE);
}

static void
get_capabilities_ready (QmiClientDms *client,
                        GAsyncResult *res)
{
    QmiMessageDmsGetCapabilitiesOutputInfo info;
    QmiMessageDmsGetCapabilitiesOutput *output;
    GError *error = NULL;
    GString *networks;
    guint i;

    output = qmi_client_dms_get_capabilities_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_get_capabilities_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get capabilities: %s\n", error->message);
        g_error_free (error);
        qmi_message_dms_get_capabilities_output_unref (output);
        shutdown (FALSE);
        return;
    }

    qmi_message_dms_get_capabilities_output_get_info (output, &info, NULL);

    networks = g_string_new ("");
    for (i = 0; i < info.radio_interface_list->len; i++) {
        g_string_append (networks,
                         qmi_dms_radio_interface_get_string (
                             g_array_index (info.radio_interface_list,
                                            QmiDmsRadioInterface,
                                            i)));
        if (i != info.radio_interface_list->len - 1)
            g_string_append (networks, ", ");
    }

    g_print ("[%s] Device capabilities retrieved:\n"
             "\tMax TX channel rate: '%u'\n"
             "\tMax RX channel rate: '%u'\n"
             "\t       Data Service: '%s'\n"
             "\t                SIM: '%s'\n"
             "\t           Networks: '%s'\n",
             qmi_device_get_path_display (ctx->device),
             info.max_tx_channel_rate,
             info.max_rx_channel_rate,
             qmi_dms_data_service_capability_get_string (info.data_service_capability),
             qmi_dms_sim_capability_get_string (info.sim_capability),
             networks->str);

    g_string_free (networks, TRUE);
    qmi_message_dms_get_capabilities_output_unref (output);
    shutdown (TRUE);
}

static void
get_manufacturer_ready (QmiClientDms *client,
                        GAsyncResult *res)
{
    const gchar *str = NULL;
    QmiMessageDmsGetManufacturerOutput *output;
    GError *error = NULL;

    output = qmi_client_dms_get_manufacturer_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_get_manufacturer_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get manufacturer: %s\n", error->message);
        g_error_free (error);
        qmi_message_dms_get_manufacturer_output_unref (output);
        shutdown (FALSE);
        return;
    }

#undef VALIDATE_UNKNOWN
#define VALIDATE_UNKNOWN(str) (str ? str : "unknown")

    qmi_message_dms_get_manufacturer_output_get_manufacturer (output, &str, NULL);

    g_print ("[%s] Device manufacturer retrieved:\n"
             "\tManufacturer: '%s'\n",
             qmi_device_get_path_display (ctx->device),
             VALIDATE_UNKNOWN (str));

    qmi_message_dms_get_manufacturer_output_unref (output);
    shutdown (TRUE);
}

static void
get_model_ready (QmiClientDms *client,
                 GAsyncResult *res)
{
    const gchar *str = NULL;
    QmiMessageDmsGetModelOutput *output;
    GError *error = NULL;

    output = qmi_client_dms_get_model_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_get_model_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get model: %s\n", error->message);
        g_error_free (error);
        qmi_message_dms_get_model_output_unref (output);
        shutdown (FALSE);
        return;
    }

#undef VALIDATE_UNKNOWN
#define VALIDATE_UNKNOWN(str) (str ? str : "unknown")

    qmi_message_dms_get_model_output_get_model (output, &str, NULL);

    g_print ("[%s] Device model retrieved:\n"
             "\tModel: '%s'\n",
             qmi_device_get_path_display (ctx->device),
             VALIDATE_UNKNOWN (str));

    qmi_message_dms_get_model_output_unref (output);
    shutdown (TRUE);
}
static gboolean
noop_cb (gpointer unused)
{
    shutdown (TRUE);
    return FALSE;
}

void
qmicli_dms_run (QmiDevice *device,
                QmiClientDms *client,
                GCancellable *cancellable)
{
    /* Initialize context */
    ctx = g_slice_new (Context);
    ctx->device = g_object_ref (device);
    ctx->client = g_object_ref (client);
    if (cancellable)
        ctx->cancellable = g_object_ref (cancellable);

    /* Request to get IDs? */
    if (get_ids_flag) {
        g_debug ("Asynchronously getting IDs...");
        qmi_client_dms_get_ids (ctx->client,
                                NULL,
                                10,
                                ctx->cancellable,
                                (GAsyncReadyCallback)get_ids_ready,
                                NULL);
        return;
    }

    /* Request to get capabilities? */
    if (get_capabilities_flag) {
        g_debug ("Asynchronously getting capabilities...");
        qmi_client_dms_get_capabilities (ctx->client,
                                         NULL,
                                         10,
                                         ctx->cancellable,
                                         (GAsyncReadyCallback)get_capabilities_ready,
                                         NULL);
        return;
    }

    /* Request to get manufacturer? */
    if (get_manufacturer_flag) {
        g_debug ("Asynchronously getting manufacturer...");
        qmi_client_dms_get_manufacturer (ctx->client,
                                         NULL,
                                         10,
                                         ctx->cancellable,
                                         (GAsyncReadyCallback)get_manufacturer_ready,
                                         NULL);
        return;
    }

    /* Request to get model? */
    if (get_model_flag) {
        g_debug ("Asynchronously getting model...");
        qmi_client_dms_get_model (ctx->client,
                                  NULL,
                                  10,
                                  ctx->cancellable,
                                  (GAsyncReadyCallback)get_model_ready,
                                  NULL);
        return;
    }

    /* Just client allocate/release? */
    if (noop_flag) {
        g_idle_add (noop_cb, NULL);
        return;
    }

    g_warn_if_reached ();
}
