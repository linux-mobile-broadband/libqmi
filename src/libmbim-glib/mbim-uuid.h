/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * libmbim-glib -- GLib/GIO based library to control MBIM devices
 *
 * Copyright (C) 2013 - 2014 Aleksander Morgado <aleksander@aleksander.es>
 */

#ifndef _LIBMBIM_GLIB_MBIM_UUID_H_
#define _LIBMBIM_GLIB_MBIM_UUID_H_

#if !defined (__LIBMBIM_GLIB_H_INSIDE__) && !defined (LIBMBIM_GLIB_COMPILATION)
#error "Only <libmbim-glib.h> can be included directly."
#endif

#include <glib.h>

G_BEGIN_DECLS

/**
 * SECTION: mbim-uuid
 * @title: UUIDs
 * @short_description: Generic UUID handling routines.
 *
 * This section defines the data type for unique identifiers.
 */

/*****************************************************************************/

/**
 * MbimUuid:
 *
 * A UUID as defined in MBIM.
 *
 * Since: 1.0
 */
typedef struct _MbimUuid MbimUuid;
#define MBIM_PACKED __attribute__((__packed__))
struct MBIM_PACKED _MbimUuid {
    guint8 a[4];
    guint8 b[2];
    guint8 c[2];
    guint8 d[2];
    guint8 e[6];
};
#undef MBIM_PACKED

/**
 * mbim_uuid_cmp:
 * @a: a #MbimUuid.
 * @b: a #MbimUuid.
 *
 * Compare two %MbimUuid values.
 *
 * Returns: %TRUE if @a and @b are equal, %FALSE otherwise.
 *
 * Since: 1.0
 */
gboolean  mbim_uuid_cmp (const MbimUuid *a,
                         const MbimUuid *b);

/**
 * mbim_uuid_get_printable:
 * @uuid: a #MbimUuid.
 *
 * Get a string with the UUID.
 *
 * Returns: (transfer full): a newly allocated string, which should be freed with g_free().
 *
 * Since: 1.0
 */
gchar *mbim_uuid_get_printable (const MbimUuid *uuid);

/**
 * mbim_uuid_from_printable:
 * @str: a MBIM UUID.
 * @uuid: pointer to the target #MbimUuid.
 *
 * Fills in @uuid from the printable representation give in @str.
 *
 * Only ccepts @str written with dashes separating items, e.g.:
 *  a289cc33-bcbb-8b4f-b6b0-133ec2aae6df
 *
 * Returns: %TRUE if @uuid was correctly set, %FALSE otherwise.
 *
 * Since: 1.8
 */
gboolean mbim_uuid_from_printable (const gchar *str,
                                   MbimUuid    *uuid);

/*****************************************************************************/

/**
 * MbimService:
 * @MBIM_SERVICE_INVALID: Invalid service.
 * @MBIM_SERVICE_BASIC_CONNECT: Basic connectivity service.
 * @MBIM_SERVICE_SMS: SMS messaging service.
 * @MBIM_SERVICE_USSD: USSD service.
 * @MBIM_SERVICE_PHONEBOOK: Phonebook service.
 * @MBIM_SERVICE_STK: SIM toolkit service.
 * @MBIM_SERVICE_AUTH: Authentication service.
 * @MBIM_SERVICE_DSS: Device Service Stream service.
 * @MBIM_SERVICE_MS_FIRMWARE_ID: Microsoft Firmware ID service. Since 1.8.
 * @MBIM_SERVICE_MS_HOST_SHUTDOWN: Microsoft Host Shutdown service. Since 1.8.
 * @MBIM_SERVICE_PROXY_CONTROL: Proxy Control service. Since 1.10.
 * @MBIM_SERVICE_QMI: QMI-over-MBIM service. Since 1.14.
 * @MBIM_SERVICE_ATDS: ATT Device service. Since 1.16.
 * @MBIM_SERVICE_INTEL_FIRMWARE_UPDATE: Intel firmware update service. Since 1.16.
 * @MBIM_SERVICE_MS_BASIC_CONNECT_EXTENSIONS: Microsoft basic connectivity extensions service. Since 1.18.
 * @MBIM_SERVICE_MS_SAR: Microsoft SAR service. Since 1.26.
 * @MBIM_SERVICE_QDU: QDU firmware update service. Since 1.26.
 * @MBIM_SERVICE_MS_UICC_LOW_LEVEL_ACCESS: Microsoft UICC Low Level Access service. Since 1.26.
 * @MBIM_SERVICE_QUECTEL: Quectel specific operations. Since 1.26.2.
 * @MBIM_SERVICE_INTEL_THERMAL_RF: Intel thermal rf related commands. Since 1.28
 * @MBIM_SERVICE_MS_VOICE_EXTENSIONS: Microsoft Voice extensions service. Since 1.28.
 * @MBIM_SERVICE_LAST: Internal value.
 *
 * Enumeration of the generic MBIM services.
 *
 * Since: 1.0
 */
