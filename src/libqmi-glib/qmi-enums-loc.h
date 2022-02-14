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

#ifndef _LIBQMI_GLIB_QMI_ENUMS_LOC_H_
#define _LIBQMI_GLIB_QMI_ENUMS_LOC_H_

#if !defined (__LIBQMI_GLIB_H_INSIDE__) && !defined (LIBQMI_GLIB_COMPILATION)
#error "Only <libqmi-glib.h> can be included directly."
#endif

/**
 * SECTION: qmi-enums-loc
 * @title: LOC enumerations and flags
 * @short_description: Enumerations and flags in the LOC service.
 *
 * This section defines enumerations and flags used in the LOC service
 * interface.
 */

/*****************************************************************************/
/* Helper enums for the 'QMI LOC Start' indication */

/**
 * QmiLocIntermediateReportState:
 * @QMI_LOC_INTERMEDIATE_REPORT_STATE_UNKNOWN: Unknown.
 * @QMI_LOC_INTERMEDIATE_REPORT_STATE_ENABLE: Enable intermediate state reporting.
 * @QMI_LOC_INTERMEDIATE_REPORT_STATE_DISABLE: Disable intermediate state reporting.
 *
 * Whether to enable or disable intermediate state reporting.
 *
 * Since: 1.20
 */
typedef enum { /*< since=1.20 >*/
    QMI_LOC_INTERMEDIATE_REPORT_STATE_UNKNOWN = 0,
    QMI_LOC_INTERMEDIATE_REPORT_STATE_ENABLE  = 1,
    QMI_LOC_INTERMEDIATE_REPORT_STATE_DISABLE = 2,
} QmiLocIntermediateReportState;

/**
 * QmiLocIndicationStatus:
 * @QMI_LOC_INDICATION_STATUS_SUCCESS: Success.
 * @QMI_LOC_INDICATION_STATUS_GENERAL_FAILURE: General Failure.
 * @QMI_LOC_INDICATION_STATUS_UNSUPPORTED: Unsupported.
 * @QMI_LOC_INDICATION_STATUS_INVALID_PARAMETER: Invalid Parameter.
 * @QMI_LOC_INDICATION_STATUS_ENGINE_BUSY: Engine is busy.
 * @QMI_LOC_INDICATION_STATUS_PHONE_OFFLINE: Phone is offline.
 * @QMI_LOC_INDICATION_STATUS_TIMEOUT: Timeout.
 *
 * General result of a received indication.
 *
 * Since 1.22
 */
typedef enum { /*< since=1.22 >*/
    QMI_LOC_INDICATION_STATUS_SUCCESS            = 0,
    QMI_LOC_INDICATION_STATUS_GENERAL_FAILURE    = 1,
    QMI_LOC_INDICATION_STATUS_UNSUPPORTED        = 2,
    QMI_LOC_INDICATION_STATUS_INVALID_PARAMETER  = 3,
    QMI_LOC_INDICATION_STATUS_ENGINE_BUSY        = 4,
    QMI_LOC_INDICATION_STATUS_PHONE_OFFLINE      = 5,
    QMI_LOC_INDICATION_STATUS_TIMEOUT            = 6,
} QmiLocIndicationStatus;

/**
 * QmiLocSessionStatus:
 * @QMI_LOC_SESSION_STATUS_SUCCESS: Success
 * @QMI_LOC_SESSION_STATUS_IN_PROGRESS: Operation in Progress.
 * @QMI_LOC_SESSION_STATUS_GENERAL_FAILURE: General Failure.
 * @QMI_LOC_SESSION_STATUS_TIMEOUT: Timeout.
 * @QMI_LOC_SESSION_STATUS_USER_ENDED: User ended.
 * @QMI_LOC_SESSION_STATUS_BAD_PARAMETER: Bad Parameter.
 * @QMI_LOC_SESSION_STATUS_PHONE_OFFLINE: Phone is offline.
 * @QMI_LOC_SESSION_STATUS_ENGINE_LOCKED: Engine is locked.
 *
 * Status of a received session indication.
 *
 * Since 1.22
 */
