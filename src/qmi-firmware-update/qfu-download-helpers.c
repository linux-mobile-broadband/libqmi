/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * qmi-firmware-update -- Command line tool to update firmware in QMI devices
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
 * Copyright (C) 2016 Aleksander Morgado <aleksander@aleksander.es>
 *
 * Additional copyrights:
 *
 *    crc16 and hdlc parts:
 *      Copyright (C) 2010 Red Hat, Inc.
 *
 *    parts of this is based on gobi-loader, which is
 *     "Copyright 2009 Red Hat <mjg@redhat.com> - heavily based on work done by
 *      Alexander Shumakovitch <shurik@gwu.edu>
 *
 *    gobi 2000 support provided by Anssi Hannula <anssi.hannula@iki.fi>
 */

#include <config.h>

#include <stdio.h>
#include <endian.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <linux/types.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

#include <libqmi-glib.h>

#include "qfu-download-helpers.h"
#include "qfu-enum-types.h"
#include "qfu-qdl-message.h"
#include "qfu-dload-message.h"
#include "qfu-utils.h"

static gboolean write_hdlc     (gint          fd,
                                const gchar  *in,
                                gsize         inlen,
                                GError      **error);
static gboolean read_hdlc      (gint          fd,
                                GError      **error);
static gboolean read_hdlc_full (gint          fd,
                                guint16      *seq_ack,
                                GError      **error);

/******************************************************************************/

static gchar *
utils_str_hex (gconstpointer mem,
               gsize         size,
               gchar         delimiter)
{
    const guint8 *data = mem;
    gsize         i;
    gsize         j;
    gsize         new_str_length;
    gchar        *new_str;

    /* Get new string length. If input string has N bytes, we need:
     * - 1 byte for last NUL char
     * - 2N bytes for hexadecimal char representation of each byte...
     * - N-1 bytes for the separator ':'
     * So... a total of (1+2N+N-1) = 3N bytes are needed... */
    new_str_length =  3 * size;

    /* Allocate memory for new array and initialize contents to NUL */
    new_str = g_malloc0 (new_str_length);

    /* Print hexadecimal representation of each byte... */
    for (i = 0, j = 0; i < size; i++, j += 3) {
        /* Print character in output string... */
        snprintf (&new_str[j], 3, "%02X", data[i]);
        /* And if needed, add separator */
        if (i != (size - 1) )
            new_str[j + 2] = delimiter;
    }

    /* Set output string */
    return new_str;
}

/******************************************************************************/
/* DLOAD protocol */

static gboolean
dload_switch_to_sdp (gint     fd,
                     gchar   *buf,
                     gsize    buflen,
                     GError **error)
{
    gsize   reqlen;
    GError *inner_error = NULL;

    reqlen = qfu_dload_request_sdp_build ((guint8 *)buf, buflen);

    /* switch to SDP - this is required for some modems like MC7710 */
    if (!write_hdlc (fd, (const gchar *) buf, reqlen, error)) {
        g_prefix_error (error, "couldn't write request: ");
        return FALSE;
    }

	/* The modem could already be in SDP mode, so ignore "unsupported" errors */
    if (!read_hdlc (fd, &inner_error)) {
        if (!g_error_matches (inner_error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED)) {
            g_propagate_error (error, inner_error);
            return FALSE;
        }
        g_error_free (inner_error);
    }

    return TRUE;
}

/******************************************************************************/
/* HDLC */

#define CONTROL 0x7e
#define ESCAPE  0x7d
#define MASK    0x20

static gsize
escape (const gchar *in, gsize inlen, gchar *out, gsize outlen)
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
unescape (const gchar *in,
          gsize        inlen,
          gchar       *out,
          gsize        outlen)
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
             gchar  *out,
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
hdlc_frame (const gchar *in,
            gsize        inlen,
            gchar       *out,
            gsize        outlen)
{
    guint16 crc;
    gsize j = 0;

    out[j++] = CONTROL;
    j += escape (in, inlen, &out[j], outlen - j);
    crc = qfu_utils_crc16 ((const guint8 *)in, inlen);
    j += escape_byte (crc & 0xff, &out[j], outlen - j);
    j += escape_byte (crc >> 8 & 0xff, &out[j], outlen - j);
    out[j++] = CONTROL;

    return j;
}

