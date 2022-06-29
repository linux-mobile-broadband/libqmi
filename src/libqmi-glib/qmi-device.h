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
 * Copyright (C) 2012-2017 Aleksander Morgado <aleksander@aleksander.es>
 */

#ifndef _LIBQMI_GLIB_QMI_DEVICE_H_
#define _LIBQMI_GLIB_QMI_DEVICE_H_

#if !defined (__LIBQMI_GLIB_H_INSIDE__) && !defined (LIBQMI_GLIB_COMPILATION)
#error "Only <libqmi-glib.h> can be included directly."
#endif

#include <glib-object.h>
#include <gio/gio.h>

#include "qmi-enums.h"
#include "qmi-message.h"
#include "qmi-message-context.h"
#include "qmi-client.h"
#include "qmi-version.h"

#if QMI_QRTR_SUPPORTED
# include <libqrtr-glib.h>
#endif

G_BEGIN_DECLS

/**
 * SECTION:qmi-device
 * @title: QmiDevice
 * @short_description: Generic QMI device handling routines
 *
 * #QmiDevice is a generic type in charge of controlling the access of multiple
 * #QmiClient objects to the managed QMI port.
 *
 * A #QmiDevice can only handle one single QMI port.
 */

#define QMI_TYPE_DEVICE            (qmi_device_get_type ())
#define QMI_DEVICE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), QMI_TYPE_DEVICE, QmiDevice))
#define QMI_DEVICE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  QMI_TYPE_DEVICE, QmiDeviceClass))
#define QMI_IS_DEVICE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), QMI_TYPE_DEVICE))
#define QMI_IS_DEVICE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  QMI_TYPE_DEVICE))
#define QMI_DEVICE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  QMI_TYPE_DEVICE, QmiDeviceClass))

typedef struct _QmiDevice QmiDevice;
typedef struct _QmiDeviceClass QmiDeviceClass;
typedef struct _QmiDevicePrivate QmiDevicePrivate;

/**
 * QMI_DEVICE_FILE:
 *
 * Symbol defining the #QmiDevice:device-file property.
 *
 * Since: 1.0
 */
#define QMI_DEVICE_FILE "device-file"

/**
 * QMI_DEVICE_NO_FILE_CHECK:
 *
 * Symbol defining the #QmiDevice:device-no-file-check property.
 *
 * Since: 1.12
 */
#define QMI_DEVICE_NO_FILE_CHECK "device-no-file-check"

/**
 * QMI_DEVICE_PROXY_PATH:
 *
 * Symbol defining the #QmiDevice:device-proxy-path property.
 *
 * Since: 1.12
 */
#define QMI_DEVICE_PROXY_PATH "device-proxy-path"

/**
 * QMI_DEVICE_WWAN_IFACE:
 *
 * Symbol defining the #QmiDevice:device-wwan-iface property.
 *
 * Since: 1.14
 */
#define QMI_DEVICE_WWAN_IFACE "device-wwan-iface"

/**
 * QMI_DEVICE_CONSECUTIVE_TIMEOUTS:
 *
 * Symbol defining the #QmiDevice:device-consecutive-timeouts property.
 *
 * Since: 1.32
 */
#define QMI_DEVICE_CONSECUTIVE_TIMEOUTS "device-consecutive-timeouts"

/**
 * QMI_DEVICE_SIGNAL_INDICATION:
 *
 * Symbol defining the #QmiDevice::indication signal.
 *
 * Since: 1.8
 */
#define QMI_DEVICE_SIGNAL_INDICATION "indication"

/**
 * QMI_DEVICE_SIGNAL_REMOVED:
 *
 * Symbol defining the #QmiDevice::device-removed signal.
 *
 * Since: 1.20
 */
#define QMI_DEVICE_SIGNAL_REMOVED "device-removed"

/**
 * QmiDevice:
 *
 * The #QmiDevice structure contains private data and should only be accessed
 * using the provided API.
 *
 * Since: 1.0
 */
struct _QmiDevice {
    /*< private >*/
    GObject parent;
    QmiDevicePrivate *priv;
};

struct _QmiDeviceClass {
    /*< private >*/
    GObjectClass parent;
};

GType qmi_device_get_type (void);
G_DEFINE_AUTOPTR_CLEANUP_FUNC (QmiDevice, g_object_unref)

/**
 * qmi_device_new:
 * @file: a #GFile.
 * @cancellable: optional #GCancellable object, %NULL to ignore.
 * @callback: a #GAsyncReadyCallback to call when the initialization is finished.
 * @user_data: the data to pass to callback function.
 *
 * Asynchronously creates a #QmiDevice object to manage @file.
 * When the operation is finished, @callback will be invoked. You can then call
 * qmi_device_new_finish() to get the result of the operation.
 *
 * Since: 1.0
 */
void qmi_device_new (GFile               *file,
                     GCancellable        *cancellable,
                     GAsyncReadyCallback  callback,
                     gpointer             user_data);

/**
 * qmi_device_new_finish:
 * @res: a #GAsyncResult.
 * @error: Return location for error or %NULL.
 *
 * Finishes an operation started with qmi_device_new().
 *
 * Returns: (transfer full): A newly created #QmiDevice, or %NULL if @error is set.
 *
 * Since: 1.0
 */
QmiDevice *qmi_device_new_finish (GAsyncResult  *res,
                                  GError       **error);

/**
 * qmi_device_get_file:
 * @self: a #QmiDevice.
 *
 * Get the #GFile associated with this #QmiDevice.
 *
 * Returns: (transfer full): a #GFile that must be freed with g_object_unref().
 *
 * Since: 1.0
 */
GFile *qmi_device_get_file (QmiDevice *self);

/**
 * qmi_device_peek_file:
 * @self: a #QmiDevice.
 *
 * Get the #GFile associated with this #QmiDevice, without increasing the reference count
 * on the returned object.
 *
 * Returns: (transfer none): a #GFile. Do not free the returned object, it is owned by @self.
 *
 * Since: 1.0
 */
