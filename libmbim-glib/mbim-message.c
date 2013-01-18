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
 */

#include <glib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <endian.h>

#include "mbim-message.h"
#include "mbim-message-private.h"
#include "mbim-error-types.h"

/**
 * SECTION:mbim-message
 * @title: MbimMessage
 * @short_description: Generic MBIM message handling routines
 *
 * #MbimMessage is a generic type representing a MBIM message of any kind
 * (request, response, indication).
 **/

/*****************************************************************************/

GType
mbim_message_get_type (void)
{
    static volatile gsize g_define_type_id__volatile = 0;

    if (g_once_init_enter (&g_define_type_id__volatile)) {
        GType g_define_type_id =
            g_boxed_type_register_static (g_intern_static_string ("MbimMessage"),
                                          (GBoxedCopyFunc) mbim_message_ref,
                                          (GBoxedFreeFunc) mbim_message_unref);

        g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
    }

    return g_define_type_id__volatile;
}

/*****************************************************************************/

static GByteArray *
_mbim_message_allocate (MbimMessageType message_type,
                        guint32         transaction_id,
                        guint32         additional_size)
{
    GByteArray *self;
    guint32 len;

    /* Compute size of the basic empty message and allocate heap for it */
    len = sizeof (struct header) + additional_size;
    self = g_byte_array_sized_new (len);
    g_byte_array_set_size (self, len);

    /* Set MBIM header */
    ((struct header *)(self->data))->type           = GUINT32_TO_LE (message_type);
    ((struct header *)(self->data))->length         = GUINT32_TO_LE (len);
    ((struct header *)(self->data))->transaction_id = GUINT32_TO_LE (transaction_id);

    return self;
}

/*****************************************************************************/
/* Generic message interface */

/**
 * mbim_message_ref:
 * @self: a #MbimMessage.
 *
 * Atomically increments the reference count of @self by one.
 *
 * Returns: (transfer full) the new reference to @self.
 */
MbimMessage *
mbim_message_ref (MbimMessage *self)
{
    g_return_val_if_fail (self != NULL, NULL);

    return (MbimMessage *) g_byte_array_ref (self);
}

/**
 * mbim_message_unref:
 * @self: a #MbimMessage.
 *
 * Atomically decrements the reference count of @self by one.
 * If the reference count drops to 0, @self is completely disposed.
 */
void
mbim_message_unref (MbimMessage *self)
{
    g_return_if_fail (self != NULL);

    g_byte_array_unref (self);
}

/**
 * mbim_message_get_message_type:
 * @self: a #MbimMessage.
 *
 * Gets the message type.
 *
 * Returns: a #MbimMessageType.
 */
MbimMessageType
mbim_message_get_message_type (const MbimMessage *self)
{
    g_return_val_if_fail (self != NULL, MBIM_MESSAGE_TYPE_INVALID);

    return MBIM_MESSAGE_GET_MESSAGE_TYPE (self);
}

/**
 * mbim_message_get_message_length:
 * @self: a #MbimMessage.
 *
 * Gets the whole message length.
 *
 * Returns: the length of the message.
 */
guint32
mbim_message_get_message_length (const MbimMessage *self)
{
    g_return_val_if_fail (self != NULL, 0);

    return MBIM_MESSAGE_GET_MESSAGE_LENGTH (self);
}

/**
 * mbim_message_get_transaction_id:
 * @self: a #MbimMessage.
 *
 * Gets the transaction ID of the message.
 *
 * Returns: the transaction ID.
 */
guint32
mbim_message_get_transaction_id (const MbimMessage *self)
{
    g_return_val_if_fail (self != NULL, 0);

    return MBIM_MESSAGE_GET_TRANSACTION_ID (self);
}

/**
 * mbim_message_new:
 * @data: contents of the message.
 * @data_length: length of the message.
 *
 * Create a #MbimMessage with the given contents.
 *
 * Returns: (transfer full): a newly created #MbimMessage, which should be freed with mbim_message_unref().
 */
MbimMessage *
mbim_message_new (const guint8 *data,
                  guint32       data_length)
{
    MbimMessage *out;

    /* Create output MbimMessage */
    out = g_byte_array_sized_new (data_length);
    g_byte_array_append (out, data, data_length);

    return out;
}

/**
 * mbim_message_dup:
 * @self: a #MbimMessage to duplicate.
 *
 * Create a #MbimMessage with the same contents as @self.
 *
 * Returns: (transfer full): a newly created #MbimMessage, which should be freed with mbim_message_unref().
 */
MbimMessage *
mbim_message_dup (const MbimMessage *self)
{
    g_return_val_if_fail (self != NULL, NULL);

    return mbim_message_new (((GByteArray *)self)->data,
                             MBIM_MESSAGE_GET_MESSAGE_LENGTH (self));
}

/**
 * mbim_message_get_raw:
 * @self: a #MbimMessage.
 * @length: (out): return location for the size of the output buffer.
 * @error: return location for error or %NULL.
 *
 * Gets the whole raw data buffer of the #MbimMessage.
 *
 * Returns: (transfer none): The raw data buffer, or #NULL if @error is set.
 */
const guint8 *
mbim_message_get_raw (const MbimMessage  *self,
                      guint32            *length,
                      GError            **error)
{
    g_return_val_if_fail (self != NULL, NULL);
    g_return_val_if_fail (length != NULL, NULL);

    if (!self->data || !self->len) {
        g_set_error (error,
                     MBIM_CORE_ERROR,
                     MBIM_CORE_ERROR_FAILED,
                     "Message is empty");
        return NULL;
    }

    *length = (guint32) self->len;
    return self->data;
}

/**
 * mbim_message_get_printable:
 * @self: a #MbimMessage.
 * @line_prefix: prefix string to use in each new generated line.
 *
 * Gets a printable string with the contents of the whole MBIM message.
 *
 * Returns: (transfer full): a newly allocated string, which should be freed with g_free().
 */
gchar *
mbim_message_get_printable (const MbimMessage *self,
                            const gchar       *line_prefix)
{
    GString *printable;

    g_return_val_if_fail (self != NULL, NULL);
    g_return_val_if_fail (line_prefix != NULL, NULL);

    if (!line_prefix)
        line_prefix = "";

    printable = g_string_new ("");
    g_string_append_printf (printable,
                            "%sHeader:\n"
                            "%s  length      = %u\n"
                            "%s  type        = 0x%08x\n"
                            "%s  transaction = %u\n",
                            line_prefix,
                            line_prefix, MBIM_MESSAGE_GET_MESSAGE_LENGTH (self),
                            line_prefix, MBIM_MESSAGE_GET_MESSAGE_TYPE (self),
                            line_prefix, MBIM_MESSAGE_GET_TRANSACTION_ID (self));

    switch (MBIM_MESSAGE_GET_MESSAGE_TYPE (self)) {
    case MBIM_MESSAGE_TYPE_INVALID:
        g_warn_if_reached ();
        break;

    case MBIM_MESSAGE_TYPE_OPEN:
    case MBIM_MESSAGE_TYPE_CLOSE:
    case MBIM_MESSAGE_TYPE_OPEN_DONE:
    case MBIM_MESSAGE_TYPE_CLOSE_DONE:
    case MBIM_MESSAGE_TYPE_HOST_ERROR:
    case MBIM_MESSAGE_TYPE_FUNCTION_ERROR:
    case MBIM_MESSAGE_TYPE_COMMAND:
    case MBIM_MESSAGE_TYPE_COMMAND_DONE:
    case MBIM_MESSAGE_TYPE_INDICATION:
        break;
    }

    return g_string_free (printable, FALSE);
}
