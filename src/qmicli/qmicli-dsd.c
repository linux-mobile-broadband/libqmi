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
 * Copyright (C) 2019 Aleksander Morgado <aleksander@aleksander.es>
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

#if defined HAVE_QMI_SERVICE_DSD

/* Context */
typedef struct {
    QmiDevice *device;
    QmiClientDsd *client;
    GCancellable *cancellable;
} Context;
static Context *ctx;

/* Options */
static gchar    *get_apn_info_str;
static gchar    *set_apn_type_str;
static gboolean  noop_flag;

static GOptionEntry entries[] = {
#if defined HAVE_QMI_MESSAGE_DSD_GET_APN_INFO
    { "dsd-get-apn-info", 0, 0, G_OPTION_ARG_STRING, &get_apn_info_str,
      "Gets the settings associated to a given APN type",
      "[(type)]"
    },
#endif
#if defined HAVE_QMI_MESSAGE_DSD_SET_APN_TYPE
    { "dsd-set-apn-type", 0, 0, G_OPTION_ARG_STRING, &set_apn_type_str,
      "Sets the types associated to a given APN name",
      "[(name), (type1|type2|type3...)]"
    },
#endif
    { "dsd-noop", 0, 0, G_OPTION_ARG_NONE, &noop_flag,
      "Just allocate or release a DSD client. Use with `--client-no-release-cid' and/or `--client-cid'",
      NULL
    },
    { NULL }
};

GOptionGroup *
qmicli_dsd_get_option_group (void)
{
    GOptionGroup *group;

    group = g_option_group_new ("dsd",
                                "DSD options:",
                                "Show Data System Determination options",
                                NULL,
                                NULL);
    g_option_group_add_entries (group, entries);

    return group;
}

