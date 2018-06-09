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
 * Copyright (C) 2018 Aleksander Morgado <aleksander@aleksander.es>
 */

#include <endian.h>
#include <errno.h>
#include <linux/qrtr.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <gio/gio.h>

#include "qmi-qrtr-control-socket.h"
#include "qmi-qrtr-node.h"
#include "qmi-enums.h"

G_DEFINE_TYPE (QrtrControlSocket, qrtr_control_socket, G_TYPE_OBJECT)

struct _QrtrControlSocketPrivate {
    /* Underlying QRTR socket */
    GSocket *socket;
    /* Map of node id -> NodeEntry */
    GHashTable *node_map;
    /* Callback source for when NEW_SERVER/DEL_SERVER control packets come in */
    GSource *source;
};

struct NodeEntry {
    QrtrNode *node;
    gboolean published;
    guint publish_source_id;
};

enum {
    SIGNAL_NODE_ADDED,
    SIGNAL_NODE_REMOVED,
    SIGNAL_LAST
};

static guint signals[SIGNAL_LAST] = { 0 };

#define PUBLISH_TIMEOUT_MS 100

/*****************************************************************************/

struct PublishRequest {
    QrtrControlSocket *socket;
    guint32 node_id;
};

static void
node_entry_free (struct NodeEntry *entry)
{
    g_clear_object (&entry->node);
    if (entry->publish_source_id)
        g_source_remove (entry->publish_source_id);
    g_slice_free (struct NodeEntry, entry);
}

static gboolean
node_entry_publish (struct PublishRequest *request)
{
    struct NodeEntry *entry;

    /* Check to make sure the node is actually still around and unpublished. */
    entry = g_hash_table_lookup (request->socket->priv->node_map,
                                 GUINT_TO_POINTER (request->node_id));
    if (!entry || entry->published)
        return FALSE;

    entry->published = TRUE;
    g_signal_emit (request->socket, signals[SIGNAL_NODE_ADDED], 0, request->node_id);
    return FALSE;
}

static void
publish_async (QrtrControlSocket *socket,
               struct NodeEntry *entry)
{
    struct PublishRequest *request;

    if (entry->published)
        return;

    if (entry->publish_source_id)
        g_source_remove (entry->publish_source_id);

    request = g_new0 (struct PublishRequest, 1);
    request->socket = socket;
    request->node_id = qrtr_node_id (entry->node);

    entry->publish_source_id = g_timeout_add_full (G_PRIORITY_DEFAULT,
                                                   PUBLISH_TIMEOUT_MS,
                                                   (GSourceFunc)node_entry_publish,
                                                   request,
                                                   (GDestroyNotify)g_free);
}

/*****************************************************************************/

static void
add_service_info (QrtrControlSocket *socket, guint32 node_id, guint32 port,
                  QmiService service, guint32 version, guint32 instance)
{
    struct NodeEntry *entry;

    entry = g_hash_table_lookup (socket->priv->node_map, GUINT_TO_POINTER (node_id));
    if (!entry) {
        entry = g_slice_new0 (struct NodeEntry);
        entry->node = qrtr_node_new (socket, node_id);
        entry->published = FALSE;

        g_assert (g_hash_table_insert (socket->priv->node_map, GUINT_TO_POINTER (node_id), entry));
        g_info ("qrtr: Created new node %u", node_id);
    }
    if (!entry->published) {
        /* Schedule or reschedule the publish callback since we might continue
         * to see more services for this node for a bit. */
        publish_async (socket, entry);
    }

    qrtr_node_add_service_info (entry->node, service, port, version, instance);
}

static void
remove_service_info (QrtrControlSocket *socket, guint32 node_id, guint32 port,
                     QmiService service, guint32 version, guint32 instance)
{
    struct NodeEntry* entry = g_hash_table_lookup (socket->priv->node_map, GUINT_TO_POINTER (node_id));
    if (!entry) {
        g_warning ("qrtr: Got DEL_SERVER for nonexistent node %u", node_id);
        return;
    }

    qrtr_node_remove_service_info (entry->node, service, port, version, instance);
    if (!qrtr_node_has_services (entry->node)) {
        g_info ("qrtr: Removing node %u", node_id);
        /* If we haven't announced that this node is available yet, don't bother
         * announcing that we've removed it. */
        if (entry->published) {
            entry->published = FALSE;
            g_signal_emit (socket, signals[SIGNAL_NODE_REMOVED], 0, node_id);
        }
        g_hash_table_remove (socket->priv->node_map, GUINT_TO_POINTER (node_id));
    }
}

/*****************************************************************************/

