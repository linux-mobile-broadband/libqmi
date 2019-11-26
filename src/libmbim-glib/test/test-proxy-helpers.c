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
#include <string.h>

#include "mbim-cid.h"
#include "mbim-uuid.h"
#include "mbim-basic-connect.h"
#include "mbim-proxy-helpers.h"

/*****************************************************************************/

static void
test_parse_single_service_0_cids (void)
{
    MbimMessage *message;
    MbimEventEntry **in;
    MbimEventEntry **out;
    GError *error = NULL;
    gsize out_size = 0;

    in = g_new0 (MbimEventEntry *, 2);
    in[0] = g_new0 (MbimEventEntry, 1);
    memcpy (&in[0]->device_service_id, MBIM_UUID_BASIC_CONNECT, sizeof (MbimUuid));
    in[0]->cids_count = 0;
    in[0]->cids = NULL;

    message = mbim_message_device_service_subscribe_list_set_new (1, (const MbimEventEntry *const *)in, &error);
    g_assert_no_error (error);
    g_assert (message != NULL);

    out = _mbim_proxy_helper_service_subscribe_request_parse (message, &out_size, &error);
    g_assert_no_error (error);
    g_assert (out != NULL);
    g_assert (_mbim_proxy_helper_service_subscribe_list_cmp ((const MbimEventEntry * const *)in, 1,
                                                             (const MbimEventEntry * const *)out, out_size));
    g_assert_cmpuint (out_size, ==, 1);

    mbim_message_unref (message);
    mbim_event_entry_array_free (in);
    mbim_event_entry_array_free (out);
}

static void
test_parse_single_service_1_cids (void)
{
    MbimMessage *message;
    MbimEventEntry **in;
    MbimEventEntry **out;
    GError *error = NULL;
    gsize out_size = 0;

    in = g_new0 (MbimEventEntry *, 2);
    in[0] = g_new0 (MbimEventEntry, 1);
    memcpy (&in[0]->device_service_id, MBIM_UUID_BASIC_CONNECT, sizeof (MbimUuid));
    in[0]->cids_count = 1;
    in[0]->cids = g_new0 (guint32, in[0]->cids_count);
    in[0]->cids[0] = MBIM_CID_BASIC_CONNECT_SUBSCRIBER_READY_STATUS;

    message = mbim_message_device_service_subscribe_list_set_new (1, (const MbimEventEntry *const *)in, &error);
    g_assert_no_error (error);
    g_assert (message != NULL);

    out = _mbim_proxy_helper_service_subscribe_request_parse (message, &out_size, &error);
    g_assert_no_error (error);
    g_assert (out != NULL);
    g_assert (_mbim_proxy_helper_service_subscribe_list_cmp ((const MbimEventEntry * const *)in, 1,
                                                             (const MbimEventEntry * const *)out, out_size));
    g_assert_cmpuint (out_size, ==, 1);

    mbim_message_unref (message);
    mbim_event_entry_array_free (in);
    mbim_event_entry_array_free (out);
}

static void
test_parse_single_service_5_cids (void)
{
    MbimMessage *message;
    MbimEventEntry **in;
    MbimEventEntry **out;
    GError *error = NULL;
    gsize out_size = 0;

    in = g_new0 (MbimEventEntry *, 2);
    in[0] = g_new0 (MbimEventEntry, 1);
    memcpy (&in[0]->device_service_id, MBIM_UUID_BASIC_CONNECT, sizeof (MbimUuid));
    in[0]->cids_count = 5;
    in[0]->cids = g_new0 (guint32, in[0]->cids_count);
    in[0]->cids[0] = MBIM_CID_BASIC_CONNECT_SUBSCRIBER_READY_STATUS;
    in[0]->cids[1] = MBIM_CID_BASIC_CONNECT_RADIO_STATE;
    in[0]->cids[2] = MBIM_CID_BASIC_CONNECT_SIGNAL_STATE;
    in[0]->cids[3] = MBIM_CID_BASIC_CONNECT_IP_CONFIGURATION;
    in[0]->cids[4] = MBIM_CID_BASIC_CONNECT_NETWORK_IDLE_HINT;

    message = mbim_message_device_service_subscribe_list_set_new (1, (const MbimEventEntry *const *)in, &error);
    g_assert_no_error (error);
    g_assert (message != NULL);

    out = _mbim_proxy_helper_service_subscribe_request_parse (message, &out_size, &error);
    g_assert_no_error (error);
    g_assert (out != NULL);
    g_assert (_mbim_proxy_helper_service_subscribe_list_cmp ((const MbimEventEntry * const *)in, 1,
                                                             (const MbimEventEntry * const *)out, out_size));
    g_assert_cmpuint (out_size, ==, 1);

    mbim_message_unref (message);
    mbim_event_entry_array_free (in);
    mbim_event_entry_array_free (out);
}

