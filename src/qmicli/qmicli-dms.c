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
 * Copyright (C) 2012-2017 Aleksander Morgado <aleksander@aleksander.es>
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

#if defined HAVE_QMI_SERVICE_DMS

#undef VALIDATE_UNKNOWN
#define VALIDATE_UNKNOWN(str) (str ? str : "unknown")

#undef VALIDATE_MASK_NONE
#define VALIDATE_MASK_NONE(str) (str ? str : "none")

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
static gboolean uim_get_state_flag;
static gchar *uim_get_ck_status_str;
static gchar *uim_set_ck_protection_str;
static gchar *uim_unblock_ck_str;
static gboolean get_hardware_revision_flag;
static gboolean get_operating_mode_flag;
static gchar *set_operating_mode_str;
static gboolean get_time_flag;
static gboolean get_prl_version_flag;
static gboolean get_activation_state_flag;
static gchar *activate_automatic_str;
static gchar *activate_manual_str;
static gboolean get_user_lock_state_flag;
static gchar *set_user_lock_state_str;
static gchar *set_user_lock_code_str;
static gboolean read_user_data_flag;
static gchar *write_user_data_str;
static gboolean read_eri_file_flag;
static gchar *restore_factory_defaults_str;
static gchar *validate_service_programming_code_str;
static gboolean set_firmware_id_flag;
static gboolean get_band_capabilities_flag;
static gboolean get_factory_sku_flag;
static gboolean list_stored_images_flag;
static gchar *select_stored_image_str;
static gchar *delete_stored_image_str;
static gboolean get_firmware_preference_flag;
static gchar *set_firmware_preference_str;
static gboolean get_boot_image_download_mode_flag;
static gchar *set_boot_image_download_mode_str;
static gboolean get_software_version_flag;
static gboolean set_fcc_authentication_flag;
static gboolean get_supported_messages_flag;
static gchar *hp_change_device_mode_str;
static gboolean swi_get_current_firmware_flag;
static gboolean swi_get_usb_composition_flag;
static gchar *swi_set_usb_composition_str;
static gchar *dell_change_device_mode_str; /* deprecated */
static gchar *foxconn_change_device_mode_str;
static gchar *dell_get_firmware_version_str; /* deprecated */
static gchar *foxconn_get_firmware_version_str;
static gint foxconn_set_fcc_authentication_int = -1;
static gchar *get_mac_address_str;
static gboolean reset_flag;
static gboolean noop_flag;

static GOptionEntry entries[] = {
#if defined HAVE_QMI_MESSAGE_DMS_GET_IDS
    { "dms-get-ids", 0, 0, G_OPTION_ARG_NONE, &get_ids_flag,
      "Get IDs",
      NULL
    },
#endif
#if defined HAVE_QMI_MESSAGE_DMS_GET_CAPABILITIES
    { "dms-get-capabilities", 0, 0, G_OPTION_ARG_NONE, &get_capabilities_flag,
      "Get capabilities",
      NULL
    },
#endif
#if defined HAVE_QMI_MESSAGE_DMS_GET_MANUFACTURER
    { "dms-get-manufacturer", 0, 0, G_OPTION_ARG_NONE, &get_manufacturer_flag,
      "Get manufacturer",
      NULL
    },
#endif
#if defined HAVE_QMI_MESSAGE_DMS_GET_MODEL
    { "dms-get-model", 0, 0, G_OPTION_ARG_NONE, &get_model_flag,
      "Get model",
      NULL
    },
#endif
#if defined HAVE_QMI_MESSAGE_DMS_GET_REVISION
    { "dms-get-revision", 0, 0, G_OPTION_ARG_NONE, &get_revision_flag,
      "Get revision",
      NULL
    },
#endif
#if defined HAVE_QMI_MESSAGE_DMS_GET_MSISDN
    { "dms-get-msisdn", 0, 0, G_OPTION_ARG_NONE, &get_msisdn_flag,
      "Get MSISDN",
      NULL
    },
#endif
#if defined HAVE_QMI_MESSAGE_DMS_GET_POWER_STATE
    { "dms-get-power-state", 0, 0, G_OPTION_ARG_NONE, &get_power_state_flag,
      "Get power state",
      NULL
    },
#endif
#if defined HAVE_QMI_MESSAGE_DMS_UIM_SET_PIN_PROTECTION
    { "dms-uim-set-pin-protection", 0, 0, G_OPTION_ARG_STRING, &uim_set_pin_protection_str,
      "Set PIN protection in the UIM",
      "[(PIN|PIN2),(disable|enable),(current PIN)]",
    },
#endif
#if defined HAVE_QMI_MESSAGE_DMS_UIM_VERIFY_PIN
    { "dms-uim-verify-pin", 0, 0, G_OPTION_ARG_STRING, &uim_verify_pin_str,
      "Verify PIN",
      "[(PIN|PIN2),(current PIN)]",
    },
#endif
#if defined HAVE_QMI_MESSAGE_DMS_UIM_UNBLOCK_PIN
    { "dms-uim-unblock-pin", 0, 0, G_OPTION_ARG_STRING, &uim_unblock_pin_str,
      "Unblock PIN",
      "[(PIN|PIN2),(PUK),(new PIN)]",
    },
#endif
#if defined HAVE_QMI_MESSAGE_DMS_UIM_CHANGE_PIN
    { "dms-uim-change-pin", 0, 0, G_OPTION_ARG_STRING, &uim_change_pin_str,
      "Change PIN",
      "[(PIN|PIN2),(old PIN),(new PIN)]",
    },
#endif
#if defined HAVE_QMI_MESSAGE_DMS_UIM_GET_PIN_STATUS
    { "dms-uim-get-pin-status", 0, 0, G_OPTION_ARG_NONE, &uim_get_pin_status_flag,
      "Get PIN status",
      NULL
    },
#endif
#if defined HAVE_QMI_MESSAGE_DMS_UIM_GET_ICCID
    { "dms-uim-get-iccid", 0, 0, G_OPTION_ARG_NONE, &uim_get_iccid_flag,
      "Get ICCID",
      NULL
    },
#endif
#if defined HAVE_QMI_MESSAGE_DMS_UIM_GET_IMSI
    { "dms-uim-get-imsi", 0, 0, G_OPTION_ARG_NONE, &uim_get_imsi_flag,
      "Get IMSI",
      NULL
    },
#endif
#if defined HAVE_QMI_MESSAGE_DMS_UIM_GET_STATE
    { "dms-uim-get-state", 0, 0, G_OPTION_ARG_NONE, &uim_get_state_flag,
      "Get UIM State",
      NULL
    },
#endif
#if defined HAVE_QMI_MESSAGE_DMS_UIM_GET_CK_STATUS
    { "dms-uim-get-ck-status", 0, 0, G_OPTION_ARG_STRING, &uim_get_ck_status_str,
      "Get CK Status",
      "[(pn|pu|pp|pc|pf)]"
    },
#endif
#if defined HAVE_QMI_MESSAGE_DMS_UIM_SET_CK_PROTECTION
    { "dms-uim-set-ck-protection", 0, 0, G_OPTION_ARG_STRING, &uim_set_ck_protection_str,
      "Disable CK protection",
      "[(pn|pu|pp|pc|pf),(disable),(key)]"
    },
#endif
    { "dms-uim-unblock-ck", 0, 0, G_OPTION_ARG_STRING, &uim_unblock_ck_str,
      "Unblock CK",
      "[(pn|pu|pp|pc|pf),(key)]"
    },
#if defined HAVE_QMI_MESSAGE_DMS_GET_HARDWARE_REVISION
    { "dms-get-hardware-revision", 0, 0, G_OPTION_ARG_NONE, &get_hardware_revision_flag,
      "Get the HW revision",
      NULL
    },
#endif
#if defined HAVE_QMI_MESSAGE_DMS_GET_OPERATING_MODE
    { "dms-get-operating-mode", 0, 0, G_OPTION_ARG_NONE, &get_operating_mode_flag,
      "Get the device operating mode",
      NULL
    },
#endif
#if defined HAVE_QMI_MESSAGE_DMS_SET_OPERATING_MODE
    { "dms-set-operating-mode", 0, 0, G_OPTION_ARG_STRING, &set_operating_mode_str,
      "Set the device operating mode",
      "[(Operating mode)]"
    },
#endif
#if defined HAVE_QMI_MESSAGE_DMS_GET_TIME
    { "dms-get-time", 0, 0, G_OPTION_ARG_NONE, &get_time_flag,
      "Get the device time",
      NULL,
    },
#endif
#if defined HAVE_QMI_MESSAGE_DMS_GET_PRL_VERSION
    { "dms-get-prl-version", 0, 0, G_OPTION_ARG_NONE, &get_prl_version_flag,
      "Get the PRL version",
      NULL,
    },
#endif
#if defined HAVE_QMI_MESSAGE_DMS_GET_ACTIVATION_STATE
    { "dms-get-activation-state", 0, 0, G_OPTION_ARG_NONE, &get_activation_state_flag,
      "Get the state of the service activation",
      NULL,
    },
#endif
#if defined HAVE_QMI_MESSAGE_DMS_ACTIVATE_AUTOMATIC
    { "dms-activate-automatic", 0, 0, G_OPTION_ARG_STRING, &activate_automatic_str,
      "Request automatic service activation",
      "[Activation Code]"
    },
#endif
#if defined HAVE_QMI_MESSAGE_DMS_ACTIVATE_MANUAL
    { "dms-activate-manual", 0, 0, G_OPTION_ARG_STRING, &activate_manual_str,
      "Request manual service activation",
      "[SPC,SID,MDN,MIN]"
    },
#endif
#if defined HAVE_QMI_MESSAGE_DMS_GET_USER_LOCK_STATE
    { "dms-get-user-lock-state", 0, 0, G_OPTION_ARG_NONE, &get_user_lock_state_flag,
      "Get the state of the user lock",
      NULL,
    },
#endif
#if defined HAVE_QMI_MESSAGE_DMS_SET_USER_LOCK_STATE
    { "dms-set-user-lock-state", 0, 0, G_OPTION_ARG_STRING, &set_user_lock_state_str,
      "Set the state of the user lock",
      "[(disable|enable),(current lock code)]",
    },
#endif
#if defined HAVE_QMI_MESSAGE_DMS_SET_USER_LOCK_CODE
    { "dms-set-user-lock-code", 0, 0, G_OPTION_ARG_STRING, &set_user_lock_code_str,
      "Change the user lock code",
      "[(old lock code),(new lock code)]",
    },
#endif
#if defined HAVE_QMI_MESSAGE_DMS_READ_USER_DATA
    { "dms-read-user-data", 0, 0, G_OPTION_ARG_NONE, &read_user_data_flag,
      "Read user data",
      NULL,
    },
#endif
#if defined HAVE_QMI_MESSAGE_DMS_WRITE_USER_DATA
    { "dms-write-user-data", 0, 0, G_OPTION_ARG_STRING, &write_user_data_str,
      "Write user data",
      "[(User data)]",
    },
#endif
#if defined HAVE_QMI_MESSAGE_DMS_READ_ERI_FILE
    { "dms-read-eri-file", 0, 0, G_OPTION_ARG_NONE, &read_eri_file_flag,
      "Read ERI file",
      NULL,
    },
#endif
#if defined HAVE_QMI_MESSAGE_DMS_RESTORE_FACTORY_DEFAULTS
    { "dms-restore-factory-defaults", 0, 0, G_OPTION_ARG_STRING, &restore_factory_defaults_str,
      "Restore factory defaults",
      "[(Service Programming Code)]",
    },
#endif
#if defined HAVE_QMI_MESSAGE_DMS_VALIDATE_SERVICE_PROGRAMMING_CODE
    { "dms-validate-service-programming-code", 0, 0, G_OPTION_ARG_STRING, &validate_service_programming_code_str,
      "Validate the Service Programming Code",
      "[(Service Programming Code)]",
    },
#endif
#if defined HAVE_QMI_MESSAGE_DMS_SET_FIRMWARE_ID
    { "dms-set-firmware-id", 0, 0, G_OPTION_ARG_NONE, &set_firmware_id_flag,
      "Set firmware id",
      NULL
    },
#endif
#if defined HAVE_QMI_MESSAGE_DMS_GET_BAND_CAPABILITIES
    { "dms-get-band-capabilities", 0, 0, G_OPTION_ARG_NONE, &get_band_capabilities_flag,
      "Get band capabilities",
      NULL
    },
#endif
#if defined HAVE_QMI_MESSAGE_DMS_GET_FACTORY_SKU
    { "dms-get-factory-sku", 0, 0, G_OPTION_ARG_NONE, &get_factory_sku_flag,
      "Get factory stock keeping unit",
      NULL
    },
#endif
#if defined HAVE_QMI_MESSAGE_DMS_LIST_STORED_IMAGES && defined HAVE_QMI_MESSAGE_DMS_GET_STORED_IMAGE_INFO
    { "dms-list-stored-images", 0, 0, G_OPTION_ARG_NONE, &list_stored_images_flag,
      "List stored images",
      NULL
    },
#endif
#if defined HAVE_QMI_MESSAGE_DMS_SET_FIRMWARE_PREFERENCE && \
    defined HAVE_QMI_MESSAGE_DMS_LIST_STORED_IMAGES
    { "dms-select-stored-image", 0, 0, G_OPTION_ARG_STRING, &select_stored_image_str,
      "Select stored image",
      "[modem#,pri#] where # is the index"
    },
#endif
#if defined HAVE_QMI_MESSAGE_DMS_SET_FIRMWARE_PREFERENCE && \
    defined HAVE_QMI_MESSAGE_DMS_LIST_STORED_IMAGES && \
    defined HAVE_QMI_MESSAGE_DMS_DELETE_STORED_IMAGE
    { "dms-delete-stored-image", 0, 0, G_OPTION_ARG_STRING, &delete_stored_image_str,
      "Delete stored image",
      "[modem#|pri#] where # is the index"
    },
#endif
#if defined HAVE_QMI_MESSAGE_DMS_GET_FIRMWARE_PREFERENCE
    { "dms-get-firmware-preference", 0, 0, G_OPTION_ARG_NONE, &get_firmware_preference_flag,
      "Get firmware preference",
      NULL
    },
#endif
#if defined HAVE_QMI_MESSAGE_DMS_SET_FIRMWARE_PREFERENCE
    { "dms-set-firmware-preference", 0, 0, G_OPTION_ARG_STRING, &set_firmware_preference_str,
      "Set firmware preference (required keys: firmware-version, config-version, carrier; optional keys: modem-storage-index, override-download=yes)",
      "[\"key=value,...\"]"
    },
#endif
#if defined HAVE_QMI_MESSAGE_DMS_GET_BOOT_IMAGE_DOWNLOAD_MODE
    { "dms-get-boot-image-download-mode", 0, 0, G_OPTION_ARG_NONE, &get_boot_image_download_mode_flag,
      "Get boot image download mode",
      NULL
    },
#endif
#if defined HAVE_QMI_MESSAGE_DMS_SET_BOOT_IMAGE_DOWNLOAD_MODE
    { "dms-set-boot-image-download-mode", 0, 0, G_OPTION_ARG_STRING, &set_boot_image_download_mode_str,
      "Set boot image download mode",
      "[normal|boot-and-recovery]"
    },
#endif
#if defined HAVE_QMI_MESSAGE_DMS_GET_SOFTWARE_VERSION
    { "dms-get-software-version", 0, 0, G_OPTION_ARG_NONE, &get_software_version_flag,
      "Get software version",
      NULL
    },
#endif
#if defined HAVE_QMI_MESSAGE_DMS_SET_FCC_AUTHENTICATION
    { "dms-set-fcc-authentication", 0, 0, G_OPTION_ARG_NONE, &set_fcc_authentication_flag,
      "Set FCC authentication",
      NULL
    },
#endif
#if defined HAVE_QMI_MESSAGE_DMS_GET_SUPPORTED_MESSAGES
    { "dms-get-supported-messages", 0, 0, G_OPTION_ARG_NONE, &get_supported_messages_flag,
      "Get supported messages",
      NULL
    },
#endif
#if defined HAVE_QMI_MESSAGE_DMS_HP_CHANGE_DEVICE_MODE
    { "dms-hp-change-device-mode", 0, 0, G_OPTION_ARG_STRING, &hp_change_device_mode_str,
      "Change device mode (HP specific)",
      "[fastboot]"
    },
#endif
#if defined HAVE_QMI_MESSAGE_DMS_SWI_GET_CURRENT_FIRMWARE
    { "dms-swi-get-current-firmware", 0, 0, G_OPTION_ARG_NONE, &swi_get_current_firmware_flag,
      "Get Current Firmware (Sierra Wireless specific)",
      NULL
    },
#endif
#if defined HAVE_QMI_MESSAGE_DMS_SWI_GET_USB_COMPOSITION
    { "dms-swi-get-usb-composition", 0, 0, G_OPTION_ARG_NONE, &swi_get_usb_composition_flag,
      "Get current and supported USB compositions (Sierra Wireless specific)",
      NULL
    },
#endif
#if defined HAVE_QMI_MESSAGE_DMS_SWI_SET_USB_COMPOSITION
    { "dms-swi-set-usb-composition", 0, 0, G_OPTION_ARG_STRING, &swi_set_usb_composition_str,
      "Set USB composition (Sierra Wireless specific)",
      "[#]"
    },
#endif
#if defined HAVE_QMI_MESSAGE_DMS_FOXCONN_CHANGE_DEVICE_MODE
    { "dms-foxconn-change-device-mode", 0, 0, G_OPTION_ARG_STRING, &foxconn_change_device_mode_str,
      "Change device mode (Foxconn specific)",
      "[fastboot-ota|fastboot-online]"
    },
#endif
#if defined HAVE_QMI_MESSAGE_DMS_FOXCONN_GET_FIRMWARE_VERSION
    { "dms-foxconn-get-firmware-version", 0, 0, G_OPTION_ARG_STRING, &foxconn_get_firmware_version_str,
      "Get firmware version (Foxconn specific)",
      "[firmware-mcfg-apps|firmware-mcfg|apps]"
    },
#endif
#if defined HAVE_QMI_MESSAGE_DMS_FOXCONN_SET_FCC_AUTHENTICATION
    { "dms-foxconn-set-fcc-authentication", 0, 0, G_OPTION_ARG_INT, &foxconn_set_fcc_authentication_int,
      "Set FCC authentication (Foxconn specific)",
      "[magic]"
    },
#endif
#if defined HAVE_QMI_MESSAGE_DMS_GET_MAC_ADDRESS
    { "dms-get-mac-address", 0, 0, G_OPTION_ARG_STRING, &get_mac_address_str,
      "Get default MAC address",
      "[wlan|bt]"
    },
#endif
#if defined HAVE_QMI_MESSAGE_DMS_RESET
    { "dms-reset", 0, 0, G_OPTION_ARG_NONE, &reset_flag,
      "Reset the service state",
      NULL
    },
#endif
    { "dms-noop", 0, 0, G_OPTION_ARG_NONE, &noop_flag,
      "Just allocate or release a DMS client. Use with `--client-no-release-cid' and/or `--client-cid'",
      NULL
    },
    /* deprecated entries (hidden in --help) */
#if defined HAVE_QMI_MESSAGE_DMS_FOXCONN_CHANGE_DEVICE_MODE
    { "dms-dell-change-device-mode", 0, G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_STRING, &dell_change_device_mode_str,
      "Change device mode (DELL specific)",
      "[fastboot-ota|fastboot-online]"
    },
#endif
#if defined HAVE_QMI_MESSAGE_DMS_FOXCONN_GET_FIRMWARE_VERSION
    { "dms-dell-get-firmware-version", 0, G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_STRING, &dell_get_firmware_version_str,
      "Get firmware version (DELL specific)",
      "[firmware-mcfg-apps|firmware-mcfg|apps]"
    },
#endif
    { NULL }
};

