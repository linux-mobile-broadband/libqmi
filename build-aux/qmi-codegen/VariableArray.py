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
Variable type for Arrays ('array' format)
"""
class VariableArray(Variable):

    def __init__(self, service, dictionary, array_element_type, container_type):

        # Call the parent constructor
        Variable.__init__(self, service, dictionary)

        self.fixed_size = 0
        self.name = dictionary['name']

        # The array and its contents need to get disposed
        self.needs_dispose = True

        # We need to know whether the variable comes in an Input container or in
        # an Output container, as we should not dump the element clear() helper method
        # if the variable is from an Input container.
        self.container_type = container_type

        # Disallow arrays of arrays, always use an intermediate struct
        if dictionary['array-element']['format'] == 'array':
                raise ValueError('Arrays of arrays not allowed in %s array: use an intermediate struct instead' % self.name)

        # Load variable type of this array
        if 'name' in dictionary['array-element']:
            self.array_element = VariableFactory.create_variable(self.service,
                                                                 dictionary['array-element'],
                                                                 array_element_type + ' ' + dictionary['array-element']['name'],
                                                                 self.container_type)
        else:
            self.array_element = VariableFactory.create_variable(self.service,
                                                                 dictionary['array-element'],
                                                                 '',
                                                                 self.container_type)

        if 'size-prefix-format' in dictionary and 'fixed-size' in dictionary:
            raise ValueError('Cannot give \'size-prefix-format\' and \'fixed-size\' in %s array at the same time' % self.name)
        elif 'size-prefix-format' in dictionary:
            # Load variable type for the array size prefix
            # We do NOT allow 64-bit types as array sizes (GArray won't support them)
            if dictionary['size-prefix-format'] not in [ 'guint8', 'guint16', 'guint32' ]:
                raise ValueError('Invalid size prefix format (%s) in %s array: not guint8 or guint16 or guint32' %
                                 (dictionary['size-prefix-format'], self.name))
            default_array_size = { 'format' : dictionary['size-prefix-format'] }
            self.array_size_element = VariableFactory.create_variable(self.service, default_array_size, '', self.container_type)
            # Load variable type for the sequence prefix, if any
            if 'sequence-prefix-format' in dictionary:
                sequence = { 'format' : dictionary['sequence-prefix-format'] }
                self.array_sequence_element = VariableFactory.create_variable(self.service, sequence, '', self.container_type)
            else:
                self.array_sequence_element = ''

        elif 'fixed-size' in dictionary:
            # fixed-size arrays have no size element, obviously
            self.fixed_size = dictionary['fixed-size']
            if int(self.fixed_size) == 0 or int(self.fixed_size) > 512:
                raise ValueError('Fixed array size %s out of bounds (not between 0 and 512)' % self.fixed_size)
            if 'sequence-prefix-format' in dictionary:
                raise ValueError('\'sequence-prefix-format\' is not supported along with \'fixed-size\' in %s array' % self.name)
            else:
                self.array_sequence_element = ''
        else:
            raise ValueError('Missing \'size-prefix-format\' or \'fixed-size\' in %s array' % self.name)

        # Arrays need compat GIR support if the array element needs compat GIR support
        self.needs_compat_gir = self.array_element.needs_compat_gir

        # Arrays contain personal info if the array element contains personal info or they themselves are personal info
        self.contains_personal_info = self.contains_personal_info or self.array_element.contains_personal_info


    def emit_types(self, hfile, cfile, since, static):
        self.array_element.emit_types(hfile, cfile, since, static)


    def emit_types_gir(self, hfile, cfile, since):
        if self.array_element.needs_compat_gir:
            self.array_element.emit_types_gir(hfile, cfile, since)


    def emit_buffer_read(self, f, line_prefix, tlv_out, error, variable_name):
        common_var_prefix = utils.build_underscore_name(self.name)
        translations = { 'lp'                          : line_prefix,
                         'variable_name'               : variable_name,
                         'private_format'              : self.private_format,
                         'array_element_public_format' : self.array_element.public_format,
                         'array_element_clear_method'  : self.array_element.clear_method,
                         'common_var_prefix'           : common_var_prefix }

        template = (
            '${lp}{\n'
            '${lp}    guint ${common_var_prefix}_i;\n')
        f.write(string.Template(template).substitute(translations))

        if self.fixed_size:
            translations['fixed_size'] = self.fixed_size

            template = (
                '${lp}    guint16 ${common_var_prefix}_n_items = ${fixed_size};\n'
                '\n')
            f.write(string.Template(template).substitute(translations))
        else:
            translations['array_size_element_format'] = self.array_size_element.public_format
            template = (
                '${lp}    ${array_size_element_format} ${common_var_prefix}_n_items;\n')

            if self.array_sequence_element != '':
                translations['array_sequence_element_format'] = self.array_sequence_element.public_format
                template += (
                    '${lp}    ${array_sequence_element_format} ${common_var_prefix}_sequence;\n')

            template += (
                '\n'
                '${lp}    /* Read number of items in the array */\n')
            f.write(string.Template(template).substitute(translations))
            self.array_size_element.emit_buffer_read(f, line_prefix + '    ', tlv_out, error, common_var_prefix + '_n_items')

            if self.array_sequence_element != '':
                template = (
                    '\n'
                    '${lp}    /* Read sequence in the array */\n')
                f.write(string.Template(template).substitute(translations))
                self.array_size_element.emit_buffer_read(f, line_prefix + '    ', tlv_out, error, common_var_prefix + '_sequence')

                template = (
                    '\n'
                    '${lp}    ${variable_name}_sequence = ${common_var_prefix}_sequence;\n')
                f.write(string.Template(template).substitute(translations))

        template = (
            '\n'
            '${lp}    ${variable_name} = g_array_sized_new (\n'
            '${lp}        FALSE,\n'
            '${lp}        FALSE,\n'
            '${lp}        sizeof (${array_element_public_format}),\n'
            '${lp}        (guint)${common_var_prefix}_n_items);\n'
            '\n')

        if self.array_element.needs_dispose:
            template += (
                '${lp}    g_array_set_clear_func (${variable_name}, (GDestroyNotify)${array_element_clear_method});\n'
                '\n')

        template += (
            '${lp}    for (${common_var_prefix}_i = 0; ${common_var_prefix}_i < ${common_var_prefix}_n_items; ${common_var_prefix}_i++) {\n'
            '${lp}        ${array_element_public_format} ${common_var_prefix}_aux;\n'
            '\n')
        f.write(string.Template(template).substitute(translations))

        self.array_element.emit_buffer_read(f, line_prefix + '        ', tlv_out, error, common_var_prefix + '_aux')

        template = (
            '${lp}        g_array_insert_val (${variable_name}, ${common_var_prefix}_i, ${common_var_prefix}_aux);\n'
            '${lp}    }\n'
            '${lp}}\n')
        f.write(string.Template(template).substitute(translations))


    def emit_buffer_write(self, f, line_prefix, tlv_name, variable_name):
        common_var_prefix = utils.build_underscore_name(self.name)
        translations = { 'lp'                : line_prefix,
                         'variable_name'     : variable_name,
                         'common_var_prefix' : common_var_prefix }

        template = (
            '${lp}{\n'
            '${lp}    guint ${common_var_prefix}_i;\n')
        f.write(string.Template(template).substitute(translations))

        if self.fixed_size == 0:
            translations['array_size_element_format'] = self.array_size_element.private_format

            template = (
                '${lp}    ${array_size_element_format} ${common_var_prefix}_n_items;\n'
                '\n'
                '${lp}    /* Write the number of items in the array first */\n'
                '${lp}    ${common_var_prefix}_n_items = (${array_size_element_format}) ${variable_name}->len;\n')
            f.write(string.Template(template).substitute(translations))

            self.array_size_element.emit_buffer_write(f, line_prefix + '    ', tlv_name, common_var_prefix + '_n_items')

        if self.array_sequence_element != '':
            self.array_sequence_element.emit_buffer_write(f, line_prefix + '    ', tlv_name, variable_name + '_sequence')


        template = (
            '\n'
            '${lp}    for (${common_var_prefix}_i = 0; ${common_var_prefix}_i < ${variable_name}->len; ${common_var_prefix}_i++) {\n')
        f.write(string.Template(template).substitute(translations))

        self.array_element.emit_buffer_write(f, line_prefix + '        ', tlv_name, 'g_array_index (' + variable_name + ', ' + self.array_element.public_format + ',' + common_var_prefix + '_i)')

        template = (
            '${lp}    }\n'
            '${lp}}\n')
        f.write(string.Template(template).substitute(translations))


    def emit_get_printable(self, f, line_prefix, is_personal):
        common_var_prefix = utils.build_underscore_name(self.name)
        translations = { 'lp'                : line_prefix,
                         'common_var_prefix' : common_var_prefix }

        template = (
            '${lp}{\n'
            '${lp}    guint ${common_var_prefix}_i;\n')
        f.write(string.Template(template).substitute(translations))

        if self.fixed_size:
            translations['fixed_size'] = self.fixed_size

            template = (
                '${lp}    guint16 ${common_var_prefix}_n_items = ${fixed_size};\n'
                '\n')
            f.write(string.Template(template).substitute(translations))
        else:
            translations['array_size_element_format'] = self.array_size_element.public_format
            template = (
                '${lp}    ${array_size_element_format} ${common_var_prefix}_n_items;\n')

            if self.array_sequence_element != '':
                translations['array_sequence_element_format'] = self.array_sequence_element.public_format
                template += (
                    '${lp}    ${array_sequence_element_format} ${common_var_prefix}_sequence;\n')

            template += (
                '\n'
                '${lp}    /* Read number of items in the array */\n')
            f.write(string.Template(template).substitute(translations))
            self.array_size_element.emit_buffer_read(f, line_prefix + '    ', 'out', '&error', common_var_prefix + '_n_items')

            if self.array_sequence_element != '':
                template = (
                    '\n'
                    '${lp}    /* Read sequence */\n')
                f.write(string.Template(template).substitute(translations))
                self.array_sequence_element.emit_buffer_read(f, line_prefix + '    ', 'out', '&error', common_var_prefix + '_sequence')
                template = (
                    '\n'
                    '${lp}    g_string_append_printf (printable, "[[Seq:%u]] ", ${common_var_prefix}_sequence);\n')
                f.write(string.Template(template).substitute(translations))

        template = (
            '\n'
            '${lp}    g_string_append (printable, "{");\n'
            '\n'
            '${lp}    for (${common_var_prefix}_i = 0; ${common_var_prefix}_i < ${common_var_prefix}_n_items; ${common_var_prefix}_i++) {\n'
            '${lp}        g_string_append_printf (printable, " [%u] = \'", ${common_var_prefix}_i);\n')
        f.write(string.Template(template).substitute(translations))

        self.array_element.emit_get_printable(f, line_prefix + '        ', self.personal_info or is_personal);

        template = (
            '${lp}        g_string_append (printable, "\'");\n'
            '${lp}    }\n'
            '\n'
            '${lp}    g_string_append (printable, "}");\n'
            '${lp}}')
        f.write(string.Template(template).substitute(translations))


    """
    We need to include SEQUENCE + GARRAY
    """
    def build_variable_declaration(self, line_prefix, variable_name):
        translations = { 'lp'   : line_prefix,
                         'name' : variable_name }

        template = ''
        if self.array_sequence_element != '':
            translations['array_sequence_element_format'] = self.array_sequence_element.public_format
            template += (
                '${lp}${array_sequence_element_format} ${name}_sequence;\n')

        template += (
            '${lp}GArray *${name};\n')
        return string.Template(template).substitute(translations)


    """
    We need to include GPTRARRAY
    """
    def build_variable_declaration_gir(self, line_prefix, variable_name):
        if not self.array_element.needs_compat_gir:
            return ''

        translations = { 'lp'   : line_prefix,
                         'name' : variable_name }

        template = (
            '${lp}GPtrArray *${name}_ptr;\n')
        return string.Template(template).substitute(translations)


    """
    We need to include SEQUENCE + GARRAY
    """
    def build_struct_field_declaration(self, line_prefix, variable_name):
        return self.build_variable_declaration(line_prefix, variable_name)


    """
    We need to include SEQUENCE + GPTRARRAY
    """
    def build_struct_field_declaration_gir(self, line_prefix, variable_name):
        if not self.array_element.needs_compat_gir:
            return self.build_struct_field_declaration(line_prefix, variable_name)

        translations = { 'lp'   : line_prefix,
                         'name' : variable_name }

        template = ''

        if self.array_sequence_element != '':
            translations['array_sequence_element_format'] = self.array_sequence_element.public_format
            template += (
                '${lp}${array_sequence_element_format} ${name}_sequence;\n')

        template += (
            '${lp}GPtrArray *${name};\n')
        return string.Template(template).substitute(translations)


    def build_getter_declaration(self, line_prefix, variable_name):
        if not self.visible:
            return ''

        translations = { 'lp'   : line_prefix,
                         'name' : variable_name }

        template = ''
        if self.array_sequence_element != '':
            translations['array_sequence_element_format'] = self.array_sequence_element.public_format
            template += (
                '${lp}${array_sequence_element_format} *${name}_sequence,\n')

        template += (
            '${lp}GArray **${name},\n')
        return string.Template(template).substitute(translations)


    def build_getter_declaration_gir(self, line_prefix, variable_name):
        if not self.array_element.needs_compat_gir:
            return self.build_getter_declaration(line_prefix, variable_name)

        if not self.visible:
            return ''

        translations = { 'lp'   : line_prefix,
                         'name' : variable_name }

        template = ''
        if self.array_sequence_element != '':
            translations['array_sequence_element_format'] = self.array_sequence_element.public_format
            template += (
                '${lp}${array_sequence_element_format} *${name}_sequence,\n')

        template += (
            '${lp}GPtrArray **${name}_ptr,\n')
        return string.Template(template).substitute(translations)


    def build_getter_documentation(self, line_prefix, variable_name):
        if not self.visible:
            return ''

        translations = { 'lp'                          : line_prefix,
                         'array_element_public_format' : self.array_element.public_format,
                         'array_element_element_type'  : self.array_element.element_type,
                         'name'                        : variable_name }

        template = ''
        if self.array_sequence_element != '':
            template += (
                '${lp}@${name}_sequence: (out)(optional): a placeholder for the output sequence number, or %NULL if not required.\n')

        template += (
            '${lp}@${name}: (out)(optional)(element-type ${array_element_element_type})(transfer none): a placeholder for the output #GArray of #${array_element_public_format} elements, or %NULL if not required. Do not free it, it is owned by @self.\n')
        return string.Template(template).substitute(translations)


    def build_getter_documentation_gir(self, line_prefix, variable_name):
        if not self.array_element.needs_compat_gir:
            return self.build_getter_documentation(line_prefix, variable_name)
        if not self.visible:
            return ''

        translations = { 'lp'                             : line_prefix,
                         'array_element_public_format'    : self.array_element.public_format,
                         'array_element_element_type_gir' : self.array_element.element_type_gir,
                         'name'                           : variable_name }

        template = ''
        if self.array_sequence_element != '':
            template += (
                '${lp}@${name}_sequence: (out)(optional): a placeholder for the output sequence number, or %NULL if not required.\n')

        template += (
            '${lp}@${name}_ptr: (out)(optional)(element-type ${array_element_element_type_gir})(transfer none): a placeholder for the output array of #${array_element_public_format} elements, or %NULL if not required. Do not free or modify it, it is owned by @self.\n')
        return string.Template(template).substitute(translations)


    def build_getter_implementation(self, line_prefix, variable_name_from, variable_name_to):
        if not self.visible:
            return ''

        translations = { 'lp'   : line_prefix,
                         'from' : variable_name_from,
                         'to'   : variable_name_to }

        template = ''
        if self.array_sequence_element != '':
            template += (
                '${lp}if (${to}_sequence)\n'
                '${lp}    *${to}_sequence = ${from}_sequence;\n')

        template += (
            '${lp}if (${to})\n'
            '${lp}    *${to} = ${from};\n')

        return string.Template(template).substitute(translations)


    def build_getter_implementation_gir(self, line_prefix, variable_name_from, variable_name_to):
        if not self.array_element.needs_compat_gir:
            return self.build_getter_implementation(line_prefix, variable_name_from, variable_name_to)
        if not self.visible:
            return ''

        common_var_prefix = utils.build_underscore_name(self.name)
        translations = { 'lp'   : line_prefix,
                         'from' : variable_name_from,
                         'to'   : variable_name_to }

        template = ''
        if self.array_sequence_element != '':
            template += (
                '${lp}if (${to}_sequence)\n'
                '${lp}    *${to}_sequence = ${from}_sequence;\n')

        template += (
            '${lp}if (${to}_ptr) {\n'
            '${lp}    if (!${from}_ptr) {\n')

        template += self.build_copy_to_gir(line_prefix + '        ',
                                           variable_name_from,
                                           variable_name_from + '_ptr')

        template += (
            '${lp}    }\n'
            '${lp}    *${to}_ptr = ${from}_ptr;\n'
            '${lp}}')

        return string.Template(template).substitute(translations)


    def build_setter_declaration(self, line_prefix, variable_name):
        if not self.visible:
            return ''

        translations = { 'lp'   : line_prefix,
                         'name' : variable_name }

        template = ''
        if self.array_sequence_element != '':
            translations['array_sequence_element_format'] = self.array_sequence_element.public_format
            template += (
                '${lp}${array_sequence_element_format} ${name}_sequence,\n')

        template += (
            '${lp}GArray *${name},\n')
        return string.Template(template).substitute(translations)


    def build_setter_declaration_gir(self, line_prefix, variable_name):
        if not self.array_element.needs_compat_gir:
            return self.build_setter_declaration(line_prefix, variable_name)
        if not self.visible:
            return ''

        translations = { 'lp'   : line_prefix,
                         'name' : variable_name }

        template = ''
        if self.array_sequence_element != '':
            translations['array_sequence_element_format'] = self.array_sequence_element.public_format
            template += (
                '${lp}${array_sequence_element_format} ${name}_sequence,\n')

        template += (
            '${lp}GPtrArray *${name}_ptr,\n')
        return string.Template(template).substitute(translations)


    def build_setter_documentation(self, line_prefix, variable_name):
        if not self.visible:
            return ""

        translations = { 'lp'                          : line_prefix,
                         'array_element_public_format' : self.array_element.public_format,
                         'array_element_element_type'  : self.array_element.element_type,
                         'name'                        : variable_name }

        template = ''
        if self.array_sequence_element != '':
            template += (
                '${lp}@${name}_sequence: the sequence number.\n')

        template += (
            '${lp}@${name}: (in)(element-type ${array_element_element_type})(transfer none): a #GArray of #${array_element_public_format} elements. A new reference to @${name} will be taken, so the caller must make sure the array was created with the correct #GDestroyNotify as clear function for each element in the array.\n')
        return string.Template(template).substitute(translations)


    def build_setter_documentation_gir(self, line_prefix, variable_name):
        if not self.array_element.needs_compat_gir:
            return self.build_setter_documentation(line_prefix, variable_name)
        if not self.visible:
            return ""

        translations = { 'lp'                             : line_prefix,
                         'array_element_public_format'    : self.array_element.public_format,
                         'array_element_element_type_gir' : self.array_element.element_type_gir,
                         'name'                           : variable_name }

        template = ''
        if self.array_sequence_element != '':
            template += (
                '${lp}@${name}_sequence: the sequence number.\n')

        template += (
            '${lp}@${name}_ptr: (in)(element-type ${array_element_element_type_gir})(transfer none): array of #${array_element_public_format} elements. The contents of the given array will be copied, the #GPtrArray will not increase its reference count.\n')
        return string.Template(template).substitute(translations)


    def build_setter_implementation(self, line_prefix, variable_name_from, variable_name_to):
        if not self.visible:
            return ""

        translations = { 'lp'   : line_prefix,
                         'from' : variable_name_from,
                         'to'   : variable_name_to }

        template = ''
        if self.array_sequence_element != '':
            template += (
                '${lp}${to}_sequence = ${from}_sequence;\n')

        template += self.build_dispose(line_prefix, variable_name_to)
        template += self.build_dispose_gir(line_prefix, variable_name_to)

        template +=(
            '${lp}${to} = g_array_ref (${from});\n')
        return string.Template(template).substitute(translations)


    def build_setter_implementation_gir(self, line_prefix, variable_name_from, variable_name_to):
        if not self.array_element.needs_compat_gir:
            return self.build_setter_implementation(line_prefix, variable_name_from, variable_name_to)
        if not self.visible:
            return ""

        common_var_prefix = utils.build_underscore_name(self.name)
        translations = { 'lp'   : line_prefix,
                         'from' : variable_name_from,
                         'to'   : variable_name_to }

        template = ''
        if self.array_sequence_element != '':
            template += (
                '${lp}${to}_sequence = ${from}_sequence;\n')

        template += self.build_dispose(line_prefix, variable_name_to)
        template += self.build_dispose_gir(line_prefix, variable_name_to + '_ptr')
        template += self.build_copy_from_gir(line_prefix, variable_name_from + '_ptr', variable_name_to)

        return string.Template(template).substitute(translations)


    def build_struct_field_documentation(self, line_prefix, variable_name):
        translations = { 'lp'                          : line_prefix,
                         'array_element_public_format' : self.array_element.public_format,
                         'name'                        : variable_name }

        template = ''
        if self.array_sequence_element != '':
            template += (
                '${lp}@${name}_sequence: the sequence number.\n')

        template += (
            '${lp}@${name}: a #GArray of #${array_element_public_format} elements.\n')
        return string.Template(template).substitute(translations)


    def build_struct_field_documentation_gir(self, line_prefix, variable_name):
        if not self.array_element.needs_compat_gir:
            return self.build_struct_field_documentation(line_prefix, variable_name)

        translations = { 'lp'                          : line_prefix,
                         'array_element_public_format' : self.array_element.public_format,
                         'array_element_element_type'  : self.array_element.element_type,
                         'name'                        : variable_name }

        template = ''
        if self.array_sequence_element != '':
            template += (
                '${lp}@${name}_sequence: the sequence number.\n')

        template += (
            '${lp}@${name}: (element-type ${array_element_element_type}): an array of #${array_element_public_format} elements.\n')
        return string.Template(template).substitute(translations)


    def build_dispose(self, line_prefix, variable_name):
        translations = { 'lp'            : line_prefix,
                         'variable_name' : variable_name }

        template = (
            '${lp}g_clear_pointer (&${variable_name}, (GDestroyNotify)g_array_unref);\n')
        return string.Template(template).substitute(translations)


    def build_dispose_gir(self, line_prefix, variable_name):
        if not self.array_element.needs_compat_gir:
            self.build_dispose(line_prefix, variable_name)

        translations = { 'lp'            : line_prefix,
                         'variable_name' : variable_name }

        template = (
            '${lp}g_clear_pointer (&${variable_name}, (GDestroyNotify)g_ptr_array_unref);\n')
        return string.Template(template).substitute(translations)


    def build_copy_to_gir(self, line_prefix, variable_name_from, variable_name_to):
        common_var_prefix = utils.build_underscore_name(self.name)
        translations = { 'lp'   : line_prefix,
                         'to'   : variable_name_to,
                         'from' : variable_name_from }

        if self.array_element.needs_compat_gir:

            translations['common_var_prefix']               = common_var_prefix
            translations['array_element_public_format']     = self.array_element.public_format
            translations['array_element_public_format_gir'] = self.array_element.public_format_gir
            translations['array_element_element_type_gir']  = self.array_element.element_type_gir
            translations['array_element_free_method_gir']   = self.array_element.free_method_gir
            translations['array_element_new_method_gir']    = self.array_element.new_method_gir

            template = (
                '${lp}{\n'
                '${lp}    guint ${common_var_prefix}_i;\n'
                '\n'
                '${lp}    ${to} = g_ptr_array_new_full (${from}->len, (GDestroyNotify)${array_element_free_method_gir});\n'
                '${lp}    for (${common_var_prefix}_i = 0; ${common_var_prefix}_i < ${from}->len; ${common_var_prefix}_i++) {\n'
                '${lp}        ${array_element_public_format} *${common_var_prefix}_aux_from;\n'
                '${lp}        ${array_element_public_format_gir} *${common_var_prefix}_aux_to;\n'
                '\n'
                '${lp}        ${common_var_prefix}_aux_from = &g_array_index (${from}, ${array_element_public_format}, ${common_var_prefix}_i);\n'
                '\n'
                '${lp}        ${common_var_prefix}_aux_to = ${array_element_new_method_gir} ();\n')

            template += self.array_element.build_copy_to_gir(line_prefix + '        ',
                                                             '(*${common_var_prefix}_aux_from)',
                                                             '${common_var_prefix}_aux_to')

            template += (
                '\n'
                '${lp}        g_ptr_array_add (${to}, ${common_var_prefix}_aux_to);\n'
                '${lp}    }\n'
                '${lp}}\n')
        else:
            template = (
                '${lp}${to} = g_array_ref (${from});\n')
        return string.Template(template).substitute(translations)


    def build_copy_from_gir(self, line_prefix, variable_name_from, variable_name_to):
        translations = { 'lp'   : line_prefix,
                         'to'   : variable_name_to,
                         'from' : variable_name_from }

        if self.array_element.needs_compat_gir:
            common_var_prefix = utils.build_underscore_name(self.name)
            translations['common_var_prefix']               = common_var_prefix
            translations['array_element_public_format']     = self.array_element.public_format
            translations['array_element_public_format_gir'] = self.array_element.public_format_gir

            template = (
                '${lp}{\n'
                '${lp}    guint ${common_var_prefix}_i;\n'
                '\n'
                '${lp}    ${to} = g_array_sized_new (FALSE, FALSE, sizeof (${array_element_public_format}), ${from}->len);\n'
                '${lp}    for (${common_var_prefix}_i = 0; ${common_var_prefix}_i < ${from}->len; ${common_var_prefix}_i++) {\n'
                '${lp}        ${array_element_public_format} ${common_var_prefix}_aux_to;\n'
                '${lp}        ${array_element_public_format_gir} *${common_var_prefix}_aux_from;\n'
                '\n'
                '${lp}        ${common_var_prefix}_aux_from = g_ptr_array_index (${from}, ${common_var_prefix}_i);\n'
                '\n')

            template += self.array_element.build_copy_from_gir(line_prefix + '        ',
                                                               '${common_var_prefix}_aux_from',
                                                               '${common_var_prefix}_aux_to')

            template += (
                '\n'
                '${lp}        g_array_append_val (${to}, ${common_var_prefix}_aux_to);\n'
                '${lp}    }\n'
                '${lp}}\n')
        else:
            template = (
                '${lp}${to} = g_array_ref (${from});\n')
        return string.Template(template).substitute(translations)


    def build_copy_gir(self, line_prefix, variable_name_from, variable_name_to):
        common_var_prefix = utils.build_underscore_name(self.name)
        translations = { 'lp'   : line_prefix,
                         'to'   : variable_name_to,
                         'from' : variable_name_from }

        # our GPtrArrays and GArrays are immutable, we can just ref the array as a copy
        if self.array_element.needs_compat_gir:
            template = (
                '${lp}${to} = g_ptr_array_ref (${from});\n')
        else:
            template = (
                '${lp}${to} = g_array_ref (${from});\n')
        return string.Template(template).substitute(translations)


    def add_sections(self, sections):
        self.array_element.add_sections(sections)
