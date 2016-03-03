/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details:
 *
 * Copyright (C) 2014 Aleksander Morgado <aleksander@aleksander.es>
 *
 * Higly based on the test-port-context setup in ModemManager.
 */

#include <config.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <gio/gio.h>
#include <gio/gunixsocketaddress.h>
#include <libqmi-glib.h>

#include "test-port-context.h"

#define BUFFER_SIZE 1024

struct _TestPortContext {
    gchar *name;
    GThread *thread;
    gboolean ready;
    GCond ready_cond;
    GMutex ready_mutex;
    GMainLoop *loop;
    GSocketService *socket_service;
    GList *clients;
    GMutex command_mutex;
    GByteArray *command;
    GByteArray *response;
};

/*****************************************************************************/
/* Helpers */

static gchar *
str_hex (gconstpointer mem,
         gsize size,
         gchar delimiter)
{
    const guint8 *data = mem;
    gsize i;
    gsize j;
    gsize new_str_length;
    gchar *new_str;

    /* Get new string length. If input string has N bytes, we need:
     * - 1 byte for last NUL char
     * - 2N bytes for hexadecimal char representation of each byte...
     * - N-1 bytes for the separator ':'
     * So... a total of (1+2N+N-1) = 3N bytes are needed... */
    new_str_length =  3 * size;

    /* Allocate memory for new array and initialize contents to NUL */
    new_str = g_malloc0 (new_str_length);

    /* Print hexadecimal representation of each byte... */
    for (i = 0, j = 0; i < size; i++, j += 3) {
        /* Print character in output string... */
        snprintf (&new_str[j], 3, "%02X", data[i]);
        /* And if needed, add separator */
        if (i != (size - 1) )
            new_str[j + 2] = delimiter;
    }

    /* Set output string */
    return new_str;
}

/*****************************************************************************/

void
test_port_context_set_command (TestPortContext *ctx,
                               const guint8    *command,
                               gsize            command_size,
                               const guint8    *response,
                               gsize            response_size,
                               guint16          transaction_id)
{
    g_mutex_lock (&ctx->command_mutex);
    {
        g_assert (!ctx->command);
        ctx->command = g_byte_array_append (g_byte_array_sized_new (command_size), command, command_size);
        qmi_message_set_transaction_id ((QmiMessage *)ctx->command, transaction_id);

        g_assert (!ctx->response);
        ctx->response = g_byte_array_append (g_byte_array_sized_new (response_size), response, response_size);
        qmi_message_set_transaction_id ((QmiMessage *)ctx->response, transaction_id);
    }
    g_mutex_unlock (&ctx->command_mutex);
}

static GByteArray *
process_next_command (TestPortContext *ctx,
                      GByteArray      *buffer)
{
    QmiMessage   *message;
    GError       *error = NULL;
    const guint8 *message_raw;
    gsize         message_raw_length;
    gchar        *expected;
    gchar        *received;
    GByteArray   *response;

    /* Every message received must start with the QMUX marker.
     * If it doesn't, we broke framing :-/
     * If we broke framing, an error should be reported and the device
     * should get closed */
    if (buffer->len > 0 && buffer->data[0] != QMI_MESSAGE_QMUX_MARKER)
        g_assert_not_reached ();

    message = qmi_message_new_from_raw (buffer, &error);
    if (!message) {
        if (!error)
            /* More data we need */
            return NULL;
        /* Fail */
        g_assert_no_error (error);
    }

    /* Process received message */
    message_raw = qmi_message_get_raw (message, &message_raw_length, &error);
    g_assert_no_error (error);
    g_assert (message_raw);

    /* Get printables to compare (we'll just get a nicer error if they are
     * different), compared to a simple memcmp(). */
    g_mutex_lock (&ctx->command_mutex);
    {
        g_assert (ctx->command);
        expected = str_hex (ctx->command->data, ctx->command->len, ':');
    }
    g_mutex_unlock (&ctx->command_mutex);

    received = str_hex (message_raw, message_raw_length, ':');
    g_assert_cmpstr (expected, ==, received);
    g_free (expected);
    g_free (received);
    qmi_message_unref (message);
    g_byte_array_unref (ctx->command);
    ctx->command = NULL;

    /* Command Expected == Received, so now return the Response */
    g_mutex_lock (&ctx->command_mutex);
    {
        response = ctx->response;
        ctx->response = NULL;
    }
    g_mutex_unlock (&ctx->command_mutex);

    return response;
}

/*****************************************************************************/

typedef struct {
    TestPortContext *ctx;
    GSocketConnection *connection;
    GSource *connection_readable_source;
    GByteArray *buffer;
} Client;

static void
client_free (Client *client)
{
    g_source_destroy (client->connection_readable_source);
    g_source_unref (client->connection_readable_source);
    g_output_stream_close (g_io_stream_get_output_stream (G_IO_STREAM (client->connection)), NULL, NULL);
    if (client->buffer)
        g_byte_array_unref (client->buffer);
    g_object_unref (client->connection);
    g_slice_free (Client, client);
}

static void
connection_close (Client *client)
{
    client->ctx->clients = g_list_remove (client->ctx->clients, client);
    client_free (client);
}

static void
client_parse_request (Client *client)
{
    GByteArray *response;

    do {
        response = process_next_command (client->ctx, client->buffer);
        if (response) {
            GError *error = NULL;

            if (!g_output_stream_write_all (g_io_stream_get_output_stream (G_IO_STREAM (client->connection)),
                                            response->data,
                                            response->len,
                                            NULL, /* bytes_written */
                                            NULL, /* cancellable */
                                            &error)) {
                g_warning ("Cannot send response to client: %s", error->message);
                g_error_free (error);
            }
            g_byte_array_unref (response);
        }
    } while (response);
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
    client_parse_request (client);

    return TRUE;
}

