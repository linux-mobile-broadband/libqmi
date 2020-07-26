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
 * Copyright (C) 2019 Zodiac Inflight Innovations
 * Copyright (C) 2019 Aleksander Morgado <aleksander@aleksander.es>
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
#include "qfu-firehose-message.h"
#include "qfu-sahara-message.h"
#include "qfu-sahara-device.h"
#include "qfu-utils.h"
#include "qfu-enum-types.h"

static void initable_iface_init (GInitableIface *iface);

G_DEFINE_TYPE_EXTENDED (QfuSaharaDevice, qfu_sahara_device, G_TYPE_OBJECT, 0,
                        G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE, initable_iface_init))

enum {
    PROP_0,
    PROP_FILE,
    PROP_LAST
};

static GParamSpec *properties[PROP_LAST];

#define MAX_PRINTABLE_SIZE 80

struct _QfuSaharaDevicePrivate {
    GFile      *file;
    gint        fd;
    GByteArray *buffer;

    /* target and transfer settings */
    guint max_payload_size_to_target_in_bytes;
    guint sector_size_in_bytes;
    guint num_partition_sectors;
    guint total_sector_size_in_bytes;
    guint pages_in_block;
    /* computed from settings */
    guint transfer_block_size;
    /* number of images setup */
    guint n_setup_images;
};

/******************************************************************************/
/* Split response into multiple XML messages */

#define XML_START_TAG "<?xml"

static gchar **
split_xml_document (const gchar *rsp)
{
    GPtrArray   *xml_docs = NULL;
    const gchar *aux;
    const gchar *next;

    xml_docs = g_ptr_array_new ();
    aux = strstr (rsp, XML_START_TAG);

    while (aux) {
        gchar *xmldoc;

        next = strstr (aux + 1, XML_START_TAG);
        if (next)
            xmldoc = g_strndup (aux, next - aux);
        else
            xmldoc = g_strdup (aux);
        g_strdelimit (xmldoc, "\r\n", ' ');
        g_ptr_array_add (xml_docs, xmldoc);
        aux = next;
    }

    if (xml_docs->len == 0) {
        g_ptr_array_free (xml_docs, TRUE);
        return NULL;
    }
    g_ptr_array_add (xml_docs, NULL);
    return (gchar **) g_ptr_array_free (xml_docs, FALSE);
}

/******************************************************************************/
/* Send */

static gboolean
send_request (QfuSaharaDevice  *self,
              const guint8     *request,
              gsize             request_size,
              GCancellable     *cancellable,
              GError          **error)
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
        g_debug ("[qfu-sahara-device] >> %s%s [%" G_GSIZE_FORMAT "]", printable, shorted ? "..." : "", request_size);
        g_free (printable);

        if (strncmp ((const gchar *)request, XML_START_TAG, strlen (XML_START_TAG)) == 0) {
            printable = g_strdup ((const gchar *)request);
            g_strdelimit (printable, "\r\n", ' ');
            g_debug ("[qfu-sahara-device] >> %s", printable);
            g_free (printable);
        }
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

/******************************************************************************/
/* Receive */

static gssize
receive_response (QfuSaharaDevice  *self,
                  guint             timeout_secs,
                  guint8          **response,
                  GCancellable     *cancellable,
                  GError          **error)
{
    fd_set         rd;
    struct timeval tv;
    gint           aux;
    gssize         rlen;

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

    /* we may not always get a response, so just return 0 bytes read if we timeout */
    if (aux == 0)
        return 0;

    /* Receive in the primary buffer
     * Always leave room for setting next byte as NUL. */
    memset (self->priv->buffer->data, 0, self->priv->buffer->len);
    rlen = read (self->priv->fd, self->priv->buffer->data, self->priv->buffer->len - 1);
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

    /* make sure that we can treat the response as a NUL-terminated string */
    g_assert ((guint)rlen <= self->priv->buffer->len - 1);
    self->priv->buffer->data[rlen] = '\0';

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
        g_debug ("[qfu-sahara-device] << %s%s [%" G_GSIZE_FORMAT "]", printable, shorted ? "..." : "", rlen);
        g_free (printable);

        if (strncmp ((const gchar *)self->priv->buffer->data, XML_START_TAG, strlen (XML_START_TAG)) == 0) {
            printable = g_strdup ((const gchar *)self->priv->buffer->data);
            g_strdelimit (printable, "\r\n", ' ');
            g_debug ("[qfu-sahara-device] << %s", printable);
            g_free (printable);
        }
    }

    if (response)
        *response = self->priv->buffer->data;

    return rlen;
}

/******************************************************************************/
/* Send/receive */

static gssize
send_receive (QfuSaharaDevice  *self,
              const guint8     *request,
              gsize             request_size,
              guint             response_timeout_secs,
              guint8          **response,
              GCancellable     *cancellable,
              GError          **error)
{
    gboolean sent;

    if (self->priv->fd < 0) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "device is closed");
        return -1;
    }

    if (request_size > 0) {
        sent = send_request (self, request, request_size, cancellable, error);
        if (!sent)
            return -1;
    }

    if (!response)
        return 0;

    return receive_response (self, response_timeout_secs, response, cancellable, error);
}

/******************************************************************************/
/* Common firehose state machine */

