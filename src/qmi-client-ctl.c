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

#include "qmi-error-types.h"
#include "qmi-client-ctl.h"

G_DEFINE_TYPE (QmiClientCtl, qmi_client_ctl, QMI_TYPE_CLIENT);

/*****************************************************************************/
/* Get version info */

/**
 * qmi_client_ctl_get_version_info_finish:
 * @res: a #GAsyncResult.
 * @error: a #GError.
 *
 * Finishes an operation started with qmi_client_ctl_get_version_info().
 *
 * Returns: A #GArray of #QmiCtlVersionInfo, or #NULL if @error is set.
 */
GArray *
qmi_client_ctl_get_version_info_finish (QmiClientCtl *self,
                                        GAsyncResult *res,
                                        GError **error)
{
    if (g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (res), error))
        return NULL;

    return g_array_ref (g_simple_async_result_get_op_res_gpointer (G_SIMPLE_ASYNC_RESULT (res)));
}

static void
version_info_ready (QmiDevice *device,
                    GAsyncResult *res,
                    GSimpleAsyncResult *simple)
{
    GError *error = NULL;
    QmiMessage *reply;
    GArray *services;

    reply = qmi_device_command_finish (device, res, &error);
    if (!reply) {
        g_prefix_error (&error, "Version info check failed: ");
        g_simple_async_result_take_error (simple, error);
        g_simple_async_result_complete (simple);
        g_object_unref (simple);
        return;
    }

    /* Parse version reply */
    services = qmi_message_ctl_version_info_reply_parse (reply, &error);
    if (!services) {
        g_prefix_error (&error, "Version info reply parsing failed: ");
        g_simple_async_result_take_error (simple, error);
    } else
        g_simple_async_result_set_op_res_gpointer (simple,
                                                   services,
                                                   (GDestroyNotify)g_array_unref);

    g_simple_async_result_complete (simple);
    g_object_unref (simple);
}

/**
 * qmi_client_ctl_get_version_info:
 * @self: a #QmiClientCtl.
 * @timeout: maximum time to wait to get the operation completed.
 * @cancellable: optional #GCancellable object, #NULL to ignore.
 * @callback: a #GAsyncReadyCallback to call when the operation is finished.
 * @user_data: the data to pass to callback function.
 *
 * Get list of supported services.
 * When the query is finished, @callback will be called. You can then call
 * qmi_client_ctl_get_version_info_finish() to get the the result of the operation.
 */
void
qmi_client_ctl_get_version_info (QmiClientCtl *self,
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
                                        qmi_client_ctl_get_version_info);

    request = qmi_message_ctl_version_info_new (qmi_client_get_next_transaction_id (QMI_CLIENT (self)));
    qmi_device_command (qmi_client_peek_device (QMI_CLIENT (self)),
                        request,
                        timeout,
                        cancellable,
                        (GAsyncReadyCallback)version_info_ready,
                        result);
    qmi_message_unref (request);
}

/*****************************************************************************/

static void
qmi_client_ctl_init (QmiClientCtl *self)
{
}

static void
qmi_client_ctl_class_init (QmiClientCtlClass *klass)
{
}
