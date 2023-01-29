/* intel_fit.h

Copyright (c) 2015, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHWARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*/

#ifndef INTEL_FIT_H
#define INTEL_FIT_H

#include "basetypes.h"
#include "ubytearray.h"

// Make sure we use right packing rules
#pragma pack(push, 1)

// Memory address of a pointer to FIT, 40h back from the end of flash chip
#define INTEL_FIT_POINTER_OFFSET 0x40

// Entry types
// https://cdrdv2-public.intel.com/599500/Firmware-Interface-Table-BIOS-Specification-r1p4.pdf
#define INTEL_FIT_TYPE_HEADER                     0x00
#define INTEL_FIT_TYPE_MICROCODE                  0x01
#define INTEL_FIT_TYPE_STARTUP_AC_MODULE          0x02
#define INTEL_FIT_TYPE_DIAG_AC_MODULE             0x03
#define INTEL_FIT_TYPE_PLATFORM_BOOT_POLICY       0x04
//#define INTEL_FIT_TYPE_INTEL_RESERVED_05        0x05
#define INTEL_FIT_TYPE_FIT_RESET_STATE            0x06
#define INTEL_FIT_TYPE_BIOS_STARTUP_MODULE        0x07
#define INTEL_FIT_TYPE_TPM_POLICY                 0x08
#define INTEL_FIT_TYPE_BIOS_POLICY                0x09
#define INTEL_FIT_TYPE_TXT_POLICY                 0x0A
#define INTEL_FIT_TYPE_BOOT_GUARD_KEY_MANIFEST    0x0B
#define INTEL_FIT_TYPE_BOOT_GUARD_BOOT_POLICY     0x0C
//#define INTEL_FIT_TYPE_INTEL_RESERVED_0D        0x0D
//#define INTEL_FIT_TYPE_INTEL_RESERVED_0E        0x0E
//#define INTEL_FIT_TYPE_INTEL_RESERVED_0F        0x0F
#define INTEL_FIT_TYPE_CSE_SECURE_BOOT            0x10
//#define INTEL_FIT_TYPE_INTEL_RESERVED_11        0x11
//...
//#define INTEL_FIT_TYPE_INTEL_RESERVED_19        0x19
#define INTEL_FIT_TYPE_VAB_PROVISIONING_TABLE     0x1A
#define INTEL_FIT_TYPE_VAB_KEY_MANIFEST           0x1B
#define INTEL_FIT_TYPE_VAB_IMAGE_MANIFEST         0x1C
#define INTEL_FIT_TYPE_VAB_IMAGE_HASH_DESCRIPTORS 0x1D
//#define INTEL_FIT_TYPE_INTEL_RESERVED_1E        0x1E
//...
//#define INTEL_FIT_TYPE_INTEL_RESERVED_2B        0x2B
#define INTEL_FIT_TYPE_SACM_DEBUG_RECORD          0x2C
#define INTEL_FIT_TYPE_ACM_FEATURE_POLICY         0x2D
#define INTEL_FIT_TYPE_SCRTM_ERROR_RECORD         0x2E
#define INTEL_FIT_TYPE_JMP_DEBUG_POLICY           0x2F
#define INTEL_FIT_TYPE_OEM_RESERVED_30            0x30
//...
#define INTEL_FIT_TYPE_OEM_RESERVED_70            0x70
//#define INTEL_FIT_TYPE_INTEL_RESERVED_71        0x71
//...
//#define INTEL_FIT_TYPE_INTEL_RESERVED_7E        0x7E
#define INTEL_FIT_TYPE_EMPTY                      0x7F

typedef struct INTEL_FIT_ENTRY_ {
    UINT64 Address;          // Base address of the component, must be 16-byte aligned
    UINT32 Size : 24;        // Size of the component, in multiple of 16 bytes
    UINT32 Reserved : 8;     // Reserved, must be set to zero
    UINT16 Version;          // BCD, minor in lower byte, major in upper byte
    UINT8  Type : 7;         // FIT entries must be aranged in ascending order of Type
    UINT8  ChecksumValid : 1;// Checksum must be ignored if this bit is not set
    UINT8  Checksum;         // checksum8 of all the bytes in the component and this field must add to zero
} INTEL_FIT_ENTRY;

//
// FIT Header (0x00)
//
// Can be exactly one entry of this type, the first one.
// If ChecksumValid bit is set, the whole FIT table must checksum8 to zero.
// Version must be 0x0100
#define INTEL_FIT_SIGNATURE 0x2020205F5449465FULL // '_FIT_   '

