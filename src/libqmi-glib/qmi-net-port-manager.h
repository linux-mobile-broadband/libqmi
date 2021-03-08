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
 * Copyright (C) 2020-2021 Eric Caruso <ejcaruso@chromium.org>
 * Copyright (C) 2020-2021 Andrew Lassalle <andrewlassalle@chromium.org>
 * Copyright (C) 2021 Aleksander Morgado <aleksander@aleksander.es>
 */

#ifndef _LIBQMI_GLIB_QMI_NET_PORT_MANAGER_H_
#define _LIBQMI_GLIB_QMI_NET_PORT_MANAGER_H_

#include <gio/gio.h>
#include <glib-object.h>

#include "qmi-device.h"

#define QMI_TYPE_NET_PORT_MANAGER            (qmi_net_port_manager_get_type ())
#define QMI_NET_PORT_MANAGER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), QMI_TYPE_NET_PORT_MANAGER, QmiNetPortManager))
#define QMI_NET_PORT_MANAGER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  QMI_TYPE_NET_PORT_MANAGER, QmiNetPortManagerClass))
#define QMI_IS_NET_PORT_MANAGER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), QMI_TYPE_NET_PORT_MANAGER))
#define QMI_IS_NET_PORT_MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  QMI_TYPE_NET_PORT_MANAGER))
#define QMI_NET_PORT_MANAGER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  QMI_TYPE_NET_PORT_MANAGER, QmiNetPortManagerClass))

typedef struct _QmiNetPortManager      QmiNetPortManager;
typedef struct _QmiNetPortManagerClass QmiNetPortManagerClass;

struct _QmiNetPortManager {
    GObject parent;
};

struct _QmiNetPortManagerClass {
    GObjectClass parent;

    gboolean (* list_links)      (QmiNetPortManager    *self,
                                  const gchar          *base_ifname,
                                  GPtrArray           **out_links,
                                  GError              **error);

    void     (* add_link)        (QmiNetPortManager      *self,
                                  guint                   mux_id,
                                  const gchar            *base_ifname,
                                  const gchar            *ifname_prefix,
                                  QmiDeviceAddLinkFlags   flags,
                                  guint                   timeout,
                                  GCancellable           *cancellable,
                                  GAsyncReadyCallback     callback,
                                  gpointer                user_data);
    gchar *  (* add_link_finish) (QmiNetPortManager      *self,
                                  guint                  *mux_id,
                                  GAsyncResult           *res,
                                  GError                **error);

    void     (* del_link)        (QmiNetPortManager    *self,
                                  const gchar          *ifname,
                                  guint                 mux_id,
                                  guint                 timeout,
                                  GCancellable         *cancellable,
                                  GAsyncReadyCallback   callback,
                                  gpointer              user_data);
    gboolean (* del_link_finish) (QmiNetPortManager    *self,
                                  GAsyncResult         *res,
                                  GError              **error);

    void     (* del_all_links)        (QmiNetPortManager    *self,
                                       const gchar          *base_ifname,
                                       GCancellable         *cancellable,
                                       GAsyncReadyCallback   callback,
                                       gpointer              user_data);
    gboolean (* del_all_links_finish) (QmiNetPortManager    *self,
                                       GAsyncResult         *res,
                                       GError              **error);
};

GType qmi_net_port_manager_get_type (void);
G_DEFINE_AUTOPTR_CLEANUP_FUNC (QmiNetPortManager, g_object_unref)

gboolean  qmi_net_port_manager_list_links      (QmiNetPortManager    *self,
                                                const gchar          *base_ifname,
                                                GPtrArray           **out_links,
                                                GError              **error);

void      qmi_net_port_manager_add_link        (QmiNetPortManager      *self,
                                                guint                   mux_id,
                                                const gchar            *base_ifname,
                                                const gchar            *ifname_prefix,
                                                QmiDeviceAddLinkFlags   flags,
                                                guint                   timeout,
                                                GCancellable           *cancellable,
                                                GAsyncReadyCallback     callback,
                                                gpointer                user_data);
gchar    *qmi_net_port_manager_add_link_finish (QmiNetPortManager      *self,
                                                guint                  *mux_id,
                                                GAsyncResult           *res,
                                                GError                **error);

void      qmi_net_port_manager_del_link        (QmiNetPortManager    *self,
                                                const gchar          *ifname,
                                                guint                 mux_id,
                                                guint                 timeout,
                                                GCancellable         *cancellable,
                                                GAsyncReadyCallback   callback,
                                                gpointer              user_data);
gboolean  qmi_net_port_manager_del_link_finish (QmiNetPortManager    *self,
                                                GAsyncResult         *res,
                                                GError              **error);

void     qmi_net_port_manager_del_all_links        (QmiNetPortManager    *self,
                                                    const gchar          *base_ifname,
                                                    GCancellable         *cancellable,
                                                    GAsyncReadyCallback   callback,
                                                    gpointer              user_data);
gboolean qmi_net_port_manager_del_all_links_finish (QmiNetPortManager    *self,
                                                    GAsyncResult         *res,
                                                    GError              **error);

#endif /* _LIBQMI_GLIB_QMI_NET_PORT_MANAGER_H_ */
