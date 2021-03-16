/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * qmicli -- Command line interface to control QMI devices
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright (C) 2018 Thomas Weißschuh <thomas@weissschuh.net>
 * Copyright (C) 2018 Aleksander Morgado <aleksander@aleksander.es>
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <string.h>

#include <glib.h>
#include <gio/gio.h>

#include <libqmi-glib.h>

#include "qmicli.h"
#include "qmicli-helpers.h"

#if defined HAVE_QMI_SERVICE_LOC

#undef VALIDATE_MASK_NONE
#define VALIDATE_MASK_NONE(str) (str ? str : "none")

/* Context */

typedef enum {
    MONITORING_STEP_FIRST,
    MONITORING_STEP_REGISTER_EVENTS,
    MONITORING_STEP_SETUP_TIMEOUT,
    MONITORING_STEP_ONGOING,
} MonitoringStep;

typedef struct {
    QmiDevice      *device;
    QmiClientLoc   *client;
    GCancellable   *cancellable;
    guint           timeout_id;
    MonitoringStep  monitoring_step;
    guint           position_report_indication_id;
    guint           nmea_indication_id;
    guint           gnss_sv_info_indication_id;
    guint           delete_assistance_data_indication_id;
    guint           get_nmea_types_indication_id;
    guint           set_nmea_types_indication_id;
    guint           get_operation_mode_indication_id;
    guint           set_operation_mode_indication_id;
    guint           get_engine_lock_indication_id;
    guint           set_engine_lock_indication_id;
} Context;
static Context *ctx;

/* Options */
static gint     session_id;
static gboolean start_flag;
static gboolean stop_flag;
static gboolean get_position_report_flag;
static gboolean get_gnss_sv_info_flag;
static gint     timeout;
static gboolean follow_position_report_flag;
static gboolean follow_gnss_sv_info_flag;
static gboolean follow_nmea_flag;
static gboolean delete_assistance_data_flag;
static gboolean get_nmea_types_flag;
static gchar   *set_nmea_types_str;
static gboolean get_operation_mode_flag;
static gchar   *set_operation_mode_str;
static gboolean get_engine_lock_flag;
static gchar   *set_engine_lock_str;
static gboolean noop_flag;

#define DEFAULT_LOC_TIMEOUT_SECS 30

static GOptionEntry entries[] = {
#if defined HAVE_QMI_MESSAGE_LOC_START || defined HAVE_QMI_MESSAGE_LOC_STOP
    {
        "loc-session-id", 0, 0, G_OPTION_ARG_INT, &session_id,
        "Session ID for the LOC session",
        "[ID]",
    },
#endif
#if defined HAVE_QMI_MESSAGE_LOC_START
    {
        "loc-start", 0, 0, G_OPTION_ARG_NONE, &start_flag,
        "Start location gathering",
        NULL,
    },
#endif
#if defined HAVE_QMI_MESSAGE_LOC_STOP
    {
        "loc-stop", 0, 0, G_OPTION_ARG_NONE, &stop_flag,
        "Stop location gathering",
        NULL,
    },
#endif
#if defined HAVE_QMI_INDICATION_LOC_POSITION_REPORT && defined HAVE_QMI_MESSAGE_LOC_REGISTER_EVENTS
    {
        "loc-get-position-report", 0, 0, G_OPTION_ARG_NONE, &get_position_report_flag,
        "Get position reported by the location module",
        NULL,
    },
#endif
#if defined HAVE_QMI_INDICATION_LOC_GNSS_SV_INFO && defined HAVE_QMI_MESSAGE_LOC_REGISTER_EVENTS
    {
        "loc-get-gnss-sv-info", 0, 0, G_OPTION_ARG_NONE, &get_gnss_sv_info_flag,
        "Show GNSS space vehicle info",
        NULL,
    },
#endif
#if (defined HAVE_QMI_INDICATION_LOC_POSITION_REPORT || defined HAVE_QMI_INDICATION_LOC_GNSS_SV_INFO) && \
    defined HAVE_QMI_MESSAGE_LOC_REGISTER_EVENTS
    {
        "loc-timeout", 0, 0, G_OPTION_ARG_INT, &timeout,
        "Maximum time to wait for information in `--loc-get-position-report' and `--loc-get-gnss-sv-info' (default 30s)",
        "[SECS]",
    },
#endif
#if defined HAVE_QMI_INDICATION_LOC_POSITION_REPORT && defined HAVE_QMI_MESSAGE_LOC_REGISTER_EVENTS
    {
        "loc-follow-position-report", 0, 0, G_OPTION_ARG_NONE, &follow_position_report_flag,
        "Follow all position updates reported by the location module indefinitely",
        NULL,
    },
#endif
#if defined HAVE_QMI_INDICATION_LOC_GNSS_SV_INFO && defined HAVE_QMI_MESSAGE_LOC_REGISTER_EVENTS
    {
        "loc-follow-gnss-sv-info", 0, 0, G_OPTION_ARG_NONE, &follow_gnss_sv_info_flag,
        "Follow all GNSS space vehicle info updates reported by the location module indefinitely",
        NULL,
    },
#endif
#if defined HAVE_QMI_INDICATION_LOC_NMEA && defined HAVE_QMI_MESSAGE_LOC_REGISTER_EVENTS
    {
        "loc-follow-nmea", 0, 0, G_OPTION_ARG_NONE, &follow_nmea_flag,
        "Follow all NMEA trace updates reported by the location module indefinitely",
        NULL,
    },
#endif
#if defined HAVE_QMI_MESSAGE_LOC_DELETE_ASSISTANCE_DATA
    {
        "loc-delete-assistance-data", 0, 0, G_OPTION_ARG_NONE, &delete_assistance_data_flag,
        "Delete positioning assistance data",
        NULL,
    },
#endif
#if defined HAVE_QMI_MESSAGE_LOC_GET_NMEA_TYPES
    { "loc-get-nmea-types", 0, 0, G_OPTION_ARG_NONE, &get_nmea_types_flag,
      "Get list of enabled NMEA traces",
      NULL
    },
#endif
#if defined HAVE_QMI_MESSAGE_LOC_SET_NMEA_TYPES
    { "loc-set-nmea-types", 0, 0, G_OPTION_ARG_STRING, &set_nmea_types_str,
      "Set list of enabled NMEA traces",
      "[type1|type2|type3...]"
    },
#endif
#if defined HAVE_QMI_MESSAGE_LOC_GET_OPERATION_MODE
    { "loc-get-operation-mode", 0, 0, G_OPTION_ARG_NONE, &get_operation_mode_flag,
      "Get operation mode",
      NULL
    },
#endif
#if defined HAVE_QMI_MESSAGE_LOC_SET_OPERATION_MODE
    { "loc-set-operation-mode", 0, 0, G_OPTION_ARG_STRING, &set_operation_mode_str,
      "Set operation mode",
      "[default|msb|msa|standalone|cellid|wwan]"
    },
#endif
#if defined HAVE_QMI_MESSAGE_LOC_GET_ENGINE_LOCK
    { "loc-get-engine-lock", 0, 0, G_OPTION_ARG_NONE, &get_engine_lock_flag,
      "Get engine lock status",
      NULL
    },
#endif
#if defined HAVE_QMI_MESSAGE_LOC_SET_ENGINE_LOCK
    { "loc-set-engine-lock", 0, 0, G_OPTION_ARG_STRING, &set_engine_lock_str,
      "Set engine lock status",
      "[none|mi|mt|all]"
    },
#endif
    { "loc-noop", 0, 0, G_OPTION_ARG_NONE, &noop_flag,
      "Just allocate or release a LOC client. Use with `--client-no-release-cid' and/or `--client-cid'",
      NULL
    },
    { NULL }
};