static gboolean
firehose_common_process_response_ack_message (const gchar  *rsp,
                                              const gchar  *expected_value,
                                              const gchar  *expected_rawmode,
                                              GError      **error)
{
    gchar *value = NULL;
    gchar *rawmode = NULL;

    g_assert (expected_value);

    if (!qfu_firehose_message_parse_response_ack (rsp, &value, &rawmode))
        return FALSE;

    if ((g_strcmp0 (value, expected_value) == 0) && (!expected_rawmode || (rawmode && g_strcmp0 (rawmode, expected_rawmode) == 0)))
        g_debug ("[qfu-sahara-device] firehose response received: value=%s, rawmode=%s",
                 value, rawmode ? rawmode : "n/a");
    else
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "unexpected firehose response received: value=%s, rawmode=%s",
                     value, rawmode ? rawmode : "n/a");

    g_free (value);
    g_free (rawmode);
    return TRUE;
}

static gboolean
firehose_common_process_log_message (const gchar *rsp)
{
    gchar *value = NULL;

    if (!qfu_firehose_message_parse_log (rsp, &value))
        return FALSE;

    g_debug ("[qfu-sahara-device] firehose log: %s", value);
    g_free (value);
    return TRUE;
}

typedef const gchar * (* PrepareRequestCallback)  (QfuSaharaDevice  *self,
                                                   gpointer          user_data);
typedef gboolean      (* ProcessResponseCallback) (QfuSaharaDevice  *self,
                                                   const gchar      *rsp,
                                                   gpointer          user_data,
                                                   GError          **error);
typedef gboolean      (* CheckCompletionCallback) (QfuSaharaDevice  *self,
                                                   gpointer          user_data);
typedef void          (* InitRetryCallback)       (QfuSaharaDevice  *self,
                                                   gpointer          user_data);

static gboolean
firehose_operation_run (QfuSaharaDevice          *self,
                        PrepareRequestCallback    prepare_request,
                        ProcessResponseCallback   process_response,
                        CheckCompletionCallback   check_completion,
                        InitRetryCallback         init_retry,
                        guint                     max_retries,
                        guint                     timeout_secs,
                        gpointer                  user_data,
                        GCancellable             *cancellable,
                        GError                  **error)
{
    GTimer *timer;
    GError *inner_error = NULL;
    guint   n_retries = 0;

    g_assert ((max_retries && init_retry) || (!max_retries && !init_retry));

    g_debug ("[qfu-sahara-device] running firehose operation...");

    timer = g_timer_new ();
    while (TRUE) {
        const gchar  *req = NULL;
        gssize        rsplen;
        guint8       *rsp = NULL;
        gchar       **xmldocs;
        guint         i;

        /* check timeout */
        if (g_timer_elapsed (timer, NULL) > timeout_secs) {
            /* retry? */
            if (max_retries && ++n_retries < max_retries) {
                g_timer_reset (timer);
                g_clear_error (&inner_error);
                init_retry (self, user_data);
                continue;
            }
            inner_error = g_error_new (G_IO_ERROR, G_IO_ERROR_TIMED_OUT, "operation timed out");
            break;
        }
        /* check cancellation */
        if (g_cancellable_is_cancelled (cancellable)) {
            inner_error = g_error_new (G_IO_ERROR, G_IO_ERROR_CANCELLED, "operation cancelled");
            break;
        }

        /* user-provided callback to prepare request, may return NULL if there's nothing to send */
        if (prepare_request)
            req = prepare_request (self, user_data);

        rsplen = send_receive (self,
                               (const guint8 *)req,
                               req ? strlen (req) : 0,
                               2,
                               &rsp,
                               cancellable,
                               &inner_error);
        if (rsplen < 0)
            break;

        /* timed out without response received */
        if (!rsplen)
            continue;

        /* we may receive multiple XML documents on a single read() */
        xmldocs = split_xml_document ((const gchar *)rsp);
        for (i = 0; xmldocs && xmldocs[i] && !inner_error; i++) {
            /* user-provided callback to process response, may return FALSE and
             * an error if the operation is detected as failed */
            process_response (self, xmldocs[i], user_data, &inner_error);
        }
        g_strfreev (xmldocs);
        if (inner_error) {
            /* retry? */
            if (max_retries && ++n_retries < max_retries) {
                g_timer_reset (timer);
                g_clear_error (&inner_error);
                init_retry (self, user_data);
                continue;
            }
            break;
        }

        /* keep on operation? */
        if (check_completion (self, user_data))
            break;
    }

    g_timer_destroy (timer);

    if (inner_error) {
        g_debug ("[qfu-sahara-device] firehose operation failed: %s", inner_error->message);
        g_propagate_error (error, inner_error);
        return FALSE;
    }

    g_debug ("[qfu-sahara-device] firehose operation finished successfully");
    return TRUE;
}

/******************************************************************************/

static gboolean
validate_ascii_print (const guint8 *rsp,
                      gsize         rsplen)
{
    guint i;

    for (i = 0; i < rsplen; i++) {
        if (!g_ascii_isprint (rsp[i]))
            return FALSE;
    }

    return TRUE;
}

/******************************************************************************/
/* Firehose setup download */

#define FIREHOSE_SETUP_DOWNLOAD_TIMEOUT_SECS 10
#define FIREHOSE_SETUP_DOWNLOAD_MAX_RETRIES   3

