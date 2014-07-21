/*
instructions.h

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


#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H

#include "config.h"
#include "prefix.h"


/*
 * Operand type possibilities:
 * Note "_FULL" suffix indicates to decode the operand as 16 bits or 32 bits depends on DecodeType -
 * actually, it depends on the decoding mode, unless there's an operand/address size prefix.
 * For example, the code: 33 c0 could be decoded/executed as XOR AX, AX or XOR EAX, EAX.
 */
typedef enum OpType {
	/* No operand is set */
	OT_NONE = 0,

	/* Read a byte(8 bits) immediate */
	OT_IMM8,
	/* Force a read of a word(16 bits) immediate, used by ret only */
	OT_IMM16,
	/* Read a word/dword immediate */
	OT_IMM_FULL,
	/* Read a double-word(32 bits) immediate */
	OT_IMM32,

	/* Read a signed extended byte(8 bits) immediate */
	OT_SEIMM8,

	/*
	 * Special immediates for instructions which have more than one immediate,
	 * which is an exception from standard instruction format.
	 * As to version v1.0: ENTER, INSERTQ, EXTRQ are the only problematic ones.
	 */
	/* 16 bits immediate using the first imm-slot */
	OT_IMM16_1,
	/* 8 bits immediate using the first imm-slot */
	OT_IMM8_1,
	/* 8 bits immediate using the second imm-slot */
	OT_IMM8_2,

	/* Use a 8bit register */
	OT_REG8,
	/* Use a 16bit register */
	OT_REG16,
	/* Use a 16/32/64bit register */
	OT_REG_FULL,
	/* Use a 32bit register */
	OT_REG32,
	/*
	 * If used with REX the reg operand size becomes 64 bits, otherwise 32 bits.
	 * VMX instructions are promoted automatically without a REX prefix.
	 */
	OT_REG32_64,
	/* Used only by MOV CR/DR(n). Promoted with REX onlly. */
	OT_FREG32_64_RM,

	/* Use or read (indirection) a 8bit register or immediate byte */
	OT_RM8,
	/* Some instructions force 16 bits (mov sreg, rm16) */
	OT_RM16,
	/* Use or read a 16/32/64bit register or immediate word/dword/qword */
	OT_RM_FULL,
	/*
	 * 32 or 64 bits (with REX) operand size indirection memory operand.
	 * Some instructions are promoted automatically without a REX prefix.
	 */
	OT_RM32_64,
	/* 16 or 32 bits RM. This is used only with MOVZXD instruction in 64bits. */
	OT_RM16_32,
	/* Same as OT_RMXX but POINTS to 16 bits [cannot use GENERAL-PURPOSE REG!] */
	OT_FPUM16,
	/* Same as OT_RMXX but POINTS to 32 bits (single precision) [cannot use GENERAL-PURPOSE REG!] */
	OT_FPUM32,
	/* Same as OT_RMXX but POINTS to 64 bits (double precision) [cannot use GENERAL-PURPOSE REG!] */
	OT_FPUM64,
	/* Same as OT_RMXX but POINTS to 80 bits (extended precision) [cannot use GENERAL-PURPOSE REG!] */
	OT_FPUM80,

	/*
	 * Special operand type for SSE4 where the ModR/M might
	 * be a 32 bits register or 8 bits memory indirection operand.
	 */
	OT_R32_M8,
	/*
	 * Special ModR/M for PINSRW, which need a 16 bits memory operand or 32 bits register.
	 * In 16 bits decoding mode R32 becomes R16, operand size cannot affect this.
	 */
	OT_R32_M16,
	/*
	 * Special type for SSE4, ModR/M might be a 32 bits or 64 bits (with REX) register or
	 * a 8 bits memory indirection operand.
	 */
	OT_R32_64_M8,
	/*
	 * Special type for SSE4, ModR/M might be a 32 bits or 64 bits (with REX) register or
	 * a 16 bits memory indirection operand.
	 */
	OT_R32_64_M16,
	/*
	 * Special operand type for MOV reg16/32/64/mem16, segReg 8C /r. and SMSW.
	 * It supports all decoding modes, but if used as a memory indirection it's a 16 bit ModR/M indirection.
	 */
	OT_RFULL_M16,

	/* Use a control register */
	OT_CREG,
	/* Use a debug register */
	OT_DREG,
	/* Use a segment register */
	OT_SREG,
	/*
	 * SEG is encoded in the flags of the opcode itself!
	 * This is used for specific "push SS" where SS is a segment where
	 * each "push SS" has an absolutely different opcode byte.
	 * We need this to detect whether an operand size prefix is used.
	 */
	OT_SEG,
	
	/* Use AL */
	OT_ACC8,
	/* Use AX (FSTSW) */
	OT_ACC16,
	/* Use AX/EAX/RAX */
	OT_ACC_FULL,
	/* Use AX/EAX, no REX is possible for RAX, used only with IN/OUT which don't support 64 bit registers */
	OT_ACC_FULL_NOT64,

	/*
	 * Read one word (seg), and a word/dword/qword (depends on operand size) from memory.
	 * JMP FAR [EBX] means EBX point to 16:32 ptr.
	 */
	OT_MEM16_FULL,
	/* Read one word (seg) and a word/dword/qword (depends on operand size), usually SEG:OFF, JMP 1234:1234 */
	OT_PTR16_FULL,
	/* Read one word (limit) and a dword/qword (limit) (depends on operand size), used by SGDT, SIDT, LGDT, LIDT. */
	OT_MEM16_3264,

	/* Read a byte(8 bits) immediate and calculate it relatively to the current offset of the instruction being decoded */
	OT_RELCB,
	/* Read a word/dword immediate and calculate it relatively to the current offset of the instruction being decoded */
	OT_RELC_FULL,

	/* Use general memory indirection, with varying sizes: */
	OT_MEM,
	/* Used when a memory indirection is required, but if the mod field is 11, this operand will be ignored. */
	OT_MEM_OPT,
	OT_MEM32,
	/* Memory dereference for MOVNTI, either 32 or 64 bits (with REX). */
	OT_MEM32_64,
	OT_MEM64,
	OT_MEM128,
	/* Used for cmpxchg8b/16b. */
	OT_MEM64_128,

	/* Read an immediate as an absolute address, size is known by instruction, used by MOV (memory offset) only */
	OT_MOFFS8,
	OT_MOFFS_FULL,
	/* Use an immediate of 1, as for SHR R/M, 1 */
	OT_CONST1,
	/* Use CL, as for SHR R/M, CL */
	OT_REGCL,

	/*
	 * Instruction-Block for one byte long instructions, used by INC/DEC/PUSH/POP/XCHG,
	 * REG is extracted from the value of opcode
	 * Use a 8bit register
	 */
	OT_IB_RB,
	/* Use a 16/32/64bit register */
	OT_IB_R_FULL,

	/* Use [(r)SI] as INDIRECTION, for repeatable instructions */
	OT_REGI_ESI,
	/* Use [(r)DI] as INDIRECTION, for repeatable instructions */
	OT_REGI_EDI,
	/* Use [(r)BX + AL] as INDIRECTIOM, used by XLAT only */
	OT_REGI_EBXAL,
	/* Use [(r)AX] as INDIRECTION, used by AMD's SVM instructions */
	OT_REGI_EAX,
	/* Use DX, as for OUTS DX, BYTE [SI] */
	OT_REGDX,
	/* Use ECX in INVLPGA instruction */
	OT_REGECX,

	/* FPU registers: */
	OT_FPU_SI, /* ST(i) */
	OT_FPU_SSI, /* ST(0), ST(i) */
	OT_FPU_SIS, /* ST(i), ST(0) */

	/* MMX registers: */
	OT_MM,
	/* Extract the MMX register from the RM bits this time (used when the REG bits are used for opcode extension) */
	OT_MM_RM,
	/* ModR/M points to 32 bits MMX variable */
	OT_MM32,
	/* ModR/M points to 32 bits MMX variable */
	OT_MM64,

	/* SSE registers: */
	OT_XMM,
	/* Extract the SSE register from the RM bits this time (used when the REG bits are used for opcode extension) */
	OT_XMM_RM,
	/* ModR/M points to 16 bits SSE variable */
	OT_XMM16,
	/* ModR/M points to 32 bits SSE variable */
	OT_XMM32,
	/* ModR/M points to 64 bits SSE variable */
	OT_XMM64,
	/* ModR/M points to 128 bits SSE variable */
	OT_XMM128,
	/* Implied XMM0 register as operand, used in SSE4. */
	OT_REGXMM0,

	/* AVX operands: */

	/* ModR/M for 32 bits. */
	OT_RM32,
	/* Reg32/Reg64 (prefix width) or Mem8. */
	OT_REG32_64_M8,
	/* Reg32/Reg64 (prefix width) or Mem16. */
	OT_REG32_64_M16,
	/* Reg32/Reg 64 depends on prefix width only. */
	OT_WREG32_64,
	/* RM32/RM64 depends on prefix width only. */
	OT_WRM32_64,
	/* XMM or Mem32/Mem64 depends on perfix width only. */
	OT_WXMM32_64,
	/* XMM is encoded in VEX.VVVV. */
	OT_VXMM,
	/* XMM is encoded in the high nibble of an immediate byte. */
	OT_XMM_IMM,
	/* YMM/XMM is dependent on VEX.L. */
	OT_YXMM,
	/* YMM/XMM (depends on prefix length) is encoded in the high nibble of an immediate byte. */
	OT_YXMM_IMM,
	/* YMM is encoded in reg. */
	OT_YMM,
	/* YMM or Mem256. */
	OT_YMM256,
	/* YMM is encoded in VEX.VVVV. */
	OT_VYMM,
	/* YMM/XMM is dependent on VEX.L, and encoded in VEX.VVVV. */
	OT_VYXMM,
	/* YMM/XMM or Mem64/Mem256 is dependent on VEX.L. */
	OT_YXMM64_256,
	/* YMM/XMM or Mem128/Mem256 is dependent on VEX.L. */
	OT_YXMM128_256,
	/* XMM or Mem64/Mem256 is dependent on VEX.L. */
	OT_LXMM64_128,
	/* Mem128/Mem256 is dependent on VEX.L. */
	OT_LMEM128_256
} _OpType;

