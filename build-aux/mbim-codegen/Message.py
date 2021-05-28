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
# Copyright (C) 2013 - 2018 Aleksander Morgado <aleksander@aleksander.es>
#

import string
import utils


"""
Flag the values which always need to be read
"""
def flag_always_read_field(fields, field_name):
    for field in fields:
        if field['name'] == field_name:
            if field['format'] != 'guint32':
                    raise ValueError('Fields to always read \'%s\' must be a guint32' % field_name)
            field['always-read'] = True
            return
    raise ValueError('Couldn\'t find field to always read \'%s\'' % field_name)


"""
Validate fields in the dictionary
"""
def validate_fields(fields):
    for field in fields:
        # Look for condition fields, which need to be always read
        if 'available-if' in field:
            condition = field['available-if']
            flag_always_read_field(fields, condition['field'])

        # Look for array size fields, which need to be always read
        if field['format'] == 'byte-array':
            pass
        elif field['format'] == 'ref-byte-array':
            pass
        elif field['format'] == 'uicc-ref-byte-array':
            pass
        elif field['format'] == 'ref-byte-array-no-offset':
            pass
        elif field['format'] == 'unsized-byte-array':
            pass
        elif field['format'] == 'uuid':
            pass
        elif field['format'] == 'guint32':
            pass
        elif field['format'] == 'guint32-array':
            flag_always_read_field(fields, field['array-size-field'])
        elif field['format'] == 'guint64':
            pass
        elif field['format'] == 'string':
            pass
        elif field['format'] == 'string-array':
            flag_always_read_field(fields, field['array-size-field'])
        elif field['format'] == 'struct':
            if 'struct-type' not in field:
                raise ValueError('Field type \'struct\' requires \'struct-type\' field')
        elif field['format'] == 'struct-array':
            flag_always_read_field(fields, field['array-size-field'])
            if 'struct-type' not in field:
                raise ValueError('Field type \'struct\' requires \'struct-type\' field')
        elif field['format'] == 'ref-struct-array':
            flag_always_read_field(fields, field['array-size-field'])
            if 'struct-type' not in field:
                raise ValueError('Field type \'struct\' requires \'struct-type\' field')
        elif field['format'] == 'ipv4':
            pass
        elif field['format'] == 'ref-ipv4':
            pass
        elif field['format'] == 'ipv4-array':
            flag_always_read_field(fields, field['array-size-field'])
        elif field['format'] == 'ipv6':
            pass
        elif field['format'] == 'ref-ipv6':
            pass
        elif field['format'] == 'ipv6-array':
            flag_always_read_field(fields, field['array-size-field'])
        else:
            raise ValueError('Cannot handle field type \'%s\'' % field['format'])


