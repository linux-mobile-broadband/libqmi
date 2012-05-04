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

#ifndef _LIBQMI_GLIB_QMI_DMS_H_
#define _LIBQMI_GLIB_QMI_DMS_H_

#include <glib.h>

#include "qmi-enums.h"

G_BEGIN_DECLS

/*****************************************************************************/
/* Supported/known messages */
typedef enum {
    QMI_DMS_MESSAGE_EVENT              = 0x0001, /* unused currently */
    QMI_DMS_MESSAGE_GET_MANUFACTURER   = 0x0021, /* unused currently */
    QMI_DMS_MESSAGE_GET_IDS            = 0x0025,
    QMI_DMS_MESSAGE_GET_OPERATING_MODE = 0x002D, /* unused currently */
    QMI_DMS_MESSAGE_SET_OPERATING_MODE = 0x002E, /* unused currently */
} QmiDmsMessage;

/*****************************************************************************/
/* Get IDs */
typedef struct _QmiDmsGetIdsOutput QmiDmsGetIdsOutput;
QmiDmsGetIdsOutput *qmi_dms_get_ids_output_ref        (QmiDmsGetIdsOutput *output);
void                qmi_dms_get_ids_output_unref      (QmiDmsGetIdsOutput *output);
gboolean            qmi_dms_get_ids_output_get_result (QmiDmsGetIdsOutput *output,
                                                       GError **error);
const gchar        *qmi_dms_get_ids_output_get_esn    (QmiDmsGetIdsOutput *output);
const gchar        *qmi_dms_get_ids_output_get_imei   (QmiDmsGetIdsOutput *output);
const gchar        *qmi_dms_get_ids_output_get_meid   (QmiDmsGetIdsOutput *output);

G_END_DECLS

#endif /* _LIBQMI_GLIB_QMI_DMS_H_ */
