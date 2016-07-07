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
 * Copyright (C) 2012-2015 Aleksander Morgado <aleksander@aleksander.es>
 */

#include <glib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <endian.h>

#include "qmi-message.h"
#include "qmi-utils.h"
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

/**
 * SECTION:qmi-message
 * @title: QmiMessage
 * @short_description: Generic QMI message handling routines
 *
 * #QmiMessage is a generic type representing a QMI message of any kind
 * (request, response, indication) or service (including #QMI_SERVICE_CTL).
 *
 * This set of generic routines help in handling these message types, and
 * allow creating any kind of message with any kind of TLV.
 **/

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

/**
 * qmi_message_is_request:
 * @self: a #QmiMessage.
 *
 * Checks whether the given #QmiMessage is a request.
 *
 * Returns: %TRUE if @self is a request message, %FALSE otherwise.
 */
gboolean
qmi_message_is_request (QmiMessage *self)
{
    return (!qmi_message_is_response (self) && !qmi_message_is_indication (self));
}

/**
 * qmi_message_is_response:
 * @self: a #QmiMessage.
 *
 * Checks whether the given #QmiMessage is a response.
 *
 * Returns: %TRUE if @self is a response message, %FALSE otherwise.
 */
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

/**
 * qmi_message_is_indication:
 * @self: a #QmiMessage.
 *
 * Checks whether the given #QmiMessage is an indication.
 *
 * Returns: %TRUE if @self is an indication message, %FALSE otherwise.
 */
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

/**
 * qmi_message_get_service:
 * @self: a #QmiMessage.
 *
 * Gets the service corresponding to the given #QmiMessage.
 *
 * Returns: a #QmiService.
 */
QmiService
qmi_message_get_service (QmiMessage *self)
{
    g_return_val_if_fail (self != NULL, QMI_SERVICE_UNKNOWN);

    return (QmiService)((struct full_message *)(self->data))->qmux.service;
}

/**
 * qmi_message_get_client_id:
 * @self: a #QmiMessage.
 *
 * Gets the client ID of the message.
 *
 * Returns: the client ID.
 */
guint8
qmi_message_get_client_id (QmiMessage *self)
{
    g_return_val_if_fail (self != NULL, 0);

    return ((struct full_message *)(self->data))->qmux.client;
}

/**
 * qmi_message_get_transaction_id:
 * @self: a #QmiMessage.
 *
 * Gets the transaction ID of the message.
 *
 * Returns: the transaction ID.
 */
guint16
qmi_message_get_transaction_id (QmiMessage *self)
{
    g_return_val_if_fail (self != NULL, 0);

    if (message_is_control (self))
        /* note: only 1 byte for transaction in CTL message */
        return (guint16)((struct full_message *)(self->data))->qmi.control.header.transaction;

    return GUINT16_FROM_LE (((struct full_message *)(self->data))->qmi.service.header.transaction);
}

/**
 * qmi_message_set_transaction_id:
 * @self: a #QmiMessage.
 * @transaction_id: transaction id.
 *
 * Overwrites the transaction ID of the message.
 */
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

/**
 * qmi_message_get_message_id:
 * @self: a #QmiMessage.
 *
 * Gets the ID of the message.
 *
 * Returns: the ID.
 */
guint16
qmi_message_get_message_id (QmiMessage *self)
{
    g_return_val_if_fail (self != NULL, 0);

    if (message_is_control (self))
        return GUINT16_FROM_LE (((struct full_message *)(self->data))->qmi.control.header.message);

    return GUINT16_FROM_LE (((struct full_message *)(self->data))->qmi.service.header.message);
}

/**
 * qmi_message_get_length:
 * @self: a #QmiMessage.
 *
 * Gets the length of the raw data corresponding to the given #QmiMessage.
 *
 * Returns: the length of the raw data.
 */
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
                     "Marker is incorrect");
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

