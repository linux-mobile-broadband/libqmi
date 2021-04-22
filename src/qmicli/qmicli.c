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
 * Copyright (C) 2012-2017 Aleksander Morgado <aleksander@aleksander.es>
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <string.h>

#include <glib.h>
#include <glib/gprintf.h>
#include <gio/gio.h>
#include <glib-unix.h>

#include <libqmi-glib.h>

#if QMI_MBIM_QMUX_SUPPORTED
#include <libmbim-glib.h>
#endif

#include "qmicli.h"
#include "qmicli-helpers.h"

#define PROGRAM_NAME    "qmicli"
#define PROGRAM_VERSION PACKAGE_VERSION

/* Globals */
static GMainLoop *loop;
static GCancellable *cancellable;
static QmiDevice *device;
static QmiClient *client;
static QmiService service;
static gboolean operation_status;
static gboolean expect_indications;
#if QMI_QRTR_SUPPORTED
static QrtrBus *qrtr_bus;
#endif

/* Main options */
static gchar *device_str;
static gboolean get_service_version_info_flag;
static gchar *device_set_instance_id_str;
static gboolean device_open_version_info_flag;
static gboolean device_open_sync_flag;
static gchar *device_open_net_str;
static gboolean device_open_proxy_flag;
static gboolean device_open_qmi_flag;
static gboolean device_open_mbim_flag;
static gboolean device_open_auto_flag;
static gchar *client_cid_str;
static gboolean client_no_release_cid_flag;
static gboolean verbose_flag;
static gboolean silent_flag;
static gboolean version_flag;

static GOptionEntry main_entries[] = {
    { "device", 'd', 0, G_OPTION_ARG_STRING, &device_str,
#if QMI_QRTR_SUPPORTED
      "Specify device path or QRTR URI (e.g. qrtr://0)",
      "[PATH|URI]"
#else
      "Specify device path",
      "[PATH]"
#endif
    },
    { "get-service-version-info", 0, 0, G_OPTION_ARG_NONE, &get_service_version_info_flag,
      "Get service version info",
      NULL
    },
    { "device-set-instance-id", 0, 0, G_OPTION_ARG_STRING, &device_set_instance_id_str,
      "Set instance ID",
      "[Instance ID]"
    },
    { "device-open-version-info", 0, 0, G_OPTION_ARG_NONE, &device_open_version_info_flag,
      "Run version info check when opening device",
      NULL
    },
    { "device-open-sync", 0, 0, G_OPTION_ARG_NONE, &device_open_sync_flag,
      "Run sync operation when opening device",
      NULL
    },
    { "device-open-proxy", 'p', 0, G_OPTION_ARG_NONE, &device_open_proxy_flag,
      "Request to use the 'qmi-proxy' proxy",
      NULL
    },
    { "device-open-qmi", 0, 0, G_OPTION_ARG_NONE, &device_open_qmi_flag,
      "Open a cdc-wdm device explicitly in QMI mode",
      NULL
    },
    { "device-open-mbim", 0, 0, G_OPTION_ARG_NONE, &device_open_mbim_flag,
      "Open a cdc-wdm device explicitly in MBIM mode",
      NULL
    },
    { "device-open-auto", 0, 0, G_OPTION_ARG_NONE, &device_open_auto_flag,
      "Open a cdc-wdm device in either QMI or MBIM mode (default)",
      NULL
    },
    { "device-open-net", 0, 0, G_OPTION_ARG_STRING, &device_open_net_str,
      "Open device with specific link protocol and QoS flags",
      "[net-802-3|net-raw-ip|net-qos-header|net-no-qos-header]"
    },
    { "client-cid", 0, 0, G_OPTION_ARG_STRING, &client_cid_str,
      "Use the given CID, don't allocate a new one",
      "[CID]"
    },
    { "client-no-release-cid", 0, 0, G_OPTION_ARG_NONE, &client_no_release_cid_flag,
      "Do not release the CID when exiting",
      NULL
    },
    { "verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose_flag,
      "Run action with verbose logs, including the debug ones",
      NULL
    },
    { "silent", 0, 0, G_OPTION_ARG_NONE, &silent_flag,
      "Run action with no logs; not even the error/warning ones",
      NULL
    },
    { "version", 'V', 0, G_OPTION_ARG_NONE, &version_flag,
      "Print version",
      NULL
    },
    { NULL }
};