typedef enum { /*< since=1.0 >*/
    MBIM_SERVICE_INVALID                     = 0,
    MBIM_SERVICE_BASIC_CONNECT               = 1,
    MBIM_SERVICE_SMS                         = 2,
    MBIM_SERVICE_USSD                        = 3,
    MBIM_SERVICE_PHONEBOOK                   = 4,
    MBIM_SERVICE_STK                         = 5,
    MBIM_SERVICE_AUTH                        = 6,
    MBIM_SERVICE_DSS                         = 7,
    MBIM_SERVICE_MS_FIRMWARE_ID              = 8,
    MBIM_SERVICE_MS_HOST_SHUTDOWN            = 9,
    MBIM_SERVICE_PROXY_CONTROL               = 10,
    MBIM_SERVICE_QMI                         = 11,
    MBIM_SERVICE_ATDS                        = 12,
    MBIM_SERVICE_INTEL_FIRMWARE_UPDATE       = 13,
    MBIM_SERVICE_MS_BASIC_CONNECT_EXTENSIONS = 14,
    MBIM_SERVICE_MS_SAR                      = 15,
    MBIM_SERVICE_QDU                         = 16,
    MBIM_SERVICE_MS_UICC_LOW_LEVEL_ACCESS    = 17,
    MBIM_SERVICE_QUECTEL                     = 18,
    MBIM_SERVICE_INTEL_THERMAL_RF            = 19,
    MBIM_SERVICE_MS_VOICE_EXTENSIONS         = 20,
#if defined LIBMBIM_GLIB_COMPILATION
    MBIM_SERVICE_LAST /*< skip >*/
#endif
} MbimService;

/**
 * MBIM_UUID_INVALID:
 *
 * Get the UUID of the %MBIM_SERVICE_INVALID service.
 *
 * Returns: (transfer none): a #MbimUuid.
 *
 * Since: 1.0
 */
#define MBIM_UUID_INVALID mbim_uuid_from_service (MBIM_SERVICE_INVALID)

/**
 * MBIM_UUID_BASIC_CONNECT:
 *
 * Get the UUID of the %MBIM_SERVICE_BASIC_CONNECT service.
 *
 * Returns: (transfer none): a #MbimUuid.
 *
 * Since: 1.0
 */
#define MBIM_UUID_BASIC_CONNECT mbim_uuid_from_service (MBIM_SERVICE_BASIC_CONNECT)

/**
 * MBIM_UUID_SMS:
 *
 * Get the UUID of the %MBIM_SERVICE_SMS service.
 *
 * Returns: (transfer none): a #MbimUuid.
 *
 * Since: 1.0
 */
#define MBIM_UUID_SMS mbim_uuid_from_service (MBIM_SERVICE_SMS)

/**
 * MBIM_UUID_USSD:
 *
 * Get the UUID of the %MBIM_SERVICE_USSD service.
 *
 * Returns: (transfer none): a #MbimUuid.
 *
 * Since: 1.0
 */
#define MBIM_UUID_USSD mbim_uuid_from_service (MBIM_SERVICE_USSD)

/**
 * MBIM_UUID_PHONEBOOK:
 *
 * Get the UUID of the %MBIM_SERVICE_PHONEBOOK service.
 *
 * Returns: (transfer none): a #MbimUuid.
 *
 * Since: 1.0
 */
#define MBIM_UUID_PHONEBOOK mbim_uuid_from_service (MBIM_SERVICE_PHONEBOOK)

