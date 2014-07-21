/*
instructions.c

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


#include "instructions.h"

#include "insts.h"
#include "prefix.h"
#include "x86defs.h"
#include "../include/mnemonics.h"


/* Helper macros to extract the type or index from an inst-node value. */
#define INST_NODE_INDEX(n) ((n) & 0x1fff)
#define INST_NODE_TYPE(n) ((n) >> 13)

/* Helper macro to read the actual flags that are associated with an inst-info. */
#define INST_INFO_FLAGS(ii) (FlagsTable[InstSharedInfoTable[(ii)->sharedIndex].flagsIndex])

/*
I use the trie data structure as I found it most fitting to a disassembler mechanism.
When you read a byte and have to decide if it's enough or you should read more bytes, 'till you get to the instruction information.
It's really fast because you POP the instruction info in top 3 iterates on the DB, because an instruction can be formed from two bytes + 3 bits reg from the ModR/M byte.
For a simple explanation, check this out:
http://www.csse.monash.edu.au/~lloyd/tildeAlgDS/Tree/Trie/
Futher reading: http://en.wikipedia.org/wiki/Trie

The first GATE (array you read off a trie data structure), as I call them, is statically allocated by the compiler.
The second and third gates if used are being allocated dynamically by the instructions-insertion functionality.

How would such a thing look in memory, say we support 4 instructions with 3 bytes top (means 2 dynamically allocated gates).

->
|-------|                                0,
|0|     -------------------------------> |-------|
|1|RET  |      1,                        |0|AND  |
|2|     -----> |-------|                 |1|XOR  |
|3|INT3 |      |0|PUSH |                 |2|OR   |         0,3,
|-------|      |1|POP  |                 |3|     --------->|-------|
               |2|PUSHF|                 |-------|         |0|ROR  |
               |3|POPF |                                   |1|ROL  |
               |-------|                                   |2|SHR  |
                                                           |3|SHL  |
                                                           |-------|

Of course, this is NOT how Intel instructions set looks!!!
but I just wanted to give a small demonstration.
Now the instructions you get from such a trie DB goes like this:

0, 0 - AND
0, 1 - XOR
0, 2 - OR
0, 3, 0, ROR
0, 3, 1, ROL
0, 3, 2, SHR
0, 3, 3, SHL
1 - RET
2, 0 - PUSH
2, 1 - POP
2, 2 - PUSHF
2, 3 - POPF
3 - INT3

I guess it's clear by now.
So now, if you read 0, you know that you have to enter the second gate(list) with the second byte specifying the index.
But if you read 1, you know that you go to an instruction (in this case, a RET).
That's why there's an Instruction-Node structure, it tells you whether you got to an instruction or another list
so you should keep on reading byte).

In Intel, you could go through 4 gates at top, because there're instructions which are built from 2 bytes and another smaller list
for the REG part, or newest SSE4 instructions which use 4 bytes for opcode.
Therefore, Intel's first gate is 256 long, and other gates are 256 (/72) or 8 long, yes, it costs pretty much alot of memory
for non-used defined instructions, but I think that it still rocks.
*/

/*
 * A helper function to look up the correct inst-info structure.
 * It does one fetch from the index-table, and then another to get the inst-info.
 * Note that it takes care about basic inst-info or inst-info-ex.
 * The caller should worry about boundary checks and whether it accesses a last-level table.
 */
static _InstInfo* inst_get_info(_InstNode in, int index)
{
	int instIndex = 0;

	in = InstructionsTree[INST_NODE_INDEX(in) + index];
	if (in == INT_NOTEXISTS) return NULL;

	instIndex = INST_NODE_INDEX(in);
	return INST_NODE_TYPE(in) == INT_INFO ? &InstInfos[instIndex] : (_InstInfo*)&InstInfosEx[instIndex];
}

