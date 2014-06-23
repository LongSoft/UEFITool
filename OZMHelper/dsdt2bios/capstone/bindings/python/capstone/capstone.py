# Capstone Python bindings, by Nguyen Anh Quynnh <aquynh@gmail.com>

import arm, arm64, mips, x86, ppc

__all__ = [
    'Cs',
    'CsInsn',

    'cs_disasm_quick',
    'cs_version',
    'cs_support',
    'version_bind',
    'debug',

    'CS_API_MAJOR',
    'CS_API_MINOR',

    'CS_ARCH_ARM',
    'CS_ARCH_ARM64',
    'CS_ARCH_MIPS',
    'CS_ARCH_X86',
    'CS_ARCH_PPC',
    'CS_ARCH_ALL',

    'CS_MODE_LITTLE_ENDIAN',
    'CS_MODE_BIG_ENDIAN',
    'CS_MODE_16',
    'CS_MODE_32',
    'CS_MODE_64',
    'CS_MODE_ARM',
    'CS_MODE_THUMB',
    'CS_MODE_MICRO',
    'CS_MODE_N64',

    'CS_OPT_SYNTAX',
    'CS_OPT_SYNTAX_DEFAULT',
    'CS_OPT_SYNTAX_INTEL',
    'CS_OPT_SYNTAX_ATT',
    'CS_OPT_SYNTAX_NOREGNAME',

    'CS_OPT_DETAIL',
    'CS_OPT_MODE',
    'CS_OPT_ON',
    'CS_OPT_OFF',

    'CS_ERR_OK',
    'CS_ERR_MEM',
    'CS_ERR_ARCH',
    'CS_ERR_HANDLE',
    'CS_ERR_CSH',
    'CS_ERR_MODE',
    'CS_ERR_OPTION',
    'CS_ERR_DETAIL',
    'CS_ERR_VERSION',
    'CS_ERR_MEMSETUP',
    'CS_ERR_DIET',

    'CS_SUPPORT_DIET',
]

# Capstone C interface

# API version
CS_API_MAJOR = 2
CS_API_MINOR = 1

# architectures
CS_ARCH_ARM = 0
CS_ARCH_ARM64 = 1
CS_ARCH_MIPS = 2
CS_ARCH_X86 = 3
CS_ARCH_PPC = 4
CS_ARCH_ALL = 0xFFFF

# disasm mode
CS_MODE_LITTLE_ENDIAN = 0      # little-endian mode (default mode)
CS_MODE_ARM = 0                # ARM mode
CS_MODE_16 = (1 << 1)          # 16-bit mode (for X86, Mips)
CS_MODE_32 = (1 << 2)          # 32-bit mode (for X86, Mips)
CS_MODE_64 = (1 << 3)          # 64-bit mode (for X86, Mips)
CS_MODE_THUMB = (1 << 4)       # ARM's Thumb mode, including Thumb-2
CS_MODE_MICRO = (1 << 4)       # MicroMips mode (MIPS architecture)
CS_MODE_N64 = (1 << 5)         # Nintendo-64 mode (MIPS architecture)
CS_MODE_BIG_ENDIAN = (1 << 31) # big-endian mode

# Capstone option type
CS_OPT_SYNTAX = 1    # Intel X86 asm syntax (CS_ARCH_X86 arch)
CS_OPT_DETAIL = 2    # Break down instruction structure into details
CS_OPT_MODE = 3      # Change engine's mode at run-time

# Capstone option value
CS_OPT_OFF = 0             # Turn OFF an option - default option of CS_OPT_DETAIL
CS_OPT_ON = 3              # Turn ON an option (CS_OPT_DETAIL)

# Capstone syntax value
CS_OPT_SYNTAX_DEFAULT = 0    # Default assembly syntax of all platforms (CS_OPT_SYNTAX)
CS_OPT_SYNTAX_INTEL = 1    # Intel X86 asm syntax - default syntax on X86 (CS_OPT_SYNTAX, CS_ARCH_X86)
CS_OPT_SYNTAX_ATT = 2      # ATT asm syntax (CS_OPT_SYNTAX, CS_ARCH_X86)
CS_OPT_SYNTAX_NOREGNAME = 3   # Asm syntax prints register name with only number - (CS_OPT_SYNTAX, CS_ARCH_PPC)

