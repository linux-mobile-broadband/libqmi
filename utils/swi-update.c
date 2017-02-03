/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * swi-update -- Command line tool to update QMI firmware
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
 * Copyright (C) Copyright 2016 Bjørn Mork <bjorn@mork.no>
 *
 * crc16 and hdlc parts:
 *   Copyright (C) 2010 Red Hat, Inc.
 *
 * parts of this is based on gobi-loader, which is
 *
 *  "Copyright 2009 Red Hat <mjg@redhat.com> - heavily based on work done by
 *   Alexander Shumakovitch <shurik@gwu.edu>
 *
 *   Gobi 2000 support provided by Anssi Hannula <anssi.hannula@iki.fi>"
 */

#include <endian.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <linux/types.h>
#include <malloc.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

/* FIXME: endianness - this works on LE for now... */

#ifndef VERSION
  #define VERSION "unknown"
#endif
#define DESCRIPTION "swi-update (" VERSION ")"

#ifdef DEBUG
static bool debug;
#define debug(arg...) if (debug) fprintf(stderr, arg)
static void print_packet(const char *pfx, void *buf, int len)
{
    int i;
    unsigned char *p = buf;

    if (!debug)
        return;
    fprintf(stderr, "%s: ", pfx);
    for (i=0; i<len; i++)
        fprintf(stderr, "%02x ", p[i]);
    fprintf(stderr, "\n");
}
#else
#define debug(arg...)
#define print_packet(pfx, buf, len)
#endif /* DEBUG */


#define CHUNK (1024 * 1024)

/** DLOAD protocol **/

/*
 * Most of this is from Josuah Hill's DLOAD tool for iPhone
 * Some spec is also available in document 80-39912-1 Rev. E  DMSS Download Protocol Interface Specification and Operational Description
 * https://github.com/posixninja/DLOADTool/blob/master/dloadtool/dload.h
 *
 * The 0x70 switching command was found by snooping on firmware updates
 */

enum dload_cmd {
    DLOAD_ACK = 0x02, /* Acknowledge receiving a packet */
    DLOAD_NOP = 0x06, /* No operation, useful for debugging */
    DLOAD_SDP = 0x70, /* Switch to Streaming DLOAD */
};

/* 0x02 - __u8 cmd only */
/* 0x06 - __u8 cmd only */

struct dload_sdp {
    __u8 cmd; /* 0x70 */
    __u16 reserved; /* 0x0000 */
} __attribute__ ((packed));

static struct dload_sdp dload_sdp = {
    .cmd = 0x70,
    .reserved = 0,
};

/** Streaming DLOAD protocol **/

/* from GobiAPI_1.0.40/Core/QDLEnum.h and
 * GobiAPI_1.0.40/Core/QDLBuffers.h with additional details from USB
 * snooping
 */
enum qdl_cmd {
    QDL_CMD_HELLO_REQ       = 0x01, /* Hello request */
    QDL_CMD_HELLO_RSP,          /* Hello response */
    QDL_CMD_ERROR           = 0x0d, /* Error report */
    QDL_CMD_OPEN_UNFRAMED_REQ   = 0x25, /* Open unframed image write request */
    QDL_CMD_OPEN_UNFRAMED_RSP,      /* Open unframed image write response */
    QDL_CMD_WRITE_UNFRAMED_REQ,     /* Unframed image write request */
    QDL_CMD_WRITE_UNFRAMED_RSP,     /* Unframed image write response */
    QDL_CMD_SESSION_DONE_REQ,       /* Unframed session done request */
    QDL_CMD_SESSION_DONE_RSP,       /* Unframed session done response */
    QDL_CMD_DOWNLOAD_REQ,           /* Switch to download protocol request */
    QDL_CMD_SESSION_CLOSE_REQ   = 0x2d, /* Close unframed session request */
    QDL_CMD_GET_IMAGE_PREF_REQ,     /* Get image preference request */
    QDL_CMD_GET_IMAGE_PREF_RSP,     /* Get image preference response */
};

