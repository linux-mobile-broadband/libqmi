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

from Value import Value

"""
The ValueUint64 class takes care of UINT64 variables
"""
class ValueUint64(Value):

    """
    Constructor
    """
    def __init__(self, dictionary):

        # Call the parent constructor
        Value.__init__(self, dictionary)

        """ The public format of the value """
        self.public_format = dictionary['public-format'] if 'public-format' in dictionary else 'guint64'

        """ Type of the value when used as input parameter """
        self.in_format = self.public_format + ' '
        self.in_description = 'The \'' + self.name + '\' field, given as a #' + self.public_format + '.'

        """ Type of the value when used as output parameter """
        self.out_format = self.public_format + ' *'
        self.out_description = 'Return location for a #' + self.public_format + ', or %NULL if the \'' + self.name + '\' field is not needed.'

        """ The name of the method used to read the value """
        self.reader_method_name = '_mbim_message_read_guint64'

        """ The size of this variable """
        self.size = 4
        self.size_string = ''

        """ Whether this value is an array """
        self.is_array = False
