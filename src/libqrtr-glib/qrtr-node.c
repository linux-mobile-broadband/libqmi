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
#include <gmodule.h>

#include "qrtr-control-socket.h"
#include "qrtr-node.h"

G_DEFINE_TYPE (QrtrNode, qrtr_node, G_TYPE_OBJECT)

enum {
    PROP_0,
    PROP_SOCKET,
    PROP_NODE_ID,
    PROP_LAST
};

enum {
    SIGNAL_REMOVED,
    SIGNAL_LAST
};

static GParamSpec *properties[PROP_LAST];
static guint       signals   [SIGNAL_LAST] = { 0 };

struct _QrtrNodePrivate {
    QrtrControlSocket *socket;
    guint32            node_id;
    guint              node_removed_id;
    gboolean           removed;

    /* Holds QrtrServiceInfo entries */
    GList *service_list;
    /* Maps service numbers to a list of service entries */
    GHashTable *service_index;
    /* Maps port number to service entry (should only be one) */
    GHashTable *port_index;

    /* Array of QrtrServiceWaiters currently registered. */
    GPtrArray *waiters;
};

struct QrtrServiceInfo {
    guint32 service;
    guint32 port;
    guint32 version;
    guint32 instance;
};

typedef struct {
    GArray   *services;
    GTask    *task;
    GSource  *timeout_source;
} QrtrServiceWaiter;

/* used to avoid calling the free function when values are overwritten
 * in the service index */
struct ListHolder {
    GList *list;
};

/*****************************************************************************/

static void
node_removed_cb (QrtrNode *self,
                 guint     node_id)
{
    guint i;

    if (node_id != self->priv->node_id)
        return;

    self->priv->removed = TRUE;

    for (i = 0; i < self->priv->waiters->len; i++) {
        QrtrServiceWaiter *waiter;

        waiter = g_ptr_array_index (self->priv->waiters, i);
        g_task_return_new_error (waiter->task,
                                 G_IO_ERROR,
                                 G_IO_ERROR_CLOSED,
                                 "QRTR node was removed from the bus");
    }
    /* waiters was instantiated with qrtr_service_waiter_free as its free function,
     * so when we get rid of the contents, all of the constitutent waiter entries
     * (and their tasks) will be unreffed as well. */
    g_ptr_array_remove_range (self->priv->waiters, 0, self->priv->waiters->len);

    g_signal_emit (self, signals[SIGNAL_REMOVED], 0);
}

/*****************************************************************************/

static void
qrtr_service_waiter_free (QrtrServiceWaiter *waiter)
{
    g_array_unref (waiter->services);
    g_object_unref (waiter->task);
    if (waiter->timeout_source) {
        g_source_destroy (waiter->timeout_source);
        g_source_unref (waiter->timeout_source);
    }
    g_slice_free (QrtrServiceWaiter, waiter);
}

static gboolean
service_waiter_timeout_cb (QrtrServiceWaiter *waiter)
{
    QrtrNode *node;

    node = g_task_get_source_object (waiter->task);
    g_task_return_new_error (waiter->task,
                             G_IO_ERROR,
                             G_IO_ERROR_TIMED_OUT,
                             "QRTR services did not appear on the bus");
    /* This takes care of unreffing the task. */
    g_ptr_array_remove_fast (node->priv->waiters, waiter);

    return G_SOURCE_REMOVE;
}

static void
dispatch_pending_waiters (QrtrNode *node)
{
    guint i;

    for (i = 0; i < node->priv->waiters->len;) {
        QrtrServiceWaiter *waiter;
        guint j;
        gboolean should_dispatch = TRUE;

        waiter = g_ptr_array_index (node->priv->waiters, i);
        for (j = 0; j < waiter->services->len; j++) {
            guint32 service;

            service = g_array_index (waiter->services, guint32, j);
            if (!g_hash_table_lookup (node->priv->service_index, GUINT_TO_POINTER (service))) {
                should_dispatch = FALSE;
                break;
            }
        }

        if (should_dispatch) {
            g_task_return_boolean (waiter->task, TRUE);
            /* This takes care of unreffing the task. */
            g_ptr_array_remove_index_fast (node->priv->waiters, i);
        } else {
            i++;
        }
    }
}

/*****************************************************************************/

static void
service_info_free (struct QrtrServiceInfo *info)
{
    g_slice_free (struct QrtrServiceInfo, info);
}

static gint
sort_services_by_version (const struct QrtrServiceInfo *a,
                          const struct QrtrServiceInfo *b)
{
    return a->version - b->version;
}

static void
list_holder_free (struct ListHolder *list_holder)
{
    g_list_free (list_holder->list);
    g_slice_free (struct ListHolder, list_holder);
}

