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

#include <endian.h>
#include <errno.h>
#include <linux/qrtr.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <gio/gio.h>

#include "qrtr-control-socket.h"
#include "qrtr-node.h"
#include "qrtr-utils.h"

static void initable_iface_init (GInitableIface *iface);

G_DEFINE_TYPE_EXTENDED (QrtrControlSocket, qrtr_control_socket, G_TYPE_OBJECT, 0,
                        G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE, initable_iface_init))

enum {
    SIGNAL_NODE_ADDED,
    SIGNAL_NODE_REMOVED,
    SIGNAL_SERVICE_ADDED,
    SIGNAL_SERVICE_REMOVED,
    SIGNAL_LAST
};

static guint signals[SIGNAL_LAST] = { 0 };

struct _QrtrControlSocketPrivate {
    /* Underlying QRTR socket */
    GSocket *socket;
    /* Map of node id -> QrtrNode */
    GHashTable *node_map;
    /* Callback source for when NEW_SERVER/DEL_SERVER control packets come in */
    GSource *source;
};

/*****************************************************************************/

static void
add_service_info (QrtrControlSocket *self,
                  guint32            node_id,
                  guint32            port,
                  guint32            service,
                  guint32            version,
                  guint32            instance)
{
    QrtrNode *node;

    node = g_hash_table_lookup (self->priv->node_map, GUINT_TO_POINTER (node_id));
    if (!node) {
        /* Node objects are exclusively created at this point */
        node = QRTR_NODE (g_object_new (QRTR_TYPE_NODE,
                                        QRTR_NODE_SOCKET, self,
                                        QRTR_NODE_ID,     node_id,
                                        NULL));
        g_assert (g_hash_table_insert (self->priv->node_map, GUINT_TO_POINTER (node_id), node));
        g_debug ("[qrtr] created new node %u", node_id);
        g_signal_emit (self, signals[SIGNAL_NODE_ADDED], 0, node_id);
    }

    qrtr_node_add_service_info (node, service, port, version, instance);
    g_signal_emit (self, signals[SIGNAL_SERVICE_ADDED], 0, node_id, service);
}

static void
remove_service_info (QrtrControlSocket *self,
                     guint32            node_id,
                     guint32            port,
                     guint32            service,
                     guint32            version,
                     guint32            instance)
{
    QrtrNode *node;

    node = g_hash_table_lookup (self->priv->node_map, GUINT_TO_POINTER (node_id));
    if (!node) {
        g_warning ("[qrtr] cannot remove service info: nonexistent node %u", node_id);
        return;
    }

    qrtr_node_remove_service_info (node, service, port, version, instance);
    g_signal_emit (self, signals[SIGNAL_SERVICE_REMOVED], 0, node_id, service);
    if (!qrtr_node_has_services (node)) {
        g_debug ("[qrtr] removing node %u", node_id);
        g_signal_emit (self, signals[SIGNAL_NODE_REMOVED], 0, node_id);
        g_hash_table_remove (self->priv->node_map, GUINT_TO_POINTER (node_id));
    }
}

/*****************************************************************************/

static gboolean
qrtr_ctrl_message_cb (GSocket           *gsocket,
                      GIOCondition       cond,
                      QrtrControlSocket *self)
{
    GError               *error = NULL;
    struct qrtr_ctrl_pkt  ctrl_packet;
    gssize                bytes_received;
    guint32               type;
    guint32               node_id;
    guint32               port;
    guint32               service;
    guint32               version;
    guint32               instance;

    /* check for message type and add/remove nodes here */

    bytes_received = g_socket_receive (gsocket, (gchar *)&ctrl_packet,
                                       sizeof (ctrl_packet), NULL, &error);
    if (bytes_received < 0) {
        g_warning ("[qrtr] socket i/o failure: %s", error->message);
        g_error_free (error);
        return FALSE;
    }

    if ((gsize)bytes_received < sizeof (ctrl_packet)) {
        g_debug ("[qrtr] short packet received: ignoring");
        return TRUE;
    }

    type = GUINT32_FROM_LE (ctrl_packet.cmd);
    if (type != QRTR_TYPE_NEW_SERVER && type != QRTR_TYPE_DEL_SERVER) {
        g_debug ("[qrtr] unknown packet type received: 0x%x", type);
        return TRUE;
    }

    /* type is something we handle, parse the packet */
    node_id = GUINT32_FROM_LE (ctrl_packet.server.node);
    port = GUINT32_FROM_LE (ctrl_packet.server.port);
    service = GUINT32_FROM_LE (ctrl_packet.server.service);
    version = GUINT32_FROM_LE (ctrl_packet.server.instance) & 0xff;
    instance = GUINT32_FROM_LE (ctrl_packet.server.instance) >> 8;

    if (type == QRTR_TYPE_NEW_SERVER) {
        g_debug ("[qrtr] added server on %u:%u -> service %u, version %u, instance %u",
                node_id, port, service, version, instance);
        add_service_info (self, node_id, port, service, version, instance);
    } else if (type == QRTR_TYPE_DEL_SERVER) {
        g_debug ("[qrtr] removed server on %u:%u -> service %u, version %u, instance %u",
                node_id, port, service, version, instance);
        remove_service_info (self, node_id, port, service, version, instance);
    } else
        g_assert_not_reached ();

    return TRUE;
}