static gsize
hdlc_unframe (const gchar  *in,
              gsize         inlen,
              gchar        *out,
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
    crc = qfu_utils_crc16 ((const guint8 *)in, j);
    if (crc != ((guint8) out[j] | (guint8) out[j + 1] << 8)) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "crc check failed: 0x%04x != 0x%04x\n", crc, out[j] | out[j + 1] << 8);
        return 0;
    }

    return j;
}

/******************************************************************************/
/* QDL */

static gboolean
process_sdp_hello (const gchar  *in,
                   gsize         inlen,
                   GError      **error)
{
	gchar buf[512];
	gsize ret;

	ret = hdlc_unframe (in, inlen, buf, sizeof (buf), error);
    if (ret == 0) {
        g_prefix_error (error, "error unframing message: ");
        return FALSE;
    }

    return qfu_qdl_response_hello_parse ((const guint8 *)buf, ret, error);
}

static gboolean
process_sdp_err (const gchar  *in,
                 gsize         inlen,
                 GError      **error)
{
	gchar buf[512];
	gsize ret;

	ret = hdlc_unframe (in, inlen, buf, sizeof (buf), error);
    if (ret == 0) {
        g_prefix_error (error, "error unframing message: ");
        return FALSE;
    }

    return qfu_qdl_response_error_parse ((const guint8 *)buf, ret, error);
}

static gboolean
process_ufopen (const gchar  *in,
                gsize         inlen,
                GError      **error)
{
	gchar buf[512];
	gsize ret;

	ret = hdlc_unframe (in, inlen, buf, sizeof (buf), error);
    if (ret == 0) {
        g_prefix_error (error, "error unframing message: ");
        return FALSE;
    }

    return qfu_qdl_response_ufopen_parse ((const guint8 *)buf, ret, error);
}

static gboolean
process_ufwrite (const gchar  *in,
                 gsize         inlen,
                 guint16      *sequence,
                 GError      **error)
{
	gchar buf[512];
	gsize ret;

	ret = hdlc_unframe (in, inlen, buf, sizeof (buf), error);
    if (ret == 0) {
        g_prefix_error (error, "error unframing message: ");
        return FALSE;
    }

    return qfu_qdl_response_ufwrite_parse ((const guint8 *)buf, ret, sequence, error);
}

static gboolean
process_ufclose (const gchar  *in,
                 gsize         inlen,
                 GError      **error)
{
	gchar          buf[512];
	gsize          ret;

	ret = hdlc_unframe (in, inlen, buf, sizeof (buf), error);
    if (ret == 0) {
        g_prefix_error (error, "error unframing message: ");
        return FALSE;
    }

    return qfu_qdl_response_ufclose_parse ((const guint8 *)buf, ret, error);
}

static gboolean
qdl_detect_version (gint     fd,
                    gchar   *buf,
                    gsize    buflen,
                    GError **error)
{
    guint    version;
    gboolean found = FALSE;

	/* Attempt to probe supported protocol version
	 *  Newer modems like Sierra Wireless MC7710 must use '6' for both fields
	 *  Gobi2000 modems like HP un2420 must use '5' for both fields
	 *  Gobi1000 modems  must use '4' for both fields
	 */
	for (version = 4; !found && version < 7; version++) {
        gsize reqlen;

        reqlen = qfu_qdl_request_hello_build ((guint8 *)buf, buflen, version, version);
		if (!write_hdlc (fd, buf, reqlen, error)) {
            g_prefix_error (error, "couldn't write request: ");
            return FALSE;
        }

        /* If no error processing the response, we assume version is found */
        found = read_hdlc (fd, NULL);
    }

    if (!found) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "couldn't detect QDL version");
        return FALSE;
    }

    g_debug ("[qfu-download,qdl] QDL version detected: %u", version);
    return TRUE;
}

static gboolean
qdl_session_done (gint     fd,
                  gchar   *buf,
                  gsize    buflen,
                  GError **error)
{
    gsize reqlen;

    reqlen = qfu_qdl_request_ufclose_build ((guint8 *)buf, buflen);

    if (!write_hdlc (fd, (const gchar *) buf, reqlen, error)) {
        g_prefix_error (error, "couldn't write session done request: ");
        return FALSE;
    }

    if (!read_hdlc (fd, error)) {
        g_prefix_error (error, "error reported in session done request: ");
        return FALSE;
    }

    return TRUE;
}

