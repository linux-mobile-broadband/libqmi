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

        # Strings will be heap-allocated
        self.needs_dispose = True

        # Strings which are given as the full value of a TLV will NOT have a
        # length prefix
        self.length_prefix = False if 'type' in dictionary and dictionary['type'] == 'TLV' else True


    """
    Read a string from the raw byte buffer.
    """
    def emit_buffer_read(self, f, line_prefix, variable_name, buffer_name, buffer_len):
        translations = { 'lp'             : line_prefix,
                         'variable_name'  : variable_name,
                         'buffer_name'    : buffer_name,
                         'buffer_len'     : buffer_len,
                         'length_prefix'  : 'TRUE' if self.length_prefix else 'FALSE' }

        template = (
            '${lp}/* Read the string variable from the buffer */\n'
            '${lp}qmi_utils_read_string_from_buffer (\n'
            '${lp}    &${buffer_name},\n'
            '${lp}    &${buffer_len},\n'
            '${lp}    ${length_prefix},\n'
            '${lp}    &(${variable_name}));\n')
        f.write(string.Template(template).substitute(translations))


    """
    Write a string to the raw byte buffer.
    """
    def emit_buffer_write(self, f, line_prefix, variable_name, buffer_name, buffer_len):
        translations = { 'lp'             : line_prefix,
                         'variable_name'  : variable_name,
                         'buffer_name'    : buffer_name,
                         'buffer_len'     : buffer_len,
                         'length_prefix'  : 'TRUE' if self.length_prefix else 'FALSE' }

        template = (
            '${lp}/* Write the string variable to the buffer */\n'
            '${lp}qmi_utils_write_string_to_buffer (\n'
            '${lp}    &${buffer_name},\n'
            '${lp}    &${buffer_len},\n'
            '${lp}    ${length_prefix},\n'
            '${lp}    &(${variable_name}));\n')
        f.write(string.Template(template).substitute(translations))


    """
    Get the string as printable
    """
    def emit_get_printable(self, f, line_prefix, printable, buffer_name, buffer_len):
        translations = { 'lp'             : line_prefix,
                         'printable'      : printable,
                         'buffer_name'    : buffer_name,
                         'buffer_len'     : buffer_len,
                         'length_prefix'  : 'TRUE' if self.length_prefix else 'FALSE' }

        template = (
            '\n'
            '${lp}{\n'
            '${lp}    gchar *tmp;\n'
            '\n'
            '${lp}    /* Read the string variable from the buffer */\n'
            '${lp}    qmi_utils_read_string_from_buffer (\n'
            '${lp}        &${buffer_name},\n'
            '${lp}        &${buffer_len},\n'
            '${lp}        ${length_prefix},\n'
            '${lp}        &tmp);\n'
            '\n'
            '${lp}    g_string_append_printf (${printable}, "%s", tmp);\n'
            '${lp}    g_free (tmp);\n'
            '${lp}}\n')
        f.write(string.Template(template).substitute(translations))


    """
    Variable declaration
    """
    def build_variable_declaration(self, line_prefix, variable_name):
        translations = { 'lp'   : line_prefix,
                         'name' : variable_name }

        template = (
            '${lp}gchar *${name};\n')
        return string.Template(template).substitute(translations)


    """
    Getter for the string type
    """
    def build_getter_declaration(self, line_prefix, variable_name):
        translations = { 'lp'   : line_prefix,
                         'name' : variable_name }

        template = (
            '${lp}const gchar **${name},\n')
        return string.Template(template).substitute(translations)


    """
    Documentation for the getter
    """
    def build_getter_documentation(self, line_prefix, variable_name):
        translations = { 'lp'   : line_prefix,
                         'name' : variable_name }

        template = (
            '${lp}@${name}: a placeholder for the output constant string, or #NULL if not required.\n')
        return string.Template(template).substitute(translations)


    """
    Builds the String getter implementation
    """
    def build_getter_implementation(self, line_prefix, variable_name_from, variable_name_to, to_is_reference):
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
        translations = { 'lp'   : line_prefix,
                         'name' : variable_name }

        template = (
            '${lp}const gchar *${name},\n')
        return string.Template(template).substitute(translations)


    """
    Documentation for the setter
    """
    def build_setter_documentation(self, line_prefix, variable_name):
        translations = { 'lp'   : line_prefix,
                         'name' : variable_name }

        template = (
            '${lp}@${name}: a placeholder for the output constant string, or #NULL if not required.\n')
        return string.Template(template).substitute(translations)


    """
    Builds the String setter implementation
    """
    def build_setter_implementation(self, line_prefix, variable_name_from, variable_name_to):
        translations = { 'lp'   : line_prefix,
                         'from' : variable_name_from,
                         'to'   : variable_name_to }

        template = (
            '${lp}g_free (${to});\n'
            '${lp}${to} = g_strdup (${from});\n')
        return string.Template(template).substitute(translations)


    """
    Dispose the string
    """
    def build_dispose(self, line_prefix, variable_name):
        translations = { 'lp'            : line_prefix,
                         'variable_name' : variable_name }

        template = (
            '${lp}g_free (${variable_name});\n')
        return string.Template(template).substitute(translations)