/*****************************************************************************/

QrtrNode *
qrtr_control_socket_peek_node (QrtrControlSocket *socket,
                               guint32            node_id)
{
    return g_hash_table_lookup (socket->priv->node_map,
                                GUINT_TO_POINTER (node_id));
}

QrtrNode *
qrtr_control_socket_get_node (QrtrControlSocket *socket,
                              guint32            node_id)
{
    QrtrNode *node;

    node = qrtr_control_socket_peek_node (socket, node_id);
    return (node ? g_object_ref (node) : NULL);
}

/*****************************************************************************/

static gboolean
send_new_lookup_ctrl_packet (QrtrControlSocket  *self,
                             GError            **error)
{
    struct qrtr_ctrl_pkt ctl_packet;
    struct sockaddr_qrtr addr;
    int                  sockfd;
    socklen_t            len;
    int                  rc;

    sockfd = g_socket_get_fd (self->priv->socket);
    len = sizeof (addr);
    rc = getsockname (sockfd, (struct sockaddr *)&addr, &len);
    if (rc < 0) {
        g_set_error (error,
                     G_IO_ERROR,
                     g_io_error_from_errno (errno),
                     "Failed to get socket name");
        return FALSE;
    }

    g_debug ("[qrtr] socket lookup from %d:%d", addr.sq_node, addr.sq_port);

    g_assert (len == sizeof (addr) && addr.sq_family == AF_QIPCRTR);
    addr.sq_port = QRTR_PORT_CTRL;

    memset (&ctl_packet, 0, sizeof (ctl_packet));
    ctl_packet.cmd = GUINT32_TO_LE (QRTR_TYPE_NEW_LOOKUP);

    rc = sendto (sockfd, (void *)&ctl_packet, sizeof (ctl_packet),
                 0, (struct sockaddr *)&addr, sizeof (addr));
    if (rc < 0) {
        g_set_error (error,
                     G_IO_ERROR,
                     g_io_error_from_errno (errno),
                     "Failed to send lookup control packet");
        return FALSE;
    }

    return TRUE;
}

static void
setup_socket_source (QrtrControlSocket *self)
{
    self->priv->source = g_socket_create_source (self->priv->socket, G_IO_IN, NULL);
    g_source_set_callback (self->priv->source,
                           (GSourceFunc) qrtr_ctrl_message_cb,
                           self,
                           NULL);
    g_source_attach (self->priv->source, g_main_context_get_thread_default ());
}

static gboolean
initable_init (GInitable     *initable,
               GCancellable  *cancellable,
               GError       **error)
{
    QrtrControlSocket *self;
    gint               fd;

    self = QRTR_CONTROL_SOCKET (initable);

    fd = socket (AF_QIPCRTR, SOCK_DGRAM, 0);
    if (fd < 0) {
        g_set_error (error, G_IO_ERROR, g_io_error_from_errno (errno),
                     "Failed to create QRTR socket");
        return FALSE;
    }

    self->priv->socket = g_socket_new_from_fd (fd, error);
    if (!self->priv->socket) {
        close (fd);
        return FALSE;
    }

    g_socket_set_timeout (self->priv->socket, 0);

    if (!send_new_lookup_ctrl_packet (self, error)) {
        close (fd);
        return FALSE;
    }

    setup_socket_source (self);
    return TRUE;
}

/*****************************************************************************/

QrtrControlSocket *
qrtr_control_socket_new (GCancellable  *cancellable,
                         GError       **error)
{
    return QRTR_CONTROL_SOCKET (g_initable_new (QRTR_TYPE_CONTROL_SOCKET,
                                                cancellable,
                                                error,
                                                NULL));
}

