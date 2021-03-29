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
 * Copyright (C) 2020 Eric Caruso <ejcaruso@chromium.org>
 * Copyright (C) 2020 Andrew Lassalle <andrewlassalle@chromium.org>
 */

#include <linux/if_link.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>

#include <net/if.h>
#include <net/if_arp.h>

/* The if_arp.h from libc may not have this symbol yet */
#if !defined ARPHRD_RAWIP
#define ARPHRD_RAWIP 519
#endif

#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>

#include "qmi-device.h"
#include "qmi-error-types.h"
#include "qmi-errors.h"
#include "qmi-net-port-manager-rmnet.h"

G_DEFINE_TYPE (QmiNetPortManagerRmnet, qmi_net_port_manager_rmnet, QMI_TYPE_NET_PORT_MANAGER)

struct _QmiNetPortManagerRmnetPrivate {
    /* Netlink socket */
    GSocket *socket;
    GSource *source;

    /* Netlink state */
    guint       current_sequence_id;
    GHashTable *transactions;
};

#define RMNET_DATA_TYPE "rmnet"
#define RMNET_MAX_MUX_ID 255

/*****************************************************************************/

static gchar *
mux_id_to_ifname (const gchar *ifname_prefix,
                  guint        mux_id)
{
    /*
     * By convention, ifname_prefix0 corresponds to mux ID 1, and so on.
     * A more defensive implementation of this class could always fetch
     * mux ID via netlink for each existing rmnet interface instead of
     * encoding it in the interface name in this manner, and then the
     * interface name could just be assigned incrementally regardless
     * of the mux ID. */
    return g_strdup_printf ("%s%u", ifname_prefix, mux_id - 1);
}

/*****************************************************************************/
/*
 * Netlink message construction functions
 */

typedef GByteArray NetlinkMessage;

typedef struct {
    struct nlmsghdr  msghdr;
    struct ifinfomsg ifreq;
} NetlinkHeader;

static NetlinkHeader *
netlink_message_header (NetlinkMessage *msg)
{
    return (NetlinkHeader *) (msg->data);
}

static guint
get_pos_of_next_attr (NetlinkMessage *msg)
{
    return NLMSG_ALIGN (msg->len);
}

static void
append_netlink_attribute (NetlinkMessage *msg,
                          gushort         type,
                          gconstpointer   value,
                          gushort         len)
{
    guint          attr_len;
    guint          old_len;
    guint          next_attr_rel_pos;
    char          *next_attr_abs_pos;
    struct rtattr  new_attr;

    /* Expand the buffer to hold the new attribute */
    attr_len = RTA_ALIGN (RTA_LENGTH (len));
    old_len = msg->len;
    next_attr_rel_pos = get_pos_of_next_attr (msg);

    g_byte_array_set_size (msg, next_attr_rel_pos + attr_len);
    /* fill new bytes with zero, since some padding is added between attributes. */
    memset ((char *) msg->data + old_len, 0, msg->len - old_len);

    new_attr.rta_type = type;
    new_attr.rta_len = attr_len;
    next_attr_abs_pos = (char *) msg->data + next_attr_rel_pos;
    memcpy (next_attr_abs_pos, &new_attr, sizeof (struct rtattr));

    if (value)
        memcpy (RTA_DATA (next_attr_abs_pos), value, len);

    /* Update the total netlink message length */
    netlink_message_header (msg)->msghdr.nlmsg_len = msg->len;
}

static void
append_netlink_attribute_nested (NetlinkMessage *msg,
                                 gushort         type)
{
    append_netlink_attribute (msg, type, NULL, 0);
}

static void
append_netlink_attribute_string (NetlinkMessage *msg,
                                 gushort         type,
                                 const gchar    *value)
{
    append_netlink_attribute (msg, type, value, strlen (value));
}

static void
append_netlink_attribute_uint16 (NetlinkMessage *msg,
                                 gushort         type,
                                 guint16         value)
{
    append_netlink_attribute (msg, type, &value, sizeof (value));
}

static void
append_netlink_attribute_uint32 (NetlinkMessage *msg,
                                 gushort         type,
                                 guint32         value)
{
    append_netlink_attribute (msg, type, &value, sizeof (value));
}

