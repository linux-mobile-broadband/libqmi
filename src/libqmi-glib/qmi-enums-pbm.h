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
 * Copyright (C) 2012 Google Inc.
 * Copyright (C) 2012-2017 Aleksander Morgado <aleksander@aleksander.es>
 */

#ifndef _LIBQMI_GLIB_QMI_ENUMS_PBM_H_
#define _LIBQMI_GLIB_QMI_ENUMS_PBM_H_

#if !defined (__LIBQMI_GLIB_H_INSIDE__) && !defined (LIBQMI_GLIB_COMPILATION)
#error "Only <libqmi-glib.h> can be included directly."
#endif

/**
 * SECTION: qmi-enums-pbm
 * @title: PBM enumerations and flags
 *
 * This section defines enumerations and flags used in the PBM service
 * interface.
 */

/*****************************************************************************/
/* Helper enums for the 'QMI PBM Indication Register' indication */

/**
 * QmiPbmEventRegistrationFlag:
 * @QMI_PBM_EVENT_REGISTRATION_FLAG_RECORD_UPDATE: Request indications when records are added/edited/deleted.
 * @QMI_PBM_EVENT_REGISTRATION_FLAG_PHONEBOOK_READY: Request indications when phonebooks are ready.
 * @QMI_PBM_EVENT_REGISTRATION_FLAG_EMERGENCY_NUMBER_LIST: Request indications when emergency numbers are changed.
 * @QMI_PBM_EVENT_REGISTRATION_FLAG_HIDDEN_RECORD_STATUS: Request indications when hidden record status is changed.
 * @QMI_PBM_EVENT_REGISTRATION_FLAG_AAS_UPDATE: Request indications when Additional number Alpha String records are added/edited/deleted.
 * @QMI_PBM_EVENT_REGISTRATION_FLAG_GAS_UPDATE: Request indications when Grouping information Alpha String records are added/edited/deleted.
 *
 * Flags to use to register to phonebook indications.
 *
 * Since: 1.6
 */
typedef enum {
    QMI_PBM_EVENT_REGISTRATION_FLAG_RECORD_UPDATE         = 1 << 0,
    QMI_PBM_EVENT_REGISTRATION_FLAG_PHONEBOOK_READY       = 1 << 1,
    QMI_PBM_EVENT_REGISTRATION_FLAG_EMERGENCY_NUMBER_LIST = 1 << 2,
    QMI_PBM_EVENT_REGISTRATION_FLAG_HIDDEN_RECORD_STATUS  = 1 << 3,
    QMI_PBM_EVENT_REGISTRATION_FLAG_AAS_UPDATE            = 1 << 4,
    QMI_PBM_EVENT_REGISTRATION_FLAG_GAS_UPDATE            = 1 << 5,
} QmiPbmEventRegistrationFlag;

/**
 * qmi_pbm_event_registration_flag_build_string_from_mask:
 *
 * Since: 1.6
 */

/*****************************************************************************/
/* Helper enums for the 'Get Capabilities' request */

/**
 * QmiPbmPhonebookType:
 * @QMI_PBM_PHONEBOOK_TYPE_ADN: Abbreviated Dialing Number.
 * @QMI_PBM_PHONEBOOK_TYPE_FDN: Fixed Dialing Number.
 * @QMI_PBM_PHONEBOOK_TYPE_MSISDN: Mobile Subscriber Integrated Services Digital Network.
 * @QMI_PBM_PHONEBOOK_TYPE_MBDN: Mail Box Dialing Number.
 * @QMI_PBM_PHONEBOOK_TYPE_SDN: Service Dialing Number.
 * @QMI_PBM_PHONEBOOK_TYPE_BDN: Barred Dialing Number.
 * @QMI_PBM_PHONEBOOK_TYPE_LND: Last Number Dialed.
 * @QMI_PBM_PHONEBOOK_TYPE_MBN: Mail Box Number.
 *
 * Phonebook type.
 *
 * Since: 1.6
 */
typedef enum {
    QMI_PBM_PHONEBOOK_TYPE_ADN    = 1 << 0,
    QMI_PBM_PHONEBOOK_TYPE_FDN    = 1 << 1,
    QMI_PBM_PHONEBOOK_TYPE_MSISDN = 1 << 2,
    QMI_PBM_PHONEBOOK_TYPE_MBDN   = 1 << 3,
    QMI_PBM_PHONEBOOK_TYPE_SDN    = 1 << 4,
    QMI_PBM_PHONEBOOK_TYPE_BDN    = 1 << 5,
    QMI_PBM_PHONEBOOK_TYPE_LND    = 1 << 6,
    QMI_PBM_PHONEBOOK_TYPE_MBN    = 1 << 7,
} QmiPbmPhonebookType;

/**
 * qmi_pbm_phonebook_type_build_string_from_mask:
 *
 * Since: 1.6
 */

/**
 * QmiPbmSessionType:
 * @QMI_PBM_SESSION_TYPE_GW_PRIMARY: Access phonebooks under GSM DF (ICC) or USIM application (UICC).
 * @QMI_PBM_SESSION_TYPE_1X_PRIMARY: Access phonebooks under CDMA DF (ICC) or CSIM application (UICC).
 * @QMI_PBM_SESSION_TYPE_GW_SECONDARY: Access phonebooks under GSM DF (ICC) or USIM application (UICC). Dual standby.
 * @QMI_PBM_SESSION_TYPE_1X_SECONDARY: Access phonebooks under CDMA DF (ICC) or CSIM application (UICC). Dual standby.
 * @QMI_PBM_SESSION_TYPE_NONPROVISIONING_SLOT_1: Access phonebooks under a nonprovisioning application in slot 1.
 * @QMI_PBM_SESSION_TYPE_NONPROVISIONING_SLOT_2: Access phonebooks under a nonprovisioning application in slot 2.
 * @QMI_PBM_SESSION_TYPE_GLOBAL_PHONEBOOK_SLOT_1: Access phonebooks that are not in any application of the card in slot 1.
 * @QMI_PBM_SESSION_TYPE_GLOBAL_PHONEBOOK_SLOT_2: Access phonebooks that are not in any application of the card in slot 2.
 *
 * Type of phonebook management session.
 *
 * Since: 1.6
 */
typedef enum {
    QMI_PBM_SESSION_TYPE_GW_PRIMARY              = 0,
    QMI_PBM_SESSION_TYPE_1X_PRIMARY              = 1,
    QMI_PBM_SESSION_TYPE_GW_SECONDARY            = 2,
    QMI_PBM_SESSION_TYPE_1X_SECONDARY            = 3,
    QMI_PBM_SESSION_TYPE_NONPROVISIONING_SLOT_1  = 4,
    QMI_PBM_SESSION_TYPE_NONPROVISIONING_SLOT_2  = 5,
    QMI_PBM_SESSION_TYPE_GLOBAL_PHONEBOOK_SLOT_1 = 6,
    QMI_PBM_SESSION_TYPE_GLOBAL_PHONEBOOK_SLOT_2 = 7,
} QmiPbmSessionType;

/**
 * qmi_pbm_session_type_get_string:
 *
 * Since: 1.6
 */

#endif /* _LIBQMI_GLIB_QMI_ENUMS_PBM_H_ */
