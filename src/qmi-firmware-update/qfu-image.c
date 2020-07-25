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

#include "qfu-image.h"
#include "qfu-enum-types.h"

static void initable_iface_init (GInitableIface *iface);

G_DEFINE_TYPE_EXTENDED (QfuImage, qfu_image, G_TYPE_OBJECT, 0,
                        G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE, initable_iface_init))

enum {
    PROP_0,
    PROP_FILE,
    PROP_IMAGE_TYPE,
    PROP_INPUT_STREAM,
    PROP_LAST
};

static GParamSpec *properties[PROP_LAST];

struct _QfuImagePrivate {
    QfuImageType  image_type;
    GFile        *file;
    GFileInfo    *info;
    GInputStream *input_stream;
};

/******************************************************************************/

gsize
qfu_image_get_data_chunk_size (QfuImage *self,
                               guint16   chunk_i)
{
    gssize chunk_size = QFU_IMAGE_CHUNK_SIZE;
    guint  n_chunks;

    n_chunks = qfu_image_get_n_data_chunks (self);
    g_assert (chunk_i < n_chunks);

    if ((guint)chunk_i == (n_chunks - 1)) {
        chunk_size = qfu_image_get_data_size (self) - ((gssize)chunk_i * QFU_IMAGE_CHUNK_SIZE);
        g_assert (chunk_size > 0);
    } else
        chunk_size = QFU_IMAGE_CHUNK_SIZE;

    return chunk_size;
}

gssize
qfu_image_read_data_chunk (QfuImage      *self,
                           guint16        chunk_i,
                           guint8        *out_buffer,
                           gsize          out_buffer_size,
                           GCancellable  *cancellable,
                           GError       **error)
{
    gsize   chunk_size;
    guint   n_chunks;
    goffset chunk_offset;
    gssize  n_read;

    g_return_val_if_fail (QFU_IS_IMAGE (self), -1);

    g_debug ("[qfu-image] reading chunk #%u", chunk_i);

    n_chunks = qfu_image_get_n_data_chunks (self);
    if (chunk_i >= n_chunks) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "invalid chunk index %u", chunk_i);
        return -1;
    }

    /* Last chunk may be shorter than the default */
    chunk_size = qfu_image_get_data_chunk_size (self, chunk_i);
    g_debug ("[qfu-image] chunk #%u size: %" G_GSIZE_FORMAT " bytes", chunk_i, chunk_size);

    /* Make sure there's enough room */
    if (out_buffer_size < chunk_size) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "buffer too small to fit chunk size: %" G_GSIZE_FORMAT, chunk_size);
        return -1;
    }

    /* Compute chunk offset */
    chunk_offset = qfu_image_get_header_size (self) + ((goffset)chunk_i * QFU_IMAGE_CHUNK_SIZE);
    g_debug ("[qfu-image] chunk #%u offset: %" G_GOFFSET_FORMAT " bytes", chunk_i, chunk_offset);

    /* Seek to the correct place: note that this is likely a noop if already in that offset */
    if (!g_seekable_seek (G_SEEKABLE (self->priv->input_stream), chunk_offset, G_SEEK_SET, cancellable, error)) {
        g_prefix_error (error, "couldn't seek input stream: ");
        return -1;
    }

    /* Read full chunk */
    n_read = g_input_stream_read (self->priv->input_stream,
                                  out_buffer,
                                  chunk_size,
                                  cancellable,
                                  error);
    if (n_read < 0) {
        g_prefix_error (error, "couldn't read chunk %u", chunk_i);
        return -1;
    }

    if ((gsize)n_read != chunk_size) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "couldn't read expected chunk %u size: %" G_GSIZE_FORMAT " (%" G_GSSIZE_FORMAT " bytes read)",
                     chunk_i, chunk_size, n_read);
        return -1;
    }

    g_debug ("[qfu-image] chunk #%u successfully read", chunk_i);

    return chunk_size;
}

/******************************************************************************/