/*
 * This function is responsible to return the instruction information of the first found in code.
 * It returns the _InstInfo of the found instruction, otherwise NULL.
 * code should point to the ModR/M byte upon exit (if used), or after the instruction binary code itself.
 * This function is NOT decoding-type dependant, it is up to the caller to see whether the instruction is valid.
 * Get the instruction info, using a Trie data structure.
 *
 * Sometimes normal prefixes become mandatory prefixes, which means they are now part of the instruction opcode bytes.

 * This is a bit tricky now,
 * if the first byte is a REP (F3) prefix, we will have to give a chance to an SSE instruction.
 * If an instruction doesn't exist, we will make it as a prefix and re-locateinst.
 * A case such that a REP prefix is being changed into an instruction byte and also an SSE instruction will not be found can't happen,
 * simply because there are no collisions between string instruction and SSE instructions (they are escaped).

 * As for S/SSE2/3, check for F2 and 66 as well.

 * In 64 bits, we have to make sure that we will skip the REX prefix, if it exists.
 * There's a specific case, where a 66 is mandatory but it was dropped because REG.W was used,
 * but it doesn't behave as an operand size prefix but as a mandatory, so we will have to take it into account.

 * For example (64 bits decoding mode):
 * 66 98 CBW
 * 48 98 CDQE
 * 66 48 98: db 0x66; CDQE
 * Shows that operand size is dropped.

 * Now, it's a mandatory prefix and NOT an operand size one.
 * 66480f2dc0 db 0x48; CVTPD2PI XMM0, XMM0
 * Although this instruction doesn't require a REX.W, it just shows, that even if it did - it doesn't matter.
 * REX.W is dropped because it's not requried, but the decode function disabled the operand size even so.
 */
static _InstInfo* inst_lookup_prefixed(_InstNode in, _PrefixState* ps)
{
	int checkOpSize = FALSE;
	int index = 0;
	_InstInfo* ii = NULL;

	/* Check prefixes of current decoded instruction (None, 0x66, 0xf3, 0xf2). */
	switch (ps->decodedPrefixes & (INST_PRE_OP_SIZE | INST_PRE_REPS))
	{
		case 0:
			/* Non-prefixed, index = 0. */
			index = 0;
		break;
		case INST_PRE_OP_SIZE:
			/* 0x66, index = 1. */
			index = 1;
			/* Mark that we used it as a mandatory prefix. */
			ps->isOpSizeMandatory = TRUE;
			ps->decodedPrefixes &= ~INST_PRE_OP_SIZE;
		break;
		case INST_PRE_REP:
			/* 0xf3, index = 2. */
			index = 2;
			ps->decodedPrefixes &= ~INST_PRE_REP;
		break;
		case INST_PRE_REPNZ:
			/* 0xf2, index = 3. */
			index = 3;
			ps->decodedPrefixes &= ~INST_PRE_REPNZ;
		break;
		default:
			/*
			 * Now we got a problem, since there are a few mandatory prefixes at once.
			 * There is only one case when it's ok, when the operand size prefix is for real (not mandatory).
			 * Otherwise we will have to return NULL, since the instruction is illegal.
			 * Therefore we will start with REPNZ and REP prefixes,
			 * try to get the instruction and only then check for the operand size prefix.
			 */

			/* If both REPNZ and REP are together, it's illegal for sure. */
			if ((ps->decodedPrefixes & INST_PRE_REPS) == INST_PRE_REPS) return NULL;

			/* Now we know it's either REPNZ+OPSIZE or REP+OPSIZE, so examine the instruction. */
			if (ps->decodedPrefixes & INST_PRE_REPNZ) {
				index = 3;
				ps->decodedPrefixes &= ~INST_PRE_REPNZ;
			} else if (ps->decodedPrefixes & INST_PRE_REP) {
				index = 2;
				ps->decodedPrefixes &= ~INST_PRE_REP;
			}
			/* Mark to verify the operand-size prefix of the fetched instruction below. */
			checkOpSize = TRUE;
		break;
	}

	/* Fetch the inst-info from the index. */
	ii = inst_get_info(in, index);

	if (checkOpSize) {
		/* If the instruction doesn't support operand size prefix, then it's illegal. */
		if ((ii == NULL) || (~INST_INFO_FLAGS(ii) & INST_PRE_OP_SIZE)) return NULL;
	}

	/* If there was a prefix, but the instruction wasn't found. Try to fall back to use the normal instruction. */
	if (ii == NULL) ii = inst_get_info(in, 0);
	return ii;
}

/* A helper function to look up special VEX instructions.
 * See if it's a MOD based instruction and fix index if required.
 * Only after a first lookup (that was done by caller), we can tell if we need to fix the index.
 * Because these are coupled instructions
 * (which means that the base instruction hints about the other instruction).
 * Note that caller should check if it's a MOD dependent instruction before getting in here.
 */
