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

#if defined HAVE_QMI_MESSAGE_DMS_GET_IDS

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

#endif /* HAVE_QMI_MESSAGE_DMS_GET_IDS */

/*****************************************************************************/
/* DMS UIM Get PIN Status */

#if defined HAVE_QMI_MESSAGE_DMS_UIM_GET_PIN_STATUS

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

#endif /* HAVE_QMI_MESSAGE_DMS_UIM_GET_PIN_STATUS */

/*****************************************************************************/
/* DMS UIM Verify PIN */

#if defined HAVE_QMI_MESSAGE_DMS_UIM_VERIFY_PIN

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

#endif /* HAVE_QMI_MESSAGE_DMS_UIM_VERIFY_PIN */

/*****************************************************************************/
/* DMS Get Time
 *
 * Note: the original device time in the real-life binary message used had the
 * time source set to QMI_DMS_TIME_SOURCE_DEVICE. This value has been modified
 * to explicitly make the 6-byte long uint read different from an 8-byte long
 * read of the same buffer, as QMI_DMS_TIME_SOURCE_DEVICE is actually 0x0000.
 */

#if defined HAVE_QMI_MESSAGE_DMS_GET_TIME

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

#endif /* HAVE_QMI_MESSAGE_DMS_GET_TIME */

/*****************************************************************************/
/* NAS Network Scan */

#if defined HAVE_QMI_MESSAGE_NAS_NETWORK_SCAN

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

#endif /* HAVE_QMI_MESSAGE_NAS_NETWORK_SCAN */

/*****************************************************************************/
/* NAS Get Cell Location */

#if defined HAVE_QMI_MESSAGE_NAS_GET_CELL_LOCATION_INFO

static void
nas_get_cell_location_info_1_ready (QmiClientNas *client,
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
test_generated_nas_get_cell_location_info_1 (TestFixture *fixture)
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
                                           (GAsyncReadyCallback) nas_get_cell_location_info_1_ready,
                                           fixture);

    test_fixture_loop_run (fixture);
}

static void
nas_get_cell_location_info_2_ready (QmiClientNas *client,
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

    {
        gboolean  value_intrafrequency_lte_info_ue_in_idle = FALSE;
        GArray   *value_intrafrequency_lte_info_plmn = NULL;
        guint16   value_intrafrequency_lte_info_tracking_area_code = 0;
        guint32   value_intrafrequency_lte_info_global_cell_id = 0;
        guint16   value_intrafrequency_lte_info_eutra_absolute_rf_channel_number = 0;
        guint16   value_intrafrequency_lte_info_serving_cell_id = 0;
        guint8    value_intrafrequency_lte_info_cell_reselection_priority = 0;
        guint8    value_intrafrequency_lte_info_s_non_intra_search_threshold = 0;
        guint8    value_intrafrequency_lte_info_serving_cell_low_threshold = 0;
        guint8    value_intrafrequency_lte_info_s_intra_search_threshold = 0;
        GArray   *value_intrafrequency_lte_info_cell = NULL;

        st = qmi_message_nas_get_cell_location_info_output_get_intrafrequency_lte_info_v2 (output,
                                                                                           &value_intrafrequency_lte_info_ue_in_idle,
                                                                                           &value_intrafrequency_lte_info_plmn,
                                                                                           &value_intrafrequency_lte_info_tracking_area_code,
                                                                                           &value_intrafrequency_lte_info_global_cell_id,
                                                                                           &value_intrafrequency_lte_info_eutra_absolute_rf_channel_number,
                                                                                           &value_intrafrequency_lte_info_serving_cell_id,
                                                                                           &value_intrafrequency_lte_info_cell_reselection_priority,
                                                                                           &value_intrafrequency_lte_info_s_non_intra_search_threshold,
                                                                                           &value_intrafrequency_lte_info_serving_cell_low_threshold,
                                                                                           &value_intrafrequency_lte_info_s_intra_search_threshold,
                                                                                           &value_intrafrequency_lte_info_cell,
                                                                                           &error);
        g_assert_no_error (error);
        g_assert (st);
    }

    qmi_message_nas_get_cell_location_info_output_unref (output);

    test_fixture_loop_stop (fixture);
}

