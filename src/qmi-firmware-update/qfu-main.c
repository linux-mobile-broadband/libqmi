/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * qmi-firmware-update -- Command line tool to update firmware in QMI devices
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
 * Copyright (C) 2016 Zodiac Inflight Innovation
 * Copyright (C) 2016 Aleksander Morgado <aleksander@aleksander.es>
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <string.h>

#include <glib.h>
#include <glib/gprintf.h>
#include <gio/gio.h>

#include <libqmi-glib.h>

#include "qfu-operation.h"
#include "qfu-udev-helpers.h"

#define PROGRAM_NAME    "qmi-firmware-update"
#define PROGRAM_VERSION PACKAGE_VERSION

/*****************************************************************************/
/* Options */

/* Generic device selections */
static guint      busnum;
static guint      devnum;
static guint16    vid;
static guint16    pid;

/* Update */
static gboolean   action_update_flag;
static gchar     *cdc_wdm_str;
static gchar     *firmware_version_str;
static gchar     *config_version_str;
static gchar     *carrier_str;
static gboolean   device_open_proxy_flag;
static gboolean   device_open_mbim_flag;

/* Reset */
static gboolean   action_reset_flag;
static gchar     *at_serial_str;

/* Update (QDL mode) */
static gboolean   action_update_qdl_flag;
static gchar     *qdl_serial_str;

/* Verify */
static gboolean   action_verify_flag;

/* Main */
static gchar    **image_strv;
static gboolean   verbose_flag;
static gboolean   silent_flag;
static gboolean   version_flag;
static gboolean   help_flag;
static gboolean   help_examples_flag;

static gboolean
parse_busnum_devnum (const gchar  *option_name,
                     const gchar  *value,
                     gpointer      data,
                     GError      **error)
{
    gchar    **strv;
    gint       busnum_idx = -1;
    gint       devnum_idx = 0;
    gulong     aux;
    gboolean   result = FALSE;

    strv = g_strsplit (value, ":", -1);
    g_assert (strv[0]);
    if (strv[1]) {
        busnum_idx = 0;
        devnum_idx = 1;
        if (strv[2]) {
            g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                         "invalid busnum-devnum string: too many fields");
            goto out;
        }
    }

    if (busnum_idx != -1) {
        aux = strtoul (strv[busnum_idx], NULL, 10);
        if (aux == 0 || aux > G_MAXUINT) {
            g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                         "invalid bus number: %s", strv[busnum_idx]);
            goto out;
        }
        busnum = (guint) aux;
    }

    aux = strtoul (strv[devnum_idx], NULL, 10);
    if (aux == 0 || aux > G_MAXUINT) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "invalid dev number: %s", strv[devnum_idx]);
        goto out;
    }
    devnum = (guint) aux;
    result = TRUE;

out:
    g_strfreev (strv);
    return result;
}

static gboolean
parse_vid_pid (const gchar  *option_name,
               const gchar  *value,
               gpointer      data,
               GError      **error)
{
    gchar    **strv;
    gint       vid_idx = 0;
    gint       pid_idx = -1;
    gulong     aux;
    gboolean   result = FALSE;

    strv = g_strsplit (value, ":", -1);
    g_assert (strv[0]);
    if (strv[1]) {
        pid_idx = 1;
        if (strv[2]) {
            g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                         "invalid vid-pid string: too many fields");
            goto out;
        }
    }

    if (pid_idx != -1) {
        aux = strtoul (strv[pid_idx], NULL, 16);
        if (aux == 0 || aux > G_MAXUINT16) {
            g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                         "invalid product id: %s", strv[pid_idx]);
            goto out;
        }
        pid = (guint) aux;
    }

    aux = strtoul (strv[vid_idx], NULL, 16);
    if (aux == 0 || aux > G_MAXUINT16) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "invalid vendor id: %s", strv[vid_idx]);
        goto out;
    }
    vid = (guint16) aux;
    result = TRUE;

out:
    g_strfreev (strv);
    return result;
}

static GOptionEntry context_selection_entries[] = {
    { "busnum-devnum", 's', 0, G_OPTION_ARG_CALLBACK, &parse_busnum_devnum,
      "Select device by bus and device number (in decimal).",
      "[BUS:]DEV"
    },
    { "vid-pid", 'd', 0, G_OPTION_ARG_CALLBACK, &parse_vid_pid,
      "Select device by device vendor and product id (in hexadecimal).",
      "VID:[PID]"
    },
    { NULL }
};

