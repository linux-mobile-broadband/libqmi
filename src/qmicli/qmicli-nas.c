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
#include "qmicli-charsets.h"

/* Context */
typedef struct {
    QmiDevice *device;
    QmiClientNas *client;
    GCancellable *cancellable;
} Context;
static Context *ctx;

/* Options */
static gboolean get_signal_strength_flag;
static gboolean get_signal_info_flag;
static gchar *get_tx_rx_info_str;
static gboolean get_home_network_flag;
static gboolean get_serving_system_flag;
static gboolean get_system_info_flag;
static gboolean get_technology_preference_flag;
static gboolean get_system_selection_preference_flag;
static gchar *set_system_selection_preference_str;
static gboolean network_scan_flag;
static gboolean get_cell_location_info_flag;
static gboolean force_network_search_flag;
static gboolean get_operator_name_flag;
static gboolean get_lte_cphy_ca_info_flag;
static gboolean get_rf_band_info_flag;
static gboolean get_supported_messages_flag;
static gboolean reset_flag;
static gboolean noop_flag;

static GOptionEntry entries[] = {
    { "nas-get-signal-strength", 0, 0, G_OPTION_ARG_NONE, &get_signal_strength_flag,
      "Get signal strength",
      NULL
    },
    { "nas-get-signal-info", 0, 0, G_OPTION_ARG_NONE, &get_signal_info_flag,
      "Get signal info",
      NULL
    },
    { "nas-get-tx-rx-info", 0, 0, G_OPTION_ARG_STRING, &get_tx_rx_info_str,
      "Get TX/RX info",
      "[(Radio Interface)]",
    },
    { "nas-get-home-network", 0, 0, G_OPTION_ARG_NONE, &get_home_network_flag,
      "Get home network",
      NULL
    },
    { "nas-get-serving-system", 0, 0, G_OPTION_ARG_NONE, &get_serving_system_flag,
      "Get serving system",
      NULL
    },
    { "nas-get-system-info", 0, 0, G_OPTION_ARG_NONE, &get_system_info_flag,
      "Get system info",
      NULL
    },
    { "nas-get-technology-preference", 0, 0, G_OPTION_ARG_NONE, &get_technology_preference_flag,
      "Get technology preference",
      NULL
    },
    { "nas-get-system-selection-preference", 0, 0, G_OPTION_ARG_NONE, &get_system_selection_preference_flag,
      "Get system selection preference",
      NULL
    },
    { "nas-set-system-selection-preference", 0, 0, G_OPTION_ARG_STRING, &set_system_selection_preference_str,
      "Set system selection preference",
      "[cdma-1x|cdma-1xevdo|gsm|umts|lte|td-scdma]"
    },
    { "nas-network-scan", 0, 0, G_OPTION_ARG_NONE, &network_scan_flag,
      "Scan networks",
      NULL
    },
    { "nas-get-cell-location-info", 0, 0, G_OPTION_ARG_NONE, &get_cell_location_info_flag,
      "Get Cell Location Info",
      NULL
    },
    { "nas-force-network-search", 0, 0, G_OPTION_ARG_NONE, &force_network_search_flag,
      "Force network search",
      NULL
    },
    { "nas-get-operator-name", 0, 0, G_OPTION_ARG_NONE, &get_operator_name_flag,
      "Get operator name data",
      NULL
    },
    { "nas-get-lte-cphy-ca-info", 0, 0, G_OPTION_ARG_NONE, &get_lte_cphy_ca_info_flag,
      "Get LTE Cphy CA Info",
      NULL
    },
    { "nas-get-rf-band-info", 0, 0, G_OPTION_ARG_NONE, &get_rf_band_info_flag,
      "Get RF Band Info",
      NULL
    },
    { "nas-get-supported-messages", 0, 0, G_OPTION_ARG_NONE, &get_supported_messages_flag,
      "Get supported messages",
      NULL
    },
    { "nas-reset", 0, 0, G_OPTION_ARG_NONE, &reset_flag,
      "Reset the service state",
      NULL
    },
    { "nas-noop", 0, 0, G_OPTION_ARG_NONE, &noop_flag,
      "Just allocate or release a NAS client. Use with `--client-no-release-cid' and/or `--client-cid'",
      NULL
    },
    { NULL }
};

GOptionGroup *
qmicli_nas_get_option_group (void)
{
    GOptionGroup *group;

    group = g_option_group_new ("nas",
                                "NAS options",
                                "Show Network Access Service options",
                                NULL,
                                NULL);
    g_option_group_add_entries (group, entries);

    return group;
}

gboolean
qmicli_nas_options_enabled (void)
{
    static guint n_actions = 0;
    static gboolean checked = FALSE;

    if (checked)
        return !!n_actions;

    n_actions = (get_signal_strength_flag +
                 get_signal_info_flag +
                 !!get_tx_rx_info_str +
                 get_home_network_flag +
                 get_serving_system_flag +
                 get_system_info_flag +
                 get_technology_preference_flag +
                 get_system_selection_preference_flag +
                 !!set_system_selection_preference_str +
                 network_scan_flag +
                 get_cell_location_info_flag +
                 force_network_search_flag +
                 get_operator_name_flag +
                 get_lte_cphy_ca_info_flag +
                 get_rf_band_info_flag +
                 get_supported_messages_flag +
                 reset_flag +
                 noop_flag);

    if (n_actions > 1) {
        g_printerr ("error: too many NAS actions requested\n");
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

static gboolean
get_db_from_sinr_level (QmiNasEvdoSinrLevel level,
                        gdouble *out)
{
    g_assert (out != NULL);

    switch (level) {
    case QMI_NAS_EVDO_SINR_LEVEL_0:
        *out = -9.0;
        return TRUE;
    case QMI_NAS_EVDO_SINR_LEVEL_1:
        *out = -6.0;
        return TRUE;
    case QMI_NAS_EVDO_SINR_LEVEL_2:
        *out = -4.5;
        return TRUE;
    case QMI_NAS_EVDO_SINR_LEVEL_3:
        *out = -3.0;
        return TRUE;
    case QMI_NAS_EVDO_SINR_LEVEL_4:
        *out = -2.0;
        return TRUE;
    case QMI_NAS_EVDO_SINR_LEVEL_5:
        *out = 1.0;
        return TRUE;
    case QMI_NAS_EVDO_SINR_LEVEL_6:
        *out = 3.0;
        return TRUE;
    case QMI_NAS_EVDO_SINR_LEVEL_7:
        *out = 6.0;
        return TRUE;
    case QMI_NAS_EVDO_SINR_LEVEL_8:
        *out = 9.0;
        return TRUE;
    default:
        g_warning ("Invalid SINR level '%u'", level);
        return FALSE;
    }
}

static void
get_signal_info_ready (QmiClientNas *client,
                       GAsyncResult *res)
{
    QmiMessageNasGetSignalInfoOutput *output;
    GError *error = NULL;
    gint8 rssi;
    gint16 ecio;
    QmiNasEvdoSinrLevel sinr_level;
    gint32 io;
    gint8 rsrq;
    gint16 rsrp;
    gint16 snr;
    gint8 rscp;

    output = qmi_client_nas_get_signal_info_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_nas_get_signal_info_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get signal info: %s\n", error->message);
        g_error_free (error);
        qmi_message_nas_get_signal_info_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] Successfully got signal info\n",
             qmi_device_get_path_display (ctx->device));

    /* CDMA... */
    if (qmi_message_nas_get_signal_info_output_get_cdma_signal_strength (output,
                                                                         &rssi,
                                                                         &ecio,
                                                                         NULL)) {
        g_print ("CDMA:\n"
                 "\tRSSI: '%d dBm'\n"
                 "\tECIO: '%.1lf dBm'\n",
                 rssi,
                 (-0.5)*((gdouble)ecio));
    }

    /* HDR... */
    if (qmi_message_nas_get_signal_info_output_get_hdr_signal_strength (output,
                                                                        &rssi,
                                                                        &ecio,
                                                                        &sinr_level,
                                                                        &io,
                                                                        NULL)) {
        gdouble db_sinr = 0.0;

        g_print ("HDR:\n"
                 "\tRSSI: '%d dBm'\n"
                 "\tECIO: '%.1lf dBm'\n"
                 "\tIO: '%d dBm'\n",
                 rssi,
                 (-0.5)*((gdouble)ecio),
                 io);

        if (get_db_from_sinr_level (sinr_level, &db_sinr))
            g_print ("\tSINR (%u): '%.1lf dB'\n", sinr_level, db_sinr);
        else
            g_print ("\tSINR (%u): N/A'\n", sinr_level);
    }

    /* GSM */
    if (qmi_message_nas_get_signal_info_output_get_gsm_signal_strength (output,
                                                                        &rssi,
                                                                        NULL)) {
        g_print ("GSM:\n"
                 "\tRSSI: '%d dBm'\n",
                 rssi);
    }

    /* WCDMA... */
    if (qmi_message_nas_get_signal_info_output_get_wcdma_signal_strength (output,
                                                                          &rssi,
                                                                          &ecio,
                                                                          NULL)) {
        g_print ("WCDMA:\n"
                 "\tRSSI: '%d dBm'\n"
                 "\tECIO: '%.1lf dBm'\n",
                 rssi,
                 (-0.5)*((gdouble)ecio));
    }

    /* LTE... */
    if (qmi_message_nas_get_signal_info_output_get_lte_signal_strength (output,
                                                                        &rssi,
                                                                        &rsrq,
                                                                        &rsrp,
                                                                        &snr,
                                                                        NULL)) {
        g_print ("LTE:\n"
                 "\tRSSI: '%d dBm'\n"
                 "\tRSRQ: '%d dB'\n"
                 "\tRSRP: '%d dBm'\n"
                 "\tSNR: '%.1lf dB'\n",
                 rssi,
                 rsrq,
                 rsrp,
                 (0.1) * ((gdouble)snr));
    }

    /* TDMA */
    if (qmi_message_nas_get_signal_info_output_get_tdma_signal_strength (output,
                                                                         &rscp,
                                                                         NULL)) {
        g_print ("TDMA:\n"
                 "\tRSCP: '%d dBm'\n",
                 rscp);
    }

    qmi_message_nas_get_signal_info_output_unref (output);
    operation_shutdown (TRUE);
}

static QmiMessageNasGetSignalStrengthInput *
get_signal_strength_input_create (void)
{
    GError *error = NULL;
    QmiMessageNasGetSignalStrengthInput *input;
    QmiNasSignalStrengthRequest mask;

    mask = (QMI_NAS_SIGNAL_STRENGTH_REQUEST_RSSI |
            QMI_NAS_SIGNAL_STRENGTH_REQUEST_ECIO |
            QMI_NAS_SIGNAL_STRENGTH_REQUEST_IO |
            QMI_NAS_SIGNAL_STRENGTH_REQUEST_SINR |
            QMI_NAS_SIGNAL_STRENGTH_REQUEST_RSRQ |
            QMI_NAS_SIGNAL_STRENGTH_REQUEST_LTE_SNR |
            QMI_NAS_SIGNAL_STRENGTH_REQUEST_LTE_RSRP);

    input = qmi_message_nas_get_signal_strength_input_new ();
    if (!qmi_message_nas_get_signal_strength_input_set_request_mask (
            input,
            mask,
            &error)) {
        g_printerr ("error: couldn't create input data bundle: '%s'\n",
                    error->message);
        g_error_free (error);
        qmi_message_nas_get_signal_strength_input_unref (input);
        input = NULL;
    }

    return input;
}

static void
get_signal_strength_ready (QmiClientNas *client,
                           GAsyncResult *res)
{
    QmiMessageNasGetSignalStrengthOutput *output;
    GError *error = NULL;
    GArray *array;
    QmiNasRadioInterface radio_interface;
    gint8 strength;
    gint32 io;
    QmiNasEvdoSinrLevel sinr_level;
    gint8 rsrq;
    gint16 rsrp;
    gint16 snr;

    output = qmi_client_nas_get_signal_strength_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_nas_get_signal_strength_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get signal strength: %s\n", error->message);
        g_error_free (error);
        qmi_message_nas_get_signal_strength_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    qmi_message_nas_get_signal_strength_output_get_signal_strength (output,
                                                                    &strength,
                                                                    &radio_interface,
                                                                    NULL);

    g_print ("[%s] Successfully got signal strength\n"
             "Current:\n"
             "\tNetwork '%s': '%d dBm'\n",
             qmi_device_get_path_display (ctx->device),
             qmi_nas_radio_interface_get_string (radio_interface),
             strength);

    /* Other signal strengths in other networks... */
    if (qmi_message_nas_get_signal_strength_output_get_strength_list (output, &array, NULL)) {
        guint i;

        g_print ("Other:\n");
        for (i = 0; i < array->len; i++) {
            QmiMessageNasGetSignalStrengthOutputStrengthListElement *element;

            element = &g_array_index (array, QmiMessageNasGetSignalStrengthOutputStrengthListElement, i);
            g_print ("\tNetwork '%s': '%d dBm'\n",
                     qmi_nas_radio_interface_get_string (element->radio_interface),
                     element->strength);
        }
    }

    /* RSSI... */
    if (qmi_message_nas_get_signal_strength_output_get_rssi_list (output, &array, NULL)) {
        guint i;

        g_print ("RSSI:\n");
        for (i = 0; i < array->len; i++) {
            QmiMessageNasGetSignalStrengthOutputRssiListElement *element;

            element = &g_array_index (array, QmiMessageNasGetSignalStrengthOutputRssiListElement, i);
            g_print ("\tNetwork '%s': '%d dBm'\n",
                     qmi_nas_radio_interface_get_string (element->radio_interface),
                     (-1) * element->rssi);
        }
    }

    /* ECIO... */
    if (qmi_message_nas_get_signal_strength_output_get_ecio_list (output, &array, NULL)) {
        guint i;

        g_print ("ECIO:\n");
        for (i = 0; i < array->len; i++) {
            QmiMessageNasGetSignalStrengthOutputEcioListElement *element;

            element = &g_array_index (array, QmiMessageNasGetSignalStrengthOutputEcioListElement, i);
            g_print ("\tNetwork '%s': '%.1lf dBm'\n",
                     qmi_nas_radio_interface_get_string (element->radio_interface),
                     (-0.5) * ((gdouble)element->ecio));
        }
    }

    /* IO... */
    if (qmi_message_nas_get_signal_strength_output_get_io (output, &io, NULL)) {
        g_print ("IO: '%d dBm'\n", io);
    }

    /* SINR level */
    if (qmi_message_nas_get_signal_strength_output_get_sinr (output, &sinr_level, NULL)) {
        gdouble db_sinr = 0.0;

        if (get_db_from_sinr_level (sinr_level, &db_sinr))
            g_print ("SINR (%u): '%.1lf dB'\n", sinr_level, db_sinr);
        else
            g_print ("SINR (%u): N/A'\n", sinr_level);
    }

    /* RSRQ */
    if (qmi_message_nas_get_signal_strength_output_get_rsrq (output, &rsrq, &radio_interface, NULL)) {
        g_print ("RSRQ:\n"
                 "\tNetwork '%s': '%d dB'\n",
                 qmi_nas_radio_interface_get_string (radio_interface),
                 rsrq);
    }

    /* LTE SNR */
    if (qmi_message_nas_get_signal_strength_output_get_lte_snr (output, &snr, NULL)) {
        g_print ("SNR:\n"
                 "\tNetwork '%s': '%.1lf dB'\n",
                 qmi_nas_radio_interface_get_string (QMI_NAS_RADIO_INTERFACE_LTE),
                 (0.1) * ((gdouble)snr));
    }

    /* LTE RSRP */
    if (qmi_message_nas_get_signal_strength_output_get_lte_rsrp (output, &rsrp, NULL)) {
        g_print ("RSRP:\n"
                 "\tNetwork '%s': '%d dBm'\n",
                 qmi_nas_radio_interface_get_string (QMI_NAS_RADIO_INTERFACE_LTE),
                 rsrp);
    }

    /* Just skip others for now */

    qmi_message_nas_get_signal_strength_output_unref (output);
    operation_shutdown (TRUE);
}

