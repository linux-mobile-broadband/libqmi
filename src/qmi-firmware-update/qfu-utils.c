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
 * Copyright (C) 2016 Zodiac Inflight Innovations
 * Copyright (C) 2016-2017 Aleksander Morgado <aleksander@aleksander.es>
 *
 * crc16 and HDLC escape code borrowed from modemmanager/libqcdm
 * Copyright (C) 2010 Red Hat, Inc.
 */

#include <stdio.h>
#include <string.h>

#include <glib.h>
#include <gio/gio.h>

#include "qfu-utils.h"

/******************************************************************************/

gchar *
qfu_utils_str_hex (gconstpointer mem,
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

gchar *
qfu_utils_get_firmware_image_unique_id_printable (const GArray *unique_id)
{
    gchar    *unique_id_str;
    guint     i;
    guint     n_ascii = 0;
    gboolean  end = FALSE;

#define UNIQUE_ID_LEN 16

    g_warn_if_fail (unique_id->len <= UNIQUE_ID_LEN);
    unique_id_str = g_malloc0 (UNIQUE_ID_LEN + 1);
    memcpy (unique_id_str, unique_id->data, UNIQUE_ID_LEN);

    /* We want an ASCII string that, if finished before the 16 bytes,
     * is suffixed with NUL bytes. */
    for (i = 0; i < UNIQUE_ID_LEN; i++) {
        /* If a byte isn't ASCII, stop */
        if (unique_id_str[i] & 0x80)
            break;
        /* If string isn't finished yet... */
        if (!end) {
            /* String finished now */
            if (unique_id_str[i] == '\0')
                end = TRUE;
            else
                n_ascii++;
        } else {
            /* String finished but we then got
             * another ASCII byte? not possible */
            if (unique_id_str[i] != '\0')
                break;
        }
    }

    if (i == UNIQUE_ID_LEN && n_ascii > 0)
        return unique_id_str;

#undef UNIQUE_ID_LEN

    g_free (unique_id_str);

    /* Get a raw hex string otherwise */
    unique_id_str = qfu_utils_str_hex (unique_id->data, unique_id->len, ':');

    return unique_id_str;
}

/******************************************************************************/

/* Table of CRCs for each possible byte, with a generator polynomial of 0x8408 */
static const guint16 crc_table[256] = {
    0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
    0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
    0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
    0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
    0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,
    0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
    0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
    0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,
    0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
    0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
    0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,
    0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
    0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9,
    0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
    0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
    0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
    0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7,
    0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
    0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,
    0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
    0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
    0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
    0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,
    0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
    0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3,
    0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
    0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
    0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
    0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1,
    0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
    0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,
    0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78
};

/* Calculate the CRC for a buffer using a seed of 0xffff */
guint16
qfu_utils_crc16 (const guint8 *buffer,
                 gsize         len)
{
    guint16 crc = 0xffff;

    while (len--)
            crc = crc_table[(crc ^ *buffer++) & 0xff] ^ (crc >> 8);
    return ~crc;
}

/******************************************************************************/

gboolean
qfu_utils_parse_cwe_version_string (const gchar  *version,
                                    gchar       **firmware_version,
                                    gchar       **config_version,
                                    gchar       **carrier,
                                    GError      **error)
{
    GRegex     *regex;
    GMatchInfo *match_info = NULL;
    gboolean    result = FALSE;

    regex = g_regex_new ("(?:.*)"
                         "_([0-9][0-9]\\.[0-9][0-9]\\.[0-9][0-9]\\.[0-9][0-9])" /* firmware version */
                         "(?:"
                             "(?:.*)"
                             "_([a-zA-Z\\-]+)" /* carrier */
                             "_([0-9][0-9][0-9]\\.[0-9][0-9][0-9]_[0-9][0-9][0-9])" /* config version */
                         ")?",
                         0, 0, NULL);
    g_assert (regex);

    if (!g_regex_match_full (regex, version, strlen (version), 0, 0, &match_info, NULL)) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "couldn't parse CWE version string '%s': didn't match",
                     version);
        goto out;
    }

    if (firmware_version)
        *firmware_version = g_match_info_fetch (match_info, 1);
    if (carrier)
        *carrier = (g_match_info_get_match_count (match_info) > 2 ? g_match_info_fetch (match_info, 2) : NULL);
    if (config_version)
        *config_version = (g_match_info_get_match_count (match_info) > 3 ? g_match_info_fetch (match_info, 3) : NULL);

    result = TRUE;

