
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
 * Copyright (C) 2016 Aleksander Morgado <aleksander@aleksander.es>
 */

#include "qmi-compat.h"

/**
 * qmi_message_dms_set_service_programming_code_input_get_new:
 * @self: a #QmiMessageDmsSetServiceProgrammingCodeInput.
 * @arg_new: a placeholder for the output constant string, or %NULL if not required.
 * @error: Return location for error or %NULL.
 *
 * Get the 'New Code' field from @self.
 *
 * Returns: %TRUE if the field is found, %FALSE otherwise.
 *
 * Deprecated: 1.14.0: Use qmi_message_dms_set_service_programming_code_input_get_new_code() instead.
 */
gboolean
qmi_message_dms_set_service_programming_code_input_get_new (
    QmiMessageDmsSetServiceProgrammingCodeInput *self,
    const gchar **arg_new,
    GError **error)
{
    return qmi_message_dms_set_service_programming_code_input_get_new_code (self, arg_new, error);
}

/**
 * qmi_message_dms_set_service_programming_code_input_set_new:
 * @self: a #QmiMessageDmsSetServiceProgrammingCodeInput.
 * @arg_new: a constant string of exactly 6 characters.
 * @error: Return location for error or %NULL.
 *
 * Set the 'New Code' field in the message.
 *
 * Returns: %TRUE if @value was successfully set, %FALSE otherwise.
 *
 * Deprecated: 1.14.0: Use qmi_message_dms_set_service_programming_code_input_set_new_code() instead.
 */
gboolean
qmi_message_dms_set_service_programming_code_input_set_new (
    QmiMessageDmsSetServiceProgrammingCodeInput *self,
    const gchar *arg_new,
    GError **error)
{
    return qmi_message_dms_set_service_programming_code_input_set_new_code (self, arg_new, error);
}

/**
 * qmi_message_dms_set_service_programming_code_input_get_current:
 * @self: a #QmiMessageDmsSetServiceProgrammingCodeInput.
 * @arg_current: a placeholder for the output constant string, or %NULL if not required.
 * @error: Return location for error or %NULL.
 *
 * Get the 'Current Code' field from @self.
 *
 * Returns: %TRUE if the field is found, %FALSE otherwise.
 *
 * Deprecated: 1.14.0: Use qmi_message_dms_set_service_programming_code_input_get_current_code() instead.
 */
gboolean
qmi_message_dms_set_service_programming_code_input_get_current (
    QmiMessageDmsSetServiceProgrammingCodeInput *self,
    const gchar **arg_current,
    GError **error)
{
  return qmi_message_dms_set_service_programming_code_input_get_current_code (self, arg_current, error);
}

/**
 * qmi_message_dms_set_service_programming_code_input_set_current:
 * @self: a #QmiMessageDmsSetServiceProgrammingCodeInput.
 * @arg_current: a placeholder for the output constant string, or %NULL if not required.
 * @error: Return location for error or %NULL.
 *
 * Get the 'Current Code' field from @self.
 *
 * Returns: %TRUE if the field is found, %FALSE otherwise.
 *
 * Deprecated: 1.14.0: Use qmi_message_dms_set_service_programming_code_input_set_current_code() instead.
 */
gboolean
qmi_message_dms_set_service_programming_code_input_set_current (
    QmiMessageDmsSetServiceProgrammingCodeInput *self,
    const gchar *arg_current,
    GError **error)
{
  return qmi_message_dms_set_service_programming_code_input_set_current_code (self, arg_current, error);
}
