/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details:
 *
 * Copyright (C) 2014 Aleksander Morgado <aleksander@aleksander.es>
 */

#include <config.h>
#include <libqmi-glib.h>

#include "test-fixture.h"

/*****************************************************************************/

static void
test_generated_core (TestFixture *fixture)
{
    /* Noop */
}

/*****************************************************************************/
/* DMS Get IDs */

static void
dms_get_ids_ready (QmiClientDms *client,
                   GAsyncResult *res,
                   TestFixture  *fixture)
{
    QmiMessageDmsGetIdsOutput *output;
    GError *error = NULL;
    gboolean st;
    const gchar *str;

    output = qmi_client_dms_get_ids_finish (client, res, &error);
    g_assert_no_error (error);
    g_assert (output);

    st = qmi_message_dms_get_ids_output_get_result (output, &error);
    g_assert_no_error (error);
    g_assert (st);

    /* [/dev/cdc-wdm3] Device IDs retrieved:
     * 	 ESN: '80997874'
     * 	IMEI: '359225050039973'
     * 	MEID: '35922505003997' */
    st = qmi_message_dms_get_ids_output_get_esn (output, &str, &error);
    g_assert_no_error (error);
    g_assert (st);
    g_assert_cmpstr (str, ==, "80997874");

    st = qmi_message_dms_get_ids_output_get_imei (output, &str, &error);
    g_assert_no_error (error);
    g_assert (st);
    g_assert_cmpstr (str, ==, "359225050039973");

    st = qmi_message_dms_get_ids_output_get_meid (output, &str, &error);
    g_assert_no_error (error);
    g_assert (st);
    g_assert_cmpstr (str, ==, "35922505003997");

    qmi_message_dms_get_ids_output_unref (output);

    test_fixture_loop_stop (fixture);
}

static void
test_generated_dms_get_ids (TestFixture *fixture)
{
    guint8 expected[] = {
        0x01,
        0x0C, 0x00, 0x00, 0x02, 0x01,
        0x00, 0xFF, 0xFF, 0x25, 0x00, 0x00, 0x00
    };
    guint8 response[] = {
        0x01,
        0x45, 0x00, 0x80, 0x02, 0x01,
        0x02, 0xFF, 0xFF, 0x25, 0x00, 0x39, 0x00, 0x02,
        0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x13, 0x01,
        0x00, 0x42, 0x12, 0x0E, 0x00, 0x33, 0x35, 0x39,
        0x32, 0x32, 0x35, 0x30, 0x35, 0x30, 0x30, 0x33,
        0x39, 0x39, 0x37, 0x10, 0x08, 0x00, 0x38, 0x30,
        0x39, 0x39, 0x37, 0x38, 0x37, 0x34, 0x11, 0x0F,
        0x00, 0x33, 0x35, 0x39, 0x32, 0x32, 0x35, 0x30,
        0x35, 0x30, 0x30, 0x33, 0x39, 0x39, 0x37, 0x33
    };

    test_port_context_set_command (fixture->ctx,
                                   expected, G_N_ELEMENTS (expected),
                                   response, G_N_ELEMENTS (response),
                                   fixture->service_info[QMI_SERVICE_DMS].transaction_id++);

    qmi_client_dms_get_ids (QMI_CLIENT_DMS (fixture->service_info[QMI_SERVICE_DMS].client), NULL, 3, NULL,
                            (GAsyncReadyCallback) dms_get_ids_ready,
                            fixture);
    test_fixture_loop_run (fixture);
}

/*****************************************************************************/
/* DMS UIM Get PIN Status */

