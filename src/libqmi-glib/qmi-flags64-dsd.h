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
 * Copyright (C) 2019 Wang Jing <clifflily@hotmail.com>
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc.
 */

#ifndef _LIBQMI_GLIB_QMI_FLAGS64_DSD_H_
#define _LIBQMI_GLIB_QMI_FLAGS64_DSD_H_

#if !defined (__LIBQMI_GLIB_H_INSIDE__) && !defined (LIBQMI_GLIB_COMPILATION)
#error "Only <libqmi-glib.h> can be included directly."
#endif

#include "qmi-enums-dsd.h"
#include <glib.h>

/**
 * QmiDsdApnTypePreference:
 * @QMI_DSD_APN_TYPE_PREFERENCE_DEFAULT: Default/Internet traffic.
 * @QMI_DSD_APN_TYPE_PREFERENCE_IMS: IMS.
 * @QMI_DSD_APN_TYPE_PREFERENCE_MMS: Multimedia Messaging Service.
 * @QMI_DSD_APN_TYPE_PREFERENCE_DUN: Dial Up Network.
 * @QMI_DSD_APN_TYPE_PREFERENCE_SUPL: Secure User Plane Location.
 * @QMI_DSD_APN_TYPE_PREFERENCE_HIPRI: High Priority Mobile Data.
 * @QMI_DSD_APN_TYPE_PREFERENCE_FOTA: over the air administration.
 * @QMI_DSD_APN_TYPE_PREFERENCE_CBS: Carrier Branded Services.
 * @QMI_DSD_APN_TYPE_PREFERENCE_IA: Initial Attach.
 * @QMI_DSD_APN_TYPE_PREFERENCE_EMERGENCY: Emergency.
 *
 * APN type preference as a bitmask.
 *
 * Since: 1.26
 */
typedef enum { /*< since=1.26 >*/
    QMI_DSD_APN_TYPE_PREFERENCE_DEFAULT   = ((guint64) 1) << QMI_DSD_APN_TYPE_DEFAULT,
    QMI_DSD_APN_TYPE_PREFERENCE_IMS       = ((guint64) 1) << QMI_DSD_APN_TYPE_IMS,
    QMI_DSD_APN_TYPE_PREFERENCE_MMS       = ((guint64) 1) << QMI_DSD_APN_TYPE_MMS,
    QMI_DSD_APN_TYPE_PREFERENCE_DUN       = ((guint64) 1) << QMI_DSD_APN_TYPE_DUN,
    QMI_DSD_APN_TYPE_PREFERENCE_SUPL      = ((guint64) 1) << QMI_DSD_APN_TYPE_SUPL,
    QMI_DSD_APN_TYPE_PREFERENCE_HIPRI     = ((guint64) 1) << QMI_DSD_APN_TYPE_HIPRI,
    QMI_DSD_APN_TYPE_PREFERENCE_FOTA      = ((guint64) 1) << QMI_DSD_APN_TYPE_FOTA,
    QMI_DSD_APN_TYPE_PREFERENCE_CBS       = ((guint64) 1) << QMI_DSD_APN_TYPE_CBS,
    QMI_DSD_APN_TYPE_PREFERENCE_IA        = ((guint64) 1) << QMI_DSD_APN_TYPE_IA,
    QMI_DSD_APN_TYPE_PREFERENCE_EMERGENCY = ((guint64) 1) << QMI_DSD_APN_TYPE_EMERGENCY,
} QmiDsdApnTypePreference;