/*****************************************************************************/

static void
test_merge_none (void)
{
    MbimEventEntry **list = NULL;
    gsize list_size = 0;
    gsize out_size = 0;

    /* merge */
    list = _mbim_proxy_helper_service_subscribe_list_merge (list, list_size, NULL, 0, &out_size);

    g_assert (list == NULL);
    g_assert_cmpuint (out_size, ==, 0);
}

static void
test_merge_standard_services (void)
{
    MbimEventEntry **list = NULL;
    gsize list_size = 0;
    MbimEventEntry **addition;
    gsize addition_size;
    gsize out_size = 0;

    /* setup a new list with a subset of standard services */
    addition_size = 2;
    addition = g_new0 (MbimEventEntry *, addition_size + 1);
    addition[0] = g_new0 (MbimEventEntry, 1);
    memcpy (&addition[0]->device_service_id, MBIM_UUID_BASIC_CONNECT, sizeof (MbimUuid));
    addition[0]->cids_count = 5;
    addition[0]->cids = g_new0 (guint32, addition[0]->cids_count);
    addition[0]->cids[0] = MBIM_CID_BASIC_CONNECT_SUBSCRIBER_READY_STATUS;
    addition[0]->cids[1] = MBIM_CID_BASIC_CONNECT_RADIO_STATE;
    addition[0]->cids[2] = MBIM_CID_BASIC_CONNECT_SIGNAL_STATE;
    addition[0]->cids[3] = MBIM_CID_BASIC_CONNECT_IP_CONFIGURATION;
    addition[0]->cids[4] = MBIM_CID_BASIC_CONNECT_NETWORK_IDLE_HINT;
    addition[1] = g_new0 (MbimEventEntry, 1);
    memcpy (&addition[1]->device_service_id, MBIM_UUID_SMS, sizeof (MbimUuid));
    addition[1]->cids_count = 2;
    addition[1]->cids = g_new0 (guint32, addition[1]->cids_count);
    addition[1]->cids[0] = MBIM_CID_SMS_READ;
    addition[1]->cids[1] = MBIM_CID_SMS_SEND;

    /* merge */
    list = _mbim_proxy_helper_service_subscribe_list_merge (list, list_size, addition, addition_size, &out_size);

    /* The new list should be empty, as standard services are ignored */
    g_assert (list == NULL);
    g_assert_cmpuint (out_size, ==, 0);

    mbim_event_entry_array_free (addition);
}

