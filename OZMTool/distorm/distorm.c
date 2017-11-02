/*
distorm.c

diStorm3 C Library Interface
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


#include "../include/distorm.h"
#include "config.h"
#include "decoder.h"
#include "x86defs.h"
#include "textdefs.h"
#include "wstring.h"
#include "../include/mnemonics.h"

/* C DLL EXPORTS */
#ifdef SUPPORT_64BIT_OFFSET
	_DLLEXPORT_ _DecodeResult distorm_decompose64(_CodeInfo* ci, _DInst result[], unsigned int maxInstructions, unsigned int* usedInstructionsCount)
#else
	_DLLEXPORT_ _DecodeResult distorm_decompose32(_CodeInfo* ci, _DInst result[], unsigned int maxInstructions, unsigned int* usedInstructionsCount)
#endif
{
	if (usedInstructionsCount == NULL) {
		return DECRES_SUCCESS;
	}

	/* DECRES_SUCCESS still may indicate we may have something in the result, so zero it first thing. */
	*usedInstructionsCount = 0;

	if ((ci == NULL) ||
		(ci->codeLen < 0) ||
		((ci->dt != Decode16Bits) && (ci->dt != Decode32Bits) && (ci->dt != Decode64Bits)) ||
		(ci->code == NULL) ||
		(result == NULL) ||
		((ci->features & (DF_MAXIMUM_ADDR16 | DF_MAXIMUM_ADDR32)) == (DF_MAXIMUM_ADDR16 | DF_MAXIMUM_ADDR32)))
	{
		return DECRES_INPUTERR;
	}

	/* Assume length=0 is success. */
	if (ci->codeLen == 0) {
		return DECRES_SUCCESS;
	}

	return decode_internal(ci, FALSE, result, maxInstructions, usedInstructionsCount);
}

#ifndef DISTORM_LIGHT

/* Helper function to concatenate an explicit size when it's unknown from the operands. */
static void distorm_format_size(_WString* str, const _DInst* di, int opNum)
{
	int isSizingRequired = 0;
	/*
	 * We only have to output the size explicitly if it's not clear from the operands.
	 * For example:
	 * mov al, [0x1234] -> The size is 8, we know it from the AL register operand.
	 * mov [0x1234], 0x11 -> Now we don't know the size. Pam pam pam
	 *
	 * If given operand number is higher than 2, then output the size anyways.
	 */
	isSizingRequired = ((opNum >= 2) || ((di->ops[0].type != O_REG) && (di->ops[1].type != O_REG)));

	/* Still not sure? Try some special instructions. */
	if (!isSizingRequired) {
		/*
		 * INS/OUTS are exception, because DX is a port specifier and not a real src/dst register.
		 * A few exceptions that always requires sizing:
		 * MOVZX, MOVSX, MOVSXD.
		 * ROL, ROR, RCL, RCR, SHL, SHR, SAL, SAR.
		 * SHLD, SHRD.
		 */
		switch (di->opcode)
		{
			case I_INS:
			case I_OUTS:
			case I_MOVZX:
			case I_MOVSX:
			case I_MOVSXD:
			case I_ROL:
			case I_ROR:
			case I_RCL:
			case I_RCR:
			case I_SHL:
			case I_SHR:
			case I_SAL:
			case I_SAR:
			case I_SHLD:
			case I_SHRD:
				isSizingRequired = 1;
			break;
			default: /* Instruction doesn't require sizing. */ break;
		}
	}

	if (isSizingRequired)
	{
		switch (di->ops[opNum].size)
		{
			case 0: break; /* OT_MEM's unknown size. */
			case 8: strcat_WSN(str, "BYTE "); break;
			case 16: strcat_WSN(str, "WORD "); break;
			case 32: strcat_WSN(str, "DWORD "); break;
			case 64: strcat_WSN(str, "QWORD "); break;
			case 80: strcat_WSN(str, "TBYTE "); break;
			case 128: strcat_WSN(str, "DQWORD "); break;
			case 256: strcat_WSN(str, "YWORD "); break;
			default: /* Big oh uh if it gets here. */ break;
		}
	}
}

