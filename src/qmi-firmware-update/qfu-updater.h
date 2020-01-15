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
 * Copyright (C) 2016 Zodiac Inflight Innovations
 * Copyright (C) 2016-2017 Aleksander Morgado <aleksander@aleksander.es>
 */

#ifndef QFU_UPDATER_H
#define QFU_UPDATER_H

#include <glib-object.h>
#include <gio/gio.h>

#include "qfu-device-selection.h"

G_BEGIN_DECLS

#define QFU_TYPE_UPDATER            (qfu_updater_get_type ())
#define QFU_UPDATER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), QFU_TYPE_UPDATER, QfuUpdater))
#define QFU_UPDATER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  QFU_TYPE_UPDATER, QfuUpdaterClass))
#define QFU_IS_UPDATER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), QFU_TYPE_UPDATER))
#define QFU_IS_UPDATER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  QFU_TYPE_UPDATER))
#define QFU_UPDATER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  QFU_TYPE_UPDATER, QfuUpdaterClass))

typedef struct _QfuUpdater QfuUpdater;
typedef struct _QfuUpdaterClass QfuUpdaterClass;
typedef struct _QfuUpdaterPrivate QfuUpdaterPrivate;

struct _QfuUpdater {
    GObject            parent;
    QfuUpdaterPrivate *priv;
};

struct _QfuUpdaterClass {
    GObjectClass parent;
};

GType qfu_updater_get_type (void);
G_DEFINE_AUTOPTR_CLEANUP_FUNC (QfuUpdater, g_object_unref);

#if defined WITH_UDEV
QfuUpdater *qfu_updater_new          (QfuDeviceSelection   *device_selection,
                                      const gchar          *firmware_version,
                                      const gchar          *config_version,
                                      const gchar          *carrier,
                                      QmiDeviceOpenFlags    device_open_flags,
                                      gboolean              ignore_version_errors,
                                      gboolean              override_download,
                                      guint8                modem_storage_index,
                                      gboolean              skip_validation);
#endif

QfuUpdater *qfu_updater_new_download (QfuDeviceSelection   *device_selection);
void        qfu_updater_run          (QfuUpdater           *self,
                                      GList                *image_file_list,
                                      GCancellable         *cancellable,
                                      GAsyncReadyCallback   callback,
                                      gpointer              user_data);
gboolean    qfu_updater_run_finish   (QfuUpdater           *self,
                                      GAsyncResult         *res,
                                      GError              **error);

G_END_DECLS

#endif /* QFU_UPDATER_H */