static void
service_index_add_info (GHashTable             *service_index,
                        guint32                 service,
                        struct QrtrServiceInfo *info)
{
    struct ListHolder *service_instances;

    service_instances = g_hash_table_lookup (service_index, GUINT_TO_POINTER (service));
    if (!service_instances) {
        service_instances = g_slice_new0 (struct ListHolder);
        g_hash_table_insert (service_index, GUINT_TO_POINTER (service), service_instances);
    }
    service_instances->list = g_list_insert_sorted (service_instances->list, info,
                                                    (GCompareFunc)sort_services_by_version);
}

static void
service_index_remove_info (GHashTable             *service_index,
                           guint32                 service,
                           struct QrtrServiceInfo *info)
{
    struct ListHolder *service_instances;

    service_instances = g_hash_table_lookup (service_index, GUINT_TO_POINTER (service));
    if (!service_instances)
        return;

    service_instances->list = g_list_remove (service_instances->list, info);
}

void
qrtr_node_add_service_info (QrtrNode *node,
                            guint32 service,
                            guint32 port,
                            guint32 version,
                            guint32 instance)
{
    struct QrtrServiceInfo *info;

    info = g_slice_new (struct QrtrServiceInfo);
    info->service = service;
    info->port = port;
    info->version = version;
    info->instance = instance;
    node->priv->service_list = g_list_append (node->priv->service_list, info);
    service_index_add_info (node->priv->service_index, service, info);
    g_hash_table_insert (node->priv->port_index, GUINT_TO_POINTER (port), info);
    dispatch_pending_waiters (node);
}

void
qrtr_node_remove_service_info (QrtrNode *node,
                               guint32 service,
                               guint32 port,
                               guint32 version,
                               guint32 instance)
{
    struct QrtrServiceInfo *info;

    info = g_hash_table_lookup (node->priv->port_index, GUINT_TO_POINTER (port));
    if (!info) {
        g_info ("[qrtr node@%u]: tried to remove unknown service %u, port %u",
                node->priv->node_id, service, port);
        return;
    }

    service_index_remove_info (node->priv->service_index, service, info);
    g_hash_table_remove (node->priv->port_index, GUINT_TO_POINTER (port));
    node->priv->service_list = g_list_remove (node->priv->service_list, info);
    service_info_free (info);
}

/*****************************************************************************/

gint32
qrtr_node_lookup_port (QrtrNode *node,
                       guint32   service)
{
    struct ListHolder *service_instances;
    struct QrtrServiceInfo *info;
    GList *list;

    service_instances = g_hash_table_lookup (node->priv->service_index,
                                             GUINT_TO_POINTER (service));
    if (!service_instances)
        return -1;

    list = g_list_last (service_instances->list);
    if (!list)
        return -1;

    info = list->data;
    return (info) ? (gint32)info->port : -1;
}

gint32
qrtr_node_lookup_service (QrtrNode *node,
                          guint32   port)
{
    struct QrtrServiceInfo *info;

    info = g_hash_table_lookup (node->priv->port_index, GUINT_TO_POINTER (port));
    return info ? (gint32)info->service : -1;
}

/*****************************************************************************/

gboolean
qrtr_node_has_services (QrtrNode *node)
{
    return node->priv->service_list != NULL;
}

guint32
qrtr_node_id (QrtrNode *node)
{
    return node->priv->node_id;
}

/*****************************************************************************/

gboolean
qrtr_node_wait_for_services_finish (QrtrNode      *self,
                                    GAsyncResult  *result,
                                    GError       **error)
{
    return g_task_propagate_boolean (G_TASK (result), error);
}

void
qrtr_node_wait_for_services (QrtrNode            *self,
                             GArray              *services,
                             guint                timeout,
                             GCancellable        *cancellable,
                             GAsyncReadyCallback  callback,
                             gpointer             user_data)
{
    GTask *task;
    QrtrServiceWaiter *waiter;
    guint i;
    gboolean services_present = TRUE;

    task = g_task_new (self, cancellable, callback, user_data);

    if (self->priv->removed) {
        g_task_return_new_error (task,
                                 G_IO_ERROR,
                                 G_IO_ERROR_CLOSED,
                                 "QRTR node was removed from the bus");
        g_object_unref (task);
        return;
    }

    for (i = 0; i < services->len; i++) {
        guint32 service;

        service = g_array_index (services, guint32, i);
        if (!g_hash_table_lookup (self->priv->service_index, GUINT_TO_POINTER (service))) {
            services_present = FALSE;
            break;
        }
    }

    if (services_present) {
        g_task_return_boolean (task, TRUE);
        g_object_unref (task);
        return;
    }

    waiter = g_slice_new0 (QrtrServiceWaiter);
    waiter->services = g_array_ref (services);
    waiter->task = task;
    if (timeout > 0) {
        waiter->timeout_source = g_timeout_source_new_seconds (timeout);
        g_source_set_callback (waiter->timeout_source, (GSourceFunc)service_waiter_timeout_cb,
                               waiter, NULL);
        g_source_attach (waiter->timeout_source, g_main_context_get_thread_default ());
    }

    g_ptr_array_add (self->priv->waiters, waiter);
}

