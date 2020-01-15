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

#ifndef QFU_IMAGE_CWE_H
#define QFU_IMAGE_CWE_H

#include <glib-object.h>
#include <gio/gio.h>

#include "qfu-image.h"

G_BEGIN_DECLS

#define QFU_TYPE_IMAGE_CWE            (qfu_image_cwe_get_type ())
#define QFU_IMAGE_CWE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), QFU_TYPE_IMAGE_CWE, QfuImageCwe))
#define QFU_IMAGE_CWE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  QFU_TYPE_IMAGE_CWE, QfuImageCweClass))
#define QFU_IS_IMAGE_CWE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), QFU_TYPE_IMAGE_CWE))
#define QFU_IS_IMAGE_CWE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  QFU_TYPE_IMAGE_CWE))
#define QFU_IMAGE_CWE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  QFU_TYPE_IMAGE_CWE, QfuImageCweClass))

typedef struct _QfuImageCwe        QfuImageCwe;
typedef struct _QfuImageCweClass   QfuImageCweClass;
typedef struct _QfuImageCwePrivate QfuImageCwePrivate;

struct _QfuImageCwe {
    QfuImage            parent;
    QfuImageCwePrivate *priv;
};

struct _QfuImageCweClass {
    QfuImageClass parent;
};

GType qfu_image_cwe_get_type (void);
G_DEFINE_AUTOPTR_CLEANUP_FUNC (QfuImageCwe, g_object_unref);

QfuImage    *qfu_image_cwe_new                              (GFile         *file,
                                                             GCancellable  *cancellable,
                                                             GError       **error);

const gchar *qfu_image_cwe_header_get_type                  (QfuImageCwe   *self);
const gchar *qfu_image_cwe_header_get_product               (QfuImageCwe   *self);
const gchar *qfu_image_cwe_header_get_version               (QfuImageCwe   *self);
const gchar *qfu_image_cwe_header_get_date                  (QfuImageCwe   *self);
guint32      qfu_image_cwe_header_get_image_size            (QfuImageCwe   *self);

const gchar *qfu_image_cwe_get_parsed_firmware_version      (QfuImageCwe   *self);
const gchar *qfu_image_cwe_get_parsed_config_version        (QfuImageCwe   *self);
const gchar *qfu_image_cwe_get_parsed_carrier               (QfuImageCwe   *self);

guint        qfu_image_cwe_get_n_embedded_headers           (QfuImageCwe   *self);
gint         qfu_image_cwe_embedded_header_get_parent_index (QfuImageCwe   *self,
                                                             guint          embedded_i);
const gchar *qfu_image_cwe_embedded_header_get_type         (QfuImageCwe   *self,
                                                             guint          embedded_i);
const gchar *qfu_image_cwe_embedded_header_get_product      (QfuImageCwe   *self,
                                                             guint          embedded_i);
const gchar *qfu_image_cwe_embedded_header_get_version      (QfuImageCwe   *self,
                                                             guint          embedded_i);
const gchar *qfu_image_cwe_embedded_header_get_date         (QfuImageCwe   *self,
                                                             guint          embedded_i);
guint32      qfu_image_cwe_embedded_header_get_image_size   (QfuImageCwe   *self,
                                                             guint          embedded_i);

G_END_DECLS

#endif /* QFU_IMAGE_CWE_H */
