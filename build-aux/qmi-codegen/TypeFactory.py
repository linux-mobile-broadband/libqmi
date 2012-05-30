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
# Copyright (C) 2012 Lanedo GmbH
#


"""
List to keep track of types already emitted to the source/header files.
"""
emitted_types = []

"""
Checks whether a given type has already been emitted.
"""
def is_type_emitted(type_name):
    for i in emitted_types:
        if i == type_name:
            return True
    else:
        return False

"""
Sets the given type as already emitted.
"""
def set_type_emitted(type_name):
    if is_type_emitted(type_name):
        return False
    else:
        emitted_types.append(type_name)
        return True



"""
List to keep track of type-specific get_printable() methods already emitted to
the source/header files.
"""
emitted_get_printable = []

"""
Checks whether a given type-specific get_printable() has already been emitted.
"""
def is_get_printable_emitted(type_name):
    for i in emitted_get_printable:
        if i == type_name:
            return True
    else:
        return False

"""
Sets the given type-specific get_printable() as already emitted.
"""
def set_get_printable_emitted(type_name):
    if is_get_printable_emitted(type_name):
        return False
    else:
        emitted_get_printable.append(type_name)
        return True
