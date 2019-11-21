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

#include <string.h>

#include <glib-object.h>
#include <gio/gio.h>

#include "qfu-qdl-message.h"
#include "qfu-utils.h"
#include "qfu-enum-types.h"

/******************************************************************************/
/* QDL generic */

/* Generic message for operations that just require the command id */
typedef struct _QdlMsg QdlMsg;
struct _QdlMsg {
    guint8 cmd;
} __attribute__ ((packed));

G_STATIC_ASSERT (sizeof (QdlMsg) <= QFU_QDL_MESSAGE_MAX_HEADER_SIZE);

static gsize
qdl_message_generic_build (guint8    *buffer,
                           gsize      buffer_len,
                           QfuQdlCmd  cmd)
{
    QdlMsg *req;

    g_assert (buffer_len >= sizeof (QdlMsg));

    /* Create request */
    req = (QdlMsg *) buffer;
    req->cmd = (guint8) cmd;

    g_debug ("[qfu,qdl-message] sent %s:", qfu_qdl_cmd_get_string ((QfuQdlCmd) req->cmd));

    return (sizeof (QdlMsg));
}

/******************************************************************************/
/* QDL Hello */

/* feature bits */
#define QDL_FEATURE_GENERIC_UNFRAMED 0x10
#define QDL_FEATURE_QDL_UNFRAMED     0x20
#define QDL_FEATURE_BAR_MODE         0x40

typedef struct _QdlHelloReq QdlHelloReq;
struct _QdlHelloReq {
    guint8 cmd; /* 0x01 */
    gchar  magic[32];
    guint8 maxver;
    guint8 minver;
    guint8 features;
} __attribute__ ((packed));

G_STATIC_ASSERT (sizeof (QdlHelloReq) <= QFU_QDL_MESSAGE_MAX_HEADER_SIZE);

gsize
qfu_qdl_request_hello_build (guint8 *buffer,
                             gsize   buffer_len,
                             guint8  minver,
                             guint8  maxver)
{
    static const QdlHelloReq common_hello_req = {
        .cmd = QFU_QDL_CMD_HELLO_REQ,
        .magic = { "QCOM high speed protocol hst" },
        .maxver = 0,
        .minver = 0,
        .features = QDL_FEATURE_QDL_UNFRAMED | QDL_FEATURE_GENERIC_UNFRAMED,
    };
    QdlHelloReq *req;

    g_assert (buffer_len >= sizeof (QdlHelloReq));

    /* Create request */
    req = (QdlHelloReq *) buffer;
    memcpy (req, &common_hello_req, sizeof (QdlHelloReq));
    req->minver = minver;
    req->maxver = maxver;

    g_debug ("[qfu,qdl-message] sent %s:", qfu_qdl_cmd_get_string ((QfuQdlCmd) req->cmd));
    g_debug ("[qfu,qdl-message]   magic:           %.*s",   req->maxver <= 5 ? 24 : 32, req->magic);
    g_debug ("[qfu,qdl-message]   maximum version: %u",     req->maxver);
    g_debug ("[qfu,qdl-message]   minimum version: %u",     req->minver);
    g_debug ("[qfu,qdl-message]   features:        0x%02x", req->features);

    return (sizeof (QdlHelloReq));
}

typedef struct _QdlHelloRsp QdlHelloRsp;
struct _QdlHelloRsp {
    guint8  cmd; /* 0x02 */
    gchar   magic[32];
    guint8  maxver;
    guint8  minver;
    guint32 reserved1;
    guint32 reserved2;
    guint8  reserved3;
    guint16 reserved4;
    guint16 reserved5;
    guint8  features;
} __attribute__ ((packed));

G_STATIC_ASSERT (sizeof (QdlHelloRsp) <= QFU_QDL_MESSAGE_MAX_HEADER_SIZE);

gboolean
qfu_qdl_response_hello_parse (const guint8  *buffer,
                              gsize          buffer_len,
                              GError       **error)
{
    QdlHelloRsp *rsp;

    if (buffer_len != sizeof (QdlHelloRsp)) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "message size mismatch: %" G_GSIZE_FORMAT " != %" G_GSIZE_FORMAT,
                     buffer_len, sizeof (QdlHelloRsp));
        return FALSE;
    }

    rsp = (QdlHelloRsp *) buffer;
    g_assert (rsp->cmd == QFU_QDL_CMD_HELLO_RSP);

    g_debug ("[qfu,qdl-message] received %s:", qfu_qdl_cmd_get_string ((QfuQdlCmd) rsp->cmd));
    g_debug ("[qfu,qdl-message]   magic:           %.*s",   rsp->maxver <= 5 ? 24 : 32, rsp->magic);
    g_debug ("[qfu,qdl-message]   maximum version: %u",     rsp->maxver);
    g_debug ("[qfu,qdl-message]   minimum version: %u",     rsp->minver);
    g_debug ("[qfu,qdl-message]   features:        0x%02x", rsp->features);

    /* For now, ignore fields */

    return TRUE;
}

