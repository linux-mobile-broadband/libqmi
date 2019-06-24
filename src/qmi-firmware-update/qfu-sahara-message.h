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
 *
 * The definitions in this header file are imported from libopenpst:
 *    https://github.com/openpst/libopenpst
 * And from CodeAurora's quic/imm/imm/qdl:
 *    https://portland.source.codeaurora.org/quic/imm/imm/qdl
 */

#ifndef QFU_SAHARA_MESSAGE_H
#define QFU_SAHARA_MESSAGE_H

#include <glib-object.h>
#include <gio/gio.h>

#include "qfu-image.h"

G_BEGIN_DECLS

typedef struct _QfuSaharaHeader QfuSaharaHeader;
struct _QfuSaharaHeader {
    guint32 cmd;
    guint32 size;
} __attribute__ ((packed));

#define QFU_SAHARA_MESSAGE_MAX_HEADER_SIZE sizeof (QfuSaharaHeader)
#define QFU_SAHARA_MESSAGE_MAX_PACKET_SIZE 0x400

typedef enum {
    QFU_SAHARA_CMD_HELLO_REQ                  = 0x01, /* Initialize connection and protocol                                     */
    QFU_SAHARA_CMD_HELLO_RSP                  = 0x02, /* Acknowledge connection/protocol, mode of operation                     */
    QFU_SAHARA_CMD_COMMAND_READ_DATA          = 0x03, /* Read specified number of bytes from host                               */
    QFU_SAHARA_CMD_COMMAND_END_IMAGE_TRANSFER = 0x04, /* image transfer end / target transfer failure                           */
    QFU_SAHARA_CMD_COMMAND_DONE               = 0x05, /* Acknowledgement: image transfer is complete                            */
    QFU_SAHARA_CMD_COMMAND_DONE_RESPONSE      = 0x06, /* Target is exiting protocol                                             */
    QFU_SAHARA_CMD_COMMAND_RESET              = 0x07, /* Instruct target to perform a reset                                     */
    QFU_SAHARA_CMD_COMMAND_RESET_RESPONSE     = 0x08, /* Indicate to host that target is about to reset                         */
    QFU_SAHARA_CMD_COMMAND_MEMORY_DEBUG       = 0x09, /* Indicate to host: target debug mode & ready to transfer memory content */
    QFU_SAHARA_CMD_COMMAND_MEMORY_READ        = 0x0A, /* Read number of bytes, starting from a specified address                */
    QFU_SAHARA_CMD_COMMAND_READY              = 0x0B, /* Indicate to host: target ready to receive client commands,             */
    QFU_SAHARA_CMD_COMMAND_SWITCH_MODE        = 0x0C, /* Switch to a mode defined in enum SAHARA_MODE                           */
    QFU_SAHARA_CMD_COMMAND_EXECUTE_REQ        = 0x0D, /* Indicate to host: to execute a given client command                    */
    QFU_SAHARA_CMD_COMMAND_EXECUTE_RSP        = 0x0E, /* Indicate to host: target command execution status                      */
    QFU_SAHARA_CMD_COMMAND_EXECUTE_DATA       = 0x0F, /* Indicate to target that host is ready to receive (more) data           */
    QFU_SAHARA_CMD_COMMAND_MEMORY_DEBUG_64    = 0x10,
    QFU_SAHARA_CMD_COMMAND_MEMORY_READ_64     = 0x11,
} QfuSaharaCmd;

typedef enum {
    QFU_SAHARA_MODE_IMAGE_TX_PENDING  = 0x00,
    QFU_SAHARA_MODE_IMAGE_TX_COMPLETE = 0x01,
    QFU_SAHARA_MODE_MEMORY_DEBUG      = 0x02,
    QFU_SAHARA_MODE_COMMAND           = 0x03,
} QfuSaharaMode;

