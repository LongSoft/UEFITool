/* ffs.h

Copyright (c) 2015, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHWARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*/

#ifndef __FFS_H__
#define __FFS_H__

#include <QByteArray>
#include <QString>
#include <QVector>
#include "basetypes.h"

// C++ functions
// GUID to QString routine
extern QString guidToQString(const EFI_GUID& guid);
// File type to QString routine
extern QString fileTypeToQString(const UINT8 type);
// Section type to QString routine
extern QString sectionTypeToQString(const UINT8 type);

// Make sure we use right packing rules
#pragma pack(push,1)

//*****************************************************************************
// EFI Capsule
//*****************************************************************************
// Standard EFI Capsule header
typedef struct _EFI_CAPSULE_HEADER {
    EFI_GUID  CapsuleGuid;
    UINT32    HeaderSize;
    UINT32    Flags;
    UINT32    CapsuleImageSize;
} EFI_CAPSULE_HEADER;

// Capsule flags
#define EFI_CAPSULE_HEADER_FLAG_SETUP                   0x00000001
#define EFI_CAPSULE_HEADER_FLAG_PERSIST_ACROSS_RESET    0x00010000
#define EFI_CAPSULE_HEADER_FLAG_POPULATE_SYSTEM_TABLE   0x00020000

// Standard EFI capsule GUID
const QByteArray EFI_CAPSULE_GUID
("\xBD\x86\x66\x3B\x76\x0D\x30\x40\xB7\x0E\xB5\x51\x9E\x2F\xC5\xA0", 16);

// Intel capsule GUID
const QByteArray INTEL_CAPSULE_GUID
("\xB9\x82\x91\x53\xB5\xAB\x91\x43\xB6\x9A\xE3\xA9\x43\xF7\x2F\xCC", 16);

// Toshiba EFI Capsule header
typedef struct _TOSHIBA_CAPSULE_HEADER {
    EFI_GUID  CapsuleGuid;
    UINT32    HeaderSize;
    UINT32    FullSize;
    UINT32    Flags;
} TOSHIBA_CAPSULE_HEADER;

// Toshiba capsule GUID
const QByteArray TOSHIBA_CAPSULE_GUID
("\x62\x70\xE0\x3B\x51\x1D\xD2\x45\x83\x2B\xF0\x93\x25\x7E\xD4\x61", 16);

// AMI Aptio extended capsule header
typedef struct _APTIO_CAPSULE_HEADER {
    EFI_CAPSULE_HEADER    CapsuleHeader;
    UINT16                RomImageOffset;   // offset in bytes from the beginning of the capsule header to the start of
    // the capsule volume
    //!TODO: Enable certificate and ROM layout reading
    //UINT16                RomLayoutOffset;    // offset to the table of the module descriptors in the capsule's volume
    // that are included in the signature calculation
    //FW_CERTIFICATE      FWCert;
    //ROM_AREA            RomAreaMap[1];
} APTIO_CAPSULE_HEADER;

// AMI Aptio signed extended capsule GUID
const QByteArray APTIO_SIGNED_CAPSULE_GUID
("\x8B\xA6\x3C\x4A\x23\x77\xFB\x48\x80\x3D\x57\x8C\xC1\xFE\xC4\x4D", 16);

// AMI Aptio unsigned extended capsule GUID
const QByteArray APTIO_UNSIGNED_CAPSULE_GUID
("\x90\xBB\xEE\x14\x0A\x89\xDB\x43\xAE\xD1\x5D\x3C\x45\x88\xA4\x18", 16);

//14EEBB90-890A-43DB-AED1-5D3C4588A418

//*****************************************************************************
// EFI Firmware Volume
//*****************************************************************************
// Firmware block map entry
// FvBlockMap ends with an entry {0x00000000, 0x00000000}
typedef struct _EFI_FV_BLOCK_MAP_ENTRY {
    UINT32  NumBlocks;
    UINT32  Length;
} EFI_FV_BLOCK_MAP_ENTRY;