/**
 * qmi_message_new:
 * @service: a #QmiService
 * @client_id: client ID of the originating control point.
 * @transaction_id: transaction ID.
 * @message_id: message ID.
 *
 * Create a new #QmiMessage with the specified parameters.
 *
 * Note that @transaction_id must be less than #G_MAXUINT8 if @service is
 * #QMI_SERVICE_CTL.
 *
 * Returns: (transfer full): a newly created #QmiMessage. The returned value should be freed with qmi_message_unref().
 */
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

/**
 * qmi_message_response_new:
 * @request: a request #QmiMessage.
 * @error: a #QmiProtocolError to set in the result TLV.
 *
 * Create a new response #QmiMessage for the specified @request.
 *
 * Returns: (transfer full): a newly created #QmiMessage. The returned value should be freed with qmi_message_unref().
 */
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

    /* Set the response flag */
    if (message_is_control (request))
        ((struct full_message *)(((GByteArray *)response)->data))->qmi.control.header.flags |= QMI_CTL_FLAG_RESPONSE;
    else
        ((struct full_message *)(((GByteArray *)response)->data))->qmi.service.header.flags |= QMI_SERVICE_FLAG_RESPONSE;

    /* Add result TLV, should never fail */
    g_assert ((tlv_offset = qmi_message_tlv_write_init (response, 0x02, NULL)) > 0);
    g_assert (qmi_message_tlv_write_guint16 (response, QMI_ENDIAN_LITTLE, (error != QMI_PROTOCOL_ERROR_NONE), NULL));
    g_assert (qmi_message_tlv_write_guint16 (response, QMI_ENDIAN_LITTLE, error, NULL));
    g_assert (qmi_message_tlv_write_complete (response, tlv_offset, NULL));

    /* We shouldn't create invalid response messages */
    g_assert (message_check (response, NULL));

    return response;
}

/**
 * qmi_message_ref:
 * @self: a #QmiMessage.
 *
 * Atomically increments the reference count of @self by one.
 *
 * Returns: (transfer full) the new reference to @self.
 */
QmiMessage *
qmi_message_ref (QmiMessage *self)
{
    g_return_val_if_fail (self != NULL, NULL);

    return (QmiMessage *)g_byte_array_ref (self);
}

/**
 * qmi_message_unref:
 * @self: a #QmiMessage.
 *
 * Atomically decrements the reference count of @self by one.
 * If the reference count drops to 0, @self is completely disposed.
 */
void
qmi_message_unref (QmiMessage *self)
{
    g_return_if_fail (self != NULL);

    g_byte_array_unref (self);
}

/**
 * qmi_message_get_raw:
 * @self: a #QmiMessage.
 * @length: (out): return location for the size of the output buffer.
 * @error: return location for error or %NULL.
 *
 * Gets the raw data buffer of the #QmiMessage.
 *
 * Returns: (transfer none): The raw data buffer, or #NULL if @error is set.
 */
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
                gssize      init_offset)
{
    g_assert (init_offset <= self->len);
    return (struct tlv *)(&self->data[init_offset]);
}

/**
 * qmi_message_tlv_write_init:
 * @self: a #QmiMessage.
 * @type: specific ID of the TLV to add.
 * @error: return location for error or %NULL.
 *
 * Starts building a new TLV in the #QmiMessage.
 *
 * In order to finish adding the TLV, qmi_message_tlv_write_complete() needs to be
 * called.
 *
 * If any error happens adding fields on the TLV, the previous state can be
 * recovered using qmi_message_tlv_write_reset().
 *
 * Returns: the offset where the TLV was started to be added, or 0 if an error happens.
 *
 * Since: 1.12
 */
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

/**
 * qmi_message_tlv_write_reset:
 * @self: a #QmiMessage.
 * @tlv_offset: offset that was returned by qmi_message_tlv_write_init().
 *
 * Removes the TLV being currently added.
 *
 * Since: 1.12
 */
void
qmi_message_tlv_write_reset (QmiMessage *self,
                             gsize       tlv_offset)
{
    g_return_if_fail (self != NULL);

    g_byte_array_set_size (self, tlv_offset);
}

