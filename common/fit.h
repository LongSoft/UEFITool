/* fit.h

Copyright (c) 2015, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHWARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*/

#ifndef FIT_H
#define FIT_H

#include "basetypes.h"
#include "ubytearray.h"

// Make sure we use right packing rules
#pragma pack(push, 1)

// Memory address of a pointer to FIT, 40h back from the end of flash chip
#define FIT_POINTER_OFFSET 0x40

// Entry types 
#define FIT_TYPE_HEADER            0x00
#define FIT_TYPE_MICROCODE         0x01
#define FIT_TYPE_BIOS_AC_MODULE    0x02
#define FIT_TYPE_BIOS_INIT_MODULE  0x07
#define FIT_TYPE_TPM_POLICY        0x08
#define FIT_TYPE_BIOS_POLICY_DATA  0x09
#define FIT_TYPE_TXT_CONF_POLICY   0x0A
#define FIT_TYPE_AC_KEY_MANIFEST   0x0B
#define FIT_TYPE_AC_BOOT_POLICY    0x0C
#define FIT_TYPE_EMPTY             0x7F

#define FIT_HEADER_VERSION         0x0100
#define FIT_MICROCODE_VERSION      0x0100

const UByteArray FIT_SIGNATURE
("\x5F\x46\x49\x54\x5F\x20\x20\x20", 8); 

typedef struct FIT_ENTRY_ {
    UINT64 Address;
    UINT32 Size : 24;
    UINT32 : 8;
    UINT16 Version;
    UINT8  Type : 7;
    UINT8  CsFlag : 1;
    UINT8  Checksum;
} FIT_ENTRY;

typedef struct INTEL_MICROCODE_HEADER_ {
    UINT32 Version;
    UINT32 Revision;
    UINT16 DateYear;
    UINT8  DateDay;
    UINT8  DateMonth;
    UINT32 CpuSignature;
    UINT32 Checksum;
    UINT32 LoaderRevision;
    UINT32 CpuFlags;
    UINT32 DataSize;
    UINT32 TotalSize;
    UINT8  Reserved[12];
} INTEL_MICROCODE_HEADER;

typedef struct {
    UINT16 IndexRegisterAddress;
    UINT16 DataRegisterAddress;
    UINT8  AccessWidth;
    UINT8  BitPosition;
    UINT16 Index;
} FIT_ENTRY_VERSION_0_CONFIG_POLICY;

#define INTEL_MICROCODE_HEADER_VERSION 0x00000001
#define INTEL_MICROCODE_HEADER_RESERVED_BYTE 0x00
#define INTEL_MICROCODE_HEADER_SIZES_VALID(ptr) (((INTEL_MICROCODE_HEADER*)ptr)->TotalSize - ((INTEL_MICROCODE_HEADER*)ptr)->DataSize == sizeof(INTEL_MICROCODE_HEADER))

#pragma pack(pop)

#endif // FIT_H