static void
test_merge_other_services (void)
{
    MbimEventEntry **list = NULL;
    gsize list_size = 0;
    MbimEventEntry **addition;
    gsize addition_size;
    gsize out_size = 0;

    /* setup a new list with a subset of other services */
    addition_size = 2;
    addition = g_new0 (MbimEventEntry *, addition_size + 1);
    addition[0] = g_new0 (MbimEventEntry, 1);
    memcpy (&addition[0]->device_service_id, MBIM_UUID_ATDS, sizeof (MbimUuid));
    addition[0]->cids_count = 5;
    addition[0]->cids = g_new0 (guint32, addition[0]->cids_count);
    addition[0]->cids[0] = MBIM_CID_ATDS_SIGNAL;
    addition[0]->cids[1] = MBIM_CID_ATDS_LOCATION;
    addition[0]->cids[2] = MBIM_CID_ATDS_OPERATORS;
    addition[0]->cids[3] = MBIM_CID_ATDS_RAT;
    addition[0]->cids[4] = MBIM_CID_ATDS_REGISTER_STATE;
    addition[1] = g_new0 (MbimEventEntry, 1);
    memcpy (&addition[1]->device_service_id, MBIM_UUID_QMI, sizeof (MbimUuid));
    addition[1]->cids_count = 1;
    addition[1]->cids = g_new0 (guint32, addition[1]->cids_count);
    addition[1]->cids[0] = MBIM_CID_QMI_MSG;

    /* merge */
    list = _mbim_proxy_helper_service_subscribe_list_merge (list, list_size, addition, addition_size, &out_size);

    /* The new list should be totally equal to the addition, as the original list was empty. */
    g_assert (_mbim_proxy_helper_service_subscribe_list_cmp ((const MbimEventEntry * const *)list, out_size,
                                                             (const MbimEventEntry * const *)addition, addition_size));

    mbim_event_entry_array_free (list);
    mbim_event_entry_array_free (addition);
}

static void
test_merge_list_same_service (void)
{
    MbimEventEntry **list;
    gsize list_size;
    MbimEventEntry **addition;
    gsize addition_size;
    MbimEventEntry **expected;
    gsize expected_size;
    gsize out_size = 0;

    /* setup a new list with a subset of non-standard services */
    list_size = 1;
    list = g_new0 (MbimEventEntry *, list_size + 1);
    list[0] = g_new0 (MbimEventEntry, 1);
    memcpy (&list[0]->device_service_id, MBIM_UUID_ATDS, sizeof (MbimUuid));
    list[0]->cids_count = 2;
    list[0]->cids = g_new0 (guint32, list[0]->cids_count);
    list[0]->cids[0] = MBIM_CID_ATDS_SIGNAL;
    list[0]->cids[1] = MBIM_CID_ATDS_LOCATION;

    /* setup a new list with a subset of non-standard services */
    addition_size = 1;
    addition = g_new0 (MbimEventEntry *, addition_size + 1);
    addition[0] = g_new0 (MbimEventEntry, 1);
    memcpy (&addition[0]->device_service_id, MBIM_UUID_ATDS, sizeof (MbimUuid));
    addition[0]->cids_count = 3;
    addition[0]->cids = g_new0 (guint32, addition[0]->cids_count);
    addition[0]->cids[0] = MBIM_CID_ATDS_OPERATORS;
    addition[0]->cids[1] = MBIM_CID_ATDS_RAT;
    addition[0]->cids[2] = MBIM_CID_ATDS_REGISTER_STATE;

    /* merge */
    list = _mbim_proxy_helper_service_subscribe_list_merge (list, list_size, addition, addition_size, &out_size);

    /* setup the expected list */
    expected_size = 1;
    expected = g_new0 (MbimEventEntry *, expected_size + 1);
    expected[0] = g_new0 (MbimEventEntry, 1);
    memcpy (&expected[0]->device_service_id, MBIM_UUID_ATDS, sizeof (MbimUuid));
    expected[0]->cids_count = 5;
    expected[0]->cids = g_new0 (guint32, expected[0]->cids_count);
    expected[0]->cids[0] = MBIM_CID_ATDS_SIGNAL;
    expected[0]->cids[1] = MBIM_CID_ATDS_LOCATION;
    expected[0]->cids[2] = MBIM_CID_ATDS_OPERATORS;
    expected[0]->cids[3] = MBIM_CID_ATDS_RAT;
    expected[0]->cids[4] = MBIM_CID_ATDS_REGISTER_STATE;

    /* Compare */
    g_assert (_mbim_proxy_helper_service_subscribe_list_cmp ((const MbimEventEntry * const *)list, out_size,
                                                             (const MbimEventEntry * const *)expected, expected_size));

    mbim_event_entry_array_free (list);
    mbim_event_entry_array_free (addition);
    mbim_event_entry_array_free (expected);
}

