/*
 * Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * qmimsg: QMI messages, and how they are read and written.
 *
 * Sources used in writing this file (see README for links):
 * [Gobi]/Core/QMIBuffers.h
 * [GobiNet]/QMI.c
 * [cros-kerne]/drivers/net/usb/gobi/qmi.c
 */

#include "qmimsg.h"

#include <assert.h>
#include <endian.h>
#include <errno.h>
#include <glib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "error.h"
#include "util.h"

#define PACKED __attribute__((packed))

static const uint8_t QMUX_MARKER = 0x01;

struct qmux {
  uint16_t length;
  uint8_t flags;
  uint8_t service;
  uint8_t client;
} PACKED;

struct control_header {
  uint8_t flags;
  uint8_t transaction;
  uint16_t message;
  uint16_t tlv_length;
} PACKED;

struct service_header {
  uint8_t flags;
  uint16_t transaction;
  uint16_t message;
  uint16_t tlv_length;
} PACKED;

struct tlv {
  uint8_t type;
  uint16_t length;
  char value[];
} PACKED;

struct control_message {
  struct control_header header;
  struct tlv tlv[];
} PACKED;

struct service_message {
  struct service_header header;
  struct tlv tlv[];
} PACKED;

struct qmimsg {
  struct {
    uint8_t marker;
    struct qmux qmux;
    union {
      struct control_message control;
      struct service_message service;
    } qmi;
  } PACKED *buf; /* buf allocated using g_malloc, not g_slice_alloc */
  size_t len; /* cached size of *buf; not part of message. */
};

static uint16_t qmux_length(struct qmimsg *m)
{
  return le16toh(m->buf->qmux.length);
}

static void set_qmux_length(struct qmimsg *m, uint16_t length)
{
  m->buf->qmux.length = htole16(length);
}

static uint8_t qmux_flags(struct qmimsg *m) { return m->buf->qmux.flags; }
static uint8_t qmux_service(struct qmimsg *m) { return m->buf->qmux.service; }
static uint8_t qmux_client(struct qmimsg *m) { return m->buf->qmux.client; }

static int qmi_is_control(struct qmimsg *m) { return !m->buf->qmux.service; }

static uint8_t qmi_flags(struct qmimsg *m)
{
  if (qmi_is_control(m))
    return m->buf->qmi.control.header.flags;
  else
    return m->buf->qmi.service.header.flags;
}

static uint16_t qmi_transaction(struct qmimsg *m)
{
  if (qmi_is_control(m))
    return (uint16_t)m->buf->qmi.control.header.transaction;
  else
    return le16toh(m->buf->qmi.service.header.transaction);
}

static uint16_t qmi_message(struct qmimsg *m)
{
  if (qmi_is_control(m))
    return le16toh(m->buf->qmi.control.header.message);
  else
    return le16toh(m->buf->qmi.service.header.message);
}

static uint16_t qmi_tlv_length(struct qmimsg *m)
{
  if (qmi_is_control(m))
    return le16toh(m->buf->qmi.control.header.tlv_length);
  else
    return le16toh(m->buf->qmi.service.header.tlv_length);
}

static void set_qmi_tlv_length(struct qmimsg *m, uint16_t length)
{
  if (qmi_is_control(m))
    m->buf->qmi.control.header.tlv_length = htole16(length);
  else
    m->buf->qmi.service.header.tlv_length = htole16(length);
}

static struct tlv *qmi_tlv(struct qmimsg *m)
{
  if (qmi_is_control(m))
    return m->buf->qmi.control.tlv;
  else
    return m->buf->qmi.service.tlv;
}

static char *qmi_end(struct qmimsg *m)
{
  return (char *)m->buf + m->len;
}

static struct tlv *tlv_next(struct tlv *tlv)
{
  return (struct tlv *)((char *)tlv + sizeof(struct tlv) + tlv->length);
}

static struct tlv *qmi_tlv_first(struct qmimsg *m)
{
  if (qmi_tlv_length(m))
    return qmi_tlv(m);
  else
    return NULL;
}

static struct tlv *qmi_tlv_next(struct qmimsg *m, struct tlv *tlv)
{
  struct tlv *end = (struct tlv *)qmi_end(m);
  struct tlv *next = tlv_next(tlv);
  if (next < end)
    return next;
  else
    return NULL;
}

/**
 * Checks the validity of a QMI message.
 *
 * In particular, checks:
 * 1. The message has space for all required headers.
 * 2. The length of the buffer, the qmux length field, and the QMI tlv_length
 *    field are all consistent.
 * 3. The TLVs in the message fit exactly in the payload size.
 *
 * Returns non-zero if the message is valid, zero if invalid.
 */
