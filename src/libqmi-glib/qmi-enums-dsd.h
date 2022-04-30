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
 * Copyright (C) 2019 Wang Jing <clifflily@hotmail.com>
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc.
 */

#ifndef _LIBQMI_GLIB_QMI_ENUMS_DSD_H_
#define _LIBQMI_GLIB_QMI_ENUMS_DSD_H_

#if !defined (__LIBQMI_GLIB_H_INSIDE__) && !defined (LIBQMI_GLIB_COMPILATION)
#error "Only <libqmi-glib.h> can be included directly."
#endif

/**
 * SECTION: qmi-enums-dsd
 * @title: DSD enumerations and flags
 * @short_description: Enumerations and flags in the DSD service.
 *
 * This section defines enumerations and flags used in the DSD service
 * interface.
 */

 /*****************************************************************************/

/**
 * QmiDsdApnType:
 * @QMI_DSD_APN_TYPE_DEFAULT: Default/Internet traffic.
 * @QMI_DSD_APN_TYPE_IMS: IMS.
 * @QMI_DSD_APN_TYPE_MMS: Multimedia Messaging Service.
 * @QMI_DSD_APN_TYPE_DUN: Dial Up Network.
 * @QMI_DSD_APN_TYPE_SUPL: Secure User Plane Location.
 * @QMI_DSD_APN_TYPE_HIPRI: High Priority Mobile Data.
 * @QMI_DSD_APN_TYPE_FOTA: over the air administration.
 * @QMI_DSD_APN_TYPE_CBS: Carrier Branded Services.
 * @QMI_DSD_APN_TYPE_IA: Initial Attach.
 * @QMI_DSD_APN_TYPE_EMERGENCY: Emergency.
 *
 * APN type.
 *
 * Since: 1.26
 */
typedef enum { /*< since=1.26 >*/
    QMI_DSD_APN_TYPE_DEFAULT   = 0,
    QMI_DSD_APN_TYPE_IMS       = 1,
    QMI_DSD_APN_TYPE_MMS       = 2,
    QMI_DSD_APN_TYPE_DUN       = 3,
    QMI_DSD_APN_TYPE_SUPL      = 4,
    QMI_DSD_APN_TYPE_HIPRI     = 5,
    QMI_DSD_APN_TYPE_FOTA      = 6,
    QMI_DSD_APN_TYPE_CBS       = 7,
    QMI_DSD_APN_TYPE_IA        = 8,
    QMI_DSD_APN_TYPE_EMERGENCY = 9,
} QmiDsdApnType;

/**
 * QmiDsdDataSystemNetworkType:
 * @QMI_DSD_DATA_SYSTEM_NETWORK_TYPE_3GPP: 3GPP network type.
 * @QMI_DSD_DATA_SYSTEM_NETWORK_TYPE_3GPP2: 3GPP2 network type.
 * @QMI_DSD_DATA_SYSTEM_NETWORK_TYPE_WLAN: WLAN network type.
 *
 * Network type of the data system.
 *
 * Since: 1.32
 */
typedef enum { /*< since=1.32 >*/
    QMI_DSD_DATA_SYSTEM_NETWORK_TYPE_3GPP  = 0,
    QMI_DSD_DATA_SYSTEM_NETWORK_TYPE_3GPP2 = 1,
    QMI_DSD_DATA_SYSTEM_NETWORK_TYPE_WLAN  = 2,
} QmiDsdDataSystemNetworkType;

/**
 * QmiDsdRadioAccessTechnology:
 * @QMI_DSD_RADIO_ACCESS_TECHNOLOGY_UNKNOWN: Unknown.
 * @QMI_DSD_RADIO_ACCESS_TECHNOLOGY_3GPP_WCDMA: WCDMA.
 * @QMI_DSD_RADIO_ACCESS_TECHNOLOGY_3GPP_GERAN: GERAN.
 * @QMI_DSD_RADIO_ACCESS_TECHNOLOGY_3GPP_LTE: LTE.
 * @QMI_DSD_RADIO_ACCESS_TECHNOLOGY_3GPP_TDSCDMA: TD-SDCMA.
 * @QMI_DSD_RADIO_ACCESS_TECHNOLOGY_3GPP_WLAN: 3GPP WLAN.
 * @QMI_DSD_RADIO_ACCESS_TECHNOLOGY_3GPP_5G: 5G.
 * @QMI_DSD_RADIO_ACCESS_TECHNOLOGY_3GPP2_1X: CDMA 1x.
 * @QMI_DSD_RADIO_ACCESS_TECHNOLOGY_3GPP2_HRPD: CDMA EVDO, HRPD.
 * @QMI_DSD_RADIO_ACCESS_TECHNOLOGY_3GPP2_EHRPD: CDMA EVDO with eHRPD.
 * @QMI_DSD_RADIO_ACCESS_TECHNOLOGY_3GPP2_WLAN: 3GPP2 WLAN.
 *
 * Radio access technology.
 *
 * Since: 1.32
 */
typedef enum { /*< since=1.32 >*/
    QMI_DSD_RADIO_ACCESS_TECHNOLOGY_UNKNOWN       = 0,
    QMI_DSD_RADIO_ACCESS_TECHNOLOGY_3GPP_WCDMA    = 1,
    QMI_DSD_RADIO_ACCESS_TECHNOLOGY_3GPP_GERAN    = 2,
    QMI_DSD_RADIO_ACCESS_TECHNOLOGY_3GPP_LTE      = 3,
    QMI_DSD_RADIO_ACCESS_TECHNOLOGY_3GPP_TDSCDMA  = 4,
    QMI_DSD_RADIO_ACCESS_TECHNOLOGY_3GPP_WLAN     = 5,
    QMI_DSD_RADIO_ACCESS_TECHNOLOGY_3GPP_5G       = 6,
    QMI_DSD_RADIO_ACCESS_TECHNOLOGY_3GPP2_1X      = 101,
    QMI_DSD_RADIO_ACCESS_TECHNOLOGY_3GPP2_HRPD    = 102,
    QMI_DSD_RADIO_ACCESS_TECHNOLOGY_3GPP2_EHRPD   = 103,
    QMI_DSD_RADIO_ACCESS_TECHNOLOGY_3GPP2_WLAN    = 104,
} QmiDsdRadioAccessTechnology;

#endif /* _LIBQMI_GLIB_QMI_ENUMS_DSD_H_ */
