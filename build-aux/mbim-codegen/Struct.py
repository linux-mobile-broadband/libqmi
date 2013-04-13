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
The Struct class takes care of emitting the struct type
"""
class Struct:

    """
    Constructor
    """
    def __init__(self, dictionary):
        self.name = dictionary['name']
        self.contents = dictionary['contents']

    """
    Emit the new struct type
    """
    def _emit_type(self, hfile):
        translations = { 'name' : self.name }
        template = (
            '\n'
            'typedef struct {\n')
        for field in self.contents:
            translations['field_name_underscore'] = utils.build_underscore_name_from_camelcase(field['name'])
            if field['format'] == 'uuid':
                inner_template = (
                    '    MbimUuid ${field_name_underscore};\n')
            elif field['format'] == 'guint32':
                inner_template = (
                    '    guint32 ${field_name_underscore};\n')
            elif field['format'] == 'guint32-array':
                inner_template = (
                    '    guint32 *${field_name_underscore};\n')
            elif field['format'] == 'string':
                inner_template = (
                    '    gchar *${field_name_underscore};\n')
            elif field['format'] == 'string-array':
                inner_template = (
                    '    gchar **${field_name_underscore};\n')
            else:
                raise ValueError('Cannot handle format \'%s\' in struct' % field['format'])
            template += string.Template(inner_template).substitute(translations)
        template += (
            '} ${name};\n')
        hfile.write(string.Template(template).substitute(translations))


    """
    Emit the type's free methods
    """
    def _emit_free(self, hfile, cfile):
        translations = { 'name' : self.name,
                         'name_underscore' : utils.build_underscore_name_from_camelcase(self.name) }
        template = (
            '\n'
            'void ${name_underscore}_free (${name} *var);\n'
            'void ${name_underscore}_array_free (${name} **array);\n')
        hfile.write(string.Template(template).substitute(translations))

        template = (
            '\n'
            '/**\n'
            ' * ${name_underscore}_free:\n'
            ' * @var: a #${name}.\n'
            ' *\n'
            ' * Frees the memory allocated for the #${name}.\n'
            ' */\n'
            'void\n'
            '${name_underscore}_free (${name} *var)\n'
            '{\n')

        for field in self.contents:
            translations['field_name_underscore'] = utils.build_underscore_name_from_camelcase(field['name'])
            inner_template = ''
            if field['format'] == 'uuid':
                pass
            elif field['format'] == 'guint32':
                pass
            elif field['format'] == 'guint32-array':
                inner_template += (
                    '    g_free (var->${field_name_underscore});\n')
            elif field['format'] == 'string':
                inner_template += (
                    '    g_free (var->${field_name_underscore});\n')
            elif field['format'] == 'string-array':
                inner_template += (
                    '    g_strfreev (var->${field_name_underscore});\n')
            else:
                raise ValueError('Cannot handle format \'%s\' in struct clear' % field['format'])
            template += string.Template(inner_template).substitute(translations)

        template += (
            '    g_free (var);\n'
            '}\n'
            '\n'
            '/**\n'
            ' * ${name_underscore}_array_free:\n'
            ' * @array: a #NULL-terminated array of #${name} structs.\n'
            ' *\n'
            ' * Frees the memory allocated for the array of #${name}s.\n'
            ' */\n'
            'void\n'
            '${name_underscore}_array_free (${name} **array)\n'
            '{\n'
            '    guint32 i;\n'
            '\n'
            '    for (i = 0; array[i]; i++)\n'
            '        ${name_underscore}_free (array[i]);\n'
            '    g_free (array);\n'
            '}\n')
        cfile.write(string.Template(template).substitute(translations))


    """
    Emit the type's read methods
    """
    def _emit_read(self, cfile):
        # Setup offset for reading fields
        offset = 0
        # Additional offset string computation
        additional_offset_str = ''

        translations = { 'name'            : self.name,
                         'name_underscore' : utils.build_underscore_name_from_camelcase(self.name) }

        template = (
            '\n'
            'static ${name} *\n'
            '_mbim_message_read_${name_underscore}_struct (\n'
            '    const MbimMessage *self,\n'
            '    guint32 relative_offset,\n'
            '    guint32 *bytes_read)\n'
            '{\n'
            '    ${name} *out;\n'
            '    guint32 offset = relative_offset;\n'
            '\n'
            '    g_assert (self != NULL);\n'
            '\n'
            '    out = g_new (${name}, 1);\n')

        for field in self.contents:
            translations['field_name_underscore'] = utils.build_underscore_name_from_camelcase(field['name'])
            translations['format_underscore']     = utils.build_underscore_name(field['format'])
            translations['offset']                = offset

            inner_template = ''
            if field['format'] == 'uuid':
                inner_template += (
                    '\n'
                    '    memcpy (&out->${field_name_underscore}, _mbim_message_read_uuid (self, offset), 16);\n'
                    '    offset += 16;\n')
            elif field['format'] == 'guint32':
                inner_template += (
                    '\n'
                    '    out->${field_name_underscore} = _mbim_message_read_guint32 (self, offset);\n'
                    '    offset += 4;\n')
            elif field['format'] == 'guint32-array':
                translations['array_size_field_name_underscore'] = utils.build_underscore_name_from_camelcase(field['array-size-field'])
                inner_template += (
                    '\n'
                    '    out->${field_name_underscore} = _mbim_message_read_guint32_array (self, out->${array_size_field_name_underscore}, offset);\n'
                    '    offset += (4 * out->${array_size_field_name_underscore});\n')
            elif field['format'] == 'string':
                inner_template += (
                    '\n'
                    '    out->${field_name_underscore} = _mbim_message_read_string (self, offset);\n'
                    '    offset += 8;\n')
            elif field['format'] == 'string-array':
                translations['array_size_field_name_underscore'] = utils.build_underscore_name_from_camelcase(field['array-size-field'])
                inner_template += (
                    '\n'
                    '    out->${field_name_underscore} = _mbim_message_read_string_array (self, out->${array_size_field_name_underscore}, offset);\n'
                    '    offset += (8 * out->${array_size_field_name_underscore});\n')
            else:
                raise ValueError('Cannot handle format \'%s\' in struct' % field['format'])

            template += string.Template(inner_template).substitute(translations)

        template += (
            '\n'
            '    if (bytes_read)\n'
            '        *bytes_read = (offset - relative_offset);\n'
            '\n'
            '    return out;\n'
            '}\n')
        cfile.write(string.Template(template).substitute(translations))


        template = (
            '\n'
            'static ${name} **\n'
            '_mbim_message_read_${name_underscore}_struct_array (\n'
            '    const MbimMessage *self,\n'
            '    guint32 array_size,\n'
            '    guint32 relative_offset_array_start)\n'
            '{\n'
            '    ${name} **out;\n'
            '    guint32 i;\n'
            '    guint32 offset;\n'
            '\n'
            '    out = g_new (${name} *, array_size + 1);\n'
            '    offset = relative_offset_array_start;\n'
            '    for (i = 0; i < array_size; i++, offset += 8)\n'
            '        out[i] = _mbim_message_read_${name_underscore}_struct (self, _mbim_message_read_guint32 (self, offset), NULL);\n'
            '    out[array_size] = NULL;\n'
            '\n'
            '    return out;\n'
            '}\n')
        cfile.write(string.Template(template).substitute(translations))


    """
    Emit the struct handling implementation
    """
    def emit(self, hfile, cfile):
        utils.add_separator(hfile, 'Struct', self.name);
        utils.add_separator(cfile, 'Struct', self.name);

        # Emit type
        self._emit_type(hfile)
        # Emit type's free
        self._emit_free(hfile, cfile)
        # Emit type's read
        self._emit_read(cfile)