GFile *qmi_device_peek_file (QmiDevice *self);

/**
 * qmi_device_get_path:
 * @self: a #QmiDevice.
 *
 * Get the system path of the underlying QMI device.
 *
 * Returns: the system path of the device.
 *
 * Since: 1.0
 */
const gchar *qmi_device_get_path (QmiDevice *self);

/**
 * qmi_device_get_path_display:
 * @self: a #QmiDevice.
 *
 * Get the system path of the underlying QMI device in UTF-8.
 *
 * Returns: UTF-8 encoded system path of the device.
 *
 * Since: 1.0
 */
const gchar *qmi_device_get_path_display (QmiDevice *self);

/**
 * qmi_device_is_open:
 * @self: a #QmiDevice.
 *
 * Checks whether the #QmiDevice is open for I/O.
 *
 * Returns: %TRUE if @self is open, %FALSE otherwise.
 *
 * Since: 1.0
 */
gboolean qmi_device_is_open (QmiDevice *self);

/**
 * QmiDeviceOpenFlags:
 * @QMI_DEVICE_OPEN_FLAGS_NONE: No flags.
 * @QMI_DEVICE_OPEN_FLAGS_VERSION_INFO: Run version info check when opening. Since 1.24.4 this flag no longer prevents requests from being sent to the modem if they're assumed not supported based on the version info of each message.
 * @QMI_DEVICE_OPEN_FLAGS_SYNC: Synchronize with endpoint once the device is open. Will release any previously allocated client ID.
 * @QMI_DEVICE_OPEN_FLAGS_NET_802_3: set network port to "802.3" mode; mutually exclusive with @QMI_DEVICE_OPEN_FLAGS_NET_RAW_IP
 * @QMI_DEVICE_OPEN_FLAGS_NET_RAW_IP: set network port to "raw IP" mode; mutally exclusive with @QMI_DEVICE_OPEN_FLAGS_NET_802_3
 * @QMI_DEVICE_OPEN_FLAGS_NET_QOS_HEADER: set network port to transmit/receive QoS headers; mutually exclusive with @QMI_DEVICE_OPEN_FLAGS_NET_NO_QOS_HEADER
 * @QMI_DEVICE_OPEN_FLAGS_NET_NO_QOS_HEADER: set network port to not transmit/receive QoS headers; mutually exclusive with @QMI_DEVICE_OPEN_FLAGS_NET_QOS_HEADER
 * @QMI_DEVICE_OPEN_FLAGS_PROXY: Try to open the port through the 'qmi-proxy'. Since: 1.8.
 * @QMI_DEVICE_OPEN_FLAGS_MBIM: open an MBIM port with QMUX tunneling service. Since: 1.16.
 * @QMI_DEVICE_OPEN_FLAGS_AUTO: open a port either in QMI or MBIM mode, depending on device driver. Since: 1.18.
 * @QMI_DEVICE_OPEN_FLAGS_EXPECT_INDICATIONS: Explicitly state that indications are wanted (implicit in QMI mode, optional when in MBIM mode).
 *
 * Flags to specify which actions to be performed when the device is open.
 *
 * Since: 1.0
 */
typedef enum { /*< since=1.0 >*/
    QMI_DEVICE_OPEN_FLAGS_NONE               = 0,
    QMI_DEVICE_OPEN_FLAGS_VERSION_INFO       = 1 << 0,
    QMI_DEVICE_OPEN_FLAGS_SYNC               = 1 << 1,
    QMI_DEVICE_OPEN_FLAGS_NET_802_3          = 1 << 2,
    QMI_DEVICE_OPEN_FLAGS_NET_RAW_IP         = 1 << 3,
    QMI_DEVICE_OPEN_FLAGS_NET_QOS_HEADER     = 1 << 4,
    QMI_DEVICE_OPEN_FLAGS_NET_NO_QOS_HEADER  = 1 << 5,
    QMI_DEVICE_OPEN_FLAGS_PROXY              = 1 << 6,
    QMI_DEVICE_OPEN_FLAGS_MBIM               = 1 << 7,
    QMI_DEVICE_OPEN_FLAGS_AUTO               = 1 << 8,
    QMI_DEVICE_OPEN_FLAGS_EXPECT_INDICATIONS = 1 << 9,
} QmiDeviceOpenFlags;

/**
 * qmi_device_open:
 * @self: a #QmiDevice.
 * @flags: mask of #QmiDeviceOpenFlags specifying how the device should be opened.
 * @timeout: maximum time, in seconds, to wait for the device to be opened.
 * @cancellable: optional #GCancellable object, %NULL to ignore.
 * @callback: a #GAsyncReadyCallback to call when the operation is finished.
 * @user_data: the data to pass to callback function.
 *
 * Asynchronously opens a #QmiDevice for I/O.
 *
 * When the operation is finished @callback will be called. You can then call
 * qmi_device_open_finish() to get the result of the operation.
 *
 * Since: 1.0
 */
void qmi_device_open (QmiDevice           *self,
                      QmiDeviceOpenFlags   flags,
                      guint                timeout,
                      GCancellable        *cancellable,
                      GAsyncReadyCallback  callback,
                      gpointer             user_data);

/**
 * qmi_device_open_finish:
 * @self: a #QmiDevice.
 * @res: a #GAsyncResult.
 * @error: Return location for error or %NULL.
 *
 * Finishes an asynchronous open operation started with qmi_device_open().
 *
 * Returns: %TRUE if successful, %FALSE if @error is set.
 *
 * Since: 1.0
 */
gboolean qmi_device_open_finish (QmiDevice     *self,
                                 GAsyncResult  *res,
                                 GError       **error);

