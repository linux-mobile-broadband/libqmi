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

#ifndef _LIBQRTR_GLIB_QRTR_NODE_H_
#define _LIBQRTR_GLIB_QRTR_NODE_H_

#if !defined (__LIBQRTR_GLIB_H_INSIDE__) && !defined (LIBQRTR_GLIB_COMPILATION)
#error "Only <libqrtr-glib.h> can be included directly."
#endif

#include <glib-object.h>

G_BEGIN_DECLS

/**
 * SECTION:qrtr-node
 * @title: QrtrNode
 * @short_description: QRTR node handler.
 *
 * #QrtrNode represents a device on the QRTR bus and can be used to look up
 * services published by that device.
 */

#define QRTR_TYPE_NODE            (qrtr_node_get_type ())
#define QRTR_NODE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), QRTR_TYPE_NODE, QrtrNode))
#define QRTR_NODE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  QRTR_TYPE_NODE, QrtrNodeClass))
#define QRTR_IS_NODE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), QRTR_TYPE_NODE))
#define QRTR_IS_NODE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  QRTR_TYPE_NODE))
#define QRTR_NODE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  QRTR_TYPE_NODE, QrtrNodeClass))

typedef struct _QrtrNode QrtrNode;
typedef struct _QrtrNodeClass QrtrNodeClass;
typedef struct _QrtrNodePrivate QrtrNodePrivate;

/**
 * QRTR_NODE_BUS:
 *
 * The QRTR bus.
 *
 * Since: 1.28
 */
#define QRTR_NODE_BUS "bus"

/**
 * QRTR_NODE_ID:
 *
 * The node id.
 *
 * Since: 1.28
 */
#define QRTR_NODE_ID "node-id"

struct _QrtrNode {
    GObject parent;
    QrtrNodePrivate *priv;
};

struct _QrtrNodeClass {
    GObjectClass parent;
};

GType qrtr_node_get_type (void);
G_DEFINE_AUTOPTR_CLEANUP_FUNC (QrtrNode, g_object_unref)

/**
 * QRTR_NODE_SIGNAL_SERVICE_ADDED:
 *
 * Symbol defining the #QrtrNode::service-added signal.
 *
 * Since: 1.28
 */
#define QRTR_NODE_SIGNAL_SERVICE_ADDED "service-added"

/**
 * QRTR_NODE_SIGNAL_SERVICE_REMOVED:
 *
 * Symbol defining the #QrtrNode::service-removed signal.
 *
 * Since: 1.28
 */
#define QRTR_NODE_SIGNAL_SERVICE_REMOVED "service-removed"

/**
 * QRTR_NODE_SIGNAL_REMOVED:
 *
 * Symbol defining the #QrtrNode::node-removed signal.
 *
 * Since: 1.28
 */
#define QRTR_NODE_SIGNAL_REMOVED "node-removed"

/**
 * qrtr_node_get_id:
 * @self: a #QrtrNode.
 *
 * Gets the node ID in the QRTR bus.
 *
 * Returns: the node id.
 *
 * Since: 1.28
 */
guint32 qrtr_node_get_id (QrtrNode *self);

/* forward references for the QrtrBus */
typedef struct _QrtrBus QrtrBus;
struct _QrtrBus;

/**
 * qrtr_node_peek_bus:
 * @self: a #QrtrNode.
 *
 * Get the #QrtrBus where this node is available, without increasing the
 * reference count on the returned object.
 *
 * Returns: (transfer none): a #QrtrBus. Do not free the returned object, it is
 *  owned by @self.
 *
 * Since: 1.28
 */
QrtrBus *qrtr_node_peek_bus (QrtrNode *self);

/**
 * qrtr_node_get_bus:
 * @self: a #QrtrNode.
 *
 * Get the #QrtrBus where this node is available.
 *
 * Returns: (transfer full): a #QrtrBus that must be freed with g_object_unref().
 *
 * Since: 1.28
 */
QrtrBus *qrtr_node_get_bus (QrtrNode *self);

/**
 * QrtrNodeServiceInfo:
 *
 * Information for a service instance in the #QrtrNode.
 *
 * Since: 1.28
 */
typedef struct _QrtrNodeServiceInfo QrtrNodeServiceInfo;

#define QRTR_TYPE_NODE_SERVICE_INFO (qrtr_node_service_info_get_type ())
GType qrtr_node_service_info_get_type (void);

/**
 * qrtr_node_service_info_get_service:
 * @info: a #QrtrNodeServiceInfo.
 *
 * Get the service number from the #QrtrNodeServiceInfo.
 *
 * Returns: the service number.
 *
 * Since: 1.28
 */
guint32 qrtr_node_service_info_get_service (QrtrNodeServiceInfo *info);

/**
 * qrtr_node_service_info_get_port:
 * @info: a #QrtrNodeServiceInfo.
 *
 * Get the port number from the #QrtrNodeServiceInfo.
 *
 * Returns: the port number.
 *
 * Since: 1.28
 */
guint32 qrtr_node_service_info_get_port (QrtrNodeServiceInfo *info);

