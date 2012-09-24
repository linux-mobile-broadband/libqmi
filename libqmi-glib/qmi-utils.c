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
 * Copyright (C) 2012 Aleksander Morgado <aleksander@lanedo.com>
 */

#include <string.h>
#include <stdint.h>
#include <stdio.h>

#include "qmi-utils.h"

gchar *
qmi_utils_str_hex (gconstpointer mem,
                   gsize size,
                   gchar delimiter)
{
    const guint8 *data = mem;
	gsize i;
	gsize j;
	gsize new_str_length;
	gchar *new_str;

	/* Get new string length. If input string has N bytes, we need:
	 * - 1 byte for last NUL char
	 * - 2N bytes for hexadecimal char representation of each byte...
	 * - N-1 bytes for the separator ':'
	 * So... a total of (1+2N+N-1) = 3N bytes are needed... */
	new_str_length =  3 * size;

	/* Allocate memory for new array and initialize contents to NUL */
	new_str = g_malloc0 (new_str_length);

	/* Print hexadecimal representation of each byte... */
	for (i = 0, j = 0; i < size; i++, j += 3) {
		/* Print character in output string... */
		snprintf (&new_str[j], 3, "%02X", data[i]);
		/* And if needed, add separator */
		if (i != (size - 1) )
			new_str[j + 2] = delimiter;
	}

	/* Set output string */
	return new_str;
}

void
qmi_utils_read_guint8_from_buffer (const guint8 **buffer,
                                   guint16       *buffer_size,
                                   guint8        *out)
{
    g_assert (out != NULL);
    g_assert (buffer != NULL);
    g_assert (buffer_size != NULL);
    g_assert (*buffer_size >= 1);

    *out = (*buffer)[0];

    *buffer = &((*buffer)[1]);
    *buffer_size = (*buffer_size) - 1;
}

void
qmi_utils_read_gint8_from_buffer (const guint8 **buffer,
                                  guint16       *buffer_size,
                                  gint8         *out)
{
    g_assert (out != NULL);
    g_assert (buffer != NULL);
    g_assert (buffer_size != NULL);
    g_assert (*buffer_size >= 1);

    *out = *((gint8 *)(&((*buffer)[0])));

    *buffer = &((*buffer)[1]);
    *buffer_size = (*buffer_size) - 1;
}

void
qmi_utils_read_guint16_from_buffer (const guint8 **buffer,
                                    guint16       *buffer_size,
                                    guint16       *out)
{
    g_assert (out != NULL);
    g_assert (buffer != NULL);
    g_assert (buffer_size != NULL);
    g_assert (*buffer_size >= 2);

    *out = GUINT16_FROM_LE (*((guint16 *)&((*buffer)[0])));

    *buffer = &((*buffer)[2]);
    *buffer_size = (*buffer_size) - 2;
}

void
qmi_utils_read_gint16_from_buffer (const guint8 **buffer,
                                   guint16       *buffer_size,
                                   gint16        *out)
{
    g_assert (out != NULL);
    g_assert (buffer != NULL);
    g_assert (buffer_size != NULL);
    g_assert (*buffer_size >= 2);

    *out = GINT16_FROM_LE (*((guint16 *)&((*buffer)[0])));

    *buffer = &((*buffer)[2]);
    *buffer_size = (*buffer_size) - 2;
}

void
qmi_utils_read_guint32_from_buffer (const guint8 **buffer,
                                    guint16       *buffer_size,
                                    guint32       *out)
{
    g_assert (out != NULL);
    g_assert (buffer != NULL);
    g_assert (buffer_size != NULL);
    g_assert (*buffer_size >= 4);

    *out = GUINT32_FROM_LE (*((guint32 *)&((*buffer)[0])));

    *buffer = &((*buffer)[4]);
    *buffer_size = (*buffer_size) - 4;
}

void
qmi_utils_read_gint32_from_buffer (const guint8 **buffer,
                                   guint16       *buffer_size,
                                   gint32        *out)
{
    g_assert (out != NULL);
    g_assert (buffer != NULL);
    g_assert (buffer_size != NULL);
    g_assert (*buffer_size >= 4);

    *out = GUINT32_FROM_LE (*((guint32 *)&((*buffer)[0])));

    *buffer = &((*buffer)[4]);
    *buffer_size = (*buffer_size) - 4;
}

void
qmi_utils_read_guint64_from_buffer (const guint8 **buffer,
                                    guint16       *buffer_size,
                                    guint64       *out)
{
    g_assert (out != NULL);
    g_assert (buffer != NULL);
    g_assert (buffer_size != NULL);
    g_assert (*buffer_size >= 8);

    *out = GUINT64_FROM_LE (*((guint64 *)&((*buffer)[0])));

    *buffer = &((*buffer)[8]);
    *buffer_size = (*buffer_size) - 8;
}

