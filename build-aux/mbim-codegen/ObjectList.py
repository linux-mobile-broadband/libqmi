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

from Message import Message
from Struct import Struct
import utils

"""
Check field to see if it holds a struct
"""
def set_struct_usage(struct, fields):
    for field in fields:
        if field['format'] == 'struct' and field['struct-type'] == struct.name:
            struct.single_member = True
            break
        if field['format'] == 'ref-struct-array' and field['struct-type'] == struct.name:
            struct.array_member = True
            break
        if field['format'] == 'struct-array' and field['struct-type'] == struct.name:
            struct.array_member = True
            break


"""
The ObjectList class handles the generation of all commands and types for a given
specific service
"""
class ObjectList:

    """
    Constructor
    """
    def __init__(self, objects_dictionary):
        self.command_list = []
        self.struct_list = []

        # Loop items in the list, creating Message objects for the messages
        for object_dictionary in objects_dictionary:
            if object_dictionary['type'] == 'Command':
                self.command_list.append(Message(object_dictionary))
            elif object_dictionary['type'] == 'Struct':
                self.struct_list.append(Struct(object_dictionary))
            else:
                raise ValueError('Cannot handle object type \'%s\'' % object_dictionary['type'])

        # Populate struct usages
        for struct in self.struct_list:
            for command in self.command_list:
                set_struct_usage(struct, command.query)
                set_struct_usage(struct, command.set)
                set_struct_usage(struct, command.response)
                set_struct_usage(struct, command.notification)

    """
    Emit the structs and commands handling implementation
    """
    def emit(self, hfile, cfile):
        # Emit all structs
        for item in self.struct_list:
            item.emit(hfile, cfile)
        # Emit all commands
        for item in self.command_list:
            item.emit(hfile, cfile)


    """
    Emit the sections
    """
    def emit_sections(self, sfile):
        for item in self.command_list:
            item.emit_sections(sfile)
