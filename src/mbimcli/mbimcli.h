/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * mbimcli -- Command line interface to control MBIM devices
 *
 * Copyright (C) 2013 - 2014 Aleksander Morgado <aleksander@aleksander.es>
 */

#include <glib.h>

#ifndef __MBIMCLI_H__
#define __MBIMCLI_H__

#define VALIDATE_UNKNOWN(str) ((str) ? (str) : "unknown")

/* Common */
void mbimcli_async_operation_done (gboolean operation_status);

/* Basic Connect group */
GOptionGroup *mbimcli_basic_connect_get_option_group    (void);
GOptionGroup *mbimcli_phonebook_get_option_group        (void);
GOptionGroup *mbimcli_dss_get_option_group              (void);
GOptionGroup *mbimcli_ms_firmware_id_get_option_group   (void);
GOptionGroup *mbimcli_ms_host_shutdown_get_option_group (void);
GOptionGroup *mbimcli_ms_sar_get_option_group           (void);
GOptionGroup *mbimcli_atds_get_option_group             (void);
GOptionGroup *mbimcli_intel_firmware_update_get_option_group (void);
GOptionGroup *mbimcli_ms_basic_connect_extensions_get_option_group (void);
GOptionGroup *mbimcli_quectel_get_option_group          (void);
GOptionGroup *mbimcli_intel_thermal_rf_get_option_group (void);

gboolean      mbimcli_basic_connect_options_enabled     (void);
gboolean      mbimcli_phonebook_options_enabled         (void);
gboolean      mbimcli_dss_options_enabled               (void);
gboolean      mbimcli_ms_firmware_id_options_enabled    (void);
gboolean      mbimcli_ms_host_shutdown_options_enabled  (void);
gboolean      mbimcli_ms_sar_options_enabled            (void);
gboolean      mbimcli_atds_options_enabled              (void);
gboolean      mbimcli_intel_firmware_update_options_enabled (void);
gboolean      mbimcli_ms_basic_connect_extensions_options_enabled (void);
gboolean      mbimcli_quectel_options_enabled           (void);
gboolean      mbimcli_intel_thermal_rf_options_enabled  (void);

void          mbimcli_basic_connect_run                 (MbimDevice *device,
                                                         GCancellable *cancellable);
void          mbimcli_phonebook_run                     (MbimDevice *device,
                                                         GCancellable *cancellable);
void          mbimcli_dss_run                           (MbimDevice *device,
                                                         GCancellable *cancellable);
void          mbimcli_ms_firmware_id_run                (MbimDevice *device,
                                                         GCancellable *cancellable);
void          mbimcli_ms_host_shutdown_run              (MbimDevice *device,
                                                         GCancellable *cancellable);
void          mbimcli_ms_sar_run                        (MbimDevice *device,
                                                         GCancellable *cancellable);
void          mbimcli_atds_run                          (MbimDevice *device,
                                                         GCancellable *cancellable);
void          mbimcli_intel_firmware_update_run         (MbimDevice *device,
                                                         GCancellable *cancellable);
void          mbimcli_ms_basic_connect_extensions_run   (MbimDevice *device,
                                                         GCancellable *cancellable);
void          mbimcli_quectel_run                       (MbimDevice *device,
                                                         GCancellable *cancellable);
void          mbimcli_intel_thermal_rf_run              (MbimDevice *device,
                                                         GCancellable *cancellable);


/* link management */
GOptionGroup *mbimcli_link_management_get_option_group (void);
gboolean      mbimcli_link_management_options_enabled  (void);
void          mbimcli_link_management_run              (MbimDevice   *device,
                                                        GCancellable *cancellable);

#endif /* __MBIMCLI_H__ */