/* Flags for instruction: */

/* Empty flags indicator: */
#define INST_FLAGS_NONE (0)
/* The instruction we are going to decode requires ModR/M encoding. */
#define INST_MODRM_REQUIRED (1)
/* Special treatment for instructions which are in the divided-category but still needs the whole byte for ModR/M... */
#define INST_NOT_DIVIDED (1 << 1)
/*
 * Used explicitly in repeatable instructions,
 * which needs a suffix letter in their mnemonic to specify operation-size (depend on operands).
 */
#define INST_16BITS (1 << 2)
/* If the opcode is supported by 80286 and upper models (16/32 bits). */
#define INST_32BITS (1 << 3)
/*
 * Prefix flags (6 types: lock/rep, seg override, addr-size, oper-size, REX, VEX)
 * There are several specific instructions that can follow LOCK prefix,
 * note that they must be using a memory operand form, otherwise they generate an exception.
 */
#define INST_PRE_LOCK (1 << 4)
/* REPNZ prefix for string instructions only - means an instruction can follow it. */
#define INST_PRE_REPNZ (1 << 5)
/* REP prefix for string instructions only - means an instruction can follow it. */
#define INST_PRE_REP (1 << 6)
/* CS override prefix. */
#define INST_PRE_CS (1 << 7)
/* SS override prefix. */
#define INST_PRE_SS (1 << 8)
/* DS override prefix. */
#define INST_PRE_DS (1 << 9)
/* ES override prefix. */
#define INST_PRE_ES (1 << 10)
/* FS override prefix. Funky Segment :) */
#define INST_PRE_FS (1 << 11)
/* GS override prefix. Groovy Segment, of course not, duh ! */
#define INST_PRE_GS (1 << 12)
/* Switch operand size from 32 to 16 and vice versa. */
#define INST_PRE_OP_SIZE (1 << 13)
/* Switch address size from 32 to 16 and vice versa. */
#define INST_PRE_ADDR_SIZE (1 << 14)
/* Native instructions which needs suffix letter to indicate their operation-size (and don't depend on operands). */
#define INST_NATIVE (1 << 15)
/* Use extended mnemonic, means it's an _InstInfoEx structure, which contains another mnemonic for 32 bits specifically. */
#define INST_USE_EXMNEMONIC (1 << 16)
/* Use third operand, means it's an _InstInfoEx structure, which contains another operand for special instructions. */
#define INST_USE_OP3 (1 << 17)
/* Use fourth operand, means it's an _InstInfoEx structure, which contains another operand for special instructions. */
#define INST_USE_OP4 (1 << 18)
/* The instruction's mnemonic depends on the mod value of the ModR/M byte (mod=11, mod!=11). */
#define INST_MNEMONIC_MODRM_BASED (1 << 19)
/* The instruction uses a ModR/M byte which the MOD must be 11 (for registers operands only). */
#define INST_MODRR_REQUIRED (1 << 20)
/* The way of 3DNow! instructions are built, we have to handle their locating specially. Suffix imm8 tells which instruction it is. */
#define INST_3DNOW_FETCH (1 << 21)
/* The instruction needs two suffixes, one for the comparison type (imm8) and the second for its operation size indication (second mnemonic). */
#define INST_PSEUDO_OPCODE (1 << 22)
/* Invalid instruction at 64 bits decoding mode. */
#define INST_INVALID_64BITS (1 << 23)
/* Specific instruction can be promoted to 64 bits (without REX, it is promoted automatically). */
#define INST_64BITS (1 << 24)
/* Indicates the instruction must be REX prefixed in order to use 64 bits operands. */
#define INST_PRE_REX (1 << 25)
/* Third mnemonic is set. */
#define INST_USE_EXMNEMONIC2 (1 << 26)
/* Instruction is only valid in 64 bits decoding mode. */
#define INST_64BITS_FETCH (1 << 27)
/* Forces that the ModRM-REG/Opcode field will be 0. (For EXTRQ). */
#define INST_FORCE_REG0 (1 << 28)
/* Indicates that instruction is encoded with a VEX prefix. */
#define INST_PRE_VEX (1 << 29)
/* Indicates that the instruction is encoded with a ModRM byte (REG field specifically). */
#define INST_MODRM_INCLUDED (1 << 30)
/* Indicates that the first (/destination) operand of the instruction is writable. */
#define INST_DST_WR (1 << 31)