void
qmi_utils_read_gint64_from_buffer (const guint8 **buffer,
                                   guint16       *buffer_size,
                                   gint64        *out)
{
    g_assert (out != NULL);
    g_assert (buffer != NULL);
    g_assert (buffer_size != NULL);
    g_assert (*buffer_size >= 8);

    *out = GUINT64_FROM_LE (*((guint64 *)&((*buffer)[0])));

    *buffer = &((*buffer)[8]);
    *buffer_size = (*buffer_size) - 8;
}

void
qmi_utils_read_sized_guint_from_buffer (const guint8 **buffer,
                                        guint16       *buffer_size,
                                        guint          n_bytes,
                                        guint64       *out)
{
    guint64 tmp = 0;

    g_assert (out != NULL);
    g_assert (buffer != NULL);
    g_assert (buffer_size != NULL);
    g_assert (*buffer_size >= n_bytes);

    memcpy (&tmp, *buffer, n_bytes);
    *out = GUINT64_FROM_LE (tmp);

    *buffer = &((*buffer)[n_bytes]);
    *buffer_size = (*buffer_size) - n_bytes;
}

void
qmi_utils_write_guint8_to_buffer (guint8  **buffer,
                                  guint16  *buffer_size,
                                  guint8   *in)
{
    g_assert (in != NULL);
    g_assert (buffer != NULL);
    g_assert (buffer_size != NULL);
    g_assert (*buffer_size >= 1);

    (*buffer)[0] = *in;

    *buffer = &((*buffer)[1]);
    *buffer_size = (*buffer_size) - 1;
}

void
qmi_utils_write_gint8_to_buffer (guint8  **buffer,
                                 guint16  *buffer_size,
                                 gint8    *in)
{
    g_assert (in != NULL);
    g_assert (buffer != NULL);
    g_assert (buffer_size != NULL);
    g_assert (*buffer_size >= 1);

    *((gint8 *)(&((*buffer)[0]))) = *in;

    *buffer = &((*buffer)[1]);
    *buffer_size = (*buffer_size) - 1;
}

void
qmi_utils_write_guint16_to_buffer (guint8  **buffer,
                                   guint16  *buffer_size,
                                   guint16  *in)
{
    g_assert (in != NULL);
    g_assert (buffer != NULL);
    g_assert (buffer_size != NULL);
    g_assert (*buffer_size >= 2);

    *((guint16 *)(&((*buffer)[0]))) = GUINT16_TO_LE (*in);

    *buffer = &((*buffer)[2]);
    *buffer_size = (*buffer_size) - 2;
}

void
qmi_utils_write_gint16_to_buffer (guint8  **buffer,
                                  guint16  *buffer_size,
                                  gint16   *in)
{
    g_assert (in != NULL);
    g_assert (buffer != NULL);
    g_assert (buffer_size != NULL);
    g_assert (*buffer_size >= 2);

    *((gint16 *)(&((*buffer)[0]))) = GINT16_TO_LE (*in);

    *buffer = &((*buffer)[2]);
    *buffer_size = (*buffer_size) - 2;
}

void
qmi_utils_write_guint32_to_buffer (guint8  **buffer,
                                   guint16  *buffer_size,
                                   guint32  *in)
{
    g_assert (in != NULL);
    g_assert (buffer != NULL);
    g_assert (buffer_size != NULL);
    g_assert (*buffer_size >= 4);

    *((guint32 *)(&((*buffer)[0]))) = GUINT32_TO_LE (*in);

    *buffer = &((*buffer)[4]);
    *buffer_size = (*buffer_size) - 4;
}

void
qmi_utils_write_gint32_to_buffer (guint8  **buffer,
                                  guint16  *buffer_size,
                                  gint32   *in)
{
    g_assert (in != NULL);
    g_assert (buffer != NULL);
    g_assert (buffer_size != NULL);
    g_assert (*buffer_size >= 4);

    *((gint32 *)(&((*buffer)[0]))) = GINT32_TO_LE (*in);

    *buffer = &((*buffer)[4]);
    *buffer_size = (*buffer_size) - 4;
}

void
qmi_utils_write_guint64_to_buffer (guint8  **buffer,
                                   guint16  *buffer_size,
                                   guint64  *in)
{
    g_assert (in != NULL);
    g_assert (buffer != NULL);
    g_assert (buffer_size != NULL);
    g_assert (*buffer_size >= 8);

    *((guint64 *)(&((*buffer)[0]))) = GUINT64_TO_LE (*in);

    *buffer = &((*buffer)[8]);
    *buffer_size = (*buffer_size) - 8;
}

