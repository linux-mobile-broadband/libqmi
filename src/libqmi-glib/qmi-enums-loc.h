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

#ifndef _LIBQMI_GLIB_QMI_ENUMS_LOC_H_
#define _LIBQMI_GLIB_QMI_ENUMS_LOC_H_

#if !defined (__LIBQMI_GLIB_H_INSIDE__) && !defined (LIBQMI_GLIB_COMPILATION)
#error "Only <libqmi-glib.h> can be included directly."
#endif

/**
 * SECTION: qmi-enums-loc
 * @title: LOC enumerations and flags
 *
 * This section defines enumerations and flags used in the LOC service
 * interface.
 */

/*****************************************************************************/
/* Helper enums for the 'QMI LOC Start' indication */

/**
 * QmiLocIntermediateReportState:
 * @QMI_LOC_INTERMEDIATE_REPORT_STATE_UNKNOWN: Unknown.
 * @QMI_LOC_INTERMEDIATE_REPORT_STATE_ENABLE: Enable intermediate state reporting.
 * @QMI_LOC_INTERMEDIATE_REPORT_STATE_DISABLE: Disable intermediate state reporting.
 *
 * Whether to enable or disable intermediate state reporting.
 *
 * Since: 1.20
 */
typedef enum {
    QMI_LOC_INTERMEDIATE_REPORT_STATE_UNKNOWN = 0,
    QMI_LOC_INTERMEDIATE_REPORT_STATE_ENABLE  = 1,
    QMI_LOC_INTERMEDIATE_REPORT_STATE_DISABLE = 2,
} QmiLocIntermediateReportState;

/**
 * qmi_loc_intermediate_report_state_get_string:
 *
 * Since: 1.20
 */

#endif /* _LIBQMI_GLIB_QMI_ENUMS_LOC_H_ */