/**
 * MBIM_UUID_STK:
 *
 * Get the UUID of the %MBIM_SERVICE_STK service.
 *
 * Returns: (transfer none): a #MbimUuid.
 *
 * Since: 1.0
 */
#define MBIM_UUID_STK mbim_uuid_from_service (MBIM_SERVICE_STK)

/**
 * MBIM_UUID_AUTH:
 *
 * Get the UUID of the %MBIM_SERVICE_AUTH service.
 *
 * Returns: (transfer none): a #MbimUuid.
 *
 * Since: 1.0
 */
#define MBIM_UUID_AUTH mbim_uuid_from_service (MBIM_SERVICE_AUTH)

/**
 * MBIM_UUID_DSS:
 *
 * Get the UUID of the %MBIM_SERVICE_DSS service.
 *
 * Returns: (transfer none): a #MbimUuid.
 *
 * Since: 1.0
 */
#define MBIM_UUID_DSS mbim_uuid_from_service (MBIM_SERVICE_DSS)

/**
 * MBIM_UUID_MS_FIRMWARE_ID:
 *
 * Get the UUID of the %MBIM_SERVICE_MS_FIRMWARE_ID service.
 *
 * Returns: (transfer none): a #MbimUuid.
 *
 * Since: 1.8
 */
#define MBIM_UUID_MS_FIRMWARE_ID mbim_uuid_from_service (MBIM_SERVICE_MS_FIRMWARE_ID)

/**
 * MBIM_UUID_MS_HOST_SHUTDOWN:
 *
 * Get the UUID of the %MBIM_SERVICE_MS_HOST_SHUTDOWN service.
 *
 * Returns: (transfer none): a #MbimUuid.
 *
 * Since: 1.8
 */
#define MBIM_UUID_MS_HOST_SHUTDOWN mbim_uuid_from_service (MBIM_SERVICE_MS_HOST_SHUTDOWN)

/**
 * MBIM_UUID_MS_SAR:
 *
 * Get the UUID of the %MBIM_SERVICE_MS_SAR service.
 *
 * Returns: (transfer none): a #MbimUuid.
 *
 * Since: 1.26
 */
#define MBIM_UUID_MS_SAR mbim_uuid_from_service (MBIM_SERVICE_MS_SAR)

/**
 * MBIM_UUID_PROXY_CONTROL:
 *
 * Get the UUID of the %MBIM_SERVICE_PROXY_CONTROL service.
 *
 * Returns: (transfer none): a #MbimUuid.
 *
 * Since: 1.10
 */
#define MBIM_UUID_PROXY_CONTROL mbim_uuid_from_service (MBIM_SERVICE_PROXY_CONTROL)

/**
 * MBIM_UUID_QMI:
 *
 * Get the UUID of the %MBIM_SERVICE_QMI service.
 *
 * Returns: (transfer none): a #MbimUuid.
 *
 * Since: 1.14
 */
#define MBIM_UUID_QMI mbim_uuid_from_service (MBIM_SERVICE_QMI)

/**
 * MBIM_UUID_ATDS:
 *
 * Get the UUID of the %MBIM_SERVICE_ATDS service.
 *
 * Returns: (transfer none): a #MbimUuid.
 *
 * Since: 1.16
 */
#define MBIM_UUID_ATDS mbim_uuid_from_service (MBIM_SERVICE_ATDS)

/**
 * MBIM_UUID_INTEL_FIRMWARE_UPDATE:
 *
 * Get the UUID of the %MBIM_SERVICE_INTEL_FIRMWARE_UPDATE service.
 *
 * Returns: (transfer none): a #MbimUuid.
 *
 * Since: 1.16
 */
#define MBIM_UUID_INTEL_FIRMWARE_UPDATE mbim_uuid_from_service (MBIM_SERVICE_INTEL_FIRMWARE_UPDATE)

/**
 * MBIM_UUID_QDU:
 *
 * Get the UUID of the %MBIM_SERVICE_QDU service.
 *
 * Returns: (transfer none): a #MbimUuid.
 *
 * Since: 1.26
 */
#define MBIM_UUID_QDU mbim_uuid_from_service (MBIM_SERVICE_QDU)