typedef struct {
    guint    n_partition_sectors;
    gboolean sent;
    gboolean acked;
} FirehoseSetupDownloadContext;

static const gchar *
firehose_setup_download_prepare_request (QfuSaharaDevice              *self,
                                         FirehoseSetupDownloadContext *ctx)
{
    if (!ctx->sent) {
        ctx->sent = TRUE;
        g_debug ("[qfu-sahara-device] sending firehose program request...");
        qfu_firehose_message_build_program (self->priv->buffer->data,
                                            self->priv->buffer->len,
                                            self->priv->pages_in_block,
                                            self->priv->sector_size_in_bytes,
                                            ctx->n_partition_sectors);
        return (const gchar *)self->priv->buffer->data;
    }

    return NULL;
}

static gboolean
firehose_setup_download_process_response (QfuSaharaDevice               *self,
                                          const gchar                   *rsp,
                                          FirehoseSetupDownloadContext  *ctx,
                                          GError                       **error)
{
    GError *inner_error = NULL;

    if (firehose_common_process_log_message (rsp))
        return TRUE;

    if (firehose_common_process_response_ack_message (rsp, "ACK", "true", &inner_error)) {
        if (inner_error) {
            g_propagate_error (error, inner_error);
            return FALSE;
        }

        ctx->acked = TRUE;
        return TRUE;
    }

    g_debug ("[qfu-sahara-device] unknown firehose message received");
    return TRUE;
}

static gboolean
firehose_setup_download_check_completion (QfuSaharaDevice              *self,
                                          FirehoseSetupDownloadContext *ctx)
{
    return (ctx->acked);
}

static void
firehose_setup_download_init_retry (QfuSaharaDevice              *self,
                                    FirehoseSetupDownloadContext *ctx)
{
    /* no need to cleanup n_partition_sectors */

    ctx->sent  = FALSE;
    ctx->acked = FALSE;
}

gboolean
qfu_sahara_device_firehose_setup_download (QfuSaharaDevice  *self,
                                           QfuImage         *image,
                                           guint            *n_blocks,
                                           GCancellable     *cancellable,
                                           GError          **error)
{
    FirehoseSetupDownloadContext ctx = {
        .n_partition_sectors = 0,
        .sent                = FALSE,
        .acked               = FALSE,
    };
    goffset image_size;
    guint   n_transfer_blocks;

    /* NOTE: the firmware download process in Windows sends an additional
     * configure message before the program request when the 2nd firmware
     * image is downloaded... but it really doesn't seem to be required
     * for anything, so we're explicitly avoiding that. Sending the
     * program request seems to be enough. */

    /* compute how many sectors are required */
    image_size = qfu_image_get_size (image);
    ctx.n_partition_sectors = image_size / self->priv->sector_size_in_bytes;
    if (image_size % self->priv->sector_size_in_bytes > 0)
        ctx.n_partition_sectors++;

    /* compute how many transfer block are required, and set them as output right away */
    n_transfer_blocks = image_size / self->priv->transfer_block_size;
    if (image_size % self->priv->transfer_block_size > 0)
        n_transfer_blocks++;
    if (n_blocks)
        *n_blocks = n_transfer_blocks;

    g_debug ("Setting up firehose download for %" G_GOFFSET_FORMAT " bytes image...", image_size);
    g_debug ("  pages in block:        %u", self->priv->pages_in_block);
    g_debug ("  sector size:           %u", self->priv->sector_size_in_bytes);
    g_debug ("  num partition sectors: %u", ctx.n_partition_sectors);
    g_debug ("  transfer block size:   %u (%u sectors/transfer)", self->priv->transfer_block_size, self->priv->transfer_block_size / self->priv->sector_size_in_bytes);
    g_debug ("  num transfers:         %u", n_transfer_blocks);

    return firehose_operation_run (self,
                                   (PrepareRequestCallback)  firehose_setup_download_prepare_request,
                                   (ProcessResponseCallback) firehose_setup_download_process_response,
                                   (CheckCompletionCallback) firehose_setup_download_check_completion,
                                   (InitRetryCallback)       firehose_setup_download_init_retry,
                                   FIREHOSE_SETUP_DOWNLOAD_MAX_RETRIES,
                                   FIREHOSE_SETUP_DOWNLOAD_TIMEOUT_SECS,
                                   &ctx,
                                   cancellable,
                                   error);
}

/******************************************************************************/
/* Firehose write block in raw mode */

#define END_OF_TRANSFER_BLOCK_SIZE 512

