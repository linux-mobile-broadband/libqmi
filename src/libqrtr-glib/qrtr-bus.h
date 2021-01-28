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
 * Copyright (C) 2019-2021 Eric Caruso <ejcaruso@chromium.org>
 * Copyright (C) 2020-2021 Aleksander Morgado <aleksander@aleksander.es>
 */

#ifndef _LIBQRTR_GLIB_QRTR_BUS_H_
#define _LIBQRTR_GLIB_QRTR_BUS_H_

#if !defined (__LIBQRTR_GLIB_H_INSIDE__) && !defined (LIBQRTR_GLIB_COMPILATION)
#error "Only <libqrtr-glib.h> can be included directly."
#endif

#include <glib-object.h>
#include <gio/gio.h>

#include "qrtr-node.h"

G_BEGIN_DECLS

/**
 * SECTION:qrtr-bus
 * @title: QrtrBus
 * @short_description: QRTR bus observer and device event listener
 *
 * #QrtrBus sets up a socket that uses the QRTR IPC protocol and
 * can call back into a client to tell them when new devices have appeared on
 * the QRTR bus. It holds QrtrNodes that can be used to look up service and
 * port information.
 */

#define QRTR_TYPE_BUS            (qrtr_bus_get_type ())
#define QRTR_BUS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), QRTR_TYPE_BUS, QrtrBus))
#define QRTR_BUS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  QRTR_TYPE_BUS, QrtrBusClass))
#define QRTR_IS_BUS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), QRTR_TYPE_BUS))
#define QRTR_IS_BUS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  QRTR_TYPE_BUS))
#define QRTR_BUS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  QRTR_TYPE_BUS, QrtrBusClass))

typedef struct _QrtrBus        QrtrBus;
typedef struct _QrtrBusClass   QrtrBusClass;
typedef struct _QrtrBusPrivate QrtrBusPrivate;

struct _QrtrBus {
    GObject parent;
    QrtrBusPrivate *priv;
};

struct _QrtrBusClass {
    GObjectClass parent;
};

GType qrtr_bus_get_type (void);
G_DEFINE_AUTOPTR_CLEANUP_FUNC (QrtrBus, g_object_unref)

/**
 * QRTR_BUS_LOOKUP_TIMEOUT:
 *
 * Symbol defining the #QrtrBus:lookup-timeout property.
 *
 * Since: 1.28
 */
#define QRTR_BUS_LOOKUP_TIMEOUT "lookup-timeout"

/**
 * QRTR_BUS_SIGNAL_NODE_ADDED:
 *
 * Symbol defining the #QrtrBus::node-added signal.
 *
 * Since: 1.28
 */
#define QRTR_BUS_SIGNAL_NODE_ADDED "node-added"

/**
 * QRTR_BUS_SIGNAL_NODE_REMOVED:
 *
 * Symbol defining the #QrtrBus::node-removed signal.
 *
 * Since: 1.28
 */
#define QRTR_BUS_SIGNAL_NODE_REMOVED "node-removed"

/**
 * QRTR_BUS_SIGNAL_SERVICE_ADDED:
 *
 * Symbol defining the #QrtrBus::service-added signal.
 *
 * Since: 1.28
 */
#define QRTR_BUS_SIGNAL_SERVICE_ADDED "service-added"

/**
 * QRTR_BUS_SIGNAL_SERVICE_REMOVED:
 *
 * Symbol defining the #QrtrBus::service-removed signal.
 *
 * Since: 1.28
 */
#define QRTR_BUS_SIGNAL_SERVICE_REMOVED "service-removed"

