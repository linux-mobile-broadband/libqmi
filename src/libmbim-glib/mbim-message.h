/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * libmbim-glib -- GLib/GIO based library to control MBIM devices
 *
 * Copyright (C) 2013 - 2022 Aleksander Morgado <aleksander@aleksander.es>
 * Copyright (C) 2022 Google, Inc.
 */

#ifndef _LIBMBIM_GLIB_MBIM_MESSAGE_H_
#define _LIBMBIM_GLIB_MBIM_MESSAGE_H_

#if !defined (__LIBMBIM_GLIB_H_INSIDE__) && !defined (LIBMBIM_GLIB_COMPILATION)
#error "Only <libmbim-glib.h> can be included directly."
#endif

#include <glib.h>
#include <glib-object.h>

#include "mbim-uuid.h"
#include "mbim-errors.h"

G_BEGIN_DECLS

/**
 * SECTION:mbim-message
 * @title: MbimMessage
 * @short_description: Generic MBIM message handling routines
 *
 * #MbimMessage is a generic type representing a MBIM message of any kind
 * (request, response, indication).
 **/

/**
 * MbimMessage:
 *
 * An opaque type representing a MBIM message.
 *
 * Since: 1.0
 */
typedef struct _MbimMessage MbimMessage;

GType mbim_message_get_type (void) G_GNUC_CONST;
#define MBIM_TYPE_MESSAGE (mbim_message_get_type ())

/**
 * MbimIPv4:
 * @addr: 4 bytes specifying the IPv4 address.
 *
 * An IPv4 address.
 *
 * Since: 1.0
 */
typedef struct _MbimIPv4 MbimIPv4;
struct _MbimIPv4 {
    guint8 addr[4];
};

/**
 * MbimIPv6:
 * @addr: 16 bytes specifying the IPv6 address.
 *
 * An IPv6 address.
 *
 * Since: 1.0
 */
typedef struct _MbimIPv6 MbimIPv6;
struct _MbimIPv6 {
    guint8 addr[16];
};

/**
 * MbimMessageType:
 * @MBIM_MESSAGE_TYPE_INVALID: Invalid MBIM message.
 * @MBIM_MESSAGE_TYPE_OPEN: Initialization request.
 * @MBIM_MESSAGE_TYPE_CLOSE: Close request.
 * @MBIM_MESSAGE_TYPE_COMMAND: Command request.
 * @MBIM_MESSAGE_TYPE_HOST_ERROR: Host-reported error in the communication.
 * @MBIM_MESSAGE_TYPE_OPEN_DONE: Response to initialization request.
 * @MBIM_MESSAGE_TYPE_CLOSE_DONE: Response to close request.
 * @MBIM_MESSAGE_TYPE_COMMAND_DONE: Response to command request.
 * @MBIM_MESSAGE_TYPE_FUNCTION_ERROR: Function-reported error in the communication.
 * @MBIM_MESSAGE_TYPE_INDICATE_STATUS: Unsolicited message from the function.
 *
 * Type of MBIM messages.
 *
 * Since: 1.0
 */
typedef enum { /*< since=1.0 >*/
    MBIM_MESSAGE_TYPE_INVALID         = 0x00000000,
    /* From Host to Function */
    MBIM_MESSAGE_TYPE_OPEN            = 0x00000001,
    MBIM_MESSAGE_TYPE_CLOSE           = 0x00000002,
    MBIM_MESSAGE_TYPE_COMMAND         = 0x00000003,
    MBIM_MESSAGE_TYPE_HOST_ERROR      = 0x00000004,
    /* From Function to Host */
    MBIM_MESSAGE_TYPE_OPEN_DONE       = 0x80000001,
    MBIM_MESSAGE_TYPE_CLOSE_DONE      = 0x80000002,
    MBIM_MESSAGE_TYPE_COMMAND_DONE    = 0x80000003,
    MBIM_MESSAGE_TYPE_FUNCTION_ERROR  = 0x80000004,
    MBIM_MESSAGE_TYPE_INDICATE_STATUS = 0x80000007
} MbimMessageType;

/*****************************************************************************/
/* Generic message interface */

