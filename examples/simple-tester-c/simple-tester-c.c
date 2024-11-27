
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
#include <libmbim-glib.h>

static GMainLoop *loop;

static gboolean
signals_handler (void)
{
    g_main_loop_quit (loop);
    return G_SOURCE_REMOVE;
}

static void
close_ready (MbimDevice   *device,
             GAsyncResult *res)
{
    g_autoptr(GError) error = NULL;

    if (!mbim_device_close_finish (device, res, &error))
        g_printerr ("error: couldn't close: %s\n", error->message);

    g_main_loop_quit (loop);
}

static void
device_close (MbimDevice *device)
{
    mbim_device_close (device,
                       10,
                       NULL,
                       (GAsyncReadyCallback) close_ready,
                       NULL);
}

static void
query_device_caps_ready (MbimDevice   *device,
                         GAsyncResult *res)
{
    g_autoptr(GError)       error = NULL;
    g_autoptr(MbimMessage)  response = NULL;
    MbimDeviceType          device_type;
    const gchar            *device_type_str;
    MbimVoiceClass          voice_class;
    const gchar            *voice_class_str;
    MbimCellularClass       cellular_class;
    g_autofree gchar       *cellular_class_str = NULL;
    MbimSimClass            sim_class;
    g_autofree gchar       *sim_class_str = NULL;
    MbimDataClass           data_class;
    g_autofree gchar       *data_class_str = NULL;
    MbimSmsCaps             sms_caps;
    g_autofree gchar       *sms_caps_str = NULL;
    MbimCtrlCaps            ctrl_caps;
    g_autofree gchar       *ctrl_caps_str = NULL;
    guint32                 max_sessions;
    g_autofree gchar       *custom_data_class = NULL;
    g_autofree gchar       *device_id = NULL;
    g_autofree gchar       *firmware_info = NULL;
    g_autofree gchar       *hardware_info = NULL;

    response = mbim_device_command_finish (device, res, &error);
    if (!response) {
        g_printerr ("error: couldn't open the MbimDevice: %s\n",
                    error->message);
        device_close (device);
        return;
    }

    if (!mbim_message_device_caps_response_parse (
            response,
            &device_type,
            &cellular_class,
            &voice_class,
            &sim_class,
            &data_class,
            &sms_caps,
            &ctrl_caps,
            &max_sessions,
            &custom_data_class,
            &device_id,
            &firmware_info,
            &hardware_info,
            &error)) {
        g_printerr ("error: couldn't parse response message: %s\n",
                    error->message);
        device_close (device);
        return;
    }

    device_type_str    = mbim_device_type_get_string (device_type);
    cellular_class_str = mbim_cellular_class_build_string_from_mask (cellular_class);
    voice_class_str    = mbim_voice_class_get_string (voice_class);
    sim_class_str      = mbim_sim_class_build_string_from_mask (sim_class);
    data_class_str     = mbim_data_class_build_string_from_mask (data_class);
    sms_caps_str       = mbim_sms_caps_build_string_from_mask (sms_caps);
    ctrl_caps_str      = mbim_ctrl_caps_build_string_from_mask (ctrl_caps);

    g_print ("device type:          %s\n", device_type_str);
    g_print ("cellular class:       %s\n", cellular_class_str);
    g_print ("voice class:          %s\n", voice_class_str);
    g_print ("sim class:            %s\n", sim_class_str);
    g_print ("data class:           %s\n", data_class_str);
    g_print ("sms capabilities:     %s\n", sms_caps_str);
    g_print ("control capabilities: %s\n", ctrl_caps_str);
    g_print ("max sessions:         %u\n", max_sessions);
    g_print ("custom data class:    %s\n", custom_data_class);
    g_print ("device id:            %s\n", device_id);
    g_print ("firmware info:        %s\n", firmware_info);
    g_print ("hardware info:        %s\n", hardware_info);

    device_close (device);
}

static void
open_full_ready (MbimDevice   *device,
                 GAsyncResult *res)
{
    g_autoptr(GError)      error = NULL;
    g_autoptr(MbimMessage) request = NULL;

    if (!mbim_device_open_full_finish (device, res, &error)) {
        g_printerr ("error: couldn't open the MbimDevice: %s\n",
                    error->message);
        g_main_loop_quit (loop);
        return;
    }

    request = mbim_message_device_caps_query_new (NULL);
    mbim_device_command (device,
                         request,
                         10,
                         NULL,
                         (GAsyncReadyCallback)query_device_caps_ready,
                         NULL);
}

static void
new_ready (GObject      *unused,
           GAsyncResult *res)
{
    g_autoptr(MbimDevice) device = NULL;
    g_autoptr(GError)     error = NULL;

    device = mbim_device_new_finish (res, &error);
    if (!device) {
        g_printerr ("error: couldn't create MbimDevice: %s\n", error->message);
        g_main_loop_quit (loop);
        return;
    }

    /* Open the device */
    mbim_device_open_full (device,
                           MBIM_DEVICE_OPEN_FLAGS_PROXY,
                           10,
                           NULL,
                           (GAsyncReadyCallback)open_full_ready,
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
    mbim_device_new (file,
                     NULL,
                     (GAsyncReadyCallback)new_ready,
                     NULL);

    loop = g_main_loop_new (NULL, FALSE);

    g_unix_signal_add (SIGHUP,  (GSourceFunc) signals_handler, NULL);
    g_unix_signal_add (SIGTERM, (GSourceFunc) signals_handler, NULL);

    g_main_loop_run (loop);
    g_main_loop_unref (loop);
    return 0;
}
