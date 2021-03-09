/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * libmbim-glib -- GLib/GIO based library to control MBIM devices
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
 *
 * Based on the QmiNetPortManagerRmnet from libqmi:
 *   Copyright (C) 2020-2021 Eric Caruso <ejcaruso@chromium.org>
 *   Copyright (C) 2020-2021 Andrew Lassalle <andrewlassalle@chromium.org>
 */

#ifndef _LIBMBIM_GLIB_MBIM_NET_PORT_MANAGER_H_
#define _LIBMBIM_GLIB_MBIM_NET_PORT_MANAGER_H_

#include <gio/gio.h>
#include <glib-object.h>

#define MBIM_TYPE_NET_PORT_MANAGER            (mbim_net_port_manager_get_type ())
#define MBIM_NET_PORT_MANAGER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MBIM_TYPE_NET_PORT_MANAGER, MbimNetPortManager))
#define MBIM_NET_PORT_MANAGER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  MBIM_TYPE_NET_PORT_MANAGER, MbimNetPortManagerClass))
#define MBIM_IS_NET_PORT_MANAGER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MBIM_TYPE_NET_PORT_MANAGER))
#define MBIM_IS_NET_PORT_MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  MBIM_TYPE_NET_PORT_MANAGER))
#define MBIM_NET_PORT_MANAGER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  MBIM_TYPE_NET_PORT_MANAGER, MbimNetPortManagerClass))

typedef struct _MbimNetPortManager        MbimNetPortManager;
typedef struct _MbimNetPortManagerClass   MbimNetPortManagerClass;
typedef struct _MbimNetPortManagerPrivate MbimNetPortManagerPrivate;

struct _MbimNetPortManager {
    GObject parent;
    MbimNetPortManagerPrivate *priv;
};

struct _MbimNetPortManagerClass {
    GObjectClass parent;
};

GType mbim_net_port_manager_get_type (void);
G_DEFINE_AUTOPTR_CLEANUP_FUNC (MbimNetPortManager, g_object_unref)

MbimNetPortManager *mbim_net_port_manager_new (const gchar  *iface,
                                               GError      **error);

gboolean  mbim_net_port_manager_list_links     (MbimNetPortManager   *self,
                                                const gchar          *base_ifname,
                                                GPtrArray           **out_links,
                                                GError              **error);

void      mbim_net_port_manager_add_link        (MbimNetPortManager   *self,
                                                 guint                 session_id,
                                                 const gchar          *base_ifname,
                                                 const gchar          *ifname_prefix,
                                                 guint                 timeout,
                                                 GCancellable         *cancellable,
                                                 GAsyncReadyCallback   callback,
                                                 gpointer              user_data);
gchar    *mbim_net_port_manager_add_link_finish (MbimNetPortManager   *self,
                                                 guint                *session_id,
                                                 GAsyncResult         *res,
                                                 GError              **error);

void      mbim_net_port_manager_del_link        (MbimNetPortManager   *self,
                                                 const gchar          *ifname,
                                                 guint                 timeout,
                                                 GCancellable         *cancellable,
                                                 GAsyncReadyCallback   callback,
                                                 gpointer              user_data);
gboolean  mbim_net_port_manager_del_link_finish (MbimNetPortManager   *self,
                                                 GAsyncResult         *res,
                                                 GError              **error);

void      mbim_net_port_manager_del_all_links        (MbimNetPortManager   *self,
                                                      const gchar          *base_ifname,
                                                      GCancellable         *cancellable,
                                                      GAsyncReadyCallback   callback,
                                                      gpointer              user_data);
gboolean  mbim_net_port_manager_del_all_links_finish (MbimNetPortManager   *self,
                                                      GAsyncResult         *res,
                                                      GError              **error);

#endif /* _LIBMBIM_GLIB_MBIM_NET_PORT_MANAGER_H_ */
