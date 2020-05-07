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
 * Copyright (C) 2020 Vladimir Podshivalov <vladimir.podshivalov@outlook.com>
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

#if defined HAVE_QMI_SERVICE_GMS

/* Context */
typedef struct {
    QmiDevice *device;
    QmiClientGms *client;
    GCancellable *cancellable;
} Context;
static Context *ctx;

/* Options */
static gboolean get_value_flag;
static gchar    *set_value_str;
static gboolean noop_flag;

static GOptionEntry entries[] = {
#if defined HAVE_QMI_MESSAGE_GMS_TEST_GET_VALUE
    { "gms-test-get-value", 0, 0, G_OPTION_ARG_NONE, &get_value_flag,
      "Gets test value",
      NULL
    },
#endif
#if defined HAVE_QMI_MESSAGE_GMS_TEST_SET_VALUE
    { "gms-test-set-value", 0, 0, G_OPTION_ARG_STRING, &set_value_str,
      "Sets test value",
      "[mandatory-value][,[optional-value]]"
    },
#endif
    { "gms-noop", 0, 0, G_OPTION_ARG_NONE, &noop_flag,
      "Just allocate or release a GMS client. Use with `--client-no-release-cid' and/or `--client-cid'",
      NULL
    },
    { NULL }
};

GOptionGroup *
qmicli_gms_get_option_group (void)
{
    GOptionGroup *group;

    group = g_option_group_new ("gms",
                                "GMS options:",
                                "Show General Modem Service options",
                                NULL,
                                NULL);
    g_option_group_add_entries (group, entries);

    return group;
}

gboolean
qmicli_gms_options_enabled (void)
{
    static guint n_actions = 0;
    static gboolean checked = FALSE;

    if (checked)
        return !!n_actions;

    n_actions = (get_value_flag +
                 !!set_value_str +
                 noop_flag);

    if (n_actions > 1) {
        g_printerr ("error: too many GMS actions requested\n");
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

#if defined HAVE_QMI_MESSAGE_GMS_TEST_GET_VALUE

static void
get_value_ready (QmiClientGms *client,
                 GAsyncResult *res)
{
    QmiMessageGmsTestGetValueOutput *output;
    GError *error = NULL;
    guint8 test_mandatory_value;
    guint8 test_optional_value;

    output = qmi_client_gms_test_get_value_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_gms_test_get_value_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get stored test value: %s\n", error->message);
        g_error_free (error);
        qmi_message_gms_test_get_value_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    if (qmi_message_gms_test_get_value_output_get_test_mandatory_value (output, &test_mandatory_value, NULL)) {
        g_print ("Test mandatory value:     %u\n", test_mandatory_value);
    }

    if (qmi_message_gms_test_get_value_output_get_test_optional_value (output, &test_optional_value, NULL)) {
        g_print ("Test optional value:      %u\n", test_optional_value);
    }

    qmi_message_gms_test_get_value_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_GMS_TEST_GET_VALUE */

#if defined HAVE_QMI_MESSAGE_GMS_TEST_SET_VALUE

static QmiMessageGmsTestSetValueInput *
set_value_input_create (const gchar *str)
{
    QmiMessageGmsTestSetValueInput *input = NULL;
    const gchar *mand_value_str = NULL;
    const gchar *opt_value_str = NULL;
    guint mand_value_int = 0;
    guint opt_value_int = 0;
    gchar **parts = NULL;

    if (strchr (str, ',')) {
        /* Both Mandatory Test Value and Optional Test Value were given */
        parts = g_strsplit_set (str, ",", -1);
        if (g_strv_length (parts) != 2) {
            g_printerr ("error: failed to parse test value: '%s'\n", str);
            g_strfreev (parts);
            goto out;
        }
        mand_value_str = parts[0];
        opt_value_str  = parts[1];
    } else {
        /* Only Mandatory Test Value was given */
        mand_value_str = str;
    }

    g_assert (mand_value_str);
    if (!qmicli_read_uint_from_string (mand_value_str, &mand_value_int) || (mand_value_int > G_MAXUINT8)) {
        g_printerr ("error: failed to parse test mandatory value as 8bit value: '%s'\n", mand_value_str);
        goto out;
    }

    if (opt_value_str && (!qmicli_read_uint_from_string (opt_value_str, &opt_value_int) || (opt_value_int > G_MAXUINT8))) {
        g_printerr ("error: failed to parse test optional value as 8bit value: '%s'\n", opt_value_str);
        goto out;
    }

    input = qmi_message_gms_test_set_value_input_new ();

    qmi_message_gms_test_set_value_input_set_test_mandatory_value (input, (guint8) mand_value_int, NULL);
    if (opt_value_str) {
        qmi_message_gms_test_set_value_input_set_test_optional_value (input, (guint8) opt_value_int, NULL);
    }

out:
    g_strfreev (parts);
    return input;
}

static void
set_value_ready (QmiClientGms *client,
                 GAsyncResult *res)
{
    QmiMessageGmsTestSetValueOutput *output;
    GError *error = NULL;

    output = qmi_client_gms_test_set_value_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_gms_test_set_value_output_get_result (output, &error)) {
        g_printerr ("error: couldn't set test value: %s\n", error->message);
        g_error_free (error);
        qmi_message_gms_test_set_value_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("Successfully set test value.\n");

    qmi_message_gms_test_set_value_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_GMS_TEST_SET_VALUE */

static gboolean
noop_cb (gpointer unused)
{
    operation_shutdown (TRUE);
    return FALSE;
}

void
qmicli_gms_run (QmiDevice *device,
                QmiClientGms *client,
                GCancellable *cancellable)
{
    /* Initialize context */
    ctx = g_slice_new (Context);
    ctx->device = g_object_ref (device);
    ctx->client = g_object_ref (client);
    ctx->cancellable = g_object_ref (cancellable);

#if defined HAVE_QMI_MESSAGE_GMS_TEST_GET_VALUE
    if (get_value_flag) {
        g_debug ("Asynchronously getting test value...");
        qmi_client_gms_test_get_value (ctx->client,
                                       NULL,
                                       10,
                                       ctx->cancellable,
                                       (GAsyncReadyCallback)get_value_ready,
                                       NULL);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_GMS_TEST_SET_VALUE
    if (set_value_str) {
        QmiMessageGmsTestSetValueInput *input;
        g_debug ("Asynchronously setting test value...");

        input = set_value_input_create (set_value_str);
        if (!input) {
            operation_shutdown (FALSE);
            return;
        }

        qmi_client_gms_test_set_value (ctx->client,
                                       input,
                                       10,
                                       ctx->cancellable,
                                       (GAsyncReadyCallback)set_value_ready,
                                       NULL);

        qmi_message_gms_test_set_value_input_unref (input);
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

#endif /* HAVE_QMI_SERVICE_GMS */
