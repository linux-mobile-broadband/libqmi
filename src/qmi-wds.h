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
  QMI_WDS_MESSAGE_GET_PACKET_SERVICE_STATUS  = 0x0022,
  QMI_WDS_MESSAGE_GET_DATA_BEARER_TECHNOLOGY = 0x0037,
  QMI_WDS_MESSAGE_GET_CURRENT_DATA_BEARER_TECHNOLOGY = 0x0044,
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

/*****************************************************************************/
/* Get packet service status */

/* Note: no defined input yet */

typedef enum {
    QMI_WDS_CONNECTION_STATUS_UNKNOWN        = 0,
    QMI_WDS_CONNECTION_STATUS_DISCONNECTED   = 1,
    QMI_WDS_CONNECTION_STATUS_CONNECTED      = 2,
    QMI_WDS_CONNECTION_STATUS_SUSPENDED      = 3,
    QMI_WDS_CONNECTION_STATUS_AUTHENTICATING = 4,
} QmiWdsConnectionStatus;

typedef struct _QmiWdsGetPacketServiceStatusOutput QmiWdsGetPacketServiceStatusOutput;
QmiWdsGetPacketServiceStatusOutput *qmi_wds_get_packet_service_status_output_ref   (QmiWdsGetPacketServiceStatusOutput *output);
void                                qmi_wds_get_packet_service_status_output_unref (QmiWdsGetPacketServiceStatusOutput *output);
gboolean                            qmi_wds_get_packet_service_status_output_get_result (QmiWdsGetPacketServiceStatusOutput *output,
                                                                                         GError **error);
QmiWdsConnectionStatus              qmi_wds_get_packet_service_status_output_get_connection_status (QmiWdsGetPacketServiceStatusOutput *output);

/*****************************************************************************/
/* Get data bearer technology */

/* Note: no defined input yet */

typedef enum {
    QMI_WDS_DATA_BEARER_TECHNOLOGY_UNKNOWN = -1,
    QMI_WDS_DATA_BEARER_TECHNOLOGY_CDMA20001X = 0x01,
    QMI_WDS_DATA_BEARER_TECHNOLOGY_1xEVDO = 0x02,
    QMI_WDS_DATA_BEARER_TECHNOLOGY_GSM = 0x03,
    QMI_WDS_DATA_BEARER_TECHNOLOGY_UMTS = 0x04,
    QMI_WDS_DATA_BEARER_TECHNOLOGY_1xEVDO_REVA = 0x05,
    QMI_WDS_DATA_BEARER_TECHNOLOGY_EDGE = 0x06,
    QMI_WDS_DATA_BEARER_TECHNOLOGY_HSDPA = 0x07,
    QMI_WDS_DATA_BEARER_TECHNOLOGY_HSUPA = 0x08,
    QMI_WDS_DATA_BEARER_TECHNOLOGY_HSDPA_HSUPDA = 0x09,
    QMI_WDS_DATA_BEARER_TECHNOLOGY_LTE = 0x0A,
    QMI_WDS_DATA_BEARER_TECHNOLOGY_EHRPD = 0x0B,
    QMI_WDS_DATA_BEARER_TECHNOLOGY_HSDPAPLUS = 0x0C,
    QMI_WDS_DATA_BEARER_TECHNOLOGY_HSDPAPLUS_HSUPA = 0x0D,
    QMI_WDS_DATA_BEARER_TECHNOLOGY_DCHSDPAPLUS = 0x0E,
    QMI_WDS_DATA_BEARER_TECHNOLOGY_DCHSDPAPLUS_HSUPA = 0x0F,
} QmiWdsDataBearerTechnology;

typedef struct _QmiWdsGetDataBearerTechnologyOutput QmiWdsGetDataBearerTechnologyOutput;
QmiWdsGetDataBearerTechnologyOutput *qmi_wds_get_data_bearer_technology_output_ref   (QmiWdsGetDataBearerTechnologyOutput *output);
void                                 qmi_wds_get_data_bearer_technology_output_unref (QmiWdsGetDataBearerTechnologyOutput *output);
gboolean                             qmi_wds_get_data_bearer_technology_output_get_result (QmiWdsGetDataBearerTechnologyOutput *output,
                                                                                           GError **error);
QmiWdsDataBearerTechnology           qmi_wds_get_data_bearer_technology_output_get_current (QmiWdsGetDataBearerTechnologyOutput *output);
QmiWdsDataBearerTechnology           qmi_wds_get_data_bearer_technology_output_get_last    (QmiWdsGetDataBearerTechnologyOutput *output);

/*****************************************************************************/
/* Get current data bearer technology */

/* Note: no defined input yet */

typedef enum {
    QMI_WDS_NETWORK_TYPE_UNKNOWN = 0,
    QMI_WDS_NETWORK_TYPE_3GPP2   = 1,
    QMI_WDS_NETWORK_TYPE_3GPP    = 2,
} QmiWdsNetworkType;

