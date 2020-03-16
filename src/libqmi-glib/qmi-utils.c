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
 * Copyright (C) 2012-2020 Dan Williams <dcbw@redhat.com>
 * Copyright (C) 2012-2020 Aleksander Morgado <aleksander@aleksander.es>
 */

#include <config.h>

#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <pwd.h>
#include <errno.h>

#include "qmi-utils.h"
#include "qmi-error-types.h"

/**
 * SECTION:qmi-utils
 * @title: Common utilities
 *
 * This section exposes a set of common utilities that may be used to work
 * with the QMI library.
 **/

/*****************************************************************************/

gchar *
__qmi_utils_str_hex (gconstpointer mem,
                     gsize size,
                     gchar delimiter)
{
    const guint8 *data = mem;
    gsize i;
    gsize j;
    gsize new_str_length;
    gchar *new_str;

    /* Get new string length. If input string has N bytes, we need:
     * - 1 byte for last NUL char
     * - 2N bytes for hexadecimal char representation of each byte...
     * - N-1 bytes for the separator ':'
     * So... a total of (1+2N+N-1) = 3N bytes are needed... */
    new_str_length =  3 * size;

    /* Allocate memory for new array and initialize contents to NUL */
    new_str = g_malloc0 (new_str_length);

    /* Print hexadecimal representation of each byte... */
    for (i = 0, j = 0; i < size; i++, j += 3) {
        /* Print character in output string... */
        snprintf (&new_str[j], 3, "%02X", data[i]);
        /* And if needed, add separator */
        if (i != (size - 1) )
            new_str[j + 2] = delimiter;
    }

    /* Set output string */
    return new_str;
}

/*****************************************************************************/

gboolean
__qmi_user_allowed (uid_t uid,
                    GError **error)
{
#ifndef QMI_USERNAME_ENABLED
    if (uid == 0)
        return TRUE;
#else
# ifndef QMI_USERNAME
#  error QMI username not defined
# endif

    struct passwd *expected_usr = NULL;

    /* Root user is always allowed, regardless of the specified QMI_USERNAME */
    if (uid == 0)
        return TRUE;

    expected_usr = getpwnam (QMI_USERNAME);
    if (!expected_usr) {
        g_set_error (error,
                     QMI_CORE_ERROR,
                     QMI_CORE_ERROR_FAILED,
                     "Not enough privileges (unknown username %s)", QMI_USERNAME);
        return FALSE;
    }

    if (uid == expected_usr->pw_uid)
        return TRUE;
#endif

    g_set_error (error,
                 QMI_CORE_ERROR,
                 QMI_CORE_ERROR_FAILED,
                 "Not enough privileges");
    return FALSE;
}

/*****************************************************************************/

gboolean
__qmi_string_utf8_validate_printable (const guint8 *utf8,
                                      gsize         utf8_len)
{
    const gchar *p;
    const gchar *init;

    g_assert (utf8);
    g_assert (utf8_len);

    /* First check if valid UTF-8 */
    init = (const gchar *)utf8;
    if (!g_utf8_validate (init, utf8_len, NULL))
        return FALSE;

    /* Then check if contents are printable. If one is not,
     * check fails. */
    for (p = init; (gsize)(p - init) < utf8_len; p = g_utf8_next_char (p)) {
        gunichar unichar;

        unichar = g_utf8_get_char (p);
        if (!g_unichar_isprint (unichar))
            return FALSE;
    }
    return TRUE;
}

/*****************************************************************************/
/* GSM 03.38 encoding conversion stuff, imported from ModemManager */

#define GSM_DEF_ALPHABET_SIZE 128
#define GSM_EXT_ALPHABET_SIZE 10

typedef struct GsmUtf8Mapping {
    gchar  chars[3];
    guint8 len;
    guint8 gsm;  /* only used for extended GSM charset */
} GsmUtf8Mapping;

#define ONE(a)     { {a, 0x00, 0x00}, 1, 0 }
#define TWO(a, b)  { {a, b,    0x00}, 2, 0 }

/*
 * Mapping from GSM default alphabet to UTF-8.
 *
 * ETSI GSM 03.38, version 6.0.1, section 6.2.1; Default alphabet. Mapping to UCS-2.
 * Mapping according to http://unicode.org/Public/MAPPINGS/ETSI/GSM0338.TXT
 */