static Client *
client_new (TestPortContext *ctx,
            GSocketConnection *connection)
{
    Client *client;

    client = g_slice_new0 (Client);
    client->ctx = ctx;
    client->connection = g_object_ref (connection);
    client->connection_readable_source = g_socket_create_source (g_socket_connection_get_socket (client->connection),
                                                                 G_IO_IN | G_IO_PRI | G_IO_ERR | G_IO_HUP,
                                                                 NULL);
    g_source_set_callback (client->connection_readable_source,
                           (GSourceFunc)connection_readable_cb,
                           client,
                           NULL);
    g_source_attach (client->connection_readable_source, g_main_context_get_thread_default ());

    return client;
}

/* /\*****************************************************************************\/ */

static void
incoming_cb (GSocketService *service,
             GSocketConnection *connection,
             GObject *unused,
             TestPortContext *ctx)
{
    Client *client;

    client = client_new (ctx, connection);
    ctx->clients = g_list_append (ctx->clients, client);
}

static void
create_socket_service (TestPortContext *ctx)
{
    GError *error = NULL;
    GSocketService *service;
    GSocketAddress *address;
    GSocket *socket;

    g_assert (ctx->socket_service == NULL);

    /* Create socket */
    socket = g_socket_new (G_SOCKET_FAMILY_UNIX,
                           G_SOCKET_TYPE_STREAM,
                           G_SOCKET_PROTOCOL_DEFAULT,
                           &error);
    if (!socket)
        g_error ("Cannot create socket: %s", error->message);

    /* Bind to address */
    address = (g_unix_socket_address_new_with_type (
                   ctx->name,
                   -1,
                   G_UNIX_SOCKET_ADDRESS_ABSTRACT));
    if (!g_socket_bind (socket, address, TRUE, &error))
        g_error ("Cannot bind socket '%s': %s", ctx->name, error->message);

    /* Listen */
    if (!g_socket_listen (socket, &error))
        g_error ("Cannot listen in socket: %s", error->message);

    /* Create socket service */
    service = g_socket_service_new ();
    g_signal_connect (service, "incoming", G_CALLBACK (incoming_cb), ctx);
    if (!g_socket_listener_add_socket (G_SOCKET_LISTENER (service),
                                       socket,
                                       NULL, /* don't pass an object, will take a reference */
                                       &error))
        g_error ("Cannot add listener to socket: %s", error->message);

    /* Start it */
    g_socket_service_start (service);

    /* And store it */
    ctx->socket_service = service;

    /* Signal that the thread is ready */
    g_mutex_lock (&ctx->ready_mutex);
    ctx->ready = TRUE;
    g_cond_signal (&ctx->ready_cond);
    g_mutex_unlock (&ctx->ready_mutex);

    if (socket)
        g_object_unref (socket);
    if (address)
        g_object_unref (address);
}

/*****************************************************************************/

void
test_port_context_stop (TestPortContext *ctx)
{
    g_assert (ctx->thread != NULL);
    g_assert (ctx->loop != NULL);

    g_main_loop_quit (ctx->loop);

    g_thread_join (ctx->thread);
    ctx->thread = NULL;
}

static gpointer
port_context_thread_func (TestPortContext *ctx)
{
    GMainContext *thread_context;

    thread_context = g_main_context_new ();
    g_main_context_push_thread_default (thread_context);

    create_socket_service (ctx);

    g_assert (ctx->loop == NULL);
    ctx->loop = g_main_loop_new (g_main_context_get_thread_default (), FALSE);
    g_main_loop_run (ctx->loop);
    g_main_loop_unref (ctx->loop);
    ctx->loop = NULL;
    return NULL;
}

void
test_port_context_start (TestPortContext *ctx)
{
    g_assert (ctx->thread == NULL);
    ctx->thread = g_thread_new (ctx->name,
                                (GThreadFunc)port_context_thread_func,
                                ctx);

    /* Now wait until the thread has finished its initialization and is
     * ready to serve connections */
    g_mutex_lock (&ctx->ready_mutex);
    while (!ctx->ready)
        g_cond_wait (&ctx->ready_cond, &ctx->ready_mutex);
    g_mutex_unlock (&ctx->ready_mutex);
}

/*****************************************************************************/

void
test_port_context_free (TestPortContext *ctx)
{
    g_assert (ctx->thread == NULL);
    g_assert (ctx->loop == NULL);

    g_cond_clear (&ctx->ready_cond);
    g_mutex_clear (&ctx->ready_mutex);
    g_mutex_clear (&ctx->command_mutex);

    g_list_free_full (ctx->clients, (GDestroyNotify)client_free);
    if (ctx->socket_service) {
        if (g_socket_service_is_active (ctx->socket_service))
            g_socket_service_stop (ctx->socket_service);
        g_object_unref (ctx->socket_service);
    }
    g_free (ctx->name);
    if (ctx->command)
        g_byte_array_unref (ctx->command);
    if (ctx->response)
        g_byte_array_unref (ctx->response);
    g_slice_free (TestPortContext, ctx);
}

TestPortContext *
test_port_context_new (const gchar *name)
{
    TestPortContext *ctx;

    ctx = g_slice_new0 (TestPortContext);
    ctx->name = g_strdup (name);
    g_cond_init (&ctx->ready_cond);
    g_mutex_init (&ctx->ready_mutex);
    g_mutex_init (&ctx->command_mutex);
    return ctx;
}
