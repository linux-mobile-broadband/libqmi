#!/usr/bin/env python
# -*- Mode: python; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU Lesser General Public License as published by the Free
# Software Foundation; either version 2 of the License, or (at your option) any
# later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
# details.
#
# You should have received a copy of the GNU Lesser General Public License along
# with this program; if not, write to the Free Software Foundation, Inc., 51
# Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#
# Copyright (C) 2012 Lanedo GmbH
#

import string

import utils
from Struct import Struct
from FieldStruct  import FieldStruct

class FieldStructResult(FieldStruct):
    """
    The FieldResult class takes care of handling the common 'Result' TLV
    """

    def emit_types(self, hfile, cfile):
        # Emit both packed/unpacked types to the SOURCE file (not public)
        self.contents.emit_packed(cfile)
        self.contents.emit(cfile)

    def emit_getter(self, hfile, cfile):
        translations = { 'variable_name'     : self.variable_name,
                         'prefix_camelcase'  : utils.build_camelcase_name(self.prefix),
                         'prefix_underscore' : utils.build_underscore_name(self.prefix) }

        # Emit the getter header
        template = (
            '\n'
            'gboolean ${prefix_underscore}_get_result (\n'
            '    ${prefix_camelcase} *self,\n'
            '    GError **error);\n')
        hfile.write(string.Template(template).substitute(translations))

        # Emit the getter source
        template = (
            '\n'
            '/**\n'
            ' * ${prefix_underscore}_get_result:\n'
            ' * @self: a ${prefix_camelcase}.\n'
            ' * @error: a #GError.\n'
            ' *\n'
            ' * Get the result of the QMI operation.\n'
            ' *\n'
            ' * Returns: #TRUE if the QMI operation succeeded, #FALSE if @error is set.\n'
            ' */\n'
            'gboolean\n'
            '${prefix_underscore}_get_result (\n'
            '    ${prefix_camelcase} *self,\n'
            '    GError **error)\n'
            '{\n'
            '    g_return_val_if_fail (self != NULL, FALSE);\n'
            '\n'
            '    /* We should always have a result set in the response message */\n'
            '    if (!self->${variable_name}_set) {\n'
            '        g_set_error (error,\n'
            '                     QMI_CORE_ERROR,\n'
            '                     QMI_CORE_ERROR_INVALID_MESSAGE,\n'
            '                     "No \'Result\' field given in the message");\n'
            '        return FALSE;\n'
            '    }\n'
            '\n'
            '    if (self->${variable_name}.error_status == QMI_STATUS_SUCCESS) {\n'
            '        /* Operation succeeded */\n'
            '        return TRUE;\n'
            '    }\n'
            '\n'
            '    /* Report a QMI protocol error */\n'
            '    g_set_error (error,\n'
            '                 QMI_PROTOCOL_ERROR,\n'
            '                 (QmiProtocolError) self->${variable_name}.error_code,\n'
            '                 "QMI protocol error (%u): \'%s\'",\n'
            '                 self->${variable_name}.error_code,\n'
            '                 qmi_protocol_error_get_string ((QmiProtocolError) self->${variable_name}.error_code));\n'
            '    return FALSE;\n'
            '}\n')
        cfile.write(string.Template(template).substitute(translations))

    def emit_output_tlv_get_printable(self, f):
        translations = { 'name'                 : self.name,
                         'underscore'           : utils.build_underscore_name (self.fullname),
                         'container_underscore' : utils.build_underscore_name (self.prefix),
                         'field_type'           : self.field_type,
                         'tlv_id'               : self.id_enum_name,
                         'variable_name'        : self.variable_name }

        template = (
            '\n'
            'static gchar *\n'
            '${underscore}_get_printable (\n'
            '    QmiMessage *self)\n'
            '{\n'
            '    GString *printable;\n'
            '    ${field_type}Packed tmp;\n'
            '\n'
            '    printable = g_string_new ("");\n'
            '    g_assert (qmi_message_tlv_get (self,\n'
            '                                   ${tlv_id},\n'
            '                                   sizeof (tmp),\n'
            '                                   &tmp,\n'
            '                                   NULL));\n')
        f.write(string.Template(template).substitute(translations))

        for struct_field in self.contents.members:
            translations['name_struct_field'] = struct_field['name']
            translations['underscore_struct_field'] = utils.build_underscore_name(struct_field['name'])
            translations['endianfix'] = utils.he_from_le ('tmp.' + utils.build_underscore_name(struct_field['name']),
                                                          'tmp.' + utils.build_underscore_name(struct_field['name']),
                                                          struct_field['format'])
            template = (
                '    ${endianfix};')

        f.write(
            '    if (tmp.error_status == QMI_STATUS_SUCCESS)\n'
            '        g_string_append (printable,\n'
            '                         "SUCCESS");\n'
            '    else\n'
            '        g_string_append_printf (printable,\n'
            '                                "FAILURE: %s",\n'
            '                                qmi_protocol_error_get_string ((QmiProtocolError) tmp.error_code));\n'
            '\n'
            '    return g_string_free (printable, FALSE);\n'
            '}\n')