/**
 * MBIM_UUID_MS_BASIC_CONNECT_EXTENSIONS:
 *
 * Get the UUID of the %MBIM_SERVICE_MS_BASIC_CONNECT_EXTENSIONS service.
 *
 * Returns: (transfer none): a #MbimUuid.
 *
 * Since: 1.18
 */
#define MBIM_UUID_MS_BASIC_CONNECT_EXTENSIONS mbim_uuid_from_service (MBIM_SERVICE_MS_BASIC_CONNECT_EXTENSIONS)

/**
 * MBIM_UUID_MS_UICC_LOW_LEVEL_ACCESS:
 *
 * Get the UUID of the %MBIM_SERVICE_MS_UICC_LOW_LEVEL_ACCESS service.
 *
 * Returns: (transfer none): a #MbimUuid.
 *
 * Since: 1.26
 */
#define MBIM_UUID_MS_UICC_LOW_LEVEL_ACCESS mbim_uuid_from_service (MBIM_SERVICE_MS_UICC_LOW_LEVEL_ACCESS)

/**
 * MBIM_UUID_QUECTEL:
 *
 * Get the UUID of the %MBIM_SERVICE_QUECTEL service.
 *
 * Returns: (transfer none): a #MbimUuid.
 *
 * Since: 1.26.2
 */
#define MBIM_UUID_QUECTEL mbim_uuid_from_service (MBIM_SERVICE_QUECTEL)

/**
 * MBIM_UUID_INTEL_THERMAL_RF:
 *
 * Get the UUID of the %MBIM_SERVICE_INTEL_THERMAL_RF service.
 *
 * Returns: (transfer none): a #MbimUuid.
 *
 * Since: 1.28
 */
#define MBIM_UUID_INTEL_THERMAL_RF mbim_uuid_from_service (MBIM_SERVICE_INTEL_THERMAL_RF)


/**
 * MBIM_UUID_MS_VOICE_EXTENSIONS:
 *
 * Get the UUID of the %MBIM_SERVICE_MS_VOICE_EXTENSIONS service.
 *
 * Returns: (transfer none): a #MbimUuid.
 *
 * Since: 1.28
 */
#define MBIM_UUID_MS_VOICE_EXTENSIONS mbim_uuid_from_service (MBIM_SERVICE_MS_VOICE_EXTENSIONS)

/**
 * mbim_service_lookup_name:
 * @service: a MbimService or custom service.
 *
 * Gets the nickname string for the @service.
 *
 * As opposed to mbim_service_get_string(), this methods takes into account
 * custom services that may have been registered by the user.
 *
 * Returns: (transfer none): a string with the nickname, or %NULL if not found. Do not free the returned value.
 *
 * Since: 1.10
 */
const gchar *mbim_service_lookup_name (guint service);

/**
 * mbim_register_custom_service:
 * @uuid: MbimUuid structure corresponding to service
 * @nickname: a printable name for service
 *
 * Register a custom service
 *
 * Returns: TRUE if service has been registered, FALSE otherwise.
 *
 * Since: 1.10
 */
guint mbim_register_custom_service (const MbimUuid *uuid,
                                    const gchar    *nickname);

/**
 * mbim_unregister_custom_service:
 * @id: ID of the service to unregister.MbimUuid structure corresponding to service
 *
 * Unregister a custom service.
 *
 * Returns: TRUE if service has been unregistered, FALSE otherwise.
 *
 * Since: 1.10
 */
gboolean mbim_unregister_custom_service (const guint id);

/**
 * mbim_service_id_is_custom:
 * @id: ID of the service
 *
 * Checks whether @id is a custom or standard service.
 *
 * Returns: TRUE if service is custom, FALSE otherwise.
 *
 * Since: 1.10
 */
gboolean mbim_service_id_is_custom (const guint id);

/**
 * mbim_uuid_from_service:
 * @service: a #MbimService.
 *
 * Get the UUID corresponding to @service.
 *
 * The @service needs to be either a generic one (including #MBIM_SERVICE_INVALID)
 * or a custom registered one.
 *
 * Returns: (transfer none): a #MbimUuid.
 *
 * Since: 1.0
 */
const MbimUuid *mbim_uuid_from_service (MbimService service);

