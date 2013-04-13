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
 * Copyright (C) 2013 Aleksander Morgado <aleksander@gnu.org>
 */

#ifndef _LIBMBIM_GLIB_MBIM_ENUMS_H_
#define _LIBMBIM_GLIB_MBIM_ENUMS_H_

#if !defined (__LIBMBIM_GLIB_H_INSIDE__) && !defined (LIBMBIM_GLIB_COMPILATION)
#error "Only <libmbim-glib.h> can be included directly."
#endif

G_BEGIN_DECLS

/*****************************************************************************/
/* 'Device Caps' enums */

/**
 * MbimDeviceType:
 * @MBIM_DEVICE_TYPE_UNKNOWN: Unknown type.
 * @MBIM_DEVICE_TYPE_EMBEDDED: Device is embedded in the system.
 * @MBIM_DEVICE_TYPE_REMOVABLE: Device is removable.
 * @MBIM_DEVICE_TYPE_REMOTE: Device is remote.
 *
 * Type of device.
 */
typedef enum {
    MBIM_DEVICE_TYPE_UNKNOWN   = 0,
    MBIM_DEVICE_TYPE_EMBEDDED  = 1,
    MBIM_DEVICE_TYPE_REMOVABLE = 2,
    MBIM_DEVICE_TYPE_REMOTE    = 3
} MbimDeviceType;

/**
 * MbimCellularClass:
 * @MBIM_CELLULAR_CLASS_GSM: Device is 3GPP.
 * @MBIM_CELLULAR_CLASS_CDMA: Device is 3GPP2.
 *
 * Cellular class.
 */
typedef enum {
    MBIM_CELLULAR_CLASS_GSM  = 1 << 0,
    MBIM_CELLULAR_CLASS_CDMA = 1 << 1
} MbimCellularClass;

/**
 * MbimVoiceClass:
 * @MBIM_VOICE_CLASS_UNKNOWN: Unknown voice class.
 * @MBIM_VOICE_CLASS_NO_VOICE: Device doesn't support voice.
 * @MBIM_VOICE_CLASS_SEPARATED_VOICE_DATA: Device supports separate voice and data connections.
 * @MBIM_VOICE_CLASS_SIMULTANEOUS_VOICE_DATA: Device supports simultaneous voice and data connections.
 *
 * Voice class.
 */
typedef enum {
    MBIM_VOICE_CLASS_UNKNOWN                 = 0,
    MBIM_VOICE_CLASS_NO_VOICE                = 1,
    MBIM_VOICE_CLASS_SEPARATED_VOICE_DATA    = 2,
    MBIM_VOICE_CLASS_SIMULTANEOUS_VOICE_DATA = 3
} MbimVoiceClass;

/**
 * MbimSimClass:
 * @MBIM_SIM_CLASS_LOGICAL: No physical SIM.
 * @MBIM_SIM_CLASS_REMOVABLE: Physical removable SIM.
 *
 * SIM class.
 */
typedef enum {
    MBIM_SIM_CLASS_LOGICAL   = 1 << 0,
    MBIM_SIM_CLASS_REMOVABLE = 1 << 1
} MbimSimClass;

/**
 * MbimDataClass:
 * @MBIM_DATA_CLASS_GPRS: GPRS.
 * @MBIM_DATA_CLASS_EDGE: EDGE.
 * @MBIM_DATA_CLASS_UMTS: UMTS.
 * @MBIM_DATA_CLASS_HSDPA: HSDPA.
 * @MBIM_DATA_CLASS_HSUPA: HSUPA.
 * @MBIM_DATA_CLASS_LTE: LTE.
 * @MBIM_DATA_CLASS_1XRTT: 1xRTT.
 * @MBIM_DATA_CLASS_1XEVDO: 1xEV-DO.
 * @MBIM_DATA_CLASS_1XEVDO_REVA: 1xEV-DO RevA
 * @MBIM_DATA_CLASS_1XEVDV: 1xEV-DV.
 * @MBIM_DATA_CLASS_3XRTT: 3xRTT.
 * @MBIM_DATA_CLASS_1XEVDO_REVB: 1xEV-DO RevB.
 * @MBIM_DATA_CLASS_UMB: UMB.
 * @MBIM_DATA_CLASS_CUSTOM: Custom.
 *
 * Data class.
 */