GOptionGroup *
qmicli_loc_get_option_group (void)
{
    GOptionGroup *group;

    group = g_option_group_new ("loc",
                                "LOC options:",
                                "Show location options", NULL, NULL);
    g_option_group_add_entries (group, entries);

    return group;
}

gboolean
qmicli_loc_options_enabled (void)
{
    static guint n_actions = 0;
    static gboolean checked = FALSE;
    gboolean follow_action;

    if (checked)
        return !!n_actions;

    /* Let's define the following actions:
     *  - Start location engine
     *  - Stop location engine
     *  - Show current position (oneshot).
     *  - Show current satellite info (oneshot).
     *  - Follow updates indefinitely, including either position, satellite info or NMEA traces.
     *  - Other single-request operations.
     */
    follow_action = !!(follow_position_report_flag + follow_gnss_sv_info_flag + follow_nmea_flag);
    n_actions = (start_flag +
                 stop_flag +
                 get_position_report_flag +
                 get_gnss_sv_info_flag +
                 follow_action +
                 delete_assistance_data_flag +
                 get_nmea_types_flag +
                 !!set_nmea_types_str +
                 get_operation_mode_flag +
                 !!set_operation_mode_str +
                 get_engine_lock_flag +
                 !!set_engine_lock_str +
                 noop_flag);

    if (n_actions > 1) {
        g_printerr ("error: too many LOC actions requested\n");
        exit (EXIT_FAILURE);
    }

    if (session_id < 0 || session_id > G_MAXUINT8) {
        g_printerr ("error: invalid session ID: %d [0,%u]\n", session_id, G_MAXUINT8);
        exit (EXIT_FAILURE);
    }

    if (timeout < 0) {
        g_printerr ("error: invalid timeout: %d", timeout);
        exit (EXIT_FAILURE);
    }

    if (timeout > 0 && !(get_position_report_flag || get_gnss_sv_info_flag)) {
        g_printerr ("error: `--loc-timeout' is only applicable with `--loc-get-position-report' or `--loc-get-gnss-sv-info'\n");
        exit (EXIT_FAILURE);
    }

    /* Actions that require receiving QMI indication messages must specify that
     * indications are expected. */
    if (get_position_report_flag ||
        get_gnss_sv_info_flag ||
        follow_action ||
        delete_assistance_data_flag ||
        get_nmea_types_flag ||
        set_nmea_types_str ||
        get_operation_mode_flag ||
        set_operation_mode_str ||
        get_engine_lock_flag ||
        set_engine_lock_str)
        qmicli_expect_indications ();

    checked = TRUE;
    return !!n_actions;
}

static void
context_free (Context *context)
{
    if (!context)
        return;

    if (context->timeout_id)
        g_source_remove (context->timeout_id);

    if (context->position_report_indication_id)
        g_signal_handler_disconnect (context->client, context->position_report_indication_id);

    if (context->gnss_sv_info_indication_id)
        g_signal_handler_disconnect (context->client, context->gnss_sv_info_indication_id);

    if (context->nmea_indication_id)
        g_signal_handler_disconnect (context->client, context->nmea_indication_id);

    if (context->delete_assistance_data_indication_id)
        g_signal_handler_disconnect (context->client, context->delete_assistance_data_indication_id);

    if (context->get_nmea_types_indication_id)
        g_signal_handler_disconnect (context->client, context->get_nmea_types_indication_id);

    if (context->set_nmea_types_indication_id)
        g_signal_handler_disconnect (context->client, context->set_nmea_types_indication_id);

    if (context->get_operation_mode_indication_id)
        g_signal_handler_disconnect (context->client, context->get_operation_mode_indication_id);

    if (context->set_operation_mode_indication_id)
        g_signal_handler_disconnect (context->client, context->set_operation_mode_indication_id);

    if (context->get_engine_lock_indication_id)
        g_signal_handler_disconnect (context->client, context->get_engine_lock_indication_id);

    if (context->set_engine_lock_indication_id)
        g_signal_handler_disconnect (context->client, context->set_engine_lock_indication_id);

    g_clear_object (&context->cancellable);
    g_clear_object (&context->client);
    g_clear_object (&context->device);
    g_slice_free (Context, context);
}

static void
operation_shutdown (gboolean operation_status)
{
    context_free (ctx);
    qmicli_async_operation_done (operation_status, FALSE);
}

#if (defined HAVE_QMI_INDICATION_LOC_POSITION_REPORT || \
     defined HAVE_QMI_INDICATION_LOC_GNSS_SV_INFO ||    \
     defined HAVE_QMI_INDICATION_LOC_NMEA) &&           \
    defined HAVE_QMI_MESSAGE_LOC_REGISTER_EVENTS

static void monitoring_step_run (void);

static gboolean
monitoring_timed_out (void)
{
    ctx->timeout_id = 0;
    g_printerr ("error: operation failed: timeout\n");
    operation_shutdown (FALSE);
    return G_SOURCE_REMOVE;
}

static void
monitoring_cancelled (GCancellable *cancellable)
{
    /* For GET operations, this is a failure */
    if (get_position_report_flag || get_gnss_sv_info_flag) {
        g_printerr ("error: operation failed: cancelled\n");
        operation_shutdown (FALSE);
        return;
    }

    /* For FOLLOW operations, silently exit */
    if (follow_position_report_flag || follow_gnss_sv_info_flag || follow_nmea_flag) {
        operation_shutdown (TRUE);
        return;
    }

    g_assert_not_reached ();
}

#endif /* HAVE_QMI_INDICATION_LOC_POSITION_REPORT
        * HAVE_QMI_INDICATION_LOC_GNSS_SV_INFO
        * HAVE_QMI_INDICATION_LOC_NMEA */

#if defined HAVE_QMI_INDICATION_LOC_NMEA && defined HAVE_QMI_MESSAGE_LOC_REGISTER_EVENTS

static void
nmea_received (QmiClientLoc               *client,
               QmiIndicationLocNmeaOutput *output)
{
    const gchar *nmea = NULL;

    qmi_indication_loc_nmea_output_get_nmea_string (output, &nmea, NULL);
    if (nmea)
        /* Note: NMEA traces already have an EOL */
        g_print ("%s", nmea);
}

#endif /* HAVE_QMI_INDICATION_LOC_NMEA */

#if defined HAVE_QMI_INDICATION_LOC_GNSS_SV_INFO && defined HAVE_QMI_MESSAGE_LOC_REGISTER_EVENTS

