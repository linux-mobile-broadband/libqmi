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

G_END_DECLS

#endif /* _LIBMBIM_GLIB_MBIM_ENUMS_H_ */