static void
get_tx_rx_info_ready (QmiClientNas *client,
                      GAsyncResult *res,
                      gpointer user_data)
{
    QmiNasRadioInterface interface;
    QmiMessageNasGetTxRxInfoOutput *output;
    GError *error = NULL;
    gboolean is_radio_tuned;
    gboolean is_in_traffic;
    gint32 power;
    gint32 ecio;
    gint32 rscp;
    gint32 rsrp;
    guint32 phase;

    interface = GPOINTER_TO_UINT (user_data);

    output = qmi_client_nas_get_tx_rx_info_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_nas_get_tx_rx_info_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get TX/RX info: %s\n", error->message);
        g_error_free (error);
        qmi_message_nas_get_tx_rx_info_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] Successfully got TX/RX info\n",
             qmi_device_get_path_display (ctx->device));

    /* RX Channel 0 */
    if (qmi_message_nas_get_tx_rx_info_output_get_rx_chain_0_info (
            output,
            &is_radio_tuned,
            &power,
            &ecio,
            &rscp,
            &rsrp,
            &phase,
            NULL)) {
        g_print ("RX Chain 0:\n"
                 "\tRadio tuned: '%s'\n"
                 "\tPower: '%.1lf dBm'\n",
                 is_radio_tuned ? "yes" : "no",
                 (0.1) * ((gdouble)power));
        if (interface == QMI_NAS_RADIO_INTERFACE_CDMA_1X ||
            interface == QMI_NAS_RADIO_INTERFACE_CDMA_1XEVDO ||
            interface == QMI_NAS_RADIO_INTERFACE_GSM ||
            interface == QMI_NAS_RADIO_INTERFACE_UMTS ||
            interface == QMI_NAS_RADIO_INTERFACE_LTE)
            g_print ("\tECIO: '%.1lf dB'\n", (0.1) * ((gdouble)ecio));

        if (interface == QMI_NAS_RADIO_INTERFACE_UMTS)
            g_print ("\tRSCP: '%.1lf dBm'\n", (0.1) * ((gdouble)rscp));

        if (interface == QMI_NAS_RADIO_INTERFACE_LTE)
            g_print ("\tRSRP: '%.1lf dBm'\n", (0.1) * ((gdouble)rsrp));

        if (interface == QMI_NAS_RADIO_INTERFACE_LTE) {
            if (phase == 0xFFFFFFFF)
                g_print ("\tPhase: 'unknown'\n");
            else
                g_print ("\tPhase: '%.2lf degrees'\n", (0.01) * ((gdouble)phase));
        }
    }

    /* RX Channel 1 */
    if (qmi_message_nas_get_tx_rx_info_output_get_rx_chain_1_info (
            output,
            &is_radio_tuned,
            &power,
            &ecio,
            &rscp,
            &rsrp,
            &phase,
            NULL)) {
        g_print ("RX Chain 1:\n"
                 "\tRadio tuned: '%s'\n"
                 "\tPower: '%.1lf dBm'\n",
                 is_radio_tuned ? "yes" : "no",
                 (0.1) * ((gdouble)power));
        if (interface == QMI_NAS_RADIO_INTERFACE_CDMA_1X ||
            interface == QMI_NAS_RADIO_INTERFACE_CDMA_1XEVDO ||
            interface == QMI_NAS_RADIO_INTERFACE_GSM ||
            interface == QMI_NAS_RADIO_INTERFACE_UMTS ||
            interface == QMI_NAS_RADIO_INTERFACE_LTE)
            g_print ("\tECIO: '%.1lf dB'\n", (0.1) * ((gdouble)ecio));

        if (interface == QMI_NAS_RADIO_INTERFACE_UMTS)
            g_print ("\tRSCP: '%.1lf dBm'\n", (0.1) * ((gdouble)rscp));

        if (interface == QMI_NAS_RADIO_INTERFACE_LTE)
            g_print ("\tRSRP: '%.1lf dBm'\n", (0.1) * ((gdouble)rsrp));

        if (interface == QMI_NAS_RADIO_INTERFACE_LTE) {
            if (phase == 0xFFFFFFFF)
                g_print ("\tPhase: 'unknown'\n");
            else
                g_print ("\tPhase: '%.2lf degrees'\n", (0.01) * ((gdouble)phase));
        }
    }

    /* TX Channel */
    if (qmi_message_nas_get_tx_rx_info_output_get_tx_info (
            output,
            &is_in_traffic,
            &power,
            NULL)) {
        g_print ("TX:\n");
        if (is_in_traffic)
            g_print ("\tIn traffic: 'yes'\n"
                     "\tPower: '%.1lf dBm'\n",
                     (0.1) * ((gdouble)power));
        else
            g_print ("\tIn traffic: 'no'\n");
    }

    qmi_message_nas_get_tx_rx_info_output_unref (output);
    operation_shutdown (TRUE);
}

static QmiMessageNasGetTxRxInfoInput *
get_tx_rx_info_input_create (const gchar *str,
                             QmiNasRadioInterface *interface)
{
    QmiMessageNasGetTxRxInfoInput *input = NULL;

    g_assert (interface != NULL);

    if (qmicli_read_nas_radio_interface_from_string (str, interface)) {
        GError *error = NULL;

        input = qmi_message_nas_get_tx_rx_info_input_new ();
        if (!qmi_message_nas_get_tx_rx_info_input_set_radio_interface (
                input,
                *interface,
                &error)) {
            g_printerr ("error: couldn't create input data bundle: '%s'\n",
                        error->message);
            g_error_free (error);
            qmi_message_nas_get_tx_rx_info_input_unref (input);
            input = NULL;
        }
    }

    return input;
}

static void
get_home_network_ready (QmiClientNas *client,
                        GAsyncResult *res)
{
    QmiMessageNasGetHomeNetworkOutput *output;
    GError *error = NULL;

    output = qmi_client_nas_get_home_network_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_nas_get_home_network_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get home network: %s\n", error->message);
        g_error_free (error);
        qmi_message_nas_get_home_network_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] Successfully got home network:\n",
             qmi_device_get_path_display (ctx->device));

    {
        guint16 mcc;
        guint16 mnc;
        const gchar *description;

        qmi_message_nas_get_home_network_output_get_home_network (
            output,
            &mcc,
            &mnc,
            &description,
            NULL);

        g_print ("\tHome network:\n"
                 "\t\tMCC: '%" G_GUINT16_FORMAT"'\n"
                 "\t\tMNC: '%" G_GUINT16_FORMAT"'\n"
                 "\t\tDescription: '%s'\n",
                 mcc,
                 mnc,
                 description);
    }

    {
        guint16 sid;
        guint16 nid;

        if (qmi_message_nas_get_home_network_output_get_home_system_id (
                output,
                &sid,
                &nid,
                NULL)) {
            g_print ("\t\tSID: '%" G_GUINT16_FORMAT"'\n"
                     "\t\tNID: '%" G_GUINT16_FORMAT"'\n",
                     sid,
                     nid);
        }
    }

    {
        guint16 mcc;
        guint16 mnc;

        if (qmi_message_nas_get_home_network_output_get_home_network_3gpp2 (
                output,
                &mcc,
                &mnc,
                NULL, /* display_description */
                NULL, /* description_encoding */
                NULL, /* description */
                NULL)) {
            g_print ("\t3GPP2 Home network (extended):\n"
                     "\t\tMCC: '%" G_GUINT16_FORMAT"'\n"
                     "\t\tMNC: '%" G_GUINT16_FORMAT"'\n",
                     mcc,
                     mnc);

            /* TODO: convert description to UTF-8 and display */
        }
    }

    qmi_message_nas_get_home_network_output_unref (output);
    operation_shutdown (TRUE);
}

static void
get_serving_system_ready (QmiClientNas *client,
                          GAsyncResult *res)
{
    QmiMessageNasGetServingSystemOutput *output;
    GError *error = NULL;

    output = qmi_client_nas_get_serving_system_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_nas_get_serving_system_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get serving system: %s\n", error->message);
        g_error_free (error);
        qmi_message_nas_get_serving_system_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] Successfully got serving system:\n",
             qmi_device_get_path_display (ctx->device));

    {
        QmiNasRegistrationState registration_state;
        QmiNasAttachState cs_attach_state;
        QmiNasAttachState ps_attach_state;
        QmiNasNetworkType selected_network;
        GArray *radio_interfaces;
        guint i;

        qmi_message_nas_get_serving_system_output_get_serving_system (
            output,
            &registration_state,
            &cs_attach_state,
            &ps_attach_state,
            &selected_network,
            &radio_interfaces,
            NULL);

        g_print ("\tRegistration state: '%s'\n"
                 "\tCS: '%s'\n"
                 "\tPS: '%s'\n"
                 "\tSelected network: '%s'\n"
                 "\tRadio interfaces: '%u'\n",
                 qmi_nas_registration_state_get_string (registration_state),
                 qmi_nas_attach_state_get_string (cs_attach_state),
                 qmi_nas_attach_state_get_string (ps_attach_state),
                 qmi_nas_network_type_get_string (selected_network),
                 radio_interfaces->len);

        for (i = 0; i < radio_interfaces->len; i++) {
            QmiNasRadioInterface iface;

            iface = g_array_index (radio_interfaces, QmiNasRadioInterface, i);
            g_print ("\t\t[%u]: '%s'\n", i, qmi_nas_radio_interface_get_string (iface));
        }
    }

    {
        QmiNasRoamingIndicatorStatus roaming;

        if (qmi_message_nas_get_serving_system_output_get_roaming_indicator (
                output,
                &roaming,
                NULL)) {
            g_print ("\tRoaming status: '%s'\n",
                     qmi_nas_roaming_indicator_status_get_string (roaming));
        }
    }

    {
        GArray *data_service_capability;

        if (qmi_message_nas_get_serving_system_output_get_data_service_capability (
                output,
                &data_service_capability,
                NULL)) {
            guint i;

            g_print ("\tData service capabilities: '%u'\n",
                     data_service_capability->len);

            for (i = 0; i < data_service_capability->len; i++) {
                QmiNasDataCapability cap;

                cap = g_array_index (data_service_capability, QmiNasDataCapability, i);
                g_print ("\t\t[%u]: '%s'\n", i, qmi_nas_data_capability_get_string (cap));
            }
        }
    }

    {
        guint16 current_plmn_mcc;
        guint16 current_plmn_mnc;
        const gchar *current_plmn_description;

        if (qmi_message_nas_get_serving_system_output_get_current_plmn (
                output,
                &current_plmn_mcc,
                &current_plmn_mnc,
                &current_plmn_description,
                NULL)) {
            g_print ("\tCurrent PLMN:\n"
                     "\t\tMCC: '%" G_GUINT16_FORMAT"'\n"
                     "\t\tMNC: '%" G_GUINT16_FORMAT"'\n"
                     "\t\tDescription: '%s'\n",
                     current_plmn_mcc,
                     current_plmn_mnc,
                     current_plmn_description);
        }
    }

    {
        guint16 sid;
        guint16 nid;

        if (qmi_message_nas_get_serving_system_output_get_cdma_system_id (
                output,
                &sid,
                &nid,
                NULL)) {
            g_print ("\tCDMA System ID:\n"
                     "\t\tSID: '%" G_GUINT16_FORMAT"'\n"
                     "\t\tNID: '%" G_GUINT16_FORMAT"'\n",
                     sid, nid);
        }
    }

    {
        guint16 id;
        gint32 latitude;
        gint32 longitude;

        if (qmi_message_nas_get_serving_system_output_get_cdma_base_station_info (
                output,
                &id,
                &latitude,
                &longitude,
                NULL)) {
            gdouble latitude_degrees;
            gdouble longitude_degrees;

            /* TODO: give degrees, minutes, seconds */
            latitude_degrees = ((gdouble)latitude * 0.25)/3600.0;
            longitude_degrees = ((gdouble)longitude * 0.25)/3600.0;

            g_print ("\tCDMA Base station info:\n"
                     "\t\tBase station ID: '%" G_GUINT16_FORMAT"'\n"
                     "\t\tLatitude: '%lf'º\n"
                     "\t\tLongitude: '%lf'º\n",
                     id, latitude_degrees, longitude_degrees);
        }
    }

    {
        GArray *roaming_indicators;

        if (qmi_message_nas_get_serving_system_output_get_roaming_indicator_list (
                output,
                &roaming_indicators,
                NULL)) {
            guint i;

            g_print ("\tRoaming indicators: '%u'\n",
                     roaming_indicators->len);

            for (i = 0; i < roaming_indicators->len; i++) {
                QmiMessageNasGetServingSystemOutputRoamingIndicatorListElement *element;

                element = &g_array_index (roaming_indicators, QmiMessageNasGetServingSystemOutputRoamingIndicatorListElement, i);
                g_print ("\t\t[%u]: '%s' (%s)\n",
                         i,
                         qmi_nas_roaming_indicator_status_get_string (element->roaming_indicator),
                         qmi_nas_radio_interface_get_string (element->radio_interface));
            }
        }
    }

    {
        QmiNasRoamingIndicatorStatus roaming;

        if (qmi_message_nas_get_serving_system_output_get_default_roaming_indicator (
                output,
                &roaming,
                NULL)) {
            g_print ("\tDefault roaming status: '%s'\n",
                     qmi_nas_roaming_indicator_status_get_string (roaming));
        }
    }

    {
        guint8 leap_seconds;
        gint8 local_time_offset;
        gboolean daylight_saving_time;

        if (qmi_message_nas_get_serving_system_output_get_time_zone_3gpp2 (
                output,
                &leap_seconds,
                &local_time_offset,
                &daylight_saving_time,
                NULL)) {
            g_print ("\t3GPP2 time zone:\n"
                     "\t\tLeap seconds: '%u' seconds\n"
                     "\t\tLocal time offset: '%d' minutes\n"
                     "\t\tDaylight saving time: '%s'\n",
                     leap_seconds,
                     (gint)local_time_offset * 30,
                     daylight_saving_time ? "yes" : "no");
        }
    }

    {
        guint8 cdma_p_rev;

        if (qmi_message_nas_get_serving_system_output_get_cdma_p_rev (
                output,
                &cdma_p_rev,
                NULL)) {
            g_print ("\tCDMA P_Rev: '%u'\n", cdma_p_rev);
        }
    }

    {
        gint8 time_zone;

        if (qmi_message_nas_get_serving_system_output_get_time_zone_3gpp (
                output,
                &time_zone,
                NULL)) {
            g_print ("\t3GPP time zone offset: '%d' minutes\n",
                     (gint)time_zone * 15);
        }
    }

    {
        guint8 adjustment;

        if (qmi_message_nas_get_serving_system_output_get_daylight_saving_time_adjustment_3gpp (
                output,
                &adjustment,
                NULL)) {
            g_print ("\t3GPP daylight saving time adjustment: '%u' hours\n",
                     adjustment);
        }
    }

    {
        guint16 lac;

        if (qmi_message_nas_get_serving_system_output_get_lac_3gpp (
                output,
                &lac,
                NULL)) {
            g_print ("\t3GPP location area code: '%" G_GUINT16_FORMAT"'\n", lac);
        }
    }

    {
        guint32 cid;

        if (qmi_message_nas_get_serving_system_output_get_cid_3gpp (
                output,
                &cid,
                NULL)) {
            g_print ("\t3GPP cell ID: '%u'\n", cid);
        }
    }

    {
        gboolean concurrent;

        if (qmi_message_nas_get_serving_system_output_get_concurrent_service_info_3gpp2 (
                output,
                &concurrent,
                NULL)) {
            g_print ("\t3GPP2 concurrent service info: '%s'\n",
                     concurrent ? "available" : "not available");
        }
    }

    {
        gboolean prl;

        if (qmi_message_nas_get_serving_system_output_get_prl_indicator_3gpp2 (
                output,
                &prl,
                NULL)) {
            g_print ("\t3GPP2 PRL indicator: '%s'\n",
                     prl ? "system in PRL" : "system not in PRL");
        }
    }

    {
        gboolean supported;

        if (qmi_message_nas_get_serving_system_output_get_dtm_support (
                output,
                &supported,
                NULL)) {
            g_print ("\tDual transfer mode: '%s'\n",
                     supported ? "supported" : "not supported");
        }
    }

    {
        QmiNasServiceStatus status;
        QmiNasNetworkServiceDomain capability;
        QmiNasServiceStatus hdr_status;
        gboolean hdr_hybrid;
        gboolean forbidden;

        if (qmi_message_nas_get_serving_system_output_get_detailed_service_status (
                output,
                &status,
                &capability,
                &hdr_status,
                &hdr_hybrid,
                &forbidden,
                NULL)) {
            g_print ("\tDetailed status:\n"
                     "\t\tStatus: '%s'\n"
                     "\t\tCapability: '%s'\n"
                     "\t\tHDR Status: '%s'\n"
                     "\t\tHDR Hybrid: '%s'\n"
                     "\t\tForbidden: '%s'\n",
                     qmi_nas_service_status_get_string (status),
                     qmi_nas_network_service_domain_get_string (capability),
                     qmi_nas_service_status_get_string (hdr_status),
                     hdr_hybrid ? "yes" : "no",
                     forbidden ? "yes" : "no");
        }
    }

    {
        guint16 mcc;
        guint8 imsi_11_12;

        if (qmi_message_nas_get_serving_system_output_get_cdma_system_info (
                output,
                &mcc,
                &imsi_11_12,
                NULL)) {
            g_print ("\tCDMA system info:\n"
                     "\t\tMCC: '%" G_GUINT16_FORMAT"'\n"
                     "\t\tIMSI_11_12: '%u'\n",
                     mcc,
                     imsi_11_12);
        }
    }

    {
        QmiNasHdrPersonality personality;

        if (qmi_message_nas_get_serving_system_output_get_hdr_personality (
                output,
                &personality,
                NULL)) {
            g_print ("\tHDR personality: '%s'\n",
                     qmi_nas_hdr_personality_get_string (personality));
        }
    }

    {
        guint16 tac;

        if (qmi_message_nas_get_serving_system_output_get_lte_tac (
                output,
                &tac,
                NULL)) {
            g_print ("\tLTE tracking area code: '%" G_GUINT16_FORMAT"'\n", tac);
        }
    }

    {
        QmiNasCallBarringStatus cs_status;
        QmiNasCallBarringStatus ps_status;

        if (qmi_message_nas_get_serving_system_output_get_call_barring_status (
                output,
                &cs_status,
                &ps_status,
                NULL)) {
            g_print ("\tCall barring status:\n"
                     "\t\tCircuit switched: '%s'\n"
                     "\t\tPacket switched: '%s'\n",
                     qmi_nas_call_barring_status_get_string (cs_status),
                     qmi_nas_call_barring_status_get_string (ps_status));
        }
    }

    {
        guint16 code;

        if (qmi_message_nas_get_serving_system_output_get_umts_primary_scrambling_code (
                output,
                &code,
                NULL)) {
            g_print ("\tUMTS primary scrambling code: '%" G_GUINT16_FORMAT"'\n", code);
        }
    }

    {
        guint16 mcc;
        guint16 mnc;
        gboolean has_pcs_digit;

        if (qmi_message_nas_get_serving_system_output_get_mnc_pcs_digit_include_status (
                output,
                &mcc,
                &mnc,
                &has_pcs_digit,
                NULL)) {
            g_print ("\tFull operator code info:\n"
                     "\t\tMCC: '%" G_GUINT16_FORMAT"'\n"
                     "\t\tMNC: '%" G_GUINT16_FORMAT"'\n"
                     "\t\tMNC with PCS digit: '%s'\n",
                     mcc,
                     mnc,
                     has_pcs_digit ? "yes" : "no");
        }
    }

    qmi_message_nas_get_serving_system_output_unref (output);
    operation_shutdown (TRUE);
}