/**
 * QmiDsdSoMask:
 * @QMI_DSD_3GPP_SO_MASK_WCDMA: WCDMA.
 * @QMI_DSD_3GPP_SO_MASK_HSDPA: HSDPA.
 * @QMI_DSD_3GPP_SO_MASK_HSUPA: HSUPA.
 * @QMI_DSD_3GPP_SO_MASK_HSDPAPLUS: HSDPAPLUS.
 * @QMI_DSD_3GPP_SO_MASK_DC_HSDPAPLUS: DC HSDPAPLUS.
 * @QMI_DSD_3GPP_SO_MASK_64_QAM: 64 QAM.
 * @QMI_DSD_3GPP_SO_MASK_HSPA: HSPA.
 * @QMI_DSD_3GPP_SO_MASK_GPRS: GPRS.
 * @QMI_DSD_3GPP_SO_MASK_EDGE: EDGE.
 * @QMI_DSD_3GPP_SO_MASK_GSM: GSM.
 * @QMI_DSD_3GPP_SO_MASK_S2B: S2B.
 * @QMI_DSD_3GPP_SO_MASK_LTE_LIMITED_SRVC: LTE Limited Service.
 * @QMI_DSD_3GPP_SO_MASK_LTE_FDD: LTE FDD.
 * @QMI_DSD_3GPP_SO_MASK_LTE_TDD: LTE TDD.
 * @QMI_DSD_3GPP_SO_MASK_TDSCDMA: TDSCDMA.
 * @QMI_DSD_3GPP_SO_MASK_DC_HSUPA: DC_HSUPA.
 * @QMI_DSD_3GPP_SO_MASK_LTE_CA_DL: LTE CA DL.
 * @QMI_DSD_3GPP_SO_MASK_LTE_CA_UL: LTE CA UL.
 * @QMI_DSD_3GPP_SO_MASK_S2B_LIMITED_SRVC: S2B Limited Service.
 * @QMI_DSD_3GPP_SO_MASK_FOUR_POINT_FIVE_G: 4.5G.
 * @QMI_DSD_3GPP_SO_MASK_FOUR_POINT_FIVE_G_PLUS: 4.5G+.
 * @QMI_DSD_3GPP2_SO_MASK_1X_IS95: 1X IS95.
 * @QMI_DSD_3GPP2_SO_MASK_1X_IS2000: 1X IS2000.
 * @QMI_DSD_3GPP2_SO_MASK_1X_IS2000_REL_A: 1X IS2000 REL A.
 * @QMI_DSD_3GPP2_SO_MASK_HDR_REV0_DPA: HDR REV0 DPA.
 * @QMI_DSD_3GPP2_SO_MASK_HDR_REVA_DPA: HDR REVB DPA.
 * @QMI_DSD_3GPP2_SO_MASK_HDR_REVB_DPA: HDR REVB DPA.
 * @QMI_DSD_3GPP2_SO_MASK_HDR_REVA_MPA: HDR REVA MPA.
 * @QMI_DSD_3GPP2_SO_MASK_HDR_REVB_MPA: HDR REVB MPA.
 * @QMI_DSD_3GPP2_SO_MASK_HDR_REVA_EMPA: HDR REVA EMPA.
 * @QMI_DSD_3GPP2_SO_MASK_HDR_REVB_EMPA: HDR REVB EMPA.
 * @QMI_DSD_3GPP2_SO_MASK_HDR_REVB_MMPA: HDR REVB MMPA.
 * @QMI_DSD_3GPP2_SO_MASK_HDR_EVDO_FMC: HDR EVDO FMC.
 * @QMI_DSD_3GPP2_SO_MASK_1X_CS: 1X Circuit Switched.
 * @QMI_DSD_3GPP_SO_MASK_5G_TDD: 5G TDD.
 * @QMI_DSD_3GPP_SO_MASK_5G_SUB6: 5G SUB6.
 * @QMI_DSD_3GPP_SO_MASK_5G_MMWAVE: 5G MMWAVE.
 * @QMI_DSD_3GPP_SO_MASK_5G_NSA: 5G NSA.
 * @QMI_DSD_3GPP_SO_MASK_5G_SA: 5G SA.
 *
 * Service Option (SO) mask.
 *
 * Since: 1.32
 */
