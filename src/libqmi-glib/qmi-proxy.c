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
 * Copyright (C) 2013 Aleksander Morgado <aleksander@lanedo.com>
 */

#include <string.h>
#include <ctype.h>
#include <sys/file.h>
#include <errno.h>

#include <glib.h>
#include <glib/gstdio.h>
#include <gio/gunixsocketaddress.h>

#include "qmi-enum-types.h"
#include "qmi-error-types.h"
#include "qmi-device.h"
#include "qmi-ctl.h"
#include "qmi-utils.h"
#include "qmi-proxy.h"

#define BUFFER_SIZE 512

#define QMI_MESSAGE_OUTPUT_TLV_RESULT 0x02
#define QMI_MESSAGE_OUTPUT_TLV_ALLOCATION_INFO 0x01
#define QMI_MESSAGE_CTL_ALLOCATE_CID 0x0022
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

    /* Clients */
    GList *clients;

    /* Devices */
    GList *devices;
};

/*****************************************************************************/

/**
 * qmi_proxy_get_n_clients:
 * @self: a #QmiProxy.
 *
 * Get the number of clients currently connected to the proxy.
 *
 * Returns: a #guint.
 */
guint
qmi_proxy_get_n_clients (QmiProxy *self)
{
    g_return_val_if_fail (QMI_IS_PROXY (self), 0);

    return g_list_length (self->priv->clients);
}

/*****************************************************************************/

typedef struct {
    QmiService service;
    guint8 cid;
} QmiClientInfo;

typedef struct {
    QmiProxy *proxy; /* not full ref */
    GSocketConnection *connection;
    GSource *connection_readable_source;
    GByteArray *buffer;
    QmiDevice *device;
    QmiMessage *internal_proxy_open_request;
    GArray *qmi_client_info_array;
    guint indication_id;
} Client;

static gboolean connection_readable_cb (GSocket *socket, GIOCondition condition, Client *client);

static void
client_free (Client *client)
{
    g_source_destroy (client->connection_readable_source);
    g_source_unref (client->connection_readable_source);

    if (client->device) {
        if (g_signal_handler_is_connected (client->device, client->indication_id))
            g_signal_handler_disconnect (client->device, client->indication_id);
        g_object_unref (client->device);
    }

    g_output_stream_close (g_io_stream_get_output_stream (G_IO_STREAM (client->connection)), NULL, NULL);

    if (client->buffer)
        g_byte_array_unref (client->buffer);

    if (client->internal_proxy_open_request)
        g_byte_array_unref (client->internal_proxy_open_request);

    g_array_unref (client->qmi_client_info_array);

    g_object_unref (client->connection);
    g_slice_free (Client, client);
}

static guint
get_n_clients_with_device (QmiProxy *self,
                           QmiDevice *device)
{
    GList *l;
    guint n = 0;

    for (l = self->priv->clients; l; l = g_list_next (l)) {
        Client *client = l->data;

        if (device == client->device ||
            g_str_equal (qmi_device_get_path (device), qmi_device_get_path (client->device)))
            n++;
    }

    return n;
}

static void
connection_close (Client *client)
{
    QmiProxy *self = client->proxy;
    QmiDevice *device;

    device = client->device ? g_object_ref (client->device) : NULL;
    client_free (client);
    self->priv->clients = g_list_remove (self->priv->clients, client);
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_N_CLIENTS]);

    if (!device)
        return;

    /* If no more clients using the device, close and cleanup */
    if (get_n_clients_with_device (self, device) == 0) {
        GList *l;

        for (l = self->priv->devices; l; l = g_list_next (l)) {
            QmiDevice *device_in_list = QMI_DEVICE (l->data);

            if (device == device_in_list ||
                g_str_equal (qmi_device_get_path (device), qmi_device_get_path (device_in_list))) {
                g_debug ("closing device '%s': no longer used", qmi_device_get_path_display (device));
                qmi_device_close (device_in_list, NULL);
                g_object_unref (device_in_list);
                self->priv->devices = g_list_remove (self->priv->devices, device_in_list);
                break;
            }
        }
    }

    g_object_unref (device);
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

