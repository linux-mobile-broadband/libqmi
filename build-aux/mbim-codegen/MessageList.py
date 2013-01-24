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
import utils

"""
The MessageList class handles the generation of all messages for a given
specific service
"""
class MessageList:

    """
    Constructor
    """
    def __init__(self, objects_dictionary):
        self.list = []

        # Loop items in the list, creating Message objects for the messages
        for object_dictionary in objects_dictionary:
            message = Message(object_dictionary)
            self.list.append(message)

    """
    Emit the message list handling implementation
    """
    def emit(self, hfile, cfile):
        # Then, emit all message handlers
        for message in self.list:
            message.emit(hfile, cfile)

    """
    Emit the sections
    """
    def emit_sections(self, sfile):
        # Emit all message sections
        for message in self.list:
            message.emit_sections(sfile)
