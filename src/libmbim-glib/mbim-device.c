/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * libmbim-glib -- GLib/GIO based library to control MBIM devices
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
 * Copyright (C) 2013 Aleksander Morgado <aleksander@lanedo.com>
 *
 * Implementation based on the 'QmiDevice' GObject from libqmi-glib.
 */

#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <gio/gio.h>
#include <gudev/gudev.h>
#include <sys/ioctl.h>
#define IOCTL_WDM_MAX_COMMAND _IOR('H', 0xA0, guint16)

#include "mbim-utils.h"
#include "mbim-device.h"
#include "mbim-message.h"
#include "mbim-message-private.h"
#include "mbim-error-types.h"

/**
 * SECTION:mbim-device
 * @title: MbimDevice
 * @short_description: Generic MBIM device handling routines
 *
 * #MbimDevice is a generic type in charge of controlling the access to the
 * managed MBIM port.
 *
 * A #MbimDevice can only handle one single MBIM port.
 */

static void async_initable_iface_init (GAsyncInitableIface *iface);

G_DEFINE_TYPE_EXTENDED (MbimDevice, mbim_device, G_TYPE_OBJECT, 0,
                        G_IMPLEMENT_INTERFACE (G_TYPE_ASYNC_INITABLE, async_initable_iface_init))

enum {
    PROP_0,
    PROP_FILE,
    PROP_TRANSACTION_ID,
    PROP_IN_SESSION,
    PROP_LAST
};

static GParamSpec *properties[PROP_LAST];

enum {
    SIGNAL_INDICATE_STATUS,
    SIGNAL_ERROR,
    SIGNAL_LAST
};

static guint signals[SIGNAL_LAST] = { 0 };

typedef enum {
    TRANSACTION_TYPE_HOST  = 0,
    TRANSACTION_TYPE_MODEM = 1,
    TRANSACTION_TYPE_LAST  = 2
} TransactionType;

struct _MbimDevicePrivate {
    /* File */
    GFile *file;
    gchar *path;
    gchar *path_display;

    /* I/O channel, set when the file is open */
    GIOChannel *iochannel;
    guint watch_id;
    GByteArray *response;

    /* HT to keep track of ongoing host/function transactions
     *  Host transactions:  created by us
     *  Modem transactions: modem-created indications with multiple fragments
     */
    GHashTable *transactions[TRANSACTION_TYPE_LAST];

    /* Transaction ID in the device */
    guint32 transaction_id;

    /* Flag to specify whether we're in a session */
    gboolean in_session;

    /* message size */
    guint16 max_control_transfer;
};

#define MAX_CONTROL_TRANSFER          4096
#define MAX_TIME_BETWEEN_FRAGMENTS_MS 1250

static void device_report_error (MbimDevice   *self,
                                 guint32       transaction_id,
                                 const GError *error);

/*****************************************************************************/
/* Message transactions (private) */

typedef struct {
    MbimDevice *self;
    guint32 transaction_id;
    TransactionType type;
} TransactionWaitContext;

typedef struct {
    MbimMessage *fragments;
    guint32 transaction_id;
    GSimpleAsyncResult *result;
    guint timeout_id;
    GCancellable *cancellable;
    gulong cancellable_id;
    TransactionWaitContext *wait_ctx;
} Transaction;

static Transaction *
transaction_new (MbimDevice          *self,
                 guint32              transaction_id,
                 GCancellable        *cancellable,
                 GAsyncReadyCallback  callback,
                 gpointer             user_data)
{
    Transaction *tr;

    tr = g_slice_new0 (Transaction);
    tr->transaction_id = transaction_id;
    tr->result = g_simple_async_result_new (G_OBJECT (self),
                                            callback,
                                            user_data,
                                            transaction_new);
    if (cancellable)
        tr->cancellable = g_object_ref (cancellable);

    return tr;
}

static void
transaction_complete_and_free (Transaction  *tr,
                               const GError *error)
{
    if (tr->timeout_id)
        g_source_remove (tr->timeout_id);

    if (tr->cancellable) {
        if (tr->cancellable_id)
            g_cancellable_disconnect (tr->cancellable, tr->cancellable_id);
        g_object_unref (tr->cancellable);
    }

    if (tr->wait_ctx)
        g_slice_free (TransactionWaitContext, tr->wait_ctx);

    if (error) {
        g_simple_async_result_set_from_error (tr->result, error);
        if (tr->fragments)
            mbim_message_unref (tr->fragments);
    } else {
        g_assert (tr->fragments != NULL);
        g_simple_async_result_set_op_res_gpointer (tr->result,
                                                   tr->fragments,
                                                   (GDestroyNotify) mbim_message_unref);
    }

    g_simple_async_result_complete_in_idle (tr->result);
    g_object_unref (tr->result);
    g_slice_free (Transaction, tr);
}

static Transaction *
device_release_transaction (MbimDevice      *self,
                            TransactionType  type,
                            guint32          transaction_id)
{
    Transaction *tr = NULL;

    if (self->priv->transactions[type]) {
        tr = g_hash_table_lookup (self->priv->transactions[type], GUINT_TO_POINTER (transaction_id));
        if (tr)
            /* If found, remove it from the HT */
            g_hash_table_remove (self->priv->transactions[type], GUINT_TO_POINTER (transaction_id));
    }

    return tr;
}

static gboolean
transaction_timed_out (TransactionWaitContext *ctx)
{
    Transaction *tr;
    GError *error = NULL;

    tr = device_release_transaction (ctx->self, ctx->type, ctx->transaction_id);
    tr->timeout_id = 0;

    /* If no fragment was received, complete transaction with a timeout error */
    if (!tr->fragments)
        error = g_error_new (MBIM_CORE_ERROR,
                             MBIM_CORE_ERROR_TIMEOUT,
                             "Transaction timed out");
    else {
        /* Fragment timeout... */
        error = g_error_new (MBIM_PROTOCOL_ERROR,
                             MBIM_PROTOCOL_ERROR_TIMEOUT_FRAGMENT,
                             "Fragment timed out");

        /* Also notify to the modem */
        device_report_error (ctx->self,
                             tr->transaction_id,
                             error);
    }

    transaction_complete_and_free (tr, error);
    g_error_free (error);

    return FALSE;
}

static void
transaction_cancelled (GCancellable           *cancellable,
                       TransactionWaitContext *ctx)
{
    Transaction *tr;
    GError *error = NULL;

    tr = device_release_transaction (ctx->self, ctx->type, ctx->transaction_id);
    tr->cancellable_id = 0;

    /* Complete transaction with an abort error */
    error = g_error_new (MBIM_CORE_ERROR,
                         MBIM_CORE_ERROR_ABORTED,
                         "Transaction aborted");
    transaction_complete_and_free (tr, error);
    g_error_free (error);
}

