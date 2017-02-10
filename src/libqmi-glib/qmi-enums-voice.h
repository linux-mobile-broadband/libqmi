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
 * Copyright (C) 2014-2017 Aleksander Morgado <aleksander@aleksander.es>
 */

#ifndef _LIBQMI_GLIB_QMI_ENUMS_VOICE_H_
#define _LIBQMI_GLIB_QMI_ENUMS_VOICE_H_

#if !defined (__LIBQMI_GLIB_H_INSIDE__) && !defined (LIBQMI_GLIB_COMPILATION)
#error "Only <libqmi-glib.h> can be included directly."
#endif

/**
 * SECTION: qmi-enums-voice
 * @title: VOICE enumerations and flags
 *
 * This section defines enumerations and flags used in the VOICE service
 * interface.
 */

/*****************************************************************************/
/* Helper enums for the 'QMI Voice All Call Status' indication */

/**
 * QmiVoiceCallState:
 * @QMI_VOICE_CALL_STATE_UNKNOWN: Unknown state.
 * @QMI_VOICE_CALL_STATE_ORIGINATION: Call is being originated.
 * @QMI_VOICE_CALL_STATE_INCOMING: Incoming call.
 * @QMI_VOICE_CALL_STATE_CONVERSATION: Call is in progress.
 * @QMI_VOICE_CALL_STATE_CC_IN_PROGRESS: Call is originating but waiting for call control to complete.
 * @QMI_VOICE_CALL_STATE_ALERTING: Alerting.
 * @QMI_VOICE_CALL_STATE_HOLD: On hold.
 * @QMI_VOICE_CALL_STATE_WAITING: Waiting.
 * @QMI_VOICE_CALL_STATE_DISCONNECTING: Disconnecting.
 * @QMI_VOICE_CALL_STATE_END: Call is finished.
 * @QMI_VOICE_CALL_STATE_SETUP: MT call is in setup state (3GPP).
 *
 * State of a call.
 *
 * Since: 1.14
 */
typedef enum {
    QMI_VOICE_CALL_STATE_UNKNOWN        = 0x00,
    QMI_VOICE_CALL_STATE_ORIGINATION    = 0x01,
    QMI_VOICE_CALL_STATE_INCOMING       = 0x02,
    QMI_VOICE_CALL_STATE_CONVERSATION   = 0x03,
    QMI_VOICE_CALL_STATE_CC_IN_PROGRESS = 0x04,
    QMI_VOICE_CALL_STATE_ALERTING       = 0x05,
    QMI_VOICE_CALL_STATE_HOLD           = 0x06,
    QMI_VOICE_CALL_STATE_WAITING        = 0x07,
    QMI_VOICE_CALL_STATE_DISCONNECTING  = 0x08,
    QMI_VOICE_CALL_STATE_END            = 0x09,
    QMI_VOICE_CALL_STATE_SETUP          = 0x0A,
} QmiVoiceCallState;

/**
 * qmi_voice_call_state_get_string:
 *
 * Since: 1.14
 */

/**
 * QmiVoiceCallType:
 * @QMI_VOICE_CALL_TYPE_VOICE: Voice call.
 * @QMI_VOICE_CALL_TYPE_VOICE_IP: VoIP call.
 * @QMI_VOICE_CALL_TYPE_OTAPA: OTAPA.
 * @QMI_VOICE_CALL_TYPE_NON_STD_OTASP: Non-standard OTASP.
 * @QMI_VOICE_CALL_TYPE_EMERGENCY: Emergency call.
 * @QMI_VOICE_CALL_TYPE_SUPS: Supplementary service.
 *
 * Type of a voice call.
 *
 * Since: 1.14
 */
typedef enum {
    QMI_VOICE_CALL_TYPE_VOICE         = 0x00,
    QMI_VOICE_CALL_TYPE_VOICE_IP      = 0x02,
    QMI_VOICE_CALL_TYPE_OTAPA         = 0x06,
    QMI_VOICE_CALL_TYPE_NON_STD_OTASP = 0x08,
    QMI_VOICE_CALL_TYPE_EMERGENCY     = 0x09,
    QMI_VOICE_CALL_TYPE_SUPS          = 0x0A,
} QmiVoiceCallType;

/**
 * qmi_voice_call_type_get_string:
 *
 * Since: 1.14
 */

/**
 * QmiVoiceCallDirection:
 * @QMI_VOICE_CALL_DIRECTION_UNKNOWN: Unknown.
 * @QMI_VOICE_CALL_DIRECTION_MO: Mobile-originated.
 * @QMI_VOICE_CALL_DIRECTION_MT: Mobile-terminated.
 *
 * Call direction.
 *
 * Since: 1.14
 */