typedef enum {
    MBIM_DATA_CLASS_GPRS        = 1 << 0,
    MBIM_DATA_CLASS_EDGE        = 1 << 1,
    MBIM_DATA_CLASS_UMTS        = 1 << 2,
    MBIM_DATA_CLASS_HSDPA       = 1 << 3,
    MBIM_DATA_CLASS_HSUPA       = 1 << 4,
    MBIM_DATA_CLASS_LTE         = 1 << 5,
    /* Bits 6 to 15 reserved for future 3GPP classes */
    MBIM_DATA_CLASS_1XRTT       = 1 << 16,
    MBIM_DATA_CLASS_1XEVDO      = 1 << 17,
    MBIM_DATA_CLASS_1XEVDO_REVA = 1 << 18,
    MBIM_DATA_CLASS_1XEVDV      = 1 << 19,
    MBIM_DATA_CLASS_3XRTT       = 1 << 20,
    MBIM_DATA_CLASS_1XEVDO_REVB = 1 << 21,
    MBIM_DATA_CLASS_UMB         = 1 << 22,
    /* Bits 23 to 30 reserved for future 3GPP2 classes */
    MBIM_DATA_CLASS_CUSTOM      = 1 << 31
} MbimDataClass;

/**
 * MbimSmsCaps:
 * @MBIM_SMS_CAPS_PDU_RECEIVE: Can receive in PDU mode.
 * @MBIM_SMS_CAPS_PDU_SEND: Can send in PDU mode.
 * @MBIM_SMS_CAPS_TEXT_RECEIVE: Can receive in text mode.
 * @MBIM_SMS_CAPS_TEXT_SEND: Can send in text mode.
 *
 * SMS capabilities.
 */
typedef enum {
    MBIM_SMS_CAPS_PDU_RECEIVE  = 1 << 0,
    MBIM_SMS_CAPS_PDU_SEND     = 1 << 1,
    MBIM_SMS_CAPS_TEXT_RECEIVE = 1 << 2,
    MBIM_SMS_CAPS_TEXT_SEND    = 1 << 3
} MbimSmsCaps;

/**
 * MbimCtrlCaps:
 * @MBIM_CTRL_CAPS_REG_MANUAL: Device allows manual network selection.
 * @MBIM_CTRL_CAPS_HW_RADIO_SWITCH: Device has a hardware radio power switch.
 * @MBIM_CTRL_CAPS_CDMA_MOBILE_IP: The CDMA function supports Mobile IP.
 * @MBIM_CTRL_CAPS_CDMA_SIMPLE_IP: The CDMA function supports Simple IP.
 * @MBIM_CTRL_CAPS_MULTI_CARRIER: Device can work with multiple providers.
 *
 * Control capabilities.
 */
typedef enum {
    MBIM_CTRL_CAPS_REG_MANUAL      = 1 << 0,
    MBIM_CTRL_CAPS_HW_RADIO_SWITCH = 1 << 1,
    MBIM_CTRL_CAPS_CDMA_MOBILE_IP  = 1 << 2,
    MBIM_CTRL_CAPS_CDMA_SIMPLE_IP  = 1 << 3,
    MBIM_CTRL_CAPS_MULTI_CARRIER   = 1 << 4
} MbimCtrlCaps;

/*****************************************************************************/
/* 'Subscriber Ready Status' enums */