static gboolean
device_store_transaction (MbimDevice       *self,
                          TransactionType   type,
                          Transaction      *tr,
                          guint             timeout_ms,
                          GError          **error)
{
    if (G_UNLIKELY (!self->priv->transactions[type]))
        self->priv->transactions[type] = g_hash_table_new (g_direct_hash, g_direct_equal);

    tr->wait_ctx = g_slice_new (TransactionWaitContext);
    tr->wait_ctx->self = self;
     /* valid as long as the transaction is in the HT */
    tr->wait_ctx->transaction_id = tr->transaction_id;
    tr->wait_ctx->type = type;

    /* don't add timeout if one already exists */
    if (!tr->timeout_id) {
        tr->timeout_id = g_timeout_add (timeout_ms,
                                        (GSourceFunc)transaction_timed_out,
                                        tr->wait_ctx);
    }

    if (tr->cancellable && !tr->cancellable_id) {
        tr->cancellable_id = g_cancellable_connect (tr->cancellable,
                                                    (GCallback)transaction_cancelled,
                                                    tr->wait_ctx,
                                                    NULL);
        if (!tr->cancellable_id) {
            g_set_error_literal (error,
                                 MBIM_CORE_ERROR,
                                 MBIM_CORE_ERROR_ABORTED,
                                 "Request is already cancelled");
            return FALSE;
        }
    }

    /* Keep in the HT */
    g_hash_table_insert (self->priv->transactions[type], GUINT_TO_POINTER (tr->transaction_id), tr);

    return TRUE;
}

static Transaction *
device_match_transaction (MbimDevice        *self,
                          TransactionType    type,
                          const MbimMessage *message)
{
    /* msg can be either the original message or the response */
    return device_release_transaction (self, type, mbim_message_get_transaction_id (message));
}

/*****************************************************************************/

/**
 * mbim_device_get_file:
 * @self: a #MbimDevice.
 *
 * Get the #GFile associated with this #MbimDevice.
 *
 * Returns: a #GFile that must be freed with g_object_unref().
 */
GFile *
mbim_device_get_file (MbimDevice *self)
{
    GFile *file = NULL;

    g_return_val_if_fail (MBIM_IS_DEVICE (self), NULL);

    g_object_get (G_OBJECT (self),
                  MBIM_DEVICE_FILE, &file,
                  NULL);
    return file;
}

/**
 * mbim_device_peek_file:
 * @self: a #MbimDevice.
 *
 * Get the #GFile associated with this #MbimDevice, without increasing the reference count
 * on the returned object.
 *
 * Returns: a #GFile. Do not free the returned object, it is owned by @self.
 */
GFile *
mbim_device_peek_file (MbimDevice *self)
{
    g_return_val_if_fail (MBIM_IS_DEVICE (self), NULL);

    return self->priv->file;
}

/**
 * mbim_device_get_path:
 * @self: a #MbimDevice.
 *
 * Get the system path of the underlying MBIM device.
 *
 * Returns: the system path of the device.
 */
const gchar *
mbim_device_get_path (MbimDevice *self)
{
    g_return_val_if_fail (MBIM_IS_DEVICE (self), NULL);

    return self->priv->path;
}

/**
 * mbim_device_get_path_display:
 * @self: a #MbimDevice.
 *
 * Get the system path of the underlying MBIM device in UTF-8.
 *
 * Returns: UTF-8 encoded system path of the device.
 */
const gchar *
mbim_device_get_path_display (MbimDevice *self)
{
    g_return_val_if_fail (MBIM_IS_DEVICE (self), NULL);

    return self->priv->path_display;
}

/**
 * mbim_device_is_open:
 * @self: a #MbimDevice.
 *
 * Checks whether the #MbimDevice is open for I/O.
 *
 * Returns: %TRUE if @self is open, %FALSE otherwise.
 */
gboolean
mbim_device_is_open (MbimDevice *self)
{
    g_return_val_if_fail (MBIM_IS_DEVICE (self), FALSE);

    return !!self->priv->iochannel;
}

/*****************************************************************************/
/* Open device */

static void
indication_ready (MbimDevice   *self,
                  GAsyncResult *res)
{
    GError *error = NULL;
    MbimMessage *indication;

    if (g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (res), &error)) {
        g_debug ("[%s] Error processing indication message: %s",
                 self->priv->path_display,
                 error->message);
        g_error_free (error);
        return;
    }

    indication = g_simple_async_result_get_op_res_gpointer (G_SIMPLE_ASYNC_RESULT (res));
    g_signal_emit (self, signals[SIGNAL_INDICATE_STATUS], 0, indication);
}