typedef enum {
    QMI_VOICE_CALL_DIRECTION_UNKNOWN = 0x00,
    QMI_VOICE_CALL_DIRECTION_MO      = 0x01,
    QMI_VOICE_CALL_DIRECTION_MT      = 0x02,
} QmiVoiceCallDirection;

/**
 * qmi_voice_call_direction_get_string:
 *
 * Since: 1.14
 */

/**
 * QmiVoiceCallMode:
 * @QMI_VOICE_CALL_MODE_UNKNOWN: Unknown.
 * @QMI_VOICE_CALL_MODE_CDMA: CDMA.
 * @QMI_VOICE_CALL_MODE_GSM: GSM.
 * @QMI_VOICE_CALL_MODE_UMTS: UMTS.
 * @QMI_VOICE_CALL_MODE_LTE: LTE.
 *
 * Call mode.
 *
 * Since: 1.14
 */
typedef enum {
    QMI_VOICE_CALL_MODE_UNKNOWN = 0x00,
    QMI_VOICE_CALL_MODE_CDMA    = 0x01,
    QMI_VOICE_CALL_MODE_GSM     = 0x02,
    QMI_VOICE_CALL_MODE_UMTS    = 0x03,
    QMI_VOICE_CALL_MODE_LTE     = 0x04,
} QmiVoiceCallMode;

/**
 * qmi_voice_call_mode_get_string:
 *
 * Since: 1.14
 */

/**
 * QmiVoiceAls:
 * @QMI_VOICE_ALS_LINE_1: Line 1 (default).
 * @QMI_VOICE_ALS_LINE_2: Line 2.
 *
 * ALS line indicator.
 *
 * Since: 1.14
 */
typedef enum {
    QMI_VOICE_ALS_LINE_1 = 0x00,
    QMI_VOICE_ALS_LINE_2 = 0x01,
} QmiVoiceAls;

/**
 * qmi_voice_als_get_string:
 *
 * Since: 1.14
 */

/**
 * QmiVoicePresentation:
 * @QMI_VOICE_PRESENTATION_ALLOWED: Allowed presentation.
 * @QMI_VOICE_PRESENTATION_RESTRICTED: Restricted presentation.
 * @QMI_VOICE_PRESENTATION_UNAVAILABLE: Unavailable presentation.
 * @QMI_VOICE_PRESENTATION_PAYPHONE: Payphone presentation (3GPP only).
 *
 * Remote party number presentation indicator.
 *
 * Since: 1.14
 */
typedef enum {
    QMI_VOICE_PRESENTATION_ALLOWED     = 0x00,
    QMI_VOICE_PRESENTATION_RESTRICTED  = 0x01,
    QMI_VOICE_PRESENTATION_UNAVAILABLE = 0x02,
    QMI_VOICE_PRESENTATION_PAYPHONE    = 0x04,
} QmiVoicePresentation;

/**
 * qmi_voice_presentation_get_string:
 *
 * Since: 1.14
 */

/*****************************************************************************/
/* Helper enums for the 'QMI Voice Get Config' request/response */

/**
 * QmiVoiceTtyMode:
 * @QMI_VOICE_TTY_MODE_FULL: Full.
 * @QMI_VOICE_TTY_MODE_VCO: Voice carry over.
 * @QMI_VOICE_TTY_MODE_HCO: Hearing carry over.
 * @QMI_VOICE_TTY_MODE_OFF: Off.
 *
 * TTY mode.
 *
 * Since: 1.14
 */
typedef enum {
    QMI_VOICE_TTY_MODE_FULL = 0x00,
    QMI_VOICE_TTY_MODE_VCO  = 0x01,
    QMI_VOICE_TTY_MODE_HCO  = 0x02,
    QMI_VOICE_TTY_MODE_OFF  = 0x03,
} QmiVoiceTtyMode;

/**
 * qmi_voice_tty_mode_get_string:
 *
 * Since: 1.14
 */

/**
 * QmiVoiceServiceOption:
 * @QMI_VOICE_SERVICE_OPTION_WILD: Any service option.
 * @QMI_VOICE_SERVICE_OPTION_IS_96A: IS-96A.
 * @QMI_VOICE_SERVICE_OPTION_EVRC: EVRC.
 * @QMI_VOICE_SERVICE_OPTION_13K_IS733: IS733.
 * @QMI_VOICE_SERVICE_OPTION_SELECTABLE_MODE_VOCODER: Selectable mode vocoder.
 * @QMI_VOICE_SERVICE_OPTION_4GV_NARROW_BAND: 4GV narrowband.
 * @QMI_VOICE_SERVICE_OPTION_4GV_WIDE_BAND: 4GV wideband.
 * @QMI_VOICE_SERVICE_OPTION_13K: 13K.
 * @QMI_VOICE_SERVICE_OPTION_IS_96: IS-96.
 * @QMI_VOICE_SERVICE_OPTION_WVRC: WVRC.
 *
 * Service option.
 *
 * Since: 1.14
 */
