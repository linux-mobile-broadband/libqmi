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
 */

#ifndef _LIBQMI_GLIB_QMI_ENUMS_DSD_H_
#define _LIBQMI_GLIB_QMI_ENUMS_DSD_H_

#if !defined (__LIBQMI_GLIB_H_INSIDE__) && !defined (LIBQMI_GLIB_COMPILATION)
#error "Only <libqmi-glib.h> can be included directly."
#endif

/**
 * SECTION: qmi-enums-dsd
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

#endif /* _LIBQMI_GLIB_QMI_ENUMS_DSD_H_ */