"""
The Message class takes care of all message handling
"""
class Message:

    """
    Constructor
    """
    def __init__(self, dictionary):
        # The message service, e.g. "Basic Connect"
        self.service = dictionary['service']

        # The name of the specific message, e.g. "Something"
        self.name = dictionary['name']

        # Query
        if 'query' in dictionary:
            self.has_query = True
            self.query = dictionary['query']
            self.query_since = dictionary['since-ex']['query'] if 'since-ex' in dictionary else dictionary['since'] if 'since' in dictionary else None
            if self.query_since is None:
                raise ValueError('Message ' + self.name + ' (query) requires a "since" tag specifying the major version where it was introduced')
            validate_fields(self.query)
        else:
            self.has_query = False
            self.query = []

        # Set
        if 'set' in dictionary:
            self.has_set = True
            self.set = dictionary['set']
            self.set_since = dictionary['since-ex']['set'] if 'since-ex' in dictionary else dictionary['since'] if 'since' in dictionary else None
            if self.set_since is None:
                raise ValueError('Message ' + self.name + ' (set) requires a "since" tag specifying the major version where it was introduced')
            validate_fields(self.set)
        else:
            self.has_set = False
            self.set = []


        # Response
        if 'response' in dictionary:
            self.has_response = True
            self.response = dictionary['response']
            self.response_since = dictionary['since-ex']['response'] if 'since-ex' in dictionary else dictionary['since'] if 'since' in dictionary else None
            if self.response_since is None:
                raise ValueError('Message ' + self.name + ' (response) requires a "since" tag specifying the major version where it was introduced')
            validate_fields(self.response)
        else:
            self.has_response = False
            self.response = []

        # Notification
        if 'notification' in dictionary:
            self.has_notification = True
            self.notification = dictionary['notification']
            self.notification_since = dictionary['since-ex']['notification'] if 'since-ex' in dictionary else dictionary['since'] if 'since' in dictionary else None
            if self.notification_since is None:
                raise ValueError('Message ' + self.name + ' (notification) requires a "since" tag specifying the major version where it was introduced')
            validate_fields(self.notification)
        else:
            self.has_notification = False
            self.notification = []

        # Build Fullname
        if self.service == 'Basic Connect':
            self.fullname = 'MBIM Message ' + self.name
        elif self.name == "":
            self.fullname = 'MBIM Message ' + self.service
        else:
            self.fullname = 'MBIM Message ' + self.service + ' ' + self.name

        # Build CID enum
        self.cid_enum_name = 'MBIM CID ' + self.service
        if self.name != "":
            self.cid_enum_name += (' ' + self.name)
        self.cid_enum_name = utils.build_underscore_name(self.cid_enum_name).upper()


    """
    Emit the message handling implementation
    """
    def emit(self, hfile, cfile):
        if self.has_query:
            utils.add_separator(hfile, 'Message (Query)', self.fullname);
            utils.add_separator(cfile, 'Message (Query)', self.fullname);
            self._emit_message_creator(hfile, cfile, 'query', self.query, self.query_since)
            self._emit_message_printable(cfile, 'query', self.query)

        if self.has_set:
            utils.add_separator(hfile, 'Message (Set)', self.fullname);
            utils.add_separator(cfile, 'Message (Set)', self.fullname);
            self._emit_message_creator(hfile, cfile, 'set', self.set, self.set_since)
            self._emit_message_printable(cfile, 'set', self.set)

        if self.has_response:
            utils.add_separator(hfile, 'Message (Response)', self.fullname);
            utils.add_separator(cfile, 'Message (Response)', self.fullname);
            self._emit_message_parser(hfile, cfile, 'response', self.response, self.response_since)
            self._emit_message_printable(cfile, 'response', self.response)

        if self.has_notification:
            utils.add_separator(hfile, 'Message (Notification)', self.fullname);
            utils.add_separator(cfile, 'Message (Notification)', self.fullname);
            self._emit_message_parser(hfile, cfile, 'notification', self.notification, self.notification_since)
            self._emit_message_printable(cfile, 'notification', self.notification)


    """
    Emit message creator
    """
    def _emit_message_creator(self, hfile, cfile, message_type, fields, since):
        translations = { 'message'                  : self.name,
                         'service'                  : self.service,
                         'since'                    : since,
                         'underscore'               : utils.build_underscore_name (self.fullname),
                         'message_type'             : message_type,
                         'message_type_upper'       : message_type.upper(),
                         'service_underscore_upper' : utils.build_underscore_name (self.service).upper(),
                         'cid_enum_name'            : self.cid_enum_name }

        template = (
            '\n'
            '/**\n'
            ' * ${underscore}_${message_type}_new:\n')

        for field in fields:
            translations['name'] = field['name']
            translations['field'] = utils.build_underscore_name_from_camelcase (field['name'])
            translations['struct'] = field['struct-type'] if 'struct-type' in field else ''
            translations['public'] = field['public-format'] if 'public-format' in field else field['format']
            translations['array_size'] = field['array-size'] if 'array-size' in field else ''

            inner_template = ''
            if field['format'] == 'byte-array':
                inner_template = (' * @${field}: (in)(element-type guint8)(array fixed-size=${array_size}): the \'${name}\' field, given as an array of ${array_size} #guint8 values.\n')
            elif field['format'] == 'unsized-byte-array' or \
                 field['format'] == 'ref-byte-array' or \
                 field['format'] == 'uicc-ref-byte-array' or \
                 field['format'] == 'ref-byte-array-no-offset':
                inner_template = (' * @${field}_size: (in): size of the ${field} array.\n'
                                  ' * @${field}: (in)(element-type guint8)(array length=${field}_size): the \'${name}\' field, given as an array of #guint8 values.\n')
            elif field['format'] == 'uuid':
                inner_template = (' * @${field}: (in): the \'${name}\' field, given as a #MbimUuid.\n')
            elif field['format'] == 'guint32':
                inner_template = (' * @${field}: (in): the \'${name}\' field, given as a #${public}.\n')
            elif field['format'] == 'guint64':
                inner_template = (' * @${field}: (in): the \'${name}\' field, given as a #${public}.\n')
            elif field['format'] == 'string':
                inner_template = (' * @${field}: (in): the \'${name}\' field, given as a string.\n')
            elif field['format'] == 'string-array':
                inner_template = (' * @${field}: (in)(type GStrv): the \'${name}\' field, given as an array of strings.\n')
            elif field['format'] == 'struct':
                inner_template = (' * @${field}: (in): the \'${name}\' field, given as a #${struct}.\n')
            elif field['format'] == 'struct-array':
                inner_template = (' * @${field}: (in)(array zero-terminated=1)(element-type ${struct}): the \'${name}\' field, given as an array of #${struct} items.\n')
            elif field['format'] == 'ref-struct-array':
                inner_template = (' * @${field}: (in): the \'${name}\' field, given as an array of #${struct} items.\n')
            elif field['format'] == 'ipv4':
                inner_template = (' * @${field}: (in): the \'${name}\' field, given as a #MbimIPv4.\n')
            elif field['format'] == 'ref-ipv4':
                inner_template = (' * @${field}: (in): the \'${name}\' field, given as a #MbimIPv4.\n')
            elif field['format'] == 'ipv4-array':
                inner_template = (' * @${field}: (in)(array zero-terminated=1)(element-type MbimIPv4): the \'${name}\' field, given as an array of #MbimIPv4 items.\n')
            elif field['format'] == 'ipv6':
                inner_template = (' * @${field}: (in): the \'${name}\' field, given as a #MbimIPv6.\n')
            elif field['format'] == 'ref-ipv6':
                inner_template = (' * @${field}: (in): the \'${name}\' field, given as a #MbimIPv6.\n')
            elif field['format'] == 'ipv6-array':
                inner_template = (' * @${field}: (in)(array zero-terminated=1)(element-type MbimIPv6): the \'${name}\' field, given as an array of #MbimIPv6 items.\n')

            template += (string.Template(inner_template).substitute(translations))

        template += (
            ' * @error: return location for error or %NULL.\n'
            ' *\n'
            ' * Create a new request for the \'${message}\' ${message_type} command in the \'${service}\' service.\n'
            ' *\n'
            ' * Returns: a newly allocated #MbimMessage, which should be freed with mbim_message_unref().\n'
            ' *\n'
            ' * Since: ${since}\n'
            ' */\n'
            'MbimMessage *${underscore}_${message_type}_new (\n')

        for field in fields:
            translations['field'] = utils.build_underscore_name_from_camelcase (field['name'])
            translations['struct'] = field['struct-type'] if 'struct-type' in field else ''
            translations['public'] = field['public-format'] if 'public-format' in field else field['format']

            inner_template = ''
            if field['format'] == 'byte-array':
                inner_template = ('    const guint8 *${field},\n')
            elif field['format'] == 'unsized-byte-array' or \
                 field['format'] == 'ref-byte-array' or \
                 field['format'] == 'uicc-ref-byte-array' or \
                 field['format'] == 'ref-byte-array-no-offset':
                inner_template = ('    const guint32 ${field}_size,\n'
                                  '    const guint8 *${field},\n')
            elif field['format'] == 'uuid':
                inner_template = ('    const MbimUuid *${field},\n')
            elif field['format'] == 'guint32':
                inner_template = ('    ${public} ${field},\n')
            elif field['format'] == 'guint64':
                inner_template = ('    ${public} ${field},\n')
            elif field['format'] == 'string':
                inner_template = ('    const gchar *${field},\n')
            elif field['format'] == 'string-array':
                inner_template = ('    const gchar *const *${field},\n')
            elif field['format'] == 'struct':
                inner_template = ('    const ${struct} *${field},\n')
            elif field['format'] == 'struct-array':
                inner_template = ('    const ${struct} *const *${field},\n')
            elif field['format'] == 'ref-struct-array':
                inner_template = ('    const ${struct} *const *${field},\n')
            elif field['format'] == 'ipv4':
                inner_template = ('    const MbimIPv4 *${field},\n')
            elif field['format'] == 'ref-ipv4':
                inner_template = ('    const MbimIPv4 *${field},\n')
            elif field['format'] == 'ipv4-array':
                inner_template = ('    const MbimIPv4 *${field},\n')
            elif field['format'] == 'ipv6':
                inner_template = ('    const MbimIPv6 *${field},\n')
            elif field['format'] == 'ref-ipv6':
                inner_template = ('    const MbimIPv6 *${field},\n')
            elif field['format'] == 'ipv6-array':
                inner_template = ('    const MbimIPv6 *${field},\n')

            template += (string.Template(inner_template).substitute(translations))

        template += (
            '    GError **error);\n')
        hfile.write(string.Template(template).substitute(translations))

        template = (
            '\n'
            'MbimMessage *\n'
            '${underscore}_${message_type}_new (\n')

        for field in fields:
            translations['field'] = utils.build_underscore_name_from_camelcase (field['name'])
            translations['struct'] = field['struct-type'] if 'struct-type' in field else ''
            translations['public'] = field['public-format'] if 'public-format' in field else field['format']
            translations['array_size'] = field['array-size'] if 'array-size' in field else ''

            inner_template = ''
            if field['format'] == 'byte-array':
                inner_template = ('    const guint8 *${field},\n')
            elif field['format'] == 'unsized-byte-array' or \
                 field['format'] == 'ref-byte-array' or \
                 field['format'] == 'uicc-ref-byte-array' or \
                 field['format'] == 'ref-byte-array-no-offset':
                inner_template = ('    const guint32 ${field}_size,\n'
                                  '    const guint8 *${field},\n')
            elif field['format'] == 'uuid':
                inner_template = ('    const MbimUuid *${field},\n')
            elif field['format'] == 'guint32':
                inner_template = ('    ${public} ${field},\n')
            elif field['format'] == 'guint64':
                inner_template = ('    ${public} ${field},\n')
            elif field['format'] == 'string':
                inner_template = ('    const gchar *${field},\n')
            elif field['format'] == 'string-array':
                inner_template = ('    const gchar *const *${field},\n')
            elif field['format'] == 'struct':
                inner_template = ('    const ${struct} *${field},\n')
            elif field['format'] == 'struct-array':
                inner_template = ('    const ${struct} *const *${field},\n')
            elif field['format'] == 'ref-struct-array':
                inner_template = ('    const ${struct} *const *${field},\n')
            elif field['format'] == 'ipv4':
                inner_template = ('    const MbimIPv4 *${field},\n')
            elif field['format'] == 'ref-ipv4':
                inner_template = ('    const MbimIPv4 *${field},\n')
            elif field['format'] == 'ipv4-array':
                inner_template = ('    const MbimIPv4 *${field},\n')
            elif field['format'] == 'ipv6':
                inner_template = ('    const MbimIPv6 *${field},\n')
            elif field['format'] == 'ref-ipv6':
                inner_template = ('    const MbimIPv6 *${field},\n')
            elif field['format'] == 'ipv6-array':
                inner_template = ('    const MbimIPv6 *${field},\n')

            template += (string.Template(inner_template).substitute(translations))

        template += (
            '    GError **error)\n'
            '{\n'
            '    MbimMessageCommandBuilder *builder;\n'
            '\n'
            '    builder = _mbim_message_command_builder_new (0,\n'
            '                                                 MBIM_SERVICE_${service_underscore_upper},\n'
            '                                                 ${cid_enum_name},\n'
            '                                                 MBIM_MESSAGE_COMMAND_TYPE_${message_type_upper});\n')

        for field in fields:
            translations['field'] = utils.build_underscore_name_from_camelcase(field['name'])
            translations['array_size_field'] = utils.build_underscore_name_from_camelcase(field['array-size-field']) if 'array-size-field' in field else ''
            translations['struct'] = field['struct-type'] if 'struct-type' in field else ''
            translations['struct_underscore'] = utils.build_underscore_name_from_camelcase (translations['struct'])
            translations['array_size'] = field['array-size'] if 'array-size' in field else ''
            translations['pad_array'] = field['pad-array'] if 'pad-array' in field else 'TRUE'

            inner_template = ''
            if 'available-if' in field:
                condition = field['available-if']
                translations['condition_field'] = utils.build_underscore_name_from_camelcase(condition['field'])
                translations['condition_operation'] = condition['operation']
                translations['condition_value'] = condition['value']
                inner_template += (
                    '    if (${condition_field} ${condition_operation} ${condition_value}) {\n')
            else:
                inner_template += ('    {\n')

            if field['format'] == 'byte-array':
                inner_template += ('        _mbim_message_command_builder_append_byte_array (builder, FALSE, FALSE, ${pad_array}, ${field}, ${array_size}, FALSE);\n')
            elif field['format'] == 'unsized-byte-array':
                inner_template += ('        _mbim_message_command_builder_append_byte_array (builder, FALSE, FALSE, ${pad_array}, ${field}, ${field}_size, FALSE);\n')
            elif field['format'] == 'ref-byte-array':
                inner_template += ('        _mbim_message_command_builder_append_byte_array (builder, TRUE, TRUE, ${pad_array}, ${field}, ${field}_size, FALSE);\n')
            elif field['format'] == 'uicc-ref-byte-array':
                inner_template += ('        _mbim_message_command_builder_append_byte_array (builder, TRUE, TRUE, ${pad_array}, ${field}, ${field}_size, TRUE);\n')
            elif field['format'] == 'ref-byte-array-no-offset':
                inner_template += ('        _mbim_message_command_builder_append_byte_array (builder, FALSE, TRUE, ${pad_array}, ${field}, ${field}_size, FALSE);\n')
            elif field['format'] == 'uuid':
                inner_template += ('        _mbim_message_command_builder_append_uuid (builder, ${field});\n')
            elif field['format'] == 'guint32':
                inner_template += ('        _mbim_message_command_builder_append_guint32 (builder, ${field});\n')
            elif field['format'] == 'guint64':
                inner_template += ('        _mbim_message_command_builder_append_guint64 (builder, ${field});\n')
            elif field['format'] == 'string':
                inner_template += ('        _mbim_message_command_builder_append_string (builder, ${field});\n')
            elif field['format'] == 'string-array':
                inner_template += ('        _mbim_message_command_builder_append_string_array (builder, ${field}, ${array_size_field});\n')
            elif field['format'] == 'struct':
                inner_template += ('        _mbim_message_command_builder_append_${struct_underscore}_struct (builder, ${field});\n')
            elif field['format'] == 'struct-array':
                inner_template += ('        _mbim_message_command_builder_append_${struct_underscore}_struct_array (builder, ${field}, ${array_size_field}, FALSE);\n')
            elif field['format'] == 'ref-struct-array':
                inner_template += ('        _mbim_message_command_builder_append_${struct_underscore}_struct_array (builder, ${field}, ${array_size_field}, TRUE);\n')
            elif field['format'] == 'ipv4':
                inner_template += ('        _mbim_message_command_builder_append_ipv4 (builder, ${field}, FALSE);\n')
            elif field['format'] == 'ref-ipv4':
                inner_template += ('        _mbim_message_command_builder_append_ipv4 (builder, ${field}, TRUE);\n')
            elif field['format'] == 'ipv4-array':
                inner_template += ('        _mbim_message_command_builder_append_ipv4_array (builder, ${field}, ${array_size_field});\n')
            elif field['format'] == 'ipv6':
                inner_template += ('        _mbim_message_command_builder_append_ipv6 (builder, ${field}, FALSE);\n')
            elif field['format'] == 'ref-ipv6':
                inner_template += ('        _mbim_message_command_builder_append_ipv6 (builder, ${field}, TRUE);\n')
            elif field['format'] == 'ipv6-array':
                inner_template += ('        _mbim_message_command_builder_append_ipv6_array (builder, ${field}, ${array_size_field});\n')
            else:
                raise ValueError('Cannot handle field type \'%s\'' % field['format'])

            inner_template += ('    }\n')

            template += (string.Template(inner_template).substitute(translations))

        template += (
            '\n'
            '    return _mbim_message_command_builder_complete (builder);\n'
            '}\n')
        cfile.write(string.Template(template).substitute(translations))


    """
    Emit message parser
    """
    def _emit_message_parser(self, hfile, cfile, message_type, fields, since):
        translations = { 'message'                  : self.name,
                         'service'                  : self.service,
                         'since'                    : since,
                         'underscore'               : utils.build_underscore_name (self.fullname),
                         'message_type'             : message_type,
                         'message_type_upper'       : message_type.upper(),
                         'service_underscore_upper' : utils.build_underscore_name (self.service).upper() }

        template = (
            '\n'
            '/**\n'
            ' * ${underscore}_${message_type}_parse:\n'
            ' * @message: the #MbimMessage.\n')

        for field in fields:
            translations['field'] = utils.build_underscore_name_from_camelcase(field['name'])
            translations['name'] = field['name']
            translations['public'] = field['public-format'] if 'public-format' in field else field['format']
            translations['struct'] = field['struct-type'] if 'struct-type' in field else ''
            translations['struct_underscore'] = utils.build_underscore_name_from_camelcase (translations['struct'])
            translations['array_size'] = field['array-size'] if 'array-size' in field else ''

            inner_template = ''
            if field['format'] == 'byte-array':
                inner_template = (' * @out_${field}: (out)(optional)(transfer none)(element-type guint8)(array fixed-size=${array_size}): return location for an array of ${array_size} #guint8 values. Do not free the returned value, it is owned by @message.\n')
            elif field['format'] == 'unsized-byte-array' or field['format'] == 'ref-byte-array' or field['format'] == 'uicc-ref-byte-array':
                inner_template = (' * @out_${field}_size: (out)(optional): return location for the size of the ${field} array.\n'
                                  ' * @out_${field}: (out)(optional)(transfer none)(element-type guint8)(array length=out_${field}_size): return location for an array of #guint8 values. Do not free the returned value, it is owned by @message.\n')
            elif field['format'] == 'uuid':
                inner_template = (' * @out_${field}: (out)(optional)(transfer none): return location for a #MbimUuid, or %NULL if the \'${name}\' field is not needed. Do not free the returned value, it is owned by @message.\n')
            elif field['format'] == 'guint32':
                inner_template = (' * @out_${field}: (out)(optional)(transfer none): return location for a #${public}, or %NULL if the \'${name}\' field is not needed.\n')
            elif field['format'] == 'guint64':
                inner_template = (' * @out_${field}: (out)(optional)(transfer none): return location for a #guint64, or %NULL if the \'${name}\' field is not needed.\n')
            elif field['format'] == 'string':
                inner_template = (' * @out_${field}: (out)(optional)(transfer full): return location for a newly allocated string, or %NULL if the \'${name}\' field is not needed. Free the returned value with g_free().\n')
            elif field['format'] == 'string-array':
                inner_template = (' * @out_${field}: (out)(optional)(transfer full)(type GStrv): return location for a newly allocated array of strings, or %NULL if the \'${name}\' field is not needed. Free the returned value with g_strfreev().\n')
            elif field['format'] == 'struct':
                inner_template = (' * @out_${field}: (out)(optional)(transfer full): return location for a newly allocated #${struct}, or %NULL if the \'${name}\' field is not needed. Free the returned value with ${struct_underscore}_free().\n')
            elif field['format'] == 'struct-array':
                inner_template = (' * @out_${field}: (out)(optional)(transfer full)(array zero-terminated=1)(element-type ${struct}): return location for a newly allocated array of #${struct} items, or %NULL if the \'${name}\' field is not needed. Free the returned value with ${struct_underscore}_array_free().\n')
            elif field['format'] == 'ref-struct-array':
                inner_template = (' * @out_${field}: (out)(optional)(transfer full)(array zero-terminated=1)(element-type ${struct}): return location for a newly allocated array of #${struct} items, or %NULL if the \'${name}\' field is not needed. Free the returned value with ${struct_underscore}_array_free().\n')
            elif field['format'] == 'ipv4':
                inner_template = (' * @out_${field}: (out)(optional)(transfer none): return location for a #MbimIPv4, or %NULL if the \'${name}\' field is not needed. Do not free the returned value, it is owned by @message.\n')
            elif field['format'] == 'ref-ipv4':
                inner_template = (' * @out_${field}: (out)(optional)(transfer none): return location for a #MbimIPv4, or %NULL if the \'${name}\' field is not needed. Do not free the returned value, it is owned by @message.\n')
            elif field['format'] == 'ipv4-array':
                inner_template = (' * @out_${field}: (out)(optional)(transfer full)(array zero-terminated=1)(element-type MbimIPv4): return location for a newly allocated array of #MbimIPv4 items, or %NULL if the \'${name}\' field is not needed. Free the returned value with g_free().\n')
            elif field['format'] == 'ipv6':
                inner_template = (' * @out_${field}: (out)(optional)(transfer none): return location for a #MbimIPv6, or %NULL if the \'${name}\' field is not needed. Do not free the returned value, it is owned by @message.\n')
            elif field['format'] == 'ref-ipv6':
                inner_template = (' * @out_${field}: (out)(optional)(transfer none): return location for a #MbimIPv6, or %NULL if the \'${name}\' field is not needed. Do not free the returned value, it is owned by @message.\n')
            elif field['format'] == 'ipv6-array':
                inner_template = (' * @out_${field}: (out)(optional)(transfer full)(array zero-terminated=1)(element-type MbimIPv6): return location for a newly allocated array of #MbimIPv6 items, or %NULL if the \'${name}\' field is not needed. Free the returned value with g_free().\n')

            template += (string.Template(inner_template).substitute(translations))

        template += (
            ' * @error: return location for error or %NULL.\n'
            ' *\n'
            ' * Parses and returns parameters of the \'${message}\' ${message_type} command in the \'${service}\' service.\n'
            ' *\n'
            ' * Returns: %TRUE if the message was correctly parsed, %FALSE if @error is set.\n'
            ' *\n'
            ' * Since: ${since}\n'
            ' */\n'
            'gboolean ${underscore}_${message_type}_parse (\n'
            '    const MbimMessage *message,\n')

        for field in fields:
            translations['field'] = utils.build_underscore_name_from_camelcase(field['name'])
            translations['public'] = field['public-format'] if 'public-format' in field else field['format']
            translations['struct'] = field['struct-type'] if 'struct-type' in field else ''

            inner_template = ''
            if field['format'] == 'byte-array':
                inner_template = ('    const guint8 **out_${field},\n')
            elif field['format'] == 'unsized-byte-array' or field['format'] == 'ref-byte-array' or field['format'] == 'uicc-ref-byte-array':
                inner_template = ('    guint32 *out_${field}_size,\n'
                                  '    const guint8 **out_${field},\n')
            elif field['format'] == 'uuid':
                inner_template = ('    const MbimUuid **out_${field},\n')
            elif field['format'] == 'guint32':
                inner_template = ('    ${public} *out_${field},\n')
            elif field['format'] == 'guint64':
                inner_template = ('    ${public} *out_${field},\n')
            elif field['format'] == 'string':
                inner_template = ('    gchar **out_${field},\n')
            elif field['format'] == 'string-array':
                inner_template = ('    gchar ***out_${field},\n')
            elif field['format'] == 'struct':
                inner_template = ('    ${struct} **out_${field},\n')
            elif field['format'] == 'struct-array':
                inner_template = ('    ${struct}Array **out_${field},\n')
            elif field['format'] == 'ref-struct-array':
                inner_template = ('    ${struct}Array **out_${field},\n')
            elif field['format'] == 'ipv4':
                inner_template = ('    const MbimIPv4 **out_${field},\n')
            elif field['format'] == 'ref-ipv4':
                inner_template = ('    const MbimIPv4 **out_${field},\n')
            elif field['format'] == 'ipv4-array':
                inner_template = ('    MbimIPv4 **out_${field},\n')
            elif field['format'] == 'ipv6':
                inner_template = ('    const MbimIPv6 **out_${field},\n')
            elif field['format'] == 'ref-ipv6':
                inner_template = ('    const MbimIPv6 **out_${field},\n')
            elif field['format'] == 'ipv6-array':
                inner_template = ('    MbimIPv6 **out_${field},\n')
            else:
                raise ValueError('Cannot handle field type \'%s\'' % field['format'])

            template += (string.Template(inner_template).substitute(translations))

        template += (
            '    GError **error);\n')
        hfile.write(string.Template(template).substitute(translations))

        template = (
            '\n'
            'gboolean\n'
            '${underscore}_${message_type}_parse (\n'
            '    const MbimMessage *message,\n')

        for field in fields:
            translations['field'] = utils.build_underscore_name_from_camelcase(field['name'])
            translations['public'] = field['public-format'] if 'public-format' in field else field['format']
            translations['struct'] = field['struct-type'] if 'struct-type' in field else ''

            inner_template = ''
            if field['format'] == 'byte-array':
                inner_template = ('    const guint8 **out_${field},\n')
            elif field['format'] == 'unsized-byte-array' or field['format'] == 'ref-byte-array' or field['format'] == 'uicc-ref-byte-array':
                inner_template = ('    guint32 *out_${field}_size,\n'
                                  '    const guint8 **out_${field},\n')
            elif field['format'] == 'uuid':
                inner_template = ('    const MbimUuid **out_${field},\n')
            elif field['format'] == 'guint32':
                inner_template = ('    ${public} *out_${field},\n')
            elif field['format'] == 'guint64':
                inner_template = ('    ${public} *out_${field},\n')
            elif field['format'] == 'string':
                inner_template = ('    gchar **out_${field},\n')
            elif field['format'] == 'string-array':
                inner_template = ('    gchar ***out_${field},\n')
            elif field['format'] == 'struct':
                inner_template = ('    ${struct} **out_${field},\n')
            elif field['format'] == 'struct-array':
                inner_template = ('    ${struct}Array **out_${field},\n')
            elif field['format'] == 'ref-struct-array':
                inner_template = ('    ${struct}Array **out_${field},\n')
            elif field['format'] == 'ipv4':
                inner_template = ('    const MbimIPv4 **out_${field},\n')
            elif field['format'] == 'ref-ipv4':
                inner_template = ('    const MbimIPv4 **out_${field},\n')
            elif field['format'] == 'ipv4-array':
                inner_template = ('    MbimIPv4 **out_${field},\n')
            elif field['format'] == 'ipv6':
                inner_template = ('    const MbimIPv6 **out_${field},\n')
            elif field['format'] == 'ref-ipv6':
                inner_template = ('    const MbimIPv6 **out_${field},\n')
            elif field['format'] == 'ipv6-array':
                inner_template = ('    MbimIPv6 **out_${field},\n')

            template += (string.Template(inner_template).substitute(translations))

        template += (
            '    GError **error)\n'
            '{\n')

        if fields != []:
            template += (
                '    gboolean success = FALSE;\n'
                '    guint32 offset = 0;\n')

        count_allocated_variables = 0
        for field in fields:
            translations['field'] = utils.build_underscore_name_from_camelcase(field['name'])
            translations['struct'] = field['struct-type'] if 'struct-type' in field else ''
            # variables to always read
            inner_template = ''
            if 'always-read' in field:
                inner_template = ('    guint32 _${field};\n')
            # now variables that require memory allocation
            elif field['format'] == 'string':
                count_allocated_variables += 1
                inner_template = ('    gchar *_${field} = NULL;\n')
            elif field['format'] == 'string-array':
                count_allocated_variables += 1
                inner_template = ('    gchar **_${field} = NULL;\n')
            elif field['format'] == 'struct':
                count_allocated_variables += 1
                inner_template = ('    ${struct} *_${field} = NULL;\n')
            elif field['format'] == 'struct-array':
                count_allocated_variables += 1
                inner_template = ('    ${struct} **_${field} = NULL;\n')
            elif field['format'] == 'ref-struct-array':
                count_allocated_variables += 1
                inner_template = ('    ${struct} **_${field} = NULL;\n')
            elif field['format'] == 'ipv4-array':
                count_allocated_variables += 1
                inner_template = ('    MbimIPv4 *_${field} = NULL;\n')
            elif field['format'] == 'ipv6-array':
                count_allocated_variables += 1
                inner_template = ('    MbimIPv6 *_${field} = NULL;\n')
            template += (string.Template(inner_template).substitute(translations))

        if message_type == 'response':
            template += (
                '\n'
                '    if (mbim_message_get_message_type (message) != MBIM_MESSAGE_TYPE_COMMAND_DONE) {\n'
                '        g_set_error (error,\n'
                '                     MBIM_CORE_ERROR,\n'
                '                     MBIM_CORE_ERROR_INVALID_MESSAGE,\n'
                '                     \"Message is not a response\");\n'
                '        return FALSE;\n'
                '    }\n')

            if fields != []:
                template += (
                    '\n'
                    '    if (!mbim_message_command_done_get_raw_information_buffer (message, NULL)) {\n'
                    '        g_set_error (error,\n'
                    '                     MBIM_CORE_ERROR,\n'
                    '                     MBIM_CORE_ERROR_INVALID_MESSAGE,\n'
                    '                     \"Message does not have information buffer\");\n'
                    '        return FALSE;\n'
                    '    }\n')
        elif message_type == 'notification':
            template += (
                '\n'
                '    if (mbim_message_get_message_type (message) != MBIM_MESSAGE_TYPE_INDICATE_STATUS) {\n'
                '        g_set_error (error,\n'
                '                     MBIM_CORE_ERROR,\n'
                '                     MBIM_CORE_ERROR_INVALID_MESSAGE,\n'
                '                     \"Message is not a notification\");\n'
                '        return FALSE;\n'
                '    }\n')

            if fields != []:
                template += (
                    '\n'
                    '    if (!mbim_message_indicate_status_get_raw_information_buffer (message, NULL)) {\n'
                    '        g_set_error (error,\n'
                    '                     MBIM_CORE_ERROR,\n'
                    '                     MBIM_CORE_ERROR_INVALID_MESSAGE,\n'
                    '                     \"Message does not have information buffer\");\n'
                    '        return FALSE;\n'
                    '    }\n')
        else:
            raise ValueError('Unexpected message type \'%s\'' % message_type)

        for field in fields:
            translations['field'] = utils.build_underscore_name_from_camelcase(field['name'])
            translations['field_format_underscore'] = utils.build_underscore_name_from_camelcase(field['format'])
            translations['field_name'] = field['name']
            translations['array_size_field'] = utils.build_underscore_name_from_camelcase(field['array-size-field']) if 'array-size-field' in field else ''
            translations['struct_name'] = utils.build_underscore_name_from_camelcase(field['struct-type']) if 'struct-type' in field else ''
            translations['struct_type'] = field['struct-type'] if 'struct-type' in field else ''
            translations['array_size'] = field['array-size'] if 'array-size' in field else ''

            inner_template = (
                '\n'
                '    /* Read the \'${field_name}\' variable */\n')
            if 'available-if' in field:
                condition = field['available-if']
                translations['condition_field'] = utils.build_underscore_name_from_camelcase(condition['field'])
                translations['condition_operation'] = condition['operation']
                translations['condition_value'] = condition['value']
                inner_template += (
                    '    if (!(_${condition_field} ${condition_operation} ${condition_value})) {\n')
                if field['format'] == 'byte-array':
                    inner_template += (
                        '        if (out_${field})\n'
                        '            *out_${field} = NULL;\n')
                elif field['format'] == 'unsized-byte-array' or \
                   field['format'] == 'ref-byte-array' or \
                   field['format'] == 'uicc-ref-byte-array':
                    inner_template += (
                        '        if (out_${field}_size)\n'
                        '            *out_${field}_size = 0;\n'
                        '        if (out_${field})\n'
                        '            *out_${field} = NULL;\n')
                elif field['format'] == 'string' or \
                     field['format'] == 'string-array' or \
                     field['format'] == 'struct' or \
                     field['format'] == 'struct-array' or \
                     field['format'] == 'ref-struct-array' or \
                     field['format'] == 'ipv4' or \
                     field['format'] == 'ref-ipv4' or \
                     field['format'] == 'ipv4-array' or \
                     field['format'] == 'ipv6' or \
                     field['format'] == 'ref-ipv6' or \
                     field['format'] == 'ipv6-array':
                    inner_template += (
                        '        if (out_${field} != NULL)\n'
                        '            *out_${field} = NULL;\n')
                else:
                    raise ValueError('Field format \'%s\' unsupported as optional field' % field['format'])

                inner_template += (
                    '    } else {\n')
            else:
                inner_template += (
                    '    {\n')

            if 'always-read' in field:
                inner_template += (
                    '        if (!_mbim_message_read_guint32 (message, offset, &_${field}, error))\n'
                    '            goto out;\n'
                    '        if (out_${field} != NULL)\n'
                    '            *out_${field} = _${field};\n'
                    '        offset += 4;\n')
            elif field['format'] == 'byte-array':
                inner_template += (
                    '        const guint8 *tmp;\n'
                    '\n'
                    '        if (!_mbim_message_read_byte_array (message, 0, offset, FALSE, FALSE, ${array_size}, &tmp, NULL, error, FALSE))\n'
                    '            goto out;\n'
                    '        if (out_${field} != NULL)\n'
                    '            *out_${field} = tmp;\n'
                    '        offset += ${array_size};\n')
            elif field['format'] == 'unsized-byte-array':
                inner_template += (
                    '        const guint8 *tmp;\n'
                    '        guint32 tmpsize;\n'
                    '\n'
                    '        if (!_mbim_message_read_byte_array (message, 0, offset, FALSE, FALSE, 0, &tmp, &tmpsize, error, FALSE))\n'
                    '            goto out;\n'
                    '        if (out_${field} != NULL)\n'
                    '            *out_${field} = tmp;\n'
                    '        if (out_${field}_size != NULL)\n'
                    '            *out_${field}_size = tmpsize;\n'
                    '        offset += tmpsize;\n')
            elif field['format'] == 'ref-byte-array':
                inner_template += (
                    '        const guint8 *tmp;\n'
                    '        guint32 tmpsize;\n'
                    '\n'
                    '        if (!_mbim_message_read_byte_array (message, 0, offset, TRUE, TRUE, 0, &tmp, &tmpsize, error, FALSE))\n'
                    '            goto out;\n'
                    '        if (out_${field} != NULL)\n'
                    '            *out_${field} = tmp;\n'
                    '        if (out_${field}_size != NULL)\n'
                    '            *out_${field}_size = tmpsize;\n'
                    '        offset += 8;\n')
            elif field['format'] == 'uicc-ref-byte-array':
                inner_template += (
                    '        const guint8 *tmp;\n'
                    '        guint32 tmpsize;\n'
                    '\n'
                    '        if (!_mbim_message_read_byte_array (message, 0, offset, TRUE, TRUE, 0, &tmp, &tmpsize, error, TRUE))\n'
                    '            goto out;\n'
                    '        if (out_${field} != NULL)\n'
                    '            *out_${field} = tmp;\n'
                    '        if (out_${field}_size != NULL)\n'
                    '            *out_${field}_size = tmpsize;\n'
                    '        offset += 8;\n')
            elif field['format'] == 'uuid':
                inner_template += (
                    '        if ((out_${field} != NULL) && !_mbim_message_read_uuid (message, offset, out_${field}, error))\n'
                    '            goto out;\n'
                    '        offset += 16;\n')
            elif field['format'] == 'guint32':
                if 'public-format' in field:
                    translations['public'] = field['public-format'] if 'public-format' in field else field['format']
                    inner_template += (
                        '        if (out_${field} != NULL) {\n'
                        '            guint32 aux;\n'
                        '\n'
                        '            if (!_mbim_message_read_guint32 (message, offset, &aux, error))\n'
                        '                goto out;\n'
                        '            *out_${field} = (${public})aux;\n'
                        '        }\n')
                else:
                    inner_template += (
                        '        if ((out_${field} != NULL) && !_mbim_message_read_guint32 (message, offset, out_${field}, error))\n'
                        '            goto out;\n')
                inner_template += (
                    '        offset += 4;\n')
            elif field['format'] == 'guint64':
                if 'public-format' in field:
                    translations['public'] = field['public-format'] if 'public-format' in field else field['format']
                    inner_template += (
                        '        if (out_${field} != NULL) {\n'
                        '            guint64 aux;\n'
                        '\n'
                        '            if (!_mbim_message_read_guint64 (message, offset, &aux, error))\n'
                        '                goto out;\n'
                        '            *out_${field} = (${public})aux;\n'
                        '        }\n')
                else:
                    inner_template += (
                        '        if ((out_${field} != NULL) && !_mbim_message_read_guint64 (message, offset, out_${field}, error))\n'
                        '            goto out;\n')
                inner_template += (
                    '        offset += 8;\n')
            elif field['format'] == 'string':
                inner_template += (
                    '        if ((out_${field} != NULL) && !_mbim_message_read_string (message, 0, offset, &_${field}, error))\n'
                    '            goto out;\n'
                    '        offset += 8;\n')
            elif field['format'] == 'string-array':
                inner_template += (
                    '        if ((out_${field} != NULL) && !_mbim_message_read_string_array (message, _${array_size_field}, 0, offset, &_${field}, error))\n'
                    '            goto out;\n'
                    '        offset += (8 * _${array_size_field});\n')
            elif field['format'] == 'struct':
                inner_template += (
                    '        ${struct_type} *tmp;\n'
                    '        guint32 bytes_read = 0;\n'
                    '\n'
                    '        tmp = _mbim_message_read_${struct_name}_struct (message, offset, &bytes_read, error);\n'
                    '        if (!tmp)\n'
                    '            goto out;\n'
                    '        if (out_${field} != NULL)\n'
                    '            _${field} = tmp;\n'
                    '        else\n'
                    '             _${struct_name}_free (tmp);\n'
                    '        offset += bytes_read;\n')
            elif field['format'] == 'struct-array':
                inner_template += (
                    '        if ((out_${field} != NULL) && !_mbim_message_read_${struct_name}_struct_array (message, _${array_size_field}, offset, FALSE, &_${field}, error))\n'
                    '            goto out;\n'
                    '        offset += 4;\n')
            elif field['format'] == 'ref-struct-array':
                inner_template += (
                    '        if ((out_${field} != NULL) && !_mbim_message_read_${struct_name}_struct_array (message, _${array_size_field}, offset, TRUE, &_${field}, error))\n'
                    '            goto out;\n'
                    '        offset += (8 * _${array_size_field});\n')
            elif field['format'] == 'ipv4':
                inner_template += (
                    '        if ((out_${field} != NULL) && !_mbim_message_read_ipv4 (message, offset, FALSE, out_${field}, error))\n'
                    '            goto out;\n'
                    '        offset += 4;\n')
            elif field['format'] == 'ref-ipv4':
                inner_template += (
                    '        if ((out_${field} != NULL) && !_mbim_message_read_ipv4 (message, offset, TRUE, out_${field}, error))\n'
                    '            goto out;\n'
                    '        offset += 4;\n')
            elif field['format'] == 'ipv4-array':
                inner_template += (
                    '        if ((out_${field} != NULL) && !_mbim_message_read_ipv4_array (message, _${array_size_field}, offset, &_${field}, error))\n'
                    '            goto out;\n'
                    '        offset += 4;\n')
            elif field['format'] == 'ipv6':
                inner_template += (
                    '        if ((out_${field} != NULL) && !_mbim_message_read_ipv6 (message, offset, FALSE, out_${field}, error))\n'
                    '            goto out;\n'
                    '        offset += 16;\n')
            elif field['format'] == 'ref-ipv6':
                inner_template += (
                    '        if ((out_${field} != NULL) && !_mbim_message_read_ipv6 (message, offset, TRUE, out_${field}, error))\n'
                    '            goto out;\n'
                    '        offset += 4;\n')
            elif field['format'] == 'ipv6-array':
                inner_template += (
                    '        if ((out_${field} != NULL) && !_mbim_message_read_ipv6_array (message, _${array_size_field}, offset, &_${field}, error))\n'
                    '            goto out;\n'
                    '        offset += 4;\n')

            inner_template += (
                '    }\n')

            template += (string.Template(inner_template).substitute(translations))

        if fields != []:
            template += (
                '\n'
                '    /* All variables successfully parsed */\n'
                '    success = TRUE;\n'
                '\n'
                ' out:\n'
                '\n')

        if count_allocated_variables > 0:
            template += (
                '    if (success) {\n'
                '        /* Memory allocated variables as output */\n')
            for field in fields:
                translations['field'] = utils.build_underscore_name_from_camelcase(field['name'])
                # set as output memory allocated values
                inner_template = ''
                if field['format'] == 'string' or \
                   field['format'] == 'string-array' or \
                   field['format'] == 'struct' or \
                   field['format'] == 'struct-array' or \
                   field['format'] == 'ref-struct-array' or \
                   field['format'] == 'ipv4-array' or \
                   field['format'] == 'ipv6-array':
                    inner_template = ('        if (out_${field} != NULL)\n'
                                      '            *out_${field} = _${field};\n')
                    template += (string.Template(inner_template).substitute(translations))
            template += (
                '    } else {\n')
            for field in fields:
                translations['field'] = utils.build_underscore_name_from_camelcase(field['name'])
                translations['struct'] = field['struct-type'] if 'struct-type' in field else ''
                translations['struct_underscore'] = utils.build_underscore_name_from_camelcase (translations['struct'])
                inner_template = ''
                if field['format'] == 'string' or \
                   field['format'] == 'ipv4-array' or \
                   field['format'] == 'ipv6-array':
                    inner_template = ('        g_free (_${field});\n')
                elif field['format'] == 'string-array':
                    inner_template = ('        g_strfreev (_${field});\n')
                elif field['format'] == 'struct':
                    inner_template = ('        ${struct_underscore}_free (_${field});\n')
                elif field['format'] == 'struct-array' or field['format'] == 'ref-struct-array':
                    inner_template = ('        ${struct_underscore}_array_free (_${field});\n')
                template += (string.Template(inner_template).substitute(translations))
            template += (
                '    }\n')

        if fields != []:
            template += (
                '\n'
                '    return success;\n'
                '}\n')
        else:
            template += (
                '\n'
                '    return TRUE;\n'
                '}\n')
        cfile.write(string.Template(template).substitute(translations))


    """
    Emit message printable
    """
    def _emit_message_printable(self, cfile, message_type, fields):
        translations = { 'message'                  : self.name,
                         'underscore'               : utils.build_underscore_name(self.name),
                         'service'                  : self.service,
                         'underscore'               : utils.build_underscore_name (self.fullname),
                         'message_type'             : message_type,
                         'message_type_upper'       : message_type.upper(),
                         'service_underscore_upper' : utils.build_underscore_name (self.service).upper() }
        template = (
            '\n'
            'static gchar *\n'
            '${underscore}_${message_type}_get_printable (\n'
            '    const MbimMessage *message,\n'
            '    const gchar *line_prefix,\n'
            '    GError **error)\n'
            '{\n'
            '    GString *str;\n')

        if fields != []:
            template += (
                '    GError *inner_error = NULL;\n'
                '    guint32 offset = 0;\n')

        for field in fields:
            if 'always-read' in field:
                translations['field'] = utils.build_underscore_name_from_camelcase(field['name'])
                inner_template = ('    guint32 _${field};\n')
                template += (string.Template(inner_template).substitute(translations))

        if message_type == 'response':
            template += (
                '\n'
                '    if (!mbim_message_response_get_result (message, MBIM_MESSAGE_TYPE_COMMAND_DONE, NULL))\n'
                '        return NULL;\n')

        template += (
            '\n'
            '    str = g_string_new ("");\n')

        for field in fields:
            translations['field']                   = utils.build_underscore_name_from_camelcase(field['name'])
            translations['field_format']            = field['format']
            translations['field_format_underscore'] = utils.build_underscore_name_from_camelcase(field['format'])
            translations['public']                  = field['public-format'] if 'public-format' in field else field['format']
            translations['public_underscore']       = utils.build_underscore_name_from_camelcase(field['public-format']) if 'public-format' in field else ''
            translations['public_underscore_upper'] = utils.build_underscore_name_from_camelcase(field['public-format']).upper() if 'public-format' in field else ''
            translations['field_name']              = field['name']
            translations['array_size_field']        = utils.build_underscore_name_from_camelcase(field['array-size-field']) if 'array-size-field' in field else ''
            translations['struct_name']             = utils.build_underscore_name_from_camelcase(field['struct-type']) if 'struct-type' in field else ''
            translations['struct_type']             = field['struct-type'] if 'struct-type' in field else ''
            translations['array_size']              = field['array-size'] if 'array-size' in field else ''

            inner_template = (
                '\n'
                '    g_string_append_printf (str, "%s  ${field_name} = ", line_prefix);\n')

            if 'available-if' in field:
                condition = field['available-if']
                translations['condition_field'] = utils.build_underscore_name_from_camelcase(condition['field'])
                translations['condition_operation'] = condition['operation']
                translations['condition_value'] = condition['value']
                inner_template += (
                    '    if (_${condition_field} ${condition_operation} ${condition_value}) {\n')
            else:
                inner_template += (
                    '    {\n')

            if 'always-read' in field:
                inner_template += (
                    '        if (!_mbim_message_read_guint32 (message, offset, &_${field}, &inner_error))\n'
                    '            goto out;\n'
                    '        offset += 4;\n'
                    '        g_string_append_printf (str, "\'%" G_GUINT32_FORMAT "\'", _${field});\n')

            elif field['format'] == 'byte-array' or \
                 field['format'] == 'unsized-byte-array' or \
                 field['format'] == 'ref-byte-array' or \
                 field['format'] == 'uicc-ref-byte-array' or \
                 field['format'] == 'ref-byte-array-no-offset':
                inner_template += (
                    '        guint i;\n'
                    '        const guint8 *tmp;\n'
                    '        guint32 tmpsize;\n'
                    '\n')
                if field['format'] == 'byte-array':
                    inner_template += (
                        '        if (!_mbim_message_read_byte_array (message, 0, offset, FALSE, FALSE, ${array_size}, &tmp, NULL, &inner_error, FALSE))\n'
                        '            goto out;\n'
                        '        tmpsize = ${array_size};\n'
                        '        offset += ${array_size};\n')
                elif field['format'] == 'unsized-byte-array':
                    inner_template += (
                        '        if (!_mbim_message_read_byte_array (message, 0, offset, FALSE, FALSE, 0, &tmp, &tmpsize, &inner_error, FALSE))\n'
                        '            goto out;\n'
                        '        offset += tmpsize;\n')

                elif field['format'] == 'ref-byte-array':
                    inner_template += (
                        '        if (!_mbim_message_read_byte_array (message, 0, offset, TRUE, TRUE, 0, &tmp, &tmpsize, &inner_error, FALSE))\n'
                        '            goto out;\n'
                        '        offset += 8;\n')

                elif field['format'] == 'uicc-ref-byte-array':
                    inner_template += (
                        '        if (!_mbim_message_read_byte_array (message, 0, offset, TRUE, TRUE, 0, &tmp, &tmpsize, &inner_error, TRUE))\n'
                        '            goto out;\n'
                        '        offset += 8;\n')

                elif field['format'] == 'ref-byte-array-no-offset':
                    inner_template += (
                        '        if (!_mbim_message_read_byte_array (message, 0, offset, FALSE, TRUE, 0, &tmp, &tmpsize, &inner_error, FALSE))\n'
                        '            goto out;\n'
                        '        offset += 4;\n')

                inner_template += (
                    '        g_string_append (str, "\'");\n'
                    '        for (i = 0; i  < tmpsize; i++)\n'
                    '            g_string_append_printf (str, "%02x%s", tmp[i], (i == (tmpsize - 1)) ? "" : ":" );\n'
                    '        g_string_append (str, "\'");\n')

            elif field['format'] == 'uuid':
                inner_template += (
                    '        const MbimUuid   *tmp;\n'
                    '        g_autofree gchar *tmpstr = NULL;\n'
                    '\n'
                    '        if (!_mbim_message_read_uuid (message, offset, &tmp, &inner_error))\n'
                    '            goto out;\n'
                    '        offset += 16;\n'
                    '        tmpstr = mbim_uuid_get_printable (tmp);\n'
                    '        g_string_append_printf (str, "\'%s\'", tmpstr);\n')

            elif field['format'] == 'guint32' or \
                 field['format'] == 'guint64':
                inner_template += (
                    '        ${field_format} tmp;\n'
                    '\n')
                if field['format'] == 'guint32' :
                    inner_template += (
                        '        if (!_mbim_message_read_guint32 (message, offset, &tmp, &inner_error))\n'
                        '            goto out;\n'
                        '        offset += 4;\n')
                elif field['format'] == 'guint64' :
                    inner_template += (
                        '        if (!_mbim_message_read_guint64 (message, offset, &tmp, &inner_error))\n'
                        '            goto out;\n'
                        '        offset += 8;\n')

                if 'public-format' in field:
                    inner_template += (
                        '#if defined __${public_underscore_upper}_IS_ENUM__\n'
                        '        g_string_append_printf (str, "\'%s\'", ${public_underscore}_get_string ((${public})tmp));\n'
                        '#elif defined __${public_underscore_upper}_IS_FLAGS__\n'
                        '        {\n'
                        '            g_autofree gchar *tmpstr = NULL;\n'
                        '\n'
                        '            tmpstr = ${public_underscore}_build_string_from_mask ((${public})tmp);\n'
                        '            g_string_append_printf (str, "\'%s\'", tmpstr);\n'
                        '        }\n'
                        '#else\n'
                        '# error neither enum nor flags\n'
                        '#endif\n'
                        '\n')
                elif field['format'] == 'guint32':
                    inner_template += (
                        '        g_string_append_printf (str, "\'%" G_GUINT32_FORMAT "\'", tmp);\n')
                elif field['format'] == 'guint64':
                    inner_template += (
                        '        g_string_append_printf (str, "\'%" G_GUINT64_FORMAT "\'", tmp);\n')

            elif field['format'] == 'string':
                inner_template += (
                    '        g_autofree gchar *tmp = NULL;\n'
                    '\n'
                    '        if (!_mbim_message_read_string (message, 0, offset, &tmp, &inner_error))\n'
                    '            goto out;\n'
                    '        offset += 8;\n'
                    '        g_string_append_printf (str, "\'%s\'", tmp);\n')

            elif field['format'] == 'string-array':
                inner_template += (
                    '        g_auto(GStrv) tmp = NULL;\n'
                    '        guint i;\n'
                    '\n'
                    '        if (!_mbim_message_read_string_array (message, _${array_size_field}, 0, offset, &tmp, &inner_error))\n'
                    '            goto out;\n'
                    '        offset += (8 * _${array_size_field});\n'
                    '\n'
                    '        g_string_append (str, "\'");\n'
                    '        for (i = 0; i < _${array_size_field}; i++) {\n'
                    '            g_string_append (str, tmp[i]);\n'
                    '            if (i < (_${array_size_field} - 1))\n'
                    '                g_string_append (str, ", ");\n'
                    '        }\n'
                    '        g_string_append (str, "\'");\n')

            elif field['format'] == 'struct':
                inner_template += (
                    '        g_autoptr(${struct_type}) tmp = NULL;\n'
                    '        g_autofree gchar *new_line_prefix = NULL;\n'
                    '        g_autofree gchar *struct_str = NULL;\n'
                    '        guint32 bytes_read = 0;\n'
                    '\n'
                    '        tmp = _mbim_message_read_${struct_name}_struct (message, offset, &bytes_read, &inner_error);\n'
                    '        if (!tmp)\n'
                    '            goto out;\n'
                    '        offset += bytes_read;\n'
                    '        g_string_append (str, "{\\n");\n'
                    '        new_line_prefix = g_strdup_printf ("%s    ", line_prefix);\n'
                    '        struct_str = _mbim_message_print_${struct_name}_struct (tmp, new_line_prefix);\n'
                    '        g_string_append (str, struct_str);\n'
                    '        g_string_append_printf (str, "%s  }\\n", line_prefix);\n')

            elif field['format'] == 'struct-array' or field['format'] == 'ref-struct-array':
                inner_template += (
                    '        g_autoptr(${struct_type}Array) tmp = NULL;\n'
                    '        g_autofree gchar *new_line_prefix = NULL;\n'
                    '        guint i;\n'
                    '\n')

                if field['format'] == 'struct-array':
                    inner_template += (
                    '        if (!_mbim_message_read_${struct_name}_struct_array (message, _${array_size_field}, offset, FALSE, &tmp, &inner_error))\n'
                    '            goto out;\n'
                    '        offset += 4;\n')
                elif field['format'] == 'ref-struct-array':
                    inner_template += (
                    '        if (!_mbim_message_read_${struct_name}_struct_array (message, _${array_size_field}, offset, TRUE, &tmp, &inner_error))\n'
                    '            goto out;\n'
                    '        offset += (8 * _${array_size_field});\n')

                inner_template += (
                    '        new_line_prefix = g_strdup_printf ("%s        ", line_prefix);\n'
                    '        g_string_append (str, "\'{\\n");\n'
                    '        for (i = 0; i < _${array_size_field}; i++) {\n'
                    '            g_autofree gchar *struct_str = NULL;\n'
                    '\n'
                    '            g_string_append_printf (str, "%s    [%u] = {\\n", line_prefix, i);\n'
                    '            struct_str = _mbim_message_print_${struct_name}_struct (tmp[i], new_line_prefix);\n'
                    '            g_string_append (str, struct_str);\n'
                    '            g_string_append_printf (str, "%s    },\\n", line_prefix);\n'
                    '        }\n'
                    '        g_string_append_printf (str, "%s  }\'", line_prefix);\n')

            elif field['format'] == 'ipv4' or \
                 field['format'] == 'ref-ipv4' or \
                 field['format'] == 'ipv4-array' or \
                 field['format'] == 'ipv6' or \
                 field['format'] == 'ref-ipv6' or \
                 field['format'] == 'ipv6-array':
                if field['format'] == 'ipv4' or \
                   field['format'] == 'ref-ipv4':
                    inner_template += (
                        '        const MbimIPv4 *tmp;\n')
                elif field['format'] == 'ipv4-array':
                    inner_template += (
                        '        g_autofree MbimIPv4 *tmp = NULL;\n')
                elif field['format'] == 'ipv6' or \
                     field['format'] == 'ref-ipv6':
                    inner_template += (
                        '        const MbimIPv6 *tmp;\n')
                elif field['format'] == 'ipv6-array':
                    inner_template += (
                        '        g_autofree MbimIPv6 *tmp = NULL;\n')

                inner_template += (
                    '        guint array_size;\n'
                    '        guint i;\n'
                    '\n')

                if field['format'] == 'ipv4':
                    inner_template += (
                        '        array_size = 1;\n'
                        '        if (!_mbim_message_read_ipv4 (message, offset, FALSE, &tmp, &inner_error))\n'
                        '            goto out;\n'
                        '        offset += 4;\n')
                elif field['format'] == 'ref-ipv4':
                    inner_template += (
                        '        array_size = 1;\n'
                        '        if (!_mbim_message_read_ipv4 (message, offset, TRUE, &tmp, &inner_error))\n'
                        '            goto out;\n'
                        '        offset += 4;\n')
                elif field['format'] == 'ipv4-array':
                    inner_template += (
                        '        array_size = _${array_size_field};\n'
                        '        if (!_mbim_message_read_ipv4_array (message, _${array_size_field}, offset, &tmp, &inner_error))\n'
                        '            goto out;\n'
                        '        offset += 4;\n')
                elif field['format'] == 'ipv6':
                    inner_template += (
                        '        array_size = 1;\n'
                        '        if (!_mbim_message_read_ipv6 (message, offset, FALSE, &tmp, &inner_error))\n'
                        '            goto out;\n'
                        '        offset += 16;\n')
                elif field['format'] == 'ref-ipv6':
                    inner_template += (
                        '        array_size = 1;\n'
                        '        if (!_mbim_message_read_ipv6 (message, offset, TRUE, &tmp, &inner_error))\n'
                        '            goto out;\n'
                        '        offset += 4;\n')
                elif field['format'] == 'ipv6-array':
                    inner_template += (
                        '        array_size = _${array_size_field};\n'
                        '        if (!_mbim_message_read_ipv6_array (message, _${array_size_field}, offset, &tmp, &inner_error))\n'
                        '            goto out;\n'
                        '        offset += 4;\n')

                inner_template += (
                    '        g_string_append (str, "\'");\n'
                    '        if (tmp) {\n'
                    '            for (i = 0; i < array_size; i++) {\n'
                    '                g_autoptr(GInetAddress)  addr = NULL;\n'
                    '                g_autofree gchar        *tmpstr = NULL;\n'
                    '\n')

                if field['format'] == 'ipv4' or \
                   field['format'] == 'ref-ipv4' or \
                   field['format'] == 'ipv4-array':
                    inner_template += (
                        '                addr = g_inet_address_new_from_bytes ((guint8 *)&(tmp[i].addr), G_SOCKET_FAMILY_IPV4);\n')
                elif field['format'] == 'ipv6' or \
                     field['format'] == 'ref-ipv6' or \
                     field['format'] == 'ipv6-array':
                    inner_template += (
                        '                addr = g_inet_address_new_from_bytes ((guint8 *)&(tmp[i].addr), G_SOCKET_FAMILY_IPV6);\n')

                inner_template += (
                    '                tmpstr = g_inet_address_to_string (addr);\n'
                    '                g_string_append_printf (str, "%s", tmpstr);\n'
                    '                if (i < (array_size - 1))\n'
                    '                    g_string_append (str, ", ");\n'
                    '            }\n'
                    '        }\n'
                    '        g_string_append (str, "\'");\n')

            else:
                raise ValueError('Field format \'%s\' not printable' % field['format'])

            inner_template += (
                '    }\n'
                '    g_string_append (str, "\\n");\n')

            template += (string.Template(inner_template).substitute(translations))

        if fields != []:
            template += (
                '\n'
                ' out:\n'
                '    if (inner_error) {\n'
                '        g_string_append_printf (str, "n/a: %s", inner_error->message);\n'
                '        g_clear_error (&inner_error);\n'
                '    }\n')

        template += (
            '\n'
            '    return g_string_free (str, FALSE);\n'
            '}\n')
        cfile.write(string.Template(template).substitute(translations))


    """
    Emit the section content
    """
    def emit_section_content(self, sfile):
        translations = { 'name_dashed' : utils.build_dashed_name(self.name),
                         'underscore'  : utils.build_underscore_name(self.fullname) }

        template = (
            '\n'
            '<SUBSECTION ${name_dashed}>\n')
        sfile.write(string.Template(template).substitute(translations))

        if self.has_query:
            template = (
                '${underscore}_query_new\n')
            sfile.write(string.Template(template).substitute(translations))

        if self.has_set:
            template = (
                '${underscore}_set_new\n')
            sfile.write(string.Template(template).substitute(translations))

        if self.has_response:
            template = (
                '${underscore}_response_parse\n')
            sfile.write(string.Template(template).substitute(translations))

        if self.has_notification:
            template = (
                '${underscore}_notification_parse\n')
            sfile.write(string.Template(template).substitute(translations))