void
qmi_utils_write_gint64_to_buffer (guint8  **buffer,
                                  guint16  *buffer_size,
                                  gint64   *in)
{
    g_assert (in != NULL);
    g_assert (buffer != NULL);
    g_assert (buffer_size != NULL);
    g_assert (*buffer_size >= 8);

    *((gint64 *)(&((*buffer)[0]))) = GINT64_TO_LE (*in);

    *buffer = &((*buffer)[8]);
    *buffer_size = (*buffer_size) - 8;
}

void
qmi_utils_write_sized_guint_to_buffer (guint8  **buffer,
                                       guint16  *buffer_size,
                                       guint     n_bytes,
                                       guint64  *in)
{
    guint64 tmp;

    g_assert (in != NULL);
    g_assert (buffer != NULL);
    g_assert (buffer_size != NULL);
    g_assert (*buffer_size >= n_bytes);

    tmp = GUINT64_TO_LE (*in);
    memcpy (*buffer, &tmp, n_bytes);

    *buffer = &((*buffer)[n_bytes]);
    *buffer_size = (*buffer_size) - n_bytes;
}

void
qmi_utils_read_string_from_buffer (const guint8 **buffer,
                                   guint16       *buffer_size,
                                   guint8         length_prefix_size,
                                   guint16        max_size,
                                   gchar        **out)
{
    guint16 string_length;
    guint16 valid_string_length;
    guint8 string_length_8;
    guint16 string_length_16;

    g_assert (out != NULL);
    g_assert (buffer != NULL);
    g_assert (buffer_size != NULL);
    g_assert (length_prefix_size == 0 ||
              length_prefix_size == 8 ||
              length_prefix_size == 16);

    switch (length_prefix_size) {
    case 0:
        /* If no length prefix given, read the whole buffer into a string */
        string_length = *buffer_size;
        break;
    case 8:
        qmi_utils_read_guint8_from_buffer (buffer, buffer_size, &string_length_8);
        string_length = string_length_8;
        break;
    case 16:
        qmi_utils_read_guint16_from_buffer (buffer, buffer_size, &string_length_16);
        string_length = string_length_16;
        break;
    default:
        g_assert_not_reached ();
    }

    if (max_size > 0 && string_length > max_size)
        valid_string_length = max_size;
    else
        valid_string_length = string_length;

    /* Read 'valid_string_length' bytes */
    *out = g_malloc (valid_string_length + 1);
    memcpy (*out, *buffer, valid_string_length);
    (*out)[valid_string_length] = '\0';

    /* And walk 'string_length' bytes */
    *buffer = &((*buffer)[string_length]);
    *buffer_size = (*buffer_size) - string_length;
}

void
qmi_utils_read_fixed_size_string_from_buffer (const guint8 **buffer,
                                              guint16       *buffer_size,
                                              guint16        fixed_size,
                                              gchar         *out)
{
    g_assert (out != NULL);
    g_assert (buffer != NULL);
    g_assert (buffer_size != NULL);
    g_assert (fixed_size > 0);

    memcpy (out, *buffer, fixed_size);

    *buffer = &((*buffer)[fixed_size]);
    *buffer_size = (*buffer_size) - fixed_size;
}

void
qmi_utils_write_string_to_buffer (guint8      **buffer,
                                  guint16      *buffer_size,
                                  guint8        length_prefix_size,
                                  const gchar  *in)
{
    guint16 len;
    guint8 len_8;
    guint16 len_16;

    g_assert (in != NULL);
    g_assert (buffer != NULL);
    g_assert (buffer_size != NULL);
    g_assert (length_prefix_size == 0 ||
              length_prefix_size == 8 ||
              length_prefix_size == 16);

    len = (guint16) strlen (in);

    switch (length_prefix_size) {
    case 0:
        break;
    case 8:
        g_warn_if_fail (len <= G_MAXUINT8);
        len_8 = (guint8)len;
        qmi_utils_write_guint8_to_buffer (buffer, buffer_size, &len_8);
        break;
    case 16:
        g_warn_if_fail (len <= G_MAXUINT16);
        len_16 = (guint16)len;
        qmi_utils_write_guint16_to_buffer (buffer, buffer_size, &len_16);
        break;
    default:
        g_assert_not_reached ();
    }

    memcpy (*buffer, in, len);
    *buffer = &((*buffer)[len]);
    *buffer_size = (*buffer_size) - len;
}

void
qmi_utils_write_fixed_size_string_to_buffer (guint8      **buffer,
                                             guint16      *buffer_size,
                                             guint16       fixed_size,
                                             const gchar  *in)
{
    g_assert (in != NULL);
    g_assert (buffer != NULL);
    g_assert (buffer_size != NULL);
    g_assert (fixed_size > 0);

    memcpy (*buffer, in, fixed_size);
    *buffer = &((*buffer)[fixed_size]);
    *buffer_size = (*buffer_size) - fixed_size;
}
