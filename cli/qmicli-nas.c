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
    QmiClientNas *client;
    GCancellable *cancellable;
} Context;
static Context *ctx;

/* Options */
static gboolean get_signal_strength_flag;
static gboolean get_signal_info_flag;
static gboolean network_scan_flag;
static gboolean reset_flag;
static gboolean noop_flag;

static GOptionEntry entries[] = {
    { "nas-get-signal-strength", 0, 0, G_OPTION_ARG_NONE, &get_signal_strength_flag,
      "Get signal strength (deprecated)",
      NULL
    },
    { "nas-get-signal-info", 0, 0, G_OPTION_ARG_NONE, &get_signal_info_flag,
      "Get signal info (deprecated)",
      NULL
    },
    { "nas-network-scan", 0, 0, G_OPTION_ARG_NONE, &network_scan_flag,
      "Scan networks",
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
                 network_scan_flag +
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

static gdouble
get_db_from_sinr_level (QmiNasEvdoSinrLevel level)
{
    switch (level) {
    case QMI_NAS_EVDO_SINR_LEVEL_0: return -9.0;
    case QMI_NAS_EVDO_SINR_LEVEL_1: return -6;
    case QMI_NAS_EVDO_SINR_LEVEL_2: return -4.5;
    case QMI_NAS_EVDO_SINR_LEVEL_3: return -3;
    case QMI_NAS_EVDO_SINR_LEVEL_4: return -2;
    case QMI_NAS_EVDO_SINR_LEVEL_5: return 1;
    case QMI_NAS_EVDO_SINR_LEVEL_6: return 3;
    case QMI_NAS_EVDO_SINR_LEVEL_7: return 6;
    case QMI_NAS_EVDO_SINR_LEVEL_8: return +9;
    default:
        g_warning ("Invalid SINR level '%u'", level);
        return -G_MAXDOUBLE;
    }
}

static void
get_signal_info_ready (QmiClientNas *client,
                       GAsyncResult *res)
{
    QmiMessageNasGetSignalInfoOutput *output;
    GError *error = NULL;
    gint8 rssi;
    guint16 ecio;
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
        shutdown (FALSE);
        return;
    }

    if (!qmi_message_nas_get_signal_info_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get signal info: %s\n", error->message);
        g_error_free (error);
        qmi_message_nas_get_signal_info_output_unref (output);
        shutdown (FALSE);
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
        g_print ("HDR:\n"
                 "\tRSSI: '%d dBm'\n"
                 "\tECIO: '%.1lf dBm'\n"
                 "\tSINR (%u): '%.1lf dB'\n"
                 "\tIO: '%d dBm'\n",
                 rssi,
                 (-0.5)*((gdouble)ecio),
                 sinr_level, get_db_from_sinr_level (sinr_level),
                 io);
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
                 "\tSNR: '%.1lf dBm'\n",
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
    shutdown (TRUE);
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
        shutdown (FALSE);
        return;
    }

    if (!qmi_message_nas_get_signal_strength_output_get_result (output, &error)) {
        g_printerr ("error: couldn't get signal strength: %s\n", error->message);
        g_error_free (error);
        qmi_message_nas_get_signal_strength_output_unref (output);
        shutdown (FALSE);
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
        g_print ("IO:\n"
                 "\tNetwork '%s': '%d dBm'\n",
                 qmi_nas_radio_interface_get_string (QMI_NAS_RADIO_INTERFACE_CDMA_1XEVDO),
                 io);
    }

    /* SINR level */
    if (qmi_message_nas_get_signal_strength_output_get_sinr (output, &sinr_level, NULL)) {
        g_print ("SINR:\n"
                 "\tNetwork '%s': (%u) '%.1lf dB'\n",
                 qmi_nas_radio_interface_get_string (QMI_NAS_RADIO_INTERFACE_CDMA_1XEVDO),
                 sinr_level, get_db_from_sinr_level (sinr_level));
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
    shutdown (TRUE);
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
        shutdown (FALSE);
        return;
    }

    if (!qmi_message_nas_network_scan_output_get_result (output, &error)) {
        g_printerr ("error: couldn't scan networks: %s\n", error->message);
        g_error_free (error);
        qmi_message_nas_network_scan_output_unref (output);
        shutdown (FALSE);
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
                     qmi_nas_radio_interface_get_string (element->rat));
        }
    }

    array = NULL;
    if (qmi_message_nas_network_scan_output_get_mnc_pds_digit_include_status (output, &array, NULL)) {
        guint i;

        for (i = 0; i < array->len; i++) {
            QmiMessageNasNetworkScanOutputMncPdsDigitIncludeStatusElement *element;

            element = &g_array_index (array, QmiMessageNasNetworkScanOutputMncPdsDigitIncludeStatusElement, i);
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
    shutdown (TRUE);
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
        shutdown (FALSE);
        return;
    }

    if (!qmi_message_nas_reset_output_get_result (output, &error)) {
        g_printerr ("error: couldn't reset the NAS service: %s\n", error->message);
        g_error_free (error);
        qmi_message_nas_reset_output_unref (output);
        shutdown (FALSE);
        return;
    }

    g_print ("[%s] Successfully performed NAS service reset\n",
             qmi_device_get_path_display (ctx->device));

    qmi_message_nas_reset_output_unref (output);
    shutdown (TRUE);
}

static gboolean
noop_cb (gpointer unused)
{
    shutdown (TRUE);
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
    if (cancellable)
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
