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
 * Copyright (C) 2013-2017 <Aleksander Morgado <aleksander@aleksander.es>
 */

#include <string.h>
#include <ctype.h>
#include <sys/file.h>
#include <sys/types.h>
#include <errno.h>

#include <glib.h>
#include <glib/gstdio.h>
#include <gio/gunixsocketaddress.h>

#include "config.h"
#include "qmi-enum-types.h"
#include "qmi-error-types.h"
#include "qmi-device.h"
#include "qmi-ctl.h"
#include "qmi-helpers.h"
#include "qmi-proxy.h"
#include "qmi-version.h"

#if QMI_QRTR_SUPPORTED
# include "libqrtr-glib.h"
#endif

#define BUFFER_SIZE 512

#define QMI_MESSAGE_OUTPUT_TLV_RESULT 0x02
#define QMI_MESSAGE_OUTPUT_TLV_ALLOCATION_INFO 0x01
#define QMI_MESSAGE_CTL_ALLOCATE_CID 0x0022

#define QMI_MESSAGE_INPUT_TLV_RELEASE_INFO 0x01
#define QMI_MESSAGE_CTL_RELEASE_CID 0x0023

#define QMI_MESSAGE_CTL_INTERNAL_PROXY_OPEN 0xFF00
#define QMI_MESSAGE_CTL_INTERNAL_PROXY_OPEN_INPUT_TLV_DEVICE_PATH 0x01

G_DEFINE_TYPE (QmiProxy, qmi_proxy, G_TYPE_OBJECT)

enum {
    PROP_0,
    PROP_N_CLIENTS,
    PROP_LAST
};

static GParamSpec *properties[PROP_LAST];

struct _QmiProxyPrivate {
    /* Unix socket service */
    GSocketService *socket_service;

    /* Client applications */
    GList *clients;

    /* Devices */
    GList *devices;

    /* Array of QMI client infos that are not owned by any client
     * application (e.g. they were allocated by a client application but
     * then not explicitly released). */
    GArray *disowned_qmi_client_info_array;

#if QMI_QRTR_SUPPORTED
    QrtrBus *qrtr_bus;
#endif
};

/*****************************************************************************/

guint
qmi_proxy_get_n_clients (QmiProxy *self)
{
    g_return_val_if_fail (QMI_IS_PROXY (self), 0);

    return g_list_length (self->priv->clients);
}

/*****************************************************************************/

typedef struct {
    QmiService service;
    guint8     cid;
} QmiClientInfo;

typedef struct {
    volatile gint ref_count;

    QmiProxy *proxy; /* not full ref */

    /* client socket connection context */
    GSocketConnection *connection;
    GSource           *connection_readable_source;
    GByteArray        *buffer;

    /* QMI device associated to connection */
    QmiDevice  *device;
    QmiMessage *internal_proxy_open_request;
    GArray     *qmi_client_info_array;
    guint       indication_id;
    guint       device_removed_id;
#if QMI_QRTR_SUPPORTED
    guint node_id;
#endif
} Client;

static gboolean connection_readable_cb (GSocket *socket, GIOCondition condition, Client *client);
static void     track_client           (QmiProxy *self, Client *client);
static void     untrack_client         (QmiProxy *self, Client *client);

static void
client_disconnect (Client *client)
{
    if (client->connection_readable_source) {
        g_source_destroy (client->connection_readable_source);
        g_source_unref (client->connection_readable_source);
        client->connection_readable_source = 0;
    }

    if (client->connection) {
        g_debug ("Client (%d) connection closed...", g_socket_get_fd (g_socket_connection_get_socket (client->connection)));
        g_output_stream_close (g_io_stream_get_output_stream (G_IO_STREAM (client->connection)), NULL, NULL);
        g_object_unref (client->connection);
        client->connection = NULL;
    }
}

static void
client_unref (Client *client)
{
    if (g_atomic_int_dec_and_test (&client->ref_count)) {
        /* Ensure disconnected */
        client_disconnect (client);

        if (client->device) {
            if (g_signal_handler_is_connected (client->device, client->indication_id))
                g_signal_handler_disconnect (client->device, client->indication_id);
            if (g_signal_handler_is_connected (client->device, client->device_removed_id))
                g_signal_handler_disconnect (client->device, client->device_removed_id);
            g_object_unref (client->device);
        }

        g_clear_pointer (&client->buffer,                      g_byte_array_unref);
        g_clear_pointer (&client->internal_proxy_open_request, g_byte_array_unref);
        g_clear_pointer (&client->qmi_client_info_array,       g_array_unref);

        g_slice_free (Client, client);
    }
}

