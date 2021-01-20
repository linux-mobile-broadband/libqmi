/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * libqrtr-glib -- GLib/GIO based library to control QRTR devices
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
 * Copyright (C) 2019-2020 Eric Caruso <ejcaruso@chromium.org>
 * Copyright (C) 2020 Aleksander Morgado <aleksander@aleksander.es>
 */

#include "qrtr-utils.h"

#include <stdlib.h>
#include <string.h>

/* Some kernels expose the qrtr header but not the address family macro. */
#if !defined AF_QIPCRTR
# define AF_QIPCRTR 42
#endif

#define QRTR_URI_SCHEME "qrtr"
#define QRTR_URI_PREFIX QRTR_URI_SCHEME "://"

gchar *
qrtr_get_uri_for_node (guint32 node_id)
{
    return g_strdup_printf (QRTR_URI_PREFIX "%" G_GUINT32_FORMAT, node_id);
}

gboolean
qrtr_get_node_for_uri (const gchar *name,
                       guint32     *node_id)
{
    const gchar *start;
    gchar       *endp = NULL;
    guint        tmp_node_id;

    if (g_ascii_strncasecmp (name, QRTR_URI_PREFIX, strlen (QRTR_URI_PREFIX)) != 0)
        return FALSE;

    start = name + strlen (QRTR_URI_PREFIX);
    tmp_node_id = strtoul (start, &endp, 10);
    if (endp == start)
        return FALSE;

    if (node_id)
        *node_id = tmp_node_id;

    return TRUE;
}