static gboolean
qdl_session_close (gint     fd,
                   gchar   *buf,
                   gsize    buflen,
                   GError **error)
{
    gsize reqlen;

    reqlen = qfu_qdl_request_reset_build ((guint8 *)buf, buflen);

    if (!write_hdlc (fd, (const gchar *) buf, reqlen, error)) {
        g_prefix_error (error, "couldn't write session close request: ");
        return FALSE;
    }

    if (!read_hdlc (fd, error)) {
        g_prefix_error (error, "error reported in session close request: ");
        return FALSE;
    }

    return TRUE;
}

static gboolean
qdl_download_image (gint           fd,
                    gchar         *buf,
                    gsize          buflen,
                    QfuImage      *image,
                    GCancellable  *cancellable,
                    GError       **error)
{
    GError  *inner_error = NULL;
    gssize   reqlen;
    gssize   aux;
    guint16  seq = 0;
    guint16  acked_sequence = 0;
    guint16  n_chunks;

    g_debug ("[qfu-download,qdl] downloading %s: %s (header %" G_GOFFSET_FORMAT " bytes, data %" G_GOFFSET_FORMAT " bytes)",
             qfu_image_type_get_string (qfu_image_get_image_type (image)),
             qfu_image_get_display_name (image),
             qfu_image_get_header_size (image),
             qfu_image_get_data_size (image));

	/* prepare open request */
    reqlen = qfu_qdl_request_ufopen_build ((guint8 *)buf, buflen, image, cancellable, &inner_error);
    if (reqlen < 0) {
        g_prefix_error (&inner_error, "couldn't create ufopen request: ");
        goto out;
    }

	if (!write_hdlc (fd, buf, reqlen, &inner_error)) {
        g_prefix_error (&inner_error, "couldn't write open request");
        goto out;
    }

	/* read ufopen response */
	if (!read_hdlc (fd, &inner_error)) {
        g_prefix_error (&inner_error, "error detected in open request: ");
        goto out;
    }

	/* remaining data to send */
    n_chunks = qfu_image_get_n_data_chunks (image);
    for (seq = 0; seq < n_chunks; seq++) {
		fd_set         wr;
		struct timeval tv = { .tv_sec = 2, .tv_usec = 0, };

        /* Check cancellable on every loop */
        if (g_cancellable_is_cancelled (cancellable)) {
            inner_error = g_error_new (G_IO_ERROR, G_IO_ERROR_CANCELLED,
                                       "download operation cancelled");
            goto out;
        }

		g_debug ("[qfu-download,qdl] writing chunk #%" G_GUINT16_FORMAT "...", seq);

        /* prepare write request */
        reqlen = qfu_qdl_request_ufwrite_build ((guint8 *)buf, buflen, image, seq, cancellable, &inner_error);
        if (reqlen < 0) {
            g_prefix_error (&inner_error, "couldn't create ufwrite request: ");
            goto out;
        }

		FD_ZERO (&wr);
		FD_SET (fd, &wr);
        aux = select (fd + 1, NULL, &wr, NULL, &tv);
        if (aux < 0) {
            inner_error = g_error_new (G_IO_ERROR, G_IO_ERROR_FAILED,
                                       "error waiting to write #%u: %s",
                                       seq, g_strerror (errno));
            goto out;
        }
        if (aux == 0) {
            inner_error = g_error_new (G_IO_ERROR, G_IO_ERROR_FAILED,
                                       "timed out waiting to write chunk #%u",
                                       seq);
            goto out;
        }

        /* Note: no HDLC here */
		if (write (fd, buf, reqlen) < 0) {
            inner_error = g_error_new (G_IO_ERROR, G_IO_ERROR_FAILED,
                                       "error writing chunk #%u: %s",
                                       seq, g_strerror (errno));
            goto out;
        }

        if (read_hdlc_full (fd, &acked_sequence, NULL))
            g_debug ("[qfu-download,qdl] ack-ed chunk #%u", (guint) acked_sequence);
	}

    g_debug ("[qfu-download,qdl] finished writing: waiting for final chunk #%u ack", (seq - 1));

	/* This may take a considerable amount of time */
	while (acked_sequence != (seq - 1)) {
        /* Wait some time */
        g_usleep (3 * G_USEC_PER_SEC);

        /* Check cancellable on every loop */
        if (g_cancellable_is_cancelled (cancellable)) {
            inner_error = g_error_new (G_IO_ERROR, G_IO_ERROR_CANCELLED,
                                       "download operation cancelled");
            goto out;
        }

        if (read_hdlc_full (fd, &acked_sequence, NULL))
            g_debug ("[qfu-download,qdl] ack-ed chunk #%u", (guint) acked_sequence);
	}

    g_debug ("[qfu-download,qdl] all chunks ack-ed");

out:

    if (inner_error) {
        g_propagate_error (error, inner_error);
        return FALSE;
    }

	return TRUE;
}

