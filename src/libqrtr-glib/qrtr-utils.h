/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * libqrtr-glib -- GLib/GIO based library to control QRTR devices
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
 * Copyright (C) 2019-2020 Eric Caruso <ejcaruso@chromium.org>
 * Copyright (C) 2020 Aleksander Morgado <aleksander@aleksander.es>
 */

#ifndef _LIBQRTR_GLIB_QRTR_UTILS_H_
#define _LIBQRTR_GLIB_QRTR_UTILS_H_

#include <gio/gio.h>
#include <glib.h>

#include "qrtr-node.h"

/**
 * qrtr_get_uri_for_node:
 * @node_id: node id.
 *
 * Build a URI for the given QRTR node.
 *
 * Returns: a string with the URI, or %NULL if none given. The returned value should be freed with g_free().
 */
gchar *qrtr_get_uri_for_node (guint32 node_id);

/**
 * qrtr_get_node_for_uri:
 * @uri: a URI.
 * @node_id: return location for the node id.
 *
 * Get the QRTR node id from the specified URI.
 *
 * Returns: %TRUE if @node_id is set, %FALSE otherwise.
 */
gboolean qrtr_get_node_for_uri (const gchar *uri,
                                guint32     *node_id);

/**
 * qrtr_node_for_id:
 * @node_id: id of the node.
 * @timeout: maximum time to wait for the node to appear, in seconds.
 * @cancellable: optional #GCancellable object, #NULL to ignore.
 * @callback: a #GAsyncReadyCallback to call when the initialization is finished.
 * @user_data: the data to pass to callback function.
 *
 * Asynchronously creates a #QrtrNode for a given node id.
 * When the operation is finished, @callback will be invoked. You can then call
 * qmi_device_new_finish() to get the result of the operation.
 */
void qrtr_node_for_id (guint32               node_id,
                       guint                 timeout,
                       GCancellable         *cancellable,
                       GAsyncReadyCallback   callback,
                       gpointer              user_data);

/**
 * qrtr_node_for_id_finish:
 * @res: a #GAsyncResult.
 * @error: Return location for error or %NULL.
 *
 * Finishes an operation started with qrtr_node_for_id().
 *
 * Returns: A newly created #QrtrNode, or %NULL if @error is set.
 */
QrtrNode *qrtr_node_for_id_finish (GAsyncResult  *res,
                                   GError       **error);

#endif /* _LIBQRTR_GLIB_QRTR_UTILS_H_ */
