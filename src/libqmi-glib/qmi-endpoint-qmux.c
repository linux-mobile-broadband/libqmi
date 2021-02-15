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
 * Copyright (C) 2012 Lanedo GmbH
 * Copyright (C) 2012-2017 Aleksander Morgado <aleksander@aleksander.es>
 * Copyright (C) 2019 Eric Caruso <ejcaruso@chromium.org>
 */

#include <errno.h>
#include <fcntl.h>
#include <gio/gio.h>
#include <gio/gunixinputstream.h>
#include <gio/gunixoutputstream.h>
#include <gio/gunixsocketaddress.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "qmi-endpoint-qmux.h"
#include "qmi-ctl.h"
#include "qmi-errors.h"
#include "qmi-error-types.h"

G_DEFINE_TYPE (QmiEndpointQmux, qmi_endpoint_qmux, QMI_TYPE_ENDPOINT)

struct _QmiEndpointQmuxPrivate {
    /* I/O streams */
    gint fd;
    GInputStream *istream;
    GOutputStream *ostream;
    GSource *input_source;

    /* Proxy socket */
    gchar *proxy_path;
    GSocketClient *socket_client;
    GSocketConnection *socket_connection;

    /* Control client */
    QmiClientCtl *client_ctl;
};

#define BUFFER_SIZE 2048
#define MAX_SPAWN_RETRIES 10

static void destroy_iostream (QmiEndpointQmux *self);

/*****************************************************************************/

static gboolean
input_ready_cb (GInputStream *istream,
                QmiEndpointQmux *self)
{
    guint8 buffer[BUFFER_SIZE];
    GError *error = NULL;
    gssize r;

    r = g_pollable_input_stream_read_nonblocking (G_POLLABLE_INPUT_STREAM (istream),
                                                  buffer,
                                                  BUFFER_SIZE,
                                                  NULL,
                                                  &error);
    if (r < 0) {
        g_warning ("Error reading from istream: %s", error ? error->message : "unknown");
        if (error)
            g_error_free (error);
        /* Hang up the endpoint */
        g_signal_emit_by_name (QMI_ENDPOINT (self), QMI_ENDPOINT_SIGNAL_HANGUP);
        return G_SOURCE_REMOVE;
    }

    if (r == 0) {
        /* HUP! */
        g_warning ("Cannot read from istream: connection broken");
        g_signal_emit_by_name (QMI_ENDPOINT (self), QMI_ENDPOINT_SIGNAL_HANGUP);
        return G_SOURCE_REMOVE;
    }

    /* else, r > 0 */
    qmi_endpoint_add_message (QMI_ENDPOINT (self), buffer, r);
    return G_SOURCE_CONTINUE;
}

/*****************************************************************************/

typedef struct {
    gboolean      use_proxy;
    guint         spawn_retries;
} QmuxDeviceOpenContext;

static void
qmux_device_open_context_free (QmuxDeviceOpenContext *ctx)
{
    g_slice_free (QmuxDeviceOpenContext, ctx);
}

static gboolean
endpoint_open_finish (QmiEndpoint   *self,
                      GAsyncResult  *res,
                      GError       **error)
{
    return g_task_propagate_boolean (G_TASK (res), error);
}

static void
internal_proxy_open_ready (QmiClientCtl *client_ctl,
                           GAsyncResult *res,
                           GTask *task)
{
    QmiMessageCtlInternalProxyOpenOutput *output;
    GError *error = NULL;

    /* Check result of the async operation */
    output = qmi_client_ctl_internal_proxy_open_finish (client_ctl, res, &error);
    if (!output) {
        g_task_return_error (task, error);
        g_object_unref (task);
        return;
    }

    /* Check result of the QMI operation */
    if (!qmi_message_ctl_internal_proxy_open_output_get_result (output, &error)) {
        g_task_return_error (task, error);
        g_object_unref (task);
        qmi_message_ctl_internal_proxy_open_output_unref (output);
        return;
    }

    qmi_message_ctl_internal_proxy_open_output_unref (output);
    g_task_return_boolean (task, TRUE);
    g_object_unref (task);
}

