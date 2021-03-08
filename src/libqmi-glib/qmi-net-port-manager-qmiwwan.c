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

#include <limits.h>
#include <stdlib.h>

#include "qmi-net-port-manager-qmiwwan.h"
#include "qmi-enum-types.h"
#include "qmi-error-types.h"
#include "qmi-errors.h"
#include "qmi-helpers.h"


G_DEFINE_TYPE (QmiNetPortManagerQmiwwan, qmi_net_port_manager_qmiwwan, QMI_TYPE_NET_PORT_MANAGER)

struct _QmiNetPortManagerQmiwwanPrivate {
    gchar *iface;
    gchar *sysfs_path;
    GFile *sysfs_file;
    gchar *add_mux_sysfs_path;
    gchar *del_mux_sysfs_path;

    /* mux id tracking table */
    GHashTable *mux_id_map;
};

/*****************************************************************************/
/* The qmap/mux_id attribute was introduced in a newer kernel version. If
 * we don't have this info, try to keep the track of what iface applies to
 * what mux id manually here.
 *
 * Not perfect, but works if the MM doesn't crash and loses the info.
 * This legacy logic won't make any sense on plain qmicli operations, though.
 */

static gboolean
track_mux_id (QmiNetPortManagerQmiwwan  *self,
              const gchar               *link_iface,
              const gchar               *mux_id,
              GError                   **error)
{
    if (g_hash_table_lookup (self->priv->mux_id_map, link_iface)) {
        g_set_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_FAILED, "Already exists");
        return FALSE;
    }

    g_hash_table_insert (self->priv->mux_id_map,
                         g_strdup (link_iface),
                         g_strdup (mux_id));
    return TRUE;
}

static gboolean
untrack_mux_id (QmiNetPortManagerQmiwwan  *self,
                const gchar               *link_iface,
                GError                   **error)
{
    if (!g_hash_table_remove (self->priv->mux_id_map, link_iface)) {
        g_set_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_FAILED, "Not found");
        return FALSE;
    }
    return TRUE;
}

static const gchar *
get_tracked_mux_id (QmiNetPortManagerQmiwwan  *self,
                    const gchar               *link_iface,
                    GError                   **error)
{
    const gchar *found;

    found = g_hash_table_lookup (self->priv->mux_id_map, link_iface);
    if (!found) {
        g_set_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_FAILED, "Not found");
        return NULL;
    }
    return found;
}

/*****************************************************************************/

static gchar *
read_link_mux_id (const gchar  *link_iface,
                  GError      **error)
{
    g_autofree gchar *link_mux_id_sysfs_path = NULL;
    g_autofree gchar *link_mux_id = NULL;

    /* mux id expected as an hex integer between 0x01 and 0xfe */
    link_mux_id = g_malloc0 (5);
    link_mux_id_sysfs_path = g_strdup_printf ("/sys/class/net/%s/qmap/mux_id", link_iface);

    if (!qmi_helpers_read_sysfs_file (link_mux_id_sysfs_path, link_mux_id, 4, error))
        return NULL;

    return g_steal_pointer (&link_mux_id);
}

static gboolean
lookup_mux_id_in_links (GPtrArray    *links,
                        const gchar  *mux_id,
                        gchar       **out_link,
                        GError      **error)
{
    guint i;

    for (i = 0; links && i < links->len; i++) {
        const gchar      *link_iface;
        g_autofree gchar *link_mux_id = NULL;

        link_iface = g_ptr_array_index (links, i);
        link_mux_id = read_link_mux_id (link_iface, error);
        if (!link_mux_id)
            return FALSE;

        if (g_strcmp0 (mux_id, link_mux_id) == 0) {
            *out_link = g_strdup (link_iface);
            return TRUE;
        }
    }

    *out_link = NULL;
    return TRUE;
}

static gchar *
lookup_first_new_link (GPtrArray *links_before,
                       GPtrArray *links_after)
{
    guint i;

    if (!links_after)
        return NULL;

    for (i = 0; i < links_after->len; i++) {
        const gchar *link_iface;

        link_iface = g_ptr_array_index (links_after, i);

        if (!links_before || !g_ptr_array_find_with_equal_func (links_before, link_iface, g_str_equal, NULL))
            return g_strdup (link_iface);
    }
    return NULL;
}

/*****************************************************************************/

static gint
cmpuint (const guint *a,
         const guint *b)
{
    return ((*a > *b) ? 1 : ((*b > *a) ? -1 : 0));
}

