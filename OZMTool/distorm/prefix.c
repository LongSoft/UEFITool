/*
prefix.c

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


#include "prefix.h"

#include "x86defs.h"
#include "instructions.h"
#include "../include/mnemonics.h"


/*
 * The main purpose of this module is to keep track of all kind of prefixes a single instruction may have.
 * The problem is that a single instruction may have up to six different prefix-types.
 * That's why I have to detect such cases and drop those excess prefixes.
 */

int prefixes_is_valid(unsigned int ch, _DecodeType dt)
{
	switch (ch) {
		/* for i in xrange(0x40, 0x50): print "case 0x%2x:" % i */
		case 0x40: /* REX: */
		case 0x41:
		case 0x42:
		case 0x43:
		case 0x44:
		case 0x45:
		case 0x46:
		case 0x47:
		case 0x48:
		case 0x49:
		case 0x4a:
		case 0x4b:
		case 0x4c:
		case 0x4d:
		case 0x4e:
		case 0x4f: return (dt == Decode64Bits);
		case PREFIX_LOCK: return TRUE;
		case PREFIX_REPNZ: return TRUE;
		case PREFIX_REP: return TRUE;
		case PREFIX_CS: return TRUE;
		case PREFIX_SS: return TRUE;
		case PREFIX_DS: return TRUE;
		case PREFIX_ES: return TRUE;
		case PREFIX_FS: return TRUE;
		case PREFIX_GS: return TRUE;
		case PREFIX_OP_SIZE: return TRUE;
		case PREFIX_ADDR_SIZE: return TRUE;
		/* The VEXs might be false positives, the decode_perfixes will determine for sure. */
		case PREFIX_VEX2b: /* VEX is supported for all modes, because 16 bits Pmode is included. */
		case PREFIX_VEX3b: return TRUE;
	}
	return FALSE;
}

/* Ignore a specific prefix type. */
void prefixes_ignore(_PrefixState* ps, _PrefixIndexer pi)
{
	/*
	 * If that type of prefix appeared already, set the bit of that *former* prefix.
	 * Anyway, set the new index of that prefix type to the current index, so next time we know its position.
	 */
	if (ps->pfxIndexer[pi] != PFXIDX_NONE) ps->unusedPrefixesMask |= (1 << ps->pfxIndexer[pi]);
}

/* Ignore all prefix. */
void prefixes_ignore_all(_PrefixState* ps)
{
	int i;
	for (i = 0; i < PFXIDX_MAX; i++)
		prefixes_ignore(ps, i);
}

/* Calculates which prefixes weren't used and accordingly sets the bits in the unusedPrefixesMask. */
uint16_t prefixes_set_unused_mask(_PrefixState* ps)
{
	/*
	 * The decodedPrefixes represents the prefixes that were *read* from the binary stream for the instruction.
	 * The usedPrefixes represents the prefixes that were actually used by the instruction in the *decode* phase.
	 * Xoring between the two will result in a 'diff' which returns the prefixes that were read
	 * from the stream *and* that were never used in the actual decoding.
	 *
	 * Only one prefix per type can be set in decodedPrefixes from the stream.
	 * Therefore it's enough to check each type once and set the flag accordingly.
	 * That's why we had to book-keep each prefix type and its position.
	 * So now we know which bits we need to set exactly in the mask.
	 */
	_iflags unusedPrefixesDiff = ps->decodedPrefixes ^ ps->usedPrefixes;

	/* Examine unused prefixes by type: */
	/*
	 * About REX: it might be set in the diff although it was never in the stream itself.
	 * This is because the vrex is shared between VEX and REX and some places flag it as REX usage, while
	 * we were really decoding an AVX instruction.
	 * It's not a big problem, because the prefixes_ignore func will ignore it anyway,
	 * since it wasn't seen earlier. But it's important to know this.
	 */
	if (unusedPrefixesDiff & INST_PRE_REX) prefixes_ignore(ps, PFXIDX_REX);
	if (unusedPrefixesDiff & INST_PRE_SEGOVRD_MASK) prefixes_ignore(ps, PFXIDX_SEG);
	if (unusedPrefixesDiff & INST_PRE_LOKREP_MASK) prefixes_ignore(ps, PFXIDX_LOREP);
	if (unusedPrefixesDiff & INST_PRE_OP_SIZE) prefixes_ignore(ps, PFXIDX_OP_SIZE);
	if (unusedPrefixesDiff & INST_PRE_ADDR_SIZE) prefixes_ignore(ps, PFXIDX_ADRS);
	/* If a VEX instruction was found, its prefix is considered as used, therefore no point for checking for it. */

	return ps->unusedPrefixesMask;
}

/*
 * Mark a prefix as unused, and bookkeep where we last saw this same type,
 * because in the future we might want to disable it too.
 */
_INLINE_ void prefixes_track_unused(_PrefixState* ps, int index, _PrefixIndexer pi)
{
	prefixes_ignore(ps, pi);
	/* Book-keep the current index for this type. */
	ps->pfxIndexer[pi] = index;
}

