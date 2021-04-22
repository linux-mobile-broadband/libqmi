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
 * Copyright (C) 2012-2019 Aleksander Morgado <aleksander@aleksander.es>
 */

#include <glib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <endian.h>

#include "qmi-message.h"
#include "qmi-helpers.h"
#include "qmi-enums-private.h"
#include "qmi-enum-types-private.h"
#include "qmi-enum-types.h"
#include "qmi-error-types.h"

#include "qmi-ctl.h"
#include "qmi-dms.h"
#include "qmi-wds.h"
#include "qmi-nas.h"
#include "qmi-wms.h"
#include "qmi-pdc.h"
#include "qmi-pds.h"
#include "qmi-pbm.h"
#include "qmi-uim.h"
#include "qmi-oma.h"
#include "qmi-wda.h"
#include "qmi-voice.h"
#include "qmi-loc.h"
#include "qmi-qos.h"
#include "qmi-gas.h"
#include "qmi-gms.h"
#include "qmi-dsd.h"
#include "qmi-sar.h"
#include "qmi-dpm.h"

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

static inline gboolean
message_is_control (QmiMessage *self)
{
    return ((struct full_message *)(self->data))->qmux.service == QMI_SERVICE_CTL;
}

static inline guint16
get_qmux_length (QmiMessage *self)
{
    return GUINT16_FROM_LE (((struct full_message *)(self->data))->qmux.length);
}

static inline void
set_qmux_length (QmiMessage *self,
                 guint16 length)
{
    ((struct full_message *)(self->data))->qmux.length = GUINT16_TO_LE (length);
}

static inline guint8
get_qmux_flags (QmiMessage *self)
{
    return ((struct full_message *)(self->data))->qmux.flags;
}

static inline guint8
get_qmi_flags (QmiMessage *self)
{
    if (message_is_control (self))
        return ((struct full_message *)(self->data))->qmi.control.header.flags;

    return ((struct full_message *)(self->data))->qmi.service.header.flags;
}

gboolean
qmi_message_is_request (QmiMessage *self)
{
    return (!qmi_message_is_response (self) && !qmi_message_is_indication (self));
}

gboolean
qmi_message_is_response (QmiMessage *self)
{
    if (message_is_control (self)) {
        if (((struct full_message *)(self->data))->qmi.control.header.flags & QMI_CTL_FLAG_RESPONSE)
            return TRUE;
    } else {
        if (((struct full_message *)(self->data))->qmi.service.header.flags & QMI_SERVICE_FLAG_RESPONSE)
            return TRUE;
    }

    return FALSE;
}

gboolean
qmi_message_is_indication (QmiMessage *self)
{
    if (message_is_control (self)) {
        if (((struct full_message *)(self->data))->qmi.control.header.flags & QMI_CTL_FLAG_INDICATION)
            return TRUE;
    } else {
        if (((struct full_message *)(self->data))->qmi.service.header.flags & QMI_SERVICE_FLAG_INDICATION)
            return TRUE;
    }

    return FALSE;
}

QmiService
qmi_message_get_service (QmiMessage *self)
{
    g_return_val_if_fail (self != NULL, QMI_SERVICE_UNKNOWN);

    return (QmiService)((struct full_message *)(self->data))->qmux.service;
}

guint8
qmi_message_get_client_id (QmiMessage *self)
{
    g_return_val_if_fail (self != NULL, 0);

    return ((struct full_message *)(self->data))->qmux.client;
}

guint16
qmi_message_get_transaction_id (QmiMessage *self)
{
    g_return_val_if_fail (self != NULL, 0);

    if (message_is_control (self))
        /* note: only 1 byte for transaction in CTL message */
        return (guint16)((struct full_message *)(self->data))->qmi.control.header.transaction;

    return GUINT16_FROM_LE (((struct full_message *)(self->data))->qmi.service.header.transaction);
}

void
qmi_message_set_transaction_id (QmiMessage *self,
                                guint16 transaction_id)
{
    g_return_if_fail (self != NULL);

    if (message_is_control (self))
        ((struct full_message *)self->data)->qmi.control.header.transaction = (guint8)transaction_id;
    else
        ((struct full_message *)self->data)->qmi.service.header.transaction = GUINT16_TO_LE (transaction_id);
}

guint16
qmi_message_get_message_id (QmiMessage *self)
{
    g_return_val_if_fail (self != NULL, 0);

    if (message_is_control (self))
        return GUINT16_FROM_LE (((struct full_message *)(self->data))->qmi.control.header.message);

    return GUINT16_FROM_LE (((struct full_message *)(self->data))->qmi.service.header.message);
}

gsize
qmi_message_get_length (QmiMessage *self)
{
    g_return_val_if_fail (self != NULL, 0);

    return self->len;
}

static inline guint16
get_all_tlvs_length (QmiMessage *self)
{
    if (message_is_control (self))
        return GUINT16_FROM_LE (((struct full_message *)(self->data))->qmi.control.header.tlv_length);

    return GUINT16_FROM_LE (((struct full_message *)(self->data))->qmi.service.header.tlv_length);
}

static inline void
set_all_tlvs_length (QmiMessage *self,
                     guint16 length)
{
    if (message_is_control (self))
        ((struct full_message *)(self->data))->qmi.control.header.tlv_length = GUINT16_TO_LE (length);
    else
        ((struct full_message *)(self->data))->qmi.service.header.tlv_length = GUINT16_TO_LE (length);
}

static inline struct tlv *
qmi_tlv (QmiMessage *self)
{
    if (message_is_control (self))
        return ((struct full_message *)(self->data))->qmi.control.tlv;

    return ((struct full_message *)(self->data))->qmi.service.tlv;
}

static inline guint8 *
qmi_end (QmiMessage *self)
{
    return (guint8 *) self->data + self->len;
}

static inline struct tlv *
tlv_next (struct tlv *tlv)
{
    return (struct tlv *)((guint8 *)tlv + sizeof(struct tlv) + GUINT16_FROM_LE (tlv->length));
}

static inline struct tlv *
qmi_tlv_first (QmiMessage *self)
{
    if (get_all_tlvs_length (self))
        return qmi_tlv (self);

    return NULL;
}

static inline struct tlv *
qmi_tlv_next (QmiMessage *self,
              struct tlv *tlv)
{
    struct tlv *end;
    struct tlv *next;

    end = (struct tlv *) qmi_end (self);
    next = tlv_next (tlv);

    return (next < end ? next : NULL);
}

/*
 * Checks the validity of a QMI message.
 *
 * In particular, checks:
 * 1. The message has space for all required headers.
 * 2. The length of the buffer, the qmux length field, and the QMI tlv_length
 *    field are all consistent.
 * 3. The TLVs in the message fit exactly in the payload size.
 *
 * Returns: %TRUE if the message is valid, %FALSE otherwise.
 */
