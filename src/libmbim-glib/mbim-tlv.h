/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * libmbim-glib -- GLib/GIO based library to control MBIM devices
 *
 * Copyright (C) 2021 Aleksander Morgado <aleksander@aleksander.es>
 * Copyright (C) 2021 Intel Corporation
 */

#ifndef _LIBMBIM_GLIB_MBIM_TLV_H_
#define _LIBMBIM_GLIB_MBIM_TLV_H_

#if !defined (__LIBMBIM_GLIB_H_INSIDE__) && !defined (LIBMBIM_GLIB_COMPILATION)
#error "Only <libmbim-glib.h> can be included directly."
#endif

#include <glib.h>
#include <glib-object.h>

#include "mbim-uuid.h"

G_BEGIN_DECLS

/**
 * SECTION:mbim-tlv
 * @title: MbimTlv
 * @short_description: A variable-sized data structure specified in Microsoft
 *   MBIM extension v3.0.
 *
 * The #MbimTlv structure is capable of exchanging a wide range of information
 * between an MBIM host and an MBIM device.
 *
 * CID payloads for requests, responses, and/or notifications may contain zero
 * or more unnamed and optional Information Elements (IE) encoded as #MbimTlv
 * fields.
 */

/**
 * MbimTlv:
 *
 * An opaque type representing a MBIM TLV.
 *
 * Since: 1.28
 */
typedef struct _MbimTlv MbimTlv;

GType mbim_tlv_get_type (void) G_GNUC_CONST;
#define MBIM_TYPE_TLV (mbim_tlv_get_type ())

/*****************************************************************************/
/* Generic TLV interface */

/**
 * MbimTlvType:
 * @MBIM_TLV_TYPE_INVALID: Invalid TLV type.
 * @MBIM_TLV_TYPE_UE_POLICITES: UE policies.
 * @MBIM_TLV_TYPE_SINGLE_NSSAI: Single NSSAI.
 * @MBIM_TLV_TYPE_ALLOWED_NSSAI: Allowed NSSAI.
 * @MBIM_TLV_TYPE_CFG_NSSAI: Configured NSSAI.
 * @MBIM_TLV_TYPE_DFLT_CFG_NSSAI: Default configured NSSAI.
 * @MBIM_TLV_TYPE_PRECFG_DFLT_CFG_NSSAI: Preconfigured default configured NSSAI.
 * @MBIM_TLV_TYPE_REJ_NSSAI: Rejected NSSAI.
 * @MBIM_TLV_TYPE_LADN: Local Area Data Network (LADN).
 * @MBIM_TLV_TYPE_TAI: Tracking Area Identity (TAI).
 * @MBIM_TLV_TYPE_WCHAR_STR: WCHAR string.
 * @MBIM_TLV_TYPE_UINT16_TBL: Array of 1 or more @guint16 entries.
 * @MBIM_TLV_TYPE_EAP_PACKET: Extensible Authentication Protocol packet.
 * @MBIM_TLV_TYPE_PCO: Protocol Configuration Option (PCO).
 * @MBIM_TLV_TYPE_ROUTE_SELECTION_DESCRIPTORS: One or more route selection descriptors.
 * @MBIM_TLV_TYPE_TRAFFIC_PARAMETERS: A traffic parameters record.
 * @MBIM_TLV_TYPE_WAKE_COMMAND: Wake command.
 * @MBIM_TLV_TYPE_WAKE_PACKET: Wake packet.
 *
 * Type of the MBIM TLV.
 *
 * Since: 1.28
 */