#define INST_PRE_REPS (INST_PRE_REPNZ | INST_PRE_REP)
#define INST_PRE_LOKREP_MASK (INST_PRE_LOCK | INST_PRE_REPNZ | INST_PRE_REP)
#define INST_PRE_SEGOVRD_MASK32 (INST_PRE_CS | INST_PRE_SS | INST_PRE_DS | INST_PRE_ES)
#define INST_PRE_SEGOVRD_MASK64 (INST_PRE_FS | INST_PRE_GS)
#define INST_PRE_SEGOVRD_MASK (INST_PRE_SEGOVRD_MASK32 | INST_PRE_SEGOVRD_MASK64)

/* Extended flags for VEX: */
/* Indicates that the instruction might have VEX.L encoded. */
#define INST_VEX_L (1)
/* Indicates that the instruction might have VEX.W encoded. */
#define INST_VEX_W (1 << 1)
/* Indicates that the mnemonic of the instruction is based on the VEX.W bit. */
#define INST_MNEMONIC_VEXW_BASED (1 << 2)
/* Indicates that the mnemonic of the instruction is based on the VEX.L bit. */
#define INST_MNEMONIC_VEXL_BASED (1 << 3)
/* Forces the instruction to be encoded with VEX.L, otherwise it's undefined. */
#define INST_FORCE_VEXL (1 << 4)
/*
 * Indicates that the instruction is based on the MOD field of the ModRM byte.
 * (MOD==11: got the right instruction, else skip +4 in prefixed table for the correct instruction).
 */