GOptionGroup *
qmicli_dms_get_option_group (void)
{
    GOptionGroup *group;

    group = g_option_group_new ("dms",
                                "DMS options:",
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
                 uim_get_state_flag +
                 !!uim_get_ck_status_str +
                 !!uim_set_ck_protection_str +
                 !!uim_unblock_ck_str +
                 get_hardware_revision_flag +
                 get_operating_mode_flag +
                 !!set_operating_mode_str +
                 get_time_flag +
                 get_prl_version_flag +
                 get_activation_state_flag +
                 !!activate_automatic_str +
                 !!activate_manual_str +
                 get_user_lock_state_flag +
                 !!set_user_lock_state_str +
                 !!set_user_lock_code_str +
                 read_user_data_flag +
                 !!write_user_data_str +
                 read_eri_file_flag +
                 !!restore_factory_defaults_str +
                 !!validate_service_programming_code_str +
                 set_firmware_id_flag +
                 get_band_capabilities_flag +
                 get_factory_sku_flag +
                 list_stored_images_flag +
                 !!select_stored_image_str +
                 !!delete_stored_image_str +
                 get_firmware_preference_flag +
                 !!set_firmware_preference_str +
                 get_boot_image_download_mode_flag +
                 !!set_boot_image_download_mode_str +
                 get_software_version_flag +
                 set_fcc_authentication_flag +
                 get_supported_messages_flag +
                 !!hp_change_device_mode_str +
                 swi_get_current_firmware_flag +
                 swi_get_usb_composition_flag +
                 !!swi_set_usb_composition_str +
                 !!dell_change_device_mode_str +
                 !!foxconn_change_device_mode_str +
                 !!dell_get_firmware_version_str +
                 !!foxconn_get_firmware_version_str +
                 (foxconn_set_fcc_authentication_int >= 0) +
                 !!get_mac_address_str +
                 reset_flag +
                 noop_flag);

    if (n_actions > 1) {
        g_printerr ("error: too many DMS actions requested\n");
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

#if defined HAVE_QMI_MESSAGE_DMS_FOXCONN_CHANGE_DEVICE_MODE || \
    defined HAVE_QMI_MESSAGE_DMS_FOXCONN_CHANGE_DEVICE_MODE

static void
operation_shutdown_skip_cid_release (gboolean operation_status)
{
    /* Cleanup context and finish async operation. Explicitly ask not to release
     * the client CID. This is so that the qmicli operation doesn't fail after
     * this step, e.g. if the device just reboots after the action. */
    context_free (ctx);
    qmicli_async_operation_done (operation_status, TRUE);
}

#endif

#if defined HAVE_QMI_MESSAGE_DMS_GET_IDS

static void
get_ids_ready (QmiClientDms *client,
               GAsyncResult *res)
{
    const gchar *esn = NULL;
    const gchar *imei = NULL;
    const gchar *meid = NULL;
    const gchar *imei_software_version = NULL;
    QmiMessageDmsGetIdsOutput *output;
    GError *error = NULL;

    output = qmi_client_dms_get_ids_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_get_ids_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get IDs: %s\n", error->message);
        g_error_free (error);
        qmi_message_dms_get_ids_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    qmi_message_dms_get_ids_output_get_esn  (output, &esn, NULL);
    qmi_message_dms_get_ids_output_get_imei (output, &imei, NULL);
    qmi_message_dms_get_ids_output_get_meid (output, &meid, NULL);

    g_print ("[%s] Device IDs retrieved:\n"
             "\t    ESN: '%s'\n"
             "\t   IMEI: '%s'\n"
             "\t   MEID: '%s'\n",
             qmi_device_get_path_display (ctx->device),
             VALIDATE_UNKNOWN (esn),
             VALIDATE_UNKNOWN (imei),
             VALIDATE_UNKNOWN (meid));

    if (qmi_message_dms_get_ids_output_get_imei_software_version (output,
                                                                  &imei_software_version,
                                                                  NULL)) {
        g_print("\tIMEI SV: '%s'\n", imei_software_version);
    }

    qmi_message_dms_get_ids_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_DMS_GET_IDS */

#if defined HAVE_QMI_MESSAGE_DMS_GET_CAPABILITIES

static void
get_capabilities_ready (QmiClientDms *client,
                        GAsyncResult *res)
{
    QmiMessageDmsGetCapabilitiesOutput *output;
    guint32 max_tx_channel_rate;
    guint32 max_rx_channel_rate;
    QmiDmsDataServiceCapability data_service_capability;
    QmiDmsSimCapability sim_capability;
    GArray *radio_interface_list;
    GError *error = NULL;
    GString *networks;
    guint i;

    output = qmi_client_dms_get_capabilities_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_get_capabilities_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get capabilities: %s\n", error->message);
        g_error_free (error);
        qmi_message_dms_get_capabilities_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    qmi_message_dms_get_capabilities_output_get_info (output,
                                                      &max_tx_channel_rate,
                                                      &max_rx_channel_rate,
                                                      &data_service_capability,
                                                      &sim_capability,
                                                      &radio_interface_list,
                                                      NULL);

    networks = g_string_new ("");
    for (i = 0; i < radio_interface_list->len; i++) {
        g_string_append (networks,
                         qmi_dms_radio_interface_get_string (
                             g_array_index (radio_interface_list,
                                            QmiDmsRadioInterface,
                                            i)));
        if (i != radio_interface_list->len - 1)
            g_string_append (networks, ", ");
    }

    g_print ("[%s] Device capabilities retrieved:\n"
             "\tMax TX channel rate: '%u'\n"
             "\tMax RX channel rate: '%u'\n"
             "\t       Data Service: '%s'\n"
             "\t                SIM: '%s'\n"
             "\t           Networks: '%s'\n",
             qmi_device_get_path_display (ctx->device),
             max_tx_channel_rate,
             max_rx_channel_rate,
             qmi_dms_data_service_capability_get_string (data_service_capability),
             qmi_dms_sim_capability_get_string (sim_capability),
             networks->str);

    g_string_free (networks, TRUE);
    qmi_message_dms_get_capabilities_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_DMS_GET_CAPABILITIES */

#if defined HAVE_QMI_MESSAGE_DMS_GET_MANUFACTURER

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
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_get_manufacturer_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get manufacturer: %s\n", error->message);
        g_error_free (error);
        qmi_message_dms_get_manufacturer_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    qmi_message_dms_get_manufacturer_output_get_manufacturer (output, &str, NULL);

    g_print ("[%s] Device manufacturer retrieved:\n"
             "\tManufacturer: '%s'\n",
             qmi_device_get_path_display (ctx->device),
             VALIDATE_UNKNOWN (str));

    qmi_message_dms_get_manufacturer_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_DMS_GET_MANUFACTURER */

#if defined HAVE_QMI_MESSAGE_DMS_GET_MODEL

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
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_get_model_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get model: %s\n", error->message);
        g_error_free (error);
        qmi_message_dms_get_model_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    qmi_message_dms_get_model_output_get_model (output, &str, NULL);

    g_print ("[%s] Device model retrieved:\n"
             "\tModel: '%s'\n",
             qmi_device_get_path_display (ctx->device),
             VALIDATE_UNKNOWN (str));

    qmi_message_dms_get_model_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_DMS_GET_MODEL */

#if defined HAVE_QMI_MESSAGE_DMS_GET_REVISION

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
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_get_revision_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get revision: %s\n", error->message);
        g_error_free (error);
        qmi_message_dms_get_revision_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    qmi_message_dms_get_revision_output_get_revision (output, &str, NULL);

    g_print ("[%s] Device revision retrieved:\n"
             "\tRevision: '%s'\n",
             qmi_device_get_path_display (ctx->device),
             VALIDATE_UNKNOWN (str));

    qmi_message_dms_get_revision_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_DMS_GET_REVISION */

#if defined HAVE_QMI_MESSAGE_DMS_GET_MSISDN

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
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_get_msisdn_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get MSISDN: %s\n", error->message);
        g_error_free (error);
        qmi_message_dms_get_msisdn_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    qmi_message_dms_get_msisdn_output_get_msisdn (output, &str, NULL);

    g_print ("[%s] Device MSISDN retrieved:\n"
             "\tMSISDN: '%s'\n",
             qmi_device_get_path_display (ctx->device),
             VALIDATE_UNKNOWN (str));

    qmi_message_dms_get_msisdn_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_DMS_GET_MSISDN */

#if defined HAVE_QMI_MESSAGE_DMS_GET_POWER_STATE

static void
get_power_state_ready (QmiClientDms *client,
                       GAsyncResult *res)
{
    g_autofree gchar *power_state_str = NULL;
    guint8 power_state_flags;
    guint8 battery_level;
    QmiMessageDmsGetPowerStateOutput *output;
    GError *error = NULL;

    output = qmi_client_dms_get_power_state_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_get_power_state_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get power state: %s\n", error->message);
        g_error_free (error);
        qmi_message_dms_get_power_state_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    qmi_message_dms_get_power_state_output_get_info (output,
                                                     &power_state_flags,
                                                     &battery_level,
                                                     NULL);
    power_state_str = qmi_dms_power_state_build_string_from_mask ((QmiDmsPowerState)power_state_flags);

    g_print ("[%s] Device power state retrieved:\n"
             "\tPower state: '%s'\n"
             "\tBattery level: '%u %%'\n",
             qmi_device_get_path_display (ctx->device),
             VALIDATE_MASK_NONE (power_state_str),
             (guint)battery_level);

    qmi_message_dms_get_power_state_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_DMS_GET_POWER_STATE */

#if defined HAVE_QMI_MESSAGE_DMS_UIM_SET_PIN_PROTECTION

static QmiMessageDmsUimSetPinProtectionInput *
uim_set_pin_protection_input_create (const gchar *str)
{
    QmiMessageDmsUimSetPinProtectionInput *input = NULL;
    gchar **split;
    QmiDmsUimPinId pin_id;
    gboolean enable_disable;
    gchar *current_pin;

    /* Prepare inputs.
     * Format of the string is:
     *    "[(PIN|PIN2),(disable|enable),(current PIN)]"
     */
    split = g_strsplit (str, ",", -1);
    if (qmicli_read_dms_uim_pin_id_from_string (split[0], &pin_id) &&
        qmicli_read_enable_disable_from_string (split[1], &enable_disable) &&
        qmicli_read_non_empty_string (split[2], "current PIN", &current_pin)) {
        GError *error = NULL;

        input = qmi_message_dms_uim_set_pin_protection_input_new ();
        if (!qmi_message_dms_uim_set_pin_protection_input_set_info (
                input,
                pin_id,
                enable_disable,
                current_pin,
                &error)) {
            g_printerr ("error: couldn't create input data bundle: '%s'\n",
                        error->message);
            g_error_free (error);
            qmi_message_dms_uim_set_pin_protection_input_unref (input);
            input = NULL;
        }
    }
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
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_uim_set_pin_protection_output_get_result (output, &error)) {
        guint8 verify_retries_left;
        guint8 unblock_retries_left;

        g_printerr ("error: couldn't set PIN protection: %s\n", error->message);
        g_error_free (error);

        if (qmi_message_dms_uim_set_pin_protection_output_get_pin_retries_status (
                output,
                &verify_retries_left,
                &unblock_retries_left,
                NULL)) {
            g_printerr ("[%s] Retries left:\n"
                        "\tVerify: %u\n"
                        "\tUnblock: %u\n",
                        qmi_device_get_path_display (ctx->device),
                        verify_retries_left,
                        unblock_retries_left);
        }

        qmi_message_dms_uim_set_pin_protection_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] PIN protection updated\n",
             qmi_device_get_path_display (ctx->device));

    qmi_message_dms_uim_set_pin_protection_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_DMS_UIM_SET_PIN_PROTECTION */

#if defined HAVE_QMI_MESSAGE_DMS_UIM_VERIFY_PIN

static QmiMessageDmsUimVerifyPinInput *
uim_verify_pin_input_create (const gchar *str)
{
    QmiMessageDmsUimVerifyPinInput *input = NULL;
    gchar **split;
    QmiDmsUimPinId pin_id;
    gchar *current_pin;

    /* Prepare inputs.
     * Format of the string is:
     *    "[(PIN|PIN2),(current PIN)]"
     */
    split = g_strsplit (str, ",", -1);
    if (qmicli_read_dms_uim_pin_id_from_string (split[0], &pin_id) &&
        qmicli_read_non_empty_string (split[1], "current PIN", &current_pin)) {
        GError *error = NULL;

        input = qmi_message_dms_uim_verify_pin_input_new ();
        if (!qmi_message_dms_uim_verify_pin_input_set_info (
                input,
                pin_id,
                current_pin,
                &error)) {
            g_printerr ("error: couldn't create input data bundle: '%s'\n",
                        error->message);
            g_error_free (error);
            qmi_message_dms_uim_verify_pin_input_unref (input);
            input = NULL;
        }
    }
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
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_uim_verify_pin_output_get_result (output, &error)) {
        guint8 verify_retries_left;
        guint8 unblock_retries_left;

        g_printerr ("error: couldn't verify PIN: %s\n", error->message);
        g_error_free (error);

        if (qmi_message_dms_uim_verify_pin_output_get_pin_retries_status (
                output,
                &verify_retries_left,
                &unblock_retries_left,
                NULL)) {
            g_printerr ("[%s] Retries left:\n"
                        "\tVerify: %u\n"
                        "\tUnblock: %u\n",
                        qmi_device_get_path_display (ctx->device),
                        verify_retries_left,
                        unblock_retries_left);
        }

        qmi_message_dms_uim_verify_pin_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] PIN verified successfully\n",
             qmi_device_get_path_display (ctx->device));

    qmi_message_dms_uim_verify_pin_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_DMS_UIM_VERIFY_PIN */

#if defined HAVE_QMI_MESSAGE_DMS_UIM_UNBLOCK_PIN

static QmiMessageDmsUimUnblockPinInput *
uim_unblock_pin_input_create (const gchar *str)
{
    QmiMessageDmsUimUnblockPinInput *input = NULL;
    gchar **split;
    QmiDmsUimPinId pin_id;
    gchar *puk;
    gchar *new_pin;

    /* Prepare inputs.
     * Format of the string is:
     *    "[(PIN|PIN2),(PUK),(new PIN)]"
     */
    split = g_strsplit (str, ",", -1);
    if (qmicli_read_dms_uim_pin_id_from_string (split[0], &pin_id) &&
        qmicli_read_non_empty_string (split[1], "PUK", &puk) &&
        qmicli_read_non_empty_string (split[2], "new PIN", &new_pin)) {
        GError *error = NULL;

        input = qmi_message_dms_uim_unblock_pin_input_new ();
        if (!qmi_message_dms_uim_unblock_pin_input_set_info (
                input,
                pin_id,
                puk,
                new_pin,
                &error)) {
            g_printerr ("error: couldn't create input data bundle: '%s'\n",
                        error->message);
            g_error_free (error);
            qmi_message_dms_uim_unblock_pin_input_unref (input);
            input = NULL;
        }
    }
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
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_uim_unblock_pin_output_get_result (output, &error)) {
        guint8 verify_retries_left;
        guint8 unblock_retries_left;

        g_printerr ("error: couldn't unblock PIN: %s\n", error->message);
        g_error_free (error);

        if (qmi_message_dms_uim_unblock_pin_output_get_pin_retries_status (
                output,
                &verify_retries_left,
                &unblock_retries_left,
                NULL)) {
            g_printerr ("[%s] Retries left:\n"
                        "\tVerify: %u\n"
                        "\tUnblock: %u\n",
                        qmi_device_get_path_display (ctx->device),
                        verify_retries_left,
                        unblock_retries_left);
        }

        qmi_message_dms_uim_unblock_pin_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] PIN unblocked successfully\n",
             qmi_device_get_path_display (ctx->device));

    qmi_message_dms_uim_unblock_pin_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_DMS_UIM_UNBLOCK_PIN */

#if defined HAVE_QMI_MESSAGE_DMS_UIM_CHANGE_PIN

static QmiMessageDmsUimChangePinInput *
uim_change_pin_input_create (const gchar *str)
{
    QmiMessageDmsUimChangePinInput *input = NULL;
    gchar **split;
    QmiDmsUimPinId pin_id;
    gchar *old_pin;
    gchar *new_pin;

    /* Prepare inputs.
     * Format of the string is:
     *    "[(PIN|PIN2),(old PIN),(new PIN)]"
     */
    split = g_strsplit (str, ",", -1);
    if (qmicli_read_dms_uim_pin_id_from_string (split[0], &pin_id) &&
        qmicli_read_non_empty_string (split[1], "old PIN", &old_pin) &&
        qmicli_read_non_empty_string (split[2], "new PIN", &new_pin)) {
        GError *error = NULL;

        input = qmi_message_dms_uim_change_pin_input_new ();
        if (!qmi_message_dms_uim_change_pin_input_set_info (
                input,
                pin_id,
                old_pin,
                new_pin,
                &error)) {
            g_printerr ("error: couldn't create input data bundle: '%s'\n",
                        error->message);
            g_error_free (error);
            qmi_message_dms_uim_change_pin_input_unref (input);
            input = NULL;
        }
    }
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
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_uim_change_pin_output_get_result (output, &error)) {
        guint8 verify_retries_left;
        guint8 unblock_retries_left;

        g_printerr ("error: couldn't change PIN: %s\n", error->message);
        g_error_free (error);

        if (qmi_message_dms_uim_change_pin_output_get_pin_retries_status (
                output,
                &verify_retries_left,
                &unblock_retries_left,
                NULL)) {
            g_printerr ("[%s] Retries left:\n"
                        "\tVerify: %u\n"
                        "\tUnblock: %u\n",
                        qmi_device_get_path_display (ctx->device),
                        verify_retries_left,
                        unblock_retries_left);
        }

        qmi_message_dms_uim_change_pin_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] PIN changed successfully\n",
             qmi_device_get_path_display (ctx->device));

    qmi_message_dms_uim_change_pin_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_DMS_UIM_CHANGE_PIN */

#if defined HAVE_QMI_MESSAGE_DMS_UIM_GET_PIN_STATUS

static void
uim_get_pin_status_ready (QmiClientDms *client,
                          GAsyncResult *res)
{
    guint8 verify_retries_left;
    guint8 unblock_retries_left;
    QmiDmsUimPinStatus current_status;
    QmiMessageDmsUimGetPinStatusOutput *output;
    GError *error = NULL;

    output = qmi_client_dms_uim_get_pin_status_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_uim_get_pin_status_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get PIN status: %s\n", error->message);
        g_error_free (error);
        qmi_message_dms_uim_get_pin_status_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] PIN status retrieved successfully\n",
             qmi_device_get_path_display (ctx->device));

    if (qmi_message_dms_uim_get_pin_status_output_get_pin1_status (
            output,
            &current_status,
            &verify_retries_left,
            &unblock_retries_left,
            NULL)) {
        g_print ("[%s] PIN1:\n"
                 "\tStatus: %s\n"
                 "\tVerify: %u\n"
                 "\tUnblock: %u\n",
                 qmi_device_get_path_display (ctx->device),
                 qmi_dms_uim_pin_status_get_string (current_status),
                 verify_retries_left,
                 unblock_retries_left);
    }

    if (qmi_message_dms_uim_get_pin_status_output_get_pin2_status (
            output,
            &current_status,
            &verify_retries_left,
            &unblock_retries_left,
            NULL)) {
        g_print ("[%s] PIN2:\n"
                 "\tStatus: %s\n"
                 "\tVerify: %u\n"
                 "\tUnblock: %u\n",
                 qmi_device_get_path_display (ctx->device),
                 qmi_dms_uim_pin_status_get_string (current_status),
                 verify_retries_left,
                 unblock_retries_left);
    }

    qmi_message_dms_uim_get_pin_status_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_DMS_UIM_GET_PIN_STATUS */

