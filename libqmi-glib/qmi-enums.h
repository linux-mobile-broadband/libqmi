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
 * Copyright (C) 2012 Google, Inc.
 */

#include "qmi-enums-wds.h"

#ifndef _LIBQMI_GLIB_QMI_ENUMS_H_
#define _LIBQMI_GLIB_QMI_ENUMS_H_

/**
 * QmiService:
 * @QMI_SERVICE_UNKNOWN: Unknown service.
 * @QMI_SERVICE_CTL: Control service.
 * @QMI_SERVICE_WDS: Wireless Data Service.
 * @QMI_SERVICE_DMS: Device Management Service.
 * @QMI_SERVICE_NAS: Network Access Service.
 * @QMI_SERVICE_QOS: Quality Of Service service.
 * @QMI_SERVICE_WMS: Wireless Messaging Service.
 * @QMI_SERVICE_PDS: Position Determination Service.
 * @QMI_SERVICE_AUTH: Authentication service.
 * @QMI_SERVICE_AT: AT service.
 * @QMI_SERVICE_VOICE: Voice service.
 * @QMI_SERVICE_CAT2: Card Application Toolkit service (v2).
 * @QMI_SERVICE_UIM: User Identity Module service.
 * @QMI_SERVICE_PBM: Phonebook Management service.
 * @QMI_SERVICE_LOC: Location service (~ PDS v2).
 * @QMI_SERVICE_SAR: SAR.
 * @QMI_SERVICE_RMTFS: Remote Filesystem service.
 * @QMI_SERVICE_CAT: Card Application Toolkit service (v1).
 * @QMI_SERVICE_RMS: Remote Management Service.
 * @QMI_SERVICE_OMA: Open Mobile Alliance device management service.
 *
 * QMI services.
 */
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
    /* Card Application Toolkit service (major version 2) */
    QMI_SERVICE_CAT2 = 0x0A,
    /* User Identity Module service */
    QMI_SERVICE_UIM = 0x0B,
    /* Phonebook Management service */
    QMI_SERVICE_PBM = 0x0C,
    /* Location service (~ PDS major version 2) */
    QMI_SERVICE_LOC = 0x10,
    /* No idea what this one means.. Search And Rescue? */
    QMI_SERVICE_SAR = 0x11,
    /* Remote Filesystem service */
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

/**
 * QmiCtlDataFormat:
 * @QMI_CTL_DATA_FORMAT_QOS_FLOW_HEADER_ABSENT: QoS header absent
 * @QMI_CTL_DATA_FORMAT_QOS_FLOW_HEADER_PRESENT: QoS header present
 *
 * Controls whether the network port data format includes a QoS header or not.
 * Should normally be set to ABSENT.
 */
typedef enum {
    QMI_CTL_DATA_FORMAT_QOS_FLOW_HEADER_ABSENT  = 0,
    QMI_CTL_DATA_FORMAT_QOS_FLOW_HEADER_PRESENT = 1,
} QmiCtlDataFormat;

/**
 * QmiCtlDataLinkProtocol:
 * @QMI_CTL_DATA_LINK_PROTOCOL_802_3: data frames formatted as 802.3 Ethernet
 * @QMI_CTL_DATA_LINK_PROTOCOL_RAW_IP: data frames are raw IP packets
 *
 * Determines the network port data format.  Despite looking like flags, these
 * values are actually mutually exclusive.
 */
typedef enum {
    QMI_CTL_DATA_LINK_PROTOCOL_802_3  = 1 << 0,
    QMI_CTL_DATA_LINK_PROTOCOL_RAW_IP = 1 << 1,
} QmiCtlDataLinkProtocol;

/**
 * QmiCtlFlag:
 * @QMI_CTL_FLAG_NONE: None.
 * @QMI_CTL_FLAG_RESPONSE: Message is a response.
 * @QMI_CTL_FLAG_INDICATION: Message is an indication.
 *
 * QMI flags in messages of the %QMI_SERVICE_CTL service.
 */
typedef enum {
    QMI_CTL_FLAG_NONE       = 0,
    QMI_CTL_FLAG_RESPONSE   = 1 << 0,
    QMI_CTL_FLAG_INDICATION = 1 << 1
} QmiCtlFlag;

/**
 * QmiServiceFlag:
 * @QMI_SERVICE_FLAG_NONE: None.
 * @QMI_SERVICE_FLAG_COMPOUND: Message is compound.
 * @QMI_SERVICE_FLAG_RESPONSE: Message is a response.
 * @QMI_SERVICE_FLAG_INDICATION: Message is an indication.
 *
 * QMI flags in messages which are not of the %QMI_SERVICE_CTL service.
 */
typedef enum {
    QMI_SERVICE_FLAG_NONE       = 0,
    QMI_SERVICE_FLAG_COMPOUND   = 1 << 0,
    QMI_SERVICE_FLAG_RESPONSE   = 1 << 1,
    QMI_SERVICE_FLAG_INDICATION = 1 << 2
} QmiServiceFlag;

#endif /* _LIBQMI_GLIB_QMI_ENUMS_H_ */