/**
 * qmi_device_close_async:
 * @self: a #QmiDevice.
 * @timeout: maximum time, in seconds, to wait for the device to be closed.
 * @cancellable: a #GCancellable, or %NULL.
 * @callback: a #GAsyncReadyCallback to call when the operation is finished.
 * @user_data: the data to pass to callback function.
 *
 * Asynchronously closes a #QmiDevice, preventing any further I/O.
 *
 * If this device was opened with @QMI_DEVICE_OPEN_FLAGS_MBIM, this
 * operation will wait for the response of the underlying MBIM close
 * sequence.
 *
 * Closing a #QmiDevice multiple times will not return an error.
 *
 * When the operation is finished @callback will be called. You can then call
 * qmi_device_close_finish() to get the result of the operation.
 *
 * Since: 1.18
 */
void qmi_device_close_async (QmiDevice           *self,
                             guint                timeout,
                             GCancellable        *cancellable,
                             GAsyncReadyCallback  callback,
                             gpointer             user_data);

/**
 * qmi_device_close_finish:
 * @self: a #QmiDevice.
 * @res: a #GAsyncResult.
 * @error: Return location for error or %NULL.
 *
 * Finishes an operation started with qmi_device_close_async().
 *
 * Returns: %TRUE if successful, %FALSE if @error is set.
 *
 * Since: 1.18
 */
gboolean qmi_device_close_finish (QmiDevice     *self,
                                  GAsyncResult  *res,
                                  GError       **error);

/**
 * qmi_device_allocate_client:
 * @self: a #QmiDevice.
 * @service: a valid #QmiService.
 * @cid: a valid client ID, or %QMI_CID_NONE.
 * @timeout: maximum time to wait.
 * @cancellable: optional #GCancellable object, %NULL to ignore.
 * @callback: a #GAsyncReadyCallback to call when the operation is finished.
 * @user_data: the data to pass to callback function.
 *
 * Asynchronously allocates a new #QmiClient in @self.
 *
 * If %QMI_CID_NONE is given in @cid, a new client ID will be allocated;
 * otherwise a client with the given @cid will be generated.
 *
 * When the operation is finished @callback will be called. You can then call
 * qmi_device_allocate_client_finish() to get the result of the operation.
 *
 * Note: Clients for the %QMI_SERVICE_CTL cannot be created with this method;
 * instead get/peek the implicit one from @self.
 *
 * Since: 1.0
 */
void qmi_device_allocate_client (QmiDevice           *self,
                                 QmiService           service,
                                 guint8               cid,
                                 guint                timeout,
                                 GCancellable        *cancellable,
                                 GAsyncReadyCallback  callback,
                                 gpointer             user_data);

/**
 * qmi_device_allocate_client_finish:
 * @self: a #QmiDevice.
 * @res: a #GAsyncResult.
 * @error: Return location for error or %NULL.
 *
 * Finishes an operation started with qmi_device_allocate_client().
 *
 * Returns: (transfer full): a newly allocated #QmiClient, or %NULL if @error is set.
 *
 * Since: 1.0
 */
QmiClient *qmi_device_allocate_client_finish (QmiDevice     *self,
                                              GAsyncResult  *res,
                                              GError       **error);

/**
 * QmiDeviceReleaseClientFlags:
 * @QMI_DEVICE_RELEASE_CLIENT_FLAGS_NONE: No flags.
 * @QMI_DEVICE_RELEASE_CLIENT_FLAGS_RELEASE_CID: Release the CID when releasing the client.
 *
 * Flags to specify which actions to be performed when releasing the client.
 *
 * Since: 1.0
 */
typedef enum { /*< since=1.0 >*/
    QMI_DEVICE_RELEASE_CLIENT_FLAGS_NONE        = 0,
    QMI_DEVICE_RELEASE_CLIENT_FLAGS_RELEASE_CID = 1 << 0
} QmiDeviceReleaseClientFlags;

/**
 * qmi_device_release_client:
 * @self: a #QmiDevice.
 * @client: the #QmiClient to release.
 * @flags: mask of #QmiDeviceReleaseClientFlags specifying how the client should be released.
 * @timeout: maximum time to wait.
 * @cancellable: optional #GCancellable object, %NULL to ignore.
 * @callback: a #GAsyncReadyCallback to call when the operation is finished.
 * @user_data: the data to pass to callback function.
 *
 * Asynchronously releases the #QmiClient from the #QmiDevice.
 *
 * Once the #QmiClient has been released, it cannot be used any more to
 * perform operations.
 *
 *
 * When the operation is finished @callback will be called. You can then call
 * qmi_device_release_client_finish() to get the result of the operation.
 *
 * Since: 1.0
 */
void qmi_device_release_client (QmiDevice                   *self,
                                QmiClient                   *client,
                                QmiDeviceReleaseClientFlags  flags,
                                guint                        timeout,
                                GCancellable                *cancellable,
                                GAsyncReadyCallback          callback,
                                gpointer                     user_data);

/**
 * qmi_device_release_client_finish:
 * @self: a #QmiDevice.
 * @res: a #GAsyncResult.
 * @error: Return location for error or %NULL.
 *
 * Finishes an operation started with qmi_device_release_client().
 *
 * Note that even if the release operation returns an error, the client should
 * anyway be considered released, and shouldn't be used afterwards.
 *
 * Returns: %TRUE if successful, or %NULL if @error is set.
 *
 * Since: 1.0
 */
gboolean qmi_device_release_client_finish (QmiDevice     *self,
                                           GAsyncResult  *res,
                                           GError       **error);

/**
 * qmi_device_set_instance_id:
 * @self: a #QmiDevice.
 * @instance_id: the instance ID.
 * @timeout: maximum time to wait.
 * @cancellable: optional #GCancellable object, %NULL to ignore.
 * @callback: a #GAsyncReadyCallback to call when the operation is finished.
 * @user_data: the data to pass to callback function.
 *
 * Sets the instance ID of the #QmiDevice.
 *
 * When the operation is finished @callback will be called. You can then call
 * qmi_device_set_instance_id_finish() to get the result of the operation.
 *
 * Since: 1.0
 */
