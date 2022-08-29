// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2019 Huawei Technologies Co., Ltd
 */
#include "sm3.h"
#include <string.h>

struct sm3_context {
    uint32_t total[2];   /* number of bytes processed */
    uint32_t state[8];   /* intermediate digest state */
    uint8_t buffer[64];  /* data block being processed */
    uint8_t ipad[64];    /* HMAC: inner padding */
    uint8_t opad[64];    /* HMAC: outer padding */
};

static void sm3_init(struct sm3_context *ctx);
static void sm3_update(struct sm3_context *ctx, const uint8_t *input, size_t ilen);
static void sm3_final(struct sm3_context *ctx, uint8_t* output);

#define GET_UINT32_BE(n, b, i)				\
	do {						\
		(n) = ((uint32_t)(b)[(i)] << 24)     |	\
		      ((uint32_t)(b)[(i) + 1] << 16) |	\
		      ((uint32_t)(b)[(i) + 2] <<  8) |	\
		      ((uint32_t)(b)[(i) + 3]);		\
	} while (0)

#define PUT_UINT32_BE(n, b, i)				\
	do {						\
		(b)[(i)] = (uint8_t)((n) >> 24);	\
		(b)[(i) + 1] = (uint8_t)((n) >> 16);	\
		(b)[(i) + 2] = (uint8_t)((n) >>  8);	\
		(b)[(i) + 3] = (uint8_t)((n));		\
	} while (0)

static void sm3_init(struct sm3_context *ctx)
{
	ctx->total[0] = 0;
	ctx->total[1] = 0;

	ctx->state[0] = 0x7380166F;
	ctx->state[1] = 0x4914B2B9;
	ctx->state[2] = 0x172442D7;
	ctx->state[3] = 0xDA8A0600;
	ctx->state[4] = 0xA96F30BC;
	ctx->state[5] = 0x163138AA;
	ctx->state[6] = 0xE38DEE4D;
	ctx->state[7] = 0xB0FB0E4E;
}

static void sm3_process(struct sm3_context *ctx, const uint8_t data[64])
{
	uint32_t SS1, SS2, TT1, TT2, W[68], W1[64];
	uint32_t A, B, C, D, E, F, G, H;
	uint32_t T[64];
	uint32_t Temp1, Temp2, Temp3, Temp4, Temp5;
	int j;

	for (j = 0; j < 16; j++)
		T[j] = 0x79CC4519;
	for (j = 16; j < 64; j++)
		T[j] = 0x7A879D8A;

	GET_UINT32_BE(W[0], data,  0);
	GET_UINT32_BE(W[1], data,  4);
	GET_UINT32_BE(W[2], data,  8);
	GET_UINT32_BE(W[3], data, 12);
	GET_UINT32_BE(W[4], data, 16);
	GET_UINT32_BE(W[5], data, 20);
	GET_UINT32_BE(W[6], data, 24);
	GET_UINT32_BE(W[7], data, 28);
	GET_UINT32_BE(W[8], data, 32);
	GET_UINT32_BE(W[9], data, 36);
	GET_UINT32_BE(W[10], data, 40);
	GET_UINT32_BE(W[11], data, 44);
	GET_UINT32_BE(W[12], data, 48);
	GET_UINT32_BE(W[13], data, 52);
	GET_UINT32_BE(W[14], data, 56);
	GET_UINT32_BE(W[15], data, 60);

#define FF0(x, y, z)	((x) ^ (y) ^ (z))
#define FF1(x, y, z)	(((x) & (y)) | ((x) & (z)) | ((y) & (z)))

#define GG0(x, y, z)	((x) ^ (y) ^ (z))
#define GG1(x, y, z)	(((x) & (y)) | ((~(x)) & (z)))

#define SHL(x, n)	((x) << (n))
#define ROTL(x, y) ( (((uint32_t)(x)<<(uint32_t)((y)&31)) | (((uint32_t)(x)&0xFFFFFFFFUL)>>(uint32_t)((32-((y)&31))&31))) & 0xFFFFFFFFUL)

#define P0(x)	((x) ^ ROTL((x), 9) ^ ROTL((x), 17))
#define P1(x)	((x) ^ ROTL((x), 15) ^ ROTL((x), 23))

	for (j = 16; j < 68; j++) {
		/*
		 * W[j] = P1( W[j-16] ^ W[j-9] ^ ROTL(W[j-3],15)) ^
		 *        ROTL(W[j - 13],7 ) ^ W[j-6];
		 */

		Temp1 = W[j - 16] ^ W[j - 9];
		Temp2 = ROTL(W[j - 3], 15);
		Temp3 = Temp1 ^ Temp2;
		Temp4 = P1(Temp3);
		Temp5 =  ROTL(W[j - 13], 7) ^ W[j - 6];
		W[j] = Temp4 ^ Temp5;
	}

	for (j =  0; j < 64; j++)
		W1[j] = W[j] ^ W[j + 4];

	A = ctx->state[0];
	B = ctx->state[1];
	C = ctx->state[2];
	D = ctx->state[3];
	E = ctx->state[4];
	F = ctx->state[5];
	G = ctx->state[6];
	H = ctx->state[7];

	for (j = 0; j < 16; j++) {
		SS1 = ROTL(ROTL(A, 12) + E + ROTL(T[j], j), 7);
		SS2 = SS1 ^ ROTL(A, 12);
		TT1 = FF0(A, B, C) + D + SS2 + W1[j];
		TT2 = GG0(E, F, G) + H + SS1 + W[j];
		D = C;
		C = ROTL(B, 9);
		B = A;
		A = TT1;
		H = G;
		G = ROTL(F, 19);
		F = E;
		E = P0(TT2);
	}

	for (j = 16; j < 64; j++) {
		SS1 = ROTL(ROTL(A, 12) + E + ROTL(T[j], j), 7);
		SS2 = SS1 ^ ROTL(A, 12);
		TT1 = FF1(A, B, C) + D + SS2 + W1[j];
		TT2 = GG1(E, F, G) + H + SS1 + W[j];
		D = C;
		C = ROTL(B, 9);
		B = A;
		A = TT1;
		H = G;
		G = ROTL(F, 19);
		F = E;
		E = P0(TT2);
	}

	ctx->state[0] ^= A;
	ctx->state[1] ^= B;
	ctx->state[2] ^= C;
	ctx->state[3] ^= D;
	ctx->state[4] ^= E;
	ctx->state[5] ^= F;
	ctx->state[6] ^= G;
	ctx->state[7] ^= H;
}