gssize
qfu_image_read (QfuImage      *self,
                goffset        offset,
                gsize          size,
                guint8        *out_buffer,
                gsize          out_buffer_size,
                GCancellable  *cancellable,
                GError       **error)
{
    gssize  n_read;
    goffset remaining_size;
    gsize   read_size;

    g_return_val_if_fail (QFU_IS_IMAGE (self), -1);

    remaining_size = qfu_image_get_size (self) - offset;
    g_assert (remaining_size >= 0);
    read_size = ((gsize)remaining_size > size) ? size : (gsize)remaining_size;

    g_debug ("[qfu-image] reading [%" G_GOFFSET_FORMAT "-%" G_GOFFSET_FORMAT "]",
             offset, offset + read_size);

    /* Make sure there's enough room */
    if (out_buffer_size < read_size) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "buffer too small to fit read size: %" G_GSIZE_FORMAT, read_size);
        return -1;
    }

    /* Seek to the correct place: note that this is likely a noop if already in that offset */
    if (!g_seekable_seek (G_SEEKABLE (self->priv->input_stream), offset, G_SEEK_SET, cancellable, error)) {
        g_prefix_error (error, "couldn't seek input stream: ");
        return -1;
    }

    /* Read full chunk */
    n_read = g_input_stream_read (self->priv->input_stream,
                                  out_buffer,
                                  read_size,
                                  cancellable,
                                  error);
    if (n_read < 0) {
        g_prefix_error (error, "couldn't read data at offset %" G_GOFFSET_FORMAT, offset);
        return -1;
    }

    if ((gsize)n_read != read_size) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "couldn't read expected data size: %" G_GSIZE_FORMAT " (%" G_GSSIZE_FORMAT " bytes read)",
                     read_size, n_read);
        return -1;
    }

    g_debug ("[qfu-image] data at offset %" G_GOFFSET_FORMAT " successfully read", offset);

    return read_size;
}

/******************************************************************************/

QfuImageType
qfu_image_get_image_type (QfuImage *self)
{
    g_return_val_if_fail (QFU_IS_IMAGE (self), QFU_IMAGE_TYPE_UNKNOWN);

    return self->priv->image_type;
}

const gchar *
qfu_image_get_display_name (QfuImage *self)
{
    g_return_val_if_fail (QFU_IS_IMAGE (self), NULL);

    return g_file_info_get_display_name (self->priv->info);
}

goffset
qfu_image_get_size (QfuImage *self)
{
    g_return_val_if_fail (QFU_IS_IMAGE (self), 0);

    return g_file_info_get_size (self->priv->info);
}

goffset
qfu_image_get_header_size (QfuImage *self)
{
    g_return_val_if_fail (QFU_IS_IMAGE (self), 0);

    return QFU_IMAGE_GET_CLASS (self)->get_header_size (self);
}

gssize
qfu_image_read_header (QfuImage      *self,
                       guint8        *out_buffer,
                       gsize          out_buffer_size,
                       GCancellable  *cancellable,
                       GError       **error)
{
    g_return_val_if_fail (QFU_IS_IMAGE (self), 0);

    return (QFU_IMAGE_GET_CLASS (self)->read_header ?
            QFU_IMAGE_GET_CLASS (self)->read_header (self, out_buffer, out_buffer_size, cancellable, error) :
            0);
}

goffset
qfu_image_get_data_size (QfuImage *self)
{
    g_return_val_if_fail (QFU_IS_IMAGE (self), 0);

    return QFU_IMAGE_GET_CLASS (self)->get_data_size (self);
}

guint16
qfu_image_get_n_data_chunks (QfuImage *self)
{
    goffset data_size;

    data_size = qfu_image_get_data_size (self);
    g_assert (data_size <= ((goffset) G_MAXUINT16 * (goffset) QFU_IMAGE_CHUNK_SIZE));

    return (guint16) (data_size / QFU_IMAGE_CHUNK_SIZE) + !!(data_size % QFU_IMAGE_CHUNK_SIZE);
}

/******************************************************************************/

static goffset
get_header_size (QfuImage *self)
{
    return 0;
}

static goffset
get_data_size (QfuImage *self)
{
    goffset file_size;

    file_size = qfu_image_get_size (self);

    /* some image types contain trailing garbage - from gobi-loader */
    if (self->priv->image_type == QFU_IMAGE_TYPE_AMSS_MODEM)
        file_size -= 8;

    return file_size;
}

/******************************************************************************/

