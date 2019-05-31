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

#include <string.h>

#include <libmbim-glib.h>

#include "qmi-endpoint-mbim.h"
#include "qmi-errors.h"
#include "qmi-error-types.h"
#include "qmi-file.h"

G_DEFINE_TYPE (QmiEndpointMbim, qmi_endpoint_mbim, QMI_TYPE_ENDPOINT)

struct _QmiEndpointMbimPrivate {
    MbimDevice *mbimdev;
    guint mbim_notification_id;
    guint mbim_removed_id;
};

/*
 * Number of extra seconds to give the MBIM timeout delay.
 * Needed so the QMI timeout triggers first and we can be sure
 * that timeouts on the QMI side are not because of libmbim timeouts.
 */
#define MBIM_TIMEOUT_DELAY_SECS 1

/*****************************************************************************/

static void
mbim_device_removed_cb (MbimDevice *device,
                        QmiEndpointMbim *self)
{
    g_signal_emit_by_name (QMI_ENDPOINT (self), QMI_ENDPOINT_SIGNAL_HANGUP);
}

/*****************************************************************************/

static void
mbim_device_command_ready (MbimDevice      *dev,
                           GAsyncResult    *res,
                           QmiEndpointMbim *self)
{
    MbimMessage *response;
    const guint8 *buf;
    guint32 len;
    GError *error = NULL;

    response = mbim_device_command_finish (dev, res, &error);
    if (!response || !mbim_message_response_get_result (response, MBIM_MESSAGE_TYPE_COMMAND_DONE, &error)) {
        g_warning ("[%s] MBIM error: %s", qmi_endpoint_get_name (QMI_ENDPOINT (self)), error->message);

        if (response)
            mbim_message_unref (response);
        g_object_unref (self);
        g_error_free (error);
        return;
    }

    /* Store the raw information buffer in the internal reception buffer,
     * as if we had read from a iochannel. */
    buf = mbim_message_command_done_get_raw_information_buffer (response, &len);
    qmi_endpoint_add_message (QMI_ENDPOINT (self), buf, len);
    mbim_message_unref (response);
    g_object_unref (self);
}

static void
mbim_qmi_notification_cb (MbimDevice      *device,
                          MbimMessage     *notification,
                          QmiEndpointMbim *self)
{
    MbimService   service;
    const guint8 *buf;
    guint32       len;

    service = mbim_message_indicate_status_get_service (notification);
    if (service != MBIM_SERVICE_QMI)
        return;

    buf = mbim_message_indicate_status_get_raw_information_buffer (notification, &len);
    qmi_endpoint_add_message (QMI_ENDPOINT (self), buf, len);
}

static void
mbim_subscribe_list_set_ready_cb (MbimDevice   *device,
                                  GAsyncResult *res,
                                  GTask        *task)
{
    QmiEndpointMbim   *self;
    MbimMessage       *response;
    GError            *error = NULL;

    self = g_task_get_source_object (task);

    response = mbim_device_command_finish (device, res, &error);
    if (response) {
        mbim_message_response_get_result (response, MBIM_MESSAGE_TYPE_COMMAND_DONE, &error);
        mbim_message_unref (response);
    }

    if (error) {
        g_warning ("[%s] couldn't enable QMI indications via MBIM: %s",
                   qmi_endpoint_get_name (QMI_ENDPOINT (self)), error->message);
        g_task_return_error (task, error);
        g_object_unref (task);
        return;
    }

    g_debug ("[%s] enabled QMI indications via MBIM", qmi_endpoint_get_name (QMI_ENDPOINT (self)));
    self->priv->mbim_notification_id = g_signal_connect (device,
                                                         MBIM_DEVICE_SIGNAL_INDICATE_STATUS,
                                                         G_CALLBACK (mbim_qmi_notification_cb),
                                                         self);
    g_task_return_boolean (task, TRUE);
    g_object_unref (task);
}

/*****************************************************************************/

typedef struct {
    gboolean use_proxy;
    guint    timeout;
} MbimDeviceOpenContext;

static void
mbim_device_open_context_free (MbimDeviceOpenContext *ctx)
{
    g_slice_free (MbimDeviceOpenContext, ctx);
}

static gboolean
endpoint_open_finish (QmiEndpoint   *self,
                      GAsyncResult  *res,
                      GError       **error)
{
    return g_task_propagate_boolean (G_TASK (res), error);
}

static void
mbim_device_open_ready (MbimDevice   *dev,
                        GAsyncResult *res,
                        GTask        *task)
{
    QmiEndpointMbim *self;
    GError *error = NULL;

    if (!mbim_device_open_finish (dev, res, &error)) {
        g_task_return_error (task, error);
        g_object_unref (task);
        return;
    }

    self = g_task_get_source_object (task);
    g_debug ("[%s] MBIM device open", qmi_endpoint_get_name (QMI_ENDPOINT (self)));
    g_task_return_boolean (task, TRUE);
    g_object_unref (task);
}

