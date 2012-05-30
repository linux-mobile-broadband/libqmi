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
import VariableFactory

"""
Variable type for Structs ('struct' format)
"""
class VariableStruct(Variable):

    """
    Constructor
    """
    def __init__(self, dictionary, struct_type_name):

        # Call the parent constructor
        Variable.__init__(self, dictionary)

        # The public format of the struct is built directly from the suggested
        # struct type name
        self.public_format = utils.build_camelcase_name(struct_type_name)
        self.private_format = self.public_format

        # Load members of this struct
        self.members = []
        for member_dictionary in dictionary['contents']:
            member = {}
            member['name'] = utils.build_underscore_name(member_dictionary['name'])
            member['object'] = VariableFactory.create_variable(member_dictionary, struct_type_name + ' ' + member['name'])
            self.members.append(member)

        # We'll need to dispose if at least one of the members needs it
        for member in self.members:
            if member['object'].needs_dispose == True:
                self.needs_dispose = True


    """
    Emit all types for the members of the struct plus the new struct type itself
    """
    def emit_types(self, f):
        # Emit types for each member
        for member in self.members:
            member['object'].emit_types(f)

        translations = { 'format' : self.public_format }
        template = (
            '\n'
            'typedef struct _${format} {\n')
        f.write(string.Template(template).substitute(translations))

        for member in self.members:
            translations['variable_format'] = member['object'].public_format
            translations['variable_name'] = member['name']
            template = (
                '    ${variable_format} ${variable_name};\n')
            f.write(string.Template(template).substitute(translations))

        template = ('} ${format};\n')
        f.write(string.Template(template).substitute(translations))


    """
    Reading the contents of a struct is just about reading each of the struct
    fields one by one.
    """
    def emit_buffer_read(self, f, line_prefix, variable_name, buffer_name, buffer_len):
        for member in self.members:
            member['object'].emit_buffer_read(f, line_prefix, variable_name + '.' +  member['name'], buffer_name, buffer_len)


    """
    Writing the contents of a struct is just about writing each of the struct
    fields one by one.
    """
    def emit_buffer_write(self, f, line_prefix, variable_name, buffer_name, buffer_len):
        for member in self.members:
            member['object'].emit_buffer_write(f, line_prefix, variable_name + '.' +  member['name'], buffer_name, buffer_len)


    """
    The struct will be printed as a list of fields enclosed between square
    brackets
    """
    def emit_get_printable(self, f, line_prefix, printable, buffer_name, buffer_len):
        translations = { 'lp'        : line_prefix,
                         'printable' : printable }

        template = (
            '${lp}g_string_append (${printable}, "[");\n')
        f.write(string.Template(template).substitute(translations))

        for member in self.members:
            translations['variable_name'] = member['name']
            template = (
                '${lp}g_string_append (${printable}, " ${variable_name} = \'");\n')
            f.write(string.Template(template).substitute(translations))

            member['object'].emit_get_printable(f, line_prefix, printable, buffer_name, buffer_len)

            template = (
                '${lp}g_string_append (${printable}, "\'");\n')
            f.write(string.Template(template).substitute(translations))

        template = (
            '${lp}g_string_append (${printable}, " ]");\n')
        f.write(string.Template(template).substitute(translations))


    """
    Copying a struct is just about copying each of the struct fields one by one.
    Note that we shouldn't just "a = b" with the structs, as the members may be
    heap-allocated strings.
    """
    def emit_copy(self, f, line_prefix, variable_name_from, variable_name_to):
        for member in self.members:
            member['object'].emit_copy(f, line_prefix, variable_name_from + '.' + member['name'], variable_name_to + '.' + member['name'])


    """
    Disposing a struct is just about disposing each of the struct fields one by
    one.
    """
    def emit_dispose(self, f, line_prefix, variable_name):
        for member in self.members:
            member['object'].emit_dispose(f, line_prefix, variable_name + '.' + member['name'])