void qmi_device_set_instance_id (QmiDevice           *self,
                                 guint8               instance_id,
                                 guint                timeout,
                                 GCancellable        *cancellable,
                                 GAsyncReadyCallback  callback,
                                 gpointer             user_data);

/**
 * qmi_device_set_instance_id_finish:
 * @self: a #QmiDevice.
 * @res: a #GAsyncResult.
 * @link_id: a placeholder for the output #guint16, or %NULL if not required.
 * @error: Return location for error or %NULL.
 *
 * Finishes an operation started with qmi_device_set_instance_id().
 *
 * Returns: %TRUE if successful, %FALSE if @error is set.
 *
 * Since: 1.0
 */
gboolean qmi_device_set_instance_id_finish (QmiDevice     *self,
                                            GAsyncResult  *res,
                                            guint16       *link_id,
                                            GError       **error);

/**
 * qmi_device_command_full:
 * @self: a #QmiDevice.
 * @message: the message to send.
 * @message_context: the context of the message.
 * @timeout: maximum time, in seconds, to wait for the response.
 * @cancellable: a #GCancellable, or %NULL.
 * @callback: a #GAsyncReadyCallback to call when the operation is finished.
 * @user_data: the data to pass to callback function.
 *
 * Asynchronously sends a #QmiMessage to the device.
 *
 * The message will be processed according to the specific @message_context
 * given. If no @context given, the behavior is the same as qmi_device_command().
 *
 * If the operation is cancelled via @cancellable, a %QMI_PROTOCOL_ERROR_ABORTED
 * error will be returned always. If the QMI method may be aborted, there is
 * no guarantee that the operation is truly aborted before the error is returned
 * so it may really happen that the operation really succeeded and the method
 * would still return %QMI_PROTOCOL_ERROR_ABORTED. In order to use abortable
 * methods and make sure the response is the correct one, use
 * qmi_device_command_abortable().
 *
 * When the operation is finished @callback will be called. You can then call
 * qmi_device_command_full_finish() to get the result of the operation.
 *
 * Since: 1.18
 */
void qmi_device_command_full (QmiDevice           *self,
                              QmiMessage          *message,
                              QmiMessageContext   *message_context,
                              guint                timeout,
                              GCancellable        *cancellable,
                              GAsyncReadyCallback  callback,
                              gpointer             user_data);

/**
 * qmi_device_command_full_finish:
 * @self: a #QmiDevice.
 * @res: a #GAsyncResult.
 * @error: Return location for error or %NULL.
 *
 * Finishes an operation started with qmi_device_command_full().
 *
 * Returns: (transfer full): a #QmiMessage response, or %NULL if @error is set. The returned value should be freed with qmi_message_unref().
 *
 * Since: 1.18
 */
QmiMessage *qmi_device_command_full_finish (QmiDevice     *self,
                                            GAsyncResult  *res,
                                            GError       **error);

/**
 * QmiDeviceCommandAbortableBuildRequestFn:
 * @self: a #QmiDevice.
 * @message: the #QmiMessage to abort.
 * @user_data: the data provided when calling qmi_device_command_abortable().
 * @error: Return location for error or %NULL.
 *
 * Callback to run when processing the command abortion. This callback
 * should create a service-specific and client-specific abort request to
 * be passed to the device.
 *
 * Returns: the abort request as a #QmiMessage or %NULL if @error is set.
 *
 * Since: 1.24
 */
typedef QmiMessage * (* QmiDeviceCommandAbortableBuildRequestFn) (QmiDevice   *self,
                                                                  QmiMessage  *message,
                                                                  gpointer     user_data,
                                                                  GError     **error);

/**
 * QmiDeviceCommandAbortableParseResponseFn:
 * @self: a #QmiDevice.
 * @abort_response: the abort response #QmiMessage.
 * @user_data: the data provided when calling qmi_device_command_abortable().
 * @error: Return location for error or %NULL.
 *
 * Callback to run when processing the command abortion. This callback
 * should parse the abort response provided by the device, and build an
 * appropriate output.
 *
 * Returns: %TRUE if the abort succeeded, %FALSE if error is set.
 *
 * Since: 1.24
 */
typedef gboolean (* QmiDeviceCommandAbortableParseResponseFn) (QmiDevice   *self,
                                                               QmiMessage  *abort_response,
                                                               gpointer     user_data,
                                                               GError     **error);

/**
 * qmi_device_command_abortable:
 * @self: a #QmiDevice.
 * @message: the message to send.
 * @message_context: the context of the message.
 * @timeout: maximum time, in seconds, to wait for the response.
 * @abort_build_request_fn: (scope async): callback to build an abort request.
 * @abort_parse_response_fn: (scope async): callback to parse an abort response.
 * @abort_user_data: (closure): user data to pass to @build_request_fn and @parse_response_fn.
 * @abort_user_data_free: (destroy abort_user_data): a #GDestroyNotify to free @abort_user_data.
 * @cancellable: a #GCancellable, or %NULL.
 * @callback: a #GAsyncReadyCallback to call when the operation is finished.
 * @user_data: the data to pass to callback function.
 *
 * Asynchronously sends a #QmiMessage to the device.
 *
 * The message will be processed according to the specific @message_context
 * given.
 *
 * If the operation is cancelled via @cancellable, an abort message will
 * be sent to the device in order to really abort the ongoing operation. The
 * qmi_device_command_abortable() method will not finish until either a
 * successful reply is received from the device or otherwise the command
 * abortion is confirmed.
 *
 * If a successful command response arrives before the abort is processed by
 * the device, the operation will succeed even if @cancellable has been set, so
 * that upper layers can do whatever they need to do to properly tear down the
 * operation.
 *
 * If an error command reponse arrives before the abort is processed by the
 * device, the operation will fail with the error returned by the device.
 *
 * The %QMI_PROTOCOL_ERROR_ABORTED error will only be returned as a result
 * of this operation if the command request was truly aborted by the device,
 * and so, the user of the method should not assume that cancelling the
 * @cancellable will always make the method return a %QMI_PROTOCOL_ERROR_ABORTED
 * error.
 *
 * The @build_request_fn and @parse_response_fn callbacks are required in order
 * to build the abort requests and parse the abort responses, because these
 * are both service and client specific.
 *
 * When the operation is finished @callback will be called. You can then call
 * qmi_device_command_abortable_finish() to get the result of the operation.
 *
 * Since: 1.24
 */
