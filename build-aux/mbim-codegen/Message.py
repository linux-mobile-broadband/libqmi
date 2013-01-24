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
# Copyright (C) 2013 Aleksander Morgado <aleksander@gnu.org>
#

import string

from Container import Container
import utils

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

        # Gather types of command
        self.can_query  = True if 'query'  in dictionary else False
        self.can_set    = True if 'set'    in dictionary else False
        self.can_notify = True if 'notify' in dictionary else False

        # Build Fullname
        self.fullname = 'MBIM Message ' + self.service + ' ' + self.name

        # Build CID enum
        self.cid_enum_name = utils.build_underscore_name('MBIM CID' + self.service + ' ' + self.name).upper()

        # Build Query containers
        self.query_input = None
        self.query_output = None
        if self.can_query:
            if 'input' in dictionary['query']:
                self.query_input = Container(self.service, self.name, 'Query', 'Input', dictionary['query']['input'])
            if 'output' in dictionary['query']:
                self.query_output = Container(self.service, self.name, 'Query', 'Output', dictionary['query']['output'])

        # Build Set containers
        self.set_input = None
        self.set_output = None
        if self.can_set:
            if 'input' in dictionary['set']:
                self.query_input = Container(self.service, self.name, 'Set', 'Input', dictionary['set']['input'])
            if 'output' in dictionary['set']:
                self.query_output = Container(self.service, self.name, 'Set', 'Output', dictionary['set']['output'])

        # Build Notify containers
        self.notify_output = None
        if self.can_notify:
            if 'output' in dictionary['notify']:
                self.query_output = Container(self.service, self.name, 'Notify', 'Output', dictionary['notify']['output'])

    """
    Emit request creator
    """
    def _emit_request_creator(self, hfile, cfile, message_type):
        translations = { 'name'                     : self.name,
                         'service'                  : self.service,
                         'underscore'               : utils.build_underscore_name (self.fullname),
                         'message_type'             : message_type,
                         'message_type_upper'       : message_type.upper(),
                         'service_underscore_upper' : utils.build_underscore_name (self.service).upper(),
                         'name_underscore_upper'    : utils.build_underscore_name (self.name).upper() }
        template = (
            '\n'
            'MbimMessage *${underscore}_${message_type}_request_new (guint32 transaction_id);\n')
        hfile.write(string.Template(template).substitute(translations))

        template = (
            '\n'
            '/**\n'
            ' * ${underscore}_${message_type}_request_new:\n'
            ' * @transaction_id: the transaction ID to use in the request.\n'
            ' *\n'
            ' * Create a new request for the \'${name}\' command in the \'${service}\' service.\n'
            ' *\n'
            ' * Returns: a newly allocated #MbimMessage, which should be freed with mbim_message_unref().\n'
            ' */\n'
            'MbimMessage *\n'
            '${underscore}_${message_type}_request_new (guint32 transaction_id)\n'
            '{\n'
            '    return mbim_message_command_new (transaction_id,\n'
            '                                     MBIM_SERVICE_${service_underscore_upper},\n'
            '                                     MBIM_CID_${service_underscore_upper}_${name_underscore_upper},\n'
            '                                     MBIM_MESSAGE_COMMAND_TYPE_${message_type_upper});\n'
            '}\n')
        cfile.write(string.Template(template).substitute(translations))


    """
    Emit the message handling implementation
    """
    def emit(self, hfile, cfile):
        if self.can_query:
            utils.add_separator(hfile, 'Message (Query)', self.fullname);
            utils.add_separator(cfile, 'Message (Query)', self.fullname);

            self._emit_request_creator(hfile, cfile, 'query')

            if self.query_input:
                hfile.write('\n/* Query request */')
                self.query_input.emit(hfile, cfile)
            if self.query_output:
                hfile.write('\n/* Query response */')
                self.query_output.emit(hfile, cfile)

        if self.can_set:
            utils.add_separator(hfile, 'Message (Set)', self.fullname);
            utils.add_separator(cfile, 'Message (Set)', self.fullname);

            self._emit_request_creator(hfile, cfile, 'set')

            if self.set_input:
                hfile.write('\n/* Set request */')
                self.set_input.emit(hfile, cfile)
            if self.set_output:
                hfile.write('\n/* Set response */')
                self.set_output.emit(hfile, cfile)

        if self.can_notify:
            utils.add_separator(hfile, 'Message (Notify)', self.fullname);
            utils.add_separator(cfile, 'Message (Notify)', self.fullname);
            if self.notify_output:
                hfile.write('\n/* Indication */')
                self.notify_output.emit(hfile, cfile)


    """
    Emit the sections
    """
    def emit_sections(self, sfile):
        pass
