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
 * Copyright (C) 2019 Zodiac Inflight Innovations
 * Copyright (C) 2019 Aleksander Morgado <aleksander@aleksander.es>
 */

#ifndef QFU_SAHARA_DEVICE_H
#define QFU_SAHARA_DEVICE_H

#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define QFU_TYPE_SAHARA_DEVICE            (qfu_sahara_device_get_type ())
#define QFU_SAHARA_DEVICE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), QFU_TYPE_SAHARA_DEVICE, QfuSaharaDevice))
#define QFU_SAHARA_DEVICE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  QFU_TYPE_SAHARA_DEVICE, QfuSaharaDeviceClass))
#define QFU_IS_SAHARA_DEVICE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), QFU_TYPE_SAHARA_DEVICE))
#define QFU_IS_SAHARA_DEVICE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  QFU_TYPE_SAHARA_DEVICE))
#define QFU_SAHARA_DEVICE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  QFU_TYPE_SAHARA_DEVICE, QfuSaharaDeviceClass))

typedef struct _QfuSaharaDevice        QfuSaharaDevice;
typedef struct _QfuSaharaDeviceClass   QfuSaharaDeviceClass;
typedef struct _QfuSaharaDevicePrivate QfuSaharaDevicePrivate;

struct _QfuSaharaDevice {
    GObject parent;
    QfuSaharaDevicePrivate *priv;
};

struct _QfuSaharaDeviceClass {
    GObjectClass parent;
};

GType qfu_sahara_device_get_type (void);
G_DEFINE_AUTOPTR_CLEANUP_FUNC (QfuSaharaDevice, g_object_unref);

QfuSaharaDevice *qfu_sahara_device_new                        (GFile            *file,
                                                               GCancellable     *cancellable,
                                                               GError          **error);
gboolean         qfu_sahara_device_firehose_setup_download    (QfuSaharaDevice  *self,
                                                               QfuImage         *image,
                                                               guint            *n_blocks,
                                                               GCancellable     *cancellable,
                                                               GError          **error);
gboolean         qfu_sahara_device_firehose_write_block       (QfuSaharaDevice  *self,
                                                               QfuImage         *image,
                                                               guint             block_i,
                                                               GCancellable     *cancellable,
                                                               GError          **error);
gboolean         qfu_sahara_device_firehose_teardown_download (QfuSaharaDevice  *self,
                                                               QfuImage         *image,
                                                               GCancellable     *cancellable,
                                                               GError          **error);
gboolean         qfu_sahara_device_firehose_reset             (QfuSaharaDevice  *self,
                                                               GCancellable     *cancellable,
                                                               GError          **error);

G_END_DECLS

#endif /* QFU_SAHARA_DEVICE_H */
