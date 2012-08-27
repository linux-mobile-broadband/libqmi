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
 * Copyright (C) 2012 Lanedo GmbH <aleksander@lanedo.com>
 */

#ifndef _LIBQMI_GLIB_QMI_ENUMS_WDS_H_
#define _LIBQMI_GLIB_QMI_ENUMS_WDS_H_

/*****************************************************************************/
/* Helper enums for the 'QMI WDS Start Network' message */

/**
 * QmiWdsAuthenticationPreference:
 * @QMI_WDS_AUTHENTICATION_PREFERENCE_ALLOW_PAP: PAP authentication may be performed.
 * @QMI_WDS_AUTHENTICATION_PREFERENCE_ALLOW_CHAP: CHAP authentication may be performed.
 *
 * Type of authentication that may be performed by the device
 */
typedef enum {
    QMI_WDS_AUTHENTICATION_PREFERENCE_ALLOW_PAP  = 1 << 0,
    QMI_WDS_AUTHENTICATION_PREFERENCE_ALLOW_CHAP = 1 << 1
} QmiWdsAuthenticationPreference;

/**
 * QmiWdsIpFamily:
 * @QMI_WDS_IP_FAMILY_IPV4: IPv4.
 * @QMI_WDS_IP_FAMILY_IPV6: IPv6.
 * @QMI_WDS_IP_FAMILY_UNSPECIFIED: None specified.
 *
 * Type of IP family preference.
 */
typedef enum {
    QMI_WDS_IP_FAMILY_IPV4        = 4,
    QMI_WDS_IP_FAMILY_IPV6        = 6,
    QMI_WDS_IP_FAMILY_UNSPECIFIED = 8
} QmiWdsIpFamily;

/**
 * QmiWdsTechnologyPreference:
 * @QMI_WDS_TECHNOLOGY_PREFERENCE_ALLOW_3GPP: 3GPP allowed.
 * @QMI_WDS_TECHNOLOGY_PREFERENCE_ALLOW_3GPP2: 3GPP2 allowed.
 *
 * Type of network allowed when trying to connect.
 */
typedef enum {
    QMI_WDS_TECHNOLOGY_PREFERENCE_ALLOW_3GPP  = 1 << 0,
    QMI_WDS_TECHNOLOGY_PREFERENCE_ALLOW_3GPP2 = 1 << 1
} QmiWdsTechnologyPreference;

/**
 * QmiWdsExtendedTechnologyPreference:
 * @QMI_WDS_EXTENDED_TECHNOLOGY_PREFERENCE_CDMA: Use CDMA.
 * @QMI_WDS_EXTENDED_TECHNOLOGY_PREFERENCE_UMTS: Use UMTS.
 * @QMI_WDS_EXTENDED_TECHNOLOGY_PREFERENCE_EMBMS: Use eMBMS.
 *
 * Type of network allowed when trying to connect.
 */
typedef enum {
    QMI_WDS_EXTENDED_TECHNOLOGY_PREFERENCE_CDMA  = -32767,
    QMI_WDS_EXTENDED_TECHNOLOGY_PREFERENCE_UMTS  = -32764,
    QMI_WDS_EXTENDED_TECHNOLOGY_PREFERENCE_EMBMS = -30590
} QmiWdsExtendedTechnologyPreference;

/**
 * QmiWdsCallType:
 * @QMI_WDS_CALL_TYPE_LAPTOP: Laptop call.
 * @QMI_WDS_CALL_TYPE_EMBEDDED: Embedded call.
 *
 * Type of call to originate.
 */
typedef enum {
    QMI_WDS_CALL_TYPE_LAPTOP   = 0,
    QMI_WDS_CALL_TYPE_EMBEDDED = 1
} QmiWdsCallType;