static NetlinkMessage *
netlink_message_new (guint16 type,
                     guint16 extra_flags)
{
    NetlinkMessage *msg;
    NetlinkHeader  *hdr;

    int size = sizeof (NetlinkHeader);

    msg = g_byte_array_new ();
    g_byte_array_set_size (msg, size);
    memset ((char *) msg->data, 0, size);

    hdr = netlink_message_header (msg);
    hdr->msghdr.nlmsg_len = msg->len;
    hdr->msghdr.nlmsg_type = type;
    hdr->msghdr.nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK | extra_flags;
    hdr->ifreq.ifi_family = AF_UNSPEC;
    if (type != RTM_DELLINK) {
        hdr->ifreq.ifi_type = ARPHRD_RAWIP;
        hdr->ifreq.ifi_flags = 0;
        hdr->ifreq.ifi_change = 0xFFFFFFFF;
    }
    return msg;
}

static void
netlink_message_free (NetlinkMessage *msg)
{
    g_byte_array_unref (msg);
}

/*****************************************************************************/

typedef struct {
    QmiNetPortManagerRmnet *manager;
    guint32                 sequence_id;
    GSource                *timeout_source;
    GTask                  *completion_task;
} Transaction;

static gboolean
transaction_timed_out (Transaction *tr)
{
    GTask *task;
    guint32 sequence_id;

    task = g_steal_pointer (&tr->completion_task);
    sequence_id = tr->sequence_id;

    g_hash_table_remove (tr->manager->priv->transactions,
                         GUINT_TO_POINTER (tr->sequence_id));

    g_task_return_new_error (task,
                             G_IO_ERROR,
                             G_IO_ERROR_TIMED_OUT,
                             "Netlink message with sequence ID %u timed out",
                             sequence_id);

    g_object_unref (task);
    return G_SOURCE_REMOVE;
}

static void
transaction_complete_with_error (Transaction *tr,
                                 GError      *error)
{
    GTask *task;

    task = g_steal_pointer (&tr->completion_task);

    g_hash_table_remove (tr->manager->priv->transactions,
                         GUINT_TO_POINTER (tr->sequence_id));

    g_task_return_error (task, error);
    g_object_unref (task);
}

static void
transaction_complete (Transaction *tr,
                      gint         saved_errno)
{
    GTask *task;
    guint32 sequence_id;

    task = g_steal_pointer (&tr->completion_task);
    sequence_id = tr->sequence_id;

    g_hash_table_remove (tr->manager->priv->transactions,
                         GUINT_TO_POINTER (tr->sequence_id));

    if (!saved_errno) {
        g_task_return_boolean (task, TRUE);
    } else {
        g_task_return_new_error (task,
                                 G_IO_ERROR,
                                 g_io_error_from_errno (saved_errno),
                                 "Netlink message with transaction %u failed",
                                 sequence_id);
    }

    g_object_unref (task);
}

static void
transaction_free (Transaction *tr)
{
    g_assert (tr->completion_task == NULL);
    g_source_destroy (tr->timeout_source);
    g_source_unref (tr->timeout_source);
    g_slice_free (Transaction, tr);
}

static Transaction *
transaction_new (QmiNetPortManagerRmnet *manager,
                 NetlinkMessage         *msg,
                 guint                   timeout,
                 GTask                  *task)
{
    Transaction *tr;

    tr = g_slice_new0 (Transaction);
    tr->manager = manager;
    tr->sequence_id = ++manager->priv->current_sequence_id;
    netlink_message_header (msg)->msghdr.nlmsg_seq = tr->sequence_id;
    if (timeout) {
        tr->timeout_source = g_timeout_source_new_seconds (timeout);
        g_source_set_callback (tr->timeout_source,
                               (GSourceFunc) transaction_timed_out,
                               tr,
                               NULL);
        g_source_attach (tr->timeout_source,
                         g_main_context_get_thread_default ());
    }
    tr->completion_task = g_object_ref (task);

    g_hash_table_insert (manager->priv->transactions,
                         GUINT_TO_POINTER (tr->sequence_id),
                         tr);
    return tr;
}

/*****************************************************************************/