static Client *
client_ref (Client *client)
{
    g_atomic_int_inc (&client->ref_count);
    return client;
}

static gboolean
client_send_message (Client      *client,
                     QmiMessage  *message,
                     GError     **error)
{
    if (!client->connection) {
        g_set_error (error,
                     QMI_CORE_ERROR,
                     QMI_CORE_ERROR_WRONG_STATE,
                     "Cannot send message: not connected");
        return FALSE;
    }

    g_debug ("Client (%d) TX: %u bytes", g_socket_get_fd (g_socket_connection_get_socket (client->connection)), message->len);
    if (!g_output_stream_write_all (g_io_stream_get_output_stream (G_IO_STREAM (client->connection)),
                                    message->data,
                                    message->len,
                                    NULL, /* bytes_written */
                                    NULL, /* cancellable */
                                    error)) {
        g_prefix_error (error, "Cannot send message to client: ");
        return FALSE;
    }

    return TRUE;
}

/*****************************************************************************/
/* Track/untrack clients */

static void
track_client (QmiProxy *self,
              Client   *client)
{
    self->priv->clients = g_list_append (self->priv->clients, client_ref (client));
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_N_CLIENTS]);
}

static void
disown_not_released_clients (QmiProxy *self,
                             Client   *client)
{
    guint i;

    if (!client->qmi_client_info_array || !client->qmi_client_info_array->len)
        return;

    for (i = 0; i < client->qmi_client_info_array->len; i++) {
        QmiClientInfo *info;

        info = &g_array_index (client->qmi_client_info_array, QmiClientInfo, i);
        g_debug ("QMI client disowned [%s,%s,%u]",
                 qmi_device_get_path_display (client->device),
                 qmi_service_get_string (info->service),
                 info->cid);
    }

    if (!self->priv->disowned_qmi_client_info_array)
        self->priv->disowned_qmi_client_info_array = g_steal_pointer (&client->qmi_client_info_array);
    else {
        self->priv->disowned_qmi_client_info_array = g_array_append_vals (self->priv->disowned_qmi_client_info_array,
                                                                         client->qmi_client_info_array->data,
                                                                         client->qmi_client_info_array->len);
        g_clear_pointer (&client->qmi_client_info_array, g_array_unref);
    }
}

static void device_close_if_unused (QmiProxy  *self,
                                    QmiDevice *device);

static void
untrack_client (QmiProxy *self,
                Client   *client)
{
    g_autoptr(QmiDevice) device = NULL;

    device = client->device ? g_object_ref (client->device) : NULL;

    /* Disconnect the client explicitly when untracking */
    client_disconnect (client);

    /* Disown all QMI clients that were not explicitly released */
    disown_not_released_clients (self, client);

    if (g_list_find (self->priv->clients, client)) {
        self->priv->clients = g_list_remove (self->priv->clients, client);
        client_unref (client);
        g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_N_CLIENTS]);
    }

    if (device)
        device_close_if_unused (self, device);
}

static QmiDevice *
find_device_for_path (QmiProxy *self,
                      const gchar *path)
{
    GList *l;

    for (l = self->priv->devices; l; l = g_list_next (l)) {
        QmiDevice *device;

        device = (QmiDevice *)l->data;

        /* Return if found */
        if (g_str_equal (qmi_device_get_path (device), path))
            return device;
    }

    return NULL;
}

static void
complete_internal_proxy_open (QmiProxy *self,
                              Client   *client)
{
    QmiMessage *response;
    GError *error = NULL;

    g_debug ("connection to QMI device '%s' established", qmi_device_get_path (client->device));

    g_assert (client->internal_proxy_open_request != NULL);
    response = qmi_message_response_new (client->internal_proxy_open_request, QMI_PROTOCOL_ERROR_NONE);
    qmi_message_unref (client->internal_proxy_open_request);
    client->internal_proxy_open_request = NULL;

    if (!client_send_message (client, response, &error)) {
        g_warning ("couldn't send proxy open response to client: %s", error->message);
        g_error_free (error);
        untrack_client (self, client);
    }

    qmi_message_unref (response);
}

static void
indication_cb (QmiDevice *device,
               QmiMessage *message,
               Client *client)
{
    guint i;

    if (!client->qmi_client_info_array)
        return;

    for (i = 0; i < client->qmi_client_info_array->len; i++) {
        QmiClientInfo *info;

        info = &g_array_index (client->qmi_client_info_array, QmiClientInfo, i);

        /* If service and CID match; or if service and broadcast, forward to
         * the remote client. This message may therefore be forwarded to multiple
         * clients, all that match the conditions. */
        if ((qmi_message_get_service (message) == info->service) &&
            (qmi_message_get_client_id (message) == info->cid ||
             qmi_message_get_client_id (message) == QMI_CID_BROADCAST)) {
            GError *error = NULL;

            if (!client_send_message (client, message, &error)) {
                g_warning ("couldn't forward indication to client: %s", error->message);
                g_error_free (error);
            }
        }
    }
}

