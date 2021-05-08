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
 * Copyright (C) 2013 - 2014 Aleksander Morgado <aleksander@aleksander.es>
 * Copyright (C) 2014 NVDIA Corporation
 * Copyright (C) 2014 Smith Micro Software, Inc.
 */

#include <config.h>
#include <stdio.h>
#include <string.h>

#include "mbim-uuid.h"
#include "generated/mbim-enum-types.h"

/*****************************************************************************/

gboolean
mbim_uuid_cmp (const MbimUuid *a,
               const MbimUuid *b)
{
    return (memcmp (a, b, sizeof (*a)) == 0);
}

gchar *
mbim_uuid_get_printable (const MbimUuid *uuid)

{
    return (g_strdup_printf (
                "%02x%02x%02x%02x-"
                "%02x%02x-"
                "%02x%02x-"
                "%02x%02x-"
                "%02x%02x%02x%02x%02x%02x",
                uuid->a[0], uuid->a[1], uuid->a[2], uuid->a[3],
                uuid->b[0], uuid->b[1],
                uuid->c[0], uuid->c[1],
                uuid->d[0], uuid->d[1],
                uuid->e[0], uuid->e[1], uuid->e[2], uuid->e[3], uuid->e[4], uuid->e[5]));
}

gboolean
mbim_uuid_from_printable (const gchar *str,
                          MbimUuid    *uuid)
{
    guint8 tmp[16];
    guint i;
    guint k;
    gint d0;
    gint d1;

    g_return_val_if_fail (str != NULL, FALSE);
    g_return_val_if_fail (uuid != NULL, FALSE);

    if (strlen (str) != 36)
        return FALSE;

    for (i = 0, k = 0, d0 = -1, d1 = -1; str[i]; i++) {
        /* Accept dashes in expected positions */
        if (str[i] == '-') {
            if (i == 8 || i == 13 || i == 18 || i == 23)
                continue;
            return FALSE;
        }
        /* Read first digit in the hex pair */
        else if (d0 == -1) {
            d0 = g_ascii_xdigit_value (str[i]);
            if (d0 == -1)
                return FALSE;
        }
        /* Read second digit in the hex pair */
        else {
            d1 = g_ascii_xdigit_value (str[i]);
            if (d1 == -1)
                return FALSE;
            tmp[k++] = (d0 << 4) | d1;
            d0 = d1 = -1;
        }
    }

    memcpy (uuid, tmp, sizeof (tmp));

    return TRUE;
}

/*****************************************************************************/

static const MbimUuid uuid_invalid = {
    .a = { 0x00, 0x00, 0x00, 0x00 },
    .b = { 0x00, 0x00 },
    .c = { 0x00, 0x00 },
    .d = { 0x00, 0x00 },
    .e = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
};

static const MbimUuid uuid_basic_connect = {
    .a = { 0xa2, 0x89, 0xcc, 0x33 },
    .b = { 0xbc, 0xbb },
    .c = { 0x8b, 0x4f },
    .d = { 0xb6, 0xb0 },
    .e = { 0x13, 0x3e, 0xc2, 0xaa, 0xe6, 0xdf }
};

static const MbimUuid uuid_sms = {
    .a = { 0x53, 0x3f, 0xbe, 0xeb },
    .b = { 0x14, 0xfe },
    .c = { 0x44, 0x67 },
    .d = { 0x9f, 0x90 },
    .e = { 0x33, 0xa2, 0x23, 0xe5, 0x6c, 0x3f }
};

static const MbimUuid uuid_ussd = {
    .a = { 0xe5, 0x50, 0xa0, 0xc8 },
    .b = { 0x5e, 0x82 },
    .c = { 0x47, 0x9e },
    .d = { 0x82, 0xf7 },
    .e = { 0x10, 0xab, 0xf4, 0xc3, 0x35, 0x1f }
};

static const MbimUuid uuid_phonebook = {
    .a = { 0x4b, 0xf3, 0x84, 0x76 },
    .b = { 0x1e, 0x6a },
    .c = { 0x41, 0xdb },
    .d = { 0xb1, 0xd8 },
    .e = { 0xbe, 0xd2, 0x89, 0xc2, 0x5b, 0xdb }
};

static const MbimUuid uuid_stk = {
    .a = { 0xd8, 0xf2, 0x01, 0x31 },
    .b = { 0xfc, 0xb5 },
    .c = { 0x4e, 0x17 },
    .d = { 0x86, 0x02 },
    .e = { 0xd6, 0xed, 0x38, 0x16, 0x16, 0x4c }
};

