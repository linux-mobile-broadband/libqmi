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
 * Copyright (C) 2015-2017 Aleksander Morgado <aleksander@aleksander.es>
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

#if defined HAVE_QMI_SERVICE_WMS

#define VALIDATE_UNKNOWN(str) (str ? str : "unknown")

/* Context */
typedef struct {
    QmiDevice *device;
    QmiClientWms *client;
    GCancellable *cancellable;
} Context;
static Context *ctx;

/* Options */
static gboolean get_supported_messages_flag;
static gboolean get_routes_flag;
static gboolean reset_flag;
static gboolean noop_flag;

static GOptionEntry entries[] = {
#if defined HAVE_QMI_MESSAGE_WMS_GET_SUPPORTED_MESSAGES
    { "wms-get-supported-messages", 0, 0, G_OPTION_ARG_NONE, &get_supported_messages_flag,
      "Get supported messages",
      NULL
    },
#endif
#if defined HAVE_QMI_MESSAGE_WMS_GET_ROUTES
    { "wms-get-routes", 0, 0, G_OPTION_ARG_NONE, &get_routes_flag,
      "Get SMS route information",
      NULL
    },
#endif
#if defined HAVE_QMI_MESSAGE_WMS_RESET
    { "wms-reset", 0, 0, G_OPTION_ARG_NONE, &reset_flag,
      "Reset the service state",
      NULL
    },
#endif
    { "wms-noop", 0, 0, G_OPTION_ARG_NONE, &noop_flag,
      "Just allocate or release a WMS client. Use with `--client-no-release-cid' and/or `--client-cid'",
      NULL
    },
    { NULL }
};

GOptionGroup *
qmicli_wms_get_option_group (void)
{
    GOptionGroup *group;

    group = g_option_group_new ("wms",
                                "WMS options:",
                                "Show Wireless Messaging Service options",
                                NULL,
                                NULL);
    g_option_group_add_entries (group, entries);

    return group;
}

gboolean
qmicli_wms_options_enabled (void)
{
    static guint n_actions = 0;
    static gboolean checked = FALSE;

    if (checked)
        return !!n_actions;

    n_actions = (get_supported_messages_flag +
                 get_routes_flag +
                 reset_flag +
                 noop_flag);

    if (n_actions > 1) {
        g_printerr ("error: too many WMS actions requested\n");
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

    if (context->client)
        g_object_unref (context->client);
    g_object_unref (context->cancellable);
    g_object_unref (context->device);
    g_slice_free (Context, context);
}

static void
operation_shutdown (gboolean operation_status)
{
    /* Cleanup context and finish async operation */
    context_free (ctx);
    qmicli_async_operation_done (operation_status, FALSE);
}

#if defined HAVE_QMI_MESSAGE_WMS_GET_SUPPORTED_MESSAGES

static void
get_supported_messages_ready (QmiClientWms *client,
                              GAsyncResult *res)
{
    QmiMessageWmsGetSupportedMessagesOutput *output;
    GError *error = NULL;
    GArray *bytearray = NULL;
    gchar *str;

    output = qmi_client_wms_get_supported_messages_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_wms_get_supported_messages_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get supported WMS messages: %s\n", error->message);
        g_error_free (error);
        qmi_message_wms_get_supported_messages_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] Successfully got supported WMS messages:\n",
             qmi_device_get_path_display (ctx->device));

    qmi_message_wms_get_supported_messages_output_get_list (output, &bytearray, NULL);
    str = qmicli_get_supported_messages_list (bytearray ? (const guint8 *)bytearray->data : NULL,
                                              bytearray ? bytearray->len : 0);
    g_print ("%s", str);
    g_free (str);

    qmi_message_wms_get_supported_messages_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_WMS_GET_SUPPORTED_MESSAGES */

#if defined HAVE_QMI_MESSAGE_WMS_GET_ROUTES

static void
get_routes_ready (QmiClientWms *client,
                  GAsyncResult *res)
{
    g_autoptr(QmiMessageWmsGetRoutesOutput) output = NULL;
    GError *error = NULL;
    GArray *route_list;
    guint i;

    output = qmi_client_wms_get_routes_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_wms_get_routes_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get SMS routes: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_wms_get_routes_output_get_route_list (output, &route_list, &error)) {
        g_printerr ("error: got invalid SMS routes: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] Got %u SMS routes:\n", qmi_device_get_path_display (ctx->device),
                                          route_list->len);

    for (i = 0; i < route_list->len; i++) {
        QmiMessageWmsGetRoutesOutputRouteListElement *route;

        route = &g_array_index (route_list, QmiMessageWmsGetRoutesOutputRouteListElement, i);
        g_print ("  Route #%u:\n", i + 1);
        g_print ("      Message Type: %s\n", VALIDATE_UNKNOWN (qmi_wms_message_type_get_string (route->message_type)));
        g_print ("     Message Class: %s\n", VALIDATE_UNKNOWN (qmi_wms_message_class_get_string (route->message_class)));
        g_print ("      Storage Type: %s\n", VALIDATE_UNKNOWN (qmi_wms_storage_type_get_string (route->storage)));
        g_print ("    Receipt Action: %s\n", VALIDATE_UNKNOWN (qmi_wms_receipt_action_get_string (route->receipt_action)));
    }

    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_WMS_GET_ROUTES */

#if defined HAVE_QMI_MESSAGE_WMS_RESET

static void
reset_ready (QmiClientWms *client,
             GAsyncResult *res)
{
    QmiMessageWmsResetOutput *output;
    GError *error = NULL;

    output = qmi_client_wms_reset_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_wms_reset_output_get_result (output, &error)) {
        g_printerr ("error: couldn't reset the WMS service: %s\n", error->message);
        g_error_free (error);
        qmi_message_wms_reset_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] Successfully performed WMS service reset\n",
             qmi_device_get_path_display (ctx->device));

    qmi_message_wms_reset_output_unref (output);
    operation_shutdown (TRUE);
}

#endif

static gboolean
noop_cb (gpointer unused)
{
    operation_shutdown (TRUE);
    return FALSE;
}

void
qmicli_wms_run (QmiDevice *device,
                QmiClientWms *client,
                GCancellable *cancellable)
{
    /* Initialize context */
    ctx = g_slice_new (Context);
    ctx->device = g_object_ref (device);
    ctx->client = g_object_ref (client);
    ctx->cancellable = g_object_ref (cancellable);

#if defined HAVE_QMI_MESSAGE_WMS_GET_SUPPORTED_MESSAGES
    if (get_supported_messages_flag) {
        g_debug ("Asynchronously getting supported WMS messages...");
        qmi_client_wms_get_supported_messages (ctx->client,
                                               NULL,
                                               10,
                                               ctx->cancellable,
                                               (GAsyncReadyCallback)get_supported_messages_ready,
                                               NULL);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_WMS_GET_ROUTES
    if (get_routes_flag) {
        g_debug ("Asynchronously getting SMS routes...");
        qmi_client_wms_get_routes (ctx->client,
                                   NULL,
                                   10,
                                   ctx->cancellable,
                                   (GAsyncReadyCallback)get_routes_ready,
                                   NULL);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_WMS_RESET
    if (reset_flag) {
        g_debug ("Asynchronously resetting WMS service...");
        qmi_client_wms_reset (ctx->client,
                              NULL,
                              10,
                              ctx->cancellable,
                              (GAsyncReadyCallback)reset_ready,
                              NULL);
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

#endif /* HAVE_QMI_SERVICE_WMS */
