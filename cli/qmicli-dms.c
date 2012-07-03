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
static gchar *uim_set_pin_protection_str;
static gchar *uim_verify_pin_str;
static gchar *uim_unblock_pin_str;
static gchar *uim_change_pin_str;
static gboolean uim_get_pin_status_flag;
static gboolean uim_get_iccid_flag;
static gboolean uim_get_imsi_flag;
static gboolean get_hardware_revision_flag;
static gboolean get_operating_mode_flag;
static gchar *set_operating_mode_str;
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
    { "dms-uim-set-pin-protection", 0, 0, G_OPTION_ARG_STRING, &uim_set_pin_protection_str,
      "Set PIN protection in the UIM",
      "[(PIN|PIN2),(disable|enable),(current PIN)]",
    },
    { "dms-uim-verify-pin", 0, 0, G_OPTION_ARG_STRING, &uim_verify_pin_str,
      "Verify PIN",
      "[(PIN|PIN2),(current PIN)]",
    },
    { "dms-uim-unblock-pin", 0, 0, G_OPTION_ARG_STRING, &uim_unblock_pin_str,
      "Unblock PIN",
      "[(PIN|PIN2),(PUK),(new PIN)]",
    },
    { "dms-uim-change-pin", 0, 0, G_OPTION_ARG_STRING, &uim_change_pin_str,
      "Change PIN",
      "[(PIN|PIN2),(old PIN),(new PIN)]",
    },
    { "dms-uim-get-pin-status", 0, 0, G_OPTION_ARG_NONE, &uim_get_pin_status_flag,
      "Get PIN status",
      NULL
    },
    { "dms-uim-get-iccid", 0, 0, G_OPTION_ARG_NONE, &uim_get_iccid_flag,
      "Get ICCID",
      NULL
    },
    { "dms-uim-get-imsi", 0, 0, G_OPTION_ARG_NONE, &uim_get_imsi_flag,
      "Get IMSI",
      NULL
    },
    { "dms-get-hardware-revision", 0, 0, G_OPTION_ARG_NONE, &get_hardware_revision_flag,
      "Get the HW revision",
      NULL
    },
    { "dms-get-operating-mode", 0, 0, G_OPTION_ARG_NONE, &get_operating_mode_flag,
      "Get the device operating mode",
      NULL
    },
    { "dms-set-operating-mode", 0, 0, G_OPTION_ARG_STRING, &set_operating_mode_str,
      "Set the device operating mode",
      "[(Operating mode)]"
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
                 !!uim_set_pin_protection_str +
                 !!uim_verify_pin_str +
                 !!uim_unblock_pin_str +
                 !!uim_change_pin_str +
                 uim_get_pin_status_flag +
                 uim_get_iccid_flag +
                 uim_get_imsi_flag +
                 get_hardware_revision_flag +
                 get_operating_mode_flag +
                 !!set_operating_mode_str +
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

static QmiDmsUimPinId
read_pin_id_from_string (const gchar *str)
{
    if (!str || str[0] == '\0') {
        g_printerr ("error: expected 'PIN' or 'PIN2', got: none\n");
        exit (EXIT_FAILURE);
    }

    if (g_str_equal (str, "PIN"))
        return QMI_DMS_UIM_PIN_ID_PIN;

    if (g_str_equal (str, "PIN2"))
        return QMI_DMS_UIM_PIN_ID_PIN2;

    g_printerr ("error: expected 'PIN' or 'PIN2', got: '%s'\n",
                str);
    exit (EXIT_FAILURE);
}

static gboolean
read_enable_disable_from_string (const gchar *str)
{
    if (!str || str[0] == '\0') {
        g_printerr ("error: expected 'disable' or 'enable', got: none\n");
        exit (EXIT_FAILURE);
    }

    if (g_str_equal (str, "disable"))
        return FALSE;

    if (g_str_equal (str, "enable"))
        return TRUE;

    g_printerr ("error: expected 'disable' or 'enable', got: '%s'\n",
                str);
    exit (EXIT_FAILURE);
}

static gchar *
read_non_empty_string (const gchar *str,
                       const gchar *description)
{
    if (!str || str[0] == '\0') {
        g_printerr ("error: empty %s given\n", description);
        exit (EXIT_FAILURE);
    }

    return (gchar *)str;
}

static QmiMessageDmsUimSetPinProtectionInput *
uim_set_pin_protection_input_create (const gchar *str)
{
    QmiMessageDmsUimSetPinProtectionInput *input;
    QmiMessageDmsUimSetPinProtectionInputInfo info;
    gchar **split;

    /* Prepare inputs.
     * Format of the string is:
     *    "[(PIN|PIN2),(disable|enable),(current PIN)]"
     */
    input = qmi_message_dms_uim_set_pin_protection_input_new ();

    split = g_strsplit (str, ",", -1);

    info.pin_id = read_pin_id_from_string (split[0]);
    info.protection_enabled = read_enable_disable_from_string (split[1]);
    info.pin = read_non_empty_string (split[2], "current PIN");

    qmi_message_dms_uim_set_pin_protection_input_set_info (input, &info, NULL);
    g_strfreev (split);

    return input;
}

static void
uim_set_pin_protection_ready (QmiClientDms *client,
                              GAsyncResult *res)
{
    QmiMessageDmsUimSetPinProtectionOutput *output;
    GError *error = NULL;

    output = qmi_client_dms_uim_set_pin_protection_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_uim_set_pin_protection_output_get_result (output, &error)) {
        QmiMessageDmsUimSetPinProtectionOutputPinRetriesStatus retry_status;

        g_printerr ("error: couldn't set PIN protection: %s\n", error->message);
        g_error_free (error);

        if (qmi_message_dms_uim_set_pin_protection_output_get_pin_retries_status (output,
                                                                                  &retry_status,
                                                                                  NULL)) {
            g_printerr ("[%s] Retries left:\n"
                        "\tVerify: %u\n"
                        "\tUnblock: %u\n",
                        qmi_device_get_path_display (ctx->device),
                        retry_status.verify_retries_left,
                        retry_status.unblock_retries_left);
        }

        qmi_message_dms_uim_set_pin_protection_output_unref (output);
        shutdown (FALSE);
        return;
    }

    g_print ("[%s] PIN protection updated\n",
             qmi_device_get_path_display (ctx->device));

    qmi_message_dms_uim_set_pin_protection_output_unref (output);
    shutdown (TRUE);
}

