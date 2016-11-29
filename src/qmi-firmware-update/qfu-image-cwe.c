/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * qmi-firmware-update -- Command line tool to update firmware in QMI devices
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright (C) 2016 Bjørn Mork <bjorn@mork.no>
 * Copyright (C) 2016 Zodiac Inflight Innovations
 * Copyright (C) 2016 Aleksander Morgado <aleksander@aleksander.es>
 */

#include <string.h>
#include "qfu-image-cwe.h"

static GInitableIface *iface_initable_parent;
static void            initable_iface_init (GInitableIface *iface);

G_DEFINE_TYPE_EXTENDED (QfuImageCwe, qfu_image_cwe, QFU_TYPE_IMAGE, 0,
                        G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE, initable_iface_init))

/* Sierra Wireless CWE file header
 *   Note: 32bit numbers are big endian
 */
typedef struct _QfuCweFileHeader QfuCweFileHeader;
struct _QfuCweFileHeader {
    gchar   reserved1[256];
    guint32 crc;         /* 32bit CRC of "reserved1" field */
    guint32 rev;         /* header revision */
    guint32 val;         /* CRC validity indicator */
    gchar   type[4];     /* ASCII - not null terminated */
    gchar   product[4];  /* ASCII - not null terminated */
    guint32 imgsize;     /* image size */
    guint32 imgcrc;      /* 32bit CRC of the image */
    gchar   version[84]; /* ASCII - null terminated */
    gchar   date[8];     /* ASCII - null terminated */
    guint32 compat;      /* backward compatibility */
    gchar   reserved2[20];
} __attribute__ ((packed));

struct _QfuImageCwePrivate {
    QfuCweFileHeader  hdr;
    gchar            *type;
    gchar            *product;
};

/******************************************************************************/

const gchar *
qfu_image_cwe_header_get_type (QfuImageCwe *self)
{
    g_return_val_if_fail (QFU_IS_IMAGE_CWE (self), NULL);

    return self->priv->type;
}

const gchar *
qfu_image_cwe_header_get_product (QfuImageCwe *self)
{
    g_return_val_if_fail (QFU_IS_IMAGE_CWE (self), NULL);

    return self->priv->product;
}

const gchar *
qfu_image_cwe_header_get_version (QfuImageCwe *self)
{
    g_return_val_if_fail (QFU_IS_IMAGE_CWE (self), NULL);

    return self->priv->hdr.version;
}

const gchar *
qfu_image_cwe_header_get_date (QfuImageCwe *self)
{
    g_return_val_if_fail (QFU_IS_IMAGE_CWE (self), NULL);

    return self->priv->hdr.date;
}

/******************************************************************************/

static goffset
get_header_size (QfuImage *self)
{
    return (goffset) sizeof (QfuCweFileHeader);
}

static goffset
get_data_size (QfuImage *self)
{
    return qfu_image_get_size (self) - sizeof (QfuCweFileHeader);
}

static gssize
read_header (QfuImage      *_self,
             guint8        *out_buffer,
             gsize          out_buffer_size,
             GCancellable  *cancellable,
             GError       **error)
{
    QfuImageCwe *self = QFU_IMAGE_CWE (_self);

    if (out_buffer_size < sizeof (QfuCweFileHeader)) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "buffer too small to read header");
        return -1;
    }

    memcpy (out_buffer, &self->priv->hdr, sizeof (QfuCweFileHeader));
    return sizeof (QfuCweFileHeader);
}

/******************************************************************************/

static gboolean
initable_init (GInitable     *initable,
               GCancellable  *cancellable,
               GError       **error)
{
    QfuImageCwe  *self;
    GInputStream *input_stream = NULL;
    gboolean      result = FALSE;
    gssize        n_read;

    self = QFU_IMAGE_CWE (initable);

    /* Run parent initable */
    if (!iface_initable_parent->init (initable, cancellable, error))
        return FALSE;

    g_object_get (self, "input-stream", &input_stream, NULL);
    g_assert (G_IS_FILE_INPUT_STREAM (input_stream));

    /* Preload file header */

    g_debug ("[qfu-image-cwe] reading file header");

    if (!g_seekable_seek (G_SEEKABLE (input_stream), 0, G_SEEK_SET, cancellable, error)) {
        g_prefix_error (error, "couldn't seek input stream: ");
        goto out;
    }

    n_read = g_input_stream_read (input_stream,
                                  &self->priv->hdr,
                                  sizeof (QfuCweFileHeader),
                                  cancellable,
                                  error);
    if (n_read < 0) {
        g_prefix_error (error, "couldn't read file header: ");
        goto out;
    }

    if (n_read != sizeof (QfuCweFileHeader)) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "CWE firmware image file is too short: full header not available");
        goto out;
    }

    /* Preload non-NUL terminated strings */
    self->priv->type    = g_strndup (self->priv->hdr.type,    sizeof (self->priv->hdr.type));
    self->priv->product = g_strndup (self->priv->hdr.product, sizeof (self->priv->hdr.product));

    g_debug ("[qfu-image-cwe] validating data size...");

    /* Validate image size as reported in the header */
    if (qfu_image_get_data_size (QFU_IMAGE (self)) != GUINT32_FROM_BE (self->priv->hdr.imgsize)) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "CWE firmware image file is too short: full data not available");
        goto out;
    }

    /* Success! */
    result = TRUE;

out:
    g_object_unref (input_stream);
    return result;
}

/******************************************************************************/

QfuImage *
qfu_image_cwe_new (GFile         *file,
                   GCancellable  *cancellable,
                   GError       **error)
{
    g_return_val_if_fail (G_IS_FILE (file), NULL);

    return QFU_IMAGE (g_initable_new (QFU_TYPE_IMAGE_CWE,
                                      cancellable,
                                      error,
                                      "file",       file,
                                      "image-type", QFU_IMAGE_TYPE_CWE,
                                      NULL));
}

static void
qfu_image_cwe_init (QfuImageCwe *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, QFU_TYPE_IMAGE_CWE, QfuImageCwePrivate);
}

static void
finalize (GObject *object)
{
    QfuImageCwe *self = QFU_IMAGE_CWE (object);

    g_free (self->priv->type);
    g_free (self->priv->product);

    G_OBJECT_CLASS (qfu_image_cwe_parent_class)->finalize (object);
}

static void
initable_iface_init (GInitableIface *iface)
{
    iface_initable_parent = g_type_interface_peek_parent (iface);
    iface->init = initable_init;
}

static void
qfu_image_cwe_class_init (QfuImageCweClass *klass)
{
    GObjectClass  *object_class = G_OBJECT_CLASS (klass);
    QfuImageClass *image_class = QFU_IMAGE_CLASS (klass);

    g_type_class_add_private (object_class, sizeof (QfuImageCwePrivate));

    object_class->finalize = finalize;

    image_class->get_header_size = get_header_size;
    image_class->get_data_size   = get_data_size;
    image_class->read_header     = read_header;

}
