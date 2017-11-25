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
 * Copyright (C) 2012-2017 Aleksander Morgado <aleksander@aleksander.es>
 */

#ifndef _LIBQMI_GLIB_QMI_CLIENT_H_
#define _LIBQMI_GLIB_QMI_CLIENT_H_

#if !defined (__LIBQMI_GLIB_H_INSIDE__) && !defined (LIBQMI_GLIB_COMPILATION)
#error "Only <libqmi-glib.h> can be included directly."
#endif

#include <glib-object.h>

#include "qmi-enums.h"
#include "qmi-message.h"

G_BEGIN_DECLS

/**
 * SECTION:qmi-client
 * @title: QmiClient
 * @short_description: Generic QMI client handling routines
 *
 * #QmiClient is a generic type representing a QMI client for any kind of
 * #QmiService.
 *
 * These objects are created by a #QmiDevice with qmi_device_allocate_client(),
 * and before completely disposing them qmi_device_release_client() needs to be
 * called in order to release the unique client ID reserved.
 */

/**
 * QMI_CID_NONE:
 *
 * A symbol specifying a special CID value that references no CID.
 *
 * Since: 1.0
 */
#define QMI_CID_NONE 0x00

/**
 * QMI_CID_BROADCAST:
 *
 * A symbol specifying the broadcast CID.
 *
 * Since: 1.0
 */
#define QMI_CID_BROADCAST 0xFF

#define QMI_TYPE_CLIENT            (qmi_client_get_type ())
#define QMI_CLIENT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), QMI_TYPE_CLIENT, QmiClient))
#define QMI_CLIENT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  QMI_TYPE_CLIENT, QmiClientClass))
#define QMI_IS_CLIENT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), QMI_TYPE_CLIENT))
#define QMI_IS_CLIENT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  QMI_TYPE_CLIENT))
#define QMI_CLIENT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  QMI_TYPE_CLIENT, QmiClientClass))

typedef struct _QmiClient QmiClient;
typedef struct _QmiClientClass QmiClientClass;
typedef struct _QmiClientPrivate QmiClientPrivate;

/**
 * QMI_CLIENT_DEVICE:
 *
 * Symbol defining the #QmiClient:client-device property.
 *
 * Since: 1.0
 */
#define QMI_CLIENT_DEVICE "client-device"

/**
 * QMI_CLIENT_SERVICE:
 *
 * Symbol defining the #QmiClient:client-service property.
 *
 * Since: 1.0
 */
#define QMI_CLIENT_SERVICE "client-service"

/**
 * QMI_CLIENT_CID:
 *
 * Symbol defining the #QmiClient:client-cid property.
 *
 * Since: 1.0
 */
#define QMI_CLIENT_CID "client-cid"

/**
 * QMI_CLIENT_VERSION_MAJOR:
 *
 * Symbol defining the #QmiClient:client-version-major property.
 *
 * Since: 1.0
 */
#define QMI_CLIENT_VERSION_MAJOR "client-version-major"

/**
 * QMI_CLIENT_VERSION_MINOR:
 *
 * Symbol defining the #QmiClient:client-version-minor property.
 *
 * Since: 1.0
 */
#define QMI_CLIENT_VERSION_MINOR "client-version-minor"

/**
 * QMI_CLIENT_VALID:
 *
 * Symbol defining the #QmiClient:valid property.
 *
 * Since: 1.20
 */
#define QMI_CLIENT_VALID "client-valid"

/**
 * QmiClient:
 *
 * The #QmiClient structure contains private data and should only be accessed
 * using the provided API.
 *
 * Since: 1.0
 */
struct _QmiClient {
    /*< private >*/
    GObject parent;
    QmiClientPrivate *priv;
};

struct _QmiClientClass {
    /*< private >*/
    GObjectClass parent;

    /* Virtual method to get indications processed */
    void (* process_indication) (QmiClient *self,
                                 QmiMessage *message);
};

GType qmi_client_get_type (void);

/**
 * qmi_client_get_device:
 * @self: a #QmiClient
 *
 * Get the #QmiDevice associated with this #QmiClient.
 *
 * Returns: a #GObject that must be freed with g_object_unref().
 *
 * Since: 1.0
 */
GObject *qmi_client_get_device (QmiClient *self);

/**
 * qmi_client_peek_device:
 * @self: a #QmiClient.
 *
 * Get the #QmiDevice associated with this #QmiClient, without increasing the reference count
 * on the returned object.
 *
 * Returns: a #GObject. Do not free the returned object, it is owned by @self.
 *
 * Since: 1.0
 */
GObject *qmi_client_peek_device (QmiClient *self);

/**
 * qmi_client_get_service:
 * @self: A #QmiClient
 *
 * Get the service being used by this #QmiClient.
 *
 * Returns: a #QmiService.
 *
 * Since: 1.0
 */
QmiService qmi_client_get_service (QmiClient *self);

/**
 * qmi_client_get_cid:
 * @self: A #QmiClient
 *
 * Get the client ID of this #QmiClient.
 *
 * Returns: the client ID.
 *
 * Since: 1.0
 */
guint8 qmi_client_get_cid (QmiClient *self);

/**
 * qmi_client_is_valid:
 * @self: a #QmiClient.
 *
 * Checks whether #QmiClient is a valid and usable client.
 *
 * The client is marked as invalid as soon as the client id is released or when
 * the associated #QmiDevice is closed.
 *
 * This method may be used if the caller needs to ensure validity before a
 * command is attempted, e.g. if the lifecycle of the object is managed in some
 * other place and the caller just has a reference to the #QmiClient.
 *
 * Returns: %TRUE if the client is valid, %FALSE otherwise.
 *
 * Since: 1.20
 */
gboolean qmi_client_is_valid (QmiClient *self);

/**
 * qmi_client_get_version:
 * @self: A #QmiClient
 * @major: placeholder for the output major version.
 * @minor: placeholder for the output minor version.
 *
 * Get the version of the service handled by this #QmiClient.
 *
 * Returns: %TRUE if the version was properly reported, %FALSE otherwise.
 *
 * Since: 1.0
 */
gboolean qmi_client_get_version (QmiClient *self,
                                 guint     *major,
                                 guint     *minor);

/**
 * qmi_client_check_version:
 * @self: A #QmiClient
 * @major: a major version.
 * @minor: a minor version.
 *
 * Checks if the version of the service handled by this #QmiClient is greater
 * or equal than the given version.
 *
 * Returns: %TRUE if the version of the service is greater or equal than the one given, %FALSE otherwise.
 *
 * Since: 1.0
 */
gboolean qmi_client_check_version (QmiClient *self,
                                   guint      major,
                                   guint      minor);

/**
 * qmi_client_get_next_transaction_id:
 * @self: A #QmiClient
 *
 * Acquire the next transaction ID of this #QmiClient.
 * The internal transaction ID gets incremented.
 *
 * Returns: the next transaction ID.
 *
 * Since: 1.0
 */
guint16 qmi_client_get_next_transaction_id (QmiClient *self);

/* not part of the public API */

#if defined (LIBQMI_GLIB_COMPILATION)
G_GNUC_INTERNAL
void __qmi_client_process_indication (QmiClient  *self,
                                      QmiMessage *message);
#endif

G_END_DECLS

#endif /* _LIBQMI_GLIB_QMI_CLIENT_H_ */
