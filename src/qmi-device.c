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
 * Copyright (C) 2012 Aleksander Morgado <aleksander@lanedo.com>
 */

#include <gio/gio.h>

#include "qmi-device.h"
#include "qmi-message.h"
#include "qmi-client-ctl.h"
#include "qmi-utils.h"
#include "qmi-error-types.h"
#include "qmi-enum-types.h"

static void async_initable_iface_init (GAsyncInitableIface *iface);

G_DEFINE_TYPE_EXTENDED (QmiDevice, qmi_device, G_TYPE_OBJECT, 0,
                        G_IMPLEMENT_INTERFACE (G_TYPE_ASYNC_INITABLE, async_initable_iface_init));

enum {
    PROP_0,
    PROP_FILE,
    PROP_CLIENT_CTL,
    PROP_LAST
};

static GParamSpec *properties[PROP_LAST];

struct _QmiDevicePrivate {
    /* File */
    GFile *file;
    gchar *path;
    gchar *path_display;

    /* Implicit CTL client */
    QmiClientCtl *client_ctl;

    /* I/O channel, set when the file is open */
    GIOChannel *iochannel;
    guint watch_id;
    GByteArray *response;

    /* HT to keep track of ongoing transactions */
    GHashTable *transactions;
};

#define BUFFER_SIZE 2048

/*****************************************************************************/
/* Message transactions (private) */

typedef struct {
    QmiMessage *message;
    GSimpleAsyncResult *result;
    guint timeout_id;
} Transaction;

static Transaction *
transaction_new (QmiDevice *self,
                 QmiMessage *message,
                 GAsyncReadyCallback callback,
                 gpointer user_data)
{
    Transaction *tr;

    tr = g_slice_new0 (Transaction);
    tr->message = qmi_message_ref (message);
    tr->result = g_simple_async_result_new (G_OBJECT (self),
                                            callback,
                                            user_data,
                                            transaction_new);

    return tr;
}

static void
transaction_complete_and_free (Transaction *tr,
                               QmiMessage *reply,
                               const GError *error)
{
    g_assert (reply != NULL || error != NULL);

    if (tr->timeout_id)
        g_source_remove (tr->timeout_id);

    if (reply)
        g_simple_async_result_set_op_res_gpointer (tr->result,
                                                   qmi_message_ref (reply),
                                                   (GDestroyNotify)qmi_message_unref);
    else
        g_simple_async_result_set_from_error (tr->result, error);

    g_simple_async_result_complete_in_idle (tr->result);
    g_object_unref (tr->result);
    qmi_message_unref (tr->message);
    g_slice_free (Transaction, tr);
}

static inline gpointer
build_transaction_key (QmiMessage *message)
{
    gpointer key;
    guint8 service;
    guint8 client_id;
    guint16 transaction_id;

    service = (guint8)qmi_message_get_service (message);
    client_id = qmi_message_get_client_id (message);
    transaction_id = qmi_message_get_transaction_id (message);

    key = (gpointer)((((service << 8) | client_id) << 16) | transaction_id);

#ifdef MESSAGE_ENABLE_TRACE
    {
        gchar *hex;

        hex = qmi_utils_str_hex (&key, sizeof (key), ':');
        g_debug ("KEY: %s", hex);
        g_free (hex);

        hex = qmi_utils_str_hex (&service, sizeof (service), ':');
        g_debug ("  Service: %s", hex);
        g_free (hex);

        hex = qmi_utils_str_hex (&client_id, sizeof (client_id), ':');
        g_debug ("  Client ID: %s", hex);
        g_free (hex);

        hex = qmi_utils_str_hex (&transaction_id, sizeof (transaction_id), ':');
        g_debug ("  Transaction ID: %s", hex);
        g_free (hex);
    }
#endif /* MESSAGE_ENABLE_TRACE */

    return key;
}

