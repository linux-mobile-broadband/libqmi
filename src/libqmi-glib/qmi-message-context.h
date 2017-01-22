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
 * Copyright (C) 2017 Aleksander Morgado <aleksander@aleksander.es>
 */

#ifndef _LIBQMI_GLIB_QMI_MESSAGE_CONTEXT_H_
#define _LIBQMI_GLIB_QMI_MESSAGE_CONTEXT_H_

#if !defined (__LIBQMI_GLIB_H_INSIDE__) && !defined (LIBQMI_GLIB_COMPILATION)
#error "Only <libqmi-glib.h> can be included directly."
#endif

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

/**
 * QmiMessageContext:
 *
 * An opaque type representing a QMI message context.
 *
 * The context defines non-standard features of the QMI message associated with
 * it, which may be required for a correct processing.
 *
 * When a context is given when sending a request with qmi_device_command_full(),
 * the same context will then be applied for the associated response.
 */
typedef struct _QmiMessageContext QmiMessageContext;

GType qmi_message_context_get_type (void);

/*****************************************************************************/
/* Basic context */

QmiMessageContext *qmi_message_context_new   (void);
QmiMessageContext *qmi_message_context_ref   (QmiMessageContext *self);
void               qmi_message_context_unref (QmiMessageContext *self);

/*****************************************************************************/
/* Vendor ID */

/**
 * QMI_MESSAGE_VENDOR_GENERIC:
 *
 * Generic vendor id (0x0000).
 */
#define QMI_MESSAGE_VENDOR_GENERIC 0x0000

void    qmi_message_context_set_vendor_id (QmiMessageContext *self,
                                           guint16            vendor_id);
guint16 qmi_message_context_get_vendor_id (QmiMessageContext *self);

G_END_DECLS

#endif /* _LIBQMI_GLIB_QMI_MESSAGE_CONTEXT_H_ */