static gboolean
message_check (QmiMessage *self,
               GError **error)
{
    gsize header_length;
    guint8 *end;
    struct tlv *tlv;

    if (((struct full_message *)(self->data))->marker != QMI_MESSAGE_QMUX_MARKER) {
        g_set_error (error,
                     QMI_CORE_ERROR,
                     QMI_CORE_ERROR_INVALID_MESSAGE,
                     "Marker is incorrect (0x%02x != 0x%02x)",
                     ((struct full_message *)(self->data))->marker,
                     QMI_MESSAGE_QMUX_MARKER);
        return FALSE;
    }

    if (get_qmux_length (self) < sizeof (struct qmux)) {
        g_set_error (error,
                     QMI_CORE_ERROR,
                     QMI_CORE_ERROR_INVALID_MESSAGE,
                     "QMUX length too short for QMUX header (%u < %" G_GSIZE_FORMAT ")",
                     get_qmux_length (self), sizeof (struct qmux));
        return FALSE;
    }

    /*
     * qmux length is one byte shorter than buffer length because qmux
     * length does not include the qmux frame marker.
     */
    if (get_qmux_length (self) != self->len - 1) {
        g_set_error (error,
                     QMI_CORE_ERROR,
                     QMI_CORE_ERROR_INVALID_MESSAGE,
                     "QMUX length and buffer length don't match (%u != %u)",
                     get_qmux_length (self), self->len - 1);
        return FALSE;
    }

    header_length = sizeof (struct qmux) + (message_is_control (self) ?
                                            sizeof (struct control_header) :
                                            sizeof (struct service_header));

    if (get_qmux_length (self) < header_length) {
        g_set_error (error,
                     QMI_CORE_ERROR,
                     QMI_CORE_ERROR_INVALID_MESSAGE,
                     "QMUX length too short for QMI header (%u < %" G_GSIZE_FORMAT ")",
                     get_qmux_length (self), header_length);
        return FALSE;
    }

    if (get_qmux_length (self) - header_length != get_all_tlvs_length (self)) {
        g_set_error (error,
                     QMI_CORE_ERROR,
                     QMI_CORE_ERROR_INVALID_MESSAGE,
                     "QMUX length and QMI TLV lengths don't match (%u - %" G_GSIZE_FORMAT " != %u)",
                     get_qmux_length (self), header_length, get_all_tlvs_length (self));
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
        if (tlv->value + GUINT16_FROM_LE (tlv->length) > end) {
            g_set_error (error,
                         QMI_CORE_ERROR,
                         QMI_CORE_ERROR_INVALID_MESSAGE,
                         "TLV value runs over buffer (%p + %u  > %p)",
                         tlv->value, GUINT16_FROM_LE (tlv->length), end);
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
    GByteArray *self;
    struct full_message *buffer;
    gsize buffer_len;

    /* Transaction ID in the control service is 8bit only */
    g_return_val_if_fail ((service != QMI_SERVICE_CTL || transaction_id <= G_MAXUINT8),
                          NULL);

    /* Create array with enough size for the QMUX marker, the QMUX header and
     * the QMI header */
    buffer_len = (1 +
                  sizeof (struct qmux) +
                  (service == QMI_SERVICE_CTL ? sizeof (struct control_header) : sizeof (struct service_header)));

    /* NOTE:
     * Don't use g_byte_array_new_take() along with g_byte_array_set_size()!
     * Not yet, at least, see:
     * https://bugzilla.gnome.org/show_bug.cgi?id=738170
     */

    /* Create the GByteArray with buffer_len bytes preallocated */
    self = g_byte_array_sized_new (buffer_len);
    /* Actually flag as all the buffer_len bytes being used. */
    g_byte_array_set_size (self, buffer_len);

    buffer = (struct full_message *)(self->data);
    buffer->marker = QMI_MESSAGE_QMUX_MARKER;
    buffer->qmux.flags = 0;
    buffer->qmux.service = service;
    buffer->qmux.client = client_id;

    if (service == QMI_SERVICE_CTL) {
        buffer->qmi.control.header.flags = 0;
        buffer->qmi.control.header.transaction = (guint8)transaction_id;
        buffer->qmi.control.header.message = GUINT16_TO_LE (message_id);
    } else {
        buffer->qmi.service.header.flags = 0;
        buffer->qmi.service.header.transaction = GUINT16_TO_LE (transaction_id);
        buffer->qmi.service.header.message = GUINT16_TO_LE (message_id);
    }

    /* Update length fields. */
    set_qmux_length (self, buffer_len - 1); /* QMUX marker not included in length */
    set_all_tlvs_length (self, 0);

    /* We shouldn't create invalid empty messages */
    g_assert (message_check (self, NULL));

    return (QmiMessage *)self;
}

QmiMessage *
qmi_message_new_from_data (QmiService   service,
                           guint8       client_id,
                           GByteArray  *qmi_data,
                           GError     **error)
{
    GByteArray *self;
    struct full_message *buffer;
    gsize buffer_len;
    gsize message_len;

    /* Create array with enough size for the QMUX marker and QMUX header, and
     * with enough room to copy the rest of the message into */
    if (service == QMI_SERVICE_CTL) {
        message_len = sizeof (struct control_header) +
            ((struct control_message *)(qmi_data->data))->header.tlv_length;
    } else {
        message_len = sizeof (struct service_header) +
            ((struct service_message *)(qmi_data->data))->header.tlv_length;
    }
    buffer_len = (1 + sizeof (struct qmux) + message_len);
    /* Create the GByteArray with buffer_len bytes preallocated */
    self = g_byte_array_sized_new (buffer_len);
    g_byte_array_set_size (self, buffer_len);

    /* Set up fake QMUX header */
    buffer = (struct full_message *)(self->data);
    buffer->marker = QMI_MESSAGE_QMUX_MARKER;
    buffer->qmux.length = GUINT16_TO_LE (buffer_len - 1);
    buffer->qmux.flags = 0;
    buffer->qmux.service = service;
    buffer->qmux.client = client_id;

    /* Move bytes from the qmi_data array to the newly created message */
    memcpy (&buffer->qmi, qmi_data->data, message_len);
    g_byte_array_remove_range (qmi_data, 0, message_len);

    /* Check input message validity as soon as we create the QmiMessage */
    if (!message_check (self, error)) {
        /* Yes, we lose the whole message here */
        qmi_message_unref (self);
        return NULL;
    }

    return (QmiMessage *)self;
}

QmiMessage *
qmi_message_response_new (QmiMessage       *request,
                          QmiProtocolError  error)
{
    QmiMessage *response;
    gsize       tlv_offset;

    response = qmi_message_new (qmi_message_get_service (request),
                                qmi_message_get_client_id (request),
                                qmi_message_get_transaction_id (request),
                                qmi_message_get_message_id (request));

    /* Set sender type flag */
    ((struct full_message *)(((GByteArray *)response)->data))->qmux.flags = 0x80;

    /* Set the response flag */
    if (message_is_control (request))
        ((struct full_message *)(((GByteArray *)response)->data))->qmi.control.header.flags |= QMI_CTL_FLAG_RESPONSE;
    else
        ((struct full_message *)(((GByteArray *)response)->data))->qmi.service.header.flags |= QMI_SERVICE_FLAG_RESPONSE;

    /* Add result TLV, should never fail */
    tlv_offset = qmi_message_tlv_write_init (response, 0x02, NULL);
    qmi_message_tlv_write_guint16 (response, QMI_ENDIAN_LITTLE, (error != QMI_PROTOCOL_ERROR_NONE), NULL);
    qmi_message_tlv_write_guint16 (response, QMI_ENDIAN_LITTLE, error, NULL);
    qmi_message_tlv_write_complete (response, tlv_offset, NULL);

    /* We shouldn't create invalid response messages */
    g_assert (message_check (response, NULL));

    return response;
}

QmiMessage *
qmi_message_ref (QmiMessage *self)
{
    g_return_val_if_fail (self != NULL, NULL);

    return (QmiMessage *)g_byte_array_ref (self);
}

void
qmi_message_unref (QmiMessage *self)
{
    g_return_if_fail (self != NULL);

    g_byte_array_unref (self);
}

const guint8 *
qmi_message_get_raw (QmiMessage *self,
                     gsize *length,
                     GError **error)
{
    g_return_val_if_fail (self != NULL, NULL);
    g_return_val_if_fail (length != NULL, NULL);

    *length = self->len;
    return self->data;
}

const guint8 *
qmi_message_get_data (QmiMessage *self,
                      gsize *length,
                      GError **error)
{
    g_return_val_if_fail (self != NULL, NULL);
    g_return_val_if_fail (length != NULL, NULL);

    if (message_is_control (self))
        *length = sizeof (struct control_header);
    else
        *length = sizeof (struct service_header);
    *length += get_all_tlvs_length (self);
    return (guint8 *)(&((struct full_message *)(self->data))->qmi);
}

/*****************************************************************************/
/* TLV builder & writer */

static gboolean
tlv_error_if_write_overflow (QmiMessage  *self,
                             gsize        len,
                             GError     **error)
{
    /* Check for overflow of message size. */
    if (self->len + len > G_MAXUINT16) {
        g_set_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_TLV_TOO_LONG,
                     "Writing TLV would overflow");
        return FALSE;
    }

    return TRUE;
}

static struct tlv *
tlv_get_header (QmiMessage *self,
                gsize       init_offset)
{
    g_assert (init_offset <= self->len);
    return (struct tlv *)(&self->data[init_offset]);
}

gsize
qmi_message_tlv_write_init (QmiMessage  *self,
                            guint8       type,
                            GError     **error)
{
    gsize init_offset;
    struct tlv *tlv;

    g_return_val_if_fail (self != NULL, 0);
    g_return_val_if_fail (self->len > 0, 0);

    /* Check for overflow of message size. Note that a valid TLV will at least
     * have 1 byte of value. */
    if (!tlv_error_if_write_overflow (self, sizeof (struct tlv) + 1, error))
        return 0;

    /* Store where exactly we started adding the TLV */
    init_offset = self->len;

    /* Resize buffer to fit the TLV header */
    g_byte_array_set_size (self, self->len + sizeof (struct tlv));

    /* Write the TLV header */
    tlv = tlv_get_header (self, init_offset);
    tlv->type = type;
    tlv->length = 0; /* Correct value will be set in complete() */

    return init_offset;
}

void
qmi_message_tlv_write_reset (QmiMessage *self,
                             gsize       tlv_offset)
{
    g_return_if_fail (self != NULL);

    g_byte_array_set_size (self, tlv_offset);
}

gboolean
qmi_message_tlv_write_complete (QmiMessage  *self,
                                gsize        tlv_offset,
                                GError     **error)
{
    gsize tlv_length;
    struct tlv *tlv;

    g_return_val_if_fail (self != NULL, FALSE);
    g_return_val_if_fail (self->len >= (tlv_offset + sizeof (struct tlv)), FALSE);

    /* A TLV without content is actually not an error, e.g. TLV strings with no
     * data are totally valid. */
    tlv_length = self->len - tlv_offset;

    /* Update length fields. */
    tlv = tlv_get_header (self, tlv_offset);
    tlv->length = GUINT16_TO_LE (tlv_length - sizeof (struct tlv));
    set_qmux_length (self, (guint16)(get_qmux_length (self) + tlv_length));
    set_all_tlvs_length (self, (guint16)(get_all_tlvs_length (self) + tlv_length));

    /* Make sure we didn't break anything. */
    g_assert (message_check (self, NULL));

    return TRUE;
}

gboolean
qmi_message_tlv_write_guint8 (QmiMessage  *self,
                              guint8       in,
                              GError     **error)
{
    g_return_val_if_fail (self != NULL, FALSE);

    /* Check for overflow of message size */
    if (!tlv_error_if_write_overflow (self, sizeof (in), error))
        return FALSE;

    g_byte_array_append (self, &in, sizeof (in));
    return TRUE;
}

gboolean
qmi_message_tlv_write_gint8 (QmiMessage  *self,
                             gint8        in,
                             GError     **error)
{
    g_return_val_if_fail (self != NULL, FALSE);

    /* Check for overflow of message size */
    if (!tlv_error_if_write_overflow (self, sizeof (in), error))
        return FALSE;

    g_byte_array_append (self, (guint8 *)&in, sizeof (in));
    return TRUE;
}

gboolean
qmi_message_tlv_write_guint16 (QmiMessage  *self,
                               QmiEndian    endian,
                               guint16      in,
                               GError     **error)
{
    guint16 tmp;

    g_return_val_if_fail (self != NULL, FALSE);

    /* Check for overflow of message size */
    if (!tlv_error_if_write_overflow (self, sizeof (in), error))
        return FALSE;

    tmp = (endian == QMI_ENDIAN_BIG ? GUINT16_TO_BE (in) : GUINT16_TO_LE (in));
    g_byte_array_append (self, (guint8 *)&tmp, sizeof (tmp));
    return TRUE;
}

gboolean
qmi_message_tlv_write_gint16 (QmiMessage  *self,
                              QmiEndian    endian,
                              gint16       in,
                              GError     **error)
{
    gint16 tmp;

    g_return_val_if_fail (self != NULL, FALSE);

    /* Check for overflow of message size */
    if (!tlv_error_if_write_overflow (self, sizeof (in), error))
        return FALSE;

    tmp = (endian == QMI_ENDIAN_BIG ? GINT16_TO_BE (in) : GINT16_TO_LE (in));
    g_byte_array_append (self, (guint8 *)&tmp, sizeof (tmp));
    return TRUE;
}

gboolean
qmi_message_tlv_write_guint32 (QmiMessage  *self,
                               QmiEndian    endian,
                               guint32      in,
                               GError     **error)
{
    guint32 tmp;

    g_return_val_if_fail (self != NULL, FALSE);

    /* Check for overflow of message size */
    if (!tlv_error_if_write_overflow (self, sizeof (in), error))
        return FALSE;

    tmp = (endian == QMI_ENDIAN_BIG ? GUINT32_TO_BE (in) : GUINT32_TO_LE (in));
    g_byte_array_append (self, (guint8 *)&tmp, sizeof (tmp));
    return TRUE;
}

gboolean
qmi_message_tlv_write_gint32 (QmiMessage  *self,
                              QmiEndian    endian,
                              gint32       in,
                              GError     **error)
{
    gint32 tmp;

    g_return_val_if_fail (self != NULL, FALSE);

    /* Check for overflow of message size */
    if (!tlv_error_if_write_overflow (self, sizeof (in), error))
        return FALSE;

    tmp = (endian == QMI_ENDIAN_BIG ? GINT32_TO_BE (in) : GINT32_TO_LE (in));
    g_byte_array_append (self, (guint8 *)&tmp, sizeof (tmp));
    return TRUE;
}

gboolean
qmi_message_tlv_write_guint64 (QmiMessage  *self,
                               QmiEndian    endian,
                               guint64      in,
                               GError     **error)
{
    guint64 tmp;

    g_return_val_if_fail (self != NULL, FALSE);

    /* Check for overflow of message size */
    if (!tlv_error_if_write_overflow (self, sizeof (in), error))
        return FALSE;

    tmp = (endian == QMI_ENDIAN_BIG ? GUINT64_TO_BE (in) : GUINT64_TO_LE (in));
    g_byte_array_append (self, (guint8 *)&tmp, sizeof (tmp));
    return TRUE;
}

gboolean
qmi_message_tlv_write_gint64 (QmiMessage  *self,
                              QmiEndian    endian,
                              gint64       in,
                              GError     **error)
{
    gint64 tmp;

    g_return_val_if_fail (self != NULL, FALSE);

    /* Check for overflow of message size */
    if (!tlv_error_if_write_overflow (self, sizeof (in), error))
        return FALSE;

    tmp = (endian == QMI_ENDIAN_BIG ? GINT64_TO_BE (in) : GINT64_TO_LE (in));
    g_byte_array_append (self, (guint8 *)&tmp, sizeof (tmp));
    return TRUE;
}

gboolean
qmi_message_tlv_write_sized_guint (QmiMessage  *self,
                                   guint        n_bytes,
                                   QmiEndian    endian,
                                   guint64      in,
                                   GError     **error)
{
    guint64 tmp;
    goffset offset;

    g_return_val_if_fail (self != NULL, FALSE);
    g_return_val_if_fail (n_bytes <= 8, FALSE);

    /* Check for overflow of message size */
    if (!tlv_error_if_write_overflow (self, n_bytes, error))
        return FALSE;

    tmp = (endian == QMI_ENDIAN_BIG ? GUINT64_TO_BE (in) : GUINT64_TO_LE (in));

    /* Update buffer size */
    offset = self->len;
    g_byte_array_set_size (self, self->len + n_bytes);

    /* In Little Endian, we read the bytes from the beginning of the buffer */
    if (endian == QMI_ENDIAN_LITTLE) {
        memcpy (&self->data[offset], &tmp, n_bytes);
    }
    /* In Big Endian, we read the bytes from the end of the buffer */
    else {
        guint8 tmp_buffer[8];

        memcpy (&tmp_buffer[0], &tmp, 8);
        memcpy (&self->data[offset], &tmp_buffer[8 - n_bytes], n_bytes);
    }

    return TRUE;
}

gboolean
qmi_message_tlv_write_string (QmiMessage   *self,
                              guint8        n_size_prefix_bytes,
                              const gchar  *in,
                              gssize        in_length,
                              GError      **error)
{
    gsize len;

    g_return_val_if_fail (self != NULL, FALSE);
    g_return_val_if_fail (in != NULL, FALSE);
    g_return_val_if_fail (n_size_prefix_bytes <= 2, FALSE);

    len = (in_length < 0 ? strlen (in) : (gsize) in_length);

    /* Write size prefix first */
    switch (n_size_prefix_bytes) {
    case 0:
        break;
    case 1:
        if (len > G_MAXUINT8) {
            g_set_error (error,
                         QMI_CORE_ERROR,
                         QMI_CORE_ERROR_INVALID_ARGS,
                         "String too long for a 1 byte size prefix: %" G_GSIZE_FORMAT, len);
            return FALSE;
        }
        if (!qmi_message_tlv_write_guint8 (self, (guint8) len, error)) {
            g_prefix_error (error, "Cannot append string 1 byte size prefix");
            return FALSE;
        }
        break;
    case 2:
        if (len > G_MAXUINT16) {
            g_set_error (error,
                         QMI_CORE_ERROR,
                         QMI_CORE_ERROR_INVALID_ARGS,
                         "String too long for a 2 byte size prefix: %" G_GSIZE_FORMAT, len);
            return FALSE;
        }
        if (!qmi_message_tlv_write_guint16 (self, QMI_ENDIAN_LITTLE, (guint16) len, error)) {
            g_prefix_error (error, "Cannot append string 2 byte size prefix");
            return FALSE;
        }
        break;
    default:
        g_assert_not_reached ();
    }

    /* Check for overflow of string size */
    if (!tlv_error_if_write_overflow (self, len, error))
        return FALSE;

    g_byte_array_append (self, (guint8 *)in, len);
    return TRUE;
}

/*****************************************************************************/
/* TLV reader */

gsize
qmi_message_tlv_read_init (QmiMessage  *self,
                           guint8       type,
                           guint16     *out_tlv_length,
                           GError     **error)
{
    struct tlv *tlv;
    guint16     tlv_length;

    g_return_val_if_fail (self != NULL, 0);
    g_return_val_if_fail (self->len > 0, 0);

    for (tlv = qmi_tlv_first (self); tlv; tlv = qmi_tlv_next (self, tlv)) {
        if (tlv->type == type)
            break;
    }

    if (!tlv) {
        g_set_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_TLV_NOT_FOUND,
                     "TLV 0x%02X not found", type);
        return 0;
    }

    tlv_length = GUINT16_FROM_LE (tlv->length);

    if (((guint8 *) tlv_next (tlv)) > ((guint8 *) qmi_end (self))) {
        g_set_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_TLV_TOO_LONG,
                     "Invalid length for TLV 0x%02X: %" G_GUINT16_FORMAT, type, tlv_length);
        return 0;
    }

    if (out_tlv_length)
        *out_tlv_length = tlv_length;

    return (((guint8 *)tlv) - self->data);
}

