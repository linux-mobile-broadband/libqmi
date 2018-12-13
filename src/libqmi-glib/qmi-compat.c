
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
 * Copyright (C) 2016-2018 Aleksander Morgado <aleksander@aleksander.es>
 */

#include <string.h>

#include "qmi-compat.h"
#include "qmi-utils.h"

#ifndef QMI_DISABLE_DEPRECATED


#if defined UTILS_ENABLE_TRACE
static void
print_read_bytes_trace (const gchar *type,
                        gconstpointer buffer,
                        gconstpointer out,
                        guint n_bytes)
{
    gchar *str1;
    gchar *str2;

    str1 = __qmi_utils_str_hex (buffer, n_bytes, ':');
    str2 = __qmi_utils_str_hex (out, n_bytes, ':');

    g_debug ("Read %s (%s) --> (%s)", type, str1, str2);
    g_warn_if_fail (g_str_equal (str1, str2));

    g_free (str1);
    g_free (str2);
}
#else
#define print_read_bytes_trace(...)
#endif

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

    print_read_bytes_trace ("guint8", &(*buffer)[0], out, 1);

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

    *out = (gint8)(*buffer)[0];

    print_read_bytes_trace ("gint8", &(*buffer)[0], out, 1);

    *buffer = &((*buffer)[1]);
    *buffer_size = (*buffer_size) - 1;
}

void
qmi_utils_read_guint16_from_buffer (const guint8 **buffer,
                                    guint16       *buffer_size,
                                    QmiEndian      endian,
                                    guint16       *out)
{
    g_assert (out != NULL);
    g_assert (buffer != NULL);
    g_assert (buffer_size != NULL);
    g_assert (*buffer_size >= 2);

    memcpy (out, &((*buffer)[0]), 2);
    if (endian == QMI_ENDIAN_BIG)
        *out = GUINT16_FROM_BE (*out);
    else
        *out = GUINT16_FROM_LE (*out);

    print_read_bytes_trace ("guint16", &(*buffer)[0], out, 2);

    *buffer = &((*buffer)[2]);
    *buffer_size = (*buffer_size) - 2;
}

void
qmi_utils_read_gint16_from_buffer (const guint8 **buffer,
                                   guint16       *buffer_size,
                                   QmiEndian      endian,
                                   gint16        *out)
{
    g_assert (out != NULL);
    g_assert (buffer != NULL);
    g_assert (buffer_size != NULL);
    g_assert (*buffer_size >= 2);

    memcpy (out, &((*buffer)[0]), 2);
    if (endian == QMI_ENDIAN_BIG)
        *out = GINT16_FROM_BE (*out);
    else
        *out = GINT16_FROM_LE (*out);

    print_read_bytes_trace ("gint16", &(*buffer)[0], out, 2);

    *buffer = &((*buffer)[2]);
    *buffer_size = (*buffer_size) - 2;
}

void
qmi_utils_read_guint32_from_buffer (const guint8 **buffer,
                                    guint16       *buffer_size,
                                    QmiEndian      endian,
                                    guint32       *out)
{
    g_assert (out != NULL);
    g_assert (buffer != NULL);
    g_assert (buffer_size != NULL);
    g_assert (*buffer_size >= 4);

    memcpy (out, &((*buffer)[0]), 4);
    if (endian == QMI_ENDIAN_BIG)
        *out = GUINT32_FROM_BE (*out);
    else
        *out = GUINT32_FROM_LE (*out);

    print_read_bytes_trace ("guint32", &(*buffer)[0], out, 4);

    *buffer = &((*buffer)[4]);
    *buffer_size = (*buffer_size) - 4;
}

