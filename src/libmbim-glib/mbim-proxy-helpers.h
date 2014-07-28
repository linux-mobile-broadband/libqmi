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
 * Copyright (C) 2014 Aleksander Morgado <aleksander@aleksander.es>
 * Copyright (C) 2014 Smith Micro Software, Inc.
 *
 * This is a private non-installed header
 */

#ifndef _LIBMBIM_GLIB_MBIM_PROXY_HELPERS_H_
#define _LIBMBIM_GLIB_MBIM_PROXY_HELPERS_H_

#if !defined (LIBMBIM_GLIB_COMPILATION)
#error "This is a private header!!"
#endif

#include <glib.h>

#include "mbim-basic-connect.h"

G_BEGIN_DECLS

MbimEventEntry **_mbim_proxy_helper_service_subscribe_standard_list_new (void);
MbimEventEntry **_mbim_proxy_helper_service_subscribe_request_parse     (MbimMessage     *message);
MbimEventEntry **_mbim_proxy_helper_service_subscribe_list_merge        (MbimEventEntry **original,
                                                                         MbimEventEntry **merge,
                                                                         guint           *events_count);

G_END_DECLS

#endif /* _LIBMBIM_GLIB_MBIM_PROXY_HELPERS_H_ */