static const GsmUtf8Mapping gsm_def_utf8_alphabet[GSM_DEF_ALPHABET_SIZE] = {
    /* @             £                $                ¥   */
    ONE(0x40),       TWO(0xc2, 0xa3), ONE(0x24),       TWO(0xc2, 0xa5),
    /* è             é                ù                ì   */
    TWO(0xc3, 0xa8), TWO(0xc3, 0xa9), TWO(0xc3, 0xb9), TWO(0xc3, 0xac),
    /* ò             Ç                \n               Ø   */
    TWO(0xc3, 0xb2), TWO(0xc3, 0x87), ONE(0x0a),       TWO(0xc3, 0x98),
    /* ø             \r               Å                å   */
    TWO(0xc3, 0xb8), ONE(0x0d),       TWO(0xc3, 0x85), TWO(0xc3, 0xa5),
    /* Δ             _                Φ                Γ   */
    TWO(0xce, 0x94), ONE(0x5f),       TWO(0xce, 0xa6), TWO(0xce, 0x93),
    /* Λ             Ω                Π                Ψ   */
    TWO(0xce, 0x9b), TWO(0xce, 0xa9), TWO(0xce, 0xa0), TWO(0xce, 0xa8),
    /* Σ             Θ                Ξ                Escape Code */
    TWO(0xce, 0xa3), TWO(0xce, 0x98), TWO(0xce, 0x9e), ONE(0xa0),
    /* Æ             æ                ß                É   */
    TWO(0xc3, 0x86), TWO(0xc3, 0xa6), TWO(0xc3, 0x9f), TWO(0xc3, 0x89),
    /* ' '           !                "                #   */
    ONE(0x20),       ONE(0x21),       ONE(0x22),       ONE(0x23),
    /* ¤             %                &                '   */
    TWO(0xc2, 0xa4), ONE(0x25),       ONE(0x26),       ONE(0x27),
    /* (             )                *                +   */
    ONE(0x28),       ONE(0x29),       ONE(0x2a),       ONE(0x2b),
    /* ,             -                .                /   */
    ONE(0x2c),       ONE(0x2d),       ONE(0x2e),       ONE(0x2f),
    /* 0             1                2                3   */
    ONE(0x30),       ONE(0x31),       ONE(0x32),       ONE(0x33),
    /* 4             5                6                7   */
    ONE(0x34),       ONE(0x35),       ONE(0x36),       ONE(0x37),
    /* 8             9                :                ;   */
    ONE(0x38),       ONE(0x39),       ONE(0x3a),       ONE(0x3b),
    /* <             =                >                ?   */
    ONE(0x3c),       ONE(0x3d),       ONE(0x3e),       ONE(0x3f),
    /* ¡             A                B                C   */
    TWO(0xc2, 0xa1), ONE(0x41),       ONE(0x42),       ONE(0x43),
    /* D             E                F                G   */
    ONE(0x44),       ONE(0x45),       ONE(0x46),       ONE(0x47),
    /* H             I                J                K   */
    ONE(0x48),       ONE(0x49),       ONE(0x4a),       ONE(0x4b),
    /* L             M                N                O   */
    ONE(0x4c),       ONE(0x4d),       ONE(0x4e),       ONE(0x4f),
    /* P             Q                R                S   */
    ONE(0x50),       ONE(0x51),       ONE(0x52),       ONE(0x53),
    /* T             U                V                W   */
    ONE(0x54),       ONE(0x55),       ONE(0x56),       ONE(0x57),
    /* X             Y                Z                Ä   */
    ONE(0x58),       ONE(0x59),       ONE(0x5a),       TWO(0xc3, 0x84),
    /* Ö             Ñ                Ü                §   */
    TWO(0xc3, 0x96), TWO(0xc3, 0x91), TWO(0xc3, 0x9c), TWO(0xc2, 0xa7),
    /* ¿             a                b                c   */
    TWO(0xc2, 0xbf), ONE(0x61),       ONE(0x62),       ONE(0x63),
    /* d             e                f                g   */
    ONE(0x64),       ONE(0x65),       ONE(0x66),       ONE(0x67),
    /* h             i                j                k   */
    ONE(0x68),       ONE(0x69),       ONE(0x6a),       ONE(0x6b),
    /* l             m                n                o   */
    ONE(0x6c),       ONE(0x6d),       ONE(0x6e),       ONE(0x6f),
    /* p             q                r                s   */
    ONE(0x70),       ONE(0x71),       ONE(0x72),       ONE(0x73),
    /* t             u                v                w   */
    ONE(0x74),       ONE(0x75),       ONE(0x76),       ONE(0x77),
    /* x             y                z                ä   */
    ONE(0x78),       ONE(0x79),       ONE(0x7a),       TWO(0xc3, 0xa4),
    /* ö             ñ                ü                à   */
    TWO(0xc3, 0xb6), TWO(0xc3, 0xb1), TWO(0xc3, 0xbc), TWO(0xc3, 0xa0)
};