static void
mbim_device_new_ready (GObject *source,
                       GAsyncResult *res,
                       GTask *task)
{
    QmiEndpointMbim *self;
    MbimDeviceOpenContext *ctx;
    MbimDeviceOpenFlags open_flags = MBIM_DEVICE_OPEN_FLAGS_NONE;
    GError *error = NULL;

    self = g_task_get_source_object (task);
    ctx = g_task_get_task_data (task);

    self->priv->mbimdev = mbim_device_new_finish (res, &error);
    if (!self->priv->mbimdev) {
        g_task_return_error (task, error);
        g_object_unref (task);
        return;
    }

    g_debug ("[%s] MBIM device created", qmi_endpoint_get_name (QMI_ENDPOINT (self)));

    self->priv->mbim_removed_id =
        g_signal_connect (self->priv->mbimdev,
                          MBIM_DEVICE_SIGNAL_REMOVED,
                          G_CALLBACK (mbim_device_removed_cb),
                          self);

    /* If QMI proxy was requested, use MBIM proxy instead */
    if (ctx->use_proxy)
        open_flags |= MBIM_DEVICE_OPEN_FLAGS_PROXY;

    /* We pass the original timeout of the request to the open operation */
    g_debug ("[%s] opening MBIM device...", qmi_endpoint_get_name (QMI_ENDPOINT (self)));
    mbim_device_open_full (self->priv->mbimdev,
                           open_flags,
                           ctx->timeout,
                           g_task_get_cancellable (task),
                           (GAsyncReadyCallback) mbim_device_open_ready,
                           task);
}

static void
endpoint_open (QmiEndpoint         *endpoint,
               gboolean             use_proxy,
               guint                timeout,
               GCancellable        *cancellable,
               GAsyncReadyCallback  callback,
               gpointer             user_data)
{
    QmiEndpointMbim *self;
    MbimDeviceOpenContext *ctx;
    QmiFile *qfile;
    GFile *file;
    GTask *task;

    self = QMI_ENDPOINT_MBIM (endpoint);
    task = g_task_new (self, cancellable, callback, user_data);

    ctx = g_slice_new0 (MbimDeviceOpenContext);
    ctx->use_proxy = use_proxy;
    ctx->timeout = timeout;
    g_task_set_task_data (task, ctx, (GDestroyNotify)mbim_device_open_context_free);

    if (self->priv->mbimdev) {
        g_task_return_new_error (task,
                                 QMI_CORE_ERROR,
                                 QMI_CORE_ERROR_WRONG_STATE,
                                 "Already open");
        g_object_unref (task);
        return;
    }

    g_object_get (self, QMI_ENDPOINT_FILE, &qfile, NULL);
    g_debug ("[%s] creating MBIM device...", qmi_endpoint_get_name (QMI_ENDPOINT (self)));
    file = g_file_new_for_path (qmi_file_get_path (qfile));
    g_object_unref (qfile);

    mbim_device_new (file,
                     g_task_get_cancellable (task),
                     (GAsyncReadyCallback) mbim_device_new_ready,
                     task);
    g_object_unref (file);
}

static gboolean
endpoint_is_open (QmiEndpoint *self)
{
    return !!(QMI_ENDPOINT_MBIM (self)->priv->mbimdev);
}

static gboolean
endpoint_setup_indications_finish (QmiEndpoint   *self,
                                   GAsyncResult  *res,
                                   GError       **error)
{
    return g_task_propagate_boolean (G_TASK (res), error);
}

static void
endpoint_setup_indications (QmiEndpoint         *self,
                            guint                timeout,
                            GCancellable        *cancellable,
                            GAsyncReadyCallback  callback,
                            gpointer             user_data)
{
    GTask *task;
    MbimEventEntry **entries;
    guint n_entries = 0;
    MbimMessage *request;

    task = g_task_new (self, cancellable, callback, user_data);

    g_debug ("[%s] Enabling QMI indications via MBIM...", qmi_endpoint_get_name (QMI_ENDPOINT (self)));

    entries = g_new0 (MbimEventEntry *, 2);
    entries[n_entries] = g_new (MbimEventEntry, 1);
    memcpy (&(entries[n_entries]->device_service_id), MBIM_UUID_QMI, sizeof (MbimUuid));
    entries[n_entries]->cids_count = 1;
    entries[n_entries]->cids = g_new0 (guint32, 1);
    entries[n_entries]->cids[0] = MBIM_CID_QMI_MSG;
    n_entries++;

    request = mbim_message_device_service_subscribe_list_set_new (
                  n_entries,
                  (const MbimEventEntry *const *)entries,
                  NULL);
    mbim_device_command (QMI_ENDPOINT_MBIM (self)->priv->mbimdev,
                         request,
                         10,
                         NULL,
                         (GAsyncReadyCallback)mbim_subscribe_list_set_ready_cb,
                         task);
    mbim_message_unref (request);
    mbim_event_entry_array_free (entries);
}

