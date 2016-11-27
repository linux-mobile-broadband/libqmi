/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * qmi-firmware-update -- Command line tool to update firmware in QMI devices
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
 * Copyright (C) 2016 Bjørn Mork <bjorn@mork.no>
 * Copyright (C) 2016 Aleksander Morgado <aleksander@aleksander.es>
 *
 * Additional copyrights:
 *
 *    crc16 and hdlc parts:
 *      Copyright (C) 2010 Red Hat, Inc.
 *
 *    parts of this is based on gobi-loader, which is
 *     "Copyright 2009 Red Hat <mjg@redhat.com> - heavily based on work done by
 *      Alexander Shumakovitch <shurik@gwu.edu>
 *
 *    gobi 2000 support provided by Anssi Hannula <anssi.hannula@iki.fi>
 */

#include <config.h>

#include <stdio.h>
#include <endian.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <linux/types.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

#include <libqmi-glib.h>

#include "qfu-download-helpers.h"

static gboolean write_hdlc     (gint          fd,
                                const gchar  *in,
                                gsize         inlen,
                                GError      **error);
static gboolean read_hdlc      (gint          fd,
                                GError      **error);
static gboolean read_hdlc_full (gint          fd,
                                guint16      *seq_ack,
                                GError      **error);

/******************************************************************************/

static gchar *
utils_str_hex (gconstpointer mem,
               gsize         size,
               gchar         delimiter)
{
    const guint8 *data = mem;
    gsize         i;
    gsize         j;
    gsize         new_str_length;
    gchar        *new_str;

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

/******************************************************************************/
/* DLOAD protocol */

/*
 * Most of this is from Josuah Hill's DLOAD tool for iPhone
 * Some spec is also available in document 80-39912-1 Rev. E  DMSS Download Protocol Interface Specification and Operational Description
 * https://github.com/posixninja/DLOADTool/blob/master/dloadtool/dload.h
 *
 * The 0x70 switching command was found by snooping on firmware updates
 */

typedef enum {
    DLOAD_CMD_ACK = 0x02, /* Acknowledge receiving a packet */
    DLOAD_CMD_NOP = 0x06, /* No operation, useful for debugging */
    DLOAD_CMD_SDP = 0x70, /* Switch to Streaming DLOAD */
} DloadCmd;

/* 0x02 - guint8 cmd only */
/* 0x06 - guint8 cmd only */

typedef struct _DloadSdp DloadSdp;

struct _DloadSdp {
    guint8  cmd;       /* 0x70 */
    guint16 reserved;  /* 0x0000 */
} __attribute__ ((packed));

static gboolean
dload_switch_to_sdp (gint     fd,
                     GError **error)
{
    static const DloadSdp dload_sdp = {
        .cmd      = DLOAD_CMD_SDP,
        .reserved = 0x0000,
    };

    GError *inner_error = NULL;

    /* switch to SDP - this is required for some modems like MC7710 */
    if (!write_hdlc (fd, (const gchar *) &dload_sdp, sizeof (DloadSdp), error)) {
        g_prefix_error (error, "couldn't write request: ");
        return FALSE;
    }

	/* The modem could already be in SDP mode, so ignore "unsupported" errors */
    if (!read_hdlc (fd, &inner_error)) {
        if (!g_error_matches (inner_error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED)) {
            g_propagate_error (error, inner_error);
            return FALSE;
        }
        g_error_free (inner_error);
    }

    return TRUE;
}

/******************************************************************************/
/* HDLC */

/*
 * crc16 and HDLC escape code borrowed from modemmanager/libqcdm
 * Copyright (C) 2010 Red Hat, Inc.
 */

/* Table of CRCs for each possible byte, with a generator polynomial of 0x8408 */
static const guint16 crc_table[256] = {
    0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
    0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
    0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
    0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
    0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,
    0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
    0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
    0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,
    0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
    0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
    0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,
    0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
    0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9,
    0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
    0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
    0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
    0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7,
    0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
    0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,
    0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
    0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
    0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
    0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,
    0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
    0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3,
    0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
    0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
    0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
    0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1,
    0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
    0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,
    0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78
};

/* Calculate the CRC for a buffer using a seed of 0xffff */
static guint16
crc16 (const gchar *buffer,
       gsize        len)
{
    guint16 crc = 0xffff;

    while (len--)
            crc = crc_table[(crc ^ *buffer++) & 0xff] ^ (crc >> 8);
    return ~crc;
}

#define CONTROL 0x7e
#define ESCAPE  0x7d
#define MASK    0x20

static gsize
escape (const gchar *in, gsize inlen, gchar *out, gsize outlen)
{
    gsize i, j;

    for (i = 0, j = 0; i < inlen; i++) {
        /* Caller should give a big enough buffer */
        g_assert ((j + 1) < outlen);
        if (in[i] == CONTROL || in[i] == ESCAPE) {
            out[j++] = ESCAPE;
            out[j++] = in[i] ^ MASK;
        } else
            out[j++] = in[i];
    }
    return j;
}

static gsize
unescape (const gchar *in,
          gsize        inlen,
          gchar       *out,
          gsize        outlen)
{
    gsize    i, j = 0;
    gboolean escaping = FALSE;

    for (i = 0; i < inlen; i++) {
        /* Caller should give a big enough buffer */
        g_assert (j < outlen);
        if (escaping) {
            out[j++] = in[i] ^ MASK;
            escaping = FALSE;
        } else if (in[i] == ESCAPE) {
            escaping = TRUE;
        } else {
            out[j++] = in[i];
        }
    }

    return j;
}

/* copy a possibly escaped single byte to out */
static gsize
escape_byte (guint8  byte,
             gchar  *out,
             gsize   outlen)
{
    gsize j = 0;

    if (byte == CONTROL || byte == ESCAPE) {
        out[j++] = ESCAPE;
        byte ^= MASK;
    }
    out[j++] = byte;
    return j;
}

static gsize
hdlc_frame (const gchar *in,
            gsize        inlen,
            gchar       *out,
            gsize        outlen)
{
    guint16 crc;
    gsize j = 0;

    out[j++] = CONTROL;
    j += escape (in, inlen, &out[j], outlen - j);
    crc = crc16 (in, inlen);
    j += escape_byte (crc & 0xff, &out[j], outlen - j);
    j += escape_byte (crc >> 8 & 0xff, &out[j], outlen - j);
    out[j++] = CONTROL;

    return j;
}

static gsize
hdlc_unframe (const gchar  *in,
              gsize         inlen,
              gchar        *out,
              gsize         outlen,
              GError      **error)
{
    guint16 crc;
    gsize j, i = inlen;

    /* the first control char is optional */
    if (*in == CONTROL) {
        in++;
        i--;
    }
    if (in[i - 1] == CONTROL)
        i--;

    j = unescape (in, i, out, outlen);
    if (j < 2) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "unescaping failed: too few bytes as output: %" G_GSIZE_FORMAT, j);
        return 0;
    }
    j -= 2; /* remove the crc */

    /* verify the crc */
    crc = crc16 (out, j);
    if (crc != ((guint8) out[j] | (guint8) out[j + 1] << 8)) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "crc check failed: 0x%04x != 0x%04x\n", crc, out[j] | out[j + 1] << 8);
        return 0;
    }

    return j;
}

