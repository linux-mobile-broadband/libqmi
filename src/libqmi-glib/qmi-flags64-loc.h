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
 * Copyright (C) 2012 Google Inc.
 * Copyright (C) 2012-2017 Aleksander Morgado <aleksander@aleksander.es>
 */

#ifndef _LIBQMI_GLIB_QMI_FLAGS64_LOC_H_
#define _LIBQMI_GLIB_QMI_FLAGS64_LOC_H_

#if !defined (__LIBQMI_GLIB_H_INSIDE__) && !defined (LIBQMI_GLIB_COMPILATION)
#error "Only <libqmi-glib.h> can be included directly."
#endif

#include <glib.h>

/**
 * QmiLocEventRegistrationFlag:
 * @QMI_LOC_EVENT_REGISTRATION_FLAG_POSITION_REPORT: Position report.
 * @QMI_LOC_EVENT_REGISTRATION_FLAG_GNSS_SATELLITE_INFO: GNSS satellite info.
 * @QMI_LOC_EVENT_REGISTRATION_FLAG_NMEA: NMEA.
 * @QMI_LOC_EVENT_REGISTRATION_FLAG_NI_NOTIFY_VERIFY_REQUEST: NI Notify verify request.
 * @QMI_LOC_EVENT_REGISTRATION_FLAG_INJECT_TIME_REQUEST: Inject time request.
 * @QMI_LOC_EVENT_REGISTRATION_FLAG_INJECT_PREDICTED_ORBITS_REQUEST: Inject predicted orbits request.
 * @QMI_LOC_EVENT_REGISTRATION_FLAG_INJECT_POSITION_REQUEST: Inject position request.
 * @QMI_LOC_EVENT_REGISTRATION_FLAG_ENGINE_STATE: Engine state.
 * @QMI_LOC_EVENT_REGISTRATION_FLAG_FIX_SESSION_STATE: Fix session state.
 * @QMI_LOC_EVENT_REGISTRATION_FLAG_WIFI_REQUEST: WIFI request.
 * @QMI_LOC_EVENT_REGISTRATION_FLAG_SENSOR_STREAMING_READY_STATUS: Sensor streaming ready status.
 * @QMI_LOC_EVENT_REGISTRATION_FLAG_TIME_SYNC_REQUEST: Time sync request.
 * @QMI_LOC_EVENT_REGISTRATION_FLAG_SET_SPI_STREAMING_REPORT: Set SPI streaming report.
 * @QMI_LOC_EVENT_REGISTRATION_FLAG_LOCATION_SERVER_CONNECTION_REQUEST: Location server connection request.
 * @QMI_LOC_EVENT_REGISTRATION_FLAG_NI_GEOFENCE_NOTIFICATION: NI geofence notification.
 * @QMI_LOC_EVENT_REGISTRATION_FLAG_GEOFENCE_GENERAL_ALERT: Geofence general alert.
 * @QMI_LOC_EVENT_REGISTRATION_FLAG_GEOFENCE_BREACH_NOTIFICATION: Geofence breach notification.
 * @QMI_LOC_EVENT_REGISTRATION_FLAG_PEDOMETER_CONTROL: Pedometer control.
 * @QMI_LOC_EVENT_REGISTRATION_FLAG_MOTION_DATA_CONTROL: Motion data control.
 *
 * For which events to register the subscription.
 *
 * Since 1.22
 */