static void
dms_uim_get_pin_status_ready (QmiClientDms *client,
                              GAsyncResult *res,
                              TestFixture  *fixture)
{
    QmiMessageDmsUimGetPinStatusOutput *output;
    GError *error = NULL;
    gboolean st;
    QmiDmsUimPinStatus current_status;
    guint8 verify_retries_left;
    guint8 unblock_retries_left;

    output = qmi_client_dms_uim_get_pin_status_finish (client, res, &error);
    g_assert_no_error (error);
    g_assert (output);

    st = qmi_message_dms_uim_get_pin_status_output_get_result (output, &error);
    g_assert_no_error (error);
    g_assert (st);

    /* [/dev/cdc-wdm3] PIN1:
     *   Status: enabled-not-verified
     *   Verify: 3
     *  Unblock: 10
     * [/dev/cdc-wdm3] PIN2:
     *   Status: enabled-not-verified
     *   Verify: 2
     *  Unblock: 10
     */

    st = (qmi_message_dms_uim_get_pin_status_output_get_pin1_status (
              output,
              &current_status,
              &verify_retries_left,
              &unblock_retries_left,
              &error));
    g_assert_no_error (error);
    g_assert (st);
    g_assert_cmpuint (current_status, ==, QMI_DMS_UIM_PIN_STATUS_ENABLED_NOT_VERIFIED);
    g_assert_cmpuint (verify_retries_left, ==, 3);
    g_assert_cmpuint (unblock_retries_left, ==, 10);

    st = (qmi_message_dms_uim_get_pin_status_output_get_pin2_status (
              output,
              &current_status,
              &verify_retries_left,
              &unblock_retries_left,
              &error));
    g_assert_no_error (error);
    g_assert (st);
    g_assert_cmpuint (current_status, ==, QMI_DMS_UIM_PIN_STATUS_ENABLED_NOT_VERIFIED);
    g_assert_cmpuint (verify_retries_left, ==, 2);
    g_assert_cmpuint (unblock_retries_left, ==, 10);

    qmi_message_dms_uim_get_pin_status_output_unref (output);

    test_fixture_loop_stop (fixture);
}

static void
test_generated_dms_uim_get_pin_status (TestFixture *fixture)
{
    guint8 expected[] = {
        0x01,
        0x0C, 0x00, 0x00, 0x02, 0x01,
        0x00, 0xFF, 0xFF, 0x2B, 0x00, 0x00, 0x00
    };
    guint8 response[] = {
        0x01,
        0x1F, 0x00, 0x80, 0x02, 0x01,
        0x02, 0xFF, 0xFF, 0x2B, 0x00, 0x13, 0x00, 0x02,
        0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x12, 0x03,
        0x00, 0x01, 0x02, 0x0A, 0x11, 0x03, 0x00, 0x01,
        0x03, 0x0A
    };

    test_port_context_set_command (fixture->ctx,
                                   expected, G_N_ELEMENTS (expected),
                                   response, G_N_ELEMENTS (response),
                                   fixture->service_info[QMI_SERVICE_DMS].transaction_id++);

    qmi_client_dms_uim_get_pin_status (QMI_CLIENT_DMS (fixture->service_info[QMI_SERVICE_DMS].client), NULL, 3, NULL,
                                       (GAsyncReadyCallback) dms_uim_get_pin_status_ready,
                                       fixture);
    test_fixture_loop_run (fixture);
}

/*****************************************************************************/
/* DMS UIM Verify PIN */

static void
dms_uim_verify_pin_ready (QmiClientDms *client,
                          GAsyncResult *res,
                          TestFixture  *fixture)
{
    QmiMessageDmsUimVerifyPinOutput *output;
    GError *error = NULL;
    gboolean st;

    output = qmi_client_dms_uim_verify_pin_finish (client, res, &error);
    g_assert_no_error (error);
    g_assert (output);

    st = qmi_message_dms_uim_verify_pin_output_get_result (output, &error);
    g_assert_no_error (error);
    g_assert (st);

    qmi_message_dms_uim_verify_pin_output_unref (output);

    test_fixture_loop_stop (fixture);
}