typedef enum { /*< since=1.28 >*/
    MBIM_TLV_TYPE_INVALID                     = 0,
    MBIM_TLV_TYPE_UE_POLICITES                = 1,
    MBIM_TLV_TYPE_SINGLE_NSSAI                = 2,
    MBIM_TLV_TYPE_ALLOWED_NSSAI               = 3,
    MBIM_TLV_TYPE_CFG_NSSAI                   = 4,
    MBIM_TLV_TYPE_DFLT_CFG_NSSAI              = 5,
    MBIM_TLV_TYPE_PRECFG_DFLT_CFG_NSSAI       = 6,
    MBIM_TLV_TYPE_REJ_NSSAI                   = 7,
    MBIM_TLV_TYPE_LADN                        = 8,
    MBIM_TLV_TYPE_TAI                         = 9,
    MBIM_TLV_TYPE_WCHAR_STR                   = 10,
    MBIM_TLV_TYPE_UINT16_TBL                  = 11,
    MBIM_TLV_TYPE_EAP_PACKET                  = 12,
    MBIM_TLV_TYPE_PCO                         = 13,
    MBIM_TLV_TYPE_ROUTE_SELECTION_DESCRIPTORS = 14,
    MBIM_TLV_TYPE_TRAFFIC_PARAMETERS          = 15,
    MBIM_TLV_TYPE_WAKE_COMMAND                = 16,
    MBIM_TLV_TYPE_WAKE_PACKET                 = 17,
} MbimTlvType;

/**
 * mbim_tlv_new:
 * @type: a #MbimTlvType.
 * @data: contents of the TLV.
 * @data_length: length of the message.
 *
 * Create a #MbimTlv with the given contents.
 *
 * Returns: (transfer full): a newly created #MbimTlv, which should be freed with mbim_tlv_unref().
 *
 * Since: 1.28
 */
MbimTlv *mbim_tlv_new (MbimTlvType   type,
                       const guint8 *data,
                       guint32       data_length);

/**
 * mbim_tlv_dup:
 * @self: a #MbimTlv to duplicate.
 *
 * Create a #MbimTlv with the same contents as @self.
 *
 * Returns: (transfer full): a newly created #MbimTlv, which should be freed with mbim_tlv_unref().
 *
 * Since: 1.28
 */
MbimTlv *mbim_tlv_dup (const MbimTlv *self);

/**
 * mbim_tlv_ref:
 * @self: a #MbimTlv.
 *
 * Atomically increments the reference count of @self by one.
 *
 * Returns: (transfer full): the new reference to @self.
 *
 * Since: 1.28
 */
MbimTlv *mbim_tlv_ref (MbimTlv *self);

/**
 * mbim_tlv_unref:
 * @self: a #MbimTlv.
 *
 * Atomically decrements the reference count of @self by one.
 * If the reference count drops to 0, @self is completely disposed.
 *
 * Since: 1.28
 */
void mbim_tlv_unref (MbimTlv *self);

/**
 * mbim_tlv_get_raw:
 * @self: a #MbimTlv.
 * @length: (out): return location for the size of the output buffer.
 * @error: return location for error or %NULL.
 *
 * Gets the whole raw data buffer of the #MbimTlv.
 *
 * Returns: The raw data buffer, or #NULL if @error is set.
 *
 * Since: 1.28
 */
const guint8 *mbim_tlv_get_raw (const MbimTlv  *self,
                                guint32        *length,
                                GError        **error);

/**
 * mbim_tlv_get_tlv_type:
 * @self: a #MbimTlv.
 *
 * Gets the message type.
 *
 * Returns: a #MbimTlvType.
 *
 * Since: 1.28
 */
MbimTlvType mbim_tlv_get_tlv_type (const MbimTlv *self);

/**
 * mbim_tlv_get_tlv_data:
 * @self: a #MbimTlv.
 * @out_length: (out): return location for the size of the output buffer.
 *
 * Gets the TLV raw data.
 *
 * Returns: (transfer none): The raw data buffer, or #NULL if empty.
 *
 * Since: 1.28
 */
const guint8 *mbim_tlv_get_tlv_data (const MbimTlv *self,
                                     guint32       *out_length);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (MbimTlv, mbim_tlv_unref)

/*****************************************************************************/
/* String TLV type helpers */

/**
 * mbim_tlv_string_new:
 * @str: a string.
 * @error: return location for error or %NULL.
 *
 * Create a #MbimTlv of type %MBIM_TLV_TYPE_WCHAR_STR with the given contents.
 *
 * Returns: (transfer full): a newly created #MbimTlv which should be freed with mbim_tlv_unref(), or %NULL if @error is set.
 *
 * Since: 1.28
 */
MbimTlv *mbim_tlv_string_new (const gchar  *str,
                              GError      **error);