# Capstone error type
CS_ERR_OK = 0      # No error: everything was fine
CS_ERR_MEM = 1     # Out-Of-Memory error: cs_open(), cs_disasm_ex()
CS_ERR_ARCH = 2    # Unsupported architecture: cs_open()
CS_ERR_HANDLE = 3  # Invalid handle: cs_op_count(), cs_op_index()
CS_ERR_CSH = 4     # Invalid csh argument: cs_close(), cs_errno(), cs_option()
CS_ERR_MODE = 5    # Invalid/unsupported mode: cs_open()
CS_ERR_OPTION = 6  # Invalid/unsupported option: cs_option()
CS_ERR_DETAIL = 7  # Invalid/unsupported option: cs_option()
CS_ERR_MEMSETUP = 8
CS_ERR_VERSION = 9 # Unsupported version (bindings)
CS_ERR_DIET = 10 # Information irrelevant in diet engine

# query id for cs_support()
CS_SUPPORT_DIET = CS_ARCH_ALL+1

import ctypes, ctypes.util, sys
from os.path import split, join, dirname
import distutils.sysconfig


import inspect
if not hasattr(sys.modules[__name__], '__file__'):
    __file__ = inspect.getfile(inspect.currentframe())

_lib_path = split(__file__)[0]
_all_libs = ['libcapstone.dll', 'libcapstone.so', 'libcapstone.dylib']
_found = False

for _lib in _all_libs:
    try:
        _lib_file = join(_lib_path, _lib)
        # print "Trying to load:", _lib_file
        _cs = ctypes.cdll.LoadLibrary(_lib_file)
        _found = True
        break
    except OSError:
        pass
if _found == False:
    # try loading from default paths
    for _lib in _all_libs:
        try:
            _cs = ctypes.cdll.LoadLibrary(_lib)
            _found = True
            break
        except OSError:
            pass

if _found == False:
    # last try: loading from python lib directory
    _lib_path = distutils.sysconfig.get_python_lib()
    for _lib in _all_libs:
        try:
            _lib_file = join(_lib_path, 'capstone', _lib)
            # print "Trying to load:", _lib_file
            _cs = ctypes.cdll.LoadLibrary(_lib_file)
            _found = True
            break
        except OSError:
            pass
    if _found == False:
        raise ImportError("ERROR: fail to load the dynamic library.")


# low-level structure for C code
class _cs_arch(ctypes.Union):
    _fields_ = (
        ('arm64', arm64.CsArm64),
        ('arm', arm.CsArm),
        ('mips', mips.CsMips),
        ('x86', x86.CsX86),
        ('ppc', ppc.CsPpc),
    )

class _cs_detail(ctypes.Structure):
    _fields_ = (
        ('regs_read', ctypes.c_ubyte * 12),
        ('regs_read_count', ctypes.c_ubyte),
        ('regs_write', ctypes.c_ubyte * 20),
        ('regs_write_count', ctypes.c_ubyte),
        ('groups', ctypes.c_ubyte * 8),
        ('groups_count', ctypes.c_ubyte),
        ('arch', _cs_arch),
    )

class _cs_insn(ctypes.Structure):
    _fields_ = (
        ('id', ctypes.c_uint),
        ('address', ctypes.c_uint64),
        ('size', ctypes.c_uint16),
        ('bytes', ctypes.c_ubyte * 16),
        ('mnemonic', ctypes.c_char * 32),
        ('op_str', ctypes.c_char * 160),
        ('detail', ctypes.POINTER(_cs_detail)),
    )

# setup all the function prototype
def _setup_prototype(lib, fname, restype, *argtypes):
    getattr(lib, fname).restype = restype
    getattr(lib, fname).argtypes = argtypes

