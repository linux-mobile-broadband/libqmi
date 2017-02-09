/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 * Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
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
 * Copyright (C) 2012-2017 Aleksander Morgado <aleksander@aleksander.es>
 */

#ifndef _LIBQMI_GLIB_QMI_MESSAGE_H_
#define _LIBQMI_GLIB_QMI_MESSAGE_H_

#if !defined (__LIBQMI_GLIB_H_INSIDE__) && !defined (LIBQMI_GLIB_COMPILATION)
#error "Only <libqmi-glib.h> can be included directly."
#endif

#include <glib.h>

#include "qmi-utils.h"
#include "qmi-enums.h"
#include "qmi-errors.h"
#include "qmi-message-context.h"

G_BEGIN_DECLS

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
 */

/**
 * QMI_MESSAGE_QMUX_MARKER:
 *
 * First byte of every QMI message.
 *
 * Since: 1.0
 */
#define QMI_MESSAGE_QMUX_MARKER (guint8) 0x01

/**
 * QmiMessage:
 *
 * An opaque type representing a QMI message.
 *
 * Since: 1.0
 */
typedef GByteArray QmiMessage;

/**
 * QMI_MESSAGE_VENDOR_GENERIC:
 *
 * Generic vendor id (0x0000).
 *
 * Since: 1.18
 */
#define QMI_MESSAGE_VENDOR_GENERIC 0x0000

/*****************************************************************************/
/* QMI Message life cycle */

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
 *
 * Since: 1.0
 */
QmiMessage *qmi_message_new (QmiService service,
                             guint8     client_id,
                             guint16    transaction_id,
                             guint16    message_id);

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
 *
 * Since: 1.0
 */
QmiMessage *qmi_message_new_from_raw (GByteArray  *raw,
                                      GError     **error);

/**
 * qmi_message_response_new:
 * @request: a request #QmiMessage.
 * @error: a #QmiProtocolError to set in the result TLV.
 *
 * Create a new response #QmiMessage for the specified @request.
 *
 * Returns: (transfer full): a newly created #QmiMessage. The returned value should be freed with qmi_message_unref().
 *
 * Since: 1.8
 */
QmiMessage *qmi_message_response_new (QmiMessage       *request,
                                      QmiProtocolError  error);

/**
 * qmi_message_ref:
 * @self: a #QmiMessage.
 *
 * Atomically increments the reference count of @self by one.
 *
 * Returns: (transfer full) the new reference to @self.
 *
 * Since: 1.0
 */
QmiMessage *qmi_message_ref (QmiMessage *self);

/**
 * qmi_message_unref:
 * @self: a #QmiMessage.
 *
 * Atomically decrements the reference count of @self by one.
 * If the reference count drops to 0, @self is completely disposed.
 *
 * Since: 1.0
 */
void qmi_message_unref (QmiMessage *self);

/*****************************************************************************/
/* QMI Message content getters */

/**
 * qmi_message_is_request:
 * @self: a #QmiMessage.
 *
 * Checks whether the given #QmiMessage is a request.
 *
 * Returns: %TRUE if @self is a request message, %FALSE otherwise.
 *
 * Since: 1.8
 */
gboolean qmi_message_is_request (QmiMessage *self);

/**
 * qmi_message_is_response:
 * @self: a #QmiMessage.
 *
 * Checks whether the given #QmiMessage is a response.
 *
 * Returns: %TRUE if @self is a response message, %FALSE otherwise.
 *
 * Since: 1.0
 */
gboolean qmi_message_is_response (QmiMessage *self);

/**
 * qmi_message_is_indication:
 * @self: a #QmiMessage.
 *
 * Checks whether the given #QmiMessage is an indication.
 *
 * Returns: %TRUE if @self is an indication message, %FALSE otherwise.
 *
 * Since: 1.0
 */
gboolean qmi_message_is_indication (QmiMessage *self);

/**
 * qmi_message_get_service:
 * @self: a #QmiMessage.
 *
 * Gets the service corresponding to the given #QmiMessage.
 *
 * Returns: a #QmiService.
 *
 * Since: 1.0
 */
QmiService qmi_message_get_service (QmiMessage *self);

