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
 * Copyright (C) 2013 - 2018 Aleksander Morgado <aleksander@aleksander.es>
 */

#include <glib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <endian.h>

#include "mbim-message.h"
#include "mbim-message-private.h"
#include "mbim-error-types.h"
#include "mbim-enum-types.h"

#include "mbim-basic-connect.h"
#include "mbim-auth.h"
#include "mbim-dss.h"
#include "mbim-phonebook.h"
#include "mbim-sms.h"
#include "mbim-stk.h"
#include "mbim-ussd.h"
#include "mbim-proxy-control.h"
#include "mbim-qmi.h"
#include "mbim-ms-firmware-id.h"
#include "mbim-ms-host-shutdown.h"
#include "mbim-ms-sar.h"
#include "mbim-atds.h"
#include "mbim-intel-firmware-update.h"
#include "mbim-qdu.h"
#include "mbim-ms-basic-connect-extensions.h"
#include "mbim-ms-uicc-low-level-access.h"

/*****************************************************************************/

static void
bytearray_apply_padding (GByteArray *buffer,
                         guint32    *len)
{
    static const guint8 padding = 0;

    g_assert (buffer);
    g_assert (len);

    /* Apply padding to the requested length until multiple of 4 */
    while (*len % 4 != 0) {
        g_byte_array_append (buffer, &padding, 1);
        (*len)++;
    }
}

static void
set_error_from_status (GError          **error,
                       MbimStatusError   status)
{
    const gchar *error_string;

    error_string = mbim_status_error_get_string (status);
    if (error_string)
        g_set_error_literal (error,
                             MBIM_STATUS_ERROR,
                             status,
                             error_string);
    else
        g_set_error (error,
                     MBIM_STATUS_ERROR,
                     status,
                     "Unknown status 0x%08x",
                     status);
}

/*****************************************************************************/

GType
mbim_message_get_type (void)
{
    static gsize g_define_type_id_initialized = 0;

    if (g_once_init_enter (&g_define_type_id_initialized)) {
        GType g_define_type_id =
            g_boxed_type_register_static (g_intern_static_string ("MbimMessage"),
                                          (GBoxedCopyFunc) mbim_message_ref,
                                          (GBoxedFreeFunc) mbim_message_unref);

        g_once_init_leave (&g_define_type_id_initialized, g_define_type_id);
    }

    return g_define_type_id_initialized;
}

/*****************************************************************************/

GByteArray *
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

static guint32
_mbim_message_get_information_buffer_offset (const MbimMessage *self)
{
    g_return_val_if_fail (MBIM_MESSAGE_GET_MESSAGE_TYPE (self) == MBIM_MESSAGE_TYPE_COMMAND ||
                          MBIM_MESSAGE_GET_MESSAGE_TYPE (self) == MBIM_MESSAGE_TYPE_COMMAND_DONE ||
                          MBIM_MESSAGE_GET_MESSAGE_TYPE (self) == MBIM_MESSAGE_TYPE_INDICATE_STATUS, 0);

    switch (MBIM_MESSAGE_GET_MESSAGE_TYPE (self)) {
    case MBIM_MESSAGE_TYPE_COMMAND:
        return (sizeof (struct header) +
                G_STRUCT_OFFSET (struct command_message, buffer));

    case MBIM_MESSAGE_TYPE_COMMAND_DONE:
        return (sizeof (struct header) +
                G_STRUCT_OFFSET (struct command_done_message, buffer));
        break;

    case MBIM_MESSAGE_TYPE_INDICATE_STATUS:
        return (sizeof (struct header) +
                G_STRUCT_OFFSET (struct indicate_status_message, buffer));
        break;

    case MBIM_MESSAGE_TYPE_INVALID:
    case MBIM_MESSAGE_TYPE_OPEN:
    case MBIM_MESSAGE_TYPE_CLOSE:
    case MBIM_MESSAGE_TYPE_HOST_ERROR:
    case MBIM_MESSAGE_TYPE_OPEN_DONE:
    case MBIM_MESSAGE_TYPE_CLOSE_DONE:
    case MBIM_MESSAGE_TYPE_FUNCTION_ERROR:
    default:
        g_assert_not_reached ();
        return 0;
    }
}

gboolean
_mbim_message_read_guint32 (const MbimMessage  *self,
                            guint32             relative_offset,
                            guint32            *value,
                            GError            **error)
{
    guint64 required_size;
    guint32 information_buffer_offset;

    g_assert (value);

    information_buffer_offset = _mbim_message_get_information_buffer_offset (self);

    required_size = (guint64)information_buffer_offset + (guint64)relative_offset + 4;
    if ((guint64)self->len < required_size) {
        g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_INVALID_MESSAGE,
                     "cannot read 32bit unsigned integer (4 bytes) (%u < %" G_GUINT64_FORMAT ")",
                     self->len, required_size);
        return FALSE;
    }

    *value = GUINT32_FROM_LE (G_STRUCT_MEMBER (
                                  guint32,
                                  self->data,
                                  (information_buffer_offset + relative_offset)));
    return TRUE;
}

gboolean
_mbim_message_read_guint32_array (const MbimMessage  *self,
                                  guint32             array_size,
                                  guint32             relative_offset_array_start,
                                  guint32           **array,
                                  GError            **error)
{
    guint64 required_size;
    guint   i;
    guint32 information_buffer_offset;

    g_assert (array != NULL);

    if (!array_size) {
        *array = NULL;
        return TRUE;
    }

    information_buffer_offset = _mbim_message_get_information_buffer_offset (self);

    required_size = (guint64)information_buffer_offset + (guint64)relative_offset_array_start + (4 * (guint64)array_size);
    if ((guint64)self->len < required_size) {
        g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_INVALID_MESSAGE,
                     "cannot read 32bit unsigned integer array (%" G_GUINT64_FORMAT " bytes) (%u < %" G_GUINT64_FORMAT ")",
                     (4 * (guint64)array_size), self->len, (guint64)required_size);
        return FALSE;
    }

    *array = g_new (guint32, array_size + 1);
    for (i = 0; i < array_size; i++) {
        (*array)[i] = GUINT32_FROM_LE (G_STRUCT_MEMBER (
                                           guint32,
                                           self->data,
                                           (information_buffer_offset +
                                            relative_offset_array_start +
                                            (4 * i))));
    }
    (*array)[array_size] = 0;
    return TRUE;
}

gboolean
_mbim_message_read_guint64 (const MbimMessage  *self,
                            guint32             relative_offset,
                            guint64            *value,
                            GError            **error)
{
    guint64 required_size;
    guint32 information_buffer_offset;

    g_assert (value != NULL);

    information_buffer_offset = _mbim_message_get_information_buffer_offset (self);

    required_size = (guint64)information_buffer_offset + (guint64)relative_offset + 8;
    if ((guint64)self->len < required_size) {
        g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_INVALID_MESSAGE,
                     "cannot read 64bit unsigned integer (8 bytes) (%u < %" G_GUINT64_FORMAT ")",
                     self->len, required_size);
        return FALSE;
    }

    *value = GUINT64_FROM_LE (G_STRUCT_MEMBER (
                                  guint64,
                                  self->data,
                                  (information_buffer_offset + relative_offset)));
    return TRUE;
}

gboolean
_mbim_message_read_string (const MbimMessage  *self,
                           guint32             struct_start_offset,
                           guint32             relative_offset,
                           gchar             **str,
                           GError            **error)
{
    guint64               required_size;
    guint32               offset;
    guint32               size;
    guint32               information_buffer_offset;
    g_autofree gunichar2 *utf16d = NULL;
    const gunichar2      *utf16 = NULL;

    information_buffer_offset = _mbim_message_get_information_buffer_offset (self);

    required_size = (guint64)information_buffer_offset + (guint64)relative_offset + 8;
    if ((guint64)self->len < required_size) {
        g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_INVALID_MESSAGE,
                     "cannot read string offset and size (%u < %" G_GUINT64_FORMAT ")",
                     self->len, required_size);
        return FALSE;
    }

    offset = GUINT32_FROM_LE (G_STRUCT_MEMBER (
                                  guint32,
                                  self->data,
                                  (information_buffer_offset + relative_offset)));
    size = GUINT32_FROM_LE (G_STRUCT_MEMBER (
                                guint32,
                                self->data,
                                (information_buffer_offset + relative_offset + 4)));
    if (!size) {
        *str = NULL;
        return TRUE;
    }

    required_size = (guint64)information_buffer_offset + (guint64)struct_start_offset + (guint64)offset + (guint64)size;
    if ((guint64)self->len < required_size) {
        g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_INVALID_MESSAGE,
                     "cannot read string data (%u bytes) (%u < %" G_GUINT64_FORMAT ")",
                     size, self->len, required_size);
        return FALSE;
    }

    utf16 = (const gunichar2 *) G_STRUCT_MEMBER_P (self->data, (information_buffer_offset + struct_start_offset + offset));

    /* For BE systems, convert from LE to BE */
    if (G_BYTE_ORDER == G_BIG_ENDIAN) {
        guint i;

        utf16d = (gunichar2 *) g_malloc (size);
        for (i = 0; i < (size / 2); i++)
            utf16d[i] = GUINT16_FROM_LE (utf16[i]);
    }

    *str = g_utf16_to_utf8 (utf16d ? utf16d : utf16,
                            size / 2,
                            NULL,
                            NULL,
                            error);

    if (!(*str)) {
        g_prefix_error (error, "Error converting string to UTF-8: ");
        return FALSE;
    }

    return TRUE;
}

gboolean
_mbim_message_read_string_array (const MbimMessage   *self,
                                 guint32              array_size,
                                 guint32              struct_start_offset,
                                 guint32              relative_offset_array_start,
                                 gchar             ***array,
                                 GError             **error)
{
    guint32  offset;
    guint32  i;
    GError  *inner_error = NULL;

    g_assert (array != NULL);

    if (!array_size) {
        *array = NULL;
        return TRUE;
    }

    *array = g_new0 (gchar *, array_size + 1);
    for (i = 0, offset = relative_offset_array_start;
         i < array_size;
         offset += 8, i++) {
        /* Read next string in the OL pair list */
        if (!_mbim_message_read_string (self, struct_start_offset, offset, &((*array)[i]), &inner_error))
            break;
    }

    if (inner_error) {
        g_strfreev (*array);
        g_propagate_error (error, inner_error);
        return FALSE;
    }

    return TRUE;
}