static GOptionEntry context_update_entries[] = {
    { "update", 'u', 0, G_OPTION_ARG_NONE, &action_update_flag,
      "Launch firmware update process.",
      NULL
    },
    { "cdc-wdm", 'w', 0, G_OPTION_ARG_FILENAME, &cdc_wdm_str,
      "Select device by QMI/MBIM cdc-wdm device path (e.g. /dev/cdc-wdm0).",
      "[PATH]"
    },
    { "firmware-version", 'f', 0, G_OPTION_ARG_STRING, &firmware_version_str,
      "Firmware version (e.g. '05.05.58.00').",
      "[VERSION]",
    },
    { "config-version", 'c', 0, G_OPTION_ARG_STRING, &config_version_str,
      "Config version (e.g. '005.025_002').",
      "[VERSION]",
    },
    { "carrier", 'C', 0, G_OPTION_ARG_STRING, &carrier_str,
      "Carrier name (e.g. 'Generic').",
      "[CARRIER]",
    },
    { "device-open-proxy", 'p', 0, G_OPTION_ARG_NONE, &device_open_proxy_flag,
      "Request to use the 'qmi-proxy' proxy.",
      NULL
    },
    { "device-open-mbim", 0, 0, G_OPTION_ARG_NONE, &device_open_mbim_flag,
      "Open an MBIM device with EXT_QMUX support.",
      NULL
    },
    { NULL }
};

static GOptionEntry context_reset_entries[] = {
    { "reset", 'b', 0, G_OPTION_ARG_NONE, &action_reset_flag,
      "Reset device into QDL download mode.",
      NULL
    },
    { "at-serial", 'a', 0, G_OPTION_ARG_FILENAME, &at_serial_str,
      "Select device by AT serial device path (e.g. /dev/ttyUSB2).",
      "[PATH]"
    },
    { NULL }
};

static GOptionEntry context_update_qdl_entries[] = {
    { "update-qdl", 'U', 0, G_OPTION_ARG_NONE, &action_update_qdl_flag,
      "Launch firmware update process in QDL mode.",
      NULL
    },
    { "qdl-serial", 'q', 0, G_OPTION_ARG_FILENAME, &qdl_serial_str,
      "Select device by QDL serial device path (e.g. /dev/ttyUSB0).",
      "[PATH]"
    },
    { NULL }
};

static GOptionEntry context_verify_entries[] = {
    { "verify", 'z', 0, G_OPTION_ARG_NONE, &action_verify_flag,
      "Analyze and verify firmware images.",
      NULL
    },
    { NULL }
};

static GOptionEntry context_main_entries[] = {
    { G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, &image_strv, "",
      "FILE1 FILE2..."
    },
    { "verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose_flag,
      "Run action with verbose logs, including the debug ones.",
      NULL
    },
    { "silent", 'S', 0, G_OPTION_ARG_NONE, &silent_flag,
      "Run action with no logs; not even the error/warning ones.",
      NULL
    },
    { "version", 'V', 0, G_OPTION_ARG_NONE, &version_flag,
      "Print version.",
      NULL
    },
    { "help", 'h', 0, G_OPTION_ARG_NONE, &help_flag,
      "Show help.",
      NULL
    },
    { "help-examples", 'H', 0, G_OPTION_ARG_NONE, &help_examples_flag,
      "Show help examples.",
      NULL
    },
    { NULL }
};

static const gchar *context_description =
    "   ***************************************************************************\n"
    "                                Warning!\n"
    "   ***************************************************************************\n"
    "\n"
    "   Use this program with caution. The authors take *no* responsibility if any\n"
    "   device gets broken as a result of using this program.\n"
    "\n"
    "   Please report issues to the libqmi mailing list at:\n"
    "     libqmi-devel@lists.freedesktop.org\n";

/*****************************************************************************/
/* Logging output */

static void
log_handler (const gchar    *log_domain,
             GLogLevelFlags  log_level,
             const gchar    *message,
             gpointer        user_data)
{
    const gchar *log_level_str;
    time_t       now;
    gchar        time_str[64];
    struct tm   *local_time;
    gboolean     err;

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
    case G_LOG_FLAG_FATAL:
    case G_LOG_LEVEL_ERROR:
        log_level_str = "-Error **";
        err = TRUE;
        break;

    case G_LOG_LEVEL_DEBUG:
        log_level_str = "[Debug]";
        break;

    default:
        log_level_str = "";
        break;
    }

    if (!verbose_flag && !err)
        return;

    g_fprintf (err ? stderr : stdout,
               "[%s] %s %s\n",
               time_str,
               log_level_str,
               message);
}

/*****************************************************************************/

