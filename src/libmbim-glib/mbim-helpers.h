/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * libmbim-glib -- GLib/GIO based library to control MBIM devices
 *
 * Copyright (C) 2013 - 2021 Aleksander Morgado <aleksander@aleksander.es>
 */

#ifndef _LIBMBIM_GLIB_MBIM_HELPERS_H_
#define _LIBMBIM_GLIB_MBIM_HELPERS_H_

#if !defined (LIBMBIM_GLIB_COMPILATION)
#error "This is a private header!!"
#endif

#include <glib.h>
#include <gio/gio.h>

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

G_GNUC_INTERNAL
gboolean mbim_helpers_list_links_wdm (GFile         *sysfs_file,
                                      GCancellable  *cancellable,
                                      GPtrArray     *previous_links,
                                      GPtrArray    **out_links,
                                      GError       **error);

#if !GLIB_CHECK_VERSION(2,54,0)

/* Pointer Array lookup with a GEqualFunc, imported from GLib 2.54 */
#define g_ptr_array_find_with_equal_func mbim_ptr_array_find_with_equal_func
G_GNUC_INTERNAL
gboolean mbim_ptr_array_find_with_equal_func (GPtrArray     *haystack,
                                              gconstpointer  needle,
                                              GEqualFunc     equal_func,
                                              guint         *index_);

#endif

G_END_DECLS

#endif /* _LIBMBIM_GLIB_MBIM_HELPERS_H_ */
