/* sha1.c
 
 Copyright (c) 2022, Nikolaj Schlej. All rights reserved.
 This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 */

//
// This implementations are based on LibTomCrypt that was released into
// public domain by Tom St Denis.
//
#include "sha1.h"
#include <stdint.h>
#include <string.h>

/* ulong64: 64-bit data type */
#ifdef _MSC_VER
   #define CONST64(n) n ## ui64
   typedef unsigned __int64 ulong64;
#else
   #define CONST64(n) n ## ULL
   typedef uint64_t ulong64;
#endif

typedef uint32_t ulong32;

#define LOAD32H(x, y)                      \
  do { x = ((ulong32)((y)[0] & 255)<<24) | \
           ((ulong32)((y)[1] & 255)<<16) | \
           ((ulong32)((y)[2] & 255)<<8)  | \
           ((ulong32)((y)[3] & 255)); } while(0)

#ifndef MIN
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#endif

#define ROL(x, y) ( (((ulong32)(x)<<(ulong32)((y)&31)) | (((ulong32)(x)&0xFFFFFFFFUL)>>(ulong32)((32-((y)&31))&31))) & 0xFFFFFFFFUL)
#define ROLc(x, y) ( (((ulong32)(x)<<(ulong32)((y)&31)) | (((ulong32)(x)&0xFFFFFFFFUL)>>(ulong32)((32-((y)&31))&31))) & 0xFFFFFFFFUL)

#define STORE32H(x, y)                                                                     \
  do { (y)[0] = (unsigned char)(((x)>>24)&255); (y)[1] = (unsigned char)(((x)>>16)&255);   \
       (y)[2] = (unsigned char)(((x)>>8)&255); (y)[3] = (unsigned char)((x)&255); } while(0)

#define STORE64H(x, y)                                                                     \
do { (y)[0] = (unsigned char)(((x)>>56)&255); (y)[1] = (unsigned char)(((x)>>48)&255);     \
     (y)[2] = (unsigned char)(((x)>>40)&255); (y)[3] = (unsigned char)(((x)>>32)&255);     \
     (y)[4] = (unsigned char)(((x)>>24)&255); (y)[5] = (unsigned char)(((x)>>16)&255);     \
     (y)[6] = (unsigned char)(((x)>>8)&255); (y)[7] = (unsigned char)((x)&255); } while(0)

#define F0(x,y,z)  (z ^ (x & (y ^ z)))
#define F1(x,y,z)  (x ^ y ^ z)
#define F2(x,y,z)  ((x & y) | (z & (x | y)))
#define F3(x,y,z)  (x ^ y ^ z)

struct sha1_state {
    ulong64 length;
    ulong32 state[5], curlen;
    unsigned char buf[64];
};

static int s_sha1_compress(struct sha1_state *md, const unsigned char *buf)
{
    ulong32 a,b,c,d,e,W[80],i;
    ulong32 t;

    /* copy the state into 512-bits into W[0..15] */
    for (i = 0; i < 16; i++) {
        LOAD32H(W[i], buf + (4*i));
    }

    /* copy state */
    a = md->state[0];
    b = md->state[1];
    c = md->state[2];
    d = md->state[3];
    e = md->state[4];

    /* expand it */
    for (i = 16; i < 80; i++) {
        W[i] = ROL(W[i-3] ^ W[i-8] ^ W[i-14] ^ W[i-16], 1);
    }

    /* compress */
    /* round one */
    #define FF0(a,b,c,d,e,i) e = (ROLc(a, 5) + F0(b,c,d) + e + W[i] + 0x5a827999UL); b = ROLc(b, 30);
    #define FF1(a,b,c,d,e,i) e = (ROLc(a, 5) + F1(b,c,d) + e + W[i] + 0x6ed9eba1UL); b = ROLc(b, 30);
    #define FF2(a,b,c,d,e,i) e = (ROLc(a, 5) + F2(b,c,d) + e + W[i] + 0x8f1bbcdcUL); b = ROLc(b, 30);
    #define FF3(a,b,c,d,e,i) e = (ROLc(a, 5) + F3(b,c,d) + e + W[i] + 0xca62c1d6UL); b = ROLc(b, 30);

    for (i = 0; i < 20; ) {
       FF0(a,b,c,d,e,i++); t = e; e = d; d = c; c = b; b = a; a = t;
    }
    for (; i < 40; ) {
       FF1(a,b,c,d,e,i++); t = e; e = d; d = c; c = b; b = a; a = t;
    }
    for (; i < 60; ) {
       FF2(a,b,c,d,e,i++); t = e; e = d; d = c; c = b; b = a; a = t;
    }
    for (; i < 80; ) {
       FF3(a,b,c,d,e,i++); t = e; e = d; d = c; c = b; b = a; a = t;
    }

    #undef FF0
    #undef FF1
    #undef FF2
    #undef FF3

    /* store */
    md->state[0] = md->state[0] + a;
    md->state[1] = md->state[1] + b;
    md->state[2] = md->state[2] + c;
    md->state[3] = md->state[3] + d;
    md->state[4] = md->state[4] + e;

    return 0;
}

