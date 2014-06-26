#!/usr/bin/env python

# Capstone Python bindings, by Nguyen Anh Quynnh <aquynh@gmail.com>

from capstone import *
from capstone.ppc import *

PPC_CODE = "\x80\x20\x00\x00\x80\x3f\x00\x00\x10\x43\x23\x0e\xd0\x44\x00\x80\x4c\x43\x22\x02\x2d\x03\x00\x80\x7c\x43\x20\x14\x7c\x43\x20\x93\x4f\x20\x00\x21\x4c\xc8\x00\x21"

all_tests = (
        (CS_ARCH_PPC, CS_MODE_BIG_ENDIAN, PPC_CODE, "PPC-64"),
        )

def to_hex(s):
    return " ".join("0x" + "{0:x}".format(ord(c)).zfill(2) for c in s) # <-- Python 3 is OK

def to_x(s):
    from struct import pack
    if not s: return '0'
    x = pack(">q", s).encode('hex')
    while x[0] == '0': x = x[1:]
    return x

def to_x_32(s):
    from struct import pack
    if not s: return '0'
    x = pack(">i", s).encode('hex')
    while x[0] == '0': x = x[1:]
    return x

### Test class Cs
def test_class():
    def print_insn_detail(insn):
        # print address, mnemonic and operands
        print("0x%x:\t%s\t%s" %(insn.address, insn.mnemonic, insn.op_str))

        if len(insn.operands) > 0:
            print("\top_count: %u" %len(insn.operands))
            c = 0
            for i in insn.operands:
                if i.type == PPC_OP_REG:
                    print("\t\toperands[%u].type: REG = %s" %(c, insn.reg_name(i.reg)))
                if i.type == PPC_OP_IMM:
                    print("\t\toperands[%u].type: IMM = 0x%s" %(c, to_x_32(i.imm)))
                if i.type == PPC_OP_MEM:
                    print("\t\toperands[%u].type: MEM" %c)
                    if i.mem.base != 0:
                        print("\t\t\toperands[%u].mem.base: REG = %s" \
                            %(c, insn.reg_name(i.mem.base)))
                    if i.mem.disp != 0:
                        print("\t\t\toperands[%u].mem.disp: 0x%s" \
                            %(c, to_x_32(i.mem.disp)))
                c += 1

        if insn.bc:
            print("\tBranch code: %u" %insn.bc)
        if insn.bh:
            print("\tBranch hint: %u" %insn.bh)
        if insn.update_cr0:
            print("\tUpdate-CR0: True")

    for (arch, mode, code, comment) in all_tests:
        print("*" * 16)
        print("Platform: %s" %comment)
        print("Code: %s" % to_hex(code))
        print("Disasm:")

        try:
            md = Cs(arch, mode)
            md.detail = True
            for insn in md.disasm(code, 0x1000):
                print_insn_detail(insn)
                print
            print "0x%x:\n" % (insn.address + insn.size)
        except CsError as e:
            print("ERROR: %s" %e)


test_class()
