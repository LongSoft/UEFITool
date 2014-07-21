/*
insts.h

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


#ifndef INSTS_H
#define INSTS_H

#include "instructions.h"


/* Flags Table */
extern _iflags FlagsTable[];

/* Root Trie DB */
extern _InstSharedInfo InstSharedInfoTable[];
extern _InstInfo InstInfos[];
extern _InstInfoEx InstInfosEx[];
extern _InstNode InstructionsTree[];

/* 3DNow! Trie DB */
extern _InstNode Table_0F_0F;
/* AVX related: */
extern _InstNode Table_0F, Table_0F_38, Table_0F_3A;

/*
 * The inst_lookup will return on of these two instructions according to the specified decoding mode.
 * ARPL or MOVSXD on 64 bits is one byte instruction at index 0x63.
 */
extern _InstInfo II_ARPL;
extern _InstInfo II_MOVSXD;

/*
 * The NOP instruction can be prefixed by REX in 64bits, therefore we have to decide in runtime whether it's an XCHG or NOP instruction.
 * If 0x90 is prefixed by a useable REX it will become XCHG, otherwise it will become a NOP.
 * Also note that if it's prefixed by 0xf3, it becomes a Pause.
 */
extern _InstInfo II_NOP;
extern _InstInfo II_PAUSE;

/*
 * Used for letting the extract operand know the type of operands without knowing the
 * instruction itself yet, because of the way those instructions work.
 * See function instructions.c!inst_lookup_3dnow.
 */
extern _InstInfo II_3DNOW;

/* Helper tables for pesudo compare mnemonics. */
extern uint16_t CmpMnemonicOffsets[8]; /* SSE */
extern uint16_t VCmpMnemonicOffsets[32]; /* AVX */

#endif /* INSTS_H */