static _InstInfo* inst_vex_mod_lookup(_CodeInfo* ci, _InstNode in, _InstInfo* ii, unsigned int index)
{
	/* Advance to read the MOD from ModRM byte. */
	ci->code += 1;
	ci->codeLen -= 1;
	if (ci->codeLen < 0) return NULL;
	if (*ci->code < INST_DIVIDED_MODRM) {
		/* MOD is not 11, therefore change the index to 8 - 12 range in the prefixed table. */
		index += 4;
		/* Make a second lookup for this special instruction. */
		return inst_get_info(in, index);
	}
	/* Return the original one, in case we didn't find a stuited instruction. */
	return ii;
}

static _InstInfo* inst_vex_lookup(_CodeInfo* ci, _PrefixState* ps)
{
	_InstNode in = 0;
	unsigned int pp = 0, start = 0;
	unsigned int index = 4; /* VEX instructions start at index 4 in the Prefixed table. */
	uint8_t vex = *ps->vexPos, vex2 = 0, v = 0;
	int instType = 0, instIndex = 0;

	/* The VEX instruction will #ud if any of 66, f0, f2, f3, REX prefixes precede. */
	_iflags illegal = (INST_PRE_OP_SIZE | INST_PRE_LOCK | INST_PRE_REP | INST_PRE_REPNZ | INST_PRE_REX);
	if ((ps->decodedPrefixes & illegal) != 0) return NULL;

	/* Read the some fields from the VEX prefix we need to extract the instruction. */
	if (ps->prefixExtType == PET_VEX2BYTES) {
		ps->vexV = v = (~vex >> 3) & 0xf;
		pp = vex & 3;
		/* Implied leading 0x0f byte by default for 2 bytes VEX prefix. */
		start = 1;
	} else { /* PET_VEX3BYTES */
		start = vex & 0x1f;
		vex2 = *(ps->vexPos + 1);
		ps->vexV = v = (~vex2 >> 3) & 0xf;
		pp = vex2 & 3;
	}

	/* start can be either 1 (0x0f), 2 (0x0f, 0x038) or 3 (0x0f, 0x3a), otherwise it's illegal. */
	switch (start)
	{
		case 1: in = Table_0F; break;
		case 2: in = Table_0F_38; break;
		case 3: in = Table_0F_3A; break;
		default: return NULL;
	}

	/* pp is actually the implied mandatory prefix, apply it to the index. */
	index += pp; /* (None, 0x66, 0xf3, 0xf2) */

	/* Read a byte from the stream. */
	ci->codeLen -= 1;
	if (ci->codeLen < 0) return NULL;

	in = InstructionsTree[INST_NODE_INDEX(in) + *ci->code];
	if (in == INT_NOTEXISTS) return NULL;

	instType = INST_NODE_TYPE(in);
	instIndex = INST_NODE_INDEX(in);

	/*
	 * If we started with 0f38 or 0f3a so it's a prefixed table,
	 * therefore it's surely a VEXed instruction (because of a high index).
	 * However, starting with 0f, could also lead immediately to a prefixed table for some bytes.
	 * it might return NULL, if the index is invalid.
	 */
	if (instType == INT_LIST_PREFIXED) {
		_InstInfo* ii = inst_get_info(in, index);
		/* See if the instruction is dependent on MOD. */
		if ((ii != NULL) && (((_InstInfoEx*)ii)->flagsEx & INST_MODRR_BASED)) {
			ii = inst_vex_mod_lookup(ci, in, ii, index);
		}
		return ii;
	}

	/*
	 * If we reached here, obviously we started with 0f. VEXed instructions must be nodes of a prefixed table.
	 * But since we found an instruction (or divided one), just return NULL.
	 * They cannot lead to a VEXed instruction.
	 */
	if ((instType == INT_INFO) || (instType == INT_INFOEX) || (instType == INT_LIST_DIVIDED)) return NULL;

	/* Now we are left with handling either GROUP or FULL tables, therefore we will read another byte from the stream. */
	ci->code += 1;
	ci->codeLen -= 1;
	if (ci->codeLen < 0) return NULL;

	if (instType == INT_LIST_GROUP) {
		in = InstructionsTree[instIndex + ((*ci->code >> 3) & 7)];
		/* Continue below to check prefixed table. */
	} else if (instType == INT_LIST_FULL) {
		in = InstructionsTree[instIndex + *ci->code];
		/* Continue below to check prefixed table. */
	}

	/* Now that we got to the last table in the trie, check for a prefixed table. */
	if (INST_NODE_TYPE(in) == INT_LIST_PREFIXED) {
		_InstInfo* ii = inst_get_info(in, index);
		/* See if the instruction is dependent on MOD. */
		if ((ii != NULL) && (((_InstInfoEx*)ii)->flagsEx & INST_MODRR_BASED)) {
			ii = inst_vex_mod_lookup(ci, in, ii, index);
		}
		return ii;
	}

	/* No VEXed instruction was found. */
	return NULL;
}

