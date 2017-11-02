/*
x86defs.h

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


#ifndef X86DEFS_H
#define X86DEFS_H


#define SEG_REGS_MAX (6)
#define CREGS_MAX (9)
#define DREGS_MAX (8)

/* Maximum instruction size, including prefixes */
#define INST_MAXIMUM_SIZE (15)

/* Maximum range of imm8 (comparison type) of special SSE CMP instructions. */
#define INST_CMP_MAX_RANGE (8)

/* Maximum range of imm8 (comparison type) of special AVX VCMP instructions. */
#define INST_VCMP_MAX_RANGE (32)

/* Wait instruction byte code. */
#define INST_WAIT_INDEX (0x9b)

/* Lea instruction byte code. */
#define INST_LEA_INDEX (0x8d)

/* NOP/XCHG instruction byte code. */
#define INST_NOP_INDEX (0x90)

/* ARPL/MOVSXD instruction byte code. */
#define INST_ARPL_INDEX (0x63)

/*
 * Minimal MODR/M value of divided instructions.
 * It's 0xc0, two MSBs set, which indicates a general purpose register is used too.
 */
#define INST_DIVIDED_MODRM (0xc0)

/* This is the escape byte value used for 3DNow! instructions. */
#define _3DNOW_ESCAPE_BYTE (0x0f)

#define PREFIX_LOCK (0xf0)
#define PREFIX_REPNZ (0xf2)
#define PREFIX_REP (0xf3)
#define PREFIX_CS (0x2e)
#define PREFIX_SS (0x36)
#define PREFIX_DS (0x3e)
#define PREFIX_ES (0x26)
#define PREFIX_FS (0x64)
#define PREFIX_GS (0x65)
#define PREFIX_OP_SIZE (0x66)
#define PREFIX_ADDR_SIZE (0x67)
#define PREFIX_VEX2b (0xc5)
#define PREFIX_VEX3b (0xc4)

/* REX prefix value range, 64 bits mode decoding only. */
#define PREFIX_REX_LOW (0x40)
#define PREFIX_REX_HI (0x4f)
/* In order to use the extended GPR's we have to add 8 to the Modr/M info values. */
#define EX_GPR_BASE (8)

/* Mask for REX and VEX features: */
/* Base */
#define PREFIX_EX_B (1)
/* Index */
#define PREFIX_EX_X (2)
/* Register */
#define PREFIX_EX_R (4)
/* Operand Width */
#define PREFIX_EX_W (8)
/* Vector Lengh */
#define PREFIX_EX_L (0x10)

#endif /* X86DEFS_H */