static gboolean
signals_handler (void)
{
    if (cancellable) {
        /* Ignore consecutive requests of cancellation */
        if (!g_cancellable_is_cancelled (cancellable)) {
            g_printerr ("cancelling the operation...\n");
            g_cancellable_cancel (cancellable);
            /* Re-set the signal handler to allow main loop cancellation on
             * second signal */
            return G_SOURCE_CONTINUE;
        }
    }

    if (loop && g_main_loop_is_running (loop)) {
        g_printerr ("cancelling the main loop...\n");
        g_idle_add ((GSourceFunc) g_main_loop_quit, loop);
    }
    return G_SOURCE_REMOVE;
}

static void
log_handler (const gchar *log_domain,
             GLogLevelFlags log_level,
             const gchar *message,
             gpointer user_data)
{
    const gchar *log_level_str;
    time_t now;
    gchar time_str[64];
    struct tm *local_time;
    gboolean err;

    /* Nothing to do if we're silent */
    if (silent_flag)
        return;

    now = time ((time_t *) NULL);
    local_time = localtime (&now);
    strftime (time_str, 64, "%d %b %Y, %H:%M:%S", local_time);
    err = FALSE;

    switch (log_level) {
    case G_LOG_LEVEL_WARNING:
        log_level_str = "-Warning **";
        err = TRUE;
        break;

    case G_LOG_LEVEL_CRITICAL:
    case G_LOG_LEVEL_ERROR:
        log_level_str = "-Error **";
        err = TRUE;
        break;

    case G_LOG_LEVEL_DEBUG:
        log_level_str = "[Debug]";
        break;

    case G_LOG_LEVEL_MESSAGE:
    case G_LOG_LEVEL_INFO:
        log_level_str = "";
        break;

    case G_LOG_FLAG_FATAL:
    case G_LOG_LEVEL_MASK:
    case G_LOG_FLAG_RECURSION:
    default:
        g_assert_not_reached ();
    }

    if (!verbose_flag && !err)
        return;

    g_fprintf (err ? stderr : stdout,
               "[%s] %s %s\n",
               time_str,
               log_level_str,
               message);
}

G_GNUC_NORETURN
static void
print_version_and_exit (void)
{
    g_print (PROGRAM_NAME " " PROGRAM_VERSION "\n"
             "Copyright (C) 2012-2021 Aleksander Morgado\n"
             "License GPLv2+: GNU GPL version 2 or later <http://gnu.org/licenses/gpl-2.0.html>\n"
             "This is free software: you are free to change and redistribute it.\n"
             "There is NO WARRANTY, to the extent permitted by law.\n"
             "\n");
    exit (EXIT_SUCCESS);
}

static gboolean
generic_options_enabled (void)
{
    static guint n_actions = 0;
    static gboolean checked = FALSE;

    if (checked)
        return !!n_actions;

    n_actions = (!!device_set_instance_id_str +
                 get_service_version_info_flag);

    if (n_actions > 1) {
        g_printerr ("error: too many generic actions requested\n");
        exit (EXIT_FAILURE);
    }

    checked = TRUE;
    return !!n_actions;
}

/*****************************************************************************/
/* Report that indications are expected */

void
qmicli_expect_indications (void)
{
    expect_indications = TRUE;
}

/*****************************************************************************/
/* Running asynchronously */

static void
close_ready (QmiDevice    *dev,
             GAsyncResult *res)
{
    GError *error = NULL;

    if (!qmi_device_close_finish (dev, res, &error)) {
        g_printerr ("error: couldn't close: %s\n", error->message);
        g_error_free (error);
    } else
        g_debug ("Closed");

    g_main_loop_quit (loop);
}

