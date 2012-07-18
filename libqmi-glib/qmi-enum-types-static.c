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
 * Copyright (C) 2012 Google, Inc.
 */

#include "qmi-enum-types-static.h"

typedef struct {
  guint64 value;
  const gchar *value_name;
  const gchar *value_nick;
} GFlags64Value;

/*****************************************************************************/
/* Band capability (64-bit flags)
 * Not handled in GType, just provide the helper build_string_from_mask() */

static const GFlags64Value qmi_dms_band_capability_values[] = {
    { QMI_DMS_BAND_CAPABILITY_BAND_CLASS_0_A_SYSTEM, "QMI_DMS_BAND_CAPABILITY_BAND_CLASS_0_A_SYSTEM", "band-class-0-a-system" },
    { QMI_DMS_BAND_CAPABILITY_BAND_CLASS_0_B_SYSTEM, "QMI_DMS_BAND_CAPABILITY_BAND_CLASS_0_B_SYSTEM", "band-class-0-b-system" },
    { QMI_DMS_BAND_CAPABILITY_BAND_CLASS_1_ALL_BLOCKS, "QMI_DMS_BAND_CAPABILITY_BAND_CLASS_1_ALL_BLOCKS", "band-class-1-all-blocks" },
    { QMI_DMS_BAND_CAPABILITY_BAND_CLASS_2, "QMI_DMS_BAND_CAPABILITY_BAND_CLASS_2", "band-class-2" },
    { QMI_DMS_BAND_CAPABILITY_BAND_CLASS_3_A_SYSTEM, "QMI_DMS_BAND_CAPABILITY_BAND_CLASS_3_A_SYSTEM", "band-class-3-a-system" },
    { QMI_DMS_BAND_CAPABILITY_BAND_CLASS_4_ALL_BLOCKS, "QMI_DMS_BAND_CAPABILITY_BAND_CLASS_4_ALL_BLOCKS", "band-class-4-all-blocks" },
    { QMI_DMS_BAND_CAPABILITY_BAND_CLASS_5_ALL_BLOCKS, "QMI_DMS_BAND_CAPABILITY_BAND_CLASS_5_ALL_BLOCKS", "band-class-5-all-blocks" },
    { QMI_DMS_BAND_CAPABILITY_GSM_DCS, "QMI_DMS_BAND_CAPABILITY_GSM_DCS", "gsm-dcs" },
    { QMI_DMS_BAND_CAPABILITY_GSM_EGSM, "QMI_DMS_BAND_CAPABILITY_GSM_EGSM", "gsm-egsm" },
    { QMI_DMS_BAND_CAPABILITY_GSM_PGSM, "QMI_DMS_BAND_CAPABILITY_GSM_PGSM", "gsm-pgsm" },
    { QMI_DMS_BAND_CAPABILITY_BAND_CLASS_6, "QMI_DMS_BAND_CAPABILITY_BAND_CLASS_6", "band-class-6" },
    { QMI_DMS_BAND_CAPABILITY_BAND_CLASS_7, "QMI_DMS_BAND_CAPABILITY_BAND_CLASS_7", "band-class-7" },
    { QMI_DMS_BAND_CAPABILITY_BAND_CLASS_8, "QMI_DMS_BAND_CAPABILITY_BAND_CLASS_8", "band-class-8" },
    { QMI_DMS_BAND_CAPABILITY_BAND_CLASS_9, "QMI_DMS_BAND_CAPABILITY_BAND_CLASS_9", "band-class-9" },
    { QMI_DMS_BAND_CAPABILITY_BAND_CLASS_10, "QMI_DMS_BAND_CAPABILITY_BAND_CLASS_10", "band-class-10" },
    { QMI_DMS_BAND_CAPABILITY_BAND_CLASS_11, "QMI_DMS_BAND_CAPABILITY_BAND_CLASS_11", "band-class-11" },
    { QMI_DMS_BAND_CAPABILITY_GSM_450, "QMI_DMS_BAND_CAPABILITY_GSM_450", "gsm-450" },
    { QMI_DMS_BAND_CAPABILITY_GSM_480, "QMI_DMS_BAND_CAPABILITY_GSM_480", "gsm-480" },
    { QMI_DMS_BAND_CAPABILITY_GSM_750, "QMI_DMS_BAND_CAPABILITY_GSM_750", "gsm-750" },
    { QMI_DMS_BAND_CAPABILITY_GSM_850, "QMI_DMS_BAND_CAPABILITY_GSM_850", "gsm-850" },
    { QMI_DMS_BAND_CAPABILITY_GSM_RAILWAYS, "QMI_DMS_BAND_CAPABILITY_GSM_RAILWAYS", "gsm-railways" },
    { QMI_DMS_BAND_CAPABILITY_GSM_PCS, "QMI_DMS_BAND_CAPABILITY_GSM_PCS", "gsm-pcs" },
    { QMI_DMS_BAND_CAPABILITY_WCDMA_2100, "QMI_DMS_BAND_CAPABILITY_WCDMA_2100", "wcdma-2100" },
    { QMI_DMS_BAND_CAPABILITY_WCDMA_PCS_1900, "QMI_DMS_BAND_CAPABILITY_WCDMA_PCS_1900", "wcdma-pcs-1900" },
    { QMI_DMS_BAND_CAPABILITY_WCDMA_DCS_1800, "QMI_DMS_BAND_CAPABILITY_WCDMA_DCS_1800", "wcdma-dcs-1800" },
    { QMI_DMS_BAND_CAPABILITY_WCDMA_1700_US, "QMI_DMS_BAND_CAPABILITY_WCDMA_1700_US", "wcdma-1700-us" },
    { QMI_DMS_BAND_CAPABILITY_WCDMA_850_US, "QMI_DMS_BAND_CAPABILITY_WCDMA_850_US", "wcdma-850-us" },
    { QMI_DMS_BAND_CAPABILITY_QWCDMA_800, "QMI_DMS_BAND_CAPABILITY_QWCDMA_800", "qwcdma-800" },
    { QMI_DMS_BAND_CAPABILITY_BAND_CLASS_12, "QMI_DMS_BAND_CAPABILITY_BAND_CLASS_12", "band-class-12" },
    { QMI_DMS_BAND_CAPABILITY_BAND_CLASS_14, "QMI_DMS_BAND_CAPABILITY_BAND_CLASS_14", "band-class-14" },
    { QMI_DMS_BAND_CAPABILITY_BAND_CLASS_15, "QMI_DMS_BAND_CAPABILITY_BAND_CLASS_15", "band-class-15" },
    { QMI_DMS_BAND_CAPABILITY_WCDMA_2600, "QMI_DMS_BAND_CAPABILITY_WCDMA_2600", "wcdma-2600" },
    { QMI_DMS_BAND_CAPABILITY_WCDMA_900, "QMI_DMS_BAND_CAPABILITY_WCDMA_900", "wcdma-900" },
    { QMI_DMS_BAND_CAPABILITY_WCDMA_1700_JAPAN, "QMI_DMS_BAND_CAPABILITY_WCDMA_1700_JAPAN", "wcdma-1700-japan" },
    { QMI_DMS_BAND_CAPABILITY_BAND_CLASS_16, "QMI_DMS_BAND_CAPABILITY_BAND_CLASS_16", "band-class-16" },
    { QMI_DMS_BAND_CAPABILITY_BAND_CLASS_17, "QMI_DMS_BAND_CAPABILITY_BAND_CLASS_17", "band-class-17" },
    { QMI_DMS_BAND_CAPABILITY_BAND_CLASS_18, "QMI_DMS_BAND_CAPABILITY_BAND_CLASS_18", "band-class-18" },
    { QMI_DMS_BAND_CAPABILITY_BAND_CLASS_19, "QMI_DMS_BAND_CAPABILITY_BAND_CLASS_19", "band-class-19" },
    { QMI_DMS_BAND_CAPABILITY_WCDMA_850_JAPAN, "QMI_DMS_BAND_CAPABILITY_WCDMA_850_JAPAN", "wcdma-850-japan" },
    { QMI_DMS_BAND_CAPABILITY_WCDMA_1500, "QMI_DMS_BANDQMI_DMS_BAND_CAPABILITY_WCDMA_170000" },
    { 0, NULL, NULL }
};