static QmiMessageDmsUimVerifyPinInput *
uim_verify_pin_input_create (const gchar *str)
{
    QmiMessageDmsUimVerifyPinInput *input;
    QmiMessageDmsUimVerifyPinInputInfo info;
    gchar **split;

    /* Prepare inputs.
     * Format of the string is:
     *    "[(PIN|PIN2),(current PIN)]"
     */
    input = qmi_message_dms_uim_verify_pin_input_new ();

    split = g_strsplit (str, ",", -1);

    info.pin_id = read_pin_id_from_string (split[0]);
    info.pin = read_non_empty_string (split[1], "current PIN");

    qmi_message_dms_uim_verify_pin_input_set_info (input, &info, NULL);
    g_strfreev (split);

    return input;
}

static void
uim_verify_pin_ready (QmiClientDms *client,
                      GAsyncResult *res)
{
    QmiMessageDmsUimVerifyPinOutput *output;
    GError *error = NULL;

    output = qmi_client_dms_uim_verify_pin_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_uim_verify_pin_output_get_result (output, &error)) {
        QmiMessageDmsUimVerifyPinOutputPinRetriesStatus retry_status;

        g_printerr ("error: couldn't verify PIN: %s\n", error->message);
        g_error_free (error);

        if (qmi_message_dms_uim_verify_pin_output_get_pin_retries_status (output,
                                                                          &retry_status,
                                                                          NULL)) {
            g_printerr ("[%s] Retries left:\n"
                        "\tVerify: %u\n"
                        "\tUnblock: %u\n",
                        qmi_device_get_path_display (ctx->device),
                        retry_status.verify_retries_left,
                        retry_status.unblock_retries_left);
        }

        qmi_message_dms_uim_verify_pin_output_unref (output);
        shutdown (FALSE);
        return;
    }

    g_print ("[%s] PIN verified successfully\n",
             qmi_device_get_path_display (ctx->device));

    qmi_message_dms_uim_verify_pin_output_unref (output);
    shutdown (TRUE);
}