/******************************************************************************/
/* Sierra */

/* Sierra Wireless CWE file header
 *   Note: 32bit numbers are big endian
 */
typedef struct _CweHdr CweHdr;
struct _CweHdr {
	gchar   reserved1[256];
	guint32 crc;		 /* 32bit CRC of "reserved1" field */
	guint32 rev;		 /* header revision */
	guint32 val;		 /* CRC validity indicator */
	gchar   type[4];	 /* ASCII - not null terminated */
	gchar   product[4];  /* ASCII - not null terminated */
	guint32 imgsize;	 /* image size */
	guint32 imgcrc;		 /* 32bit CRC of the image */
	gchar   version[84]; /* ASCII - null terminated */
	gchar   date[8];	 /* ASCII - null terminated */
	guint32 compat;		 /* backward compatibility */
	gchar   reserved2[20];
} __attribute__ ((packed));

static void
verify_cwehdr (gchar *buf)
{
	CweHdr *hdr = (void *) buf;

    g_debug ("[qfu-download,cwe] revision:   %u",   GUINT32_FROM_BE (hdr->rev));
    g_debug ("[qfu-download,cwe] type:       %.4s", hdr->type);
    g_debug ("[qfu-download,cwe] product:    %.4s", hdr->product);
    g_debug ("[qfu-download,cwe] image size: %u",   GUINT32_FROM_BE (hdr->imgsize));
    g_debug ("[qfu-download,cwe] version:    %s",   hdr->version);
    g_debug ("[qfu-download,cwe] date:       %s",   hdr->date);
}

/******************************************************************************/
/* QDL */

/* from GobiAPI_1.0.40/Core/QDLEnum.h and
 * GobiAPI_1.0.40/Core/QDLBuffers.h with additional details from USB
 * snooping
 */
typedef enum {
    QDL_CMD_HELLO_REQ          = 0x01,
    QDL_CMD_HELLO_RSP          = 0x02,
    QDL_CMD_ERROR              = 0x0d,
    QDL_CMD_OPEN_UNFRAMED_REQ  = 0x25,
    QDL_CMD_OPEN_UNFRAMED_RSP  = 0x26,
    QDL_CMD_WRITE_UNFRAMED_REQ = 0x27,
    QDL_CMD_WRITE_UNFRAMED_RSP = 0x28,
    QDL_CMD_SESSION_DONE_REQ   = 0x29,
    QDL_CMD_SESSION_DONE_RSP   = 0x2a,
    QDL_CMD_DOWNLOAD_REQ       = 0x2b,
    QDL_CMD_SESSION_CLOSE_REQ  = 0x2d,
    QDL_CMD_GET_IMAGE_PREF_REQ = 0x2e,
    QDL_CMD_GET_IMAGE_PREF_RSP = 0x2f,
} QdlCmd;

static const gchar *
qdl_cmd_to_string (QdlCmd cmd)
{
    switch (cmd) {
    case QDL_CMD_HELLO_REQ:
        return "Hello request";
    case QDL_CMD_HELLO_RSP:
        return "Hello response";
    case QDL_CMD_ERROR:
        return "Error";
    case QDL_CMD_OPEN_UNFRAMED_REQ:
        return "Open unframed image write request";
    case QDL_CMD_OPEN_UNFRAMED_RSP:
        return "Open unframed image write response";
    case QDL_CMD_WRITE_UNFRAMED_REQ:
        return "Unframed image write request";
    case QDL_CMD_WRITE_UNFRAMED_RSP:
        return "Unframed image write response";
    case QDL_CMD_SESSION_DONE_REQ:
        return "Unframed session done request";
    case QDL_CMD_SESSION_DONE_RSP:
        return "Unframed session done response";
    case QDL_CMD_DOWNLOAD_REQ:
        return "Switch to download protocol request";
    case QDL_CMD_SESSION_CLOSE_REQ:
        return "Close unframed session request";
    case QDL_CMD_GET_IMAGE_PREF_REQ:
        return "Get image preference request";
    case QDL_CMD_GET_IMAGE_PREF_RSP:
        return "Get image preference response";
    default:
        return NULL;
    }
}

