// Capstone Java binding
// By Nguyen Anh Quynh & Dang Hoang Vu,  2013

package capstone;

import com.sun.jna.Library;
import com.sun.jna.Native;
import com.sun.jna.NativeLong;
import com.sun.jna.ptr.NativeLongByReference;
import com.sun.jna.Structure;
import com.sun.jna.Union;
import com.sun.jna.Pointer;
import com.sun.jna.ptr.PointerByReference;
import com.sun.jna.ptr.IntByReference;

import java.util.List;
import java.util.Arrays;
import java.lang.RuntimeException;

public class Capstone {

  protected static abstract class OpInfo {};
  protected static abstract class UnionOpInfo extends Structure {};

  public static class UnionArch extends Union {
    public static class ByValue extends UnionArch implements Union.ByValue {};

    public Arm.UnionOpInfo arm;
    public Arm64.UnionOpInfo arm64;
    public X86.UnionOpInfo x86;
    public Mips.UnionOpInfo mips;
    public Ppc.UnionOpInfo ppc;
  }

  protected static class _cs_insn extends Structure {
    // instruction ID.
    public int id;
    // instruction address.
    public long address;
    // instruction size.
    public short size;
    // machine bytes of instruction.
    public byte[] bytes;
    // instruction mnemonic. NOTE: irrelevant for diet engine.
    public byte[] mnemonic;
    // instruction operands. NOTE: irrelevant for diet engine.
    public byte[] operands;
    // detail information of instruction.
    public _cs_detail.ByReference cs_detail;

    public _cs_insn() {
      bytes = new byte[16];
      mnemonic = new byte[32];
      operands = new byte[160];
    }

    public _cs_insn(Pointer p) {
      this();
      useMemory(p);
      read();
    }

    @Override
    public List getFieldOrder() {
      return Arrays.asList("id", "address", "size", "bytes", "mnemonic", "operands", "cs_detail");
    }
  }

  protected static class _cs_detail extends Structure {
    public static class ByReference extends _cs_detail implements Structure.ByReference {};

    // list of all implicit registers being read.
    public byte[] regs_read = new byte[12];
    public byte regs_read_count;
    // list of all implicit registers being written.
    public byte[] regs_write = new byte[20];
    public byte regs_write_count;
    // list of semantic groups this instruction belongs to.
    public byte[] groups = new byte[8];
    public byte groups_count;

    public UnionArch arch;

    @Override
    public List getFieldOrder() {
      return Arrays.asList("regs_read", "regs_read_count", "regs_write", "regs_write_count", "groups", "groups_count", "arch");
    }
  }

  public static class CsInsn {
    private NativeLong csh;
    private CS cs;
    private _cs_insn raw;
    private int arch;

    // instruction ID.
    public int id;
    // instruction address.
    public long address;
    // instruction size.
    public short size;
    // instruction mnemonic. NOTE: irrelevant for diet engine.
    public String mnemonic;
    // instruction operands. NOTE: irrelevant for diet engine.
    public String opStr;
    // list of all implicit registers being read.
    public byte[] regsRead;
    // list of all implicit registers being written.
    public byte[] regsWrite;
    // list of semantic groups this instruction belongs to.
    public byte[] groups;
    public OpInfo operands;

    public CsInsn (_cs_insn insn, int _arch, NativeLong _csh, CS _cs, boolean diet) {
      id = insn.id;
      address = insn.address;
      size = insn.size;

      if (!diet) {
        mnemonic = new String(insn.mnemonic).replace("\u0000","");
        opStr = new String(insn.operands).replace("\u0000","");
      }

      cs = _cs;
      arch = _arch;
      raw = insn;
      csh = _csh;

      if (insn.cs_detail != null) {
        if (!diet) {
          regsRead = new byte[insn.cs_detail.regs_read_count];
          for (int i=0; i<regsRead.length; i++)
            regsRead[i] = insn.cs_detail.regs_read[i];
          regsWrite = new byte[insn.cs_detail.regs_write_count];
          for (int i=0; i<regsWrite.length; i++)
            regsWrite[i] = insn.cs_detail.regs_write[i];
          groups = new byte[insn.cs_detail.groups_count];
          for (int i=0; i<groups.length; i++)
            groups[i] = insn.cs_detail.groups[i];
        }

        operands = getOptInfo(insn.cs_detail);
      }
    }

