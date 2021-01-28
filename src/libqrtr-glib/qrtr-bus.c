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

#include <endian.h>
#include <errno.h>
#include <linux/qrtr.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <gio/gio.h>

#include "qrtr-bus.h"
#include "qrtr-node.h"
#include "qrtr-utils.h"

static void async_initable_iface_init (GAsyncInitableIface *iface);

G_DEFINE_TYPE_EXTENDED (QrtrBus, qrtr_bus, G_TYPE_OBJECT, 0,
                        G_IMPLEMENT_INTERFACE (G_TYPE_ASYNC_INITABLE, async_initable_iface_init))

enum {
    PROP_0,
    PROP_LOOKUP_TIMEOUT,
    PROP_PORT,
    PROP_LAST
};

enum {
    SIGNAL_NODE_ADDED,
    SIGNAL_NODE_REMOVED,
    SIGNAL_SERVICE_ADDED,
    SIGNAL_SERVICE_REMOVED,
    SIGNAL_LAST
};

static GParamSpec *properties[PROP_LAST];
static guint       signals[SIGNAL_LAST] = { 0 };

struct _QrtrBusPrivate {
    /* Underlying QRTR socket */
    GSocket *socket;

    /* Map of node id -> QrtrNode. This hash table contains full references to
     * the available QrtrNodes; i.e. the nodes are owned by the bus
     * unconditionally. */
    GHashTable *node_map;

    /* Callback source for when NEW_SERVER/DEL_SERVER control packets come in */
    GSource *source;

    /* initial lookup support */
    guint    lookup_timeout;
    GTask   *init_task;
    GSource *init_timeout_source;
};

/*****************************************************************************/

static void
add_service_info (QrtrBus *self,
                  guint32  node_id,
                  guint32  port,
                  guint32  service,
                  guint32  version,
                  guint32  instance)
{
    QrtrNode *node;

    node = g_hash_table_lookup (self->priv->node_map, GUINT_TO_POINTER (node_id));
    if (!node) {
        /* Node objects are exclusively created at this point */
        node = QRTR_NODE (g_object_new (QRTR_TYPE_NODE,
                                        QRTR_NODE_BUS, self,
                                        QRTR_NODE_ID,  node_id,
                                        NULL));
        g_assert (g_hash_table_insert (self->priv->node_map, GUINT_TO_POINTER (node_id), node));
        g_debug ("[qrtr] created new node %u", node_id);
        g_signal_emit (self, signals[SIGNAL_NODE_ADDED], 0, node_id);
    }

    qrtr_node_add_service_info (node, service, port, version, instance);
    g_signal_emit (self, signals[SIGNAL_SERVICE_ADDED], 0, node_id, service);
}

static void
remove_service_info (QrtrBus *self,
                     guint32  node_id,
                     guint32  port,
                     guint32  service,
                     guint32  version,
                     guint32  instance)
{
    QrtrNode *node;

    node = g_hash_table_lookup (self->priv->node_map, GUINT_TO_POINTER (node_id));
    if (!node) {
        g_warning ("[qrtr] cannot remove service info: nonexistent node %u", node_id);
        return;
    }

    qrtr_node_remove_service_info (node, service, port, version, instance);
    g_signal_emit (self, signals[SIGNAL_SERVICE_REMOVED], 0, node_id, service);
    if (!qrtr_node_peek_service_info_list (node)) {
        g_debug ("[qrtr] removing node %u", node_id);
        g_signal_emit (self, signals[SIGNAL_NODE_REMOVED], 0, node_id);
        g_hash_table_remove (self->priv->node_map, GUINT_TO_POINTER (node_id));
    }
}

/*****************************************************************************/

static void initable_complete (QrtrBus *self);

static gboolean
qrtr_ctrl_message_cb (GSocket      *gsocket,
                      GIOCondition  cond,
                      QrtrBus      *self)
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

    if (type == QRTR_TYPE_DEL_SERVER) {
        g_debug ("[qrtr] removed server on %u:%u -> service %u, version %u, instance %u",
                 node_id, port, service, version, instance);
        remove_service_info (self, node_id, port, service, version, instance);
        return TRUE;
    }

    g_assert (type == QRTR_TYPE_NEW_SERVER);

    if (!node_id && !port && !service && !version && !instance) {
        g_debug ("[qrtr] initial lookup finished");
        initable_complete (self);
        return TRUE;
    }

    g_debug ("[qrtr] added server on %u:%u -> service %u, version %u, instance %u",
             node_id, port, service, version, instance);
    add_service_info (self, node_id, port, service, version, instance);
    return TRUE;
}

