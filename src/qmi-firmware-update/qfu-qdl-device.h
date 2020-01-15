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

#ifndef QFU_QDL_DEVICE_H
#define QFU_QDL_DEVICE_H

#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define QFU_TYPE_QDL_DEVICE            (qfu_qdl_device_get_type ())
#define QFU_QDL_DEVICE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), QFU_TYPE_QDL_DEVICE, QfuQdlDevice))
#define QFU_QDL_DEVICE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  QFU_TYPE_QDL_DEVICE, QfuQdlDeviceClass))
#define QFU_IS_QDL_DEVICE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), QFU_TYPE_QDL_DEVICE))
#define QFU_IS_QDL_DEVICE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  QFU_TYPE_QDL_DEVICE))
#define QFU_QDL_DEVICE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  QFU_TYPE_QDL_DEVICE, QfuQdlDeviceClass))

typedef struct _QfuQdlDevice        QfuQdlDevice;
typedef struct _QfuQdlDeviceClass   QfuQdlDeviceClass;
typedef struct _QfuQdlDevicePrivate QfuQdlDevicePrivate;

struct _QfuQdlDevice {
    GObject parent;
    QfuQdlDevicePrivate *priv;
};

struct _QfuQdlDeviceClass {
    GObjectClass parent;
};

GType qfu_qdl_device_get_type (void);
G_DEFINE_AUTOPTR_CLEANUP_FUNC (QfuQdlDevice, g_object_unref);

QfuQdlDevice *qfu_qdl_device_new       (GFile         *file,
                                        GCancellable  *cancellable,
                                        GError       **error);
gboolean      qfu_qdl_device_hello     (QfuQdlDevice  *self,
                                        GCancellable  *cancellable,
                                        GError       **error);
gboolean      qfu_qdl_device_ufopen    (QfuQdlDevice  *self,
                                        QfuImage      *image,
                                        GCancellable  *cancellable,
                                        GError       **error);
gboolean      qfu_qdl_device_ufwrite   (QfuQdlDevice  *self,
                                        QfuImage      *image,
                                        guint16        sequence,
                                        GCancellable  *cancellable,
                                        GError       **error);
gboolean      qfu_qdl_device_ufclose   (QfuQdlDevice  *self,
                                        GCancellable  *cancellable,
                                        GError       **error);
gboolean      qfu_qdl_device_reset     (QfuQdlDevice  *self,
                                        GCancellable  *cancellable,
                                        GError       **error);

G_END_DECLS

#endif /* QFU_QDL_DEVICE_H */