static void
gnss_sv_info_received (QmiClientLoc                     *client,
                       QmiIndicationLocGnssSvInfoOutput *output)
{
    GArray   *satellite_infos = NULL;
    guint     i, num_satellite_infos;
    gboolean  altitude_assumed;

    if (qmi_indication_loc_gnss_sv_info_output_get_altitude_assumed (output, &altitude_assumed, NULL))
        g_print ("[gnss sv info] Altitude assumed: %s\n", altitude_assumed ? "yes" : "no");
    else
        g_print ("[gnss sv info] Altitude assumed: n/a\n");

    qmi_indication_loc_gnss_sv_info_output_get_list (output, &satellite_infos, NULL);

    num_satellite_infos = satellite_infos ? satellite_infos->len : 0;
    g_print ("[gnss sv info] %d satellites detected:\n", num_satellite_infos);
    for (i = 0; i < num_satellite_infos; i++) {
        QmiIndicationLocGnssSvInfoOutputListElement *element;

        element = &g_array_index (satellite_infos, QmiIndicationLocGnssSvInfoOutputListElement, i);
        g_print ("   [satellite #%u]\n", i);
        g_print ("      system:           %s\n", (element->valid_information & QMI_LOC_SATELLITE_VALID_INFORMATION_SYSTEM) ? qmi_loc_system_get_string (element->system) : "n/a");
        if (element->valid_information & QMI_LOC_SATELLITE_VALID_INFORMATION_GNSS_SATELLITE_ID)
            g_print ("      satellite id:     %u\n", element->gnss_satellite_id);
        else
            g_print ("      satellite id:     n/a\n");
        g_print ("      health status:    %s\n", (element->valid_information & QMI_LOC_SATELLITE_VALID_INFORMATION_HEALTH_STATUS) ? qmi_loc_health_status_get_string (element->health_status) : "n/a");
        g_print ("      satellite status: %s\n", (element->valid_information & QMI_LOC_SATELLITE_VALID_INFORMATION_PROCESS_STATUS) ? qmi_loc_satellite_status_get_string (element->satellite_status) : "n/a");
        g_print ("      navigation data:  %s\n", (element->valid_information & QMI_LOC_SATELLITE_VALID_INFORMATION_SATELLITE_INFO_MASK) ? qmi_loc_navigation_data_get_string (element->navigation_data) : "n/a");

        if (element->valid_information & QMI_LOC_SATELLITE_VALID_INFORMATION_ELEVATION)
            g_print ("      elevation:        %lf\n", (gdouble)element->elevation_degrees);
        else
            g_print ("      elevation:        n/a\n");

        if (element->valid_information & QMI_LOC_SATELLITE_VALID_INFORMATION_AZIMUTH)
            g_print ("      azimuth:          %lf\n", (gdouble)element->azimuth_degrees);
        else
            g_print ("      azimuth:          n/a\n");

        if (element->valid_information & QMI_LOC_SATELLITE_VALID_INFORMATION_SIGNAL_TO_NOISE_RATIO)
            g_print ("      SNR:              %lf\n", (gdouble)element->signal_to_noise_ratio_bhz);
        else
            g_print ("      SNR:              n/a\n");
    }

    /* Terminate GET request */
    if (get_gnss_sv_info_flag)
        operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_INDICATION_LOC_GNSS_SV_INFO */

#if defined HAVE_QMI_INDICATION_LOC_POSITION_REPORT && defined HAVE_QMI_MESSAGE_LOC_REGISTER_EVENTS

static void
position_report_received (QmiClientLoc                         *client,
                          QmiIndicationLocPositionReportOutput *output)
{
    QmiLocSessionStatus status;

    qmi_indication_loc_position_report_output_get_session_status (output, &status, NULL);
    g_print ("[position report] status: %s\n", qmi_loc_session_status_get_string (status));

    if (status == QMI_LOC_SESSION_STATUS_SUCCESS || status == QMI_LOC_SESSION_STATUS_IN_PROGRESS) {
        gdouble auxd;
        gfloat auxf;
        guint8 aux8;
        guint32 aux32;
        guint64 aux64;
        QmiLocReliability reliability;
        QmiLocTechnologyUsed technology;
        QmiLocTimeSource time_source;
        QmiLocSensorDataUsage sensor_data_usage;
        QmiIndicationLocPositionReportOutputDilutionOfPrecision dop;
        QmiIndicationLocPositionReportOutputGpsTime gps_time;
        gboolean auxb;
        GArray *array;

        if (qmi_indication_loc_position_report_output_get_latitude (output, &auxd, NULL))
            g_print ("   latitude:  %lf degrees\n", auxd);
        else
            g_print ("   latitude:  n/a\n");

        if (qmi_indication_loc_position_report_output_get_longitude (output, &auxd, NULL))
            g_print ("   longitude: %lf degrees\n", auxd);
        else
            g_print ("   longitude: n/a\n");

        if (qmi_indication_loc_position_report_output_get_horizontal_uncertainty_circular (output, &auxf, NULL))
            g_print ("   circular horizontal position uncertainty:            %lf meters\n", (gdouble)auxf);
        else
            g_print ("   circular horizontal position uncertainty:            n/a\n");

        if (qmi_indication_loc_position_report_output_get_horizontal_uncertainty_elliptical_minor (output, &auxf, NULL))
            g_print ("   horizontal elliptical uncertainty (semi-minor axis): %lf meters\n", (gdouble)auxf);
        else
            g_print ("   horizontal elliptical uncertainty (semi-minor axis): n/a\n");

        if (qmi_indication_loc_position_report_output_get_horizontal_uncertainty_elliptical_major (output, &auxf, NULL))
            g_print ("   horizontal elliptical uncertainty (semi-major axis): %lf meters\n", (gdouble)auxf);
        else
            g_print ("   horizontal elliptical uncertainty (semi-major axis): n/a\n");

        if (qmi_indication_loc_position_report_output_get_horizontal_uncertainty_elliptical_azimuth (output, &auxf, NULL))
            g_print ("   horizontal elliptical uncertainty azimuth:           %lf meters\n", (gdouble)auxf);
        else
            g_print ("   horizontal elliptical uncertainty azimuth:           n/a\n");

        if (qmi_indication_loc_position_report_output_get_horizontal_confidence (output, &aux8, NULL))
            g_print ("   horizontal confidence: %u%%\n", aux8);
        else
            g_print ("   horizontal confidence: n/a\n");

        if (qmi_indication_loc_position_report_output_get_horizontal_reliability (output, &reliability, NULL))
            g_print ("   horizontal reliability: %s\n", qmi_loc_reliability_get_string (reliability));
        else
            g_print ("   horizontal reliability: n/a\n");

        if (qmi_indication_loc_position_report_output_get_horizontal_speed (output, &auxf, NULL))
            g_print ("   horizontal speed: %lf m/s\n", (gdouble)auxf);
        else
            g_print ("   horizontal speed: n/a\n");

        if (qmi_indication_loc_position_report_output_get_speed_uncertainty (output, &auxf, NULL))
            g_print ("   speed uncertainty: %lf m/s\n", (gdouble)auxf);
        else
            g_print ("   speed uncertainty: n/a\n");

        if (qmi_indication_loc_position_report_output_get_altitude_from_ellipsoid (output, &auxf, NULL))
            g_print ("   altitude w.r.t. ellipsoid: %lf meters\n", (gdouble)auxf);
        else
            g_print ("   altitude w.r.t. ellipsoid: n/a\n");

        if (qmi_indication_loc_position_report_output_get_altitude_from_sealevel (output, &auxf, NULL))
            g_print ("   altitude w.r.t. mean sea level: %lf meters\n", (gdouble)auxf);
        else
            g_print ("   altitude w.r.t. mean sea level: n/a\n");

        if (qmi_indication_loc_position_report_output_get_vertical_uncertainty (output, &auxf, NULL))
            g_print ("   vertical uncertainty: %lf meters\n", (gdouble)auxf);
        else
            g_print ("   vertical uncertainty: n/a\n");

        if (qmi_indication_loc_position_report_output_get_vertical_confidence (output, &aux8, NULL))
            g_print ("   vertical confidence: %u%%\n", aux8);
        else
            g_print ("   vertical confidence: n/a\n");

        if (qmi_indication_loc_position_report_output_get_vertical_reliability (output, &reliability, NULL))
            g_print ("   vertical reliability: %s\n", qmi_loc_reliability_get_string (reliability));
        else
            g_print ("   vertical reliability: n/a\n");

        if (qmi_indication_loc_position_report_output_get_vertical_speed (output, &auxf, NULL))
            g_print ("   vertical speed: %lf m/s\n", (gdouble)auxf);
        else
            g_print ("   vertical speed: n/a\n");

        if (qmi_indication_loc_position_report_output_get_heading (output, &auxf, NULL))
            g_print ("   heading: %lf degrees\n", (gdouble)auxf);
        else
            g_print ("   heading: n/a\n");

        if (qmi_indication_loc_position_report_output_get_heading_uncertainty (output, &auxf, NULL))
            g_print ("   heading uncertainty: %lf meters\n", (gdouble)auxf);
        else
            g_print ("   heading uncertainty: n/a\n");

        if (qmi_indication_loc_position_report_output_get_magnetic_deviation (output, &auxf, NULL))
            g_print ("   magnetic deviation: %lf degrees\n", (gdouble)auxf);
        else
            g_print ("   magnetic deviation: n/a\n");

        if (qmi_indication_loc_position_report_output_get_technology_used (output, &technology, NULL)) {
            g_autofree gchar *technology_str = NULL;

            technology_str = qmi_loc_technology_used_build_string_from_mask (technology);
            g_print ("   technology: %s\n", VALIDATE_MASK_NONE (technology_str));
        } else
            g_print ("   technology: n/a\n");

        if (qmi_indication_loc_position_report_output_get_dilution_of_precision (output, &dop, NULL)) {
            g_print ("   position DOP:   %lf\n", (gdouble)dop.position_dilution_of_precision);
            g_print ("   horizontal DOP: %lf\n", (gdouble)dop.horizontal_dilution_of_precision);
            g_print ("   vertical DOP:   %lf\n", (gdouble)dop.vertical_dilution_of_precision);
        } else {
            g_print ("   position DOP:   n/a\n");
            g_print ("   horizontal DOP: n/a\n");
            g_print ("   vertical DOP:   n/a\n");
        }

        if (qmi_indication_loc_position_report_output_get_utc_timestamp (output, &aux64, NULL))
            g_print ("   UTC timestamp: %" G_GUINT64_FORMAT " ms\n", aux64);
        else
            g_print ("   UTC timestamp: n/a\n");

        if (qmi_indication_loc_position_report_output_get_leap_seconds (output, &aux8, NULL))
            g_print ("   Leap seconds: %u\n", aux8);
        else
            g_print ("   Leap seconds: n/a\n");

        if (qmi_indication_loc_position_report_output_get_gps_time (output, &gps_time, NULL))
            g_print ("   GPS time: %u weeks and %ums\n", gps_time.gps_weeks, gps_time.gps_time_of_week_milliseconds);
        else
            g_print ("   GPS time: n/a\n");

        if (qmi_indication_loc_position_report_output_get_time_uncertainty (output, &auxf, NULL))
            g_print ("   time uncertainty: %lf ms\n", (gdouble)auxf);
        else
            g_print ("   time uncertainty: n/a\n");

        if (qmi_indication_loc_position_report_output_get_time_source (output, &time_source, NULL))
            g_print ("   time source: %s\n", qmi_loc_time_source_get_string (time_source));
        else
            g_print ("   time source: n/a\n");

        if (qmi_indication_loc_position_report_output_get_sensor_data_usage (output, &sensor_data_usage, NULL)) {
            g_autofree gchar *sensor_data_usage_str = NULL;

            sensor_data_usage_str = qmi_loc_sensor_data_usage_build_string_from_mask (sensor_data_usage);
            g_print ("   sensor data usage: %s\n", VALIDATE_MASK_NONE (sensor_data_usage_str));
        } else
            g_print ("   sensor data usage: n/a\n");

        if (qmi_indication_loc_position_report_output_get_session_fix_count (output, &aux32, NULL))
            g_print ("   Fix count: %u\n", aux32);
        else
            g_print ("   Fix count: n/a\n");

        if (qmi_indication_loc_position_report_output_get_satellites_used (output, &array, NULL)) {
            guint i;

            g_print ("   Satellites used: ");
            for (i = 0; i < array->len; i++) {
                guint16 sv_id;

                /*
                 * - For GPS:     1 to 32
                 * - For SBAS:    33 to 64
                 * - For GLONASS: 65 to 96
                 * - For QZSS:    193 to 197
                 * - For BDS:     201 to 237
                 */
                sv_id = g_array_index (array, guint16, i);
                g_print ("%u%s", sv_id, i == array->len - 1 ? "" : ",");
            }
            g_print ("\n");
        } else
            g_print ("   Satellites used: n/a\n");

        if (qmi_indication_loc_position_report_output_get_altitude_assumed (output, &auxb, NULL))
            g_print ("   Altitude assumed: %s\n", auxb ? "yes" : "no");
        else
            g_print ("   Altitude assumed: n/a\n");

        /* Terminate GET request */
        if (get_position_report_flag)
            operation_shutdown (TRUE);

        return;
    }

    /* Otherwise, treat as error */
    g_printerr ("[position report] error: %s\n", qmi_loc_session_status_get_string (status));
    if (get_position_report_flag)
        operation_shutdown (FALSE);
}

#endif /* HAVE_QMI_INDICATION_LOC_POSITION_REPORT */

#if (defined HAVE_QMI_INDICATION_LOC_POSITION_REPORT || \
     defined HAVE_QMI_INDICATION_LOC_GNSS_SV_INFO ||    \
     defined HAVE_QMI_INDICATION_LOC_NMEA) && \
    defined HAVE_QMI_MESSAGE_LOC_REGISTER_EVENTS