/**
 * qmi_message_get_client_id:
 * @self: a #QmiMessage.
 *
 * Gets the client ID of the message.
 *
 * Returns: the client ID.
 *
 * Since: 1.0
 */
guint8 qmi_message_get_client_id (QmiMessage *self);

/**
 * qmi_message_get_transaction_id:
 * @self: a #QmiMessage.
 *
 * Gets the transaction ID of the message.
 *
 * Returns: the transaction ID.
 *
 * Since: 1.0
 */
guint16 qmi_message_get_transaction_id (QmiMessage *self);

/**
 * qmi_message_get_message_id:
 * @self: a #QmiMessage.
 *
 * Gets the ID of the message.
 *
 * Returns: the ID.
 *
 * Since: 1.0
 */
guint16 qmi_message_get_message_id (QmiMessage *self);

/**
 * qmi_message_get_length:
 * @self: a #QmiMessage.
 *
 * Gets the length of the raw data corresponding to the given #QmiMessage.
 *
 * Returns: the length of the raw data.
 *
 * Since: 1.0
 */
gsize qmi_message_get_length (QmiMessage *self);

/**
 * qmi_message_get_raw:
 * @self: a #QmiMessage.
 * @length: (out): return location for the size of the output buffer.
 * @error: return location for error or %NULL.
 *
 * Gets the raw data buffer of the #QmiMessage.
 *
 * Returns: (transfer none): The raw data buffer, or #NULL if @error is set.
 *
 * Since: 1.0
 */
const guint8 *qmi_message_get_raw (QmiMessage  *self,
                                   gsize       *length,
                                   GError     **error);

/*****************************************************************************/
/* Version support from the database */

/**
 * qmi_message_get_version_introduced:
 * @self: a #QmiMessage.
 * @major: (out) return location for the major version.
 * @minor: (out) return location for the minor version.
 *
 * Gets, if known, the service version in which the given message was first
 * introduced.
 *
 * Returns: %TRUE if @major and @minor are set, %FALSE otherwise.
 *
 * Since: 1.0
 * Deprecated: 1.18: Use qmi_message_get_version_introduced_full() instead.
 */
G_DEPRECATED_FOR (qmi_message_get_version_introduced_full)
gboolean qmi_message_get_version_introduced (QmiMessage *self,
                                             guint      *major,
                                             guint      *minor);

/**
 * qmi_message_get_version_introduced_full:
 * @self: a #QmiMessage.
 * @context: a #QmiMessageContext.
 * @major: (out) return location for the major version.
 * @minor: (out) return location for the minor version.
 *
 * Gets, if known, the service version in which the given message was first
 * introduced.
 *
 * The lookup of the version may be specific to the @context provided, e.g. for
 * vendor-specific messages.
 *
 * If no @context given, the behavior is the same as qmi_message_get_version_introduced().
 *
 * Returns: %TRUE if @major and @minor are set, %FALSE otherwise.
 *
 * Since: 1.18
 */
gboolean qmi_message_get_version_introduced_full (QmiMessage        *self,
                                                  QmiMessageContext *context,
                                                  guint             *major,
                                                  guint             *minor);

/*****************************************************************************/
/* TLV builder & writer */

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
gsize qmi_message_tlv_write_init (QmiMessage  *self,
                                  guint8       type,
                                  GError     **error);

/**
 * qmi_message_tlv_write_reset:
 * @self: a #QmiMessage.
 * @tlv_offset: offset that was returned by qmi_message_tlv_write_init().
 *
 * Removes the TLV being currently added.
 *
 * Since: 1.12
 */
