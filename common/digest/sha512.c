/* sha512.c
 
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

#include "sha2.h"
#include <stdint.h>
#include <string.h>

/* ulong64: 64-bit data type */
#ifdef _MSC_VER
   #define CONST64(n) n ## ui64
   typedef unsigned __int64 ulong64;
   typedef __int64 long64;
#else
   #define CONST64(n) n ## ULL
   typedef unsigned long long ulong64;
   typedef long long long64;
#endif

#define ROR64c(x, y) \
    ( ((((x)&CONST64(0xFFFFFFFFFFFFFFFF))>>((ulong64)(y)&CONST64(63))) | \
      ((x)<<(((ulong64)64-((y)&63))&63))) & CONST64(0xFFFFFFFFFFFFFFFF))

#define LOAD64H(x, y)                                                      \
do { x = (((ulong64)((y)[0] & 255))<<56)|(((ulong64)((y)[1] & 255))<<48) | \
         (((ulong64)((y)[2] & 255))<<40)|(((ulong64)((y)[3] & 255))<<32) | \
         (((ulong64)((y)[4] & 255))<<24)|(((ulong64)((y)[5] & 255))<<16) | \
         (((ulong64)((y)[6] & 255))<<8)|(((ulong64)((y)[7] & 255))); } while(0)

#define STORE64H(x, y)                                                                     \
do { (y)[0] = (unsigned char)(((x)>>56)&255); (y)[1] = (unsigned char)(((x)>>48)&255);     \
     (y)[2] = (unsigned char)(((x)>>40)&255); (y)[3] = (unsigned char)(((x)>>32)&255);     \
     (y)[4] = (unsigned char)(((x)>>24)&255); (y)[5] = (unsigned char)(((x)>>16)&255);     \
     (y)[6] = (unsigned char)(((x)>>8)&255); (y)[7] = (unsigned char)((x)&255); } while(0)

#ifndef MIN
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#endif

/* the K array */
static const ulong64 K[80] = {
CONST64(0x428a2f98d728ae22), CONST64(0x7137449123ef65cd),
CONST64(0xb5c0fbcfec4d3b2f), CONST64(0xe9b5dba58189dbbc),
CONST64(0x3956c25bf348b538), CONST64(0x59f111f1b605d019),
CONST64(0x923f82a4af194f9b), CONST64(0xab1c5ed5da6d8118),
CONST64(0xd807aa98a3030242), CONST64(0x12835b0145706fbe),
CONST64(0x243185be4ee4b28c), CONST64(0x550c7dc3d5ffb4e2),
CONST64(0x72be5d74f27b896f), CONST64(0x80deb1fe3b1696b1),
CONST64(0x9bdc06a725c71235), CONST64(0xc19bf174cf692694),
CONST64(0xe49b69c19ef14ad2), CONST64(0xefbe4786384f25e3),
CONST64(0x0fc19dc68b8cd5b5), CONST64(0x240ca1cc77ac9c65),
CONST64(0x2de92c6f592b0275), CONST64(0x4a7484aa6ea6e483),
CONST64(0x5cb0a9dcbd41fbd4), CONST64(0x76f988da831153b5),
CONST64(0x983e5152ee66dfab), CONST64(0xa831c66d2db43210),
CONST64(0xb00327c898fb213f), CONST64(0xbf597fc7beef0ee4),
CONST64(0xc6e00bf33da88fc2), CONST64(0xd5a79147930aa725),
CONST64(0x06ca6351e003826f), CONST64(0x142929670a0e6e70),
CONST64(0x27b70a8546d22ffc), CONST64(0x2e1b21385c26c926),
CONST64(0x4d2c6dfc5ac42aed), CONST64(0x53380d139d95b3df),
CONST64(0x650a73548baf63de), CONST64(0x766a0abb3c77b2a8),
CONST64(0x81c2c92e47edaee6), CONST64(0x92722c851482353b),
CONST64(0xa2bfe8a14cf10364), CONST64(0xa81a664bbc423001),
CONST64(0xc24b8b70d0f89791), CONST64(0xc76c51a30654be30),
CONST64(0xd192e819d6ef5218), CONST64(0xd69906245565a910),
CONST64(0xf40e35855771202a), CONST64(0x106aa07032bbd1b8),
CONST64(0x19a4c116b8d2d0c8), CONST64(0x1e376c085141ab53),
CONST64(0x2748774cdf8eeb99), CONST64(0x34b0bcb5e19b48a8),
CONST64(0x391c0cb3c5c95a63), CONST64(0x4ed8aa4ae3418acb),
CONST64(0x5b9cca4f7763e373), CONST64(0x682e6ff3d6b2b8a3),
CONST64(0x748f82ee5defb2fc), CONST64(0x78a5636f43172f60),
CONST64(0x84c87814a1f0ab72), CONST64(0x8cc702081a6439ec),
CONST64(0x90befffa23631e28), CONST64(0xa4506cebde82bde9),
CONST64(0xbef9a3f7b2c67915), CONST64(0xc67178f2e372532b),
CONST64(0xca273eceea26619c), CONST64(0xd186b8c721c0c207),
CONST64(0xeada7dd6cde0eb1e), CONST64(0xf57d4f7fee6ed178),
CONST64(0x06f067aa72176fba), CONST64(0x0a637dc5a2c898a6),
CONST64(0x113f9804bef90dae), CONST64(0x1b710b35131c471b),
CONST64(0x28db77f523047d84), CONST64(0x32caab7b40c72493),
CONST64(0x3c9ebe0a15c9bebc), CONST64(0x431d67c49c100d4c),
CONST64(0x4cc5d4becb3e42b6), CONST64(0x597f299cfc657e2a),
CONST64(0x5fcb6fab3ad6faec), CONST64(0x6c44198c4a475817)
};

