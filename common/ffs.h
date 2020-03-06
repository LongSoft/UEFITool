/* ffs.h

Copyright (c) 2016, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHWARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*/

#ifndef FFS_H
#define FFS_H

#include <vector>

#include "basetypes.h"
#include "ubytearray.h"
#include "ustring.h"

// Make sure we use right packing rules
#pragma pack(push,1)

extern UString guidToUString(const EFI_GUID& guid, bool convertToString = true);
extern bool ustringToGuid(const UString& str, EFI_GUID& guid);
extern UString fileTypeToUString(const UINT8 type);
extern UString sectionTypeToUString(const UINT8 type);
extern UString bpdtEntryTypeToUString(const UINT16 type);
extern UString cpdExtensionTypeToUstring(const UINT32 type);
//*****************************************************************************
// EFI Capsule
//*****************************************************************************
// Capsule header
typedef struct EFI_CAPSULE_HEADER_ {
    EFI_GUID  CapsuleGuid;
    UINT32    HeaderSize;
    UINT32    Flags;
    UINT32    CapsuleImageSize;
} EFI_CAPSULE_HEADER;

// Capsule flags
#define EFI_CAPSULE_HEADER_FLAG_SETUP                   0x00000001
#define EFI_CAPSULE_HEADER_FLAG_PERSIST_ACROSS_RESET    0x00010000
#define EFI_CAPSULE_HEADER_FLAG_POPULATE_SYSTEM_TABLE   0x00020000

// Standard FMP capsule GUID
const UByteArray EFI_FMP_CAPSULE_GUID // 6DCBD5ED-E82D-4C44-BDA1-7194199AD92A
("\xED\xD5\xCB\x6D\x2D\xE8\x44\x4C\xBD\xA1\x71\x94\x19\x9A\xD9\x2A", 16);

// Standard EFI capsule GUID
const UByteArray EFI_CAPSULE_GUID
("\xBD\x86\x66\x3B\x76\x0D\x30\x40\xB7\x0E\xB5\x51\x9E\x2F\xC5\xA0", 16);

// Intel capsule GUID
const UByteArray INTEL_CAPSULE_GUID
("\xB9\x82\x91\x53\xB5\xAB\x91\x43\xB6\x9A\xE3\xA9\x43\xF7\x2F\xCC", 16);

// Lenovo capsule GUID
const UByteArray LENOVO_CAPSULE_GUID
("\xD3\xAF\x0B\xE2\x14\x99\x4F\x4F\x95\x37\x31\x29\xE0\x90\xEB\x3C", 16);

// Another Lenovo capsule GUID
const UByteArray LENOVO2_CAPSULE_GUID
("\x76\xFE\xB5\x25\x43\x82\x5C\x4A\xA9\xBD\x7E\xE3\x24\x61\x98\xB5", 16);

// Toshiba EFI Capsule header
typedef struct TOSHIBA_CAPSULE_HEADER_ {
    EFI_GUID  CapsuleGuid;
    UINT32    HeaderSize;
    UINT32    FullSize;
    UINT32    Flags;
} TOSHIBA_CAPSULE_HEADER;

// Toshiba capsule GUID
const UByteArray TOSHIBA_CAPSULE_GUID
("\x62\x70\xE0\x3B\x51\x1D\xD2\x45\x83\x2B\xF0\x93\x25\x7E\xD4\x61", 16);

// AMI Aptio extended capsule header
typedef struct APTIO_CAPSULE_HEADER_ {
    EFI_CAPSULE_HEADER    CapsuleHeader;
    UINT16                RomImageOffset;  // offset in bytes from the beginning of the capsule header to the start of the capsule volume
    UINT16                RomLayoutOffset; // offset to the table of the module descriptors in the capsule's volume that are included in the signature calculation
    //FW_CERTIFICATE      FWCert;
    //ROM_AREA            RomAreaMap[1];
} APTIO_CAPSULE_HEADER;

// AMI Aptio signed extended capsule GUID
const UByteArray APTIO_SIGNED_CAPSULE_GUID
("\x8B\xA6\x3C\x4A\x23\x77\xFB\x48\x80\x3D\x57\x8C\xC1\xFE\xC4\x4D", 16);

// AMI Aptio unsigned extended capsule GUID
const UByteArray APTIO_UNSIGNED_CAPSULE_GUID
("\x90\xBB\xEE\x14\x0A\x89\xDB\x43\xAE\xD1\x5D\x3C\x45\x88\xA4\x18", 16);

//*****************************************************************************
// EFI Firmware Volume
//*****************************************************************************
// Firmware block map entry
// FvBlockMap ends with an entry {0x00000000, 0x00000000}
typedef struct EFI_FV_BLOCK_MAP_ENTRY_ {
    UINT32  NumBlocks;
    UINT32  Length;
} EFI_FV_BLOCK_MAP_ENTRY;

// Volume header
typedef struct EFI_FIRMWARE_VOLUME_HEADER_ {
    UINT8                  ZeroVector[16];
    EFI_GUID               FileSystemGuid;
    UINT64                 FvLength;
    UINT32                 Signature;
    UINT32                 Attributes;
    UINT16                 HeaderLength;
    UINT16                 Checksum;
    UINT16                 ExtHeaderOffset;  //Reserved in Revision 1
    UINT8                  Reserved;
    UINT8                  Revision;
    //EFI_FV_BLOCK_MAP_ENTRY FvBlockMap[2];
} EFI_FIRMWARE_VOLUME_HEADER;

