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
#include "qmi-enum-types.h"
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
    guint     node_removed_id;
    gboolean  node_removed;

    gboolean  endpoint_open;
    GList    *clients;
};

/*****************************************************************************/

static void
add_qmi_message_to_buffer (QmiEndpointQrtr *self,
                           QmiMessage      *message)
{
    g_autoptr(GError)  error = NULL;
    const guint8      *raw_message;
    gsize              raw_message_len;

    raw_message = qmi_message_get_raw (message, &raw_message_len, &error);
    if (!raw_message)
        g_warning ("[%s] Got malformed QMI message: %s",
                   qmi_endpoint_get_name (QMI_ENDPOINT (self)), error->message);
    else
        qmi_endpoint_add_message (QMI_ENDPOINT (self), raw_message, raw_message_len);
    qmi_message_unref (message);
}

/*****************************************************************************/

#define QRTR_CLIENT_DATA_SERVICE "service"
#define QRTR_CLIENT_DATA_CID     "cid"

typedef struct {
    QmiService  service;
    guint       cid;
    QrtrClient *client;
    guint       client_message_id;
} ClientInfo;

static void
client_info_free (ClientInfo *client_info)
{
    g_signal_handler_disconnect (client_info->client, client_info->client_message_id);
    g_object_unref (client_info->client);
    g_slice_free (ClientInfo, client_info);
}

static void
client_info_list_free (GList *list)
{
    g_list_free_full (list, (GDestroyNotify) client_info_free);
}

static ClientInfo *
client_info_lookup (QmiEndpointQrtr *self,
                    QmiService       service,
                    guint            cid)
{
    GList *l;

    for (l = self->priv->clients; l; l = g_list_next (l)) {
        ClientInfo *client_info = l->data;

        if ((service == client_info->service) &&
            (cid == client_info->cid))
            return client_info;
    }
    return NULL;
}

static gint
client_info_cmp (const ClientInfo *a,
                 const ClientInfo *b)
{
    if (a->service != b->service)
        return a->service - b->service;
    return a->cid - b->cid;
}

static void
client_message_cb (QrtrClient      *qrtr_client,
                   GByteArray      *qrtr_message,
                   QmiEndpointQrtr *self)
{
    QmiMessage        *message;
    guint              service;
    guint              cid;
    g_autoptr(GError)  error = NULL;

    service = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (qrtr_client), QRTR_CLIENT_DATA_SERVICE));
    cid     = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (qrtr_client), QRTR_CLIENT_DATA_CID));

    /* Create a fake QMUX header and add this message to the buffer */
    message = qmi_message_new_from_data (service, cid, qrtr_message, &error);
    if (!message)
        g_warning ("[%s] Got malformed QMI message: %s",
                   qmi_endpoint_get_name (QMI_ENDPOINT (self)), error->message);
    else
        add_qmi_message_to_buffer (self, message);
}

static ClientInfo *
client_info_new (QmiEndpointQrtr  *self,
                 QmiService        service,
                 GError          **error)
{
    ClientInfo *client_info;
    QrtrClient *qrtr_client;
    GList      *l;
    guint       max_cid = 0;
    guint       min_available_cid = 1;
    guint       cid = 0;
    gint32      port;

    for (l = self->priv->clients; l; l = g_list_next (l)) {
        client_info = l->data;

        if (service != client_info->service)
            continue;

        max_cid = client_info->cid;
        if (min_available_cid == client_info->cid)
            min_available_cid++;
    }

    cid = max_cid + 1;
    if (cid > G_MAXUINT8) {
        cid = min_available_cid;
        if (min_available_cid > G_MAXUINT8) {
            g_set_error (error, QMI_PROTOCOL_ERROR, QMI_PROTOCOL_ERROR_CLIENT_IDS_EXHAUSTED,
                         "Client IDs have been exhausted");
            return NULL;
        }
    }

    port = qrtr_node_lookup_port (self->priv->node, service);
    if (port < 0) {
        g_set_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_UNSUPPORTED,
                     "Service not supported");
        return NULL;
    }

    qrtr_client = qrtr_client_new (self->priv->node, (guint)port, NULL, error);
    if (!qrtr_client) {
        g_prefix_error (error, "Couldn't create QRTR client: ");
        return NULL;
    }

    /* store service and cid info in the client itself for quicker access */
    g_object_set_data (G_OBJECT (qrtr_client), QRTR_CLIENT_DATA_SERVICE, GUINT_TO_POINTER (service));
    g_object_set_data (G_OBJECT (qrtr_client), QRTR_CLIENT_DATA_CID,     GUINT_TO_POINTER (cid));

    client_info = g_slice_new0 (ClientInfo);
    client_info->service = service;
    client_info->cid = cid;
    client_info->client = qrtr_client;
    client_info->client_message_id = g_signal_connect (qrtr_client,
                                                       QRTR_CLIENT_SIGNAL_MESSAGE,
                                                       G_CALLBACK (client_message_cb),
                                                       self);

    self->priv->clients = g_list_insert_sorted (self->priv->clients,
                                                client_info,
                                                (GCompareFunc)client_info_cmp);

    return client_info;
}