/* Various logical functions */
#define Ch(x,y,z)       (z ^ (x & (y ^ z)))
#define Maj(x,y,z)      (((x | y) & z) | (x & y))
#define S(x, n)         ROR64c(x, n)
#define R(x, n)         (((x)&CONST64(0xFFFFFFFFFFFFFFFF))>>((ulong64)n))
#define Sigma0(x)       (S(x, 28) ^ S(x, 34) ^ S(x, 39))
#define Sigma1(x)       (S(x, 14) ^ S(x, 18) ^ S(x, 41))
#define Gamma0(x)       (S(x, 1) ^ S(x, 8) ^ R(x, 7))
#define Gamma1(x)       (S(x, 19) ^ S(x, 61) ^ R(x, 6))

struct sha512_state {
    ulong64  length, state[8];
    unsigned long curlen;
    unsigned char buf[128];
};

/* compress 1024-bits */
static int s_sha512_compress(struct sha512_state * md, const unsigned char *buf)
{
    ulong64 S[8], W[80], t0, t1;
    int i;

    /* copy state into S */
    for (i = 0; i < 8; i++) {
        S[i] = md->state[i];
    }

    /* copy the state into 1024-bits into W[0..15] */
    for (i = 0; i < 16; i++) {
        LOAD64H(W[i], buf + (8*i));
    }

    /* fill W[16..79] */
    for (i = 16; i < 80; i++) {
        W[i] = Gamma1(W[i - 2]) + W[i - 7] + Gamma0(W[i - 15]) + W[i - 16];
    }

    /* Compress */
#define RND(a,b,c,d,e,f,g,h,i)                         \
     t0 = h + Sigma1(e) + Ch(e, f, g) + K[i] + W[i];   \
     t1 = Sigma0(a) + Maj(a, b, c);                    \
     d += t0;                                          \
     h  = t0 + t1;

    for (i = 0; i < 80; i += 8) {
        RND(S[0],S[1],S[2],S[3],S[4],S[5],S[6],S[7],i+0);
        RND(S[7],S[0],S[1],S[2],S[3],S[4],S[5],S[6],i+1);
        RND(S[6],S[7],S[0],S[1],S[2],S[3],S[4],S[5],i+2);
        RND(S[5],S[6],S[7],S[0],S[1],S[2],S[3],S[4],i+3);
        RND(S[4],S[5],S[6],S[7],S[0],S[1],S[2],S[3],i+4);
        RND(S[3],S[4],S[5],S[6],S[7],S[0],S[1],S[2],i+5);
        RND(S[2],S[3],S[4],S[5],S[6],S[7],S[0],S[1],i+6);
        RND(S[1],S[2],S[3],S[4],S[5],S[6],S[7],S[0],i+7);
    }

    /* feedback */
    for (i = 0; i < 8; i++) {
        md->state[i] = md->state[i] + S[i];
    }

    return 0;
}