// Standard file system GUIDs
const UByteArray EFI_FIRMWARE_FILE_SYSTEM_GUID // 7A9354D9-0468-444A-81CE-0BF617D890DF
("\xD9\x54\x93\x7A\x68\x04\x4A\x44\x81\xCE\x0B\xF6\x17\xD8\x90\xDF", 16);

const UByteArray EFI_FIRMWARE_FILE_SYSTEM2_GUID // 8C8CE578-8A3D-4F1C-9935-896185C32DD3
("\x78\xE5\x8C\x8C\x3D\x8A\x1C\x4F\x99\x35\x89\x61\x85\xC3\x2D\xD3", 16);

const UByteArray EFI_FIRMWARE_FILE_SYSTEM3_GUID // 5473C07A-3DCB-4DCA-BD6F-1E9689E7349A
("\x7A\xC0\x73\x54\xCB\x3D\xCA\x4D\xBD\x6F\x1E\x96\x89\xE7\x34\x9A", 16);

// Vendor-specific file system GUIDs
const UByteArray EFI_APPLE_IMMUTABLE_FV_GUID // 04ADEEAD-61FF-4D31-B6BA-64F8BF901F5A
("\xAD\xEE\xAD\x04\xFF\x61\x31\x4D\xB6\xBA\x64\xF8\xBF\x90\x1F\x5A", 16);

const UByteArray EFI_APPLE_AUTHENTICATION_FV_GUID // BD001B8C-6A71-487B-A14F-0C2A2DCF7A5D
("\x8C\x1B\x00\xBD\x71\x6A\x7B\x48\xA1\x4F\x0C\x2A\x2D\xCF\x7A\x5D", 16);

const UByteArray EFI_APPLE_MICROCODE_VOLUME_GUID // 153D2197-29BD-44DC-AC59-887F70E41A6B
("\x97\x21\x3D\x15\xBD\x29\xDC\x44\xAC\x59\x88\x7F\x70\xE4\x1A\x6B", 16);
#define EFI_APPLE_MICROCODE_VOLUME_HEADER_SIZE 0x100

const UByteArray EFI_INTEL_FILE_SYSTEM_GUID // AD3FFFFF-D28B-44C4-9F13-9EA98A97F9F0
("\xFF\xFF\x3F\xAD\x8B\xD2\xC4\x44\x9F\x13\x9E\xA9\x8A\x97\xF9\xF0", 16);

const UByteArray EFI_INTEL_FILE_SYSTEM2_GUID // D6A1CD70-4B33-4994-A6EA-375F2CCC5437
("\x70\xCD\xA1\xD6\x33\x4B\x94\x49\xA6\xEA\x37\x5F\x2C\xCC\x54\x37", 16);

const UByteArray EFI_SONY_FILE_SYSTEM_GUID // 4F494156-AED6-4D64-A537-B8A5557BCEEC
("\x56\x41\x49\x4F\xD6\xAE\x64\x4D\xA5\x37\xB8\xA5\x55\x7B\xCE\xEC", 16);

// Vector of volume GUIDs with FFSv2-compatible files
extern const std::vector<UByteArray> FFSv2Volumes;

// Vector of volume GUIDs with FFSv3-compatible files
extern const std::vector<UByteArray> FFSv3Volumes;

// Firmware volume signature
#define EFI_FV_SIGNATURE 0x4856465F // _FVH
#define EFI_FV_SIGNATURE_OFFSET 0x28