static const MbimUuid uuid_auth = {
    .a = { 0x1d, 0x2b, 0x5f, 0xf7 },
    .b = { 0x0a, 0xa1 },
    .c = { 0x48, 0xb2 },
    .d = { 0xaa, 0x52 },
    .e = { 0x50, 0xf1, 0x57, 0x67, 0x17, 0x4e }
};

static const MbimUuid uuid_dss = {
    .a = { 0xc0, 0x8a, 0x26, 0xdd },
    .b = { 0x77, 0x18 },
    .c = { 0x43, 0x82 },
    .d = { 0x84, 0x82 },
    .e = { 0x6e, 0x0d, 0x58, 0x3c, 0x4d, 0x0e }
};

static const MbimUuid uuid_ms_firmware_id = {
    .a = { 0xe9, 0xf7, 0xde, 0xa2 },
    .b = { 0xfe, 0xaf },
    .c = { 0x40, 0x09 },
    .d = { 0x93, 0xce },
    .e = { 0x90, 0xa3, 0x69, 0x41, 0x03, 0xb6 }
};

static const MbimUuid uuid_ms_host_shutdown = {
    .a = { 0x88, 0x3b, 0x7c, 0x26 },
    .b = { 0x98, 0x5f },
    .c = { 0x43, 0xfa },
    .d = { 0x98, 0x04 },
    .e = { 0x27, 0xd7, 0xfb, 0x80, 0x95, 0x9c }
};

static const MbimUuid uuid_ms_sar = {
    .a = { 0x68, 0x22, 0x3d, 0x04 },
    .b = { 0x9f, 0x6c },
    .c = { 0x4e, 0x0f },
    .d = { 0x82, 0x2d },
    .e = { 0x28, 0x44, 0x1f, 0xb7, 0x23, 0x40 }
};

static const MbimUuid uuid_proxy_control = {
    .a = { 0x83, 0x8c, 0xf7, 0xfb },
    .b = { 0x8d, 0x0d },
    .c = { 0x4d, 0x7f },
    .d = { 0x87, 0x1e },
    .e = { 0xd7, 0x1d , 0xbe, 0xfb, 0xb3, 0x9b }
};

/* Note: this UUID is likely to work only for Sierra modems */
static const MbimUuid uuid_qmi = {
    .a = { 0xd1, 0xa3, 0x0b, 0xc2 },
    .b = { 0xf9, 0x7a },
    .c = { 0x6e, 0x43 },
    .d = { 0xbf, 0x65 },
    .e = { 0xc7, 0xe2 , 0x4f, 0xb0, 0xf0, 0xd3 }
};

static const MbimUuid uuid_atds = {
    .a = { 0x59, 0x67, 0xbd, 0xcc },
    .b = { 0x7f, 0xd2 },
    .c = { 0x49, 0xa2 },
    .d = { 0x9f, 0x5c },
    .e = { 0xb2, 0xe7, 0x0e, 0x52, 0x7d, 0xb3 }
};

static const MbimUuid uuid_intel_firmware_update = {
    .a = { 0x0e, 0xd3, 0x74, 0xcb },
    .b = { 0xf8, 0x35 },
    .c = { 0x44, 0x74 },
    .d = { 0xbc, 0x11 },
    .e = { 0x3b, 0x3f, 0xd7, 0x6f, 0x56, 0x41 }
};

static const MbimUuid uuid_qdu = {
    .a = { 0x64, 0x27, 0x01, 0x5f },
    .b = { 0x57, 0x9d },
    .c = { 0x48, 0xf5 },
    .d = { 0x8c, 0x54 },
    .e = { 0xf4, 0x3e, 0xd1, 0xe7, 0x6f, 0x83 }
};

static const MbimUuid uuid_ms_basic_connect_extensions = {
    .a = { 0x3d, 0x01, 0xdc, 0xc5 },
    .b = { 0xfe, 0xf5 },
    .c = { 0x4d, 0x05 },
    .d = { 0x0d, 0x3a },
    .e = { 0xbe, 0xf7, 0x05, 0x8e, 0x9a, 0xaf }
};

static const MbimUuid uuid_ms_uicc_low_level_access = {
    .a = { 0xc2, 0xf6, 0x58, 0x8e },
    .b = { 0xf0, 0x37 },
    .c = { 0x4b, 0xc9 },
    .d = { 0x86, 0x65 },
    .e = { 0xf4, 0xd4, 0x4b, 0xd0, 0x93, 0x67 }
};

