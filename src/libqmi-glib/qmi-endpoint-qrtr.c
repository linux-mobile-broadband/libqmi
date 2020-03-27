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
 * Copyright (C) 2019-2020 Eric Caruso <ejcaruso@chromium.org>
 * Copyright (C) 2020 Aleksander Morgado <aleksander@aleksander.es>
 */

#include <errno.h>
#include <linux/qrtr.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <gmodule.h>
#include <gio/gio.h>

#include <libqrtr-glib.h>

#include "qmi-endpoint-qrtr.h"
#include "qmi-errors.h"
#include "qmi-error-types.h"
#include "qmi-message.h"

#define QMI_MESSAGE_OUTPUT_TLV_RESULT 0x02

/* Constants for allocating/releasing clients */
#define QMI_MESSAGE_CTL_ALLOCATE_CID 0x0022
#define QMI_MESSAGE_CTL_RELEASE_CID 0x0023
#define QMI_MESSAGE_TLV_ALLOCATION_INFO 0x01
#define QMI_MESSAGE_INPUT_TLV_SERVICE 0x01

#define QMI_MESSAGE_CTL_GET_VERSION_INFO 0x0021
#define QMI_MESSAGE_CTL_SYNC 0x0027

G_DEFINE_TYPE (QmiEndpointQrtr, qmi_endpoint_qrtr, QMI_TYPE_ENDPOINT)

struct _QmiEndpointQrtrPrivate {
    QrtrNode *node;
    guint node_removed_id;
    gboolean node_removed;

    /* Holds ClientInfo entries */
    GList *client_list;
    /* Map of client id -> ClientInfo */
    GTree *client_map;
    /* Map of socket -> ClientInfo */
    GHashTable *socket_map;
};

typedef struct {
    guint client_id;
    GSocket *socket;
    GSource *source;
} ClientInfo;

/*****************************************************************************/

static gboolean
add_qmi_message_to_buffer (QmiEndpointQrtr *self,
                           QmiMessage *message)
{
    const guint8 *raw_message;
    gsize raw_message_len;
    GError *error = NULL;

    raw_message = qmi_message_get_raw (message, &raw_message_len, &error);
    if (!raw_message) {
        g_warning ("[%s] Got malformed QMI message: %s",
                   qmi_endpoint_get_name (QMI_ENDPOINT (self)), error->message);
        return FALSE;
    }

    qmi_endpoint_add_message (QMI_ENDPOINT (self), raw_message, raw_message_len);
    qmi_message_unref (message);
    return TRUE;
}

static gboolean
qrtr_message_cb (GSocket *gsocket,
                 GIOCondition cond,
                 QmiEndpointQrtr *self)
{
    g_autoptr(GError) error = NULL;
    g_autoptr(GSocketAddress) addr = NULL;
    struct sockaddr_qrtr sq;
    g_autoptr(GByteArray) buf = NULL;
    gssize next_datagram_size;
    gssize bytes_received;
    ClientInfo *info;
    QmiService service;
    guint client;
    QmiMessage *message;

    info = g_hash_table_lookup (self->priv->socket_map, gsocket);
    if (!info) {
        /* We're no longer watching this socket. */
        return FALSE;
    }

    next_datagram_size = g_socket_get_available_bytes (gsocket);
    buf = g_byte_array_new ();
    g_byte_array_set_size (buf, next_datagram_size);

    bytes_received = g_socket_receive_from (gsocket, &addr, (gchar *)buf->data,
                                            next_datagram_size, NULL, &error);
    if (bytes_received < 0) {
        g_warning ("[%s] Socket IO failure: %s",
                   qmi_endpoint_get_name (QMI_ENDPOINT (self)), error->message);
        g_signal_emit_by_name (QMI_ENDPOINT (self), QMI_ENDPOINT_SIGNAL_HANGUP);
        return FALSE;
    }

    if (bytes_received != next_datagram_size) {
        g_debug ("[%s] Datagram was not expected size", qmi_endpoint_get_name (QMI_ENDPOINT (self)));
        return TRUE;
    }

    /* Figure out where we got this message from */
    if (!g_socket_address_to_native (addr, &sq, sizeof(sq), &error)) {
        g_warning ("[%s] Could not parse QRTR address: %s",
                   qmi_endpoint_get_name (QMI_ENDPOINT (self)), error->message);
        return TRUE;
    }

    if (sq.sq_family != AF_QIPCRTR ||
        sq.sq_node != qrtr_node_id (self->priv->node) ||
        sq.sq_port == QRTR_PORT_CTRL) {
        /* ignore all CTRL messages or ones not from our node, we only want real
         * QMI messages */
        return TRUE;
    }

    /* Create a fake QMUX header and add this message to the buffer */
    service = qrtr_node_lookup_service (self->priv->node, sq.sq_port);
    client = info->client_id;
    message = qmi_message_new_from_data (service, client, buf, &error);
    if (!message) {
        g_warning ("[%s] Got malformed QMI message: %s",
                   qmi_endpoint_get_name (QMI_ENDPOINT (self)), error->message);
        return TRUE;
    }

    add_qmi_message_to_buffer (self, message);
    return TRUE;
}

