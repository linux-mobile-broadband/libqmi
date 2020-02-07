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

#ifndef _LIBQRTR_GLIB_QRTR_NODE_H_
#define _LIBQRTR_GLIB_QRTR_NODE_H_

#if !defined (__LIBQRTR_GLIB_H_INSIDE__) && !defined (LIBQRTR_GLIB_COMPILATION)
#error "Only <libqrtr-glib.h> can be included directly."
#endif

#include <glib-object.h>

G_BEGIN_DECLS

/* Forward declare QrtrControlSocket for qrtr_node_new. */
typedef struct _QrtrControlSocket QrtrControlSocket;

/**
 * SECTION:qrtr-node
 * @title: QrtrNode
 * @short_description: QRTR bus observer and service event listener
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
 * QRTR_NODE_SOCKET:
 *
 * The control socket.
 */
#define QRTR_NODE_SOCKET "socket"

/**
 * QRTR_NODE_ID:
 *
 * The node id.
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

/**
 * QRTR_NODE_SIGNAL_REMOVED:
 *
 * Symbol defining the #QrtrNode::removed signal.
 */
#define QRTR_NODE_SIGNAL_REMOVED "removed"

/**
 * qrtr_node_has_services:
 *
 * Returns TRUE if there are services currently registered on this node.
 */
gboolean qrtr_node_has_services (QrtrNode *node);

/**
 * qrtr_node_id:
 *
 * Returns the node id of the QRTR node represented by @node.
 */
guint32 qrtr_node_id (QrtrNode *node);

/**
 * qrtr_node_lookup_port:
 * @service: a QMI service
 *
 * If a server has announced itself for the given node and service number,
 * return the port number of that service. Otherwise, return -1. This will
 * return the service with the highest version number if multiple instances
 * are registered.
 */
gint32 qrtr_node_lookup_port (QrtrNode *node,
                              guint32   service);

/**
 * qrtr_node_lookup_service:
 * @port: a port number
 *
 * If a server has announced itself for the given node and port number,
 * return the service it serves. Otherwise, return -1.
 */
gint32 qrtr_node_lookup_service (QrtrNode *node,
                                 guint32   port);

G_END_DECLS

/**
 * qrtr_node_new:
 * @socket: the control socket that created this QrtrNode.
 * @node: the node number of this QrtrNode.
 *
 * Create a new QRTR node.
 */
QrtrNode *qrtr_node_new (QrtrControlSocket *socket,
                         guint32            node);

/* Other private methods */

#if defined (LIBQRTR_GLIB_COMPILATION)

G_GNUC_INTERNAL
void __qrtr_node_add_service_info (QrtrNode *node,
                                   guint32   service,
                                   guint32   port,
                                   guint32   version,
                                   guint32   instance);

G_GNUC_INTERNAL
void __qrtr_node_remove_service_info (QrtrNode *node,
                                      guint32   service,
                                      guint32   port,
                                      guint32   version,
                                      guint32   instance);

#endif /* defined (LIBQMI_GLIB_COMPILATION) */

#endif /* _LIBQRTR_GLIB_QRTR_NODE_H_ */