static void
monitoring_step_ongoing (void)
{
#if defined HAVE_QMI_INDICATION_LOC_POSITION_REPORT
    if (get_position_report_flag || follow_position_report_flag)
        ctx->position_report_indication_id = g_signal_connect (ctx->client,
                                                               "position-report",
                                                               G_CALLBACK (position_report_received),
                                                               NULL);
#endif

#if defined HAVE_QMI_INDICATION_LOC_GNSS_SV_INFO
    if (get_gnss_sv_info_flag || follow_gnss_sv_info_flag)
        ctx->gnss_sv_info_indication_id = g_signal_connect (ctx->client,
                                                            "gnss-sv-info",
                                                            G_CALLBACK (gnss_sv_info_received),
                                                            NULL);
#endif

#if defined HAVE_QMI_INDICATION_LOC_NMEA
    if (follow_nmea_flag)
        ctx->nmea_indication_id = g_signal_connect (ctx->client,
                                                    "nmea",
                                                    G_CALLBACK (nmea_received),
                                                    NULL);
#endif

    g_assert (ctx->position_report_indication_id ||
              ctx->gnss_sv_info_indication_id ||
              ctx->nmea_indication_id);
}

static void
monitoring_step_setup_timeout (void)
{
    /* User can use Ctrl+C to cancel the monitoring at any time */
    g_cancellable_connect (ctx->cancellable,
                           G_CALLBACK (monitoring_cancelled),
                           NULL,
                           NULL);

    /* For non-follow requests, we also setup a timeout */
    if (get_position_report_flag || get_gnss_sv_info_flag)
        ctx->timeout_id = g_timeout_add_seconds (timeout > 0 ? timeout : DEFAULT_LOC_TIMEOUT_SECS,
                                                 (GSourceFunc) monitoring_timed_out,
                                                 NULL);

    /* Go on */
    ctx->monitoring_step++;
    monitoring_step_run ();
}

