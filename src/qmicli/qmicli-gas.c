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
#include <qmi-common.h>

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
static gboolean  get_firmware_list_flag;
static gboolean  get_active_firmware_flag;
static gint      set_active_firmware_int = -1;
static gint      set_usb_composition_int = -1;
static gboolean  get_usb_composition_flag;
static gboolean  get_ethernet_mac_address_flag;
static gchar    *set_firmware_auto_sim_str;
static gboolean  get_firmware_auto_sim_flag;
static gboolean  noop_flag;

static GOptionEntry entries[] = {
#if defined HAVE_QMI_MESSAGE_GAS_DMS_SET_USB_COMPOSITION
    { "gas-dms-set-usb-composition", 0, 0, G_OPTION_ARG_INT, &set_usb_composition_int,
      "Sets the USB composition",
      "[pid]"
    },
#endif
#if defined HAVE_QMI_MESSAGE_GAS_DMS_GET_USB_COMPOSITION
    { "gas-dms-get-usb-composition", 0, 0, G_OPTION_ARG_NONE, &get_usb_composition_flag,
      "Gets the current USB composition",
      NULL
    },
#endif
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
#if defined HAVE_QMI_MESSAGE_GAS_DMS_GET_ETHERNET_PDU_MAC_ADDRESS
    { "gas-dms-get-ethernet-mac-address", 0, 0, G_OPTION_ARG_NONE, &get_ethernet_mac_address_flag,
      "Gets the Ethernet PDU MAC address available in the modem",
      NULL
    },
#endif
#if defined HAVE_QMI_MESSAGE_GAS_DMS_SET_FIRMWARE_AUTO_SIM
    { "gas-dms-set-firmware-auto-sim", 0, 0, G_OPTION_ARG_STRING, &set_firmware_auto_sim_str,
      "Sets the automatic carrier switching mode according to the sim (allowed keys: mode (disable|enable|enable-one-shot), config-id",
      "[\"key=value,...\"]"
    },
#endif
#if defined HAVE_QMI_MESSAGE_GAS_DMS_GET_FIRMWARE_AUTO_SIM
    { "gas-dms-get-firmware-auto-sim", 0, 0, G_OPTION_ARG_NONE, &get_firmware_auto_sim_flag,
      "Gets the automatic carrier switching mode according to the sim",
      NULL
    },
#endif
    { "gas-noop", 0, 0, G_OPTION_ARG_NONE, &noop_flag,
      "Just allocate or release a GAS client. Use with `--client-no-release-cid' and/or `--client-cid'",
      NULL
    },
    { NULL, 0, 0, 0, NULL, NULL, NULL }
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

    n_actions = ((set_usb_composition_int >= 0) +
                 get_usb_composition_flag +
                 get_firmware_list_flag +
                 get_active_firmware_flag +
                 (set_active_firmware_int >= 0) +
                 get_ethernet_mac_address_flag +
                 !!set_firmware_auto_sim_str +
                 get_firmware_auto_sim_flag +
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

#if defined HAVE_QMI_MESSAGE_GAS_DMS_SET_USB_COMPOSITION

static void
set_usb_composition_ready (QmiClientGas *client,
                           GAsyncResult *res)
{
    QmiMessageGasDmsSetUsbCompositionOutput *output;
    GError *error = NULL;

    output = qmi_client_gas_dms_set_usb_composition_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_gas_dms_set_usb_composition_output_get_result (output, &error)) {
        g_printerr ("error: unable to switch composition: %s\n", error->message);
        g_error_free (error);
        qmi_message_gas_dms_set_usb_composition_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] Successfully switched composition.\n",
             qmi_device_get_path_display (ctx->device));

    qmi_message_gas_dms_set_usb_composition_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_GAS_DMS_SET_USB_COMPOSITION */

#if defined HAVE_QMI_MESSAGE_GAS_DMS_GET_USB_COMPOSITION

static void
get_usb_composition_ready (QmiClientGas *client,
                           GAsyncResult *res)
{
    QmiMessageGasDmsGetUsbCompositionOutput *output;
    GError *error = NULL;
    guint32 composition;

    output = qmi_client_gas_dms_get_usb_composition_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_gas_dms_get_usb_composition_output_get_result (output, &error)) {
        g_printerr ("error: unable to get current composition: %s\n", error->message);
        g_error_free (error);
        qmi_message_gas_dms_get_usb_composition_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    qmi_message_gas_dms_get_usb_composition_output_get_usb_composition (output, &composition, NULL);
    g_print ("[%s] Current composition is 0x%x\n",
             qmi_device_get_path_display (ctx->device),
             composition);

    qmi_message_gas_dms_get_usb_composition_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_GAS_DMS_GET_USB_COMPOSITION */

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

#if defined HAVE_QMI_MESSAGE_GAS_DMS_GET_ETHERNET_PDU_MAC_ADDRESS

static void
print_mac_address (GArray *address)
{
    g_autofree gchar *str = NULL;

    str = qmi_common_str_hex (address->data, address->len, ':');
    g_print ("%s\n", str);
}

static void
get_ethernet_pdu_mac_address_ready (QmiClientGas *client,
                                    GAsyncResult *res)
{
    g_autoptr(QmiMessageGasDmsGetEthernetPduMacAddressOutput) output;
    g_autoptr(GError) error = NULL;
    GArray *address;

    output = qmi_client_gas_dms_get_ethernet_pdu_mac_address_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_gas_dms_get_ethernet_pdu_mac_address_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get Ethernet PDU MAC address: %s\n", error->message);
        operation_shutdown (FALSE);
        return;
    }

    if (qmi_message_gas_dms_get_ethernet_pdu_mac_address_output_get_mac_address_0 (output, &address, &error)) {
        g_print ("Ethernet MAC address 0: ");
        print_mac_address (address);
    } else {
        g_printerr ("error: couldn't get Ethernet PDU MAC address 0: %s\n", error->message);
        operation_shutdown (FALSE);
        return;
    }

    if (qmi_message_gas_dms_get_ethernet_pdu_mac_address_output_get_mac_address_1 (output, &address, NULL)) {
        g_print ("Ethernet MAC address 1: ");
        print_mac_address (address);
    }

    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_GAS_DMS_GET_ETHERNET_PDU_MAC_ADDRESS */

#if defined HAVE_QMI_MESSAGE_GAS_DMS_SET_FIRMWARE_AUTO_SIM

typedef struct {
    QmiGasFirmwareAutoSimMode auto_sim_mode;
    guint8 config_id;
} FirmwareAutoSimProperties;

static gboolean
set_firmware_auto_sim_properties_handle (const gchar  *key,
                                         const gchar  *value,
                                         GError      **error,
                                         gpointer      user_data)
{
    FirmwareAutoSimProperties *props = user_data;

    if (!value || !value[0]) {
        g_set_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_FAILED,
                     "key '%s' requires a value", key);
        return FALSE;
    }

    if (g_ascii_strcasecmp (key, "mode") == 0) {
        if (!qmicli_read_gas_firmware_auto_sim_mode_from_string (value, &props->auto_sim_mode)) {
            g_set_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_FAILED,
                         "failed reading key 'mode' unrecognized value: '%s'", value);
            return FALSE;
        }
        return TRUE;
    }

    if (g_ascii_strcasecmp (key, "config-id") == 0) {
        guint aux;

        if (!qmicli_read_uint_from_string (value, &aux) || (aux >= G_MAXUINT8)) {
            g_set_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_FAILED,
                         "failed reading key 'config-id' value in range [0,254]: '%s'", value);
            return FALSE;
        }
        props->config_id = (guint8) aux;
        return TRUE;
    }

    g_set_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_FAILED,
                 "unrecognized key: '%s'", key);
    return FALSE;
}

