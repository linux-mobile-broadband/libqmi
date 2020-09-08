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
 * Copyright (C) 2012 Lanedo GmbH
 * Copyright (C) 2012-2017 Aleksander Morgado <aleksander@aleksander.es>
 * Copyright (C) 2019 Eric Caruso <ejcaruso@chromium.org>
 */

#include "qmi-file.h"

#include "qmi-error-types.h"
#include "qmi-errors.h"

G_DEFINE_TYPE (QmiFile, qmi_file, G_TYPE_OBJECT)

struct _QmiFilePrivate {
    GFile    *file;
    gboolean  is_uri;
    gchar    *path;
    gchar    *path_display;
};

/*****************************************************************************/

GFile *
qmi_file_get_file (QmiFile *self)
{
    return g_object_ref (self->priv->file);
}

GFile *
qmi_file_peek_file (QmiFile *self)
{
    return self->priv->file;
}

const gchar *
qmi_file_get_path (QmiFile *self)
{
    return self->priv->path;
}

const gchar *
qmi_file_get_path_display (QmiFile *self)
{
    return self->priv->path_display;
}

/*****************************************************************************/

gboolean
qmi_file_check_type_finish (QmiFile       *self,
                            GAsyncResult  *res,
                            GError       **error)
{
    return g_task_propagate_boolean (G_TASK (res), error);
}

static void
query_info_async_ready (GFile        *file,
                        GAsyncResult *res,
                        GTask        *task)
{
    GError *error = NULL;
    GFileInfo *info;
    GFileType file_type;

    info = g_file_query_info_finish (file, res, &error);
    if (!info) {
        g_prefix_error (&error,
                        "Couldn't query file info: ");
        g_task_return_error (task, error);
        g_object_unref (task);
        return;
    }

    file_type = g_file_info_get_file_type (info);
    g_object_unref (info);

    if (file_type != G_FILE_TYPE_SPECIAL) {
        g_task_return_new_error (task,
                                 QMI_CORE_ERROR,
                                 QMI_CORE_ERROR_FAILED,
                                 "Wrong file type");
        g_object_unref (task);
        return;
    }

    g_task_return_boolean (task, TRUE);
    g_object_unref (task);
}

void
qmi_file_check_type_async (QmiFile             *self,
                           GCancellable        *cancellable,
                           GAsyncReadyCallback  callback,
                           gpointer             user_data)
{
    GTask *task;

    task = g_task_new (self, cancellable, callback, user_data);

    /* If this file only has a URI and no local path, then it refers to some
     * non-filesystem resource, such as something on a device bus. We can't
     * figure out if it's the right type in this case, so we just try to see
     * if it's OK later. */
    if (self->priv->is_uri) {
        g_task_return_boolean (task, TRUE);
        g_object_unref (task);
        return;
    }

    /* Check the file type. Note that this is just a quick check to avoid
     * creating QmiDevices pointing to a location already known not to be a QMI
     * device. */
    g_file_query_info_async (self->priv->file,
                             G_FILE_ATTRIBUTE_STANDARD_TYPE,
                             G_FILE_QUERY_INFO_NONE,
                             G_PRIORITY_DEFAULT,
                             cancellable,
                             (GAsyncReadyCallback)query_info_async_ready,
                             task);
}

/*****************************************************************************/

QmiFile *
qmi_file_new (GFile *file)
{
    QmiFile *self;
    gchar   *path;

    g_assert (G_IS_FILE (file));

    self = g_object_new (QMI_TYPE_FILE, NULL);
    self->priv->file = g_object_ref (file);

    path = g_file_get_path (self->priv->file);
    if (path) {
        self->priv->is_uri = FALSE;
        self->priv->path = path;
        self->priv->path_display = g_filename_display_name (self->priv->path);
    } else {
        self->priv->is_uri = TRUE;
        self->priv->path = g_file_get_uri (self->priv->file);
        self->priv->path_display = g_strdup (self->priv->path);
    }

    return self;
}

/*****************************************************************************/

static void
qmi_file_init (QmiFile *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, QMI_TYPE_FILE, QmiFilePrivate);
}

static void
dispose (GObject *object)
{
    QmiFile *self = QMI_FILE (object);

    g_clear_object (&self->priv->file);
    g_clear_pointer (&self->priv->path, g_free);
    g_clear_pointer (&self->priv->path_display, g_free);

    G_OBJECT_CLASS (qmi_file_parent_class)->dispose (object);
}

static void
qmi_file_class_init (QmiFileClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (object_class, sizeof (QmiFilePrivate));

    object_class->dispose = dispose;
}