_setup_prototype(_cs, "cs_open", ctypes.c_int, ctypes.c_uint, ctypes.c_uint, ctypes.POINTER(ctypes.c_size_t))
_setup_prototype(_cs, "cs_disasm_ex", ctypes.c_size_t, ctypes.c_size_t, ctypes.POINTER(ctypes.c_char), ctypes.c_size_t, \
        ctypes.c_uint64, ctypes.c_size_t, ctypes.POINTER(ctypes.POINTER(_cs_insn)))
_setup_prototype(_cs, "cs_free", None, ctypes.c_void_p, ctypes.c_size_t)
_setup_prototype(_cs, "cs_close", ctypes.c_int, ctypes.POINTER(ctypes.c_size_t))
_setup_prototype(_cs, "cs_reg_name", ctypes.c_char_p, ctypes.c_size_t, ctypes.c_uint)
_setup_prototype(_cs, "cs_insn_name", ctypes.c_char_p, ctypes.c_size_t, ctypes.c_uint)
_setup_prototype(_cs, "cs_op_count", ctypes.c_int, ctypes.c_size_t, ctypes.POINTER(_cs_insn), ctypes.c_uint)
_setup_prototype(_cs, "cs_op_index", ctypes.c_int, ctypes.c_size_t, ctypes.POINTER(_cs_insn), ctypes.c_uint, ctypes.c_uint)
_setup_prototype(_cs, "cs_errno", ctypes.c_int, ctypes.c_size_t)
_setup_prototype(_cs, "cs_option", ctypes.c_int, ctypes.c_size_t, ctypes.c_int, ctypes.c_size_t)
_setup_prototype(_cs, "cs_version", ctypes.c_int, ctypes.POINTER(ctypes.c_int), ctypes.POINTER(ctypes.c_int))
_setup_prototype(_cs, "cs_support", ctypes.c_bool, ctypes.c_int)
_setup_prototype(_cs, "cs_strerror", ctypes.c_char_p, ctypes.c_int)


# access to error code via @errno of CsError
class CsError(Exception):
    def __init__(self, errno):
        self.errno = errno

    def __str__(self):
        return _cs.cs_strerror(self.errno)


# return the core's version
def cs_version():
    major = ctypes.c_int()
    minor = ctypes.c_int()
    combined = _cs.cs_version(ctypes.byref(major), ctypes.byref(minor))
    return (major.value, minor.value, combined)


# return the binding's version
def version_bind():
    return (CS_API_MAJOR, CS_API_MINOR, (CS_API_MAJOR << 8) + CS_API_MINOR)


def cs_support(query):
    return _cs.cs_support(query)


# dummy class resembling Cs class, just for cs_disasm_quick()
# this class only need to be referenced to via 2 fields: @csh & @arch
class _dummy_cs(object):
    def __init__(self, csh, arch):
        self.csh = csh
        self.arch = arch


# Quick & dirty Python function to disasm raw binary code
# This function return CsInsn objects
# NOTE: you might want to use more efficient Cs class & its methods.
def cs_disasm_quick(arch, mode, code, offset, count = 0):
    # verify version compatibility with the core before doing anything
    (major, minor, _combined) = cs_version()
    if major != CS_API_MAJOR or minor != CS_API_MINOR:
        # our binding version is different from the core's API version
        raise CsError(CS_ERR_VERSION)

    csh = ctypes.c_size_t()
    status = _cs.cs_open(arch, mode, ctypes.byref(csh))
    if status != CS_ERR_OK:
        raise CsError(status)

    all_insn = ctypes.POINTER(_cs_insn)()
    res = _cs.cs_disasm_ex(csh, code, len(code), offset, count, ctypes.byref(all_insn))
    if res > 0:
        for i in xrange(res):
            yield CsInsn(_dummy_cs(csh, arch), all_insn[i])

        _cs.cs_free(all_insn, res)
    else:
        status = _cs.cs_errno(csh)
        if status != CS_ERR_OK:
            raise CsError(status)
        return
        yield

    status = _cs.cs_close(ctypes.byref(csh))
    if status != CS_ERR_OK:
        raise CsError(status)