/******************************************************************************/
/* QDL Error */

typedef enum {
    QDL_ERROR_NONE              = 0x00,
    QDL_ERROR_01_RESERVED       = 0x01,
    QDL_ERROR_BAD_ADDR          = 0x02,
    QDL_ERROR_BAD_LEN           = 0x03,
    QDL_ERROR_BAD_PACKET        = 0x04,
    QDL_ERROR_BAD_CMD           = 0x05,
    QDL_ERROR_06                = 0x06,
    QDL_ERROR_OP_FAILED         = 0x07,
    QDL_ERROR_BAD_FLASH_ID      = 0x08,
    QDL_ERROR_BAD_VOLTAGE       = 0x09,
    QDL_ERROR_WRITE_FAILED      = 0x0a,
    QDL_ERROR_11_RESERVED       = 0x0b,
    QDL_ERROR_BAD_SPC           = 0x0c,
    QDL_ERROR_POWERDOWN         = 0x0d,
    QDL_ERROR_UNSUPPORTED       = 0x0e,
    QDL_ERROR_CMD_SEQ           = 0x0f,
    QDL_ERROR_CLOSE             = 0x10,
    QDL_ERROR_BAD_FEATURES      = 0x11,
    QDL_ERROR_SPACE             = 0x12,
    QDL_ERROR_BAD_SECURITY      = 0x13,
    QDL_ERROR_MULTI_UNSUPPORTED = 0x14,
    QDL_ERROR_POWEROFF          = 0x15,
    QDL_ERROR_CMD_UNSUPPORTED   = 0x16,
    QDL_ERROR_BAD_CRC           = 0x17,
    QDL_ERROR_STATE             = 0x18,
    QDL_ERROR_TIMEOUT           = 0x19,
    QDL_ERROR_IMAGE_AUTH        = 0x1a,
    QDL_ERROR_LAST
} QdlError;

static const gchar *qdl_error_str[] = {
    [QDL_ERROR_NONE              ] = "None",
    [QDL_ERROR_01_RESERVED       ] = "Reserved",
    [QDL_ERROR_BAD_ADDR          ] = "Invalid destination address",
    [QDL_ERROR_BAD_LEN           ] = "Invalid length",
    [QDL_ERROR_BAD_PACKET        ] = "Unexpected end of packet",
    [QDL_ERROR_BAD_CMD           ] = "Invalid command",
    [QDL_ERROR_06                ] = "Reserved",
    [QDL_ERROR_OP_FAILED         ] = "Operation failed",
    [QDL_ERROR_BAD_FLASH_ID      ] = "Invalid flash intelligent ID",
    [QDL_ERROR_BAD_VOLTAGE       ] = "Invalid programming voltage",
    [QDL_ERROR_WRITE_FAILED      ] = "Write verify failed",
    [QDL_ERROR_11_RESERVED       ] = "Reserved",
    [QDL_ERROR_BAD_SPC           ] = "Invalid security code",
    [QDL_ERROR_POWERDOWN         ] = "Power-down failed",
    [QDL_ERROR_UNSUPPORTED       ] = "NAND flash programming not supported",
    [QDL_ERROR_CMD_SEQ           ] = "Command out of sequence",
    [QDL_ERROR_CLOSE             ] = "Close failed",
    [QDL_ERROR_BAD_FEATURES      ] = "Invalid feature bits",
    [QDL_ERROR_SPACE             ] = "Out of space",
    [QDL_ERROR_BAD_SECURITY      ] = "Invalid security mode",
    [QDL_ERROR_MULTI_UNSUPPORTED ] = "Multi-image NAND not supported",
    [QDL_ERROR_POWEROFF          ] = "Power-off command not supported",
    [QDL_ERROR_CMD_UNSUPPORTED   ] = "Command not supported",
    [QDL_ERROR_BAD_CRC           ] = "Invalid CRC",
    [QDL_ERROR_STATE             ] = "Command received in invalid state",
    [QDL_ERROR_TIMEOUT           ] = "Receive timeout",
    [QDL_ERROR_IMAGE_AUTH        ] = "Image authentication error",
};