/**
 * mbim_message_new:
 * @data: contents of the message.
 * @data_length: length of the message.
 *
 * Create a #MbimMessage with the given contents.
 *
 * Returns: (transfer full): a newly created #MbimMessage, which should be freed with mbim_message_unref().
 *
 * Since: 1.0
 */
MbimMessage *mbim_message_new (const guint8 *data,
                               guint32       data_length);

/**
 * mbim_message_dup:
 * @self: a #MbimMessage to duplicate.
 *
 * Create a #MbimMessage with the same contents as @self.
 *
 * Returns: (transfer full): a newly created #MbimMessage, which should be freed with mbim_message_unref().
 *
 * Since: 1.0
 */
MbimMessage *mbim_message_dup (const MbimMessage *self);

/**
 * mbim_message_ref:
 * @self: a #MbimMessage.
 *
 * Atomically increments the reference count of @self by one.
 *
 * Returns: (transfer full): the new reference to @self.
 *
 * Since: 1.0
 */
MbimMessage *mbim_message_ref (MbimMessage *self);

/**
 * mbim_message_unref:
 * @self: a #MbimMessage.
 *
 * Atomically decrements the reference count of @self by one.
 * If the reference count drops to 0, @self is completely disposed.
 *
 * Since: 1.0
 */
void mbim_message_unref (MbimMessage *self);

/**
 * mbim_message_get_printable:
 * @self: a #MbimMessage.
 * @line_prefix: prefix string to use in each new generated line.
 * @headers_only: %TRUE if only basic headers should be printed.
 *
 * Gets a printable string with the contents of the whole MBIM message.
 *
 * This method will not fail if the parsing of the message contents fails,
 * a fallback text with the error will be included in the generated printable
 * information instead.
 *
 * Returns: a newly allocated string, which should be freed with g_free().
 *
 * Since: 1.0
 */
gchar *mbim_message_get_printable (const MbimMessage  *self,
                                   const gchar        *line_prefix,
                                   gboolean            headers_only);

/**
 * mbim_message_get_printable_full:
 * @self: a #MbimMessage.
 * @mbimex_version_major: major version of the agreed MBIMEx support.
 * @mbimex_version_minor: minor version of the agreed MBIMEx support.
 * @line_prefix: prefix string to use in each new generated line.
 * @headers_only: %TRUE if only basic headers should be printed.
 * @error: return location for error or %NULL.
 *
 * Gets a printable string with the contents of the whole MBIM message.
 *
 * Unlike mbim_message_get_printable(), this method allows specifying the
 * MBIMEx version agreed between host and device, so that the correct
 * processing and parsing is done on messages in the newer MBIMEx versions.
 *
 * If @mbimex_version_major < 2, this method behaves exactly as
 * mbim_message_get_printable().
 *
 * If the specified @mbimex_version_major is unsupported, an error will be
 * returned.
 *
 * This method will not fail if the parsing of the message contents fails,
 * a fallback text with the error will be included in the generated printable
 * information instead.
 *
 * Returns: a newly allocated string which should be freed with g_free(), or
 * #NULL if @error is set.
 *
 * Since: 1.28
 */
gchar *mbim_message_get_printable_full (const MbimMessage  *self,
                                        guint8              mbimex_version_major,
                                        guint8              mbimex_version_minor,
                                        const gchar        *line_prefix,
                                        gboolean            headers_only,
                                        GError            **error);

/**
 * mbim_message_get_raw:
 * @self: a #MbimMessage.
 * @length: (out): return location for the size of the output buffer.
 * @error: return location for error or %NULL.
 *
 * Gets the whole raw data buffer of the #MbimMessage.
 *
 * Returns: The raw data buffer, or #NULL if @error is set.
 *
 * Since: 1.0
 */
const guint8 *mbim_message_get_raw (const MbimMessage  *self,
                                    guint32            *length,
                                    GError            **error);

/**
 * mbim_message_get_message_type:
 * @self: a #MbimMessage.
 *
 * Gets the message type.
 *
 * Returns: a #MbimMessageType.
 *
 * Since: 1.0
 */
MbimMessageType mbim_message_get_message_type (const MbimMessage *self);

/**
 * mbim_message_get_message_length:
 * @self: a #MbimMessage.
 *
 * Gets the whole message length.
 *
 * Returns: the length of the message.
 *
 * Since: 1.0
 */