static void
get_system_info_ready (QmiClientNas *client,
                       GAsyncResult *res)
{
    QmiMessageNasGetSystemInfoOutput *output;
    GError *error = NULL;

    output = qmi_client_nas_get_system_info_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_nas_get_system_info_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get system info: %s\n", error->message);
        g_error_free (error);
        qmi_message_nas_get_system_info_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] Successfully got system info:\n",
             qmi_device_get_path_display (ctx->device));

    /* CDMA 1x */
    {
        QmiNasServiceStatus service_status;
        gboolean preferred_data_path;
        gboolean domain_valid;
        QmiNasNetworkServiceDomain domain;
        gboolean service_capability_valid;
        QmiNasNetworkServiceDomain service_capability;
        gboolean roaming_status_valid;
        QmiNasRoamingStatus roaming_status;
        gboolean forbidden_valid;
        gboolean forbidden;
        gboolean prl_match_valid;
        gboolean prl_match;
        gboolean p_rev_valid;
        guint8 p_rev;
        gboolean base_station_p_rev_valid;
        guint8 base_station_p_rev;
        gboolean concurrent_service_support_valid;
        gboolean concurrent_service_support;
        gboolean cdma_system_id_valid;
        guint16 sid;
        guint16 nid;
        gboolean base_station_info_valid;
        guint16 base_station_id;
        gint32 base_station_latitude;
        gint32 base_station_longitude;
        gboolean packet_zone_valid;
        guint16 packet_zone;
        gboolean network_id_valid;
        const gchar *mcc;
        const gchar *mnc;
        guint16 geo_system_index;
        guint16 registration_period;

        if (qmi_message_nas_get_system_info_output_get_cdma_service_status (
                output,
                &service_status,
                &preferred_data_path,
                NULL)) {
            g_print ("\tCDMA 1x service:\n"
                     "\t\tStatus: '%s'\n"
                     "\t\tPreferred data path: '%s'\n",
                     qmi_nas_service_status_get_string (service_status),
                     preferred_data_path ? "yes" : "no");

            if (qmi_message_nas_get_system_info_output_get_cdma_system_info (
                    output,
                    &domain_valid, &domain,
                    &service_capability_valid, &service_capability,
                    &roaming_status_valid, &roaming_status,
                    &forbidden_valid, &forbidden,
                    &prl_match_valid, &prl_match,
                    &p_rev_valid, &p_rev,
                    &base_station_p_rev_valid, &base_station_p_rev,
                    &concurrent_service_support_valid, &concurrent_service_support,
                    &cdma_system_id_valid, &sid, &nid,
                    &base_station_info_valid, &base_station_id, &base_station_longitude, &base_station_latitude,
                    &packet_zone_valid, &packet_zone,
                    &network_id_valid, &mcc, &mnc,
                    NULL)) {
                if (domain_valid)
                    g_print ("\t\tDomain: '%s'\n", qmi_nas_network_service_domain_get_string (domain));
                if (service_capability_valid)
                    g_print ("\t\tService capability: '%s'\n", qmi_nas_network_service_domain_get_string (service_capability));
                if (roaming_status_valid)
                    g_print ("\t\tRoaming status: '%s'\n", qmi_nas_roaming_status_get_string (roaming_status));
                if (forbidden_valid)
                    g_print ("\t\tForbidden: '%s'\n", forbidden ? "yes" : "no");
                if (prl_match_valid)
                    g_print ("\t\tPRL match: '%s'\n", prl_match ? "yes" : "no");
                if (p_rev_valid)
                    g_print ("\t\tP-Rev: '%u'\n", p_rev);
                if (base_station_p_rev_valid)
                    g_print ("\t\tBase station P-Rev: '%u'\n", base_station_p_rev);
                if (concurrent_service_support_valid)
                    g_print ("\t\tConcurrent service support: '%s'\n", concurrent_service_support ? "yes" : "no");
                if (cdma_system_id_valid) {
                    g_print ("\t\tSID: '%" G_GUINT16_FORMAT"'\n", sid);
                    g_print ("\t\tNID: '%" G_GUINT16_FORMAT"'\n", nid);
                }
                if (base_station_info_valid) {
                    gdouble latitude_degrees;
                    gdouble longitude_degrees;

                    /* TODO: give degrees, minutes, seconds */
                    latitude_degrees = ((gdouble)base_station_latitude * 0.25)/3600.0;
                    longitude_degrees = ((gdouble)base_station_longitude * 0.25)/3600.0;
                    g_print ("\t\tBase station ID: '%" G_GUINT16_FORMAT"'\n", base_station_id);
                    g_print ("\t\tBase station latitude: '%lf'º\n", latitude_degrees);
                    g_print ("\t\tBase station longitude: '%lf'º\n", longitude_degrees);
                }
                if (packet_zone_valid)
                    g_print ("\t\tPacket zone: '%" G_GUINT16_FORMAT "'\n", packet_zone);
                if (network_id_valid) {
                    g_print ("\t\tMCC: '%s'\n", mcc);
                    if ((guchar)mnc[2] == 0xFF)
                        g_print ("\t\tMNC: '%.2s'\n", mnc);
                    else
                        g_print ("\t\tMNC: '%.3s'\n", mnc);
                }
            }

            if (qmi_message_nas_get_system_info_output_get_additional_cdma_system_info (
                    output,
                    &geo_system_index,
                    &registration_period,
                    NULL)) {
                if (geo_system_index != 0xFFFF)
                    g_print ("\t\tGeo system index: '%" G_GUINT16_FORMAT "'\n", geo_system_index);
                if (registration_period != 0xFFFF)
                    g_print ("\t\tRegistration period: '%" G_GUINT16_FORMAT "'\n", registration_period);
            }
        }
    }

    /* CDMA 1xEV-DO */
    {
        QmiNasServiceStatus service_status;
        gboolean preferred_data_path;
        gboolean domain_valid;
        QmiNasNetworkServiceDomain domain;
        gboolean service_capability_valid;
        QmiNasNetworkServiceDomain service_capability;
        gboolean roaming_status_valid;
        QmiNasRoamingStatus roaming_status;
        gboolean forbidden_valid;
        gboolean forbidden;
        gboolean prl_match_valid;
        gboolean prl_match;
        gboolean personality_valid;
        QmiNasHdrPersonality personality;
        gboolean protocol_revision_valid;
        QmiNasHdrProtocolRevision protocol_revision;
        gboolean is_856_system_id_valid;
        const gchar *is_856_system_id;
        guint16 geo_system_index;

        if (qmi_message_nas_get_system_info_output_get_hdr_service_status (
                output,
                &service_status,
                &preferred_data_path,
                NULL)) {
            g_print ("\tCDMA 1xEV-DO (HDR) service:\n"
                     "\t\tStatus: '%s'\n"
                     "\t\tPreferred data path: '%s'\n",
                     qmi_nas_service_status_get_string (service_status),
                     preferred_data_path ? "yes" : "no");

            if (qmi_message_nas_get_system_info_output_get_hdr_system_info (
                    output,
                    &domain_valid, &domain,
                    &service_capability_valid, &service_capability,
                    &roaming_status_valid, &roaming_status,
                    &forbidden_valid, &forbidden,
                    &prl_match_valid, &prl_match,
                    &personality_valid, &personality,
                    &protocol_revision_valid, &protocol_revision,
                    &is_856_system_id_valid, &is_856_system_id,
                    NULL)) {
                if (domain_valid)
                    g_print ("\t\tDomain: '%s'\n", qmi_nas_network_service_domain_get_string (domain));
                if (service_capability_valid)
                    g_print ("\t\tService capability: '%s'\n", qmi_nas_network_service_domain_get_string (service_capability));
                if (roaming_status_valid)
                    g_print ("\t\tRoaming status: '%s'\n", qmi_nas_roaming_status_get_string (roaming_status));
                if (forbidden_valid)
                    g_print ("\t\tForbidden: '%s'\n", forbidden ? "yes" : "no");
                if (prl_match_valid)
                    g_print ("\t\tPRL match: '%s'\n", prl_match ? "yes" : "no");
                if (personality_valid)
                    g_print ("\t\tPersonality: '%s'\n", qmi_nas_hdr_personality_get_string (personality));
                if (protocol_revision_valid)
                    g_print ("\t\tProtocol revision: '%s'\n", qmi_nas_hdr_protocol_revision_get_string (protocol_revision));
                if (is_856_system_id_valid)
                    g_print ("\t\tIS-856 system ID: '%s'\n", is_856_system_id);
            }

            if (qmi_message_nas_get_system_info_output_get_additional_hdr_system_info (
                    output,
                    &geo_system_index,
                    NULL)) {
                if (geo_system_index != 0xFFFF)
                    g_print ("\t\tGeo system index: '%" G_GUINT16_FORMAT "'\n", geo_system_index);
            }
        }
    }

    /* GSM */
    {
        QmiNasServiceStatus service_status;
        QmiNasServiceStatus true_service_status;
        gboolean preferred_data_path;
        gboolean domain_valid;
        QmiNasNetworkServiceDomain domain;
        gboolean service_capability_valid;
        QmiNasNetworkServiceDomain service_capability;
        gboolean roaming_status_valid;
        QmiNasRoamingStatus roaming_status;
        gboolean forbidden_valid;
        gboolean forbidden;
        gboolean lac_valid;
        guint16 lac;
        gboolean cid_valid;
        guint32 cid;
        gboolean registration_reject_info_valid;
        QmiNasNetworkServiceDomain registration_reject_domain;
        guint8 registration_reject_cause;
        gboolean network_id_valid;
        const gchar *mcc;
        const gchar *mnc;
        gboolean egprs_support_valid;
        gboolean egprs_support;
        gboolean dtm_support_valid;
        gboolean dtm_support;
        guint16 geo_system_index;
        QmiNasCellBroadcastCapability cell_broadcast_support;
        QmiNasCallBarringStatus call_barring_status_cs;
        QmiNasCallBarringStatus call_barring_status_ps;
        QmiNasNetworkServiceDomain cipher_domain;

        if (qmi_message_nas_get_system_info_output_get_gsm_service_status (
                output,
                &service_status,
                &true_service_status,
                &preferred_data_path,
                NULL)) {
            g_print ("\tGSM service:\n"
                     "\t\tStatus: '%s'\n"
                     "\t\tTrue Status: '%s'\n"
                     "\t\tPreferred data path: '%s'\n",
                     qmi_nas_service_status_get_string (service_status),
                     qmi_nas_service_status_get_string (true_service_status),
                     preferred_data_path ? "yes" : "no");

            if (qmi_message_nas_get_system_info_output_get_gsm_system_info (
                    output,
                    &domain_valid, &domain,
                    &service_capability_valid, &service_capability,
                    &roaming_status_valid, &roaming_status,
                    &forbidden_valid, &forbidden,
                    &lac_valid, &lac,
                    &cid_valid, &cid,
                    &registration_reject_info_valid, &registration_reject_domain, &registration_reject_cause,
                    &network_id_valid, &mcc, &mnc,
                    &egprs_support_valid, &egprs_support,
                    &dtm_support_valid, &dtm_support,
                    NULL)) {
                if (domain_valid)
                    g_print ("\t\tDomain: '%s'\n", qmi_nas_network_service_domain_get_string (domain));
                if (service_capability_valid)
                    g_print ("\t\tService capability: '%s'\n", qmi_nas_network_service_domain_get_string (service_capability));
                if (roaming_status_valid)
                    g_print ("\t\tRoaming status: '%s'\n", qmi_nas_roaming_status_get_string (roaming_status));
                if (forbidden_valid)
                    g_print ("\t\tForbidden: '%s'\n", forbidden ? "yes" : "no");
                if (lac_valid)
                    g_print ("\t\tLocation Area Code: '%" G_GUINT16_FORMAT"'\n", lac);
                if (cid_valid)
                    g_print ("\t\tCell ID: '%u'\n", cid);
                if (registration_reject_info_valid)
                    g_print ("\t\tRegistration reject: '%s' (%u)\n",
                             qmi_nas_network_service_domain_get_string (registration_reject_domain),
                             registration_reject_cause);
                if (network_id_valid) {
                    g_print ("\t\tMCC: '%s'\n", mcc);
                    if ((guchar)mnc[2] == 0xFF)
                        g_print ("\t\tMNC: '%.2s'\n", mnc);
                    else
                        g_print ("\t\tMNC: '%.3s'\n", mnc);
                }
                if (egprs_support_valid)
                    g_print ("\t\tE-GPRS supported: '%s'\n", egprs_support ? "yes" : "no");
                if (dtm_support_valid)
                    g_print ("\t\tDual Transfer Mode supported: '%s'\n", dtm_support ? "yes" : "no");
            }

            if (qmi_message_nas_get_system_info_output_get_additional_gsm_system_info (
                    output,
                    &geo_system_index,
                    &cell_broadcast_support,
                    NULL)) {
                if (geo_system_index != 0xFFFF)
                    g_print ("\t\tGeo system index: '%" G_GUINT16_FORMAT "'\n", geo_system_index);
                g_print ("\t\tCell broadcast support: '%s'\n", qmi_nas_cell_broadcast_capability_get_string (cell_broadcast_support));
            }

            if (qmi_message_nas_get_system_info_output_get_gsm_call_barring_status (
                    output,
                    &call_barring_status_cs,
                    &call_barring_status_ps,
                    NULL)) {
                g_print ("\t\tCall barring status (CS): '%s'\n", qmi_nas_call_barring_status_get_string (call_barring_status_cs));
                g_print ("\t\tCall barring status (PS): '%s'\n", qmi_nas_call_barring_status_get_string (call_barring_status_ps));
            }

            if (qmi_message_nas_get_system_info_output_get_gsm_cipher_domain (
                    output,
                    &cipher_domain,
                    NULL)) {
                g_print ("\t\tCipher Domain: '%s'\n", qmi_nas_network_service_domain_get_string (cipher_domain));
            }
        }
    }

    /* WCDMA */
    {
        QmiNasServiceStatus service_status;
        QmiNasServiceStatus true_service_status;
        gboolean preferred_data_path;
        gboolean domain_valid;
        QmiNasNetworkServiceDomain domain;
        gboolean service_capability_valid;
        QmiNasNetworkServiceDomain service_capability;
        gboolean roaming_status_valid;
        QmiNasRoamingStatus roaming_status;
        gboolean forbidden_valid;
        gboolean forbidden;
        gboolean lac_valid;
        guint16 lac;
        gboolean cid_valid;
        guint32 cid;
        gboolean registration_reject_info_valid;
        QmiNasNetworkServiceDomain registration_reject_domain;
        guint8 registration_reject_cause;
        gboolean network_id_valid;
        const gchar *mcc;
        const gchar *mnc;
        gboolean hs_call_status_valid;
        QmiNasWcdmaHsService hs_call_status;
        gboolean hs_service_valid;
        QmiNasWcdmaHsService hs_service;
        gboolean primary_scrambling_code_valid;
        guint16 primary_scrambling_code;
        guint16 geo_system_index;
        QmiNasCellBroadcastCapability cell_broadcast_support;
        QmiNasCallBarringStatus call_barring_status_cs;
        QmiNasCallBarringStatus call_barring_status_ps;
        QmiNasNetworkServiceDomain cipher_domain;

        if (qmi_message_nas_get_system_info_output_get_wcdma_service_status (
                output,
                &service_status,
                &true_service_status,
                &preferred_data_path,
                NULL)) {
            g_print ("\tWCDMA service:\n"
                     "\t\tStatus: '%s'\n"
                     "\t\tTrue Status: '%s'\n"
                     "\t\tPreferred data path: '%s'\n",
                     qmi_nas_service_status_get_string (service_status),
                     qmi_nas_service_status_get_string (true_service_status),
                     preferred_data_path ? "yes" : "no");

            if (qmi_message_nas_get_system_info_output_get_wcdma_system_info (
                    output,
                    &domain_valid, &domain,
                    &service_capability_valid, &service_capability,
                    &roaming_status_valid, &roaming_status,
                    &forbidden_valid, &forbidden,
                    &lac_valid, &lac,
                    &cid_valid, &cid,
                    &registration_reject_info_valid, &registration_reject_domain, &registration_reject_cause,
                    &network_id_valid, &mcc, &mnc,
                    &hs_call_status_valid, &hs_call_status,
                    &hs_service_valid, &hs_service,
                    &primary_scrambling_code_valid, &primary_scrambling_code,
                NULL)) {
                if (domain_valid)
                    g_print ("\t\tDomain: '%s'\n", qmi_nas_network_service_domain_get_string (domain));
                if (service_capability_valid)
                    g_print ("\t\tService capability: '%s'\n", qmi_nas_network_service_domain_get_string (service_capability));
                if (roaming_status_valid)
                    g_print ("\t\tRoaming status: '%s'\n", qmi_nas_roaming_status_get_string (roaming_status));
                if (forbidden_valid)
                    g_print ("\t\tForbidden: '%s'\n", forbidden ? "yes" : "no");
                if (lac_valid)
                    g_print ("\t\tLocation Area Code: '%" G_GUINT16_FORMAT"'\n", lac);
                if (cid_valid)
                    g_print ("\t\tCell ID: '%u'\n", cid);
                if (registration_reject_info_valid)
                    g_print ("\t\tRegistration reject: '%s' (%u)\n",
                             qmi_nas_network_service_domain_get_string (registration_reject_domain),
                             registration_reject_cause);
                if (network_id_valid) {
                    g_print ("\t\tMCC: '%s'\n", mcc);
                    if ((guchar)mnc[2] == 0xFF)
                        g_print ("\t\tMNC: '%.2s'\n", mnc);
                    else
                        g_print ("\t\tMNC: '%.3s'\n", mnc);
                }
                if (hs_call_status_valid)
                    g_print ("\t\tHS call status: '%s'\n", qmi_nas_wcdma_hs_service_get_string (hs_call_status));
                if (hs_service_valid)
                    g_print ("\t\tHS service: '%s'\n", qmi_nas_wcdma_hs_service_get_string (hs_service));
                if (primary_scrambling_code_valid)
                    g_print ("\t\tPrimary scrambling code: '%" G_GUINT16_FORMAT"'\n", primary_scrambling_code);
            }

            if (qmi_message_nas_get_system_info_output_get_additional_wcdma_system_info (
                    output,
                    &geo_system_index,
                    &cell_broadcast_support,
                    NULL)) {
                if (geo_system_index != 0xFFFF)
                    g_print ("\t\tGeo system index: '%" G_GUINT16_FORMAT "'\n", geo_system_index);
                g_print ("\t\tCell broadcast support: '%s'\n", qmi_nas_cell_broadcast_capability_get_string (cell_broadcast_support));
            }

            if (qmi_message_nas_get_system_info_output_get_wcdma_call_barring_status (
                    output,
                    &call_barring_status_cs,
                    &call_barring_status_ps,
                    NULL)) {
                g_print ("\t\tCall barring status (CS): '%s'\n", qmi_nas_call_barring_status_get_string (call_barring_status_cs));
                g_print ("\t\tCall barring status (PS): '%s'\n", qmi_nas_call_barring_status_get_string (call_barring_status_ps));
            }

            if (qmi_message_nas_get_system_info_output_get_wcdma_cipher_domain (
                    output,
                    &cipher_domain,
                    NULL)) {
                g_print ("\t\tCipher Domain: '%s'\n", qmi_nas_network_service_domain_get_string (cipher_domain));
            }
        }
    }

    /* LTE */
    {
        QmiNasServiceStatus service_status;
        QmiNasServiceStatus true_service_status;
        gboolean preferred_data_path;
        gboolean domain_valid;
        QmiNasNetworkServiceDomain domain;
        gboolean service_capability_valid;
        QmiNasNetworkServiceDomain service_capability;
        gboolean roaming_status_valid;
        QmiNasRoamingStatus roaming_status;
        gboolean forbidden_valid;
        gboolean forbidden;
        gboolean lac_valid;
        guint16 lac;
        gboolean cid_valid;
        guint32 cid;
        gboolean registration_reject_info_valid;
        QmiNasNetworkServiceDomain registration_reject_domain;
        guint8 registration_reject_cause;
        gboolean network_id_valid;
        const gchar *mcc;
        const gchar *mnc;
        gboolean tac_valid;
        guint16 tac;
        guint16 geo_system_index;
        gboolean voice_support;
        gboolean embms_coverage_info_support;

        if (qmi_message_nas_get_system_info_output_get_lte_service_status (
                output,
                &service_status,
                &true_service_status,
                &preferred_data_path,
                NULL)) {
            g_print ("\tLTE service:\n"
                     "\t\tStatus: '%s'\n"
                     "\t\tTrue Status: '%s'\n"
                     "\t\tPreferred data path: '%s'\n",
                     qmi_nas_service_status_get_string (service_status),
                     qmi_nas_service_status_get_string (true_service_status),
                     preferred_data_path ? "yes" : "no");

            if (qmi_message_nas_get_system_info_output_get_lte_system_info (
                    output,
                    &domain_valid, &domain,
                    &service_capability_valid, &service_capability,
                    &roaming_status_valid, &roaming_status,
                    &forbidden_valid, &forbidden,
                    &lac_valid, &lac,
                    &cid_valid, &cid,
                    &registration_reject_info_valid,&registration_reject_domain,&registration_reject_cause,
                    &network_id_valid, &mcc, &mnc,
                    &tac_valid, &tac,
                    NULL)) {
                if (domain_valid)
                    g_print ("\t\tDomain: '%s'\n", qmi_nas_network_service_domain_get_string (domain));
                if (service_capability_valid)
                    g_print ("\t\tService capability: '%s'\n", qmi_nas_network_service_domain_get_string (service_capability));
                if (roaming_status_valid)
                    g_print ("\t\tRoaming status: '%s'\n", qmi_nas_roaming_status_get_string (roaming_status));
                if (forbidden_valid)
                    g_print ("\t\tForbidden: '%s'\n", forbidden ? "yes" : "no");
                if (lac_valid)
                    g_print ("\t\tLocation Area Code: '%" G_GUINT16_FORMAT"'\n", lac);
                if (cid_valid)
                    g_print ("\t\tCell ID: '%u'\n", cid);
                if (registration_reject_info_valid)
                    g_print ("\t\tRegistration reject: '%s' (%u)\n",
                             qmi_nas_network_service_domain_get_string (registration_reject_domain),
                             registration_reject_cause);
                if (network_id_valid) {
                    g_print ("\t\tMCC: '%s'\n", mcc);
                    if ((guchar)mnc[2] == 0xFF)
                        g_print ("\t\tMNC: '%.2s'\n", mnc);
                    else
                        g_print ("\t\tMNC: '%.3s'\n", mnc);
                }
                if (tac_valid)
                    g_print ("\t\tTracking Area Code: '%" G_GUINT16_FORMAT"'\n", tac);
            }

            if (qmi_message_nas_get_system_info_output_get_additional_lte_system_info (
                    output,
                    &geo_system_index,
                    NULL)) {
                if (geo_system_index != 0xFFFF)
                    g_print ("\t\tGeo system index: '%" G_GUINT16_FORMAT "'\n", geo_system_index);
            }

            if (qmi_message_nas_get_system_info_output_get_lte_voice_support (
                    output,
                    &voice_support,
                    NULL)) {
                g_print ("\t\tVoice support: '%s'\n", voice_support ? "yes" : "no");
            }

            if (qmi_message_nas_get_system_info_output_get_lte_embms_coverage_info_support (
                    output,
                    &embms_coverage_info_support,
                    NULL)) {
                g_print ("\t\teMBMS coverage info support: '%s'\n", embms_coverage_info_support ? "yes" : "no");
            }
        }
    }

    /* TD-SCDMA */
    {
        QmiNasServiceStatus service_status;
        QmiNasServiceStatus true_service_status;
        gboolean preferred_data_path;
        gboolean domain_valid;
        QmiNasNetworkServiceDomain domain;
        gboolean service_capability_valid;
        QmiNasNetworkServiceDomain service_capability;
        gboolean roaming_status_valid;
        QmiNasRoamingStatus roaming_status;
        gboolean forbidden_valid;
        gboolean forbidden;
        gboolean lac_valid;
        guint16 lac;
        gboolean cid_valid;
        guint32 cid;
        gboolean registration_reject_info_valid;
        QmiNasNetworkServiceDomain registration_reject_domain;
        guint8 registration_reject_cause;
        gboolean network_id_valid;
        const gchar *mcc;
        const gchar *mnc;
        gboolean hs_call_status_valid;
        QmiNasWcdmaHsService hs_call_status;
        gboolean hs_service_valid;
        QmiNasWcdmaHsService hs_service;
        gboolean cell_parameter_id_valid;
        guint16 cell_parameter_id;
        gboolean cell_broadcast_support_valid;
        QmiNasCellBroadcastCapability cell_broadcast_support;
        gboolean call_barring_status_cs_valid;
        QmiNasCallBarringStatus call_barring_status_cs;
        gboolean call_barring_status_ps_valid;
        QmiNasCallBarringStatus call_barring_status_ps;
        gboolean cipher_domain_valid;
        QmiNasNetworkServiceDomain cipher_domain;

        if (qmi_message_nas_get_system_info_output_get_td_scdma_service_status (
                output,
                &service_status,
                &true_service_status,
                &preferred_data_path,
                NULL)) {
            g_print ("\tTD-SCDMA service:\n"
                     "\t\tStatus: '%s'\n"
                     "\t\tTrue Status: '%s'\n"
                     "\t\tPreferred data path: '%s'\n",
                     qmi_nas_service_status_get_string (service_status),
                     qmi_nas_service_status_get_string (true_service_status),
                     preferred_data_path ? "yes" : "no");

            if (qmi_message_nas_get_system_info_output_get_td_scdma_system_info (
                    output,
                    &domain_valid, &domain,
                    &service_capability_valid, &service_capability,
                    &roaming_status_valid, &roaming_status,
                    &forbidden_valid, &forbidden,
                    &lac_valid, &lac,
                    &cid_valid, &cid,
                    &registration_reject_info_valid, &registration_reject_domain, &registration_reject_cause,
                    &network_id_valid, &mcc, &mnc,
                    &hs_call_status_valid, &hs_call_status,
                    &hs_service_valid, &hs_service,
                    &cell_parameter_id_valid, &cell_parameter_id,
                    &cell_broadcast_support_valid, &cell_broadcast_support,
                    &call_barring_status_cs_valid, &call_barring_status_cs,
                    &call_barring_status_ps_valid, &call_barring_status_ps,
                    &cipher_domain_valid, &cipher_domain,
                    NULL)) {
                if (domain_valid)
                    g_print ("\t\tDomain: '%s'\n", qmi_nas_network_service_domain_get_string (domain));
                if (service_capability_valid)
                    g_print ("\t\tService capability: '%s'\n", qmi_nas_network_service_domain_get_string (service_capability));
                if (roaming_status_valid)
                    g_print ("\t\tRoaming status: '%s'\n", qmi_nas_roaming_status_get_string (roaming_status));
                if (forbidden_valid)
                    g_print ("\t\tForbidden: '%s'\n", forbidden ? "yes" : "no");
                if (lac_valid)
                    g_print ("\t\tLocation Area Code: '%" G_GUINT16_FORMAT"'\n", lac);
                if (cid_valid)
                    g_print ("\t\tCell ID: '%u'\n", cid);
                if (registration_reject_info_valid)
                    g_print ("\t\tRegistration reject: '%s' (%u)\n",
                             qmi_nas_network_service_domain_get_string (registration_reject_domain),
                             registration_reject_cause);
                if (network_id_valid) {
                    g_print ("\t\tMCC: '%s'\n", mcc);
                    if ((guchar)mnc[2] == 0xFF)
                        g_print ("\t\tMNC: '%.2s'\n", mnc);
                    else
                        g_print ("\t\tMNC: '%.3s'\n", mnc);
                }
                if (hs_call_status_valid)
                    g_print ("\t\tHS call status: '%s'\n", qmi_nas_wcdma_hs_service_get_string (hs_call_status));
                if (hs_service_valid)
                    g_print ("\t\tHS service: '%s'\n", qmi_nas_wcdma_hs_service_get_string (hs_service));
                if (cell_parameter_id_valid)
                    g_print ("\t\tCell parameter ID: '%" G_GUINT16_FORMAT"'\n", cell_parameter_id);
                if (cell_broadcast_support_valid)
                    g_print ("\t\tCell broadcast support: '%s'\n", qmi_nas_cell_broadcast_capability_get_string (cell_broadcast_support));
                if (call_barring_status_cs_valid)
                    g_print ("\t\tCall barring status (CS): '%s'\n", qmi_nas_call_barring_status_get_string (call_barring_status_cs));
                if (call_barring_status_ps_valid)
                    g_print ("\t\tCall barring status (PS): '%s'\n", qmi_nas_call_barring_status_get_string (call_barring_status_ps));
                if (cipher_domain_valid)
                    g_print ("\t\tCipher Domain: '%s'\n", qmi_nas_network_service_domain_get_string (cipher_domain));
            }
        }

        /* Common */
        {
            QmiNasSimRejectState sim_reject_info;

            if (qmi_message_nas_get_system_info_output_get_sim_reject_info (
                    output,
                    &sim_reject_info,
                    NULL)) {
                g_print ("\tSIM reject info: '%s'\n", qmi_nas_sim_reject_state_get_string (sim_reject_info));
            }
        }
    }

    qmi_message_nas_get_system_info_output_unref (output);
    operation_shutdown (TRUE);
}

