/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2014 Aleksander Morgado <aleksander@aleksander.es>
 */

#include "mbim-compat.h"

#ifndef MBIM_DISABLE_DEPRECATED

/*****************************************************************************/
/* 'Service Subscriber List' rename to 'Service Subscribe List' */

MbimMessage *
mbim_message_device_service_subscriber_list_set_new (
    guint32 events_count,
    const MbimEventEntry *const *events,
    GError **error)
{
    return (mbim_message_device_service_subscribe_list_set_new (
                events_count,
                events,
                error));
}

gboolean
mbim_message_device_service_subscriber_list_response_parse (
    const MbimMessage *message,
    guint32 *events_count,
    MbimEventEntry ***events,
    GError **error)
{
    return (mbim_message_device_service_subscribe_list_response_parse (
                message,
                events_count,
                events,
                error));
}

/*****************************************************************************/
/* 'LTE Attach Status' rename to 'LTE Attach Info', to avoid the unneeded
 * MbimLteAttachStatus struct */

void
mbim_lte_attach_status_free (MbimDeprecatedLteAttachStatus *var)
{
    if (!var)
        return;

    g_free (var->access_string);
    g_free (var->user_name);
    g_free (var->password);
    g_free (var);
}

MbimMessage *
mbim_message_ms_basic_connect_extensions_lte_attach_status_query_new (GError **error)
{
    return mbim_message_ms_basic_connect_extensions_lte_attach_info_query_new (error);
}

gboolean
mbim_message_ms_basic_connect_extensions_lte_attach_status_response_parse (
    const MbimMessage *message,
    MbimDeprecatedLteAttachStatus **out_lte_attach_status,
    GError **error)
{
    g_autoptr(MbimLteAttachStatus) var = NULL;

    var = g_new0 (MbimDeprecatedLteAttachStatus, 1);
    if (!mbim_message_ms_basic_connect_extensions_lte_attach_info_response_parse (
            message,
            &var->lte_attach_state,
            &var->ip_type,
            &var->access_string,
            &var->user_name,
            &var->password,
            &var->compression,
            &var->auth_protocol,
            error))
        return FALSE;

    *out_lte_attach_status = g_steal_pointer (&var);
    return TRUE;
}

gboolean
mbim_message_ms_basic_connect_extensions_lte_attach_status_notification_parse (
    const MbimMessage *message,
    MbimDeprecatedLteAttachStatus **out_lte_attach_status,
    GError **error)
{
    g_autoptr(MbimLteAttachStatus) var = NULL;

    var = g_new0 (MbimDeprecatedLteAttachStatus, 1);
    if (!mbim_message_ms_basic_connect_extensions_lte_attach_info_notification_parse (
            message,
            &var->lte_attach_state,
            &var->ip_type,
            &var->access_string,
            &var->user_name,
            &var->password,
            &var->compression,
            &var->auth_protocol,
            error))
        return FALSE;

    *out_lte_attach_status = g_steal_pointer (&var);
    return TRUE;
}

#endif /* MBIM_DISABLE_DEPRECATED */