static void
print_version (void)
{
    g_print ("\n"
             PROGRAM_NAME " " PROGRAM_VERSION "\n"
             "\n"
             "  Copyright (C) 2016 Bjørn Mork\n"
             "  Copyright (C) 2016 Zodiac Inflight Innovations\n"
             "  Copyright (C) 2016 Aleksander Morgado\n"
             "\n"
             "License GPLv2+: GNU GPL version 2 or later <http://gnu.org/licenses/gpl-2.0.html>\n"
             "This is free software: you are free to change and redistribute it.\n"
             "There is NO WARRANTY, to the extent permitted by law.\n"
             "\n");
}

static void
print_help (GOptionContext *context)
{
    gchar *str;

    /* Always print --help-all */
    str = g_option_context_get_help (context, FALSE, NULL);
    g_print ("%s", str);
    g_free (str);
}

static void
print_help_examples (void)
{
    g_print ("\n"
             "********************************************************************************\n"
             "\n"
             " Example 1: Updating a Sierra Wireless MC7354.\n"
             "\n"
             " The MC7354 is a 9x15 device which requires the firmware updater to specify the\n"
             " firmware version string, the config version string and the carrier string, so\n"
             " that they are included as identifiers of the firmware images downloaded.\n"
             "\n"
             " While in normal operation, the device will expose multiple cdc-wdm ports, and\n"
             " the updater application just needs one of those cdc-wdm ports to start the\n"
             " operation. The user can explicitly specify the cdc-wdm port to use, or\n"
             " otherwise use the generic device selection options (i.e. --busnum-devnum or\n"
             " --vid-pid) to do that automatically.\n"
             "\n"
             " Note that the firmware for the MC7354 is usually composed of a core system image\n"
             " (.cwe) and a carrier-specific image (.nvu). These two images need to be flashed\n"
             " on the same operation.\n"
             "\n"
             " 1a) An update operation specifying the QMI cdc-wdm device:\n"
             " $ sudo " PROGRAM_NAME " \\\n"
             "       --update \\\n"
             "       --cdc-wdm /dev/cdc-wdm0 \\\n"
             "       --firmware-version 05.05.58.00 \\\n"
             "       --config-version 005.025_002 \\\n"
             "       --carrier Generic \\\n"
             "       SWI9X15C_05.05.58.00.cwe \\\n"
             "       SWI9X15C_05.05.58.00_Generic_005.025_002.nvu\n"
             "\n"
             " 1b) An update operation specifying the vid:pid of the device (fails if multiple\n"
             "     devices with the same vid:pid are found):\n"
             " $ sudo " PROGRAM_NAME " \\\n"
             "       --update \\\n"
             "       -d 1199:68c0 \\\n"
             "       --firmware-version 05.05.58.00 \\\n"
             "       --config-version 005.025_002 \\\n"
             "       --carrier Generic \\\n"
             "       SWI9X15C_05.05.58.00.cwe \\\n"
             "       SWI9X15C_05.05.58.00_Generic_005.025_002.nvu\n");

    g_print ("\n"
             "********************************************************************************\n"
             "\n"
             " Example 2: Updating a Sierra Wireless MC7700.\n"
             "\n"
             " The MC7700 is a 9200 device which doesn't require the explicit firmware, config\n"
             " and carrier strings. Unlike the MC7354, which would reboot itself into QDL\n"
             " download mode once these previous strings were configured, the MC7700 requires\n"
             " an AT command to be sent in a TTY port to request the reset in QDL download\n"
             " mode.\n"
             "\n"
             " The user doesn't need to explicitly specify the path to the TTY, though, it will\n"
             " be automatically detected and processed during the firmware update process.\n"
             "\n"
             " 2a) An update operation specifying the QMI cdc-wdm device:\n"
             " $ sudo " PROGRAM_NAME " \\\n"
             "       --update \\\n"
             "       --cdc-wdm /dev/cdc-wdm0 \\\n"
             "       9999999_9999999_9200_03.05.14.00_00_generic_000.000_001_SPKG_MC.cwe\n"
             "\n"
             " 2b) An update operation specifying the vid:pid of the device (fails if multiple\n"
             "     devices with the same vid:pid are found):\n"
             " $ sudo " PROGRAM_NAME " \\\n"
             "       --update \\\n"
             "       -d 1199:68a2 \\\n"
             "       9999999_9999999_9200_03.05.14.00_00_generic_000.000_001_SPKG_MC.cwe\n");

    g_print ("\n"
             "********************************************************************************\n"
             "\n"
             " Example 3: Manual process to update a Sierra Wireless MC7700.\n"
             "\n"
             " Instead of letting the " PROGRAM_NAME " manage the full firmware update\n"
             " operation, the user can trigger the actions manually as follows:\n"
             "\n"
             " 3a) Request device to go into QDL download mode:\n"
             " $ sudo " PROGRAM_NAME " \\\n"
             "       -d 1199:68a2 \\\n"
             "       --reset\n"
             "\n"
             " 3b) Run updater operation while in QDL download mode:\n"
             " $ sudo " PROGRAM_NAME " \\\n"
             "       -d 1199:68a2 \\\n"
             "       --update-qdl \\\n"
             "       9999999_9999999_9200_03.05.14.00_00_generic_000.000_001_SPKG_MC.cwe\n");

    g_print ("\n"
             "********************************************************************************\n"
             "\n"
             " Example 4: Verify firmware images.\n"
             "\n"
             " 3a) Verify several images at once:\n"
             " $ " PROGRAM_NAME " \\\n"
             "       --verify \\\n"
             "       SWI9X15C_05.05.58.00.cwe \\\n"
             "       SWI9X15C_05.05.58.00_Generic_005.025_002.nvu\n"
             "\n"
             " 3b) Verify all .cwe, .nvu and .spk images inside a directory:\n"
             " $ find . -regex \".*\\.\\(nvu\\|spk\\|cwe\\)\" -exec " PROGRAM_NAME " -v -z {} \\;\n"
             "\n"
             " 3c) Image files may be given within .exe files; extract them with 7-Zip:\n"
             " $ 7z x SWI9200M_3.5-Release13-SWI9200X_03.05.29.03.exe\n"
             " $ ls *.{cwe,nvu,spk} 2>/dev/null\n"
             "   9999999_9999999_9200_03.05.29.03_00_generic_000.000_001_SPKG_MC.cwe\n"
             "\n");
}