static void
node_removed_cb (QrtrNode *node,
                 QmiEndpointQrtr *self)
{
    self->priv->node_removed = TRUE;
    g_signal_emit_by_name (QMI_ENDPOINT (self), QMI_ENDPOINT_SIGNAL_HANGUP);
}


/*****************************************************************************/
/* Client info operations */

static void
client_info_destroy (ClientInfo *info)
{
    if (info->source) {
        g_source_destroy (info->source);
        g_source_unref (info->source);
    }
    if (info->socket) {
        g_socket_close (info->socket, NULL);
        g_clear_object (&info->socket);
    }
    g_slice_free (ClientInfo, info);
}

static gint
client_map_key_cmp (gconstpointer v1,
                    gconstpointer v2)
{
    return GPOINTER_TO_INT (v1) - GPOINTER_TO_INT (v2);
}

static gboolean
client_map_traverse_func (gpointer key,
                          gpointer value,
                          gpointer data)
{
    if (GPOINTER_TO_UINT (key) > *(guint *)data + 1)
        return TRUE;

    *(guint *)data = GPOINTER_TO_UINT (key);
    return FALSE;
}

static guint
get_next_free_id (GTree *client_map)
{
    guint last_contiguous_id = -1;
    g_tree_foreach (client_map, client_map_traverse_func, &last_contiguous_id);
    return last_contiguous_id + 1;
}

static gint
allocate_client (QmiEndpointQrtr *self, GError **error)
{
    GError *inner_error = NULL;
    int socket_fd;
    GSocket *gsocket;
    guint client_id;
    ClientInfo *info;

    if (!self->priv->client_map) {
        g_set_error (error,
                     QMI_CORE_ERROR,
                     QMI_CORE_ERROR_WRONG_STATE,
                     "[%s] Endpoint is not open",
                     qmi_endpoint_get_name (QMI_ENDPOINT (self)));
        return -QMI_PROTOCOL_ERROR_INTERNAL;
    }

    client_id = get_next_free_id (self->priv->client_map);
    if (client_id > G_MAXUINT8) {
        g_set_error (error,
                     QMI_CORE_ERROR,
                     QMI_CORE_ERROR_FAILED,
                     "[%s] Client IDs have been exhausted",
                     qmi_endpoint_get_name (QMI_ENDPOINT (self)));
        return -QMI_PROTOCOL_ERROR_CLIENT_IDS_EXHAUSTED;
    }

    socket_fd = socket (AF_QIPCRTR, SOCK_DGRAM, 0);
    if (socket_fd < 0) {
        g_set_error (error,
                     QMI_CORE_ERROR,
                     QMI_CORE_ERROR_FAILED,
                     "[%s] Could not open QRTR socket: %s",
                     qmi_endpoint_get_name (QMI_ENDPOINT (self)), strerror(errno));
        return -QMI_PROTOCOL_ERROR_INTERNAL;
    }

    gsocket = g_socket_new_from_fd (socket_fd, &inner_error);
    if (!gsocket) {
        g_propagate_prefixed_error (error, inner_error,
                                    "[%s] Socket setup failed: ",
                                    qmi_endpoint_get_name (QMI_ENDPOINT (self)));
        close (socket_fd);
        return -QMI_PROTOCOL_ERROR_INTERNAL;
    }

    g_socket_set_timeout (gsocket, 0);

    info = g_slice_new0 (ClientInfo);
    info->client_id = client_id;
    info->socket = gsocket;
    info->source = g_socket_create_source (gsocket, G_IO_IN, NULL);
    g_source_set_callback (info->source, (GSourceFunc) qrtr_message_cb,
                           self, NULL);
    g_source_attach (info->source, NULL);

    self->priv->client_list = g_list_append (self->priv->client_list, info);
    g_tree_insert (self->priv->client_map, GUINT_TO_POINTER (info->client_id), info);
    g_hash_table_insert (self->priv->socket_map, gsocket, info);
    return info->client_id;
}