static const guint8 *
tlv_error_if_read_overflow (QmiMessage  *self,
                            gsize        tlv_offset,
                            gsize        offset,
                            gsize        len,
                            GError     **error)
{
    const guint8 *ptr;
    struct tlv   *tlv;

    tlv = (struct tlv *) &(self->data[tlv_offset]);
    ptr = ((guint8 *)tlv) + sizeof (struct tlv) + offset;

    if (((guint8 *)(ptr + len) > (guint8 *)tlv_next (tlv)) ||
        ((guint8 *)(ptr + len) > (guint8 *)qmi_end (self))) {
        g_set_error (error,
                     QMI_CORE_ERROR,
                     QMI_CORE_ERROR_TLV_TOO_LONG,
                     "Reading TLV would overflow");
        return NULL;
    }

    return ptr;
}

gboolean
qmi_message_tlv_read_guint8 (QmiMessage  *self,
                             gsize        tlv_offset,
                             gsize       *offset,
                             guint8      *out,
                             GError     **error)
{
    const guint8 *ptr;

    g_return_val_if_fail (self != NULL, FALSE);
    g_return_val_if_fail (offset != NULL, FALSE);
    g_return_val_if_fail (out != NULL, FALSE);

    if (!(ptr = tlv_error_if_read_overflow (self, tlv_offset, *offset, sizeof (*out), error)))
        return FALSE;

    *offset = *offset + 1;
    *out = *ptr;
    return TRUE;
}