// Firmware volume attributes
// Revision 1
#define EFI_FVB_READ_DISABLED_CAP   0x00000001
#define EFI_FVB_READ_ENABLED_CAP    0x00000002
#define EFI_FVB_READ_STATUS         0x00000004
#define EFI_FVB_WRITE_DISABLED_CAP  0x00000008
#define EFI_FVB_WRITE_ENABLED_CAP   0x00000010
#define EFI_FVB_WRITE_STATUS        0x00000020
#define EFI_FVB_LOCK_CAP            0x00000040
#define EFI_FVB_LOCK_STATUS         0x00000080
#define EFI_FVB_STICKY_WRITE        0x00000200
#define EFI_FVB_MEMORY_MAPPED       0x00000400
#define EFI_FVB_ERASE_POLARITY      0x00000800
#define EFI_FVB_ALIGNMENT_CAP       0x00008000
#define EFI_FVB_ALIGNMENT_2         0x00010000
#define EFI_FVB_ALIGNMENT_4         0x00020000
#define EFI_FVB_ALIGNMENT_8         0x00040000
#define EFI_FVB_ALIGNMENT_16        0x00080000
#define EFI_FVB_ALIGNMENT_32        0x00100000
#define EFI_FVB_ALIGNMENT_64        0x00200000
#define EFI_FVB_ALIGNMENT_128       0x00400000
#define EFI_FVB_ALIGNMENT_256       0x00800000
#define EFI_FVB_ALIGNMENT_512       0x01000000
#define EFI_FVB_ALIGNMENT_1K        0x02000000
#define EFI_FVB_ALIGNMENT_2K        0x04000000
#define EFI_FVB_ALIGNMENT_4K        0x08000000
#define EFI_FVB_ALIGNMENT_8K        0x10000000
#define EFI_FVB_ALIGNMENT_16K       0x20000000
#define EFI_FVB_ALIGNMENT_32K       0x40000000
#define EFI_FVB_ALIGNMENT_64K       0x80000000
// Revision 2
#define EFI_FVB2_READ_DISABLED_CAP  0x00000001
#define EFI_FVB2_READ_ENABLED_CAP   0x00000002
#define EFI_FVB2_READ_STATUS        0x00000004
#define EFI_FVB2_WRITE_DISABLED_CAP 0x00000008
#define EFI_FVB2_WRITE_ENABLED_CAP  0x00000010
#define EFI_FVB2_WRITE_STATUS       0x00000020
#define EFI_FVB2_LOCK_CAP           0x00000040
#define EFI_FVB2_LOCK_STATUS        0x00000080
#define EFI_FVB2_STICKY_WRITE       0x00000200
#define EFI_FVB2_MEMORY_MAPPED      0x00000400
#define EFI_FVB2_ERASE_POLARITY     0x00000800
#define EFI_FVB2_READ_LOCK_CAP      0x00001000
#define EFI_FVB2_READ_LOCK_STATUS   0x00002000
#define EFI_FVB2_WRITE_LOCK_CAP     0x00004000
#define EFI_FVB2_WRITE_LOCK_STATUS  0x00008000
#define EFI_FVB2_ALIGNMENT          0x001F0000
#define EFI_FVB2_ALIGNMENT_1        0x00000000
#define EFI_FVB2_ALIGNMENT_2        0x00010000
#define EFI_FVB2_ALIGNMENT_4        0x00020000
#define EFI_FVB2_ALIGNMENT_8        0x00030000
#define EFI_FVB2_ALIGNMENT_16       0x00040000
#define EFI_FVB2_ALIGNMENT_32       0x00050000
#define EFI_FVB2_ALIGNMENT_64       0x00060000
#define EFI_FVB2_ALIGNMENT_128      0x00070000
#define EFI_FVB2_ALIGNMENT_256      0x00080000
#define EFI_FVB2_ALIGNMENT_512      0x00090000
#define EFI_FVB2_ALIGNMENT_1K       0x000A0000
#define EFI_FVB2_ALIGNMENT_2K       0x000B0000
#define EFI_FVB2_ALIGNMENT_4K       0x000C0000
#define EFI_FVB2_ALIGNMENT_8K       0x000D0000
#define EFI_FVB2_ALIGNMENT_16K      0x000E0000
#define EFI_FVB2_ALIGNMENT_32K      0x000F0000
#define EFI_FVB2_ALIGNMENT_64K      0x00100000
#define EFI_FVB2_ALIGNMENT_128K     0x00110000
#define EFI_FVB2_ALIGNMENT_256K     0x00120000
#define EFI_FVB2_ALIGNMENT_512K     0x00130000
#define EFI_FVB2_ALIGNMENT_1M       0x00140000
#define EFI_FVB2_ALIGNMENT_2M       0x00150000
#define EFI_FVB2_ALIGNMENT_4M       0x00160000
#define EFI_FVB2_ALIGNMENT_8M       0x00170000
#define EFI_FVB2_ALIGNMENT_16M      0x00180000
#define EFI_FVB2_ALIGNMENT_32M      0x00190000
#define EFI_FVB2_ALIGNMENT_64M      0x001A0000
#define EFI_FVB2_ALIGNMENT_128M     0x001B0000
#define EFI_FVB2_ALIGNMENT_256M     0x001C0000
#define EFI_FVB2_ALIGNMENT_512M     0x001D0000
#define EFI_FVB2_ALIGNMENT_1G       0x001E0000
#define EFI_FVB2_ALIGNMENT_2G       0x001F0000
#define EFI_FVB2_WEAK_ALIGNMENT     0x80000000

// Extended firmware volume header
typedef struct EFI_FIRMWARE_VOLUME_EXT_HEADER_ {
    EFI_GUID          FvName;
    UINT32            ExtHeaderSize;
} EFI_FIRMWARE_VOLUME_EXT_HEADER;

// Extended header entry
// The extended header entries follow each other and are
// terminated by ExtHeaderType EFI_FV_EXT_TYPE_END
#define EFI_FV_EXT_TYPE_END        0x0000
typedef struct EFI_FIRMWARE_VOLUME_EXT_ENTRY_ {
    UINT16  ExtEntrySize;
    UINT16  ExtEntryType;
} EFI_FIRMWARE_VOLUME_EXT_ENTRY;

// GUID that maps OEM file types to GUIDs
#define EFI_FV_EXT_TYPE_OEM_TYPE   0x0001
typedef struct EFI_FIRMWARE_VOLUME_EXT_HEADER_OEM_TYPE_ {
    EFI_FIRMWARE_VOLUME_EXT_ENTRY    Header;
    UINT32                           TypeMask;
    //EFI_GUID                       Types[];
} EFI_FIRMWARE_VOLUME_EXT_HEADER_OEM_TYPE;