out:
    g_match_info_free (match_info);
    g_regex_unref (regex);

    return result;
}

/******************************************************************************/

typedef struct {
    QmiDevice          *qmi_device;
    QmiDeviceOpenFlags  device_open_flags;
    gint                qmi_client_retries;
    QmiClientDms       *qmi_client;

    gboolean                                  load_capabilities;
    gchar                                    *revision;
    gboolean                                  revision_done;
    gboolean                                  supports_stored_image_management;
    gboolean                                  supports_stored_image_management_done;
    guint8                                    max_storage_index;
    gboolean                                  supports_firmware_preference_management;
    QmiMessageDmsGetFirmwarePreferenceOutput *firmware_preference;
    gboolean                                  supports_firmware_preference_management_done;
    QmiMessageDmsSwiGetCurrentFirmwareOutput *current_firmware;
    gboolean                                  supports_current_firmware_done;
} NewClientDmsContext;

static void
new_client_dms_context_free (NewClientDmsContext *ctx)
{
    if (ctx->current_firmware)
        qmi_message_dms_swi_get_current_firmware_output_unref (ctx->current_firmware);
    if (ctx->firmware_preference)
        qmi_message_dms_get_firmware_preference_output_unref (ctx->firmware_preference);
    if (ctx->qmi_client)
        g_object_unref (ctx->qmi_client);
    if (ctx->qmi_device)
        g_object_unref (ctx->qmi_device);
    g_free (ctx->revision);
    g_slice_free (NewClientDmsContext, ctx);
}

gboolean
qfu_utils_new_client_dms_finish (GAsyncResult  *res,
                                 QmiDevice    **qmi_device,
                                 QmiClientDms **qmi_client,
                                 gchar        **revision,
                                 gboolean      *supports_stored_image_management,
                                 guint8        *max_storage_index,
                                 gboolean      *supports_firmware_preference_management,
                                 QmiMessageDmsGetFirmwarePreferenceOutput **firmware_preference,
                                 QmiMessageDmsSwiGetCurrentFirmwareOutput **current_firmware,
                                 GError       **error)
{
    NewClientDmsContext *ctx;

    if (!g_task_propagate_boolean (G_TASK (res), error))
        return FALSE;

    ctx = g_task_get_task_data (G_TASK (res));
    if (qmi_device)
        *qmi_device = g_object_ref (ctx->qmi_device);
    if (qmi_client)
        *qmi_client = g_object_ref (ctx->qmi_client);
    if (revision)
        *revision = (ctx->revision ? g_strdup (ctx->revision) : NULL);
    if (supports_stored_image_management)
        *supports_stored_image_management = ctx->supports_stored_image_management;
    if (max_storage_index)
        *max_storage_index = ctx->max_storage_index;
    if (supports_firmware_preference_management)
        *supports_firmware_preference_management = ctx->supports_firmware_preference_management;
    if (firmware_preference)
        *firmware_preference = (ctx->firmware_preference ? qmi_message_dms_get_firmware_preference_output_ref (ctx->firmware_preference) : NULL);
    if (current_firmware)
        *current_firmware = (ctx->current_firmware ? qmi_message_dms_swi_get_current_firmware_output_ref (ctx->current_firmware) : NULL);
    return TRUE;
}