static int sha512_init(struct sha512_state * md)
{
    if (md == NULL) return -1;
    md->curlen = 0;
    md->length = 0;
    md->state[0] = CONST64(0x6a09e667f3bcc908);
    md->state[1] = CONST64(0xbb67ae8584caa73b);
    md->state[2] = CONST64(0x3c6ef372fe94f82b);
    md->state[3] = CONST64(0xa54ff53a5f1d36f1);
    md->state[4] = CONST64(0x510e527fade682d1);
    md->state[5] = CONST64(0x9b05688c2b3e6c1f);
    md->state[6] = CONST64(0x1f83d9abfb41bd6b);
    md->state[7] = CONST64(0x5be0cd19137e2179);
    return 0;
}

static int sha512_process(struct sha512_state * md, const unsigned char *in, unsigned long inlen)
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
        if (md->curlen == 0 && inlen >= 128) {
            if ((err = s_sha512_compress(md, in)) != 0) {
                return err;
            }
            md->length += 128 * 8;
            in         += 128;
            inlen      -= 128;
        } else {
            n = MIN(inlen, (128 - md->curlen));
            memcpy(md->buf + md->curlen, in, (size_t)n);
            md->curlen += n;
            in         += n;
            inlen      -= n;
            if (md->curlen == 128) {
                if ((err = s_sha512_compress(md, md->buf)) != 0) {
                    return err;
                }
                md->length += 8 * 128;
                md->curlen = 0;
            }
        }
    }
    return 0;
}

static int sha512_done(struct sha512_state * md, unsigned char *out)
{
    int i;

    if (md == NULL) return -1;
    if (out == NULL) return -1;

    if (md->curlen >= sizeof(md->buf)) {
       return -1;
    }

    /* increase the length of the message */
    md->length += md->curlen * CONST64(8);

    /* append the '1' bit */
    md->buf[md->curlen++] = (unsigned char)0x80;

    /* if the length is currently above 112 bytes we append zeros
     * then compress.  Then we can fall back to padding zeros and length
     * encoding like normal.
     */
    if (md->curlen > 112) {
        while (md->curlen < 128) {
            md->buf[md->curlen++] = (unsigned char)0;
        }
        s_sha512_compress(md, md->buf);
        md->curlen = 0;
    }

    /* pad upto 120 bytes of zeroes
     * note: that from 112 to 120 is the 64 MSB of the length.  We assume that you won't hash
     * > 2^64 bits of data... :-)
     */
    while (md->curlen < 120) {
        md->buf[md->curlen++] = (unsigned char)0;
    }

    /* store length */
    STORE64H(md->length, md->buf+120);
    s_sha512_compress(md, md->buf);

    /* copy output */
    for (i = 0; i < 8; i++) {
        STORE64H(md->state[i], out+(8*i));
    }

    return 0;
}

static int sha384_init(struct sha512_state * md)
{
    if (md == NULL) return -1;

    md->curlen = 0;
    md->length = 0;
    md->state[0] = CONST64(0xcbbb9d5dc1059ed8);
    md->state[1] = CONST64(0x629a292a367cd507);
    md->state[2] = CONST64(0x9159015a3070dd17);
    md->state[3] = CONST64(0x152fecd8f70e5939);
    md->state[4] = CONST64(0x67332667ffc00b31);
    md->state[5] = CONST64(0x8eb44a8768581511);
    md->state[6] = CONST64(0xdb0c2e0d64f98fa7);
    md->state[7] = CONST64(0x47b5481dbefa4fa4);
    return 0;
}

static int sha384_done(struct sha512_state * md, unsigned char *out)
{
    unsigned char buf[64];

    if (md == NULL) return -1;
    if (out == NULL) return -1;;

    if (md->curlen >= sizeof(md->buf)) {
       return -1;
    }

   sha512_done(md, buf);
   memcpy(out, buf, 48);
   return 0;
}

void sha384(const void *in, unsigned long inlen, void* out)
{
    struct sha512_state ctx;
    sha384_init(&ctx);
    sha512_process(&ctx, (const unsigned char*)in, inlen);
    sha384_done(&ctx, (unsigned char *)out);
}

void sha512(const void *in, unsigned long inlen, void* out)
{
    struct sha512_state ctx;
    sha512_init(&ctx);
    sha512_process(&ctx, (const unsigned char*)in, inlen);
    sha512_done(&ctx, (unsigned char *)out);
}