/*
 * Byte arrays may be given in very different ways:
 *  - (a) Offset + Length pair in static buffer, data in variable buffer.
 *  - (b) Just length in static buffer, data just afterwards.
 *  - (c) Just offset in static buffer, length given in another variable, data in variable buffer.
 *  - (d) Fixed-sized array directly in the static buffer.
 *  - (e) Unsized array directly in the variable buffer, length is assumed until end of message.
 */
gboolean
_mbim_message_read_byte_array (const MbimMessage  *self,
                               guint32             struct_start_offset,
                               guint32             relative_offset,
                               gboolean            has_offset,
                               gboolean            has_length,
                               guint32             explicit_array_size,
                               const guint8      **array,
                               guint32            *array_size,
                               GError            **error,
                               gboolean            swapped_offset_length)
{
    guint32 information_buffer_offset;

    information_buffer_offset = _mbim_message_get_information_buffer_offset (self);

    /* (a) Offset + Length pair in static buffer, data in variable buffer. */
    if (has_offset && has_length) {
        guint32 offset;
        guint64 required_size;

        g_assert (array_size != NULL);
        g_assert (explicit_array_size == 0);

        /* requires 8 bytes in relative offset */
        required_size = (guint64)information_buffer_offset + (guint64)relative_offset + 8;
        if ((guint64)self->len < required_size) {
            g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_INVALID_MESSAGE,
                         "cannot read byte array offset and size (%u < %" G_GUINT64_FORMAT ")",
                         self->len, required_size);
            return FALSE;
        }

        if (!swapped_offset_length) {
            /* (b) Offset followed by length encoding format. */
            offset = GUINT32_FROM_LE (G_STRUCT_MEMBER (
                                      guint32,
                                      self->data,
                                      (information_buffer_offset + relative_offset)));
            *array_size = GUINT32_FROM_LE (G_STRUCT_MEMBER (
                                           guint32,
                                           self->data,
                                           (information_buffer_offset + relative_offset + 4)));
        } else {
            /* (b) length followed by offset encoding format. */
            *array_size = GUINT32_FROM_LE (G_STRUCT_MEMBER (
                                           guint32,
                                           self->data,
                                           (information_buffer_offset + relative_offset)));

            offset = GUINT32_FROM_LE (G_STRUCT_MEMBER (
                                      guint32,
                                      self->data,
                                      (information_buffer_offset + relative_offset + 4)));
        }

        /* requires array_size bytes in offset */
        required_size = (guint64)information_buffer_offset + (guint64)struct_start_offset + (guint64)offset + (guint64)(*array_size);
        if ((guint64)self->len < required_size) {
            g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_INVALID_MESSAGE,
                         "cannot read byte array data (%u bytes) (%u < %" G_GUINT64_FORMAT ")",
                         *array_size, self->len, required_size);
            return FALSE;
        }

        *array = (const guint8 *) G_STRUCT_MEMBER_P (self->data,
                                                     (information_buffer_offset + struct_start_offset + offset));
        return TRUE;
    }

    /* (b) Just length in static buffer, data just afterwards. */
    if (!has_offset && has_length) {
        guint64 required_size;

        g_assert (array_size != NULL);
        g_assert (explicit_array_size == 0);

        /* requires 4 bytes in relative offset */
        required_size = (guint64)information_buffer_offset + (guint64)relative_offset + 4;
        if ((guint64)self->len < required_size) {
            g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_INVALID_MESSAGE,
                         "cannot read byte array size (%u < %" G_GUINT64_FORMAT ")",
                         self->len, required_size);
            return FALSE;
        }

        *array_size = GUINT32_FROM_LE (G_STRUCT_MEMBER (
                                           guint32,
                                           self->data,
                                           (information_buffer_offset + relative_offset)));

        /* requires array_size bytes in after the array_size variable */
        required_size += (guint64)(*array_size);
        if ((guint64)self->len < required_size) {
            g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_INVALID_MESSAGE,
                         "cannot read byte array data (%u bytes) (%u < %" G_GUINT64_FORMAT ")",
                         *array_size, self->len, required_size);
            return FALSE;
        }

        *array = (const guint8 *) G_STRUCT_MEMBER_P (self->data,
                                                     (information_buffer_offset + relative_offset + 4));
        return TRUE;
    }

    /* (c) Just offset in static buffer, length given in another variable, data in variable buffer. */
    if (has_offset && !has_length) {
        guint64 required_size;
        guint32 offset;

        g_assert (array_size == NULL);

        /* requires 4 bytes in relative offset */
        required_size = (guint64)information_buffer_offset + (guint64)relative_offset + 4;
        if ((guint64)self->len < required_size) {
            g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_INVALID_MESSAGE,
                         "cannot read byte array offset (%u < %" G_GUINT64_FORMAT ")",
                         self->len, required_size);
            return FALSE;
        }

        offset = GUINT32_FROM_LE (G_STRUCT_MEMBER (
                                      guint32,
                                      self->data,
                                      (information_buffer_offset + relative_offset)));

        /* requires explicit_array_size bytes in offset */
        required_size = (guint64)information_buffer_offset + (guint64)struct_start_offset + (guint64)offset + (guint64)explicit_array_size;
        if ((guint64)self->len < required_size) {
            g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_INVALID_MESSAGE,
                         "cannot read byte array data (%u bytes) (%u < %" G_GUINT64_FORMAT ")",
                         explicit_array_size, self->len, required_size);
            return FALSE;
        }

        *array = (const guint8 *) G_STRUCT_MEMBER_P (self->data,
                                                     (information_buffer_offset + struct_start_offset + offset));
        return TRUE;
    }

    /* (d) Fixed-sized array directly in the static buffer.
     * (e) Unsized array directly in the variable buffer, length is assumed until end of message. */
    if (!has_offset && !has_length) {
        /* If array size is requested, it's case (e) */
        if (array_size) {
            /* no need to validate required size, as it's implicitly validated based on the message
             * length */
            *array_size = self->len - (information_buffer_offset + relative_offset);
        } else {
            guint64 required_size;

            /* requires explicit_array_size bytes in offset */
            required_size = (guint64)information_buffer_offset + (guint64)relative_offset + (guint64)explicit_array_size;
            if ((guint64)self->len < required_size) {
                g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_INVALID_MESSAGE,
                             "cannot read byte array data (%u bytes) (%u < %" G_GUINT64_FORMAT ")",
                             explicit_array_size, self->len, required_size);
                return FALSE;
            }
        }

        *array = (const guint8 *) G_STRUCT_MEMBER_P (self->data,
                                                     (information_buffer_offset + relative_offset));
        return TRUE;
    }

    g_assert_not_reached ();
}

gboolean
_mbim_message_read_uuid (const MbimMessage  *self,
                         guint32             relative_offset,
                         const MbimUuid    **uuid,
                         GError            **error)
{
    guint64 required_size;
    guint32 information_buffer_offset;

    g_assert (uuid);

    information_buffer_offset = _mbim_message_get_information_buffer_offset (self);

    required_size = (guint64)information_buffer_offset + (guint64)relative_offset + 16;
    if ((guint64)self->len < required_size) {
        g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_INVALID_MESSAGE,
                     "cannot read UUID (16 bytes) (%u < %" G_GUINT64_FORMAT ")",
                     self->len, required_size);
        return FALSE;
    }

    *uuid = (const MbimUuid *) G_STRUCT_MEMBER_P (self->data,
                                                  (information_buffer_offset + relative_offset));
    return TRUE;
}

gboolean
_mbim_message_read_ipv4 (const MbimMessage  *self,
                         guint32             relative_offset,
                         gboolean            ref,
                         const MbimIPv4    **ipv4,
                         GError            **error)
{
    guint64 required_size;
    guint32 information_buffer_offset;
    guint32 offset;

    g_assert (ipv4 != NULL);

    information_buffer_offset = _mbim_message_get_information_buffer_offset (self);

    if (ref) {
        required_size = (guint64)information_buffer_offset + (guint64)relative_offset + 4;
        if ((guint64)self->len < required_size) {
            g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_INVALID_MESSAGE,
                         "cannot read IPv4 offset (4 bytes) (%u < %" G_GUINT64_FORMAT ")",
                         self->len, required_size);
            return FALSE;
        }

        offset = GUINT32_FROM_LE (G_STRUCT_MEMBER (guint32,
                                                   self->data,
                                                   (information_buffer_offset + relative_offset)));
        if (!offset) {
            *ipv4 = NULL;
            return TRUE;
        }
    } else
        offset = relative_offset;

    required_size = (guint64)information_buffer_offset + (guint64)offset + 4;
    if ((guint64)self->len < required_size) {
        g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_INVALID_MESSAGE,
                     "cannot read IPv4 (4 bytes) (%u < %" G_GUINT64_FORMAT ")",
                     self->len, required_size);
        return FALSE;
    }

    *ipv4 = (const MbimIPv4 *) G_STRUCT_MEMBER_P (self->data,
                                                  (information_buffer_offset + offset));
    return TRUE;
}

gboolean
_mbim_message_read_ipv4_array (const MbimMessage  *self,
                               guint32             array_size,
                               guint32             relative_offset_array_start,
                               MbimIPv4          **array,
                               GError            **error)
{
    guint64 required_size;
    guint32 offset;
    guint32 i;
    guint32 information_buffer_offset;

    g_assert (array != NULL);

    if (!array_size) {
        *array = NULL;
        return TRUE;
    }

    information_buffer_offset = _mbim_message_get_information_buffer_offset (self);

    required_size = (guint64)information_buffer_offset + (guint64)relative_offset_array_start + 4;
    if ((guint64)self->len < required_size) {
        g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_INVALID_MESSAGE,
                     "cannot read IPv4 array offset (4 bytes) (%u < %" G_GUINT64_FORMAT ")",
                     self->len, required_size);
        return FALSE;
    }

    offset = GUINT32_FROM_LE (G_STRUCT_MEMBER (
                                  guint32,
                                  self->data,
                                  (information_buffer_offset + relative_offset_array_start)));

    required_size = (guint64)information_buffer_offset + (guint64)offset + (4 * (guint64)array_size);
    if ((guint64)self->len < required_size) {
        g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_INVALID_MESSAGE,
                     "cannot read IPv4 array data (%" G_GUINT64_FORMAT " bytes) (%u < %" G_GUINT64_FORMAT ")",
                     (4 * (guint64)array_size), self->len, required_size);
        return FALSE;
    }

    *array = g_new (MbimIPv4, array_size);
    for (i = 0; i < array_size; i++, offset += 4) {
        memcpy (&((*array)[i]),
                G_STRUCT_MEMBER_P (self->data,
                                   (information_buffer_offset + offset)),
                4);
    }

    return TRUE;
}