static QmiMessageGasDmsSetFirmwareAutoSimInput *
set_firmware_auto_sim_input_create (const gchar *str)
{
    g_autoptr(QmiMessageGasDmsSetFirmwareAutoSimInput) input = NULL;
    g_autoptr(GError)                                  error = NULL;
    FirmwareAutoSimProperties props = {
        .auto_sim_mode = 0xFF,
        .config_id = 0xFF,
    };

    if (!str[0])
        return NULL;

    if (!qmicli_parse_key_value_string (str,
                                        &error,
                                        set_firmware_auto_sim_properties_handle,
                                        &props)) {
        g_printerr ("error: could not parse input string '%s'\n", error->message);
        return NULL;
    }

    if (props.auto_sim_mode == 0xFF) {
        g_printerr ("error: 'mode' is mandatory\n");
        return NULL;
    }

    input = qmi_message_gas_dms_set_firmware_auto_sim_input_new ();
    if (!qmi_message_gas_dms_set_firmware_auto_sim_input_set_auto_sim_mode (input, props.auto_sim_mode, &error)) {
        g_printerr ("error: couldn't set auto sim mode: '%s'\n", error->message);
        return NULL;
    }
    if (props.config_id != 0xFF) {
        if (!qmi_message_gas_dms_set_firmware_auto_sim_input_set_config_id (input, props.config_id, &error)) {
            g_printerr ("error: couldn't set config id: '%s'\n", error->message);
            return NULL;
        }
    }

    return g_steal_pointer (&input);
}