static void
get_technology_preference_ready (QmiClientNas *client,
                                 GAsyncResult *res)
{
    QmiMessageNasGetTechnologyPreferenceOutput *output;
    GError *error = NULL;
    QmiNasRadioTechnologyPreference preference;
    QmiNasPreferenceDuration duration;
    gchar *preference_string;

    output = qmi_client_nas_get_technology_preference_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_nas_get_technology_preference_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get technology preference: %s\n", error->message);
        g_error_free (error);
        qmi_message_nas_get_technology_preference_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    qmi_message_nas_get_technology_preference_output_get_active (
        output,
        &preference,
        &duration,
        NULL);

    preference_string = qmi_nas_radio_technology_preference_build_string_from_mask (preference);
    g_print ("[%s] Successfully got technology preference\n"
             "\tActive: '%s', duration: '%s'\n",
             qmi_device_get_path_display (ctx->device),
             preference_string,
             qmi_nas_preference_duration_get_string (duration));
    g_free (preference_string);

    if (qmi_message_nas_get_technology_preference_output_get_persistent (
            output,
            &preference,
            NULL)) {
        preference_string = qmi_nas_radio_technology_preference_build_string_from_mask (preference);
        g_print ("\tPersistent: '%s'\n",
                 preference_string);
        g_free (preference_string);
    }

    qmi_message_nas_get_technology_preference_output_unref (output);
    operation_shutdown (TRUE);
}