gboolean
_mbim_message_read_ipv6 (const MbimMessage  *self,
                         guint32             relative_offset,
                         gboolean            ref,
                         const MbimIPv6    **ipv6,
                         GError            **error)
{
    guint64 required_size;
    guint32 information_buffer_offset;
    guint32 offset;

    g_assert (ipv6 != NULL);

    information_buffer_offset = _mbim_message_get_information_buffer_offset (self);

    if (ref) {
        required_size = (guint64)information_buffer_offset + (guint64)relative_offset + 4;
        if ((guint64)self->len < required_size) {
            g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_INVALID_MESSAGE,
                         "cannot read IPv6 offset (4 bytes) (%u < %" G_GUINT64_FORMAT ")",
                         self->len, required_size);
            return FALSE;
        }

        offset = GUINT32_FROM_LE (G_STRUCT_MEMBER (guint32,
                                                   self->data,
                                                   (information_buffer_offset + relative_offset)));
        if (!offset) {
            *ipv6 = NULL;
            return TRUE;
        }
    } else
        offset = relative_offset;

    required_size = (guint64)information_buffer_offset + (guint64)offset + 16;
    if ((guint64)self->len < required_size) {
        g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_INVALID_MESSAGE,
                     "cannot read IPv6 (16 bytes) (%u < %" G_GUINT64_FORMAT ")",
                     self->len, required_size);
        return FALSE;
    }

    *ipv6 = (const MbimIPv6 *) G_STRUCT_MEMBER_P (self->data,
                                                  (information_buffer_offset + offset));
    return TRUE;
}

gboolean
_mbim_message_read_ipv6_array (const MbimMessage  *self,
                               guint32             array_size,
                               guint32             relative_offset_array_start,
                               MbimIPv6          **array,
                               GError            **error)
{
    guint64 required_size;
    guint32 offset;
    guint32 i;
    guint32 information_buffer_offset;

    g_assert (array != NULL);

    if (!array_size) {
        *array = NULL;
        return TRUE;
    }

    information_buffer_offset = _mbim_message_get_information_buffer_offset (self);

    required_size = (guint64)information_buffer_offset + (guint64)relative_offset_array_start + 4;
    if ((guint64)self->len < required_size) {
        g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_INVALID_MESSAGE,
                     "cannot read IPv6 array offset (4 bytes) (%u < %" G_GUINT64_FORMAT ")",
                     self->len, required_size);
        return FALSE;
    }

    offset = GUINT32_FROM_LE (G_STRUCT_MEMBER (
                                  guint32,
                                  self->data,
                                  (information_buffer_offset + relative_offset_array_start)));

    required_size = (guint64)information_buffer_offset + (guint64)offset + (16 * (guint64)array_size);
    if ((guint64)self->len < required_size) {
        g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_INVALID_MESSAGE,
                     "cannot read IPv6 array data (%" G_GUINT64_FORMAT " bytes) (%u < %" G_GUINT64_FORMAT ")",
                     (16 * (guint64)array_size), self->len, required_size);
        return FALSE;
    }

    *array = g_new (MbimIPv6, array_size);
    for (i = 0; i < array_size; i++, offset += 16) {
        memcpy (&((*array)[i]),
                G_STRUCT_MEMBER_P (self->data,
                                   (information_buffer_offset + offset)),
                16);
    }

    return TRUE;
}

/*****************************************************************************/
/* Struct builder interface
 *
 * Types like structs consist of a fixed sized prefix plus a variable length
 * data buffer. Items of variable size are usually given as an offset (with
 * respect to the start of the struct) plus a size field. */

MbimStructBuilder *
_mbim_struct_builder_new (void)
{
    MbimStructBuilder *builder;

    builder = g_slice_new (MbimStructBuilder);
    builder->fixed_buffer = g_byte_array_new ();
    builder->variable_buffer = g_byte_array_new ();
    builder->offsets = g_array_new (FALSE, FALSE, sizeof (guint32));
    return builder;
}

GByteArray *
_mbim_struct_builder_complete (MbimStructBuilder *builder)
{
    GByteArray *out;
    guint i;

    /* Update offsets with the length of the information buffer, and store them
     * in LE. */
    for (i = 0; i < builder->offsets->len; i++) {
        guint32 offset_offset;
        guint32 offset_value;

        offset_offset = g_array_index (builder->offsets, guint32, i);
        memcpy (&offset_value, &(builder->fixed_buffer->data[offset_offset]), sizeof (guint32));
        offset_value = GUINT32_TO_LE (offset_value + builder->fixed_buffer->len);
        memcpy (&(builder->fixed_buffer->data[offset_offset]), &offset_value, sizeof (guint32));
    }

    /* Merge both buffers */
    g_byte_array_append (builder->fixed_buffer,
                         (const guint8 *)builder->variable_buffer->data,
                         (guint32)builder->variable_buffer->len);

    /* Steal the buffer to return */
    out = builder->fixed_buffer;

    /* Dispose the builder */
    g_array_unref (builder->offsets);
    g_byte_array_unref (builder->variable_buffer);
    g_slice_free (MbimStructBuilder, builder);

    return out;
}

/*
 * Byte arrays may be given in very different ways:
 *  - (a) Offset + Length pair in static buffer, data in variable buffer.
 *  - (b) Just length in static buffer, data just afterwards.
 *  - (c) Just offset in static buffer, length given in another variable, data in variable buffer.
 *  - (d) Fixed-sized array directly in the static buffer.
 *  - (e) Unsized array directly in the variable buffer, length is assumed until end of message.
 */
void
_mbim_struct_builder_append_byte_array (MbimStructBuilder *builder,
                                        gboolean           with_offset,
                                        gboolean           with_length,
                                        gboolean           pad_buffer,
                                        const guint8      *buffer,
                                        guint32            buffer_len,
                                        gboolean           swapped_offset_length)
{
    /*
     * (d) Fixed-sized array directly in the static buffer.
     * (e) Unsized array directly in the variable buffer (here end of static buffer is also beginning of variable)
     */
    if (!with_offset && !with_length) {
        g_byte_array_append (builder->fixed_buffer, buffer, buffer_len);
        if (pad_buffer)
            bytearray_apply_padding (builder->fixed_buffer, &buffer_len);
        return;
    }

    /* (a) Offset + Length pair in static buffer, data in variable buffer.
     * This case is the sum of cases b+c */

    /* (b) Just length in static buffer, data just afterwards. */
    if (swapped_offset_length && with_length) {
        guint32 length;

        /* Add the length value in beginning and offset in the end later*/
        length = GUINT32_TO_LE (buffer_len);
        g_byte_array_append (builder->fixed_buffer, (guint8 *)&length, sizeof (length));
    }

    /* (c) Just offset in static buffer, length given in another variable, data in variable buffer. */

    if (with_offset) {
        guint32 offset;

        /* If string length is greater than 0, add the offset to fix, otherwise set
         * the offset to 0 and don't configure the update */
        if (buffer_len == 0) {
            offset = 0;
            g_byte_array_append (builder->fixed_buffer, (guint8 *)&offset, sizeof (offset));
        } else {
            guint32 offset_offset;

            /* Offset of the offset */
            offset_offset = builder->fixed_buffer->len;

            /* Length *not* in LE yet */
            offset = builder->variable_buffer->len;
            /* Add the offset value */
            g_byte_array_append (builder->fixed_buffer, (guint8 *)&offset, sizeof (offset));
            /* Configure the value to get updated */
            g_array_append_val (builder->offsets, offset_offset);
        }
    }

    /* (b) Just length in static buffer, data just afterwards. */
    if (!swapped_offset_length && with_length) {
        guint32 length;

        /* Add the length value */
        length = GUINT32_TO_LE (buffer_len);
        g_byte_array_append (builder->fixed_buffer, (guint8 *)&length, sizeof (length));
    }

    /* And finally, the bytearray itself to the variable buffer */
    if (buffer_len) {
        g_byte_array_append (builder->variable_buffer, (const guint8 *)buffer, (guint)buffer_len);

        /* Note: adding zero padding causes trouble for QMI service */
        if (pad_buffer)
            bytearray_apply_padding (builder->variable_buffer, &buffer_len);
    }
}

void
_mbim_struct_builder_append_uuid (MbimStructBuilder *builder,
                                  const MbimUuid    *value)
{
    static const MbimUuid uuid_invalid = {
        .a = { 0x00, 0x00, 0x00, 0x00 },
        .b = { 0x00, 0x00 },
        .c = { 0x00, 0x00 },
        .d = { 0x00, 0x00 },
        .e = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
    };

    /* uuids are added in the static buffer only */
    g_byte_array_append (builder->fixed_buffer,
                         value ? (guint8 *)value : (guint8 *)&uuid_invalid,
                         sizeof (MbimUuid));
}

void
_mbim_struct_builder_append_guint32 (MbimStructBuilder *builder,
                                     guint32            value)
{
    guint32 tmp;

    /* guint32 values are added in the static buffer only */
    tmp = GUINT32_TO_LE (value);
    g_byte_array_append (builder->fixed_buffer, (guint8 *)&tmp, sizeof (tmp));
}

void
_mbim_struct_builder_append_guint32_array (MbimStructBuilder *builder,
                                           const guint32     *values,
                                           guint32            n_values)
{
    guint i;

    /* guint32 array added directly in the static buffer */
    for (i = 0; i < n_values; i++)
        _mbim_struct_builder_append_guint32 (builder, values[i]);
}