_InstInfo* inst_lookup(_CodeInfo* ci, _PrefixState* ps)
{
	unsigned int tmpIndex0 = 0, tmpIndex1 = 0, tmpIndex2 = 0, rex = ps->vrex;
	int instType = 0;
	_InstNode in = 0;
	_InstInfo* ii = NULL;
	int isWaitIncluded = FALSE;

	/* See whether we have to handle a VEX prefixed instruction. */
	if (ps->decodedPrefixes & INST_PRE_VEX) {
		ii = inst_vex_lookup(ci, ps);
		if (ii != NULL) {
			/* Make sure that VEX.L exists when forced. */
			if ((((_InstInfoEx*)ii)->flagsEx & INST_FORCE_VEXL) && (~ps->vrex & PREFIX_EX_L)) return NULL;
			/* If the instruction doesn't use VEX.vvvv it must be zero. */
			if ((((_InstInfoEx*)ii)->flagsEx & INST_VEX_V_UNUSED) && ps->vexV) return NULL;
		}
		return ii;
	}

	/* Read first byte. */
	ci->codeLen -= 1;
	if (ci->codeLen < 0) return NULL;
	tmpIndex0 = *ci->code;

	/* Check for special 0x9b, WAIT instruction, which can be part of some instructions(x87). */
	if (tmpIndex0 == INST_WAIT_INDEX) {
		/* Only OCST_1dBYTES get a chance to include this byte as part of the opcode. */
		isWaitIncluded = TRUE;

		/* Ignore all prefixes, since they are useless and operate on the WAIT instruction itself. */
		prefixes_ignore_all(ps);

		/* Move to next code byte as a new whole instruction. */
		ci->code += 1;
		ci->codeLen -= 1;
		if (ci->codeLen < 0) return NULL; /* Faster to return NULL, it will be detected as WAIT later anyway. */
		/* Since we got a WAIT prefix, we re-read the first byte. */
		tmpIndex0 = *ci->code;
	}

	/* Walk first byte in InstructionsTree root. */
	in = InstructionsTree[tmpIndex0];
	if (in == INT_NOTEXISTS) return NULL;
	instType = INST_NODE_TYPE(in);

	/* Single byte instruction (OCST_1BYTE). */
	if ((instType < INT_INFOS) && (!isWaitIncluded)) {
		/* Some single byte instructions need extra treatment. */
		switch (tmpIndex0)
		{
			case INST_ARPL_INDEX:
				/*
				 * ARPL/MOVSXD share the same opcode, and both have different operands and mnemonics, of course.
				 * Practically, I couldn't come up with a comfortable way to merge the operands' types of ARPL/MOVSXD.
				 * And since the DB can't be patched dynamically, because the DB has to be multi-threaded compliant,
				 * I have no choice but to check for ARPL/MOVSXD right here - "right about now, the funk soul brother, check it out now, the funk soul brother...", fatboy slim
				 */
				if (ci->dt == Decode64Bits) {
					return &II_MOVSXD;
				} /* else ARPL will be returned because its defined in the DB already. */
			break;

			case INST_NOP_INDEX: /* Nopnopnop */
				/* Check for Pause, since it's prefixed with 0xf3, which is not a real mandatory prefix. */
				if (ps->decodedPrefixes & INST_PRE_REP) {
					/* Flag this prefix as used. */
					ps->usedPrefixes |= INST_PRE_REP;
					return &II_PAUSE;
				}

				/*
				 * Treat NOP/XCHG specially.
				 * If we're not in 64bits restore XCHG to NOP, since in the DB it's XCHG.
				 * Else if we're in 64bits examine REX, if exists, and decide which instruction should go to output.
				 * 48 90 XCHG RAX, RAX is a true NOP (eat REX in this case because it's valid).
				 * 90 XCHG EAX, EAX is a true NOP (and not high dword of RAX = 0 although it should be a 32 bits operation).
				 * Note that if the REX.B is used, then the register is not RAX anymore but R8, which means it's not a NOP.
				 */
				if (rex & PREFIX_EX_W) ps->usedPrefixes |= INST_PRE_REX;
				if ((ci->dt != Decode64Bits) || (~rex & PREFIX_EX_B)) return &II_NOP;
			break;
			
			case INST_LEA_INDEX:
				/* Ignore segment override prefixes for LEA instruction. */
				ps->decodedPrefixes &= ~INST_PRE_SEGOVRD_MASK;
				/* Update unused mask for ignoring segment prefix. */
				prefixes_ignore(ps, PFXIDX_SEG);
			break;
		}

		/* Return the 1 byte instruction we found. */
		return instType == INT_INFO ? &InstInfos[INST_NODE_INDEX(in)] : (_InstInfo*)&InstInfosEx[INST_NODE_INDEX(in)];
	}

	/* Read second byte, still doens't mean all of its bits are used (I.E: ModRM). */
	ci->code += 1;
	ci->codeLen -= 1;
	if (ci->codeLen < 0) return NULL;
	tmpIndex1 = *ci->code;
	
	/* Try single byte instruction + reg bits (OCST_13BYTES). */
	if ((instType == INT_LIST_GROUP) && (!isWaitIncluded)) return inst_get_info(in, (tmpIndex1 >> 3) & 7);

	/* Try single byte instruction + reg byte OR one whole byte (OCST_1dBYTES). */
	if (instType == INT_LIST_DIVIDED) {
		/* OCST_1dBYTES is relatively simple to OCST_2dBYTES, since it's really divided at 0xc0. */
		if (tmpIndex1 < INST_DIVIDED_MODRM) {
			/* An instruction which requires a ModR/M byte. Thus it's 1.3 bytes long instruction. */
			tmpIndex1 = (tmpIndex1 >> 3) & 7; /* Isolate the 3 REG/OPCODE bits. */
		} else { /* Normal 2 bytes instruction. */
			/*
			 * Divided instructions can't be in the range of 0x8-0xc0.
			 * That's because 0-8 are used for 3 bits group.
			 * And 0xc0-0xff are used for not-divided instruction.
			 * So the inbetween range is omitted, thus saving some more place in the tables.
			 */
			tmpIndex1 -= INST_DIVIDED_MODRM - 8;
		}

		in = InstructionsTree[INST_NODE_INDEX(in) + tmpIndex1];
		if (in == INT_NOTEXISTS) return NULL;
		instType = INST_NODE_TYPE(in);

		if (instType < INT_INFOS) {
			/* If the instruction doesn't support the wait (marked as opsize) as part of the opcode, it's illegal. */
			ii = instType == INT_INFO ? &InstInfos[INST_NODE_INDEX(in)] : (_InstInfo*)&InstInfosEx[INST_NODE_INDEX(in)];
			if ((~INST_INFO_FLAGS(ii) & INST_PRE_OP_SIZE) && (isWaitIncluded)) return NULL;
			return ii;
		}
		/*
		 * If we got here the instruction can support the wait prefix, so see if it was part of the stream.
		 * Examine prefixed table, specially used for 0x9b, since it's optional.
		 * No Wait: index = 0.
		 * Wait Exists, index = 1.
		 */
		return inst_get_info(in, isWaitIncluded);
	}

	/* Don't allow to continue if WAIT is part of the opcode, because there are no instructions that include it. */
	if (isWaitIncluded) return NULL;

	/* Try 2 bytes long instruction (doesn't include ModRM byte). */
	if (instType == INT_LIST_FULL) {
		in = InstructionsTree[INST_NODE_INDEX(in) + tmpIndex1];
		if (in == INT_NOTEXISTS) return NULL;
		instType = INST_NODE_TYPE(in);

		/* This is where we check if we just read two escape bytes in a row, which means it is a 3DNow! instruction. */
		if ((tmpIndex0 == _3DNOW_ESCAPE_BYTE) && (tmpIndex1 == _3DNOW_ESCAPE_BYTE)) return &II_3DNOW;

		/* 2 bytes instruction (OCST_2BYTES). */
		if (instType < INT_INFOS)
			return instType == INT_INFO ? &InstInfos[INST_NODE_INDEX(in)] : (_InstInfo*)&InstInfosEx[INST_NODE_INDEX(in)];

		/*
		 * 2 bytes + mandatory perfix.
		 * Mandatory prefixes can be anywhere in the prefixes.
		 * There cannot be more than one mandatory prefix, unless it's a normal operand size prefix.
		 */
		if (instType == INT_LIST_PREFIXED) return inst_lookup_prefixed(in, ps);
	}

	/* Read third byte, still doens't mean all of its bits are used (I.E: ModRM). */
	ci->code += 1;
	ci->codeLen -= 1;
	if (ci->codeLen < 0) return NULL;
	tmpIndex2 = *ci->code;

	/* Try 2 bytes + reg instruction (OCST_23BYTES). */
	if (instType == INT_LIST_GROUP) {
		in = InstructionsTree[INST_NODE_INDEX(in) + ((tmpIndex2 >> 3) & 7)];
		if (in == INT_NOTEXISTS) return NULL;
		instType = INST_NODE_TYPE(in);

		if (instType < INT_INFOS)
			return instType == INT_INFO ? &InstInfos[INST_NODE_INDEX(in)] : (_InstInfo*)&InstInfosEx[INST_NODE_INDEX(in)];

		/* It has to be a prefixed table then. */
		return inst_lookup_prefixed(in, ps);
	}

	/* Try 2 bytes + divided range (OCST_2dBYTES). */
	if (instType == INT_LIST_DIVIDED) {
		_InstNode in2 = InstructionsTree[INST_NODE_INDEX(in) + ((tmpIndex2 >> 3) & 7)];
		/*
		 * Do NOT check for NULL here, since we do a bit of a guess work,
		 * hence we don't override 'in', cause we might still need it.
		 */
		instType = INST_NODE_TYPE(in2);
		
		if (instType == INT_INFO) ii = &InstInfos[INST_NODE_INDEX(in2)];
		else if (instType == INT_INFOEX) ii = (_InstInfo*)&InstInfosEx[INST_NODE_INDEX(in2)];

		/*
		 * OCST_2dBYTES is complex, because there are a few instructions which are not divided in some special cases.
		 * If the instruction wasn't divided (but still it must be a 2.3 because we are in divided category)
		 * or it was an official 2.3 (because its index was less than 0xc0) -
		 * Then it means the instruction should be using the REG bits, otherwise give a chance to range 0xc0-0xff.
		 */
		/* If we found an instruction only by its REG bits, AND it is not divided, then return it. */
		if ((ii != NULL) && (INST_INFO_FLAGS(ii) & INST_NOT_DIVIDED)) return ii;
		/* Otherwise, if the range is above 0xc0, try the special divided range (range 0x8-0xc0 is omitted). */
		if (tmpIndex2 >= INST_DIVIDED_MODRM) return inst_get_info(in, tmpIndex2 - INST_DIVIDED_MODRM + 8);

		/* It might be that we got here without touching ii in the above if statements, then it becomes an invalid instruction prolly. */
		return ii;
	}

	/* Try 3 full bytes (OCST_3BYTES - no ModRM byte). */
	if (instType == INT_LIST_FULL) {
		/* OCST_3BYTES. */
		in = InstructionsTree[INST_NODE_INDEX(in) + tmpIndex2];
		if (in == INT_NOTEXISTS) return NULL;
		instType = INST_NODE_TYPE(in);

		if (instType < INT_INFOS)
			return instType == INT_INFO ? &InstInfos[INST_NODE_INDEX(in)] : (_InstInfo*)&InstInfosEx[INST_NODE_INDEX(in)];

		if (instType == INT_LIST_PREFIXED) return inst_lookup_prefixed(in, ps);
	}

	/* Kahtchinggg, damn. */
	return NULL;
}

