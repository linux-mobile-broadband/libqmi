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

from MessageList import MessageList
import utils

class Client:
    def __init__(self, objects_dictionary):
        self.name = None

        # Loop items in the list, looking for the special 'Client' type
        for object_dictionary in objects_dictionary:
            if object_dictionary['type'] == 'Client':
                self.name = object_dictionary['name']

        # We NEED the Client field
        if self.name is None:
            raise ValueError('Missing Client field')


    def __emit_class(self, hfile, cfile):
        translations = { 'underscore'                 : utils.build_underscore_name(self.name),
                         'no_prefix_underscore_upper' : string.upper(utils.build_underscore_name(self.name[4:])),
                         'camelcase'                  : utils.build_camelcase_name (self.name) }

        # Emit class header
        template = (
            '#define QMI_TYPE_${no_prefix_underscore_upper}            (${underscore}_get_type ())\n'
            '#define QMI_${no_prefix_underscore_upper}(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), QMI_TYPE_${no_prefix_underscore_upper}, ${camelcase}))\n'
            '#define QMI_${no_prefix_underscore_upper}_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  QMI_TYPE_${no_prefix_underscore_upper}, ${camelcase}Class))\n'
            '#define QMI_IS_${no_prefix_underscore_upper}(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), QMI_TYPE_${no_prefix_underscore_upper}))\n'
            '#define QMI_IS_${no_prefix_underscore_upper}_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  QMI_TYPE_${no_prefix_underscore_upper}))\n'
            '#define QMI_${no_prefix_underscore_upper}_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  QMI_TYPE_${no_prefix_underscore_upper}, ${camelcase}Class))\n'
            '\n'
            'typedef struct _${camelcase} ${camelcase};\n'
            'typedef struct _${camelcase}Class ${camelcase}Class;\n'
            '\n'
            'struct _${camelcase} {\n'
            '    QmiClient parent;\n'
            '    gpointer priv_unused;\n'
            '};\n'
            '\n'
            'struct _${camelcase}Class {\n'
            '    QmiClientClass parent;\n'
            '};\n'
            '\n'
            'GType ${underscore}_get_type (void);\n'
            '\n')
        hfile.write(string.Template(template).substitute(translations))

        # Emit class source
        template = (
            '\n'
            'G_DEFINE_TYPE (${camelcase}, ${underscore}, QMI_TYPE_CLIENT);\n'
            '\n'
            'static void\n'
            '${underscore}_init (${camelcase} *self)\n'
            '{\n'
            '}\n'
            '\n'
            'static void\n'
            '${underscore}_class_init (${camelcase}Class *klass)\n'
            '{\n'
            '}\n'
            '\n')
        cfile.write(string.Template(template).substitute(translations))


    def __emit_methods(self, hfile, cfile, message_list):
        translations = { 'underscore' : utils.build_underscore_name(self.name),
                         'camelcase'  : utils.build_camelcase_name (self.name) }

        for message in message_list.list:
            translations['message_underscore'] = utils.build_underscore_name(message.name)
            translations['message_fullname_underscore'] = utils.build_underscore_name(message.fullname)
            translations['input_camelcase'] = utils.build_camelcase_name(message.input.fullname)
            translations['output_camelcase'] = utils.build_camelcase_name(message.output.fullname)
            translations['input_underscore'] = utils.build_underscore_name(message.input.fullname)
            translations['output_underscore'] = utils.build_underscore_name(message.output.fullname)

            if message.input.fields is None:
                input_arg_template = 'gpointer unused'
                translations['input_var'] = 'NULL'
            else:
                input_arg_template = '${input_camelcase} *input'
                translations['input_var'] = 'input'
            template = (
                '\n'
                'void ${underscore}_${message_underscore} (\n'
                '    ${camelcase} *self,\n'
                '    %s,\n'
                '    guint timeout,\n'
                '    GCancellable *cancellable,\n'
                '    GAsyncReadyCallback callback,\n'
                '    gpointer user_data);\n'
                '${output_camelcase} *${underscore}_${message_underscore}_finish (\n'
                '    ${camelcase} *self,\n'
                '    GAsyncResult *res,\n'
                '    GError **error);\n' % input_arg_template)
            hfile.write(string.Template(template).substitute(translations))

            template = (
                '\n'
                '${output_camelcase} *\n'
                '${underscore}_${message_underscore}_finish (\n'
                '    ${camelcase} *self,\n'
                '    GAsyncResult *res,\n'
                '    GError **error)\n'
                '{\n'
                '   if (g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (res), error))\n'
                '       return NULL;\n'
                '\n'
                '   return ${output_underscore}_ref (g_simple_async_result_get_op_res_gpointer (G_SIMPLE_ASYNC_RESULT (res)));\n'
                '}\n'
                '\n'
                'static void\n'
                '${message_underscore}_ready (\n'
                '    QmiDevice *device,\n'
                '    GAsyncResult *res,\n'
                '    GSimpleAsyncResult *simple)\n'
                '{\n'
                '    GError *error = NULL;\n'
                '    QmiMessage *reply;\n'
                '    ${output_camelcase} *output;\n'
                '\n'
                '    reply = qmi_device_command_finish (device, res, &error);\n'
                '    if (!reply) {\n'
                '        g_simple_async_result_take_error (simple, error);\n'
                '        g_simple_async_result_complete (simple);\n'
                '        g_object_unref (simple);\n'
                '        return;\n'
                '    }\n'
                '\n'
                '    /* Parse reply */\n'
                '    output = ${message_fullname_underscore}_response_parse (reply, &error);\n'
                '    if (!output)\n'
                '        g_simple_async_result_take_error (simple, error);\n'
                '    else\n'
                '        g_simple_async_result_set_op_res_gpointer (simple,\n'
                '                                                   output,\n'
                '                                                   (GDestroyNotify)${output_underscore}_unref);\n'
                '    g_simple_async_result_complete (simple);\n'
                '    g_object_unref (simple);\n'
                '    qmi_message_unref (reply);\n'
                '}\n'
                '\n'
                'void\n'
                '${underscore}_${message_underscore} (\n'
                '    ${camelcase} *self,\n'
                '    %s,\n'
                '    guint timeout,\n'
                '    GCancellable *cancellable,\n'
                '    GAsyncReadyCallback callback,\n'
                '    gpointer user_data)\n'
                '{\n'
                '    GSimpleAsyncResult *result;\n'
                '    QmiMessage *request;\n'
                '    GError *error = NULL;\n'
                '\n'
                '    result = g_simple_async_result_new (G_OBJECT (self),\n'
                '                                        callback,\n'
                '                                        user_data,\n'
                '                                        ${underscore}_${message_underscore});\n'
                '\n'
                '    request = ${message_fullname_underscore}_request_create (\n'
                '                  qmi_client_get_next_transaction_id (QMI_CLIENT (self)),\n'
                '                  qmi_client_get_cid (QMI_CLIENT (self)),\n'
                '                  ${input_var},\n'
                '                  &error);\n'
                '    if (!request) {\n'
                '        g_prefix_error (&error, "Couldn\'t create request message: ");\n'
                '        g_simple_async_result_take_error (result, error);\n'
                '        g_simple_async_result_complete_in_idle (result);\n'
                '        g_object_unref (result);\n'
                '        return;\n'
                '    }\n'
                '\n'
                '    qmi_device_command (QMI_DEVICE (qmi_client_peek_device (QMI_CLIENT (self))),\n'
                '                        request,\n'
                '                        timeout,\n'
                '                        cancellable,\n'
                '                        (GAsyncReadyCallback)${message_underscore}_ready,\n'
                '                        result);\n'
                '    qmi_message_unref (request);\n'
                '}\n'
                '\n'  % input_arg_template)
            cfile.write(string.Template(template).substitute(translations))

    def emit(self, hfile, cfile, message_list):
        # First, emit common class code
        utils.add_separator(hfile, 'CLIENT', self.name);
        utils.add_separator(cfile, 'CLIENT', self.name);
        self.__emit_class(hfile, cfile)
        self.__emit_methods(hfile, cfile, message_list)
