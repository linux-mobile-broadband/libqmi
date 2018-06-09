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

#ifndef _LIBQMI_GLIB_QMI_QRTR_NODE_H_
#define _LIBQMI_GLIB_QMI_QRTR_NODE_H_

#if !defined (__LIBQMI_GLIB_H_INSIDE__) && !defined (LIBQMI_GLIB_COMPILATION)
#error "Only <libqmi-glib.h> can be included directly."
#endif

#include <glib-object.h>

#include "qmi-enums.h"

G_BEGIN_DECLS

/* Forward declare QrtrControlSocket for qrtr_node_new. */
struct _QrtrControlSocket;

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
 *
 * Since: 1.24
 */
#define QRTR_NODE_SIGNAL_REMOVED "removed"

/**
 * qrtr_node_has_services:
 *
 * Returns TRUE if there are services currently registered on this node.
 *
 * Since: 1.24
 */
gboolean qrtr_node_has_services (QrtrNode *node);

/**
 * qrtr_node_id:
 *
 * Returns the node id of the QRTR node represented by @node.
 *
 * Since: 1.24
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
 *
 * Since: 1.24
 */
gint32 qrtr_node_lookup_port (QrtrNode *node, QmiService service);

/**
 * qrtr_node_lookup_service:
 * @port: a port number
 *
 * If a server has announced itself for the given node and port number,
 * return the QMI service it serves. Otherwise, return @QMI_SERVICE_UNKNOWN.
 *
 * Since: 1.24
 */
QmiService qrtr_node_lookup_service (QrtrNode *node, guint32 port);

G_END_DECLS

/*
 * qrtr_node_new:
 * @socket: the control socket that created this QrtrNode
 * @node: the node number of this QrtrNode
 */
QrtrNode *qrtr_node_new (struct _QrtrControlSocket *socket, guint32 node);

/*
 * qrtr_node_add_service_info:
 * @service: a #QmiService value representing this service
 * @port: the port number of the new service
 * @version: the version number of the new service
 * @instance: the instance number of the new service
 *
 * Adds the given service entry to this node. Should only be called by the
 * #QrtrControlSocket.
 */
void qrtr_node_add_service_info (QrtrNode *node, QmiService service, guint32 port,
                                 guint32 version, guint32 instance);

/*
 * qrtr_node_remove_service_info:
 * @info: a #QrtrServceInfo struct
 *
 * Removes the given service entry from this node. Should only be called by the
 * #QrtrControlSocket.
 */
void qrtr_node_remove_service_info (QrtrNode *node, QmiService service, guint32 port,
                                    guint32 version, guint32 instance);

#endif /* _LIBQMI_GLIB_QMI_QRTR_NODE_H_ */