typedef enum {
    QDL_ERROR_NONE              = 0x00,
    QDL_ERROR_01_RESERVED       = 0x01,
    QDL_ERROR_BAD_ADDR          = 0x02,
    QDL_ERROR_BAD_LEN           = 0x03,
    QDL_ERROR_BAD_PACKET        = 0x04,
    QDL_ERROR_BAD_CMD           = 0x05,
    QDL_ERROR_06                = 0x06,
    QDL_ERROR_OP_FAILED         = 0x07,
    QDL_ERROR_BAD_FLASH_ID      = 0x08,
    QDL_ERROR_BAD_VOLTAGE       = 0x09,
    QDL_ERROR_WRITE_FAILED      = 0x0a,
    QDL_ERROR_11_RESERVED       = 0x0b,
    QDL_ERROR_BAD_SPC           = 0x0c,
    QDL_ERROR_POWERDOWN         = 0x0d,
    QDL_ERROR_UNSUPPORTED       = 0x0e,
    QDL_ERROR_CMD_SEQ           = 0x0f,
    QDL_ERROR_CLOSE             = 0x10,
    QDL_ERROR_BAD_FEATURES      = 0x11,
    QDL_ERROR_SPACE             = 0x12,
    QDL_ERROR_BAD_SECURITY      = 0x13,
    QDL_ERROR_MULTI_UNSUPPORTED = 0x14,
    QDL_ERROR_POWEROFF          = 0x15,
    QDL_ERROR_CMD_UNSUPPORTED   = 0x16,
    QDL_ERROR_BAD_CRC           = 0x17,
    QDL_ERROR_STATE             = 0x18,
    QDL_ERROR_TIMEOUT           = 0x19,
    QDL_ERROR_IMAGE_AUTH        = 0x1a,
    QDL_ERROR_LAST
} QdlError;

static const gchar *qdl_error_str[] = {
    [QDL_ERROR_NONE              ] = "None",
    [QDL_ERROR_01_RESERVED       ] = "Reserved",
    [QDL_ERROR_BAD_ADDR          ] = "Invalid destination address",
    [QDL_ERROR_BAD_LEN           ] = "Invalid length",
    [QDL_ERROR_BAD_PACKET        ] = "Unexpected end of packet",
    [QDL_ERROR_BAD_CMD           ] = "Invalid command",
    [QDL_ERROR_06                ] = "Reserved",
    [QDL_ERROR_OP_FAILED         ] = "Operation failed",
    [QDL_ERROR_BAD_FLASH_ID      ] = "Invalid flash intelligent ID",
    [QDL_ERROR_BAD_VOLTAGE       ] = "Invalid programming voltage",
    [QDL_ERROR_WRITE_FAILED      ] = "Write verify failed",
    [QDL_ERROR_11_RESERVED       ] = "Reserved",
    [QDL_ERROR_BAD_SPC           ] = "Invalid security code",
    [QDL_ERROR_POWERDOWN         ] = "Power-down failed",
    [QDL_ERROR_UNSUPPORTED       ] = "NAND flash programming not supported",
    [QDL_ERROR_CMD_SEQ           ] = "Command out of sequence",
    [QDL_ERROR_CLOSE             ] = "Close failed",
    [QDL_ERROR_BAD_FEATURES      ] = "Invalid feature bits",
    [QDL_ERROR_SPACE             ] = "Out of space",
    [QDL_ERROR_BAD_SECURITY      ] = "Invalid security mode",
    [QDL_ERROR_MULTI_UNSUPPORTED ] = "Multi-image NAND not supported",
    [QDL_ERROR_POWEROFF          ] = "Power-off command not supported",
    [QDL_ERROR_CMD_UNSUPPORTED   ] = "Command not supported",
    [QDL_ERROR_BAD_CRC           ] = "Invalid CRC",
    [QDL_ERROR_STATE             ] = "Command received in invalid state",
    [QDL_ERROR_TIMEOUT           ] = "Receive timeout",
    [QDL_ERROR_IMAGE_AUTH        ] = "Image authentication error",
};

G_STATIC_ASSERT (G_N_ELEMENTS (qdl_error_str) == QDL_ERROR_LAST);

static GIOErrorEnum
qdl_error_to_gio_error_enum (QdlError err)
{
    switch (err) {
    case QDL_ERROR_CMD_UNSUPPORTED:
        return G_IO_ERROR_NOT_SUPPORTED;
    default:
        return G_IO_ERROR_FAILED;
    }
}

static const gchar *
qdl_error_to_string (QdlError err)
{
    return (err < QDL_ERROR_LAST ? qdl_error_str[err] : "Unknown");
}

/* most of these origin from GobiAPI_1.0.40/Core/QDLEnum.h
 *
 * The gobi-loader's snooped magic strings use types
 *   0x05 => "amss.mbn"
 *   0x06 => "apps.mbn"
 *   0x0d => "uqcn.mbn" (Gobi 2000 only)
 *  with no file header data
 *
 * The 0x80 type is snooped from the Sierra Wireless firmware
 * uploaders, using 400 bytes file header data
 */

typedef enum {
    QDL_IMAGE_TYPE_AMSS_MODEM       = 0x05,
    QDL_IMAGE_TYPE_AMSS_APPLICATION = 0x06,
    QDL_IMAGE_TYPE_AMSS_UQCN        = 0x0d,
    QDL_IMAGE_TYPE_DBL              = 0x0f,
    QDL_IMAGE_TYPE_OSBL             = 0x10,
    QDL_IMAGE_TYPE_CWE              = 0x80,
} QdlImageType;

static const gchar *
qdl_image_type_to_string (QdlImageType type)
{
    switch (type) {
    case QDL_IMAGE_TYPE_AMSS_MODEM:
        return "AMSS modem image";
    case QDL_IMAGE_TYPE_AMSS_APPLICATION:
        return "AMSS application image";
    case QDL_IMAGE_TYPE_AMSS_UQCN:
        return "AMSS Provisioning information image";
    case QDL_IMAGE_TYPE_DBL:
        return "DBL image";
    case QDL_IMAGE_TYPE_OSBL:
        return "OSBL image";
    case QDL_IMAGE_TYPE_CWE:
        return "CWE image";
    default:
        return "Unknown image";
    }
}

/* feature bits */
#define QDL_FEATURE_GENERIC_UNFRAMED 0x10
#define QDL_FEATURE_QDL_UNFRAMED     0x20
#define QDL_FEATURE_BAR_MODE         0x40