/*
 * Read as many prefixes as possible, up to 15 bytes, and halt when we encounter non-prefix byte.
 * This algorithm tries to imitate a real processor, where the same prefix can appear a few times, etc.
 * The tiny complexity is that we want to know when a prefix was superfluous and mark any copy of it as unused.
 * Note that the last prefix of its type will be considered as used, and all the others (of same type) before it as unused.
 */
void prefixes_decode(const uint8_t* code, int codeLen, _PrefixState* ps, _DecodeType dt)
{
	int index, done;
	uint8_t vex;

	/*
	 * First thing to do, scan for prefixes, there are six types of prefixes.
	 * There may be up to six prefixes before a single instruction, not the same type, no special order,
	 * except REX/VEX must precede immediately the first opcode byte.
	 * BTW - This is the reason why I didn't make the REP prefixes part of the instructions (STOS/SCAS/etc).
	 *
	 * Another thing, the instruction maximum size is 15 bytes, thus if we read more than 15 bytes, we will halt.
	 *
	 * We attach all prefixes to the next instruction, there might be two or more occurrences from the same prefix.
	 * Also, since VEX can be allowed only once we will test it separately.
	 */
	for (index = 0, done = FALSE;
		 (codeLen > 0) && (code - ps->start < INST_MAXIMUM_SIZE);
		 code++, codeLen--, index++) {
		/*
		NOTE: AMD treat lock/rep as two different groups... But I am based on Intel.

			- Lock and Repeat:
				- 0xF0 — LOCK
				- 0xF2 — REPNE/REPNZ
				- 0xF3 - REP/REPE/REPZ
			- Segment Override:
				- 0x2E - CS
				- 0x36 - SS
				- 0x3E - DS
				- 0x26 - ES
				- 0x64 - FS
				- 0x65 - GS
			- Operand-Size Override: 0x66, switching default size.
			- Address-Size Override: 0x67, switching default size.

		64 Bits:
			- REX: 0x40 - 0x4f, extends register access.
			- 2 Bytes VEX: 0xc4
			- 3 Bytes VEX: 0xc5
		32 Bits:
			- 2 Bytes VEX: 0xc4 11xx-xxxx
			- 3 Bytes VEX: 0xc5 11xx-xxxx
		*/

		/* Examine what type of prefix we got. */
		switch (*code)
		{
			/* REX type, 64 bits decoding mode only: */
			case 0x40:
			case 0x41:
			case 0x42:
			case 0x43:
			case 0x44:
			case 0x45:
			case 0x46:
			case 0x47:
			case 0x48:
			case 0x49:
			case 0x4a:
			case 0x4b:
			case 0x4c:
			case 0x4d:
			case 0x4e:
			case 0x4f:
				if (dt == Decode64Bits) {
					ps->decodedPrefixes |= INST_PRE_REX;
					ps->vrex = *code & 0xf; /* Keep only BXRW. */
					ps->rexPos = code;
					ps->prefixExtType = PET_REX;
					prefixes_track_unused(ps, index, PFXIDX_REX);
				} else done = TRUE; /* If we are not in 64 bits mode, it's an instruction, then halt. */
			break;

			/* LOCK and REPx type: */
			case PREFIX_LOCK:
				ps->decodedPrefixes |= INST_PRE_LOCK;
				prefixes_track_unused(ps, index, PFXIDX_LOREP);
			break;
			case PREFIX_REPNZ:
				ps->decodedPrefixes |= INST_PRE_REPNZ;
				prefixes_track_unused(ps, index, PFXIDX_LOREP);
			break;
			case PREFIX_REP:
				ps->decodedPrefixes |= INST_PRE_REP;
				prefixes_track_unused(ps, index, PFXIDX_LOREP);
			break;

			/* Seg Overide type: */
			case PREFIX_CS:
				ps->decodedPrefixes |= INST_PRE_CS;
				prefixes_track_unused(ps, index, PFXIDX_SEG);
			break;
			case PREFIX_SS:
				ps->decodedPrefixes |= INST_PRE_SS;
				prefixes_track_unused(ps, index, PFXIDX_SEG);
			break;
			case PREFIX_DS:
				ps->decodedPrefixes |= INST_PRE_DS;
				prefixes_track_unused(ps, index, PFXIDX_SEG);
			break;
			case PREFIX_ES:
				ps->decodedPrefixes |= INST_PRE_ES;
				prefixes_track_unused(ps, index, PFXIDX_SEG);
			break;
			case PREFIX_FS:
				ps->decodedPrefixes |= INST_PRE_FS;
				prefixes_track_unused(ps, index, PFXIDX_SEG);
			break;
			case PREFIX_GS:
				ps->decodedPrefixes |= INST_PRE_GS;
				prefixes_track_unused(ps, index, PFXIDX_SEG);
			break;

			/* Op Size type: */
			case PREFIX_OP_SIZE:
				ps->decodedPrefixes |= INST_PRE_OP_SIZE;
				prefixes_track_unused(ps, index, PFXIDX_OP_SIZE);
			break;

			/* Addr Size type: */
			case PREFIX_ADDR_SIZE:
				ps->decodedPrefixes |= INST_PRE_ADDR_SIZE;
				prefixes_track_unused(ps, index, PFXIDX_ADRS);
			break;

			/* Non-prefix byte now, so break 2. */
			default: done = TRUE; break;
		}
		if (done) break;
	}

	/* 2 Bytes VEX: */
	if ((codeLen >= 2) &&
		(*code == PREFIX_VEX2b) &&
		((code - ps->start) <= INST_MAXIMUM_SIZE - 2)) {
		/*
		 * In 32 bits the second byte has to be in the special range of Mod=11.
		 * Otherwise it might be a normal LDS instruction.
		 */
		if ((dt == Decode64Bits) || (*(code + 1) >= INST_DIVIDED_MODRM)) {
			ps->vexPos = code + 1;
			ps->decodedPrefixes |= INST_PRE_VEX;
			ps->prefixExtType = PET_VEX2BYTES;

			/*
			 * VEX 1 byte bits:
			 * |7-6--3-2-10|
			 * |R|vvvv|L|pp|
			 * |-----------|
			 */

			/* -- Convert from VEX prefix to VREX flags -- */
			vex = *ps->vexPos;
			if (~vex & 0x80 && dt == Decode64Bits) ps->vrex |= PREFIX_EX_R; /* Convert VEX.R. */
			if (vex & 4) ps->vrex |= PREFIX_EX_L; /* Convert VEX.L. */

			code += 2;
		}
	}

	/* 3 Bytes VEX: */
	if ((codeLen >= 3) &&
		(*code == PREFIX_VEX3b) &&
		((code - ps->start) <= INST_MAXIMUM_SIZE - 3) &&
		(~ps->decodedPrefixes & INST_PRE_VEX)) {
		/*
		 * In 32 bits the second byte has to be in the special range of Mod=11.
		 * Otherwise it might be a normal LES instruction.
		 * And we don't care now about the 3rd byte.
		 */
		if ((dt == Decode64Bits) || (*(code + 1) >= INST_DIVIDED_MODRM)) {
			ps->vexPos = code + 1;
			ps->decodedPrefixes |= INST_PRE_VEX;
			ps->prefixExtType = PET_VEX3BYTES;

			/*
			 * VEX first and second bytes:
			 * |7-6-5-4----0|  |7-6--3-2-10|
			 * |R|X|B|m-mmmm|  |W|vvvv|L|pp|
			 * |------------|  |-----------|
			 */

			/* -- Convert from VEX prefix to VREX flags -- */
			vex = *ps->vexPos;
			ps->vrex |= ((~vex >> 5) & 0x7); /* Shift and invert VEX.R/X/B to their place */
			vex = *(ps->vexPos + 1);
			if (vex & 4) ps->vrex |= PREFIX_EX_L; /* Convert VEX.L. */
			if (vex & 0x80) ps->vrex |= PREFIX_EX_W; /* Convert VEX.W. */

			/* Clear some flags if the mode isn't 64 bits. */
			if (dt != Decode64Bits) ps->vrex &= ~(PREFIX_EX_B | PREFIX_EX_X | PREFIX_EX_R | PREFIX_EX_W);

			code += 3;
		}
	}

	/*
	 * Save last byte scanned address, so the decoder could keep on scanning from this point and on and on and on.
	 * In addition the decoder is able to know that the last byte could lead to MMX/SSE instructions (preceding REX if exists).
	 */
	ps->last = code; /* ps->last points to an opcode byte. */
}