/**
 * QmiWdsCallEndReason:
 * @QMI_WDS_CALL_END_REASON_GENERIC_UNSPECIFIED: Unspecified reason.
 * @QMI_WDS_CALL_END_REASON_GENERIC_CLIENT_END: Client end.
 * @QMI_WDS_CALL_END_REASON_GENERIC_NO_SERVICE: No service.
 * @QMI_WDS_CALL_END_REASON_GENERIC_FADE: Fade.
 * @QMI_WDS_CALL_END_REASON_GENERIC_RELEASE_NORMAL: Release normal.
 * @QMI_WDS_CALL_END_REASON_GENERIC_ACCESS_ATTEMPT_IN_PROGRESS: Access attempt in progress.
 * @QMI_WDS_CALL_END_REASON_GENERIC_ACCESS_FAILURE: Access Failure.
 * @QMI_WDS_CALL_END_REASON_GENERIC_REDIRECTION_OR_HANDOFF: Redirection or handoff.
 * @QMI_WDS_CALL_END_REASON_GENERIC_CLOSE_IN_PROGRESS: Close in progress.
 * @QMI_WDS_CALL_END_REASON_GENERIC_AUTHENTICATION_FAILED: Authentication failed.
 * @QMI_WDS_CALL_END_REASON_GENERIC_INTERNAL_ERROR: Internal error.
 * @QMI_WDS_CALL_END_REASON_CDMA_LOCK: (CDMA) Phone is CDMA-locked.
 * @QMI_WDS_CALL_END_REASON_CDMA_INTERCEPT: (CDMA) Received intercept from the BS.
 * @QMI_WDS_CALL_END_REASON_CDMA_REORDER: (CDMA) Received reorder from the BS.
 * @QMI_WDS_CALL_END_REASON_CDMA_RELEASE_SO_REJECT: (CDMA) Received release from the BS, SO reject.
 * @QMI_WDS_CALL_END_REASON_CDMA_INCOMING_CALL: (CDMA) Received incoming call from the BS.
 * @QMI_WDS_CALL_END_REASON_CDMA_ALERT_STOP: (CDMA) Received alert stop from the BS.
 * @QMI_WDS_CALL_END_REASON_CDMA_ACTIVATION: (CDMA) Received end activation.
 * @QMI_WDS_CALL_END_REASON_CDMA_MAX_ACCESS_PROBES: (CDMA) Maximum access probes transmitted.
 * @QMI_WDS_CALL_END_REASON_CDMA_CCS_NOT_SUPPORTED_BY_BS: (CDMA) Concurrent service not supported by the BS.
 * @QMI_WDS_CALL_END_REASON_CDMA_NO_RESPONSE_FROM_BS: (CDMA) No response received from the BS.
 * @QMI_WDS_CALL_END_REASON_CDMA_REJECTED_BY_BS: (CDMA) Rejected by the BS.
 * @QMI_WDS_CALL_END_REASON_CDMA_INCOMPATIBLE: (CDMA) Concurrent services requested are incompatible.
 * @QMI_WDS_CALL_END_REASON_CDMA_ALREADY_IN_TC: (CDMA) Already in TC.
 * @QMI_WDS_CALL_END_REASON_CDMA_USER_CALL_ORIGINATED_DURING_GPS: (CDMA) Call originated during GPS.
 * @QMI_WDS_CALL_END_REASON_CDMA_USER_CALL_ORIGINATED_DURING_SMS: (CDMA) Call originated during SMS.
 * @QMI_WDS_CALL_END_REASON_CDMA_NO_SERVICE: (CDMA) No service.
 * @QMI_WDS_CALL_END_REASON_GSM_WCDMA_CONFERENCE_FAILED: (GSM/WCDMA) Call origination request failed.
 * @QMI_WDS_CALL_END_REASON_GSM_WCDMA_INCOMING_REJECTED: (GSM/WCDMA) Client rejected incoming call.
 * @QMI_WDS_CALL_END_REASON_GSM_WCDMA_NO_SERVICE: (GSM/WCDMA) No service.
 * @QMI_WDS_CALL_END_REASON_GSM_WCDMA_NETWORK_END: (GSM/WCDMA) Network ended the call.
 * @QMI_WDS_CALL_END_REASON_GSM_WCDMA_LLC_SNDCP_FAILURE: (GSM/WCDMA) LLC or SNDCP failure.
 * @QMI_WDS_CALL_END_REASON_GSM_WCDMA_INSUFFICIENT_RESOURCES: (GSM/WCDMA) Insufficient resources.
 * @QMI_WDS_CALL_END_REASON_GSM_WCDMA_OPTION_TEMPORARILY_OUT_OF_ORDER: (GSM/WCDMA) Service option temporarily out of order.
 * @QMI_WDS_CALL_END_REASON_GSM_WCDMA_NSAPI_ALREADY_USED: (GSM/WCDMA) NSAPI already used.
 * @QMI_WDS_CALL_END_REASON_GSM_WCDMA_REGULAR_DEACTIVATION: (GSM/WCDMA) Regular PDP context deactivation.
 * @QMI_WDS_CALL_END_REASON_GSM_WCDMA_NETWORK_FAILURE: (GSM/WCDMA) Network failure.
 * @QMI_WDS_CALL_END_REASON_GSM_WCDMA_REATTACH_REQUIRED: (GSM/WCDMA) Reattach required.
 * @QMI_WDS_CALL_END_REASON_GSM_WCDMA_PROTOCOL_ERROR: (GSM/WCDMA) Protocol error.
 * @QMI_WDS_CALL_END_REASON_GSM_WCDMA_OPERATOR_DETERMINED_BARRING: (GSM/WCDMA) Operator-determined barring.
 * @QMI_WDS_CALL_END_REASON_GSM_WCDMA_UNKNOWN_APN: (GSM/WCDMA) Unknown or missing APN.
 * @QMI_WDS_CALL_END_REASON_GSM_WCDMA_UNKNOWN_PDP: (GSM/WCDMA) Unknown PDP address or type.
 * @QMI_WDS_CALL_END_REASON_GSM_WCDMA_GGSN_REJECT: (GSM/WCDMA) Activation rejected by GGSN.
 * @QMI_WDS_CALL_END_REASON_GSM_WCDMA_ACTIVATION_REJECT: (GSM/WCDMA) Activation rejected.
 * @QMI_WDS_CALL_END_REASON_GSM_WCDMA_OPTION_NOT_SUPPORTED: (GSM/WCDMA) Service option not supported.
 * @QMI_WDS_CALL_END_REASON_GSM_WCDMA_OPTION_UNSUBSCRIBED: (GSM/WCDMA) Service option not subscribed.
 * @QMI_WDS_CALL_END_REASON_GSM_WCDMA_QOS_NOT_ACCEPTED: (GSM/WCDMA) QoS not accepted.
 * @QMI_WDS_CALL_END_REASON_GSM_WCDMA_TFT_SEMANTIC_ERROR: (GSM/WCDMA) Semantic error in TFT operation.
 * @QMI_WDS_CALL_END_REASON_GSM_WCDMA_TFT_SYNTAX_ERROR: (GSM/WCDMA) Syntax error in TFT operation.
 * @QMI_WDS_CALL_END_REASON_GSM_WCDMA_UNKNOWN_PDP_CONTEXT: (GSM/WCDMA) Unknown PDP context.
 * @QMI_WDS_CALL_END_REASON_GSM_WCDMA_FILTER_SEMANTIC_ERROR: (GSM/WCDMA) Semantic error in packet filters.
 * @QMI_WDS_CALL_END_REASON_GSM_WCDMA_FILTER_SYNTAX_ERROR: (GSM/WCDMA) Syntax error in packet filters.
 * @QMI_WDS_CALL_END_REASON_GSM_WCDMA_PDP_WITHOUT_ACTIVE_TFT: (GSM/WCDMA) PDP context without TFT activated.
 * @QMI_WDS_CALL_END_REASON_GSM_WCDMA_INVALID_TRANSACTION_ID: (GSM/WCDMA) Invalid transaction ID.
 * @QMI_WDS_CALL_END_REASON_GSM_WCDMA_MESSAGE_INCORRECT_SEMANTIC: (GSM/WCDMA) Message incorrect semantically.
 * @QMI_WDS_CALL_END_REASON_GSM_WCDMA_INVALID_MANDATORY_INFO: (GSM/WCDMA) Invalid mandatory information.
 * @QMI_WDS_CALL_END_REASON_GSM_WCDMA_MESSAGE_TYPE_UNSUPPORTED: (GSM/WCDMA) Message type not implemented.
 * @QMI_WDS_CALL_END_REASON_GSM_WCDMA_MESSAGE_TYPE_NONCOMPATIBLE_STATE: (GSM/WCDMA) Message not compatible with state.
 * @QMI_WDS_CALL_END_REASON_GSM_WCDMA_UNKNOWN_INFO_ELEMENT: (GSM/WCDMA) Information element unknown.
 * @QMI_WDS_CALL_END_REASON_GSM_WCDMA_CONDITIONAL_IE_ERROR: (GSM/WCDMA) Conditional IE error.
 * @QMI_WDS_CALL_END_REASON_GSM_WCDMA_MESSAGE_AND_PROTOCOL_STATE_UNCOMPATIBLE: (GSM/WCDMA) Message and protocol state uncompatible.
 * @QMI_WDS_CALL_END_REASON_GSM_WCDMA_APN_TYPE_CONFLICT: (GSM/WCDMA) APN type conflict.
 * @QMI_WDS_CALL_END_REASON_GSM_WCDMA_NO_GPRS_CONTEXT: (GSM/WCDMA) No GPRS context.
 * @QMI_WDS_CALL_END_REASON_GSM_WCDMA_FEATURE_NOT_SUPPORTED: (GSM/WCDMA) Feature not supported.
 * @QMI_WDS_CALL_END_REASON_EVDO_CONNECTION_DENY_GENERAL_OR_BUSY: (EV-DO) Received Connection Deny (General or Network busy).
 * @QMI_WDS_CALL_END_REASON_EVDO_CONNECTION_DENY_BILLING_OR_AUTHENTICATION_FAILURE: (EV-DO) Received Connection Deny (Billing or Authentication failure).
 * @QMI_WDS_CALL_END_REASON_EVDO_HDR_CHANGE: (EV-DO) Change HDR.
 * @QMI_WDS_CALL_END_REASON_EVDO_HDR_EXIT: (EV-DO) Exit HDR.
 * @QMI_WDS_CALL_END_REASON_EVDO_HDR_NO_SESSION: (EV-DO) No HDR session.
 * @QMI_WDS_CALL_END_REASON_EVDO_HDR_ORIGINATION_DURING_GPS_FIX: (EV-DO) HDR call ended in favor of a GPS fix.
 * @QMI_WDS_CALL_END_REASON_EVDO_HDR_CONNECTION_SETUP_TIMEOUT: (EV-DO) Connection setup timeout.
 * @QMI_WDS_CALL_END_REASON_EVDO_HDR_RELEASED_BY_CM: (EV-DO) Released HDR call by call manager.
 *
 * Reason for ending the call.
 */