static void
release_client (QmiEndpointQrtr *self, guint client_id)
{
    ClientInfo *info;

    info = g_tree_lookup (self->priv->client_map, GUINT_TO_POINTER (client_id));
    if (!info)
        return;

    g_hash_table_remove (self->priv->socket_map, info->socket);
    g_tree_remove (self->priv->client_map, GUINT_TO_POINTER (client_id));
    self->priv->client_list = g_list_remove (self->priv->client_list, info);
    client_info_destroy (info);
}

static void
client_list_destroy (GList *list)
{
    g_list_free_full (list, (GDestroyNotify) client_info_destroy);
}

/*****************************************************************************/

static gboolean
construct_alloc_tlv (QmiMessage *message, guint8 service, guint8 client)
{
    gsize init_offset;

    init_offset = qmi_message_tlv_write_init (message,
                                              QMI_MESSAGE_TLV_ALLOCATION_INFO,
                                              NULL);
	return init_offset &&
	    qmi_message_tlv_write_guint8 (message, service, NULL) &&
	    qmi_message_tlv_write_guint8 (message, client, NULL) &&
	    qmi_message_tlv_write_complete (message, init_offset, NULL);
}

static void
handle_alloc_cid (QmiEndpointQrtr *self, QmiMessage *message)
{
    gsize offset = 0;
    gsize init_offset;
    guint8 service;
    gint client_id;
    QmiMessage *response;
    QmiProtocolError result = QMI_PROTOCOL_ERROR_NONE;
    GError *error = NULL;

    if (((init_offset = qmi_message_tlv_read_init (message, QMI_MESSAGE_TLV_ALLOCATION_INFO, NULL, &error)) == 0) ||
        !qmi_message_tlv_read_guint8 (message, init_offset, &offset, &service, &error)) {
        g_debug ("Error allocating CID: could not parse message: %s", error->message);
        g_error_free (error);
        result = QMI_PROTOCOL_ERROR_MALFORMED_MESSAGE;
    }

    client_id = allocate_client (self, &error);
    if (client_id < 0) {
        g_debug ("Error allocating CID: could not parse message: %s", error->message);
        g_error_free (error);
        result = -client_id;
    }

    response = qmi_message_response_new (message, result);
    if (!response)
        return;

    if (!construct_alloc_tlv (response, service, client_id)) {
        qmi_message_unref (response);
        return;
    }

    add_qmi_message_to_buffer (self, response);
}

static void
handle_release_cid (QmiEndpointQrtr *self, QmiMessage *message)
{
    gsize offset = 0;
    gsize init_offset;
    guint8 service;
    guint8 client_id = 0;
    QmiMessage *response;
    QmiProtocolError result = QMI_PROTOCOL_ERROR_NONE;
    GError *error = NULL;

    if (((init_offset = qmi_message_tlv_read_init (message, QMI_MESSAGE_TLV_ALLOCATION_INFO, NULL, &error)) == 0) ||
        !qmi_message_tlv_read_guint8 (message, init_offset, &offset, &service, &error) ||
        !qmi_message_tlv_read_guint8 (message, init_offset, &offset, &client_id, &error)) {
        g_debug ("Error releasing CID: could not parse message: %s", error->message);
        g_error_free (error);
        result = QMI_PROTOCOL_ERROR_MALFORMED_MESSAGE;
    }

    release_client (self, client_id);

    response = qmi_message_response_new (message, result);
    if (!response)
        return;

    if (!construct_alloc_tlv (response, service, client_id)) {
        qmi_message_unref (response);
        return;
    }

    add_qmi_message_to_buffer (self, response);
}

static void
handle_sync (QmiEndpointQrtr *self, QmiMessage *message)
{
    QmiMessage *response;

    response = qmi_message_response_new (message, QMI_PROTOCOL_ERROR_NONE);
    if (!response)
        return;

    add_qmi_message_to_buffer (self, response);
}

static void
unhandled_message (QmiEndpointQrtr *self,
                   QmiMessage *message,
                   gboolean log_warning)
{
    QmiMessage *response;

    if (log_warning) {
        gchar *message_text;

        message_text = qmi_message_get_printable_full (message, NULL, "  ");
        g_warning ("[%s] Unhandled client message:\n%s",
                   qmi_endpoint_get_name (QMI_ENDPOINT (self)),
                   message_text);
        g_free (message_text);
    }

    response = qmi_message_response_new (message, QMI_PROTOCOL_ERROR_NOT_SUPPORTED);
    if (!response)
        return;
    add_qmi_message_to_buffer (self, response);
}

