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

#include "qmi-message-wds.h"
#include "qmi-enums.h"
#include "qmi-error-types.h"

/*****************************************************************************/
/* Start network */

QmiMessage *
qmi_message_wds_start_network_new (guint8 transaction_id,
                                   guint8 client_id)
{
    /* TODO: handle optional TLVs */
    return qmi_message_new (QMI_SERVICE_WDS,
                            client_id,
                            transaction_id,
                            QMI_WDS_MESSAGE_START_NETWORK);
}

enum {
    START_NETWORK_OUTPUT_TLV_PACKET_DATA_HANDLE      = 0x01,
    START_NETWORK_OUTPUT_TLV_CALL_END_REASON         = 0x10,
    START_NETWORK_OUTPUT_TLV_VERBOSE_CALL_END_REASON = 0x11
};

struct verbose_call_end_reason {
	guint16 call_end_reason_type;
	guint16 call_end_reason;
} __attribute__((__packed__));

guint32
qmi_message_wds_start_network_reply_parse (QmiMessage *self,
                                           GError **error)
{
    GError *inner_error = NULL;
    guint32 packet_data_handle = 0;

    g_assert (qmi_message_get_message_id (self) == QMI_WDS_MESSAGE_START_NETWORK);

    /* If we got a QMI error reported and is a CALL_FAILED one, try to gather
     * the call end reason */
    if (!qmi_message_get_response_result (self, &inner_error)) {
        /* On CALL_FAILED errors, we can try to get more info on the reason */
        if (g_error_matches (inner_error,
                             QMI_PROTOCOL_ERROR,
                             QMI_PROTOCOL_ERROR_CALL_FAILED)) {
            guint16 cer = 0;
            struct verbose_call_end_reason verbose_cer = { 0, 0 };

            /* TODO: Prepare an enum with all the possible call end reasons,
             * in order to do this nicely */

            /* Try to get the verbose reason first */
            if (qmi_message_tlv_get (self,
                                     START_NETWORK_OUTPUT_TLV_VERBOSE_CALL_END_REASON,
                                     sizeof (verbose_cer),
                                     &verbose_cer,
                                     NULL)) {
                g_set_error (error,
                             QMI_PROTOCOL_ERROR,
                             QMI_PROTOCOL_ERROR_CALL_FAILED,
                             "Call end reason: %u, %u",
                             verbose_cer.call_end_reason_type,
                             verbose_cer.call_end_reason);
                g_error_free (inner_error);
                return 0;
            }

            /* If no verbose reason, try to use the legacy one */
            if (qmi_message_tlv_get (self,
                                     START_NETWORK_OUTPUT_TLV_CALL_END_REASON,
                                     sizeof (cer),
                                     &cer,
                                     NULL)) {
                g_set_error (error,
                             QMI_PROTOCOL_ERROR,
                             QMI_PROTOCOL_ERROR_CALL_FAILED,
                             "Call end reason: %u",
                             cer);
                g_error_free (inner_error);
                return 0;
            }

            /* otherwise, fall down and propagate the error */
        }

        g_propagate_error (error, inner_error);
        return 0;
    }

    if (!qmi_message_tlv_get (self,
                              START_NETWORK_OUTPUT_TLV_PACKET_DATA_HANDLE,
                              sizeof (packet_data_handle),
                              &packet_data_handle,
                              error)) {
        g_prefix_error (error, "Couldn't get the packet data handle TLV: ");
        return 0;
    }

    return packet_data_handle;
}

/*****************************************************************************/
/* Stop network */

enum {
    STOP_NETWORK_INPUT_TLV_PACKET_DATA_HANDLE = 0x01,
};

QmiMessage *
qmi_message_wds_stop_network_new (guint8 transaction_id,
                                  guint8 client_id,
                                  guint32 packet_data_handle)
{
    QmiMessage *message;
    GError *error = NULL;

    /* TODO: handle optional TLVs */
    message =  qmi_message_new (QMI_SERVICE_WDS,
                                client_id,
                                transaction_id,
                                QMI_WDS_MESSAGE_STOP_NETWORK);
    qmi_message_tlv_add (message,
                         STOP_NETWORK_INPUT_TLV_PACKET_DATA_HANDLE,
                         sizeof (packet_data_handle),
                         &packet_data_handle,
                         &error);
    g_assert_no_error (error);

    return message;
}

gboolean
qmi_message_wds_stop_network_reply_parse (QmiMessage *self,
                                          GError **error)
{
    g_assert (qmi_message_get_message_id (self) == QMI_WDS_MESSAGE_STOP_NETWORK);

    return qmi_message_get_response_result (self, error);
}