static void
release_client_ready (QmiDevice *dev,
                      GAsyncResult *res)
{
    GError *error = NULL;

    if (!qmi_device_release_client_finish (dev, res, &error)) {
        g_printerr ("error: couldn't release client: %s\n", error->message);
        g_error_free (error);
    } else
        g_debug ("Client released");

    qmi_device_close_async (dev, 10, NULL, (GAsyncReadyCallback) close_ready, NULL);
}

void
qmicli_async_operation_done (gboolean reported_operation_status,
                             gboolean skip_cid_release)
{
    QmiDeviceReleaseClientFlags flags = QMI_DEVICE_RELEASE_CLIENT_FLAGS_NONE;

    /* Keep the result of the operation */
    operation_status = reported_operation_status;

    /* Cleanup cancellation */
    g_clear_object (&cancellable);

    /* If no client was allocated (e.g. generic action), just quit */
    if (!client) {
        g_main_loop_quit (loop);
        return;
    }

    if (skip_cid_release)
        g_debug ("Skipped CID release");
    else if (!client_no_release_cid_flag)
        flags |= QMI_DEVICE_RELEASE_CLIENT_FLAGS_RELEASE_CID;
    else
        g_print ("[%s] Client ID not released:\n"
                 "\tService: '%s'\n"
                 "\t    CID: '%u'\n",
                 qmi_device_get_path_display (device),
                 qmi_service_get_string (service),
                 qmi_client_get_cid (client));

    qmi_device_release_client (device,
                               client,
                               flags,
                               10,
                               NULL,
                               (GAsyncReadyCallback)release_client_ready,
                               NULL);
}