typedef enum { /*< since=1.22 >*/
    QMI_LOC_SESSION_STATUS_SUCCESS         = 0,
    QMI_LOC_SESSION_STATUS_IN_PROGRESS     = 1,
    QMI_LOC_SESSION_STATUS_GENERAL_FAILURE = 2,
    QMI_LOC_SESSION_STATUS_TIMEOUT         = 3,
    QMI_LOC_SESSION_STATUS_USER_ENDED      = 4,
    QMI_LOC_SESSION_STATUS_BAD_PARAMETER   = 5,
    QMI_LOC_SESSION_STATUS_PHONE_OFFLINE   = 6,
    QMI_LOC_SESSION_STATUS_ENGINE_LOCKED   = 6,
} QmiLocSessionStatus;

/**
 * QmiLocFixRecurrenceType:
 * @QMI_LOC_FIX_RECURRENCE_TYPE_REQUEST_PERIODIC_FIXES: Periodic fixes.
 * @QMI_LOC_FIX_RECURRENCE_TYPE_REQUEST_SINGLE_FIX: Single fix.
 *
 * Whether to request a single or periodic fixes.
 *
 * Since 1.22
 */
typedef enum { /*< since=1.22 >*/
    QMI_LOC_FIX_RECURRENCE_TYPE_REQUEST_PERIODIC_FIXES = 1,
    QMI_LOC_FIX_RECURRENCE_TYPE_REQUEST_SINGLE_FIX     = 2,
} QmiLocFixRecurrenceType;

/**
 * QmiLocOperationMode:
 * @QMI_LOC_OPERATION_MODE_DEFAULT: Default.
 * @QMI_LOC_OPERATION_MODE_MSB: Mobile Station Based (MSB) A-GPS.
 * @QMI_LOC_OPERATION_MODE_MSA: Mobile Station Assisted (MSA) A-GPS.
 * @QMI_LOC_OPERATION_MODE_STANDALONE: Standalone.
 * @QMI_LOC_OPERATION_MODE_CELLID: Cell ID.
 * @QMI_LOC_OPERATION_MODE_WWAN: WWAN.
 *
 * Which operation mode to use.
 *
 * Since 1.22
 */
typedef enum { /*< since=1.22 >*/
    QMI_LOC_OPERATION_MODE_DEFAULT    = 1,
    QMI_LOC_OPERATION_MODE_MSB        = 2,
    QMI_LOC_OPERATION_MODE_MSA        = 3,
    QMI_LOC_OPERATION_MODE_STANDALONE = 4,
    QMI_LOC_OPERATION_MODE_CELLID     = 5,
    QMI_LOC_OPERATION_MODE_WWAN       = 6,
} QmiLocOperationMode;

/**
 * QmiLocEngineState:
 * @QMI_LOC_ENGINE_STATE_ON: On.
 * @QMI_LOC_ENGINE_STATE_OFF: Off.
 *
 * State of the engine.
 *
 * Since 1.22
 */
typedef enum { /*< since=1.22 >*/
    QMI_LOC_ENGINE_STATE_ON  = 1,
    QMI_LOC_ENGINE_STATE_OFF = 2,
} QmiLocEngineState;

/**
 * QmiLocTechnologyUsed:
 * @QMI_LOC_TECHNOLOGY_USED_SATELLITE: Satellite.
 * @QMI_LOC_TECHNOLOGY_USED_CELLULAR: Cellular.
 * @QMI_LOC_TECHNOLOGY_USED_WIFI: Wifi.
 * @QMI_LOC_TECHNOLOGY_USED_SENSORS: Sensors.
 * @QMI_LOC_TECHNOLOGY_USED_REFERENCE_LOCATION: Reference Location.
 * @QMI_LOC_TECHNOLOGY_USED_INJECTED_POSITION: Injected Position.
 * @QMI_LOC_TECHNOLOGY_USED_AFLT: AFLT (Advanced Forward Link Trilateration).
 * @QMI_LOC_TECHNOLOGY_USED_HYBRID: Hybrid.
 *
 * Which technology has been used.
 *
 * Since 1.22
 */
