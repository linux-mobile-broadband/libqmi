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

import utils

class Struct:

    def __init__(self, name, members):
        # The struct type name, e.g. QmiTheStruct
        self.name = name
        # The struct members, a dictionary of 'format'+'name' pairs
        self.members = members


    # Emits the packed definition of the struct, meant to be private
    def emit_packed(self, f):
        translations = { 'name' : self.name }
        template = (
            '\n'
            'typedef struct _${name}Packed {\n')
        f.write(string.Template(template).substitute(translations))

        for var in self.members:
            translations['variable_type'] = var['format']
            translations['variable_name'] = utils.build_underscore_name(var['name'])
            template = (
                '    ${variable_type} ${variable_name};\n')
            f.write(string.Template(template).substitute(translations))

        template = ('} __attribute__((__packed__)) ${name}Packed;\n\n')
        f.write(string.Template(template).substitute(translations))


    # Emits the public non-packed definition of the struct
    def emit(self, f):
        translations = { 'name' : self.name }
        template = (
            '\n'
            'typedef struct _${name} {\n')
        f.write(string.Template(template).substitute(translations))

        for var in self.members:
            translations['variable_type'] = var['public-format'] if 'public-format' in var else var['format']
            translations['variable_name'] = utils.build_underscore_name(var['name'])
            template = (
                '    ${variable_type} ${variable_name};\n')
            f.write(string.Template(template).substitute(translations))

        template = ('} ${name};\n\n')
        f.write(string.Template(template).substitute(translations))
