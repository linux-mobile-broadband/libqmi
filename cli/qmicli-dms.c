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
static gboolean get_revision_flag;
static gboolean get_msisdn_flag;
static gboolean get_power_state_flag;
static gchar *set_pin_protection_str;
static gchar *verify_pin_str;
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
    { "dms-get-revision", 0, 0, G_OPTION_ARG_NONE, &get_revision_flag,
      "Get revision",
      NULL
    },
    { "dms-get-msisdn", 0, 0, G_OPTION_ARG_NONE, &get_msisdn_flag,
      "Get MSISDN",
      NULL
    },
    { "dms-get-power-state", 0, 0, G_OPTION_ARG_NONE, &get_power_state_flag,
      "Get power state",
      NULL
    },
    { "dms-set-pin-protection", 0, 0, G_OPTION_ARG_STRING, &set_pin_protection_str,
      "Set PIN protection",
      "[(PIN|PIN2),(disable|enable),(current PIN)]",
    },
    { "dms-verify-pin", 0, 0, G_OPTION_ARG_STRING, &verify_pin_str,
      "Verify PIN",
      "[(PIN|PIN2),(current PIN)]",
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
                 get_revision_flag +
                 get_msisdn_flag +
                 get_power_state_flag +
                 !!set_pin_protection_str +
                 !!verify_pin_str +
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

static void
get_revision_ready (QmiClientDms *client,
                    GAsyncResult *res)
{
    const gchar *str = NULL;
    QmiMessageDmsGetRevisionOutput *output;
    GError *error = NULL;

    output = qmi_client_dms_get_revision_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_get_revision_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get revision: %s\n", error->message);
        g_error_free (error);
        qmi_message_dms_get_revision_output_unref (output);
        shutdown (FALSE);
        return;
    }

#undef VALIDATE_UNKNOWN
#define VALIDATE_UNKNOWN(str) (str ? str : "unknown")

    qmi_message_dms_get_revision_output_get_revision (output, &str, NULL);

    g_print ("[%s] Device revision retrieved:\n"
             "\tRevision: '%s'\n",
             qmi_device_get_path_display (ctx->device),
             VALIDATE_UNKNOWN (str));

    qmi_message_dms_get_revision_output_unref (output);
    shutdown (TRUE);
}

static void
get_msisdn_ready (QmiClientDms *client,
                  GAsyncResult *res)
{
    const gchar *str = NULL;
    QmiMessageDmsGetMsisdnOutput *output;
    GError *error = NULL;

    output = qmi_client_dms_get_msisdn_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_get_msisdn_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get MSISDN: %s\n", error->message);
        g_error_free (error);
        qmi_message_dms_get_msisdn_output_unref (output);
        shutdown (FALSE);
        return;
    }

#undef VALIDATE_UNKNOWN
#define VALIDATE_UNKNOWN(str) (str ? str : "unknown")

    qmi_message_dms_get_msisdn_output_get_msisdn (output, &str, NULL);

    g_print ("[%s] Device MSISDN retrieved:\n"
             "\tMSISDN: '%s'\n",
             qmi_device_get_path_display (ctx->device),
             VALIDATE_UNKNOWN (str));

    qmi_message_dms_get_msisdn_output_unref (output);
    shutdown (TRUE);
}

static void
get_power_state_ready (QmiClientDms *client,
                       GAsyncResult *res)
{
    gchar *power_state_str;
    QmiMessageDmsGetPowerStateOutputInfo info;
    QmiMessageDmsGetPowerStateOutput *output;
    GError *error = NULL;

    output = qmi_client_dms_get_power_state_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_get_power_state_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get power state: %s\n", error->message);
        g_error_free (error);
        qmi_message_dms_get_power_state_output_unref (output);
        shutdown (FALSE);
        return;
    }

    qmi_message_dms_get_power_state_output_get_info (output, &info, NULL);
    power_state_str = qmi_dms_power_state_build_string_from_mask ((QmiDmsPowerState)info.power_state_flags);

    g_print ("[%s] Device power state retrieved:\n"
             "\tPower state: '%s'\n"
             "\tBattery level: '%u %%'\n",
             qmi_device_get_path_display (ctx->device),
             power_state_str,
             (guint)info.battery_level);

    g_free (power_state_str);
    qmi_message_dms_get_power_state_output_unref (output);
    shutdown (TRUE);
}


