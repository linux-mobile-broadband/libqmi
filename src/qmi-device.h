/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * libqmi-glib -- GLib/GIO based library to control QMI devices
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
 * Copyright (C) 2012 Aleksander Morgado <aleksander@lanedo.com>
 */

#ifndef _LIBQMI_GLIB_QMI_DEVICE_H_
#define _LIBQMI_GLIB_QMI_DEVICE_H_

#include <glib-object.h>

G_BEGIN_DECLS

#define QMI_TYPE_DEVICE            (qmi_device_get_type ())
#define QMI_DEVICE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), QMI_TYPE_DEVICE, QmiDevice))
#define QMI_DEVICE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  QMI_TYPE_DEVICE, QmiDeviceClass))
#define QMI_IS_DEVICE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), QMI_TYPE_DEVICE))
#define QMI_IS_DEVICE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  QMI_TYPE_DEVICE))
#define QMI_DEVICE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  QMI_TYPE_DEVICE, QmiDeviceClass))

typedef struct _QmiDevice QmiDevice;
typedef struct _QmiDeviceClass QmiDeviceClass;
typedef struct _QmiDevicePrivate QmiDevicePrivate;

#define QMI_DEVICE_FILE "device-file"

struct _QmiDevice {
    GObject parent;
    QmiDevicePrivate *priv;
};

struct _QmiDeviceClass {
    GObjectClass parent;
};

GType qmi_device_get_type (void);

void       qmi_device_new       (GFile *file,
                                 GCancellable *cancellable,
                                 GAsyncReadyCallback callback,
                                 gpointer user_data);
QmiDevice *qmi_device_new_finish (GAsyncResult *res,
                                  GError **error);

GFile       *qmi_device_get_file         (QmiDevice *self);
GFile       *qmi_device_peek_file        (QmiDevice *self);
const gchar *qmi_device_get_path         (QmiDevice *self);
const gchar *qmi_device_get_path_display (QmiDevice *self);
gboolean     qmi_device_is_open          (QmiDevice *self);

void         qmi_device_open        (QmiDevice *self,
                                     GCancellable *cancellable,
                                     GAsyncReadyCallback callback,
                                     gpointer user_data);
gboolean     qmi_device_open_finish (QmiDevice *self,
                                     GAsyncResult *res,
                                     GError **error);

gboolean     qmi_device_close (QmiDevice *self,
                               GError **error);

G_END_DECLS

#endif /* _LIBQMI_GLIB_QMI_DEVICE_H_ */