gboolean
qmi_message_tlv_read_gint8 (QmiMessage  *self,
                            gsize        tlv_offset,
                            gsize       *offset,
                            gint8       *out,
                            GError     **error)
{
    const guint8 *ptr;

    g_return_val_if_fail (self != NULL, FALSE);
    g_return_val_if_fail (offset != NULL, FALSE);
    g_return_val_if_fail (out != NULL, FALSE);

    if (!(ptr = tlv_error_if_read_overflow (self, tlv_offset, *offset, sizeof (*out), error)))
        return FALSE;

    *out = (gint8)(*ptr);
    *offset = *offset + 1;
    return TRUE;
}

gboolean
qmi_message_tlv_read_guint16 (QmiMessage  *self,
                              gsize        tlv_offset,
                              gsize       *offset,
                              QmiEndian    endian,
                              guint16     *out,
                              GError     **error)
{
    const guint8 *ptr;

    g_return_val_if_fail (self != NULL, FALSE);
    g_return_val_if_fail (offset != NULL, FALSE);
    g_return_val_if_fail (out != NULL, FALSE);

    if (!(ptr = tlv_error_if_read_overflow (self, tlv_offset, *offset, sizeof (*out), error)))
        return FALSE;

    memcpy (out, ptr, 2);
    if (endian == QMI_ENDIAN_BIG)
        *out = GUINT16_FROM_BE (*out);
    else
        *out = GUINT16_FROM_LE (*out);
    *offset = *offset + 2;
    return TRUE;
}