/**
 * qrtr_node_service_info_get_version:
 * @info: a #QrtrNodeServiceInfo.
 *
 * Get the version number from the #QrtrNodeServiceInfo.
 *
 * Returns: the version number.
 *
 * Since: 1.28
 */
guint32 qrtr_node_service_info_get_version (QrtrNodeServiceInfo *info);

/**
 * qrtr_node_service_info_get_instance:
 * @info: a #QrtrNodeServiceInfo.
 *
 * Get the instance number from the #QrtrNodeServiceInfo.
 *
 * Returns: the instance number.
 *
 * Since: 1.28
 */
guint32 qrtr_node_service_info_get_instance (QrtrNodeServiceInfo *info);

/**
 * qrtr_node_service_info_free:
 * @info: a #QrtrNodeServiceInfo.
 *
 * Frees a single #QrtrNodeServiceInfo, as returned by qrtr_node_get_service_info_list().
 *
 * Since: 1.28
 */
void qrtr_node_service_info_free (QrtrNodeServiceInfo *info);
G_DEFINE_AUTOPTR_CLEANUP_FUNC (QrtrNodeServiceInfo, qrtr_node_service_info_free)

/**
 * qrtr_node_peek_service_info_list:
 * @self: a #QrtrNode.
 *
 * Get the list of services currently supported by the #QrtrNode.
 *
 * Returns: (element-type QrtrNodeServiceInfo)(transfer none): a #GList of #QrtrNodeServiceInfo elements. Do not
 *  free the returned object, it is owned by @self.
 *
 * Since: 1.28
 */
GList *qrtr_node_peek_service_info_list (QrtrNode *self);

/**
 * qrtr_node_get_service_info_list:
 * @self: a #QrtrNode.
 *
 * Get the list of services currently supported by the #QrtrNode.
 *
 * Returns: (element-type QrtrNodeServiceInfo)(transfer full): a #GList of #QrtrNodeServiceInfo elements, that
 *  must be freed with g_list_free_full() using qrtr_node_service_info_free()
 *  as #GDestroyNotify.
 *
 * Since: 1.28
 */
GList *qrtr_node_get_service_info_list (QrtrNode *self);

/**
 * qrtr_node_lookup_port:
 * @self: a #QrtrNode.
 * @service: a service number.
 *
 * If a server has announced itself for the given node and service number,
 * return the port number of that service.
 *
 * If multiple instances are registered, this method returns the port number
 * for the service with the highest version number.
 *
 * Returns: the port number of the service in the node, or -1 if not found.
 *
 * Since: 1.28
 */
gint32 qrtr_node_lookup_port (QrtrNode *self,
                              guint32   service);

/**
 * qrtr_node_lookup_service:
 * @self: a #QrtrNode.
 * @port: a port number.
 *
 * If a server has announced itself for the given node and port number,
 * return the service it serves.
 *
 * Returns: the service number, or -1 if not found.
 *
 * Since: 1.28
 */
gint32 qrtr_node_lookup_service (QrtrNode *self,
                                 guint32   port);

/**
 * qrtr_node_wait_for_services:
 * @self: a #QrtrNode.
 * @services: (in)(element-type guint32): a #GArray of service types
 * @timeout_ms: the timeout, in milliseconds, to wait for the the services to
 *  be exposed in the node.
 * @cancellable: a #GCancellable, or #NULL.
 * @callback: a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: user data to pass to @callback.
 *
 * Asynchronously waits until all the services listed in @services are present
 * on the node.
 *
 * The operation may fail if any of the requested services isn't notified, or
 * if the node is removed from the bus while waiting.
 *
 * When the operation is finished @callback will be called. You can then call
 * qrtr_node_wait_for_services_finish() to get the result of the
 * operation.
 *
 * Since: 1.28
 */
void qrtr_node_wait_for_services (QrtrNode            *self,
                                  GArray              *services,
                                  guint                timeout_ms,
                                  GCancellable        *cancellable,
                                  GAsyncReadyCallback  callback,
                                  gpointer             user_data);

/**
 * qrtr_node_wait_for_services_finish:
 * @self: a #QrtrNode.
 * @result: a #GAsyncResult.
 * @error: Return location for #GError or %NULL.
 *
 * Finishes an operation started with qrtr_node_wait_for_services().
 *
 * Returns: %TRUE if all requested services are present on this node,
 * or %FALSE if @error is set.
 *
 * Since: 1.28
 */
gboolean qrtr_node_wait_for_services_finish (QrtrNode      *self,
                                             GAsyncResult  *result,
                                             GError       **error);

G_END_DECLS

/* Other private methods */

#if defined (LIBQRTR_GLIB_COMPILATION)

G_GNUC_INTERNAL
void qrtr_node_add_service_info (QrtrNode *node,
                                 guint32   service,
                                 guint32   port,
                                 guint32   version,
                                 guint32   instance);

G_GNUC_INTERNAL
void qrtr_node_remove_service_info (QrtrNode *node,
                                    guint32   service,
                                    guint32   port,
                                    guint32   version,
                                    guint32   instance);

#endif /* defined (LIBQMI_GLIB_COMPILATION) */

#endif /* _LIBQRTR_GLIB_QRTR_NODE_H_ */