static GList *mbim_custom_service_list = NULL;

typedef struct {
    guint service_id;
    MbimUuid uuid;
    gchar *nickname;
} MbimCustomService;

guint
mbim_register_custom_service (const MbimUuid *uuid,
                              const gchar *nickname)
{
    MbimCustomService *s;
    GList *l;
    guint service_id = 100;

    for (l = mbim_custom_service_list; l != NULL; l = l->next) {
        s = (MbimCustomService *)l->data;
        if (mbim_uuid_cmp (&s->uuid, uuid))
            return s->service_id;
        else
            service_id = MAX (service_id, s->service_id);
    }

    /* create a new custom service */
    s = g_slice_new (MbimCustomService);
    s->service_id = service_id + 1;
    memcpy (&s->uuid, uuid, sizeof (MbimUuid));
    s->nickname = g_strdup (nickname);

    mbim_custom_service_list = g_list_append (mbim_custom_service_list, s);
    return s->service_id;
}

gboolean
mbim_unregister_custom_service (const guint id)
{
    MbimCustomService *s;
    GList *l;

    for (l = mbim_custom_service_list; l != NULL; l = l->next) {
        s = (MbimCustomService *)l->data;
        if (s->service_id == id) {
            g_free (s->nickname);
            g_slice_free (MbimCustomService, s);
            mbim_custom_service_list = \
                g_list_delete_link (mbim_custom_service_list, l);
            return TRUE;
        }
    }

    return FALSE;
}

gboolean
mbim_service_id_is_custom (const guint id)
{
    GList *l;

    if (id < MBIM_SERVICE_LAST)
        return FALSE;

    for (l = mbim_custom_service_list; l != NULL; l = l->next) {
        if (((MbimCustomService *)l->data)->service_id == id)
            return TRUE;
    }

    return FALSE;
}

const gchar *
mbim_service_lookup_name (guint service)
{
    GList *l;

    if (service < MBIM_SERVICE_LAST)
        return mbim_service_get_string (service);

    for (l = mbim_custom_service_list; l != NULL; l = l->next) {
        if (service == ((MbimCustomService *)l->data)->service_id)
            return ((MbimCustomService *)l->data)->nickname;
    }
    return NULL;
}

const MbimUuid *
mbim_uuid_from_service (MbimService service)
{
    GList *l;

    g_return_val_if_fail (service < MBIM_SERVICE_LAST || mbim_service_id_is_custom (service), &uuid_invalid);

    switch (service) {
    case MBIM_SERVICE_INVALID:
        return &uuid_invalid;
    case MBIM_SERVICE_BASIC_CONNECT:
        return &uuid_basic_connect;
    case MBIM_SERVICE_SMS:
        return &uuid_sms;
    case MBIM_SERVICE_USSD:
        return &uuid_ussd;
    case MBIM_SERVICE_PHONEBOOK:
        return &uuid_phonebook;
    case MBIM_SERVICE_STK:
        return &uuid_stk;
    case MBIM_SERVICE_AUTH:
        return &uuid_auth;
    case MBIM_SERVICE_DSS:
        return &uuid_dss;
    case MBIM_SERVICE_MS_FIRMWARE_ID:
        return &uuid_ms_firmware_id;
    case MBIM_SERVICE_MS_HOST_SHUTDOWN:
        return &uuid_ms_host_shutdown;
    case MBIM_SERVICE_MS_SAR:
        return &uuid_ms_sar;
    case MBIM_SERVICE_PROXY_CONTROL:
        return &uuid_proxy_control;
    case MBIM_SERVICE_QMI:
        return &uuid_qmi;
    case MBIM_SERVICE_ATDS:
        return &uuid_atds;
    case MBIM_SERVICE_QDU:
        return &uuid_qdu;
    case MBIM_SERVICE_INTEL_FIRMWARE_UPDATE:
        return &uuid_intel_firmware_update;
    case MBIM_SERVICE_MS_BASIC_CONNECT_EXTENSIONS:
        return &uuid_ms_basic_connect_extensions;
    case MBIM_SERVICE_MS_UICC_LOW_LEVEL_ACCESS:
        return &uuid_ms_uicc_low_level_access;
    case MBIM_SERVICE_LAST:
        g_assert_not_reached ();
    default:
        for (l = mbim_custom_service_list; l != NULL; l = l->next) {
            if (service == ((MbimCustomService *)l->data)->service_id)
                return &((MbimCustomService *)l->data)->uuid;
        }
        g_return_val_if_reached (NULL);
    }
}