/*****************************************************************************/

static gboolean
validate_inputs (const char *manual)
{
    if (manual && (vid != 0 || pid != 0)) {
        g_printerr ("error: cannot specify device path and vid:pid lookup\n");
        return FALSE;
    }

    if (manual && (busnum != 0 || devnum != 0)) {
        g_printerr ("error: cannot specify device path and busnum:devnum lookup\n");
        return FALSE;
    }

    if ((vid != 0 || pid != 0) && (busnum != 0 || devnum != 0)) {
        g_printerr ("error: cannot specify busnum:devnum and vid:pid lookups\n");
        return FALSE;
    }

    return TRUE;
}

static gchar *
select_single_path (const char              *manual,
                    QfuUdevHelperDeviceType  type)
{
    gchar  *path = NULL;
    GError *error = NULL;
    gchar  *sysfs_path = NULL;
    GList  *list = NULL;

    if (!validate_inputs (manual))
        goto out;

    if (manual) {
        path = g_strdup (manual);
        goto out;
    }

    /* lookup sysfs path */
    sysfs_path = qfu_udev_helper_find_by_device_info (vid, pid, busnum, devnum, &error);
    if (!sysfs_path) {
        g_printerr ("error: %s\n", error->message);
        g_error_free (error);
        goto out;
    }

    list = qfu_udev_helper_list_devices (type, sysfs_path);
    if (!list) {
        g_printerr ("error: no devices found in sysfs path: %s\n", sysfs_path);
        goto out;
    }

    path = g_file_get_path (G_FILE (list->data));

out:
    if (list)
        g_list_free_full (list, (GDestroyNotify) g_object_unref);

    return path;
}

static gchar **
select_multiple_paths (const char              *manual,
                       QfuUdevHelperDeviceType  type)
{
    GError  *error = NULL;
    gchar   *sysfs_path = NULL;
    GList   *list = NULL;
    GList   *l;
    gchar  **paths = NULL;
    guint    i;

    if (!validate_inputs (manual))
        goto out;

    if (manual) {
        paths = g_strsplit (manual, ",", -1);
        goto out;
    }

    /* lookup sysfs path */
    sysfs_path = qfu_udev_helper_find_by_device_info (vid, pid, busnum, devnum, &error);
    if (!sysfs_path) {
        g_printerr ("error: %s\n", error->message);
        g_error_free (error);
        goto out;
    }

    list = qfu_udev_helper_list_devices (type, sysfs_path);
    if (!list) {
        g_printerr ("error: no devices found in sysfs path: %s\n", sysfs_path);
        goto out;
    }

    paths = g_new0 (gchar *, g_list_length (list) + 1);
    for (l = list, i = 0; l; l = g_list_next (l), i++)
        paths[i] = g_file_get_path (G_FILE (l->data));

out:
    if (list)
        g_list_free_full (list, (GDestroyNotify) g_object_unref);

    return paths;
}

