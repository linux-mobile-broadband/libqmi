/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * libmbim-glib -- GLib/GIO based library to control MBIM devices
 *
 * Copyright (C) 2021 Aleksander Morgado <aleksander@aleksander.es>
 *
 * Based on the QmiNetPortManagerRmnet from libqmi:
 *   Copyright (C) 2020-2021 Eric Caruso <ejcaruso@chromium.org>
 *   Copyright (C) 2020-2021 Andrew Lassalle <andrewlassalle@chromium.org>
 */

#include <linux/if_link.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <net/if.h>

#include "mbim-device.h"
#include "mbim-helpers.h"
#include "mbim-error-types.h"
#include "mbim-net-port-manager.h"
#include "mbim-helpers-netlink.h"

G_DEFINE_ABSTRACT_TYPE (MbimNetPortManager, mbim_net_port_manager, G_TYPE_OBJECT)

struct _MbimNetPortManagerPrivate {
    gchar *iface;

    /* Netlink socket */
    GSocket *socket;
    GSource *source;

    /* Netlink state */
    guint       current_sequence_id;
    GHashTable *transactions;
};

/*****************************************************************************/

void
mbim_net_port_manager_add_link (MbimNetPortManager  *self,
                                guint                session_id,
                                const gchar         *base_ifname,
                                const gchar         *ifname_prefix,
                                guint                timeout,
                                GCancellable        *cancellable,
                                GAsyncReadyCallback  callback,
                                gpointer             user_data)
{
    MBIM_NET_PORT_MANAGER_GET_CLASS (self)->add_link (self,
                                                      session_id,
                                                      base_ifname,
                                                      ifname_prefix,
                                                      timeout,
                                                      cancellable,
                                                      callback,
                                                      user_data);
}

gchar *
mbim_net_port_manager_add_link_finish (MbimNetPortManager  *self,
                                       guint               *session_id,
                                       GAsyncResult        *res,
                                       GError             **error)
{
    return MBIM_NET_PORT_MANAGER_GET_CLASS (self)->add_link_finish (self, session_id, res, error);
}

void
mbim_net_port_manager_del_link (MbimNetPortManager  *self,
                                const gchar         *ifname,
                                guint                timeout,
                                GCancellable        *cancellable,
                                GAsyncReadyCallback  callback,
                                gpointer             user_data)
{
    MBIM_NET_PORT_MANAGER_GET_CLASS (self)->del_link (self,
                                                      ifname,
                                                      timeout,
                                                      cancellable,
                                                      callback,
                                                      user_data);
}

gboolean
mbim_net_port_manager_del_link_finish (MbimNetPortManager  *self,
                                       GAsyncResult        *res,
                                       GError             **error)
{
    return MBIM_NET_PORT_MANAGER_GET_CLASS (self)->del_link_finish (self, res, error);
}

void
mbim_net_port_manager_del_all_links (MbimNetPortManager  *self,
                                     const gchar         *base_ifname,
                                     GCancellable        *cancellable,
                                     GAsyncReadyCallback  callback,
                                     gpointer             user_data)
{
    MBIM_NET_PORT_MANAGER_GET_CLASS (self)->del_all_links (self,
                                                           base_ifname,
                                                           cancellable,
                                                           callback,
                                                           user_data);
}

gboolean
mbim_net_port_manager_del_all_links_finish (MbimNetPortManager  *self,
                                            GAsyncResult        *res,
                                            GError             **error)
{
    return MBIM_NET_PORT_MANAGER_GET_CLASS (self)->del_all_links_finish (self, res, error);
}

gboolean
mbim_net_port_manager_list_links (MbimNetPortManager  *self,
                                  const gchar         *base_ifname,
                                  GPtrArray          **out_links,
                                  GError             **error)
{
    return MBIM_NET_PORT_MANAGER_GET_CLASS (self)->list_links (self, base_ifname, out_links, error);
}

/*****************************************************************************/
/* Default implementations */

