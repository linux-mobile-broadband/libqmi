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
 * Copyright (C) 2016 Bjørn Mork <bjorn@mork.no>
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

#include "qfu-log.h"
#include "qfu-qdl-message.h"
#include "qfu-dload-message.h"
#include "qfu-qdl-device.h"
#include "qfu-utils.h"
#include "qfu-enum-types.h"

static void initable_iface_init (GInitableIface *iface);

G_DEFINE_TYPE_EXTENDED (QfuQdlDevice, qfu_qdl_device, G_TYPE_OBJECT, 0,
                        G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE, initable_iface_init))

enum {
    PROP_0,
    PROP_FILE,
    PROP_LAST
};

static GParamSpec *properties[PROP_LAST];

#define SECONDARY_BUFFER_DEFAULT_SIZE 512
#define MAX_PRINTABLE_SIZE            80

struct _QfuQdlDevicePrivate {
    GFile      *file;
    gint        fd;
    guint       qdl_version;
    GByteArray *buffer;
    GByteArray *secondary_buffer;
};

/******************************************************************************/
/* HDLC */

#define CONTROL 0x7e
#define ESCAPE  0x7d
#define MASK    0x20

static gsize
escape (const guint8 *in,
        gsize         inlen,
        guint8       *out,
        gsize         outlen)
{
    gsize i, j;

    for (i = 0, j = 0; i < inlen; i++) {
        /* Caller should give a big enough buffer */
        g_assert ((j + 1) < outlen);
        if (in[i] == CONTROL || in[i] == ESCAPE) {
            out[j++] = ESCAPE;
            out[j++] = in[i] ^ MASK;
        } else
            out[j++] = in[i];
    }
    return j;
}

static gsize
unescape (const guint8 *in,
          gsize         inlen,
          guint8       *out,
          gsize         outlen)
{
    gsize    i, j = 0;
    gboolean escaping = FALSE;

    for (i = 0; i < inlen; i++) {
        /* Caller should give a big enough buffer */
        g_assert (j < outlen);
        if (escaping) {
            out[j++] = in[i] ^ MASK;
            escaping = FALSE;
        } else if (in[i] == ESCAPE) {
            escaping = TRUE;
        } else {
            out[j++] = in[i];
        }
    }

    return j;
}

/* copy a possibly escaped single byte to out */
static gsize
escape_byte (guint8  byte,
             guint8 *out,
             gsize   outlen)
{
    gsize j = 0;

    if (byte == CONTROL || byte == ESCAPE) {
        out[j++] = ESCAPE;
        byte ^= MASK;
    }
    out[j++] = byte;
    return j;
}

static gsize
hdlc_max_framed_size (gsize unframed_size)
{
    /* 1 header byte, (2 * input size) bytes, 2 crc bytes and 1 trailing byte */
    return 4 + (2 * unframed_size);
}

static gsize
hdlc_frame (const guint8 *in,
            gsize         inlen,
            guint8       *out,
            gsize         outlen)
{
    guint16 crc;
    gsize j = 0;

    out[j++] = CONTROL;
    j += escape (in, inlen, &out[j], outlen - j);
    crc = qfu_utils_crc16 (in, inlen);
    j += escape_byte (crc & 0xff, &out[j], outlen - j);
    j += escape_byte (crc >> 8 & 0xff, &out[j], outlen - j);
    out[j++] = CONTROL;

    return j;
}

static gsize
hdlc_max_unframed_size (gsize framed_size)
{
    /* -1 header byte, -2 crc bytes and -1 trailing byte */
    g_assert (framed_size > 3);
    return framed_size - 3;
}

static gsize
hdlc_unframe (const guint8 *in,
              gsize         inlen,
              guint8       *out,
              gsize         outlen,
              GError      **error)
{
    guint16 crc;
    gsize j, i = inlen;

    /* the first control char is optional */
    if (*in == CONTROL) {
        in++;
        i--;
    }
    if (in[i - 1] == CONTROL)
        i--;

    j = unescape (in, i, out, outlen);
    if (j < 2) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "unescaping failed: too few bytes as output: %" G_GSIZE_FORMAT, j);
        return 0;
    }
    j -= 2; /* remove the crc */

    /* verify the crc */
    crc = qfu_utils_crc16 (out, j);
    if (crc != (out[j] | out[j + 1] << 8)) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "crc check failed: 0x%04x != 0x%04x\n", crc, out[j] | out[j + 1] << 8);
        return 0;
    }

    return j;
}

/******************************************************************************/
/* Send */