typedef enum {
    QFU_SAHARA_STATUS_SUCCESS                     = 0x00,
    QFU_SAHARA_STATUS_INVALID_COMMAND             = 0x01,
    QFU_SAHARA_STATUS_PROTOCOL_MISMATCH           = 0x02,
    QFU_SAHARA_STATUS_INVALID_TARGET_PROTOCOL     = 0x03,
    QFU_SAHARA_STATUS_INVALID_HOST_PROTOCOL       = 0x04,
    QFU_SAHARA_STATUS_INVALID_PACKET_SIZE         = 0x05,
    QFU_SAHARA_STATUS_UNEXPECTED_IMAGE_ID         = 0x06,
    QFU_SAHARA_STATUS_INVALID_HEADER_SIZE         = 0x07,
    QFU_SAHARA_STATUS_INVALID_DATA_SIZE           = 0x08,
    QFU_SAHARA_STATUS_INVALID_IMAGE_TYPE          = 0x09,
    QFU_SAHARA_STATUS_INVALID_TX_LENGTH           = 0x0A,
    QFU_SAHARA_STATUS_INVALID_RX_LENGTH           = 0x0B,
    QFU_SAHARA_STATUS_TX_RX_ERROR                 = 0x0C,
    QFU_SAHARA_STATUS_READ_DATA_ERROR             = 0x0D,
    QFU_SAHARA_STATUS_UNSUPPORTED_NUM_PHDRS       = 0x0E,
    QFU_SAHARA_STATUS_INVALID_PHDR_SIZE           = 0x0F,
    QFU_SAHARA_STATUS_MULTIPLE_SHARED_SEG         = 0x10,
    QFU_SAHARA_STATUS_UNINIT_PHDR_LOC             = 0x11,
    QFU_SAHARA_STATUS_INVALID_DEST_ADDRESS        = 0x12,
    QFU_SAHARA_STATUS_INVALID_IMAGE_HEADER_SIZE   = 0x13,
    QFU_SAHARA_STATUS_INVALID_ELF_HEADER          = 0x14,
    QFU_SAHARA_STATUS_UNKNOWN_ERROR               = 0x15,
    QFU_SAHARA_STATUS_TIMEOUT_RX                  = 0x16,
    QFU_SAHARA_STATUS_TIMEOUT_TX                  = 0x17,
    QFU_SAHARA_STATUS_INVALID_MODE                = 0x18,
    QFU_SAHARA_STATUS_INVALID_MEMORY_READ         = 0x19,
    QFU_SAHARA_STATUS_INVALID_DATA_SIZE_REQUEST   = 0x1A,
    QFU_SAHARA_STATUS_MEMORY_DEBUG_NOT_SUPPORTED  = 0x1B,
    QFU_SAHARA_STATUS_INVALID_MODE_SWITCH         = 0x1C,
    QFU_SAHARA_STATUS_EXEC_FAILURE                = 0x1D,
    QFU_SAHARA_STATUS_EXEC_CMD_INVALID_PARAM      = 0x1E,
    QFU_SAHARA_STATUS_EXEC_CMD_UNSUPPORTED        = 0x1F,
    QFU_SAHARA_STATUS_EXEC_DATA_INVALID           = 0x20,
    QFU_SAHARA_STATUS_HASH_TABLE_AUTH_FAILURE     = 0x21,
    QFU_SAHARA_STATUS_HASH_VERIFICATION_FAILURE   = 0x22,
    QFU_SAHARA_STATUS_HASH_TABLE_NOT_FOUND        = 0x23,
    QFU_SAHARA_STATUS_TARGET_INIT_FAILURE         = 0x24,
    QFU_SAHARA_STATUS_IMAGE_AUTH_FAILURE          = 0x25,
    QFU_SAHARA_STATUS_INVALID_IMG_HASH_TABLE_SIZE = 0x26,
} QfuSaharaStatus;

gsize    qfu_sahara_response_hello_build              (guint8        *buffer,
                                                       gsize          buffer_len);
gsize    qfu_sahara_request_switch_build              (guint8        *buffer,
                                                       gsize          buffer_len);
gsize    qfu_sahara_request_switch_data_build         (guint8        *buffer,
                                                       gsize          buffer_len);
gboolean qfu_sahara_request_hello_parse               (const guint8  *buffer,
                                                       gsize          buffer_len,
                                                       GError       **error);
gboolean qfu_sahara_response_switch_parse             (const guint8  *buffer,
                                                       gsize          buffer_len,
                                                       GError       **error);
gboolean qfu_sahara_response_end_image_transfer_parse (const guint8  *buffer,
                                                       gsize          buffer_len,
                                                       GError       **error);

G_END_DECLS

#endif /* QFU_SAHARA_MESSAGE_H */