static void
test_generated_nas_get_cell_location_info_2 (TestFixture *fixture)
{
    guint8 expected[] = {
        0x01,
        0x0C, 0x00, 0x00, 0x03,
        0x01, 0x00, 0x01, 0x00, 0x43, 0x00, 0x00, 0x00,
    };
    guint8 response[] = {
        0x01,
        0x67, 0x00, 0x80, 0x03, 0x01,
        0x02, 0x01, 0x00, 0x43, 0x00, 0x5B, 0x00, 0x02,
        0x04, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x13, 0x1D, 0x00,
            0x01, 0x99, 0xF9, 0x04, 0x99, 0x00, 0x01,
            0xC2, 0x01, 0x00, 0x7E, 0xA9, 0x00, 0x00, 0x01,
            0x3E, 0x28, 0x3E, 0x01, 0x00, 0x00, 0xBD, 0xFF,
            0x19, 0xFC, 0x23, 0xFD, 0x1E, 0x00,
        0x14, 0x02, 0x00,
            0x01, 0x00,
        0x15, 0x02, 0x00,
            0x01, 0x00,
        0x16, 0x02, 0x00,
            0x01, 0x00,
        0x1E, 0x04, 0x00,
            0xFF, 0xFF, 0xFF, 0xFF,
        0x26, 0x02, 0x00,
            0x46, 0x00,
        0x27, 0x04, 0x00,
            0x7E, 0xA9, 0x00, 0x00,
        0x28, 0x01, 0x00,
            0x00,
        0x2A, 0x04, 0x00,
            0x03, 0x00, 0x00, 0x00,
        0x2C, 0x04, 0x00,
            0x00, 0x00, 0x00, 0x00
    };

    test_port_context_set_command (fixture->ctx,
                                   expected, G_N_ELEMENTS (expected),
                                   response, G_N_ELEMENTS (response),
                                   fixture->service_info[QMI_SERVICE_NAS].transaction_id++);

    qmi_client_nas_get_cell_location_info (QMI_CLIENT_NAS (fixture->service_info[QMI_SERVICE_NAS].client), NULL, 3, NULL,
                                           (GAsyncReadyCallback) nas_get_cell_location_info_2_ready,
                                           fixture);

    test_fixture_loop_run (fixture);
}

static void
nas_get_cell_location_info_invalid_ready (QmiClientNas *client,
                                          GAsyncResult *res,
                                          TestFixture  *fixture)
{
    QmiMessageNasGetCellLocationInfoOutput *output;
    GError *error = NULL;

    output = qmi_client_nas_get_cell_location_info_finish (client, res, &error);
    g_assert_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_UNEXPECTED_MESSAGE);
    g_assert (!output);

    test_fixture_loop_stop (fixture);
}

static void
test_generated_nas_get_cell_location_info_invalid (TestFixture *fixture)
{
    guint8 expected[] = {
        0x01,
        0x0C, 0x00, 0x00, 0x03, 0x01,
        0x00, 0x01, 0x00, 0x43, 0x00, 0x00, 0x00
    };
    guint8 response[] = {
        0x01,
        0x53, 0x00, 0x80, 0x03, 0x01,
        0x02, 0x01, 0x00, 0x44, 0x00, 0x47, 0x00, 0x02, /* command id set to 0x0044 instead of 0x0043 */
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
                                           (GAsyncReadyCallback) nas_get_cell_location_info_invalid_ready,
                                           fixture);

    test_fixture_loop_run (fixture);
}

#endif /* HAVE_QMI_MESSAGE_NAS_GET_CELL_LOCATION_INFO */

/*****************************************************************************/
/* NAS Get Serving System */

#if defined HAVE_QMI_MESSAGE_NAS_GET_SERVING_SYSTEM

