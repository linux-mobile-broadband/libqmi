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
 */

#ifndef _LIBQMI_GLIB_QMI_ENUMS_PBM_H_
#define _LIBQMI_GLIB_QMI_ENUMS_PBM_H_

#if !defined (__LIBQMI_GLIB_H_INSIDE__) && !defined (LIBQMI_GLIB_COMPILATION)
#error "Only <libqmi-glib.h> can be included directly."
#endif

/**
 * SECTION: qmi-enums-pbm
 * @title: PBM enumerations and flags
 *
 * This section defines enumerations and flags used in the PBM service
 * interface.
 */

/*****************************************************************************/
/* Helper enums for the 'QMI PBM Indication Register' indication */

/**
 * QmiPbmEventRegistrationFlag:
 * @QMI_PBM_EVENT_REGISTRATION_FLAG_RECORD_UPDATE: Request indications when records are added/edited/deleted.
 * @QMI_PBM_EVENT_REGISTRATION_FLAG_PHONEBOOK_READY: Request indications when phonebooks are ready.
 * @QMI_PBM_EVENT_REGISTRATION_FLAG_EMERGENCY_NUMBER_LIST: Request indications when emergency numbers are changed.
 * @QMI_PBM_EVENT_REGISTRATION_FLAG_HIDDEN_RECORD_STATUS: Request indications when hidden record status is changed.
 * @QMI_PBM_EVENT_REGISTRATION_FLAG_AAS_UPDATE: Request indications when Additional number Alpha String records are added/edited/deleted.
 * @QMI_PBM_EVENT_REGISTRATION_FLAG_GAS_UPDATE: Request indications when Grouping information Alpha String records are added/edited/deleted.
 *
 * Flags to use to register to phonebook indications.
 */
typedef enum {
    QMI_PBM_EVENT_REGISTRATION_FLAG_RECORD_UPDATE         = 1 << 0,
    QMI_PBM_EVENT_REGISTRATION_FLAG_PHONEBOOK_READY       = 1 << 1,
    QMI_PBM_EVENT_REGISTRATION_FLAG_EMERGENCY_NUMBER_LIST = 1 << 2,
    QMI_PBM_EVENT_REGISTRATION_FLAG_HIDDEN_RECORD_STATUS  = 1 << 3,
    QMI_PBM_EVENT_REGISTRATION_FLAG_AAS_UPDATE            = 1 << 4,
    QMI_PBM_EVENT_REGISTRATION_FLAG_GAS_UPDATE            = 1 << 5,
} QmiPbmEventRegistrationFlag;

#endif /* _LIBQMI_GLIB_QMI_ENUMS_PBM_H_ */