#if defined HAVE_QMI_MESSAGE_DMS_UIM_GET_ICCID

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
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_uim_get_iccid_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get ICCID: %s\n", error->message);
        g_error_free (error);
        qmi_message_dms_uim_get_iccid_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    qmi_message_dms_uim_get_iccid_output_get_iccid (output, &str, NULL);

    g_print ("[%s] UIM ICCID retrieved:\n"
             "\tICCID: '%s'\n",
             qmi_device_get_path_display (ctx->device),
             VALIDATE_UNKNOWN (str));

    qmi_message_dms_uim_get_iccid_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_DMS_UIM_GET_ICCID */

#if defined HAVE_QMI_MESSAGE_DMS_UIM_GET_IMSI

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
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_uim_get_imsi_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get IMSI: %s\n", error->message);
        g_error_free (error);
        qmi_message_dms_uim_get_imsi_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    qmi_message_dms_uim_get_imsi_output_get_imsi (output, &str, NULL);

    g_print ("[%s] UIM IMSI retrieved:\n"
             "\tIMSI: '%s'\n",
             qmi_device_get_path_display (ctx->device),
             VALIDATE_UNKNOWN (str));

    qmi_message_dms_uim_get_imsi_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_DMS_UIM_GET_IMSI */

#if defined HAVE_QMI_MESSAGE_DMS_UIM_GET_STATE

static void
uim_get_state_ready (QmiClientDms *client,
                    GAsyncResult *res)
{
    QmiDmsUimState state;
    QmiMessageDmsUimGetStateOutput *output;
    GError *error = NULL;

    output = qmi_client_dms_uim_get_state_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_uim_get_state_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get UIM state: %s\n", error->message);
        g_error_free (error);
        qmi_message_dms_uim_get_state_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    qmi_message_dms_uim_get_state_output_get_state (output, &state, NULL);

    g_print ("[%s] UIM state retrieved:\n"
             "\tState: '%s'\n",
             qmi_device_get_path_display (ctx->device),
             qmi_dms_uim_state_get_string (state));

    qmi_message_dms_uim_get_state_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_DMS_UIM_GET_STATE */

#if defined HAVE_QMI_MESSAGE_DMS_UIM_GET_CK_STATUS

static QmiMessageDmsUimGetCkStatusInput *
uim_get_ck_status_input_create (const gchar *str)
{
    QmiMessageDmsUimGetCkStatusInput *input = NULL;
    QmiDmsUimFacility facility;

    if (qmicli_read_dms_uim_facility_from_string (str, &facility)) {
        GError *error = NULL;

        input = qmi_message_dms_uim_get_ck_status_input_new ();
        if (!qmi_message_dms_uim_get_ck_status_input_set_facility (
                input,
                facility,
                &error)) {
            g_printerr ("error: couldn't create input data bundle: '%s'\n",
                        error->message);
            g_error_free (error);
            qmi_message_dms_uim_get_ck_status_input_unref (input);
            input = NULL;
        }
    }

    return input;
}

static void
uim_get_ck_status_ready (QmiClientDms *client,
                         GAsyncResult *res)
{
    QmiMessageDmsUimGetCkStatusOutput *output;
    GError *error = NULL;
    QmiDmsUimFacilityState state;
    guint8 verify_retries_left;
    guint8 unblock_retries_left;
    gboolean blocking;

    output = qmi_client_dms_uim_get_ck_status_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_uim_get_ck_status_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get UIM CK status: %s\n", error->message);
        g_error_free (error);
        qmi_message_dms_uim_get_ck_status_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    qmi_message_dms_uim_get_ck_status_output_get_ck_status (
        output,
        &state,
        &verify_retries_left,
        &unblock_retries_left,
        NULL);

    g_print ("[%s] UIM facility state retrieved:\n"
             "\tState: '%s'\n",
             qmi_device_get_path_display (ctx->device),
             qmi_dms_uim_facility_state_get_string (state));
    g_print ("[%s] Retries left:\n"
             "\tVerify: %u\n"
             "\tUnblock: %u\n",
             qmi_device_get_path_display (ctx->device),
             verify_retries_left,
             unblock_retries_left);

    if (qmi_message_dms_uim_get_ck_status_output_get_operation_blocking_facility (
            output,
            &blocking,
            NULL) &&
        blocking) {
        g_print ("[%s] Facility is blocking operation\n",
                 qmi_device_get_path_display (ctx->device));
    }

    qmi_message_dms_uim_get_ck_status_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_DMS_UIM_GET_CK_STATUS */

#if defined HAVE_QMI_MESSAGE_DMS_UIM_SET_CK_PROTECTION

static QmiMessageDmsUimSetCkProtectionInput *
uim_set_ck_protection_input_create (const gchar *str)
{
    QmiMessageDmsUimSetCkProtectionInput *input = NULL;
    gchar **split;
    QmiDmsUimFacility facility;
    gboolean enable_disable;
    gchar *key;

    /* Prepare inputs.
     * Format of the string is:
     *    "[(facility),disable,(key)]"
     */
    split = g_strsplit (str, ",", -1);
    if (qmicli_read_dms_uim_facility_from_string (split[0], &facility) &&
        qmicli_read_enable_disable_from_string (split[1], &enable_disable) &&
        qmicli_read_non_empty_string (split[2], "control key", &key)) {

        /* We should only allow 'disable' here */
        if (enable_disable) {
            g_printerr ("error: only 'disable' action is allowed\n");
        } else {
            GError *error = NULL;

            input = qmi_message_dms_uim_set_ck_protection_input_new ();
            if (!qmi_message_dms_uim_set_ck_protection_input_set_facility (
                    input,
                    facility,
                    (QmiDmsUimFacilityState)enable_disable, /* 0 == DISABLE */
                    key,
                    &error)) {
                g_printerr ("error: couldn't create input data bundle: '%s'\n",
                            error->message);
                g_error_free (error);
                qmi_message_dms_uim_set_ck_protection_input_unref (input);
                input = NULL;
            }
        }
    }
    g_strfreev (split);

    return input;
}

static void
uim_set_ck_protection_ready (QmiClientDms *client,
                             GAsyncResult *res)
{
    QmiMessageDmsUimSetCkProtectionOutput *output;
    GError *error = NULL;

    output = qmi_client_dms_uim_set_ck_protection_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_uim_set_ck_protection_output_get_result (output, &error)) {
        guint8 verify_retries_left;

        g_printerr ("error: couldn't set UIM CK protection: %s\n", error->message);
        g_error_free (error);

        if (qmi_message_dms_uim_set_ck_protection_output_get_verify_retries_left (
                output,
                &verify_retries_left,
                NULL)) {
            g_printerr ("[%s] Retries left:\n"
                        "\tVerify: %u\n",
                        qmi_device_get_path_display (ctx->device),
                        verify_retries_left);
        }

        qmi_message_dms_uim_set_ck_protection_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] UIM CK protection set\n",
             qmi_device_get_path_display (ctx->device));

    qmi_message_dms_uim_set_ck_protection_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_DMS_UIM_SET_CK_PROTECTION */

#if defined HAVE_QMI_MESSAGE_DMS_UIM_UNBLOCK_CK

static QmiMessageDmsUimUnblockCkInput *
uim_unblock_ck_input_create (const gchar *str)
{
    QmiMessageDmsUimUnblockCkInput *input = NULL;
    gchar **split;
    QmiDmsUimFacility facility;
    gchar *key;

    /* Prepare inputs.
     * Format of the string is:
     *    "[(facility),(key)]"
     */
    split = g_strsplit (str, ",", -1);
    if (qmicli_read_dms_uim_facility_from_string (split[0], &facility) &&
        qmicli_read_non_empty_string (split[1], "control key", &key)) {
        GError *error = NULL;

        input = qmi_message_dms_uim_unblock_ck_input_new ();
        if (!qmi_message_dms_uim_unblock_ck_input_set_facility (
                input,
                facility,
                key,
                &error)) {
            g_printerr ("error: couldn't create input data bundle: '%s'\n",
                        error->message);
            g_error_free (error);
            qmi_message_dms_uim_unblock_ck_input_unref (input);
            input = NULL;
        }
    }
    g_strfreev (split);

    return input;
}

static void
uim_unblock_ck_ready (QmiClientDms *client,
                      GAsyncResult *res)
{
    QmiMessageDmsUimUnblockCkOutput *output;
    GError *error = NULL;

    output = qmi_client_dms_uim_unblock_ck_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_uim_unblock_ck_output_get_result (output, &error)) {
        guint8 unblock_retries_left;

        g_printerr ("error: couldn't unblock CK: %s\n", error->message);
        g_error_free (error);

        if (qmi_message_dms_uim_unblock_ck_output_get_unblock_retries_left (
                output,
                &unblock_retries_left,
                NULL)) {
            g_printerr ("[%s] Retries left:\n"
                        "\tUnblock: %u\n",
                        qmi_device_get_path_display (ctx->device),
                        unblock_retries_left);
        }

        qmi_message_dms_uim_unblock_ck_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] UIM CK unblocked\n",
             qmi_device_get_path_display (ctx->device));

    qmi_message_dms_uim_unblock_ck_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_DMS_UIM_UNBLOCK_CK */

#if defined HAVE_QMI_MESSAGE_DMS_GET_HARDWARE_REVISION

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
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_get_hardware_revision_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get the HW revision: %s\n", error->message);
        g_error_free (error);
        qmi_message_dms_get_hardware_revision_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    qmi_message_dms_get_hardware_revision_output_get_revision (output, &str, NULL);

    g_print ("[%s] Hardware revision retrieved:\n"
             "\tRevision: '%s'\n",
             qmi_device_get_path_display (ctx->device),
             VALIDATE_UNKNOWN (str));

    qmi_message_dms_get_hardware_revision_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_DMS_GET_HARDWARE_REVISION */

#if defined HAVE_QMI_MESSAGE_DMS_GET_OPERATING_MODE

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
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_get_operating_mode_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get the HW revision: %s\n", error->message);
        g_error_free (error);
        qmi_message_dms_get_operating_mode_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    qmi_message_dms_get_operating_mode_output_get_mode (output, &mode, NULL);

    g_print ("[%s] Operating mode retrieved:\n"
             "\tMode: '%s'\n",
             qmi_device_get_path_display (ctx->device),
             qmi_dms_operating_mode_get_string (mode));

    if (mode == QMI_DMS_OPERATING_MODE_OFFLINE || mode == QMI_DMS_OPERATING_MODE_LOW_POWER) {
        QmiDmsOfflineReason reason;

        if (qmi_message_dms_get_operating_mode_output_get_offline_reason (output, &reason, NULL)) {
            g_autofree gchar *reason_str = NULL;

            reason_str = qmi_dms_offline_reason_build_string_from_mask (reason);
            g_print ("\tReason: '%s'\n", VALIDATE_MASK_NONE (reason_str));
        }
    }

    qmi_message_dms_get_operating_mode_output_get_hardware_restricted_mode (output, &hw_restricted, NULL);
    g_print ("\tHW restricted: '%s'\n", hw_restricted ? "yes" : "no");

    qmi_message_dms_get_operating_mode_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_DMS_GET_OPERATING_MODE */

#if defined HAVE_QMI_MESSAGE_DMS_SET_OPERATING_MODE

static QmiMessageDmsSetOperatingModeInput *
set_operating_mode_input_create (const gchar *str)
{
    QmiMessageDmsSetOperatingModeInput *input = NULL;
    QmiDmsOperatingMode mode;

    if (qmicli_read_dms_operating_mode_from_string (str, &mode)) {
        GError *error = NULL;

        input = qmi_message_dms_set_operating_mode_input_new ();
        if (!qmi_message_dms_set_operating_mode_input_set_mode (
                input,
                mode,
                &error)) {
            g_printerr ("error: couldn't create input data bundle: '%s'\n",
                        error->message);
            g_error_free (error);
            qmi_message_dms_set_operating_mode_input_unref (input);
            input = NULL;
        }
    }

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
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_set_operating_mode_output_get_result (output, &error)) {
        g_printerr ("error: couldn't set operating mode: %s\n", error->message);
        g_error_free (error);
        qmi_message_dms_set_operating_mode_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] Operating mode set successfully\n",
             qmi_device_get_path_display (ctx->device));

    qmi_message_dms_set_operating_mode_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_DMS_SET_OPERATING_MODE */

#if defined HAVE_QMI_MESSAGE_DMS_GET_TIME