gboolean
qfu_sahara_device_firehose_write_block (QfuSaharaDevice  *self,
                                        QfuImage         *image,
                                        guint             block_i,
                                        GCancellable     *cancellable,
                                        GError          **error)
{
    gssize   reqlen;
    goffset  offset;
    gsize    size;
    gboolean send_last = FALSE;

    g_debug ("[qfu-sahara-device] writing block %u...", block_i);

    g_assert (self->priv->transfer_block_size < self->priv->buffer->len);
    memset (self->priv->buffer->data, 0, self->priv->transfer_block_size);

    offset = block_i * (goffset)self->priv->transfer_block_size;
    size = qfu_image_get_size (image) - offset;
    if (size >= self->priv->transfer_block_size)
        size = self->priv->transfer_block_size;
    else {
        gsize last_block_size;

        /* we need to send an additional packet full of 0s after the last
         * sector is transferred. */
        send_last = TRUE;

        /* last transfer block adjusted to sector size multiple */
        last_block_size = self->priv->sector_size_in_bytes;
        while (last_block_size < size)
            last_block_size += self->priv->sector_size_in_bytes;
        size = last_block_size;
        g_assert (size <= self->priv->transfer_block_size);
    }

    reqlen = qfu_image_read (image,
                             offset,
                             size,
                             self->priv->buffer->data,
                             self->priv->buffer->len,
                             cancellable,
                             error);
    if (reqlen < 0) {
        g_prefix_error (error, "couldn't read transfer block %u", block_i);
        return FALSE;
    }

    g_assert ((guint)reqlen <= self->priv->transfer_block_size);
    if (send_receive (self,
                      self->priv->buffer->data,
                      size,
                      0,
                      NULL,
                      cancellable,
                      error) < 0) {
        g_prefix_error (error, "couldn't send transfer block %u", block_i);
        return FALSE;
    }

    if (send_last) {
        /* We're sending a last block to notify the end of the transmission,
         * which seems to be a reliable way to tell the modem that it shouldn't
         * expect more data.
         *
         * This block is full of 0s, but the modem seems to end up storing
         * it and leaving it to be processed once the image has been processed,
         * which will trigger a warning during the next firehose operation:
         *    ERROR: XML not formed correctly. Expected a &lt; character at loc 0
         * And it will also fail the operation with a NAK...
         *
         * But, just retrying the operation (the program request for the next
         * file to download, or the reset if no more files to download) is enough
         * to make it work.
         */
        memset (self->priv->buffer->data, 0, END_OF_TRANSFER_BLOCK_SIZE);
        if (send_receive (self,
                          self->priv->buffer->data,
                          END_OF_TRANSFER_BLOCK_SIZE,
                          0,
                          NULL,
                          cancellable,
                          error) < 0) {
            g_prefix_error (error, "couldn't send last end-of-transfer block");
            return FALSE;
        }
    }

    return TRUE;
}

/******************************************************************************/
/* Firehose teardown download */

#define FIREHOSE_TEARDOWN_DOWNLOAD_TIMEOUT_SECS 300

typedef struct {
    gboolean acked;
} FirehoseTeardownDownloadContext;

static gboolean
firehose_teardown_download_process_response (QfuSaharaDevice                  *self,
                                             const gchar                      *rsp,
                                             FirehoseTeardownDownloadContext  *ctx,
                                             GError                          **error)
{
    GError *inner_error = NULL;

    if (firehose_common_process_log_message (rsp))
        return TRUE;

    if (firehose_common_process_response_ack_message (rsp, "ACK", "false", &inner_error)) {
        /* We've seen in a EM7511 how the response to the download operation comes
         * followed *right away* in the same read() with the "XML not formed correctly"
         * warning plus an additional response with a NAK. In order to avoid failing
         * the teardown operation with that second response, we'll ignore it completely
         * if we have already detected a successful response earlier.
         */
        if (ctx->acked) {
            g_debug ("[qfu-sahara-device] ignoring additional response message detected");
            g_clear_error (&inner_error);
            return TRUE;
        }
        if (inner_error) {
            g_propagate_error (error, inner_error);
            return FALSE;
        }
        ctx->acked = TRUE;
        return TRUE;
    }

    g_debug ("[qfu-sahara-device] unknown firehose message received");
    return TRUE;
}

static gboolean
firehose_teardown_download_check_completion (QfuSaharaDevice                 *self,
                                             FirehoseTeardownDownloadContext *ctx)
{
    return (ctx->acked);
}

gboolean
qfu_sahara_device_firehose_teardown_download (QfuSaharaDevice  *self,
                                              QfuImage         *image,
                                              GCancellable     *cancellable,
                                              GError          **error)
{
    FirehoseTeardownDownloadContext ctx = {
        .acked = FALSE,
    };

    return firehose_operation_run (self,
                                   NULL, /* PrepareRequestCallback */
                                   (ProcessResponseCallback) firehose_teardown_download_process_response,
                                   (CheckCompletionCallback) firehose_teardown_download_check_completion,
                                   NULL, /* InitRetryCallback */
                                   0,    /* max_retries */
                                   FIREHOSE_TEARDOWN_DOWNLOAD_TIMEOUT_SECS,
                                   &ctx,
                                   cancellable,
                                   error);
}

/******************************************************************************/
/* Firehose reset */

#define FIREHOSE_RESET_TIMEOUT_SECS 10
#define FIREHOSE_RESET_MAX_RETRIES  10

typedef struct {
    gboolean sent;
    gboolean acked;
} FirehoseResetContext;

static const gchar *
firehose_reset_prepare_request (QfuSaharaDevice      *self,
                                FirehoseResetContext *ctx)
{
    if (!ctx->sent) {
        ctx->sent = TRUE;
        g_debug ("[qfu-sahara-device] sending firehose reset...");
        qfu_firehose_message_build_reset (self->priv->buffer->data, self->priv->buffer->len);
        return (const gchar *)self->priv->buffer->data;
    }

    return NULL;
}

