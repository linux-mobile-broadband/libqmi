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
 * Copyright (C) 2021 Google Inc.
 */

#ifndef _LIBQMI_GLIB_QMI_FLAGS64_WDS_H_
#define _LIBQMI_GLIB_QMI_FLAGS64_WDS_H_

#if !defined (__LIBQMI_GLIB_H_INSIDE__) && !defined (LIBQMI_GLIB_COMPILATION)
#error "Only <libqmi-glib.h> can be included directly."
#endif

#include <glib.h>

/**
 * QmiWdsApnTypeMask:
 * @QMI_WDS_APN_TYPE_MASK_DEFAULT: Default/Internet traffic.
 * @QMI_WDS_APN_TYPE_MASK_IMS: IP Multimedia Subsystem.
 * @QMI_WDS_APN_TYPE_MASK_MMS: Multimedia Messaging Service.
 * @QMI_WDS_APN_TYPE_MASK_FOTA: over the air administration.
 * @QMI_WDS_APN_TYPE_MASK_IA: Initial Attach.
 * @QMI_WDS_APN_TYPE_MASK_EMERGENCY: Emergency.
 *
 * APN type as a bitmask.
 *
 * Since: 1.30
 */
typedef enum { /*< since=1.30 >*/
    QMI_WDS_APN_TYPE_MASK_DEFAULT   = ((guint64) 1) << 0,
    QMI_WDS_APN_TYPE_MASK_IMS       = ((guint64) 1) << 1,
    QMI_WDS_APN_TYPE_MASK_MMS       = ((guint64) 1) << 2,
    QMI_WDS_APN_TYPE_MASK_FOTA      = ((guint64) 1) << 6,
    QMI_WDS_APN_TYPE_MASK_IA        = ((guint64) 1) << 8,
    QMI_WDS_APN_TYPE_MASK_EMERGENCY = ((guint64) 1) << 9,
} QmiWdsApnTypeMask;

#endif /* _LIBQMI_GLIB_QMI_FLAGS64_WDS_H_ */