/* Generic message for operations that just require the command id */
typedef struct _QdlMsg QdlMsg;
struct _QdlMsg {
    guint8 cmd;
} __attribute__ ((packed));

typedef struct _QdlHelloReq QdlHelloReq;
struct _QdlHelloReq {
    guint8 cmd; /* 0x01 */
    gchar  magic[32];
    guint8 maxver;
    guint8 minver;
    guint8 features;
} __attribute__ ((packed));

static const QdlHelloReq qdl_hello_req = {
    .cmd = QDL_CMD_HELLO_REQ,
    .magic = { 'Q', 'C', 'O', 'M', ' ', 'h', 'i', 'g', 'h', ' ', 's', 'p', 'e', 'e', 'd', ' ', 'p', 'r', 'o', 't', 'o', 'c', 'o', 'l', ' ', 'h', 's', 't', },
    .maxver = 0,
    .minver = 0,
    .features = QDL_FEATURE_QDL_UNFRAMED | QDL_FEATURE_GENERIC_UNFRAMED,
};

typedef struct _QdlHelloRsp QdlHelloRsp;
struct _QdlHelloRsp {
    guint8  cmd; /* 0x02 */
    gchar   magic[32];
    guint8  maxver;
    guint8  minver;
    guint32 reserved1;
    guint32 reserved2;
    guint8  reserved3;
    guint16 reserved4;
    guint16 reserved5;
    guint8  features;
} __attribute__ ((packed));

typedef struct _QdlErrRsp QdlErrRsp;
struct _QdlErrRsp {
    guint8  cmd; /* 0x0d */
    guint32 error;
    guint8  errortxt;
} __attribute__ ((packed));

typedef struct _QdlUfopenReq QdlUfopenReq;
struct _QdlUfopenReq {
    guint8  cmd; /* 0x25 */
    guint8  type;
    guint32 length;
    guint8  windowsize;
    guint32 chunksize;
    guint16 reserved;
    /* The first 400 bytes of the image is appended to the "open
     * unframed" request on Sierra Wireless modems.  This chunk is
     * not included here as it is not a part of the request.
     *
     * The file header inclusion here seems to depend on the file
     * type
     */
} __attribute__ ((packed));

typedef struct _QdlUfopenRsp QdlUfopenRsp;
struct _QdlUfopenRsp {
    guint8  cmd; /* 0x26 */
    guint16 status;
    guint8  windowsize;
    guint32 chunksize;
} __attribute__ ((packed));

/* This request is not HDLC framed, so this "header" includes the crc */
typedef struct _QdlUfwriteReq QdlUfwriteReq;
struct _QdlUfwriteReq {
    guint8  cmd; /* 0x27 */
    guint16 sequence;
    guint32 reserved;
    guint32 chunksize;
    guint16 crc;
} __attribute__ ((packed));

/* the buffer most hold a file chunk + this header */
#define CHUNK   (1024 * 1024)
#define BUFSIZE (CHUNK + sizeof (QdlUfwriteReq))

/* The response is HDLC framed, so the crc is part of the framing */
typedef struct _QdlUfwriteRsp QdlUfwriteRsp;
struct _QdlUfwriteRsp {
    guint8  cmd; /* 0x28 */
    guint16 sequence;
    guint32 reserved;
    guint16 status;
} __attribute__ ((packed));

/* 0x29 - cmd only */

typedef struct _QdlUfcloseRsp QdlUfcloseRsp;
struct _QdlUfcloseRsp {
    guint8  cmd; /* 0x2a */
    guint16 status;
    guint8  type;
    guint8  errortxt;
} __attribute__ ((packed));

/* 0x2d - cmd only */
/* 0x2e - cmd only */

typedef struct _QdlImageprefRsp QdlImageprefRsp;
struct _QdlImageprefRsp {
    guint8 cmd; /* 0x2f */
    guint8 entries;
    struct {
        guint8 type;
        gchar  id[16];
    } image[];
} __attribute__ ((packed));

/* should the unframed open request include a file header? */
static inline gsize
qdl_image_get_hdrlen (QdlImageType type)
{
	switch (type) {
	case QDL_IMAGE_TYPE_CWE:
		return 400;
	default:
		return 0;
	}
}

/* some image types contain trailing garbage - from gobi-loader */
static inline gsize
qdl_image_get_len (QdlImageType type, gsize len)
{
	switch (type) {
	case QDL_IMAGE_TYPE_AMSS_MODEM:
		return len - 8;
	default:
		return len;
	}
}

static gsize
create_ufopen_req (gchar        *out,
                   gsize         outlen,
                   gsize         filelen,
                   QdlImageType  type)
{
    QdlUfopenReq *req = (void *) out;

    g_assert (outlen >= sizeof (QdlUfopenReq));

	req->cmd = QDL_CMD_OPEN_UNFRAMED_REQ;
	req->type = (guint8) type;
	req->windowsize = 1; /* snooped */
	req->length = qdl_image_get_len (type, filelen);
	req->chunksize = req->length - qdl_image_get_hdrlen (type);
	return sizeof (QdlUfopenReq);
}

static gsize
create_ufwrite_req (gchar *out,
                    gsize  outlen,
                    gsize  chunksize,
                    gint   sequence)
{
    QdlUfwriteReq *req = (void *) out;

	req->cmd = QDL_CMD_WRITE_UNFRAMED_REQ;
	req->sequence = sequence;
	req->reserved = 0;
	req->chunksize = chunksize;
	req->crc = crc16 (out, sizeof (QdlUfwriteReq) - 2);
	return sizeof (QdlUfwriteReq);
}

