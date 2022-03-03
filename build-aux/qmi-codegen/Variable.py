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
# Copyright (C) 2012-2022 Aleksander Morgado <aleksander@aleksander.es>
#

import string
import utils

"""
Base class for every variable type defined in the database
"""
class Variable:

    """
    Constructor with common variable handling
    """
    def __init__(self, service, dictionary):
        """
        The current QMI service
        """
        self.service = service

        """
        Variables can define specific public and private formats to be used.
        The public format will be that used in the generated interface file,
        while the private one will only be used internally.
        """
        self.format = dictionary['format']
        self.public_format = None
        self.private_format = None

        """
        Element type to be used in introspection annotations.
        """
        self.element_type = None

        """
        Whether the variable is visible in public API or is reserved
        """
        self.visible = False if ('visible' in dictionary and dictionary['visible'] == 'no') else True

        """
        Variables that get allocated in heap need to get properly disposed.
        """
        self.needs_dispose = False

        """
        Variables that get allocated in heap need to have a clear method so that it
        can be used as part of an array of this variable type.
        """
        self.clear_method = ''

        """
        Custom endianness configuration for a specific variable; if none given, defaults
        to host endian.
        """
        self.endian = "QMI_ENDIAN_LITTLE"
        if 'endian' in dictionary:
            endian = dictionary['endian']
            if endian == 'network' or endian == 'big':
                self.endian = "QMI_ENDIAN_BIG"
            elif endian == 'little':
                pass
            else:
                raise ValueError("Invalid endian value %s" % endian)

        """
        Initially all variables are flagged as not being public
        """
        self.public = False

        # -----------------------------------------------------------------------------
        # Variables to support the new GIR array/struct compat methods in 1.32

        """
        Flag specifying whether a given variable contains any kind of array, indicating
        the GIR array/struct compat methods in 1.32 are required. There is no need to
        flag structs independently, because it is ensured that structs are always array
        members.
        """
        self.needs_compat_gir = False

        """
        Formats and element types
        """
        self.public_format_gir = None
        self.private_format_gir = None
        self.element_type_gir = None

        """
        Types involved in GIR support may need additional free() and new() methods.
        """
        self.new_method_gir = ''
        self.free_method_gir = ''

    """
    Emits the code to declare specific new types required by the variable.
    """
    def emit_types(self, hfile, cfile, since, static):
        pass

    """
    Emits the code involved in reading the variable from the raw byte stream
    into the specific private format.
    """
    def emit_buffer_read(self, f, line_prefix, tlv_out, error, variable_name):
        pass

    """
    Emits the code involved in writing the variable to the raw byte stream
    from the specific private format.
    """
    def emit_buffer_write(self, f, line_prefix, tlv_name, variable_name):
        pass

    """
    Emits the code to get the contents of the given variable as a printable string.
    """
    def emit_get_printable(self, f, line_prefix):
        pass

    """
    Builds the code to include the declaration of a variable of this kind,
    used when generating input/output bundles.
    """
    def build_variable_declaration(self, line_prefix, variable_name):
        return ''

    """
    Builds the code to include in the getter method declaration for this kind of variable.
    """
    def build_getter_declaration(self, line_prefix, variable_name):
        return ''

    """
    Builds the documentation of the getter code
    """
    def build_getter_documentation(self, line_prefix, variable_name):
        return ''

    """
    Builds the code to implement getting this kind of variable.
    """
    def build_getter_implementation(self, line_prefix, variable_name_from, variable_name_to):
        return ''

    """
    Builds the code to include in the setter method for this kind of variable.
    """
    def build_setter_declaration(self, line_prefix, variable_name):
        return ''

    """
    Builds the documentation of the setter code
    """
    def build_setter_documentation(self, line_prefix, variable_name):
        return ''

    """
    Builds the code to implement setting this kind of variable.
    """
    def build_setter_implementation(self, line_prefix, variable_name_from, variable_name_to):
        return ''

    """
    Builds the code to include the declaration of a variable of this kind
    as a field in a public struct
    """
    def build_struct_field_declaration(self, line_prefix, variable_name):
        return ''

    """
    Documentation for the struct field
    """
    def build_struct_field_documentation(self, line_prefix, variable_name):
        return ''

    """
    Emits the code to dispose the variable.
    """
    def build_dispose(self, line_prefix, variable_name):
        return ''

    """
    Add sections
    """
    def add_sections(self, sections):
        pass

    """
    Flag as being public
    """
    def flag_public(self):
        self.public = True

    # -----------------------------------------------------------------------------
    # Support for GIR array/struct compat methods in 1.32

    """
    Emits the code to declare specific new types required by the variable
    """
    def emit_types_gir(self, hfile, cfile, since):
        pass

    """
    Builds the code to include the declaration of a variable of this kind,
    used when generating input/output bundles
    """
    def build_variable_declaration_gir(self, line_prefix, variable_name):
        # By default, no extra variable is required in the input/output bundle
        # for GIR compat methods
        return ''

    """
    Builds the code to include in the getter method declaration for this kind of variable
    """
    def build_getter_declaration_gir(self, line_prefix, variable_name):
        return self.build_getter_declaration(line_prefix, variable_name)

    """
    Builds the documentation of the getter code
    """
    def build_getter_documentation_gir(self, line_prefix, variable_name):
        return self.build_getter_documentation(line_prefix, variable_name)

    """
    Builds the code to implement getting this kind of variable
    """
    def build_getter_implementation_gir(self, line_prefix, variable_name_from, variable_name_to):
        return self.build_getter_implementation(line_prefix, variable_name_from, variable_name_to)

    """
    Builds the code to include in the setter method declaration for this kind of variable
    """
    def build_setter_declaration_gir(self, line_prefix, variable_name):
        return self.build_setter_declaration(line_prefix, variable_name)

    """
    Builds the documentation of the setter code
    """
    def build_setter_documentation_gir(self, line_prefix, variable_name):
        return self.build_setter_documentation(line_prefix, variable_name)

    """
    Builds the code to implement setting this kind of variable
    """
    def build_setter_implementation_gir(self, line_prefix, variable_name_from, variable_name_to):
        return self.build_setter_implementation(line_prefix, variable_name_from, variable_name_to)

    """
    Declaration of the variable as a struct field
    """
    def build_struct_field_declaration_gir(self, line_prefix, variable_name):
        return self.build_struct_field_declaration(line_prefix, variable_name)

    """
    Documentation for the struct field
    """
    def build_struct_field_documentation_gir(self, line_prefix, variable_name):
        return self.build_struct_field_documentation(line_prefix, variable_name)

    """
    Emits the code to dispose the variable
    """
    def build_dispose_gir(self, line_prefix, variable_name):
        return self.build_dispose(line_prefix, variable_name)

    """
    Emits the code to copy a variable into its GIR specific format
    """
    def build_copy_to_gir(self, line_prefix, variable_name_from, variable_name_to):
        return ''

    """
    Emits the code to copy a variable from its GIR specific format
    """
    def build_copy_from_gir(self, line_prefix, variable_name_from, variable_name_to):
        return ''

    """
    Emits the code to copy a variable from its GIR specific format to another
    variable in the same format
    """
    def build_copy_gir(self, line_prefix, variable_name_from, variable_name_to):
        return ''
