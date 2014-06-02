#ifndef DSDT2BIOS_H
#define DSDT2BIOS_H

#include <../basetypes.h>

#define debug 1

class Dsdt2Bios
{
public:
    unsigned int getFromAmiBoardInfo(const char *FileName, unsigned char *d,unsigned long *len, unsigned short *Old_Dsdt_Size, unsigned short *Old_Dsdt_Ofs, int Extract);
    unsigned int injectIntoAmiBoardInfo(const char *FileName, unsigned char *d, unsigned long len, unsigned short Old_Dsdt_Size, unsigned short Old_Dsdt_Ofs, unsigned short *reloc_padding);

private:
    csh handle;
    UINT64 insn_detail(csh ud, cs_mode mode, cs_insn *ins);
    int Disass(unsigned char *X86_CODE64, int CodeSize, int size);
};

struct platform
{
    cs_arch arch;
    cs_mode mode;
    unsigned char *code;
    size_t size;
    char *comment;
    cs_opt_type opt_type;
    cs_opt_value opt_value;
};

#endif // DSDT2BIOS_H