static gboolean
process_sdp_hello (const gchar  *in,
                   gsize         inlen,
                   GError      **error)
{
	gchar        buf[sizeof (QdlHelloRsp) + sizeof (guint16)];
    QdlHelloRsp *rsp;
	gsize        ret;

	rsp = (QdlHelloRsp *) buf;

	ret = hdlc_unframe (in, inlen, buf, sizeof (buf), error);
    if (ret == 0) {
        g_prefix_error (error, "error unframing message: ");
        return FALSE;
    }
	if (ret != sizeof (QdlHelloRsp)) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "unframed message size mismatch: %" G_GSIZE_FORMAT " != %" G_GSIZE_FORMAT,
                     ret, sizeof (QdlHelloRsp));
        return FALSE;
    }

    /* Unframing should keep command */
    g_assert (rsp->cmd == QDL_CMD_HELLO_RSP);

    g_debug ("[qfu-download,qdl] %s:", qdl_cmd_to_string ((QdlCmd) rsp->cmd));
    g_debug ("[qfu-download,qdl]   magic:           %.*s\n",   rsp->maxver <= 5 ? 24 : 32, rsp->magic);
    g_debug ("[qfu-download,qdl]   maximum version: %u\n",     rsp->maxver);
    g_debug ("[qfu-download,qdl]   minimum version: %u\n",     rsp->minver);
    g_debug ("[qfu-download,qdl]   features:        0x%02x\n", rsp->features);

    return TRUE;
}

static gboolean
process_sdp_err (const gchar  *in,
                 gsize         inlen,
                 GError      **error)
{
	gchar      buf[sizeof (QdlErrRsp) + sizeof (guint16)];
    QdlErrRsp *rsp;
	gsize      ret;

	rsp = (QdlErrRsp *) buf;

	ret = hdlc_unframe (in, inlen, buf, sizeof (buf), error);
    if (ret == 0) {
        g_prefix_error (error, "error unframing message: ");
        return FALSE;
    }
	if (ret != sizeof (QdlErrRsp)) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "unframed message size mismatch: %" G_GSIZE_FORMAT " != %" G_GSIZE_FORMAT,
                     ret, sizeof (QdlErrRsp));
        return FALSE;
    }

    /* Unframing should keep command */
    g_assert (rsp->cmd == QDL_CMD_ERROR);

    g_debug ("[qfu-download,qdl] %s", qdl_cmd_to_string ((QdlCmd) rsp->cmd));
    g_debug ("[qfu-download,qdl]   error:    %d", rsp->error);
    g_debug ("[qfu-download,qdl]   errortxt: %d", rsp->errortxt);

    /* Always return an error in this case */

    g_set_error (error, G_IO_ERROR, qdl_error_to_gio_error_enum (rsp->error),
                 "%s", qdl_error_to_string ((QdlError) rsp->error));
    return FALSE;
}

static gboolean
process_ufopen (const gchar  *in,
                gsize         inlen,
                GError      **error)
{
	gchar         buf[sizeof (QdlUfopenRsp) + sizeof (guint16)];
    QdlUfopenRsp *rsp;
	gsize         ret;

	rsp = (QdlUfopenRsp *) buf;

	ret = hdlc_unframe (in, inlen, buf, sizeof (buf), error);
    if (ret == 0) {
        g_prefix_error (error, "error unframing message: ");
        return FALSE;
    }
	if (ret != sizeof (QdlUfopenRsp)) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "unframed message size mismatch: %" G_GSIZE_FORMAT " != %" G_GSIZE_FORMAT,
                     ret, sizeof (QdlUfopenRsp));
        return FALSE;
    }

    /* Unframing should keep command */
    g_assert (rsp->cmd == QDL_CMD_OPEN_UNFRAMED_RSP);

    g_debug ("[qfu-download,qdl] %s", qdl_cmd_to_string ((QdlCmd) rsp->cmd));
    g_debug ("[qfu-download,qdl]   status:      %d", rsp->status);
    g_debug ("[qfu-download,qdl]   window size: %d", rsp->windowsize);
    g_debug ("[qfu-download,qdl]   chunk size:  %d", rsp->chunksize);

    /* Return error if status != 0 */
    if (rsp->status != 0) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "operation returned an error status: %d",
                     rsp->status);
        return FALSE;
    }

    return TRUE;
}

static gboolean
process_ufwrite (const gchar  *in,
                 gsize         inlen,
                 guint16      *sequence,
                 GError      **error)
{
	gchar          buf[sizeof (QdlUfwriteRsp) + sizeof (guint16)];
    QdlUfwriteRsp *rsp;
	gsize          ret;

	rsp = (QdlUfwriteRsp *) buf;

	ret = hdlc_unframe (in, inlen, buf, sizeof (buf), error);
    if (ret == 0) {
        g_prefix_error (error, "error unframing message: ");
        return FALSE;
    }
	if (ret != sizeof (QdlUfwriteRsp)) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "unframed message size mismatch: %" G_GSIZE_FORMAT " != %" G_GSIZE_FORMAT,
                     ret, sizeof (QdlUfwriteRsp));
        return FALSE;
    }

    /* Unframing should keep command */
    g_assert (rsp->cmd == QDL_CMD_WRITE_UNFRAMED_RSP);

    g_debug ("[qfu-download,qdl] %s", qdl_cmd_to_string ((QdlCmd) rsp->cmd));
    g_debug ("[qfu-download,qdl]   status:   %d",     rsp->status);
    g_debug ("[qfu-download,qdl]   sequence: 0x%04x", rsp->sequence);

    /* Return error if status != 0 */
    if (rsp->status != 0) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "operation returned an error status: %d",
                     rsp->status);
        return FALSE;
    }

    if (sequence)
        *sequence = rsp->sequence;
    return TRUE;
}