static void
get_system_selection_preference_ready (QmiClientNas *client,
                                       GAsyncResult *res)
{
    QmiMessageNasGetSystemSelectionPreferenceOutput *output;
    GError *error = NULL;
    gboolean emergency_mode;
    QmiNasRatModePreference mode_preference;
    QmiNasBandPreference band_preference;
    QmiNasLteBandPreference lte_band_preference;
    QmiNasTdScdmaBandPreference td_scdma_band_preference;
    QmiNasCdmaPrlPreference cdma_prl_preference;
    QmiNasRoamingPreference roaming_preference;
    QmiNasNetworkSelectionPreference network_selection_preference;
    QmiNasServiceDomainPreference service_domain_preference;
    QmiNasGsmWcdmaAcquisitionOrderPreference gsm_wcdma_acquisition_order_preference;
    guint16 mcc;
    guint16 mnc;
    guint64 extended_lte_band_preference[4];
    gboolean has_pcs_digit;
    GArray *acquisition_order_preference;

    output = qmi_client_nas_get_system_selection_preference_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_nas_get_system_selection_preference_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get system_selection preference: %s\n", error->message);
        g_error_free (error);
        qmi_message_nas_get_system_selection_preference_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] Successfully got system selection preference\n",
             qmi_device_get_path_display (ctx->device));

    if (qmi_message_nas_get_system_selection_preference_output_get_emergency_mode (
            output,
            &emergency_mode,
            NULL)) {
        g_print ("\tEmergency mode: '%s'\n",
                 emergency_mode ? "yes" : "no");
    }

    if (qmi_message_nas_get_system_selection_preference_output_get_mode_preference (
            output,
            &mode_preference,
            NULL)) {
        gchar *str;

        str = qmi_nas_rat_mode_preference_build_string_from_mask (mode_preference);
        g_print ("\tMode preference: '%s'\n", str);
        g_free (str);
    }

    if (qmi_message_nas_get_system_selection_preference_output_get_band_preference (
            output,
            &band_preference,
            NULL)) {
        gchar *str;

        str = qmi_nas_band_preference_build_string_from_mask (band_preference);
        g_print ("\tBand preference: '%s'\n", str);
        g_free (str);
    }

    if (qmi_message_nas_get_system_selection_preference_output_get_lte_band_preference (
            output,
            &lte_band_preference,
            NULL)) {
        gchar *str;

        str = qmi_nas_lte_band_preference_build_string_from_mask (lte_band_preference);
        g_print ("\tLTE band preference: '%s'\n", str);
        g_free (str);
    }

    if (qmi_message_nas_get_system_selection_preference_output_get_extended_lte_band_preference (
            output,
            &extended_lte_band_preference[0],
            &extended_lte_band_preference[1],
            &extended_lte_band_preference[2],
            &extended_lte_band_preference[3],
            NULL)) {
        guint    i;
        gboolean first = TRUE;

        g_print ("\tLTE band preference (extended): '");
        for (i = 0; i < G_N_ELEMENTS (extended_lte_band_preference); i++) {
            guint j;

            for (j = 0; j < 64; j++) {
                guint band;

                if (!(extended_lte_band_preference[i] & (((guint64) 1) << j)))
                    continue;
                band = 1 + j + (i * 64);
                if (first) {
                    g_print ("%u", band);
                    first = FALSE;
                } else
                    g_print (", %u", band);
            }
        }
        g_print ("'\n");
    }

    if (qmi_message_nas_get_system_selection_preference_output_get_td_scdma_band_preference (
            output,
            &td_scdma_band_preference,
            NULL)) {
        gchar *str;

        str = qmi_nas_td_scdma_band_preference_build_string_from_mask (td_scdma_band_preference);
        g_print ("\tTD-SCDMA band preference: '%s'\n", str);
        g_free (str);
    }

    if (qmi_message_nas_get_system_selection_preference_output_get_cdma_prl_preference (
            output,
            &cdma_prl_preference,
            NULL)) {
        g_print ("\tCDMA PRL preference: '%s'\n",
                 qmi_nas_cdma_prl_preference_get_string (cdma_prl_preference));
    }

    if (qmi_message_nas_get_system_selection_preference_output_get_roaming_preference (
            output,
            &roaming_preference,
            NULL)) {
        g_print ("\tRoaming preference: '%s'\n",
                 qmi_nas_roaming_preference_get_string (roaming_preference));
    }

    if (qmi_message_nas_get_system_selection_preference_output_get_network_selection_preference (
            output,
            &network_selection_preference,
            NULL)) {
        g_print ("\tNetwork selection preference: '%s'\n",
                 qmi_nas_network_selection_preference_get_string (network_selection_preference));
    }


    if (qmi_message_nas_get_system_selection_preference_output_get_service_domain_preference (
            output,
            &service_domain_preference,
            NULL)) {
        g_print ("\tService domain preference: '%s'\n",
                 qmi_nas_service_domain_preference_get_string (service_domain_preference));
    }

    if (qmi_message_nas_get_system_selection_preference_output_get_gsm_wcdma_acquisition_order_preference (
            output,
            &gsm_wcdma_acquisition_order_preference,
            NULL)) {
        g_print ("\tGSM/WCDMA acquisition order preference: '%s'\n",
                 qmi_nas_gsm_wcdma_acquisition_order_preference_get_string (gsm_wcdma_acquisition_order_preference));
    }

    if (qmi_message_nas_get_system_selection_preference_output_get_manual_network_selection (
            output,
            &mcc,
            &mnc,
            &has_pcs_digit,
            NULL)) {
        g_print ("\tManual network selection:\n"
                 "\t\tMCC: '%" G_GUINT16_FORMAT"'\n"
                 "\t\tMNC: '%" G_GUINT16_FORMAT"'\n"
                 "\t\tMCC with PCS digit: '%s'\n",
                 mcc,
                 mnc,
                 has_pcs_digit ? "yes" : "no");
    }

    if (qmi_message_nas_get_system_selection_preference_output_get_acquisition_order_preference (
            output,
            &acquisition_order_preference,
            NULL)) {
        guint i;

        g_print ("\tAcquisition order preference: ");
        for (i = 0; i < acquisition_order_preference->len; i++) {
            QmiNasRadioInterface radio_interface;

            radio_interface = g_array_index (acquisition_order_preference, QmiNasRadioInterface, i);
            g_print ("%s%s",
                     i > 0 ? ", " : "",
                     qmi_nas_radio_interface_get_string (radio_interface));
        }
        g_print ("\n");
    }

    qmi_message_nas_get_system_selection_preference_output_unref (output);
    operation_shutdown (TRUE);
}

static QmiMessageNasSetSystemSelectionPreferenceInput *
set_system_selection_preference_input_create (const gchar *str)
{
    QmiMessageNasSetSystemSelectionPreferenceInput *input = NULL;
    QmiNasRatModePreference                         rat_mode_preference;
    GArray                                         *acquisition_order;
    GError                                         *error = NULL;

    if (!qmicli_read_ssp_options_from_string (str, &rat_mode_preference, &acquisition_order)) {
        g_printerr ("error: failed to parse system selection preference options\n");
        return NULL;
    }

    input = qmi_message_nas_set_system_selection_preference_input_new ();

    if (!qmi_message_nas_set_system_selection_preference_input_set_change_duration (input, QMI_NAS_CHANGE_DURATION_PERMANENT, &error))
        goto out;

    if (rat_mode_preference && !qmi_message_nas_set_system_selection_preference_input_set_mode_preference (input, rat_mode_preference, &error))
        goto out;

    if ((rat_mode_preference & (QMI_NAS_RAT_MODE_PREFERENCE_GSM | QMI_NAS_RAT_MODE_PREFERENCE_UMTS | QMI_NAS_RAT_MODE_PREFERENCE_LTE)) &&
        (!qmi_message_nas_set_system_selection_preference_input_set_gsm_wcdma_acquisition_order_preference (
            input,
            QMI_NAS_GSM_WCDMA_ACQUISITION_ORDER_PREFERENCE_AUTOMATIC,
            &error)))
        goto out;

    if (acquisition_order && !qmi_message_nas_set_system_selection_preference_input_set_acquisition_order_preference (input, acquisition_order, &error))
        goto out;

out:

    if (acquisition_order)
        g_array_unref (acquisition_order);

    if (error) {
        g_printerr ("error: couldn't create input data bundle: '%s'\n", error->message);
        g_error_free (error);
        qmi_message_nas_set_system_selection_preference_input_unref (input);
        return NULL;
    }

    return input;
}

static void
set_system_selection_preference_ready (QmiClientNas *client,
                                       GAsyncResult *res)
{
    QmiMessageNasSetSystemSelectionPreferenceOutput *output = NULL;
    GError *error = NULL;

    output = qmi_client_nas_set_system_selection_preference_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_nas_set_system_selection_preference_output_get_result (output, &error)) {
        g_printerr ("error: couldn't set operating mode: %s\n", error->message);
        g_error_free (error);
        qmi_message_nas_set_system_selection_preference_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] System selection preference set successfully; replug your device.\n",
             qmi_device_get_path_display (ctx->device));

    qmi_message_nas_set_system_selection_preference_output_unref (output);
    operation_shutdown (TRUE);
}

