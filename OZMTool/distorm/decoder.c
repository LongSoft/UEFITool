/*
decoder.c

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


#include "decoder.h"
#include "instructions.h"
#include "insts.h"
#include "prefix.h"
#include "x86defs.h"
#include "operands.h"
#include "insts.h"
#include "../include/mnemonics.h"


/* Instruction Prefixes - Opcode - ModR/M - SIB - Displacement - Immediate */

static _DecodeType decode_get_effective_addr_size(_DecodeType dt, _iflags decodedPrefixes)
{
	/*
	 * This table is to map from the current decoding mode to an effective address size:
	 * Decode16 -> Decode32
	 * Decode32 -> Decode16
	 * Decode64 -> Decode32
	 */
	static _DecodeType AddrSizeTable[] = {Decode32Bits, Decode16Bits, Decode32Bits};

	/* Switch to non default mode if prefix exists, only for ADDRESS SIZE. */
	if (decodedPrefixes & INST_PRE_ADDR_SIZE) dt = AddrSizeTable[dt];
	return dt;
}

static _DecodeType decode_get_effective_op_size(_DecodeType dt, _iflags decodedPrefixes, unsigned int rex, _iflags instFlags)
{
	/*
	 * This table is to map from the current decoding mode to an effective operand size:
	 * Decode16 -> Decode32
	 * Decode32 -> Decode16
	 * Decode64 -> Decode16
	 * Not that in 64bits it's a bit more complicated, because of REX and promoted instructions.
	 */
	static _DecodeType OpSizeTable[] = {Decode32Bits, Decode16Bits, Decode16Bits};

	if (decodedPrefixes & INST_PRE_OP_SIZE) return OpSizeTable[dt];

	if (dt == Decode64Bits) {
		/*
		 * REX Prefix toggles data size to 64 bits.
		 * Operand size prefix toggles data size to 16.
		 * Default data size is 32 bits.
		 * Promoted instructions are 64 bits if they don't require a REX perfix.
		 * Non promoted instructions are 64 bits if the REX prefix exists.
		 */
		/* Automatically promoted instructions have only INST_64BITS SET! */
		if (((instFlags & (INST_64BITS | INST_PRE_REX)) == INST_64BITS) ||
		/* Other instructions in 64 bits can be promoted only with a REX prefix. */
			((decodedPrefixes & INST_PRE_REX) && (rex & PREFIX_EX_W))) dt = Decode64Bits;
		else dt = Decode32Bits; /* Default. */
	}
	return dt;
}

/* A helper macro to convert from diStorm's CPU flags to EFLAGS. */
#define CONVERT_FLAGS_TO_EFLAGS(dst, src, field) dst->field = ((src->field & D_COMPACT_SAME_FLAGS) | \
	((src->field & D_COMPACT_IF) ? D_IF : 0) | \
	((src->field & D_COMPACT_DF) ? D_DF : 0) | \
	((src->field & D_COMPACT_OF) ? D_OF : 0));

