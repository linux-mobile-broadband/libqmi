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
 * Copyright (C) 2017 Aleksander Morgado <aleksander@aleksander.es>
 */

#include <glib.h>
#include "qmi-enums-dms.h"

/*****************************************************************************/
/* Helper for the 'QMI DMS Swi Get USB Composition' message */

static const gchar *usb_composition_description[] = {
    [QMI_DMS_SWI_USB_COMPOSITION_0]  = "HIP, DM, NMEA, AT, MDM1, MDM2, MDM3, MS",
    [QMI_DMS_SWI_USB_COMPOSITION_1]  = "HIP, DM, NMEA, AT, MDM1, MS",
    [QMI_DMS_SWI_USB_COMPOSITION_2]  = "HIP, DM, NMEA, AT, NIC1, MS",
    [QMI_DMS_SWI_USB_COMPOSITION_3]  = "HIP, DM, NMEA, AT, MDM1, NIC1, MS",
    [QMI_DMS_SWI_USB_COMPOSITION_4]  = "HIP, DM, NMEA, AT, NIC1, NIC2, NIC3, MS",
    [QMI_DMS_SWI_USB_COMPOSITION_5]  = "HIP, DM, NMEA, AT, ECM1, MS",
    [QMI_DMS_SWI_USB_COMPOSITION_6]  = "DM, NMEA, AT, QMI",
    [QMI_DMS_SWI_USB_COMPOSITION_7]  = "DM, NMEA, AT, RMNET1, RMNET2, RMNET3",
    [QMI_DMS_SWI_USB_COMPOSITION_8]  = "DM, NMEA, AT, MBIM",
    [QMI_DMS_SWI_USB_COMPOSITION_9]  = "MBIM",
    [QMI_DMS_SWI_USB_COMPOSITION_10] = "NMEA, MBIM",
    [QMI_DMS_SWI_USB_COMPOSITION_11] = "DM, MBIM",
    [QMI_DMS_SWI_USB_COMPOSITION_12] = "DM, NMEA, MBIM",
    [QMI_DMS_SWI_USB_COMPOSITION_13] = "Dual configuration: USB composition 6 and USB composition 8",
    [QMI_DMS_SWI_USB_COMPOSITION_14] = "Dual configuration: USB composition 6 and USB composition 9",
    [QMI_DMS_SWI_USB_COMPOSITION_15] = "Dual configuration: USB composition 6 and USB composition 10",
    [QMI_DMS_SWI_USB_COMPOSITION_16] = "Dual configuration: USB composition 6 and USB composition 11",
    [QMI_DMS_SWI_USB_COMPOSITION_17] = "Dual configuration: USB composition 6 and USB composition 12",
    [QMI_DMS_SWI_USB_COMPOSITION_18] = "Dual configuration: USB composition 7 and USB composition 8",
    [QMI_DMS_SWI_USB_COMPOSITION_19] = "Dual configuration: USB composition 7 and USB composition 9",
    [QMI_DMS_SWI_USB_COMPOSITION_20] = "Dual configuration: USB composition 7 and USB composition 10",
    [QMI_DMS_SWI_USB_COMPOSITION_21] = "Dual configuration: USB composition 7 and USB composition 11",
    [QMI_DMS_SWI_USB_COMPOSITION_22] = "Dual configuration: USB composition 7 and USB composition 12",
};

const gchar *
qmi_dms_swi_usb_composition_get_description (QmiDmsSwiUsbComposition value)
{
    return (((value > QMI_DMS_SWI_USB_COMPOSITION_UNKNOWN) &&
             ((guint)value < G_N_ELEMENTS (usb_composition_description))) ?
            usb_composition_description[value] : NULL);
}