static QmiMessageDmsSetPinProtectionInput *
set_pin_protection_input_create (const gchar *str)
{
    QmiMessageDmsSetPinProtectionInput *input;
    QmiMessageDmsSetPinProtectionInputInfo info;
    gchar **split;

    /* Prepare inputs.
     * Format of the string is:
     *    "[(PIN|PIN2),(disable|enable),(current PIN)]"
     */
    input = qmi_message_dms_set_pin_protection_input_new ();

    split = g_strsplit (str, ",", -1);

    if (g_str_equal (split[0], "PIN"))
        info.pin_id = QMI_DMS_PIN_ID_PIN;
    else if (g_str_equal (split[0], "PIN2"))
        info.pin_id = QMI_DMS_PIN_ID_PIN2;
    else {
        g_printerr ("error: expected 'PIN' or 'PIN2', got: '%s'\n",
                    split[0]);
        exit (EXIT_FAILURE);
    }

    if (g_str_equal (split[1], "disable"))
        info.protection_enabled = FALSE;
    else if (g_str_equal (split[1], "enable"))
        info.protection_enabled = TRUE;
    else {
        g_printerr ("error: expected 'disable' or 'enable', got: '%s'\n",
                    split[1]);
        exit (EXIT_FAILURE);
    }

    info.pin = split[2];
    if (info.pin[0] == '\0') {
        g_printerr ("error: empty current PIN given\n");
        exit (EXIT_FAILURE);
    }

    qmi_message_dms_set_pin_protection_input_set_info (input, info, NULL);
    g_strfreev (split);

    return input;
}

static void
set_pin_protection_ready (QmiClientDms *client,
                          GAsyncResult *res)
{
    QmiMessageDmsSetPinProtectionOutput *output;
    GError *error = NULL;

    output = qmi_client_dms_set_pin_protection_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_set_pin_protection_output_get_result (output, &error)) {
        QmiMessageDmsSetPinProtectionOutputPinRetriesStatus retry_status;

        g_printerr ("error: couldn't set PIN protection: %s\n", error->message);
        g_error_free (error);

        if (qmi_message_dms_set_pin_protection_output_get_pin_retries_status (output,
                                                                              &retry_status,
                                                                              NULL)) {
            g_printerr ("[%s] Retries left:\n"
                        "\tVerify: %u\n"
                        "\tUnblock: %u\n",
                        qmi_device_get_path_display (ctx->device),
                        retry_status.verify_retries_left,
                        retry_status.unblock_retries_left);
        }

        qmi_message_dms_set_pin_protection_output_unref (output);
        shutdown (FALSE);
        return;
    }

    g_print ("[%s] PIN protection updated\n",
             qmi_device_get_path_display (ctx->device));

    qmi_message_dms_set_pin_protection_output_unref (output);
    shutdown (TRUE);
}

static QmiMessageDmsVerifyPinInput *
verify_pin_input_create (const gchar *str)
{
    QmiMessageDmsVerifyPinInput *input;
    QmiMessageDmsVerifyPinInputInfo info;
    gchar **split;

    /* Prepare inputs.
     * Format of the string is:
     *    "[(PIN|PIN2),(current PIN)]"
     */
    input = qmi_message_dms_verify_pin_input_new ();

    split = g_strsplit (str, ",", -1);

    if (g_str_equal (split[0], "PIN"))
        info.pin_id = QMI_DMS_PIN_ID_PIN;
    else if (g_str_equal (split[0], "PIN2"))
        info.pin_id = QMI_DMS_PIN_ID_PIN2;
    else {
        g_printerr ("error: expected 'PIN' or 'PIN2', got: '%s'\n",
                    split[0]);
        exit (EXIT_FAILURE);
    }

    info.pin = split[1];
    if (info.pin[0] == '\0') {
        g_printerr ("error: empty current PIN given\n");
        exit (EXIT_FAILURE);
    }

    qmi_message_dms_verify_pin_input_set_info (input, info, NULL);
    g_strfreev (split);

    return input;
}

