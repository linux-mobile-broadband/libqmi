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

#include <string.h>

#include "qmi-message-wds.h"
#include "qmi-enums.h"
#include "qmi-error-types.h"

/*****************************************************************************/
/* Start network */

enum {
  QMI_WDS_TLV_START_NETWORK_APN      = 0x14,
  QMI_WDS_TLV_START_NETWORK_USERNAME = 0x17,
  QMI_WDS_TLV_START_NETWORK_PASSWORD = 0x18,
};

/**
 * QmiWdsStartNetworkInput:
 *
 * An opaque type handling the input arguments that may be passed to the Start
 * Network operation in the WDS service.
 */
struct _QmiWdsStartNetworkInput {
    volatile gint ref_count;
    gchar *apn;
    gchar *username;
    gchar *password;
};

/**
 * qmi_wds_start_network_input_set_apn:
 * @input: a #QmiWdsStartNetworkInput.
 * @str: a string with the APN.
 *
 * Set the APN to use.
 */
void
qmi_wds_start_network_input_set_apn (QmiWdsStartNetworkInput *input,
                                     const gchar *str)
{
    g_return_if_fail (input != NULL);

    g_free (input->apn);
    input->apn = (str ? g_strdup (str) : NULL);
}

/**
 * qmi_wds_start_network_input_get_apn:
 * @input: a #QmiWdsStartNetworkInput.
 *
 * Get the configured APN to use.
 *
 * Returns: a constant string with the APN, or #NULL if none set.
 */
const gchar *
qmi_wds_start_network_input_get_apn (QmiWdsStartNetworkInput *input)
{
    g_return_val_if_fail (input != NULL, NULL);

    return input->apn;
}

/**
 * qmi_wds_start_network_input_set_username:
 * @input: a #QmiWdsStartNetworkInput.
 * @str: a string with the username.
 *
 * Set the username to use when authenticating with the network.
 */
void
qmi_wds_start_network_input_set_username (QmiWdsStartNetworkInput *input,
                                          const gchar *str)
{
    g_return_if_fail (input != NULL);

    g_free (input->username);
    input->username = (str ? g_strdup (str) : NULL);
}

/**
 * qmi_wds_start_network_input_get_username:
 * @input: a #QmiWdsStartNetworkInput.
 *
 * Get the configured username to use when authenticating with the network.
 *
 * Returns: a constant string with the username, or #NULL if none set.
 */
const gchar *
qmi_wds_start_network_input_get_username (QmiWdsStartNetworkInput *input)
{
    g_return_val_if_fail (input != NULL, NULL);

    return input->username;
}

/**
 * qmi_wds_start_network_input_set_password:
 * @input: a #QmiWdsStartNetworkInput.
 * @str: a string with the password.
 *
 * Set the password to use when authenticating with the network.
 */
void
qmi_wds_start_network_input_set_password (QmiWdsStartNetworkInput *input,
                                          const gchar *str)
{
    g_return_if_fail (input != NULL);

    g_free (input->password);
    input->password = (str ? g_strdup (str) : NULL);
}

/**
 * qmi_wds_start_network_input_get_password:
 * @input: a #QmiWdsStartNetworkInput.
 *
 * Get the configured password to use when authenticating with the network.
 *
 * Returns: a constant string with the password, or #NULL if none set.
 */
const gchar *
qmi_wds_start_network_input_get_password (QmiWdsStartNetworkInput *input)
{
    g_return_val_if_fail (input != NULL, NULL);

    return input->password;
}

/**
 * qmi_wds_start_network_input_new:
 *
 * Allocates a new #QmiWdsStartNetworkInput.
 *
 * Returns: the newly created #QmiWdsStartNetworkInput.
 */
QmiWdsStartNetworkInput *
qmi_wds_start_network_input_new (void)
{
    return g_slice_new0 (QmiWdsStartNetworkInput);
}

/**
 * qmi_wds_start_network_input_ref:
 * @input: a #QmiWdsStartNetworkInput.
 *
 * Atomically increments the reference count of @input by one.
 *
 * Returns: the new reference to @input.
 */