static gboolean
firehose_reset_process_response (QfuSaharaDevice       *self,
                                 const gchar           *rsp,
                                 FirehoseResetContext  *ctx,
                                 GError               **error)
{
    GError *inner_error = NULL;

    if (firehose_common_process_log_message (rsp))
        return TRUE;

    if (firehose_common_process_response_ack_message (rsp, "ACK", NULL, &inner_error)) {
        if (inner_error) {
            g_propagate_error (error, inner_error);
            return FALSE;
        }
        ctx->acked = TRUE;
        return TRUE;
    }

    g_debug ("[qfu-sahara-device] unknown firehose message received");
    return TRUE;
}

static gboolean
firehose_reset_check_completion (QfuSaharaDevice      *self,
                                 FirehoseResetContext *ctx)
{
    return (ctx->acked);
}

static void
firehose_reset_init_retry (QfuSaharaDevice      *self,
                           FirehoseResetContext *ctx)
{
    ctx->sent  = FALSE;
    ctx->acked = FALSE;
}

gboolean
qfu_sahara_device_firehose_reset (QfuSaharaDevice  *self,
                                  GCancellable     *cancellable,
                                  GError          **error)
{
    FirehoseResetContext ctx = {
        .sent  = FALSE,
        .acked = FALSE,
    };

    return firehose_operation_run (self,
                                   (PrepareRequestCallback)  firehose_reset_prepare_request,
                                   (ProcessResponseCallback) firehose_reset_process_response,
                                   (CheckCompletionCallback) firehose_reset_check_completion,
                                   (InitRetryCallback)       firehose_reset_init_retry,
                                   FIREHOSE_RESET_MAX_RETRIES,
                                   FIREHOSE_RESET_TIMEOUT_SECS,
                                   &ctx,
                                   cancellable,
                                   error);
}

/******************************************************************************/
/* Firehose initialization */

#define FIREHOSE_INIT_TIMEOUT_SECS 10

typedef enum {
    FIREHOSE_INIT_STEP_PING,
    FIREHOSE_INIT_STEP_WAIT_PING,
    FIREHOSE_INIT_STEP_CONFIGURE,
    FIREHOSE_INIT_STEP_WAIT_CONFIGURE,
    FIREHOSE_INIT_STEP_STORAGE_INFO,
    FIREHOSE_INIT_STEP_WAIT_STORAGE_INFO,
    FIREHOSE_INIT_STEP_LAST,
} FirehoseInitStep;

typedef struct {
    FirehoseInitStep step;
    guint            max_payload_size_to_target_in_bytes;
    guint            sector_size_in_bytes;
    guint            num_partition_sectors;
    guint            total_sector_size_in_bytes;
    guint            pages_in_block;
} FirehoseInitContext;

static gboolean
firehose_init_context_process_log_message (const gchar         *rsp,
                                           FirehoseInitContext *ctx)
{
    gchar  *value = NULL;
    gchar **strv;

    if (!qfu_firehose_message_parse_log (rsp, &value))
        return FALSE;

    /* The log message may contain specific settings that we want to read */
    strv = g_strsplit (value, "=", -1);
    if (g_strv_length (strv) == 2) {
        g_strstrip (strv[0]);
        g_strstrip (strv[1]);
        if (g_ascii_strcasecmp (strv[0], "sector_size_in_bytes") == 0)
            ctx->sector_size_in_bytes = atoi (strv[1]);
        else if (g_ascii_strcasecmp (strv[0], "num_partition_sectors") == 0)
            ctx->num_partition_sectors = atoi (strv[1]);
        else if (g_ascii_strcasecmp (strv[0], "total_sector_size_in_bytes") == 0)
            ctx->total_sector_size_in_bytes = atoi (strv[1]);
        else if (g_ascii_strcasecmp (strv[0], "pages_in_block") == 0)
            ctx->pages_in_block = atoi (strv[1]);
    }
    g_strfreev (strv);

    g_debug ("[qfu-sahara-device] firehose log: %s", value);
    g_free (value);
    return TRUE;
}

static gboolean
firehose_init_context_process_response_configure_message (const gchar          *rsp,
                                                          FirehoseInitContext  *ctx,
                                                          GError              **error)
{
    guint32 max_payload_size_to_target_in_bytes = 0;

    if (!qfu_firehose_message_parse_response_configure (rsp, &max_payload_size_to_target_in_bytes))
        return FALSE;

    if (max_payload_size_to_target_in_bytes > 0) {
        g_debug ("[qfu-sahara-device] firehose requested max payload size: %u bytes", max_payload_size_to_target_in_bytes);
        ctx->max_payload_size_to_target_in_bytes = max_payload_size_to_target_in_bytes;
    } else {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "unexpected max payload size: %u", max_payload_size_to_target_in_bytes);
    }

    return TRUE;
}