#define INST_MODRR_BASED (1 << 5)
/* Indicates that the instruction doesn't use the VVVV field of the VEX prefix, if it does then it's undecodable. */
#define INST_VEX_V_UNUSED (1 << 6)

/* Indication that the instruction is privileged (Ring 0), this should be checked on the opcodeId field. */
#define OPCODE_ID_PRIVILEGED ((uint16_t)0x8000)

/*
 * Indicates which operand is being decoded.
 * Destination (1st), Source (2nd), op3 (3rd), op4 (4th).
 * Used to set the operands' fields in the _DInst structure!
 */
typedef enum {ONT_NONE = -1, ONT_1 = 0, ONT_2 = 1, ONT_3 = 2, ONT_4 = 3} _OperandNumberType;

/* CPU Flags that instructions modify, test or undefine, in compacted form (CF,PF,AF,ZF,SF are 1:1 map to EFLAGS). */
#define D_COMPACT_CF 1		/* Carry */
#define D_COMPACT_PF 4		/* Parity */
#define D_COMPACT_AF 0x10	/* Auxiliary */
#define D_COMPACT_ZF 0x40	/* Zero */
#define D_COMPACT_SF 0x80	/* Sign */
/* The following flags have to be translated to EFLAGS. */
#define D_COMPACT_IF 2		/* Interrupt */
#define D_COMPACT_DF 8		/* Direction */
#define D_COMPACT_OF 0x20	/* Overflow */