static gboolean
send_request (QfuQdlDevice  *self,
              const guint8  *request,
              gsize          request_size,
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
    if (qfu_log_get_verbose ()) {
        gchar    *printable;
        gsize     printable_size = request_size;
        gboolean  shorted = FALSE;

        if (printable_size > MAX_PRINTABLE_SIZE) {
            printable_size = MAX_PRINTABLE_SIZE;
            shorted = TRUE;
        }

        printable = qfu_utils_str_hex (request, printable_size, ':');
        g_debug ("[qfu-qdl-device] >> %s%s [%" G_GSIZE_FORMAT "]", printable, shorted ? "..." : "", request_size);
        g_free (printable);
    }

    wlen = write (self->priv->fd, request, request_size);
    if (wlen < 0) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "error writting: %s",
                     g_strerror (errno));
        return FALSE;
    }

    /* We treat EINTR as an error, so we also treat as an error if not all bytes
     * were wlen */
    if ((gsize)wlen != request_size) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "error writing: only %" G_GSSIZE_FORMAT "/%" G_GSIZE_FORMAT " bytes written",
                     wlen, request_size);
        return FALSE;
    }

    return TRUE;
}

static gboolean
send_framed_request (QfuQdlDevice  *self,
                     const guint8  *request,
                     gsize          request_size,
                     GCancellable  *cancellable,
                     GError       **error)
{
    gsize max_framed_size;
    gsize framed_size;

    /* Debug output */
    if (qfu_log_get_verbose ()) {
        gchar    *printable;
        gsize     printable_size = request_size;
        gboolean  shorted = FALSE;

        if (printable_size > MAX_PRINTABLE_SIZE) {
            printable_size = MAX_PRINTABLE_SIZE;
            shorted = TRUE;
        }

        printable = qfu_utils_str_hex (request, printable_size, ':');
        g_debug ("[qfu-qdl-device] >> %s%s [%" G_GSIZE_FORMAT ", unframed]", printable, shorted ? "..." : "", request_size);
        g_free (printable);
    }

    max_framed_size = hdlc_max_framed_size (request_size);
    if (G_UNLIKELY (max_framed_size > self->priv->secondary_buffer->len))
        g_byte_array_set_size (self->priv->secondary_buffer, max_framed_size);

    /* Pack into an HDLC frame */
    framed_size = hdlc_frame (request, request_size, self->priv->secondary_buffer->data, self->priv->secondary_buffer->len);
    g_assert (framed_size > 0);

    return send_request (self, self->priv->secondary_buffer->data, framed_size, cancellable, error);
}

/******************************************************************************/
/* Receive */

static gssize
receive_response (QfuQdlDevice  *self,
                  guint          timeout_secs,
                  guint8       **response,
                  GCancellable  *cancellable,
                  GError       **error)
{
    fd_set         rd;
    struct timeval tv;
    gint           aux;
    gssize         rlen;
    guint8        *end;
    gssize         frame_size;
    gsize          max_unframed_size;
    gsize          unframed_size;

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

    /* Debug output */
    if (qfu_log_get_verbose ()) {
        gchar    *printable;
        gsize     printable_size = rlen;
        gboolean  shorted = FALSE;

        if (printable_size > MAX_PRINTABLE_SIZE) {
            printable_size = MAX_PRINTABLE_SIZE;
            shorted = TRUE;
        }

        printable = qfu_utils_str_hex (self->priv->buffer->data, printable_size, ':');
        g_debug ("[qfu-qdl-device] << %s%s [%" G_GSIZE_FORMAT "]", printable, shorted ? "..." : "", rlen);
        g_free (printable);
    }

    end = memchr (self->priv->buffer->data + 1, CONTROL, rlen - 1);
    if (!end) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "HDLC trailing control character not found");
        return -1;
    }

    frame_size = end - self->priv->buffer->data + 1;
    g_assert (frame_size >= 0);
    g_assert (frame_size <= rlen);
    if (frame_size < 5) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "minimum HDLC frame size not received");
        return -1;
    }

    if (frame_size < rlen)
        g_debug ("[qfu-qdl-device] received %" G_GSSIZE_FORMAT " trailing bytes after HDLC frame (ignored)",
                 rlen - frame_size);

    max_unframed_size = hdlc_max_unframed_size (frame_size);
    if (G_UNLIKELY (max_unframed_size > self->priv->secondary_buffer->len))
        g_byte_array_set_size (self->priv->secondary_buffer, max_unframed_size);

    unframed_size = hdlc_unframe (self->priv->buffer->data, (gsize)frame_size, self->priv->secondary_buffer->data, self->priv->secondary_buffer->len, error);
    if (unframed_size == 0) {
        g_prefix_error (error, "error unframing message: ");
        return -1;
    }

    /* Debug output */
    if (qfu_log_get_verbose ()) {
        gchar    *printable;
        gsize     printable_size = unframed_size;
        gboolean  shorted = FALSE;

        if (printable_size > MAX_PRINTABLE_SIZE) {
            printable_size = MAX_PRINTABLE_SIZE;
            shorted = TRUE;
        }

        printable = qfu_utils_str_hex (self->priv->secondary_buffer->data, printable_size, ':');
        g_debug ("[qfu-qdl-device] << %s%s [%" G_GSIZE_FORMAT ", unframed]", printable, shorted ? "..." : "", unframed_size);
        g_free (printable);
    }

    if (response)
        *response = self->priv->secondary_buffer->data;

    return unframed_size;
}

