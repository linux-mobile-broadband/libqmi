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

    def __init__(self, service, dictionary, struct_type_name, container_type):

        # Call the parent constructor
        Variable.__init__(self, service, dictionary)

        self.container_type = container_type

        # The public format of the struct is built directly from the suggested
        # struct type name
        self.struct_type_name = struct_type_name
        self.element_type = utils.build_camelcase_name(self.struct_type_name)
        self.public_format = self.element_type
        self.private_format = self.public_format

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
            if member['object'].needs_dispose:
                self.needs_dispose = True
                break

        if self.needs_dispose:
            self.clear_method = '__' + utils.build_underscore_name(self.struct_type_name) + '_clear'


        # When using structs, we always require GIR compat methods because they're going
        # to be stored via reference pointers in the arrays
        self.needs_compat_gir = True

        # If any of the contents needs GIR compat methods, then we must also create a new
        # GIR compat struct type; otherwise just use the original one
        self.content_needs_compat_gir = False
        for member in self.members:
            if member['object'].needs_compat_gir:
                self.content_needs_compat_gir = True
                break

        if self.content_needs_compat_gir:
            self.struct_type_name_gir = self.struct_type_name + ' Gir'
        else:
            self.struct_type_name_gir = self.struct_type_name
        self.element_type_gir = utils.build_camelcase_name(self.struct_type_name_gir)
        self.public_format_gir = self.element_type_gir
        self.private_format_gir = self.public_format_gir
        self.new_method_gir = '__' + utils.build_underscore_name(self.struct_type_name_gir) + '_new'
        self.free_method_gir = '__' + utils.build_underscore_name(self.struct_type_name_gir) + '_free'


    def build_variable_declaration(self, line_prefix, variable_name):
        raise RuntimeError('Variable of type "struct" can only be defined as array members')


    def build_struct_field_declaration(self, line_prefix, variable_name):
        raise RuntimeError('Variable of type "struct" cannot be defined as struct fields')


    def emit_types(self, hfile, cfile, since, static):
        for member in self.members:
            member['object'].emit_types(hfile, cfile, since, static)

        translations = { 'format' : self.public_format,
                         'since'  : since }
        template = '\n'
        hfile.write(string.Template(template).substitute(translations))

        if not static and self.service != 'CTL':
            template = (
                '\n'
                '/**\n')
            if self.content_needs_compat_gir:
                template += (
                    ' * ${format}: (skip)\n')
            else:
                template += (
                    ' * ${format}:\n')
            hfile.write(string.Template(template).substitute(translations))
            for member in self.members:
                hfile.write(member['object'].build_struct_field_documentation(' * ', member['name']))
            template = (
                ' *\n'
                ' * A ${format} struct.\n'
                ' *\n'
                ' * Since: ${since}\n'
                ' */\n')
            hfile.write(string.Template(template).substitute(translations))

        template = (
            'typedef struct _${format} {\n')
        hfile.write(string.Template(template).substitute(translations))

        for member in self.members:
            hfile.write(member['object'].build_struct_field_declaration('    ', member['name']))

        template = ('} ${format};\n')
        hfile.write(string.Template(template).substitute(translations))

        # No need for the clear func if no need to dispose the contents
        if self.needs_dispose:
            translations['clear_method'] = self.clear_method
            template = (
                '\n'
                'static void\n'
                '${clear_method} (${format} *value)\n'
                '{\n')
            for member in self.members:
                template += member['object'].build_dispose('    ', 'value->' + member['name'])
            template += (
                '}\n')
            cfile.write(string.Template(template).substitute(translations))


    def emit_types_gir(self, hfile, cfile, since):
        for member in self.members:
            member['object'].emit_types_gir(hfile, cfile, since)

        translations = { 'element_type_no_gir' : self.public_format,
                         'element_type'        : self.element_type_gir,
                         'underscore'          : utils.build_underscore_name(self.struct_type_name_gir),
                         'new_method'          : self.new_method_gir,
                         'free_method'         : self.free_method_gir,
                         'since'               : since if utils.version_compare('1.32',since) > 0 else '1.32' }

        # The type emission should happen ONLY if the GIR required type is
        # different to the original one
        if self.content_needs_compat_gir:
            template = (
                '\n'
                '/**\n'
                ' * ${element_type}: (rename-to ${element_type_no_gir})\n')
            hfile.write(string.Template(template).substitute(translations))
            for member in self.members:
                hfile.write(member['object'].build_struct_field_documentation_gir(' * ', member['name']))
            template = (
                ' *\n'
                ' * A ${element_type} struct.\n'
                ' *\n'
                ' * This type is a version of #${element_type_no_gir}, using arrays of pointers to\n'
                ' * structs instead of arrays of structs, for easier binding in other languages.\n'
                ' *\n'
                ' * Since: ${since}\n'
                ' */\n')
            hfile.write(string.Template(template).substitute(translations))

            template = (
                'typedef struct _${element_type} {\n')
            hfile.write(string.Template(template).substitute(translations))
            for member in self.members:
                hfile.write(member['object'].build_struct_field_declaration_gir('    ', member['name']))
                template = (
                    '} ${element_type};\n')
            hfile.write(string.Template(template).substitute(translations))

        # Free and New methods are required always for the GIR required type,
        # regardless of whether it's different to the original one or the same
        template = (
            '\n'
            'static void\n'
            '${free_method} (${element_type} *value)\n'
            '{\n')
        if not self.content_needs_compat_gir and self.needs_dispose:
            translations['clear_method'] = self.clear_method
            template += '    ${clear_method} (value);\n'
        else:
            for member in self.members:
                template += member['object'].build_dispose_gir('    ', 'value->' + member['name'])
        template += (
            '    g_slice_free (${element_type}, value);\n'
            '}\n'
            '\n'
            'static ${element_type} *\n'
            '${new_method} (void)\n'
            '{\n'
            '    return g_slice_new0 (${element_type});\n'
            '}\n')
        cfile.write(string.Template(template).substitute(translations))

        template = (
            '\n'
            'GType ${underscore}_get_type (void) G_GNUC_CONST;\n')
        hfile.write(string.Template(template).substitute(translations))

        template = (
            '\n'
            'static ${element_type} *\n'
            '__${underscore}_copy (const ${element_type} *value)\n'
            '{\n'
            '    ${element_type} *copy;\n'
            '\n'
            '    copy = ${new_method} ();\n')
        template += self.build_copy_gir('    ', 'value', 'copy')
        template += (
            '    return copy;\n'
            '}\n'
            '\n'
            'G_DEFINE_BOXED_TYPE (${element_type}, ${underscore}, (GBoxedCopyFunc)__${underscore}_copy, (GBoxedFreeFunc)__${underscore}_free)\n')
        cfile.write(string.Template(template).substitute(translations))


    def emit_buffer_read(self, f, line_prefix, tlv_out, error, variable_name):
        for member in self.members:
            member['object'].emit_buffer_read(f, line_prefix, tlv_out, error, variable_name + '.' +  member['name'])


    def emit_buffer_write(self, f, line_prefix, tlv_name, variable_name):
        for member in self.members:
            member['object'].emit_buffer_write(f, line_prefix, tlv_name, variable_name + '.' +  member['name'])


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


    def build_dispose(self, line_prefix, variable_name):
        translations = { 'lp'            : line_prefix,
                         'underscore'    : utils.build_underscore_name(self.struct_type_name),
                         'variable_name' : variable_name }

        template = '${lp}${underscore}_clear (&${variable_name});\n'
        return string.Template(template).substitute(translations)


    def build_dispose_gir(self, line_prefix, variable_name):
        translations = { 'lp'            : line_prefix,
                         'underscore'    : utils.build_underscore_name(self.struct_type_name_gir),
                         'variable_name' : variable_name }

        template = '${lp}${underscore}_free (${variable_name});\n'
        return string.Template(template).substitute(translations)


    def build_copy_to_gir(self, line_prefix, variable_name_from, variable_name_to):
        translations = { 'lp'                 : line_prefix,
                         'element_type_gir'   : self.element_type_gir,
                         'variable_name_to'   : variable_name_to,
                         'variable_name_from' : variable_name_from }

        template = ''
        for member in self.members:
            template += member['object'].build_copy_to_gir(line_prefix,
                                                           '${variable_name_from}.' + member['name'],
                                                           '${variable_name_to}->' + member['name'])
        return string.Template(template).substitute(translations)


    def build_copy_from_gir(self, line_prefix, variable_name_from, variable_name_to):
        translations = { 'lp'                 : line_prefix,
                         'element_type_gir'   : self.element_type_gir,
                         'variable_name_to'   : variable_name_to,
                         'variable_name_from' : variable_name_from }

        template = ''
        for member in self.members:
            template += member['object'].build_copy_from_gir(line_prefix,
                                                             '${variable_name_from}->' + member['name'],
                                                             '${variable_name_to}.' + member['name'])
        return string.Template(template).substitute(translations)


    def build_copy_gir(self, line_prefix, variable_name_from, variable_name_to):
        translations = { 'lp'                 : line_prefix,
                         'element_type_gir'   : self.element_type_gir,
                         'variable_name_to'   : variable_name_to,
                         'variable_name_from' : variable_name_from }

        template = ''
        for member in self.members:
            template += member['object'].build_copy_gir(line_prefix,
                                                        '${variable_name_from}->' + member['name'],
                                                        '${variable_name_to}->' + member['name'])
        return string.Template(template).substitute(translations)


    def add_sections(self, sections):
        for member in self.members:
            member['object'].add_sections(sections)

        sections['public-types'] += self.element_type + '\n'
        sections['standard'] += utils.build_underscore_name(self.struct_type_name) + '_get_type\n'
        if self.content_needs_compat_gir:
            sections['public-types'] += self.element_type_gir + '\n'
            sections['standard'] += utils.build_underscore_name(self.struct_type_name_gir) + '_get_type\n'
