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
#include <sys/socket.h>

#include "mbim-helpers-netlink.h"

/*****************************************************************************/
/*
 * Netlink message construction functions
 */

NetlinkHeader *
mbim_helpers_netlink_get_message_header (NetlinkMessage *msg)
{
    return (NetlinkHeader *) (msg->data);
}

guint
mbim_helpers_netlink_get_pos_of_next_attr (NetlinkMessage *msg)
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
    next_attr_rel_pos = mbim_helpers_netlink_get_pos_of_next_attr (msg);

    g_byte_array_set_size (msg, next_attr_rel_pos + attr_len);
    /* fill new bytes with zero, since some padding is added between attributes. */
    memset ((char *) msg->data + old_len, 0, msg->len - old_len);

    new_attr.rta_type = type;
    new_attr.rta_len = RTA_LENGTH (len);
    next_attr_abs_pos = (char *) msg->data + next_attr_rel_pos;
    memcpy (next_attr_abs_pos, &new_attr, sizeof (struct rtattr));

    if (value)
        memcpy (RTA_DATA (next_attr_abs_pos), value, len);

    /* Update the total netlink message length */
    mbim_helpers_netlink_get_message_header (msg)->msghdr.nlmsg_len = msg->len;
}

void
mbim_helpers_netlink_append_attribute_nested (NetlinkMessage *msg,
                                              gushort         type)
{
    append_netlink_attribute (msg, type, NULL, 0);
}

void
mbim_helpers_netlink_append_attribute_string (NetlinkMessage *msg,
                                              gushort         type,
                                              const gchar    *value)
{
    append_netlink_attribute (msg, type, value, strlen (value));
}

void
mbim_helpers_netlink_append_attribute_uint16 (NetlinkMessage *msg,
                                              gushort         type,
                                              guint16         value)
{
    append_netlink_attribute (msg, type, &value, sizeof (value));
}

void
mbim_helpers_netlink_append_attribute_uint32 (NetlinkMessage *msg,
                                              gushort         type,
                                              guint32         value)
{
    append_netlink_attribute (msg, type, &value, sizeof (value));
}

NetlinkMessage *
mbim_helpers_netlink_message_new (guint16 type,
                                  guint16 extra_flags)
{
    NetlinkMessage *msg;
    NetlinkHeader  *hdr;

    int size = sizeof (NetlinkHeader);

    msg = g_byte_array_new ();
    g_byte_array_set_size (msg, size);
    memset ((char *) msg->data, 0, size);

    hdr = mbim_helpers_netlink_get_message_header (msg);
    hdr->msghdr.nlmsg_len = msg->len;
    hdr->msghdr.nlmsg_type = type;
    hdr->msghdr.nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK | extra_flags;
    hdr->ifreq.ifi_family = AF_UNSPEC;
    return msg;
}

void
mbim_helpers_netlink_message_free (NetlinkMessage *msg)
{
    g_byte_array_unref (msg);
}

/*****************************************************************************/
/*
 * Transaction management functions
 */

static gboolean
transaction_timed_out (NetlinkTransaction *tr,
                       GHashTable         *transactions)
{
    GTask   *task;
    guint32  sequence_id;

    task = g_steal_pointer (&tr->completion_task);
    sequence_id = tr->sequence_id;

    g_hash_table_remove (transactions,
                         GUINT_TO_POINTER (tr->sequence_id));

    g_task_return_new_error (task,
                             G_IO_ERROR,
                             G_IO_ERROR_TIMED_OUT,
                             "Netlink message with sequence ID %u timed out",
                             sequence_id);

    g_object_unref (task);
    return G_SOURCE_REMOVE;
}

void
mbim_helpers_netlink_transaction_complete_with_error (NetlinkTransaction *tr,
                                                      GHashTable         *transactions,
                                                      GError             *error)
{
    GTask *task;

    task = g_steal_pointer (&tr->completion_task);

    g_hash_table_remove (transactions,
                         GUINT_TO_POINTER (tr->sequence_id));

    g_task_return_error (task, error);
    g_object_unref (task);
}

void
mbim_helpers_netlink_transaction_complete (NetlinkTransaction *tr,
                                           GHashTable         *transactions,
                                           gint                saved_errno)
{
    GTask   *task;
    guint32  sequence_id;

    task = g_steal_pointer (&tr->completion_task);
    sequence_id = tr->sequence_id;

    g_hash_table_remove (transactions,
                         GUINT_TO_POINTER (tr->sequence_id));

    if (!saved_errno) {
        g_task_return_boolean (task, TRUE);
    } else {
        g_task_return_new_error (task,
                                 G_IO_ERROR,
                                 g_io_error_from_errno (saved_errno),
                                 "Netlink message with transaction %u failed: %s",
                                 sequence_id,
                                 g_strerror (saved_errno));
    }

    g_object_unref (task);
}

void
mbim_helpers_netlink_transaction_free (NetlinkTransaction *tr)
{
    g_assert (tr->completion_task == NULL);
    g_source_destroy (tr->timeout_source);
    g_source_unref (tr->timeout_source);
    g_slice_free (NetlinkTransaction, tr);
}

NetlinkTransaction *
mbim_helpers_netlink_transaction_new (guint          *sequence_id,
                                      GHashTable     *transactions,
                                      NetlinkMessage *msg,
                                      guint           timeout,
                                      GTask          *task)
{
    NetlinkTransaction *tr;

    tr = g_slice_new0 (NetlinkTransaction);
    tr->sequence_id = ++(*sequence_id);
    mbim_helpers_netlink_get_message_header (msg)->msghdr.nlmsg_seq = tr->sequence_id;
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

    g_hash_table_insert (transactions,
                         GUINT_TO_POINTER (tr->sequence_id),
                         tr);
    return tr;
}

/*****************************************************************************/

static gboolean
netlink_message_cb (GSocket      *socket,
                    GIOCondition  condition,
                    GHashTable   *transactions)
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
        NetlinkTransaction *tr;
        struct nlmsgerr    *err;

        if (hdr->nlmsg_type != NLMSG_ERROR)
            continue;

        tr = g_hash_table_lookup (transactions,
                                  GUINT_TO_POINTER (hdr->nlmsg_seq));
        if (!tr)
            continue;

        err = NLMSG_DATA (buf);
        mbim_helpers_netlink_transaction_complete (tr, transactions, err->error);
    }
    return G_SOURCE_CONTINUE;
}

void
mbim_helpers_netlink_set_callback (GSource    **source,
                                   GSocket     *socket,
                                   GHashTable  *transactions)
{
    *source = g_socket_create_source (socket,
                                      G_IO_IN | G_IO_ERR | G_IO_HUP,
                                      NULL);
    g_source_set_callback (*source,
                           (GSourceFunc) netlink_message_cb,
                           transactions,
                           NULL);
    g_source_attach (*source, g_main_context_get_thread_default ());
}
