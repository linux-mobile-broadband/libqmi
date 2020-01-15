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

#ifndef _LIBQMI_GLIB_QMI_ENDPOINT_MBIM_H_
#define _LIBQMI_GLIB_QMI_ENDPOINT_MBIM_H_

#include "qmi-endpoint.h"
#include "qmi-file.h"

#define QMI_TYPE_ENDPOINT_MBIM            (qmi_endpoint_mbim_get_type ())
#define QMI_ENDPOINT_MBIM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), QMI_TYPE_ENDPOINT_MBIM, QmiEndpointMbim))
#define QMI_ENDPOINT_MBIM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  QMI_TYPE_ENDPOINT_MBIM, QmiEndpointMbimClass))
#define QMI_IS_ENDPOINT_MBIM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), QMI_TYPE_ENDPOINT_MBIM))
#define QMI_IS_ENDPOINT_MBIM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  QMI_TYPE_ENDPOINT_MBIM))
#define QMI_ENDPOINT_MBIM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  QMI_TYPE_ENDPOINT_MBIM, QmiEndpointMbimClass))

typedef struct _QmiEndpointMbim QmiEndpointMbim;
typedef struct _QmiEndpointMbimClass QmiEndpointMbimClass;
typedef struct _QmiEndpointMbimPrivate QmiEndpointMbimPrivate;

struct _QmiEndpointMbim {
    /*< private >*/
    QmiEndpoint parent;
    QmiEndpointMbimPrivate *priv;
};

struct _QmiEndpointMbimClass {
    /*< private >*/
    QmiEndpointClass parent;
};

GType qmi_endpoint_mbim_get_type (void);
G_DEFINE_AUTOPTR_CLEANUP_FUNC (QmiEndpointMbim, g_object_unref)

QmiEndpointMbim *qmi_endpoint_mbim_new (QmiFile *file);

#endif /* _LIBQMI_GLIB_QMI_ENDPOINT_MBIM_H_ */
