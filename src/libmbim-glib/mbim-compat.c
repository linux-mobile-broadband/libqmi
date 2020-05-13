/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
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

#endif /* MBIM_DISABLE_DEPRECATED */