static const gchar *
firehose_init_prepare_request (QfuSaharaDevice     *self,
                               FirehoseInitContext *ctx)
{
    switch (ctx->step) {
    case FIREHOSE_INIT_STEP_PING:
        g_debug ("[qfu-sahara-device] sending firehose ping...");
        qfu_firehose_message_build_ping (self->priv->buffer->data, self->priv->buffer->len);
        ctx->step++;
        return (const gchar *)self->priv->buffer->data;
    case FIREHOSE_INIT_STEP_WAIT_PING:
        /* not sending anything, just processing responses */
        return NULL;
    case FIREHOSE_INIT_STEP_CONFIGURE:
        g_debug ("[qfu-sahara-device] sending firehose configure...");
        qfu_firehose_message_build_configure (self->priv->buffer->data, self->priv->buffer->len, 0);
        ctx->step++;
        return (const gchar *)self->priv->buffer->data;
    case FIREHOSE_INIT_STEP_WAIT_CONFIGURE:
        /* not sending anything, just processing responses */
        return NULL;
    case FIREHOSE_INIT_STEP_STORAGE_INFO:
        g_debug ("[qfu-sahara-device] sending firehose storage info request...");
        qfu_firehose_message_build_get_storage_info (self->priv->buffer->data, self->priv->buffer->len);
        ctx->step++;
        return (const gchar *)self->priv->buffer->data;
    case FIREHOSE_INIT_STEP_WAIT_STORAGE_INFO:
        /* not sending anything, just processing responses */
        return NULL;
    case FIREHOSE_INIT_STEP_LAST:
    default:
        g_assert_not_reached ();
        return NULL;
    }
}

static gboolean
firehose_init_process_response (QfuSaharaDevice      *self,
                                const gchar          *rsp,
                                FirehoseInitContext  *ctx,
                                GError             **error)
{
    GError *inner_error = NULL;

    if (firehose_init_context_process_log_message (rsp, ctx))
        return TRUE;

    if (firehose_common_process_response_ack_message (rsp, "ACK", NULL, &inner_error)) {
        if (inner_error) {
            g_propagate_error (error, inner_error);
            return FALSE;
        }
        /* if we were expecting a response, go on to next step */
        if (ctx->step == FIREHOSE_INIT_STEP_WAIT_PING || ctx->step == FIREHOSE_INIT_STEP_WAIT_STORAGE_INFO)
            ctx->step++;
        return TRUE;
    }

    if (firehose_init_context_process_response_configure_message (rsp, ctx, &inner_error)) {
        if (inner_error) {
            g_propagate_error (error, inner_error);
            return FALSE;
        }
        /* if we were expecting a response, go on to next step */
        if (ctx->step == FIREHOSE_INIT_STEP_WAIT_CONFIGURE)
            ctx->step++;
        return TRUE;
    }

    g_debug ("[qfu-sahara-device] unknown firehose message received");
    return TRUE;
}

static gboolean
firehose_init_check_completion (QfuSaharaDevice     *self,
                                FirehoseInitContext *ctx)
{
    return (ctx->step == FIREHOSE_INIT_STEP_LAST);
}

static gboolean
sahara_device_firehose_init (QfuSaharaDevice  *self,
                             GCancellable     *cancellable,
                             GError          **error)
{
    FirehoseInitContext ctx = {
        .step                                = FIREHOSE_INIT_STEP_PING,
        .max_payload_size_to_target_in_bytes = 0,
        .sector_size_in_bytes                = 0,
        .num_partition_sectors               = 0,
        .total_sector_size_in_bytes          = 0,
        .pages_in_block                      = 0,
    };

    if (!firehose_operation_run (self,
                                 (PrepareRequestCallback)  firehose_init_prepare_request,
                                 (ProcessResponseCallback) firehose_init_process_response,
                                 (CheckCompletionCallback) firehose_init_check_completion,
                                 NULL, /* InitRetryCallback */
                                 0,    /* max_retries */
                                 FIREHOSE_INIT_TIMEOUT_SECS,
                                 &ctx,
                                 cancellable,
                                 error))
        return FALSE;

#define VALIDATE_FIELD(FIELD,DESCR) do {                                          \
        if (!ctx.FIELD) {                                                         \
            g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "unknown " DESCR); \
            return FALSE;                                                         \
        }                                                                         \
        self->priv->FIELD = ctx.FIELD;                                                \
    } while (0)

    VALIDATE_FIELD (max_payload_size_to_target_in_bytes, "max payload size");
    VALIDATE_FIELD (sector_size_in_bytes,                "sector size");
    VALIDATE_FIELD (num_partition_sectors,               "number of partition sectors");
    VALIDATE_FIELD (total_sector_size_in_bytes,          "total sector size");
    VALIDATE_FIELD (pages_in_block,                      "pages in block");

#undef VALIDATE_FIELD

    /* compute transfer block size, which will be equal to the max payload size
     * to target if it's multiple of sector size */
    self->priv->transfer_block_size = (self->priv->max_payload_size_to_target_in_bytes / self->priv->sector_size_in_bytes) * self->priv->sector_size_in_bytes;
    g_assert (self->priv->transfer_block_size <= self->priv->max_payload_size_to_target_in_bytes);
    g_assert (self->priv->transfer_block_size > 0);

    return TRUE;
}

/******************************************************************************/
/* Sahara initialization */

#define SAHARA_MAX_PROTOCOL_STEP_ATTEMPTS 5

typedef enum {
    SAHARA_PROTOCOL_STEP_UNKNOWN,
    SAHARA_PROTOCOL_STEP_INIT,
    SAHARA_PROTOCOL_STEP_HELLO,
    SAHARA_PROTOCOL_STEP_SWITCH,
    SAHARA_PROTOCOL_STEP_DATA,
    SAHARA_PROTOCOL_STEP_LAST,
} SaharaProtocolStep;