/*****************************************************************************/
/* Client info operations */

static guint
allocate_client (QmiEndpointQrtr  *self,
                 QmiService        service,
                 GError          **error)
{
    ClientInfo *client_info;

    if (!self->priv->endpoint_open) {
        g_set_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_WRONG_STATE,
                     "Endpoint is not open");
        return 0;
    }

    client_info = client_info_new (self, service, error);
    if (!client_info)
        return 0;

    return client_info->cid;
}

static void
release_client (QmiEndpointQrtr *self,
                QmiService       service,
                guint            cid)
{
    ClientInfo *client_info;

    client_info = client_info_lookup (self, service, cid);
    if (!client_info)
        return;

    self->priv->clients = g_list_remove (self->priv->clients, client_info);
    client_info_free (client_info);
}

/*****************************************************************************/

static gboolean
construct_alloc_tlv (QmiMessage *message,
                     guint8      service,
                     guint8      client)
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
handle_alloc_cid (QmiEndpointQrtr *self,
                  QmiMessage      *message)
{
    gsize                 offset = 0;
    gsize                 init_offset;
    guint8                service;
    guint                 cid;
    QmiProtocolError      result = QMI_PROTOCOL_ERROR_NONE;
    g_autoptr(QmiMessage) response = NULL;
    g_autoptr(GError)     error = NULL;

    if (((init_offset = qmi_message_tlv_read_init (message, QMI_MESSAGE_TLV_ALLOCATION_INFO, NULL, &error)) == 0) ||
        !qmi_message_tlv_read_guint8 (message, init_offset, &offset, &service, &error)) {
        g_debug ("[%s] error allocating CID: could not parse message: %s",
                 qmi_endpoint_get_name (QMI_ENDPOINT (self)), error->message);
        result = QMI_PROTOCOL_ERROR_MALFORMED_MESSAGE;
    }

    cid = allocate_client (self, service, &error);
    if (!cid) {
        g_debug ("[%s] error allocating CID: %s",
                 qmi_endpoint_get_name (QMI_ENDPOINT (self)), error->message);
        result = QMI_PROTOCOL_ERROR_INTERNAL;
    }

    response = qmi_message_response_new (message, result);
    if (!response)
        return;

    if ((result == QMI_PROTOCOL_ERROR_NONE) && !construct_alloc_tlv (response, service, cid))
        return;

    add_qmi_message_to_buffer (self, g_steal_pointer (&response));
}

static void
handle_release_cid (QmiEndpointQrtr *self,
                    QmiMessage      *message)
{
    gsize                 offset = 0;
    gsize                 init_offset;
    guint8                service;
    guint8                cid = 0;
    QmiProtocolError      result = QMI_PROTOCOL_ERROR_NONE;
    g_autoptr(QmiMessage) response = NULL;
    g_autoptr(GError)     error = NULL;

    if (((init_offset = qmi_message_tlv_read_init (message, QMI_MESSAGE_TLV_ALLOCATION_INFO, NULL, &error)) == 0) ||
        !qmi_message_tlv_read_guint8 (message, init_offset, &offset, &service, &error) ||
        !qmi_message_tlv_read_guint8 (message, init_offset, &offset, &cid, &error)) {
        g_debug ("[%s] error releasing CID: could not parse message: %s",
                 qmi_endpoint_get_name (QMI_ENDPOINT (self)), error->message);
        result = QMI_PROTOCOL_ERROR_MALFORMED_MESSAGE;
    }

    release_client (self, service, cid);

    response = qmi_message_response_new (message, result);
    if (!response)
        return;

    if ((result == QMI_PROTOCOL_ERROR_NONE) && !construct_alloc_tlv (response, service, cid))
        return;

    add_qmi_message_to_buffer (self, g_steal_pointer (&response));
}