/*****************************************************************************/

QrtrNode *
qrtr_bus_peek_node (QrtrBus *self,
                    guint32  node_id)
{
    g_return_val_if_fail (QRTR_IS_BUS (self), NULL);

    return g_hash_table_lookup (self->priv->node_map, GUINT_TO_POINTER (node_id));
}

QrtrNode *
qrtr_bus_get_node (QrtrBus *self,
                   guint32  node_id)
{
    QrtrNode *node;

    g_return_val_if_fail (QRTR_IS_BUS (self), NULL);

    node = qrtr_bus_peek_node (self, node_id);
    return (node ? g_object_ref (node) : NULL);
}

/*****************************************************************************/

typedef struct {
    guint32  node_id;
    guint    added_id;
    GSource *timeout_source;
} WaitForNodeContext;

static void
wait_for_node_context_cleanup (QrtrBus            *self,
                               WaitForNodeContext *ctx)
{
    if (ctx->timeout_source) {
        g_source_destroy (ctx->timeout_source);
        g_source_unref (ctx->timeout_source);
        ctx->timeout_source = NULL;
    }

    if (ctx->added_id) {
        g_signal_handler_disconnect (self, ctx->added_id);
        ctx->added_id = 0;
    }
}

static void
wait_for_node_context_free (WaitForNodeContext *ctx)
{
    g_assert (!ctx->added_id);
    g_assert (!ctx->timeout_source);
    g_slice_free (WaitForNodeContext, ctx);
}

QrtrNode *
qrtr_bus_wait_for_node_finish (QrtrBus       *self,
                               GAsyncResult  *res,
                               GError       **error)
{
    return g_task_propagate_pointer (G_TASK (res), error);
}

static gboolean
wait_for_node_timeout_cb (GTask *task)
{
    QrtrBus            *self;
    WaitForNodeContext *ctx;

    self = g_task_get_source_object (task);
    ctx  = g_task_get_task_data (task);

    /* cleanup the context right away, so that we take exclusive ownership
     * of the task */
    wait_for_node_context_cleanup (self, ctx);

    g_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_TIMED_OUT,
                             "QRTR node %u did not appear on the bus", ctx->node_id);
    g_object_unref (task);

    return G_SOURCE_REMOVE;
}

static void
wait_for_node_added_cb (QrtrBus *self,
                        guint    node_id,
                        GTask   *task)
{
    WaitForNodeContext *ctx;
    QrtrNode           *node;

    ctx = g_task_get_task_data (task);

    /* not the one we want, ignore */
    if (node_id != ctx->node_id)
        return;

    /* cleanup the context right away, so that we take exclusive ownership
     * of the task */
    wait_for_node_context_cleanup (self, ctx);

    /* get a full node reference */
    node = qrtr_bus_get_node (self, node_id);
    g_task_return_pointer (task, node, g_object_unref);
    g_object_unref (task);
}

void
qrtr_bus_wait_for_node (QrtrBus             *self,
                        guint32              node_id,
                        guint                timeout_ms,
                        GCancellable        *cancellable,
                        GAsyncReadyCallback  callback,
                        gpointer             user_data)
{
    GTask              *task;
    WaitForNodeContext *ctx;
    QrtrNode           *existing_node;

    g_return_if_fail (QRTR_IS_BUS (self));
    g_return_if_fail (timeout_ms > 0);

    task = g_task_new (self, cancellable, callback, user_data);

    /* Nothing to do if it already exists */
    existing_node = qrtr_bus_get_node (self, node_id);
    if (existing_node) {
        g_task_return_pointer (task, existing_node, (GDestroyNotify)g_object_unref);
        g_object_unref (task);
        return;
    }

    /* The ownership of the task is shared between the signal handler and the timeout;
     * we need to make sure that we cancel the other one if we're completing the task
     * from one of them. */
    ctx = g_slice_new0 (WaitForNodeContext);
    ctx->node_id = node_id;

    /* Monitor added nodes */
    ctx->added_id = g_signal_connect (self,
                                      QRTR_BUS_SIGNAL_NODE_ADDED,
                                      G_CALLBACK (wait_for_node_added_cb),
                                      task);

    /* Setup timeout for the operation */
    ctx->timeout_source = g_timeout_source_new (timeout_ms);
    g_source_set_callback (ctx->timeout_source, (GSourceFunc)wait_for_node_timeout_cb, task, NULL);
    g_source_attach (ctx->timeout_source, g_main_context_get_thread_default ());

    g_task_set_task_data (task, ctx, (GDestroyNotify)wait_for_node_context_free);
}