static gboolean
validate_firehose_switch_confirmation (const guint8  *rsp,
                                       gsize          rsplen,
                                       GError       **error)
{
    if (!rsplen) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "no confirmation received");
        return FALSE;
    }

    if (!validate_ascii_print (rsp, rsplen)) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "invalid confirmation data");
        return FALSE;
    }

    return TRUE;
}

static SaharaProtocolStep
sahara_device_run_protocol_step (QfuSaharaDevice    *self,
                                 SaharaProtocolStep  step,
                                 GCancellable       *cancellable,
                                 GError            **error)
{
    SaharaProtocolStep  next_step = SAHARA_PROTOCOL_STEP_UNKNOWN;
    gsize               reqlen = 0;
    gssize              rsplen;
    guint8             *rsp = NULL;

    memset (self->priv->buffer->data, 0, self->priv->buffer->len);

    switch (step) {
    case SAHARA_PROTOCOL_STEP_INIT:
        /*
         * Just after opening the port we must NOT SEND anything to the device.
         * If we do that, we would get the sahara hello back, but the initialization
         * process would fail later on with a command-end-image-transfer message
         * reporting that the 0x0000ff00 command to switch to firehose protocol is
         * unsupported.
         */
        break;
    case SAHARA_PROTOCOL_STEP_HELLO:
        reqlen = qfu_sahara_response_hello_build (self->priv->buffer->data, self->priv->buffer->len);
        break;
    case SAHARA_PROTOCOL_STEP_SWITCH:
        reqlen = qfu_sahara_request_switch_build (self->priv->buffer->data, self->priv->buffer->len);
        break;
    case SAHARA_PROTOCOL_STEP_DATA:
        reqlen = qfu_sahara_request_switch_data_build (self->priv->buffer->data, self->priv->buffer->len);
        break;
    case SAHARA_PROTOCOL_STEP_UNKNOWN:
    case SAHARA_PROTOCOL_STEP_LAST:
    default:
        g_assert_not_reached ();
        break;
    }

    rsplen = send_receive (self,
                           reqlen ? self->priv->buffer->data : NULL,
                           reqlen,
                           3,
                           &rsp,
                           cancellable,
                           error);
    if (rsplen < 0)
        return SAHARA_PROTOCOL_STEP_UNKNOWN;

    if (rsplen == 0) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "no sahara response received");
        return SAHARA_PROTOCOL_STEP_UNKNOWN;
    }

    /* The sahara initialization finishes once the switch to firehose is confirmed.
     * The EM7565 replies "confirmed" explicitly, but we'll just accept any printable
     * ASCII string. */
    if (step == SAHARA_PROTOCOL_STEP_DATA) {
        if (!validate_firehose_switch_confirmation (rsp, rsplen, error))
            return SAHARA_PROTOCOL_STEP_UNKNOWN;

        /* initialization finished */
        g_debug ("[qfu-sahara-device] sahara initialization finished: %.*s", (gint)rsplen, rsp);
        return SAHARA_PROTOCOL_STEP_LAST;
    }

    /* in case several messages are received together, parse and process them one by one */
    while (rsplen > 0) {
        gsize            msglen;
        QfuSaharaHeader *msghdr;

        if ((gsize)rsplen < QFU_SAHARA_MESSAGE_MAX_HEADER_SIZE) {
            g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "sahara header not fully received: %" G_GSSIZE_FORMAT " < %" G_GSIZE_FORMAT,
                         rsplen, QFU_SAHARA_MESSAGE_MAX_HEADER_SIZE);
            return SAHARA_PROTOCOL_STEP_UNKNOWN;
        }

        msghdr = (QfuSaharaHeader *)rsp;
        msglen = GUINT32_FROM_LE (msghdr->size);
        if ((gsize)rsplen < msglen) {
            g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "sahara message not fully received: %" G_GSSIZE_FORMAT " < %" G_GSIZE_FORMAT,
                         rsplen, msglen);
            return SAHARA_PROTOCOL_STEP_UNKNOWN;
        }

        switch (GUINT32_FROM_LE (msghdr->cmd)) {
        case QFU_SAHARA_CMD_HELLO_REQ:
            if (!qfu_sahara_request_hello_parse (rsp, rsplen, error))
                return SAHARA_PROTOCOL_STEP_UNKNOWN;
            g_debug ("[qfu-sahara-device] sahara hello request received");
            next_step = SAHARA_PROTOCOL_STEP_HELLO;
            break;
        case QFU_SAHARA_CMD_COMMAND_READY:
            g_debug ("[qfu-sahara-device] module is ready for commands");
            next_step = SAHARA_PROTOCOL_STEP_SWITCH;
            break;
        case QFU_SAHARA_CMD_COMMAND_EXECUTE_RSP:
            g_debug ("[qfu-sahara-device] request to switch to firehose accepted");
            if (!qfu_sahara_response_switch_parse (rsp, rsplen, error))
                return SAHARA_PROTOCOL_STEP_UNKNOWN;
            next_step = SAHARA_PROTOCOL_STEP_DATA;
            break;
        case QFU_SAHARA_CMD_COMMAND_END_IMAGE_TRANSFER:
            if (!qfu_sahara_response_end_image_transfer_parse (rsp, rsplen, error))
                return SAHARA_PROTOCOL_STEP_UNKNOWN;
            g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "unexpected sahara message");
            return SAHARA_PROTOCOL_STEP_UNKNOWN;
        default:
            g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "unsupported sahara message: '0x%08x'", GUINT32_FROM_LE (msghdr->cmd));
            return SAHARA_PROTOCOL_STEP_UNKNOWN;
        }

        rsp    += msglen;
        rsplen -= msglen;
    }

    g_assert (next_step != SAHARA_PROTOCOL_STEP_UNKNOWN);
    return next_step;
}