static void
verify_pin_ready (QmiClientDms *client,
                  GAsyncResult *res)
{
    QmiMessageDmsVerifyPinOutput *output;
    GError *error = NULL;

    output = qmi_client_dms_verify_pin_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_verify_pin_output_get_result (output, &error)) {
        QmiMessageDmsVerifyPinOutputPinRetriesStatus retry_status;

        g_printerr ("error: couldn't verify PIN: %s\n", error->message);
        g_error_free (error);

        if (qmi_message_dms_verify_pin_output_get_pin_retries_status (output,
                                                                      &retry_status,
                                                                      NULL)) {
            g_printerr ("[%s] Retries left:\n"
                        "\tVerify: %u\n"
                        "\tUnblock: %u\n",
                        qmi_device_get_path_display (ctx->device),
                        retry_status.verify_retries_left,
                        retry_status.unblock_retries_left);
        }

        qmi_message_dms_verify_pin_output_unref (output);
        shutdown (FALSE);
        return;
    }

    g_print ("[%s] PIN verified successfully\n",
             qmi_device_get_path_display (ctx->device));

    qmi_message_dms_verify_pin_output_unref (output);
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

    /* Request to get revision? */
    if (get_revision_flag) {
        g_debug ("Asynchronously getting revision...");
        qmi_client_dms_get_revision (ctx->client,
                                     NULL,
                                     10,
                                     ctx->cancellable,
                                     (GAsyncReadyCallback)get_revision_ready,
                                     NULL);
        return;
    }

    /* Request to get msisdn? */
    if (get_msisdn_flag) {
        g_debug ("Asynchronously getting msisdn...");
        qmi_client_dms_get_msisdn (ctx->client,
                                   NULL,
                                   10,
                                   ctx->cancellable,
                                   (GAsyncReadyCallback)get_msisdn_ready,
                                   NULL);
        return;
    }

    /* Request to get power status? */
    if (get_power_state_flag) {
        g_debug ("Asynchronously getting power status...");
        qmi_client_dms_get_power_state (ctx->client,
                                        NULL,
                                        10,
                                        ctx->cancellable,
                                        (GAsyncReadyCallback)get_power_state_ready,
                                        NULL);
        return;
    }

    /* Request to set PIN protection? */
    if (set_pin_protection_str) {
        QmiMessageDmsSetPinProtectionInput *input;

        g_debug ("Asynchronously setting PIN protection...");
        input = set_pin_protection_input_create (set_pin_protection_str);
        qmi_client_dms_set_pin_protection (ctx->client,
                                           input,
                                           10,
                                           ctx->cancellable,
                                           (GAsyncReadyCallback)set_pin_protection_ready,
                                           NULL);
        qmi_message_dms_set_pin_protection_input_unref (input);
        return;
    }

    /* Request to verify PIN? */
    if (verify_pin_str) {
        QmiMessageDmsVerifyPinInput *input;

        g_debug ("Asynchronously verifying PIN...");
        input = verify_pin_input_create (verify_pin_str);
        qmi_client_dms_verify_pin (ctx->client,
                                   input,
                                   10,
                                   ctx->cancellable,
                                   (GAsyncReadyCallback)verify_pin_ready,
                                   NULL);
        qmi_message_dms_verify_pin_input_unref (input);
        return;
    }

    /* Just client allocate/release? */
    if (noop_flag) {
        g_idle_add (noop_cb, NULL);
        return;
    }

    g_warn_if_reached ();
}