/**
 * MbimSubscriberReadyState:
 * @MBIM_SUBSCRIBER_READY_STATE_NOT_INITIALIZED: Not initialized.
 * @MBIM_SUBSCRIBER_READY_STATE_INITIALIZED: Initialized.
 * @MBIM_SUBSCRIBER_READY_STATE_SIM_NOT_INSERTED: SIM not inserted.
 * @MBIM_SUBSCRIBER_READY_STATE_BAD_SIM: Bad SIM.
 * @MBIM_SUBSCRIBER_READY_STATE_FAILURE: Failure.
 * @MBIM_SUBSCRIBER_READY_STATE_NOT_ACTIVATED: Not activated.
 * @MBIM_SUBSCRIBER_READY_STATE_DEVICE_LOCKED: Device locked.
 *
 * Ready state of the subscriber.
 */
typedef enum {
    MBIM_SUBSCRIBER_READY_STATE_NOT_INITIALIZED  = 0,
    MBIM_SUBSCRIBER_READY_STATE_INITIALIZED      = 1,
    MBIM_SUBSCRIBER_READY_STATE_SIM_NOT_INSERTED = 2,
    MBIM_SUBSCRIBER_READY_STATE_BAD_SIM          = 3,
    MBIM_SUBSCRIBER_READY_STATE_FAILURE          = 4,
    MBIM_SUBSCRIBER_READY_STATE_NOT_ACTIVATED    = 5,
    MBIM_SUBSCRIBER_READY_STATE_DEVICE_LOCKED    = 6,
} MbimSubscriberReadyState;

/**
 * MbimReadyInfoFlag:
 * @MBIM_READY_INFO_FLAG_PROTECT_UNIQUE_ID: Request to avoid displaying subscriber ID.
 */
typedef enum {
    MBIM_READY_INFO_FLAG_PROTECT_UNIQUE_ID = 1 << 0
} MbimReadyInfoFlag;

/*****************************************************************************/
/* 'Radio State' enums */

/**
 * MbimRadioSwitchState:
 * @MBIM_RADIO_SWITCH_STATE_OFF: Radio is off.
 * @MBIM_RADIO_SWITCH_STATE_ON: Radio is on.
 *
 * Radio switch state.
 */
typedef enum {
    MBIM_RADIO_SWITCH_STATE_OFF = 0,
    MBIM_RADIO_SWITCH_STATE_ON  = 1
} MbimRadioSwitchState;

/*****************************************************************************/
/* 'Pin' enums */

/**
 * MbimPinType:
 * @MBIM_PIN_TYPE_CUSTOM: The PIN type is a custom type and is none of the other PIN types listed in this enumeration.
 * @MBIM_PIN_TYPE_PIN1: The PIN1 key.
 * @MBIM_PIN_TYPE_PIN2: The PIN2 key.
 * @MBIM_PIN_TYPE_DEVICE_SIM_PIN: The device to SIM key.
 * @MBIM_PIN_TYPE_DEVICE_FIRST_SIM_PIN: The device to very first SIM key.
 * @MBIM_PIN_TYPE_NETWORK_PIN: The network personalization key.
 * @MBIM_PIN_TYPE_NETWORK_SUBSET_PIN: The network subset personalization key.
 * @MBIM_PIN_TYPE_SERVICE_PROVIDER_PIN: The service provider (SP) personalization key.
 * @MBIM_PIN_TYPE_CORPORATE_PIN: The corporate personalization key.
 * @MBIM_PIN_TYPE_SUBSIDY_PIN: The subsidy unlock key.
 * @MBIM_PIN_TYPE_PUK1: The Personal Identification Number1 Unlock Key (PUK1).
 * @MBIM_PIN_TYPE_PUK2: The Personal Identification Number2 Unlock Key (PUK2).
 * @MBIM_PIN_TYPE_DEVICE_FIRST_SIM_PUK: The device to very first SIM PIN unlock key.
 * @MBIM_PIN_TYPE_NETWORK_PUK: The network personalization unlock key.
 * @MBIM_PIN_TYPE_NETWORK_SUBSET_PUK: The network subset personalization unlock key.
 * @MBIM_PIN_TYPE_SERVICE_PROVIDER_PUK: The service provider (SP) personalization unlock key.
 * @MBIM_PIN_TYPE_CORPORATE_PUK: The corporate personalization unlock key.
 *
 * PIN Types.
 */
