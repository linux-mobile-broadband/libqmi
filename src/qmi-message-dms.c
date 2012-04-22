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

#include "qmi-message-dms.h"
#include "qmi-enums.h"

/*****************************************************************************/
/* Get IDs */

enum {
  QMI_DMS_TLV_GET_IDS_ESN = 0x10,
  QMI_DMS_TLV_GET_IDS_IMEI,
  QMI_DMS_TLV_GET_IDS_MEID
};

QmiMessage *
qmi_message_dms_get_ids_new (guint8 transaction_id,
                             guint8 client_id)
{
    return qmi_message_new (QMI_SERVICE_DMS,
                            client_id,
                            transaction_id,
                            QMI_DMS_MESSAGE_GET_IDS);
}

gboolean
qmi_message_dms_get_ids_reply_parse (QmiMessage *self,
                                     gchar **esn,
                                     gchar **imei,
                                     gchar **meid,
                                     GError **error)
{
    gchar *str;

    g_assert (qmi_message_get_message_id (self) == QMI_DMS_MESSAGE_GET_IDS);
    g_assert (esn != NULL);
    g_assert (imei != NULL);
    g_assert (meid != NULL);

    *esn = qmi_message_tlv_get_string (self,
                                       QMI_DMS_TLV_GET_IDS_ESN,
                                       error);
    if (!*esn)
        return FALSE;

    *imei = qmi_message_tlv_get_string (self,
                                        QMI_DMS_TLV_GET_IDS_IMEI,
                                        error);
    if (!*imei)
        return FALSE;

    *meid = qmi_message_tlv_get_string (self,
                                        QMI_DMS_TLV_GET_IDS_MEID,
                                        error);
    if (!*meid)
        return FALSE;

    return TRUE;
}