QrtrNode *
qrtr_node_new (QrtrControlSocket *socket,
               guint32            node_id)
{
    return QRTR_NODE (g_object_new (QRTR_TYPE_NODE,
                                    QRTR_NODE_SOCKET, socket,
                                    QRTR_NODE_ID,     node_id,
                                    NULL));
}

static void
qrtr_node_init (QrtrNode *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, QRTR_TYPE_NODE, QrtrNodePrivate);

    self->priv->removed = FALSE;
    self->priv->service_index = g_hash_table_new_full (g_direct_hash, g_direct_equal,
                                                       NULL, (GDestroyNotify)list_holder_free);
    self->priv->port_index = g_hash_table_new (g_direct_hash, g_direct_equal);
    self->priv->waiters = g_ptr_array_new_with_free_func ((GDestroyNotify)qrtr_service_waiter_free);
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    QrtrNode *self = QRTR_NODE (object);

    switch (prop_id) {
    case PROP_SOCKET:
        g_assert (!self->priv->socket);
        self->priv->socket = g_value_dup_object (value);
        self->priv->node_removed_id = g_signal_connect_swapped (self->priv->socket,
                                                                QRTR_CONTROL_SOCKET_SIGNAL_NODE_REMOVED,
                                                                G_CALLBACK (node_removed_cb),
                                                                self);
        break;
    case PROP_NODE_ID:
        self->priv->node_id = (guint32) g_value_get_uint (value);
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
    QrtrNode *self = QRTR_NODE (object);

    switch (prop_id) {
    case PROP_SOCKET:
        g_value_set_object (value, self->priv->socket);
        break;
    case PROP_NODE_ID:
        g_value_set_uint (value, (guint)self->priv->node_id);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
dispose (GObject *object)
{
    QrtrNode *self = QRTR_NODE (object);

    if (self->priv->node_removed_id) {
        g_signal_handler_disconnect (self->priv->socket, self->priv->node_removed_id);
        self->priv->node_removed_id = 0;
    }
    g_clear_object (&self->priv->socket);

    /* We shouldn't have any waiters because they should have been removed when the
     * node was removed from the bus, and they hold references to self. */
    g_assert (self->priv->waiters->len == 0);
    g_clear_pointer (&self->priv->waiters, (GDestroyNotify)g_ptr_array_unref);

    G_OBJECT_CLASS (qrtr_node_parent_class)->dispose (object);
}

static void
finalize (GObject *object)
{
    QrtrNode *self = QRTR_NODE (object);

    g_hash_table_unref (self->priv->service_index);
    g_hash_table_unref (self->priv->port_index);
    g_list_free_full (self->priv->service_list, (GDestroyNotify)service_info_free);

    G_OBJECT_CLASS (qrtr_node_parent_class)->finalize (object);
}

static void
qrtr_node_class_init (QrtrNodeClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (object_class, sizeof (QrtrNodePrivate));

    object_class->get_property = get_property;
    object_class->set_property = set_property;
    object_class->dispose      = dispose;
    object_class->finalize     = finalize;

    /**
     * QrtrNode:socket:
     */
    properties[PROP_SOCKET] =
        g_param_spec_object (QRTR_NODE_SOCKET,
                             "Socket",
                             "Control socket",
                             QRTR_TYPE_CONTROL_SOCKET,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    g_object_class_install_property (object_class, PROP_SOCKET, properties[PROP_SOCKET]);

    /**
     * QrtrNode:node-id:
     */
    properties[PROP_NODE_ID] =
        g_param_spec_uint (QRTR_NODE_ID,
                           "Node ID",
                           "ID of the QRTR node",
                           0,
                           G_MAXUINT32,
                           0,
                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    g_object_class_install_property (object_class, PROP_NODE_ID, properties[PROP_NODE_ID]);

    /**
     * QrtrNode::removed:
     * @self: the #QrtrNode
     */
    signals[SIGNAL_REMOVED] =
        g_signal_new (QRTR_NODE_SIGNAL_REMOVED,
                      G_OBJECT_CLASS_TYPE (G_OBJECT_CLASS (klass)),
                      G_SIGNAL_RUN_LAST,
                      0,
                      NULL,
                      NULL,
                      NULL,
                      G_TYPE_NONE,
                      0);
}