static gboolean
send_message (Client *client,
              QmiMessage *message,
              GError **error)
{
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

static void
complete_internal_proxy_open (Client *client)
{
    QmiMessage *response;
    GError *error = NULL;

    g_debug ("connection to QMI device '%s' established", qmi_device_get_path (client->device));

    g_assert (client->internal_proxy_open_request != NULL);
    response = qmi_message_response_new (client->internal_proxy_open_request, QMI_PROTOCOL_ERROR_NONE);

    if (!send_message (client, response, &error)) {
        qmi_message_unref (response);
        connection_close (client);
        return;
    }

    qmi_message_unref (response);
    qmi_message_unref (client->internal_proxy_open_request);
    client->internal_proxy_open_request = NULL;
}

static void
indication_cb (QmiDevice *device,
               QmiMessage *message,
               Client *client)
{
    guint i;

    for (i = 0; i < client->qmi_client_info_array->len; i++) {
        QmiClientInfo *info;

        info = &g_array_index (client->qmi_client_info_array, QmiClientInfo, i);
        /* If service and CID match; or if service and broadcast, forward to
         * the remote client */
        if ((qmi_message_get_service (message) == info->service) &&
            (qmi_message_get_client_id (message) == info->cid ||
             qmi_message_get_client_id (message) == QMI_CID_BROADCAST)) {
            GError *error = NULL;

            if (!send_message (client, message, &error)) {
                g_warning ("couldn't forward indication to client");
                g_error_free (error);
            }

            /* Avoid forwarding broadcast messages multiple times */
            break;
        }
    }
}

static void
device_open_ready (QmiDevice *device,
                   GAsyncResult *res,
                   Client *client)
{
    QmiProxy *self = client->proxy;
    QmiDevice *existing;
    GError *error = NULL;

    if (!qmi_device_open_finish (device, res, &error)) {
        g_debug ("couldn't open QMI device: %s", error->message);
        g_debug ("client connection closed");
        connection_close (client);
        g_error_free (error);
        return;
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

    /* Register for device indications */
    client->indication_id = g_signal_connect (client->device,
                                              "indication",
                                              G_CALLBACK (indication_cb),
                                              client);

    complete_internal_proxy_open (client);
}

static void
device_new_ready (GObject *source,
                  GAsyncResult *res,
                  Client *client)
{
    GError *error = NULL;

    client->device = qmi_device_new_finish (res, &error);
    if (!client->device) {
        g_debug ("couldn't open QMI device: %s", error->message);
        g_debug ("client connection closed");
        connection_close (client);
        g_error_free (error);
        return;
    }

    qmi_device_open (client->device,
                     QMI_DEVICE_OPEN_FLAGS_NONE,
                     10,
                     NULL,
                     (GAsyncReadyCallback)device_open_ready,
                     client);
}

static gboolean
process_internal_proxy_open (Client *client,
                             QmiMessage *message)
{
    QmiProxy *self = client->proxy;
    const guint8 *buffer;
    guint16 buffer_len;
    gchar *device_file_path;

    buffer = qmi_message_get_raw_tlv (message,
                                      QMI_MESSAGE_CTL_INTERNAL_PROXY_OPEN_INPUT_TLV_DEVICE_PATH,
                                      &buffer_len);
    if (!buffer) {
        g_debug ("ignoring message from client: invalid proxy open request");
        return FALSE;
    }

    qmi_utils_read_string_from_buffer (&buffer, &buffer_len, 0, 0, &device_file_path);
    g_debug ("valid request to open connection to QMI device file: %s", device_file_path);


    /* Keep it */
    client->internal_proxy_open_request = qmi_message_ref (message);

    client->device = find_device_for_path (self, device_file_path);

    /* Need to create a device ourselves */
    if (!client->device) {
        GFile *file;

        file = g_file_new_for_path (device_file_path);
        qmi_device_new (file,
                        NULL,
                        (GAsyncReadyCallback)device_new_ready,
                        client);
        g_object_unref (file);
        g_free (device_file_path);
        return TRUE;
    }

    g_free (device_file_path);

    /* Keep a reference to the device in the client */
    g_object_ref (client->device);

    complete_internal_proxy_open (client);
    return FALSE;
}

static void
track_cid (Client *client,
           gboolean track,
           QmiMessage *message)
{
    const guint8 *buffer;
    guint16 buffer_len;
    guint16 error_status;
    guint16 error_code;
    guint8 tmp;
    QmiClientInfo info;
    guint i;
    gboolean exists;

    buffer = qmi_message_get_raw_tlv (message, QMI_MESSAGE_OUTPUT_TLV_RESULT, &buffer_len);
    if (!buffer || buffer_len != 4) {
        g_warning ("invalid 'CTL allocate CID' response: missing or invalid result TLV");
        return;
    }

    qmi_utils_read_guint16_from_buffer (&buffer, &buffer_len, QMI_ENDIAN_LITTLE, &error_status);
    if (error_status != 0x00)
        return;

    qmi_utils_read_guint16_from_buffer (&buffer, &buffer_len, QMI_ENDIAN_LITTLE, &error_code);
    if (error_code != QMI_PROTOCOL_ERROR_NONE)
        return;

    buffer = qmi_message_get_raw_tlv (message, QMI_MESSAGE_OUTPUT_TLV_ALLOCATION_INFO, &buffer_len);
    if (!buffer || buffer_len != 2) {
        g_warning ("invalid 'CTL allocate CID' response: missing or invalid allocation info TLV");
        return;
    }

    qmi_utils_read_guint8_from_buffer (&buffer, &buffer_len, &tmp);
    info.service = (QmiService)tmp;
    qmi_utils_read_guint8_from_buffer (&buffer, &buffer_len, &(info.cid));


    /* Check if it already exists */
    for (i = 0; i < client->qmi_client_info_array->len; i++) {
        QmiClientInfo *existing;

        existing = &g_array_index (client->qmi_client_info_array, QmiClientInfo, i);
        if (existing->service == info.service && existing->cid == info.cid)
            break;
    }
    exists = (i < client->qmi_client_info_array->len);

    if (track && !exists) {
        g_debug ("QMI client tracked [%s,%s,%u]",
                 qmi_device_get_path_display (client->device),
                 qmi_service_get_string (info.service),
                 info.cid);
        g_array_append_val (client->qmi_client_info_array, info);
    } else if (!track && exists) {
        g_debug ("QMI client untracked [%s,%s,%u]",
                 qmi_device_get_path_display (client->device),
                 qmi_service_get_string (info.service),
                 info.cid);
        g_array_remove_index (client->qmi_client_info_array, i);
    }
}

typedef struct {
    Client *client;
    guint8 in_trid;
} Request;

static void
device_command_ready (QmiDevice *device,
                      GAsyncResult *res,
                      Request *request)
{
    QmiMessage *response;
    GError *error = NULL;

    response = qmi_device_command_finish (device, res, &error);
    if (!response) {
        g_warning ("sending request to device failed: %s", error->message);
        g_error_free (error);
        return;
    }

    if (qmi_message_get_service (response) == QMI_SERVICE_CTL) {
        qmi_message_set_transaction_id (response, request->in_trid);
        if (qmi_message_get_message_id (response) == QMI_MESSAGE_CTL_ALLOCATE_CID)
            track_cid (request->client, TRUE, response);
        else if (qmi_message_get_message_id (response) == QMI_MESSAGE_CTL_RELEASE_CID)
            track_cid (request->client, FALSE, response);
    }

    if (!send_message (request->client, response, &error))
        connection_close (request->client);

    qmi_message_unref (response);
    g_slice_free (Request, request);
}

static gboolean
process_message (Client *client,
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
        return process_internal_proxy_open (client, message);

    request = g_slice_new0 (Request);
    request->client = client;
    if (qmi_message_get_service (message) == QMI_SERVICE_CTL) {
        request->in_trid = qmi_message_get_transaction_id (message);
        qmi_message_set_transaction_id (message, 0);
    }

    qmi_device_command (client->device,
                        message,
                        10, /* for now... */
                        NULL,
                        (GAsyncReadyCallback)device_command_ready,
                        request);
    return TRUE;
}

static void
parse_request (Client *client)
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
            process_message (client, message);
            qmi_message_unref (message);
        }
    } while (client->buffer->len > 0);
}