gboolean
qmi_message_tlv_read_gint16 (QmiMessage  *self,
                             gsize        tlv_offset,
                             gsize       *offset,
                             QmiEndian    endian,
                             gint16      *out,
                             GError     **error)
{
    const guint8 *ptr;

    g_return_val_if_fail (self != NULL, FALSE);
    g_return_val_if_fail (offset != NULL, FALSE);
    g_return_val_if_fail (out != NULL, FALSE);

    if (!(ptr = tlv_error_if_read_overflow (self, tlv_offset, *offset, sizeof (*out), error)))
        return FALSE;

    memcpy (out, ptr, 2);
    if (endian == QMI_ENDIAN_BIG)
        *out = GUINT16_FROM_BE (*out);
    else
        *out = GUINT16_FROM_LE (*out);
    *offset = *offset + 2;
    return TRUE;
}

gboolean
qmi_message_tlv_read_guint32 (QmiMessage  *self,
                              gsize        tlv_offset,
                              gsize       *offset,
                              QmiEndian    endian,
                              guint32     *out,
                              GError     **error)
{
    const guint8 *ptr;

    g_return_val_if_fail (self != NULL, FALSE);
    g_return_val_if_fail (offset != NULL, FALSE);
    g_return_val_if_fail (out != NULL, FALSE);

    if (!(ptr = tlv_error_if_read_overflow (self, tlv_offset, *offset, sizeof (*out), error)))
        return FALSE;

    memcpy (out, ptr, 4);
    if (endian == QMI_ENDIAN_BIG)
        *out = GUINT32_FROM_BE (*out);
    else
        *out = GUINT32_FROM_LE (*out);
    *offset = *offset + 4;
    return TRUE;
}

gboolean
qmi_message_tlv_read_gint32 (QmiMessage  *self,
                             gsize        tlv_offset,
                             gsize       *offset,
                             QmiEndian    endian,
                             gint32      *out,
                             GError     **error)
{
    const guint8 *ptr;

    g_return_val_if_fail (self != NULL, FALSE);
    g_return_val_if_fail (offset != NULL, FALSE);
    g_return_val_if_fail (out != NULL, FALSE);

    if (!(ptr = tlv_error_if_read_overflow (self, tlv_offset, *offset, sizeof (*out), error)))
        return FALSE;

    memcpy (out, ptr, 4);
    if (endian == QMI_ENDIAN_BIG)
        *out = GINT32_FROM_BE (*out);
    else
        *out = GINT32_FROM_LE (*out);
    *offset = *offset + 4;
    return TRUE;
}

gboolean
qmi_message_tlv_read_guint64 (QmiMessage  *self,
                              gsize        tlv_offset,
                              gsize       *offset,
                              QmiEndian    endian,
                              guint64     *out,
                              GError     **error)
{
    const guint8 *ptr;

    g_return_val_if_fail (self != NULL, FALSE);
    g_return_val_if_fail (offset != NULL, FALSE);
    g_return_val_if_fail (out != NULL, FALSE);

    if (!(ptr = tlv_error_if_read_overflow (self, tlv_offset, *offset, sizeof (*out), error)))
        return FALSE;

    memcpy (out, ptr, 8);
    if (endian == QMI_ENDIAN_BIG)
        *out = GUINT64_FROM_BE (*out);
    else
        *out = GUINT64_FROM_LE (*out);
    *offset = *offset + 8;
    return TRUE;
}

gboolean
qmi_message_tlv_read_gint64 (QmiMessage  *self,
                             gsize        tlv_offset,
                             gsize       *offset,
                             QmiEndian    endian,
                             gint64      *out,
                             GError     **error)
{
    const guint8 *ptr;

    g_return_val_if_fail (self != NULL, FALSE);
    g_return_val_if_fail (offset != NULL, FALSE);
    g_return_val_if_fail (out != NULL, FALSE);

    if (!(ptr = tlv_error_if_read_overflow (self, tlv_offset, *offset, sizeof (*out), error)))
        return FALSE;

    memcpy (out, ptr, 8);
    if (endian == QMI_ENDIAN_BIG)
        *out = GINT64_FROM_BE (*out);
    else
        *out = GINT64_FROM_LE (*out);
    *offset = *offset + 8;
    return TRUE;
}

