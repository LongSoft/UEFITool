/* sha256.c

Copyright (c) 2017, LongSoft. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#include "sha256.h"
#include <stdint.h>
#include <string.h>

struct sha256_state {
    uint64_t length;
    uint32_t state[8], curlen;
    uint8_t buf[SHA256_DIGEST_SIZE*2];
};

void sha256_init(struct sha256_state *md);
int sha256_process(struct sha256_state *md, const unsigned char *in, unsigned long inlen);
int sha256_done(struct sha256_state *md, uint8_t *out);

#define GET_BE32(a) ((((uint32_t) (a)[0]) << 24) | (((uint32_t) (a)[1]) << 16) | \
                          (((uint32_t) (a)[2]) << 8) | ((uint32_t) (a)[3]))

#define PUT_BE32(a, val)                                     \
         do {                                                    \
                 (a)[0] = (uint8_t) ((((uint32_t) (val)) >> 24) & 0xff);   \
                 (a)[1] = (uint8_t) ((((uint32_t) (val)) >> 16) & 0xff);   \
                 (a)[2] = (uint8_t) ((((uint32_t) (val)) >> 8) & 0xff);    \
                 (a)[3] = (uint8_t) (((uint32_t) (val)) & 0xff);           \
         } while (0)

#define PUT_BE64(a, val)                             \
         do {                                            \
                 (a)[0] = (uint8_t) (((uint64_t) (val)) >> 56);    \
                 (a)[1] = (uint8_t) (((uint64_t) (val)) >> 48);    \
                 (a)[2] = (uint8_t) (((uint64_t) (val)) >> 40);    \
                 (a)[3] = (uint8_t) (((uint64_t) (val)) >> 32);    \
                 (a)[4] = (uint8_t) (((uint64_t) (val)) >> 24);    \
                 (a)[5] = (uint8_t) (((uint64_t) (val)) >> 16);    \
                 (a)[6] = (uint8_t) (((uint64_t) (val)) >> 8);     \
                 (a)[7] = (uint8_t) (((uint64_t) (val)) & 0xff);   \
         } while (0)

void sha256(const void *in, unsigned long inlen, void* out)
{
    struct sha256_state ctx;
    sha256_init(&ctx);
    sha256_process(&ctx, (const unsigned char*)in, inlen);
    sha256_done(&ctx, (unsigned char *)out);
}

/* This is based on SHA256 implementation in LibTomCrypt that was released into
 * public domain by Tom St Denis. */