static gboolean
send_new_lookup_ctrl_packet (GSocket *gsocket, GError **error)
{
    struct qrtr_ctrl_pkt ctl_packet;
    struct sockaddr_qrtr addr;
    int sockfd;
    socklen_t len;
    int rc;

    sockfd = g_socket_get_fd (gsocket);
    len = sizeof (addr);
    rc = getsockname (sockfd, (struct sockaddr *)&addr, &len);
    if (rc < 0) {
        g_set_error (error,
                     G_IO_ERROR,
                     g_io_error_from_errno (errno),
                     "Failed to get socket name");
        return FALSE;
    }

    g_info ("qrtr: socket lookup from %d:%d", addr.sq_node, addr.sq_port);

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

static gboolean
qrtr_ctrl_message_cb (GSocket *gsocket,
                      GIOCondition cond,
                      QrtrControlSocket *socket)
{
    /* check for message type and add/remove nodes here */
    GError *error = NULL;
    struct qrtr_ctrl_pkt ctrl_packet;
    gssize bytes_received;
    guint32 type;
    guint32 node_id;
    guint32 port;
    QmiService service;
    guint32 version;
    guint32 instance;

    bytes_received = g_socket_receive (gsocket, (gchar *)&ctrl_packet,
                                       sizeof (ctrl_packet), NULL, &error);
    if (bytes_received < 0) {
        g_warning ("Socket IO failure: %s", error->message);
        return FALSE;
    }

    if (bytes_received < sizeof (ctrl_packet)) {
        g_warning ("Got short QRTR datagram");
        return TRUE;
    }

    type = GUINT32_FROM_LE (ctrl_packet.cmd);
    if (type != QRTR_TYPE_NEW_SERVER && type != QRTR_TYPE_DEL_SERVER) {
        g_info ("Got packet of unused type %u", type);
        return TRUE;
    }

    /* type is something we handle, parse the packet */
    node_id = GUINT32_FROM_LE (ctrl_packet.server.node);
    port = GUINT32_FROM_LE (ctrl_packet.server.port);
    service = (QmiService)GUINT32_FROM_LE (ctrl_packet.server.service);
    version = GUINT32_FROM_LE (ctrl_packet.server.instance) & 0xff;
    instance = GUINT32_FROM_LE (ctrl_packet.server.instance) >> 8;

    if (type == QRTR_TYPE_NEW_SERVER) {
        g_info ("NEW_SERVER on %u:%u -> service %u, version %u, instance %u",
                node_id, port, service, version, instance);
        add_service_info (socket, node_id, port, service, version, instance);
    } else /* type is DEL_SERVER */ {
        g_info ("DEL_SERVER on %u:%u -> service %u, version %u, instance %u",
                node_id, port, service, version, instance);
        remove_service_info (socket, node_id, port, service, version, instance);
    }

    return TRUE;
}

static void
setup_socket_source (QrtrControlSocket *socket)
{
    GSocket *gsocket;
    GSource *source;

    gsocket = socket->priv->socket;
    source = g_socket_create_source (gsocket, G_IO_IN, NULL);
    g_source_set_callback (source, (GSourceFunc) qrtr_ctrl_message_cb,
                           socket, NULL);
    g_source_attach (source, NULL);

    socket->priv->source = source;
}

/*****************************************************************************/

QrtrNode *
qrtr_control_socket_get_node (QrtrControlSocket *socket, guint32 node_id)
{
    struct NodeEntry *entry;

    entry = g_hash_table_lookup (socket->priv->node_map,
                                 GUINT_TO_POINTER (node_id));
    /* Don't return unpublished nodes. They are still receiving server packets
     * and are thus incompletely specified for the time being, and the caller
     * probably has a stale node ID anyway. */
    return (entry && entry->published) ? entry->node : NULL;
}

/*****************************************************************************/

QrtrControlSocket *
qrtr_control_socket_new (GError **error)
{
    GError *local_error = NULL;
    GSocket *gsocket = NULL;
    QrtrControlSocket *qsocket;
    int socket_fd;

    socket_fd = socket (AF_QIPCRTR, SOCK_DGRAM, 0);
    if (socket_fd < 0) {
        g_set_error (error,
                     G_IO_ERROR,
                     g_io_error_from_errno (errno),
                     "Failed to create socket");
        return NULL;
    }

    gsocket = g_socket_new_from_fd (socket_fd, &local_error);
    if (!gsocket) {
        g_propagate_error (error, local_error);
        close (socket_fd);
        return NULL;
    }

    g_socket_set_timeout (gsocket, 0);

    if (!send_new_lookup_ctrl_packet (gsocket, &local_error)) {
        g_propagate_error (error, local_error);
        g_socket_close (gsocket, NULL);
        g_clear_object (&gsocket);
        return NULL;
    }

    qsocket = g_object_new (QRTR_TYPE_CONTROL_SOCKET, NULL);
    qsocket->priv->socket = gsocket;
    qsocket->priv->node_map = g_hash_table_new_full (g_direct_hash, g_direct_equal,
                                                     NULL, (GDestroyNotify)node_entry_free);

    setup_socket_source (qsocket);
    return qsocket;
}

static void
qrtr_control_socket_init (QrtrControlSocket *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
                                              QRTR_TYPE_CONTROL_SOCKET,
                                              QrtrControlSocketPrivate);
}

static void
dispose (GObject *object)
{
    QrtrControlSocket *self = QRTR_CONTROL_SOCKET (object);

    g_hash_table_unref (self->priv->node_map);

    if (self->priv->source) {
        g_source_destroy (self->priv->source);
        g_clear_object (&self->priv->source);
    }

    g_socket_close (self->priv->socket, NULL);
    g_clear_object (&self->priv->socket);

    G_OBJECT_CLASS (qrtr_control_socket_parent_class)->dispose (object);
}

static void
qrtr_control_socket_class_init (QrtrControlSocketClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (object_class, sizeof (QrtrControlSocketPrivate));

    object_class->dispose = dispose;

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
}
