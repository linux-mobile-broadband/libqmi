/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * libmbim-glib -- GLib/GIO based library to control MBIM devices
 *
 * Copyright (C) 2021 Aleksander Morgado <aleksander@aleksander.es>
 * Copyright (C) 2021 Intel Corporation
 */

#include <glib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <endian.h>

#include "mbim-tlv.h"
#include "mbim-tlv-private.h"
#include "mbim-error-types.h"
#include "mbim-enum-types.h"
#include "mbim-common.h"

/*****************************************************************************/

GType
mbim_tlv_get_type (void)
{
    static gsize g_define_type_id_initialized = 0;

    if (g_once_init_enter (&g_define_type_id_initialized)) {
        GType g_define_type_id =
            g_boxed_type_register_static (g_intern_static_string ("MbimTlv"),
                                          (GBoxedCopyFunc) mbim_tlv_ref,
                                          (GBoxedFreeFunc) mbim_tlv_unref);

        g_once_init_leave (&g_define_type_id_initialized, g_define_type_id);
    }

    return g_define_type_id_initialized;
}

/*****************************************************************************/

gchar *
_mbim_tlv_print (const MbimTlv *tlv,
                 const gchar   *line_prefix)
{
    GString          *str;
    MbimTlvType       tlv_type;
    const gchar      *tlv_type_str;
    const guint8     *tlv_data;
    guint32           tlv_data_size;
    g_autofree gchar *tlv_data_str = NULL;

    tlv_type = mbim_tlv_get_tlv_type (tlv);
    tlv_type_str = mbim_tlv_type_get_string (tlv_type);

    str = g_string_new ("");
    g_string_append_printf (str, "{\n");
    g_string_append_printf (str, "%s  tlv type   = %s (0x%04x)\n", line_prefix, tlv_type_str ? tlv_type_str : "unknown", tlv_type);

    tlv_data = mbim_tlv_get_tlv_data (tlv, &tlv_data_size);
    tlv_data_str = mbim_common_str_hex (tlv_data, tlv_data_size, ':');
    g_string_append_printf (str, "%s  tlv data   = %s\n", line_prefix, tlv_data_str ? tlv_data_str : "");

    if (tlv_type == MBIM_TLV_TYPE_WCHAR_STR) {
        g_autoptr(GError) error = NULL;
        g_autofree gchar *tlv_data_string_str = NULL;

        tlv_data_string_str = mbim_tlv_string_get (tlv, &error);
        if (!tlv_data_string_str)
            tlv_data_string_str = g_strdup_printf ("*** error: %s", error->message);
        g_string_append_printf (str, "%s  tlv string = %s\n", line_prefix, tlv_data_string_str ? tlv_data_string_str : "");
    }

    g_string_append_printf (str, "%s}", line_prefix);

    return g_string_free (str, FALSE);
}

/*****************************************************************************/

MbimTlv *
mbim_tlv_new (MbimTlvType   tlv_type,
              const guint8 *tlv_data,
              guint32       tlv_data_length)
{
    GByteArray *self;
    guint32     tlv_size;
    guint32     padding_size;

    g_return_val_if_fail (tlv_type != MBIM_TLV_TYPE_INVALID, NULL);

    /* Compute size of the TLV and allocate heap for it */
    padding_size = (tlv_data_length % 4) ? (4 - (tlv_data_length % 4)) : 0;
    tlv_size = sizeof (struct tlv) + tlv_data_length + padding_size;
    self = g_byte_array_sized_new (tlv_size);
    g_byte_array_set_size (self, tlv_size);

    /* Set TLV header */
    MBIM_TLV_FIELD_TYPE (self)           = GUINT16_TO_LE (tlv_type);
    MBIM_TLV_FIELD_RESERVED (self)       = 0;
    MBIM_TLV_FIELD_PADDING_LENGTH (self) = padding_size;
    MBIM_TLV_FIELD_DATA_LENGTH (self)    = GUINT32_TO_LE (tlv_data_length);

    if (tlv_data && tlv_data_length) {
        memcpy (MBIM_TLV_FIELD_DATA (self), tlv_data, tlv_data_length);
        if (padding_size)
            memset (MBIM_TLV_FIELD_DATA (self) + tlv_data_length, 0, padding_size);
    }

    return (MbimTlv *)self;
}

