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
 * Copyright (C) 2019 Eric Caruso <ejcaruso@chromium.org>
 */

#ifndef _LIBQMI_GLIB_QMI_ENDPOINT_H_
#define _LIBQMI_GLIB_QMI_ENDPOINT_H_

#include <glib-object.h>
#include <gio/gio.h>

#include "qmi-ctl.h"
#include "qmi-file.h"
#include "qmi-message.h"

typedef void (*QmiMessageHandler) (QmiMessage *message,
                                   gpointer user_data);

#define QMI_TYPE_ENDPOINT            (qmi_endpoint_get_type ())
#define QMI_ENDPOINT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), QMI_TYPE_ENDPOINT, QmiEndpoint))
#define QMI_ENDPOINT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  QMI_TYPE_ENDPOINT, QmiEndpointClass))
#define QMI_IS_ENDPOINT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), QMI_TYPE_ENDPOINT))
#define QMI_IS_ENDPOINT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  QMI_TYPE_ENDPOINT))
#define QMI_ENDPOINT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  QMI_TYPE_ENDPOINT, QmiEndpointClass))

typedef struct _QmiEndpoint QmiEndpoint;
typedef struct _QmiEndpointClass QmiEndpointClass;
typedef struct _QmiEndpointPrivate QmiEndpointPrivate;

#define QMI_ENDPOINT_FILE            "endpoint-file"
#define QMI_ENDPOINT_SIGNAL_NEW_DATA "new-data"
#define QMI_ENDPOINT_SIGNAL_HANGUP   "hangup"

struct _QmiEndpoint {
    /*< private >*/
    GObject parent;
    QmiEndpointPrivate *priv;
};

struct _QmiEndpointClass {
    /*< private >*/
    GObjectClass parent;

    /* low level I/O primitives */
    void (* open)            (QmiEndpoint         *self,
                              gboolean             use_proxy,
                              guint                timeout,
                              GCancellable        *cancellable,
                              GAsyncReadyCallback  callback,
                              gpointer             user_data);
    gboolean (* open_finish) (QmiEndpoint   *self,
                              GAsyncResult  *res,
                              GError       **error);

    gboolean (* is_open) (QmiEndpoint *self);

    void (* setup_indications)            (QmiEndpoint         *self,
                                           guint                timeout,
                                           GCancellable        *cancellable,
                                           GAsyncReadyCallback  callback,
                                           gpointer             user_data);
    gboolean (* setup_indications_finish) (QmiEndpoint   *self,
                                           GAsyncResult  *res,
                                           GError       **error);

    gboolean (* send) (QmiEndpoint   *self,
                       QmiMessage    *message,
                       guint          timeout,
                       GCancellable  *cancellable,
                       GError       **error);

    void (* close)            (QmiEndpoint         *self,
                               guint                timeout,
                               GCancellable        *cancellable,
                               GAsyncReadyCallback  callback,
                               gpointer             user_data);
    gboolean (* close_finish) (QmiEndpoint   *self,
                               GAsyncResult  *res,
                               GError       **error);
};

GType qmi_endpoint_get_type (void);
G_DEFINE_AUTOPTR_CLEANUP_FUNC (QmiEndpoint, g_object_unref)

const gchar *qmi_endpoint_get_name (QmiEndpoint *self);

void qmi_endpoint_open (QmiEndpoint         *self,
                        gboolean             use_proxy,
                        guint                timeout,
                        GCancellable        *cancellable,
                        GAsyncReadyCallback  callback,
                        gpointer             user_data);
gboolean qmi_endpoint_open_finish (QmiEndpoint   *self,
                                   GAsyncResult  *res,
                                   GError       **error);

gboolean qmi_endpoint_is_open (QmiEndpoint *self);

void qmi_endpoint_setup_indications (QmiEndpoint         *self,
                                     guint                timeout,
                                     GCancellable        *cancellable,
                                     GAsyncReadyCallback  callback,
                                     gpointer             user_data);
gboolean qmi_endpoint_setup_indications_finish (QmiEndpoint   *self,
                                                GAsyncResult  *res,
                                                GError       **error);

gboolean qmi_endpoint_send (QmiEndpoint   *self,
                            QmiMessage    *message,
                            guint          timeout,
                            GCancellable  *cancellable,
                            GError       **error);

void qmi_endpoint_close (QmiEndpoint         *self,
                         guint                timeout,
                         GCancellable        *cancellable,
                         GAsyncReadyCallback  callback,
                         gpointer             user_data);
gboolean qmi_endpoint_close_finish (QmiEndpoint   *self,
                                    GAsyncResult  *res,
                                    GError       **error);

/*
 * Parse all messages, calling @handler on each one while also passing
 * along @user_data.
 *
 * If it hits an unrecoverable error such as a framing issue, returns
 * false and sets @error. Otherwise, returns true.
 */
gboolean qmi_endpoint_parse_buffer (QmiEndpoint *self,
                                    QmiMessageHandler handler,
                                    gpointer user_data,
                                    GError **error);

/*
 * Adds the message in @buf to the buffer.
 *
 * This function should only be called by subclasses when they receive
 * something on the underlying transport.
 */
void qmi_endpoint_add_message (QmiEndpoint *self,
                               const guint8 *buf,
                               guint len);

#endif /* _LIBQMI_GLIB_QMI_ENDPOINT_H_ */
