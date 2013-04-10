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

"""
The Value class takes care of all kind of variables
"""
class Value:

    """
    Constructor
    """
    def __init__(self, dictionary):

        self.dictionary = dictionary

        """ The name of the variable """
        self.name = dictionary['name']

        """ The type of the variable """
        self.format = dictionary['format']

        """ Whether this field is visible or not """
        self.visible = False if 'visibility' in dictionary and dictionary['visibility'] == 'private' else True

        """ The public format of the value """
        self.public_format = ''

        """ Type of the value when used as input parameter """
        self.in_format = ''
        self.in_description = ''

        """ Type of the value when used as output parameter """
        self.out_format = ''
        self.out_description = ''

        """ Flag to identify this value as being used as an array size """
        self.is_array_size = False

        """ The name of the method used to read the value """
        self.reader_method_name = ''

        """ The size of this variable """
        self.size = 0
        self.size_string = ''

        """ Whether this value is an array """
        self.is_array = False

        """ The size of a member of the array """
        self.array_member_size = 0

        """ The field giving the size of the array """
        self.array_size_field = ''