static void
test_merge_list_different_services (void)
{
    MbimEventEntry **list;
    gsize list_size;
    MbimEventEntry **addition;
    gsize addition_size;
    MbimEventEntry **expected;
    gsize expected_size;
    gsize out_size = 0;

    /* setup a new list with a subset of non-standard services */
    list_size = 1;
    list = g_new0 (MbimEventEntry *, list_size + 1);
    list[0] = g_new0 (MbimEventEntry, 1);
    memcpy (&list[0]->device_service_id, MBIM_UUID_ATDS, sizeof (MbimUuid));
    list[0]->cids_count = 2;
    list[0]->cids = g_new0 (guint32, list[0]->cids_count);
    list[0]->cids[0] = MBIM_CID_ATDS_SIGNAL;
    list[0]->cids[1] = MBIM_CID_ATDS_LOCATION;

    /* setup a new list with a subset of non-standard services */
    addition_size = 1;
    addition = g_new0 (MbimEventEntry *, addition_size + 1);
    addition[0] = g_new0 (MbimEventEntry, 1);
    memcpy (&addition[0]->device_service_id, MBIM_UUID_QMI, sizeof (MbimUuid));
    addition[0]->cids_count = 1;
    addition[0]->cids = g_new0 (guint32, addition[0]->cids_count);
    addition[0]->cids[0] = MBIM_CID_QMI_MSG;

    /* merge */
    list = _mbim_proxy_helper_service_subscribe_list_merge (list, list_size, addition, addition_size, &out_size);

    /* setup the expected list */
    expected_size = 2;
    expected = g_new0 (MbimEventEntry *, expected_size + 1);
    expected[0] = g_new0 (MbimEventEntry, 1);
    memcpy (&expected[0]->device_service_id, MBIM_UUID_ATDS, sizeof (MbimUuid));
    expected[0]->cids_count = 2;
    expected[0]->cids = g_new0 (guint32, expected[0]->cids_count);
    expected[0]->cids[0] = MBIM_CID_ATDS_SIGNAL;
    expected[0]->cids[1] = MBIM_CID_ATDS_LOCATION;
    expected[1] = g_new0 (MbimEventEntry, 1);
    memcpy (&expected[1]->device_service_id, MBIM_UUID_QMI, sizeof (MbimUuid));
    expected[1]->cids_count = 1;
    expected[1]->cids = g_new0 (guint32, expected[1]->cids_count);
    expected[1]->cids[0] = MBIM_CID_QMI_MSG;

    /* Compare */
    g_assert (_mbim_proxy_helper_service_subscribe_list_cmp ((const MbimEventEntry * const *)list, out_size,
                                                             (const MbimEventEntry * const *)expected, expected_size));

    mbim_event_entry_array_free (list);
    mbim_event_entry_array_free (addition);
    mbim_event_entry_array_free (expected);
}