guint32 mbim_message_get_message_length (const MbimMessage *self);

/**
 * mbim_message_get_transaction_id:
 * @self: a #MbimMessage.
 *
 * Gets the transaction ID of the message.
 *
 * Returns: the transaction ID.
 *
 * Since: 1.0
 */
guint32 mbim_message_get_transaction_id (const MbimMessage *self);

/**
 * mbim_message_set_transaction_id:
 * @self: a #MbimMessage.
 * @transaction_id: the transaction id.
 *
 * Sets the transaction ID of the message.
 *
 * Since: 1.0
 */
void mbim_message_set_transaction_id (MbimMessage *self,
                                      guint32      transaction_id);

/**
 * mbim_message_validate:
 * @self: a #MbimMessage.
 * @error: return location for error or %NULL.
 *
 * Validates the contents of the headers in the MBIM message.
 *
 * This operation may be used to ensure that the message is full and of a valid
 * type.
 *
 * This operation does not validate that the specific contents of a given
 * message type are available, that is done by the methods retrieving those
 * specific contents.
 *
 * Returns: %TRUE if the message is valid, %FALSE if @error is set.
 *
 * Since: 1.28
 */
gboolean mbim_message_validate (const MbimMessage  *self,
                                GError            **error);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (MbimMessage, mbim_message_unref)

/*****************************************************************************/
/* 'Open' message interface */

/**
 * mbim_message_open_new:
 * @transaction_id: transaction ID.
 * @max_control_transfer: maximum control transfer.
 *
 * Create a new #MbimMessage of type %MBIM_MESSAGE_TYPE_OPEN with the specified
 * parameters.
 *
 * Returns: (transfer full): a newly created #MbimMessage. The returned value
 * should be freed with mbim_message_unref().
 *
 * Since: 1.0
 */
MbimMessage *mbim_message_open_new (guint32 transaction_id,
                                    guint32 max_control_transfer);

/**
 * mbim_message_open_get_max_control_transfer:
 * @self: a #MbimMessage.
 *
 * Get the maximum control transfer set to be used in the #MbimMessage of type
 * %MBIM_MESSAGE_TYPE_OPEN.
 *
 * Returns: the maximum control transfer.
 *
 * Since: 1.0
 */
guint32 mbim_message_open_get_max_control_transfer (const MbimMessage *self);

/*****************************************************************************/
/* 'Open Done' message interface */

/**
 * mbim_message_open_done_new:
 * @transaction_id: transaction ID.
 * @error_status_code: a #MbimStatusError.
 *
 * Create a new #MbimMessage of type %MBIM_MESSAGE_TYPE_OPEN_DONE with the specified
 * parameters.
 *
 * Returns: (transfer full): a newly created #MbimMessage, which should be freed with mbim_message_unref().
 *
 * Since: 1.10
 */
MbimMessage *mbim_message_open_done_new (guint32         transaction_id,
                                         MbimStatusError error_status_code);

/**
 * mbim_message_open_done_get_status_code:
 * @self: a #MbimMessage.
 *
 * Get status code from the %MBIM_MESSAGE_TYPE_OPEN_DONE message.
 *
 * Returns: a #MbimStatusError.
 *
 * Since: 1.0
 */
MbimStatusError mbim_message_open_done_get_status_code (const MbimMessage  *self);

/**
 * mbim_message_open_done_get_result:
 * @self: a #MbimMessage.
 * @error: return location for error or %NULL.
 *
 * Gets the result of the 'Open' operation in the %MBIM_MESSAGE_TYPE_OPEN_DONE message.
 *
 * Returns: %TRUE if the operation succeeded, %FALSE if @error is set.
 *
 * Since: 1.0
 */
gboolean mbim_message_open_done_get_result (const MbimMessage  *self,
                                            GError            **error);

/*****************************************************************************/
/* 'Close' message interface */

/**
 * mbim_message_close_new:
 * @transaction_id: transaction ID.
 *
 * Create a new #MbimMessage of type %MBIM_MESSAGE_TYPE_CLOSE with the specified
 * parameters.
 *
 * Returns: (transfer full): a newly created #MbimMessage. The returned value
 * should be freed with mbim_message_unref().
 *
 * Since: 1.0
 */
