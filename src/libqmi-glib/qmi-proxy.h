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
 * Copyright (C) 2013-2017 Aleksander Morgado <aleksander@aleksander.es>
 */

#ifndef QMI_PROXY_H
#define QMI_PROXY_H

/**
 * SECTION:qmi-proxy
 * @title: QmiProxy
 * @short_description: QMI proxy handling routines
 *
 * The #QmiProxy will setup an abstract socket listening on a predefined
 * address, and will take care of synchronizing the access to a set of shared
 * QMI ports.
 *
 * Multiple #QmiDevices may be connected to the #QmiProxy at any given time. The
 * #QmiProxy acts as a stateless proxy for non-CTL services (messages are
 * transferred unmodified), and as a stateful proxy for the CTL service (all
 * remote #QmiDevices will need to share the same CTL message sequence ID).
 */

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

/**
 * QMI_PROXY_SOCKET_PATH:
 *
 * Symbol defining the default abstract socket name where the #QmiProxy will listen.
 *
 * Since: 1.8
 */
#define QMI_PROXY_SOCKET_PATH "qmi-proxy"

/**
 * QMI_PROXY_N_CLIENTS:
 *
 * Symbol defining the #QmiProxy:qmi-proxy-n-clients property.
 *
 * Since: 1.8
 */
#define QMI_PROXY_N_CLIENTS   "qmi-proxy-n-clients"

/**
 * QmiProxy:
 *
 * The #QmiProxy structure contains private data and should only be accessed
 * using the provided API.
 *
 * Since: 1.8
 */
struct _QmiProxy {
    GObject parent;
    QmiProxyPrivate *priv;
};

struct _QmiProxyClass {
    GObjectClass parent;
};

GType qmi_proxy_get_type (void);

/**
 * qmi_proxy_new:
 * @error: Return location for error or %NULL.
 *
 * Creates a #QmiProxy listening in the default proxy addess.
 *
 * Returns: A newly created #QmiProxy, or #NULL if @error is set.
 *
 * Since: 1.8
 */
QmiProxy *qmi_proxy_new (GError **error);

/**
 * qmi_proxy_get_n_clients:
 * @self: a #QmiProxy.
 *
 * Get the number of clients currently connected to the proxy.
 *
 * Returns: a #guint.
 *
 * Since: 1.8
 */
guint qmi_proxy_get_n_clients (QmiProxy *self);

#endif /* QMI_PROXY_H */