static void
allocate_client_ready (QmiDevice *dev,
                       GAsyncResult *res)
{
    GError *error = NULL;

    client = qmi_device_allocate_client_finish (dev, res, &error);
    if (!client) {
        g_printerr ("error: couldn't create client for the '%s' service: %s\n",
                    qmi_service_get_string (service),
                    error->message);
        exit (EXIT_FAILURE);
    }

    /* Run the service-specific action */
    switch (service) {
    case QMI_SERVICE_DMS:
#if defined HAVE_QMI_SERVICE_DMS
        qmicli_dms_run (dev, QMI_CLIENT_DMS (client), cancellable);
        return;
#else
        break;
#endif
    case QMI_SERVICE_NAS:
#if defined HAVE_QMI_SERVICE_NAS
        qmicli_nas_run (dev, QMI_CLIENT_NAS (client), cancellable);
        return;
#else
        break;
#endif
    case QMI_SERVICE_WDS:
#if defined HAVE_QMI_SERVICE_WDS
        qmicli_wds_run (dev, QMI_CLIENT_WDS (client), cancellable);
        return;
#else
        break;
#endif
    case QMI_SERVICE_PBM:
#if defined HAVE_QMI_SERVICE_PBM
        qmicli_pbm_run (dev, QMI_CLIENT_PBM (client), cancellable);
        return;
#else
        break;
#endif
    case QMI_SERVICE_PDC:
#if defined HAVE_QMI_SERVICE_PDC
        qmicli_pdc_run (dev, QMI_CLIENT_PDC (client), cancellable);
        return;
#else
        break;
#endif
    case QMI_SERVICE_UIM:
#if defined HAVE_QMI_SERVICE_UIM
        qmicli_uim_run (dev, QMI_CLIENT_UIM (client), cancellable);
        return;
#else
        break;
#endif
    case QMI_SERVICE_WMS:
#if defined HAVE_QMI_SERVICE_WMS
        qmicli_wms_run (dev, QMI_CLIENT_WMS (client), cancellable);
        return;
#else
        break;
#endif
    case QMI_SERVICE_WDA:
#if defined HAVE_QMI_SERVICE_WDA
        qmicli_wda_run (dev, QMI_CLIENT_WDA (client), cancellable);
        return;
#else
        break;
#endif
    case QMI_SERVICE_VOICE:
#if defined HAVE_QMI_SERVICE_VOICE
        qmicli_voice_run (dev, QMI_CLIENT_VOICE (client), cancellable);
        return;
#else
        break;
#endif
    case QMI_SERVICE_LOC:
#if defined HAVE_QMI_SERVICE_LOC
        qmicli_loc_run (dev, QMI_CLIENT_LOC (client), cancellable);
        return;
#else
        break;
#endif
    case QMI_SERVICE_QOS:
#if defined HAVE_QMI_SERVICE_QOS
        qmicli_qos_run (dev, QMI_CLIENT_QOS (client), cancellable);
        return;
#else
        break;
#endif
    case QMI_SERVICE_GAS:
#if defined HAVE_QMI_SERVICE_GAS
        qmicli_gas_run (dev, QMI_CLIENT_GAS (client), cancellable);
        return;
#else
        break;
#endif
    case QMI_SERVICE_GMS:
#if defined HAVE_QMI_SERVICE_GMS
        qmicli_gms_run (dev, QMI_CLIENT_GMS (client), cancellable);
        return;
#else
        break;
#endif
    case QMI_SERVICE_DSD:
#if defined HAVE_QMI_SERVICE_DSD
        qmicli_dsd_run (dev, QMI_CLIENT_DSD (client), cancellable);
        return;
#else
        break;
#endif
    case QMI_SERVICE_SAR:
#if defined HAVE_QMI_SERVICE_SAR
        qmicli_sar_run (dev, QMI_CLIENT_SAR (client), cancellable);
        return;
#else
        break;
#endif
    case QMI_SERVICE_DPM:
#if defined HAVE_QMI_SERVICE_DPM
        qmicli_dpm_run (dev, QMI_CLIENT_DPM (client), cancellable);
        return;
#else
        break;
#endif
    case QMI_SERVICE_UNKNOWN:
    case QMI_SERVICE_CTL:
    case QMI_SERVICE_AUTH:
    case QMI_SERVICE_AT:
    case QMI_SERVICE_CAT2:
    case QMI_SERVICE_QCHAT:
    case QMI_SERVICE_RMTFS:
    case QMI_SERVICE_TEST:
    case QMI_SERVICE_IMS:
    case QMI_SERVICE_ADC:
    case QMI_SERVICE_CSD:
    case QMI_SERVICE_MFS:
    case QMI_SERVICE_TIME:
    case QMI_SERVICE_TS:
    case QMI_SERVICE_TMD:
    case QMI_SERVICE_SAP:
    case QMI_SERVICE_TSYNC:
    case QMI_SERVICE_RFSA:
    case QMI_SERVICE_CSVT:
    case QMI_SERVICE_QCMAP:
    case QMI_SERVICE_IMSP:
    case QMI_SERVICE_IMSVT:
    case QMI_SERVICE_IMSA:
    case QMI_SERVICE_COEX:
    case QMI_SERVICE_STX:
    case QMI_SERVICE_BIT:
    case QMI_SERVICE_IMSRTP:
    case QMI_SERVICE_RFRPE:
    case QMI_SERVICE_SSCTL:
    case QMI_SERVICE_CAT:
    case QMI_SERVICE_RMS:
    case QMI_SERVICE_FOTA:
    case QMI_SERVICE_PDS:
    case QMI_SERVICE_OMA:
    default:
        break;
    }
    g_assert_not_reached ();
}

static void
device_allocate_client (QmiDevice *dev)
{
    guint8 cid = QMI_CID_NONE;

    if (client_cid_str) {
        guint32 cid32;

        cid32 = atoi (client_cid_str);
        if (!cid32 || cid32 > G_MAXUINT8) {
            g_printerr ("error: invalid CID given '%s'\n",
                        client_cid_str);
            exit (EXIT_FAILURE);
        }

        cid = (guint8)cid32;
        g_debug ("Reusing CID '%u'", cid);
    }

    /* As soon as we get the QmiDevice, create a client for the requested
     * service */
    qmi_device_allocate_client (dev,
                                service,
                                cid,
                                10,
                                cancellable,
                                (GAsyncReadyCallback)allocate_client_ready,
                                NULL);
}