void qmi_device_command_abortable (QmiDevice                                *self,
                                   QmiMessage                               *message,
                                   QmiMessageContext                        *message_context,
                                   guint                                     timeout,
                                   QmiDeviceCommandAbortableBuildRequestFn   abort_build_request_fn,
                                   QmiDeviceCommandAbortableParseResponseFn  abort_parse_response_fn,
                                   gpointer                                  abort_user_data,
                                   GDestroyNotify                            abort_user_data_free,
                                   GCancellable                             *cancellable,
                                   GAsyncReadyCallback                       callback,
                                   gpointer                                  user_data);

/**
 * qmi_device_command_abortable_finish:
 * @self: a #QmiDevice.
 * @res: a #GAsyncResult.
 * @error: Return location for error or %NULL.
 *
 * Finishes an operation started with qmi_device_command_abortable().
 *
 * Returns: (transfer full): a #QmiMessage response, or %NULL if @error is set. The returned value should be freed with qmi_message_unref().
 *
 * Since: 1.24
 */
QmiMessage *qmi_device_command_abortable_finish (QmiDevice     *self,
                                                 GAsyncResult  *res,
                                                 GError       **error);

/**
 * QmiDeviceServiceVersionInfo:
 * @service: a #QmiService.
 * @major_version: major version of the service.
 * @minor_version: minor version of the service.
 *
 * Version information for a service.
 *
 * Since: 1.6
 */
typedef struct {
    QmiService service;
    guint16    major_version;
    guint16    minor_version;
} QmiDeviceServiceVersionInfo;

/**
 * qmi_device_get_service_version_info:
 * @self: a #QmiDevice.
 * @timeout: maximum time to wait for the method to complete, in seconds.
 * @cancellable: a #GCancellable or %NULL.
 * @callback: a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: user data to pass to @callback.
 *
 * Asynchronously requests the service version information of the device.
 *
 * When the operation is finished, @callback will be invoked in the thread-default main loop of the thread you are calling this method from.
 *
 * You can then call qmi_device_get_service_version_info_finish() to get the result of the operation.
 *
 * Since: 1.6
 */
void qmi_device_get_service_version_info (QmiDevice           *self,
                                          guint                timeout,
                                          GCancellable        *cancellable,
                                          GAsyncReadyCallback  callback,
                                          gpointer             user_data);

/**
 * qmi_device_get_service_version_info_finish:
 * @self: a #QmiDevice.
 * @res: a #GAsyncResult.
 * @error: Return location for error or %NULL.
 *
 * Finishes an operation started with qmi_device_get_service_version_info().
 *
 * Returns: (transfer full)(element-type QmiDeviceServiceVersionInfo):
 *  a #GArray of #QmiDeviceServiceVersionInfo elements, or %NULL if @error
 *  is set. The returned value should be freed with g_array_unref().
 *
 * Since: 1.6
 */
GArray *qmi_device_get_service_version_info_finish (QmiDevice     *self,
                                                    GAsyncResult  *res,
                                                    GError       **error);

/**
 * qmi_device_get_consecutive_timeouts:
 * @self: a #QmiDevice.
 *
 * Gets the number of consecutive transaction timeouts in the device.
 *
 * Returns: a #guint.
 *
 * Since: 1.32
 */
guint qmi_device_get_consecutive_timeouts (QmiDevice *self);

/******************************************************************************/
/* qmi_wwan specific APIs */

/**
 * qmi_device_get_wwan_iface:
 * @self: a #QmiDevice.
 *
 * Get the WWAN interface name associated with the QMI control port.
 * This value will be loaded every time it's asked for it.
 *
 * <note><para>
 * This method is only applicable when using the qmi_wwan kernel driver.
 * </para></note>
 *
 * Returns: UTF-8 encoded network interface name, or %NULL if not available.
 *
 * Since: 1.14
 */
const gchar *qmi_device_get_wwan_iface (QmiDevice *self);

/**
 * QmiDeviceExpectedDataFormat:
 * @QMI_DEVICE_EXPECTED_DATA_FORMAT_UNKNOWN: Unknown.
 * @QMI_DEVICE_EXPECTED_DATA_FORMAT_802_3: 802.3.
 * @QMI_DEVICE_EXPECTED_DATA_FORMAT_RAW_IP: Raw IP.
 * @QMI_DEVICE_EXPECTED_DATA_FORMAT_QMAP_PASS_THROUGH: QMAP pass-through. Since 1.28.
 *
 * Data format expected by the kernel.
 *
 * <note><para>
 * This type is only applicable when using the qmi_wwan kernel driver.
 * </para></note>
 *
 * Since: 1.14
 */
typedef enum { /*< since=1.14 >*/
    QMI_DEVICE_EXPECTED_DATA_FORMAT_UNKNOWN,
    QMI_DEVICE_EXPECTED_DATA_FORMAT_802_3,
    QMI_DEVICE_EXPECTED_DATA_FORMAT_RAW_IP,
    QMI_DEVICE_EXPECTED_DATA_FORMAT_QMAP_PASS_THROUGH,
} QmiDeviceExpectedDataFormat;