G_STATIC_ASSERT (G_N_ELEMENTS (qdl_error_str) == QDL_ERROR_LAST);

static GIOErrorEnum
qdl_error_to_gio_error_enum (QdlError err)
{
    if (err == QDL_ERROR_CMD_UNSUPPORTED)
        return G_IO_ERROR_NOT_SUPPORTED;
    return G_IO_ERROR_FAILED;
}

static const gchar *
qdl_error_to_string (QdlError err)
{
    return (err < QDL_ERROR_LAST ? qdl_error_str[err] : "Unknown");
}

typedef struct _QdlErrRsp QdlErrRsp;
struct _QdlErrRsp {
    guint8  cmd; /* 0x0d */
    guint32 error;
    guint8  errortxt;
} __attribute__ ((packed));

G_STATIC_ASSERT (sizeof (QdlErrRsp) <= QFU_QDL_MESSAGE_MAX_HEADER_SIZE);

gboolean
qfu_qdl_response_error_parse (const guint8  *buffer,
                              gsize          buffer_len,
                              GError       **error)
{
    QdlErrRsp *rsp;

    if (buffer_len != sizeof (QdlErrRsp)) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "message size mismatch: %" G_GSIZE_FORMAT " != %" G_GSIZE_FORMAT,
                     buffer_len, sizeof (QdlErrRsp));
        return FALSE;
    }

    rsp = (QdlErrRsp *) buffer;
    g_assert (rsp->cmd == QFU_QDL_CMD_ERROR);

    g_debug ("[qfu,qdl-message] received %s", qfu_qdl_cmd_get_string ((QfuQdlCmd) rsp->cmd));
    g_debug ("[qfu,qdl-message]   error:    %" G_GUINT32_FORMAT, GUINT32_FROM_LE (rsp->error));
    g_debug ("[qfu,qdl-message]   errortxt: %u", rsp->errortxt);

    /* Always return an error in this case */
    g_set_error (error, G_IO_ERROR, qdl_error_to_gio_error_enum (GUINT32_FROM_LE (rsp->error)),
                 "%s", qdl_error_to_string ((QdlError) GUINT32_FROM_LE (rsp->error)));
    return FALSE;
}

/******************************************************************************/
/* QDL Ufopen */

typedef struct _QdlUfopenReq QdlUfopenReq;
struct _QdlUfopenReq {
    guint8  cmd; /* 0x25 */
    guint8  type;
    guint32 length;
    guint8  windowsize;
    guint32 chunksize;
    guint16 reserved;
} __attribute__ ((packed));

G_STATIC_ASSERT (sizeof (QdlUfopenReq) <= QFU_QDL_MESSAGE_MAX_HEADER_SIZE);

gssize
qfu_qdl_request_ufopen_build (guint8        *buffer,
                              gsize          buffer_len,
                              QfuImage      *image,
                              GCancellable  *cancellable,
                              GError       **error)
{
    QdlUfopenReq *req;

    g_assert (buffer_len >= sizeof (QdlUfopenReq));

    /* Create request */
    req = (QdlUfopenReq *) buffer;
    memset (req, 0, sizeof (QdlUfopenReq));
    req->cmd        = QFU_QDL_CMD_OPEN_UNFRAMED_REQ;
    req->type       = (guint8) qfu_image_get_image_type (image);
    req->windowsize = 1; /* snooped */
    req->length     = GUINT32_TO_LE (qfu_image_get_header_size (image) + qfu_image_get_data_size (image));
    req->chunksize  = GUINT32_TO_LE (qfu_image_get_data_size (image));

    /* Append header */
    if (qfu_image_read_header (image, buffer + sizeof (QdlUfopenReq), buffer_len - sizeof (QdlUfopenReq), cancellable, error) < 0) {
        g_prefix_error (error, "couldn't read image header: ");
        return -1;
    }

    g_debug ("[qfu,qdl-message] sent %s:", qfu_qdl_cmd_get_string ((QfuQdlCmd) req->cmd));
    g_debug ("[qfu,qdl-message]   type:        %u", req->type);
    g_debug ("[qfu,qdl-message]   length:      %" G_GUINT32_FORMAT, GUINT32_FROM_LE (req->length));
    g_debug ("[qfu,qdl-message]   window size: %u", req->windowsize);
    g_debug ("[qfu,qdl-message]   chunk size:  %" G_GUINT32_FORMAT, GUINT32_FROM_LE (req->chunksize));

    return (sizeof (QdlUfopenReq) + qfu_image_get_header_size (image));
}