static void
set_instance_id_ready (QmiDevice *dev,
                       GAsyncResult *res)
{
    GError *error = NULL;
    guint16 link_id;

    if (!qmi_device_set_instance_id_finish (dev, res, &link_id, &error)) {
        g_printerr ("error: couldn't set instance ID: %s\n",
                    error->message);
        exit (EXIT_FAILURE);
    }

    g_print ("[%s] Instance ID set:\n"
             "\tLink ID: '%" G_GUINT16_FORMAT "'\n",
             qmi_device_get_path_display (dev),
             link_id);

    /* We're done now */
    qmicli_async_operation_done (TRUE, FALSE);
}

static void
device_set_instance_id (QmiDevice *dev)
{
    gint instance_id;

    if (g_str_equal (device_set_instance_id_str, "0"))
        instance_id = 0;
    else {
        instance_id = atoi (device_set_instance_id_str);
        if (instance_id == 0) {
            g_printerr ("error: invalid instance ID given: '%s'\n", device_set_instance_id_str);
            exit (EXIT_FAILURE);
        } else if (instance_id < 0 || instance_id > G_MAXUINT8) {
            g_printerr ("error: given instance ID is out of range [0,%u]: '%s'\n",
                        G_MAXUINT8,
                        device_set_instance_id_str);
            exit (EXIT_FAILURE);
        }
    }

    g_debug ("Setting instance ID '%d'...", instance_id);
    qmi_device_set_instance_id (dev,
                                (guint8)instance_id,
                                10,
                                cancellable,
                                (GAsyncReadyCallback)set_instance_id_ready,
                                NULL);
}

static void
get_service_version_info_ready (QmiDevice *dev,
                                GAsyncResult *res)
{
    GError *error = NULL;
    GArray *services;
    guint i;

    services = qmi_device_get_service_version_info_finish (dev, res, &error);
    if (!services) {
        g_printerr ("error: couldn't get service version info: %s\n",
                    error->message);
        exit (EXIT_FAILURE);
    }

    g_print ("[%s] Supported versions:\n",
             qmi_device_get_path_display (dev));
    for (i = 0; i < services->len; i++) {
        QmiDeviceServiceVersionInfo *info;
        const gchar *service_str;

        info = &g_array_index (services, QmiDeviceServiceVersionInfo, i);
        service_str = qmi_service_get_string (info->service);
        if (service_str)
            g_print ("\t%s (%u.%u)\n",
                     service_str,
                     info->major_version,
                     info->minor_version);
        else
            g_print ("\tunknown [0x%02x] (%u.%u)\n",
                     info->service,
                     info->major_version,
                     info->minor_version);
    }
    g_array_unref (services);

    /* We're done now */
    qmicli_async_operation_done (TRUE, FALSE);
}

static void
device_get_service_version_info (QmiDevice *dev)
{
    g_debug ("Getting service version info...");
    qmi_device_get_service_version_info (dev,
                                         10,
                                         cancellable,
                                         (GAsyncReadyCallback)get_service_version_info_ready,
                                         NULL);
}

static void
device_open_ready (QmiDevice *dev,
                   GAsyncResult *res)
{
    GError *error = NULL;

    if (!qmi_device_open_finish (dev, res, &error)) {
        g_printerr ("error: couldn't open the QmiDevice: %s\n",
                    error->message);
        exit (EXIT_FAILURE);
    }

    g_debug ("QMI Device at '%s' ready",
             qmi_device_get_path_display (dev));

    if (device_set_instance_id_str)
        device_set_instance_id (dev);
    else if (get_service_version_info_flag)
        device_get_service_version_info (dev);
    else if (qmicli_link_management_options_enabled ())
        qmicli_link_management_run (dev, cancellable);
    else if (qmicli_qmiwwan_options_enabled ())
        qmicli_qmiwwan_run (dev, cancellable);
    else
        device_allocate_client (dev);
}

