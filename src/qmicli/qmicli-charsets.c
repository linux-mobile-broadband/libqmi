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
 * Copyright (C) 2010-2017 Red Hat, Inc.
 */

#include "config.h"

#include <unistd.h>
#include <string.h>
#include <ctype.h>

#include <glib.h>

#include "qmicli-charsets.h"

/* GSM 03.38 encoding conversion stuff */

#define GSM_DEF_ALPHABET_SIZE 128
#define GSM_EXT_ALPHABET_SIZE 10

typedef struct GsmUtf8Mapping {
    gchar chars[3];
    guint8 len;
    guint8 gsm;  /* only used for extended GSM charset */
} GsmUtf8Mapping;

#define ONE(a)     { {a, 0x00, 0x00}, 1, 0 }
#define TWO(a, b)  { {a, b,    0x00}, 2, 0 }

/**
 * gsm_def_utf8_alphabet:
 *
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
gsm_def_char_to_utf8 (const guint8 gsm, guint8 out_utf8[2])
{
    g_return_val_if_fail (gsm < GSM_DEF_ALPHABET_SIZE, 0);
    memcpy (&out_utf8[0], &gsm_def_utf8_alphabet[gsm].chars[0], gsm_def_utf8_alphabet[gsm].len);
    return gsm_def_utf8_alphabet[gsm].len;
}

#define EONE(a, g)        { {a, 0x00, 0x00}, 1, g }
#define ETHR(a, b, c, g)  { {a, b,    c},    3, g }

/**
 * gsm_ext_utf8_alphabet:
 *
 * Mapping from GSM extended alphabet to UTF-8.
 *
 */
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
gsm_ext_char_to_utf8 (const guint8 gsm, guint8 out_utf8[3])
{
    int i;

    for (i = 0; i < GSM_EXT_ALPHABET_SIZE; i++) {
        if (gsm == gsm_ext_utf8_alphabet[i].gsm) {
            memcpy (&out_utf8[0], &gsm_ext_utf8_alphabet[i].chars[0], gsm_ext_utf8_alphabet[i].len);
            return gsm_ext_utf8_alphabet[i].len;
        }
    }
    return 0;
}

guint8 *
qmicli_charset_gsm_unpack (const guint8 *gsm,
                           guint32 num_septets,
                           guint32 *out_unpacked_len)
{
    GByteArray *unpacked;
    int i;

    unpacked = g_byte_array_sized_new (num_septets + 1);

    for (i = 0; i < num_septets; i++) {
        guint8 bits_here, bits_in_next, octet, offset, c;
        guint32 start_bit;

        start_bit = i * 7; /* Overall bit offset of char in buffer */
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

guint8 *
qmicli_charset_gsm_unpacked_to_utf8 (const guint8 *gsm, guint32 len)
{
    int i;
    GByteArray *utf8;

    g_return_val_if_fail (gsm != NULL, NULL);
    g_return_val_if_fail (len < 4096, NULL);

    /* worst case initial length */
    utf8 = g_byte_array_sized_new (len * 2 + 1);

    for (i = 0; i < len; i++) {
        guint8 uchars[4];
        guint8 ulen;

        if (gsm[i] == GSM_ESCAPE_CHAR) {
            /* Extended alphabet, decode next char */
            ulen = gsm_ext_char_to_utf8 (gsm[i+1], uchars);
            if (ulen)
                i += 1;
        } else {
            /* Default alphabet */
            ulen = gsm_def_char_to_utf8 (gsm[i], uchars);
        }

        if (ulen)
            g_byte_array_append (utf8, &uchars[0], ulen);
        else
            g_byte_array_append (utf8, (guint8 *) "?", 1);
    }

    g_byte_array_append (utf8, (guint8 *) "\0", 1);  /* NULL terminator */
    return g_byte_array_free (utf8, FALSE);
}
