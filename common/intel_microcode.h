/* intel_microcode.h

Copyright (c) 2022, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHWARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*/

#ifndef INTEL_MICROCODE_H
#define INTEL_MICROCODE_H

#include "basetypes.h"
#include "ubytearray.h"

// Make sure we use right packing rules
#pragma pack(push, 1)

// This structure is described in Section 9.11.1 of the Intel Software Developer manual Volume 3A Part 1
typedef struct INTEL_MICROCODE_HEADER_ {
    UINT32 HeaderVersion;             // 0x00000001
    UINT32 UpdateRevision;
    UINT16 DateYear;                  // BCD
    UINT8  DateDay;                   // BCD
    UINT8  DateMonth;                 // BCD
    UINT32 ProcessorSignature;
    UINT32 Checksum;                  // Checksum of Update Data and Header. Used to verify the integrity of the update header and data.
                                      // Checksum is correct when the summation of all the DWORDs (including the extended Processor Signature Table)
                                      // that comprise the microcode update result in 00000000H.
    UINT32 LoaderRevision;            // 0x00000001
    UINT8  ProcessorFlags;
    UINT8  ProcessorFlagsReserved[3]; // Zeroes
    UINT32 DataSize;                  // Specifies the size of the encrypted data in bytes, and must be a multiple of DWORDs.
                                      // If this value is 00000000H, then the microcode update encrypted data is 2000 bytes (or 500 DWORDs).
                                      // Sane values are less than 0x1000000
    UINT32 TotalSize;                 // Specifies the total size of the microcode update in bytes.
                                      // It is the summation of the header size, the encrypted data size and the size of the optional extended signature table.
                                      // This value is always a multiple of 1024 according to the spec, but Intel already breached it several times.
                                      // Sane values are less than 0x1000000
    UINT8  Reserved[12];              // Zeroes
} INTEL_MICROCODE_HEADER;

#define INTEL_MICROCODE_REAL_DATA_SIZE_ON_ZERO 2000

typedef struct INTEL_MICROCODE_EXTENDED_HEADER_ {
    UINT32 EntryCount;
    UINT32 Checksum; // Checksum of extended processor signature table.
                     // Used to verify the integrity of the extended processor signature table.
                     // Checksum is correct when the summation of the DWORDs that comprise the extended processor signature table results in 00000000H.

    UINT8  Reserved[12];
    // INTEL_MICROCODE_EXTENDED_HEADER_ENTRY Entries[EntryCount];
} INTEL_MICROCODE_EXTENDED_HEADER;

typedef struct INTEL_MICROCODE_EXTENDED_HEADER_ENTRY_ {
    UINT32 ProcessorSignature;
    UINT32 ProcessorFlags;
    UINT32 Checksum;          // To calculate the Checksum, substitute the Primary Processor Signature entry and the Processor Flags entry with the corresponding Extended Patch entry.
                              // Delete the Extended Processor Signature Table entries.
                              // Checksum is correct when the summation of all DWORDs that comprise the created Extended Processor Patch results in 00000000H.
} INTEL_MICROCODE_EXTENDED_HEADER_ENTRY;

#define INTEL_MICROCODE_HEADER_VERSION_1 0x00000001

#pragma pack(pop)

#endif // INTEL_MICROCODE_H