void
qmi_utils_read_gint32_from_buffer (const guint8 **buffer,
                                   guint16       *buffer_size,
                                   QmiEndian      endian,
                                   gint32        *out)
{
    g_assert (out != NULL);
    g_assert (buffer != NULL);
    g_assert (buffer_size != NULL);
    g_assert (*buffer_size >= 4);

    memcpy (out, &((*buffer)[0]), 4);
    if (endian == QMI_ENDIAN_BIG)
        *out = GINT32_FROM_BE (*out);
    else
        *out = GINT32_FROM_LE (*out);

    print_read_bytes_trace ("gint32", &(*buffer)[0], out, 4);

    *buffer = &((*buffer)[4]);
    *buffer_size = (*buffer_size) - 4;
}

void
qmi_utils_read_guint64_from_buffer (const guint8 **buffer,
                                    guint16       *buffer_size,
                                    QmiEndian      endian,
                                    guint64       *out)
{
    g_assert (out != NULL);
    g_assert (buffer != NULL);
    g_assert (buffer_size != NULL);
    g_assert (*buffer_size >= 8);

    memcpy (out, &((*buffer)[0]), 8);
    if (endian == QMI_ENDIAN_BIG)
        *out = GUINT64_FROM_BE (*out);
    else
        *out = GUINT64_FROM_LE (*out);

    print_read_bytes_trace ("guint64", &(*buffer)[0], out, 8);

    *buffer = &((*buffer)[8]);
    *buffer_size = (*buffer_size) - 8;
}

void
qmi_utils_read_gint64_from_buffer (const guint8 **buffer,
                                   guint16       *buffer_size,
                                   QmiEndian      endian,
                                   gint64        *out)
{
    g_assert (out != NULL);
    g_assert (buffer != NULL);
    g_assert (buffer_size != NULL);
    g_assert (*buffer_size >= 8);

    memcpy (out, &((*buffer)[0]), 8);
    if (endian == QMI_ENDIAN_BIG)
        *out = GINT64_FROM_BE (*out);
    else
        *out = GINT64_FROM_LE (*out);

    print_read_bytes_trace ("gint64", &(*buffer)[0], out, 8);

    *buffer = &((*buffer)[8]);
    *buffer_size = (*buffer_size) - 8;
}

void
qmi_utils_read_sized_guint_from_buffer (const guint8 **buffer,
                                        guint16       *buffer_size,
                                        guint          n_bytes,
                                        QmiEndian      endian,
                                        guint64       *out)
{
    g_assert (out != NULL);
    g_assert (buffer != NULL);
    g_assert (buffer_size != NULL);
    g_assert (*buffer_size >= n_bytes);
    g_assert (n_bytes <= 8);

    *out = 0;

    /* In Little Endian, we copy the bytes to the beginning of the output
     * buffer. */
    if (endian == QMI_ENDIAN_LITTLE) {
        memcpy (out, *buffer, n_bytes);
        *out = GUINT64_FROM_LE (*out);
    }
    /* In Big Endian, we copy the bytes to the end of the output buffer */
    else {
        guint8 tmp[8] = { 0 };

        memcpy (&tmp[8 - n_bytes], *buffer, n_bytes);
        memcpy (out, &tmp[0], 8);
        *out = GUINT64_FROM_BE (*out);
    }

    *buffer = &((*buffer)[n_bytes]);
    *buffer_size = (*buffer_size) - n_bytes;
}

void
qmi_utils_read_gfloat_from_buffer (const guint8 **buffer,
                                   guint16       *buffer_size,
                                   gfloat        *out)
{
    g_assert (out != NULL);
    g_assert (buffer != NULL);
    g_assert (buffer_size != NULL);
    g_assert (*buffer_size >= 4);

    /* Yeah, do this for now */
    memcpy (out, &((*buffer)[0]), 4);

    print_read_bytes_trace ("gfloat", &(*buffer)[0], out, 4);

    *buffer = &((*buffer)[4]);
    *buffer_size = (*buffer_size) - 4;
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

    memcpy (&(*buffer)[0], in, sizeof (*in));

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

    memcpy (&(*buffer)[0], in, sizeof (*in));

    *buffer = &((*buffer)[1]);
    *buffer_size = (*buffer_size) - 1;
}