MbimService
mbim_uuid_to_service (const MbimUuid *uuid)
{
    GList *l;

    if (mbim_uuid_cmp (uuid, &uuid_basic_connect))
        return MBIM_SERVICE_BASIC_CONNECT;

    if (mbim_uuid_cmp (uuid, &uuid_sms))
        return MBIM_SERVICE_SMS;

    if (mbim_uuid_cmp (uuid, &uuid_ussd))
        return MBIM_SERVICE_USSD;

    if (mbim_uuid_cmp (uuid, &uuid_phonebook))
        return MBIM_SERVICE_PHONEBOOK;

    if (mbim_uuid_cmp (uuid, &uuid_stk))
        return MBIM_SERVICE_STK;

    if (mbim_uuid_cmp (uuid, &uuid_auth))
        return MBIM_SERVICE_AUTH;

    if (mbim_uuid_cmp (uuid, &uuid_dss))
        return MBIM_SERVICE_DSS;

    if (mbim_uuid_cmp (uuid, &uuid_ms_firmware_id))
        return MBIM_SERVICE_MS_FIRMWARE_ID;

    if (mbim_uuid_cmp (uuid, &uuid_ms_host_shutdown))
        return MBIM_SERVICE_MS_HOST_SHUTDOWN;

    if (mbim_uuid_cmp (uuid, &uuid_ms_sar))
        return MBIM_SERVICE_MS_SAR;

    if (mbim_uuid_cmp (uuid, &uuid_proxy_control))
        return MBIM_SERVICE_PROXY_CONTROL;

    if (mbim_uuid_cmp (uuid, &uuid_qmi))
        return MBIM_SERVICE_QMI;

    if (mbim_uuid_cmp (uuid, &uuid_atds))
        return MBIM_SERVICE_ATDS;

    if (mbim_uuid_cmp (uuid, &uuid_intel_firmware_update))
        return MBIM_SERVICE_INTEL_FIRMWARE_UPDATE;

    if (mbim_uuid_cmp (uuid, &uuid_qdu))
        return MBIM_SERVICE_QDU;

    if (mbim_uuid_cmp (uuid, &uuid_ms_basic_connect_extensions))
        return MBIM_SERVICE_MS_BASIC_CONNECT_EXTENSIONS;

    if (mbim_uuid_cmp (uuid, &uuid_ms_uicc_low_level_access))
        return MBIM_SERVICE_MS_UICC_LOW_LEVEL_ACCESS;

    for (l = mbim_custom_service_list; l != NULL; l = l->next) {
        if (mbim_uuid_cmp (&((MbimCustomService *)l->data)->uuid, uuid))
            return ((MbimCustomService *)l->data)->service_id;
    }

    return MBIM_SERVICE_INVALID;
}

/*****************************************************************************/

static const MbimUuid uuid_context_type_none = {
    .a = { 0xB4, 0x3F, 0x75, 0x8C },
    .b = { 0xA5, 0x60 },
    .c = { 0x4B, 0x46 },
    .d = { 0xB3, 0x5E },
    .e = { 0xC5, 0x86, 0x96, 0x41, 0xFB, 0x54 }
};

static const MbimUuid uuid_context_type_internet = {
    .a = { 0x7E, 0x5E, 0x2A, 0x7E },
    .b = { 0x4E, 0x6F },
    .c = { 0x72, 0x72 },
    .d = { 0x73, 0x6B },
    .e = { 0x65, 0x6E, 0x7E, 0x5E, 0x2A, 0x7E }
};

static const MbimUuid uuid_context_type_vpn = {
    .a = { 0x9B, 0x9F, 0x7B, 0xBE },
    .b = { 0x89, 0x52 },
    .c = { 0x44, 0xB7 },
    .d = { 0x83, 0xAC },
    .e = { 0xCA, 0x41, 0x31, 0x8D, 0xF7, 0xA0 }
};

static const MbimUuid uuid_context_type_voice = {
    .a = { 0x88, 0x91, 0x82, 0x94 },
    .b = { 0x0E, 0xF4 },
    .c = { 0x43, 0x96 },
    .d = { 0x8C, 0xCA },
    .e = { 0xA8, 0x58, 0x8F, 0xBC, 0x02, 0xB2 }
};