void
_mbim_struct_builder_append_guint64 (MbimStructBuilder *builder,
                                     guint64            value)
{
    guint64 tmp;

    /* guint64 values are added in the static buffer only */
    tmp = GUINT64_TO_LE (value);
    g_byte_array_append (builder->fixed_buffer, (guint8 *)&tmp, sizeof (tmp));
}

void
_mbim_struct_builder_append_string (MbimStructBuilder *builder,
                                    const gchar       *value)
{
    guint32               offset;
    guint32               length;
    g_autofree gunichar2 *utf16 = NULL;
    guint32               utf16_bytes = 0;

    /* A string consists of Offset+Size in the static buffer, plus the
     * string itself in the variable buffer */

    /* Convert the string from UTF-8 to UTF-16HE */
    if (value && value[0]) {
        g_autoptr(GError) error = NULL;
        glong             items_written = 0;

        utf16 = g_utf8_to_utf16 (value,
                                 -1,
                                 NULL, /* bytes */
                                 &items_written, /* gunichar2 */
                                 &error);
        if (!utf16) {
            g_warning ("Error converting string: %s", error->message);
            return;
        }

        utf16_bytes = items_written * 2;
    }

    /* If string length is greater than 0, add the offset to fix, otherwise set
     * the offset to 0 and don't configure the update */
    if (utf16_bytes == 0) {
        offset = 0;
        g_byte_array_append (builder->fixed_buffer, (guint8 *)&offset, sizeof (offset));
    } else {
        guint32 offset_offset;

        /* Offset of the offset */
        offset_offset = builder->fixed_buffer->len;

        /* Length *not* in LE yet */
        offset = builder->variable_buffer->len;
        /* Add the offset value */
        g_byte_array_append (builder->fixed_buffer, (guint8 *)&offset, sizeof (offset));
        /* Configure the value to get updated */
        g_array_append_val (builder->offsets, offset_offset);
    }

    /* Add the length value */
    length = GUINT32_TO_LE (utf16_bytes);
    g_byte_array_append (builder->fixed_buffer, (guint8 *)&length, sizeof (length));

    /* And finally, the string itself to the variable buffer */
    if (utf16_bytes) {
        /* For BE systems, convert from BE to LE */
        if (G_BYTE_ORDER == G_BIG_ENDIAN) {
            guint i;

            for (i = 0; i < (utf16_bytes / 2); i++)
                utf16[i] = GUINT16_TO_LE (utf16[i]);
        }
        g_byte_array_append (builder->variable_buffer, (const guint8 *)utf16, (guint)utf16_bytes);
        bytearray_apply_padding (builder->variable_buffer, &utf16_bytes);
    }
}

void
_mbim_struct_builder_append_string_array (MbimStructBuilder  *builder,
                                          const gchar *const *values,
                                          guint32             n_values)
{
    /* TODO */
    g_warn_if_reached ();
}

void
_mbim_struct_builder_append_ipv4 (MbimStructBuilder *builder,
                                  const MbimIPv4    *value,
                                  gboolean           ref)
{
    if (ref)
        _mbim_struct_builder_append_ipv4_array (builder, value, value ? 1 : 0);
    else
        g_byte_array_append (builder->fixed_buffer, (guint8 *)value, sizeof (MbimIPv4));
}

void
_mbim_struct_builder_append_ipv4_array (MbimStructBuilder *builder,
                                        const MbimIPv4    *values,
                                        guint32            n_values)
{
    guint32 offset;

    if (!n_values) {
        offset = 0;
        g_byte_array_append (builder->fixed_buffer, (guint8 *)&offset, sizeof (offset));
    } else {
        guint32 offset_offset;

        /* Offset of the offset */
        offset_offset = builder->fixed_buffer->len;

        /* Length *not* in LE yet */
        offset = builder->variable_buffer->len;
        /* Add the offset value */
        g_byte_array_append (builder->fixed_buffer, (guint8 *)&offset, sizeof (offset));
        /* Configure the value to get updated */
        g_array_append_val (builder->offsets, offset_offset);

        /* NOTE: length of the array must be given in a separate variable */

        /* And finally, the array of IPs itself to the variable buffer */
        g_byte_array_append (builder->variable_buffer, (guint8 *)values, n_values * sizeof (MbimIPv4));
    }
}

void
_mbim_struct_builder_append_ipv6 (MbimStructBuilder *builder,
                                  const MbimIPv6    *value,
                                  gboolean           ref)
{
    if (ref)
        _mbim_struct_builder_append_ipv6_array (builder, value, value ? 1 : 0);
    else
        g_byte_array_append (builder->fixed_buffer, (guint8 *)value, sizeof (MbimIPv6));
}

void
_mbim_struct_builder_append_ipv6_array (MbimStructBuilder *builder,
                                        const MbimIPv6    *values,
                                        guint32            n_values)
{
    guint32 offset;

    if (!n_values) {
        offset = 0;
        g_byte_array_append (builder->fixed_buffer, (guint8 *)&offset, sizeof (offset));
    } else {
        guint32 offset_offset;

        /* Offset of the offset */
        offset_offset = builder->fixed_buffer->len;

        /* Length *not* in LE yet */
        offset = builder->variable_buffer->len;
        /* Add the offset value */
        g_byte_array_append (builder->fixed_buffer, (guint8 *)&offset, sizeof (offset));
        /* Configure the value to get updated */
        g_array_append_val (builder->offsets, offset_offset);

        /* NOTE: length of the array must be given in a separate variable */

        /* And finally, the array of IPs itself to the variable buffer */
        g_byte_array_append (builder->variable_buffer, (guint8 *)values, n_values * sizeof (MbimIPv6));
    }
}

/*****************************************************************************/
/* Command message builder interface */

MbimMessageCommandBuilder *
_mbim_message_command_builder_new (guint32                transaction_id,
                                   MbimService            service,
                                   guint32                cid,
                                   MbimMessageCommandType command_type)
{
    MbimMessageCommandBuilder *builder;

    builder = g_slice_new (MbimMessageCommandBuilder);
    builder->message = mbim_message_command_new (transaction_id, service, cid, command_type);
    builder->contents_builder = _mbim_struct_builder_new ();
    return builder;
}

MbimMessage *
_mbim_message_command_builder_complete (MbimMessageCommandBuilder *builder)
{
    MbimMessage *message;
    GByteArray *contents;

    /* Complete contents, which disposes the builder itself */
    contents = _mbim_struct_builder_complete (builder->contents_builder);

    /* Merge both buffers */
    mbim_message_command_append (builder->message,
                                 (const guint8 *)contents->data,
                                 (guint32)contents->len);
    g_byte_array_unref (contents);

    /* Steal the message to return */
    message = builder->message;

    /* Dispose the remaining stuff from the message builder */
    g_slice_free (MbimMessageCommandBuilder, builder);

    return message;
}

void
_mbim_message_command_builder_append_byte_array (MbimMessageCommandBuilder *builder,
                                                 gboolean                   with_offset,
                                                 gboolean                   with_length,
                                                 gboolean                   pad_buffer,
                                                 const guint8              *buffer,
                                                 guint32                    buffer_len,
                                                 gboolean                   swapped_offset_length)
{
    _mbim_struct_builder_append_byte_array (builder->contents_builder, with_offset, with_length, pad_buffer, buffer, buffer_len, swapped_offset_length);
}

void
_mbim_message_command_builder_append_uuid (MbimMessageCommandBuilder *builder,
                                           const MbimUuid            *value)
{
    _mbim_struct_builder_append_uuid (builder->contents_builder, value);
}

void
_mbim_message_command_builder_append_guint32 (MbimMessageCommandBuilder *builder,
                                              guint32                    value)
{
    _mbim_struct_builder_append_guint32 (builder->contents_builder, value);
}

void
_mbim_message_command_builder_append_guint32_array (MbimMessageCommandBuilder *builder,
                                                    const guint32             *values,
                                                    guint32                    n_values)
{
    _mbim_struct_builder_append_guint32_array (builder->contents_builder, values, n_values);
}

void
_mbim_message_command_builder_append_guint64 (MbimMessageCommandBuilder *builder,
                                              guint64                    value)
{
    _mbim_struct_builder_append_guint64 (builder->contents_builder, value);
}

void
_mbim_message_command_builder_append_string (MbimMessageCommandBuilder *builder,
                                             const gchar               *value)
{
    _mbim_struct_builder_append_string (builder->contents_builder, value);
}

void
_mbim_message_command_builder_append_string_array (MbimMessageCommandBuilder *builder,
                                                   const gchar *const        *values,
                                                   guint32                    n_values)
{
    _mbim_struct_builder_append_string_array (builder->contents_builder, values, n_values);
}

void
_mbim_message_command_builder_append_ipv4 (MbimMessageCommandBuilder *builder,
                                           const MbimIPv4            *value,
                                           gboolean                   ref)
{
    _mbim_struct_builder_append_ipv4 (builder->contents_builder, value, ref);
}

void
_mbim_message_command_builder_append_ipv4_array (MbimMessageCommandBuilder *builder,
                                                 const MbimIPv4            *values,
                                                 guint32                    n_values)
{
    _mbim_struct_builder_append_ipv4_array (builder->contents_builder, values, n_values);
}

void
_mbim_message_command_builder_append_ipv6 (MbimMessageCommandBuilder *builder,
                                           const MbimIPv6            *value,
                                           gboolean                   ref)
{
    _mbim_struct_builder_append_ipv6 (builder->contents_builder, value, ref);
}

void
_mbim_message_command_builder_append_ipv6_array (MbimMessageCommandBuilder *builder,
                                                 const MbimIPv6            *values,
                                                 guint32                    n_values)
{
    _mbim_struct_builder_append_ipv6_array (builder->contents_builder, values, n_values);
}

/*****************************************************************************/
/* Generic message interface */

MbimMessage *
mbim_message_ref (MbimMessage *self)
{
    g_return_val_if_fail (self != NULL, NULL);

    return (MbimMessage *) g_byte_array_ref ((GByteArray *)self);
}

