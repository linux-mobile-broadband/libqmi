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
 * Copyright (C) 2016-2017 Aleksander Morgado <aleksander@aleksander.es>
 */

#include <string.h>

#include "qfu-image-cwe.h"
#include "qfu-utils.h"

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

typedef struct {
    guint             parent_image_index;
    QfuCweFileHeader  hdr;
    gchar            *type;
    gchar            *product;
} ImageInfo;

struct _QfuImageCwePrivate {
    GArray *images;

    /* Parsed */
    gchar *firmware_version;
    gchar *config_version;
    gchar *carrier;
};

/******************************************************************************/

guint
qfu_image_cwe_get_n_embedded_headers (QfuImageCwe *self)
{
    g_return_val_if_fail (QFU_IS_IMAGE_CWE (self), 0);

    return (self->priv->images ? self->priv->images->len : 0);
}

gint
qfu_image_cwe_embedded_header_get_parent_index (QfuImageCwe *self,
                                                guint        embedded_i)
{
    ImageInfo *info;

    g_return_val_if_fail (QFU_IS_IMAGE_CWE (self), -1);
    g_return_val_if_fail (self->priv->images, -1);
    g_return_val_if_fail (embedded_i < self->priv->images->len, -1);

    info = &g_array_index (self->priv->images, ImageInfo, embedded_i);
    return info->parent_image_index;
}

const gchar *
qfu_image_cwe_embedded_header_get_type (QfuImageCwe *self,
                                        guint        embedded_i)
{
    ImageInfo *info;

    g_return_val_if_fail (QFU_IS_IMAGE_CWE (self), NULL);
    g_return_val_if_fail (self->priv->images, NULL);
    g_return_val_if_fail (embedded_i < self->priv->images->len, NULL);

    info = &g_array_index (self->priv->images, ImageInfo, embedded_i);
    return info->type;
}

const gchar *
qfu_image_cwe_embedded_header_get_product (QfuImageCwe *self,
                                           guint        embedded_i)
{
    ImageInfo *info;

    g_return_val_if_fail (QFU_IS_IMAGE_CWE (self), NULL);
    g_return_val_if_fail (self->priv->images, NULL);
    g_return_val_if_fail (embedded_i < self->priv->images->len, NULL);

    info = &g_array_index (self->priv->images, ImageInfo, embedded_i);
    return info->product;
}

const gchar *
qfu_image_cwe_embedded_header_get_version (QfuImageCwe *self,
                                           guint        embedded_i)
{
    ImageInfo *info;

    g_return_val_if_fail (QFU_IS_IMAGE_CWE (self), NULL);
    g_return_val_if_fail (self->priv->images, NULL);
    g_return_val_if_fail (embedded_i < self->priv->images->len, NULL);

    info = &g_array_index (self->priv->images, ImageInfo, embedded_i);
    return info->hdr.version;
}

const gchar *
qfu_image_cwe_embedded_header_get_date (QfuImageCwe *self,
                                        guint        embedded_i)
{
    ImageInfo *info;

    g_return_val_if_fail (QFU_IS_IMAGE_CWE (self), NULL);
    g_return_val_if_fail (self->priv->images, NULL);
    g_return_val_if_fail (embedded_i < self->priv->images->len, NULL);

    info = &g_array_index (self->priv->images, ImageInfo, embedded_i);
    return info->hdr.date;
}

guint32
qfu_image_cwe_embedded_header_get_image_size (QfuImageCwe *self,
                                              guint        embedded_i)
{
    ImageInfo *info;

    g_return_val_if_fail (QFU_IS_IMAGE_CWE (self), 0);
    g_return_val_if_fail (self->priv->images, 0);
    g_return_val_if_fail (embedded_i < self->priv->images->len, 0);

    info = &g_array_index (self->priv->images, ImageInfo, embedded_i);
    return GUINT32_FROM_BE (info->hdr.imgsize);
}

/******************************************************************************/
/* The 'main' header is the one at index 0 of the array, always */

