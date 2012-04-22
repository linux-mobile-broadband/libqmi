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

#ifndef _LIBQMI_GLIB_QMI_ENUMS_H_
#define _LIBQMI_GLIB_QMI_ENUMS_H_

typedef enum {
    /* Unknown service */
    QMI_SERVICE_UNKNOWN = -1,
    /* Control service */
    QMI_SERVICE_CTL = 0x00,
    /* Wireless Data Service */
    QMI_SERVICE_WDS = 0x01,
    /* Device Management Service */
    QMI_SERVICE_DMS = 0x02,
    /* Network Access Service */
    QMI_SERVICE_NAS = 0x03,
    /* Quality Of Service service */
    QMI_SERVICE_QOS = 0x04,
    /* Wireless Messaging Service */
    QMI_SERVICE_WMS = 0x05,
    /* Position Determination Service */
    QMI_SERVICE_PDS = 0x06,
    /* Authentication service */
    QMI_SERVICE_AUTH = 0x07,
    /* AT service */
	QMI_SERVICE_AT = 0x08,
    /* Voice service */
    QMI_SERVICE_VOICE = 0x09,

	QMI_SERVICE_CAT2 = 0x0A,
	QMI_SERVICE_UIM = 0x0B,
	QMI_SERVICE_PBM = 0x0C,
	QMI_SERVICE_LOC = 0x10,
	QMI_SERVICE_SAR = 0x11,
	QMI_SERVICE_RMTFS = 0x14,

    /* Card Application Toolkit service */
    QMI_SERVICE_CAT = 0xE0,
    /* Remote Management Service */
    QMI_SERVICE_RMS = 0xE1,
    /* Open Mobile Alliance device management service */
    QMI_SERVICE_OMA = 0xE2
} QmiService;

/*****************************************************************************/
/* QMI Control */

typedef enum {
    QMI_CTL_MESSAGE_SET_INSTANCE_ID        = 0x0020, /* unused currently */
    QMI_CTL_MESSAGE_GET_VERSION_INFO       = 0x0021,
    QMI_CTL_MESSAGE_ALLOCATE_CLIENT_ID     = 0x0022,
    QMI_CTL_MESSAGE_RELEASE_CLIENT_ID      = 0x0023,
    QMI_CTL_MESSAGE_REVOKE_CLIENT_ID       = 0x0024, /* unused currently */
    QMI_CTL_MESSAGE_INVALID_CLIENT_ID      = 0x0025, /* unused currently */
    QMI_CTL_MESSAGE_SET_DATA_FORMAT        = 0x0026, /* unused currently */
    QMI_CTL_MESSAGE_SYNC                   = 0x0027, /* unused currently */
    QMI_CTL_MESSAGE_EVENT                  = 0x0028, /* unused currently */
    QMI_CTL_MESSAGE_SET_POWER_SAVE_CONFIG  = 0x0029, /* unused currently */
    QMI_CTL_MESSAGE_SET_POWER_SAVE_MODE    = 0x002A, /* unused currently */
    QMI_CTL_MESSAGE_GET_POWER_SAVE_MODE    = 0x002B, /* unused currently */
} QmiCtlMessage;

typedef enum {
    QMI_CTL_FLAG_NONE       = 0,
    QMI_CTL_FLAG_RESPONSE   = 1 << 0,
    QMI_CTL_FLAG_INDICATION = 1 << 1,
} QmiCtlFlag;

/*****************************************************************************/
/* QMI Services */

typedef enum {
    QMI_SERVICE_FLAG_NONE       = 0,
    QMI_SERVICE_FLAG_COMPOUND   = 1 << 0,
    QMI_SERVICE_FLAG_RESPONSE   = 1 << 1,
    QMI_SERVICE_FLAG_INDICATION = 1 << 2,
} QmiServiceFlag;

#define QMI_CID_NONE      0x00
#define QMI_CID_BROADCAST 0xFF

typedef enum {
    QMI_DMS_MESSAGE_EVENT              = 0x0001, /* unused currently */
    QMI_DMS_MESSAGE_GET_MANUFACTURER   = 0x0021, /* unused currently */
    QMI_DMS_MESSAGE_GET_IDS            = 0x0025,
    QMI_DMS_MESSAGE_GET_OPERATING_MODE = 0x002D, /* unused currently */
    QMI_DMS_MESSAGE_SET_OPERATING_MODE = 0x002E, /* unused currently */
} QmiDmsMessage;

#endif /* _LIBQMI_GLIB_QMI_ENUMS_H_ */