MbimTlv *
_mbim_tlv_new_from_raw (const guint8  *raw,
                        guint32        raw_length,
                        guint32       *bytes_read,
                        GError       **error)
{
    guint32 tlv_size;

    g_assert (raw_length >= sizeof (struct tlv));
    tlv_size = sizeof (struct tlv) + GUINT32_FROM_LE (((struct tlv *)raw)->data_length) + ((struct tlv *)raw)->padding_length;

    *bytes_read = tlv_size;
    return (MbimTlv *) g_byte_array_append (g_byte_array_sized_new (tlv_size), raw, tlv_size);
}

MbimTlv *
mbim_tlv_dup (const MbimTlv *self)
{
    g_return_val_if_fail (self != NULL, NULL);

    return mbim_tlv_new (MBIM_TLV_GET_TLV_TYPE (self),
                         MBIM_TLV_FIELD_DATA (self),
                         MBIM_TLV_GET_DATA_LENGTH (self));
}

MbimTlv *
mbim_tlv_ref (MbimTlv *self)
{
    g_return_val_if_fail (self != NULL, NULL);

    return (MbimTlv *) g_byte_array_ref ((GByteArray *)self);
}

void
mbim_tlv_unref (MbimTlv *self)
{
    g_return_if_fail (self != NULL);

    g_byte_array_unref ((GByteArray *)self);
}

MbimTlvType
mbim_tlv_get_tlv_type (const MbimTlv *self)
{
    g_return_val_if_fail (self != NULL, MBIM_TLV_TYPE_INVALID);

    return MBIM_TLV_GET_TLV_TYPE (self);
}

const guint8 *
mbim_tlv_get_tlv_data (const MbimTlv *self,
                       guint32       *out_length)
{
    g_return_val_if_fail (self != NULL, NULL);
    g_return_val_if_fail (out_length != NULL, NULL);

    *out_length = MBIM_TLV_GET_DATA_LENGTH (self);
    return MBIM_TLV_FIELD_DATA (self);
}

const guint8 *
mbim_tlv_get_raw (const MbimTlv  *self,
                  guint32        *length,
                  GError        **error)
{
    g_return_val_if_fail (self != NULL, NULL);
    g_return_val_if_fail (length != NULL, NULL);

    if (!self->data || !self->len) {
        g_set_error_literal (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_FAILED, "TLV is invalid");
        return NULL;
    }

    *length = (guint32) self->len;
    return self->data;
}

/*****************************************************************************/

MbimTlv *
mbim_tlv_string_new (const gchar  *str,
                     GError      **error)
{
    g_autofree gunichar2 *utf16 = NULL;
    guint32               utf16_bytes = 0;

    /* Convert the string from UTF-8 to UTF-16HE */
    if (str && str[0]) {
        glong items_written = 0;

        utf16 = g_utf8_to_utf16 (str,
                                 -1,
                                 NULL, /* bytes */
                                 &items_written, /* gunichar2 */
                                 error);
        if (!utf16)
            return NULL;

        utf16_bytes = items_written * 2;

        /* For BE systems, convert from BE to LE */
        if (G_BYTE_ORDER == G_BIG_ENDIAN) {
            guint i;

            for (i = 0; i < items_written; i++)
                utf16[i] = GUINT16_TO_LE (utf16[i]);
        }
    }

    return mbim_tlv_new (MBIM_TLV_TYPE_WCHAR_STR, (const guint8 *)utf16, utf16_bytes);
}

gchar *
mbim_tlv_string_get (const MbimTlv  *self,
                     GError        **error)
{
    const gunichar2      *utf16 = NULL;
    g_autofree gunichar2 *utf16d = NULL;
    guint32               size;

    g_return_val_if_fail (self != NULL, NULL);

    if (MBIM_TLV_GET_TLV_TYPE (self) != MBIM_TLV_TYPE_WCHAR_STR) {
        g_set_error (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_INVALID_ARGS,
                     "TLV is not a WCHAR string");
        return NULL;
    }

    utf16 = (const gunichar2 *) MBIM_TLV_FIELD_DATA (self);
    size = MBIM_TLV_FIELD_DATA_LENGTH (self);

    /* For BE systems, convert from LE to BE */
    if (G_BYTE_ORDER == G_BIG_ENDIAN) {
        guint i;

        utf16d = (gunichar2 *) g_malloc (size);
        for (i = 0; i < (size / 2); i++)
            utf16d[i] = GUINT16_FROM_LE (utf16[i]);
    }

    return g_utf16_to_utf8 (utf16d ? utf16d : utf16,
                            size / 2,
                            NULL,
                            NULL,
                            error);
}
