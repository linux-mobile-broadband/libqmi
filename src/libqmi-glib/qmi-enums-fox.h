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
 * Copyright (C) 2022 Freedom Liu <lk@linuxdev.top>
 */

#ifndef _LIBQMI_GLIB_QMI_ENUMS_FOX_H_
#define _LIBQMI_GLIB_QMI_ENUMS_FOX_H_

#if !defined (__LIBQMI_GLIB_H_INSIDE__) && !defined (LIBQMI_GLIB_COMPILATION)
#error "Only <libqmi-glib.h> can be included directly."
#endif

/**
 * SECTION: qmi-enums-fox
 * @title: FOX enumerations and flags
 * @short_description: Enumerations and flags in the FOX service.
 *
 * This section defines enumerations and flags used in the FOX service
 * interface.
 */

/**
 * QmiFoxFirmwareVersionType:
 * @QMI_FOX_FIRMWARE_VERSION_TYPE_FIRMWARE_MCFG: E.g. T99W265.F0.0.0.0.1.GC.004.
 * @QMI_FOX_FIRMWARE_VERSION_TYPE_FIRMWARE_MCFG_APPS: E.g. T99W265.F0.0.0.0.1.GC.004.001.
 * @QMI_FOX_FIRMWARE_VERSION_TYPE_APPS: E.g. 001.
 *
 * Foxconn specific firmware version types.
 *
 * Since: 1.32
 */
typedef enum { /*< since=1.32 >*/
    QMI_FOX_FIRMWARE_VERSION_TYPE_FIRMWARE_MCFG      = 0x00,
    QMI_FOX_FIRMWARE_VERSION_TYPE_FIRMWARE_MCFG_APPS = 0x01,
    QMI_FOX_FIRMWARE_VERSION_TYPE_APPS               = 0x02,
} QmiFoxFirmwareVersionType;

#endif /* _LIBQMI_GLIB_QMI_ENUMS_FOX_H_ */

