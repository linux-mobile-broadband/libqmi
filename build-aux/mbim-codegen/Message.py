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
        self.query        = True if 'query'        in dictionary else False
        self.set          = True if 'set'          in dictionary else False
        self.response     = True if 'response'     in dictionary else False
        self.notification = True if 'notification' in dictionary else False

        # Build Fullname
        self.fullname = 'MBIM Message ' + self.name

        # Build CID enum
        self.cid_enum_name = utils.build_underscore_name('MBIM CID' + self.service + ' ' + self.name).upper()

        # Build input containers
        self.query_container = None
        if self.query:
            self.query_container = Container('query', dictionary['query'])
        self.set_container = None
        if self.set:
            self.set_container = Container('set', dictionary['set'])
        self.notify_container = None

        # Build output containers
        self.response_container = None
        if self.response:
            self.response_container = Container('response', dictionary['response'])
        self.notification_container = None
        if self.notification:
            self.notification_container = Container('notification', dictionary['notification'])


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
            'MbimMessage *${underscore}_${message_type}_new (\n')

        if container != None:
            for field in container.fields:
                translations['field_name_underscore'] = utils.build_underscore_name_from_camelcase (field.name)
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
            ' * ${underscore}_${message_type}_new:\n')

        if container != None:
            for field in container.fields:
                translations['field_name']            = field.name
                translations['field_name_underscore'] = utils.build_underscore_name_from_camelcase (field.name)
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
            '${underscore}_${message_type}_new (\n')

        if container != None:
            for field in container.fields:
                translations['field_name_underscore'] = utils.build_underscore_name_from_camelcase (field.name)
                translations['field_in_format']       = field.in_format
                inner_template = (
                    '    ${field_in_format}${field_name_underscore},\n')
                template += (string.Template(inner_template).substitute(translations))

        template += (
            '    GError **error)\n'
            '{\n'
            '    MbimMessageCommandBuilder *builder;\n'
            '\n'
            '    builder = _mbim_message_command_builder_new (0,\n'
            '                                                 MBIM_SERVICE_${service_underscore_upper},\n'
            '                                                 MBIM_CID_${service_underscore_upper}_${name_underscore_upper},\n'
            '                                                 MBIM_MESSAGE_COMMAND_TYPE_${message_type_upper});\n')

        if container != None:
            template += ('\n')
            for field in container.fields:
                translations['field_name_underscore']   = utils.build_underscore_name_from_camelcase(field.name)
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
    def _emit_message_parser(self, hfile, cfile, message_type, container):
        translations = { 'name'                     : self.name,
                         'service'                  : self.service,
                         'underscore'               : utils.build_underscore_name (self.fullname),
                         'message_type'             : message_type,
                         'message_type_upper'       : message_type.upper(),
                         'service_underscore_upper' : utils.build_underscore_name (self.service).upper(),
                         'name_underscore_upper'    : utils.build_underscore_name (self.name).upper() }
        template = (
            '\n'
            'gboolean ${underscore}_${message_type}_parse (\n'
            '    const MbimMessage *message,\n')

        if container != None:
            for field in container.fields:
                translations['field_name_underscore'] = utils.build_underscore_name_from_camelcase(field.name)
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
            ' * ${underscore}_${message_type}_parse:\n'
            ' * @message: the #MbimMessage.\n')

        if container != None:
            for field in container.fields:
                translations['field_name']             = field.name
                translations['field_name_underscore']  = utils.build_underscore_name_from_camelcase(field.name)
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
            '${underscore}_${message_type}_parse (\n'
            '    const MbimMessage *message,\n')

        if container != None:
            for field in container.fields:
                translations['field_name_underscore'] = utils.build_underscore_name_from_camelcase(field.name)
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
                    translations['field_name_underscore'] = utils.build_underscore_name_from_camelcase(field.name)
                    inner_template = (
                        '    guint32 _${field_name_underscore};\n')
                    template += (string.Template(inner_template).substitute(translations))

        if container != None:
            for field in container.fields:
                translations['field_name_underscore']   = utils.build_underscore_name_from_camelcase(field.name)
                translations['field_format_underscore'] = utils.build_underscore_name(field.format)
                translations['field_size']              = field.size
                translations['field_size_string']       = field.size_string
                translations['field_name']              = field.name

                inner_template = (
                    '\n'
                    '    /* Read the \'${field_name}\' variable */\n'
                    '    {\n')

                if field.is_array_size:
                    inner_template += (
                        '        _${field_name_underscore} = _mbim_message_read_guint32 (message, offset);\n'
                        '        if (${field_name_underscore} != NULL)\n'
                        '            *${field_name_underscore} = _${field_name_underscore};\n'
                        '        offset += 4;\n')
                elif field.format == 'uuid':
                    inner_template += (
                        '        if (${field_name_underscore} != NULL)\n'
                        '            *${field_name_underscore} =  _mbim_message_read_uuid (message, offset);\n'
                        '        offset += 16;\n')
                elif field.format == 'guint32':
                    inner_template += (
                        '        if (${field_name_underscore} != NULL)\n'
                        '            *${field_name_underscore} =  _mbim_message_read_guint32 (message, offset);\n'
                        '        offset += 4;\n')
                elif field.format == 'guint32-array':
                    translations['array_size_field_name_underscore'] = utils.build_underscore_name_from_camelcase(field.array_size_field)
                    inner_template += (
                        '        if (${field_name_underscore} != NULL)\n'
                        '            *${field_name_underscore} = _mbim_message_read_guint32_array (message, _{array_size_field_name_underscore}, offset);\n'
                        '        offset += (4 * _${array_size_field_name_underscore});\n')
                elif field.format == 'string':
                    inner_template += (
                        '        if (${field_name_underscore} != NULL)\n'
                        '            *${field_name_underscore} = _mbim_message_read_string (message, offset);\n'
                        '        offset += 8;\n')
                elif field.format == 'string-array':
                    translations['array_size_field_name_underscore'] = utils.build_underscore_name_from_camelcase(field.array_size_field)
                    inner_template += (
                        '        if (${field_name_underscore} != NULL)\n'
                        '            *${field_name_underscore} = _mbim_message_read_string_array (message, _${array_size_field_name_underscore}, offset);\n'
                        '        offset += (8 * _${array_size_field_name_underscore});\n')
                elif field.format == 'struct':
                    translations['struct_name'] = field.struct_type_underscore
                    translations['struct_type'] = field.struct_type

                    inner_template += (
                        '        ${struct_type} *tmp;\n'
                        '        guint32 bytes_read = 0;\n'
                        '\n'
                        '        tmp = _mbim_message_read_${struct_name}_struct (message, offset, &bytes_read);\n'
                        '        if (${field_name_underscore} != NULL)\n'
                        '            *${field_name_underscore} = tmp;\n'
                        '        else\n'
                        '             ${struct_name}_free (tmp);\n'
                        '        offset += bytes_read;\n')
                elif field.format == 'struct-array':
                    translations['array_size_field_name_underscore'] = utils.build_underscore_name_from_camelcase (field.array_size_field)
                    translations['array_member_size']                = str(field.array_member_size)
                    translations['struct_name']                      = field.struct_type_underscore

                    inner_template += (
                        '        if (${field_name_underscore} != NULL)\n'
                        '            *${field_name_underscore} = _mbim_message_read_${struct_name}_struct_array (message, _${array_size_field_name_underscore}, offset);\n'
                        '        offset += (${array_member_size} * _${array_size_field_name_underscore});\n')
                else:
                    raise ValueError('Cannot handle format \'%s\' as a field' % field.format)

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
        if self.query:
            utils.add_separator(hfile, 'Message (Query)', self.fullname);
            utils.add_separator(cfile, 'Message (Query)', self.fullname);
            self._emit_message_creator(hfile, cfile, 'query', self.query_container)

        if self.set:
            utils.add_separator(hfile, 'Message (Set)', self.fullname);
            utils.add_separator(cfile, 'Message (Set)', self.fullname);
            self._emit_message_creator(hfile, cfile, 'set', self.set_container)

        if self.response:
            utils.add_separator(hfile, 'Message (Response)', self.fullname);
            utils.add_separator(cfile, 'Message (Response)', self.fullname);
            self._emit_message_parser(hfile, cfile, 'response', self.response_container)

        if self.notification:
            utils.add_separator(hfile, 'Message (Notification)', self.fullname);
            utils.add_separator(cfile, 'Message (Notification)', self.fullname);
            self._emit_message_parser(hfile, cfile, 'notification', self.notification_container)


    """
    Emit the sections
    """
    def emit_sections(self, sfile):
        pass