static void
network_scan_ready (QmiClientNas *client,
                    GAsyncResult *res)
{
    QmiMessageNasNetworkScanOutput *output;
    GError *error = NULL;
    GArray *array;

    output = qmi_client_nas_network_scan_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_nas_network_scan_output_get_result (output, &error)) {
        g_printerr ("error: couldn't scan networks: %s\n", error->message);
        g_error_free (error);
        qmi_message_nas_network_scan_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] Successfully scanned networks\n",
             qmi_device_get_path_display (ctx->device));

    array = NULL;
    if (qmi_message_nas_network_scan_output_get_network_information (output, &array, NULL)) {
        guint i;

        for (i = 0; i < array->len; i++) {
            QmiMessageNasNetworkScanOutputNetworkInformationElement *element;
            gchar *status_str;

            element = &g_array_index (array, QmiMessageNasNetworkScanOutputNetworkInformationElement, i);
            status_str = qmi_nas_network_status_build_string_from_mask (element->network_status);
            g_print ("Network [%u]:\n"
                     "\tMCC: '%" G_GUINT16_FORMAT"'\n"
                     "\tMNC: '%" G_GUINT16_FORMAT"'\n"
                     "\tStatus: '%s'\n"
                     "\tDescription: '%s'\n",
                     i,
                     element->mcc,
                     element->mnc,
                     status_str,
                     element->description);
            g_free (status_str);
        }
    }

    array = NULL;
    if (qmi_message_nas_network_scan_output_get_radio_access_technology (output, &array, NULL)) {
        guint i;

        for (i = 0; i < array->len; i++) {
            QmiMessageNasNetworkScanOutputRadioAccessTechnologyElement *element;

            element = &g_array_index (array, QmiMessageNasNetworkScanOutputRadioAccessTechnologyElement, i);
            g_print ("Network [%u]:\n"
                     "\tMCC: '%" G_GUINT16_FORMAT"'\n"
                     "\tMNC: '%" G_GUINT16_FORMAT"'\n"
                     "\tRAT: '%s'\n",
                     i,
                     element->mcc,
                     element->mnc,
                     qmi_nas_radio_interface_get_string (element->radio_interface));
        }
    }

    array = NULL;
    if (qmi_message_nas_network_scan_output_get_mnc_pcs_digit_include_status (output, &array, NULL)) {
        guint i;

        for (i = 0; i < array->len; i++) {
            QmiMessageNasNetworkScanOutputMncPcsDigitIncludeStatusElement *element;

            element = &g_array_index (array, QmiMessageNasNetworkScanOutputMncPcsDigitIncludeStatusElement, i);
            g_print ("Network [%u]:\n"
                     "\tMCC: '%" G_GUINT16_FORMAT"'\n"
                     "\tMNC: '%" G_GUINT16_FORMAT"'\n"
                     "\tMCC with PCS digit: '%s'\n",
                     i,
                     element->mcc,
                     element->mnc,
                     element->includes_pcs_digit ? "yes" : "no");
        }
    }

    qmi_message_nas_network_scan_output_unref (output);
    operation_shutdown (TRUE);
}

static gchar *
str_from_bcd_plmn (const gchar *bcd)
{
    static const gchar bcd_chars[] = "0123456789*#abc\0\0";
    gchar *str;
    guint i;
    guint j;

    str = g_malloc (7);
    for (i = 0, j = 0 ; i < 3; i++) {
        str[j] = bcd_chars[bcd[i] & 0xF];
        if (str[j])
            j++;
        str[j] = bcd_chars[(bcd[i] >> 4) & 0xF];
        if (str[j])
            j++;
    }
    str[j] = '\0';

    return str;
}

static void
get_cell_location_info_ready (QmiClientNas *client,
                              GAsyncResult *res)
{
    QmiMessageNasGetCellLocationInfoOutput *output;
    GError *error = NULL;
    GArray *array;
    GArray *array2;

    const gchar *operator;

    guint32 cell_id;
    guint16 lac;
    guint16 absolute_rf_channel_number;
    guint8 base_station_identity_code;
    guint32 timing_advance;
    guint16 rx_level;

    guint16 cell_id_16;
    guint16 primary_scrambling_code;
    gint16 rscp;
    gint16 ecio;

    guint16 system_id;
    guint16 network_id;
    guint16 base_station_id;
    guint16 reference_pn;
    guint32 latitude;
    guint32 longitude;

    gboolean ue_in_idle;
    guint16 tracking_area_code;
    guint32 global_cell_id;
    guint16 serving_cell_id;
    guint8 cell_reselection_priority;
    guint8 s_non_intra_search_threshold;
    guint8 serving_cell_low_threshold;
    guint8 s_intra_search_threshold;

    QmiNasWcdmaRrcState rrc_state;

    output = qmi_client_nas_get_cell_location_info_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_nas_get_cell_location_info_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get cell location info: %s\n", error->message);
        g_error_free (error);
        qmi_message_nas_get_cell_location_info_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] Successfully got cell location info\n",
             qmi_device_get_path_display (ctx->device));

    array = NULL;
    if (qmi_message_nas_get_cell_location_info_output_get_geran_info (
            output,
            &cell_id,
            &operator,
            &lac,
            &absolute_rf_channel_number,
            &base_station_identity_code,
            &timing_advance,
            &rx_level,
            &array, NULL)) {
        guint i;

        g_print ("GERAN Info\n");
        if (cell_id == 0xFFFFFFFF)
            g_print ("\tCell ID: 'unavailable'\n"
                     "\tPLMN: 'unavailable'\n"
                     "\tLocation Area Code: 'unavailable'\n");
        else {
            gchar *plmn;

            plmn = str_from_bcd_plmn (operator);
            g_print ("\tCell ID: '%" G_GUINT32_FORMAT"'\n"
                     "\tPLMN: '%s'\n"
                     "\tLocation Area Code: '%" G_GUINT16_FORMAT"'\n",
                     cell_id,
                     plmn,
                     lac);
            g_free (plmn);
        }
        g_print ("\tGERAN Absolute RF Channel Number: '%" G_GUINT16_FORMAT"'\n"
                 "\tBase Station Identity Code: '%u'\n",
                 absolute_rf_channel_number,
                 base_station_identity_code);
        if (timing_advance == 0xFFFFFFFF)
            g_print ("\tTiming Advance: 'unavailable'\n");
        else
            g_print ("\tTiming Advance: '%" G_GUINT32_FORMAT"' bit periods ('%lf' us)\n",
                     timing_advance,
                     (gdouble)timing_advance * 48.0 / 13.0);
        if (rx_level == 0)
            g_print ("\tRX Level: -110 dBm > level ('%" G_GUINT16_FORMAT"')\n", rx_level);
        else if (rx_level == 63)
            g_print ("\tRX Level: level > -48 dBm ('%" G_GUINT16_FORMAT"')\n", rx_level);
        else if (rx_level > 0 && rx_level < 63)
            g_print ("\tRX Level: %d dBm > level > %d dBm ('%" G_GUINT16_FORMAT"')\n",
                     (gint32) rx_level - 111,
                     (gint32) rx_level - 110,
                     rx_level);
        else
            g_print ("\tRX Level: invalid ('%" G_GUINT16_FORMAT"')\n", rx_level);


        for (i = 0; i < array->len; i++) {
            QmiMessageNasGetCellLocationInfoOutputGeranInfoCellElement *element;

            element = &g_array_index (array, QmiMessageNasGetCellLocationInfoOutputGeranInfoCellElement, i);

            g_print ("\tCell [%u]:\n", i);
            if (element->cell_id == 0xFFFFFFFF)
                g_print ("\t\tCell ID: 'unavailable'\n"
                         "\t\tPLMN: 'unavailable'\n"
                         "\t\tLocation Area Code: 'unavailable'\n");
            else {
                gchar *plmn;

                plmn = str_from_bcd_plmn (element->plmn);
                g_print ("\t\tCell ID: '%" G_GUINT32_FORMAT"'\n"
                         "\t\tPLMN: '%s'\n"
                         "\t\tLocation Area Code: '%" G_GUINT16_FORMAT"'\n",
                         element->cell_id,
                         plmn,
                         element->lac);
                g_free (plmn);
            }
            g_print ("\t\tGERAN Absolute RF Channel Number: '%" G_GUINT16_FORMAT"'\n"
                     "\t\tBase Station Identity Code: '%u'\n",
                     element->geran_absolute_rf_channel_number,
                     element->base_station_identity_code);
            if (element->rx_level == 0)
                g_print ("\t\tRX Level: -110 dBm > level ('%" G_GUINT16_FORMAT"')\n", element->rx_level);
            else if (element->rx_level == 63)
                g_print ("\t\tRX Level: level > -48 dBm ('%" G_GUINT16_FORMAT"')\n", element->rx_level);
            else if (element->rx_level > 0 && element->rx_level < 63)
                g_print ("\t\tRX Level: %d dBm > level > %d dBm ('%" G_GUINT16_FORMAT"')\n",
                         (gint32) element->rx_level - 111,
                         (gint32) element->rx_level - 110,
                         element->rx_level);
            else
                g_print ("\tRX Level: invalid ('%" G_GUINT16_FORMAT"')\n", element->rx_level);
        }
    }

    array = NULL;
    array2 = NULL;
    if (qmi_message_nas_get_cell_location_info_output_get_umts_info (
            output,
            &cell_id_16,
            &operator,
            &lac,
            &absolute_rf_channel_number,
            &primary_scrambling_code,
            &rscp,
            &ecio,
            &array, &array2, NULL)) {
        guint i;
        gchar *plmn;

        g_print ("UMTS Info\n");
        if (cell_id_16 == 0xFFFF)
            g_print ("\tCell ID: 'unavailable'\n");
        else
            g_print ("\tCell ID: '%" G_GUINT16_FORMAT"'\n", cell_id_16);
        plmn = str_from_bcd_plmn (operator);
        g_print ("\tPLMN: '%s'\n"
                 "\tLocation Area Code: '%" G_GUINT16_FORMAT"'\n"
                 "\tUTRA Absolute RF Channel Number: '%" G_GUINT16_FORMAT"'\n"
                 "\tPrimary Scrambling Code: '%" G_GUINT16_FORMAT"'\n"
                 "\tRSCP: '%" G_GINT16_FORMAT"' dBm\n"
                 "\tECIO: '%" G_GINT16_FORMAT"' dBm\n",
                 plmn,
                 lac,
                 absolute_rf_channel_number,
                 primary_scrambling_code,
                 rscp,
                 ecio);
        g_free (plmn);

        for (i = 0; i < array->len; i++) {
            QmiMessageNasGetCellLocationInfoOutputUmtsInfoCellElement *element;

            element = &g_array_index (array, QmiMessageNasGetCellLocationInfoOutputUmtsInfoCellElement, i);
            g_print ("\tCell [%u]:\n"
                     "\t\tUTRA Absolute RF Channel Number: '%" G_GUINT16_FORMAT"'\n"
                     "\t\tPrimary Scrambling Code: '%" G_GUINT16_FORMAT"'\n"
                     "\t\tRSCP: '%" G_GINT16_FORMAT"' dBm\n"
                     "\t\tECIO: '%" G_GINT16_FORMAT"' dBm\n",
                     i,
                     element->utra_absolute_rf_channel_number,
                     element->primary_scrambling_code,
                     element->rscp,
                     element->ecio);
        }

        for (i = 0; i < array2->len; i++) {
            QmiMessageNasGetCellLocationInfoOutputUmtsInfoNeighboringGeranElement *element;

            element = &g_array_index (array2, QmiMessageNasGetCellLocationInfoOutputUmtsInfoNeighboringGeranElement, i);
            g_print ("\tNeighboring GERAN Cell [%u]:\n"
                     "\t\tGERAN Absolute RF Channel Number: '%" G_GUINT16_FORMAT"'\n",
                     i,
                     element->geran_absolute_rf_channel_number);
            if (element->network_color_code == 0xFF)
                g_print ("\t\tNetwork Color Code: 'unavailable'\n");
            else
                g_print ("\t\tNetwork Color Code: '%u'\n", element->network_color_code);
            if (element->base_station_color_code == 0xFF)
                g_print ("\t\tBase Station Color Code: 'unavailable'\n");
            else
                g_print ("\t\tBase Station Color Code: '%u'\n", element->base_station_color_code);
            g_print ("\t\tRSSI: '%" G_GUINT16_FORMAT"'\n",
                     element->rssi);
        }
    }

    if (qmi_message_nas_get_cell_location_info_output_get_cdma_info  (
            output,
            &system_id,
            &network_id,
            &base_station_id,
            &reference_pn,
            &latitude,
            &longitude,
            NULL)) {
        gdouble latitude_degrees;
        gdouble longitude_degrees;

        /* TODO: give degrees, minutes, seconds */
        latitude_degrees = ((gdouble)latitude * 0.25)/3600.0;
        longitude_degrees = ((gdouble)longitude * 0.25)/3600.0;
        g_print ("CDMA Info\n"
                 "\tSystem ID: '%" G_GUINT16_FORMAT"'\n"
                 "\tNetwork ID: '%" G_GUINT16_FORMAT"'\n"
                 "\tBase Station ID: '%" G_GUINT16_FORMAT"'\n"
                 "\tReference PN: '%" G_GUINT16_FORMAT"'\n"
                 "\tLatitude: '%lf'º\n"
                 "\tLongitude: '%lfº'\n",
                 system_id,
                 network_id,
                 base_station_id,
                 reference_pn,
                 latitude_degrees,
                 longitude_degrees);
    }

    array = NULL;
    if (qmi_message_nas_get_cell_location_info_output_get_intrafrequency_lte_info (
            output,
            &ue_in_idle,
            &operator,
            &tracking_area_code,
            &global_cell_id,
            &absolute_rf_channel_number,
            &serving_cell_id,
            &cell_reselection_priority,
            &s_non_intra_search_threshold,
            &serving_cell_low_threshold,
            &s_intra_search_threshold,
            &array, NULL)) {
        guint i;
        gchar *plmn;

        plmn = str_from_bcd_plmn (operator);
        g_print ("Intrafrequency LTE Info\n"
                 "\tUE In Idle: '%s'\n"
                 "\tPLMN: '%s'\n"
                 "\tTracking Area Code: '%" G_GUINT16_FORMAT"'\n"
                 "\tGlobal Cell ID: '%" G_GUINT32_FORMAT"'\n"
                 "\tEUTRA Absolute RF Channel Number: '%" G_GUINT16_FORMAT"' (%s)\n"
                 "\tServing Cell ID: '%" G_GUINT16_FORMAT"'\n",
                 ue_in_idle ? "yes" : "no",
                 plmn,
                 tracking_area_code,
                 global_cell_id,
                 absolute_rf_channel_number, qmicli_earfcn_to_eutra_band_string (absolute_rf_channel_number),
                 serving_cell_id);
        g_free (plmn);
        if (ue_in_idle)
            g_print ("\tCell Reselection Priority: '%u'\n"
                     "\tS Non Intra Search Threshold: '%u'\n"
                     "\tServing Cell Low Threshold: '%u'\n"
                     "\tS Intra Search Threshold: '%u'\n",
                     cell_reselection_priority,
                     s_non_intra_search_threshold,
                     serving_cell_low_threshold,
                     s_intra_search_threshold);

        for (i = 0; i < array->len; i++) {
            QmiMessageNasGetCellLocationInfoOutputIntrafrequencyLteInfoCellElement *element;

            element = &g_array_index (array, QmiMessageNasGetCellLocationInfoOutputIntrafrequencyLteInfoCellElement, i);
            g_print ("\tCell [%u]:\n"
                     "\t\tPhysical Cell ID: '%" G_GUINT16_FORMAT"'\n"
                     "\t\tRSRQ: '%.1lf' dB\n"
                     "\t\tRSRP: '%.1lf' dBm\n"
                     "\t\tRSSI: '%.1lf' dBm\n",
                     i,
                     element->physical_cell_id,
                     (gdouble) element->rsrq * 0.1,
                     (gdouble) element->rsrp * 0.1,
                     (gdouble) element->rssi * 0.1);
            if (ue_in_idle)
                g_print ("\t\tCell Selection RX Level: '%" G_GINT16_FORMAT"'\n",
                         element->cell_selection_rx_level);
        }
    }

    array = NULL;
    if (qmi_message_nas_get_cell_location_info_output_get_interfrequency_lte_info (
            output,
            &ue_in_idle,
            &array, NULL)) {
        guint i;

        g_print ("Interfrequency LTE Info\n"
                 "\tUE In Idle: '%s'\n", ue_in_idle ? "yes" : "no");

        for (i = 0; i < array->len; i++) {
            QmiMessageNasGetCellLocationInfoOutputInterfrequencyLteInfoFrequencyElement *element;
            GArray *cell_array;
            guint j;

            element = &g_array_index (array, QmiMessageNasGetCellLocationInfoOutputInterfrequencyLteInfoFrequencyElement, i);
            g_print ("\tFrequency [%u]:\n"
                     "\t\tEUTRA Absolute RF Channel Number: '%" G_GUINT16_FORMAT"' (%s)\n"
                     "\t\tSelection RX Level Low Threshold: '%u'\n"
                     "\t\tCell Selection RX Level High Threshold: '%u'\n",
                     i,
                     element->eutra_absolute_rf_channel_number, qmicli_earfcn_to_eutra_band_string (element->eutra_absolute_rf_channel_number),
                     element->cell_selection_rx_level_low_threshold,
                     element->cell_selection_rx_level_high_threshold);
            if (ue_in_idle)
                g_print ("\t\tCell Reselection Priority: '%u'\n",
                         element->cell_reselection_priority);

            cell_array = element->cell;

            for (j = 0; j < cell_array->len; j++) {
                QmiMessageNasGetCellLocationInfoOutputInterfrequencyLteInfoFrequencyElementCellElement *cell;

                cell = &g_array_index (cell_array, QmiMessageNasGetCellLocationInfoOutputInterfrequencyLteInfoFrequencyElementCellElement, j);
                g_print ("\t\tCell [%u]:\n"
                         "\t\t\tPhysical Cell ID: '%" G_GUINT16_FORMAT"'\n"
                         "\t\t\tRSRQ: '%.1lf' dB\n"
                         "\t\t\tRSRP: '%.1lf' dBm\n"
                         "\t\t\tRSSI: '%.1lf' dBm\n"
                         "\t\t\tCell Selection RX Level: '%" G_GINT16_FORMAT"'\n",
                         j,
                         cell->physical_cell_id,
                         (gdouble) cell->rsrq * 0.1,
                         (gdouble) cell->rsrp * 0.1,
                         (gdouble) cell->rssi * 0.1,
                         cell->cell_selection_rx_level);
            }
        }
    }

    array = NULL;
    if (qmi_message_nas_get_cell_location_info_output_get_lte_info_neighboring_gsm (
            output,
            &ue_in_idle,
            &array, NULL)) {
        guint i;

        g_print ("LTE Info Neighboring GSM\n"
                 "\tUE In Idle: '%s'\n", ue_in_idle ? "yes" : "no");

        for (i = 0; i < array->len; i++) {
            QmiMessageNasGetCellLocationInfoOutputLteInfoNeighboringGsmFrequencyElement *element;
            GArray *cell_array;
            guint j;

            element = &g_array_index (array, QmiMessageNasGetCellLocationInfoOutputLteInfoNeighboringGsmFrequencyElement, i);

            g_print ("\tFrequency [%u]:\n", i);
            if (ue_in_idle)
                g_print ("\t\tCell Reselection Priority: '%u'\n"
                         "\t\tCell Reselection High Threshold: '%u'\n"
                         "\t\tCell Reselection Low Threshold: '%u'\n"
                         "\t\tNCC Permitted: '0x%2X'\n",
                         element->cell_reselection_priority,
                         element->cell_reselection_high_threshold,
                         element->cell_reselection_low_threshold,
                         element->ncc_permitted);

            cell_array = element->cell;

            for (j = 0; j < cell_array->len; j++) {
                QmiMessageNasGetCellLocationInfoOutputLteInfoNeighboringGsmFrequencyElementCellElement *cell;

                cell = &g_array_index (cell_array, QmiMessageNasGetCellLocationInfoOutputLteInfoNeighboringGsmFrequencyElementCellElement, j);
                g_print ("\t\tCell [%u]:\n"
                         "\t\t\tGERAN Absolute RF Channel Number: '%" G_GUINT16_FORMAT"'\n"
                         "\t\t\tBand Is 1900: '%s'\n",
                         j,
                         cell->geran_absolute_rf_channel_number,
                         cell->band_is_1900 ? "yes" : "no");
                if (cell->cell_id_valid)
                    g_print ("\t\t\tBase Station Identity Code: '%u'\n",
                             cell->base_station_identity_code);
                else
                    g_print ("\t\t\tBase Station Identity Code: 'unknown'\n");
                g_print ("\t\t\tRSSI: '%.1lf' dB\n"
                         "\t\t\tCell Selection RX Level: '%" G_GINT16_FORMAT"'\n",
                         (gdouble) cell->rssi * 0.1,
                         cell->cell_selection_rx_level);
            }
        }
    }

    array = NULL;
    if (qmi_message_nas_get_cell_location_info_output_get_lte_info_neighboring_wcdma (
            output,
            &ue_in_idle,
            &array, NULL)) {
        guint i;

        g_print ("LTE Info Neighboring WCDMA\n"
                 "\tUE In Idle: '%s'\n", ue_in_idle ? "yes" : "no");

        for (i = 0; i < array->len; i++) {
            QmiMessageNasGetCellLocationInfoOutputLteInfoNeighboringWcdmaFrequencyElement *element;
            GArray *cell_array;
            guint j;

            element = &g_array_index (array, QmiMessageNasGetCellLocationInfoOutputLteInfoNeighboringWcdmaFrequencyElement, i);

            g_print ("\tFrequency [%u]:\n"
                     "\t\tUTRA Absolute RF Channel Number: '%" G_GUINT16_FORMAT"'\n",
                     i,
                     element->utra_absolute_rf_channel_number);
            if (ue_in_idle)
                g_print ("\t\tCell Reselection Priority: '%u'\n"
                         "\t\tCell Reselection High Threshold: '%" G_GINT16_FORMAT"'\n"
                         "\t\tCell Reselection Low Threshold: '%" G_GINT16_FORMAT"'\n",
                         element->cell_reselection_priority,
                         element->cell_reselection_high_threshold,
                         element->cell_reselection_low_threshold);

            cell_array = element->cell;

            for (j = 0; j < cell_array->len; j++) {
                QmiMessageNasGetCellLocationInfoOutputLteInfoNeighboringWcdmaFrequencyElementCellElement *cell;

                cell = &g_array_index (cell_array, QmiMessageNasGetCellLocationInfoOutputLteInfoNeighboringWcdmaFrequencyElementCellElement, j);
                g_print ("\t\tCell [%u]:\n"
                         "\t\t\tPrimary Scrambling Code: '%" G_GUINT16_FORMAT"'\n"
                         "\t\t\tCPICH RSCP: '%.1lf' dBm\n"
                         "\t\t\tCPICH EcNo: '%.1lf' dB\n",
                         j,
                         cell->primary_scrambling_code,
                         (gdouble) cell->cpich_rscp * 0.1,
                         (gdouble) cell->cpich_ecno * 0.1);
                if (ue_in_idle)
                    g_print ("\t\t\tCell Selection RX Level: '%" G_GUINT16_FORMAT"'\n",
                             cell->cell_selection_rx_level);
            }
        }
    }

    if (qmi_message_nas_get_cell_location_info_output_get_umts_cell_id (
            output,
            &cell_id,
            NULL)) {
        g_print ("UMTS Cell ID: '%" G_GUINT32_FORMAT"'\n", cell_id);
    }

    array = NULL;
    if (qmi_message_nas_get_cell_location_info_output_get_umts_info_neighboring_lte (
            output,
            &rrc_state,
            &array, NULL)) {
        guint i;

        g_print ("UMTS Info Neighboring LTE\n"
                 "\tRRC State: '%s'\n", qmi_nas_wcdma_rrc_state_get_string (rrc_state));

        for (i = 0; i < array->len; i++) {
            QmiMessageNasGetCellLocationInfoOutputUmtsInfoNeighboringLteFrequencyElement *element;

            element = &g_array_index (array, QmiMessageNasGetCellLocationInfoOutputUmtsInfoNeighboringLteFrequencyElement, i);

            g_print ("\tFrequency [%u]:\n"
                     "\t\tEUTRA Absolute RF Channel Number: '%" G_GUINT16_FORMAT"' (%s)\n"
                     "\t\tPhysical Cell ID: '%" G_GUINT16_FORMAT "'\n"
                     "\t\tRSRP: '%f' dBm\n"
                     "\t\tRSRQ: '%f' dB\n",
                     i,
                     element->eutra_absolute_rf_channel_number, qmicli_earfcn_to_eutra_band_string (element->eutra_absolute_rf_channel_number),
                     element->physical_cell_id,
                     element->rsrp,
                     element->rsrq);
            if (rrc_state != QMI_NAS_WCDMA_RRC_STATE_CELL_FACH &&
                rrc_state != QMI_NAS_WCDMA_RRC_STATE_CELL_DCH)
                g_print ("\t\tCell Selection RX Level: '%" G_GINT16_FORMAT"'\n",
                         element->cell_selection_rx_level);
            g_print ("\t\tIs TDD?: '%s'\n",
                     element->is_tdd ? "yes" : "no");
        }
    }

    qmi_message_nas_get_cell_location_info_output_unref (output);
    operation_shutdown (TRUE);
}

