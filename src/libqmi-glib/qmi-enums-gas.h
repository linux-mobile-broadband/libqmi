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
 * Copyright (C) 2019 Andreas Kling <awesomekling@gmail.com>
 */

#ifndef _LIBQMI_GLIB_QMI_ENUMS_GAS_H_
#define _LIBQMI_GLIB_QMI_ENUMS_GAS_H_

#if !defined (__LIBQMI_GLIB_H_INSIDE__) && !defined (LIBQMI_GLIB_COMPILATION)
#error "Only <libqmi-glib.h> can be included directly."
#endif

/**
 * SECTION: qmi-enums-gas
 * @title: GAS enumerations and flags
 * @short_description: Enumerations and flags in the GAS service.
 *
 * This section defines enumerations and flags used in the GAS service
 * interface.
 */

/**
 * QmiGasFirmwareListingMode:
 * @QMI_GAS_FIRMWARE_LISTING_MODE_ACTIVE_FIRMWARE: List only the active firmware.
 * @QMI_GAS_FIRMWARE_LISTING_MODE_ALL_FIRMWARE: List all stored firmwares.
 * @QMI_GAS_FIRMWARE_LISTING_MODE_SPECIFIC_FIRMWARE: List only specific firmware with condition.
 *
 * Mode when retrieving a list of stored firmwares.
 *
 * Since: 1.24
 */
typedef enum { /*< since=1.24 >*/
    QMI_GAS_FIRMWARE_LISTING_MODE_ACTIVE_FIRMWARE   = 0,
    QMI_GAS_FIRMWARE_LISTING_MODE_ALL_FIRMWARE      = 1,
    QMI_GAS_FIRMWARE_LISTING_MODE_SPECIFIC_FIRMWARE = 2,
} QmiGasFirmwareListingMode;

/**
 * QmiGasUsbCompositionEndpointType:
 * @QMI_GAS_USB_COMPOSITION_ENDPOINT_TYPE_HSUSB: High-speed USB.
 * @QMI_GAS_USB_COMPOSITION_ENDPOINT_TYPE_HSIC: High-speed inter-chip interface.
 *
 * Peripheral endpoint type.
 *
 * Since: 1.32
 */
typedef enum { /*< since=1.32 >*/
    QMI_GAS_USB_COMPOSITION_ENDPOINT_TYPE_HSUSB = 0,
    QMI_GAS_USB_COMPOSITION_ENDPOINT_TYPE_HSIC  = 1,
} QmiGasUsbCompositionEndpointType;

/**
 * QmiGasFirmwareAutoSimMode:
 * @QMI_GAS_FIRMWARE_AUTO_SIM_DISABLE: Disable the automatic carrier switch according to the SIM.
 * @QMI_GAS_FIRMWARE_AUTO_SIM_ENABLE: Enable the automatic carrier switch according to the SIM.
 * @QMI_GAS_FIRMWARE_AUTO_SIM_ENABLE_ONE_SHOT: Enable the automatic carrier switch according to the SIM just for one time.
 * @QMI_GAS_FIRMWARE_AUTO_SIM_PENDING_ONE_SHOT: Indicate the pending status of a single switch (used only in DMS Get Firmware Auto Sim).
 *
 * Automatic carrier switch according to the SIM mode.
 *
 * Since: 1.36
 */
typedef enum { /*< since=1.36 >*/
    QMI_GAS_FIRMWARE_AUTO_SIM_DISABLE          = 0,
    QMI_GAS_FIRMWARE_AUTO_SIM_ENABLE           = 1,
    QMI_GAS_FIRMWARE_AUTO_SIM_ENABLE_ONE_SHOT  = 2,
    QMI_GAS_FIRMWARE_AUTO_SIM_PENDING_ONE_SHOT = 3,
} QmiGasFirmwareAutoSimMode;

#endif /* _LIBQMI_GLIB_QMI_ENUMS_GAS_H_ */