void qmi_message_tlv_write_reset (QmiMessage *self,
                                  gsize       tlv_offset);

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
gboolean qmi_message_tlv_write_complete (QmiMessage  *self,
                                         gsize        tlv_offset,
                                         GError     **error);

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
gboolean qmi_message_tlv_write_guint8 (QmiMessage  *self,
                                       guint8       in,
                                       GError     **error);

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
gboolean qmi_message_tlv_write_gint8 (QmiMessage  *self,
                                      gint8        in,
                                      GError     **error);

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
gboolean qmi_message_tlv_write_guint16 (QmiMessage  *self,
                                        QmiEndian    endian,
                                        guint16      in,
                                        GError     **error);

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
gboolean qmi_message_tlv_write_gint16 (QmiMessage  *self,
                                       QmiEndian    endian,
                                       gint16       in,
                                       GError     **error);

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
gboolean qmi_message_tlv_write_guint32 (QmiMessage  *self,
                                        QmiEndian    endian,
                                        guint32      in,
                                        GError     **error);

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
gboolean qmi_message_tlv_write_gint32 (QmiMessage  *self,
                                       QmiEndian    endian,
                                       gint32       in,
                                       GError     **error);

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
gboolean qmi_message_tlv_write_guint64 (QmiMessage  *self,
                                        QmiEndian    endian,
                                        guint64      in,
                                        GError     **error);

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
gboolean qmi_message_tlv_write_gint64 (QmiMessage  *self,
                                       QmiEndian    endian,
                                       gint64       in,
                                       GError     **error);

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
gboolean qmi_message_tlv_write_sized_guint (QmiMessage  *self,
                                            guint        n_bytes,
                                            QmiEndian    endian,
                                            guint64      in,
                                            GError     **error);


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
gboolean qmi_message_tlv_write_string (QmiMessage   *self,
                                       guint8        n_size_prefix_bytes,
                                       const gchar  *in,
                                       gssize        in_length,
                                       GError      **error);

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
gsize qmi_message_tlv_read_init (QmiMessage  *self,
                                 guint8       type,
                                 guint16     *out_tlv_length,
                                 GError     **error);

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
gboolean qmi_message_tlv_read_guint8 (QmiMessage  *self,
                                      gsize        tlv_offset,
                                      gsize       *offset,
                                      guint8      *out,
                                      GError     **error);

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
gboolean qmi_message_tlv_read_gint8 (QmiMessage  *self,
                                     gsize        tlv_offset,
                                     gsize       *offset,
                                     gint8       *out,
                                     GError     **error);

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
gboolean qmi_message_tlv_read_guint16 (QmiMessage  *self,
                                       gsize        tlv_offset,
                                       gsize       *offset,
                                       QmiEndian    endian,
                                       guint16     *out,
                                       GError     **error);

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
gboolean qmi_message_tlv_read_gint16 (QmiMessage  *self,
                                      gsize        tlv_offset,
                                      gsize       *offset,
                                      QmiEndian    endian,
                                      gint16      *out,
                                      GError     **error);

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
gboolean qmi_message_tlv_read_guint32 (QmiMessage  *self,
                                       gsize        tlv_offset,
                                       gsize       *offset,
                                       QmiEndian    endian,
                                       guint32     *out,
                                       GError     **error);

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
gboolean qmi_message_tlv_read_gint32 (QmiMessage  *self,
                                      gsize        tlv_offset,
                                      gsize       *offset,
                                      QmiEndian    endian,
                                      gint32      *out,
                                      GError     **error);

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
gboolean qmi_message_tlv_read_guint64 (QmiMessage  *self,
                                       gsize        tlv_offset,
                                       gsize       *offset,
                                       QmiEndian    endian,
                                       guint64     *out,
                                       GError     **error);

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
gboolean qmi_message_tlv_read_gint64 (QmiMessage  *self,
                                      gsize        tlv_offset,
                                      gsize       *offset,
                                      QmiEndian    endian,
                                      gint64      *out,
                                      GError     **error);

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
gboolean qmi_message_tlv_read_sized_guint (QmiMessage  *self,
                                           gsize        tlv_offset,
                                           gsize       *offset,
                                           guint        n_bytes,
                                           QmiEndian    endian,
                                           guint64     *out,
                                           GError     **error);

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
gboolean qmi_message_tlv_read_gfloat (QmiMessage  *self,
                                      gsize        tlv_offset,
                                      gsize       *offset,
                                      gfloat      *out,
                                      GError     **error);

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
gboolean qmi_message_tlv_read_string (QmiMessage  *self,
                                      gsize        tlv_offset,
                                      gsize       *offset,
                                      guint8       n_size_prefix_bytes,
                                      guint16      max_size,
                                      gchar      **out,
                                      GError     **error);

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
gboolean qmi_message_tlv_read_fixed_size_string (QmiMessage  *self,
                                                 gsize        tlv_offset,
                                                 gsize       *offset,
                                                 guint16      string_length,
                                                 gchar       *out,
                                                 GError     **error);