gboolean
qmi_message_tlv_read_sized_guint (QmiMessage  *self,
                                  gsize        tlv_offset,
                                  gsize       *offset,
                                  guint        n_bytes,
                                  QmiEndian    endian,
                                  guint64     *out,
                                  GError     **error)
{
    const guint8 *ptr;

    g_return_val_if_fail (self != NULL, FALSE);
    g_return_val_if_fail (offset != NULL, FALSE);
    g_return_val_if_fail (out != NULL, FALSE);
    g_return_val_if_fail (n_bytes <= 8, FALSE);

    if (!(ptr = tlv_error_if_read_overflow (self, tlv_offset, *offset, n_bytes, error)))
        return FALSE;

    *out = 0;

    /* In Little Endian, we copy the bytes to the beginning of the output
     * buffer. */
    if (endian == QMI_ENDIAN_LITTLE) {
        memcpy (out, ptr, n_bytes);
        *out = GUINT64_FROM_LE (*out);
    }
    /* In Big Endian, we copy the bytes to the end of the output buffer */
    else {
        guint8 tmp[8] = { 0 };

        memcpy (&tmp[8 - n_bytes], ptr, n_bytes);
        memcpy (out, &tmp[0], 8);
        *out = GUINT64_FROM_BE (*out);
    }

    *offset = *offset + n_bytes;
    return TRUE;
}

gboolean
qmi_message_tlv_read_gfloat_endian (QmiMessage  *self,
                                    gsize        tlv_offset,
                                    gsize       *offset,
                                    QmiEndian    endian,
                                    gfloat      *out,
                                    GError     **error)
{
    const guint8 *ptr;

    g_return_val_if_fail (self != NULL, FALSE);
    g_return_val_if_fail (offset != NULL, FALSE);
    g_return_val_if_fail (out != NULL, FALSE);

    if (!(ptr = tlv_error_if_read_overflow (self, tlv_offset, *offset, 4, error)))
        return FALSE;

    memcpy (out, ptr, 4);
    if (endian == QMI_ENDIAN_BIG)
        *out = QMI_GFLOAT_FROM_BE (*out);
    else
        *out = QMI_GFLOAT_FROM_LE (*out);
    *offset = *offset + 4;
    return TRUE;
}

gboolean
qmi_message_tlv_read_gdouble (QmiMessage  *self,
                              gsize        tlv_offset,
                              gsize       *offset,
                              QmiEndian    endian,
                              gdouble     *out,
                              GError     **error)
{
    const guint8 *ptr;

    g_return_val_if_fail (self != NULL, FALSE);
    g_return_val_if_fail (offset != NULL, FALSE);
    g_return_val_if_fail (out != NULL, FALSE);

    if (!(ptr = tlv_error_if_read_overflow (self, tlv_offset, *offset, 8, error)))
        return FALSE;

    /* Yeah, do this for now */
    memcpy (out, ptr, 8);
    if (endian == QMI_ENDIAN_BIG)
        *out = QMI_GDOUBLE_FROM_BE (*out);
    else
        *out = QMI_GDOUBLE_FROM_LE (*out);
    *offset = *offset + 8;
    return TRUE;
}

gboolean
qmi_message_tlv_read_string (QmiMessage  *self,
                             gsize        tlv_offset,
                             gsize       *offset,
                             guint8       n_size_prefix_bytes,
                             guint16      max_size,
                             gchar      **out,
                             GError     **error)
{
    const guint8 *ptr;
    guint16 string_length;
    guint16 valid_string_length;

    g_return_val_if_fail (self != NULL, FALSE);
    g_return_val_if_fail (offset != NULL, FALSE);
    g_return_val_if_fail (out != NULL, FALSE);
    g_return_val_if_fail (n_size_prefix_bytes <= 2, FALSE);

    switch (n_size_prefix_bytes) {
    case 0: {
        struct tlv *tlv;

        if (!tlv_error_if_read_overflow (self, tlv_offset, *offset, 0, error))
            return FALSE;

        /* If no length prefix given, read the remaining TLV buffer into a string */
        tlv = (struct tlv *) &(self->data[tlv_offset]);
        string_length = (GUINT16_FROM_LE (tlv->length) - *offset);
        break;
    }
    case 1: {
        guint8 string_length_8;

        if (!qmi_message_tlv_read_guint8 (self, tlv_offset, offset, &string_length_8, error))
            return FALSE;
        string_length = (guint16) string_length_8;
        break;
    }
    case 2:
        if (!qmi_message_tlv_read_guint16 (self, tlv_offset, offset, QMI_ENDIAN_LITTLE, &string_length, error))
            return FALSE;
        break;
    default:
        g_assert_not_reached ();
    }

    if (string_length == 0) {
        *out = g_strdup ("");
        return TRUE;
    }

    if (max_size > 0 && string_length > max_size)
        valid_string_length = max_size;
    else
        valid_string_length = string_length;

    if (!(ptr = tlv_error_if_read_overflow (self, tlv_offset, *offset, valid_string_length, error)))
        return FALSE;

    /* Perform a quick UTF-8 validation check first. This check isn't perfect,
     * because there may be GSM-7 encoded strings that are valid UTF-8 as well,
     * but hey, the strings read using this method should all really be ASCII-7
     * and we're trying to do our best to overcome modem firmware problems...
     */
    if (qmi_helpers_string_utf8_validate_printable (ptr, valid_string_length)) {
        *out = g_malloc (valid_string_length + 1);
        memcpy (*out, ptr, valid_string_length);
        (*out)[valid_string_length] = '\0';
    } else {
        /* Otherwise, attempt GSM-7 */
        *out =qmi_helpers_string_utf8_from_gsm7 (ptr, valid_string_length);
        if (*out == NULL) {
            /* Otherwise, attempt UCS-2 */
            *out = qmi_helpers_string_utf8_from_ucs2le (ptr, valid_string_length);
            if (*out == NULL) {
                /* Otherwise, error */
                g_set_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_INVALID_DATA, "invalid string");
                return FALSE;
            }
        }
    }

    *offset = (*offset + string_length);
    return TRUE;
}

