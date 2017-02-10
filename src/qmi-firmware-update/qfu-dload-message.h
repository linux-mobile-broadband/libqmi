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

#ifndef QFU_DLOAD_MESSAGE_H
#define QFU_DLOAD_MESSAGE_H

#include <glib-object.h>
#include <gio/gio.h>

#include "qfu-image.h"

G_BEGIN_DECLS

/* Maximum DLOAD message size */
#define QFU_DLOAD_MESSAGE_MAX_SIZE 3

/*
 * Most of this is from Josuah Hill's DLOAD tool for iPhone
 * Some spec is also available in document 80-39912-1 Rev. E  DMSS Download Protocol Interface Specification and Operational Description
 * https://github.com/posixninja/DLOADTool/blob/master/dloadtool/dload.h
 *
 * The 0x70 switching command was found by snooping on firmware updates
 */
typedef enum {
    QFU_DLOAD_CMD_ACK = 0x02,
    QFU_DLOAD_CMD_NOP = 0x06,
    QFU_DLOAD_CMD_SDP = 0x70,
} QfuDloadCmd;

/* Request builders */

gsize qfu_dload_request_sdp_build (guint8 *buffer,
                                   gsize   buffer_len);

/* Response parsers */

gboolean qfu_dload_response_ack_parse (const guint8  *buffer,
                                       gsize          buffer_len,
                                       GError       **error);

G_END_DECLS

#endif /* QFU_DLOAD_MESSAGE_H */