static void
test_generated_dms_uim_verify_pin (TestFixture *fixture)
{
    QmiMessageDmsUimVerifyPinInput *input;
    gboolean st;
    GError *error = NULL;
    guint8 expected[] = {
        0x01,
        0x15, 0x00, 0x00, 0x02, 0x01,
        0x00, 0x01, 0x00, 0x28, 0x00, 0x09, 0x00, 0x01,
        0x06, 0x00, 0x01, 0x04, 0x31, 0x32, 0x33, 0x34
    };
    guint8 response[] = {
        0x01,
        0x13, 0x00, 0x80, 0x02, 0x01,
        0x02, 0xFF, 0xFF, 0x28, 0x00, 0x07, 0x00, 0x02,
        0x04, 0x00, 0x00, 0x00, 0x00, 0x00
    };

    test_port_context_set_command (fixture->ctx,
                                   expected, G_N_ELEMENTS (expected),
                                   response, G_N_ELEMENTS (response),
                                   fixture->service_info[QMI_SERVICE_DMS].transaction_id++);

    input = qmi_message_dms_uim_verify_pin_input_new ();
    st = qmi_message_dms_uim_verify_pin_input_set_info (input, QMI_DMS_UIM_PIN_ID_PIN, "1234", &error);
    g_assert_no_error (error);
    g_assert (st);

    qmi_client_dms_uim_verify_pin (QMI_CLIENT_DMS (fixture->service_info[QMI_SERVICE_DMS].client), input, 3, NULL,
                                   (GAsyncReadyCallback) dms_uim_verify_pin_ready,
                                   fixture);

    qmi_message_dms_uim_verify_pin_input_unref (input);

    test_fixture_loop_run (fixture);
}

/*****************************************************************************/
/* DMS Get Time
 *
 * Note: the original device time in the real-life binary message used had the
 * time source set to QMI_DMS_TIME_SOURCE_DEVICE. This value has been modified
 * to explicitly make the 6-byte long uint read different from an 8-byte long
 * read of the same buffer, as QMI_DMS_TIME_SOURCE_DEVICE is actually 0x0000.
 */

static void
dms_get_time_ready (QmiClientDms *client,
                    GAsyncResult *res,
                    TestFixture  *fixture)
{
    QmiMessageDmsGetTimeOutput *output;
    GError *error = NULL;
    gboolean st;
    guint64 device_time_time_count;
    QmiDmsTimeSource device_time_time_source;
    guint64 system_time;
    guint64 user_time;

    output = qmi_client_dms_get_time_finish (client, res, &error);
    g_assert_no_error (error);
    g_assert (output);

    st = qmi_message_dms_get_time_output_get_result (output, &error);
    g_assert_no_error (error);
    g_assert (st);

    st = qmi_message_dms_get_time_output_get_device_time (output, &device_time_time_count, &device_time_time_source, &error);
    g_assert_no_error (error);
    g_assert (st);
    g_assert_cmpuint (device_time_time_count, == , 884789480513ULL);
    g_assert_cmpuint (device_time_time_source, ==, QMI_DMS_TIME_SOURCE_HDR_NETWORK);

    st = qmi_message_dms_get_time_output_get_system_time (output, &system_time, &error);
    g_assert_no_error (error);
    g_assert (st);
    g_assert_cmpuint (system_time, ==, 1105986850641ULL);

    st = qmi_message_dms_get_time_output_get_user_time (output, &user_time, &error);
    g_assert_no_error (error);
    g_assert (st);
    g_assert_cmpuint (user_time, ==, 11774664);

    qmi_message_dms_get_time_output_unref (output);

    test_fixture_loop_stop (fixture);
}

static void
test_generated_dms_get_time (TestFixture *fixture)
{
    guint8 expected[] = {
        0x01,
        0x0C, 0x00, 0x00, 0x02, 0x01, 0x00, 0x01, 0x00, 0x2F, 0x00,
        0x00, 0x00
    };
    guint8 response[] = {
        0x01,
        0x34, 0x00, 0x80, 0x02, 0x01, 0x02, 0x01, 0x00, 0x2F, 0x00,
        0x28, 0x00,
        0x02, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x01, 0x08, 0x00, 0x41, 0x0C, 0x90, 0x01, 0xCE, 0x00, 0x02, 0x00, /* Note: last 0x0200 for HDR network source */
        0x10, 0x08, 0x00, 0x51, 0x0F, 0xF4, 0x81, 0x01, 0x01, 0x00, 0x00,
        0x11, 0x08, 0x00, 0xC8, 0xAA, 0xB3, 0x00, 0x00, 0x00, 0x00, 0x00
    };

    test_port_context_set_command (fixture->ctx,
                                   expected, G_N_ELEMENTS (expected),
                                   response, G_N_ELEMENTS (response),
                                   fixture->service_info[QMI_SERVICE_DMS].transaction_id++);

    qmi_client_dms_get_time (QMI_CLIENT_DMS (fixture->service_info[QMI_SERVICE_DMS].client), NULL, 3, NULL,
                             (GAsyncReadyCallback) dms_get_time_ready,
                             fixture);

    test_fixture_loop_run (fixture);
}