/******************************************************************************/
/* R/W HDLC */

static gboolean
write_hdlc (gint          fd,
            const gchar  *in,
            gsize         inlen,
            GError      **error)
{
    gchar  wbuf[512];
    gsize  wlen;
    gssize written;

    /* Pack into an HDLC frame */
    wlen = hdlc_frame (in, inlen, wbuf, sizeof (wbuf));
    g_assert_cmpuint (wlen, >, 0);

    /* Send HDLC frame to device */
    written = write (fd, wbuf, wlen);
    if (written < 0) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "couldn't write HDLC frame: %s",
                     g_strerror (errno));
        return FALSE;
    }

    /* Treat this as an error, we may want to improve this by writing out the
     * remaning amount of bytes */
    if (written != wlen) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "error writing full HDLC frame: only %" G_GSSIZE_FORMAT "/%" G_GSIZE_FORMAT " bytes written",
                     written, wlen);
        return FALSE;
    }

    /* Debug output */
    if (qmi_utils_get_traces_enabled ()) {
        gchar *printable;

        printable = utils_str_hex (wbuf, wlen, ':');
        g_debug ("[qfu-download,hdlc] >> %s", printable);
        g_free (printable);
    }

    return TRUE;
}

static gboolean
read_hdlc_full (gint      fd,
                guint16  *seq_ack,
                GError  **error)
{
	gchar          *end;
    gchar          *p;
    gchar           rbuf[512];
	gsize           rlen;
    gsize           framelen;
	fd_set          rd;
	struct timeval  tv = { .tv_sec = 1, .tv_usec = 0, };

	FD_ZERO (&rd);
	FD_SET (fd, &rd);
	if (select (fd + 1, &rd, NULL, NULL, &tv) <= 0) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_TIMED_OUT,
                     "read operation timed out");
        return FALSE;
	}

	rlen = read (fd, rbuf, sizeof (rbuf));
	if (rlen < 0) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "couldn't read HDLC frame: %s",
                     g_strerror (errno));
        return FALSE;
    }

    if (rlen == 0) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "couldn't read HDLC frame: HUP detected");
        return FALSE;
    }

    /* Debug output */
    if (qmi_utils_get_traces_enabled ()) {
        gchar *printable;

        printable = utils_str_hex (rbuf, rlen, ':');
        g_debug ("[qfu-download,hdlc] << %s", printable);
        g_free (printable);
    }

	p = rbuf;
	while (rlen > 0 && (end = memchr (p + 1, CONTROL, rlen - 1)) != NULL) {
		end++;
		framelen = end - p;
		switch (p[1]) {
		case QFU_QDL_CMD_ERROR:
            if (!process_sdp_err (p, framelen, error))
                return FALSE;
            break;
		case QFU_QDL_CMD_HELLO_RSP: /* == DLOAD_CMD_ACK */
			if (framelen != 5) {
                if (!process_sdp_hello (p, framelen, error))
                    return FALSE;
            } else
				g_debug("[qfu-download,dload] ack");
			break;
		case QFU_QDL_CMD_OPEN_UNFRAMED_RSP:
            if (!process_ufopen (p, framelen, error))
                return FALSE;
			break;
		case QFU_QDL_CMD_WRITE_UNFRAMED_RSP:
            if (!process_ufwrite (p, framelen, seq_ack, error))
                return FALSE;
			break;
		case QFU_QDL_CMD_CLOSE_UNFRAMED_RSP:
            if (!process_ufclose (p, framelen, error))
                return FALSE;
			break;
		default:
            g_warning ("qfu-download,hdlc] unsupported message received: 0x%02x\n", p[1]);
            break;
		}

		p = end;
		rlen -= framelen;
	}

	return TRUE;
}

static gboolean
read_hdlc (gint      fd,
           GError  **error)
{
    return read_hdlc_full (fd, NULL, error);
}

/******************************************************************************/
/* Serial port */