#define EFI_FV_EXT_TYPE_GUID_TYPE  0x0002
typedef struct EFI_FIRMWARE_VOLUME_EXT_ENTRY_GUID_TYPE_ {
    EFI_FIRMWARE_VOLUME_EXT_ENTRY Header;
    EFI_GUID FormatType;
    //UINT8 Data[];
} EFI_FIRMWARE_VOLUME_EXT_ENTRY_GUID_TYPE;

//*****************************************************************************
// EFI FFS File
//*****************************************************************************
// Integrity check
typedef union {
    struct {
        UINT8	Header;
        UINT8	File;
    } Checksum;
    UINT16 TailReference;   // Revision 1
    UINT16 Checksum16;      // Revision 2
} EFI_FFS_INTEGRITY_CHECK;
// File header
typedef struct EFI_FFS_FILE_HEADER_ {
    EFI_GUID                Name;
    EFI_FFS_INTEGRITY_CHECK IntegrityCheck;
    UINT8                   Type;
    UINT8                   Attributes;
    UINT8                   Size[3];
    UINT8                   State;
} EFI_FFS_FILE_HEADER;

// Large file header
typedef struct EFI_FFS_FILE_HEADER2_ {
EFI_GUID                Name;
EFI_FFS_INTEGRITY_CHECK IntegrityCheck;
UINT8                   Type;
UINT8                   Attributes;
UINT8                   Size[3]; // Set to 0xFFFFFF
UINT8                   State;
UINT64                  ExtendedSize;
} EFI_FFS_FILE_HEADER2;

// Standard data checksum, used if FFS_ATTRIB_CHECKSUM is clear
#define FFS_FIXED_CHECKSUM   0x5A
#define FFS_FIXED_CHECKSUM2  0xAA

// File types
#define EFI_FV_FILETYPE_ALL                     0x00
#define EFI_FV_FILETYPE_RAW                     0x01
#define EFI_FV_FILETYPE_FREEFORM                0x02
#define EFI_FV_FILETYPE_SECURITY_CORE           0x03
#define EFI_FV_FILETYPE_PEI_CORE                0x04
#define EFI_FV_FILETYPE_DXE_CORE                0x05
#define EFI_FV_FILETYPE_PEIM                    0x06
#define EFI_FV_FILETYPE_DRIVER                  0x07
#define EFI_FV_FILETYPE_COMBINED_PEIM_DRIVER    0x08
#define EFI_FV_FILETYPE_APPLICATION             0x09
#define EFI_FV_FILETYPE_MM                      0x0A
#define EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE   0x0B
#define EFI_FV_FILETYPE_COMBINED_MM_DXE         0x0C
#define EFI_FV_FILETYPE_MM_CORE                 0x0D
#define EFI_FV_FILETYPE_MM_STANDALONE           0x0E
#define EFI_FV_FILETYPE_MM_CORE_STANDALONE      0x0F
#define EFI_FV_FILETYPE_OEM_MIN                 0xC0
#define EFI_FV_FILETYPE_OEM_MAX                 0xDF
#define EFI_FV_FILETYPE_DEBUG_MIN               0xE0
#define EFI_FV_FILETYPE_DEBUG_MAX               0xEF
#define EFI_FV_FILETYPE_PAD                     0xF0
#define EFI_FV_FILETYPE_FFS_MIN                 0xF0
#define EFI_FV_FILETYPE_FFS_MAX                 0xFF

// File attributes
#define FFS_ATTRIB_TAIL_PRESENT       0x01 // Valid only for revision 1 volumes
#define FFS_ATTRIB_RECOVERY           0x02 // Valid only for revision 1 volumes
#define FFS_ATTRIB_LARGE_FILE         0x01 // Valid only for FFSv3 volumes
#define FFS_ATTRIB_DATA_ALIGNMENT2    0x02 // Volaid only for revision 2 volumes, added in UEFI PI 1.6
#define FFS_ATTRIB_FIXED              0x04
#define FFS_ATTRIB_DATA_ALIGNMENT     0x38
#define FFS_ATTRIB_CHECKSUM           0x40

// FFS alignment table
extern const UINT8 ffsAlignmentTable[];

// Extended FFS alignment table, added in UEFI PI 1.6
extern const UINT8 ffsAlignment2Table[];


// File states
#define EFI_FILE_HEADER_CONSTRUCTION    0x01
#define EFI_FILE_HEADER_VALID           0x02
#define EFI_FILE_DATA_VALID             0x04
#define EFI_FILE_MARKED_FOR_UPDATE      0x08
#define EFI_FILE_DELETED                0x10
#define EFI_FILE_HEADER_INVALID         0x20
#define EFI_FILE_ERASE_POLARITY         0x80 // Defined as "all other bits must be set to ERASE_POLARITY" in UEFI PI

// PEI apriori file
const UByteArray EFI_PEI_APRIORI_FILE_GUID
("\x0A\xCC\x45\x1B\x6A\x15\x8A\x42\xAF\x62\x49\x86\x4D\xA0\xE6\xE6", 16);

// DXE apriori file
const UByteArray EFI_DXE_APRIORI_FILE_GUID
("\xE7\x0E\x51\xFC\xDC\xFF\xD4\x11\xBD\x41\x00\x80\xC7\x3C\x88\x81", 16);

// Volume top file
const UByteArray EFI_FFS_VOLUME_TOP_FILE_GUID
("\x2E\x06\xA0\x1B\x79\xC7\x82\x45\x85\x66\x33\x6A\xE8\xF7\x8F\x09", 16);

