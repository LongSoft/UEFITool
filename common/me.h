/* me.h

Copyright (c) 2015, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHWARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*/

#ifndef ME_H
#define ME_H

#include "basetypes.h"

// Make sure we use right packing rules
#pragma pack(push, 1)

typedef struct ME_VERSION_ {
    UINT32 Signature;
    UINT32 Reserved;
    UINT16 Major;
    UINT16 Minor;
    UINT16 Bugfix;
    UINT16 Build;
} ME_VERSION;

#define ME_VERSION_SIGNATURE 0x4E414D24 //$MAN
#define ME_VERSION_SIGNATURE2  0x324E4D24 //$MN2

// FPT
#define ME_ROM_BYPASS_VECTOR_SIZE 0x10
#define FPT_HEADER_SIGNATURE 0x54504624 //$FPT

// Header version 1.0 or 2.0, default
typedef struct FPT_HEADER_ {
    UINT32 Signature;
    UINT32 NumEntries;
    UINT8  HeaderVersion;  // 0x10 or 0x20
    UINT8  EntryVersion;
    UINT8  HeaderLength;
    UINT8  HeaderChecksum; // One bit for Redundant
    UINT16 FlashCycleLife; // Maybe also TicksToAdd
    UINT16 FlashCycleLimit;// Maybe also TokensToAdd
    UINT32 UmaSize;        // Maybe also Flags
    UINT32 Flags;          // Maybe also FlashLayout
    UINT16 FitcMajor;
    UINT16 FitcMinor;
    UINT16 FitcHotfix;
    UINT16 FitcBuild;
} FPT_HEADER;

// Header version 2.1, special case
#define FPT_HEADER_VERSION_21 0x21
typedef struct FPT_HEADER_21_ {
    UINT32 Signature;
    UINT32 NumEntries;
    UINT8  HeaderVersion;  // 0x21
    UINT8  EntryVersion;
    UINT8  HeaderLength;
    UINT8  Flags;          // One bit for Redundant
    UINT16 TicksToAdd;
    UINT16 TokensToAdd;
    UINT32 SPSFlags;
    UINT32 HeaderCrc32;    // Header + Entries sums to 0
    UINT16 FitcMajor;
    UINT16 FitcMinor;
    UINT16 FitcHotfix;
    UINT16 FitcBuild;
} FPT_HEADER_21;

typedef struct FPT_HEADER_ENTRY_{
    CHAR8  Name[4];
    CHAR8  Owner[4];
    UINT32 Offset;
    UINT32 Size;
    UINT32 Reserved[3];
    UINT8  Type             : 7;
    UINT8  CopyToDramCache  : 1;
    UINT8  Reserved1        : 7;
    UINT8  BuiltWithLength1 : 1;
    UINT8  BuiltWithLength2 : 1;
    UINT8  Reserved2        : 7;
    UINT8  EntryValid;
} FPT_HEADER_ENTRY;

// IFWI
typedef struct IFWI_HEADER_ENTRY_ {
    UINT32 Offset;
    UINT32 Size;
} IFWI_HEADER_ENTRY;

// IFWI 1.6 (ME), 2.0 (BIOS)
typedef struct IFWI_16_LAYOUT_HEADER_ {
    UINT8             RomBypassVector[16];
    IFWI_HEADER_ENTRY DataPartition;
    IFWI_HEADER_ENTRY BootPartition[5];
    UINT64            Checksum;
} IFWI_16_LAYOUT_HEADER;

// IFWI 1.7 (ME)
typedef struct IFWI_17_LAYOUT_HEADER_ {
    UINT8  RomBypassVector[16];
    UINT16 HeaderSize;
    UINT8  Flags;
    UINT8  Reserved;
    UINT32 Checksum;
    IFWI_HEADER_ENTRY DataPartition;
    IFWI_HEADER_ENTRY BootPartition[5];
    IFWI_HEADER_ENTRY TempPage;
} IFWI_17_LAYOUT_HEADER;

#define ME_MANIFEST_HEADER_ID 0x324E4D24 //$MN2

// Restore previous packing rules
#pragma pack(pop)

#endif // ME_H
