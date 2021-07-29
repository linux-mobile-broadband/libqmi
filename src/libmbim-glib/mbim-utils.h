/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * libmbim-glib -- GLib/GIO based library to control MBIM devices
 *
 * Copyright (C) 2013 - 2014 Aleksander Morgado <aleksander@aleksander.es>
 */

#ifndef _LIBMBIM_GLIB_MBIM_UTILS_H_
#define _LIBMBIM_GLIB_MBIM_UTILS_H_

#if !defined (__LIBMBIM_GLIB_H_INSIDE__) && !defined (LIBMBIM_GLIB_COMPILATION)
#error "Only <libmbim-glib.h> can be included directly."
#endif

#include <glib.h>

G_BEGIN_DECLS

/* Enabling/Disabling traces */

/**
 * mbim_utils_get_traces_enabled:
 *
 * Checks whether MBIM message traces are currently enabled.
 *
 * Returns: %TRUE if traces are enabled, %FALSE otherwise.
 *
 * Since: 1.0
 */
gboolean mbim_utils_get_traces_enabled (void);

/**
 * mbim_utils_set_traces_enabled:
 * @enabled: %TRUE to enable traces, %FALSE to disable them.
 *
 * Sets whether MBIM message traces are enabled or disabled.
 *
 * Since: 1.0
 */
void mbim_utils_set_traces_enabled (gboolean enabled);

G_END_DECLS

#endif /* _LIBMBIM_GLIB_MBIM_UTILS_H_ */