typedef enum { /*< since=1.22 >*/
    QMI_LOC_TECHNOLOGY_USED_SATELLITE          = 1 << 0,
    QMI_LOC_TECHNOLOGY_USED_CELLULAR           = 1 << 1,
    QMI_LOC_TECHNOLOGY_USED_WIFI               = 1 << 2,
    QMI_LOC_TECHNOLOGY_USED_SENSORS            = 1 << 3,
    QMI_LOC_TECHNOLOGY_USED_REFERENCE_LOCATION = 1 << 4,
    QMI_LOC_TECHNOLOGY_USED_INJECTED_POSITION  = 1 << 5,
    QMI_LOC_TECHNOLOGY_USED_AFLT               = 1 << 6,
    QMI_LOC_TECHNOLOGY_USED_HYBRID             = 1 << 7,
} QmiLocTechnologyUsed;

/**
 * QmiLocReliability:
 * @QMI_LOC_RELIABILITY_NOT_SET: Not set.
 * @QMI_LOC_RELIABILITY_VERY_LOW: Very low.
 * @QMI_LOC_RELIABILITY_LOW: Low.
 * @QMI_LOC_RELIABILITY_MEDIUM: Medium.
 * @QMI_LOC_RELIABILITY_HIGH: High.
 *
 * Reliability of the provided information.
 *
 * Since 1.22
 */
typedef enum { /*< since=1.22 >*/
    QMI_LOC_RELIABILITY_NOT_SET  = 0,
    QMI_LOC_RELIABILITY_VERY_LOW = 1,
    QMI_LOC_RELIABILITY_LOW      = 2,
    QMI_LOC_RELIABILITY_MEDIUM   = 3,
    QMI_LOC_RELIABILITY_HIGH     = 4,
} QmiLocReliability;

/**
 * QmiLocTimeSource:
 * @QMI_LOC_TIME_SOURCE_INVALID: Invalid.
 * @QMI_LOC_TIME_SOURCE_NETWORK_TIME_TRANSFER: Time is set by the 1X system.
 * @QMI_LOC_TIME_SOURCE_NETWORK_TIME_TAGGING: Time is set by WCDMA/GSM time tagging.
 * @QMI_LOC_TIME_SOURCE_EXTERNAL_INPUT: Time set by external injection.
 * @QMI_LOC_TIME_SOURCE_TOW_DECODE: Time is set after decoding over-the-air GPS navigation data from one GPS satellite.
 * @QMI_LOC_TIME_SOURCE_TOW_CONFIRMED: Time is set after decoding over-the-air GPS navigation data from multiple satellites.
 * @QMI_LOC_TIME_SOURCE_TOW_AND_WEEK_CONFIRMED: Both time of the week and GPS week number are known.
 * @QMI_LOC_TIME_SOURCE_NAVIGATION_SOLUTION: Time is set by the position engine after the fix is obtained.
 * @QMI_LOC_TIME_SOURCE_SOLVE_FOR_TIME: Time is set by the position engine after performing SFT.
 * @QMI_LOC_TIME_SOURCE_GLO_TOW_DECODE: Time is set after decoding QZSS satellites.
 * @QMI_LOC_TIME_SOURCE_TIME_TRANSFORM: Time is set after transforming the GPS to GLO time.
 * @QMI_LOC_TIME_SOURCE_WCDMA_SLEEP_TIME_TAG: Time is set by the sleep time tag provided by the WCDMA network.
 * @QMI_LOC_TIME_SOURCE_GSM_SLEEP_TIME_TAG: Time is set by the sleep time tag provided by the GSM network.
 * @QMI_LOC_TIME_SOURCE_UNKNOWN: Unknown.
 * @QMI_LOC_TIME_SOURCE_SYSTEM_TIMETICK: Time is derived from the system clock.
 * @QMI_LOC_TIME_SOURCE_QZSS_TOW_DECODE: Time set after decoding QZSS satellites.
 * @QMI_LOC_TIME_SOURCE_BDS_TOW_DECODE: Time set after decoding BDS satellites.
 *
 * Source of the used time.
 *
 * Since 1.22
 */