static void
process_message (MbimDevice  *self,
                 const MbimMessage *message)
{
    if (mbim_utils_get_traces_enabled ()) {
        gchar *printable;
        gboolean is_partial_fragment;

        is_partial_fragment = (_mbim_message_is_fragment (message) &&
                               _mbim_message_fragment_get_total (message) > 1);

        printable = __mbim_utils_str_hex (((GByteArray *)message)->data,
                                          ((GByteArray *)message)->len,
                                          ':');
        g_debug ("[%s] Received message...%s\n"
                 ">>>>>> RAW:\n"
                 ">>>>>>   length = %u\n"
                 ">>>>>>   data   = %s\n",
                 self->priv->path_display,
                 is_partial_fragment ? " (partial fragment)" : "",
                 ((GByteArray *)message)->len,
                 printable);
        g_free (printable);

        if (is_partial_fragment) {
            printable = mbim_message_get_printable (message, ">>>>>> ", TRUE);
            g_debug ("[%s] Received message fragment (translated)...\n%s",
                     self->priv->path_display,
                     printable);
            g_free (printable);
        }
    }

    switch (MBIM_MESSAGE_GET_MESSAGE_TYPE (message)) {
    case MBIM_MESSAGE_TYPE_OPEN_DONE:
    case MBIM_MESSAGE_TYPE_CLOSE_DONE:
    case MBIM_MESSAGE_TYPE_COMMAND_DONE:
    case MBIM_MESSAGE_TYPE_INDICATE_STATUS: {
        GError *error = NULL;
        Transaction *tr;

        if (MBIM_MESSAGE_GET_MESSAGE_TYPE (message) == MBIM_MESSAGE_TYPE_INDICATE_STATUS) {
            /* Grab transaction */
            tr = device_match_transaction (self, TRANSACTION_TYPE_MODEM, message);
            if (!tr)
                /* Create new transaction for the indication */
                tr = transaction_new (self,
                                      mbim_message_get_transaction_id (message),
                                      NULL, /* no cancellable */
                                      (GAsyncReadyCallback)indication_ready,
                                      NULL);
        } else {
            /* Grab transaction */
            tr = device_match_transaction (self, TRANSACTION_TYPE_HOST, message);
            if (!tr) {
                g_debug ("[%s] No transaction matched in received message",
                         self->priv->path_display);
                return;
            }

            /* If the message doesn't have fragments, we're done */
            if (!_mbim_message_is_fragment (message)) {
                g_assert (tr->fragments == NULL);
                tr->fragments = mbim_message_dup (message);
                transaction_complete_and_free (tr, NULL);
                return;
            }
        }

        /* More than one fragment expected; is this the first one? */
        if (!tr->fragments)
            tr->fragments = _mbim_message_fragment_collector_init (message, &error);
        else
            _mbim_message_fragment_collector_add (tr->fragments, message, &error);

        if (error) {
            device_report_error (self,
                                 tr->transaction_id,
                                 error);
            transaction_complete_and_free (tr, error);
            g_error_free (error);
            return;
        }

        /* Did we get all needed fragments? */
        if (_mbim_message_fragment_collector_complete (tr->fragments)) {
            /* Now, translate the whole message */
            if (mbim_utils_get_traces_enabled ()) {
                gchar *printable;

                printable = mbim_message_get_printable (tr->fragments, ">>>>>> ", FALSE);
                g_debug ("[%s] Received message (translated)...\n%s",
                         self->priv->path_display,
                         printable);
                g_free (printable);
            }

            transaction_complete_and_free (tr, NULL);
            return;
        }

        /* Need more fragments, store transaction */
        g_assert (device_store_transaction (self,
                                            TRANSACTION_TYPE_HOST,
                                            tr,
                                            MAX_TIME_BETWEEN_FRAGMENTS_MS,
                                            NULL));
        return;
    }

    case MBIM_MESSAGE_TYPE_FUNCTION_ERROR: {
        GError *error_indication;

        if (mbim_utils_get_traces_enabled ()) {
            gchar *printable;

            printable = mbim_message_get_printable (message, ">>>>>> ", FALSE);
            g_debug ("[%s] Received message (translated)...\n%s",
                     self->priv->path_display,
                     printable);
            g_free (printable);
        }

        error_indication = mbim_message_error_get_error (message);
        g_signal_emit (self, signals[SIGNAL_ERROR], 0, error_indication);
        g_error_free (error_indication);
        return;
    }

    case MBIM_MESSAGE_TYPE_INVALID:
    case MBIM_MESSAGE_TYPE_OPEN:
    case MBIM_MESSAGE_TYPE_CLOSE:
    case MBIM_MESSAGE_TYPE_COMMAND:
    case MBIM_MESSAGE_TYPE_HOST_ERROR:
        /* Shouldn't expect host-generated messages as replies */
        g_warning ("[%s] Host-generated message received: ignoring",
                   self->priv->path_display);
        return;
    }
}

static void
parse_response (MbimDevice *self)
{
    do {
        const MbimMessage *message;
        guint32 in_length;

        /* If not even the MBIM header available, just return */
        if (self->priv->response->len < 12)
            return;

        message = (const MbimMessage *)self->priv->response;

        /* No full message yet */
        in_length = mbim_message_get_message_length (message);

        if (self->priv->response->len < in_length)
            return;

        /* Play with the received message */
        process_message (self, message);

        /* Remove message from buffer */
        g_byte_array_remove_range (self->priv->response, 0, in_length);
    } while (self->priv->response->len > 0);
}

static gboolean
data_available (GIOChannel *source,
                GIOCondition condition,
                MbimDevice *self)
{
    gsize bytes_read;
    GIOStatus status;
    gchar buffer[MAX_CONTROL_TRANSFER + 1];

    if (condition & G_IO_HUP) {
        g_debug ("[%s] unexpected port hangup!",
                 self->priv->path_display);

        if (self->priv->response &&
            self->priv->response->len)
            g_byte_array_remove_range (self->priv->response, 0, self->priv->response->len);

        mbim_device_close_force (self, NULL);
        return FALSE;
    }

    if (condition & G_IO_ERR) {
        if (self->priv->response &&
            self->priv->response->len)
            g_byte_array_remove_range (self->priv->response, 0, self->priv->response->len);
        return TRUE;
    }

    /* If not ready yet, prepare the response with default initial size. */
    if (G_UNLIKELY (!self->priv->response))
        self->priv->response = g_byte_array_sized_new (500);

    do {
        GError *error = NULL;

        status = g_io_channel_read_chars (source,
                                          buffer,
                                          self->priv->max_control_transfer,
                                          &bytes_read,
                                          &error);
        if (status == G_IO_STATUS_ERROR) {
            if (error) {
                g_warning ("[%s] error reading from the IOChannel: '%s'",
                           self->priv->path_display,
                           error->message);
                g_error_free (error);
            }

            /* Port is closed; we're done */
            if (self->priv->watch_id == 0)
                break;
        }

        /* If no bytes read, just let g_io_channel wait for more data */
        if (bytes_read == 0)
            break;

        if (bytes_read > 0)
            g_byte_array_append (self->priv->response, (const guint8 *)buffer, bytes_read);

        /* Try to parse what we already got */
        parse_response (self);

        /* And keep on if we were told to keep on */
    } while (bytes_read == self->priv->max_control_transfer || status == G_IO_STATUS_AGAIN);

    return TRUE;
}

/* "MBIM Control Model Functional Descriptor" */
struct usb_cdc_mbim_desc {
    guint8  bLength;
    guint8  bDescriptorType;
    guint8  bDescriptorSubType;
    guint16 bcdMBIMVersion;
    guint16 wMaxControlMessage;
    guint8  bNumberFilters;
    guint8  bMaxFilterSize;
    guint16 wMaxSegmentSize;
    guint8  bmNetworkCapabilities;
} __attribute__ ((packed));