static void
check_capabilities_done (GTask *task)
{
    NewClientDmsContext *ctx;

    ctx = g_task_get_task_data (task);

    if (!ctx->revision_done ||
        !ctx->supports_stored_image_management_done ||
        !ctx->supports_firmware_preference_management_done ||
        !ctx->supports_current_firmware_done)
        return;

    /* All done! */
    g_task_return_boolean (task, TRUE);
    g_object_unref (task);
}

static void
dms_get_revision_ready (QmiClientDms *client,
                        GAsyncResult *res,
                        GTask        *task)
{
    QmiMessageDmsGetRevisionOutput *output;
    const gchar                    *str = NULL;
    NewClientDmsContext            *ctx;

    ctx = g_task_get_task_data (task);

    output = qmi_client_dms_get_revision_finish (client, res, NULL);
    if (output && qmi_message_dms_get_revision_output_get_result (output, NULL))
        qmi_message_dms_get_revision_output_get_revision (output, &str, NULL);

    if (str) {
        g_debug ("[qfu,utils] current revision loaded: %s", str);
        ctx->revision = g_strdup (str);
    }

    if (output)
        qmi_message_dms_get_revision_output_unref (output);

    ctx->revision_done = TRUE;
    check_capabilities_done (task);
}

static void
dms_list_stored_images_ready (QmiClientDms *client,
                              GAsyncResult *res,
                              GTask        *task)
{
    QmiMessageDmsListStoredImagesOutput *output;
    NewClientDmsContext                 *ctx;

    ctx = g_task_get_task_data (task);

    output = qmi_client_dms_list_stored_images_finish (client, res, NULL);
    ctx->supports_stored_image_management = (output && qmi_message_dms_list_stored_images_output_get_result (output, NULL));

    if (ctx->supports_stored_image_management) {
        GArray *array;
        guint i;

        qmi_message_dms_list_stored_images_output_get_list (output, &array, NULL);

        for (i = 0; i < array->len; i++) {
            QmiMessageDmsListStoredImagesOutputListImage *image;

            image = &g_array_index (array, QmiMessageDmsListStoredImagesOutputListImage, i);
            if (image->type == QMI_DMS_FIRMWARE_IMAGE_TYPE_MODEM) {
                ctx->max_storage_index = image->maximum_images;
                break;
            }
        }
    }

    if (output)
        qmi_message_dms_list_stored_images_output_unref (output);

    ctx->supports_stored_image_management_done = TRUE;
    check_capabilities_done (task);
}

static void
dms_get_firmware_preference_ready (QmiClientDms *client,
                                   GAsyncResult *res,
                                   GTask        *task)
{
    QmiMessageDmsGetFirmwarePreferenceOutput *output;
    NewClientDmsContext                      *ctx;

    ctx = g_task_get_task_data (task);

    output = qmi_client_dms_get_firmware_preference_finish (client, res, NULL);
    ctx->supports_firmware_preference_management = (output && qmi_message_dms_get_firmware_preference_output_get_result (output, NULL));

    /* Log the current firmware preference */
    if (ctx->supports_firmware_preference_management) {
        GArray *array;
        guint   i;

        g_debug ("[qfu,utils] current firmware preference loaded:");

        /* Store */
        ctx->firmware_preference = qmi_message_dms_get_firmware_preference_output_ref (output);

        qmi_message_dms_get_firmware_preference_output_get_list (output, &array, NULL);

        if (array->len > 0) {
            for (i = 0; i < array->len; i++) {
                QmiMessageDmsGetFirmwarePreferenceOutputListImage *image;
                gchar                                             *unique_id_str;

                image = &g_array_index (array, QmiMessageDmsGetFirmwarePreferenceOutputListImage, i);
                unique_id_str = qfu_utils_get_firmware_image_unique_id_printable (image->unique_id);

                g_debug ("[qfu,utils] [image %u]",         i);
                g_debug ("[qfu,utils] \tImage type: '%s'", qmi_dms_firmware_image_type_get_string (image->type));
                g_debug ("[qfu,utils] \tUnique ID:  '%s'", unique_id_str);
                g_debug ("[qfu,utils] \tBuild ID:   '%s'", image->build_id);

                g_free (unique_id_str);
            }
        } else
            g_debug ("[qfu,utils] no images specified");
    }

    if (output)
        qmi_message_dms_get_firmware_preference_output_unref (output);

    ctx->supports_firmware_preference_management_done = TRUE;
    check_capabilities_done (task);
}