typedef enum { /*< since=1.22 >*/
    QMI_LOC_TIME_SOURCE_INVALID                =  0,
    QMI_LOC_TIME_SOURCE_NETWORK_TIME_TRANSFER  =  1,
    QMI_LOC_TIME_SOURCE_NETWORK_TIME_TAGGING   =  2,
    QMI_LOC_TIME_SOURCE_EXTERNAL_INPUT         =  3,
    QMI_LOC_TIME_SOURCE_TOW_DECODE             =  4,
    QMI_LOC_TIME_SOURCE_TOW_CONFIRMED          =  5,
    QMI_LOC_TIME_SOURCE_TOW_AND_WEEK_CONFIRMED =  6,
    QMI_LOC_TIME_SOURCE_NAVIGATION_SOLUTION    =  7,
    QMI_LOC_TIME_SOURCE_SOLVE_FOR_TIME         =  8,
    QMI_LOC_TIME_SOURCE_GLO_TOW_DECODE         =  9,
    QMI_LOC_TIME_SOURCE_TIME_TRANSFORM         = 10,
    QMI_LOC_TIME_SOURCE_WCDMA_SLEEP_TIME_TAG   = 11,
    QMI_LOC_TIME_SOURCE_GSM_SLEEP_TIME_TAG     = 12,
    QMI_LOC_TIME_SOURCE_UNKNOWN                = 13,
    QMI_LOC_TIME_SOURCE_SYSTEM_TIMETICK        = 14,
    QMI_LOC_TIME_SOURCE_QZSS_TOW_DECODE        = 15,
    QMI_LOC_TIME_SOURCE_BDS_TOW_DECODE         = 16,
} QmiLocTimeSource;

/**
 * QmiLocSatelliteValidInformation:
 * @QMI_LOC_SATELLITE_VALID_INFORMATION_SYSTEM: System,
 * @QMI_LOC_SATELLITE_VALID_INFORMATION_GNSS_SATELLITE_ID: GNSS Satellite ID.
 * @QMI_LOC_SATELLITE_VALID_INFORMATION_HEALTH_STATUS: Health status.
 * @QMI_LOC_SATELLITE_VALID_INFORMATION_PROCESS_STATUS: Process status.
 * @QMI_LOC_SATELLITE_VALID_INFORMATION_SATELLITE_INFO_MASK: Satellite info mask.
 * @QMI_LOC_SATELLITE_VALID_INFORMATION_ELEVATION: Elevation.
 * @QMI_LOC_SATELLITE_VALID_INFORMATION_AZIMUTH: Azimuth.
 * @QMI_LOC_SATELLITE_VALID_INFORMATION_SIGNAL_TO_NOISE_RATIO: Signal to noise ratio.
 *
 * Which information about the satellite is valid.
 *
 * Since 1.22
 */
typedef enum { /*< since=1.22 >*/
    QMI_LOC_SATELLITE_VALID_INFORMATION_SYSTEM                = 1 << 0,
    QMI_LOC_SATELLITE_VALID_INFORMATION_GNSS_SATELLITE_ID     = 1 << 1,
    QMI_LOC_SATELLITE_VALID_INFORMATION_HEALTH_STATUS         = 1 << 2,
    QMI_LOC_SATELLITE_VALID_INFORMATION_PROCESS_STATUS        = 1 << 3,
    QMI_LOC_SATELLITE_VALID_INFORMATION_SATELLITE_INFO_MASK   = 1 << 4,
    QMI_LOC_SATELLITE_VALID_INFORMATION_ELEVATION             = 1 << 5,
    QMI_LOC_SATELLITE_VALID_INFORMATION_AZIMUTH               = 1 << 6,
    QMI_LOC_SATELLITE_VALID_INFORMATION_SIGNAL_TO_NOISE_RATIO = 1 << 7,
} QmiLocSatelliteValidInformation;

/**
 * QmiLocSystem:
 * @QMI_LOC_SYSTEM_GPS: GPS.
 * @QMI_LOC_SYSTEM_GALILEO: Galileo.
 * @QMI_LOC_SYSTEM_SBAS: SBAS.
 * @QMI_LOC_SYSTEM_COMPASS: COMPASS (BeiDou).
 * @QMI_LOC_SYSTEM_GLONASS: GLONASS.
 *
 * Which location system a satellite is part of.
 *
 * Since 1.22
 */
typedef enum { /*< since=1.22 >*/
    QMI_LOC_SYSTEM_GPS     = 1,
    QMI_LOC_SYSTEM_GALILEO = 2,
    QMI_LOC_SYSTEM_SBAS    = 3,
    QMI_LOC_SYSTEM_COMPASS = 4,
    QMI_LOC_SYSTEM_GLONASS = 5,
} QmiLocSystem;

