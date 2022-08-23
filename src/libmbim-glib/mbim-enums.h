/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * libmbim-glib -- GLib/GIO based library to control MBIM devices
 *
 * Copyright (C) 2013 - 2014 Aleksander Morgado <aleksander@aleksander.es>
 */

#ifndef _LIBMBIM_GLIB_MBIM_ENUMS_H_
#define _LIBMBIM_GLIB_MBIM_ENUMS_H_

#if !defined (__LIBMBIM_GLIB_H_INSIDE__) && !defined (LIBMBIM_GLIB_COMPILATION)
#error "Only <libmbim-glib.h> can be included directly."
#endif

G_BEGIN_DECLS

/**
 * SECTION: mbim-enums
 * @title: Enumerations and Flags
 * @short_description: Common enumeration and flag types.
 *
 * This section defines common enum and flag types used in the interface.
 */

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
 *
 * Since: 1.0
 */
typedef enum { /*< since=1.0 >*/
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
 *
 * Since: 1.0
 */
typedef enum { /*< since=1.0 >*/
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
 *
 * Since: 1.0
 */
typedef enum { /*< since=1.0 >*/
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
 *
 * Since: 1.0
 */
typedef enum { /*< since=1.0 >*/
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
 * @MBIM_DATA_CLASS_5G_NSA: 5G NSA (MS MBIMEx v2.0). Since 1.28.
 * @MBIM_DATA_CLASS_5G_SA: 5G SA (MS MBIMEx v2.0). Since 1.28.
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
 *
 * Both 5G related values are introduced in MBIM Extension v2.0, but given
 * the update is compatible with the original MBIM enumeration, devices may
 * report them without having enabled MBIM Extension v2.0 support.
 *
 * This type is updated in MBIM Extension v3.0 in a non-backwards compatible
 * way, see #MbimDataClassV3.
 *
 * Since: 1.0
 */
typedef enum { /*< since=1.0 >*/
    MBIM_DATA_CLASS_GPRS        = 1 << 0,
    MBIM_DATA_CLASS_EDGE        = 1 << 1,
    MBIM_DATA_CLASS_UMTS        = 1 << 2,
    MBIM_DATA_CLASS_HSDPA       = 1 << 3,
    MBIM_DATA_CLASS_HSUPA       = 1 << 4,
    MBIM_DATA_CLASS_LTE         = 1 << 5,
    MBIM_DATA_CLASS_5G_NSA      = 1 << 6,
    MBIM_DATA_CLASS_5G_SA       = 1 << 7,
    /* Bits 8 to 15 reserved for future 3GPP classes */
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
 *
 * Since: 1.0
 */
typedef enum { /*< since=1.0 >*/
    MBIM_SMS_CAPS_PDU_RECEIVE  = 1 << 0,
    MBIM_SMS_CAPS_PDU_SEND     = 1 << 1,
    MBIM_SMS_CAPS_TEXT_RECEIVE = 1 << 2,
    MBIM_SMS_CAPS_TEXT_SEND    = 1 << 3
} MbimSmsCaps;

/**
 * MbimCtrlCaps:
 * @MBIM_CTRL_CAPS_NONE: None. Since 1.28.
 * @MBIM_CTRL_CAPS_REG_MANUAL: Device allows manual network selection.
 * @MBIM_CTRL_CAPS_HW_RADIO_SWITCH: Device has a hardware radio power switch.
 * @MBIM_CTRL_CAPS_CDMA_MOBILE_IP: The CDMA function supports Mobile IP.
 * @MBIM_CTRL_CAPS_CDMA_SIMPLE_IP: The CDMA function supports Simple IP.
 * @MBIM_CTRL_CAPS_MULTI_CARRIER: Device can work with multiple providers.
 * @MBIM_CTRL_CAPS_ESIM: Device supports eSIM (MS MBIMEx v3.0). Since 1.28.
 * @MBIM_CTRL_CAPS_UE_POLICY_ROUTE_SELECTION: Device supports including the route selection descriptors as part of the UE policies (MS MBIMEx v3.0). Since 1.28.
 * @MBIM_CTRL_CAPS_SIM_HOT_SWAP_CAPABLE: Device supports SIM hot-swap (MS MBIMEx v3.0). Since 1.28.
 *
 * Control capabilities.
 *
 * Since: 1.0
 */
typedef enum { /*< since=1.0 >*/
    MBIM_CTRL_CAPS_NONE                      = 0,
    MBIM_CTRL_CAPS_REG_MANUAL                = 1 << 0,
    MBIM_CTRL_CAPS_HW_RADIO_SWITCH           = 1 << 1,
    MBIM_CTRL_CAPS_CDMA_MOBILE_IP            = 1 << 2,
    MBIM_CTRL_CAPS_CDMA_SIMPLE_IP            = 1 << 3,
    MBIM_CTRL_CAPS_MULTI_CARRIER             = 1 << 4,
    MBIM_CTRL_CAPS_ESIM                      = 1 << 5,
    MBIM_CTRL_CAPS_UE_POLICY_ROUTE_SELECTION = 1 << 6,
    MBIM_CTRL_CAPS_SIM_HOT_SWAP_CAPABLE      = 1 << 7,
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
 * @MBIM_SUBSCRIBER_READY_STATE_NO_ESIM_PROFILE: The card is ready but does not
 *  have any enabled profiles (MS MBIMEx). Since 1.28.
 *
 * Ready state of the subscriber.
 *
 * Since: 1.0
 */
typedef enum { /*< since=1.0 >*/
    MBIM_SUBSCRIBER_READY_STATE_NOT_INITIALIZED  = 0,
    MBIM_SUBSCRIBER_READY_STATE_INITIALIZED      = 1,
    MBIM_SUBSCRIBER_READY_STATE_SIM_NOT_INSERTED = 2,
    MBIM_SUBSCRIBER_READY_STATE_BAD_SIM          = 3,
    MBIM_SUBSCRIBER_READY_STATE_FAILURE          = 4,
    MBIM_SUBSCRIBER_READY_STATE_NOT_ACTIVATED    = 5,
    MBIM_SUBSCRIBER_READY_STATE_DEVICE_LOCKED    = 6,
    MBIM_SUBSCRIBER_READY_STATE_NO_ESIM_PROFILE  = 7,
} MbimSubscriberReadyState;

/**
 * MbimReadyInfoFlag:
 * @MBIM_READY_INFO_FLAG_NONE: Device in normal mode. Since 1.16.
 * @MBIM_READY_INFO_FLAG_PROTECT_UNIQUE_ID: Request to avoid displaying subscriber ID.
 *
 * Flag specifying how the ready info is treated.
 *
 * Since: 1.0
 */
typedef enum { /*< since=1.0 >*/
    MBIM_READY_INFO_FLAG_NONE              = 0,
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
 *
 * Since: 1.0
 */
typedef enum { /*< since=1.0 >*/
    MBIM_RADIO_SWITCH_STATE_OFF = 0,
    MBIM_RADIO_SWITCH_STATE_ON  = 1
} MbimRadioSwitchState;

/*****************************************************************************/
/* 'Pin' enums */

/**
 * MbimPinType:
 * @MBIM_PIN_TYPE_UNKNOWN: Unknown or unset.
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
 * @MBIM_PIN_TYPE_NEV: The NEV key (MS UICC low-level access). Since 1.28.
 * @MBIM_PIN_TYPE_ADM: The administrative key (MS UICC low-level access). Since 1.28.
 *
 * PIN Types.
 *
 * Since: 1.0
 */
typedef enum { /*< since=1.0 >*/
    MBIM_PIN_TYPE_UNKNOWN              = 0,
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
    MBIM_PIN_TYPE_CORPORATE_PUK        = 17,
    MBIM_PIN_TYPE_NEV                  = 18,
    MBIM_PIN_TYPE_ADM                  = 19,
} MbimPinType;

/**
 * MbimPinState:
 * @MBIM_PIN_STATE_UNLOCKED: The device does not require a PIN.
 * @MBIM_PIN_STATE_LOCKED: The device requires the user to enter a PIN.
 *
 * PIN States.
 *
 * Since: 1.0
 */
typedef enum { /*< since=1.0 >*/
    MBIM_PIN_STATE_UNLOCKED = 0,
    MBIM_PIN_STATE_LOCKED   = 1
} MbimPinState;

/**
 * MbimPinOperation:
 * @MBIM_PIN_OPERATION_ENTER: Enter the specified PIN into the device.
 * @MBIM_PIN_OPERATION_ENABLE: Enable the specified PIN.
 * @MBIM_PIN_OPERATION_DISABLE: Disable the specified PIN.
 * @MBIM_PIN_OPERATION_CHANGE:  Change the specified PIN.
 *
 * Operation to perform on the PIN.
 *
 * Since: 1.0
 */
typedef enum { /*< since=1.0 >*/
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
 *
 * Since: 1.0
 */
typedef enum { /*< since=1.0 >*/
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
 *
 * Since: 1.0
 */
typedef enum { /*< since=1.0 >*/
    MBIM_PIN_FORMAT_UNKNOWN      = 0,
    MBIM_PIN_FORMAT_NUMERIC      = 1,
    MBIM_PIN_FORMAT_ALPHANUMERIC = 2
} MbimPinFormat;

/*****************************************************************************/
/* 'Home Provider' enums */

/**
 * MbimProviderState:
 * @MBIM_PROVIDER_STATE_UNKNOWN: Unknown.
 * @MBIM_PROVIDER_STATE_HOME: Home operator.
 * @MBIM_PROVIDER_STATE_FORBIDDEN: Provider blocked.
 * @MBIM_PROVIDER_STATE_PREFERRED: Provider is in the preferred list.
 * @MBIM_PROVIDER_STATE_VISIBLE: Provider is visible.
 * @MBIM_PROVIDER_STATE_REGISTERED: Currently registered to the provider.
 * @MBIM_PROVIDER_STATE_PREFERRED_MULTICARRIER: Provider is a preferred multicarrier network.
 *
 * State of the provider.
 *
 * Since: 1.2
 */
typedef enum { /*< since=1.2 >*/
    MBIM_PROVIDER_STATE_UNKNOWN                = 0,
    MBIM_PROVIDER_STATE_HOME                   = 1 << 0,
    MBIM_PROVIDER_STATE_FORBIDDEN              = 1 << 1,
    MBIM_PROVIDER_STATE_PREFERRED              = 1 << 2,
    MBIM_PROVIDER_STATE_VISIBLE                = 1 << 3,
    MBIM_PROVIDER_STATE_REGISTERED             = 1 << 4,
    MBIM_PROVIDER_STATE_PREFERRED_MULTICARRIER = 1 << 5
} MbimProviderState;

/*****************************************************************************/
/* Connect V3' enums */

/**
 * MbimAccessMediaType:
 * @MBIM_ACCESS_MEDIA_TYPE_UNKNOWN: None, or unknown.
 * @MBIM_ACCESS_MEDIA_TYPE_3GPP: 3GPP only.
 * @MBIM_ACCESS_MEDIA_TYPE_3GPP_PREFERRED: 3GPP Preferred.
 *
 * Access type preference.
 *
 * Since: 1.28
 */
typedef enum { /*< since=1.28 >*/
    MBIM_ACCESS_MEDIA_TYPE_UNKNOWN        = 0,
    MBIM_ACCESS_MEDIA_TYPE_3GPP           = 1,
    MBIM_ACCESS_MEDIA_TYPE_3GPP_PREFERRED = 2,
} MbimAccessMediaType;

/*****************************************************************************/
/* 'Visible Providers' enums */

/**
 * MbimVisibleProvidersAction:
 * @MBIM_VISIBLE_PROVIDERS_ACTION_FULL_SCAN: Full scan.
 * @MBIM_VISIBLE_PROVIDERS_ACTION_RESTRICTED_SCAN: Locate preferred multicarrier providers.
 *
 * Type of action to perform when listing visible providers.
 *
 * Since: 1.2
 */
typedef enum { /*< since=1.2 >*/
    MBIM_VISIBLE_PROVIDERS_ACTION_FULL_SCAN       = 0,
    MBIM_VISIBLE_PROVIDERS_ACTION_RESTRICTED_SCAN = 1
} MbimVisibleProvidersAction;

/*****************************************************************************/
/* 'Register State' enums */

/**
 * MbimNwError:
 * @MBIM_NW_ERROR_NONE: No error. Since 1.28.
 * @MBIM_NW_ERROR_IMSI_UNKNOWN_IN_HLR: IMSI unknown in the HLR.
 * @MBIM_NW_ERROR_ILLEGAL_MS: Illegal MS. Since 1.10.
 * @MBIM_NW_ERROR_IMSI_UNKNOWN_IN_VLR: IMSI unknown in the VLR.
 * @MBIM_NW_ERROR_IMEI_NOT_ACCEPTED: IMEI not accepted. Since 1.10.
 * @MBIM_NW_ERROR_ILLEGAL_ME: Illegal ME.
 * @MBIM_NW_ERROR_GPRS_NOT_ALLOWED: GPRS not allowed.
 * @MBIM_NW_ERROR_GPRS_AND_NON_GPRS_NOT_ALLOWED: GPRS and non-GPRS not allowed.
 * @MBIM_NW_ERROR_MS_IDENTITY_NOT_DERIVED_BY_NETWORK: MS identity cannot be derived by the network. Since 1.10.
 * @MBIM_NW_ERROR_IMPLICITLY_DETACHED: Implicitly detached. Since 1.10.
 * @MBIM_NW_ERROR_PLMN_NOT_ALLOWED: PLMN not allowed.
 * @MBIM_NW_ERROR_LOCATION_AREA_NOT_ALLOWED: Location area not allowed.
 * @MBIM_NW_ERROR_ROAMING_NOT_ALLOWED_IN_LOCATION_AREA: Roaming not allowed in the location area.
 * @MBIM_NW_ERROR_GPRS_NOT_ALLOWED_IN_PLMN: GPRS not allowed in PLMN.
 * @MBIM_NW_ERROR_NO_CELLS_IN_LOCATION_AREA: No cells in location area.
 * @MBIM_NW_ERROR_MSC_TEMPORARILY_NOT_REACHABLE: MSC temporarily not reachable. Since 1.10.
 * @MBIM_NW_ERROR_NETWORK_FAILURE: Network failure.
 * @MBIM_NW_ERROR_MAC_FAILURE: MAC failure. Since 1.10.
 * @MBIM_NW_ERROR_SYNCH_FAILURE: Synch failure. Since 1.10.
 * @MBIM_NW_ERROR_CONGESTION: Congestion.
 * @MBIM_NW_ERROR_GSM_AUTHENTICATION_UNACCEPTABLE: GSM authentication unacceptable. Since 1.10.
 * @MBIM_NW_ERROR_NOT_AUTHORIZED_FOR_CSG: Not authorized for this CSG. Since 1.10.
 * @MBIM_NW_ERROR_INSUFFICIENT_RESOURCES: Insufficient resources. Since 1.18.
 * @MBIM_NW_ERROR_MISSING_OR_UNKNOWN_APN: Missing or unknown access point name. Since 1.10.
 * @MBIM_NW_ERROR_UNKNOWN_PDP_ADDRESS_OR_TYPE: Unknown PDP address or PDP type. Since 1.18.
 * @MBIM_NW_ERROR_USER_AUTHENTICATION_FAILED: User authentication failed. Since 1.18.
 * @MBIM_NW_ERROR_ACTIVATION_REJECTED_BY_GGSN_OR_GW: Activation rejected by GGSN, Serving GW or PDN GW. Since 1.18.
 * @MBIM_NW_ERROR_ACTIVATION_REJECTED_UNSPECIFIED: Activation rejected, unspecified. Since 1.18.
 * @MBIM_NW_ERROR_SERVICE_OPTION_NOT_SUPPORTED: Service option not supported. Since 1.10.
 * @MBIM_NW_ERROR_REQUESTED_SERVICE_OPTION_NOT_SUBSCRIBED: Requested service option not subscribed. Since 1.10.
 * @MBIM_NW_ERROR_SERVICE_OPTION_TEMPORARILY_OUT_OF_ORDER: Service option temporarily out of order. Since 1.10.
 * @MBIM_NW_ERROR_NO_PDP_CONTEXT_ACTIVATED: No PDP context activated. Since 1.10.
 * @MBIM_NW_ERROR_SEMANTIC_ERROR_IN_THE_TFT_OPERATION: Semantic error in the TFT operation. Since 1.28.
 * @MBIM_NW_ERROR_SYNTACTICAL_ERROR_IN_THE_TFT_OPERATION: Syntactical error in the TFT operation. Since 1.28.
 * @MBIM_NW_ERROR_UNKNOWN_PDP_CONTEXT: Unknown PDP context. Since 1.28.
 * @MBIM_NW_ERROR_SEMANTIC_ERRORS_IN_PACKET_FILTER: Semantic errors in packet filter. Since 1.28.
 * @MBIM_NW_ERROR_SYNTACTICAL_ERRORS_IN_PACKET_FILTER: Syntactical errors in packet filter. Since 1.28.
 * @MBIM_NW_ERROR_PDP_CONTEXT_WITHOUT_TFT_ALREADY_ACTIVATED: PDP context without TFT already activated. Since 1.28.
 * @MBIM_NW_ERROR_REQUEST_REJECTED_OR_BEARER_CONTROL_MODE_VIOLATION: Request rejected, Bearer Control Mode violation. Since 1.28.
 * @MBIM_NW_ERROR_LAST_PDN_DISCONNECTION_NOT_ALLOWED: Last PDN disconnection not allowed. Since 1.28.
 * @MBIM_NW_ERROR_PDP_TYPE_IPV4_ONLY_ALLOWED: PDP type IPv4 only allowed. Since 1.18.
 * @MBIM_NW_ERROR_PDP_TYPE_IPV6_ONLY_ALLOWED: PDP type IPv6 only allowed. Since 1.18.
 * @MBIM_NW_ERROR_NO_NETWORK_SLICES_AVAILABLE: No network slices available  Since 1.28.
 * @MBIM_NW_ERROR_MAXIMUM_NUMBER_OF_PDP_CONTEXTS_REACHED: Maximum number of PDP contexts reached. Since 1.18.
 * @MBIM_NW_ERROR_REQUESTED_APN_NOT_SUPPORTED_IN_CURRENT_RAT_AND_PLMN: Requested APN not supported in current RAT and PLMN combination. Since 1.18.
 * @MBIM_NW_ERROR_INSUFFICIENT_RESOURCES_FOR_SPECIFIC_SLICE_AND_DNN: Insufficient resources for specific slice and DNN. Since 1.28.
 * @MBIM_NW_ERROR_INSUFFICIENT_RESOURCES_FOR_SPECIFIC_SLICE: Insufficient resources for specific slice. Since 1.28.
 * @MBIM_NW_ERROR_NGKSI_ALREADY_IN_USE: NgKSI already in use. Since 1.28.
 * @MBIM_NW_ERROR_NON_3GPP_ACCESS_TO_5GCN_NOT_ALLOWED: Non-3GPP access to 5GCN not allowe. Since 1.28.
 * @MBIM_NW_ERROR_SERVING_NETWORK_NOT_AUTHORIZED: Serving network not authorized. Since 1.28.
 * @MBIM_NW_ERROR_TEMPORARILY_NOT_AUTHORIZED_FOR_THIS_SNPN: Temporarily not authorized for this SNPN. Since 1.28.
 * @MBIM_NW_ERROR_PERMANENTLY_NOT_AUTHORIZED_FOR_THIS_SNPN: Permanently not authorized for this SNPN. Since 1.28.
 * @MBIM_NW_ERROR_NOT_AUTHORIZED_FOR_THIS_CAG_OR_AUTHORIZED_FOR_CAG_CELLS_ONLY: Not authorized for this CAG or authorized for CAG cells. Since 1.28.
 * @MBIM_NW_ERROR_WIRELINE_ACCESS_AREA_NOT_ALLOWED: Wireline access area not allowed. Since 1.28.
 * @MBIM_NW_ERROR_PAYLOAD_WAS_NOT_FORWARDED: Payload was not forwarded. Since 1.28.
 * @MBIM_NW_ERROR_DNN_NOT_SUPPORTED_OR_NOT_SUBSCRIBED_IN_THE_SLICE: DNN not supported or not subscribed in the slice. Since 1.28.
 * @MBIM_NW_ERROR_INSUFFICIENT_USER_PLANE_RESOURCES_FOR_THE_PDU_SESSION: Insufficient user-plane resources for the PDU session. Since 1.28.
 * @MBIM_NW_ERROR_SEMANTICALLY_INCORRECT_MESSAGE: Semantically incorrect message. Since 1.10.
 * @MBIM_NW_ERROR_INVALID_MANDATORY_INFORMATION: Invalid mandatory information. Since 1.10.
 * @MBIM_NW_ERROR_MESSAGE_TYPE_NON_EXISTENT_OR_NOT_IMPLEMENTED: Message type non-existent or not implemented. Since 1.10.
 * @MBIM_NW_ERROR_MESSAGE_TYPE_NOT_COMPATIBLE_WITH_PROTOCOL_STATE: Message type not compatible with protocol state. Since 1.10.
 * @MBIM_NW_ERROR_INFORMATION_ELEMENT_NON_EXISTENT_OR_NOT_IMPLEMENTED: Information element non-existent or not implemented. Since 1.10.
 * @MBIM_NW_ERROR_CONDITIONAL_IE_ERROR: Conditional IE error. Since 1.10.
 * @MBIM_NW_ERROR_MESSAGE_NOT_COMPATIBLE_WITH_PROTOCOL_STATE: Message not compatible with protocol state. Since 1.10.
 * @MBIM_NW_ERROR_PROTOCOL_ERROR_UNSPECIFIED: Protocol error, unspecified. Since 1.10.
 * @MBIM_NW_ERROR_APN_RESTRICTION_VALUE_INCOMPATIBLE_WITH_ACTIVE_PDP_CONTEXT: APN restriction value incompatible with active PDP context. Since 1.18.
 * @MBIM_NW_ERROR_MULTIPLE_ACCESSES_TO_A_PDN_CONNECTION_NOT_ALLOWED: Multiple accesses to a PDN connection not allowed. Since 1.18.
 *
 * Network errors.
 *
 * Since: 1.0
 */
typedef enum { /*< since=1.0 >*/
    MBIM_NW_ERROR_NONE                                                         = 0,
    MBIM_NW_ERROR_IMSI_UNKNOWN_IN_HLR                                          = 2,
    MBIM_NW_ERROR_ILLEGAL_MS                                                   = 3,
    MBIM_NW_ERROR_IMSI_UNKNOWN_IN_VLR                                          = 4,
    MBIM_NW_ERROR_IMEI_NOT_ACCEPTED                                            = 5,
    MBIM_NW_ERROR_ILLEGAL_ME                                                   = 6,
    MBIM_NW_ERROR_GPRS_NOT_ALLOWED                                             = 7,
    MBIM_NW_ERROR_GPRS_AND_NON_GPRS_NOT_ALLOWED                                = 8,
    MBIM_NW_ERROR_MS_IDENTITY_NOT_DERIVED_BY_NETWORK                           = 9,
    MBIM_NW_ERROR_IMPLICITLY_DETACHED                                          = 10,
    MBIM_NW_ERROR_PLMN_NOT_ALLOWED                                             = 11,
    MBIM_NW_ERROR_LOCATION_AREA_NOT_ALLOWED                                    = 12,
    MBIM_NW_ERROR_ROAMING_NOT_ALLOWED_IN_LOCATION_AREA                         = 13,
    MBIM_NW_ERROR_GPRS_NOT_ALLOWED_IN_PLMN                                     = 14,
    MBIM_NW_ERROR_NO_CELLS_IN_LOCATION_AREA                                    = 15,
    MBIM_NW_ERROR_MSC_TEMPORARILY_NOT_REACHABLE                                = 16,
    MBIM_NW_ERROR_NETWORK_FAILURE                                              = 17,
    MBIM_NW_ERROR_MAC_FAILURE                                                  = 20,
    MBIM_NW_ERROR_SYNCH_FAILURE                                                = 21,
    MBIM_NW_ERROR_CONGESTION                                                   = 22,
    MBIM_NW_ERROR_GSM_AUTHENTICATION_UNACCEPTABLE                              = 23,
    MBIM_NW_ERROR_NOT_AUTHORIZED_FOR_CSG                                       = 25,
    MBIM_NW_ERROR_INSUFFICIENT_RESOURCES                                       = 26,
    MBIM_NW_ERROR_MISSING_OR_UNKNOWN_APN                                       = 27,
    MBIM_NW_ERROR_UNKNOWN_PDP_ADDRESS_OR_TYPE                                  = 28,
    MBIM_NW_ERROR_USER_AUTHENTICATION_FAILED                                   = 29,
    MBIM_NW_ERROR_ACTIVATION_REJECTED_BY_GGSN_OR_GW                            = 30,
    MBIM_NW_ERROR_ACTIVATION_REJECTED_UNSPECIFIED                              = 31,
    MBIM_NW_ERROR_SERVICE_OPTION_NOT_SUPPORTED                                 = 32,
    MBIM_NW_ERROR_REQUESTED_SERVICE_OPTION_NOT_SUBSCRIBED                      = 33,
    MBIM_NW_ERROR_SERVICE_OPTION_TEMPORARILY_OUT_OF_ORDER                      = 34,
    MBIM_NW_ERROR_NO_PDP_CONTEXT_ACTIVATED                                     = 40,
    MBIM_NW_ERROR_SEMANTIC_ERROR_IN_THE_TFT_OPERATION                          = 41,
    MBIM_NW_ERROR_SYNTACTICAL_ERROR_IN_THE_TFT_OPERATION                       = 42,
    MBIM_NW_ERROR_UNKNOWN_PDP_CONTEXT                                          = 43,
    MBIM_NW_ERROR_SEMANTIC_ERRORS_IN_PACKET_FILTER                             = 44,
    MBIM_NW_ERROR_SYNTACTICAL_ERRORS_IN_PACKET_FILTER                          = 45,
    MBIM_NW_ERROR_PDP_CONTEXT_WITHOUT_TFT_ALREADY_ACTIVATED                    = 46,
    MBIM_NW_ERROR_REQUEST_REJECTED_OR_BEARER_CONTROL_MODE_VIOLATION            = 48,
    MBIM_NW_ERROR_LAST_PDN_DISCONNECTION_NOT_ALLOWED                           = 49,
    MBIM_NW_ERROR_PDP_TYPE_IPV4_ONLY_ALLOWED                                   = 50,
    MBIM_NW_ERROR_PDP_TYPE_IPV6_ONLY_ALLOWED                                   = 51,
    MBIM_NW_ERROR_NO_NETWORK_SLICES_AVAILABLE                                  = 62,
    MBIM_NW_ERROR_MAXIMUM_NUMBER_OF_PDP_CONTEXTS_REACHED                       = 65,
    MBIM_NW_ERROR_REQUESTED_APN_NOT_SUPPORTED_IN_CURRENT_RAT_AND_PLMN          = 66,
    MBIM_NW_ERROR_INSUFFICIENT_RESOURCES_FOR_SPECIFIC_SLICE_AND_DNN            = 67,
    MBIM_NW_ERROR_INSUFFICIENT_RESOURCES_FOR_SPECIFIC_SLICE                    = 69,
    MBIM_NW_ERROR_NGKSI_ALREADY_IN_USE                                         = 71,
    MBIM_NW_ERROR_NON_3GPP_ACCESS_TO_5GCN_NOT_ALLOWED                          = 72,
    MBIM_NW_ERROR_SERVING_NETWORK_NOT_AUTHORIZED                               = 73,
    MBIM_NW_ERROR_TEMPORARILY_NOT_AUTHORIZED_FOR_THIS_SNPN                     = 74,
    MBIM_NW_ERROR_PERMANENTLY_NOT_AUTHORIZED_FOR_THIS_SNPN                     = 75,
    MBIM_NW_ERROR_NOT_AUTHORIZED_FOR_THIS_CAG_OR_AUTHORIZED_FOR_CAG_CELLS_ONLY = 76,
    MBIM_NW_ERROR_WIRELINE_ACCESS_AREA_NOT_ALLOWED                             = 77,
    MBIM_NW_ERROR_PAYLOAD_WAS_NOT_FORWARDED                                    = 90,
    MBIM_NW_ERROR_DNN_NOT_SUPPORTED_OR_NOT_SUBSCRIBED_IN_THE_SLICE             = 91,
    MBIM_NW_ERROR_INSUFFICIENT_USER_PLANE_RESOURCES_FOR_THE_PDU_SESSION        = 92,
    MBIM_NW_ERROR_SEMANTICALLY_INCORRECT_MESSAGE                               = 95,
    MBIM_NW_ERROR_INVALID_MANDATORY_INFORMATION                                = 96,
    MBIM_NW_ERROR_MESSAGE_TYPE_NON_EXISTENT_OR_NOT_IMPLEMENTED                 = 97,
    MBIM_NW_ERROR_MESSAGE_TYPE_NOT_COMPATIBLE_WITH_PROTOCOL_STATE              = 98,
    MBIM_NW_ERROR_INFORMATION_ELEMENT_NON_EXISTENT_OR_NOT_IMPLEMENTED          = 99,
    MBIM_NW_ERROR_CONDITIONAL_IE_ERROR                                         = 100,
    MBIM_NW_ERROR_MESSAGE_NOT_COMPATIBLE_WITH_PROTOCOL_STATE                   = 101,
    MBIM_NW_ERROR_PROTOCOL_ERROR_UNSPECIFIED                                   = 111,
    MBIM_NW_ERROR_APN_RESTRICTION_VALUE_INCOMPATIBLE_WITH_ACTIVE_PDP_CONTEXT   = 112,
    MBIM_NW_ERROR_MULTIPLE_ACCESSES_TO_A_PDN_CONNECTION_NOT_ALLOWED            = 113
} MbimNwError;

/**
 * MbimRegisterAction:
 * @MBIM_REGISTER_ACTION_AUTOMATIC: Automatic registration.
 * @MBIM_REGISTER_ACTION_MANUAL: Manual registration.
 *
 * Type of registration requested.
 *
 * Since: 1.0
 */
typedef enum { /*< since=1.0 >*/
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
 *
 * Since: 1.0
 */
typedef enum { /*< since=1.0 >*/
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
 *
 * Since: 1.0
 */
typedef enum { /*< since=1.0 >*/
    MBIM_REGISTER_MODE_UNKNOWN   = 0,
    MBIM_REGISTER_MODE_AUTOMATIC = 1,
    MBIM_REGISTER_MODE_MANUAL    = 2
} MbimRegisterMode;

/**
 * MbimRegistrationFlag:
 * @MBIM_REGISTRATION_FLAG_NONE: None.
 * @MBIM_REGISTRATION_FLAG_MANUAL_SELECTION_NOT_AVAILABLE: Network doesn't support manual network selection.
 * @MBIM_REGISTRATION_FLAG_PACKET_SERVICE_AUTOMATIC_ATTACH: Modem should auto-attach to the network after registration. Since 1.8.
 *
 * Registration flags.
 *
 * Since: 1.0
 */
typedef enum { /*< since=1.0 >*/
    MBIM_REGISTRATION_FLAG_NONE                            = 0,
    MBIM_REGISTRATION_FLAG_MANUAL_SELECTION_NOT_AVAILABLE  = 1 << 0,
    MBIM_REGISTRATION_FLAG_PACKET_SERVICE_AUTOMATIC_ATTACH = 1 << 1,
} MbimRegistrationFlag;

/*****************************************************************************/
/* 'Wake Reason' enums */
/**
 * MbimWakeType:
 * @MBIM_WAKE_TYPE_CID_RESPONSE: A CID response wakes device.
 * @MBIM_WAKE_TYPE_CID_INDICATION: A CID indication wakes device.
 * @MBIM_WAKE_TYPE_PACKET: An incoming packet wakes device.
 *
 * Wake type.
 *
 * Since: 1.28
 */
typedef enum { /*< since=1.28 >*/
    MBIM_WAKE_TYPE_CID_RESPONSE   = 0,
    MBIM_WAKE_TYPE_CID_INDICATION = 1,
    MBIM_WAKE_TYPE_PACKET         = 2,
} MbimWakeType;

/*****************************************************************************/
/* 'Packet Service' enums */

/**
 * MbimPacketServiceAction:
 * @MBIM_PACKET_SERVICE_ACTION_ATTACH: Attach.
 * @MBIM_PACKET_SERVICE_ACTION_DETACH: Detach.
 *
 * Packet Service Action.
 *
 * Since: 1.0
 */
typedef enum { /*< since=1.0 >*/
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
 *
 * Since: 1.0
 */
typedef enum { /*< since=1.0 >*/
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
 *
 * Since: 1.0
 */
typedef enum { /*< since=1.0 >*/
    MBIM_ACTIVATION_COMMAND_DEACTIVATE = 0,
    MBIM_ACTIVATION_COMMAND_ACTIVATE   = 1
} MbimActivationCommand;

/**
 * MbimCompression:
 * @MBIM_COMPRESSION_NONE: None.
 * @MBIM_COMPRESSION_ENABLE: Enable.
 *
 * Compression.
 *
 * Since: 1.0
 */
typedef enum { /*< since=1.0 >*/
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
 *
 * Since: 1.0
 */
typedef enum { /*< since=1.0 >*/
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
 *
 * Since: 1.0
 */
typedef enum { /*< since=1.0 >*/
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
 *
 * Since: 1.0
 */
typedef enum { /*< since=1.0 >*/
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
 *
 * Since: 1.0
 */
typedef enum { /*< since=1.0 >*/
    MBIM_VOICE_CALL_STATE_NONE        = 0,
    MBIM_VOICE_CALL_STATE_IN_PROGRESS = 1,
    MBIM_VOICE_CALL_STATE_HANG_UP     = 2
} MbimVoiceCallState;

/*****************************************************************************/
/* 'IP Configuration' enums */

/**
 * MbimIPConfigurationAvailableFlag:
 * @MBIM_IP_CONFIGURATION_AVAILABLE_FLAG_NONE: No info available.
 * @MBIM_IP_CONFIGURATION_AVAILABLE_FLAG_ADDRESS: Address info available.
 * @MBIM_IP_CONFIGURATION_AVAILABLE_FLAG_GATEWAY: Gateway info available.
 * @MBIM_IP_CONFIGURATION_AVAILABLE_FLAG_DNS: DNS info available.
 * @MBIM_IP_CONFIGURATION_AVAILABLE_FLAG_MTU: MTU info available.
 *
 * Mask of available information about an IP address.
 *
 * Since: 1.0
 */
typedef enum { /*< underscore_name=mbim_ip_configuration_available_flag,since=1.0 >*/
    MBIM_IP_CONFIGURATION_AVAILABLE_FLAG_NONE     = 0,
    MBIM_IP_CONFIGURATION_AVAILABLE_FLAG_ADDRESS  = 1 << 0,
    MBIM_IP_CONFIGURATION_AVAILABLE_FLAG_GATEWAY  = 1 << 1,
    MBIM_IP_CONFIGURATION_AVAILABLE_FLAG_DNS      = 1 << 2,
    MBIM_IP_CONFIGURATION_AVAILABLE_FLAG_MTU      = 1 << 3,
} MbimIPConfigurationAvailableFlag;

/*****************************************************************************/
/* 'SMS Configuration' enums */

/**
 * MbimSmsStorageState:
 * @MBIM_SMS_STORAGE_STATE_NOT_INITIALIZED: Storage not initialized.
 * @MBIM_SMS_STORAGE_STATE_INITIALIZED: Storage initialized.
 *
 * State of the SMS storage.
 *
 * Since: 1.4
 */
typedef enum { /*< since=1.4 >*/
    MBIM_SMS_STORAGE_STATE_NOT_INITIALIZED = 0,
    MBIM_SMS_STORAGE_STATE_INITIALIZED     = 1
} MbimSmsStorageState;

/**
 * MbimSmsFormat:
 * @MBIM_SMS_FORMAT_PDU: PDU format.
 * @MBIM_SMS_FORMAT_CDMA: CDMA format.
 *
 * SMS format.
 *
 * Since: 1.4
 */
typedef enum { /*< since=1.4 >*/
    MBIM_SMS_FORMAT_PDU  = 0,
    MBIM_SMS_FORMAT_CDMA = 1
} MbimSmsFormat;

/*****************************************************************************/
/* 'SMS Read' enums */

/**
 * MbimSmsFlag:
 * @MBIM_SMS_FLAG_ALL: All.
 * @MBIM_SMS_FLAG_INDEX: Index.
 * @MBIM_SMS_FLAG_NEW: New.
 * @MBIM_SMS_FLAG_OLD: Old.
 * @MBIM_SMS_FLAG_SENT: Sent.
 * @MBIM_SMS_FLAG_DRAFT: Draft.
 *
 * Flags to use when requesting to read SMS. @MBIM_SMS_FLAG_ALL and
 * @MBIM_SMS_FLAG_NEW are mandatory, all the others are optional.
 *
 * Since: 1.4
*/
typedef enum { /*< since=1.4 >*/
    MBIM_SMS_FLAG_ALL   = 0,
    MBIM_SMS_FLAG_INDEX = 1,
    MBIM_SMS_FLAG_NEW   = 2,
    MBIM_SMS_FLAG_OLD   = 3,
    MBIM_SMS_FLAG_SENT  = 4,
    MBIM_SMS_FLAG_DRAFT = 5
} MbimSmsFlag;

/**
 * MbimSmsCdmaLang:
 * @MBIM_SMS_CDMA_LANG_UNKNOWN: Unknown language.
 * @MBIM_SMS_CDMA_LANG_ENGLISH: English.
 * @MBIM_SMS_CDMA_LANG_FRENCH: French.
 * @MBIM_SMS_CDMA_LANG_SPANISH: Spanish.
 * @MBIM_SMS_CDMA_LANG_JAPANESE: Japanese.
 * @MBIM_SMS_CDMA_LANG_KOREAN: Korean.
 * @MBIM_SMS_CDMA_LANG_CHINESE: Chinese.
 * @MBIM_SMS_CDMA_LANG_HEBREW: Hebrew.
 *
 * Language of a CDMA SMS.
 *
 * Since: 1.4
 */
typedef enum { /*< since=1.4 >*/
    MBIM_SMS_CDMA_LANG_UNKNOWN  = 0,
    MBIM_SMS_CDMA_LANG_ENGLISH  = 1,
    MBIM_SMS_CDMA_LANG_FRENCH   = 2,
    MBIM_SMS_CDMA_LANG_SPANISH  = 3,
    MBIM_SMS_CDMA_LANG_JAPANESE = 4,
    MBIM_SMS_CDMA_LANG_KOREAN   = 5,
    MBIM_SMS_CDMA_LANG_CHINESE  = 6,
    MBIM_SMS_CDMA_LANG_HEBREW   = 7
} MbimSmsCdmaLang;

/**
 * MbimSmsCdmaEncoding:
 * @MBIM_SMS_CDMA_ENCODING_OCTET: Octet.
 * @MBIM_SMS_CDMA_ENCODING_EPM: EPM.
 * @MBIM_SMS_CDMA_ENCODING_7BIT_ASCII: 7-bit ASCII.
 * @MBIM_SMS_CDMA_ENCODING_LA5: LA5.
 * @MBIM_SMS_CDMA_ENCODING_UNICODE: Unicode.
 * @MBIM_SMS_CDMA_ENCODING_SHIFT_JIS: Shift JIS.
 * @MBIM_SMS_CDMA_ENCODING_KOREAN: Korean.
 * @MBIM_SMS_CDMA_ENCODING_LATIN_HEBREW: Latin hebrew.
 * @MBIM_SMS_CDMA_ENCODING_LATIN: Latin.
 * @MBIM_SMS_CDMA_ENCODING_GSM_7BIT: 7-bit GSM.
 *
 * Type of encoding of a CDMA SMS.
 *
 * Since: 1.4
 */
typedef enum { /*< since=1.4 >*/
    MBIM_SMS_CDMA_ENCODING_OCTET        = 0,
    MBIM_SMS_CDMA_ENCODING_EPM          = 1,
    MBIM_SMS_CDMA_ENCODING_7BIT_ASCII   = 2,
    MBIM_SMS_CDMA_ENCODING_LA5          = 3,
    MBIM_SMS_CDMA_ENCODING_UNICODE      = 4,
    MBIM_SMS_CDMA_ENCODING_SHIFT_JIS    = 5,
    MBIM_SMS_CDMA_ENCODING_KOREAN       = 6,
    MBIM_SMS_CDMA_ENCODING_LATIN_HEBREW = 7,
    MBIM_SMS_CDMA_ENCODING_LATIN        = 8,
    MBIM_SMS_CDMA_ENCODING_GSM_7BIT     = 9
} MbimSmsCdmaEncoding;

/**
 * MbimSmsStatus:
 * @MBIM_SMS_STATUS_NEW: New.
 * @MBIM_SMS_STATUS_OLD: Old.
 * @MBIM_SMS_STATUS_DRAFT: Draft.
 * @MBIM_SMS_STATUS_SENT: Sent.
 *
 * Status of a SMS message.
 *
 * Since: 1.4
 */
typedef enum { /*< since=1.4 >*/
    MBIM_SMS_STATUS_NEW   = 0,
    MBIM_SMS_STATUS_OLD   = 1,
    MBIM_SMS_STATUS_DRAFT = 2,
    MBIM_SMS_STATUS_SENT  = 3
} MbimSmsStatus;

/*****************************************************************************/
/* 'SMS Message Store Status' enums */

/**
 * MbimSmsStatusFlag:
 * @MBIM_SMS_STATUS_FLAG_NONE: None.
 * @MBIM_SMS_STATUS_FLAG_MESSAGE_STORE_FULL: Message store is full.
 * @MBIM_SMS_STATUS_FLAG_NEW_MESSAGE: New non-Class 0 message arrived.
 *
 * SMS status flags.
 *
 * Since: 1.4
 */
typedef enum { /*< since=1.4 >*/
    MBIM_SMS_STATUS_FLAG_NONE               = 0,
    MBIM_SMS_STATUS_FLAG_MESSAGE_STORE_FULL = 1,
    MBIM_SMS_STATUS_FLAG_NEW_MESSAGE        = 2
} MbimSmsStatusFlag;

/*****************************************************************************/
/* 'USSD' enums */

/**
 * MbimUssdAction:
 * @MBIM_USSD_ACTION_INITIATE: Initiate USSD session.
 * @MBIM_USSD_ACTION_CONTINUE: Continue USSD session.
 * @MBIM_USSD_ACTION_CANCEL: Cancel USSD session.
 *
 * USSD action.
 *
 * Since: 1.4
 */
typedef enum { /*< since=1.4 >*/
    MBIM_USSD_ACTION_INITIATE = 0,
    MBIM_USSD_ACTION_CONTINUE = 1,
    MBIM_USSD_ACTION_CANCEL = 2
} MbimUssdAction;

/**
 * MbimUssdResponse:
 * @MBIM_USSD_RESPONSE_NO_ACTION_REQUIRED: No action required.
 * @MBIM_USSD_RESPONSE_ACTION_REQUIRED: An action is required.
 * @MBIM_USSD_RESPONSE_TERMINATED_BY_NETWORK: Terminated by network
 * @MBIM_USSD_RESPONSE_OTHER_LOCAL_CLIENT: Other local client.
 * @MBIM_USSD_RESPONSE_OPERATION_NOT_SUPPORTED: Operation not supported.
 * @MBIM_USSD_RESPONSE_NETWORK_TIMEOUT: Network timeout.
 *
 * USSD response.
 *
 * Since: 1.4
 */
typedef enum { /*< since=1.4 >*/
    MBIM_USSD_RESPONSE_NO_ACTION_REQUIRED      = 0,
    MBIM_USSD_RESPONSE_ACTION_REQUIRED         = 1,
    MBIM_USSD_RESPONSE_TERMINATED_BY_NETWORK   = 2,
    MBIM_USSD_RESPONSE_OTHER_LOCAL_CLIENT      = 3,
    MBIM_USSD_RESPONSE_OPERATION_NOT_SUPPORTED = 4,
    MBIM_USSD_RESPONSE_NETWORK_TIMEOUT         = 5
} MbimUssdResponse;

/**
 * MbimUssdSessionState:
 * @MBIM_USSD_SESSION_STATE_NEW_SESSION: New session.
 * @MBIM_USSD_SESSION_STATE_EXISTING_SESSION: Existing session.
 *
 * Session state.
 *
 * Since: 1.4
 */
typedef enum { /*< since=1.4 >*/
    MBIM_USSD_SESSION_STATE_NEW_SESSION      = 0,
    MBIM_USSD_SESSION_STATE_EXISTING_SESSION = 1
} MbimUssdSessionState;

/*****************************************************************************/
/* 'Phonebook configuration' enums */

/**
 * MbimPhonebookState:
 * @MBIM_PHONEBOOK_STATE_NOT_INITIALIZED: Not initialized.
 * @MBIM_PHONEBOOK_STATE_INITIALIZED: Initialized
 *
 * Phonebook state.
 *
 * Since: 1.4
 */
typedef enum { /*< since=1.4 >*/
    MBIM_PHONEBOOK_STATE_NOT_INITIALIZED = 0,
    MBIM_PHONEBOOK_STATE_INITIALIZED     = 1
} MbimPhonebookState;

/*****************************************************************************/
/* 'Phonebook read' enums */

/**
 * MbimPhonebookFlag:
 * @MBIM_PHONEBOOK_FLAG_ALL: Request all.
 * @MBIM_PHONEBOOK_FLAG_INDEX: Request single entry by index.
 *
 * Flags to use when reading the phonebook.
 *
 * Since: 1.4
 */
typedef enum { /*< since=1.4 >*/
    MBIM_PHONEBOOK_FLAG_ALL   = 0,
    MBIM_PHONEBOOK_FLAG_INDEX = 1
} MbimPhonebookFlag;

/**
 * MbimPhonebookWriteFlag:
 * @MBIM_PHONEBOOK_WRITE_FLAG_SAVE_UNUSED: Store the record in an unused slot.
 * @MBIM_PHONEBOOK_WRITE_FLAG_SAVE_INDEX: Index where to store the record.
 *
 * Flags to use when writing the phonebook.
 *
 * Since: 1.4
 */
typedef enum { /*< since=1.4 >*/
    MBIM_PHONEBOOK_WRITE_FLAG_SAVE_UNUSED = 0,
    MBIM_PHONEBOOK_WRITE_FLAG_SAVE_INDEX  = 1,
} MbimPhonebookWriteFlag;

/*****************************************************************************/
/* 'STK PAC' enums */

/**
 * MbimStkPacProfile:
 * @MBIM_STK_PAC_PROFILE_NOT_HANDLED_BY_FUNCTION_HANDLED_BY_HOST: Command not handled by function but handled by host.
 * @MBIM_STK_PAC_PROFILE_NOT_HANDLED_BY_FUNCTION_MAY_BE_HANDLED_BY_HOST: Command not handled by function but may be handled by host.
 * @MBIM_STK_PAC_PROFILE_HANDLED_BY_FUNCTION_ONLY_TRANSPARENT_TO_HOST: Command handled by function without informing the host.
 * @MBIM_STK_PAC_PROFILE_HANDLED_BY_FUNCTION_NOTIFICATION_TO_HOST_POSSIBLE: Command handled by function without informing the host, but notifications may be sent to host.
 * @MBIM_STK_PAC_PROFILE_HANDLED_BY_FUNCTION_NOTIFICATIONS_TO_HOST_ENABLED: Command handled by function, and the function wil also send notification to the host.
 * @MBIM_STK_PAC_PROFILE_HANDLED_BY_FUNCTION_CAN_BE_OVERRIDEN_BY_HOST: Command handled by function, but the host may request full control of the command.
 * @MBIM_STK_PAC_PROFILE_HANDLED_BY_HOST_FUNCTION_NOT_ABLE_TO_HANDLE: Command will be forwarded to the host. If the host decides not to receive the command, the function will not handle it.
 * @MBIM_STK_PAC_PROFILE_HANDLED_BY_HOST_FUNCTION_ABLE_TO_HANDLE: Command will be forwarded to the host. If the host decides not to receive the command, the function will handle it.
 *
 * Proactive command profile.
 *
 * Since: 1.4
 */
typedef enum { /*< since=1.4 >*/
    MBIM_STK_PAC_PROFILE_NOT_HANDLED_BY_FUNCTION_HANDLED_BY_HOST           = 0,
    MBIM_STK_PAC_PROFILE_NOT_HANDLED_BY_FUNCTION_MAY_BE_HANDLED_BY_HOST    = 1,
    MBIM_STK_PAC_PROFILE_HANDLED_BY_FUNCTION_ONLY_TRANSPARENT_TO_HOST      = 2,
    MBIM_STK_PAC_PROFILE_HANDLED_BY_FUNCTION_NOTIFICATION_TO_HOST_POSSIBLE = 3,
    MBIM_STK_PAC_PROFILE_HANDLED_BY_FUNCTION_NOTIFICATIONS_TO_HOST_ENABLED = 4,
    MBIM_STK_PAC_PROFILE_HANDLED_BY_FUNCTION_CAN_BE_OVERRIDEN_BY_HOST      = 5,
    MBIM_STK_PAC_PROFILE_HANDLED_BY_HOST_FUNCTION_NOT_ABLE_TO_HANDLE       = 6,
    MBIM_STK_PAC_PROFILE_HANDLED_BY_HOST_FUNCTION_ABLE_TO_HANDLE           = 7
} MbimStkPacProfile;

/**
 * MbimStkPacType:
 * @MBIM_STK_PAC_TYPE_PROACTIVE_COMMAND: Host is requested to handle the Proactive command.
 * @MBIM_STK_PAC_TYPE_NOTIFICATION: Proactive command is handled by the function, but the host is notified.
 *
 * Type of proactive command.
 *
 * Since: 1.4
 */
typedef enum { /*< since=1.4 >*/
    MBIM_STK_PAC_TYPE_PROACTIVE_COMMAND = 0,
    MBIM_STK_PAC_TYPE_NOTIFICATION      = 1
} MbimStkPacType;

/*****************************************************************************/
/* 'Network idle hint' enums */

/**
 * MbimNetworkIdleHintState:
 * @MBIM_NETWORK_IDLE_HINT_STATE_DISABLED: Disabled.
 * @MBIM_NETWORK_IDLE_HINT_STATE_ENABLED: Enabled.
 *
 * Enable or disable network idle hint.
 *
 * Since: 1.4
 */
typedef enum { /*< since=1.4 >*/
    MBIM_NETWORK_IDLE_HINT_STATE_DISABLED = 0,
    MBIM_NETWORK_IDLE_HINT_STATE_ENABLED  = 1
} MbimNetworkIdleHintState;

/*****************************************************************************/
/* 'Emergency mode' enums */

/**
 * MbimEmergencyModeState:
 * @MBIM_EMERGENCY_MODE_STATE_OFF: Off.
 * @MBIM_EMERGENCY_MODE_STATE_ON: On.
 *
 * Emergency mode state.
 *
 * Since: 1.4
 */
typedef enum { /*< since=1.4 >*/
    MBIM_EMERGENCY_MODE_STATE_OFF = 0,
    MBIM_EMERGENCY_MODE_STATE_ON  = 1
} MbimEmergencyModeState;

/*****************************************************************************/
/* 'DSS connect' enums */

/**
 * MbimDssLinkState:
 * @MBIM_DSS_LINK_STATE_DEACTIVATE: Deactivate.
 * @MBIM_DSS_LINK_STATE_ACTIVATE: Activate.
 *
 * Action performed in the link state.
 *
 * Since: 1.4
 */
typedef enum { /*< since=1.4 >*/
    MBIM_DSS_LINK_STATE_DEACTIVATE = 0,
    MBIM_DSS_LINK_STATE_ACTIVATE   = 1
} MbimDssLinkState;

/*****************************************************************************/
/* 'QDU update session' enums */

/**
 * MbimQduSessionAction:
 * @MBIM_QDU_SESSION_ACTION_START: Start.
 * @MBIM_QDU_SESSION_ACTION_STOP: Stop.
 *
 * Action performed for the session initialization.
 *
 * Since: 1.26
 */
typedef enum { /*< since=1.26 >*/
    MBIM_QDU_SESSION_ACTION_START = 0,
    MBIM_QDU_SESSION_ACTION_STOP  = 1,
} MbimQduSessionAction;

/*****************************************************************************/
/* 'QDU session type' enums */

/**
 * MbimQduSessionType:
 * @MBIM_QDU_SESSION_TYPE_NONE: No update session.
 * @MBIM_QDU_SESSION_TYPE_LE: LE-based update procedure.
 *
 * QDU session types.
 *
 * Since: 1.26
 */
typedef enum { /*< since=1.26 >*/
    MBIM_QDU_SESSION_TYPE_NONE = 0,
    MBIM_QDU_SESSION_TYPE_LE   = 1,
} MbimQduSessionType;

/*****************************************************************************/
/* 'QDU session status' enums */

/**
 * MbimQduSessionStatus:
 * @MBIM_QDU_SESSION_STATUS_INACTIVE: Update session is not active.
 * @MBIM_QDU_SESSION_STATUS_FILE_TRANSFER: Updated files are being transferred from host to function or function is waiting host to send updated files.
 * @MBIM_QDU_SESSION_STATUS_APPLY_UPDATE: The function is applying the update package.
 *
 * QDU session status.
 *
 * Since: 1.26
 */
typedef enum { /*< since=1.26 >*/
    MBIM_QDU_SESSION_STATUS_INACTIVE      = 0,
    MBIM_QDU_SESSION_STATUS_FILE_TRANSFER = 1,
    MBIM_QDU_SESSION_STATUS_APPLY_UPDATE  = 2,
} MbimQduSessionStatus;

/*****************************************************************************/
/* 'QDU session result' enums */

/**
 * MbimQduSessionResult:
 * @MBIM_QDU_SESSION_RESULT_SUCCESS: Success
 * @MBIM_QDU_SESSION_RESULT_FAILURE: Failure
 *
 * QDU session result.
 *
 * Since: 1.26
 */
typedef enum { /*< since=1.26 >*/
    MBIM_QDU_SESSION_RESULT_SUCCESS = 0,
    MBIM_QDU_SESSION_RESULT_FAILURE = 1,
} MbimQduSessionResult;

/*****************************************************************************/
/* 'QDU file type' enums */

/**
 * MbimQduFileType:
 * @MBIM_QDU_FILE_TYPE_LITTLE_ENDIAN_PACKAGE: LE Package
 *
 * QDU session result.
 *
 * Since: 1.26
 */
typedef enum { /*< since=1.26 >*/
    MBIM_QDU_FILE_TYPE_LITTLE_ENDIAN_PACKAGE = 0,
} MbimQduFileType;

/*****************************************************************************/
/* 'QDU Quectel Reboot' enums */

/**
 * MbimQduQuectelRebootType:
 * @MBIM_QDU_QUECTEL_REBOOT_TYPE_EDL: Reboot to Emergency Download mode
 *
 * QDU Quectel Reboot session type.
 *
 * Since: 1.28
 */
typedef enum { /*< since=1.28 >*/
    MBIM_QDU_QUECTEL_REBOOT_TYPE_EDL = 17,
} MbimQduQuectelRebootType;

/*****************************************************************************/
/* 'QDU Quectel Version' enums */

/**
 * MbimQduQuectelVersionType:
 * @MBIM_QDU_QUECTEL_VERSION_TYPE_FW_BUILD_ID: Firmware Build ID
 *
 * QDU Quectel Read Version session type.
 *
 * Since: 1.28
 */
typedef enum { /*< since=1.28 >*/
    MBIM_QDU_QUECTEL_VERSION_TYPE_FW_BUILD_ID = 256,
} MbimQduQuectelVersionType;

/*****************************************************************************/
/* 'ATDS RAT' enums */

/**
 * MbimAtdsRatMode:
 * @MBIM_ATDS_RAT_MODE_AUTOMATIC: Automatic.
 * @MBIM_ATDS_RAT_MODE_2G_ONLY: 2G only.
 * @MBIM_ATDS_RAT_MODE_3G_ONLY: 3G only.
 * @MBIM_ATDS_RAT_MODE_4G_ONLY: 4G only.
 *
 * RAT mode preferences.
 *
 * Since: 1.16
 */
typedef enum { /*< since=1.16 >*/
    MBIM_ATDS_RAT_MODE_AUTOMATIC = 0,
    MBIM_ATDS_RAT_MODE_2G_ONLY   = 1,
    MBIM_ATDS_RAT_MODE_3G_ONLY   = 2,
    MBIM_ATDS_RAT_MODE_4G_ONLY   = 3,
} MbimAtdsRatMode;

/*****************************************************************************/
/* 'ATDS Operators' enums */

/**
 * MbimAtdsProviderPlmnMode:
 * @MBIM_ATDS_PROVIDER_PLMN_MODE_GSM: GSM.
 * @MBIM_ATDS_PROVIDER_PLMN_MODE_UTRAN: UTRAN (UMTS).
 * @MBIM_ATDS_PROVIDER_PLMN_MODE_LTE: LTE.
 *
 * Provider PLMN mode.
 *
 * Since: 1.16
 */
typedef enum { /*< since=1.16 >*/
    MBIM_ATDS_PROVIDER_PLMN_MODE_GSM   = 0,
    MBIM_ATDS_PROVIDER_PLMN_MODE_UTRAN = 6,
    MBIM_ATDS_PROVIDER_PLMN_MODE_LTE   = 7,
} MbimAtdsProviderPlmnMode;

/*****************************************************************************/
/* 'PCO' enums */

/**
 * MbimPcoType:
 * @MBIM_PCO_TYPE_COMPLETE: The PCO structure is complete.
 * @MBIM_PCO_TYPE_PARTIAL: The PCO structure is a subset of what was received from the network.
 *
 * Type of PCO structure.
 *
 * Since: 1.18
 */
typedef enum { /*< since=1.18 >*/
    MBIM_PCO_TYPE_COMPLETE = 0,
    MBIM_PCO_TYPE_PARTIAL  = 1
} MbimPcoType;

/*****************************************************************************/
/* 'LTE Attach Configuration' enums */

/**
 * MbimLteAttachContextOperation:
 * @MBIM_LTE_ATTACH_CONTEXT_OPERATION_DEFAULT: Overwrite existing contexts.
 * @MBIM_LTE_ATTACH_CONTEXT_OPERATION_RESTORE_FACTORY: Restore factory preconfigured contexts.
 *
 * Command to run when updating LTE attach configuration.
 *
 * Since: 1.18
 */
typedef enum { /*< since=1.18 >*/
    MBIM_LTE_ATTACH_CONTEXT_OPERATION_DEFAULT         = 0,
    MBIM_LTE_ATTACH_CONTEXT_OPERATION_RESTORE_FACTORY = 1
} MbimLteAttachContextOperation;

/**
 * MbimLteAttachContextRoamingControl:
 * @MBIM_LTE_ATTACH_CONTEXT_ROAMING_CONTROL_HOME: Context allowed to be used on home network.
 * @MBIM_LTE_ATTACH_CONTEXT_ROAMING_CONTROL_PARTNER: Context allowed to be used on partner network.
 * @MBIM_LTE_ATTACH_CONTEXT_ROAMING_CONTROL_NON_PARTNER: Context allowed to be used on non-partner network.
 *
 * Type of roaming control.
 *
 * Since: 1.18
*/
typedef enum { /*< since=1.18 >*/
    MBIM_LTE_ATTACH_CONTEXT_ROAMING_CONTROL_HOME        = 0,
    MBIM_LTE_ATTACH_CONTEXT_ROAMING_CONTROL_PARTNER     = 1,
    MBIM_LTE_ATTACH_CONTEXT_ROAMING_CONTROL_NON_PARTNER = 2
} MbimLteAttachContextRoamingControl;

/**
 * MbimContextSource:
 * @MBIM_CONTEXT_SOURCE_ADMIN: Context created by enterprise IT.
 * @MBIM_CONTEXT_SOURCE_USER: Context created by user.
 * @MBIM_CONTEXT_SOURCE_OPERATOR: Context created by operator.
 * @MBIM_CONTEXT_SOURCE_MODEM: Context created by modem manufacturer.
 * @MBIM_CONTEXT_SOURCE_DEVICE: Context created by OS APN database.
 *
 * Source of context creation.
 *
 * Since: 1.18
 */
typedef enum { /*< since=1.18 >*/
    MBIM_CONTEXT_SOURCE_ADMIN    = 0,
    MBIM_CONTEXT_SOURCE_USER     = 1,
    MBIM_CONTEXT_SOURCE_OPERATOR = 2,
    MBIM_CONTEXT_SOURCE_MODEM    = 3,
    MBIM_CONTEXT_SOURCE_DEVICE   = 4
} MbimContextSource;

/**
 * MbimLteAttachState:
 * @MBIM_LTE_ATTACH_STATE_DETACHED: Detached.
 * @MBIM_LTE_ATTACH_STATE_ATTACHED: Attached.
 *
 * LTE attach state.
 *
 * Since: 1.18
 */
typedef enum { /*< since=1.18 >*/
    MBIM_LTE_ATTACH_STATE_DETACHED = 0,
    MBIM_LTE_ATTACH_STATE_ATTACHED = 1
} MbimLteAttachState;

/*****************************************************************************/
/* 'SAR config' enums */

/**
 * MbimSarControlMode:
 * @MBIM_SAR_CONTROL_MODE_DEVICE: SAR backoff is controlled by the modem directly.
 * @MBIM_SAR_CONTROL_MODE_OS: SAR backoff is controlled by the host operating system.
 *
 * SAR backoff mechanism control modes.
 *
 * Since: 1.26
 */
typedef enum { /*< since=1.26 >*/
    MBIM_SAR_CONTROL_MODE_DEVICE = 0,
    MBIM_SAR_CONTROL_MODE_OS     = 1
} MbimSarControlMode;

/**
 * MbimSarBackoffState:
 * @MBIM_SAR_BACKOFF_STATE_DISABLED: SAR backoff is disabled.
 * @MBIM_SAR_BACKOFF_STATE_ENABLED: SAR backoff is enabled.
 *
 * SAR backoff state.
 *
 * Since: 1.26
 */
typedef enum { /*< since=1.26 >*/
    MBIM_SAR_BACKOFF_STATE_DISABLED = 0,
    MBIM_SAR_BACKOFF_STATE_ENABLED  = 1
} MbimSarBackoffState;

/**
 * MbimSarWifiHardwareState:
 * @MBIM_SAR_WIFI_HARDWARE_STATE_INTEGRATED: Integrated.
 * @MBIM_SAR_WIFI_HARDWARE_STATE_NOT_INTEGRATED: Not integrated.
 *
 * Whether the Wi-Fi and cellular SAR are integrated at hardware level.
 *
 * Since: 1.26
 */
typedef enum { /*< since=1.26 >*/
    MBIM_SAR_WIFI_HARDWARE_STATE_INTEGRATED     = 0,
    MBIM_SAR_WIFI_HARDWARE_STATE_NOT_INTEGRATED = 1
} MbimSarWifiHardwareState;

/**
 * MbimTransmissionNotificationStatus:
 * @MBIM_TRANSMISSION_NOTIFICATION_STATUS_DISABLED: Notification disabled.
 * @MBIM_TRANSMISSION_NOTIFICATION_STATUS_ENABLED: Notification enabled.
 *
 * Whether modem channel transmission status notification is disabled or enabled.
 *
 * Since: 1.26
 */
typedef enum { /*< since=1.26 >*/
    MBIM_TRANSMISSION_NOTIFICATION_STATUS_DISABLED = 0,
    MBIM_TRANSMISSION_NOTIFICATION_STATUS_ENABLED  = 1
} MbimTransmissionNotificationStatus;

/**
 * MbimTransmissionState:
 * @MBIM_TRANSMISSION_STATE_INACTIVE: Modem was not actively transmitting data.
 * @MBIM_TRANSMISSION_STATE_ACTIVE: Modem was actively transmitting data.
 *
 * Whether modem is having TX traffic every hysteresis timeout.
 *
 * Since: 1.26
 */
typedef enum { /*< since=1.26 >*/
    MBIM_TRANSMISSION_STATE_INACTIVE = 0,
    MBIM_TRANSMISSION_STATE_ACTIVE   = 1
} MbimTransmissionState;

/*****************************************************************************/
/* 'Slot Info Status' enums */

/**
 * MbimUiccSlotState:
 * @MBIM_UICC_SLOT_STATE_UNKNOWN: The modem is still in the process of initializing so the SIM slot state is not deterministic.
 * @MBIM_UICC_SLOT_SATE_OFF_EMPTY: The UICC slot is powered off and no card is present.
 * @MBIM_UICC_SLOT_STATE_OFF: The UICC slot is powered off.
 * @MBIM_UICC_SLOT_STATE_EMPTY: The UICC slot is empty(there is no card in it).
 * @MBIM_UICC_SLOT_STATE_NOT_READY: The UICC slot is occupied and powered on but the card within it is not yet ready.
 * @MBIM_UICC_SLOT_STATE_ACTIVE: The UICC slot is occupied and the card within it is ready.
 * @MBIM_UICC_SLOT_STATE_ERROR: The UICC slot is occupied and powreed on but the card is in an error state and cannot be used until it is next reset.
 * @MBIM_UICC_SLOT_STATE_ACTIVE_ESIM: The card in the slot is an eSIM with an active profile and is ready to accept commands.
 * @MBIM_UICC_SLOT_STATE_ACTIVE_ESIM_NO_PROFILES: The card in the slot is an eSIM with no profiles(or no active profiles) and is ready to accept commands.
 *
 * MbimUiccSlotState
 *
 * Since: 1.26
 */
typedef enum { /*< since=1.26 >*/
    MBIM_UICC_SLOT_STATE_UNKNOWN = 0,
    MBIM_UICC_SLOT_SATE_OFF_EMPTY = 1,
    MBIM_UICC_SLOT_STATE_OFF = 2,
    MBIM_UICC_SLOT_STATE_EMPTY = 3,
    MBIM_UICC_SLOT_STATE_NOT_READY = 4,
    MBIM_UICC_SLOT_STATE_ACTIVE = 5,
    MBIM_UICC_SLOT_STATE_ERROR = 6,
    MBIM_UICC_SLOT_STATE_ACTIVE_ESIM = 7,
    MBIM_UICC_SLOT_STATE_ACTIVE_ESIM_NO_PROFILES = 8
} MbimUiccSlotState;

/*****************************************************************************/
/* 'UICC APDU' enums */

/**
 * MbimUiccSecureMessaging:
 * @MBIM_UICC_SECURE_MESSAGING_NONE: No secure messaging.
 * @MBIM_UICC_SECURE_MESSAGING_NO_HDR_AUTH: Secure, command header not authenticated.
 *
 * Specifies whether apdu is exchanged using secure messaging.
 *
 * Since: 1.26
 */
typedef enum { /*< since=1.26 >*/
    MBIM_UICC_SECURE_MESSAGING_NONE             = 0,
    MBIM_UICC_SECURE_MESSAGING_NO_HDR_AUTH      = 1,
} MbimUiccSecureMessaging;

/**
 * MbimUiccClassByteType:
 * @MBIM_UICC_CLASS_BYTE_TYPE_INTER_INDUSTRY: Defined according to first interindustry definition in ISO 7816-4.
 * @MBIM_UICC_CLASS_BYTE_TYPE_EXTENDED: Defined according to extended definition in ETSI 102 221.
 *
 * Specifies the type of class byte definition.
 *
 * Since: 1.26
 */
typedef enum { /*< since=1.26 >*/
    MBIM_UICC_CLASS_BYTE_TYPE_INTER_INDUSTRY   = 0,
    MBIM_UICC_CLASS_BYTE_TYPE_EXTENDED         = 1,
} MbimUiccClassByteType;

/* 'UICC Reset' enums */

/**
 * MbimUiccPassThroughAction:
 * @MBIM_UICC_PASS_THROUGH_ACTION_DISABLE: Disable pass through.
 * @MBIM_UICC_PASS_THROUGH_ACTION_ENABLE: Enable pass through.
 *
 * Pass through actions host can specify to modem. Upon reset
 * treats UICC as if pass through or as a regular UICC
 *
 * Since: 1.26
 */
typedef enum { /*< since=1.26 >*/
    MBIM_UICC_PASS_THROUGH_ACTION_DISABLE     = 0,
    MBIM_UICC_PASS_THROUGH_ACTION_ENABLE      = 1,
} MbimUiccPassThroughAction;

/**
 * MbimUiccPassThroughStatus:
 * @MBIM_UICC_PASS_THROUGH_STATUS_DISABLED: Pass through disabled.
 * @MBIM_UICC_PASS_THROUGH_STATUS_ENABLED: Pass through enabled.
 *
 * Pass through status of the modem specifies to the host.
 *
 * Since: 1.26
 */
typedef enum { /*< since=1.26 >*/
    MBIM_UICC_PASS_THROUGH_STATUS_DISABLED   = 0,
    MBIM_UICC_PASS_THROUGH_STATUS_ENABLED    = 1,
} MbimUiccPassThroughStatus;

/*****************************************************************************/
/* 'Modem Configuration' enums */

/**
 * MbimModemConfigurationStatus:
 * @MBIM_MODEM_CONFIGURATION_STATUS_UNKNOWN: Unknown
 * @MBIM_MODEM_CONFIGURATION_STATUS_STARTED: SIM is ready and configuration started.
 * @MBIM_MODEM_CONFIGURATION_STATUS_COMPLETED: SIM is ready and configuration completed.
 *
 * Modem configuration status.
 *
 * Since: 1.28
 */
typedef enum {/*< since=1.28 >*/
    MBIM_MODEM_CONFIGURATION_STATUS_UNKNOWN   = 0,
    MBIM_MODEM_CONFIGURATION_STATUS_STARTED   = 1,
    MBIM_MODEM_CONFIGURATION_STATUS_COMPLETED = 2
} MbimModemConfigurationStatus;

/*****************************************************************************/
/* 'Packet service v2'  enums */

/**
 * MbimFrequencyRange:
 * @MBIM_FREQUENCY_RANGE_UNKNOWN: Unknown.
 * @MBIM_FREQUENCY_RANGE_1: Frequency range 1 (FR1) in 3GPP TS 38.101-1 (Sub-6G).
 * @MBIM_FREQUENCY_RANGE_2: FR2 in 3GPP TS 38.101-2 (mmWave).
 *
 * Frequency Range.
 *
 * Since: 1.28
 */
typedef enum { /*< since=1.28 >*/
    MBIM_FREQUENCY_RANGE_UNKNOWN = 0,
    MBIM_FREQUENCY_RANGE_1       = 1 << 0,
    MBIM_FREQUENCY_RANGE_2       = 1 << 1
} MbimFrequencyRange;

/*****************************************************************************/
/* 'Provisioned Context v2' enums */

/**
 * MbimContextRoamingControl:
 * @MBIM_CONTEXT_ROAMING_CONTROL_HOME_ONLY: Context allowed to be used in home network.
 * @MBIM_CONTEXT_ROAMING_CONTROL_PARTNER_ONLY: Context allowed to be used in partner network.
 * @MBIM_CONTEXT_ROAMING_CONTROL_NON_PARTNER_ONLY: Context allowed to be used in non-partner network.
 * @MBIM_CONTEXT_ROAMING_CONTROL_HOME_AND_PARTNER: Context allowed to be used in home and partner networks.
 * @MBIM_CONTEXT_ROAMING_CONTROL_HOME_AND_NON_PARTNER: Context allowed to be used in home and non-partner networks.
 * @MBIM_CONTEXT_ROAMING_CONTROL_PARTNER_AND_NON_PARTNER: Context allowed to be used in partner and non-partner networks.
 * @MBIM_CONTEXT_ROAMING_CONTROL_ALLOW_ALL: Context allowed to be used in any roaming condition.
 *
 * Type of roaming control.
 *
 * Since: 1.28
 */
typedef enum { /*< since=1.28 >*/
    MBIM_CONTEXT_ROAMING_CONTROL_HOME_ONLY               = 0,
    MBIM_CONTEXT_ROAMING_CONTROL_PARTNER_ONLY            = 1,
    MBIM_CONTEXT_ROAMING_CONTROL_NON_PARTNER_ONLY        = 2,
    MBIM_CONTEXT_ROAMING_CONTROL_HOME_AND_PARTNER        = 3,
    MBIM_CONTEXT_ROAMING_CONTROL_HOME_AND_NON_PARTNER    = 4,
    MBIM_CONTEXT_ROAMING_CONTROL_PARTNER_AND_NON_PARTNER = 5,
    MBIM_CONTEXT_ROAMING_CONTROL_ALLOW_ALL               = 6
} MbimContextRoamingControl;

/**
 * MbimContextMediaType:
 * @MBIM_CONTEXT_MEDIA_TYPE_CELLULAR_ONLY: Context allowed to be used only wen registered over cellular.
 * @MBIM_CONTEXT_MEDIA_TYPE_WIFI_ONLY: Context allowed to be used only when registered over Wi-Fi.
 * @MBIM_CONTEXT_MEDIA_TYPE_ALL: Context allowed to be used when registered either over cellular or Wi-Fi.
 *
 * Media type, specifying whether the modem is used for cellular or
 * iWLAN (Wi-Fi offload).
 *
 * Since: 1.28
 */
typedef enum { /*< since=1.28 >*/
    MBIM_CONTEXT_MEDIA_TYPE_CELLULAR_ONLY = 0,
    MBIM_CONTEXT_MEDIA_TYPE_WIFI_ONLY     = 1,
    MBIM_CONTEXT_MEDIA_TYPE_ALL           = 2
} MbimContextMediaType;

/**
 * MbimContextState:
 * @MBIM_CONTEXT_STATE_DISABLED: Disabled.
 * @MBIM_CONTEXT_STATE_ENABLED: Enabled.
 *
 * Status of the context, specifying whether it's enabled or disabled.
 *
 * Since: 1.28
 */
typedef enum { /*< since=1.28 >*/
    MBIM_CONTEXT_STATE_DISABLED = 0,
    MBIM_CONTEXT_STATE_ENABLED  = 1,
} MbimContextState;

/**
 * MbimContextOperation:
 * @MBIM_CONTEXT_OPERATION_DEFAULT: Adds or replaces an existing context.
 * @MBIM_CONTEXT_OPERATION_DELETE: Delete an existing context.
 * @MBIM_CONTEXT_OPERATION_RESTORE_FACTORY: Restore preconfigured contexts for the provider ID of the current SIM card.
 *
 * Command to run when updating context configuration.
 *
 * Since: 1.28
 */
typedef enum { /*< since=1.28 >*/
    MBIM_CONTEXT_OPERATION_DEFAULT         = 0,
    MBIM_CONTEXT_OPERATION_DELETE          = 1,
    MBIM_CONTEXT_OPERATION_RESTORE_FACTORY = 2,
} MbimContextOperation;

/*****************************************************************************/
/* 'Registration Parameters' enums */

/**
 * MbimMicoMode:
 * @MBIM_MICO_MODE_DISABLED: Disabled.
 * @MBIM_MICO_MODE_ENABLED: Enabled.
 * @MBIM_MICO_MODE_UNSUPPORTED: Device does not support MICO mode.
 * @MBIM_MICO_MODE_DEFAULT: Default.
 *
 * Mobile Initiated Connection Only (MICO) Mode state.
 *
 * The @MBIM_MICO_MODE_DEFAULT value should only be used in set operations, it
 * should never be returned by the module upon a query.
 *
 * Since: 1.28
 */
typedef enum { /*< since=1.28 >*/
    MBIM_MICO_MODE_DISABLED    = 0,
    MBIM_MICO_MODE_ENABLED     = 1,
    MBIM_MICO_MODE_UNSUPPORTED = 2,
    MBIM_MICO_MODE_DEFAULT     = 3,
} MbimMicoMode;

/**
 * MbimDefaultPduActivationHint:
 * @MBIM_DEFAULT_PDU_ACTIVATION_HINT_UNLIKELY: Default PDU session activation is unlikely to happen soon.
 * @MBIM_DEFAULT_PDU_ACTIVATION_HINT_LIKELY: Default PDU session activation is likely to happen soon.
 *
 * Hint regarding the activation of the default PDU session.
 *
 * Since: 1.28
 */
typedef enum { /*< since=1.28 >*/
    MBIM_DEFAULT_PDU_ACTIVATION_HINT_UNLIKELY = 0,
    MBIM_DEFAULT_PDU_ACTIVATION_HINT_LIKELY   = 1,
} MbimDefaultPduActivationHint;

/**
 * MbimLadnInfo:
 * @MBIM_LADN_INFO_NOT_NEEDED: Information not needed.
 * @MBIM_LADN_INFO_REQUESTED: Information requested.
 *
 * Local Area Data Network (LADN) information state.
 *
 * Since: 1.28
 */
typedef enum { /*< since=1.28 >*/
    MBIM_LADN_INFO_NOT_NEEDED = 0,
    MBIM_LADN_INFO_REQUESTED  = 1,
} MbimLadnInfo;

/**
 * MbimDrxCycle:
 * @MBIM_DRX_CYCLE_NOT_SPECIFIED: DRX cycle not specified.
 * @MBIM_DRX_CYCLE_NOT_SUPPORTED: DRX cycle setting not supported.
 * @MBIM_DRX_CYCLE_32: DRX cycle T=32.
 * @MBIM_DRX_CYCLE_64: DRX cycle T=64.
 * @MBIM_DRX_CYCLE_128: DRX cycle T=128.
 * @MBIM_DRX_CYCLE_256: DRX cycle T=256.
 *
 * DRX cycle settings.
 *
 * Since: 1.28
 */
typedef enum { /*< since=1.28 >*/
    MBIM_DRX_CYCLE_NOT_SPECIFIED = 0,
    MBIM_DRX_CYCLE_NOT_SUPPORTED = 1,
    MBIM_DRX_CYCLE_32            = 2,
    MBIM_DRX_CYCLE_64            = 3,
    MBIM_DRX_CYCLE_128           = 4,
    MBIM_DRX_CYCLE_256           = 5,
} MbimDrxCycle;

/*****************************************************************************/
/* 'Subscriber Ready Status v3' enums */

/**
 * MbimSubscriberReadyStatusFlag:
 * @MBIM_SUBSCRIBER_READY_STATUS_FLAG_NONE: None.
 * @MBIM_SUBSCRIBER_READY_STATUS_FLAG_ESIM: Whether the SIM is an eSIM or not.
 * @MBIM_SUBSCRIBER_READY_STATUS_FLAG_SIM_REMOVABILITY_KNOWN: Whether the SIM removability details are know or not.
 * @MBIM_SUBSCRIBER_READY_STATUS_FLAG_SIM_REMOVABLE: Whether the SIM is removable or not, valid only if @MBIM_SUBSCRIBER_READY_STATUS_FLAG_SIM_REMOVABLITY_KNOWN is also given.
 *
 * Flags specifying SIM related details.
 *
 * Since: 1.28
 */
typedef enum { /*< since=1.28 >*/
    MBIM_SUBSCRIBER_READY_STATUS_FLAG_NONE                   = 0,
    MBIM_SUBSCRIBER_READY_STATUS_FLAG_ESIM                   = 1 << 0,
    MBIM_SUBSCRIBER_READY_STATUS_FLAG_SIM_REMOVABILITY_KNOWN = 1 << 1,
    MBIM_SUBSCRIBER_READY_STATUS_FLAG_SIM_REMOVABLE          = 1 << 2,
} MbimSubscriberReadyStatusFlag;

/*****************************************************************************/
/* 'Device Caps v3' and 'Base Stations Info v3' enums */

/**
 * MbimDataClassV3:
 * @MBIM_DATA_CLASS_V3_GPRS: GPRS.
 * @MBIM_DATA_CLASS_V3_EDGE: EDGE.
 * @MBIM_DATA_CLASS_V3_UMTS: UMTS.
 * @MBIM_DATA_CLASS_V3_HSDPA: HSDPA.
 * @MBIM_DATA_CLASS_V3_HSUPA: HSUPA.
 * @MBIM_DATA_CLASS_V3_LTE: LTE.
 * @MBIM_DATA_CLASS_V3_5G: 5G.
 * @MBIM_DATA_CLASS_V3_1XRTT: 1xRTT.
 * @MBIM_DATA_CLASS_V3_1XEVDO: 1xEV-DO.
 * @MBIM_DATA_CLASS_V3_1XEVDO_REVA: 1xEV-DO RevA
 * @MBIM_DATA_CLASS_V3_1XEVDV: 1xEV-DV.
 * @MBIM_DATA_CLASS_V3_3XRTT: 3xRTT.
 * @MBIM_DATA_CLASS_V3_1XEVDO_REVB: 1xEV-DO RevB.
 * @MBIM_DATA_CLASS_V3_UMB: UMB.
 * @MBIM_DATA_CLASS_V3_CUSTOM: Custom.
 *
 * Data class update in MBIMEx v3.0.
 *
 * There is now a single flag for 5G, and the new #MbimDataSubclass helps to
 * identify the specific 5G setup.
 *
 * This type should be considered incompatible with #MbimDataClass.
 *
 * Since: 1.28
 */
typedef enum { /*< since=1.28 >*/
    MBIM_DATA_CLASS_V3_GPRS        = 1 << 0,
    MBIM_DATA_CLASS_V3_EDGE        = 1 << 1,
    MBIM_DATA_CLASS_V3_UMTS        = 1 << 2,
    MBIM_DATA_CLASS_V3_HSDPA       = 1 << 3,
    MBIM_DATA_CLASS_V3_HSUPA       = 1 << 4,
    MBIM_DATA_CLASS_V3_LTE         = 1 << 5,
    MBIM_DATA_CLASS_V3_5G          = 1 << 6,
    /* Bit 7 unused, bits 8 to 15 reserved for future 3GPP classes */
    MBIM_DATA_CLASS_V3_1XRTT       = 1 << 16,
    MBIM_DATA_CLASS_V3_1XEVDO      = 1 << 17,
    MBIM_DATA_CLASS_V3_1XEVDO_REVA = 1 << 18,
    MBIM_DATA_CLASS_V3_1XEVDV      = 1 << 19,
    MBIM_DATA_CLASS_V3_3XRTT       = 1 << 20,
    MBIM_DATA_CLASS_V3_1XEVDO_REVB = 1 << 21,
    MBIM_DATA_CLASS_V3_UMB         = 1 << 22,
    /* Bits 23 to 30 reserved for future 3GPP2 classes */
    MBIM_DATA_CLASS_V3_CUSTOM      = 1 << 31
} MbimDataClassV3;

/**
 * MbimDataSubclass:
 * @MBIM_DATA_SUBCLASS_NONE: No data subclass.
 * @MBIM_DATA_SUBCLASS_5G_ENDC: EUTRAN and NR dual connectivity as in 5G option 3.
 * @MBIM_DATA_SUBCLASS_5G_NR: Standalone NR as in 5G option 2.
 * @MBIM_DATA_SUBCLASS_5G_NEDC: NR and EUTRAN dual connectivity as in 5G option 4.
 * @MBIM_DATA_SUBCLASS_5G_ELTE: eLTE as in 5G option 5.
 * @MBIM_DATA_SUBCLASS_5G_NGENDC: Next-gen eLTE and NR dual connectivity as in 5G option 7.
 *
 * Flags specifying the data subclass.
 *
 * Since: 1.28
 */
typedef enum { /*< since=1.28 >*/
    MBIM_DATA_SUBCLASS_NONE      = 0,
    MBIM_DATA_SUBCLASS_5G_ENDC   = 1 << 0,
    MBIM_DATA_SUBCLASS_5G_NR     = 1 << 1,
    MBIM_DATA_SUBCLASS_5G_NEDC   = 1 << 2,
    MBIM_DATA_SUBCLASS_5G_ELTE   = 1 << 3,
    MBIM_DATA_SUBCLASS_5G_NGENDC = 1 << 4,
} MbimDataSubclass;

/*****************************************************************************/
/* 'Quectel Radio State' enums */

/**
 * MbimQuectelRadioSwitchState:
 * @MBIM_QUECTEL_RADIO_SWITCH_STATE_OFF: Radio is off.
 * @MBIM_QUECTEL_RADIO_SWITCH_STATE_ON: Radio is on.
 * @MBIM_QUECTEL_RADIO_SWITCH_STATE_FCC_LOCKED: Radio is FCC locked.
 *
 * Radio switch state.
 *
 * Since: 1.26.2
 */
typedef enum { /*< since=1.26.2 >*/
    MBIM_QUECTEL_RADIO_SWITCH_STATE_OFF        = 0,
    MBIM_QUECTEL_RADIO_SWITCH_STATE_ON         = 1,
    MBIM_QUECTEL_RADIO_SWITCH_STATE_FCC_LOCKED = 4,
} MbimQuectelRadioSwitchState;

/*****************************************************************************/
/* 'Serving Cell Information' */

/**
 * MbimIntelServingCellInfo:
 * @MBIM_INTEL_SERVING_CELL_INFO_PCELL: Primary cell.
 * @MBIM_INTEL_SERVING_CELL_INFO_SCELL: Secondary cell.
 * @MBIM_INTEL_SERVING_CELL_INFO_PSCELL: Primary cell in SCS.
 * @MBIM_INTEL_SERVING_CELL_INFO_SSCELL: Secondary cell in SCS.
 * @MBIM_INTEL_SERVING_CELL_INFO_RADIO_OFF: Radio state is off.
 *
 * Serving cell information.
 *
 * Since: 1.28
 */
typedef enum { /*< since=1.28 >*/
    MBIM_INTEL_SERVING_CELL_INFO_PCELL     = 0,
    MBIM_INTEL_SERVING_CELL_INFO_SCELL     = 1,
    MBIM_INTEL_SERVING_CELL_INFO_PSCELL    = 2,
    MBIM_INTEL_SERVING_CELL_INFO_SSCELL    = 3,
    MBIM_INTEL_SERVING_CELL_INFO_RADIO_OFF = 0xFFFFFFFF,
} MbimIntelServingCellInfo;

/*****************************************************************************/
/* 'Mbim Intel Boot Mode enums */

/**
 * MbimIntelBootMode:
 * @MBIM_INTEL_BOOT_MODE_NORMAL_MODE: Normal boot mode.
 * @MBIM_INTEL_BOOT_MODE_DOWNLOAD_MODE: Download boot mode.
 * @MBIM_INTEL_BOOT_MODE_POWER_OFF_DEVICE: Power off device.
 * @MBIM_INTEL_BOOT_MODE_NON_RESETABLE_REGISTER: Configure non-resetable register without reboot or power off.
 * @MBIM_INTEL_BOOT_MODE_WITHOUT_REBOOT_POWER_OFF: Configure without reboot power-off.
 * @MBIM_INTEL_BOOT_MODE_FAST_DOWNLOAD_MODE: Fast boot in download mode.
 *
 * Modem intel boot mode.
 *
 * Since: 1.28
 */
typedef enum { /*< since=1.28 >*/
    MBIM_INTEL_BOOT_MODE_NORMAL_MODE              = 0,
    MBIM_INTEL_BOOT_MODE_DOWNLOAD_MODE            = 1,
    MBIM_INTEL_BOOT_MODE_POWER_OFF_DEVICE         = 2,
    MBIM_INTEL_BOOT_MODE_NON_RESETABLE_REGISTER   = 3,
    MBIM_INTEL_BOOT_MODE_WITHOUT_REBOOT_POWER_OFF = 4,
    MBIM_INTEL_BOOT_MODE_FAST_DOWNLOAD_MODE       = 5,
} MbimIntelBootMode;

/**
 * MbimUiccApplicationType:
 * @MBIM_UICC_APPLICATION_TYPE_UNKNOWN: Unknown.
 * @MBIM_UICC_APPLICATION_TYPE_MF: Legacy SIM directories rooted at the MF.
 * @MBIM_UICC_APPLICATION_TYPE_MF_SIM: Legacy SIM directories rooted at the DF_GSM.
 * @MBIM_UICC_APPLICATION_TYPE_MF_RUIM: Legacy SIM directories rooted at the DF_CDMA.
 * @MBIM_UICC_APPLICATION_TYPE_USIM: USIM application.
 * @MBIM_UICC_APPLICATION_TYPE_CSIM: CSIM application.
 * @MBIM_UICC_APPLICATION_TYPE_ISIM: ISIM application.
 *
 * Type of UICC application.
 *
 * Since: 1.28
 */
typedef enum { /*< since=1.28 >*/
    MBIM_UICC_APPLICATION_TYPE_UNKNOWN = 0,
    MBIM_UICC_APPLICATION_TYPE_MF      = 1,
    MBIM_UICC_APPLICATION_TYPE_MF_SIM  = 2,
    MBIM_UICC_APPLICATION_TYPE_MF_RUIM = 3,
    MBIM_UICC_APPLICATION_TYPE_USIM    = 4,
    MBIM_UICC_APPLICATION_TYPE_CSIM    = 5,
    MBIM_UICC_APPLICATION_TYPE_ISIM    = 6,
} MbimUiccApplicationType;

/**
 * MbimUiccFileAccessibility:
 * @MBIM_UICC_FILE_ACCESSIBILITY_UNKNOWN: Unknown.
 * @MBIM_UICC_FILE_ACCESSIBILITY_NOT_SHAREABLE: Not shareable.
 * @MBIM_UICC_FILE_ACCESSIBILITY_SHAREABLE: Shareable.
 *
 * The UICC file accessibility.
 *
 * Since: 1.28
 */
typedef enum { /*< since=1.28 >*/
    MBIM_UICC_FILE_ACCESSIBILITY_UNKNOWN       = 0,
    MBIM_UICC_FILE_ACCESSIBILITY_NOT_SHAREABLE = 1,
    MBIM_UICC_FILE_ACCESSIBILITY_SHAREABLE     = 2,
} MbimUiccFileAccessibility;

/**
 * MbimUiccFileType:
 * @MBIM_UICC_FILE_TYPE_UNKNOWN: Unknown.
 * @MBIM_UICC_FILE_TYPE_WORKING_EF: Working EF.
 * @MBIM_UICC_FILE_TYPE_INTERNAL_EF: Internal EF.
 * @MBIM_UICC_FILE_TYPE_DF_OR_ADF: Dedicated file, DF or ADF.
 *
 * The UICC file type.
 *
 * Since: 1.28
 */
typedef enum { /*< since=1.28 >*/
    MBIM_UICC_FILE_TYPE_UNKNOWN     = 0,
    MBIM_UICC_FILE_TYPE_WORKING_EF  = 1,
    MBIM_UICC_FILE_TYPE_INTERNAL_EF = 2,
    MBIM_UICC_FILE_TYPE_DF_OR_ADF   = 3,
} MbimUiccFileType;

/**
 * MbimUiccFileStructure:
 * @MBIM_UICC_FILE_STRUCTURE_UNKNOWN: Unknown.
 * @MBIM_UICC_FILE_STRUCTURE_TRANSPARENT: A single record of variable length.
 * @MBIM_UICC_FILE_STRUCTURE_CYCLIC: A cyclic set of records, each of the same length.
 * @MBIM_UICC_FILE_STRUCTURE_LINEAR: A linear set of records, each of the same length.
 * @MBIM_UICC_FILE_STRUCTURE_BER_TLV: A set of data values accessible by tag.
 *
 * The UICC file structure.
 *
 * Since: 1.28
 */
typedef enum { /*< since=1.28 >*/
    MBIM_UICC_FILE_STRUCTURE_UNKNOWN     = 0,
    MBIM_UICC_FILE_STRUCTURE_TRANSPARENT = 1,
    MBIM_UICC_FILE_STRUCTURE_CYCLIC      = 2,
    MBIM_UICC_FILE_STRUCTURE_LINEAR      = 3,
    MBIM_UICC_FILE_STRUCTURE_BER_TLV     = 4,
} MbimUiccFileStructure;

G_END_DECLS

#endif /* _LIBMBIM_GLIB_MBIM_ENUMS_H_ */
