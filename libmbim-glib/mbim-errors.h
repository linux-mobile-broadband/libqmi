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
 * Copyright (C) 2012 Aleksander Morgado <aleksander@lanedo.com>
 */

#ifndef _LIBMBIM_GLIB_MBIM_ERRORS_H_
#define _LIBMBIM_GLIB_MBIM_ERRORS_H_

#if !defined (__LIBMBIM_GLIB_H_INSIDE__) && !defined (LIBMBIM_GLIB_COMPILATION)
#error "Only <libmbim-glib.h> can be included directly."
#endif

/**
 * SECTION: mbim-errors
 * @title: Errors
 *
 * This section defines common error types used in the interface.
 */

/* Prefixes for errors registered in DBus */
#define MBIM_DBUS_ERROR_PREFIX          "org.freedesktop.libmbim.Error"
#define MBIM_CORE_ERROR_DBUS_PREFIX     MBIM_DBUS_ERROR_PREFIX ".Core"
#define MBIM_PROTOCOL_ERROR_DBUS_PREFIX MBIM_DBUS_ERROR_PREFIX ".Protocol"
#define MBIM_STATUS_ERROR_DBUS_PREFIX   MBIM_DBUS_ERROR_PREFIX ".Status"

/**
 * MbimCoreError:
 * @MBIM_CORE_ERROR_FAILED: Operation failed.
 * @MBIM_CORE_ERROR_WRONG_STATE: Operation cannot be executed in the current state.
 * @MBIM_CORE_ERROR_TIMEOUT: Operation timed out.
 * @MBIM_CORE_ERROR_INVALID_ARGS: Invalid arguments given.
 * @MBIM_CORE_ERROR_INVALID_MESSAGE: MBIM message is invalid.
 * @MBIM_CORE_ERROR_UNSUPPORTED: Not supported.
 * @MBIM_CORE_ERROR_ABORTED: Operation aborted..
 *
 * Common errors that may be reported by libmbim-glib.
 */
typedef enum { /*< underscore_name=mbim_core_error >*/
    MBIM_CORE_ERROR_FAILED           = 0, /*< nick=Failed >*/
    MBIM_CORE_ERROR_WRONG_STATE      = 1, /*< nick=WrongState >*/
    MBIM_CORE_ERROR_TIMEOUT          = 2, /*< nick=Timeout >*/
    MBIM_CORE_ERROR_INVALID_ARGS     = 3, /*< nick=InvalidArgs >*/
    MBIM_CORE_ERROR_INVALID_MESSAGE  = 4, /*< nick=InvalidMessage >*/
    MBIM_CORE_ERROR_UNSUPPORTED      = 5, /*< nick=Unsupported >*/
    MBIM_CORE_ERROR_ABORTED          = 6  /*< nick=Aborted >*/
} MbimCoreError;

/**
 * MbimProtocolError:
 * @MBIM_PROTOCOL_ERROR_INVALID: Invalid MBIM error.
 * @MBIM_PROTOCOL_ERROR_TIMEOUT_FRAGMENT: Timeout waiting for fragment.
 * @MBIM_PROTOCOL_ERROR_FRAGMENT_OUT_OF_SEQUENCE: Fragment received out of sequence.
 * @MBIM_PROTOCOL_ERROR_LENGTH_MISMATCH: Length mismatch.
 * @MBIM_PROTOCOL_ERROR_DUPLICATED_TID: Duplicated transaction ID.
 * @MBIM_PROTOCOL_ERROR_NOT_OPENED: Not opened.
 * @MBIM_PROTOCOL_ERROR_UNKNOWN: Unknown error.
 * @MBIM_PROTOCOL_ERROR_CANCEL: Cancel the operation.
 * @MBIM_PROTOCOL_ERROR_MAX_TRANSFER: Maximum control transfer not supported.
 *
 * MBIM protocol errors.
 */
typedef enum {
    MBIM_PROTOCOL_ERROR_INVALID                  = 0, /*< nick=Invalid >*/
    MBIM_PROTOCOL_ERROR_TIMEOUT_FRAGMENT         = 1, /*< nick=TimeoutFragment >*/
    MBIM_PROTOCOL_ERROR_FRAGMENT_OUT_OF_SEQUENCE = 2, /*< nick=FragmentOutOfSequence >*/
    MBIM_PROTOCOL_ERROR_LENGTH_MISMATCH          = 3, /*< nick=LengthMismatch >*/
    MBIM_PROTOCOL_ERROR_DUPLICATED_TID           = 4, /*< nick=DuplicatedTid >*/
    MBIM_PROTOCOL_ERROR_NOT_OPENED               = 5, /*< nick=NotOpened >*/
    MBIM_PROTOCOL_ERROR_UNKNOWN                  = 6, /*< nick=Unknown >*/
    MBIM_PROTOCOL_ERROR_CANCEL                   = 7, /*< nick=Cancel >*/
    MBIM_PROTOCOL_ERROR_MAX_TRANSFER             = 8  /*< nick=MaxTransfer >*/
} MbimProtocolError;


