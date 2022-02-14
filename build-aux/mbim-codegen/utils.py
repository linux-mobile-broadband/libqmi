# -*- Mode: python; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
#
# SPDX-License-Identifier: LGPL-2.1-or-later
#
# Copyright (C) 2012 Lanedo GmbH
# Copyright (C) 2013 - 2018 Aleksander Morgado <aleksander@aleksander.es>
#
# Implementation originally developed in 'libqmi'.
#

import string
import re

"""
Add the common copyright header to the given file
"""
def add_copyright(f):
    f.write(
        "\n"
        "/* GENERATED CODE... DO NOT EDIT */\n"
        "\n"
        "/* SPDX-License-Identifier: LGPL-2.1-or-later */\n"
        "/*\n"
        " * Copyright (C) 2013 - 2018 Aleksander Morgado <aleksander@aleksander.es>\n"
        " */\n"
        "\n");


"""
Build a header guard string based on the given filename
"""
def build_header_guard(output_name):
    return "__LIBMBIM_GLIB_" + output_name.replace('-', '_').upper() + "__"

"""
Write the common header start chunk
"""
def add_header_start(f, output_name):
    translations = { 'guard' : build_header_guard(output_name) }
    template = (
        "\n"
        "#include <glib.h>\n"
        "#include <glib-object.h>\n"
        "#include <gio/gio.h>\n"
        "\n"
        "#include \"mbim-message.h\"\n"
        "#include \"mbim-device.h\"\n"
        "#include \"mbim-enums.h\"\n"
        "#include \"mbim-tlv.h\"\n"
        "\n"
        "#ifndef ${guard}\n"
        "#define ${guard}\n"
        "\n"
        "G_BEGIN_DECLS\n")
    f.write(string.Template(template).substitute(translations))


"""
Write the header documentation sections
"""
def add_header_sections(f, input_name):
    translations = { 'section_name' : "mbim-" + remove_prefix(input_name,"mbim-service-"),
                     'service_name' : string.capwords(remove_prefix(input_name,"mbim-service-").replace('-', ' ')) }
    template = (
        "\n"
        "/**\n"
        " * SECTION:${section_name}\n"
        " * @title: ${service_name} service\n"
        " * @short_description: Support for the ${service_name} service.\n"
        " *\n"
        " * This section implements support for requests, responses and notifications in the\n"
        " * ${service_name} service.\n"
        " */\n")
    f.write(string.Template(template).substitute(translations))


"""
Write the common header stop chunk
"""
def add_header_stop(f, output_name):
    template = string.Template (
        "\n"
        "G_END_DECLS\n"
        "\n"
        "#endif /* ${guard} */\n")
    f.write(template.substitute(guard = build_header_guard(output_name)))


"""
Write the common source file start chunk
"""
def add_source_start(f, output_name):
    template = string.Template (
        "\n"
        "#include <string.h>\n"
        "\n"
        "#include \"${name}.h\"\n"
        "#include \"mbim-message-private.h\"\n"
        "#include \"mbim-tlv-private.h\"\n"
        "#include \"mbim-enum-types.h\"\n"
        "#include \"mbim-error-types.h\"\n"
        "#include \"mbim-device.h\"\n"
        "#include \"mbim-utils.h\"\n")
    f.write(template.substitute(name = output_name))


"""
Write a separator comment in the file
"""
def add_separator(f, separator_type, separator_name):
    template = string.Template (
        "\n"
        "/*****************************************************************************/\n"
        "/* ${type}: ${name} */\n")
    f.write(template.substitute(type = separator_type,
                                name = separator_name))


"""
Build an underscore name from the given full name
e.g.: "This is a message" --> "this_is_a_message"
"""
def build_underscore_name(name):
    return name.replace(' ', '_').replace('-', '_').lower()


"""
Build an underscore uppercase name from the given full name
e.g.: "This is a message" --> "THIS_IS_A_MESSAGE"
"""
def build_underscore_uppercase_name(name):
    return name.replace(' ', '_').replace('-', '_').upper()



"""
Build an underscore name from the given camelcase name
e.g.: "ThisIsAMessage" --> "this_is_a_message"
"""
def build_underscore_name_from_camelcase(camelcase):
    s0 = camelcase.replace('IP','Ip')
    s1 = re.sub('(.)([A-Z][a-z]+)', r'\1_\2', s0)
    return re.sub('([a-z0-9])([A-Z])', r'\1_\2', s1).lower()



"""
Build a camelcase name from the given full name
e.g.: "This is a message" --> "ThisIsAMessage"
"""
def build_camelcase_name(name):
    return string.capwords(name).replace(' ', '')


"""
Build a dashed lowercase name from the given full name
e.g.: "This is a message" --> "this-is-a-message"
"""
def build_dashed_name(name):
    return name.lower().replace(' ', '-')


"""
Remove the given prefix from the string
"""
def remove_prefix(line, prefix):
    return line[len(prefix):] if line.startswith(prefix) else line


"""
Read the contents of the JSON file, skipping lines prefixed with '//', which are
considered comments.
"""
def read_json_file(path):
    f = open(path)
    out = ''
    for line in f.readlines():
        stripped = line.strip()
        if stripped.startswith('//'):
            # Skip this line
            # We add an empty line instead so that errors when parsing the JSON
            # report the proper line number
            out += "\n"
        else:
            out += line
    return out

"""
Compare two version strings given in MAJOR.MINOR format.
Just to avoid needing to include e.g. packaging.version.parse just for this
"""
def version_compare(v1,v2):
    major_v1 = int(v1.partition(".")[0])
    major_v2 = int(v2.partition(".")[0])
    if major_v2 > major_v1:
        return 1
    if major_v2 < major_v1:
        return -1
    # major_v2 == major_v1
    minor_v1 = int(v1.partition(".")[2])
    minor_v2 = int(v2.partition(".")[2])
    if minor_v2 > minor_v1:
        return 1
    if minor_v2 < minor_v1:
        return -1
    # minor_v2 == minor_v1
    return 0
