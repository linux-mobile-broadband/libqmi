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
 * Copyright (C) 2021 Aleksander Morgado <aleksander@aleksander.es>
 */

#include <endian.h>
#include <errno.h>
#include <linux/qrtr.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <gio/gio.h>

#include "qrtr-bus.h"
#include "qrtr-node.h"
#include "qrtr-client.h"

static void initable_iface_init (GInitableIface *iface);

G_DEFINE_TYPE_EXTENDED (QrtrClient, qrtr_client, G_TYPE_OBJECT, 0,
                        G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE, initable_iface_init))

enum {
    PROP_0,
    PROP_NODE,
    PROP_PORT,
    PROP_LAST
};

enum {
    SIGNAL_MESSAGE,
    SIGNAL_LAST
};

static GParamSpec *properties[PROP_LAST];
static guint       signals   [SIGNAL_LAST] = { 0 };

struct _QrtrClientPrivate {
    QrtrNode *node;
    guint     node_removed_id;
    gboolean  removed;
    guint     port;

    GSocket *socket;
    GSource *source;
    struct sockaddr_qrtr addr;
};

/*****************************************************************************/

static void
node_removed_cb (QrtrClient *self)
{
    g_debug ("[qrtr client %u:%u] node removed from bus",
             qrtr_node_get_id (self->priv->node), self->priv->port);
    self->priv->removed = TRUE;
}

/*****************************************************************************/

guint32
qrtr_client_get_port (QrtrClient *self)
{
    g_return_val_if_fail (QRTR_IS_CLIENT (self), 0);

    return self->priv->port;
}

QrtrNode *
qrtr_client_peek_node (QrtrClient *self)
{
    g_return_val_if_fail (QRTR_IS_CLIENT (self), NULL);

    return self->priv->node;
}

QrtrNode *
qrtr_client_get_node (QrtrClient *self)
{
    g_return_val_if_fail (QRTR_IS_CLIENT (self), NULL);

    return g_object_ref (self->priv->node);
}

/*****************************************************************************/

gboolean
qrtr_client_send (QrtrClient    *self,
                  GByteArray    *message,
                  GCancellable  *cancellable,
                  GError       **error)
{
    gint fd;

    if (self->priv->removed) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_CLOSED,
                     "QRTR node was removed from the bus");
        return FALSE;
    }

    fd = g_socket_get_fd (self->priv->socket);
    if (sendto (fd, (void *)message->data, message->len,
                0, (struct sockaddr *)&self->priv->addr, sizeof (self->priv->addr)) < 0) {
        g_set_error (error,
                     G_IO_ERROR,
                     g_io_error_from_errno (errno),
                     "Failed to send QRTR message: %s", g_strerror (errno));
        return FALSE;
    }

    return TRUE;
}

/*****************************************************************************/

static gboolean
qrtr_message_cb (GSocket      *gsocket,
                 GIOCondition  cond,
                 QrtrClient   *self)
{
    g_autoptr(GError)         error = NULL;
    g_autoptr(GSocketAddress) addr = NULL;
    struct sockaddr_qrtr      sq;
    g_autoptr(GByteArray)     buf = NULL;
    gssize                    next_datagram_size;
    gssize                    bytes_received;

    next_datagram_size = g_socket_get_available_bytes (gsocket);
    buf = g_byte_array_sized_new (next_datagram_size);
    g_byte_array_set_size (buf, next_datagram_size);

    bytes_received = g_socket_receive_from (gsocket, &addr, (gchar *)buf->data,
                                            next_datagram_size, NULL, &error);
    if (bytes_received < 0) {
        g_warning ("[qrtr client %u:%u] socket i/o failure: %s",
                   qrtr_node_get_id (self->priv->node), self->priv->port, error->message);
        return FALSE;
    }

    if (bytes_received != next_datagram_size) {
        g_warning ("[qrtr client %u:%u] unexpected message size",
                   qrtr_node_get_id (self->priv->node), self->priv->port);
        return TRUE;
    }

    if (!g_socket_address_to_native (addr, &sq, sizeof (sq), &error)) {
        g_warning ("[qrtr client %u:%u] could not parse QRTR address: %s",
                   qrtr_node_get_id (self->priv->node), self->priv->port, error->message);
        return TRUE;
    }

    if (sq.sq_family != AF_QIPCRTR ||
        sq.sq_node != qrtr_node_get_id (self->priv->node) ||
        sq.sq_port != self->priv->port) {
        return TRUE;
    }

    g_signal_emit (self, signals[SIGNAL_MESSAGE], 0, buf);

    return TRUE;
}

/*****************************************************************************/

