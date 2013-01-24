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

#ifndef _LIBMBIM_GLIB_MBIM_MESSAGE_BASIC_CONNECT_H_
#define _LIBMBIM_GLIB_MBIM_MESSAGE_BASIC_CONNECT_H_

#if !defined (__LIBMBIM_GLIB_H_INSIDE__) && !defined (LIBMBIM_GLIB_COMPILATION)
#error "Only <libmbim-glib.h> can be included directly."
#endif

#include "mbim-enums.h"
#include "mbim-message.h"

G_BEGIN_DECLS

/*****************************************************************************/
/* 'Device Caps' message interface */

/* Query request */
MbimMessage *mbim_message_basic_connect_device_caps_query_request_new (guint32 transaction_id);

/* Query response */
MbimDeviceType     mbim_message_basic_connect_device_caps_query_response_get_device_type       (const MbimMessage *self);
MbimCellularClass  mbim_message_basic_connect_device_caps_query_response_get_cellular_class    (const MbimMessage *self);
MbimVoiceClass     mbim_message_basic_connect_device_caps_query_response_get_voice_class       (const MbimMessage *self);
MbimSimClass       mbim_message_basic_connect_device_caps_query_response_get_sim_class         (const MbimMessage *self);
MbimDataClass      mbim_message_basic_connect_device_caps_query_response_get_data_class        (const MbimMessage *self);
MbimSmsCaps        mbim_message_basic_connect_device_caps_query_response_get_sms_caps          (const MbimMessage *self);
MbimCtrlCaps       mbim_message_basic_connect_device_caps_query_response_get_ctrl_caps         (const MbimMessage *self);
guint32            mbim_message_basic_connect_device_caps_query_response_get_max_sessions      (const MbimMessage *self);
gchar             *mbim_message_basic_connect_device_caps_query_response_get_custom_data_class (const MbimMessage *self);
gchar             *mbim_message_basic_connect_device_caps_query_response_get_device_id         (const MbimMessage *self);
gchar             *mbim_message_basic_connect_device_caps_query_response_get_firmware_info     (const MbimMessage *self);
gchar             *mbim_message_basic_connect_device_caps_query_response_get_hardware_info     (const MbimMessage *self);

/*****************************************************************************/
/* 'Subscriber Ready Status' message interface */

/* Query request */
MbimMessage *mbim_message_basic_connect_subscriber_ready_status_query_request_new (guint32 transaction_id);

/* Query response */
MbimSubscriberReadyState   mbim_message_basic_connect_subscriber_ready_status_query_response_get_ready_state       (const MbimMessage *self);
gchar                     *mbim_message_basic_connect_subscriber_ready_status_query_response_get_subscriber_id     (const MbimMessage *self);
gchar                     *mbim_message_basic_connect_subscriber_ready_status_query_response_get_sim_iccid         (const MbimMessage *self);
MbimReadyInfoFlag          mbim_message_basic_connect_subscriber_ready_status_query_response_get_ready_info        (const MbimMessage *self);
gchar                    **mbim_message_basic_connect_subscriber_ready_status_query_response_get_telephone_numbers (const MbimMessage *self);

G_END_DECLS

#endif /* _LIBMBIM_GLIB_MBIM_MESSAGE_BASIC_CONNECT_H_ */
