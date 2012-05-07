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
 * Copyright (C) 2012 Aleksander Morgado <aleksander@lanedo.com>
 */

#include "qmi-message-dms.h"
#include "qmi-enums.h"
#include "qmi-error-types.h"

/*****************************************************************************/
/* Get IDs */

enum {
  QMI_DMS_TLV_GET_IDS_ESN = 0x10,
  QMI_DMS_TLV_GET_IDS_IMEI,
  QMI_DMS_TLV_GET_IDS_MEID
};

/**
 * QmiDmsGetIdsOutput:
 *
 * An opaque type handling the output of the Get IDs operation.
 */
struct _QmiDmsGetIdsOutput {
    volatile gint ref_count;
    GError *error;
    gchar *esn;
    gchar *imei;
    gchar *meid;
};

/**
 * qmi_dms_get_ids_output_get_result:
 * @output: a #QmiDmsGetIdsOutput.
 * @error: a #GError.
 *
 * Get the result of the Get IDs operation.
 *
 * Returns: #TRUE if the operation succeeded, and #FALSE if @error is set.
 */
gboolean
qmi_dms_get_ids_output_get_result (QmiDmsGetIdsOutput *output,
                                   GError **error)
{
    g_return_val_if_fail (output != NULL, FALSE);

    if (output->error) {
        if (error)
            *error = g_error_copy (output->error);

        return FALSE;
    }

    return TRUE;
}

/**
 * qmi_dms_get_ids_output_get_esn:
 * @output: a #QmiDmsGetIdsOutput.
 *
 * Get the ESN.
 *
 * Returns: the ESN.
 */
const gchar *
qmi_dms_get_ids_output_get_esn (QmiDmsGetIdsOutput *output)
{
    g_return_val_if_fail (output != NULL, NULL);

    return output->esn;
}

/**
 * qmi_dms_get_ids_output_get_imei:
 * @output: a #QmiDmsGetIdsOutput.
 *
 * Get the IMEI.
 *
 * Returns: the IMEI.
 */
const gchar *
qmi_dms_get_ids_output_get_imei (QmiDmsGetIdsOutput *output)
{
    g_return_val_if_fail (output != NULL, NULL);

    return output->imei;
}

/**
 * qmi_dms_get_ids_output_get_meid:
 * @output: a #QmiDmsGetIdsOutput.
 *
 * Get the MEID.
 *
 * Returns: the MEID.
 */
const gchar *
qmi_dms_get_ids_output_get_meid (QmiDmsGetIdsOutput *output)
{
    g_return_val_if_fail (output != NULL, NULL);

    return output->meid;
}

/**
 * qmi_dms_get_ids_output_ref:
 * @output: a #QmiDmsGetIdsOutput.
 *
 * Atomically increments the reference count of @output by one.
 *
 * Returns: the new reference to @output.
 */
QmiDmsGetIdsOutput *
qmi_dms_get_ids_output_ref (QmiDmsGetIdsOutput *output)
{
    g_return_val_if_fail (output != NULL, NULL);

    g_atomic_int_inc (&output->ref_count);
    return output;
}

/**
 * qmi_dms_get_ids_output_unref:
 * @output: a #QmiDmsGetIdsOutput.
 *
 * Atomically decrements the reference count of @output by one.
 * If the reference count drops to 0, @output is completely disposed.
 */
void
qmi_dms_get_ids_output_unref (QmiDmsGetIdsOutput *output)
{
    g_return_if_fail (output != NULL);

    if (g_atomic_int_dec_and_test (&output->ref_count)) {
        g_free (output->esn);
        g_free (output->imei);
        g_free (output->meid);
        if (output->error)
            g_error_free (output->error);
        g_slice_free (QmiDmsGetIdsOutput, output);
    }
}

QmiMessage *
qmi_message_dms_get_ids_new (guint8 transaction_id,
                             guint8 client_id)
{
    return qmi_message_new (QMI_SERVICE_DMS,
                            client_id,
                            transaction_id,
                            QMI_DMS_MESSAGE_GET_IDS);
}

QmiDmsGetIdsOutput *
qmi_message_dms_get_ids_reply_parse (QmiMessage *self,
                                     GError **error)
{
    QmiDmsGetIdsOutput *output;
    GError *inner_error = NULL;

    g_assert (qmi_message_get_message_id (self) == QMI_DMS_MESSAGE_GET_IDS);

    if (!qmi_message_get_response_result (self, &inner_error)) {
        /* Only QMI protocol errors are set in the Output result, all the
         * others (e.g. failures parsing) are directly propagated to error. */
        if (inner_error->domain != QMI_PROTOCOL_ERROR) {
            g_propagate_error (error, inner_error);
            return NULL;
        }

        /* Otherwise, build output */
    }

    output = g_slice_new0 (QmiDmsGetIdsOutput);
    output->ref_count = 1;
    output->error = inner_error;

    /* Note: all ESN/IMEI/MEID are OPTIONAL; so it's ok if none of them appear */
    output->esn = qmi_message_tlv_get_string (self,
                                              QMI_DMS_TLV_GET_IDS_ESN,
                                              NULL);
    output->imei = qmi_message_tlv_get_string (self,
                                               QMI_DMS_TLV_GET_IDS_IMEI,
                                               NULL);
    output->meid = qmi_message_tlv_get_string (self,
                                               QMI_DMS_TLV_GET_IDS_MEID,
                                               NULL);

    return output;
}
