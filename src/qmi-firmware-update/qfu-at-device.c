/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * Qfu-firmware-update -- Command line tool to update firmware in QFU devices
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright (C) 2016 Zodiac Inflight Innovations
 * Copyright (C) 2016-2017 Aleksander Morgado <aleksander@aleksander.es>
 */

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

#include <glib-object.h>
#include <gio/gio.h>

#include <libqmi-glib.h>

#include "qfu-log.h"
#include "qfu-at-device.h"
#include "qfu-utils.h"

#define QFU_AT_BUFFER_SIZE 128

static void initable_iface_init (GInitableIface *iface);

G_DEFINE_TYPE_EXTENDED (QfuAtDevice, qfu_at_device, G_TYPE_OBJECT, 0,
                        G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE, initable_iface_init))

enum {
    PROP_0,
    PROP_FILE,
    PROP_LAST
};

static GParamSpec *properties[PROP_LAST];

struct _QfuAtDevicePrivate {
    GFile      *file;
    gchar      *name;
    gint        fd;
    GByteArray *buffer;
};

/******************************************************************************/
/* Send */

static gboolean
send_request (QfuAtDevice   *self,
              const gchar   *request,
              GCancellable  *cancellable,
              GError       **error)
{
    gssize         wlen;
    fd_set         wr;
    gint           aux;
    struct timeval tv = {
        .tv_sec = 2,
        .tv_usec = 0,
    };

    /* Wait for the fd to be writable and don't wait forever */
    FD_ZERO (&wr);
    FD_SET (self->priv->fd, &wr);
    aux = select (self->priv->fd + 1, NULL, &wr, NULL, &tv);

    if (g_cancellable_set_error_if_cancelled (cancellable, error))
        return FALSE;

    if (aux < 0) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "error waiting to write: %s",
                     g_strerror (errno));
        return FALSE;
    }

    if (aux == 0) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "timed out waiting to write");
        return FALSE;
    }

    /* Debug output */
    if (qfu_log_get_verbose ())
        g_debug ("[qfu-at-device,%s] >> %s", self->priv->name, request);

    wlen = write (self->priv->fd, request, strlen (request));
    if (wlen < 0) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "error writting: %s",
                     g_strerror (errno));
        return FALSE;
    }

    /* We treat EINTR as an error, so we also treat as an error if not all bytes
     * were wlen */
    if ((gsize)wlen != strlen (request)) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "error writing: only %" G_GSSIZE_FORMAT "/%" G_GSIZE_FORMAT " bytes written",
                     wlen, strlen (request));
        return FALSE;
    }

    wlen = write (self->priv->fd, "\r", 1);
    if (wlen < 0) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "error writting <CR>: %s",
                     g_strerror (errno));
        return FALSE;
    }

    return TRUE;
}

/******************************************************************************/
/* Receive */

static gssize
receive_response (QfuAtDevice   *self,
                  guint          timeout_secs,
                  gchar        **response,
                  GCancellable  *cancellable,
                  GError       **error)
{
    fd_set          rd;
    struct timeval  tv;
    gint            aux;
    gssize          rlen;
    gchar          *start;

    /* Use requested timeout */
    tv.tv_sec  = timeout_secs;
    tv.tv_usec = 0;

    FD_ZERO (&rd);
    FD_SET (self->priv->fd, &rd);
    aux = select (self->priv->fd + 1, &rd, NULL, NULL, &tv);

    if (g_cancellable_set_error_if_cancelled (cancellable, error))
        return -1;

    if (aux < 0) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "error waiting to read response: %s",
                     g_strerror (errno));
        return -1;
    }

    if (aux == 0) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "timed out waiting for the response");
        return -1;
    }

    /* Receive in the primary buffer */
    rlen = read (self->priv->fd, self->priv->buffer->data, self->priv->buffer->len);
    if (rlen < 0) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "couldn't read response: %s",
                     g_strerror (errno));
        return -1;
    }

    if (rlen == 0) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "couldn't read response: HUP detected");
        return -1;
    }

    /* Force end of string */
    start = (gchar *) &self->priv->buffer->data[0];
    start[rlen] = '\0';

    /* Remove trailing LFs and CRs */
    while (rlen > 0 && (start[rlen - 1] == '\r' || start[rlen - 1] == '\n'))
        start[--rlen] = '\0';

    /* Remove heading LFs and CRs */
    while (rlen > 0 && (start[0] == '\r' || start[0] == '\n')) {
        start++;
        rlen--;
    }

    /* Debug output */
    if (qfu_log_get_verbose ())
        g_debug ("[qfu-at-device,%s] << %s", self->priv->name, start);

    if (response)
        *response = start;

    return rlen;
}

