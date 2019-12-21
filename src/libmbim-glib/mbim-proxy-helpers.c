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
#include "mbim-error-types.h"
#include "mbim-cid.h"
#include "mbim-uuid.h"

/*****************************************************************************/

static gboolean
cmp_event_entry_contents (const MbimEventEntry *in,
                          const MbimEventEntry *out)
{
    guint i, o;

    g_assert (mbim_uuid_cmp (&(in->device_service_id), &(out->device_service_id)));

    /* First, compare number of cids in the array */
    if (in->cids_count != out->cids_count)
        return FALSE;

    if (in->cids_count == 0)
        g_assert (in->cids == NULL);
    if (out->cids_count == 0)
        g_assert (out->cids == NULL);

    for (i = 0; i < in->cids_count; i++) {
        for (o = 0; o < out->cids_count; o++) {
            if (in->cids[i] == out->cids[o])
                break;
        }
        if (o == out->cids_count)
            return FALSE;
    }

    return TRUE;
}

gboolean
_mbim_proxy_helper_service_subscribe_list_cmp (const MbimEventEntry * const *a,
                                               gsize                         a_size,
                                               const MbimEventEntry * const *b,
                                               gsize                         b_size)
{
    gsize i, o;

    /* First, compare number of entries a the array */
    if (a_size != b_size)
        return FALSE;

    /* Now compare each service one by one */
    for (i = 0; i < a_size; i++) {
        /* Look for this same service a the other array */
        for (o = 0; o < b_size; o++) {
            /* When service found, compare contents */
            if (mbim_uuid_cmp (&(a[i]->device_service_id), &(b[o]->device_service_id))) {
                if (!cmp_event_entry_contents (a[i], b[o]))
                    return FALSE;
                break;
            }
        }
        /* Service not found! */
        if (!b[o])
            return FALSE;
    }

    return TRUE;
}

/*****************************************************************************/

void
_mbim_proxy_helper_service_subscribe_list_debug (const MbimEventEntry * const *list,
                                                 gsize                         list_size)
{
    gsize i;

    for (i = 0; i < list_size; i++) {
        const MbimEventEntry *entry = list[i];
        MbimService service;
        gchar *str;

        service = mbim_uuid_to_service (&entry->device_service_id);
        str = mbim_uuid_get_printable (&entry->device_service_id);
        g_debug ("[service %u] %s (%s)",
                 (guint)i, str, mbim_service_lookup_name (service));
        g_free (str);

        if (entry->cids_count == 0)
            g_debug ("[service %u] No CIDs explicitly enabled", (guint)i);
        else {
            guint j;

            g_debug ("[service %u] %u CIDs enabled", (guint)i, entry->cids_count);
            for (j = 0; j < entry->cids_count; j++) {
                const gchar *cid_str;

                cid_str = mbim_cid_get_printable (service, entry->cids[j]);
                g_debug ("[service %u] [cid %u] %u (%s)",
                         (guint)i, j, entry->cids[j], cid_str ? cid_str : "unknown");
            }
        }
    }
}

/*****************************************************************************/

MbimEventEntry **
_mbim_proxy_helper_service_subscribe_request_parse (MbimMessage  *message,
                                                    gsize        *out_size,
                                                    GError      **error)
{
    MbimEventEntry **array = NULL;
    guint32 i;
    guint32 element_count;
    guint32 offset = 0;
    guint32 array_offset;
    GError *inner_error = NULL;

    g_assert (message != NULL);
    g_assert (out_size != NULL);

    if (mbim_message_get_message_type (message) != MBIM_MESSAGE_TYPE_COMMAND) {
        g_set_error (error,
                     MBIM_CORE_ERROR,
                     MBIM_CORE_ERROR_INVALID_MESSAGE,
                     "Message is not a request");
        return FALSE;
    }

    if (!mbim_message_command_get_raw_information_buffer (message, NULL)) {
        g_set_error (error,
                     MBIM_CORE_ERROR,
                     MBIM_CORE_ERROR_INVALID_MESSAGE,
                     "Message does not have information buffer");
        return FALSE;
    }

    if (!_mbim_message_read_guint32 (message, offset, &element_count, error))
        return NULL;

    if (element_count) {
        array = g_new0 (MbimEventEntry *, element_count + 1);

        offset += 4;
        for (i = 0; i < element_count; i++) {
            const MbimUuid *uuid;

            if (!_mbim_message_read_guint32 (message, offset, &array_offset, &inner_error))
                break;
            if (!_mbim_message_read_uuid (message, array_offset, &uuid, &inner_error))
                break;

            array[i] = g_new0 (MbimEventEntry, 1);
            memcpy (&(array[i]->device_service_id), uuid, 16);
            array_offset += 16;

            if (!_mbim_message_read_guint32 (message, array_offset, &(array[i])->cids_count, &inner_error))
                break;
            array_offset += 4;

            if (array[i]->cids_count && !_mbim_message_read_guint32_array (message, array[i]->cids_count, array_offset, &array[i]->cids, &inner_error))
                break;
            offset += 8;
        }
    }

    if (inner_error) {
        mbim_event_entry_array_free (array);
        g_propagate_error (error, inner_error);
        return NULL;
    }

    *out_size = element_count;
    return array;
}

