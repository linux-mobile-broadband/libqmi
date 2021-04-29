/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 * libmbim-glib -- GLib/GIO based library to control MBIM devices
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
 * Copyright (C) 2014 Aleksander Morgado <aleksander@lanedo.com>
 * Copyright (C) 2014 Smith Micro Software, Inc.
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
#include "mbim-device.h"
#include "mbim-utils.h"
#include "mbim-helpers.h"
#include "mbim-proxy.h"
#include "mbim-message-private.h"
#include "mbim-cid.h"
#include "mbim-enum-types.h"
#include "mbim-error-types.h"
#include "mbim-basic-connect.h"
#include "mbim-proxy-helpers.h"

/* The mbim-proxy may be used for bulk data transfer, such as modem
 * firmware upgrade, and the BUFFER_SIZE should be at least equal
 * to MAX_CONTROL_TRANSFER which defined in mbim-device.c, which
 * will bring better performance in such case.
 */
#define BUFFER_SIZE 4096

G_DEFINE_TYPE (MbimProxy, mbim_proxy, G_TYPE_OBJECT)

enum {
    PROP_0,
    PROP_N_CLIENTS,
    PROP_N_DEVICES,
    PROP_LAST
};

static GParamSpec *properties[PROP_LAST];

struct _MbimProxyPrivate {
    /* Unix socket service */
    GSocketService *socket_service;

    /* Clients */
    GList *clients;

    /* Devices */
    GList *devices;
    GList *opening_devices;
};

static void        track_device         (MbimProxy *self, MbimDevice *device);
static void        untrack_device       (MbimProxy *self, MbimDevice *device);
static MbimDevice *peek_device_for_path (MbimProxy *self, const gchar *path);

/*****************************************************************************/

guint
mbim_proxy_get_n_clients (MbimProxy *self)
{
    g_return_val_if_fail (MBIM_IS_PROXY (self), 0);

    return g_list_length (self->priv->clients);
}

guint
mbim_proxy_get_n_devices (MbimProxy *self)
{
    g_return_val_if_fail (MBIM_IS_PROXY (self), 0);

    return g_list_length (self->priv->devices);
}

/*****************************************************************************/
/* Client info */

typedef struct {
    volatile gint ref_count;
    gulong        id;

    MbimProxy *self; /* not full ref */
    GSocketConnection *connection;
    GSource *connection_readable_source;
    GByteArray *buffer;

    /* Only one proxy config allowed at a time */
    gboolean config_ongoing;

    MbimDevice *device;
    guint indication_id;
    MbimEventEntry **mbim_event_entry_array;
    gsize mbim_event_entry_array_size;
} Client;

static gboolean connection_readable_cb (GSocket *socket, GIOCondition condition, Client *client);
static void     track_client           (MbimProxy *self, Client *client);
static void     untrack_client         (MbimProxy *self, Client *client);

static void
client_disconnect (Client *client)
{
    g_clear_pointer (&client->mbim_event_entry_array, mbim_event_entry_array_free);
    client->mbim_event_entry_array_size = 0;

    if (client->connection_readable_source) {
        g_source_destroy (client->connection_readable_source);
        g_source_unref (client->connection_readable_source);
        client->connection_readable_source = 0;
    }

    if (client->connection) {
        g_debug ("[client %lu] connection closed", client->id);
        g_output_stream_close (g_io_stream_get_output_stream (G_IO_STREAM (client->connection)), NULL, NULL);
        g_object_unref (client->connection);
        client->connection = NULL;
    }
}

static void client_indication_cb (MbimDevice *device,
                                  MbimMessage *message,
                                  Client *client);

static void
client_set_device (Client *client,
                   MbimDevice *device)
{
    if (client->device) {
        if (g_signal_handler_is_connected (client->device, client->indication_id))
            g_signal_handler_disconnect (client->device, client->indication_id);
        g_object_unref (client->device);
    }

    if (device) {
        client->device = g_object_ref (device);
        client->indication_id = g_signal_connect (client->device,
                                                  MBIM_DEVICE_SIGNAL_INDICATE_STATUS,
                                                  G_CALLBACK (client_indication_cb),
                                                  client);
    } else {
        client->device = NULL;
        client->indication_id = 0;
    }
}

static void
client_unref (Client *client)
{
    if (g_atomic_int_dec_and_test (&client->ref_count)) {
        /* Ensure disconnected */
        client_disconnect (client);
        /* Reset device */
        client_set_device (client, NULL);

        if (client->buffer)
            g_byte_array_unref (client->buffer);

        if (client->mbim_event_entry_array)
            mbim_event_entry_array_free (client->mbim_event_entry_array);

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
client_send_message (Client       *client,
                     MbimMessage  *message,
                     GError      **error)
{
    if (!client->connection) {
        g_set_error (error,
                     MBIM_CORE_ERROR,
                     MBIM_CORE_ERROR_WRONG_STATE,
                     "Cannot send message: not connected");
        return FALSE;
    }

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
track_client (MbimProxy *self,
              Client *client)
{
    self->priv->clients = g_list_append (self->priv->clients, client_ref (client));
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_N_CLIENTS]);
}

static void
untrack_client (MbimProxy *self,
                Client *client)
{
    /* Disconnect the client explicitly when untracking */
    client_disconnect (client);

    if (g_list_find (self->priv->clients, client)) {
        self->priv->clients = g_list_remove (self->priv->clients, client);
        client_unref (client);
        g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_N_CLIENTS]);
    }
}

/*****************************************************************************/
/* Client indications */

static void
forward_indication (Client      *client,
                    MbimMessage *message)
{
    g_autoptr(GError) error = NULL;

    if (!client_send_message (client, message, &error))
        g_warning ("[client %lu] couldn't forward indication: %s", client->id, error->message);
}