/******************************************************************************/
/* Send/receive */

static gssize
send_receive (QfuQdlDevice  *self,
              const guint8  *request,
              gsize          request_size,
              gboolean       request_framed,
              guint          response_timeout_secs,
              guint8       **response,
              GCancellable  *cancellable,
              GError       **error)
{
    gboolean sent;

    if (self->priv->fd < 0) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "device is closed");
        return FALSE;
    }

    if (request_framed)
        sent = send_framed_request (self, request, request_size, cancellable, error);
    else
        sent = send_request (self, request, request_size, cancellable, error);
    if (!sent)
        return -1;

    if (!response)
        return 0;

    return receive_response (self, response_timeout_secs, response, cancellable, error);
}

/******************************************************************************/

/******************************************************************************/

gboolean
qfu_qdl_device_ufopen (QfuQdlDevice  *self,
                       QfuImage      *image,
                       GCancellable  *cancellable,
                       GError       **error)
{
    gssize  reqlen;
    gssize  rsplen;
    guint8 *rsp = NULL;

    reqlen = qfu_qdl_request_ufopen_build (self->priv->buffer->data, self->priv->buffer->len, image, cancellable, error);
    if (reqlen < 0)
        return FALSE;

    rsplen = send_receive (self, self->priv->buffer->data, reqlen, TRUE, 1, &rsp, cancellable, error);
    if (rsplen < 0)
        return FALSE;

    switch (rsp[0]) {
    case QFU_QDL_CMD_OPEN_UNFRAMED_RSP:
        return qfu_qdl_response_ufopen_parse (rsp, rsplen, error);
    case QFU_QDL_CMD_ERROR:
        return qfu_qdl_response_error_parse (rsp, rsplen, error);
    default:
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "unexpected response received in ufopen: 0x%02x (%s)",
                     rsp[0], qfu_qdl_cmd_get_string (rsp[0]) ? qfu_qdl_cmd_get_string (rsp[0]) : "unknown");
        return FALSE;
    }
}

/******************************************************************************/

gboolean
qfu_qdl_device_ufwrite (QfuQdlDevice  *self,
                        QfuImage      *image,
                        guint16        sequence,
                        GCancellable  *cancellable,
                        GError       **error)
{
    gssize   reqlen;
    gssize   rsplen;
    guint8  *rsp = NULL;
    guint16  ack_sequence = 0;

    reqlen = qfu_qdl_request_ufwrite_build (self->priv->buffer->data, self->priv->buffer->len, image, sequence, cancellable, error);
    if (reqlen < 0)
        return FALSE;

    /* NOTE: the last chunk will require a long timeout, so just define the
     * same one for all chunks */
    rsplen = send_receive (self, self->priv->buffer->data, reqlen, FALSE, 120, &rsp, cancellable, error);
    if (rsplen < 0)
        return FALSE;

    switch (rsp[0]) {
    case QFU_QDL_CMD_WRITE_UNFRAMED_RSP:
        if (!qfu_qdl_response_ufwrite_parse (rsp, rsplen, &ack_sequence, error))
            return FALSE;
        if (ack_sequence != sequence) {
            g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                         "received ack for chunk #%" G_GUINT16_FORMAT " instead of chunk #%" G_GUINT16_FORMAT,
                         ack_sequence, sequence);
            return FALSE;
        }
        return TRUE;
    case QFU_QDL_CMD_ERROR:
        return qfu_qdl_response_error_parse (rsp, rsplen, error);
    default:
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "unexpected response received in ufwrite: 0x%02x (%s)",
                     rsp[0], qfu_qdl_cmd_get_string (rsp[0]) ? qfu_qdl_cmd_get_string (rsp[0]) : "unknown");
        return FALSE;
    }
}

/******************************************************************************/