static gint
serial_open (GFile   *tty,
             GError **error)
{
    struct termios  terminal_data;
    gint            fd;
    gchar          *path;

    path = g_file_get_path (tty);
    g_debug ("[qfu-download] opening TTY: %s", path);

    fd = open (path, O_RDWR | O_NOCTTY);
    if (fd < 0) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "error opening serial device: %s",
                     g_strerror (errno));
    } else {
        g_debug ("[qfu-download] setting terminal in raw mode...");
        tcgetattr (fd, &terminal_data);
        cfmakeraw (&terminal_data);
        tcsetattr (fd, TCSANOW, &terminal_data);
    }

    g_free (path);

    return fd;
}

/******************************************************************************/

typedef struct {
    GFile    *tty;
    QfuImage *image;
    gint      fd;
    gchar    *buffer;
} RunContext;

static void
run_context_free (RunContext *ctx)
{
    if (!(ctx->fd < 0))
        close (ctx->fd);
    g_free (ctx->buffer);
    g_object_unref (ctx->image);
    g_object_unref (ctx->tty);
    g_slice_free (RunContext, ctx);
}

gboolean
qfu_download_helper_run_finish (GAsyncResult  *res,
                                GError       **error)
{
    return g_task_propagate_boolean (G_TASK (res), error);
}

static void
download_helper_run (GTask *task)
{
    RunContext *ctx;
    GError     *error = NULL;

    ctx = (RunContext *) g_task_get_task_data (task);

    g_debug ("[qfu-download] opening tty...");
    ctx->fd = serial_open (ctx->tty, &error);
    if (ctx->fd < 0) {
        g_prefix_error (&error, "couldn't open serial device:");
        goto out;
    }

    if (g_task_return_error_if_cancelled (task))
        return;

    g_debug ("[qfu-download] switching to SDP...");
    if (!dload_switch_to_sdp (ctx->fd,
                              ctx->buffer,
                              QFU_QDL_MESSAGE_MAX_SIZE,
                              &error)) {
        g_prefix_error (&error, "couldn't switch to SDP: ");
        goto out;
    }

    if (g_task_return_error_if_cancelled (task))
        return;

    g_debug ("[qfu-download] detecting QDL version...");
    if (!qdl_detect_version (ctx->fd,
                             ctx->buffer,
                             QFU_QDL_MESSAGE_MAX_SIZE,
                             &error)) {
        g_prefix_error (&error, "couldn't detect QDL version: ");
        goto out;
    }

    if (g_task_return_error_if_cancelled (task))
        return;

    if (!qdl_download_image (ctx->fd,
                             ctx->buffer,
                             QFU_QDL_MESSAGE_MAX_SIZE,
                             ctx->image,
                             g_task_get_cancellable (task),
                             &error)) {
        g_prefix_error (&error, "couldn't download image: ");
        goto out;
    }

    if (!qdl_session_done (ctx->fd,
                           ctx->buffer,
                           QFU_QDL_MESSAGE_MAX_SIZE,
                           &error)) {
        /* Error not fatal */
        g_debug ("[qfu-download] error reporting session done: %s", error->message);
        g_clear_error (&error);
    }

    /* Ignore error completely, we likely get a HUP when reading */
    qdl_session_close (ctx->fd,
                       ctx->buffer,
                       QFU_QDL_MESSAGE_MAX_SIZE,
                       NULL);

out:
    if (error)
        g_task_return_error (task, error);
    else
        g_task_return_boolean (task, TRUE);
    g_object_unref (task);
}

void
qfu_download_helper_run (GFile               *tty,
                         QfuImage            *image,
                         GCancellable        *cancellable,
                         GAsyncReadyCallback  callback,
                         gpointer             user_data)
{
    GTask      *task;
    RunContext *ctx;

    g_assert (G_IS_FILE (tty));
    g_assert (QFU_IS_IMAGE (image));

    ctx = g_slice_new0 (RunContext);
    ctx->fd = -1;
    ctx->tty = g_object_ref (tty);
    ctx->image = g_object_ref (image);
    ctx->buffer = g_malloc (QFU_QDL_MESSAGE_MAX_SIZE);

    task = g_task_new (NULL, cancellable, callback, user_data);
    g_task_set_task_data (task, ctx, (GDestroyNotify) run_context_free);

    /* Run */
    download_helper_run (task);
}