/**
 * qmi_message_tlv_write_complete:
 * @self: a #QmiMessage.
 * @tlv_offset: offset that was returned by qmi_message_tlv_write_init().
 * @error: return location for error or %NULL.
 *
 * Completes building a TLV in the #QmiMessage.
 *
 * In case of error the TLV will be reseted; i.e. there is no need to explicitly
 * call qmi_message_tlv_write_reset().
 *
 * Returns: %TRUE if the TLV is successfully completed, otherwise %FALSE is returned and @error is set.
 *
 * Since: 1.12
 */
gboolean
qmi_message_tlv_write_complete (QmiMessage  *self,
                                gsize        tlv_offset,
                                GError     **error)
{
    gsize tlv_length;
    struct tlv *tlv;

    g_return_val_if_fail (self != NULL, FALSE);
    g_return_val_if_fail (self->len >= (tlv_offset + sizeof (struct tlv)), FALSE);

    tlv_length = self->len - tlv_offset;
    if (tlv_length == sizeof (struct tlv)) {
        g_set_error (error,
                     QMI_CORE_ERROR,
                     QMI_CORE_ERROR_TLV_EMPTY,
                     "Empty TLV, no value set");
        g_byte_array_set_size (self, tlv_offset);
        return FALSE;
    }

    /* Update length fields. */
    tlv = tlv_get_header (self, tlv_offset);
    tlv->length = GUINT16_TO_LE (tlv_length - sizeof (struct tlv));
    set_qmux_length (self, (guint16)(get_qmux_length (self) + tlv_length));
    set_all_tlvs_length (self, (guint16)(get_all_tlvs_length (self) + tlv_length));

    /* Make sure we didn't break anything. */
    g_assert (message_check (self, NULL));

    return TRUE;
}

/**
 * qmi_message_tlv_write_guint8:
 * @self: a #QmiMessage.
 * @in: a #guint8.
 * @error: return location for error or %NULL.
 *
 * Appends an unsigned byte to the TLV being built.
 *
 * Returns: %TRUE if the variable is successfully added, otherwise %FALSE is returned and @error is set.
 *
 * Since: 1.12
 */
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

/**
 * qmi_message_tlv_write_gint8:
 * @self: a #QmiMessage.
 * @in: a #gint8.
 * @error: return location for error or %NULL.
 *
 * Appends a signed byte variable to the TLV being built.
 *
 * Returns: %TRUE if the variable is successfully added, otherwise %FALSE is returned and @error is set.
 *
 * Since: 1.12
 */
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

/**
 * qmi_message_tlv_write_guint16:
 * @self: a #QmiMessage.
 * @endian: target endianness, swapped from host byte order if necessary.
 * @in: a #guint16 in host byte order.
 * @error: return location for error or %NULL.
 *
 * Appends an unsigned 16-bit integer to the TLV being built. The number to be
 * written is expected to be given in host endianness, and this method takes
 * care of converting the value written to the byte order specified by @endian.
 *
 * Returns: %TRUE if the variable is successfully added, otherwise %FALSE is returned and @error is set.
 *
 * Since: 1.12
 */
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

/**
 * qmi_message_tlv_write_gint16:
 * @self: a #QmiMessage.
 * @endian: target endianness, swapped from host byte order if necessary.
 * @in: a #gint16 in host byte order.
 * @error: return location for error or %NULL.
 *
 * Appends a signed 16-bit integer to the TLV being built. The number to be
 * written is expected to be given in host endianness, and this method takes
 * care of converting the value written to the byte order specified by @endian.
 *
 * Returns: %TRUE if the variable is successfully added, otherwise %FALSE is returned and @error is set.
 *
 * Since: 1.12
 */
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

/**
 * qmi_message_tlv_write_guint32:
 * @self: a #QmiMessage.
 * @endian: target endianness, swapped from host byte order if necessary.
 * @in: a #guint32 in host byte order.
 * @error: return location for error or %NULL.
 *
 * Appends an unsigned 32-bit integer to the TLV being built. The number to be
 * written is expected to be given in host endianness, and this method takes
 * care of converting the value written to the byte order specified by @endian.
 *
 * Returns: %TRUE if the variable is successfully added, otherwise %FALSE is returned and @error is set.
 *
 * Since: 1.12
 */
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

