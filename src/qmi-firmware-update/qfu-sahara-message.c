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

#include <glib-object.h>
#include <gio/gio.h>
#include <string.h>

#include "qfu-image.h"

#include "qfu-sahara-message.h"
#include "qfu-enum-types.h"

/******************************************************************************/
/* Sahara messages */

#define CURRENT_SAHARA_VERSION 0x00000002

typedef struct _SaharaHelloRequest SaharaHelloRequest;
struct _SaharaHelloRequest {
    QfuSaharaHeader header;
    guint32         version;
    guint32         compatible;
    guint32         max_len;
    guint32         mode;
    guint32         reserved[6];
} __attribute__ ((packed));

gboolean
qfu_sahara_request_hello_parse (const guint8   *buffer,
                                gsize           buffer_len,
                                GError        **error)
{
    SaharaHelloRequest *msg;
    QfuSaharaMode       mode;
    const gchar        *mode_str;

    if (buffer_len < sizeof (SaharaHelloRequest)) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "message size mismatch: %" G_GSIZE_FORMAT " != %" G_GSIZE_FORMAT,
                     buffer_len, sizeof (SaharaHelloRequest));
        return FALSE;
    }

    msg = (SaharaHelloRequest *) buffer;
    g_assert (msg->header.cmd == GUINT32_FROM_LE (QFU_SAHARA_CMD_HELLO_REQ));
    mode = (QfuSaharaMode) GUINT32_FROM_LE (msg->mode);
    mode_str = qfu_sahara_mode_get_string (mode);

    g_debug ("[qfu,sahara-message] received %s:", qfu_sahara_cmd_get_string (QFU_SAHARA_CMD_HELLO_REQ));
    g_debug ("[qfu,sahara-message]   version:    %u", GUINT32_FROM_LE (msg->version));
    g_debug ("[qfu,sahara-message]   compatible: %u", GUINT32_FROM_LE (msg->compatible));
    g_debug ("[qfu,sahara-message]   max length: %u", GUINT32_FROM_LE (msg->max_len));
    if (mode_str)
        g_debug ("[qfu,sahara-message]   mode:       %s", mode_str ? mode_str : "unknown");
    else
        g_debug ("[qfu,sahara-message]   mode:       unknown (0x%08x)", mode);

    /* our version needs to be greater or equal than the minimum version reported */
    if (GUINT32_FROM_LE (msg->compatible) > CURRENT_SAHARA_VERSION) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "unsupported sahara version (%u > %u)",
                     GUINT32_FROM_LE (msg->compatible), CURRENT_SAHARA_VERSION);
        return FALSE;
    }

    return TRUE;
}

typedef struct _SaharaHelloResponse SaharaHelloResponse;
struct _SaharaHelloResponse {
    QfuSaharaHeader header;
    guint32         version;
    guint32         compatible;
    guint32         status;
    guint32         mode;
    guint32         reserved[6];
} __attribute__ ((packed));

gsize
qfu_sahara_response_hello_build (guint8 *buffer,
                                 gsize   buffer_len)
{
    SaharaHelloResponse *msg = (SaharaHelloResponse *)buffer;

    g_assert (buffer_len >= sizeof (SaharaHelloResponse));

    msg->header.cmd  = GUINT32_TO_LE (QFU_SAHARA_CMD_HELLO_RSP);
    msg->header.size = GUINT32_TO_LE (sizeof (SaharaHelloResponse));
    msg->version     = GUINT32_TO_LE (CURRENT_SAHARA_VERSION);
    msg->compatible  = GUINT32_TO_LE (CURRENT_SAHARA_VERSION);
    msg->status      = GUINT32_TO_LE (QFU_SAHARA_STATUS_SUCCESS);
    msg->mode        = GUINT32_TO_LE (QFU_SAHARA_MODE_COMMAND);
    memset (msg->reserved, 0, sizeof (msg->reserved));

    return sizeof (SaharaHelloResponse);
}

#define EXECUTE_SWITCH_FIREHOSE 0x0000ff00

typedef struct _SaharaCommandExecuteRequest SaharaCommandExecuteRequest;
struct _SaharaCommandExecuteRequest {
    QfuSaharaHeader header;
    guint32         execute;
} __attribute__ ((packed));

gsize
qfu_sahara_request_switch_build (guint8 *buffer,
                                 gsize   buffer_len)
{
    SaharaCommandExecuteRequest *msg = (SaharaCommandExecuteRequest *)buffer;

    g_assert (buffer_len >= sizeof (SaharaCommandExecuteRequest));

    msg->header.cmd  = GUINT32_TO_LE (QFU_SAHARA_CMD_COMMAND_EXECUTE_REQ);
    msg->header.size = GUINT32_TO_LE (sizeof (SaharaCommandExecuteRequest));
    msg->execute     = GUINT32_TO_LE (EXECUTE_SWITCH_FIREHOSE);

    return sizeof (SaharaCommandExecuteRequest);
}