MbimMessage *mbim_message_close_new (guint32 transaction_id);

/*****************************************************************************/
/* 'Close Done' message interface */

/**
 * mbim_message_close_done_new:
 * @transaction_id: transaction ID.
 * @error_status_code: a #MbimStatusError.
 *
 * Create a new #MbimMessage of type %MBIM_MESSAGE_TYPE_CLOSE_DONE with the specified
 * parameters.
 *
 * Returns: (transfer full): a newly created #MbimMessage, which should be freed with mbim_message_unref().
 *
 * Since: 1.10
 */
MbimMessage *mbim_message_close_done_new (guint32         transaction_id,
                                          MbimStatusError error_status_code);

/**
 * mbim_message_close_done_get_status_code:
 * @self: a #MbimMessage.
 *
 * Get status code from the %MBIM_MESSAGE_TYPE_CLOSE_DONE message.
 *
 * Returns: a #MbimStatusError.
 *
 * Since: 1.0
 */
MbimStatusError mbim_message_close_done_get_status_code (const MbimMessage  *self);

/**
 * mbim_message_close_done_get_result:
 * @self: a #MbimMessage.
 * @error: return location for error or %NULL.
 *
 * Gets the result of the 'Close' operation in the %MBIM_MESSAGE_TYPE_CLOSE_DONE message.
 *
 * Returns: %TRUE if the operation succeeded, %FALSE if @error is set.
 *
 * Since: 1.0
 */
gboolean mbim_message_close_done_get_result (const MbimMessage  *self,
                                             GError            **error);

/*****************************************************************************/
/* 'Error' message interface */

/**
 * mbim_message_error_new:
 * @transaction_id: transaction ID.
 * @error_status_code: a #MbimProtocolError.
 *
 * Create a new #MbimMessage of type %MBIM_MESSAGE_TYPE_HOST_ERROR with the specified
 * parameters.
 *
 * Returns: (transfer full): a newly created #MbimMessage. The returned value
 * should be freed with mbim_message_unref().
 *
 * Since: 1.0
 */
MbimMessage *mbim_message_error_new (guint32           transaction_id,
                                     MbimProtocolError error_status_code);

/**
 * mbim_message_function_error_new:
 * @transaction_id: transaction ID.
 * @error_status_code: a #MbimProtocolError.
 *
 * Create a new #MbimMessage of type %MBIM_MESSAGE_TYPE_FUNCTION_ERROR with the specified
 * parameters.
 *
 * Returns: (transfer full): a newly created #MbimMessage. The returned value
 * should be freed with mbim_message_unref().
 *
 * Since: 1.12
 */
MbimMessage *mbim_message_function_error_new (guint32           transaction_id,
                                              MbimProtocolError error_status_code);

/**
 * mbim_message_error_get_error_status_code:
 * @self: a #MbimMessage.
 *
 * Get the error code in a %MBIM_MESSAGE_TYPE_HOST_ERROR or
 * %MBIM_MESSAGE_TYPE_FUNCTION_ERROR message.
 *
 * Returns: a #MbimProtocolError.
 *
 * Since: 1.0
 */
MbimProtocolError mbim_message_error_get_error_status_code (const MbimMessage *self);

/**
 * mbim_message_error_get_error:
 * @self: a #MbimMessage.
 *
 * Get the error in a %MBIM_MESSAGE_TYPE_HOST_ERROR or
 * %MBIM_MESSAGE_TYPE_FUNCTION_ERROR message.
 *
 * Returns: a newly allocated #GError, which should be freed with g_error_free().
 *
 * Since: 1.0
 */
GError *mbim_message_error_get_error (const MbimMessage *self);

/*****************************************************************************/
/* 'Command' message interface */

/**
 * MbimMessageCommandType:
 * @MBIM_MESSAGE_COMMAND_TYPE_UNKNOWN: Unknown type.
 * @MBIM_MESSAGE_COMMAND_TYPE_QUERY: Query command.
 * @MBIM_MESSAGE_COMMAND_TYPE_SET: Set command.
 *
 * Type of command message.
 *
 * Since: 1.0
 */