/**
 * qmi_message_tlv_write_gint32:
 * @self: a #QmiMessage.
 * @endian: target endianness, swapped from host byte order if necessary.
 * @in: a #gint32 in host byte order.
 * @error: return location for error or %NULL.
 *
 * Appends a signed 32-bit integer to the TLV being built. The number to be
 * written is expected to be given in host endianness, and this method takes
 * care of converting the value written to the byte order specified by @endian.
 *
 * Returns: %TRUE if the variable is successfully added, otherwise %FALSE is returned and @error is set.
 *
 * Since: 1.12
 */
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

/**
 * qmi_message_tlv_write_guint64:
 * @self: a #QmiMessage.
 * @endian: target endianness, swapped from host byte order if necessary.
 * @in: a #guint64 in host byte order.
 * @error: return location for error or %NULL.
 *
 * Appends an unsigned 64-bit integer to the TLV being built. The number to be
 * written is expected to be given in host endianness, and this method takes
 * care of converting the value written to the byte order specified by @endian.
 *
 * Returns: %TRUE if the variable is successfully added, otherwise %FALSE is returned and @error is set.
 *
 * Since: 1.12
 */
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

/**
 * qmi_message_tlv_write_gint64:
 * @self: a #QmiMessage.
 * @endian: target endianness, swapped from host byte order if necessary.
 * @in: a #gint64 in host byte order.
 * @error: return location for error or %NULL.
 *
 * Appends a signed 32-bit integer to the TLV being built. The number to be
 * written is expected to be given in host endianness, and this method takes
 * care of converting the value written to the byte order specified by @endian.
 *
 * Returns: %TRUE if the variable is successfully added, otherwise %FALSE is returned and @error is set.
 *
 * Since: 1.12
 */
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

/**
 * qmi_message_tlv_write_sized_guint:
 * @self: a #QmiMessage.
 * @n_bytes: number of bytes to write.
 * @endian: target endianness, swapped from host byte order if necessary.
 * @in: a #guint64 in host byte order.
 * @error: return location for error or %NULL.
 *
 * Appends a @n_bytes-sized unsigned integer to the TLV being built. The number
 * to be written is expected to be given in host endianness, and this method
 * takes care of converting the value written to the byte order specified by
 * @endian.
 *
 * The value of @n_bytes can be any between 1 and 8.
 *
 * Returns: %TRUE if the variable is successfully added, otherwise %FALSE is returned and @error is set.
 *
 * Since: 1.12
 */
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

/**
 * qmi_message_tlv_write_string:
 * @self: a #QmiMessage.
 * @n_size_prefix_bytes: number of bytes to use in the size prefix.
 * @in: string to write.
 * @in_length: length of @in, or -1 if @in is NUL-terminated.
 * @error: return location for error or %NULL.
 *
 * Appends a string to the TLV being built.
 *
 * Fixed-sized strings should give @n_size_prefix_bytes equal to 0.
 *
 * Returns: %TRUE if the string is successfully added, otherwise %FALSE is returned and @error is set.
 *
 * Since: 1.12
 */
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

/**
 * qmi_message_tlv_read_init:
 * @self: a #QmiMessage.
 * @type: specific ID of the TLV to read.
 * @out_tlv_length: optional return location for the TLV size.
 * @error: return location for error or %NULL.
 *
 * Starts reading a given TLV from the #QmiMessage.
 *
 * Returns: the offset where the TLV starts, or 0 if an error happens.
 *
 * Since: 1.12
 */
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
    if (!tlv_length) {
        g_set_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_TLV_EMPTY,
                     "TLV 0x%02X is empty", type);
        return 0;
    }

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