static void sm3_update(struct sm3_context *ctx, const uint8_t *input, size_t ilen)
{
	size_t fill;
	size_t left;

	if (!ilen)
		return;

	left = ctx->total[0] & 0x3F;
	fill = 64 - left;

	ctx->total[0] += ilen;

	if (ctx->total[0] < ilen)
		ctx->total[1]++;

	if (left && ilen >= fill) {
		memcpy(ctx->buffer + left, input, fill);
		sm3_process(ctx, ctx->buffer);
		input += fill;
		ilen -= fill;
		left = 0;
	}

	while (ilen >= 64) {
		sm3_process(ctx, input);
		input += 64;
		ilen -= 64;
	}

	if (ilen > 0)
		memcpy(ctx->buffer + left, input, ilen);
}

static const uint8_t sm3_padding[64] = {
	0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static void sm3_final(struct sm3_context *ctx, uint8_t* output)
{
	uint32_t last, padn;
	uint32_t high, low;
	uint8_t msglen[8];

	high = (ctx->total[0] >> 29) | (ctx->total[1] <<  3);
	low  = ctx->total[0] << 3;

	PUT_UINT32_BE(high, msglen, 0);
	PUT_UINT32_BE(low,  msglen, 4);

	last = ctx->total[0] & 0x3F;
	padn = (last < 56) ? (56 - last) : (120 - last);

	sm3_update(ctx, sm3_padding, padn);
	sm3_update(ctx, msglen, 8);

	PUT_UINT32_BE(ctx->state[0], output,  0);
	PUT_UINT32_BE(ctx->state[1], output,  4);
	PUT_UINT32_BE(ctx->state[2], output,  8);
	PUT_UINT32_BE(ctx->state[3], output, 12);
	PUT_UINT32_BE(ctx->state[4], output, 16);
	PUT_UINT32_BE(ctx->state[5], output, 20);
	PUT_UINT32_BE(ctx->state[6], output, 24);
	PUT_UINT32_BE(ctx->state[7], output, 28);
}

void sm3(const void *in, unsigned long inlen, void* out)
{
	struct sm3_context ctx;
	sm3_init(&ctx);
	sm3_update(&ctx, (const uint8_t *)in, (size_t)inlen);
	sm3_final(&ctx, (uint8_t*)out);
}