    private OpInfo getOptInfo(_cs_detail detail) {
      OpInfo op_info = null;

      switch (this.arch) {
        case CS_ARCH_ARM:
          detail.arch.setType(Arm.UnionOpInfo.class);
          detail.arch.read();
          op_info = new Arm.OpInfo((Arm.UnionOpInfo) detail.arch.arm);
          break;
        case CS_ARCH_ARM64:
          detail.arch.setType(Arm64.UnionOpInfo.class);
          detail.arch.read();
          op_info = new Arm64.OpInfo((Arm64.UnionOpInfo) detail.arch.arm64);
          break;
        case CS_ARCH_MIPS:
          detail.arch.setType(Mips.UnionOpInfo.class);
          detail.arch.read();
          op_info = new Mips.OpInfo((Mips.UnionOpInfo) detail.arch.mips);
          break;
        case CS_ARCH_X86:
          detail.arch.setType(X86.UnionOpInfo.class);
          detail.arch.read();
          op_info = new X86.OpInfo((X86.UnionOpInfo) detail.arch.x86);
          break;
        case CS_ARCH_PPC:
          detail.arch.setType(Ppc.UnionOpInfo.class);
          detail.arch.read();
          op_info = new Ppc.OpInfo((Ppc.UnionOpInfo) detail.arch.ppc);
          break;
        default:
      }

      return op_info;
    }

    public int opCount(int type) {
      return cs.cs_op_count(csh, raw.getPointer(), type);
    }

    public int opIndex(int type, int index) {
      return cs.cs_op_index(csh, raw.getPointer(), type, index);
    }

    public boolean regRead(int reg_id) {
      return cs.cs_reg_read(csh, raw.getPointer(), reg_id) != 0;
    }

    public boolean regWrite(int reg_id) {
      return cs.cs_reg_write(csh, raw.getPointer(), reg_id) != 0;
    }

    public int errno() {
      return cs.cs_errno(csh);
    }

    public String regName(int reg_id) {
      return cs.cs_reg_name(csh, reg_id);
    }

    public String insnName() {
      return cs.cs_insn_name(csh, id);
    }

    public boolean group(int gid) {
      return cs.cs_insn_group(csh, raw.getPointer(), gid) != 0;
    }

  }

  private CsInsn[] fromArrayRaw(_cs_insn[] arr_raw) {
    CsInsn[] arr = new CsInsn[arr_raw.length];

    for (int i = 0; i < arr_raw.length; i++) {
      arr[i] = new CsInsn(arr_raw[i], this.arch, ns.csh, cs, this.diet);
    }

    return arr;
  }

  private interface CS extends Library {
    public int cs_open(int arch, int mode, NativeLongByReference handle);
    public NativeLong cs_disasm_ex(NativeLong handle, byte[] code, NativeLong code_len,
        long addr, NativeLong count, PointerByReference insn);
    public void cs_free(Pointer p, NativeLong count);
    public int cs_close(NativeLongByReference handle);
    public int cs_option(NativeLong handle, int option, NativeLong optionValue);

    public String cs_reg_name(NativeLong csh, int id);
    public int cs_op_count(NativeLong csh, Pointer insn, int type);
    public int cs_op_index(NativeLong csh, Pointer insn, int type, int index);

    public String cs_insn_name(NativeLong csh, int id);
    public byte cs_insn_group(NativeLong csh, Pointer insn, int id);
    public byte cs_reg_read(NativeLong csh, Pointer insn, int id);
    public byte cs_reg_write(NativeLong csh, Pointer insn, int id);
    public int cs_errno(NativeLong csh);
    public int cs_version(IntByReference major, IntByReference minor);
    public boolean cs_support(int query);
  }

  // Capstone API version
  public static final int CS_API_MAJOR = 2;
  public static final int CS_API_MINOR = 1;

  // architectures
  public static final int CS_ARCH_ARM = 0;
  public static final int CS_ARCH_ARM64 = 1;
  public static final int CS_ARCH_MIPS = 2;
  public static final int CS_ARCH_X86 = 3;
  public static final int CS_ARCH_PPC = 4;
  public static final int CS_ARCH_ALL = 0xFFFF; // query id for cs_support()

  // disasm mode
  public static final int CS_MODE_LITTLE_ENDIAN = 0;  // default mode
  public static final int CS_MODE_ARM = 0;	          // 32-bit ARM
  public static final int CS_MODE_16 = 1 << 1;
  public static final int CS_MODE_32 = 1 << 2;
  public static final int CS_MODE_64 = 1 << 3;
  public static final int CS_MODE_THUMB = 1 << 4;	      // ARM's Thumb mode, including Thumb-2
  public static final int CS_MODE_MICRO = 1 << 4;	      // MicroMips mode (Mips arch)
  public static final int CS_MODE_N64 = 1 << 5;	      // Nintendo-64 mode (Mips arch)
  public static final int CS_MODE_BIG_ENDIAN = 1 << 31;

