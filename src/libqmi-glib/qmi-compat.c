
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
 * Copyright (C) 2016-2022 Aleksander Morgado <aleksander@aleksander.es>
 */

#include <string.h>

#include "qmi-common.h"
#include "qmi-compat.h"
#include "qmi-helpers.h"
#include "qmi-enum-types.h"
#include "qmi-flag-types.h"

#ifndef QMI_DISABLE_DEPRECATED

/*****************************************************************************/

#if defined UTILS_ENABLE_TRACE
static void
print_read_bytes_trace (const gchar *type,
                        gconstpointer buffer,
                        gconstpointer out,
                        guint n_bytes)
{
    gchar *str1;
    gchar *str2;

    str1 = qmi_common_str_hex (buffer, n_bytes, ':');
    str2 = qmi_common_str_hex (out, n_bytes, ':');

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

/*****************************************************************************/

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
qmi_message_get_version_introduced_full (QmiMessage        *self,
                                         QmiMessageContext *context,
                                         guint             *major,
                                         guint             *minor)
{
    /* We keep the method just avoid breaking API, but this is really no longer
     * usable */
    return FALSE;
}

gboolean
qmi_message_tlv_read_gfloat (QmiMessage  *self,
                             gsize        tlv_offset,
                             gsize       *offset,
                             gfloat      *out,
                             GError     **error)
{
    return qmi_message_tlv_read_gfloat_endian (self, tlv_offset, offset, QMI_ENDIAN_HOST, out, error);
}

/*****************************************************************************/

gboolean
qmi_device_close (QmiDevice *self,
                  GError **error)
{
    g_return_val_if_fail (QMI_IS_DEVICE (self), FALSE);
    qmi_device_close_async (self, 0, NULL, NULL, NULL);
    return TRUE;
}

QmiMessage *
qmi_device_command_finish (QmiDevice     *self,
                           GAsyncResult  *res,
                           GError       **error)
{
    return qmi_device_command_full_finish (self, res, error);
}

void
qmi_device_command (QmiDevice           *self,
                    QmiMessage          *message,
                    guint                timeout,
                    GCancellable        *cancellable,
                    GAsyncReadyCallback  callback,
                    gpointer             user_data)
{
    qmi_device_command_full (self, message, NULL, timeout, cancellable, callback, user_data);
}

/*****************************************************************************/

#if defined HAVE_QMI_MESSAGE_DMS_SET_SERVICE_PROGRAMMING_CODE

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

#endif /* HAVE_QMI_MESSAGE_DMS_SET_SERVICE_PROGRAMMING_CODE */

/*****************************************************************************/

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

#if defined HAVE_QMI_MESSAGE_UIM_READ_TRANSPARENT
SESSION_INFORMATION_DEPRECATED_METHOD (ReadTransparent, read_transparent)
#endif
#if defined HAVE_QMI_MESSAGE_UIM_READ_RECORD
SESSION_INFORMATION_DEPRECATED_METHOD (ReadRecord, read_record)
#endif
#if defined HAVE_QMI_MESSAGE_UIM_GET_FILE_ATTRIBUTES
SESSION_INFORMATION_DEPRECATED_METHOD (GetFileAttributes, get_file_attributes)
#endif
#if defined HAVE_QMI_MESSAGE_UIM_SET_PIN_PROTECTION
SESSION_INFORMATION_DEPRECATED_METHOD (SetPinProtection, set_pin_protection)
#endif
#if defined HAVE_QMI_MESSAGE_UIM_VERIFY_PIN
SESSION_INFORMATION_DEPRECATED_METHOD (VerifyPin, verify_pin)
#endif
#if defined HAVE_QMI_MESSAGE_UIM_UNBLOCK_PIN
SESSION_INFORMATION_DEPRECATED_METHOD (UnblockPin, unblock_pin)
#endif
#if defined HAVE_QMI_MESSAGE_UIM_CHANGE_PIN
SESSION_INFORMATION_DEPRECATED_METHOD (ChangePin, change_pin)
#endif

/*****************************************************************************/

GType
qmi_dms_dell_firmware_version_type_get_type (void)
{
    return qmi_dms_foxconn_firmware_version_type_get_type ();
}

const gchar *
qmi_dms_dell_firmware_version_type_get_string (QmiDeprecatedDmsDellFirmwareVersionType val)
{
    return qmi_dms_foxconn_firmware_version_type_get_string ((QmiDmsFoxconnFirmwareVersionType) val);
}

#if defined HAVE_QMI_MESSAGE_DMS_FOXCONN_GET_FIRMWARE_VERSION

GType
qmi_message_dms_dell_get_firmware_version_output_get_type (void)
{
    return qmi_message_dms_foxconn_get_firmware_version_output_get_type ();
}

gboolean
qmi_message_dms_dell_get_firmware_version_output_get_version (
    QmiDeprecatedMessageDmsDellGetFirmwareVersionOutput *self,
    const gchar **value_version,
    GError **error)
{
    return qmi_message_dms_foxconn_get_firmware_version_output_get_version (self, value_version, error);
}

gboolean
qmi_message_dms_dell_get_firmware_version_output_get_result (
    QmiDeprecatedMessageDmsDellGetFirmwareVersionOutput *self,
    GError **error)
{
    return qmi_message_dms_foxconn_get_firmware_version_output_get_result (self, error);
}

GType
qmi_message_dms_dell_get_firmware_version_input_get_type (void)
{
    return qmi_message_dms_foxconn_get_firmware_version_input_get_type ();
}

gboolean
qmi_message_dms_dell_get_firmware_version_input_get_version_type (
    QmiDeprecatedMessageDmsDellGetFirmwareVersionInput *self,
    QmiDeprecatedDmsDellFirmwareVersionType *value_version_type,
    GError **error)
{
    return qmi_message_dms_foxconn_get_firmware_version_input_get_version_type (self, (QmiDmsFoxconnFirmwareVersionType *)value_version_type, error);
}

gboolean
qmi_message_dms_dell_get_firmware_version_input_set_version_type (
    QmiDeprecatedMessageDmsDellGetFirmwareVersionInput *self,
    QmiDeprecatedDmsDellFirmwareVersionType value_version_type,
    GError **error)
{
    return qmi_message_dms_foxconn_get_firmware_version_input_set_version_type (self, (QmiDmsFoxconnFirmwareVersionType)value_version_type, error);
}

QmiDeprecatedMessageDmsDellGetFirmwareVersionInput *
qmi_message_dms_dell_get_firmware_version_input_ref (QmiDeprecatedMessageDmsDellGetFirmwareVersionInput *self)
{
    return qmi_message_dms_foxconn_get_firmware_version_input_ref (self);
}

void
qmi_message_dms_dell_get_firmware_version_input_unref (QmiDeprecatedMessageDmsDellGetFirmwareVersionInput *self)
{
    qmi_message_dms_foxconn_get_firmware_version_input_unref (self);
}

QmiDeprecatedMessageDmsDellGetFirmwareVersionInput *
qmi_message_dms_dell_get_firmware_version_input_new (void)
{
    return qmi_message_dms_foxconn_get_firmware_version_input_new ();
}

QmiDeprecatedMessageDmsDellGetFirmwareVersionOutput *
qmi_message_dms_dell_get_firmware_version_output_ref (QmiDeprecatedMessageDmsDellGetFirmwareVersionOutput *self)
{
    return qmi_message_dms_foxconn_get_firmware_version_output_ref (self);
}

void
qmi_message_dms_dell_get_firmware_version_output_unref (QmiDeprecatedMessageDmsDellGetFirmwareVersionOutput *self)
{
    qmi_message_dms_foxconn_get_firmware_version_output_unref (self);
}

void
qmi_client_dms_dell_get_firmware_version (
    QmiClientDms *self,
    QmiDeprecatedMessageDmsDellGetFirmwareVersionInput *input,
    guint timeout,
    GCancellable *cancellable,
    GAsyncReadyCallback callback,
    gpointer user_data)
{
    qmi_client_dms_foxconn_get_firmware_version (self, input, timeout, cancellable, callback, user_data);
}

QmiDeprecatedMessageDmsDellGetFirmwareVersionOutput *
qmi_client_dms_dell_get_firmware_version_finish (
    QmiClientDms *self,
    GAsyncResult *res,
    GError **error)
{
    return qmi_client_dms_foxconn_get_firmware_version_finish (self, res, error);
}

#endif /* HAVE_QMI_MESSAGE_DMS_FOXCONN_GET_FIRMWARE_VERSION */

/*****************************************************************************/

GType
qmi_dms_dell_device_mode_get_type (void)
{
    return qmi_dms_foxconn_device_mode_get_type ();
}

const gchar *
qmi_dms_dell_device_mode_get_string (QmiDeprecatedDmsDellDeviceMode val)
{
    return qmi_dms_foxconn_device_mode_get_string ((QmiDmsFoxconnDeviceMode) val);
}

#if defined HAVE_QMI_MESSAGE_DMS_FOXCONN_CHANGE_DEVICE_MODE

GType
qmi_message_dms_dell_change_device_mode_input_get_type (void)
{
    return qmi_message_dms_foxconn_change_device_mode_input_get_type ();
}

gboolean
qmi_message_dms_dell_change_device_mode_input_get_mode (
    QmiDeprecatedMessageDmsDellChangeDeviceModeInput *self,
    QmiDeprecatedDmsDellDeviceMode *value_mode,
    GError **error)
{
    return qmi_message_dms_foxconn_change_device_mode_input_get_mode (self, (QmiDmsFoxconnDeviceMode *) value_mode, error);
}

gboolean
qmi_message_dms_dell_change_device_mode_input_set_mode (
    QmiDeprecatedMessageDmsDellChangeDeviceModeInput *self,
    QmiDeprecatedDmsDellDeviceMode value_mode,
    GError **error)
{
    return qmi_message_dms_foxconn_change_device_mode_input_set_mode (self, (QmiDmsFoxconnDeviceMode) value_mode, error);
}

QmiDeprecatedMessageDmsDellChangeDeviceModeInput *
qmi_message_dms_dell_change_device_mode_input_ref (QmiDeprecatedMessageDmsDellChangeDeviceModeInput *self)
{
    return qmi_message_dms_foxconn_change_device_mode_input_ref (self);
}

void
qmi_message_dms_dell_change_device_mode_input_unref (QmiDeprecatedMessageDmsDellChangeDeviceModeInput *self)
{
    qmi_message_dms_foxconn_change_device_mode_input_unref (self);
}

QmiDeprecatedMessageDmsDellChangeDeviceModeInput *
qmi_message_dms_dell_change_device_mode_input_new (void)
{
    return qmi_message_dms_foxconn_change_device_mode_input_new ();
}

GType
qmi_message_dms_dell_change_device_mode_output_get_type (void)
{
    return qmi_message_dms_foxconn_change_device_mode_output_get_type ();
}

gboolean
qmi_message_dms_dell_change_device_mode_output_get_result (
    QmiDeprecatedMessageDmsDellChangeDeviceModeOutput *self,
    GError **error)
{
    return qmi_message_dms_foxconn_change_device_mode_output_get_result (self, error);
}

QmiDeprecatedMessageDmsDellChangeDeviceModeOutput *
qmi_message_dms_dell_change_device_mode_output_ref (QmiDeprecatedMessageDmsDellChangeDeviceModeOutput *self)
{
    return qmi_message_dms_foxconn_change_device_mode_output_ref (self);
}

void
qmi_message_dms_dell_change_device_mode_output_unref (QmiDeprecatedMessageDmsDellChangeDeviceModeOutput *self)
{
    qmi_message_dms_foxconn_change_device_mode_output_unref (self);
}

void
qmi_client_dms_dell_change_device_mode (
    QmiClientDms *self,
    QmiDeprecatedMessageDmsDellChangeDeviceModeInput *input,
    guint timeout,
    GCancellable *cancellable,
    GAsyncReadyCallback callback,
    gpointer user_data)
{
    qmi_client_dms_foxconn_change_device_mode (self, input, timeout, cancellable, callback, user_data);
}

QmiDeprecatedMessageDmsDellChangeDeviceModeOutput *
qmi_client_dms_dell_change_device_mode_finish (
    QmiClientDms *self,
    GAsyncResult *res,
    GError **error)
{
    return qmi_client_dms_foxconn_change_device_mode_finish (self, res, error);
}

#endif /* HAVE_QMI_MESSAGE_DMS_FOXCONN_CHANGE_DEVICE_MODE */

/*****************************************************************************/

#if defined HAVE_QMI_MESSAGE_NAS_GET_OPERATOR_NAME

gboolean
qmi_message_nas_get_operator_name_output_get_operator_nitz_information (
    QmiMessageNasGetOperatorNameOutput *self,
    QmiNasPlmnEncodingScheme *value_operator_nitz_information_name_encoding,
    QmiNasPlmnNameCountryInitials *value_operator_nitz_information_short_country_initials,
    QmiNasPlmnNameSpareBits *value_operator_nitz_information_long_name_spare_bits,
    QmiNasPlmnNameSpareBits *value_operator_nitz_information_short_name_spare_bits,
    const gchar **value_operator_nitz_information_long_name,
    const gchar **value_operator_nitz_information_short_name,
    GError **error)
{
    GArray *long_name = NULL;
    GArray *short_name = NULL;

    if (!qmi_message_nas_get_operator_name_output_get_nitz_information (
            self,
            value_operator_nitz_information_name_encoding,
            value_operator_nitz_information_short_country_initials,
            value_operator_nitz_information_long_name_spare_bits,
            value_operator_nitz_information_short_name_spare_bits,
            &long_name,
            &short_name,
            error))
        return FALSE;

    if (value_operator_nitz_information_long_name)
        *value_operator_nitz_information_long_name = (const gchar *)long_name->data;
    if (value_operator_nitz_information_short_name)
        *value_operator_nitz_information_short_name = (const gchar *)short_name->data;
    return TRUE;
}

#endif /* HAVE_QMI_MESSAGE_NAS_GET_OPERATOR_NAME */

#if defined HAVE_QMI_INDICATION_NAS_OPERATOR_NAME

gboolean
qmi_indication_nas_operator_name_output_get_operator_nitz_information (
    QmiIndicationNasOperatorNameOutput *self,
    QmiNasPlmnEncodingScheme *value_operator_nitz_information_name_encoding,
    QmiNasPlmnNameCountryInitials *value_operator_nitz_information_short_country_initials,
    QmiNasPlmnNameSpareBits *value_operator_nitz_information_long_name_spare_bits,
    QmiNasPlmnNameSpareBits *value_operator_nitz_information_short_name_spare_bits,
    const gchar **value_operator_nitz_information_long_name,
    const gchar **value_operator_nitz_information_short_name,
    GError **error)
{
    GArray *long_name = NULL;
    GArray *short_name = NULL;

    if (!qmi_indication_nas_operator_name_output_get_nitz_information (
            self,
            value_operator_nitz_information_name_encoding,
            value_operator_nitz_information_short_country_initials,
            value_operator_nitz_information_long_name_spare_bits,
            value_operator_nitz_information_short_name_spare_bits,
            &long_name,
            &short_name,
            error))
        return FALSE;

    if (value_operator_nitz_information_long_name)
        *value_operator_nitz_information_long_name = (const gchar *)long_name->data;
    if (value_operator_nitz_information_short_name)
        *value_operator_nitz_information_short_name = (const gchar *)short_name->data;
    return TRUE;
}

#endif /* HAVE_QMI_INDICATION_NAS_OPERATOR_NAME */

/*****************************************************************************/

#if defined HAVE_QMI_MESSAGE_NAS_GET_HOME_NETWORK

gboolean
qmi_message_nas_get_home_network_output_get_home_network_3gpp2 (
    QmiMessageNasGetHomeNetworkOutput *self,
    guint16 *value_home_network_3gpp2_mcc,
    guint16 *value_home_network_3gpp2_mnc,
    QmiNasNetworkDescriptionDisplay *value_home_network_3gpp2_display_description,
    QmiNasNetworkDescriptionEncoding *value_home_network_3gpp2_description_encoding,
    const gchar **value_home_network_3gpp2_description,
    GError **error)
{
    GArray *description = NULL;

    if (!qmi_message_nas_get_home_network_output_get_home_network_3gpp2_ext (
            self,
            value_home_network_3gpp2_mcc,
            value_home_network_3gpp2_mnc,
            value_home_network_3gpp2_display_description,
            value_home_network_3gpp2_description_encoding,
            &description,
            error))
        return FALSE;

    if (value_home_network_3gpp2_description)
        *value_home_network_3gpp2_description = (const gchar *)description->data;

    return TRUE;
}

#endif /* HAVE_QMI_MESSAGE_NAS_GET_HOME_NETWORK */

#if defined HAVE_QMI_MESSAGE_NAS_GET_CELL_LOCATION_INFO

typedef struct {
    GArray *geran_info_cell;
} MessageNasGetCellLocationInfoOutputCompatContext;

static void
message_nas_get_cell_location_info_output_compat_context_free (MessageNasGetCellLocationInfoOutputCompatContext *ctx)
{
    if (ctx->geran_info_cell)
        g_array_unref (ctx->geran_info_cell);
    g_slice_free (MessageNasGetCellLocationInfoOutputCompatContext, ctx);
}

static MessageNasGetCellLocationInfoOutputCompatContext *
message_nas_get_cell_location_info_output_get_compat_context (QmiMessageNasGetCellLocationInfoOutput *self)
{
    MessageNasGetCellLocationInfoOutputCompatContext *ctx;

    ctx = qmi_message_nas_get_cell_location_info_output_get_compat_context (self);
    if (!ctx) {
        ctx = g_slice_new0 (MessageNasGetCellLocationInfoOutputCompatContext);
        qmi_message_nas_get_cell_location_info_output_set_compat_context (self, ctx, (GDestroyNotify)message_nas_get_cell_location_info_output_compat_context_free);
    }

    return ctx;
}

/* This PLMN string is returned because it's a 3-char long valid UTF-8. */
static const gchar invalid_plmn_str[] = "   ";

gboolean
qmi_message_nas_get_cell_location_info_output_get_intrafrequency_lte_info (
    QmiMessageNasGetCellLocationInfoOutput *self,
    gboolean *value_intrafrequency_lte_info_ue_in_idle,
    const gchar **value_intrafrequency_lte_info_plmn,
    guint16 *value_intrafrequency_lte_info_tracking_area_code,
    guint32 *value_intrafrequency_lte_info_global_cell_id,
    guint16 *value_intrafrequency_lte_info_eutra_absolute_rf_channel_number,
    guint16 *value_intrafrequency_lte_info_serving_cell_id,
    guint8 *value_intrafrequency_lte_info_cell_reselection_priority,
    guint8 *value_intrafrequency_lte_info_s_non_intra_search_threshold,
    guint8 *value_intrafrequency_lte_info_serving_cell_low_threshold,
    guint8 *value_intrafrequency_lte_info_s_intra_search_threshold,
    GArray **value_intrafrequency_lte_info_cell,
    GError **error)
{
    if (!qmi_message_nas_get_cell_location_info_output_get_intrafrequency_lte_info_v2 (
             self,
             value_intrafrequency_lte_info_ue_in_idle,
             NULL,
             value_intrafrequency_lte_info_tracking_area_code,
             value_intrafrequency_lte_info_global_cell_id,
             value_intrafrequency_lte_info_eutra_absolute_rf_channel_number,
             value_intrafrequency_lte_info_serving_cell_id,
             value_intrafrequency_lte_info_cell_reselection_priority,
             value_intrafrequency_lte_info_s_non_intra_search_threshold,
             value_intrafrequency_lte_info_serving_cell_low_threshold,
             value_intrafrequency_lte_info_s_intra_search_threshold,
             value_intrafrequency_lte_info_cell,
             error))
      return FALSE;

    if (value_intrafrequency_lte_info_plmn)
        *value_intrafrequency_lte_info_plmn = invalid_plmn_str;
    return TRUE;
}

gboolean
qmi_message_nas_get_cell_location_info_output_get_umts_info (
    QmiMessageNasGetCellLocationInfoOutput *self,
    guint16 *value_umts_info_cell_id,
    const gchar **value_umts_info_plmn,
    guint16 *value_umts_info_lac,
    guint16 *value_umts_info_utra_absolute_rf_channel_number,
    guint16 *value_umts_info_primary_scrambling_code,
    gint16 *value_umts_info_rscp,
    gint16 *value_umts_info_ecio,
    GArray **value_umts_info_cell,
    GArray **value_umts_info_neighboring_geran,
    GError **error)
{
    if (!qmi_message_nas_get_cell_location_info_output_get_umts_info_v2 (
           self,
           value_umts_info_cell_id,
           NULL,
           value_umts_info_lac,
           value_umts_info_utra_absolute_rf_channel_number,
           value_umts_info_primary_scrambling_code,
           value_umts_info_rscp,
           value_umts_info_ecio,
           value_umts_info_cell,
           value_umts_info_neighboring_geran,
           error))
        return FALSE;

    if (value_umts_info_plmn)
        *value_umts_info_plmn = invalid_plmn_str;
    return TRUE;
}

gboolean
qmi_message_nas_get_cell_location_info_output_get_geran_info (
    QmiMessageNasGetCellLocationInfoOutput *self,
    guint32 *value_geran_info_cell_id,
    const gchar **value_geran_info_plmn,
    guint16 *value_geran_info_lac,
    guint16 *value_geran_info_geran_absolute_rf_channel_number,
    guint8 *value_geran_info_base_station_identity_code,
    guint32 *value_geran_info_timing_advance,
    guint16 *value_geran_info_rx_level,
    GArray **value_geran_info_cell,
    GError **error)
{
    GArray *geran_info_v2_cell = NULL;

    if (!qmi_message_nas_get_cell_location_info_output_get_geran_info_v2 (
            self,
            value_geran_info_cell_id,
            NULL,
            value_geran_info_lac,
            value_geran_info_geran_absolute_rf_channel_number,
            value_geran_info_base_station_identity_code,
            value_geran_info_timing_advance,
            value_geran_info_rx_level,
            &geran_info_v2_cell,
            error))
        return FALSE;

    if (value_geran_info_plmn)
      *value_geran_info_plmn = invalid_plmn_str;

    if (value_geran_info_cell) {
        MessageNasGetCellLocationInfoOutputCompatContext *ctx;

        /* We have an array of QmiMessageNasGetCellLocationInfoOutputGeranInfoV2CellElement elements,
         * we need to return an array of QmiMessageNasGetCellLocationInfoOutputGeranInfoCellElement elements
         * instead.
         */
        ctx = message_nas_get_cell_location_info_output_get_compat_context (self);
        if (!ctx->geran_info_cell) {
            guint i;

            ctx->geran_info_cell = g_array_sized_new (FALSE, FALSE, sizeof (QmiMessageNasGetCellLocationInfoOutputGeranInfoCellElement), geran_info_v2_cell->len);
            for (i = 0; i < geran_info_v2_cell->len; i++) {
                QmiMessageNasGetCellLocationInfoOutputGeranInfoV2CellElement *elemv2;
                QmiMessageNasGetCellLocationInfoOutputGeranInfoCellElement elem;

                elemv2 = &g_array_index (geran_info_v2_cell, QmiMessageNasGetCellLocationInfoOutputGeranInfoV2CellElement, i);
                elem.cell_id = elemv2->cell_id;
                elem.plmn = (gchar *)invalid_plmn_str;
                elem.lac = elemv2->lac;
                elem.geran_absolute_rf_channel_number = elemv2->geran_absolute_rf_channel_number;
                elem.base_station_identity_code = elemv2->base_station_identity_code;
                elem.rx_level = elemv2->rx_level;
                g_array_append_val (ctx->geran_info_cell, elem);
            }
        }
        *value_geran_info_cell = ctx->geran_info_cell;
    }
    return TRUE;
}

#endif /* HAVE_QMI_MESSAGE_NAS_GET_CELL_LOCATION_INFO */

#if defined HAVE_QMI_MESSAGE_WDS_GET_DEFAULT_PROFILE_NUMBER

GType
qmi_message_wds_get_default_profile_num_input_get_type (void)
{
    return qmi_message_wds_get_default_profile_number_input_get_type ();
}

gboolean
qmi_message_wds_get_default_profile_num_input_get_profile_type (
    QmiDeprecatedMessageWdsGetDefaultProfileNumInput *self,
    QmiWdsProfileType *value_profile_type_profile_type,
    QmiWdsProfileFamily *value_profile_type_profile_family,
    GError **error)
{
    return qmi_message_wds_get_default_profile_number_input_get_profile_type (self,
                                                                              value_profile_type_profile_type,
                                                                              value_profile_type_profile_family,
                                                                              error);
}

gboolean
qmi_message_wds_get_default_profile_num_input_set_profile_type (
    QmiDeprecatedMessageWdsGetDefaultProfileNumInput *self,
    QmiWdsProfileType value_profile_type_profile_type,
    QmiWdsProfileFamily value_profile_type_profile_family,
    GError **error)
{
    return qmi_message_wds_get_default_profile_number_input_set_profile_type (self,
                                                                              value_profile_type_profile_type,
                                                                              value_profile_type_profile_family,
                                                                              error);
}

QmiDeprecatedMessageWdsGetDefaultProfileNumInput *
qmi_message_wds_get_default_profile_num_input_ref (QmiDeprecatedMessageWdsGetDefaultProfileNumInput *self)
{
    return qmi_message_wds_get_default_profile_number_input_ref (self);
}

void
qmi_message_wds_get_default_profile_num_input_unref (QmiDeprecatedMessageWdsGetDefaultProfileNumInput *self)
{
    qmi_message_wds_get_default_profile_number_input_unref (self);
}

QmiDeprecatedMessageWdsGetDefaultProfileNumInput *
qmi_message_wds_get_default_profile_num_input_new (void)
{
    return qmi_message_wds_get_default_profile_number_input_new ();
}

GType
qmi_message_wds_get_default_profile_num_output_get_type (void)
{
    return qmi_message_wds_get_default_profile_number_output_get_type ();
}

gboolean
qmi_message_wds_get_default_profile_num_output_get_result (
    QmiDeprecatedMessageWdsGetDefaultProfileNumOutput *self,
    GError **error)
{
    return qmi_message_wds_get_default_profile_number_output_get_result (self, error);
}

gboolean
qmi_message_wds_get_default_profile_num_output_get_default_profile_number (
    QmiDeprecatedMessageWdsGetDefaultProfileNumOutput *self,
    guint8 *value_default_profile_number,
    GError **error)
{
    return qmi_message_wds_get_default_profile_number_output_get_index (self,
                                                                        value_default_profile_number,
                                                                        error);
}

gboolean
qmi_message_wds_get_default_profile_num_output_get_extended_error_code (
    QmiDeprecatedMessageWdsGetDefaultProfileNumOutput *self,
    QmiWdsDsProfileError *value_extended_error_code,
    GError **error)
{
    return qmi_message_wds_get_default_profile_number_output_get_extended_error_code (self,
                                                                                      value_extended_error_code,
                                                                                      error);
}

QmiDeprecatedMessageWdsGetDefaultProfileNumOutput *
qmi_message_wds_get_default_profile_num_output_ref (QmiDeprecatedMessageWdsGetDefaultProfileNumOutput *self)
{
    return qmi_message_wds_get_default_profile_number_output_ref (self);
}

void
qmi_message_wds_get_default_profile_num_output_unref (QmiDeprecatedMessageWdsGetDefaultProfileNumOutput *self)
{
    qmi_message_wds_get_default_profile_number_output_unref (self);
}

void
qmi_client_wds_get_default_profile_num (
    QmiClientWds *self,
    QmiMessageWdsGetDefaultProfileNumberInput *input,
    guint timeout,
    GCancellable *cancellable,
    GAsyncReadyCallback callback,
    gpointer user_data)
{
    qmi_client_wds_get_default_profile_number (
        self,
        input,
        timeout,
        cancellable,
        callback,
        user_data);
}

QmiMessageWdsGetDefaultProfileNumberOutput *
qmi_client_wds_get_default_profile_num_finish (
    QmiClientWds *self,
    GAsyncResult *res,
    GError **error)
{
    return qmi_client_wds_get_default_profile_number_finish (self, res, error);
}

#endif /* HAVE_QMI_MESSAGE_WDS_GET_DEFAULT_PROFILE_NUMBER */

#if defined HAVE_QMI_MESSAGE_WDS_SET_DEFAULT_PROFILE_NUMBER

GType
qmi_message_wds_set_default_profile_num_output_get_type (void)
{
    return qmi_message_wds_set_default_profile_number_output_get_type ();
}

gboolean
qmi_message_wds_set_default_profile_num_input_get_profile_identifier (
    QmiDeprecatedMessageWdsSetDefaultProfileNumInput *self,
    QmiWdsProfileType *value_profile_identifier_profile_type,
    QmiWdsProfileFamily *value_profile_identifier_profile_family,
    guint8 *value_profile_identifier_profile_index,
    GError **error)
{
    return qmi_message_wds_set_default_profile_number_input_get_profile_identifier (self,
                                                                                    value_profile_identifier_profile_type,
                                                                                    value_profile_identifier_profile_family,
                                                                                    value_profile_identifier_profile_index,
                                                                                    error);
}

gboolean
qmi_message_wds_set_default_profile_num_input_set_profile_identifier (
    QmiDeprecatedMessageWdsSetDefaultProfileNumInput *self,
    QmiWdsProfileType value_profile_identifier_profile_type,
    QmiWdsProfileFamily value_profile_identifier_profile_family,
    guint8 value_profile_identifier_profile_index,
    GError **error)
{
    return qmi_message_wds_set_default_profile_number_input_set_profile_identifier (self,
                                                                                    value_profile_identifier_profile_type,
                                                                                    value_profile_identifier_profile_family,
                                                                                    value_profile_identifier_profile_index,
                                                                                    error);
}

QmiDeprecatedMessageWdsSetDefaultProfileNumInput *
qmi_message_wds_set_default_profile_num_input_ref (QmiDeprecatedMessageWdsSetDefaultProfileNumInput *self)
{
    return qmi_message_wds_set_default_profile_number_input_ref (self);
}

void
qmi_message_wds_set_default_profile_num_input_unref (QmiDeprecatedMessageWdsSetDefaultProfileNumInput *self)
{
    qmi_message_wds_set_default_profile_number_input_unref (self);
}

QmiDeprecatedMessageWdsSetDefaultProfileNumInput *
qmi_message_wds_set_default_profile_num_input_new (void)
{
    return qmi_message_wds_set_default_profile_number_input_new ();
}

GType
qmi_message_wds_set_default_profile_num_input_get_type (void)
{
    return qmi_message_wds_set_default_profile_number_input_get_type ();
}

gboolean
qmi_message_wds_set_default_profile_num_output_get_result (
    QmiDeprecatedMessageWdsSetDefaultProfileNumOutput *self,
    GError **error)
{
    return qmi_message_wds_set_default_profile_number_output_get_result (self, error);
}

gboolean
qmi_message_wds_set_default_profile_num_output_get_extended_error_code (
    QmiDeprecatedMessageWdsSetDefaultProfileNumOutput *self,
    QmiWdsDsProfileError *value_extended_error_code,
    GError **error)
{
    return qmi_message_wds_set_default_profile_number_output_get_extended_error_code (self,
                                                                                      value_extended_error_code,
                                                                                      error);
}

QmiDeprecatedMessageWdsSetDefaultProfileNumOutput *
qmi_message_wds_set_default_profile_num_output_ref (QmiDeprecatedMessageWdsSetDefaultProfileNumOutput *self)
{
    return qmi_message_wds_set_default_profile_number_output_ref (self);
}

void
qmi_message_wds_set_default_profile_num_output_unref (QmiDeprecatedMessageWdsSetDefaultProfileNumOutput *self)
{
    qmi_message_wds_set_default_profile_number_output_unref (self);
}

void
qmi_client_wds_set_default_profile_num (
    QmiClientWds *self,
    QmiMessageWdsSetDefaultProfileNumberInput *input,
    guint timeout,
    GCancellable *cancellable,
    GAsyncReadyCallback callback,
    gpointer user_data)
{
    qmi_client_wds_set_default_profile_number (
        self,
        input,
        timeout,
        cancellable,
        callback,
        user_data);
}

QmiMessageWdsSetDefaultProfileNumberOutput *
qmi_client_wds_set_default_profile_num_finish (
    QmiClientWds *self,
    GAsyncResult *res,
    GError **error)
{
    return qmi_client_wds_set_default_profile_number_finish (self, res, error);
}

#endif /* HAVE_QMI_MESSAGE_WDS_SET_DEFAULT_PROFILE_NUMBER */

#if defined HAVE_QMI_MESSAGE_NAS_GET_SYSTEM_INFO

gboolean
qmi_message_nas_get_system_info_output_get_gsm_system_info (
    QmiMessageNasGetSystemInfoOutput *self,
    gboolean *value_gsm_system_info_domain_valid,
    QmiNasNetworkServiceDomain *value_gsm_system_info_domain,
    gboolean *value_gsm_system_info_service_capability_valid,
    QmiNasNetworkServiceDomain *value_gsm_system_info_service_capability,
    gboolean *value_gsm_system_info_roaming_status_valid,
    QmiNasRoamingStatus *value_gsm_system_info_roaming_status,
    gboolean *value_gsm_system_info_forbidden_valid,
    gboolean *value_gsm_system_info_forbidden,
    gboolean *value_gsm_system_info_lac_valid,
    guint16 *value_gsm_system_info_lac,
    gboolean *value_gsm_system_info_cid_valid,
    guint32 *value_gsm_system_info_cid,
    gboolean *value_gsm_system_info_registration_reject_info_valid,
    QmiNasNetworkServiceDomain *value_gsm_system_info_registration_reject_domain,
    guint8 *value_gsm_system_info_registration_reject_cause,
    gboolean *value_gsm_system_info_network_id_valid,
    const gchar **value_gsm_system_info_mcc,
    const gchar **value_gsm_system_info_mnc,
    gboolean *value_gsm_system_info_egprs_support_valid,
    gboolean *value_gsm_system_info_egprs_support,
    gboolean *value_gsm_system_info_dtm_support_valid,
    gboolean *value_gsm_system_info_dtm_support,
    GError **error)
{
    QmiNasRejectCause reject_cause = QMI_NAS_REJECT_CAUSE_NONE;

    if (!qmi_message_nas_get_system_info_output_get_gsm_system_info_v2 (
            self,
            value_gsm_system_info_domain_valid,
            value_gsm_system_info_domain,
            value_gsm_system_info_service_capability_valid,
            value_gsm_system_info_service_capability,
            value_gsm_system_info_roaming_status_valid,
            value_gsm_system_info_roaming_status,
            value_gsm_system_info_forbidden_valid,
            value_gsm_system_info_forbidden,
            value_gsm_system_info_lac_valid,
            value_gsm_system_info_lac,
            value_gsm_system_info_cid_valid,
            value_gsm_system_info_cid,
            value_gsm_system_info_registration_reject_info_valid,
            value_gsm_system_info_registration_reject_domain,
            &reject_cause,
            value_gsm_system_info_network_id_valid,
            value_gsm_system_info_mcc,
            value_gsm_system_info_mnc,
            value_gsm_system_info_egprs_support_valid,
            value_gsm_system_info_egprs_support,
            value_gsm_system_info_dtm_support_valid,
            value_gsm_system_info_dtm_support,
            error))
        return FALSE;

    if (value_gsm_system_info_registration_reject_cause)
        *value_gsm_system_info_registration_reject_cause = (guint8) reject_cause;
    return TRUE;
}

gboolean
qmi_message_nas_get_system_info_output_get_wcdma_system_info (
    QmiMessageNasGetSystemInfoOutput *self,
    gboolean *value_wcdma_system_info_domain_valid,
    QmiNasNetworkServiceDomain *value_wcdma_system_info_domain,
    gboolean *value_wcdma_system_info_service_capability_valid,
    QmiNasNetworkServiceDomain *value_wcdma_system_info_service_capability,
    gboolean *value_wcdma_system_info_roaming_status_valid,
    QmiNasRoamingStatus *value_wcdma_system_info_roaming_status,
    gboolean *value_wcdma_system_info_forbidden_valid,
    gboolean *value_wcdma_system_info_forbidden,
    gboolean *value_wcdma_system_info_lac_valid,
    guint16 *value_wcdma_system_info_lac,
    gboolean *value_wcdma_system_info_cid_valid,
    guint32 *value_wcdma_system_info_cid,
    gboolean *value_wcdma_system_info_registration_reject_info_valid,
    QmiNasNetworkServiceDomain *value_wcdma_system_info_registration_reject_domain,
    guint8 *value_wcdma_system_info_registration_reject_cause,
    gboolean *value_wcdma_system_info_network_id_valid,
    const gchar **value_wcdma_system_info_mcc,
    const gchar **value_wcdma_system_info_mnc,
    gboolean *value_wcdma_system_info_hs_call_status_valid,
    QmiNasWcdmaHsService *value_wcdma_system_info_hs_call_status,
    gboolean *value_wcdma_system_info_hs_service_valid,
    QmiNasWcdmaHsService *value_wcdma_system_info_hs_service,
    gboolean *value_wcdma_system_info_primary_scrambling_code_valid,
    guint16 *value_wcdma_system_info_primary_scrambling_code,
    GError **error)
{
    QmiNasRejectCause reject_cause = QMI_NAS_REJECT_CAUSE_NONE;

    if (!qmi_message_nas_get_system_info_output_get_wcdma_system_info_v2 (
            self,
            value_wcdma_system_info_domain_valid,
            value_wcdma_system_info_domain,
            value_wcdma_system_info_service_capability_valid,
            value_wcdma_system_info_service_capability,
            value_wcdma_system_info_roaming_status_valid,
            value_wcdma_system_info_roaming_status,
            value_wcdma_system_info_forbidden_valid,
            value_wcdma_system_info_forbidden,
            value_wcdma_system_info_lac_valid,
            value_wcdma_system_info_lac,
            value_wcdma_system_info_cid_valid,
            value_wcdma_system_info_cid,
            value_wcdma_system_info_registration_reject_info_valid,
            value_wcdma_system_info_registration_reject_domain,
            &reject_cause,
            value_wcdma_system_info_network_id_valid,
            value_wcdma_system_info_mcc,
            value_wcdma_system_info_mnc,
            value_wcdma_system_info_hs_call_status_valid,
            value_wcdma_system_info_hs_call_status,
            value_wcdma_system_info_hs_service_valid,
            value_wcdma_system_info_hs_service,
            value_wcdma_system_info_primary_scrambling_code_valid,
            value_wcdma_system_info_primary_scrambling_code,
            error))
        return FALSE;

    if (value_wcdma_system_info_registration_reject_cause)
        *value_wcdma_system_info_registration_reject_cause = (guint8) reject_cause;
    return TRUE;
}

gboolean
qmi_message_nas_get_system_info_output_get_lte_system_info (
    QmiMessageNasGetSystemInfoOutput *self,
    gboolean *value_lte_system_info_domain_valid,
    QmiNasNetworkServiceDomain *value_lte_system_info_domain,
    gboolean *value_lte_system_info_service_capability_valid,
    QmiNasNetworkServiceDomain *value_lte_system_info_service_capability,
    gboolean *value_lte_system_info_roaming_status_valid,
    QmiNasRoamingStatus *value_lte_system_info_roaming_status,
    gboolean *value_lte_system_info_forbidden_valid,
    gboolean *value_lte_system_info_forbidden,
    gboolean *value_lte_system_info_lac_valid,
    guint16 *value_lte_system_info_lac,
    gboolean *value_lte_system_info_cid_valid,
    guint32 *value_lte_system_info_cid,
    gboolean *value_lte_system_info_registration_reject_info_valid,
    QmiNasNetworkServiceDomain *value_lte_system_info_registration_reject_domain,
    guint8 *value_lte_system_info_registration_reject_cause,
    gboolean *value_lte_system_info_network_id_valid,
    const gchar **value_lte_system_info_mcc,
    const gchar **value_lte_system_info_mnc,
    gboolean *value_lte_system_info_tac_valid,
    guint16 *value_lte_system_info_tac,
    GError **error)
{
    QmiNasRejectCause reject_cause = QMI_NAS_REJECT_CAUSE_NONE;

    if (!qmi_message_nas_get_system_info_output_get_lte_system_info_v2 (
            self,
            value_lte_system_info_domain_valid,
            value_lte_system_info_domain,
            value_lte_system_info_service_capability_valid,
            value_lte_system_info_service_capability,
            value_lte_system_info_roaming_status_valid,
            value_lte_system_info_roaming_status,
            value_lte_system_info_forbidden_valid,
            value_lte_system_info_forbidden,
            value_lte_system_info_lac_valid,
            value_lte_system_info_lac,
            value_lte_system_info_cid_valid,
            value_lte_system_info_cid,
            value_lte_system_info_registration_reject_info_valid,
            value_lte_system_info_registration_reject_domain,
            &reject_cause,
            value_lte_system_info_network_id_valid,
            value_lte_system_info_mcc,
            value_lte_system_info_mnc,
            value_lte_system_info_tac_valid,
            value_lte_system_info_tac,
            error))
        return FALSE;

    if (value_lte_system_info_registration_reject_cause)
        *value_lte_system_info_registration_reject_cause = (guint8) reject_cause;

    return TRUE;
}

gboolean
qmi_message_nas_get_system_info_output_get_td_scdma_system_info (
    QmiMessageNasGetSystemInfoOutput *self,
    gboolean *value_td_scdma_system_info_domain_valid,
    QmiNasNetworkServiceDomain *value_td_scdma_system_info_domain,
    gboolean *value_td_scdma_system_info_service_capability_valid,
    QmiNasNetworkServiceDomain *value_td_scdma_system_info_service_capability,
    gboolean *value_td_scdma_system_info_roaming_status_valid,
    QmiNasRoamingStatus *value_td_scdma_system_info_roaming_status,
    gboolean *value_td_scdma_system_info_forbidden_valid,
    gboolean *value_td_scdma_system_info_forbidden,
    gboolean *value_td_scdma_system_info_lac_valid,
    guint16 *value_td_scdma_system_info_lac,
    gboolean *value_td_scdma_system_info_cid_valid,
    guint32 *value_td_scdma_system_info_cid,
    gboolean *value_td_scdma_system_info_registration_reject_info_valid,
    QmiNasNetworkServiceDomain *value_td_scdma_system_info_registration_reject_domain,
    guint8 *value_td_scdma_system_info_registration_reject_cause,
    gboolean *value_td_scdma_system_info_network_id_valid,
    const gchar **value_td_scdma_system_info_mcc,
    const gchar **value_td_scdma_system_info_mnc,
    gboolean *value_td_scdma_system_info_hs_call_status_valid,
    QmiNasWcdmaHsService *value_td_scdma_system_info_hs_call_status,
    gboolean *value_td_scdma_system_info_hs_service_valid,
    QmiNasWcdmaHsService *value_td_scdma_system_info_hs_service,
    gboolean *value_td_scdma_system_info_cell_parameter_id_valid,
    guint16 *value_td_scdma_system_info_cell_parameter_id,
    gboolean *value_td_scdma_system_info_cell_broadcast_support_valid,
    QmiNasCellBroadcastCapability *value_td_scdma_system_info_cell_broadcast_support,
    gboolean *value_td_scdma_system_info_cs_call_barring_status_valid,
    QmiNasCallBarringStatus *value_td_scdma_system_info_cs_call_barring_status,
    gboolean *value_td_scdma_system_info_ps_call_barring_status_valid,
    QmiNasCallBarringStatus *value_td_scdma_system_info_ps_call_barring_status,
    gboolean *value_td_scdma_system_info_cipher_domain_valid,
    QmiNasNetworkServiceDomain *value_td_scdma_system_info_cipher_domain,
    GError **error)
{
    QmiNasRejectCause reject_cause = QMI_NAS_REJECT_CAUSE_NONE;

    if (!qmi_message_nas_get_system_info_output_get_td_scdma_system_info_v2 (
            self,
            value_td_scdma_system_info_domain_valid,
            value_td_scdma_system_info_domain,
            value_td_scdma_system_info_service_capability_valid,
            value_td_scdma_system_info_service_capability,
            value_td_scdma_system_info_roaming_status_valid,
            value_td_scdma_system_info_roaming_status,
            value_td_scdma_system_info_forbidden_valid,
            value_td_scdma_system_info_forbidden,
            value_td_scdma_system_info_lac_valid,
            value_td_scdma_system_info_lac,
            value_td_scdma_system_info_cid_valid,
            value_td_scdma_system_info_cid,
            value_td_scdma_system_info_registration_reject_info_valid,
            value_td_scdma_system_info_registration_reject_domain,
            &reject_cause,
            value_td_scdma_system_info_network_id_valid,
            value_td_scdma_system_info_mcc,
            value_td_scdma_system_info_mnc,
            value_td_scdma_system_info_hs_call_status_valid,
            value_td_scdma_system_info_hs_call_status,
            value_td_scdma_system_info_hs_service_valid,
            value_td_scdma_system_info_hs_service,
            value_td_scdma_system_info_cell_parameter_id_valid,
            value_td_scdma_system_info_cell_parameter_id,
            value_td_scdma_system_info_cell_broadcast_support_valid,
            value_td_scdma_system_info_cell_broadcast_support,
            value_td_scdma_system_info_cs_call_barring_status_valid,
            value_td_scdma_system_info_cs_call_barring_status,
            value_td_scdma_system_info_ps_call_barring_status_valid,
            value_td_scdma_system_info_ps_call_barring_status,
            value_td_scdma_system_info_cipher_domain_valid,
            value_td_scdma_system_info_cipher_domain,
            error))
        return FALSE;

    if (value_td_scdma_system_info_registration_reject_cause)
        *value_td_scdma_system_info_registration_reject_cause = (guint8) reject_cause;
    return TRUE;
}

#endif /* HAVE_QMI_MESSAGE_NAS_GET_SYSTEM_INFO */

#if defined HAVE_QMI_INDICATION_NAS_SYSTEM_INFO

gboolean qmi_indication_nas_system_info_output_get_gsm_system_info (
    QmiIndicationNasSystemInfoOutput *self,
    gboolean *value_gsm_system_info_domain_valid,
    QmiNasNetworkServiceDomain *value_gsm_system_info_domain,
    gboolean *value_gsm_system_info_service_capability_valid,
    QmiNasNetworkServiceDomain *value_gsm_system_info_service_capability,
    gboolean *value_gsm_system_info_roaming_status_valid,
    QmiNasRoamingStatus *value_gsm_system_info_roaming_status,
    gboolean *value_gsm_system_info_forbidden_valid,
    gboolean *value_gsm_system_info_forbidden,
    gboolean *value_gsm_system_info_lac_valid,
    guint16 *value_gsm_system_info_lac,
    gboolean *value_gsm_system_info_cid_valid,
    guint32 *value_gsm_system_info_cid,
    gboolean *value_gsm_system_info_registration_reject_info_valid,
    QmiNasNetworkServiceDomain *value_gsm_system_info_registration_reject_domain,
    guint8 *value_gsm_system_info_registration_reject_cause,
    gboolean *value_gsm_system_info_network_id_valid,
    const gchar **value_gsm_system_info_mcc,
    const gchar **value_gsm_system_info_mnc,
    gboolean *value_gsm_system_info_egprs_support_valid,
    gboolean *value_gsm_system_info_egprs_support,
    gboolean *value_gsm_system_info_dtm_support_valid,
    gboolean *value_gsm_system_info_dtm_support,
    GError **error)
{
    QmiNasRejectCause reject_cause = QMI_NAS_REJECT_CAUSE_NONE;

    if (!qmi_indication_nas_system_info_output_get_gsm_system_info_v2 (
            self,
            value_gsm_system_info_domain_valid,
            value_gsm_system_info_domain,
            value_gsm_system_info_service_capability_valid,
            value_gsm_system_info_service_capability,
            value_gsm_system_info_roaming_status_valid,
            value_gsm_system_info_roaming_status,
            value_gsm_system_info_forbidden_valid,
            value_gsm_system_info_forbidden,
            value_gsm_system_info_lac_valid,
            value_gsm_system_info_lac,
            value_gsm_system_info_cid_valid,
            value_gsm_system_info_cid,
            value_gsm_system_info_registration_reject_info_valid,
            value_gsm_system_info_registration_reject_domain,
            &reject_cause,
            value_gsm_system_info_network_id_valid,
            value_gsm_system_info_mcc,
            value_gsm_system_info_mnc,
            value_gsm_system_info_egprs_support_valid,
            value_gsm_system_info_egprs_support,
            value_gsm_system_info_dtm_support_valid,
            value_gsm_system_info_dtm_support,
            error))
        return FALSE;

    if (value_gsm_system_info_registration_reject_cause)
        *value_gsm_system_info_registration_reject_cause = (guint8) reject_cause;
    return TRUE;
}

gboolean qmi_indication_nas_system_info_output_get_wcdma_system_info (
    QmiIndicationNasSystemInfoOutput *self,
    gboolean *value_wcdma_system_info_domain_valid,
    QmiNasNetworkServiceDomain *value_wcdma_system_info_domain,
    gboolean *value_wcdma_system_info_service_capability_valid,
    QmiNasNetworkServiceDomain *value_wcdma_system_info_service_capability,
    gboolean *value_wcdma_system_info_roaming_status_valid,
    QmiNasRoamingStatus *value_wcdma_system_info_roaming_status,
    gboolean *value_wcdma_system_info_forbidden_valid,
    gboolean *value_wcdma_system_info_forbidden,
    gboolean *value_wcdma_system_info_lac_valid,
    guint16 *value_wcdma_system_info_lac,
    gboolean *value_wcdma_system_info_cid_valid,
    guint32 *value_wcdma_system_info_cid,
    gboolean *value_wcdma_system_info_registration_reject_info_valid,
    QmiNasNetworkServiceDomain *value_wcdma_system_info_registration_reject_domain,
    guint8 *value_wcdma_system_info_registration_reject_cause,
    gboolean *value_wcdma_system_info_network_id_valid,
    const gchar **value_wcdma_system_info_mcc,
    const gchar **value_wcdma_system_info_mnc,
    gboolean *value_wcdma_system_info_hs_call_status_valid,
    QmiNasWcdmaHsService *value_wcdma_system_info_hs_call_status,
    gboolean *value_wcdma_system_info_hs_service_valid,
    QmiNasWcdmaHsService *value_wcdma_system_info_hs_service,
    gboolean *value_wcdma_system_info_primary_scrambling_code_valid,
    guint16 *value_wcdma_system_info_primary_scrambling_code,
    GError **error)
{
    QmiNasRejectCause reject_cause = QMI_NAS_REJECT_CAUSE_NONE;

    if (!qmi_indication_nas_system_info_output_get_wcdma_system_info_v2 (
            self,
            value_wcdma_system_info_domain_valid,
            value_wcdma_system_info_domain,
            value_wcdma_system_info_service_capability_valid,
            value_wcdma_system_info_service_capability,
            value_wcdma_system_info_roaming_status_valid,
            value_wcdma_system_info_roaming_status,
            value_wcdma_system_info_forbidden_valid,
            value_wcdma_system_info_forbidden,
            value_wcdma_system_info_lac_valid,
            value_wcdma_system_info_lac,
            value_wcdma_system_info_cid_valid,
            value_wcdma_system_info_cid,
            value_wcdma_system_info_registration_reject_info_valid,
            value_wcdma_system_info_registration_reject_domain,
            &reject_cause,
            value_wcdma_system_info_network_id_valid,
            value_wcdma_system_info_mcc,
            value_wcdma_system_info_mnc,
            value_wcdma_system_info_hs_call_status_valid,
            value_wcdma_system_info_hs_call_status,
            value_wcdma_system_info_hs_service_valid,
            value_wcdma_system_info_hs_service,
            value_wcdma_system_info_primary_scrambling_code_valid,
            value_wcdma_system_info_primary_scrambling_code,
            error))
        return FALSE;

    if (value_wcdma_system_info_registration_reject_cause)
        *value_wcdma_system_info_registration_reject_cause = (guint8) reject_cause;
    return TRUE;
}

gboolean
qmi_indication_nas_system_info_output_get_lte_system_info (
    QmiIndicationNasSystemInfoOutput *self,
    gboolean *value_lte_system_info_domain_valid,
    QmiNasNetworkServiceDomain *value_lte_system_info_domain,
    gboolean *value_lte_system_info_service_capability_valid,
    QmiNasNetworkServiceDomain *value_lte_system_info_service_capability,
    gboolean *value_lte_system_info_roaming_status_valid,
    QmiNasRoamingStatus *value_lte_system_info_roaming_status,
    gboolean *value_lte_system_info_forbidden_valid,
    gboolean *value_lte_system_info_forbidden,
    gboolean *value_lte_system_info_lac_valid,
    guint16 *value_lte_system_info_lac,
    gboolean *value_lte_system_info_cid_valid,
    guint32 *value_lte_system_info_cid,
    gboolean *value_lte_system_info_registration_reject_info_valid,
    QmiNasNetworkServiceDomain *value_lte_system_info_registration_reject_domain,
    guint8 *value_lte_system_info_registration_reject_cause,
    gboolean *value_lte_system_info_network_id_valid,
    const gchar **value_lte_system_info_mcc,
    const gchar **value_lte_system_info_mnc,
    gboolean *value_lte_system_info_tac_valid,
    guint16 *value_lte_system_info_tac,
    GError **error)
{
    QmiNasRejectCause reject_cause = QMI_NAS_REJECT_CAUSE_NONE;

    if (!qmi_indication_nas_system_info_output_get_lte_system_info_v2 (
            self,
            value_lte_system_info_domain_valid,
            value_lte_system_info_domain,
            value_lte_system_info_service_capability_valid,
            value_lte_system_info_service_capability,
            value_lte_system_info_roaming_status_valid,
            value_lte_system_info_roaming_status,
            value_lte_system_info_forbidden_valid,
            value_lte_system_info_forbidden,
            value_lte_system_info_lac_valid,
            value_lte_system_info_lac,
            value_lte_system_info_cid_valid,
            value_lte_system_info_cid,
            value_lte_system_info_registration_reject_info_valid,
            value_lte_system_info_registration_reject_domain,
            &reject_cause,
            value_lte_system_info_network_id_valid,
            value_lte_system_info_mcc,
            value_lte_system_info_mnc,
            value_lte_system_info_tac_valid,
            value_lte_system_info_tac,
            error))
        return FALSE;

    if (value_lte_system_info_registration_reject_cause)
        *value_lte_system_info_registration_reject_cause = (guint8) reject_cause;
    return TRUE;
}

gboolean
qmi_indication_nas_system_info_output_get_td_scma_system_info (
    QmiIndicationNasSystemInfoOutput *self,
    gboolean *value_td_scma_system_info_domain_valid,
    QmiNasNetworkServiceDomain *value_td_scma_system_info_domain,
    gboolean *value_td_scma_system_info_service_capability_valid,
    QmiNasNetworkServiceDomain *value_td_scma_system_info_service_capability,
    gboolean *value_td_scma_system_info_roaming_status_valid,
    QmiNasRoamingStatus *value_td_scma_system_info_roaming_status,
    gboolean *value_td_scma_system_info_forbidden_valid,
    gboolean *value_td_scma_system_info_forbidden,
    gboolean *value_td_scma_system_info_lac_valid,
    guint16 *value_td_scma_system_info_lac,
    gboolean *value_td_scma_system_info_cid_valid,
    guint32 *value_td_scma_system_info_cid,
    gboolean *value_td_scma_system_info_registration_reject_info_valid,
    QmiNasNetworkServiceDomain *value_td_scma_system_info_registration_reject_domain,
    guint8 *value_td_scma_system_info_registration_reject_cause,
    gboolean *value_td_scma_system_info_network_id_valid,
    const gchar **value_td_scma_system_info_mcc,
    const gchar **value_td_scma_system_info_mnc,
    gboolean *value_td_scma_system_info_hs_call_status_valid,
    QmiNasWcdmaHsService *value_td_scma_system_info_hs_call_status,
    gboolean *value_td_scma_system_info_hs_service_valid,
    QmiNasWcdmaHsService *value_td_scma_system_info_hs_service,
    gboolean *value_td_scma_system_info_cell_parameter_id_valid,
    guint16 *value_td_scma_system_info_cell_parameter_id,
    gboolean *value_td_scma_system_info_cell_broadcast_support_valid,
    QmiNasCellBroadcastCapability *value_td_scma_system_info_cell_broadcast_support,
    gboolean *value_td_scma_system_info_cs_call_barring_status_valid,
    QmiNasCallBarringStatus *value_td_scma_system_info_cs_call_barring_status,
    gboolean *value_td_scma_system_info_ps_call_barring_status_valid,
    QmiNasCallBarringStatus *value_td_scma_system_info_ps_call_barring_status,
    gboolean *value_td_scma_system_info_cipher_domain_valid,
    QmiNasNetworkServiceDomain *value_td_scma_system_info_cipher_domain,
    GError **error)
{
    QmiNasRejectCause reject_cause = QMI_NAS_REJECT_CAUSE_NONE;

    if (!qmi_indication_nas_system_info_output_get_td_scma_system_info_v2 (
            self,
            value_td_scma_system_info_domain_valid,
            value_td_scma_system_info_domain,
            value_td_scma_system_info_service_capability_valid,
            value_td_scma_system_info_service_capability,
            value_td_scma_system_info_roaming_status_valid,
            value_td_scma_system_info_roaming_status,
            value_td_scma_system_info_forbidden_valid,
            value_td_scma_system_info_forbidden,
            value_td_scma_system_info_lac_valid,
            value_td_scma_system_info_lac,
            value_td_scma_system_info_cid_valid,
            value_td_scma_system_info_cid,
            value_td_scma_system_info_registration_reject_info_valid,
            value_td_scma_system_info_registration_reject_domain,
            &reject_cause,
            value_td_scma_system_info_network_id_valid,
            value_td_scma_system_info_mcc,
            value_td_scma_system_info_mnc,
            value_td_scma_system_info_hs_call_status_valid,
            value_td_scma_system_info_hs_call_status,
            value_td_scma_system_info_hs_service_valid,
            value_td_scma_system_info_hs_service,
            value_td_scma_system_info_cell_parameter_id_valid,
            value_td_scma_system_info_cell_parameter_id,
            value_td_scma_system_info_cell_broadcast_support_valid,
            value_td_scma_system_info_cell_broadcast_support,
            value_td_scma_system_info_cs_call_barring_status_valid,
            value_td_scma_system_info_cs_call_barring_status,
            value_td_scma_system_info_ps_call_barring_status_valid,
            value_td_scma_system_info_ps_call_barring_status,
            value_td_scma_system_info_cipher_domain_valid,
            value_td_scma_system_info_cipher_domain,
            error))
        return FALSE;

    if (value_td_scma_system_info_registration_reject_cause)
        *value_td_scma_system_info_registration_reject_cause = (guint8) reject_cause;
    return TRUE;
}

#endif /* HAVE_QMI_INDICATION_NAS_SYSTEM_INFO */

#if defined HAVE_QMI_MESSAGE_NAS_SWI_GET_STATUS

gboolean
qmi_message_nas_swi_get_status_output_get_common_info (
    QmiMessageNasSwiGetStatusOutput *self,
    guint8 *value_common_info_temperature,
    QmiNasSwiModemMode *value_common_info_modem_mode,
    QmiNasSwiSystemMode *value_common_info_system_mode,
    QmiNasSwiImsRegState *value_common_info_ims_registration_state,
    QmiNasSwiPsState *value_common_info_packet_service_state,
    GError **error)
{
    gint8 signed_temperature;

    if (!qmi_message_nas_swi_get_status_output_get_common_info_v2 (
            self,
            &signed_temperature,
            value_common_info_modem_mode,
            value_common_info_system_mode,
            value_common_info_ims_registration_state,
            value_common_info_packet_service_state,
            error))
      return FALSE;

    if (value_common_info_temperature)
      memcpy (value_common_info_temperature, &signed_temperature, sizeof (guint8));

    return TRUE;
}

#endif /* HAVE_QMI_MESSAGE_NAS_SWI_GET_STATUS */

#if defined HAVE_QMI_MESSAGE_NAS_SET_SYSTEM_SELECTION_PREFERENCE

gboolean
qmi_message_nas_set_system_selection_preference_input_get_mnc_pds_digit_include_status (
    QmiMessageNasSetSystemSelectionPreferenceInput *self,
    gboolean *value_mnc_pds_digit_include_status,
    GError **error)
{
    return qmi_message_nas_set_system_selection_preference_input_get_mnc_pcs_digit_include_status (self, value_mnc_pds_digit_include_status, error);
}

#endif /* HAVE_QMI_MESSAGE_NAS_SET_SYSTEM_SELECTION_PREFERENCE */

#if defined HAVE_QMI_MESSAGE_PDC_CONFIG_CHANGE

gboolean
qmi_message_pdc_config_change_input_get_type_with_id (
    QmiMessagePdcConfigChangeInput *self,
    QmiDeprecatedConfigTypeAndId *value_type_with_id,
    GError **error)
{
    QmiPdcConfigurationType config_type = QMI_PDC_CONFIGURATION_TYPE_PLATFORM;
    GArray *id = NULL;

    if (!qmi_message_pdc_config_change_input_get_type_with_id_v2 (self, &config_type, &id, error))
        return FALSE;
    if (value_type_with_id) {
        value_type_with_id->config_type = config_type;
        value_type_with_id->id = id;
    }
    return TRUE;
}

gboolean
qmi_message_pdc_config_change_input_set_type_with_id (
    QmiMessagePdcConfigChangeInput *self,
    const QmiDeprecatedConfigTypeAndId *value_type_with_id,
    GError **error)
{
    return qmi_message_pdc_config_change_input_set_type_with_id_v2 (self, value_type_with_id->config_type, value_type_with_id->id, error);
}

gboolean
qmi_message_pdc_config_change_output_get_type_with_id (
    QmiMessagePdcConfigChangeOutput *self,
    QmiDeprecatedConfigTypeAndId *value_type_with_id,
    GError **error)
{
    QmiPdcConfigurationType config_type = QMI_PDC_CONFIGURATION_TYPE_PLATFORM;
    GArray *id = NULL;

    if (!qmi_message_pdc_config_change_output_get_type_with_id_v2 (self, &config_type, &id, error))
        return FALSE;
    if (value_type_with_id) {
        value_type_with_id->config_type = config_type;
        value_type_with_id->id = id;
    }
    return TRUE;
}

#endif /* HAVE_QMI_MESSAGE_PDC_CONFIG_CHANGE */

#if defined HAVE_QMI_MESSAGE_PDC_SET_SELECTED_CONFIG

gboolean
qmi_message_pdc_set_selected_config_input_get_type_with_id (
    QmiMessagePdcSetSelectedConfigInput *self,
    QmiDeprecatedConfigTypeAndId *value_type_with_id,
    GError **error)
{
    QmiPdcConfigurationType config_type = QMI_PDC_CONFIGURATION_TYPE_PLATFORM;
    GArray *id = NULL;

    if (!qmi_message_pdc_set_selected_config_input_get_type_with_id_v2 (self, &config_type, &id, error))
        return FALSE;
    if (value_type_with_id) {
        value_type_with_id->config_type = config_type;
        value_type_with_id->id = id;
    }
    return TRUE;
}

gboolean
qmi_message_pdc_set_selected_config_input_set_type_with_id (
    QmiMessagePdcSetSelectedConfigInput *self,
    const QmiDeprecatedConfigTypeAndId *value_type_with_id,
    GError **error)
{
    return qmi_message_pdc_set_selected_config_input_set_type_with_id_v2 (self, value_type_with_id->config_type, value_type_with_id->id, error);
}

#endif /* HAVE_QMI_MESSAGE_PDC_SET_SELECTED_CONFIG */

#if defined HAVE_QMI_MESSAGE_PDC_GET_CONFIG_INFO

gboolean
qmi_message_pdc_get_config_info_input_get_type_with_id (
    QmiMessagePdcGetConfigInfoInput *self,
    QmiDeprecatedConfigTypeAndId *value_type_with_id,
    GError **error)
{
    QmiPdcConfigurationType config_type = QMI_PDC_CONFIGURATION_TYPE_PLATFORM;
    GArray *id = NULL;

    if (!qmi_message_pdc_get_config_info_input_get_type_with_id_v2 (self, &config_type, &id, error))
        return FALSE;
    if (value_type_with_id) {
        value_type_with_id->config_type = config_type;
        value_type_with_id->id = id;
    }
    return TRUE;
}

gboolean
qmi_message_pdc_get_config_info_input_set_type_with_id (
    QmiMessagePdcGetConfigInfoInput *self,
    const QmiDeprecatedConfigTypeAndId *value_type_with_id,
    GError **error)
{
    return qmi_message_pdc_get_config_info_input_set_type_with_id_v2 (self, value_type_with_id->config_type, value_type_with_id->id, error);
}

#endif /* HAVE_QMI_MESSAGE_PDC_GET_CONFIG_INFO */

#if defined HAVE_QMI_MESSAGE_DMS_DELETE_STORED_IMAGE

gboolean
qmi_message_dms_delete_stored_image_input_get_image (
    QmiMessageDmsDeleteStoredImageInput *self,
    QmiDeprecatedMessageDmsDeleteStoredImageInputImage *value_image,
    GError **error)
{
    QmiDmsFirmwareImageType  type = QMI_DMS_FIRMWARE_IMAGE_TYPE_MODEM;
    GArray                  *unique_id = NULL;
    const gchar             *build_id = NULL;

    if (!qmi_message_dms_delete_stored_image_input_get_image_details (self, &type, &unique_id, &build_id, error))
        return FALSE;
    if (value_image) {
        value_image->type = type;
        value_image->unique_id = unique_id;
        value_image->build_id = (gchar *) build_id;
    }
    return TRUE;
}

gboolean
qmi_message_dms_delete_stored_image_input_set_image (
    QmiMessageDmsDeleteStoredImageInput *self,
    const QmiDeprecatedMessageDmsDeleteStoredImageInputImage *value_image,
    GError **error)
{
    return qmi_message_dms_delete_stored_image_input_set_image_details (self, value_image->type, value_image->unique_id, value_image->build_id, error);
}

#endif /* HAVE_QMI_MESSAGE_DMS_DELETE_STORED_IMAGE */

#if defined HAVE_QMI_MESSAGE_DMS_GET_STORED_IMAGE_INFO

gboolean
qmi_message_dms_get_stored_image_info_input_get_image (
    QmiMessageDmsGetStoredImageInfoInput *self,
    QmiDeprecatedMessageDmsGetStoredImageInfoInputImage *value_image,
    GError **error)
{
    QmiDmsFirmwareImageType  type = QMI_DMS_FIRMWARE_IMAGE_TYPE_MODEM;
    GArray                  *unique_id = NULL;
    const gchar             *build_id = NULL;

    if (!qmi_message_dms_get_stored_image_info_input_get_image_details (self, &type, &unique_id, &build_id, error))
        return FALSE;
    if (value_image) {
        value_image->type = type;
        value_image->unique_id = unique_id;
        value_image->build_id = (gchar *) build_id;
    }
    return TRUE;
}

gboolean
qmi_message_dms_get_stored_image_info_input_set_image (
    QmiMessageDmsGetStoredImageInfoInput *self,
    const QmiDeprecatedMessageDmsGetStoredImageInfoInputImage *value_image,
    GError **error)
{
    return qmi_message_dms_get_stored_image_info_input_set_image_details (self, value_image->type, value_image->unique_id, value_image->build_id, error);
}

#endif /* HAVE_QMI_MESSAGE_DMS_GET_STORED_IMAGE_INFO */

#if defined  HAVE_QMI_INDICATION_LOC_POSITION_REPORT

gboolean
qmi_indication_loc_position_report_output_get_dilution_of_precision (
    QmiIndicationLocPositionReportOutput *self,
    QmiDeprecatedIndicationLocPositionReportOutputDilutionOfPrecision *value_dilution_of_precision,
    GError **error)
{
    gfloat pdop = 0.0;
    gfloat hdop = 0.0;
    gfloat vdop = 0.0;

    if (!qmi_indication_loc_position_report_output_get_dop (self, &pdop, &hdop, &vdop, error))
        return FALSE;
    if (value_dilution_of_precision) {
        value_dilution_of_precision->position_dilution_of_precision = pdop;
        value_dilution_of_precision->horizontal_dilution_of_precision = hdop;
        value_dilution_of_precision->vertical_dilution_of_precision = vdop;
    }
    return TRUE;
}

gboolean
qmi_indication_loc_position_report_output_get_gps_time (
    QmiIndicationLocPositionReportOutput *self,
    QmiDeprecatedIndicationLocPositionReportOutputGpsTime *value_gps_time,
    GError **error)
{
    guint16 gps_weeks = 0;
    guint32 gps_time_of_week_milliseconds = 0;

    if (!qmi_indication_loc_position_report_output_get_gps_date_time (self, &gps_weeks, &gps_time_of_week_milliseconds, error))
        return FALSE;
    if (value_gps_time) {
        value_gps_time->gps_weeks = gps_weeks;
        value_gps_time->gps_time_of_week_milliseconds = gps_time_of_week_milliseconds;
    }
    return TRUE;
}

#endif /* HAVE_QMI_INDICATION_LOC_POSITION_REPORT */

#if defined HAVE_QMI_MESSAGE_UIM_GET_SLOT_STATUS

typedef struct {
    GArray *slot_eid_information;
} MessageUimGetSlotStatusOutputCompatContext;

static void
message_uim_get_slot_status_output_clear_slot_eid_information (GArray *array)
{
    guint i;

    for (i = 0; i < array->len; i++)
        g_array_unref (g_array_index (array, GArray *, i));
    g_array_unref (array);
}

static void
message_uim_get_slot_status_output_compat_context_free (MessageUimGetSlotStatusOutputCompatContext *ctx)
{
    if (ctx->slot_eid_information)
        message_uim_get_slot_status_output_clear_slot_eid_information (ctx->slot_eid_information);
    g_slice_free (MessageUimGetSlotStatusOutputCompatContext, ctx);
}

static MessageUimGetSlotStatusOutputCompatContext *
message_uim_get_slot_status_output_get_compat_context (QmiMessageUimGetSlotStatusOutput *self)
{
    MessageUimGetSlotStatusOutputCompatContext *ctx;

    ctx = qmi_message_uim_get_slot_status_output_get_compat_context (self);
    if (!ctx) {
        ctx = g_slice_new0 (MessageUimGetSlotStatusOutputCompatContext);
        qmi_message_uim_get_slot_status_output_set_compat_context (self, ctx, (GDestroyNotify)message_uim_get_slot_status_output_compat_context_free);
    }

    return ctx;
}

gboolean
qmi_message_uim_get_slot_status_output_get_slot_eid_information (
    QmiMessageUimGetSlotStatusOutput *self,
    GArray **value_slot_eid_information,
    GError **error)
{
    GArray *slot_eid = NULL;

    if (!qmi_message_uim_get_slot_status_output_get_slot_eid (self, &slot_eid, error))
        return FALSE;

    if (value_slot_eid_information) {
        MessageUimGetSlotStatusOutputCompatContext *ctx;
        guint                                       i;

        ctx = message_uim_get_slot_status_output_get_compat_context (self);

        if (ctx->slot_eid_information)
            message_uim_get_slot_status_output_clear_slot_eid_information (ctx->slot_eid_information);
        ctx->slot_eid_information = g_array_sized_new (FALSE, FALSE, sizeof (GArray *), slot_eid->len);

        for (i = 0; i < slot_eid->len; i++) {
            QmiSlotEidElement *element;
            GArray            *aux;

            element = &g_array_index (slot_eid, QmiSlotEidElement, i);
            aux = g_array_ref (element->eid);
            g_array_append_val (ctx->slot_eid_information, aux);
        }

        *value_slot_eid_information = ctx->slot_eid_information;
    }
    return TRUE;
}

#endif /* HAVE_QMI_MESSAGE_UIM_GET_SLOT_STATUS */

#if defined HAVE_QMI_INDICATION_UIM_SLOT_STATUS

typedef struct {
    GArray *slot_eid_information;
} IndicationUimSlotStatusOutputCompatContext;

static void
indication_uim_slot_status_output_clear_slot_eid_information (GArray *array)
{
    guint i;

    for (i = 0; i < array->len; i++)
        g_array_unref (g_array_index (array, GArray *, i));
    g_array_unref (array);
}

static void
indication_uim_slot_status_output_compat_context_free (IndicationUimSlotStatusOutputCompatContext *ctx)
{
    if (ctx->slot_eid_information)
        indication_uim_slot_status_output_clear_slot_eid_information (ctx->slot_eid_information);
    g_slice_free (IndicationUimSlotStatusOutputCompatContext, ctx);
}

static IndicationUimSlotStatusOutputCompatContext *
indication_uim_slot_status_output_get_compat_context (QmiIndicationUimSlotStatusOutput *self)
{
    IndicationUimSlotStatusOutputCompatContext *ctx;

    ctx = qmi_indication_uim_slot_status_output_get_compat_context (self);
    if (!ctx) {
        ctx = g_slice_new0 (IndicationUimSlotStatusOutputCompatContext);
        qmi_indication_uim_slot_status_output_set_compat_context (self, ctx, (GDestroyNotify)indication_uim_slot_status_output_compat_context_free);
    }

    return ctx;
}

gboolean
qmi_indication_uim_slot_status_output_get_slot_eid_information (
    QmiIndicationUimSlotStatusOutput *self,
    GArray **value_slot_eid_information,
    GError **error)
{
    GArray *slot_eid = NULL;

    if (!qmi_indication_uim_slot_status_output_get_slot_eid (self, &slot_eid, error))
        return FALSE;

    if (value_slot_eid_information) {
        IndicationUimSlotStatusOutputCompatContext *ctx;
        guint                                       i;

        ctx = indication_uim_slot_status_output_get_compat_context (self);

        if (ctx->slot_eid_information)
            indication_uim_slot_status_output_clear_slot_eid_information (ctx->slot_eid_information);

        ctx->slot_eid_information = g_array_sized_new (FALSE, FALSE, sizeof (GArray *), slot_eid->len);
        for (i = 0; i < slot_eid->len; i++) {
            QmiSlotEidElement *element;
            GArray            *aux;

            element = &g_array_index (slot_eid, QmiSlotEidElement, i);
            aux = g_array_ref (element->eid);
            g_array_append_val (ctx->slot_eid_information, aux);
        }

        *value_slot_eid_information = ctx->slot_eid_information;
    }
    return TRUE;
}

#endif /* HAVE_QMI_INDICATION_UIM_SLOT_STATUS */

#if defined HAVE_QMI_MESSAGE_UIM_GET_CONFIGURATION

typedef struct {
    GArray *personalization_status_other_slots;
} MessageUimGetConfigurationOutputCompatContext;

static void
message_uim_get_configuration_output_clear_personalization_status_other_slots (GArray *array)
{
    guint i;

    for (i = 0; i < array->len; i++)
        g_array_unref (g_array_index (array, GArray *, i));
    g_array_unref (array);
}

static void
message_uim_get_configuration_output_compat_context_free (MessageUimGetConfigurationOutputCompatContext *ctx)
{
    if (ctx->personalization_status_other_slots)
        message_uim_get_configuration_output_clear_personalization_status_other_slots (ctx->personalization_status_other_slots);
    g_slice_free (MessageUimGetConfigurationOutputCompatContext, ctx);
}

static MessageUimGetConfigurationOutputCompatContext *
message_uim_get_configuration_output_get_compat_context (QmiMessageUimGetConfigurationOutput *self)
{
    MessageUimGetConfigurationOutputCompatContext *ctx;

    ctx = qmi_message_uim_get_configuration_output_get_compat_context (self);
    if (!ctx) {
        ctx = g_slice_new0 (MessageUimGetConfigurationOutputCompatContext);
        qmi_message_uim_get_configuration_output_set_compat_context (self, ctx, (GDestroyNotify)message_uim_get_configuration_output_compat_context_free);
    }

    return ctx;
}

gboolean
qmi_message_uim_get_configuration_output_get_personalization_status_other_slots (
    QmiMessageUimGetConfigurationOutput *self,
    GArray **value_personalization_status_other_slots,
    GError **error)
{
    GArray *array = NULL;

    if (!qmi_message_uim_get_configuration_output_get_personalization_status_other (self, &array, error))
        return FALSE;

    if (value_personalization_status_other_slots) {
        MessageUimGetConfigurationOutputCompatContext *ctx;
        guint                                          i;

        ctx = message_uim_get_configuration_output_get_compat_context (self);

        if (ctx->personalization_status_other_slots)
            message_uim_get_configuration_output_clear_personalization_status_other_slots (ctx->personalization_status_other_slots);

        ctx->personalization_status_other_slots = g_array_sized_new (FALSE, FALSE, sizeof (GArray *), array->len);


        for (i = 0; i < array->len; i++) {
            QmiMessageUimGetConfigurationOutputPersonalizationStatusOtherElement *element;
            GArray                                                               *aux;

            element = &g_array_index (array, QmiMessageUimGetConfigurationOutputPersonalizationStatusOtherElement, i);

            /* We can do this because QmiMessageUimGetConfigurationOutputPersonalizationStatusOtherElementSlotElement (the new type)
             * and QmiMessageUimGetConfigurationOutputPersonalizationStatusOtherSlotsSlotsElement (the old type) are equivalent */
            aux = g_array_ref (element->slot);
            g_array_append_val (ctx->personalization_status_other_slots, aux);
        }

        *value_personalization_status_other_slots = ctx->personalization_status_other_slots;
    }
    return TRUE;
}

#endif /* HAVE_QMI_MESSAGE_UIM_GET_CONFIGURATION */

#if defined HAVE_QMI_INDICATION_WDS_SET_LTE_ATTACH_PDN_LIST

gboolean
qmi_indication_wds_set_lte_attach_pdn_list_output_get_result (
    QmiIndicationWdsSetLteAttachPdnListOutput *self,
    GError **error)
{
    /* NO-OP */
    return TRUE;
}

#endif /* HAVE_QMI_INDICATION_WDS_SET_LTE_ATTACH_PDN_LIST */

/*****************************************************************************/

GType
qmi_wds_get_current_settings_requested_settings_get_type (void)
{
    return qmi_wds_requested_settings_get_type ();
}

gchar *
qmi_wds_get_current_settings_requested_settings_build_string_from_mask (QmiDeprecatedWdsGetCurrentSettingsRequestedSettings mask)
{
    return qmi_wds_requested_settings_build_string_from_mask ((QmiWdsRequestedSettings)mask);
}

/*****************************************************************************/

#if defined HAVE_QMI_MESSAGE_UIM_GET_CARD_STATUS

GType
qmi_message_uim_get_card_status_output_card_status_cards_element_applications_element_get_type (void)
{
    return qmi_message_uim_get_card_status_output_card_status_cards_element_applications_element_v2_get_type ();
}

#endif /* HAVE_QMI_MESSAGE_UIM_GET_CARD_STATUS */

/*****************************************************************************/

#if defined HAVE_QMI_INDICATION_UIM_CARD_STATUS

GType
qmi_indication_uim_card_status_output_card_status_cards_element_applications_element_get_type (void)
{
  return qmi_indication_uim_card_status_output_card_status_cards_element_applications_element_v2_get_type ();
}

#endif /* HAVE_QMI_INDICATION_UIM_CARD_STATUS */

#endif /* QMI_DISABLE_DEPRECATED */
