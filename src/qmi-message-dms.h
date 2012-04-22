/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

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

/* NOTE: this is a private non-installable header */

#ifndef _LIBQMI_GLIB_QMI_MESSAGE_DMS_H_
#define _LIBQMI_GLIB_QMI_MESSAGE_DMS_H_

#include <glib.h>
#include "qmi-message.h"

G_BEGIN_DECLS

/* Get IDs */

QmiMessage *qmi_message_dms_get_ids_new         (guint8 transaction_id,
                                                 guint8 client_id);
gboolean    qmi_message_dms_get_ids_reply_parse (QmiMessage *self,
                                                 gchar **esn,
                                                 gchar **imei,
                                                 gchar **meid,
                                                 GError **error);

G_END_DECLS

#endif /* _LIBQMI_GLIB_QMI_MESSAGE_DMS_H_ */