/*****************************************************************************/

static gboolean
send_new_lookup_ctrl_packet (QrtrBus  *self,
                             GError  **error)
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
setup_socket_source (QrtrBus *self)
{
    self->priv->source = g_socket_create_source (self->priv->socket, G_IO_IN, NULL);
    g_source_set_callback (self->priv->source,
                           (GSourceFunc) qrtr_ctrl_message_cb,
                           self,
                           NULL);
    g_source_attach (self->priv->source, g_main_context_get_thread_default ());
}

static gboolean
common_init (QrtrBus  *self,
             GError  **error)
{
    gint fd;

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

typedef struct {

} InitContext;

static gboolean
initable_init_finish (GAsyncInitable  *initable,
                      GAsyncResult    *result,
                      GError         **error)
{
    return g_task_propagate_boolean (G_TASK (result), error);
}

static gboolean
initable_timeout (QrtrBus *self)
{
    g_autoptr(GTask) task = NULL;

    task = g_steal_pointer (&self->priv->init_task);
    g_assert (task);

    g_clear_pointer (&self->priv->init_timeout_source, g_source_unref);

    g_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_TIMED_OUT,
                             "Timed out waiting for the initial bus lookup");
    return G_SOURCE_REMOVE;
}

static void
initable_complete (QrtrBus *self)
{
    g_autoptr(GTask) task = NULL;

    task = g_steal_pointer (&self->priv->init_task);
    if (!task) {
        g_assert (!self->priv->init_timeout_source);
        return;
    }

    g_assert (self->priv->init_timeout_source);
    g_source_destroy (self->priv->init_timeout_source);
    g_clear_pointer (&self->priv->init_timeout_source, g_source_unref);

    g_task_return_boolean (task, TRUE);
}

static void
initable_init_async (GAsyncInitable      *initable,
                     int                  io_priority,
                     GCancellable        *cancellable,
                     GAsyncReadyCallback  callback,
                     gpointer             user_data)
{
    QrtrBus          *self = QRTR_BUS (initable);
    GError           *error = NULL;
    g_autoptr(GTask)  task = NULL;

    task = g_task_new (initable, cancellable, callback, user_data);

    if (!common_init (self, &error)) {
        g_task_return_error (task, error);
        return;
    }

    /* if lookup timeout is disabled, we're done */
    if (!self->priv->lookup_timeout) {
        g_task_return_boolean (task, TRUE);
        return;
    }

    /* setup wait for the initial lookup completion */
    self->priv->init_task = g_steal_pointer (&task);
    self->priv->init_timeout_source = g_timeout_source_new (self->priv->lookup_timeout);
    g_source_set_callback (self->priv->init_timeout_source, (GSourceFunc)initable_timeout, self, NULL);
    g_source_attach (self->priv->init_timeout_source, g_main_context_get_thread_default ());
}

/*****************************************************************************/

QrtrBus *
qrtr_bus_new_finish (GAsyncResult  *res,
                     GError       **error)
{
    g_autoptr(GObject) source_object = NULL;

    source_object = g_async_result_get_source_object (res);
    return QRTR_BUS (g_async_initable_new_finish (G_ASYNC_INITABLE (source_object), res, error));
}

void
qrtr_bus_new (guint                lookup_timeout_ms,
              GCancellable        *cancellable,
              GAsyncReadyCallback  callback,
              gpointer             user_data)
{
    g_async_initable_new_async (QRTR_TYPE_BUS,
                                G_PRIORITY_DEFAULT,
                                cancellable,
                                callback,
                                user_data,
                                QRTR_BUS_LOOKUP_TIMEOUT, lookup_timeout_ms,
                                NULL);
}