static _DecodeResult decode_inst(_CodeInfo* ci, _PrefixState* ps, _DInst* di)
{
	/* Remember whether the instruction is privileged. */
	uint16_t privilegedFlag = 0;

	/* The ModR/M byte of the current instruction. */
	unsigned int modrm = 0;

	/* The REX/VEX prefix byte value. */
	unsigned int vrex = ps->vrex;

	/*
	 * Backup original input, so we can use it later if a problem occurs
	 * (like not enough data for decoding, invalid opcode, etc).
	 */
	const uint8_t* startCode = ci->code;

	/* Holds the info about the current found instruction. */
	_InstInfo* ii = NULL;
	_InstSharedInfo* isi = NULL;

	/* Used only for special CMP instructions which have pseudo opcodes suffix. */
	unsigned char cmpType = 0;

	/*
	 * Indicates whether it is right to LOCK the instruction by decoding its first operand.
	 * Only then you know if it's ok to output the LOCK prefix's text...
	 * Used for first operand only.
	 */
	int lockable = FALSE;

	/* Calcualte (and cache) effective-operand-size and effective-address-size only once. */
	_DecodeType effOpSz, effAdrSz;
	_iflags instFlags;

	ii = inst_lookup(ci, ps);
	if (ii == NULL) goto _Undecodable;
	isi = &InstSharedInfoTable[ii->sharedIndex];
	instFlags = FlagsTable[isi->flagsIndex];

	/* Copy the privileged bit and remove it from the opcodeId field ASAP. */
	privilegedFlag = ii->opcodeId & OPCODE_ID_PRIVILEGED;
	ii->opcodeId &= ~OPCODE_ID_PRIVILEGED;

	/*
	 * If both REX and OpSize are available we will have to disable the OpSize, because REX has precedence.
	 * However, only if REX.W is set !
	 * We had to wait with this test, since the operand size may be a mandatory prefix,
	 * and we know it only after prefetching.
	 */
	if ((ps->prefixExtType == PET_REX) &&
		(ps->decodedPrefixes & INST_PRE_OP_SIZE) &&
		(!ps->isOpSizeMandatory) &&
		(vrex & PREFIX_EX_W)) {
		ps->decodedPrefixes &= ~INST_PRE_OP_SIZE;
		prefixes_ignore(ps, PFXIDX_OP_SIZE);
	}

	/*
	 * In this point we know the instruction we are about to decode and its operands (unless, it's an invalid one!),
	 * so it makes it the right time for decoding-type suitability testing.
	 * Which practically means, don't allow 32 bits instructions in 16 bits decoding mode, but do allow
	 * 16 bits instructions in 32 bits decoding mode, of course...

	 * NOTE: Make sure the instruction set for 32 bits has explicitly this specfic flag set.
	 * NOTE2: Make sure the instruction set for 64 bits has explicitly this specfic flag set.

	 * If this is the case, drop what we've got and restart all over after DB'ing that byte.

	 * Though, don't drop an instruction which is also supported in 16 and 32 bits.
	 */

	/* ! ! ! DISABLED UNTIL FURTHER NOTICE ! ! ! Decode16Bits CAN NOW DECODE 32 BITS INSTRUCTIONS ! ! !*/
	/* if (ii && (dt == Decode16Bits) && (instFlags & INST_32BITS) && (~instFlags & INST_16BITS)) ii = NULL; */

	/* Drop instructions which are invalid in 64 bits. */
	if ((ci->dt == Decode64Bits) && (instFlags & INST_INVALID_64BITS)) goto _Undecodable;

	/* If it's only a 64 bits instruction drop it in other decoding modes. */
	if ((ci->dt != Decode64Bits) && (instFlags & INST_64BITS_FETCH)) goto _Undecodable;

	if (instFlags & INST_MODRM_REQUIRED) {
		/* If the ModRM byte is not part of the opcode, skip the last byte code, so code points now to ModRM. */
		if (~instFlags & INST_MODRM_INCLUDED) {
			ci->code++;
			if (--ci->codeLen < 0) goto _Undecodable;
		}
		modrm = *ci->code;

		/* Some instructions enforce that reg=000, so validate that. (Specifically EXTRQ). */
		if ((instFlags & INST_FORCE_REG0) && (((modrm >> 3) & 7) != 0)) goto _Undecodable;
		/* Some instructions enforce that mod=11, so validate that. */
		if ((instFlags & INST_MODRR_REQUIRED) && (modrm < INST_DIVIDED_MODRM)) goto _Undecodable;
	}

	ci->code++; /* Skip the last byte we just read (either last opcode's byte code or a ModRM). */

	/* Cache the effective operand-size and address-size. */
	effOpSz = decode_get_effective_op_size(ci->dt, ps->decodedPrefixes, vrex, instFlags);
	effAdrSz = decode_get_effective_addr_size(ci->dt, ps->decodedPrefixes);

	memset(di, 0, sizeof(_DInst));
	di->base = R_NONE;

	/*
	 * Try to extract the next operand only if the latter exists.
	 * For example, if there is not first operand, no reason to try to extract second operand...
	 * I decided that a for-break is better for readability in this specific case than goto.
	 * Note: do-while with a constant 0 makes the compiler warning about it.
	 */
	for (;;) {
		if (isi->d != OT_NONE) {
			if (!operands_extract(ci, di, ii, instFlags, (_OpType)isi->d, ONT_1, modrm, ps, effOpSz, effAdrSz, &lockable)) goto _Undecodable;
		} else break;

		if (isi->s != OT_NONE) {
			if (!operands_extract(ci, di, ii, instFlags, (_OpType)isi->s, ONT_2, modrm, ps, effOpSz, effAdrSz, NULL)) goto _Undecodable;
		} else break;

		/* Use third operand, only if the flags says this InstInfo requires it. */
		if (instFlags & INST_USE_OP3) {
			if (!operands_extract(ci, di, ii, instFlags, (_OpType)((_InstInfoEx*)ii)->op3, ONT_3, modrm, ps, effOpSz, effAdrSz, NULL)) goto _Undecodable;
		} else break;
		
		/* Support for a fourth operand is added for (i.e:) INSERTQ instruction. */
		if (instFlags & INST_USE_OP4) {
			if (!operands_extract(ci, di, ii, instFlags, (_OpType)((_InstInfoEx*)ii)->op4, ONT_4, modrm, ps, effOpSz, effAdrSz, NULL)) goto _Undecodable;
		}
		break;
	} /* Continue here after all operands were extracted. */

	/* If it were a 3DNow! instruction, we will have to find the instruction itself now that we got its operands extracted. */
	if (instFlags & INST_3DNOW_FETCH) {
		ii = inst_lookup_3dnow(ci);
		if (ii == NULL) goto _Undecodable;
		isi = &InstSharedInfoTable[ii->sharedIndex];
		instFlags = FlagsTable[isi->flagsIndex];
	}

	/* Check whether pseudo opcode is needed, only for CMP instructions: */
	if (instFlags & INST_PSEUDO_OPCODE) {
		if (--ci->codeLen < 0) goto _Undecodable;
		cmpType = *ci->code;
		ci->code++;
		if (instFlags & INST_PRE_VEX) {
			/* AVX Comparison type must be between 0 to 32, otherwise Reserved. */
			if (cmpType >= INST_VCMP_MAX_RANGE) goto _Undecodable;
		} else {
			/* SSE Comparison type must be between 0 to 8, otherwise Reserved. */
			if (cmpType >= INST_CMP_MAX_RANGE) goto _Undecodable;
		}
	}

	/*
	 * There's a limit of 15 bytes on instruction length. The only way to violate
	 * this limit is by putting redundant prefixes before an instruction.
	 * start points to first prefix if any, otherwise it points to instruction first byte.
	 */
	if ((ci->code - ps->start) > INST_MAXIMUM_SIZE) goto _Undecodable; /* Drop instruction. */

	/*
	 * If we reached here the instruction was fully decoded, we located the instruction in the DB and extracted operands.
	 * Use the correct mnemonic according to the DT.
	 * If we are in 32 bits decoding mode it doesn't necessarily mean we will choose mnemonic2, alas,
	 * it means that if there is a mnemonic2, it will be used.
	 */

	/* Start with prefix LOCK. */
	if ((lockable == TRUE) && (instFlags & INST_PRE_LOCK)) {
		ps->usedPrefixes |= INST_PRE_LOCK;
		di->flags |= FLAG_LOCK;
	} else if ((instFlags & INST_PRE_REPNZ) && (ps->decodedPrefixes & INST_PRE_REPNZ)) {
		ps->usedPrefixes |= INST_PRE_REPNZ;
		di->flags |= FLAG_REPNZ;
	} else if ((instFlags & INST_PRE_REP) && (ps->decodedPrefixes & INST_PRE_REP)) {
		ps->usedPrefixes |= INST_PRE_REP;
		di->flags |= FLAG_REP;
	}

	/* If it's JeCXZ the ADDR_SIZE prefix affects them. */
	if ((instFlags & (INST_PRE_ADDR_SIZE | INST_USE_EXMNEMONIC)) == (INST_PRE_ADDR_SIZE | INST_USE_EXMNEMONIC)) {
		ps->usedPrefixes |= INST_PRE_ADDR_SIZE;
		if (effAdrSz == Decode16Bits) di->opcode = ii->opcodeId;
		else if (effAdrSz == Decode32Bits) di->opcode = ((_InstInfoEx*)ii)->opcodeId2;
		/* Ignore REX.W in 64bits, JECXZ is promoted. */
		else /* Decode64Bits */ di->opcode = ((_InstInfoEx*)ii)->opcodeId3;
	}

	/* LOOPxx instructions are also native instruction, but they are special case ones, ADDR_SIZE prefix affects them. */
	else if ((instFlags & (INST_PRE_ADDR_SIZE | INST_NATIVE)) == (INST_PRE_ADDR_SIZE | INST_NATIVE)) {
		di->opcode = ii->opcodeId;

		/* If LOOPxx gets here from 64bits, it must be Decode32Bits because Address Size perfix is set. */
		ps->usedPrefixes |= INST_PRE_ADDR_SIZE;
	}
	/*
	 * Note:
	 * If the instruction is prefixed by operand size we will format it in the non-default decoding mode!
	 * So there might be a situation that an instruction of 32 bit gets formatted in 16 bits decoding mode.
	 * Both ways should end up with a correct and expected formatting of the text.
	*/
	else if (effOpSz == Decode16Bits) { /* Decode16Bits */

		/* Set operand size. */
		FLAG_SET_OPSIZE(di, Decode16Bits);

		/*
		 * If it's a special instruction which has two mnemonics, then use the 16 bits one + update usedPrefixes.
		 * Note: use 16 bits mnemonic if that instruction supports 32 bit or 64 bit explicitly.
		 */
		if ((instFlags & INST_USE_EXMNEMONIC) && ((instFlags & (INST_32BITS | INST_64BITS)) == 0)) ps->usedPrefixes |= INST_PRE_OP_SIZE;
		di->opcode = ii->opcodeId;
	} else if (effOpSz == Decode32Bits) { /* Decode32Bits */

		/* Set operand size. */
		FLAG_SET_OPSIZE(di, Decode32Bits);

		/* Give a chance for special mnemonic instruction in 32 bits decoding. */
		if (instFlags & INST_USE_EXMNEMONIC) {
			ps->usedPrefixes |= INST_PRE_OP_SIZE;
			/* Is it a special instruction which has another mnemonic for mod=11 ? */
			if (instFlags & INST_MNEMONIC_MODRM_BASED) {
				if (modrm >= INST_DIVIDED_MODRM) di->opcode = ii->opcodeId;
				else di->opcode = ((_InstInfoEx*)ii)->opcodeId2;
			} else di->opcode = ((_InstInfoEx*)ii)->opcodeId2;
		} else di->opcode = ii->opcodeId;
	} else { /* Decode64Bits, note that some instructions might be decoded in Decode32Bits above. */

		/* Set operand size. */
		FLAG_SET_OPSIZE(di, Decode64Bits);

		if (instFlags & (INST_USE_EXMNEMONIC | INST_USE_EXMNEMONIC2)) {
			/*
			 * We shouldn't be here for MODRM based mnemonics with a MOD=11,
			 * because they must not use REX (otherwise it will get to the wrong instruction which share same opcode).
			 * See XRSTOR and XSAVEOPT.
			 */
			if ((instFlags & INST_MNEMONIC_MODRM_BASED) && (modrm >= INST_DIVIDED_MODRM)) goto _Undecodable;

			/* Use third mnemonic, for 64 bits. */
			if ((instFlags & INST_USE_EXMNEMONIC2) && (vrex & PREFIX_EX_W)) {
				ps->usedPrefixes |= INST_PRE_REX;
				di->opcode = ((_InstInfoEx*)ii)->opcodeId3;
			} else di->opcode = ((_InstInfoEx*)ii)->opcodeId2; /* Use second mnemonic. */
		} else di->opcode = ii->opcodeId;
	}

	/* If it's a native instruction use OpSize Prefix. */
	if ((instFlags & INST_NATIVE) && (ps->decodedPrefixes & INST_PRE_OP_SIZE)) ps->usedPrefixes |= INST_PRE_OP_SIZE;

	/* Check VEX mnemonics: */
	if ((instFlags & INST_PRE_VEX) &&
		(((((_InstInfoEx*)ii)->flagsEx & INST_MNEMONIC_VEXW_BASED) && (vrex & PREFIX_EX_W)) ||
		 ((((_InstInfoEx*)ii)->flagsEx & INST_MNEMONIC_VEXL_BASED) && (vrex & PREFIX_EX_L)))) {
		di->opcode = ((_InstInfoEx*)ii)->opcodeId2;
	}

	/* Or is it a special CMP instruction which needs a pseudo opcode suffix ? */
	if (instFlags & INST_PSEUDO_OPCODE) {
		/*
		 * The opcodeId is the offset to the FIRST pseudo compare mnemonic,
		 * we will have to fix it so it offsets into the corrected mnemonic.
		 * Therefore, we use another table to fix the offset.
		 */
		if (instFlags & INST_PRE_VEX) {
			/* Use the AVX pesudo compare mnemonics table. */
			di->opcode = ii->opcodeId + VCmpMnemonicOffsets[cmpType];
		} else {
			/* Use the SSE psuedo compare mnemonics table. */
			di->opcode = ii->opcodeId + CmpMnemonicOffsets[cmpType];
		}
	}

	/*
	 * Store the address size inside the flags.
	 * This is necessary for the caller to know the size of rSP when using PUSHA for example.
	 */
	FLAG_SET_ADDRSIZE(di, effAdrSz);

	/* Copy DST_WR flag. */
	if (instFlags & INST_DST_WR) di->flags |= FLAG_DST_WR;

	/* Set the unused prefixes mask. */
	di->unusedPrefixesMask = prefixes_set_unused_mask(ps);

	/* Fix privileged. Assumes the privilegedFlag is 0x8000 only. */
	di->flags |= privilegedFlag;

	/* Copy instruction meta. */
	di->meta = isi->meta;
	if (di->segment == 0) di->segment = R_NONE;

	/* Take into account the O_MEM base register for the mask. */
	if (di->base != R_NONE) di->usedRegistersMask |= _REGISTERTORCLASS[di->base];

	/* Copy CPU affected flags. */
	CONVERT_FLAGS_TO_EFLAGS(di, isi, modifiedFlagsMask);
	CONVERT_FLAGS_TO_EFLAGS(di, isi, testedFlagsMask);
	CONVERT_FLAGS_TO_EFLAGS(di, isi, undefinedFlagsMask);

	/* Calculate the size of the instruction we've just decoded. */
	di->size = (uint8_t)((ci->code - startCode) & 0xff);
	return DECRES_SUCCESS;

_Undecodable: /* If the instruction couldn't be decoded for some reason, drop the first byte. */
	memset(di, 0, sizeof(_DInst));
	di->base = R_NONE;

	di->size = 1;
	/* Clean prefixes just in case... */
	ps->usedPrefixes = 0;

	/* Special case for WAIT instruction: If it's dropped, you have to return a valid instruction! */
	if (*startCode == INST_WAIT_INDEX) {
		di->opcode = I_WAIT;
		META_SET_ISC(di, ISC_INTEGER);
		return DECRES_SUCCESS;
	}

	/* Mark that we didn't manage to decode the instruction well, caller will drop it. */
	return DECRES_INPUTERR;
}

