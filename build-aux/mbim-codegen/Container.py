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
    def __init__(self, message_type, dictionary):
        # We may have 'Query', 'Set' or 'Notify' message types
        if message_type == 'query':
            self.message_type_enum = 'MBIM_MESSAGE_TYPE_COMMAND'
        elif message_type == 'set':
            self.message_type_enum = 'MBIM_MESSAGE_TYPE_COMMAND'
        elif message_type == 'response':
            self.message_type_enum = 'MBIM_MESSAGE_TYPE_COMMAND_DONE'
        elif message_type == 'notification':
            self.message_type_enum = 'MBIM_MESSAGE_TYPE_INDICATE_STATUS'
        else:
            raise ValueError('Cannot handle message type \'%s\'' % message_type)

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