const gchar *
qfu_image_cwe_header_get_type (QfuImageCwe *self)
{
    return qfu_image_cwe_embedded_header_get_type (self, 0);
}

const gchar *
qfu_image_cwe_header_get_product (QfuImageCwe *self)
{
    return qfu_image_cwe_embedded_header_get_product (self, 0);
}

const gchar *
qfu_image_cwe_header_get_version (QfuImageCwe *self)
{
    return qfu_image_cwe_embedded_header_get_version (self, 0);
}

const gchar *
qfu_image_cwe_header_get_date (QfuImageCwe *self)
{
    return qfu_image_cwe_embedded_header_get_date (self, 0);
}

guint32
qfu_image_cwe_header_get_image_size (QfuImageCwe *self)
{
    return qfu_image_cwe_embedded_header_get_image_size (self, 0);
}

/******************************************************************************/

static void
parse_firmware_config_carrier (QfuImageCwe *self)
{
    GError *inner_error = NULL;
    guint   i;

    g_assert (!self->priv->firmware_version);
    g_assert (!self->priv->config_version);
    g_assert (!self->priv->carrier);

    /* Try using the internal version first */
    if (!qfu_utils_parse_cwe_version_string (
            qfu_image_cwe_header_get_version (self),
            &self->priv->firmware_version,
            &self->priv->config_version,
            &self->priv->carrier,
            &inner_error)) {
        /* Just log the error message */
        g_debug ("[qfu-image-cwe] couldn't parse internal version string '%s': %s",
                 qfu_image_cwe_header_get_version (self),
                 inner_error->message);
        g_clear_error (&inner_error);
    }

    /* If all retrieved with the internal version string, we're done */
    if (self->priv->firmware_version && self->priv->config_version && self->priv->carrier)
        goto done;

    /* Try using the filename to gather more info */
    if (!qfu_utils_parse_cwe_version_string (
            qfu_image_get_display_name (QFU_IMAGE (self)),
            self->priv->firmware_version ? NULL : &self->priv->firmware_version,
            self->priv->config_version   ? NULL : &self->priv->config_version,
            self->priv->carrier          ? NULL : &self->priv->carrier,
            &inner_error)) {
        /* Just log the error message */
        g_debug ("[qfu-image-cwe] couldn't parse filename '%s': %s",
                 qfu_image_get_display_name (QFU_IMAGE (self)),
                 inner_error->message);
        g_clear_error (&inner_error);
    }

    /* If all retrieved with the filename, we're done */
    if (self->priv->firmware_version && self->priv->config_version && self->priv->carrier)
        goto done;

    /* Try with embedded images of type BOOT or NVU */
    for (i = 0; i < self->priv->images->len; i++) {
        ImageInfo *info;

        info = &g_array_index (self->priv->images, ImageInfo, i);

        /* BOOT partition in system images won't likely contain anything else
         * than firmware version */
        if (!g_strcmp0 (info->type, "BOOT") && !self->priv->firmware_version) {
            if (!qfu_utils_parse_cwe_version_string (
                    info->hdr.version,
                    &self->priv->firmware_version,
                    NULL,
                    NULL,
                    &inner_error)) {
                /* Just log the error message */
                g_debug ("[qfu-image-cwe] couldn't parse BOOT version '%s': %s",
                         qfu_image_get_display_name (QFU_IMAGE (self)),
                         inner_error->message);
                g_clear_error (&inner_error);
            }
        }

        /* NVUP partition in nvu images are usually carrier-specific */
        if (!g_strcmp0 (info->type, "NVUP")) {
            if (!qfu_utils_parse_cwe_version_string (
                    info->hdr.version,
                    self->priv->firmware_version ? NULL : &self->priv->firmware_version,
                    self->priv->config_version   ? NULL : &self->priv->config_version,
                    self->priv->carrier          ? NULL : &self->priv->carrier,
                    &inner_error)) {
                /* Just log the error message */
                g_debug ("[qfu-image-cwe] couldn't parse NVUP version '%s': %s",
                         qfu_image_get_display_name (QFU_IMAGE (self)),
                         inner_error->message);
                g_clear_error (&inner_error);
            }
        }

        /* As soon as all retrieved, we're done */
        if (self->priv->firmware_version && self->priv->config_version && self->priv->carrier)
            goto done;
    }

done:
    g_debug ("[qfu-image-cwe]   firmware version: %s", self->priv->firmware_version ? self->priv->firmware_version : "unknown");
    g_debug ("[qfu-image-cwe]   config version:   %s", self->priv->config_version   ? self->priv->config_version   : "unknown");
    g_debug ("[qfu-image-cwe]   carrier:          %s", self->priv->carrier          ? self->priv->carrier          : "unknown");
}

