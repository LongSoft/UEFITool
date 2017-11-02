/*
operands.c

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


#include "config.h"
#include "operands.h"
#include "x86defs.h"
#include "insts.h"
#include "../include/mnemonics.h"


/* Maps a register to its register-class mask. */
uint16_t _REGISTERTORCLASS[] = /* Based on _RegisterType enumeration! */
{RM_AX, RM_CX, RM_DX, RM_BX, RM_SP, RM_BP, RM_SI, RM_DI, 0, 0, 0, 0, 0, 0, 0, 0,
 RM_AX, RM_CX, RM_DX, RM_BX, RM_SP, RM_BP, RM_SI, RM_DI, 0, 0, 0, 0, 0, 0, 0, 0,
 RM_AX, RM_CX, RM_DX, RM_BX, RM_SP, RM_BP, RM_SI, RM_DI, 0, 0, 0, 0, 0, 0, 0, 0,
 RM_AX, RM_CX, RM_DX, RM_BX, RM_AX, RM_CX, RM_DX, RM_BX, 0, 0, 0, 0, 0, 0, 0, 0,
 RM_SP, RM_BP, RM_SI, RM_DI,
 0, 0, 0, 0, 0, 0,
 0,
 RM_FPU, RM_FPU, RM_FPU, RM_FPU, RM_FPU, RM_FPU, RM_FPU, RM_FPU,
 RM_MMX, RM_MMX, RM_MMX, RM_MMX, RM_MMX, RM_MMX, RM_MMX, RM_MMX,
 RM_SSE, RM_SSE, RM_SSE, RM_SSE, RM_SSE, RM_SSE, RM_SSE, RM_SSE, RM_SSE, RM_SSE, RM_SSE, RM_SSE, RM_SSE, RM_SSE, RM_SSE, RM_SSE,
 RM_AVX, RM_AVX, RM_AVX, RM_AVX, RM_AVX, RM_AVX, RM_AVX, RM_AVX, RM_AVX, RM_AVX, RM_AVX, RM_AVX, RM_AVX, RM_AVX, RM_AVX, RM_AVX,
 RM_CR, 0, RM_CR, RM_CR, RM_CR, 0, 0, 0, RM_CR,
 RM_DR, RM_DR, RM_DR, RM_DR, 0, 0, RM_DR, RM_DR
};

typedef enum {OPERAND_SIZE_NONE = 0, OPERAND_SIZE8, OPERAND_SIZE16, OPERAND_SIZE32, OPERAND_SIZE64, OPERAND_SIZE80, OPERAND_SIZE128, OPERAND_SIZE256} _OperandSizeType;
static uint16_t _OPSIZETOINT[] = {0, 8, 16, 32, 64, 80, 128, 256};

/* A helper function to fix the 8 bits register if REX is used (to support SIL, DIL, etc). */
static unsigned int _FASTCALL_ operands_fix_8bit_rex_base(unsigned int reg)
{
	if ((reg >= 4) && (reg < 8)) return reg + REGS8_REX_BASE - 4;
	return reg + REGS8_BASE;
}

/* A helper function to set operand's type and size. */
static void _FASTCALL_ operands_set_ts(_Operand* op, _OperandType type, uint16_t size)
{
	op->type = type;
	op->size = size;
}

/* A helper function to set operand's type, size and index. */
static void _FASTCALL_ operands_set_tsi(_Operand* op, _OperandType type, uint16_t size, unsigned int index)
{
	op->type = type;
	op->index = (uint8_t)index;
	op->size = size;
}

/* A helper function to read an unsigned integer from the stream safely. */
static int _FASTCALL_ read_stream_safe_uint(_CodeInfo* ci, void* result, unsigned int size)
{
	ci->codeLen -= size;
	if (ci->codeLen < 0) return FALSE;
	switch (size)
	{
		case 1: *(uint8_t*)result = *(uint8_t*)ci->code; break;
		case 2: *(uint16_t*)result = RUSHORT(ci->code); break;
		case 4: *(uint32_t*)result = RULONG(ci->code); break;
		case 8: *(uint64_t*)result = RULLONG(ci->code); break;
	}
	ci->code += size;
	return TRUE;
}

/* A helper function to read a signed integer from the stream safely. */
static int _FASTCALL_ read_stream_safe_sint(_CodeInfo* ci, int64_t* result, unsigned int size)
{
	ci->codeLen -= size;
	if (ci->codeLen < 0) return FALSE;
	switch (size)
	{
		case 1: *result = *(int8_t*)ci->code; break;
		case 2: *result = RSHORT(ci->code); break;
		case 4: *result = RLONG(ci->code); break;
		case 8: *result = RLLONG(ci->code); break;
	}
	ci->code += size;
	return TRUE;
}

/*
 * SIB decoding is the most confusing part when decoding IA-32 instructions.
 * This explanation should clear up some stuff.
 *
 * ! When base == 5, use EBP as the base register !
 * if (rm == 4) {
 *	if mod == 01, decode SIB byte and ALSO read a 8 bits displacement.
 *	if mod == 10, decode SIB byte and ALSO read a 32 bits displacement.
 *	if mod == 11 <-- EXCEPTION, this is a general-purpose register and mustn't lead to SIB decoding!
 *	; So far so good, now the confusing part comes in with mod == 0 and base=5, but no worry.
 *	if (mod == 00) {
 *	 decode SIB byte WITHOUT any displacement.
 *	 EXCEPTION!!! when base == 5, read a 32 bits displacement, but this time DO NOT use (EBP) BASE at all!
 *	}
 *
 *	NOTE: base could specify None (no base register) if base==5 and mod==0, but then you also need DISP32.
 * }
 */
static void operands_extract_sib(_DInst* di, _OperandNumberType opNum,
                                 _PrefixState* ps, _DecodeType effAdrSz,
                                 unsigned int sib, unsigned int mod)
{
	unsigned int scale = 0, index = 0, base = 0;
	unsigned int vrex = ps->vrex;
	uint8_t* pIndex = NULL;

	_Operand* op = &di->ops[opNum];

	/*
	 * SIB bits:
	 * |7---6-5----3-2---0|
	 * |SCALE| INDEX| BASE|
	 * |------------------|
	 */
	scale = (sib >> 6) & 3;
	index = (sib >> 3) & 7;
	base = sib & 7;

	/*
	 * The following fields: base/index/scale/disp8/32 are ALL optional by specific rules!
	 * The idea here is to keep the indirection as a simple-memory type.
	 * Because the base is optional, and we might be left with only one index.
	 * So even if there's a base but no index, or vice versa, we end up with one index register.
	 */

	/* In 64 bits the REX prefix might affect the index of the SIB byte. */
	if (vrex & PREFIX_EX_X) {
		ps->usedPrefixes |= INST_PRE_REX;
		index += EX_GPR_BASE;
	}

	if (index == 4) { /* No index is used. Use SMEM. */
		op->type = O_SMEM;
		pIndex = &op->index;
	} else {
		op->type = O_MEM;
		pIndex = &di->base;
		/* No base, unless it is updated below. E.G: [EAX*4] has no base reg. */
	}

	if (base != 5) {
		if (vrex & PREFIX_EX_B) ps->usedPrefixes |= INST_PRE_REX;
		*pIndex = effAdrSz == Decode64Bits ? REGS64_BASE : REGS32_BASE;
		*pIndex += (uint8_t)(base + ((vrex & PREFIX_EX_B) ? EX_GPR_BASE : 0));
	} else if (mod != 0) {
		/*
		 * if base == 5 then you have to decode according to MOD.
		 * mod(00) - disp32.
		 * mod(01) - disp8 + rBP
		 * mod(10) - disp32 + rBP
		 * mod(11) - not possible, it's a general-purpose register.
		 */

		if (vrex & PREFIX_EX_B) ps->usedPrefixes |= INST_PRE_REX;
		if (effAdrSz == Decode64Bits) *pIndex = REGS64_BASE + 5 + ((vrex & PREFIX_EX_B) ? EX_GPR_BASE : 0);
		else *pIndex = REGS32_BASE + 5 + ((vrex & PREFIX_EX_B) ? EX_GPR_BASE : 0);
	} else if (index == 4) {
		 /* 32bits displacement only. */
		op->type = O_DISP;
		return;
	}

	if (index != 4) { /* In 64 bits decoding mode, if index == R12, it's valid! */
		if (effAdrSz == Decode64Bits) op->index = (uint8_t)(REGS64_BASE + index);
		else op->index = (uint8_t)(REGS32_BASE + index);
		di->scale = scale != 0 ? (1 << scale) : 0;
	}
}