// Pad file GUID
const UByteArray EFI_FFS_PAD_FILE_GUID
("\x85\x65\x53\xE4\x09\x79\x60\x4A\xB5\xC6\xEC\xDE\xA6\xEB\xFB\x54", 16);

// AMI DXE core file
const UByteArray AMI_CORE_DXE_GUID // 5AE3F37E-4EAE-41AE-8240-35465B5E81EB
("\x7E\xF3\xE3\x5A\xAE\x4E\xAE\x41\x82\x40\x35\x46\x5B\x5E\x81\xEB", 16);

// EDK2 DXE code file
const UByteArray EFI_DXE_CORE_GUID // D6A2CB7F-6A18-4E2F-B43B-9920A733700A
("\x7F\xCB\xA2\xD6\x18\x6A\x2F\x4E\xB4\x3B\x99\x20\xA7\x33\x70\x0A", 16);

// TXT ACM
const UByteArray EFI_TXT_ACM_GUID // 2D27C618-7DCD-41F5-BB10-21166BE7E143
("\x18\xC6\x27\x2D\xCD\x7D\xF5\x41\xBB\x10\x21\x16\x6B\xE7\xE1\x43", 16);

// FFS size conversion routines
extern VOID uint32ToUint24(UINT32 size, UINT8* ffsSize);
extern UINT32 uint24ToUint32(const UINT8* ffsSize);

//*****************************************************************************
// EFI FFS File Section
//*****************************************************************************
// Common section header
typedef struct EFI_COMMON_SECTION_HEADER_ {
    UINT8    Size[3];
    UINT8    Type;
} EFI_COMMON_SECTION_HEADER;

// Large file common section header
typedef struct EFI_COMMON_SECTION_HEADER2_ {
    UINT8    Size[3];    // Must be 0xFFFFFF for this header to be used
    UINT8    Type;
    UINT32   ExtendedSize;
} EFI_COMMON_SECTION_HEADER2;

// Apple common section header
typedef struct EFI_COMMON_SECTION_HEADER_APPLE {
    UINT8    Size[3];    
    UINT8    Type;
    UINT32   Reserved;   // Must be 0x7FFF for this header to be used
} EFI_COMMON_SECTION_HEADER_APPLE;

// Section2 usage indicator
#define EFI_SECTION2_IS_USED 0xFFFFFF

// Apple section usage indicator
#define EFI_SECTION_APPLE_USED 0x7FFF

// File section types
#define EFI_SECTION_ALL 0x00 // Impossible attribute for file in the FS

// Encapsulation section types
#define EFI_SECTION_COMPRESSION     0x01
#define EFI_SECTION_GUID_DEFINED    0x02
#define EFI_SECTION_DISPOSABLE      0x03

// Leaf section types
#define EFI_SECTION_PE32                    0x10
#define EFI_SECTION_PIC                     0x11
#define EFI_SECTION_TE                      0x12
#define EFI_SECTION_DXE_DEPEX               0x13
#define EFI_SECTION_VERSION                 0x14
#define EFI_SECTION_USER_INTERFACE          0x15
#define EFI_SECTION_COMPATIBILITY16         0x16
#define EFI_SECTION_FIRMWARE_VOLUME_IMAGE   0x17
#define EFI_SECTION_FREEFORM_SUBTYPE_GUID   0x18
#define EFI_SECTION_RAW                     0x19
#define EFI_SECTION_PEI_DEPEX               0x1B
#define EFI_SECTION_MM_DEPEX                0x1C
#define PHOENIX_SECTION_POSTCODE            0xF0 // Specific to Phoenix SCT images
#define INSYDE_SECTION_POSTCODE             0x20 // Specific to Insyde H2O images

// Compression section
typedef struct EFI_COMPRESSION_SECTION_ {
    UINT32   UncompressedLength;
    UINT8    CompressionType;
} EFI_COMPRESSION_SECTION;

typedef struct EFI_COMPRESSION_SECTION_APPLE_ {
    UINT32   UncompressedLength;
    UINT32   CompressionType;
} EFI_COMPRESSION_SECTION_APPLE;

// Compression types
#define EFI_NOT_COMPRESSED                 0x00
#define EFI_STANDARD_COMPRESSION           0x01
#define EFI_CUSTOMIZED_COMPRESSION         0x02
#define EFI_CUSTOMIZED_COMPRESSION_LZMAF86 0x86

//GUID defined section
typedef struct EFI_GUID_DEFINED_SECTION_ {
    EFI_GUID SectionDefinitionGuid;
    UINT16   DataOffset;
    UINT16   Attributes;
} EFI_GUID_DEFINED_SECTION;

typedef struct EFI_GUID_DEFINED_SECTION_APPLE_ {
    EFI_GUID SectionDefinitionGuid;
    UINT16   DataOffset;
    UINT16   Attributes;
    UINT32   Reserved;
} EFI_GUID_DEFINED_SECTION_APPLE;

// Attributes for GUID defined section
#define EFI_GUIDED_SECTION_PROCESSING_REQUIRED  0x01
#define EFI_GUIDED_SECTION_AUTH_STATUS_VALID    0x02