# Another quick, but lighter function to disasm raw binary code.
# This function is faster than cs_disasm_quick() around 20% because
# cs_disasm_lite() only return tuples of (address, size, mnemonic, op_str),
# rather than CsInsn objects.
# NOTE: you might want to use more efficient Cs class & its methods.
def cs_disasm_lite(arch, mode, code, offset, count = 0):
    # verify version compatibility with the core before doing anything
    (major, minor, _combined) = cs_version()
    if major != CS_API_MAJOR or minor != CS_API_MINOR:
        # our binding version is different from the core's API version
        raise CsError(CS_ERR_VERSION)

    if cs_support(CS_SUPPORT_DIET):
        # Diet engine cannot provide @mnemonic & @op_str
        raise CsError(CS_ERR_DIET)

    csh = ctypes.c_size_t()
    status = _cs.cs_open(arch, mode, ctypes.byref(csh))
    if status != CS_ERR_OK:
        raise CsError(status)

    all_insn = ctypes.POINTER(_cs_insn)()
    res = _cs.cs_disasm_ex(csh, code, len(code), offset, count, ctypes.byref(all_insn))
    if res > 0:
        for i in xrange(res):
            insn = all_insn[i]
            yield (insn.address, insn.size, insn.mnemonic, insn.op_str)

        _cs.cs_free(all_insn, res)
    else:
        status = _cs.cs_errno(csh)
        if status != CS_ERR_OK:
            raise CsError(status)
        return
        yield

    status = _cs.cs_close(ctypes.byref(csh))
    if status != CS_ERR_OK:
        raise CsError(status)


