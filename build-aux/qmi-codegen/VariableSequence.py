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
# Copyright (c) 2022 Qualcomm Innovation Center, Inc.
#

import string
import utils
from Variable import Variable
import VariableFactory

"""
Variable type for Sequences ('sequence' format)
"""
class VariableSequence(Variable):

    def __init__(self, service, dictionary, sequence_type_name, container_type):

        # Call the parent constructor
        Variable.__init__(self, service, dictionary)

        self.container_type = container_type

        # Load members of this sequence
        self.members = []
        for member_dictionary in dictionary['contents']:
            member = {}
            member['name'] = utils.build_underscore_name(member_dictionary['name'])
            member['object'] = VariableFactory.create_variable(self.service, member_dictionary, sequence_type_name + ' ' + member_dictionary['name'], self.container_type)
            self.members.append(member)

        # We'll need to dispose if at least one of the members needs it
        for member in self.members:
            if member['object'].needs_dispose:
                self.needs_dispose = True
                break

        # We'll flag the variable as needing compat GIR methods if we find any
        # sequence member needing it
        for member in self.members:
            if member['object'].needs_compat_gir:
                self.needs_compat_gir = True
                break

        # We'll contain personal info if at least one of the members contains personal info or we ourselves are personal info
        if not self.contains_personal_info:
            for member in self.members:
                if member['object'].contains_personal_info:
                    self.contains_personal_info = True
                    break


    def emit_types(self, hfile, cfile, since, static):
        for member in self.members:
            member['object'].emit_types(hfile, cfile, since, static)


    def emit_types_gir(self, hfile, cfile, since):
        for member in self.members:
            member['object'].emit_types_gir(hfile, cfile, since)


    def emit_buffer_read(self, f, line_prefix, tlv_out, error, variable_name):
        for member in self.members:
            member['object'].emit_buffer_read(f, line_prefix, tlv_out, error, variable_name + '_' +  member['name'])


    def emit_buffer_write(self, f, line_prefix, tlv_name, variable_name):
        for member in self.members:
            member['object'].emit_buffer_write(f, line_prefix, tlv_name, variable_name + '_' +  member['name'])


    def emit_get_printable(self, f, line_prefix, is_personal):
        translations = { 'lp' : line_prefix }

        template = (
            '${lp}g_string_append (printable, "[");\n')
        f.write(string.Template(template).substitute(translations))

        for member in self.members:
            translations['variable_name'] = member['name']
            template = (
                '${lp}g_string_append (printable, " ${variable_name} = \'");\n')
            f.write(string.Template(template).substitute(translations))

            member['object'].emit_get_printable(f, line_prefix, self.personal_info or is_personal)

            template = (
                '${lp}g_string_append (printable, "\'");\n')
            f.write(string.Template(template).substitute(translations))

        template = (
            '${lp}g_string_append (printable, " ]");\n')
        f.write(string.Template(template).substitute(translations))


    def build_variable_declaration(self, line_prefix, variable_name):
        built = ''
        for member in self.members:
            built += member['object'].build_variable_declaration(line_prefix, variable_name + '_' + member['name'])
        return built


    def build_variable_declaration_gir(self, line_prefix, variable_name):
        built = ''
        for member in self.members:
            built += member['object'].build_variable_declaration_gir(line_prefix, variable_name + '_' + member['name'])
        return built


    def build_struct_field_declaration(self, line_prefix, variable_name):
        raise RuntimeError('Variable of type "sequence" is never expected as a struct field')


    def build_getter_declaration(self, line_prefix, variable_name):
        if not self.visible:
            return ""

        built = ''
        for member in self.members:
            built += member['object'].build_getter_declaration(line_prefix, variable_name + '_' + member['name'])
        return built


    def build_getter_declaration_gir(self, line_prefix, variable_name):
        if not self.visible:
            return ""

        built = ''
        for member in self.members:
            built += member['object'].build_getter_declaration_gir(line_prefix, variable_name + '_' + member['name'])
        return built


    def build_getter_documentation(self, line_prefix, variable_name):
        if not self.visible:
            return ""

        built = ''
        for member in self.members:
            built += member['object'].build_getter_documentation(line_prefix, variable_name + '_' + member['name'])
        return built


    def build_getter_documentation_gir(self, line_prefix, variable_name):
        if not self.visible:
            return ""

        built = ''
        for member in self.members:
            built += member['object'].build_getter_documentation_gir(line_prefix, variable_name + '_' + member['name'])
        return built


    def build_getter_implementation(self, line_prefix, variable_name_from, variable_name_to):
        if not self.visible:
            return ""

        built = ''
        for member in self.members:
            built += member['object'].build_getter_implementation(line_prefix,
                                                                  variable_name_from + '_' + member['name'],
                                                                  variable_name_to + '_' + member['name'])
        return built


    def build_getter_implementation_gir(self, line_prefix, variable_name_from, variable_name_to):
        if not self.visible:
            return ""

        built = ''
        for member in self.members:
            built += member['object'].build_getter_implementation_gir(line_prefix,
                                                                      variable_name_from + '_' + member['name'],
                                                                      variable_name_to + '_' + member['name'])
        return built


    def build_setter_declaration(self, line_prefix, variable_name):
        if not self.visible:
            return ""

        built = ''
        for member in self.members:
            built += member['object'].build_setter_declaration(line_prefix, variable_name + '_' + member['name'])
        return built


    def build_setter_declaration_gir(self, line_prefix, variable_name):
        if not self.visible:
            return ""

        built = ''
        for member in self.members:
            built += member['object'].build_setter_declaration_gir(line_prefix, variable_name + '_' + member['name'])
        return built


    def build_setter_documentation(self, line_prefix, variable_name):
        if not self.visible:
            return ""

        built = ''
        for member in self.members:
            built += member['object'].build_setter_documentation(line_prefix, variable_name + '_' + member['name'])
        return built


    def build_setter_documentation_gir(self, line_prefix, variable_name):
        if not self.visible:
            return ""

        built = ''
        for member in self.members:
            built += member['object'].build_setter_documentation_gir(line_prefix, variable_name + '_' + member['name'])
        return built


    def build_setter_implementation(self, line_prefix, variable_name_from, variable_name_to):
        if not self.visible:
            return ""

        built = ''
        for member in self.members:
            built += member['object'].build_setter_implementation(line_prefix,
                                                                  variable_name_from + '_' + member['name'],
                                                                  variable_name_to + '_' + member['name'])
        return built


    def build_setter_implementation_gir(self, line_prefix, variable_name_from, variable_name_to):
        if not self.visible:
            return ""

        built = ''
        for member in self.members:
            built += member['object'].build_setter_implementation_gir(line_prefix,
                                                                      variable_name_from + '_' + member['name'],
                                                                      variable_name_to + '_' + member['name'])
        return built


    def build_dispose(self, line_prefix, variable_name):
        built = ''
        for member in self.members:
            built += member['object'].build_dispose(line_prefix, variable_name + '_' + member['name'])
        return built


    def build_dispose_gir(self, line_prefix, variable_name):
        built = ''
        for member in self.members:
            built += member['object'].build_dispose_gir(line_prefix, variable_name + '_' + member['name'])
        return built


    def build_copy_to_gir(self, line_prefix, variable_name_from, variable_name_to):
        built = ''
        for member in self.members:
            built += member['object'].build_copy_to_gir(line_prefix, variable_name_from, variable_name_to)
        return built


    def build_copy_from_gir(self, line_prefix, variable_name_from, variable_name_to):
        built = ''
        for member in self.members:
            built += member['object'].build_copy_from_gir(line_prefix, variable_name_from, variable_name_to)
        return built


    def build_copy_gir(self, line_prefix, variable_name_from, variable_name_to):
        built = ''
        for member in self.members:
            built += member['object'].build_copy_gir(line_prefix, variable_name_from, variable_name_to)
        return built


    def add_sections(self, sections):
        # Add sections for each member
        for member in self.members:
            member['object'].add_sections(sections)
