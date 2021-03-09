/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 * libmbim-glib -- GLib/GIO based library to control MBIM devices
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
 * Copyright (C) 2013 - 2021 Aleksander Morgado <aleksander@aleksander.es>
 */

#ifndef _LIBMBIM_GLIB_MBIM_HELPERS_H_
#define _LIBMBIM_GLIB_MBIM_HELPERS_H_

#if !defined (LIBMBIM_GLIB_COMPILATION)
#error "This is a private header!!"
#endif

#include <glib.h>

G_BEGIN_DECLS

G_GNUC_INTERNAL
gboolean mbim_helpers_check_user_allowed (uid_t    uid,
                                          GError **error);

G_GNUC_INTERNAL
gchar *mbim_helpers_get_devpath (const gchar  *cdc_wdm_path,
                                 GError      **error);

G_GNUC_INTERNAL
gchar *mbim_helpers_get_devname (const gchar  *cdc_wdm_path,
                                 GError      **error);

G_END_DECLS

#endif /* _LIBMBIM_GLIB_MBIM_HELPERS_H_ */
