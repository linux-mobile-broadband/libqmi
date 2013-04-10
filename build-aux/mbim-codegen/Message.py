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
# Copyright (C) 2013 Aleksander Morgado <aleksander@gnu.org>
#

import string

from Container import Container
import utils

"""
The Message class takes care of all message handling
"""
class Message:

    """
    Constructor
    """
    def __init__(self, dictionary):
        # The message service, e.g. "Basic Connect"
        self.service = dictionary['service']
        # The name of the specific message, e.g. "Something"
        self.name = dictionary['name']

        # Gather types of command
        self.can_query  = True if 'query'  in dictionary else False
        self.can_set    = True if 'set'    in dictionary else False
        self.can_notify = True if 'notify' in dictionary else False

        # Build Fullname
        self.fullname = 'MBIM Message ' + self.service + ' ' + self.name

        # Build CID enum
        self.cid_enum_name = utils.build_underscore_name('MBIM CID' + self.service + ' ' + self.name).upper()

        # Build Query containers
        self.query_input = None
        self.query_output = None
        if self.can_query:
            if 'input' in dictionary['query']:
                self.query_input = Container(self.service, self.name, 'Query', 'Input', dictionary['query']['input'])
            if 'output' in dictionary['query']:
                self.query_output = Container(self.service, self.name, 'Query', 'Output', dictionary['query']['output'])

        # Build Set containers
        self.set_input = None
        self.set_output = None
        if self.can_set:
            if 'input' in dictionary['set']:
                self.set_input = Container(self.service, self.name, 'Set', 'Input', dictionary['set']['input'])
            if 'output' in dictionary['set']:
                self.set_output = Container(self.service, self.name, 'Set', 'Output', dictionary['set']['output'])

        # Build Notify containers
        self.notify_output = None
        if self.can_notify:
            if 'output' in dictionary['notify']:
                self.notify_output = Container(self.service, self.name, 'Notify', 'Output', dictionary['notify']['output'])


    """
    Emit message creator
    """
    def _emit_message_creator(self, hfile, cfile, message_type, container):
        translations = { 'name'                     : self.name,
                         'service'                  : self.service,
                         'underscore'               : utils.build_underscore_name (self.fullname),
                         'message_type'             : message_type,
                         'message_type_upper'       : message_type.upper(),
                         'service_underscore_upper' : utils.build_underscore_name (self.service).upper(),
                         'name_underscore_upper'    : utils.build_underscore_name (self.name).upper() }
        template = (
            '\n'
            'MbimMessage *${underscore}_${message_type}_request_new (\n'
            '    guint32 transaction_id,\n')

        if container != None:
            for field in container.fields:
                translations['field_name_underscore'] = utils.build_underscore_name (field.name)
                translations['field_in_format']       = field.in_format
                inner_template = (
                    '    ${field_in_format}${field_name_underscore},\n')
                template += (string.Template(inner_template).substitute(translations))

        template += (
            '    GError **error);\n')
        hfile.write(string.Template(template).substitute(translations))

        template = (
            '\n'
            '/**\n'
            ' * ${underscore}_${message_type}_request_new:\n'
            ' * @transaction_id: the transaction ID to use in the request.\n')

        if container != None:
            for field in container.fields:
                translations['field_name']            = field.name
                translations['field_name_underscore'] = utils.build_underscore_name (field.name)
                translations['field_in_description']  = field.in_description
                inner_template = (
                    ' * @${field_name_underscore}: the \'${field_name}\' field, given as ${field_in_description}\n')
                template += (string.Template(inner_template).substitute(translations))

        template += (
            ' * @error: return location for error or %NULL.\n'
            ' *\n'
            ' * Create a new request for the \'${name}\' ${message_type} command in the \'${service}\' service.\n'
            ' *\n'
            ' * Returns: a newly allocated #MbimMessage, which should be freed with mbim_message_unref().\n'
            ' */\n'
            'MbimMessage *\n'
            '${underscore}_${message_type}_request_new (\n'
            '    guint32 transaction_id,\n')

        if container != None:
            for field in container.fields:
                translations['field_name_underscore'] = utils.build_underscore_name (field.name)
                translations['field_in_format']       = field.in_format
                inner_template = (
                    '    ${field_in_format}${field_name_underscore},\n')
                template += (string.Template(inner_template).substitute(translations))

        template += (
            '    GError **error)\n'
            '{\n'
            '    MbimMessageCommandBuilder *builder;\n'
            '\n'
            '    builder = _mbim_message_command_builder_new (transaction_id,\n'
            '                                                 MBIM_SERVICE_${service_underscore_upper},\n'
            '                                                 MBIM_CID_${service_underscore_upper}_${name_underscore_upper},\n'
            '                                                 MBIM_MESSAGE_COMMAND_TYPE_${message_type_upper});\n')

        if container != None:
            template += ('\n')
            for field in container.fields:
                translations['field_name_underscore']   = utils.build_underscore_name(field.name)
                translations['field_format_underscore'] = utils.build_underscore_name(field.format)
                inner_template = (
                    '    _mbim_message_command_builder_append_${field_format_underscore} (builder, ${field_name_underscore});\n')
                template += (string.Template(inner_template).substitute(translations))

        template += (
            '\n'
            '    return _mbim_message_command_builder_complete (builder);\n'
            '}\n')
        cfile.write(string.Template(template).substitute(translations))


    """
    Emit message parser
    """
    def _emit_message_parser(self, hfile, cfile, request_or_response, message_type, container):
        translations = { 'name'                     : self.name,
                         'service'                  : self.service,
                         'underscore'               : utils.build_underscore_name (self.fullname),
                         'request_or_response'      : request_or_response,
                         'message_type'             : message_type,
                         'message_type_upper'       : message_type.upper(),
                         'service_underscore_upper' : utils.build_underscore_name (self.service).upper(),
                         'name_underscore_upper'    : utils.build_underscore_name (self.name).upper() }
        template = (
            '\n'
            'gboolean ${underscore}_${message_type}_${request_or_response}_parse (\n'
            '    const MbimMessage *message,\n')

        if container != None:
            for field in container.fields:
                translations['field_name_underscore'] = utils.build_underscore_name (field.name)
                translations['field_out_format']      = field.out_format
                inner_template = (
                    '    ${field_out_format}${field_name_underscore},\n')
                template += (string.Template(inner_template).substitute(translations))

        template += (
            '    GError **error);\n')
        hfile.write(string.Template(template).substitute(translations))

        template = (
            '\n'
            '/**\n'
            ' * ${underscore}_${message_type}_${request_or_response}_parse:\n'
            ' * @message: the #MbimMessage.\n')

        if container != None:
            for field in container.fields:
                translations['field_name']             = field.name
                translations['field_name_underscore']  = utils.build_underscore_name (field.name)
                translations['field_out_description']  = field.out_description
                inner_template = (
                    ' * @${field_name_underscore}: ${field_out_description}\n')
                template += (string.Template(inner_template).substitute(translations))

        template += (
            ' * @error: return location for error or %NULL.\n'
            ' *\n'
            ' * Create a new request for the \'${name}\' ${message_type} command in the \'${service}\' service.\n'
            ' *\n'
            ' * Returns: %TRUE if the message was correctly parsed, %FALSE if @error is set.\n'
            ' */\n'
            'gboolean\n'
            '${underscore}_${message_type}_${request_or_response}_parse (\n'
            '    const MbimMessage *message,\n')

        if container != None:
            for field in container.fields:
                translations['field_name_underscore'] = utils.build_underscore_name (field.name)
                translations['field_out_format']      = field.out_format
                inner_template = (
                    '    ${field_out_format}${field_name_underscore},\n')
                template += (string.Template(inner_template).substitute(translations))

        template += (
            '    GError **error)\n'
            '{\n'
            '    guint32 offset = 0;\n')
        if container != None:
            for field in container.fields:
                if field.is_array_size:
                    translations['field_name_underscore'] = utils.build_underscore_name (field.name)
                    inner_template = (
                        '    guint32 _${field_name_underscore};\n')
                    template += (string.Template(inner_template).substitute(translations))

        if container != None:
            for field in container.fields:
                translations['field_name_underscore']   = utils.build_underscore_name(field.name)
                translations['field_format_underscore'] = utils.build_underscore_name(field.format)
                translations['field_size']              = field.size
                translations['field_size_string']       = field.size_string
                translations['field_name']              = field.name

                inner_template = (
                    '\n'
                    '    /* Read the \'${field_name}\' variable */\n'
                    '    {\n')

                if field.is_array:
                    translations['array_size_field_name_underscore'] = utils.build_underscore_name (field.array_size_field)
                    translations['array_member_size']                = str(field.array_member_size)
                    translations['struct_name']                      = (field.struct_type_underscore + '_') if field.format == 'struct-array' else ''

                    inner_template += (
                        '        if (${field_name_underscore} != NULL)\n'
                        '            *${field_name_underscore} = _mbim_message_read_${struct_name}${field_format_underscore} (message, _${array_size_field_name_underscore}, offset);\n'
                        '        offset += (${array_member_size} * _${array_size_field_name_underscore});\n')
                else:
                    if field.is_array_size:
                        inner_template += (
                            '        _${field_name_underscore} = _mbim_message_read_${field_format_underscore} (message, offset);\n'
                            '        if (${field_name_underscore} != NULL)\n'
                            '            *${field_name_underscore} = _${field_name_underscore};\n')
                    else:
                        inner_template += (
                            '        if (${field_name_underscore} != NULL)\n'
                            '            *${field_name_underscore} = _mbim_message_read_${field_format_underscore} (message, offset);\n')
                    if field.size > 0:
                        inner_template += (
                            '        offset += ${field_size};\n')
                    if field.size_string != '':
                        inner_template += (
                            '        offset += ${field_size_string};\n')

                inner_template += (
                    '    }\n')

                template += (string.Template(inner_template).substitute(translations))

        template += (
            '\n'
            '    return TRUE;\n'
            '}\n')
        cfile.write(string.Template(template).substitute(translations))


    """
    Emit the message handling implementation
    """
    def emit(self, hfile, cfile):
        if self.can_query:
            utils.add_separator(hfile, 'Message (Query)', self.fullname);
            utils.add_separator(cfile, 'Message (Query)', self.fullname);

            self._emit_message_creator(hfile, cfile, 'query', self.query_input)
            if self.query_input:
                self._emit_message_parser(hfile, cfile, 'request',  'query', self.query_input)
            if self.query_output:
                self._emit_message_parser(hfile, cfile, 'response', 'query', self.query_output)

        if self.can_set:
            utils.add_separator(hfile, 'Message (Set)', self.fullname);
            utils.add_separator(cfile, 'Message (Set)', self.fullname);

            self._emit_message_creator(hfile, cfile, 'set', self.set_input)
            if self.set_input:
                self._emit_message_parser(hfile, cfile, 'request',  'set', self.set_input)
            if self.set_output:
                self._emit_message_parser(hfile, cfile, 'response', 'set', self.set_output)

        if self.can_notify:
            utils.add_separator(hfile, 'Message (Notify)', self.fullname);
            utils.add_separator(cfile, 'Message (Notify)', self.fullname);


    """
    Emit the sections
    """
    def emit_sections(self, sfile):
        pass