typedef enum {
    MBIM_PIN_TYPE_CUSTOM               = 1,
    MBIM_PIN_TYPE_PIN1                 = 2,
    MBIM_PIN_TYPE_PIN2                 = 3,
    MBIM_PIN_TYPE_DEVICE_SIM_PIN       = 4,
    MBIM_PIN_TYPE_DEVICE_FIRST_SIM_PIN = 5,
    MBIM_PIN_TYPE_NETWORK_PIN          = 6,
    MBIM_PIN_TYPE_NETWORK_SUBSET_PIN   = 7,
    MBIM_PIN_TYPE_SERVICE_PROVIDER_PIN = 8,
    MBIM_PIN_TYPE_CORPORATE_PIN        = 9,
    MBIM_PIN_TYPE_SUBSIDY_PIN          = 10,
    MBIM_PIN_TYPE_PUK1                 = 11,
    MBIM_PIN_TYPE_PUK2                 = 12,
    MBIM_PIN_TYPE_DEVICE_FIRST_SIM_PUK = 13,
    MBIM_PIN_TYPE_NETWORK_PUK          = 14,
    MBIM_PIN_TYPE_NETWORK_SUBSET_PUK   = 15,
    MBIM_PIN_TYPE_SERVICE_PROVIDER_PUK = 16,
    MBIM_PIN_TYPE_CORPORATE_PUK        = 17
} MbimPinType;

/**
 * MbimPinState:
 * @MBIM_PIN_STATE_UNLOCKED: The device does not require a PIN.
 * @MBIM_PIN_STATE_LOCKED: The device requires the user to enter a PIN.
 *
 * PIN States.
 */
typedef enum {
    MBIM_PIN_STATE_UNLOCKED = 0,
    MBIM_PIN_STATE_LOCKED   = 1
} MbimPinState;

/**
 * MbimPinOperation:
 * @MBIM_PIN_OPERATION_ENTER: Enter the specified PIN into the device.
 * @MBIM_PIN_OPERATION_ENABLE: Enable the specified PIN.
 * @MBIM_PIN_OPERATION_DISABLE: Disable the specified PIN.
 * @MBIM_PIN_OPERATION_CHANGE:  Change the specified PIN.
*/
typedef enum {
    MBIM_PIN_OPERATION_ENTER   = 0,
    MBIM_PIN_OPERATION_ENABLE  = 1,
    MBIM_PIN_OPERATION_DISABLE = 2,
    MBIM_PIN_OPERATION_CHANGE  = 3
} MbimPinOperation;

/*****************************************************************************/
/* 'Pin List' enums */

/**
 * MbimPinMode:
 * @MBIM_PIN_MODE_NOT_SUPPORTED: Not supported.
 * @MBIM_PIN_MODE_ENABLED: Enabled.
 * @MBIM_PIN_MODE_DISABLED: Disabled.
 *
 * Whether the lock is enabled or disabled.
 */
typedef enum {
    MBIM_PIN_MODE_NOT_SUPPORTED = 0,
    MBIM_PIN_MODE_ENABLED       = 1,
    MBIM_PIN_MODE_DISABLED      = 2
} MbimPinMode;

/**
 * MbimPinFormat:
 * @MBIM_PIN_FORMAT_UNKNOWN: Unknown format.
 * @MBIM_PIN_FORMAT_NUMERIC: Numeric-only format.
 * @MBIM_PIN_FORMAT_ALPHANUMERIC: Alphanumeric format.
 *
 * Format of the expected PIN code.
 */
typedef enum {
    MBIM_PIN_FORMAT_UNKNOWN      = 0,
    MBIM_PIN_FORMAT_NUMERIC      = 1,
    MBIM_PIN_FORMAT_ALPHANUMERIC = 2
} MbimPinFormat;

