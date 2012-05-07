/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 * Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file of the libqmi library.
 */

/*
 * libqmi-glib -- GLib/GIO based library to control QMI devices
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 * Copyright (C) 2012 Aleksander Morgado <aleksander@lanedo.com>
 */

#include <glib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <endian.h>

#include "qmi-message.h"
#include "qmi-utils.h"
#include "qmi-enum-types.h"
#include "qmi-error-types.h"

#define PACKED __attribute__((packed))

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

struct full_message {
    uint8_t marker;
    struct qmux qmux;
    union {
        struct control_message control;
        struct service_message service;
    } qmi;
} PACKED;

struct _QmiMessage {
    /* TODO: avoid memory split here */
    struct full_message *buf; /* buf allocated using g_malloc, not g_slice_alloc */
    gsize len; /* cached size of *buf; not part of message. */
    volatile gint ref_count; /* the ref count */
};

static inline uint16_t
qmux_length (QmiMessage *self)
{
    return le16toh (self->buf->qmux.length);
}

static inline void
set_qmux_length (QmiMessage *self,
                 uint16_t length)
{
    self->buf->qmux.length = htole16 (length);
}

gboolean
qmi_message_is_control (QmiMessage *self)
{
    g_return_val_if_fail (self != NULL, FALSE);

    return self->buf->qmux.service == QMI_SERVICE_CTL;
}

guint8
qmi_message_get_qmux_flags (QmiMessage *self)
{
    g_return_val_if_fail (self != NULL, 0);

    return self->buf->qmux.flags;
}

QmiService
qmi_message_get_service (QmiMessage *self)
{
    g_return_val_if_fail (self != NULL, QMI_SERVICE_UNKNOWN);

    return (QmiService)self->buf->qmux.service;
}

guint8
qmi_message_get_client_id (QmiMessage *self)
{
    g_return_val_if_fail (self != NULL, 0);

    return self->buf->qmux.client;
}

guint8
qmi_message_get_qmi_flags (QmiMessage *self)
{
    g_return_val_if_fail (self != NULL, 0);

    if (qmi_message_is_control (self))
        return self->buf->qmi.control.header.flags;

    return self->buf->qmi.service.header.flags;
}

gboolean
qmi_message_is_response (QmiMessage *self)
{
    if (qmi_message_is_control (self)) {
        if (self->buf->qmi.control.header.flags & QMI_CTL_FLAG_RESPONSE)
            return TRUE;
    } else {
        if (self->buf->qmi.service.header.flags & QMI_SERVICE_FLAG_RESPONSE)
            return TRUE;
    }

    return FALSE;
}

gboolean
qmi_message_is_indication (QmiMessage *self)
{
    if (qmi_message_is_control (self)) {
        if (self->buf->qmi.control.header.flags & QMI_CTL_FLAG_INDICATION)
            return TRUE;
    } else {
        if (self->buf->qmi.service.header.flags & QMI_SERVICE_FLAG_INDICATION)
            return TRUE;
    }

    return FALSE;
}

guint16
qmi_message_get_transaction_id (QmiMessage *self)
{
    g_return_val_if_fail (self != NULL, 0);

    if (qmi_message_is_control (self))
        /* note: only 1 byte for transaction in CTL message */
        return (guint16)self->buf->qmi.control.header.transaction;

    return le16toh (self->buf->qmi.service.header.transaction);
}

guint16
qmi_message_get_message_id (QmiMessage *self)
{
    g_return_val_if_fail (self != NULL, 0);

    if (qmi_message_is_control (self))
        return le16toh (self->buf->qmi.control.header.message);

    return le16toh (self->buf->qmi.service.header.message);
}

gsize
qmi_message_get_length (QmiMessage *self)
{
    g_return_val_if_fail (self != NULL, 0);

    return self->len;
}

static uint16_t
qmi_tlv_length (QmiMessage *self)
{
    if (qmi_message_is_control (self))
        return le16toh (self->buf->qmi.control.header.tlv_length);

    return le16toh (self->buf->qmi.service.header.tlv_length);
}

static void
set_qmi_tlv_length (QmiMessage *self,
                    uint16_t length)
{
    if (qmi_message_is_control (self))
        self->buf->qmi.control.header.tlv_length = htole16 (length);
    else
        self->buf->qmi.service.header.tlv_length = htole16 (length);
}

