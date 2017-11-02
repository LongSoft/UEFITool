/*
mnemonics.c

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


#include "../include/mnemonics.h"

#ifndef DISTORM_LIGHT


const unsigned char _MNEMONICS[] =
"\x09" "UNDEFINED\0" "\x03" "ADD\0" "\x04" "PUSH\0" "\x03" "POP\0" "\x02" "OR\0" \
"\x03" "ADC\0" "\x03" "SBB\0" "\x03" "AND\0" "\x03" "DAA\0" "\x03" "SUB\0" \
"\x03" "DAS\0" "\x03" "XOR\0" "\x03" "AAA\0" "\x03" "CMP\0" "\x03" "AAS\0" \
"\x03" "INC\0" "\x03" "DEC\0" "\x05" "PUSHA\0" "\x04" "POPA\0" "\x05" "BOUND\0" \
"\x04" "ARPL\0" "\x04" "IMUL\0" "\x03" "INS\0" "\x04" "OUTS\0" "\x02" "JO\0" \
"\x03" "JNO\0" "\x02" "JB\0" "\x03" "JAE\0" "\x02" "JZ\0" "\x03" "JNZ\0" "\x03" "JBE\0" \
"\x02" "JA\0" "\x02" "JS\0" "\x03" "JNS\0" "\x02" "JP\0" "\x03" "JNP\0" "\x02" "JL\0" \
"\x03" "JGE\0" "\x03" "JLE\0" "\x02" "JG\0" "\x04" "TEST\0" "\x04" "XCHG\0" \
"\x03" "MOV\0" "\x03" "LEA\0" "\x03" "CBW\0" "\x04" "CWDE\0" "\x04" "CDQE\0" \
"\x03" "CWD\0" "\x03" "CDQ\0" "\x03" "CQO\0" "\x08" "CALL FAR\0" "\x05" "PUSHF\0" \
"\x04" "POPF\0" "\x04" "SAHF\0" "\x04" "LAHF\0" "\x04" "MOVS\0" "\x04" "CMPS\0" \
"\x04" "STOS\0" "\x04" "LODS\0" "\x04" "SCAS\0" "\x03" "RET\0" "\x03" "LES\0" \
"\x03" "LDS\0" "\x05" "ENTER\0" "\x05" "LEAVE\0" "\x04" "RETF\0" "\x05" "INT 3\0" \
"\x03" "INT\0" "\x04" "INTO\0" "\x04" "IRET\0" "\x03" "AAM\0" "\x03" "AAD\0" \
"\x04" "SALC\0" "\x04" "XLAT\0" "\x06" "LOOPNZ\0" "\x05" "LOOPZ\0" "\x04" "LOOP\0" \
"\x04" "JCXZ\0" "\x05" "JECXZ\0" "\x05" "JRCXZ\0" "\x02" "IN\0" "\x03" "OUT\0" \
"\x04" "CALL\0" "\x03" "JMP\0" "\x07" "JMP FAR\0" "\x04" "INT1\0" "\x03" "HLT\0" \
"\x03" "CMC\0" "\x03" "CLC\0" "\x03" "STC\0" "\x03" "CLI\0" "\x03" "STI\0" \
"\x03" "CLD\0" "\x03" "STD\0" "\x03" "LAR\0" "\x03" "LSL\0" "\x07" "SYSCALL\0" \
"\x04" "CLTS\0" "\x06" "SYSRET\0" "\x04" "INVD\0" "\x06" "WBINVD\0" "\x03" "UD2\0" \
"\x05" "FEMMS\0" "\x03" "NOP\0" "\x05" "WRMSR\0" "\x05" "RDTSC\0" "\x05" "RDMSR\0" \
"\x05" "RDPMC\0" "\x08" "SYSENTER\0" "\x07" "SYSEXIT\0" "\x06" "GETSEC\0" "\x05" "CMOVO\0" \
"\x06" "CMOVNO\0" "\x05" "CMOVB\0" "\x06" "CMOVAE\0" "\x05" "CMOVZ\0" "\x06" "CMOVNZ\0" \
"\x06" "CMOVBE\0" "\x05" "CMOVA\0" "\x05" "CMOVS\0" "\x06" "CMOVNS\0" "\x05" "CMOVP\0" \
"\x06" "CMOVNP\0" "\x05" "CMOVL\0" "\x06" "CMOVGE\0" "\x06" "CMOVLE\0" "\x05" "CMOVG\0" \
"\x04" "SETO\0" "\x05" "SETNO\0" "\x04" "SETB\0" "\x05" "SETAE\0" "\x04" "SETZ\0" \
"\x05" "SETNZ\0" "\x05" "SETBE\0" "\x04" "SETA\0" "\x04" "SETS\0" "\x05" "SETNS\0" \
"\x04" "SETP\0" "\x05" "SETNP\0" "\x04" "SETL\0" "\x05" "SETGE\0" "\x05" "SETLE\0" \
"\x04" "SETG\0" "\x05" "CPUID\0" "\x02" "BT\0" "\x04" "SHLD\0" "\x03" "RSM\0" \
"\x03" "BTS\0" "\x04" "SHRD\0" "\x07" "CMPXCHG\0" "\x03" "LSS\0" "\x03" "BTR\0" \
"\x03" "LFS\0" "\x03" "LGS\0" "\x05" "MOVZX\0" "\x03" "BTC\0" "\x05" "MOVSX\0" \
"\x04" "XADD\0" "\x06" "MOVNTI\0" "\x05" "BSWAP\0" "\x03" "ROL\0" "\x03" "ROR\0" \
"\x03" "RCL\0" "\x03" "RCR\0" "\x03" "SHL\0" "\x03" "SHR\0" "\x03" "SAL\0" \
"\x03" "SAR\0" "\x04" "FADD\0" "\x04" "FMUL\0" "\x04" "FCOM\0" "\x05" "FCOMP\0" \
"\x04" "FSUB\0" "\x05" "FSUBR\0" "\x04" "FDIV\0" "\x05" "FDIVR\0" "\x03" "FLD\0" \
"\x03" "FST\0" "\x04" "FSTP\0" "\x06" "FLDENV\0" "\x05" "FLDCW\0" "\x04" "FXCH\0" \
"\x04" "FNOP\0" "\x04" "FCHS\0" "\x04" "FABS\0" "\x04" "FTST\0" "\x04" "FXAM\0" \
"\x04" "FLD1\0" "\x06" "FLDL2T\0" "\x06" "FLDL2E\0" "\x05" "FLDPI\0" "\x06" "FLDLG2\0" \
"\x06" "FLDLN2\0" "\x04" "FLDZ\0" "\x05" "F2XM1\0" "\x05" "FYL2X\0" "\x05" "FPTAN\0" \
"\x06" "FPATAN\0" "\x07" "FXTRACT\0" "\x06" "FPREM1\0" "\x07" "FDECSTP\0" "\x07" "FINCSTP\0" \
"\x05" "FPREM\0" "\x07" "FYL2XP1\0" "\x05" "FSQRT\0" "\x07" "FSINCOS\0" "\x07" "FRNDINT\0" \
"\x06" "FSCALE\0" "\x04" "FSIN\0" "\x04" "FCOS\0" "\x05" "FIADD\0" "\x05" "FIMUL\0" \
"\x05" "FICOM\0" "\x06" "FICOMP\0" "\x05" "FISUB\0" "\x06" "FISUBR\0" "\x05" "FIDIV\0" \
"\x06" "FIDIVR\0" "\x06" "FCMOVB\0" "\x06" "FCMOVE\0" "\x07" "FCMOVBE\0" "\x06" "FCMOVU\0" \
"\x07" "FUCOMPP\0" "\x04" "FILD\0" "\x06" "FISTTP\0" "\x04" "FIST\0" "\x05" "FISTP\0" \
"\x07" "FCMOVNB\0" "\x07" "FCMOVNE\0" "\x08" "FCMOVNBE\0" "\x07" "FCMOVNU\0" \
"\x04" "FENI\0" "\x06" "FEDISI\0" "\x06" "FSETPM\0" "\x06" "FUCOMI\0" "\x05" "FCOMI\0" \
"\x06" "FRSTOR\0" "\x05" "FFREE\0" "\x05" "FUCOM\0" "\x06" "FUCOMP\0" "\x05" "FADDP\0" \
"\x05" "FMULP\0" "\x06" "FCOMPP\0" "\x06" "FSUBRP\0" "\x05" "FSUBP\0" "\x06" "FDIVRP\0" \
"\x05" "FDIVP\0" "\x04" "FBLD\0" "\x05" "FBSTP\0" "\x07" "FUCOMIP\0" "\x06" "FCOMIP\0" \
"\x03" "NOT\0" "\x03" "NEG\0" "\x03" "MUL\0" "\x03" "DIV\0" "\x04" "IDIV\0" \
"\x04" "SLDT\0" "\x03" "STR\0" "\x04" "LLDT\0" "\x03" "LTR\0" "\x04" "VERR\0" \
"\x04" "VERW\0" "\x04" "SGDT\0" "\x04" "SIDT\0" "\x04" "LGDT\0" "\x04" "LIDT\0" \
"\x04" "SMSW\0" "\x04" "LMSW\0" "\x06" "INVLPG\0" "\x06" "VMCALL\0" "\x08" "VMLAUNCH\0" \
"\x08" "VMRESUME\0" "\x06" "VMXOFF\0" "\x07" "MONITOR\0" "\x05" "MWAIT\0" "\x06" "XGETBV\0" \
"\x06" "XSETBV\0" "\x06" "VMFUNC\0" "\x05" "VMRUN\0" "\x07" "VMMCALL\0" "\x06" "VMLOAD\0" \
"\x06" "VMSAVE\0" "\x04" "STGI\0" "\x04" "CLGI\0" "\x06" "SKINIT\0" "\x07" "INVLPGA\0" \
"\x06" "SWAPGS\0" "\x06" "RDTSCP\0" "\x08" "PREFETCH\0" "\x09" "PREFETCHW\0" \
"\x05" "PI2FW\0" "\x05" "PI2FD\0" "\x05" "PF2IW\0" "\x05" "PF2ID\0" "\x06" "PFNACC\0" \
"\x07" "PFPNACC\0" "\x07" "PFCMPGE\0" "\x05" "PFMIN\0" "\x05" "PFRCP\0" "\x07" "PFRSQRT\0" \
"\x05" "PFSUB\0" "\x05" "PFADD\0" "\x07" "PFCMPGT\0" "\x05" "PFMAX\0" "\x08" "PFRCPIT1\0" \
"\x08" "PFRSQIT1\0" "\x06" "PFSUBR\0" "\x05" "PFACC\0" "\x07" "PFCMPEQ\0" "\x05" "PFMUL\0" \
"\x08" "PFRCPIT2\0" "\x07" "PMULHRW\0" "\x06" "PSWAPD\0" "\x07" "PAVGUSB\0" \
"\x06" "MOVUPS\0" "\x06" "MOVUPD\0" "\x05" "MOVSS\0" "\x05" "MOVSD\0" "\x07" "VMOVUPS\0" \
"\x07" "VMOVUPD\0" "\x06" "VMOVSS\0" "\x06" "VMOVSD\0" "\x07" "MOVHLPS\0" "\x06" "MOVLPS\0" \
"\x06" "MOVLPD\0" "\x08" "MOVSLDUP\0" "\x07" "MOVDDUP\0" "\x08" "VMOVHLPS\0" \
"\x07" "VMOVLPS\0" "\x07" "VMOVLPD\0" "\x09" "VMOVSLDUP\0" "\x08" "VMOVDDUP\0" \
"\x08" "UNPCKLPS\0" "\x08" "UNPCKLPD\0" "\x09" "VUNPCKLPS\0" "\x09" "VUNPCKLPD\0" \
"\x08" "UNPCKHPS\0" "\x08" "UNPCKHPD\0" "\x09" "VUNPCKHPS\0" "\x09" "VUNPCKHPD\0" \
"\x07" "MOVLHPS\0" "\x06" "MOVHPS\0" "\x06" "MOVHPD\0" "\x08" "MOVSHDUP\0" \
"\x08" "VMOVLHPS\0" "\x07" "VMOVHPS\0" "\x07" "VMOVHPD\0" "\x09" "VMOVSHDUP\0" \
"\x0b" "PREFETCHNTA\0" "\x0a" "PREFETCHT0\0" "\x0a" "PREFETCHT1\0" "\x0a" "PREFETCHT2\0" \
"\x06" "MOVAPS\0" "\x06" "MOVAPD\0" "\x07" "VMOVAPS\0" "\x07" "VMOVAPD\0" "\x08" "CVTPI2PS\0" \
"\x08" "CVTPI2PD\0" "\x08" "CVTSI2SS\0" "\x08" "CVTSI2SD\0" "\x09" "VCVTSI2SS\0" \
"\x09" "VCVTSI2SD\0" "\x07" "MOVNTPS\0" "\x07" "MOVNTPD\0" "\x07" "MOVNTSS\0" \
"\x07" "MOVNTSD\0" "\x08" "VMOVNTPS\0" "\x08" "VMOVNTPD\0" "\x09" "CVTTPS2PI\0" \
"\x09" "CVTTPD2PI\0" "\x09" "CVTTSS2SI\0" "\x09" "CVTTSD2SI\0" "\x0a" "VCVTTSS2SI\0" \
"\x0a" "VCVTTSD2SI\0" "\x08" "CVTPS2PI\0" "\x08" "CVTPD2PI\0" "\x08" "CVTSS2SI\0" \
"\x08" "CVTSD2SI\0" "\x09" "VCVTSS2SI\0" "\x09" "VCVTSD2SI\0" "\x07" "UCOMISS\0" \
"\x07" "UCOMISD\0" "\x08" "VUCOMISS\0" "\x08" "VUCOMISD\0" "\x06" "COMISS\0" \
"\x06" "COMISD\0" "\x07" "VCOMISS\0" "\x07" "VCOMISD\0" "\x08" "MOVMSKPS\0" \
"\x08" "MOVMSKPD\0" "\x09" "VMOVMSKPS\0" "\x09" "VMOVMSKPD\0" "\x06" "SQRTPS\0" \
"\x06" "SQRTPD\0" "\x06" "SQRTSS\0" "\x06" "SQRTSD\0" "\x07" "VSQRTPS\0" "\x07" "VSQRTPD\0" \
"\x07" "VSQRTSS\0" "\x07" "VSQRTSD\0" "\x07" "RSQRTPS\0" "\x07" "RSQRTSS\0" \
"\x08" "VRSQRTPS\0" "\x08" "VRSQRTSS\0" "\x05" "RCPPS\0" "\x05" "RCPSS\0" "\x06" "VRCPPS\0" \
"\x06" "VRCPSS\0" "\x05" "ANDPS\0" "\x05" "ANDPD\0" "\x06" "VANDPS\0" "\x06" "VANDPD\0" \
"\x06" "ANDNPS\0" "\x06" "ANDNPD\0" "\x07" "VANDNPS\0" "\x07" "VANDNPD\0" "\x04" "ORPS\0" \
"\x04" "ORPD\0" "\x05" "VORPS\0" "\x05" "VORPD\0" "\x05" "XORPS\0" "\x05" "XORPD\0" \
"\x06" "VXORPS\0" "\x06" "VXORPD\0" "\x05" "ADDPS\0" "\x05" "ADDPD\0" "\x05" "ADDSS\0" \
"\x05" "ADDSD\0" "\x06" "VADDPS\0" "\x06" "VADDPD\0" "\x06" "VADDSS\0" "\x06" "VADDSD\0" \
"\x05" "MULPS\0" "\x05" "MULPD\0" "\x05" "MULSS\0" "\x05" "MULSD\0" "\x06" "VMULPS\0" \
"\x06" "VMULPD\0" "\x06" "VMULSS\0" "\x06" "VMULSD\0" "\x08" "CVTPS2PD\0" "\x08" "CVTPD2PS\0" \
"\x08" "CVTSS2SD\0" "\x08" "CVTSD2SS\0" "\x09" "VCVTPS2PD\0" "\x09" "VCVTPD2PS\0" \
"\x09" "VCVTSS2SD\0" "\x09" "VCVTSD2SS\0" "\x08" "CVTDQ2PS\0" "\x08" "CVTPS2DQ\0" \
"\x09" "CVTTPS2DQ\0" "\x09" "VCVTDQ2PS\0" "\x09" "VCVTPS2DQ\0" "\x0a" "VCVTTPS2DQ\0" \
"\x05" "SUBPS\0" "\x05" "SUBPD\0" "\x05" "SUBSS\0" "\x05" "SUBSD\0" "\x06" "VSUBPS\0" \
"\x06" "VSUBPD\0" "\x06" "VSUBSS\0" "\x06" "VSUBSD\0" "\x05" "MINPS\0" "\x05" "MINPD\0" \
"\x05" "MINSS\0" "\x05" "MINSD\0" "\x06" "VMINPS\0" "\x06" "VMINPD\0" "\x06" "VMINSS\0" \
"\x06" "VMINSD\0" "\x05" "DIVPS\0" "\x05" "DIVPD\0" "\x05" "DIVSS\0" "\x05" "DIVSD\0" \
"\x06" "VDIVPS\0" "\x06" "VDIVPD\0" "\x06" "VDIVSS\0" "\x06" "VDIVSD\0" "\x05" "MAXPS\0" \
"\x05" "MAXPD\0" "\x05" "MAXSS\0" "\x05" "MAXSD\0" "\x06" "VMAXPS\0" "\x06" "VMAXPD\0" \
"\x06" "VMAXSS\0" "\x06" "VMAXSD\0" "\x09" "PUNPCKLBW\0" "\x0a" "VPUNPCKLBW\0" \
"\x09" "PUNPCKLWD\0" "\x0a" "VPUNPCKLWD\0" "\x09" "PUNPCKLDQ\0" "\x0a" "VPUNPCKLDQ\0" \
"\x08" "PACKSSWB\0" "\x09" "VPACKSSWB\0" "\x07" "PCMPGTB\0" "\x08" "VPCMPGTB\0" \
"\x07" "PCMPGTW\0" "\x08" "VPCMPGTW\0" "\x07" "PCMPGTD\0" "\x08" "VPCMPGTD\0" \
"\x08" "PACKUSWB\0" "\x09" "VPACKUSWB\0" "\x09" "PUNPCKHBW\0" "\x0a" "VPUNPCKHBW\0" \
"\x09" "PUNPCKHWD\0" "\x0a" "VPUNPCKHWD\0" "\x09" "PUNPCKHDQ\0" "\x0a" "VPUNPCKHDQ\0" \
"\x08" "PACKSSDW\0" "\x09" "VPACKSSDW\0" "\x0a" "PUNPCKLQDQ\0" "\x0b" "VPUNPCKLQDQ\0" \
"\x0a" "PUNPCKHQDQ\0" "\x0b" "VPUNPCKHQDQ\0" "\x04" "MOVD\0" "\x04" "MOVQ\0" \
"\x05" "VMOVD\0" "\x05" "VMOVQ\0" "\x06" "MOVDQA\0" "\x06" "MOVDQU\0" "\x07" "VMOVDQA\0" \
"\x07" "VMOVDQU\0" "\x06" "PSHUFW\0" "\x06" "PSHUFD\0" "\x07" "PSHUFHW\0" "\x07" "PSHUFLW\0" \
"\x07" "VPSHUFD\0" "\x08" "VPSHUFHW\0" "\x08" "VPSHUFLW\0" "\x07" "PCMPEQB\0" \
"\x08" "VPCMPEQB\0" "\x07" "PCMPEQW\0" "\x08" "VPCMPEQW\0" "\x07" "PCMPEQD\0" \
"\x08" "VPCMPEQD\0" "\x04" "EMMS\0" "\x0a" "VZEROUPPER\0" "\x08" "VZEROALL\0" \
"\x06" "VMREAD\0" "\x05" "EXTRQ\0" "\x07" "INSERTQ\0" "\x07" "VMWRITE\0" "\x08" "CVTPH2PS\0" \
"\x08" "CVTPS2PH\0" "\x06" "HADDPD\0" "\x06" "HADDPS\0" "\x07" "VHADDPD\0" \
"\x07" "VHADDPS\0" "\x06" "HSUBPD\0" "\x06" "HSUBPS\0" "\x07" "VHSUBPD\0" "\x07" "VHSUBPS\0" \
"\x05" "XSAVE\0" "\x07" "XSAVE64\0" "\x06" "LFENCE\0" "\x06" "XRSTOR\0" "\x08" "XRSTOR64\0" \
"\x06" "MFENCE\0" "\x08" "XSAVEOPT\0" "\x0a" "XSAVEOPT64\0" "\x06" "SFENCE\0" \
"\x07" "CLFLUSH\0" "\x06" "POPCNT\0" "\x03" "BSF\0" "\x05" "TZCNT\0" "\x03" "BSR\0" \
"\x05" "LZCNT\0" "\x07" "CMPEQPS\0" "\x07" "CMPLTPS\0" "\x07" "CMPLEPS\0" "\x0a" "CMPUNORDPS\0" \
"\x08" "CMPNEQPS\0" "\x08" "CMPNLTPS\0" "\x08" "CMPNLEPS\0" "\x08" "CMPORDPS\0" \
"\x07" "CMPEQPD\0" "\x07" "CMPLTPD\0" "\x07" "CMPLEPD\0" "\x0a" "CMPUNORDPD\0" \
"\x08" "CMPNEQPD\0" "\x08" "CMPNLTPD\0" "\x08" "CMPNLEPD\0" "\x08" "CMPORDPD\0" \
"\x07" "CMPEQSS\0" "\x07" "CMPLTSS\0" "\x07" "CMPLESS\0" "\x0a" "CMPUNORDSS\0" \
"\x08" "CMPNEQSS\0" "\x08" "CMPNLTSS\0" "\x08" "CMPNLESS\0" "\x08" "CMPORDSS\0" \
"\x07" "CMPEQSD\0" "\x07" "CMPLTSD\0" "\x07" "CMPLESD\0" "\x0a" "CMPUNORDSD\0" \
"\x08" "CMPNEQSD\0" "\x08" "CMPNLTSD\0" "\x08" "CMPNLESD\0" "\x08" "CMPORDSD\0" \
"\x08" "VCMPEQPS\0" "\x08" "VCMPLTPS\0" "\x08" "VCMPLEPS\0" "\x0b" "VCMPUNORDPS\0" \
"\x09" "VCMPNEQPS\0" "\x09" "VCMPNLTPS\0" "\x09" "VCMPNLEPS\0" "\x09" "VCMPORDPS\0" \
"\x0b" "VCMPEQ_UQPS\0" "\x09" "VCMPNGEPS\0" "\x09" "VCMPNGTPS\0" "\x0b" "VCMPFALSEPS\0" \
"\x0c" "VCMPNEQ_OQPS\0" "\x08" "VCMPGEPS\0" "\x08" "VCMPGTPS\0" "\x0a" "VCMPTRUEPS\0" \
"\x0b" "VCMPEQ_OSPS\0" "\x0b" "VCMPLT_OQPS\0" "\x0b" "VCMPLE_OQPS\0" "\x0d" "VCMPUNORD_SPS\0" \
"\x0c" "VCMPNEQ_USPS\0" "\x0c" "VCMPNLT_UQPS\0" "\x0c" "VCMPNLE_UQPS\0" "\x0b" "VCMPORD_SPS\0" \
"\x0b" "VCMPEQ_USPS\0" "\x0c" "VCMPNGE_UQPS\0" "\x0c" "VCMPNGT_UQPS\0" "\x0e" "VCMPFALSE_OSPS\0" \
"\x0c" "VCMPNEQ_OSPS\0" "\x0b" "VCMPGE_OQPS\0" "\x0b" "VCMPGT_OQPS\0" "\x0d" "VCMPTRUE_USPS\0" \
"\x08" "VCMPEQPD\0" "\x08" "VCMPLTPD\0" "\x08" "VCMPLEPD\0" "\x0b" "VCMPUNORDPD\0" \
"\x09" "VCMPNEQPD\0" "\x09" "VCMPNLTPD\0" "\x09" "VCMPNLEPD\0" "\x09" "VCMPORDPD\0" \
"\x0b" "VCMPEQ_UQPD\0" "\x09" "VCMPNGEPD\0" "\x09" "VCMPNGTPD\0" "\x0b" "VCMPFALSEPD\0" \
"\x0c" "VCMPNEQ_OQPD\0" "\x08" "VCMPGEPD\0" "\x08" "VCMPGTPD\0" "\x0a" "VCMPTRUEPD\0" \
"\x0b" "VCMPEQ_OSPD\0" "\x0b" "VCMPLT_OQPD\0" "\x0b" "VCMPLE_OQPD\0" "\x0d" "VCMPUNORD_SPD\0" \
"\x0c" "VCMPNEQ_USPD\0" "\x0c" "VCMPNLT_UQPD\0" "\x0c" "VCMPNLE_UQPD\0" "\x0b" "VCMPORD_SPD\0" \
"\x0b" "VCMPEQ_USPD\0" "\x0c" "VCMPNGE_UQPD\0" "\x0c" "VCMPNGT_UQPD\0" "\x0e" "VCMPFALSE_OSPD\0" \
"\x0c" "VCMPNEQ_OSPD\0" "\x0b" "VCMPGE_OQPD\0" "\x0b" "VCMPGT_OQPD\0" "\x0d" "VCMPTRUE_USPD\0" \
"\x08" "VCMPEQSS\0" "\x08" "VCMPLTSS\0" "\x08" "VCMPLESS\0" "\x0b" "VCMPUNORDSS\0" \
"\x09" "VCMPNEQSS\0" "\x09" "VCMPNLTSS\0" "\x09" "VCMPNLESS\0" "\x09" "VCMPORDSS\0" \
"\x0b" "VCMPEQ_UQSS\0" "\x09" "VCMPNGESS\0" "\x09" "VCMPNGTSS\0" "\x0b" "VCMPFALSESS\0" \
"\x0c" "VCMPNEQ_OQSS\0" "\x08" "VCMPGESS\0" "\x08" "VCMPGTSS\0" "\x0a" "VCMPTRUESS\0" \
"\x0b" "VCMPEQ_OSSS\0" "\x0b" "VCMPLT_OQSS\0" "\x0b" "VCMPLE_OQSS\0" "\x0d" "VCMPUNORD_SSS\0" \
"\x0c" "VCMPNEQ_USSS\0" "\x0c" "VCMPNLT_UQSS\0" "\x0c" "VCMPNLE_UQSS\0" "\x0b" "VCMPORD_SSS\0" \
"\x0b" "VCMPEQ_USSS\0" "\x0c" "VCMPNGE_UQSS\0" "\x0c" "VCMPNGT_UQSS\0" "\x0e" "VCMPFALSE_OSSS\0" \
"\x0c" "VCMPNEQ_OSSS\0" "\x0b" "VCMPGE_OQSS\0" "\x0b" "VCMPGT_OQSS\0" "\x0d" "VCMPTRUE_USSS\0" \
"\x08" "VCMPEQSD\0" "\x08" "VCMPLTSD\0" "\x08" "VCMPLESD\0" "\x0b" "VCMPUNORDSD\0" \
"\x09" "VCMPNEQSD\0" "\x09" "VCMPNLTSD\0" "\x09" "VCMPNLESD\0" "\x09" "VCMPORDSD\0" \
"\x0b" "VCMPEQ_UQSD\0" "\x09" "VCMPNGESD\0" "\x09" "VCMPNGTSD\0" "\x0b" "VCMPFALSESD\0" \
"\x0c" "VCMPNEQ_OQSD\0" "\x08" "VCMPGESD\0" "\x08" "VCMPGTSD\0" "\x0a" "VCMPTRUESD\0" \
"\x0b" "VCMPEQ_OSSD\0" "\x0b" "VCMPLT_OQSD\0" "\x0b" "VCMPLE_OQSD\0" "\x0d" "VCMPUNORD_SSD\0" \
"\x0c" "VCMPNEQ_USSD\0" "\x0c" "VCMPNLT_UQSD\0" "\x0c" "VCMPNLE_UQSD\0" "\x0b" "VCMPORD_SSD\0" \
"\x0b" "VCMPEQ_USSD\0" "\x0c" "VCMPNGE_UQSD\0" "\x0c" "VCMPNGT_UQSD\0" "\x0e" "VCMPFALSE_OSSD\0" \
"\x0c" "VCMPNEQ_OSSD\0" "\x0b" "VCMPGE_OQSD\0" "\x0b" "VCMPGT_OQSD\0" "\x0d" "VCMPTRUE_USSD\0" \
"\x06" "PINSRW\0" "\x07" "VPINSRW\0" "\x06" "PEXTRW\0" "\x07" "VPEXTRW\0" "\x06" "SHUFPS\0" \
"\x06" "SHUFPD\0" "\x07" "VSHUFPS\0" "\x07" "VSHUFPD\0" "\x09" "CMPXCHG8B\0" \
"\x0a" "CMPXCHG16B\0" "\x07" "VMPTRST\0" "\x08" "ADDSUBPD\0" "\x08" "ADDSUBPS\0" \
"\x09" "VADDSUBPD\0" "\x09" "VADDSUBPS\0" "\x05" "PSRLW\0" "\x06" "VPSRLW\0" \
"\x05" "PSRLD\0" "\x06" "VPSRLD\0" "\x05" "PSRLQ\0" "\x06" "VPSRLQ\0" "\x05" "PADDQ\0" \
"\x06" "VPADDQ\0" "\x06" "PMULLW\0" "\x07" "VPMULLW\0" "\x07" "MOVQ2DQ\0" "\x07" "MOVDQ2Q\0" \
"\x08" "PMOVMSKB\0" "\x09" "VPMOVMSKB\0" "\x07" "PSUBUSB\0" "\x08" "VPSUBUSB\0" \
"\x07" "PSUBUSW\0" "\x08" "VPSUBUSW\0" "\x06" "PMINUB\0" "\x07" "VPMINUB\0" \
"\x04" "PAND\0" "\x05" "VPAND\0" "\x07" "PADDUSB\0" "\x08" "VPADDUSW\0" "\x07" "PADDUSW\0" \
"\x06" "PMAXUB\0" "\x07" "VPMAXUB\0" "\x05" "PANDN\0" "\x06" "VPANDN\0" "\x05" "PAVGB\0" \
"\x06" "VPAVGB\0" "\x05" "PSRAW\0" "\x06" "VPSRAW\0" "\x05" "PSRAD\0" "\x06" "VPSRAD\0" \
"\x05" "PAVGW\0" "\x06" "VPAVGW\0" "\x07" "PMULHUW\0" "\x08" "VPMULHUW\0" "\x06" "PMULHW\0" \
"\x07" "VPMULHW\0" "\x09" "CVTTPD2DQ\0" "\x08" "CVTDQ2PD\0" "\x08" "CVTPD2DQ\0" \
"\x0a" "VCVTTPD2DQ\0" "\x09" "VCVTDQ2PD\0" "\x09" "VCVTPD2DQ\0" "\x06" "MOVNTQ\0" \
"\x07" "MOVNTDQ\0" "\x08" "VMOVNTDQ\0" "\x06" "PSUBSB\0" "\x07" "VPSUBSB\0" \
"\x06" "PSUBSW\0" "\x07" "VPSUBSW\0" "\x06" "PMINSW\0" "\x07" "VPMINSW\0" "\x03" "POR\0" \
"\x04" "VPOR\0" "\x06" "PADDSB\0" "\x07" "VPADDSB\0" "\x06" "PADDSW\0" "\x07" "VPADDSW\0" \
"\x06" "PMAXSW\0" "\x07" "VPMAXSW\0" "\x04" "PXOR\0" "\x05" "VPXOR\0" "\x05" "LDDQU\0" \
"\x06" "VLDDQU\0" "\x05" "PSLLW\0" "\x06" "VPSLLW\0" "\x05" "PSLLD\0" "\x06" "VPSLLD\0" \
"\x05" "PSLLQ\0" "\x06" "VPSLLQ\0" "\x07" "PMULUDQ\0" "\x08" "VPMULUDQ\0" "\x07" "PMADDWD\0" \
"\x08" "VPMADDWD\0" "\x06" "PSADBW\0" "\x07" "VPSADBW\0" "\x08" "MASKMOVQ\0" \
"\x0a" "MASKMOVDQU\0" "\x0b" "VMASKMOVDQU\0" "\x05" "PSUBB\0" "\x06" "VPSUBB\0" \
"\x05" "PSUBW\0" "\x06" "VPSUBW\0" "\x05" "PSUBD\0" "\x06" "VPSUBD\0" "\x05" "PSUBQ\0" \
"\x06" "VPSUBQ\0" "\x05" "PADDB\0" "\x06" "VPADDB\0" "\x05" "PADDW\0" "\x06" "VPADDW\0" \
"\x05" "PADDD\0" "\x06" "VPADDD\0" "\x07" "FNSTENV\0" "\x06" "FSTENV\0" "\x06" "FNSTCW\0" \
"\x05" "FSTCW\0" "\x06" "FNCLEX\0" "\x05" "FCLEX\0" "\x06" "FNINIT\0" "\x05" "FINIT\0" \
"\x06" "FNSAVE\0" "\x05" "FSAVE\0" "\x06" "FNSTSW\0" "\x05" "FSTSW\0" "\x06" "PSHUFB\0" \
"\x07" "VPSHUFB\0" "\x06" "PHADDW\0" "\x07" "VPHADDW\0" "\x06" "PHADDD\0" "\x07" "VPHADDD\0" \
"\x07" "PHADDSW\0" "\x08" "VPHADDSW\0" "\x09" "PMADDUBSW\0" "\x0a" "VPMADDUBSW\0" \
"\x06" "PHSUBW\0" "\x07" "VPHSUBW\0" "\x06" "PHSUBD\0" "\x07" "VPHSUBD\0" "\x07" "PHSUBSW\0" \
"\x08" "VPHSUBSW\0" "\x06" "PSIGNB\0" "\x07" "VPSIGNB\0" "\x06" "PSIGNW\0" \
"\x07" "VPSIGNW\0" "\x06" "PSIGND\0" "\x07" "VPSIGND\0" "\x08" "PMULHRSW\0" \
"\x09" "VPMULHRSW\0" "\x09" "VPERMILPS\0" "\x09" "VPERMILPD\0" "\x07" "VTESTPS\0" \
"\x07" "VTESTPD\0" "\x08" "PBLENDVB\0" "\x08" "BLENDVPS\0" "\x08" "BLENDVPD\0" \
"\x05" "PTEST\0" "\x06" "VPTEST\0" "\x0c" "VBROADCASTSS\0" "\x0c" "VBROADCASTSD\0" \
"\x0e" "VBROADCASTF128\0" "\x05" "PABSB\0" "\x06" "VPABSB\0" "\x05" "PABSW\0" \
"\x06" "VPABSW\0" "\x05" "PABSD\0" "\x06" "VPABSD\0" "\x08" "PMOVSXBW\0" "\x09" "VPMOVSXBW\0" \
"\x08" "PMOVSXBD\0" "\x09" "VPMOVSXBD\0" "\x08" "PMOVSXBQ\0" "\x09" "VPMOVSXBQ\0" \
"\x08" "PMOVSXWD\0" "\x09" "VPMOVSXWD\0" "\x08" "PMOVSXWQ\0" "\x09" "VPMOVSXWQ\0" \
"\x08" "PMOVSXDQ\0" "\x09" "VPMOVSXDQ\0" "\x06" "PMULDQ\0" "\x07" "VPMULDQ\0" \
"\x07" "PCMPEQQ\0" "\x08" "VPCMPEQQ\0" "\x08" "MOVNTDQA\0" "\x09" "VMOVNTDQA\0" \
"\x08" "PACKUSDW\0" "\x09" "VPACKUSDW\0" "\x0a" "VMASKMOVPS\0" "\x0a" "VMASKMOVPD\0" \
"\x08" "PMOVZXBW\0" "\x09" "VPMOVZXBW\0" "\x08" "PMOVZXBD\0" "\x09" "VPMOVZXBD\0" \
"\x08" "PMOVZXBQ\0" "\x09" "VPMOVZXBQ\0" "\x08" "PMOVZXWD\0" "\x09" "VPMOVZXWD\0" \
"\x08" "PMOVZXWQ\0" "\x09" "VPMOVZXWQ\0" "\x08" "PMOVZXDQ\0" "\x09" "VPMOVZXDQ\0" \
"\x07" "PCMPGTQ\0" "\x08" "VPCMPGTQ\0" "\x06" "PMINSB\0" "\x07" "VPMINSB\0" \
"\x06" "PMINSD\0" "\x07" "VPMINSD\0" "\x06" "PMINUW\0" "\x07" "VPMINUW\0" "\x06" "PMINUD\0" \
"\x07" "VPMINUD\0" "\x06" "PMAXSB\0" "\x07" "VPMAXSB\0" "\x06" "PMAXSD\0" "\x07" "VPMAXSD\0" \
"\x06" "PMAXUW\0" "\x07" "VPMAXUW\0" "\x06" "PMAXUD\0" "\x07" "VPMAXUD\0" "\x06" "PMULLD\0" \
"\x07" "VPMULLD\0" "\x0a" "PHMINPOSUW\0" "\x0b" "VPHMINPOSUW\0" "\x06" "INVEPT\0" \
"\x07" "INVVPID\0" "\x07" "INVPCID\0" "\x0e" "VFMADDSUB132PS\0" "\x0e" "VFMADDSUB132PD\0" \
"\x0e" "VFMSUBADD132PS\0" "\x0e" "VFMSUBADD132PD\0" "\x0b" "VFMADD132PS\0" \
"\x0b" "VFMADD132PD\0" "\x0b" "VFMADD132SS\0" "\x0b" "VFMADD132SD\0" "\x0b" "VFMSUB132PS\0" \
"\x0b" "VFMSUB132PD\0" "\x0b" "VFMSUB132SS\0" "\x0b" "VFMSUB132SD\0" "\x0c" "VFNMADD132PS\0" \
"\x0c" "VFNMADD132PD\0" "\x0c" "VFNMADD132SS\0" "\x0c" "VFNMADD132SD\0" "\x0c" "VFNMSUB132PS\0" \
"\x0c" "VFNMSUB132PD\0" "\x0c" "VFNMSUB132SS\0" "\x0c" "VFNMSUB132SD\0" "\x0e" "VFMADDSUB213PS\0" \
"\x0e" "VFMADDSUB213PD\0" "\x0e" "VFMSUBADD213PS\0" "\x0e" "VFMSUBADD213PD\0" \
"\x0b" "VFMADD213PS\0" "\x0b" "VFMADD213PD\0" "\x0b" "VFMADD213SS\0" "\x0b" "VFMADD213SD\0" \
"\x0b" "VFMSUB213PS\0" "\x0b" "VFMSUB213PD\0" "\x0b" "VFMSUB213SS\0" "\x0b" "VFMSUB213SD\0" \
"\x0c" "VFNMADD213PS\0" "\x0c" "VFNMADD213PD\0" "\x0c" "VFNMADD213SS\0" "\x0c" "VFNMADD213SD\0" \
"\x0c" "VFNMSUB213PS\0" "\x0c" "VFNMSUB213PD\0" "\x0c" "VFNMSUB213SS\0" "\x0c" "VFNMSUB213SD\0" \
"\x0e" "VFMADDSUB231PS\0" "\x0e" "VFMADDSUB231PD\0" "\x0e" "VFMSUBADD231PS\0" \
"\x0e" "VFMSUBADD231PD\0" "\x0b" "VFMADD231PS\0" "\x0b" "VFMADD231PD\0" "\x0b" "VFMADD231SS\0" \
"\x0b" "VFMADD231SD\0" "\x0b" "VFMSUB231PS\0" "\x0b" "VFMSUB231PD\0" "\x0b" "VFMSUB231SS\0" \
"\x0b" "VFMSUB231SD\0" "\x0c" "VFNMADD231PS\0" "\x0c" "VFNMADD231PD\0" "\x0c" "VFNMADD231SS\0" \
"\x0c" "VFNMADD231SD\0" "\x0c" "VFNMSUB231PS\0" "\x0c" "VFNMSUB231PD\0" "\x0c" "VFNMSUB231SS\0" \
"\x0c" "VFNMSUB231SD\0" "\x06" "AESIMC\0" "\x07" "VAESIMC\0" "\x06" "AESENC\0" \
"\x07" "VAESENC\0" "\x0a" "AESENCLAST\0" "\x0b" "VAESENCLAST\0" "\x06" "AESDEC\0" \
"\x07" "VAESDEC\0" "\x0a" "AESDECLAST\0" "\x0b" "VAESDECLAST\0" "\x05" "MOVBE\0" \
"\x05" "CRC32\0" "\x0a" "VPERM2F128\0" "\x07" "ROUNDPS\0" "\x08" "VROUNDPS\0" \
"\x07" "ROUNDPD\0" "\x08" "VROUNDPD\0" "\x07" "ROUNDSS\0" "\x08" "VROUNDSS\0" \
"\x07" "ROUNDSD\0" "\x08" "VROUNDSD\0" "\x07" "BLENDPS\0" "\x08" "VBLENDPS\0" \
"\x07" "BLENDPD\0" "\x08" "VBLENDPD\0" "\x07" "PBLENDW\0" "\x08" "VPBLENDW\0" \
"\x07" "PALIGNR\0" "\x08" "VPALIGNR\0" "\x06" "PEXTRB\0" "\x07" "VPEXTRB\0" \
"\x06" "PEXTRD\0" "\x06" "PEXTRQ\0" "\x07" "VPEXTRD\0" "\x07" "VPEXTRQ\0" "\x09" "EXTRACTPS\0" \
"\x0a" "VEXTRACTPS\0" "\x0b" "VINSERTF128\0" "\x0c" "VEXTRACTF128\0" "\x06" "PINSRB\0" \
"\x07" "VPINSRB\0" "\x08" "INSERTPS\0" "\x09" "VINSERTPS\0" "\x06" "PINSRD\0" \
"\x06" "PINSRQ\0" "\x07" "VPINSRD\0" "\x07" "VPINSRQ\0" "\x04" "DPPS\0" "\x05" "VDPPS\0" \
"\x04" "DPPD\0" "\x05" "VDPPD\0" "\x07" "MPSADBW\0" "\x08" "VMPSADBW\0" "\x09" "PCLMULQDQ\0" \
"\x0a" "VPCLMULQDQ\0" "\x09" "VBLENDVPS\0" "\x09" "VBLENDVPD\0" "\x09" "VPBLENDVB\0" \
"\x09" "PCMPESTRM\0" "\x0a" "VPCMPESTRM\0" "\x09" "PCMPESTRI\0" "\x0a" "VPCMPESTRI\0" \
"\x09" "PCMPISTRM\0" "\x0a" "VPCMPISTRM\0" "\x09" "PCMPISTRI\0" "\x0a" "VPCMPISTRI\0" \
"\x0f" "AESKEYGENASSIST\0" "\x10" "VAESKEYGENASSIST\0" "\x06" "PSRLDQ\0" "\x07" "VPSRLDQ\0" \
"\x06" "PSLLDQ\0" "\x07" "VPSLLDQ\0" "\x06" "FXSAVE\0" "\x08" "FXSAVE64\0" \
"\x08" "RDFSBASE\0" "\x07" "FXRSTOR\0" "\x09" "FXRSTOR64\0" "\x08" "RDGSBASE\0" \
"\x07" "LDMXCSR\0" "\x08" "WRFSBASE\0" "\x08" "VLDMXCSR\0" "\x07" "STMXCSR\0" \
"\x08" "WRGSBASE\0" "\x08" "VSTMXCSR\0" "\x06" "RDRAND\0" "\x07" "VMPTRLD\0" \
"\x07" "VMCLEAR\0" "\x05" "VMXON\0" "\x06" "MOVSXD\0" "\x05" "PAUSE\0" "\x04" "WAIT\0" \
"\x06" "_3DNOW\0";

const _WRegister _REGISTERS[] = {
	{3, "RAX"}, {3, "RCX"}, {3, "RDX"}, {3, "RBX"}, {3, "RSP"}, {3, "RBP"}, {3, "RSI"}, {3, "RDI"}, {2, "R8"}, {2, "R9"}, {3, "R10"}, {3, "R11"}, {3, "R12"}, {3, "R13"}, {3, "R14"}, {3, "R15"},
	{3, "EAX"}, {3, "ECX"}, {3, "EDX"}, {3, "EBX"}, {3, "ESP"}, {3, "EBP"}, {3, "ESI"}, {3, "EDI"}, {3, "R8D"}, {3, "R9D"}, {4, "R10D"}, {4, "R11D"}, {4, "R12D"}, {4, "R13D"}, {4, "R14D"}, {4, "R15D"},
	{2, "AX"}, {2, "CX"}, {2, "DX"}, {2, "BX"}, {2, "SP"}, {2, "BP"}, {2, "SI"}, {2, "DI"}, {3, "R8W"}, {3, "R9W"}, {4, "R10W"}, {4, "R11W"}, {4, "R12W"}, {4, "R13W"}, {4, "R14W"}, {4, "R15W"},
	{2, "AL"}, {2, "CL"}, {2, "DL"}, {2, "BL"}, {2, "AH"}, {2, "CH"}, {2, "DH"}, {2, "BH"}, {3, "R8B"}, {3, "R9B"}, {4, "R10B"}, {4, "R11B"}, {4, "R12B"}, {4, "R13B"}, {4, "R14B"}, {4, "R15B"},
	{3, "SPL"}, {3, "BPL"}, {3, "SIL"}, {3, "DIL"},
	{2, "ES"}, {2, "CS"}, {2, "SS"}, {2, "DS"}, {2, "FS"}, {2, "GS"},
	{3, "RIP"},
	{3, "ST0"}, {3, "ST1"}, {3, "ST2"}, {3, "ST3"}, {3, "ST4"}, {3, "ST5"}, {3, "ST6"}, {3, "ST7"},
	{3, "MM0"}, {3, "MM1"}, {3, "MM2"}, {3, "MM3"}, {3, "MM4"}, {3, "MM5"}, {3, "MM6"}, {3, "MM7"},
	{4, "XMM0"}, {4, "XMM1"}, {4, "XMM2"}, {4, "XMM3"}, {4, "XMM4"}, {4, "XMM5"}, {4, "XMM6"}, {4, "XMM7"}, {4, "XMM8"}, {4, "XMM9"}, {5, "XMM10"}, {5, "XMM11"}, {5, "XMM12"}, {5, "XMM13"}, {5, "XMM14"}, {5, "XMM15"},
	{4, "YMM0"}, {4, "YMM1"}, {4, "YMM2"}, {4, "YMM3"}, {4, "YMM4"}, {4, "YMM5"}, {4, "YMM6"}, {4, "YMM7"}, {4, "YMM8"}, {4, "YMM9"}, {5, "YMM10"}, {5, "YMM11"}, {5, "YMM12"}, {5, "YMM13"}, {5, "YMM14"}, {5, "YMM15"},
	{3, "CR0"}, {0, ""}, {3, "CR2"}, {3, "CR3"}, {3, "CR4"}, {0, ""}, {0, ""}, {0, ""}, {3, "CR8"},
	{3, "DR0"}, {3, "DR1"}, {3, "DR2"}, {3, "DR3"}, {0, ""}, {0, ""}, {3, "DR6"}, {3, "DR7"}
};

#endif /* DISTORM_LIGHT */
