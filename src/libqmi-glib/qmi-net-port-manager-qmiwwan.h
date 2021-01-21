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
 * Copyright (C) 2021 Aleksander Morgado <aleksander@aleksander.es>
 */

#ifndef _LIBQMI_GLIB_QMI_NET_PORT_MANAGER_QMIWWAN_H_
#define _LIBQMI_GLIB_QMI_NET_PORT_MANAGER_QMIWWAN_H_

#include <gio/gio.h>
#include <glib-object.h>

#include "qmi-net-port-manager.h"

#define QMI_TYPE_NET_PORT_MANAGER_QMIWWAN            (qmi_net_port_manager_qmiwwan_get_type ())
#define QMI_NET_PORT_MANAGER_QMIWWAN(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), QMI_TYPE_NET_PORT_MANAGER_QMIWWAN, QmiNetPortManagerQmiwwan))
#define QMI_NET_PORT_MANAGER_QMIWWAN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  QMI_TYPE_NET_PORT_MANAGER_QMIWWAN, QmiNetPortManagerQmiwwanClass))
#define QMI_IS_NET_PORT_MANAGER_QMIWWAN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), QMI_TYPE_NET_PORT_MANAGER_QMIWWAN))
#define QMI_IS_NET_PORT_MANAGER_QMIWWAN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  QMI_TYPE_NET_PORT_MANAGER_QMIWWAN))
#define QMI_NET_PORT_MANAGER_QMIWWAN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  QMI_TYPE_NET_PORT_MANAGER_QMIWWAN, QmiNetPortManagerQmiwwanClass))

typedef struct _QmiNetPortManagerQmiwwan        QmiNetPortManagerQmiwwan;
typedef struct _QmiNetPortManagerQmiwwanClass   QmiNetPortManagerQmiwwanClass;
typedef struct _QmiNetPortManagerQmiwwanPrivate QmiNetPortManagerQmiwwanPrivate;

struct _QmiNetPortManagerQmiwwan {
    QmiNetPortManager parent;
    QmiNetPortManagerQmiwwanPrivate *priv;
};

struct _QmiNetPortManagerQmiwwanClass {
    QmiNetPortManagerClass parent;
};

GType qmi_net_port_manager_qmiwwan_get_type (void);
G_DEFINE_AUTOPTR_CLEANUP_FUNC (QmiNetPortManagerQmiwwan, g_object_unref)

QmiNetPortManagerQmiwwan *qmi_net_port_manager_qmiwwan_new (const gchar  *iface,
                                                            GError      **error);

#endif /* _LIBQMI_GLIB_QMI_NET_PORT_MANAGER_QMIWWAN_H_ */