/**
 * qmi_device_get_expected_data_format:
 * @self: a #QmiDevice.
 * @error: Return location for error or %NULL.
 *
 * Retrieves the data format currently expected by the kernel in the network
 * interface.
 *
 * If @QMI_DEVICE_EXPECTED_DATA_FORMAT_UNKNOWN is returned, the user should
 * assume that 802.3 is the expected format, as that is what the qmi_wwan
 * driver expected by default before kernel 4.5.
 *
 * <note><para>
 * This method is only applicable when using the qmi_wwan kernel driver.
 * </para></note>
 *
 * Returns: a valid #QmiDeviceExpectedDataFormat, or @QMI_DEVICE_EXPECTED_DATA_FORMAT_UNKNOWN if @error is set.
 *
 * Since: 1.14
 */
QmiDeviceExpectedDataFormat qmi_device_get_expected_data_format (QmiDevice  *self,
                                                                 GError    **error);

/**
 * qmi_device_set_expected_data_format:
 * @self: a #QmiDevice.
 * @format: a known #QmiDeviceExpectedDataFormat.
 * @error: Return location for error or %NULL.
 *
 * Configures the data format currently expected by the kernel in the network
 * interface.
 *
 * <note><para>
 * This method is only applicable when using the qmi_wwan kernel driver.
 * </para></note>
 *
 * Returns: %TRUE if successful, or %FALSE if @error is set.
 *
 * Since: 1.14
 */
gboolean qmi_device_set_expected_data_format (QmiDevice                    *self,
                                              QmiDeviceExpectedDataFormat   format,
                                              GError                      **error);

/**
 * qmi_device_check_expected_data_format_supported:
 * @self: a #QmiDevice.
 * @format: a known #QmiDeviceExpectedDataFormat.
 * @error: Return location for error or %NULL.
 *
 * Checks whether the given data format is supported by the kernel.
 * interface.
 *
 * <note><para>
 * This method is only applicable when using the qmi_wwan kernel driver.
 * </para></note>
 *
 * Returns: %TRUE if the data format is supported, or %FALSE if @error is set.
 *
 * Since: 1.28
 */
gboolean qmi_device_check_expected_data_format_supported (QmiDevice                    *self,
                                                          QmiDeviceExpectedDataFormat   format,
                                                          GError                      **error);

/******************************************************************************/
/* Link management APIs */

/**
 * QMI_DEVICE_MUX_ID_AUTOMATIC:
 *
 * Symbol defining a mux id that will be automatically allocated during runtime
 * when creating net links.
 *
 * Since: 1.28
 */
#define QMI_DEVICE_MUX_ID_AUTOMATIC G_MAXUINT

/**
 * QMI_DEVICE_MUX_ID_UNBOUND:
 *
 * Symbol defining the mux id for an unbound interface. This value is also used
 * to indicate an invalid mux id.
 *
 * Since: 1.28
 */
#define QMI_DEVICE_MUX_ID_UNBOUND 0

/**
 * QMI_DEVICE_MUX_ID_MIN:
 *
 * Symbol defining the minimum supported mux id.
 *
 * Since: 1.28
 */
#define QMI_DEVICE_MUX_ID_MIN 1

/**
 * QMI_DEVICE_MUX_ID_MAX:
 *
 * Symbol defining the maximum supported mux id.
 *
 * Since: 1.28
 */
#define QMI_DEVICE_MUX_ID_MAX 0xFE

/**
 * qmi_device_add_link:
 * @self: a #QmiDevice.
 * @mux_id: the mux id for the link, in the
 *   [%QMI_DEVICE_MUX_ID_MIN,%QMI_DEVICE_MUX_ID_MAX] range, or
 *   %QMI_DEVICE_MUX_ID_AUTOMATIC to find the first available mux id.
 * @base_ifname: the interface which the new link will be created on.
 * @ifname_prefix: the prefix suggested to be used for the name of the new link
 *   created.
 * @cancellable: a #GCancellable, or %NULL.
 * @callback: a #GAsyncReadyCallback to call when the operation is finished.
 * @user_data: the data to pass to callback function.
 *
 * Asynchronously creates a new virtual network device node with a custom prefix
 * on top of @base_ifname. This allows having multiple net interfaces running on
 * top of another using multiplexing.
 *
 * If the kernel driver doesn't allow this functionality, a
 * %QMI_CORE_ERROR_UNSUPPORTED error will be returned.
 *
 * The operation may fail if the given interface name is not associated to the
 * QMI control port managed by the #QmiDevice.
 *
 * Depending on the kernel driver in use and the multiplexing method, the given
 * @ifname_prefix may be ignored. The user should not assume that the returned
 * link interface name is prefixed with @ifname_prefix as it may not be the
 * case.
 *
 * When the operation is finished @callback will be called. You can then call
 * qmi_device_add_link_finish() to get the result of the operation.
 *
 * <note><para>
 * When using the qmi_wwan kernel driver, the configured expected kernel data
 * format will be used to select the type of multiplexing method. If the
 * format is %QMI_DEVICE_EXPECTED_DATA_FORMAT_RAW_IP the qmi_wwan specific
 * add_mux/del_mux operations will be used. If the format is
 * %QMI_DEVICE_EXPECTED_DATA_FORMAT_QMAP_PASS_THROUGH, the generic rmnet netlink
 * operations will be used. No multiplexing support exists when the format is
 * %QMI_DEVICE_EXPECTED_DATA_FORMAT_802_3.
 * </para><para>
 * For every other kernel driver (e.g. ipa), rmnet netlink operations are
 * assumed to be supported.
 * </para></note>
 *
 * <note><para>
 * When using the qmi_wwan driver from a kernel older than v5.12, some of the
 * multiplexing features like using %QMI_DEVICE_MUX_ID_AUTOMATIC may not be fully
 * available for programs that use ephimeral #QmiDevice objects for single
 * operations.
 * </para></note>
 *
 * Since: 1.28
 */
void qmi_device_add_link (QmiDevice           *self,
                          guint                mux_id,
                          const gchar         *base_ifname,
                          const gchar         *ifname_prefix,
                          GCancellable        *cancellable,
                          GAsyncReadyCallback  callback,
                          gpointer             user_data);