static gboolean
process_ufdone (const gchar  *in,
                gsize         inlen,
                GError      **error)
{
	gchar          buf[sizeof (QdlUfcloseRsp) + sizeof (guint16)];
    QdlUfcloseRsp *rsp;
	gsize          ret;

	rsp = (QdlUfcloseRsp *) buf;

	ret = hdlc_unframe (in, inlen, buf, sizeof (buf), error);
    if (ret == 0) {
        g_prefix_error (error, "error unframing message: ");
        return FALSE;
    }
	if (ret != sizeof (QdlUfcloseRsp)) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "unframed message size mismatch: %" G_GSIZE_FORMAT " != %" G_GSIZE_FORMAT,
                     ret, sizeof (QdlUfcloseRsp));
        return FALSE;
    }

    /* Unframing should keep command */
    g_assert (rsp->cmd == QDL_CMD_SESSION_DONE_RSP);

    g_debug ("[qfu-download,qdl] %s", qdl_cmd_to_string ((QdlCmd) rsp->cmd));
    g_debug ("[qfu-download,qdl]   status:   %d", rsp->status);
    g_debug ("[qfu-download,qdl]   type:     %d", rsp->type);
    g_debug ("[qfu-download,qdl]   errortxt: %d", rsp->errortxt);

    /* Return error if status != 0 */
    if (rsp->status != 0) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "operation returned an error status: %d",
                     rsp->status);
        return FALSE;
    }

    return TRUE;
}

static gboolean
qdl_detect_version (gint     fd,
                    GError **error)
{
    guint    version;
    gboolean found = FALSE;

	/* Attempt to probe supported protocol version
	 *  Newer modems like Sierra Wireless MC7710 must use '6' for both fields
	 *  Gobi2000 modems like HP un2420 must use '5' for both fields
	 *  Gobi1000 modems  must use '4' for both fields
	 */
	for (version = 4; !found && version < 7; version++) {
        QdlHelloReq req;

        memcpy (&req, &qdl_hello_req, sizeof (QdlHelloReq));
		req.minver = version;
		req.maxver = version;

		if (!write_hdlc (fd, (const gchar *) &req, sizeof (QdlHelloReq), error)) {
            g_prefix_error (error, "couldn't write request: ");
            return FALSE;
        }

        /* If no error processing the response, we assume version is found */
        found = read_hdlc (fd, NULL);
    }

    if (!found) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "couldn't detect QDL version");
        return FALSE;
    }

    g_debug ("[qfu-download,qdl] QDL version detected: %u", version);
    return TRUE;
}

static gboolean
qdl_session_done (gint     fd,
                  GError **error)
{
    static const QdlMsg session_done = {
        .cmd =  QDL_CMD_SESSION_DONE_REQ,
    };

    if (!write_hdlc (fd, (const gchar *) &session_done, sizeof (QdlMsg), error)) {
        g_prefix_error (error, "couldn't write session done request: ");
        return FALSE;
    }

    if (!read_hdlc (fd, error)) {
        g_prefix_error (error, "error reported in session done request: ");
        return FALSE;
    }

    return TRUE;
}

static gboolean
qdl_session_close (gint     fd,
                   GError **error)
{
    static const QdlMsg session_close = {
        .cmd =  QDL_CMD_SESSION_CLOSE_REQ,
    };

    if (!write_hdlc (fd, (const gchar *) &session_close, sizeof (QdlMsg), error)) {
        g_prefix_error (error, "couldn't write session close request: ");
        return FALSE;
    }

    if (!read_hdlc (fd, error)) {
        g_prefix_error (error, "error reported in session close request: ");
        return FALSE;
    }

    return TRUE;
}

/* guessing image type based on the well known Gobi 1k and 2k
 * filenames, and assumes anything else is a CWE image
 *
 * This is based on the types in gobi-loader's snooped magic strings:
 *   0x05 => "amss.mbn"
 *   0x06 => "apps.mbn"
 *   0x0d => "uqcn.mbn" (Gobi 2000 only)
 */
static QdlImageType
qdl_image_image_get_type_from_file (GFile *image)
{
	gchar        *basename;
    QdlImageType  image_type;

    g_assert (G_IS_FILE (image));
    basename = g_file_get_basename (image);

    if (g_ascii_strcasecmp (basename, "amss.mbn") == 0)
        image_type = QDL_IMAGE_TYPE_AMSS_MODEM;
    else if (g_ascii_strcasecmp (basename, "apps.mbn") == 0)
        image_type = QDL_IMAGE_TYPE_AMSS_APPLICATION;
    else if (g_ascii_strcasecmp (basename, "uqcn.mbn") == 0)
        image_type = QDL_IMAGE_TYPE_AMSS_UQCN;
    else
        image_type = QDL_IMAGE_TYPE_CWE;

    g_free (basename);
    return image_type;
}