static guint16
read_max_control_transfer (MbimDevice *self)
{
    static const guint8 mbim_signature[4] = { 0x0c, 0x24, 0x1b, 0x00 };
    guint16 max = MAX_CONTROL_TRANSFER;
    GUdevClient *client;
    GUdevDevice *device = NULL;
    GUdevDevice *parent_device = NULL;
    GUdevDevice *grandparent_device = NULL;
    gchar *descriptors_path = NULL;
    gchar *device_basename = NULL;
    GError *error = NULL;
    gchar *contents = NULL;
    gsize length = 0;
    guint i;

    client = g_udev_client_new (NULL);
    if (!G_UDEV_IS_CLIENT (client)) {
        g_warning ("[%s] Couldn't get udev client",
                   self->priv->path_display);
        goto out;
    }

    /* We need to get the sysfs of the cdc-wdm's grandfather:
     *
     *   * Device's sysfs path is like:
     *      /sys/devices/pci0000:00/0000:00:1d.0/usb2/2-1/2-1.5/2-1.5:2.0/usbmisc/cdc-wdm0
     *   * Parent's sysfs path is like:
     *      /sys/devices/pci0000:00/0000:00:1d.0/usb2/2-1/2-1.5/2-1.5:2.0
     *   * Grandparent's sysfs path is like:
     *      /sys/devices/pci0000:00/0000:00:1d.0/usb2/2-1/2-1.5
     *
     *   Which is the one with the descriptors file.
     */

    device_basename = g_path_get_basename (self->priv->path);
    device = g_udev_client_query_by_subsystem_and_name (client, "usb", device_basename);
    if (!device) {
        device = g_udev_client_query_by_subsystem_and_name (client, "usbmisc", device_basename);
        if (!device) {
            g_warning ("[%s] Couldn't find udev device",
                       self->priv->path_display);
            goto out;
        }
    }

    parent_device = g_udev_device_get_parent (device);
    if (!parent_device) {
        g_warning ("[%s] Couldn't find parent udev device",
                   self->priv->path_display);
        goto out;
    }

    grandparent_device = g_udev_device_get_parent (parent_device);
    if (!grandparent_device) {
        g_warning ("[%s] Couldn't find grandparent udev device",
                   self->priv->path_display);
        goto out;
    }

    descriptors_path = g_build_path (G_DIR_SEPARATOR_S,
                                     g_udev_device_get_sysfs_path (grandparent_device),
                                     "descriptors",
                                     NULL);
    if (!g_file_get_contents (descriptors_path,
                              &contents,
                              &length,
                              &error)) {
        g_warning ("[%s] Couldn't read descriptors file: %s",
                   self->priv->path_display,
                   error->message);
        g_error_free (error);
        goto out;
    }

    i = 0;
    while (i <= (length - sizeof (struct usb_cdc_mbim_desc))) {
        /* Try to match the MBIM descriptor signature */
        if ((memcmp (&contents[i], mbim_signature, sizeof (mbim_signature)) == 0)) {
            /* Found! */
            max = GUINT16_FROM_LE (((struct usb_cdc_mbim_desc *)&contents[i])->wMaxControlMessage);
            g_debug ("[%s] Read max control message size from descriptors file: %" G_GUINT16_FORMAT,
                     self->priv->path_display,
                     max);
            goto out;
        }

        /* The first byte of the descriptor info is the length; so keep on
         * skipping descriptors until we match the MBIM one */
        i += contents[i];
    }

    g_warning ("[%s] Couldn't find MBIM signature in descriptors file",
               self->priv->path_display);

out:
    g_free (contents);
    g_free (device_basename);
    g_free (descriptors_path);
    if (parent_device)
        g_object_unref (parent_device);
    if (grandparent_device)
        g_object_unref (grandparent_device);
    if (device)
        g_object_unref (device);
    if (client)
        g_object_unref (client);

    return max;
}

static gboolean
create_iochannel (MbimDevice *self,
                  GError **error)
{
    GError *inner_error = NULL;
    gint fd;
    guint16 max;

    if (self->priv->iochannel) {
        g_set_error_literal (error,
                             MBIM_CORE_ERROR,
                             MBIM_CORE_ERROR_WRONG_STATE,
                             "Already open");
        return FALSE;
    }

    g_assert (self->priv->file);
    g_assert (self->priv->path);

    errno = 0;
    fd = open (self->priv->path, O_RDWR | O_EXCL | O_NONBLOCK | O_NOCTTY);
    if (fd < 0) {
        g_set_error (error,
                     MBIM_CORE_ERROR,
                     MBIM_CORE_ERROR_FAILED,
                     "Cannot open device file '%s': %s",
                     self->priv->path_display,
                     strerror (errno));
        return FALSE;
    }

    /* Query message size */
    if (ioctl (fd, IOCTL_WDM_MAX_COMMAND, &max) < 0) {
        g_debug ("[%s] Couldn't query maximum message size: "
                 "IOCTL_WDM_MAX_COMMAND failed: %s",
                 self->priv->path_display,
                 strerror (errno));
        /* Fallback, try to read the descriptor file */
        max = read_max_control_transfer (self);
    } else {
        g_debug ("[%s] Queried max control message size: %" G_GUINT16_FORMAT,
                 self->priv->path_display,
                 max);
    }
    self->priv->max_control_transfer = max;

    /* Create new GIOChannel */
    self->priv->iochannel = g_io_channel_unix_new (fd);

    /* We don't want UTF-8 encoding, we're playing with raw binary data */
    g_io_channel_set_encoding (self->priv->iochannel, NULL, NULL);

    /* We don't want to get the channel buffered */
    g_io_channel_set_buffered (self->priv->iochannel, FALSE);

    /* Let the GIOChannel own the FD */
    g_io_channel_set_close_on_unref (self->priv->iochannel, TRUE);

    /* We don't want to get blocked while writing stuff */
    if (!g_io_channel_set_flags (self->priv->iochannel,
                                 G_IO_FLAG_NONBLOCK,
                                 &inner_error)) {
        g_prefix_error (&inner_error, "Cannot set non-blocking channel: ");
        g_propagate_error (error, inner_error);
        g_io_channel_shutdown (self->priv->iochannel, FALSE, NULL);
        g_io_channel_unref (self->priv->iochannel);
        self->priv->iochannel = NULL;
        return FALSE;
    }

    self->priv->watch_id = g_io_add_watch (self->priv->iochannel,
                                           G_IO_IN | G_IO_ERR | G_IO_HUP,
                                           (GIOFunc)data_available,
                                           self);

    return !!self->priv->iochannel;
}

typedef struct {
    MbimDevice *self;
    GSimpleAsyncResult *result;
    GCancellable *cancellable;
    guint timeout;
} DeviceOpenContext;

static void
device_open_context_complete_and_free (DeviceOpenContext *ctx)
{
    g_simple_async_result_complete_in_idle (ctx->result);
    g_object_unref (ctx->result);
    if (ctx->cancellable)
        g_object_unref (ctx->cancellable);
    g_object_unref (ctx->self);
    g_slice_free (DeviceOpenContext, ctx);
}

/**
 * mbim_device_open_finish:
 * @self: a #MbimDevice.
 * @res: a #GAsyncResult.
 * @error: Return location for error or %NULL.
 *
 * Finishes an asynchronous open operation started with mbim_device_open().
 *
 * Returns: %TRUE if successful, %FALSE if @error is set.
 */
gboolean
mbim_device_open_finish (MbimDevice   *self,
                        GAsyncResult  *res,
                        GError       **error)
{
    return !g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (res), error);
}

static void open_message (DeviceOpenContext *ctx);

static void
open_message_ready (MbimDevice        *self,
                    GAsyncResult      *res,
                    DeviceOpenContext *ctx)
{
    MbimMessage *response;
    GError *error = NULL;

    response = mbim_device_command_finish (self, res, &error);
    if (!response) {
        /* Check if we should be retrying */
        if (g_error_matches (error, MBIM_CORE_ERROR, MBIM_CORE_ERROR_TIMEOUT)) {
            /* The timeout will tell us how many retries we should do */
            ctx->timeout--;
            if (ctx->timeout) {
                g_error_free (error);
                open_message (ctx);
                return;
            }

            /* No more seconds left in the timeout... return error */
        }

        g_simple_async_result_take_error (ctx->result, error);
    } else if (!mbim_message_open_done_get_result (response, &error))
        g_simple_async_result_take_error (ctx->result, error);
    else
        g_simple_async_result_set_op_res_gboolean (ctx->result, TRUE);

    if (response)
        mbim_message_unref (response);
    device_open_context_complete_and_free (ctx);
}

