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
 * Copyright (C) 2012 Lanedo GmbH <aleksander@lanedo.com>
 */

#ifndef _LIBQMI_GLIB_QMI_ENUMS_WDS_H_
#define _LIBQMI_GLIB_QMI_ENUMS_WDS_H_

/*****************************************************************************/
/* Helper enums for the 'QMI WDS Get Packet Service Status' message */

/**
 * QmiWdsConnectionStatus:
 * @QMI_WDS_CONNECTION_STATUS_UNKNOWN: Unknown status.
 * @QMI_WDS_CONNECTION_STATUS_DISCONNECTED: Network is disconnected
 * @QMI_WDS_CONNECTION_STATUS_CONNECTED: Network is connected.
 * @QMI_WDS_CONNECTION_STATUS_SUSPENDED: Network connection is suspended.
 * @QMI_WDS_CONNECTION_STATUS_AUTHENTICATING: Network authentication is ongoing.
 *
 * Status of the network connection.
 */
typedef enum {
    QMI_WDS_CONNECTION_STATUS_UNKNOWN        = 0,
    QMI_WDS_CONNECTION_STATUS_DISCONNECTED   = 1,
    QMI_WDS_CONNECTION_STATUS_CONNECTED      = 2,
    QMI_WDS_CONNECTION_STATUS_SUSPENDED      = 3,
    QMI_WDS_CONNECTION_STATUS_AUTHENTICATING = 4
} QmiWdsConnectionStatus;

#endif /* _LIBQMI_GLIB_QMI_ENUMS_WDS_H_ */
