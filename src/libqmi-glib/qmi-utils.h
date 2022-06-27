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
 * Copyright (C) 2012-2015 Dan Williams <dcbw@redhat.com>
 * Copyright (C) 2012-2017 Aleksander Morgado <aleksander@aleksander.es>
 * Copyright (C) 1999,2000 Erik Walthinsen <omega@cse.ogi.edu>
 * Copyright (C) 2000 Wim Taymans <wtay@chello.be>
 * Copyright (C) 2002 Thomas Vander Stichele <thomas@apestaart.org>
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc.
 */

#ifndef _LIBQMI_GLIB_QMI_UTILS_H_
#define _LIBQMI_GLIB_QMI_UTILS_H_

#if !defined (__LIBQMI_GLIB_H_INSIDE__) && !defined (LIBQMI_GLIB_COMPILATION)
#error "Only <libqmi-glib.h> can be included directly."
#endif

#include <glib.h>

G_BEGIN_DECLS

/**
 * SECTION: qmi-utils
 * @title: Common utilities
 * @short_description: Common utilities in the library.
 *
 * This section exposes a set of common utilities that may be used to work
 * with the QMI library.
 */

/* Enabling/Disabling traces */

/**
 * qmi_utils_get_traces_enabled:
 *
 * Checks whether QMI message traces are currently enabled.
 *
 * Returns: %TRUE if traces are enabled, %FALSE otherwise.
 *
 * Since: 1.0
 */
gboolean qmi_utils_get_traces_enabled (void);

/**
 * qmi_utils_set_traces_enabled:
 * @enabled: %TRUE to enable traces, %FALSE to disable them.
 *
 * Sets whether QMI message traces are enabled or disabled.
 *
 * Since: 1.0
 */
void qmi_utils_set_traces_enabled (gboolean enabled);

/**
 * qmi_utils_set_show_personal_info:
 * @show_personal_info: %TRUE to show personal info in traces, %FALSE otherwise.
 *
 * Sets whether personal info is printed when traces are enabled.
 *
 * Since: 1.32
 */
void qmi_utils_set_show_personal_info (gboolean show_personal_info);

/**
 * qmi_utils_get_show_personal_info:
 *
 * Checks whether personal info should be hidden when traces are enabled.
 *
 * Returns: %TRUE to show personal info in trace, %FALSE otherwise.
 *
 * Since: 1.32
 */
gboolean qmi_utils_get_show_personal_info (void);

G_END_DECLS

#endif /* _LIBQMI_GLIB_QMI_UTILS_H_ */