/*****************************************************************************/
/* NAS Network Scan */
typedef struct {
    guint16 mcc;
    guint16 mnc;
    gboolean includes_pcs_digit;
    QmiNasNetworkStatus network_status;
    QmiNasRadioInterface rat;
    const gchar *description;

} NetworkScanResult;

static NetworkScanResult scan_results[] =  {
    {
        .mcc = 214,
        .mnc = 1,
        .includes_pcs_digit = FALSE,
        .network_status = (QMI_NAS_NETWORK_STATUS_AVAILABLE |
                           QMI_NAS_NETWORK_STATUS_ROAMING |
                           QMI_NAS_NETWORK_STATUS_NOT_FORBIDDEN |
                           QMI_NAS_NETWORK_STATUS_NOT_PREFERRED),
        .rat = QMI_NAS_RADIO_INTERFACE_GSM,
        .description = "voda ES"
    },
    {
        .mcc = 214,
        .mnc = 3,
        .includes_pcs_digit = FALSE,
        .network_status = (QMI_NAS_NETWORK_STATUS_AVAILABLE |
                           QMI_NAS_NETWORK_STATUS_ROAMING |
                           QMI_NAS_NETWORK_STATUS_NOT_FORBIDDEN |
                           QMI_NAS_NETWORK_STATUS_NOT_PREFERRED),
        .rat = QMI_NAS_RADIO_INTERFACE_GSM,
        .description = "Orange"
    },
    {
        .mcc = 214,
        .mnc = 4,
        .includes_pcs_digit = FALSE,
        .network_status = (QMI_NAS_NETWORK_STATUS_AVAILABLE |
                           QMI_NAS_NETWORK_STATUS_ROAMING |
                           QMI_NAS_NETWORK_STATUS_NOT_FORBIDDEN |
                           QMI_NAS_NETWORK_STATUS_NOT_PREFERRED),
        .rat = QMI_NAS_RADIO_INTERFACE_UMTS,
        .description = "YOIGO"
    },
    {
        .mcc = 214,
        .mnc = 1,
        .includes_pcs_digit = FALSE,
        .network_status = (QMI_NAS_NETWORK_STATUS_AVAILABLE |
                           QMI_NAS_NETWORK_STATUS_ROAMING |
                           QMI_NAS_NETWORK_STATUS_NOT_FORBIDDEN |
                           QMI_NAS_NETWORK_STATUS_NOT_PREFERRED),
        .rat = QMI_NAS_RADIO_INTERFACE_UMTS,
        .description = "voda ES"
    },
    {
        .mcc = 214,
        .mnc = 4,
        .includes_pcs_digit = FALSE,
        .network_status = (QMI_NAS_NETWORK_STATUS_AVAILABLE |
                           QMI_NAS_NETWORK_STATUS_ROAMING |
                           QMI_NAS_NETWORK_STATUS_NOT_FORBIDDEN |
                           QMI_NAS_NETWORK_STATUS_NOT_PREFERRED),
        .rat = QMI_NAS_RADIO_INTERFACE_GSM,
        .description = "YOIGO"
    },
    {
        .mcc = 214,
        .mnc = 7,
        .includes_pcs_digit = FALSE,
        .network_status = (QMI_NAS_NETWORK_STATUS_AVAILABLE |
                           QMI_NAS_NETWORK_STATUS_ROAMING |
                           QMI_NAS_NETWORK_STATUS_NOT_FORBIDDEN |
                           QMI_NAS_NETWORK_STATUS_NOT_PREFERRED),
        .rat = QMI_NAS_RADIO_INTERFACE_GSM,
        .description = "Movistar"
    },
    {
        .mcc = 214,
        .mnc = 7,
        .includes_pcs_digit = FALSE,
        .network_status = (QMI_NAS_NETWORK_STATUS_AVAILABLE |
                           QMI_NAS_NETWORK_STATUS_ROAMING |
                           QMI_NAS_NETWORK_STATUS_NOT_FORBIDDEN |
                           QMI_NAS_NETWORK_STATUS_NOT_PREFERRED),
        .rat = QMI_NAS_RADIO_INTERFACE_UMTS,
        .description = "Movistar"
    },
    {
        .mcc = 214,
        .mnc = 3,
        .includes_pcs_digit = FALSE,
        .network_status = (QMI_NAS_NETWORK_STATUS_CURRENT_SERVING |
                           QMI_NAS_NETWORK_STATUS_ROAMING |
                           QMI_NAS_NETWORK_STATUS_NOT_FORBIDDEN |
                           QMI_NAS_NETWORK_STATUS_NOT_PREFERRED),
        .rat = QMI_NAS_RADIO_INTERFACE_UMTS,
        .description = ""
    },
};