static int qmi_check(struct qmimsg *m)
{
  assert(m);
  assert(m->buf);

  if (m->buf->marker != QMUX_MARKER) {
    fprintf(stderr, "qmi_check: marker is incorrect\n");
    return 0;
  }

  if (qmux_length(m) < sizeof(struct qmux)) {
    fprintf(stderr, "qmi_check: QMUX length too short for QMUX header\n");
    return 0;
  }

  /*
   * qmux length is one byte shorter than buffer length because qmux
   * length does not include the qmux frame marker.
   */
  if (qmux_length(m) != m->len - 1) {
    fprintf(stderr, "qmi_check: QMUX length and buffer length don't match\n");
    return 0;
  }

  size_t header_length;
  if (qmi_is_control(m))
    header_length = sizeof(struct qmux) + sizeof(struct control_header);
  else
    header_length = sizeof(struct qmux) + sizeof(struct service_header);

  if (qmux_length(m) < header_length) {
    fprintf(stderr, "qmi_check: QMUX length too short for QMI header\n");
    return 0;
  }

  if (qmux_length(m) - header_length != qmi_tlv_length(m)) {
    fprintf(stderr, "qmi_check: QMUX length and QMI TLV lengths don't match\n");
    return 0;
  }

  char *end = qmi_end(m);
  struct tlv *tlv;
  for (tlv = qmi_tlv(m); tlv < (struct tlv *)end; tlv = tlv_next(tlv)) {
    if (tlv->value > end) {
      fprintf(stderr, "qmi_check: TLV header runs over buffer\n");
      return 0;
    }
    if (tlv->value + tlv->length > end) {
      fprintf(stderr, "qmi_check: TLV value runs over buffer\n");
      return 0;
    }
  }
  /*
   * If this assert triggers, one of the if statements in the loop is wrong.
   * (It shouldn't be reached on malformed QMI messages.)
   */
  assert(tlv == (struct tlv *)end);

  return 1;
}

int qmimsg_new(uint8_t qmux_flags, uint8_t service, uint8_t client,
               uint8_t qmi_flags, uint16_t transaction, uint16_t message,
               struct qmimsg **message_out)
{
  assert(service || (transaction <= UINT8_MAX));

  struct qmimsg *m = g_slice_new(struct qmimsg);

  if (!service)
    m->len = 1 + sizeof(struct qmux) + sizeof(struct control_header);
  else
    m->len = 1 + sizeof(struct qmux) + sizeof(struct service_header);
  m->buf = g_malloc(m->len);

  m->buf->marker = QMUX_MARKER;

  m->buf->qmux.flags = qmux_flags;
  m->buf->qmux.service = service;
  m->buf->qmux.client = client;
  set_qmux_length(m, m->len - 1);

  if (!service) {
    m->buf->qmi.control.header.flags = qmi_flags;
    m->buf->qmi.control.header.transaction = (uint8_t)transaction;
    m->buf->qmi.control.header.message = htole16(message);
  } else {
    m->buf->qmi.service.header.flags = qmi_flags;
    m->buf->qmi.service.header.transaction = htole16(transaction);
    m->buf->qmi.service.header.message = htole16(message);
  }
  set_qmi_tlv_length(m, 0);

  assert(qmi_check(m));

  *message_out = m;

  return 0;
}

int qmimsg_read(qmimsg_read_fn read_fn, void *context,
                struct qmimsg **message_out)
{
  struct qmimsg *m;
  struct { uint8_t marker; struct qmux qmux; } PACKED framed_qmux;
  int result;

  result = read_fn(context, &framed_qmux, sizeof(framed_qmux));
  if (result)
    return result;
  if (framed_qmux.marker != QMUX_MARKER)
    return QMI_ERR_FRAMING_INVALID;

  m = g_slice_new(struct qmimsg);
  m->len = le16toh(framed_qmux.qmux.length) + 1;
  m->buf = g_malloc(m->len);

  memcpy(m->buf, &framed_qmux, sizeof(framed_qmux));
  result = read_fn(context, &m->buf->qmi, m->len - sizeof(framed_qmux));
  if (result) {
    qmimsg_free(m);
    return result;
  }

  if (!qmi_check(m)) {
    qmimsg_free(m);
    return QMI_ERR_HEADER_INVALID;
  }

  *message_out = m;

  return 0;
}

int qmimsg_write(struct qmimsg *m, qmimsg_write_fn write_fn, void *context)
{
  int result;

  assert(m);
  assert(write_fn);
  assert(qmi_check(m));

  result = write_fn(context, m->buf, m->len);
  if (result)
    return result;

  return 0;
}