static Transaction *
device_release_transaction (QmiDevice *self,
                            gpointer key)
{
    Transaction *tr = NULL;

    if (self->priv->transactions) {
        tr = g_hash_table_lookup (self->priv->transactions, key);
        if (tr)
            /* If found, remove it from the HT */
            g_hash_table_remove (self->priv->transactions, key);
    }

    return tr;
}

typedef struct {
    QmiDevice *self;
    gpointer key;
} TransactionTimeoutContext;

static gboolean
transaction_timed_out (TransactionTimeoutContext *ctx)
{
    Transaction *tr;
    GError *error = NULL;

    tr = device_release_transaction (ctx->self, ctx->key);
    tr->timeout_id = 0;

    /* Complete transaction with a timeout error */
    error = g_error_new (QMI_CORE_ERROR,
                         QMI_CORE_ERROR_TIMEOUT,
                         "Transaction timed out");
    transaction_complete_and_free (tr, NULL, error);
    g_error_free (error);

    return FALSE;
}

static gboolean
device_store_transaction (QmiDevice *self,
                          Transaction *tr,
                          guint timeout)
{
    TransactionTimeoutContext *timeout_ctx;
    gpointer key;

    if (G_UNLIKELY (!self->priv->transactions))
        self->priv->transactions = g_hash_table_new (g_direct_hash,
                                                     g_direct_equal);

    key = build_transaction_key (tr->message);
    g_hash_table_insert (self->priv->transactions, key, tr);

    /* Once it gets into the HT, setup the timeout */
    timeout_ctx = g_slice_new (TransactionTimeoutContext);
    timeout_ctx->self = self;
    timeout_ctx->key = key;
    tr->timeout_id = g_timeout_add_seconds (timeout,
                                            (GSourceFunc)transaction_timed_out,
                                            timeout_ctx);
}

static Transaction *
device_match_transaction (QmiDevice *self,
                          QmiMessage *message)
{
    /* msg can be either the original message or the response */
    return device_release_transaction (self, build_transaction_key (message));
}

/*****************************************************************************/

/**
 * qmi_device_get_file:
 * @self: a #QmiDevice.
 *
 * Get the #GFile associated with this #QmiDevice.
 *
 * Returns: a #GFile that must be freed with g_object_unref().
 */
GFile *
qmi_device_get_file (QmiDevice *self)
{
    GFile *file = NULL;

    g_return_val_if_fail (QMI_IS_DEVICE (self), NULL);

    g_object_get (G_OBJECT (self),
                  QMI_DEVICE_FILE, &file,
                  NULL);
    return file;
}

/**
 * qmi_device_peek_client_ctl:
 * @self: a #QmiDevice.
 *
 * Get the #QmiClientCtl handled by this #QmiDevice, without increasing the reference count
 * on the returned object.
 *
 * Returns: a #GFile. Do not free the returned object, it is owned by @self.
 */
QmiClientCtl *
qmi_device_peek_client_ctl (QmiDevice *self)
{
    g_return_val_if_fail (QMI_IS_DEVICE (self), NULL);

    return self->priv->client_ctl;
}

/**
 * qmi_device_get_client_ctl:
 * @self: a #QmiDevice.
 *
 * Get the #QmiClientCtl handled by this #QmiDevice.
 *
 * Returns: a #QmiClientCtl that must be freed with g_object_unref().
 */
QmiClientCtl *
qmi_device_get_client_ctl (QmiDevice *self)
{
    QmiClientCtl *client_ctl = NULL;

    g_return_val_if_fail (QMI_IS_DEVICE (self), NULL);

    g_object_get (G_OBJECT (self),
                  QMI_DEVICE_CLIENT_CTL, &client_ctl,
                  NULL);
    return client_ctl;
}

/**
 * qmi_device_peek_file:
 * @self: a #QmiDevice.
 *
 * Get the #GFile associated with this #QmiDevice, without increasing the reference count
 * on the returned object.
 *
 * Returns: a #GFile. Do not free the returned object, it is owned by @self.
 */