QmiWdsStartNetworkInput *
qmi_wds_start_network_input_ref (QmiWdsStartNetworkInput *input)
{
    g_return_val_if_fail (input != NULL, NULL);

    g_atomic_int_inc (&input->ref_count);
    return input;
}

/**
 * qmi_wds_start_network_input_unref:
 * @input: a #QmiWdsStartNetworkInput.
 *
 * Atomically decrements the reference count of @input by one.
 * If the reference count drops to 0, @input is completely disposed.
 */
void
qmi_wds_start_network_input_unref (QmiWdsStartNetworkInput *input)
{
    g_return_if_fail (input != NULL);

    if (g_atomic_int_dec_and_test (&input->ref_count)) {
        g_free (input->apn);
        g_free (input->username);
        g_free (input->password);
        g_slice_free (QmiWdsStartNetworkInput, input);
    }
}

QmiMessage *
qmi_message_wds_start_network_new (guint8 transaction_id,
                                   guint8 client_id,
                                   QmiWdsStartNetworkInput *input,
                                   GError **error)
{
    QmiMessage *message;

    message = qmi_message_new (QMI_SERVICE_WDS,
                               client_id,
                               transaction_id,
                               QMI_WDS_MESSAGE_START_NETWORK);

    /* If input arguments given, use them */
    if (input) {
        /* Add APN if any */
        if (input->apn &&
            !qmi_message_tlv_add (message,
                                  QMI_WDS_TLV_START_NETWORK_APN,
                                  strlen (input->apn) + 1,
                                  input->apn,
                                  error)) {
            g_prefix_error (error, "Failed to add APN to message: ");
            qmi_message_unref (message);
            return NULL;
        }

        /* Add username if any */
        if (input->username &&
            !qmi_message_tlv_add (message,
                                  QMI_WDS_TLV_START_NETWORK_USERNAME,
                                  strlen (input->username) + 1,
                                  input->username,
                                  error)) {
            g_prefix_error (error, "Failed to add username to message: ");
            qmi_message_unref (message);
            return NULL;
        }

        /* Add password if any */
        if (input->password &&
            !qmi_message_tlv_add (message,
                                  QMI_WDS_TLV_START_NETWORK_PASSWORD,
                                  strlen (input->password) + 1,
                                  input->password,
                                  error)) {
            g_prefix_error (error, "Failed to add password to message: ");
            qmi_message_unref (message);
            return NULL;
        }
    }

    return message;
}

enum {
    START_NETWORK_OUTPUT_TLV_PACKET_DATA_HANDLE      = 0x01,
    START_NETWORK_OUTPUT_TLV_CALL_END_REASON         = 0x10,
    START_NETWORK_OUTPUT_TLV_VERBOSE_CALL_END_REASON = 0x11
};

/**
 * QmiWdsStartNetworkOutput:
 *
 * An opaque type handling the output of the Start Network operation.
 */
struct _QmiWdsStartNetworkOutput {
    volatile gint ref_count;
    GError *error;

    gboolean packet_data_handle_set;
    guint32 packet_data_handle;

    gboolean call_end_reason_set;
    guint16 call_end_reason;

    gboolean verbose_call_end_reason_set;
    guint16 verbose_call_end_reason_domain;
    guint16 verbose_call_end_reason_value;
};

/**
 * qmi_wds_start_network_output_get_result:
 * @output: a #QmiWdsStartNetworkOutput.
 * @error: a #GError.
 *
 * Get the result of the Start Network operation.
 *
 * Returns: #TRUE if the operation succeeded, and #FALSE if @error is set.
 */
