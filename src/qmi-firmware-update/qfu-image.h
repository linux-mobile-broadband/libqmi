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

#ifndef QFU_IMAGE_H
#define QFU_IMAGE_H

#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

/* Most of these origin from GobiAPI_1.0.40/Core/QDLEnum.h
 *
 * The gobi-loader's snooped magic strings use types
 *   0x05 => "amss.mbn"
 *   0x06 => "apps.mbn"
 *   0x0d => "uqcn.mbn" (Gobi 2000 only)
 *  with no file header data
 *
 * The 0x80 type is snooped from the Sierra Wireless firmware
 * uploaders, using 400 bytes file header data
 */
typedef enum {
    QFU_IMAGE_TYPE_UNKNOWN          = 0x00,
    QFU_IMAGE_TYPE_AMSS_MODEM       = 0x05,
    QFU_IMAGE_TYPE_AMSS_APPLICATION = 0x06,
    QFU_IMAGE_TYPE_AMSS_UQCN        = 0x0d,
    QFU_IMAGE_TYPE_DBL              = 0x0f,
    QFU_IMAGE_TYPE_OSBL             = 0x10,
    QFU_IMAGE_TYPE_CWE              = 0x80,
} QfuImageType;

/* Default chunk size */
#define QFU_IMAGE_CHUNK_SIZE (1024 * 1024)

#define QFU_TYPE_IMAGE            (qfu_image_get_type ())
#define QFU_IMAGE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), QFU_TYPE_IMAGE, QfuImage))
#define QFU_IMAGE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  QFU_TYPE_IMAGE, QfuImageClass))
#define QFU_IS_IMAGE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), QFU_TYPE_IMAGE))
#define QFU_IS_IMAGE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  QFU_TYPE_IMAGE))
#define QFU_IMAGE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  QFU_TYPE_IMAGE, QfuImageClass))

typedef struct _QfuImage        QfuImage;
typedef struct _QfuImageClass   QfuImageClass;
typedef struct _QfuImagePrivate QfuImagePrivate;

struct _QfuImage {
    GObject          parent;
    QfuImagePrivate *priv;
};

struct _QfuImageClass {
    GObjectClass parent;

    goffset (* get_header_size) (QfuImage      *self);
    gssize  (* read_header)     (QfuImage      *self,
                                 guint8        *out_buffer,
                                 gsize          out_buffer_size,
                                 GCancellable  *cancellable,
                                 GError       **error);
    goffset (* get_data_size)   (QfuImage      *self);
};

GType qfu_image_get_type (void);
G_DEFINE_AUTOPTR_CLEANUP_FUNC (QfuImage, g_object_unref);

QfuImage     *qfu_image_new                 (GFile         *file,
                                             QfuImageType   image_type,
                                             GCancellable  *cancellable,
                                             GError       **error);
QfuImageType  qfu_image_get_image_type      (QfuImage      *self);
const gchar  *qfu_image_get_display_name    (QfuImage      *self);
goffset       qfu_image_get_size            (QfuImage      *self);
goffset       qfu_image_get_header_size     (QfuImage      *self);
gssize        qfu_image_read_header         (QfuImage      *self,
                                             guint8        *out_buffer,
                                             gsize          out_buffer_size,
                                             GCancellable  *cancellable,
                                             GError       **error);
goffset       qfu_image_get_data_size       (QfuImage      *self);
guint16       qfu_image_get_n_data_chunks   (QfuImage      *self);
gsize         qfu_image_get_data_chunk_size (QfuImage      *self,
                                             guint16        chunk_i);
gssize        qfu_image_read_data_chunk     (QfuImage      *self,
                                             guint16        chunk_i,
                                             guint8        *out_buffer,
                                             gsize          out_buffer_size,
                                             GCancellable  *cancellable,
                                             GError       **error);
gssize        qfu_image_read                (QfuImage      *self,
                                             goffset        offset,
                                             gsize          size,
                                             guint8        *out_buffer,
                                             gsize          out_buffer_size,
                                             GCancellable  *cancellable,
                                             GError       **error);

G_END_DECLS

#endif /* QFU_IMAGE_H */
