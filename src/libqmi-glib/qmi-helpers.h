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

 */

#ifndef _LIBQMI_GLIB_QMI_HELPERS_H_
#define _LIBQMI_GLIB_QMI_HELPERS_H_

#include <glib.h>
#include <gio/gio.h>

G_BEGIN_DECLS

G_GNUC_INTERNAL
gchar *qmi_helpers_str_hex (gconstpointer mem,
                            gsize         size,
                            gchar         delimiter);
G_GNUC_INTERNAL
gboolean qmi_helpers_check_user_allowed  (uid_t    uid,
                                          GError **error);

G_GNUC_INTERNAL
gboolean qmi_helpers_string_utf8_validate_printable (const guint8 *utf8,
                                                     gsize         utf8_len);

G_GNUC_INTERNAL
gchar *qmi_helpers_string_utf8_from_gsm7 (const guint8 *gsm,
                                          gsize         gsm_len);

G_GNUC_INTERNAL
gchar *qmi_helpers_string_utf8_from_ucs2le (const guint8 *ucs2le,
                                            gsize         ucs2le_len);

typedef enum {
    QMI_HELPERS_TRANSPORT_TYPE_UNKNOWN,
    QMI_HELPERS_TRANSPORT_TYPE_QMUX,
    QMI_HELPERS_TRANSPORT_TYPE_MBIM,
} QmiHelpersTransportType;

G_GNUC_INTERNAL
QmiHelpersTransportType qmi_helpers_get_transport_type (const gchar  *path,
                                                        GError      **error);

G_GNUC_INTERNAL
gchar *qmi_helpers_get_devpath (const gchar  *cdc_wdm_path,
                                GError      **error);

G_GNUC_INTERNAL
gchar *qmi_helpers_get_devname (const gchar  *cdc_wdm_path,
                                GError      **error);

G_GNUC_INTERNAL
gboolean qmi_helpers_read_sysfs_file (const gchar  *sysfs_path,
                                      gchar        *out_value, /* caller allocates */
                                      guint         max_read_size,
                                      GError      **error);

G_GNUC_INTERNAL
gboolean qmi_helpers_write_sysfs_file (const gchar  *sysfs_path,
                                       const gchar  *value,
                                       GError      **error);

G_GNUC_INTERNAL
gboolean qmi_helpers_list_links (GFile         *sysfs_file,
                                 GCancellable  *cancellable,
                                 GPtrArray     *previous_links,
                                 GPtrArray    **out_links,
                                 GError       **error);

static inline gfloat
QMI_GFLOAT_SWAP_LE_BE (gfloat in)
{
  union
  {
    guint32 i;
    gfloat f;
  } u;

  u.f = in;
  u.i = GUINT32_SWAP_LE_BE (u.i);
  return u.f;
}

static inline gdouble
QMI_GDOUBLE_SWAP_LE_BE (gdouble in)
{
  union
  {
    guint64 i;
    gdouble d;
  } u;

  u.d = in;
  u.i = GUINT64_SWAP_LE_BE (u.i);
  return u.d;
}

#if G_BYTE_ORDER == G_LITTLE_ENDIAN
#define QMI_ENDIAN_HOST          QMI_ENDIAN_LITTLE
#define QMI_GFLOAT_TO_LE(val)    ((gfloat) (val))
#define QMI_GFLOAT_TO_BE(val)    (QMI_GFLOAT_SWAP_LE_BE (val))
#define QMI_GDOUBLE_TO_LE(val)   ((gdouble) (val))
#define QMI_GDOUBLE_TO_BE(val)   (QMI_GDOUBLE_SWAP_LE_BE (val))

#elif G_BYTE_ORDER == G_BIG_ENDIAN
#define QMI_ENDIAN_HOST          QMI_ENDIAN_BIG
#define QMI_GFLOAT_TO_LE(val)    (QMI_GFLOAT_SWAP_LE_BE (val))
#define QMI_GFLOAT_TO_BE(val)    ((gfloat) (val))
#define QMI_GDOUBLE_TO_LE(val)   (QMI_GDOUBLE_SWAP_LE_BE (val))
#define QMI_GDOUBLE_TO_BE(val)   ((gdouble) (val))

#else /* !G_LITTLE_ENDIAN && !G_BIG_ENDIAN */
#error unknown ENDIAN type
#endif /* !G_LITTLE_ENDIAN && !G_BIG_ENDIAN */

#define QMI_GFLOAT_FROM_LE(val)  (QMI_GFLOAT_TO_LE (val))
#define QMI_GFLOAT_FROM_BE(val)  (QMI_GFLOAT_TO_BE (val))
#define QMI_GDOUBLE_FROM_LE(val) (QMI_GDOUBLE_TO_LE (val))
#define QMI_GDOUBLE_FROM_BE(val) (QMI_GDOUBLE_TO_BE (val))



#if !GLIB_CHECK_VERSION(2,54,0)

/* Pointer Array lookup with a GEqualFunc, imported from GLib 2.54 */
#define g_ptr_array_find_with_equal_func qmi_ptr_array_find_with_equal_func
G_GNUC_INTERNAL
gboolean qmi_ptr_array_find_with_equal_func (GPtrArray     *haystack,
                                             gconstpointer  needle,
                                             GEqualFunc     equal_func,
                                             guint         *index_);

#endif

G_END_DECLS

#endif /* _LIBQMI_GLIB_QMI_UTILS_PRIVATE_H_ */