static void
get_time_ready (QmiClientDms *client,
                GAsyncResult *res)
{
    QmiMessageDmsGetTimeOutput *output;
    guint64 time_count;
    QmiDmsTimeSource time_source;
    GError *error = NULL;
    gchar *str;
    GTimeZone *time_zone;
    GDateTime *gpstime_epoch;
    GDateTime *computed_epoch;

    output = qmi_client_dms_get_time_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_get_time_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get the device time: %s\n", error->message);
        g_error_free (error);
        qmi_message_dms_get_time_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    qmi_message_dms_get_time_output_get_device_time (
        output,
        &time_count,
        &time_source,
        NULL);

    /* January 6th 1980 */
    time_zone = g_time_zone_new_utc ();
    gpstime_epoch = g_date_time_new (time_zone, 1980, 1, 6, 0, 0, 0.0);

    computed_epoch = g_date_time_add_seconds (gpstime_epoch, ((gdouble) time_count / 1000.0));
    str = g_date_time_format (computed_epoch, "%F %T");
    g_print ("[%s] Time retrieved:\n"
             "\tTime count: '%" G_GUINT64_FORMAT " (x 1.25ms): %s'\n"
             "\tTime source: '%s'\n",
             qmi_device_get_path_display (ctx->device),
             time_count, str,
             qmi_dms_time_source_get_string (time_source));
    g_free (str);
    g_date_time_unref (computed_epoch);

    if (qmi_message_dms_get_time_output_get_system_time (
        output,
        &time_count,
        NULL)){
        computed_epoch = g_date_time_add_seconds (gpstime_epoch, ((gdouble) time_count / 1000.0));
        str = g_date_time_format (computed_epoch, "%F %T");
        g_print ("\tSystem time: '%" G_GUINT64_FORMAT " (ms): %s'\n",
                 time_count, str);
        g_free (str);
        g_date_time_unref (computed_epoch);
    }

    if (qmi_message_dms_get_time_output_get_user_time (
        output,
        &time_count,
        NULL)){
        computed_epoch = g_date_time_add_seconds (gpstime_epoch, ((gdouble) time_count / 1000.0));
        str = g_date_time_format (computed_epoch, "%F %T");
        g_print ("\tUser time: '%" G_GUINT64_FORMAT " (ms): %s'\n",
                 time_count, str);
        g_free (str);
        g_date_time_unref (computed_epoch);
    }

    g_date_time_unref (gpstime_epoch);
    g_time_zone_unref (time_zone);

    qmi_message_dms_get_time_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_DMS_GET_TIME */

#if defined HAVE_QMI_MESSAGE_DMS_GET_PRL_VERSION

static void
get_prl_version_ready (QmiClientDms *client,
                       GAsyncResult *res)
{
    QmiMessageDmsGetPrlVersionOutput *output;
    guint16 prl_version;
    gboolean prl_only;
    GError *error = NULL;

    output = qmi_client_dms_get_prl_version_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_get_prl_version_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get the PRL version: %s\n", error->message);
        g_error_free (error);
        qmi_message_dms_get_prl_version_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    qmi_message_dms_get_prl_version_output_get_version (
        output,
        &prl_version,
        NULL);

    g_print ("[%s] PRL version retrieved:\n"
             "\tPRL version: '%" G_GUINT16_FORMAT "'\n",
             qmi_device_get_path_display (ctx->device),
             prl_version);

    if (qmi_message_dms_get_prl_version_output_get_prl_only_preference (
        output,
        &prl_only,
        NULL)){
        g_print ("\tPRL only preference: '%s'\n",
                 prl_only ? "yes" : "no");
    }

    qmi_message_dms_get_prl_version_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_DMS_GET_PRL_VERSION */

#if defined HAVE_QMI_MESSAGE_DMS_GET_ACTIVATION_STATE

static void
get_activation_state_ready (QmiClientDms *client,
                            GAsyncResult *res)
{
    QmiMessageDmsGetActivationStateOutput *output;
    QmiDmsActivationState activation_state;
    GError *error = NULL;

    output = qmi_client_dms_get_activation_state_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_get_activation_state_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get the state of the service activation: %s\n", error->message);
        g_error_free (error);
        qmi_message_dms_get_activation_state_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    qmi_message_dms_get_activation_state_output_get_info (
        output,
        &activation_state,
        NULL);

    g_print ("[%s] Activation state retrieved:\n"
             "\tState: '%s'\n",
             qmi_device_get_path_display (ctx->device),
             qmi_dms_activation_state_get_string (activation_state));

    qmi_message_dms_get_activation_state_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_DMS_GET_ACTIVATION_STATE */

#if defined HAVE_QMI_MESSAGE_DMS_ACTIVATE_MANUAL

static QmiMessageDmsActivateManualInput *
activate_manual_input_create (const gchar *str)
{
    QmiMessageDmsActivateManualInput *input;
    gchar **split;
    GError *error = NULL;
    gulong split_1_int;

    split = g_strsplit (str, ",", -1);
    if (g_strv_length (split) != 4) {
        g_printerr ("error: incorrect number of arguments given\n");
        g_strfreev (split);
        return NULL;
    }

    split_1_int = strtoul (split[1], NULL, 10);
    if (split_1_int > G_MAXUINT16) {
        g_printerr ("error: invalid SID given '%s'\n",
                    split[1]);
        return NULL;
    }

    input = qmi_message_dms_activate_manual_input_new ();
    if (!qmi_message_dms_activate_manual_input_set_info (
            input,
            split[0],
            (guint16)split_1_int,
            split[2],
            split[3],
            &error)) {
        g_printerr ("error: couldn't create input data bundle: '%s'\n",
                    error->message);
        g_error_free (error);
        qmi_message_dms_activate_manual_input_unref (input);
        input = NULL;
    }

    g_strfreev(split);
    return input;
}

static void
activate_manual_ready (QmiClientDms *client,
                       GAsyncResult *res)
{
    QmiMessageDmsActivateManualOutput *output;
    GError *error = NULL;

    output = qmi_client_dms_activate_manual_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_activate_manual_output_get_result (output, &error)) {
        g_printerr ("error: couldn't request manual service activation: %s\n", error->message);
        g_error_free (error);
        qmi_message_dms_activate_manual_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    qmi_message_dms_activate_manual_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_DMS_ACTIVATE_MANUAL */

#if defined HAVE_QMI_MESSAGE_DMS_ACTIVATE_AUTOMATIC

static QmiMessageDmsActivateAutomaticInput *
activate_automatic_input_create (const gchar *str)
{
    QmiMessageDmsActivateAutomaticInput *input;
    GError *error = NULL;

    input = qmi_message_dms_activate_automatic_input_new ();
    if (!qmi_message_dms_activate_automatic_input_set_activation_code (
            input,
            str,
            &error)) {
        g_printerr ("error: couldn't create input data bundle: '%s'\n",
                    error->message);
        g_error_free (error);
        qmi_message_dms_activate_automatic_input_unref (input);
        input = NULL;
    }

    return input;
}

static void
activate_automatic_ready (QmiClientDms *client,
                          GAsyncResult *res)
{
    QmiMessageDmsActivateAutomaticOutput *output;
    GError *error = NULL;

    output = qmi_client_dms_activate_automatic_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_activate_automatic_output_get_result (output, &error)) {
        g_printerr ("error: couldn't request automatic service activation: %s\n", error->message);
        g_error_free (error);
        qmi_message_dms_activate_automatic_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    qmi_message_dms_activate_automatic_output_unref (output);
    operation_shutdown (TRUE);
}

#endif

#if defined HAVE_QMI_MESSAGE_DMS_GET_USER_LOCK_STATE

static void
get_user_lock_state_ready (QmiClientDms *client,
                           GAsyncResult *res)
{
    QmiMessageDmsGetUserLockStateOutput *output;
    gboolean enabled;
    GError *error = NULL;

    output = qmi_client_dms_get_user_lock_state_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_get_user_lock_state_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get the state of the user lock: %s\n", error->message);
        g_error_free (error);
        qmi_message_dms_get_user_lock_state_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    qmi_message_dms_get_user_lock_state_output_get_enabled (
        output,
        &enabled,
        NULL);

    g_print ("[%s] User lock state retrieved:\n"
             "\tEnabled: '%s'\n",
             qmi_device_get_path_display (ctx->device),
             enabled ? "yes" : "no");

    qmi_message_dms_get_user_lock_state_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_DMS_GET_USER_LOCK_STATE */

#if defined HAVE_QMI_MESSAGE_DMS_SET_USER_LOCK_STATE

static QmiMessageDmsSetUserLockStateInput *
set_user_lock_state_input_create (const gchar *str)
{
    QmiMessageDmsSetUserLockStateInput *input = NULL;
    gchar **split;
    gboolean enable_disable;
    gchar *code;

    /* Prepare inputs.
     * Format of the string is:
     *    "[(disable|enable),(current lock code)]"
     */
    split = g_strsplit (str, ",", -1);

    if (qmicli_read_enable_disable_from_string (split[0], &enable_disable) &&
        qmicli_read_non_empty_string (split[1], "current lock code", &code)) {
        GError *error = NULL;

        input = qmi_message_dms_set_user_lock_state_input_new ();
        if (!qmi_message_dms_set_user_lock_state_input_set_info (
                input,
                enable_disable,
                code,
                &error)) {
            g_printerr ("error: couldn't create input data bundle: '%s'\n",
                        error->message);
            g_error_free (error);
            qmi_message_dms_set_user_lock_state_input_unref (input);
            input = NULL;
        }
    }
    g_strfreev (split);

    return input;
}

static void
set_user_lock_state_ready (QmiClientDms *client,
                           GAsyncResult *res)
{
    QmiMessageDmsSetUserLockStateOutput *output;
    GError *error = NULL;

    output = qmi_client_dms_set_user_lock_state_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_set_user_lock_state_output_get_result (output, &error)) {
        g_printerr ("error: couldn't set state of the user lock: %s\n", error->message);
        g_error_free (error);
        qmi_message_dms_set_user_lock_state_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] User lock state updated\n",
             qmi_device_get_path_display (ctx->device));

    qmi_message_dms_set_user_lock_state_output_unref (output);
    operation_shutdown (TRUE);
}

#endif

#if defined HAVE_QMI_MESSAGE_DMS_SET_USER_LOCK_CODE

static QmiMessageDmsSetUserLockCodeInput *
set_user_lock_code_input_create (const gchar *str)
{
    QmiMessageDmsSetUserLockCodeInput *input = NULL;
    gchar **split;
    gchar *old_code;
    gchar *new_code;

    /* Prepare inputs.
     * Format of the string is:
     *    "[(old lock code),(new lock code)]"
     */
    split = g_strsplit (str, ",", -1);
    if (qmicli_read_non_empty_string (split[0], "old lock code", &old_code) &&
        qmicli_read_non_empty_string (split[1], "new lock code", &new_code)) {
        GError *error = NULL;

        input = qmi_message_dms_set_user_lock_code_input_new ();
        if (!qmi_message_dms_set_user_lock_code_input_set_info (
                input,
                old_code,
                new_code,
                &error)) {
            g_printerr ("error: couldn't create input data bundle: '%s'\n",
                        error->message);
            g_error_free (error);
            qmi_message_dms_set_user_lock_code_input_unref (input);
            input = NULL;
        }
    }
    g_strfreev (split);

    return input;
}

static void
set_user_lock_code_ready (QmiClientDms *client,
                          GAsyncResult *res)
{
    QmiMessageDmsSetUserLockCodeOutput *output;
    GError *error = NULL;

    output = qmi_client_dms_set_user_lock_code_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_set_user_lock_code_output_get_result (output, &error)) {
        g_printerr ("error: couldn't change user lock code: %s\n", error->message);
        g_error_free (error);
        qmi_message_dms_set_user_lock_code_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] User lock code changed\n",
             qmi_device_get_path_display (ctx->device));

    qmi_message_dms_set_user_lock_code_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_DMS_SET_USER_LOCK_CODE */

#if defined HAVE_QMI_MESSAGE_DMS_READ_USER_DATA

static void
read_user_data_ready (QmiClientDms *client,
                      GAsyncResult *res)
{
    QmiMessageDmsReadUserDataOutput *output;
    GArray *user_data = NULL;
    gchar *user_data_printable;
    GError *error = NULL;

    output = qmi_client_dms_read_user_data_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_read_user_data_output_get_result (output, &error)) {
        g_printerr ("error: couldn't read user data: %s\n", error->message);
        g_error_free (error);
        qmi_message_dms_read_user_data_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    qmi_message_dms_read_user_data_output_get_user_data (
        output,
        &user_data,
        NULL);
    user_data_printable = qmicli_get_raw_data_printable (user_data, 80, "\t\t");

    g_print ("[%s] User data read:\n"
             "\tSize: '%u' bytes\n"
             "\tContents:\n"
             "%s",
             qmi_device_get_path_display (ctx->device),
             user_data->len,
             user_data_printable);
    g_free (user_data_printable);

    qmi_message_dms_read_user_data_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_DMS_READ_USER_DATA */

#if defined HAVE_QMI_MESSAGE_DMS_WRITE_USER_DATA

static QmiMessageDmsWriteUserDataInput *
write_user_data_input_create (const gchar *str)
{
    QmiMessageDmsWriteUserDataInput *input;
    GArray *array;
    GError *error = NULL;

    /* Prepare inputs. Just assume we'll get some text string here, although
     * nobody said this had to be text. Read User Data actually treats the
     * contents of the user data as raw binary data. */
    array = g_array_sized_new (FALSE, FALSE, 1, strlen (str));
    g_array_insert_vals (array, 0, str, strlen (str));
    input = qmi_message_dms_write_user_data_input_new ();
    if (!qmi_message_dms_write_user_data_input_set_user_data (
            input,
            array,
            &error)) {
        g_printerr ("error: couldn't create input data bundle: '%s'\n",
                    error->message);
        g_error_free (error);
        qmi_message_dms_write_user_data_input_unref (input);
        input = NULL;
    }
    g_array_unref (array);

    return input;
}

static void
write_user_data_ready (QmiClientDms *client,
                       GAsyncResult *res)
{
    QmiMessageDmsWriteUserDataOutput *output;
    GError *error = NULL;

    output = qmi_client_dms_write_user_data_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_write_user_data_output_get_result (output, &error)) {
        g_printerr ("error: couldn't write user data: %s\n", error->message);
        g_error_free (error);
        qmi_message_dms_write_user_data_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] User data written",
             qmi_device_get_path_display (ctx->device));

    qmi_message_dms_write_user_data_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_DMS_WRITE_USER_DATA */

#if defined HAVE_QMI_MESSAGE_DMS_READ_ERI_FILE

static void
read_eri_file_ready (QmiClientDms *client,
                     GAsyncResult *res)
{
    QmiMessageDmsReadEriFileOutput *output;
    GArray *eri_file = NULL;
    gchar *eri_file_printable;
    GError *error = NULL;

    output = qmi_client_dms_read_eri_file_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_read_eri_file_output_get_result (output, &error)) {
        g_printerr ("error: couldn't read eri file: %s\n", error->message);
        g_error_free (error);
        qmi_message_dms_read_eri_file_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    qmi_message_dms_read_eri_file_output_get_eri_file (
        output,
        &eri_file,
        NULL);
    eri_file_printable = qmicli_get_raw_data_printable (eri_file, 80, "\t\t");

    g_print ("[%s] ERI file read:\n"
             "\tSize: '%u' bytes\n"
             "\tContents:\n"
             "%s",
             qmi_device_get_path_display (ctx->device),
             eri_file->len,
             eri_file_printable);
    g_free (eri_file_printable);

    qmi_message_dms_read_eri_file_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_DMS_READ_ERI_FILE */

#if defined HAVE_QMI_MESSAGE_DMS_RESTORE_FACTORY_DEFAULTS

static QmiMessageDmsRestoreFactoryDefaultsInput *
restore_factory_defaults_input_create (const gchar *str)
{
    QmiMessageDmsRestoreFactoryDefaultsInput *input;
    GError *error = NULL;

    input = qmi_message_dms_restore_factory_defaults_input_new ();
    if (!qmi_message_dms_restore_factory_defaults_input_set_service_programming_code (
            input,
            str,
            &error)) {
        g_printerr ("error: couldn't create input data bundle: '%s'\n",
                    error->message);
        g_error_free (error);
        qmi_message_dms_restore_factory_defaults_input_unref (input);
        input = NULL;
    }

    return input;
}

static void
restore_factory_defaults_ready (QmiClientDms *client,
                                GAsyncResult *res)
{
    QmiMessageDmsRestoreFactoryDefaultsOutput *output;
    GError *error = NULL;

    output = qmi_client_dms_restore_factory_defaults_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_restore_factory_defaults_output_get_result (output, &error)) {
        g_printerr ("error: couldn't restores factory defaults: %s\n", error->message);
        g_error_free (error);
        qmi_message_dms_restore_factory_defaults_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] Factory defaults restored\n"
             "Device needs to get power-cycled for reset to take effect.\n",
             qmi_device_get_path_display (ctx->device));

    qmi_message_dms_restore_factory_defaults_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_DMS_RESTORE_FACTORY_DEFAULTS */

#if defined HAVE_QMI_MESSAGE_DMS_VALIDATE_SERVICE_PROGRAMMING_CODE

static QmiMessageDmsValidateServiceProgrammingCodeInput *
validate_service_programming_code_input_create (const gchar *str)
{
    QmiMessageDmsValidateServiceProgrammingCodeInput *input;
    GError *error = NULL;

    input = qmi_message_dms_validate_service_programming_code_input_new ();
    if (!qmi_message_dms_validate_service_programming_code_input_set_service_programming_code (
            input,
            str,
            &error)) {
        g_printerr ("error: couldn't create input data bundle: '%s'\n",
                    error->message);
        g_error_free (error);
        qmi_message_dms_validate_service_programming_code_input_unref (input);
        input = NULL;
    }

    return input;
}

static void
validate_service_programming_code_ready (QmiClientDms *client,
                                         GAsyncResult *res)
{
    QmiMessageDmsValidateServiceProgrammingCodeOutput *output;
    GError *error = NULL;

    output = qmi_client_dms_validate_service_programming_code_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_validate_service_programming_code_output_get_result (output, &error)) {
        g_printerr ("error: couldn't validate Service Programming Code: %s\n", error->message);
        g_error_free (error);
        qmi_message_dms_validate_service_programming_code_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] Service Programming Code validated\n",
             qmi_device_get_path_display (ctx->device));

    qmi_message_dms_validate_service_programming_code_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_DMS_VALIDATE_SERVICE_PROGRAMMING_CODE */

#if defined HAVE_QMI_MESSAGE_DMS_SET_FIRMWARE_ID

static void
set_firmware_id_ready (QmiClientDms *client,
                       GAsyncResult *res)
{
    QmiMessageDmsSetFirmwareIdOutput *output;
    GError *error = NULL;

    output = qmi_client_dms_set_firmware_id_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_set_firmware_id_output_get_result (output, &error)) {
        g_printerr ("error: couldn't set firmware id: %s\n", error->message);
        g_error_free (error);
        qmi_message_dms_set_firmware_id_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] Firmware id set\n",
             qmi_device_get_path_display (ctx->device));

    qmi_message_dms_set_firmware_id_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_DMS_SET_FIRMWARE_ID */

#if defined HAVE_QMI_MESSAGE_DMS_GET_BAND_CAPABILITIES