/* Flags-specific method to build a string with the given mask.
 * We get a comma separated list of the nicks of the GFlagsValues.
 * Note that this will be valid even if the GFlagsClass is not referenced
 * anywhere. */
gchar *
qmi_dms_band_capability_build_string_from_mask (QmiDmsBandCapability mask)
{
    guint i;
    gboolean first = TRUE;
    GString *str = NULL;

    for (i = 0; qmi_dms_band_capability_values[i].value_nick; i++) {
        /* We also look for exact matches */
        if (mask == qmi_dms_band_capability_values[i].value) {
            if (str)
                g_string_free (str, TRUE);
            return g_strdup (qmi_dms_band_capability_values[i].value_nick);
        }

        /* Build list with single-bit masks */
        if (mask & qmi_dms_band_capability_values[i].value) {
            guint c;
            guint64 number = qmi_dms_band_capability_values[i].value;

            for (c = 0; number; c++)
                number &= number - 1;

            if (c == 1) {
                if (!str)
                    str = g_string_new ("");
                g_string_append_printf (str, "%s%s",
                                        first ? "" : ", ",
                                        qmi_dms_band_capability_values[i].value_nick);
                if (first)
                    first = FALSE;
            }
        }
    }

    return (str ? g_string_free (str, FALSE) : NULL);
}