static const MbimUuid uuid_context_type_video_share = {
    .a = { 0x05, 0xA2, 0xA7, 0x16 },
    .b = { 0x7C, 0x34 },
    .c = { 0x4B, 0x4D },
    .d = { 0x9A, 0x91 },
    .e = { 0xC5, 0xEF, 0x0C, 0x7A, 0xAA, 0xCC }
};

static const MbimUuid uuid_context_type_purchase = {
    .a = { 0xB3, 0x27, 0x24, 0x96 },
    .b = { 0xAC, 0x6C },
    .c = { 0x42, 0x2B },
    .d = { 0xA8, 0xC0 },
    .e = { 0xAC, 0xF6, 0x87, 0xA2, 0x72, 0x17 }
};

static const MbimUuid uuid_context_type_ims = {
    .a = { 0x21, 0x61, 0x0D, 0x01 },
    .b = { 0x30, 0x74 },
    .c = { 0x4B, 0xCE },
    .d = { 0x94, 0x25 },
    .e = { 0xB5, 0x3A, 0x07, 0xD6, 0x97, 0xD6 }
};

static const MbimUuid uuid_context_type_mms = {
    .a = { 0x46, 0x72, 0x66, 0x64 },
    .b = { 0x72, 0x69 },
    .c = { 0x6B, 0xC6 },
    .d = { 0x96, 0x24 },
    .e = { 0xD1, 0xD3, 0x53, 0x89, 0xAC, 0xA9 }
};

static const MbimUuid uuid_context_type_local = {
    .a = { 0xA5, 0x7A, 0x9A, 0xFC },
    .b = { 0xB0, 0x9F },
    .c = { 0x45, 0xD7 },
    .d = { 0xBB, 0x40 },
    .e = { 0x03, 0x3C, 0x39, 0xF6, 0x0D, 0xB9 }
};

const MbimUuid *
mbim_uuid_from_context_type (MbimContextType context_type)
{
    g_return_val_if_fail (context_type <= MBIM_CONTEXT_TYPE_LOCAL, &uuid_invalid);

    switch (context_type) {
    case MBIM_CONTEXT_TYPE_INVALID:
        return &uuid_invalid;
    case MBIM_CONTEXT_TYPE_NONE:
        return &uuid_context_type_none;
    case MBIM_CONTEXT_TYPE_INTERNET:
        return &uuid_context_type_internet;
    case MBIM_CONTEXT_TYPE_VPN:
        return &uuid_context_type_vpn;
    case MBIM_CONTEXT_TYPE_VOICE:
        return &uuid_context_type_none;
    case MBIM_CONTEXT_TYPE_VIDEO_SHARE:
        return &uuid_context_type_video_share;
    case MBIM_CONTEXT_TYPE_PURCHASE:
        return &uuid_context_type_purchase;
    case MBIM_CONTEXT_TYPE_IMS:
        return &uuid_context_type_ims;
    case MBIM_CONTEXT_TYPE_MMS:
        return &uuid_context_type_mms;
    case MBIM_CONTEXT_TYPE_LOCAL:
        return &uuid_context_type_local;
    default:
        g_assert_not_reached ();
    }
}

MbimContextType
mbim_uuid_to_context_type (const MbimUuid *uuid)
{
    if (mbim_uuid_cmp (uuid, &uuid_context_type_none))
        return MBIM_CONTEXT_TYPE_NONE;

    if (mbim_uuid_cmp (uuid, &uuid_context_type_internet))
        return MBIM_CONTEXT_TYPE_INTERNET;

    if (mbim_uuid_cmp (uuid, &uuid_context_type_vpn))
        return MBIM_CONTEXT_TYPE_VPN;

    if (mbim_uuid_cmp (uuid, &uuid_context_type_voice))
        return MBIM_CONTEXT_TYPE_VOICE;

    if (mbim_uuid_cmp (uuid, &uuid_context_type_video_share))
        return MBIM_CONTEXT_TYPE_VIDEO_SHARE;

    if (mbim_uuid_cmp (uuid, &uuid_context_type_purchase))
        return MBIM_CONTEXT_TYPE_PURCHASE;

    if (mbim_uuid_cmp (uuid, &uuid_context_type_ims))
        return MBIM_CONTEXT_TYPE_IMS;

    if (mbim_uuid_cmp (uuid, &uuid_context_type_mms))
        return MBIM_CONTEXT_TYPE_MMS;

    if (mbim_uuid_cmp (uuid, &uuid_context_type_local))
        return MBIM_CONTEXT_TYPE_LOCAL;

    return MBIM_CONTEXT_TYPE_INVALID;
}