typedef enum {
    /* Generic reasons */
    QMI_WDS_CALL_END_REASON_GENERIC_UNSPECIFIED                = 1,
    QMI_WDS_CALL_END_REASON_GENERIC_CLIENT_END                 = 2,
    QMI_WDS_CALL_END_REASON_GENERIC_NO_SERVICE                 = 3,
    QMI_WDS_CALL_END_REASON_GENERIC_FADE                       = 4,
    QMI_WDS_CALL_END_REASON_GENERIC_RELEASE_NORMAL             = 5,
    QMI_WDS_CALL_END_REASON_GENERIC_ACCESS_ATTEMPT_IN_PROGRESS = 6,
    QMI_WDS_CALL_END_REASON_GENERIC_ACCESS_FAILURE             = 7,
    QMI_WDS_CALL_END_REASON_GENERIC_REDIRECTION_OR_HANDOFF     = 8,
    QMI_WDS_CALL_END_REASON_GENERIC_CLOSE_IN_PROGRESS          = 9,
    QMI_WDS_CALL_END_REASON_GENERIC_AUTHENTICATION_FAILED      = 10,
    QMI_WDS_CALL_END_REASON_GENERIC_INTERNAL_ERROR             = 11,

    /* CDMA specific reasons */
    QMI_WDS_CALL_END_REASON_CDMA_LOCK                            = 500,
    QMI_WDS_CALL_END_REASON_CDMA_INTERCEPT                       = 501,
    QMI_WDS_CALL_END_REASON_CDMA_REORDER                         = 502,
    QMI_WDS_CALL_END_REASON_CDMA_RELEASE_SO_REJECT               = 503,
    QMI_WDS_CALL_END_REASON_CDMA_INCOMING_CALL                   = 504,
    QMI_WDS_CALL_END_REASON_CDMA_ALERT_STOP                      = 505,
    QMI_WDS_CALL_END_REASON_CDMA_ACTIVATION                      = 506,
    QMI_WDS_CALL_END_REASON_CDMA_MAX_ACCESS_PROBES               = 507,
    QMI_WDS_CALL_END_REASON_CDMA_CCS_NOT_SUPPORTED_BY_BS         = 508,
    QMI_WDS_CALL_END_REASON_CDMA_NO_RESPONSE_FROM_BS             = 509,
    QMI_WDS_CALL_END_REASON_CDMA_REJECTED_BY_BS                  = 510,
    QMI_WDS_CALL_END_REASON_CDMA_INCOMPATIBLE                    = 511,
    QMI_WDS_CALL_END_REASON_CDMA_ALREADY_IN_TC                   = 512,
    QMI_WDS_CALL_END_REASON_CDMA_USER_CALL_ORIGINATED_DURING_GPS = 513,
    QMI_WDS_CALL_END_REASON_CDMA_USER_CALL_ORIGINATED_DURING_SMS = 514,
    QMI_WDS_CALL_END_REASON_CDMA_NO_SERVICE                      = 515,

    /* GSM/WCDMA specific reasons */
    QMI_WDS_CALL_END_REASON_GSM_WCDMA_CONFERENCE_FAILED                       = 1000,
    QMI_WDS_CALL_END_REASON_GSM_WCDMA_INCOMING_REJECTED                       = 1001,
    QMI_WDS_CALL_END_REASON_GSM_WCDMA_NO_SERVICE                              = 1002,
    QMI_WDS_CALL_END_REASON_GSM_WCDMA_NETWORK_END                             = 1003,
    QMI_WDS_CALL_END_REASON_GSM_WCDMA_LLC_SNDCP_FAILURE                       = 1004,
    QMI_WDS_CALL_END_REASON_GSM_WCDMA_INSUFFICIENT_RESOURCES                  = 1005,
    QMI_WDS_CALL_END_REASON_GSM_WCDMA_OPTION_TEMPORARILY_OUT_OF_ORDER         = 1006,
    QMI_WDS_CALL_END_REASON_GSM_WCDMA_NSAPI_ALREADY_USED                      = 1007,
    QMI_WDS_CALL_END_REASON_GSM_WCDMA_REGULAR_DEACTIVATION                    = 1008,
    QMI_WDS_CALL_END_REASON_GSM_WCDMA_NETWORK_FAILURE                         = 1009,
    QMI_WDS_CALL_END_REASON_GSM_WCDMA_REATTACH_REQUIRED                       = 1010,
    QMI_WDS_CALL_END_REASON_GSM_WCDMA_PROTOCOL_ERROR                          = 1011,
    QMI_WDS_CALL_END_REASON_GSM_WCDMA_OPERATOR_DETERMINED_BARRING             = 1012,
    QMI_WDS_CALL_END_REASON_GSM_WCDMA_UNKNOWN_APN                             = 1013,
    QMI_WDS_CALL_END_REASON_GSM_WCDMA_UNKNOWN_PDP                             = 1014,
    QMI_WDS_CALL_END_REASON_GSM_WCDMA_GGSN_REJECT                             = 1015,
    QMI_WDS_CALL_END_REASON_GSM_WCDMA_ACTIVATION_REJECT                       = 1016,
    QMI_WDS_CALL_END_REASON_GSM_WCDMA_OPTION_NOT_SUPPORTED                    = 1017,
    QMI_WDS_CALL_END_REASON_GSM_WCDMA_OPTION_UNSUBSCRIBED                     = 1018,
    QMI_WDS_CALL_END_REASON_GSM_WCDMA_QOS_NOT_ACCEPTED                        = 1019,
    QMI_WDS_CALL_END_REASON_GSM_WCDMA_TFT_SEMANTIC_ERROR                      = 1020,
    QMI_WDS_CALL_END_REASON_GSM_WCDMA_TFT_SYNTAX_ERROR                        = 1021,
    QMI_WDS_CALL_END_REASON_GSM_WCDMA_UNKNOWN_PDP_CONTEXT                     = 1022,
    QMI_WDS_CALL_END_REASON_GSM_WCDMA_FILTER_SEMANTIC_ERROR                   = 1023,
    QMI_WDS_CALL_END_REASON_GSM_WCDMA_FILTER_SYNTAX_ERROR                     = 1024,
    QMI_WDS_CALL_END_REASON_GSM_WCDMA_PDP_WITHOUT_ACTIVE_TFT                  = 1025,
    QMI_WDS_CALL_END_REASON_GSM_WCDMA_INVALID_TRANSACTION_ID                  = 1026,
    QMI_WDS_CALL_END_REASON_GSM_WCDMA_MESSAGE_INCORRECT_SEMANTIC              = 1027,
    QMI_WDS_CALL_END_REASON_GSM_WCDMA_INVALID_MANDATORY_INFO                  = 1028,
    QMI_WDS_CALL_END_REASON_GSM_WCDMA_MESSAGE_TYPE_UNSUPPORTED                = 1029,
    QMI_WDS_CALL_END_REASON_GSM_WCDMA_MESSAGE_TYPE_NONCOMPATIBLE_STATE        = 1030,
    QMI_WDS_CALL_END_REASON_GSM_WCDMA_UNKNOWN_INFO_ELEMENT                    = 1031,
    QMI_WDS_CALL_END_REASON_GSM_WCDMA_CONDITIONAL_IE_ERROR                    = 1032,
    QMI_WDS_CALL_END_REASON_GSM_WCDMA_MESSAGE_AND_PROTOCOL_STATE_UNCOMPATIBLE = 1033,
    QMI_WDS_CALL_END_REASON_GSM_WCDMA_APN_TYPE_CONFLICT                       = 1034,
    QMI_WDS_CALL_END_REASON_GSM_WCDMA_NO_GPRS_CONTEXT                         = 1035,
    QMI_WDS_CALL_END_REASON_GSM_WCDMA_FEATURE_NOT_SUPPORTED                   = 1036,

    /* EV-DO specific reasons */
    QMI_WDS_CALL_END_REASON_EVDO_CONNECTION_DENY_GENERAL_OR_BUSY                   = 1500,
    QMI_WDS_CALL_END_REASON_EVDO_CONNECTION_DENY_BILLING_OR_AUTHENTICATION_FAILURE = 1501,
    QMI_WDS_CALL_END_REASON_EVDO_HDR_CHANGE                                        = 1502,
    QMI_WDS_CALL_END_REASON_EVDO_HDR_EXIT                                          = 1503,
    QMI_WDS_CALL_END_REASON_EVDO_HDR_NO_SESSION                                    = 1504,
    QMI_WDS_CALL_END_REASON_EVDO_HDR_ORIGINATION_DURING_GPS_FIX                    = 1505,
    QMI_WDS_CALL_END_REASON_EVDO_HDR_CONNECTION_SETUP_TIMEOUT                      = 1506,
    QMI_WDS_CALL_END_REASON_EVDO_HDR_RELEASED_BY_CM                                = 1507
} QmiWdsCallEndReason;