/*****************************************************************************/
/* Band capability (64-bit flags)
 * Not handled in GType, just provide the helper build_string_from_mask() */

static const GFlags64Value qmi_dms_lte_band_capability_values[] = {
    { QMI_DMS_LTE_BAND_CAPABILITY_BAND_1, "QMI_DMS_LTE_BAND_CAPABILITY_BAND_1", "1" },
    { QMI_DMS_LTE_BAND_CAPABILITY_BAND_2, "QMI_DMS_LTE_BAND_CAPABILITY_BAND_2", "2" },
    { QMI_DMS_LTE_BAND_CAPABILITY_BAND_3, "QMI_DMS_LTE_BAND_CAPABILITY_BAND_3", "3" },
    { QMI_DMS_LTE_BAND_CAPABILITY_BAND_4, "QMI_DMS_LTE_BAND_CAPABILITY_BAND_4", "4" },
    { QMI_DMS_LTE_BAND_CAPABILITY_BAND_5, "QMI_DMS_LTE_BAND_CAPABILITY_BAND_5", "5" },
    { QMI_DMS_LTE_BAND_CAPABILITY_BAND_6, "QMI_DMS_LTE_BAND_CAPABILITY_BAND_6", "6" },
    { QMI_DMS_LTE_BAND_CAPABILITY_BAND_7, "QMI_DMS_LTE_BAND_CAPABILITY_BAND_7", "7" },
    { QMI_DMS_LTE_BAND_CAPABILITY_BAND_8, "QMI_DMS_LTE_BAND_CAPABILITY_BAND_8", "8" },
    { QMI_DMS_LTE_BAND_CAPABILITY_BAND_9, "QMI_DMS_LTE_BAND_CAPABILITY_BAND_9", "9" },
    { QMI_DMS_LTE_BAND_CAPABILITY_BAND_10, "QMI_DMS_LTE_BAND_CAPABILITY_BAND_10", "10" },
    { QMI_DMS_LTE_BAND_CAPABILITY_BAND_11, "QMI_DMS_LTE_BAND_CAPABILITY_BAND_11", "11" },
    { QMI_DMS_LTE_BAND_CAPABILITY_BAND_12, "QMI_DMS_LTE_BAND_CAPABILITY_BAND_12", "12" },
    { QMI_DMS_LTE_BAND_CAPABILITY_BAND_13, "QMI_DMS_LTE_BAND_CAPABILITY_BAND_13", "13" },
    { QMI_DMS_LTE_BAND_CAPABILITY_BAND_14, "QMI_DMS_LTE_BAND_CAPABILITY_BAND_14", "14" },
    { QMI_DMS_LTE_BAND_CAPABILITY_BAND_17, "QMI_DMS_LTE_BAND_CAPABILITY_BAND_17", "17" },
    { QMI_DMS_LTE_BAND_CAPABILITY_BAND_18, "QMI_DMS_LTE_BAND_CAPABILITY_BAND_18", "18" },
    { QMI_DMS_LTE_BAND_CAPABILITY_BAND_19, "QMI_DMS_LTE_BAND_CAPABILITY_BAND_19", "19" },
    { QMI_DMS_LTE_BAND_CAPABILITY_BAND_20, "QMI_DMS_LTE_BAND_CAPABILITY_BAND_20", "20" },
    { QMI_DMS_LTE_BAND_CAPABILITY_BAND_21, "QMI_DMS_LTE_BAND_CAPABILITY_BAND_21", "21" },
    { QMI_DMS_LTE_BAND_CAPABILITY_BAND_24, "QMI_DMS_LTE_BAND_CAPABILITY_BAND_24", "24" },
    { QMI_DMS_LTE_BAND_CAPABILITY_BAND_25, "QMI_DMS_LTE_BAND_CAPABILITY_BAND_25", "25" },
    { QMI_DMS_LTE_BAND_CAPABILITY_BAND_33, "QMI_DMS_LTE_BAND_CAPABILITY_BAND_33", "33" },
    { QMI_DMS_LTE_BAND_CAPABILITY_BAND_34, "QMI_DMS_LTE_BAND_CAPABILITY_BAND_34", "34" },
    { QMI_DMS_LTE_BAND_CAPABILITY_BAND_35, "QMI_DMS_LTE_BAND_CAPABILITY_BAND_35", "35" },
    { QMI_DMS_LTE_BAND_CAPABILITY_BAND_36, "QMI_DMS_LTE_BAND_CAPABILITY_BAND_36", "36" },
    { QMI_DMS_LTE_BAND_CAPABILITY_BAND_37, "QMI_DMS_LTE_BAND_CAPABILITY_BAND_37", "37" },
    { QMI_DMS_LTE_BAND_CAPABILITY_BAND_38, "QMI_DMS_LTE_BAND_CAPABILITY_BAND_38", "38" },
    { QMI_DMS_LTE_BAND_CAPABILITY_BAND_39, "QMI_DMS_LTE_BAND_CAPABILITY_BAND_39", "39" },
    { QMI_DMS_LTE_BAND_CAPABILITY_BAND_40, "QMI_DMS_LTE_BAND_CAPABILITY_BAND_40", "40" },
    { QMI_DMS_LTE_BAND_CAPABILITY_BAND_41, "QMI_DMS_LTE_BAND_CAPABILITY_BAND_41", "41" },
    { QMI_DMS_LTE_BAND_CAPABILITY_BAND_42, "QMI_DMS_LTE_BAND_CAPABILITY_BAND_42", "42" },
    { QMI_DMS_LTE_BAND_CAPABILITY_BAND_43, "QMI_DMS_LTE_BAND_CAPABILITY_BAND_43", "43" },
    { 0, NULL, NULL }
};

gchar *
qmi_dms_lte_band_capability_build_string_from_mask (QmiDmsLteBandCapability mask)
{
    guint i;
    gboolean first = TRUE;
    GString *str = NULL;

    for (i = 0; qmi_dms_lte_band_capability_values[i].value_nick; i++) {
        /* We also look for exact matches */
        if (mask == qmi_dms_lte_band_capability_values[i].value) {
            if (str)
                g_string_free (str, TRUE);
            return g_strdup (qmi_dms_lte_band_capability_values[i].value_nick);
        }

        /* Build list with single-bit masks */
        if (mask & qmi_dms_lte_band_capability_values[i].value) {
            guint c;
            guint64 number = qmi_dms_lte_band_capability_values[i].value;

            for (c = 0; number; c++)
                number &= number - 1;

            if (c == 1) {
                if (!str)
                    str = g_string_new ("");
                g_string_append_printf (str, "%s%s",
                                        first ? "" : ", ",
                                        qmi_dms_lte_band_capability_values[i].value_nick);
                if (first)
                    first = FALSE;
            }
        }
    }

    return (str ? g_string_free (str, FALSE) : NULL);
}