static gboolean
initable_init (GInitable    *initable,
               GCancellable *cancellable,
               GError      **error)
{
    QrtrClient *self = QRTR_CLIENT (initable);
    gint        fd;

    if (g_cancellable_is_cancelled (cancellable)) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_CANCELLED,
                     "Operation cancelled");
        return FALSE;
    }

    self->priv->addr.sq_family = AF_QIPCRTR;
    self->priv->addr.sq_node = qrtr_node_get_id (self->priv->node);
    self->priv->addr.sq_port = (guint) self->priv->port;

    fd = socket (AF_QIPCRTR, SOCK_DGRAM, 0);
    if (fd < 0) {
        g_set_error (error, G_IO_ERROR, g_io_error_from_errno (errno),
                     "Could not create QRTR socket: %s", g_strerror (errno));
        return FALSE;
    }

    self->priv->socket = g_socket_new_from_fd (fd, error);
    if (!self->priv->socket) {
        g_prefix_error (error, "Could not create QRTR socket: ");
        close (fd);
        return FALSE;
    }

    g_socket_set_timeout (self->priv->socket, 0);

    self->priv->source = g_socket_create_source (self->priv->socket, G_IO_IN, NULL);
    g_source_set_callback (self->priv->source, (GSourceFunc) qrtr_message_cb, self, NULL);
    g_source_attach (self->priv->source, g_main_context_get_thread_default ());

    return TRUE;
}

/*****************************************************************************/

QrtrClient *
qrtr_client_new (QrtrNode      *node,
                 guint32        port,
                 GCancellable  *cancellable,
                 GError       **error)
{
    g_return_val_if_fail (QRTR_IS_NODE (node), NULL);
    g_return_val_if_fail (port > 0, NULL);

    return g_initable_new (QRTR_TYPE_CLIENT,
                           cancellable,
                           error,
                           QRTR_CLIENT_NODE, node,
                           QRTR_CLIENT_PORT, port,
                           NULL);
}

static void
qrtr_client_init (QrtrClient *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
                                              QRTR_TYPE_CLIENT,
                                              QrtrClientPrivate);
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    QrtrClient *self = QRTR_CLIENT (object);

    switch (prop_id) {
    case PROP_NODE:
        g_assert (!self->priv->node);
        self->priv->node = g_value_dup_object (value);
        self->priv->node_removed_id = g_signal_connect_swapped (self->priv->node,
                                                                QRTR_NODE_SIGNAL_REMOVED,
                                                                G_CALLBACK (node_removed_cb),
                                                                self);
        break;
    case PROP_PORT:
        self->priv->port = (guint32) g_value_get_uint (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
get_property (GObject    *object,
              guint       prop_id,
              GValue     *value,
              GParamSpec *pspec)
{
    QrtrClient *self = QRTR_CLIENT (object);

    switch (prop_id) {
    case PROP_NODE:
        g_value_set_object (value, self->priv->node);
        break;
    case PROP_PORT:
        g_value_set_uint (value, (guint) self->priv->port);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
dispose (GObject *object)
{
    QrtrClient *self = QRTR_CLIENT (object);

    if (self->priv->source) {
        g_source_destroy (self->priv->source);
        g_clear_pointer (&self->priv->source, g_source_unref);
    }
    if (self->priv->socket) {
        if (!g_socket_is_closed (self->priv->socket))
            g_socket_close (self->priv->socket, NULL);
        g_clear_object (&self->priv->socket);
    }
    if (self->priv->node_removed_id) {
        g_signal_handler_disconnect (self->priv->node, self->priv->node_removed_id);
        self->priv->node_removed_id = 0;
    }
    g_clear_object (&self->priv->node);

    G_OBJECT_CLASS (qrtr_client_parent_class)->dispose (object);
}

static void
initable_iface_init (GInitableIface *iface)
{
    iface->init = initable_init;
}

static void
qrtr_client_class_init (QrtrClientClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (object_class, sizeof (QrtrClientPrivate));

    object_class->get_property = get_property;
    object_class->set_property = set_property;
    object_class->dispose      = dispose;

    /**
     * QrtrClient:client-node:
     *
     * Since: 1.28
     */
    properties[PROP_NODE] =
        g_param_spec_object (QRTR_CLIENT_NODE,
                             "node",
                             "The QRTR node",
                             QRTR_TYPE_NODE,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    g_object_class_install_property (object_class, PROP_NODE, properties[PROP_NODE]);

    /**
     * QrtrClient:client-port:
     *
     * Since: 1.28
     */
    properties[PROP_PORT] =
        g_param_spec_uint (QRTR_CLIENT_PORT,
                           "port",
                           "The QRTR node port",
                           0,
                           G_MAXUINT32,
                           0,
                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    g_object_class_install_property (object_class, PROP_PORT, properties[PROP_PORT]);

    /**
     * QrtrClient::client-message
     * @self: the #QrtrClient
     * @message: the message data.
     *
     * The ::client-message signal is emitted when a message is received
     * from the port in the node.
     *
     * There must be one single user connected to this signal, because it is
     * not guaranteed that the contents of the @message byte array aren't
     * modified by multiple users. In other words, the user connected to this
     * signal may modify the contents of the @message byte array if needed.
     *
     * Since: 1.28
     */
    signals[SIGNAL_MESSAGE] =
        g_signal_new (QRTR_CLIENT_SIGNAL_MESSAGE,
                      G_OBJECT_CLASS_TYPE (G_OBJECT_CLASS (klass)),
                      G_SIGNAL_RUN_LAST,
                      0,
                      NULL,
                      NULL,
                      NULL,
                      G_TYPE_NONE,
                      1,
                      G_TYPE_BYTE_ARRAY);
}