static void
nas_get_serving_system_ready (QmiClientNas *client,
                              GAsyncResult *res,
                              TestFixture  *fixture)
{
    QmiMessageNasGetServingSystemOutput *output;
    GError *error = NULL;
    gboolean st;

    output = qmi_client_nas_get_serving_system_finish (client, res, &error);
    g_assert_no_error (error);
    g_assert (output);

    st = qmi_message_nas_get_serving_system_output_get_result (output, &error);
    g_assert_no_error (error);
    g_assert (st);

    /* current plmn in GSM-7 instead of UTF-8:
     *   <<<<<< TLV:
     *   <<<<<<   type       = "Current PLMN" (0x12)
     *   <<<<<<   length     = 10
     *   <<<<<<   value      = DE:00:32:00:05:49:76:3A:4C:06
     *   <<<<<<   translated = [ mcc = '222' mnc = '50' description = 'Iliad' ]
     */
    {
        guint16 mcc;
        guint16 mnc;
        const gchar *description;

        g_assert (qmi_message_nas_get_serving_system_output_get_current_plmn (output, &mcc, &mnc, &description, &error));
        g_assert_no_error (error);
        g_assert_cmpuint (mcc, ==, 222);
        g_assert_cmpuint (mnc, ==, 50);
        g_assert_cmpstr (description, ==, "Iliad");
    }

    qmi_message_nas_get_serving_system_output_unref (output);

    test_fixture_loop_stop (fixture);
}

static void
test_generated_nas_get_serving_system (TestFixture *fixture)
{
    guint8 expected[] = {
        0x01,
        0x0C, 0x00, 0x00, 0x03, 0x01,
        0x00, 0x01, 0x00, 0x24, 0x00, 0x00, 0x00
    };
    guint8 response[] = {
        0x01,
        0x6E, 0x00, 0x80, 0x03, 0x01,
        0x02, 0x01, 0x00, 0x24, 0x00, 0x62, 0x00, 0x02,
        0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x06,
        0x00, 0x01, 0x01, 0x01, 0x02, 0x01, 0x05, 0x10,
        0x01, 0x00, 0x01, 0x11, 0x04, 0x00, 0x03, 0x03,
        0x04, 0x05, 0x12, 0x0A, 0x00, 0xDE, 0x00, 0x32,
        0x00, 0x05, 0x49, 0x76, 0x3A, 0x4C, 0x06, 0x15,
        0x03, 0x00, 0x01, 0x05, 0x01, 0x1B, 0x01, 0x00,
        0x00, 0x1C, 0x02, 0x00, 0xB4, 0x5F, 0x1D, 0x04,
        0x00, 0xCF, 0x5A, 0x13, 0x01, 0x21, 0x05, 0x00,
        0x02, 0x03, 0x00, 0x00, 0x00, 0x25, 0x08, 0x00,
        0x03, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
        0x26, 0x02, 0x00, 0x22, 0x01, 0x27, 0x05, 0x00,
        0xDE, 0x00, 0x32, 0x00, 0x00, 0x28, 0x01, 0x00,
        0x00
    };

    test_port_context_set_command (fixture->ctx,
                                   expected, G_N_ELEMENTS (expected),
                                   response, G_N_ELEMENTS (response),
                                   fixture->service_info[QMI_SERVICE_NAS].transaction_id++);

    qmi_client_nas_get_serving_system (QMI_CLIENT_NAS (fixture->service_info[QMI_SERVICE_NAS].client), NULL, 3, NULL,
                                       (GAsyncReadyCallback) nas_get_serving_system_ready,
                                       fixture);

    test_fixture_loop_run (fixture);
}

#endif /* HAVE_QMI_MESSAGE_NAS_GET_SERVING_SYSTEM */

/*****************************************************************************/
/* NAS Get System Info */

#if defined HAVE_QMI_MESSAGE_NAS_GET_SYSTEM_INFO

