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

#include "qmi-error-types.h"
#include "qmi-device.h"
#include "qmi-ctl.h"
#include "qmi-utils.h"
#include "qmi-proxy.h"

#define BUFFER_SIZE 512
#define QMI_MESSAGE_CTL_INTERNAL_PROXY_OPEN 0xFF00
#define QMI_MESSAGE_CTL_INTERNAL_PROXY_OPEN_INPUT_TLV_DEVICE_PATH 0x01

G_DEFINE_TYPE (QmiProxy, qmi_proxy, G_TYPE_OBJECT)

struct _QmiProxyPrivate {
    /* Unix socket service */
    GSocketService *socket_service;

    /* Clients */
    GList *clients;

    /* Devices */
    GList *devices;
};

/*****************************************************************************/

typedef struct {
    QmiProxy *proxy; /* not full ref */
    GSocketConnection *connection;
    GSource *connection_readable_source;
    GByteArray *buffer;
    QmiDevice *device;
    QmiMessage *internal_proxy_open_request;
} Client;

static gboolean connection_readable_cb (GSocket *socket, GIOCondition condition, Client *client);

static void
client_free (Client *client)
{
    g_source_destroy (client->connection_readable_source);
    g_source_unref (client->connection_readable_source);

    g_output_stream_close (g_io_stream_get_output_stream (G_IO_STREAM (client->connection)), NULL, NULL);

    if (client->buffer)
        g_byte_array_unref (client->buffer);

    if (client->internal_proxy_open_request)
        g_byte_array_unref (client->internal_proxy_open_request);

    g_object_unref (client->connection);
    g_slice_free (Client, client);
}

static void
connection_close (Client *client)
{
    QmiProxy *self = client->proxy;

    client_free (client);
    self->priv->clients = g_list_remove (self->priv->clients, client);
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
device_command_ready (QmiDevice *device,
                      GAsyncResult *res,
                      Client *client)
{
    QmiMessage *response;
    GError *error = NULL;

    response = qmi_device_command_finish (device, res, &error);
    if (!response) {
        g_warning ("sending request to device failed: %s", error->message);
        g_error_free (error);
        return;
    }

    if (!send_message (client, response, &error)) {
        qmi_message_unref (response);
        connection_close (client);
        return;
    }

    qmi_message_unref (response);
}

static gboolean
process_message (Client *client,
                 QmiMessage *message)
{
    /* Accept only request messages from the client */
    if (!qmi_message_is_request (message)) {
        g_debug ("invalid message from client: not a request message");
        return FALSE;
    }

    if (qmi_message_get_service (message) == QMI_SERVICE_CTL) {
        if (qmi_message_get_message_id (message) == QMI_MESSAGE_CTL_INTERNAL_PROXY_OPEN)
            return process_internal_proxy_open (client, message);

        /* CTL, fixup transaction id and keep on */
    }

    /* Non-CTL service, just forward to qmi device */

    qmi_device_command (client->device,
                        message,
                        10, /* for now... */
                        NULL,
                        (GAsyncReadyCallback)device_command_ready,
                        client);
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
    gssize read;

    if (condition & G_IO_HUP || condition & G_IO_ERR) {
        g_debug ("client connection closed");
        connection_close (client);
        return FALSE;
    }

    if (!(condition & G_IO_IN || condition & G_IO_PRI))
        return TRUE;

    read = g_input_stream_read (g_io_stream_get_input_stream (G_IO_STREAM (client->connection)),
                                buffer,
                                BUFFER_SIZE,
                                NULL,
                                &error);
    if (read < 0) {
        g_warning ("Error reading from istream: %s", error ? error->message : "unknown");
        if (error)
            g_error_free (error);
        /* Close the device */
        connection_close (client);
        return FALSE;
    }

    if (read == 0)
        return TRUE;

    /* else, read > 0 */
    if (!G_UNLIKELY (client->buffer))
        client->buffer = g_byte_array_sized_new (read);
    g_byte_array_append (client->buffer, buffer, read);

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

    g_debug ("client connection open...");

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

    /* Keep the client info around */
    self->priv->clients = g_list_append (self->priv->clients, client);
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
    object_class->dispose = dispose;
}
