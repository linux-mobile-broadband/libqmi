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

#include "qmi-client-wds.h"
#include "qmi-message-wds.h"

G_DEFINE_TYPE (QmiClientWds, qmi_client_wds, QMI_TYPE_CLIENT);

/*****************************************************************************/
/* Start network */

guint32
qmi_client_wds_start_network_finish (QmiClientWds *self,
                                     GAsyncResult *res,
                                     GError **error)
{
    if (g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (res), error))
        return 0; /* TODO: what would be a good invalid packet data handle? */

    return (guint32) GPOINTER_TO_UINT (g_simple_async_result_get_op_res_gpointer (G_SIMPLE_ASYNC_RESULT (res)));
}

static void
start_network_ready (QmiDevice *device,
                     GAsyncResult *res,
                     GSimpleAsyncResult *simple)
{
    GError *error = NULL;
    QmiMessage *reply;
    guint32 result;

    reply = qmi_device_command_finish (device, res, &error);
    if (!reply) {
        g_simple_async_result_take_error (simple, error);
        g_simple_async_result_complete (simple);
        g_object_unref (simple);
        return;
    }

    /* Parse reply */
    result = qmi_message_wds_start_network_reply_parse (reply, &error);
    if (!result)
        g_simple_async_result_take_error (simple, error);
    else
        g_simple_async_result_set_op_res_gpointer (simple,
                                                   GUINT_TO_POINTER (result),
                                                   NULL);
    g_simple_async_result_complete (simple);
    g_object_unref (simple);
}

void
qmi_client_wds_start_network (QmiClientWds *self,
                              guint timeout,
                              GCancellable *cancellable,
                              GAsyncReadyCallback callback,
                              gpointer user_data)
{
    GSimpleAsyncResult *result;
    QmiMessage *request;

    result = g_simple_async_result_new (G_OBJECT (self),
                                        callback,
                                        user_data,
                                        qmi_client_wds_start_network);

    request = qmi_message_wds_start_network_new (qmi_client_get_next_transaction_id (QMI_CLIENT (self)),
                                                 qmi_client_get_cid (QMI_CLIENT (self)));
    qmi_device_command (qmi_client_peek_device (QMI_CLIENT (self)),
                        request,
                        timeout,
                        cancellable,
                        (GAsyncReadyCallback)start_network_ready,
                        result);
    qmi_message_unref (request);
}

/*****************************************************************************/
/* Stop network */

gboolean
qmi_client_wds_stop_network_finish (QmiClientWds *self,
                                    GAsyncResult *res,
                                    GError **error)
{
    return !g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (res), error);
}

static void
stop_network_ready (QmiDevice *device,
                    GAsyncResult *res,
                    GSimpleAsyncResult *simple)
{
    GError *error = NULL;
    QmiMessage *reply;

    reply = qmi_device_command_finish (device, res, &error);
    if (!reply) {
        g_simple_async_result_take_error (simple, error);
        g_simple_async_result_complete (simple);
        g_object_unref (simple);
        return;
    }

    /* Parse reply */
    if (!qmi_message_wds_stop_network_reply_parse (reply, &error))
        g_simple_async_result_take_error (simple, error);
    else
        g_simple_async_result_set_op_res_gboolean (simple, TRUE);
    g_simple_async_result_complete (simple);
    g_object_unref (simple);
}

void
qmi_client_wds_stop_network (QmiClientWds *self,
                             guint32 packet_data_handle,
                             guint timeout,
                             GCancellable *cancellable,
                             GAsyncReadyCallback callback,
                             gpointer user_data)
{
    GSimpleAsyncResult *result;
    QmiMessage *request;

    result = g_simple_async_result_new (G_OBJECT (self),
                                        callback,
                                        user_data,
                                        qmi_client_wds_stop_network);

    request = qmi_message_wds_stop_network_new (qmi_client_get_next_transaction_id (QMI_CLIENT (self)),
                                                qmi_client_get_cid (QMI_CLIENT (self)),
                                                packet_data_handle);
    qmi_device_command (qmi_client_peek_device (QMI_CLIENT (self)),
                        request,
                        timeout,
                        cancellable,
                        (GAsyncReadyCallback)stop_network_ready,
                        result);
    qmi_message_unref (request);
}

/*****************************************************************************/

static void
qmi_client_wds_init (QmiClientWds *self)
{
}

static void
qmi_client_wds_class_init (QmiClientWdsClass *klass)
{
}