/*****************************************************************************/

MbimEventEntry **
_mbim_proxy_helper_service_subscribe_list_merge (MbimEventEntry **in,
                                                 gsize            in_size,
                                                 MbimEventEntry **merge,
                                                 gsize            merge_size,
                                                 gsize           *out_size)
{
    gsize m;

    g_assert (out_size != NULL);

    *out_size = in_size;

    if (!merge || !merge_size)
        return in;

    for (m = 0; m < merge_size; m++) {
        MbimEventEntry *entry = NULL;
        MbimService id;

        /* ignore all merge additions for standard services */
        id = mbim_uuid_to_service (&merge[m]->device_service_id);
        if (id >= MBIM_SERVICE_BASIC_CONNECT && id <= MBIM_SERVICE_DSS)
            continue;

        /* look for matching uuid */
        if (in && in_size) {
            gsize i;

            for (i = 0; i < in_size; i++) {
                if (mbim_uuid_cmp (&merge[m]->device_service_id, &in[i]->device_service_id)) {
                    entry = in[i];
                    break;
                }
            }
        }

        /* matching uuid not found in merge array, add it */
        if (!entry) {
            gsize o;

            /* Index of the new element to add... */
            o = *out_size;

            /* Increase number of events in the output array */
            (*out_size)++;
            in = g_realloc (in, sizeof (MbimEventEntry *) * (*out_size + 1));
            in[o] = g_memdup (merge[m], sizeof (MbimEventEntry));
            if (merge[m]->cids_count)
                in[o]->cids = g_memdup (merge[m]->cids, sizeof (guint32) * merge[m]->cids_count);
            else
                in[o]->cids = NULL;
            in[*out_size] = NULL;
        } else {
            gsize cm, co;

            /* matching uuid found, add cids */
            if (!entry->cids_count)
                /* all cids already enabled for uuid */
                continue;

            /* If we're adding all enabled cids, directly apply that */
            if (merge[m]->cids_count == 0) {
                g_free (entry->cids);
                entry->cids = NULL;
                entry->cids_count = 0;
            }

            for (cm = 0; cm < merge[m]->cids_count; cm++) {
                for (co = 0; co < entry->cids_count; co++) {
                    if (merge[m]->cids[cm] == entry->cids[co]) {
                        break;
                    }
                }

                if (co == entry->cids_count) {
                    /* cid not found in merge array, add it */
                    entry->cids = g_realloc (entry->cids, sizeof (guint32) * (++entry->cids_count));
                    entry->cids[co] = merge[m]->cids[cm];
                }
            }
        }
    }

    return in;
}

/*****************************************************************************/

MbimEventEntry **
_mbim_proxy_helper_service_subscribe_list_dup (MbimEventEntry **in,
                                               gsize            in_size,
                                               gsize           *out_size)
{
    MbimEventEntry **out;
    guint            i;

    g_assert (out_size != NULL);

    out = g_new0 (MbimEventEntry *, in_size + 1);
    for (i = 0; i < in_size; i++) {
        MbimEventEntry *entry_in;
        MbimEventEntry *entry_out;

        entry_in  = in[i];
        entry_out = g_new (MbimEventEntry, 1);
        memcpy (&entry_out->device_service_id, &entry_in->device_service_id, sizeof (MbimUuid));
        entry_out->cids_count = entry_in->cids_count;
        entry_out->cids = g_new (guint32, entry_out->cids_count);
        memcpy (entry_out->cids, entry_in->cids, sizeof (guint32) * entry_out->cids_count);
        out[i] = entry_out;
    }

    *out_size = in_size;
    return out;
}