static gboolean
endpoint_send (QmiEndpoint   *self,
               QmiMessage    *message,
               guint          timeout,
               GCancellable  *cancellable,
               GError       **error)
{
    MbimMessage *mbim_message;
    gconstpointer raw_message;
    gsize raw_message_len;
    GError *inner_error = NULL;

    /* Get raw message */
    raw_message = qmi_message_get_raw (message, &raw_message_len, &inner_error);
    if (!raw_message) {
        g_propagate_prefixed_error (error, inner_error, "Cannot get raw message: ");
        return FALSE;
    }

    mbim_message = mbim_message_qmi_msg_set_new (raw_message_len, raw_message, &inner_error);
    if (!mbim_message) {
        g_propagate_error (error, inner_error);
        return FALSE;
    }

    /* Note:
     *
     * Pass a full reference to the QMI endpoint to the MBIM command
     * operation, so that we make sure the parent object is valid regardless
     * of when the underlying device is fully disposed. This is required
     * because device close is async().
     */
    mbim_device_command (QMI_ENDPOINT_MBIM (self)->priv->mbimdev,
                         mbim_message,
                         timeout + MBIM_TIMEOUT_DELAY_SECS,
                         cancellable,
                         (GAsyncReadyCallback) mbim_device_command_ready,
                         g_object_ref (self));

    mbim_message_unref (mbim_message);
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
mbim_device_close_ready (MbimDevice   *dev,
                         GAsyncResult *res,
                         GTask        *task)
{
    GError *error = NULL;

    if (!mbim_device_close_finish (dev, res, &error))
        g_task_return_error (task, error);
    else
        g_task_return_boolean (task, TRUE);
    g_object_unref (task);
}

static void
endpoint_close (QmiEndpoint         *endpoint,
                guint                timeout,
                GCancellable        *cancellable,
                GAsyncReadyCallback  callback,
                gpointer             user_data)
{
    GTask *task;
    QmiEndpointMbim *self;

    self = QMI_ENDPOINT_MBIM (endpoint);
    task = g_task_new (self, cancellable, callback, user_data);

    if (!self->priv->mbimdev) {
        g_task_return_boolean (task, TRUE);
        g_object_unref (task);
    }

    /* Schedule in new main context */
    mbim_device_close (self->priv->mbimdev,
                       timeout,
                       NULL,
                       (GAsyncReadyCallback) mbim_device_close_ready,
                       task);
    /* Cleanup right away, we don't want multiple close attempts on the
     * device */
    if (self->priv->mbim_notification_id) {
        g_signal_handler_disconnect (self->priv->mbimdev, self->priv->mbim_notification_id);
        self->priv->mbim_notification_id = 0;
    }
    if (self->priv->mbim_removed_id) {
        g_signal_handler_disconnect (self->priv->mbimdev, self->priv->mbim_removed_id);
        self->priv->mbim_removed_id = 0;
    }
    g_clear_object (&self->priv->mbimdev);
}

/*****************************************************************************/

QmiEndpointMbim *
qmi_endpoint_mbim_new (QmiFile *file)
{
    if (!file)
        return NULL;

    return g_object_new (QMI_TYPE_ENDPOINT_MBIM,
                         QMI_ENDPOINT_FILE, file,
                         NULL);
}

/*****************************************************************************/

static void
qmi_endpoint_mbim_init (QmiEndpointMbim *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE ((self),
                                              QMI_TYPE_ENDPOINT_MBIM,
                                              QmiEndpointMbimPrivate);
}

static void
dispose (GObject *object)
{
    QmiEndpointMbim *self = QMI_ENDPOINT_MBIM (object);

    if (self->priv->mbimdev) {
        g_warning ("[%s] MBIM device wasn't explicitly closed",
                   qmi_endpoint_get_name (QMI_ENDPOINT (self)));

        if (self->priv->mbim_notification_id) {
            g_signal_handler_disconnect (self->priv->mbimdev, self->priv->mbim_notification_id);
            self->priv->mbim_notification_id = 0;
        }
        if (self->priv->mbim_removed_id) {
            g_signal_handler_disconnect (self->priv->mbimdev, self->priv->mbim_removed_id);
            self->priv->mbim_removed_id = 0;
        }
        g_clear_object (&self->priv->mbimdev);
    }

    G_OBJECT_CLASS (qmi_endpoint_mbim_parent_class)->dispose (object);
}

static void
qmi_endpoint_mbim_class_init (QmiEndpointMbimClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    QmiEndpointClass *endpoint_class = QMI_ENDPOINT_CLASS (klass);

    g_type_class_add_private (object_class, sizeof (QmiEndpointMbimPrivate));

    object_class->dispose = dispose;

    endpoint_class->open = endpoint_open;
    endpoint_class->open_finish = endpoint_open_finish;
    endpoint_class->is_open = endpoint_is_open;
    endpoint_class->setup_indications = endpoint_setup_indications;
    endpoint_class->setup_indications_finish = endpoint_setup_indications_finish;
    endpoint_class->send = endpoint_send;
    endpoint_class->close = endpoint_close;
    endpoint_class->close_finish = endpoint_close_finish;
}