static void
register_events_ready (QmiClientLoc *client,
                       GAsyncResult *res)
{
   QmiMessageLocRegisterEventsOutput *output;
   GError                            *error = NULL;

   output = qmi_client_loc_register_events_finish (client, res, &error);
   if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
   }

    if (!qmi_message_loc_register_events_output_get_result (output, &error)) {
        g_printerr ("error: could not register location tracking events: %s\n", error->message);
        qmi_message_loc_register_events_output_unref (output);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    g_debug ("Registered location tracking events...");

    /* Go on */
    ctx->monitoring_step++;
    monitoring_step_run ();

    qmi_message_loc_register_events_output_unref (output);
}

static void
monitoring_step_register_events (void)
{
    QmiMessageLocRegisterEventsInput *re_input;
    QmiLocEventRegistrationFlag       indication_mask = 0;

    /* Configure events to enable */

    if (get_position_report_flag || follow_position_report_flag)
        indication_mask |= QMI_LOC_EVENT_REGISTRATION_FLAG_POSITION_REPORT;

    if (get_gnss_sv_info_flag || follow_gnss_sv_info_flag)
        indication_mask |= QMI_LOC_EVENT_REGISTRATION_FLAG_GNSS_SATELLITE_INFO;

    if (follow_nmea_flag)
        indication_mask |= QMI_LOC_EVENT_REGISTRATION_FLAG_NMEA;

    g_assert (indication_mask);

    re_input = qmi_message_loc_register_events_input_new ();
    qmi_message_loc_register_events_input_set_event_registration_mask (
        re_input, indication_mask, NULL);
    qmi_client_loc_register_events (ctx->client,
                                    re_input,
                                    10,
                                    ctx->cancellable,
                                    (GAsyncReadyCallback) register_events_ready,
                                    NULL);
    qmi_message_loc_register_events_input_unref (re_input);
}

static void
monitoring_step_run (void)
{
    switch (ctx->monitoring_step) {
    case MONITORING_STEP_FIRST:
        ctx->monitoring_step++;
        /* fall through */

    case MONITORING_STEP_REGISTER_EVENTS:
        monitoring_step_register_events ();
        return;

    case MONITORING_STEP_SETUP_TIMEOUT:
        monitoring_step_setup_timeout ();
        return;

    case MONITORING_STEP_ONGOING:
        monitoring_step_ongoing ();
        return;

    default:
        g_assert_not_reached();
    }
}

#endif /* HAVE_QMI_INDICATION_LOC_POSITION_REPORT
        * HAVE_QMI_INDICATION_LOC_GNSS_SV_INFO
        * HAVE_QMI_INDICATION_LOC_NMEA */

#if defined HAVE_QMI_MESSAGE_LOC_DELETE_ASSISTANCE_DATA

static gboolean
delete_assistance_data_timed_out (void)
{
    ctx->timeout_id = 0;
    g_printerr ("error: operation failed: timeout\n");
    operation_shutdown (FALSE);
    return G_SOURCE_REMOVE;
}

