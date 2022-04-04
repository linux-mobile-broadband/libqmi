/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * libmbim-glib -- GLib/GIO based library to control MBIM devices
 *
 * Copyright (C) 2013 - 2014 Aleksander Morgado <aleksander@aleksander.es>
 */

#ifndef _LIBMBIM_GLIB_H_
#define _LIBMBIM_GLIB_H_

#define __LIBMBIM_GLIB_H_INSIDE__

/* libmbim-glib headers */

#include "mbim-version.h"
#include "mbim-utils.h"
#include "mbim-uuid.h"
#include "mbim-cid.h"
#include "mbim-message.h"
#include "mbim-device.h"
#include "mbim-enums.h"
#include "mbim-proxy.h"
#include "mbim-tlv.h"

/* generated */
#include "mbim-enum-types.h"
#include "mbim-error-types.h"
#include "mbim-basic-connect.h"
#include "mbim-sms.h"
#include "mbim-ussd.h"
#include "mbim-auth.h"
#include "mbim-phonebook.h"
#include "mbim-stk.h"
#include "mbim-dss.h"
#include "mbim-ms-firmware-id.h"
#include "mbim-ms-host-shutdown.h"
#include "mbim-ms-sar.h"
#include "mbim-qmi.h"
#include "mbim-atds.h"
#include "mbim-qdu.h"
#include "mbim-intel-firmware-update.h"
#include "mbim-ms-basic-connect-extensions.h"
#include "mbim-ms-uicc-low-level-access.h"
#include "mbim-quectel.h"
#include "mbim-intel-thermal-rf.h"
#include "mbim-ms-voice-extensions.h"

/* backwards compatibility */
#include "mbim-compat.h"

#endif /* _LIBMBIM_GLIB_H_ */
