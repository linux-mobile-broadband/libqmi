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
Variable type for signed/unsigned Integers
('guint8', 'gint8', 'guint16', 'gint16', 'guint32', 'gint32' formats)
"""
class VariableInteger(Variable):

    """
    Constructor
    """
    def __init__(self, dictionary):

        # Call the parent constructor
        Variable.__init__(self, dictionary)

        self.private_format = self.format
        self.public_format = dictionary['public-format'] if 'public-format' in dictionary else self.private_format


    """
    Read a single integer from the raw byte buffer
    """
    def emit_buffer_read(self, f, line_prefix, variable_name, buffer_name, buffer_len):
        translations = { 'lp'             : line_prefix,
                         'public_format'  : self.public_format,
                         'private_format' : self.private_format,
                         'variable_name'  : variable_name,
                         'buffer_name'    : buffer_name,
                         'buffer_len'     : buffer_len }

        if self.private_format == self.public_format:
            template = (
                '${lp}/* Read the ${private_format} variable from the buffer */\n'
                '${lp}qmi_utils_read_${private_format}_from_buffer (\n'
                '${lp}    &${buffer_name},\n'
                '${lp}    &${buffer_len},\n'
                '${lp}    &(${variable_name}));\n')
        else:
            template = (
                '${lp}{\n'
                '${lp}    ${private_format} tmp;\n'
                '\n'
                '${lp}    /* Read the ${private_format} variable from the buffer */\n'
                '${lp}    qmi_utils_read_${private_format}_from_buffer (\n'
                '${lp}        &${buffer_name},\n'
                '${lp}        &${buffer_len},\n'
                '${lp}        &tmp);\n'
                '${lp}    ${variable_name} = (${public_format})tmp;\n'
                '${lp}}\n')
        f.write(string.Template(template).substitute(translations))


    """
    Write a single integer to the raw byte buffer
    """
    def emit_buffer_write(self, f, line_prefix, variable_name, buffer_name, buffer_len):
        translations = { 'lp'             : line_prefix,
                         'private_format' : self.private_format,
                         'variable_name'  : variable_name,
                         'buffer_name'    : buffer_name,
                         'buffer_len'     : buffer_len }
        if self.private_format == self.public_format:
            template = (
                '${lp}/* Write the ${private_format} variable to the buffer */\n'
                '${lp}qmi_utils_write_${private_format}_to_buffer (\n'
                '${lp}    &${buffer_name},\n'
                '${lp}    &${buffer_len},\n'
                '${lp}    &(${variable_name}));\n')
        else:
            template = (
                '${lp}{\n'
                '${lp}    ${private_format} tmp;\n'
                '\n'
                '${lp}    tmp = (${private_format})${variable_name};\n'
                '${lp}    /* Write the ${private_format} variable to the buffer */\n'
                '${lp}    qmi_utils_write_${private_format}_to_buffer (\n'
                '${lp}        &${buffer_name},\n'
                '${lp}        &${buffer_len},\n'
                '${lp}        &tmp);\n'
                '${lp}}\n')
        f.write(string.Template(template).substitute(translations))


    """
    Get the integer as a printable string. Given that we support max 32-bit
    integers, it is safe to cast all them to standard integers when printing.
    """
    def emit_get_printable(self, f, line_prefix, printable, buffer_name, buffer_len):
        if utils.format_is_unsigned_integer(self.private_format):
            common_format = '%u'
            common_cast = 'guint'
        else:
            common_format = '%d'
            common_cast = 'gint'

        translations = { 'lp'             : line_prefix,
                         'private_format' : self.private_format,
                         'printable'      : printable,
                         'buffer_name'    : buffer_name,
                         'buffer_len'     : buffer_len,
                         'common_format'  : common_format,
                         'common_cast'    : common_cast }

        template = (
            '\n'
            '${lp}{\n'
            '${lp}    ${private_format} tmp;\n'
            '\n'
            '${lp}    /* Read the ${private_format} variable from the buffer */\n'
            '${lp}    qmi_utils_read_${private_format}_from_buffer (\n'
            '${lp}        &${buffer_name},\n'
            '${lp}        &${buffer_len},\n'
            '${lp}        &tmp);\n'
            '\n'
            '${lp}    g_string_append_printf (${printable}, "${common_format}", (${common_cast})tmp);\n'
            '${lp}}\n')
        f.write(string.Template(template).substitute(translations))


    """
    Copy an integer to another one
    """
    def emit_copy(self, f, line_prefix, variable_name_from, variable_name_to):
        translations = { 'lp'   : line_prefix,
                         'from' : variable_name_from,
                         'to'   : variable_name_to }

        template = (
            '${lp}${to} = ${from};\n')
        f.write(string.Template(template).substitute(translations))
