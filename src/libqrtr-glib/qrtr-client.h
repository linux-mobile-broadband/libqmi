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
 * Copyright (C) 2021 Aleksander Morgado <aleksander@aleksander.es>
 */

#ifndef _LIBQRTR_GLIB_QRTR_CLIENT_H_
#define _LIBQRTR_GLIB_QRTR_CLIENT_H_

#if !defined (__LIBQRTR_GLIB_H_INSIDE__) && !defined (LIBQRTR_GLIB_COMPILATION)
#error "Only <libqrtr-glib.h> can be included directly."
#endif

#include <glib-object.h>

#include "qrtr-node.h"

G_BEGIN_DECLS

/**
 * SECTION:qrtr-client
 * @title: QrtrClient
 * @short_description: The QRTR client.
 *
 * The #QrtrClient object represents a channel that can be used to
 * communicate with a #QrtrNode in a given port.
 */

#define QRTR_TYPE_CLIENT            (qrtr_client_get_type ())
#define QRTR_CLIENT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), QRTR_TYPE_CLIENT, QrtrClient))
#define QRTR_CLIENT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  QRTR_TYPE_CLIENT, QrtrClientClass))
#define QRTR_IS_CLIENT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), QRTR_TYPE_CLIENT))
#define QRTR_IS_CLIENT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  QRTR_TYPE_CLIENT))
#define QRTR_CLIENT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  QRTR_TYPE_CLIENT, QrtrClientClass))

typedef struct _QrtrClient QrtrClient;
typedef struct _QrtrClientClass QrtrClientClass;
typedef struct _QrtrClientPrivate QrtrClientPrivate;

/**
 * QRTR_CLIENT_NODE:
 *
 * The QRTR node this client refers to.
 *
 * Since: 1.28
 */
#define QRTR_CLIENT_NODE "client-node"

/**
 * QRTR_CLIENT_PORT:
 *
 * The QRTR node port this client refers to.
 *
 * Since: 1.28
 */
#define QRTR_CLIENT_PORT "client-port"

/**
 * QRTR_CLIENT_SIGNAL_MESSAGE:
 *
 * Symbol defining the #QrtrClient::client-message signal.
 *
 * Since: 1.28
 */
#define QRTR_CLIENT_SIGNAL_MESSAGE "client-message"

struct _QrtrClient {
    GObject parent;
    QrtrClientPrivate *priv;
};

struct _QrtrClientClass {
    GObjectClass parent;
};

GType qrtr_client_get_type (void);
G_DEFINE_AUTOPTR_CLEANUP_FUNC (QrtrClient, g_object_unref)


/**
 * qrtr_client_new:
 * @node: a #QrtrNode.
 * @port: a node port.
 * @cancellable: optional #GCancellable object, %NULL to ignore.
 * @error: rReturn location for error or %NULL.
 *
 * Creates a new #QrtrClient to communicate with @port at #QrtrNode.
 *
 * Returns: (transfer full): a newly allocated #QrtrClient, or %NULL if @error is set.
 *
 * Since: 1.28
 */
QrtrClient *qrtr_client_new (QrtrNode      *node,
                             guint32        port,
                             GCancellable  *cancellable,
                             GError       **error);

/**
 * qrtr_client_peek_node:
 * @self: a #QrtrClient.
 *
 * Get the #QrtrNode that this client refers to, without increasing the
 * reference count on the returned object.
 *
 * Returns: (transfer none): a #QrtrNode. Do not free the returned object, it is
 *  owned by @self.
 *
 * Since: 1.28
 */
QrtrNode *qrtr_client_peek_node (QrtrClient *self);

/**
 * qrtr_client_get_node:
 * @self: a #QrtrClient.
 *
 * Get the #QrtrNode that this client refers to.
 *
 * Returns: (transfer full): a #QrtrNode that must be freed with g_object_unref().
 *
 * Since: 1.28
 */
QrtrNode *qrtr_client_get_node (QrtrClient *self);

/**
 * qrtr_client_get_port:
 * @self: a #QrtrClient.
 *
 * Gets the node port where this client communicates.
 *
 * Returns: the node port.
 *
 * Since: 1.28
 */
guint32 qrtr_client_get_port (QrtrClient *self);


/**
 * qrtr_client_send:
 * @self: a #QrtrClient.
 * @message: the message.
 * @cancellable: a #GCancellable.
 * @error: Return location for #GError or %NULL.
 *
 * Sends a message to the port at the node.
 *
 * Returns: %TRUE if the message is sent, or %FALSE if @error is set.
 *
 * Since: 1.28
 */
gboolean qrtr_client_send (QrtrClient    *self,
                           GByteArray    *message,
                           GCancellable  *cancellable,
                           GError       **error);

G_END_DECLS

#endif /* _LIBQRTR_GLIB_QRTR_CLIENT_H_ */
