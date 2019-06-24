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

#ifndef QFU_FIREHOSE_MESSAGE_H
#define QFU_FIREHOSE_MESSAGE_H

#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

gsize    qfu_firehose_message_build_ping               (guint8       *buffer,
                                                        gsize         buffer_len);
gsize    qfu_firehose_message_build_configure          (guint8       *buffer,
                                                        gsize         buffer_len,
                                                        guint         max_payload_size_to_target_in_bytes);
gsize    qfu_firehose_message_build_get_storage_info   (guint8       *buffer,
                                                        gsize         buffer_len);
gsize    qfu_firehose_message_build_program            (guint8       *buffer,
                                                        gsize         buffer_len,
                                                        guint         pages_per_block,
                                                        guint         sector_size_in_bytes,
                                                        guint         num_partition_sectors);
gsize    qfu_firehose_message_build_reset              (guint8       *buffer,
                                                        gsize         buffer_len);

gboolean qfu_firehose_message_parse_response_ack       (const gchar  *rsp,
                                                        gchar       **value,
                                                        gchar       **rawmode);
gboolean qfu_firehose_message_parse_response_configure (const gchar  *rsp,
                                                        guint32      *max_payload_size_to_target_in_bytes);
gboolean qfu_firehose_message_parse_log                (const gchar  *rsp,
                                                        gchar       **value);

G_END_DECLS

#endif /* QFU_FIREHOSE_MESSAGE_H */