static void
device_new_ready (GObject *unused,
                  GAsyncResult *res)
{
    QmiDeviceOpenFlags open_flags = QMI_DEVICE_OPEN_FLAGS_NONE;
    GError *error = NULL;

    device = qmi_device_new_finish (res, &error);
    if (!device) {
        g_printerr ("error: couldn't create QmiDevice: %s\n",
                    error->message);
        exit (EXIT_FAILURE);
    }

    if (device_open_mbim_flag + device_open_qmi_flag + device_open_auto_flag > 1) {
        g_printerr ("error: cannot specify multiple mode flags to open device\n");
        exit (EXIT_FAILURE);
    }

    /* Setup device open flags */
    if (device_open_version_info_flag)
        open_flags |= QMI_DEVICE_OPEN_FLAGS_VERSION_INFO;
    if (device_open_sync_flag)
        open_flags |= QMI_DEVICE_OPEN_FLAGS_SYNC;
    if (device_open_proxy_flag)
        open_flags |= QMI_DEVICE_OPEN_FLAGS_PROXY;
    if (device_open_mbim_flag)
        open_flags |= QMI_DEVICE_OPEN_FLAGS_MBIM;
    if (device_open_auto_flag || (!device_open_qmi_flag && !device_open_mbim_flag))
        open_flags |= QMI_DEVICE_OPEN_FLAGS_AUTO;
    if (expect_indications)
        open_flags |= QMI_DEVICE_OPEN_FLAGS_EXPECT_INDICATIONS;
    if (device_open_net_str) {
        if (!qmicli_read_device_open_flags_from_string (device_open_net_str, &open_flags) ||
            !qmicli_validate_device_open_flags (open_flags))
            exit (EXIT_FAILURE);
    }

    /* Open the device */
    qmi_device_open (device,
                     open_flags,
                     15,
                     cancellable,
                     (GAsyncReadyCallback)device_open_ready,
                     NULL);
}

#if QMI_QRTR_SUPPORTED

static void
bus_new_ready (GObject      *source,
               GAsyncResult *res,
               gpointer      user_data)
{
    g_autoptr(GError)  error = NULL;
    guint              node_id;
    QrtrNode          *node;

    node_id = GPOINTER_TO_UINT (user_data);

    qrtr_bus = qrtr_bus_new_finish (res, &error);
    if (!qrtr_bus) {
        g_printerr ("error: couldn't access QRTR bus: %s\n", error->message);
        exit (EXIT_FAILURE);
    }

    node = qrtr_bus_peek_node (qrtr_bus, node_id);
    if (!node) {
        g_printerr ("error: node with id %u not found in QRTR bus\n", node_id);
        exit (EXIT_FAILURE);
    }

    qmi_device_new_from_node (node,
                              cancellable,
                              (GAsyncReadyCallback)device_new_ready,
                              NULL);
}

#endif

static gboolean
make_device (GFile *file)
{
    g_autofree gchar *id = NULL;

    id = g_file_get_path (file);
    if (id) {
        /* Describes a local device file. */
        qmi_device_new (file,
                        cancellable,
                        (GAsyncReadyCallback)device_new_ready,
                        NULL);
        return TRUE;
    }

#if QMI_QRTR_SUPPORTED
    {
        guint32 node_id;

        id = g_file_get_uri (file);
        if (qrtr_get_node_for_uri (id, &node_id)) {
            qrtr_bus_new (1000, /* ms */
                          cancellable,
                          (GAsyncReadyCallback)bus_new_ready,
                          GUINT_TO_POINTER (node_id));
            return TRUE;
        }

        g_printerr ("error: URI is neither a local file path nor a QRTR node: %s\n", id);
        return FALSE;
    }
#else
    g_printerr ("error: URI is not a local file path: %s\n", id);
    return FALSE;
#endif
}

/*****************************************************************************/

