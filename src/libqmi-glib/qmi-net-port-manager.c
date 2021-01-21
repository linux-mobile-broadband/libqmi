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

#include "qmi-net-port-manager.h"

G_DEFINE_ABSTRACT_TYPE (QmiNetPortManager, qmi_net_port_manager, G_TYPE_OBJECT)

/*****************************************************************************/

void
qmi_net_port_manager_add_link (QmiNetPortManager    *self,
                               guint                 mux_id,
                               const gchar          *base_ifname,
                               const gchar          *ifname_prefix,
                               guint                 timeout,
                               GCancellable         *cancellable,
                               GAsyncReadyCallback   callback,
                               gpointer              user_data)
{
    QMI_NET_PORT_MANAGER_GET_CLASS (self)->add_link (self,
                                                     mux_id,
                                                     base_ifname,
                                                     ifname_prefix,
                                                     timeout,
                                                     cancellable,
                                                     callback,
                                                     user_data);
}

gchar *
qmi_net_port_manager_add_link_finish (QmiNetPortManager  *self,
                                      guint              *mux_id,
                                      GAsyncResult       *res,
                                      GError            **error)
{
    return QMI_NET_PORT_MANAGER_GET_CLASS (self)->add_link_finish (self, mux_id, res, error);
}

void
qmi_net_port_manager_del_link (QmiNetPortManager    *self,
                               const gchar          *ifname,
                               guint                 timeout,
                               GCancellable         *cancellable,
                               GAsyncReadyCallback   callback,
                               gpointer              user_data)
{
    QMI_NET_PORT_MANAGER_GET_CLASS (self)->del_link (self,
                                                     ifname,
                                                     timeout,
                                                     cancellable,
                                                     callback,
                                                     user_data);
}

gboolean
qmi_net_port_manager_del_link_finish (QmiNetPortManager  *self,
                                      GAsyncResult       *res,
                                      GError            **error)
{
    return QMI_NET_PORT_MANAGER_GET_CLASS (self)->del_link_finish (self, res, error);
}

/*****************************************************************************/

static void
qmi_net_port_manager_init (QmiNetPortManager *self)
{
}

static void
qmi_net_port_manager_class_init (QmiNetPortManagerClass *klass)
{
}
