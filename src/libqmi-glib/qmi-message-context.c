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

#include <glib.h>

#include "qmi-message-context.h"

/*****************************************************************************/
/* Basic context */

struct _QmiMessageContext {
    volatile gint ref_count;

    /* Vendor ID */
    guint16 vendor_id;
};

QmiMessageContext *
qmi_message_context_new (void)
{
    QmiMessageContext *self;

    self = g_slice_new0 (QmiMessageContext);
    self->ref_count = 1;
    return self;
}

GType
qmi_message_context_get_type (void)
{
    static gsize g_define_type_id_initialized = 0;

    if (g_once_init_enter (&g_define_type_id_initialized)) {
        GType g_define_type_id =
            g_boxed_type_register_static (g_intern_static_string ("QmiMessageContext"),
                                          (GBoxedCopyFunc) qmi_message_context_ref,
                                          (GBoxedFreeFunc) qmi_message_context_unref);

        g_once_init_leave (&g_define_type_id_initialized, g_define_type_id);
    }

    return g_define_type_id_initialized;
}

QmiMessageContext *
qmi_message_context_ref (QmiMessageContext *self)
{
    g_return_val_if_fail (self != NULL, NULL);

    g_atomic_int_inc (&self->ref_count);
    return self;
}

void
qmi_message_context_unref (QmiMessageContext *self)
{
    g_return_if_fail (self != NULL);

    if (g_atomic_int_dec_and_test (&self->ref_count)) {
        g_slice_free (QmiMessageContext, self);
    }
}

/*****************************************************************************/
/* Vendor ID */

void
qmi_message_context_set_vendor_id (QmiMessageContext *self,
                                   guint16            vendor_id)
{
    g_return_if_fail (self != NULL);

    self->vendor_id = vendor_id;
}

guint16
qmi_message_context_get_vendor_id (QmiMessageContext *self)
{
    g_return_val_if_fail (self != NULL, QMI_MESSAGE_VENDOR_GENERIC);
    return self->vendor_id;
}