static guint
get_first_free_mux_id (QmiNetPortManagerQmiwwan  *self,
                       GPtrArray                 *links,
                       GError                   **error)
{
    guint              i;
    g_autoptr(GArray)  existing_mux_ids = NULL;
    guint              next_mux_id;
    static const guint max_mux_id_upper_threshold = QMI_DEVICE_MUX_ID_MAX + 1;

    if (!links)
        return QMI_DEVICE_MUX_ID_MIN;

    existing_mux_ids = g_array_new (FALSE, FALSE, sizeof (guint));

    for (i = 0; i < links->len; i++) {
        const gchar      *link_iface;
        g_autofree gchar *link_mux_id = NULL;
        gulong            link_mux_id_num;

        link_iface = g_ptr_array_index (links, i);
        link_mux_id = read_link_mux_id (link_iface, NULL);
        if (!link_mux_id) {
            const gchar *tracked_link_mux_id;

            g_debug ("Couldn't read mux id from sysfs for link '%s': unsupported by driver", link_iface);
            /* fallback to use our internal tracking table... far from perfect */
            tracked_link_mux_id = get_tracked_mux_id (self, link_iface, NULL);
            if (!tracked_link_mux_id) {
                g_set_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_UNSUPPORTED,
                             "Couldn't get tracked mux id for link '%s'", link_iface);
                return QMI_DEVICE_MUX_ID_UNBOUND;
            }
            link_mux_id_num = strtoul (tracked_link_mux_id, NULL, 16);
        } else
            link_mux_id_num = strtoul (link_mux_id, NULL, 16);

        if (!link_mux_id_num) {
            g_set_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_FAILED,
                         "Couldn't parse mux id '%s'", link_mux_id);
            return QMI_DEVICE_MUX_ID_UNBOUND;
        }

        g_array_append_val (existing_mux_ids, link_mux_id_num);
    }

    /* add upper level threshold, so that if we end up out of the loop
     * below, it means we have exhausted all mux ids */
    g_array_append_val (existing_mux_ids, max_mux_id_upper_threshold);
    g_array_sort (existing_mux_ids, (GCompareFunc)cmpuint);

    for (next_mux_id = QMI_DEVICE_MUX_ID_MIN, i = 0; i < existing_mux_ids->len; next_mux_id++, i++) {
        guint existing;

        existing = g_array_index (existing_mux_ids, guint, i);
        if (next_mux_id < existing)
            return next_mux_id;
        g_assert_cmpuint (next_mux_id, ==, existing);
    }

    g_set_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_FAILED, "No mux ids left");
    return QMI_DEVICE_MUX_ID_UNBOUND;
}

/*****************************************************************************/

static gchar *
net_port_manager_add_link_finish (QmiNetPortManager  *self,
                                  guint              *mux_id,
                                  GAsyncResult       *res,
                                  GError            **error)
{
    gchar *link_name;

    link_name = g_task_propagate_pointer (G_TASK (res), error);
    if (!link_name)
        return NULL;

    if (mux_id)
        *mux_id = GPOINTER_TO_UINT (g_task_get_task_data (G_TASK (res)));

    return link_name;
}