void
mbim_message_unref (MbimMessage *self)
{
    g_return_if_fail (self != NULL);

    g_byte_array_unref ((GByteArray *)self);
}

MbimMessageType
mbim_message_get_message_type (const MbimMessage *self)
{
    g_return_val_if_fail (self != NULL, MBIM_MESSAGE_TYPE_INVALID);

    return MBIM_MESSAGE_GET_MESSAGE_TYPE (self);
}

guint32
mbim_message_get_message_length (const MbimMessage *self)
{
    g_return_val_if_fail (self != NULL, 0);

    return MBIM_MESSAGE_GET_MESSAGE_LENGTH (self);
}

guint32
mbim_message_get_transaction_id (const MbimMessage *self)
{
    g_return_val_if_fail (self != NULL, 0);

    return MBIM_MESSAGE_GET_TRANSACTION_ID (self);
}

void
mbim_message_set_transaction_id (MbimMessage *self,
                                 guint32      transaction_id)
{
    g_return_if_fail (self != NULL);

    ((struct header *)(self->data))->transaction_id = GUINT32_TO_LE (transaction_id);
}

MbimMessage *
mbim_message_new (const guint8 *data,
                  guint32       data_length)
{
    GByteArray *out;

    /* Create output MbimMessage */
    out = g_byte_array_sized_new (data_length);
    g_byte_array_append (out, data, data_length);

    return (MbimMessage *)out;
}

MbimMessage *
mbim_message_dup (const MbimMessage *self)
{
    g_return_val_if_fail (self != NULL, NULL);

    return mbim_message_new (((GByteArray *)self)->data,
                             MBIM_MESSAGE_GET_MESSAGE_LENGTH (self));
}

const guint8 *
mbim_message_get_raw (const MbimMessage  *self,
                      guint32            *length,
                      GError            **error)
{
    g_return_val_if_fail (self != NULL, NULL);
    g_return_val_if_fail (length != NULL, NULL);

    if (!self->data || !self->len) {
        g_set_error_literal (error,
                             MBIM_CORE_ERROR,
                             MBIM_CORE_ERROR_FAILED,
                             "Message is empty");
        return NULL;
    }

    *length = (guint32) self->len;
    return self->data;
}

gchar *
mbim_message_get_printable (const MbimMessage *self,
                            const gchar       *line_prefix,
                            gboolean           headers_only)
{
    GString *printable;
    MbimService service_read_fields = MBIM_SERVICE_INVALID;

    g_return_val_if_fail (self != NULL, NULL);
    g_return_val_if_fail (line_prefix != NULL, NULL);

    if (!line_prefix)
        line_prefix = "";

    printable = g_string_new ("");
    g_string_append_printf (printable,
                            "%sHeader:\n"
                            "%s  length      = %u\n"
                            "%s  type        = %s (0x%08x)\n"
                            "%s  transaction = %u\n",
                            line_prefix,
                            line_prefix, MBIM_MESSAGE_GET_MESSAGE_LENGTH (self),
                            line_prefix, mbim_message_type_get_string (MBIM_MESSAGE_GET_MESSAGE_TYPE (self)), MBIM_MESSAGE_GET_MESSAGE_TYPE (self),
                            line_prefix, MBIM_MESSAGE_GET_TRANSACTION_ID (self));

    switch (MBIM_MESSAGE_GET_MESSAGE_TYPE (self)) {
    case MBIM_MESSAGE_TYPE_INVALID:
        g_warn_if_reached ();
        break;

    case MBIM_MESSAGE_TYPE_OPEN:
        if (!headers_only)
            g_string_append_printf (printable,
                                    "%sContents:\n"
                                    "%s  max control transfer = %u\n",
                                    line_prefix,
                                    line_prefix, mbim_message_open_get_max_control_transfer (self));
        break;

    case MBIM_MESSAGE_TYPE_CLOSE:
        break;

    case MBIM_MESSAGE_TYPE_OPEN_DONE:
        if (!headers_only) {
            MbimStatusError status;

            status = mbim_message_open_done_get_status_code (self);
            g_string_append_printf (printable,
                                    "%sContents:\n"
                                    "%s  status error = '%s' (0x%08x)\n",
                                    line_prefix,
                                    line_prefix, mbim_status_error_get_string (status), status);
        }
        break;

    case MBIM_MESSAGE_TYPE_CLOSE_DONE:
        if (!headers_only) {
            MbimStatusError status;

            status = mbim_message_close_done_get_status_code (self);
            g_string_append_printf (printable,
                                    "%sContents:\n"
                                    "%s  status error = '%s' (0x%08x)\n",
                                    line_prefix,
                                    line_prefix, mbim_status_error_get_string (status), status);
        }
        break;

    case MBIM_MESSAGE_TYPE_HOST_ERROR:
    case MBIM_MESSAGE_TYPE_FUNCTION_ERROR:
        if (!headers_only) {
            MbimProtocolError error;

            error = mbim_message_error_get_error_status_code (self);
            g_string_append_printf (printable,
                                    "%sContents:\n"
                                    "%s  error = '%s' (0x%08x)\n",
                                    line_prefix,
                                    line_prefix, mbim_protocol_error_get_string (error), error);
        }
        break;

    case MBIM_MESSAGE_TYPE_COMMAND:
        g_string_append_printf (printable,
                                "%sFragment header:\n"
                                "%s  total   = %u\n"
                                "%s  current = %u\n",
                                line_prefix,
                                line_prefix, _mbim_message_fragment_get_total (self),
                                line_prefix, _mbim_message_fragment_get_current (self));
        if (!headers_only) {
            g_autofree gchar *uuid_printable = NULL;
            const gchar      *cid_printable;

            service_read_fields = mbim_message_command_get_service (self);

            uuid_printable = mbim_uuid_get_printable (mbim_message_command_get_service_id (self));
            cid_printable = mbim_cid_get_printable (mbim_message_command_get_service (self),
                                                    mbim_message_command_get_cid (self));
            g_string_append_printf (printable,
                                    "%sContents:\n"
                                    "%s  service = '%s' (%s)\n"
                                    "%s  cid     = '%s' (0x%08x)\n"
                                    "%s  type    = '%s' (0x%08x)\n",
                                    line_prefix,
                                    line_prefix, mbim_service_lookup_name (mbim_message_command_get_service (self)), uuid_printable,
                                    line_prefix, cid_printable, mbim_message_command_get_cid (self),
                                    line_prefix, mbim_message_command_type_get_string (mbim_message_command_get_command_type (self)), mbim_message_command_get_command_type (self));
        }
        break;

    case MBIM_MESSAGE_TYPE_COMMAND_DONE:
        g_string_append_printf (printable,
                                "%sFragment header:\n"
                                "%s  total   = %u\n"
                                "%s  current = %u\n",
                                line_prefix,
                                line_prefix, _mbim_message_fragment_get_total (self),
                                line_prefix, _mbim_message_fragment_get_current (self));
        if (!headers_only) {
            g_autofree gchar *uuid_printable = NULL;
            MbimStatusError   status;
            const gchar      *cid_printable;

            service_read_fields = mbim_message_command_done_get_service (self);

            status = mbim_message_command_done_get_status_code (self);
            uuid_printable = mbim_uuid_get_printable (mbim_message_command_done_get_service_id (self));
            cid_printable = mbim_cid_get_printable (mbim_message_command_done_get_service (self),
                                                    mbim_message_command_done_get_cid (self));
            g_string_append_printf (printable,
                                    "%sContents:\n"
                                    "%s  status error = '%s' (0x%08x)\n"
                                    "%s  service      = '%s' (%s)\n"
                                    "%s  cid          = '%s' (0x%08x)\n",
                                    line_prefix,
                                    line_prefix, mbim_status_error_get_string (status), status,
                                    line_prefix, mbim_service_lookup_name (mbim_message_command_done_get_service (self)), uuid_printable,
                                    line_prefix, cid_printable, mbim_message_command_done_get_cid (self));
        }
        break;

    case MBIM_MESSAGE_TYPE_INDICATE_STATUS:
        g_string_append_printf (printable,
                                "%sFragment header:\n"
                                "%s  total   = %u\n"
                                "%s  current = %u\n",
                                line_prefix,
                                line_prefix, _mbim_message_fragment_get_total (self),
                                line_prefix, _mbim_message_fragment_get_current (self));
        if (!headers_only) {
            g_autofree gchar *uuid_printable = NULL;
            const gchar      *cid_printable;

            service_read_fields = mbim_message_indicate_status_get_service (self);

            uuid_printable = mbim_uuid_get_printable (mbim_message_indicate_status_get_service_id (self));
            cid_printable = mbim_cid_get_printable (mbim_message_indicate_status_get_service (self),
                                                    mbim_message_indicate_status_get_cid (self));
            g_string_append_printf (printable,
                                    "%sContents:\n"
                                    "%s  service = '%s' (%s)\n"
                                    "%s  cid     = '%s' (0x%08x)\n",
                                    line_prefix,
                                    line_prefix, mbim_service_lookup_name (mbim_message_indicate_status_get_service (self)), uuid_printable,
                                    line_prefix, cid_printable, mbim_message_indicate_status_get_cid (self));
        }
        break;

    default:
        g_assert_not_reached ();
    }

    if (service_read_fields != MBIM_SERVICE_INVALID) {
        g_autofree gchar  *fields_printable = NULL;
        g_autoptr(GError)  error = NULL;

        switch (service_read_fields) {
        case MBIM_SERVICE_BASIC_CONNECT:
            fields_printable = __mbim_message_basic_connect_get_printable_fields (self, line_prefix, &error);
            break;
        case MBIM_SERVICE_SMS:
            fields_printable = __mbim_message_sms_get_printable_fields (self, line_prefix, &error);
            break;
        case MBIM_SERVICE_USSD:
            fields_printable = __mbim_message_ussd_get_printable_fields (self, line_prefix, &error);
            break;
        case MBIM_SERVICE_PHONEBOOK:
            fields_printable = __mbim_message_phonebook_get_printable_fields (self, line_prefix, &error);
            break;
        case MBIM_SERVICE_STK:
            fields_printable = __mbim_message_stk_get_printable_fields (self, line_prefix, &error);
            break;
        case MBIM_SERVICE_AUTH:
            fields_printable = __mbim_message_auth_get_printable_fields (self, line_prefix, &error);
            break;
        case MBIM_SERVICE_DSS:
            fields_printable = __mbim_message_dss_get_printable_fields (self, line_prefix, &error);
            break;
        case MBIM_SERVICE_MS_FIRMWARE_ID:
            fields_printable = __mbim_message_ms_firmware_id_get_printable_fields (self, line_prefix, &error);
            break;
        case MBIM_SERVICE_MS_HOST_SHUTDOWN:
            fields_printable = __mbim_message_ms_host_shutdown_get_printable_fields (self, line_prefix, &error);
            break;
        case MBIM_SERVICE_MS_SAR:
            fields_printable = __mbim_message_ms_sar_get_printable_fields (self, line_prefix, &error);
            break;
        case MBIM_SERVICE_PROXY_CONTROL:
            fields_printable = __mbim_message_proxy_control_get_printable_fields (self, line_prefix, &error);
            break;
        case MBIM_SERVICE_QMI:
            fields_printable = __mbim_message_qmi_get_printable_fields (self, line_prefix, &error);
            break;
        case MBIM_SERVICE_ATDS:
            fields_printable = __mbim_message_atds_get_printable_fields (self, line_prefix, &error);
            break;
        case MBIM_SERVICE_INTEL_FIRMWARE_UPDATE:
            fields_printable = __mbim_message_intel_firmware_update_get_printable_fields (self, line_prefix, &error);
            break;
        case MBIM_SERVICE_QDU:
            fields_printable = __mbim_message_qdu_get_printable_fields (self, line_prefix, &error);
            break;
        case MBIM_SERVICE_MS_BASIC_CONNECT_EXTENSIONS:
            fields_printable = __mbim_message_ms_basic_connect_extensions_get_printable_fields (self, line_prefix, &error);
            break;
        case MBIM_SERVICE_MS_UICC_LOW_LEVEL_ACCESS:
            fields_printable = __mbim_message_ms_uicc_low_level_access_get_printable_fields (self, line_prefix, &error);
            break;
        case MBIM_SERVICE_INVALID:
        case MBIM_SERVICE_LAST:
            g_assert_not_reached ();
        default:
            break;
        }

        if (error)
            g_string_append_printf (printable,
                                    "%sFields: %s\n",
                                    line_prefix, error->message);
        else if (fields_printable && fields_printable[0])
            g_string_append_printf (printable,
                                    "%sFields:\n"
                                    "%s",
                                    line_prefix, fields_printable);
    }

    return g_string_free (printable, FALSE);
}

