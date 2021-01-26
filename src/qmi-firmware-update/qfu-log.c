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
 * Copyright (C) 2016-2017 Zodiac Inflight Innovations
 * Copyright (C) 2016-2017 Aleksander Morgado <aleksander@aleksander.es>
 */

#include <config.h>
#include <errno.h>

#include <glib.h>
#include <glib/gprintf.h>

#include <libqmi-glib.h>

#if QMI_MBIM_QMUX_SUPPORTED
#include <libmbim-glib.h>
#endif

#include "qfu-log.h"

static gboolean  stdout_silent_flag;
static gboolean  stdout_verbose_flag;
static FILE     *verbose_log_file;

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
    if (stdout_silent_flag && !verbose_log_file)
        return;

    now = time ((time_t *) NULL);
    local_time = localtime (&now);
    strftime (time_str, 64, "%d %b %Y, %H:%M:%S", local_time);
    err = FALSE;

    switch (log_level) {
    case G_LOG_LEVEL_WARNING:
        log_level_str = "-Warning **";
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

    if (verbose_log_file) {
        g_fprintf (verbose_log_file,
                   "[%s] %s %s\n",
                   time_str,
                   log_level_str,
                   message);
        fflush (verbose_log_file);
    }

    if (stdout_verbose_flag || err)
        g_fprintf (err ? stderr : stdout,
                   "[%s] %s %s\n",
                   time_str,
                   log_level_str,
                   message);
}

gboolean
qfu_log_get_verbose (void)
{
    return (stdout_verbose_flag || verbose_log_file);
}

gboolean
qfu_log_get_verbose_stdout (void)
{
    return (stdout_verbose_flag);
}

gboolean
qfu_log_init (gboolean     stdout_verbose,
              gboolean     stdout_silent,
              const gchar *verbose_log_path)
{
    if (stdout_verbose && stdout_silent) {
        g_printerr ("error: cannot specify --verbose and --silent at the same time\n");
        return FALSE;
    }

    /* Store flags */
    stdout_verbose_flag = stdout_verbose;
    stdout_silent_flag  = stdout_silent;

    /* Open verbose log if required */
    if (verbose_log_path) {
        verbose_log_file = fopen (verbose_log_path, "w");
        if (!verbose_log_file) {
            g_printerr ("error: cannot open verbose log file for writing: %s\n", g_strerror (errno));
            return FALSE;
        }
    }

    /* Default application logging */
    g_log_set_handler (NULL,  G_LOG_LEVEL_MASK, log_handler, NULL);

    /* libqmi logging */
    g_log_set_handler ("Qmi", G_LOG_LEVEL_MASK, log_handler, NULL);
    if (stdout_verbose_flag || verbose_log_file)
        qmi_utils_set_traces_enabled (TRUE);

#if QMI_MBIM_QMUX_SUPPORTED
    /* libmbim logging */
    g_log_set_handler ("Mbim", G_LOG_LEVEL_MASK, log_handler, NULL);
    if (stdout_verbose_flag || verbose_log_file)
        mbim_utils_set_traces_enabled (TRUE);
#endif

    return TRUE;
}

void
qfu_log_shutdown (void)
{
    if (verbose_log_file)
        fclose (verbose_log_file);
}