gboolean
qfu_qdl_device_ufclose (QfuQdlDevice  *self,
                        GCancellable  *cancellable,
                        GError       **error)
{
    gsize   reqlen;
    gssize  rsplen;
    guint8 *rsp = NULL;

    reqlen = qfu_qdl_request_ufclose_build (self->priv->buffer->data, self->priv->buffer->len);
    rsplen = send_receive (self, self->priv->buffer->data, reqlen, TRUE, 1, &rsp, cancellable, error);
    if (rsplen < 0)
        return FALSE;

    switch (rsp[0]) {
    case QFU_QDL_CMD_CLOSE_UNFRAMED_RSP:
        return qfu_qdl_response_ufclose_parse (rsp, rsplen, error);
    case QFU_QDL_CMD_ERROR:
        return qfu_qdl_response_error_parse (rsp, rsplen, error);
    default:
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "unexpected response received in ufclose: 0x%02x (%s)",
                     rsp[0], qfu_qdl_cmd_get_string (rsp[0]) ? qfu_qdl_cmd_get_string (rsp[0]) : "unknown");
        return FALSE;
    }
}

/******************************************************************************/

gboolean
qfu_qdl_device_hello (QfuQdlDevice  *self,
                      GCancellable  *cancellable,
                      GError       **error)
{
    gsize   reqlen;
    gssize  rsplen;
    guint8 *rsp = NULL;

    g_assert (self->priv->qdl_version > 0);

    /* If no error, we assume version is found */
    reqlen = qfu_qdl_request_hello_build (self->priv->buffer->data, self->priv->buffer->len, self->priv->qdl_version, self->priv->qdl_version);
    rsplen = send_receive (self, self->priv->buffer->data, reqlen, TRUE, 1, &rsp, cancellable, error);
    if (rsplen < 0)
        return FALSE;

    switch (rsp[0]) {
    case QFU_QDL_CMD_HELLO_RSP:
        return qfu_qdl_response_hello_parse (rsp, rsplen, error);
    case QFU_QDL_CMD_ERROR:
        return qfu_qdl_response_error_parse (rsp, rsplen, error);
    default:
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "unexpected response received in hello: 0x%02x (%s)",
                     rsp[0], qfu_qdl_cmd_get_string (rsp[0]) ? qfu_qdl_cmd_get_string (rsp[0]) : "unknown");
        return FALSE;
    }
}

/******************************************************************************/

gboolean
qfu_qdl_device_reset (QfuQdlDevice  *self,
                      GCancellable  *cancellable,
                      GError       **error)
{
    gsize  reqlen;
    gssize rsplen;

    if (self->priv->fd < 0)
        return TRUE;

    reqlen = qfu_qdl_request_reset_build (self->priv->buffer->data, self->priv->buffer->len);
    rsplen = send_receive (self, self->priv->buffer->data, reqlen, TRUE, 0, NULL, cancellable, error);

    /* Close device after a reset, even if we got an error */
    close (self->priv->fd);
    self->priv->fd = -1;

    if (rsplen < 0)
        return FALSE;

    return TRUE;
}

/******************************************************************************/

static gboolean
qdl_device_dload_sdp (QfuQdlDevice  *self,
                      GCancellable  *cancellable,
                      GError       **error)
{
    gsize   reqlen;
    gssize  rsplen;
    guint8 *rsp = NULL;

    reqlen = qfu_dload_request_sdp_build (self->priv->buffer->data, self->priv->buffer->len);
    rsplen = send_receive (self, self->priv->buffer->data, reqlen, TRUE, 1, &rsp, cancellable, error);
    if (rsplen < 0)
        return FALSE;

    switch (rsp[0]) {
    case QFU_DLOAD_CMD_ACK:
        return qfu_dload_response_ack_parse (rsp, rsplen, error);
    case QFU_QDL_CMD_ERROR:
        return qfu_qdl_response_error_parse (rsp, rsplen, error);
    default:
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "unexpected response received in dload sdp: 0x%02x", rsp[0]);
        return FALSE;
    }

    return TRUE;
}

static gboolean
qdl_device_detect_version (QfuQdlDevice  *self,
                           GCancellable  *cancellable,
                           GError       **error)
{
    guint version;

#define MAX_VALID_VERSION 6

    /* Attempt to probe supported protocol version
     *  Newer modems like Sierra Wireless MC7710 must use '6' for both fields
     *  Gobi2000 modems like HP un2420 must use '5' for both fields
     *  Gobi1000 modems  must use '4' for both fields
     */
    for (version = 4; version <= MAX_VALID_VERSION; version++) {
        gsize   reqlen;
        gssize  rsplen;
        guint8 *rsp = NULL;

        /* If no error, we assume version is found */
        reqlen = qfu_qdl_request_hello_build (self->priv->buffer->data, self->priv->buffer->len, version, version);
        rsplen = send_receive (self, self->priv->buffer->data, reqlen, TRUE, 1, &rsp, cancellable, error);
        if (rsplen < 0)
            return FALSE;

        /* Break right away on a successful parse, so that we finish with the
         * correct version tested */
        if (qfu_qdl_response_hello_parse (rsp, rsplen, NULL))
            break;
    }

    if (version > MAX_VALID_VERSION) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "couldn't detect QDL version");
        return FALSE;
    }

    g_debug ("[qfu-qdl-device] QDL version detected: %u", version);
    self->priv->qdl_version = version;

    return TRUE;
}