static gboolean
net_port_manager_del_link_finish (MbimNetPortManager  *self,
                                  GAsyncResult        *res,
                                  GError             **error)
{
    return g_task_propagate_boolean (G_TASK (res), error);
}

static NetlinkMessage *
netlink_message_del_link (guint ifindex)
{
    NetlinkMessage *msg;

    g_assert (ifindex != 0);

    msg = mbim_helpers_netlink_message_new (RTM_DELLINK, 0);
    mbim_helpers_netlink_get_message_header (msg)->ifreq.ifi_index = ifindex;

    return msg;
}

static void
net_port_manager_del_link (MbimNetPortManager  *self,
                           const gchar         *ifname,
                           guint                timeout,
                           GCancellable        *cancellable,
                           GAsyncReadyCallback  callback,
                           gpointer             user_data)
{
    guint               ifindex;
    NetlinkMessage     *msg;
    NetlinkTransaction *tr;
    GTask              *task;
    GError             *error = NULL;
    gssize              bytes_sent;

    task = g_task_new (self, cancellable, callback, user_data);

    ifindex = if_nametoindex (ifname);
    if (ifindex == 0) {
        g_task_return_new_error (task, MBIM_CORE_ERROR, MBIM_CORE_ERROR_FAILED,
                                 "Failed to retrieve interface index for interface %s",
                                 ifname);
        g_object_unref (task);
        return;
    }

    msg = netlink_message_del_link (ifindex);
    /* The task ownership is transferred to the transaction. */
    tr = mbim_helpers_netlink_transaction_new (&self->priv->current_sequence_id,
                                               self->priv->transactions,
                                               msg,
                                               timeout,
                                               task);

    bytes_sent = g_socket_send (self->priv->socket,
                                (const gchar *) msg->data,
                                msg->len,
                                cancellable,
                                &error);
    mbim_helpers_netlink_message_free (msg);

    if (bytes_sent < 0)
        mbim_helpers_netlink_transaction_complete_with_error (tr, self->priv->transactions, error);

    g_object_unref (task);
}

typedef struct {
    GPtrArray *links;
    guint      link_i;
} DelAllLinksContext;

static void
del_all_links_context_free (DelAllLinksContext *ctx)
{
    g_clear_pointer (&ctx->links, g_ptr_array_unref);
    g_slice_free (DelAllLinksContext, ctx);
}

static gboolean
net_port_manager_del_all_links_finish (MbimNetPortManager  *self,
                                       GAsyncResult        *res,
                                       GError             **error)
{
    return g_task_propagate_boolean (G_TASK (res), error);
}

static void delete_next_link (GTask *task);

static void
port_manager_del_link_ready (MbimNetPortManager *self,
                             GAsyncResult       *res,
                             GTask              *task)
{
    DelAllLinksContext *ctx;
    GError             *error = NULL;

    ctx = g_task_get_task_data (task);

    if (!mbim_net_port_manager_del_link_finish (self, res, &error)) {
        g_task_return_error (task, error);
        g_object_unref (task);
        return;
    }

    g_ptr_array_remove_index_fast (ctx->links, 0);
    delete_next_link (task);
}

static void
delete_next_link (GTask *task)
{
    MbimNetPortManager *self;
    DelAllLinksContext *ctx;

    self = g_task_get_source_object (task);
    ctx = g_task_get_task_data (task);

    if (!ctx->links || ctx->links->len == 0) {
        g_task_return_boolean (task, TRUE);
        g_object_unref (task);
        return;
    }

    mbim_net_port_manager_del_link (self,
                                    g_ptr_array_index (ctx->links, 0),
                                    5,
                                    g_task_get_cancellable (task),
                                    (GAsyncReadyCallback) port_manager_del_link_ready,
                                    task);
}