static void
get_band_capabilities_ready (QmiClientDms *client,
                             GAsyncResult *res)
{
    QmiMessageDmsGetBandCapabilitiesOutput *output;
    QmiDmsBandCapability band_capability;
    QmiDmsLteBandCapability lte_band_capability;
    GArray *extended_lte_band_capability;
    GError *error = NULL;

    output = qmi_client_dms_get_band_capabilities_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_get_band_capabilities_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get band capabilities: %s\n", error->message);
        g_error_free (error);
        qmi_message_dms_get_band_capabilities_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    if (qmi_message_dms_get_band_capabilities_output_get_band_capability (
            output,
            &band_capability,
            NULL)) {
        g_autofree gchar *str = NULL;

        str = qmi_dms_band_capability_build_string_from_mask (band_capability);
        g_print ("[%s] Device band capabilities retrieved:\n"
                 "\tBands: '%s'\n",
                 qmi_device_get_path_display (ctx->device),
                 VALIDATE_MASK_NONE (str));
    }

    if (qmi_message_dms_get_band_capabilities_output_get_lte_band_capability (
            output,
            &lte_band_capability,
            NULL)) {
        g_autofree gchar *str = NULL;

        str = qmi_dms_lte_band_capability_build_string_from_mask (lte_band_capability);
        g_print ("\tLTE bands: '%s'\n", VALIDATE_MASK_NONE (str));
    }

    if (qmi_message_dms_get_band_capabilities_output_get_extended_lte_band_capability (
            output,
            &extended_lte_band_capability,
            NULL)) {
        guint i;

        g_print ("\tLTE bands (extended): '");
        for (i = 0; i < extended_lte_band_capability->len; i++)
            g_print ("%s%" G_GUINT16_FORMAT,
                     i == 0 ? "" : ", ",
                     g_array_index (extended_lte_band_capability, guint16, i));
        g_print ("'\n");
    }

    qmi_message_dms_get_band_capabilities_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_DMS_GET_BAND_CAPABILITIES */

#if defined HAVE_QMI_MESSAGE_DMS_GET_FACTORY_SKU

static void
get_factory_sku_ready (QmiClientDms *client,
                       GAsyncResult *res)
{
    const gchar *str = NULL;
    QmiMessageDmsGetFactorySkuOutput *output;
    GError *error = NULL;

    output = qmi_client_dms_get_factory_sku_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_get_factory_sku_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get factory SKU: %s\n", error->message);
        g_error_free (error);
        qmi_message_dms_get_factory_sku_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    qmi_message_dms_get_factory_sku_output_get_sku (output, &str, NULL);

    g_print ("[%s] Device factory SKU retrieved:\n"
             "\tSKU: '%s'\n",
             qmi_device_get_path_display (ctx->device),
             VALIDATE_UNKNOWN (str));

    qmi_message_dms_get_factory_sku_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_DMS_GET_FACTORY_SKU */

#if defined HAVE_QMI_MESSAGE_DMS_LIST_STORED_IMAGES &&  \
    defined HAVE_QMI_MESSAGE_DMS_GET_STORED_IMAGE_INFO

typedef struct {
    QmiMessageDmsListStoredImagesOutput *list_images_output;
    guint i;
    guint j;
} ListImagesContext;

static void
list_images_context_free (ListImagesContext *operation_ctx)
{
    qmi_message_dms_list_stored_images_output_unref (operation_ctx->list_images_output);
    g_slice_free (ListImagesContext, operation_ctx);
}

static void get_image_info (ListImagesContext *operation_ctx);

static void
print_image_info (ListImagesContext *operation_ctx,
                  QmiMessageDmsGetStoredImageInfoOutput *output)
{
    GArray *array;
    QmiMessageDmsListStoredImagesOutputListImage *image;
    QmiMessageDmsListStoredImagesOutputListImageSublistSublistElement *subimage;
    g_autofree gchar *unique_id_str = NULL;

    qmi_message_dms_list_stored_images_output_get_list (
        operation_ctx->list_images_output,
        &array,
        NULL);

    image = &g_array_index (array, QmiMessageDmsListStoredImagesOutputListImage, operation_ctx->i);
    subimage = &g_array_index (image->sublist,
                               QmiMessageDmsListStoredImagesOutputListImageSublistSublistElement,
                               operation_ctx->j);

    unique_id_str = qmicli_get_firmware_image_unique_id_printable (subimage->unique_id);

    g_print ("%s"
             "\t\t[%s%u]\n"
             "\t\tUnique ID:     '%s'\n"
             "\t\tBuild ID:      '%s'\n",
             operation_ctx->j == image->index_of_running_image ? "\t\t>>>>>>>>>> [CURRENT] <<<<<<<<<<\n" : "",
             qmi_dms_firmware_image_type_get_string (image->type),
             operation_ctx->j,
             unique_id_str,
             subimage->build_id);

    if (subimage->storage_index != 255)
        g_print ("\t\tStorage index: '%u'\n", subimage->storage_index);

    if (subimage->failure_count != 255)
        g_print ("\t\tFailure count: '%u'\n", subimage->failure_count);

    if (output) {
        /* Boot version (optional) */
        {
            guint16 boot_major_version;
            guint16 boot_minor_version;

            if (qmi_message_dms_get_stored_image_info_output_get_boot_version (
                    output,
                    &boot_major_version,
                    &boot_minor_version,
                    NULL)) {
                g_print ("\t\tBoot version:  '%u.%u'\n",
                         boot_major_version,
                         boot_minor_version);
            }
        }

        /* PRI version (optional) */
        {
            guint32 pri_version;
            const gchar *pri_info;

            if (qmi_message_dms_get_stored_image_info_output_get_pri_version (
                    output,
                    &pri_version,
                    &pri_info,
                    NULL)) {
                g_print ("\t\tPRI version:   '%u'\n"
                         "\t\tPRI info:      '%s'\n",
                         pri_version,
                         pri_info);
            }
        }

        /* OEM lock ID (optional) */
        {
            guint32 lock_id;

            if (qmi_message_dms_get_stored_image_info_output_get_oem_lock_id (
                    output,
                    &lock_id,
                    NULL)) {
                g_print ("\t\tOEM lock ID:   '%u'\n",
                         lock_id);
            }
        }
    }
    g_print ("\n");
}

static void
get_stored_image_info_ready (QmiClientDms *client,
                             GAsyncResult *res,
                             ListImagesContext *operation_ctx)
{

    g_autoptr(QmiMessageDmsGetStoredImageInfoOutput) output = NULL;

    output = qmi_client_dms_get_stored_image_info_finish (client, res, NULL);
    if (output && !qmi_message_dms_get_stored_image_info_output_get_result (output, NULL))
        g_clear_pointer (&output, qmi_message_dms_get_stored_image_info_output_unref);

    print_image_info (operation_ctx, output);

    /* Go on to the next one */
    operation_ctx->j++;
    get_image_info (operation_ctx);
}

static void
get_image_info (ListImagesContext *operation_ctx)
{
    GArray *array;
    QmiMessageDmsListStoredImagesOutputListImage *image;
    QmiMessageDmsListStoredImagesOutputListImageSublistSublistElement *subimage;
    QmiMessageDmsGetStoredImageInfoInputImage image_id;
    g_autoptr(QmiMessageDmsGetStoredImageInfoInput) input = NULL;

    qmi_message_dms_list_stored_images_output_get_list (
        operation_ctx->list_images_output,
        &array,
        NULL);

    if (operation_ctx->i >= array->len) {
        /* We're done */
        list_images_context_free (operation_ctx);
        operation_shutdown (TRUE);
        return;
    }

    image = &g_array_index (array,
                            QmiMessageDmsListStoredImagesOutputListImage,
                            operation_ctx->i);

    if (operation_ctx->j >= image->sublist->len) {
        /* No more images in the sublist, go to next image type */
        operation_ctx->j = 0;
        operation_ctx->i++;
        get_image_info (operation_ctx);
        return;
    }

    /* Print info of the image type */
    if (operation_ctx->j == 0) {
        g_print ("\t[%u] Type:    '%s'\n"
                 "\t    Maximum: '%u'\n"
                 "\n",
                 operation_ctx->i,
                 qmi_dms_firmware_image_type_get_string (image->type),
                 image->maximum_images);
    }

    subimage = &g_array_index (image->sublist,
                               QmiMessageDmsListStoredImagesOutputListImageSublistSublistElement,
                               operation_ctx->j);

    /* Query image info */
    image_id.type = image->type;
    image_id.unique_id = subimage->unique_id;
    image_id.build_id = subimage->build_id;
    input = qmi_message_dms_get_stored_image_info_input_new ();
    qmi_message_dms_get_stored_image_info_input_set_image (input, &image_id, NULL);
    qmi_client_dms_get_stored_image_info (ctx->client,
                                          input,
                                          10,
                                          ctx->cancellable,
                                          (GAsyncReadyCallback)get_stored_image_info_ready,
                                          operation_ctx);
}

static void
list_stored_images_ready (QmiClientDms *client,
                          GAsyncResult *res)
{
    QmiMessageDmsListStoredImagesOutput *output;
    GError *error = NULL;
    ListImagesContext *operation_ctx;

    output = qmi_client_dms_list_stored_images_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_list_stored_images_output_get_result (output, &error)) {
        g_printerr ("error: couldn't list stored images: %s\n", error->message);
        g_error_free (error);
        qmi_message_dms_list_stored_images_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] Device list of stored images retrieved:\n\n",
             qmi_device_get_path_display (ctx->device));

    operation_ctx = g_slice_new0 (ListImagesContext);
    operation_ctx->list_images_output = output;
    operation_ctx->i = 0;
    operation_ctx->j = 0;

    get_image_info (operation_ctx);
}

#endif /* HAVE_QMI_MESSAGE_DMS_LIST_STORED_IMAGES && HAVE_QMI_MESSAGE_DMS_GET_STORED_IMAGE_INFO */

#if defined HAVE_QMI_MESSAGE_DMS_LIST_STORED_IMAGES && \
    defined HAVE_QMI_MESSAGE_DMS_SET_FIRMWARE_PREFERENCE

typedef struct {
    gint modem_index;
    gint pri_index;
} GetStoredImageContext;

typedef struct {
    GArray *modem_unique_id;
    gchar *modem_build_id;
    GArray *pri_unique_id;
    gchar *pri_build_id;
} GetStoredImageResult;

static void
get_stored_image_context_free (GetStoredImageContext *operation_ctx)
{
    g_slice_free (GetStoredImageContext, operation_ctx);
}

static void
get_stored_image_result_free (GetStoredImageResult *result)
{
    if (result) {
        g_clear_pointer (&result->modem_unique_id, g_array_unref);
        g_free (result->modem_build_id);
        g_clear_pointer (&result->pri_unique_id, g_array_unref);
        g_free (result->pri_build_id);
        g_slice_free (GetStoredImageResult, result);
    }
}

static void
get_stored_image_finish (QmiClientDms *client,
                         GAsyncResult *res,
                         GArray **modem_unique_id,
                         gchar **modem_build_id,
                         GArray **pri_unique_id,
                         gchar **pri_build_id)
{
    GetStoredImageResult *result;
    GError *error = NULL;

    result = g_task_propagate_pointer (G_TASK (res), &error);

    /* The operation always returns a result */
    g_assert (result);
    g_assert_no_error (error);

    /* Simply pass ownership to caller */
    *modem_unique_id = result->modem_unique_id;
    *modem_build_id = result->modem_build_id;
    *pri_unique_id = result->pri_unique_id;
    *pri_build_id = result->pri_build_id;

    g_slice_free (GetStoredImageResult, result);
}

static void
get_stored_image_list_stored_images_ready (QmiClientDms *client,
                                           GAsyncResult *res,
                                           GTask *task)
{
    GetStoredImageContext *operation_ctx;
    GetStoredImageResult *result;
    GArray *array;
    QmiMessageDmsListStoredImagesOutput *output;
    GError *error = NULL;
    guint i;

    output = qmi_client_dms_list_stored_images_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        g_object_unref (task);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_list_stored_images_output_get_result (output, &error)) {
        g_printerr ("error: couldn't list stored images: %s\n", error->message);
        g_error_free (error);
        qmi_message_dms_list_stored_images_output_unref (output);
        g_object_unref (task);
        operation_shutdown (FALSE);
        return;
    }

    qmi_message_dms_list_stored_images_output_get_list (
        output,
        &array,
        NULL);

    operation_ctx = g_task_get_task_data (task);

    /* A single result struct is used for all iterations */
    result = g_slice_new0 (GetStoredImageResult);

    for (i = 0; i < array->len; i++) {
        QmiMessageDmsListStoredImagesOutputListImageSublistSublistElement *subimage;
        QmiMessageDmsListStoredImagesOutputListImage *image;
        gchar *unique_id_str;
        gint image_index;

        image = &g_array_index (array,
                                QmiMessageDmsListStoredImagesOutputListImage,
                                i);

        if (image->type == QMI_DMS_FIRMWARE_IMAGE_TYPE_MODEM)
            image_index = operation_ctx->modem_index;
        else if (image->type == QMI_DMS_FIRMWARE_IMAGE_TYPE_PRI)
            image_index = operation_ctx->pri_index;
        else
            g_assert_not_reached ();

        /* If not looking for the specific image type, go on */
        if (image_index < 0)
            continue;

        if ((guint)image_index >= image->sublist->len) {
            g_printerr ("error: couldn't find '%s' image at index '%d'\n",
                        qmi_dms_firmware_image_type_get_string (image->type),
                        image_index);
            qmi_message_dms_list_stored_images_output_unref (output);
            g_object_unref (task);
            operation_shutdown (FALSE);
            return;
        }

        subimage = &g_array_index (image->sublist,
                                   QmiMessageDmsListStoredImagesOutputListImageSublistSublistElement,
                                   image_index);

        unique_id_str = qmicli_get_firmware_image_unique_id_printable (subimage->unique_id);
        g_debug ("Found [%s%d]: Unique ID: '%s', Build ID: '%s'",
                 qmi_dms_firmware_image_type_get_string (image->type),
                 image_index,
                 unique_id_str,
                 subimage->build_id);
        g_free (unique_id_str);

        if (image->type == QMI_DMS_FIRMWARE_IMAGE_TYPE_MODEM) {
            result->modem_unique_id = g_array_ref (subimage->unique_id);
            result->modem_build_id = g_strdup (subimage->build_id);
        } else if (image->type == QMI_DMS_FIRMWARE_IMAGE_TYPE_PRI) {
            result->pri_unique_id = g_array_ref (subimage->unique_id);
            result->pri_build_id = g_strdup (subimage->build_id);
        } else
            g_assert_not_reached ();
    }

    /* Complete */
    g_task_return_pointer (task, result, (GDestroyNotify)get_stored_image_result_free);
    g_object_unref (task);
    qmi_message_dms_list_stored_images_output_unref (output);
}

static void
get_stored_image (QmiClientDms *client,
                  const gchar *str,
                  GAsyncReadyCallback callback,
                  gpointer user_data)
{
    GetStoredImageContext *operation_ctx;
    GTask *task;
    gchar **split;
    guint i = 0;
    gint modem_index = -1;
    gint pri_index = -1;

    split = g_strsplit (str, ",", -1);
    while (split[i]) {
        QmiDmsFirmwareImageType type;
        guint image_index;

        if (i >= 3) {
            g_printerr ("A maximum of 2 images should be given: '%s'\n", str);
            operation_shutdown (FALSE);
            return;
        }

        if (!qmicli_read_firmware_id_from_string (split[i], &type, &image_index)) {
            g_printerr ("Couldn't parse input string as firmware index info: '%s'\n", str);
            operation_shutdown (FALSE);
            return;
        }

        if (type == QMI_DMS_FIRMWARE_IMAGE_TYPE_MODEM) {
            if (modem_index >= 0) {
                g_printerr ("Cannot handle two 'modem' type firmware indices: '%s'\n", str);
                operation_shutdown (FALSE);
                return;
            }
            modem_index = (gint)image_index;
        } else if (type == QMI_DMS_FIRMWARE_IMAGE_TYPE_PRI) {
            if (pri_index >= 0) {
                g_printerr ("Cannot handle two 'pri' type firmware indices: '%s'\n", str);
                operation_shutdown (FALSE);
                return;
            }
            pri_index = (gint)image_index;
        }

        i++;
    }

    operation_ctx = g_slice_new (GetStoredImageContext);
    operation_ctx->modem_index = modem_index;
    operation_ctx->pri_index = pri_index;

    task = g_task_new (client, NULL, callback, user_data);
    g_task_set_task_data (task, operation_ctx, (GDestroyNotify)get_stored_image_context_free);

    qmi_client_dms_list_stored_images (
        ctx->client,
        NULL,
        10,
        ctx->cancellable,
        (GAsyncReadyCallback)get_stored_image_list_stored_images_ready,
        task);
}

#endif /* HAVE_QMI_MESSAGE_DMS_LIST_STORED_IMAGES
        * HAVE_QMI_MESSAGE_DMS_SET_FIRMWARE_PREFERENCE */

#if defined HAVE_QMI_MESSAGE_DMS_SET_FIRMWARE_PREFERENCE

/* Note:
 *  used by both --dms-set-firmware-preference and --dms-select-stored-image
 * But only one single symbol check, as the select operation already
 * requires SET_FIRMWARE_PREFERENCE.
 */
static void
dms_set_firmware_preference_ready (QmiClientDms *client,
                                   GAsyncResult *res)
{
    QmiMessageDmsSetFirmwarePreferenceOutput *output;
    GError                                   *error = NULL;
    GArray                                   *array;
    GString                                  *pending_images = NULL;

    output = qmi_client_dms_set_firmware_preference_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_set_firmware_preference_output_get_result (output, &error)) {
        g_printerr ("error: couldn't select stored image: %s\n", error->message);
        g_error_free (error);
        qmi_message_dms_set_firmware_preference_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] Firmware preference successfully selected\n"
             "\n"
             "\tYou may want to power-cycle the modem now, or just set it offline and reset it:\n"
             "\t\t$> sudo qmicli ... --dms-set-operating-mode=offline\n"
             "\t\t$> sudo qmicli ... --dms-set-operating-mode=reset\n"
             "\n",
             qmi_device_get_path_display (ctx->device));

    /* do we need to download a new modem and/or pri image? */
    if (qmi_message_dms_set_firmware_preference_output_get_image_download_list (output, &array, NULL) && array->len) {
        guint                   i;
        QmiDmsFirmwareImageType type;

        pending_images = g_string_new ("");
        for (i = 0; i < array->len; i++) {
            type = g_array_index (array, QmiDmsFirmwareImageType, i);
            g_string_append (pending_images, qmi_dms_firmware_image_type_get_string (type));
            if (i < array->len -1)
                g_string_append (pending_images, ", ");
        }
    }

    if (pending_images) {
        g_print ("\tAfter reset, the modem will wait in QDL mode for new firmware.\n"
                 "\tImages to download: '%s'\n"
                 "\n",
                 pending_images->str);
        g_string_free (pending_images, TRUE);
    } else {
        /* If we're selecting an already stored image, or if we don't need any
         * more images to be downloaded, we're done. */
        g_print ("\tNo new images are required to be downloaded.\n"
                 "\n"
                 "\tYou should check that the modem|pri image pair is valid by checking the current operating mode:\n"
                 "\t\t$> sudo qmicli .... --dms-get-operating-mode\n"
                 "\tIf the Mode is reported as 'online', you're good to go.\n"
                 "\tIf the Mode is reported as 'offline' with a 'pri-version-incompatible' reason, you chose an incorrect pair\n"
                 "\n");
    }

    qmi_message_dms_set_firmware_preference_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_DMS_SET_FIRMWARE_PREFERENCE */

#if defined HAVE_QMI_MESSAGE_DMS_SET_FIRMWARE_PREFERENCE && \
    defined HAVE_QMI_MESSAGE_DMS_LIST_STORED_IMAGES

static void
get_stored_image_select_ready (QmiClientDms *client,
                               GAsyncResult *res)
{
    QmiMessageDmsSetFirmwarePreferenceInput *input;
    GArray *array;
    QmiMessageDmsSetFirmwarePreferenceInputListImage modem_image_id;
    QmiMessageDmsSetFirmwarePreferenceInputListImage pri_image_id;

    modem_image_id.type = QMI_DMS_FIRMWARE_IMAGE_TYPE_MODEM;
    pri_image_id.type = QMI_DMS_FIRMWARE_IMAGE_TYPE_PRI;

    get_stored_image_finish (client,
                             res,
                             &modem_image_id.unique_id,
                             &modem_image_id.build_id,
                             &pri_image_id.unique_id,
                             &pri_image_id.build_id);

    if (!modem_image_id.unique_id || !modem_image_id.build_id ||
        !pri_image_id.unique_id || !pri_image_id.build_id) {
        g_printerr ("error: must specify a pair of 'modem' and 'pri' images to select\n");
        operation_shutdown (FALSE);
        return;
    }

    array = g_array_sized_new (FALSE, FALSE, sizeof (QmiMessageDmsSetFirmwarePreferenceInputListImage), 2);
    g_array_append_val (array, modem_image_id);
    g_array_append_val (array, pri_image_id);

    input = qmi_message_dms_set_firmware_preference_input_new ();
    qmi_message_dms_set_firmware_preference_input_set_list (input, array, NULL);

    qmi_client_dms_set_firmware_preference (
        client,
        input,
        10,
        NULL,
        (GAsyncReadyCallback)dms_set_firmware_preference_ready,
        NULL);
    qmi_message_dms_set_firmware_preference_input_unref (input);

    g_free        (modem_image_id.build_id);
    g_array_unref (modem_image_id.unique_id);
    g_free        (pri_image_id.build_id);
    g_array_unref (pri_image_id.unique_id);
}

#endif /* HAVE_QMI_MESSAGE_DMS_SET_FIRMWARE_PREFERENCE
        * HAVE_QMI_MESSAGE_DMS_LIST_STORED_IMAGES */

#if defined HAVE_QMI_MESSAGE_DMS_SET_FIRMWARE_PREFERENCE && \
    defined HAVE_QMI_MESSAGE_DMS_LIST_STORED_IMAGES && \
    defined HAVE_QMI_MESSAGE_DMS_DELETE_STORED_IMAGE

static void
delete_stored_image_ready (QmiClientDms *client,
                           GAsyncResult *res)
{
    QmiMessageDmsDeleteStoredImageOutput *output;
    GError *error = NULL;

    output = qmi_client_dms_delete_stored_image_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_delete_stored_image_output_get_result (output, &error)) {
        g_printerr ("error: couldn't delete stored image: %s\n", error->message);
        g_error_free (error);
        qmi_message_dms_delete_stored_image_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] Stored image successfully deleted\n",
             qmi_device_get_path_display (ctx->device));
    qmi_message_dms_delete_stored_image_output_unref (output);
    operation_shutdown (TRUE);
}

