/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2014-2021 Aleksander Morgado <aleksander@aleksander.es>
 */

#ifndef _LIBMBIM_GLIB_MBIM_COMPAT_H_
#define _LIBMBIM_GLIB_MBIM_COMPAT_H_

#if !defined (__LIBMBIM_GLIB_H_INSIDE__) && !defined (LIBMBIM_GLIB_COMPILATION)
#error "Only <libmbim-glib.h> can be included directly."
#endif

#include <glib.h>

#include "mbim-basic-connect.h"
#include "mbim-ms-basic-connect-extensions.h"
#include "mbim-cid.h"

G_BEGIN_DECLS

/**
 * SECTION: mbim-compat
 * @title: Deprecated API
 * @short_description: Types and functions flagged as deprecated.
 *
 * This section defines types and functions that have been deprecated.
 */

#ifndef MBIM_DISABLE_DEPRECATED

/*****************************************************************************/
/* Registration flags name fixup */

/* The following type exists just so that we can get deprecation warnings */
G_DEPRECATED
typedef int MbimDeprecatedRegistrationFlag;

/**
 * MBIM_REGISTRATION_FLAG_MANUAL_PACKET_SERVICE_AUTOMATIC_ATTACH:
 *
 * Modem should auto-attach to the network after registration.
 *
 * Since: 1.0
 * Deprecated: 1.8: Use MBIM_REGISTRATION_FLAG_PACKET_SERVICE_AUTOMATIC_ATTACH instead.
 */
#define MBIM_REGISTRATION_FLAG_MANUAL_PACKET_SERVICE_AUTOMATIC_ATTACH (MbimDeprecatedRegistrationFlag) MBIM_REGISTRATION_FLAG_PACKET_SERVICE_AUTOMATIC_ATTACH

/*****************************************************************************/
/* 'Service Subscriber List' rename to 'Service Subscribe List' */

/* The following type exists just so that we can get deprecation warnings */
G_DEPRECATED
typedef int MbimDeprecatedCidBasicConnect;

/**
 * MBIM_CID_BASIC_CONNECT_DEVICE_SERVICE_SUBSCRIBER_LIST:
 *
 * Device service subscribe list.
 *
 * Since: 1.0
 * Deprecated: 1.8: Use MBIM_CID_BASIC_CONNECT_DEVICE_SERVICE_SUBSCRIBE_LIST instead.
 */
#define MBIM_CID_BASIC_CONNECT_DEVICE_SERVICE_SUBSCRIBER_LIST (MbimDeprecatedCidBasicConnect) MBIM_CID_BASIC_CONNECT_DEVICE_SERVICE_SUBSCRIBE_LIST

/**
 * mbim_message_device_service_subscriber_list_set_new:
 * @events_count: the 'EventsCount' field, given as a #guint32.
 * @events: the 'Events' field, given as an array of #MbimEventEntrys.
 * @error: return location for error or %NULL.
 *
 * Create a new request for the 'Device Service Subscribe List' set command in the 'Basic Connect' service.
 *
 * Returns: a newly allocated #MbimMessage, which should be freed with mbim_message_unref().
 *
 * Since: 1.0
 * Deprecated: 1.8: Use mbim_message_device_service_subscribe_list_set_new() instead.
 */
G_DEPRECATED_FOR (mbim_message_device_service_subscribe_list_set_new)
MbimMessage *mbim_message_device_service_subscriber_list_set_new (
    guint32 events_count,
    const MbimEventEntry *const *events,
    GError **error);

/**
 * mbim_message_device_service_subscriber_list_response_parse:
 * @message: the #MbimMessage.
 * @events_count: return location for a #guint32, or %NULL if the 'EventsCount' field is not needed.
 * @events: return location for a newly allocated array of #MbimEventEntrys, or %NULL if the 'Events' field is not needed. Free the returned value with mbim_event_entry_array_free().
 * @error: return location for error or %NULL.
 *
 * Create a new request for the 'Events' response command in the 'Basic Connect' service.
 *
 * Returns: %TRUE if the message was correctly parsed, %FALSE if @error is set.
 *
 * Since: 1.0
 * Deprecated: 1.8: Use mbim_message_device_service_subscribe_list_response_parse() instead.
 */
