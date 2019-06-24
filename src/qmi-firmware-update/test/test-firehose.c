/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * qmi-firmware-update -- Command line tool to update firmware in QMI devices
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
 * Copyright (C) 2019 Zodiac Inflight Innovations
 * Copyright (C) 2019 Aleksander Morgado <aleksander@aleksander.es>
 */

#include "qfu-firehose-message.h"

static void
test_firehose_response_ack_parser_value (void)
{
    const gchar *rsp = "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n<data>\n<response value=\"ACK\" />\n</data>";
    gchar       *value = NULL;
    gchar       *rawmode = NULL;

    g_assert (qfu_firehose_message_parse_response_ack (rsp, &value, &rawmode));
    g_assert_cmpstr (value, ==, "ACK");
    g_assert (rawmode == NULL);
    g_clear_pointer (&value, g_free);
}

static void
test_firehose_response_ack_parser_value_rawmode (void)
{
    const gchar *rsp = "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n<data>\n<response value=\"ACK\" rawmode=\"true\" />\n</data>";
    gchar       *value = NULL;
    gchar       *rawmode = NULL;

    g_assert (qfu_firehose_message_parse_response_ack (rsp, &value, &rawmode));
    g_assert_cmpstr (value, ==, "ACK");
    g_assert_cmpstr (rawmode, ==, "true");
    g_clear_pointer (&value, g_free);
    g_clear_pointer (&rawmode, g_free);
}

static void
test_firehose_response_configure_parser (void)
{
    const gchar *rsp = "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n<data>\n<response value=\"NAK\" MemoryName=\"NAND\" MaxPayloadSizeFromTargetInBytes=\"2048\" MaxPayloadSizeToTargetInBytes=\"8192\" MaxPayloadSizeToTargetInBytesSupported=\"8192\" TargetName=\"9x55\" />\n</data>";
    guint        number = 0;

    g_assert (qfu_firehose_message_parse_response_configure (rsp, &number));
    g_assert_cmpuint (number, ==, 8192);
}

static void
test_firehose_log_parser_value (void)
{
    const gchar *rsp = "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n<data>\n<log value=\"SWI supported functions: CWE\"/>\n</data>";
    gchar       *value = NULL;

    g_assert (qfu_firehose_message_parse_log (rsp, &value));
    g_assert_cmpstr (value, ==, "SWI supported functions: CWE");
    g_clear_pointer (&value, g_free);
}

/******************************************************************************/

int main (int argc, char **argv)
{
    g_test_init (&argc, &argv, NULL);

    g_test_add_func ("/qmi-firmware-update/firehose/response-ack-parser/value",         test_firehose_response_ack_parser_value);
    g_test_add_func ("/qmi-firmware-update/firehose/response-ack-varser/value-rawmode", test_firehose_response_ack_parser_value_rawmode);
    g_test_add_func ("/qmi-firmware-update/firehose/response-configure-parser",         test_firehose_response_configure_parser);
    g_test_add_func ("/qmi-firmware-update/firehose/log-parser/value",                  test_firehose_log_parser_value);

    return g_test_run ();
}