GFile *
qmi_device_peek_file (QmiDevice *self)
{
    g_return_val_if_fail (QMI_IS_DEVICE (self), NULL);

    return self->priv->file;
}

/**
 * qmi_device_get_path:
 * @self: a #QmiDevice.
 *
 * Get the system path of the underlying QMI device.
 *
 * Returns: the system path of the device.
 */
const gchar *
qmi_device_get_path (QmiDevice *self)
{
    g_return_val_if_fail (QMI_IS_DEVICE (self), NULL);

    return self->priv->path;
}

/**
 * qmi_device_get_path_display:
 * @self: a #QmiDevice.
 *
 * Get the system path of the underlying QMI device in UTF-8.
 *
 * Returns: UTF-8 encoded system path of the device.
 */
const gchar *
qmi_device_get_path_display (QmiDevice *self)
{
    g_return_val_if_fail (QMI_IS_DEVICE (self), NULL);

    return self->priv->path_display;
}

/**
 * qmi_device_is_open:
 * @self: a #QmiDevice.
 *
 * Checks whether the #QmiDevice is open for I/O.
 *
 * Returns: #TRUE if @self is open, #FALSE otherwise.
 */
gboolean
qmi_device_is_open (QmiDevice *self)
{
    g_return_val_if_fail (QMI_IS_DEVICE (self), FALSE);

    return !!self->priv->iochannel;
}

/*****************************************************************************/
/* Open device */

static void
parse_response (QmiDevice *self)
{
    do {
        GError *error = NULL;
        QmiMessage *message;

        /* Every message received must start with the QMUX marker.
         * If it doesn't, we broke framing :-/
         * If we broke framing, an error should be reported and the device
         * should get closed */
        if (self->priv->response->len > 0 &&
            self->priv->response->data[0] != QMI_MESSAGE_QMUX_MARKER) {
            /* TODO: Report fatal error */
            g_warning ("QMI framing error detected");
            return;
        }

        message = qmi_message_new_from_raw (self->priv->response->data,
                                            self->priv->response->len);
        if (!message)
            /* More data we need */
            return;

        /* Remove the read data from the response buffer */
        g_byte_array_remove_range (self->priv->response,
                                   0,
                                   qmi_message_get_length (message));

        /* Ensure the read message is valid */
        if (!qmi_message_check (message, &error)) {
            g_warning ("Invalid QMI message received: %s",
                       error->message);
            g_error_free (error);
        } else {
            Transaction *tr;

            /* Got a valid QMI message, do whatever we need to do with it now */

#ifdef MESSAGE_ENABLE_TRACE
            {
                gchar *printable;

                printable = qmi_message_get_printable (message);
                g_debug ("[%s] Received message...\n%s",
                         self->priv->path_display,
                         printable);
                g_free (printable);
            }
#endif /* MESSAGE_ENABLE_TRACE */

            tr = device_match_transaction (self, message);
            if (!tr)
                g_debug ("[%s] No transaction matched in received message",
                         self->priv->path_display);
            else {
                /* Received message is a response in a transaction; handle QMI protocol
                 * errors */
                if (!qmi_message_get_result (message, &error)) {
                    transaction_complete_and_free (tr, NULL, error);
                    g_error_free (error);
                } else {
                    /* Report the reply message */
                    transaction_complete_and_free (tr, message, NULL);
                }
            }
        }

        qmi_message_unref (message);
    } while (self->priv->response->len > 0);
}