static gboolean
connection_readable_cb (GSocket *socket,
                        GIOCondition condition,
                        Client *client)
{
    guint8 buffer[BUFFER_SIZE];
    GError *error = NULL;
    gssize r;

    if (condition & G_IO_HUP || condition & G_IO_ERR) {
        g_debug ("client connection closed");
        connection_close (client);
        return FALSE;
    }

    if (!(condition & G_IO_IN || condition & G_IO_PRI))
        return TRUE;

    r = g_input_stream_read (g_io_stream_get_input_stream (G_IO_STREAM (client->connection)),
                             buffer,
                             BUFFER_SIZE,
                             NULL,
                             &error);
    if (r < 0) {
        g_warning ("Error reading from istream: %s", error ? error->message : "unknown");
        if (error)
            g_error_free (error);
        /* Close the device */
        connection_close (client);
        return FALSE;
    }

    if (r == 0)
        return TRUE;

    /* else, r > 0 */
    if (!G_UNLIKELY (client->buffer))
        client->buffer = g_byte_array_sized_new (r);
    g_byte_array_append (client->buffer, buffer, r);

    /* Try to parse input messages */
    parse_request (client);

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

    g_debug ("client connection open...");

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

    if (uid != 0) {
        g_warning ("Client not allowed: Not enough privileges");
        return;
    }

    /* Create client */
    client = g_slice_new0 (Client);
    client->proxy = self;
    client->connection = g_object_ref (connection);
    client->connection_readable_source = g_socket_create_source (g_socket_connection_get_socket (client->connection),
                                                                 G_IO_IN | G_IO_PRI | G_IO_ERR | G_IO_HUP,
                                                                 NULL);
    g_source_set_callback (client->connection_readable_source,
                           (GSourceFunc)connection_readable_cb,
                           client,
                           NULL);
    g_source_attach (client->connection_readable_source, NULL);
    client->qmi_client_info_array = g_array_sized_new (FALSE, FALSE, sizeof (QmiClientInfo), 8);

    /* Keep the client info around */
    self->priv->clients = g_list_append (self->priv->clients, client);
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_N_CLIENTS]);
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

    /* Only root can run the qmi-proxy */
    if (getuid () != 0) {
        g_set_error (error,
                     QMI_CORE_ERROR,
                     QMI_CORE_ERROR_FAILED,
                     "Not enough privileges");
        return NULL;
    }

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

    if (priv->clients) {
        g_list_free_full (priv->clients, (GDestroyNotify) client_free);
        priv->clients = NULL;
    }

    if (priv->socket_service) {
        if (g_socket_service_is_active (priv->socket_service))
            g_socket_service_stop (priv->socket_service);
        g_clear_object (&priv->socket_service);
        g_unlink (QMI_PROXY_SOCKET_PATH);
        g_debug ("UNIX socket service at '%s' stopped", QMI_PROXY_SOCKET_PATH);
    }

    G_OBJECT_CLASS (qmi_proxy_parent_class)->dispose (object);
}

static void
qmi_proxy_class_init (QmiProxyClass *proxy_class)
{
    GObjectClass *object_class = G_OBJECT_CLASS (proxy_class);

    g_type_class_add_private (object_class, sizeof (QmiProxyPrivate));

    /* Virtual methods */
    object_class->get_property = get_property;
    object_class->dispose = dispose;

    /* Properties */
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