static void
swi_get_current_firmware_ready (QmiClientDms *client,
                                GAsyncResult *res,
                                GTask        *task)
{
    QmiMessageDmsSwiGetCurrentFirmwareOutput *output;
    NewClientDmsContext                      *ctx;

    ctx = g_task_get_task_data (task);

    output = qmi_client_dms_swi_get_current_firmware_finish (client, res, NULL);
    if (output) {
        if (qmi_message_dms_swi_get_current_firmware_output_get_result (output, NULL))
            ctx->current_firmware = output;
        else
            qmi_message_dms_swi_get_current_firmware_output_unref (output);
    }

    ctx->supports_current_firmware_done = TRUE;
    check_capabilities_done (task);
}

static void retry_allocate_qmi_client (GTask *task);

static void
qmi_client_ready (QmiDevice    *device,
                  GAsyncResult *res,
                  GTask        *task)
{
    NewClientDmsContext *ctx;
    GError              *error = NULL;

    ctx = (NewClientDmsContext *) g_task_get_task_data (task);

    ctx->qmi_client = QMI_CLIENT_DMS (qmi_device_allocate_client_finish (device, res, &error));
    if (!ctx->qmi_client) {
        /* If this was the last attempt, error out */
        if (!ctx->qmi_client_retries) {
            g_prefix_error (&error, "couldn't allocate DMS QMI client: ");
            g_task_return_error (task, error);
            g_object_unref (task);
            return;
        }

        /* Retry allocation */
        g_debug ("[qfu,utils] DMS QMI client allocation failed: %s", error->message);
        g_error_free (error);

        g_debug ("[qfu,utils] retrying...");
        retry_allocate_qmi_client (task);
        return;
    }

    g_debug ("[qfu,utils] DMS QMI client allocated");

    /* If loading capabilities not required, we're done */
    if (!ctx->load_capabilities) {
        g_task_return_boolean (task, TRUE);
        g_object_unref (task);
        return;
    }

    /* Query capabilities */
    qmi_client_dms_get_revision (ctx->qmi_client,
                                 NULL, 10, g_task_get_cancellable (task),
                                 (GAsyncReadyCallback) dms_get_revision_ready, task);
    qmi_client_dms_list_stored_images (ctx->qmi_client,
                                       NULL, 10, g_task_get_cancellable (task),
                                       (GAsyncReadyCallback) dms_list_stored_images_ready, task);
    qmi_client_dms_get_firmware_preference (ctx->qmi_client,
                                            NULL, 10, g_task_get_cancellable (task),
                                            (GAsyncReadyCallback) dms_get_firmware_preference_ready, task);
    qmi_client_dms_swi_get_current_firmware (ctx->qmi_client,
                                             NULL, 10, g_task_get_cancellable (task),
                                             (GAsyncReadyCallback) swi_get_current_firmware_ready, task);
}

static void
retry_allocate_qmi_client (GTask *task)
{
    NewClientDmsContext *ctx;

    ctx = (NewClientDmsContext *) g_task_get_task_data (task);

    /* Consume one retry attempt */
    ctx->qmi_client_retries--;
    g_assert (ctx->qmi_client_retries >= 0);

    g_debug ("[qfu,utils] allocating new DMS QMI client...");
    qmi_device_allocate_client (ctx->qmi_device,
                                QMI_SERVICE_DMS,
                                QMI_CID_NONE,
                                10,
                                g_task_get_cancellable (task),
                                (GAsyncReadyCallback) qmi_client_ready,
                                task);
}

