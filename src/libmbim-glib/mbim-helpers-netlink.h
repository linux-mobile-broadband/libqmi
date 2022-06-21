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

#ifndef _LIBMBIM_GLIB_MBIM_HELPERS_NETLINK_H_
#define _LIBMBIM_GLIB_MBIM_HELPERS_NETLINK_H_

#if !defined (LIBMBIM_GLIB_COMPILATION)
#error "This is a private header!!"
#endif

#include <glib.h>
#include <gio/gio.h>

G_BEGIN_DECLS

typedef GByteArray NetlinkMessage;

typedef struct {
    struct nlmsghdr  msghdr;
    struct ifinfomsg ifreq;
} NetlinkHeader;

G_GNUC_INTERNAL
NetlinkHeader *mbim_helpers_netlink_get_message_header (NetlinkMessage *msg);

G_GNUC_INTERNAL
guint mbim_helpers_netlink_get_pos_of_next_attr (NetlinkMessage *msg);

G_GNUC_INTERNAL
void mbim_helpers_netlink_append_attribute_nested (NetlinkMessage *msg,
                                                   gushort         type);

G_GNUC_INTERNAL
void mbim_helpers_netlink_append_attribute_string (NetlinkMessage *msg,
                                                   gushort         type,
                                                   const gchar    *value);

G_GNUC_INTERNAL
void mbim_helpers_netlink_append_attribute_string_null (NetlinkMessage *msg,
                                                        gushort         type,
                                                        const gchar    *value);

G_GNUC_INTERNAL
void mbim_helpers_netlink_append_attribute_uint16 (NetlinkMessage *msg,
                                                   gushort         type,
                                                   guint16         value);

G_GNUC_INTERNAL
void mbim_helpers_netlink_append_attribute_uint32 (NetlinkMessage *msg,
                                                   gushort         type,
                                                   guint32         value);

G_GNUC_INTERNAL
NetlinkMessage *mbim_helpers_netlink_message_new (guint16 type,
                                                  guint16 extra_flags);

G_GNUC_INTERNAL
void mbim_helpers_netlink_message_free (NetlinkMessage *msg);

typedef struct {
    guint32  sequence_id;
    GSource *timeout_source;
    GTask   *completion_task;
} NetlinkTransaction;

G_GNUC_INTERNAL
void mbim_helpers_netlink_transaction_complete_with_error (NetlinkTransaction *tr,
                                                           GHashTable         *transactions,
                                                           GError             *error);

G_GNUC_INTERNAL
void mbim_helpers_netlink_transaction_complete (NetlinkTransaction *tr,
                                                GHashTable         *transactions,
                                                gint                saved_errno);

G_GNUC_INTERNAL
void mbim_helpers_netlink_transaction_free (NetlinkTransaction *tr);

G_GNUC_INTERNAL
NetlinkTransaction *mbim_helpers_netlink_transaction_new (guint          *sequence_id,
                                                          GHashTable     *transactions,
                                                          NetlinkMessage *msg,
                                                          guint           timeout,
                                                          GTask          *task);

G_GNUC_INTERNAL
void mbim_helpers_netlink_set_callback (GSource    **source,
                                        GSocket     *socket,
                                        GHashTable  *transactions);

G_END_DECLS

#endif /* _LIBMBIM_GLIB_MBIM_HELPERS_NETLINK_H_ */