/*****************************************************************************/
/* Helper enums for the 'QMI WDS Get Packet Service Status' message */

/**
 * QmiWdsConnectionStatus:
 * @QMI_WDS_CONNECTION_STATUS_UNKNOWN: Unknown status.
 * @QMI_WDS_CONNECTION_STATUS_DISCONNECTED: Network is disconnected
 * @QMI_WDS_CONNECTION_STATUS_CONNECTED: Network is connected.
 * @QMI_WDS_CONNECTION_STATUS_SUSPENDED: Network connection is suspended.
 * @QMI_WDS_CONNECTION_STATUS_AUTHENTICATING: Network authentication is ongoing.
 *
 * Status of the network connection.
 */
typedef enum {
    QMI_WDS_CONNECTION_STATUS_UNKNOWN        = 0,
    QMI_WDS_CONNECTION_STATUS_DISCONNECTED   = 1,
    QMI_WDS_CONNECTION_STATUS_CONNECTED      = 2,
    QMI_WDS_CONNECTION_STATUS_SUSPENDED      = 3,
    QMI_WDS_CONNECTION_STATUS_AUTHENTICATING = 4
} QmiWdsConnectionStatus;


/*****************************************************************************/
/* Helper enums for the 'QMI WDS Get Data Bearer Technology' message */