/**
 * QmiLocHealthStatus:
 * @QMI_LOC_HEALTH_STATUS_UNHEALTHY: Unhealthy.
 * @QMI_LOC_HEALTH_STATUS_HEALTHY: Healthy.
 *
 * Whether a satellite is healthy or unhealthy.
 *
 * Since 1.22
 */
typedef enum { /*< since=1.22 >*/
    QMI_LOC_HEALTH_STATUS_UNHEALTHY = 0,
    QMI_LOC_HEALTH_STATUS_HEALTHY   = 1,
} QmiLocHealthStatus;

/**
 * QmiLocNavigationData:
 * @QMI_LOC_NAVIGATION_DATA_HAS_EPHEMERIS: Has ephemeris.
 * @QMI_LOC_NAVIGATION_DATA_HAS_ALMANAC: Has almanac.
 *
 * Whether the ephemeris or almanac exist for a satellite.
 *
 * Since 1.22
 */
typedef enum { /*< since=1.22 >*/
    QMI_LOC_NAVIGATION_DATA_HAS_EPHEMERIS = 0,
    QMI_LOC_NAVIGATION_DATA_HAS_ALMANAC   = 1,
} QmiLocNavigationData;

/**
 * QmiLocSatelliteStatus:
 * @QMI_LOC_SATELLITE_STATUS_IDLE: Idle.
 * @QMI_LOC_SATELLITE_STATUS_SEARCHING: Searching.
 * @QMI_LOC_SATELLITE_STATUS_TRACKING: Tracking.
 *
 * Whether the engine searches or tracks a satellite.
 *
 * Since 1.22
 */
typedef enum { /*< since=1.22 >*/
    QMI_LOC_SATELLITE_STATUS_IDLE      = 0,
    QMI_LOC_SATELLITE_STATUS_SEARCHING = 1,
    QMI_LOC_SATELLITE_STATUS_TRACKING  = 2,
} QmiLocSatelliteStatus;

/**
 * QmiLocServerType:
 * @QMI_LOC_SERVER_TYPE_UNKNOWN: Unknown.
 * @QMI_LOC_SERVER_TYPE_CDMA_PDE: CDMA Position Determining System.
 * @QMI_LOC_SERVER_TYPE_CDMA_MPC: CDMA Mobile Positioning Center.
 * @QMI_LOC_SERVER_TYPE_UMTS_SLP: SUPL Location Platform.
 * @QMI_LOC_SERVER_TYPE_CUSTOM_PDE: Custom position determining system.
 *
 * Server type for Assisted-GPS.
 *
 * Since: 1.22
 */
typedef enum { /*< since=1.22 >*/
    QMI_LOC_SERVER_TYPE_UNKNOWN    = 0,
    QMI_LOC_SERVER_TYPE_CDMA_PDE   = 1,
    QMI_LOC_SERVER_TYPE_CDMA_MPC   = 2,
    QMI_LOC_SERVER_TYPE_UMTS_SLP   = 3,
    QMI_LOC_SERVER_TYPE_CUSTOM_PDE = 4,
} QmiLocServerType;

/**
 * QmiLocServerAddressType:
 * @QMI_LOC_SERVER_ADDRESS_TYPE_NONE: None.
 * @QMI_LOC_SERVER_ADDRESS_TYPE_IPV4: IPv4 server address.
 * @QMI_LOC_SERVER_ADDRESS_TYPE_IPV6: IPv6 server address.
 * @QMI_LOC_SERVER_ADDRESS_TYPE_URL: URL server address.
 *
 * Type of server address for Assisted-GPS.
 *
 * Since: 1.22
 */
typedef enum { /*< since=1.22 >*/
    QMI_LOC_SERVER_ADDRESS_TYPE_NONE = 0,
    QMI_LOC_SERVER_ADDRESS_TYPE_IPV4 = 1 << 0,
    QMI_LOC_SERVER_ADDRESS_TYPE_IPV6 = 1 << 1,
    QMI_LOC_SERVER_ADDRESS_TYPE_URL  = 1 << 2,
} QmiLocServerAddressType;

/**
 * QmiLocPredictedOrbitsDataFormat:
 * @QMI_LOC_PREDICTED_ORBITS_DATA_FORMAT_XTRA: Qualcomm XTRA database.
 *
 * Type of predicted orbits data.
 *
 * Since: 1.22
 */