static void
nas_network_scan_ready (QmiClientNas *client,
                        GAsyncResult *res,
                        TestFixture  *fixture)
{
    QmiMessageNasNetworkScanOutput *output;
    GError *error = NULL;
    gboolean st;
    GArray *network_information = NULL;
    GArray *radio_access_technology = NULL;
    GArray *mnc_pcs_digit_include_status = NULL;
    guint i;

    output = qmi_client_nas_network_scan_finish (client, res, &error);
    g_assert_no_error (error);
    g_assert (output);

    st = qmi_message_nas_network_scan_output_get_result (output, &error);
    g_assert_no_error (error);
    g_assert (st);

    st = (qmi_message_nas_network_scan_output_get_network_information (
              output,
              &network_information,
              &error));
    g_assert_no_error (error);
    g_assert (st);
    g_assert (network_information);
    g_assert_cmpuint (network_information->len, ==, 8);
    for (i = 0; i < network_information->len; i++) {
        QmiMessageNasNetworkScanOutputNetworkInformationElement *el;

        el = &g_array_index (network_information, QmiMessageNasNetworkScanOutputNetworkInformationElement, i);

        g_assert_cmpuint (el->mcc, ==, scan_results[i].mcc);
        g_assert_cmpuint (el->mnc, ==, scan_results[i].mnc);
        g_assert_cmpuint (el->network_status, ==, scan_results[i].network_status);
        g_assert_cmpstr  (el->description, ==, scan_results[i].description);
    }

    st = (qmi_message_nas_network_scan_output_get_radio_access_technology (
              output,
              &radio_access_technology,
              &error));
    g_assert_no_error (error);
    g_assert (st);
    g_assert (radio_access_technology);
    g_assert_cmpuint (radio_access_technology->len, ==, 8);
    for (i = 0; i < radio_access_technology->len; i++) {
        QmiMessageNasNetworkScanOutputRadioAccessTechnologyElement *el;

        el = &g_array_index (radio_access_technology, QmiMessageNasNetworkScanOutputRadioAccessTechnologyElement, i);

        g_assert_cmpuint (el->mcc, ==, scan_results[i].mcc);
        g_assert_cmpuint (el->mnc, ==, scan_results[i].mnc);
        g_assert_cmpuint (el->radio_interface, ==, scan_results[i].rat);
    }

    st = (qmi_message_nas_network_scan_output_get_mnc_pcs_digit_include_status (
              output,
              &mnc_pcs_digit_include_status,
              &error));
    g_assert_no_error (error);
    g_assert (st);
    g_assert (mnc_pcs_digit_include_status);
    g_assert_cmpuint (mnc_pcs_digit_include_status->len, ==, 8);
    for (i = 0; i < mnc_pcs_digit_include_status->len; i++) {
        QmiMessageNasNetworkScanOutputMncPcsDigitIncludeStatusElement *el;

        el = &g_array_index (mnc_pcs_digit_include_status, QmiMessageNasNetworkScanOutputMncPcsDigitIncludeStatusElement, i);

        g_assert_cmpuint (el->mcc, ==, scan_results[i].mcc);
        g_assert_cmpuint (el->mnc, ==, scan_results[i].mnc);
        g_assert_cmpuint (el->includes_pcs_digit, ==, scan_results[i].includes_pcs_digit);
    }

    qmi_message_nas_network_scan_output_unref (output);

    test_fixture_loop_stop (fixture);
}