static gboolean
qdl_download_image (gint           fd,
                    gchar         *buf,
                    gsize          buflen,
                    GFile         *image,
                    GFileInfo     *image_info,
                    GCancellable  *cancellable,
                    GError       **error)
{
    gint          image_fd;
    gchar        *image_path;
    GError       *inner_error = NULL;
    gssize        image_datalen;
    QdlImageType  image_type;
    gsize         reqlen;
    gsize         image_hdrlen;
    guint16       seq = 0;
    guint16       acked_sequence = 0;

    image_path = g_file_get_path (image);

    image_fd = open (image_path, O_RDONLY);
    if (image_fd < 0) {
        inner_error = g_error_new (G_IO_ERROR, G_IO_ERROR_FAILED,
                                   "error opening image file: %s",
                                   g_strerror (errno));
        goto out;
    }

    image_type = qdl_image_image_get_type_from_file (image);
    image_hdrlen = qdl_image_get_hdrlen (image_type);
    image_datalen = qdl_image_get_len (image_type, (gsize) g_file_info_get_size (image_info)) - image_hdrlen;
	if (image_datalen < 0) {
        inner_error = g_error_new (G_IO_ERROR, G_IO_ERROR_FAILED,
                                   "image is too short");
		goto out;
	}

    g_debug ("[qfu-download,qdl] downloading %s: %s",
             qdl_image_type_to_string (image_type),
             g_file_info_get_display_name (image_info));

	/* send open request */
	reqlen = create_ufopen_req (buf, buflen, (gsize) g_file_info_get_size (image_info), image_type);
	if (image_hdrlen > 0) {
		g_assert (image_hdrlen + reqlen <= buflen);
        /* read header from image file */
		if (read (image_fd, buf + reqlen, image_hdrlen) != image_hdrlen) {
            inner_error = g_error_new (G_IO_ERROR, G_IO_ERROR_FAILED,
                                       "couldn't read image header: %s",
                                       g_strerror (errno));
            goto out;
        }
		if (image_type == QDL_IMAGE_TYPE_CWE)
			verify_cwehdr (buf + reqlen);
	}
	if (!write_hdlc (fd, buf, reqlen + image_hdrlen, &inner_error)) {
        g_prefix_error (&inner_error, "couldn't write open request");
        goto out;
    }

	/* read ufopen response */
	if (!read_hdlc (fd, &inner_error)) {
        g_prefix_error (&inner_error, "error detected in open request: ");
        goto out;
    }

	/* remaining data to send */
	while (image_datalen > 0) {
        gsize          chunksize;
		fd_set         wr;
		struct timeval tv = { .tv_sec = 2, .tv_usec = 0, };
        gint           aux;

        /* Check cancellable on every loop */
        if (g_cancellable_is_cancelled (cancellable)) {
            inner_error = g_error_new (G_IO_ERROR, G_IO_ERROR_CANCELLED,
                                       "download operation cancelled");
            goto out;
        }

		chunksize = MIN (CHUNK, image_datalen);

		g_debug ("[qfu-download,qdl] writing chunk #%u (%" G_GSIZE_FORMAT ")...", (guint) seq, chunksize);
		reqlen = create_ufwrite_req (buf, buflen, chunksize, seq);
		aux = read (image_fd, buf + reqlen, chunksize);
        if (aux != chunksize) {
            inner_error = g_error_new (G_IO_ERROR, G_IO_ERROR_FAILED,
                                       "error reading chunk #%u: %s",
                                       seq, g_strerror (errno));
            goto out;
        }

		FD_ZERO (&wr);
		FD_SET (fd, &wr);
        aux = select (fd + 1, NULL, &wr, NULL, &tv);
        if (aux < 0) {
            inner_error = g_error_new (G_IO_ERROR, G_IO_ERROR_FAILED,
                                       "error waiting to write #%u: %s",
                                       seq, g_strerror (errno));
            goto out;
        }
        if (aux == 0) {
            inner_error = g_error_new (G_IO_ERROR, G_IO_ERROR_FAILED,
                                       "timed out waiting to write chunk #%u",
                                       seq);
            goto out;
        }

        /* Note: no HDLC here */
		if (write (fd, buf, reqlen + chunksize) < 0) {
            inner_error = g_error_new (G_IO_ERROR, G_IO_ERROR_FAILED,
                                       "error writing chunk #%u: %s",
                                       seq, g_strerror (errno));
            goto out;
        }

        if (read_hdlc_full (fd, &acked_sequence, NULL))
            g_debug ("[qfu-download,qdl] ack-ed chunk #%u", (guint) acked_sequence);

		image_datalen -= chunksize;
        g_assert (image_datalen >= 0);

        seq++;
	}

    g_debug ("[qfu-download,qdl] finished writing: waiting for final chunk #%u ack", (seq - 1));

	/* This may take a considerable amount of time */
	while (acked_sequence != (seq - 1)) {
        /* Wait some time */
        g_usleep (3 * G_USEC_PER_SEC);

        /* Check cancellable on every loop */
        if (g_cancellable_is_cancelled (cancellable)) {
            inner_error = g_error_new (G_IO_ERROR, G_IO_ERROR_CANCELLED,
                                       "download operation cancelled");
            goto out;
        }

        if (read_hdlc_full (fd, &acked_sequence, NULL))
            g_debug ("[qfu-download,qdl] ack-ed chunk #%u", (guint) acked_sequence);
	}

    g_debug ("[qfu-download,qdl] all chunks ack-ed");

out:

	if (!(image_fd < 0))
		close (image_fd);
    g_free (image_path);

    if (inner_error) {
        g_propagate_error (error, inner_error);
        return FALSE;
    }

	return TRUE;
}

/******************************************************************************/
/* R/W HDLC */

static gboolean
write_hdlc (gint          fd,
            const gchar  *in,
            gsize         inlen,
            GError      **error)
{
    gchar  wbuf[512];
    gsize  wlen;
    gssize written;

    /* Pack into an HDLC frame */
    wlen = hdlc_frame (in, inlen, wbuf, sizeof (wbuf));
    g_assert_cmpuint (wlen, >, 0);

    /* Send HDLC frame to device */
    written = write (fd, wbuf, wlen);
    if (written < 0) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "couldn't write HDLC frame: %s",
                     g_strerror (errno));
        return FALSE;
    }

    /* Treat this as an error, we may want to improve this by writing out the
     * remaning amount of bytes */
    if (written != wlen) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "error writing full HDLC frame: only %" G_GSSIZE_FORMAT "/%" G_GSIZE_FORMAT " bytes written",
                     written, wlen);
        return FALSE;
    }

    /* Debug output */
    if (qmi_utils_get_traces_enabled ()) {
        gchar *printable;

        printable = utils_str_hex (wbuf, wlen, ':');
        g_debug ("[qfu-download,hdlc] >> %s", printable);
        g_free (printable);
    }

    return TRUE;
}