static struct tlv *
qmi_tlv (QmiMessage *self)
{
    if (qmi_message_is_control (self))
        return self->buf->qmi.control.tlv;

    return self->buf->qmi.service.tlv;
}

static char *
qmi_end (QmiMessage *self)
{
    return (char *) self->buf + self->len;
}

static struct tlv *
tlv_next (struct tlv *tlv)
{
    return (struct tlv *)((char *)tlv + sizeof(struct tlv) + le16toh (tlv->length));
}

static struct tlv *
qmi_tlv_first (QmiMessage *self)
{
    if (qmi_tlv_length (self))
        return qmi_tlv (self);

    return NULL;
}

static struct tlv *
qmi_tlv_next (QmiMessage *self,
              struct tlv *tlv)
{
    struct tlv *end;
    struct tlv *next;

    end = (struct tlv *) qmi_end (self);
    next = tlv_next (tlv);

    return (next < end ? next : NULL);
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
gboolean
qmi_message_check (QmiMessage *self,
                   GError **error)
{
    size_t header_length;
    char *end;
    struct tlv *tlv;

    g_assert (self != NULL);
    g_assert (self->buf != NULL);

    if (self->buf->marker != QMI_MESSAGE_QMUX_MARKER) {
        g_set_error (error,
                     QMI_CORE_ERROR,
                     QMI_CORE_ERROR_INVALID_MESSAGE,
                     "Marker is incorrect");
        return FALSE;
    }

    if (qmux_length (self) < sizeof (struct qmux)) {
        g_set_error (error,
                     QMI_CORE_ERROR,
                     QMI_CORE_ERROR_INVALID_MESSAGE,
                     "QMUX length too short for QMUX header (%u < %u)",
                     qmux_length (self), sizeof (struct qmux));
        return FALSE;
    }

    /*
     * qmux length is one byte shorter than buffer length because qmux
     * length does not include the qmux frame marker.
     */
    if (qmux_length (self) != self->len - 1) {
        g_set_error (error,
                     QMI_CORE_ERROR,
                     QMI_CORE_ERROR_INVALID_MESSAGE,
                     "QMUX length and buffer length don't match (%u != %u)",
                     qmux_length (self), self->len - 1);
        return FALSE;
    }

    header_length = sizeof (struct qmux) + (qmi_message_is_control (self) ?
                                            sizeof (struct control_header) :
                                            sizeof (struct service_header));

    if (qmux_length (self) < header_length) {
        g_set_error (error,
                     QMI_CORE_ERROR,
                     QMI_CORE_ERROR_INVALID_MESSAGE,
                     "QMUX length too short for QMI header (%u < %u)",
                     qmux_length (self), header_length);
        return FALSE;
    }

    if (qmux_length (self) - header_length != qmi_tlv_length (self)) {
        g_set_error (error,
                     QMI_CORE_ERROR,
                     QMI_CORE_ERROR_INVALID_MESSAGE,
                     "QMUX length and QMI TLV lengths don't match (%u - %u != %u)",
                     qmux_length (self), header_length, qmi_tlv_length (self));
        return FALSE;
    }

    end = qmi_end (self);
    for (tlv = qmi_tlv (self); tlv < (struct tlv *)end; tlv = tlv_next (tlv)) {
        if (tlv->value > end) {
            g_set_error (error,
                         QMI_CORE_ERROR,
                         QMI_CORE_ERROR_INVALID_MESSAGE,
                         "TLV header runs over buffer (%p > %p)",
                         tlv->value, end);
            return FALSE;
        }
        if (tlv->value + le16toh (tlv->length) > end) {
            g_set_error (error,
                         QMI_CORE_ERROR,
                         QMI_CORE_ERROR_INVALID_MESSAGE,
                         "TLV value runs over buffer (%p + %u  > %p)",
                         tlv->value, le16toh (tlv->length), end);
            return FALSE;
        }
    }

    /*
     * If this assert triggers, one of the if statements in the loop is wrong.
     * (It shouldn't be reached on malformed QMI messages.)
     */
    g_assert (tlv == (struct tlv *)end);

    return TRUE;
}

QmiMessage *
qmi_message_new (QmiService service,
                 guint8 client_id,
                 guint16 transaction_id,
                 guint16 message_id)
{
    QmiMessage *self;

    /* Transaction ID in the control service is 8bit only */
    g_assert (service != QMI_SERVICE_CTL ||
              transaction_id <= G_MAXUINT8);

    self = g_slice_new (QmiMessage);
    self->ref_count = 1;

    self->len = 1 + sizeof (struct qmux) + (service == QMI_SERVICE_CTL ?
                                            sizeof (struct control_header) :
                                            sizeof (struct service_header));

    /* TODO: Allocate both the message and the buffer together */
    self->buf = g_malloc (self->len);

    self->buf->marker = QMI_MESSAGE_QMUX_MARKER;
    self->buf->qmux.flags = 0;
    self->buf->qmux.service = service;
    self->buf->qmux.client = client_id;
    set_qmux_length (self, self->len - 1);

    if (service == QMI_SERVICE_CTL) {
        self->buf->qmi.control.header.flags = 0;
        self->buf->qmi.control.header.transaction = (uint8_t)transaction_id;
        self->buf->qmi.control.header.message = htole16 (message_id);
    } else {
        self->buf->qmi.service.header.flags = 0;
        self->buf->qmi.service.header.transaction = htole16 (transaction_id);
        self->buf->qmi.service.header.message = htole16 (message_id);
    }

    set_qmi_tlv_length (self, 0);

    g_assert (qmi_message_check (self, NULL));

    return self;
}

QmiMessage *
qmi_message_ref (QmiMessage *self)
{
    g_assert (self != NULL);

    g_atomic_int_inc (&self->ref_count);
    return self;
}

void
qmi_message_unref (QmiMessage *self)
{
    g_assert (self != NULL);

    if (g_atomic_int_dec_and_test (&self->ref_count)) {
        g_free (self->buf);
        g_slice_free (QmiMessage, self);
    }
}

gconstpointer
qmi_message_get_raw (QmiMessage *self,
                     gsize *len,
                     GError **error)
{
    g_assert (self != NULL);
    g_assert (len != NULL);

    if (!qmi_message_check (self, error))
        return NULL;

    *len = self->len;
    return self->buf;
}

static gboolean
qmimsg_tlv_get_internal (QmiMessage *self,
                         guint8 type,
                         guint16 *length,
                         gpointer value,
                         gboolean length_exact,
                         GError **error)
{
    struct tlv *tlv;

    g_assert (self != NULL);
    g_assert (self->buf != NULL);
    g_assert (length != NULL);
    /* note: we allow querying only for the exact length */

    for (tlv = qmi_tlv_first (self); tlv; tlv = qmi_tlv_next (self, tlv)) {
        if (tlv->type == type) {
            if (length_exact && (le16toh (tlv->length) != *length)) {
                g_set_error (error,
                             QMI_CORE_ERROR,
                             QMI_CORE_ERROR_TLV_NOT_FOUND,
                             "TLV found but wrong length (%u != %u)",
                             tlv->length,
                             *length);
                return FALSE;
            } else if (value && le16toh (tlv->length) > *length) {
                g_set_error (error,
                             QMI_CORE_ERROR,
                             QMI_CORE_ERROR_TLV_TOO_LONG,
                             "TLV found but too long (%u > %u)",
                             le16toh (tlv->length),
                             *length);
                return FALSE;
            }

            *length = le16toh (tlv->length);
            if (value)
                memcpy (value, tlv->value, le16toh (tlv->length));
            return TRUE;
        }
    }

    g_set_error (error,
                 QMI_CORE_ERROR,
                 QMI_CORE_ERROR_TLV_TOO_LONG,
                 "TLV not found");
    return FALSE;
}

gboolean
qmi_message_tlv_get (QmiMessage *self,
                     guint8 type,
                     guint16 length,
                     gpointer value,
                     GError **error)
{
    return qmimsg_tlv_get_internal (self, type, &length, value, TRUE, error);
}

gboolean
qmi_message_tlv_get_varlen (QmiMessage *self,
                            guint8 type,
                            guint16 *length,
                            gpointer value,
                            GError **error)
{
    return qmimsg_tlv_get_internal (self, type, length, value, FALSE, error);
}

gchar *
qmi_message_tlv_get_string (QmiMessage *self,
                            guint8 type,
                            GError **error)
{
    uint16_t length;
    gchar *value;

    /* Read length only first */
    if (!qmi_message_tlv_get_varlen (self, type, &length, NULL, error))
        return NULL;

    /* Read exact length and value */
    value = g_malloc (length + 1);
    if (!qmi_message_tlv_get (self, type, length, value, error)) {
        g_free (value);
        return NULL;
    }
    value[length] = '\0';

    return value;
}

void
qmi_message_tlv_foreach (QmiMessage *self,
                         QmiMessageForeachTlvFn callback,
                         gpointer user_data)
{
    struct tlv *tlv;

    g_assert (self != NULL);
    g_assert (callback != NULL);

    for (tlv = qmi_tlv_first (self); tlv; tlv = qmi_tlv_next (self, tlv)) {
        callback (tlv->type,
                  (gsize)(le16toh (tlv->length)),
                  (gconstpointer)tlv->value,
                  user_data);
    }
}

gboolean
qmi_message_tlv_add (QmiMessage *self,
                     guint8 type,
                     gsize length,
                     gconstpointer value,
                     GError **error)
{
    size_t tlv_len;
    struct tlv *tlv;

    g_assert (self != NULL);
    g_assert ((length == 0) || value != NULL);

    /* Make sure nothing's broken to start. */
    if (!qmi_message_check (self, error)) {
        g_prefix_error (error, "Invalid QMI message detected: ");
        return FALSE;
    }

    /* Find length of new TLV. */
    tlv_len = sizeof (struct tlv) + length;

    /* Check for overflow of message size. */
    if (qmux_length (self) + tlv_len > UINT16_MAX) {
        g_set_error (error,
                     QMI_CORE_ERROR,
                     QMI_CORE_ERROR_TLV_TOO_LONG,
                     "TLV to add is too long");
        return FALSE;
    }

    /* Resize buffer. */
    self->len += tlv_len;
    self->buf = g_realloc (self->buf, self->len);

    /* Fill in new TLV. */
    tlv = (struct tlv *)(qmi_end (self) - tlv_len);
    tlv->type = type;
    tlv->length = htole16 (length);
    if (value)
        memcpy (tlv->value, value, length);

    /* Update length fields. */
    set_qmux_length (self, (uint16_t)(qmux_length (self) + tlv_len));
    set_qmi_tlv_length (self, (uint16_t)(qmi_tlv_length(self) + tlv_len));

    /* Make sure we didn't break anything. */
    if (!qmi_message_check (self, error)) {
        g_prefix_error (error, "Invalid QMI message built: ");
        return FALSE;
    }

    return TRUE;
}

QmiMessage *
qmi_message_new_from_raw (const guint8 *raw,
                          gsize raw_len)
{
    QmiMessage *self;
    gsize message_len;
    struct qmux *qmux;

    /* If we didn't even read the header, leave */
    if (raw_len < (sizeof (struct qmux) + 1))
        return NULL;

    /* We need to have read the length reported by the header.
     * Otherwise, return. */
    message_len = le16toh (((struct full_message *)raw)->qmux.length);
    if (raw_len < (message_len - 1))
        return NULL;

    /* Ok, so we should have all the data available already */
    self = g_slice_new (QmiMessage);
    self->ref_count = 1;
    self->len = message_len + 1;
    self->buf = g_malloc (self->len);
    memcpy (self->buf, raw, self->len);

    /* NOTE: we don't check if the message is valid here, let the caller do it */

    return self;
}

gchar *
qmi_message_get_printable (QmiMessage *self,
                           const gchar *line_prefix)
{
    GString *printable;
    gchar *qmi_flags_str;
    const gchar *qmi_message_str;
    struct tlv *tlv;

    if (!qmi_message_check (self, NULL))
        return NULL;

    if (!line_prefix)
        line_prefix = "";

    printable = g_string_new ("");
    g_string_append_printf (printable,
                            "%sQMUX:\n"
                            "%s  length  = %u (0x%04x)\n"
                            "%s  flags   = 0x%02x\n"
                            "%s  service = \"%s\" (0x%02x)\n"
                            "%s  client  = %u (0x%02x)\n",
                            line_prefix,
                            line_prefix, qmux_length (self), qmux_length (self),
                            line_prefix, qmi_message_get_qmux_flags (self),
                            line_prefix, qmi_service_get_string (qmi_message_get_service (self)), qmi_message_get_service (self),
                            line_prefix, qmi_message_get_client_id (self), qmi_message_get_client_id (self));

    if (qmi_message_get_service (self) == QMI_SERVICE_CTL)
        qmi_flags_str = qmi_ctl_flag_build_string_from_mask (qmi_message_get_qmi_flags (self));
    else
        qmi_flags_str = qmi_service_flag_build_string_from_mask (qmi_message_get_qmi_flags (self));

    switch (qmi_message_get_service (self)) {
    case QMI_SERVICE_CTL:
        qmi_message_str = qmi_ctl_message_get_string (qmi_message_get_message_id (self));
        break;
    case QMI_SERVICE_DMS:
        qmi_message_str = qmi_dms_message_get_string (qmi_message_get_message_id (self));
        break;
    case QMI_SERVICE_WDS:
        qmi_message_str = qmi_wds_message_get_string (qmi_message_get_message_id (self));
        break;
    default:
        qmi_message_str = "unknown";
        break;
    }

    g_string_append_printf (printable,
                            "%sQMI:\n"
                            "%s  flags       = \"%s\" (0x%02x)\n"
                            "%s  transaction = %u (0x%04x)\n"
                            "%s  message     = \"%s\" (0x%04x)\n"
                            "%s  tlv_length  = %u (0x%04x)\n",
                            line_prefix,
                            line_prefix, qmi_flags_str, qmi_message_get_qmi_flags (self),
                            line_prefix, qmi_message_get_transaction_id (self), qmi_message_get_transaction_id (self),
                            line_prefix, qmi_message_str, qmi_message_get_message_id (self),
                            line_prefix, qmi_tlv_length (self), qmi_tlv_length (self));
    g_free (qmi_flags_str);

    for (tlv = qmi_tlv_first (self); tlv; tlv = qmi_tlv_next (self, tlv)) {
        gchar *value_hex;

        value_hex = qmi_utils_str_hex (tlv->value, le16toh (tlv->length), ':');
        g_string_append_printf (printable,
                                "%sTLV:\n"
                                "%s  type   = 0x%02x\n"
                                "%s  length = %u (0x%04x)\n"
                                "%s  value  = %s\n",
                                line_prefix,
                                line_prefix, tlv->type,
                                line_prefix, le16toh (tlv->length), le16toh (tlv->length),
                                line_prefix, value_hex);
        g_free (value_hex);
    }

    return g_string_free (printable, FALSE);
}

/*****************************************************************************/
/* QMI protocol errors handling */

struct qmi_result {
  uint16_t status;
  uint16_t error;
} PACKED;

/* 0x02 is the TLV for the result of any request.
   (grep "Result Code" [Gobi3000API]/Database/QMI/Entity.txt) */
#define QMI_TLV_RESULT_CODE ((uint8_t)0x02)

enum {
  QMI_STATUS_SUCCESS = 0x0000,
  QMI_STATUS_FAILURE = 0x0001
};

gboolean
qmi_message_get_response_result (QmiMessage *self,
                                 GError **error)
{
    struct qmi_result msg_result;

    g_assert (self != NULL);

    if (!qmi_message_is_response (self)) {
        g_set_error (error,
                     QMI_CORE_ERROR,
                     QMI_CORE_ERROR_INVALID_MESSAGE,
                     "Cannot get result code from non-response message");
        return FALSE;
    }

    if (!qmi_message_tlv_get (self,
                              QMI_TLV_RESULT_CODE,
                              sizeof (msg_result),
                              &msg_result,
                              error)) {
        g_prefix_error (error, "Couldn't get result code: ");
        return FALSE;
    }

    switch (le16toh (msg_result.status)) {
    case QMI_STATUS_SUCCESS:
        /* Operation succeeded */
        return TRUE;

    case QMI_STATUS_FAILURE:
        /* Report a QMI protocol error */
        g_set_error (error,
                     QMI_PROTOCOL_ERROR,
                     (QmiProtocolError)le16toh (msg_result.error),
                     "QMI protocol error (%u): '%s'",
                     (guint) le16toh (msg_result.error),
                     qmi_protocol_error_get_string ((QmiProtocolError) le16toh (msg_result.error)));
        return FALSE;

    default:
        g_set_error (error,
                     QMI_CORE_ERROR,
                     QMI_CORE_ERROR_INVALID_MESSAGE,
                     "Unexpected result status (%u)",
                     (guint) le16toh (msg_result.status));
        return FALSE;
    }
}