static void
net_port_manager_del_all_links (MbimNetPortManager   *self,
                                const gchar          *base_ifname,
                                GCancellable         *cancellable,
                                GAsyncReadyCallback   callback,
                                gpointer              user_data)
{
    GTask              *task;
    DelAllLinksContext *ctx;
    GError             *error = NULL;

    task = g_task_new (self, cancellable, callback, user_data);
    ctx = g_slice_new0 (DelAllLinksContext);
    g_task_set_task_data (task, ctx, (GDestroyNotify)del_all_links_context_free);

    if (!mbim_net_port_manager_list_links (self, base_ifname, &ctx->links, &error)) {
        g_task_return_error (task, error);
        g_object_unref (task);
        return;
    }

    delete_next_link (task);
}

/*****************************************************************************/

GHashTable *
mbim_net_port_manager_peek_transactions (MbimNetPortManager *self)
{
    return self->priv->transactions;
}

gchar *
mbim_net_port_manager_peek_iface (MbimNetPortManager *self)
{
    return self->priv->iface;
}

guint *
mbim_net_port_manager_peek_current_sequence_id (MbimNetPortManager *self)
{
    return &self->priv->current_sequence_id;
}

GSocket *
mbim_net_port_manager_peek_socket (MbimNetPortManager *self)
{
    return self->priv->socket;
}

void
mbim_net_port_manager_common_setup (MbimNetPortManager *self,
                                    const gchar        *iface,
                                    GSocket            *gsocket)
{
    self->priv->iface = g_strdup (iface);
    self->priv->socket = gsocket;
    self->priv->current_sequence_id = 0;
    self->priv->transactions =
        g_hash_table_new_full (g_direct_hash,
                               g_direct_equal,
                               NULL,
                               (GDestroyNotify) mbim_helpers_netlink_transaction_free);
    mbim_helpers_netlink_set_callback (&self->priv->source,
                                       self->priv->socket,
                                       self->priv->transactions);
}

gchar *
mbim_net_port_manager_util_session_id_to_ifname (const gchar *ifname_prefix,
                                                 guint        session_id)
{
    /* Link names are in the form <PREFIX>.<SESSION ID> */
    return g_strdup_printf ("%s%u", ifname_prefix, session_id);
}

gboolean
mbim_net_port_manager_util_get_first_free_session_id (const gchar *ifname_prefix,
                                                      guint       *session_id)
{
    guint i;

    /* The minimum session id is really 0 (MBIM_DEVICE_SESSION_ID_MIN), but
     * when we have to automatically allocate a new session id we'll start at
     * 1, because 0 is also used by the non-muxed setup. */
    for (i = 1; i <= MBIM_DEVICE_SESSION_ID_MAX; i++) {
        g_autofree gchar *ifname = NULL;

        ifname = mbim_net_port_manager_util_session_id_to_ifname (ifname_prefix, i);
        if (!if_nametoindex (ifname)) {
            *session_id = i;
            return TRUE;
        }
    }

    return FALSE;
}

/*****************************************************************************/

static void
mbim_net_port_manager_init (MbimNetPortManager *self)
{
    /* Initialize private data */
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, MBIM_TYPE_NET_PORT_MANAGER, MbimNetPortManagerPrivate);
}

static void
finalize (GObject *object)
{
    MbimNetPortManager *self = MBIM_NET_PORT_MANAGER (object);

    g_assert (g_hash_table_size (self->priv->transactions) == 0);
    g_hash_table_unref (self->priv->transactions);
    g_free (self->priv->iface);

    G_OBJECT_CLASS (mbim_net_port_manager_parent_class)->finalize (object);
}

static void
mbim_net_port_manager_class_init (MbimNetPortManagerClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (object_class, sizeof (MbimNetPortManagerPrivate));

    object_class->finalize = finalize;

    klass->del_link = net_port_manager_del_link;
    klass->del_link_finish = net_port_manager_del_link_finish;
    klass->del_all_links = net_port_manager_del_all_links;
    klass->del_all_links_finish = net_port_manager_del_all_links_finish;
}
