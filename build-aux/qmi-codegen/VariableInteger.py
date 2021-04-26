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
# Copyright (C) 2012-2017 Aleksander Morgado <aleksander@aleksander.es>
#

import string
import utils
from Variable import Variable

"""
Variable type for signed/unsigned Integers and floating point numbers:
 'guint8', 'gint8'
 'guint16', 'gint16'
 'guint32', 'gint32'
 'guint64', 'gint64'
 'guint-sized'
 'gfloat', 'gdouble'
"""
class VariableInteger(Variable):

    """
    Constructor
    """
    def __init__(self, dictionary):

        # Call the parent constructor
        Variable.__init__(self, dictionary)

        self.guint_sized_size = ''
        if self.format == "guint-sized":
            if 'guint-size' not in dictionary:
                raise RuntimeError('Format \'guint-sized\' requires \'guint-size\' parameter')
            else:
                self.guint_sized_size = dictionary['guint-size']
            self.private_format = 'guint64'
            self.public_format  = 'guint64'
        else:
            self.private_format = self.format
            self.public_format = dictionary['public-format'] if 'public-format' in dictionary else self.private_format

        self.element_type = self.public_format

    """
    Read a single integer from the raw byte buffer
    """
    def emit_buffer_read(self, f, line_prefix, tlv_out, error, variable_name):
        translations = { 'lp'             : line_prefix,
                         'tlv_out'        : tlv_out,
                         'variable_name'  : variable_name,
                         'error'          : error,
                         'public_format'  : self.public_format,
                         'private_format' : self.private_format,
                         'len'            : self.guint_sized_size }

        if self.private_format not in ('guint8', 'gint8'):
            translations['endian'] = ' ' + self.endian + ','
        else:
            translations['endian'] = ''

        if self.format == 'guint-sized':
            template = (
                '${lp}if (!qmi_message_tlv_read_sized_guint (message, init_offset, &offset, ${len},${endian} &(${variable_name}), ${error}))\n'
                '${lp}    goto ${tlv_out};\n')
        elif self.format == 'gfloat':
            template = (
                '${lp}if (!qmi_message_tlv_read_gfloat_endian (message, init_offset, &offset,${endian} &(${variable_name}), ${error}))\n'
                '${lp}    goto ${tlv_out};\n')
        elif self.private_format == self.public_format:
            template = (
                '${lp}if (!qmi_message_tlv_read_${private_format} (message, init_offset, &offset,${endian} &(${variable_name}), ${error}))\n'
                '${lp}    goto ${tlv_out};\n')
        else:
            template = (
                '${lp}{\n'
                '${lp}    ${private_format} tmp;\n'
                '\n'
                '${lp}    if (!qmi_message_tlv_read_${private_format} (message, init_offset, &offset,${endian} &tmp, ${error}))\n'
                '${lp}        goto ${tlv_out};\n'
                '${lp}    ${variable_name} = (${public_format})tmp;\n'
                '${lp}}\n')
        f.write(string.Template(template).substitute(translations))


    """
    Return the data type size of fixed c-types
    """
    @staticmethod
    def fixed_type_byte_size(fmt):
        if fmt == 'guint8':
            return 1
        if fmt == 'guint16':
            return 2
        if fmt == 'guint32':
            return 4
        if fmt == 'guint64':
            return 8
        if fmt == 'gint8':
            return 1
        if fmt == 'gint16':
            return 2
        if fmt == 'gint32':
            return 4
        if fmt == 'gint64':
            return 8
        raise Exception("Unsupported format %s" % (fmt))

    """
    Write a single integer to the raw byte buffer
    """
    def emit_buffer_write(self, f, line_prefix, tlv_name, variable_name):
        translations = { 'lp'             : line_prefix,
                         'private_format' : self.private_format,
                         'len'            : self.guint_sized_size,
                         'tlv_name'       : tlv_name,
                         'variable_name'  : variable_name }

        if self.private_format != 'guint8' and self.private_format != 'gint8':
            translations['endian'] = ' ' + self.endian + ','
        else:
            translations['endian'] = ''

        if self.format == 'guint-sized':
            template = (
                '${lp}/* Write the ${len}-byte long variable to the buffer */\n'
                '${lp}if (!qmi_message_tlv_write_sized_guint (self, ${len},${endian} ${variable_name}, error)) {\n'
                '${lp}    g_prefix_error (error, "Cannot write sized integer in TLV \'${tlv_name}\': ");\n'
                '${lp}    return NULL;\n'
                '${lp}}\n')
        elif self.private_format == self.public_format:
            template = (
                '${lp}/* Write the ${private_format} variable to the buffer */\n'
                '${lp}if (!qmi_message_tlv_write_${private_format} (self,${endian} ${variable_name}, error)) {\n'
                '${lp}    g_prefix_error (error, "Cannot write integer in TLV \'${tlv_name}\': ");\n'
                '${lp}    return NULL;\n'
                '${lp}}\n')
        else:
            template = (
                '${lp}{\n'
                '${lp}    ${private_format} tmp;\n'
                '\n'
                '${lp}    tmp = (${private_format}) ${variable_name};\n'
                '${lp}    /* Write the ${private_format} variable to the buffer */\n'
                '${lp}    if (!qmi_message_tlv_write_${private_format} (self,${endian} tmp, error)) {\n'
                '${lp}        g_prefix_error (error, "Cannot write enum in TLV \'${tlv_name}\': ");\n'
                '${lp}        return NULL;\n'
                '${lp}    }\n'
                '${lp}}\n')
        f.write(string.Template(template).substitute(translations))


    """
    Get the integer as a printable string.
    """
    def emit_get_printable(self, f, line_prefix):
        common_format = ''
        common_cast = ''

        if self.private_format == 'guint8':
            common_format = '%u'
            common_cast = '(guint)'
        elif self.private_format == 'guint16':
            common_format = '%" G_GUINT16_FORMAT "'
        elif self.private_format == 'guint32':
            common_format = '%" G_GUINT32_FORMAT "'
        elif self.private_format == 'guint64':
            common_format = '%" G_GUINT64_FORMAT "'
        elif self.private_format == 'gint8':
            common_format = '%d'
            common_cast = '(gint)'
        elif self.private_format == 'gint16':
            common_format = '%" G_GINT16_FORMAT "'
        elif self.private_format == 'gint32':
            common_format = '%" G_GINT32_FORMAT "'
        elif self.private_format == 'gint64':
            common_format = '%" G_GINT64_FORMAT "'
        elif self.private_format in ('gfloat', 'gdouble'):
            common_format = '%lf'
            common_cast = '(gdouble)'

        translations = { 'lp'             : line_prefix,
                         'private_format' : self.private_format,
                         'public_format'  : self.public_format,
                         'len'            : self.guint_sized_size,
                         'common_format'  : common_format,
                         'common_cast'    : common_cast }

        if self.private_format not in ('guint8', 'gint8'):
            translations['endian'] = ' ' + self.endian + ','
        else:
            translations['endian'] = ''

        template = (
            '\n'
            '${lp}{\n'
            '${lp}    ${private_format} tmp;\n'
            '\n')

        if self.format == 'guint-sized':
            template += (
                '${lp}    if (!qmi_message_tlv_read_sized_guint (message, init_offset, &offset, ${len},${endian} &tmp, &error))\n'
                '${lp}        goto out;\n')
        elif self.format == 'gfloat':
            template += (
                '${lp}    if (!qmi_message_tlv_read_gfloat_endian (message, init_offset, &offset,${endian} &tmp, &error))\n'
                '${lp}        goto out;\n')
        else:
            template += (
                '${lp}    if (!qmi_message_tlv_read_${private_format} (message, init_offset, &offset,${endian} &tmp, &error))\n'
                '${lp}        goto out;\n')

        if self.public_format == 'gboolean':
            template += (
                '${lp}    g_string_append_printf (printable, "%s", tmp ? "yes" : "no");\n')
        elif self.public_format != self.private_format:
            translations['public_type_underscore'] = utils.build_underscore_name_from_camelcase(self.public_format)
            translations['public_type_underscore_upper'] = utils.build_underscore_name_from_camelcase(self.public_format).upper()
            template += (
                '#if defined  __${public_type_underscore_upper}_IS_ENUM__\n'
                '${lp}    g_string_append_printf (printable, "%s", ${public_type_underscore}_get_string ((${public_format})tmp));\n'
                '#elif defined  __${public_type_underscore_upper}_IS_FLAGS__\n'
                '${lp}    {\n'
                '${lp}        g_autofree gchar *flags_str = NULL;\n'
                '\n'
                '${lp}        flags_str = ${public_type_underscore}_build_string_from_mask ((${public_format})tmp);\n'
                '${lp}        g_string_append_printf (printable, "%s", flags_str);\n'
                '${lp}    }\n'
                '#else\n'
                '# error unexpected public format: ${public_format}\n'
                '#endif\n')
        else:
            template += (
                '${lp}    g_string_append_printf (printable, "${common_format}", ${common_cast}tmp);\n')

        template += (
            '${lp}}\n')

        f.write(string.Template(template).substitute(translations))


    """
    Variable declaration
    """
    def build_variable_declaration(self, public, line_prefix, variable_name):
        translations = { 'lp'             : line_prefix,
                         'private_format' : self.private_format,
                         'public_format'  : self.public_format,
                         'name'           : variable_name }
        template = ''
        if public:
            template += (
                '${lp}${public_format} ${name};\n')
        else:
            template += (
                '${lp}${private_format} ${name};\n')
        return string.Template(template).substitute(translations)


    """
    Getter for the integer type
    """
    def build_getter_declaration(self, line_prefix, variable_name):
        if not self.visible:
            return ""

        translations = { 'lp'            : line_prefix,
                         'public_format' : self.public_format,
                         'name'          : variable_name }

        template = (
            '${lp}${public_format} *${name},\n')
        return string.Template(template).substitute(translations)


    """
    Documentation for the getter
    """
    def build_getter_documentation(self, line_prefix, variable_name):
        if not self.visible:
            return ""

        translations = { 'lp'            : line_prefix,
                         'public_format' : self.public_format,
                         'name'          : variable_name }

        template = (
            '${lp}@${name}: (out)(optional): a placeholder for the output #${public_format}, or %NULL if not required.\n')
        return string.Template(template).substitute(translations)

    """
    Builds the Integer getter implementation
    """
    def build_getter_implementation(self, line_prefix, variable_name_from, variable_name_to, to_is_reference):
        if not self.visible:
            return ""

        needs_cast = True if self.public_format != self.private_format else False
        translations = { 'lp'       : line_prefix,
                         'from'     : variable_name_from,
                         'to'       : variable_name_to,
                         'cast_ini' : '(' + self.public_format + ')(' if needs_cast else '',
                         'cast_end' : ')' if needs_cast else '' }

        if to_is_reference:
            template = (
                '${lp}if (${to})\n'
                '${lp}    *${to} = ${cast_ini}${from}${cast_end};\n')
            return string.Template(template).substitute(translations)
        else:
            template = (
                '${lp}${to} = ${cast_ini}${from}${cast_end};\n')
            return string.Template(template).substitute(translations)


    """
    Setter for the integer type
    """
    def build_setter_declaration(self, line_prefix, variable_name):
        if not self.visible:
            return ""

        translations = { 'lp'            : line_prefix,
                         'public_format' : self.public_format,
                         'name'          : variable_name }

        template = (
            '${lp}${public_format} ${name},\n')
        return string.Template(template).substitute(translations)


    """
    Documentation for the setter
    """
    def build_setter_documentation(self, line_prefix, variable_name):
        if not self.visible:
            return ""

        translations = { 'lp'            : line_prefix,
                         'public_format' : self.public_format,
                         'name'          : variable_name }

        template = (
            '${lp}@${name}: a #${public_format}.\n')
        return string.Template(template).substitute(translations)


    """
    Implementation of the setter
    """
    def build_setter_implementation(self, line_prefix, variable_name_from, variable_name_to):
        if not self.visible:
            return ""

        needs_cast = True if self.public_format != self.private_format else False
        translations = { 'lp'       : line_prefix,
                         'from'     : variable_name_from,
                         'to'       : variable_name_to,
                         'cast_ini' : '(' + self.private_format + ')(' if needs_cast else '',
                         'cast_end' : ')' if needs_cast else '' }

        template = (
            '${lp}${to} = ${cast_ini}${from}${cast_end};\n')
        return string.Template(template).substitute(translations)


    """
    Documentation for the struct field
    """
    def build_struct_field_documentation(self, line_prefix, variable_name):
        translations = { 'lp'            : line_prefix,
                         'public_format' : self.public_format,
                         'name'          : variable_name }

        template = (
            '${lp}@${name}: a #${public_format}.\n')
        return string.Template(template).substitute(translations)
