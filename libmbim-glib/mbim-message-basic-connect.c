/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 * libmbim-glib -- GLib/GIO based library to control MBIM devices
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 * Copyright (C) 2013 Aleksander Morgado <aleksander@gnu.org>
 */

#include "mbim-message-private.h"
#include "mbim-message-basic-connect.h"
#include "mbim-cid.h"

/*****************************************************************************/
/* 'Device Caps' message interface */

MbimMessage *
mbim_message_basic_connect_device_caps_query_request_new (guint32 transaction_id)
{
    return mbim_message_command_new (transaction_id,
                                     MBIM_SERVICE_BASIC_CONNECT,
                                     MBIM_CID_BASIC_CONNECT_DEVICE_CAPS,
                                     MBIM_MESSAGE_COMMAND_TYPE_QUERY);
}

MbimDeviceType
mbim_message_basic_connect_device_caps_query_response_get_device_type (const MbimMessage *self)
{
    return _mbim_message_command_done_read_guint32 (self, 0);
}

MbimCellularClass
mbim_message_basic_connect_device_caps_query_response_get_cellular_class (const MbimMessage *self)
{
    return _mbim_message_command_done_read_guint32 (self, 4);
}

MbimVoiceClass
mbim_message_basic_connect_device_caps_query_response_get_voice_class (const MbimMessage *self)
{
    return _mbim_message_command_done_read_guint32 (self, 8);
}

MbimSimClass
mbim_message_basic_connect_device_caps_query_response_get_sim_class (const MbimMessage *self)
{
    return _mbim_message_command_done_read_guint32 (self, 12);
}

MbimDataClass
mbim_message_basic_connect_device_caps_query_response_get_data_class (const MbimMessage *self)
{
    return _mbim_message_command_done_read_guint32 (self, 16);
}

MbimSmsCaps
mbim_message_basic_connect_device_caps_query_response_get_sms_caps (const MbimMessage *self)
{
    return _mbim_message_command_done_read_guint32 (self, 20);
}

MbimCtrlCaps
mbim_message_basic_connect_device_caps_query_response_get_ctrl_caps (const MbimMessage *self)
{
    return _mbim_message_command_done_read_guint32 (self, 24);
}

guint32
mbim_message_basic_connect_device_caps_query_response_get_max_sessions (const MbimMessage *self)
{
    return _mbim_message_command_done_read_guint32 (self, 28);
}

gchar *
mbim_message_basic_connect_device_caps_query_response_get_custom_data_class (const MbimMessage *self)
{
    return _mbim_message_command_done_read_string (self, 32);
}

gchar *
mbim_message_basic_connect_device_caps_query_response_get_device_id (const MbimMessage *self)
{
    return _mbim_message_command_done_read_string (self, 40);
}

gchar *
mbim_message_basic_connect_device_caps_query_response_get_firmware_info (const MbimMessage *self)
{
    return _mbim_message_command_done_read_string (self, 48);
}

gchar *
mbim_message_basic_connect_device_caps_query_response_get_hardware_info (const MbimMessage *self)
{
    return _mbim_message_command_done_read_string (self, 56);
}