/**
 * QmiWdsDataBearerTechnology:
 * @QMI_WDS_DATA_BEARER_TECHNOLOGY_UNKNOWN: Unknown.
 * @QMI_WDS_DATA_BEARER_TECHNOLOGY_CDMA20001X: CDMA2000 1x.
 * @QMI_WDS_DATA_BEARER_TECHNOLOGY_1xEVDO: CDMA2000 HRPD 1xEV-DO.
 * @QMI_WDS_DATA_BEARER_TECHNOLOGY_GSM: GSM.
 * @QMI_WDS_DATA_BEARER_TECHNOLOGY_UMTS: UMTS.
 * @QMI_WDS_DATA_BEARER_TECHNOLOGY_1xEVDO_REVA: CDMA2000 HRPD 1xEV-DO RevA.
 * @QMI_WDS_DATA_BEARER_TECHNOLOGY_EDGE: EDGE.
 * @QMI_WDS_DATA_BEARER_TECHNOLOGY_HSDPA: HSDPA and WCDMA.
 * @QMI_WDS_DATA_BEARER_TECHNOLOGY_HSUPA: WCDMA and HSUPA.
 * @QMI_WDS_DATA_BEARER_TECHNOLOGY_HSDPA_HSUPDA: HSDPA and HSUPA.
 * @QMI_WDS_DATA_BEARER_TECHNOLOGY_LTE: LTE.
 * @QMI_WDS_DATA_BEARER_TECHNOLOGY_EHRPD: CDMA2000 eHRPD.
 * @QMI_WDS_DATA_BEARER_TECHNOLOGY_HSDPAPLUS: HSDPA+ and WCDMA.
 * @QMI_WDS_DATA_BEARER_TECHNOLOGY_HSDPAPLUS_HSUPA: HSDPA+ and HSUPA.
 * @QMI_WDS_DATA_BEARER_TECHNOLOGY_DCHSDPAPLUS: DC-HSDPA+ and WCDMA.
 * @QMI_WDS_DATA_BEARER_TECHNOLOGY_DCHSDPAPLUS_HSUPA: DC-HSDPA+ and HSUPA.
 *
 * Data bearer technology.
 */
