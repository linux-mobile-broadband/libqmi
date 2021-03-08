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
#include "qmi-device.h"
#include "qmi-helpers.h"

G_DEFINE_ABSTRACT_TYPE (QmiNetPortManager, qmi_net_port_manager, G_TYPE_OBJECT)

/*****************************************************************************/

void
qmi_net_port_manager_add_link (QmiNetPortManager     *self,
                               guint                  mux_id,
                               const gchar           *base_ifname,
                               const gchar           *ifname_prefix,
                               QmiDeviceAddLinkFlags  flags,
                               guint                  timeout,
                               GCancellable          *cancellable,
                               GAsyncReadyCallback    callback,
                               gpointer               user_data)
{
    QMI_NET_PORT_MANAGER_GET_CLASS (self)->add_link (self,
                                                     mux_id,
                                                     base_ifname,
                                                     ifname_prefix,
                                                     flags,
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
                               guint                 mux_id,
                               guint                 timeout,
                               GCancellable         *cancellable,
                               GAsyncReadyCallback   callback,
                               gpointer              user_data)
{
    QMI_NET_PORT_MANAGER_GET_CLASS (self)->del_link (self,
                                                     ifname,
                                                     mux_id,
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

void
qmi_net_port_manager_del_all_links (QmiNetPortManager    *self,
                                    const gchar          *base_ifname,
                                    GCancellable         *cancellable,
                                    GAsyncReadyCallback   callback,
                                    gpointer              user_data)
{
    QMI_NET_PORT_MANAGER_GET_CLASS (self)->del_all_links (self,
                                                          base_ifname,
                                                          cancellable,
                                                          callback,
                                                          user_data);
}

gboolean
qmi_net_port_manager_del_all_links_finish (QmiNetPortManager    *self,
                                           GAsyncResult         *res,
                                           GError              **error)
{
    return QMI_NET_PORT_MANAGER_GET_CLASS (self)->del_all_links_finish (self, res, error);
}

gboolean
qmi_net_port_manager_list_links (QmiNetPortManager  *self,
                                 const gchar        *base_ifname,
                                 GPtrArray         **out_links,
                                 GError            **error)
{
    return QMI_NET_PORT_MANAGER_GET_CLASS (self)->list_links (self, base_ifname, out_links, error);
}

/*****************************************************************************/
/* Default implementations */

static gboolean
net_port_manager_list_links (QmiNetPortManager  *self,
                             const gchar        *base_ifname,
                             GPtrArray         **out_links,
                             GError            **error)
{
    g_autoptr(GFile)  sysfs_file = NULL;
    g_autofree gchar *sysfs_path = NULL;

    sysfs_path = g_strdup_printf ("/sys/class/net/%s", base_ifname);
    sysfs_file = g_file_new_for_path (sysfs_path);

    return qmi_helpers_list_links (sysfs_file, NULL, NULL, out_links, error);
}

typedef struct {
    GPtrArray *links;
    guint      link_i;
} DelAllLinksContext;

static void
del_all_links_context_free (DelAllLinksContext *ctx)
{
    g_clear_pointer (&ctx->links, g_ptr_array_unref);
    g_slice_free (DelAllLinksContext, ctx);
}

static gboolean
net_port_manager_del_all_links_finish (QmiNetPortManager  *self,
                                       GAsyncResult       *res,
                                       GError            **error)
{
    return g_task_propagate_boolean (G_TASK (res), error);
}

static void delete_next_link (GTask *task);

static void
port_manager_del_link_ready (QmiNetPortManager *self,
                             GAsyncResult      *res,
                             GTask             *task)
{
    DelAllLinksContext *ctx;
    GError             *error = NULL;

    ctx = g_task_get_task_data (task);

    if (!qmi_net_port_manager_del_link_finish (self, res, &error)) {
        g_task_return_error (task, error);
        g_object_unref (task);
        return;
    }

    g_ptr_array_remove_index_fast (ctx->links, 0);
    delete_next_link (task);
}

static void
delete_next_link (GTask *task)
{
    QmiNetPortManager  *self;
    DelAllLinksContext *ctx;

    self = g_task_get_source_object (task);
    ctx = g_task_get_task_data (task);

    if (!ctx->links || ctx->links->len == 0) {
        g_task_return_boolean (task, TRUE);
        g_object_unref (task);
        return;
    }

    qmi_net_port_manager_del_link (self,
                                   g_ptr_array_index (ctx->links, 0),
                                   QMI_DEVICE_MUX_ID_UNBOUND,
                                   5,
                                   g_task_get_cancellable (task),
                                   (GAsyncReadyCallback)port_manager_del_link_ready,
                                   task);
}

static void
net_port_manager_del_all_links (QmiNetPortManager    *self,
                                const gchar          *base_ifname,
                                GCancellable         *cancellable,
                                GAsyncReadyCallback   callback,
                                gpointer              user_data)
{
    GTask              *task;
    DelAllLinksContext *ctx;
    GError             *error = NULL;

    task = g_task_new (self, cancellable, callback, user_data);
    ctx = g_slice_new0 (DelAllLinksContext);
    g_task_set_task_data (task, ctx, (GDestroyNotify)del_all_links_context_free);

    if (!qmi_net_port_manager_list_links (self, base_ifname, &ctx->links, &error)) {
        g_task_return_error (task, error);
        g_object_unref (task);
        return;
    }

    delete_next_link (task);
}

/*****************************************************************************/

static void
qmi_net_port_manager_init (QmiNetPortManager *self)
{
}

static void
qmi_net_port_manager_class_init (QmiNetPortManagerClass *klass)
{
    klass->list_links = net_port_manager_list_links;
    klass->del_all_links = net_port_manager_del_all_links;
    klass->del_all_links_finish = net_port_manager_del_all_links_finish;
}