/* the K array */
static const unsigned long K[64] = {
    0x428a2f98UL, 0x71374491UL, 0xb5c0fbcfUL, 0xe9b5dba5UL, 0x3956c25bUL,
    0x59f111f1UL, 0x923f82a4UL, 0xab1c5ed5UL, 0xd807aa98UL, 0x12835b01UL,
    0x243185beUL, 0x550c7dc3UL, 0x72be5d74UL, 0x80deb1feUL, 0x9bdc06a7UL,
    0xc19bf174UL, 0xe49b69c1UL, 0xefbe4786UL, 0x0fc19dc6UL, 0x240ca1ccUL,
    0x2de92c6fUL, 0x4a7484aaUL, 0x5cb0a9dcUL, 0x76f988daUL, 0x983e5152UL,
    0xa831c66dUL, 0xb00327c8UL, 0xbf597fc7UL, 0xc6e00bf3UL, 0xd5a79147UL,
    0x06ca6351UL, 0x14292967UL, 0x27b70a85UL, 0x2e1b2138UL, 0x4d2c6dfcUL,
    0x53380d13UL, 0x650a7354UL, 0x766a0abbUL, 0x81c2c92eUL, 0x92722c85UL,
    0xa2bfe8a1UL, 0xa81a664bUL, 0xc24b8b70UL, 0xc76c51a3UL, 0xd192e819UL,
    0xd6990624UL, 0xf40e3585UL, 0x106aa070UL, 0x19a4c116UL, 0x1e376c08UL,
    0x2748774cUL, 0x34b0bcb5UL, 0x391c0cb3UL, 0x4ed8aa4aUL, 0x5b9cca4fUL,
    0x682e6ff3UL, 0x748f82eeUL, 0x78a5636fUL, 0x84c87814UL, 0x8cc70208UL,
    0x90befffaUL, 0xa4506cebUL, 0xbef9a3f7UL, 0xc67178f2UL
};
/* Various logical functions */
#define RORc(x, y) \
( ((((unsigned long) (x) & 0xFFFFFFFFUL) >> (unsigned long) ((y) & 31)) | \
((unsigned long) (x) << (unsigned long) (32 - ((y) & 31)))) & 0xFFFFFFFFUL)
#define Ch(x,y,z)       (z ^ (x & (y ^ z)))
#define Maj(x,y,z)      (((x | y) & z) | (x & y))
#define S(x, n)         RORc((x), (n))
#define R(x, n)         (((x)&0xFFFFFFFFUL)>>(n))
#define Sigma0(x)       (S(x, 2) ^ S(x, 13) ^ S(x, 22))
#define Sigma1(x)       (S(x, 6) ^ S(x, 11) ^ S(x, 25))
#define Gamma0(x)       (S(x, 7) ^ S(x, 18) ^ R(x, 3))
#define Gamma1(x)       (S(x, 17) ^ S(x, 19) ^ R(x, 10))
#ifndef MIN
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#endif
/* compress 512-bits */
static void sha256_compress(struct sha256_state *md, unsigned char *buf)
{
    uint32_t S[8], W[64], t0, t1;
    uint32_t t;
    int i;
    /* copy state into S */
    for (i = 0; i < 8; i++) {
        S[i] = md->state[i];
    }
    /* copy the state into 512-bits into W[0..15] */
    for (i = 0; i < 16; i++)
        W[i] = GET_BE32(buf + (4 * i));
    /* fill W[16..63] */
    for (i = 16; i < 64; i++) {
        W[i] = Gamma1(W[i - 2]) + W[i - 7] + Gamma0(W[i - 15]) +
        W[i - 16];
    }
    /* Compress */
#define RND(a,b,c,d,e,f,g,h,i)                          \
t0 = h + Sigma1(e) + Ch(e, f, g) + K[i] + W[i];	\
t1 = Sigma0(a) + Maj(a, b, c);			\
d += t0;					\
h  = t0 + t1;
    for (i = 0; i < 64; ++i) {
        RND(S[0], S[1], S[2], S[3], S[4], S[5], S[6], S[7], i);
        t = S[7]; S[7] = S[6]; S[6] = S[5]; S[5] = S[4];
        S[4] = S[3]; S[3] = S[2]; S[2] = S[1]; S[1] = S[0]; S[0] = t;
    }
    /* feedback */
    for (i = 0; i < 8; i++) {
        md->state[i] = md->state[i] + S[i];
    }
}
/* Initialize the hash state */
void sha256_init(struct sha256_state *md)
{
    md->curlen = 0;
    md->length = 0;
    md->state[0] = 0x6A09E667UL;
    md->state[1] = 0xBB67AE85UL;
    md->state[2] = 0x3C6EF372UL;
    md->state[3] = 0xA54FF53AUL;
    md->state[4] = 0x510E527FUL;
    md->state[5] = 0x9B05688CUL;
    md->state[6] = 0x1F83D9ABUL;
    md->state[7] = 0x5BE0CD19UL;
}
/**
 Process a block of memory though the hash
 @param md     The hash state
 @param in     The data to hash
 @param inlen  The length of the data (octets)
 @return CRYPT_OK if successful
 */
int sha256_process(struct sha256_state *md, const unsigned char *in,
                          unsigned long inlen)
{
    unsigned long n;
#define block_size 64
    if (md->curlen > sizeof(md->buf))
        return -1;
    while (inlen > 0) {
        if (md->curlen == 0 && inlen >= block_size) {
            sha256_compress(md, (unsigned char *) in);
            md->length += block_size * 8;
            in += block_size;
            inlen -= block_size;
        } else {
            n = MIN(inlen, (block_size - md->curlen));
            memcpy(md->buf + md->curlen, in, n);
            md->curlen += n;
            in += n;
            inlen -= n;
            if (md->curlen == block_size) {
                sha256_compress(md, md->buf);
                md->length += 8 * block_size;
                md->curlen = 0;
            }
        }
    }
    return 0;
}
/**
 Terminate the hash to get the digest
 @param md  The hash state
 @param out [out] The destination of the hash (32 bytes)
 @return CRYPT_OK if successful
 */
int sha256_done(struct sha256_state *md, unsigned char *out)
{
    int i;
    if (md->curlen >= sizeof(md->buf))
        return -1;
    /* increase the length of the message */
    md->length += (uint64_t)md->curlen * 8;
    /* append the '1' bit */
    md->buf[md->curlen++] = (unsigned char) 0x80;
    /* if the length is currently above 56 bytes we append zeros
     * then compress.  Then we can fall back to padding zeros and length
     * encoding like normal.
     */
    if (md->curlen > 56) {
        while (md->curlen < 64) {
            md->buf[md->curlen++] = (unsigned char) 0;
        }
        sha256_compress(md, md->buf);
        md->curlen = 0;
    }
    /* pad upto 56 bytes of zeroes */
    while (md->curlen < 56) {
        md->buf[md->curlen++] = (unsigned char) 0;
    }
    /* store length */
    PUT_BE64(md->buf + 56, md->length);
    sha256_compress(md, md->buf);
    /* copy output */
    for (i = 0; i < 8; i++)
        PUT_BE32(out + (4 * i), md->state[i]);
    return 0;
}