static gboolean
initable_init (GInitable     *initable,
               GCancellable  *cancellable,
               GError       **error)
{
    QfuQdlDevice   *self;
    struct termios  terminal_data;
    gchar          *path = NULL;
    GError         *inner_error = NULL;

    self = QFU_QDL_DEVICE (initable);

    if (g_cancellable_set_error_if_cancelled (cancellable, &inner_error))
        goto out;

    path = g_file_get_path (self->priv->file);
    g_debug ("[qfu-qdl-device] opening TTY: %s", path);

    self->priv->fd = open (path, O_RDWR | O_NOCTTY);
    if (self->priv->fd < 0) {
        inner_error = g_error_new (G_IO_ERROR, G_IO_ERROR_FAILED,
                                   "error opening serial device: %s",
                                   g_strerror (errno));
        goto out;
    }

    g_debug ("[qfu-qdl-device] setting terminal in raw mode...");
    if (tcgetattr (self->priv->fd, &terminal_data) < 0) {
        inner_error = g_error_new (G_IO_ERROR, G_IO_ERROR_FAILED,
                                   "error getting serial port attributes: %s",
                                   g_strerror (errno));
        goto out;
    }
    cfmakeraw (&terminal_data);
    if (tcsetattr (self->priv->fd, TCSANOW, &terminal_data) < 0) {
        inner_error = g_error_new (G_IO_ERROR, G_IO_ERROR_FAILED,
                                   "error setting serial port attributes: %s",
                                   g_strerror (errno));
        goto out;
    }

    if (!qdl_device_dload_sdp (self, cancellable, &inner_error)) {
        if (!g_error_matches (inner_error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED))
            goto out;

        g_debug ("[qfu-qdl-device] error (ignored): DLOAD SDP not supported");
        g_clear_error (&inner_error);
    }

    if (!qdl_device_detect_version (self, cancellable, &inner_error))
        goto out;

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

QfuQdlDevice *
qfu_qdl_device_new (GFile         *file,
                    GCancellable  *cancellable,
                    GError       **error)
{
    g_return_val_if_fail (G_IS_FILE (file), NULL);

    return QFU_QDL_DEVICE (g_initable_new (QFU_TYPE_QDL_DEVICE,
                                           cancellable,
                                           error,
                                           "file", file,
                                           NULL));
}


static void
qfu_qdl_device_init (QfuQdlDevice *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, QFU_TYPE_QDL_DEVICE, QfuQdlDevicePrivate);
    self->priv->fd = -1;
    /* Long buffer for I/O */
    self->priv->buffer = g_byte_array_new ();
    g_byte_array_set_size (self->priv->buffer, QFU_QDL_MESSAGE_MAX_SIZE);
    /* Shorter secondary buffer for framing/unframing */
    self->priv->secondary_buffer = g_byte_array_new ();
    g_byte_array_set_size (self->priv->secondary_buffer, SECONDARY_BUFFER_DEFAULT_SIZE);
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    QfuQdlDevice *self = QFU_QDL_DEVICE (object);

    switch (prop_id) {
    case PROP_FILE:
        self->priv->file = g_value_dup_object (value);
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
    QfuQdlDevice *self = QFU_QDL_DEVICE (object);

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
    QfuQdlDevice *self = QFU_QDL_DEVICE (object);

    if (!(self->priv->fd < 0)) {
        close (self->priv->fd);
        self->priv->fd = -1;
    }
    g_clear_pointer (&self->priv->buffer,           g_byte_array_unref);
    g_clear_pointer (&self->priv->secondary_buffer, g_byte_array_unref);
    g_clear_object  (&self->priv->file);

    G_OBJECT_CLASS (qfu_qdl_device_parent_class)->dispose (object);
}

static void
initable_iface_init (GInitableIface *iface)
{
    iface->init = initable_init;
}

static void
qfu_qdl_device_class_init (QfuQdlDeviceClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (object_class, sizeof (QfuQdlDevicePrivate));

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