static void
net_port_manager_add_link (QmiNetPortManager     *_self,
                           guint                  mux_id,
                           const gchar           *base_ifname,
                           const gchar           *ifname_prefix,
                           QmiDeviceAddLinkFlags  flags,
                           guint                  timeout,
                           GCancellable          *cancellable,
                           GAsyncReadyCallback    callback,
                           gpointer               user_data)
{
    QmiNetPortManagerQmiwwan *self = QMI_NET_PORT_MANAGER_QMIWWAN (_self);
    GTask                    *task;
    GError                   *error = NULL;
    g_autoptr(GPtrArray)      links_before = NULL;
    g_autoptr(GPtrArray)      links_after = NULL;
    g_autofree gchar         *link_name = NULL;
    g_autofree gchar         *mux_id_str = NULL;

    g_debug ("Net port manager based on qmi_wwan ignores the ifname prefix '%s'", ifname_prefix);
    g_debug ("Running add link operation...");

    task = g_task_new (self, cancellable, callback, user_data);

    if (flags != QMI_DEVICE_ADD_LINK_FLAGS_NONE) {
        g_autofree gchar *flags_str = NULL;

        flags_str = qmi_device_add_link_flags_build_string_from_mask (flags);
        g_task_return_new_error (task, QMI_CORE_ERROR, QMI_CORE_ERROR_UNSUPPORTED,
                                 "Adding link with flags '%s' is not supported", flags_str);
        g_object_unref (task);
        return;
    }

    if (!qmi_helpers_list_links (self->priv->sysfs_file,
                                 cancellable,
                                 NULL,
                                 &links_before,
                                 &error)) {
        g_prefix_error (&error, "Couldn't enumerate files in the sysfs directory before link addition: ");
        g_task_return_error (task, error);
        g_object_unref (task);
        return;
    }

    if (mux_id == QMI_DEVICE_MUX_ID_AUTOMATIC) {
        mux_id = get_first_free_mux_id (self, links_before, &error);
        if (mux_id == QMI_DEVICE_MUX_ID_UNBOUND) {
            g_prefix_error (&error, "Couldn't add link with automatic mux id: ");
            g_task_return_error (task, error);
            g_object_unref (task);
            return;
        }
        g_debug ("Using mux id %u", mux_id);
    }

    g_task_set_task_data (task, GUINT_TO_POINTER (mux_id), NULL);
    mux_id_str = g_strdup_printf ("0x%02x", mux_id);

    if (!qmi_helpers_write_sysfs_file (self->priv->add_mux_sysfs_path, mux_id_str, &error)) {
        g_prefix_error (&error, "Couldn't add create link with mux id %s: ", mux_id_str);
        g_task_return_error (task, error);
        g_object_unref (task);
        return;
    }

    if (!qmi_helpers_list_links (self->priv->sysfs_file,
                                 cancellable,
                                 links_before,
                                 &links_after,
                                 &error)) {
        g_prefix_error (&error, "Couldn't enumerate files in the sysfs directory after link addition: ");
        g_task_return_error (task, error);
        g_object_unref (task);
        return;
    }

    if (!lookup_mux_id_in_links (links_after, mux_id_str, &link_name, NULL)) {
        /* Now, assume this is because the mux_id attribute was added in a newer
         * kernel. As a fallback, we'll try to detect the first new link listed,
         * even if this is definitely very racy. */
        link_name = lookup_first_new_link (links_before, links_after);
        if (!link_name) {
            g_task_return_new_error (task, QMI_CORE_ERROR, QMI_CORE_ERROR_FAILED,
                                     "No new link detected for mux id %s", mux_id_str);
            g_object_unref (task);
            return;
        }
        g_debug ("Found first new link '%s' (unknown mux id)", link_name);
    } else
        g_debug ("Found link '%s' associated to mux id '%s'", link_name, mux_id_str);


    if (!track_mux_id (self, link_name, mux_id_str, &error)) {
        g_warning ("Couldn't track mux id: %s", error->message);
        g_clear_error (&error);
    }

    g_task_return_pointer (task, g_steal_pointer (&link_name), g_free);
    g_object_unref (task);
}

/*****************************************************************************/

static gboolean
net_port_manager_del_link_finish (QmiNetPortManager  *self,
                                  GAsyncResult       *res,
                                  GError            **error)
{
    return g_task_propagate_boolean (G_TASK (res), error);
}