gboolean
qmi_message_tlv_read_fixed_size_string (QmiMessage  *self,
                                        gsize        tlv_offset,
                                        gsize       *offset,
                                        guint16      string_length,
                                        gchar       *out,
                                        GError     **error)
{
    const guint8 *ptr;
    const gchar  *end = NULL;

    g_return_val_if_fail (self != NULL, FALSE);
    g_return_val_if_fail (offset != NULL, FALSE);
    g_return_val_if_fail (out != NULL, FALSE);

    if (string_length == 0)
        return TRUE;

    if (!(ptr = tlv_error_if_read_overflow (self, tlv_offset, *offset, string_length, error)))
        return FALSE;

    /* full string valid? */
    if (g_utf8_validate ((const gchar *)ptr, string_length, &end)) {
        memcpy (out, ptr, string_length);
        *offset = (*offset + string_length);
        return TRUE;
    }

    /* partial string valid? */
    if (end && end > (const gchar *)ptr) {
        /* copy only the valid bytes */
        memcpy (out, ptr, end - (const gchar *)ptr);
        /* but update offset with the full expected length */
        *offset = (*offset + string_length);
        return TRUE;
    }

    g_set_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_INVALID_DATA, "invalid string");
    return FALSE;
}

guint16
qmi_message_tlv_read_remaining_size (QmiMessage *self,
                                     gsize       tlv_offset,
                                     gsize       offset)
{
    struct tlv *tlv;

    g_return_val_if_fail (self != NULL, FALSE);

    tlv = (struct tlv *) &(self->data[tlv_offset]);

    g_warn_if_fail (GUINT16_FROM_LE (tlv->length) >= offset);
    return (GUINT16_FROM_LE (tlv->length) >= offset ? (GUINT16_FROM_LE (tlv->length) - offset) : 0);
}

/*****************************************************************************/

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

    /* Find length of new TLV */
    tlv_len = sizeof (struct tlv) + length;

    /* Check for overflow of message size. */
    if (get_qmux_length (self) + tlv_len > G_MAXUINT16) {
        g_set_error (error,
                     QMI_CORE_ERROR,
                     QMI_CORE_ERROR_TLV_TOO_LONG,
                     "TLV to add is too long");
        return FALSE;
    }

    /* Resize buffer. */
    g_byte_array_set_size (self, self->len + tlv_len);

    /* Fill in new TLV. */
    tlv = (struct tlv *)(qmi_end (self) - tlv_len);
    tlv->type = type;
    tlv->length = GUINT16_TO_LE (length);
    memcpy (tlv->value, raw, length);

    /* Update length fields. */
    set_qmux_length (self, (guint16)(get_qmux_length (self) + tlv_len));
    set_all_tlvs_length (self, (guint16)(get_all_tlvs_length (self) + tlv_len));

    /* Make sure we didn't break anything. */
    g_assert (message_check (self, NULL));

    return TRUE;
}

QmiMessage *
qmi_message_new_from_raw (GByteArray *raw,
                          GError **error)
{
    GByteArray *self;
    gsize message_len;

    g_return_val_if_fail (raw != NULL, NULL);

    /* If we didn't even read the QMUX header (comes after the 1-byte marker),
     * leave */
    if (raw->len < (sizeof (struct qmux) + 1))
        return NULL;

    /* We need to have read the length reported by the QMUX header (plus the
     * initial 1-byte marker) */
    message_len = GUINT16_FROM_LE (((struct full_message *)raw->data)->qmux.length);
    if (raw->len < (message_len + 1))
        return NULL;

    /* Ok, so we should have all the data available already */
    self = g_byte_array_sized_new (message_len + 1);
    g_byte_array_prepend (self, raw->data, message_len + 1);

    /* We got a complete QMI message, remove from input buffer */
    g_byte_array_remove_range (raw, 0, self->len);

    /* Check input message validity as soon as we create the QmiMessage */
    if (!message_check (self, error)) {
        /* Yes, we lose the whole message here */
        qmi_message_unref (self);
        return NULL;
    }

    return (QmiMessage *)self;
}

gchar *
qmi_message_get_tlv_printable (QmiMessage *self,
                               const gchar *line_prefix,
                               guint8 type,
                               const guint8 *raw,
                               gsize raw_length)
{
    gchar *printable;
    gchar *value_hex;

    g_return_val_if_fail (self != NULL, NULL);
    g_return_val_if_fail (line_prefix != NULL, NULL);
    g_return_val_if_fail (raw != NULL, NULL);

    value_hex = qmi_helpers_str_hex (raw, raw_length, ':');
    printable = g_strdup_printf ("%sTLV:\n"
                                 "%s  type   = 0x%02x\n"
                                 "%s  length = %" G_GSIZE_FORMAT "\n"
                                 "%s  value  = %s\n",
                                 line_prefix,
                                 line_prefix, type,
                                 line_prefix, raw_length,
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
                                                       tlv->value,
                                                       GUINT16_FROM_LE (tlv->length));
        g_string_append (printable, printable_tlv);
        g_free (printable_tlv);
    }

    return g_string_free (printable, FALSE);
}