/*
 * This seems to be the hardest part in decoding the operands.
 * If you take a look carefully at Table 2-2. 32-Bit Addressing Forms with the ModR/M Byte,
 * you will understand it's easy to decode the operands.

 * First we check the DT, so we can decide according to which Table in the documentation we are supposed to decode.
 * Then we follow the specific table whether it's 16 bits or 32/64 bits.

 * Don't forget that Operand Size AND Address Size prefixes may change the decoding!

 * Some instructions force the use of RM16 or other specific types, so take it into account.
 */
static int operands_extract_modrm(_CodeInfo* ci,
                                  _DInst* di, _OpType type,
                                  _OperandNumberType opNum, _PrefixState* ps,
                                  _DecodeType effOpSz, _DecodeType effAdrSz,
                                  int* lockableInstruction, unsigned int mod, unsigned int rm,
                                  _iflags instFlags)
{
	unsigned int vrex = ps->vrex, sib = 0, base = 0;
	_Operand* op = &di->ops[opNum];
	uint16_t size = 0;

	if (mod == 3) {
		/*
		 * General-purpose register is handled the same way in 16/32/64 bits decoding modes.
		 * NOTE!! that we have to override the size of the register, since it was set earlier as Memory and not Register!
		 */
		op->type = O_REG;
		/* Start with original size which was set earlier, some registers have same size of memory and depend on it. */
		size = op->size;
		switch(type)
		{
			case OT_RFULL_M16:
			case OT_RM_FULL:
				switch (effOpSz)
				{
					case Decode16Bits:
						ps->usedPrefixes |= INST_PRE_OP_SIZE;
						if (vrex & PREFIX_EX_B) {
							ps->usedPrefixes |= INST_PRE_REX;
							rm += EX_GPR_BASE;
						}
						size = 16;
						rm += REGS16_BASE;
					break;
					case Decode32Bits:
						ps->usedPrefixes |= INST_PRE_OP_SIZE;
						if (vrex & PREFIX_EX_B) {
							ps->usedPrefixes |= INST_PRE_REX;
							rm += EX_GPR_BASE;
						}
						size = 32;
						rm += REGS32_BASE;
					break;
					case Decode64Bits:
						/* A fix for SMSW RAX which use the REX prefix. */
						if (type == OT_RFULL_M16) ps->usedPrefixes |= INST_PRE_REX;
						/* CALL NEAR/PUSH/POP defaults to 64 bits. --> INST_64BITS, REX isn't required, thus ignored anyways. */
						if (instFlags & INST_PRE_REX) ps->usedPrefixes |= INST_PRE_REX;
						/* Include REX if used for REX.B. */
						if (vrex & PREFIX_EX_B) {
							ps->usedPrefixes |= INST_PRE_REX;
							rm += EX_GPR_BASE;
						}
						size = 64;
						rm += REGS64_BASE;
					break;
				}
			break;
			case OT_R32_64_M8:
			/* FALL THROUGH, decode 32 or 64 bits register. */
			case OT_R32_64_M16:
			/* FALL THROUGH, decode 32 or 64 bits register. */
			case OT_RM32_64: /* Take care specifically in MOVNTI/MOVD/CVT's instructions, making it _REG64 with REX or if they are promoted. */
				if (vrex & PREFIX_EX_B) {
					ps->usedPrefixes |= INST_PRE_REX;
					rm += EX_GPR_BASE;
				}
				/* Is it a promoted instruction? (only INST_64BITS is set and REX isn't required.) */
				if ((ci->dt == Decode64Bits) && ((instFlags & (INST_64BITS | INST_PRE_REX)) == INST_64BITS)) {
					size = 64;
					rm += REGS64_BASE;
					break;
				}
				/* Give a chance to REX.W. Because if it was a promoted instruction we don't care about REX.W anyways. */
				if (vrex & PREFIX_EX_W) {
					ps->usedPrefixes |= INST_PRE_REX;
					size = 64;
					rm += REGS64_BASE;
				} else {
					size = 32;
					rm += REGS32_BASE;
				}
			break;
			case OT_RM16_32: /* Used only with MOVZXD instruction to support 16 bits operand. */
				if (vrex & PREFIX_EX_B) {
					ps->usedPrefixes |= INST_PRE_REX;
					rm += EX_GPR_BASE;
				}
				/* Is it 16 bits operand size? */
				if (ps->decodedPrefixes & INST_PRE_OP_SIZE) {
					ps->usedPrefixes |= INST_PRE_OP_SIZE;
					size = 16;
					rm += REGS16_BASE;
				} else {
					size = 32;
					rm += REGS32_BASE;
				}
			break;
			case OT_RM16:
				if (vrex & PREFIX_EX_B) {
					ps->usedPrefixes |= INST_PRE_REX;
					rm += EX_GPR_BASE;
				}
				rm += REGS16_BASE;
			break;
			case OT_RM8:
				if (ps->prefixExtType == PET_REX) {
					ps->usedPrefixes |= INST_PRE_REX;
					rm = operands_fix_8bit_rex_base(rm + ((vrex & PREFIX_EX_B) ? EX_GPR_BASE : 0));
				} else rm += REGS8_BASE;
			break;
			case OT_MM32:
			case OT_MM64:
				/* MMX doesn't support extended registers. */
				size = 64;
				rm += MMXREGS_BASE;
			break;

			case OT_XMM16:
			case OT_XMM32:
			case OT_XMM64:
			case OT_XMM128:
				if (vrex & PREFIX_EX_B) {
					ps->usedPrefixes |= INST_PRE_REX;
					rm += EX_GPR_BASE;
				}
				size = 128;
				rm += SSEREGS_BASE;
			break;

			case OT_RM32:
			case OT_R32_M8:
			case OT_R32_M16:
				if (vrex & PREFIX_EX_B) {
					ps->usedPrefixes |= INST_PRE_REX;
					rm += EX_GPR_BASE;
				}
				size = 32;
				rm += REGS32_BASE;
			break;

			case OT_YMM256:
				if (vrex & PREFIX_EX_B) rm += EX_GPR_BASE;
				rm += AVXREGS_BASE;
			break;
			case OT_YXMM64_256:
			case OT_YXMM128_256:
				if (vrex & PREFIX_EX_B) rm += EX_GPR_BASE;
				if (vrex & PREFIX_EX_L) {
					size = 256;
					rm += AVXREGS_BASE;
				} else {
					size = 128;
					rm += SSEREGS_BASE;
				}
			break;
			case OT_WXMM32_64:
			case OT_LXMM64_128:
				if (vrex & PREFIX_EX_B) rm += EX_GPR_BASE;
				size = 128;
				rm += SSEREGS_BASE;
			break;

			case OT_WRM32_64:
			case OT_REG32_64_M8:
			case OT_REG32_64_M16:
				if (vrex & PREFIX_EX_B) rm += EX_GPR_BASE;
				if (vrex & PREFIX_EX_W) {
					size = 64;
					rm += REGS64_BASE;
				} else {
					size = 32;
					rm += REGS32_BASE;
				}
			break;

			default: return FALSE;
		}
		op->size = size;
		op->index = (uint8_t)rm;
		return TRUE;
	}

	/* Memory indirection decoding ahead:) */

	ps->usedPrefixes |= INST_PRE_ADDR_SIZE;
	if (lockableInstruction && (ps->decodedPrefixes & INST_PRE_LOCK)) *lockableInstruction = TRUE;

	if (effAdrSz == Decode16Bits) {
		/* Decoding according to Table 2-1. (16 bits) */
		if ((mod == 0) && (rm == 6)) {
			/* 6 is a special case - only 16 bits displacement. */
			op->type = O_DISP;
			di->dispSize = 16;
			if (!read_stream_safe_sint(ci, (int64_t*)&di->disp, sizeof(int16_t))) return FALSE;
		} else {
			/*
			 * Create the O_MEM for 16 bits indirection that requires 2 registers, E.G: [BS+SI].
			 * or create O_SMEM for a single register indirection, E.G: [BP].
			 */
			static uint8_t MODS[] = {R_BX, R_BX, R_BP, R_BP, R_SI, R_DI, R_BP, R_BX};
			static uint8_t MODS2[] = {R_SI, R_DI, R_SI, R_DI};
			if (rm < 4) {
				op->type = O_MEM;
				di->base = MODS[rm];
				op->index = MODS2[rm];
			} else {
				op->type = O_SMEM;
				op->index = MODS[rm];
			}

			if (mod == 1) { /* 8 bits displacement + indirection */
				di->dispSize = 8;
				if (!read_stream_safe_sint(ci, (int64_t*)&di->disp, sizeof(int8_t))) return FALSE;
			} else if (mod == 2) { /* 16 bits displacement + indirection */
				di->dispSize = 16;
				if (!read_stream_safe_sint(ci, (int64_t*)&di->disp, sizeof(int16_t))) return FALSE;
			}
		}

		if ((rm == 2) || (rm == 3) || ((rm == 6) && (mod != 0))) {
			/* BP's default segment is SS, so ignore it. */
			prefixes_use_segment(INST_PRE_SS, ps, ci->dt, di);
		} else {
			/* Ignore default DS segment. */
			prefixes_use_segment(INST_PRE_DS, ps, ci->dt, di);
		}
	} else { /* Decode32Bits or Decode64Bits! */
		/* Remember that from a 32/64 bits ModR/M byte a SIB byte could follow! */
		if ((mod == 0) && (rm == 5)) {

			/* 5 is a special case - only 32 bits displacement, or RIP relative. */
			di->dispSize = 32;
			if (!read_stream_safe_sint(ci, (int64_t*)&di->disp, sizeof(int32_t))) return FALSE;

			if (ci->dt == Decode64Bits) {
				/* In 64 bits decoding mode depsite of the address size, a RIP-relative address it is. */
				op->type = O_SMEM;
				op->index = R_RIP;
				di->flags |= FLAG_RIP_RELATIVE;
			} else {
				/* Absolute address: */
				op->type = O_DISP;
			}
		} else {
			if (rm == 4) {
				/* 4 is a special case - SIB byte + disp8/32 follows! */
				/* Read SIB byte. */
				if (!read_stream_safe_uint(ci, &sib, sizeof(int8_t))) return FALSE;
				operands_extract_sib(di, opNum, ps, effAdrSz, sib, mod);
			} else {
				op->type = O_SMEM;
				if (vrex & PREFIX_EX_B) {
					ps->usedPrefixes |= INST_PRE_REX;
					rm += EX_GPR_BASE;
				}

				if (effAdrSz == Decode64Bits) op->index = (uint8_t)(REGS64_BASE + rm);
				else op->index = (uint8_t)(REGS32_BASE + rm);
			}

			if (mod == 1) {
				di->dispSize = 8;
				if (!read_stream_safe_sint(ci, (int64_t*)&di->disp, sizeof(int8_t))) return FALSE;
			} else if ((mod == 2) || ((sib & 7) == 5)) { /* If there is no BASE, read DISP32! */
				di->dispSize = 32;
				if (!read_stream_safe_sint(ci, (int64_t*)&di->disp, sizeof(int32_t))) return FALSE;
			}
		}

		/* Get the base register. */
		base = op->index;
		if (di->base != R_NONE) base = di->base;
		else if (di->scale >= 2) base = 0; /* If it's only an index but got scale, it's still DS. */
		/* Default for EBP/ESP is SS segment. 64 bits mode ignores DS anyway. */
		if ((base == R_EBP) || (base == R_ESP)) prefixes_use_segment(INST_PRE_SS, ps, ci->dt, di);
		else prefixes_use_segment(INST_PRE_DS, ps, ci->dt, di);
	}

	return TRUE;
}


