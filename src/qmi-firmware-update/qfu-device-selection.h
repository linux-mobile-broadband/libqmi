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
 * Copyright (C) 2017 Zodiac Inflight Innovations
 * Copyright (C) 2017 Aleksander Morgado <aleksander@aleksander.es>
 */

#ifndef QFU_DEVICE_SELECTION_H
#define QFU_DEVICE_SELECTION_H

#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define QFU_TYPE_DEVICE_SELECTION            (qfu_device_selection_get_type ())
#define QFU_DEVICE_SELECTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), QFU_TYPE_DEVICE_SELECTION, QfuDeviceSelection))
#define QFU_DEVICE_SELECTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  QFU_TYPE_DEVICE_SELECTION, QfuDeviceSelectionClass))
#define QFU_IS_DEVICE_SELECTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), QFU_TYPE_DEVICE_SELECTION))
#define QFU_IS_DEVICE_SELECTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  QFU_TYPE_DEVICE_SELECTION))
#define QFU_DEVICE_SELECTION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  QFU_TYPE_DEVICE_SELECTION, QfuDeviceSelectionClass))

typedef struct _QfuDeviceSelection        QfuDeviceSelection;
typedef struct _QfuDeviceSelectionClass   QfuDeviceSelectionClass;
typedef struct _QfuDeviceSelectionPrivate QfuDeviceSelectionPrivate;

struct _QfuDeviceSelection {
    GObject                    parent;
    QfuDeviceSelectionPrivate *priv;
};

struct _QfuDeviceSelectionClass {
    GObjectClass parent;
};

GType qfu_device_selection_get_type (void);
G_DEFINE_AUTOPTR_CLEANUP_FUNC (QfuDeviceSelection, g_object_unref);

QfuDeviceSelection *qfu_device_selection_new      (const gchar  *preferred_cdc_wdm,
                                                   const gchar  *preferred_tty,
                                                   guint16       preferred_vid,
                                                   guint16       preferred_pid,
                                                   guint         preferred_busnum,
                                                   guint         preferred_devnum,
                                                   GError      **error);

GFile *qfu_device_selection_get_single_cdc_wdm      (QfuDeviceSelection   *self);
#if defined WITH_UDEV
void   qfu_device_selection_wait_for_cdc_wdm        (QfuDeviceSelection   *self,
                                                     GCancellable         *cancellable,
                                                     GAsyncReadyCallback   callback,
                                                     gpointer              user_data);
GFile *qfu_device_selection_wait_for_cdc_wdm_finish (QfuDeviceSelection   *self,
                                                     GAsyncResult         *res,
                                                     GError              **error);
#endif /* WITH_UDEV */

GFile *qfu_device_selection_get_single_tty      (QfuDeviceSelection   *self);
GList *qfu_device_selection_get_multiple_ttys   (QfuDeviceSelection   *self);
#if defined WITH_UDEV
void   qfu_device_selection_wait_for_tty        (QfuDeviceSelection   *self,
                                                 GCancellable         *cancellable,
                                                 GAsyncReadyCallback   callback,
                                                 gpointer              user_data);
GFile *qfu_device_selection_wait_for_tty_finish (QfuDeviceSelection   *self,
                                                 GAsyncResult         *res,
                                                 GError              **error);
#endif /* WITH_UDEV */

G_END_DECLS

#endif /* QFU_DEVICE_SELECTION_H */
