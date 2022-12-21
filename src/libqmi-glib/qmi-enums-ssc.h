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

#ifndef _LIBQMI_GLIB_QMI_ENUMS_SSC_H_
#define _LIBQMI_GLIB_QMI_ENUMS_SSC_H_

#if !defined (__LIBQMI_GLIB_H_INSIDE__) && !defined (LIBQMI_GLIB_COMPILATION)
#error "Only <libqmi-glib.h> can be included directly."
#endif

/**
 * SECTION: qmi-enums-ssc
 * @title: SSC enumerations and flags
 * @short_description: Enumerations and flags in the SSC service.
 *
 * This section defines enumerations and flags used in the SSC service
 * interface.
 */

/**
 * QmiSscReportType:
 * @QMI_SSC_REPORT_TYPE_SMALL: Small size report.
 * @QMI_SSC_REPORT_TYPE_LARGE: Large size report.
 *
 * SSC service report types.
 *
 * Since: 1.34
 */
typedef enum { /*< since=1.34 >*/
    QMI_SSC_REPORT_TYPE_SMALL = 0x00,
    QMI_SSC_REPORT_TYPE_LARGE = 0x01,
} QmiSscReportType;

#endif /* _LIBQMI_GLIB_QMI_ENUMS_SSC_H_ */