typedef struct _SaharaCommandExecuteResponse SaharaCommandExecuteResponse;
struct _SaharaCommandExecuteResponse {
    QfuSaharaHeader header;
    guint32         execute;
    guint32         expected_data_length;
} __attribute__ ((packed));

gboolean
qfu_sahara_response_switch_parse (const guint8  *buffer,
                                  gsize          buffer_len,
                                  GError       **error)
{
    SaharaCommandExecuteResponse *msg;

    if (buffer_len < sizeof (SaharaCommandExecuteResponse)) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "message size mismatch: %" G_GSIZE_FORMAT " != %" G_GSIZE_FORMAT,
                     buffer_len, sizeof (SaharaCommandExecuteResponse));
        return FALSE;
    }

    msg = (SaharaCommandExecuteResponse *) buffer;
    g_assert (msg->header.cmd == GUINT32_FROM_LE (QFU_SAHARA_CMD_COMMAND_EXECUTE_RSP));

    g_debug ("[qfu,sahara-message] received %s:", qfu_sahara_cmd_get_string (QFU_SAHARA_CMD_COMMAND_EXECUTE_RSP));
    g_debug ("[qfu,sahara-message]   execute:              0x%08x", GUINT32_FROM_LE (msg->execute));
    g_debug ("[qfu,sahara-message]   expected data length: %u",     GUINT32_FROM_LE (msg->expected_data_length));

    /* Note: the expected data length is the length of the data expected in the next sahara
     * protocol step, the modem is telling us how much data it's going to send; e.g. the EM7565
     * returns just 9 bytes ("confirmed"). Not doing anything else with this value because
     * we don't really need it for anything */

    return TRUE;
}

typedef struct _SaharaCommandExecuteDataRequest SaharaCommandExecuteDataRequest;
struct _SaharaCommandExecuteDataRequest {
    QfuSaharaHeader header;
    guint32         execute;
} __attribute__ ((packed));

gsize
qfu_sahara_request_switch_data_build (guint8 *buffer,
                                      gsize   buffer_len)
{
    SaharaCommandExecuteDataRequest *msg = (SaharaCommandExecuteDataRequest *)buffer;

    g_assert (buffer_len >= sizeof (SaharaCommandExecuteDataRequest));

    msg->header.cmd  = GUINT32_TO_LE (QFU_SAHARA_CMD_COMMAND_EXECUTE_DATA);
    msg->header.size = GUINT32_TO_LE (sizeof (SaharaCommandExecuteDataRequest));
    msg->execute     = GUINT32_TO_LE (EXECUTE_SWITCH_FIREHOSE);

    return sizeof (SaharaCommandExecuteDataRequest);
}

typedef struct _SaharaEndImageTransferResponse SaharaEndImageTransferResponse;
struct _SaharaEndImageTransferResponse {
    QfuSaharaHeader header;
    guint32         file;
    guint32         status;
} __attribute__ ((packed));

gboolean
qfu_sahara_response_end_image_transfer_parse (const guint8   *buffer,
                                              gsize           buffer_len,
                                              GError        **error)
{
    SaharaEndImageTransferResponse *msg;
    QfuSaharaStatus                 status;
    const gchar                    *status_str;

    if (buffer_len < sizeof (SaharaEndImageTransferResponse)) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "message size mismatch: %" G_GSIZE_FORMAT " != %" G_GSIZE_FORMAT,
                     buffer_len, sizeof (SaharaEndImageTransferResponse));
        return FALSE;
    }

    msg = (SaharaEndImageTransferResponse *) buffer;
    g_assert (msg->header.cmd == GUINT32_FROM_LE (QFU_SAHARA_CMD_COMMAND_END_IMAGE_TRANSFER));
    status = (QfuSaharaStatus) GUINT32_FROM_LE (msg->status);
    status_str = qfu_sahara_status_get_string (status);

    g_debug ("[qfu,sahara-message] received %s:", qfu_sahara_cmd_get_string (QFU_SAHARA_CMD_COMMAND_END_IMAGE_TRANSFER));
    g_debug ("[qfu,sahara-message]   file:   %u", GUINT32_FROM_LE (msg->file));
    if (status_str)
        g_debug ("[qfu,sahara-message]   status: %s", status_str);
    else
        g_debug ("[qfu,sahara-message]   status: unknown (0x%08x)", status);

    if (status != QFU_SAHARA_STATUS_SUCCESS) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "operation failed: %s", status_str);
        return FALSE;
    }

    return TRUE;
}