static void
device_removed_cb (QmiDevice *device,
                   Client *client)
{
    untrack_client (client->proxy, client);
}

static void
register_signal_handlers (Client *client)
{
    /* Register for device indications */
    client->indication_id = g_signal_connect (client->device,
                                              "indication",
                                              G_CALLBACK (indication_cb),
                                              client);
    client->device_removed_id = g_signal_connect (client->device,
                                                  "device-removed",
                                                  G_CALLBACK (device_removed_cb),
                                                  client);
}

static void
device_open_ready (QmiDevice *device,
                   GAsyncResult *res,
                   Client *client)
{
    QmiProxy *self = client->proxy;
    QmiDevice *existing;
    GError *error = NULL;

    /* Note: we get a full client ref */

    if (!qmi_device_open_finish (device, res, &error)) {
        g_debug ("couldn't open QMI device: %s", error->message);
        g_error_free (error);
        untrack_client (self, client);
        goto out;
    }

    /* Store device in the proxy independently */
    existing = find_device_for_path (self, qmi_device_get_path (client->device));
    if (existing) {
        /* Race condition, we created two QmiDevices for the same port, just skip ours, no big deal */
        g_object_unref (client->device);
        client->device = g_object_ref (existing);
    } else {
        /* Keep the newly added device in the proxy */
        self->priv->devices = g_list_append (self->priv->devices, g_object_ref (client->device));
    }

    register_signal_handlers (client);

    complete_internal_proxy_open (self, client);

out:
    /* Balance out the reference we got */
    client_unref (client);
}

static void
device_new_ready (GObject *source,
                  GAsyncResult *res,
                  Client *client)
{
    QmiProxy *self = client->proxy;
    GError *error = NULL;

    /* Note: we get a full client ref */

    client->device = qmi_device_new_finish (res, &error);
    if (!client->device) {
        g_debug ("couldn't open QMI device: %s", error->message);
        g_error_free (error);
        untrack_client (self, client);
        goto out;
    }

    qmi_device_open (client->device,
                     QMI_DEVICE_OPEN_FLAGS_NONE,
                     10,
                     NULL,
                     (GAsyncReadyCallback)device_open_ready,
                     client_ref (client)); /* Full ref */

out:
    /* Balance out the reference we got */
    client_unref (client);
}

#if QMI_QRTR_SUPPORTED

static void
device_from_node (QmiProxy *self,
                  Client   *client)
{
    QrtrNode *node;

    g_assert (self->priv->qrtr_bus);

    node = qrtr_bus_peek_node (self->priv->qrtr_bus, client->node_id);
    if (!node) {
        g_debug ("node with id %u not found in QRTR bus", client->node_id);
        untrack_client (self, client);
        return;
    }

    qmi_device_new_from_node (node,
                              NULL,
                              (GAsyncReadyCallback)device_new_ready,
                              client_ref (client)); /* full reference */
}

static void
bus_new_ready (GObject      *source,
               GAsyncResult *res,
               Client       *client)
{
    QmiProxy            *self;
    g_autoptr(QrtrBus)   qrtr_bus = NULL;
    g_autoptr(GError)    error = NULL;

    /* Note: we get a full client ref */

    self = client->proxy;

    qrtr_bus = qrtr_bus_new_finish (res, &error);
    if (!qrtr_bus) {
        g_debug ("couldn't access QRTR bus: %s", error->message);
        untrack_client (self, client);
        client_unref (client);
        return;
    }

    /* Don't store the new bus if some other concurrent request already did
     * it. We don't really care which bus object to use, we just need to use
     * always the same */
    if (!self->priv->qrtr_bus)
        self->priv->qrtr_bus = g_object_ref (qrtr_bus);

    device_from_node (self, client);
    client_unref (client);
}

#endif

