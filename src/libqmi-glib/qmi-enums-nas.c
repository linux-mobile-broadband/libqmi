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
 * Copyright (C) 2020 Aleksander Morgado <aleksander@aleksander.es>
 */

#include <glib.h>
#include "qmi-enums-nas.h"
#include "qmi-helpers.h"

gchar *
qmi_nas_read_string_from_plmn_encoded_array (QmiNasPlmnEncodingScheme  encoding,
                                             const GArray             *array)
{
    switch (encoding) {
    case QMI_NAS_PLMN_ENCODING_SCHEME_GSM:
      return qmi_helpers_string_utf8_from_gsm7 ((const guint8 *)array->data, array->len);
    case QMI_NAS_PLMN_ENCODING_SCHEME_UCS2LE:
        return qmi_helpers_string_utf8_from_ucs2le ((const guint8 *)array->data, array->len);
    default:
        return NULL;
    }
}

gchar *
qmi_nas_read_string_from_network_description_encoded_array (QmiNasNetworkDescriptionEncoding  encoding,
                                                            const GArray                     *array)
{
    switch (encoding) {
    case QMI_NAS_NETWORK_DESCRIPTION_ENCODING_UNSPECIFIED:
    case QMI_NAS_NETWORK_DESCRIPTION_ENCODING_ASCII7:
        return (g_utf8_validate ((const gchar *)array->data, array->len, NULL) ?
                g_strndup ((const gchar *)array->data, array->len) :
                NULL);
    case QMI_NAS_NETWORK_DESCRIPTION_ENCODING_GSM:
        return qmi_helpers_string_utf8_from_gsm7 ((const guint8 *)array->data, array->len);
    case QMI_NAS_NETWORK_DESCRIPTION_ENCODING_UNICODE:
        return qmi_helpers_string_utf8_from_ucs2le ((const guint8 *)array->data, array->len);
    default:
        return NULL;
    }
}
