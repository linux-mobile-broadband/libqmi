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

class MessageList:
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


    def emit(self, hfile, cfile):
        # First, emit the message IDs enum
        self.emit_message_ids_enum(cfile)

        # Then, emit all message handlers
        for message in self.list:
            message.emit(hfile, cfile)