static NetlinkMessage *
netlink_message_new_link (guint  mux_id,
                          gchar *ifname,
                          guint  base_if_index,
                          guint  rmnet_flags,
                          guint  rmnet_mask)
{
    NetlinkMessage          *msg;
    guint                    linkinfo_pos, datainfo_pos;
    struct rtattr            info;
    struct ifla_rmnet_flags  flags;

    g_assert (mux_id != QMI_DEVICE_MUX_ID_UNBOUND);

    msg = netlink_message_new (RTM_NEWLINK, NLM_F_CREATE | NLM_F_EXCL);
    append_netlink_attribute_uint32 (msg, IFLA_LINK, base_if_index);
    append_netlink_attribute_string (msg, IFLA_IFNAME, ifname);

    /* Store the position of the next attribute to adjust its length later. */
    linkinfo_pos = get_pos_of_next_attr (msg);
    append_netlink_attribute_nested (msg, IFLA_LINKINFO);
    append_netlink_attribute_string (msg, IFLA_INFO_KIND, RMNET_DATA_TYPE);

    /* Store the position of the next attribute to adjust its length later. */
    datainfo_pos = get_pos_of_next_attr (msg);
    append_netlink_attribute_nested (msg, IFLA_INFO_DATA);
    append_netlink_attribute_uint16 (msg, IFLA_RMNET_MUX_ID, mux_id);

    flags.flags = rmnet_flags;
    flags.mask = rmnet_mask;
    append_netlink_attribute (msg, IFLA_RMNET_FLAGS, &flags, sizeof (struct ifla_rmnet_flags));

    /* Use memcpy to preserve byte alignment */
    memcpy (&info, (char *) msg->data + datainfo_pos, sizeof (struct rtattr));
    info.rta_len = msg->len - datainfo_pos;
    memcpy ((char *) msg->data + datainfo_pos, &info, sizeof (struct rtattr));

    memcpy (&info, (char *) msg->data + linkinfo_pos, sizeof (struct rtattr));
    info.rta_len = msg->len - linkinfo_pos;
    memcpy ((char *) msg->data + linkinfo_pos, &info, sizeof (struct rtattr));

    return msg;
}

static NetlinkMessage *
netlink_message_del_link (guint ifindex)
{
    NetlinkMessage *msg;

    g_assert (ifindex != 0);

    msg = netlink_message_new (RTM_DELLINK, 0);
    netlink_message_header (msg)->ifreq.ifi_index = ifindex;

    return msg;
}

/*****************************************************************************/

static gboolean
netlink_message_cb (GSocket                *socket,
                    GIOCondition            condition,
                    QmiNetPortManagerRmnet *self)
{
    GError          *error = NULL;
    gchar            buf[512];
    int              bytes_received;
    unsigned int     buffer_len;
    struct nlmsghdr *hdr;

    if (condition & G_IO_HUP || condition & G_IO_ERR) {
        g_warning ("[netlink] socket connection closed.");
        return G_SOURCE_REMOVE;
    }

    bytes_received = g_socket_receive (socket, buf, sizeof (buf), NULL, &error);

    if (bytes_received < 0) {
        g_warning ("[netlink] socket i/o failure: %s", error->message);
        g_error_free (error);
        return G_SOURCE_REMOVE;
    }

    buffer_len = (unsigned int ) bytes_received;
    for (hdr = (struct nlmsghdr *) buf; NLMSG_OK (hdr, buffer_len);
         NLMSG_NEXT (hdr, buffer_len)) {
        Transaction     *tr;
        struct nlmsgerr *err;

        if (hdr->nlmsg_type != NLMSG_ERROR)
            continue;

        tr = g_hash_table_lookup (self->priv->transactions,
                                  GUINT_TO_POINTER (hdr->nlmsg_seq));
        if (!tr)
            continue;

        err = NLMSG_DATA (buf);
        transaction_complete (tr, err->error);
    }
    return G_SOURCE_CONTINUE;
}

/*****************************************************************************/

static guint
get_first_free_mux_id (QmiNetPortManagerRmnet *self,
                       const gchar            *ifname_prefix)
{
    guint i;

    /* Note that this function does not actually need to use the
     * QmiNetPortManagerRmnet at the moment. But it will if we ever do rewrite it
     * to inspect mux IDs of existing rmnet interfaces, so we will pass it
     * for now. */

    for (i = QMI_DEVICE_MUX_ID_MIN; i <= QMI_DEVICE_MUX_ID_MAX; i++) {
        gchar   *ifname;
        gboolean mux_id_is_free;

        ifname = mux_id_to_ifname (ifname_prefix, i);
        mux_id_is_free = !if_nametoindex (ifname);
        g_free (ifname);
        if (mux_id_is_free)
            return i;
    }

    return QMI_DEVICE_MUX_ID_UNBOUND;
}

/*****************************************************************************/
typedef struct {
    guint  mux_id;
    gchar *ifname;
} AddLinkContext;

static void
add_link_context_free (AddLinkContext *ctx)
{
    g_free (ctx->ifname);
    g_free (ctx);
}