G_DEPRECATED_FOR (mbim_message_device_service_subscribe_list_response_parse)
gboolean mbim_message_device_service_subscriber_list_response_parse (
    const MbimMessage *message,
    guint32 *events_count,
    MbimEventEntry ***events,
    GError **error);

/*****************************************************************************/
/* 'LTE Attach Status' rename to 'LTE Attach Info', to avoid the unneeded
 * MbimLteAttachStatus struct */

/* The following type exists just so that we can get deprecation warnings */
G_DEPRECATED
typedef int MbimDeprecatedCidMsBasicConnectExtensions;

/**
 * MBIM_CID_MS_BASIC_CONNECT_EXTENSIONS_LTE_ATTACH_STATUS:
 *
 * LTE attach info.
 *
 * Since: 1.18
 * Deprecated: 1.26: Use MBIM_CID_MS_BASIC_CONNECT_EXTENSIONS_LTE_ATTACH_INFO instead.
 */
#define MBIM_CID_MS_BASIC_CONNECT_EXTENSIONS_LTE_ATTACH_STATUS (MbimDeprecatedCidMsBasicConnectExtensions)MBIM_CID_MS_BASIC_CONNECT_EXTENSIONS_LTE_ATTACH_INFO

/**
 * MbimLteAttachStatus:
 * @lte_attach_state: a #guint32.
 * @ip_type: a #guint32.
 * @access_string: a string.
 * @user_name: a string.
 * @password: a string.
 * @compression: a #guint32.
 * @auth_protocol: a #guint32.
 *
 * LTE attach status information.
 *
 * Since: 1.18
 * Deprecated: 1.26
 */

/* The following type exists just so that we don't getdeprecation warnings on
 * our own methods */
typedef struct {
    guint32 lte_attach_state;
    guint32 ip_type;
    gchar *access_string;
    gchar *user_name;
    gchar *password;
    guint32 compression;
    guint32 auth_protocol;
} MbimDeprecatedLteAttachStatus;

G_DEPRECATED
typedef MbimDeprecatedLteAttachStatus MbimLteAttachStatus;

/**
 * mbim_lte_attach_status_free:
 * @var: a #MbimLteAttachStatus.
 *
 * Frees the memory allocated for the #MbimLteAttachStatus.
 *
 * Since: 1.18
 * Deprecated: 1.26
 */
G_DEPRECATED
void mbim_lte_attach_status_free (MbimDeprecatedLteAttachStatus *var);

#if defined(__clang__)
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wdeprecated-declarations"
#elif defined(__GNUC__)
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
G_DEFINE_AUTOPTR_CLEANUP_FUNC (MbimLteAttachStatus, mbim_lte_attach_status_free)
#if defined(__clang__)
# pragma clang diagnostic pop
#elif defined(__GNUC__)
# pragma GCC diagnostic pop
#endif

/**
 * mbim_message_ms_basic_connect_extensions_lte_attach_status_query_new:
 * @error: return location for error or %NULL.
 *
 * Create a new request for the 'Lte Attach Status' query command in the 'Ms Basic Connect Extensions' service.
 *
 * Returns: a newly allocated #MbimMessage, which should be freed with mbim_message_unref().
 *
 * Since: 1.18
 * Deprecated: 1.26: Use mbim_message_ms_basic_connect_extensions_lte_attach_info_query_new() instead.
 */
G_DEPRECATED_FOR (mbim_message_ms_basic_connect_extensions_lte_attach_info_query_new)
MbimMessage *mbim_message_ms_basic_connect_extensions_lte_attach_status_query_new (
    GError **error);