typedef enum { /*< since=1.22 >*/
    QMI_LOC_PREDICTED_ORBITS_DATA_FORMAT_XTRA = 0
} QmiLocPredictedOrbitsDataFormat;

/**
 * QmiLocDeleteSvInfo:
 * @QMI_LOC_DELETE_SV_INFO_EPHEMERIS: Ephemeris.
 * @QMI_LOC_DELETE_SV_INFO_ALMANAC: Almanac.
 *
 * Type of data to delete for a given satellite.
 *
 * Since: 1.22
 */
typedef enum { /*< since=1.22 >*/
    QMI_LOC_DELETE_SV_INFO_EPHEMERIS = 1 << 0,
    QMI_LOC_DELETE_SV_INFO_ALMANAC   = 1 << 1,
} QmiLocDeleteSvInfo;

/**
 * QmiLocDeleteCellDatabase:
 * @QMI_LOC_DELETE_CELL_DATABASE_POS: Position.
 * @QMI_LOC_DELETE_CELL_DATABASE_LATEST_GPS_POS: Latest GPS position.
 * @QMI_LOC_DELETE_CELL_DATABASE_OTA_POS: OTA position.
 * @QMI_LOC_DELETE_CELL_DATABASE_EXT_REF_POS: External reference position.
 * @QMI_LOC_DELETE_CELL_DATABASE_TIMETAG: Timetag.
 * @QMI_LOC_DELETE_CELL_DATABASE_CELLID: Cell ID.
 * @QMI_LOC_DELETE_CELL_DATABASE_CACHED_CELLID: Cached cell ID.
 * @QMI_LOC_DELETE_CELL_DATABASE_LAST_SRV_CELL: Last serving cell.
 * @QMI_LOC_DELETE_CELL_DATABASE_CUR_SRV_CELL: Current servince cell.
 * @QMI_LOC_DELETE_CELL_DATABASE_NEIGHBOR_INFO: Neighboring cell info.
 *
 * Type of data to delete from the cell database.
 *
 * Since: 1.22
 */
typedef enum { /*< since=1.22 >*/
    QMI_LOC_DELETE_CELL_DATABASE_POS            = 1 << 0,
    QMI_LOC_DELETE_CELL_DATABASE_LATEST_GPS_POS = 1 << 1,
    QMI_LOC_DELETE_CELL_DATABASE_OTA_POS        = 1 << 2,
    QMI_LOC_DELETE_CELL_DATABASE_EXT_REF_POS    = 1 << 3,
    QMI_LOC_DELETE_CELL_DATABASE_TIMETAG        = 1 << 4,
    QMI_LOC_DELETE_CELL_DATABASE_CELLID         = 1 << 5,
    QMI_LOC_DELETE_CELL_DATABASE_CACHED_CELLID  = 1 << 6,
    QMI_LOC_DELETE_CELL_DATABASE_LAST_SRV_CELL  = 1 << 7,
    QMI_LOC_DELETE_CELL_DATABASE_CUR_SRV_CELL   = 1 << 8,
    QMI_LOC_DELETE_CELL_DATABASE_NEIGHBOR_INFO  = 1 << 9,
} QmiLocDeleteCellDatabase;

/**
 * QmiLocDeleteClockInfo:
 * @QMI_LOC_DELETE_CLOCK_INFO_TIME_EST: Time estimation.
 * @QMI_LOC_DELETE_CLOCK_INFO_FREQ_EST: Frequency estimation.
 * @QMI_LOC_DELETE_CLOCK_INFO_WEEK_NUMBER: Week number.
 * @QMI_LOC_DELETE_CLOCK_INFO_RTC_TIME: RTC time.
 * @QMI_LOC_DELETE_CLOCK_INFO_TIME_TRANSFER: Time transfer.
 * @QMI_LOC_DELETE_CLOCK_INFO_GPS_TIME_EST: GPS time estimation.
 * @QMI_LOC_DELETE_CLOCK_INFO_GLO_TIME_EST: GLONASS time estimation.
 * @QMI_LOC_DELETE_CLOCK_INFO_GLO_DAY_NUMBER: GLONASS day number.
 * @QMI_LOC_DELETE_CLOCK_INFO_GLO_YEAR_NUMBER: GLONASS year number.
 * @QMI_LOC_DELETE_CLOCK_INFO_GLO_RF_GRP_DELAY: GLONASS RF group delay.
 * @QMI_LOC_DELETE_CLOCK_INFO_DISABLE_TT: Disable TT.
 *
 * Type of data to delete from the clock information.
 *
 * Since: 1.22
 */