typedef enum { /*< since=1.22 >*/
    QMI_LOC_EVENT_REGISTRATION_FLAG_POSITION_REPORT                    = 1 <<  0,
    QMI_LOC_EVENT_REGISTRATION_FLAG_GNSS_SATELLITE_INFO                = 1 <<  1,
    QMI_LOC_EVENT_REGISTRATION_FLAG_NMEA                               = 1 <<  2,
    QMI_LOC_EVENT_REGISTRATION_FLAG_NI_NOTIFY_VERIFY_REQUEST           = 1 <<  3,
    QMI_LOC_EVENT_REGISTRATION_FLAG_INJECT_TIME_REQUEST                = 1 <<  4,
    QMI_LOC_EVENT_REGISTRATION_FLAG_INJECT_PREDICTED_ORBITS_REQUEST    = 1 <<  5,
    QMI_LOC_EVENT_REGISTRATION_FLAG_INJECT_POSITION_REQUEST            = 1 <<  6,
    QMI_LOC_EVENT_REGISTRATION_FLAG_ENGINE_STATE                       = 1 <<  7,
    QMI_LOC_EVENT_REGISTRATION_FLAG_FIX_SESSION_STATE                  = 1 <<  8,
    QMI_LOC_EVENT_REGISTRATION_FLAG_WIFI_REQUEST                       = 1 <<  9,
    QMI_LOC_EVENT_REGISTRATION_FLAG_SENSOR_STREAMING_READY_STATUS      = 1 << 10,
    QMI_LOC_EVENT_REGISTRATION_FLAG_TIME_SYNC_REQUEST                  = 1 << 11,
    QMI_LOC_EVENT_REGISTRATION_FLAG_SET_SPI_STREAMING_REPORT           = 1 << 12,
    QMI_LOC_EVENT_REGISTRATION_FLAG_LOCATION_SERVER_CONNECTION_REQUEST = 1 << 13,
    QMI_LOC_EVENT_REGISTRATION_FLAG_NI_GEOFENCE_NOTIFICATION           = 1 << 14,
    QMI_LOC_EVENT_REGISTRATION_FLAG_GEOFENCE_GENERAL_ALERT             = 1 << 15,
    QMI_LOC_EVENT_REGISTRATION_FLAG_GEOFENCE_BREACH_NOTIFICATION       = 1 << 16,
    QMI_LOC_EVENT_REGISTRATION_FLAG_PEDOMETER_CONTROL                  = 1 << 17,
    QMI_LOC_EVENT_REGISTRATION_FLAG_MOTION_DATA_CONTROL                = 1 << 18,
} QmiLocEventRegistrationFlag;

/**
 * QmiLocSensorDataUsage:
 * @QMI_LOC_SENSOR_DATA_USAGE_ACCELEROMETER_USED: Accelerometer used.
 * @QMI_LOC_SENSOR_DATA_USAGE_GYRO_USED: Gyro used.
 * @QMI_LOC_SENSOR_DATA_USAGE_AIDED_HEADING: Aided heading.
 * @QMI_LOC_SENSOR_DATA_USAGE_AIDED_SPEED: Aided speed.
 * @QMI_LOC_SENSOR_DATA_USAGE_AIDED_POSITION: Aided position.
 * @QMI_LOC_SENSOR_DATA_USAGE_AIDED_VELOCITY: Aided velocity.
 *
 * Which sensors where used and for which measurements.
 *
 * Since 1.22
 */
typedef enum { /*< since=1.22 >*/
    QMI_LOC_SENSOR_DATA_USAGE_ACCELEROMETER_USED = 1 <<  0,
    QMI_LOC_SENSOR_DATA_USAGE_GYRO_USED          = 1 <<  1,
    QMI_LOC_SENSOR_DATA_USAGE_AIDED_HEADING      = ((guint64) 1) << 32,
    QMI_LOC_SENSOR_DATA_USAGE_AIDED_SPEED        = ((guint64) 1) << 33,
    QMI_LOC_SENSOR_DATA_USAGE_AIDED_POSITION     = ((guint64) 1) << 34,
    QMI_LOC_SENSOR_DATA_USAGE_AIDED_VELOCITY     = ((guint64) 1) << 35,
} QmiLocSensorDataUsage;

