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

/**
 * SECTION:qmi-file
 * @title: QmiFile
 * @short_description: Generic QMI USB file handling routines
 *
 * #QmiFile is a generic type representing a device node for a QMI-based modem.
 */

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

QmiFile *qmi_file_new (GFile *file);

/**
 * qmi_file_get_file:
 *
 * Get the #GFile associated with this #QmiFile.
 */
GFile *qmi_file_get_file (QmiFile *self);

/**
 * qmi_file_peek_file:
 *
 * Get the #GFile associated with this #QmiFile, without increasing the reference count
 * on the returned object.
 */
GFile *qmi_file_peek_file (QmiFile *self);

/**
 * qmi_file_get_path:
 *
 * Get the system path of the underlying QMI file.
 */
const gchar *qmi_file_get_path (QmiFile *self);

/**
 * qmi_file_get_path_display:
 *
 * Get the system path of the underlying QMI file in UTF-8.
 */
const gchar *qmi_file_get_path_display (QmiFile *self);

/**
 * qmi_file_check_type_async:
 *
 * Checks that the #GFile associated with this #QmiFile is a special file.
 */
void qmi_file_check_type_async (QmiFile             *self,
                                GCancellable        *cancellable,
                                GAsyncReadyCallback  callback,
                                gpointer             user_data);
gboolean qmi_file_check_type_finish (QmiFile       *self,
                                     GAsyncResult  *res,
                                     GError       **error);

#endif /* _LIBQMI_GLIB_QMI_FILE_H_ */