static gboolean
process_internal_proxy_open (QmiProxy   *self,
                             Client     *client,
                             QmiMessage *message)
{
    gsize              offset = 0;
    gsize              init_offset;
    g_autofree gchar  *incoming_path = NULL;
    g_autofree gchar  *device_file_path = NULL;
    g_autoptr(GError)  error = NULL;

    if ((init_offset = qmi_message_tlv_read_init (message, QMI_MESSAGE_CTL_INTERNAL_PROXY_OPEN_INPUT_TLV_DEVICE_PATH, NULL, &error)) == 0) {
        g_debug ("ignoring message from client: invalid proxy open request: %s", error->message);
        return FALSE;
    }

    if (!qmi_message_tlv_read_string (message, init_offset, &offset, 0, 0, &incoming_path, &error)) {
        g_debug ("ignoring message from client: invalid device file path: %s", error->message);
        return FALSE;
    }

    /* The incoming path may be a symlink. In the proxy, we always use the real path of the
     * device, so that clients using different symlinks for the same file don't collide with
     * each other. */
    device_file_path = qmi_helpers_get_devpath (incoming_path, &error);
    if (!device_file_path) {
        g_warning ("Error looking up real device path: %s", error->message);
        return FALSE;
    }

    /* The remaining size of the buffer needs to be 0 if we successfully read the TLV */
    if ((offset = qmi_message_tlv_read_remaining_size (message, init_offset, offset)) > 0)
        g_warning ("Left '%" G_GSIZE_FORMAT "' bytes unread when getting the 'Device Path' TLV", offset);

    g_debug ("valid request to open connection to QMI device file: %s", device_file_path);

    /* Keep it */
    client->internal_proxy_open_request = qmi_message_ref (message);

    client->device = find_device_for_path (self, device_file_path);

    /* Need to create a device ourselves */
    if (!client->device) {
#if QMI_QRTR_SUPPORTED
        if (qrtr_get_node_for_uri (device_file_path, &client->node_id)) {
            if (!self->priv->qrtr_bus) {
                qrtr_bus_new (1000, /* ms */
                              NULL,
                              (GAsyncReadyCallback)bus_new_ready,
                              client_ref (client)); /* Full ref */
                return TRUE;
            }

            device_from_node (self, client);
            return TRUE;
        }
#endif
        {
            g_autoptr(GFile) file = NULL;

            file = g_file_new_for_path (device_file_path);
            qmi_device_new (file,
                            NULL,
                            (GAsyncReadyCallback)device_new_ready,
                            client_ref (client)); /* Full ref */
            return TRUE;
        }
    }

    register_signal_handlers (client);

    /* Keep a reference to the device in the client */
    g_object_ref (client->device);

    complete_internal_proxy_open (self, client);
    return FALSE;
}

static gint
qmi_client_info_array_lookup_cid (GArray     *array,
                                  QmiService  service,
                                  guint8      cid)
{
    guint i;

    if (!array)
        return -1;

    for (i = 0; i < array->len; i++) {
        QmiClientInfo *item;

        item = &g_array_index (array, QmiClientInfo, i);
        if (item->service == service && item->cid == cid)
            return (gint)i;
    }
    return -1;
}

static void
track_cid (Client     *client,
           QmiMessage *message)
{
    gsize          offset = 0;
    gsize          init_offset;
    guint16        error_status;
    guint16        error_code;
    GError        *error = NULL;
    guint8         service_tmp;
    QmiClientInfo  info;
    gint           i;

    g_assert_cmpuint (qmi_message_get_service (message), ==, QMI_SERVICE_CTL);
    g_assert (qmi_message_is_response (message));

    if (((init_offset = qmi_message_tlv_read_init (message, QMI_MESSAGE_OUTPUT_TLV_RESULT, NULL, &error)) == 0) ||
        !qmi_message_tlv_read_guint16 (message, init_offset, &offset, QMI_ENDIAN_LITTLE, &error_status, &error) ||
        !qmi_message_tlv_read_guint16 (message, init_offset, &offset, QMI_ENDIAN_LITTLE, &error_code, &error)) {
        g_warning ("invalid 'CTL allocate CID' response: missing or invalid result TLV: %s", error->message);
        g_error_free (error);
        return;
    }
    g_warn_if_fail (qmi_message_tlv_read_remaining_size (message, init_offset, offset) == 0);
    if ((error_status != 0x00) || (error_code != QMI_PROTOCOL_ERROR_NONE))
        return;

    offset = 0;
    if (((init_offset = qmi_message_tlv_read_init (message, QMI_MESSAGE_OUTPUT_TLV_ALLOCATION_INFO, NULL, &error)) == 0) ||
        !qmi_message_tlv_read_guint8 (message, init_offset, &offset, &service_tmp, &error) ||
        !qmi_message_tlv_read_guint8 (message, init_offset, &offset, &(info.cid), &error)) {
        g_warning ("invalid 'CTL allocate CID' response: missing or invalid allocation info TLV: %s", error->message);
        g_error_free (error);
        return;
    }
    info.service = (QmiService)service_tmp;

    /* Check if it already exists */
    i = qmi_client_info_array_lookup_cid (client->qmi_client_info_array, info.service, info.cid);
    if (i < 0) {
        g_debug ("QMI client tracked [%s,%s,%u]",
                 qmi_device_get_path_display (client->device),
                 qmi_service_get_string (info.service),
                 info.cid);
        g_array_append_val (client->qmi_client_info_array, info);
    }
}