typedef enum { /*< since=1.0 >*/
    MBIM_MESSAGE_COMMAND_TYPE_UNKNOWN = -1,
    MBIM_MESSAGE_COMMAND_TYPE_QUERY   = 0,
    MBIM_MESSAGE_COMMAND_TYPE_SET     = 1
} MbimMessageCommandType;

/**
 * mbim_message_command_new:
 * @transaction_id: transaction ID.
 * @service: a #MbimService.
 * @cid: the command ID.
 * @command_type: the command type.
 *
 * Create a new #MbimMessage of type %MBIM_MESSAGE_TYPE_COMMAND with the
 * specified parameters and an empty information buffer.
 *
 * Returns: (transfer full): a newly created #MbimMessage. The returned value
 * should be freed with mbim_message_unref().
 *
 * Since: 1.0
 */
MbimMessage *mbim_message_command_new (guint32                transaction_id,
                                       MbimService            service,
                                       guint32                cid,
                                       MbimMessageCommandType command_type);

/**
 * mbim_message_command_append:
 * @self: a #MbimMessage.
 * @buffer: raw buffer to append to the message.
 * @buffer_size: length of the data in @buffer.
 *
 * Appends the contents of @buffer to @self.
 *
 * Since: 1.0
 */
void mbim_message_command_append (MbimMessage  *self,
                                  const guint8 *buffer,
                                  guint32       buffer_size);

/**
 * mbim_message_command_get_service:
 * @self: a #MbimMessage.
 *
 * Get the service of a %MBIM_MESSAGE_TYPE_COMMAND message.
 *
 * Returns: a #MbimService.
 *
 * Since: 1.0
 */
MbimService mbim_message_command_get_service (const MbimMessage *self);

/**
 * mbim_message_command_get_service_id:
 * @self: a #MbimMessage.
 *
 * Get the service UUID of a %MBIM_MESSAGE_TYPE_COMMAND message.
 *
 * Returns: a #MbimUuid.
 *
 * Since: 1.0
 */
const MbimUuid *mbim_message_command_get_service_id (const MbimMessage *self);

/**
 * mbim_message_command_get_cid:
 * @self: a #MbimMessage.
 *
 * Get the command id of a %MBIM_MESSAGE_TYPE_COMMAND message.
 *
 * Returns: a CID.
 *
 * Since: 1.0
 */
guint32 mbim_message_command_get_cid (const MbimMessage *self);

/**
 * mbim_message_command_get_command_type:
 * @self: a #MbimMessage.
 *
 * Get the command type of a %MBIM_MESSAGE_TYPE_COMMAND message.
 *
 * Returns: a #MbimMessageCommandType.
 *
 * Since: 1.0
 */
MbimMessageCommandType mbim_message_command_get_command_type (const MbimMessage *self);

/**
 * mbim_message_command_get_raw_information_buffer:
 * @self: a #MbimMessage.
 * @out_length: (out): return location for the size of the output buffer.
 *
 * Gets the information buffer of the %MBIM_MESSAGE_TYPE_COMMAND message.
 *
 * Returns: (transfer none): The raw data buffer, or #NULL if empty.
 *
 * Since: 1.0
 */
const guint8 *mbim_message_command_get_raw_information_buffer (const MbimMessage *self,
                                                               guint32           *out_length);

/*****************************************************************************/
/* 'Command Done' message interface */

/**
 * mbim_message_command_done_get_service:
 * @self: a #MbimMessage.
 *
 * Get the service of a %MBIM_MESSAGE_TYPE_COMMAND_DONE message.
 *
 * Returns: a #MbimService.
 *
 * Since: 1.0
 */
MbimService mbim_message_command_done_get_service (const MbimMessage  *self);

/**
 * mbim_message_command_done_get_service_id:
 * @self: a #MbimMessage.
 *
 * Get the service UUID of a %MBIM_MESSAGE_TYPE_COMMAND_DONE message.
 *
 * Returns: a #MbimUuid.
 *
 * Since: 1.0
 */
const MbimUuid *mbim_message_command_done_get_service_id (const MbimMessage  *self);

