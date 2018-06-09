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

#ifndef _LIBQMI_GLIB_QMI_QRTR_CONTROL_SOCKET_H_
#define _LIBQMI_GLIB_QMI_QRTR_CONTROL_SOCKET_H_

#if !defined (__LIBQMI_GLIB_H_INSIDE__) && !defined (LIBQMI_GLIB_COMPILATION)
#error "Only <libqmi-glib.h> can be included directly."
#endif

#include <glib-object.h>
#include <gio/gio.h>

#include "qmi-qrtr-node.h"

/* Some kernels expose the qrtr header but not the address family macro. */
#ifndef AF_QIPCRTR
#define AF_QIPCRTR 42
#endif

G_BEGIN_DECLS

/**
 * SECTION:qrtr-control-socket
 * @title: QrtrControlSocket
 * @short_description: QRTR bus observer and device event listener
 *
 * #QrtrControlSocket sets up a socket that uses the QRTR IPC protocol and
 * can call back into a client to tell them when new devices have appeared on
 * the QRTR bus. It holds QrtrNodes that can be used to look up service and
 * port information.
 */

#define QRTR_TYPE_CONTROL_SOCKET            (qrtr_control_socket_get_type ())
#define QRTR_CONTROL_SOCKET(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), QRTR_TYPE_CONTROL_SOCKET, QrtrControlSocket))
#define QRTR_CONTROL_SOCKET_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  QRTR_TYPE_CONTROL_SOCKET, QrtrControlSocketClass))
#define QRTR_IS_CONTROL_SOCKET(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), QRTR_TYPE_CONTROL_SOCKET))
#define QRTR_IS_CONTROL_SOCKET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  QRTR_TYPE_CONTROL_SOCKET))
#define QRTR_CONTROL_SOCKET_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  QRTR_TYPE_CONTROL_SOCKET, QrtrControlSocketClass))

typedef struct _QrtrControlSocket QrtrControlSocket;
typedef struct _QrtrControlSocketClass QrtrControlSocketClass;
typedef struct _QrtrControlSocketPrivate QrtrControlSocketPrivate;

struct _QrtrControlSocket {
    GObject parent;
    QrtrControlSocketPrivate *priv;
};

struct _QrtrControlSocketClass {
    GObjectClass parent;
};

GType qrtr_control_socket_get_type (void);

/**
 * QRTR_CONTROL_SOCKET_SIGNAL_NODE_ADDED:
 *
 * Symbol defining the #QrtrControlSocket::qrtr-node-added signal.
 *
 * Since: 1.24
 */
#define QRTR_CONTROL_SOCKET_SIGNAL_NODE_ADDED   "qrtr-node-added"

/**
 * QRTR_CONTROL_SOCKET_SIGNAL_NODE_REMOVED:
 *
 * Symbol defining the #QrtrControlSocket::qrtr-node-removed signal.
 *
 * Since: 1.24
 */
#define QRTR_CONTROL_SOCKET_SIGNAL_NODE_REMOVED "qrtr-node-removed"

/**
 * qrtr_control_socket_new:
 *
 * Creates a #QrtrControlSocket object.
 *
 * Since: 1.24
 */
QrtrControlSocket *qrtr_control_socket_new (GError **error);

/**
 * qrtr_control_socket_get_node:
 * @node_id: the QRTR bus node ID to get
 *
 * Returns a #QrtrNode object representing the device with the given node ID
 * on the QRTR bus.
 *
 * Since: 1.24
 */
QrtrNode *qrtr_control_socket_get_node (QrtrControlSocket *socket,
                                        guint32 node_id);

G_END_DECLS

#endif /* _LIBQMI_GLIB_QMI_QRTR_CONTROL_SOCKET_H_ */
