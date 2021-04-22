/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * qmicli -- Command line interface to control QMI devices
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright (C) 2012-2017 Aleksander Morgado <aleksander@aleksander.es>
 */

#include <glib.h>
#include <libqmi-glib.h>

#ifndef __QMICLI_H__
#define __QMICLI_H__

/* Common */
void          qmicli_async_operation_done (gboolean reported_operation_status,
                                           gboolean skip_cid_release);
void          qmicli_expect_indications   (void);

/* qmi_wwan specific */
GOptionGroup *qmicli_qmiwwan_get_option_group (void);
gboolean      qmicli_qmiwwan_options_enabled  (void);
void          qmicli_qmiwwan_run              (QmiDevice    *device,
                                               GCancellable *cancellable);

/* link management */
GOptionGroup *qmicli_link_management_get_option_group (void);
gboolean      qmicli_link_management_options_enabled  (void);
void          qmicli_link_management_run              (QmiDevice    *device,
                                                       GCancellable *cancellable);

#if defined HAVE_QMI_SERVICE_DMS
GOptionGroup *qmicli_dms_get_option_group (void);
gboolean      qmicli_dms_options_enabled  (void);
void          qmicli_dms_run              (QmiDevice *device,
                                           QmiClientDms *client,
                                           GCancellable *cancellable);
#endif

#if defined HAVE_QMI_SERVICE_WDS
GOptionGroup *qmicli_wds_get_option_group (void);
gboolean      qmicli_wds_options_enabled  (void);
void          qmicli_wds_run              (QmiDevice *device,
                                           QmiClientWds *client,
                                           GCancellable *cancellable);
#endif

#if defined HAVE_QMI_SERVICE_NAS
GOptionGroup *qmicli_nas_get_option_group (void);
gboolean      qmicli_nas_options_enabled  (void);
void          qmicli_nas_run              (QmiDevice *device,
                                           QmiClientNas *client,
                                           GCancellable *cancellable);
#endif

#if defined HAVE_QMI_SERVICE_PBM
GOptionGroup *qmicli_pbm_get_option_group (void);
gboolean      qmicli_pbm_options_enabled  (void);
void          qmicli_pbm_run              (QmiDevice *device,
                                           QmiClientPbm *client,
                                           GCancellable *cancellable);
#endif

#if defined HAVE_QMI_SERVICE_PDC
GOptionGroup *qmicli_pdc_get_option_group (void);
gboolean      qmicli_pdc_options_enabled  (void);
void          qmicli_pdc_run              (QmiDevice *device,
                                           QmiClientPdc *client,
                                           GCancellable *cancellable);
#endif

#if defined HAVE_QMI_SERVICE_UIM
GOptionGroup *qmicli_uim_get_option_group (void);
gboolean      qmicli_uim_options_enabled  (void);
void          qmicli_uim_run              (QmiDevice *device,
                                           QmiClientUim *client,
                                           GCancellable *cancellable);
#endif

#if defined HAVE_QMI_SERVICE_WMS
GOptionGroup *qmicli_wms_get_option_group (void);
gboolean      qmicli_wms_options_enabled  (void);
void          qmicli_wms_run              (QmiDevice *device,
                                           QmiClientWms *client,
                                           GCancellable *cancellable);
#endif

#if defined HAVE_QMI_SERVICE_WDA
GOptionGroup *qmicli_wda_get_option_group (void);
gboolean      qmicli_wda_options_enabled  (void);
void          qmicli_wda_run              (QmiDevice *device,
                                           QmiClientWda *client,
                                           GCancellable *cancellable);
#endif

#if defined HAVE_QMI_SERVICE_VOICE
GOptionGroup *qmicli_voice_get_option_group (void);
gboolean      qmicli_voice_options_enabled  (void);
void          qmicli_voice_run              (QmiDevice *device,
                                             QmiClientVoice *client,
                                             GCancellable *cancellable);
#endif

#if defined HAVE_QMI_SERVICE_LOC
GOptionGroup *qmicli_loc_get_option_group (void);
gboolean      qmicli_loc_options_enabled  (void);
void          qmicli_loc_run              (QmiDevice *device,
                                           QmiClientLoc *client,
                                           GCancellable *cancellable);
#endif

#if defined HAVE_QMI_SERVICE_QOS
GOptionGroup *qmicli_qos_get_option_group (void);
gboolean      qmicli_qos_options_enabled  (void);
void          qmicli_qos_run              (QmiDevice *device,
                                           QmiClientQos *client,
                                           GCancellable *cancellable);
#endif

#if defined HAVE_QMI_SERVICE_GAS
GOptionGroup *qmicli_gas_get_option_group (void);
gboolean      qmicli_gas_options_enabled  (void);
void          qmicli_gas_run              (QmiDevice *device,
                                           QmiClientGas *client,
                                           GCancellable *cancellable);
#endif

#if defined HAVE_QMI_SERVICE_GMS
GOptionGroup *qmicli_gms_get_option_group (void);
gboolean      qmicli_gms_options_enabled  (void);
void          qmicli_gms_run              (QmiDevice *device,
                                           QmiClientGms *client,
                                           GCancellable *cancellable);
#endif

#if defined HAVE_QMI_SERVICE_DSD
GOptionGroup *qmicli_dsd_get_option_group (void);
gboolean      qmicli_dsd_options_enabled  (void);
void          qmicli_dsd_run              (QmiDevice *device,
                                           QmiClientDsd *client,
                                           GCancellable *cancellable);
#endif

#if defined HAVE_QMI_SERVICE_SAR
GOptionGroup *qmicli_sar_get_option_group (void);
gboolean      qmicli_sar_options_enabled  (void);
void          qmicli_sar_run              (QmiDevice *device,
                                           QmiClientSar *client,
                                           GCancellable *cancellable);
#endif

#if defined HAVE_QMI_SERVICE_DPM
GOptionGroup *qmicli_dpm_get_option_group (void);
gboolean      qmicli_dpm_options_enabled  (void);
void          qmicli_dpm_run              (QmiDevice *device,
                                           QmiClientDpm *client,
                                           GCancellable *cancellable);
#endif

#endif /* __QMICLI_H__ */