/**
 * mbim_message_ms_basic_connect_extensions_lte_attach_status_response_parse:
 * @message: the #MbimMessage.
 * @out_lte_attach_status: (out)(optional)(transfer full): return location for a newly allocated #MbimLteAttachStatus, or %NULL if the 'LteAttachStatus' field is not needed. Free the returned value with mbim_lte_attach_status_free().
 * @error: return location for error or %NULL.
 *
 * Parses and returns parameters of the 'Lte Attach Status' response command in the 'Ms Basic Connect Extensions' service.
 *
 * Returns: %TRUE if the message was correctly parsed, %FALSE if @error is set.
 *
 * Since: 1.18
 * Deprecated: 1.26: Use mbim_message_ms_basic_connect_extensions_lte_attach_info_response_parse() instead.
 */
G_DEPRECATED_FOR (mbim_message_ms_basic_connect_extensions_lte_attach_info_response_parse)
gboolean mbim_message_ms_basic_connect_extensions_lte_attach_status_response_parse (
    const MbimMessage *message,
    MbimDeprecatedLteAttachStatus **out_lte_attach_status,
    GError **error);

/**
 * mbim_message_ms_basic_connect_extensions_lte_attach_status_notification_parse:
 * @message: the #MbimMessage.
 * @out_lte_attach_status: (out)(optional)(transfer full): return location for a newly allocated #MbimLteAttachStatus, or %NULL if the 'LteAttachStatus' field is not needed. Free the returned value with mbim_lte_attach_status_free().
 * @error: return location for error or %NULL.
 *
 * Parses and returns parameters of the 'Lte Attach Status' notification command in the 'Ms Basic Connect Extensions' service.
 *
 * Returns: %TRUE if the message was correctly parsed, %FALSE if @error is set.
 *
 * Since: 1.18
 * Deprecated: 1.26: Use mbim_message_ms_basic_connect_extensions_lte_attach_info_notification_parse() instead.
 */
G_DEPRECATED_FOR (mbim_message_ms_basic_connect_extensions_lte_attach_info_notification_parse)
gboolean mbim_message_ms_basic_connect_extensions_lte_attach_status_notification_parse (
    const MbimMessage *message,
    MbimDeprecatedLteAttachStatus **out_lte_attach_status,
    GError **error);

/*****************************************************************************/
/* Network errors fixup */

/* The following type exists just so that we can get deprecation warnings */
G_DEPRECATED
typedef int MbimDeprecatedNwError;

/**
 * MBIM_NW_ERROR_UNKNOWN:
 *
 * Network error not set.
 *
 * Since: 1.0
 * Deprecated: 1.28: Use %MBIM_NW_ERROR_NONE instead.
 */
#define MBIM_NW_ERROR_UNKNOWN (MbimDeprecatedNwError) MBIM_NW_ERROR_NONE

/*****************************************************************************/
/* Rename blacklist to denylist */

/* The following type exists just so that we can get deprecation warnings */
G_DEPRECATED
typedef int MbimDeprecatedCidMsBasicConnectExtensions;

/**
 * MBIM_CID_MS_BASIC_CONNECT_EXTENSIONS_NETWORK_BLACKLIST:
 *
 * Network deny list.
 *
 * Since: 1.18
 * Deprecated: 1.28: Use MBIM_CID_MS_BASIC_CONNECT_EXTENSIONS_NETWORK_DENYLIST instead.
 */
#define MBIM_CID_MS_BASIC_CONNECT_EXTENSIONS_NETWORK_BLACKLIST (MbimDeprecatedCidMsBasicConnectExtensions) MBIM_CID_MS_BASIC_CONNECT_EXTENSIONS_NETWORK_DENYLIST

#endif /* MBIM_DISABLE_DEPRECATED */

G_END_DECLS

#endif /* _LIBMBIM_GLIB_MBIM_COMPAT_H_ */