static void
setup_proxy (GTask *task)
{
    QmiEndpointQmux *self;
    QmiMessageCtlInternalProxyOpenInput *input;
    QmiFile *file;

    self = g_task_get_source_object (task);

    g_object_get (self, QMI_ENDPOINT_FILE, &file, NULL);
    input = qmi_message_ctl_internal_proxy_open_input_new ();
    qmi_message_ctl_internal_proxy_open_input_set_device_path (input, qmi_file_get_path (file), NULL);
    qmi_client_ctl_internal_proxy_open (self->priv->client_ctl,
                                        input,
                                        5,
                                        g_task_get_cancellable (task),
                                        (GAsyncReadyCallback)internal_proxy_open_ready,
                                        task);
    qmi_message_ctl_internal_proxy_open_input_unref (input);
    g_object_unref (file);
}

static void
setup_iostream (GTask *task)
{
    QmiEndpointQmux *self;
    QmuxDeviceOpenContext *ctx;

    self = g_task_get_source_object (task);
    ctx = g_task_get_task_data (task);

    /* Check in/out streams */
    if (!self->priv->istream || !self->priv->ostream) {
        destroy_iostream (self);
        g_task_return_new_error (task,
                                 QMI_CORE_ERROR,
                                 QMI_CORE_ERROR_FAILED,
                                 "Cannot get input/output streams");
        g_object_unref (task);
        return;
    }

    /* Setup input events */
    self->priv->input_source = g_pollable_input_stream_create_source (
                                   G_POLLABLE_INPUT_STREAM (self->priv->istream),
                                   NULL);
    g_source_set_callback (self->priv->input_source,
                           (GSourceFunc)input_ready_cb,
                           self,
                           NULL);
    g_source_attach (self->priv->input_source, g_main_context_get_thread_default ());

    if (!ctx->use_proxy) {
        /* We're done here */
        g_task_return_boolean (task, TRUE);
        g_object_unref (task);
        return;
    }

    setup_proxy (task);
}

static void
create_iostream_with_fd (GTask *task)
{
    QmiEndpointQmux *self;
    QmiFile *file;
    gint fd;

    self = g_task_get_source_object (task);
    g_object_get (self, QMI_ENDPOINT_FILE, &file, NULL);
    fd = open (qmi_file_get_path (file), O_RDWR | O_EXCL | O_NONBLOCK | O_NOCTTY);

    if (fd < 0) {
        g_task_return_new_error (task,
                                 QMI_CORE_ERROR,
                                 QMI_CORE_ERROR_FAILED,
                                 "Cannot open device file '%s': %s",
                                 qmi_file_get_path_display (file),
                                 strerror (errno));
        g_object_unref (file);
        g_object_unref (task);
        return;
    }

    g_object_unref (file);
    g_assert (self->priv->fd < 0);
    self->priv->fd = fd;
    self->priv->istream = g_unix_input_stream_new  (fd, FALSE);
    self->priv->ostream = g_unix_output_stream_new (fd, FALSE);

    setup_iostream (task);
}

static void create_iostream_with_socket (GTask *task);

static gboolean
wait_for_proxy_cb (GTask *task)
{
    create_iostream_with_socket (task);
    return FALSE;
}

static void
spawn_child_setup (void)
{
    if (setpgid (0, 0) < 0)
        g_warning ("couldn't setup proxy specific process group");
}