static void
nas_get_cell_location_info_ready (QmiClientNas *client,
                                  GAsyncResult *res,
                                  TestFixture  *fixture)
{
    QmiMessageNasGetCellLocationInfoOutput *output;
    GError *error = NULL;
    gboolean st;

    output = qmi_client_nas_get_cell_location_info_finish (client, res, &error);
    g_assert_no_error (error);
    g_assert (output);

    st = qmi_message_nas_get_cell_location_info_output_get_result (output, &error);
    g_assert_no_error (error);
    g_assert (st);

    qmi_message_nas_get_cell_location_info_output_unref (output);

    test_fixture_loop_stop (fixture);
}

static void
test_generated_nas_network_scan (TestFixture *fixture)
{
    guint8 expected[] = {
        0x01,
        0x0C, 0x00, 0x00, 0x03, 0x01,
        0x00, 0xFF, 0xFF, 0x21, 0x00, 0x00, 0x00
    };
    guint8 response[] = {
        0x01,
        0x43, 0x01, 0x80, 0x03, 0x01,
        0x02, 0xFF, 0xFF, 0x21, 0x00, 0x37, 0x01, 0x02,
        0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x60,
        0x00, 0x08, 0x00, 0xD6, 0x00, 0x01, 0x00, 0xAA,
        0x07, 0x76, 0x6F, 0x64, 0x61, 0x20, 0x45, 0x53,
        0xD6, 0x00, 0x03, 0x00, 0xAA, 0x06, 0x4F, 0x72,
        0x61, 0x6E, 0x67, 0x65, 0xD6, 0x00, 0x04, 0x00,
        0xAA, 0x05, 0x59, 0x4F, 0x49, 0x47, 0x4F, 0xD6,
        0x00, 0x01, 0x00, 0xAA, 0x07, 0x76, 0x6F, 0x64,
        0x61, 0x20, 0x45, 0x53, 0xD6, 0x00, 0x04, 0x00,
        0xAA, 0x05, 0x59, 0x4F, 0x49, 0x47, 0x4F, 0xD6,
        0x00, 0x07, 0x00, 0xAA, 0x08, 0x4D, 0x6F, 0x76,
        0x69, 0x73, 0x74, 0x61, 0x72, 0xD6, 0x00, 0x07,
        0x00, 0xAA, 0x08, 0x4D, 0x6F, 0x76, 0x69, 0x73,
        0x74, 0x61, 0x72, 0xD6, 0x00, 0x03, 0x00, 0xA9,
        0x00, 0x11, 0x2A, 0x00, 0x08, 0x00, 0xD6, 0x00,
        0x01, 0x00, 0x04, 0xD6, 0x00, 0x03, 0x00, 0x04,
        0xD6, 0x00, 0x04, 0x00, 0x05, 0xD6, 0x00, 0x01,
        0x00, 0x05, 0xD6, 0x00, 0x04, 0x00, 0x04, 0xD6,
        0x00, 0x07, 0x00, 0x04, 0xD6, 0x00, 0x07, 0x00,
        0x05, 0xD6, 0x00, 0x03, 0x00, 0x05, 0x12, 0x2A,
        0x00, 0x08, 0x00, 0xD6, 0x00, 0x01, 0x00, 0x00,
        0xD6, 0x00, 0x03, 0x00, 0x00, 0xD6, 0x00, 0x04,
        0x00, 0x00, 0xD6, 0x00, 0x01, 0x00, 0x00, 0xD6,
        0x00, 0x04, 0x00, 0x00, 0xD6, 0x00, 0x07, 0x00,
        0x00, 0xD6, 0x00, 0x07, 0x00, 0x00, 0xD6, 0x00,
        0x03, 0x00, 0x00, 0x13, 0x04, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x14, 0x69, 0x00, 0x08, 0xD6, 0x00,
        0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0xD6, 0x00, 0x03, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0xD6, 0x00, 0x04, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xD6,
        0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };

    test_port_context_set_command (fixture->ctx,
                                   expected, G_N_ELEMENTS (expected),
                                   response, G_N_ELEMENTS (response),
                                   fixture->service_info[QMI_SERVICE_NAS].transaction_id++);

    qmi_client_nas_network_scan (QMI_CLIENT_NAS (fixture->service_info[QMI_SERVICE_NAS].client), NULL, 3, NULL,
                                 (GAsyncReadyCallback) nas_network_scan_ready,
                                 fixture);

    test_fixture_loop_run (fixture);
}

