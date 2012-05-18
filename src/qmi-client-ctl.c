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
#include "qmi-enum-types.h"
#include "qmi-device.h"
#include "qmi-client-ctl.h"
#include "qmi-message-ctl.h"

G_DEFINE_TYPE (QmiClientCtl, qmi_client_ctl, QMI_TYPE_CLIENT)

/*****************************************************************************/
/* Get version info */

/**
 * qmi_client_ctl_get_version_info_finish:
 * @res: a #GAsyncResult.
 * @error: a #GError.
 *
 * Finishes an operation started with qmi_client_ctl_get_version_info().
 *
 * Returns: A #GPtrArray of #QmiCtlVersionInfo, or #NULL if @error is set.
 */
GPtrArray *
qmi_client_ctl_get_version_info_finish (QmiClientCtl *self,
                                        GAsyncResult *res,
                                        GError **error)
{
    if (g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (res), error))
        return NULL;

    return g_ptr_array_ref (g_simple_async_result_get_op_res_gpointer (G_SIMPLE_ASYNC_RESULT (res)));
}

static void
version_info_ready (QmiDevice *device,
                    GAsyncResult *res,
                    GSimpleAsyncResult *simple)
{
    GError *error = NULL;
    QmiMessage *reply;
    GPtrArray *result;

    reply = qmi_device_command_finish (device, res, &error);
    if (!reply) {
        g_prefix_error (&error, "Version info check failed: ");
        g_simple_async_result_take_error (simple, error);
        g_simple_async_result_complete (simple);
        g_object_unref (simple);
        return;
    }

    /* Parse version reply */
    result = qmi_message_ctl_version_info_reply_parse (reply, &error);
    if (!result) {
        g_prefix_error (&error, "Version info reply parsing failed: ");
        g_simple_async_result_take_error (simple, error);
    } else
        g_simple_async_result_set_op_res_gpointer (simple,
                                                   result,
                                                   (GDestroyNotify)g_ptr_array_unref);

    g_simple_async_result_complete (simple);
    g_object_unref (simple);
    qmi_message_unref (reply);
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
    qmi_device_command (QMI_DEVICE (qmi_client_peek_device (QMI_CLIENT (self))),
                        request,
                        timeout,
                        cancellable,
                        (GAsyncReadyCallback)version_info_ready,
                        result);
    qmi_message_unref (request);
}

/*****************************************************************************/
/* Allocate CID */

typedef struct {
    QmiClientCtl *self;
    GSimpleAsyncResult *result;
    QmiService service;
} AllocateCidContext;

static void
allocate_cid_context_complete_and_free (AllocateCidContext *ctx)
{
    g_simple_async_result_complete (ctx->result);
    g_object_unref (ctx->result);
    g_object_unref (ctx->self);
    g_slice_free (AllocateCidContext, ctx);
}

/**
 * qmi_client_ctl_allocate_cid_finish:
 * @self: a #QmiClientCtl.
 * @res: a #GAsyncResult.
 * @error: a #GError.
 *
 * Finishes an operation started with qmi_client_ctl_allocate_cid().
 *
 * Returns: the new CID, or 0 if @error is set.
 */
guint8
qmi_client_ctl_allocate_cid_finish (QmiClientCtl *self,
                                    GAsyncResult *res,
                                    GError **error)
{
    if (g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (res), error))
        return 0;

    return (guint8) GPOINTER_TO_UINT (g_simple_async_result_get_op_res_gpointer (G_SIMPLE_ASYNC_RESULT (res)));
}

static void
allocate_cid_ready (QmiDevice *device,
                    GAsyncResult *res,
                    AllocateCidContext *ctx)
{
    GError *error = NULL;
    QmiMessage *reply;
    guint8 cid;
    QmiService service;

    reply = qmi_device_command_finish (device, res, &error);
    if (!reply) {
        g_prefix_error (&error, "CID allocation failed: ");
        g_simple_async_result_take_error (ctx->result, error);
        allocate_cid_context_complete_and_free (ctx);
        return;
    }

    /* Parse reply */
    cid = 0;
    service = QMI_SERVICE_UNKNOWN;
    if (!qmi_message_ctl_allocate_cid_reply_parse (reply, &cid, &service, &error)) {
        g_prefix_error (&error, "CID allocation reply parsing failed: ");
        g_simple_async_result_take_error (ctx->result, error);
        allocate_cid_context_complete_and_free (ctx);
        qmi_message_unref (reply);
        return;
    }

    /* The service we got must match the one we requested */
    if (service != ctx->service) {
        g_simple_async_result_set_error (ctx->result,
                                         QMI_CORE_ERROR,
                                         QMI_CORE_ERROR_FAILED,
                                         "Service mismatch (%s vs %s)",
                                         qmi_service_get_string (service),
                                         qmi_service_get_string (ctx->service));
        allocate_cid_context_complete_and_free (ctx);
        qmi_message_unref (reply);
        return;
    }

    g_debug ("Allocated client ID '%u' for service '%s'",
             cid,
             qmi_service_get_string (ctx->service));

    /* Set the CID as result */
    g_simple_async_result_set_op_res_gpointer (ctx->result,
                                               GUINT_TO_POINTER ((guint)cid),
                                               NULL);
    allocate_cid_context_complete_and_free (ctx);
    qmi_message_unref (reply);
}

