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
from Field  import Field

class FieldBasic(Field):
    """
    The FieldBasic class takes care of handling Input and Output TLVs based
    on basic types (e.g. integers)
    """

    def __init__(self, prefix, dictionary, common_objects_dictionary):

        # Call the parent constructor
        Field.__init__(self, prefix, dictionary, common_objects_dictionary)

        # The field type to be used in the generated code is the same as the one
        # given in the database
        self.field_type = self.format;
        self.public_field_type = self.public_format;

    def emit_input_tlv_add(self, cfile, line_prefix):
        translations = { 'name'          : self.name,
                         'tlv_id'        : self.id_enum_name,
                         'field_type'    : self.field_type,
                         'variable_name' : self.variable_name,
                         'set_variable'  : utils.le_from_he ('input->' + self.variable_name, 'tmp', self.field_type),
                         'lp'            : line_prefix }

        template = (
            '${lp}${field_type} tmp;\n'
            '\n'
            '${lp}${set_variable};\n'
            '\n'
            '${lp}if (!qmi_message_tlv_add (self,\n'
            '${lp}                          (guint8)${tlv_id},\n'
            '${lp}                          sizeof (tmp),\n'
            '${lp}                          &tmp,\n'
            '${lp}                          error)) {\n'
            '${lp}    g_prefix_error (error, \"Couldn\'t set the ${name} TLV: \");\n'
            '${lp}    qmi_message_unref (self);\n'
            '${lp}    return NULL;\n'
            '${lp}}\n')
        cfile.write(string.Template(template).substitute(translations))


    def emit_output_tlv_get(self, cfile, line_prefix):
        translations = { 'name'                 : self.name,
                         'container_underscore' : utils.build_underscore_name (self.prefix),
                         'tlv_id'               : self.id_enum_name,
                         'variable_name'        : self.variable_name,
                         'lp'                   : line_prefix,
                         'error'                : 'error' if self.mandatory == 'yes' else 'NULL'}

        template = (
                '${lp}if (qmi_message_tlv_get (message,\n'
                '${lp}                         ${tlv_id},\n'
                '${lp}                         sizeof (self->${variable_name}),\n'
                '${lp}                         &self->${variable_name},\n'
                '${lp}                         ${error})) {\n'
                '${lp}    self->${variable_name}_set = TRUE;\n')
        cfile.write(string.Template(template).substitute(translations))

        cfile.write('%s    %s;\n' % (line_prefix,
                                     utils.le_from_he ('self->' + self.variable_name,
                                                       'self->' + self.variable_name,
                                                        self.field_type)))
        if self.mandatory is 'yes':
            template = (
                    '${lp}} else {\n'
                    '${lp}    g_prefix_error (error, \"Couldn\'t get the ${name} TLV: \");\n'
                    '${lp}    ${container_underscore}_unref (self);\n'
                    '${lp}    return NULL;\n'
                    '${lp}}\n')
        else:
            template = (
                    '${lp}}\n')
        cfile.write(string.Template(template).substitute(translations))