static void
untrack_cid (QmiProxy   *self,
             Client     *client,
             QmiMessage *message)
{
    gsize          offset = 0;
    gsize          init_offset;
    GError        *error = NULL;
    guint8         service_tmp;
    QmiClientInfo  info;
    gint           i;

    g_assert_cmpuint (qmi_message_get_service (message), ==, QMI_SERVICE_CTL);
    g_assert (qmi_message_is_request (message));

    if (((init_offset = qmi_message_tlv_read_init (message, QMI_MESSAGE_INPUT_TLV_RELEASE_INFO, NULL, &error)) == 0) ||
        !qmi_message_tlv_read_guint8 (message, init_offset, &offset, &service_tmp, &error) ||
        !qmi_message_tlv_read_guint8 (message, init_offset, &offset, &(info.cid), &error)) {
        g_warning ("invalid 'CTL release CID' request: missing or invalid release info TLV: %s", error->message);
        g_error_free (error);
        return;
    }
    info.service = (QmiService)service_tmp;

    /* Check if it already exists in the client */
    i = qmi_client_info_array_lookup_cid (client->qmi_client_info_array, info.service, info.cid);
    if (i >= 0) {
        g_debug ("QMI client untracked [%s,%s,%u]",
                 qmi_device_get_path_display (client->device),
                 qmi_service_get_string (info.service),
                 info.cid);
        g_array_remove_index (client->qmi_client_info_array, i);
        return;
    }

    /* Otherwise, check if it wasn't onwned */
    i = qmi_client_info_array_lookup_cid (self->priv->disowned_qmi_client_info_array, info.service, info.cid);
    if (i >= 0) {
        g_debug ("disowned QMI client untracked [%s,%s,%u]",
                 qmi_device_get_path_display (client->device),
                 qmi_service_get_string (info.service),
                 info.cid);
        g_array_remove_index (self->priv->disowned_qmi_client_info_array, i);
        return;
    }

    g_debug ("unexpected attempt to release QMI client [%s,%s,%u]",
             qmi_device_get_path_display (client->device),
             qmi_service_get_string (info.service),
             info.cid);
}

static void
track_implicit_cid (QmiProxy   *self,
                    Client     *client,
                    QmiMessage *message)
{
    QmiClientInfo info;
    gint          i;

    info.service = qmi_message_get_service (message);
    info.cid     = qmi_message_get_client_id (message);

    g_assert_cmpuint (info.service, !=, QMI_SERVICE_CTL);

    /* Check if the QMI client already exists in the client application, and if
     * so, nothing else to do */
    i = qmi_client_info_array_lookup_cid (client->qmi_client_info_array, info.service, info.cid);
    if (i >= 0)
        return;

    /* The QMI client doesn't exist in the client application, see if it
     * was disowned previously */
    i = qmi_client_info_array_lookup_cid (self->priv->disowned_qmi_client_info_array, info.service, info.cid);
    if (i >= 0) {
        /* Remove client info from array of disowned ones, and append it to the client */
        g_debug ("QMI client reowned [%s,%s,%u]",
                 qmi_device_get_path_display (client->device),
                 qmi_service_get_string (info.service),
                 info.cid);
        g_array_remove_index (self->priv->disowned_qmi_client_info_array, i);
        g_array_append_val (client->qmi_client_info_array, info);
        return;
    }

    /* The QMI client wasn't disowned earlier either. Well, this could be due to the proxy
     * having crashed and restarted. Just create a new client info from scratch. */
    g_debug ("QMI client tracked implicitly [%s,%s,%u]",
             qmi_device_get_path_display (client->device),
             qmi_service_get_string (info.service),
             info.cid);
    g_array_append_val (client->qmi_client_info_array, info);
}

/*****************************************************************************/

#define TRACK_CTL_QUARK_STR "track-ctl-data"
static GQuark track_ctl_quark;