//
// Microcode (0x01)
//
// At least one entry is required, more is optional
// Each entry must point to a valid base address
// Microcode slots can be empty (first 4 bytes at the base address are FF FF FF FF)
// Base address must be aligned to 16 bytes
// The item at the base address must not be compressed/encoded/encrypted
// ChecksumValid bit must be 0
// Size is not used, should be set to 0

//
// Startup Authenticated Code Module (0x02)
//
// Optional, required for AC boot and BootGuard
// Address must point to a valid base address
// Points to the first byte of ACM header
// One MTRR base/limit pair is used to map Startup ACM, so
// MTRR_Base must be a multiple of MTRR_Size, the can be found by the following formula
// MTTR_Size = 2^(ceil(log2(Startup_ACM_Size))), i.e. the next integer that's a full power of 2 to Startup_ACM_Size
// The whole area of [MTRR_Base; MTRR_Base + MTRR_Size) is named
// Authenticated Code Execution Area (ACEA) and should not contain any code or data that is not the Startup ACM itself
// ChecksumValid bit must be 0
// Size is not used, should be set to 0
// Version must be 0x0100
#define INTEL_ACM_HARDCODED_RSA_EXPONENT 0x10001

//
// TPM Boot Policy (0x08)
//
// Optional, used for legacy TXT FIT boot, if used, can be only one
// Address entry is INTEL_FIT_POLICY_PTR.IndexIo if Version is 0,
// or INTEL_FIT_INDEX_IO_ADDRESS.FlatMemoryAddress if Version is 1
// Bit 0 at the pointed address holds the TPM policy, 0 - TPM disabled, 1 - TPM enabled
// ChecksumValid bit must be 0
// Size is not used, should be set to 0
typedef struct INTEL_FIT_INDEX_IO_ADDRESS_ {
    UINT16 IndexRegisterAddress;
    UINT16 DataRegisterAddress;
    UINT8  AccessWidthInBytes; // 1 => 1-byte accesses, 2 => 2-byte
    UINT8  BitPosition; // Bit number, 15 => Bit15
    UINT16 Index;
} INTEL_FIT_INDEX_IO_ADDRESS;

typedef union INTEL_FIT_POLICY_PTR_ {
    UINT64 FlatMemoryAddress;
    INTEL_FIT_INDEX_IO_ADDRESS IndexIo;
} INTEL_FIT_POLICY_PTR;

#define INTEL_FIT_POLICY_VERSION_INDEX_IO 0
#define INTEL_FIT_POLICY_VERSION_FLAT_MEMORY_ADDRESS 1

#define INTEL_FIT_POLICY_DISABLED 1
#define INTEL_FIT_POLICY_ENABLED 1

//
// CSE SecureBoot (0x10)
//
// Optional, can be multiple, order is not important
// If present, BootGuardKeyManifest and BootGuardBootPolicy should also be present
// Reserved field further determines the subtype of this entry
// ChecksumValid bit must be 0
// Version must be 0x0100

#define INTEL_FIT_CSE_SECURE_BOOT_RESERVED 0
#define INTEL_FIT_CSE_SECURE_BOOT_KEY_HASH 1
#define INTEL_FIT_CSE_SECURE_BOOT_CSE_MEASUREMENT_HASH 2
#define INTEL_FIT_CSE_SECURE_BOOT_BOOT_POLICY 3
#define INTEL_FIT_CSE_SECURE_BOOT_OTHER_BOOT_POLICY 4
#define INTEL_FIT_CSE_SECURE_BOOT_OEM_SMIP 5
#define INTEL_FIT_CSE_SECURE_BOOT_MRC_TRAINING_DATA 6
#define INTEL_FIT_CSE_SECURE_BOOT_IBBL_HASH 7
#define INTEL_FIT_CSE_SECURE_BOOT_IBB_HASH 8
#define INTEL_FIT_CSE_SECURE_BOOT_OEM_ID 9
#define INTEL_FIT_CSE_SECURE_BOOT_OEM_SKU_ID 10
#define INTEL_FIT_CSE_SECURE_BOOT_BOOT_DEVICE_INDICATOR 11 // 1 => SPI, 2 => eMMC, 3 => UFS, rest => reserved
#define INTEL_FIT_CSE_SECURE_BOOT_FIT_PATCH_MANIFEST 12
#define INTEL_FIT_CSE_SECURE_BOOT_AC_MODULE_MANIFEST 13

#pragma pack(pop)

#endif // INTEL_FIT_H
