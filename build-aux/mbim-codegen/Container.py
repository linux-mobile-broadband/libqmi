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
# Copyright (C) 2013 Aleksander Morgado <aleksander@gnu.org>
#

import string
import utils
from ValueUint32      import ValueUint32
from ValueUint32Array import ValueUint32Array
from ValueString      import ValueString
from ValueStringArray import ValueStringArray
from ValueStruct      import ValueStruct
from ValueStructArray import ValueStructArray

"""
The Container class takes care of handling collections of Input or
Output fields
"""
class Container:

    """
    Constructor
    """
    def __init__(self, service, command, message_type, container_type, dictionary):
        self.service = service
        self.command = command

        # e.g. "Mbim Message Service Something"
        self.prefix = "Mbim Message " + service + ' ' + command

        # We may have 'Query', 'Set' or 'Notify' message types
        if message_type == 'Query' or message_type == 'Set' or message_type == 'Notify':
            self.message_type = message_type
        else:
            raise ValueError('Cannot handle message type \'%s\'' % message_type)

        # We may have 'Input' or 'Output' containers
        if container_type == 'Input':
            self.name = 'Request'
            self.message_type_enum = 'MBIM_MESSAGE_TYPE_COMMAND'
        elif container_type == 'Output':
            if message_type == 'Notify':
                self.name = 'Indication'
                self.message_type_enum = 'MBIM_MESSAGE_TYPE_INDICATION'
            else:
                self.name = 'Response'
                self.message_type_enum = 'MBIM_MESSAGE_TYPE_COMMAND_DONE'
        else:
            raise ValueError('Cannot handle container type \'%s\'' % container_type)

        # Create the composed full name (prefix + name),
        #  e.g. "Mbim Message Service Something Query Response"
        self.fullname = self.prefix + ' ' + self.message_type + ' ' + self.name

        self.fields_dictionary = dictionary

        self.fields = []
        for field in dictionary:
            if field['format'] == 'uuid':
                self.fields.append(ValueUuid(field))
            elif field['format'] == 'guint32':
                self.fields.append(ValueUint32(field))
            elif field['format'] == 'guint32-array':
                new_field = ValueUint32Array(field)
                self.mark_array_length(new_field.array_size_field)
                self.fields.append(new_field)
            elif field['format'] == 'string':
                self.fields.append(ValueString(field))
            elif field['format'] == 'string-array':
                new_field = ValueStringArray(field)
                self.mark_array_length(new_field.array_size_field)
                self.fields.append(new_field)
            elif field['format'] == 'struct':
                self.fields.append(ValueStruct(field))
            elif field['format'] == 'struct-array':
                new_field = ValueStructArray(field)
                self.mark_array_length(new_field.array_size_field)
                self.fields.append(new_field)
            else:
                raise ValueError('Cannot handle field type \'%s\'' % field['format'])


    """
    Flag the values which are used as length of arrays
    """
    def mark_array_length(self, array_size_field):
        found = False
        for field in self.fields:
            if field.name == array_size_field:
                field.is_array_size = True
                found = True
                break
        if found == False:
            raise ValueError('Couldn\'t find array size field \'%s\'' % array_size_field)
