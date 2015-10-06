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
 * Copyright (C) 2012-2015 Aleksander Morgado <aleksander@aleksander.es>
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

G_BEGIN_DECLS

#define QMI_MESSAGE_QMUX_MARKER (guint8)0x01

/**
 * QmiMessage:
 *
 * An opaque type representing a QMI message.
 */
typedef GByteArray QmiMessage;

/*****************************************************************************/
/* QMI Message life cycle */

QmiMessage   *qmi_message_new          (QmiService service,
                                        guint8 client_id,
                                        guint16 transaction_id,
                                        guint16 message_id);
QmiMessage   *qmi_message_new_from_raw (GByteArray *raw,
                                        GError **error);
QmiMessage   *qmi_message_response_new (QmiMessage *request,
                                        QmiProtocolError error);
QmiMessage   *qmi_message_ref          (QmiMessage *self);
void          qmi_message_unref        (QmiMessage *self);

/*****************************************************************************/
/* QMI Message content getters */

gboolean      qmi_message_is_request            (QmiMessage *self);
gboolean      qmi_message_is_response           (QmiMessage *self);
gboolean      qmi_message_is_indication         (QmiMessage *self);
QmiService    qmi_message_get_service           (QmiMessage *self);
guint8        qmi_message_get_client_id         (QmiMessage *self);
guint16       qmi_message_get_transaction_id    (QmiMessage *self);
guint16       qmi_message_get_message_id        (QmiMessage *self);
gsize         qmi_message_get_length            (QmiMessage *self);
const guint8 *qmi_message_get_raw               (QmiMessage *self,
                                                 gsize *length,
                                                 GError **error);
gboolean     qmi_message_get_version_introduced (QmiMessage *self,
                                                 guint *major,
                                                 guint *minor);


/*****************************************************************************/
/* TLV builder & writer */

gsize    qmi_message_tlv_write_init        (QmiMessage   *self,
                                            guint8        type,
                                            GError      **error);
void     qmi_message_tlv_write_reset       (QmiMessage   *self,
                                            gsize         tlv_offset);
gboolean qmi_message_tlv_write_complete    (QmiMessage   *self,
                                            gsize         tlv_offset,
                                            GError      **error);
gboolean qmi_message_tlv_write_guint8      (QmiMessage   *self,
                                            guint8        in,
                                            GError      **error);
gboolean qmi_message_tlv_write_gint8       (QmiMessage   *self,
                                            gint8         in,
                                            GError      **error);
gboolean qmi_message_tlv_write_guint16     (QmiMessage   *self,
                                            QmiEndian     endian,
                                            guint16       in,
                                            GError      **error);
gboolean qmi_message_tlv_write_gint16      (QmiMessage   *self,
                                            QmiEndian     endian,
                                            gint16        in,
                                            GError      **error);
gboolean qmi_message_tlv_write_guint32     (QmiMessage   *self,
                                            QmiEndian     endian,
                                            guint32       in,
                                            GError      **error);
gboolean qmi_message_tlv_write_gint32      (QmiMessage   *self,
                                            QmiEndian     endian,
                                            gint32        in,
                                            GError      **error);
gboolean qmi_message_tlv_write_guint64     (QmiMessage   *self,
                                            QmiEndian     endian,
                                            guint64       in,
                                            GError      **error);
gboolean qmi_message_tlv_write_gint64      (QmiMessage   *self,
                                            QmiEndian     endian,
                                            gint64        in,
                                            GError      **error);
gboolean qmi_message_tlv_write_sized_guint (QmiMessage   *self,
                                            guint         n_bytes,
                                            QmiEndian     endian,
                                            guint64       in,
                                            GError      **error);
gboolean qmi_message_tlv_write_string      (QmiMessage   *self,
                                            guint8        n_size_prefix_bytes,
                                            const gchar  *in,
                                            gssize        in_length,
                                            GError      **error);

/*****************************************************************************/
/* TLV reader */

gsize    qmi_message_tlv_read_init              (QmiMessage  *self,
                                                 guint8       type,
                                                 guint16     *out_tlv_length,
                                                 GError     **error);
