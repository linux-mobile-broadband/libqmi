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

#ifndef _LIBQMI_GLIB_QMI_UTILS_H_
#define _LIBQMI_GLIB_QMI_UTILS_H_

#if !defined (__LIBQMI_GLIB_H_INSIDE__) && !defined (LIBQMI_GLIB_COMPILATION)
#error "Only <libqmi-glib.h> can be included directly."
#endif

#include <glib.h>

G_BEGIN_DECLS

/**
 * QmiEndian:
 * @QMI_ENDIAN_LITTLE: Little endian.
 * @QMI_ENDIAN_BIG: Big endian.
 *
 * Type of endianness
 *
 * Since: 1.0
 */
typedef enum {
    QMI_ENDIAN_LITTLE = 0,
    QMI_ENDIAN_BIG    = 1
} QmiEndian;

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

/* Other private methods */

#if defined (LIBQMI_GLIB_COMPILATION)
G_GNUC_INTERNAL
gchar *__qmi_utils_str_hex (gconstpointer mem,
                            gsize size,
                            gchar delimiter);
G_GNUC_INTERNAL
gboolean __qmi_user_allowed (uid_t uid,
                             GError **error);
G_GNUC_INTERNAL
gchar *__qmi_utils_get_driver (const gchar *cdc_wdm_path);

static inline gfloat
__QMI_GFLOAT_SWAP_LE_BE(gfloat in)
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
__QMI_GDOUBLE_SWAP_LE_BE(gdouble in)
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
#define __QMI_ENDIAN_HOST          QMI_ENDIAN_LITTLE
#define __QMI_GFLOAT_TO_LE(val)    ((gfloat) (val))
#define __QMI_GFLOAT_TO_BE(val)    (__QMI_GFLOAT_SWAP_LE_BE (val))
#define __QMI_GDOUBLE_TO_LE(val)   ((gdouble) (val))
#define __QMI_GDOUBLE_TO_BE(val)   (__QMI_GDOUBLE_SWAP_LE_BE (val))

#elif G_BYTE_ORDER == G_BIG_ENDIAN
#define __QMI_ENDIAN_HOST          QMI_ENDIAN_BIG
#define __QMI_GFLOAT_TO_LE(val)    (__QMI_GFLOAT_SWAP_LE_BE (val))
#define __QMI_GFLOAT_TO_BE(val)    ((gfloat) (val))
#define __QMI_GDOUBLE_TO_LE(val)   (__QMI_GDOUBLE_SWAP_LE_BE (val))
#define __QMI_GDOUBLE_TO_BE(val)   ((gdouble) (val))

#else /* !G_LITTLE_ENDIAN && !G_BIG_ENDIAN */
#error unknown ENDIAN type
#endif /* !G_LITTLE_ENDIAN && !G_BIG_ENDIAN */

#define __QMI_GFLOAT_FROM_LE(val)  (__QMI_GFLOAT_TO_LE (val))
#define __QMI_GFLOAT_FROM_BE(val)  (__QMI_GFLOAT_TO_BE (val))
#define __QMI_GDOUBLE_FROM_LE(val) (__QMI_GDOUBLE_TO_LE (val))
#define __QMI_GDOUBLE_FROM_BE(val) (__QMI_GDOUBLE_TO_BE (val))

#endif /* defined (LIBQMI_GLIB_COMPILATION) */

G_END_DECLS

#endif /* _LIBQMI_GLIB_QMI_UTILS_H_ */