/*
 * For every memory-indirection operand we want to set its corresponding default segment.
 * If the segment is being overrided, we need to see whether we use it or not.
 * We will use it only if it's not the default one already.
 */
void prefixes_use_segment(_iflags defaultSeg, _PrefixState* ps, _DecodeType dt, _DInst* di)
{
	_iflags flags = 0;
	if (dt == Decode64Bits) flags = ps->decodedPrefixes & INST_PRE_SEGOVRD_MASK64;
	else flags = ps->decodedPrefixes & INST_PRE_SEGOVRD_MASK;

	if ((flags == 0) || (flags == defaultSeg)) {
		flags = defaultSeg;
		di->segment |= SEGMENT_DEFAULT;
	} else if (flags != defaultSeg) {
		/* Use it only if it's non-default segment. */
		ps->usedPrefixes |= flags;
	}

	/* ASSERT: R_XX must be below 128. */
	switch (flags)
	{
		case INST_PRE_ES: di->segment |= R_ES; break;
		case INST_PRE_CS: di->segment |= R_CS; break;
		case INST_PRE_SS: di->segment |= R_SS; break;
		case INST_PRE_DS: di->segment |= R_DS; break;
		case INST_PRE_FS: di->segment |= R_FS; break;
		case INST_PRE_GS: di->segment |= R_GS; break;
	}

	/* If it's one of the CS,SS,DS,ES and the mode is 64 bits, set segment it to none, since it's ignored. */
	if ((dt == Decode64Bits) && (flags & INST_PRE_SEGOVRD_MASK32)) di->segment = R_NONE;
}