const gchar *
qfu_image_cwe_get_parsed_firmware_version (QfuImageCwe *self)
{
    g_return_val_if_fail (QFU_IS_IMAGE_CWE (self), NULL);

    return self->priv->firmware_version;
}

const gchar *
qfu_image_cwe_get_parsed_config_version (QfuImageCwe *self)
{
    g_return_val_if_fail (QFU_IS_IMAGE_CWE (self), NULL);

    return self->priv->config_version;
}

const gchar *
qfu_image_cwe_get_parsed_carrier (QfuImageCwe *self)
{
    g_return_val_if_fail (QFU_IS_IMAGE_CWE (self), NULL);

    return self->priv->carrier;
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
    ImageInfo   *info;

    if (out_buffer_size < sizeof (QfuCweFileHeader)) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "buffer too small to read header");
        return -1;
    }

    info = &g_array_index (self->priv->images, ImageInfo, 0);
    memcpy (out_buffer, &(info->hdr), sizeof (QfuCweFileHeader));
    return sizeof (QfuCweFileHeader);
}

/******************************************************************************/

static void
clear_image_info (ImageInfo *info)
{
    g_free (info->type);
    g_free (info->product);
}

static gboolean
is_ascii_str (const gchar *str,
              guint        str_len)
{
    guint i;
    guint real_len = 0;

    for (i = 0; (str[i] != '\0') && (i < str_len); i++)
        real_len++;

    /* All valid characters need to be printable */
    for (i = 0; i < real_len; i++) {
        if (!g_ascii_isprint (str[i]))
            return FALSE;
    }

    /* All remaining characters need to be zero */
    for (i = real_len; i < str_len; i++) {
        if (str[i] != '\0')
            return FALSE;
    }

    return TRUE;
}

