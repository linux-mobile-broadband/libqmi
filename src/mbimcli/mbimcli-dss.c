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
 * Copyright (C) 2014 NVIDIA CORPORATION
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
static gchar    *set_connect_activate_str;
static gchar    *set_connect_deactivate_str;

static GOptionEntry entries[] = {
    { "dss-connect", 0, 0, G_OPTION_ARG_STRING, &set_connect_activate_str,
      "DSS Connect (DeviceServiceId, DssSessionId)",
      "[(UUID),(Session)]"
    },
    { "dss-disconnect", 0, 0, G_OPTION_ARG_STRING, &set_connect_deactivate_str,
      "DSS Disconnect (DeviceServiceId, DssSessionId)",
      "[(UUID),(Session)]"
    },
    { NULL }
};

GOptionGroup *
mbimcli_dss_get_option_group (void)
{
    GOptionGroup *group;

    group = g_option_group_new ("dss",
                                "Device Service Stream options",
                                "Show Device Service Stream options",
                                NULL,
                                NULL);
    g_option_group_add_entries (group, entries);

	return group;
}

gboolean
mbimcli_dss_options_enabled (void)
{
    static guint n_actions = 0;
    static gboolean checked = FALSE;

    if (checked)
        return !!n_actions;

    n_actions = (!!set_connect_activate_str +
                 !!set_connect_deactivate_str );

    if (n_actions > 1) {
        g_printerr ("error: too many DSS actions requested\n");
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

enum {
    CONNECT,
    DISCONNECT
};

static void
set_dss_ready (MbimDevice   *device,
               GAsyncResult *res,
               gpointer user_data)
{
    MbimMessage *response;
    GError *error = NULL;

    response = mbim_device_command_finish (device, res, &error);
    if (!response || !mbim_message_command_done_get_result (response, &error)) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        shutdown (FALSE);
        return;
    }

    if (!mbim_message_dss_connect_response_parse (
            response,
            &error)) {
        g_printerr ("error: couldn't parse response message: %s\n", error->message);
        g_error_free (error);
        shutdown (FALSE);
        return;
    }

    switch (GPOINTER_TO_UINT (user_data)) {
    case CONNECT:
        g_print ("[%s] Successfully connected\n\n",
                 mbim_device_get_path_display (device));
        break;
    case DISCONNECT:
        g_print ("[%s] Successfully disconnected\n\n",
                 mbim_device_get_path_display (device));
        break;
    default:
        break;
    }

    mbim_message_unref (response);
    shutdown (TRUE);
}

static gboolean parse_uuid(const gchar *str, MbimUuid* uuid)
{
    guint a0, a1, a2, a3;
    guint b0, b1;
    guint c0, c1;
    guint d0, d1;
    guint e0, e1, e2, e3, e4, e5;

    if ( (strlen(str) != 36) ||
         (0 == sscanf(str, "%02x%02x%02x%02x-"
                           "%02x%02x-"
                           "%02x%02x-"
                           "%02x%02x-"
                           "%02x%02x%02x%02x%02x%02x",
                           &a0, &a1, &a2, &a3,
                           &b0, &b1,
                           &c0, &c1,
                           &d0, &d1,
                           &e0, &e1, &e2, &e3, &e4, &e5)) )
        return FALSE;

    uuid->a[0] = a0; uuid->a[1] = a1; uuid->a[2] = a2; uuid->a[3] = a3;
    uuid->b[0] = b0; uuid->b[1] = b1;
    uuid->c[0] = c0; uuid->c[1] = c1;
    uuid->d[0] = d0; uuid->d[1] = d1;
    uuid->e[0] = e0; uuid->e[1] = e1; uuid->e[2] = e2; uuid->e[3] = e3; uuid->e[4] = e4; uuid->e[5] = e5;
    return TRUE;
}

static gboolean parse_uint(const gchar *str, guint32 *u)
{
    guint32 t;

    if (0 == sscanf(str, "%u", &t))
        return FALSE;

    *u = t;
    return TRUE;
}

static gboolean
set_dss_command_parse (const gchar *str,
                       MbimUuid    *dsid,
                       guint32     *ssid)
{
    gchar **split;

    g_assert (dsid != NULL);
    g_assert (ssid != NULL);

    /* Format of the string is:
     * [(DevSrvID),(SessionId)]
     */
    split = g_strsplit (str, ",", -1);

    if (g_strv_length (split) > 2) {
        g_printerr ("error: couldn't parse input string, too many arguments\n");
        g_strfreev (split);
        return FALSE;
    }

    if (g_strv_length (split) < 1) {
        g_printerr ("error: couldn't parse input string, missing arguments\n");
        g_strfreev (split);
        return FALSE;
    }

    /* DeviceServiceId */
    if (parse_uuid(split[0], dsid) == FALSE) {
        g_printerr ("error: couldn't parse UUID, should be xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx\n");
        g_strfreev (split);
        return FALSE;
    }

    /* SessionId */
    if (parse_uint(split[1], ssid) == FALSE) {
        g_printerr ("error: couldn't parse Session ID, should be a number\n");
        g_strfreev (split);
        return FALSE;
    }

    g_strfreev (split);
    return TRUE;
}

void
mbimcli_dss_run (MbimDevice   *device,
                 GCancellable *cancellable)
{
    MbimMessage *request;
    GError *error = NULL;
    MbimUuid service_id;
    guint32  session_id;

    /* Initialize context */
    ctx = g_slice_new (Context);
    ctx->device = g_object_ref (device);
    if (cancellable)
        ctx->cancellable = g_object_ref (cancellable);

    /* Connect? */
    if (set_connect_activate_str) 
    {
        if (!set_dss_command_parse (set_connect_activate_str,
                                    &service_id,
                                    &session_id)) {
            shutdown (FALSE);
            return;
        }

        request = mbim_message_dss_connect_set_new (&service_id,
                                                    session_id,
                                                    MBIM_DSS_LINK_STATE_ACTIVATE,
                                                    &error);

        if (!request) {
            g_printerr ("error: couldn't create request: %s\n", error->message);
            g_error_free (error);
            shutdown (FALSE);
            return;
        }

        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)set_dss_ready,
                             GUINT_TO_POINTER (CONNECT));
        mbim_message_unref (request);
        return;
    }

    /* Disconnect? */
    if (set_connect_deactivate_str) 
    {
        if (!set_dss_command_parse (set_connect_deactivate_str,
                                    &service_id,
                                    &session_id)) {
            shutdown (FALSE);
            return;
        }

        request = mbim_message_dss_connect_set_new (&service_id,
                                                    session_id,
                                                    MBIM_DSS_LINK_STATE_DEACTIVATE,
                                                    &error);
        if (!request) {
            g_printerr ("error: couldn't create request: %s\n", error->message);
            g_error_free (error);
            shutdown (FALSE);
            return;
        }

        mbim_device_command (ctx->device,
                             request,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)set_dss_ready,
                             GUINT_TO_POINTER (DISCONNECT));
        mbim_message_unref (request);
        return;
    }

    g_warn_if_reached ();
}
