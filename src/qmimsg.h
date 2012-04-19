/*
 * Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * qmimsg: QMI messages, and how they are read and written.
 *
 * (See qmimsg.c for sources.)
 */

#ifndef LIBQMI_QMIMSG_H
#define LIBQMI_QMIMSG_H

#include <stddef.h>
#include <stdint.h>

struct qmimsg;

/**
 * Callback type used by qmimsg_read to read bytes from the input.
 */
typedef int (*qmimsg_read_fn)(void *context, void *buf, size_t len);

/**
 * Callback type used by qmimsg_write to write bytes to the output.
 */
typedef int (*qmimsg_write_fn)(void *context, const void *buf, size_t len);

/**
 * Creates a new QMI message with the given header data and no TLVs.
 */
int qmimsg_new(uint8_t qmux_flags, uint8_t service, uint8_t client,
               uint8_t qmi_flags, uint16_t transaction, uint16_t message,
               struct qmimsg **message_out);

/**
 * Reads a QMI message from the given input (read_fn and context).
 */
int qmimsg_read(qmimsg_read_fn read_fn, void *context,
                struct qmimsg **message_out);

/**
 * Frees a QMI message returned by qmimsg_new or qmimsg_read.
 */
void qmimsg_free(struct qmimsg *message);

/**
 * Writes a QMI message to the given output (write_fn and context).
 */
int qmimsg_write(struct qmimsg *message,
                 qmimsg_write_fn write_fn, void *context);

/**
 * Prints the contents of a QMI message to stderr for debugging purposes.
 */
void qmimsg_print(struct qmimsg *message);

/**
 * Retrieves a tasteful subset of the header fields in a QMI message.
 */
void qmimsg_get_header(struct qmimsg *message,
                       uint8_t *service_out,
                       uint8_t *client_out,
                       uint8_t *qmi_flags_out,
                       uint16_t *transaction_out,
                       uint16_t *message_out);

/**
 * Finds a TLV element with the given type in the payload of the given QMI
 * message, checks that the length is length, and copies the value into value.
 *
 * Returns 0 if the element was found and was the correct length;
 * EINVAL if the element was found but the length was incorrect;
 * ENOENT if the element was not found.
 */
int qmimsg_tlv_get(struct qmimsg *message,
                   uint8_t type, uint16_t length, void *value);

/**
 * Finds a TLV element with the given type in the payload of the given QMI
 * message, copies the length into length, and copies the value into value.
 *
 * If value is NULL, overwrites length with the length of the message without
 * checking the existing value of length.
 *
 * Returns 0 if the element was found and fit into the buffer;
 * ENOSPC if the element was found but the value was too big;
 * ENOENT if the element was not found.
 */
int qmimsg_tlv_get_varlen(struct qmimsg *message,
                          uint8_t type, uint16_t *length, void *value);

typedef void (*qmimsg_tlv_foreach_fn)(void *context,
                                      uint8_t type,
                                      uint16_t length,
                                      void *value);

void qmimsg_tlv_foreach(struct qmimsg *message,
                        qmimsg_tlv_foreach_fn func, void *context);

/**
 * Appends a TLV element with the given type, length, and value to the payload
 * of the given QMI message.
 *
 * On success, returns zero; on failure, returns errno.
 *
 * Returns 0 if the element was added successfully;
 * ENOSPC if adding the element would overflow one of the length fields.
 */
int qmimsg_tlv_add(struct qmimsg *message,
                   uint8_t type, uint16_t length, const void *buf);

#endif /* LIBQMI_QMIMSG_H */