/**
 * MbimStatusError:
 * @MBIM_STATUS_ERROR_NONE: Success, no error.
 * @MBIM_STATUS_ERROR_BUSY: Busy.
 * @MBIM_STATUS_ERROR_FAILURE: Failure.
 * @MBIM_STATUS_ERROR_SIM_NOT_INSERTED: SIM not inserted.
 * @MBIM_STATUS_ERROR_BAD_SIM: Bad SIM.
 * @MBIM_STATUS_ERROR_PIN_REQUIRED: PIN required.
 * @MBIM_STATUS_ERROR_PIN_DISABLED: PIN disabled.
 * @MBIM_STATUS_ERROR_NOT_REGISTERED: Not registered.
 * @MBIM_STATUS_ERROR_PROVIDERS_NOT_FOUND: Providers not found.
 * @MBIM_STATUS_ERROR_NO_DEVICE_SUPPORT: No device support.
 * @MBIM_STATUS_ERROR_PROVIDER_NOT_VISIBLE: Provider not visible.
 * @MBIM_STATUS_ERROR_DATA_CLASS_NOT_AVAILABLE: Data class not available.
 * @MBIM_STATUS_ERROR_PACKET_SERVICE_DETACHED: Packet service detached.
 * @MBIM_STATUS_ERROR_MAX_ACTIVATED_CONTEXTS: Max activated contexts.
 * @MBIM_STATUS_ERROR_NOT_INITIALIZED: Not initialized.
 * @MBIM_STATUS_ERROR_VOICE_CALL_IN_PROGRESS: Voice call in progress.
 * @MBIM_STATUS_ERROR_CONTEXT_NOT_ACTIVATED: Context not activated.
 * @MBIM_STATUS_ERROR_SERVICE_NOT_ACTIVATED: Service not activated.
 * @MBIM_STATUS_ERROR_INVALID_ACCESS_STRING: Invalid access string.
 * @MBIM_STATUS_ERROR_INVALID_USER_NAME_PWD: Invalid user name or password.
 * @MBIM_STATUS_ERROR_RADIO_POWER_OFF: Radio power off.
 * @MBIM_STATUS_ERROR_INVALID_PARAMETERS: Invalid parameters.
 * @MBIM_STATUS_ERROR_READ_FAILURE: Read failure.
 * @MBIM_STATUS_ERROR_WRITE_FAILURE: Write failure.
 * @MBIM_STATUS_ERROR_NO_PHONEBOOK: No phonebook.
 * @MBIM_STATUS_ERROR_PARAMETER_TOO_LONG: Parameter too long.
 * @MBIM_STATUS_ERROR_STK_BUSY: SIM toolkit busy.
 * @MBIM_STATUS_ERROR_OPERATION_NOT_ALLOWED: Operation not allowed.
 * @MBIM_STATUS_ERROR_MEMORY_FAILURE: Memory failure.
 * @MBIM_STATUS_ERROR_INVALID_MEMORY_INDEX: Invalid memory index.
 * @MBIM_STATUS_ERROR_MEMORY_FULL: Memory full.
 * @MBIM_STATUS_ERROR_FILTER_NOT_SUPPORTED: Filter not supported.
 * @MBIM_STATUS_ERROR_DSS_INSTANCE_LIMIT: DSS instance limit.
 * @MBIM_STATUS_ERROR_INVALID_DEVICE_SERVICE_OPERATION: Invalid device service operation.
 * @MBIM_STATUS_ERROR_AUTH_INCORRECT_AUTN: Incorrect AUTN when sending authentication.
 * @MBIM_STATUS_ERROR_AUTH_SYNC_FAILURE: Synchronization failure during the authentication.
 * @MBIM_STATUS_ERROR_AUTH_AMF_NOT_SET: AMF bit not set in the authentication.
 * @MBIM_STATUS_ERROR_SMS_UNKNOWN_SMSC_ADDRESS: Unknown SMSC address.
 * @MBIM_STATUS_ERROR_SMS_NETWORK_TIMEOUT: Network timeout when sending SMS.
 * @MBIM_STATUS_ERROR_SMS_LANG_NOT_SUPPORTED: Language not supported in SMS.
 * @MBIM_STATUS_ERROR_SMS_ENCODING_NOT_SUPPORTED: Encoding not supported in SMS.
 * @MBIM_STATUS_ERROR_SMS_FORMAT_NOT_SUPPORTED: Format not supported in SMS.
 *
 * Status of the MBIM request.
 */
