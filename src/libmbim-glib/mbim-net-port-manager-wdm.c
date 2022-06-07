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
#include <net/if.h>
#include <net/if_arp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>

#include "mbim-device.h"
#include "mbim-helpers.h"
#include "mbim-error-types.h"
#include "mbim-net-port-manager.h"
#include "mbim-net-port-manager-wdm.h"
#include "mbim-helpers-netlink.h"

G_DEFINE_TYPE (MbimNetPortManagerWdm, mbim_net_port_manager_wdm, MBIM_TYPE_NET_PORT_MANAGER)

/* alternative VLAN for IP session 0 if not untagged */
#define MBIM_IPS0_VID	4094

#define VLAN_DATA_TYPE "vlan"

/*****************************************************************************/

static guint
session_id_to_vlan_id (guint session_id)
{
    /* VLAN ID 4094 is an alternative mapping of MBIM session 0. If you create
     * a subinterface with this ID then it will take over the session 0 traffic
     * and no packets go untagged anymore. */
    return (session_id == 0 ? MBIM_IPS0_VID : session_id);
}

/*****************************************************************************/

static NetlinkMessage *
netlink_message_new_link (guint  vlan_id,
                          gchar *ifname,
                          guint  base_if_index)
{
    NetlinkMessage *msg;
    guint           linkinfo_pos, datainfo_pos;
    struct rtattr   info;

    msg = mbim_helpers_netlink_message_new (RTM_NEWLINK, NLM_F_CREATE | NLM_F_EXCL);
    mbim_helpers_netlink_append_attribute_uint32 (msg, IFLA_LINK, base_if_index);
    mbim_helpers_netlink_append_attribute_string (msg, IFLA_IFNAME, ifname);

    /* Store the position of the next attribute to adjust its length later. */
    linkinfo_pos = mbim_helpers_netlink_get_pos_of_next_attr (msg);
    mbim_helpers_netlink_append_attribute_nested (msg, IFLA_LINKINFO);
    mbim_helpers_netlink_append_attribute_string (msg, IFLA_INFO_KIND, VLAN_DATA_TYPE);

    /* Store the position of the next attribute to adjust its length later. */
    datainfo_pos = mbim_helpers_netlink_get_pos_of_next_attr (msg);
    mbim_helpers_netlink_append_attribute_nested (msg, IFLA_INFO_DATA);
    mbim_helpers_netlink_append_attribute_uint16 (msg, IFLA_VLAN_ID, vlan_id);

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
mbim_net_port_manager_wdm_list_links (MbimNetPortManager  *self,
                                      const gchar         *base_ifname,
                                      GPtrArray          **out_links,
                                      GError             **error)
{
    g_autoptr(GFile)  sysfs_file = NULL;
    g_autofree gchar *sysfs_path = NULL;

    sysfs_path = g_strdup_printf ("/sys/class/net/%s", base_ifname);
    sysfs_file = g_file_new_for_path (sysfs_path);

    return mbim_helpers_list_links_wdm (sysfs_file, NULL, NULL, out_links, error);
}

/*****************************************************************************/

typedef struct {
    guint  session_id;
    guint  vlan_id;
    gchar *ifname;
} AddLinkContext;

static void
add_link_context_free (AddLinkContext *ctx)
{
    g_free (ctx->ifname);
    g_free (ctx);
}

static gchar *
mbim_net_port_manager_wdm_add_link_finish (MbimNetPortManager  *self,
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
mbim_net_port_manager_wdm_add_link (MbimNetPortManager  *self,
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

    /* validate interface to use */
    if (g_strcmp0 (mbim_net_port_manager_peek_iface (self),
                   base_ifname) != 0) {
        g_task_return_new_error (task, MBIM_CORE_ERROR, MBIM_CORE_ERROR_FAILED,
                                 "Invalid network interface %s: expected %s",
                                 base_ifname, mbim_net_port_manager_peek_iface (self));
        g_object_unref (task);
        return;
    }

    base_if_index = if_nametoindex (base_ifname);
    if (!base_if_index) {
        g_task_return_new_error (task, MBIM_CORE_ERROR, MBIM_CORE_ERROR_FAILED,
                                 "%s interface is not available",
                                 base_ifname);
        g_object_unref (task);
        return;
    }

    ctx->ifname = mbim_net_port_manager_util_session_id_to_ifname (ifname_prefix, ctx->session_id);
    ctx->vlan_id = session_id_to_vlan_id (ctx->session_id);
    g_debug ("Using ifname '%s' and vlan id %u", ctx->ifname, ctx->vlan_id);

    msg = netlink_message_new_link (ctx->vlan_id, ctx->ifname, base_if_index);

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

MbimNetPortManagerWdm *
mbim_net_port_manager_wdm_new (const gchar  *iface,
                               GError      **error)
{
    MbimNetPortManagerWdm *self;
    gint                   socket_fd;
    GSocket               *gsocket;
    GError                *inner_error = NULL;

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

    self = g_object_new (MBIM_TYPE_NET_PORT_MANAGER_WDM, NULL);
    mbim_net_port_manager_common_setup (MBIM_NET_PORT_MANAGER (self), iface, gsocket);

    return self;
}

static void
mbim_net_port_manager_wdm_init (MbimNetPortManagerWdm *self)
{
}

static void
mbim_net_port_manager_wdm_class_init (MbimNetPortManagerWdmClass *klass)
{
    MbimNetPortManagerClass *net_port_manager_class = MBIM_NET_PORT_MANAGER_CLASS (klass);

    net_port_manager_class->list_links = mbim_net_port_manager_wdm_list_links;
    net_port_manager_class->add_link = mbim_net_port_manager_wdm_add_link;
    net_port_manager_class->add_link_finish = mbim_net_port_manager_wdm_add_link_finish;
}