typedef enum { /*< since=1.32 >*/
  QMI_DSD_3GPP_SO_MASK_WCDMA                  = 1 << 0,
  QMI_DSD_3GPP_SO_MASK_HSDPA                  = 1 << 1,
  QMI_DSD_3GPP_SO_MASK_HSUPA                  = 1 << 2,
  QMI_DSD_3GPP_SO_MASK_HSDPAPLUS              = 1 << 3,
  QMI_DSD_3GPP_SO_MASK_DC_HSDPAPLUS           = 1 << 4,
  QMI_DSD_3GPP_SO_MASK_64_QAM                 = 1 << 5,
  QMI_DSD_3GPP_SO_MASK_HSPA                   = 1 << 6,
  QMI_DSD_3GPP_SO_MASK_GPRS                   = 1 << 7,
  QMI_DSD_3GPP_SO_MASK_EDGE                   = 1 << 8,
  QMI_DSD_3GPP_SO_MASK_GSM                    = 1 << 9,
  QMI_DSD_3GPP_SO_MASK_S2B                    = 1 << 10,
  QMI_DSD_3GPP_SO_MASK_LTE_LIMITED_SRVC       = 1 << 11,
  QMI_DSD_3GPP_SO_MASK_LTE_FDD                = 1 << 12,
  QMI_DSD_3GPP_SO_MASK_LTE_TDD                = 1 << 13,
  QMI_DSD_3GPP_SO_MASK_TDSCDMA                = 1 << 14,
  QMI_DSD_3GPP_SO_MASK_DC_HSUPA               = 1 << 15,
  QMI_DSD_3GPP_SO_MASK_LTE_CA_DL              = 1 << 16,
  QMI_DSD_3GPP_SO_MASK_LTE_CA_UL              = 1 << 17,
  QMI_DSD_3GPP_SO_MASK_S2B_LIMITED_SRVC       = 1 << 18,
  QMI_DSD_3GPP_SO_MASK_FOUR_POINT_FIVE_G      = 1 << 19,
  QMI_DSD_3GPP_SO_MASK_FOUR_POINT_FIVE_G_PLUS = 1 << 20,
  QMI_DSD_3GPP2_SO_MASK_1X_IS95               = 1 << 24,
  QMI_DSD_3GPP2_SO_MASK_1X_IS2000             = 1 << 25,
  QMI_DSD_3GPP2_SO_MASK_1X_IS2000_REL_A       = 1 << 26,
  QMI_DSD_3GPP2_SO_MASK_HDR_REV0_DPA          = 1 << 27,
  QMI_DSD_3GPP2_SO_MASK_HDR_REVA_DPA          = 1 << 28,
  QMI_DSD_3GPP2_SO_MASK_HDR_REVB_DPA          = 1 << 29,
  QMI_DSD_3GPP2_SO_MASK_HDR_REVA_MPA          = 1 << 30,
  QMI_DSD_3GPP2_SO_MASK_HDR_REVB_MPA          = ((guint64) 1) << 31,
  QMI_DSD_3GPP2_SO_MASK_HDR_REVA_EMPA         = ((guint64) 1) << 32,
  QMI_DSD_3GPP2_SO_MASK_HDR_REVB_EMPA         = ((guint64) 1) << 33,
  QMI_DSD_3GPP2_SO_MASK_HDR_REVB_MMPA         = ((guint64) 1) << 34,
  QMI_DSD_3GPP2_SO_MASK_HDR_EVDO_FMC          = ((guint64) 1) << 35,
  QMI_DSD_3GPP2_SO_MASK_1X_CS                 = ((guint64) 1) << 36,
  QMI_DSD_3GPP_SO_MASK_5G_TDD                 = ((guint64) 1) << 40,
  QMI_DSD_3GPP_SO_MASK_5G_SUB6                = ((guint64) 1) << 41,
  QMI_DSD_3GPP_SO_MASK_5G_MMWAVE              = ((guint64) 1) << 42,
  QMI_DSD_3GPP_SO_MASK_5G_NSA                 = ((guint64) 1) << 43,
  QMI_DSD_3GPP_SO_MASK_5G_SA                  = ((guint64) 1) << 44,
} QmiDsdSoMask;

#endif /* _LIBQMI_GLIB_QMI_FLAGS64_DSD_H_ */