enum qdl_error {
    QDL_ERROR_01 = 1,            // 01 Reserved
    QDL_ERROR_BAD_ADDR,          // 02 Invalid destination address
    QDL_ERROR_BAD_LEN,           // 03 Invalid length
    QDL_ERROR_BAD_PACKET,        // 04 Unexpected end of packet
    QDL_ERROR_BAD_CMD,           // 05 Invalid command
    QDL_ERROR_06,                // 06 Reserved
    QDL_ERROR_OP_FAILED,         // 07 Operation failed
    QDL_ERROR_BAD_FLASH_ID,      // 08 Invalid flash intelligent ID
    QDL_ERROR_BAD_VOLTAGE,       // 09 Invalid programming voltage
    QDL_ERROR_WRITE_FAILED,      // 10 Write verify failed
    QDL_ERROR_11,                // 11 Reserved
    QDL_ERROR_BAD_SPC,           // 12 Invalid security code
    QDL_ERROR_POWERDOWN,         // 13 Power-down failed
    QDL_ERROR_UNSUPPORTED,       // 14 NAND flash programming not supported
    QDL_ERROR_CMD_SEQ,           // 15 Command out of sequence
    QDL_ERROR_CLOSE,             // 16 Close failed
    QDL_ERROR_BAD_FEATURES,      // 17 Invalid feature bits
    QDL_ERROR_SPACE,             // 18 Out of space
    QDL_ERROR_BAD_SECURITY,      // 19 Invalid security mode
    QDL_ERROR_MULTI_UNSUPPORTED, // 20 Multi-image NAND not supported
    QDL_ERROR_POWEROFF,          // 21 Power-off command not supported
    QDL_ERROR_CMD_UNSUPPORTED,   // 22 Command not supported
    QDL_ERROR_BAD_CRC,           // 23 Invalid CRC
    QDL_ERROR_STATE,             // 24 Command received in invalid state
    QDL_ERROR_TIMEOUT,           // 25 Receive timeout
    QDL_ERROR_IMAGE_AUTH,        // 26 Image authentication error
};

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

enum qdl_type {
    QDL_IMAGE_AMSS_MODEM        = 0x05,     // 05 AMSS modem image
    QDL_IMAGE_AMSS_APPLICATION,         // 06 AMSS application image
    QDL_IMAGE_AMSS_UQCN     = 0x0d,     // 13 Provisioning information
    QDL_IMAGE_DBL           = 0x0f,     // 15 DBL image
    QDL_IMAGE_OSBL,                 // 16 OSBL image
    QDL_IMAGE_CWE           = 0x80,     // 128 CWE image
};

static const char *qdl_type2str(__u8 type)
{
    switch (type) {
    case QDL_IMAGE_AMSS_MODEM:
        return "AMSS_MODEM";
    case QDL_IMAGE_AMSS_APPLICATION:
        return "AMSS_APPLICATION";
    case QDL_IMAGE_AMSS_UQCN:
        return "AMSS_UQCN";
    case QDL_IMAGE_DBL:
        return "DBL";
    case QDL_IMAGE_OSBL:
        return "OSBL";
    case QDL_IMAGE_CWE:
        return "CWE";
    default:
        return "UNKNOWN";
    }
}

/* feature bits */
#define QDL_FEATURE_GENERIC_UNFRAMED    0x10
#define QDL_FEATURE_QDL_UNFRAMED    0x20
#define QDL_FEATURE_BAR_MODE        0x40

struct qdl_hello_req {
    __u8 cmd; /* 0x01 */
    char magic[32];
    __u8 maxver;
    __u8 minver;
    __u8 features;
}  __attribute__ ((packed));

static struct qdl_hello_req qdl_hello_req = {
    .cmd = QDL_CMD_HELLO_REQ,
    .magic = { 'Q', 'C', 'O', 'M', ' ', 'h', 'i', 'g', 'h', ' ', 's', 'p', 'e', 'e', 'd', ' ', 'p', 'r', 'o', 't', 'o', 'c', 'o', 'l', ' ', 'h', 's', 't', },
    .maxver = 0,
    .minver = 0,
    .features = QDL_FEATURE_QDL_UNFRAMED | QDL_FEATURE_GENERIC_UNFRAMED,
};