/*****************************************************************************/
/* Fragment interface */

#define MBIM_MESSAGE_IS_FRAGMENT(self)                                  \
    (MBIM_MESSAGE_GET_MESSAGE_TYPE (self) == MBIM_MESSAGE_TYPE_COMMAND || \
     MBIM_MESSAGE_GET_MESSAGE_TYPE (self) == MBIM_MESSAGE_TYPE_COMMAND_DONE || \
     MBIM_MESSAGE_GET_MESSAGE_TYPE (self) == MBIM_MESSAGE_TYPE_INDICATE_STATUS)

#define MBIM_MESSAGE_FRAGMENT_GET_TOTAL(self)                           \
    GUINT32_FROM_LE (((struct full_message *)(self->data))->message.fragment.fragment_header.total)

#define MBIM_MESSAGE_FRAGMENT_GET_CURRENT(self)                         \
    GUINT32_FROM_LE (((struct full_message *)(self->data))->message.fragment.fragment_header.current)


gboolean
_mbim_message_is_fragment (const MbimMessage *self)
{
    return MBIM_MESSAGE_IS_FRAGMENT (self);
}

guint32
_mbim_message_fragment_get_total (const MbimMessage  *self)
{
    g_assert (MBIM_MESSAGE_IS_FRAGMENT (self));

    return MBIM_MESSAGE_FRAGMENT_GET_TOTAL (self);
}

guint32
_mbim_message_fragment_get_current (const MbimMessage  *self)
{
    g_assert (MBIM_MESSAGE_IS_FRAGMENT (self));

    return MBIM_MESSAGE_FRAGMENT_GET_CURRENT (self);
}

const guint8 *
_mbim_message_fragment_get_payload (const MbimMessage *self,
                                    guint32           *length)
{
    g_assert (MBIM_MESSAGE_IS_FRAGMENT (self));

    *length = (MBIM_MESSAGE_GET_MESSAGE_LENGTH (self) - \
               sizeof (struct header) -                 \
               sizeof (struct fragment_header));
    return ((struct full_message *)(self->data))->message.fragment.buffer;
}

MbimMessage *
_mbim_message_fragment_collector_init (const MbimMessage  *fragment,
                                       GError            **error)
{
   g_assert (MBIM_MESSAGE_IS_FRAGMENT (fragment));

   /* Collector must start with fragment #0 */
   if (MBIM_MESSAGE_FRAGMENT_GET_CURRENT (fragment) != 0) {
       g_set_error (error,
                    MBIM_PROTOCOL_ERROR,
                    MBIM_PROTOCOL_ERROR_FRAGMENT_OUT_OF_SEQUENCE,
                    "Expecting fragment '0/%u', got '%u/%u'",
                    MBIM_MESSAGE_FRAGMENT_GET_TOTAL (fragment),
                    MBIM_MESSAGE_FRAGMENT_GET_CURRENT (fragment),
                    MBIM_MESSAGE_FRAGMENT_GET_TOTAL (fragment));
       return NULL;
   }

   return mbim_message_dup (fragment);
}

gboolean
_mbim_message_fragment_collector_add (MbimMessage        *self,
                                      const MbimMessage  *fragment,
                                      GError            **error)
{
    guint32 buffer_len;
    const guint8 *buffer;

    g_assert (MBIM_MESSAGE_IS_FRAGMENT (self));
    g_assert (MBIM_MESSAGE_IS_FRAGMENT (fragment));

    /* We can only add a fragment if it is the next one we're expecting.
     * Otherwise, we return an error. */
    if (MBIM_MESSAGE_FRAGMENT_GET_CURRENT (self) != (MBIM_MESSAGE_FRAGMENT_GET_CURRENT (fragment) - 1)) {
        g_set_error (error,
                     MBIM_PROTOCOL_ERROR,
                     MBIM_PROTOCOL_ERROR_FRAGMENT_OUT_OF_SEQUENCE,
                     "Expecting fragment '%u/%u', got '%u/%u'",
                     MBIM_MESSAGE_FRAGMENT_GET_CURRENT (self) + 1,
                     MBIM_MESSAGE_FRAGMENT_GET_TOTAL (self),
                     MBIM_MESSAGE_FRAGMENT_GET_CURRENT (fragment),
                     MBIM_MESSAGE_FRAGMENT_GET_TOTAL (fragment));
        return FALSE;
    }

    buffer = _mbim_message_fragment_get_payload (fragment, &buffer_len);
    if (buffer_len) {
        /* Concatenate information buffers */
        g_byte_array_append ((GByteArray *)self, buffer, buffer_len);
        /* Update the whole message length */
        ((struct header *)(self->data))->length =
            GUINT32_TO_LE (MBIM_MESSAGE_GET_MESSAGE_LENGTH (self) + buffer_len);
    }

    /* Update the current fragment info in the main message; skip endian changes */
    ((struct full_message *)(self->data))->message.fragment.fragment_header.current =
        ((struct full_message *)(fragment->data))->message.fragment.fragment_header.current;

    return TRUE;
}

gboolean
_mbim_message_fragment_collector_complete (MbimMessage *self)
{
    g_assert (MBIM_MESSAGE_IS_FRAGMENT (self));

    if (MBIM_MESSAGE_FRAGMENT_GET_CURRENT (self) != (MBIM_MESSAGE_FRAGMENT_GET_TOTAL (self) - 1))
        /* Not complete yet */
        return FALSE;

    /* Reset current & total */
    ((struct full_message *)(self->data))->message.fragment.fragment_header.current = 0;
    ((struct full_message *)(self->data))->message.fragment.fragment_header.total = GUINT32_TO_LE (1);
    return TRUE;
}

struct fragment_info *
_mbim_message_split_fragments (const MbimMessage *self,
                               guint32            max_fragment_size,
                               guint             *n_fragments)
{
    GArray *array;
    guint32 total_message_length;
    guint32 total_payload_length;
    guint32 fragment_header_length;
    guint32 fragment_payload_length;
    guint32 total_fragments;
    guint i;
    const guint8 *data;
    guint32 data_length;

    /* A message which is longer than the maximum fragment size needs to be
     * split in different fragments before sending it. */

    total_message_length = mbim_message_get_message_length (self);

    /* If a single fragment is enough, don't try to split */
    if (total_message_length <= max_fragment_size)
        return NULL;

    /* Total payload length is the total length minus the headers of the
     * input message */
    fragment_header_length = sizeof (struct header) + sizeof (struct fragment_header);
    total_payload_length = total_message_length - fragment_header_length;

    /* Fragment payload length is the maximum amount of data that can fit in a
     * single fragment */
    fragment_payload_length = max_fragment_size - fragment_header_length;

    /* We can now compute the number of fragments that we'll get */
    total_fragments = total_payload_length / fragment_payload_length;
    if (total_payload_length % fragment_payload_length)
        total_fragments++;

    array = g_array_sized_new (FALSE,
                               FALSE,
                               sizeof (struct fragment_info),
                               total_fragments);

    /* Initialize data walkers */
    data = ((struct full_message *)(((GByteArray *)self)->data))->message.fragment.buffer;
    data_length = total_payload_length;

    /* Create fragment infos */
    for (i = 0; i < total_fragments; i++) {
        struct fragment_info info;

        /* Set data info */
        info.data = data;
        info.data_length = (data_length > fragment_payload_length ? fragment_payload_length : data_length);

        /* Set header info */
        info.header.type             = GUINT32_TO_LE (MBIM_MESSAGE_GET_MESSAGE_TYPE (self));
        info.header.length           = GUINT32_TO_LE (fragment_header_length + info.data_length);
        info.header.transaction_id   = GUINT32_TO_LE (MBIM_MESSAGE_GET_TRANSACTION_ID (self));
        info.fragment_header.total   = GUINT32_TO_LE (total_fragments);
        info.fragment_header.current = GUINT32_TO_LE (i);

        g_array_insert_val (array, i, info);

        /* Update walkers */
        data = &data[info.data_length];
        data_length -= info.data_length;
    }

    g_warn_if_fail (data_length == 0);

    *n_fragments = total_fragments;
    return (struct fragment_info *) g_array_free (array, FALSE);
}

