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
 * Copyright (C) 2013 Google Inc.
 * Copyright (C) 2013-2017 Aleksander Morgado <aleksander@aleksander.es>
 */

#ifndef _LIBQMI_GLIB_QMI_ENUMS_OMA_H_
#define _LIBQMI_GLIB_QMI_ENUMS_OMA_H_

#if !defined (__LIBQMI_GLIB_H_INSIDE__) && !defined (LIBQMI_GLIB_COMPILATION)
#error "Only <libqmi-glib.h> can be included directly."
#endif

/**
 * SECTION: qmi-enums-oma
 * @title: OMA enumerations and flags
 *
 * This section defines enumerations and flags used in the OMA service
 * interface.
 */

/**
 * QmiOmaSessionType:
 * @QMI_OMA_SESSION_TYPE_CLIENT_INITIATED_DEVICE_CONFIGURE: Client-initiated device configure.
 * @QMI_OMA_SESSION_TYPE_CLIENT_INITIATED_PRL_UPDATE: Client-initiated PRL update.
 * @QMI_OMA_SESSION_TYPE_CLIENT_INITIATED_HANDS_FREE_ACTIVATION: Client-initiated hands free activation.
 * @QMI_OMA_SESSION_TYPE_DEVICE_INITIATED_HANDS_FREE_ACTIVATION: Device-initiated hands free activation.
 * @QMI_OMA_SESSION_TYPE_NETWORK_INITIATED_PRL_UPDATE: Network-initiated PRL update.
 * @QMI_OMA_SESSION_TYPE_NETWORK_INITIATED_DEVICE_CONFIGURE: Network-initiated device configure.
 * @QMI_OMA_SESSION_TYPE_DEVICE_INITIATED_PRL_UPDATE: Device-initiated PRL update.
 *
 * Type of OMA-DM session.
 *
 * Since: 1.6
 */
typedef enum {
    QMI_OMA_SESSION_TYPE_CLIENT_INITIATED_DEVICE_CONFIGURE      = 0,
    QMI_OMA_SESSION_TYPE_CLIENT_INITIATED_PRL_UPDATE            = 1,
    QMI_OMA_SESSION_TYPE_CLIENT_INITIATED_HANDS_FREE_ACTIVATION = 2,
    QMI_OMA_SESSION_TYPE_DEVICE_INITIATED_HANDS_FREE_ACTIVATION = 3,
    QMI_OMA_SESSION_TYPE_NETWORK_INITIATED_PRL_UPDATE           = 4,
    QMI_OMA_SESSION_TYPE_NETWORK_INITIATED_DEVICE_CONFIGURE     = 5,
    QMI_OMA_SESSION_TYPE_DEVICE_INITIATED_PRL_UPDATE            = 6
} QmiOmaSessionType;

/**
 * qmi_oma_session_type_get_string:
 *
 * Since: 1.6
 */

/**
 * QmiOmaSessionState:
 * @QMI_OMA_SESSION_STATE_COMPLETE_INFORMATION_UPDATED: Session complete and information updated.
 * @QMI_OMA_SESSION_STATE_COMPLETE_UPDATED_INFORMATION_UNAVAILABLE: Session complete but updated information not available.
 * @QMI_OMA_SESSION_STATE_FAILED: Session failed.
 * @QMI_OMA_SESSION_STATE_RETRYING: Session retrying.
 * @QMI_OMA_SESSION_STATE_CONNECTING: Session connecting.
 * @QMI_OMA_SESSION_STATE_CONNECTED: Session connected.
 * @QMI_OMA_SESSION_STATE_AUTHENTICATED: Session authenticated.
 * @QMI_OMA_SESSION_STATE_MDN_DOWNLOADED: MDN downloaded.
 * @QMI_OMA_SESSION_STATE_MSID_DOWNLOADED: MSID downloaded.
 * @QMI_OMA_SESSION_STATE_PRL_DOWNLOADED: PRL downloaded.
 * @QMI_OMA_SESSION_STATE_MIP_PROFILE_DOWNLOADED: MIP profile downloaded.
 *
 * State of the OMA-DM session.
 *
 * Since: 1.6
 */
