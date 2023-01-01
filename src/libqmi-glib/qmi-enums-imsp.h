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
 * Copyright (C) 2023 Dylan Van Assche <me@dylanvanassche.be>
 */

#ifndef _LIBQMI_GLIB_QMI_ENUMS_IMSP_H_
#define _LIBQMI_GLIB_QMI_ENUMS_IMSP_H_

#if !defined (__LIBQMI_GLIB_H_INSIDE__) && !defined (LIBQMI_GLIB_COMPILATION)
#error "Only <libqmi-glib.h> can be included directly."
#endif

/**
 * SECTION: qmi-enums-imsp
 * @title: IMSP enumerations and flags
 * @short_description: Enumerations and flags in the IMSP service.
 *
 * This section defines enumerations and flags used in the IMSP service
 * interface.
 */

/**
 * QmiImspEnablerState:
 * @QMI_IMSP_ENABLER_STATE_UNINITIALIZED: IMS is not initialized yet.
 * @QMI_IMSP_ENABLER_STATE_INITIALIZED: IMS is initialized, but not registered yet with the network IMS service.
 * @QMI_IMSP_ENABLER_STATE_AIRPLANE: IMS is initialized but device is in airplane mode.
 * @QMI_IMSP_ENABLER_STATE_REGISTERED: IMS is initialized and registered.
 *
 * IMS Presence enabler state.
 *
 * Since: 1.34
 */
typedef enum { /*< since=1.34 >*/
    QMI_IMSP_ENABLER_STATE_UNINITIALIZED = 0x01,
    QMI_IMSP_ENABLER_STATE_INITIALIZED   = 0x02,
    QMI_IMSP_ENABLER_STATE_AIRPLANE      = 0x03,
    QMI_IMSP_ENABLER_STATE_REGISTERED    = 0x04,
} QmiImspEnablerState;

#endif /* _LIBQMI_GLIB_QMI_ENUMS_IMSP_H_ */