/**
 * QmiLocDeleteGnssData:
 * @QMI_LOC_DELETE_GNSS_DATA_GPS_SVDIR: GPS SV dir.
 * @QMI_LOC_DELETE_GNSS_DATA_GPS_SVSTEER: GPS SV steer.
 * @QMI_LOC_DELETE_GNSS_DATA_GPS_TIME: GPS time.
 * @QMI_LOC_DELETE_GNSS_DATA_GPS_ALM_CORR: GPS alm corr.
 * @QMI_LOC_DELETE_GNSS_DATA_GLO_SVDIR: GLONASS SV dir.
 * @QMI_LOC_DELETE_GNSS_DATA_GLO_SVSTEER: GLONASS SV steer.
 * @QMI_LOC_DELETE_GNSS_DATA_GLO_TIME: GLONASS time.
 * @QMI_LOC_DELETE_GNSS_DATA_GLO_ALM_CORR: GLONASS alm corr.
 * @QMI_LOC_DELETE_GNSS_DATA_SBAS_SVDIR: SBAS SV dir.
 * @QMI_LOC_DELETE_GNSS_DATA_SBAS_SVSTEER: SBAS SV steer.
 * @QMI_LOC_DELETE_GNSS_DATA_POSITION: Position.
 * @QMI_LOC_DELETE_GNSS_DATA_TIME: Time.
 * @QMI_LOC_DELETE_GNSS_DATA_IONO: Ionospheric data.
 * @QMI_LOC_DELETE_GNSS_DATA_UTC: UTC time.
 * @QMI_LOC_DELETE_GNSS_DATA_HEALTH: Health information.
 * @QMI_LOC_DELETE_GNSS_DATA_SADATA: SA data.
 * @QMI_LOC_DELETE_GNSS_DATA_RTI: RTI.
 * @QMI_LOC_DELETE_GNSS_DATA_SV_NO_EXIST: SV no exist.
 * @QMI_LOC_DELETE_GNSS_DATA_FREQ_BIAS_EST: Frequency bias estimation.
 *
 * Flags to use when deleting GNSS assistance data.
 *
 * Since 1.22
 */
typedef enum { /*< since=1.22 >*/
    QMI_LOC_DELETE_GNSS_DATA_GPS_SVDIR     =  1 << 0,
    QMI_LOC_DELETE_GNSS_DATA_GPS_SVSTEER   =  1 << 1,
    QMI_LOC_DELETE_GNSS_DATA_GPS_TIME      =  1 << 2,
    QMI_LOC_DELETE_GNSS_DATA_GPS_ALM_CORR  =  1 << 3,
    QMI_LOC_DELETE_GNSS_DATA_GLO_SVDIR     =  1 << 4,
    QMI_LOC_DELETE_GNSS_DATA_GLO_SVSTEER   =  1 << 5,
    QMI_LOC_DELETE_GNSS_DATA_GLO_TIME      =  1 << 6,
    QMI_LOC_DELETE_GNSS_DATA_GLO_ALM_CORR  =  1 << 7,
    QMI_LOC_DELETE_GNSS_DATA_SBAS_SVDIR    =  1 << 8,
    QMI_LOC_DELETE_GNSS_DATA_SBAS_SVSTEER  =  1 << 9,
    QMI_LOC_DELETE_GNSS_DATA_POSITION      =  1 << 10,
    QMI_LOC_DELETE_GNSS_DATA_TIME          =  1 << 11,
    QMI_LOC_DELETE_GNSS_DATA_IONO          =  1 << 12,
    QMI_LOC_DELETE_GNSS_DATA_UTC           =  1 << 13,
    QMI_LOC_DELETE_GNSS_DATA_HEALTH        =  1 << 14,
    QMI_LOC_DELETE_GNSS_DATA_SADATA        =  1 << 15,
    QMI_LOC_DELETE_GNSS_DATA_RTI           =  1 << 16,
    QMI_LOC_DELETE_GNSS_DATA_SV_NO_EXIST   =  1 << 17,
    QMI_LOC_DELETE_GNSS_DATA_FREQ_BIAS_EST =  1 << 18,
} QmiLocDeleteGnssData;

#endif /* _LIBQMI_GLIB_QMI_FLAGS64_LOC_H_ */