static void
get_stored_image_delete_ready (QmiClientDms *client,
                               GAsyncResult *res)
{
    QmiMessageDmsDeleteStoredImageInput *input;
    QmiMessageDmsDeleteStoredImageInputImage modem_image_id;
    QmiMessageDmsDeleteStoredImageInputImage pri_image_id;

    modem_image_id.type = QMI_DMS_FIRMWARE_IMAGE_TYPE_MODEM;
    pri_image_id.type = QMI_DMS_FIRMWARE_IMAGE_TYPE_PRI;

    get_stored_image_finish (client,
                             res,
                             &modem_image_id.unique_id,
                             &modem_image_id.build_id,
                             &pri_image_id.unique_id,
                             &pri_image_id.build_id);

    if (modem_image_id.unique_id && modem_image_id.build_id &&
        pri_image_id.unique_id && pri_image_id.build_id) {
        g_printerr ("error: cannot specify multiple images to delete\n");
        operation_shutdown (FALSE);
        return;
    }

    input = qmi_message_dms_delete_stored_image_input_new ();
    if (modem_image_id.unique_id && modem_image_id.build_id)
        qmi_message_dms_delete_stored_image_input_set_image (input, &modem_image_id, NULL);
    else if (pri_image_id.unique_id && pri_image_id.build_id)
        qmi_message_dms_delete_stored_image_input_set_image (input, &pri_image_id, NULL);
    else {
        g_printerr ("error: didn't specify correctly an image to delete\n");
        operation_shutdown (FALSE);
        return;
    }

    qmi_client_dms_delete_stored_image (
        client,
        input,
        10,
        NULL,
        (GAsyncReadyCallback)delete_stored_image_ready,
        NULL);
    qmi_message_dms_delete_stored_image_input_unref (input);

    g_free (modem_image_id.build_id);
    if (modem_image_id.unique_id)
        g_array_unref (modem_image_id.unique_id);
    g_free (pri_image_id.build_id);
    if (pri_image_id.unique_id)
        g_array_unref (pri_image_id.unique_id);
}

#endif /* HAVE_QMI_MESSAGE_DMS_SET_FIRMWARE_PREFERENCE
        * HAVE_QMI_MESSAGE_DMS_LIST_STORED_IMAGES
        * HAVE_QMI_MESSAGE_DMS_DELETE_STORED_IMAGE */

#if defined HAVE_QMI_MESSAGE_DMS_GET_FIRMWARE_PREFERENCE

static void
dms_get_firmware_preference_ready (QmiClientDms *client,
                                   GAsyncResult *res)
{
    GError                                   *error = NULL;
    QmiMessageDmsGetFirmwarePreferenceOutput *output;
    GArray                                   *array;
    guint                                     i;

    output = qmi_client_dms_get_firmware_preference_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_get_firmware_preference_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get firmware preference: %s\n", error->message);
        g_error_free (error);
        qmi_message_dms_get_firmware_preference_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    qmi_message_dms_get_firmware_preference_output_get_list (output, &array, NULL);

    g_print ("firmware preference successfully retrieved:\n");

    if (array->len > 0) {
        for (i = 0; i < array->len; i++) {
            QmiMessageDmsGetFirmwarePreferenceOutputListImage *image;
            gchar                                             *unique_id_str;

            image = &g_array_index (array, QmiMessageDmsGetFirmwarePreferenceOutputListImage, i);

            unique_id_str = qmicli_get_firmware_image_unique_id_printable (image->unique_id);

            g_print ("[image %u]\n"
                     "\tImage type: '%s'\n"
                     "\tUnique ID:  '%s'\n"
                     "\tBuild ID:   '%s'\n",
                     i,
                     qmi_dms_firmware_image_type_get_string (image->type),
                     unique_id_str,
                     image->build_id);

            g_free (unique_id_str);
        }
    } else
        g_print ("no images specified\n");

    qmi_message_dms_get_firmware_preference_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_DMS_GET_FIRMWARE_PREFERENCE */

#if defined HAVE_QMI_MESSAGE_DMS_SET_FIRMWARE_PREFERENCE

typedef struct {
    QmiMessageDmsSetFirmwarePreferenceInputListImage modem_image_id;
    QmiMessageDmsSetFirmwarePreferenceInputListImage pri_image_id;
} SetFirmwarePreferenceContext;

static void
set_firmware_preference_context_clear (SetFirmwarePreferenceContext *firmware_preference_ctx)
{
    g_clear_pointer (&firmware_preference_ctx->modem_image_id.unique_id, g_array_unref);
    g_free (firmware_preference_ctx->modem_image_id.build_id);

    g_clear_pointer (&firmware_preference_ctx->pri_image_id.unique_id, g_array_unref);
    g_free (firmware_preference_ctx->pri_image_id.build_id);
}

typedef struct {
    gchar    *firmware_version;
    gchar    *config_version;
    gchar    *carrier;
    gint      modem_storage_index;
    gboolean  override_download;
    gboolean  override_download_set;
} SetFirmwarePreferenceProperties;

static void
set_firmware_preference_properties_clear (SetFirmwarePreferenceProperties *props)
{
    g_free (props->firmware_version);
    g_free (props->config_version);
    g_free (props->carrier);
}

static gboolean
set_firmware_preference_properties_handle (const gchar  *key,
                                           const gchar  *value,
                                           GError      **error,
                                           gpointer      user_data)
{
    SetFirmwarePreferenceProperties *props = user_data;

    if (!value || !value[0]) {
        g_set_error (error,
                     QMI_CORE_ERROR,
                     QMI_CORE_ERROR_FAILED,
                     "key '%s' required a value",
                     key);
        return FALSE;
    }

    if (g_ascii_strcasecmp (key, "firmware-version") == 0 && !props->firmware_version) {
        props->firmware_version = g_strdup (value);
        return TRUE;
    }

    if (g_ascii_strcasecmp (key, "config-version") == 0 && !props->config_version) {
        props->config_version = g_strdup (value);
        return TRUE;
    }

    if (g_ascii_strcasecmp (key, "carrier") == 0 && !props->carrier) {
        props->carrier = g_strdup (value);
        return TRUE;
    }

    if (g_ascii_strcasecmp (key, "override-download") == 0 && !props->override_download_set) {
        if (!qmicli_read_yes_no_from_string (value, &(props->override_download))) {
            g_set_error (error,
                         QMI_CORE_ERROR,
                         QMI_CORE_ERROR_FAILED,
                         "unknown override-download '%s'",
                         value);
            return FALSE;
        }
        props->override_download_set = TRUE;
        return TRUE;
    }

    if (g_ascii_strcasecmp (key, "modem-storage-index") == 0 && props->modem_storage_index == -1) {
        props->modem_storage_index = atoi (value);
        if (props->modem_storage_index < 0 || props->modem_storage_index > G_MAXUINT8) {
            g_set_error (error,
                         QMI_CORE_ERROR,
                         QMI_CORE_ERROR_FAILED,
                         "invalid modem-storage-index '%s'",
                         value);
            return FALSE;
        }
        return TRUE;
    }

    g_set_error (error,
                 QMI_CORE_ERROR,
                 QMI_CORE_ERROR_FAILED,
                 "unrecognized or duplicate option '%s'",
                 key);
    return FALSE;
}

static QmiMessageDmsSetFirmwarePreferenceInput *
set_firmware_preference_input_create (const gchar                   *str,
                                      SetFirmwarePreferenceContext  *firmware_preference_ctx,
                                      GError                       **error)
{
    g_autoptr(QmiMessageDmsSetFirmwarePreferenceInput) input = NULL;
    g_autoptr(GArray)                                  array = NULL;

    SetFirmwarePreferenceProperties props = {
        .firmware_version = NULL,
        .config_version = NULL,
        .carrier = NULL,
        .modem_storage_index = -1,
        .override_download = FALSE,
        .override_download_set = FALSE,
    };

    /* New key=value format */
    if (strchr (str, '=')) {
        g_autoptr(GError) parse_error = NULL;

        if (!qmicli_parse_key_value_string (str,
                                            &parse_error,
                                            set_firmware_preference_properties_handle,
                                            &props)) {
            g_set_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_FAILED,
                         "Couldn't parse input string: %s", parse_error->message);
            set_firmware_preference_properties_clear (&props);
            return NULL;
        }

        if (!props.firmware_version || !props.config_version || !props.carrier) {
            g_set_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_FAILED,
                         "Missing mandatory parameters: 'firmware-version', 'config-version' and 'carrier' are mandatory");
            set_firmware_preference_properties_clear (&props);
            return NULL;
        }
    }
    /* Old non key=value format, like this:
     *    "[(firmware_version),(config_version),(carrier)]"
     */
    else {
        g_auto(GStrv) split = NULL;

        split = g_strsplit (str, ",", -1);
        if (g_strv_length (split) != 3) {
            g_set_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_FAILED,
                         "Invalid format string, expected 3 elements: 'firmware-version', 'config-version' and 'carrier'");
            return NULL;
        }

        props.firmware_version = g_strdup (split[0]);
        props.config_version   = g_strdup (split[1]);
        props.carrier          = g_strdup (split[2]);
    }

    /* modem unique id is the fixed wildcard string '?_?' matching any pri.
     * modem build id format is "(firmware_version)_?", matching any carrier */
    firmware_preference_ctx->modem_image_id.type = QMI_DMS_FIRMWARE_IMAGE_TYPE_MODEM;
    firmware_preference_ctx->modem_image_id.unique_id = g_array_sized_new (FALSE, TRUE, 1, 16);
    g_array_insert_vals (firmware_preference_ctx->modem_image_id.unique_id, 0, "?_?", 3);
    g_array_set_size (firmware_preference_ctx->modem_image_id.unique_id, 16);
    firmware_preference_ctx->modem_image_id.build_id = g_strdup_printf ("%s_?", props.firmware_version);

    /* pri unique id is the "(config_version)" input */
    firmware_preference_ctx->pri_image_id.type = QMI_DMS_FIRMWARE_IMAGE_TYPE_PRI;
    firmware_preference_ctx->pri_image_id.unique_id = g_array_sized_new (FALSE, TRUE, 1, 16);
    g_array_insert_vals (firmware_preference_ctx->pri_image_id.unique_id, 0, props.config_version, strlen (props.config_version));
    g_array_set_size (firmware_preference_ctx->pri_image_id.unique_id, 16);
    firmware_preference_ctx->pri_image_id.build_id = g_strdup_printf ("%s_%s", props.firmware_version, props.carrier);

    /* Create an array with both images, the contents of each image struct,
     * though, aren't owned by the array (i.e. need to be disposed afterwards
     * when no longer used). */
    array = g_array_sized_new (FALSE, FALSE, sizeof (QmiMessageDmsSetFirmwarePreferenceInputListImage), 2);
    g_array_append_val (array, firmware_preference_ctx->modem_image_id);
    g_array_append_val (array, firmware_preference_ctx->pri_image_id);

    /* The input bundle takes a reference to the array itself */
    input = qmi_message_dms_set_firmware_preference_input_new ();
    qmi_message_dms_set_firmware_preference_input_set_list (input, array, NULL);

    /* Other optional settings */
    if (props.modem_storage_index >= 0)
        qmi_message_dms_set_firmware_preference_input_set_modem_storage_index (input, (guint8)props.modem_storage_index, NULL);
    if (props.override_download_set)
        qmi_message_dms_set_firmware_preference_input_set_download_override (input, props.override_download, NULL);

    set_firmware_preference_properties_clear (&props);
    return g_steal_pointer (&input);
}

#endif /* HAVE_QMI_MESSAGE_DMS_SET_FIRMWARE_PREFERENCE */

#if defined HAVE_QMI_MESSAGE_DMS_GET_BOOT_IMAGE_DOWNLOAD_MODE

static void
get_boot_image_download_mode_ready (QmiClientDms *client,
                                    GAsyncResult *res)
{
    QmiMessageDmsGetBootImageDownloadModeOutput *output;
    GError *error = NULL;
    QmiDmsBootImageDownloadMode mode;

    output = qmi_client_dms_get_boot_image_download_mode_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_get_boot_image_download_mode_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get boot image download mode: %s\n", error->message);
        g_error_free (error);
        qmi_message_dms_get_boot_image_download_mode_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    qmi_message_dms_get_boot_image_download_mode_output_get_mode (output, &mode, NULL);

    g_print ("[%s] Boot image download mode: %s\n",
             qmi_device_get_path_display (ctx->device),
             qmi_dms_boot_image_download_mode_get_string (mode));

    qmi_message_dms_get_boot_image_download_mode_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_DMS_GET_BOOT_IMAGE_DOWNLOAD_MODE */

#if defined HAVE_QMI_MESSAGE_DMS_SET_BOOT_IMAGE_DOWNLOAD_MODE

static QmiMessageDmsSetBootImageDownloadModeInput *
set_boot_image_download_mode_input_create (const gchar *str)
{
    QmiMessageDmsSetBootImageDownloadModeInput *input = NULL;
    QmiDmsBootImageDownloadMode mode;
    GError *error = NULL;

    /* Prepare inputs.
     * Format of the string is:
     *    [normal|boot-and-recovery]
     */
    if (!qmicli_read_dms_boot_image_download_mode_from_string (str, &mode))
        return NULL;

    input = qmi_message_dms_set_boot_image_download_mode_input_new ();
    if (!qmi_message_dms_set_boot_image_download_mode_input_set_mode (input, mode, &error)) {
        g_printerr ("error: couldn't create input bundle: '%s'\n", error->message);
        g_error_free (error);
        qmi_message_dms_set_boot_image_download_mode_input_unref (input);
        return NULL;
    }

    return input;
}

static void
set_boot_image_download_mode_ready (QmiClientDms *client,
                                    GAsyncResult *res)
{
    QmiMessageDmsSetBootImageDownloadModeOutput *output;
    GError *error = NULL;

    output = qmi_client_dms_set_boot_image_download_mode_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_set_boot_image_download_mode_output_get_result (output, &error)) {
        g_printerr ("error: couldn't set boot image download mode: %s\n", error->message);
        g_error_free (error);
        qmi_message_dms_set_boot_image_download_mode_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] Boot image download mode successfully set\n",
             qmi_device_get_path_display (ctx->device));

    qmi_message_dms_set_boot_image_download_mode_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_DMS_SET_BOOT_IMAGE_DOWNLOAD_MODE */

#if defined HAVE_QMI_MESSAGE_DMS_GET_SOFTWARE_VERSION

static void
get_software_version_ready (QmiClientDms *client,
                            GAsyncResult *res)
{
    QmiMessageDmsGetSoftwareVersionOutput *output;
    GError *error = NULL;
    const gchar *version;

    output = qmi_client_dms_get_software_version_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_get_software_version_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get boot image download mode: %s\n", error->message);
        g_error_free (error);
        qmi_message_dms_get_software_version_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    qmi_message_dms_get_software_version_output_get_version (output, &version, NULL);

    g_print ("[%s] Software version: %s\n",
             qmi_device_get_path_display (ctx->device),
             version);

    qmi_message_dms_get_software_version_output_unref (output);
    operation_shutdown (TRUE);
}

#endif

#if defined HAVE_QMI_MESSAGE_DMS_SET_FCC_AUTHENTICATION

static void
set_fcc_authentication_ready (QmiClientDms *client,
                              GAsyncResult *res)
{
    QmiMessageDmsSetFccAuthenticationOutput *output;
    GError *error = NULL;

    output = qmi_client_dms_set_fcc_authentication_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_set_fcc_authentication_output_get_result (output, &error)) {
        g_printerr ("error: couldn't set FCC authentication: %s\n", error->message);
        g_error_free (error);
        qmi_message_dms_set_fcc_authentication_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] Successfully set FCC authentication\n",
             qmi_device_get_path_display (ctx->device));

    qmi_message_dms_set_fcc_authentication_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_DMS_SET_FCC_AUTHENTICATION */

#if defined HAVE_QMI_MESSAGE_DMS_GET_SUPPORTED_MESSAGES

static void
get_supported_messages_ready (QmiClientDms *client,
                              GAsyncResult *res)
{
    QmiMessageDmsGetSupportedMessagesOutput *output;
    GError *error = NULL;
    GArray *bytearray = NULL;
    gchar *str;

    output = qmi_client_dms_get_supported_messages_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_get_supported_messages_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get supported DMS messages: %s\n", error->message);
        g_error_free (error);
        qmi_message_dms_get_supported_messages_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] Successfully got supported DMS messages:\n",
             qmi_device_get_path_display (ctx->device));

    qmi_message_dms_get_supported_messages_output_get_list (output, &bytearray, NULL);
    str = qmicli_get_supported_messages_list (bytearray ? (const guint8 *)bytearray->data : NULL,
                                              bytearray ? bytearray->len : 0);
    g_print ("%s", str);
    g_free (str);

    qmi_message_dms_get_supported_messages_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_DMS_GET_SUPPORTED_MESSAGES */

#if defined HAVE_QMI_MESSAGE_DMS_HP_CHANGE_DEVICE_MODE

static QmiMessageDmsHpChangeDeviceModeInput *
hp_change_device_mode_input_create (const gchar *str)
{
    QmiMessageDmsHpChangeDeviceModeInput *input = NULL;
    QmiDmsHpDeviceMode mode;
    GError *error = NULL;

    if (!qmicli_read_dms_hp_device_mode_from_string (str, &mode)) {
        g_printerr ("error: couldn't parse input HP device mode : '%s'\n", str);
        return NULL;
    }

    input = qmi_message_dms_hp_change_device_mode_input_new ();
    if (!qmi_message_dms_hp_change_device_mode_input_set_mode (input, mode, &error)) {
        g_printerr ("error: couldn't create input data bundle: '%s'\n",
                    error->message);
        g_error_free (error);
        qmi_message_dms_hp_change_device_mode_input_unref (input);
        return NULL;
    }

    return input;
}