// Volume header
typedef struct _EFI_FIRMWARE_VOLUME_HEADER {
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
const QByteArray EFI_FIRMWARE_FILE_SYSTEM_GUID
("\xD9\x54\x93\x7A\x68\x04\x4A\x44\x81\xCE\x0B\xF6\x17\xD8\x90\xDF", 16);
const QByteArray EFI_FIRMWARE_FILE_SYSTEM2_GUID
("\x78\xE5\x8C\x8C\x3D\x8A\x1C\x4F\x99\x35\x89\x61\x85\xC3\x2D\xD3", 16);
// Vendor-specific file system GUIDs
const QByteArray EFI_APPLE_BOOT_VOLUME_FILE_SYSTEM_GUID
("\xAD\xEE\xAD\x04\xFF\x61\x31\x4D\xB6\xBA\x64\xF8\xBF\x90\x1F\x5A", 16);
const QByteArray EFI_APPLE_BOOT_VOLUME_FILE_SYSTEM2_GUID
("\x8C\x1B\x00\xBD\x71\x6A\x7B\x48\xA1\x4F\x0C\x2A\x2D\xCF\x7A\x5D", 16);
const QByteArray EFI_INTEL_FILE_SYSTEM_GUID
("\xFF\xFF\x3F\xAD\x8B\xD2\xC4\x44\x9F\x13\x9E\xA9\x8A\x97\xF9\xF0", 16);
// AD3FFFFF-D28B-44C4-9F13-9EA98A97F9F0 // Intel 1
const QByteArray EFI_INTEL_FILE_SYSTEM2_GUID
("\x70\xCD\xA1\xD6\x33\x4B\x94\x49\xA6\xEA\x37\x5F\x2C\xCC\x54\x37", 16);
// D6A1CD70-4B33-4994-A6EA-375F2CCC5437 // Intel 2
const QByteArray EFI_SONY_FILE_SYSTEM_GUID
("\x56\x41\x49\x4F\xD6\xAE\x64\x4D\xA5\x37\xB8\xA5\x55\x7B\xCE\xEC", 16);
// 4F494156-AED6-4D64-A537-B8A5557BCEEC // Sony 1

// Vector of volume GUIDs with FFSv2-compatible files
extern const QVector<QByteArray> FFSv2Volumes;

const QByteArray EFI_FIRMWARE_FILE_SYSTEM3_GUID // 5473C07A-3DCB-4dca-BD6F-1E9689E7349A
("\x7A\xC0\x73\x54\xCB\x3D\xCA\x4D\xBD\x6F\x1E\x96\x89\xE7\x34\x9A", 16);

// Vector of volume GUIDs with FFSv3-compatible files
extern const QVector<QByteArray> FFSv3Volumes;

// Firmware volume signature
const QByteArray EFI_FV_SIGNATURE("_FVH", 4);
#define EFI_FV_SIGNATURE_OFFSET 0x28

// Firmware volume attributes
// Revision 1
#define EFI_FVB_READ_DISABLED_CAP  0x00000001
#define EFI_FVB_READ_ENABLED_CAP   0x00000002
#define EFI_FVB_READ_STATUS        0x00000004
#define EFI_FVB_WRITE_DISABLED_CAP 0x00000008
#define EFI_FVB_WRITE_ENABLED_CAP  0x00000010
#define EFI_FVB_WRITE_STATUS       0x00000020
#define EFI_FVB_LOCK_CAP           0x00000040
#define EFI_FVB_LOCK_STATUS        0x00000080
#define EFI_FVB_STICKY_WRITE       0x00000200
#define EFI_FVB_MEMORY_MAPPED      0x00000400
#define EFI_FVB_ERASE_POLARITY     0x00000800
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
typedef struct _EFI_FIRMWARE_VOLUME_EXT_HEADER {
    EFI_GUID          FvName;
    UINT32            ExtHeaderSize;
} EFI_FIRMWARE_VOLUME_EXT_HEADER;

// Extended header entry
// The extended header entries follow each other and are
// terminated by ExtHeaderType EFI_FV_EXT_TYPE_END
#define EFI_FV_EXT_TYPE_END        0x0000
typedef struct _EFI_FIRMWARE_VOLUME_EXT_ENTRY {
    UINT16  ExtEntrySize;
    UINT16  ExtEntryType;
} EFI_FIRMWARE_VOLUME_EXT_ENTRY;

// GUID that maps OEM file types to GUIDs
#define EFI_FV_EXT_TYPE_OEM_TYPE   0x0001
typedef struct _EFI_FIRMWARE_VOLUME_EXT_HEADER_OEM_TYPE {
    EFI_FIRMWARE_VOLUME_EXT_ENTRY    Header;
    UINT32                           TypeMask;
    //EFI_GUID                         Types[1];
} EFI_FIRMWARE_VOLUME_EXT_HEADER_OEM_TYPE;

#define EFI_FV_EXT_TYPE_GUID_TYPE  0x0002
typedef struct _EFI_FIRMWARE_VOLUME_EXT_ENTRY_GUID_TYPE {
    EFI_FIRMWARE_VOLUME_EXT_ENTRY Header;
    EFI_GUID FormatType;
    //UINT8 Data[];
} EFI_FIRMWARE_VOLUME_EXT_ENTRY_GUID_TYPE;

//!TODO: add proper NVRAM parsing
//const QByteArray EFI_FIRMWARE_VOLUME_NVRAM_SIGNATURE("$VSS", 4);

// Volume header 16bit checksum calculation routine
extern UINT16 calculateChecksum16(const UINT16* buffer, UINT32 bufferSize);

//*****************************************************************************
// EFI FFS File
//*****************************************************************************
// Integrity check
typedef union {
    struct {
        UINT8   Header;
        UINT8   File;
    } Checksum;
    UINT16 TailReference;   // Revision 1
    UINT16 Checksum16;      // Revision 2
} EFI_FFS_INTEGRITY_CHECK;
// File header
typedef struct _EFI_FFS_FILE_HEADER {
    EFI_GUID                Name;
    EFI_FFS_INTEGRITY_CHECK IntegrityCheck;
    UINT8                   Type;
    UINT8                   Attributes;
    UINT8                   Size[3];
    UINT8                   State;
} EFI_FFS_FILE_HEADER;

// Large file header
typedef struct _EFI_FFS_FILE_HEADER2 {
EFI_GUID                Name;
EFI_FFS_INTEGRITY_CHECK IntegrityCheck;
UINT8                   Type;
UINT8                   Attributes;
UINT8                   Size[3]; // Set to 0xFFFFFF
UINT8                   State;
UINT32                  ExtendedSize;
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
#define EFI_FV_FILETYPE_SMM                     0x0A
#define EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE   0x0B
#define EFI_FV_FILETYPE_COMBINED_SMM_DXE        0x0C
#define EFI_FV_FILETYPE_SMM_CORE                0x0D
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
#define FFS_ATTRIB_FIXED              0x04
#define FFS_ATTRIB_DATA_ALIGNMENT     0x38
#define FFS_ATTRIB_CHECKSUM           0x40

// FFS alignment table
extern const UINT8 ffsAlignmentTable[];

// File states
#define EFI_FILE_HEADER_CONSTRUCTION    0x01
#define EFI_FILE_HEADER_VALID           0x02
#define EFI_FILE_DATA_VALID             0x04
#define EFI_FILE_MARKED_FOR_UPDATE      0x08
#define EFI_FILE_DELETED                0x10
#define EFI_FILE_HEADER_INVALID         0x20

// PEI apriori file
const QByteArray EFI_PEI_APRIORI_FILE_GUID
("\x0A\xCC\x45\x1B\x6A\x15\x8A\x42\xAF\x62\x49\x86\x4D\xA0\xE6\xE6", 16);

// DXE apriori file
const QByteArray EFI_DXE_APRIORI_FILE_GUID
("\xE7\x0E\x51\xFC\xDC\xFF\xD4\x11\xBD\x41\x00\x80\xC7\x3C\x88\x81", 16);

// Volume top file
const QByteArray EFI_FFS_VOLUME_TOP_FILE_GUID
("\x2E\x06\xA0\x1B\x79\xC7\x82\x45\x85\x66\x33\x6A\xE8\xF7\x8F\x09", 16);

// Pad file GUID
const QByteArray EFI_FFS_PAD_FILE_GUID
("\x85\x65\x53\xE4\x09\x79\x60\x4A\xB5\xC6\xEC\xDE\xA6\xEB\xFB\x54", 16);

// FFS size conversion routines
extern VOID uint32ToUint24(UINT32 size, UINT8* ffsSize);
extern UINT32 uint24ToUint32(const UINT8* ffsSize);
// FFS file 8bit checksum calculation routine
extern UINT8 calculateChecksum8(const UINT8* buffer, UINT32 bufferSize);

//*****************************************************************************
// EFI FFS File Section
//*****************************************************************************
// Common section header
typedef struct _EFI_COMMON_SECTION_HEADER {
    UINT8    Size[3];
    UINT8    Type;
} EFI_COMMON_SECTION_HEADER;

// Large file common section header
typedef struct _EFI_COMMON_SECTION_HEADER2 {
    UINT8    Size[3];    //Must be 0xFFFFFF for this header to be used
    UINT8    Type;
    UINT32   ExtendedSize;
} EFI_COMMON_SECTION_HEADER2;

// Section2 usage indicator
#define EFI_SECTION2_IS_USED 0xFFFFFF

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
#define EFI_SECTION_SMM_DEPEX               0x1C
#define SCT_SECTION_POSTCODE                0xF0 // Specific to Phoenix SCT images
#define INSYDE_SECTION_POSTCODE             0x20 // Specific to Insyde images

// Compression section
typedef struct _EFI_COMPRESSION_SECTION {
    UINT8    Size[3];
    UINT8    Type;
    UINT32   UncompressedLength;
    UINT8    CompressionType;
} EFI_COMPRESSION_SECTION;

typedef struct _EFI_COMPRESSION_SECTION2 {
    UINT8    Size[3];
    UINT8    Type;
    UINT32   ExtendedSize;
    UINT32   UncompressedLength;
    UINT8    CompressionType;
} EFI_COMPRESSION_SECTION2;

// Compression types
#define EFI_NOT_COMPRESSED          0x00
#define EFI_STANDARD_COMPRESSION    0x01
#define EFI_CUSTOMIZED_COMPRESSION  0x02

//GUID defined section
typedef struct _EFI_GUID_DEFINED_SECTION {
    UINT8    Size[3];
    UINT8    Type;
    EFI_GUID SectionDefinitionGuid;
    UINT16   DataOffset;
    UINT16   Attributes;
} EFI_GUID_DEFINED_SECTION;

typedef struct _EFI_GUID_DEFINED_SECTION2 {
    UINT8    Size[3];
    UINT8    Type;
    UINT32   ExtendedSize;
    EFI_GUID SectionDefinitionGuid;
    UINT16   DataOffset;
    UINT16   Attributes;
} EFI_GUID_DEFINED_SECTION2;

// Attributes for GUID defined section
#define EFI_GUIDED_SECTION_PROCESSING_REQUIRED  0x01
#define EFI_GUIDED_SECTION_AUTH_STATUS_VALID    0x02

// GUIDs of GUID-defined sections
const QByteArray EFI_GUIDED_SECTION_CRC32 // FC1BCDB0-7D31-49AA-936A-A4600D9DD083
("\xB0\xCD\x1B\xFC\x31\x7D\xAA\x49\x93\x6A\xA4\x60\x0D\x9D\xD0\x83", 16);

const QByteArray EFI_GUIDED_SECTION_TIANO // A31280AD-481E-41B6-95E8-127F4C984779
("\xAD\x80\x12\xA3\x1E\x48\xB6\x41\x95\xE8\x12\x7F\x4C\x98\x47\x79", 16);

const QByteArray EFI_GUIDED_SECTION_LZMA // EE4E5898-3914-4259-9D6E-DC7BD79403CF
("\x98\x58\x4E\xEE\x14\x39\x59\x42\x9D\x6E\xDC\x7B\xD7\x94\x03\xCF", 16);

const QByteArray EFI_FIRMWARE_CONTENTS_SIGNED_GUID //0F9D89E8-9259-4F76-A5AF-0C89E34023DF
("\xE8\x89\x9D\x0F\x59\x92\x76\x4F\xA5\xAF\x0C\x89\xE3\x40\x23\xDF", 16);

// Version section
typedef struct _EFI_VERSION_SECTION {
    UINT8    Size[3];
    UINT8    Type;
    UINT16   BuildNumber;
} EFI_VERSION_SECTION;

typedef struct _EFI_VERSION_SECTION2 {
    UINT8    Size[3];
    UINT8    Type;
    UINT32   ExtendedSize;
    UINT16   BuildNumber;
} EFI_VERSION_SECTION2;

// Freeform subtype GUID section
typedef struct _EFI_FREEFORM_SUBTYPE_GUID_SECTION {
    UINT8    Size[3];
    UINT8    Type;
    EFI_GUID SubTypeGuid;
} EFI_FREEFORM_SUBTYPE_GUID_SECTION;

typedef struct _EFI_FREEFORM_SUBTYPE_GUID_SECTION2 {
    UINT8    Size[3];
    UINT8    Type;
    UINT32   ExtendedSize;
    EFI_GUID SubTypeGuid;
} EFI_FREEFORM_SUBTYPE_GUID_SECTION2;

// Phoenix SCT and HP postcode section
typedef struct _POSTCODE_SECTION {
    UINT8    Size[3];
    UINT8    Type;
    UINT32   Postcode;
} POSTCODE_SECTION;

typedef struct _POSTCODE_SECTION2 {
    UINT8    Size[3];
    UINT8    Type;
    UINT32   ExtendedSize;
    UINT32   Postcode;
} POSTCODE_SECTION2;

// Other sections
typedef EFI_COMMON_SECTION_HEADER  EFI_DISPOSABLE_SECTION;
typedef EFI_COMMON_SECTION_HEADER2 EFI_DISPOSABLE_SECTION2;
typedef EFI_COMMON_SECTION_HEADER  EFI_RAW_SECTION;
typedef EFI_COMMON_SECTION_HEADER2 EFI_RAW_SECTION2;
typedef EFI_COMMON_SECTION_HEADER  EFI_DXE_DEPEX_SECTION;
typedef EFI_COMMON_SECTION_HEADER2 EFI_DXE_DEPEX_SECTION2;
typedef EFI_COMMON_SECTION_HEADER  EFI_PEI_DEPEX_SECTION;
typedef EFI_COMMON_SECTION_HEADER2 EFI_PEI_DEPEX_SECTION2;
typedef EFI_COMMON_SECTION_HEADER  EFI_SMM_DEPEX_SECTION;
typedef EFI_COMMON_SECTION_HEADER2 EFI_SMM_DEPEX_SECTION2;
typedef EFI_COMMON_SECTION_HEADER  EFI_PE32_SECTION;
typedef EFI_COMMON_SECTION_HEADER2 EFI_PE32_SECTION2;
typedef EFI_COMMON_SECTION_HEADER  EFI_PIC_SECTION;
typedef EFI_COMMON_SECTION_HEADER2 EFI_PIC_SECTION2;
typedef EFI_COMMON_SECTION_HEADER  EFI_TE_SECTION;
typedef EFI_COMMON_SECTION_HEADER2 EFI_TE_SECTION2;
typedef EFI_COMMON_SECTION_HEADER  EFI_COMPATIBILITY16_SECTION;
typedef EFI_COMMON_SECTION_HEADER2 EFI_COMPATIBILITY16_SECTION2;
typedef EFI_COMMON_SECTION_HEADER  EFI_FIRMWARE_VOLUME_IMAGE_SECTION;
typedef EFI_COMMON_SECTION_HEADER2 EFI_FIRMWARE_VOLUME_IMAGE_SECTION2;
typedef EFI_COMMON_SECTION_HEADER  EFI_USER_INTERFACE_SECTION;
typedef EFI_COMMON_SECTION_HEADER2 EFI_USER_INTERFACE_SECTION2;

//Section routines
extern UINT32 sizeOfSectionHeader(const EFI_COMMON_SECTION_HEADER* header);

//*****************************************************************************
// EFI Dependency Expression
//*****************************************************************************

#define EFI_DEP_OPCODE_SIZE   1

///
/// If present, this must be the first and only opcode,
/// EFI_DEP_BEFORE is only used by DXE driver.
///
#define EFI_DEP_BEFORE        0x00

///
/// If present, this must be the first and only opcode,
/// EFI_DEP_AFTER is only used by DXE driver.
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
/// EFI_DEP_SOR is only used by DXE driver.
///
#define EFI_DEP_SOR           0x09

//*****************************************************************************
// UEFI Crypto-signed Stuff
//*****************************************************************************

#define WIN_CERT_TYPE_PKCS_SIGNED_DATA 0x0002
#define WIN_CERT_TYPE_EFI_GUID         0x0EF1

typedef struct _WIN_CERTIFICATE {
    UINT32  Length;
    UINT16  Revision;
    UINT16  CertificateType;
    //UINT8 CertData[];
} WIN_CERTIFICATE;

typedef struct _WIN_CERTIFICATE_UEFI_GUID {
    WIN_CERTIFICATE   Header;     // Standard WIN_CERTIFICATE
    EFI_GUID          CertType;   // Determines format of CertData
    // UINT8          CertData[]; // Certificate data follows
} WIN_CERTIFICATE_UEFI_GUID;

// WIN_CERTIFICATE_UEFI_GUID.CertType
const QByteArray EFI_CERT_TYPE_RSA2048_SHA256_GUID
("\x14\x74\x71\xA7\x16\xC6\x77\x49\x94\x20\x84\x47\x12\xA7\x35\xBF");
const QByteArray EFI_CERT_TYPE_PKCS7_GUID
("\x9D\xD2\xAF\x4A\xDF\x68\xEE\x49\x8A\xA9\x34\x7D\x37\x56\x65\xA7");

// WIN_CERTIFICATE_UEFI_GUID.CertData
typedef struct _EFI_CERT_BLOCK_RSA_2048_SHA256 {
    UINT32  HashType;
    UINT8   PublicKey[256];
    UINT8   Signature[256];
} EFI_CERT_BLOCK_RSA_2048_SHA256;

// Restore previous packing rules
#pragma pack(pop)

#endif