typedef enum { /*< underscore_name=qmi_wds_rat_3gpp2 >*/
    QMI_WDS_RAT_3GPP2_NONE        = 0,
    QMI_WDS_RAT_3GPP2_CDMA1X      = 1 << 0,
    QMI_WDS_RAT_3GPP2_EVDO_REV0   = 1 << 1,
    QMI_WDS_RAT_3GPP2_EVDO_REVA   = 1 << 2,
    QMI_WDS_RAT_3GPP2_EVDO_REVB   = 1 << 3,
    QMI_WDS_RAT_3GPP2_NULL_BEARER = 1 << 15,
} QmiWdsRat3gpp2;

typedef enum { /*< underscore_name=qmi_wds_rat_3gpp >*/
    QMI_WDS_RAT_3GPP_NONE        = 0,
    QMI_WDS_RAT_3GPP_WCDMA       = 1 << 0,
    QMI_WDS_RAT_3GPP_GPRS        = 1 << 1,
    QMI_WDS_RAT_3GPP_HSDPA       = 1 << 2,
    QMI_WDS_RAT_3GPP_HSUPA       = 1 << 3,
    QMI_WDS_RAT_3GPP_EDGE        = 1 << 4,
    QMI_WDS_RAT_3GPP_LTE         = 1 << 5,
    QMI_WDS_RAT_3GPP_HSDPAPLUS   = 1 << 6,
    QMI_WDS_RAT_3GPP_DCHSDPAPLUS = 1 << 7,
    QMI_WDS_RAT_3GPP_NULL_BEARER = 1 << 15,
} QmiWdsRat3gpp;

typedef enum {
    QMI_WDS_SO_CDMA1X_NONE         = 0,
    QMI_WDS_SO_CDMA1X_IS95         = 1 << 0,
    QMI_WDS_SO_CDMA1X_IS2000       = 1 << 1,
    QMI_WDS_SO_CDMA1X_IS2000_REL_A = 1 << 2,
} QmiWdsSoCdma1x;

typedef enum { /*< underscore_name=qmi_wds_so_evdo_reva >*/
    QMI_WDS_SO_EVDO_REVA_NONE       = 0,
    QMI_WDS_SO_EVDO_REVA_DPA        = 1 << 0,
    QMI_WDS_SO_EVDO_REVA_MFPA       = 1 << 1,
    QMI_WDS_SO_EVDO_REVA_EMPA       = 1 << 2,
    QMI_WDS_SO_EVDO_REVA_EMPA_EHRPD = 1 << 3,
} QmiWdsSoEvdoRevA;

typedef struct _QmiWdsGetCurrentDataBearerTechnologyOutput QmiWdsGetCurrentDataBearerTechnologyOutput;
QmiWdsGetCurrentDataBearerTechnologyOutput *qmi_wds_get_current_data_bearer_technology_output_ref   (QmiWdsGetCurrentDataBearerTechnologyOutput *output);
void                                        qmi_wds_get_current_data_bearer_technology_output_unref (QmiWdsGetCurrentDataBearerTechnologyOutput *output);
gboolean                                    qmi_wds_get_current_data_bearer_technology_output_get_result (QmiWdsGetCurrentDataBearerTechnologyOutput *output,
                                                                                                          GError **error);
QmiWdsNetworkType qmi_wds_get_current_data_bearer_technology_output_get_current_network_type (QmiWdsGetCurrentDataBearerTechnologyOutput *output);
QmiWdsRat3gpp2    qmi_wds_get_current_data_bearer_technology_output_get_current_rat_3gpp2 (QmiWdsGetCurrentDataBearerTechnologyOutput *output);
QmiWdsRat3gpp     qmi_wds_get_current_data_bearer_technology_output_get_current_rat_3gpp (QmiWdsGetCurrentDataBearerTechnologyOutput *output);
QmiWdsSoCdma1x    qmi_wds_get_current_data_bearer_technology_output_get_current_so_cdma1x (QmiWdsGetCurrentDataBearerTechnologyOutput *output);
QmiWdsSoEvdoRevA  qmi_wds_get_current_data_bearer_technology_output_get_current_so_evdo_reva (QmiWdsGetCurrentDataBearerTechnologyOutput *output);
QmiWdsNetworkType qmi_wds_get_current_data_bearer_technology_output_get_last_network_type (QmiWdsGetCurrentDataBearerTechnologyOutput *output);
QmiWdsRat3gpp2    qmi_wds_get_current_data_bearer_technology_output_get_last_rat_3gpp2 (QmiWdsGetCurrentDataBearerTechnologyOutput *output);
QmiWdsRat3gpp     qmi_wds_get_current_data_bearer_technology_output_get_last_rat_3gpp (QmiWdsGetCurrentDataBearerTechnologyOutput *output);
QmiWdsSoCdma1x    qmi_wds_get_current_data_bearer_technology_output_get_last_so_cdma1x (QmiWdsGetCurrentDataBearerTechnologyOutput *output);
QmiWdsSoEvdoRevA  qmi_wds_get_current_data_bearer_technology_output_get_last_so_evdo_reva (QmiWdsGetCurrentDataBearerTechnologyOutput *output);

G_END_DECLS

#endif /* _LIBQMI_GLIB_QMI_WDS_H_ */
