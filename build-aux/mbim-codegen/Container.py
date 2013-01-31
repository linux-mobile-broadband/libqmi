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
# Copyright (C) 2013 Aleksander Morgado <aleksander@gnu.org>
#

import string
import utils
from ValueUint32      import ValueUint32
from ValueUint32Array import ValueUint32Array
from ValueString      import ValueString
from ValueStringArray import ValueStringArray
from ValueStruct      import ValueStruct
from ValueStructArray import ValueStructArray

"""
The Container class takes care of handling collections of Input or
Output fields
"""
class Container:

    """
    Constructor
    """
    def __init__(self, service, command, message_type, container_type, dictionary):
        self.service = service
        self.command = command

        # e.g. "Mbim Message Service Something"
        self.prefix = "Mbim Message " + service + ' ' + command

        # We may have 'Query', 'Set' or 'Notify' message types
        if message_type == 'Query' or message_type == 'Set' or message_type == 'Notify':
            self.message_type = message_type
        else:
            raise ValueError('Cannot handle message type \'%s\'' % message_type)

        # We may have 'Input' or 'Output' containers
        if container_type == 'Input':
            self.name = 'Request'
            self.message_type_enum = 'MBIM_MESSAGE_TYPE_COMMAND'
        elif container_type == 'Output':
            if message_type == 'Notify':
                self.name = 'Indication'
                self.message_type_enum = 'MBIM_MESSAGE_TYPE_INDICATION'
            else:
                self.name = 'Response'
                self.message_type_enum = 'MBIM_MESSAGE_TYPE_COMMAND_DONE'
        else:
            raise ValueError('Cannot handle container type \'%s\'' % container_type)

        # Create the composed full name (prefix + name),
        #  e.g. "Mbim Message Service Something Query Response"
        self.fullname = self.prefix + ' ' + self.message_type + ' ' + self.name

        self.fields_dictionary = dictionary

        self.fields = []
        for field in dictionary:
            if field['format'] == 'uuid':
                self.fields.append(ValueUuid(field))
            elif field['format'] == 'guint32':
                self.fields.append(ValueUint32(field))
            elif field['format'] == 'guint32-array':
                self.fields.append(ValueUint32Array(field))
            elif field['format'] == 'string':
                self.fields.append(ValueString(field))
            elif field['format'] == 'string-array':
                self.fields.append(ValueStringArray(field))
            elif field['format'] == 'struct':
                self.fields.append(ValueStruct(field))
            elif field['format'] == 'struct-array':
                self.fields.append(ValueStructArray(field))
            else:
                raise ValueError('Cannot handle field type \'%s\'' % field['format'])


    """
    Emit the container handling implementation
    """
    def emit(self, hfile, cfile):
        # Setup offset for reading fields
        offset = 0
        # Additional offset string computation
        additional_offset_str = ''

        translations = { 'underscore'              : utils.build_underscore_name (self.fullname),
                         'command_underscore'      : utils.build_underscore_name (self.command),
                         'name_underscore'         : utils.build_underscore_name (self.name),
                         'message_type_underscore' : utils.build_underscore_name (self.message_type),
                         'message_type_enum'       : self.message_type_enum }

        for field in self.fields:
            translations['offset']                    = offset
            translations['field_name']                = field.name
            translations['field_name_underscore']     = utils.build_underscore_name (field.name)
            translations['method_prefix']             = '_' if field.visible == False else ''
            translations['static']                    = 'static ' if field.visible == False else ''
            translations['getter_return']             = field.getter_return
            translations['getter_return_error']       = field.getter_return_error
            translations['getter_return_description'] = field.getter_return_description
            translations['reader_method_name']        = field.reader_method_name
            translations['additional_offset_str']     = additional_offset_str
            if field.is_array:
                translations['array_size_field_name_underscore'] = utils.build_underscore_name (field.array_size_field)
                translations['array_member_size']                = str(field.array_member_size)

            cfile.write('\n')

            if field.visible:
                # Dump the getter header
                template = (
                    '\n'
                    '${getter_return} ${underscore}_get_${field_name_underscore} (\n')
                if field.is_array:
                    template += (
                        '    const MbimMessage *self,\n'
                        '    guint32 *size);\n')
                else:
                    template += (
                        '    const MbimMessage *self);\n')
                hfile.write(string.Template(template).substitute(translations))

                # Dump the getter documentation
                template = (
                    '/**\n'
                    ' * ${underscore}_get_${field_name_underscore}:\n'
                    ' * @self: a #MbimMessage.\n')

                if field.is_array:
                    template += (
                        ' * @size: (out) (allow-none): number of items in the output array.\n')

                template += (
                    ' *\n'
                    ' * Get the \'${field_name}\' field from the \'${command_underscore}\' ${message_type_underscore} ${name_underscore}\n'
                    ' *\n'
                    ' * Returns: ${getter_return_description}\n'
                    ' */\n')
                cfile.write(string.Template(template).substitute(translations))

            # Dump the getter source
            template = (
                '${static}${getter_return}\n'
                '${method_prefix}${underscore}_get_${field_name_underscore} (\n')

            if field.is_array:
                template += (
                    '    const MbimMessage *self,\n'
                    '    guint32 *size)\n')
            else:
                template += (
                    '    const MbimMessage *self)\n')

            template += (
                '{\n'
                '    guint32 offset = ${offset};\n')

            if field.is_array:
                template += (
                    '    guint32 tmp;\n')

            template += (
                '\n'
                '    g_return_val_if_fail (MBIM_MESSAGE_GET_MESSAGE_TYPE (self) == ${message_type_enum}, ${getter_return_error});\n')

            if additional_offset_str != '':
                template += (
                    '\n'
                    '    offset += ${additional_offset_str};\n')

            if field.is_array:
                # Arrays need two inputs: offset of the element indicating the size of the array,
                # and the offset of where the array itself starts. The size of the array will
                # be given by another field, which we must look for.
                template += (
                    '    tmp = _${underscore}_get_${array_size_field_name_underscore} (self);\n'
                    '    if (size)\n'
                    '        *size = tmp;\n'
                    '    return (${getter_return}) ${reader_method_name} (self, tmp, offset)\n;')
            else:
                template += (
                    '    return (${getter_return}) ${reader_method_name} (self, offset);\n')

            template += (
                '}\n')
            cfile.write(string.Template(template).substitute(translations))

            # Update offsets
            if field.size > 0:
                offset += field.size
            if field.size_string != '':
                if additional_offset_str != '':
                    additional_offset_str += ' + '
                additional_offset_str += field.size_string
            if field.is_array:
                if additional_offset_str != '':
                    additional_offset_str += ' + '
                additional_offset_str += string.Template('(${array_member_size} * _${underscore}_get_${array_size_field_name_underscore} (self))').substitute(translations)