static void
net_port_manager_del_link (QmiNetPortManager   *_self,
                           const gchar         *ifname,
                           guint                mux_id,
                           guint                timeout,
                           GCancellable        *cancellable,
                           GAsyncReadyCallback  callback,
                           gpointer             user_data)
{
    QmiNetPortManagerQmiwwan *self = QMI_NET_PORT_MANAGER_QMIWWAN (_self);
    GTask                    *task;
    GError                   *error = NULL;
    g_autoptr(GPtrArray)      links_before = NULL;
    g_autoptr(GPtrArray)      links_after = NULL;
    g_autofree gchar         *mux_id_str = NULL;

    g_debug ("Running del link (%s) operation...", ifname);

    task = g_task_new (self, cancellable, callback, user_data);

    if (!qmi_helpers_list_links (self->priv->sysfs_file,
                                 cancellable,
                                 NULL,
                                 &links_before,
                                 &error)) {
        g_prefix_error (&error, "Couldn't enumerate files in the sysfs directory before link deletion: ");
        g_task_return_error (task, error);
        g_object_unref (task);
        return;
    }

    if (!links_before || !g_ptr_array_find_with_equal_func (links_before, ifname, g_str_equal, NULL)) {
        g_task_return_new_error (task, QMI_CORE_ERROR, QMI_CORE_ERROR_INVALID_ARGS,
                                 "Cannot delete link '%s': interface not found",
                                 ifname);
        g_object_unref (task);
        return;
    }

    /* Try to guess mux id if not given as input */
    if (mux_id != QMI_DEVICE_MUX_ID_UNBOUND)
        mux_id_str = g_strdup_printf ("0x%02x", mux_id);
    else {
        mux_id_str = read_link_mux_id (ifname, NULL);
        if (!mux_id_str) {
            mux_id_str = g_strdup (get_tracked_mux_id (self, ifname, NULL));
            if (!mux_id_str) {
                /* This unsupported error allows us to flag when del_all_links()
                 * needs to switch to the fallback mechanism */
                g_task_return_new_error (task, QMI_CORE_ERROR, QMI_CORE_ERROR_UNSUPPORTED,
                                         "Cannot delete link '%s': unknown mux id",
                                         ifname);
                g_object_unref (task);
                return;
            }
        }
    }

    if (!qmi_helpers_write_sysfs_file (self->priv->del_mux_sysfs_path, mux_id_str, &error)) {
        g_prefix_error (&error, "Couldn't delete link with mux id %s: ", mux_id_str);
        g_task_return_error (task, error);
        g_object_unref (task);
        return;
    }

    if (!qmi_helpers_list_links (self->priv->sysfs_file,
                                 cancellable,
                                 links_before,
                                 &links_after,
                                 &error)) {
        g_prefix_error (&error, "Couldn't enumerate files in the sysfs directory after link deletion: ");
        g_task_return_error (task, error);
        g_object_unref (task);
        return;
    }

    if (links_after && g_ptr_array_find_with_equal_func (links_after, ifname, g_str_equal, NULL)) {
        g_task_return_new_error (task, QMI_CORE_ERROR, QMI_CORE_ERROR_FAILED,
                                 "link '%s' still detected", ifname);
        g_object_unref (task);
        return;
    }

    if (!untrack_mux_id (self, ifname, &error)) {
        g_debug ("couldn't untrack mux id: %s", error->message);
        g_clear_error (&error);
    }
    g_task_return_boolean (task, TRUE);
    g_object_unref (task);
}

/*****************************************************************************/

static gboolean
net_port_manager_del_all_links_finish (QmiNetPortManager  *self,
                                       GAsyncResult       *res,
                                       GError            **error)
{
    return g_task_propagate_boolean (G_TASK (res), error);
}

static void
fallback_del_all_links (GTask *task)
{
    QmiNetPortManagerQmiwwan *self;
    guint                     i;
    g_autoptr(GPtrArray)      links_before = NULL;
    g_autoptr(GPtrArray)      links_after = NULL;
    GError                   *error = NULL;
    guint                     n_deleted = 0;

    self = g_task_get_source_object (task);

    g_debug ("Running fallback link deletion logic...");

    if (!qmi_helpers_list_links (self->priv->sysfs_file,
                                 g_task_get_cancellable (task),
                                 NULL,
                                 &links_before,
                                 &error)) {
        g_prefix_error (&error, "Couldn't list links before deleting all: ");
        g_task_return_error (task, error);
        g_object_unref (task);
        return;
    }

    if (!links_before) {
        g_task_return_boolean (task, TRUE);
        g_object_unref (task);
        return;
    }

    for (i = QMI_DEVICE_MUX_ID_MIN; i <= QMI_DEVICE_MUX_ID_MAX; i++) {
        g_autofree gchar *mux_id = NULL;

        mux_id = g_strdup_printf ("0x%02x", i);

        /* attempt to delete link with the given mux id; if there is no such link
         * the kernel will complain with a harmless "mux_id not present" warning */
        if ((qmi_helpers_write_sysfs_file (self->priv->del_mux_sysfs_path, mux_id, NULL) &&
             (++n_deleted == links_before->len))) {
            /* early break if all N links deleted already */
            break;
        }
    }

    if (!qmi_helpers_list_links (self->priv->sysfs_file,
                                 g_task_get_cancellable (task),
                                 NULL,
                                 &links_after,
                                 &error)) {
        g_prefix_error (&error, "Couldn't list links after deleting all: ");
        g_task_return_error (task, error);
    } else if (links_after)
        g_task_return_new_error (task, QMI_CORE_ERROR, QMI_CORE_ERROR_FAILED,
                                 "Not all links were deleted");
    else
        g_task_return_boolean (task, TRUE);
    g_object_unref (task);
}

static void
parent_del_all_links_ready (QmiNetPortManager *self,
                            GAsyncResult      *res,
                            GTask             *task)
{
    g_autoptr(GError) error = NULL;

    if (!QMI_NET_PORT_MANAGER_CLASS (qmi_net_port_manager_qmiwwan_parent_class)->del_all_links_finish (self, res, &error)) {
        if (g_error_matches (error, QMI_CORE_ERROR, QMI_CORE_ERROR_UNSUPPORTED)) {
            fallback_del_all_links (task);
            return;
        }
        g_task_return_error (task, g_steal_pointer (&error));
    } else
        g_task_return_boolean (task, TRUE);
    g_object_unref (task);
}