static void
device_track_ctl_request (QmiDevice *device)
{
    guint ongoing_ctl;

    if (G_UNLIKELY (!track_ctl_quark)) {
        track_ctl_quark = g_quark_from_static_string (TRACK_CTL_QUARK_STR);
        ongoing_ctl = 0;
    } else
        ongoing_ctl = GPOINTER_TO_UINT (g_object_get_qdata (G_OBJECT (device), track_ctl_quark));
    g_object_set_qdata (G_OBJECT (device), track_ctl_quark, GUINT_TO_POINTER (ongoing_ctl + 1));
}

static void
device_untrack_ctl_request (QmiDevice *device)
{
    guint ongoing_ctl;

    g_assert (track_ctl_quark != 0);
    ongoing_ctl = GPOINTER_TO_UINT (g_object_get_qdata (G_OBJECT (device), track_ctl_quark));
    g_assert_cmpuint (ongoing_ctl, >, 0);
    g_object_set_qdata (G_OBJECT (device), track_ctl_quark, GUINT_TO_POINTER (ongoing_ctl - 1));
}

static void
device_close_if_unused (QmiProxy  *self,
                        QmiDevice *device)
{
    GList *l;

    /* If there is at least one client using the device,
     * no need to close */
    for (l = self->priv->clients; l; l = g_list_next (l)) {
        Client *client = l->data;

        if (client->device &&
            (device == client->device ||
             g_str_equal (qmi_device_get_path (device), qmi_device_get_path (client->device))))
            return;
    }

    /* If there are no clients using the device BUT there
     * are still ongoing CTL requests ongoing, no need to
     * close */
    if (GPOINTER_TO_UINT (g_object_get_qdata (G_OBJECT (device), track_ctl_quark)) > 0)
        return;

    /* Now, untrack device from proxy and close it */
    for (l = self->priv->devices; l; l = g_list_next (l)) {
        QmiDevice *device_in_list = QMI_DEVICE (l->data);

        if (device_in_list &&
            (device == device_in_list ||
             g_str_equal (qmi_device_get_path (device), qmi_device_get_path (device_in_list)))) {
            g_debug ("closing device '%s': no longer used", qmi_device_get_path_display (device));
            qmi_device_close_async (device_in_list, 0, NULL, NULL, NULL);
            g_object_unref (device_in_list);
            self->priv->devices = g_list_remove (self->priv->devices, device_in_list);
            return;
        }
    }

    g_assert_not_reached ();
}

/*****************************************************************************/

typedef struct {
    QmiProxy *self;   /* Full ref */
    Client   *client; /* Full ref */
    guint8    in_trid;
    gboolean  ctl;
} Request;

static void
request_free (Request *request)
{
    if (!request)
        return;
    client_unref (request->client);
    g_object_unref (request->self);
    g_slice_free (Request, request);
}

static void
device_command_ready (QmiDevice    *device,
                      GAsyncResult *res,
                      Request      *request)
{
    g_autoptr(QmiMessage) response = NULL;
    g_autoptr(GError)     error = NULL;

    response = qmi_device_command_full_finish (device, res, &error);
    if (!response) {
        g_warning ("sending request to device failed: %s", error->message);
        goto out;
    }

    if (qmi_message_get_service (response) == QMI_SERVICE_CTL) {
        qmi_message_set_transaction_id (response, request->in_trid);
        if (qmi_message_get_message_id (response) == QMI_MESSAGE_CTL_ALLOCATE_CID)
            track_cid (request->client, response);
    }

    if (!client_send_message (request->client, response, &error)) {
        /* ignore errors when client is not connected, because it really didn't
         * need this response back */
        if (!g_error_matches (error, QMI_CORE_ERROR, QMI_CORE_ERROR_WRONG_STATE))
            g_warning ("forwarding response to client failed: %s", error->message);
        untrack_client (request->self, request->client);
    }

 out:
    if (request->ctl) {
        device_untrack_ctl_request (device);
        device_close_if_unused (request->self, device);
    }
    request_free (request);
}