static gchar *
net_port_manager_add_link_finish (QmiNetPortManager  *self,
                                  guint              *mux_id,
                                  GAsyncResult       *res,
                                  GError            **error)
{
    AddLinkContext *ctx;

    ctx = g_task_get_task_data (G_TASK (res));

    if (!g_task_propagate_boolean (G_TASK (res), error)) {
        g_prefix_error (error, "Failed to add link with mux id %d: ",
                        ctx->mux_id);
        return NULL;
    }

    *mux_id = ctx->mux_id;
    return g_steal_pointer (&ctx->ifname);
}

static void
net_port_manager_add_link (QmiNetPortManager     *_self,
                           guint                  mux_id,
                           const gchar           *base_ifname,
                           const gchar           *ifname_prefix,
                           QmiDeviceAddLinkFlags  flags,
                           guint                  timeout,
                           GCancellable          *cancellable,
                           GAsyncReadyCallback    callback,
                           gpointer               user_data)
{
    QmiNetPortManagerRmnet *self = QMI_NET_PORT_MANAGER_RMNET (_self);
    NetlinkMessage         *msg;
    Transaction            *tr;
    GTask                  *task;
    GError                 *error = NULL;
    gssize                  bytes_sent;
    guint                   base_if_index;
    AddLinkContext         *ctx;
    guint                   rmnet_flags;
    guint                   rmnet_mask;

    task = g_task_new (self, cancellable, callback, user_data);

    ctx = g_new0 (AddLinkContext, 1);
    ctx->mux_id = mux_id;
    g_task_set_task_data (task, ctx, (GDestroyNotify) add_link_context_free);

    if (ctx->mux_id == QMI_DEVICE_MUX_ID_UNBOUND) {
        g_task_return_new_error (task,
                                 QMI_CORE_ERROR,
                                 QMI_CORE_ERROR_FAILED,
                                 "Tried to create interface for unbound mux ID");
        g_object_unref (task);
        return;
    }

    if (ctx->mux_id == QMI_DEVICE_MUX_ID_AUTOMATIC) {
        ctx->mux_id = get_first_free_mux_id (self, ifname_prefix);

        g_debug ("Using dynamic mux ID %u", ctx->mux_id);
        if (ctx->mux_id == QMI_DEVICE_MUX_ID_UNBOUND) {
            g_task_return_new_error (task,
                                     QMI_CORE_ERROR,
                                     QMI_CORE_ERROR_FAILED,
                                     "Failed to find an available mux ID");
            g_object_unref (task);
            return;
        }
    } else
        g_debug ("Using static mux ID %u", ctx->mux_id);

    base_if_index = if_nametoindex (base_ifname);
    if (!base_if_index) {
        g_task_return_new_error (task,
                                 QMI_CORE_ERROR,
                                 QMI_CORE_ERROR_FAILED,
                                 "%s interface is not available",
                                 base_ifname);
        g_object_unref (task);
        return;
    }

    ctx->ifname = mux_id_to_ifname (ifname_prefix, ctx->mux_id);

    /* Convert flags from libqmi API to rmnet API */
    rmnet_flags = RMNET_FLAGS_INGRESS_DEAGGREGATION;
    if (flags & QMI_DEVICE_ADD_LINK_FLAGS_INGRESS_MAP_CKSUMV4)
        rmnet_flags |= RMNET_FLAGS_INGRESS_MAP_CKSUMV4;
    if (flags & QMI_DEVICE_ADD_LINK_FLAGS_EGRESS_MAP_CKSUMV4)
        rmnet_flags |= RMNET_FLAGS_EGRESS_MAP_CKSUMV4;
    rmnet_mask = (RMNET_FLAGS_EGRESS_MAP_CKSUMV4  |
                  RMNET_FLAGS_INGRESS_MAP_CKSUMV4 |
                  RMNET_FLAGS_INGRESS_DEAGGREGATION);

    msg = netlink_message_new_link (ctx->mux_id, ctx->ifname, base_if_index, rmnet_flags, rmnet_mask);

    /* The task ownership is transferred to the transaction. */
    tr = transaction_new (self, msg, timeout, task);

    bytes_sent = g_socket_send (self->priv->socket,
                                (const gchar *) msg->data,
                                msg->len,
                                cancellable,
                                &error);
    netlink_message_free (msg);

    if (bytes_sent < 0)
        transaction_complete_with_error (tr, error);

    g_object_unref (task);
}