static void
net_port_manager_del_all_links (QmiNetPortManager    *_self,
                                const gchar          *base_ifname,
                                GCancellable         *cancellable,
                                GAsyncReadyCallback   callback,
                                gpointer              user_data)
{
    QmiNetPortManagerQmiwwan *self = QMI_NET_PORT_MANAGER_QMIWWAN (_self);
    GTask                    *task;

    task = g_task_new (self, cancellable, callback, user_data);

    /* validate base ifname before doing anything else */
    if (base_ifname && !g_str_equal (base_ifname, self->priv->iface)) {
        g_task_return_new_error (task, QMI_CORE_ERROR, QMI_CORE_ERROR_INVALID_ARGS,
                                 "Invalid base interface given: '%s' (must be '%s')",
                                 base_ifname, self->priv->iface);
        g_object_unref (task);
        return;
    }

    QMI_NET_PORT_MANAGER_CLASS (qmi_net_port_manager_qmiwwan_parent_class)->del_all_links (_self,
                                                                                           base_ifname,
                                                                                           cancellable,
                                                                                           (GAsyncReadyCallback)parent_del_all_links_ready,
                                                                                           task);
}

/*****************************************************************************/

QmiNetPortManagerQmiwwan *
qmi_net_port_manager_qmiwwan_new (const gchar  *iface,
                                  GError      **error)
{
    g_autoptr(QmiNetPortManagerQmiwwan) self = NULL;

    self = QMI_NET_PORT_MANAGER_QMIWWAN (g_object_new (QMI_TYPE_NET_PORT_MANAGER_QMIWWAN, NULL));

    self->priv->iface = g_strdup (iface);

    self->priv->sysfs_path = g_strdup_printf ("/sys/class/net/%s", iface);
    self->priv->sysfs_file = g_file_new_for_path (self->priv->sysfs_path);

    self->priv->add_mux_sysfs_path = g_strdup_printf ("%s/qmi/add_mux", self->priv->sysfs_path);
    self->priv->del_mux_sysfs_path = g_strdup_printf ("%s/qmi/del_mux", self->priv->sysfs_path);

    if (!g_file_test (self->priv->add_mux_sysfs_path, G_FILE_TEST_EXISTS) ||
        !g_file_test (self->priv->del_mux_sysfs_path, G_FILE_TEST_EXISTS)) {
        g_set_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_FAILED,
                     "No support for multiplexing in the interface");
        g_object_unref (self);
        return NULL;
    }

    return g_steal_pointer (&self);
}

/*****************************************************************************/

static void
qmi_net_port_manager_qmiwwan_init (QmiNetPortManagerQmiwwan *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
                                              QMI_TYPE_NET_PORT_MANAGER_QMIWWAN,
                                              QmiNetPortManagerQmiwwanPrivate);

    self->priv->mux_id_map = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
}

static void
finalize (GObject *object)
{
    QmiNetPortManagerQmiwwan *self = QMI_NET_PORT_MANAGER_QMIWWAN (object);

    g_hash_table_unref (self->priv->mux_id_map);
    g_free (self->priv->iface);
    g_object_unref (self->priv->sysfs_file);
    g_free (self->priv->sysfs_path);
    g_free (self->priv->del_mux_sysfs_path);
    g_free (self->priv->add_mux_sysfs_path);

    G_OBJECT_CLASS (qmi_net_port_manager_qmiwwan_parent_class)->finalize (object);
}

static void
qmi_net_port_manager_qmiwwan_class_init (QmiNetPortManagerQmiwwanClass *klass)
{
    GObjectClass           *object_class = G_OBJECT_CLASS (klass);
    QmiNetPortManagerClass *net_port_manager_class = QMI_NET_PORT_MANAGER_CLASS (klass);

    g_type_class_add_private (object_class, sizeof (QmiNetPortManagerQmiwwanPrivate));

    object_class->finalize = finalize;

    net_port_manager_class->add_link = net_port_manager_add_link;
    net_port_manager_class->add_link_finish = net_port_manager_add_link_finish;
    net_port_manager_class->del_link = net_port_manager_del_link;
    net_port_manager_class->del_link_finish = net_port_manager_del_link_finish;
    net_port_manager_class->del_all_links = net_port_manager_del_all_links;
    net_port_manager_class->del_all_links_finish = net_port_manager_del_all_links_finish;
}