/**
 * qmi_message_tlv_read_guint8:
 * @self: a #QmiMessage.
 * @tlv_offset: offset that was returned by qmi_message_tlv_read_init().
 * @offset: address of the offset within the TLV value.
 * @out: return location for the read #guint8.
 * @error: return location for error or %NULL.
 *
 * Reads an unsigned byte from the TLV.
 *
 * @offset needs to point to a valid @gsize specifying the index to start
 * reading from within the TLV value (0 for the first item). If the variable
 * is successfully read, @offset will be updated to point past the read item.
 *
 * Returns: %TRUE if the variable is successfully read, otherwise %FALSE is returned and @error is set.
 *
 * Since: 1.12
 */
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

/**
 * qmi_message_tlv_read_gint8:
 * @self: a #QmiMessage.
 * @tlv_offset: offset that was returned by qmi_message_tlv_read_init().
 * @offset: address of a the offset within the TLV value.
 * @out: return location for the read #gint8.
 * @error: return location for error or %NULL.
 *
 * Reads a signed byte from the TLV.
 *
 * @offset needs to point to a valid @gsize specifying the index to start
 * reading from within the TLV value (0 for the first item). If the variable
 * is successfully read, @offset will be updated to point past the read item.
 *
 * Returns: %TRUE if the variable is successfully read, otherwise %FALSE is returned and @error is set.
 *
 * Since: 1.12
 */
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

/**
 * qmi_message_tlv_read_guint16:
 * @self: a #QmiMessage.
 * @tlv_offset: offset that was returned by qmi_message_tlv_read_init().
 * @offset: address of a the offset within the TLV value.
 * @endian: source endianness, which will be swapped to host byte order if necessary.
 * @out: return location for the read #guint16.
 * @error: return location for error or %NULL.
 *
 * Reads an unsigned 16-bit integer from the TLV, in host byte order.
 *
 * @offset needs to point to a valid @gsize specifying the index to start
 * reading from within the TLV value (0 for the first item). If the variable
 * is successfully read, @offset will be updated to point past the read item.
 *
 * Returns: %TRUE if the variable is successfully read, otherwise %FALSE is returned and @error is set.
 *
 * Since: 1.12
 */
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

/**
 * qmi_message_tlv_read_gint16:
 * @self: a #QmiMessage.
 * @tlv_offset: offset that was returned by qmi_message_tlv_read_init().
 * @offset: address of a the offset within the TLV value.
 * @endian: source endianness, which will be swapped to host byte order if necessary.
 * @out: return location for the read #gint16.
 * @error: return location for error or %NULL.
 *
 * Reads a signed 16-bit integer from the TLV, in host byte order.
 *
 * @offset needs to point to a valid @gsize specifying the index to start
 * reading from within the TLV value (0 for the first item). If the variable
 * is successfully read, @offset will be updated to point past the read item.
 *
 * Returns: %TRUE if the variable is successfully read, otherwise %FALSE is returned and @error is set.
 *
 * Since: 1.12
 */
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

/**
 * qmi_message_tlv_read_guint32:
 * @self: a #QmiMessage.
 * @tlv_offset: offset that was returned by qmi_message_tlv_read_init().
 * @offset: address of a the offset within the TLV value.
 * @endian: source endianness, which will be swapped to host byte order if necessary.
 * @out: return location for the read #guint32.
 * @error: return location for error or %NULL.
 *
 * Reads an unsigned 32-bit integer from the TLV, in host byte order.
 *
 * @offset needs to point to a valid @gsize specifying the index to start
 * reading from within the TLV value (0 for the first item). If the variable
 * is successfully read, @offset will be updated to point past the read item.
 *
 * Returns: %TRUE if the variable is successfully read, otherwise %FALSE is returned and @error is set.
 *
 * Since: 1.12
 */
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

/**
 * qmi_message_tlv_read_gint32:
 * @self: a #QmiMessage.
 * @tlv_offset: offset that was returned by qmi_message_tlv_read_init().
 * @offset: address of a the offset within the TLV value.
 * @endian: source endianness, which will be swapped to host byte order if necessary.
 * @out: return location for the read #gint32.
 * @error: return location for error or %NULL.
 *
 * Reads a signed 32-bit integer from the TLV, in host byte order.
 *
 * @offset needs to point to a valid @gsize specifying the index to start
 * reading from within the TLV value (0 for the first item). If the variable
 * is successfully read, @offset will be updated to point past the read item.
 *
 * Returns: %TRUE if the variable is successfully read, otherwise %FALSE is returned and @error is set.
 *
 * Since: 1.12
 */
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