void
qmi_utils_write_guint16_to_buffer (guint8  **buffer,
                                   guint16  *buffer_size,
                                   QmiEndian endian,
                                   guint16  *in)
{
    guint16 tmp;

    g_assert (in != NULL);
    g_assert (buffer != NULL);
    g_assert (buffer_size != NULL);
    g_assert (*buffer_size >= 2);

    if (endian == QMI_ENDIAN_BIG)
        tmp = GUINT16_TO_BE (*in);
    else
        tmp = GUINT16_TO_LE (*in);
    memcpy (&(*buffer)[0], &tmp, sizeof (tmp));

    *buffer = &((*buffer)[2]);
    *buffer_size = (*buffer_size) - 2;
}

void
qmi_utils_write_gint16_to_buffer (guint8  **buffer,
                                  guint16  *buffer_size,
                                  QmiEndian endian,
                                  gint16   *in)
{
    gint16 tmp;

    g_assert (in != NULL);
    g_assert (buffer != NULL);
    g_assert (buffer_size != NULL);
    g_assert (*buffer_size >= 2);

    if (endian == QMI_ENDIAN_BIG)
        tmp = GINT16_TO_BE (*in);
    else
        tmp = GINT16_TO_LE (*in);
    memcpy (&(*buffer)[0], &tmp, sizeof (tmp));

    *buffer = &((*buffer)[2]);
    *buffer_size = (*buffer_size) - 2;
}

void
qmi_utils_write_guint32_to_buffer (guint8  **buffer,
                                   guint16  *buffer_size,
                                   QmiEndian endian,
                                   guint32  *in)
{
    guint32 tmp;

    g_assert (in != NULL);
    g_assert (buffer != NULL);
    g_assert (buffer_size != NULL);
    g_assert (*buffer_size >= 4);

    if (endian == QMI_ENDIAN_BIG)
        tmp = GUINT32_TO_BE (*in);
    else
        tmp = GUINT32_TO_LE (*in);
    memcpy (&(*buffer)[0], &tmp, sizeof (tmp));

    *buffer = &((*buffer)[4]);
    *buffer_size = (*buffer_size) - 4;
}

void
qmi_utils_write_gint32_to_buffer (guint8  **buffer,
                                  guint16  *buffer_size,
                                  QmiEndian endian,
                                  gint32   *in)
{
    gint32 tmp;

    g_assert (in != NULL);
    g_assert (buffer != NULL);
    g_assert (buffer_size != NULL);
    g_assert (*buffer_size >= 4);

    if (endian == QMI_ENDIAN_BIG)
        tmp = GINT32_TO_BE (*in);
    else
        tmp = GINT32_TO_LE (*in);
    memcpy (&(*buffer)[0], &tmp, sizeof (tmp));

    *buffer = &((*buffer)[4]);
    *buffer_size = (*buffer_size) - 4;
}

void
qmi_utils_write_guint64_to_buffer (guint8  **buffer,
                                   guint16  *buffer_size,
                                   QmiEndian endian,
                                   guint64  *in)
{
    guint64 tmp;

    g_assert (in != NULL);
    g_assert (buffer != NULL);
    g_assert (buffer_size != NULL);
    g_assert (*buffer_size >= 8);

    if (endian == QMI_ENDIAN_BIG)
        tmp = GUINT64_TO_BE (*in);
    else
        tmp = GUINT64_TO_LE (*in);
    memcpy (&(*buffer)[0], &tmp, sizeof (tmp));

    *buffer = &((*buffer)[8]);
    *buffer_size = (*buffer_size) - 8;
}

void
qmi_utils_write_gint64_to_buffer (guint8  **buffer,
                                  guint16  *buffer_size,
                                  QmiEndian endian,
                                  gint64   *in)
{
    gint64 tmp;

    g_assert (in != NULL);
    g_assert (buffer != NULL);
    g_assert (buffer_size != NULL);
    g_assert (*buffer_size >= 8);

    if (endian == QMI_ENDIAN_BIG)
        tmp = GINT64_TO_BE (*in);
    else
        tmp = GINT64_TO_LE (*in);
    memcpy (&(*buffer)[0], &tmp, sizeof (tmp));

    *buffer = &((*buffer)[8]);
    *buffer_size = (*buffer_size) - 8;
}

