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
 * Copyright (C) 2012-2017 Aleksander Morgado <aleksander@aleksander.es>
 */

#ifndef _LIBQMI_GLIB_QMI_ENUMS_PDC_H_
#define _LIBQMI_GLIB_QMI_ENUMS_PDC_H_

#if !defined (__LIBQMI_GLIB_H_INSIDE__) && !defined (LIBQMI_GLIB_COMPILATION)
#error "Only <libqmi-glib.h> can be included directly."
#endif

/**
 * SECTION: qmi-enums-pdc
 * @title: PDC enumerations and flags
 *
 * This section defines enumerations and flags used in the PDC service
 * interface.
 */

/*****************************************************************************/
/* Helper enums for the 'QMI PDC' calls */

/**
 * QmiPdcConfigurationType:
 * @QMI_PDC_CONFIGURATION_TYPE_PLATFORM: Platform
 * @QMI_PDC_CONFIGURATION_TYPE_SOFTWARE: Software
 *
 * Configuration type for change/load configuration.
 *
 * Since: 1.18
 */
typedef enum {
   QMI_PDC_CONFIGURATION_TYPE_PLATFORM = 0,
   QMI_PDC_CONFIGURATION_TYPE_SOFTWARE = 1,
} QmiPdcConfigurationType;

/**
 * qmi_pdc_configuration_type_get_string:
 *
 * Since: 1.18
 */

#endif /* _LIBQMI_GLIB_QMI_ENUMS_PDC_H_ */