/*
 * decode_internal
 *
 * supportOldIntr - Since now we work with new structure instead of the old _DecodedInst, we are still interested in backward compatibility.
 *                  So although, the array is now of type _DInst, we want to read it in jumps of the old array element's size.
 *                  This is in order to save memory allocation for conversion between the new and the old structures.
 *                  It really means we can do the conversion in-place now.
 */
_DecodeResult decode_internal(_CodeInfo* _ci, int supportOldIntr, _DInst result[], unsigned int maxResultCount, unsigned int* usedInstructionsCount)
{
	_PrefixState ps;
	unsigned int prefixSize;
	_CodeInfo ci;
	unsigned int features;
	unsigned int mfc;

	_OffsetType codeOffset = _ci->codeOffset;
	const uint8_t* code = _ci->code;
	int codeLen = _ci->codeLen;

	/*
	 * This is used for printing only, it is the real offset of where the whole instruction begins.
	 * We need this variable in addition to codeOffset, because prefixes might change the real offset an instruction begins at.
	 * So we keep track of both.
	 */
	_OffsetType startInstOffset = 0;

	const uint8_t* p;

	/* Current working decoded instruction in results. */
	unsigned int nextPos = 0;
	_DInst *pdi = NULL;

	_OffsetType addrMask = (_OffsetType)-1;

	_DecodeResult decodeResult;

#ifdef DISTORM_LIGHT
	supportOldIntr; /* Unreferenced. */
#endif

	if (_ci->features & DF_MAXIMUM_ADDR32) addrMask = 0xffffffff;
	else if (_ci->features & DF_MAXIMUM_ADDR16) addrMask = 0xffff;

	/* No entries are used yet. */
	*usedInstructionsCount = 0;
	ci.dt = _ci->dt;
	_ci->nextOffset = codeOffset;

	/* Decode instructions as long as we have what to decode/enough room in entries. */
	while (codeLen > 0) {

		/* startInstOffset holds the displayed offset of current instruction. */
		startInstOffset = codeOffset;

		memset(&ps, 0, (size_t)((char*)&ps.pfxIndexer[0] - (char*)&ps));
		memset(ps.pfxIndexer, PFXIDX_NONE, sizeof(int) * PFXIDX_MAX);
		ps.start = code;
		ps.last = code;
		prefixSize = 0;

		if (prefixes_is_valid(*code, ci.dt)) {
			prefixes_decode(code, codeLen, &ps, ci.dt);
			/* Count prefixes, start points to first prefix. */
			prefixSize = (unsigned int)(ps.last - ps.start);
			/*
			 * It might be that we will just notice that we ran out of bytes, or only prefixes
			 * so we will have to drop everything and halt.
			 * Also take into consideration of flow control instruction filter.
			 */
			codeLen -= prefixSize;
			if ((codeLen == 0) || (prefixSize == INST_MAXIMUM_SIZE)) {
				if (~_ci->features & DF_RETURN_FC_ONLY) {
					/* Make sure there is enough room. */
					if (nextPos + (ps.last - code) > maxResultCount) return DECRES_MEMORYERR;

					for (p = code; p < ps.last; p++, startInstOffset++) {
						/* Use next entry. */
#ifndef DISTORM_LIGHT
						if (supportOldIntr) {
							pdi = (_DInst*)((char*)result + nextPos * sizeof(_DecodedInst));
						}
						else
#endif /* DISTORM_LIGHT */
						{
							pdi = &result[nextPos];
						}
						nextPos++;
						memset(pdi, 0, sizeof(_DInst));

						pdi->flags = FLAG_NOT_DECODABLE;
						pdi->imm.byte = *p;
						pdi->size = 1;
						pdi->addr = startInstOffset & addrMask;
					}
					*usedInstructionsCount = nextPos; /* Include them all. */
				}
				if (codeLen == 0) break; /* Bye bye, out of bytes. */
			}
			code += prefixSize;
			codeOffset += prefixSize;

			/* If we got only prefixes continue to next instruction. */
			if (prefixSize == INST_MAXIMUM_SIZE) continue;
		}

		/*
		 * Now we decode the instruction and only then we do further prefixes handling.
		 * This is because the instruction could not be decoded at all, or an instruction requires
		 * a mandatory prefix, or some of the prefixes were useless, etc...

		 * Even if there were a mandatory prefix, we already took into account its size as a normal prefix.
		 * so prefixSize includes that, and the returned size in pdi is simply the size of the real(=without prefixes) instruction.
		 */
		if (ci.dt == Decode64Bits) {
			if (ps.decodedPrefixes & INST_PRE_REX) {
				/* REX prefix must precede first byte of instruction. */
				if (ps.rexPos != (code - 1)) {
					ps.decodedPrefixes &= ~INST_PRE_REX;
					ps.prefixExtType = PET_NONE;
					prefixes_ignore(&ps, PFXIDX_REX);
				}
				/*
				 * We will disable operand size prefix,
				 * if it exists only after decoding the instruction, since it might be a mandatory prefix.
				 * This will be done after calling inst_lookup in decode_inst.
				 */
			}
			/* In 64 bits, segment overrides of CS, DS, ES and SS are ignored. So don't take'em into account. */
			if (ps.decodedPrefixes & INST_PRE_SEGOVRD_MASK32) {
				ps.decodedPrefixes &= ~INST_PRE_SEGOVRD_MASK32;
				prefixes_ignore(&ps, PFXIDX_SEG);
			}
		}

		/* Make sure there is at least one more entry to use, for the upcoming instruction. */
		if (nextPos + 1 > maxResultCount) return DECRES_MEMORYERR;
#ifndef DISTORM_LIGHT
		if (supportOldIntr) {
			pdi = (_DInst*)((char*)result + nextPos * sizeof(_DecodedInst));
		}
		else
#endif /* DISTORM_LIGHT */
		{
			pdi = &result[nextPos];
		}
		nextPos++;

		/*
		 * The reason we copy these two again is because we have to keep track on the input ourselves.
		 * There might be a case when an instruction is invalid, and then it will be counted as one byte only.
		 * But that instruction already read a byte or two from the stream and only then returned the error.
		 * Thus, we end up unsynchronized on the stream.
		 * This way, we are totally safe, because we keep track after the call to decode_inst, using the returned size.
		 */
		ci.code = code;
		ci.codeLen = codeLen;
		/* Nobody uses codeOffset in the decoder itself, so spare it. */

		decodeResult = decode_inst(&ci, &ps, pdi);

		/* See if we need to filter this instruction. */
		if ((_ci->features & DF_RETURN_FC_ONLY) && (META_GET_FC(pdi->meta) == FC_NONE)) decodeResult = DECRES_FILTERED;

		/* Set address to the beginning of the instruction. */
		pdi->addr = startInstOffset & addrMask;
		/* pdi->disp &= addrMask; */

		if ((decodeResult == DECRES_INPUTERR) && (ps.decodedPrefixes & INST_PRE_VEX)) {
			if (ps.prefixExtType == PET_VEX3BYTES) {
				prefixSize -= 2;
				codeLen += 2;
			} else if (ps.prefixExtType == PET_VEX2BYTES) {
				prefixSize -= 1;
				codeLen += 1;
			}
			ps.last = ps.start + prefixSize - 1;
			code = ps.last + 1;
			codeOffset = startInstOffset + prefixSize;
		} else {
			/* Advance to next instruction. */
			codeLen -= pdi->size;
			codeOffset += pdi->size;
			code += pdi->size;

			/* Instruction's size should include prefixes. */
			pdi->size += (uint8_t)prefixSize;
		}

		/* Drop all prefixes and the instruction itself, because the instruction wasn't successfully decoded. */
		if ((decodeResult == DECRES_INPUTERR) && (~_ci->features & DF_RETURN_FC_ONLY)) {
			nextPos--; /* Undo last result. */
			if ((prefixSize + 1) > 0) { /* 1 for the first instruction's byte. */
				if ((nextPos + prefixSize + 1) > maxResultCount) return DECRES_MEMORYERR;

				for (p = ps.start; p < ps.last + 1; p++, startInstOffset++) {
					/* Use next entry. */
#ifndef DISTORM_LIGHT
					if (supportOldIntr) {
						pdi = (_DInst*)((char*)result + nextPos * sizeof(_DecodedInst));
					}
					else
#endif /* DISTORM_LIGHT */
					{
						pdi = &result[nextPos];
					}
					nextPos++;

					memset(pdi, 0, sizeof(_DInst));
					pdi->flags = FLAG_NOT_DECODABLE;
					pdi->imm.byte = *p;
					pdi->size = 1;
					pdi->addr = startInstOffset & addrMask;
				}
			}
		} else if (decodeResult == DECRES_FILTERED) nextPos--; /* Return it to pool, since it was filtered. */

		/* Alright, the caller can read, at least, up to this one. */
		*usedInstructionsCount = nextPos;
		/* Fix next offset. */
		_ci->nextOffset = codeOffset;

		/* Check whether we need to stop on any flow control instruction. */
		features = _ci->features;
		mfc = META_GET_FC(pdi->meta);
		if ((decodeResult == DECRES_SUCCESS) && (features & DF_STOP_ON_FLOW_CONTROL)) {
			if (((features & DF_STOP_ON_CALL) && (mfc == FC_CALL)) ||
				((features & DF_STOP_ON_RET) && (mfc == FC_RET)) ||
				((features & DF_STOP_ON_SYS) && (mfc == FC_SYS)) ||
				((features & DF_STOP_ON_UNC_BRANCH) && (mfc == FC_UNC_BRANCH)) ||
				((features & DF_STOP_ON_CND_BRANCH) && (mfc == FC_CND_BRANCH)) ||
				((features & DF_STOP_ON_INT) && (mfc == FC_INT)) ||
				((features & DF_STOP_ON_CMOV) && (mfc == FC_CMOV)))
				return DECRES_SUCCESS;
		}
	}

	return DECRES_SUCCESS;
}
