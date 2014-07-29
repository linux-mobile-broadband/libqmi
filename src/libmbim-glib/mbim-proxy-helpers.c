/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 * libmbim-glib -- GLib/GIO based library to control MBIM devices
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
 * Copyright (C) 2014 Aleksander Morgado <aleksander@aleksander.es>
 * Copyright (C) 2014 Smith Micro Software, Inc.
 */

#include <string.h>
#include "mbim-proxy-helpers.h"
#include "mbim-message-private.h"
#include "mbim-message.h"

MbimEventEntry **
_mbim_proxy_helper_service_subscribe_standard_list_new (void)
{
    guint32  i, service;
    MbimEventEntry **out;

    out = g_new0 (MbimEventEntry *, 1 + (MBIM_SERVICE_DSS - MBIM_SERVICE_BASIC_CONNECT + 1));

    for (service = MBIM_SERVICE_BASIC_CONNECT, i = 0;
         service <= MBIM_SERVICE_DSS;
         service++, i++) {
         out[i] = g_new0 (MbimEventEntry, 1);
         memcpy (&out[i]->device_service_id, mbim_uuid_from_service (service), sizeof (MbimUuid));
    }

    return out;
}

MbimEventEntry **
_mbim_proxy_helper_service_subscribe_request_parse (MbimMessage *message)
{
    MbimEventEntry **array = NULL;
    guint32 i;
    guint32 element_count;
    guint32 offset = 0;
    guint32 array_offset;
    MbimEventEntry *event;

    element_count = _mbim_message_read_guint32 (message, offset);
    if (element_count) {
        array = g_new (MbimEventEntry *, element_count + 1);

        offset += 4;
        for (i = 0; i < element_count; i++) {
            array_offset = _mbim_message_read_guint32 (message, offset);

            event = g_new (MbimEventEntry, 1);

            memcpy (&(event->device_service_id), _mbim_message_read_uuid (message, array_offset), 16);
            array_offset += 16;

            event->cids_count = _mbim_message_read_guint32 (message, array_offset);
            array_offset += 4;

            if (event->cids_count)
                event->cids = _mbim_message_read_guint32_array (message, event->cids_count, array_offset);
            else
                event->cids = NULL;

            array[i] = event;
            offset += 8;
        }

        array[element_count] = NULL;
    }

    return array;
}

MbimEventEntry **
_mbim_proxy_helper_service_subscribe_list_merge (MbimEventEntry **original,
                                                 MbimEventEntry **merge,
                                                 guint           *events_count)
{

    guint32 i, ii;
    guint32 out_idx, out_cid_idx;
    MbimEventEntry *entry;

    for (i = 0; merge[i]; i++) {
        entry = NULL;

        /* look for matching uuid */
        for (out_idx = 0; original[out_idx]; out_idx++) {
            if (mbim_uuid_cmp (&merge[i]->device_service_id, &original[out_idx]->device_service_id)) {
                entry = original[out_idx];
                break;
            }
        }

        if (!entry) {
            /* matching uuid not found in merge array, add it */
            original = g_realloc (original, sizeof (*original) * (out_idx + 2));
            original[out_idx] = g_memdup (merge[i], sizeof (MbimEventEntry));
            if (merge[i]->cids_count)
                original[out_idx]->cids = g_memdup (merge[i]->cids,
                                                    sizeof (guint32) * merge[i]->cids_count);
            else
                original[out_idx]->cids = NULL;

            original[++out_idx] = NULL;
            *events_count = out_idx;
        } else {
            /* matching uuid found, add cids */
            if (!entry->cids_count)
                /* all cids already enabled for uuid */
                continue;

            /* If we're adding all enabled cids, directly apply that */
            if (merge[i]->cids_count == 0) {
                g_free (entry->cids);
                entry->cids = NULL;
                entry->cids_count = 0;
            }

            for (ii = 0; ii < merge[i]->cids_count; ii++) {
                for (out_cid_idx = 0; out_cid_idx < entry->cids_count; out_cid_idx++) {
                    if (merge[i]->cids[ii] == entry->cids[out_cid_idx]) {
                        break;
                    }
                }

                if (out_cid_idx == entry->cids_count) {
                    /* cid not found in merge array, add it */
                    entry->cids = g_realloc (entry->cids, sizeof (guint32) * (++entry->cids_count));
                    entry->cids[out_cid_idx] = merge[i]->cids[ii];
                }
            }
        }
    }

    return original;
}