/*****************************************************************************/
/* 'Open' message interface */

MbimMessage *
mbim_message_open_new (guint32 transaction_id,
                       guint32 max_control_transfer)
{
    GByteArray *self;

    self = _mbim_message_allocate (MBIM_MESSAGE_TYPE_OPEN,
                                   transaction_id,
                                   sizeof (struct open_message));

    /* Open header */
    ((struct full_message *)(self->data))->message.open.max_control_transfer = GUINT32_TO_LE (max_control_transfer);

    return (MbimMessage *)self;
}

guint32
mbim_message_open_get_max_control_transfer (const MbimMessage *self)
{
    g_return_val_if_fail (self != NULL, 0);
    g_return_val_if_fail (MBIM_MESSAGE_GET_MESSAGE_TYPE (self) == MBIM_MESSAGE_TYPE_OPEN, 0);

    return GUINT32_FROM_LE (((struct full_message *)(self->data))->message.open.max_control_transfer);
}

/*****************************************************************************/
/* 'Open Done' message interface */

MbimMessage *
mbim_message_open_done_new (guint32         transaction_id,
                            MbimStatusError error_status_code)
{
    GByteArray *self;

    self = _mbim_message_allocate (MBIM_MESSAGE_TYPE_OPEN_DONE,
                                   transaction_id,
                                   sizeof (struct open_done_message));

    /* Open header */
    ((struct full_message *)(self->data))->message.open_done.status_code = GUINT32_TO_LE (error_status_code);

    return (MbimMessage *)self;
}

MbimStatusError
mbim_message_open_done_get_status_code (const MbimMessage *self)
{
    g_return_val_if_fail (self != NULL, MBIM_STATUS_ERROR_FAILURE);
    g_return_val_if_fail (MBIM_MESSAGE_GET_MESSAGE_TYPE (self) == MBIM_MESSAGE_TYPE_OPEN_DONE, MBIM_STATUS_ERROR_FAILURE);

    return (MbimStatusError) GUINT32_FROM_LE (((struct full_message *)(self->data))->message.open_done.status_code);
}

gboolean
mbim_message_open_done_get_result (const MbimMessage  *self,
                                   GError            **error)
{
    MbimStatusError status;

    g_return_val_if_fail (self != NULL, FALSE);
    g_return_val_if_fail (MBIM_MESSAGE_GET_MESSAGE_TYPE (self) == MBIM_MESSAGE_TYPE_OPEN_DONE, FALSE);

    status = (MbimStatusError) GUINT32_FROM_LE (((struct full_message *)(self->data))->message.open_done.status_code);
    if (status == MBIM_STATUS_ERROR_NONE)
        return TRUE;

    set_error_from_status (error, status);
    return FALSE;
}

/*****************************************************************************/
/* 'Close' message interface */

MbimMessage *
mbim_message_close_new (guint32 transaction_id)
{
    return (MbimMessage *) _mbim_message_allocate (MBIM_MESSAGE_TYPE_CLOSE,
                                                   transaction_id,
                                                   0);
}

/*****************************************************************************/
/* 'Close Done' message interface */

MbimMessage *
mbim_message_close_done_new (guint32         transaction_id,
                             MbimStatusError error_status_code)
{
    GByteArray *self;

    self = _mbim_message_allocate (MBIM_MESSAGE_TYPE_CLOSE_DONE,
                                   transaction_id,
                                   sizeof (struct close_done_message));

    /* Open header */
    ((struct full_message *)(self->data))->message.close_done.status_code = GUINT32_TO_LE (error_status_code);

    return (MbimMessage *)self;
}

MbimStatusError
mbim_message_close_done_get_status_code (const MbimMessage *self)
{
    g_return_val_if_fail (self != NULL, MBIM_STATUS_ERROR_FAILURE);
    g_return_val_if_fail (MBIM_MESSAGE_GET_MESSAGE_TYPE (self) == MBIM_MESSAGE_TYPE_CLOSE_DONE, MBIM_STATUS_ERROR_FAILURE);

    return (MbimStatusError) GUINT32_FROM_LE (((struct full_message *)(self->data))->message.close_done.status_code);
}

gboolean
mbim_message_close_done_get_result (const MbimMessage  *self,
                                    GError            **error)
{
    MbimStatusError status;

    g_return_val_if_fail (self != NULL, FALSE);
    g_return_val_if_fail (MBIM_MESSAGE_GET_MESSAGE_TYPE (self) == MBIM_MESSAGE_TYPE_CLOSE_DONE, FALSE);

    status = (MbimStatusError) GUINT32_FROM_LE (((struct full_message *)(self->data))->message.close_done.status_code);
    if (status == MBIM_STATUS_ERROR_NONE)
        return TRUE;

    set_error_from_status (error, status);
    return FALSE;
}

/*****************************************************************************/
/* 'Error' message interface */

MbimMessage *
mbim_message_error_new (guint32           transaction_id,
                        MbimProtocolError error_status_code)
{
    GByteArray *self;

    self = _mbim_message_allocate (MBIM_MESSAGE_TYPE_HOST_ERROR,
                                   transaction_id,
                                   sizeof (struct error_message));

    /* Open header */
    ((struct full_message *)(self->data))->message.error.error_status_code = GUINT32_TO_LE (error_status_code);

    return (MbimMessage *)self;
}

MbimMessage *
mbim_message_function_error_new (guint32           transaction_id,
                                 MbimProtocolError error_status_code)
{
    GByteArray *self;

    self = _mbim_message_allocate (MBIM_MESSAGE_TYPE_FUNCTION_ERROR,
                                   transaction_id,
                                   sizeof (struct error_message));

    /* Open header */
    ((struct full_message *)(self->data))->message.error.error_status_code = GUINT32_TO_LE (error_status_code);

    return (MbimMessage *)self;
}

MbimProtocolError
mbim_message_error_get_error_status_code (const MbimMessage *self)
{
    g_return_val_if_fail (self != NULL, MBIM_PROTOCOL_ERROR_INVALID);
    g_return_val_if_fail ((MBIM_MESSAGE_GET_MESSAGE_TYPE (self) == MBIM_MESSAGE_TYPE_HOST_ERROR ||
                           MBIM_MESSAGE_GET_MESSAGE_TYPE (self) == MBIM_MESSAGE_TYPE_FUNCTION_ERROR),
                          MBIM_PROTOCOL_ERROR_INVALID);

    return (MbimProtocolError) GUINT32_FROM_LE (((struct full_message *)(self->data))->message.error.error_status_code);
}

GError *
mbim_message_error_get_error (const MbimMessage *self)
{
    MbimProtocolError error_status_code;


    g_return_val_if_fail (self != NULL, NULL);
    g_return_val_if_fail ((MBIM_MESSAGE_GET_MESSAGE_TYPE (self) == MBIM_MESSAGE_TYPE_HOST_ERROR ||
                           MBIM_MESSAGE_GET_MESSAGE_TYPE (self) == MBIM_MESSAGE_TYPE_FUNCTION_ERROR),
                          NULL);

    error_status_code = (MbimProtocolError) GUINT32_FROM_LE (((struct full_message *)(self->data))->message.error.error_status_code);

    return g_error_new (MBIM_PROTOCOL_ERROR,
                        error_status_code,
                        "MBIM protocol error: %s",
                        mbim_protocol_error_get_string (error_status_code));
}

/*****************************************************************************/
/* 'Command' message interface */

MbimMessage *
mbim_message_command_new (guint32                transaction_id,
                          MbimService            service,
                          guint32                cid,
                          MbimMessageCommandType command_type)
{
    GByteArray *self;
    const MbimUuid *service_id;

    /* Known service required */
    service_id = mbim_uuid_from_service (service);
    g_return_val_if_fail (service_id != NULL, NULL);

    self = _mbim_message_allocate (MBIM_MESSAGE_TYPE_COMMAND,
                                   transaction_id,
                                   sizeof (struct command_message));

    /* Fragment header */
    ((struct full_message *)(self->data))->message.command.fragment_header.total   = GUINT32_TO_LE (1);
    ((struct full_message *)(self->data))->message.command.fragment_header.current = 0;

    /* Command header */
    memcpy (((struct full_message *)(self->data))->message.command.service_id, service_id, sizeof (*service_id));
    ((struct full_message *)(self->data))->message.command.command_id    = GUINT32_TO_LE (cid);
    ((struct full_message *)(self->data))->message.command.command_type  = GUINT32_TO_LE (command_type);
    ((struct full_message *)(self->data))->message.command.buffer_length = 0;

    return (MbimMessage *)self;
}

void
mbim_message_command_append (MbimMessage  *self,
                             const guint8 *buffer,
                             guint32       buffer_size)
{
    g_byte_array_append ((GByteArray *)self, buffer, buffer_size);

    /* Update message and buffer length */
    ((struct header *)(self->data))->length =
        GUINT32_TO_LE (MBIM_MESSAGE_GET_MESSAGE_LENGTH (self) + buffer_size);
    ((struct full_message *)(self->data))->message.command.buffer_length =
        GUINT32_TO_LE (GUINT32_FROM_LE (((struct full_message *)(self->data))->message.command.buffer_length) + buffer_size);
}

MbimService
mbim_message_command_get_service (const MbimMessage *self)
{
    g_return_val_if_fail (self != NULL, MBIM_SERVICE_INVALID);
    g_return_val_if_fail (MBIM_MESSAGE_GET_MESSAGE_TYPE (self) == MBIM_MESSAGE_TYPE_COMMAND, MBIM_SERVICE_INVALID);

    return mbim_uuid_to_service ((const MbimUuid *)&(((struct full_message *)(self->data))->message.command.service_id));
}

