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

#ifndef _LIBQMI_GLIB_QMI_ENUMS_IMS_H_
#define _LIBQMI_GLIB_QMI_ENUMS_IMS_H_

#if !defined (__LIBQMI_GLIB_H_INSIDE__) && !defined (LIBQMI_GLIB_COMPILATION)
#error "Only <libqmi-glib.h> can be included directly."
#endif

/**
 * SECTION: qmi-enums-ims
 * @title: IMS enumerations and flags
 * @short_description: Enumerations and flags in the IMS service.
 *
 * This section defines enumerations and flags used in the IMS service
 * interface.
 */

/**
 * QmiImsCallModePreference:
 * @QMI_IMS_CALL_MODE_PREFERENCE_NONE: None
 * @QMI_IMS_CALL_MODE_PREFERENCE_CELLUAR: Prefer Celluar
 * @QMI_IMS_CALL_MODE_PREFERENCE_WIFI: Prefer Wifi
 * @QMI_IMS_CALL_MODE_PREFERENCE_WIFI_ONLY: Only Wifi
 * @QMI_IMS_CALL_MODE_PREFERENCE_CELLULAR_ONLY: Only Celluar
 * @QMI_IMS_CALL_MODE_PREFERENCE_IMS: IMS
 *
 * IMS Call Mode Preference
 *
 * Since: 1.37
 */
typedef enum { /*< since=1.37 >*/
    QMI_IMS_CALL_MODE_PREFERENCE_NONE          = 0x00,
    QMI_IMS_CALL_MODE_PREFERENCE_CELLULAR      = 0x01,
    QMI_IMS_CALL_MODE_PREFERENCE_WIFI          = 0x02,
    QMI_IMS_CALL_MODE_PREFERENCE_WIFI_ONLY     = 0x03,
    QMI_IMS_CALL_MODE_PREFERENCE_CELLULAR_ONLY = 0x04,
    QMI_IMS_CALL_MODE_PREFERENCE_IMS           = 0x05,
} QmiImsCallModePreference;

#endif /* _LIBQMI_GLIB_QMI_ENUMS_IMS_H_ */