static void
hp_change_device_mode_ready (QmiClientDms *client,
                             GAsyncResult *res)
{
    QmiMessageDmsHpChangeDeviceModeOutput *output;
    GError *error = NULL;

    output = qmi_client_dms_hp_change_device_mode_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_hp_change_device_mode_output_get_result (output, &error)) {
        g_printerr ("error: couldn't change HP device mode: %s\n", error->message);
        g_error_free (error);
        qmi_message_dms_hp_change_device_mode_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] Successfully changed HP device mode\n",
             qmi_device_get_path_display (ctx->device));

    qmi_message_dms_hp_change_device_mode_output_unref (output);

    /* Changing the mode will end up power cycling the device right away, so
     * just ignore any error from now on and don't even try to release the
     * client CID */
    operation_shutdown_skip_cid_release (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_DMS_HP_CHANGE_DEVICE_MODE */

#if defined HAVE_QMI_MESSAGE_DMS_SWI_GET_CURRENT_FIRMWARE

static void
swi_get_current_firmware_ready (QmiClientDms *client,
                                GAsyncResult *res)
{
    QmiMessageDmsSwiGetCurrentFirmwareOutput *output;
    GError *error = NULL;
    const gchar *model = NULL;
    const gchar *boot_version = NULL;
    const gchar *amss_version = NULL;
    const gchar *sku_id = NULL;
    const gchar *package_id = NULL;
    const gchar *carrier_id = NULL;
    const gchar *pri_version = NULL;
    const gchar *carrier = NULL;
    const gchar *config_version = NULL;

    output = qmi_client_dms_swi_get_current_firmware_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_swi_get_current_firmware_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get current firmware: %s\n", error->message);
        g_error_free (error);
        qmi_message_dms_swi_get_current_firmware_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    qmi_message_dms_swi_get_current_firmware_output_get_model          (output, &model,          NULL);
    qmi_message_dms_swi_get_current_firmware_output_get_boot_version   (output, &boot_version,   NULL);
    qmi_message_dms_swi_get_current_firmware_output_get_amss_version   (output, &amss_version,   NULL);
    qmi_message_dms_swi_get_current_firmware_output_get_sku_id         (output, &sku_id,         NULL);
    qmi_message_dms_swi_get_current_firmware_output_get_package_id     (output, &package_id,     NULL);
    qmi_message_dms_swi_get_current_firmware_output_get_carrier_id     (output, &carrier_id,     NULL);
    qmi_message_dms_swi_get_current_firmware_output_get_pri_version    (output, &pri_version,    NULL);
    qmi_message_dms_swi_get_current_firmware_output_get_carrier        (output, &carrier,        NULL);
    qmi_message_dms_swi_get_current_firmware_output_get_config_version (output, &config_version, NULL);

    /* We'll consider it a success if we got at least one of the expected strings */
    if (!model          &&
        !boot_version   &&
        !amss_version   &&
        !sku_id         &&
        !package_id     &&
        !carrier_id     &&
        !pri_version    &&
        !carrier        &&
        !config_version) {
        g_printerr ("error: couldn't get any of the current firmware fields\n");
        qmi_message_dms_swi_get_current_firmware_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] Successfully retrieved current firmware:\n",
             qmi_device_get_path_display (ctx->device));
    g_print ("\tModel: %s\n",          VALIDATE_UNKNOWN (model));
    g_print ("\tBoot version: %s\n",   VALIDATE_UNKNOWN (boot_version));
    g_print ("\tAMSS version: %s\n",   VALIDATE_UNKNOWN (amss_version));
    g_print ("\tSKU ID: %s\n",         VALIDATE_UNKNOWN (sku_id));
    g_print ("\tPackage ID: %s\n",     VALIDATE_UNKNOWN (package_id));
    g_print ("\tCarrier ID: %s\n",     VALIDATE_UNKNOWN (carrier_id));
    g_print ("\tConfig version: %s\n", VALIDATE_UNKNOWN (config_version));

    qmi_message_dms_swi_get_current_firmware_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_DMS_SWI_GET_CURRENT_FIRMWARE */

#if defined HAVE_QMI_MESSAGE_DMS_SWI_GET_USB_COMPOSITION

static void
swi_get_usb_composition_ready (QmiClientDms *client,
                               GAsyncResult *res)
{
    QmiMessageDmsSwiGetUsbCompositionOutput *output;
    GError                                  *error = NULL;
    GArray                                  *supported = NULL;
    QmiDmsSwiUsbComposition                  current = QMI_DMS_SWI_USB_COMPOSITION_UNKNOWN;
    guint                                    i;

    output = qmi_client_dms_swi_get_usb_composition_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_swi_get_usb_composition_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get USB composite modes: %s\n", error->message);
        g_error_free (error);
        qmi_message_dms_swi_get_usb_composition_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] Successfully retrieved USB compositions:\n",
             qmi_device_get_path_display (ctx->device));

    if (!qmi_message_dms_swi_get_usb_composition_output_get_current (output, &current, &error)) {
        g_printerr ("error: couldn't get current USB composition: %s\n", error->message);
        g_clear_error (&error);
    }

    if (!qmi_message_dms_swi_get_usb_composition_output_get_supported (output, &supported, &error)) {
        g_printerr ("error: couldn't get list of USB compositions: %s\n", error->message);
        g_clear_error (&error);
    }

    for (i = 0; i < supported->len; i++) {
        QmiDmsSwiUsbComposition value;

        value = g_array_index (supported, QmiDmsSwiUsbComposition, i);
        g_print ("\t%sUSB composition %s: %s\n",
                 (value == current ? "[*] " : "    "),
                 qmi_dms_swi_usb_composition_get_string (value),
                 qmi_dms_swi_usb_composition_get_description (value));
    }

    qmi_message_dms_swi_get_usb_composition_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_DMS_SWI_GET_USB_COMPOSITION */

#if defined HAVE_QMI_MESSAGE_DMS_SWI_SET_USB_COMPOSITION

static void
swi_set_usb_composition_ready (QmiClientDms *client,
                               GAsyncResult *res)
{
    QmiMessageDmsSwiSetUsbCompositionOutput *output;
    GError                                  *error = NULL;

    output = qmi_client_dms_swi_set_usb_composition_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_swi_set_usb_composition_output_get_result (output, &error)) {
        g_printerr ("error: couldn't set USB composite modes: %s\n", error->message);
        g_error_free (error);
        qmi_message_dms_swi_set_usb_composition_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] Successfully set USB composition\n"
             "\n"
             "\tYou may want to power-cycle the modem now, or just set it offline and reset it:\n"
             "\t\t$> sudo qmicli ... --dms-set-operating-mode=offline\n"
             "\t\t$> sudo qmicli ... --dms-set-operating-mode=reset\n"
             "\n",
             qmi_device_get_path_display (ctx->device));

    qmi_message_dms_swi_set_usb_composition_output_unref (output);
    operation_shutdown (TRUE);
}

static QmiMessageDmsSwiSetUsbCompositionInput *
swi_set_usb_composition_input_create (const gchar *str)
{
    QmiMessageDmsSwiSetUsbCompositionInput *input = NULL;
    QmiDmsSwiUsbComposition value;
    GError *error = NULL;

    if (!qmicli_read_dms_swi_usb_composition_from_string (str, &value))
        return NULL;

    input = qmi_message_dms_swi_set_usb_composition_input_new ();
    if (!qmi_message_dms_swi_set_usb_composition_input_set_current (input, value, &error)) {
        g_printerr ("error: couldn't create input bundle: '%s'\n", error->message);
        g_error_free (error);
        qmi_message_dms_swi_set_usb_composition_input_unref (input);
        return NULL;
    }

    return input;
}

#endif /* HAVE_QMI_MESSAGE_DMS_SWI_SET_USB_COMPOSITION */

#if defined HAVE_QMI_MESSAGE_DMS_FOXCONN_CHANGE_DEVICE_MODE

static QmiMessageDmsFoxconnChangeDeviceModeInput *
foxconn_change_device_mode_input_create (const gchar *str)
{
    QmiMessageDmsFoxconnChangeDeviceModeInput *input = NULL;
    QmiDmsFoxconnDeviceMode mode;
    GError *error = NULL;

    if (!qmicli_read_dms_foxconn_device_mode_from_string (str, &mode)) {
        g_printerr ("error: couldn't parse input foxconn device mode : '%s'\n", str);
        return NULL;
    }

    input = qmi_message_dms_foxconn_change_device_mode_input_new ();
    if (!qmi_message_dms_foxconn_change_device_mode_input_set_mode (input, mode, &error)) {
        g_printerr ("error: couldn't create input data bundle: '%s'\n",
                    error->message);
        g_error_free (error);
        qmi_message_dms_foxconn_change_device_mode_input_unref (input);
        return NULL;
    }

    return input;
}

