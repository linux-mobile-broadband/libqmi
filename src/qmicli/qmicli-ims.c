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
 * Copyright (C) 2023 Dylan Van Assche <me@dylanvanassche.be>
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

#if defined HAVE_QMI_SERVICE_IMS

/* Context */
typedef struct {
    QmiDevice *device;
    QmiClientIms *client;
    GCancellable *cancellable;
} Context;
static Context *ctx;

/* Options */
static gint bind_flag = -1;
static gboolean get_services_enabled_flag;
static gboolean noop_flag;

static GOptionEntry entries[] = {
#if defined HAVE_QMI_MESSAGE_IMS_BIND
    { "ims-bind", 0, 0, G_OPTION_ARG_INT, &bind_flag,
      "Bind to IMS Settings (use with --client-no-release-cid)",
      "[binding]"
    },
#endif
#if defined HAVE_QMI_MESSAGE_IMS_GET_IMS_SERVICES_ENABLED_SETTING
    { "ims-get-ims-services-enabled-setting", 0, 0, G_OPTION_ARG_NONE, &get_services_enabled_flag,
      "Get IMS Services Enabled Setting",
      NULL
    },
#endif
    { "ims-noop", 0, 0, G_OPTION_ARG_NONE, &noop_flag,
      "Just allocate or release a IMS client. Use with `--client-no-release-cid' and/or `--client-cid'",
      NULL
    },
    { NULL }
};

GOptionGroup *
qmicli_ims_get_option_group (void)
{
    GOptionGroup *group;

    group = g_option_group_new ("ims",
                                "IMS options:",
                                "Show IP Multimedia Subsystem Settings Service options",
                                NULL,
                                NULL);
    g_option_group_add_entries (group, entries);

    return group;
}

gboolean
qmicli_ims_options_enabled (void)
{
    static guint n_actions = 0;
    static gboolean checked = FALSE;

    if (checked)
        return !!n_actions;

    n_actions = ((bind_flag >= 0) +
                 get_services_enabled_flag +
                 noop_flag);

    if (n_actions > 1) {
        g_printerr ("error: too many IMS actions requested\n");
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

#if defined HAVE_QMI_MESSAGE_IMS_BIND

static void
bind_ready (QmiClientIms *client,
            GAsyncResult *res)
{
    QmiMessageImsBindOutput *output;
    g_autoptr(GError) error = NULL;

    output = qmi_client_ims_bind_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_ims_bind_output_get_result (output, &error)) {
        g_printerr ("error: couldn't bind to IMS Settings: %s\n", error->message);
        qmi_message_ims_bind_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] IMS Settings bind successful\n", qmi_device_get_path_display (ctx->device));

    qmi_message_ims_bind_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_IMS_BIND */

#if defined HAVE_QMI_MESSAGE_IMS_GET_IMS_SERVICES_ENABLED_SETTING

static void
get_services_enabled_ready (QmiClientIms *client,
                            GAsyncResult *res)
{
    QmiMessageImsGetImsServicesEnabledSettingOutput *output;
    gboolean service_voice_enabled;
    gboolean service_vt_enabled;
    gboolean service_voice_wifi_enabled;
    gboolean service_ims_registration_enabled;
    gboolean service_ut_enabled;
    gboolean service_sms_enabled;
    gboolean service_ussd_enabled;
    GError *error = NULL;

    output = qmi_client_ims_get_ims_services_enabled_setting_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_ims_get_ims_services_enabled_setting_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get IMS services enabled setting: %s\n", error->message);
        g_error_free (error);
    	qmi_message_ims_get_ims_services_enabled_setting_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] IMS services:\n", qmi_device_get_path_display (ctx->device));

    if (qmi_message_ims_get_ims_services_enabled_setting_output_get_ims_voice_service_enabled (output, &service_voice_enabled, NULL))
        g_print ("\t       IMS registration enabled: %s\n", service_ims_registration_enabled? "yes" : "no");

    if (qmi_message_ims_get_ims_services_enabled_setting_output_get_ims_video_telephony_service_enabled (output, &service_vt_enabled, NULL))
        g_print ("\t          Voice service enabled: %s\n", service_voice_enabled? "yes" : "no");

    if (qmi_message_ims_get_ims_services_enabled_setting_output_get_ims_voice_wifi_service_enabled (output, &service_voice_wifi_enabled, NULL))
        g_print ("\t     Voice WiFi service enabled: %s\n", service_voice_wifi_enabled? "yes" : "no");

    if (qmi_message_ims_get_ims_services_enabled_setting_output_get_ims_registration_service_enabled (output, &service_ims_registration_enabled, NULL))
        g_print ("\tVideo Telephony service enabled: %s\n", service_vt_enabled? "yes" : "no");
 
    if (qmi_message_ims_get_ims_services_enabled_setting_output_get_ims_ut_service_enabled (output, &service_ut_enabled, NULL))
        g_print ("\t      UE to TAS service enabled: %s\n", service_ut_enabled? "yes" : "no");

    if (qmi_message_ims_get_ims_services_enabled_setting_output_get_ims_sms_service_enabled (output, &service_sms_enabled, NULL))
        g_print ("\t            SMS service enabled: %s\n", service_sms_enabled? "yes" : "no");

    if (qmi_message_ims_get_ims_services_enabled_setting_output_get_ims_ussd_service_enabled (output, &service_ussd_enabled, NULL))
        g_print ("\t           USSD service enabled: %s\n", service_ut_enabled? "yes" : "no");

    qmi_message_ims_get_ims_services_enabled_setting_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_IMS_GET_IMS_SERVICES_ENABLED_SETTING */

static gboolean
noop_cb (gpointer unused)
{
    operation_shutdown (TRUE);
    return FALSE;
}

void
qmicli_ims_run (QmiDevice *device,
                QmiClientIms *client,
                GCancellable *cancellable)
{
    /* Initialize context */
    ctx = g_slice_new (Context);
    ctx->device = g_object_ref (device);
    ctx->client = g_object_ref (client);
    ctx->cancellable = g_object_ref (cancellable);

#if defined HAVE_QMI_MESSAGE_IMS_BIND
    if (bind_flag >= 0) {
        QmiMessageImsBindInput *input;

        input = qmi_message_ims_bind_input_new ();
        qmi_message_ims_bind_input_set_binding (input, bind_flag, NULL);
        g_debug ("Asynchronously binding to IMS settings service...");
        qmi_client_ims_bind (ctx->client,
                             input,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback)bind_ready,
                             NULL);
        qmi_message_ims_bind_input_unref (input);
        return;
    }
#endif /* HAVE_QMI_MESSAGE_IMS_BIND */
#if defined HAVE_QMI_MESSAGE_IMS_GET_IMS_SERVICES_ENABLED_SETTING
    if (get_services_enabled_flag) {
        g_debug ("Asynchronously getting services enabled setting...");

        qmi_client_ims_get_ims_services_enabled_setting (ctx->client,
                                                         NULL,
                                                         10,
                                                         ctx->cancellable,
                                                         (GAsyncReadyCallback)get_services_enabled_ready,
                                                         NULL);
        return;
    }
#endif /* HAVE_QMI_MESSAGE_IMS_GET_IMS_SERVICES_ENABLED_SETTING */

    /* Just client allocate/release? */
    if (noop_flag) {
        g_idle_add (noop_cb, NULL);
        return;
    }

    g_warn_if_reached ();
}

#endif /* HAVE_QMI_SERVICE_IMS */