static void
create_iostream_with_socket (GTask *task)
{
    QmiEndpointQmux *self;
    QmuxDeviceOpenContext *ctx;
    GSocketAddress *socket_address;
    GError *error = NULL;

    self = g_task_get_source_object (task);
    ctx = g_task_get_task_data (task);

    /* Create socket client */
    self->priv->socket_client = g_socket_client_new ();
    g_socket_client_set_family (self->priv->socket_client, G_SOCKET_FAMILY_UNIX);
    g_socket_client_set_socket_type (self->priv->socket_client, G_SOCKET_TYPE_STREAM);
    g_socket_client_set_protocol (self->priv->socket_client, G_SOCKET_PROTOCOL_DEFAULT);

    /* Setup socket address */
    socket_address = g_unix_socket_address_new_with_type (
                         self->priv->proxy_path,
                         -1,
                         G_UNIX_SOCKET_ADDRESS_ABSTRACT);

    /* Connect to address */
    self->priv->socket_connection = g_socket_client_connect (
                                        self->priv->socket_client,
                                        G_SOCKET_CONNECTABLE (socket_address),
                                        NULL,
                                        &error);
    g_object_unref (socket_address);

    if (!self->priv->socket_connection) {
        gchar **argc;
        GSource *source;

        g_debug ("cannot connect to proxy: %s", error->message);
        g_clear_error (&error);
        g_clear_object (&self->priv->socket_client);

        /* Don't retry forever */
        ctx->spawn_retries++;
        if (ctx->spawn_retries > MAX_SPAWN_RETRIES) {
            g_task_return_new_error (task,
                                     QMI_CORE_ERROR,
                                     QMI_CORE_ERROR_FAILED,
                                     "Couldn't spawn the qmi-proxy");
            g_object_unref (task);
            return;
        }

        g_debug ("spawning new qmi-proxy (try %u)...", ctx->spawn_retries);

        argc = g_new0 (gchar *, 2);
        argc[0] = g_strdup (LIBEXEC_PATH "/qmi-proxy");
        if (!g_spawn_async (NULL, /* working directory */
                            argc,
                            NULL, /* envp */
                            G_SPAWN_STDOUT_TO_DEV_NULL | G_SPAWN_STDERR_TO_DEV_NULL,
                            (GSpawnChildSetupFunc) spawn_child_setup,
                            NULL, /* child_setup_user_data */
                            NULL,
                            &error)) {
            g_debug ("error spawning qmi-proxy: %s", error->message);
            g_clear_error (&error);
        }
        g_strfreev (argc);

        /* Wait some ms and retry */
        source = g_timeout_source_new (100);
        g_source_set_callback (source, (GSourceFunc)wait_for_proxy_cb, task, NULL);
        g_source_attach (source, g_main_context_get_thread_default ());
        g_source_unref (source);
        return;
    }

    self->priv->istream = g_io_stream_get_input_stream (G_IO_STREAM (self->priv->socket_connection));
    if (self->priv->istream)
        g_object_ref (self->priv->istream);

    self->priv->ostream = g_io_stream_get_output_stream (G_IO_STREAM (self->priv->socket_connection));
    if (self->priv->ostream)
        g_object_ref (self->priv->ostream);

    setup_iostream (task);
}

static void
endpoint_open (QmiEndpoint         *endpoint,
               gboolean             use_proxy,
               guint                timeout,
               GCancellable        *cancellable,
               GAsyncReadyCallback  callback,
               gpointer             user_data)
{
    QmiEndpointQmux *self;
    QmuxDeviceOpenContext *ctx;
    GTask *task;

    ctx = g_slice_new (QmuxDeviceOpenContext);
    ctx->use_proxy = use_proxy;
    ctx->spawn_retries = 0;

    self = QMI_ENDPOINT_QMUX (endpoint);
    task = g_task_new (self, NULL, callback, user_data);
    g_task_set_task_data (task,
                          ctx,
                          (GDestroyNotify)qmux_device_open_context_free);

    if (self->priv->istream || self->priv->ostream) {
        g_task_return_new_error (task,
                                 QMI_CORE_ERROR,
                                 QMI_CORE_ERROR_WRONG_STATE,
                                 "Already open");
        g_object_unref (task);
        return;
    }

    if (use_proxy)
        create_iostream_with_socket (task);
    else
        create_iostream_with_fd (task);
}

/*****************************************************************************/

static gboolean
endpoint_is_open (QmiEndpoint *self)
{
    return !!(QMI_ENDPOINT_QMUX (self)->priv->istream ||
              QMI_ENDPOINT_QMUX (self)->priv->ostream);
}