void qmimsg_get_header(struct qmimsg *message,
                       uint8_t *service_out,
                       uint8_t *client_out,
                       uint8_t *qmi_flags_out,
                       uint16_t *transaction_out,
                       uint16_t *message_out)
{
  assert(message);

  if (service_out)
    *service_out = qmux_service(message);
  if (client_out)
    *client_out = qmux_client(message);
  if (qmi_flags_out)
    *qmi_flags_out = qmi_flags(message);
  if (transaction_out)
    *transaction_out = qmi_transaction(message);
  if (message_out)
    *message_out = qmi_message(message);
}

void qmimsg_print(struct qmimsg *m)
{
  assert(m);

  fprintf(stderr,
          "QMUX: length=0x%04x flags=0x%02x service=0x%02x client=0x%02x\n",
          qmux_length(m),
          qmux_flags(m),
          qmux_service(m),
          qmux_client(m));

  fprintf(stderr,
          "QMI:  flags=0x%02x transaction=0x%04x "
          "message=0x%04x tlv_length=0x%04x\n",
          qmi_flags(m),
          qmi_transaction(m),
          qmi_message(m),
          qmi_tlv_length(m));

  struct tlv *tlv;
  for (tlv = qmi_tlv_first(m); tlv; tlv = qmi_tlv_next(m, tlv)) {
    fprintf(stderr, "TLV:  type=0x%02x length=0x%04x\n",
            tlv->type, tlv->length);
    hexdump(tlv->value, tlv->length);
  }
}

void qmimsg_free(struct qmimsg *message)
{
  assert(message);
  g_free(message->buf);
  g_slice_free(struct qmimsg, message);
}

static int qmimsg_tlv_get_internal(struct qmimsg *message,
                                   uint8_t type,
                                   uint16_t *length,
                                   void *value,
                                   int length_exact)
{
  assert(message);
  assert(message->buf);
  assert(length);

  struct tlv *tlv;
  for (tlv = qmi_tlv_first(message); tlv; tlv = qmi_tlv_next(message, tlv)) {
    if (tlv->type == type) {
      if (length_exact && (tlv->length != *length)) {
        return QMI_ERR_TLV_NOT_FOUND;
      } else if (value && tlv->length > *length) {
        return QMI_ERR_TOO_LONG;
      }
      *length = tlv->length;
      if (value)
        memcpy(value, tlv->value, tlv->length);
      return 0;
    }
  }

  return QMI_ERR_TLV_NOT_FOUND;
}

int qmimsg_tlv_get(struct qmimsg *message,
                   uint8_t type, uint16_t length, void *value)
{
  assert(value);

  return qmimsg_tlv_get_internal(message, type, &length, value, 1);
}

int qmimsg_tlv_get_varlen(struct qmimsg *message,
                          uint8_t type, uint16_t *length, void *value)
{
  return qmimsg_tlv_get_internal(message, type, length, value, 0);
}

void qmimsg_tlv_foreach(struct qmimsg *message,
                        qmimsg_tlv_foreach_fn func, void *context)
{
  assert(message);
  assert(func);

  struct tlv *tlv;
  for (tlv = qmi_tlv_first(message); tlv; tlv = qmi_tlv_next(message, tlv)) {
    func(context, tlv->type, tlv->length, tlv->value);
  }
}


int qmimsg_tlv_add(struct qmimsg *m,
                   uint8_t type, uint16_t length, const void *value)
{
  assert(m);
  assert((length == 0) || value);

  /* Make sure nothing's broken to start. */
  assert(qmi_check(m));

  /* Find length of new TLV. */
  size_t tlv_len = sizeof(struct tlv) + length;

  /* Check for overflow of message size. */
  if (qmux_length(m) + tlv_len > UINT16_MAX) {
    return QMI_ERR_TOO_LONG;
  }

  /* Resize buffer. */
  m->len += tlv_len;
  m->buf = g_realloc(m->buf, m->len);

  /* Fill in new TLV. */
  struct tlv *tlv = (struct tlv *)(qmi_end(m) - tlv_len);
  tlv->type = type;
  tlv->length = length;
  if (value)
    memcpy(tlv->value, value, length);

  /* Update length fields. */
  set_qmux_length(m, (uint16_t)(qmux_length(m) + tlv_len));
  set_qmi_tlv_length(m, (uint16_t)(qmi_tlv_length(m) + tlv_len));

  /* Make sure we didn't break anything. */
  assert(qmi_check(m));

  return 0;
}