/******************************************************************************/
/* Send/receive */

static gssize
send_receive (QfuAtDevice   *self,
              const gchar   *request,
              guint          response_timeout_secs,
              gchar        **response,
              GCancellable  *cancellable,
              GError       **error)
{
    gboolean sent;

    if (self->priv->fd < 0) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "device is closed");
        return FALSE;
    }

    sent = send_request (self, request, cancellable, error);
    if (!sent)
        return -1;

    if (!response)
        return 0;

    while (1) {
        gssize rsplen;

        rsplen = receive_response (self, response_timeout_secs, response, cancellable, error);
        if (rsplen > 0 && g_str_has_prefix (*response, request)) {
            *response += strlen (request);
            rsplen -= strlen (request);
            if (rsplen == 0)
                continue;
        }

        return rsplen;
    }
}

/******************************************************************************/

gboolean
qfu_at_device_boothold (QfuAtDevice  *self,
                        GCancellable  *cancellable,
                        GError       **error)
{
    gssize  rsplen;
    gchar  *rsp = NULL;

    rsplen = send_receive (self, "AT!BOOTHOLD", 3, &rsp, cancellable, error);
    if (rsplen < 0)
        return FALSE;

    if (strstr (rsp, "OK"))
        return TRUE;

    if (strstr (rsp, "ERROR"))
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "unknown command");
    else
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "unknown error");
    return FALSE;
}

/******************************************************************************/

static gboolean
initable_init (GInitable     *initable,
               GCancellable  *cancellable,
               GError       **error)
{
    QfuAtDevice    *self;
    struct termios  terminal_data;
    gchar          *path = NULL;
    GError         *inner_error = NULL;

    self = QFU_AT_DEVICE (initable);

    if (g_cancellable_set_error_if_cancelled (cancellable, &inner_error))
        goto out;

    path = g_file_get_path (self->priv->file);
    g_debug ("[qfu-at-device,%s] opening TTY", self->priv->name);

    self->priv->fd = open (path, O_RDWR | O_NOCTTY);
    if (self->priv->fd < 0) {
        inner_error = g_error_new (G_IO_ERROR, G_IO_ERROR_FAILED,
                                   "error opening serial device: %s",
                                   g_strerror (errno));
        goto out;
    }

    g_debug ("[qfu-at-device,%s] setting up serial port...", self->priv->name);
    if (tcgetattr (self->priv->fd, &terminal_data) < 0) {
        inner_error = g_error_new (G_IO_ERROR, G_IO_ERROR_FAILED,
                                   "error getting serial port attributes: %s",
                                   g_strerror (errno));
        goto out;
    }

    terminal_data.c_iflag &= ~(IGNCR | ICRNL | IUCLC | INPCK | IXON | IXANY );
    terminal_data.c_oflag &= ~(OPOST | OLCUC | OCRNL | ONLCR | ONLRET);
    terminal_data.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHONL);
    terminal_data.c_cc[VMIN] = 1;
    terminal_data.c_cc[VTIME] = 0;
    terminal_data.c_cc[VEOF] = 1;
    terminal_data.c_iflag |= (IXON | IXOFF | IXANY | IGNPAR);
    terminal_data.c_cflag &= ~(CBAUD | CSIZE | CSTOPB | PARENB | CRTSCTS);
    terminal_data.c_cflag |= (CS8 | CREAD | CLOCAL); /* 8N1 */

    errno = 0;
    if (cfsetispeed (&terminal_data, B115200) != 0) {
        inner_error = g_error_new (G_IO_ERROR, G_IO_ERROR_FAILED,
                                   "failed to set serial port input speed: %s",
                                   g_strerror (errno));
        goto out;
    }

    errno = 0;
    if (cfsetospeed (&terminal_data, B115200) != 0) {
        inner_error = g_error_new (G_IO_ERROR, G_IO_ERROR_FAILED,
                                   "failed to set serial port output speed: %s",
                                   g_strerror (errno));
        goto out;
    }

    if (tcsetattr (self->priv->fd, TCSANOW, &terminal_data) < 0) {
        inner_error = g_error_new (G_IO_ERROR, G_IO_ERROR_FAILED,
                                   "error setting serial port attributes: %s",
                                   g_strerror (errno));
        goto out;
    }