// GUIDs of GUID-defined sections
const UByteArray EFI_GUIDED_SECTION_CRC32 // FC1BCDB0-7D31-49AA-936A-A4600D9DD083
("\xB0\xCD\x1B\xFC\x31\x7D\xAA\x49\x93\x6A\xA4\x60\x0D\x9D\xD0\x83", 16);

const UByteArray EFI_GUIDED_SECTION_TIANO // A31280AD-481E-41B6-95E8-127F4C984779
("\xAD\x80\x12\xA3\x1E\x48\xB6\x41\x95\xE8\x12\x7F\x4C\x98\x47\x79", 16);

const UByteArray EFI_GUIDED_SECTION_LZMA // EE4E5898-3914-4259-9D6E-DC7BD79403CF
("\x98\x58\x4E\xEE\x14\x39\x59\x42\x9D\x6E\xDC\x7B\xD7\x94\x03\xCF", 16);

const UByteArray EFI_GUIDED_SECTION_LZMAF86 // D42AE6BD-1352-4BFB-909A-CA72A6EAE889
("\xBD\xE6\x2A\xD4\x52\x13\xFB\x4B\x90\x9A\xCA\x72\xA6\xEA\xE8\x89", 16);

const UByteArray EFI_GUIDED_SECTION_GZIP // 1D301FE9-BE79-4353-91C2-D23BC959AE0C
("\xE9\x1F\x30\x1D\x79\xBE\x53\x43\x91\xC2\xD2\x3B\xC9\x59\xAE\x0C", 16);

const UByteArray EFI_FIRMWARE_CONTENTS_SIGNED_GUID // 0F9D89E8-9259-4F76-A5AF-0C89E34023DF
("\xE8\x89\x9D\x0F\x59\x92\x76\x4F\xA5\xAF\x0C\x89\xE3\x40\x23\xDF", 16);

//#define WIN_CERT_TYPE_PKCS_SIGNED_DATA 0x0002
#define WIN_CERT_TYPE_EFI_GUID         0x0EF1

typedef struct WIN_CERTIFICATE_ {
    UINT32  Length;
    UINT16  Revision;
    UINT16  CertificateType;
    //UINT8 CertData[];
} WIN_CERTIFICATE;

typedef struct WIN_CERTIFICATE_UEFI_GUID_ {
    WIN_CERTIFICATE   Header;     // Standard WIN_CERTIFICATE
    EFI_GUID          CertType;   // Determines format of CertData
    // UINT8          CertData[]; // Certificate data follows
} WIN_CERTIFICATE_UEFI_GUID;

// WIN_CERTIFICATE_UEFI_GUID.CertType
const UByteArray EFI_CERT_TYPE_RSA2048_SHA256_GUID // A7717414-C616-4977-9420-844712A735BF
("\x14\x74\x71\xA7\x16\xC6\x77\x49\x94\x20\x84\x47\x12\xA7\x35\xBF");

// WIN_CERTIFICATE_UEFI_GUID.CertData
typedef struct EFI_CERT_BLOCK_RSA2048_SHA256_ {
    EFI_GUID  HashType;
    UINT8     PublicKey[256];
    UINT8     Signature[256];
} EFI_CERT_BLOCK_RSA2048_SHA256;

const UByteArray EFI_HASH_ALGORITHM_SHA256_GUID // 51aa59de-fdf2-4ea3-bc63-875fb7842ee9
("\xde\x59\xAA\x51\xF2\xFD\xA3\x4E\xBC\x63\x87\x5F\xB7\x84\x2E\xE9");

// Version section
typedef struct EFI_VERSION_SECTION_ {
    UINT16   BuildNumber;
} EFI_VERSION_SECTION;

// Freeform subtype GUID section
typedef struct EFI_FREEFORM_SUBTYPE_GUID_SECTION_ {
    EFI_GUID SubTypeGuid;
} EFI_FREEFORM_SUBTYPE_GUID_SECTION;

// Phoenix SCT and Insyde postcode section
typedef struct POSTCODE_SECTION_ {
    UINT32   Postcode;
} POSTCODE_SECTION;

//*****************************************************************************
// EFI Dependency Expression
//*****************************************************************************
#define EFI_DEP_OPCODE_SIZE   1

///
/// If present, this must be the first and only opcode,
/// EFI_DEP_BEFORE is only used by DXE drivers
///
#define EFI_DEP_BEFORE        0x00

///
/// If present, this must be the first and only opcode,
/// EFI_DEP_AFTER is only used by DXE drivers
///
#define EFI_DEP_AFTER         0x01

#define EFI_DEP_PUSH          0x02
#define EFI_DEP_AND           0x03
#define EFI_DEP_OR            0x04
#define EFI_DEP_NOT           0x05
#define EFI_DEP_TRUE          0x06
#define EFI_DEP_FALSE         0x07
#define EFI_DEP_END           0x08


///
/// If present, this must be the first opcode,
/// EFI_DEP_SOR is only used by DXE drivers
///
#define EFI_DEP_SOR           0x09