/* The mask of flags that are already compatible with EFLAGS. */
#define D_COMPACT_SAME_FLAGS (D_COMPACT_CF | D_COMPACT_PF | D_COMPACT_AF | D_COMPACT_ZF | D_COMPACT_SF)

/*
 * In order to save more space for storing the DB statically,
 * I came up with another level of shared info.
 * Because I saw that most of the information that instructions use repeats itself.
 *
 * Info about the instruction, source/dest types, meta and flags.
 * _InstInfo points to a table of _InstSharedInfo.
 */
typedef struct {
	uint8_t flagsIndex; /* An index into FlagsTables */
	uint8_t s, d; /* OpType. */
	uint8_t meta; /* Hi 5 bits = Instruction set class | Lo 3 bits = flow control flags. */
	/*
	 * The following are CPU flag masks that the instruction changes.
	 * The flags are compacted so 8 bits representation is enough.
	 * They will be expanded in runtime to be compatible to EFLAGS.
	 */
	uint8_t modifiedFlagsMask;
	uint8_t testedFlagsMask;
	uint8_t undefinedFlagsMask;
} _InstSharedInfo;

/*
 * This structure is used for the instructions DB and NOT for the disassembled result code!
 * This is the BASE structure, there are extensions to this structure below.
 */
typedef struct {
	uint16_t sharedIndex; /* An index into the SharedInfoTable. */
	uint16_t opcodeId; /* The opcodeId is really a byte-offset into the mnemonics table. MSB is a privileged indication. */
} _InstInfo;

/*
 * There are merely few instructions which need a second mnemonic for 32 bits.
 * Or a third for 64 bits. Therefore sometimes the second mnemonic is empty but not the third.
 * In all decoding modes the first mnemonic is the default.
 * A flag will indicate it uses another mnemonic.
 *
 * There are a couple of (SSE4) instructions in the whole DB which need both op3 and 3rd mnemonic for 64bits,
 * therefore, I decided to make the extended structure contain all extra info in the same structure.
 * There are a few instructions (SHLD/SHRD/IMUL and SSE too) which use third operand (or a fourth).
 * A flag will indicate it uses a third/fourth operand.
 */
typedef struct {
	/* Base structure (doesn't get accessed directly from code). */
	_InstInfo BASE;

	/* Extended starts here. */
	uint8_t flagsEx; /* 8 bits are enough, in the future we might make it a bigger integer. */
	uint8_t op3, op4; /* OpType. */
	uint16_t opcodeId2, opcodeId3;
} _InstInfoEx;

/* Trie data structure node type: */
typedef enum {
	INT_NOTEXISTS = 0, /* Not exists. */
	INT_INFO = 1, /* It's an instruction info. */
	INT_INFOEX,
	INT_LIST_GROUP,
	INT_LIST_FULL,
	INT_LIST_DIVIDED,
	INT_LIST_PREFIXED
} _InstNodeType;

/* Used to check instType < INT_INFOS, means we got an inst-info. Cause it has to be only one of them. */
#define INT_INFOS (INT_LIST_GROUP)

/* Instruction node is treated as { int index:13;  int type:3; } */
typedef uint16_t _InstNode;

_InstInfo* inst_lookup(_CodeInfo* ci, _PrefixState* ps);
_InstInfo* inst_lookup_3dnow(_CodeInfo* ci);

#endif /* INSTRUCTIONS_H */
