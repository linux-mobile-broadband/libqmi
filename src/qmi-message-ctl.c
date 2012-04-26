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

#include <endian.h>

#include "qmi-message-ctl.h"
#include "qmi-enums.h"
#include "qmi-error-types.h"

/*****************************************************************************/
/* Version info */

/**
 * QmiCtlVersionInfo:
 *
 * An opaque type specifying a supported service.
 */
struct _QmiCtlVersionInfo {
    volatile gint ref_count;
    QmiService service;
    guint16 major_version;
    guint16 minor_version;
};

/**
 * qmi_ctl_version_info_get_service:
 * @info: a #QmiCtlVersionInfo.
 *
 * Get the QMI service being reported.
 *
 * Returns: A #QmiService.
 */
QmiService
qmi_ctl_version_info_get_service (QmiCtlVersionInfo *info)
{
    g_return_val_if_fail (info != NULL, QMI_SERVICE_UNKNOWN);

    return info->service;
}

/**
 * qmi_ctl_version_info_get_major_version:
 * @info: a #QmiCtlVersionInfo.
 *
 * Get the major version of the QMI service being reported.
 *
 * Returns: the major version.
 */
guint16
qmi_ctl_version_info_get_major_version (QmiCtlVersionInfo *info)
{
    g_return_val_if_fail (info != NULL, 0);

    return info->major_version;
}

/**
 * qmi_ctl_version_info_get_minor_version:
 * @info: a #QmiCtlVersionInfo.
 *
 * Get the minor version of the QMI service being reported.
 *
 * Returns: the minor version.
 */
guint16
qmi_ctl_version_info_get_minor_version (QmiCtlVersionInfo *info)
{
    g_return_val_if_fail (info != NULL, 0);

    return info->minor_version;
}

/**
 * qmi_ctl_version_info_ref:
 * @info: a #QmiCtlVersionInfo.
 *
 * Atomically increments the reference count of @info by one.
 *
 * Returns: the new reference to @info.
 */
QmiCtlVersionInfo *
qmi_ctl_version_info_ref (QmiCtlVersionInfo *info)
{
    g_return_val_if_fail (info != NULL, NULL);

    g_atomic_int_inc (&info->ref_count);
    return info;
}

/**
 * qmi_ctl_version_info_unref:
 * @info: a #QmiCtlVersionInfo.
 *
 * Atomically decrements the reference count of array by one.
 * If the reference count drops to 0, @info is completely disposed.
 */
void
qmi_ctl_version_info_unref (QmiCtlVersionInfo *info)
{
    g_return_if_fail (info != NULL);

    if (g_atomic_int_dec_and_test (&info->ref_count)) {
        g_slice_free (QmiCtlVersionInfo, info);
    }
}

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

GPtrArray *
qmi_message_ctl_version_info_reply_parse (QmiMessage *self,
                                          GError **error)
{
    struct qmi_tlv_ctl_version_info_list *service_list;
    struct qmi_ctl_version_info_list_service *svc;
    guint8 svcbuf[100];
    guint16 svcbuflen = 100;
    GPtrArray *result;
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

    result = g_ptr_array_sized_new (service_list->count);
    g_ptr_array_set_free_func (result, (GDestroyNotify)qmi_ctl_version_info_unref);

    for (i = 0, svc = &(service_list->services[0]);
         i < service_list->count;
         i++, svc++) {
        QmiCtlVersionInfo *info;

        info = g_slice_new (QmiCtlVersionInfo);
        info->service = (QmiService)svc->service_type;
        info->major_version = le16toh (svc->major_version);
        info->minor_version = le16toh (svc->minor_version);

        g_ptr_array_add (result, info);
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

/*****************************************************************************/
/* Release CID */

QmiMessage *
qmi_message_ctl_release_cid_new (guint8 transaction_id,
                                 QmiService service,
                                 guint8 cid)
{
    QmiMessage *message;
    GError *error = NULL;
    struct qmi_ctl_cid id;

    g_assert (service != QMI_SERVICE_UNKNOWN);
    id.service_type = (guint8)service;
    id.cid = cid;

    message = qmi_message_new (QMI_SERVICE_CTL,
                               0,
                               transaction_id,
                               QMI_CTL_MESSAGE_RELEASE_CLIENT_ID);

    qmi_message_tlv_add (message,
                         (guint8)0x01,
                         sizeof (id),
                         &id,
                         &error);
    g_assert_no_error (error);

    return message;
}

gboolean
qmi_message_ctl_release_cid_reply_parse (QmiMessage *self,
                                         guint8 *cid,
                                         QmiService *service,
                                         GError **error)
{
    struct qmi_ctl_cid id;

    g_assert (qmi_message_get_message_id (self) == QMI_CTL_MESSAGE_RELEASE_CLIENT_ID);

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

/*****************************************************************************/
/* Sync */

QmiMessage *
qmi_message_ctl_sync_new (guint8 transaction_id)
{
    return qmi_message_new (QMI_SERVICE_CTL,
                            0,
                            transaction_id,
                            QMI_CTL_MESSAGE_SYNC);
}