//*****************************************************************************
// X86 Reset Vector Data
//*****************************************************************************
typedef struct X86_RESET_VECTOR_DATA_ {
    UINT8  ApEntryVector[8];   // Base: 0xffffffd0
    UINT8  Reserved0[8];
    UINT32 PeiCoreEntryPoint;  // Base: 0xffffffe0
    UINT8  Reserved1[12];
    UINT8  ResetVector[8];     // Base: 0xfffffff0
    UINT32 ApStartupSegment;   // Base: 0xfffffff8
    UINT32 BootFvBaseAddress;  // Base: 0xfffffffc
} X86_RESET_VECTOR_DATA;

#define X86_RESET_VECTOR_DATA_UNPOPULATED 0x12345678

//*****************************************************************************
// IFWI
//*****************************************************************************

// BPDT
#define BPDT_GREEN_SIGNATURE  0x000055AA
#define BPDT_YELLOW_SIGNATURE 0x00AA55AA

typedef struct BPDT_HEADER_ {
    UINT32 Signature;
    UINT16 NumEntries;
    UINT8  HeaderVersion;
    UINT8  RedundancyFlag; // Reserved zero in version 1
    UINT32 Checksum;
    UINT32 IfwiVersion;
    UINT16 FitcMajor;
    UINT16 FitcMinor;
    UINT16 FitcHotfix;
    UINT16 FitcBuild;
} BPDT_HEADER;

#define BPDT_HEADER_VERSION_1 1
#define BPDT_HEADER_VERSION_2 2

typedef struct BPDT_ENTRY_ {
    UINT32 Type : 16;
    UINT32 SplitSubPartitionFirstPart : 1;
    UINT32 SplitSubPartitionSecondPart : 1;
    UINT32 CodeSubPartition : 1;
    UINT32 UmaCachable : 1;
    UINT32 Reserved: 12;
    UINT32 Offset;
    UINT32 Size;
} BPDT_ENTRY;

#define BPDT_ENTRY_TYPE_OEM_SMIP         0
#define BPDT_ENTRY_TYPE_OEM_RBE          1
#define BPDT_ENTRY_TYPE_CSE_BUP          2
#define BPDT_ENTRY_TYPE_UCODE            3
#define BPDT_ENTRY_TYPE_IBB              4
#define BPDT_ENTRY_TYPE_SBPDT            5
#define BPDT_ENTRY_TYPE_OBB              6
#define BPDT_ENTRY_TYPE_CSE_MAIN         7
#define BPDT_ENTRY_TYPE_ISH              8
#define BPDT_ENTRY_TYPE_CSE_IDLM         9
#define BPDT_ENTRY_TYPE_IFP_OVERRIDE     10
#define BPDT_ENTRY_TYPE_DEBUG_TOKENS     11
#define BPDT_ENTRY_TYPE_USF_PHY_CONFIG   12
#define BPDT_ENTRY_TYPE_USB_GPP_LUN_ID   13
#define BPDT_ENTRY_TYPE_PMC              14
#define BPDT_ENTRY_TYPE_IUNIT            15
#define BPDT_ENTRY_TYPE_NVM_CONFIG       16
#define BPDT_ENTRY_TYPE_UEP              17
#define BPDT_ENTRY_TYPE_WLAN_UCODE       18
#define BPDT_ENTRY_TYPE_LOCL_SPRITES     19
#define BPDT_ENTRY_TYPE_OEM_KEY_MANIFEST 20
#define BPDT_ENTRY_TYPE_DEFAULTS         21
#define BPDT_ENTRY_TYPE_PAVP             22
#define BPDT_ENTRY_TYPE_TCSS_FW_IOM      23
#define BPDT_ENTRY_TYPE_TCSS_FW_PHY      24
#define BPDT_ENTRY_TYPE_TBT              25
#define BPDT_LAST_KNOWN_ENTRY_TYPE       BPDT_ENTRY_TYPE_TBT

// CPD
#define CPD_SIGNATURE 0x44504324 //$CPD

typedef struct CPD_REV1_HEADER_ {
    UINT32 Signature;
    UINT32 NumEntries;
    UINT8  HeaderVersion; // 1
    UINT8  EntryVersion;
    UINT8  HeaderLength;
    UINT8  HeaderChecksum;
    UINT8  ShortName[4];
} CPD_REV1_HEADER;

typedef struct CPD_REV2_HEADER_ {
    UINT32 Signature;
    UINT32 NumEntries;
    UINT8  HeaderVersion; // 2
    UINT8  EntryVersion;
    UINT8  HeaderLength;
    UINT8  Reserved;
    UINT8  ShortName[4];
    UINT32 Checksum;
} CPD_REV2_HEADER;

typedef struct CPD_ENTRY_ {
    UINT8  EntryName[12];
    struct {
        UINT32 Offset : 25;
        UINT32 HuffmanCompressed : 1;
        UINT32 Reserved : 6;
    } Offset;
    UINT32 Length;
    UINT32 Reserved;
} CPD_ENTRY;

typedef struct CPD_MANIFEST_HEADER_ {
    UINT32 HeaderType;
    UINT32 HeaderLength;
    UINT32 HeaderVersion;
    UINT32 Flags;
    UINT32 Vendor;
    UINT32 Date;
    UINT32 Size;
    UINT32 HeaderId;
    UINT32 Reserved1;
    UINT16 VersionMajor;
    UINT16 VersionMinor;
    UINT16 VersionBugfix;
    UINT16 VersionBuild;
    UINT32 SecurityVersion;
    UINT8  Reserved2[8];
    UINT8  Reserved3[64];
    UINT32 ModulusSize;
    UINT32 ExponentSize;
    //manifest_rsa_key_t public_key;
    //manifest_signature_t signature;
} CPD_MANIFEST_HEADER;

