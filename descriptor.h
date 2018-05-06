/* descriptor.h

Copyright (c) 2015, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHWARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*/

#ifndef __DESCRIPTOR_H__
#define __DESCRIPTOR_H__

#include <QString>
#include "basetypes.h"

// Make sure we use right packing rules
#pragma pack(push, 1)

// Flash descriptor header
typedef struct _FLASH_DESCRIPTOR_HEADER {
    UINT8  FfVector[16];           // Must be 16 0xFFs
    UINT32 Signature;              // 0x0FF0A55A
} FLASH_DESCRIPTOR_HEADER;

// Flash descriptor signature
#define FLASH_DESCRIPTOR_SIGNATURE 0x0FF0A55A

// Descriptor region size
#define FLASH_DESCRIPTOR_SIZE      0x1000

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
typedef struct _FLASH_DESCRIPTOR_MAP {
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
    UINT32: 16;
    // FLMAP 3
    UINT32 DescriptorVersion;           // Reserved prior to Coffee Lake
} FLASH_DESCRIPTOR_MAP;

#define FLASH_DESCRIPTOR_MAX_BASE 0xE0

// Component section
// Flash parameters DWORD structure
typedef struct _FLASH_PARAMETERS {
    UINT8 FirstChipDensity : 4;
    UINT8 SecondChipDensity : 4;
    UINT8 : 8;
    UINT8 : 1;
    UINT8 ReadClockFrequency : 3; // Hardcoded value of 20 Mhz (000b) in v1 descriptors and 17 Mhz (110b) in v2 ones
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
typedef struct _FLASH_DESCRIPTOR_COMPONENT_SECTION {
    FLASH_PARAMETERS FlashParameters;
    UINT8            InvalidInstruction0;  // Instructions for SPI chip, that must not be executed, like FLASH ERASE
    UINT8            InvalidInstruction1;  //
    UINT8            InvalidInstruction2;  //
    UINT8            InvalidInstruction3;  //
} FLASH_DESCRIPTOR_COMPONENT_SECTION;

// Component section structure
typedef struct _FLASH_DESCRIPTOR_COMPONENT_SECTION_V2 {
    FLASH_PARAMETERS FlashParameters;
    UINT8            InvalidInstruction0;  // Instructions for SPI chip, that must not be executed, like FLASH ERASE
    UINT8            InvalidInstruction1;  //
    UINT8            InvalidInstruction2;  //
    UINT8            InvalidInstruction3;  //
    UINT8            InvalidInstruction4;  //
    UINT8            InvalidInstruction5;  //
    UINT8            InvalidInstruction6;  //
    UINT8            InvalidInstruction7;  //
} FLASH_DESCRIPTOR_COMPONENT_SECTION_V2;

// Region section
// All base and limit register are storing upper part of actual UINT32 base and limit
// If limit is zero - region is not present
typedef struct _FLASH_DESCRIPTOR_REGION_SECTION {
    UINT16 DescriptorBase;             // Descriptor
    UINT16 DescriptorLimit;            //
    UINT16 BiosBase;                   // BIOS
    UINT16 BiosLimit;                  //
    UINT16 MeBase;                     // ME
    UINT16 MeLimit;                    //
    UINT16 GbeBase;                    // GbE
    UINT16 GbeLimit;                   //
    UINT16 PdrBase;                    // PDR
    UINT16 PdrLimit;                   //
    UINT16 Region5Base;                // Reserved region
    UINT16 Region5Limit;               //
    UINT16 Region6Base;                // Reserved region
    UINT16 Region6Limit;               //
    UINT16 Region7Base;                // Reserved region
    UINT16 Region7Limit;               //
    UINT16 EcBase;                     // EC
    UINT16 EcLimit;                    //
} FLASH_DESCRIPTOR_REGION_SECTION;

// Master section
typedef struct _FLASH_DESCRIPTOR_MASTER_SECTION {
    UINT16 BiosId;
    UINT8  BiosRead;
    UINT8  BiosWrite;
    UINT16 MeId;
    UINT8  MeRead;
    UINT8  MeWrite;
    UINT16 GbeId;
    UINT8  GbeRead;
    UINT8  GbeWrite;
} FLASH_DESCRIPTOR_MASTER_SECTION;

// Master section v2 (Skylake+)
typedef struct _FLASH_DESCRIPTOR_MASTER_SECTION_V2 {
    UINT32 : 8;
    UINT32 BiosRead : 12;
    UINT32 BiosWrite : 12;
    UINT32 : 8;
    UINT32 MeRead : 12;
    UINT32 MeWrite : 12;
    UINT32 : 8;
    UINT32 GbeRead : 12;
    UINT32 GbeWrite : 12;
    UINT32 :32;
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
typedef struct _FLASH_DESCRIPTOR_UPPER_MAP {
    UINT8 VsccTableBase; // Base address of VSCC Table for ME, bits [11:4]
    UINT8 VsccTableSize; // Counted in UINT32s
    UINT16 ReservedZero; // Still unknown, zero in all descriptors I have seen
} FLASH_DESCRIPTOR_UPPER_MAP;

// VSCC table entry structure
typedef struct _VSCC_TABLE_ENTRY {
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
#endif
