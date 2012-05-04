/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 * libqmi-glib -- GLib/GIO based library to control QMI devices
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
 * Copyright (C) 2012 Aleksander Morgado <aleksander@gnu.org>
 */

#ifndef _LIBQMI_GLIB_QMI_WDS_H_
#define _LIBQMI_GLIB_QMI_WDS_H_

#include <glib.h>

G_BEGIN_DECLS

/*****************************************************************************/
/* Supported/known messages */
typedef enum {
  QMI_WDS_MESSAGE_EVENT         = 0x0001, /* unused currently */
  QMI_WDS_MESSAGE_START_NETWORK = 0x0020,
  QMI_WDS_MESSAGE_STOP_NETWORK  = 0x0021,
  QMI_WDS_MESSAGE_PACKET_STATUS = 0x0022, /* unused currently */
} QmiWdsMessage;

/*****************************************************************************/
/* Start network */

typedef struct _QmiWdsStartNetworkInput QmiWdsStartNetworkInput;
QmiWdsStartNetworkInput *qmi_wds_start_network_input_new   (void);
QmiWdsStartNetworkInput *qmi_wds_start_network_input_ref   (QmiWdsStartNetworkInput *input);
void                     qmi_wds_start_network_input_unref (QmiWdsStartNetworkInput *input);
void                     qmi_wds_start_network_input_set_apn      (QmiWdsStartNetworkInput *input,
                                                                   const gchar *str);
const gchar             *qmi_wds_start_network_input_get_apn      (QmiWdsStartNetworkInput *input);
void                     qmi_wds_start_network_input_set_username (QmiWdsStartNetworkInput *input,
                                                                   const gchar *str);
const gchar             *qmi_wds_start_network_input_get_username (QmiWdsStartNetworkInput *input);
void                     qmi_wds_start_network_input_set_password (QmiWdsStartNetworkInput *input,
                                                                   const gchar *str);
const gchar             *qmi_wds_start_network_input_get_password (QmiWdsStartNetworkInput *input);

typedef struct _QmiWdsStartNetworkOutput QmiWdsStartNetworkOutput;
QmiWdsStartNetworkOutput *qmi_wds_start_network_output_ref   (QmiWdsStartNetworkOutput *output);
void                      qmi_wds_start_network_output_unref (QmiWdsStartNetworkOutput *output);
gboolean                  qmi_wds_start_network_output_get_result             (QmiWdsStartNetworkOutput *output,
                                                                               GError **error);
gboolean                  qmi_wds_start_network_output_get_packet_data_handle (QmiWdsStartNetworkOutput *output,
                                                                               guint32 *packet_data_handle);
/* TODO: provide proper enums for the call end reasons */
gboolean                  qmi_wds_start_network_output_get_call_end_reason    (QmiWdsStartNetworkOutput *output,
                                                                               guint16 *call_end_reason);
gboolean                  qmi_wds_start_network_output_get_verbose_call_end_reason (QmiWdsStartNetworkOutput *output,
                                                                                    guint16 *verbose_call_end_reason_domain,
                                                                                    guint16 *verbose_call_end_reason_value);

/*****************************************************************************/
/* Stop network */

typedef struct _QmiWdsStopNetworkInput QmiWdsStopNetworkInput;
QmiWdsStopNetworkInput *qmi_wds_stop_network_input_new   (void);
QmiWdsStopNetworkInput *qmi_wds_stop_network_input_ref   (QmiWdsStopNetworkInput *input);
void                    qmi_wds_stop_network_input_unref (QmiWdsStopNetworkInput *input);
void                    qmi_wds_stop_network_input_set_packet_data_handle (QmiWdsStopNetworkInput *input,
                                                                           guint32 packet_data_handle);
gboolean                qmi_wds_stop_network_input_get_packet_data_handle (QmiWdsStopNetworkInput *input,
                                                                           guint32 *packet_data_handle);

typedef struct _QmiWdsStopNetworkOutput QmiWdsStopNetworkOutput;
QmiWdsStopNetworkOutput *qmi_wds_stop_network_output_ref   (QmiWdsStopNetworkOutput *output);
void                     qmi_wds_stop_network_output_unref (QmiWdsStopNetworkOutput *output);
gboolean                 qmi_wds_stop_network_output_get_result (QmiWdsStopNetworkOutput *output,
                                                                 GError **error);

G_END_DECLS

#endif /* _LIBQMI_GLIB_QMI_WDS_H_ */
