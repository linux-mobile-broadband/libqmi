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

def add_copyright(f):
    f.write("\n"
            "/* GENERATED CODE... DO NOT EDIT */\n"
            "\n"
            "/*\n"
            " * This library is free software; you can redistribute it and/or\n"
            " * modify it under the terms of the GNU Lesser General Public\n"
            " * License as published by the Free Software Foundation; either\n"
            " * version 2 of the License, or (at your option) any later version.\n"
            " *\n"
            " * This library is distributed in the hope that it will be useful,\n"
            " * but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
            " * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU\n"
            " * Lesser General Public License for more details.\n"
            " *\n"
            " * You should have received a copy of the GNU Lesser General Public\n"
            " * License along with this library; if not, write to the\n"
            " * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,\n"
            " * Boston, MA 02110-1301 USA.\n"
            " *\n"
            " * Copyright (C) 2012 Lanedo GmbH\n"
            " */\n"
            "\n");

def build_header_guard(output_name):
    return "__LIBQMI_GLIB_" + string.upper(string.replace (output_name, '-', '_')) + "__"

def add_header_start(f, output_name):
    template = string.Template (
        "\n"
        "#ifndef ${guard}\n"
        "#define ${guard}\n"
        "\n"
        "#include <glib.h>\n"
        "#include <glib-object.h>\n"
        "#include <gio/gio.h>\n"
        "\n"
        "#include \"qmi-enum-types.h\"\n"
        "#include \"qmi-message.h\"\n"
        "#include \"qmi-client.h\"\n"
        "\n"
        "G_BEGIN_DECLS\n"
        "\n")
    f.write(template.substitute(guard = build_header_guard(output_name)))

def add_header_stop(f, output_name):
    template = string.Template (
        "\n"
        "G_END_DECLS\n"
        "\n"
        "#endif /* ${guard} */\n")
    f.write(template.substitute(guard = build_header_guard(output_name)))

#def add_global_include(f, header):
#    template = string.Template (
#        "\n"
#        "#include <${header}>\n")
#    f.write(template.substitute(header = header))
#
#def add_local_include(f, header):
#    template = string.Template (
#        "\n"
#        "#include \"${header}\"\n")
#    f.write(template.substitute(header = header))


def add_source_start(f, output_name):
    template = string.Template (
        "\n"
        "#include <string.h>\n"
        "\n"
        "#include \"${name}.h\"\n"
        "#include \"qmi-error-types.h\"\n"
        "#include \"qmi-device.h\"\n"
        "#include \"qmi-utils.h\"\n"
        '\n'
        '#define QMI_STATUS_SUCCESS 0x0000\n'
        '#define QMI_STATUS_FAILURE 0x0001\n'
        "\n")
    f.write(template.substitute(name = output_name))


def add_separator(f, separator_type, separator_name):
    template = string.Template (
        "\n"
        "/*****************************************************************************/\n"
        "/* ${type}: ${name} */\n"
        "\n")
    f.write(template.substitute(type = separator_type,
                                name = separator_name))

def build_underscore_name(name):
    return string.lower(string.replace (name, ' ', '_'))

def build_camelcase_name(name):
    return string.replace(string.capwords(name), ' ', '')


#def emit_struct_type(f, struct_type, dictionary):
#    translations = { 'struct_type' : struct_type }
#    template = (
#        '\n'
#        'typedef struct _${struct_type} {\n')
#    f.write(string.Template(template).substitute(translations))
#    for var in dictionary['contents']:
#        translations['variable_type'] = var['type']
#        translations['variable_name'] = build_underscore_name(var['name'])
#        template = (
#            '    ${variable_type} ${variable_name};\n')
#        f.write(string.Template(template).substitute(translations))
#    template = ('} __attribute__((__packed__)) ${struct_type};\n\n')
#    f.write(string.Template(template).substitute(translations))


def he_from_le(input_variable, output_variable, variable_type):
    if variable_type == 'guint16':
        return '%s = GUINT16_FROM_LE (%s)' % (output_variable, input_variable)
    elif variable_type == 'guint32':
        return '%s = GUINT32_FROM_LE (%s)' % (output_variable, input_variable)
    if variable_type == 'gint16':
        return '%s = GINT16_FROM_LE (%s)' % (output_variable, input_variable)
    elif variable_type == 'gint32':
        return '%s = GINT32_FROM_LE (%s)' % (output_variable, input_variable)
    else:
        return '%s = %s' % (output_variable, input_variable)


def le_from_he(input_variable, output_variable, variable_type):
    return he_from_le(input_variable, output_variable, variable_type)


def read_json_file(path):
    f = open(path)
    out = ''
    for line in f.readlines():
        stripped = line.strip()
        if stripped.startswith('//'):
            # Skip this line
            pass
        else:
            out += line
    return out
