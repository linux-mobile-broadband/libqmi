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
 */

#ifndef _LIBQMI_GLIB_QMI_ENUMS_UIM_H_
#define _LIBQMI_GLIB_QMI_ENUMS_UIM_H_

#if !defined (__LIBQMI_GLIB_H_INSIDE__) && !defined (LIBQMI_GLIB_COMPILATION)
#error "Only <libqmi-glib.h> can be included directly."
#endif

/**
 * SECTION: qmi-enums-uim
 * @title: UIM enumerations and flags
 *
 * This section defines enumerations and flags used in the UIM service
 * interface.
 */

/*****************************************************************************/
/* Helper enums for the 'QMI UIM Read Record' request/response */

/**
 * QmiUimSessionType:
 * @QMI_UIM_SESSION_TYPE_PRIMARY_GW_PROVISIONING: Primary GSM/WCDMA provisioning.
 * @QMI_UIM_SESSION_TYPE_PRIMARY_1X_PROVISIONING: Primary CDMA1x provisioning.
 * @QMI_UIM_SESSION_TYPE_SECONDARY_GW_PROVISIONING: Secondary GSM/WCDMA provisioning.
 * @QMI_UIM_SESSION_TYPE_SECONDARY_1X_PROVISIONING: Secondary CDMA1x provisioning.
 * @QMI_UIM_SESSION_TYPE_NONPROVISIONING_SLOT_1: Nonprovisioning on slot 1.
 * @QMI_UIM_SESSION_TYPE_NONPROVISIONING_SLOT_2: Nonprovisioning on slot 2.
 * @QMI_UIM_SESSION_TYPE_CARD_SLOT_1: Card on slot 1.
 * @QMI_UIM_SESSION_TYPE_CARD_SLOT_2: Card on slot 2.
 * @QMI_UIM_SESSION_TYPE_LOGICAL_CHANNEL_SLOT_1: Logical channel on slot 1.
 * @QMI_UIM_SESSION_TYPE_LOGICAL_CHANNEL_SLOT_2: Logical channel on slot 2.
 *
 * Type of UIM session.
 */
typedef enum {
    QMI_UIM_SESSION_TYPE_PRIMARY_GW_PROVISIONING   = 0,
    QMI_UIM_SESSION_TYPE_PRIMARY_1X_PROVISIONING   = 1,
    QMI_UIM_SESSION_TYPE_SECONDARY_GW_PROVISIONING = 2,
    QMI_UIM_SESSION_TYPE_SECONDARY_1X_PROVISIONING = 3,
    QMI_UIM_SESSION_TYPE_NONPROVISIONING_SLOT_1    = 4,
    QMI_UIM_SESSION_TYPE_NONPROVISIONING_SLOT_2    = 5,
    QMI_UIM_SESSION_TYPE_CARD_SLOT_1               = 6,
    QMI_UIM_SESSION_TYPE_CARD_SLOT_2               = 7,
    QMI_UIM_SESSION_TYPE_LOGICAL_CHANNEL_SLOT_1    = 8,
    QMI_UIM_SESSION_TYPE_LOGICAL_CHANNEL_SLOT_2    = 9
} QmiUimSessionType;

/*****************************************************************************/
/* Helper enums for the 'QMI UIM Get File Attributes' request/response */

/**
 * QmiUimFileType:
 * @QMI_UIM_FILE_TYPE_TRANSPARENT: Transparent.
 * @QMI_UIM_FILE_TYPE_CYCLIC: Cyclic.
 * @QMI_UIM_FILE_TYPE_LINEAR_FIXED: Linear fixed.
 * @QMI_UIM_FILE_TYPE_DEDICATED_FILE: Dedicated file.
 * @QMI_UIM_FILE_TYPE_MASTER_FILE: Master file.
 *
 * Type of UIM file.
 */
typedef enum {
    QMI_UIM_FILE_TYPE_TRANSPARENT    = 0,
    QMI_UIM_FILE_TYPE_CYCLIC         = 1,
    QMI_UIM_FILE_TYPE_LINEAR_FIXED   = 2,
    QMI_UIM_FILE_TYPE_DEDICATED_FILE = 3,
    QMI_UIM_FILE_TYPE_MASTER_FILE    = 4
} QmiUimFileType;

/**
 * QmiUimSecurityAttributeLogic:
 * @QMI_UIM_SECURITY_ATTRIBUTE_LOGIC_ALWAYS: Always.
 * @QMI_UIM_SECURITY_ATTRIBUTE_LOGIC_NEVER: Never.
 * @QMI_UIM_SECURITY_ATTRIBUTE_LOGIC_AND: And.
 * @QMI_UIM_SECURITY_ATTRIBUTE_LOGIC_OR: Or.
 * @QMI_UIM_SECURITY_ATTRIBUTE_LOGIC_SINGLE: Single.
 *
 * Logic applicable to security attributes.
 */
typedef enum {
    QMI_UIM_SECURITY_ATTRIBUTE_LOGIC_ALWAYS = 0,
    QMI_UIM_SECURITY_ATTRIBUTE_LOGIC_NEVER  = 1,
    QMI_UIM_SECURITY_ATTRIBUTE_LOGIC_AND    = 2,
    QMI_UIM_SECURITY_ATTRIBUTE_LOGIC_OR     = 3,
    QMI_UIM_SECURITY_ATTRIBUTE_LOGIC_SINGLE = 4
} QmiUimSecurityAttributeLogic;

/**
 * QmiUimSecurityAttribute:
 * @QMI_UIM_SECURITY_ATTRIBUTE_PIN1: PIN1.
 * @QMI_UIM_SECURITY_ATTRIBUTE_PIN2: PIN2.
 * @QMI_UIM_SECURITY_ATTRIBUTE_UPIN: UPIN.
 * @QMI_UIM_SECURITY_ATTRIBUTE_ADM: ADM.
 *
 * Security Attributes.
 */
typedef enum {
    QMI_UIM_SECURITY_ATTRIBUTE_PIN1 = 1 << 0,
    QMI_UIM_SECURITY_ATTRIBUTE_PIN2 = 1 << 1,
    QMI_UIM_SECURITY_ATTRIBUTE_UPIN = 1 << 2,
    QMI_UIM_SECURITY_ATTRIBUTE_ADM  = 1 << 3
} QmiUimSecurityAttribute;

#endif /* _LIBQMI_GLIB_QMI_ENUMS_UIM_H_ */