static void
client_indication_cb (MbimDevice *device,
                      MbimMessage *message,
                      Client *client)
{
    MbimEventEntry *entry;
    guint           i;

    /* if client doesn't have a subscribe list, we're done. */
    if (!client->mbim_event_entry_array)
        return;

    /* Look for the event list associated to the service */
    entry = NULL;
    for (i = 0; i < client->mbim_event_entry_array_size; i++) {
        if (mbim_uuid_cmp (mbim_message_indicate_status_get_service_id (message),
                           &client->mbim_event_entry_array[i]->device_service_id)) {
            entry = client->mbim_event_entry_array[i];
            break;
        }
    }

    /* if client didn't subscribe to anything in this service, we're done */
    if (!entry)
        return;

    /* if client subscribed using the wildcard, no need to match specific cid */
    if (entry->cids_count == 0) {
        forward_indication (client, message);
        return;
    }

    /* Look for the specific cid in the event list */
    for (i = 0; i < entry->cids_count; i++) {
        if (mbim_message_indicate_status_get_cid (message) == entry->cids[i]) {
            /* exact match in the subscription */
            forward_indication (client, message);
            return;
        }
    }
}

/*****************************************************************************/
/* Request info */

typedef struct {
    MbimProxy *self;
    Client *client;
    MbimMessage *message;
    MbimMessage *response;
    guint32 original_transaction_id;
    /* Only used in proxy config */
    guint32 timeout_secs;
} Request;

static void
request_complete_and_free (Request *request)
{
    if (request->response) {
        g_autoptr(GError) error = NULL;

        /* Try to send response to client; if it fails, always assume we have
         * to close the connection */
        if (!client_send_message (request->client, request->response, &error)) {
            g_warning ("[client %lu,0x%08x] couldn't send response back to client: %s",
                       request->client->id, request->original_transaction_id, error->message);
            /* Disconnect and untrack client */
            untrack_client (request->self, request->client);
        }

        mbim_message_unref (request->response);
    }

    if (request->message)
        mbim_message_unref (request->message);
    client_unref (request->client);
    g_object_unref (request->self);
    g_slice_free (Request, request);
}

static Request *
request_new (MbimProxy   *self,
             Client      *client,
             MbimMessage *message)
{
    Request *request;

    request = g_slice_new0 (Request);
    request->self = g_object_ref (self);
    request->client = client_ref (client);
    request->message = mbim_message_ref (message);
    request->original_transaction_id = mbim_message_get_transaction_id (message);

    return request;
}

/*****************************************************************************/
/* Internal proxy device opening operation */

static MbimEventEntry **merge_client_service_subscribe_lists (MbimProxy  *self,
                                                              MbimDevice *device,
                                                              gsize      *out_size);
static void             reset_client_service_subscribe_lists (MbimProxy  *self,
                                                              MbimDevice *device);

typedef struct {
    MbimDevice *device;
    guint32     timeout_secs;
} InternalDeviceOpenContext;

static void
internal_device_open_context_free (InternalDeviceOpenContext *ctx)
{
    g_object_unref (ctx->device);
    g_slice_free (InternalDeviceOpenContext, ctx);
}

static gboolean
internal_device_open_finish (MbimProxy     *self,
                             GAsyncResult  *res,
                             GError       **error)
{
    return g_task_propagate_boolean (G_TASK (res), error);
}

typedef struct {
    MbimDevice *device;
    GList *pending;
} OpeningDevice;

static void
opening_device_complete_and_free (OpeningDevice *info,
                                  const GError  *error)
{
    GList *l;

    /* Complete all pending open tasks */
    for (l = info->pending; l; l = g_list_next (l)) {
        GTask *task = (GTask *)(l->data);

        if (error)
            g_task_return_error (task, g_error_copy (error));
        else
            g_task_return_boolean (task, TRUE);
        g_object_unref (task);
    }

    g_list_free (info->pending);
    g_object_unref (info->device);
    g_slice_free (OpeningDevice, info);
}

static OpeningDevice *
peek_opening_device_info (MbimProxy  *self,
                          MbimDevice *device)
{
    GList *l;

    /* If already being opened, queue it up */
    for (l = self->priv->opening_devices; l; l = g_list_next (l)) {
        OpeningDevice *info;

        info = (OpeningDevice *)(l->data);
        if (device == info->device)
            return info;
    }

    return NULL;
}

static void
complete_opening_device (MbimProxy    *self,
                         MbimDevice   *device,
                         const GError *error)
{
    OpeningDevice *info;

    info = peek_opening_device_info (self, device);
    if (!info)
        return;

    self->priv->opening_devices = g_list_remove (self->priv->opening_devices, info);
    opening_device_complete_and_free (info, error);
}

static void
cancel_opening_device (MbimProxy  *self,
                       MbimDevice *device)
{
    OpeningDevice *info;
    GError *error;

    info = peek_opening_device_info (self, device);
    if (!info)
        return;

    error = g_error_new (MBIM_CORE_ERROR, MBIM_CORE_ERROR_ABORTED, "Device is gone");
    complete_opening_device (self, device, error);
    g_error_free (error);
}

static void
device_open_ready (MbimDevice   *device,
                   GAsyncResult *res,
                   MbimProxy    *self)
{
    GError *error = NULL;

    mbim_device_open_finish (device, res, &error);

    /* Complete all pending open actions */
    complete_opening_device (self, device, error);

    if (error) {
        /* Fully untrack the device as it wasn't correctly open */
        untrack_device (self, device);
        g_error_free (error);
    }
}

static void
internal_open (GTask *task)
{
    MbimProxy                 *self;
    InternalDeviceOpenContext *ctx;
    OpeningDevice             *info;

    self = g_task_get_source_object (task);
    ctx  = g_task_get_task_data (task);

    /* If already being opened, queue it up */
    info = peek_opening_device_info (self, ctx->device);
    if (info) {
        info->pending = g_list_append (info->pending, task);
        return;
    }

    /* First time opening */
    info = g_slice_new0 (OpeningDevice);
    info->device = g_object_ref (ctx->device);
    info->pending = g_list_append (info->pending, task);
    self->priv->opening_devices = g_list_prepend (self->priv->opening_devices, info);

    /* Note: for now, only the first timeout request is taken into account */

    /* Need to open the device; and we must make sure the proxy only does this once, even
     * when multiple clients request it */
    mbim_device_open (ctx->device,
                      ctx->timeout_secs,
                      NULL,
                      (GAsyncReadyCallback)device_open_ready,
                      g_object_ref (self));
}

static void proxy_device_error_cb (MbimDevice *device,
                                   GError     *error,
                                   MbimProxy  *self);

