/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * Simple tester
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
 * Copyright (c) 2024 Aleksander Morgado <aleksander@aleksander.es>
 */

#include <glib.h>
#include <glib-unix.h>
#include <libqmi-glib.h>

static GMainLoop *loop;
static QmiDevice *device;

static gboolean
signals_handler (void)
{
    g_main_loop_quit (loop);
    return G_SOURCE_REMOVE;
}

static void
close_ready (QmiDevice    *dev,
             GAsyncResult *res)
{
    g_autoptr(GError) error = NULL;

    if (!qmi_device_close_finish (dev, res, &error))
        g_printerr ("error: couldn't close: %s\n", error->message);

    g_main_loop_quit (loop);
}


static void
device_close (void)
{
    qmi_device_close_async (device,
                            10,
                            NULL,
                            (GAsyncReadyCallback) close_ready,
                            NULL);
}

static void
release_client_ready (QmiDevice    *dev,
                      GAsyncResult *res)
{
    g_autoptr(GError) error = NULL;

    if (!qmi_device_release_client_finish (dev, res, &error))
        g_printerr ("error: couldn't release client: %s\n", error->message);

    device_close ();
}

static void
release_client (QmiClientDms *client)
{
    qmi_device_release_client (device,
                               QMI_CLIENT (client),
                               QMI_DEVICE_RELEASE_CLIENT_FLAGS_RELEASE_CID,
                               10,
                               NULL,
                               (GAsyncReadyCallback)release_client_ready,
                               NULL);
}

static void
get_ids_ready (QmiClientDms *client,
               GAsyncResult *res)
{
    g_autoptr(QmiMessageDmsGetIdsOutput) output = NULL;
    g_autoptr(GError)  error = NULL;
    const gchar       *esn = NULL;
    const gchar       *imei = NULL;
    const gchar       *meid = NULL;
    const gchar       *imei_software_version = NULL;

    output = qmi_client_dms_get_ids_finish (client, res, &error);
    if (!output || !qmi_message_dms_get_ids_output_get_result (output, &error)) {
        g_printerr ("error: couldn't query device ids: %s\n", error->message);
        release_client (client);
        return;
    }

    if (qmi_message_dms_get_ids_output_get_imei (output, &imei, NULL))
        g_print("imei:                  %s\n", imei);

    if (qmi_message_dms_get_ids_output_get_imei_software_version (output,
                                                                  &imei_software_version,
                                                                  NULL))
        g_print("imei software version: %s\n", imei_software_version);

    if (qmi_message_dms_get_ids_output_get_meid (output, &meid, NULL))
        g_print("meid:                  %s\n", meid);

    if (qmi_message_dms_get_ids_output_get_esn  (output, &esn, NULL))
        g_print("esn:                   %s\n", esn);

    release_client (client);
}

static void
get_capabilities_ready (QmiClientDms *client,
                        GAsyncResult *res)
{
    g_autoptr(QmiMessageDmsGetCapabilitiesOutput) output = NULL;
    g_autoptr(GError)           error = NULL;
    guint32                     max_tx_channel_rate;
    guint32                     max_rx_channel_rate;
    QmiDmsDataServiceCapability data_service_capability;
    QmiDmsSimCapability         sim_capability;
    GArray                     *radio_interface_list;
    g_autoptr(GString)          networks = NULL;
    guint                       i;

    output = qmi_client_dms_get_capabilities_finish (client, res, &error);
    if (!output || !qmi_message_dms_get_capabilities_output_get_result (output, &error)) {
        g_printerr ("error: couldn't query device capabilities: %s\n", error->message);
        release_client (client);
        return;
    }

    qmi_message_dms_get_capabilities_output_get_info (output,
                                                      &max_tx_channel_rate,
                                                      &max_rx_channel_rate,
                                                      &data_service_capability,
                                                      &sim_capability,
                                                      &radio_interface_list,
                                                      NULL);

    g_print ("max tx channel rate:   %u\n", max_tx_channel_rate);
    g_print ("max rx channel rate:   %u\n", max_rx_channel_rate);
    g_print ("data service:          %s\n", qmi_dms_data_service_capability_get_string (data_service_capability));
    g_print ("sim:                   %s\n", qmi_dms_sim_capability_get_string (sim_capability));

    networks = g_string_new ("");
    for (i = 0; i < radio_interface_list->len; i++) {
        if (i != 0)
            g_string_append (networks, ", ");
        g_string_append (networks,
                         qmi_dms_radio_interface_get_string (g_array_index (radio_interface_list,
                                                                            QmiDmsRadioInterface,
                                                                            i)));
    }
    g_print ("networks:              %s\n", networks->str);

    qmi_client_dms_get_ids (client,
                            NULL,
                            10,
                            NULL,
                            (GAsyncReadyCallback)get_ids_ready,
                            NULL);
}

static void
allocate_client_ready (QmiDevice    *dev,
                       GAsyncResult *res)
{
    g_autoptr(GError)    error = NULL;
    g_autoptr(QmiClient) client = NULL;

    client = qmi_device_allocate_client_finish (dev, res, &error);
    if (!client) {
        g_printerr ("error: couldn't allocate QMI client: %s\n",
                    error->message);
        device_close ();
        return;
    }

    qmi_client_dms_get_capabilities (QMI_CLIENT_DMS (client),
                                     NULL,
                                     10,
                                     NULL,
                                     (GAsyncReadyCallback)get_capabilities_ready,
                                     NULL);
}

static void
open_ready (QmiDevice    *dev,
            GAsyncResult *res)
{
    g_autoptr(GError) error = NULL;

    if (!qmi_device_open_finish (dev, res, &error)) {
        g_printerr ("error: couldn't open the QmiDevice: %s\n",
                    error->message);
        g_main_loop_quit (loop);
        return;
    }

    qmi_device_allocate_client (device,
                                QMI_SERVICE_DMS,
                                QMI_CID_NONE,
                                10,
                                NULL,
                                (GAsyncReadyCallback)allocate_client_ready,
                                NULL);
}

static void
new_ready (GObject      *unused,
           GAsyncResult *res)
{
    g_autoptr(GError) error = NULL;

    device = qmi_device_new_finish (res, &error);
    if (!device) {
        g_printerr ("error: couldn't create QmiDevice: %s\n", error->message);
        g_main_loop_quit (loop);
        return;
    }

    /* Open the device */
    qmi_device_open (device,
                     QMI_DEVICE_OPEN_FLAGS_PROXY | QMI_DEVICE_OPEN_FLAGS_AUTO,
                     10,
                     NULL,
                     (GAsyncReadyCallback)open_ready,
                     NULL);
}

int main (int argc, char **argv)
{
    g_autoptr(GFile) file = NULL;

    if (argc != 2) {
        g_printerr ("error: wrong number of arguments\n");
        g_printerr ("usage: simple-tester-c <DEVICE>\n");
        exit (1);
    }

    file = g_file_new_for_path (argv[1]);
    qmi_device_new (file,
                    NULL,
                    (GAsyncReadyCallback)new_ready,
                    NULL);

    loop = g_main_loop_new (NULL, FALSE);

    g_unix_signal_add (SIGHUP,  (GSourceFunc) signals_handler, NULL);
    g_unix_signal_add (SIGTERM, (GSourceFunc) signals_handler, NULL);

    g_main_loop_run (loop);

    g_clear_object (&device);
    g_main_loop_unref (loop);
    return 0;
}