struct qdl_hello_rsp {
    __u8 cmd; /* 0x02 */
    char magic[32];
    __u8 maxver;
    __u8 minver;
    __u32 reserved1;
    __u32 reserved2;
    __u8 reserved3;
    __u16 reserved4;
    __u16 reserved5;
    __u8 features;
}  __attribute__ ((packed));

struct qdl_err_rsp {
    __u8 cmd; /* 0x0d */
    __u32 error;
    __u8 errortxt;
}  __attribute__ ((packed));

struct qdl_ufopen_req {
    __u8 cmd; /* 0x25 */
    __u8 type;
    __u32 length;
    __u8 windowsize;
    __u32 chunksize;
    __u16 reserved;
    /* The first 400 bytes of the image is appended to the "open
     * unframed" request on Sierra Wireless modems.  This chunk is
     * not included here as it is not a part of the request.
     *
     * The file header inclusion here seems to depend on the file
     * type
     */
}  __attribute__ ((packed));

struct qdl_ufopen_rsp {
    __u8 cmd; /* 0x26 */
    __u16 status;
    __u8 windowsize;
    __u32 chunksize;
}  __attribute__ ((packed));

/* This request is not HDLC framed, so this "header" includes the crc */
struct qdl_ufwrite_req {
    __u8 cmd; /* 0x27 */
    __u16 sequence;
    __u32 reserved;
    __u32 chunksize;
    __u16 crc;
}  __attribute__ ((packed));

/* the buffer most hold a file chunk + this header */
#define BUFSIZE (CHUNK + sizeof(struct qdl_ufwrite_req))

/* The response is HDLC framed, so the crc is part of the framing */
struct qdl_ufwrite_rsp {
    __u8 cmd; /* 0x28 */
    __u16 sequence;
    __u32 reserved;
    __u16 status;
}  __attribute__ ((packed));

/* __u8 0x29 - cmd only */

struct qdl_ufclose_rsp {
    __u8 cmd; /* 0x2a */
    __u16 status;
    __u8 type;
    __u8 errortxt;
}  __attribute__ ((packed));

/* __u8 0x2d - cmd only */
/* __u8 0x2e - cmd only */

struct qdl_imagepref_rsp {
    __u8 cmd; /* 0x2f */
    __u8 entries;
    struct {
        __u8 type;
        char id[16];
    } image[];
}  __attribute__ ((packed));

/*
 * crc16 and HDLC escape code borrowed from modemmanager/libqcdm
 * Copyright (C) 2010 Red Hat, Inc.
 */