static void
set_firmware_auto_sim_ready (QmiClientGas *client,
                             GAsyncResult *res)
{
    g_autoptr(QmiMessageGasDmsSetFirmwareAutoSimOutput) output = NULL;
    g_autoptr(GError)                                   error = NULL;

    output = qmi_client_gas_dms_set_firmware_auto_sim_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_gas_dms_set_firmware_auto_sim_output_get_result (output, &error)) {
        g_printerr ("error: couldn't set firmware auto sim: %s\n", error->message);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("Firmware auto sim successfully updated\n");
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_GAS_DMS_SET_FIRMWARE_AUTO_SIM */

#if defined HAVE_QMI_MESSAGE_GAS_DMS_GET_FIRMWARE_AUTO_SIM

static void
get_firmware_auto_sim_ready (QmiClientGas *client,
                             GAsyncResult *res)
{
    g_autoptr(QmiMessageGasDmsGetFirmwareAutoSimOutput) output = NULL;
    QmiGasFirmwareAutoSimMode auto_sim_mode;
    guint8 config_id;
    g_autoptr(GError) error = NULL;

    output = qmi_client_gas_dms_get_firmware_auto_sim_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_gas_dms_get_firmware_auto_sim_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get firmware auto sim values: %s\n", error->message);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_gas_dms_get_firmware_auto_sim_output_get_auto_sim_mode (output, &auto_sim_mode, &error)) {
        g_printerr ("error: couldn't get firmware auto sim mode: %s\n", error->message);
        operation_shutdown (FALSE);
        return;
    }
    g_print ("Firmware auto sim mode: %s\n", qmi_gas_firmware_auto_sim_mode_get_string (auto_sim_mode));

    if (qmi_message_gas_dms_get_firmware_auto_sim_output_get_config_id (output, &config_id, &error)) {
        g_print ("Firmware auto sim config id: %u\n", config_id);
    }

    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_GAS_DMS_GET_FIRMWARE_AUTO_SIM */

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

#if defined HAVE_QMI_MESSAGE_GAS_DMS_SET_USB_COMPOSITION
    if (set_usb_composition_int >= 0) {
        QmiMessageGasDmsSetUsbCompositionInput *input;

        input = qmi_message_gas_dms_set_usb_composition_input_new ();
        qmi_message_gas_dms_set_usb_composition_input_set_usb_composition (input, set_usb_composition_int, NULL);
        qmi_message_gas_dms_set_usb_composition_input_set_endpoint_type (input, QMI_GAS_USB_COMPOSITION_ENDPOINT_TYPE_HSUSB, NULL);
        qmi_message_gas_dms_set_usb_composition_input_set_composition_persistence (input, TRUE, NULL);
        qmi_message_gas_dms_set_usb_composition_input_set_immediate_setting (input, FALSE, NULL);
        qmi_message_gas_dms_set_usb_composition_input_set_reboot_after_setting (input, TRUE, NULL);
        g_debug ("Asynchronously switching the USB composition...");
        qmi_client_gas_dms_set_usb_composition (ctx->client,
                                                input,
                                                10,
                                                ctx->cancellable,
                                                (GAsyncReadyCallback)set_usb_composition_ready,
                                                NULL);
        qmi_message_gas_dms_set_usb_composition_input_unref (input);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_GAS_DMS_GET_USB_COMPOSITION
    if (get_usb_composition_flag) {
        g_debug ("Asynchronously getting the USB composition...");
        qmi_client_gas_dms_get_usb_composition (ctx->client,
                                                NULL,
                                                10,
                                                ctx->cancellable,
                                                (GAsyncReadyCallback)get_usb_composition_ready,
                                                NULL);
        return;
    }
#endif

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

#if defined HAVE_QMI_MESSAGE_GAS_DMS_GET_ETHERNET_PDU_MAC_ADDRESS
    if (get_ethernet_mac_address_flag) {
        g_debug ("Asynchronously getting ethernet mac adress...");
        qmi_client_gas_dms_get_ethernet_pdu_mac_address (ctx->client,
                                                         NULL,
                                                         10,
                                                         ctx->cancellable,
                                                         (GAsyncReadyCallback)get_ethernet_pdu_mac_address_ready,
                                                         NULL);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_GAS_DMS_SET_FIRMWARE_AUTO_SIM
    if (set_firmware_auto_sim_str) {
        g_autoptr(QmiMessageGasDmsSetFirmwareAutoSimInput) input = NULL;

        input = set_firmware_auto_sim_input_create (set_firmware_auto_sim_str);
        if (input) {
            g_debug ("Asynchronously setting firmware auto sim...");
            qmi_client_gas_dms_set_firmware_auto_sim (client,
                                                      input,
                                                      10,
                                                      ctx->cancellable,
                                                      (GAsyncReadyCallback)set_firmware_auto_sim_ready,
                                                      NULL);
        } else
            operation_shutdown (FALSE);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_GAS_DMS_GET_FIRMWARE_AUTO_SIM
    if (get_firmware_auto_sim_flag) {
        g_debug ("Asynchronously getting firmware auto sim...");
        qmi_client_gas_dms_get_firmware_auto_sim (ctx->client,
                                                  NULL,
                                                  10,
                                                  ctx->cancellable,
                                                  (GAsyncReadyCallback)get_firmware_auto_sim_ready,
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

#endif /* HAVE_QMI_SERVICE_GAS */
