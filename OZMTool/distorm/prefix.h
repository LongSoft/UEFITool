/*
prefix.h

diStorm3 - Powerful disassembler for X86/AMD64
http://ragestorm.net/distorm/
distorm at gmail dot com
Copyright (C) 2003-2012 Gil Dabah

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>
*/


#ifndef PREFIX_H
#define PREFIX_H

#include "config.h"
#include "decoder.h"


/* Specifies the type of the extension prefix, such as: REX, 2 bytes VEX, 3 bytes VEX. */
typedef enum {PET_NONE = 0, PET_REX, PET_VEX2BYTES, PET_VEX3BYTES} _PrefixExtType;

/* Specifies an index into a table of prefixes by their type. */
typedef enum {PFXIDX_NONE = -1, PFXIDX_REX, PFXIDX_LOREP, PFXIDX_SEG, PFXIDX_OP_SIZE, PFXIDX_ADRS, PFXIDX_MAX} _PrefixIndexer;

/*
* This holds the prefixes state for the current instruction we decode.
* decodedPrefixes includes all specific prefixes that the instruction got.
* start is a pointer to the first prefix to take into account.
* last is a pointer to the last byte we scanned.
* Other pointers are used to keep track of prefixes positions and help us know if they appeared already and where.
*/
typedef struct {
	_iflags decodedPrefixes, usedPrefixes;
	const uint8_t *start, *last, *vexPos, *rexPos;
	_PrefixExtType prefixExtType;
	uint16_t unusedPrefixesMask;
	/* Indicates whether the operand size prefix (0x66) was used as a mandatory prefix. */
	int isOpSizeMandatory;
	/* If VEX prefix is used, store the VEX.vvvv field. */
	unsigned int vexV;
	/* The fields B/X/R/W/L of REX and VEX are stored together in this byte. */
	unsigned int vrex;

	/* !! Make sure pfxIndexer is LAST! Otherwise memset won't work well with it. !! */

	/* Holds the offset to the prefix byte by its type. */
	int pfxIndexer[PFXIDX_MAX];
} _PrefixState;

/*
* Intel supports 6 types of prefixes, whereas AMD supports 5 types (lock is seperated from rep/nz).
* REX is the fifth prefix type, this time I'm based on AMD64.
* VEX is the 6th, though it can't be repeated.
*/
#define MAX_PREFIXES (5)

int prefixes_is_valid(unsigned int ch, _DecodeType dt);
void prefixes_ignore(_PrefixState* ps, _PrefixIndexer pi);
void prefixes_ignore_all(_PrefixState* ps);
uint16_t prefixes_set_unused_mask(_PrefixState* ps);
void prefixes_decode(const uint8_t* code, int codeLen, _PrefixState* ps, _DecodeType dt);
void prefixes_use_segment(_iflags defaultSeg, _PrefixState* ps, _DecodeType dt, _DInst* di);

#endif /* PREFIX_H */