gboolean qmi_message_tlv_read_guint8            (QmiMessage  *self,
                                                 gsize        tlv_offset,
                                                 gsize       *offset,
                                                 guint8      *out,
                                                 GError     **error);
gboolean qmi_message_tlv_read_gint8             (QmiMessage  *self,
                                                 gsize        tlv_offset,
                                                 gsize       *offset,
                                                 gint8       *out,
                                                 GError     **error);
gboolean qmi_message_tlv_read_guint16           (QmiMessage  *self,
                                                 gsize        tlv_offset,
                                                 gsize       *offset,
                                                 QmiEndian    endian,
                                                 guint16     *out,
                                                 GError     **error);
gboolean qmi_message_tlv_read_gint16            (QmiMessage  *self,
                                                 gsize        tlv_offset,
                                                 gsize       *offset,
                                                 QmiEndian    endian,
                                                 gint16      *out,
                                                 GError     **error);
gboolean qmi_message_tlv_read_guint32           (QmiMessage  *self,
                                                 gsize        tlv_offset,
                                                 gsize       *offset,
                                                 QmiEndian    endian,
                                                 guint32     *out,
                                                 GError     **error);
gboolean qmi_message_tlv_read_gint32            (QmiMessage  *self,
                                                 gsize        tlv_offset,
                                                 gsize       *offset,
                                                 QmiEndian    endian,
                                                 gint32      *out,
                                                 GError     **error);
gboolean qmi_message_tlv_read_guint64           (QmiMessage  *self,
                                                 gsize        tlv_offset,
                                                 gsize       *offset,
                                                 QmiEndian    endian,
                                                 guint64     *out,
                                                 GError     **error);
gboolean qmi_message_tlv_read_gint64            (QmiMessage  *self,
                                                 gsize        tlv_offset,
                                                 gsize       *offset,
                                                 QmiEndian    endian,
                                                 gint64      *out,
                                                 GError     **error);
gboolean qmi_message_tlv_read_sized_guint       (QmiMessage  *self,
                                                 gsize        tlv_offset,
                                                 gsize       *offset,
                                                 guint        n_bytes,
                                                 QmiEndian    endian,
                                                 guint64     *out,
                                                 GError     **error);
gboolean qmi_message_tlv_read_gfloat            (QmiMessage  *self,
                                                 gsize        tlv_offset,
                                                 gsize       *offset,
                                                 gfloat      *out,
                                                 GError     **error);
gboolean qmi_message_tlv_read_string            (QmiMessage  *self,
                                                 gsize        tlv_offset,
                                                 gsize       *offset,
                                                 guint8       n_size_prefix_bytes,
                                                 guint16      max_size,
                                                 gchar      **out,
                                                 GError     **error);
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

typedef void (* QmiMessageForeachRawTlvFn) (guint8 type,
                                            const guint8 *value,
                                            gsize length,
                                            gpointer user_data);
void          qmi_message_foreach_raw_tlv  (QmiMessage *self,
                                            QmiMessageForeachRawTlvFn func,
                                            gpointer user_data);
const guint8 *qmi_message_get_raw_tlv      (QmiMessage *self,
                                            guint8 type,
                                            guint16 *length);
gboolean      qmi_message_add_raw_tlv      (QmiMessage *self,
                                            guint8 type,
                                            const guint8 *raw,
                                            gsize length,
                                            GError **error);

/*****************************************************************************/
/* Other helpers */

void qmi_message_set_transaction_id (QmiMessage *self,
                                     guint16 transaction_id);

/*****************************************************************************/
/* Printable helpers */

gchar *qmi_message_get_printable (QmiMessage *self,
                                  const gchar *line_prefix);

gchar *qmi_message_get_tlv_printable (QmiMessage *self,
                                      const gchar *line_prefix,
                                      guint8 type,
                                      const guint8 *raw,
                                      gsize raw_length);

G_END_DECLS

#endif /* _LIBQMI_GLIB_QMI_MESSAGE_H_ */