static void
internal_device_open_caps_query_ready (MbimDevice   *device,
                                       GAsyncResult *res,
                                       GTask        *task)
{
    MbimProxy              *self;
    g_autoptr(MbimMessage)  response = NULL;
    g_autoptr(GError)       error = NULL;

    self = g_task_get_source_object (task);

    /* Always unblock error signals */
    g_signal_handlers_unblock_by_func (device, proxy_device_error_cb, self);

    response = mbim_device_command_finish (device, res, &error);
    if (!response || !mbim_message_response_get_result (response, MBIM_MESSAGE_TYPE_COMMAND_DONE, &error)) {
        /* If we get a not-opened error, well, force closing right away and reopen */
        if (g_error_matches (error, MBIM_PROTOCOL_ERROR, MBIM_PROTOCOL_ERROR_NOT_OPENED)) {
            g_debug ("[%s] device not-opened error reported, reopening", mbim_device_get_path (device));
            reset_client_service_subscribe_lists (self, device);
            mbim_device_close_force (device, NULL);
            internal_open (task);
            return;
        }

        /* Warn other (unlikely!) errors, but keep on anyway */
        g_warning ("[%s] device caps query during internal open failed: %s",
                   mbim_device_get_path (device), error->message);
    }

    g_debug ("[%s] device caps query during internal open succeeded",
             mbim_device_get_path (device));

    g_task_return_boolean (task, TRUE);
    g_object_unref (task);
}

static void
internal_device_open (MbimProxy           *self,
                      MbimDevice          *device,
                      guint32              timeout_secs,
                      GAsyncReadyCallback  callback,
                      gpointer             user_data)
{
    InternalDeviceOpenContext *ctx;
    GTask                     *task;

    task = g_task_new (self, NULL, callback, user_data);

    ctx = g_slice_new0 (InternalDeviceOpenContext);
    ctx->device = g_object_ref (device);
    ctx->timeout_secs = timeout_secs;
    g_task_set_task_data (task, ctx, (GDestroyNotify) internal_device_open_context_free);

    /* If the device is flagged as already open, we still want to check
     * whether that's totally true, and we do that with a standard command
     * (loading caps in this case). */
    if (mbim_device_is_open (device)) {
        MbimMessage *message;

        /* Avoid getting notified of errors in this internal check, as we're
         * already going to check for the NotOpened error ourselves in the
         * ready callback, and we'll reopen silently if we find this. */
        g_signal_handlers_block_by_func (device, proxy_device_error_cb, self);

        g_debug ("[%s] checking device caps during client device open...",
                 mbim_device_get_path (device));
        message = mbim_message_device_caps_query_new (NULL);
        mbim_device_command (device,
                             message,
                             5,
                             NULL,
                             (GAsyncReadyCallback)internal_device_open_caps_query_ready,
                             task);
        mbim_message_unref (message);
        return;
    }

    internal_open (task);
}

/*****************************************************************************/
/* Proxy open */

static gboolean
process_internal_proxy_open (MbimProxy   *self,
                             Client      *client,
                             MbimMessage *message)
{
    Request *request;
    MbimStatusError status = MBIM_STATUS_ERROR_FAILURE;

    /* create request holder */
    request = request_new (self, client, message);

    if (!client->device)
        g_warning ("[client %lu] cannot process MBIM open: device not set", client->id);
    else if (!mbim_device_is_open (client->device))
        g_warning ("[client %lu] cannot process MBIM open: device not opened by proxy", client->id);
    else {
        g_debug ("[client %lu] connection to MBIM device '%s' established", client->id, mbim_device_get_path (client->device));
        status = MBIM_STATUS_ERROR_NONE;
    }

    request->response = mbim_message_open_done_new (mbim_message_get_transaction_id (request->message), status);
    request_complete_and_free (request);
    return TRUE;
}

/*****************************************************************************/
/* Proxy close */

static gboolean
process_internal_proxy_close (MbimProxy   *self,
                              Client      *client,
                              MbimMessage *message)
{
    Request *request;
    guint32  original_transaction_id;

    original_transaction_id = mbim_message_get_transaction_id (message);
    g_debug ("[client %lu,0x%08x] requested explicit MBIM channel close",
             client->id, original_transaction_id);

    request = request_new (self, client, message);
    request->response = mbim_message_close_done_new (original_transaction_id, MBIM_STATUS_ERROR_NONE);
    request_complete_and_free (request);
    return TRUE;
}

/*****************************************************************************/
/* Proxy config */

static MbimMessage *
build_proxy_control_command_done (MbimMessage     *message,
                                  MbimStatusError  status)
{
    MbimMessage *response;
    struct command_done_message *command_done;

    response = (MbimMessage *) _mbim_message_allocate (MBIM_MESSAGE_TYPE_COMMAND_DONE,
                                                       mbim_message_get_transaction_id (message),
                                                       sizeof (struct command_done_message));
    command_done = &(((struct full_message *)(response->data))->message.command_done);
    command_done->fragment_header.total   = GUINT32_TO_LE (1);
    command_done->fragment_header.current = 0;
    memcpy (command_done->service_id, MBIM_UUID_PROXY_CONTROL, sizeof (MbimUuid));
    command_done->command_id  = GUINT32_TO_LE (mbim_message_command_get_cid (message));
    command_done->status_code = GUINT32_TO_LE (status);
    command_done->buffer_length = 0;

    return response;
}

static void
proxy_config_internal_device_open_ready (MbimProxy    *self,
                                         GAsyncResult *res,
                                         Request      *request)
{
    g_autoptr(GError) error = NULL;

    if (!internal_device_open_finish (self, res, &error)) {
        g_warning ("[client %lu,0x%08x] cannot configure proxy: couldn't open MBIM device: %s",
                   request->client->id, request->original_transaction_id, error->message);
        /* Untrack client and complete without response */
        untrack_client (request->self, request->client);
        request_complete_and_free (request);
        return;
    }

    g_debug ("[client %lu,0x%08x] proxy configured",
             request->client->id, request->original_transaction_id);

    if (request->client->config_ongoing == TRUE)
        request->client->config_ongoing = FALSE;
    request->response = build_proxy_control_command_done (request->message, MBIM_STATUS_ERROR_NONE);
    request_complete_and_free (request);
}

