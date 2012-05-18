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
 * Copyright (C) 2012 Aleksander Morgado <aleksander@gnu.org>
 */

#ifndef _LIBQMI_GLIB_QMI_CTL_H_
#define _LIBQMI_GLIB_QMI_CTL_H_

#include <glib.h>

#include "qmi-enums.h"

G_BEGIN_DECLS

/*****************************************************************************/
/* Supported/known messages */
typedef enum {
    QMI_CTL_MESSAGE_SET_INSTANCE_ID        = 0x0020, /* unused currently */
    QMI_CTL_MESSAGE_GET_VERSION_INFO       = 0x0021,
    QMI_CTL_MESSAGE_ALLOCATE_CLIENT_ID     = 0x0022,
    QMI_CTL_MESSAGE_RELEASE_CLIENT_ID      = 0x0023,
    QMI_CTL_MESSAGE_REVOKE_CLIENT_ID       = 0x0024, /* unused currently */
    QMI_CTL_MESSAGE_INVALID_CLIENT_ID      = 0x0025, /* unused currently */
    QMI_CTL_MESSAGE_SET_DATA_FORMAT        = 0x0026, /* unused currently */
    QMI_CTL_MESSAGE_SYNC                   = 0x0027,
    QMI_CTL_MESSAGE_EVENT                  = 0x0028, /* unused currently */
    QMI_CTL_MESSAGE_SET_POWER_SAVE_CONFIG  = 0x0029, /* unused currently */
    QMI_CTL_MESSAGE_SET_POWER_SAVE_MODE    = 0x002A, /* unused currently */
    QMI_CTL_MESSAGE_GET_POWER_SAVE_MODE    = 0x002B  /* unused currently */
} QmiCtlMessage;

/*****************************************************************************/
/* Version info */
typedef struct _QmiCtlVersionInfo QmiCtlVersionInfo;
QmiService qmi_ctl_version_info_get_service       (QmiCtlVersionInfo *info);
guint16    qmi_ctl_version_info_get_major_version (QmiCtlVersionInfo *info);
guint16    qmi_ctl_version_info_get_minor_version (QmiCtlVersionInfo *info);

G_END_DECLS

#endif /* _LIBQMI_GLIB_QMI_CTL_H_ */