# Python-style class to disasm code
class CsInsn(object):
    def __init__(self, cs, all_info):
        self._raw = all_info
        self._cs = cs

    # return instruction's ID.
    @property
    def id(self):
        return self._raw.id

    # return instruction's address.
    @property
    def address(self):
        return self._raw.address

    # return instruction's size.
    @property
    def size(self):
        return self._raw.size

    # return instruction's machine bytes (which should have @size bytes).
    @property
    def bytes(self):
        return bytearray(self._raw.bytes)[:self._raw.size]

    # return instruction's mnemonic.
    @property
    def mnemonic(self):
        if self._cs._diet:
            # Diet engine cannot provide @mnemonic.
            raise CsError(CS_ERR_DIET)

        return self._raw.mnemonic

    # return instruction's operands (in string).
    @property
    def op_str(self):
        if self._cs._diet:
            # Diet engine cannot provide @op_str.
            raise CsError(CS_ERR_DIET)

        return self._raw.op_str

    # return list of all implicit registers being read.
    @property
    def regs_read(self):
        if self._cs._diet:
            # Diet engine cannot provide @regs_read.
            raise CsError(CS_ERR_DIET)

        if self._cs._detail:
            detail = self._raw.detail.contents
            return detail.regs_read[:detail.regs_read_count]

        raise CsError(CS_ERR_DETAIL)

    # return list of all implicit registers being modified
    @property
    def regs_write(self):
        if self._cs._diet:
            # Diet engine cannot provide @regs_write
            raise CsError(CS_ERR_DIET)

        if self._cs._detail:
            detail = self._raw.detail.contents
            return detail.regs_write[:detail.regs_write_count]

        raise CsError(CS_ERR_DETAIL)

    # return list of semantic groups this instruction belongs to.
    @property
    def groups(self):
        if self._cs._diet:
            # Diet engine cannot provide @groups
            raise CsError(CS_ERR_DIET)

        if self._cs._detail:
            detail = self._raw.detail.contents
            return detail.groups[:detail.groups_count]

        raise CsError(CS_ERR_DETAIL)

    def __gen_detail(self):
        arch = self._cs.arch
        detail = self._raw.detail.contents
        if arch == CS_ARCH_ARM:
            (self.cc, self.update_flags, self.writeback, self.operands) = \
                arm.get_arch_info(detail.arch.arm)
        elif arch == CS_ARCH_ARM64:
            (self.cc, self.update_flags, self.writeback, self.operands) = \
                arm64.get_arch_info(detail.arch.arm64)
        elif arch == CS_ARCH_X86:
            (self.prefix, self.segment, self.opcode, self.op_size, self.addr_size, \
                self.disp_size, self.imm_size, self.modrm, self.sib, self.disp, \
                self.sib_index, self.sib_scale, self.sib_base, self.operands) = x86.get_arch_info(detail.arch.x86)
        elif arch == CS_ARCH_MIPS:
                self.operands = mips.get_arch_info(detail.arch.mips)
        elif arch == CS_ARCH_PPC:
            (self.bc, self.bh, self.update_cr0, self.operands) = \
                ppc.get_arch_info(detail.arch.ppc)

    def __getattr__(self, name):
        if not self._cs._detail:
            raise CsError(CS_ERR_DETAIL)

        attr = object.__getattribute__
        if not attr(self, '_cs')._detail:
            return None
        _dict = attr(self, '__dict__')
        if 'operands' not in _dict:
            self.__gen_detail()
        if name not in _dict:
            return None
        return _dict[name]

    # get the last error code
    def errno(self):
        return _cs.cs_errno(self._cs.csh)

    # get the register name, given the register ID
    def reg_name(self, reg_id):
        if self._cs._diet:
            # Diet engine cannot provide register name
            raise CsError(CS_ERR_DIET)

        return _cs.cs_reg_name(self._cs.csh, reg_id)

    # get the instruction string
    def insn_name(self):
        if self._cs._diet:
            # Diet engine cannot provide instruction name
            raise CsError(CS_ERR_DIET)

        return _cs.cs_insn_name(self._cs.csh, self.id)

    # verify if this insn belong to group with id as @group_id
    def group(self, group_id):
        if self._cs._diet:
            # Diet engine cannot provide group information
            raise CsError(CS_ERR_DIET)

        return group_id in self.groups

    # verify if this instruction implicitly read register @reg_id
    def reg_read(self, reg_id):
        if self._cs._diet:
            # Diet engine cannot provide regs_read information
            raise CsError(CS_ERR_DIET)

        return reg_id in self.regs_read

    # verify if this instruction implicitly modified register @reg_id
    def reg_write(self, reg_id):
        if self._cs._diet:
            # Diet engine cannot provide regs_write information
            raise CsError(CS_ERR_DIET)

        return reg_id in self.regs_write

    # return number of operands having same operand type @op_type
    def op_count(self, op_type):
        c = 0
        for op in self.operands:
            if op.type == op_type:
                c += 1
        return c

    # get the operand at position @position of all operands having the same type @op_type
    def op_find(self, op_type, position):
        c = 0
        for op in self.operands:
            if op.type == op_type:
                c += 1
            if c == position:
                return op


