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
 * Copyright (C) 2012 Google Inc.
 */

#ifndef _LIBQMI_GLIB_QMI_ENUMS_WMS_H_
#define _LIBQMI_GLIB_QMI_ENUMS_WMS_H_

/*****************************************************************************/
/* Helper enums for the 'QMI WMS Event Report' indication */

/**
 * QmiWmsStorageType:
 * @QMI_WMS_STORAGE_TYPE_UIM: Message stored in UIM.
 * @QMI_WMS_STORAGE_TYPE_NV: Message stored in non-volatile memory.
 *
 * Type of messaging storage
 */
typedef enum {
    QMI_WMS_STORAGE_TYPE_UIM = 0x00,
    QMI_WMS_STORAGE_TYPE_NV  = 0x01
} QmiWmsStorageType;

/**
 * QmiWmsAckIndicator:
 * @QMI_WMS_ACK_INDICATOR_SEND: ACK needs to be sent.
 * @QMI_WMS_ACK_INDICATOR_DO_NOT_SEND: ACK doesn't need to be sent.
 *
 * Indication of whether ACK needs to be sent or not.
 */
typedef enum {
    QMI_WMS_ACK_INDICATOR_SEND        = 0x00,
    QMI_WMS_ACK_INDICATOR_DO_NOT_SEND = 0x01
} QmiWmsAckIndicator;

/**
 * QmiWmsMessageFormat:
 * @QMI_WMS_MESSAGE_FORMAT_CDMA: CDMA message.
 * @QMI_WMS_MESSAGE_FORMAT_GSM_WCDMA_POINT_TO_POINT: Point-to-point 3GPP message.
 * @QMI_WMS_MESSAGE_FORMAT_GSM_WCDMA_BROADCAST: Broadcast 3GPP message.
 *
 * Type of message.
 */
typedef enum {
    QMI_WMS_MESSAGE_FORMAT_CDMA                     = 0x00,
    QMI_WMS_MESSAGE_FORMAT_GSM_WCDMA_POINT_TO_POINT = 0x06,
    QMI_WMS_MESSAGE_FORMAT_GSM_WCDMA_BROADCAST      = 0x07
} QmiWmsMessageFormat;

/**
 * QmiWmsMessageMode:
 * @QMI_WMS_MESSAGE_MODE_CDMA: Message sent using 3GPP2 technologies.
 * @QMI_WMS_MESSAGE_MODE_GSM_WCDMA: Message sent using 3GPP technologies.
 *
 * Message mode.
 */
typedef enum {
    QMI_WMS_MESSAGE_MODE_CDMA      = 0x00,
    QMI_WMS_MESSAGE_MODE_GSM_WCDMA = 0x01
} QmiWmsMessageMode;

/**
 * QmiWmsNotificationType:
 * @QMI_WMS_NOTIFICATION_TYPE_PRIMARY: Primary.
 * @QMI_WMS_NOTIFICATION_TYPE_SECONDARY_GSM: Secondary GSM.
 * @QMI_WMS_NOTIFICATION_TYPE_SECONDARY_UMTS: Secondary UMTS.
 *
 * Type of notification.
 */
typedef enum {
    QMI_WMS_NOTIFICATION_TYPE_PRIMARY        = 0x00,
    QMI_WMS_NOTIFICATION_TYPE_SECONDARY_GSM  = 0x01,
    QMI_WMS_NOTIFICATION_TYPE_SECONDARY_UMTS = 0x02
} QmiWmsNotificationType;

#endif /* _LIBQMI_GLIB_QMI_ENUMS_WMS_H_ */