typedef struct CPD_EXTENTION_HEADER_ {
    UINT32 Type;
    UINT32 Length;
} CPD_EXTENTION_HEADER;

#define CPD_EXT_TYPE_SYSTEM_INFO             0
#define CPD_EXT_TYPE_INIT_SCRIPT             1
#define CPD_EXT_TYPE_FEATURE_PERMISSIONS     2
#define CPD_EXT_TYPE_PARTITION_INFO          3
#define CPD_EXT_TYPE_SHARED_LIB_ATTRIBUTES   4
#define CPD_EXT_TYPE_PROCESS_ATTRIBUTES      5
#define CPD_EXT_TYPE_THREAD_ATTRIBUTES       6
#define CPD_EXT_TYPE_DEVICE_TYPE             7
#define CPD_EXT_TYPE_MMIO_RANGE              8
#define CPD_EXT_TYPE_SPEC_FILE_PRODUCER      9
#define CPD_EXT_TYPE_MODULE_ATTRIBUTES       10
#define CPD_EXT_TYPE_LOCKED_RANGES           11
#define CPD_EXT_TYPE_CLIENT_SYSTEM_INFO      12
#define CPD_EXT_TYPE_USER_INFO               13
#define CPD_EXT_TYPE_KEY_MANIFEST            14
#define CPD_EXT_TYPE_SIGNED_PACKAGE_INFO     15
#define CPD_EXT_TYPE_ANTI_CLONING_SKU_ID     16
#define CPD_EXT_TYPE_CAVS                    17
#define CPD_EXT_TYPE_IMR_INFO                18
#define CPD_EXT_TYPE_BOOT_POLICY             19
#define CPD_EXT_TYPE_RCIP_INFO               20
#define CPD_EXT_TYPE_SECURE_TOKEN            21
#define CPD_EXT_TYPE_IFWI_PARTITION_MANIFEST 22
#define CPD_EXT_TYPE_FD_HASH                 23
#define CPD_EXT_TYPE_IOM_METADATA            24
#define CPD_EXT_TYPE_MGP_METADATA            25
#define CPD_EXT_TYPE_TBT_METADATA            26
#define CPD_LAST_KNOWN_EXT_TYPE              CPD_EXT_TYPE_TBT_METADATA

typedef struct CPD_EXT_SIGNED_PACKAGE_INFO_MODULE_ {
    UINT8  Name[12];
    UINT8  Type;
    UINT8  HashAlgorithm;
    UINT16 HashSize;
    UINT32 MetadataSize;
    UINT8  MetadataHash[32];
} CPD_EXT_SIGNED_PACKAGE_INFO_MODULE;

typedef struct CPD_EXT_SIGNED_PACKAGE_INFO_ {
    UINT32 ExtensionType;
    UINT32 ExtensionLength;
    UINT8  PackageName[4];
    UINT32 Vcn;
    UINT8  UsageBitmap[16];
    UINT32 Svn;
    UINT8  Reserved[16];
    // EXT_SIGNED_PACKAGE_INFO_MODULE Modules[];
} CPD_EXT_SIGNED_PACKAGE_INFO;

typedef struct CPD_EXT_MODULE_ATTRIBUTES_ {
    UINT32 ExtensionType;
    UINT32 ExtensionLength;
    UINT8  CompressionType;
    UINT8  Reserved[3];
    UINT32 UncompressedSize;
    UINT32 CompressedSize;
    UINT32 GlobalModuleId;
    UINT8  ImageHash[32];
} CPD_EXT_MODULE_ATTRIBUTES;

#define CPD_EXT_MODULE_COMPRESSION_TYPE_UNCOMPRESSED 0
#define CPD_EXT_MODULE_COMPRESSION_TYPE_HUFFMAN 1
#define CPD_EXT_MODULE_COMPRESSION_TYPE_LZMA 2

typedef struct CPD_EXT_IFWI_PARTITION_MANIFEST_ {
    UINT32 ExtensionType;
    UINT32 ExtensionLength;
    UINT8  PartitionName[4];
    UINT32 CompletePartitionLength;
    UINT16 PartitionVersionMinor;
    UINT16 PartitionVersionMajor;
    UINT32 DataFormatVersion;
    UINT32 InstanceId;
    UINT32 SupportMultipleInstances : 1;
    UINT32 SupportApiVersionBasedUpdate : 1;
    UINT32 ActionOnUpdate : 2;
    UINT32 ObeyFullUpdateRules : 1;
    UINT32 IfrEnableOnly : 1;
    UINT32 AllowCrossPointUpdate : 1;
    UINT32 AllowCrossHotfixUpdate : 1;
    UINT32 PartialUpdateOnly : 1;
    UINT32 ReservedFlags : 23;
    UINT32 HashAlgorithm : 8;
    UINT32 HashSize : 24;
    UINT8  CompletePartitionHash[32];
    UINT8  Reserved[20];
} CPD_EXT_IFWI_PARTITION_MANIFEST;

// Restore previous packing rules
#pragma pack(pop)

#endif // FFS_H