static void
delete_assistance_data_received (QmiClientLoc                               *client,
                                 QmiIndicationLocDeleteAssistanceDataOutput *output)
{
    QmiLocIndicationStatus  status;
    GError                 *error = NULL;

    if (!qmi_indication_loc_delete_assistance_data_output_get_indication_status (output, &status, &error)) {
        g_printerr ("error: couldn't delete assistance data: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("Successfully deleted assistance data\n");
    operation_shutdown (TRUE);
}

static void
delete_assistance_data_ready (QmiClientLoc *client,
                              GAsyncResult *res)
{
    QmiMessageLocDeleteAssistanceDataOutput *output;
    GError                                  *error = NULL;

    output = qmi_client_loc_delete_assistance_data_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_loc_delete_assistance_data_output_get_result (output, &error)) {
        g_printerr ("error: could not delete assistance data: %s\n", error->message);
        qmi_message_loc_delete_assistance_data_output_unref (output);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    /* Wait for response asynchronously */
    ctx->timeout_id = g_timeout_add_seconds (timeout > 0 ? timeout : DEFAULT_LOC_TIMEOUT_SECS,
                                             (GSourceFunc) delete_assistance_data_timed_out,
                                             NULL);

    ctx->delete_assistance_data_indication_id = g_signal_connect (ctx->client,
                                                                  "delete-assistance-data",
                                                                  G_CALLBACK (delete_assistance_data_received),
                                                                  NULL);

    qmi_message_loc_delete_assistance_data_output_unref (output);
}

#endif /* HAVE_QMI_MESSAGE_LOC_DELETE_ASSISTANCE_DATA */

#if defined HAVE_QMI_MESSAGE_LOC_GET_NMEA_TYPES

static gboolean
get_nmea_types_timed_out (void)
{
    ctx->timeout_id = 0;
    g_printerr ("error: operation failed: timeout\n");
    operation_shutdown (FALSE);
    return G_SOURCE_REMOVE;
}

static void
get_nmea_types_received (QmiClientLoc                       *client,
                         QmiIndicationLocGetNmeaTypesOutput *output)
{
    QmiLocIndicationStatus  status;
    QmiLocNmeaType          nmea_types_mask;
    g_autoptr(GError)       error = NULL;
    g_autofree gchar       *nmea_types_str = NULL;

    if (!qmi_indication_loc_get_nmea_types_output_get_indication_status (output, &status, &error)) {
        g_printerr ("error: couldn't get NMEA types: %s\n", error->message);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_indication_loc_get_nmea_types_output_get_nmea_types (output, &nmea_types_mask, NULL)) {
        g_printerr ("error: couldn't get NMEA types: missing\n");
        operation_shutdown (FALSE);
        return;
    }

    nmea_types_str = qmi_loc_nmea_type_build_string_from_mask (nmea_types_mask);
    g_print ("Successfully retrieved NMEA types: %s\n", VALIDATE_MASK_NONE (nmea_types_str));
    operation_shutdown (TRUE);
}

static void
get_nmea_types_ready (QmiClientLoc *client,
                      GAsyncResult *res)
{
    g_autoptr(QmiMessageLocGetNmeaTypesOutput) output = NULL;
    g_autoptr(GError)                          error = NULL;

    output = qmi_client_loc_get_nmea_types_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_loc_get_nmea_types_output_get_result (output, &error)) {
        g_printerr ("error: could not get NMEA types: %s\n", error->message);
        operation_shutdown (FALSE);
        return;
    }

    /* Wait for response asynchronously */
    ctx->timeout_id = g_timeout_add_seconds (timeout > 0 ? timeout : DEFAULT_LOC_TIMEOUT_SECS,
                                             (GSourceFunc) get_nmea_types_timed_out,
                                             NULL);

    ctx->get_nmea_types_indication_id = g_signal_connect (ctx->client,
                                                          "get-nmea-types",
                                                          G_CALLBACK (get_nmea_types_received),
                                                          NULL);
}

#endif /* HAVE_QMI_MESSAGE_LOC_GET_NMEA_TYPES */

#if defined HAVE_QMI_MESSAGE_LOC_SET_NMEA_TYPES

static gboolean
set_nmea_types_timed_out (void)
{
    ctx->timeout_id = 0;
    g_printerr ("error: operation failed: timeout\n");
    operation_shutdown (FALSE);
    return G_SOURCE_REMOVE;
}

static void
set_nmea_types_received (QmiClientLoc                       *client,
                         QmiIndicationLocSetNmeaTypesOutput *output)
{
    QmiLocIndicationStatus status;
    g_autoptr(GError)      error = NULL;

    if (!qmi_indication_loc_set_nmea_types_output_get_indication_status (output, &status, &error)) {
        g_printerr ("error: couldn't set NMEA types: %s\n", error->message);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("Successfully set NMEA types\n");
    operation_shutdown (TRUE);
}

static void
set_nmea_types_ready (QmiClientLoc *client,
                      GAsyncResult *res)
{
    g_autoptr(QmiMessageLocSetNmeaTypesOutput) output = NULL;
    g_autoptr(GError)                          error = NULL;

    output = qmi_client_loc_set_nmea_types_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_loc_set_nmea_types_output_get_result (output, &error)) {
        g_printerr ("error: could not set NMEA types: %s\n", error->message);
        operation_shutdown (FALSE);
        return;
    }

    /* Wait for response asynchronously */
    ctx->timeout_id = g_timeout_add_seconds (timeout > 0 ? timeout : DEFAULT_LOC_TIMEOUT_SECS,
                                             (GSourceFunc) set_nmea_types_timed_out,
                                             NULL);

    ctx->set_nmea_types_indication_id = g_signal_connect (ctx->client,
                                                          "set-nmea-types",
                                                          G_CALLBACK (set_nmea_types_received),
                                                          NULL);
}

static QmiMessageLocSetNmeaTypesInput *
set_nmea_types_input_create (const gchar *str)
{
    g_autoptr(QmiMessageLocSetNmeaTypesInput) input = NULL;
    g_autoptr(GError)                         error = NULL;
    QmiLocNmeaType                            nmea_type_mask;

    if (!qmicli_read_loc_nmea_type_from_string (str, &nmea_type_mask)) {
        g_printerr ("error: couldn't parse input string as NMEA types: '%s'\n", str);
        return NULL;
    }

    input = qmi_message_loc_set_nmea_types_input_new ();
    if (!qmi_message_loc_set_nmea_types_input_set_nmea_types (input, nmea_type_mask, &error)) {
        g_printerr ("error: couldn't create input data bundle: '%s'\n", error->message);
        return NULL;
    }

    return g_steal_pointer (&input);
}

#endif /* HAVE_QMI_MESSAGE_LOC_SET_NMEA_TYPES */

#if defined HAVE_QMI_MESSAGE_LOC_GET_OPERATION_MODE

static gboolean
get_operation_mode_timed_out (void)
{
    ctx->timeout_id = 0;
    g_printerr ("error: operation failed: timeout\n");
    operation_shutdown (FALSE);
    return G_SOURCE_REMOVE;
}

static void
get_operation_mode_received (QmiClientLoc                           *client,
                             QmiIndicationLocGetOperationModeOutput *output)
{
    QmiLocIndicationStatus  status;
    QmiLocOperationMode     operation_mode;
    g_autoptr(GError)       error = NULL;

    if (!qmi_indication_loc_get_operation_mode_output_get_indication_status (output, &status, &error)) {
        g_printerr ("error: couldn't get operation mode %s\n", error->message);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_indication_loc_get_operation_mode_output_get_operation_mode (output, &operation_mode, NULL)) {
        g_printerr ("error: couldn't get operation mode: missing\n");
        operation_shutdown (FALSE);
        return;
    }

    g_print ("Successfully retrieved operation mode: %s\n", qmi_loc_operation_mode_get_string (operation_mode));
    operation_shutdown (TRUE);
}

static void
get_operation_mode_ready (QmiClientLoc *client,
                          GAsyncResult *res)
{
    g_autoptr(QmiMessageLocGetOperationModeOutput) output = NULL;
    g_autoptr(GError)                              error = NULL;

    output = qmi_client_loc_get_operation_mode_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_loc_get_operation_mode_output_get_result (output, &error)) {
        g_printerr ("error: could not get operation mode: %s\n", error->message);
        operation_shutdown (FALSE);
        return;
    }

    /* Wait for response asynchronously */
    ctx->timeout_id = g_timeout_add_seconds (timeout > 0 ? timeout : DEFAULT_LOC_TIMEOUT_SECS,
                                             (GSourceFunc) get_operation_mode_timed_out,
                                             NULL);

    ctx->get_operation_mode_indication_id = g_signal_connect (ctx->client,
                                                              "get-operation-mode",
                                                              G_CALLBACK (get_operation_mode_received),
                                                              NULL);
}
#endif /* HAVE_QMI_MESSAGE_LOC_GET_OPERATION_MODE */

#if defined HAVE_QMI_MESSAGE_LOC_SET_OPERATION_MODE

static gboolean
set_operation_mode_timed_out (void)
{
    ctx->timeout_id = 0;
    g_printerr ("error: operation failed: timeout\n");
    operation_shutdown (FALSE);
    return G_SOURCE_REMOVE;
}

