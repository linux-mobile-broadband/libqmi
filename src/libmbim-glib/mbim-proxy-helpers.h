/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * libmbim-glib -- GLib/GIO based library to control MBIM devices
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

gboolean         _mbim_proxy_helper_service_subscribe_list_cmp          (const MbimEventEntry * const *a,
                                                                         gsize                         a_size,
                                                                         const MbimEventEntry * const *b,
                                                                         gsize                         b_size);
void             _mbim_proxy_helper_service_subscribe_list_debug        (const MbimEventEntry * const *list,
                                                                         gsize                         list_size);
MbimEventEntry **_mbim_proxy_helper_service_subscribe_request_parse     (MbimMessage     *message,
                                                                         gsize           *out_size,
                                                                         GError         **error);
MbimEventEntry **_mbim_proxy_helper_service_subscribe_list_merge        (MbimEventEntry **original,
                                                                         gsize            original_size,
                                                                         MbimEventEntry **merge,
                                                                         gsize            merge_size,
                                                                         gsize           *out_size);
MbimEventEntry **_mbim_proxy_helper_service_subscribe_list_dup          (MbimEventEntry **original,
                                                                         gsize            original_size,
                                                                         gsize           *out_size);
MbimEventEntry **_mbim_proxy_helper_service_subscribe_list_new_standard (gsize           *out_size);

G_END_DECLS

#endif /* _LIBMBIM_GLIB_MBIM_PROXY_HELPERS_H_ */