/**
 * qmi_device_add_link_finish:
 * @self: a #QmiDevice.
 * @res: a #GAsyncResult.
 * @mux_id: the mux ID for the link created.
 * @error: Return location for error or %NULL.
 *
 * Finishes an operation started with qmi_device_add_link().
 *
 * Returns: The name of the net interface created, %NULL if @error is set.
 *
 * Since: 1.28
 */
gchar *qmi_device_add_link_finish (QmiDevice     *self,
                                   GAsyncResult  *res,
                                   guint         *mux_id,
                                   GError       **error);

/**
 * QmiDeviceAddLinkFlags:
 * @QMI_DEVICE_ADD_LINK_FLAGS_NONE: none.
 * @QMI_DEVICE_ADD_LINK_FLAGS_INGRESS_MAP_CKSUMV4: checksum offload (v4) is enabled in ingress path.
 * @QMI_DEVICE_ADD_LINK_FLAGS_EGRESS_MAP_CKSUMV4: checksum offload (v4) is enabled in egress path.
 * @QMI_DEVICE_ADD_LINK_FLAGS_INGRESS_MAP_CKSUMV5: checksum offload (v5) is enabled in ingress path. Since 1.32.
 * @QMI_DEVICE_ADD_LINK_FLAGS_EGRESS_MAP_CKSUMV5: checksum offload (v5) is enabled in egress path. Since 1.32.
 *
 * Flags specifying the behavior of the new link.
 *
 * Since: 1.30
 */
typedef enum { /*< since=1.30 >*/
    QMI_DEVICE_ADD_LINK_FLAGS_NONE                = 0,
    QMI_DEVICE_ADD_LINK_FLAGS_INGRESS_MAP_CKSUMV4 = 1 << 1,
    QMI_DEVICE_ADD_LINK_FLAGS_EGRESS_MAP_CKSUMV4  = 1 << 2,
    QMI_DEVICE_ADD_LINK_FLAGS_INGRESS_MAP_CKSUMV5 = 1 << 3,
    QMI_DEVICE_ADD_LINK_FLAGS_EGRESS_MAP_CKSUMV5  = 1 << 4,
} QmiDeviceAddLinkFlags;

/**
 * qmi_device_add_link_with_flags:
 * @self: a #QmiDevice.
 * @mux_id: the mux id for the link, in the
 *   [%QMI_DEVICE_MUX_ID_MIN,%QMI_DEVICE_MUX_ID_MAX] range, or
 *   %QMI_DEVICE_MUX_ID_AUTOMATIC to find the first available mux id.
 * @base_ifname: the interface which the new link will be created on.
 * @ifname_prefix: the prefix suggested to be used for the name of the new link
 *   created.
 * @flags: bitmask of %QmiDeviceAddLinkFlags values to pass to the kernel when
 *   creating the new link.
 * @cancellable: a #GCancellable, or %NULL.
 * @callback: a #GAsyncReadyCallback to call when the operation is finished.
 * @user_data: the data to pass to callback function.
 *
 * Asynchronously creates a new virtual network device in the same way as
 * qmi_device_add_link() does, but passing the additional @flags to the kernel
 * during the operation.
 *
 * Using %QMI_DEVICE_ADD_LINK_FLAGS_NONE as @flags is equivalent to calling
 * qmi_device_add_link() directly.
 *
 * If the link creation with the given set of @flags is unsupported by the
 * backend, the operation may fail.
 *
 * <note><para>
 * None of the @flags supported are applicable when using the multiplexing
 * support provided by the qmi_wwan kernel driver, they are only used if using
 * the rmnet backend for link management support.
 * </para></note>
 *
 * Since: 1.30
 */
void qmi_device_add_link_with_flags (QmiDevice             *self,
                                     guint                  mux_id,
                                     const gchar           *base_ifname,
                                     const gchar           *ifname_prefix,
                                     QmiDeviceAddLinkFlags  flags,
                                     GCancellable          *cancellable,
                                     GAsyncReadyCallback    callback,
                                     gpointer               user_data);

/**
 * qmi_device_add_link_with_flags_finish:
 * @self: a #QmiDevice.
 * @res: a #GAsyncResult.
 * @mux_id: the mux ID for the link created.
 * @error: Return location for error or %NULL.
 *
 * Finishes an operation started with qmi_device_add_link_finish().
 *
 * Returns: The name of the net interface created, %NULL if @error is set.
 *
 * Since: 1.30
 */
gchar *qmi_device_add_link_with_flags_finish (QmiDevice     *self,
                                              GAsyncResult  *res,
                                              guint         *mux_id,
                                              GError       **error);

/**
 * qmi_device_delete_link:
 * @self: a #QmiDevice.
 * @ifname: the name of the link to remove.
 * @mux_id: the mux ID of the link to remove.
 * @cancellable: a #GCancellable, or %NULL.
 * @callback: a #GAsyncReadyCallback to call when the operation is finished.
 * @user_data: the data to pass to callback function.
 *
 * Asynchronously deletes a virtual network interface that has been previously
 * created with qmi_device_add_link().
 *
 * If the kernel driver doesn't allow this functionality, a
 * %QMI_CORE_ERROR_UNSUPPORTED error will be returned.
 *
 * When the operation is finished @callback will be called. You can then call
 * qmi_device_delete_link_finish() to get the result of the operation.
 *
 * <note><para>
 * The %QMI_DEVICE_MUX_ID_UNBOUND value may be given as @mux_id if the user
 * can guarantee that the underlying kernel support doesn't require the
 * mux id info to delete the link. When using the qmi_wwan driver from a kernel
 * older than v5.12, a valid @mux_id is required.
 * </para></note>
 *
 * Since: 1.28
 */
void qmi_device_delete_link (QmiDevice           *self,
                             const gchar         *ifname,
                             guint                mux_id,
                             GCancellable        *cancellable,
                             GAsyncReadyCallback  callback,
                             gpointer             user_data);