static QmiMessageDmsUimUnblockPinInput *
uim_unblock_pin_input_create (const gchar *str)
{
    QmiMessageDmsUimUnblockPinInput *input;
    QmiMessageDmsUimUnblockPinInputInfo info;
    gchar **split;

    /* Prepare inputs.
     * Format of the string is:
     *    "[(PIN|PIN2),(PUK),(new PIN)]"
     */
    input = qmi_message_dms_uim_unblock_pin_input_new ();

    split = g_strsplit (str, ",", -1);

    info.pin_id = read_pin_id_from_string (split[0]);
    info.puk = read_non_empty_string (split[1], "PUK");
    info.new_pin = read_non_empty_string (split[2], "new PIN");

    qmi_message_dms_uim_unblock_pin_input_set_info (input, &info, NULL);
    g_strfreev (split);

    return input;
}

static void
uim_unblock_pin_ready (QmiClientDms *client,
                       GAsyncResult *res)
{
    QmiMessageDmsUimUnblockPinOutput *output;
    GError *error = NULL;

    output = qmi_client_dms_uim_unblock_pin_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_uim_unblock_pin_output_get_result (output, &error)) {
        QmiMessageDmsUimUnblockPinOutputPinRetriesStatus retry_status;

        g_printerr ("error: couldn't unblock PIN: %s\n", error->message);
        g_error_free (error);

        if (qmi_message_dms_uim_unblock_pin_output_get_pin_retries_status (output,
                                                                       &retry_status,
                                                                       NULL)) {
            g_printerr ("[%s] Retries left:\n"
                        "\tVerify: %u\n"
                        "\tUnblock: %u\n",
                        qmi_device_get_path_display (ctx->device),
                        retry_status.verify_retries_left,
                        retry_status.unblock_retries_left);
        }

        qmi_message_dms_uim_unblock_pin_output_unref (output);
        shutdown (FALSE);
        return;
    }

    g_print ("[%s] PIN unblocked successfully\n",
             qmi_device_get_path_display (ctx->device));

    qmi_message_dms_uim_unblock_pin_output_unref (output);
    shutdown (TRUE);
}

static QmiMessageDmsUimChangePinInput *
uim_change_pin_input_create (const gchar *str)
{
    QmiMessageDmsUimChangePinInput *input;
    QmiMessageDmsUimChangePinInputInfo info;
    gchar **split;

    /* Prepare inputs.
     * Format of the string is:
     *    "[(PIN|PIN2),(old PIN),(new PIN)]"
     */
    input = qmi_message_dms_uim_change_pin_input_new ();

    split = g_strsplit (str, ",", -1);

    info.pin_id = read_pin_id_from_string (split[0]);
    info.old_pin = read_non_empty_string (split[1], "old PIN");
    info.new_pin = read_non_empty_string (split[2], "new PIN");

    qmi_message_dms_uim_change_pin_input_set_info (input, &info, NULL);
    g_strfreev (split);

    return input;
}

static void
uim_change_pin_ready (QmiClientDms *client,
                      GAsyncResult *res)
{
    QmiMessageDmsUimChangePinOutput *output;
    GError *error = NULL;

    output = qmi_client_dms_uim_change_pin_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_uim_change_pin_output_get_result (output, &error)) {
        QmiMessageDmsUimChangePinOutputPinRetriesStatus retry_status;

        g_printerr ("error: couldn't change PIN: %s\n", error->message);
        g_error_free (error);

        if (qmi_message_dms_uim_change_pin_output_get_pin_retries_status (output,
                                                                      &retry_status,
                                                                      NULL)) {
            g_printerr ("[%s] Retries left:\n"
                        "\tVerify: %u\n"
                        "\tUnblock: %u\n",
                        qmi_device_get_path_display (ctx->device),
                        retry_status.verify_retries_left,
                        retry_status.unblock_retries_left);
        }

        qmi_message_dms_uim_change_pin_output_unref (output);
        shutdown (FALSE);
        return;
    }

    g_print ("[%s] PIN changed successfully\n",
             qmi_device_get_path_display (ctx->device));

    qmi_message_dms_uim_change_pin_output_unref (output);
    shutdown (TRUE);
}