/*
 * This function is reponsible to textually format a required operand according to its type.
 * It is vital to understand that there are other operands than what the ModR/M byte specifies.

 * Only by decoding the operands of an instruction which got a LOCK prefix, we could tell whether it may use the LOCK prefix.
 * According to Intel, LOCK prefix must precede some specific instructions AND in their memory destination operand form (which means first operand).
 * LOCK INC EAX, would generate an exception, but LOCK INC [EAX] is alright.
 * Also LOCK ADD BX, [BP] would generate an exception.

 * Return code:
 * TRUE - continue parsing the instruction and its operands, everything went right 'till now.
 * FALSE - not enough bytes, or invalid operands.
 */

int operands_extract(_CodeInfo* ci, _DInst* di, _InstInfo* ii,
                     _iflags instFlags, _OpType type, _OperandNumberType opNum,
                     unsigned int modrm, _PrefixState* ps, _DecodeType effOpSz,
                     _DecodeType effAdrSz, int* lockableInstruction)
{
	int ret = 0;
	unsigned int mod = 0, reg = 0, rm = 0, vexV = ps->vexV;
	unsigned int vrex = ps->vrex, typeHandled = TRUE;
	_Operand* op = &di->ops[opNum];

	/* Used to indicate the size of the MEMORY INDIRECTION only. */
	_OperandSizeType opSize = OPERAND_SIZE_NONE;

	/*
	 * ModRM bits:
	 * |7-6-5--------3-2-0|
	 * |MOD|REG/OPCODE|RM |
	 * |------------------|
	 */
	mod = (modrm >> 6) & 3; /* Mode(register-indirection, disp8+reg+indirection, disp16+reg+indirection, general-purpose register) */
	reg = (modrm >> 3) & 7; /* Register(could be part of the opcode itself or general-purpose register) */
	rm = modrm & 7; /* Specifies which general-purpose register or disp+reg to use. */

	/* -- Memory Indirection Operands (that cannot be a general purpose register) -- */
	switch (type)
	{
		case OT_MEM64_128: /* Used only by CMPXCHG8/16B. */
			/* Make a specific check when the type is OT_MEM64_128 since the lockable CMPXCHG8B uses this one... */
			if (lockableInstruction && (ps->decodedPrefixes & INST_PRE_LOCK)) *lockableInstruction = TRUE;
			if (effOpSz == Decode64Bits) {
				ps->usedPrefixes |= INST_PRE_REX;
				opSize = OPERAND_SIZE128;
			} else opSize = OPERAND_SIZE64;
		break;
		case OT_MEM32: opSize = OPERAND_SIZE32; break;
		case OT_MEM32_64:
			/* Used by MOVNTI. Default size is 32bits, 64bits with REX. */
			if (effOpSz == Decode64Bits) {
				ps->usedPrefixes |= INST_PRE_REX;
				opSize = OPERAND_SIZE64;
			} else opSize = OPERAND_SIZE32;
		break;
		case OT_MEM64: opSize = OPERAND_SIZE64; break;
		case OT_MEM128: opSize = OPERAND_SIZE128; break;
		case OT_MEM16_FULL: /* The size indicates about the second item of the pair. */
			switch (effOpSz)
			{
				case Decode16Bits:
					ps->usedPrefixes |= INST_PRE_OP_SIZE;
					opSize = OPERAND_SIZE16;
				break;
				case Decode32Bits:
					ps->usedPrefixes |= INST_PRE_OP_SIZE;
					opSize = OPERAND_SIZE32;
				break;
				case Decode64Bits:
					/* Mark usage of REX only if it was required. */
					if ((instFlags & (INST_64BITS | INST_PRE_REX)) == (INST_64BITS | INST_PRE_REX)) ps->usedPrefixes |= INST_PRE_REX;
					opSize = OPERAND_SIZE64;
				break;
			}
		break;
		case OT_MEM16_3264: /* The size indicates about the second item of the pair. */
			if (ci->dt == Decode64Bits) opSize = OPERAND_SIZE64;
			else opSize = OPERAND_SIZE32;
		break;
		case OT_MEM_OPT:
			/* Since the MEM is optional, only when mod != 3, then return true as if the operand was alright. */
			if (mod == 0x3) return TRUE;
		break;
		case OT_FPUM16: opSize = OPERAND_SIZE16; break;
		case OT_FPUM32: opSize = OPERAND_SIZE32; break;
		case OT_FPUM64: opSize = OPERAND_SIZE64; break;
		case OT_FPUM80: opSize = OPERAND_SIZE80; break;
		case OT_LMEM128_256:
			if (vrex & PREFIX_EX_L) opSize = OPERAND_SIZE256;
			else opSize = OPERAND_SIZE128;
		break;
		case OT_MEM: /* Size is unknown, but still handled. */ break;
		default: typeHandled = FALSE; break;
	}
	if (typeHandled) {
		/* All of the above types can't use a general-purpose register (a MOD of 3)!. */
		if (mod == 0x3) {
			if (lockableInstruction) *lockableInstruction = FALSE;
			return FALSE;
		}
		op->size = _OPSIZETOINT[opSize];
		ret = operands_extract_modrm(ci, di, type, opNum, ps, effOpSz, effAdrSz, lockableInstruction, mod, rm, instFlags);
		if ((op->type == O_REG) || (op->type == O_SMEM) || (op->type == O_MEM)) {
			di->usedRegistersMask |= _REGISTERTORCLASS[op->index];
		}
		return ret;
	}

	/* -- Memory Indirection Operands (that can be a register) -- */
	typeHandled = TRUE;
	switch (type)
	{
		case OT_RM_FULL:
			ps->usedPrefixes |= INST_PRE_OP_SIZE;
			/* PUSH/JMP/CALL are automatically promoted to 64 bits! */
			if (effOpSz == Decode32Bits) {
				opSize = OPERAND_SIZE32;
				break;
			} else if (effOpSz == Decode64Bits) {
				/* Mark usage of REX only if it was required. */
				if ((instFlags & INST_64BITS) == 0) ps->usedPrefixes |= INST_PRE_REX;
				opSize = OPERAND_SIZE64;
				break;
			}
			/* FALL THROUGH BECAUSE dt==Decoded16Bits @-<----*/
		case OT_RM16:
			/* If we got here not from OT_RM16, then the prefix was used. */
			if (type != OT_RM16) ps->usedPrefixes |= INST_PRE_OP_SIZE;
			opSize = OPERAND_SIZE16;
		break;
		case OT_RM32_64:
			/* The default size is 32, which can be 64 with a REX only. */
			if (effOpSz == Decode64Bits) {
				opSize = OPERAND_SIZE64;
				/* Mark REX prefix as used if non-promoted instruction. */
				if ((instFlags & (INST_64BITS | INST_PRE_REX)) == (INST_64BITS | INST_PRE_REX)) {
					ps->usedPrefixes |= INST_PRE_REX;
				}
			} else opSize = OPERAND_SIZE32;
		break;
		case OT_RM16_32:
			/* Ignore REX, it's either 32 or 16 bits RM. */
			if (ps->decodedPrefixes & INST_PRE_OP_SIZE) {
				ps->usedPrefixes |= INST_PRE_OP_SIZE;
				/* Assume: We are in 64bits when we have this operand used. */
				opSize = OPERAND_SIZE16;
			} else opSize = OPERAND_SIZE32;
		break;
		case OT_WXMM32_64:
		case OT_WRM32_64:
			if (vrex & PREFIX_EX_W) opSize = OPERAND_SIZE64;
			else opSize = OPERAND_SIZE32;
		break;
		case OT_YXMM64_256:
			if (vrex & PREFIX_EX_L) opSize = OPERAND_SIZE256;
			else opSize = OPERAND_SIZE64;
		break;
		case OT_YXMM128_256:
			if (vrex & PREFIX_EX_L) opSize = OPERAND_SIZE256;
			else opSize = OPERAND_SIZE128;
		break;
		case OT_LXMM64_128:
			if (vrex & PREFIX_EX_L) opSize = OPERAND_SIZE128;
			else opSize = OPERAND_SIZE64;
		break;
		case OT_RFULL_M16:
			ps->usedPrefixes |= INST_PRE_OP_SIZE;
			opSize = OPERAND_SIZE16;
		break;

		case OT_RM8:
		case OT_R32_M8:
		case OT_R32_64_M8:
		case OT_REG32_64_M8:
			opSize = OPERAND_SIZE8;
		break;

		case OT_XMM16:
		case OT_R32_M16:
		case OT_R32_64_M16:
		case OT_REG32_64_M16:
			opSize = OPERAND_SIZE16;
		break;

		case OT_RM32:
		case OT_MM32:
		case OT_XMM32:
			opSize = OPERAND_SIZE32;
		break;

		case OT_MM64:
		case OT_XMM64:
			opSize = OPERAND_SIZE64;
		break;

		case OT_XMM128: opSize = OPERAND_SIZE128; break;
		case OT_YMM256: opSize = OPERAND_SIZE256; break;
		default: typeHandled = FALSE; break;
	}
	if (typeHandled) {
		/* Fill size of memory dereference for operand. */
		op->size = _OPSIZETOINT[opSize];
		ret = operands_extract_modrm(ci, di, type, opNum, ps, effOpSz, effAdrSz, lockableInstruction, mod, rm, instFlags);
		if ((op->type == O_REG) || (op->type == O_SMEM) || (op->type == O_MEM)) {
			di->usedRegistersMask |= _REGISTERTORCLASS[op->index];
		}
		return ret;
	}

	/* Simple operand type (no ModRM byte). */
	switch (type)
	{
		case OT_IMM8:
			operands_set_ts(op, O_IMM, 8);
			if (!read_stream_safe_uint(ci, &di->imm.byte, sizeof(int8_t))) return FALSE;
		break;
		case OT_IMM_FULL: /* 16, 32 or 64, depends on prefixes. */
			if (effOpSz == Decode16Bits) {
				ps->usedPrefixes |= INST_PRE_OP_SIZE;
				/* FALL THROUGH */
		case OT_IMM16: /* Force 16 bits imm. */
			operands_set_ts(op, O_IMM, 16);
			if (!read_stream_safe_uint(ci, &di->imm.word, sizeof(int16_t))) return FALSE;
		break;
			/*
			 * Extension: MOV imm64, requires REX.
			 * Make sure it needs the REX.
			 * REX must be present because op size function takes it into consideration.
			 */
			} else if ((effOpSz == Decode64Bits) &&
				        ((instFlags & (INST_64BITS | INST_PRE_REX)) == (INST_64BITS | INST_PRE_REX))) {
				ps->usedPrefixes |= INST_PRE_REX;

				operands_set_ts(op, O_IMM, 64);
				if (!read_stream_safe_uint(ci, &di->imm.qword, sizeof(int64_t))) return FALSE;
				break;
			} else ps->usedPrefixes |= INST_PRE_OP_SIZE;
			/* FALL THROUGH BECAUSE dt==Decoded32Bits @-<----*/
		case OT_IMM32:
			op->type = O_IMM;
			if (ci->dt == Decode64Bits) {
				/* Imm32 is sign extended to 64 bits! */
				op->size = 64;
				di->flags |= FLAG_IMM_SIGNED;
				if (!read_stream_safe_sint(ci, &di->imm.sqword, sizeof(int32_t))) return FALSE;
			} else {
				op->size = 32;
				if (!read_stream_safe_uint(ci, &di->imm.dword, sizeof(int32_t))) return FALSE;
			}
		break;
		case OT_SEIMM8: /* Sign extended immediate. */
			/*
			 * PUSH SEIMM8 can be prefixed by operand size:
			 * Input stream: 66, 6a, 55
			 * 64bits DT: push small 55
			 * 32bits DT: push small 55
			 * 16bits DT: push large 55
			 * small/large indicates the size of the eSP pointer advancement.
			 * Check the instFlags (ii->flags) if it can be operand-size-prefixed and if the prefix exists.
			 */
			op->type = O_IMM;
			if ((instFlags & INST_PRE_OP_SIZE) && (ps->decodedPrefixes & INST_PRE_OP_SIZE)) {
				ps->usedPrefixes |= INST_PRE_OP_SIZE;
				switch (ci->dt)
				{
					case Decode16Bits: op->size = 32; break;
					case Decode32Bits:
					case Decode64Bits:
						op->size = 16;
					break;
				}
			} else op->size = 8;
			di->flags |= FLAG_IMM_SIGNED;
			if (!read_stream_safe_sint(ci, &di->imm.sqword, sizeof(int8_t))) return FALSE;
		break;
		case OT_IMM16_1:
			operands_set_ts(op, O_IMM1, 16);
			if (!read_stream_safe_uint(ci, &di->imm.ex.i1, sizeof(int16_t))) return FALSE;
		break;
		case OT_IMM8_1:
			operands_set_ts(op, O_IMM1, 8);
			if (!read_stream_safe_uint(ci, &di->imm.ex.i1, sizeof(int8_t))) return FALSE;
		break;
		case OT_IMM8_2:
			operands_set_ts(op, O_IMM2, 8);
			if (!read_stream_safe_uint(ci, &di->imm.ex.i2, sizeof(int8_t))) return FALSE;
		break;
		case OT_REG8:
			operands_set_ts(op, O_REG, 8);
			if (ps->prefixExtType) {
				/*
				 * If REX prefix is valid then we will have to use low bytes.
				 * This is a PASSIVE behaviour changer of REX prefix, it affects operands even if its value is 0x40 !
				 */
				ps->usedPrefixes |= INST_PRE_REX;
				op->index = (uint8_t)operands_fix_8bit_rex_base(reg + ((vrex & PREFIX_EX_R) ? EX_GPR_BASE : 0));
			} else op->index = (uint8_t)(REGS8_BASE + reg);
		break;
		case OT_REG16:
			operands_set_tsi(op, O_REG, 16, REGS16_BASE + reg);
		break;
		case OT_REG_FULL:
			switch (effOpSz)
			{
				case Decode16Bits:
					ps->usedPrefixes |= INST_PRE_OP_SIZE;
					if (vrex & PREFIX_EX_R) {
						ps->usedPrefixes |= INST_PRE_REX;
						reg += EX_GPR_BASE;
					}
					operands_set_tsi(op, O_REG, 16, REGS16_BASE + reg);
				break;
				case Decode32Bits:
					if (vrex & PREFIX_EX_R) {
						ps->usedPrefixes |= INST_PRE_REX;
						reg += EX_GPR_BASE;
					} else ps->usedPrefixes |= INST_PRE_OP_SIZE;
					operands_set_tsi(op, O_REG, 32, REGS32_BASE + reg);
				break;
				case Decode64Bits: /* rex must be presented. */
					ps->usedPrefixes |= INST_PRE_REX;
					operands_set_tsi(op, O_REG, 64, REGS64_BASE + reg + ((vrex & PREFIX_EX_R) ? EX_GPR_BASE : 0));
				break;
			}
		break;
		case OT_REG32:
			if (vrex & PREFIX_EX_R) {
				ps->usedPrefixes |= INST_PRE_REX;
				reg += EX_GPR_BASE;
			}
			operands_set_tsi(op, O_REG, 32, REGS32_BASE + reg);
		break;
		case OT_REG32_64: /* Handle CVT's, MOVxX and MOVNTI instructions which could be extended to 64 bits registers with REX. */
			if (vrex & PREFIX_EX_R) {
				ps->usedPrefixes |= INST_PRE_REX;
				reg += EX_GPR_BASE;
			}

			/* Is it a promoted instruction? (only INST_64BITS is set and REX isn't required.) */
			if ((ci->dt == Decode64Bits) && ((instFlags & (INST_64BITS | INST_PRE_REX)) == INST_64BITS)) {
				operands_set_tsi(op, O_REG, 64, REGS64_BASE + reg);
				break;
			}
			/* Give a chance to REX.W. Because if it was a promoted instruction we don't care about REX.W anyways. */
			if (vrex & PREFIX_EX_W) {
				ps->usedPrefixes |= INST_PRE_REX;
				operands_set_tsi(op, O_REG, 64, REGS64_BASE + reg);
			} else operands_set_tsi(op, O_REG, 32, REGS32_BASE + reg);
		break;
		case OT_FREG32_64_RM: /* Force decoding mode. Used for MOV CR(n)/DR(n) which defaults to 64 bits operand size in 64 bits. */
			if (vrex & PREFIX_EX_B) {
				ps->usedPrefixes |= INST_PRE_REX;
				rm += EX_GPR_BASE;
			}

			if (ci->dt == Decode64Bits) operands_set_tsi(op, O_REG, 64, REGS64_BASE + rm);
			else operands_set_tsi(op, O_REG, 32, REGS32_BASE + rm);
		break;
		case OT_MM: /* MMX register */
			operands_set_tsi(op, O_REG, 64, MMXREGS_BASE + reg);
		break;
		case OT_MM_RM: /* MMX register, this time from the RM field */
			operands_set_tsi(op, O_REG, 64, MMXREGS_BASE + rm);
		break;
		case OT_REGXMM0: /* Implicit XMM0 operand. */
			reg = 0;
			vrex = 0;
		/* FALL THROUGH */
		case OT_XMM: /* SSE register */
			if (vrex & PREFIX_EX_R) {
				ps->usedPrefixes |= INST_PRE_REX;
				reg += EX_GPR_BASE;
			}
			operands_set_tsi(op, O_REG, 128, SSEREGS_BASE + reg);
		break;
		case OT_XMM_RM: /* SSE register, this time from the RM field */
			if (vrex & PREFIX_EX_B) {
				ps->usedPrefixes |= INST_PRE_REX;
				rm += EX_GPR_BASE;
			}
			operands_set_tsi(op, O_REG, 128, SSEREGS_BASE + rm);
		break;
		case OT_CREG:
			/*
			 * Don't parse if the reg exceeds the bounds of the array.
			 * Most of the CR's are not implemented, so if there's no matching string, the operand is invalid.
			 */
			if (vrex & PREFIX_EX_R) {
				ps->usedPrefixes |= INST_PRE_REX;
				reg += EX_GPR_BASE;
			} else if ((ci->dt == Decode32Bits) && (ps->decodedPrefixes & INST_PRE_LOCK)) {
				/*
				 * NOTE: In 32 bits decoding mode,
				 * if the lock prefix is set before MOV CR(n) it will become the 4th bit of the REG field like REX.R in 64 bits.
				 */
				reg += EX_GPR_BASE;
				ps->usedPrefixes |= INST_PRE_LOCK;
			}
			/* Ignore some registers which do not exist. */
			if ((reg >= CREGS_MAX) || (reg == 1) || ((reg >= 5) && (reg <= 7))) return FALSE;

			op->type = O_REG;
			if (ci->dt == Decode64Bits) op->size = 64;
			else op->size = 32;
			op->index = (uint8_t)(CREGS_BASE + reg);
		break;
		case OT_DREG:
			/*
			 * In 64 bits there are 16 debug registers.
			 * but accessing any of dr8-15 which aren't implemented will cause an #ud.
			 */
			if ((reg == 4) || (reg == 5) || (vrex & PREFIX_EX_R)) return FALSE;

			op->type = O_REG;
			if (ci->dt == Decode64Bits) op->size = 64;
			else op->size = 32;
			op->index = (uint8_t)(DREGS_BASE + reg);
		break;
		case OT_SREG: /* Works with REG16 only! */
			/* If lockableInstruction pointer is non-null we know it's the first operand. */
			if (lockableInstruction && (reg == 1)) return FALSE; /* Can't MOV CS, <REG>. */
			/*Don't parse if the reg exceeds the bounds of the array. */
			if (reg <= SEG_REGS_MAX - 1) operands_set_tsi(op, O_REG, 16, SREGS_BASE + reg);
			else return FALSE;
		break;
		case OT_SEG:
			op->type = O_REG;
			/* Size of reg is always 16, it's up to caller to zero extend it to operand size. */
			op->size = 16;
			ps->usedPrefixes |= INST_PRE_OP_SIZE;
			/*
			 * Extract the SEG from ii->flags this time!!!
			 * Check whether an operand size prefix is used.
			 */
			switch (instFlags & INST_PRE_SEGOVRD_MASK)
			{
				case INST_PRE_ES: op->index = R_ES; break;
				case INST_PRE_CS: op->index = R_CS; break;
				case INST_PRE_SS: op->index = R_SS; break;
				case INST_PRE_DS: op->index = R_DS; break;
				case INST_PRE_FS: op->index = R_FS; break;
				case INST_PRE_GS: op->index = R_GS; break;
			}
		break;
		case OT_ACC8:
			operands_set_tsi(op, O_REG, 8, R_AL);
		break;
		case OT_ACC16:
			operands_set_tsi(op, O_REG, 16, R_AX);
		break;
		case OT_ACC_FULL_NOT64: /* No REX.W support for IN/OUT. */
			vrex &= ~PREFIX_EX_W;
		case OT_ACC_FULL:
			if (effOpSz == Decode16Bits) {
				ps->usedPrefixes |= INST_PRE_OP_SIZE;
				operands_set_tsi(op, O_REG, 16, R_AX);
			} else if (effOpSz == Decode32Bits) {
				ps->usedPrefixes |= INST_PRE_OP_SIZE;
				operands_set_tsi(op, O_REG, 32, R_EAX);
			} else { /* Decode64Bits */
				/* Only non-promoted instructions need REX in order to decode in 64 bits. */
				/* MEM-OFFSET MOV's are NOT automatically promoted to 64 bits. */
				if (~instFlags & INST_64BITS) {
					ps->usedPrefixes |= INST_PRE_REX;
				}
				operands_set_tsi(op, O_REG, 64, R_RAX);
			}
		break;
		case OT_PTR16_FULL:
			/* ptr16:full - full is size of operand size to read, therefore Operand Size Prefix affects this. So we need to handle it. */
			if (effOpSz == Decode16Bits) {
				ps->usedPrefixes |= INST_PRE_OP_SIZE;
				ci->codeLen -= sizeof(int16_t)*2;
				if (ci->codeLen < 0) return FALSE;

				operands_set_ts(op, O_PTR, 16);
				di->imm.ptr.off = RUSHORT(ci->code); /* Read offset first. */
				di->imm.ptr.seg = RUSHORT((ci->code + sizeof(int16_t))); /* And read segment. */

				ci->code += sizeof(int16_t)*2;
			} else { /* Decode32Bits, for Decode64Bits this instruction is invalid. */
				ps->usedPrefixes |= INST_PRE_OP_SIZE;
				ci->codeLen -= sizeof(int32_t) + sizeof(int16_t);
				if (ci->codeLen < 0) return FALSE;

				operands_set_ts(op, O_PTR, 32);
				di->imm.ptr.off = RULONG(ci->code); /* Read 32bits offset this time. */
				di->imm.ptr.seg = RUSHORT((ci->code + sizeof(int32_t))); /* And read segment, 16 bits. */
				
				ci->code += sizeof(int32_t) + sizeof(int16_t);
			}
		break;
		case OT_RELCB:
		case OT_RELC_FULL:

			if (type == OT_RELCB) {
				operands_set_ts(op, O_PC, 8);
				if (!read_stream_safe_sint(ci, &di->imm.sqword, sizeof(int8_t))) return FALSE;
			} else { /* OT_RELC_FULL */

				/* Yep, operand size prefix affects relc also.  */
				ps->usedPrefixes |= INST_PRE_OP_SIZE;
				if (effOpSz == Decode16Bits) {
					operands_set_ts(op, O_PC, 16);
					if (!read_stream_safe_sint(ci, &di->imm.sqword, sizeof(int16_t))) return FALSE;
				} else { /* Decode32Bits or Decode64Bits = for now they are the same */
					operands_set_ts(op, O_PC, 32);
					if (!read_stream_safe_sint(ci, &di->imm.sqword, sizeof(int32_t))) return FALSE;
				}
			}

			/* Support for hint, see if there's a segment override. */
			if ((ii->opcodeId >= I_JO) && (ii->opcodeId <= I_JG)) {
				if (ps->decodedPrefixes & INST_PRE_CS) {
					ps->usedPrefixes |= INST_PRE_CS;
					di->flags |= FLAG_HINT_NOT_TAKEN;
				} else if (ps->decodedPrefixes & INST_PRE_DS) {
					ps->usedPrefixes |= INST_PRE_DS;
					di->flags |= FLAG_HINT_TAKEN;
				}
			}
		break;
		case OT_MOFFS8:
			op->size = 8;
			/* FALL THROUGH, size won't be changed. */
		case OT_MOFFS_FULL:
			op->type = O_DISP;
			if (op->size == 0) {
				/* Calculate size of operand (same as ACC size). */
				switch (effOpSz)
				{
					case Decode16Bits: op->size = 16; break;
					case Decode32Bits: op->size = 32; break;
					case Decode64Bits: op->size = 64; break;
				}
			}

			prefixes_use_segment(INST_PRE_DS, ps, ci->dt, di);

			/*
			 * Just a pointer to a BYTE, WORD, DWORD, QWORD. Works only with ACC8/16/32/64 respectively. 
			 * MOV [0x1234], AL ; MOV AX, [0x1234] ; MOV EAX, [0x1234], note that R/E/AX will be chosen by OT_ACC_FULL.
			 */
			if (effAdrSz == Decode16Bits) {
				ps->usedPrefixes |= INST_PRE_ADDR_SIZE;

				di->dispSize = 16;
				if (!read_stream_safe_uint(ci, &di->disp, sizeof(int16_t))) return FALSE;
			} else if (effAdrSz == Decode32Bits) {
				ps->usedPrefixes |= INST_PRE_ADDR_SIZE;

				di->dispSize = 32;
				if (!read_stream_safe_uint(ci, &di->disp, sizeof(int32_t))) return FALSE;
			} else { /* Decode64Bits */
				di->dispSize = 64;
				if (!read_stream_safe_uint(ci, &di->disp, sizeof(int64_t))) return FALSE;
			}
		break;
		case OT_CONST1:
			operands_set_ts(op, O_IMM, 8);
			di->imm.byte = 1;
		break;
		case OT_REGCL:
			operands_set_tsi(op, O_REG, 8, R_CL);
		break;

		case OT_FPU_SI:
			/* Low 3 bits specify the REG, similar to the MODR/M byte reg. */
			operands_set_tsi(op, O_REG, 32, FPUREGS_BASE + (*(ci->code-1) & 7));
		break;
		case OT_FPU_SSI:
			operands_set_tsi(op, O_REG, 32, R_ST0);
			operands_set_tsi(op + 1, O_REG, 32, FPUREGS_BASE + (*(ci->code-1) & 7));
		break;
		case OT_FPU_SIS:
			operands_set_tsi(op, O_REG, 32, FPUREGS_BASE + (*(ci->code-1) & 7));
			operands_set_tsi(op + 1, O_REG, 32, R_ST0);
		break;

		/*
		 * Special treatment for Instructions-Block:
		 * INC/DEC (only 16/32 bits) /PUSH/POP/XCHG instructions, which get their REG from their own binary code.

		 * Notice these instructions are 1 or 2 byte long,
		 * code points after the byte which represents the instruction itself,
		 * thus, even if the instructions are 2 bytes long it will read its last byte which contains the REG info.
		 */
		case OT_IB_RB:
			/* Low 3 bits specify the REG, similar to the MODR/M byte reg. */
			operands_set_ts(op, O_REG, 8);
			reg = *(ci->code-1) & 7;
			if (vrex & PREFIX_EX_B) {
				ps->usedPrefixes |= INST_PRE_REX;
				op->index = (uint8_t)operands_fix_8bit_rex_base(reg + EX_GPR_BASE);
			} else if (ps->prefixExtType == PET_REX) {
				ps->usedPrefixes |= INST_PRE_REX;
				op->index = (uint8_t)operands_fix_8bit_rex_base(reg);
			} else op->index = (uint8_t)(REGS8_BASE + reg);
		break;
		case OT_IB_R_FULL:
			reg = *(ci->code-1) & 7;
			switch (effOpSz)
			{
				case Decode16Bits:
					ps->usedPrefixes |= INST_PRE_OP_SIZE;
					if (vrex & PREFIX_EX_B) {
						ps->usedPrefixes |= INST_PRE_REX;
						reg += EX_GPR_BASE;
					}
					operands_set_tsi(op, O_REG, 16, REGS16_BASE + reg);
				break;
				case Decode32Bits:
					if (vrex & PREFIX_EX_B) {
						ps->usedPrefixes |= INST_PRE_REX;
						reg += EX_GPR_BASE;
					} else ps->usedPrefixes |= INST_PRE_OP_SIZE;
					operands_set_tsi(op, O_REG, 32, REGS32_BASE + reg);
				break;
				case Decode64Bits:
					/*
					 * Automatically promoted instruction can drop REX prefix if not required.
					 * PUSH/POP defaults to 64 bits. --> INST_64BITS
					 * MOV imm64 / BSWAP requires REX.W to be 64 bits --> INST_64BITS | INST_PRE_REX
					 */
					if ((instFlags & INST_64BITS) && ((instFlags & INST_PRE_REX) == 0)) {
						if (vrex & PREFIX_EX_B) {
							ps->usedPrefixes |= INST_PRE_REX;
							reg += EX_GPR_BASE;
						}
					} else {
						ps->usedPrefixes |= INST_PRE_REX;
						reg += (vrex & PREFIX_EX_B) ? EX_GPR_BASE : 0;
					}
					operands_set_tsi(op, O_REG, 64, REGS64_BASE + reg);
				break;
			}
		break;

		/*
		 * Special treatment for repeatable instructions.

		 * We want the following output:
		 * If there's only the REP/NZ prefix, we won't output anything (All operands are implicit).
		 * If there's an operand size prefix, we will change the suffix letter of the mnemonic, which specifies the size of operand to the required one.
		 * If there's a segment override prefix, we will output the segment and the used index register (EDI/ESI).
		 * If there's an address size prefix, we will output the (segment if needed and) the used and inverted index register (DI/SI).

		 * Example:
		 * :: Decoding in 16 bits mode! ::
		 * AD ~ LODSW
		 * 66 AD ~ LODSD
		 * F3 AC ~ REP LODSB
		 * F3 66 AD ~ REP LODSD
		 * F3 3E AC ~ REP LODS BYTE DS:[SI]
		 * F3 67 AD ~ REP LODS WORD [ESI]

		 * The basic form of a repeatable instruction has its operands hidden and has a suffix letter
		 * which implies on the size of operation being done.
		 * Therefore, we cannot change the mnemonic here when we encounter another prefix and its not the decoder's responsibility to do so.
		 * That's why the caller is responsible to add the suffix letter if no other prefixes are used.
		 * And all we are doing here is formatting the operand correctly.
		 */
		case OT_REGI_ESI:
			ps->usedPrefixes |= INST_PRE_ADDR_SIZE;

			op->type = O_SMEM;

			/* This might be a 16, 32 or 64 bits instruction, depends on the decoding mode. */
			if (instFlags & INST_16BITS) {
				ps->usedPrefixes |= INST_PRE_OP_SIZE;

				if (effOpSz == Decode16Bits) op->size = 16;
				else if ((effOpSz == Decode64Bits) && (instFlags & INST_64BITS)) {
					ps->usedPrefixes |= INST_PRE_REX;
					op->size = 64;
				} else op->size = 32;
			} else op->size = 8;

			/*
			 * Clear segment in case OT_REGI_EDI was parsed earlier,
			 * DS can be overridden and therefore has precedence.
			 */
			di->segment = 0;
			prefixes_use_segment(INST_PRE_DS, ps, ci->dt, di);

			if (effAdrSz == Decode16Bits) op->index = R_SI;
			else if (effAdrSz == Decode32Bits) op->index = R_ESI;
			else op->index = R_RSI;
		break;
		case OT_REGI_EDI:
			ps->usedPrefixes |= INST_PRE_ADDR_SIZE;

			op->type = O_SMEM;

			/* This might be a 16 or 32 bits instruction, depends on the decoding mode. */
			if (instFlags & INST_16BITS) {
				ps->usedPrefixes |= INST_PRE_OP_SIZE;

				if (effOpSz == Decode16Bits) op->size = 16;
				else if ((effOpSz == Decode64Bits) && (instFlags & INST_64BITS)) {
					ps->usedPrefixes |= INST_PRE_REX;
					op->size = 64;
				} else op->size = 32;
			} else op->size = 8;

			/* Note: The [rDI] operand can't be prefixed by a segment override, therefore we don't set usedPrefixes. */
			if ((opNum == ONT_1) && (ci->dt != Decode64Bits)) di->segment = R_ES | SEGMENT_DEFAULT; /* No ES in 64 bits mode. */

			if (effAdrSz == Decode16Bits) op->index = R_DI;
			else if (effAdrSz == Decode32Bits) op->index = R_EDI;
			else op->index = R_RDI;
		break;

		/* Used for In/Out instructions varying forms. */
		case OT_REGDX:
			/* Simple single IN/OUT instruction. */
			operands_set_tsi(op, O_REG, 16, R_DX);
		break;

		/* Used for INVLPGA instruction. */
		case OT_REGECX:
			operands_set_tsi(op, O_REG, 32, R_ECX);
		break;
		case OT_REGI_EBXAL:
			/* XLAT BYTE [rBX + AL] */
			ps->usedPrefixes |= INST_PRE_ADDR_SIZE;

			prefixes_use_segment(INST_PRE_DS, ps, ci->dt, di);

			/* Size of deref is always 8 for xlat. */
			operands_set_tsi(op, O_MEM, 8, R_AL);

			if (effAdrSz == Decode16Bits) di->base = R_BX;
			else if (effAdrSz == Decode32Bits) di->base = R_EBX;
			else {
				ps->usedPrefixes |= INST_PRE_REX;
				di->base = R_RBX;
			}
		break;
		case OT_REGI_EAX:
			/*
			 * Implicit rAX as memory indirection operand. Used by AMD's SVM instructions.
			 * Since this is a memory indirection, the default address size in 64bits decoding mode is 64.
			 */

			if (effAdrSz == Decode64Bits) operands_set_tsi(op, O_SMEM, 64, R_RAX);
			else if (effAdrSz == Decode32Bits) {
				ps->usedPrefixes |= INST_PRE_ADDR_SIZE;
				operands_set_tsi(op, O_SMEM, 32, R_EAX);
			} else {
				ps->usedPrefixes |= INST_PRE_ADDR_SIZE;
				operands_set_tsi(op, O_SMEM, 16, R_AX);
			}
		break;
		case OT_VXMM:
			operands_set_tsi(op, O_REG, 128, SSEREGS_BASE + vexV);
		break;
		case OT_XMM_IMM:
			ci->codeLen -= sizeof(int8_t);
			if (ci->codeLen < 0) return FALSE;

			if (ci->dt == Decode32Bits) reg = (*ci->code >> 4) & 0x7;
			else reg = (*ci->code >> 4) & 0xf;
			operands_set_tsi(op, O_REG, 128, SSEREGS_BASE + reg);

			ci->code += sizeof(int8_t);
		break;
		case OT_YXMM:
			if (vrex & PREFIX_EX_R) reg += EX_GPR_BASE;
			if (ps->vrex & PREFIX_EX_L) operands_set_tsi(op, O_REG, 256, AVXREGS_BASE + reg);
			else operands_set_tsi(op, O_REG, 128, SSEREGS_BASE + reg);
		break;
		case OT_YXMM_IMM:
			ci->codeLen -= sizeof(int8_t);
			if (ci->codeLen < 0) return FALSE;

			if (ci->dt == Decode32Bits) reg = (*ci->code >> 4) & 0x7;
			else reg = (*ci->code >> 4) & 0xf;

			if (ps->vrex & PREFIX_EX_L) operands_set_tsi(op, O_REG, 256, AVXREGS_BASE + reg);
			else operands_set_tsi(op, O_REG, 128, SSEREGS_BASE + reg);

			ci->code += sizeof(int8_t);
		break;
		case OT_YMM:
			if (vrex & PREFIX_EX_R) reg += EX_GPR_BASE;
			operands_set_tsi(op, O_REG, 256, AVXREGS_BASE + reg);
		break;
		case OT_VYMM:
			operands_set_tsi(op, O_REG, 256, AVXREGS_BASE + vexV);
		break;
		case OT_VYXMM:
			if (ps->vrex & PREFIX_EX_L) operands_set_tsi(op, O_REG, 256, AVXREGS_BASE + vexV);
			else operands_set_tsi(op, O_REG, 128, SSEREGS_BASE + vexV); 
		break;
		case OT_WREG32_64:
			if (vrex & PREFIX_EX_R) reg += EX_GPR_BASE;
			if (ps->vrex & PREFIX_EX_W) operands_set_tsi(op, O_REG, 64, REGS64_BASE + reg);
			else operands_set_tsi(op, O_REG, 32, REGS32_BASE + reg);
		break;
		default: return FALSE;
	}

	if ((op->type == O_REG) || (op->type == O_SMEM) || (op->type == O_MEM)) {
		di->usedRegistersMask |= _REGISTERTORCLASS[op->index];
	}

	return TRUE;
}
