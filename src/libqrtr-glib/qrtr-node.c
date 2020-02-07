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

    /* Holds QrtrServiceInfo entries */
    GList *service_list;
    /* Maps service numbers to a list of service entries */
    GHashTable *service_index;
    /* Maps port number to service entry (should only be one) */
    GHashTable *port_index;
};

struct QrtrServiceInfo {
    guint32 service;
    guint32 port;
    guint32 version;
    guint32 instance;
};

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
    if (node_id != self->priv->node_id)
        return;

    g_signal_emit (self, signals[SIGNAL_REMOVED], 0);
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
__qrtr_node_add_service_info (QrtrNode *node,
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
}

void
__qrtr_node_remove_service_info (QrtrNode *node,
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

    service_instances = g_hash_table_lookup (node->priv->service_index,
                                             GUINT_TO_POINTER (service));
    if (!service_instances)
        return -1;

    info = g_list_last (service_instances->list)->data;
    return info->port;
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

    self->priv->service_index = g_hash_table_new_full (g_direct_hash, g_direct_equal,
                                                       NULL, (GDestroyNotify)list_holder_free);
    self->priv->port_index = g_hash_table_new (g_direct_hash, g_direct_equal);
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