static void
uim_get_pin_status_ready (QmiClientDms *client,
                          GAsyncResult *res)
{
    QmiMessageDmsUimGetPinStatusOutputPin1Status pin1_status;
    QmiMessageDmsUimGetPinStatusOutputPin2Status pin2_status;
    QmiMessageDmsUimGetPinStatusOutput *output;
    GError *error = NULL;

    output = qmi_client_dms_uim_get_pin_status_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_uim_get_pin_status_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get PIN status: %s\n", error->message);
        g_error_free (error);
        qmi_message_dms_uim_get_pin_status_output_unref (output);
        shutdown (FALSE);
        return;
    }

    g_print ("[%s] PIN status retrieved successfully\n",
             qmi_device_get_path_display (ctx->device));

    if (qmi_message_dms_uim_get_pin_status_output_get_pin1_status (output,
                                                                   &pin1_status,
                                                                   NULL)) {
        g_print ("[%s] PIN1:\n"
                 "\tStatus: %s\n"
                 "\tVerify: %u\n"
                 "\tUnblock: %u\n",
                 qmi_device_get_path_display (ctx->device),
                 qmi_dms_uim_pin_status_get_string (pin1_status.current_status),
                 pin1_status.verify_retries_left,
                 pin1_status.unblock_retries_left);
    }

    if (qmi_message_dms_uim_get_pin_status_output_get_pin2_status (output,
                                                                   &pin2_status,
                                                                   NULL)) {
        g_print ("[%s] PIN2:\n"
                 "\tStatus: %s\n"
                 "\tVerify: %u\n"
                 "\tUnblock: %u\n",
                 qmi_device_get_path_display (ctx->device),
                 qmi_dms_uim_pin_status_get_string (pin2_status.current_status),
                 pin2_status.verify_retries_left,
                 pin2_status.unblock_retries_left);
    }

    qmi_message_dms_uim_get_pin_status_output_unref (output);
    shutdown (TRUE);
}

static void
uim_get_iccid_ready (QmiClientDms *client,
                     GAsyncResult *res)
{
    const gchar *str = NULL;
    QmiMessageDmsUimGetIccidOutput *output;
    GError *error = NULL;

    output = qmi_client_dms_uim_get_iccid_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_uim_get_iccid_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get ICCID: %s\n", error->message);
        g_error_free (error);
        qmi_message_dms_uim_get_iccid_output_unref (output);
        shutdown (FALSE);
        return;
    }

#undef VALIDATE_UNKNOWN
#define VALIDATE_UNKNOWN(str) (str ? str : "unknown")

    qmi_message_dms_uim_get_iccid_output_get_iccid (output, &str, NULL);

    g_print ("[%s] UIM ICCID retrieved:\n"
             "\tICCID: '%s'\n",
             qmi_device_get_path_display (ctx->device),
             VALIDATE_UNKNOWN (str));

    qmi_message_dms_uim_get_iccid_output_unref (output);
    shutdown (TRUE);
}

static void
uim_get_imsi_ready (QmiClientDms *client,
                    GAsyncResult *res)
{
    const gchar *str = NULL;
    QmiMessageDmsUimGetImsiOutput *output;
    GError *error = NULL;

    output = qmi_client_dms_uim_get_imsi_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_uim_get_imsi_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get IMSI: %s\n", error->message);
        g_error_free (error);
        qmi_message_dms_uim_get_imsi_output_unref (output);
        shutdown (FALSE);
        return;
    }

#undef VALIDATE_UNKNOWN
#define VALIDATE_UNKNOWN(str) (str ? str : "unknown")

    qmi_message_dms_uim_get_imsi_output_get_imsi (output, &str, NULL);

    g_print ("[%s] UIM IMSI retrieved:\n"
             "\tIMSI: '%s'\n",
             qmi_device_get_path_display (ctx->device),
             VALIDATE_UNKNOWN (str));

    qmi_message_dms_uim_get_imsi_output_unref (output);
    shutdown (TRUE);
}

static void
get_hardware_revision_ready (QmiClientDms *client,
                             GAsyncResult *res)
{
    const gchar *str = NULL;
    QmiMessageDmsGetHardwareRevisionOutput *output;
    GError *error = NULL;

    output = qmi_client_dms_get_hardware_revision_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_get_hardware_revision_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get the HW revision: %s\n", error->message);
        g_error_free (error);
        qmi_message_dms_get_hardware_revision_output_unref (output);
        shutdown (FALSE);
        return;
    }

