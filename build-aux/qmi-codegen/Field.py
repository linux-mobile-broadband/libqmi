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
# Copyright (C) 2012 Lanedo GmbH
#

import string

from Struct import Struct
import utils

class Field:
    """
    The Field class takes care of handling Input and Output TLVs
    """

    def __init__(self, prefix, dictionary, common_objects_dictionary):
        # The field prefix, usually the name of the Container,
        #  e.g. "Qmi Message Ctl Something Output"
        self.prefix = prefix
        # The name of the specific field, e.g. "Result"
        self.name = dictionary['name']
        # The specific TLV ID
        self.id = dictionary['id']
        # Whether the field is to be considered mandatory in the message
        self.mandatory = dictionary['mandatory']
        # Specific format of the field
        self.format = dictionary['format']
        # The type, which must always be "TLV"
        self.type = dictionary['type']
        # Output Fields may have prerequisites
        self.prerequisites = []
        if 'prerequisites' in dictionary:
            self.prerequisites = dictionary['prerequisites']
            # First, look for references to common types
            for prerequisite_dictionary in self.prerequisites:
                if 'common-ref' in prerequisite_dictionary:
                    for common in common_objects_dictionary:
                        if common['type'] == 'prerequisite' and \
                           common['common-ref'] == prerequisite_dictionary['common-ref']:
                           # Replace the reference with the actual common dictionary
                           self.prerequisites.remove(prerequisite_dictionary)
                           self.prerequisites.append(common)
                           break
                    else:
                        raise RuntimeError('Common type \'%s\' not found' % prerequisite_dictionary['name'])

        # Strings containing how the given type is to be copied and disposed
        self.copy = None
        self.dispose = None
        # The field type to be used in the generated code
        self.field_type = None

        # Create the composed full name (prefix + name),
        #  e.g. "Qmi Message Ctl Something Output Result"
        self.fullname = self.prefix + ' ' + self.name

        # Create the variable name within the Container
        self.variable_name = 'arg_' + string.lower(utils.build_underscore_name(self.name))

        # Create the ID enumeration name
        self.id_enum_name = string.upper(utils.build_underscore_name(self.prefix + ' TLV ' + self.name))


    def emit_types(self, hfile, cfile):
        '''
        Subclasses can implement the method to emit the required type
        information
        '''
        pass


    def emit_getter(self, hfile, cfile):
        translations = { 'name'              : self.name,
                         'variable_name'     : self.variable_name,
                         'field_type'        : self.field_type,
                         'dispose_warn'      : ' Do not free the returned @value, it is owned by @self.' if self.dispose is not None else '',
                         'underscore'        : utils.build_underscore_name(self.name),
                         'prefix_camelcase'  : utils.build_camelcase_name(self.prefix),
                         'prefix_underscore' : utils.build_underscore_name(self.prefix) }

        # Emit the getter header
        template = (
            '\n'
            'gboolean ${prefix_underscore}_get_${underscore} (\n'
            '    ${prefix_camelcase} *self,\n'
            '    ${field_type} *value,\n'
            '    GError **error);\n')
        hfile.write(string.Template(template).substitute(translations))

        # Emit the getter source
        template = (
            '\n'
            '/**\n'
            ' * ${prefix_underscore}_get_${underscore}:\n'
            ' * @self: a ${prefix_camelcase}.\n'
            ' * @value: a placeholder for the output value, or #NULL.\n'
            ' * @error: a #GError.\n'
            ' *\n'
            ' * Get the \'${name}\' field from @self.\n'
            ' *\n'
            ' * Returns: #TRUE if the field is found and @value is set, #FALSE otherwise.${dispose_warn}\n'
            ' */\n'
            'gboolean\n'
            '${prefix_underscore}_get_${underscore} (\n'
            '    ${prefix_camelcase} *self,\n'
            '    ${field_type} *value,\n'
            '    GError **error)\n'
            '{\n'
            '    g_return_val_if_fail (self != NULL, FALSE);\n'
            '\n'
            '    if (!self->${variable_name}_set) {\n'
            '        g_set_error (error,\n'
            '                     QMI_CORE_ERROR,\n'
            '                     QMI_CORE_ERROR_TLV_NOT_FOUND,\n'
            '                     "Field \'${name}\' was not found in the message");\n'
            '        return FALSE;\n'
            '    }\n'
            '\n'
            '    /* Just for now, transfer-none always */\n'
            '    if (value)\n'
            '        *value = self->${variable_name};\n'
            '    return TRUE;\n'
            '}\n')
        cfile.write(string.Template(template).substitute(translations))


    def emit_setter(self, hfile, cfile):
        translations = { 'name'              : self.name,
                         'variable_name'     : self.variable_name,
                         'field_type'        : self.field_type,
                         'field_dispose'     : self.dispose + '(self->' + self.variable_name + ');\n' if self.dispose is not None else '',
                         'field_copy'        : self.copy + ' ' if self.copy is not None else '',
                         'underscore'        : utils.build_underscore_name(self.name),
                         'prefix_camelcase'  : utils.build_camelcase_name(self.prefix),
                         'prefix_underscore' : utils.build_underscore_name(self.prefix) }

        # Emit the setter header
        template = (
            '\n'
            'gboolean ${prefix_underscore}_set_${underscore} (\n'
            '    ${prefix_camelcase} *self,\n'
            '    ${field_type} value,\n'
            '    GError **error);\n')
        hfile.write(string.Template(template).substitute(translations))

        # Emit the setter source
        template = (
            '\n'
            '/**\n'
            ' * ${prefix_underscore}_set_${underscore}:\n'
            ' * @self: a ${prefix_camelcase}.\n'
            ' * @value: the value to set.\n'
            ' * @error: a #GError.\n'
            ' *\n'
            ' * Set the \'${name}\' field in the message.\n'
            ' *\n'
            ' * Returns: #TRUE if @value was successfully set, #FALSE otherwise.\n'
            ' */\n'
            'gboolean\n'
            '${prefix_underscore}_set_${underscore} (\n'
            '    ${prefix_camelcase} *self,\n'
            '    ${field_type} value,\n'
            '    GError **error)\n'
            '{\n'
            '    g_return_val_if_fail (self != NULL, FALSE);\n'
            '\n'
            '    ${field_dispose}\n'
            '    self->${variable_name}_set = TRUE;\n'
            '    self->${variable_name} = ${field_copy}(value);\n'
            '\n'
            '    return TRUE;\n'
            '}\n'
            '\n')
        cfile.write(string.Template(template).substitute(translations))

    def emit_input_tlv_add(self, f, line_prefix):
        '''
        Subclasses can implement the method to emit the required TLV adding
        '''
        pass


    def emit_output_prerequisite_check(self, f, line_prefix):
        if self.prerequisites == []:
            f.write('%s/* No Prerequisites for field */\n' % line_prefix)
            return

        for prerequisite in self.prerequisites:
            translations = { 'lp'                     : line_prefix,
                             'prerequisite_field'     : utils.build_underscore_name(prerequisite['field']),
                             'prerequisite_operation' : prerequisite['operation'],
                             'prerequisite_value'     : prerequisite['value'] }
            template = (
                '${lp}/* Prerequisite.... */\n'
                '${lp}if (!(self->arg_${prerequisite_field} ${prerequisite_operation} ${prerequisite_value}))\n'
                '${lp}    break;\n')
            f.write(string.Template(template).substitute(translations))

    def emit_output_tlv_get(self, f, line_prefix):
        '''
        Subclasses can implement the method to emit the required TLV retrieval
        '''
        pass