typedef enum {
    QMI_WDS_DATA_BEARER_TECHNOLOGY_UNKNOWN           = -1,
    QMI_WDS_DATA_BEARER_TECHNOLOGY_CDMA20001X        = 0x01,
    QMI_WDS_DATA_BEARER_TECHNOLOGY_1xEVDO            = 0x02,
    QMI_WDS_DATA_BEARER_TECHNOLOGY_GSM               = 0x03,
    QMI_WDS_DATA_BEARER_TECHNOLOGY_UMTS              = 0x04,
    QMI_WDS_DATA_BEARER_TECHNOLOGY_1xEVDO_REVA       = 0x05,
    QMI_WDS_DATA_BEARER_TECHNOLOGY_EDGE              = 0x06,
    QMI_WDS_DATA_BEARER_TECHNOLOGY_HSDPA             = 0x07,
    QMI_WDS_DATA_BEARER_TECHNOLOGY_HSUPA             = 0x08,
    QMI_WDS_DATA_BEARER_TECHNOLOGY_HSDPA_HSUPDA      = 0x09,
    QMI_WDS_DATA_BEARER_TECHNOLOGY_LTE               = 0x0A,
    QMI_WDS_DATA_BEARER_TECHNOLOGY_EHRPD             = 0x0B,
    QMI_WDS_DATA_BEARER_TECHNOLOGY_HSDPAPLUS         = 0x0C,
    QMI_WDS_DATA_BEARER_TECHNOLOGY_HSDPAPLUS_HSUPA   = 0x0D,
    QMI_WDS_DATA_BEARER_TECHNOLOGY_DCHSDPAPLUS       = 0x0E,
    QMI_WDS_DATA_BEARER_TECHNOLOGY_DCHSDPAPLUS_HSUPA = 0x0F
} QmiWdsDataBearerTechnology;