static void
parse_actions (void)
{
    guint actions_enabled = 0;

    if (generic_options_enabled ()) {
        service = QMI_SERVICE_CTL;
        actions_enabled++;
    }

    if (qmicli_link_management_options_enabled ()) {
        service = QMI_SERVICE_UNKNOWN;
        actions_enabled++;
    }

    if (qmicli_qmiwwan_options_enabled ()) {
        service = QMI_SERVICE_UNKNOWN;
        actions_enabled++;
    }

#if defined HAVE_QMI_SERVICE_DMS
    if (qmicli_dms_options_enabled ()) {
        service = QMI_SERVICE_DMS;
        actions_enabled++;
    }
#endif

#if defined HAVE_QMI_SERVICE_NAS
    if (qmicli_nas_options_enabled ()) {
        service = QMI_SERVICE_NAS;
        actions_enabled++;
    }
#endif

#if defined HAVE_QMI_SERVICE_WDS
    if (qmicli_wds_options_enabled ()) {
        service = QMI_SERVICE_WDS;
        actions_enabled++;
    }
#endif

#if defined HAVE_QMI_SERVICE_PBM
    if (qmicli_pbm_options_enabled ()) {
        service = QMI_SERVICE_PBM;
        actions_enabled++;
    }
#endif

#if defined HAVE_QMI_SERVICE_PDC
    if (qmicli_pdc_options_enabled ()) {
        service = QMI_SERVICE_PDC;
        actions_enabled++;
    }
#endif

#if defined HAVE_QMI_SERVICE_UIM
    if (qmicli_uim_options_enabled ()) {
        service = QMI_SERVICE_UIM;
        actions_enabled++;
    }
#endif

#if defined HAVE_QMI_SERVICE_SAR
    if (qmicli_sar_options_enabled ()) {
        service = QMI_SERVICE_SAR;
        actions_enabled++;
    }
#endif

#if defined HAVE_QMI_SERVICE_WMS
    if (qmicli_wms_options_enabled ()) {
        service = QMI_SERVICE_WMS;
        actions_enabled++;
    }
#endif

#if defined HAVE_QMI_SERVICE_WDA
    if (qmicli_wda_options_enabled ()) {
        service = QMI_SERVICE_WDA;
        actions_enabled++;
    }
#endif

#if defined HAVE_QMI_SERVICE_VOICE
    if (qmicli_voice_options_enabled ()) {
        service = QMI_SERVICE_VOICE;
        actions_enabled++;
    }
#endif

#if defined HAVE_QMI_SERVICE_LOC
    if (qmicli_loc_options_enabled ()) {
        service = QMI_SERVICE_LOC;
        actions_enabled++;
    }
#endif

#if defined HAVE_QMI_SERVICE_QOS
    if (qmicli_qos_options_enabled ()) {
        service = QMI_SERVICE_QOS;
        actions_enabled++;
    }
#endif

#if defined HAVE_QMI_SERVICE_GAS
    if (qmicli_gas_options_enabled ()) {
        service = QMI_SERVICE_GAS;
        actions_enabled++;
    }
#endif

#if defined HAVE_QMI_SERVICE_GMS
    if (qmicli_gms_options_enabled ()) {
        service = QMI_SERVICE_GMS;
        actions_enabled++;
    }
#endif

#if defined HAVE_QMI_SERVICE_DSD
    if (qmicli_dsd_options_enabled ()) {
        service = QMI_SERVICE_DSD;
        actions_enabled++;
    }
#endif

#if defined HAVE_QMI_SERVICE_DPM
    if (qmicli_dpm_options_enabled ()) {
        service = QMI_SERVICE_DPM;
        actions_enabled++;
    }
#endif

    /* Cannot mix actions from different services */
    if (actions_enabled > 1) {
        g_printerr ("error: cannot execute multiple actions of different services\n");
        exit (EXIT_FAILURE);
    }

    /* No options? */
    if (actions_enabled == 0) {
        g_printerr ("error: no actions specified\n");
        exit (EXIT_FAILURE);
    }

    /* Go on! */
}