static void
nas_get_system_info_ready (QmiClientNas *client,
                           GAsyncResult *res,
                           TestFixture  *fixture)
{
    QmiMessageNasGetSystemInfoOutput *output;
    GError *error = NULL;
    gboolean st;

    output = qmi_client_nas_get_system_info_finish (client, res, &error);
    g_assert_no_error (error);
    g_assert (output);

    st = qmi_message_nas_get_system_info_output_get_result (output, &error);
    g_assert_no_error (error);
    g_assert (st);

    {
        gboolean                    domain_valid = FALSE;
        QmiNasNetworkServiceDomain  domain;
        gboolean                    service_capability_valid = FALSE;
        QmiNasNetworkServiceDomain  service_capability;
        gboolean                    roaming_status_valid = FALSE;
        QmiNasRoamingStatus         roaming_status;
        gboolean                    forbidden_valid = FALSE;
        gboolean                    forbidden;
        gboolean                    lac_valid = FALSE;
        guint16                     lac;
        gboolean                    cid_valid = FALSE;
        guint32                     cid;
        gboolean                    registration_reject_info_valid = FALSE;
        QmiNasNetworkServiceDomain  registration_reject_domain;
        QmiNasRejectCause           registration_reject_cause;
        gboolean                    network_id_valid = FALSE;
        const gchar                *mcc = NULL;
        const gchar                *mnc = NULL;
        gboolean                    tac_valid = FALSE;
        guint16                     tac;

        /*
         *	LTE service:
         *		Status: 'available'
         *		True Status: 'available'
         *		Preferred data path: 'no'
         *		Domain: 'cs-ps'
         *		Service capability: 'cs-ps'
         *		Roaming status: 'off'
         *		Forbidden: 'no'
         *		Cell ID: '1609474'
         *		MCC: '530'
         *		MNC: '24'                -- Given as 2 digits, suffixed with 0xFF!
         *		Tracking Area Code: '63001'
         *		Voice support: 'yes'
         *		IMS voice support: 'no'
         *		eMBMS coverage info support: 'no'
         *		eMBMS coverage info trace ID: '65535'
         *		Cell access: 'all-calls'
         *		Registration restriction: 'unrestricted'
         *		Registration domain: 'not-applicable'
         */

        g_assert (qmi_message_nas_get_system_info_output_get_lte_system_info_v2 (
                      output,
                      &domain_valid,
                      &domain,
                      &service_capability_valid,
                      &service_capability,
                      &roaming_status_valid,
                      &roaming_status,
                      &forbidden_valid,
                      &forbidden,
                      &lac_valid,
                      &lac,
                      &cid_valid,
                      &cid,
                      &registration_reject_info_valid,
                      &registration_reject_domain,
                      &registration_reject_cause,
                      &network_id_valid,
                      &mcc,
                      &mnc,
                      &tac_valid,
                      &tac,
                      &error));
        g_assert_no_error (error);
        g_assert (domain_valid);
        g_assert_cmpuint (domain, ==, QMI_NAS_NETWORK_SERVICE_DOMAIN_CS_PS);
        g_assert (service_capability_valid);
        g_assert_cmpuint (service_capability, ==, QMI_NAS_NETWORK_SERVICE_DOMAIN_CS_PS);
        g_assert (roaming_status_valid);
        g_assert_cmpuint (roaming_status, ==, QMI_NAS_ROAMING_STATUS_OFF);
        g_assert (forbidden_valid);
        g_assert (!forbidden);
        g_assert (!lac_valid);
        g_assert (cid_valid);
        g_assert_cmpuint (cid, ==, 1616133);
        g_assert (!registration_reject_info_valid);
        g_assert (network_id_valid);
        g_assert_cmpstr (mcc, ==, "530");
        g_assert_cmpstr (mnc, ==, "24");
        g_assert (tac_valid);
        g_assert_cmpuint (tac, ==, 63001);
    }

    qmi_message_nas_get_system_info_output_unref (output);

    test_fixture_loop_stop (fixture);
}