static guint8
gsm_def_char_to_utf8 (const guint8  gsm,
                      guint8       *out_utf8)
{
    if (gsm >= GSM_DEF_ALPHABET_SIZE)
        return 0;
    memcpy (out_utf8, &gsm_def_utf8_alphabet[gsm].chars[0], gsm_def_utf8_alphabet[gsm].len);
    return gsm_def_utf8_alphabet[gsm].len;
}

#define EONE(a, g)        { {a, 0x00, 0x00}, 1, g }
#define ETHR(a, b, c, g)  { {a, b,    c},    3, g }

/* Mapping from GSM extended alphabet to UTF-8 */
static const GsmUtf8Mapping gsm_ext_utf8_alphabet[GSM_EXT_ALPHABET_SIZE] = {
    /* form feed      ^                 {                 }  */
    EONE(0x0c, 0x0a), EONE(0x5e, 0x14), EONE(0x7b, 0x28), EONE(0x7d, 0x29),
    /* \              [                 ~                 ]  */
    EONE(0x5c, 0x2f), EONE(0x5b, 0x3c), EONE(0x7e, 0x3d), EONE(0x5d, 0x3e),
    /* |              €                                      */
    EONE(0x7c, 0x40), ETHR(0xe2, 0x82, 0xac, 0x65)
};

#define GSM_ESCAPE_CHAR 0x1b

static guint8
gsm_ext_char_to_utf8 (const guint8  gsm,
                      guint8       *out_utf8)
{
    int i;

    for (i = 0; i < GSM_EXT_ALPHABET_SIZE; i++) {
        if (gsm == gsm_ext_utf8_alphabet[i].gsm) {
            memcpy (out_utf8, &gsm_ext_utf8_alphabet[i].chars[0], gsm_ext_utf8_alphabet[i].len);
            return gsm_ext_utf8_alphabet[i].len;
        }
    }
    return 0;
}

static guint8 *
charset_gsm_unpack (const guint8 *gsm,
                    guint32       num_septets,
                    guint8        start_offset,  /* in _bits_ */
                    gsize        *out_unpacked_len)
{
    GByteArray *unpacked;
    guint       i;

    unpacked = g_byte_array_sized_new (num_septets + 1);
    for (i = 0; i < num_septets; i++) {
        guint8 bits_here, bits_in_next, octet, offset, c;
        guint32 start_bit;

        start_bit = start_offset + (i * 7); /* Overall bit offset of char in buffer */
        offset = start_bit % 8;  /* Offset to start of char in this byte */
        bits_here = offset ? (8 - offset) : 7;
        bits_in_next = 7 - bits_here;

        /* Grab bits in the current byte */
        octet = gsm[start_bit / 8];
        c = (octet >> offset) & (0xFF >> (8 - bits_here));

        /* Grab any bits that spilled over to next byte */
        if (bits_in_next) {
            octet = gsm[(start_bit / 8) + 1];
            c |= (octet & (0xFF >> (8 - bits_in_next))) << bits_here;
        }
        g_byte_array_append (unpacked, &c, 1);
    }

    *out_unpacked_len = unpacked->len;
    return g_byte_array_free (unpacked, FALSE);
}

