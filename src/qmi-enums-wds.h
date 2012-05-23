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
