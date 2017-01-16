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

#ifndef QFU_LOG_H
#define QFU_LOG_H

#include <glib.h>

G_BEGIN_DECLS

gboolean qfu_log_init               (gboolean     stdout_verbose,
                                     gboolean     stdout_silent,
                                     const gchar *verbose_log_path);
void     qfu_log_shutdown           (void);
gboolean qfu_log_get_verbose        (void);
gboolean qfu_log_get_verbose_stdout (void);

G_END_DECLS

#endif /* QFU_LOG_H */