class Cs(object):
    def __init__(self, arch, mode):
        # verify version compatibility with the core before doing anything
        (major, minor, _combined) = cs_version()
        if major != CS_API_MAJOR or minor != CS_API_MINOR:
            self.csh = None
            # our binding version is different from the core's API version
            raise CsError(CS_ERR_VERSION)

        self.arch, self._mode = arch, mode
        self.csh = ctypes.c_size_t()
        status = _cs.cs_open(arch, mode, ctypes.byref(self.csh))
        if status != CS_ERR_OK:
            self.csh = None
            raise CsError(status)

        try:
            import ccapstone
            # rewire disasm to use the faster version
            self.disasm = ccapstone.Cs(self).disasm
        except:
            pass

        if arch == CS_ARCH_X86:
            # Intel syntax is default for X86
            self._syntax = CS_OPT_SYNTAX_INTEL
        else:
            self._syntax = None

        self._detail = False    # by default, do not produce instruction details
        self._diet = cs_support(CS_SUPPORT_DIET)


    # destructor to be called automatically when object is destroyed.
    def __del__(self):
        if self.csh:
            status = _cs.cs_close(ctypes.byref(self.csh))
            if status != CS_ERR_OK:
                raise CsError(status)


    #def option(self, opt_type, opt_value):
    #    return _cs.cs_option(self.csh, opt_type, opt_value)


    # is this a diet engine?
    @property
    def diet(self):
        return self._diet


    # return assembly syntax.
    @property
    def syntax(self):
        return self._syntax


    # setter: modify assembly syntax.
    @syntax.setter
    def syntax(self, style):
        status = _cs.cs_option(self.csh, CS_OPT_SYNTAX, style)
        if status != CS_ERR_OK:
            raise CsError(status)
        # save syntax
        self._syntax = style


    # is detail mode enable?
    @property
    def detail(self):
        return self._detail


    # check to see if this engine supports a particular arch,
    # or diet mode (depending on @query).
    def support(self, query):
        return cs_support(query)


    # modify detail mode.
    @detail.setter
    def detail(self, opt):  # opt is boolean type, so must be either 'True' or 'False'
        if opt == False:
            status = _cs.cs_option(self.csh, CS_OPT_DETAIL, CS_OPT_OFF)
        else:
            status = _cs.cs_option(self.csh, CS_OPT_DETAIL, CS_OPT_ON)
        if status != CS_ERR_OK:
            raise CsError(status)
        # save detail
        self._detail = opt


    # return disassembly mode of this engine.
    @property
    def mode(self):
        return self._mode


    # modify engine's mode at run-time.
    @mode.setter
    def mode(self, opt):  # opt is new disasm mode, of int type
        status = _cs.cs_option(self.csh, CS_OPT_MODE, opt)
        if status != CS_ERR_OK:
            raise CsError(status)
        # save mode
        self._mode = opt


    # Disassemble binary & return disassembled instructions in CsInsn objects
    def disasm(self, code, offset, count = 0):
        all_insn = ctypes.POINTER(_cs_insn)()
        res = _cs.cs_disasm_ex(self.csh, code, len(code), offset, count, ctypes.byref(all_insn))
        if res > 0:
            for i in xrange(res):
                yield CsInsn(self, all_insn[i])
            _cs.cs_free(all_insn, res)
        else:
            status = _cs.cs_errno(self.csh)
            if status != CS_ERR_OK:
                raise CsError(status)
            return
            yield


    # Light function to disassemble binary. This is about 20% faster than disasm() because
    # unlike disasm(), disasm_lite() only return tuples of (address, size, mnemonic, op_str),
    # rather than CsInsn objects.
    def disasm_lite(self, code, offset, count = 0):
        if self._diet:
            # Diet engine cannot provide @mnemonic & @op_str
            raise CsError(CS_ERR_DIET)

        all_insn = ctypes.POINTER(_cs_insn)()
        res = _cs.cs_disasm_ex(self.csh, code, len(code), offset, count, ctypes.byref(all_insn))
        if res > 0:
            for i in xrange(res):
                insn = all_insn[i]
                yield (insn.address, insn.size, insn.mnemonic, insn.op_str)
            _cs.cs_free(all_insn, res)
        else:
            status = _cs.cs_errno(self.csh)
            if status != CS_ERR_OK:
                raise CsError(status)
            return
            yield


# print out debugging info
def debug():
    # is Cython there?
    try:
        import ccapstone
        return ccapstone.debug()
    except:
        # no Cython, fallback to Python code below
        pass

    if cs_support(CS_SUPPORT_DIET):
        diet = "diet"
    else:
        diet = "standard"

    archs = { "arm": CS_ARCH_ARM, "arm64": CS_ARCH_ARM64, \
        "mips": CS_ARCH_MIPS, "ppc": CS_ARCH_PPC, "x86": CS_ARCH_X86 }

    all_archs = ""
    keys = archs.keys()
    keys.sort()
    for k in keys:
        if cs_support(archs[k]):
            all_archs += "-%s" %k

    (major, minor, _combined) = cs_version()

    return "python-%s%s-c%u.%u-b%u.%u" %(diet, all_archs, major, minor, CS_API_MAJOR, CS_API_MINOR)