/**
 * qmi_message_tlv_read_guint64:
 * @self: a #QmiMessage.
 * @tlv_offset: offset that was returned by qmi_message_tlv_read_init().
 * @offset: address of a the offset within the TLV value.
 * @endian: source endianness, which will be swapped to host byte order if necessary.
 * @out: return location for the read #guint64.
 * @error: return location for error or %NULL.
 *
 * Reads an unsigned 64-bit integer from the TLV, in host byte order.
 *
 * @offset needs to point to a valid @gsize specifying the index to start
 * reading from within the TLV value (0 for the first item). If the variable
 * is successfully read, @offset will be updated to point past the read item.
 *
 * Returns: %TRUE if the variable is successfully read, otherwise %FALSE is returned and @error is set.
 *
 * Since: 1.12
 */
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

/**
 * qmi_message_tlv_read_gint64:
 * @self: a #QmiMessage.
 * @tlv_offset: offset that was returned by qmi_message_tlv_read_init().
 * @offset: address of a the offset within the TLV value.
 * @endian: source endianness, which will be swapped to host byte order if necessary.
 * @out: return location for the read #gint64.
 * @error: return location for error or %NULL.
 *
 * Reads a signed 64-bit integer from the TLV, in host byte order.
 *
 * @offset needs to point to a valid @gsize specifying the index to start
 * reading from within the TLV value (0 for the first item). If the variable
 * is successfully read, @offset will be updated to point past the read item.
 *
 * Returns: %TRUE if the variable is successfully read, otherwise %FALSE is returned and @error is set.
 *
 * Since: 1.12
 */
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

/**
 * qmi_message_tlv_read_sized_guint:
 * @self: a #QmiMessage.
 * @tlv_offset: offset that was returned by qmi_message_tlv_read_init().
 * @offset: address of a the offset within the TLV value.
 * @n_bytes: number of bytes to read.
 * @endian: source endianness, which will be swapped to host byte order if necessary.
 * @out: return location for the read #guint64.
 * @error: return location for error or %NULL.
 *
 * Reads a @b_bytes-sized integer from the TLV, in host byte order.
 *
 * @offset needs to point to a valid @gsize specifying the index to start
 * reading from within the TLV value (0 for the first item). If the variable
 * is successfully read, @offset will be updated to point past the read item.
 *
 * Returns: %TRUE if the variable is successfully read, otherwise %FALSE is returned and @error is set.
 *
 * Since: 1.12
 */
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

/**
 * qmi_message_tlv_read_gfloat:
 * @self: a #QmiMessage.
 * @tlv_offset: offset that was returned by qmi_message_tlv_read_init().
 * @offset: address of a the offset within the TLV value.
 * @out: return location for the read #gfloat.
 * @error: return location for error or %NULL.
 *
 * Reads a 32-bit floating-point number from the TLV.
 *
 * @offset needs to point to a valid @gsize specifying the index to start
 * reading from within the TLV value (0 for the first item). If the variable
 * is successfully read, @offset will be updated to point past the read item.
 *
 * Returns: %TRUE if the variable is successfully read, otherwise %FALSE is returned and @error is set.
 *
 * Since: 1.12
 */
gboolean
qmi_message_tlv_read_gfloat (QmiMessage  *self,
                             gsize        tlv_offset,
                             gsize       *offset,
                             gfloat      *out,
                             GError     **error)
{
    const guint8 *ptr;

    g_return_val_if_fail (self != NULL, FALSE);
    g_return_val_if_fail (offset != NULL, FALSE);
    g_return_val_if_fail (out != NULL, FALSE);

    if (!(ptr = tlv_error_if_read_overflow (self, tlv_offset, *offset, 4, error)))
        return FALSE;

    /* Yeah, do this for now */
    memcpy (out, ptr, 4);
    *offset = *offset + 4;
    return TRUE;
}