out:
    g_free (path);

    if (inner_error) {
        if (!(self->priv->fd < 0)) {
            close (self->priv->fd);
            self->priv->fd = -1;
        }
        g_propagate_error (error, inner_error);
        return FALSE;
    }

    return TRUE;
}

/******************************************************************************/

const gchar *
qfu_at_device_get_name (QfuAtDevice *self)
{
    return self->priv->name;
}

/******************************************************************************/

QfuAtDevice *
qfu_at_device_new (GFile         *file,
                   GCancellable  *cancellable,
                   GError       **error)
{
    g_return_val_if_fail (G_IS_FILE (file), NULL);

    return QFU_AT_DEVICE (g_initable_new (QFU_TYPE_AT_DEVICE,
                                           cancellable,
                                           error,
                                           "file", file,
                                           NULL));
}


static void
qfu_at_device_init (QfuAtDevice *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, QFU_TYPE_AT_DEVICE, QfuAtDevicePrivate);
    self->priv->fd = -1;
    /* Long buffer for I/O */
    self->priv->buffer = g_byte_array_new ();
    g_byte_array_set_size (self->priv->buffer, QFU_AT_BUFFER_SIZE);
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    QfuAtDevice *self = QFU_AT_DEVICE (object);

    switch (prop_id) {
    case PROP_FILE:
        self->priv->file = g_value_dup_object (value);
        if (self->priv->file)
            self->priv->name = g_file_get_basename (self->priv->file);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
get_property (GObject    *object,
              guint       prop_id,
              GValue     *value,
              GParamSpec *pspec)
{
    QfuAtDevice *self = QFU_AT_DEVICE (object);

    switch (prop_id) {
    case PROP_FILE:
        g_value_set_object (value, self->priv->file);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
dispose (GObject *object)
{
    QfuAtDevice *self = QFU_AT_DEVICE (object);

    if (!(self->priv->fd < 0)) {
        close (self->priv->fd);
        self->priv->fd = -1;
    }
    g_clear_pointer (&self->priv->name,   g_free);
    g_clear_pointer (&self->priv->buffer, g_byte_array_unref);
    g_clear_object  (&self->priv->file);

    G_OBJECT_CLASS (qfu_at_device_parent_class)->dispose (object);
}

static void
initable_iface_init (GInitableIface *iface)
{
    iface->init = initable_init;
}

static void
qfu_at_device_class_init (QfuAtDeviceClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (object_class, sizeof (QfuAtDevicePrivate));

    object_class->dispose      = dispose;
    object_class->get_property = get_property;
    object_class->set_property = set_property;

    properties[PROP_FILE] =
        g_param_spec_object ("file",
                             "File",
                             "File object",
                             G_TYPE_FILE,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    g_object_class_install_property (object_class, PROP_FILE, properties[PROP_FILE]);
}