static void
handle_ctl_message (QmiEndpointQrtr *self,
                    QmiMessage *message)
{
    switch (qmi_message_get_message_id (message)) {
        case QMI_MESSAGE_CTL_ALLOCATE_CID:
            handle_alloc_cid (self, message);
            break;
        case QMI_MESSAGE_CTL_RELEASE_CID:
            handle_release_cid (self, message);
            break;
        case QMI_MESSAGE_CTL_SYNC:
            handle_sync (self, message);
            break;
        case QMI_MESSAGE_CTL_GET_VERSION_INFO:
            /* Messages we expect to see should not be logged. */
            unhandled_message (self, message, FALSE);
            break;
        default:
            unhandled_message (self, message, TRUE);
            break;
    }
}

/*****************************************************************************/

static gboolean
endpoint_open_finish (QmiEndpoint   *self,
                      GAsyncResult  *res,
                      GError       **error)
{
    return g_task_propagate_boolean (G_TASK (res), error);
}

static void
endpoint_open (QmiEndpoint         *endpoint,
               gboolean             use_proxy,
               guint                timeout,
               GCancellable        *cancellable,
               GAsyncReadyCallback  callback,
               gpointer             user_data)
{
    QmiEndpointQrtr *self;
    GTask *task;

    g_assert (!use_proxy);

    self = QMI_ENDPOINT_QRTR (endpoint);
    task = g_task_new (self, cancellable, callback, user_data);

    if (self->priv->node_removed) {
        g_task_return_new_error (task,
                                 QMI_CORE_ERROR,
                                 QMI_CORE_ERROR_FAILED,
                                 "Node is not present on bus");
        g_object_unref (task);
        return;
    }

    if (self->priv->client_map) {
        g_task_return_new_error (task,
                                 QMI_CORE_ERROR,
                                 QMI_CORE_ERROR_WRONG_STATE,
                                 "Already open");
        g_object_unref (task);
        return;
    }

    g_assert (self->priv->client_list == NULL);
    self->priv->client_map = g_tree_new (client_map_key_cmp);
    self->priv->socket_map = g_hash_table_new (g_direct_hash, g_direct_equal);
    g_task_return_boolean (task, TRUE);
    g_object_unref (task);
}

static gboolean
endpoint_is_open (QmiEndpoint *self)
{
    return !!(QMI_ENDPOINT_QRTR (self)->priv->client_map);
}

static gboolean
endpoint_send (QmiEndpoint   *endpoint,
               QmiMessage    *message,
               guint          timeout,
               GCancellable  *cancellable,
               GError       **error)
{
    QmiEndpointQrtr *self;
    gconstpointer raw_message;
    gsize raw_message_len;
    guint client_id;
    ClientInfo *client;
    QmiService service;
    struct sockaddr_qrtr addr;
    gint sockfd;
    gint rc;
    GError *inner_error = NULL;
    gint sq_port;

    self = QMI_ENDPOINT_QRTR (endpoint);

    /* Figure out if we have a valid client */
    service = qmi_message_get_service (message);
    client_id = qmi_message_get_client_id (message);
    client = g_tree_lookup (self->priv->client_map, GUINT_TO_POINTER (client_id));
    if (service == QMI_SERVICE_CTL) {
        if (client && client->socket) {
            g_set_error (error,
                         QMI_CORE_ERROR,
                         QMI_CORE_ERROR_WRONG_STATE,
                         "Client %u is not a CTL client", client_id);
            return FALSE;
        }
        if (!client) {
            /* Dummy entry for CTL client with no socket */
            ClientInfo *info;

            info = g_slice_new0 (ClientInfo);
            info->client_id = client_id;
            self->priv->client_list = g_list_append (self->priv->client_list, info);
            g_tree_insert (self->priv->client_map, GUINT_TO_POINTER (info->client_id), info);
        }
        /* We implement the control client here, so divert those messages */
        handle_ctl_message (self, message);
        return TRUE;
    }

    /* It's a normal service, so we should have an open client for it. */
    if (!client) {
        g_set_error (error,
                     QMI_CORE_ERROR,
                     QMI_CORE_ERROR_WRONG_STATE,
                     "Client %u is not open", client_id);
        return FALSE;
    }

    /* Lookup port */
    sq_port = qrtr_node_lookup_port (self->priv->node, service);
    if (sq_port == -1) {
        g_set_error (error,
                     QMI_CORE_ERROR,
                     QMI_CORE_ERROR_FAILED,
                     "Service %u has no servers", service);
        return FALSE;
    }

    /* Set up QRTR bus destination address */
    addr.sq_family = AF_QIPCRTR;
    addr.sq_node = qrtr_node_id (self->priv->node);
    addr.sq_port = (guint) sq_port;

    /* Get raw message */
    raw_message = qmi_message_get_data (message, &raw_message_len, &inner_error);
    if (!raw_message) {
        g_propagate_prefixed_error (error, inner_error, "Message was invalid: ");
        return FALSE;
    }

    sockfd = g_socket_get_fd (client->socket);
    rc = sendto (sockfd, (void *)raw_message, raw_message_len,
                 0, (struct sockaddr *)&addr, sizeof (addr));
    if (rc < 0) {
        g_set_error (error,
                     G_IO_ERROR,
                     g_io_error_from_errno (errno),
                     "Failed to send QMI packet");
        return FALSE;
    }

    return TRUE;
}