static gboolean
data_available (GIOChannel *source,
                GIOCondition condition,
                QmiDevice *self)
{
    gsize bytes_read;
    GIOStatus status;
    guint8 buffer[BUFFER_SIZE + 1];

    if (condition & G_IO_HUP) {
        g_debug ("[%s] unexpected port hangup!",
                 self->priv->path_display);

        if (self->priv->response &&
            self->priv->response->len)
            g_byte_array_remove_range (self->priv->response, 0, self->priv->response->len);

        qmi_device_close (self, NULL);
        return FALSE;
    }

    if (condition & G_IO_ERR) {
        if (self->priv->response &&
            self->priv->response->len)
            g_byte_array_remove_range (self->priv->response, 0, self->priv->response->len);
        return TRUE;
    }

    /* If not ready yet, prepare the response with default initial size. */
    if (G_UNLIKELY (!self->priv->response))
        self->priv->response = g_byte_array_sized_new (500);

    do {
        GError *error = NULL;

        status = g_io_channel_read_chars (source,
                                          buffer,
                                          BUFFER_SIZE,
                                          &bytes_read,
                                          &error);
        if (status == G_IO_STATUS_ERROR) {
            if (error) {
                g_warning ("error reading from the IOChannel: '%s'", error->message);
                g_error_free (error);
            }

            /* Port is closed; we're done */
            if (self->priv->watch_id == 0)
                break;
        }

        /* If no bytes read, just let g_io_channel wait for more data */
        if (bytes_read == 0)
            break;

        if (bytes_read > 0)
            g_byte_array_append (self->priv->response, buffer, bytes_read);

        /* Try to parse what we already got */
        parse_response (self);

        /* And keep on if we were told to keep on */
    } while (bytes_read == BUFFER_SIZE || status == G_IO_STATUS_AGAIN);

    return TRUE;
}

static gboolean
create_iochannel (QmiDevice *self,
                  GError **error)
{
    if (self->priv->iochannel) {
        g_set_error (error,
                     QMI_CORE_ERROR,
                     QMI_CORE_ERROR_WRONG_STATE,
                     "already open",
                     self->priv->path_display);
        return FALSE;
    }

    g_assert (self->priv->file);
    g_assert (self->priv->path);
    self->priv->iochannel = g_io_channel_new_file (self->priv->path, "r+", error);

    if (self->priv->iochannel) {
        GError *inner_error = NULL;

        /* We don't want UTF-8 encoding, we're playing with raw binary data */
        g_io_channel_set_encoding (self->priv->iochannel, NULL, NULL);

        /* We don't want to get the channel buffered */
        g_io_channel_set_buffered (self->priv->iochannel, FALSE);

        /* We don't want to get blocked while writing stuff */
        if (!g_io_channel_set_flags (self->priv->iochannel,
                                     G_IO_FLAG_NONBLOCK,
                                     &inner_error)) {
            g_prefix_error (&inner_error, "Cannot set non-blocking channel: ");
            g_propagate_error (error, inner_error);
            g_io_channel_shutdown (self->priv->iochannel, FALSE, NULL);
            g_io_channel_unref (self->priv->iochannel);
            self->priv->iochannel = NULL;
        }
    }

    if (self->priv->iochannel)
        self->priv->watch_id = g_io_add_watch (self->priv->iochannel,
                                               G_IO_IN | G_IO_ERR | G_IO_HUP,
                                               (GIOFunc)data_available,
                                               self);

    return !!self->priv->iochannel;
}

/**
 * qmi_device_open_finish:
 * @self: a #QmiDevice.
 * @res: a #GAsyncResult.
 * @error: a #GError.
 *
 * Finishes an asynchronous open operation started with qmi_device_open_async().
 *
 * Returns: #TRUE if successful, #FALSE if @error is set.
 */
gboolean
qmi_device_open_finish (QmiDevice *self,
                        GAsyncResult *res,
                        GError **error)
{
    return !g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (res), error);
}