typedef enum {
    QMI_VOICE_SERVICE_OPTION_WILD                    = 0x0000,
    QMI_VOICE_SERVICE_OPTION_IS_96A                  = 0x0001,
    QMI_VOICE_SERVICE_OPTION_EVRC                    = 0x0003,
    QMI_VOICE_SERVICE_OPTION_13K_IS733               = 0x0011,
    QMI_VOICE_SERVICE_OPTION_SELECTABLE_MODE_VOCODER = 0x0038,
    QMI_VOICE_SERVICE_OPTION_4GV_NARROW_BAND         = 0x0044,
    QMI_VOICE_SERVICE_OPTION_4GV_WIDE_BAND           = 0x0046,
    QMI_VOICE_SERVICE_OPTION_13K                     = 0x8000,
    QMI_VOICE_SERVICE_OPTION_IS_96                   = 0x8001,
    QMI_VOICE_SERVICE_OPTION_WVRC                    = 0x8023,
} QmiVoiceServiceOption;

/**
 * qmi_voice_service_option_get_string:
 *
 * Since: 1.14
 */

/**
 * QmiVoiceWcdmaAmrStatus:
 * @QMI_VOICE_WCDMA_AMR_STATUS_NOT_SUPPORTED: Not supported.
 * @QMI_VOICE_WCDMA_AMR_STATUS_WCDMA_AMR_WB: WCDMA AMR wideband.
 * @QMI_VOICE_WCDMA_AMR_STATUS_GSM_HR_AMR: GSM half-rate AMR.
 * @QMI_VOICE_WCDMA_AMR_STATUS_GSM_AMR_WB: GSM AMR wideband.
 * @QMI_VOICE_WCDMA_AMR_STATUS_GSM_AMR_NB: GSM AMR narrowband.
 *
 * WCDMA AMR status.
 *
 * Since: 1.14
 */
typedef enum {
    QMI_VOICE_WCDMA_AMR_STATUS_NOT_SUPPORTED = 1 << 0,
    QMI_VOICE_WCDMA_AMR_STATUS_WCDMA_AMR_WB  = 1 << 1,
    QMI_VOICE_WCDMA_AMR_STATUS_GSM_HR_AMR    = 1 << 2,
    QMI_VOICE_WCDMA_AMR_STATUS_GSM_AMR_WB    = 1 << 3,
    QMI_VOICE_WCDMA_AMR_STATUS_GSM_AMR_NB    = 1 << 4,
} QmiVoiceWcdmaAmrStatus;

/**
 * qmi_voice_wcdma_amr_status_build_string_from_mask:
 *
 * Since: 1.14
 */

/**
 * QmiVoicePrivacy:
 * @QMI_VOICE_PRIVACY_STANDARD: Standard.
 * @QMI_VOICE_PRIVACY_ENHANCED: Enhanced.
 *
 * Voice privacy.
 *
 * Since: 1.14
 */
typedef enum {
    QMI_VOICE_PRIVACY_STANDARD = 0x00,
    QMI_VOICE_PRIVACY_ENHANCED = 0x01,
} QmiVoicePrivacy;

/**
 * qmi_voice_privacy_get_string:
 *
 * Since: 1.14
 */

/**
 * QmiVoiceDomain:
 * @QMI_VOICE_DOMAIN_CS_ONLY: CS-only.
 * @QMI_VOICE_DOMAIN_PS_ONLY: PS-only.
 * @QMI_VOICE_DOMAIN_CS_PREFERRED: CS preferred, PS secondary.
 * @QMI_VOICE_DOMAIN_PS_PREFERRED: PS preferred, CS secondary.
 *
 * Voice domain preference.
 *
 * Since: 1.14
 */
typedef enum {
    QMI_VOICE_DOMAIN_CS_ONLY      = 0x00,
    QMI_VOICE_DOMAIN_PS_ONLY      = 0x01,
    QMI_VOICE_DOMAIN_CS_PREFERRED = 0x02,
    QMI_VOICE_DOMAIN_PS_PREFERRED = 0x03,
} QmiVoiceDomain;

/**
 * qmi_voice_domain_get_string:
 *
 * Since: 1.14
 */

#endif /* _LIBQMI_GLIB_QMI_ENUMS_VOICE_H_ */
