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
 * Copyright (C) 2019 Eric Caruso <ejcaruso@chromium.org>
 */

#ifndef _LIBQMI_GLIB_QRTR_UTILS_H_
#define _LIBQMI_GLIB_QRTR_UTILS_H_

#include <gio/gio.h>
#include <glib.h>

#include "qmi-qrtr-node.h"

/* Some kernels expose the qrtr header but not the address family macro. */
#ifndef AF_QIPCRTR
#define AF_QIPCRTR 42
#endif

#define QRTR_URI_SCHEME "qrtr"

gchar *qrtr_get_uri_for_node (guint32 node_id);
gboolean qrtr_get_node_for_uri (const gchar *uri, guint32 *node_id);

void qrtr_node_for_id (guint32              node_id,
                       guint                timeout,
                       GCancellable        *cancellable,
                       GAsyncReadyCallback  callback,
                       gpointer             user_data);

QrtrNode* qrtr_node_for_id_finish (GAsyncResult  *res,
                                   GError       **error);

#endif /* _LIBQMI_GLIB_QRTR_UTILS_H_ */