static void
open_message (DeviceOpenContext *ctx)
{
    MbimMessage *request;

    /* Launch 'Open' command */
    request = mbim_message_open_new (mbim_device_get_next_transaction_id (ctx->self),
                                     ctx->self->priv->max_control_transfer);
    mbim_device_command (ctx->self,
                         request,
                         1, /* 1s per retry */
                         ctx->cancellable,
                         (GAsyncReadyCallback)open_message_ready,
                         ctx);
    mbim_message_unref (request);
}

/**
 * mbim_device_open:
 * @self: a #MbimDevice.
 * @timeout: maximum time, in seconds, to wait for the device to be opened.
 * @cancellable: optional #GCancellable object, #NULL to ignore.
 * @callback: a #GAsyncReadyCallback to call when the operation is finished.
 * @user_data: the data to pass to callback function.
 *
 * Asynchronously opens a #MbimDevice for I/O.
 *
 * When the operation is finished @callback will be called. You can then call
 * mbim_device_open_finish() to get the result of the operation.
 */
void
mbim_device_open (MbimDevice          *self,
                  guint                timeout,
                  GCancellable        *cancellable,
                  GAsyncReadyCallback  callback,
                  gpointer             user_data)
{
    DeviceOpenContext *ctx;
    GError *error = NULL;

    g_return_if_fail (MBIM_IS_DEVICE (self));
    g_return_if_fail (timeout > 0);

    ctx = g_slice_new (DeviceOpenContext);
    ctx->self = g_object_ref (self);
    ctx->result = g_simple_async_result_new (G_OBJECT (self),
                                             callback,
                                             user_data,
                                             mbim_device_open);
    ctx->timeout = timeout;
    ctx->cancellable = (cancellable ? g_object_ref (cancellable) : NULL);

    if (!create_iochannel (self, &error)) {
        g_prefix_error (&error,
                        "Cannot open MBIM device: ");
        g_simple_async_result_take_error (ctx->result, error);
        device_open_context_complete_and_free (ctx);
        return;
    }

    /* If the device is already in-session, avoid the open message */
    if (self->priv->in_session) {
        g_simple_async_result_set_op_res_gboolean (ctx->result, TRUE);
        device_open_context_complete_and_free (ctx);
        return;
    }

    open_message (ctx);
}

/*****************************************************************************/
/* Close channel */

static gboolean
destroy_iochannel (MbimDevice  *self,
                   GError     **error)
{
    GError *inner_error = NULL;

    g_io_channel_shutdown (self->priv->iochannel, TRUE, &inner_error);

    /* Failures when closing still make the device to get closed */
    g_io_channel_unref (self->priv->iochannel);
    self->priv->iochannel = NULL;

    if (self->priv->watch_id) {
        g_source_remove (self->priv->watch_id);
        self->priv->watch_id = 0;
    }

    if (self->priv->response) {
        g_byte_array_unref (self->priv->response);
        self->priv->response = NULL;
    }

    if (inner_error) {
        g_propagate_error (error, inner_error);
        return FALSE;
    }

    return TRUE;
}

/**
 * mbim_device_close_force:
 * @self: a #MbimDevice.
 * @error: Return location for error or %NULL.
 *
 * Forces the #MbimDevice to be closed.
 *
 * Returns: %TRUE if @self if no error happens, otherwise %FALSE and @error is set.
 */
gboolean
mbim_device_close_force (MbimDevice *self,
                         GError **error)
{
    g_return_val_if_fail (MBIM_IS_DEVICE (self), FALSE);

    /* Already closed? */
    if (!self->priv->iochannel)
        return TRUE;

    return destroy_iochannel (self, error);
}

typedef struct {
    MbimDevice *self;
    GSimpleAsyncResult *result;
    GCancellable *cancellable;
    guint timeout;
} DeviceCloseContext;

static void
device_close_context_complete_and_free (DeviceCloseContext *ctx)
{
    g_simple_async_result_complete_in_idle (ctx->result);
    g_object_unref (ctx->result);
    if (ctx->cancellable)
        g_object_unref (ctx->cancellable);
    g_object_unref (ctx->self);
    g_slice_free (DeviceCloseContext, ctx);
}

/**
 * mbim_device_close_finish:
 * @self: a #MbimDevice.
 * @res: a #GAsyncResult.
 * @error: Return location for error or %NULL.
 *
 * Finishes an asynchronous close operation started with mbim_device_close().
 *
 * Returns: %TRUE if successful, %FALSE if @error is set.
 */
gboolean
mbim_device_close_finish (MbimDevice    *self,
                          GAsyncResult  *res,
                          GError       **error)
{
    return !g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (res), error);
}

static void
close_message_ready (MbimDevice         *self,
                     GAsyncResult       *res,
                     DeviceCloseContext *ctx)
{
    MbimMessage *response;
    GError *error = NULL;

    response = mbim_device_command_finish (self, res, &error);
    if (!response)
        g_simple_async_result_take_error (ctx->result, error);
    else if (!mbim_message_close_done_get_result (response, &error))
        g_simple_async_result_take_error (ctx->result, error);
    else if (!destroy_iochannel (self, &error))
        g_simple_async_result_take_error (ctx->result, error);
    else
        g_simple_async_result_set_op_res_gboolean (ctx->result, TRUE);

    if (response)
        mbim_message_unref (response);
    device_close_context_complete_and_free (ctx);
}

/**
 * mbim_device_close:
 * @self: a #MbimDevice.
 * @timeout: maximum time, in seconds, to wait for the device to be closed.
 * @cancellable: optional #GCancellable object, #NULL to ignore.
 * @callback: a #GAsyncReadyCallback to call when the operation is finished.
 * @user_data: the data to pass to callback function.
 *
 * Asynchronously closes a #MbimDevice for I/O.
 *
 * When the operation is finished @callback will be called. You can then call
 * mbim_device_close_finish() to get the result of the operation.
 */