void
qmi_utils_write_sized_guint_to_buffer (guint8  **buffer,
                                       guint16  *buffer_size,
                                       guint     n_bytes,
                                       QmiEndian endian,
                                       guint64  *in)
{
    guint64 tmp;

    g_assert (in != NULL);
    g_assert (buffer != NULL);
    g_assert (buffer_size != NULL);
    g_assert (*buffer_size >= n_bytes);
    g_assert (n_bytes <= 8);

    if (endian == QMI_ENDIAN_BIG)
        tmp = GUINT64_TO_BE (*in);
    else
        tmp = GUINT64_TO_LE (*in);

    /* In Little Endian, we read the bytes from the beginning of the buffer */
    if (endian == QMI_ENDIAN_LITTLE) {
        memcpy (*buffer, &tmp, n_bytes);
    }
    /* In Big Endian, we read the bytes from the end of the buffer */
    else {
        guint8 tmp_buffer[8];

        memcpy (&tmp_buffer[0], &tmp, 8);
        memcpy (*buffer, &tmp_buffer[8 - n_bytes], n_bytes);
    }

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
        qmi_utils_read_guint8_from_buffer (buffer,
                                           buffer_size,
                                           &string_length_8);
        string_length = string_length_8;
        break;
    case 16:
        qmi_utils_read_guint16_from_buffer (buffer,
                                            buffer_size,
                                            QMI_ENDIAN_LITTLE,
                                            &string_length_16);
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
    gsize len;
    guint8 len_8;
    guint16 len_16;

    g_assert (in != NULL);
    g_assert (buffer != NULL);
    g_assert (buffer_size != NULL);
    g_assert (length_prefix_size == 0 ||
              length_prefix_size == 8 ||
              length_prefix_size == 16);

    len = strlen (in);

    g_assert (   len + (length_prefix_size/8) <= *buffer_size
              || (length_prefix_size==8 && ((int) G_MAXUINT8 + 1) < *buffer_size));

    switch (length_prefix_size) {
    case 0:
        break;
    case 8:
        if (len > G_MAXUINT8) {
            g_warn_if_reached ();
            len = G_MAXUINT8;
        }
        len_8 = (guint8)len;
        qmi_utils_write_guint8_to_buffer (buffer,
                                          buffer_size,
                                          &len_8);
        break;
    case 16:
        /* already asserted that @len is no larger then @buffer_size */
        len_16 = (guint16)len;
        qmi_utils_write_guint16_to_buffer (buffer,
                                           buffer_size,
                                           QMI_ENDIAN_LITTLE,
                                           &len_16);
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
    g_assert (fixed_size <= *buffer_size);

    memcpy (*buffer, in, fixed_size);
    *buffer = &((*buffer)[fixed_size]);
    *buffer_size = (*buffer_size) - fixed_size;
}

gboolean
qmi_message_dms_set_service_programming_code_input_get_new (
    QmiMessageDmsSetServiceProgrammingCodeInput *self,
    const gchar **arg_new,
    GError **error)
{
    return qmi_message_dms_set_service_programming_code_input_get_new_code (self, arg_new, error);
}

gboolean
qmi_message_dms_set_service_programming_code_input_set_new (
    QmiMessageDmsSetServiceProgrammingCodeInput *self,
    const gchar *arg_new,
    GError **error)
{
    return qmi_message_dms_set_service_programming_code_input_set_new_code (self, arg_new, error);
}

gboolean
qmi_message_dms_set_service_programming_code_input_get_current (
    QmiMessageDmsSetServiceProgrammingCodeInput *self,
    const gchar **arg_current,
    GError **error)
{
  return qmi_message_dms_set_service_programming_code_input_get_current_code (self, arg_current, error);
}

gboolean
qmi_message_dms_set_service_programming_code_input_set_current (
    QmiMessageDmsSetServiceProgrammingCodeInput *self,
    const gchar *arg_current,
    GError **error)
{
  return qmi_message_dms_set_service_programming_code_input_set_current_code (self, arg_current, error);
}

gchar *
qmi_message_get_printable (QmiMessage  *self,
                           const gchar *line_prefix)
{
    return qmi_message_get_printable_full (self, NULL, line_prefix);
}

gboolean
qmi_message_get_version_introduced (QmiMessage *self,
                                    guint      *major,
                                    guint      *minor)
{
    return qmi_message_get_version_introduced_full (self, NULL, major, minor);
}

gboolean
qmi_message_tlv_read_gfloat (QmiMessage  *self,
                             gsize        tlv_offset,
                             gsize       *offset,
                             gfloat      *out,
                             GError     **error)
{
    return qmi_message_tlv_read_gfloat_endian (self, tlv_offset, offset, __QMI_ENDIAN_HOST, out, error);
}

#define SESSION_INFORMATION_DEPRECATED_METHOD(BUNDLE_SUBSTR,METHOD_SUBSTR)                                 \
    gboolean                                                                                               \
    qmi_message_uim_##METHOD_SUBSTR##_input_get_session_information (                                      \
        QmiMessageUim##BUNDLE_SUBSTR##Input *self,                                                         \
        QmiUimSessionType *value_session_session_type,                                                     \
        const gchar **value_session_information_application_identifier,                                    \
        GError **error)                                                                                    \
    {                                                                                                      \
        /* just ignore the output string */                                                                \
        return qmi_message_uim_##METHOD_SUBSTR##_input_get_session (self,                                  \
                                                                    value_session_session_type,            \
                                                                    NULL,                                  \
                                                                    error);                                \
    }                                                                                                      \
    gboolean                                                                                               \
    qmi_message_uim_##METHOD_SUBSTR##_input_set_session_information (                                      \
        QmiMessageUim##BUNDLE_SUBSTR##Input *self,                                                         \
        QmiUimSessionType value_session_information_session_type,                                          \
        const gchar *value_session_information_application_identifier,                                     \
        GError **error)                                                                                    \
    {                                                                                                      \
        GArray   *array;                                                                                   \
        gboolean  ret;                                                                                     \
                                                                                                           \
        array = g_array_append_vals (g_array_new (FALSE, FALSE, sizeof (guint8)),                          \
                                     value_session_information_application_identifier,                     \
                                     strlen (value_session_information_application_identifier));           \
        ret = qmi_message_uim_##METHOD_SUBSTR##_input_set_session (self,                                   \
                                                                   value_session_information_session_type, \
                                                                   array,                                  \
                                                                   error);                                 \
        g_array_unref (array);                                                                             \
        return ret;                                                                                        \
    }

SESSION_INFORMATION_DEPRECATED_METHOD (ReadTransparent,   read_transparent)
SESSION_INFORMATION_DEPRECATED_METHOD (ReadRecord,        read_record)
SESSION_INFORMATION_DEPRECATED_METHOD (GetFileAttributes, get_file_attributes)
SESSION_INFORMATION_DEPRECATED_METHOD (SetPinProtection,  set_pin_protection)
SESSION_INFORMATION_DEPRECATED_METHOD (VerifyPin,         verify_pin)
SESSION_INFORMATION_DEPRECATED_METHOD (UnblockPin,        unblock_pin)
SESSION_INFORMATION_DEPRECATED_METHOD (ChangePin,         change_pin)

gboolean
qmi_device_close (QmiDevice *self,
                  GError **error)
{
    g_return_val_if_fail (QMI_IS_DEVICE (self), FALSE);
    qmi_device_close_async (self, 0, NULL, NULL, NULL);
    return TRUE;
}

#endif /* QMI_DISABLE_DEPRECATED */