static gboolean
process_message (QmiProxy   *self,
                 Client     *client,
                 QmiMessage *message)
{
    Request *request;

    /* Accept only request messages from the client */
    if (!qmi_message_is_request (message)) {
        g_debug ("invalid message from client: not a request message");
        return FALSE;
    }

    if (qmi_message_get_service (message) == QMI_SERVICE_CTL &&
        qmi_message_get_message_id (message) == QMI_MESSAGE_CTL_INTERNAL_PROXY_OPEN)
        return process_internal_proxy_open (self, client, message);

    request = g_slice_new0 (Request);
    request->self = g_object_ref (self);
    request->client = client_ref (client);

    if (qmi_message_get_service (message) == QMI_SERVICE_CTL) {
        /* Keep track of how many CTL requests are ongoing */
        device_track_ctl_request (client->device);
        request->ctl = TRUE;
        request->in_trid = qmi_message_get_transaction_id (message);
        qmi_message_set_transaction_id (message, 0);
        /* Try to untrack QMI client as soon as we detect the associated
         * release message, no need to wait for the response. */
        if (qmi_message_get_message_id (message) == QMI_MESSAGE_CTL_RELEASE_CID)
            untrack_cid (self, client, message);
    } else
        track_implicit_cid (self, client, message);

    /* The timeout needs to be big enough for any kind of transaction to
     * complete, otherwise the remote clients will lose the reply if they
     * configured a timeout bigger than this internal one. We should likely
     * make this value configurable per-client, instead of a hardcoded value.
     *
     * Note: the proxy will not translate vendor-specific messages in its
     * logs (as it doesn't have the original message context with the vendor
     * id).
     */
    qmi_device_command_full (client->device,
                             message,
                             NULL,
                             300,
                             NULL,
                             (GAsyncReadyCallback)device_command_ready,
                             request);
    return TRUE;
}

static void
parse_request (QmiProxy *self,
               Client   *client)
{
    do {
        GError *error = NULL;
        QmiMessage *message;

        /* Every message received must start with the QMUX marker.
         * If it doesn't, we broke framing :-/
         * If we broke framing, an error should be reported and the device
         * should get closed */
        if (client->buffer->len > 0 &&
            client->buffer->data[0] != QMI_MESSAGE_QMUX_MARKER) {
            /* TODO: Report fatal error */
            g_warning ("QMI framing error detected");
            return;
        }

        message = qmi_message_new_from_raw (client->buffer, &error);
        if (!message) {
            if (!error)
                /* More data we need */
                return;

            /* Warn about the issue */
            g_warning ("Invalid QMI message received: '%s'",
                       error->message);
            g_error_free (error);
        } else {
            /* Play with the received message */
            process_message (self, client, message);
            qmi_message_unref (message);
        }
    } while (client->buffer->len > 0);
}

static gboolean
connection_readable_cb (GSocket *socket,
                        GIOCondition condition,
                        Client *client)
{
    QmiProxy *self;
    guint8 buffer[BUFFER_SIZE];
    GError *error = NULL;
    gssize r;

    self = client->proxy;

    if (condition & G_IO_IN || condition & G_IO_PRI) {
        r = g_input_stream_read (g_io_stream_get_input_stream (G_IO_STREAM (client->connection)),
                                buffer,
                                BUFFER_SIZE,
                                NULL,
                                &error);
        if (r < 0) {
            g_warning ("Error reading from istream: %s", error ? error->message : "unknown");
            if (error)
                g_error_free (error);
            untrack_client (self, client);
            return FALSE;
        }

        if (r > 0) {
            if (!G_UNLIKELY (client->buffer))
                client->buffer = g_byte_array_sized_new (r);
            g_byte_array_append (client->buffer, buffer, r);

            /* Try to parse input messages */
            parse_request (self, client);
        }
    }

    if (condition & G_IO_HUP || condition & G_IO_ERR) {
        untrack_client (self, client);
        return FALSE;
    }

    return TRUE;
}

static void
incoming_cb (GSocketService *service,
             GSocketConnection *connection,
             GObject *unused,
             QmiProxy *self)
{
    Client *client;
    GCredentials *credentials;
    GError *error = NULL;
    uid_t uid;

    g_debug ("Client (%d) connection open...", g_socket_get_fd (g_socket_connection_get_socket (connection)));

    credentials = g_socket_get_credentials (g_socket_connection_get_socket (connection), &error);
    if (!credentials) {
        g_warning ("Client not allowed: Error getting socket credentials: %s", error->message);
        g_error_free (error);
        return;
    }

    uid = g_credentials_get_unix_user (credentials, &error);
    g_object_unref (credentials);
    if (error) {
        g_warning ("Client not allowed: Error getting unix user id: %s", error->message);
        g_error_free (error);
        return;
    }
    if (!qmi_helpers_check_user_allowed (uid, &error)) {
        g_warning ("Client not allowed: %s", error->message);
        g_error_free (error);
        return;
    }

    /* Create client */
    client = g_slice_new0 (Client);
    client->ref_count = 1;
    client->proxy = self;
    client->connection = g_object_ref (connection);
    client->connection_readable_source = g_socket_create_source (g_socket_connection_get_socket (client->connection),
                                                                 G_IO_IN | G_IO_PRI | G_IO_ERR | G_IO_HUP,
                                                                 NULL);
    g_source_set_callback (client->connection_readable_source,
                           (GSourceFunc)connection_readable_cb,
                           client,
                           NULL);
    g_source_attach (client->connection_readable_source, g_main_context_get_thread_default ());
    client->qmi_client_info_array = g_array_sized_new (FALSE, FALSE, sizeof (QmiClientInfo), 8);

    /* Keep the client info around */
    track_client (self, client);

    client_unref (client);
}

