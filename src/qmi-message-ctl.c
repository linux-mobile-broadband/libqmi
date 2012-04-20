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
 * Copyright (C) 2011 - 2012 Red Hat, Inc.
 * Copyright (C) 2012 Aleksander Morgado <aleksander@lanedo.com>
 */

#include "qmi-message-ctl.h"
#include "qmi-enums.h"
#include "qmi-error-types.h"

/*****************************************************************************/
/* Version info */

QmiMessage *
qmi_message_ctl_version_info_new (guint8 transaction_id)
{
    return qmi_message_new (QMI_SERVICE_CTL,
                            0,
                            transaction_id,
                            QMI_CTL_MESSAGE_GET_VERSION_INFO);
}

struct qmi_ctl_version_info_list_service {
	guint8 service_type;
	guint16 major_version;
	guint16 minor_version;
} __attribute__((__packed__));

struct qmi_tlv_ctl_version_info_list {
	guint8 count;
	struct qmi_ctl_version_info_list_service services[0];
}__attribute__((__packed__));

GArray *
qmi_message_ctl_version_info_reply_parse (QmiMessage *self,
                                          GError **error)
{
    struct qmi_tlv_ctl_version_info_list *service_list;
    struct qmi_ctl_version_info_list_service *svc;
    guint8 svcbuf[100];
    guint16 svcbuflen = 100;
    GArray *result;
    guint i;

    g_assert (qmi_message_get_message_id (self) == QMI_CTL_MESSAGE_GET_VERSION_INFO);

    if (!qmi_message_tlv_get_varlen (self,
                                     0x01,
                                     &svcbuflen,
                                     svcbuf,
                                     error)) {
        g_prefix_error (error, "Couldn't get services TLV: ");
        return NULL;
    }

    service_list = (struct qmi_tlv_ctl_version_info_list *) svcbuf;
    if (svcbuflen < (service_list->count * sizeof (struct qmi_ctl_version_info_list_service))) {
        g_set_error (error,
                     QMI_CORE_ERROR,
                     QMI_CORE_ERROR_FAILED,
                     "Couldn't read the whole services list (%u < %u)",
                     svcbuflen,
                     (service_list->count * sizeof (struct qmi_ctl_version_info_list_service)));
        return NULL;
    }

    result = g_array_sized_new (FALSE,
                                FALSE,
                                sizeof (QmiCtlVersionInfo),
                                service_list->count);

    for (i = 0, svc = &(service_list->services[0]);
         i < service_list->count;
         i++, svc++) {
        QmiCtlVersionInfo service;

        service.service_type = (QmiService)svc->service_type;
        service.major_version = le16toh (svc->major_version);
        service.minor_version = le16toh (svc->minor_version);

        g_array_insert_val (result, i, service);
    }

    return result;
}

/*****************************************************************************/
/* Allocate CID */

QmiMessage *
qmi_message_ctl_allocate_cid_new (guint8 transaction_id,
                                  QmiService service)
{
    QmiMessage *message;
    guint8 service_id;
    GError *error = NULL;

    g_assert (service != QMI_SERVICE_UNKNOWN);
    service_id = service;

    message = qmi_message_new (QMI_SERVICE_CTL,
                               0,
                               transaction_id,
                               QMI_CTL_MESSAGE_ALLOCATE_CLIENT_ID);

    qmi_message_tlv_add (message,
                         (guint8)0x01,
                         sizeof (service_id),
                         &service_id,
                         &error);
    g_assert_no_error (error);

    return message;
}

struct qmi_ctl_cid {
	guint8 service_type;
    guint8 cid;
} __attribute__((__packed__));;

gboolean
qmi_message_ctl_allocate_cid_reply_parse (QmiMessage *self,
                                          guint8 *cid,
                                          QmiService *service,
                                          GError **error)
{
    struct qmi_ctl_cid id;

    g_assert (qmi_message_get_message_id (self) == QMI_CTL_MESSAGE_ALLOCATE_CLIENT_ID);

    if (!qmi_message_tlv_get (self, 0x01, sizeof (id), &id, error)) {
        g_prefix_error (error, "Couldn't get TLV: ");
        return FALSE;
    }

    if (cid)
        *cid = id.cid;
    if (service)
        *service = (QmiService)id.service_type;

    return TRUE;
}
}