static void
handle_sync (QmiEndpointQrtr *self,
             QmiMessage      *message)
{
    QmiMessage *response;

    response = qmi_message_response_new (message, QMI_PROTOCOL_ERROR_NONE);
    if (!response)
        return;

    add_qmi_message_to_buffer (self, response);
}

static void
unhandled_message (QmiEndpointQrtr *self,
                   QmiMessage      *message)
{
    QmiMessage *response;

    response = qmi_message_response_new (message, QMI_PROTOCOL_ERROR_NOT_SUPPORTED);
    if (!response)
        return;

    add_qmi_message_to_buffer (self, response);
}

static void
handle_ctl_message (QmiEndpointQrtr *self,
                    QmiMessage      *message)
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
        default:
            unhandled_message (self, message);
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
    QmiEndpointQrtr *self = QMI_ENDPOINT_QRTR (endpoint);
    GTask           *task;

    g_assert (!use_proxy);

    task = g_task_new (self, cancellable, callback, user_data);

    if (self->priv->node_removed) {
        g_task_return_new_error (task,
                                 QMI_CORE_ERROR,
                                 QMI_CORE_ERROR_FAILED,
                                 "Node is not present on bus");
        g_object_unref (task);
        return;
    }

    if (self->priv->endpoint_open) {
        g_task_return_new_error (task,
                                 QMI_CORE_ERROR,
                                 QMI_CORE_ERROR_WRONG_STATE,
                                 "Already open");
        g_object_unref (task);
        return;
    }

    g_assert (self->priv->clients == NULL);
    self->priv->endpoint_open = TRUE;

    g_task_return_boolean (task, TRUE);
    g_object_unref (task);
}

static gboolean
endpoint_is_open (QmiEndpoint *self)
{
    return QMI_ENDPOINT_QRTR (self)->priv->endpoint_open;
}

static gboolean
endpoint_send (QmiEndpoint   *endpoint,
               QmiMessage    *message,
               guint          timeout,
               GCancellable  *cancellable,
               GError       **error)
{
    QmiEndpointQrtr       *self = QMI_ENDPOINT_QRTR (endpoint);
    ClientInfo            *client_info;
    QmiService             service;
    guint                  cid;
    gconstpointer          raw_message;
    gsize                  raw_message_len;
    g_autoptr(GByteArray)  qrtr_message = NULL;

    /* We implement the CTL service here, so divert those messages */
    service = qmi_message_get_service (message);
    if (service == QMI_SERVICE_CTL) {
        handle_ctl_message (self, message);
        return TRUE;
    }

    cid = qmi_message_get_client_id (message);
    client_info = client_info_lookup (self, service, cid);
    if (!client_info) {
        g_set_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_WRONG_STATE,
                     "Unknown client %u for service %s", cid, qmi_service_get_string (service));
        return FALSE;
    }

    /* Build raw QRTR message without QMUX header */
    raw_message = qmi_message_get_data (message, &raw_message_len, error);
    if (!raw_message) {
        g_prefix_error (error, "Invalid QMI message: ");
        return FALSE;
    }
    qrtr_message = g_byte_array_sized_new (raw_message_len);
    g_byte_array_append (qrtr_message, raw_message, raw_message_len);

    return qrtr_client_send (client_info->client,
                             qrtr_message,
                             cancellable,
                             error);
}

/*****************************************************************************/

static void
internal_close (QmiEndpointQrtr *self)
{
    g_clear_pointer (&self->priv->clients, client_info_list_free);
    self->priv->endpoint_open = FALSE;
}

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
    QmiEndpointQrtr *self = QMI_ENDPOINT_QRTR (endpoint);
    GTask           *task;

    task = g_task_new (self, cancellable, callback, user_data);

    internal_close (self);

    g_task_return_boolean (task, TRUE);
    g_object_unref (task);
}

/*****************************************************************************/

static void
node_removed_cb (QrtrNode        *node,
                 QmiEndpointQrtr *self)
{
    self->priv->node_removed = TRUE;
    g_signal_emit_by_name (QMI_ENDPOINT (self), QMI_ENDPOINT_SIGNAL_HANGUP);
}

QmiEndpointQrtr *
qmi_endpoint_qrtr_new (QrtrNode *node)
{
    QmiEndpointQrtr    *self;
    g_autofree gchar   *uri = NULL;
    g_autoptr(GFile)    gfile = NULL;
    g_autoptr(QmiFile)  file = NULL;

    if (!node)
        return NULL;

    uri = qrtr_get_uri_for_node (qrtr_node_get_id (node));
    gfile = g_file_new_for_uri (uri);
    file = qmi_file_new (gfile);

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

    internal_close (self);

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