/**
 * mbim_message_command_done_get_cid:
 * @self: a #MbimMessage.
 *
 * Get the command id of a %MBIM_MESSAGE_TYPE_COMMAND_DONE message.
 *
 * Returns: a CID.
 *
 * Since: 1.0
 */
guint32 mbim_message_command_done_get_cid (const MbimMessage  *self);

/**
 * mbim_message_command_done_get_status_code:
 * @self: a #MbimMessage.
 *
 * Get status code from the %MBIM_MESSAGE_TYPE_COMMAND_DONE message.
 *
 * Returns: a #MbimStatusError.
 *
 * Since: 1.0
 */
MbimStatusError mbim_message_command_done_get_status_code (const MbimMessage  *self);

/**
 * mbim_message_command_done_get_result:
 * @self: a #MbimMessage.
 * @error: return location for error or %NULL.
 *
 * Gets the result of the 'Command' operation in the %MBIM_MESSAGE_TYPE_COMMAND_DONE message.
 *
 * Returns: %TRUE if the operation succeeded, %FALSE if @error is set.
 *
 * Since: 1.0
 */
gboolean mbim_message_command_done_get_result (const MbimMessage  *self,
                                               GError            **error);

/**
 * mbim_message_command_done_get_raw_information_buffer:
 * @self: a #MbimMessage.
 * @out_length: (out): return location for the size of the output buffer.
 *
 * Gets the information buffer of the %MBIM_MESSAGE_TYPE_COMMAND_DONE message.
 *
 * Returns: (transfer none): The raw data buffer, or #NULL if empty.
 *
 * Since: 1.0
 */
const guint8 *mbim_message_command_done_get_raw_information_buffer (const MbimMessage *self,
                                                                    guint32           *out_length);

/*****************************************************************************/
/* 'Indicate Status' message interface */

/**
 * mbim_message_indicate_status_get_service:
 * @self: a #MbimMessage.
 *
 * Get the service of a %MBIM_MESSAGE_TYPE_INDICATE_STATUS message.
 *
 * Returns: a #MbimService.
 *
 * Since: 1.0
 */
MbimService mbim_message_indicate_status_get_service (const MbimMessage *self);

/**
 * mbim_message_indicate_status_get_service_id:
 * @self: a #MbimMessage.
 *
 * Get the service UUID of a %MBIM_MESSAGE_TYPE_INDICATE_STATUS message.
 *
 * Returns: a #MbimUuid.
 *
 * Since: 1.0
 */
const MbimUuid *mbim_message_indicate_status_get_service_id (const MbimMessage  *self);

/**
 * mbim_message_indicate_status_get_cid:
 * @self: a #MbimMessage.
 *
 * Get the command id of a %MBIM_MESSAGE_TYPE_INDICATE_STATUS message.
 *
 * Returns: a CID.
 *
 * Since: 1.0
 */
guint32 mbim_message_indicate_status_get_cid (const MbimMessage *self);

/**
 * mbim_message_indicate_status_get_raw_information_buffer:
 * @self: a #MbimMessage.
 * @out_length: (out): return location for the size of the output buffer.
 *
 * Gets the information buffer of the %MBIM_MESSAGE_TYPE_INDICATE_STATUS message.
 *
 * Returns: (transfer none): The raw data buffer, or #NULL if empty.
 *
 * Since: 1.0
 */
const guint8 *mbim_message_indicate_status_get_raw_information_buffer (const MbimMessage *self,
                                                                       guint32           *out_length);

/*****************************************************************************/
/* Other helpers */

/**
 * mbim_message_response_get_result:
 * @self: a #MbimMessage response message.
 * @expected: expected #MbimMessageType if there isn't any error in the operation.
 * @error: return location for error or %NULL.
 *
 * Gets the result of the operation from the response message, which
 * can be either a %MBIM_MESSAGE_TYPE_FUNCTION_ERROR message or a message of the
 * specified @expected type.
 *
 * Returns: %TRUE if the operation succeeded, %FALSE if @error is set.
 *
 * Since: 1.12
 */
gboolean mbim_message_response_get_result (const MbimMessage  *self,
                                           MbimMessageType     expected,
                                           GError            **error);

G_END_DECLS

#endif /* _LIBMBIM_GLIB_MBIM_MESSAGE_H_ */