/* Table of CRCs for each possible byte, with a generator polynomial of 0x8408 */
static const __u16 crc_table[256] = {
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
static __u16 crc16(const char *buffer, size_t len)
{
    __u16 crc = 0xffff;

    while (len--)
            crc = crc_table[(crc ^ *buffer++) & 0xff] ^ (crc >> 8);
    return ~crc;
}

#define CONTROL 0x7e
#define ESCAPE  0x7d
#define MASK    0x20

static size_t escape(const char *in, size_t inlen, char *out, size_t outlen)
{
    size_t i, j;

    for (i = 0, j = 0; i < inlen; i++) {
        if (in[i] == CONTROL || in[i] == ESCAPE) {
            out[j++] = ESCAPE;
            if (j < outlen)
                out[j++] = in[i] ^ MASK;
        } else {
            out[j++] = in[i];
        }
        if (j >= outlen)
            return 0; /* failed to enscape -  buffer too small */
    }
    return j;
}

static size_t unescape(const char *in, size_t inlen, char *out, size_t outlen)
{
    size_t i, j = 0;
    bool escaping = false;

    for (i = 0; i < inlen; i++) {
        if (j >= outlen) {
            debug("i=%zu, j=%zu, inlen=%zu, outlen=%zu\n", i, j, inlen, outlen);
            return 0;
        }
        if (escaping) {
            out[j++] = in[i] ^ MASK;
            escaping = false;
        } else if (in[i] == ESCAPE) {
            escaping = true;
        } else {
            out[j++] = in[i];
        }
    }
    return j;
}

/* copy a possibly escaped single byte to out */
static size_t escape_byte(__u8 byte, char *out, size_t outlen)
{
    size_t j = 0;

    if (byte == CONTROL || byte == ESCAPE) {
        out[j++] = ESCAPE;
        byte ^= MASK;
    }
    out[j++] = byte;
    return j;
}

static size_t hdlc_frame(const char *in, size_t inlen, char *out, size_t outlen)
{
    __u16 crc;
    size_t j = 0;

    out[j++] = CONTROL;
    j += escape(in, inlen, &out[j], outlen - j);
    crc = crc16(in, inlen);
    j += escape_byte(crc & 0xff, &out[j], outlen - j);
    j += escape_byte(crc >> 8 & 0xff, &out[j], outlen - j);
    out[j++] = CONTROL;

    if (j < inlen)
        return 0;    /* FIXME */

    return j;
}

static size_t hdlc_unframe(const char *in, size_t inlen, char *out, size_t outlen)
{
    __u16 crc;
    size_t j, i = inlen;

    /* the first control char is optional */
    if (*in == CONTROL) {
        in++;
        i--;
    }
    if (in[i - 1] == CONTROL)
        i--;

    j = unescape(in, i, out, outlen);
    if (j < 2) {
        debug("unescape failed: j = %zu\n", j);
        return 0;
    }
    j -= 2; /* remove the crc */

    /* verify the crc */
    crc = crc16(out, j);
    if (crc != ((unsigned char)out[j] | (unsigned char)out[j + 1] << 8)) {
        debug("crc failed: 0x%04x != 0x%04x\n", crc, out[j] | out[j + 1] << 8);
        return 0;
    }
    return j;
}

/* should the unframed open request include a file header? */
static inline size_t hdrlen(__u8 type)
{
    switch (type) {
    case QDL_IMAGE_CWE:
        return 400;
    default:
        return 0;
    }
}

/* some image types contain trailing garbage - from gobi-loader */
static inline size_t imglen(__u8 type, size_t len)
{
    switch (type) {
    case QDL_IMAGE_AMSS_MODEM:
        return len - 8;
    default:
        return len;
    }
}

static size_t create_ufopen_req(char *in, size_t filelen, __u8 type)
{
    struct qdl_ufopen_req *req = (void *)in;

    req->cmd = QDL_CMD_OPEN_UNFRAMED_REQ;
    req->type = type;
    req ->windowsize = 1; /* snooped */
    req->length = imglen(type, filelen);
    req->chunksize = imglen(type, filelen) - hdrlen(type);
    return sizeof(struct qdl_ufopen_req);
}

static size_t create_ufwrite_req(char *out, size_t chunksize, int sequence)
{
    struct qdl_ufwrite_req *req = (void *)out;

    req->cmd = QDL_CMD_WRITE_UNFRAMED_REQ;
    req->sequence = sequence;
    req->reserved = 0;
    req->chunksize = chunksize;
    req->crc = crc16(out, sizeof(*req) - 2);
    return sizeof(*req);
}

static int parse_sdp_hello(const char *in, size_t inlen)
{
    struct qdl_hello_rsp *r;
    char buf[sizeof(*r) + sizeof(__u16)];
    char magicbuf[33] = {0};
    size_t ret;

    r = (struct qdl_hello_rsp *)buf;
    ret = hdlc_unframe(in, inlen, buf, sizeof(buf));
    if (ret == sizeof(*r) && r->cmd == QDL_CMD_HELLO_RSP) {
        memcpy(magicbuf, r->magic, r->maxver <= 5 ? 24 : 32);
        debug("magic: '%s'\nmaxver: %u\nminver: %u\nfeatures: 0x%02x\n", magicbuf, r->maxver, r->minver, r->features);
        return 0;
    }
    return -1; /* unexpected error */
}

static const char *sdperr2str(__u32 err)
{
    switch (err) {
    case QDL_ERROR_01:
        return "Reserved";
    case QDL_ERROR_BAD_ADDR:
        return "Invalid destination address";
    case QDL_ERROR_BAD_LEN:
        return "Invalid length";
    case QDL_ERROR_BAD_PACKET:
        return "Unexpected end of packet";
    case QDL_ERROR_BAD_CMD:
        return "Invalid command";
    case QDL_ERROR_06:
        return "Reserved";
    case QDL_ERROR_OP_FAILED:
        return "Operation failed";
    case QDL_ERROR_BAD_FLASH_ID:
        return "Invalid flash intelligent ID";
    case QDL_ERROR_BAD_VOLTAGE:
        return "Invalid programming voltage";
    case QDL_ERROR_WRITE_FAILED:
        return "Write verify failed";
    case QDL_ERROR_11:
        return "Reserved";
    case QDL_ERROR_BAD_SPC:
        return "Invalid security code";
    case QDL_ERROR_POWERDOWN:
        return "Power-down failed";
    case QDL_ERROR_UNSUPPORTED:
        return "NAND flash programming not supported";
    case QDL_ERROR_CMD_SEQ:
        return "Command out of sequence";
    case QDL_ERROR_CLOSE:
        return "Close failed";
    case QDL_ERROR_BAD_FEATURES:
        return "Invalid feature bits";
    case QDL_ERROR_SPACE:
        return "Out of space";
    case QDL_ERROR_BAD_SECURITY:
        return "Invalid security mode";
    case QDL_ERROR_MULTI_UNSUPPORTED:
        return "Multi-image NAND not supported";
    case QDL_ERROR_POWEROFF:
        return "Power-off command not supported";
    case QDL_ERROR_CMD_UNSUPPORTED:
        return "Command not supported";
    case QDL_ERROR_BAD_CRC:
        return "Invalid CRC";
    case QDL_ERROR_STATE:
        return "Command received in invalid state";
    case QDL_ERROR_TIMEOUT:
        return "Receive timeout";
    case QDL_ERROR_IMAGE_AUTH:
        return "Image authentication error";
    default:
        return "Unknown error";
    }
}

static int parse_sdp_err(const char *in, size_t inlen, bool silent)
{
    struct qdl_err_rsp *r;
    char buf[sizeof(*r) + sizeof(__u16)];
    size_t ret;

    r = (struct qdl_err_rsp *)buf;
    ret = hdlc_unframe(in, inlen, buf, sizeof(buf));
    if (ret == sizeof(*r) && r->cmd == QDL_CMD_ERROR) {
        if (!silent)
            fprintf(stderr, "SDP error %d (%d): %s\n", r->error, r->errortxt, sdperr2str(r->error));
        return -r->error;
    }
    return -1; /* not an proper error frame */
}

static int parse_ufopen(const char *in, size_t inlen)
{
    struct qdl_ufopen_rsp *r;
    char buf[sizeof(*r) + sizeof(__u16)];
    size_t ret;

    r = (struct qdl_ufopen_rsp *)buf;
    ret = hdlc_unframe(in, inlen, buf, sizeof(buf));
    if (ret != sizeof(*r) || r->cmd != QDL_CMD_OPEN_UNFRAMED_RSP)
        return -1;
    debug("status=%d, windowsize=%d, chunksize=%d\n", r->status, r->windowsize, r->chunksize);
    return -r->status;
}

static int parse_ufwrite(const char *in, size_t inlen)
{
    struct qdl_ufwrite_rsp *r;
    char buf[sizeof(*r) + sizeof(__u16)];
    size_t ret;

    r = (struct qdl_ufwrite_rsp *)buf;
    ret = hdlc_unframe(in, inlen, buf, sizeof(buf));
    if (ret != sizeof(*r) || r->cmd != QDL_CMD_WRITE_UNFRAMED_RSP)
        return -1;
    if (r->status) {
        fprintf(stderr, "seq 0x%04x status=%d\n", r->sequence, r->status);
        return -r->status;
    }
    debug("ack: %d\n", r->sequence);
    return r->sequence;
}

static int parse_ufdone(const char *in, size_t inlen)
{
    struct qdl_ufclose_rsp *r;
    char buf[sizeof(*r) + sizeof(__u16)];
    size_t ret;

    r = (struct qdl_ufclose_rsp *)buf;
    ret = hdlc_unframe(in, inlen, buf, sizeof(buf));
    if (ret != sizeof(*r) || r->cmd != QDL_CMD_SESSION_DONE_RSP)
        return -1;
    debug("UF close: status=%d, type=%d, errortxt=%d\n", r->status, r->type, r->errortxt);
    return -r->status;
}

/* read and parse QDL if available.  Will return unless data is
 * available withing 5 ms
 */
static int read_and_parse(int fd, bool silent)
{
    char *end, *p, rbuf[512];
    int rlen, framelen, ret = 0;
    fd_set rd;
    struct timeval tv = { .tv_sec = 1, .tv_usec = 0, };

    FD_ZERO(&rd);
    FD_SET(fd, &rd);
    if (select(fd + 1, &rd, NULL, NULL, &tv) <= 0) {
        debug("timeout: no data read\n");
        return 0;
    }

    rlen = read(fd, rbuf, sizeof(rbuf));
    if (rlen <= 0)
        return rlen;
    print_packet("read", rbuf, rlen);

    p = rbuf;
    while (rlen > 0 && (end = memchr(p + 1, CONTROL, rlen - 1)) != NULL) {
        end++;
        framelen = end - p;
        switch (p[1]) {
        case QDL_CMD_ERROR:
            ret = parse_sdp_err(p, framelen, silent);
            break;
        case QDL_CMD_HELLO_RSP: /* == DLOAD_ACK */
            if (framelen != 5)
                ret = parse_sdp_hello(p, framelen);
            else
                debug("Got DLOAD_ACK\n");
            break;
        case QDL_CMD_OPEN_UNFRAMED_RSP:
            ret = parse_ufopen(p, framelen);
            break;
        case QDL_CMD_WRITE_UNFRAMED_RSP:
            ret = parse_ufwrite(p, framelen);
            break;
        case QDL_CMD_SESSION_DONE_RSP:
            ret = parse_ufdone(p, framelen);
            break;
        default:
            fprintf(stderr, "Unsupported response code: 0x%02x\n", p[1]);
        }
        if (ret < 0)
            return ret;
        p = end;
        rlen -= framelen;
    }
    return ret;
}

static int write_hdlc(int fd, const char *in, size_t inlen)
{
    char wbuf[512];
    int wlen;

    wlen = hdlc_frame(in, inlen, wbuf, sizeof(wbuf));
    if (wlen > 0) {
        if (write(fd, wbuf, wlen) < 0)
            fprintf(stderr, "error writing HDLC");
        else
            print_packet("write", wbuf, wlen);
    } else {
        debug("hdlc_frame() returned %d\n", wlen);
    }
    return wlen;
}

static int serial_open(const char *dev)
{
    struct termios terminal_data;
    int fd;

    /* FIXME: verify that the serial device is a Sierra Wireless device in QDL mode */
    fd = open(dev,  O_RDWR | O_NOCTTY);
    if (fd > 0) {
        tcgetattr(fd, &terminal_data);
        cfmakeraw(&terminal_data);
        tcsetattr(fd, TCSANOW, &terminal_data);
    }
    debug("opened %s\n", dev);
    return fd;
}

/* Sierra Wireless CWE file header
 *   Note: 32bit numbers are big endian
 */
struct cwehdr {
    char reserved1[256];
    __u32 crc;      /* 32bit CRC of "reserved1" field */
    __u32 rev;      /* header revision */
    __u32 val;      /* CRC validity indicator */
    char type[4];       /* ASCII - not null terminated */
    char product[4];    /* ASCII - not null terminated */
    __u32 imgsize;      /* image size */
    __u32 imgcrc;       /* 32bit CRC of the image */
    char version[84];   /* ASCII - null terminated */
    char date[8];       /* ASCII - null terminated */
    __u32 compat;       /* backward compatibility */
    char reserved2[20];
};

static int verify_cwehdr(char *buf)
{
    struct cwehdr *h = (void *)buf;
    char tmp[5];

    fprintf(stderr, "  CWE revision: %d\n", be32toh(h->rev));
    memcpy(tmp, h->type, 4);
    tmp[4] = 0;
    fprintf(stderr, "  type: %s\n", tmp);
    memcpy(tmp, h->product, 4);
    tmp[4] = 0;
    fprintf(stderr, "  product: %s\n", tmp);
    fprintf(stderr, "  image size: %d\n", be32toh(h->imgsize));
    fprintf(stderr, "  version: %s\n", h->version);
    fprintf(stderr, "  date: %s\n", h->date);
    return 0;
}

/* guessing image type based on the well known Gobi 1k and 2k
 * filenames, and assumes anything else is a CWE image
 *
 * This is based on the types in gobi-loader's snooped magic strings:
 *   0x05 => "amss.mbn"
 *   0x06 => "apps.mbn"
 *   0x0d => "uqcn.mbn" (Gobi 2000 only)
 */
static __u8 filename2type(const char *filename)
{
    char *base = strrchr(filename, '/');

    if (!base)
        base = (char *)filename;
    if (!strcasecmp(base, "amss.mbn"))
        return QDL_IMAGE_AMSS_MODEM;
    if (!strcasecmp(base, "apps.mbn"))
        return QDL_IMAGE_AMSS_APPLICATION;
    if (!strcasecmp(base, "uqcn.mbn"))
        return QDL_IMAGE_AMSS_UQCN;
    return QDL_IMAGE_CWE;
}

static int download_image(int serfd, char *buf, const char *image)
{
    int imgfd = -1, ret = 0, seq = 0;
    size_t chunksize, rlen, filelen;
    struct stat img_data;
    __u8 type = filename2type(image);

    /* FIXME: verify that this image matches the modem */
    imgfd = open(image, O_RDONLY);
    if (imgfd < 0) {
        fprintf(stderr, "Cannot open %s: %d\n", image, imgfd);
        ret = imgfd;
        goto out;
    }
    fstat(imgfd, &img_data);
    if (imglen(type, img_data.st_size) < hdrlen(type)) {
        fprintf(stderr, "%s is too short\n", image);
        ret = -1;
        goto out;
    }

    fprintf(stderr, "Downloading %s image '%s'\n", qdl_type2str(type), image);

    /* send open request */
    rlen = create_ufopen_req(buf, img_data.st_size, type);
    if (hdrlen(type) > 0) {
        ret = -1;
        if (hdrlen(type) + rlen > BUFSIZE)
            goto out;
        if (read(imgfd, buf + rlen, hdrlen(type)) != hdrlen(type))
            goto out;
        if (type == QDL_IMAGE_CWE)
            verify_cwehdr(buf + rlen);
    }
    write_hdlc(serfd, buf, rlen + hdrlen(type));

    /* read ufopen response - FIXME: act on errors! */
    if (read_and_parse(serfd, false) < 0)
        goto out;

    filelen = imglen(type, img_data.st_size) - hdrlen(type);
    /* remaining data to send */
    while (filelen > 0) {
        fd_set wr;
        struct timeval tv = { .tv_sec = 2, .tv_usec = 0, };

        chunksize = CHUNK;
        if (chunksize > filelen)
            chunksize = filelen;

        debug("write #%d (%zu)...", seq, chunksize);
        rlen = create_ufwrite_req(buf, chunksize, seq++);
        rlen += read(imgfd, buf + rlen, CHUNK);
        filelen -= chunksize;

        FD_ZERO(&wr);
        FD_SET(serfd, &wr);
        if (select(serfd + 1, NULL, &wr, NULL, &tv) <= 0)
            goto out;
        if (write(serfd,  buf, rlen) < 0) {
            fprintf(stderr, "error writing data");
            goto out;
        }
        ret = read_and_parse(serfd, false);
        if (ret < 0)
            goto out;
    }

    debug("finished writing\n");

    /* This may take a considerable amount of time */
    fprintf(stderr, "\nWaiting for ack");
    while (ret != seq - 1) {
        fprintf(stderr, ".");
        sleep(3);
        ret = read_and_parse(serfd, false);
    }
    fprintf(stderr, "\n");

out:
    if (imgfd > 0)
        close(imgfd);
    return ret;
}

static struct option main_options[] = {
    { "help",   0, 0, 'h' },
    { "serial",     1, 0, 's' },
    { "debug",      0, 0, 'd' },
    { 0, 0, 0, 0 }
};

static void usage(const char *prog)
{
    fprintf(stderr,
        "\n%s: %s [ --help ]"
#ifdef DEBUG
        " [--debug] "
#endif
        "--serial <device> <image> [image2] [image3]\n",
        __func__, prog);
}

int main(int argc, char *argv[])
{
    int opt, serfd = -1, ret = 0, version;
    char *buffer = NULL;

    fprintf(stderr, "%s\n", DESCRIPTION);
    while ((opt = getopt_long(argc, argv, "i:s:m:dvh", main_options, NULL)) != -1) {
        switch(opt) {
        case 's':
            serfd = serial_open(optarg);
            break;
#ifdef DEBUG
        case 'd':
            debug = true;
            break;
#endif
        case 'h':
            usage(argv[0]);
            return 0;
        }
    }

    buffer = malloc(BUFSIZE);
    if (!buffer) {
        ret = -ENOMEM;
        goto err;
    }


/* FIXME: should do the following for a complete image upload*
 *
 *  For CWE images:
 *    - verify image sanity and retrive model/version data for next step if applicable
 *    - if application mode:
 *       + verify qmi model/version matching image
 *       + check usb sysfs (serial++) to match against bootloader mode
 *       + switch to bootloader mode
 *       + wait for QDL device to appear
 *
 * For all images:
 *    - verify that the QDL device is the correct one (check serial etc)
 *
 * NON WORKING:
 *    - allow multiple images to be uploaded in one run
 * How do we do this properly?
 */

    /* need at least one firmware filename */
    if (optind == argc) {
        usage(argv[0]);
        return -1;
    }

    if (serfd < 0)
        return serfd;

    /* switch to SDP - this is required for some modems like MC7710 */
    write_hdlc(serfd, (const char *)&dload_sdp, sizeof(dload_sdp));
    ret = read_and_parse(serfd, true);

    /* the modem could already be in SDP mode, so ignore "unsupported" errors */
    if (ret < 0 && ret != -QDL_ERROR_CMD_UNSUPPORTED)
        goto err;

    /* attempt to probe supported protocol version
     *  Newer modems like Sierra Wireless MC7710 must use '6' for both fields
     *  Gobi2000 modems like HP un2420 must use '5' for both fields
     *  Gobi1000 modems  must use '4' for both fields
     */
    for (version = 4; version < 7; version++) {
        qdl_hello_req.minver = version;
        qdl_hello_req.maxver = version;
        write_hdlc(serfd, (const char *)&qdl_hello_req, sizeof(qdl_hello_req));
        ret = read_and_parse(serfd, true);
        if (!ret)
            break;
    }
    if (ret < 0) {
        fprintf(stderr, "Unable to detect QDL version\n");
        goto err;
    }
    fprintf(stderr, "Got QDL version: %u\n", version);

    /* download all images */
    while (optind < argc && ret >= 0)
        ret = download_image(serfd, buffer, argv[optind++]);

    /* close unframed session */
    buffer[0] = QDL_CMD_SESSION_DONE_REQ;
    write_hdlc(serfd, buffer, 1);

    /* read close response  */
    if (!read_and_parse(serfd, false))
        fprintf(stderr, "Success!\n");

    /* terminate SDP session */
    fprintf(stderr, "Terminating session - rebooting modem...\n");
    buffer[0] = QDL_CMD_SESSION_CLOSE_REQ;
    write_hdlc(serfd, buffer, 1);

    /* no response? */
    read_and_parse(serfd, false);

err:
    if (serfd > 0)
        close(serfd);
    free(buffer);
    return ret;
}