typedef enum {
    QMI_OMA_SESSION_STATE_COMPLETE_INFORMATION_UPDATED             = 0,
    QMI_OMA_SESSION_STATE_COMPLETE_UPDATED_INFORMATION_UNAVAILABLE = 1,
    QMI_OMA_SESSION_STATE_FAILED                                   = 2,
    QMI_OMA_SESSION_STATE_RETRYING                                 = 3,
    QMI_OMA_SESSION_STATE_CONNECTING                               = 4,
    QMI_OMA_SESSION_STATE_CONNECTED                                = 5,
    QMI_OMA_SESSION_STATE_AUTHENTICATED                            = 6,
    QMI_OMA_SESSION_STATE_MDN_DOWNLOADED                           = 7,
    QMI_OMA_SESSION_STATE_MSID_DOWNLOADED                          = 8,
    QMI_OMA_SESSION_STATE_PRL_DOWNLOADED                           = 9,
    QMI_OMA_SESSION_STATE_MIP_PROFILE_DOWNLOADED                   = 10
} QmiOmaSessionState;

/**
 * qmi_oma_session_state_get_string:
 *
 * Since: 1.6
 */

/**
 * QmiOmaSessionFailedReason:
 * @QMI_OMA_SESSION_FAILED_REASON_UNKNOWN: Unknown reason.
 * @QMI_OMA_SESSION_FAILED_REASON_NETWORK_UNAVAILABLE: Network unavailable.
 * @QMI_OMA_SESSION_FAILED_REASON_SERVER_UNAVAILABLE: Server unavailable.
 * @QMI_OMA_SESSION_FAILED_REASON_AUTHENTICATION_FAILED: Authentication failed.
 * @QMI_OMA_SESSION_FAILED_REASON_MAX_RETRY_EXCEEDED: Maximum retries exceeded.
 * @QMI_OMA_SESSION_FAILED_REASON_SESSION_CANCELLED: Session cancelled.
 *
 * Session failure reason.
 *
 * Since: 1.6
 */
typedef enum {
    QMI_OMA_SESSION_FAILED_REASON_UNKNOWN               = 0,
    QMI_OMA_SESSION_FAILED_REASON_NETWORK_UNAVAILABLE   = 1,
    QMI_OMA_SESSION_FAILED_REASON_SERVER_UNAVAILABLE    = 2,
    QMI_OMA_SESSION_FAILED_REASON_AUTHENTICATION_FAILED = 3,
    QMI_OMA_SESSION_FAILED_REASON_MAX_RETRY_EXCEEDED    = 4,
    QMI_OMA_SESSION_FAILED_REASON_SESSION_CANCELLED     = 5
} QmiOmaSessionFailedReason;

/**
 * qmi_oma_session_failed_reason_get_string:
 *
 * Since: 1.6
 */

/**
 * QmiOmaHfaFeatureDoneState:
 * @QMI_OMA_HFA_FEATURE_DONE_STATE_NONE: None.
 * @QMI_OMA_HFA_FEATURE_DONE_STATE_SUCCEEDED: Succeeded.
 * @QMI_OMA_HFA_FEATURE_DONE_STATE_FAILED: Failed.
 *
 * HFA feature done state.
 *
 * Since: 1.6
 */
typedef enum {
    QMI_OMA_HFA_FEATURE_DONE_STATE_NONE      = 0,
    QMI_OMA_HFA_FEATURE_DONE_STATE_SUCCEEDED = 1,
    QMI_OMA_HFA_FEATURE_DONE_STATE_FAILED    = 2
} QmiOmaHfaFeatureDoneState;

/**
 * qmi_oma_hfa_feature_done_state_get_string:
 *
 * Since: 1.6
 */

#endif /* _LIBQMI_GLIB_QMI_ENUMS_OMA_H_ */