/**
 * qmi_message_tlv_read_string:
 * @self: a #QmiMessage.
 * @tlv_offset: offset that was returned by qmi_message_tlv_read_init().
 * @offset: address of a the offset within the TLV value.
 * @n_size_prefix_bytes: number of bytes used in the size prefix.
 * @max_size: maximum number of bytes to read, or 0 to read all available bytes.
 * @out: return location for the read string. The returned value should be freed with g_free().
 * @error: return location for error or %NULL.
 *
 * Reads a string from the TLV.
 *
 * @offset needs to point to a valid @gsize specifying the index to start
 * reading from within the TLV value (0 for the first item). If the variable
 * is successfully read, @offset will be updated to point past the read item.
 *
 * Returns: %TRUE if the variable is successfully read, otherwise %FALSE is returned and @error is set.
 *
 * Since: 1.12
 */
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

    *out = g_malloc (valid_string_length + 1);
    memcpy (*out, ptr, valid_string_length);
    (*out)[valid_string_length] = '\0';

    *offset = (*offset + string_length);
    return TRUE;
}

/**
 * qmi_message_tlv_read_fixed_size_string:
 * @self: a #QmiMessage.
 * @tlv_offset: offset that was returned by qmi_message_tlv_read_init().
 * @offset: address of a the offset within the TLV value.
 * @string_length: amount of bytes to read.
 * @out: buffer preallocated by the client, with at least @string_length bytes.
 * @error: return location for error or %NULL.
 *
 * Reads a string from the TLV.
 *
 * The string written in @out will need to be NUL-terminated by the caller.
 *
 * @offset needs to point to a valid @gsize specifying the index to start
 * reading from within the TLV value (0 for the first item). If the variable
 * is successfully read, @offset will be updated to point past the read item.
 *
 * Returns: %TRUE if the variable is successfully read, otherwise %FALSE is returned and @error is set.
 *
 * Since: 1.12
 */
gboolean
qmi_message_tlv_read_fixed_size_string (QmiMessage  *self,
                                        gsize        tlv_offset,
                                        gsize       *offset,
                                        guint16      string_length,
                                        gchar       *out,
                                        GError     **error)
{
    g_return_val_if_fail (self != NULL, FALSE);
    g_return_val_if_fail (offset != NULL, FALSE);
    g_return_val_if_fail (out != NULL, FALSE);

    if (string_length > 0) {
        const guint8 *ptr;

        if (!(ptr = tlv_error_if_read_overflow (self, tlv_offset, *offset, string_length, error)))
            return FALSE;

        memcpy (out, ptr, string_length);
    }

    *offset = (*offset + string_length);
    return TRUE;
}

