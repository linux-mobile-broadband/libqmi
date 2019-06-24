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

#include <glib.h>
#include <string.h>
#include <stdlib.h>
#include "qfu-firehose-message.h"

/* NOTE: these parsers are NOT supposed to be general purpose parsers for the
 * firehose protocol message types, we're exclusively processing the messages
 * expected in the Sierra 9x50 firmware upgrade protocol, and therefore it's
 * assumed that using the regex matching approach is enough... */

#define FIREHOSE_MESSAGE_HEADER  "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n<data>\n"
#define FIREHOSE_MESSAGE_TRAILER "\n</data>\n\n"

gsize
qfu_firehose_message_build_ping (guint8 *buffer,
                                 gsize   buffer_len)
{
    g_snprintf ((gchar *)buffer, buffer_len,
                "%s"
                "<NOP value=\"ping\" />"
                "%s",
                FIREHOSE_MESSAGE_HEADER,
                FIREHOSE_MESSAGE_TRAILER);
    return strlen ((gchar *)buffer);
}

gsize
qfu_firehose_message_build_configure (guint8 *buffer,
                                      gsize   buffer_len,
                                      guint   max_payload_size_to_target_in_bytes)
{


    g_snprintf ((gchar *)buffer, buffer_len,
                "%s"
                "<configure MemoryName=\"eMMC\" Verbose=\"0\" AlwaysValidate=\"0\" MaxDigestTableSizeInBytes=\"8192\" MaxPayloadSizeToTargetInBytes=\"%u\" ZlpAwareHost=\"0\" SkipStorageInit=\"0\" TargetName=\"8960\" />"
                "%s",
                FIREHOSE_MESSAGE_HEADER,
                /* if not a specific value given, pass some dummy big value; we expect the modem
                 * to return a NAK with the correct value to use after that. */
                max_payload_size_to_target_in_bytes ? max_payload_size_to_target_in_bytes : 1048576,
                FIREHOSE_MESSAGE_TRAILER);
    return strlen ((gchar *)buffer);
}

gsize
qfu_firehose_message_build_get_storage_info (guint8 *buffer,
                                             gsize   buffer_len)
{
    g_snprintf ((gchar *)buffer, buffer_len,
                "%s"
                "<getStorageInfo physical_partition_number=\"0\" />"
                "%s",
                FIREHOSE_MESSAGE_HEADER,
                FIREHOSE_MESSAGE_TRAILER);
    return strlen ((gchar *)buffer);
}

gsize
qfu_firehose_message_build_program (guint8 *buffer,
                                    gsize   buffer_len,
                                    guint   pages_per_block,
                                    guint   sector_size_in_bytes,
                                    guint   num_partition_sectors)
{
    g_snprintf ((gchar *)buffer, buffer_len,
                "%s"
                "<program PAGES_PER_BLOCK=\"%u\" SECTOR_SIZE_IN_BYTES=\"%u\" filename=\"spkg.cwe\" num_partition_sectors=\"%u\" physical_partition_number=\"0\" start_sector=\"-1\" />"
                "%s",
                FIREHOSE_MESSAGE_HEADER,
                pages_per_block,
                sector_size_in_bytes,
                num_partition_sectors,
                FIREHOSE_MESSAGE_TRAILER);
    return strlen ((gchar *)buffer);
}

gsize
qfu_firehose_message_build_reset (guint8 *buffer,
                                  gsize   buffer_len)
{
    g_snprintf ((gchar *)buffer, buffer_len,
                "%s"
                "<power DelayInSeconds=\"0\" value=\"reset\" />"
                "%s",
                FIREHOSE_MESSAGE_HEADER,
                FIREHOSE_MESSAGE_TRAILER);
    return strlen ((gchar *)buffer);
}

gboolean
qfu_firehose_message_parse_response_ack (const gchar  *rsp,
                                         gchar       **value,
                                         gchar       **rawmode)
{
    GRegex     *r;
    GMatchInfo *match_info = NULL;
    gboolean    success = FALSE;

    /*
     * <?xml version="1.0" encoding="UTF-8" ?>
     * <data>
     * <response value="ACK" rawmode="true" />
     * </data>
     */

    r = g_regex_new ("<data>\\s*<response\\s*value=\"([^\"]*)\"(?:\\s*rawmode=\"([^\"]*)\")?\\s*/>\\s*</data>",
                     G_REGEX_RAW, 0, NULL);
    g_assert (r);

    if (!g_regex_match (r, rsp, 0, &match_info))
        goto out;

    if (!g_match_info_matches (match_info))
        goto out;

    if (value)
        *value = g_match_info_fetch (match_info, 1);
    if (rawmode) {
        if (g_match_info_get_match_count (match_info) > 2)
            *rawmode = g_match_info_fetch (match_info, 2);
        else
            *rawmode = NULL;
    }

    success = TRUE;

out:
    if (match_info)
        g_match_info_unref (match_info);
    g_regex_unref (r);
    return success;
}

gboolean
qfu_firehose_message_parse_log (const gchar  *rsp,
                                gchar       **value)
{
    GRegex     *r;
    GMatchInfo *match_info = NULL;
    gboolean    success = FALSE;

    /*
     * <?xml version="1.0" encoding="UTF-8" ?>
     * <data>
     * <log value="SWI supported functions: CWE"/>
     * </data>
     */

    r = g_regex_new ("<data>\\s*<log\\s*value=\"([^\"]*)\"\\s*/>\\s*</data>",
                     G_REGEX_RAW, 0, NULL);
    g_assert (r);

    if (!g_regex_match (r, rsp, 0, &match_info))
        goto out;

    if (!g_match_info_matches (match_info))
        goto out;

    if (value)
        *value = g_match_info_fetch (match_info, 1);

    success = TRUE;

out:
    if (match_info)
        g_match_info_unref (match_info);
    g_regex_unref (r);
    return success;
}

gboolean
qfu_firehose_message_parse_response_configure (const gchar  *rsp,
                                               guint32      *max_payload_size_to_target_in_bytes)
{
    GRegex     *r;
    GMatchInfo *match_info = NULL;
    gboolean    success = FALSE;

    /*
     * <?xml version="1.0" encoding="UTF-8" ?>
     * <data>
     * <response value="NAK" MemoryName="NAND" MaxPayloadSizeFromTargetInBytes="2048" MaxPayloadSizeToTargetInBytes="8192" MaxPayloadSizeToTargetInBytesSupported="8192" TargetName="9x55" />
     * </data>
     */

    r = g_regex_new ("<data>\\s*<response\\s*(?:.*)MaxPayloadSizeToTargetInBytes=\"([^\"]*)\"(?:.*)/>\\s*</data>",
                     G_REGEX_RAW, 0, NULL);
    g_assert (r);

    if (!g_regex_match (r, rsp, 0, &match_info))
        goto out;

    if (!g_match_info_matches (match_info))
        goto out;

    if (max_payload_size_to_target_in_bytes) {
        gchar *aux;

        aux = g_match_info_fetch (match_info, 1);
        *max_payload_size_to_target_in_bytes = atoi (aux);
        g_free (aux);
    }

    success = TRUE;

out:
    if (match_info)
        g_match_info_unref (match_info);
    g_regex_unref (r);
    return success;
}