gchar *
qmi_message_get_printable_full (QmiMessage        *self,
                                QmiMessageContext *context,
                                const gchar       *line_prefix)
{
    GString *printable;
    gchar *qmi_flags_str;
    gchar *contents;

    g_return_val_if_fail (self != NULL, NULL);

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
                            line_prefix, get_qmux_length (self),
                            line_prefix, get_qmux_flags (self),
                            line_prefix, qmi_service_get_string (qmi_message_get_service (self)),
                            line_prefix, qmi_message_get_client_id (self));

    if (qmi_message_get_service (self) == QMI_SERVICE_CTL)
        qmi_flags_str = qmi_ctl_flag_build_string_from_mask (get_qmi_flags (self));
    else
        qmi_flags_str = qmi_service_flag_build_string_from_mask (get_qmi_flags (self));

    g_string_append_printf (printable,
                            "%sQMI:\n"
                            "%s  flags       = \"%s\"\n"
                            "%s  transaction = %u\n"
                            "%s  tlv_length  = %u\n",
                            line_prefix,
                            line_prefix, qmi_flags_str,
                            line_prefix, qmi_message_get_transaction_id (self),
                            line_prefix, get_all_tlvs_length (self));
    g_free (qmi_flags_str);

    contents = NULL;
    switch (qmi_message_get_service (self)) {
    case QMI_SERVICE_CTL:
#if defined HAVE_QMI_SERVICE_CTL
        contents = __qmi_message_ctl_get_printable (self, context, line_prefix);
#endif
        break;
    case QMI_SERVICE_DMS:
#if defined HAVE_QMI_SERVICE_DMS
        contents = __qmi_message_dms_get_printable (self, context, line_prefix);
#endif
        break;
    case QMI_SERVICE_WDS:
#if defined HAVE_QMI_SERVICE_WDS
        contents = __qmi_message_wds_get_printable (self, context, line_prefix);
#endif
        break;
    case QMI_SERVICE_NAS:
#if defined HAVE_QMI_SERVICE_NAS
        contents = __qmi_message_nas_get_printable (self, context, line_prefix);
#endif
        break;
    case QMI_SERVICE_WMS:
#if defined HAVE_QMI_SERVICE_WMS
        contents = __qmi_message_wms_get_printable (self, context, line_prefix);
#endif
        break;
    case QMI_SERVICE_PDC:
#if defined HAVE_QMI_SERVICE_PDC
        contents = __qmi_message_pdc_get_printable (self, context, line_prefix);
#endif
        break;
    case QMI_SERVICE_PDS:
#if defined HAVE_QMI_SERVICE_PDS
        contents = __qmi_message_pds_get_printable (self, context, line_prefix);
#endif
        break;
    case QMI_SERVICE_PBM:
#if defined HAVE_QMI_SERVICE_PBM
        contents = __qmi_message_pbm_get_printable (self, context, line_prefix);
#endif
        break;
    case QMI_SERVICE_UIM:
#if defined HAVE_QMI_SERVICE_UIM
        contents = __qmi_message_uim_get_printable (self, context, line_prefix);
#endif
        break;
    case QMI_SERVICE_OMA:
#if defined HAVE_QMI_SERVICE_OMA
        contents = __qmi_message_oma_get_printable (self, context, line_prefix);
#endif
        break;
    case QMI_SERVICE_GAS:
#if defined HAVE_QMI_SERVICE_GAS
        contents = __qmi_message_gas_get_printable (self, context, line_prefix);
#endif
        break;
    case QMI_SERVICE_GMS:
#if defined HAVE_QMI_SERVICE_GMS
        contents = __qmi_message_gms_get_printable (self, context, line_prefix);
#endif
        break;
    case QMI_SERVICE_WDA:
#if defined HAVE_QMI_SERVICE_WDA
        contents = __qmi_message_wda_get_printable (self, context, line_prefix);
#endif
        break;
    case QMI_SERVICE_VOICE:
#if defined HAVE_QMI_SERVICE_VOICE
        contents = __qmi_message_voice_get_printable (self, context, line_prefix);
#endif
        break;
    case QMI_SERVICE_LOC:
#if defined HAVE_QMI_SERVICE_LOC
        contents = __qmi_message_loc_get_printable (self, context, line_prefix);
#endif
        break;
    case QMI_SERVICE_QOS:
#if defined HAVE_QMI_SERVICE_QOS
        contents = __qmi_message_qos_get_printable (self, context, line_prefix);
#endif
        break;
    case QMI_SERVICE_DSD:
#if defined HAVE_QMI_SERVICE_DSD
        contents = __qmi_message_dsd_get_printable (self, context, line_prefix);
#endif
        break;
    case QMI_SERVICE_DPM:
#if defined HAVE_QMI_SERVICE_DPM
        contents = __qmi_message_dpm_get_printable (self, context, line_prefix);
#endif
        break;

    case QMI_SERVICE_UNKNOWN:
        g_assert_not_reached ();

    case QMI_SERVICE_AUTH:
    case QMI_SERVICE_AT:
    case QMI_SERVICE_CAT2:
    case QMI_SERVICE_QCHAT:
    case QMI_SERVICE_RMTFS:
    case QMI_SERVICE_TEST:
    case QMI_SERVICE_SAR:
    case QMI_SERVICE_IMS:
    case QMI_SERVICE_ADC:
    case QMI_SERVICE_CSD:
    case QMI_SERVICE_MFS:
    case QMI_SERVICE_TIME:
    case QMI_SERVICE_TS:
    case QMI_SERVICE_TMD:
    case QMI_SERVICE_SAP:
    case QMI_SERVICE_TSYNC:
    case QMI_SERVICE_RFSA:
    case QMI_SERVICE_CSVT:
    case QMI_SERVICE_QCMAP:
    case QMI_SERVICE_IMSP:
    case QMI_SERVICE_IMSVT:
    case QMI_SERVICE_IMSA:
    case QMI_SERVICE_COEX:
    case QMI_SERVICE_STX:
    case QMI_SERVICE_BIT:
    case QMI_SERVICE_IMSRTP:
    case QMI_SERVICE_RFRPE:
    case QMI_SERVICE_SSCTL:
    case QMI_SERVICE_CAT:
    case QMI_SERVICE_RMS:
    case QMI_SERVICE_FOTA:
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
__qmi_message_is_abortable (QmiMessage        *self,
                            QmiMessageContext *context)
{
    switch (qmi_message_get_service (self)) {
    case QMI_SERVICE_WDS:
#if defined HAVE_QMI_SERVICE_WDS
        return __qmi_message_wds_is_abortable (self, context);
#else
        return FALSE;
#endif
    case QMI_SERVICE_NAS:
#if defined HAVE_QMI_SERVICE_NAS
        return __qmi_message_nas_is_abortable (self, context);
#else
        return FALSE;
#endif

    case QMI_SERVICE_UNKNOWN:
        g_assert_not_reached ();

    case QMI_SERVICE_CTL:
    case QMI_SERVICE_DMS:
    case QMI_SERVICE_WMS:
    case QMI_SERVICE_PDS:
    case QMI_SERVICE_PBM:
    case QMI_SERVICE_UIM:
    case QMI_SERVICE_OMA:
    case QMI_SERVICE_GAS:
    case QMI_SERVICE_WDA:
    case QMI_SERVICE_LOC:
    case QMI_SERVICE_AUTH:
    case QMI_SERVICE_AT:
    case QMI_SERVICE_CAT2:
    case QMI_SERVICE_QCHAT:
    case QMI_SERVICE_RMTFS:
    case QMI_SERVICE_TEST:
    case QMI_SERVICE_SAR:
    case QMI_SERVICE_IMS:
    case QMI_SERVICE_ADC:
    case QMI_SERVICE_CSD:
    case QMI_SERVICE_MFS:
    case QMI_SERVICE_TIME:
    case QMI_SERVICE_TS:
    case QMI_SERVICE_TMD:
    case QMI_SERVICE_SAP:
    case QMI_SERVICE_TSYNC:
    case QMI_SERVICE_RFSA:
    case QMI_SERVICE_CSVT:
    case QMI_SERVICE_QCMAP:
    case QMI_SERVICE_IMSP:
    case QMI_SERVICE_IMSVT:
    case QMI_SERVICE_IMSA:
    case QMI_SERVICE_COEX:
    case QMI_SERVICE_STX:
    case QMI_SERVICE_BIT:
    case QMI_SERVICE_IMSRTP:
    case QMI_SERVICE_RFRPE:
    case QMI_SERVICE_SSCTL:
    case QMI_SERVICE_DPM:
    case QMI_SERVICE_CAT:
    case QMI_SERVICE_RMS:
    case QMI_SERVICE_FOTA:
    case QMI_SERVICE_GMS:
    case QMI_SERVICE_VOICE:
    case QMI_SERVICE_PDC:
    case QMI_SERVICE_DSD:
    case QMI_SERVICE_QOS:
    default:
        return FALSE;
    }
}