/*****************************************************************************/
/* 'Register State' enums */

/**
 * MbimNwError:
 * @MBIM_NW_ERROR_IMSI_UNKNOWN_IN_HLR: IMSI unknown in the HLR.
 * @MBIM_NW_ERROR_IMSI_UNKNOWN_IN_VLR: IMSI unknown in the VLR.
 * @MBIM_NW_ERROR_ILLEGAL_ME: Illegal ME.
 * @MBIM_NW_ERROR_GPRS_NOT_ALLOWED: GPRS not allowed.
 * @MBIM_NW_ERROR_GPRS_AND_NON_GPRS_NOT_ALLOWED: GPRS and non-GPRS not allowed.
 * @MBIM_NW_ERROR_PLMN_NOT_ALLOWED: PLMN not allowed.
 * @MBIM_NW_ERROR_LOCATION_AREA_NOT_ALLOWED: Location area not allowed.
 * @MBIM_NW_ERROR_ROAMING_NOT_ALLOWED_IN_LOCATION_AREA: Roaming not allowed in the location area.
 * @MBIM_NW_ERROR_GPRS_NOT_ALLOWED_IN_PLMN: GPRS not allowed in PLMN.
 * @MBIM_NW_ERROR_NO_CELLS_IN_LOCATION_AREA: No cells in location area.
 * @MBIM_NW_ERROR_NETWORK_FAILURE: Network failure.
 * @MBIM_NW_ERROR_CONGESTION: Congestion.
 *
 *  Network errors.
 */
typedef enum {
    MBIM_NW_ERROR_IMSI_UNKNOWN_IN_HLR                  = 2,
    MBIM_NW_ERROR_IMSI_UNKNOWN_IN_VLR                  = 4,
    MBIM_NW_ERROR_ILLEGAL_ME                           = 6,
    MBIM_NW_ERROR_GPRS_NOT_ALLOWED                     = 7,
    MBIM_NW_ERROR_GPRS_AND_NON_GPRS_NOT_ALLOWED        = 8,
    MBIM_NW_ERROR_PLMN_NOT_ALLOWED                     = 11,
    MBIM_NW_ERROR_LOCATION_AREA_NOT_ALLOWED            = 12,
    MBIM_NW_ERROR_ROAMING_NOT_ALLOWED_IN_LOCATION_AREA = 13,
    MBIM_NW_ERROR_GPRS_NOT_ALLOWED_IN_PLMN             = 14,
    MBIM_NW_ERROR_NO_CELLS_IN_LOCATION_AREA            = 15,
    MBIM_NW_ERROR_NETWORK_FAILURE                      = 17,
    MBIM_NW_ERROR_CONGESTION                           = 22
} MbimNwError;

/**
 * MbimRegisterAction:
 * @MBIM_REGISTER_ACTION_AUTOMATIC: Automatic registration.
 * @MBIM_REGISTER_ACTION_MANUAL: Manual registration.
 *
 * Type of registration requested.
 */
typedef enum {
    MBIM_REGISTER_ACTION_AUTOMATIC = 0,
    MBIM_REGISTER_ACTION_MANUAL    = 1
} MbimRegisterAction;

/**
 * MbimRegisterState:
 * @MBIM_REGISTER_STATE_UNKNOWN: Unknown registration state.
 * @MBIM_REGISTER_STATE_DEREGISTERED: Not registered.
 * @MBIM_REGISTER_STATE_SEARCHING: Searching.
 * @MBIM_REGISTER_STATE_HOME: Registered in home network.
 * @MBIM_REGISTER_STATE_ROAMING: Registered in roaming network.
 * @MBIM_REGISTER_STATE_PARTNER: Registered in a preferred roaming network.
 * @MBIM_REGISTER_STATE_DENIED: Registration denied.
 *
 * Registration state.
 */