static void
force_network_search_ready (QmiClientNas *client,
                            GAsyncResult *res)
{
    QmiMessageNasForceNetworkSearchOutput *output;
    GError *error = NULL;

    output = qmi_client_nas_force_network_search_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_nas_force_network_search_output_get_result (output, &error)) {
        g_printerr ("error: couldn't force network search: %s\n", error->message);
        g_error_free (error);
        qmi_message_nas_force_network_search_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] Successfully forced network search\n",
             qmi_device_get_path_display (ctx->device));
    qmi_message_nas_force_network_search_output_unref (output);
    operation_shutdown (TRUE);
}

static gchar *
garray_of_uint8_to_string (GArray *array,
                           QmiNasPlmnEncodingScheme scheme)
{
    gchar *decoded = NULL;

    if (array->len == 0)
        return NULL;

    if (scheme == QMI_NAS_PLMN_ENCODING_SCHEME_GSM) {
        guint8 *unpacked;
        guint32 unpacked_len = 0;

        /* Unpack the GSM and decode it */
        unpacked = qmicli_charset_gsm_unpack ((const guint8 *) array->data, (array->len * 8) / 7, &unpacked_len);
        if (unpacked) {
            decoded = (gchar *) qmicli_charset_gsm_unpacked_to_utf8 (unpacked, unpacked_len);
            g_free (unpacked);
        }
    } else if (scheme == QMI_NAS_PLMN_ENCODING_SCHEME_UCS2LE) {
        decoded = g_convert (
                        array->data,
                        array->len,
                        "UTF-8",
                        "UCS-2LE",
                        NULL,
                        NULL,
                        NULL);
    }

    return decoded;
}

static void
get_operator_name_ready (QmiClientNas *client,
                         GAsyncResult *res)
{
    QmiMessageNasGetOperatorNameOutput *output;
    GError *error = NULL;
    QmiNasNetworkNameDisplayCondition spn_display_condition;
    const gchar *spn;
    const gchar *operator_name;
    GArray *array;

    output = qmi_client_nas_get_operator_name_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_nas_get_operator_name_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get operator name data: %s\n", error->message);
        g_error_free (error);
        qmi_message_nas_get_operator_name_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] Successfully got operator name data\n",
             qmi_device_get_path_display (ctx->device));

    if (qmi_message_nas_get_operator_name_output_get_service_provider_name (
        output,
        &spn_display_condition,
        &spn,
        NULL)) {
        gchar *dc_string = qmi_nas_network_name_display_condition_build_string_from_mask (spn_display_condition);

        g_print ("Service Provider Name\n");
        g_print ("\tDisplay Condition: '%s'\n"
                 "\tName             : '%s'\n",
                 dc_string,
                 spn);
        g_free (dc_string);
    }

    if (qmi_message_nas_get_operator_name_output_get_operator_string_name (
        output,
        &operator_name,
        NULL)) {
        g_print ("Operator Name: '%s'\n",
                 operator_name);
    }

    if (qmi_message_nas_get_operator_name_output_get_operator_plmn_list (output, &array, NULL)) {
        guint i;

        g_print ("PLMN List:\n");
        for (i = 0; i < array->len; i++) {
            QmiMessageNasGetOperatorNameOutputOperatorPlmnListElement *element;
            gchar *mnc;

            element = &g_array_index (array, QmiMessageNasGetOperatorNameOutputOperatorPlmnListElement, i);
            mnc = g_strdup (element->mnc);
            if (strlen (mnc) >= 3 && (mnc[2] == 'F' || mnc[2] == 'f'))
                mnc[2] = '\0';
            g_print ("\tMCC/MNC: '%s-%s'%s LAC Range: %u->%u\tPNN Record: %u\n",
                     element->mcc,
                     mnc,
                     mnc[2] == '\0' ? " " : "",
                     element->lac1,
                     element->lac2,
                     element->plmn_name_record_identifier);
        }
    }

    if (qmi_message_nas_get_operator_name_output_get_operator_plmn_name (output, &array, NULL)) {
        guint i;

        g_print ("PLMN Names:\n");
        for (i = 0; i < array->len; i++) {
            QmiMessageNasGetOperatorNameOutputOperatorPlmnNameElement *element;
            gchar *long_name;
            gchar *short_name;

            element = &g_array_index (array, QmiMessageNasGetOperatorNameOutputOperatorPlmnNameElement, i);
            long_name = garray_of_uint8_to_string (element->long_name, element->name_encoding);
            short_name = garray_of_uint8_to_string (element->short_name, element->name_encoding);
            g_print ("\t%d: '%s'%s%s%s\t\tCountry: '%s'\n",
                     i,
                     long_name ?: "",
                     short_name ? " ('" : "",
                     short_name ?: "",
                     short_name ? "')" : "",
                     qmi_nas_plmn_name_country_initials_get_string (element->short_country_initials));
            g_free (long_name);
            g_free (short_name);
        }
    }

    qmi_message_nas_get_operator_name_output_unref (output);
    operation_shutdown (TRUE);
}