static gboolean
load_image_info (QfuImageCwe   *self,
                 GInputStream  *input_stream,
                 const gchar   *parent_prefix,
                 gint           parent_image_index,
                 goffset        parent_image_end_offset,
                 GCancellable  *cancellable,
                 GError       **error)
{
    ImageInfo  info;
    gssize     n_read;
    gchar     *image_prefix;
    guint      image_index;
    goffset    image_start_offset;
    goffset    image_end_offset;
    goffset    walker;

    /* Store image start offset */
    image_start_offset = g_seekable_tell (G_SEEKABLE (input_stream));

    memset (&info, 0, sizeof (info));

    /* Store parent image index */
    info.parent_image_index = parent_image_index;

    /* Read header from file */
    n_read = g_input_stream_read (input_stream, &(info.hdr), sizeof (QfuCweFileHeader), cancellable, error);
    if (n_read < 0) {
        g_prefix_error (error, "couldn't read file header: ");
        return FALSE;
    }
    if (n_read != sizeof (QfuCweFileHeader)) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "CWE firmware image file is too short: full header not available");
        return FALSE;
    }

    /* No image size reported */
    if (!info.hdr.imgsize) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "invalid image size");
        return FALSE;
    }

    /* Check limits of the current image */
    image_end_offset = image_start_offset + GUINT32_FROM_BE (info.hdr.imgsize) + sizeof (QfuCweFileHeader);
    if (parent_image_end_offset > 0 && parent_image_end_offset < image_end_offset) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "embedded image out of parent image bounds");
        return FALSE;
    }

    /* Validate strings */
    if (!is_ascii_str (info.hdr.type,    sizeof (info.hdr.type)) ||
        !is_ascii_str (info.hdr.product, sizeof (info.hdr.product)) ||
        !is_ascii_str (info.hdr.version, sizeof (info.hdr.version)) ||
        !is_ascii_str (info.hdr.date,    sizeof (info.hdr.date))) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "invalid strings given in image");
        return FALSE;
    }

    /* Preload non-NUL terminated strings */
    info.type    = g_strndup (info.hdr.type,    sizeof (info.hdr.type));
    info.product = g_strndup (info.hdr.product, sizeof (info.hdr.product));

    /* Valid image! We'll append to the array */
    image_index = self->priv->images->len;
    g_array_insert_val (self->priv->images, image_index, info);

    g_debug ("[qfu-image-cwe] %simage offset range: [%" G_GOFFSET_FORMAT ",%" G_GOFFSET_FORMAT "]",
             parent_prefix, image_start_offset, image_end_offset);

    /* And check if it has embedded images */
    image_prefix = g_strdup_printf ("%s  ", parent_prefix);
    walker = image_start_offset;
    while (walker < image_end_offset) {
        goffset tested_offset;
        /* Read embedded image */
        tested_offset = g_seekable_tell (G_SEEKABLE (input_stream));
        if (!load_image_info (self, input_stream, image_prefix, image_index, image_end_offset, cancellable, NULL))
            break;
        g_debug ("[qfu-image-cwe] %simage at offset %" G_GOFFSET_FORMAT " is valid", parent_prefix, tested_offset);
        walker = g_seekable_tell (G_SEEKABLE (input_stream));
    }
    g_free (image_prefix);

    /* Finally, seek to just after this image */
    if (!g_seekable_seek (G_SEEKABLE (input_stream), image_end_offset, G_SEEK_SET, cancellable, error)) {
        g_prefix_error (error, "couldn't seek after image: ");
        return FALSE;
    }

    return TRUE;
}

static gboolean
initable_init (GInitable     *initable,
               GCancellable  *cancellable,
               GError       **error)
{
    QfuImageCwe  *self;
    GInputStream *input_stream = NULL;
    gboolean      result = FALSE;

    self = QFU_IMAGE_CWE (initable);

    /* Run parent initable */
    if (!iface_initable_parent->init (initable, cancellable, error))
        return FALSE;

    g_object_get (self, "input-stream", &input_stream, NULL);
    g_assert (G_IS_FILE_INPUT_STREAM (input_stream));

    g_debug ("[qfu-image-cwe] reading image headers...");
    if (!g_seekable_seek (G_SEEKABLE (input_stream), 0, G_SEEK_SET, cancellable, error)) {
        g_prefix_error (error, "couldn't seek input stream: ");
        goto out;
    }
    if (!load_image_info (self, input_stream, "", -1, (goffset) -1, cancellable, error)) {
        g_prefix_error (error, "couldn't read file header: ");
        goto out;
    }

    g_debug ("[qfu-image-cwe] validating data size...");
    if (qfu_image_get_data_size (QFU_IMAGE (self)) != qfu_image_cwe_header_get_image_size (self)) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "CWE image file size mismatch (expected size: %" G_GUINT32_FORMAT " bytes, real size: %" G_GOFFSET_FORMAT " bytes)",
                     qfu_image_cwe_header_get_image_size (self), qfu_image_get_data_size (QFU_IMAGE (self)));
        goto out;
    }

    g_debug ("[qfu-image-cwe] preloading firmware/config/carrier...");
    parse_firmware_config_carrier (self);

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

    self->priv->images = g_array_new (FALSE, FALSE, sizeof (ImageInfo));
    g_array_set_clear_func (self->priv->images, (GDestroyNotify) clear_image_info);
}

static void
finalize (GObject *object)
{
    QfuImageCwe *self = QFU_IMAGE_CWE (object);

    g_free (self->priv->firmware_version);
    g_free (self->priv->config_version);
    g_free (self->priv->carrier);
    g_array_unref (self->priv->images);

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
