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
from Field  import Field

class FieldStruct(Field):
    """
    The FieldStruct class takes care of handling 'struct' format based
    Input and Output TLVs
    """

    def __init__(self, prefix, dictionary, common_objects_dictionary):
        # Call the parent constructor
        Field.__init__(self, prefix, dictionary, common_objects_dictionary)

        # The field type will be the given struct name
        self.field_type = utils.build_camelcase_name(self.fullname)

        # Set a struct as content
        self.contents = Struct(self.field_type,
                               dictionary['contents'])


    def emit_types(self, hfile, cfile):
        '''
        Emit the Struct type info
        '''
        self.contents.emit_packed(cfile)
        self.contents.emit(hfile)


    def emit_input_tlv_add(self, cfile, line_prefix):
        translations = { 'name'          : self.name,
                         'tlv_id'        : self.id_enum_name,
                         'field_type'    : self.field_type,
                         'variable_name' : self.variable_name,
                         'lp'            : line_prefix }

        template = (
            '${lp}${field_type}Packed tmp;\n'
            '\n')
        cfile.write(string.Template(template).substitute(translations))

        # uint32 and uint16 fields need to be converted to host-endianness
        for struct_field in self.contents.members:
            cfile.write('%s%s;\n' % (line_prefix,
                                     utils.le_from_he ('input->' + self.variable_name + '.' + utils.build_underscore_name(struct_field['name']),
                                                       'tmp.' + utils.build_underscore_name(struct_field['name']),
                                                       struct_field['format'])))

        template = (
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


    def emit_output_tlv_get(self, f, line_prefix):
        translations = { 'name'                 : self.name,
                         'container_underscore' : utils.build_underscore_name (self.prefix),
                         'field_type'           : self.field_type,
                         'tlv_id'               : self.id_enum_name,
                         'variable_name'        : self.variable_name,
                         'lp'                   : line_prefix,
                         'error'                : 'error' if self.mandatory == 'yes' else 'NULL'}

        template = (
            '${lp}${field_type}Packed tmp;\n'
            '\n'
            '${lp}if (qmi_message_tlv_get (message,\n'
            '${lp}                         ${tlv_id},\n'
            '${lp}                         sizeof (tmp),\n'
            '${lp}                         &tmp,\n'
            '${lp}                         ${error})) {\n'
            '${lp}    self->${variable_name}_set = TRUE;\n')
        f.write(string.Template(template).substitute(translations))

        for struct_field in self.contents.members:
            f.write('%s    %s;\n' % (line_prefix,
                                     utils.he_from_le ('tmp.' + utils.build_underscore_name(struct_field['name']),
                                                       'self->' + self.variable_name + '.' + utils.build_underscore_name(struct_field['name']),
                                                       struct_field['format'])))
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
        f.write(string.Template(template).substitute(translations))