int main (int argc, char **argv)
{
    GError *error = NULL;
    GFile *file;
    GOptionContext *context;

    setlocale (LC_ALL, "");

    /* Setup option context, process it and destroy it */
    context = g_option_context_new ("- Control QMI devices");
#if defined HAVE_QMI_SERVICE_DMS
    g_option_context_add_group (context, qmicli_dms_get_option_group ());
#endif
#if defined HAVE_QMI_SERVICE_NAS
    g_option_context_add_group (context, qmicli_nas_get_option_group ());
#endif
#if defined HAVE_QMI_SERVICE_WDS
    g_option_context_add_group (context, qmicli_wds_get_option_group ());
#endif
#if defined HAVE_QMI_SERVICE_PBM
    g_option_context_add_group (context, qmicli_pbm_get_option_group ());
#endif
#if defined HAVE_QMI_SERVICE_PDC
    g_option_context_add_group (context, qmicli_pdc_get_option_group ());
#endif
#if defined HAVE_QMI_SERVICE_UIM
    g_option_context_add_group (context, qmicli_uim_get_option_group ());
#endif
#if defined HAVE_QMI_SERVICE_SAR
    g_option_context_add_group (context, qmicli_sar_get_option_group ());
#endif
#if defined HAVE_QMI_SERVICE_WMS
    g_option_context_add_group (context, qmicli_wms_get_option_group ());
#endif
#if defined HAVE_QMI_SERVICE_WDA
    g_option_context_add_group (context, qmicli_wda_get_option_group ());
#endif
#if defined HAVE_QMI_SERVICE_VOICE
    g_option_context_add_group (context, qmicli_voice_get_option_group ());
#endif
#if defined HAVE_QMI_SERVICE_LOC
    g_option_context_add_group (context, qmicli_loc_get_option_group ());
#endif
#if defined HAVE_QMI_SERVICE_QOS
    g_option_context_add_group (context, qmicli_qos_get_option_group ());
#endif
#if defined HAVE_QMI_SERVICE_GAS
    g_option_context_add_group (context, qmicli_gas_get_option_group ());
#endif
#if defined HAVE_QMI_SERVICE_GMS
    g_option_context_add_group (context, qmicli_gms_get_option_group ());
#endif
#if defined HAVE_QMI_SERVICE_DSD
    g_option_context_add_group (context, qmicli_dsd_get_option_group ());
#endif
#if defined HAVE_QMI_SERVICE_DPM
    g_option_context_add_group (context, qmicli_dpm_get_option_group ());
#endif
    g_option_context_add_group (context, qmicli_link_management_get_option_group ());
    g_option_context_add_group (context, qmicli_qmiwwan_get_option_group ());
    g_option_context_add_main_entries (context, main_entries, NULL);
    if (!g_option_context_parse (context, &argc, &argv, &error)) {
        g_printerr ("error: %s\n",
                    error->message);
        exit (EXIT_FAILURE);
    }
    g_option_context_free (context);

    if (version_flag)
        print_version_and_exit ();

    g_log_set_handler (NULL, G_LOG_LEVEL_MASK, log_handler, NULL);
    g_log_set_handler ("Qmi", G_LOG_LEVEL_MASK, log_handler, NULL);
    if (verbose_flag)
        qmi_utils_set_traces_enabled (TRUE);

#if QMI_MBIM_QMUX_SUPPORTED
    /* libmbim logging */
    g_log_set_handler ("Mbim", G_LOG_LEVEL_MASK, log_handler, NULL);
    if (verbose_flag)
        mbim_utils_set_traces_enabled (TRUE);
#endif

#if QMI_QRTR_SUPPORTED
    /* libqrtr-glib logging */
    g_log_set_handler ("Qrtr", G_LOG_LEVEL_MASK, log_handler, NULL);
#endif

    /* No device path given? */
    if (!device_str) {
        g_printerr ("error: no device path specified\n");
        exit (EXIT_FAILURE);
    }

    /* Build new GFile from the commandline arg */
    file = g_file_new_for_commandline_arg (device_str);

    parse_actions ();

    /* Create requirements for async options */
    cancellable = g_cancellable_new ();
    loop = g_main_loop_new (NULL, FALSE);

    /* Setup signals */
    g_unix_signal_add (SIGINT,  (GSourceFunc) signals_handler, NULL);
    g_unix_signal_add (SIGHUP,  (GSourceFunc) signals_handler, NULL);
    g_unix_signal_add (SIGTERM, (GSourceFunc) signals_handler, NULL);

    /* Launch QmiDevice creation */
    if (!make_device (file))
        return EXIT_FAILURE;

    g_main_loop_run (loop);

    if (cancellable)
        g_object_unref (cancellable);
    if (client)
        g_object_unref (client);
    if (device)
        g_object_unref (device);
    g_main_loop_unref (loop);
    g_object_unref (file);

    return (operation_status ? EXIT_SUCCESS : EXIT_FAILURE);
}