#if defined (LIBQMI_GLIB_COMPILATION)
G_GNUC_INTERNAL
guint16 __qmi_message_tlv_read_remaining_size (QmiMessage  *self,
                                               gsize        tlv_offset,
                                               gsize        offset);
#endif

/*****************************************************************************/
/* Raw TLV handling */

/**
 * QmiMessageForeachRawTlvFn:
 * @type: specific ID of the TLV.
 * @value: value of the TLV.
 * @length: length of the TLV, in bytes.
 * @user_data: user data.
 *
 * Callback type to use when iterating raw TLVs with
 * qmi_message_foreach_raw_tlv().
 *
 * Since: 1.0
 */
typedef void (* QmiMessageForeachRawTlvFn) (guint8        type,
                                            const guint8 *value,
                                            gsize         length,
                                            gpointer      user_data);

/**
 * qmi_message_foreach_raw_tlv:
 * @self: a #QmiMessage.
 * @func: the function to call for each TLV.
 * @user_data: user data to pass to the function.
 *
 * Calls the given function for each TLV found within the #QmiMessage.
 *
 * Since: 1.0
 */
void qmi_message_foreach_raw_tlv (QmiMessage                *self,
                                  QmiMessageForeachRawTlvFn  func,
                                  gpointer                   user_data);

/**
 * qmi_message_get_raw_tlv:
 * @self: a #QmiMessage.
 * @type: specific ID of the TLV to get.
 * @length: (out): return location for the length of the TLV.
 *
 * Get the raw data buffer of a specific TLV within the #QmiMessage.
 *
 * Returns: (transfer none): The raw data buffer of the TLV, or #NULL if not found.
 *
 * Since: 1.0
 */
const guint8 *qmi_message_get_raw_tlv (QmiMessage *self,
                                       guint8      type,
                                       guint16    *length);

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
 *
 * Since: 1.0
 */
gboolean qmi_message_add_raw_tlv (QmiMessage   *self,
                                  guint8        type,
                                  const guint8 *raw,
                                  gsize         length,
                                  GError      **error);

/*****************************************************************************/
/* Other helpers */

/**
 * qmi_message_set_transaction_id:
 * @self: a #QmiMessage.
 * @transaction_id: transaction id.
 *
 * Overwrites the transaction ID of the message.
 *
 * Since: 1.8
 */
void qmi_message_set_transaction_id (QmiMessage *self,
                                     guint16 transaction_id);

/*****************************************************************************/
/* Printable helpers */

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
 *
 * Since: 1.0
 * Deprecated: 1.18: Use qmi_message_get_printable_full() instead.
 */
G_DEPRECATED_FOR (qmi_message_get_printable_full)
gchar *qmi_message_get_printable (QmiMessage  *self,
                                  const gchar *line_prefix);

/**
 * qmi_message_get_printable_full:
 * @self: a #QmiMessage.
 * @context: a #QmiMessageContext.
 * @line_prefix: prefix string to use in each new generated line.
 *
 * Gets a printable string with the contents of the whole QMI message.
 *
 * If known, the printable string will contain translated TLV values as well as
 * the raw data buffer contents.
 *
 * The translation of the contents may be specific to the @context provided,
 * e.g. for vendor-specific messages.
 *
 * If no @context given, the behavior is the same as qmi_message_get_printable().
 *
 * Returns: (transfer full): a newly allocated string, which should be freed with g_free().
 *
 * Since: 1.18
 */
gchar *qmi_message_get_printable_full (QmiMessage        *self,
                                       QmiMessageContext *context,
                                       const gchar       *line_prefix);

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
 *
 * Since: 1.0
 */
gchar *qmi_message_get_tlv_printable (QmiMessage   *self,
                                      const gchar  *line_prefix,
                                      guint8        type,
                                      const guint8 *raw,
                                      gsize         raw_length);

G_END_DECLS

#endif /* _LIBQMI_GLIB_QMI_MESSAGE_H_ */