static void
foxconn_change_device_mode_ready (QmiClientDms *client,
                                  GAsyncResult *res)
{
    QmiMessageDmsFoxconnChangeDeviceModeOutput *output;
    GError *error = NULL;

    output = qmi_client_dms_foxconn_change_device_mode_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_foxconn_change_device_mode_output_get_result (output, &error)) {
        g_printerr ("error: couldn't change foxconn device mode: %s\n", error->message);
        g_error_free (error);
        qmi_message_dms_foxconn_change_device_mode_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] Successfully changed foxconn device mode\n",
             qmi_device_get_path_display (ctx->device));

    qmi_message_dms_foxconn_change_device_mode_output_unref (output);

    /* Changing the mode will end up power cycling the device right away, so
     * just ignore any error from now on and don't even try to release the
     * client CID */
    operation_shutdown_skip_cid_release (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_DMS_FOXCONN_CHANGE_DEVICE_MODE */

#if defined HAVE_QMI_MESSAGE_DMS_FOXCONN_GET_FIRMWARE_VERSION

static QmiMessageDmsFoxconnGetFirmwareVersionInput *
foxconn_get_firmware_version_input_create (const gchar *str)
{
    QmiMessageDmsFoxconnGetFirmwareVersionInput *input = NULL;
    QmiDmsFoxconnFirmwareVersionType type;
    GError *error = NULL;

    if (!qmicli_read_dms_foxconn_firmware_version_type_from_string (str, &type)) {
        g_printerr ("error: couldn't parse input foxconn firmware version type : '%s'\n", str);
        return NULL;
    }

    input = qmi_message_dms_foxconn_get_firmware_version_input_new ();
    if (!qmi_message_dms_foxconn_get_firmware_version_input_set_version_type (input, type, &error)) {
        g_printerr ("error: couldn't create input data bundle: '%s'\n",
                    error->message);
        g_error_free (error);
        qmi_message_dms_foxconn_get_firmware_version_input_unref (input);
        return NULL;
    }

    return input;
}

static void
foxconn_get_firmware_version_ready (QmiClientDms *client,
                                    GAsyncResult *res)
{
    const gchar *str = NULL;
    QmiMessageDmsFoxconnGetFirmwareVersionOutput *output;
    GError *error = NULL;

    output = qmi_client_dms_foxconn_get_firmware_version_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_foxconn_get_firmware_version_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get foxconn firmware version: %s\n", error->message);
        g_error_free (error);
        qmi_message_dms_foxconn_get_firmware_version_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    qmi_message_dms_foxconn_get_firmware_version_output_get_version (output, &str, NULL);

    g_print ("[%s] Firmware version retrieved:\n"
             "\tVersion: '%s'\n",
             qmi_device_get_path_display (ctx->device),
             VALIDATE_UNKNOWN (str));

    qmi_message_dms_foxconn_get_firmware_version_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_DMS_FOXCONN_GET_FIRMWARE_VERSION */

#if defined HAVE_QMI_MESSAGE_DMS_FOXCONN_SET_FCC_AUTHENTICATION

static void
foxconn_set_fcc_authentication_ready (QmiClientDms *client,
                                      GAsyncResult *res)
{
    g_autoptr(QmiMessageDmsFoxconnSetFccAuthenticationOutput) output = NULL;
    g_autoptr(GError)                                         error = NULL;

    output = qmi_client_dms_foxconn_set_fcc_authentication_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_foxconn_set_fcc_authentication_output_get_result (output, &error)) {
        g_printerr ("error: couldn't run Foxconn FCC authentication: %s\n", error->message);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] Successfully run Foxconn FCC authentication\n",
             qmi_device_get_path_display (ctx->device));
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_DMS_FOXCONN_SET_FCC_AUTHENTICATION */

#if defined HAVE_QMI_MESSAGE_DMS_GET_MAC_ADDRESS

static QmiMessageDmsGetMacAddressInput *
get_mac_address_input_create (const gchar *str)
{
    QmiMessageDmsGetMacAddressInput *input = NULL;
    QmiDmsMacType device;

    if (qmicli_read_dms_mac_type_from_string (str, &device)) {
        GError *error = NULL;

        input = qmi_message_dms_get_mac_address_input_new ();
        if (!qmi_message_dms_get_mac_address_input_set_device (
                input,
                device,
                &error)) {
            g_printerr ("error: couldn't create input data bundle: '%s'\n",
                        error->message);
            g_error_free (error);
            qmi_message_dms_get_mac_address_input_unref (input);
            input = NULL;
        }
    }

    return input;
}

static void
get_mac_address_ready (QmiClientDms *client,
                       GAsyncResult *res)
{
    QmiMessageDmsGetMacAddressOutput *output;
    GError *error = NULL;
    gchar *mac_address_printable;
    GArray *mac_address = NULL;

    output = qmi_client_dms_get_mac_address_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_get_mac_address_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get mac address: %s\n", error->message);
        g_error_free (error);
        qmi_message_dms_get_mac_address_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    qmi_message_dms_get_mac_address_output_get_mac_address (output, &mac_address, NULL);
    mac_address_printable = qmicli_get_raw_data_printable (mac_address, 80, "\t\t");

    g_print ("[%s] MAC address read:\n"
             "\tSize: '%u' bytes\n"
             "\tContents:\n"
             "%s",
             qmi_device_get_path_display (ctx->device),
             mac_address->len,
             mac_address_printable);
    g_free (mac_address_printable);

    qmi_message_dms_get_mac_address_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_DMS_GET_MAC_ADDRESS */

#if defined HAVE_QMI_MESSAGE_DMS_RESET

static void
reset_ready (QmiClientDms *client,
             GAsyncResult *res)
{
    QmiMessageDmsResetOutput *output;
    GError *error = NULL;

    output = qmi_client_dms_reset_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_dms_reset_output_get_result (output, &error)) {
        g_printerr ("error: couldn't reset the DMS service: %s\n", error->message);
        g_error_free (error);
        qmi_message_dms_reset_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] Successfully performed DMS service reset\n",
             qmi_device_get_path_display (ctx->device));

    qmi_message_dms_reset_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_DMS_RESET */

static gboolean
noop_cb (gpointer unused)
{
    operation_shutdown (TRUE);
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
    ctx->cancellable = g_object_ref (cancellable);

#if defined HAVE_QMI_MESSAGE_DMS_GET_IDS
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
#endif

#if defined HAVE_QMI_MESSAGE_DMS_GET_CAPABILITIES
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
#endif

#if defined HAVE_QMI_MESSAGE_DMS_GET_MANUFACTURER
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
#endif

#if defined HAVE_QMI_MESSAGE_DMS_GET_MODEL
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
#endif

#if defined HAVE_QMI_MESSAGE_DMS_GET_REVISION
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
#endif

#if defined HAVE_QMI_MESSAGE_DMS_GET_MSISDN
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
#endif

#if defined HAVE_QMI_MESSAGE_DMS_GET_POWER_STATE
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
#endif

#if defined HAVE_QMI_MESSAGE_DMS_UIM_SET_PIN_PROTECTION
    if (uim_set_pin_protection_str) {
        QmiMessageDmsUimSetPinProtectionInput *input;

        g_debug ("Asynchronously setting PIN protection...");
        input = uim_set_pin_protection_input_create (uim_set_pin_protection_str);
        if (!input) {
            operation_shutdown (FALSE);
            return;
        }
        qmi_client_dms_uim_set_pin_protection (ctx->client,
                                               input,
                                               10,
                                               ctx->cancellable,
                                               (GAsyncReadyCallback)uim_set_pin_protection_ready,
                                               NULL);
        qmi_message_dms_uim_set_pin_protection_input_unref (input);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_DMS_UIM_VERIFY_PIN
    if (uim_verify_pin_str) {
        QmiMessageDmsUimVerifyPinInput *input;

        g_debug ("Asynchronously verifying PIN...");
        input = uim_verify_pin_input_create (uim_verify_pin_str);
        if (!input) {
            operation_shutdown (FALSE);
            return;
        }
        qmi_client_dms_uim_verify_pin (ctx->client,
                                       input,
                                       10,
                                       ctx->cancellable,
                                       (GAsyncReadyCallback)uim_verify_pin_ready,
                                       NULL);
        qmi_message_dms_uim_verify_pin_input_unref (input);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_DMS_UIM_UNBLOCK_PIN
    if (uim_unblock_pin_str) {
        QmiMessageDmsUimUnblockPinInput *input;

        g_debug ("Asynchronously unblocking PIN...");
        input = uim_unblock_pin_input_create (uim_unblock_pin_str);
        if (!input) {
            operation_shutdown (FALSE);
            return;
        }
        qmi_client_dms_uim_unblock_pin (ctx->client,
                                        input,
                                        10,
                                        ctx->cancellable,
                                        (GAsyncReadyCallback)uim_unblock_pin_ready,
                                        NULL);
        qmi_message_dms_uim_unblock_pin_input_unref (input);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_DMS_UIM_CHANGE_PIN
    if (uim_change_pin_str) {
        QmiMessageDmsUimChangePinInput *input;

        g_debug ("Asynchronously changing PIN...");
        input = uim_change_pin_input_create (uim_change_pin_str);
        if (!input) {
            operation_shutdown (FALSE);
            return;
        }
        qmi_client_dms_uim_change_pin (ctx->client,
                                       input,
                                       10,
                                       ctx->cancellable,
                                       (GAsyncReadyCallback)uim_change_pin_ready,
                                       NULL);
        qmi_message_dms_uim_change_pin_input_unref (input);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_DMS_UIM_GET_PIN_STATUS
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
#endif

#if defined HAVE_QMI_MESSAGE_DMS_UIM_GET_ICCID
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
#endif

#if defined HAVE_QMI_MESSAGE_DMS_UIM_GET_IMSI
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
#endif

#if defined HAVE_QMI_MESSAGE_DMS_UIM_GET_STATE
    if (uim_get_state_flag) {
        g_debug ("Asynchronously getting UIM state...");
        qmi_client_dms_uim_get_state (ctx->client,
                                      NULL,
                                      10,
                                      ctx->cancellable,
                                      (GAsyncReadyCallback)uim_get_state_ready,
                                      NULL);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_DMS_GET_HARDWARE_REVISION
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
#endif

#if defined HAVE_QMI_MESSAGE_DMS_GET_OPERATING_MODE
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
#endif

#if defined HAVE_QMI_MESSAGE_DMS_SET_OPERATING_MODE
    if (set_operating_mode_str) {
        QmiMessageDmsSetOperatingModeInput *input;

        g_debug ("Asynchronously setting operating mode...");
        input = set_operating_mode_input_create (set_operating_mode_str);
        if (!input) {
            operation_shutdown (FALSE);
            return;
        }
        qmi_client_dms_set_operating_mode (ctx->client,
                                           input,
                                           10,
                                           ctx->cancellable,
                                           (GAsyncReadyCallback)set_operating_mode_ready,
                                           NULL);
        qmi_message_dms_set_operating_mode_input_unref (input);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_DMS_GET_TIME
    if (get_time_flag) {
        g_debug ("Asynchronously getting time...");
        qmi_client_dms_get_time (ctx->client,
                                 NULL,
                                 10,
                                 ctx->cancellable,
                                 (GAsyncReadyCallback)get_time_ready,
                                 NULL);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_DMS_GET_PRL_VERSION
    if (get_prl_version_flag) {
        g_debug ("Asynchronously getting PRL version...");
        qmi_client_dms_get_prl_version (ctx->client,
                                        NULL,
                                        10,
                                        ctx->cancellable,
                                        (GAsyncReadyCallback)get_prl_version_ready,
                                        NULL);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_DMS_GET_ACTIVATION_STATE
    if (get_activation_state_flag) {
        g_debug ("Asynchronously getting activation state...");
        qmi_client_dms_get_activation_state (ctx->client,
                                             NULL,
                                             10,
                                             ctx->cancellable,
                                             (GAsyncReadyCallback)get_activation_state_ready,
                                             NULL);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_DMS_ACTIVATE_AUTOMATIC
    if (activate_automatic_str) {
        QmiMessageDmsActivateAutomaticInput *input;

        g_debug ("Asynchronously requesting automatic activation...");
        input = activate_automatic_input_create (activate_automatic_str);
        if (!input) {
            operation_shutdown (FALSE);
            return;
        }
        qmi_client_dms_activate_automatic (ctx->client,
                                           input,
                                           10,
                                           ctx->cancellable,
                                           (GAsyncReadyCallback)activate_automatic_ready,
                                           NULL);
        qmi_message_dms_activate_automatic_input_unref (input);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_DMS_ACTIVATE_MANUAL
    if (activate_manual_str) {
        QmiMessageDmsActivateManualInput *input;

        g_debug ("Asynchronously requesting manual activation...");
        input = activate_manual_input_create (activate_manual_str);
        if (!input) {
            operation_shutdown (FALSE);
            return;
        }
        qmi_client_dms_activate_manual (ctx->client,
                                        input,
                                        10,
                                        ctx->cancellable,
                                        (GAsyncReadyCallback)activate_manual_ready,
                                        NULL);
        qmi_message_dms_activate_manual_input_unref (input);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_DMS_GET_USER_LOCK_STATE
    if (get_user_lock_state_flag) {
        g_debug ("Asynchronously getting user lock state...");
        qmi_client_dms_get_user_lock_state (ctx->client,
                                            NULL,
                                            10,
                                            ctx->cancellable,
                                            (GAsyncReadyCallback)get_user_lock_state_ready,
                                            NULL);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_DMS_SET_USER_LOCK_STATE
    if (set_user_lock_state_str) {
        QmiMessageDmsSetUserLockStateInput *input;

        g_debug ("Asynchronously setting user lock state...");
        input = set_user_lock_state_input_create (set_user_lock_state_str);
        if (!input) {
            operation_shutdown (FALSE);
            return;
        }
        qmi_client_dms_set_user_lock_state (ctx->client,
                                            input,
                                            10,
                                            ctx->cancellable,
                                            (GAsyncReadyCallback)set_user_lock_state_ready,
                                            NULL);
        qmi_message_dms_set_user_lock_state_input_unref (input);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_DMS_SET_USER_LOCK_CODE
    if (set_user_lock_code_str) {
        QmiMessageDmsSetUserLockCodeInput *input;

        g_debug ("Asynchronously changing user lock code...");
        input = set_user_lock_code_input_create (set_user_lock_code_str);
        if (!input) {
            operation_shutdown (FALSE);
            return;
        }
        qmi_client_dms_set_user_lock_code (ctx->client,
                                           input,
                                           10,
                                           ctx->cancellable,
                                           (GAsyncReadyCallback)set_user_lock_code_ready,
                                            NULL);
        qmi_message_dms_set_user_lock_code_input_unref (input);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_DMS_READ_USER_DATA
    if (read_user_data_flag) {
        g_debug ("Asynchronously reading user data...");
        qmi_client_dms_read_user_data (ctx->client,
                                       NULL,
                                       10,
                                       ctx->cancellable,
                                       (GAsyncReadyCallback)read_user_data_ready,
                                       NULL);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_DMS_WRITE_USER_DATA
    if (write_user_data_str) {
        QmiMessageDmsWriteUserDataInput *input;

        g_debug ("Asynchronously writing user data...");
        input = write_user_data_input_create (write_user_data_str);
        if (!input) {
            operation_shutdown (FALSE);
            return;
        }
        qmi_client_dms_write_user_data (ctx->client,
                                        input,
                                        10,
                                        ctx->cancellable,
                                        (GAsyncReadyCallback)write_user_data_ready,
                                        NULL);
        qmi_message_dms_write_user_data_input_unref (input);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_DMS_READ_ERI_FILE
    if (read_eri_file_flag) {
        g_debug ("Asynchronously reading ERI file...");
        qmi_client_dms_read_eri_file (ctx->client,
                                      NULL,
                                      10,
                                      ctx->cancellable,
                                      (GAsyncReadyCallback)read_eri_file_ready,
                                      NULL);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_DMS_RESTORE_FACTORY_DEFAULTS
    if (restore_factory_defaults_str) {
        QmiMessageDmsRestoreFactoryDefaultsInput *input;

        g_debug ("Asynchronously restoring factory defaults...");
        input = restore_factory_defaults_input_create (restore_factory_defaults_str);
        if (!input) {
            operation_shutdown (FALSE);
            return;
        }
        qmi_client_dms_restore_factory_defaults (ctx->client,
                                                 input,
                                                 10,
                                                 ctx->cancellable,
                                                 (GAsyncReadyCallback)restore_factory_defaults_ready,
                                                 NULL);
        qmi_message_dms_restore_factory_defaults_input_unref (input);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_DMS_VALIDATE_SERVICE_PROGRAMMING_CODE
    if (validate_service_programming_code_str) {
        QmiMessageDmsValidateServiceProgrammingCodeInput *input;

        g_debug ("Asynchronously validating SPC...");
        input = validate_service_programming_code_input_create (validate_service_programming_code_str);
        if (!input) {
            operation_shutdown (FALSE);
            return;
        }
        qmi_client_dms_validate_service_programming_code (ctx->client,
                                                          input,
                                                          10,
                                                          ctx->cancellable,
                                                          (GAsyncReadyCallback)validate_service_programming_code_ready,
                                                          NULL);
        qmi_message_dms_validate_service_programming_code_input_unref (input);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_DMS_SET_FIRMWARE_ID
    if (set_firmware_id_flag) {
        g_debug ("Asynchronously setting firmware id...");
        qmi_client_dms_set_firmware_id (ctx->client,
                                        NULL,
                                        10,
                                        ctx->cancellable,
                                        (GAsyncReadyCallback)set_firmware_id_ready,
                                        NULL);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_DMS_UIM_GET_CK_STATUS
    if (uim_get_ck_status_str) {
        QmiMessageDmsUimGetCkStatusInput *input;

        g_debug ("Asynchronously getting CK status...");
        input = uim_get_ck_status_input_create (uim_get_ck_status_str);
        if (!input) {
            operation_shutdown (FALSE);
            return;
        }
        qmi_client_dms_uim_get_ck_status (ctx->client,
                                          input,
                                          10,
                                          ctx->cancellable,
                                          (GAsyncReadyCallback)uim_get_ck_status_ready,
                                          NULL);
        qmi_message_dms_uim_get_ck_status_input_unref (input);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_DMS_UIM_SET_CK_PROTECTION
    if (uim_set_ck_protection_str) {
        QmiMessageDmsUimSetCkProtectionInput *input;

        g_debug ("Asynchronously setting CK protection...");
        input = uim_set_ck_protection_input_create (uim_set_ck_protection_str);
        if (!input) {
            operation_shutdown (FALSE);
            return;
        }
        qmi_client_dms_uim_set_ck_protection (ctx->client,
                                              input,
                                              10,
                                              ctx->cancellable,
                                              (GAsyncReadyCallback)uim_set_ck_protection_ready,
                                              NULL);
        qmi_message_dms_uim_set_ck_protection_input_unref (input);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_DMS_UIM_UNBLOCK_CK
    if (uim_unblock_ck_str) {
        QmiMessageDmsUimUnblockCkInput *input;

        g_debug ("Asynchronously unblocking CK...");
        input = uim_unblock_ck_input_create (uim_unblock_ck_str);
        if (!input) {
            operation_shutdown (FALSE);
            return;
        }
        qmi_client_dms_uim_unblock_ck (ctx->client,
                                       input,
                                       10,
                                       ctx->cancellable,
                                       (GAsyncReadyCallback)uim_unblock_ck_ready,
                                       NULL);
        qmi_message_dms_uim_unblock_ck_input_unref (input);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_DMS_GET_BAND_CAPABILITIES
    if (get_band_capabilities_flag) {
        g_debug ("Asynchronously getting band capabilities...");
        qmi_client_dms_get_band_capabilities (ctx->client,
                                              NULL,
                                              10,
                                              ctx->cancellable,
                                              (GAsyncReadyCallback)get_band_capabilities_ready,
                                              NULL);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_DMS_GET_FACTORY_SKU
    if (get_factory_sku_flag) {
        g_debug ("Asynchronously getting factory SKU...");
        qmi_client_dms_get_factory_sku (ctx->client,
                                        NULL,
                                        10,
                                        ctx->cancellable,
                                        (GAsyncReadyCallback)get_factory_sku_ready,
                                        NULL);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_DMS_LIST_STORED_IMAGES && defined HAVE_QMI_MESSAGE_DMS_GET_STORED_IMAGE_INFO
    if (list_stored_images_flag) {
        g_debug ("Asynchronously listing stored images...");
        qmi_client_dms_list_stored_images (ctx->client,
                                           NULL,
                                           10,
                                           ctx->cancellable,
                                           (GAsyncReadyCallback)list_stored_images_ready,
                                           NULL);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_DMS_SET_FIRMWARE_PREFERENCE && \
    defined HAVE_QMI_MESSAGE_DMS_LIST_STORED_IMAGES
    if (select_stored_image_str) {
        g_debug ("Asynchronously selecting stored image...");
        get_stored_image (ctx->client,
                          select_stored_image_str,
                          (GAsyncReadyCallback)get_stored_image_select_ready,
                          NULL);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_DMS_SET_FIRMWARE_PREFERENCE && \
    defined HAVE_QMI_MESSAGE_DMS_LIST_STORED_IMAGES && \
    defined HAVE_QMI_MESSAGE_DMS_DELETE_STORED_IMAGE
    if (delete_stored_image_str) {
        g_debug ("Asynchronously deleting stored image...");
        get_stored_image (ctx->client,
                          delete_stored_image_str,
                          (GAsyncReadyCallback)get_stored_image_delete_ready,
                          NULL);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_DMS_GET_FIRMWARE_PREFERENCE
    if (get_firmware_preference_flag) {
        qmi_client_dms_get_firmware_preference (
            client,
            NULL,
            10,
            NULL,
            (GAsyncReadyCallback)dms_get_firmware_preference_ready,
            NULL);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_DMS_SET_FIRMWARE_PREFERENCE
    if (set_firmware_preference_str) {
        g_autoptr(GError)                                  error = NULL;
        g_autoptr(QmiMessageDmsSetFirmwarePreferenceInput) input = NULL;
        SetFirmwarePreferenceContext                       firmware_preference_ctx;

        memset (&firmware_preference_ctx, 0, sizeof (firmware_preference_ctx));

        g_debug ("Asynchronously setting firmware preference...");
        input = set_firmware_preference_input_create (set_firmware_preference_str, &firmware_preference_ctx, &error);
        if (!input) {
            g_printerr ("error: %s\n", error->message);
            operation_shutdown (FALSE);
            return;
        }

        qmi_client_dms_set_firmware_preference (
            client,
            input,
            10,
            NULL,
            (GAsyncReadyCallback)dms_set_firmware_preference_ready,
            NULL);
        set_firmware_preference_context_clear (&firmware_preference_ctx);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_DMS_GET_BOOT_IMAGE_DOWNLOAD_MODE
    if (get_boot_image_download_mode_flag) {
        g_debug ("Asynchronously getting boot image download mode...");
        qmi_client_dms_get_boot_image_download_mode (ctx->client,
                                                     NULL,
                                                     10,
                                                     ctx->cancellable,
                                                     (GAsyncReadyCallback)get_boot_image_download_mode_ready,
                                                     NULL);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_DMS_SET_BOOT_IMAGE_DOWNLOAD_MODE
    if (set_boot_image_download_mode_str) {
        QmiMessageDmsSetBootImageDownloadModeInput *input;

        g_debug ("Asynchronously setting boot image download mode...");
        input = set_boot_image_download_mode_input_create (set_boot_image_download_mode_str);
        if (!input) {
            operation_shutdown (FALSE);
            return;
        }
        qmi_client_dms_set_boot_image_download_mode (ctx->client,
                                                     input,
                                                     10,
                                                     ctx->cancellable,
                                                     (GAsyncReadyCallback)set_boot_image_download_mode_ready,
                                                     NULL);
        qmi_message_dms_set_boot_image_download_mode_input_unref (input);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_DMS_GET_SOFTWARE_VERSION
    if (get_software_version_flag) {
        g_debug ("Asynchronously getting software version...");
        qmi_client_dms_get_software_version (ctx->client,
                                             NULL,
                                             10,
                                             ctx->cancellable,
                                             (GAsyncReadyCallback)get_software_version_ready,
                                             NULL);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_DMS_SET_FCC_AUTHENTICATION
    if (set_fcc_authentication_flag) {
        g_debug ("Asynchronously setting FCC auth...");
        qmi_client_dms_set_fcc_authentication (ctx->client,
                                               NULL,
                                               10,
                                               ctx->cancellable,
                                               (GAsyncReadyCallback)set_fcc_authentication_ready,
                                               NULL);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_DMS_GET_SUPPORTED_MESSAGES
    if (get_supported_messages_flag) {
        g_debug ("Asynchronously getting supported DMS messages...");
        qmi_client_dms_get_supported_messages (ctx->client,
                                               NULL,
                                               10,
                                               ctx->cancellable,
                                               (GAsyncReadyCallback)get_supported_messages_ready,
                                               NULL);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_DMS_HP_CHANGE_DEVICE_MODE
    if (hp_change_device_mode_str) {
        QmiMessageDmsHpChangeDeviceModeInput *input;

        g_debug ("Asynchronously changing device mode (HP specific)...");

        input = hp_change_device_mode_input_create (hp_change_device_mode_str);
        if (!input) {
            operation_shutdown (FALSE);
            return;
        }

        qmi_client_dms_hp_change_device_mode (ctx->client,
                                              input,
                                              10,
                                              ctx->cancellable,
                                              (GAsyncReadyCallback)hp_change_device_mode_ready,
                                              NULL);
        qmi_message_dms_hp_change_device_mode_input_unref (input);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_DMS_SWI_GET_CURRENT_FIRMWARE
    if (swi_get_current_firmware_flag) {
        g_debug ("Asynchronously getting current firmware (Sierra Wireless specific)...");
        qmi_client_dms_swi_get_current_firmware (ctx->client,
                                                 NULL,
                                                 10,
                                                 ctx->cancellable,
                                                 (GAsyncReadyCallback)swi_get_current_firmware_ready,
                                                 NULL);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_DMS_SWI_GET_USB_COMPOSITION
    if (swi_get_usb_composition_flag) {
        g_debug ("Asynchronously getting USB compositionss (Sierra Wireless specific)...");
        qmi_client_dms_swi_get_usb_composition (ctx->client,
                                                NULL,
                                                10,
                                                ctx->cancellable,
                                                (GAsyncReadyCallback)swi_get_usb_composition_ready,
                                                NULL);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_DMS_SWI_SET_USB_COMPOSITION
    if (swi_set_usb_composition_str) {
        QmiMessageDmsSwiSetUsbCompositionInput *input;

        g_debug ("Asynchronously setting USB composition (Sierra Wireless specific)...");

        input = swi_set_usb_composition_input_create (swi_set_usb_composition_str);
        if (!input) {
            operation_shutdown (FALSE);
            return;
        }

        qmi_client_dms_swi_set_usb_composition  (ctx->client,
                                                 input,
                                                 10,
                                                 ctx->cancellable,
                                                 (GAsyncReadyCallback)swi_set_usb_composition_ready,
                                              NULL);
        qmi_message_dms_swi_set_usb_composition_input_unref (input);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_DMS_FOXCONN_CHANGE_DEVICE_MODE
    if (foxconn_change_device_mode_str || dell_change_device_mode_str) {
        QmiMessageDmsFoxconnChangeDeviceModeInput *input;

        g_debug ("Asynchronously changing device mode (Foxconn specific)...");

        input = foxconn_change_device_mode_input_create (foxconn_change_device_mode_str ? foxconn_change_device_mode_str : dell_change_device_mode_str);
        if (!input) {
            operation_shutdown (FALSE);
            return;
        }

        qmi_client_dms_foxconn_change_device_mode (ctx->client,
                                                   input,
                                                   10,
                                                   ctx->cancellable,
                                                   (GAsyncReadyCallback)foxconn_change_device_mode_ready,
                                                   NULL);
        qmi_message_dms_foxconn_change_device_mode_input_unref (input);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_DMS_FOXCONN_GET_FIRMWARE_VERSION
    if (foxconn_get_firmware_version_str || dell_get_firmware_version_str) {
        QmiMessageDmsFoxconnGetFirmwareVersionInput *input;

        g_debug ("Asynchronously getting firmware version (Foxconn specific)...");

        input = foxconn_get_firmware_version_input_create (foxconn_get_firmware_version_str ? foxconn_get_firmware_version_str : dell_get_firmware_version_str);
        if (!input) {
            operation_shutdown (FALSE);
            return;
        }

        qmi_client_dms_foxconn_get_firmware_version (ctx->client,
                                                     input,
                                                     10,
                                                     ctx->cancellable,
                                                     (GAsyncReadyCallback)foxconn_get_firmware_version_ready,
                                                     NULL);
        qmi_message_dms_foxconn_get_firmware_version_input_unref (input);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_DMS_FOXCONN_SET_FCC_AUTHENTICATION
    if (foxconn_set_fcc_authentication_int >= 0) {
        g_autoptr(QmiMessageDmsFoxconnSetFccAuthenticationInput) input = NULL;

        if (foxconn_set_fcc_authentication_int > 0xFF) {
            g_printerr ("error: magic value out of [0,255] range\n");
            operation_shutdown (FALSE);
            return;
        }

        g_debug ("Asynchronously running Foxconn FCC authentication...");

        input = qmi_message_dms_foxconn_set_fcc_authentication_input_new ();
        qmi_message_dms_foxconn_set_fcc_authentication_input_set_value (input, (guint8)foxconn_set_fcc_authentication_int, NULL);
        qmi_client_dms_foxconn_set_fcc_authentication (ctx->client,
                                                       input,
                                                       10,
                                                       ctx->cancellable,
                                                       (GAsyncReadyCallback)foxconn_set_fcc_authentication_ready,
                                                       NULL);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_DMS_GET_MAC_ADDRESS
    if (get_mac_address_str) {
        QmiMessageDmsGetMacAddressInput *input;

        g_debug ("Asynchronously getting MAC address...");

        input = get_mac_address_input_create (get_mac_address_str);
        if (!input) {
            operation_shutdown (FALSE);
            return;
        }

        qmi_client_dms_get_mac_address (ctx->client,
                                        input,
                                        10,
                                        ctx->cancellable,
                                        (GAsyncReadyCallback)get_mac_address_ready,
                                        NULL);
        qmi_message_dms_get_mac_address_input_unref (input);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_DMS_RESET
    if (reset_flag) {
        g_debug ("Asynchronously resetting DMS service...");
        qmi_client_dms_reset (ctx->client,
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

#endif /* HAVE_QMI_SERVICE_DMS */
