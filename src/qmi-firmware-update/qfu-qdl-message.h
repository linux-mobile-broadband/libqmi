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
 * Copyright (C) 2016 Bjørn Mork <bjorn@mork.no>
 * Copyright (C) 2016 Zodiac Inflight Innovations
 * Copyright (C) 2016-2017 Aleksander Morgado <aleksander@aleksander.es>
 */

#ifndef QFU_QDL_MESSAGE_H
#define QFU_QDL_MESSAGE_H

#include <glib-object.h>
#include <gio/gio.h>

#include "qfu-image.h"

G_BEGIN_DECLS

/* Maximum QDL header size (i.e. without payload) */
#define QFU_QDL_MESSAGE_MAX_HEADER_SIZE 50

/* Maximum QDL message size (header and payload) */
#define QFU_QDL_MESSAGE_MAX_SIZE (QFU_QDL_MESSAGE_MAX_HEADER_SIZE + QFU_IMAGE_CHUNK_SIZE)

/* from GobiAPI_1.0.40/Core/QDLEnum.h and
 * GobiAPI_1.0.40/Core/QDLBuffers.h with additional details from USB
 * snooping
 */
typedef enum {
    QFU_QDL_CMD_HELLO_REQ          = 0x01,
    QFU_QDL_CMD_HELLO_RSP          = 0x02,
    QFU_QDL_CMD_ERROR              = 0x0d,
    QFU_QDL_CMD_OPEN_UNFRAMED_REQ  = 0x25,
    QFU_QDL_CMD_OPEN_UNFRAMED_RSP  = 0x26,
    QFU_QDL_CMD_WRITE_UNFRAMED_REQ = 0x27,
    QFU_QDL_CMD_WRITE_UNFRAMED_RSP = 0x28,
    QFU_QDL_CMD_CLOSE_UNFRAMED_REQ = 0x29,
    QFU_QDL_CMD_CLOSE_UNFRAMED_RSP = 0x2a,
    QFU_QDL_CMD_DOWNLOAD_REQ       = 0x2b,
    QFU_QDL_CMD_RESET_REQ          = 0x2d,
    QFU_QDL_CMD_GET_IMAGE_PREF_REQ = 0x2e,
    QFU_QDL_CMD_GET_IMAGE_PREF_RSP = 0x2f,
} QfuQdlCmd;

/* Request builders */

gsize  qfu_qdl_request_hello_build   (guint8        *buffer,
                                      gsize          buffer_len,
                                      guint8         minver,
                                      guint8         maxver);
gssize qfu_qdl_request_ufopen_build  (guint8        *buffer,
                                      gsize          buffer_len,
                                      QfuImage      *image,
                                      GCancellable  *cancellable,
                                      GError       **error);
gssize qfu_qdl_request_ufwrite_build (guint8        *buffer,
                                      gsize          buffer_len,
                                      QfuImage      *image,
                                      guint16        sequence,
                                      GCancellable  *cancellable,
                                      GError       **error);
gsize  qfu_qdl_request_ufclose_build (guint8        *buffer,
                                      gsize          buffer_len);
gsize  qfu_qdl_request_reset_build   (guint8        *buffer,
                                      gsize          buffer_len);

/* Response parsers */

gboolean qfu_qdl_response_hello_parse    (const guint8  *buffer,
                                          gsize          buffer_len,
                                          GError       **error);
gboolean qfu_qdl_response_error_parse    (const guint8  *buffer,
                                          gsize          buffer_len,
                                          GError       **error);
gboolean qfu_qdl_response_ufopen_parse   (const guint8  *buffer,
                                          gsize          buffer_len,
                                          GError       **error);
gboolean qfu_qdl_response_ufwrite_parse  (const guint8  *buffer,
                                          gsize          buffer_len,
                                          guint16       *sequence,
                                          GError       **error);
gboolean qfu_qdl_response_ufclose_parse  (const guint8  *buffer,
                                          gsize          buffer_len,
                                          GError       **error);

G_END_DECLS

#endif /* QFU_QDL_MESSAGE_H */