gboolean
qmi_wds_start_network_output_get_result (QmiWdsStartNetworkOutput *output,
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
 * qmi_wds_start_network_output_get_packet_data_handle:
 * @output: a #QmiWdsStartNetworkOutput.
 * @packet_data_handle: location for the output packet data handle.
 *
 * Get the packet data handle on a successful Start Network operation.
 *
 * Returns: #TRUE if @packet_data_handle is set, #FALSE otherwise.
 */
gboolean
qmi_wds_start_network_output_get_packet_data_handle (QmiWdsStartNetworkOutput *output,
                                                     guint32 *packet_data_handle)
{
    g_return_val_if_fail (output != NULL, FALSE);

    if (output->packet_data_handle_set)
        *packet_data_handle = output->packet_data_handle;
    return output->packet_data_handle_set;
}

/**
 * qmi_wds_start_network_output_get_call_end_reason:
 * @output: a #QmiWdsStartNetworkOutput.
 * @call_end_reason: location for the output call end reason.
 *
 * Get the call end reason, if the operation failed with a CALL_FAILED error.
 * This field is optional.
 *
 * Returns: #TRUE if @call_end_reason is set, #FALSE otherwise.
 */
gboolean
qmi_wds_start_network_output_get_call_end_reason (QmiWdsStartNetworkOutput *output,
                                                  guint16 *call_end_reason)
{
    g_return_val_if_fail (output != NULL, FALSE);

    if (output->call_end_reason_set)
        *call_end_reason = output->call_end_reason;
    return output->call_end_reason_set;
}

/**
 * qmi_wds_start_network_output_get_call_end_reason:
 * @output: a #QmiWdsStartNetworkOutput.
 * @verbose_call_end_reason_domain: location for the output call end reason domain.
 * @verbose_call_end_reason_value: location for the output call end reason value.
 *
 * Get the verbose call end reason, if the operation failed with a CALL_FAILED error.
 * This field is optional.
 *
 * Returns: #TRUE if @call_end_reason is set, #FALSE otherwise.
 */
gboolean
qmi_wds_start_network_output_get_verbose_call_end_reason (QmiWdsStartNetworkOutput *output,
                                                          guint16 *verbose_call_end_reason_domain,
                                                          guint16 *verbose_call_end_reason_value)
{
    g_return_val_if_fail (output != NULL, FALSE);

    if (output->verbose_call_end_reason_set) {
        *verbose_call_end_reason_domain = output->verbose_call_end_reason_domain;
        *verbose_call_end_reason_value = output->verbose_call_end_reason_value;
    }
    return output->call_end_reason_set;
}

/**
 * qmi_wds_start_network_output_ref:
 * @output: a #QmiWdsStartNetworkOutput.
 *
 * Atomically increments the reference count of @output by one.
 *
 * Returns: the new reference to @output.
 */
QmiWdsStartNetworkOutput *
qmi_wds_start_network_output_ref (QmiWdsStartNetworkOutput *output)
{
    g_return_val_if_fail (output != NULL, NULL);

    g_atomic_int_inc (&output->ref_count);
    return output;
}

/**
 * qmi_wds_start_network_output_unref:
 * @output: a #QmiWdsStartNetworkOutput.
 *
 * Atomically decrements the reference count of @output by one.
 * If the reference count drops to 0, @output is completely disposed.
 */
void
qmi_wds_start_network_output_unref (QmiWdsStartNetworkOutput *output)
{
    g_return_if_fail (output != NULL);

    if (g_atomic_int_dec_and_test (&output->ref_count)) {
        if (output->error)
            g_error_free (output->error);
        g_slice_free (QmiWdsStartNetworkOutput, output);
    }
}

QmiWdsStartNetworkOutput *
qmi_message_wds_start_network_reply_parse (QmiMessage *self,
                                           GError **error)
{
    QmiWdsStartNetworkOutput *output;
    GError *inner_error = NULL;

    g_assert (qmi_message_get_message_id (self) == QMI_WDS_MESSAGE_START_NETWORK);

    if (!qmi_message_get_response_result (self, &inner_error)) {
        /* Only QMI protocol errors are set in the Output result, all the
         * others (e.g. failures parsing) are directly propagated to error. */
        if (inner_error->domain != QMI_PROTOCOL_ERROR) {
            g_propagate_error (error, inner_error);
            return NULL;
        }

        /* Otherwise, build output */
    }

    output = g_slice_new0 (QmiWdsStartNetworkOutput);
    output->error = inner_error;

    /* On CALL_FAILED errors, we can try to get more info on the reason */
    if (output->error) {
        if (g_error_matches (output->error,
                             QMI_PROTOCOL_ERROR,
                             QMI_PROTOCOL_ERROR_CALL_FAILED)) {
            guint16 cer = 0;
            struct verbose_call_end_reason {
                guint16 call_end_reason_domain;
                guint16 call_end_reason_value;
            } __attribute__((__packed__)) verbose_cer = { 0, 0 };

            /* TODO: Prepare an enum with all the possible call end reasons,
             * in order to do this nicely */

            if (qmi_message_tlv_get (self,
                                     START_NETWORK_OUTPUT_TLV_VERBOSE_CALL_END_REASON,
                                     sizeof (verbose_cer),
                                     &verbose_cer,
                                     NULL)) {
                output->verbose_call_end_reason_set = TRUE;
                output->verbose_call_end_reason_domain = verbose_cer.call_end_reason_domain;
                output->verbose_call_end_reason_value = verbose_cer.call_end_reason_value;
            }

            if (qmi_message_tlv_get (self,
                                     START_NETWORK_OUTPUT_TLV_CALL_END_REASON,
                                     sizeof (cer),
                                     &cer,
                                     NULL)) {
                output->call_end_reason_set = TRUE;
                output->call_end_reason = cer;
            }
        }

        return output;
    }

    /* success */

    if (!qmi_message_tlv_get (self,
                              START_NETWORK_OUTPUT_TLV_PACKET_DATA_HANDLE,
                              sizeof (output->packet_data_handle),
                              &output->packet_data_handle,
                              error)) {
        g_prefix_error (error, "Couldn't get the packet data handle TLV: ");
        qmi_wds_start_network_output_unref (output);
        return NULL;
    } else
        output->packet_data_handle_set = TRUE;

    return output;
}

/*****************************************************************************/
/* Stop network */

enum {
    STOP_NETWORK_INPUT_TLV_PACKET_DATA_HANDLE = 0x01,
};

/**
 * QmiWdsStopNetworkInput:
 *
 * An opaque type handling the input arguments that may be passed to the Stop
 * Network operation in the WDS service.
 */
struct _QmiWdsStopNetworkInput {
    volatile gint ref_count;

    gboolean packet_data_handle_set;
    guint32 packet_data_handle;
};

/**
 * qmi_wds_stop_network_input_set_packet_data_handle:
 * @input: a #QmiWdsStopNetworkInput.
 * @packet_data_handle: the packet data handle.
 *
 * Set the packet data handle of the connection.
 */
void
qmi_wds_stop_network_input_set_packet_data_handle (QmiWdsStopNetworkInput *input,
                                                   guint32 packet_data_handle)
{
    g_return_if_fail (input != NULL);

    input->packet_data_handle_set = TRUE;
    input->packet_data_handle = packet_data_handle;
}

/**
 * qmi_wds_stop_network_input_get_packet_data_handle:
 * @input: a #QmiWdsStopNetworkInput.
 * @packet_data_handle: output location for the packet data handle.
 *
 * Get the packet data handle of the connection.
 *
 * Returns: #TRUE if @packet_data_handle is set, #FALSE otherwise.
 */
gboolean
qmi_wds_stop_network_input_get_packet_data_handle (QmiWdsStopNetworkInput *input,
                                                   guint32 *packet_data_handle)
{
    g_return_val_if_fail (input != NULL, FALSE);

    if (input->packet_data_handle_set)
        *packet_data_handle = input->packet_data_handle;
    return input->packet_data_handle_set;
}

/**
 * qmi_wds_stop_network_input_new:
 *
 * Allocates a new #QmiWdsStopNetworkInput.
 *
 * Returns: the newly created #QmiWdsStopNetworkInput.
 */
QmiWdsStopNetworkInput *
qmi_wds_stop_network_input_new (void)
{
    return g_slice_new0 (QmiWdsStopNetworkInput);
}

/**
 * qmi_wds_stop_network_input_ref:
 * @input: a #QmiWdsStopNetworkInput.
 *
 * Atomically increments the reference count of @input by one.
 *
 * Returns: the new reference to @input.
 */
QmiWdsStopNetworkInput *
qmi_wds_stop_network_input_ref (QmiWdsStopNetworkInput *input)
{
    g_return_val_if_fail (input != NULL, NULL);

    g_atomic_int_inc (&input->ref_count);
    return input;
}

/**
 * qmi_wds_stop_network_input_unref:
 * @input: a #QmiWdsStopNetworkInput.
 *
 * Atomically decrements the reference count of @input by one.
 * If the reference count drops to 0, @input is completely disposed.
 */
void
qmi_wds_stop_network_input_unref (QmiWdsStopNetworkInput *input)
{
    g_return_if_fail (input != NULL);

    if (g_atomic_int_dec_and_test (&input->ref_count)) {
        g_slice_free (QmiWdsStopNetworkInput, input);
    }
}

QmiMessage *
qmi_message_wds_stop_network_new (guint8 transaction_id,
                                  guint8 client_id,
                                  QmiWdsStopNetworkInput *input,
                                  GError **error)
{
    QmiMessage *message;

    /* Check mandatory input arguments */
    if (!input ||
        !input->packet_data_handle_set) {
        g_set_error (error,
                     QMI_CORE_ERROR,
                     QMI_CORE_ERROR_INVALID_ARGS,
                     "Missing mandatory argument 'packet data handle'");
        return NULL;
    }

    message = qmi_message_new (QMI_SERVICE_WDS,
                               client_id,
                               transaction_id,
                               QMI_WDS_MESSAGE_STOP_NETWORK);

    if (!qmi_message_tlv_add (message,
                              STOP_NETWORK_INPUT_TLV_PACKET_DATA_HANDLE,
                              sizeof (input->packet_data_handle),
                              &input->packet_data_handle,
                              error)) {
        g_prefix_error (error, "Failed to add packet data handle to message: ");
        qmi_message_unref (message);
        return NULL;
    }

    return message;
}

/**
 * QmiWdsStopNetworkOutput:
 *
 * An opaque type handling the output of the Stop Network operation.
 */
struct _QmiWdsStopNetworkOutput {
    volatile gint ref_count;
    GError *error;
};

/**
 * qmi_wds_stop_network_output_get_result:
 * @output: a #QmiWdsStopNetworkOutput.
 * @error: a #GError.
 *
 * Get the result of the Stop Network operation.
 *
 * Returns: #TRUE if the operation succeeded, and #FALSE if @error is set.
 */
gboolean
qmi_wds_stop_network_output_get_result (QmiWdsStopNetworkOutput *output,
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
 * qmi_wds_stop_network_output_ref:
 * @output: a #QmiWdsStopNetworkOutput.
 *
 * Atomically increments the reference count of @output by one.
 *
 * Returns: the new reference to @output.
 */
QmiWdsStopNetworkOutput *
qmi_wds_stop_network_output_ref (QmiWdsStopNetworkOutput *output)
{
    g_return_val_if_fail (output != NULL, NULL);

    g_atomic_int_inc (&output->ref_count);
    return output;
}

/**
 * qmi_wds_stop_network_output_unref:
 * @output: a #QmiWdsStopNetworkOutput.
 *
 * Atomically decrements the reference count of @output by one.
 * If the reference count drops to 0, @output is completely disposed.
 */
void
qmi_wds_stop_network_output_unref (QmiWdsStopNetworkOutput *output)
{
    g_return_if_fail (output != NULL);

    if (g_atomic_int_dec_and_test (&output->ref_count)) {
        if (output->error)
            g_error_free (output->error);
        g_slice_free (QmiWdsStopNetworkOutput, output);
    }
}

QmiWdsStopNetworkOutput *
qmi_message_wds_stop_network_reply_parse (QmiMessage *self,
                                           GError **error)
{
    QmiWdsStopNetworkOutput *output;
    GError *inner_error = NULL;

    g_assert (qmi_message_get_message_id (self) == QMI_WDS_MESSAGE_STOP_NETWORK);

    if (!qmi_message_get_response_result (self, &inner_error)) {
        /* Only QMI protocol errors are set in the Output result, all the
         * others (e.g. failures parsing) are directly propagated to error. */
        if (inner_error->domain != QMI_PROTOCOL_ERROR) {
            g_propagate_error (error, inner_error);
            return NULL;
        }

        /* Otherwise, build output */
    }

    output = g_slice_new0 (QmiWdsStopNetworkOutput);
    output->error = inner_error;

    return output;
}

/*****************************************************************************/
/* Get packet service status */

QmiMessage *
qmi_message_wds_get_packet_service_status_new (guint8 transaction_id,
                                               guint8 client_id)
{
    return qmi_message_new (QMI_SERVICE_WDS,
                            client_id,
                            transaction_id,
                            QMI_WDS_MESSAGE_GET_PACKET_SERVICE_STATUS);
}

enum {
    GET_PACKET_SERVICE_STATUS_OUTPUT_TLV_CONNECTION_STATUS = 0x01,
};

/**
 * QmiWdsGetPacketServiceStatusOutput:
 *
 * An opaque type handling the output of the Stop Network operation.
 */
struct _QmiWdsGetPacketServiceStatusOutput {
    volatile gint ref_count;
    GError *error;
    guint8 connection_status;
};

/**
 * qmi_wds_get_packet_service_status_output_get_result:
 * @output: a #QmiWdsGetPacketServiceStatusOutput.
 * @error: a #GError.
 *
 * Get the result of the Get Packet Status operation.
 *
 * Returns: #TRUE if the operation succeeded, and #FALSE if @error is set.
 */
gboolean
qmi_wds_get_packet_service_status_output_get_result (QmiWdsGetPacketServiceStatusOutput *output,
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
 * qmi_wds_start_network_output_get_connection_status:
 * @output: a #QmiWdsGetPacketServiceStatusOutput.
 *
 * Get the connection status.
 *
 * Returns: a #QmiWdsConnectionStatus.
 */
QmiWdsConnectionStatus
qmi_wds_get_packet_service_status_output_get_connection_status (QmiWdsGetPacketServiceStatusOutput *output)
{
    g_return_val_if_fail (output != NULL, QMI_WDS_CONNECTION_STATUS_UNKNOWN);

    return (QmiWdsConnectionStatus)output->connection_status;
}

/**
 * qmi_wds_get_packet_service_status_output_ref:
 * @output: a #QmiWdsGetPacketServiceStatusOutput.
 *
 * Atomically increments the reference count of @output by one.
 *
 * Returns: the new reference to @output.
 */
QmiWdsGetPacketServiceStatusOutput *
qmi_wds_get_packet_service_status_output_ref (QmiWdsGetPacketServiceStatusOutput *output)
{
    g_return_val_if_fail (output != NULL, NULL);

    g_atomic_int_inc (&output->ref_count);
    return output;
}

/**
 * qmi_wds_get_packet_service_status_output_unref:
 * @output: a #QmiWdsGetPacketServiceStatusOutput.
 *
 * Atomically decrements the reference count of @output by one.
 * If the reference count drops to 0, @output is completely disposed.
 */
void
qmi_wds_get_packet_service_status_output_unref (QmiWdsGetPacketServiceStatusOutput *output)
{
    g_return_if_fail (output != NULL);

    if (g_atomic_int_dec_and_test (&output->ref_count)) {
        if (output->error)
            g_error_free (output->error);
        g_slice_free (QmiWdsGetPacketServiceStatusOutput, output);
    }
}

QmiWdsGetPacketServiceStatusOutput *
qmi_message_wds_get_packet_service_status_reply_parse (QmiMessage *self,
                                                       GError **error)
{
    QmiWdsGetPacketServiceStatusOutput *output;
    GError *inner_error = NULL;

    g_assert (qmi_message_get_message_id (self) == QMI_WDS_MESSAGE_GET_PACKET_SERVICE_STATUS);

    if (!qmi_message_get_response_result (self, &inner_error)) {
        /* Only QMI protocol errors are set in the Output result, all the
         * others (e.g. failures parsing) are directly propagated to error. */
        if (inner_error->domain != QMI_PROTOCOL_ERROR) {
            g_propagate_error (error, inner_error);
            return NULL;
        }

        /* Otherwise, build output */
    }

    /* success */

    output = g_slice_new0 (QmiWdsGetPacketServiceStatusOutput);
    output->error = inner_error;

    if (!qmi_message_tlv_get (self,
                              GET_PACKET_SERVICE_STATUS_OUTPUT_TLV_CONNECTION_STATUS,
                              sizeof (output->connection_status),
                              &output->connection_status,
                              error)) {
        g_prefix_error (error, "Couldn't get the connection status TLV: ");
        qmi_wds_get_packet_service_status_output_unref (output);
        return NULL;
    }

    return output;
}