static gboolean
read_hdlc_full (gint      fd,
                guint16  *seq_ack,
                GError  **error)
{
	gchar          *end;
    gchar          *p;
    gchar           rbuf[512];
	gsize           rlen;
    gsize           framelen;
	fd_set          rd;
	struct timeval  tv = { .tv_sec = 1, .tv_usec = 0, };

	FD_ZERO (&rd);
	FD_SET (fd, &rd);
	if (select (fd + 1, &rd, NULL, NULL, &tv) <= 0) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_TIMED_OUT,
                     "read operation timed out");
        return FALSE;
	}

	rlen = read (fd, rbuf, sizeof (rbuf));
	if (rlen < 0) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "couldn't read HDLC frame: %s",
                     g_strerror (errno));
        return FALSE;
    }

    if (rlen == 0) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "couldn't read HDLC frame: HUP detected");
        return FALSE;
    }

    /* Debug output */
    if (qmi_utils_get_traces_enabled ()) {
        gchar *printable;

        printable = utils_str_hex (rbuf, rlen, ':');
        g_debug ("[qfu-download,hdlc] << %s", printable);
        g_free (printable);
    }

	p = rbuf;
	while (rlen > 0 && (end = memchr (p + 1, CONTROL, rlen - 1)) != NULL) {
		end++;
		framelen = end - p;
		switch (p[1]) {
		case QDL_CMD_ERROR:
            if (!process_sdp_err (p, framelen, error))
                return FALSE;
            break;
		case QDL_CMD_HELLO_RSP: /* == DLOAD_CMD_ACK */
			if (framelen != 5) {
                if (!process_sdp_hello (p, framelen, error))
                    return FALSE;
            } else
				g_debug("[qfu-download,dload] ack");
			break;
		case QDL_CMD_OPEN_UNFRAMED_RSP:
            if (!process_ufopen (p, framelen, error))
                return FALSE;
			break;
		case QDL_CMD_WRITE_UNFRAMED_RSP:
            if (!process_ufwrite (p, framelen, seq_ack, error))
                return FALSE;
			break;
		case QDL_CMD_SESSION_DONE_RSP:
            if (!process_ufdone (p, framelen, error))
                return FALSE;
			break;
		default:
            g_warning ("qfu-download,hdlc] unsupported message received: 0x%02x\n", p[1]);
            break;
		}

		p = end;
		rlen -= framelen;
	}

	return TRUE;
}

static gboolean
read_hdlc (gint      fd,
           GError  **error)
{
    return read_hdlc_full (fd, NULL, error);
}

/******************************************************************************/
/* Serial port */

static gint
serial_open (GFile   *tty,
             GError **error)
{
    struct termios  terminal_data;
    gint            fd;
    gchar          *path;

    path = g_file_get_path (tty);
    g_debug ("[qfu-download] opening TTY: %s", path);

    fd = open (path, O_RDWR | O_NOCTTY);
    if (fd < 0) {
        g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                     "error opening serial device: %s",
                     g_strerror (errno));
    } else {
        g_debug ("[qfu-download] setting terminal in raw mode...");
        tcgetattr (fd, &terminal_data);
        cfmakeraw (&terminal_data);
        tcsetattr (fd, TCSANOW, &terminal_data);
    }

    g_free (path);

    return fd;
}

/******************************************************************************/

typedef struct {
    GFile     *tty;
    GFile     *image;
    GFileInfo *image_info;
    gint       fd;
    gchar     *buffer;
} RunContext;

static void
run_context_free (RunContext *ctx)
{
    if (!(ctx->fd < 0))
        close (ctx->fd);
    g_free (ctx->buffer);
    g_object_unref (ctx->image_info);
    g_object_unref (ctx->image);
    g_object_unref (ctx->tty);
    g_slice_free (RunContext, ctx);
}

gboolean
qfu_download_helper_run_finish (GAsyncResult  *res,
                                GError       **error)
{
    return g_task_propagate_boolean (G_TASK (res), error);
}

static void
download_helper_run (GTask *task)
{
    RunContext *ctx;
    GError     *error = NULL;

    ctx = (RunContext *) g_task_get_task_data (task);

    g_debug ("[qfu-download] opening tty...");
    ctx->fd = serial_open (ctx->tty, &error);
    if (ctx->fd < 0) {
        g_prefix_error (&error, "couldn't open serial device:");
        goto out;
    }

    if (g_task_return_error_if_cancelled (task))
        return;

    g_debug ("[qfu-download] switching to SDP...");
    if (!dload_switch_to_sdp (ctx->fd, &error)) {
        g_prefix_error (&error, "couldn't switch to SDP: ");
        goto out;
    }

    if (g_task_return_error_if_cancelled (task))
        return;

    g_debug ("[qfu-download] detecting QDL version...");
    if (!qdl_detect_version (ctx->fd, &error)) {
        g_prefix_error (&error, "couldn't detect QDL version: ");
        goto out;
    }

    if (g_task_return_error_if_cancelled (task))
        return;

    if (!qdl_download_image (ctx->fd,
                             ctx->buffer,
                             BUFSIZE,
                             ctx->image,
                             ctx->image_info,
                             g_task_get_cancellable (task),
                             &error)) {
        g_prefix_error (&error, "couldn't download image: ");
        goto out;
    }

    if (!qdl_session_done (ctx->fd, &error)) {
        /* Error not fatal */
        g_debug ("[qfu-download] error reporting session done: %s", error->message);
        g_clear_error (&error);
    }

    if (!qdl_session_close (ctx->fd, &error)) {
        /* Error not fatal */
        g_debug ("[qfu-download] error closing session: %s", error->message);
        g_clear_error (&error);
    }

out:
    if (error)
        g_task_return_error (task, error);
    else
        g_task_return_boolean (task, TRUE);
    g_object_unref (task);
}

void
qfu_download_helper_run (GFile               *tty,
                         GFile               *image,
                         GFileInfo           *image_info,
                         GCancellable        *cancellable,
                         GAsyncReadyCallback  callback,
                         gpointer             user_data)
{
    GTask      *task;
    RunContext *ctx;

    g_assert (G_IS_FILE (tty));
    g_assert (G_IS_FILE (image));

    ctx = g_slice_new0 (RunContext);
    ctx->fd = -1;
    ctx->tty = g_object_ref (tty);
    ctx->image = g_object_ref (image);
    ctx->image_info = g_object_ref (image_info);
    ctx->buffer = g_malloc (BUFSIZE);

    task = g_task_new (NULL, cancellable, callback, user_data);
    g_task_set_task_data (task, ctx, (GDestroyNotify) run_context_free);

    /* Run */
    download_helper_run (task);
}