static gboolean
initable_init (GInitable     *initable,
               GCancellable  *cancellable,
               GError       **error)
{
    QfuImage *self;

    self = QFU_IMAGE (initable);
    g_assert (self->priv->file);

    /* Load file info */
    g_debug ("[qfu-image] loading file info...");
    self->priv->info = g_file_query_info (self->priv->file,
                                          G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME "," G_FILE_ATTRIBUTE_STANDARD_SIZE,
                                          G_FILE_QUERY_INFO_NONE,
                                          cancellable,
                                          error);
    if (!self->priv->info)
        return FALSE;

    /* Check minimum file size */
    if (qfu_image_get_size (self) < qfu_image_get_header_size (self)) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "image is too short");
        return FALSE;
    }

    /* Open file for reading. Kept open while the input stream reference is valid. */
    g_debug ("[qfu-image] opening file for reading...");
    self->priv->input_stream = G_INPUT_STREAM (g_file_read (self->priv->file, cancellable, error));
    if (!self->priv->input_stream)
        return FALSE;

    return TRUE;
}

/******************************************************************************/

QfuImage *
qfu_image_new (GFile         *file,
               QfuImageType   image_type,
               GCancellable  *cancellable,
               GError       **error)
{
    g_return_val_if_fail (G_IS_FILE (file), NULL);

    return QFU_IMAGE (g_initable_new (QFU_TYPE_IMAGE,
                                      cancellable,
                                      error,
                                      "file",       file,
                                      "image-type", image_type,
                                      NULL));
}

static void
qfu_image_init (QfuImage *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, QFU_TYPE_IMAGE, QfuImagePrivate);
    self->priv->image_type = QFU_IMAGE_TYPE_UNKNOWN;
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    QfuImage *self = QFU_IMAGE (object);

    switch (prop_id) {
    case PROP_FILE:
        self->priv->file = g_value_dup_object (value);
        break;
    case PROP_IMAGE_TYPE:
        self->priv->image_type = g_value_get_enum (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
get_property (GObject    *object,
              guint       prop_id,
              GValue     *value,
              GParamSpec *pspec)
{
    QfuImage *self = QFU_IMAGE (object);

    switch (prop_id) {
    case PROP_FILE:
        g_value_set_object (value, self->priv->file);
        break;
    case PROP_IMAGE_TYPE:
        g_value_set_enum (value, self->priv->image_type);
        break;
    case PROP_INPUT_STREAM:
        g_value_set_object (value, self->priv->input_stream);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
dispose (GObject *object)
{
    QfuImage *self = QFU_IMAGE (object);

    g_clear_object (&self->priv->input_stream);
    g_clear_object (&self->priv->info);
    g_clear_object (&self->priv->file);

    G_OBJECT_CLASS (qfu_image_parent_class)->dispose (object);
}

static void
initable_iface_init (GInitableIface *iface)
{
    iface->init = initable_init;
}

static void
qfu_image_class_init (QfuImageClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (object_class, sizeof (QfuImagePrivate));

    object_class->dispose      = dispose;
    object_class->get_property = get_property;
    object_class->set_property = set_property;

    klass->get_header_size = get_header_size;
    klass->get_data_size   = get_data_size;

    properties[PROP_FILE] =
        g_param_spec_object ("file",
                             "File",
                             "File object",
                             G_TYPE_FILE,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    g_object_class_install_property (object_class, PROP_FILE, properties[PROP_FILE]);

    properties[PROP_IMAGE_TYPE] =
        g_param_spec_enum ("image-type",
                           "Image type",
                           "Type of firmware image",
                           QFU_TYPE_IMAGE_TYPE,
                           QFU_IMAGE_TYPE_UNKNOWN,
                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    g_object_class_install_property (object_class, PROP_IMAGE_TYPE, properties[PROP_IMAGE_TYPE]);

    properties[PROP_INPUT_STREAM] =
        g_param_spec_object ("input-stream",
                             "Input stream",
                             "Input stream object",
                             G_TYPE_FILE_INPUT_STREAM,
                             G_PARAM_READABLE);
    g_object_class_install_property (object_class, PROP_INPUT_STREAM, properties[PROP_INPUT_STREAM]);
}