/**
 * mbim_tlv_string_get:
 * @self: a #MbimTlv of type %MBIM_TLV_TYPE_WCHAR_STR.
 * @error: return location for error or %NULL.
 *
 * Get a string with the contents in the #MbimTlv.
 *
 * Returns: (transfer full): a newly created string, which should be freed with g_free(), or %NULL if @error is set.
 *
 * Since: 1.28
 */
gchar *mbim_tlv_string_get (const MbimTlv  *self,
                            GError        **error);

/*****************************************************************************/
/* guint16 array type helpers */

/**
 * mbim_tlv_guint16_array_get:
 * @self: a #MbimTlv of type %MBIM_TLV_TYPE_UINT16_TBL.
 * @array_size: (out)(optional)(transfer none): return location for a #guint32,
 *  or %NULL if the field is not needed.
 * @array: (out)(optional)(transfer full)(type guint16): return location for a
 *  newly allocated array of #guint16 values, or %NULL if the field is not
 *  needed. Free the returned value with g_free().
 * @error: return location for error or %NULL.
 *
 * Get an array of #guint16 values with the contents in the #MbimTlv.
 *
 * The method may return a successful return even with on empty arrays (i.e.
 * with @array_size set to 0 and @array set to %NULL).
 *
 * Returns: %TRUE if on success, %FALSE if @error is set.
 *
 * Since: 1.28
 */
gboolean mbim_tlv_guint16_array_get (const MbimTlv  *self,
                                     guint32        *array_size,
                                     guint16       **array,
                                     GError        **error);

/*****************************************************************************/
/* wake command type helpers */

/**
 * mbim_tlv_wake_command_get:
 * @self: a #MbimTlv of type %MBIM_TLV_TYPE_WAKE_COMMAND.
 * @service: (out)(optional)(transfer none): return location for a #MbimUuid
 *   specifying the service that triggered the wake.
 * @cid: (out)(optional)(transfer none): return location for the command id that
 *   triggered the wake.
 * @payload_size: (out)(optional)(transfer none): return location for a #guint32,
 *  or %NULL if the field is not needed.
 * @payload: (out)(optional)(transfer full)(type guint8): return location for a
 *  newly allocated array of #guint8 values, or %NULL if the field is not
 *  needed. Free the returned value with g_free().
 * @error: return location for error or %NULL.
 *
 * Get the contents of a wake command TLV.
 *
 * The method may return a successful return even with on empty payload (i.e.
 * with @payload_size set to 0 and @payload set to %NULL).
 *
 * Returns: %TRUE if on success, %FALSE if @error is set.
 *
 * Since: 1.28
 */
gboolean mbim_tlv_wake_command_get (const MbimTlv   *self,
                                    const MbimUuid **service,
                                    guint32         *cid,
                                    guint32         *payload_size,
                                    guint8         **payload,
                                    GError         **error);

/*****************************************************************************/
/* wake packet type helpers */

/**
 * mbim_tlv_wake_packet_get:
 * @self: a #MbimTlv of type %MBIM_TLV_TYPE_WAKE_PACKET.
 * @filter_id: (out)(optional)(transfer none): return location for a #guint32
 *   specifying the filter id.
 * @original_packet_size: (out)(optional)(transfer none): return location for a
 *  #guint32, or %NULL if the field is not needed.
 * @packet_size: (out)(optional)(transfer none): return location for a #guint32,
 *  or %NULL if the field is not needed.
 * @packet: (out)(optional)(transfer full)(type guint8): return location for a
 *  newly allocated array of #guint8 values, or %NULL if the field is not
 *  needed. Free the returned value with g_free().
 * @error: return location for error or %NULL.
 *
 * Get the contents of a wake packet TLV.
 *
 * Returns: %TRUE if on success, %FALSE if @error is set.
 *
 * Since: 1.28
 */
gboolean mbim_tlv_wake_packet_get (const MbimTlv  *self,
                                   guint32        *filter_id,
                                   guint32        *original_packet_size,
                                   guint32        *packet_size,
                                   guint8        **packet,
                                   GError        **error);

G_END_DECLS

#endif /* _LIBMBIM_GLIB_MBIM_TLV_H_ */