  // Capstone error
  public static final int CS_ERR_OK = 0;
  public static final int CS_ERR_MEM = 1;	    // Out-Of-Memory error
  public static final int CS_ERR_ARCH = 2;	  // Unsupported architecture
  public static final int CS_ERR_HANDLE = 3;	// Invalid handle
  public static final int CS_ERR_CSH = 4;	    // Invalid csh argument
  public static final int CS_ERR_MODE = 5;	  // Invalid/unsupported mode
  public static final int CS_ERR_OPTION = 6;  // Invalid/unsupported option: cs_option()
  public static final int CS_ERR_DETAIL = 7;  // Invalid/unsupported option: cs_option()
  public static final int CS_ERR_MEMSETUP = 8;
  public static final int CS_ERR_VERSION = 9;  //Unsupported version (bindings)
  public static final int CS_ERR_DIET = 10;  //Information irrelevant in diet engine

  // Capstone option type
  public static final int CS_OPT_SYNTAX = 1;  // Intel X86 asm syntax (CS_ARCH_X86 arch)
  public static final int CS_OPT_DETAIL = 2;  // Break down instruction structure into details
  public static final int CS_OPT_MODE = 3;  // Change engine's mode at run-time

  // Capstone option value
  public static final int CS_OPT_OFF = 0;  // Turn OFF an option - default option of CS_OPT_DETAIL
  public static final int CS_OPT_SYNTAX_INTEL = 1;  // Intel X86 asm syntax - default syntax on X86 (CS_OPT_SYNTAX,  CS_ARCH_X86)
  public static final int CS_OPT_SYNTAX_ATT = 2;    // ATT asm syntax (CS_OPT_SYNTAX, CS_ARCH_X86)
  public static final int CS_OPT_ON = 3;  // Turn ON an option (CS_OPT_DETAIL)
  public static final int CS_OPT_SYNTAX_NOREGNAME = 3; // PPC asm syntax: Prints register name with only number (CS_OPT_SYNTAX)

  // query id for cs_support()
  public static final int CS_SUPPORT_DIET = CS_ARCH_ALL+1;	  // diet mode

  protected class NativeStruct {
      private NativeLong csh;
      private NativeLongByReference handleRef;
  }

  protected NativeStruct ns; // for memory retention
  private CS cs;
  public int arch;
  public int mode;
  private int syntax;
  private int detail;
  private boolean diet;

  public Capstone(int arch, int mode) {
    cs = (CS)Native.loadLibrary("capstone", CS.class);
    int version = cs.cs_version(null, null);
    if (version != (CS_API_MAJOR << 8) + CS_API_MINOR) {
      throw new RuntimeException("Different API version between core & binding (CS_ERR_VERSION)");
    }

    this.arch = arch;
    this.mode = mode;
    ns = new NativeStruct();
    ns.handleRef = new NativeLongByReference();
    if (cs.cs_open(arch, mode, ns.handleRef) != CS_ERR_OK) {
      throw new RuntimeException("ERROR: Wrong arch or mode");
    }
    ns.csh = ns.handleRef.getValue();
    this.detail = CS_OPT_OFF;
	this.diet = cs.cs_support(CS_SUPPORT_DIET);
  }

  // return combined API version
  public int version() {
    return cs.cs_version(null, null);
  }

  // set Assembly syntax
  public void setSyntax(int syntax) {
    if (cs.cs_option(ns.csh, CS_OPT_SYNTAX, new NativeLong(syntax)) == CS_ERR_OK) {
      this.syntax = syntax;
    } else {
      throw new RuntimeException("ERROR: Failed to set assembly syntax");
    }
  }

  // set detail option at run-time
  public void setDetail(int opt) {
    if (cs.cs_option(ns.csh, CS_OPT_DETAIL, new NativeLong(opt)) == CS_ERR_OK) {
      this.detail = opt;
    } else {
      throw new RuntimeException("ERROR: Failed to set detail option");
    }
  }

  // set mode option at run-time
  public void setMode(int opt) {
    if (cs.cs_option(ns.csh, CS_OPT_MODE, new NativeLong(opt)) == CS_ERR_OK) {
      this.mode = opt;
    } else {
      throw new RuntimeException("ERROR: Failed to set mode option");
    }
  }

  // destructor automatically caled at destroyed time.
  protected void finalize() {
    cs.cs_close(ns.handleRef);
  }

  // disassemble until either no more code, or encounter broken insn.
  public CsInsn[] disasm(byte[] code, long address) {
    return disasm(code, address, 0);
  }

  // disassemble maximum @count instructions, or until encounter broken insn.
  public CsInsn[] disasm(byte[] code, long address, long count) {
    PointerByReference insnRef = new PointerByReference();

    NativeLong c = cs.cs_disasm_ex(ns.csh, code, new NativeLong(code.length), address, new NativeLong(count), insnRef);

    Pointer p = insnRef.getValue();
    _cs_insn byref = new _cs_insn(p);

    CsInsn[] allInsn = fromArrayRaw((_cs_insn[]) byref.toArray(c.intValue()));

    // free allocated memory
    cs.cs_free(p, c);

    return allInsn;
  }
}