/*****************************************************************************/

MbimEventEntry **
_mbim_proxy_helper_service_subscribe_list_new_standard (gsize *out_size)
{
    MbimEventEntry **out;
    guint            i = 0;
    MbimEventEntry  *entry;

    g_assert (out_size != NULL);

#define STANDARD_SERVICES_LIST_SIZE 5
    out = g_new0 (MbimEventEntry *, STANDARD_SERVICES_LIST_SIZE + 1);

    /* Basic connect service */
    {
        static const guint32 notify_cids[] = {
            MBIM_CID_BASIC_CONNECT_SUBSCRIBER_READY_STATUS,
            MBIM_CID_BASIC_CONNECT_RADIO_STATE,
            MBIM_CID_BASIC_CONNECT_PREFERRED_PROVIDERS,
            MBIM_CID_BASIC_CONNECT_REGISTER_STATE,
            MBIM_CID_BASIC_CONNECT_PACKET_SERVICE,
            MBIM_CID_BASIC_CONNECT_SIGNAL_STATE,
            MBIM_CID_BASIC_CONNECT_CONNECT,
            MBIM_CID_BASIC_CONNECT_PROVISIONED_CONTEXTS,
            MBIM_CID_BASIC_CONNECT_IP_CONFIGURATION,
            MBIM_CID_BASIC_CONNECT_EMERGENCY_MODE,
            MBIM_CID_BASIC_CONNECT_MULTICARRIER_PROVIDERS,
        };

        entry = g_new (MbimEventEntry, 1);
        memcpy (&entry->device_service_id, mbim_uuid_from_service (MBIM_SERVICE_BASIC_CONNECT), sizeof (MbimUuid));
        entry->cids_count = G_N_ELEMENTS (notify_cids);
        entry->cids = g_memdup (notify_cids, sizeof (guint32) * entry->cids_count);
        out[i++] = entry;
    }

    /* SMS service */
    {
        static const guint32 notify_cids[] = {
            MBIM_CID_SMS_CONFIGURATION,
            MBIM_CID_SMS_READ,
            MBIM_CID_SMS_MESSAGE_STORE_STATUS,
        };

        entry = g_new (MbimEventEntry, 1);
        memcpy (&entry->device_service_id, mbim_uuid_from_service (MBIM_SERVICE_SMS), sizeof (MbimUuid));
        entry->cids_count = G_N_ELEMENTS (notify_cids);
        entry->cids = g_memdup (notify_cids, sizeof (guint32) * entry->cids_count);
        out[i++] = entry;
    }

    /* USSD service */
    {
        static const guint32 notify_cids[] = {
            MBIM_CID_USSD,
        };

        entry = g_new (MbimEventEntry, 1);
        memcpy (&entry->device_service_id, mbim_uuid_from_service (MBIM_SERVICE_USSD), sizeof (MbimUuid));
        entry->cids_count = G_N_ELEMENTS (notify_cids);
        entry->cids = g_memdup (notify_cids, sizeof (guint32) * entry->cids_count);
        out[i++] = entry;
    }

    /* Phonebook service */
    {
        static const guint32 notify_cids[] = {
            MBIM_CID_PHONEBOOK_CONFIGURATION,
        };

        entry = g_new (MbimEventEntry, 1);
        memcpy (&entry->device_service_id, mbim_uuid_from_service (MBIM_SERVICE_PHONEBOOK), sizeof (MbimUuid));
        entry->cids_count = G_N_ELEMENTS (notify_cids);
        entry->cids = g_memdup (notify_cids, sizeof (guint32) * entry->cids_count);
        out[i++] = entry;
    }

    /* STK service */
    {
        static const guint32 notify_cids[] = {
            MBIM_CID_STK_PAC,
        };

        entry = g_new (MbimEventEntry, 1);
        memcpy (&entry->device_service_id, mbim_uuid_from_service (MBIM_SERVICE_STK), sizeof (MbimUuid));
        entry->cids_count = G_N_ELEMENTS (notify_cids);
        entry->cids = g_memdup (notify_cids, sizeof (guint32) * entry->cids_count);
        out[i++] = entry;
    }

    g_assert_cmpuint (i, ==, STANDARD_SERVICES_LIST_SIZE);
    *out_size = i;
    return out;
}