static void
qmi_device_open_ready (QmiDevice    *device,
                       GAsyncResult *res,
                       GTask        *task)
{
    GError *error = NULL;

    if (!qmi_device_open_finish (device, res, &error)) {
        g_prefix_error (&error, "couldn't open QMI device: ");
        g_task_return_error (task, error);
        g_object_unref (task);
        return;
    }

    g_debug ("[qfu,utils] QMI device open");
    retry_allocate_qmi_client (task);
}

static void
qmi_device_ready (GObject      *source,
                  GAsyncResult *res,
                  GTask        *task)
{
    NewClientDmsContext *ctx;
    GError              *error = NULL;

    ctx = (NewClientDmsContext *) g_task_get_task_data (task);

    ctx->qmi_device = qmi_device_new_finish (res, &error);
    if (!ctx->qmi_device) {
        g_prefix_error (&error, "couldn't create QMI device: ");
        g_task_return_error (task, error);
        g_object_unref (task);
        return;
    }

    g_debug ("[qfu,utils] QMI device created");

    g_debug ("[qfu,utils] opening QMI device (%s proxy, %s mode)...",
             (ctx->device_open_flags & QMI_DEVICE_OPEN_FLAGS_PROXY) ? "with" : "without",
             (ctx->device_open_flags & QMI_DEVICE_OPEN_FLAGS_MBIM) ? "mbim" : "qmi");

    qmi_device_open (ctx->qmi_device,
                     ctx->device_open_flags | QMI_DEVICE_OPEN_FLAGS_SYNC,
                     20,
                     g_task_get_cancellable (task),
                     (GAsyncReadyCallback) qmi_device_open_ready,
                     task);
}

void
qfu_utils_new_client_dms (GFile               *cdc_wdm_file,
                          guint                retries,
                          QmiDeviceOpenFlags   device_open_flags,
                          gboolean             load_capabilities,
                          GCancellable        *cancellable,
                          GAsyncReadyCallback  callback,
                          gpointer             user_data)
{
    GTask               *task;
    NewClientDmsContext *ctx;

    ctx = g_slice_new0 (NewClientDmsContext);
    ctx->qmi_client_retries = retries;
    ctx->device_open_flags  = device_open_flags;
    ctx->load_capabilities  = load_capabilities;

    task = g_task_new (NULL, cancellable, callback, user_data);
    g_task_set_task_data (task, ctx, (GDestroyNotify) new_client_dms_context_free);

    g_debug ("[qfu,utils] creating QMI device...");
    qmi_device_new (cdc_wdm_file,
                    g_task_get_cancellable (task),
                    (GAsyncReadyCallback) qmi_device_ready,
                    task);
}

/******************************************************************************/

typedef enum {
    POWER_CYCLE_STEP_OFFLINE,
    POWER_CYCLE_STEP_RESET,
    POWER_CYCLE_STEP_DONE,
} PowerCycleStep;

typedef struct {
    QmiClientDms   *qmi_client;
    PowerCycleStep  step;
} PowerCycleContext;

static void
power_cycle_context_free (PowerCycleContext *ctx)
{
    g_object_unref (ctx->qmi_client);
    g_slice_free (PowerCycleContext, ctx);
}

gboolean
qfu_utils_power_cycle_finish (QmiClientDms  *qmi_client,
                              GAsyncResult  *res,
                              GError       **error)
{
    return g_task_propagate_boolean (G_TASK (res), error);
}

static void power_cycle_step (GTask *task);