static void
device_new_ready (GObject      *source,
                  GAsyncResult *res,
                  Request      *request)
{
    g_autoptr(GError)  error = NULL;
    MbimDevice        *existing;
    MbimDevice        *device;

    device = mbim_device_new_finish (res, &error);
    if (!device) {
        g_warning ("[client %lu,0x%08x] cannot configure proxy: couldn't create MBIM device: %s",
                   request->client->id, request->original_transaction_id, error->message);
        /* Untrack client and complete without response */
        untrack_client (request->self, request->client);
        request_complete_and_free (request);
        return;
    }

    /* Store device in the proxy independently */
    existing = peek_device_for_path (request->self, mbim_device_get_path (device));
    if (existing) {
        /* Race condition, we created two MbimDevices for the same port, just skip ours, no big deal */
        client_set_device (request->client, existing);
    } else {
        /* Keep the newly added device in the proxy */
        track_device (request->self, device);
        /* Also keep track of the device in the client */
        client_set_device (request->client, device);
    }
    g_object_unref (device);

    internal_device_open (request->self,
                          request->client->device,
                          request->timeout_secs,
                          (GAsyncReadyCallback)proxy_config_internal_device_open_ready,
                          request);
}

static gboolean
process_internal_proxy_config (MbimProxy   *self,
                               Client      *client,
                               MbimMessage *message)
{
    Request           *request;
    MbimDevice        *device;
    g_autofree gchar  *incoming_path = NULL;
    g_autofree gchar  *path = NULL;
    g_autoptr(GFile)   file = NULL;
    g_autoptr(GError)  error = NULL;

    /* create request holder */
    request = request_new (self, client, message);

    g_debug ("[client %lu,0x%08x] request to configure proxy",
             request->client->id, request->original_transaction_id);

    /* Error out if there is already a proxy config ongoing */
    if (client->config_ongoing) {
        g_warning ("[client %lu,0x%08x] cannot configure proxy: another request already ongoing",
                   request->client->id, request->original_transaction_id);
        request->response = build_proxy_control_command_done (message, MBIM_STATUS_ERROR_BUSY);
        request_complete_and_free (request);
        return TRUE;
    }

    /* Only allow SET command */
    if (mbim_message_command_get_command_type (message) != MBIM_MESSAGE_COMMAND_TYPE_SET) {
        g_warning ("[client %lu,0x%08x] cannot configure proxy: invalid request type",
                   request->client->id, request->original_transaction_id);
        request->response = build_proxy_control_command_done (message, MBIM_STATUS_ERROR_INVALID_PARAMETERS);
        request_complete_and_free (request);
        return TRUE;
    }

    /* Retrieve path from request */
    if (!_mbim_message_read_string (message, 0, 0, &incoming_path, &error)) {
        g_warning ("[client %lu,0x%08x] cannot configure proxy: couldn't read device path from request: %s",
                   request->client->id, request->original_transaction_id, error->message);
        request->response = build_proxy_control_command_done (message, MBIM_STATUS_ERROR_INVALID_PARAMETERS);
        request_complete_and_free (request);
        return TRUE;
    }

    /* The incoming path may be a symlink. In the proxy, we always use the real path of the
     * device, so that clients using different symlinks for the same file don't collide with
     * each other. */
    path = mbim_helpers_get_devpath (incoming_path, &error);
    if (!path) {
        g_warning ("[client %lu,0x%08x] cannot configure proxy: couldn't lookup real device path: %s",
                   request->client->id, request->original_transaction_id, error->message);
        request->response = build_proxy_control_command_done (message, MBIM_STATUS_ERROR_INVALID_PARAMETERS);
        request_complete_and_free (request);
        return TRUE;
    }

    /* Only allow subsequent requests with the same path */
    if (client->device) {
        if (g_str_equal (path, mbim_device_get_path (client->device))) {
            g_debug ("[client %lu,0x%08x] proxy re-configured",
                       request->client->id, request->original_transaction_id);
            request->response = build_proxy_control_command_done (message, MBIM_STATUS_ERROR_NONE);
        } else {
            g_warning ("[client %lu,0x%08x] cannot configure proxy: different device path given",
                       request->client->id, request->original_transaction_id);
            request->response = build_proxy_control_command_done (message, MBIM_STATUS_ERROR_FAILURE);
        }
        request_complete_and_free (request);
        return TRUE;
    }

    /* Read requested timeout value */
    if (!_mbim_message_read_guint32 (message, 8, &request->timeout_secs, &error)) {
        g_warning ("[client %lu,0x%08x] cannot configure proxy: couldn't read timeout from request: %s",
                   request->client->id, request->original_transaction_id, error->message);
        request->response = build_proxy_control_command_done (message, MBIM_STATUS_ERROR_INVALID_PARAMETERS);
        request_complete_and_free (request);
        return TRUE;
    }

    /* Check if some other client already handled the same device */
    device = peek_device_for_path (self, path);
    if (device) {
        /* Keep reference and continue */
        client_set_device (client, device);
        internal_device_open (self,
                              device,
                              request->timeout_secs,
                              (GAsyncReadyCallback)proxy_config_internal_device_open_ready,
                              request);
        return TRUE;
    }

    /* Flag as ongoing */
    client->config_ongoing = TRUE;

    /* Create new MBIM device */
    file = g_file_new_for_path (path);
    mbim_device_new (file,
                     NULL,
                     (GAsyncReadyCallback)device_new_ready,
                     request);
    return TRUE;
}

/*****************************************************************************/
/* Subscriber list */

static void
track_service_subscribe_list (Client      *client,
                              MbimMessage *message)
{
    g_autoptr(GError)              error = NULL;
    g_autoptr(MbimEventEntryArray) mbim_event_entry_array = NULL;
    gsize                          mbim_event_entry_array_size;

    mbim_event_entry_array = _mbim_proxy_helper_service_subscribe_request_parse (message, &mbim_event_entry_array_size, &error);
    if (error) {
        g_warning ("[client %lu] invalid subscribe request message: %s", client->id, error->message);
        return;
    }

    /* On each new request from the client, it should provide the FULL list of
     * events it's subscribed to, so we can safely recreate the whole array each
     * time. */
    g_clear_pointer (&client->mbim_event_entry_array, mbim_event_entry_array_free);
    client->mbim_event_entry_array = g_steal_pointer (&mbim_event_entry_array);
    client->mbim_event_entry_array_size = mbim_event_entry_array_size;

    if (mbim_utils_get_traces_enabled ()) {
        g_debug ("[client %lu] service subscribe list built", client->id);
        _mbim_proxy_helper_service_subscribe_list_debug ((const MbimEventEntry * const *)client->mbim_event_entry_array,
                                                         client->mbim_event_entry_array_size);
    }
}