#undef VALIDATE_UNKNOWN
#define VALIDATE_UNKNOWN(str) (str ? str : "unknown")

    qmi_message_dms_get_hardware_revision_output_get_revision (output, &str, NULL);

    g_print ("[%s] Hardware revision retrieved:\n"
             "\tRevision: '%s'\n",
             qmi_device_get_path_display (ctx->device),
             VALIDATE_UNKNOWN (str));

    qmi_message_dms_get_hardware_revision_output_unref (output);
    shutdown (TRUE);
}

static void
get_operating_mode_ready (QmiClientDms *client,
                          GAsyncResult *res)
{
    QmiMessageDmsGetOperatingModeOutput *output;
    QmiDmsOperatingMode mode;
    gboolean hw_restricted = FALSE;
    GError *error = NULL;

    output = qmi_client_dms_get_operating_mode_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_get_operating_mode_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get the HW revision: %s\n", error->message);
        g_error_free (error);
        qmi_message_dms_get_operating_mode_output_unref (output);
        shutdown (FALSE);
        return;
    }

#undef VALIDATE_UNKNOWN
#define VALIDATE_UNKNOWN(str) (str ? str : "unknown")

    qmi_message_dms_get_operating_mode_output_get_mode (output, &mode, NULL);

    g_print ("[%s] Operating mode retrieved:\n"
             "\tMode: '%s'\n",
             qmi_device_get_path_display (ctx->device),
             qmi_dms_operating_mode_get_string (mode));

    if (mode == QMI_DMS_OPERATING_MODE_OFFLINE) {
        QmiDmsOfflineReason reason;
        gchar *reason_str = NULL;

        if (qmi_message_dms_get_operating_mode_output_get_offline_reason (output, &reason, NULL)) {
            reason_str = qmi_dms_offline_reason_build_string_from_mask (reason);
        }

        g_print ("\tReason: '%s'\n", VALIDATE_UNKNOWN (reason_str));
        g_free (reason_str);
    }

    qmi_message_dms_get_operating_mode_output_get_hardware_restricted_mode (output, &hw_restricted, NULL);
    g_print ("\tHW restricted: '%s'\n", hw_restricted ? "yes" : "no");

    qmi_message_dms_get_operating_mode_output_unref (output);
    shutdown (TRUE);
}

static QmiMessageDmsSetOperatingModeInput *
set_operating_mode_input_create (const gchar *str)
{
    QmiMessageDmsSetOperatingModeInput *input;
    GType type;
    GEnumClass *enum_class;
    GEnumValue *enum_value;

    type = qmi_dms_operating_mode_get_type ();
    enum_class = G_ENUM_CLASS (g_type_class_ref (type));
    enum_value = g_enum_get_value_by_nick (enum_class, str);

    if (!enum_value) {
        g_printerr ("error: invalid operating mode value given: '%s'\n", str);
        exit (EXIT_FAILURE);
    }

    input = qmi_message_dms_set_operating_mode_input_new ();
    qmi_message_dms_set_operating_mode_input_set_mode (input, (QmiDmsOperatingMode)enum_value->value, NULL);
    g_type_class_unref (enum_class);

    return input;
}

