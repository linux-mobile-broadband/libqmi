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
 * Copyright (C) 2019-2020 Eric Caruso <ejcaruso@chromium.org>
 * Copyright (C) 2020 Aleksander Morgado <aleksander@aleksander.es>
 */

#ifndef _LIBQMI_GLIB_QMI_ENDPOINT_QRTR_H_
#define _LIBQMI_GLIB_QMI_ENDPOINT_QRTR_H_

#include <libqrtr-glib.h>

#include "qmi-endpoint.h"

#define QMI_TYPE_ENDPOINT_QRTR            (qmi_endpoint_qrtr_get_type ())
#define QMI_ENDPOINT_QRTR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), QMI_TYPE_ENDPOINT_QRTR, QmiEndpointQrtr))
#define QMI_ENDPOINT_QRTR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  QMI_TYPE_ENDPOINT_QRTR, QmiEndpointQrtrClass))
#define QMI_IS_ENDPOINT_QRTR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), QMI_TYPE_ENDPOINT_QRTR))
#define QMI_IS_ENDPOINT_QRTR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  QMI_TYPE_ENDPOINT_QRTR))
#define QMI_ENDPOINT_QRTR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  QMI_TYPE_ENDPOINT_QRTR, QmiEndpointQrtrClass))

typedef struct _QmiEndpointQrtr QmiEndpointQrtr;
typedef struct _QmiEndpointQrtrClass QmiEndpointQrtrClass;
typedef struct _QmiEndpointQrtrPrivate QmiEndpointQrtrPrivate;

struct _QmiEndpointQrtr {
    /*< private >*/
    QmiEndpoint parent;
    QmiEndpointQrtrPrivate *priv;
};

struct _QmiEndpointQrtrClass {
    /*< private >*/
    QmiEndpointClass parent;
};

GType qmi_endpoint_qrtr_get_type (void);

QmiEndpointQrtr *qmi_endpoint_qrtr_new (QrtrNode *node);

#endif /* _LIBQMI_GLIB_QMI_ENDPOINT_QRTR_H_ */
