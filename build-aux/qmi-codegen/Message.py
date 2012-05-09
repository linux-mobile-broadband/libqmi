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
from ContainerOutput import ContainerOutput
from ContainerInput  import ContainerInput

class Message:
    def __init__(self, dictionary):
        # The message prefix
        self.prefix = 'Qmi Message'
        # The message service, e.g. "Ctl"
        self.service = dictionary['service']
        # The name of the specific message, e.g. "Something"
        self.name = dictionary['name']
        # The specific message ID
        self.id = dictionary['id']
        # The type, which must always be 'Message'
        self.type = dictionary['type']

        # Create the composed full name (prefix + service + name),
        #  e.g. "Qmi Message Ctl Something Output Result"
        self.fullname = self.prefix + ' ' + self.service + ' ' + self.name

        # Create the ID enumeration name
        self.id_enum_name = string.upper(utils.build_underscore_name(self.fullname))

        # Build output container.
        # Every defined message will have its own output container, which
        # will generate a new Output type and public getters for each output
        # field
        self.output = ContainerOutput(self.fullname,
                                      dictionary['output'])

        # Build input container.
        # Every defined message will have its own input container, which
        # will generate a new Input type and public getters for each input
        # field
        self.input = ContainerInput(self.fullname,
                                    dictionary['input'] if 'input' in dictionary else None)


    def __emit_request_creator(self, hfile, cfile):
        translations = { 'service'    : self.service,
                         'container'  : utils.build_camelcase_name (self.input.fullname),
                         'underscore' : utils.build_underscore_name (self.fullname),
                         'message_id' : self.id_enum_name }

        input_arg_template = 'gpointer unused' if self.input.fields is None else '${container} *input'
        template = (
            '\n'
            'QmiMessage *${underscore}_request_create (\n'
            '    guint8 transaction_id,\n'
            '    guint8 cid,\n'
            '    %s,\n'
            '    GError **error);\n' % input_arg_template)
        hfile.write(string.Template(template).substitute(translations))

        # Emit message creator
        template = (
            '\n'
            '/**\n'
            ' * ${underscore}_request_create:\n'
            ' */\n'
            'QmiMessage *\n'
            '${underscore}_request_create (\n'
            '    guint8 transaction_id,\n'
            '    guint8 cid,\n'
            '    %s,\n'
            '    GError **error)\n'
            '{\n'
            '    QmiMessage *self;\n'
            '\n'
            '    self = qmi_message_new (QMI_SERVICE_${service},\n'
            '                            cid,\n'
            '                            transaction_id,\n'
            '                            ${message_id});\n'
            '\n' % input_arg_template)
        cfile.write(string.Template(template).substitute(translations))

        if self.input.fields is not None:
            for field in self.input.fields:
                field.emit_input_mandatory_check(cfile, '    ')
                field.emit_input_tlv_add(cfile, '    ')

        cfile.write(
            '    return self;\n'
            '}\n')


    def __emit_response_parser(self, hfile, cfile):
        translations = { 'name'                 : self.name,
                         'container'            : utils.build_camelcase_name (self.output.fullname),
                         'container_underscore' : utils.build_underscore_name (self.output.fullname),
                         'underscore'           : utils.build_underscore_name (self.fullname),
                         'message_id'           : self.id_enum_name }

        template = (
            '\n'
            '${container} *${underscore}_response_parse (\n'
            '    QmiMessage *message,\n'
            '    GError **error);\n')
        hfile.write(string.Template(template).substitute(translations))

        template = (
            '\n'
            '/**\n'
            ' * ${underscore}_response_parse:\n'
            ' * @message: a #QmiMessage response.\n'
            ' * @error: a #GError.\n'
            ' *\n'
            ' * Parse the \'${name}\' response.\n'
            ' *\n'
            ' * Returns: a #${container} which should be disposed with ${container_underscore}_unref(), or #NULL if @error is set.\n'
            ' */\n'
            '${container} *\n'
            '${underscore}_response_parse (\n'
            '    QmiMessage *message,\n'
            '    GError **error)\n'
            '{\n'
            '    ${container} *self;\n'
            '\n'
            '    g_return_val_if_fail (qmi_message_get_message_id (message) == ${message_id}, NULL);\n'
            '\n'
            '    self = g_slice_new0 (${container});\n'
            '    self->ref_count = 1;\n')
        cfile.write(string.Template(template).substitute(translations))

        for field in self.output.fields:
            cfile.write(
                '\n'
                '    do {\n')
            field.emit_output_prerequisite_check(cfile, '        ')
            cfile.write(
                '\n'
                '        {\n')
            field.emit_output_tlv_get(cfile, '            ')
            cfile.write(
                '\n'
                '        }\n')
            cfile.write(
                '    } while (0);\n')
        cfile.write(
            '\n'
            '    return self;\n'
            '}\n')


    def emit(self, hfile, cfile):
        utils.add_separator(hfile, 'REQUEST/RESPONSE', self.fullname);
        utils.add_separator(cfile, 'REQUEST/RESPONSE', self.fullname);

        hfile.write('\n/* --- Input -- */\n');
        cfile.write('\n/* --- Input -- */\n');
        self.input.emit(hfile, cfile)
        self.__emit_request_creator (hfile, cfile)

        hfile.write('\n/* --- Output -- */\n');
        cfile.write('\n/* --- Output -- */\n');
        self.output.emit(hfile, cfile)
        self.__emit_response_parser (hfile, cfile)