/**
 * qmi_client_ctl_allocate_cid:
 * @self: a #QmiClientCtl.
 * @service: a #QmiService.
 * @timeout: maximum time to wait to get the operation completed.
 * @cancellable: optional #GCancellable object, #NULL to ignore.
 * @callback: a #GAsyncReadyCallback to call when the operation is finished.
 * @user_data: the data to pass to callback function.
 *
 * Allocate a new client ID for the given @service..
 * When the query is finished, @callback will be called. You can then call
 * qmi_client_ctl_allocate_cid_finish() to get the the result of the operation.
 */
void
qmi_client_ctl_allocate_cid (QmiClientCtl *self,
                             QmiService service,
                             guint timeout,
                             GCancellable *cancellable,
                             GAsyncReadyCallback callback,
                             gpointer user_data)
{
    AllocateCidContext *ctx;
    QmiMessage *request;

    ctx = g_slice_new (AllocateCidContext);
    ctx->self = g_object_ref (self);
    ctx->service = service;
    ctx->result = g_simple_async_result_new (G_OBJECT (self),
                                             callback,
                                             user_data,
                                             qmi_client_ctl_allocate_cid);

    request = qmi_message_ctl_allocate_cid_new (qmi_client_get_next_transaction_id (QMI_CLIENT (self)),
                                                service);
    qmi_device_command (QMI_DEVICE (qmi_client_peek_device (QMI_CLIENT (self))),
                        request,
                        timeout,
                        cancellable,
                        (GAsyncReadyCallback)allocate_cid_ready,
                        ctx);
    qmi_message_unref (request);
}

/*****************************************************************************/
/* Release CID */

typedef struct {
    QmiClientCtl *self;
    GSimpleAsyncResult *result;
    QmiService service;
    guint8 cid;
} ReleaseCidContext;

static void
release_cid_context_complete_and_free (ReleaseCidContext *ctx)
{
    g_simple_async_result_complete (ctx->result);
    g_object_unref (ctx->result);
    g_object_unref (ctx->self);
    g_slice_free (ReleaseCidContext, ctx);
}

/**
 * qmi_client_ctl_release_cid_finish:
 * @self: a #QmiClientCtl.
 * @res: a #GAsyncResult.
 * @error: a #GError.
 *
 * Finishes an operation started with qmi_client_ctl_release_cid().
 *
 * Returns: #TRUE if the operation succeeded, or #FALSE if @error is set.
 */
gboolean
qmi_client_ctl_release_cid_finish (QmiClientCtl *self,
                                   GAsyncResult *res,
                                   GError **error)
{
    return !g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (res), error);
}

static void
release_cid_ready (QmiDevice *device,
                   GAsyncResult *res,
                   ReleaseCidContext *ctx)
{
    GError *error = NULL;
    QmiMessage *reply;
    guint8 cid;
    QmiService service;

    reply = qmi_device_command_finish (device, res, &error);
    if (!reply) {
        g_prefix_error (&error, "CID release failed: ");
        g_simple_async_result_take_error (ctx->result, error);
        release_cid_context_complete_and_free (ctx);
        return;
    }

    /* Parse reply */
    cid = 0;
    service = QMI_SERVICE_UNKNOWN;
    if (!qmi_message_ctl_release_cid_reply_parse (reply, &cid, &service, &error)) {
        g_prefix_error (&error, "CID release reply parsing failed: ");
        g_simple_async_result_take_error (ctx->result, error);
        release_cid_context_complete_and_free (ctx);
        qmi_message_unref (reply);
        return;
    }

    /* The service we got must match the one we requested */
    if (service != ctx->service) {
        g_simple_async_result_set_error (ctx->result,
                                         QMI_CORE_ERROR,
                                         QMI_CORE_ERROR_FAILED,
                                         "Service mismatch (%s vs %s)",
                                         qmi_service_get_string (service),
                                         qmi_service_get_string (ctx->service));
        release_cid_context_complete_and_free (ctx);
        qmi_message_unref (reply);
        return;
    }

    /* The cid we got must match the one we requested */
    if (cid != ctx->cid) {
        g_simple_async_result_set_error (ctx->result,
                                         QMI_CORE_ERROR,
                                         QMI_CORE_ERROR_FAILED,
                                         "CID mismatch (%s vs %s)",
                                         qmi_service_get_string (service),
                                         qmi_service_get_string (ctx->service));
        release_cid_context_complete_and_free (ctx);
        qmi_message_unref (reply);
        return;
    }

    g_debug ("Released client ID '%u' for service '%s'",
             cid,
             qmi_service_get_string (service));

    /* Set the CID as result */
    g_simple_async_result_set_op_res_gboolean (ctx->result, TRUE);
    release_cid_context_complete_and_free (ctx);
    qmi_message_unref (reply);
}

