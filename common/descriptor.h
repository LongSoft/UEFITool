/* descriptor.h

Copyright (c) 2016, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHWARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*/

#ifndef DESCRIPTOR_H
#define DESCRIPTOR_H

#include "basetypes.h"
#include "ustring.h"
#include "ubytearray.h"

// Make sure we use right packing rules
#pragma pack(push,1)

// Flash descriptor header
typedef struct FLASH_DESCRIPTOR_HEADER_ {
    UINT8  ReservedVector[16];     // Reserved for ARM ResetVector, 0xFFs on x86/x86-64 machines
    UINT32 Signature;              // 0x0FF0A55A
} FLASH_DESCRIPTOR_HEADER;

// Flash descriptor signature
#define FLASH_DESCRIPTOR_SIGNATURE 0x0FF0A55A

// Descriptor region size
#define FLASH_DESCRIPTOR_SIZE      0x1000

// Maximum base value in descriptor map
#define FLASH_DESCRIPTOR_MAX_BASE  0xE0

// Descriptor version was reserved in older firmware
#define FLASH_DESCRIPTOR_VERSION_INVALID 0xFFFFFFFF
// The only known version found in Coffee Lake
#define FLASH_DESCRIPTOR_VERSION_MAJOR   1
#define FLASH_DESCRIPTOR_VERSION_MINOR   0

// Descriptor version present in Coffee Lake and newer
typedef struct _FLASH_DESCRIPTOR_VERSION {
    UINT32 Reserved : 14;
    UINT32 Minor : 7;
    UINT32 Major : 11;
} FLASH_DESCRIPTOR_VERSION;

// Descriptor map
// Base fields are storing bits [11:4] of actual base addresses, all other bits are 0
typedef struct FLASH_DESCRIPTOR_MAP_ {
    // FLMAP0
    UINT32 ComponentBase : 8;
    UINT32 NumberOfFlashChips : 2;      // Zero-based number of flash chips installed on board
    UINT32 : 6;
    UINT32 RegionBase : 8;
    UINT32 NumberOfRegions : 3;         // Reserved in v2 descriptor
    UINT32 : 5;
    // FLMAP 1
    UINT32 MasterBase : 8;
    UINT32 NumberOfMasters : 2;         // Zero-based number of flash masters
    UINT32 : 6;
    UINT32 PchStrapsBase : 8;
    UINT32 NumberOfPchStraps : 8;       // One-based number of UINT32s to read as PCH straps, min=0, max=255 (1 Kb)
    // FLMAP 2
    UINT32 ProcStrapsBase : 8;
    UINT32 NumberOfProcStraps : 8;      // One-based number of UINT32s to read as processor straps, min=0, max=255 (1 Kb)
    UINT32 : 16;
    // FLMAP 3
    UINT32 DescriptorVersion;           // Reserved prior to Coffee Lake
} FLASH_DESCRIPTOR_MAP;

// Component section
// Flash parameters DWORD structure
typedef struct FLASH_PARAMETERS_ {
    UINT8 FirstChipDensity : 4;
    UINT8 SecondChipDensity : 4;
    UINT8 : 8;
    UINT8 : 1;
    UINT8 ReadClockFrequency : 3; // Hardcoded value of 20 Mhz (000b) in v1 descriptors
    UINT8 FastReadEnabled : 1;
    UINT8 FastReadFrequency : 3;
    UINT8 FlashWriteFrequency : 3;
    UINT8 FlashReadStatusFrequency : 3;
    UINT8 DualOutputFastReadSupported : 1;
    UINT8 : 1;
} FLASH_PARAMETERS;

// Flash densities
#define FLASH_DENSITY_512KB   0x00
#define FLASH_DENSITY_1MB     0x01
#define FLASH_DENSITY_2MB     0x02
#define FLASH_DENSITY_4MB     0x03
#define FLASH_DENSITY_8MB     0x04
#define FLASH_DENSITY_16MB    0x05
#define FLASH_DENSITY_32MB    0x06
#define FLASH_DENSITY_64MB    0x07
#define FLASH_DENSITY_UNUSED  0x0F

// Flash frequencies
#define FLASH_FREQUENCY_20MHZ       0x00
#define FLASH_FREQUENCY_33MHZ       0x01
#define FLASH_FREQUENCY_48MHZ       0x02
#define FLASH_FREQUENCY_50MHZ_30MHZ 0x04
#define FLASH_FREQUENCY_17MHZ       0x06

// Component section structure
typedef struct FLASH_DESCRIPTOR_COMPONENT_SECTION_ {
    FLASH_PARAMETERS FlashParameters;
    UINT8            InvalidInstruction0;  // Instructions for SPI chip, that must not be executed, like FLASH ERASE
    UINT8            InvalidInstruction1;  //
    UINT8            InvalidInstruction2;  //
    UINT8            InvalidInstruction3;  //
    UINT16           PartitionBoundary;    // Upper 16 bit of partition boundary address. Default is 0x0000, which makes the boundary to be 0x00001000
    UINT16 : 16;
} FLASH_DESCRIPTOR_COMPONENT_SECTION;