/*
* 3DNow! instruction handling:

* This is used when we encounter a 3DNow! instruction.
* We can't really locate a 3DNow! instruction before we see two escaped bytes,
* 0x0f, 0x0f. Then we have to extract operands which are, dest=mmx register, src=mmx register or quadword indirection.
* When we are finished with the extraction of operands we can resume to locate the instruction by reading another byte
* which tells us which 3DNow instruction we really tracked down...
* So in order to tell the extract operands function which operands the 3DNow! instruction require, we need to set up some
* generic instruction info for 3DNow! instructions.

* In the inst_lookup itself, when we read an OCST_3BYTES which the two first bytes are 0x0f and 0x0f.
* we will return this special generic II for the specific operands we are interested in (MM, MM64).
* Then after extracting the operand, we'll call a completion routine for locating the instruction
* which will be called only for 3DNow! instructions, distinguished by a flag, and it will read the last byte of the 3 bytes.
*
* The id of this opcode should not be used, the following function should change it anyway.
*/
_InstInfo* inst_lookup_3dnow(_CodeInfo* ci)
{
	/* Start off from the two escape bytes gates... which is 3DNow! table.*/
	_InstNode in = Table_0F_0F;

	int index;

	/* Make sure we can read a byte off the stream. */
	if (ci->codeLen < 1) return NULL;

	index = *ci->code;

	ci->codeLen -= 1;
	ci->code += 1;
	return inst_get_info(in, index);
}