static void distorm_format_signed_disp(_WString* str, const _DInst* di, uint64_t addrMask)
{
	int64_t tmpDisp64;

	if (di->dispSize) {
		chrcat_WS(str, ((int64_t)di->disp < 0) ? MINUS_DISP_CHR : PLUS_DISP_CHR);
		if ((int64_t)di->disp < 0) tmpDisp64 = -(int64_t)di->disp;
		else tmpDisp64 = di->disp;
		tmpDisp64 &= addrMask;
		str_code_hqw(str, (uint8_t*)&tmpDisp64);
	}
}

#ifdef SUPPORT_64BIT_OFFSET
	_DLLEXPORT_ void distorm_format64(const _CodeInfo* ci, const _DInst* di, _DecodedInst* result)
#else
	_DLLEXPORT_ void distorm_format32(const _CodeInfo* ci, const _DInst* di, _DecodedInst* result)
#endif
{
	_WString* str;
	unsigned int i, isDefault;
	int64_t tmpDisp64;
	uint64_t addrMask = (uint64_t)-1;
	uint8_t segment;
	const _WMnemonic* mnemonic;

	/* Set address mask, when default is for 64bits addresses. */
	if (ci->features & DF_MAXIMUM_ADDR32) addrMask = 0xffffffff;
	else if (ci->features & DF_MAXIMUM_ADDR16) addrMask = 0xffff;

	/* Copy other fields. */
	result->size = di->size;
	result->offset = di->addr & addrMask;

	if (di->flags == FLAG_NOT_DECODABLE) {
		str = &result->mnemonic;
		strclear_WS(&result->operands);
		strcpy_WSN(str, "DB ");
		str_code_hb(str, di->imm.byte);
		strclear_WS(&result->instructionHex);
		str_hex_b(&result->instructionHex, di->imm.byte);
		return; /* Skip to next instruction. */
	}

	str = &result->instructionHex;
	strclear_WS(str);
	for (i = 0; i < di->size; i++)
		str_hex_b(str, ci->code[(unsigned int)(di->addr - ci->codeOffset + i)]);

	str = &result->mnemonic;
	switch (FLAG_GET_PREFIX(di->flags))
	{
		case FLAG_LOCK:
			strcpy_WSN(str, "LOCK ");
		break;
		case FLAG_REP:
			/* REP prefix for CMPS and SCAS is really a REPZ. */
			if ((di->opcode == I_CMPS) || (di->opcode == I_SCAS)) strcpy_WSN(str, "REPZ ");
			else strcpy_WSN(str, "REP ");
		break;
		case FLAG_REPNZ:
			strcpy_WSN(str, "REPNZ ");
		break;
		default:
			/* Init mnemonic string, cause next touch is concatenation. */
			strclear_WS(str);
		break;
	}

	mnemonic = (const _WMnemonic*)&_MNEMONICS[di->opcode];
	memcpy((int8_t*)&str->p[str->length], mnemonic->p, mnemonic->length + 1);
	str->length += mnemonic->length;

	/* Format operands: */
	str = &result->operands;
	strclear_WS(str);

	/* Special treatment for String instructions. */
	if ((META_GET_ISC(di->meta) == ISC_INTEGER) &&
		((di->opcode == I_MOVS) ||
		 (di->opcode == I_CMPS) ||
		 (di->opcode == I_STOS) ||
		 (di->opcode == I_LODS) ||
		 (di->opcode == I_SCAS)))
	{
		/*
		 * No operands are needed if the address size is the default one,
		 * and no segment is overridden, so add the suffix letter,
		 * to indicate size of operation and continue to next instruction.
		 */
		if ((FLAG_GET_ADDRSIZE(di->flags) == ci->dt) && (SEGMENT_IS_DEFAULT(di->segment))) {
			str = &result->mnemonic;
			switch (di->ops[0].size)
			{
				case 8: chrcat_WS(str, 'B'); break;
				case 16: chrcat_WS(str, 'W'); break;
				case 32: chrcat_WS(str, 'D'); break;
				case 64: chrcat_WS(str, 'Q'); break;
			}
			return;
		}
	}

	for (i = 0; ((i < OPERANDS_NO) && (di->ops[i].type != O_NONE)); i++) {
		if (i > 0) strcat_WSN(str, ", ");
		switch (di->ops[i].type)
		{
			case O_REG:
				strcat_WS(str, (const _WString*)&_REGISTERS[di->ops[i].index]);
			break;
			case O_IMM:
				/* If the instruction is 'push', show explicit size (except byte imm). */
				if ((di->opcode == I_PUSH) && (di->ops[i].size != 8)) distorm_format_size(str, di, i);
				/* Special fix for negative sign extended immediates. */
				if ((di->flags & FLAG_IMM_SIGNED) && (di->ops[i].size == 8)) {
					if (di->imm.sbyte < 0) {
						chrcat_WS(str, MINUS_DISP_CHR);
						str_code_hb(str, -di->imm.sbyte);
						break;
					}
				}
				if (di->ops[i].size == 64) str_code_hqw(str, (uint8_t*)&di->imm.qword);
				else str_code_hdw(str, di->imm.dword);
			break;
			case O_IMM1:
				str_code_hdw(str, di->imm.ex.i1);
			break;
			case O_IMM2:
				str_code_hdw(str, di->imm.ex.i2);
			break;
			case O_DISP:
				distorm_format_size(str, di, i);
				chrcat_WS(str, OPEN_CHR);
				if ((SEGMENT_GET(di->segment) != R_NONE) && !SEGMENT_IS_DEFAULT(di->segment)) {
					strcat_WS(str, (const _WString*)&_REGISTERS[SEGMENT_GET(di->segment)]);
					chrcat_WS(str, SEG_OFF_CHR);
				}
				tmpDisp64 = di->disp & addrMask;
				str_code_hqw(str, (uint8_t*)&tmpDisp64);
				chrcat_WS(str, CLOSE_CHR);
			break;
			case O_SMEM:
				distorm_format_size(str, di, i);
				chrcat_WS(str, OPEN_CHR);

				/*
				 * This is where we need to take special care for String instructions.
				 * If we got here, it means we need to explicitly show their operands.
				 * The problem with CMPS and MOVS is that they have two(!) memory operands.
				 * So we have to complete it ourselves, since the structure supplies only the segment that can be overridden.
				 * And make the rest of the String operations explicit.
				 */
				segment = SEGMENT_GET(di->segment);
				isDefault = SEGMENT_IS_DEFAULT(di->segment);
				switch (di->opcode)
				{
					case I_MOVS:
						isDefault = FALSE;
						if (i == 0) segment = R_ES;
					break;
					case I_CMPS:
						isDefault = FALSE;
						if (i == 1) segment = R_ES;
					break;
					case I_INS:
					case I_LODS:
					case I_STOS:
					case I_SCAS: isDefault = FALSE; break;
				}
				if (!isDefault && (segment != R_NONE)) {
					strcat_WS(str, (const _WString*)&_REGISTERS[segment]);
					chrcat_WS(str, SEG_OFF_CHR);
				}

				strcat_WS(str, (const _WString*)&_REGISTERS[di->ops[i].index]);

				distorm_format_signed_disp(str, di, addrMask);
				chrcat_WS(str, CLOSE_CHR);
			break;
			case O_MEM:
				distorm_format_size(str, di, i);
				chrcat_WS(str, OPEN_CHR);
				if ((SEGMENT_GET(di->segment) != R_NONE) && !SEGMENT_IS_DEFAULT(di->segment)) {
					strcat_WS(str, (const _WString*)&_REGISTERS[SEGMENT_GET(di->segment)]);
					chrcat_WS(str, SEG_OFF_CHR);
				}
				if (di->base != R_NONE) {
					strcat_WS(str, (const _WString*)&_REGISTERS[di->base]);
					chrcat_WS(str, PLUS_DISP_CHR);
				}
				strcat_WS(str, (const _WString*)&_REGISTERS[di->ops[i].index]);
				if (di->scale != 0) {
					chrcat_WS(str, '*');
					if (di->scale == 2) chrcat_WS(str, '2');
					else if (di->scale == 4) chrcat_WS(str, '4');
					else /* if (di->scale == 8) */ chrcat_WS(str, '8');
				}

				distorm_format_signed_disp(str, di, addrMask);
				chrcat_WS(str, CLOSE_CHR);
			break;
			case O_PC:
#ifdef SUPPORT_64BIT_OFFSET
				str_off64(str, (di->imm.sqword + di->addr + di->size) & addrMask);
#else
				str_code_hdw(str, ((_OffsetType)di->imm.sdword + di->addr + di->size) & (uint32_t)addrMask);
#endif
			break;
			case O_PTR:
				str_code_hdw(str, di->imm.ptr.seg);
				chrcat_WS(str, SEG_OFF_CHR);
				str_code_hdw(str, di->imm.ptr.off);
			break;
		}
	}

	if (di->flags & FLAG_HINT_TAKEN) strcat_WSN(str, " ;TAKEN");
	else if (di->flags & FLAG_HINT_NOT_TAKEN) strcat_WSN(str, " ;NOT TAKEN");
}

