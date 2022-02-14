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
 * Copyright (C) 2020 Google Inc.
 */

#ifndef _LIBQMI_GLIB_QMI_ENUMS_SAR_H_
#define _LIBQMI_GLIB_QMI_ENUMS_SAR_H_

#if !defined (__LIBQMI_GLIB_H_INSIDE__) && !defined (LIBQMI_GLIB_COMPILATION)
#error "Only <libqmi-glib.h> can be included directly."
#endif

/**
 * SECTION: qmi-enums-sar
 * @title: SAR enumerations and flags
 * @short_description: Enumerations and flags in the SAR service.
 *
 * This section defines enumerations and flags used in the SAR service
 * interface.
 */

/**
 * QmiSarRfState:
 * @QMI_SAR_RF_STATE_0  : RF State 0
 * @QMI_SAR_RF_STATE_1  : RF State 1
 * @QMI_SAR_RF_STATE_2  : RF State 2
 * @QMI_SAR_RF_STATE_3  : RF State 3
 * @QMI_SAR_RF_STATE_4  : RF State 4
 * @QMI_SAR_RF_STATE_5  : RF State 5
 * @QMI_SAR_RF_STATE_6  : RF State 6
 * @QMI_SAR_RF_STATE_7  : RF State 7
 * @QMI_SAR_RF_STATE_8  : RF State 8
 * @QMI_SAR_RF_STATE_9  : RF State 9
 * @QMI_SAR_RF_STATE_10 : RF State 10
 * @QMI_SAR_RF_STATE_11 : RF State 11
 * @QMI_SAR_RF_STATE_12 : RF State 12
 * @QMI_SAR_RF_STATE_13 : RF State 13
 * @QMI_SAR_RF_STATE_14 : RF State 14
 * @QMI_SAR_RF_STATE_15 : RF State 15
 * @QMI_SAR_RF_STATE_16 : RF State 16
 * @QMI_SAR_RF_STATE_17 : RF State 17
 * @QMI_SAR_RF_STATE_18 : RF State 18
 * @QMI_SAR_RF_STATE_19 : RF State 19
 * @QMI_SAR_RF_STATE_20 : RF State 20
 *
 * SAR RF state. Each RF state corresponds to a TX power, and the mapping between TX power and RF state is dictated by NV items.
 *
 * Since: 1.28
 */

typedef enum { /*< since=1.28 >*/
    QMI_SAR_RF_STATE_0   = 0,
    QMI_SAR_RF_STATE_1   = 1,
    QMI_SAR_RF_STATE_2   = 2,
    QMI_SAR_RF_STATE_3   = 3,
    QMI_SAR_RF_STATE_4   = 4,
    QMI_SAR_RF_STATE_5   = 5,
    QMI_SAR_RF_STATE_6   = 6,
    QMI_SAR_RF_STATE_7   = 7,
    QMI_SAR_RF_STATE_8   = 8,
    QMI_SAR_RF_STATE_9   = 9,
    QMI_SAR_RF_STATE_10  = 10,
    QMI_SAR_RF_STATE_11  = 11,
    QMI_SAR_RF_STATE_12  = 12,
    QMI_SAR_RF_STATE_13  = 13,
    QMI_SAR_RF_STATE_14  = 14,
    QMI_SAR_RF_STATE_15  = 15,
    QMI_SAR_RF_STATE_16  = 16,
    QMI_SAR_RF_STATE_17  = 17,
    QMI_SAR_RF_STATE_18  = 18,
    QMI_SAR_RF_STATE_19  = 19,
    QMI_SAR_RF_STATE_20  = 20
} QmiSarRfState;

#endif /* _LIBQMI_GLIB_QMI_ENUMS_SAR_H_ */