static void
version_info_ready (QmiClientCtl *client_ctl,
                    GAsyncResult *res,
                    GSimpleAsyncResult *simple)
{
    QmiDevice *self;
    GError *error = NULL;
    GArray *services;
    guint i;

    services = qmi_client_ctl_get_version_info_finish (client_ctl, res, &error);
    if (!services) {
        g_prefix_error (&error, "Version info check failed: ");
        g_simple_async_result_take_error (simple, error);
        g_simple_async_result_complete (simple);
        g_object_unref (simple);
        return;
    }

    /* Recover the QmiDevice */
    self = QMI_DEVICE (g_async_result_get_source_object (G_ASYNC_RESULT (simple)));

    g_debug ("[%s] QMI Device supports %u services:",
             self->priv->path_display,
             services->len);
    for (i = 0; i < services->len; i++) {
        QmiCtlVersionInfo *service;

        service = &g_array_index (services, QmiCtlVersionInfo, i);
        g_debug ("[%s]    %s (%u.%u)",
                 self->priv->path_display,
                 qmi_service_get_string (service->service_type),
                 service->major_version,
                 service->minor_version);
    }

    g_array_unref (services);
    g_simple_async_result_set_op_res_gboolean (simple, TRUE);
    g_simple_async_result_complete (simple);
    g_object_unref (simple);
    g_object_unref (self);
}

/**
 * qmi_device_open:
 * @self: a #QmiDevice.
 * @timeout: maximum time, in seconds, to wait for the device to be opened.
 *
 * Asynchronously opens a #QmiDevice for I/O.
 *
 * When the operation is finished @callback will be called. You can then call
 * qmi_device_open_finish() to get the result of the operation.
 */
void
qmi_device_open (QmiDevice *self,
                 guint timeout,
                 GCancellable *cancellable,
                 GAsyncReadyCallback callback,
                 gpointer user_data)
{
    GSimpleAsyncResult *result;
    GError *error = NULL;

    g_return_if_fail (QMI_IS_DEVICE (self));

    result = g_simple_async_result_new (G_OBJECT (self),
                                        callback,
                                        user_data,
                                        qmi_device_open);

    if (!create_iochannel (self, &error)) {
        g_prefix_error (&error,
                        "Cannot open (async) QMI device: ");
        g_simple_async_result_take_error (result, error);
        g_simple_async_result_complete_in_idle (result);
        g_object_unref (result);
        return;
    }

    /* Query version info */
    g_debug ("Checking version info...");
    qmi_client_ctl_get_version_info (self->priv->client_ctl,
                                     timeout,
                                     cancellable,
                                     (GAsyncReadyCallback)version_info_ready,
                                     result);
}

/*****************************************************************************/
/* Close channel */

static gboolean
destroy_iochannel (QmiDevice *self,
                   GError **error)
{
    GError *inner_error = NULL;

    /* Already closed? */
    if (!self->priv->iochannel)
        return TRUE;

    g_io_channel_shutdown (self->priv->iochannel, TRUE, &inner_error);

    /* Failures when closing still make the device to get closed */
    g_io_channel_unref (self->priv->iochannel);
    self->priv->iochannel = NULL;
    self->priv->watch_id = 0;
    if (self->priv->response) {
        g_byte_array_unref (self->priv->response);
        self->priv->response = NULL;
    }

    if (inner_error) {
        g_propagate_error (error, inner_error);
        return FALSE;
    }

    return TRUE;
}

/**
 * qmi_device_close:
 * @self: a #QmiDevice
 * @error: a #GError
 *
 * Synchronously closes a #QmiDevice, preventing any further I/O.
 *
 * Closing a #QmiDevice multiple times will not return an error.
 *
 * Returns: #TRUE if successful, #FALSE if @error is set.
 */
gboolean
qmi_device_close (QmiDevice *self,
                  GError **error)
{
    g_return_val_if_fail (QMI_IS_DEVICE (self), FALSE);

    if (!destroy_iochannel (self, error)) {
        g_prefix_error (error,
                        "Cannot close QMI device: ");
        return FALSE;
    }

    return TRUE;
}

/*****************************************************************************/
/* Command */