#ifdef SUPPORT_64BIT_OFFSET
	_DLLEXPORT_ _DecodeResult distorm_decode64(_OffsetType codeOffset, const unsigned char* code, int codeLen, _DecodeType dt, _DecodedInst result[], unsigned int maxInstructions, unsigned int* usedInstructionsCount)
#else
	_DLLEXPORT_ _DecodeResult distorm_decode32(_OffsetType codeOffset, const unsigned char* code, int codeLen, _DecodeType dt, _DecodedInst result[], unsigned int maxInstructions, unsigned int* usedInstructionsCount)
#endif
{
	_DecodeResult res;
	_DInst di;
	_CodeInfo ci;
	unsigned int instsCount = 0, i;

	*usedInstructionsCount = 0;

	/* I use codeLen as a signed variable in order to ease detection of underflow... and besides - */
	if (codeLen < 0) {
		return DECRES_INPUTERR;
	}

	if ((dt != Decode16Bits) && (dt != Decode32Bits) && (dt != Decode64Bits)) {
		return DECRES_INPUTERR;
	}

	if (code == NULL || result == NULL) {
		return DECRES_INPUTERR;
	}

	/* Assume length=0 is success. */
	if (codeLen == 0) {
		return DECRES_SUCCESS;
	}

	/*
	 * We have to format the result into text. But the interal decoder works with the new structure of _DInst.
	 * Therefore, we will pass the result array(!) from the caller and the interal decoder will fill it in with _DInst's.
	 * Then we will copy each result to a temporary structure, and use it to reformat that specific result.
	 *
	 * This is all done to save memory allocation and to work on the same result array in-place!!!
	 * It's a bit ugly, I have to admit, but worth it.
	 */

	ci.codeOffset = codeOffset;
	ci.code = code;
	ci.codeLen = codeLen;
	ci.dt = dt;
	ci.features = DF_NONE;
	if (dt == Decode16Bits) ci.features = DF_MAXIMUM_ADDR16;
	else if (dt == Decode32Bits) ci.features = DF_MAXIMUM_ADDR32;

	res = decode_internal(&ci, TRUE, (_DInst*)result, maxInstructions, &instsCount);
	for (i = 0; i < instsCount; i++) {
		if ((*usedInstructionsCount + i) >= maxInstructions) return DECRES_MEMORYERR;

		/* Copy the current decomposed result to a temp structure, so we can override the result with text. */
		memcpy(&di, (char*)result + (i * sizeof(_DecodedInst)), sizeof(_DInst));
#ifdef SUPPORT_64BIT_OFFSET
		distorm_format64(&ci, &di, &result[i]);
#else
		distorm_format32(&ci, &di, &result[i]);
#endif
	}

	*usedInstructionsCount = instsCount;
	return res;
}

#endif /* DISTORM_LIGHT */

_DLLEXPORT_ unsigned int distorm_version()
{
	return __DISTORMV__;
}