static void
power_cycle_step_ready (QmiClientDms *qmi_client,
                        GAsyncResult *res,
                        GTask        *task)
{
    QmiMessageDmsSetOperatingModeOutput *output;
    GError                              *error = NULL;
    PowerCycleContext                   *ctx;

    ctx = (PowerCycleContext *) g_task_get_task_data (task);

    output = qmi_client_dms_set_operating_mode_finish (qmi_client, res, &error);
    if (!output) {
        g_prefix_error (&error, "QMI operation failed: couldn't set operating mode: ");
        g_task_return_error (task, error);
        g_object_unref (task);
        return;
    }

    if (!qmi_message_dms_set_operating_mode_output_get_result (output, &error)) {
        g_prefix_error (&error, "couldn't set operating mode: ");
        g_task_return_error (task, error);
        g_object_unref (task);
        qmi_message_dms_set_operating_mode_output_unref (output);
        return;
    }

    g_debug ("[qfu,utils] operating mode set successfully...");
    qmi_message_dms_set_operating_mode_output_unref (output);

    /* Go on */
    ctx->step++;
    power_cycle_step (task);
}

static void
power_cycle_step (GTask *task)
{
    QmiMessageDmsSetOperatingModeInput *input;
    QmiDmsOperatingMode                 mode;
    PowerCycleContext                  *ctx;

    ctx = (PowerCycleContext *) g_task_get_task_data (task);

    switch (ctx->step) {
    case POWER_CYCLE_STEP_OFFLINE:
        mode = QMI_DMS_OPERATING_MODE_OFFLINE;
        break;
    case POWER_CYCLE_STEP_RESET:
        mode = QMI_DMS_OPERATING_MODE_RESET;
        break;
    case POWER_CYCLE_STEP_DONE:
        /* Finished! */
        g_task_return_boolean (task, TRUE);
        g_object_unref (task);
        return;
    default:
        g_assert_not_reached ();
    }

    input = qmi_message_dms_set_operating_mode_input_new ();
    qmi_message_dms_set_operating_mode_input_set_mode (input, mode, NULL);
    qmi_client_dms_set_operating_mode (ctx->qmi_client,
                                       input,
                                       10,
                                       g_task_get_cancellable (task),
                                       (GAsyncReadyCallback) power_cycle_step_ready,
                                       task);
    qmi_message_dms_set_operating_mode_input_unref (input);
}

void
qfu_utils_power_cycle (QmiClientDms         *qmi_client,
                       GCancellable         *cancellable,
                       GAsyncReadyCallback   callback,
                       gpointer              user_data)
{
    GTask             *task;
    PowerCycleContext *ctx;

    ctx = g_slice_new0 (PowerCycleContext);
    ctx->step = POWER_CYCLE_STEP_OFFLINE;
    ctx->qmi_client = g_object_ref (qmi_client);

    task = g_task_new (NULL, cancellable, callback, user_data);
    g_task_set_task_data (task, ctx, (GDestroyNotify) power_cycle_context_free);

    power_cycle_step (task);
}

/******************************************************************************/

#if defined MM_RUNTIME_CHECK_ENABLED

gboolean
qfu_utils_modemmanager_running (gboolean  *mm_running,
                                GError   **error)
{
    GDBusConnection *connection;
    GVariant        *response;
    GError          *inner_error = NULL;

    g_assert (mm_running);

    connection = g_bus_get_sync (G_BUS_TYPE_SYSTEM, NULL, error);
    if (!connection) {
        g_prefix_error (error, "Couldn't get system bus: ");
        return FALSE;
    }

    response = g_dbus_connection_call_sync (connection,
                                            "org.freedesktop.ModemManager1",
                                            "/org/freedesktop/ModemManager1",
                                            "org.freedesktop.DBus.Peer",
                                            "Ping",
                                            NULL,
                                            NULL,
                                            G_DBUS_CALL_FLAGS_NO_AUTO_START,
                                            -1,
                                            NULL,
                                            &inner_error);
    if (!response) {
        g_debug ("[qfu-utils] couldn't ping ModemManager: %s", inner_error->message);
        g_error_free (inner_error);
        *mm_running = FALSE;
    } else {
        *mm_running = TRUE;
        g_variant_unref (response);
    }

    g_object_unref (connection);
    return TRUE;
}

#endif