static void
set_operation_mode_received (QmiClientLoc                           *client,
                             QmiIndicationLocSetOperationModeOutput *output)
{
    QmiLocIndicationStatus status;
    g_autoptr(GError)      error = NULL;

    if (!qmi_indication_loc_set_operation_mode_output_get_indication_status (output, &status, &error)) {
        g_printerr ("error: couldn't set operation mode: %s\n", error->message);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("Successfully set operation mode\n");
    operation_shutdown (TRUE);
}

static void
set_operation_mode_ready (QmiClientLoc *client,
                          GAsyncResult *res)
{
    g_autoptr(QmiMessageLocSetOperationModeOutput) output = NULL;
    g_autoptr(GError)                              error = NULL;

    output = qmi_client_loc_set_operation_mode_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_loc_set_operation_mode_output_get_result (output, &error)) {
        g_printerr ("error: could not set operation mode: %s\n", error->message);
        operation_shutdown (FALSE);
        return;
    }

    /* Wait for response asynchronously */
    ctx->timeout_id = g_timeout_add_seconds (timeout > 0 ? timeout : DEFAULT_LOC_TIMEOUT_SECS,
                                             (GSourceFunc) set_operation_mode_timed_out,
                                             NULL);

    ctx->set_operation_mode_indication_id = g_signal_connect (ctx->client,
                                                              "set-operation-mode",
                                                              G_CALLBACK (set_operation_mode_received),
                                                              NULL);
}

static QmiMessageLocSetOperationModeInput *
set_operation_mode_input_create (const gchar *str)
{
    g_autoptr(QmiMessageLocSetOperationModeInput) input = NULL;
    g_autoptr(GError)                             error = NULL;
    QmiLocOperationMode                           operation_mode;

    if (!qmicli_read_loc_operation_mode_from_string (str, &operation_mode))
        return NULL;

    input = qmi_message_loc_set_operation_mode_input_new ();
    if (!qmi_message_loc_set_operation_mode_input_set_operation_mode (input, operation_mode, &error)) {
        g_printerr ("error: couldn't create input data bundle: '%s'\n", error->message);
        return NULL;
    }

    return g_steal_pointer (&input);
}

#endif /* HAVE_QMI_MESSAGE_LOC_SET_OPERATION_MODE */

#if defined HAVE_QMI_MESSAGE_LOC_GET_ENGINE_LOCK

static gboolean
get_engine_lock_timed_out (void)
{
    ctx->timeout_id = 0;
    g_printerr ("error: operation failed: timeout\n");
    operation_shutdown (FALSE);
    return G_SOURCE_REMOVE;
}

static void
get_engine_lock_received (QmiClientLoc                        *client,
                          QmiIndicationLocGetEngineLockOutput *output)
{
    QmiLocIndicationStatus  status;
    QmiLocLockType          type;
    g_autoptr(GError)       error = NULL;

    if (!qmi_indication_loc_get_engine_lock_output_get_indication_status (output, &status, &error)) {
        g_printerr ("error: couldn't get engine lock %s\n", error->message);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_indication_loc_get_engine_lock_output_get_lock_type (output, &type, NULL)) {
        g_printerr ("error: couldn't get engine lock: missing\n");
        operation_shutdown (FALSE);
        return;
    }

    g_print ("Successfully retrieved engine lock: %s\n", qmi_loc_lock_type_get_string (type));
    operation_shutdown (TRUE);
}

static void
get_engine_lock_ready (QmiClientLoc *client,
                       GAsyncResult *res)
{
    g_autoptr(QmiMessageLocGetEngineLockOutput) output = NULL;
    g_autoptr(GError)                           error = NULL;

    output = qmi_client_loc_get_engine_lock_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_loc_get_engine_lock_output_get_result (output, &error)) {
        g_printerr ("error: could not get engine lock: %s\n", error->message);
        operation_shutdown (FALSE);
        return;
    }

    /* Wait for response asynchronously */
    ctx->timeout_id = g_timeout_add_seconds (timeout > 0 ? timeout : DEFAULT_LOC_TIMEOUT_SECS,
                                             (GSourceFunc) get_engine_lock_timed_out,
                                             NULL);

    ctx->get_engine_lock_indication_id = g_signal_connect (ctx->client,
                                                           "get-engine-lock",
                                                           G_CALLBACK (get_engine_lock_received),
                                                           NULL);
}
#endif /* HAVE_QMI_MESSAGE_LOC_GET_ENGINE_LOCK */

#if defined HAVE_QMI_MESSAGE_LOC_SET_ENGINE_LOCK

static gboolean
set_engine_lock_timed_out (void)
{
    ctx->timeout_id = 0;
    g_printerr ("error: operation failed: timeout\n");
    operation_shutdown (FALSE);
    return G_SOURCE_REMOVE;
}

static void
set_engine_lock_received (QmiClientLoc                        *client,
                          QmiIndicationLocSetEngineLockOutput *output)
{
    QmiLocIndicationStatus status;
    g_autoptr(GError)      error = NULL;

    if (!qmi_indication_loc_set_engine_lock_output_get_indication_status (output, &status, &error)) {
        g_printerr ("error: couldn't set engine lock: %s\n", error->message);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("Successfully set engine lock\n");
    operation_shutdown (TRUE);
}

static void
set_engine_lock_ready (QmiClientLoc *client,
                          GAsyncResult *res)
{
    g_autoptr(QmiMessageLocSetEngineLockOutput) output = NULL;
    g_autoptr(GError)                           error = NULL;

    output = qmi_client_loc_set_engine_lock_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_loc_set_engine_lock_output_get_result (output, &error)) {
        g_printerr ("error: could not set engine lock: %s\n", error->message);
        operation_shutdown (FALSE);
        return;
    }

    /* Wait for response asynchronously */
    ctx->timeout_id = g_timeout_add_seconds (timeout > 0 ? timeout : DEFAULT_LOC_TIMEOUT_SECS,
                                             (GSourceFunc) set_engine_lock_timed_out,
                                             NULL);

    ctx->set_engine_lock_indication_id = g_signal_connect (ctx->client,
                                                           "set-engine-lock",
                                                           G_CALLBACK (set_engine_lock_received),
                                                           NULL);
}

static QmiMessageLocSetEngineLockInput *
set_engine_lock_input_create (const gchar *str)
{
    g_autoptr(QmiMessageLocSetEngineLockInput) input = NULL;
    g_autoptr(GError)                          error = NULL;
    QmiLocLockType                             type;

    if (!qmicli_read_loc_lock_type_from_string (str, &type))
        return NULL;

    input = qmi_message_loc_set_engine_lock_input_new ();
    if (!qmi_message_loc_set_engine_lock_input_set_lock_type (input, type, &error)) {
        g_printerr ("error: couldn't create input data bundle: '%s'\n", error->message);
        return NULL;
    }

    return g_steal_pointer (&input);
}

#endif /* HAVE_QMI_MESSAGE_LOC_SET_ENGINE_LOCK */

#if defined HAVE_QMI_MESSAGE_LOC_STOP

static void
stop_ready (QmiClientLoc *client,
            GAsyncResult *res)
{
    QmiMessageLocStopOutput *output;
    GError                  *error = NULL;

    output = qmi_client_loc_stop_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_loc_stop_output_get_result (output, &error)) {
        g_printerr ("error: could not stop location tracking: %s\n", error->message);
        qmi_message_loc_stop_output_unref (output);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] Successfully stopped location tracking (session id %u)\n",
             qmi_device_get_path_display (ctx->device), session_id);

    qmi_message_loc_stop_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_LOC_STOP */

