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

#ifndef _LIBQMI_GLIB_QMI_FLAGS64_DSD_H_
#define _LIBQMI_GLIB_QMI_FLAGS64_DSD_H_

#if !defined (__LIBQMI_GLIB_H_INSIDE__) && !defined (LIBQMI_GLIB_COMPILATION)
#error "Only <libqmi-glib.h> can be included directly."
#endif

#include "qmi-enums-dsd.h"
#include <glib.h>

/**
 * QmiDsdApnTypePreference:
 * @QMI_DSD_APN_TYPE_PREFERENCE_DEFAULT: Default/Internet traffic.
 * @QMI_DSD_APN_TYPE_PREFERENCE_IMS: IMS.
 * @QMI_DSD_APN_TYPE_PREFERENCE_MMS: Multimedia Messaging Service.
 * @QMI_DSD_APN_TYPE_PREFERENCE_DUN: Dial Up Network.
 * @QMI_DSD_APN_TYPE_PREFERENCE_SUPL: Secure User Plane Location.
 * @QMI_DSD_APN_TYPE_PREFERENCE_HIPRI: High Priority Mobile Data.
 * @QMI_DSD_APN_TYPE_PREFERENCE_FOTA: over the air administration.
 * @QMI_DSD_APN_TYPE_PREFERENCE_CBS: Carrier Branded Services.
 * @QMI_DSD_APN_TYPE_PREFERENCE_IA: Initial Attach.
 * @QMI_DSD_APN_TYPE_PREFERENCE_EMERGENCY: Emergency.
 *
 * APN type preference as a bitmask.
 *
 * Since: 1.26
 */
typedef enum { /*< since=1.26 >*/
    QMI_DSD_APN_TYPE_PREFERENCE_DEFAULT   = ((guint64) 1) << QMI_DSD_APN_TYPE_DEFAULT,
    QMI_DSD_APN_TYPE_PREFERENCE_IMS       = ((guint64) 1) << QMI_DSD_APN_TYPE_IMS,
    QMI_DSD_APN_TYPE_PREFERENCE_MMS       = ((guint64) 1) << QMI_DSD_APN_TYPE_MMS,
    QMI_DSD_APN_TYPE_PREFERENCE_DUN       = ((guint64) 1) << QMI_DSD_APN_TYPE_DUN,
    QMI_DSD_APN_TYPE_PREFERENCE_SUPL      = ((guint64) 1) << QMI_DSD_APN_TYPE_SUPL,
    QMI_DSD_APN_TYPE_PREFERENCE_HIPRI     = ((guint64) 1) << QMI_DSD_APN_TYPE_HIPRI,
    QMI_DSD_APN_TYPE_PREFERENCE_FOTA      = ((guint64) 1) << QMI_DSD_APN_TYPE_FOTA,
    QMI_DSD_APN_TYPE_PREFERENCE_CBS       = ((guint64) 1) << QMI_DSD_APN_TYPE_CBS,
    QMI_DSD_APN_TYPE_PREFERENCE_IA        = ((guint64) 1) << QMI_DSD_APN_TYPE_IA,
    QMI_DSD_APN_TYPE_PREFERENCE_EMERGENCY = ((guint64) 1) << QMI_DSD_APN_TYPE_EMERGENCY,
} QmiDsdApnTypePreference;

#endif /* _LIBQMI_GLIB_QMI_FLAGS64_DSD_H_ */
