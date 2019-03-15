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

#include <glib.h>

#include "qmi-message.h"

typedef void (*QmiMessageHandler) (QmiMessage *message,
                                   gpointer user_data);

/**
 * SECTION:qmi-endpoint
 * @title: QmiEndpoint
 * @short_description: Transport-level abstraction for QMI
 *
 * #QmiEndpoint handles the low-level details of sending and receiving QMI
 * messages to the modem.
 */

#define QMI_TYPE_ENDPOINT            (qmi_endpoint_get_type ())
#define QMI_ENDPOINT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), QMI_TYPE_ENDPOINT, QmiEndpoint))
#define QMI_ENDPOINT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  QMI_TYPE_ENDPOINT, QmiEndpointClass))
#define QMI_IS_ENDPOINT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), QMI_TYPE_ENDPOINT))
#define QMI_IS_ENDPOINT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  QMI_TYPE_ENDPOINT))
#define QMI_ENDPOINT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  QMI_TYPE_ENDPOINT, QmiEndpointClass))

typedef struct _QmiEndpoint QmiEndpoint;
typedef struct _QmiEndpointClass QmiEndpointClass;
typedef struct _QmiEndpointPrivate QmiEndpointPrivate;

/**
 * QMI_ENDPOINT_SIGNAL_NEW_DATA:
 *
 * Symbol defining the #QmiEndpoint::new_data signal.
 */
#define QMI_ENDPOINT_SIGNAL_NEW_DATA "new-data"

/**
 * QMI_ENDPOINT_SIGNAL_HANGUP:
 *
 * Symbol defining the #QmiEndpoint::hangup signal.
 */
#define QMI_ENDPOINT_SIGNAL_HANGUP "hangup"

struct _QmiEndpoint {
    /*< private >*/
    GObject parent;
    QmiEndpointPrivate *priv;
};

struct _QmiEndpointClass {
    /*< private >*/
    GObjectClass parent;
};

GType qmi_endpoint_get_type (void);

QmiEndpoint *qmi_endpoint_new (void);

/**
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

/**
 * Adds the message in @buf to the buffer.
 */
void qmi_endpoint_add_message (QmiEndpoint *self,
                               const guint8 *buf,
                               guint len);

/**
 * Signals that the endpoint has hung up.
 *
 * Transitional function until the transport is fully controlled by the
 * #QmiEndpoint class.
 */
void __qmi_endpoint_hangup (QmiEndpoint *self);

#endif /* _LIBQMI_GLIB_QMI_ENDPOINT_H_ */