static gboolean
net_port_manager_del_link_finish (QmiNetPortManager  *self,
                                  GAsyncResult       *res,
                                  GError            **error)
{
    return g_task_propagate_boolean (G_TASK (res), error);
}

static void
net_port_manager_del_link (QmiNetPortManager   *_self,
                           const gchar         *ifname,
                           guint                mux_id_unused,
                           guint                timeout,
                           GCancellable        *cancellable,
                           GAsyncReadyCallback  callback,
                           gpointer             user_data)
{
    QmiNetPortManagerRmnet *self = QMI_NET_PORT_MANAGER_RMNET (_self);
    guint                   ifindex;
    NetlinkMessage         *msg;
    Transaction            *tr;
    GTask                  *task;
    GError                 *error = NULL;
    gssize                  bytes_sent;

    task = g_task_new (self, cancellable, callback, user_data);

    ifindex = if_nametoindex (ifname);
    if (ifindex == 0) {
        g_task_return_new_error (task,
                                 QMI_CORE_ERROR,
                                 QMI_CORE_ERROR_FAILED,
                                 "Failed to retrieve interface index for interface:%s",
                                 ifname);
        g_object_unref (task);
        return;
    }

    msg = netlink_message_del_link (ifindex);
    /* The task ownership is transferred to the transaction. */
    tr = transaction_new (self, msg, timeout, task);

    bytes_sent = g_socket_send (self->priv->socket,
                                (const gchar *) msg->data,
                                msg->len,
                                cancellable,
                                &error);
    netlink_message_free (msg);

    if (bytes_sent < 0)
        transaction_complete_with_error (tr, error);

    g_object_unref (task);
}

/*****************************************************************************/

QmiNetPortManagerRmnet *
qmi_net_port_manager_rmnet_new (GError **error)
{
    QmiNetPortManagerRmnet *self;
    gint                    socket_fd;
    GSocket                *gsocket;
    GError                 *inner_error = NULL;

    socket_fd = socket (AF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE);
    if (socket_fd < 0) {
        g_set_error (error,
                     QMI_CORE_ERROR,
                     QMI_CORE_ERROR_FAILED,
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

    self = g_object_new (QMI_TYPE_NET_PORT_MANAGER_RMNET, NULL);
    self->priv->socket = gsocket;
    self->priv->source = g_socket_create_source (self->priv->socket,
                                                 G_IO_IN | G_IO_ERR | G_IO_HUP,
                                                 NULL);
    g_source_set_callback (self->priv->source,
                           (GSourceFunc) netlink_message_cb,
                           self,
                           NULL);
    g_source_attach (self->priv->source, g_main_context_get_thread_default ());

    self->priv->current_sequence_id = 0;
    self->priv->transactions =
        g_hash_table_new_full (g_direct_hash,
                               g_direct_equal,
                               NULL,
                               (GDestroyNotify) transaction_free);
    return self;
}

/*****************************************************************************/

static void
qmi_net_port_manager_rmnet_init (QmiNetPortManagerRmnet *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
                                              QMI_TYPE_NET_PORT_MANAGER_RMNET,
                                              QmiNetPortManagerRmnetPrivate);
}

static void
dispose (GObject *object)
{
    QmiNetPortManagerRmnet *self = QMI_NET_PORT_MANAGER_RMNET (object);

    g_assert (g_hash_table_size (self->priv->transactions) == 0);

    g_clear_pointer (&self->priv->transactions, g_hash_table_unref);
    if (self->priv->source)
        g_source_destroy (self->priv->source);
    g_clear_pointer (&self->priv->source, g_source_unref);
    g_clear_object (&self->priv->socket);

    G_OBJECT_CLASS (qmi_net_port_manager_rmnet_parent_class)->dispose (object);
}

static void
qmi_net_port_manager_rmnet_class_init (QmiNetPortManagerRmnetClass *klass)
{
    GObjectClass           *object_class = G_OBJECT_CLASS (klass);
    QmiNetPortManagerClass *net_port_manager_class = QMI_NET_PORT_MANAGER_CLASS (klass);

    g_type_class_add_private (object_class, sizeof (QmiNetPortManagerRmnetPrivate));

    object_class->dispose = dispose;

    net_port_manager_class->add_link = net_port_manager_add_link;
    net_port_manager_class->add_link_finish = net_port_manager_add_link_finish;
    net_port_manager_class->del_link = net_port_manager_del_link;
    net_port_manager_class->del_link_finish = net_port_manager_del_link_finish;
}
