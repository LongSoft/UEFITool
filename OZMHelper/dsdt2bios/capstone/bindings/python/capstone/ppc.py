# Capstone Python bindings, by Nguyen Anh Quynnh <aquynh@gmail.com>

import ctypes, copy
from ppc_const import *

# define the API
class PpcOpMem(ctypes.Structure):
    _fields_ = (
        ('base', ctypes.c_uint),
        ('disp', ctypes.c_int32),
    )

class PpcOpValue(ctypes.Union):
    _fields_ = (
        ('reg', ctypes.c_uint),
        ('imm', ctypes.c_int32),
        ('mem', PpcOpMem),
    )

class PpcOp(ctypes.Structure):
    _fields_ = (
        ('type', ctypes.c_uint),
        ('value', PpcOpValue),
    )

    @property
    def imm(self):
        return self.value.imm

    @property
    def reg(self):
        return self.value.reg

    @property
    def mem(self):
        return self.value.mem


class CsPpc(ctypes.Structure):
    _fields_ = (
        ('bc', ctypes.c_uint),
        ('bh', ctypes.c_uint),
        ('update_cr0', ctypes.c_bool),
        ('op_count', ctypes.c_uint8),
        ('operands', PpcOp * 8),
    )

def get_arch_info(a):
    return (a.bc, a.bh, a.update_cr0, copy.deepcopy(a.operands[:a.op_count]))

