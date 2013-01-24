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
        elif container_type == 'Output':
            if message_type == 'Notify':
                self.name = 'Indication'
            else:
                self.name = 'Response'
        else:
            raise ValueError('Cannot handle container type \'%s\'' % container_type)

        # Create the composed full name (prefix + name),
        #  e.g. "Mbim Message Service Something Query Response"
        self.fullname = self.prefix + ' ' + self.message_type + ' ' + self.name

        self.fields_dictionary = dictionary

    """
    Emit the container handling implementation
    """
    def emit(self, hfile, cfile):
        # Setup offset for reading fields
        offset = 0
        # Additional offset string computation
        additional_offset_str = ''

        for field in self.fields_dictionary:
            translations = { 'underscore'              : utils.build_underscore_name (self.fullname),
                             'field_name'              : field['name'],
                             'field_name_underscore'   : utils.build_underscore_name (field['name']),
                             'format_underscore'       : utils.build_underscore_name (field['format']),
                             'command_underscore'      : utils.build_underscore_name (self.command),
                             'name_underscore'         : utils.build_underscore_name (self.name),
                             'message_type_underscore' : utils.build_underscore_name (self.message_type),
                             'offset'                  : offset }

            if field['format'] == 'guint32':
                if 'public-format' in field:
                    translations['public_format'] = field['public-format']
                else:
                    translations['public_format'] = field['format']
                translations['format_description'] = 'a #' + translations['public_format']
            elif field['format'] == 'string':
                translations['public_format'] = 'gchar *'
                translations['format_description'] = 'a newly allocated string, which should be freed with g_free()'
            elif field['format'] == 'string-array':
                translations['public_format'] = 'gchar **'
                translations['format_description'] = 'a newly allocated array of strings, which should be freed with g_strfreev()'

            template = (
                '\n'
                '${public_format} ${underscore}_get_${field_name_underscore} (const MbimMessage *self);\n')
            hfile.write(string.Template(template).substitute(translations))

            template = (
                '\n'
                '/**\n'
                ' * ${underscore}_get_${field_name_underscore}:\n'
                ' * @self: a #MbimMessage.\n'
                ' *\n'
                ' * Get the \'${field_name}\' field from the \'${command_underscore}\' ${message_type_underscore} ${name_underscore}\n'
                ' *\n'
                ' * Returns: ${format_description}.\n'
                ' */\n'
                '${public_format}\n'
                '${underscore}_get_${field_name_underscore} (const MbimMessage *self)\n'
                '{\n')

            if additional_offset_str != '':
                translations['additional_offset_str'] = additional_offset_str
                template += (
                    '    guint32 offset = ${offset};\n'
                    '\n'
                    '    offset += ${additional_offset_str};\n'
                    '\n'
                    '    return (${public_format}) _mbim_message_command_done_read_${format_underscore} (self, offset);\n')
            else:
                template += (
                    '    return (${public_format}) _mbim_message_command_done_read_${format_underscore} (self, ${offset});\n')

            template += (
                '}\n')
            cfile.write(string.Template(template).substitute(translations))

            # Update offset
            if field['format'] == 'guint32':
                offset += 4
            elif field['format'] == 'string':
                offset += 8
            elif field['format'] == 'string-array':
                offset += 4
                if additional_offset_str != '':
                    additional_offset_str += ' + '
                additional_offset_str += string.Template('(8 * _mbim_message_command_done_read_guint32 (self, ${offset}))').substitute(translations)