static int sha1_init(struct sha1_state * md)
{
   if (md == NULL) return -1;
   md->state[0] = 0x67452301UL;
   md->state[1] = 0xefcdab89UL;
   md->state[2] = 0x98badcfeUL;
   md->state[3] = 0x10325476UL;
   md->state[4] = 0xc3d2e1f0UL;
   md->curlen = 0;
   md->length = 0;
   return 0;
}

static int sha1_process(struct sha1_state * md, const unsigned char *in, unsigned long inlen)
{
    unsigned long n;
    int err;
    if (md == NULL) return -1;
    if (in == NULL) return -1;
    if (md->curlen > sizeof(md->buf)) {
        return -1;
    }
    if (((md->length + inlen * 8) < md->length)
        || ((inlen * 8) < inlen)) {
        return -1;
    }
    while (inlen > 0) {
        if (md->curlen == 0 && inlen >= 64) {
            if ((err = s_sha1_compress(md, in)) != 0) {
                return err;
            }
            md->length += 64 * 8;
            in         += 64;
            inlen      -= 64;
        } else {
            n = MIN(inlen, (64 - md->curlen));
            memcpy(md->buf + md->curlen, in, (size_t)n);
            md->curlen += n;
            in         += n;
            inlen      -= n;
            if (md->curlen == 64) {
                if ((err = s_sha1_compress(md, md->buf)) != 0) {
                    return err;
                }
                md->length += 8 * 64;
                md->curlen = 0;
            }
        }
    }
    return 0;
}

static int sha1_done(struct sha1_state * md, unsigned char *out)
{
    int i;

    if (md == NULL) return -1;
    if (out == NULL) return -1;

    if (md->curlen >= sizeof(md->buf)) {
       return -1;
    }

    /* increase the length of the message */
    md->length += md->curlen * 8;

    /* append the '1' bit */
    md->buf[md->curlen++] = (unsigned char)0x80;

    /* if the length is currently above 56 bytes we append zeros
     * then compress.  Then we can fall back to padding zeros and length
     * encoding like normal.
     */
    if (md->curlen > 56) {
        while (md->curlen < 64) {
            md->buf[md->curlen++] = (unsigned char)0;
        }
        s_sha1_compress(md, md->buf);
        md->curlen = 0;
    }

    /* pad upto 56 bytes of zeroes */
    while (md->curlen < 56) {
        md->buf[md->curlen++] = (unsigned char)0;
    }

    /* store length */
    STORE64H(md->length, md->buf+56);
    s_sha1_compress(md, md->buf);

    /* copy output */
    for (i = 0; i < 5; i++) {
        STORE32H(md->state[i], out+(4*i));
    }

    return 0;
}

void sha1(const void *in, unsigned long inlen, void* out)
{
    struct sha1_state ctx;
    sha1_init(&ctx);
    sha1_process(&ctx, (const unsigned char*)in, inlen);
    sha1_done(&ctx, (unsigned char *)out);
}


