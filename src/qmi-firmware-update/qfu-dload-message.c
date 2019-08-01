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

#include "qfu-dload-message.h"
#include "qfu-utils.h"
#include "qfu-enum-types.h"

/******************************************************************************/
/* DLOAD SDP */

/* Generic message for operations that just require the command id */
typedef struct _DloadSdpReq DloadSdpReq;
struct _DloadSdpReq {
    guint8  cmd;
    guint16 reserved;
} __attribute__ ((packed));

G_STATIC_ASSERT (sizeof (DloadSdpReq) <= QFU_DLOAD_MESSAGE_MAX_SIZE);

gsize
qfu_dload_request_sdp_build (guint8 *buffer,
                             gsize   buffer_len)
{
    DloadSdpReq *req;

    g_assert (buffer_len >= sizeof (DloadSdpReq));

    /* Create request */
    req = (DloadSdpReq *) buffer;
    memset (buffer, 0, sizeof (DloadSdpReq));
    req->cmd = (guint8) QFU_DLOAD_CMD_SDP;

    g_debug ("[qfu,dload-message] sent %s:", qfu_dload_cmd_get_string ((QfuDloadCmd) req->cmd));

    return (sizeof (DloadSdpReq));
}

/******************************************************************************/
/* DLOAD Ack */

typedef struct _DloadAckRsp DloadAckRsp;
struct _DloadAckRsp {
    guint8  cmd; /* 0x2a */
} __attribute__ ((packed));

G_STATIC_ASSERT (sizeof (DloadAckRsp) <= QFU_DLOAD_MESSAGE_MAX_SIZE);

gboolean
qfu_dload_response_ack_parse (const guint8  *buffer,
                              gsize          buffer_len,
                              GError       **error)
{
    DloadAckRsp *rsp;

    if (buffer_len != sizeof (DloadAckRsp)) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "message size mismatch: %" G_GSIZE_FORMAT " != %" G_GSIZE_FORMAT,
                     buffer_len, sizeof (DloadAckRsp));
        return FALSE;
    }

    rsp = (DloadAckRsp *) buffer;
    g_assert (rsp->cmd == QFU_DLOAD_CMD_ACK);

    g_debug ("[qfu,dload-message] received %s:", qfu_dload_cmd_get_string ((QfuDloadCmd) rsp->cmd));

    return TRUE;
}