typedef enum { /*< since=1.22 >*/
    QMI_LOC_DELETE_CLOCK_INFO_TIME_EST         = 1 << 0,
    QMI_LOC_DELETE_CLOCK_INFO_FREQ_EST         = 1 << 1,
    QMI_LOC_DELETE_CLOCK_INFO_WEEK_NUMBER      = 1 << 2,
    QMI_LOC_DELETE_CLOCK_INFO_RTC_TIME         = 1 << 3,
    QMI_LOC_DELETE_CLOCK_INFO_TIME_TRANSFER    = 1 << 4,
    QMI_LOC_DELETE_CLOCK_INFO_GPS_TIME_EST     = 1 << 5,
    QMI_LOC_DELETE_CLOCK_INFO_GLO_TIME_EST     = 1 << 6,
    QMI_LOC_DELETE_CLOCK_INFO_GLO_DAY_NUMBER   = 1 << 7,
    QMI_LOC_DELETE_CLOCK_INFO_GLO_YEAR_NUMBER  = 1 << 8,
    QMI_LOC_DELETE_CLOCK_INFO_GLO_RF_GRP_DELAY = 1 << 9,
    QMI_LOC_DELETE_CLOCK_INFO_DISABLE_TT       = 1 << 10,
} QmiLocDeleteClockInfo;

/**
 * QmiLocNmeaType:
 * @QMI_LOC_NMEA_TYPE_GGA: Enable GGA traces.
 * @QMI_LOC_NMEA_TYPE_RMC: Enable RMC traces.
 * @QMI_LOC_NMEA_TYPE_GSV: Enable GSV traces.
 * @QMI_LOC_NMEA_TYPE_GSA: Enable GSA traces.
 * @QMI_LOC_NMEA_TYPE_VTG: Enable VTG traces.
 * @QMI_LOC_NMEA_TYPE_PQXFI: Enable PQXFI traces.
 * @QMI_LOC_NMEA_TYPE_PSTIS: Enable PSTIS traces.
 * @QMI_LOC_NMEA_TYPE_ALL: Enable all supported trace types.
 *
 * NMEA trace types that may be enabled in the GNSS engine.
 *
 * Since: 1.26
 */
typedef enum { /*< since=1.26 >*/
    QMI_LOC_NMEA_TYPE_GGA   = 1 << 0,
    QMI_LOC_NMEA_TYPE_RMC   = 1 << 1,
    QMI_LOC_NMEA_TYPE_GSV   = 1 << 2,
    QMI_LOC_NMEA_TYPE_GSA   = 1 << 3,
    QMI_LOC_NMEA_TYPE_VTG   = 1 << 4,
    QMI_LOC_NMEA_TYPE_PQXFI = 1 << 5,
    QMI_LOC_NMEA_TYPE_PSTIS = 1 << 6,
    QMI_LOC_NMEA_TYPE_ALL   = 0x0000FFFF,
} QmiLocNmeaType;

/**
 * QmiLocLockType:
 * @QMI_LOC_LOCK_TYPE_NONE: Do not lock any position sessions.
 * @QMI_LOC_LOCK_TYPE_MI: Lock mobile-initiated position sessions.
 * @QMI_LOC_LOCK_TYPE_MT: Lock mobile-terminated position sessions.
 * @QMI_LOC_LOCK_TYPE_ALL: Lock all position sessions.
 *
 * Location engine lock type.
 *
 * Since: 1.28
 */
typedef enum { /*< since=1.28 >*/
    QMI_LOC_LOCK_TYPE_NONE = 1,
    QMI_LOC_LOCK_TYPE_MI   = 2,
    QMI_LOC_LOCK_TYPE_MT   = 3,
    QMI_LOC_LOCK_TYPE_ALL  = 4,
} QmiLocLockType;

#endif /* _LIBQMI_GLIB_QMI_ENUMS_LOC_H_ */