static void
get_lte_cphy_ca_info_ready (QmiClientNas *client,
                            GAsyncResult *res)
{
    QmiMessageNasGetLteCphyCaInfoOutput *output;
    GError *error = NULL;
    guint16 pci;
    guint16 channel;
    QmiNasDLBandwidth dl_bandwidth;
    QmiNasActiveBand band;
    QmiNasScellState state;
    guint8 scell_index;
    GArray *array;

    output = qmi_client_nas_get_lte_cphy_ca_info_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_nas_get_lte_cphy_ca_info_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get carrier aggregation info: %s\n", error->message);
        g_error_free (error);
        qmi_message_nas_get_lte_cphy_ca_info_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] Successfully got carrier aggregation info\n",
             qmi_device_get_path_display (ctx->device));

    if (qmi_message_nas_get_lte_cphy_ca_info_output_get_dl_bandwidth (
        output,
        &dl_bandwidth,
        NULL)) {
        g_print ("DL Bandwidth: '%s'\n",
                 qmi_nas_dl_bandwidth_get_string (dl_bandwidth));
    }

    if (qmi_message_nas_get_lte_cphy_ca_info_output_get_phy_ca_agg_pcell_info (
        output,
        &pci,
        &channel,
        &dl_bandwidth,
        &band,
        NULL)) {
        g_print ("Primary Cell Info\n");
        g_print ("\tPhysical Cell ID: '%" G_GUINT16_FORMAT"'\n"
                 "\tRX Channel: '%" G_GUINT16_FORMAT"'\n"
                 "\tDL Bandwidth: '%s'\n"
                 "\tLTE Band: '%s'\n",
                 pci, channel,
                 qmi_nas_dl_bandwidth_get_string (dl_bandwidth),
                 qmi_nas_active_band_get_string (band));
    }

    if (qmi_message_nas_get_lte_cphy_ca_info_output_get_phy_ca_agg_secondary_cells (
        output, &array, NULL)) {
        guint i;

        if (!array->len)
            g_print ("No Secondary Cells\n");
        for (i = 0; i < array->len; i++) {
            QmiMessageNasGetLteCphyCaInfoOutputPhyCaAggSecondaryCellsSsc *e;
            e = &g_array_index (array, QmiMessageNasGetLteCphyCaInfoOutputPhyCaAggSecondaryCellsSsc, i);
            g_print ("Secondary Cell %u Info\n"
                     "\tPhysical Cell ID: '%" G_GUINT16_FORMAT"'\n"
                     "\tRX Channel: '%" G_GUINT16_FORMAT"'\n"
                     "\tDL Bandwidth: '%s'\n"
                     "\tLTE Band: '%s'\n"
                     "\tState: '%s'\n"
                     "\tCell index: '%u'\n",
                     i + 1, e->physical_cell_id, e->rx_channel,
                     qmi_nas_dl_bandwidth_get_string (e->dl_bandwidth),
                     qmi_nas_active_band_get_string (e->lte_band),
                     qmi_nas_scell_state_get_string (e->state),
                     e->cell_index);
        }

    } else {
        if (qmi_message_nas_get_lte_cphy_ca_info_output_get_phy_ca_agg_scell_info (
            output,
            &pci,
            &channel,
            &dl_bandwidth,
            &band,
            &state,
            NULL)) {
            g_print ("Secondary Cell Info\n");
            g_print ("\tPhysical Cell ID: '%" G_GUINT16_FORMAT"'\n"
                     "\tRX Channel: '%" G_GUINT16_FORMAT"'\n"
                     "\tDL Bandwidth: '%s'\n"
                     "\tLTE Band: '%s'\n"
                     "\tState: '%s'\n",
                     pci, channel,
                     qmi_nas_dl_bandwidth_get_string (dl_bandwidth),
                     qmi_nas_active_band_get_string (band),
                     qmi_nas_scell_state_get_string (state));
        }

        if (qmi_message_nas_get_lte_cphy_ca_info_output_get_scell_index (
            output,
            &scell_index,
            NULL)) {
            g_print ("Secondary Cell index: '%u'\n", scell_index);
        }
    }

    qmi_message_nas_get_lte_cphy_ca_info_output_unref (output);
    operation_shutdown (TRUE);
}

static void
get_rf_band_info_ready (QmiClientNas *client,
                        GAsyncResult *res)
{
    QmiMessageNasGetRfBandInformationOutput *output;
    GError *error = NULL;
    GArray *band_array = NULL;
    guint i;

    output = qmi_client_nas_get_rf_band_information_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_nas_get_rf_band_information_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get rf band info: %s\n", error->message);
        g_error_free (error);
        qmi_message_nas_get_rf_band_information_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] Successfully got RF band info\n",
             qmi_device_get_path_display (ctx->device));

    if (!qmi_message_nas_get_rf_band_information_output_get_list (
        output,
        &band_array,
        &error)) {
        g_printerr ("error: couldn't get rf band list: %s\n", error->message);
        g_error_free (error);
        qmi_message_nas_get_rf_band_information_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    for (i = 0; band_array && i < band_array->len; i++) {
        QmiMessageNasGetRfBandInformationOutputListElement *info;

        info = &g_array_index (band_array, QmiMessageNasGetRfBandInformationOutputListElement, i);
        g_print ("\tRadio Interface:   '%s'\n"
                 "\tActive Band Class: '%s'\n"
                 "\tActive Channel:    '%" G_GUINT16_FORMAT"'\n",
                 qmi_nas_radio_interface_get_string (info->radio_interface),
                 qmi_nas_active_band_get_string (info->active_band_class),
                 info->active_channel);
    }

    qmi_message_nas_get_rf_band_information_output_unref (output);
    operation_shutdown (TRUE);
}

static void
get_supported_messages_ready (QmiClientNas *client,
                              GAsyncResult *res)
{
    QmiMessageNasGetSupportedMessagesOutput *output;
    GError *error = NULL;
    GArray *bytearray = NULL;
    gchar *str;

    output = qmi_client_nas_get_supported_messages_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_nas_get_supported_messages_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get supported NAS messages: %s\n", error->message);
        g_error_free (error);
        qmi_message_nas_get_supported_messages_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] Successfully got supported NAS messages:\n",
             qmi_device_get_path_display (ctx->device));

    qmi_message_nas_get_supported_messages_output_get_list (output, &bytearray, NULL);
    str = qmicli_get_supported_messages_list (bytearray ? (const guint8 *)bytearray->data : NULL,
                                              bytearray ? bytearray->len : 0);
    g_print ("%s", str);
    g_free (str);

    qmi_message_nas_get_supported_messages_output_unref (output);
    operation_shutdown (TRUE);
}

static void
reset_ready (QmiClientNas *client,
             GAsyncResult *res)
{
    QmiMessageNasResetOutput *output;
    GError *error = NULL;

    output = qmi_client_nas_reset_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_nas_reset_output_get_result (output, &error)) {
        g_printerr ("error: couldn't reset the NAS service: %s\n", error->message);
        g_error_free (error);
        qmi_message_nas_reset_output_unref (output);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] Successfully performed NAS service reset\n",
             qmi_device_get_path_display (ctx->device));

    qmi_message_nas_reset_output_unref (output);
    operation_shutdown (TRUE);
}

static gboolean
noop_cb (gpointer unused)
{
    operation_shutdown (TRUE);
    return FALSE;
}

void
qmicli_nas_run (QmiDevice *device,
                QmiClientNas *client,
                GCancellable *cancellable)
{
    /* Initialize context */
    ctx = g_slice_new (Context);
    ctx->device = g_object_ref (device);
    ctx->client = g_object_ref (client);
    ctx->cancellable = g_object_ref (cancellable);

    /* Request to get signal strength? */
    if (get_signal_strength_flag) {
        QmiMessageNasGetSignalStrengthInput *input;

        input = get_signal_strength_input_create ();

        g_debug ("Asynchronously getting signal strength...");
        qmi_client_nas_get_signal_strength (ctx->client,
                                            input,
                                            10,
                                            ctx->cancellable,
                                            (GAsyncReadyCallback)get_signal_strength_ready,
                                            NULL);
        qmi_message_nas_get_signal_strength_input_unref (input);
        return;
    }

    /* Request to get signal info? */
    if (get_signal_info_flag) {
        g_debug ("Asynchronously getting signal info...");
        qmi_client_nas_get_signal_info (ctx->client,
                                        NULL,
                                        10,
                                        ctx->cancellable,
                                        (GAsyncReadyCallback)get_signal_info_ready,
                                        NULL);
        return;
    }

    /* Request to get tx/rx info? */
    if (get_tx_rx_info_str) {
        QmiMessageNasGetTxRxInfoInput *input;
        QmiNasRadioInterface interface;

        input = get_tx_rx_info_input_create (get_tx_rx_info_str,
                                             &interface);
        if (!input) {
            operation_shutdown (FALSE);
            return;
        }

        g_debug ("Asynchronously getting TX/RX info...");
        qmi_client_nas_get_tx_rx_info (ctx->client,
                                       input,
                                       10,
                                       ctx->cancellable,
                                       (GAsyncReadyCallback)get_tx_rx_info_ready,
                                       GUINT_TO_POINTER (interface));
        qmi_message_nas_get_tx_rx_info_input_unref (input);
        return;
    }

    /* Request to get home network? */
    if (get_home_network_flag) {
        g_debug ("Asynchronously getting home network...");
        qmi_client_nas_get_home_network (ctx->client,
                                         NULL,
                                         10,
                                         ctx->cancellable,
                                         (GAsyncReadyCallback)get_home_network_ready,
                                         NULL);
        return;
    }

    /* Request to get serving system? */
    if (get_serving_system_flag) {
        g_debug ("Asynchronously getting serving system...");
        qmi_client_nas_get_serving_system (ctx->client,
                                           NULL,
                                           10,
                                           ctx->cancellable,
                                           (GAsyncReadyCallback)get_serving_system_ready,
                                           NULL);
        return;
    }

    /* Request to get system info? */
    if (get_system_info_flag) {
        g_debug ("Asynchronously getting system info...");
        qmi_client_nas_get_system_info (ctx->client,
                                        NULL,
                                        10,
                                        ctx->cancellable,
                                        (GAsyncReadyCallback)get_system_info_ready,
                                        NULL);
        return;
    }

    /* Request to get technology preference? */
    if (get_technology_preference_flag) {
        g_debug ("Asynchronously getting technology preference...");
        qmi_client_nas_get_technology_preference (ctx->client,
                                                  NULL,
                                                  10,
                                                  ctx->cancellable,
                                                  (GAsyncReadyCallback)get_technology_preference_ready,
                                                  NULL);
        return;
    }

    /* Request to get system_selection preference? */
    if (get_system_selection_preference_flag) {
        g_debug ("Asynchronously getting system selection preference...");
        qmi_client_nas_get_system_selection_preference (ctx->client,
                                                        NULL,
                                                        10,
                                                        ctx->cancellable,
                                                        (GAsyncReadyCallback)get_system_selection_preference_ready,
                                                        NULL);
        return;
    }

    /* Request to set system_selection preference? */
    if (set_system_selection_preference_str) {
        QmiMessageNasSetSystemSelectionPreferenceInput *input;
        g_debug ("Asynchronously setting system selection preference...");

        input = set_system_selection_preference_input_create (set_system_selection_preference_str);
        if (!input) {
            operation_shutdown (FALSE);
            return;
        }

        qmi_client_nas_set_system_selection_preference (ctx->client,
                                                        input,
                                                        10,
                                                        ctx->cancellable,
                                                        (GAsyncReadyCallback)set_system_selection_preference_ready,
                                                        NULL);
        qmi_message_nas_set_system_selection_preference_input_unref (input);
        return;
    }

    /* Request to scan networks? */
    if (network_scan_flag) {
        g_debug ("Asynchronously scanning networks...");
        qmi_client_nas_network_scan (ctx->client,
                                     NULL,
                                     300, /* this operation takes a lot of time! */
                                     ctx->cancellable,
                                     (GAsyncReadyCallback)network_scan_ready,
                                     NULL);
        return;
    }

    /* Request to get cell location info? */
    if (get_cell_location_info_flag) {
        g_debug ("Asynchronously getting cell location info ...");
        qmi_client_nas_get_cell_location_info (ctx->client,
                                               NULL,
                                               10,
                                               ctx->cancellable,
                                               (GAsyncReadyCallback)get_cell_location_info_ready,
                                               NULL);
        return;
    }

    /* Request to force network search */
    if (force_network_search_flag) {
        g_debug ("Forcing network search...");
        qmi_client_nas_force_network_search (ctx->client,
                                             NULL,
                                             10,
                                             ctx->cancellable,
                                             (GAsyncReadyCallback)force_network_search_ready,
                                             NULL);
        return;
    }

    /* Request to get operator name data */
    if (get_operator_name_flag) {
        g_debug ("Asynchronously getting operator name data...");
        qmi_client_nas_get_operator_name (ctx->client,
                                          NULL,
                                          10,
                                          ctx->cancellable,
                                          (GAsyncReadyCallback)get_operator_name_ready,
                                          NULL);
        return;
    }

    /* Request to get carrier aggregation info? */
    if (get_lte_cphy_ca_info_flag) {
        g_debug ("Asynchronously getting carrier aggregation info ...");
        qmi_client_nas_get_lte_cphy_ca_info (ctx->client,
                                             NULL,
                                             10,
                                             ctx->cancellable,
                                             (GAsyncReadyCallback)get_lte_cphy_ca_info_ready,
                                             NULL);
        return;
    }

    /* Request to get rf band info? */
    if (get_rf_band_info_flag) {
        g_debug ("Asynchronously getting RF band info ...");
        qmi_client_nas_get_rf_band_information (ctx->client,
                                                NULL,
                                                10,
                                                ctx->cancellable,
                                                (GAsyncReadyCallback)get_rf_band_info_ready,
                                                NULL);
        return;
    }

    /* Request to list supported messages? */
    if (get_supported_messages_flag) {
        g_debug ("Asynchronously getting supported NAS messages...");
        qmi_client_nas_get_supported_messages (ctx->client,
                                               NULL,
                                               10,
                                               ctx->cancellable,
                                               (GAsyncReadyCallback)get_supported_messages_ready,
                                               NULL);
        return;
    }

    /* Request to reset NAS service? */
    if (reset_flag) {
        g_debug ("Asynchronously resetting NAS service...");
        qmi_client_nas_reset (ctx->client,
                              NULL,
                              10,
                              ctx->cancellable,
                              (GAsyncReadyCallback)reset_ready,
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