#if defined HAVE_QMI_MESSAGE_LOC_START

static void
start_ready (QmiClientLoc *client,
             GAsyncResult *res)
{
    QmiMessageLocStartOutput *output;
    GError                   *error = NULL;

    output = qmi_client_loc_start_finish (client, res, &error);
    if (!output) {
        g_printerr ("error: operation failed: %s\n", error->message);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    if (!qmi_message_loc_start_output_get_result (output, &error)) {
        g_printerr ("error: could not start location tracking: %s\n", error->message);
        qmi_message_loc_start_output_unref (output);
        g_error_free (error);
        operation_shutdown (FALSE);
        return;
    }

    g_print ("[%s] Successfully started location tracking (session id %u)\n",
             qmi_device_get_path_display (ctx->device), session_id);

    qmi_message_loc_start_output_unref (output);
    operation_shutdown (TRUE);
}

#endif /* HAVE_QMI_MESSAGE_LOC_START */

static gboolean
noop_cb (gpointer unused)
{
    operation_shutdown (TRUE);
    return G_SOURCE_REMOVE;
}

void
qmicli_loc_run (QmiDevice    *device,
                QmiClientLoc *client,
                GCancellable *cancellable)
{
    /* Initialize context */
    ctx = g_slice_new0 (Context);
    ctx->device = g_object_ref (device);
    ctx->client = g_object_ref (client);
    ctx->cancellable = g_object_ref (cancellable);

#if defined HAVE_QMI_MESSAGE_LOC_START
    if (start_flag) {
        QmiMessageLocStartInput *input;

        input = qmi_message_loc_start_input_new ();
        qmi_message_loc_start_input_set_session_id (input, (guint8) session_id, NULL);
        qmi_message_loc_start_input_set_intermediate_report_state (input, QMI_LOC_INTERMEDIATE_REPORT_STATE_ENABLE, NULL);
        qmi_message_loc_start_input_set_minimum_interval_between_position_reports (input, 1000, NULL);
        qmi_message_loc_start_input_set_fix_recurrence_type (input, QMI_LOC_FIX_RECURRENCE_TYPE_REQUEST_PERIODIC_FIXES, NULL);
        qmi_client_loc_start (ctx->client,
                              input,
                              10,
                              ctx->cancellable,
                              (GAsyncReadyCallback) start_ready,
                              NULL);
        qmi_message_loc_start_input_unref (input);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_LOC_STOP
    if (stop_flag) {
        QmiMessageLocStopInput *input;

        input = qmi_message_loc_stop_input_new ();
        qmi_message_loc_stop_input_set_session_id (input, (guint8) session_id, NULL);
        qmi_client_loc_stop (ctx->client,
                             input,
                             10,
                             ctx->cancellable,
                             (GAsyncReadyCallback) stop_ready,
                             NULL);
        qmi_message_loc_stop_input_unref (input);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_LOC_DELETE_ASSISTANCE_DATA
    if (delete_assistance_data_flag) {
        QmiMessageLocDeleteAssistanceDataInput *input;

        input = qmi_message_loc_delete_assistance_data_input_new ();
        qmi_message_loc_delete_assistance_data_input_set_delete_all (input, TRUE, NULL);
        qmi_client_loc_delete_assistance_data (ctx->client,
                                               input,
                                               10,
                                               ctx->cancellable,
                                               (GAsyncReadyCallback) delete_assistance_data_ready,
                                               NULL);
        qmi_message_loc_delete_assistance_data_input_unref (input);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_LOC_GET_NMEA_TYPES
    if (get_nmea_types_flag) {
        qmi_client_loc_get_nmea_types (ctx->client,
                                       NULL,
                                       10,
                                       ctx->cancellable,
                                       (GAsyncReadyCallback) get_nmea_types_ready,
                                       NULL);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_LOC_SET_NMEA_TYPES
    if (set_nmea_types_str) {
        g_autoptr(QmiMessageLocSetNmeaTypesInput) input = NULL;

        g_debug ("Asynchronously setting APN type...");
        input = set_nmea_types_input_create (set_nmea_types_str);
        if (!input) {
            operation_shutdown (FALSE);
            return;
        }
        qmi_client_loc_set_nmea_types (ctx->client,
                                       input,
                                       10,
                                       ctx->cancellable,
                                       (GAsyncReadyCallback)set_nmea_types_ready,
                                       NULL);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_LOC_GET_OPERATION_MODE
    if (get_operation_mode_flag) {
        qmi_client_loc_get_operation_mode (ctx->client,
                                           NULL,
                                           10,
                                           ctx->cancellable,
                                           (GAsyncReadyCallback) get_operation_mode_ready,
                                           NULL);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_LOC_SET_OPERATION_MODE
    if (set_operation_mode_str) {
        g_autoptr(QmiMessageLocSetOperationModeInput) input = NULL;

        g_debug ("Asynchronously setting operation mode...");
        input = set_operation_mode_input_create (set_operation_mode_str);
        if (!input) {
            operation_shutdown (FALSE);
            return;
        }
        qmi_client_loc_set_operation_mode (ctx->client,
                                           input,
                                           10,
                                           ctx->cancellable,
                                           (GAsyncReadyCallback)set_operation_mode_ready,
                                           NULL);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_LOC_GET_ENGINE_LOCK
    if (get_engine_lock_flag) {
        qmi_client_loc_get_engine_lock (ctx->client,
                                        NULL,
                                        10,
                                        ctx->cancellable,
                                        (GAsyncReadyCallback) get_engine_lock_ready,
                                        NULL);
        return;
    }
#endif

#if defined HAVE_QMI_MESSAGE_LOC_SET_ENGINE_LOCK
    if (set_engine_lock_str) {
        g_autoptr(QmiMessageLocSetEngineLockInput) input = NULL;

        g_debug ("Asynchronously setting engine lock...");
        input = set_engine_lock_input_create (set_engine_lock_str);
        if (!input) {
            operation_shutdown (FALSE);
            return;
        }
        qmi_client_loc_set_engine_lock (ctx->client,
                                        input,
                                        10,
                                        ctx->cancellable,
                                        (GAsyncReadyCallback)set_engine_lock_ready,
                                        NULL);
        return;
    }
#endif

#if (defined HAVE_QMI_INDICATION_LOC_POSITION_REPORT || \
     defined HAVE_QMI_INDICATION_LOC_GNSS_SV_INFO ||    \
     defined HAVE_QMI_INDICATION_LOC_NMEA) &&           \
    defined HAVE_QMI_MESSAGE_LOC_REGISTER_EVENTS
    if (get_position_report_flag || get_gnss_sv_info_flag || follow_position_report_flag || follow_gnss_sv_info_flag || follow_nmea_flag) {
        /* All the remaining actions require monitoring */
        ctx->monitoring_step = MONITORING_STEP_FIRST;
        monitoring_step_run ();
        return;
    }
#endif

    /* Just client allocate/release? */
    if (noop_flag) {
        g_idle_add (noop_cb, NULL);
        return;
    }

    g_warn_if_reached ();
}

#endif /* HAVE_QMI_SERVICE_LOC */
