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
 * Copyright (C) 2016 Zodiac Inflight Innovations
 * Copyright (C) 2016 Aleksander Morgado <aleksander@aleksander.es>
 */

#include "qfu-utils.h"

/******************************************************************************/

static void
common_version_parser_test (const gchar *version,
                            const gchar *expected_firmware_version,
                            const gchar *expected_config_version,
                            const gchar *expected_carrier)
{
    gboolean  res;
    GError   *error = NULL;
    gchar    *firmware_version = NULL;
    gchar    *config_version = NULL;
    gchar    *carrier = NULL;

    res = qfu_utils_parse_cwe_version_string (version,
                                              &firmware_version,
                                              &config_version,
                                              &carrier,
                                              &error);
    g_assert_no_error (error);
    g_assert (res);

    g_assert_cmpstr (firmware_version, ==, expected_firmware_version);
    g_assert_cmpstr (config_version,   ==, expected_config_version);
    g_assert_cmpstr (carrier,          ==, expected_carrier);

    g_free (firmware_version);
    g_free (config_version);
    g_free (carrier);
}

static void
test_cwe_version_parser_mc7700 (void)
{
    common_version_parser_test ("9999999_9999999_9200_03.05.29.03_00_generic_000.000_001_SPKG_MC",
                                "03.05.29.03",
                                "000.000_001",
                                "generic");
}

static void
test_cwe_version_parser_mc7354_cwe (void)
{
    common_version_parser_test ("INTERNAL_?_SWI9X15C_05.05.63.01_?_?_?_?",
                                "05.05.63.01",
                                NULL,
                                NULL);
}

static void
test_cwe_version_parser_mc7354_nvu (void)
{
    common_version_parser_test ("9999999_9902350_SWI9X15C_05.05.63.01_00_SPRINT_005.037_000",
                                "05.05.63.01",
                                "005.037_000",
                                "SPRINT");
}

static void
test_cwe_version_parser_mc7354b_spk (void)
{
    common_version_parser_test ("9999999_9902574_SWI9X15C_05.05.66.00_00_GENNA-UMTS_005.028_000",
                                "05.05.66.00",
                                "005.028_000",
                                "GENNA-UMTS");
}

/******************************************************************************/

int main (int argc, char **argv)
{
    g_test_init (&argc, &argv, NULL);

    g_test_add_func ("/qmi-firmware-update/cwe-version-parser/mc7700",      test_cwe_version_parser_mc7700);
    g_test_add_func ("/qmi-firmware-update/cwe-version-parser/mc7354/cwe",  test_cwe_version_parser_mc7354_cwe);
    g_test_add_func ("/qmi-firmware-update/cwe-version-parser/mc7354/nvu",  test_cwe_version_parser_mc7354_nvu);
    g_test_add_func ("/qmi-firmware-update/cwe-version-parser/mc7354b/spk", test_cwe_version_parser_mc7354b_spk);

    return g_test_run ();
}
