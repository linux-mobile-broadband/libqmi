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

#ifndef _LIBQMI_GLIB_QMI_ENUMS_PDS_H_
#define _LIBQMI_GLIB_QMI_ENUMS_PDS_H_

/*****************************************************************************/
/* Helper enums for the 'QMI PDS Event Report' indication */

/**
 * QmiPdsOperationMode:
 * @QMI_PDS_OPERATION_MODE_UNKNOWN: Unknown (position not fixed yet).
 * @QMI_PDS_OPERATION_MODE_STANDALONE: Standalone.
 * @QMI_PDS_OPERATION_MODE_MS_BASED: MS based.
 * @QMI_PDS_OPERATION_MODE_MS_ASSISTED: MS assisted.
 *
 * Operation mode used to compute the position.
 */
typedef enum {
    QMI_PDS_OPERATION_MODE_UNKNOWN     = -1,
    QMI_PDS_OPERATION_MODE_STANDALONE  =  0,
    QMI_PDS_OPERATION_MODE_MS_BASED    =  1,
    QMI_PDS_OPERATION_MODE_MS_ASSISTED =  2
} QmiPdsOperationMode;

/**
 * QmiPdsPositionSessionStatus:
 * @QMI_PDS_POSITION_SESSION_STATUS_SUCCESS: Success.
 * @QMI_PDS_POSITION_SESSION_STATUS_IN_PROGRESS: In progress.
 * @QMI_PDS_POSITION_SESSION_STATUS_GENERAL_FAILURE: General failure.
 * @QMI_PDS_POSITION_SESSION_STATUS_TIMEOUT: Timeout.
 * @QMI_PDS_POSITION_SESSION_STATUS_USER_ENDED_SESSION: User ended session.
 * @QMI_PDS_POSITION_SESSION_STATUS_BAD_PARAMETER: Bad parameter.
 * @QMI_PDS_POSITION_SESSION_STATUS_PHONE_OFFLINE: Phone is offline.
 * @QMI_PDS_POSITION_SESSION_STATUS_ENGINE_LOCKED: Engine locked.
 * @QMI_PDS_POSITION_SESSION_STATUS_E911_SESSION_IN_PROGRESS: Emergency call in progress.
 *
 * Status of the positioning session.
 */
typedef enum {
    QMI_PDS_POSITION_SESSION_STATUS_SUCCESS                  = 0x00,
    QMI_PDS_POSITION_SESSION_STATUS_IN_PROGRESS              = 0x01,
    QMI_PDS_POSITION_SESSION_STATUS_GENERAL_FAILURE          = 0x02,
    QMI_PDS_POSITION_SESSION_STATUS_TIMEOUT                  = 0x03,
    QMI_PDS_POSITION_SESSION_STATUS_USER_ENDED_SESSION       = 0x04,
    QMI_PDS_POSITION_SESSION_STATUS_BAD_PARAMETER            = 0x05,
    QMI_PDS_POSITION_SESSION_STATUS_PHONE_OFFLINE            = 0x06,
    QMI_PDS_POSITION_SESSION_STATUS_ENGINE_LOCKED            = 0x07,
    QMI_PDS_POSITION_SESSION_STATUS_E911_SESSION_IN_PROGRESS = 0x08
} QmiPdsPositionSessionStatus;

#endif /* _LIBQMI_GLIB_QMI_ENUMS_PDS_H_ */
