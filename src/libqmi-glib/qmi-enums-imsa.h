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

#ifndef _LIBQMI_GLIB_QMI_ENUMS_IMSA_H_
#define _LIBQMI_GLIB_QMI_ENUMS_IMSA_H_

#if !defined (__LIBQMI_GLIB_H_INSIDE__) && !defined (LIBQMI_GLIB_COMPILATION)
#error "Only <libqmi-glib.h> can be included directly."
#endif

/**
 * SECTION: qmi-enums-imsa
 * @title: IMSA enumerations and flags
 * @short_description: Enumerations and flags in the IMSA service.
 *
 * This section defines enumerations and flags used in the IMSA service
 * interface.
 */

/**
 * QmiImsaImsRegistrationStatus:
 * @QMI_IMSA_IMS_NOT_REGISTERED: no registration for IMS.
 * @QMI_IMSA_IMS_REGISTERING: IMS is registering.
 * @QMI_IMSA_IMS_REGISTERED: IMS is fully registered.
 * @QMI_IMSA_IMS_LIMITED_REGISTERED: IMS is limited registered, expect limited services.
 *
 * IMS registration status.
 *
 * Since: 1.34
 */
typedef enum { /*< since=1.34 >*/
    QMI_IMSA_IMS_NOT_REGISTERED     = 0x00,
    QMI_IMSA_IMS_REGISTERING        = 0x01,
    QMI_IMSA_IMS_REGISTERED         = 0x02,
    QMI_IMSA_IMS_LIMITED_REGISTERED = 0x03,
} QmiImsaImsRegistrationStatus;

/**
 * QmiImsaServiceStatus:
 * @QMI_IMSA_SERVICE_UNAVAILABLE: Service unavailable.
 * @QMI_IMSA_SERVICE_LIMITED: Service limited available.
 * @QMI_IMSA_SERVICE_AVAILABLE: Service available.
 *
 * IMS Application Service availibility status.
 *
 * Since: 1.34
 */
typedef enum { /*< since=1.34 >*/
    QMI_IMSA_SERVICE_UNAVAILABLE = 0x00,
    QMI_IMSA_SERVICE_LIMITED     = 0x01,
    QMI_IMSA_SERVICE_AVAILABLE   = 0x02,
} QmiImsaServiceStatus;

/**
 * QmiImsaRegistrationTechnology:
 * @QMI_IMSA_REGISTERED_WLAN: registered on WLAN interface.
 * @QMI_IMSA_REGISTERED_WWAN: registered on WWAN interface.
 * @QMI_IMSA_REGISTERED_INTERWORKING_WLAN: registered on Interworking WLAN interface.
 *
 * Network technology on which service is available.
 *
 * Since: 1.34
 */
typedef enum { /*< since=1.34 >*/
    QMI_IMSA_REGISTERED_WLAN              = 0x00,
    QMI_IMSA_REGISTERED_WWAN              = 0x01,
    QMI_IMSA_REGISTERED_INTERWORKING_WLAN = 0x02,
} QmiImsaRegistrationTechnology;

#endif /* _LIBQMI_GLIB_QMI_ENUMS_IMSA_H_ */

