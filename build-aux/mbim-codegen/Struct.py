# -*- Mode: python; tab-width: 4; indent-tabs-mode: nil -*-
#
# SPDX-License-Identifier: LGPL-2.1-or-later
#
# Copyright (C) 2013 - 2018 Aleksander Morgado <aleksander@aleksander.es>
#

import string
import utils

"""
The Struct class takes care of emitting the struct type
"""
class Struct:

    """
    Constructor
    """
    def __init__(self, service, mbimex_service, mbimex_version, dictionary):
        # The message service, e.g. "Basic Connect"
        self.service = service
        self.mbimex_service = mbimex_service
        self.mbimex_version = mbimex_version

        self.name = dictionary['name']
        self.contents = dictionary['contents']
        self.since = dictionary['since'] if 'since' in dictionary else None
        if self.since is None:
            raise ValueError('Struct ' + self.name + ' requires a "since" tag specifying the major version where it was introduced')

        # Whether the struct is used as a single field, or as an array of
        # fields. Will be updated after having created the object.
        self.single_member = False
        self.ms_struct = False
        self.ref_struct_array_member = False
        self.struct_array_member = False
        self.ms_struct_array_member = False

        # Check whether the struct is composed of fixed-sized fields
        self.size = 0
        for field in self.contents:
            if field['format'] == 'guint16':
                self.size += 2
            elif field['format'] == 'guint32':
                self.size += 4
            elif field['format'] == 'gint32':
                self.size += 4
            elif field['format'] == 'guint64':
                self.size += 8
            elif field['format'] == 'uuid':
                self.size += 16
            elif field['format'] == 'ipv4':
                self.size += 4
            elif field['format'] == 'ipv6':
                self.size += 16
            else:
                self.size = 0
                break


    """
    Emit the new struct type
    """
    def _emit_type(self, hfile):
        translations = { 'name'  : self.name,
                         'since' : self.since }
        template = (
            '\n'
            '/**\n'
            ' * ${name}:\n')
        for field in self.contents:
            translations['field_name_underscore'] = utils.build_underscore_name_from_camelcase(field['name'])
            if field['format'] == 'uuid':
                inner_template = (
                    ' * @${field_name_underscore}: a #MbimUuid.\n')
            elif field['format'] =='byte-array':
                inner_template = (' * @${field_name_underscore}: an array of #guint8 values.\n')
            elif field['format'] in ['unsized-byte-array', 'ref-byte-array', 'ref-byte-array-no-offset']:
                inner_template = ''
                if 'array-size-field' not in field:
                    inner_template += (' * @${field_name_underscore}_size: size of the ${field_name_underscore} array.\n')
                inner_template += (' * @${field_name_underscore}: an array of #guint8 values.\n')
            elif field['format'] == 'guint16':
                if 'public-format' in field:
                    translations['public'] = field['public-format']
                    inner_template = (
                        ' * @${field_name_underscore}: a #${public} given as a #guint16.\n')
                else:
                    inner_template = (
                        ' * @${field_name_underscore}: a #guint16.\n')
            elif field['format'] == 'guint32':
                if 'public-format' in field:
                    translations['public'] = field['public-format']
                    inner_template = (
                        ' * @${field_name_underscore}: a #${public} given as a #guint32.\n')
                else:
                    inner_template = (
                        ' * @${field_name_underscore}: a #guint32.\n')
            elif field['format'] == 'gint32':
                inner_template = (
                    ' * @${field_name_underscore}: a #gint32.\n')
            elif field['format'] == 'guint32-array':
                inner_template = (
                    ' * @${field_name_underscore}: an array of #guint32 values.\n')
            elif field['format'] == 'guint64':
                if 'public-format' in field:
                    translations['public'] = field['public-format']
                    inner_template = (
                        ' * @${field_name_underscore}: a #${public} given as a #guint64.\n')
                else:
                    inner_template = (
                        ' * @${field_name_underscore}: a #guint64.\n')
            elif field['format'] == 'string':
                inner_template = (
                    ' * @${field_name_underscore}: a string.\n')
            elif field['format'] == 'string-array':
                inner_template = (
                    ' * @${field_name_underscore}: an array of strings.\n')
            elif field['format'] == 'ipv4':
                inner_template = (
                    ' * @${field_name_underscore}: a #MbimIPv4.\n')
            elif field['format'] == 'ipv6':
                inner_template = (
                    ' * @${field_name_underscore}: a #MbimIPv6\n')
            else:
                raise ValueError('Cannot handle format \'%s\' in struct' % field['format'])
            template += string.Template(inner_template).substitute(translations)

        template += (
            ' *\n'
            ' * A ${name} element.\n'
            ' *\n'
            ' * Since: ${since}\n'
            ' */\n'
            'typedef struct {\n')
        for field in self.contents:
            translations['field_name_underscore'] = utils.build_underscore_name_from_camelcase(field['name'])
            if field['format'] == 'uuid':
                inner_template = (
                    '    MbimUuid ${field_name_underscore};\n')
            elif field['format'] == 'byte-array':
                translations['array_size'] = field['array-size']
                inner_template = (
                    '    guint8 ${field_name_underscore}[${array_size}];\n')
            elif field['format'] in ['unsized-byte-array', 'ref-byte-array', 'ref-byte-array-no-offset']:
                inner_template = ''
                if 'array-size-field' not in field:
                    inner_template += (
                        '    guint32 ${field_name_underscore}_size;\n')
                inner_template += (
                    '    guint8 *${field_name_underscore};\n')
            elif field['format'] == 'guint16':
                inner_template = (
                    '    guint16 ${field_name_underscore};\n')
            elif field['format'] == 'guint32':
                inner_template = (
                    '    guint32 ${field_name_underscore};\n')
            elif field['format'] == 'gint32':
                inner_template = (
                    '    gint32 ${field_name_underscore};\n')
            elif field['format'] == 'guint32-array':
                inner_template = (
                    '    guint32 *${field_name_underscore};\n')
            elif field['format'] == 'guint64':
                inner_template = (
                    '    guint64 ${field_name_underscore};\n')
            elif field['format'] == 'string':
                inner_template = (
                    '    gchar *${field_name_underscore};\n')
            elif field['format'] == 'string-array':
                inner_template = (
                    '    gchar **${field_name_underscore};\n')
            elif field['format'] == 'ipv4':
                inner_template = (
                    '    MbimIPv4 ${field_name_underscore};\n')
            elif field['format'] == 'ipv6':
                inner_template = (
                    '    MbimIPv6 ${field_name_underscore};\n')
            else:
                raise ValueError('Cannot handle format \'%s\' in struct' % field['format'])
            template += string.Template(inner_template).substitute(translations)
        template += (
            '} ${name};\n')
        hfile.write(string.Template(template).substitute(translations))


    """
    Emit the type's free methods
    """
    def _emit_free(self, hfile, cfile):
        translations = { 'name'  : self.name,
                         'since' : self.since,
                         'name_underscore' : utils.build_underscore_name_from_camelcase(self.name) }
        template = ''

        if self.single_member == True:
            template = (
                '\n'
                '/**\n'
                ' * ${name_underscore}_free:\n'
                ' * @var: a #${name}.\n'
                ' *\n'
                ' * Frees the memory allocated for the #${name}.\n'
                ' *\n'
                ' * Since: ${since}\n'
                ' */\n'
                'void ${name_underscore}_free (${name} *var);\n'
                'G_DEFINE_AUTOPTR_CLEANUP_FUNC (${name}, ${name_underscore}_free)\n')
            hfile.write(string.Template(template).substitute(translations))


        template = (
            '\n'
            'static void\n'
            '_${name_underscore}_free (${name} *var)\n'
            '{\n'
            '    if (!var)\n'
            '        return;\n'
            '\n')

        for field in self.contents:
            translations['field_name_underscore'] = utils.build_underscore_name_from_camelcase(field['name'])
            inner_template = ''
            if field['format'] == 'uuid':
                pass
            elif field['format'] in ['unsized-byte-array', 'ref-byte-array', 'ref-byte-array-no-offset']:
                inner_template += (
                    '    g_free (var->${field_name_underscore});\n')
            elif field['format'] == 'guint16':
                pass
            elif field['format'] == 'guint32':
                pass
            elif field['format'] == 'gint32':
                pass
            elif field['format'] == 'guint32-array':
                inner_template += (
                    '    g_free (var->${field_name_underscore});\n')
            elif field['format'] == 'guint64':
                pass
            elif field['format'] == 'string':
                inner_template += (
                    '    g_free (var->${field_name_underscore});\n')
            elif field['format'] == 'string-array':
                inner_template += (
                    '    g_strfreev (var->${field_name_underscore});\n')
            elif field['format'] == 'ipv4':
                pass
            elif field['format'] == 'ipv6':
                pass
            else:
                raise ValueError('Cannot handle format \'%s\' in struct clear' % field['format'])
            template += string.Template(inner_template).substitute(translations)

        template += (
            '    g_free (var);\n'
            '}\n')
        cfile.write(string.Template(template).substitute(translations))

        if self.single_member == True:
            template = (
                '\n'
                'void\n'
                '${name_underscore}_free (${name} *var)\n'
                '{\n'
                '    _${name_underscore}_free (var);\n'
                '}\n')
            cfile.write(string.Template(template).substitute(translations))

        if self.struct_array_member == True or self.ref_struct_array_member == True or self.ms_struct_array_member == True:
            # TypeArray was introduced in 1.24
            translations['array_since'] = self.since if utils.version_compare('1.24', self.since) > 0 else '1.24'
            template = (
                '\n'
                '/**\n'
                ' * ${name}Array:\n'
                ' *\n'
                ' * A NULL-terminated array of ${name} elements.\n'
                ' *\n'
                ' * Since: ${array_since}\n'
                ' */\n'
                'typedef ${name} *${name}Array;\n'
                '/**\n'
                ' * ${name_underscore}_array_free:\n'
                ' * @array: a #NULL terminated array of #${name} structs.\n'
                ' *\n'
                ' * Frees the memory allocated for the array of #${name} structs.\n'
                ' *\n'
                ' * Since: ${since}\n'
                ' */\n'
                'void ${name_underscore}_array_free (${name}Array *array);\n'
                'G_DEFINE_AUTOPTR_CLEANUP_FUNC (${name}Array, ${name_underscore}_array_free)\n')
            hfile.write(string.Template(template).substitute(translations))

            template = (
                '\n'
                'void\n'
                '${name_underscore}_array_free (${name}Array *array)\n'
                '{\n'
                '    guint32 i;\n'
                '\n'
                '    if (!array)\n'
                '        return;\n'
                '\n'
                '    for (i = 0; array[i]; i++)\n'
                '        _${name_underscore}_free (array[i]);\n'
                '    g_free (array);\n'
                '}\n')
            cfile.write(string.Template(template).substitute(translations))

    """
    Emit the type's print methods
    """
    def _emit_print(self, cfile):
        translations = { 'name'            : self.name,
                         'name_underscore' : utils.build_underscore_name_from_camelcase(self.name),
                         'struct_size'     : self.size }

        template = (
            '\n'
            'static gchar *\n'
            '_mbim_message_print_${name_underscore}_struct (\n'
            '    const ${name} *self,\n'
            '    const gchar *line_prefix)\n'
            '{\n'
            '    GString *str;\n')

        for field in self.contents:
            if 'personal-info' in field:
                template += (
                    '    gboolean show_field;\n'
                    '\n'
                    '    show_field = mbim_utils_get_show_personal_info ();\n')
                break

        template += (
            '\n'
            '    str = g_string_new ("");\n'
            '\n')

        for field in self.contents:
            translations['field_name']              = field['name']
            translations['field_name_underscore']   = utils.build_underscore_name_from_camelcase(field['name'])
            translations['public']                  = field['public-format'] if 'public-format' in field else field['format']
            translations['public_underscore']       = utils.build_underscore_name_from_camelcase(field['public-format']) if 'public-format' in field else ''
            translations['public_underscore_upper'] = utils.build_underscore_name_from_camelcase(field['public-format']).upper() if 'public-format' in field else ''

            if 'personal-info' in field:
                translations['if_show_field'] = 'if (show_field) '
            else:
                translations['if_show_field'] = ''

            inner_template = (
                '    g_string_append_printf (str, "%s  ${field_name} = ", line_prefix);\n'
                '    {\n')

            if field['format'] == 'uuid':
                inner_template += (
                    '        ${if_show_field}{\n'
                    '            g_autofree gchar *tmpstr = NULL;\n'
                    '\n'
                    '            tmpstr = mbim_uuid_get_printable (&(self->${field_name_underscore}));\n'
                    '            g_string_append_printf (str, "\'%s\'", tmpstr);\n'
                    '        }\n')

            elif field['format'] in ['byte-array', 'ref-byte-array', 'ref-byte-array-no-offset', 'unsized-byte-array']:
                inner_template += (
                    '        ${if_show_field}{\n'
                    '            guint i;\n'
                    '            guint array_size;\n'
                    '\n')

                if field['format'] == 'byte-array':
                    translations['array_size'] = field['array-size']
                    inner_template += (
                        '            array_size = ${array_size};\n')
                elif 'array-size-field' in field:
                    translations['array_size_field_name_underscore'] = utils.build_underscore_name_from_camelcase(field['array-size-field'])
                    inner_template += (
                        '            array_size = self->${array_size_field_name_underscore};\n')
                else:
                    inner_template += (
                        '            array_size = self->${field_name_underscore}_size;\n')

                inner_template += (
                    '            g_string_append (str, "\'");\n'
                    '            for (i = 0; i < array_size; i++)\n'
                    '                g_string_append_printf (str, "%02x%s", self->${field_name_underscore}[i], (i == (array_size - 1)) ? "" : ":" );\n'
                    '            g_string_append (str, "\'");\n'
                    '        }\n')

            elif field['format'] in ['guint16', 'guint32', 'guint64']:
                if 'public-format' in field:
                    if field['public-format'] == 'gboolean':
                        inner_template += (
                            '        ${if_show_field}{\n'
                            '            g_string_append_printf (str, "\'%s\'", (${public})self->${field_name_underscore} ? "true" : "false");\n'
                            '        }\n'
                            '\n')
                    else:
                        inner_template += (
                            '        ${if_show_field}{\n'
                            '#if defined __${public_underscore_upper}_IS_ENUM__\n'
                            '            g_string_append_printf (str, "\'%s\'", ${public_underscore}_get_string ((${public})self->${field_name_underscore}));\n'
                            '#elif defined __${public_underscore_upper}_IS_FLAGS__\n'
                            '            g_autofree gchar *tmpstr = NULL;\n'
                            '\n'
                            '            tmpstr = ${public_underscore}_build_string_from_mask ((${public})self->${field_name_underscore});\n'
                            '            g_string_append_printf (str, "\'%s\'", tmpstr);\n'
                            '#else\n'
                            '# error neither enum nor flags\n'
                            '#endif\n'
                            '        }\n'
                            '\n')

                elif field['format'] == 'guint16':
                    inner_template += (
                        '        ${if_show_field}{\n'
                        '            g_string_append_printf (str, "\'%" G_GUINT16_FORMAT "\'", self->${field_name_underscore});\n'
                        '        }\n')
                elif field['format'] == 'guint32':
                    inner_template += (
                        '        ${if_show_field}{\n'
                        '            g_string_append_printf (str, "\'%" G_GUINT32_FORMAT "\'", self->${field_name_underscore});\n'
                        '        }\n')
                elif field['format'] == 'guint64':
                    inner_template += (
                        '        ${if_show_field}{\n'
                        '            g_string_append_printf (str, "\'%" G_GUINT64_FORMAT "\'", self->${field_name_underscore});\n'
                        '        }\n')
            elif field['format'] == 'gint32':
                inner_template += (
                    '        ${if_show_field}{\n'
                    '            g_string_append_printf (str, "\'%" G_GINT32_FORMAT "\'", self->${field_name_underscore});\n'
                    '        }\n')
            elif field['format'] == 'guint32-array':
                translations['array_size_field_name_underscore'] = utils.build_underscore_name_from_camelcase(field['array-size-field'])
                inner_template += (
                    '        ${if_show_field}{\n'
                    '            guint i;\n'
                    '\n'
                    '            g_string_append (str, "\'");\n'
                    '            for (i = 0; i < self->${array_size_field_name_underscore}; i++)\n'
                    '                g_string_append_printf (str, "%" G_GUINT32_FORMAT "%s", self->${field_name_underscore}[i], (i == (self->${array_size_field_name_underscore} - 1)) ? "" : "," );\n'
                    '            g_string_append (str, "\'");\n'
                    '        }\n')

            elif field['format'] == 'string':
                inner_template += (
                    '        ${if_show_field}{\n'
                    '            g_string_append_printf (str, "\'%s\'", self->${field_name_underscore});\n'
                    '        }\n')

            elif field['format'] == 'string-array':
                translations['array_size_field_name_underscore'] = utils.build_underscore_name_from_camelcase(field['array-size-field'])
                inner_template += (
                    '        ${if_show_field}{\n'
                    '            guint i;\n'
                    '\n'
                    '            g_string_append (str, "\'");\n'
                    '            for (i = 0; i < self->${array_size_field_name_underscore}; i++)\n'
                    '                g_string_append_printf (str, "%s%s", self->${field_name_underscore}[i], (i == (self->${array_size_field_name_underscore} - 1)) ? "" : "," );\n'
                    '            g_string_append (str, "\'");\n'
                    '        }\n')

            elif field['format'] == 'ipv4' or \
                 field['format'] == 'ipv6':
                inner_template += (
                    '        ${if_show_field}{\n'
                    '            g_autoptr(GInetAddress)  addr = NULL;\n'
                    '            g_autofree gchar        *tmpstr = NULL;\n'
                    '\n')

                if field['format'] == 'ipv4':
                    inner_template += (
                        '            addr = g_inet_address_new_from_bytes ((guint8 *)&(self->${field_name_underscore}.addr), G_SOCKET_FAMILY_IPV4);\n')
                elif field['format'] == 'ipv6':
                    inner_template += (
                        '            addr = g_inet_address_new_from_bytes ((guint8 *)&(self->${field_name_underscore}.addr), G_SOCKET_FAMILY_IPV6);\n')

                inner_template += (
                    '            tmpstr = g_inet_address_to_string (addr);\n'
                    '            g_string_append_printf (str, "\'%s\'", tmpstr);\n'
                    '        }\n')

            else:
                raise ValueError('Cannot handle format \'%s\' in struct' % field['format'])

            if 'personal-info' in field:
                inner_template += (
                    '        if (!show_field)\n'
                    '           g_string_append (str, "\'###\'");\n')

            inner_template += (
                '    }\n'
                '    g_string_append (str, "\\n");\n')
            template += (string.Template(inner_template).substitute(translations))

        template += (
            '    return g_string_free (str, FALSE);\n'
            '}\n')
        cfile.write(string.Template(template).substitute(translations))

    """
    Emit the type's read methods
    """
    def _emit_read(self, cfile):
        translations = { 'name'            : self.name,
                         'name_underscore' : utils.build_underscore_name_from_camelcase(self.name),
                         'struct_size'     : self.size }

        template = (
            '\n'
            'static ${name} *\n'
            '_mbim_message_read_${name_underscore}_struct (\n'
            '    const MbimMessage *self,\n'
            '    guint32 relative_offset,\n'
            '    guint32 explicit_struct_size,\n'
            '    guint32 *bytes_read,\n'
            '    GError **error)\n'
            '{\n'
            '    gboolean success = FALSE;\n'
            '    ${name} *out;\n'
            '    guint32 offset = relative_offset;\n')
        if self.ms_struct_array_member == True:
            template += (
                '    guint32 extra_bytes_read = 0;\n')

        template += (
            '\n'
            '    g_assert (self != NULL);\n'
            '\n'
            '    out = g_new0 (${name}, 1);\n'
            '\n')

        for field in self.contents:
            translations['field_name_underscore'] = utils.build_underscore_name_from_camelcase(field['name'])

            inner_template = ''
            if field['format'] == 'uuid':
                inner_template += (
                    '\n'
                    '    if (!_mbim_message_read_uuid (self, offset, NULL, &(out->${field_name_underscore}), error))\n'
                    '        goto out;\n'
                    '    offset += 16;\n')
            elif field['format'] in ['ref-byte-array', 'ref-byte-array-no-offset']:
                # Unsupported because ms-struct-array requires the read bytes of the struct to contain the size read
                # fro the variable buffer, which is currently not implemented for this type.
                if self.ms_struct_array_member == True:
                    raise ValueError('type unsupported in \'ms-struct-array\'')

                translations['has_offset'] = 'TRUE' if field['format'] == 'ref-byte-array' else 'FALSE'
                if 'array-size-field' in field:
                    translations['array_size_field_name_underscore'] = utils.build_underscore_name_from_camelcase(field['array-size-field'])
                    inner_template += (
                        '\n'
                        '    {\n'
                        '        const guint8 *tmp;\n'
                        '\n'
                        '        if (!_mbim_message_read_byte_array (self, relative_offset, offset, ${has_offset}, FALSE, out->${array_size_field_name_underscore}, &tmp, NULL, error, FALSE))\n'
                        '            goto out;\n'
                        '        out->${field_name_underscore} = g_malloc (out->${array_size_field_name_underscore});\n'
                        '        memcpy (out->${field_name_underscore}, tmp, out->${array_size_field_name_underscore});\n'
                        '        offset += 4;\n'
                        '    }\n')
                else:
                    inner_template += (
                        '\n'
                        '    {\n'
                        '        const guint8 *tmp;\n'
                        '\n'
                        '        if (!_mbim_message_read_byte_array (self, relative_offset, offset, ${has_offset}, TRUE, 0, &tmp, &(out->${field_name_underscore}_size), error, FALSE))\n'
                        '            goto out;\n'
                        '        out->${field_name_underscore} = g_malloc (out->${field_name_underscore}_size);\n'
                        '        memcpy (out->${field_name_underscore}, tmp, out->${field_name_underscore}_size);\n'
                        '        offset += 8;\n'
                        '    }\n')
            elif field['format'] == 'unsized-byte-array':
                # Unsupported because ms-struct-array requires the read bytes of the struct to contain the size read
                # fro the variable buffer, which is currently not implemented for this type.
                if self.ms_struct_array_member == True:
                    raise ValueError('type unsupported in \'ms-struct-array\'')

                inner_template += (
                    '\n'
                    '    {\n'
                    '        const guint8 *tmp;\n'
                    '\n'
                    '        if (!_mbim_message_read_byte_array (self, relative_offset, offset, FALSE, FALSE, 0, &tmp, &(out->${field_name_underscore}_size), error, FALSE))\n'
                    '            goto out;\n'
                    '        out->${field_name_underscore} = g_malloc (out->${field_name_underscore}_size);\n'
                    '        memcpy (out->${field_name_underscore}, tmp, out->${field_name_underscore}_size);\n'
                    '        /* no offset update expected, this should be the last field */\n'
                    '    }\n')
            elif field['format'] == 'byte-array':
                translations['array_size'] = field['array-size']
                inner_template += (
                    '\n'
                    '    {\n'
                    '        const guint8 *tmp;\n'
                    '\n'
                    '        if (!_mbim_message_read_byte_array (self, relative_offset, offset, FALSE, FALSE, ${array_size}, &tmp, NULL, error, FALSE))\n'
                    '            goto out;\n'
                    '        memcpy (out->${field_name_underscore}, tmp, ${array_size});\n'
                    '        offset += ${array_size};\n'
                    '    }\n')
            elif field['format'] == 'guint16':
                inner_template += (
                    '\n'
                    '    if (!_mbim_message_read_guint16 (self, offset, &out->${field_name_underscore}, error))\n'
                    '        goto out;\n'
                    '    offset += 2;\n')
            elif field['format'] == 'guint32':
                inner_template += (
                    '\n'
                    '    if (!_mbim_message_read_guint32 (self, offset, &out->${field_name_underscore}, error))\n'
                    '        goto out;\n'
                    '    offset += 4;\n')
            elif field['format'] == 'gint32':
                inner_template += (
                    '\n'
                    '    if (!_mbim_message_read_gint32 (self, offset, &out->${field_name_underscore}, error))\n'
                    '        goto out;\n'
                    '    offset += 4;\n')
            elif field['format'] == 'guint32-array':
                translations['array_size_field_name_underscore'] = utils.build_underscore_name_from_camelcase(field['array-size-field'])
                inner_template += (
                    '\n'
                    '    if (!_mbim_message_read_guint32_array (self, out->${array_size_field_name_underscore}, offset, &out->${field_name_underscore}, error))\n'
                    '        goto out;\n'
                    '    offset += (4 * out->${array_size_field_name_underscore});\n')
            elif field['format'] == 'guint64':
                inner_template += (
                    '\n'
                    '    if (!_mbim_message_read_guint64 (self, offset, &out->${field_name_underscore}, error))\n'
                    '        goto out;\n'
                    '    offset += 8;\n')
            elif field['format'] == 'string':
                translations['encoding'] = 'MBIM_STRING_ENCODING_UTF8' if 'encoding' in field and field['encoding'] == 'utf-8' else 'MBIM_STRING_ENCODING_UTF16'
                if self.ms_struct_array_member == True:
                    inner_template += (
                        '\n'
                        '    {\n'
                        '        guint32 str_bytes_read;\n'
                        '\n'
                        '        if (!_mbim_message_read_string (self, relative_offset, offset, ${encoding}, &out->${field_name_underscore}, &str_bytes_read, error))\n'
                        '            goto out;\n'
                        '        if (str_bytes_read % 4)\n'
                        '            str_bytes_read = (str_bytes_read + (4 - (str_bytes_read % 4)));\n'
                        '        extra_bytes_read += str_bytes_read;\n'
                        '        offset += 8;\n'
                        '    }\n')
                else:
                    inner_template += (
                        '\n'
                        '    if (!_mbim_message_read_string (self, relative_offset, offset, ${encoding}, &out->${field_name_underscore}, NULL, error))\n'
                        '        goto out;\n'
                        '    offset += 8;\n')
            elif field['format'] == 'string-array':
                # Unsupported because ms-struct-array requires the read bytes of the struct to contain the size read
                # fro the variable buffer, which is currently not implemented for this type.
                if self.ms_struct_array_member == True:
                    raise ValueError('type \'ref-byte-array\' unsupported in \'ms-struct-array\'')

                translations['encoding'] = 'MBIM_STRING_ENCODING_UTF8' if 'encoding' in field and field['encoding'] == 'utf-8' else 'MBIM_STRING_ENCODING_UTF16'
                translations['array_size_field_name_underscore'] = utils.build_underscore_name_from_camelcase(field['array-size-field'])
                inner_template += (
                    '\n'
                    '    if (!_mbim_message_read_string_array (self, out->${array_size_field_name_underscore}, relative_offset, offset, ${encoding}, &out->${field_name_underscore}, error))\n'
                    '        goto out;\n'
                    '    offset += (8 * out->${array_size_field_name_underscore});\n')
            elif field['format'] == 'ipv4':
                inner_template += (
                    '\n'
                    '    if (!_mbim_message_read_ipv4 (self, offset, FALSE, NULL, &(out->${field_name_underscore}), error))\n'
                    '        goto out;\n'
                    '    offset += 4;\n')
            elif field['format'] == 'ipv6':
                inner_template += (
                    '\n'
                    '    if (!_mbim_message_read_ipv6 (self, offset, FALSE, NULL, &(out->${field_name_underscore}), error))\n'
                    '        goto out;\n'
                    '    offset += 16;\n')
            else:
                raise ValueError('Cannot handle format \'%s\' in struct' % field['format'])

            template += string.Template(inner_template).substitute(translations)

        template += (
            '\n'
            '    success = TRUE;\n'
            '\n'
            ' out:\n'
            '    if (success) {\n')
        if self.ms_struct_array_member == True:
            template += (
                '        if (bytes_read)\n'
                '            *bytes_read = (offset - relative_offset) + extra_bytes_read;\n'
                '        return out;\n'
                '    }\n'
                '\n')
        else:
            template += (
                '        if (bytes_read)\n'
                '            *bytes_read = (offset - relative_offset);\n'
                '        return out;\n'
                '    }\n'
                '\n')

        for field in self.contents:
            translations['field_name_underscore'] = utils.build_underscore_name_from_camelcase(field['name'])
            inner_template = ''
            if field['format'] in ['ref-byte-array', 'ref-byte-array-no-offset', 'unsized-byte-array', 'byte-array', 'string']:
                inner_template = ('    g_free (out->${field_name_underscore});\n')
            elif field['format'] == 'string-array':
                inner_template = ('    g_strfreev (out->${field_name_underscore});\n')
            template += string.Template(inner_template).substitute(translations)

        template += (
            '    g_free (out);\n'
            '    return NULL;\n'
            '}\n')
        cfile.write(string.Template(template).substitute(translations))

        if self.ms_struct == True:
            template = (
                '\n'
                'static gboolean\n'
                '_mbim_message_read_${name_underscore}_ms_struct (\n'
                '    const MbimMessage *self,\n'
                '    guint32 relative_offset,\n'
                '    ${name} **out_struct,\n'
                '    GError **error)\n'
                '{\n'
                '    ${name} *out;\n'
                '    guint32 offset;\n'
                '    guint32 size;\n'
                '\n'
                '    g_assert (self != NULL);\n'
                '\n'
                '    if (!_mbim_message_read_guint32 (self, relative_offset, &offset, error))\n'
                '        return FALSE;\n'
                '    relative_offset += 4;\n'
                '\n'
                '    if (!_mbim_message_read_guint32 (self, relative_offset, &size, error))\n'
                '        return FALSE;\n'
                '    relative_offset += 4;\n'
                '\n'
                '    if (!offset) {\n'
                '        *out_struct = NULL;\n'
                '        return TRUE;\n'
                '    }\n'
                '\n'
                '    out = _mbim_message_read_${name_underscore}_struct (self, offset, size, NULL, error);\n'
                '    if (!out)\n'
                '        return FALSE;\n'
                '    *out_struct = out;\n'
                '    return TRUE;\n'
                '}\n')
            cfile.write(string.Template(template).substitute(translations))

        if self.struct_array_member == True:
            # struct size is REQUIRED to be non-zero at this point, because that ensures
            # that the struct has exclusively fixed-sized variables, required to iterate
            # the elements with "offset += ${struct_size}".
            if self.size == 0:
                raise ValueError('Struct ' + self.name + ' has unexpected variable length fields')

            template = (
                '\n'
                'static gboolean\n'
                '_mbim_message_read_${name_underscore}_struct_array (\n'
                '    const MbimMessage *self,\n'
                '    guint32 array_size,\n'
                '    guint32 relative_offset_array_start,\n'
                '    ${name}Array **out_array,\n'
                '    GError **error)\n'
                '{\n'
                '    g_autoptr(GPtrArray) out = NULL;\n'
                '    guint32 i;\n'
                '    guint32 offset;\n'
                '\n'
                '    if (!array_size) {\n'
                '        *out_array = NULL;\n'
                '        return TRUE;\n'
                '    }\n'
                '\n'
                '    if (!_mbim_message_read_guint32 (self, relative_offset_array_start, &offset, error))\n'
                '        return FALSE;\n'
                '\n'
                '    out = g_ptr_array_new_with_free_func ((GDestroyNotify)_${name_underscore}_free);\n'
                '\n'
                '    for (i = 0; i < array_size; i++, offset += ${struct_size}) {\n'
                '        ${name} *array_item;\n'
                '\n'
                '        array_item = _mbim_message_read_${name_underscore}_struct (self, offset, ${struct_size}, NULL, error);\n'
                '        if (!array_item)\n'
                '            return FALSE;\n'
                '        g_ptr_array_add (out, array_item);\n'
                '    }\n'
                '\n'
                '    g_ptr_array_add (out, NULL);\n'
                '    *out_array = (${name}Array *) g_ptr_array_free (g_steal_pointer (&out), FALSE);\n'
                '    return TRUE;\n'
                '}\n')
            cfile.write(string.Template(template).substitute(translations))

        if self.ref_struct_array_member == True:
            template = (
                '\n'
                'static gboolean\n'
                '_mbim_message_read_${name_underscore}_ref_struct_array (\n'
                '    const MbimMessage *self,\n'
                '    guint32 array_size,\n'
                '    guint32 relative_offset_array_start,\n'
                '    ${name}Array **out_array,\n'
                '    GError **error)\n'
                '{\n'
                '    g_autoptr(GPtrArray) out = NULL;\n'
                '    guint32 i;\n'
                '    guint32 offset;\n'
                '\n'
                '    if (!array_size) {\n'
                '        *out_array = NULL;\n'
                '        return TRUE;\n'
                '    }\n'
                '\n'
                '    out = g_ptr_array_new_with_free_func ((GDestroyNotify)_${name_underscore}_free);\n'
                '\n'
                '    offset = relative_offset_array_start;\n'
                '    for (i = 0; i < array_size; i++, offset += 8) {\n'
                '        guint32 tmp_offset;\n'
                '        guint32 tmp_length;\n'
                '        ${name} *array_item;\n'
                '\n'
                '        if (!_mbim_message_read_guint32 (self, offset, &tmp_offset, error)) \n'
                '            return FALSE;\n'
                '        if (!_mbim_message_read_guint32 (self, offset + 4, &tmp_length, error)) \n'
                '            return FALSE;\n'
                '\n'
                '        array_item = _mbim_message_read_${name_underscore}_struct (self, tmp_offset, tmp_length, NULL, error);\n'
                '        if (!array_item)\n'
                '            return FALSE;\n'
                '        g_ptr_array_add (out, array_item);\n'
                '    }\n'
                '\n'
                '    g_ptr_array_add (out, NULL);\n'
                '    *out_array = (${name}Array *) g_ptr_array_free (g_steal_pointer (&out), FALSE);\n'
                '    return TRUE;\n'
                '}\n')
            cfile.write(string.Template(template).substitute(translations))

        if self.ms_struct_array_member == True:
            template = (
                '\n'
                'static gboolean\n'
                '_mbim_message_read_${name_underscore}_ms_struct_array (\n'
                '    const MbimMessage *self,\n'
                '    guint32 offset,\n'
                '    guint32 *out_array_size,\n'
                '    ${name}Array **out_array,\n'
                '    GError **error)\n'
                '{\n'
                '    g_autoptr(GPtrArray) out = NULL;\n'
                '    guint32 i;\n'
                '    guint32 intermediate_struct_offset;\n'
                '    guint32 intermediate_struct_size;\n'
                '    guint32 array_size;\n'
                '    guint32 bytes_read = 0;\n'
                '\n'
                '    if (!_mbim_message_read_guint32 (self, offset, &intermediate_struct_offset, error))\n'
                '        return FALSE;\n'
                '    offset += 4;\n'
                '\n'
                '    if (!_mbim_message_read_guint32 (self, offset, &intermediate_struct_size, error))\n'
                '        return FALSE;\n'
                '    offset += 4;\n'
                '\n'
                '    if (!intermediate_struct_offset) {\n'
                '        *out_array_size = 0;\n'
                '        *out_array = NULL;\n'
                '        return TRUE;\n'
                '    }\n'
                '\n'
                '    if (!_mbim_message_read_guint32 (self, intermediate_struct_offset, &array_size, error))\n'
                '        return FALSE;\n'
                '\n'
                '    if (!array_size) {\n'
                '        *out_array_size = 0;\n'
                '        *out_array = NULL;\n'
                '        return TRUE;\n'
                '    }\n'
                '\n'
                '    intermediate_struct_offset += 4;\n'
                '\n'
                '    out = g_ptr_array_new_with_free_func ((GDestroyNotify)_${name_underscore}_free);\n'
                '\n'
                '    for (i = 0; i < array_size; i++, intermediate_struct_offset += bytes_read) {\n'
                '        ${name} *array_item;\n'
                '\n'
                '        array_item = _mbim_message_read_${name_underscore}_struct (self, intermediate_struct_offset, 0, &bytes_read, error);\n'
                '        if (!array_item)\n'
                '            return FALSE;\n'
                '        g_ptr_array_add (out, array_item);\n'
                '    }\n'
                '\n'
                '    g_ptr_array_add (out, NULL);\n'
                '    *out_array_size = array_size;\n'
                '    *out_array = (${name}Array *) g_ptr_array_free (g_steal_pointer (&out), FALSE);\n'
                '    return TRUE;\n'
                '}\n')
            cfile.write(string.Template(template).substitute(translations))

    """
    Emit the type's append methods
    """
    def _emit_append(self, cfile):
        translations = { 'name'            : self.name,
                         'name_underscore' : utils.build_underscore_name_from_camelcase(self.name),
                         'struct_size'     : self.size }

        template = (
            '\n'
            'static GByteArray *\n'
            '_${name_underscore}_struct_new (const ${name} *value)\n'
            '{\n'
            '    MbimStructBuilder *builder;\n'
            '\n'
            '    g_assert (value != NULL);\n'
            '\n'
            '    builder = _mbim_struct_builder_new ();\n')

        for field in self.contents:
            translations['field'] = utils.build_underscore_name_from_camelcase(field['name'])
            translations['array_size'] = field['array-size'] if 'array-size' in field else ''
            translations['array_size_field'] = utils.build_underscore_name_from_camelcase(field['array-size-field']) if 'array-size-field' in field else ''
            translations['pad_array'] = field['pad-array'] if 'pad-array' in field else 'TRUE'

            if field['format'] == 'uuid':
                inner_template = ('    _mbim_struct_builder_append_uuid (builder, &(value->${field}));\n')
            elif field['format'] == 'byte-array':
                inner_template = ('    _mbim_struct_builder_append_byte_array (builder, FALSE, FALSE, ${pad_array}, value->${field}, ${array_size}, FALSE);\n')
            elif field['format'] == 'unsized-byte-array':
                inner_template = ('    _mbim_struct_builder_append_byte_array (builder, FALSE, FALSE, ${pad_array}, value->${field}, value->${field}_size, FALSE);\n')
            elif field['format'] in ['ref-byte-array', 'ref-byte-array-no-offset']:
                translations['has_offset'] = 'TRUE' if field['format'] == 'ref-byte-array' else 'FALSE'
                if 'array-size-field' in field:
                    inner_template = ('    _mbim_struct_builder_append_byte_array (builder, ${has_offset}, FALSE, ${pad_array}, value->${field}, value->${array_size_field}, FALSE);\n')
                else:
                    inner_template = ('    _mbim_struct_builder_append_byte_array (builder, ${has_offset}, TRUE, ${pad_array}, value->${field}, value->${field}_size, FALSE);\n')
            elif field['format'] == 'guint16':
                inner_template = ('    _mbim_struct_builder_append_guint16 (builder, value->${field});\n')
            elif field['format'] == 'guint32':
                inner_template = ('    _mbim_struct_builder_append_guint32 (builder, value->${field});\n')
            elif field['format'] == 'gint32':
                inner_template = ('    _mbim_struct_builder_append_gint32 (builder, value->${field});\n')
            elif field['format'] == 'guint32-array':
                inner_template = ('    _mbim_struct_builder_append_guint32_array (builder, value->${field}, value->${array_size_field});\n')
            elif field['format'] == 'guint64':
                inner_template = ('    _mbim_struct_builder_append_guint64 (builder, value->${field});\n')
            elif field['format'] == 'string':
                inner_template = ('    _mbim_struct_builder_append_string (builder, value->${field});\n')
            elif field['format'] == 'string-array':
                inner_template = ('    _mbim_struct_builder_append_string_array (builder, value->${field}, value->${array_size_field});\n')
            elif field['format'] == 'ipv4':
                inner_template = ('    _mbim_struct_builder_append_ipv4 (builder, &value->${field}, FALSE);\n')
            elif field['format'] == 'ipv6':
                inner_template = ('    _mbim_struct_builder_append_ipv6 (builder, &value->${field}, FALSE);\n')
            else:
                raise ValueError('Cannot handle format \'%s\' in struct' % field['format'])

            template += string.Template(inner_template).substitute(translations)

        template += (
            '\n'
            '    return _mbim_struct_builder_complete (builder);\n'
            '}\n')
        cfile.write(string.Template(template).substitute(translations))

        template = (
            '\n'
            'static void\n'
            '_mbim_struct_builder_append_${name_underscore}_struct (\n'
            '    MbimStructBuilder *builder,\n'
            '    const ${name} *value)\n'
            '{\n'
            '    GByteArray *raw;\n'
            '\n'
            '    raw = _${name_underscore}_struct_new (value);\n'
            '    g_byte_array_append (builder->fixed_buffer, raw->data, raw->len);\n'
            '    g_byte_array_unref (raw);\n'
            '}\n'
            '\n'
            'static void\n'
            '_mbim_message_command_builder_append_${name_underscore}_struct (\n'
            '    MbimMessageCommandBuilder *builder,\n'
            '    const ${name} *value)\n'
            '{\n'
            '    _mbim_struct_builder_append_${name_underscore}_struct (builder->contents_builder, value);\n'
            '}\n')
        cfile.write(string.Template(template).substitute(translations))

        if self.struct_array_member == True:
            template = (
                '\n'
                'static void\n'
                '_mbim_struct_builder_append_${name_underscore}_struct_array (\n'
                '    MbimStructBuilder *builder,\n'
                '    const ${name} *const *values,\n'
                '    guint32 n_values)\n'
                '{\n'
                '    guint32 offset;\n'
                '    guint32 i;\n'
                '    GByteArray *raw_all = NULL;\n'
                '\n'
                '    for (i = 0; i < n_values; i++) {\n'
                '        GByteArray *raw;\n'
                '\n'
                '        raw = _${name_underscore}_struct_new (values[i]);\n'
                '        if (!raw_all)\n'
                '            raw_all = raw;\n'
                '        else {\n'
                '            g_byte_array_append (raw_all, raw->data, raw->len);\n'
                '            g_byte_array_unref (raw);\n'
                '        }\n'
                '    }\n'
                '\n'
                '    if (!raw_all) {\n'
                '        offset = 0;\n'
                '        g_byte_array_append (builder->fixed_buffer, (guint8 *)&offset, sizeof (offset));\n'
                '    } else {\n'
                '        guint32 offset_offset;\n'
                '\n'
                '        /* Offset of the offset */\n'
                '        offset_offset = builder->fixed_buffer->len;\n'
                '        /* Length *not* in LE yet */\n'
                '        offset = builder->variable_buffer->len;\n'
                '        /* Add the offset value */\n'
                '        g_byte_array_append (builder->fixed_buffer, (guint8 *)&offset, sizeof (offset));\n'
                '        /* Configure the value to get updated */\n'
                '        g_array_append_val (builder->offsets, offset_offset);\n'
                '        /* Add the final array itself */\n'
                '        g_byte_array_append (builder->variable_buffer, raw_all->data, raw_all->len);\n'
                '        g_byte_array_unref (raw_all);\n'
                '    }\n'
                '}\n'
                '\n'
                'static void\n'
                '_mbim_message_command_builder_append_${name_underscore}_struct_array (\n'
                '    MbimMessageCommandBuilder *builder,\n'
                '    const ${name} *const *values,\n'
                '    guint32 n_values)\n'
                '{\n'
                '    _mbim_struct_builder_append_${name_underscore}_struct_array (builder->contents_builder, values, n_values);\n'
                '}\n')
            cfile.write(string.Template(template).substitute(translations))

        if self.ref_struct_array_member == True:
            template = (
                '\n'
                'static void\n'
                '_mbim_struct_builder_append_${name_underscore}_ref_struct_array (\n'
                '    MbimStructBuilder *builder,\n'
                '    const ${name} *const *values,\n'
                '    guint32 n_values)\n'
                '{\n'
                '    guint32 offset;\n'
                '    guint32 i;\n'
                '\n'
                '    for (i = 0; i < n_values; i++) {\n'
                '        guint32 length;\n'
                '        guint32 offset_offset;\n'
                '        GByteArray *raw;\n'
                '\n'
                '        raw = _${name_underscore}_struct_new (values[i]);\n'
                '        g_assert (raw->len > 0);\n'
                '\n'
                '        /* Offset of the offset */\n'
                '        offset_offset = builder->fixed_buffer->len;\n'
                '\n'
                '        /* Length *not* in LE yet */\n'
                '        offset = builder->variable_buffer->len;\n'
                '        /* Add the offset value */\n'
                '        g_byte_array_append (builder->fixed_buffer, (guint8 *)&offset, sizeof (offset));\n'
                '        /* Configure the value to get updated */\n'
                '        g_array_append_val (builder->offsets, offset_offset);\n'
                '\n'
                '        /* Add the length value */\n'
                '        length = GUINT32_TO_LE (raw->len);\n'
                '        g_byte_array_append (builder->fixed_buffer, (guint8 *)&length, sizeof (length));\n'
                '\n'
                '        /* And finally, the bytearray itself to the variable buffer */\n'
                '        g_byte_array_append (builder->variable_buffer, (const guint8 *)raw->data, (guint)raw->len);\n'
                '        g_byte_array_unref (raw);\n'
                '    }\n'
                '}\n'
                '\n'
                'static void\n'
                '_mbim_message_command_builder_append_${name_underscore}_ref_struct_array (\n'
                '    MbimMessageCommandBuilder *builder,\n'
                '    const ${name} *const *values,\n'
                '    guint32 n_values)\n'
                '{\n'
                '    _mbim_struct_builder_append_${name_underscore}_ref_struct_array (builder->contents_builder, values, n_values);\n'
                '}\n')
            cfile.write(string.Template(template).substitute(translations))

        # append operations not implemented for self.ms_struct_array_member == True


    """
    Emit the struct handling implementation
    """
    def emit(self, hfile, cfile):
        utils.add_separator(hfile, 'Struct', self.name);
        utils.add_separator(cfile, 'Struct', self.name);

        # Emit type
        self._emit_type(hfile)
        # Emit type's free
        self._emit_free(hfile, cfile)
        # Emit type's read
        self._emit_read(cfile)
        # Emit type's print
        self._emit_print(cfile)
        # Emit type's append
        self._emit_append(cfile)


    """
    Emit the section contents
    """
    def emit_section_content(self, sfile):
        translations = { 'struct_name'     : self.name,
                         'name_underscore' : utils.build_underscore_name_from_camelcase(self.name) }
        template = (
            '<SUBSECTION ${struct_name}>\n'
            '${struct_name}\n')
        if self.struct_array_member == True or self.ref_struct_array_member == True or self.ms_struct_array_member == True:
            template += (
                '${struct_name}Array\n')
        if self.single_member == True:
            template += (
                '${name_underscore}_free\n')
        if self.struct_array_member == True or self.ref_struct_array_member == True or self.ms_struct_array_member == True:
            template += (
                '${name_underscore}_array_free\n')
        sfile.write(string.Template(template).substitute(translations))