void
mbim_device_close (MbimDevice          *self,
                   guint                timeout,
                   GCancellable        *cancellable,
                   GAsyncReadyCallback  callback,
                   gpointer             user_data)
{
    MbimMessage *request;
    DeviceCloseContext *ctx;

    g_return_if_fail (MBIM_IS_DEVICE (self));

    ctx = g_slice_new (DeviceCloseContext);
    ctx->self = g_object_ref (self);
    ctx->result = g_simple_async_result_new (G_OBJECT (self),
                                             callback,
                                             user_data,
                                             mbim_device_close);
    ctx->timeout = timeout;
    ctx->cancellable = (cancellable ? g_object_ref (cancellable) : NULL);

    /* Already closed? */
    if (!self->priv->iochannel) {
        g_simple_async_result_set_op_res_gboolean (ctx->result, TRUE);
        device_close_context_complete_and_free (ctx);
        return;
    }

    /* If the device is in-session, avoid the close message */
    if (self->priv->in_session) {
        GError *error = NULL;

        if (!destroy_iochannel (self, &error))
            g_simple_async_result_take_error (ctx->result, error);
        else
            g_simple_async_result_set_op_res_gboolean (ctx->result, TRUE);
        device_close_context_complete_and_free (ctx);
        return;
    }

    /* Launch 'Close' command */
    request = mbim_message_close_new (mbim_device_get_next_transaction_id (self));
    mbim_device_command (self,
                         request,
                         10,
                         ctx->cancellable,
                         (GAsyncReadyCallback) close_message_ready,
                         ctx);
    mbim_message_unref (request);
}

/*****************************************************************************/

/**
 * mbim_device_get_next_transaction_id:
 * @self: A #MbimDevice.
 *
 * Acquire the next transaction ID of this #MbimDevice.
 * The internal transaction ID gets incremented.
 *
 * Returns: the next transaction ID.
 */
guint32
mbim_device_get_next_transaction_id (MbimDevice *self)
{
    guint32 next;

    g_return_val_if_fail (MBIM_IS_DEVICE (self), 0);

    next = self->priv->transaction_id;

    /* Don't go further than 8bits in the CTL service */
    if (self->priv->transaction_id == G_MAXUINT32)
        /* Reset! */
        self->priv->transaction_id = 0x01;
    else
        self->priv->transaction_id++;

    return next;
}

/*****************************************************************************/

static gboolean
device_write (MbimDevice    *self,
              const guint8  *data,
              guint32        data_length,
              GError       **error)
{
    gsize written;
    GIOStatus write_status;

    written = 0;
    write_status = G_IO_STATUS_AGAIN;
    while (write_status == G_IO_STATUS_AGAIN) {
        write_status = g_io_channel_write_chars (self->priv->iochannel,
                                                 (gconstpointer)data,
                                                 (gssize)data_length,
                                                 &written,
                                                 error);
        switch (write_status) {
        case G_IO_STATUS_ERROR:
            g_prefix_error (error, "Cannot write message: ");
            return FALSE;

        case G_IO_STATUS_EOF:
            /* We shouldn't get EOF when writing */
            g_assert_not_reached ();
            break;

        case G_IO_STATUS_NORMAL:
            /* All good, we'll exit the loop now */
            break;

        case G_IO_STATUS_AGAIN:
            /* We're in a non-blocking channel and therefore we're up to receive
             * EAGAIN; just retry in this case. TODO: in an idle? */
            break;
        }
    }

    return TRUE;
}

static gboolean
device_send (MbimDevice   *self,
             MbimMessage  *message,
             GError      **error)
{
    const guint8 *raw_message;
    guint32 raw_message_len;
    struct fragment_info *fragments;
    guint n_fragments;
    guint i;

    raw_message = mbim_message_get_raw (message, &raw_message_len, NULL);
    g_assert (raw_message);

    if (mbim_utils_get_traces_enabled ()) {
        gchar *printable;

        printable = __mbim_utils_str_hex (raw_message, raw_message_len, ':');
        g_debug ("[%s] Sent message...\n"
                 "<<<<<< RAW:\n"
                 "<<<<<<   length = %u\n"
                 "<<<<<<   data   = %s\n",
                 self->priv->path_display,
                 ((GByteArray *)message)->len,
                 printable);
        g_free (printable);

        printable = mbim_message_get_printable (message, "<<<<<< ", FALSE);
        g_debug ("[%s] Sent message (translated)...\n%s",
                 self->priv->path_display,
                 printable);
        g_free (printable);
    }

    /* Single fragment? Send it! */
    if (raw_message_len <= MAX_CONTROL_TRANSFER)
        return device_write (self, raw_message, raw_message_len, error);

    /* The message to send must be able to handle fragments */
    g_assert (_mbim_message_is_fragment (message));

    fragments = _mbim_message_split_fragments (message,
                                               MAX_CONTROL_TRANSFER,
                                               &n_fragments);
    for (i = 0; i < n_fragments; i++) {
        if (mbim_utils_get_traces_enabled ()) {
            GByteArray *bytearray;
            gchar *printable;
            gchar *printable_h;
            gchar *printable_fh;
            gchar *printable_d;

            printable_h  = __mbim_utils_str_hex (&fragments[i].header, sizeof (fragments[i].header), ':');
            printable_fh = __mbim_utils_str_hex (&fragments[i].fragment_header, sizeof (fragments[i].fragment_header), ':');
            printable_d  = __mbim_utils_str_hex (fragments[i].data, fragments[i].data_length, ':');
            g_debug ("[%s] Sent fragment (%u)...\n"
                     "<<<<<< RAW:\n"
                     "<<<<<<   length = %u\n"
                     "<<<<<<   data   = %s%s%s\n",
                     self->priv->path_display, i,
                     (guint)(sizeof (fragments[i].header) +
                             sizeof (fragments[i].fragment_header) +
                             fragments[i].data_length),
                     printable_h, printable_fh, printable_d);
            g_free (printable_h);
            g_free (printable_fh);
            g_free (printable_d);

            /* Dummy message for printable purposes only */
            bytearray = g_byte_array_new ();
            g_byte_array_append (bytearray, (guint8 *)&fragments[i].header, sizeof (fragments[i].header));
            g_byte_array_append (bytearray, (guint8 *)&fragments[i].fragment_header, sizeof (fragments[i].fragment_header));
            printable = mbim_message_get_printable ((MbimMessage *)bytearray, "<<<<<< ", TRUE);
            g_debug ("[%s] Sent fragment (translated)...\n%s",
                     self->priv->path_display,
                     printable);
            g_free (printable);
            g_byte_array_unref (bytearray);
        }

        /* Write fragment headers */
        if (!device_write (self,
                           (guint8 *)&fragments[i].header,
                           sizeof (fragments[i].header),
                           error))
            return FALSE;

        if (!device_write (self,
                           (guint8 *)&fragments[i].fragment_header,
                           sizeof (fragments[i].fragment_header),
                           error))
            return FALSE;

        /* Write fragment data */
        if (!device_write (self,
                           fragments[i].data,
                           fragments[i].data_length,
                           error))
            return FALSE;
    }
    g_free (fragments);

    return TRUE;
}

/*****************************************************************************/
/* Report error */

typedef struct {
    MbimDevice  *self;
    MbimMessage *message;
} ReportErrorContext;

static void
device_report_error_context_free (ReportErrorContext *ctx)
{
    mbim_message_unref (ctx->message);
    g_object_unref (ctx->self);
    g_slice_free (ReportErrorContext, ctx);
}