QmiMessage *
qmi_device_command_finish (QmiDevice *self,
                           GAsyncResult *res,
                           GError **error)
{
    if (g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (res), error))
        return NULL;

    return qmi_message_ref (g_simple_async_result_get_op_res_gpointer (
                                G_SIMPLE_ASYNC_RESULT (res)));
}

void
qmi_device_command (QmiDevice *self,
                    QmiMessage *message,
                    guint timeout,
                    GCancellable *cancellable,
                    GAsyncReadyCallback callback,
                    gpointer user_data)
{
    GError *error = NULL;
    Transaction *tr;
    gconstpointer raw_message;
    gsize raw_message_len;
    gsize written;

    g_return_if_fail (QMI_IS_DEVICE (self));
    g_return_if_fail (message != NULL);

    tr = transaction_new (self, message, callback, user_data);

    /* Device must be open */
    if (!self->priv->iochannel) {
        error = g_error_new (QMI_CORE_ERROR,
                             QMI_CORE_ERROR_WRONG_STATE,
                             "Device must be open to send commands");
        transaction_complete_and_free (tr, NULL, error);
        g_error_free (error);
        return;
    }

    /* Non-CTL services should use a proper CID */
    if (qmi_message_get_service (message) != QMI_SERVICE_CTL &&
        qmi_message_get_client_id (message) == 0) {
        error = g_error_new (QMI_CORE_ERROR,
                             QMI_CORE_ERROR_FAILED,
                             "Cannot send message in service '%s' without a CID",
                             qmi_service_get_string (qmi_message_get_service (message)));
        transaction_complete_and_free (tr, NULL, error);
        g_error_free (error);
        return;
    }

#ifdef MESSAGE_ENABLE_TRACE
    {
        gchar *printable;

        printable = qmi_message_get_printable (message);
        g_debug ("[%s] Sending message...\n%s",
                 self->priv->path_display,
                 printable);
        g_free (printable);
    }
#endif /* MESSAGE_ENABLE_TRACE */

    /* Get raw message */
    raw_message = qmi_message_get_raw (message, &raw_message_len, &error);
    if (!raw_message) {
        g_prefix_error (&error, "Cannot get raw message: ");
        transaction_complete_and_free (tr, NULL, error);
        g_error_free (error);
        return;
    }

    /* Setup context to match response */
    device_store_transaction (self, tr, timeout);

    written = 0;
    if (g_io_channel_write_chars (self->priv->iochannel,
                                  raw_message,
                                  (gssize)raw_message_len,
                                  &written,
                                  &error) != G_IO_STATUS_NORMAL) {
        g_prefix_error (&error, "Cannot write message: ");

        /* Match transaction so that we remove it from our tracking table */
        tr = device_match_transaction (self, message);
        transaction_complete_and_free (tr, NULL, error);
        g_error_free (error);
        return;
    }

    /* Just return, we'll get response asynchronously */
}

/*****************************************************************************/
/* New QMI device */

/**
 * qmi_device_new_finish:
 * @res: a #GAsyncResult.
 * @error: a #GError.
 *
 * Finishes an operation started with qmi_device_new().
 *
 * Returns: A newly created #QmiDevice, or #NULL if @error is set.
 */
QmiDevice *
qmi_device_new_finish (GAsyncResult *res,
                       GError **error)
{
  GObject *ret;
  GObject *source_object;

  source_object = g_async_result_get_source_object (res);
  ret = g_async_initable_new_finish (G_ASYNC_INITABLE (source_object), res, error);
  g_object_unref (source_object);

  return (ret ? QMI_DEVICE (ret) : NULL);
}

/**
 * qmi_device_new:
 * @file: a #GFile.
 * @cancellable: optional #GCancellable object, #NULL to ignore.
 * @callback: a #GAsyncReadyCallback to call when the initialization is finished.
 * @user_data: the data to pass to callback function.
 *
 * Asynchronously creates a #QmiDevice object to manage @file.
 * When the operation is finished, @callback will be invoked. You can then call
 * qmi_device_new_finish() to get the result of the operation.
 */