typedef enum {
    MBIM_REGISTER_STATE_UNKNOWN      = 0,
    MBIM_REGISTER_STATE_DEREGISTERED = 1,
    MBIM_REGISTER_STATE_SEARCHING    = 2,
    MBIM_REGISTER_STATE_HOME         = 3,
    MBIM_REGISTER_STATE_ROAMING      = 4,
    MBIM_REGISTER_STATE_PARTNER      = 5,
    MBIM_REGISTER_STATE_DENIED       = 6
} MbimRegisterState;

/**
 * MbimRegisterMode:
 * @MBIM_REGISTER_MODE_UNKNOWN: Unknown.
 * @MBIM_REGISTER_MODE_AUTOMATIC: Automatic registration.
 * @MBIM_REGISTER_MODE_MANUAL: Manual registration.
 *
 * Type of registration requested.
 */
typedef enum {
    MBIM_REGISTER_MODE_UNKNOWN   = 0,
    MBIM_REGISTER_MODE_AUTOMATIC = 1,
    MBIM_REGISTER_MODE_MANUAL    = 2
} MbimRegisterMode;

/**
 * MbimRegistrationFlag:
 * @MBIM_REGISTRATION_FLAG_NONE: None.
 * @MBIM_REGISTRATION_FLAG_MANUAL_SELECTION_NOT_AVAILABLE: Network doesn't support manual network selection.
 * @MBIM_REGISTRATION_FLAG_MANUAL_PACKET_SERVICE_AUTOMATIC_ATTACH: Modem should auto-attach to the network after registration.
 *
 * Registration flags.
 */
typedef enum {
    MBIM_REGISTRATION_FLAG_NONE                                   = 0,
    MBIM_REGISTRATION_FLAG_MANUAL_SELECTION_NOT_AVAILABLE         = 1 << 0,
    MBIM_REGISTRATION_FLAG_MANUAL_PACKET_SERVICE_AUTOMATIC_ATTACH = 1 << 2,
} MbimRegistrationFlag;

/*****************************************************************************/
/* 'Packet Service' enums */

/**
 * MbimPacketServiceAction:
 * @MBIM_PACKET_SERVICE_ACTION_ATTACH: Attach.
 * @MBIM_PACKET_SERVICE_ACTION_DETACH: Detach.
 *
 * Packet Service Action.
 */
typedef enum {
    MBIM_PACKET_SERVICE_ACTION_ATTACH = 0,
    MBIM_PACKET_SERVICE_ACTION_DETACH = 1
} MbimPacketServiceAction;

/**
 * MbimPacketServiceState:
 * @MBIM_PACKET_SERVICE_STATE_UNKNOWN: Unknown.
 * @MBIM_PACKET_SERVICE_STATE_ATTACHING: Attaching.
 * @MBIM_PACKET_SERVICE_STATE_ATTACHED: Attached.
 * @MBIM_PACKET_SERVICE_STATE_DETACHING: Detaching.
 * @MBIM_PACKET_SERVICE_STATE_DETACHED: Detached.
 *
 * Packet Service State.
 */
typedef enum {
    MBIM_PACKET_SERVICE_STATE_UNKNOWN   = 0,
    MBIM_PACKET_SERVICE_STATE_ATTACHING = 1,
    MBIM_PACKET_SERVICE_STATE_ATTACHED  = 2,
    MBIM_PACKET_SERVICE_STATE_DETACHING = 3,
    MBIM_PACKET_SERVICE_STATE_DETACHED  = 4
} MbimPacketServiceState;

/*****************************************************************************/
/* 'Connect' enums */

/**
 * MbimActivationCommand:
 * @MBIM_ACTIVATION_COMMAND_DEACTIVATE: Deactivate.
 * @MBIM_ACTIVATION_COMMAND_ACTIVATE: Activate.
 *
 * Activation Command.
 */
typedef enum {
    MBIM_ACTIVATION_COMMAND_DEACTIVATE = 0,
    MBIM_ACTIVATION_COMMAND_ACTIVATE   = 1
} MbimActivationCommand;