// Region section
// All base and limit register are storing upper part of actual UINT32 base and limit
// If limit is zero - region is not present
typedef struct FLASH_DESCRIPTOR_REGION_SECTION_ {
    UINT16 DescriptorBase;             // Descriptor
    UINT16 DescriptorLimit;            //
    UINT16 BiosBase;                   // BIOS
    UINT16 BiosLimit;                  //
    UINT16 MeBase;                     // Management Engine
    UINT16 MeLimit;                    //
    UINT16 GbeBase;                    // Gigabit Ethernet
    UINT16 GbeLimit;                   //
    UINT16 PdrBase;                    // Platform Data
    UINT16 PdrLimit;                   //
    UINT16 DevExp1Base;                // Device Expansion 1
    UINT16 DevExp1Limit;               //
    UINT16 Bios2Base;                  // Secondary BIOS
    UINT16 Bios2Limit;                 //
    UINT16 MicrocodeBase;              // CPU microcode
    UINT16 MicrocodeLimit;             //
    UINT16 EcBase;                     // Embedded Controller
    UINT16 EcLimit;                    //
    UINT16 DevExp2Base;                // Device Expansion 2
    UINT16 DevExp2Limit;               //
    UINT16 IeBase;                     // Innovation Engine
    UINT16 IeLimit;                    //
    UINT16 Tgbe1Base;                  // 10 Gigabit Ethernet 1
    UINT16 Tgbe1Limit;                 //
    UINT16 Tgbe2Base;                  // 10 Gigabit Ethernet 2
    UINT16 Tgbe2Limit;                 //
    UINT16 Reserved1Base;              // Reserved 1
    UINT16 Reserved1Limit;             //
    UINT16 Reserved2Base;              // Reserved 2
    UINT16 Reserved2Limit;             //
    UINT16 PttBase;                    // Platform Trust Technology
    UINT16 PttLimit;                   //
} FLASH_DESCRIPTOR_REGION_SECTION;

// Master section
typedef struct FLASH_DESCRIPTOR_MASTER_SECTION_ {
    UINT16 BiosId;
    UINT8 BiosRead;
    UINT8 BiosWrite;
    UINT16 MeId;
    UINT8 MeRead;
    UINT8 MeWrite;
    UINT16 GbeId;
    UINT8 GbeRead;
    UINT8 GbeWrite;
} FLASH_DESCRIPTOR_MASTER_SECTION;

// Master section v2 (Skylake+)
typedef struct FLASH_DESCRIPTOR_MASTER_SECTION_V2_ {
    UINT32 : 8;
    UINT32 BiosRead : 12;
    UINT32 BiosWrite : 12;
    UINT32 : 8;
    UINT32 MeRead : 12;
    UINT32 MeWrite : 12;
    UINT32 : 8;
    UINT32 GbeRead : 12;
    UINT32 GbeWrite : 12;
    UINT32 : 32;
    UINT32 : 8;
    UINT32 EcRead : 12;
    UINT32 EcWrite : 12;
} FLASH_DESCRIPTOR_MASTER_SECTION_V2;

// Region access bits in master section
#define FLASH_DESCRIPTOR_REGION_ACCESS_DESC 0x01
#define FLASH_DESCRIPTOR_REGION_ACCESS_BIOS 0x02
#define FLASH_DESCRIPTOR_REGION_ACCESS_ME   0x04
#define FLASH_DESCRIPTOR_REGION_ACCESS_GBE  0x08
#define FLASH_DESCRIPTOR_REGION_ACCESS_PDR  0x10
#define FLASH_DESCRIPTOR_REGION_ACCESS_EC   0x20

// Base address of descriptor upper map
#define FLASH_DESCRIPTOR_UPPER_MAP_BASE 0x0EFC

// Descriptor upper map structure
typedef struct FLASH_DESCRIPTOR_UPPER_MAP_ {
    UINT8 VsccTableBase; // Base address of VSCC Table for ME, bits [11:4]
    UINT8 VsccTableSize; // Counted in UINT32s
    UINT16 ReservedZero; // Still unknown, zero in all descriptors I have seen
} FLASH_DESCRIPTOR_UPPER_MAP;

// VSCC table entry structure
typedef struct VSCC_TABLE_ENTRY_ {
    UINT8   VendorId;          // JEDEC VendorID byte
    UINT8   DeviceId0;         // JEDEC DeviceID first byte
    UINT8   DeviceId1;         // JEDEC DeviceID second byte
    UINT8   ReservedZero;      // Reserved, must be zero
    UINT32  VsccRegisterValue; // VSCC register value
} VSCC_TABLE_ENTRY;

// Base address and size of OEM section
#define FLASH_DESCRIPTOR_OEM_SECTION_BASE 0x0F00
#define FLASH_DESCRIPTOR_OEM_SECTION_SIZE 0x100

// Restore previous packing rules
#pragma pack(pop)

// Calculate address of data structure addressed by descriptor address format
// 8 bit base or limit
extern const UINT8* calculateAddress8(const UINT8* baseAddress, const UINT8 baseOrLimit);
// 16 bit base or limit
extern const UINT8* calculateAddress16(const UINT8* baseAddress, const UINT16 baseOrLimit);

// Calculate offset of region using it's base
extern UINT32 calculateRegionOffset(const UINT16 base);
// Calculate size of region using it's base and limit
extern UINT32 calculateRegionSize(const UINT16 base, const UINT16 limit);

// Return human-readable chip name for given JEDEC ID
extern UString jedecIdToUString(UINT8 vendorId, UINT8 deviceId0, UINT8 deviceId1);

#endif // DESCRIPTOR_H