static void
device_service_subscribe_list_set_complete (Request         *request,
                                            MbimStatusError  status)
{
    struct command_done_message *command_done;
    guint32                      raw_len;
    const guint8                *raw_data;

    /* The raw message data to send back as response to client */
    raw_data = mbim_message_command_get_raw_information_buffer (request->message, &raw_len);

    request->response = (MbimMessage *)_mbim_message_allocate (MBIM_MESSAGE_TYPE_COMMAND_DONE,
                                                               mbim_message_get_transaction_id (request->message),
                                                               sizeof (struct command_done_message) +
                                                               raw_len);
    command_done = &(((struct full_message *)(request->response->data))->message.command_done);
    command_done->fragment_header.total = GUINT32_TO_LE (1);
    command_done->fragment_header.current = 0;
    memcpy (command_done->service_id, MBIM_UUID_BASIC_CONNECT, sizeof (MbimUuid));
    command_done->command_id = GUINT32_TO_LE (MBIM_CID_BASIC_CONNECT_DEVICE_SERVICE_SUBSCRIBE_LIST);
    command_done->status_code = GUINT32_TO_LE (status);
    command_done->buffer_length = GUINT32_TO_LE (raw_len);
    memcpy (&command_done->buffer[0], raw_data, raw_len);

    request_complete_and_free (request);
}

static void
device_service_subscribe_list_set_ready (MbimDevice   *device,
                                         GAsyncResult *res,
                                         Request      *request)
{
    g_autoptr(MbimMessage) tmp_response = NULL;
    g_autoptr(GError)      error = NULL;
    MbimStatusError        error_status_code;

    tmp_response = mbim_device_command_finish (device, res, &error);
    if (!tmp_response) {
        /* Translate a MbimDevice wrong state error into a Not-Opened function error. */
        if (g_error_matches (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_WRONG_STATE)) {
            g_debug ("[client %lu,0x%08x] sending request to device failed: wrong state",
                     request->client->id, request->original_transaction_id);
            request->response = mbim_message_function_error_new (mbim_message_get_transaction_id (request->message), MBIM_PROTOCOL_ERROR_NOT_OPENED);
            request_complete_and_free (request);
            return;
        }

        /* Don't disconnect client, just let the request timeout in its side */
        g_debug ("[client %lu,0x%08x] sending request to device failed: %s",
                 request->client->id, request->original_transaction_id, error->message);
        request_complete_and_free (request);
        return;
    }

    g_debug ("[client %lu,0x%08x] response from device received",
             request->client->id, request->original_transaction_id);
    error_status_code = GUINT32_FROM_LE (((struct full_message *)(tmp_response->data))->message.command_done.status_code);
    device_service_subscribe_list_set_complete (request, error_status_code);
}

static gboolean
process_device_service_subscribe_list (MbimProxy   *self,
                                       Client      *client,
                                       MbimMessage *message)
{
    MbimEventEntry        **updated;
    gsize                   updated_size = 0;
    Request                *request;
    g_autoptr(MbimMessage)  request_message = NULL;

    /* create request holder */
    request = request_new (self, client, message);

    g_debug ("[client %lu,0x%08x] request to update service subscribe list received",
             request->client->id, request->original_transaction_id);

    /* trace the service subscribe list for the client */
    track_service_subscribe_list (client, message);

    /* merge all service subscribe list for all clients to set on device */
    updated = merge_client_service_subscribe_lists (self, client->device, &updated_size);
    if (!updated) {
        g_debug ("[client %lu,0x%08x] service subscribe list update in device not needed",
                 request->client->id, request->original_transaction_id);
        device_service_subscribe_list_set_complete (request, MBIM_STATUS_ERROR_NONE);
        return TRUE;
    }

    /* the 'updated' array was given to us as transfer-none!!!! */
    request_message = mbim_message_device_service_subscribe_list_set_new (updated_size, (const MbimEventEntry *const *)updated, NULL);
    mbim_message_set_transaction_id (request_message, mbim_device_get_next_transaction_id (client->device));

    g_debug ("[client %lu,0x%08x] updating service subscribe list in device...",
             request->client->id, request->original_transaction_id);
    mbim_device_command (client->device,
                         request_message,
                         300,
                         NULL,
                         (GAsyncReadyCallback)device_service_subscribe_list_set_ready,
                         request);
    return TRUE;
}

/*****************************************************************************/
/* Standard command */

static void
device_command_ready (MbimDevice   *device,
                      GAsyncResult *res,
                      Request      *request)
{
    g_autoptr(GError) error = NULL;

    request->response = mbim_device_command_finish (device, res, &error);
    if (!request->response) {
        /* Translate a MbimDevice wrong state error into a Not-Opened function error. */
        if (g_error_matches (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_WRONG_STATE)) {
            g_debug ("[client %lu,0x%08x] sending request to device failed: wrong state",
                     request->client->id, request->original_transaction_id);
            request->response = mbim_message_function_error_new (request->original_transaction_id, MBIM_PROTOCOL_ERROR_NOT_OPENED);
            request_complete_and_free (request);
            return;
        }

        /* Don't disconnect client, just let the request timeout in its side */
        g_debug ("[client %lu,0x%08x] sending request to device failed: %s",
                 request->client->id, request->original_transaction_id, error->message);
        request_complete_and_free (request);
        return;
    }

    /* replace reponse transaction id with the requested transaction id */
    g_debug ("[client %lu,0x%08x] response from device received",
             request->client->id, request->original_transaction_id);
    mbim_message_set_transaction_id (request->response, request->original_transaction_id);
    request_complete_and_free (request);
}