guint16
__qmi_message_tlv_read_remaining_size (QmiMessage *self,
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

/**
 * qmi_message_get_raw_tlv:
 * @self: a #QmiMessage.
 * @type: specific ID of the TLV to get.
 * @length: (out): return location for the length of the TLV.
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
 * Returns: %TRUE if the TLV is successfully added, otherwise %FALSE is returned and @error is set.
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

/**
 * qmi_message_new_from_raw:
 * @raw: (inout): raw data buffer.
 * @error: return location for error or %NULL.
 *
 * Create a new #QmiMessage from the given raw data buffer.
 *
 * Whenever a complete QMI message is read, its raw data gets removed from the @raw buffer.
 *
 * Returns: (transfer full): a newly created #QmiMessage, which should be freed with qmi_message_unref(). If @raw doesn't contain a complete QMI message #NULL is returned. If there is a complete QMI message but it appears not to be valid, #NULL is returned and @error is set.
 */
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

/**
 * qmi_message_get_tlv_printable:
 * @self: a #QmiMessage.
 * @line_prefix: prefix string to use in each new generated line.
 * @type: type of the TLV.
 * @raw: raw data buffer with the value of the TLV.
 * @raw_length: length of the raw data buffer.
 *
 * Gets a printable string with the contents of the TLV.
 *
 * This method is the most generic one and doesn't try to translate the TLV contents.
 *
 * Returns: (transfer full): a newly allocated string, which should be freed with g_free().
 */
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
    g_return_val_if_fail (raw_length > 0, NULL);

    value_hex = __qmi_utils_str_hex (raw, raw_length, ':');
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

/**
 * qmi_message_get_printable:
 * @self: a #QmiMessage.
 * @line_prefix: prefix string to use in each new generated line.
 *
 * Gets a printable string with the contents of the whole QMI message.
 *
 * If known, the printable string will contain translated TLV values as well as the raw
 * data buffer contents.
 *
 * Returns: (transfer full): a newly allocated string, which should be freed with g_free().
 */
gchar *
qmi_message_get_printable (QmiMessage *self,
                           const gchar *line_prefix)
{
    GString *printable;
    gchar *qmi_flags_str;
    gchar *contents;

    g_return_val_if_fail (self != NULL, NULL);
    g_return_val_if_fail (line_prefix != NULL, NULL);

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
        contents = __qmi_message_ctl_get_printable (self, line_prefix);
        break;
    case QMI_SERVICE_DMS:
        contents = __qmi_message_dms_get_printable (self, line_prefix);
        break;
    case QMI_SERVICE_WDS:
        contents = __qmi_message_wds_get_printable (self, line_prefix);
        break;
    case QMI_SERVICE_NAS:
        contents = __qmi_message_nas_get_printable (self, line_prefix);
        break;
    case QMI_SERVICE_WMS:
        contents = __qmi_message_wms_get_printable (self, line_prefix);
        break;
    case QMI_SERVICE_PDC:
        contents = __qmi_message_pdc_get_printable (self, line_prefix);
        break;
    case QMI_SERVICE_PDS:
        contents = __qmi_message_pds_get_printable (self, line_prefix);
        break;
    case QMI_SERVICE_PBM:
        contents = __qmi_message_pbm_get_printable (self, line_prefix);
        break;
    case QMI_SERVICE_UIM:
        contents = __qmi_message_uim_get_printable (self, line_prefix);
        break;
    case QMI_SERVICE_OMA:
        contents = __qmi_message_oma_get_printable (self, line_prefix);
        break;
    case QMI_SERVICE_WDA:
        contents = __qmi_message_wda_get_printable (self, line_prefix);
        break;
    case QMI_SERVICE_VOICE:
        contents = __qmi_message_voice_get_printable (self, line_prefix);
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

/**
 * qmi_message_get_version_introduced:
 * @self: a #QmiMessage.
 * @major: (out) return location for the major version.
 * @minor: (out) return location for the minor version.
 *
 * Gets, if known, the service version in which the given message was first introduced.
 *
 * Returns: %TRUE if @major and @minor are set, %FALSE otherwise.
 */
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
        return __qmi_message_dms_get_version_introduced (self, major, minor);

    case QMI_SERVICE_WDS:
        return __qmi_message_wds_get_version_introduced (self, major, minor);

    case QMI_SERVICE_NAS:
        return __qmi_message_nas_get_version_introduced (self, major, minor);

    case QMI_SERVICE_WMS:
        return __qmi_message_wms_get_version_introduced (self, major, minor);

    case QMI_SERVICE_PDS:
        return __qmi_message_pds_get_version_introduced (self, major, minor);

    case QMI_SERVICE_PBM:
        return __qmi_message_pbm_get_version_introduced (self, major, minor);

    case QMI_SERVICE_UIM:
        return __qmi_message_uim_get_version_introduced (self, major, minor);

    case QMI_SERVICE_OMA:
        return __qmi_message_oma_get_version_introduced (self, major, minor);

    case QMI_SERVICE_WDA:
        return __qmi_message_wda_get_version_introduced (self, major, minor);

    default:
        /* For the still unsupported services, cannot do anything */
        return FALSE;
    }
}
