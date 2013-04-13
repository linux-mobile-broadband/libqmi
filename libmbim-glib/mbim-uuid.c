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
 * Copyright (C) 2013 Aleksander Morgado <aleksander@gnu.org>
 */

#include <config.h>
#include <stdio.h>
#include <string.h>

#include "mbim-uuid.h"

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

/**
 * mbim_uuid_from_service:
 * @service: a #MbimService.
 *
 * Get the UUID corresponding to @service.
 *
 * Returns: (transfer none): a #MbimUuid.
 */
const MbimUuid *
mbim_uuid_from_service (MbimService service)
{
    g_return_val_if_fail (service >= MBIM_SERVICE_INVALID && service <= MBIM_SERVICE_DSS,
                          &uuid_invalid);

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
    default:
        g_assert_not_reached ();
    }
}

/**
 * mbim_uuid_to_service:
 * @uuid: a #MbimUuid.
 *
 * Get the service corresponding to @uuid.
 *
 * Returns: a #MbimService.
 */
MbimService
mbim_uuid_to_service (const MbimUuid *uuid)
{
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

    return MBIM_SERVICE_INVALID;
}

/**
 * mbim_uuid_cmp:
 * @a: a #MbimUuid.
 * @b: a #MbimUuid.
 *
 * Compare two %MbimUuid values.
 *
 * Returns: %TRUE if @a and @b are equal, %FALSE otherwise.
 */
gboolean
mbim_uuid_cmp (const MbimUuid *a,
               const MbimUuid *b)
{
    return (memcmp (a, b, sizeof (*a)) == 0);
}

/**
 * mbim_uuid_get_printable:
 * @uuid: a #MbimUuid.
 *
 * Get a string with the UUID.
 *
 * Returns: (transfer full): a newly allocated string, which should be freed with g_free().
 */
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