static void
test_merge_list_merged_services (void)
{
    MbimEventEntry **list;
    gsize list_size;
    MbimEventEntry **addition;
    gsize addition_size;
    MbimEventEntry **expected;
    gsize expected_size;
    gsize out_size = 0;

    /* setup a new list with a subset of non-standard services */
    list_size = 2;
    list = g_new0 (MbimEventEntry *, list_size + 1);
    list[0] = g_new0 (MbimEventEntry, 1);
    memcpy (&list[0]->device_service_id, MBIM_UUID_ATDS, sizeof (MbimUuid));
    list[0]->cids_count = 3;
    list[0]->cids = g_new0 (guint32, list[0]->cids_count);
    list[0]->cids[0] = MBIM_CID_ATDS_SIGNAL;
    list[0]->cids[1] = MBIM_CID_ATDS_LOCATION;
    list[0]->cids[2] = MBIM_CID_ATDS_OPERATORS;
    list[1] = g_new0 (MbimEventEntry, 1);
    memcpy (&list[1]->device_service_id, MBIM_UUID_QMI, sizeof (MbimUuid));
    list[1]->cids_count = 1;
    list[1]->cids = g_new0 (guint32, list[1]->cids_count);
    list[1]->cids[0] = MBIM_CID_QMI_MSG;

    /* setup a new list with a subset of standard and non-standard services */
    addition_size = 2;
    addition = g_new0 (MbimEventEntry *, addition_size + 1);
    addition[0] = g_new0 (MbimEventEntry, 1);
    memcpy (&addition[0]->device_service_id, MBIM_UUID_ATDS, sizeof (MbimUuid));
    addition[0]->cids_count = 1;
    addition[0]->cids = g_new0 (guint32, addition[0]->cids_count);
    addition[0]->cids[0] = MBIM_CID_ATDS_RAT;
    addition[1] = g_new0 (MbimEventEntry, 1);
    memcpy (&addition[1]->device_service_id, MBIM_UUID_MS_HOST_SHUTDOWN, sizeof (MbimUuid));
    addition[1]->cids_count = 1;
    addition[1]->cids = g_new0 (guint32, addition[1]->cids_count);
    addition[1]->cids[0] = MBIM_CID_MS_HOST_SHUTDOWN_NOTIFY;

    /* merge */
    list = _mbim_proxy_helper_service_subscribe_list_merge (list, list_size, addition, addition_size, &out_size);

    /* setup the expected list */
    expected_size = 3;
    expected = g_new0 (MbimEventEntry *, expected_size + 1);
    expected[0] = g_new0 (MbimEventEntry, 1);
    memcpy (&expected[0]->device_service_id, MBIM_UUID_ATDS, sizeof (MbimUuid));
    expected[0]->cids_count = 4;
    expected[0]->cids = g_new0 (guint32, expected[0]->cids_count);
    expected[0]->cids[0] = MBIM_CID_ATDS_SIGNAL;
    expected[0]->cids[1] = MBIM_CID_ATDS_LOCATION;
    expected[0]->cids[2] = MBIM_CID_ATDS_OPERATORS;
    expected[0]->cids[3] = MBIM_CID_ATDS_RAT;
    expected[1] = g_new0 (MbimEventEntry, 1);
    memcpy (&expected[1]->device_service_id, MBIM_UUID_QMI, sizeof (MbimUuid));
    expected[1]->cids_count = 1;
    expected[1]->cids = g_new0 (guint32, expected[1]->cids_count);
    expected[1]->cids[0] = MBIM_CID_QMI_MSG;
    expected[2] = g_new0 (MbimEventEntry, 1);
    memcpy (&expected[2]->device_service_id, MBIM_UUID_MS_HOST_SHUTDOWN, sizeof (MbimUuid));
    expected[2]->cids_count = 1;
    expected[2]->cids = g_new0 (guint32, expected[2]->cids_count);
    expected[2]->cids[0] = MBIM_CID_MS_HOST_SHUTDOWN_NOTIFY;

    /* Compare */
    g_assert (_mbim_proxy_helper_service_subscribe_list_cmp ((const MbimEventEntry * const *)list, out_size,
                                                             (const MbimEventEntry * const *)expected, expected_size));

    mbim_event_entry_array_free (list);
    mbim_event_entry_array_free (addition);
    mbim_event_entry_array_free (expected);
}

/*****************************************************************************/

int main (int argc, char **argv)
{
    g_test_init (&argc, &argv, NULL);

    g_test_add_func ("/libmbim-glib/proxy/parse/single-service/0",     test_parse_single_service_0_cids);
    g_test_add_func ("/libmbim-glib/proxy/parse/single-service/1",     test_parse_single_service_1_cids);
    g_test_add_func ("/libmbim-glib/proxy/parse/single-service/5",     test_parse_single_service_5_cids);
    g_test_add_func ("/libmbim-glib/proxy/merge/none",                 test_merge_none);
    g_test_add_func ("/libmbim-glib/proxy/merge/standard",             test_merge_standard_services);
    g_test_add_func ("/libmbim-glib/proxy/merge/other",                test_merge_other_services);
    g_test_add_func ("/libmbim-glib/proxy/merge/same-service",         test_merge_list_same_service);
    g_test_add_func ("/libmbim-glib/proxy/merge/different-services",   test_merge_list_different_services);
    g_test_add_func ("/libmbim-glib/proxy/merge/merged-services",      test_merge_list_merged_services);

    return g_test_run ();
}
