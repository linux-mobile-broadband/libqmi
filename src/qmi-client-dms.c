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

#include "qmi-enum-types.h"
#include "qmi-client-dms.h"
#include "qmi-message-dms.h"

G_DEFINE_TYPE (QmiClientDms, qmi_client_dms, QMI_TYPE_CLIENT);

/*****************************************************************************/
/* Get IDs */

QmiDmsGetIdsResult *
qmi_client_dms_get_ids_finish (QmiClientDms *self,
                               GAsyncResult *res,
                               GError **error)
{
    if (g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (res), error))
        return NULL;

    return qmi_dms_get_ids_result_ref (g_simple_async_result_get_op_res_gpointer (G_SIMPLE_ASYNC_RESULT (res)));
}

static void
get_ids_ready (QmiDevice *device,
               GAsyncResult *res,
               GSimpleAsyncResult *simple)
{
    QmiDmsGetIdsResult *result;
    GError *error = NULL;
    QmiMessage *reply;

    reply = qmi_device_command_finish (device, res, &error);
    if (!reply) {
        g_prefix_error (&error, "Getting IDs failed: ");
        g_simple_async_result_take_error (simple, error);
        g_simple_async_result_complete (simple);
        g_object_unref (simple);
        return;
    }

    /* Parse reply */
    result = qmi_message_dms_get_ids_reply_parse (reply, &error);
    if (!result) {
        g_prefix_error (&error, "Getting IDs reply parsing failed: ");
        g_simple_async_result_take_error (simple, error);
    } else
        g_simple_async_result_set_op_res_gpointer (simple,
                                                   result,
                                                   (GDestroyNotify)qmi_dms_get_ids_result_unref);
    g_simple_async_result_complete (simple);
    g_object_unref (simple);
}

void
qmi_client_dms_get_ids (QmiClientDms *self,
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
                                        qmi_client_dms_get_ids);

    request = qmi_message_dms_get_ids_new (qmi_client_get_next_transaction_id (QMI_CLIENT (self)),
                                           qmi_client_get_cid (QMI_CLIENT (self)));
    qmi_device_command (qmi_client_peek_device (QMI_CLIENT (self)),
                        request,
                        timeout,
                        cancellable,
                        (GAsyncReadyCallback)get_ids_ready,
                        result);
    qmi_message_unref (request);
}

/*****************************************************************************/
/* New DMS client */

/**
 * qmi_device_new_finish:
 * @res: a #GAsyncResult.
 * @error: a #GError.
 *
 * Finishes an operation started with qmi_device_new().
 *
 * Returns: A newly created #QmiDevice, or #NULL if @error is set.
 */
QmiClientDms *
qmi_client_dms_new_finish (GAsyncResult *res,
                           GError **error)
{
  GObject *ret;
  GObject *source_object;

  source_object = g_async_result_get_source_object (res);
  ret = g_async_initable_new_finish (G_ASYNC_INITABLE (source_object), res, error);
  g_object_unref (source_object);

  return (ret ? QMI_CLIENT_DMS (ret) : NULL);
}

void
qmi_client_dms_new (QmiDevice *device,
                    GCancellable *cancellable,
                    GAsyncReadyCallback callback,
                    gpointer user_data)
{
    g_async_initable_new_async (QMI_TYPE_CLIENT_DMS,
                                G_PRIORITY_DEFAULT,
                                cancellable,
                                callback,
                                user_data,
                                QMI_CLIENT_DEVICE, device,
                                QMI_CLIENT_SERVICE, QMI_SERVICE_DMS,
                                NULL);
}

/*****************************************************************************/

static void
qmi_client_dms_init (QmiClientDms *self)
{
}

static void
qmi_client_dms_class_init (QmiClientDmsClass *klass)
{
}