static void
set_operating_mode_ready (QmiClientDms *client,
                          GAsyncResult *res)
{
    QmiMessageDmsSetOperatingModeOutput *output;
    GError *error = NULL;

    output = qmi_client_dms_set_operating_mode_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_set_operating_mode_output_get_result (output, &error)) {
        g_printerr ("error: couldn't set operating mode: %s\n", error->message);
        g_error_free (error);
        qmi_message_dms_set_operating_mode_output_unref (output);
        shutdown (FALSE);
        return;
    }

    g_print ("[%s] Operating mode set successfully\n",
             qmi_device_get_path_display (ctx->device));

    qmi_message_dms_set_operating_mode_output_unref (output);
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
    if (uim_set_pin_protection_str) {
        QmiMessageDmsUimSetPinProtectionInput *input;

        g_debug ("Asynchronously setting PIN protection...");
        input = uim_set_pin_protection_input_create (uim_set_pin_protection_str);
        qmi_client_dms_uim_set_pin_protection (ctx->client,
                                               input,
                                               10,
                                               ctx->cancellable,
                                               (GAsyncReadyCallback)uim_set_pin_protection_ready,
                                               NULL);
        qmi_message_dms_uim_set_pin_protection_input_unref (input);
        return;
    }

    /* Request to verify PIN? */
    if (uim_verify_pin_str) {
        QmiMessageDmsUimVerifyPinInput *input;

        g_debug ("Asynchronously verifying PIN...");
        input = uim_verify_pin_input_create (uim_verify_pin_str);
        qmi_client_dms_uim_verify_pin (ctx->client,
                                       input,
                                       10,
                                       ctx->cancellable,
                                       (GAsyncReadyCallback)uim_verify_pin_ready,
                                       NULL);
        qmi_message_dms_uim_verify_pin_input_unref (input);
        return;
    }

    /* Request to unblock PIN? */
    if (uim_unblock_pin_str) {
        QmiMessageDmsUimUnblockPinInput *input;

        g_debug ("Asynchronously unblocking PIN...");
        input = uim_unblock_pin_input_create (uim_unblock_pin_str);
        qmi_client_dms_uim_unblock_pin (ctx->client,
                                        input,
                                        10,
                                        ctx->cancellable,
                                        (GAsyncReadyCallback)uim_unblock_pin_ready,
                                        NULL);
        qmi_message_dms_uim_unblock_pin_input_unref (input);
        return;
    }

    /* Request to change the PIN? */
    if (uim_change_pin_str) {
        QmiMessageDmsUimChangePinInput *input;

        g_debug ("Asynchronously changing PIN...");
        input = uim_change_pin_input_create (uim_change_pin_str);
        qmi_client_dms_uim_change_pin (ctx->client,
                                       input,
                                       10,
                                       ctx->cancellable,
                                       (GAsyncReadyCallback)uim_change_pin_ready,
                                       NULL);
        qmi_message_dms_uim_change_pin_input_unref (input);
        return;
    }

    /* Request to get PIN status? */
    if (uim_get_pin_status_flag) {
        g_debug ("Asynchronously getting PIN status...");
        qmi_client_dms_uim_get_pin_status (ctx->client,
                                           NULL,
                                           10,
                                           ctx->cancellable,
                                           (GAsyncReadyCallback)uim_get_pin_status_ready,
                                           NULL);
        return;
    }

    /* Request to get UIM ICCID? */
    if (uim_get_iccid_flag) {
        g_debug ("Asynchronously getting UIM ICCID...");
        qmi_client_dms_uim_get_iccid (ctx->client,
                                      NULL,
                                      10,
                                      ctx->cancellable,
                                      (GAsyncReadyCallback)uim_get_iccid_ready,
                                      NULL);
        return;
    }

    /* Request to get UIM IMSI? */
    if (uim_get_imsi_flag) {
        g_debug ("Asynchronously getting UIM IMSI...");
        qmi_client_dms_uim_get_imsi (ctx->client,
                                     NULL,
                                     10,
                                     ctx->cancellable,
                                     (GAsyncReadyCallback)uim_get_imsi_ready,
                                     NULL);
        return;
    }

    /* Request to get hardware revision? */
    if (get_hardware_revision_flag) {
        g_debug ("Asynchronously getting hardware revision...");
        qmi_client_dms_get_hardware_revision (ctx->client,
                                              NULL,
                                              10,
                                              ctx->cancellable,
                                              (GAsyncReadyCallback)get_hardware_revision_ready,
                                              NULL);
        return;
    }

    /* Request to get operating mode? */
    if (get_operating_mode_flag) {
        g_debug ("Asynchronously getting operating mode...");
        qmi_client_dms_get_operating_mode (ctx->client,
                                           NULL,
                                           10,
                                           ctx->cancellable,
                                           (GAsyncReadyCallback)get_operating_mode_ready,
                                           NULL);
        return;
    }

    /* Request to set operating mode? */
    if (set_operating_mode_str) {
        QmiMessageDmsSetOperatingModeInput *input;

        g_debug ("Asynchronously setting operating mode...");
        input = set_operating_mode_input_create (set_operating_mode_str);
        qmi_client_dms_set_operating_mode (ctx->client,
                                           input,
                                           10,
                                           ctx->cancellable,
                                           (GAsyncReadyCallback)set_operating_mode_ready,
                                           NULL);
        qmi_message_dms_set_operating_mode_input_unref (input);
        return;
    }

    /* Just client allocate/release? */
    if (noop_flag) {
        g_idle_add (noop_cb, NULL);
        return;
    }

    g_warn_if_reached ();
}