static gboolean
setup_socket_service (QmiProxy *self,
                      GError **error)
{
    GSocketAddress *socket_address;
    GSocket *socket;

    socket = g_socket_new (G_SOCKET_FAMILY_UNIX,
                           G_SOCKET_TYPE_STREAM,
                           G_SOCKET_PROTOCOL_DEFAULT,
                           error);
    if (!socket)
        return FALSE;

    /* Bind to address */
    socket_address = (g_unix_socket_address_new_with_type (
                          QMI_PROXY_SOCKET_PATH,
                          -1,
                          G_UNIX_SOCKET_ADDRESS_ABSTRACT));
    if (!g_socket_bind (socket, socket_address, TRUE, error))
        return FALSE;
    g_object_unref (socket_address);

    g_debug ("creating UNIX socket service...");

    /* Listen */
    if (!g_socket_listen (socket, error)) {
        g_object_unref (socket);
        return FALSE;
    }

    /* Create socket service */
    self->priv->socket_service = g_socket_service_new ();
    g_signal_connect (self->priv->socket_service, "incoming", G_CALLBACK (incoming_cb), self);
    if (!g_socket_listener_add_socket (G_SOCKET_LISTENER (self->priv->socket_service),
                                       socket,
                                       NULL, /* don't pass an object, will take a reference */
                                       error)) {
        g_prefix_error (error, "Error adding socket at '%s' to socket service: ", QMI_PROXY_SOCKET_PATH);
        g_object_unref (socket);
        return FALSE;
    }

    g_debug ("starting UNIX socket service at '%s'...", QMI_PROXY_SOCKET_PATH);
    g_socket_service_start (self->priv->socket_service);
    g_object_unref (socket);
    return TRUE;
}

/*****************************************************************************/

QmiProxy *
qmi_proxy_new (GError **error)
{
    QmiProxy *self;

    if (!qmi_helpers_check_user_allowed (getuid (), error))
        return NULL;

    self = g_object_new (QMI_TYPE_PROXY, NULL);
    if (!setup_socket_service (self, error))
        g_clear_object (&self);
    return self;
}

static void
qmi_proxy_init (QmiProxy *self)
{
    /* Setup private data */
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
                                              QMI_TYPE_PROXY,
                                              QmiProxyPrivate);
}

static void
get_property (GObject *object,
              guint prop_id,
              GValue *value,
              GParamSpec *pspec)
{
    QmiProxy *self = QMI_PROXY (object);

    switch (prop_id) {
    case PROP_N_CLIENTS:
        g_value_set_uint (value, g_list_length (self->priv->clients));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
dispose (GObject *object)
{
    QmiProxyPrivate *priv = QMI_PROXY (object)->priv;

    g_clear_pointer (&priv->disowned_qmi_client_info_array, g_array_unref);
    g_list_free_full (g_steal_pointer (&priv->clients), (GDestroyNotify) client_unref);

    if (priv->socket_service) {
        if (g_socket_service_is_active (priv->socket_service))
            g_socket_service_stop (priv->socket_service);
        g_clear_object (&priv->socket_service);
        g_unlink (QMI_PROXY_SOCKET_PATH);
        g_debug ("UNIX socket service at '%s' stopped", QMI_PROXY_SOCKET_PATH);
    }

#if QMI_QRTR_SUPPORTED
    g_clear_object (&priv->qrtr_bus);
#endif

    G_OBJECT_CLASS (qmi_proxy_parent_class)->dispose (object);
}

static void
qmi_proxy_class_init (QmiProxyClass *proxy_class)
{
    GObjectClass *object_class = G_OBJECT_CLASS (proxy_class);

    g_type_class_add_private (object_class, sizeof (QmiProxyPrivate));

    object_class->get_property = get_property;
    object_class->dispose = dispose;

    /**
     * QmiProxy:qmi-proxy-n-clients
     *
     * Since: 1.8
     */
    properties[PROP_N_CLIENTS] =
        g_param_spec_uint (QMI_PROXY_N_CLIENTS,
                           "Number of clients",
                           "Number of clients currently connected to the proxy",
                           0,
                           G_MAXUINT,
                           0,
                           G_PARAM_READABLE);
    g_object_class_install_property (object_class, PROP_N_CLIENTS, properties[PROP_N_CLIENTS]);
}
