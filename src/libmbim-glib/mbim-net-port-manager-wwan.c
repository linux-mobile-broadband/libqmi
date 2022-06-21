/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * libmbim-glib -- GLib/GIO based library to control MBIM devices
 *
 * Copyright (C) 2022 Daniele Palmas <dnlplm@gmail.com>
 *
 * Based on previous work:
 *   Copyright (C) 2020-2021 Eric Caruso <ejcaruso@chromium.org>
 *   Copyright (C) 2020-2021 Andrew Lassalle <andrewlassalle@chromium.org>
 *   Copyright (C) 2021 Aleksander Morgado <aleksander@aleksander.es>
 */

#include <linux/if_link.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>

/* This is a built-in file, not provided by the kernel headers,
 * used to add wwan symbols if not available */
#include <kernel/wwan.h>

#include <net/if.h>
#include <net/if_arp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>

#include "mbim-device.h"
#include "mbim-helpers.h"
#include "mbim-error-types.h"
#include "mbim-net-port-manager.h"
#include "mbim-net-port-manager-wwan.h"
#include "mbim-helpers-netlink.h"

G_DEFINE_TYPE (MbimNetPortManagerWwan, mbim_net_port_manager_wwan, MBIM_TYPE_NET_PORT_MANAGER)

#define WWAN_DATA_TYPE "wwan"

/*****************************************************************************/

static NetlinkMessage *
netlink_message_new_link (guint        link_id,
                          gchar       *ifname,
                          const gchar *base_if_name)
{
    NetlinkMessage *msg;
    guint           linkinfo_pos, datainfo_pos;
    struct rtattr   info;

    msg = mbim_helpers_netlink_message_new (RTM_NEWLINK, NLM_F_CREATE | NLM_F_EXCL);
    /* IFLA_PARENT_DEV_NAME has type NLA_NUL_STRING */
    mbim_helpers_netlink_append_attribute_string_null (msg, IFLA_PARENT_DEV_NAME, base_if_name);
    mbim_helpers_netlink_append_attribute_string (msg, IFLA_IFNAME, ifname);

    /* Store the position of the next attribute to adjust its length later. */
    linkinfo_pos = mbim_helpers_netlink_get_pos_of_next_attr (msg);
    mbim_helpers_netlink_append_attribute_nested (msg, IFLA_LINKINFO);
    mbim_helpers_netlink_append_attribute_string (msg, IFLA_INFO_KIND, WWAN_DATA_TYPE);

    /* Store the position of the next attribute to adjust its length later. */
    datainfo_pos = mbim_helpers_netlink_get_pos_of_next_attr (msg);
    mbim_helpers_netlink_append_attribute_nested (msg, IFLA_INFO_DATA);
    mbim_helpers_netlink_append_attribute_uint32 (msg, IFLA_WWAN_LINK_ID, link_id);

    /* Use memcpy to preserve byte alignment */
    memcpy (&info, (char *) msg->data + datainfo_pos, sizeof (struct rtattr));
    info.rta_len = msg->len - datainfo_pos;
    memcpy ((char *) msg->data + datainfo_pos, &info, sizeof (struct rtattr));

    memcpy (&info, (char *) msg->data + linkinfo_pos, sizeof (struct rtattr));
    info.rta_len = msg->len - linkinfo_pos;
    memcpy ((char *) msg->data + linkinfo_pos, &info, sizeof (struct rtattr));

    return msg;
}

/*****************************************************************************/

static gboolean
mbim_net_port_manager_wwan_list_links (MbimNetPortManager  *self,
                                       const gchar         *base_ifname,
                                       GPtrArray          **out_links,
                                       GError             **error)
{
    g_autoptr(GFile)  sysfs_file = NULL;
    g_autofree gchar *sysfs_path = NULL;

    sysfs_path = g_strdup_printf ("/sys/class/net/%s/device/net", base_ifname);
    sysfs_file = g_file_new_for_path (sysfs_path);

    return mbim_helpers_list_links_wwan (base_ifname, sysfs_file, NULL, NULL, out_links, error);
}

/*****************************************************************************/

typedef struct {
    guint  session_id;
    guint  link_id;
    gchar *ifname;
} AddLinkContext;

static void
add_link_context_free (AddLinkContext *ctx)
{
    g_free (ctx->ifname);
    g_free (ctx);
}

static gchar *
mbim_net_port_manager_wwan_add_link_finish (MbimNetPortManager  *self,
                                            guint               *session_id,
                                            GAsyncResult        *res,
                                            GError             **error)
{
    AddLinkContext *ctx;

    ctx = g_task_get_task_data (G_TASK (res));

    if (!g_task_propagate_boolean (G_TASK (res), error)) {
        g_prefix_error (error, "Failed to add link with session id %d: ",
                        ctx->session_id);
        return NULL;
    }

    *session_id = ctx->session_id;
    return g_steal_pointer (&ctx->ifname);
}