static gboolean
device_report_error_in_idle (ReportErrorContext *ctx)
{
    /* Device must be open */
    if (ctx->self->priv->iochannel) {
        GError *error = NULL;

        if (!device_send (ctx->self, ctx->message, &error)) {
            g_warning ("[%s] Couldn't send host error message: %s",
                       ctx->self->priv->path_display,
                       error->message);
            g_error_free (error);
        }
    }

    device_report_error_context_free (ctx);
    return FALSE;
}

static void
device_report_error (MbimDevice   *self,
                     guint32       transaction_id,
                     const GError *error)
{
    ReportErrorContext *ctx;

    /* Only protocol errors to be reported to the modem */
    if (error->domain != MBIM_PROTOCOL_ERROR)
        return;

    ctx = g_slice_new (ReportErrorContext);
    ctx->self = g_object_ref (self);
    ctx->message = mbim_message_error_new (transaction_id, error->code);

    g_idle_add ((GSourceFunc) device_report_error_in_idle, ctx);
}

/*****************************************************************************/
/* Command */

/**
 * mbim_device_command_finish:
 * @self: a #MbimDevice.
 * @res: a #GAsyncResult.
 * @error: Return location for error or %NULL.
 *
 * Finishes an operation started with mbim_device_command().
 *
 * Returns: a #MbimMessage response, or #NULL if @error is set. The returned value should be freed with mbim_message_unref().
 */
MbimMessage *
mbim_device_command_finish (MbimDevice    *self,
                            GAsyncResult  *res,
                            GError       **error)
{
    if (g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (res), error))
        return NULL;

    return mbim_message_ref (g_simple_async_result_get_op_res_gpointer (
                                 G_SIMPLE_ASYNC_RESULT (res)));
}

/**
 * mbim_device_command:
 * @self: a #MbimDevice.
 * @message: the message to send.
 * @timeout: maximum time, in seconds, to wait for the response.
 * @cancellable: a #GCancellable, or %NULL.
 * @callback: a #GAsyncReadyCallback to call when the operation is finished.
 * @user_data: the data to pass to callback function.
 *
 * Asynchronously sends a #MbimMessage to the device.
 *
 * When the operation is finished @callback will be called. You can then call
 * mbim_device_command_finish() to get the result of the operation.
 */
void
mbim_device_command (MbimDevice          *self,
                     MbimMessage         *message,
                     guint                timeout,
                     GCancellable        *cancellable,
                     GAsyncReadyCallback  callback,
                     gpointer             user_data)
{
    GError *error = NULL;
    Transaction *tr;
    guint32 transaction_id;

    g_return_if_fail (MBIM_IS_DEVICE (self));
    g_return_if_fail (message != NULL);

    /* If the message comes without a explicit transaction ID, add one
     * ourselves */
    transaction_id = mbim_message_get_transaction_id (message);
    if (!transaction_id) {
        transaction_id = mbim_device_get_next_transaction_id (self);
        mbim_message_set_transaction_id (message, transaction_id);
    }

    tr = transaction_new (self,
                          transaction_id,
                          cancellable,
                          callback,
                          user_data);

    /* Device must be open */
    if (!self->priv->iochannel) {
        error = g_error_new (MBIM_CORE_ERROR,
                             MBIM_CORE_ERROR_WRONG_STATE,
                             "Device must be open to send commands");
        transaction_complete_and_free (tr, error);
        g_error_free (error);
        return;
    }

    /* Setup context to match response */
    if (!device_store_transaction (self, TRANSACTION_TYPE_HOST, tr, timeout * 1000, &error)) {
        g_prefix_error (&error, "Cannot store transaction: ");
        transaction_complete_and_free (tr, error);
        g_error_free (error);
        return;
    }

    if (!device_send (self, message, &error)) {
        /* Match transaction so that we remove it from our tracking table */
        tr = device_match_transaction (self, TRANSACTION_TYPE_HOST, message);
        transaction_complete_and_free (tr, error);
        g_error_free (error);
        return;
    }

    /* Just return, we'll get response asynchronously */
}

/*****************************************************************************/
/* New MBIM device */

/**
 * mbim_device_new_finish:
 * @res: a #GAsyncResult.
 * @error: Return location for error or %NULL.
 *
 * Finishes an operation started with mbim_device_new().
 *
 * Returns: A newly created #MbimDevice, or #NULL if @error is set.
 */
MbimDevice *
mbim_device_new_finish (GAsyncResult  *res,
                        GError       **error)
{
  GObject *ret;
  GObject *source_object;

  source_object = g_async_result_get_source_object (res);
  ret = g_async_initable_new_finish (G_ASYNC_INITABLE (source_object), res, error);
  g_object_unref (source_object);

  return (ret ? MBIM_DEVICE (ret) : NULL);
}

/**
 * mbim_device_new:
 * @file: a #GFile.
 * @cancellable: optional #GCancellable object, #NULL to ignore.
 * @callback: a #GAsyncReadyCallback to call when the initialization is finished.
 * @user_data: the data to pass to callback function.
 *
 * Asynchronously creates a #MbimDevice object to manage @file.
 * When the operation is finished, @callback will be invoked. You can then call
 * mbim_device_new_finish() to get the result of the operation.
 */
void
mbim_device_new (GFile               *file,
                 GCancellable        *cancellable,
                 GAsyncReadyCallback  callback,
                 gpointer             user_data)
{
    g_async_initable_new_async (MBIM_TYPE_DEVICE,
                                G_PRIORITY_DEFAULT,
                                cancellable,
                                callback,
                                user_data,
                                MBIM_DEVICE_FILE, file,
                                NULL);
}

/*****************************************************************************/
/* Async init */

typedef struct {
    MbimDevice *self;
    GSimpleAsyncResult *result;
    GCancellable *cancellable;
} InitContext;

static void
init_context_complete_and_free (InitContext *ctx)
{
    g_simple_async_result_complete_in_idle (ctx->result);
    if (ctx->cancellable)
        g_object_unref (ctx->cancellable);
    g_object_unref (ctx->result);
    g_object_unref (ctx->self);
    g_slice_free (InitContext, ctx);
}

static gboolean
initable_init_finish (GAsyncInitable  *initable,
                      GAsyncResult    *result,
                      GError         **error)
{
    return !g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (result), error);
}

static void
query_info_async_ready (GFile *file,
                        GAsyncResult *res,
                        InitContext *ctx)
{
    GError *error = NULL;
    GFileInfo *info;

    info = g_file_query_info_finish (file, res, &error);
    if (!info) {
        g_prefix_error (&error,
                        "Couldn't query file info: ");
        g_simple_async_result_take_error (ctx->result, error);
        init_context_complete_and_free (ctx);
        return;
    }

    /* Our MBIM device must be of SPECIAL type */
    if (g_file_info_get_file_type (info) != G_FILE_TYPE_SPECIAL) {
        g_simple_async_result_set_error (ctx->result,
                                         MBIM_CORE_ERROR,
                                         MBIM_CORE_ERROR_FAILED,
                                         "Wrong file type");
        init_context_complete_and_free (ctx);
        return;
    }
    g_object_unref (info);

    /* Done we are */
    g_simple_async_result_set_op_res_gboolean (ctx->result, TRUE);
    init_context_complete_and_free (ctx);
}

