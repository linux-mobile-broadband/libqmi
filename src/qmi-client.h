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

#ifndef _LIBQMI_GLIB_QMI_CLIENT_H_
#define _LIBQMI_GLIB_QMI_CLIENT_H_

#include <glib-object.h>

#include "qmi-enums.h"
#include "qmi-device.h"

G_BEGIN_DECLS

#define QMI_CID_NONE      0x00
#define QMI_CID_BROADCAST 0xFF

#define QMI_TYPE_CLIENT            (qmi_client_get_type ())
#define QMI_CLIENT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), QMI_TYPE_CLIENT, QmiClient))
#define QMI_CLIENT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  QMI_TYPE_CLIENT, QmiClientClass))
#define QMI_IS_CLIENT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), QMI_TYPE_CLIENT))
#define QMI_IS_CLIENT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  QMI_TYPE_CLIENT))
#define QMI_CLIENT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  QMI_TYPE_CLIENT, QmiClientClass))

typedef struct _QmiClient QmiClient;
typedef struct _QmiClientClass QmiClientClass;
typedef struct _QmiClientPrivate QmiClientPrivate;

#define QMI_CLIENT_DEVICE   "client-device"
#define QMI_CLIENT_SERVICE  "client-service"
#define QMI_CLIENT_CID      "client-cid"

struct _QmiClient {
    GObject parent;
    QmiClientPrivate *priv;
};

struct _QmiClientClass {
    GObjectClass parent;

    /* Virtual method to get indications processed */
    void (* process_indication) (QmiClient *self,
                                 QmiMessage *message);
};

GType qmi_client_get_type (void);

QmiDevice  *qmi_client_get_device  (QmiClient *self);
QmiDevice  *qmi_client_peek_device (QmiClient *self);
QmiService  qmi_client_get_service (QmiClient *self);
guint8      qmi_client_get_cid     (QmiClient *self);

guint16     qmi_client_get_next_transaction_id (QmiClient *self);

/* not part of the public API */
void qmi_client_process_indication (QmiClient *self,
                                    QmiMessage *message);

G_END_DECLS

#endif /* _LIBQMI_GLIB_QMI_CLIENT_H_ */