typedef struct _QdlUfopenRsp QdlUfopenRsp;
struct _QdlUfopenRsp {
    guint8  cmd; /* 0x26 */
    guint16 status;
    guint8  windowsize;
    guint32 chunksize;
} __attribute__ ((packed));

G_STATIC_ASSERT (sizeof (QdlUfopenRsp) <= QFU_QDL_MESSAGE_MAX_HEADER_SIZE);

gboolean
qfu_qdl_response_ufopen_parse (const guint8  *buffer,
                               gsize          buffer_len,
                               GError       **error)
{
    QdlUfopenRsp *rsp;

    if (buffer_len != sizeof (QdlUfopenRsp)) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "message size mismatch: %" G_GSIZE_FORMAT " != %" G_GSIZE_FORMAT,
                     buffer_len, sizeof (QdlUfopenRsp));
        return FALSE;
    }

    rsp = (QdlUfopenRsp *) buffer;
    g_assert (rsp->cmd == QFU_QDL_CMD_OPEN_UNFRAMED_RSP);

    g_debug ("[qfu,qdl-message] received %s", qfu_qdl_cmd_get_string ((QfuQdlCmd) rsp->cmd));
    g_debug ("[qfu,qdl-message]   status:      %" G_GUINT16_FORMAT, GUINT16_FROM_LE (rsp->status));
    g_debug ("[qfu,qdl-message]   window size: %u", rsp->windowsize);
    g_debug ("[qfu,qdl-message]   chunk size:  %" G_GUINT32_FORMAT, GUINT32_FROM_LE (rsp->chunksize));

    /* For now, ignore all fields but build a GError based on status */

    /* Return error if status != 0 */
    if (rsp->status != 0) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "operation returned an error status: %" G_GUINT16_FORMAT,
                     GUINT16_FROM_LE (rsp->status));
        return FALSE;
    }

    return TRUE;
}

/******************************************************************************/
/* QDL Ufwrite */

/* This request is not HDLC framed, so this "header" includes the crc */
typedef struct _QdlUfwriteReq QdlUfwriteReq;
struct _QdlUfwriteReq {
    guint8  cmd; /* 0x27 */
    guint16 sequence;
    guint32 reserved;
    guint32 chunksize;
    guint16 crc;
} __attribute__ ((packed));

G_STATIC_ASSERT (sizeof (QdlUfwriteReq) <= QFU_QDL_MESSAGE_MAX_HEADER_SIZE);

gssize
qfu_qdl_request_ufwrite_build (guint8        *buffer,
                               gsize          buffer_len,
                               QfuImage      *image,
                               guint16        sequence,
                               GCancellable  *cancellable,
                               GError       **error)
{
    QdlUfwriteReq *req;
    gssize         n_read;

    g_assert (buffer_len >= sizeof (QdlUfwriteReq));

    /* Append chunk */
    n_read = qfu_image_read_data_chunk (image, sequence, buffer + sizeof (QdlUfwriteReq), buffer_len - sizeof (QdlUfwriteReq), cancellable, error);
    if (n_read < 0) {
        g_prefix_error (error, "couldn't read image chunk #%u: ", sequence);
        return -1;
    }

    /* Create request after appending, so that we have correct chunksize */
    req = (QdlUfwriteReq *) buffer;
    memset (req, 0, sizeof (QdlUfwriteReq));
    req->cmd       = QFU_QDL_CMD_WRITE_UNFRAMED_REQ;
    req->sequence  = GUINT16_TO_LE (sequence);
    req->reserved  = 0;
    req->chunksize = GUINT32_TO_LE ((guint32) n_read);
    req->crc       = GUINT16_TO_LE (qfu_utils_crc16 (buffer, sizeof (QdlUfwriteReq) - 2));

    g_debug ("[qfu,qdl-message] sent %s:", qfu_qdl_cmd_get_string ((QfuQdlCmd) req->cmd));
    g_debug ("[qfu,qdl-message]   sequence:   %" G_GUINT16_FORMAT, GUINT16_FROM_LE (req->sequence));
    g_debug ("[qfu,qdl-message]   chunk size: %" G_GUINT32_FORMAT, GUINT32_FROM_LE (req->chunksize));

    return (sizeof (QdlUfopenReq) + n_read);
}

