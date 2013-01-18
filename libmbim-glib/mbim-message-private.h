/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 * libmbim-glib -- GLib/GIO based library to control MBIM devices
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
 * Copyright (C) 2013 Aleksander Morgado <aleksander@gnu.org>
 *
 * This is a private non-installed header
 */

#ifndef _LIBMBIM_GLIB_MBIM_MESSAGE_PRIVATE_H_
#define _LIBMBIM_GLIB_MBIM_MESSAGE_PRIVATE_H_

#if !defined (LIBMBIM_GLIB_COMPILATION)
#error "This is a private header!!"
#endif

#include <glib.h>

#include "mbim-message.h"

G_BEGIN_DECLS

/*****************************************************************************/
/* Basic message types */

struct header {
  guint32 type;
  guint32 length;
  guint32 transaction_id;
} __attribute__((packed));

#define MBIM_MESSAGE_GET_MESSAGE_TYPE(self)                             \
    (MbimMessageType) GUINT32_FROM_LE (((struct header *)(self->data))->type)
#define MBIM_MESSAGE_GET_MESSAGE_LENGTH(self)                           \
    (MbimMessageType) GUINT32_FROM_LE (((struct header *)(self->data))->length)
#define MBIM_MESSAGE_GET_TRANSACTION_ID(self)                           \
    (MbimMessageType) GUINT32_FROM_LE (((struct header *)(self->data))->transaction_id)

struct open_message {
    guint32 max_control_transfer;
} __attribute__((packed));

struct open_done_message {
    guint32 status_code;
} __attribute__((packed));

struct close_done_message {
    guint32 status_code;
} __attribute__((packed));

struct error_message {
    guint32 error_status_code;
} __attribute__((packed));

struct fragment_header {
  guint32 total;
  guint32 current;
} __attribute__((packed));

struct fragment_message {
    struct fragment_header fragment_header;
    guint8                 buffer[];
} __attribute__((packed));

struct full_message {
    struct header header;
    union {
        struct open_message       open;
        struct open_done_message  open_done;
        /* nothing needed for close_message */
        struct close_done_message close_done;
        struct error_message      error;
        struct fragment_message   fragment;
    } message;
} __attribute__((packed));

/*****************************************************************************/
/* Fragment interface */

gboolean      _mbim_message_is_fragment          (const MbimMessage  *self);
guint32       _mbim_message_fragment_get_total   (const MbimMessage  *self);
guint32       _mbim_message_fragment_get_current (const MbimMessage  *self);
const guint8 *_mbim_message_fragment_get_payload (const MbimMessage  *self,
                                                  guint32            *length);

/* Merge fragments into a message... */

MbimMessage *_mbim_message_fragment_collector_init     (const MbimMessage  *fragment,
                                                        GError            **error);
gboolean     _mbim_message_fragment_collector_add      (MbimMessage        *self,
                                                        const MbimMessage  *fragment,
                                                        GError            **error);
gboolean     _mbim_message_fragment_collector_complete (MbimMessage        *self);

/* Split message into fragments... */

struct fragment_info {
    struct header           header;
    struct fragment_header  fragment_header;
    guint32                 data_length;
    const guint8           *data;
} __attribute__((packed));

struct fragment_info *_mbim_message_split_fragments (const MbimMessage *self,
                                                     guint32            max_fragment_size,
                                                     guint             *n_fragments);

G_END_DECLS

#endif /* _LIBMBIM_GLIB_MBIM_MESSAGE_PRIVATE_H_ */