static void
test_generated_nas_get_cell_location_info (TestFixture *fixture)
{
    guint8 expected[] = {
        0x01,
        0x0C, 0x00, 0x00, 0x03, 0x01,
        0x00, 0x01, 0x00, 0x43, 0x00, 0x00, 0x00
    };
    guint8 response[] = {
        0x01,
        0x53, 0x00, 0x80, 0x03, 0x01,
        0x02, 0x01, 0x00, 0x43, 0x00, 0x47, 0x00, 0x02,
        0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x3D,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF,
        0xFF, 0x28, 0x00, 0x03, 0x7D, 0x6F, 0x00, 0x00,
        0x32, 0xF4, 0x51, 0xB3, 0x00, 0x4D, 0x00, 0x11,
        0x2A, 0x00, 0x8A, 0x3C, 0x00, 0x00, 0x32, 0xF4,
        0x51, 0xB3, 0x00, 0x63, 0x00, 0x30, 0x14, 0x00,
        0x89, 0x3C, 0x00, 0x00, 0x32, 0xF4, 0x51, 0xB3,
        0x00, 0x59, 0x00, 0x11, 0x0D, 0x00
    };

    test_port_context_set_command (fixture->ctx,
                                   expected, G_N_ELEMENTS (expected),
                                   response, G_N_ELEMENTS (response),
                                   fixture->service_info[QMI_SERVICE_NAS].transaction_id++);

    qmi_client_nas_get_cell_location_info (QMI_CLIENT_NAS (fixture->service_info[QMI_SERVICE_NAS].client), NULL, 3, NULL,
                                           (GAsyncReadyCallback) nas_get_cell_location_info_ready,
                                           fixture);

    test_fixture_loop_run (fixture);
}


/*****************************************************************************/

int main (int argc, char **argv)
{
    g_test_init (&argc, &argv, NULL);

    /* Test the setup/teardown test methods */
    TEST_ADD ("/libqmi-glib/generated/core", test_generated_core);

    /* DMS */
    TEST_ADD ("/libqmi-glib/generated/dms/get-ids",                test_generated_dms_get_ids);
    TEST_ADD ("/libqmi-glib/generated/dms/uim-get-pin-status",     test_generated_dms_uim_get_pin_status);
    TEST_ADD ("/libqmi-glib/generated/dms/uim-verify-pin",         test_generated_dms_uim_verify_pin);
    TEST_ADD ("/libqmi-glib/generated/dms/get-time",               test_generated_dms_get_time);
    /* NAS */
    TEST_ADD ("/libqmi-glib/generated/nas/network-scan",           test_generated_nas_network_scan);
    TEST_ADD ("/libqmi-glib/generated/nas/get-cell-location-info", test_generated_nas_get_cell_location_info);

    return g_test_run ();
}