/**
 * qrtr_bus_new:
 * @lookup_timeout_ms: the timeout, in milliseconds, to wait for the initial bus
 *   lookup to complete. A zero timeout disables the lookup.
 * @cancellable: optional #GCancellable object, %NULL to ignore.
 * @callback: a #GAsyncReadyCallback to call when the initialization is finished.
 * @user_data: the data to pass to callback function.
 *
 * Asynchronously creates a #QrtrBus object.
 *
 * This method will fail if the access to the QRTR bus is not possible, or if
 * the initial lookup doesn't finish on time.
 *
 * When @lookup_timeout_ms is 0, this method does not guarantee that the
 * initial bus lookup has already finished, the user should wait for the required
 * #QrtrBus::node-added or #QrtrBus::service-added signals before assuming the
 * nodes are accessible.
 *
 * When the operation is finished, @callback will be invoked. You can then call
 * qrtr_bus_new_finish() to get the result of the operation.
 *
 * Since: 1.28
 */
void qrtr_bus_new (guint                lookup_timeout_ms,
                   GCancellable        *cancellable,
                   GAsyncReadyCallback  callback,
                   gpointer             user_data);

/**
 * qrtr_bus_new_finish:
 * @res: a #GAsyncResult.
 * @error: Return location for error or %NULL.
 *
 * Finishes an operation started with qrtr_bus_new().
 *
 * Returns: (transfer full): A newly created #QrtrBus, or %NULL if @error is set.
 *
 * Since: 1.28
 */
QrtrBus *qrtr_bus_new_finish (GAsyncResult  *res,
                              GError       **error);

/**
 * qrtr_bus_peek_node:
 * @self: a #QrtrBus.
 * @node_id: the QRTR bus node ID to get
 *
 * Get the #QrtrNode with node ID @node_id, without increasing the reference count
 * on the returned object.
 *
 * This method will fail if there is no node with the given @node_id in the QRTR bus.
 *
 * Returns: (transfer none): a #QrtrNode, or %NULL if none available.
 *  Do not free the returned object, it is owned by @self.
 *
 * Since: 1.28
 */
QrtrNode *qrtr_bus_peek_node (QrtrBus *self,
                              guint32  node_id);

/**
 * qrtr_bus_get_node:
 * @self: a #QrtrBus.
 * @node_id: the QRTR bus node ID to get
 *
 * Get the #QrtrNode with node ID @node_id.
 *
 * This method will fail if there is no node with the given @node_id in the QRTR bus.
 *
 * Returns: (transfer full): a #QrtrNode that must be freed with g_object_unref(),
 *  or %NULL if none available
 *
 * Since: 1.28
 */
QrtrNode *qrtr_bus_get_node (QrtrBus *self,
                             guint32  node_id);

/**
 * qrtr_bus_wait_for_node:
 * @self: a #QrtrBus.
 * @node_id: the QRTR bus node ID to lookup.
 * @timeout_ms: the timeout, in milliseconds, to wait for the node to appear in
 *  the bus.
 * @cancellable: a #GCancellable, or #NULL.
 * @callback: a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: user data to pass to @callback.
 *
 * Asynchronously waits for the node with ID @node_id.
 *
 * When the operation is finished @callback will be called. You can then call
 * qrtr_bus_wait_for_node_finish() to get the result of the
 * operation.
 *
 * Since: 1.28
 */
void qrtr_bus_wait_for_node (QrtrBus             *self,
                             guint32              node_id,
                             guint                timeout_ms,
                             GCancellable        *cancellable,
                             GAsyncReadyCallback  callback,
                             gpointer             user_data);

/**
 * qrtr_bus_wait_for_node_finish:
 * @self: a #QrtrBus.
 * @res: a #GAsyncResult.
 * @error: Return location for error or %NULL.
 *
 * Finishes an operation started with qrtr_bus_wait_for_node().
 *
 * Returns: (transfer full): A #QrtrNode, or %NULL if @error is set.
 *
 * Since: 1.28
 */
QrtrNode *qrtr_bus_wait_for_node_finish (QrtrBus       *self,
                                         GAsyncResult  *res,
                                         GError       **error);

G_END_DECLS

#endif /* _LIBQRTR_GLIB_QRTR_BUS_H_ */
