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
# Copyright (C) 2012-2015 Aleksander Morgado <aleksander@aleksander.es>
#

import string
import utils
from Variable import Variable

"""
Variable type for Strings ('string' format)
"""
class VariableString(Variable):

    """
    Constructor
    """
    def __init__(self, dictionary):

        # Call the parent constructor
        Variable.__init__(self, dictionary)

        self.private_format = 'gchar *'
        self.public_format = self.private_format
        self.element_type = 'utf8'

        if 'fixed-size' in dictionary:
            self.is_fixed_size = True
            # Fixed-size strings
            self.needs_dispose = False
            self.length_prefix_size = 0
            self.n_size_prefix_bytes = 0
            self.fixed_size = dictionary['fixed-size']
            self.max_size = ''
        else:
            self.is_fixed_size = False
            self.fixed_size = '-1'
            # Variable-length strings in heap
            self.needs_dispose = True
            if 'size-prefix-format' in dictionary:
                if dictionary['size-prefix-format'] == 'guint8':
                    self.length_prefix_size = 8
                    self.n_size_prefix_bytes = 1
                elif dictionary['size-prefix-format'] == 'guint16':
                    self.length_prefix_size = 16
                    self.n_size_prefix_bytes = 2
                else:
                    raise ValueError('Invalid size prefix format (%s): not guint8 or guint16' % dictionary['size-prefix-format'])
            # Strings which are given as the full value of a TLV and which don't have
            # a explicit 'size-prefix-format' will NOT have a length prefix
            elif 'type' in dictionary and dictionary['type'] == 'TLV':
                self.length_prefix_size = 0
                self.n_size_prefix_bytes = 0
            else:
                # Default to UINT8
                self.length_prefix_size = 8
                self.n_size_prefix_bytes = 1
            self.max_size = dictionary['max-size'] if 'max-size' in dictionary else ''


    """
    Read a string from the raw byte buffer.
    """
    def emit_buffer_read(self, f, line_prefix, tlv_out, error, variable_name):
        translations = { 'lp'            : line_prefix,
                         'tlv_out'       : tlv_out,
                         'variable_name' : variable_name,
                         'error'         : error }

        if self.is_fixed_size:
            translations['fixed_size'] = self.fixed_size

            # Fixed sized strings exposed in public fields will need to be
            # explicitly allocated in heap
            if self.public:
                translations['fixed_size_plus_one'] = int(self.fixed_size) + 1
                template = (
                    '${lp}${variable_name} = g_malloc (${fixed_size_plus_one});\n'
                    '${lp}if (!qmi_message_tlv_read_fixed_size_string (message, init_offset, &offset, ${fixed_size}, &${variable_name}[0], ${error})) {\n'
                    '${lp}    g_free (${variable_name});\n'
                    '${lp}    ${variable_name} = NULL;\n'
                    '${lp}    goto ${tlv_out};\n'
                    '${lp}}\n'
                    '${lp}${variable_name}[${fixed_size}] = \'\\0\';\n')
            else:
                template = (
                    '${lp}if (!qmi_message_tlv_read_fixed_size_string (message, init_offset, &offset, ${fixed_size}, &${variable_name}[0], ${error}))\n'
                    '${lp}    goto ${tlv_out};\n'
                    '${lp}${variable_name}[${fixed_size}] = \'\\0\';\n')
        else:
            translations['n_size_prefix_bytes'] = self.n_size_prefix_bytes
            translations['max_size'] = self.max_size if self.max_size != '' else '0'
            template = (
                '${lp}if (!qmi_message_tlv_read_string (message, init_offset, &offset, ${n_size_prefix_bytes}, ${max_size}, &(${variable_name}), ${error}))\n'
                '${lp}    goto ${tlv_out};\n')
        f.write(string.Template(template).substitute(translations))


    """
    Write a string to the raw byte buffer.
    """
    def emit_buffer_write(self, f, line_prefix, tlv_name, variable_name):
        translations = { 'lp'                  : line_prefix,
                         'tlv_name'            : tlv_name,
                         'variable_name'       : variable_name,
                         'fixed_size'          : self.fixed_size,
                         'n_size_prefix_bytes' : self.n_size_prefix_bytes }

        template = (
            '${lp}if (!qmi_message_tlv_write_string (self, ${n_size_prefix_bytes}, ${variable_name}, ${fixed_size}, error)) {\n'
            '${lp}    g_prefix_error (error, "Cannot write string in TLV \'${tlv_name}\': ");\n'
            '${lp}    return NULL;\n'
            '${lp}}\n')

        f.write(string.Template(template).substitute(translations))


    """
    Get the string as printable
    """
    def emit_get_printable(self, f, line_prefix):
        translations = { 'lp' : line_prefix }

        if self.is_fixed_size:
            translations['fixed_size'] = self.fixed_size
            translations['fixed_size_plus_one'] = int(self.fixed_size) + 1
            template = (
                '\n'
                '${lp}{\n'
                '${lp}    gchar tmp[${fixed_size_plus_one}];\n'
                '\n'
                '${lp}    if (!qmi_message_tlv_read_fixed_size_string (message, init_offset, &offset, ${fixed_size}, &tmp[0], &error))\n'
                '${lp}        goto out;\n'
                '${lp}    tmp[${fixed_size}] = \'\\0\';\n'
                '${lp}    g_string_append (printable, tmp);\n'
                '${lp}}\n')
        else:
            translations['n_size_prefix_bytes'] = self.n_size_prefix_bytes
            translations['max_size'] = self.max_size if self.max_size != '' else '0'
            template = (
                '\n'
                '${lp}{\n'
                '${lp}    g_autofree gchar *tmp = NULL;\n'
                '\n'
                '${lp}    if (!qmi_message_tlv_read_string (message, init_offset, &offset, ${n_size_prefix_bytes}, ${max_size}, &tmp, &error))\n'
                '${lp}        goto out;\n'
                '${lp}    g_string_append (printable, tmp);\n'
                '${lp}}\n')

        f.write(string.Template(template).substitute(translations))


    """
    Variable declaration
    """
    def build_variable_declaration(self, public, line_prefix, variable_name):
        translations = { 'lp'   : line_prefix,
                         'name' : variable_name }

        if public:
            # Fixed sized strings given in public structs are given as pointers,
            # instead of as fixed-sized arrays directly in the struct.
            template = (
                '${lp}gchar *${name};\n')
            self.is_public = True
        elif self.is_fixed_size:
            translations['fixed_size_plus_one'] = int(self.fixed_size) + 1
            template = (
                '${lp}gchar ${name}[${fixed_size_plus_one}];\n')
        else:
            template = (
                '${lp}gchar *${name};\n')
        return string.Template(template).substitute(translations)


    """
    Getter for the string type
    """
    def build_getter_declaration(self, line_prefix, variable_name):
        if not self.visible:
            return ""

        translations = { 'lp'   : line_prefix,
                         'name' : variable_name }

        template = (
            '${lp}const gchar **${name},\n')
        return string.Template(template).substitute(translations)


    """
    Documentation for the getter
    """
    def build_getter_documentation(self, line_prefix, variable_name):
        if not self.visible:
            return ""

        translations = { 'lp'   : line_prefix,
                         'name' : variable_name }

        template = (
            '${lp}@${name}: (out)(optional)(transfer none): a placeholder for the output constant string, or %NULL if not required.\n')
        return string.Template(template).substitute(translations)


    """
    Builds the String getter implementation
    """
    def build_getter_implementation(self, line_prefix, variable_name_from, variable_name_to, to_is_reference):
        if not self.visible:
            return ""

        translations = { 'lp'   : line_prefix,
                         'from' : variable_name_from,
                         'to'   : variable_name_to }

        if to_is_reference:
            template = (
                '${lp}if (${to})\n'
                '${lp}    *${to} = ${from};\n')
            return string.Template(template).substitute(translations)
        else:
            template = (
                '${lp}${to} = ${from};\n')
            return string.Template(template).substitute(translations)


    """
    Setter for the string type
    """
    def build_setter_declaration(self, line_prefix, variable_name):
        if not self.visible:
            return ""

        translations = { 'lp'   : line_prefix,
                         'name' : variable_name }

        template = (
            '${lp}const gchar *${name},\n')
        return string.Template(template).substitute(translations)


    """
    Documentation for the setter
    """
    def build_setter_documentation(self, line_prefix, variable_name):
        if not self.visible:
            return ""

        translations = { 'lp'   : line_prefix,
                         'name' : variable_name }

        if self.is_fixed_size:
            translations['fixed_size'] = self.fixed_size
            template = (
                '${lp}@${name}: a constant string of exactly ${fixed_size} characters.\n')
        elif self.max_size != '':
            translations['max_size'] = self.max_size
            template = (
                '${lp}@${name}: a constant string with a maximum length of ${max_size} characters.\n')
        else:
            template = (
                '${lp}@${name}: a constant string.\n')
        return string.Template(template).substitute(translations)


    """
    Builds the String setter implementation
    """
    def build_setter_implementation(self, line_prefix, variable_name_from, variable_name_to):
        if not self.visible:
            return ""

        translations = { 'lp'   : line_prefix,
                         'from' : variable_name_from,
                         'to'   : variable_name_to }

        if self.is_fixed_size:
            translations['fixed_size'] = self.fixed_size
            template = (
                '${lp}if (!${from} || strlen (${from}) != ${fixed_size}) {\n'
                '${lp}    g_set_error (error,\n'
                '${lp}                 QMI_CORE_ERROR,\n'
                '${lp}                 QMI_CORE_ERROR_INVALID_ARGS,\n'
                '${lp}                 "Input variable \'${from}\' must be ${fixed_size} characters long");\n'
                '${lp}    return FALSE;\n'
                '${lp}}\n'
                '${lp}memcpy (${to}, ${from}, ${fixed_size});\n'
                '${lp}${to}[${fixed_size}] = \'\\0\';\n')
        else:
            template = ''
            if self.max_size != '':
                translations['max_size'] = self.max_size
                template += (
                    '${lp}if (${from} && strlen (${from}) > ${max_size}) {\n'
                    '${lp}    g_set_error (error,\n'
                    '${lp}                 QMI_CORE_ERROR,\n'
                    '${lp}                 QMI_CORE_ERROR_INVALID_ARGS,\n'
                    '${lp}                 "Input variable \'${from}\' must be less than ${max_size} characters long");\n'
                    '${lp}    return FALSE;\n'
                    '${lp}}\n')
            template += (
                '${lp}g_free (${to});\n'
                '${lp}${to} = g_strdup (${from} ? ${from} : "");\n')

        return string.Template(template).substitute(translations)


    """
    Documentation for the struct field
    """
    def build_struct_field_documentation(self, line_prefix, variable_name):
        translations = { 'lp'   : line_prefix,
                         'name' : variable_name }

        if self.is_fixed_size:
            translations['fixed_size'] = self.fixed_size
            template = (
                '${lp}@${name}: a string of exactly ${fixed_size} characters.\n')
        elif self.max_size != '':
            translations['max_size'] = self.max_size
            template = (
                '${lp}@${name}: a string with a maximum length of ${max_size} characters.\n')
        else:
            template = (
                '${lp}@${name}: a string.\n')
        return string.Template(template).substitute(translations)


    """
    Dispose the string
    """
    def build_dispose(self, line_prefix, variable_name):
        # Fixed-size strings don't need dispose
        if self.is_fixed_size and not self.public:
            return ''

        translations = { 'lp'            : line_prefix,
                         'variable_name' : variable_name }

        template = (
            '${lp}g_free (${variable_name});\n')
        return string.Template(template).substitute(translations)


    """
    Flag as being public
    """
    def flag_public(self):
        # Call the parent method
        Variable.flag_public(self)
        # Fixed-sized strings will need dispose if they are in the public header
        if self.is_fixed_size:
            self.needs_dispose = True