static gboolean
endpoint_send (QmiEndpoint   *self,
               QmiMessage    *message,
               guint          timeout,
               GCancellable  *cancellable,
               GError       **error)
{
    gconstpointer raw_message;
    gsize raw_message_len;
    GError *inner_error = NULL;

    /* Get raw message */
    raw_message = qmi_message_get_raw (message, &raw_message_len, &inner_error);
    if (!raw_message) {
        g_propagate_prefixed_error (error, inner_error, "Cannot get raw message: ");
        return FALSE;
    }

    if (!g_output_stream_write_all (QMI_ENDPOINT_QMUX (self)->priv->ostream,
                                    raw_message,
                                    raw_message_len,
                                    NULL, /* bytes_written */
                                    NULL, /* cancellable */
                                    &inner_error)) {
        g_propagate_prefixed_error (error, inner_error, "Cannot write message: ");
        return FALSE;
    }

    /* Flush explicitly if correctly written */
    g_output_stream_flush (QMI_ENDPOINT_QMUX (self)->priv->ostream, NULL, NULL);
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
destroy_iostream (QmiEndpointQmux *self)
{
    if (self->priv->input_source) {
        g_source_destroy (self->priv->input_source);
        g_clear_pointer (&self->priv->input_source, g_source_unref);
    }
    g_clear_object (&self->priv->istream);
    g_clear_object (&self->priv->ostream);
    g_clear_object (&self->priv->socket_connection);
    g_clear_object (&self->priv->socket_client);
    if (self->priv->fd >= 0) {
        close (self->priv->fd);
        self->priv->fd = -1;
    }
}

static void
endpoint_close (QmiEndpoint         *endpoint,
                guint                timeout,
                GCancellable        *cancellable,
                GAsyncReadyCallback  callback,
                gpointer             user_data)
{
    QmiEndpointQmux *self;
    GTask *task;

    self = QMI_ENDPOINT_QMUX (endpoint);
    task = g_task_new (self, cancellable, callback, user_data);

    destroy_iostream (self);

    g_task_return_boolean (task, TRUE);
    g_object_unref (task);
}

/*****************************************************************************/

QmiEndpointQmux *
qmi_endpoint_qmux_new (QmiFile      *file,
                       const gchar  *proxy_path,
                       QmiClientCtl *client_ctl)
{
    QmiEndpointQmux *self;

    if (!file)
        return NULL;

    self = g_object_new (QMI_TYPE_ENDPOINT_QMUX,
                         QMI_ENDPOINT_FILE, file,
                         NULL);
    self->priv->proxy_path = g_strdup (proxy_path);
    self->priv->client_ctl = g_object_ref (client_ctl);
    return self;
}

/*****************************************************************************/

static void
qmi_endpoint_qmux_init (QmiEndpointQmux *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE ((self),
                                              QMI_TYPE_ENDPOINT_QMUX,
                                              QmiEndpointQmuxPrivate);
    self->priv->fd = -1;
}

static void
dispose (GObject *object)
{
    QmiEndpointQmux *self = QMI_ENDPOINT_QMUX (object);

    destroy_iostream (self);
    g_clear_object (&self->priv->client_ctl);
    g_clear_pointer (&self->priv->proxy_path, g_free);

    G_OBJECT_CLASS (qmi_endpoint_qmux_parent_class)->dispose (object);
}

static void
qmi_endpoint_qmux_class_init (QmiEndpointQmuxClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    QmiEndpointClass *endpoint_class = QMI_ENDPOINT_CLASS (klass);

    g_type_class_add_private (object_class, sizeof (QmiEndpointQmuxPrivate));

    object_class->dispose = dispose;

    endpoint_class->open = endpoint_open;
    endpoint_class->open_finish = endpoint_open_finish;
    endpoint_class->is_open = endpoint_is_open;
    endpoint_class->send = endpoint_send;
    endpoint_class->close = endpoint_close;
    endpoint_class->close_finish = endpoint_close_finish;
}