typedef enum {
    MBIM_STATUS_ERROR_NONE                             = 0,
    MBIM_STATUS_ERROR_BUSY                             = 1,
    MBIM_STATUS_ERROR_FAILURE                          = 2,
    MBIM_STATUS_ERROR_SIM_NOT_INSERTED                 = 3,
    MBIM_STATUS_ERROR_BAD_SIM                          = 4,
    MBIM_STATUS_ERROR_PIN_REQUIRED                     = 5,
    MBIM_STATUS_ERROR_PIN_DISABLED                     = 6,
    MBIM_STATUS_ERROR_NOT_REGISTERED                   = 7,
    MBIM_STATUS_ERROR_PROVIDERS_NOT_FOUND              = 8,
    MBIM_STATUS_ERROR_NO_DEVICE_SUPPORT                = 9,
    MBIM_STATUS_ERROR_PROVIDER_NOT_VISIBLE             = 10,
    MBIM_STATUS_ERROR_DATA_CLASS_NOT_AVAILABLE         = 11,
    MBIM_STATUS_ERROR_PACKET_SERVICE_DETACHED          = 12,
    MBIM_STATUS_ERROR_MAX_ACTIVATED_CONTEXTS           = 13,
    MBIM_STATUS_ERROR_NOT_INITIALIZED                  = 14,
    MBIM_STATUS_ERROR_VOICE_CALL_IN_PROGRESS           = 15,
    MBIM_STATUS_ERROR_CONTEXT_NOT_ACTIVATED            = 16,
    MBIM_STATUS_ERROR_SERVICE_NOT_ACTIVATED            = 17,
    MBIM_STATUS_ERROR_INVALID_ACCESS_STRING            = 18,
    MBIM_STATUS_ERROR_INVALID_USER_NAME_PWD            = 19,
    MBIM_STATUS_ERROR_RADIO_POWER_OFF                  = 20,
    MBIM_STATUS_ERROR_INVALID_PARAMETERS               = 21,
    MBIM_STATUS_ERROR_READ_FAILURE                     = 22,
    MBIM_STATUS_ERROR_WRITE_FAILURE                    = 23,
    /* 24 = reserved */
    MBIM_STATUS_ERROR_NO_PHONEBOOK                     = 25,
    MBIM_STATUS_ERROR_PARAMETER_TOO_LONG               = 26,
    MBIM_STATUS_ERROR_STK_BUSY                         = 27,
    MBIM_STATUS_ERROR_OPERATION_NOT_ALLOWED            = 28,
    MBIM_STATUS_ERROR_MEMORY_FAILURE                   = 29,
    MBIM_STATUS_ERROR_INVALID_MEMORY_INDEX             = 30,
    MBIM_STATUS_ERROR_MEMORY_FULL                      = 31,
    MBIM_STATUS_ERROR_FILTER_NOT_SUPPORTED             = 32,
    MBIM_STATUS_ERROR_DSS_INSTANCE_LIMIT               = 33,
    MBIM_STATUS_ERROR_INVALID_DEVICE_SERVICE_OPERATION = 34,
    MBIM_STATUS_ERROR_AUTH_INCORRECT_AUTN              = 35,
    MBIM_STATUS_ERROR_AUTH_SYNC_FAILURE                = 36,
    MBIM_STATUS_ERROR_AUTH_AMF_NOT_SET                 = 37,
    MBIM_STATUS_ERROR_SMS_UNKNOWN_SMSC_ADDRESS         = 100,
    MBIM_STATUS_ERROR_SMS_NETWORK_TIMEOUT              = 101,
    MBIM_STATUS_ERROR_SMS_LANG_NOT_SUPPORTED           = 102,
    MBIM_STATUS_ERROR_SMS_ENCODING_NOT_SUPPORTED       = 103,
    MBIM_STATUS_ERROR_SMS_FORMAT_NOT_SUPPORTED         = 104
} MbimStatusError;

#endif /* _LIBMBIM_GLIB_MBIM_ERRORS_H_ */
