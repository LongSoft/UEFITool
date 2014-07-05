/* Dsdt2Bios.h

Copyright (c) 2014, tuxuser. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#ifndef DSDT2BIOS_H
#define DSDT2BIOS_H

#include <QByteArray>
#include <capstone.h>
#include <../basetypes.h>
#include <common.h>

#define DSDT_HEADER "DSDT"
#define UNPATCHABLE_SECTION ".ROM"

class Dsdt2Bios
{
public:
    UINT8 getDSDTFromAmi(QByteArray amiboard, UINT32 &DSDTOffset, UINT32 &DSDTSize);
    UINT8 injectDSDTIntoAmi(QByteArray amiboardbuf, QByteArray dsdtbuf, UINT32 DSDTOffsetOld, UINT32 DSDTSizeOld, QByteArray & out, UINT32 &relocPadding);

private:
    UINT64 insn_detail(csh ud, cs_mode mode, cs_insn *ins);
    UINT8 Disass(UINT8 *X86_CODE64, INT32 CodeSize, INT32 size);
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