/* The response is HDLC framed, so the crc is part of the framing */
typedef struct _QdlUfwriteRsp QdlUfwriteRsp;
struct _QdlUfwriteRsp {
    guint8  cmd; /* 0x28 */
    guint16 sequence;
    guint32 reserved;
    guint16 status;
} __attribute__ ((packed));

G_STATIC_ASSERT (sizeof (QdlUfwriteRsp) <= QFU_QDL_MESSAGE_MAX_HEADER_SIZE);

gboolean
qfu_qdl_response_ufwrite_parse (const guint8  *buffer,
                                gsize          buffer_len,
                                guint16       *sequence,
                                GError       **error)
{
    QdlUfwriteRsp *rsp;

    if (buffer_len != sizeof (QdlUfwriteRsp)) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "message size mismatch: %" G_GSIZE_FORMAT " != %" G_GSIZE_FORMAT,
                     buffer_len, sizeof (QdlUfwriteRsp));
        return FALSE;
    }

    rsp = (QdlUfwriteRsp *) buffer;
    g_assert (rsp->cmd == QFU_QDL_CMD_WRITE_UNFRAMED_RSP);

    g_debug ("[qfu,qdl-message] received %s", qfu_qdl_cmd_get_string ((QfuQdlCmd) rsp->cmd));
    g_debug ("[qfu,qdl-message]   status:   %" G_GUINT16_FORMAT, GUINT16_FROM_LE (rsp->status));
    g_debug ("[qfu,qdl-message]   sequence: %" G_GUINT16_FORMAT, GUINT16_FROM_LE (rsp->sequence));

    /* Only return sequence and return GError based on status */

    /* Return error if status != 0 */
    if (rsp->status != 0) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "operation returned an error status: %" G_GUINT16_FORMAT,
                     GUINT16_FROM_LE (rsp->status));
        return FALSE;
    }

    if (sequence)
        *sequence = GUINT16_FROM_LE (rsp->sequence);

    return TRUE;
}

/******************************************************************************/
/* QDL Ufclose */

gsize
qfu_qdl_request_ufclose_build (guint8 *buffer,
                               gsize   buffer_len)
{
    return qdl_message_generic_build (buffer, buffer_len, QFU_QDL_CMD_CLOSE_UNFRAMED_REQ);
}

typedef struct _QdlUfcloseRsp QdlUfcloseRsp;
struct _QdlUfcloseRsp {
    guint8  cmd; /* 0x2a */
    guint16 status;
    guint8  type;
    guint8  errortxt;
} __attribute__ ((packed));

gboolean
qfu_qdl_response_ufclose_parse (const guint8  *buffer,
                                gsize          buffer_len,
                                GError       **error)
{
    QdlUfcloseRsp *rsp;

    if (buffer_len != sizeof (QdlUfcloseRsp)) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "message size mismatch: %" G_GSIZE_FORMAT " != %" G_GSIZE_FORMAT,
                     buffer_len, sizeof (QdlUfcloseRsp));
        return FALSE;
    }

    rsp = (QdlUfcloseRsp *) buffer;
    g_assert (rsp->cmd == QFU_QDL_CMD_CLOSE_UNFRAMED_RSP);

    g_debug ("[qfu,qdl-message] received %s", qfu_qdl_cmd_get_string ((QfuQdlCmd) rsp->cmd));
    g_debug ("[qfu,qdl-message]   status:      %" G_GUINT16_FORMAT, GUINT16_FROM_LE (rsp->status));
    g_debug ("[qfu,qdl-message]   type:        %u", rsp->type);
    g_debug ("[qfu,qdl-message]   errortxt:    %u", rsp->errortxt);

    /* For now, ignore all fields but build a GError based on status */

    /* Return error if status != 0 */
    if (rsp->status != 0) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "operation returned an error status: %" G_GUINT16_FORMAT,
                     GUINT16_FROM_LE (rsp->status));
        return FALSE;
    }

    return TRUE;
}

/******************************************************************************/
/* QDL session close */

gsize
qfu_qdl_request_reset_build (guint8 *buffer,
                             gsize   buffer_len)
{
    return qdl_message_generic_build (buffer, buffer_len, QFU_QDL_CMD_RESET_REQ);
}

/******************************************************************************/
/* Other unused messages */

/* 0x29 - cmd only */
/* 0x2d - cmd only */
/* 0x2e - cmd only */

typedef struct _QdlImageprefRsp QdlImageprefRsp;
struct _QdlImageprefRsp {
    guint8 cmd; /* 0x2f */
    guint8 entries;
    struct {
        guint8 type;
        gchar  id[16];
    } image[];
} __attribute__ ((packed));