gchar *
__qmi_string_utf8_from_gsm7 (const guint8 *gsm_packed,
                             gsize         gsm_packed_len)
{
    GByteArray *utf8;
    guint8     *gsm_unpacked;
    gsize       gsm_unpacked_len;
    gsize       i;

    /* unpack operation needs input length in SEPTETS */
    gsm_unpacked = charset_gsm_unpack (gsm_packed, gsm_packed_len * 8 / 7, 0, &gsm_unpacked_len);

    /* worst case initial length */
    utf8 = g_byte_array_sized_new (gsm_unpacked_len * 2 + 1);

    for (i = 0; i < gsm_unpacked_len; i++) {
        guint8 uchars[4];
        guint8 ulen;

        /*
         * 	0x00 is NULL (when followed only by 0x00 up to the
         * 	end of (fixed byte length) message, possibly also up to
         * 	FORM FEED.  But 0x00 is also the code for COMMERCIAL AT
         * 	when some other character (CARRIAGE RETURN if nothing else)
         * 	comes after the 0x00.
         *  http://unicode.org/Public/MAPPINGS/ETSI/GSM0338.TXT
         *
         * So, if we find a '@' (0x00) and all the next chars after that
         * are also 0x00, we can consider the string finished already.
         */
        if (gsm_unpacked[i] == 0x00) {
            gsize j;

            for (j = i + 1; j < gsm_unpacked_len; j++) {
                if (gsm_unpacked[j] != 0x00)
                    break;
            }
            if (j == gsm_unpacked_len)
                break;
        }

        if (gsm_unpacked[i] == GSM_ESCAPE_CHAR) {
            /* Extended alphabet, decode next char */
            ulen = gsm_ext_char_to_utf8 (gsm_unpacked[i+1], uchars);
            if (ulen)
                i += 1;
        } else {
            /* Default alphabet */
            ulen = gsm_def_char_to_utf8 (gsm_unpacked[i], uchars);
        }

        /* Invalid GSM-7, abort */
        if (!ulen) {
            g_free (gsm_unpacked);
            g_byte_array_unref (utf8);
            return NULL;
        }

        g_byte_array_append (utf8, &uchars[0], ulen);
    }

    g_byte_array_append (utf8, (guint8 *) "\0", 1);  /* NUL terminator */
    g_free (gsm_unpacked);
    return (gchar *) g_byte_array_free (utf8, FALSE);
}

/*****************************************************************************/

gchar *
__qmi_string_utf8_from_ucs2le (const guint8 *ucs2le,
                               gsize         ucs2le_len)
{
    gsize               ucs2he_nchars;
    g_autofree guint16 *ucs2he = NULL;

    /* UCS2 data length given in bytes must be multiple of 2 */
    if (ucs2le_len % 2 != 0)
        return NULL;

    /* Convert length from bytes to number of ucs2 characters */
    ucs2he_nchars = ucs2le_len / 2;

    /* We'll attempt to convert UCS2 to UTF-8, using GLib's support to convert
     * from UTF-16 to UTF-8 (given that UCS2 is a subset of UTF-16). In order
     * to do so, we need the input string in "host endian", so if we're in a
     * BE system (where host endian is not little endian), we do an explicit
     * conversion.
     *
     * We don't want to use g_convert() because that would mean requiring
     * full iconv() support in the system.
     *
     * We're using an extra allocation always because we need to increase
     * alignment (guint8 -> (guint16)gunichar2)
     */
    ucs2he = g_new (guint16, ucs2he_nchars);
    g_assert (sizeof (guint16) * ucs2he_nchars == ucs2le_len);
    memcpy (ucs2he, ucs2le, ucs2le_len);
#if G_BYTE_ORDER == G_BIG_ENDIAN
    {
        guint i;

        for (i = 0; i < ucs2he_nchars; i++)
            ucs2he[i] = GUINT16_FROM_LE (ucs2he[i]);
    }
#endif
    return g_utf16_to_utf8 ((const gunichar2 *)ucs2he, ucs2he_nchars, NULL, NULL, NULL);
}
/*****************************************************************************/