/**
 * qmi_client_ctl_release_cid:
 * @self: a #QmiClientCtl.
 * @service: a #QmiService.
 * @cid: the client ID to release.
 * @timeout: maximum time to wait to get the operation completed.
 * @cancellable: optional #GCancellable object, #NULL to ignore.
 * @callback: a #GAsyncReadyCallback to call when the operation is finished.
 * @user_data: the data to pass to callback function.
 *
 * Release a previously allocated client ID for the given @service.
 * When the query is finished, @callback will be called. You can then call
 * qmi_client_ctl_release_cid_finish() to get the the result of the operation.
 */
void
qmi_client_ctl_release_cid (QmiClientCtl *self,
                            QmiService service,
                            guint8 cid,
                            guint timeout,
                            GCancellable *cancellable,
                            GAsyncReadyCallback callback,
                            gpointer user_data)
{
    ReleaseCidContext *ctx;
    QmiMessage *request;

    ctx = g_slice_new (ReleaseCidContext);
    ctx->self = g_object_ref (self);
    ctx->service = service;
    ctx->cid = cid;
    ctx->result = g_simple_async_result_new (G_OBJECT (self),
                                             callback,
                                             user_data,
                                             qmi_client_ctl_release_cid);

    request = qmi_message_ctl_release_cid_new (qmi_client_get_next_transaction_id (QMI_CLIENT (self)),
                                               service,
                                               cid);
    qmi_device_command (QMI_DEVICE (qmi_client_peek_device (QMI_CLIENT (self))),
                        request,
                        timeout,
                        cancellable,
                        (GAsyncReadyCallback)release_cid_ready,
                        ctx);
    qmi_message_unref (request);
}

/*****************************************************************************/
/* Sync */

/**
 * qmi_client_ctl_sync_finish:
 * @self: a #QmiClientCtl.
 * @res: a #GAsyncResult.
 * @error: a #GError.
 *
 * Finishes an operation started with qmi_client_ctl_sync().
 *
 * Returns: #TRUE if the operation succeeded, or #FALSE if @error is set.
 */
gboolean
qmi_client_ctl_sync_finish (QmiClientCtl *self,
                            GAsyncResult *res,
                            GError **error)
{
    return !g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (res), error);
}

static void
sync_command_ready (QmiDevice *device,
                    GAsyncResult *res,
                    GSimpleAsyncResult *simple)
{
    GError *error = NULL;
    QmiMessage *reply;

    reply = qmi_device_command_finish (device, res, &error);
    if (!reply)
        g_simple_async_result_take_error (simple, error);
    else {
        g_simple_async_result_set_op_res_gboolean (simple, TRUE);
        qmi_message_unref (reply);
    }

    g_simple_async_result_complete (simple);
    g_object_unref (simple);
}

/**
 * qmi_client_ctl_sync:
 * @self: a #QmiClientCtl.
 * @timeout: maximum time to wait to get the operation completed.
 * @cancellable: optional #GCancellable object, #NULL to ignore.
 * @callback: a #GAsyncReadyCallback to call when the operation is finished.
 * @user_data: the data to pass to callback function.
 *
 * Request to sync with the device.
 * When the operation is finished, @callback will be called. You can then call
 * qmi_client_ctl_sync_finish() to get the the result of the operation.
 */
void
qmi_client_ctl_sync (QmiClientCtl *self,
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
                                        qmi_client_ctl_sync);

    request = qmi_message_ctl_sync_new (qmi_client_get_next_transaction_id (QMI_CLIENT (self)));
    qmi_device_command (QMI_DEVICE (qmi_client_peek_device (QMI_CLIENT (self))),
                        request,
                        timeout,
                        cancellable,
                        (GAsyncReadyCallback)sync_command_ready,
                        result);
    qmi_message_unref (request);
}

static void
qmi_client_ctl_init (QmiClientCtl *self)
{
}

static void
qmi_client_ctl_class_init (QmiClientCtlClass *klass)
{
}