static void
qrtr_control_socket_init (QrtrControlSocket *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
                                              QRTR_TYPE_CONTROL_SOCKET,
                                              QrtrControlSocketPrivate);

    self->priv->node_map = g_hash_table_new_full (g_direct_hash,
                                                  g_direct_equal,
                                                  NULL,
                                                  (GDestroyNotify)g_object_unref);
}

static void
dispose (GObject *object)
{
    QrtrControlSocket *self = QRTR_CONTROL_SOCKET (object);

    if (self->priv->source) {
        g_source_destroy (self->priv->source);
        g_source_unref (self->priv->source);
        self->priv->source = NULL;
    }

    if (self->priv->socket) {
        g_socket_close (self->priv->socket, NULL);
        g_clear_object (&self->priv->socket);
    }

    G_OBJECT_CLASS (qrtr_control_socket_parent_class)->dispose (object);
}

static void
finalize (GObject *object)
{
    QrtrControlSocket *self = QRTR_CONTROL_SOCKET (object);

    g_hash_table_unref (self->priv->node_map);

    G_OBJECT_CLASS (qrtr_control_socket_parent_class)->finalize (object);
}

static void
initable_iface_init (GInitableIface *iface)
{
    iface->init = initable_init;
}

static void
qrtr_control_socket_class_init (QrtrControlSocketClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (object_class, sizeof (QrtrControlSocketPrivate));

    object_class->dispose = dispose;
    object_class->finalize = finalize;

    /**
     * QrtrControlSocket::qrtr-node-added:
     * @self: the #QrtrControlSocket
     * @node: the node ID of the node that has been added
     *
     * The ::qrtr-node-added signal is emitted when a new node registers a service on
     * the QRTR bus.
     */
    signals[SIGNAL_NODE_ADDED] =
        g_signal_new (QRTR_CONTROL_SOCKET_SIGNAL_NODE_ADDED,
                      G_OBJECT_CLASS_TYPE (G_OBJECT_CLASS (klass)),
                      G_SIGNAL_RUN_LAST,
                      0,
                      NULL,
                      NULL,
                      NULL,
                      G_TYPE_NONE,
                      1,
                      G_TYPE_UINT);

    /**
     * QrtrControlSocket::qrtr-node-removed:
     * @self: the #QrtrControlSocket
     * @node: the node ID of the node that was removed
     *
     * The ::qrtr-node-removed signal is emitted when a node deregisters all services
     * from the QRTR bus.
     */
    signals[SIGNAL_NODE_REMOVED] =
        g_signal_new (QRTR_CONTROL_SOCKET_SIGNAL_NODE_REMOVED,
                      G_OBJECT_CLASS_TYPE (G_OBJECT_CLASS (klass)),
                      G_SIGNAL_RUN_LAST,
                      0,
                      NULL,
                      NULL,
                      NULL,
                      G_TYPE_NONE,
                      1,
                      G_TYPE_UINT);

    /**
     * QrtrControlSocket::qrtr-service-added:
     * @self: the #QrtrControlSocket
     * @node: the node ID where service is added
     * @service: the service ID of the service that has been added
     *
     * The ::qrtr-service-added signal is emitted when a new service registers
     * on the QRTR bus.
     */
    signals[SIGNAL_SERVICE_ADDED] =
        g_signal_new (QRTR_CONTROL_SOCKET_SIGNAL_SERVICE_ADDED,
                      G_OBJECT_CLASS_TYPE (G_OBJECT_CLASS (klass)),
                      G_SIGNAL_RUN_LAST,
                      0,
                      NULL,
                      NULL,
                      NULL,
                      G_TYPE_NONE,
                      2,
                      G_TYPE_UINT,
                      G_TYPE_UINT);

    /**
     * QrtrControlSocket::qrtr-service-removed:
     * @self: the #QrtrControlSocket
     * @node: the node ID where service is removed
     * @service: the service ID of the service that was removed
     *
     * The ::qrtr-service-removed signal is emitted when a service deregisters
     * from the QRTR bus.
     */
    signals[SIGNAL_SERVICE_REMOVED] =
        g_signal_new (QRTR_CONTROL_SOCKET_SIGNAL_SERVICE_REMOVED,
                      G_OBJECT_CLASS_TYPE (G_OBJECT_CLASS (klass)),
                      G_SIGNAL_RUN_LAST,
                      0,
                      NULL,
                      NULL,
                      NULL,
                      G_TYPE_NONE,
                      2,
                      G_TYPE_UINT,
                      G_TYPE_UINT);
}