const MbimUuid *
mbim_message_command_get_service_id (const MbimMessage *self)
{
    g_return_val_if_fail (self != NULL, MBIM_UUID_INVALID);
    g_return_val_if_fail (MBIM_MESSAGE_GET_MESSAGE_TYPE (self) == MBIM_MESSAGE_TYPE_COMMAND, MBIM_UUID_INVALID);

    return (const MbimUuid *)&(((struct full_message *)(self->data))->message.command.service_id);
}

guint32
mbim_message_command_get_cid (const MbimMessage *self)
{
    g_return_val_if_fail (self != NULL, 0);
    g_return_val_if_fail (MBIM_MESSAGE_GET_MESSAGE_TYPE (self) == MBIM_MESSAGE_TYPE_COMMAND, 0);

    return GUINT32_FROM_LE (((struct full_message *)(self->data))->message.command.command_id);
}

MbimMessageCommandType
mbim_message_command_get_command_type (const MbimMessage *self)
{
    g_return_val_if_fail (self != NULL, MBIM_MESSAGE_COMMAND_TYPE_UNKNOWN);
    g_return_val_if_fail (MBIM_MESSAGE_GET_MESSAGE_TYPE (self) == MBIM_MESSAGE_TYPE_COMMAND, MBIM_MESSAGE_COMMAND_TYPE_UNKNOWN);

    return (MbimMessageCommandType) GUINT32_FROM_LE (((struct full_message *)(self->data))->message.command.command_type);
}

const guint8 *
mbim_message_command_get_raw_information_buffer (const MbimMessage *self,
                                                 guint32           *out_length)
{
    guint32 length;

    g_return_val_if_fail (self != NULL, NULL);
    g_return_val_if_fail (MBIM_MESSAGE_GET_MESSAGE_TYPE (self) == MBIM_MESSAGE_TYPE_COMMAND, NULL);

    length = GUINT32_FROM_LE (((struct full_message *)(self->data))->message.command.buffer_length);
    if (out_length)
        *out_length = length;

    return (length > 0 ?
            ((struct full_message *)(self->data))->message.command.buffer :
            NULL);
}

/*****************************************************************************/
/* 'Command Done' message interface */

MbimService
mbim_message_command_done_get_service (const MbimMessage *self)
{
    g_return_val_if_fail (self != NULL, MBIM_SERVICE_INVALID);
    g_return_val_if_fail (MBIM_MESSAGE_GET_MESSAGE_TYPE (self) == MBIM_MESSAGE_TYPE_COMMAND_DONE, MBIM_SERVICE_INVALID);

    return mbim_uuid_to_service ((const MbimUuid *)&(((struct full_message *)(self->data))->message.command_done.service_id));
}

const MbimUuid *
mbim_message_command_done_get_service_id (const MbimMessage *self)
{
    g_return_val_if_fail (self != NULL, MBIM_UUID_INVALID);
    g_return_val_if_fail (MBIM_MESSAGE_GET_MESSAGE_TYPE (self) == MBIM_MESSAGE_TYPE_COMMAND_DONE, MBIM_UUID_INVALID);

    return (const MbimUuid *)&(((struct full_message *)(self->data))->message.command_done.service_id);
}

guint32
mbim_message_command_done_get_cid (const MbimMessage *self)
{
    g_return_val_if_fail (self != NULL, 0);
    g_return_val_if_fail (MBIM_MESSAGE_GET_MESSAGE_TYPE (self) == MBIM_MESSAGE_TYPE_COMMAND_DONE, 0);

    return GUINT32_FROM_LE (((struct full_message *)(self->data))->message.command_done.command_id);
}

MbimStatusError
mbim_message_command_done_get_status_code (const MbimMessage *self)
{
    g_return_val_if_fail (self != NULL, MBIM_STATUS_ERROR_FAILURE);
    g_return_val_if_fail (MBIM_MESSAGE_GET_MESSAGE_TYPE (self) == MBIM_MESSAGE_TYPE_COMMAND_DONE, MBIM_STATUS_ERROR_FAILURE);

    return (MbimStatusError) GUINT32_FROM_LE (((struct full_message *)(self->data))->message.command_done.status_code);
}

gboolean
mbim_message_command_done_get_result (const MbimMessage  *self,
                                      GError            **error)
{
    MbimStatusError status;

    g_return_val_if_fail (self != NULL, FALSE);
    g_return_val_if_fail (MBIM_MESSAGE_GET_MESSAGE_TYPE (self) == MBIM_MESSAGE_TYPE_COMMAND_DONE, FALSE);

    status = (MbimStatusError) GUINT32_FROM_LE (((struct full_message *)(self->data))->message.command_done.status_code);
    if (status == MBIM_STATUS_ERROR_NONE)
        return TRUE;

    set_error_from_status (error, status);
    return FALSE;
}

const guint8 *
mbim_message_command_done_get_raw_information_buffer (const MbimMessage *self,
                                                      guint32           *out_length)
{
    guint32 length;

    g_return_val_if_fail (self != NULL, NULL);
    g_return_val_if_fail (MBIM_MESSAGE_GET_MESSAGE_TYPE (self) == MBIM_MESSAGE_TYPE_COMMAND_DONE, NULL);

    length = GUINT32_FROM_LE (((struct full_message *)(self->data))->message.command_done.buffer_length);
    if (out_length)
        *out_length = length;

    return (length > 0 ?
            ((struct full_message *)(self->data))->message.command_done.buffer :
            NULL);
}

/*****************************************************************************/
/* 'Indicate Status' message interface */

MbimService
mbim_message_indicate_status_get_service (const MbimMessage *self)
{
    g_return_val_if_fail (self != NULL, MBIM_SERVICE_INVALID);
    g_return_val_if_fail (MBIM_MESSAGE_GET_MESSAGE_TYPE (self) == MBIM_MESSAGE_TYPE_INDICATE_STATUS, MBIM_SERVICE_INVALID);

    return mbim_uuid_to_service ((const MbimUuid *)&(((struct full_message *)(self->data))->message.indicate_status.service_id));
}

const MbimUuid *
mbim_message_indicate_status_get_service_id (const MbimMessage *self)
{
    g_return_val_if_fail (self != NULL, MBIM_UUID_INVALID);
    g_return_val_if_fail (MBIM_MESSAGE_GET_MESSAGE_TYPE (self) == MBIM_MESSAGE_TYPE_INDICATE_STATUS, MBIM_UUID_INVALID);

    return (const MbimUuid *)&(((struct full_message *)(self->data))->message.indicate_status.service_id);
}

guint32
mbim_message_indicate_status_get_cid (const MbimMessage *self)
{
    g_return_val_if_fail (self != NULL, 0);
    g_return_val_if_fail (MBIM_MESSAGE_GET_MESSAGE_TYPE (self) == MBIM_MESSAGE_TYPE_INDICATE_STATUS, 0);

    return GUINT32_FROM_LE (((struct full_message *)(self->data))->message.indicate_status.command_id);
}

const guint8 *
mbim_message_indicate_status_get_raw_information_buffer (const MbimMessage *self,
                                                         guint32           *out_length)
{
    guint32 length;

    g_return_val_if_fail (self != NULL, NULL);
    g_return_val_if_fail (MBIM_MESSAGE_GET_MESSAGE_TYPE (self) == MBIM_MESSAGE_TYPE_INDICATE_STATUS, NULL);

    length = GUINT32_FROM_LE (((struct full_message *)(self->data))->message.indicate_status.buffer_length);
    if (out_length)
        *out_length = length;

    return (length > 0 ?
            ((struct full_message *)(self->data))->message.indicate_status.buffer :
            NULL);
}

/*****************************************************************************/
/* Other helpers */

gboolean
mbim_message_response_get_result (const MbimMessage  *self,
                                  MbimMessageType     expected,
                                  GError            **error)
{
    MbimStatusError status = MBIM_STATUS_ERROR_NONE;
    MbimMessageType type;

    g_return_val_if_fail (self != NULL, FALSE);
    g_return_val_if_fail (expected == MBIM_MESSAGE_TYPE_OPEN_DONE  ||
                          expected == MBIM_MESSAGE_TYPE_CLOSE_DONE ||
                          expected == MBIM_MESSAGE_TYPE_COMMAND_DONE, FALSE);

    type = MBIM_MESSAGE_GET_MESSAGE_TYPE (self);
    if (type != MBIM_MESSAGE_TYPE_FUNCTION_ERROR && type != expected) {
        g_set_error (error,
                     MBIM_CORE_ERROR,
                     MBIM_CORE_ERROR_INVALID_MESSAGE,
                     "Unexpected response message type: 0x%04X", (guint32) type);
        return FALSE;
    }

    switch (type) {
    case MBIM_MESSAGE_TYPE_OPEN_DONE:
        status = (MbimStatusError) GUINT32_FROM_LE (((struct full_message *)(self->data))->message.open_done.status_code);
        break;

    case MBIM_MESSAGE_TYPE_CLOSE_DONE:
        status = (MbimStatusError) GUINT32_FROM_LE (((struct full_message *)(self->data))->message.close_done.status_code);
        break;

    case MBIM_MESSAGE_TYPE_COMMAND_DONE:
        status = (MbimStatusError) GUINT32_FROM_LE (((struct full_message *)(self->data))->message.command_done.status_code);
        break;

    case MBIM_MESSAGE_TYPE_FUNCTION_ERROR:
        if (error)
            *error = mbim_message_error_get_error (self);
        return FALSE;

    case MBIM_MESSAGE_TYPE_INVALID:
    case MBIM_MESSAGE_TYPE_OPEN:
    case MBIM_MESSAGE_TYPE_CLOSE:
    case MBIM_MESSAGE_TYPE_COMMAND:
    case MBIM_MESSAGE_TYPE_HOST_ERROR:
    case MBIM_MESSAGE_TYPE_INDICATE_STATUS:
    default:
        g_assert_not_reached ();
    }

    if (status == MBIM_STATUS_ERROR_NONE)
        return TRUE;

    /* Build error */
    set_error_from_status (error, status);
    return FALSE;
}