gboolean
qmicli_dsd_options_enabled (void)
{
    static guint n_actions = 0;
    static gboolean checked = FALSE;

    if (checked)
        return !!n_actions;

    n_actions = (!!get_apn_info_str +
                 !!set_apn_type_str +
                 noop_flag);

    if (n_actions > 1) {
        g_printerr ("error: too many DSD actions requested\n");
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

#if defined HAVE_QMI_MESSAGE_DSD_GET_APN_INFO

static void
get_apn_info_ready (QmiClientDsd *client,
                    GAsyncResult *res)
{
    QmiMessageDsdGetApnInfoOutput *output;
    GError *error = NULL;
    const gchar *apn_name = NULL;

    output = qmi_client_dsd_get_apn_info_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_dsd_get_apn_info_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get APN info: %s\n", error->message);
        g_error_free (error);
        qmi_message_dsd_get_apn_info_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("APN info found:\n");
    if (qmi_message_dsd_get_apn_info_output_get_apn_name (output, &apn_name, NULL))
        g_print ("APN name: %s\n", apn_name);
    else
        g_print ("APN name: n/a\n");

    qmi_message_dsd_get_apn_info_output_unref (output);
    operation_shutdown (TRUE);
}

static QmiMessageDsdGetApnInfoInput *
get_apn_info_input_create (const gchar *str)
{
    QmiMessageDsdGetApnInfoInput *input = NULL;
    GError                       *error = NULL;
    QmiDsdApnType                 apn_type;

    if (!qmicli_read_dsd_apn_type_from_string (str, &apn_type)) {
        g_printerr ("error: couldn't parse input string as APN type: '%s'\n", str);
        return NULL;
    }

    input = qmi_message_dsd_get_apn_info_input_new ();
    if (!qmi_message_dsd_get_apn_info_input_set_apn_type (input, apn_type, &error)) {
        g_printerr ("error: couldn't create input data bundle: '%s'\n",
                    error->message);
        g_error_free (error);
        qmi_message_dsd_get_apn_info_input_unref (input);
        input = NULL;
    }

    return input;
}

#endif /* HAVE_QMI_MESSAGE_DSD_GET_APN_INFO */

#if defined HAVE_QMI_MESSAGE_DSD_SET_APN_TYPE

static void
set_apn_type_ready (QmiClientDsd *client,
                    GAsyncResult *res)
{
    QmiMessageDsdSetApnTypeOutput *output;
    GError *error = NULL;

    output = qmi_client_dsd_set_apn_type_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_dsd_set_apn_type_output_get_result (output, &error)) {
        g_printerr ("error: couldn't set APN type: %s\n", error->message);
        g_error_free (error);
        qmi_message_dsd_set_apn_type_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("APN type set\n");

    qmi_message_dsd_set_apn_type_output_unref (output);
    operation_shutdown (TRUE);
}

static QmiMessageDsdSetApnTypeInput *
set_apn_type_input_create (const gchar *str)
{
    QmiMessageDsdSetApnTypeInput  *input = NULL;
    GError                        *error = NULL;
    QmiDsdApnTypePreference        apn_type_preference;
    gchar                        **split;

    split = g_strsplit_set (str, ",", 0);
    if (g_strv_length (split) != 2) {
        g_printerr ("input string requires 2 values, %u given: '%s'\n", g_strv_length (split), str);
        g_strfreev (split);
        return NULL;
    }

    g_strstrip (split[0]);
    g_strstrip (split[1]);

    if (!qmicli_read_dsd_apn_type_preference_from_string (split[1], &apn_type_preference)) {
        g_printerr ("error: couldn't parse input string as APN type preference mask: '%s'\n", split[1]);
        g_strfreev (split);
        return NULL;
    }

    input = qmi_message_dsd_set_apn_type_input_new ();
    if (!qmi_message_dsd_set_apn_type_input_set_apn_type (input, split[0], apn_type_preference, &error)) {
        g_printerr ("error: couldn't create input data bundle: '%s'\n",
                    error->message);
        g_error_free (error);
        qmi_message_dsd_set_apn_type_input_unref (input);
        input = NULL;
    }

    g_strfreev (split);

    return input;
}

#endif /* HAVE_QMI_MESSAGE_DSD_SET_APN_TYPE */

static gboolean
noop_cb (gpointer unused)
{
    operation_shutdown (TRUE);
    return FALSE;
}

void
qmicli_dsd_run (QmiDevice    *device,
                QmiClientDsd *client,
                GCancellable *cancellable)
{
    /* Initialize context */
    ctx = g_slice_new (Context);
    ctx->device = g_object_ref (device);
    ctx->client = g_object_ref (client);
    ctx->cancellable = g_object_ref (cancellable);

#if defined HAVE_QMI_MESSAGE_DSD_GET_APN_INFO
    if (get_apn_info_str) {
        QmiMessageDsdGetApnInfoInput *input;

        g_debug ("Asynchronously getting APN info...");
        input = get_apn_info_input_create (get_apn_info_str);
        if (!input) {
            operation_shutdown (FALSE);
            return;
        }
        qmi_client_dsd_get_apn_info (ctx->client,
                                     input,
                                     10,
                                     ctx->cancellable,
                                     (GAsyncReadyCallback)get_apn_info_ready,
                                     NULL);
        qmi_message_dsd_get_apn_info_input_unref (input);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_DSD_SET_APN_TYPE
    if (set_apn_type_str) {
        QmiMessageDsdSetApnTypeInput *input;

        g_debug ("Asynchronously setting APN type...");
        input = set_apn_type_input_create (set_apn_type_str);
        if (!input) {
            operation_shutdown (FALSE);
            return;
        }
        qmi_client_dsd_set_apn_type (ctx->client,
                                     input,
                                     10,
                                     ctx->cancellable,
                                     (GAsyncReadyCallback)set_apn_type_ready,
                                     NULL);
        qmi_message_dsd_set_apn_type_input_unref (input);
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

#endif /* HAVE_QMI_SERVICE_DSD */