/*****************************************************************************/

static gboolean
endpoint_close_finish (QmiEndpoint   *self,
                       GAsyncResult  *res,
                       GError       **error)
{
    return g_task_propagate_boolean (G_TASK (res), error);
}

static void
endpoint_close (QmiEndpoint         *endpoint,
                guint                timeout,
                GCancellable        *cancellable,
                GAsyncReadyCallback  callback,
                gpointer             user_data)
{
    QmiEndpointQrtr *self;
    GTask *task;

    self = QMI_ENDPOINT_QRTR (endpoint);
    task = g_task_new (self, cancellable, callback, user_data);

    g_clear_pointer (&self->priv->socket_map, g_hash_table_destroy);
    g_clear_pointer (&self->priv->client_map, g_tree_destroy);
    g_clear_pointer (&self->priv->client_list, client_list_destroy);
    g_task_return_boolean (task, TRUE);
    g_object_unref (task);
}

/*****************************************************************************/

QmiEndpointQrtr *
qmi_endpoint_qrtr_new (QrtrNode *node)
{
    QmiEndpointQrtr *self;
    gchar *uri;
    QmiFile *file;

    if (!node)
        return NULL;

    uri = qrtr_get_uri_for_node (qrtr_node_id (node));
    file = qmi_file_new (g_file_new_for_uri (uri));
    g_free (uri);

    self = g_object_new (QMI_TYPE_ENDPOINT_QRTR,
                         QMI_ENDPOINT_FILE, file,
                         NULL);

    self->priv->node = g_object_ref (node);
    self->priv->node_removed_id = g_signal_connect (node,
                                                    QRTR_NODE_SIGNAL_REMOVED,
                                                    G_CALLBACK (node_removed_cb),
                                                    self);
    return self;
}

/*****************************************************************************/

static void
qmi_endpoint_qrtr_init (QmiEndpointQrtr *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE ((self),
                                              QMI_TYPE_ENDPOINT_QRTR,
                                              QmiEndpointQrtrPrivate);
}

static void
dispose (GObject *object)
{
    QmiEndpointQrtr *self = QMI_ENDPOINT_QRTR (object);

    g_clear_pointer (&self->priv->socket_map, g_hash_table_destroy);
    g_clear_pointer (&self->priv->client_map, g_tree_destroy);
    g_clear_pointer (&self->priv->client_list, client_list_destroy);

    if (self->priv->node) {
        if (self->priv->node_removed_id) {
            g_signal_handler_disconnect (self->priv->node, self->priv->node_removed_id);
            self->priv->node_removed_id = 0;
        }
        g_clear_object (&self->priv->node);
    }

    G_OBJECT_CLASS (qmi_endpoint_qrtr_parent_class)->dispose (object);
}

static void
qmi_endpoint_qrtr_class_init (QmiEndpointQrtrClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    QmiEndpointClass *endpoint_class = QMI_ENDPOINT_CLASS (klass);

    g_type_class_add_private (object_class, sizeof (QmiEndpointQrtrPrivate));

    object_class->dispose = dispose;

    endpoint_class->open = endpoint_open;
    endpoint_class->open_finish = endpoint_open_finish;
    endpoint_class->is_open = endpoint_is_open;
    endpoint_class->send = endpoint_send;
    endpoint_class->close = endpoint_close;
    endpoint_class->close_finish = endpoint_close_finish;
}
