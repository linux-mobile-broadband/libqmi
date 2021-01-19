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

#include "qmi-helpers.h"
#include "qmi-error-types.h"
#include "qmi-errors.h"

G_DEFINE_TYPE (QmiEndpoint, qmi_endpoint, G_TYPE_OBJECT)

struct _QmiEndpointPrivate {
    GByteArray *buffer;
    QmiFile *file;
};

enum {
    PROP_0,
    PROP_FILE,
    PROP_LAST
};

enum {
    SIGNAL_NEW_DATA,
    SIGNAL_HANGUP,
    SIGNAL_LAST
};

static GParamSpec *properties[PROP_LAST];
static guint signals[SIGNAL_LAST] = { 0 };

/*****************************************************************************/

gboolean
qmi_endpoint_parse_buffer (QmiEndpoint        *self,
                           QmiMessageHandler   handler,
                           gpointer            user_data,
                           GError            **error)
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
            g_signal_emit (self, signals[SIGNAL_HANGUP], 0);
            return FALSE;
        }

        message = qmi_message_new_from_raw (self->priv->buffer, &inner_error);
        if (!message) {
            if (!inner_error)
                /* More data we need */
                return TRUE;

            /* Warn about the issue */
            g_warning ("[%s] Invalid QMI message received: '%s'",
                       qmi_file_get_path_display (self->priv->file),
                       inner_error->message);
            g_error_free (inner_error);

            if (qmi_utils_get_traces_enabled ()) {
                gchar *printable;
                guint len = MIN (self->priv->buffer->len, 2048);

                printable = qmi_helpers_str_hex (self->priv->buffer->data, len, ':');
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

void
qmi_endpoint_add_message (QmiEndpoint  *self,
                          const guint8 *data,
                          guint         len)
{
    self->priv->buffer = g_byte_array_append (self->priv->buffer, data, len);
    g_signal_emit (self, signals[SIGNAL_NEW_DATA], 0);
}

/*****************************************************************************/

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

    task = g_task_new (self, cancellable, callback, user_data);

    g_task_return_boolean (task, TRUE);
    g_object_unref (task);
}

/*****************************************************************************/

const gchar *
qmi_endpoint_get_name (QmiEndpoint *self)
{
    return qmi_file_get_path_display (self->priv->file);
}

gboolean
qmi_endpoint_open_finish (QmiEndpoint   *self,
                          GAsyncResult  *res,
                          GError       **error)
{
    return QMI_ENDPOINT_GET_CLASS (self)->open_finish (self, res, error);
}

void
qmi_endpoint_open (QmiEndpoint         *self,
                   gboolean             use_proxy,
                   guint                timeout,
                   GCancellable        *cancellable,
                   GAsyncReadyCallback  callback,
                   gpointer             user_data)
{
    g_assert (QMI_ENDPOINT_GET_CLASS (self)->open &&
              QMI_ENDPOINT_GET_CLASS (self)->open_finish);

    return QMI_ENDPOINT_GET_CLASS (self)->open (self,
                                                use_proxy,
                                                timeout,
                                                cancellable,
                                                callback,
                                                user_data);
}

gboolean
qmi_endpoint_is_open (QmiEndpoint *self)
{
    g_assert (QMI_ENDPOINT_GET_CLASS (self)->is_open);

    return QMI_ENDPOINT_GET_CLASS (self)->is_open (self);
}

gboolean
qmi_endpoint_setup_indications_finish (QmiEndpoint   *self,
                                       GAsyncResult  *res,
                                       GError       **error)
{
    return QMI_ENDPOINT_GET_CLASS (self)->setup_indications_finish (self, res, error);
}

void
qmi_endpoint_setup_indications (QmiEndpoint         *self,
                                guint                timeout,
                                GCancellable        *cancellable,
                                GAsyncReadyCallback  callback,
                                gpointer             user_data)
{
    g_assert (QMI_ENDPOINT_GET_CLASS (self)->setup_indications &&
              QMI_ENDPOINT_GET_CLASS (self)->setup_indications_finish);

    return QMI_ENDPOINT_GET_CLASS (self)->setup_indications (self,
                                                             timeout,
                                                             cancellable,
                                                             callback,
                                                             user_data);
}

gboolean
qmi_endpoint_send (QmiEndpoint   *self,
                   QmiMessage    *message,
                   guint          timeout,
                   GCancellable  *cancellable,
                   GError       **error)
{
    g_assert (QMI_ENDPOINT_GET_CLASS (self)->send);

    return QMI_ENDPOINT_GET_CLASS (self)->send (self, message, timeout, cancellable, error);
}

gboolean
qmi_endpoint_close_finish (QmiEndpoint   *self,
                           GAsyncResult  *res,
                           GError       **error)
{
    return QMI_ENDPOINT_GET_CLASS (self)->close_finish (self, res, error);
}

void
qmi_endpoint_close (QmiEndpoint         *self,
                    guint                timeout,
                    GCancellable        *cancellable,
                    GAsyncReadyCallback  callback,
                    gpointer             user_data)
{
    g_assert (QMI_ENDPOINT_GET_CLASS (self)->close &&
              QMI_ENDPOINT_GET_CLASS (self)->close_finish);

    return QMI_ENDPOINT_GET_CLASS (self)->close (self,
                                                 timeout,
                                                 cancellable,
                                                 callback,
                                                 user_data);
}

/*****************************************************************************/

static void
set_property (GObject *object,
              guint prop_id,
              const GValue *value,
              GParamSpec *pspec)
{
    QmiEndpoint *self = QMI_ENDPOINT (object);

    switch (prop_id) {
    case PROP_FILE:
        g_assert (self->priv->file == NULL);
        self->priv->file = g_value_dup_object (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
get_property (GObject *object,
              guint prop_id,
              GValue *value,
              GParamSpec *pspec)
{
    QmiEndpoint *self = QMI_ENDPOINT (object);

    switch (prop_id) {
    case PROP_FILE:
        g_value_set_object (value, self->priv->file);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
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
    g_clear_object (&self->priv->file);

    G_OBJECT_CLASS (qmi_endpoint_parent_class)->dispose (object);
}

static void
qmi_endpoint_class_init (QmiEndpointClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (object_class, sizeof (QmiEndpointPrivate));

    object_class->get_property = get_property;
    object_class->set_property = set_property;
    object_class->dispose = dispose;

    klass->setup_indications = endpoint_setup_indications;
    klass->setup_indications_finish = endpoint_setup_indications_finish;

    /**
     * QmiEndpoint:endpoint-file:
     */
    properties[PROP_FILE] =
        g_param_spec_object (QMI_ENDPOINT_FILE,
                             "Device file",
                             "File to the underlying QMI device",
                             QMI_TYPE_FILE,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    g_object_class_install_property (object_class, PROP_FILE, properties[PROP_FILE]);

    /**
     * QmiEndpoint::new-data:
     * @object: A #QmiEndpoint.
     * @output: none
     *
     * The ::new-data signal is emitted when the endpoint receives data.
     */
    signals[SIGNAL_NEW_DATA] =
        g_signal_new (QMI_ENDPOINT_SIGNAL_NEW_DATA,
                      G_OBJECT_CLASS_TYPE (G_OBJECT_CLASS (klass)),
                      G_SIGNAL_RUN_LAST,
                      0,
                      NULL,
                      NULL,
                      NULL,
                      G_TYPE_NONE,
                      0);

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
