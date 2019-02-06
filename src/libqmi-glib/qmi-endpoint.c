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

#include "qmi-endpoint.h"

#include "qmi-error-types.h"
#include "qmi-errors.h"

G_DEFINE_TYPE (QmiEndpoint, qmi_endpoint, G_TYPE_OBJECT)

struct _QmiEndpointPrivate {
    GByteArray *buffer;
};

enum {
    SIGNAL_HANGUP,
    SIGNAL_LAST
};

static guint signals[SIGNAL_LAST] = { 0 };

/*****************************************************************************/

gboolean
qmi_endpoint_parse_buffer (QmiEndpoint *self,
                           QmiMessageHandler handler,
                           gpointer user_data,
                           GError **error)
{
    do {
        GError *inner_error = NULL;
        QmiMessage *message;

        /* Every message received must start with the QMUX marker.
         * If it doesn't, we broke framing :-/
         * If we broke framing, an error should be reported and the device
         * should get closed */
        if (self->priv->buffer->len > 0 &&
            self->priv->buffer->data[0] != QMI_MESSAGE_QMUX_MARKER) {
            g_set_error (error,
                         QMI_PROTOCOL_ERROR,
                         QMI_PROTOCOL_ERROR_MALFORMED_MESSAGE,
                         "QMI framing error detected");
            __qmi_endpoint_hangup (self);
            return FALSE;
        }

        message = qmi_message_new_from_raw (self->priv->buffer, &inner_error);
        if (!message) {
            if (!inner_error)
                /* More data we need */
                return TRUE;

            /* Warn about the issue */
            g_warning ("Invalid QMI message received: '%s'",
                       inner_error->message);
            g_error_free (inner_error);

            if (qmi_utils_get_traces_enabled ()) {
                gchar *printable;
                guint len = MIN (self->priv->buffer->len, 2048);

                printable = __qmi_utils_str_hex (self->priv->buffer->data,
                                                 len, ':');
                g_debug ("<<<<<< RAW INVALID MESSAGE:\n"
                         "<<<<<<   length = %u\n"
                         "<<<<<<   data   = %s\n",
                         self->priv->buffer->len, /* show full buffer len */
                         printable);
                g_free (printable);
            }

        } else {
            /* Play with the received message */
            handler (message, user_data);
            qmi_message_unref (message);
        }
    } while (self->priv->buffer->len > 0);

    return TRUE;
}

void qmi_endpoint_add_message (QmiEndpoint *self,
                               const guint8 *data,
                               guint len)
{
    self->priv->buffer = g_byte_array_append (self->priv->buffer, data, len);
}

void __qmi_endpoint_hangup (QmiEndpoint *self)
{
    g_signal_emit (self, signals[SIGNAL_HANGUP], 0);
}

/*****************************************************************************/

QmiEndpoint *
qmi_endpoint_new (void)
{
    return g_object_new (QMI_TYPE_ENDPOINT, NULL);
}

/*****************************************************************************/

static void
qmi_endpoint_init (QmiEndpoint *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE ((self),
                                              QMI_TYPE_ENDPOINT,
                                              QmiEndpointPrivate);

    self->priv->buffer = g_byte_array_new ();
}

static void
dispose (GObject *object)
{
    QmiEndpoint *self = QMI_ENDPOINT (object);

    g_clear_pointer (&self->priv->buffer, g_byte_array_unref);

    G_OBJECT_CLASS (qmi_endpoint_parent_class)->dispose (object);
}

static void
qmi_endpoint_class_init (QmiEndpointClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (object_class, sizeof (QmiEndpointPrivate));

    object_class->dispose = dispose;

    /**
     * QmiEndpoint::hangup:
     * @object: A #QmiEndpoint.
     * @output: none
     *
     * The ::endpoint signal is emitted when an unexpected port hang-up is received.
     */
    signals[SIGNAL_HANGUP] =
        g_signal_new (QMI_ENDPOINT_SIGNAL_HANGUP,
                      G_OBJECT_CLASS_TYPE (G_OBJECT_CLASS (klass)),
                      G_SIGNAL_RUN_LAST,
                      0,
                      NULL,
                      NULL,
                      NULL,
                      G_TYPE_NONE,
                      0);
}
