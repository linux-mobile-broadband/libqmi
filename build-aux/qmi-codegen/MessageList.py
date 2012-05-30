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

from Message import Message
import utils

"""
The MessageList class handles the generation of all messages for a given
specific service
"""
class MessageList:

    """
    Constructor
    """
    def __init__(self, objects_dictionary, common_objects_dictionary):
        self.list = []
        self.message_id_enum_name = None
        self.service = None

        # Loop items in the list, creating Message objects for the messages
        # and looking for the special 'Message-ID-Enum' type
        for object_dictionary in objects_dictionary:
            if object_dictionary['type'] == 'Message':
                message = Message(object_dictionary, common_objects_dictionary)
                self.list.append(message)
            elif object_dictionary['type'] == 'Message-ID-Enum':
                self.message_id_enum_name = object_dictionary['name']
            elif object_dictionary['type'] == 'Service':
                self.service = object_dictionary['name']

        # We NEED the Message-ID-Enum field
        if self.message_id_enum_name is None:
            raise ValueError('Missing Message-ID-Enum field')

        # We NEED the Service field
        if self.service is None:
            raise ValueError('Missing Service field')


    """
    Emit the enumeration of the messages found in the specific service
    """
    def emit_message_ids_enum(self, f):
        translations = { 'enum_type' : utils.build_camelcase_name (self.message_id_enum_name) }
        template = (
            '\n'
            'typedef enum {\n')
        for message in self.list:
            translations['enum_name'] = message.id_enum_name
            translations['enum_value'] = message.id
            enum_template = (
                '    ${enum_name} = ${enum_value},\n')
            template += string.Template(enum_template).substitute(translations)

        template += (
            '} ${enum_type};\n'
            '\n')
        f.write(string.Template(template).substitute(translations))


    """
    Emit the method responsible for getting a printable representation of all
    messages of a given service.
    """
    def __emit_get_printable(self, hfile, cfile):
        translations = { 'service'    : string.lower(self.service) }

        template = (
            '\n'
            'gchar *qmi_message_${service}_get_printable (\n'
            '    QmiMessage *self,\n'
            '    const gchar *line_prefix);\n')
        hfile.write(string.Template(template).substitute(translations))

        template = (
            '\n'
            'gchar *\n'
            'qmi_message_${service}_get_printable (\n'
            '    QmiMessage *self,\n'
            '    const gchar *line_prefix)\n'
            '{\n'
            '    switch (qmi_message_get_message_id (self)) {\n')

        for message in self.list:
            translations['enum_name'] = message.id_enum_name
            translations['message_underscore'] = utils.build_underscore_name (message.name)
            translations['enum_value'] = message.id
            inner_template = (
                'case ${enum_name}:\n'
                '    return ${message_underscore}_get_printable (self, line_prefix);\n')
            template += string.Template(inner_template).substitute(translations)

        template += (
            '    default:\n'
            '        return NULL;\n'
            '    }\n'
            '}\n')
        cfile.write(string.Template(template).substitute(translations))


    """
    Emit the message list handling implementation
    """
    def emit(self, hfile, cfile):
        # First, emit the message IDs enum
        self.emit_message_ids_enum(cfile)

        # Then, emit all message handlers
        for message in self.list:
            message.emit(hfile, cfile)

        # First, emit common class code
        utils.add_separator(hfile, 'Service-specific printable', self.service);
        utils.add_separator(cfile, 'Service-specific printable', self.service);
        self.__emit_get_printable(hfile, cfile)