int main (int argc, char **argv)
{
    GError         *error = NULL;
    GOptionContext *context;
    GOptionGroup   *group;
    guint           n_actions;
    gboolean        result = FALSE;

    setlocale (LC_ALL, "");

    g_type_init ();

    /* Setup option context, process it and destroy it */
    context = g_option_context_new ("- Update firmware in QMI devices");
    g_option_context_set_description (context, context_description);

    group = g_option_group_new ("selection", "Generic device selection options", "", NULL, NULL);
    g_option_group_add_entries (group, context_selection_entries);
    g_option_context_add_group (context, group);

    group = g_option_group_new ("update", "Update options (normal mode)", "", NULL, NULL);
    g_option_group_add_entries (group, context_update_entries);
    g_option_context_add_group (context, group);

    group = g_option_group_new ("reset", "Reset options (normal mode)", "", NULL, NULL);
    g_option_group_add_entries (group, context_reset_entries);
    g_option_context_add_group (context, group);

    group = g_option_group_new ("update-qdl", "Update options (QDL mode)", "", NULL, NULL);
    g_option_group_add_entries (group, context_update_qdl_entries);
    g_option_context_add_group (context, group);

    group = g_option_group_new ("verify", "Verify options", "", NULL, NULL);
    g_option_group_add_entries (group, context_verify_entries);
    g_option_context_add_group (context, group);

    g_option_context_add_main_entries (context, context_main_entries, NULL);
    g_option_context_set_help_enabled (context, FALSE);

    if (!g_option_context_parse (context, &argc, &argv, &error)) {
        g_printerr ("error: couldn't parse option context: %s\n", error->message);
        g_error_free (error);
        goto out;
    }

    if (version_flag) {
        print_version ();
        result = TRUE;
        goto out;
    }

    if (help_flag) {
        print_help (context);
        result = TRUE;
        goto out;
    }

    if (help_examples_flag) {
        print_help_examples ();
        result = TRUE;
        goto out;
    }

    g_log_set_handler (NULL, G_LOG_LEVEL_MASK, log_handler, NULL);
    g_log_set_handler ("Qmi", G_LOG_LEVEL_MASK, log_handler, NULL);
    if (verbose_flag)
        qmi_utils_set_traces_enabled (TRUE);

    /* We don't allow multiple actions at the same time */
    n_actions = (action_verify_flag +
                 action_update_flag +
                 action_update_qdl_flag +
                 action_reset_flag);
    if (n_actions == 0) {
        g_printerr ("error: no actions specified\n");
        goto out;
    }
    if (n_actions > 1) {
        g_printerr ("error: too many actions specified\n");
        goto out;
    }

    /* A list of images must be provided for update and verify operations */
    if ((action_verify_flag || action_update_flag || action_update_qdl_flag) && !image_strv) {
        g_printerr ("error: no firmware images specified\n");
        goto out;
    }

    /* Run */

    if (action_update_flag) {
        gchar *path;

        path = select_single_path (cdc_wdm_str, QFU_UDEV_HELPER_DEVICE_TYPE_CDC_WDM);
        if (path) {
            g_debug ("using cdc-wdm device: %s", path);
            result = qfu_operation_update_run ((const gchar **) image_strv,
                                               path,
                                               firmware_version_str,
                                               config_version_str,
                                               carrier_str,
                                               device_open_proxy_flag,
                                               device_open_mbim_flag);
            g_free (path);
        }
        goto out;
    }

    if (action_update_qdl_flag) {
        gchar *path;

        path = select_single_path (qdl_serial_str, QFU_UDEV_HELPER_DEVICE_TYPE_TTY);
        if (path) {
            g_debug ("using tty device: %s", path);
            result = qfu_operation_update_qdl_run ((const gchar **) image_strv,
                                                   path);
            g_free (path);
        }
        goto out;
    }

    if (action_reset_flag) {
        gchar **paths;

        paths = select_multiple_paths (at_serial_str, QFU_UDEV_HELPER_DEVICE_TYPE_TTY);
        if (paths) {
            guint i;

            for (i = 0; paths[i]; i++)
                g_debug ("using tty device #%u: %s", i, paths[i]);
            result = qfu_operation_reset_run ((const gchar **) paths);
            g_strfreev (paths);
        }
        goto out;
    }

    if (action_verify_flag) {
        result = qfu_operation_verify_run ((const gchar **) image_strv);
        goto out;
    }

    g_assert_not_reached ();

out:
    /* Clean exit for a clean memleak report */
    if (context)
        g_option_context_free (context);

    return (result ? EXIT_SUCCESS : EXIT_FAILURE);
}