static void
mbim_net_port_manager_wwan_add_link (MbimNetPortManager  *self,
                                     guint                session_id,
                                     const gchar         *base_ifname,
                                     const gchar         *ifname_prefix,
                                     guint                timeout,
                                     GCancellable        *cancellable,
                                     GAsyncReadyCallback  callback,
                                     gpointer             user_data)
{
    NetlinkMessage     *msg;
    NetlinkTransaction *tr;
    GTask              *task;
    GError             *error = NULL;
    gssize              bytes_sent;
    guint               base_if_index;
    AddLinkContext     *ctx;

    task = g_task_new (self, cancellable, callback, user_data);

    ctx = g_new0 (AddLinkContext, 1);
    ctx->session_id = session_id;
    g_task_set_task_data (task, ctx, (GDestroyNotify) add_link_context_free);

    if (ctx->session_id == MBIM_DEVICE_SESSION_ID_AUTOMATIC) {
        if (!mbim_net_port_manager_util_get_first_free_session_id (ifname_prefix, &ctx->session_id)) {
            g_task_return_new_error (task, MBIM_CORE_ERROR, MBIM_CORE_ERROR_FAILED,
                                     "Failed to find an available session ID");
            g_object_unref (task);
            return;
        }
        g_debug ("Using dynamic session ID %u", ctx->session_id);
    } else
        g_debug ("Using static session ID %u", ctx->session_id);

    base_if_index = if_nametoindex (base_ifname);
    if (!base_if_index) {
        g_task_return_new_error (task, MBIM_CORE_ERROR, MBIM_CORE_ERROR_FAILED,
                                 "%s interface is not available",
                                 base_ifname);
        g_object_unref (task);
        return;
    }

    ctx->ifname = mbim_net_port_manager_util_session_id_to_ifname (ifname_prefix, ctx->session_id);
    ctx->link_id = ctx->session_id;
    g_debug ("Using ifname '%s' and link id %u", ctx->ifname, ctx->link_id);

    msg = netlink_message_new_link (ctx->link_id, ctx->ifname, base_ifname);

    /* The task ownership is transferred to the transaction. */
    tr = mbim_helpers_netlink_transaction_new (mbim_net_port_manager_peek_current_sequence_id (MBIM_NET_PORT_MANAGER (self)),
                                               mbim_net_port_manager_peek_transactions (MBIM_NET_PORT_MANAGER (self)),
                                               msg, timeout, task);

    bytes_sent = g_socket_send (mbim_net_port_manager_peek_socket (MBIM_NET_PORT_MANAGER (self)),
                                (const gchar *) msg->data,
                                msg->len,
                                cancellable,
                                &error);
    mbim_helpers_netlink_message_free (msg);

    if (bytes_sent < 0)
        mbim_helpers_netlink_transaction_complete_with_error (tr,
                                                              mbim_net_port_manager_peek_transactions (MBIM_NET_PORT_MANAGER (self)),
                                                              error);

    g_object_unref (task);
}

/*****************************************************************************/

MbimNetPortManagerWwan *
mbim_net_port_manager_wwan_new (GError **error)
{
    MbimNetPortManagerWwan *self;
    gint                    socket_fd;
    GSocket                *gsocket;
    GError                 *inner_error = NULL;

    socket_fd = socket (AF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE);
    if (socket_fd < 0) {
        g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_FAILED,
                     "Failed to create netlink socket");
        return NULL;
    }

    gsocket = g_socket_new_from_fd (socket_fd, &inner_error);
    if (inner_error) {
        g_debug ("Could not create socket: %s", inner_error->message);
        close (socket_fd);
        g_propagate_error (error, inner_error);
        return NULL;
    }

    self = g_object_new (MBIM_TYPE_NET_PORT_MANAGER_WWAN, NULL);

    mbim_net_port_manager_common_setup (MBIM_NET_PORT_MANAGER (self), NULL, gsocket);

    return self;
}

static void
mbim_net_port_manager_wwan_init (MbimNetPortManagerWwan *self)
{
}

static void
mbim_net_port_manager_wwan_class_init (MbimNetPortManagerWwanClass *klass)
{
    MbimNetPortManagerClass *net_port_manager_class = MBIM_NET_PORT_MANAGER_CLASS (klass);

    net_port_manager_class->list_links = mbim_net_port_manager_wwan_list_links;
    net_port_manager_class->add_link = mbim_net_port_manager_wwan_add_link;
    net_port_manager_class->add_link_finish = mbim_net_port_manager_wwan_add_link_finish;
}