static void
initable_init_async (GAsyncInitable      *initable,
                     int                  io_priority,
                     GCancellable        *cancellable,
                     GAsyncReadyCallback  callback,
                     gpointer             user_data)
{
    InitContext *ctx;

    ctx = g_slice_new0 (InitContext);
    ctx->self = g_object_ref (initable);
    if (cancellable)
        ctx->cancellable = g_object_ref (cancellable);
    ctx->result = g_simple_async_result_new (G_OBJECT (initable),
                                             callback,
                                             user_data,
                                             initable_init_async);

    /* We need a proper file to initialize */
    if (!ctx->self->priv->file) {
        g_simple_async_result_set_error (ctx->result,
                                         MBIM_CORE_ERROR,
                                         MBIM_CORE_ERROR_INVALID_ARGS,
                                         "Cannot initialize MBIM device: No file given");
        init_context_complete_and_free (ctx);
        return;
    }

    /* Check the file type. Note that this is just a quick check to avoid
     * creating MbimDevices pointing to a location already known not to be a MBIM
     * device. */
    g_file_query_info_async (ctx->self->priv->file,
                             G_FILE_ATTRIBUTE_STANDARD_TYPE,
                             G_FILE_QUERY_INFO_NONE,
                             G_PRIORITY_DEFAULT,
                             ctx->cancellable,
                             (GAsyncReadyCallback)query_info_async_ready,
                             ctx);
}

/*****************************************************************************/

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    MbimDevice *self = MBIM_DEVICE (object);

    switch (prop_id) {
    case PROP_FILE:
        g_assert (self->priv->file == NULL);
        self->priv->file = g_value_dup_object (value);
        self->priv->path = g_file_get_path (self->priv->file);
        self->priv->path_display = g_filename_display_name (self->priv->path);
        break;
    case PROP_TRANSACTION_ID:
        self->priv->transaction_id = g_value_get_uint (value);
        break;
    case PROP_IN_SESSION:
        self->priv->in_session = g_value_get_boolean (value);
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
    MbimDevice *self = MBIM_DEVICE (object);

    switch (prop_id) {
    case PROP_FILE:
        g_value_set_object (value, self->priv->file);
        break;
    case PROP_TRANSACTION_ID:
        g_value_set_uint (value, self->priv->transaction_id);
        break;
    case PROP_IN_SESSION:
        g_value_set_boolean (value, self->priv->in_session);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
mbim_device_init (MbimDevice *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE ((self),
                                              MBIM_TYPE_DEVICE,
                                              MbimDevicePrivate);

    /* Initialize transaction ID */
    self->priv->transaction_id = 0x01;
}

static void
dispose (GObject *object)
{
    MbimDevice *self = MBIM_DEVICE (object);

    g_clear_object (&self->priv->file);

    G_OBJECT_CLASS (mbim_device_parent_class)->dispose (object);
}

static void
finalize (GObject *object)
{
    MbimDevice *self = MBIM_DEVICE (object);
    guint i;

    /* Transactions keep refs to the device, so it's actually
     * impossible to have any content in the HT */
    for (i = 0; i < TRANSACTION_TYPE_LAST; i++) {
        if (self->priv->transactions[i]) {
            g_assert (g_hash_table_size (self->priv->transactions[i]) == 0);
            g_hash_table_unref (self->priv->transactions[i]);
        }
    }

    g_free (self->priv->path);
    g_free (self->priv->path_display);
    if (self->priv->watch_id)
        g_source_remove (self->priv->watch_id);
    if (self->priv->response)
        g_byte_array_unref (self->priv->response);
    if (self->priv->iochannel)
        g_io_channel_unref (self->priv->iochannel);

    G_OBJECT_CLASS (mbim_device_parent_class)->finalize (object);
}

static void
async_initable_iface_init (GAsyncInitableIface *iface)
{
    iface->init_async = initable_init_async;
    iface->init_finish = initable_init_finish;
}

static void
mbim_device_class_init (MbimDeviceClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (object_class, sizeof (MbimDevicePrivate));

    object_class->get_property = get_property;
    object_class->set_property = set_property;
    object_class->finalize = finalize;
    object_class->dispose = dispose;

    properties[PROP_FILE] =
        g_param_spec_object (MBIM_DEVICE_FILE,
                             "Device file",
                             "File to the underlying MBIM device",
                             G_TYPE_FILE,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    g_object_class_install_property (object_class, PROP_FILE, properties[PROP_FILE]);

    properties[PROP_TRANSACTION_ID] =
        g_param_spec_uint (MBIM_DEVICE_TRANSACTION_ID,
                           "Transaction ID",
                           "Current transaction ID",
                           0x01,
                           G_MAXUINT32,
                           0x01,
                           G_PARAM_READWRITE);
    g_object_class_install_property (object_class, PROP_TRANSACTION_ID, properties[PROP_TRANSACTION_ID]);

    properties[PROP_IN_SESSION] =
        g_param_spec_boolean (MBIM_DEVICE_IN_SESSION,
                              "In session",
                              "Flag to specify if the device is within a session",
                              FALSE,
                              G_PARAM_READWRITE);
    g_object_class_install_property (object_class, PROP_IN_SESSION, properties[PROP_IN_SESSION]);

  /**
   * MbimDevice::device-indicate-status:
   * @self: the #MbimDevice
   * @message: the #MbimMessage indication
   *
   * The ::device-indication-status signal is emitted when a MBIM indication is received.
   */
    signals[SIGNAL_INDICATE_STATUS] =
        g_signal_new (MBIM_DEVICE_SIGNAL_INDICATE_STATUS,
                      G_OBJECT_CLASS_TYPE (G_OBJECT_CLASS (klass)),
                      G_SIGNAL_RUN_LAST,
                      0,
                      NULL,
                      NULL,
                      NULL,
                      G_TYPE_NONE,
                      1,
                      MBIM_TYPE_MESSAGE);

  /**
   * MbimDevice::device-error:
   * @self: the #MbimDevice
   * @message: the #MbimMessage error
   *
   * The ::device-error signal is emitted when a MBIM error is received.
   */
    signals[SIGNAL_ERROR] =
        g_signal_new (MBIM_DEVICE_SIGNAL_ERROR,
                      G_OBJECT_CLASS_TYPE (G_OBJECT_CLASS (klass)),
                      G_SIGNAL_RUN_LAST,
                      0,
                      NULL,
                      NULL,
                      NULL,
                      G_TYPE_NONE,
                      1,
                      G_TYPE_ERROR);
}