/**
 * qmi_device_delete_link_finish:
 * @self: a #QmiDevice.
 * @res: a #GAsyncResult.
 * @error: Return location for error or %NULL.
 *
 * Finishes an operation started with qmi_device_delete_link().
 *
 * Returns: %TRUE if successful, %FALSE if @error is set.
 *
 * Since: 1.28
 */
gboolean qmi_device_delete_link_finish (QmiDevice     *self,
                                        GAsyncResult  *res,
                                        GError       **error);

/**
 * qmi_device_delete_all_links:
 * @self: a #QmiDevice.
 * @base_ifname: the interface where all links are available.
 * @cancellable: a #GCancellable, or %NULL.
 * @callback: a #GAsyncReadyCallback to call when the operation is finished.
 * @user_data: the data to pass to callback function.
 *
 * Asynchronously deletes all virtual network interfaces that have been previously
 * created with qmi_device_add_link() in @base_ifname.
 *
 * When the operation is finished @callback will be called. You can then call
 * qmi_device_delete_link_finish() to get the result of the operation.
 *
 * <note><para>
 * There is no guarantee that other processes haven't created new links by the
 * time this method returns. This method should be used with caution, or in setups
 * where only one single process is expected to do QMI network interface link
 * management.
 * </para></note>
 *
 * Since: 1.28
 */
void qmi_device_delete_all_links (QmiDevice           *self,
                                  const gchar         *base_ifname,
                                  GCancellable        *cancellable,
                                  GAsyncReadyCallback  callback,
                                  gpointer             user_data);

/**
 * qmi_device_delete_all_links_finish:
 * @self: a #QmiDevice.
 * @res: a #GAsyncResult.
 * @error: Return location for error or %NULL.
 *
 * Finishes an operation started with qmi_device_delete_all_links().
 *
 * Returns: %TRUE if successful, %FALSE if @error is set.
 *
 * Since: 1.28
 */
gboolean qmi_device_delete_all_links_finish (QmiDevice     *self,
                                             GAsyncResult  *res,
                                             GError       **error);

/**
 * qmi_device_list_links:
 * @self: a #QmiDevice.
 * @base_ifname: the base interface.
 * @out_links: (out)(transfer full)(element-type utf8): a placeholder for the
 *   output #GPtrArray of link names.
 * @error: Return location for error or %NULL.
 *
 * Synchronously lists all virtual network interfaces that have been previously
 * created with qmi_device_add_link() in @base_ifname.
 *
 * Returns: %TRUE if successful, %FALSE if @error is set.
 *
 * Since: 1.28
 */
gboolean qmi_device_list_links (QmiDevice    *self,
                                const gchar  *base_ifname,
                                GPtrArray   **out_links,
                                GError      **error);

/**
 * qmi_device_check_link_supported:
 * @self: a #QmiDevice.
 * @error: Return location for error or %NULL.
 *
 * Checks whether link management is supported by the kernel.
 *
 * Returns: %TRUE if link management is supported, or %FALSE if @error is set.
 *
 * Since: 1.28
 */
gboolean qmi_device_check_link_supported (QmiDevice  *self,
                                          GError    **error);

/******************************************************************************/
/* New QRTR based APIs */

#if QMI_QRTR_SUPPORTED

/**
 * QMI_DEVICE_NODE:
 *
 * Symbol defining the #QmiDevice:device-node property.
 *
 * <note><para>
 * This property is only available when the library is built with QRTR support.
 * </para></note>
 *
 * Since: 1.28
 */
#define QMI_DEVICE_NODE "device-node"

/**
 * qmi_device_new_from_node:
 * @node: a #QrtrNode.
 * @cancellable: optional #GCancellable object, #NULL to ignore.
 * @callback: a #GAsyncReadyCallback to call when the initialization is finished.
 * @user_data: the data to pass to callback function.
 *
 * Asynchronously creates a #QmiDevice object to manage @node.
 * When the operation is finished, @callback will be invoked. You can then call
 * qmi_device_new_finish() to get the result of the operation.
 *
 * <note><para>
 * This method is only available when the library is built with QRTR support.
 * </para></note>
 *
 * Since: 1.28
 */
void qmi_device_new_from_node (QrtrNode            *node,
                               GCancellable        *cancellable,
                               GAsyncReadyCallback  callback,
                               gpointer             user_data);

/**
 * qmi_device_new_from_node_finish:
 * @res: a #GAsyncResult.
 * @error: Return location for error or %NULL.
 *
 * Finishes an operation started with qmi_device_new_from_node().
 *
 * <note><para>
 * This method is only available when the library is built with QRTR support.
 * </para></note>
 *
 * Returns: (transfer full): A newly created #QmiDevice, or %NULL if @error is set.
 *
 * Since: 1.28
 */
QmiDevice *qmi_device_new_from_node_finish (GAsyncResult  *res,
                                            GError       **error);

/**
 * qmi_device_get_node:
 * @self: a #QmiDevice.
 *
 * Get the #QrtrNode associated with this #QmiDevice.
 *
 * <note><para>
 * This method is only available when the library is built with QRTR support.
 * </para></note>
 *
 * Returns: (transfer full): a #QrtrNode that must be freed with g_object_unref() or %NULL if none available.
 *
 * Since: 1.28
 */
QrtrNode *qmi_device_get_node (QmiDevice *self);

/**
 * qmi_device_peek_node:
 * @self: a #QmiDevice.
 *
 * Get the #QrtrNode associated with this #QmiDevice, without increasing the
 * reference count on the returned object.
 *
 * <note><para>
 * This method is only available when the library is built with QRTR support.
 * </para></note>
 *
 * Returns: (transfer none): a #QrtrNode or %NULL if none available. Do not free the returned object, it is owned by @self.
 *
 * Since: 1.28
 */
QrtrNode *qmi_device_peek_node (QmiDevice *self);

#endif /* QMI_QRTR_SUPPORTED */

G_END_DECLS

#endif /* _LIBQMI_GLIB_QMI_DEVICE_H_ */
