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
# Copyright (C) 2012-2022 Aleksander Morgado <aleksander@aleksander.es>
#

import string
import utils
from Variable import Variable
import VariableFactory

"""
Variable type for Structs ('struct' format)
These variables are exclusively used as array elements, they can not be used as
independent TLVs. Use Sequence instead for that.
"""
class VariableStruct(Variable):

    """
    Constructor
    """
    def __init__(self, service, dictionary, struct_type_name, container_type):

        # Call the parent constructor
        Variable.__init__(self, service, dictionary)

        # The public format of the struct is built directly from the suggested
        # struct type name
        self.public_format = utils.build_camelcase_name(struct_type_name)
        self.private_format = self.public_format
        self.element_type = self.public_format
        self.container_type = container_type

        # Load members of this struct
        self.members = []
        for member_dictionary in dictionary['contents']:
            member = {}
            member['name'] = utils.build_underscore_name(member_dictionary['name'])
            member['object'] = VariableFactory.create_variable(self.service, member_dictionary, struct_type_name + ' ' + member['name'], self.container_type)
            # Specify that the variable will be defined in the public header
            member['object'].flag_public()
            self.members.append(member)

        # We'll need to dispose if at least one of the members needs it
        for member in self.members:
            if member['object'].needs_dispose == True:
                self.needs_dispose = True


    """
    Emit all types for the members of the struct plus the new struct type itself
    """
    def emit_types(self, f, since, static):
        # Emit types for each member
        for member in self.members:
            member['object'].emit_types(f, since, static)

        translations = { 'format' : self.public_format,
                         'since'  : since }
        template = '\n'
        f.write(string.Template(template).substitute(translations))

        if static == False and self.service != 'CTL':
            template = (
                '\n'
                '/**\n'
                ' * ${format}:\n')
            f.write(string.Template(template).substitute(translations))
            for member in self.members:
                f.write(member['object'].build_struct_field_documentation(' * ', member['name']))
            template = (
                ' *\n'
                ' * A ${format} struct.\n'
                ' *\n'
                ' * Since: ${since}\n'
                ' */\n')
            f.write(string.Template(template).substitute(translations))

        template = (
            'typedef struct _${format} {\n')
        f.write(string.Template(template).substitute(translations))

        for member in self.members:
            f.write(member['object'].build_variable_declaration(True, '    ', member['name']))

        template = ('} ${format};\n')
        f.write(string.Template(template).substitute(translations))


    """
    Emit helper methods for all types in the struct
    """
    def emit_helper_methods(self, hfile, cfile):
        # Emit for each member
        for member in self.members:
            member['object'].emit_helper_methods(hfile, cfile)


    """
    Reading the contents of a struct is just about reading each of the struct
    fields one by one.
    """
    def emit_buffer_read(self, f, line_prefix, tlv_out, error, variable_name):
        for member in self.members:
            member['object'].emit_buffer_read(f, line_prefix, tlv_out, error, variable_name + '.' +  member['name'])


    """
    Writing the contents of a struct is just about writing each of the struct
    fields one by one.
    """
    def emit_buffer_write(self, f, line_prefix, tlv_name, variable_name):
        for member in self.members:
            member['object'].emit_buffer_write(f, line_prefix, tlv_name, variable_name + '.' +  member['name'])


    """
    The struct will be printed as a list of fields enclosed between square
    brackets
    """
    def emit_get_printable(self, f, line_prefix):
        translations = { 'lp' : line_prefix }

        template = (
            '${lp}g_string_append (printable, "[");\n')
        f.write(string.Template(template).substitute(translations))

        for member in self.members:
            translations['variable_name'] = member['name']
            template = (
                '${lp}g_string_append (printable, " ${variable_name} = \'");\n')
            f.write(string.Template(template).substitute(translations))

            member['object'].emit_get_printable(f, line_prefix)

            template = (
                '${lp}g_string_append (printable, "\'");\n')
            f.write(string.Template(template).substitute(translations))

        template = (
            '${lp}g_string_append (printable, " ]");\n')
        f.write(string.Template(template).substitute(translations))


    """
    Disposing a struct is just about disposing each of the struct fields one by
    one.
    """
    def build_dispose(self, line_prefix, variable_name):
        built = ''
        for member in self.members:
            built += member['object'].build_dispose(line_prefix, variable_name + '.' + member['name'])
        return built


    """
    Add sections
    """
    def add_sections(self, sections):
        # Add sections for each member
        for member in self.members:
            member['object'].add_sections(sections)

        sections['public-types'] += self.public_format + '\n'
