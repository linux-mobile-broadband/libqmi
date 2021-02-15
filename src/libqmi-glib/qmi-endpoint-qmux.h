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

#ifndef _LIBQMI_GLIB_QMI_ENDPOINT_QMUX_H_
#define _LIBQMI_GLIB_QMI_ENDPOINT_QMUX_H_

#include "qmi-ctl.h"
#include "qmi-endpoint.h"
#include "qmi-file.h"

#define QMI_TYPE_ENDPOINT_QMUX            (qmi_endpoint_qmux_get_type ())
#define QMI_ENDPOINT_QMUX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), QMI_TYPE_ENDPOINT_QMUX, QmiEndpointQmux))
#define QMI_ENDPOINT_QMUX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  QMI_TYPE_ENDPOINT_QMUX, QmiEndpointQmuxClass))
#define QMI_IS_ENDPOINT_QMUX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), QMI_TYPE_ENDPOINT_QMUX))
#define QMI_IS_ENDPOINT_QMUX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  QMI_TYPE_ENDPOINT_QMUX))
#define QMI_ENDPOINT_QMUX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  QMI_TYPE_ENDPOINT_QMUX, QmiEndpointQmuxClass))

typedef struct _QmiEndpointQmux QmiEndpointQmux;
typedef struct _QmiEndpointQmuxClass QmiEndpointQmuxClass;
typedef struct _QmiEndpointQmuxPrivate QmiEndpointQmuxPrivate;

struct _QmiEndpointQmux {
    /*< private >*/
    QmiEndpoint parent;
    QmiEndpointQmuxPrivate *priv;
};

struct _QmiEndpointQmuxClass {
    /*< private >*/
    QmiEndpointClass parent;
};

GType qmi_endpoint_qmux_get_type (void);
G_DEFINE_AUTOPTR_CLEANUP_FUNC (QmiEndpointQmux, g_object_unref)

QmiEndpointQmux *qmi_endpoint_qmux_new (QmiFile      *file,
                                        const gchar  *proxy_path,
                                        QmiClientCtl *client_ctl);

#endif /* _LIBQMI_GLIB_QMI_ENDPOINT_QMUX_H_ */
