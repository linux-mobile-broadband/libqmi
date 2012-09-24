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

#include "qmi-ctl.h"
#include "qmi-dms.h"
#include "qmi-wds.h"
#include "qmi-nas.h"
#include "qmi-wms.h"
#include "qmi-pds.h"

#define PACKED __attribute__((packed))

struct qmux {
  guint16 length;
  guint8 flags;
  guint8 service;
  guint8 client;
} PACKED;

struct control_header {
  guint8 flags;
  guint8 transaction;
  guint16 message;
  guint16 tlv_length;
} PACKED;

struct service_header {
  guint8 flags;
  guint16 transaction;
  guint16 message;
  guint16 tlv_length;
} PACKED;

struct tlv {
  guint8 type;
  guint16 length;
  guint8 value[];
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
    guint8 marker;
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

guint16
qmi_message_get_qmux_length (QmiMessage *self)
{
    return GUINT16_FROM_LE (self->buf->qmux.length);
}

static inline void
set_qmux_length (QmiMessage *self,
                 guint16 length)
{
    self->buf->qmux.length = GUINT16_TO_LE (length);
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

guint16
qmi_message_get_tlv_length (QmiMessage *self)
{
    if (qmi_message_is_control (self))
        return GUINT16_FROM_LE (self->buf->qmi.control.header.tlv_length);

    return GUINT16_FROM_LE (self->buf->qmi.service.header.tlv_length);
}

static void
set_qmi_message_get_tlv_length (QmiMessage *self,
                                guint16 length)
{
    if (qmi_message_is_control (self))
        self->buf->qmi.control.header.tlv_length = GUINT16_TO_LE (length);
    else
        self->buf->qmi.service.header.tlv_length = GUINT16_TO_LE (length);
}

static struct tlv *
qmi_tlv (QmiMessage *self)
{
    if (qmi_message_is_control (self))
        return self->buf->qmi.control.tlv;

    return self->buf->qmi.service.tlv;
}

static guint8 *
qmi_end (QmiMessage *self)
{
    return (guint8 *) self->buf + self->len;
}

static struct tlv *
tlv_next (struct tlv *tlv)
{
    return (struct tlv *)((guint8 *)tlv + sizeof(struct tlv) + le16toh (tlv->length));
}

static struct tlv *
qmi_tlv_first (QmiMessage *self)
{
    if (qmi_message_get_tlv_length (self))
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
    gsize header_length;
    guint8 *end;
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

    if (qmi_message_get_qmux_length (self) < sizeof (struct qmux)) {
        g_set_error (error,
                     QMI_CORE_ERROR,
                     QMI_CORE_ERROR_INVALID_MESSAGE,
                     "QMUX length too short for QMUX header (%u < %" G_GSIZE_FORMAT ")",
                     qmi_message_get_qmux_length (self), sizeof (struct qmux));
        return FALSE;
    }

    /*
     * qmux length is one byte shorter than buffer length because qmux
     * length does not include the qmux frame marker.
     */
    if (qmi_message_get_qmux_length (self) != self->len - 1) {
        g_set_error (error,
                     QMI_CORE_ERROR,
                     QMI_CORE_ERROR_INVALID_MESSAGE,
                     "QMUX length and buffer length don't match (%u != %" G_GSIZE_FORMAT ")",
                     qmi_message_get_qmux_length (self), self->len - 1);
        return FALSE;
    }

    header_length = sizeof (struct qmux) + (qmi_message_is_control (self) ?
                                            sizeof (struct control_header) :
                                            sizeof (struct service_header));

    if (qmi_message_get_qmux_length (self) < header_length) {
        g_set_error (error,
                     QMI_CORE_ERROR,
                     QMI_CORE_ERROR_INVALID_MESSAGE,
                     "QMUX length too short for QMI header (%u < %" G_GSIZE_FORMAT ")",
                     qmi_message_get_qmux_length (self), header_length);
        return FALSE;
    }

    if (qmi_message_get_qmux_length (self) - header_length != qmi_message_get_tlv_length (self)) {
        g_set_error (error,
                     QMI_CORE_ERROR,
                     QMI_CORE_ERROR_INVALID_MESSAGE,
                     "QMUX length and QMI TLV lengths don't match (%u - %" G_GSIZE_FORMAT " != %u)",
                     qmi_message_get_qmux_length (self), header_length, qmi_message_get_tlv_length (self));
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
        self->buf->qmi.control.header.transaction = (guint8)transaction_id;
        self->buf->qmi.control.header.message = htole16 (message_id);
    } else {
        self->buf->qmi.service.header.flags = 0;
        self->buf->qmi.service.header.transaction = htole16 (transaction_id);
        self->buf->qmi.service.header.message = htole16 (message_id);
    }

    set_qmi_message_get_tlv_length (self, 0);

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

/**
 * qmi_message_get_raw_tlv:
 * @self: a #QmiMessage.
 * @type: specific ID of the TLV to get.
 * @length: (out): return location for the length of the TLV.
 * @error: return location for error or %NULL.
 *
 * Get the raw data buffer of a specific TLV within the #QmiMessage.
 *
 * Returns: (transfer none): The raw data buffer of the TLV, or #NULL if not found.
 */
const guint8 *
qmi_message_get_raw_tlv (QmiMessage *self,
                         guint8 type,
                         guint16 *length)
{
    struct tlv *tlv;

    g_return_val_if_fail (self != NULL, NULL);
    g_return_val_if_fail (length != NULL, NULL);

    for (tlv = qmi_tlv_first (self); tlv; tlv = qmi_tlv_next (self, tlv)) {
        if (tlv->type == type) {
            *length = GUINT16_FROM_LE (tlv->length);
            return (guint8 *)&(tlv->value[0]);
        }
    }

    return NULL;
}

/**
 * qmi_message_foreach_raw_tlv:
 * @self: a #QmiMessage.
 * @func: the function to call for each TLV.
 * @user_data: user data to pass to the function.
 *
 * Calls the given function for each TLV found within the #QmiMessage.
 */
void
qmi_message_foreach_raw_tlv (QmiMessage *self,
                             QmiMessageForeachRawTlvFn func,
                             gpointer user_data)
{
    struct tlv *tlv;

    g_return_if_fail (self != NULL);
    g_return_if_fail (func != NULL);

    for (tlv = qmi_tlv_first (self); tlv; tlv = qmi_tlv_next (self, tlv)) {
        func (tlv->type,
              (const guint8 *)tlv->value,
              (gsize)(GUINT16_FROM_LE (tlv->length)),
              user_data);
    }
}

/**
 * qmi_message_add_raw_tlv:
 * @self: a #QmiMessage.
 * @type: specific ID of the TLV to add.
 * @raw: raw data buffer with the value of the TLV.
 * @length: length of the raw data buffer.
 * @error: return location for error or %NULL.
 *
 * Creates a new @type TLV with the value given in @raw, and adds it to the #QmiMessage.
 *
 * Returns: #TRUE if the TLV as successfully added, otherwise #FALSE is returned and @error is set.
 */
gboolean
qmi_message_add_raw_tlv (QmiMessage *self,
                         guint8 type,
                         const guint8 *raw,
                         gsize length,
                         GError **error)
{
    size_t tlv_len;
    struct tlv *tlv;

    g_return_val_if_fail (self != NULL, FALSE);
    g_return_val_if_fail (raw != NULL, FALSE);
    g_return_val_if_fail (length > 0, FALSE);

    /* Make sure nothing's broken to start. */
    if (!qmi_message_check (self, error)) {
        g_prefix_error (error, "Invalid QMI message detected: ");
        return FALSE;
    }

    /* Find length of new TLV */
    tlv_len = sizeof (struct tlv) + length;

    /* Check for overflow of message size. */
    if (qmi_message_get_qmux_length (self) + tlv_len > G_MAXUINT16) {
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
    tlv->length = GUINT16_TO_LE (length);
    memcpy (tlv->value, raw, length);

    /* Update length fields. */
    set_qmux_length (self, (guint16)(qmi_message_get_qmux_length (self) + tlv_len));
    set_qmi_message_get_tlv_length (self, (guint16)(qmi_message_get_tlv_length(self) + tlv_len));

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

    /* If we didn't even read the QMUX header (comes after the 1-byte marker),
     * leave */
    if (raw_len < (sizeof (struct qmux) + 1))
        return NULL;

    /* We need to have read the length reported by the QMUX header (plus the
     * initial 1-byte marker) */
    message_len = GUINT16_FROM_LE (((struct full_message *)raw)->qmux.length);
    if (raw_len < (message_len + 1))
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
qmi_message_get_tlv_printable (QmiMessage *self,
                               const gchar *line_prefix,
                               guint8 type,
                               gsize length,
                               gconstpointer value)
{
    gchar *printable;
    gchar *value_hex;

    value_hex = qmi_utils_str_hex (value, length, ':');
    printable = g_strdup_printf ("%sTLV:\n"
                                 "%s  type   = 0x%02x\n"
                                 "%s  length = %" G_GSIZE_FORMAT "\n"
                                 "%s  value  = %s\n",
                                 line_prefix,
                                 line_prefix, type,
                                 line_prefix, length,
                                 line_prefix, value_hex);
    g_free (value_hex);
    return printable;
}

static gchar *
get_generic_printable (QmiMessage *self,
                       const gchar *line_prefix)
{
    GString *printable;
    struct tlv *tlv;

    printable = g_string_new ("");

    g_string_append_printf (printable,
                            "%s  message     = (0x%04x)\n",
                            line_prefix, qmi_message_get_message_id (self));

    for (tlv = qmi_tlv_first (self); tlv; tlv = qmi_tlv_next (self, tlv)) {
        gchar *printable_tlv;

        printable_tlv = qmi_message_get_tlv_printable (self,
                                                       line_prefix,
                                                       tlv->type,
                                                       tlv->length,
                                                       tlv->value);
        g_string_append (printable, printable_tlv);
        g_free (printable_tlv);
    }

    return g_string_free (printable, FALSE);
}

gchar *
qmi_message_get_printable (QmiMessage *self,
                           const gchar *line_prefix)
{
    GString *printable;
    gchar *qmi_flags_str;
    gchar *contents;

    if (!qmi_message_check (self, NULL))
        return NULL;

    if (!line_prefix)
        line_prefix = "";

    printable = g_string_new ("");
    g_string_append_printf (printable,
                            "%sQMUX:\n"
                            "%s  length  = %u\n"
                            "%s  flags   = 0x%02x\n"
                            "%s  service = \"%s\"\n"
                            "%s  client  = %u\n",
                            line_prefix,
                            line_prefix, qmi_message_get_qmux_length (self),
                            line_prefix, qmi_message_get_qmux_flags (self),
                            line_prefix, qmi_service_get_string (qmi_message_get_service (self)),
                            line_prefix, qmi_message_get_client_id (self));

    if (qmi_message_get_service (self) == QMI_SERVICE_CTL)
        qmi_flags_str = qmi_ctl_flag_build_string_from_mask (qmi_message_get_qmi_flags (self));
    else
        qmi_flags_str = qmi_service_flag_build_string_from_mask (qmi_message_get_qmi_flags (self));

    g_string_append_printf (printable,
                            "%sQMI:\n"
                            "%s  flags       = \"%s\"\n"
                            "%s  transaction = %u\n"
                            "%s  tlv_length  = %u\n",
                            line_prefix,
                            line_prefix, qmi_flags_str,
                            line_prefix, qmi_message_get_transaction_id (self),
                            line_prefix, qmi_message_get_tlv_length (self));
    g_free (qmi_flags_str);

    contents = NULL;
    switch (qmi_message_get_service (self)) {
    case QMI_SERVICE_CTL:
        contents = qmi_message_ctl_get_printable (self, line_prefix);
        break;
    case QMI_SERVICE_DMS:
        contents = qmi_message_dms_get_printable (self, line_prefix);
        break;
    case QMI_SERVICE_WDS:
        contents = qmi_message_wds_get_printable (self, line_prefix);
        break;
    case QMI_SERVICE_NAS:
        contents = qmi_message_nas_get_printable (self, line_prefix);
        break;
    case QMI_SERVICE_WMS:
        contents = qmi_message_wms_get_printable (self, line_prefix);
        break;
    case QMI_SERVICE_PDS:
        contents = qmi_message_pds_get_printable (self, line_prefix);
        break;
    default:
        break;
    }

    if (!contents)
        contents = get_generic_printable (self, line_prefix);
    g_string_append (printable, contents);
    g_free (contents);

    return g_string_free (printable, FALSE);
}

gboolean
qmi_message_get_version_introduced (QmiMessage *self,
                                    guint *major,
                                    guint *minor)
{
    switch (qmi_message_get_service (self)) {
    case QMI_SERVICE_CTL:
        /* For CTL service, we'll assume the minimum one */
        *major = 0;
        *minor = 0;
        return TRUE;

    case QMI_SERVICE_DMS:
        return qmi_message_dms_get_version_introduced (self, major, minor);

    case QMI_SERVICE_WDS:
        return qmi_message_wds_get_version_introduced (self, major, minor);

    case QMI_SERVICE_NAS:
        return qmi_message_nas_get_version_introduced (self, major, minor);

    case QMI_SERVICE_WMS:
        return qmi_message_wms_get_version_introduced (self, major, minor);

    case QMI_SERVICE_PDS:
        return qmi_message_pds_get_version_introduced (self, major, minor);

    default:
        /* For the still unsupported services, cannot do anything */
        return FALSE;
    }
}