static gboolean
process_command (MbimProxy   *self,
                 Client      *client,
                 MbimMessage *message)
{
    Request     *request;
    const gchar *command;
    const gchar *command_type;
    const gchar *service;

    command = mbim_cid_get_printable (mbim_message_command_get_service (message),
                                      mbim_message_command_get_cid (message));
    command_type = mbim_message_command_type_get_string (mbim_message_command_get_command_type (message));
    service = mbim_service_get_string (mbim_message_command_get_service (message));

    /* create request holder */
    request = request_new (self, client, message);

    g_debug ("[client %lu,0x%08x] forwarding request to device: %s, %s, %s",
             client->id, request->original_transaction_id,
             service      ? service      : "unknown service",
             command_type ? command_type : "unknown command type",
             command      ? command      : "unknown command");

    if (_mbim_message_fragment_get_current (message) == _mbim_message_fragment_get_total (message) - 1)
        /* replace command transaction id with internal proxy transaction id to avoid collision */
        mbim_message_set_transaction_id (message, mbim_device_get_next_transaction_id (client->device));
    else
        /* avoid incrementing transaction until the last fragment is processed */
        mbim_message_set_transaction_id (message, mbim_device_get_transaction_id (client->device));

    /* The timeout needs to be big enough for any kind of transaction to
     * complete, otherwise the remote clients will lose the reply if they
     * configured a timeout bigger than this internal one. We should likely
     * make this value configurable per-client, instead of a hardcoded value.
     */
    mbim_device_command (client->device,
                         message,
                         300,
                         NULL,
                         (GAsyncReadyCallback)device_command_ready,
                         request);
    return TRUE;
}

/*****************************************************************************/

static gboolean
process_message (MbimProxy   *self,
                 Client      *client,
                 MbimMessage *message)
{
    /* Filter by message type */
    switch (mbim_message_get_message_type (message)) {
    case MBIM_MESSAGE_TYPE_OPEN:
        return process_internal_proxy_open (self, client, message);
    case MBIM_MESSAGE_TYPE_CLOSE:
        return process_internal_proxy_close (self, client, message);
    case MBIM_MESSAGE_TYPE_COMMAND:
        /* Proxy control message? */
        if (mbim_message_command_get_service (message) == MBIM_SERVICE_PROXY_CONTROL &&
            mbim_message_command_get_cid (message) == MBIM_CID_PROXY_CONTROL_CONFIGURATION)
            return process_internal_proxy_config (self, client, message);
        /* device service subscribe list message? */
        if (mbim_message_command_get_service (message) == MBIM_SERVICE_BASIC_CONNECT &&
            mbim_message_command_get_cid (message) == MBIM_CID_BASIC_CONNECT_DEVICE_SERVICE_SUBSCRIBE_LIST)
            return process_device_service_subscribe_list (self, client, message);
        /* Otherwise, standard command to forward */
        return process_command (self, client, message);
    case MBIM_MESSAGE_TYPE_INVALID:
    case MBIM_MESSAGE_TYPE_COMMAND_DONE:
    case MBIM_MESSAGE_TYPE_INDICATE_STATUS:
    case MBIM_MESSAGE_TYPE_HOST_ERROR:
    case MBIM_MESSAGE_TYPE_OPEN_DONE:
    case MBIM_MESSAGE_TYPE_CLOSE_DONE:
    case MBIM_MESSAGE_TYPE_FUNCTION_ERROR:
    default:
        g_debug ("[client %lu] invalid message: not a command", client->id);
        return FALSE;
    }

    g_assert_not_reached ();
}

static void
parse_request (MbimProxy *self,
               Client    *client)
{
    do {
        g_autoptr(MbimMessage) message = NULL;
        guint32                len = 0;

        if (client->buffer->len >= sizeof (struct header) &&
            (len = GUINT32_FROM_LE (((struct header *)client->buffer->data)->length)) > client->buffer->len) {
            /* have not received complete message */
            return;
        }

        if (!len)
            return;

        message = mbim_message_new (client->buffer->data, len);
        if (!message)
            return;

        g_byte_array_remove_range (client->buffer, 0, len);
        process_message (self, client, message);
    } while (client->buffer->len > 0);
}

static gboolean
connection_readable_cb (GSocket *socket,
                        GIOCondition condition,
                        Client *client)
{
    MbimProxy         *self;
    guint8             buffer[BUFFER_SIZE];
    g_autoptr(GError)  error = NULL;
    gssize             r;

    /* Recover proxy pointer soon */
    self = client->self;

    if (condition & G_IO_HUP || condition & G_IO_ERR) {
        untrack_client (self, client);
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
        g_warning ("[client %lu] error reading from istream: %s", client->id, error ? error->message : "unknown");
        /* Close the device */
        untrack_client (self, client);
        return FALSE;
    }

    if (r == 0)
        return TRUE;

    /* else, r > 0 */
    if (!G_UNLIKELY (client->buffer))
        client->buffer = g_byte_array_sized_new (r);
    g_byte_array_append (client->buffer, buffer, r);

    /* Try to parse input messages */
    parse_request (self, client);

    return TRUE;
}

static void
incoming_cb (GSocketService    *service,
             GSocketConnection *connection,
             GObject           *unused,
             MbimProxy         *self)
{
    static gulong            client_id = 0;
    Client                  *client;
    g_autoptr(GCredentials)  credentials = NULL;
    g_autoptr(GError)        error = NULL;
    uid_t                    uid;

    /* Each new incoming request updates the client id, even if the request is
     * not accepted */
    client_id++;

    g_debug ("[client %lu] connection open...", client_id);

    credentials = g_socket_get_credentials (g_socket_connection_get_socket (connection), &error);
    if (!credentials) {
        g_warning ("[client %lu] not allowed: error getting socket credentials: %s", client_id, error->message);
        return;
    }

    uid = g_credentials_get_unix_user (credentials, &error);
    if (error) {
        g_warning ("[client %lu] not allowed: error getting unix user id: %s", client_id, error->message);
        return;
    }

    if (!mbim_helpers_check_user_allowed (uid, &error)) {
        g_warning ("[client %lu] not allowed: %s", client_id, error->message);
        return;
    }

    /* Create client */
    client = g_slice_new0 (Client);
    client->self = self;
    client->ref_count = 1;
    client->id = client_id;
    client->connection = g_object_ref (connection);

    /* By default, a new client has all the standard services enabled for indications */
    client->mbim_event_entry_array = _mbim_proxy_helper_service_subscribe_list_new_standard (&client->mbim_event_entry_array_size);

    client->connection_readable_source = g_socket_create_source (g_socket_connection_get_socket (client->connection),
                                                                 G_IO_IN | G_IO_PRI | G_IO_ERR | G_IO_HUP,
                                                                 NULL);
    g_source_set_callback (client->connection_readable_source,
                           (GSourceFunc)connection_readable_cb,
                           client,
                           NULL);
    g_source_attach (client->connection_readable_source, g_main_context_get_thread_default ());

    /* Keep the client info around */
    track_client (self, client);

    client_unref (client);
}