void
qmi_device_new (GFile *file,
                GCancellable *cancellable,
                GAsyncReadyCallback callback,
                gpointer user_data)
{
    g_async_initable_new_async (QMI_TYPE_DEVICE,
                                G_PRIORITY_DEFAULT,
                                cancellable,
                                callback,
                                user_data,
                                QMI_DEVICE_FILE, file,
                                NULL);
}

/*****************************************************************************/
/* Async init */

typedef struct {
    QmiDevice *self;
    GSimpleAsyncResult *result;
    GCancellable *cancellable;
} InitContext;

static void
init_context_complete_and_free (InitContext *ctx)
{
    g_simple_async_result_complete_in_idle (ctx->result);
    if (ctx->cancellable)
        g_object_unref (ctx->cancellable);
    g_object_unref (ctx->result);
    g_object_unref (ctx->self);
    g_slice_free (InitContext, ctx);
}

static gboolean
initable_init_finish (GAsyncInitable  *initable,
                      GAsyncResult    *result,
                      GError         **error)
{
    return !g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (result), error);
}

static void
client_ctl_ready (GAsyncInitable *initable,
                  GAsyncResult *res,
                  InitContext *ctx)
{
    GError *error = NULL;
    GObject *obj;

    obj = g_async_initable_new_finish (initable, res, &error);
    if (!obj) {
        g_prefix_error (&error,
                        "Couldn't create CTL client: ");
        g_simple_async_result_take_error (ctx->result, error);
        init_context_complete_and_free (ctx);
        return;
    }

    /* Keep the client */
    ctx->self->priv->client_ctl = QMI_CLIENT_CTL (obj);

    /* Done we are */
    g_simple_async_result_set_op_res_gboolean (ctx->result, TRUE);
    init_context_complete_and_free (ctx);
}

static void
query_info_async_ready (GFile *file,
                        GAsyncResult *res,
                        InitContext *ctx)
{
    GError *error = NULL;
    GFileInfo *info;

    info = g_file_query_info_finish (file, res, &error);
    if (!info) {
        g_prefix_error (&error,
                        "Couldn't query file info: ");
        g_simple_async_result_take_error (ctx->result, error);
        init_context_complete_and_free (ctx);
        return;
    }

    /* Our QMI device must be of SPECIAL type */
    if (g_file_info_get_file_type (info) != G_FILE_TYPE_SPECIAL) {
        g_simple_async_result_set_error (ctx->result,
                                         QMI_CORE_ERROR,
                                         QMI_CORE_ERROR_FAILED,
                                         "Wrong file type");
        init_context_complete_and_free (ctx);
        return;
    }
    g_object_unref (info);

    /* Create the implicit CTL client
     * We are giving already the CID of the CTL client, the default one */
    g_async_initable_new_async (QMI_TYPE_CLIENT_CTL,
                                G_PRIORITY_DEFAULT,
                                ctx->cancellable,
                                (GAsyncReadyCallback)client_ctl_ready,
                                ctx,
                                QMI_CLIENT_DEVICE, ctx->self,
                                QMI_CLIENT_SERVICE, QMI_SERVICE_CTL,
                                NULL);
}