/**
 * MbimCompression:
 * @MBIM_COMPRESSION_NONE: None.
 * @MBIM_COMPRESSION_ENABLE: Enable.
 *
 * Compression.
 */
typedef enum {
    MBIM_COMPRESSION_NONE   = 0,
    MBIM_COMPRESSION_ENABLE = 1
} MbimCompression;

/**
 * MbimAuthProtocol:
 * @MBIM_AUTH_PROTOCOL_NONE: None.
 * @MBIM_AUTH_PROTOCOL_PAP: Pap.
 * @MBIM_AUTH_PROTOCOL_CHAP: Chap.
 * @MBIM_AUTH_PROTOCOL_MSCHAPV2: V2.
 *
 * Auth Protocol.
 */
typedef enum {
    MBIM_AUTH_PROTOCOL_NONE     = 0,
    MBIM_AUTH_PROTOCOL_PAP      = 1,
    MBIM_AUTH_PROTOCOL_CHAP     = 2,
    MBIM_AUTH_PROTOCOL_MSCHAPV2 = 3
} MbimAuthProtocol;

/**
 * MbimContextIpType:
 * @MBIM_CONTEXT_IP_TYPE_DEFAULT: It is up to the function to decide, the host does not care.
 * @MBIM_CONTEXT_IP_TYPE_IPV4: IPv4 context.
 * @MBIM_CONTEXT_IP_TYPE_IPV6: IPv6 context.
 * @MBIM_CONTEXT_IP_TYPE_IPV4V6: The context is IPv4, IPv6 or dualstack IPv4v6.
 * @MBIM_CONTEXT_IP_TYPE_IPV4_AND_IPV6: Both an IPv4 and an IPv6 context.
 *
 * Context IP Type.
 */
typedef enum {
    MBIM_CONTEXT_IP_TYPE_DEFAULT       = 0,
    MBIM_CONTEXT_IP_TYPE_IPV4          = 1,
    MBIM_CONTEXT_IP_TYPE_IPV6          = 2,
    MBIM_CONTEXT_IP_TYPE_IPV4V6        = 3,
    MBIM_CONTEXT_IP_TYPE_IPV4_AND_IPV6 = 4
} MbimContextIpType;

/**
 * MbimActivationState:
 * @MBIM_ACTIVATION_STATE_UNKNOWN: Unknown.
 * @MBIM_ACTIVATION_STATE_ACTIVATED: Activated.
 * @MBIM_ACTIVATION_STATE_ACTIVATING: Activating.
 * @MBIM_ACTIVATION_STATE_DEACTIVATED: Deactivated.
 * @MBIM_ACTIVATION_STATE_DEACTIVATING: Deactivating.
 *
 * Activation State.
 */
typedef enum {
    MBIM_ACTIVATION_STATE_UNKNOWN      = 0,
    MBIM_ACTIVATION_STATE_ACTIVATED    = 1,
    MBIM_ACTIVATION_STATE_ACTIVATING   = 2,
    MBIM_ACTIVATION_STATE_DEACTIVATED  = 3,
    MBIM_ACTIVATION_STATE_DEACTIVATING = 4
} MbimActivationState;

/**
 * MbimVoiceCallState:
 * @MBIM_VOICE_CALL_STATE_NONE: None.
 * @MBIM_VOICE_CALL_STATE_IN_PROGRESS: Progress.
 * @MBIM_VOICE_CALL_STATE_HANG_UP: Up.
 *
 * Voice Call State.
 */
typedef enum {
    MBIM_VOICE_CALL_STATE_NONE        = 0,
    MBIM_VOICE_CALL_STATE_IN_PROGRESS = 1,
    MBIM_VOICE_CALL_STATE_HANG_UP     = 2
} MbimVoiceCallState;

G_END_DECLS

#endif /* _LIBMBIM_GLIB_MBIM_ENUMS_H_ */