/*****************************************************************************/

static void
qrtr_bus_init (QrtrBus *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
                                              QRTR_TYPE_BUS,
                                              QrtrBusPrivate);

    self->priv->node_map = g_hash_table_new_full (g_direct_hash,
                                                  g_direct_equal,
                                                  NULL,
                                                  (GDestroyNotify)g_object_unref);
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    QrtrBus *self = QRTR_BUS (object);

    switch (prop_id) {
    case PROP_LOOKUP_TIMEOUT:
        self->priv->lookup_timeout = g_value_get_uint (value);
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
    QrtrBus *self = QRTR_BUS (object);

    switch (prop_id) {
    case PROP_LOOKUP_TIMEOUT:
        g_value_set_uint (value, self->priv->lookup_timeout);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
dispose (GObject *object)
{
    QrtrBus *self = QRTR_BUS (object);

    g_assert (!self->priv->init_task);
    g_assert (!self->priv->init_timeout_source);

    if (self->priv->source) {
        g_source_destroy (self->priv->source);
        g_source_unref (self->priv->source);
        self->priv->source = NULL;
    }

    if (self->priv->socket) {
        g_socket_close (self->priv->socket, NULL);
        g_clear_object (&self->priv->socket);
    }

    g_clear_pointer (&self->priv->node_map, g_hash_table_unref);

    G_OBJECT_CLASS (qrtr_bus_parent_class)->dispose (object);
}

static void
async_initable_iface_init (GAsyncInitableIface *iface)
{
    iface->init_async  = initable_init_async;
    iface->init_finish = initable_init_finish;
}

static void
qrtr_bus_class_init (QrtrBusClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (object_class, sizeof (QrtrBusPrivate));

    object_class->get_property = get_property;
    object_class->set_property = set_property;
    object_class->dispose = dispose;

    /**
     * QrtrBus:lookup-timeout:
     *
     * Since: 1.28
     */
    properties[PROP_LOOKUP_TIMEOUT] =
        g_param_spec_uint (QRTR_BUS_LOOKUP_TIMEOUT,
                           "lookup timeout",
                           "Timeout in ms to wait for the initial lookup to finish, or 0 to disable it",
                           0,
                           G_MAXUINT,
                           0,
                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    g_object_class_install_property (object_class, PROP_LOOKUP_TIMEOUT, properties[PROP_LOOKUP_TIMEOUT]);

    /**
     * QrtrBus::node-added:
     * @self: the #QrtrBus
     * @node: the node ID of the node that has been added
     *
     * The ::node-added signal is emitted when a new node registers a service on
     * the QRTR bus.
     *
     * Since: 1.28
     */
    signals[SIGNAL_NODE_ADDED] =
        g_signal_new (QRTR_BUS_SIGNAL_NODE_ADDED,
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
     * QrtrBus::node-removed:
     * @self: the #QrtrBus
     * @node: the node ID of the node that was removed
     *
     * The ::node-removed signal is emitted when a node deregisters all services
     * from the QRTR bus.
     *
     * Since: 1.28
     */
    signals[SIGNAL_NODE_REMOVED] =
        g_signal_new (QRTR_BUS_SIGNAL_NODE_REMOVED,
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
     * QrtrBus::service-added:
     * @self: the #QrtrBus
     * @node: the node ID where service is added
     * @service: the service ID of the service that has been added
     *
     * The ::service-added signal is emitted when a new service registers
     * on the QRTR bus.
     *
     * Since: 1.28
     */
    signals[SIGNAL_SERVICE_ADDED] =
        g_signal_new (QRTR_BUS_SIGNAL_SERVICE_ADDED,
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
     * QrtrBus::service-removed:
     * @self: the #QrtrBus
     * @node: the node ID where service is removed
     * @service: the service ID of the service that was removed
     *
     * The ::service-removed signal is emitted when a service deregisters
     * from the QRTR bus.
     *
     * Since: 1.28
     */
    signals[SIGNAL_SERVICE_REMOVED] =
        g_signal_new (QRTR_BUS_SIGNAL_SERVICE_REMOVED,
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