static void
initable_init_async (GAsyncInitable *initable,
                     int io_priority,
                     GCancellable *cancellable,
                     GAsyncReadyCallback callback,
                     gpointer user_data)
{
    InitContext *ctx;

    ctx = g_slice_new0 (InitContext);
    ctx->self = g_object_ref (initable);
    if (cancellable)
        ctx->cancellable = g_object_ref (cancellable);
    ctx->result = g_simple_async_result_new (G_OBJECT (initable),
                                             callback,
                                             user_data,
                                             initable_init_async);

    /* We need a proper file to initialize */
    if (!ctx->self->priv->file) {
        g_simple_async_result_set_error (ctx->result,
                                         QMI_CORE_ERROR,
                                         QMI_CORE_ERROR_INVALID_ARGS,
                                         "Cannot initialize QMI device: No file given");
        init_context_complete_and_free (ctx);
        return;
    }

    /* Check the file type. Note that this is just a quick check to avoid
     * creating QmiDevices pointing to a location already known not to be a QMI
     * device. */
    g_file_query_info_async (ctx->self->priv->file,
                             G_FILE_ATTRIBUTE_STANDARD_TYPE,
                             G_FILE_QUERY_INFO_NONE,
                             G_PRIORITY_DEFAULT,
                             ctx->cancellable,
                             (GAsyncReadyCallback)query_info_async_ready,
                             ctx);
}

/*****************************************************************************/

static void
set_property (GObject *object,
              guint prop_id,
              const GValue *value,
              GParamSpec *pspec)
{
    QmiDevice *self = QMI_DEVICE (object);

    switch (prop_id) {
    case PROP_FILE:
        g_assert (self->priv->file == NULL);
        self->priv->file = g_value_dup_object (value);
        self->priv->path = g_file_get_path (self->priv->file);
        self->priv->path_display = g_filename_display_name (self->priv->path);
        break;
    case PROP_CLIENT_CTL:
        /* Not writable */
        g_assert_not_reached ();
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
get_property (GObject *object,
              guint prop_id,
              GValue *value,
              GParamSpec *pspec)
{
    QmiDevice *self = QMI_DEVICE (object);

    switch (prop_id) {
    case PROP_FILE:
        g_value_set_object (value, self->priv->file);
        break;
    case PROP_CLIENT_CTL:
        g_value_set_object (value, self->priv->client_ctl);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
qmi_device_init (QmiDevice *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE ((self),
                                              QMI_TYPE_DEVICE,
                                              QmiDevicePrivate);
}

static void
dispose (GObject *object)
{
    QmiDevice *self = QMI_DEVICE (object);

    g_clear_object (&self->priv->file);
    g_clear_object (&self->priv->client_ctl);

    G_OBJECT_CLASS (qmi_device_parent_class)->dispose (object);
}

static void
finalize (GObject *object)
{
    QmiDevice *self = QMI_DEVICE (object);

    /* Transactions keep refs to the device, so it's actually
     * impossible to have any content in the HT */
    g_assert (g_hash_table_size (self->priv->transactions) == 0);
    g_hash_table_unref (self->priv->transactions);

    g_free (self->priv->path);
    g_free (self->priv->path_display);
    if (self->priv->response)
        g_byte_array_unref (self->priv->response);
    if (self->priv->iochannel)
        g_io_channel_unref (self->priv->iochannel);

    G_OBJECT_CLASS (qmi_device_parent_class)->finalize (object);
}

static void
async_initable_iface_init (GAsyncInitableIface *iface)
{
    iface->init_async = initable_init_async;
    iface->init_finish = initable_init_finish;
}

static void
qmi_device_class_init (QmiDeviceClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (object_class, sizeof (QmiDevicePrivate));

    object_class->get_property = get_property;
    object_class->set_property = set_property;
    object_class->finalize = finalize;
    object_class->dispose = dispose;

    properties[PROP_FILE] =
        g_param_spec_object (QMI_DEVICE_FILE,
                             "Device file",
                             "File to the underlying QMI device",
                             G_TYPE_FILE,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    g_object_class_install_property (object_class, PROP_FILE, properties[PROP_FILE]);

    properties[PROP_CLIENT_CTL] =
        g_param_spec_object (QMI_DEVICE_CLIENT_CTL,
                             "CTL client",
                             "Implicit CTL client",
                             QMI_TYPE_CLIENT_CTL,
                             G_PARAM_READABLE);
    g_object_class_install_property (object_class, PROP_CLIENT_CTL, properties[PROP_CLIENT_CTL]);
}