/**
 * mbim_uuid_to_service:
 * @uuid: a #MbimUuid.
 *
 * Get the service corresponding to @uuid.
 *
 * Returns: a #MbimService.
 *
 * Since: 1.0
 */
MbimService mbim_uuid_to_service (const MbimUuid *uuid);

/*****************************************************************************/

/**
 * MbimContextType:
 * @MBIM_CONTEXT_TYPE_INVALID: Invalid context type.
 * @MBIM_CONTEXT_TYPE_NONE: Context not yet provisioned.
 * @MBIM_CONTEXT_TYPE_INTERNET: Context representing a connection to the
 *  Internet.
 * @MBIM_CONTEXT_TYPE_VPN: Context representing a connection to a a private
 *  network (e.g. VPN to a corporate network).
 * @MBIM_CONTEXT_TYPE_VOICE: Context representing a connection to a VoIP service.
 * @MBIM_CONTEXT_TYPE_VIDEO_SHARE: Context representing a connection to a video
 *  sharing service.
 * @MBIM_CONTEXT_TYPE_PURCHASE: Context representing a connection to an
 *  OTA (over-the-air) activation site.
 * @MBIM_CONTEXT_TYPE_IMS: Context representing a connection to IMS.
 * @MBIM_CONTEXT_TYPE_MMS: Context representing a connection to MMS.
 * @MBIM_CONTEXT_TYPE_LOCAL: Context representing a connection which is
 *  terminated at the device. No IP traffic sent over the air.
 * @MBIM_CONTEXT_TYPE_ADMIN: Context used for administrative purposes, such as
 *  device management (MS MBIMEx). Since 1.28.
 * @MBIM_CONTEXT_TYPE_APP: Context used for certain applications allowed by
 *  mobile operators (MS MBIMEx). Since 1.28.
 * @MBIM_CONTEXT_TYPE_XCAP: Context used for XCAP provisioning on IMS services
 *  (MS MBIMEx). Since 1.28.
 * @MBIM_CONTEXT_TYPE_TETHERING: Context used for mobile hotspot tethering
 *  (MS MBIMEx). Since 1.28.
 * @MBIM_CONTEXT_TYPE_EMERGENCY_CALLING: Context used for IMS emergency calling
 *  (MS MBIMEx). Since 1.28.
 *
 * Enumeration of the generic MBIM context types.
 *
 * Since: 1.0
 */
typedef enum { /*< since=1.0 >*/
    MBIM_CONTEXT_TYPE_INVALID           = 0,
    MBIM_CONTEXT_TYPE_NONE              = 1,
    MBIM_CONTEXT_TYPE_INTERNET          = 2,
    MBIM_CONTEXT_TYPE_VPN               = 3,
    MBIM_CONTEXT_TYPE_VOICE             = 4,
    MBIM_CONTEXT_TYPE_VIDEO_SHARE       = 5,
    MBIM_CONTEXT_TYPE_PURCHASE          = 6,
    MBIM_CONTEXT_TYPE_IMS               = 7,
    MBIM_CONTEXT_TYPE_MMS               = 8,
    MBIM_CONTEXT_TYPE_LOCAL             = 9,
    MBIM_CONTEXT_TYPE_ADMIN             = 10,
    MBIM_CONTEXT_TYPE_APP               = 11,
    MBIM_CONTEXT_TYPE_XCAP              = 12,
    MBIM_CONTEXT_TYPE_TETHERING         = 13,
    MBIM_CONTEXT_TYPE_EMERGENCY_CALLING = 14,
} MbimContextType;

/**
 * mbim_uuid_from_context_type:
 * @context_type: a #MbimContextType.
 *
 * Get the UUID corresponding to @context_type.
 *
 * Returns: (transfer none): a #MbimUuid.
 *
 * Since: 1.0
 */
const MbimUuid *mbim_uuid_from_context_type (MbimContextType context_type);

/**
 * mbim_uuid_to_context_type:
 * @uuid: a #MbimUuid.
 *
 * Get the context type corresponding to @uuid.
 *
 * Returns: a #MbimContextType.
 *
 * Since: 1.0
 */
MbimContextType mbim_uuid_to_context_type (const MbimUuid *uuid);

G_END_DECLS

#endif /* _LIBMBIM_GLIB_MBIM_UUID_H_ */
