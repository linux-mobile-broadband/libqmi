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

import utils
from FieldString       import FieldString
from FieldStruct       import FieldStruct
from FieldStructResult import FieldStructResult
from FieldArray        import FieldArray
from FieldBasic        import FieldBasic

class Container:
    """
    The Container class takes care of handling collections of Input or
    Output fields
    """

    def __init__(self, prefix, dictionary, common_objects_dictionary):
        # The field container prefix usually contains the name of the Message,
        # e.g. "Qmi Message Ctl Something"
        self.prefix = prefix

        # self.name needs to be set by the subclass
        if self.name != 'Input' and self.name != 'Output':
            raise ValueError('Cannot handle container \'%s\'' % self.name)

        # Create the composed full name (prefix + name),
        #  e.g. "Qmi Message Ctl Something Output Result"
        self.fullname = self.prefix + ' ' + self.name

        self.fields = None
        if dictionary is not None:
            self.fields = []

            # First, look for references to common types
            for field_dictionary in dictionary:
                if 'common-ref' in field_dictionary:
                    for common in common_objects_dictionary:
                        if common['type'] == 'TLV' and \
                           common['common-ref'] == field_dictionary['common-ref']:
                           # Replace the reference with a copy of the common dictionary
                           # If the source reference has prerequisites, add them to the copy
                           copy = dict(common)
                           if 'prerequisites' in field_dictionary:
                               copy['prerequisites'] = field_dictionary['prerequisites']
                           dictionary.remove(field_dictionary)
                           dictionary.append(copy)
                           break
                    else:
                        raise RuntimeError('Common type \'%s\' not found' % field_dictionary['name'])

            # We need to sort the fields, so that the ones with prerequisites are
            # include after the prerequisites themselves. Note: we don't currently
            # support complex setups yet.
            sorted_dictionary = []
            for field_dictionary in dictionary:
                if 'prerequisites' in field_dictionary:
                    sorted_dictionary.append(field_dictionary)
                else:
                    sorted_dictionary.insert(0, field_dictionary)

            # Then, really parse each field
            for field_dictionary in sorted_dictionary:
                if field_dictionary['type'] == 'TLV':
                    if field_dictionary['format'] == 'array':
                        self.fields.append(FieldArray(self.fullname, field_dictionary, common_objects_dictionary))
                    elif field_dictionary['format'] == 'string':
                        self.fields.append(FieldString(self.fullname, field_dictionary, common_objects_dictionary))
                    elif field_dictionary['format'] == 'struct':
                        if field_dictionary['name'] == 'Result':
                            self.fields.append(FieldStructResult(self.fullname, field_dictionary, common_objects_dictionary))
                        else:
                            self.fields.append(FieldStruct(self.fullname, field_dictionary, common_objects_dictionary))
                    elif field_dictionary['format'] == 'guint8' or \
                         field_dictionary['format'] == 'guint16' or \
                         field_dictionary['format'] == 'guint32' or \
                         field_dictionary['format'] == 'gint8' or \
                         field_dictionary['format'] == 'gint16' or \
                         field_dictionary['format'] == 'gint32':
                        self.fields.append(FieldBasic(self.fullname, field_dictionary, common_objects_dictionary))
                    else:
                        raise ValueError('Cannot handle TLV format \'%s\'' % field_dictionary['format'])


    def __emit_tlv_ids_enum(self, f):
        if self.fields is None:
            return

        translations = { 'enum_type' : utils.build_camelcase_name (self.fullname + ' TLV') }
        template = (
            '\n'
            '/**\n'
            ' * ${enum_type}:\n')
        f.write(string.Template(template).substitute(translations))

        for tlv in self.fields:
            translations['enum_name'] = tlv.id_enum_name
            translations['enum_value'] = tlv.id
            template = (
                ' * @${enum_name}: TODO,\n')
            f.write(string.Template(template).substitute(translations))

        template = (
            ' * TODO: comment enum\n'
            ' */\n'
            'typedef enum {\n')
        f.write(string.Template(template).substitute(translations))

        for tlv in self.fields:
            translations['enum_name'] = tlv.id_enum_name
            translations['enum_value'] = tlv.id
            template = (
                '    ${enum_name} = ${enum_value},\n')
            f.write(string.Template(template).substitute(translations))

        template = (
            '} ${enum_type};\n')
        f.write(string.Template(template).substitute(translations))


    def __emit_types(self, hfile, cfile, translations):
        # Emit types header
        template = (
            '\n'
            'typedef struct _${camelcase} ${camelcase};\n')
        hfile.write(string.Template(template).substitute(translations))

        # Emit types source
        template = (
            '\n'
            'struct _${camelcase} {\n'
            '    volatile gint ref_count;\n'
            '\n')
        cfile.write(string.Template(template).substitute(translations))

        if self.fields is not None:
            for field in self.fields:
                translations['field_type'] = field.field_type
                translations['field_variable_name'] = field.variable_name
                translations['field_name'] = field.name
                template = (
                    '\n'
                    '    /* ${field_name} */\n'
                    '    gboolean ${field_variable_name}_set;\n'
                    '    ${field_type} ${field_variable_name};\n')
                cfile.write(string.Template(template).substitute(translations))

        cfile.write(
            '};\n')


    def __emit_core(self, hfile, cfile, translations):
        # Emit container core header
        template = (
            '\n'
            '${camelcase} *${underscore}_ref (${camelcase} *self);\n'
            'void ${underscore}_unref (${camelcase} *self);\n')
        if self.readonly == False:
            template += (
                '${camelcase} *${underscore}_new (void);\n')
        hfile.write(string.Template(template).substitute(translations))

        # Emit container core source
        template = (
            '\n'
            '/**\n'
            ' * ${underscore}_ref:\n'
            ' * @self: a #${camelcase}.\n'
            ' *\n'
            ' * Atomically increments the reference count of @self by one.\n'
            ' *\n'
            ' * Returns: the new reference to @self.\n'
            ' */\n'
            '${camelcase} *\n'
            '${underscore}_ref (${camelcase} *self)\n'
            '{\n'
            '    g_return_val_if_fail (self != NULL, NULL);\n'
            '\n'
            '    g_atomic_int_inc (&self->ref_count);\n'
            '    return self;\n'
            '}\n'
            '\n'
            '/**\n'
            ' * ${underscore}_unref:\n'
            ' * @self: a #${camelcase}.\n'
            ' *\n'
            ' * Atomically decrements the reference count of @self by one.\n'
            ' * If the reference count drops to 0, @self is completely disposed.\n'
            ' */\n'
            'void\n'
            '${underscore}_unref (${camelcase} *self)\n'
            '{\n'
            '    g_return_if_fail (self != NULL);\n'
            '\n'
            '    if (g_atomic_int_dec_and_test (&self->ref_count)) {\n')
        cfile.write(string.Template(template).substitute(translations))

        if self.fields is not None:
            for field in self.fields:
                if field.dispose is not None:
                    translations['field_dispose'] = field.dispose
                    translations['field_variable_name'] = field.variable_name
                    template = (
                        '        ${field_dispose} (self->${field_variable_name});\n')
                    cfile.write(string.Template(template).substitute(translations))

        template = (
            '        g_slice_free (${camelcase}, self);\n'
            '    }\n'
            '}\n')
        cfile.write(string.Template(template).substitute(translations))

        # _new() is only generated if the container is not readonly
        if self.readonly == True:
            return

        template = (
            '\n'
            '/**\n'
            ' * ${underscore}_new:\n'
            ' *\n'
            ' * Allocates a new ${camelcase}.\n'
            ' *\n'
            ' * Returns: the newly created ${camelcase}.\n'
            ' */\n'
            '${camelcase} *\n'
            '${underscore}_new (void)\n'
            '{\n'
            '    ${camelcase} *self;\n'
            '\n'
            '    self = g_slice_new0 (${camelcase});\n'
            '    self->ref_count = 1;\n'
            '    return self;\n'
            '}\n')
        cfile.write(string.Template(template).substitute(translations))


    def emit(self, hfile, cfile):
        translations = { 'name'       : self.name,
                         'camelcase'  : utils.build_camelcase_name (self.fullname),
                         'underscore' : utils.build_underscore_name (self.fullname) }

        if self.fields is None:
            template = ('\n'
                        '/* Note: no fields in the ${name} container */\n')
            hfile.write(string.Template(template).substitute(translations))
            cfile.write(string.Template(template).substitute(translations))
            return

        # Emit the container and field  types
        # Emit field getter/setter
        if self.fields is not None:
            for field in self.fields:
                field.emit_types(hfile, cfile)
        self.__emit_types(hfile, cfile, translations)

        # Emit TLV enums
        self.__emit_tlv_ids_enum(cfile)

        # Emit fields
        if self.fields is not None:
            for field in self.fields:
                field.emit_getter(hfile, cfile)
                if self.readonly == False:
                    field.emit_setter(hfile, cfile)

        # Emit the container core
        self.__emit_core(hfile, cfile, translations)
