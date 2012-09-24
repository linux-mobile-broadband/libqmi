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
 * Copyright (C) 2012 Aleksander Morgado <aleksander@lanedo.com>
 */

#ifndef _LIBQMI_GLIB_QMI_MESSAGE_H_
#define _LIBQMI_GLIB_QMI_MESSAGE_H_

#include <glib.h>

#include "qmi-enums.h"

G_BEGIN_DECLS

#define QMI_MESSAGE_QMUX_MARKER (guint8)0x01
typedef struct _QmiMessage QmiMessage;

QmiMessage *qmi_message_new          (QmiService service,
                                      guint8 client_id,
                                      guint16 transaction_id,
                                      guint16 message_id);
QmiMessage *qmi_message_new_from_raw (const guint8 *raw,
                                      gsize raw_len);
QmiMessage *qmi_message_ref          (QmiMessage *self);
void qmi_message_unref               (QmiMessage *self);

typedef void (* QmiMessageForeachRawTlvFn) (guint8 type,
                                            const guint8 *value,
                                            gsize length,
                                            gpointer user_data);
void qmi_message_foreach_raw_tlv (QmiMessage *self,
                                  QmiMessageForeachRawTlvFn func,
                                  gpointer user_data);

const guint8 *qmi_message_get_raw_tlv (QmiMessage *self,
                                       guint8 type,
                                       guint16 *length);

gboolean qmi_message_add_raw_tlv (QmiMessage *self,
                                  guint8 type,
                                  const guint8 *raw,
                                  gsize length,
                                  GError **error);

gconstpointer qmi_message_get_raw (QmiMessage *self,
                                   gsize *length,
                                   GError **error);

guint16 qmi_message_get_qmux_length (QmiMessage *self);
guint16 qmi_message_get_tlv_length  (QmiMessage *self);
gsize   qmi_message_get_length      (QmiMessage *self);

gchar *qmi_message_get_printable (QmiMessage *self,
                                  const gchar *line_prefix);

gchar *qmi_message_get_tlv_printable (QmiMessage *self,
                                      const gchar *line_prefix,
                                      guint8 type,
                                      gsize length,
                                      gconstpointer value);

gboolean qmi_message_check (QmiMessage *self,
                            GError **error);

gboolean qmi_message_is_control    (QmiMessage *self);
gboolean qmi_message_is_response   (QmiMessage *self);
gboolean qmi_message_is_indication (QmiMessage *self);

guint16    qmi_message_get_message_id     (QmiMessage *self);
QmiService qmi_message_get_service        (QmiMessage *self);
guint8     qmi_message_get_client_id      (QmiMessage *self);
guint16    qmi_message_get_transaction_id (QmiMessage *self);
guint8     qmi_message_get_qmux_flags     (QmiMessage *self);
guint8     qmi_message_get_qmi_flags      (QmiMessage *self);

gboolean   qmi_message_get_version_introduced (QmiMessage *self,
                                               guint *major,
                                               guint *minor);

G_END_DECLS

#endif /* _LIBQMI_GLIB_QMI_MESSAGE_H_ */