static gchar *
utils_get_driver (const gchar  *device_basename,
                  GError      **error)
{
    static const gchar *subsystems[] = { "usbmisc", "usb" };
    guint               i;
    gchar              *driver = NULL;

    for (i = 0; !driver && i < G_N_ELEMENTS (subsystems); i++) {
        gchar *tmp;
        gchar *path;

        /* driver sysfs can be built directly using subsystem and name; e.g. for subsystem
         * usbmisc and name cdc-wdm0:
         *    $ realpath /sys/class/usbmisc/cdc-wdm0/device/driver
         *    /sys/bus/usb/drivers/qmi_wwan
         */
        tmp = g_strdup_printf ("/sys/class/%s/%s/device/driver", subsystems[i], device_basename);
        path = realpath (tmp, NULL);
        g_free (tmp);

        if (!path)
            continue;

        driver = g_path_get_basename (path);
        g_free (path);
    }

    if (!driver)
        g_set_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_FAILED,
                     "couldn't detect device driver");
    return driver;
}

__QmiTransportType
__qmi_utils_get_transport_type (const gchar  *path,
                                GError      **error)
{
    __QmiTransportType  transport = __QMI_TRANSPORT_TYPE_UNKNOWN;
    gchar              *device_basename = NULL;
    gchar              *driver = NULL;
    gchar              *sysfs_path = NULL;
    GError             *inner_error = NULL;

    device_basename = __qmi_utils_get_devname (path, &inner_error);
    if (!device_basename)
        goto out;

    driver = utils_get_driver (device_basename, &inner_error);

    if (!driver) {
        /* On Android systems we get access to the QMI control port through
         * virtual smdcntl devices in the smdpkt subsystem. */
        sysfs_path = g_strdup_printf ("/sys/class/smdpkt/%s", device_basename);
        if (g_file_test (sysfs_path, G_FILE_TEST_EXISTS)) {
            g_clear_error (&inner_error);
            transport = __QMI_TRANSPORT_TYPE_QMUX;
            goto out;
        }
        g_free (sysfs_path);
        /* On mainline kernels this control port is provided by rpmsg */
        sysfs_path = g_strdup_printf ("/sys/class/rpmsg/%s", device_basename);
        if (g_file_test (sysfs_path, G_FILE_TEST_EXISTS)) {
            g_clear_error (&inner_error);
            transport = __QMI_TRANSPORT_TYPE_QMUX;
        }
        goto out;
    }

    if (!g_strcmp0 (driver, "cdc_mbim")) {
        transport = __QMI_TRANSPORT_TYPE_MBIM;
        goto out;
    }

    if (!g_strcmp0 (driver, "qmi_wwan")) {
        transport = __QMI_TRANSPORT_TYPE_QMUX;
        goto out;
    }

    g_set_error (&inner_error, QMI_CORE_ERROR, QMI_CORE_ERROR_FAILED,
                 "unexpected driver detected: %s", driver);

 out:

    g_free (device_basename);
    g_free (driver);
    g_free (sysfs_path);

    if (inner_error) {
        g_assert (transport == __QMI_TRANSPORT_TYPE_UNKNOWN);
        g_propagate_error (error, inner_error);
    } else
        g_assert (transport != __QMI_TRANSPORT_TYPE_UNKNOWN);

    return transport;
}

gchar *
__qmi_utils_get_devpath (const gchar  *cdc_wdm_path,
                         GError      **error)
{
    gchar *aux;

    if (!g_file_test (cdc_wdm_path, G_FILE_TEST_IS_SYMLINK))
        return g_strdup (cdc_wdm_path);

    aux = realpath (cdc_wdm_path, NULL);
    if (!aux) {
        int saved_errno = errno;

        g_set_error (error, QMI_CORE_ERROR, QMI_CORE_ERROR_FAILED,
                     "Couldn't get realpath: %s", g_strerror (saved_errno));
        return NULL;
    }

    return aux;
}

gchar *
__qmi_utils_get_devname (const gchar  *cdc_wdm_path,
                         GError      **error)
{
    gchar *aux;
    gchar *devname = NULL;

    aux = __qmi_utils_get_devpath (cdc_wdm_path, error);
    if (aux) {
        devname = g_path_get_basename (aux);
        g_free (aux);
    }

    return devname;
}

/*****************************************************************************/

static volatile gint __traces_enabled = FALSE;

gboolean
qmi_utils_get_traces_enabled (void)
{
    return (gboolean) g_atomic_int_get (&__traces_enabled);
}

void
qmi_utils_set_traces_enabled (gboolean enabled)
{
    g_atomic_int_set (&__traces_enabled, enabled);
}