/*****************************************************************************/
/* Helper enums for the 'QMI WDS Get Current Data Bearer Technology' message */

/**
 * QmiWdsNetworkType:
 * @QMI_WDS_NETWORK_TYPE_UNKNOWN: Unknown.
 * @QMI_WDS_NETWORK_TYPE_3GPP2: 3GPP2 network type.
 * @QMI_WDS_NETWORK_TYPE_3GPP: 3GPP network type.
 *
 * Network type of the data bearer.
 */
typedef enum {
    QMI_WDS_NETWORK_TYPE_UNKNOWN = 0,
    QMI_WDS_NETWORK_TYPE_3GPP2   = 1,
    QMI_WDS_NETWORK_TYPE_3GPP    = 2
} QmiWdsNetworkType;

/**
 * QmiWdsRat3gpp2:
 * @QMI_WDS_RAT_3GPP2_UNKNOWN: Unknown, to be ignored.
 * @QMI_WDS_RAT_3GPP2_CDMA1X: CDMA 1x.
 * @QMI_WDS_RAT_3GPP2_EVDO_REV0: EVDO Rev0.
 * @QMI_WDS_RAT_3GPP2_EVDO_REVA: EVDO RevA.
 * @QMI_WDS_RAT_3GPP2_EVDO_REVB: EVDO RevB.
 * @QMI_WDS_RAT_3GPP2_NULL_BEARER: No bearer.
 *
 * Flags specifying the 3GPP2-specific Radio Access Technology, when the data
 * bearer network type is @QMI_WDS_NETWORK_TYPE_3GPP2.
 */
typedef enum { /*< underscore_name=qmi_wds_rat_3gpp2 >*/
    QMI_WDS_RAT_3GPP2_NONE        = 0,
    QMI_WDS_RAT_3GPP2_CDMA1X      = 1 << 0,
    QMI_WDS_RAT_3GPP2_EVDO_REV0   = 1 << 1,
    QMI_WDS_RAT_3GPP2_EVDO_REVA   = 1 << 2,
    QMI_WDS_RAT_3GPP2_EVDO_REVB   = 1 << 3,
    QMI_WDS_RAT_3GPP2_NULL_BEARER = 1 << 15
} QmiWdsRat3gpp2;

