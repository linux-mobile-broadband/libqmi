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

#ifndef QFU_RESETER_H
#define QFU_RESETER_H

#include <glib-object.h>
#include <gio/gio.h>
#include <libqmi-glib.h>

#include "qfu-device-selection.h"

G_BEGIN_DECLS

#define QFU_TYPE_RESETER            (qfu_reseter_get_type ())
#define QFU_RESETER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), QFU_TYPE_RESETER, QfuReseter))
#define QFU_RESETER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  QFU_TYPE_RESETER, QfuReseterClass))
#define QFU_IS_RESETER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), QFU_TYPE_RESETER))
#define QFU_IS_RESETER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  QFU_TYPE_RESETER))
#define QFU_RESETER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  QFU_TYPE_RESETER, QfuReseterClass))

typedef struct _QfuReseter QfuReseter;
typedef struct _QfuReseterClass QfuReseterClass;
typedef struct _QfuReseterPrivate QfuReseterPrivate;

struct _QfuReseter {
    GObject             parent;
    QfuReseterPrivate *priv;
};

struct _QfuReseterClass {
    GObjectClass parent;
};

GType qfu_reseter_get_type (void);
G_DEFINE_AUTOPTR_CLEANUP_FUNC (QfuReseter, g_object_unref);

QfuReseter  *qfu_reseter_new        (QfuDeviceSelection   *device_selection,
                                     QmiClientDms         *qmi_client,
                                     QmiDeviceOpenFlags    device_open_flags);
void         qfu_reseter_run        (QfuReseter           *self,
                                     GCancellable         *cancellable,
                                     GAsyncReadyCallback   callback,
                                     gpointer              user_data);
gboolean     qfu_reseter_run_finish (QfuReseter           *self,
                                     GAsyncResult         *res,
                                     GError              **error);

G_END_DECLS

#endif /* QFU_RESETER_H */
