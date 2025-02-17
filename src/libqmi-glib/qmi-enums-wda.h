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

#ifndef _LIBQMI_GLIB_QMI_ENUMS_WDA_H_
#define _LIBQMI_GLIB_QMI_ENUMS_WDA_H_

#if !defined (__LIBQMI_GLIB_H_INSIDE__) && !defined (LIBQMI_GLIB_COMPILATION)
#error "Only <libqmi-glib.h> can be included directly."
#endif

/**
 * SECTION: qmi-enums-wda
 * @title: WDA enumerations and flags
 * @short_description: Enumerations and flags in the WDA service.
 *
 * This section defines enumerations and flags used in the WDA service
 * interface.
 */

/**
 * QmiWdaLinkLayerProtocol:
 * @QMI_WDA_LINK_LAYER_PROTOCOL_UNKNOWN: Unknown.
 * @QMI_WDA_LINK_LAYER_PROTOCOL_802_3: 802.3 ethernet mode.
 * @QMI_WDA_LINK_LAYER_PROTOCOL_RAW_IP: Raw IP mode.
 *
 * Link layer protocol.
 *
 * Since: 1.10
 */
typedef enum { /*< since=1.10 >*/
    QMI_WDA_LINK_LAYER_PROTOCOL_UNKNOWN = 0x00,
    QMI_WDA_LINK_LAYER_PROTOCOL_802_3   = 0x01,
    QMI_WDA_LINK_LAYER_PROTOCOL_RAW_IP  = 0x02,
} QmiWdaLinkLayerProtocol;

/**
 * QmiWdaDataAggregationProtocol:
 * @QMI_WDA_DATA_AGGREGATION_PROTOCOL_DISABLED: Disabled.
 * @QMI_WDA_DATA_AGGREGATION_PROTOCOL_TLP: TLP enabled.
 * @QMI_WDA_DATA_AGGREGATION_PROTOCOL_QC_NCM: QC NCM enabled.
 * @QMI_WDA_DATA_AGGREGATION_PROTOCOL_MBIM: MBIM enabled.
 * @QMI_WDA_DATA_AGGREGATION_PROTOCOL_RNDIS: RNDIS enabled.
 * @QMI_WDA_DATA_AGGREGATION_PROTOCOL_QMAP: QMAP enabled.
 * @QMI_WDA_DATA_AGGREGATION_PROTOCOL_QMAPV2: QMAPV2 enabled. Since 1.30.
 * @QMI_WDA_DATA_AGGREGATION_PROTOCOL_QMAPV3: QMAPV3 enabled. Since 1.30.
 * @QMI_WDA_DATA_AGGREGATION_PROTOCOL_QMAPV4: QMAPV4 enabled. Since 1.30.
 * @QMI_WDA_DATA_AGGREGATION_PROTOCOL_QMAPV5: QMAPV5 enabled. Since 1.28.
 *
 * Data aggregation protocol in uplink or downlink.
 *
 * Since: 1.10
 */
typedef enum { /*< since=1.10 >*/
    QMI_WDA_DATA_AGGREGATION_PROTOCOL_DISABLED = 0x00,
    QMI_WDA_DATA_AGGREGATION_PROTOCOL_TLP      = 0x01,
    QMI_WDA_DATA_AGGREGATION_PROTOCOL_QC_NCM   = 0x02,
    QMI_WDA_DATA_AGGREGATION_PROTOCOL_MBIM     = 0x03,
    QMI_WDA_DATA_AGGREGATION_PROTOCOL_RNDIS    = 0x04,
    QMI_WDA_DATA_AGGREGATION_PROTOCOL_QMAP     = 0x05,
    QMI_WDA_DATA_AGGREGATION_PROTOCOL_QMAPV2   = 0x06,
    QMI_WDA_DATA_AGGREGATION_PROTOCOL_QMAPV3   = 0x07,
    QMI_WDA_DATA_AGGREGATION_PROTOCOL_QMAPV4   = 0x08,
    QMI_WDA_DATA_AGGREGATION_PROTOCOL_QMAPV5   = 0x09,
} QmiWdaDataAggregationProtocol;

/**
 * QmiWdaLoopBackState:
 * @QMI_WDA_LOOPBACK_DISABLED: Disabled.
 * @QMI_WDA_LOOPBACK_ENABLED: Enabled.
 *
 * Loopback configuration state.
 *
 * Since: 1.36
 */
typedef enum { /*< since=1.36 >*/
    QMI_WDA_LOOPBACK_DISABLED = 0x00,
    QMI_WDA_LOOPBACK_ENABLED  = 0x01,
} QmiWdaLoopBackState;

#endif /* _LIBQMI_GLIB_QMI_ENUMS_WDA_H_ */