static gboolean
setup_socket_service (MbimProxy  *self,
                      GError    **error)
{
    g_autoptr(GSocketAddress) socket_address = NULL;
    g_autoptr(GSocket)        socket = NULL;

    socket = g_socket_new (G_SOCKET_FAMILY_UNIX,
                           G_SOCKET_TYPE_STREAM,
                           G_SOCKET_PROTOCOL_DEFAULT,
                           error);
    if (!socket)
        return FALSE;

    /* Bind to address */
    socket_address = (g_unix_socket_address_new_with_type (
                          MBIM_PROXY_SOCKET_PATH,
                          -1,
                          G_UNIX_SOCKET_ADDRESS_ABSTRACT));
    if (!g_socket_bind (socket, socket_address, TRUE, error))
        return FALSE;

    g_debug ("creating UNIX socket service...");

    /* Listen */
    if (!g_socket_listen (socket, error))
        return FALSE;

    /* Create socket service */
    self->priv->socket_service = g_socket_service_new ();
    g_signal_connect (self->priv->socket_service, "incoming", G_CALLBACK (incoming_cb), self);
    if (!g_socket_listener_add_socket (G_SOCKET_LISTENER (self->priv->socket_service),
                                       socket,
                                       NULL, /* don't pass an object, will take a reference */
                                       error)) {
        g_prefix_error (error, "Error adding socket at '%s' to socket service: ", MBIM_PROXY_SOCKET_PATH);
        return FALSE;
    }

    g_debug ("starting UNIX socket service at '%s'...", MBIM_PROXY_SOCKET_PATH);
    g_socket_service_start (self->priv->socket_service);
    return TRUE;
}

/*****************************************************************************/
/* Device tracking */

#define DEVICE_CONTEXT_TAG "device-context-tag"
static GQuark device_context_quark;

typedef struct {
    /* Combined events array */
    MbimEventEntry **mbim_event_entry_array;
    gsize            mbim_event_entry_array_size;
} DeviceContext;

static void
device_context_free (DeviceContext *ctx)
{
    mbim_event_entry_array_free (ctx->mbim_event_entry_array);
    g_slice_free (DeviceContext, ctx);
}

static DeviceContext *
device_context_get (MbimDevice *device)
{
    DeviceContext *ctx;

    if (G_UNLIKELY (!device_context_quark))
        device_context_quark = g_quark_from_static_string (DEVICE_CONTEXT_TAG);

    ctx = g_object_get_qdata (G_OBJECT (device), device_context_quark);
    if (!ctx) {
        ctx = g_slice_new0 (DeviceContext);
        ctx->mbim_event_entry_array = _mbim_proxy_helper_service_subscribe_list_new_standard (&ctx->mbim_event_entry_array_size);

        g_debug ("[%s] initial device subscribe list...", mbim_device_get_path (device));
        _mbim_proxy_helper_service_subscribe_list_debug ((const MbimEventEntry * const *)ctx->mbim_event_entry_array, ctx->mbim_event_entry_array_size);

        g_object_set_qdata_full (G_OBJECT (device), device_context_quark, ctx, (GDestroyNotify)device_context_free);
    }

    return ctx;
}

static MbimEventEntry **
merge_client_service_subscribe_lists (MbimProxy  *self,
                                      MbimDevice *device,
                                      gsize      *out_size)
{
    GList                          *l;
    g_autoptr(MbimEventEntryArray)  updated = NULL;
    gsize                           updated_size = 0;
    DeviceContext                  *ctx;

    g_debug ("[%s] merging client service subscribe lists...", mbim_device_get_path (device));
    ctx = device_context_get (device);
    g_assert (ctx);

    g_assert (out_size != NULL);

    /* Init default list */
    updated = _mbim_proxy_helper_service_subscribe_list_new_standard (&updated_size);

    /* Lookup all clients with this device */
    for (l = self->priv->clients; l; l = g_list_next (l)) {
        Client *client;

        client = l->data;
        if (!client->mbim_event_entry_array)
            continue;

        /* Add per-client list */
        if (client->device == device)
            updated = _mbim_proxy_helper_service_subscribe_list_merge (updated, updated_size,
                                                                       client->mbim_event_entry_array, client->mbim_event_entry_array_size,
                                                                       &updated_size);
    }

    /* If lists are equal, ignore re-setting them up */
    if (_mbim_proxy_helper_service_subscribe_list_cmp (
            (const MbimEventEntry *const *)updated, updated_size,
            (const MbimEventEntry *const *)ctx->mbim_event_entry_array, ctx->mbim_event_entry_array_size)) {
        g_debug ("[%s] merged service subscribe list not updated", mbim_device_get_path (device));
        return NULL;
    }

    /* Lists are different, update stored one */
    g_clear_pointer (&ctx->mbim_event_entry_array, mbim_event_entry_array_free);
    ctx->mbim_event_entry_array = g_steal_pointer (&updated);
    ctx->mbim_event_entry_array_size = updated_size;

    if (mbim_utils_get_traces_enabled ()) {
        g_debug ("[%s] merged service subscribe list built", mbim_device_get_path (device));
        _mbim_proxy_helper_service_subscribe_list_debug ((const MbimEventEntry * const *)ctx->mbim_event_entry_array,
                                                         ctx->mbim_event_entry_array_size);
    }

    *out_size = ctx->mbim_event_entry_array_size;
    return ctx->mbim_event_entry_array;
}

static void
reset_client_service_subscribe_lists (MbimProxy  *self,
                                      MbimDevice *device)
{
    DeviceContext *ctx;
    GList         *l;

    g_debug ("[%s] reseting client service subscribe lists...", mbim_device_get_path (device));
    ctx = device_context_get (device);
    g_assert (ctx);

    /* make sure that all clients of this device don't track any event registered */
    for (l = self->priv->clients; l; l = g_list_next (l)) {
        Client *client;

        client = l->data;
        if (!client->mbim_event_entry_array)
            continue;

        if (client->device == device) {
            g_clear_pointer (&client->mbim_event_entry_array, mbim_event_entry_array_free);
            client->mbim_event_entry_array = _mbim_proxy_helper_service_subscribe_list_new_standard (&client->mbim_event_entry_array_size);
        }
    }

    /* And reset the device-specific merged list */
    g_clear_pointer (&ctx->mbim_event_entry_array, mbim_event_entry_array_free);
    ctx->mbim_event_entry_array = _mbim_proxy_helper_service_subscribe_list_new_standard (&ctx->mbim_event_entry_array_size);
}