static gboolean
sahara_device_initialize (QfuSaharaDevice  *self,
                          GCancellable     *cancellable,
                          GError          **error)
{
    SaharaProtocolStep step = SAHARA_PROTOCOL_STEP_INIT;
    guint              n_attempts = 0;

    while (step != SAHARA_PROTOCOL_STEP_LAST) {
        SaharaProtocolStep next_step;

        /* check cancellation */
        if (g_cancellable_is_cancelled (cancellable)) {
            g_set_error (error, G_IO_ERROR, G_IO_ERROR_CANCELLED, "operation cancelled");
            return FALSE;
        }

        /* check step error */
        next_step = sahara_device_run_protocol_step (self, step, cancellable, error);
        if (next_step == SAHARA_PROTOCOL_STEP_UNKNOWN)
            return FALSE;

        /* retrying with same step? */
        if (next_step == step) {
            if (++n_attempts == SAHARA_MAX_PROTOCOL_STEP_ATTEMPTS) {
                g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "too many attempts");
                return FALSE;
            }
        } else
            n_attempts = 0;

        /* continue to next step */
        step = next_step;
    }

    return TRUE;
}

/******************************************************************************/

static gboolean
initable_init (GInitable     *initable,
               GCancellable  *cancellable,
               GError       **error)
{
    QfuSaharaDevice *self;
    struct termios   terminal_data;
    gchar           *path = NULL;
    GError          *inner_error = NULL;

    self = QFU_SAHARA_DEVICE (initable);

    if (g_cancellable_set_error_if_cancelled (cancellable, &inner_error))
        goto out;

    path = g_file_get_path (self->priv->file);
    g_debug ("[qfu-sahara-device] opening TTY: %s", path);

    self->priv->fd = open (path, O_RDWR | O_NOCTTY);
    if (self->priv->fd < 0) {
        inner_error = g_error_new (G_IO_ERROR, G_IO_ERROR_FAILED,
                                   "error opening serial device: %s",
                                   g_strerror (errno));
        goto out;
    }

    g_debug ("[qfu-sahara-device] setting terminal in raw mode...");
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

    /* We need to give some time to the device before trying to initialize
     * the sahara protocol, because otherwise the sequence won't work. If
     * this wait time is not given, the initialization sequence will fail with
     * a command-end-image-transfer message reporting that the 0x0000ff00
     * command to switch to firehose protocol is unsupported.
     *
     * 2 full seconds selected a bit arbitrarily, didn't get any failure when
     * using this amount of time. */
    g_debug ("[qfu-sahara-device] waiting time for device to boot properly...");
    g_usleep (2 * G_USEC_PER_SEC);

    g_debug ("[qfu-sahara-device] initializing sahara protocol...");
    if (!sahara_device_initialize (self, cancellable, &inner_error))
        goto out;

    g_debug ("[qfu-sahara-device] initializing firehose protocol...");
    if (!sahara_device_firehose_init (self, cancellable, &inner_error))
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

QfuSaharaDevice *
qfu_sahara_device_new (GFile         *file,
                       GCancellable  *cancellable,
                       GError       **error)
{
    g_return_val_if_fail (G_IS_FILE (file), NULL);

    return QFU_SAHARA_DEVICE (g_initable_new (QFU_TYPE_SAHARA_DEVICE,
                                              cancellable,
                                              error,
                                              "file", file,
                                              NULL));
}


static void
qfu_sahara_device_init (QfuSaharaDevice *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, QFU_TYPE_SAHARA_DEVICE, QfuSaharaDevicePrivate);
    self->priv->fd = -1;
    /* Long buffer for I/O, way more than ever needed when using sahara/firehose */
    self->priv->buffer = g_byte_array_new ();
    g_byte_array_set_size (self->priv->buffer, QFU_IMAGE_CHUNK_SIZE);
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    QfuSaharaDevice *self = QFU_SAHARA_DEVICE (object);

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
    QfuSaharaDevice *self = QFU_SAHARA_DEVICE (object);

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
    QfuSaharaDevice *self = QFU_SAHARA_DEVICE (object);

    if (!(self->priv->fd < 0)) {
        close (self->priv->fd);
        self->priv->fd = -1;
    }

    G_OBJECT_CLASS (qfu_sahara_device_parent_class)->dispose (object);
}

static void
initable_iface_init (GInitableIface *iface)
{
    iface->init = initable_init;
}

static void
qfu_sahara_device_class_init (QfuSaharaDeviceClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (object_class, sizeof (QfuSaharaDevicePrivate));

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