/**
 * QmiWdsRat3gpp:
 * @QMI_WDS_RAT_3GPP_NONE: Unknown, to be ignored.
 * @QMI_WDS_RAT_3GPP_WCDMA: WCDMA.
 * @QMI_WDS_RAT_3GPP_GPRS: GPRS.
 * @QMI_WDS_RAT_3GPP_HSDPA: HSDPA.
 * @QMI_WDS_RAT_3GPP_HSUPA: HSUPA.
 * @QMI_WDS_RAT_3GPP_EDGE: EDGE.
 * @QMI_WDS_RAT_3GPP_LTE: LTE.
 * @QMI_WDS_RAT_3GPP_HSDPAPLUS: HSDPA+.
 * @QMI_WDS_RAT_3GPP_DCHSDPAPLUS: DC-HSDPA+
 * @QMI_WDS_RAT_3GPP_NULL_BEARER: No bearer.
 *
 * Flags specifying the 3GPP-specific Radio Access Technology, when the data
 * bearer network type is @QMI_WDS_NETWORK_TYPE_3GPP.
 */
typedef enum { /*< underscore_name=qmi_wds_rat_3gpp >*/
    QMI_WDS_RAT_3GPP_NONE        = 0,
    QMI_WDS_RAT_3GPP_WCDMA       = 1 << 0,
    QMI_WDS_RAT_3GPP_GPRS        = 1 << 1,
    QMI_WDS_RAT_3GPP_HSDPA       = 1 << 2,
    QMI_WDS_RAT_3GPP_HSUPA       = 1 << 3,
    QMI_WDS_RAT_3GPP_EDGE        = 1 << 4,
    QMI_WDS_RAT_3GPP_LTE         = 1 << 5,
    QMI_WDS_RAT_3GPP_HSDPAPLUS   = 1 << 6,
    QMI_WDS_RAT_3GPP_DCHSDPAPLUS = 1 << 7,
    QMI_WDS_RAT_3GPP_NULL_BEARER = 1 << 15
} QmiWdsRat3gpp;

/**
 * QmiWdsSoCdma1x:
 * @QMI_WDS_SO_CDMA1X_NONE: Unknown, to be ignored.
 * @QMI_WDS_SO_CDMA1X_IS95: IS95.
 * @QMI_WDS_SO_CDMA1X_IS2000: IS2000.
 * @QMI_WDS_SO_CDMA1X_IS2000_REL_A: IS2000 RelA.
 *
 * Flags specifying the Service Option when the bearer network type is
 * @QMI_WDS_NETWORK_TYPE_3GPP2 and when the Radio Access Technology mask
 * contains @QMI_WDS_RAT_3GPP2_CDMA1X.
 */
typedef enum {
    QMI_WDS_SO_CDMA1X_NONE         = 0,
    QMI_WDS_SO_CDMA1X_IS95         = 1 << 0,
    QMI_WDS_SO_CDMA1X_IS2000       = 1 << 1,
    QMI_WDS_SO_CDMA1X_IS2000_REL_A = 1 << 2
} QmiWdsSoCdma1x;

/**
 * QmiWdsSoEvdoRevA:
 * @QMI_WDS_SO_EVDO_REVA_NONE: Unknown, to be ignored.
 * @QMI_WDS_SO_EVDO_REVA_DPA: DPA.
 * @QMI_WDS_SO_EVDO_REVA_MFPA: MFPA.
 * @QMI_WDS_SO_EVDO_REVA_EMPA: EMPA.
 * @QMI_WDS_SO_EVDO_REVA_EMPA_EHRPD: EMPA EHRPD.
 *
 * Flags specifying the Service Option when the bearer network type is
 * @QMI_WDS_NETWORK_TYPE_3GPP2 and when the Radio Access Technology mask
 * contains @QMI_WDS_RAT_3GPP2_EVDO_REVA.
 */
typedef enum { /*< underscore_name=qmi_wds_so_evdo_reva >*/
    QMI_WDS_SO_EVDO_REVA_NONE       = 0,
    QMI_WDS_SO_EVDO_REVA_DPA        = 1 << 0,
    QMI_WDS_SO_EVDO_REVA_MFPA       = 1 << 1,
    QMI_WDS_SO_EVDO_REVA_EMPA       = 1 << 2,
    QMI_WDS_SO_EVDO_REVA_EMPA_EHRPD = 1 << 3
} QmiWdsSoEvdoRevA;

#endif /* _LIBQMI_GLIB_QMI_ENUMS_WDS_H_ */
