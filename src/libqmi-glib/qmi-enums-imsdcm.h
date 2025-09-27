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
 * Copyright (C) 2025 Alexander Couzens <lynxis@fe80.eu>
 */

#ifndef _LIBQMI_GLIB_QMI_ENUMS_IMSDCM_H_
#define _LIBQMI_GLIB_QMI_ENUMS_IMSDCM_H_

/**
 * SECTION: qmi-enums-imsdcm
 * @title: IMSDCM enumerations and flags
 * @short_description: Enumerations and flags in the IMSDCM service.
 *
 * This section defines enumerations and flags used in the IMSDCM service
 * interface.
 */

/**
 * QmiImsDcmApnType:
 * @QMI_IMSDCM_APN_TYPE_IMS: generic IMS
 * @QMI_IMSDCM_APN_TYPE_INTERNET: generic data
 * @QMI_IMSDCM_APN_TYPE_EMERGENCY: Emergency
 * @QMI_IMSDCM_APN_TYPE_RCS: RCS services
 * @QMI_IMSDCM_APN_TYPE_UT: User Terminal
 * @QMI_IMSDCM_APN_TYPE_WLAN: for WLAN
 *
 * IMS DCM APN type.
 *
 * Since: 1.37
 */
typedef enum { /*< since=1.37 >*/
    QMI_IMSDCM_APN_TYPE_IMS       = 0x00,
    QMI_IMSDCM_APN_TYPE_INTERNET  = 0x01,
    QMI_IMSDCM_APN_TYPE_EMERGENCY = 0x02,
    QMI_IMSDCM_APN_TYPE_RCS       = 0x03,
    QMI_IMSDCM_APN_TYPE_UT        = 0x04,
    QMI_IMSDCM_APN_TYPE_WLAN      = 0x05,
} QmiImsDcmApnType;

/**
 * QmiImsDcmIpFamily:
 * @QMI_IMSDCM_IP_FAMILY_V4: IPv4
 * @QMI_IMSDCM_IP_FAMILY_V6: IPv6
 *
 * IMS DCM IP familiy.
 *
 * Since: 1.37
 */
typedef enum { /*< since=1.37 >*/
    QMI_IMSDCM_IP_FAMILY_V4 = 0x00,
    QMI_IMSDCM_IP_FAMILY_V6 = 0x01,
} QmiImsDcmIpFamiliy;

/**
 * QmiImsDcmRatType:
 * @QMI_IMSDCM_RAT_TYPE_EHPRD: CDMA/EHPRD
 * @QMI_IMSDCM_RAT_TYPE_LTE: IMS is initialized, but not registered yet with the network IMS service.
 * @QMI_IMSDCM_RAT_TYPE_EPC: IMS is initialized but device is in airplane mode.
 * @QMI_IMSDCM_RAT_TYPE_WLAN: IMS is initialized and registered.
 *
 * IMS DCM RAT type.
 *
 * Since: 1.37
 */
typedef enum { /*< since=1.37 >*/
    QMI_IMSDCM_RAT_TYPE_EHPRD = 0x00,
    QMI_IMSDCM_RAT_TYPE_LTE   = 0x01,
    QMI_IMSDCM_RAT_TYPE_EPC   = 0x02,
    QMI_IMSDCM_RAT_TYPE_WLAN  = 0x03,
} QmiImsDcmRatType;

/**
 * QmiImsDcmInstanceId:
 * @QMI_IMSDCM_INSTANCE_NONE: None
 * @QMI_IMSDCM_INSTANCE_GLOBAL: Global
 * @QMI_IMSDCM_INSTANCE_ID1: ID1
 * @QMI_IMSDCM_INSTANCE_ID2: ID2
 *
 * IMS DCM Instance Id
 *
 * Since: 1.37
 */
typedef enum { /*< since=1.37 >*/
    QMI_IMSDCM_INSTANCE_GLOBAL = 0x01,
    QMI_IMSDCM_INSTANCE_ID1    = 0x02,
    QMI_IMSDCM_INSTANCE_ID2    = 0x03,
    QMI_IMSDCM_INSTANCE_NONE   = 0xFF, /* Unsure if 0xff is correct */
} QmiImsDcmInstanceId;

#endif /* _LIBQMI_GLIB_QMI_ENUMS_IMSDCM_H_ */
