/* descriptor.h

Copyright (c) 2014, Nikolaj Schlej. All rights reserved.
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
#pragma pack(push,1)

// Flash descriptor header
typedef struct {
    UINT8 FfVector[16];            // Must be 16 0xFFs
    UINT32 Signature;              // 0x0FF0A55A
} FLASH_DESCRIPTOR_HEADER;

// Flash descriptor signature
#define FLASH_DESCRIPTOR_SIGNATURE 0x0FF0A55A

// Descriptor region size
#define FLASH_DESCRIPTOR_SIZE      0x1000

// Descriptor map
// Base fields are storing bits [11:4] of actual base addresses, all other bits are 0
typedef struct {
    UINT8  ComponentBase;           // 0x03 on most machines
    UINT8  NumberOfFlashChips;      // Zero-based number of flash chips installed on board
    UINT8  RegionBase;              // 0x04 on most machines
    UINT8  NumberOfRegions;         // Zero-based number of flash regions (descriptor is always included)
    UINT8  MasterBase;              // 0x06 on most machines
    UINT8  NumberOfMasters;         // Zero-based number of flash masters
    UINT8  PchStrapsBase;           // 0x10 on most machines
    UINT8  NumberOfPchStraps;       // One-based number of UINT32s to read as PCH Straps, min=0, max=255 (1 Kb)
    UINT8  ProcStrapsBase;          // 0x20 on most machines
    UINT8  NumberOfProcStraps;      // Number of PROC straps to be read, can be 0 or 1
    UINT8  IccTableBase;            // 0x21 on most machines
    UINT8  NumberOfIccTableEntries; // 0x00 on most machines
    UINT8  DmiTableBase;            // 0x25 on most machines
    UINT8  NumberOfDmiTableEntries; // 0x00 on most machines
    UINT16 ReservedZero;            // Still unknown, zeros in all descriptors I have seen
} FLASH_DESCRIPTOR_MAP;


// Component section
// Flash parameters DWORD structure 
typedef struct {
    UINT8 FirstChipDensity            : 3;
    UINT8 SecondChipDensity           : 3;
    UINT8 ReservedZero0               : 2;  // Still unknown, zeros in all descriptors I have seen
    UINT8 ReservedZero1               : 8;  // Still unknown, zeros in all descriptors I have seen
    UINT8 ReservedZero2               : 4;  // Still unknown, zeros in all descriptors I have seen
    UINT8 FastReadEnabled             : 1;
    UINT8 FastReadFreqency            : 3;
    UINT8 FlashReadStatusFrequency    : 3;
    UINT8 FlashWriteFrequency         : 3;
    UINT8 DualOutputFastReadSupported : 1;
    UINT8 ReservedZero3               : 1;  // Still unknown, zero in all descriptors I have seen
} FLASH_PARAMETERS;

// Flash densities
#define FLASH_DENSITY_512KB   0x00
#define FLASH_DENSITY_1MB     0x01
#define FLASH_DENSITY_2MB     0x02
#define FLASH_DENSITY_4MB     0x03
#define FLASH_DENSITY_8MB     0x04
#define FLASH_DENSITY_16MB    0x05

// Flash frequencies
#define FLASH_FREQUENCY_20MHZ  0x00
#define FLASH_FREQUENCY_33MHZ  0x01
#define FLASH_FREQUENCY_50MHZ  0x04

// Component section structure
typedef struct {
    FLASH_PARAMETERS FlashParameters;
    UINT8            InvalidInstruction0;  // Instructions for SPI chip, that must not be executed, like FLASH ERASE
    UINT8            InvalidInstruction1;  //
    UINT8            InvalidInstruction2;  //
    UINT8            InvalidInstruction3;  //
    UINT16           PartitionBoundary;    // Upper 16 bit of partition boundary address. Default is 0x0000, which makes the boundary to be 0x00001000
    UINT16           ReservedZero;         // Still unknown, zero in all descriptors I have seen
} FLASH_DESCRIPTOR_COMPONENT_SECTION;

// Region section
// All base and limit register are storing upper part of actual UINT32 base and limit
// If limit is zero - region is not present
typedef struct {
    UINT16 ReservedZero;                  // Still unknown, zero in all descriptors I have seen
    UINT16 FlashBlockEraseSize;           // Size of block erased by single BLOCK ERASE command
    UINT16 BiosBase;
    UINT16 BiosLimit;
    UINT16 MeBase;
    UINT16 MeLimit;
    UINT16 GbeBase;
    UINT16 GbeLimit;
    UINT16 PdrBase;
    UINT16 PdrLimit;
} FLASH_DESCRIPTOR_REGION_SECTION;

// Flash block erase sizes
#define FLASH_BLOCK_ERASE_SIZE_4KB  0x0000
#define FLASH_BLOCK_ERASE_SIZE_8KB  0x0001
#define FLASH_BLOCK_ERASE_SIZE_64KB 0x000F

// Master section
typedef struct {
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

// Region access bits in master section
#define FLASH_DESCRIPTOR_REGION_ACCESS_DESC 0x01
#define FLASH_DESCRIPTOR_REGION_ACCESS_BIOS 0x02
#define FLASH_DESCRIPTOR_REGION_ACCESS_ME   0x04
#define FLASH_DESCRIPTOR_REGION_ACCESS_GBE  0x08
#define FLASH_DESCRIPTOR_REGION_ACCESS_PDR  0x10

//!TODO: Describe PCH and PROC straps sections, as well as ICC and DMI tables

// Base address of descriptor upper map
#define FLASH_DESCRIPTOR_UPPER_MAP_BASE 0x0EFC

// Descriptor upper map structure
typedef struct {
    UINT8 VsccTableBase; // Base address of VSCC Table for ME, bits [11:4]
    UINT8 VsccTableSize; // Counted in UINT32s
    UINT16 ReservedZero; // Still unknown, zero in all descriptors I have seen
} FLASH_DESCRIPTOR_UPPER_MAP;

// VSCC table entry structure
typedef struct {
    UINT8   VendorId;       // JEDEC VendorID byte
    UINT8   DeviceId0;      // JEDEC DeviceID first byte
    UINT8   DeviceId1;      // JEDEC DeviceID second byte
    UINT8   ReservedZero;   // Reserved, must be zero
    UINT32  VsccId;         // VSCC ID, normally it is 0x20052005 or 0x20152015
} VSCC_TABLE_ENTRY;

// Base address and size of OEM section
#define FLASH_DESCRIPTOR_OEM_SECTION_BASE 0x0F00
#define FLASH_DESCRIPTOR_OEM_SECTION_SIZE 0xFF

// Restore previous packing rules
#pragma pack(pop)

// Calculate address of data structure addressed by descriptor address format
// 8 bit base or limit
extern UINT8* calculateAddress8(UINT8* baseAddress, const UINT8 baseOrLimit);
// 16 bit base or limit
extern UINT8* calculateAddress16(UINT8* baseAddress, const UINT16 baseOrLimit);

// Calculate offset of region using it's base
extern UINT32 calculateRegionOffset(const UINT16 base);
// Calculate size of region using it's base and limit
extern UINT32 calculateRegionSize(const UINT16 base, const UINT16 limit);
#endif
