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
 * Copyright (C) 2019 Eric Caruso <ejcaruso@chromium.org>
 */

#ifndef _LIBQMI_GLIB_QMI_FILE_H_
#define _LIBQMI_GLIB_QMI_FILE_H_

#include <glib-object.h>
#include <gio/gio.h>

#define QMI_TYPE_FILE            (qmi_file_get_type ())
#define QMI_FILE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), QMI_TYPE_FILE, QmiFile))
#define QMI_FILE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  QMI_TYPE_FILE, QmiFileClass))
#define QMI_IS_FILE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), QMI_TYPE_FILE))
#define QMI_IS_FILE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  QMI_TYPE_FILE))
#define QMI_FILE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  QMI_TYPE_FILE, QmiFileClass))

typedef struct _QmiFile QmiFile;
typedef struct _QmiFileClass QmiFileClass;
typedef struct _QmiFilePrivate QmiFilePrivate;

struct _QmiFile {
    /*< private >*/
    GObject parent;
    QmiFilePrivate *priv;
};

struct _QmiFileClass {
    /*< private >*/
    GObjectClass parent;
};

GType qmi_file_get_type (void);
G_DEFINE_AUTOPTR_CLEANUP_FUNC (QmiFile, g_object_unref)

QmiFile     *qmi_file_new               (GFile *file);
GFile       *qmi_file_get_file          (QmiFile              *self);
GFile       *qmi_file_peek_file         (QmiFile              *self);
const gchar *qmi_file_get_path          (QmiFile              *self);
const gchar *qmi_file_get_path_display  (QmiFile              *self);
void         qmi_file_check_type_async  (QmiFile              *self,
                                         GCancellable         *cancellable,
                                         GAsyncReadyCallback   callback,
                                         gpointer              user_data);
gboolean     qmi_file_check_type_finish (QmiFile              *self,
                                         GAsyncResult         *res,
                                         GError              **error);

#endif /* _LIBQMI_GLIB_QMI_FILE_H_ */
