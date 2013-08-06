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
 * Copyright (C) 2013 Aleksander Morgado <aleksander@lanedo.com>
 */

#ifndef QMI_PROXY_H
#define QMI_PROXY_H

#include <glib-object.h>
#include <gio/gio.h>

#define QMI_TYPE_PROXY            (qmi_proxy_get_type ())
#define QMI_PROXY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), QMI_TYPE_PROXY, QmiProxy))
#define QMI_PROXY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), QMI_TYPE_PROXY, QmiProxyClass))
#define QMI_IS_PROXY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), QMI_TYPE_PROXY))
#define QMI_IS_PROXY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), QMI_TYPE_PROXY))
#define QMI_PROXY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), QMI_TYPE_PROXY, QmiProxyClass))

typedef struct _QmiProxy QmiProxy;
typedef struct _QmiProxyClass QmiProxyClass;
typedef struct _QmiProxyPrivate QmiProxyPrivate;

#define QMI_PROXY_SOCKET_PATH "qmi-proxy"

#define QMI_PROXY_N_CLIENTS   "qmi-proxy-n-clients"

struct _QmiProxy {
    GObject parent;
    QmiProxyPrivate *priv;
};

struct _QmiProxyClass {
    GObjectClass parent;
};

GType qmi_proxy_get_type (void);

QmiProxy *qmi_proxy_new           (GError **error);
guint     qmi_proxy_get_n_clients (QmiProxy *self);

#endif /* QMI_PROXY_H */
