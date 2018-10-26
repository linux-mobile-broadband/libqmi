
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

#include "qmi-compat.h"
#include "qmi-utils.h"

#ifndef QMI_DISABLE_DEPRECATED

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

#endif /* QMI_DISABLE_DEPRECATED */