static void
proxy_device_error_cb (MbimDevice *device,
                       GError     *error,
                       MbimProxy  *self)
{
    if (g_error_matches (error, MBIM_PROTOCOL_ERROR, MBIM_PROTOCOL_ERROR_NOT_OPENED)) {
        g_debug ("[%s] reports as being closed...", mbim_device_get_path (device));
        reset_client_service_subscribe_lists (self, device);
        mbim_device_close_force (device, NULL);
    }
}

static MbimDevice *
peek_device_for_path (MbimProxy   *self,
                      const gchar *path)
{
    GList *l;

    for (l = self->priv->devices; l; l = g_list_next (l)) {
        /* Return if found */
        if (g_str_equal (mbim_device_get_path ((MbimDevice *)l->data), path))
            return (MbimDevice *)l->data;
    }

    return NULL;
}

static void
proxy_device_removed_cb (MbimDevice *device,
                         MbimProxy  *self)
{
    untrack_device (self, device);
}

static void
untrack_device (MbimProxy  *self,
                MbimDevice *device)
{
    GList *l;
    GList *to_remove = NULL;

    g_debug ("[%s] untracking device...", mbim_device_get_path (device));

    if (!g_list_find (self->priv->devices, device))
        return;

    /* Disconnect right away */
    g_signal_handlers_disconnect_by_func (device, proxy_device_error_cb, self);
    g_signal_handlers_disconnect_by_func (device, proxy_device_removed_cb, self);

    /* If pending openings ongoing, complete them with error */
    cancel_opening_device (self, device);

    /* Lookup all clients with this device */
    for (l = self->priv->clients; l; l = g_list_next (l)) {
        if (((Client *)(l->data))->device == device)
            to_remove = g_list_append (to_remove, l->data);
    }

    /* Remove all these clients */
    for (l = to_remove; l; l = g_list_next (l))
        untrack_client (self, (Client *)(l->data));
    g_list_free (to_remove);

    /* And finally, remove the device */
    self->priv->devices = g_list_remove (self->priv->devices, device);
    g_object_unref (device);
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_N_DEVICES]);
}

static void
track_device (MbimProxy *self,
              MbimDevice *device)
{
    g_signal_connect (device,
                      MBIM_DEVICE_SIGNAL_REMOVED,
                      G_CALLBACK (proxy_device_removed_cb),
                      self);

    g_signal_connect (device,
                      MBIM_DEVICE_SIGNAL_ERROR,
                      G_CALLBACK (proxy_device_error_cb),
                      self);

    self->priv->devices = g_list_append (self->priv->devices, g_object_ref (device));
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_N_DEVICES]);
}

/*****************************************************************************/

MbimProxy *
mbim_proxy_new (GError **error)
{
    g_autoptr(MbimProxy) self = NULL;

    if (!mbim_helpers_check_user_allowed (getuid(), error))
        return NULL;

    self = g_object_new (MBIM_TYPE_PROXY, NULL);
    if (!setup_socket_service (self, error))
        return NULL;

    return g_steal_pointer (&self);
}

static void
mbim_proxy_init (MbimProxy *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, MBIM_TYPE_PROXY, MbimProxyPrivate);
}

static void
get_property (GObject    *object,
              guint       prop_id,
              GValue     *value,
              GParamSpec *pspec)
{
    MbimProxy *self = MBIM_PROXY (object);

    switch (prop_id) {
    case PROP_N_CLIENTS:
        g_value_set_uint (value, g_list_length (self->priv->clients));
        break;
    case PROP_N_DEVICES:
        g_value_set_uint (value, g_list_length (self->priv->devices));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
dispose (GObject *object)
{
    MbimProxyPrivate *priv = MBIM_PROXY (object)->priv;

    /* This list should always be empty when disposing */
    g_assert (priv->opening_devices == NULL);

    if (priv->clients) {
        g_list_free_full (priv->clients, (GDestroyNotify) client_unref);
        priv->clients = NULL;
    }

    if (priv->devices) {
        g_list_free_full (priv->devices, g_object_unref);
        priv->devices = NULL;
    }

    if (priv->socket_service) {
        if (g_socket_service_is_active (priv->socket_service))
            g_socket_service_stop (priv->socket_service);
        g_clear_object (&priv->socket_service);
        g_unlink (MBIM_PROXY_SOCKET_PATH);
        g_debug ("UNIX socket service at '%s' stopped", MBIM_PROXY_SOCKET_PATH);
    }

    G_OBJECT_CLASS (mbim_proxy_parent_class)->dispose (object);
}

static void
mbim_proxy_class_init (MbimProxyClass *proxy_class)
{
    GObjectClass *object_class = G_OBJECT_CLASS (proxy_class);

    g_type_class_add_private (object_class, sizeof (MbimProxyPrivate));

    /* Virtual methods */
    object_class->get_property = get_property;
    object_class->dispose = dispose;

    /**
     * MbimProxy:mbim-proxy-n-clients
     *
     * Since: 1.10
     */
    properties[PROP_N_CLIENTS] =
        g_param_spec_uint (MBIM_PROXY_N_CLIENTS,
                           "Number of clients",
                           "Number of clients currently connected to the proxy",
                           0,
                           G_MAXUINT,
                           0,
                           G_PARAM_READABLE);
    g_object_class_install_property (object_class, PROP_N_CLIENTS, properties[PROP_N_CLIENTS]);

    /**
     * MbimProxy:mbim-proxy-n-devices
     *
     * Since: 1.10
     */
    properties[PROP_N_DEVICES] =
        g_param_spec_uint (MBIM_PROXY_N_DEVICES,
                           "Number of devices",
                           "Number of devices currently managed by the proxy",
                           0,
                           G_MAXUINT,
                           0,
                           G_PARAM_READABLE);
    g_object_class_install_property (object_class, PROP_N_DEVICES, properties[PROP_N_DEVICES]);
}
