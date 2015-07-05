/* fit.h

Copyright (c) 2015, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHWARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*/

#ifndef __FIT_H__
#define __FIT_H__

#include <QByteArray>
#include "basetypes.h"

// Make sure we use right packing rules
#pragma pack(push,1)

// Memory address of a pointer to FIT, 40h back from the end of flash chip
#define FIT_POINTER_ADDRESS 0xFFFFFFC0

// FIT can reside in the last 1 MB of the flash chip
#define FIT_TABLE_LOWEST_ADDRESS 0xFF000000

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
#define FIT_TYPE_EMPTY             0xFF

#define FIT_HEADER_VERSION         0x0100
#define FIT_MICROCODE_VERSION      0x0100

const QByteArray FIT_SIGNATURE
("\x5F\x46\x49\x54\x5F\x20\x20\x20", 8); 

typedef struct _FIT_ENTRY {
    UINT64 Address;
    UINT64 ReservedSize;
    UINT16 Version;
    UINT8  ChecksumValid : 1;
    UINT8  Type : 7;
    UINT8  Checksum;
} FIT_ENTRY;

typedef struct _INTEL_MICROCODE_HEADER {
    UINT32 Version;
    UINT32 Revision;
    UINT32 Date;
    UINT32 CpuSignature;
    UINT32 Checksum;
    UINT32 LoaderRevision;
    UINT32 CpuFlags;
    UINT32 DataSize;
    UINT32 TotalSize;
    UINT8  Reserved[12];
} INTEL_MICROCODE_HEADER;

#pragma pack(pop)

#endif