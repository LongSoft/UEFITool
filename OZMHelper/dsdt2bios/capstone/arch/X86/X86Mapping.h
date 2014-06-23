/* Capstone Disassembler Engine */
/* By Nguyen Anh Quynh <aquynh@gmail.com>, 2013> */

#ifndef CS_X86_MAP_H
#define CS_X86_MAP_H

#include "../../include/capstone.h"
#include "../../include/x86.h"
#include "../../cs_priv.h"

// map sib_base to x86_reg
x86_reg x86_map_sib_base(int r);

// map sib_index to x86_reg
x86_reg x86_map_sib_index(int r);

// map seg_override to x86_reg
x86_reg x86_map_segment(int r);

// map register name to x86_reg
x86_reg x86_map_regname(const char *reg);

// return name of regiser in friendly string
const char *X86_reg_name(csh handle, unsigned int reg);

// given internal insn id, return public instruction info
void X86_get_insn_id(cs_struct *h, cs_insn *insn, unsigned int id);

// return insn name, given insn id
const char *X86_insn_name(csh handle, unsigned int id);

// post printer for X86.
void X86_post_printer(csh handle, cs_insn *pub_insn, char *insn_asm);

// handle X86 prefixes
bool X86_insn_check_combine(cs_struct *h, cs_insn *insn);

// merge with previous instruction
// this is to handle some 'prefixed' instructions such as LOCK or REP
void X86_insn_combine(cs_struct *h, cs_insn *insn, cs_insn *prev);

// return register of given instruction id
// return 0 if not found
// this is to handle instructions embedding accumulate registers into AsmStrs[]
x86_reg X86_insn_reg(unsigned int id);

extern uint64_t arch_masks[9];

#endif
