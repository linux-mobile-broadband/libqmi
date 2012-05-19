#!/usr/bin/env python
# -*- Mode: python; tab-width: 4; indent-tabs-mode: nil -*-
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

class FieldArray(Field):
    """
    The FieldArray class takes care of handling 'array' format based
    Input and Output TLVs
    """

    def __init__(self, prefix, dictionary):
        # Call the parent constructor
        Field.__init__(self, prefix, dictionary)

        if dictionary['array-element']['format'] != 'struct':
            raise ValueError('Cannot handle arrays of format \'%s\'' % dictionary['array-element']['format'])

        # Set a struct as content
        self.array_element = Struct(utils.build_camelcase_name (self.fullname +
                                                                ' ' +
                                                                dictionary['array-element']['name']),
                                    dictionary['array-element']['contents'])

        # We'll use standard GArrays
        self.field_type = 'GArray *';
        # The array needs to get disposed
        self.dispose = 'g_array_unref'


    def emit_types(self, hfile, cfile):
        '''
        Emit the type for the struct used as array element
        '''
        self.array_element.emit(hfile)


    def emit_input_tlv_add(self, cfile, line_prefix):
        # TODO
        raise ValueError('Array as input not implemented yet')

    def emit_output_tlv_get(self, f, line_prefix):
        translations = { 'name'                 : self.name,
                         'container_underscore' : utils.build_underscore_name (self.prefix),
                         'array_element_type'   : self.array_element.name,
                         'tlv_id'               : self.id_enum_name,
                         'variable_name'        : self.variable_name,
                         'lp'                   : line_prefix,
                         'error'                : 'error' if self.mandatory == 'yes' else 'NULL'}

        template = (
            '\n'
            '${lp}guint i;\n'
            '${lp}guint8 buffer[1024];\n'
            '${lp}guint16 buffer_len = 1024;\n'
            '${lp}${array_element_type} *item;\n'
            '\n'
            '${lp}if (qmi_message_tlv_get_varlen (message,\n'
            '${lp}                                ${tlv_id},\n'
            '${lp}                                &buffer_len,\n'
            '${lp}                                buffer,\n'
            '${lp}                                ${error})) {\n'
            '${lp}    guint8 nitems = buffer[0];\n'
            '\n'
            '${lp}    self->${variable_name}_set = TRUE;\n'
            '${lp}    self->${variable_name} = g_array_sized_new (FALSE, FALSE, sizeof (${array_element_type}), nitems);\n'
            '${lp}    for (i = 0, item = (${array_element_type} *)&buffer[1]; i < nitems; i++, item++) {\n')
        f.write(string.Template(template).substitute(translations))

        for struct_field in self.array_element.members:
            f.write('%s        %s;\n' % (line_prefix,
                                         utils.he_from_le ('item->' + utils.build_underscore_name(struct_field['name']),
                                                           'item->' + utils.build_underscore_name(struct_field['name']),
                                                           struct_field['type'])))

        template = (
            '${lp}        g_array_insert_val (self->${variable_name}, i, *item);\n'
            '${lp}    }\n')

        if self.mandatory == 'yes':
            template += (
                '${lp}} else {\n'
                '${lp}    g_prefix_error (error, \"Couldn\'t get the ${name} TLV: \");\n'
                '${lp}    ${container_underscore}_unref (self);\n'
                '${lp}    return NULL;\n'
                '${lp}}\n')
        else:
            template += (
                '${lp}}\n')
        f.write(string.Template(template).substitute(translations))
