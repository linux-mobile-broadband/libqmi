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
 * Copyright (C) 2012 Aleksander Morgado <aleksander@lanedo.com>
 */

/* NOTE: this is a private non-installable header */

#ifndef _LIBQMI_GLIB_QMI_MESSAGE_CTL_H_
#define _LIBQMI_GLIB_QMI_MESSAGE_CTL_H_

#include <glib.h>
#include "qmi-message.h"

G_BEGIN_DECLS

/* Version info */

typedef struct {
    QmiService service_type;
    guint16 major_version;
    guint16 minor_version;
} QmiCtlVersionInfo;

QmiMessage *qmi_message_ctl_version_info_new         (guint8 transaction_id);
/* array of QmiCtlVersionInfo */
GArray     *qmi_message_ctl_version_info_reply_parse (QmiMessage *self,
                                                      GError **error);

G_END_DECLS

#endif /* _LIBQMI_GLIB_QMI_MESSAGE_CTL_H_ */