static void
test_generated_nas_get_system_info (TestFixture *fixture)
{
    guint8 expected[] = {
        0x01,
        0x0C, 0x00, 0x00, 0x03, 0x01,
        0x00, 0x01, 0x00, 0x4D, 0x00, 0x00, 0x00
    };
    guint8 response[] = {
        0x01,
        0x9A, 0x00, 0x80, 0x03, 0x01,
        0x02, 0x01, 0x00, 0x4D, 0x00, 0x8E, 0x00, 0x02,
        0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x12, 0x03,
        0x00, 0x00, 0x00, 0x00, 0x13, 0x03, 0x00, 0x00,
        0x00, 0x00, 0x14, 0x03, 0x00, 0x02, 0x02, 0x00,
        0x19, 0x1D, 0x00, 0x01, 0x03, 0x01, 0x03, 0x01,
        0x00, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0x01, 0x05,
        0xA9, 0x18, 0x00, 0x00, 0x00, 0x00, 0x01, 0x35,
        0x33, 0x30, 0x32, 0x34, 0xFF, 0x01, 0x19, 0xF6,
        0x1E, 0x02, 0x00, 0xFF, 0xFF, 0x21, 0x01, 0x00,
        0x01, 0x26, 0x01, 0x00, 0x00, 0x27, 0x04, 0x00,
        0x01, 0x00, 0x00, 0x00, 0x29, 0x01, 0x00, 0x00,
        0x2A, 0x04, 0x00, 0x03, 0x00, 0x00, 0x00, 0x2F,
        0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x31, 0x04,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x34, 0x02, 0x00,
        0xFF, 0xFF, 0x38, 0x04, 0x00, 0x03, 0x00, 0x00,
        0x00, 0x39, 0x04, 0x00, 0x01, 0x00, 0x00, 0x00,
        0x3E, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x44,
        0x04, 0x00, 0x03, 0x00, 0x00, 0x00, 0x46, 0x04,
        0x00, 0x04, 0x00, 0x00, 0x00
    };

    test_port_context_set_command (fixture->ctx,
                                   expected, G_N_ELEMENTS (expected),
                                   response, G_N_ELEMENTS (response),
                                   fixture->service_info[QMI_SERVICE_NAS].transaction_id++);

    qmi_client_nas_get_system_info (QMI_CLIENT_NAS (fixture->service_info[QMI_SERVICE_NAS].client), NULL, 3, NULL,
                                    (GAsyncReadyCallback) nas_get_system_info_ready,
                                    fixture);

    test_fixture_loop_run (fixture);
}

#endif

/*****************************************************************************/

int main (int argc, char **argv)
{
    g_test_init (&argc, &argv, NULL);

    /* Test the setup/teardown test methods */
    TEST_ADD ("/libqmi-glib/generated/core", test_generated_core);

#if defined HAVE_QMI_MESSAGE_DMS_GET_IDS
    TEST_ADD ("/libqmi-glib/generated/dms/get-ids", test_generated_dms_get_ids);
#endif
#if defined HAVE_QMI_MESSAGE_DMS_UIM_GET_PIN_STATUS
    TEST_ADD ("/libqmi-glib/generated/dms/uim-get-pin-status", test_generated_dms_uim_get_pin_status);
#endif
#if defined HAVE_QMI_MESSAGE_DMS_UIM_VERIFY_PIN
    TEST_ADD ("/libqmi-glib/generated/dms/uim-verify-pin", test_generated_dms_uim_verify_pin);
#endif
#if defined HAVE_QMI_MESSAGE_DMS_GET_TIME
    TEST_ADD ("/libqmi-glib/generated/dms/get-time", test_generated_dms_get_time);
#endif

#if defined HAVE_QMI_MESSAGE_NAS_NETWORK_SCAN
    TEST_ADD ("/libqmi-glib/generated/nas/network-scan", test_generated_nas_network_scan);
#endif
#if defined HAVE_QMI_MESSAGE_NAS_GET_CELL_LOCATION_INFO
    TEST_ADD ("/libqmi-glib/generated/nas/get-cell-location-info/1",       test_generated_nas_get_cell_location_info_1);
    TEST_ADD ("/libqmi-glib/generated/nas/get-cell-location-info/2",       test_generated_nas_get_cell_location_info_2);
    TEST_ADD ("/libqmi-glib/generated/nas/get-cell-location-info/invalid", test_generated_nas_get_cell_location_info_invalid);
#endif
#if defined HAVE_QMI_MESSAGE_NAS_GET_SERVING_SYSTEM
    TEST_ADD ("/libqmi-glib/generated/nas/get-serving-system", test_generated_nas_get_serving_system);
#endif
#if defined HAVE_QMI_MESSAGE_NAS_GET_SYSTEM_INFO
    TEST_ADD ("/libqmi-glib/generated/nas/get-system-info", test_generated_nas_get_system_info);
#endif

    return g_test_run ();
}
