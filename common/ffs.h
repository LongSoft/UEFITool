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
extern const UByteArray EFI_FMP_CAPSULE_GUID; // 6DCBD5ED-E82D-4C44-BDA1-7194199AD92A

// Standard EFI capsule GUID
extern const UByteArray EFI_CAPSULE_GUID; // 3B6686BD-0D76-4030-B70E-B5519E2FC5A0

// Intel capsule GUID
extern const UByteArray INTEL_CAPSULE_GUID; // 539182B9-ABB5-4391-B69A-E3A943F72FCC

// Lenovo capsule GUID
extern const UByteArray LENOVO_CAPSULE_GUID; // E20BAFD3-9914-4F4F-9537-3129E090EB3C

// Another Lenovo capsule GUID
extern const UByteArray LENOVO2_CAPSULE_GUID; // 25B5FE76-8243-4A5C-A9BD-7EE3246198B5

// Toshiba EFI Capsule header
typedef struct TOSHIBA_CAPSULE_HEADER_ {
    EFI_GUID  CapsuleGuid;
    UINT32    HeaderSize;
    UINT32    FullSize;
    UINT32    Flags;
} TOSHIBA_CAPSULE_HEADER;

// Toshiba capsule GUID
extern const UByteArray TOSHIBA_CAPSULE_GUID; // 3BE07062-1D51-45D2-832B-F093257ED461

// AMI Aptio extended capsule header
typedef struct APTIO_CAPSULE_HEADER_ {
    EFI_CAPSULE_HEADER    CapsuleHeader;
    UINT16                RomImageOffset;  // offset in bytes from the beginning of the capsule header to the start of the capsule volume
    UINT16                RomLayoutOffset; // offset to the table of the module descriptors in the capsule's volume that are included in the signature calculation
    //FW_CERTIFICATE      FWCert;
    //ROM_AREA            RomAreaMap[1];
} APTIO_CAPSULE_HEADER;

// AMI Aptio signed extended capsule GUID
extern const UByteArray APTIO_SIGNED_CAPSULE_GUID; // 4A3CA68B-7723-48FB-803D-578CC1FEC44D

// AMI Aptio unsigned extended capsule GUID
extern const UByteArray APTIO_UNSIGNED_CAPSULE_GUID; // 14EEBB90-890A-43DB-AED1-5D3C4588A418

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
extern const UByteArray EFI_FIRMWARE_FILE_SYSTEM_GUID; // 7A9354D9-0468-444A-81CE-0BF617D890DF

extern const UByteArray EFI_FIRMWARE_FILE_SYSTEM2_GUID; // 8C8CE578-8A3D-4F1C-9935-896185C32DD3

extern const UByteArray EFI_FIRMWARE_FILE_SYSTEM3_GUID; // 5473C07A-3DCB-4DCA-BD6F-1E9689E7349A

// Vendor-specific file system GUIDs
extern const UByteArray EFI_APPLE_IMMUTABLE_FV_GUID; // 04ADEEAD-61FF-4D31-B6BA-64F8BF901F5A

extern const UByteArray EFI_APPLE_AUTHENTICATION_FV_GUID; // BD001B8C-6A71-487B-A14F-0C2A2DCF7A5D

extern const UByteArray EFI_APPLE_MICROCODE_VOLUME_GUID; // 153D2197-29BD-44DC-AC59-887F70E41A6B
#define EFI_APPLE_MICROCODE_VOLUME_HEADER_SIZE 0x100

extern const UByteArray EFI_INTEL_FILE_SYSTEM_GUID; // AD3FFFFF-D28B-44C4-9F13-9EA98A97F9F0

extern const UByteArray EFI_INTEL_FILE_SYSTEM2_GUID; // D6A1CD70-4B33-4994-A6EA-375F2CCC5437

extern const UByteArray EFI_SONY_FILE_SYSTEM_GUID; // 4F494156-AED6-4D64-A537-B8A5557BCEEC

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
        UINT8 Header;
        UINT8 File;
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
extern const UByteArray EFI_PEI_APRIORI_FILE_GUID; // 1B45CC0A-156A-428A-AF62-49864DA0E6E6

// DXE apriori file
extern const UByteArray EFI_DXE_APRIORI_FILE_GUID; // FC510EE7-FFDC-11D4-BD41-0080C73C8881

// Volume top file
extern const UByteArray EFI_FFS_VOLUME_TOP_FILE_GUID; // 1BA0062E-C779-4582-8566-336AE8F78F09

// AMI padding file GUID
extern const UByteArray EFI_FFS_PAD_FILE_GUID; // E4536585-7909-4A60-B5C6-ECDEA6EBFB5

// AMI DXE core file
extern const UByteArray AMI_CORE_DXE_GUID; // 5AE3F37E-4EAE-41AE-8240-35465B5E81EB

// EDK2 DXE core file
extern const UByteArray EFI_DXE_CORE_GUID; // D6A2CB7F-6A18-4E2F-B43B-9920A733700A

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
extern const UByteArray EFI_GUIDED_SECTION_CRC32; // FC1BCDB0-7D31-49AA-936A-A4600D9DD083
extern const UByteArray EFI_GUIDED_SECTION_TIANO; // A31280AD-481E-41B6-95E8-127F4C984779
extern const UByteArray EFI_GUIDED_SECTION_LZMA; // EE4E5898-3914-4259-9D6E-DC7BD79403CF
extern const UByteArray EFI_GUIDED_SECTION_LZMA_HP; // 0ED85E23-F253-413F-A03C-901987B04397
extern const UByteArray EFI_GUIDED_SECTION_LZMAF86; // D42AE6BD-1352-4BFB-909A-CA72A6EAE889
extern const UByteArray EFI_GUIDED_SECTION_GZIP; // 1D301FE9-BE79-4353-91C2-D23BC959AE0C
extern const UByteArray EFI_FIRMWARE_CONTENTS_SIGNED_GUID; // 0F9D89E8-9259-4F76-A5AF-0C89E34023DF

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
extern const UByteArray EFI_CERT_TYPE_RSA2048_SHA256_GUID; // A7717414-C616-4977-9420-844712A735BF

// WIN_CERTIFICATE_UEFI_GUID.CertData
typedef struct EFI_CERT_BLOCK_RSA2048_SHA256_ {
    EFI_GUID  HashType;
    UINT8     PublicKey[256];
    UINT8     Signature[256];
} EFI_CERT_BLOCK_RSA2048_SHA256;

extern const UByteArray EFI_HASH_ALGORITHM_SHA256_GUID; // 51AA59DE-FDF2-4EA3-BC63-875FB7842EE9

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
// X86 Startup AP Data
//*****************************************************************************
#define RECOVERY_STARTUP_AP_DATA_X86_SIZE 0x10
extern const UByteArray RECOVERY_STARTUP_AP_DATA_X86_64K;
extern const UByteArray RECOVERY_STARTUP_AP_DATA_X86_128K;

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
#define BPDT_ENTRY_TYPE_USF_GPP_LUN_ID   13
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
#define BPDT_ENTRY_TYPE_USB_PHY          31
#define BPDT_ENTRY_TYPE_PCHC             32
#define BPDT_ENTRY_TYPE_SAMF             41
#define BPDT_ENTRY_TYPE_PPHY             42

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
#define CPD_EXT_TYPE_GMF_CERTIFICATE         30
#define CPD_EXT_TYPE_GMF_BODY                31
#define CPD_EXT_TYPE_KEY_MANIFEST_EXT        34
#define CPD_EXT_TYPE_SIGNED_PACKAGE_INFO_EXT 35
#define CPD_EXT_TYPE_SPS_PLATFORM_ID         50

typedef struct CPD_EXT_SIGNED_PACKAGE_INFO_MODULE_ {
    UINT8  Name[12];
    UINT8  Type;
    UINT8  HashAlgorithm;
    UINT16 HashSize;
    UINT32 MetadataSize;
    // UINT8  MetadataHash[]; with the actual hash size is 32 or 48 bytes
} CPD_EXT_SIGNED_PACKAGE_INFO_MODULE;

static const size_t CpdExtSignedPkgMetadataHashOffset = sizeof(CPD_EXT_SIGNED_PACKAGE_INFO_MODULE);

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
    UINT32 CompressionType;
    UINT32 UncompressedSize;
    UINT32 CompressedSize;
    UINT32 GlobalModuleId;
    // UINT8  ImageHash[]; with the actual hash size is 32 or 48 bytes
} CPD_EXT_MODULE_ATTRIBUTES;

static const size_t CpdExtModuleImageHashOffset = sizeof(CPD_EXT_MODULE_ATTRIBUTES);

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
    UINT8  CompletePartitionHash[48];
    UINT8  Reserved[4];
} CPD_EXT_IFWI_PARTITION_MANIFEST;

//*****************************************************************************
// Protected range
//*****************************************************************************
extern const UByteArray PROTECTED_RANGE_VENDOR_HASH_FILE_GUID_PHOENIX; // 389CC6F2-1EA8-467B-AB8A-78E769AE2A15

#define BG_VENDOR_HASH_FILE_SIGNATURE_PHOENIX 0x4C42544853414824ULL // '$HASHTBL'

extern const UByteArray PROTECTED_RANGE_VENDOR_HASH_FILE_GUID_AMI; // CBC91F44-A4BC-4A5B-8696-703451D0B053

typedef struct BG_VENDOR_HASH_FILE_ENTRY
{
    UINT8  Hash[SHA256_HASH_SIZE];
    UINT32 Base;
    UINT32 Size;
} PROTECTED_RANGE_VENDOR_HASH_FILE_ENTRY;

typedef struct PROTECTED_RANGE_VENDOR_HASH_FILE_HEADER_PHOENIX_
{
    UINT64 Signature;
    UINT32 NumEntries;
    //BG_VENDOR_HASH_FILE_ENTRY Entries[];
} PROTECTED_RANGE_VENDOR_HASH_FILE_HEADER_PHOENIX;

typedef struct PROTECTED_RANGE_VENDOR_HASH_FILE_HEADER_AMI_V1_
{
    UINT8  Hash[SHA256_HASH_SIZE];
    UINT32 Size;
    // Base is derived from flash map, will be detected as root volume with DXE core
} PROTECTED_RANGE_VENDOR_HASH_FILE_HEADER_AMI_V1;

typedef struct PROTECTED_RANGE_VENDOR_HASH_FILE_HEADER_AMI_V2_
{
    BG_VENDOR_HASH_FILE_ENTRY Hash0;
    BG_VENDOR_HASH_FILE_ENTRY Hash1;
} PROTECTED_RANGE_VENDOR_HASH_FILE_HEADER_AMI_V2;

typedef struct PROTECTED_RANGE_VENDOR_HASH_FILE_HEADER_AMI_V3_
{
    UINT8 Hash[SHA256_HASH_SIZE];
    // UINT32 Base[SOME_HARDCODED_N]
    // UINT32 Size[SOME_HARDCODED_N];
} PROTECTED_RANGE_VENDOR_HASH_FILE_HEADER_AMI_V3;

//
// AMI ROM Hole files
//
extern const UByteArray AMI_ROM_HOLE_FILE_GUID_0; //05CA01FC-0FC1-11DC-9011-00173153EBA8
extern const UByteArray AMI_ROM_HOLE_FILE_GUID_1; //05CA01FD-0FC1-11DC-9011-00173153EBA8
extern const UByteArray AMI_ROM_HOLE_FILE_GUID_2; //05CA01FE-0FC1-11DC-9011-00173153EBA8
extern const UByteArray AMI_ROM_HOLE_FILE_GUID_3; //05CA01FF-0FC1-11DC-9011-00173153EBA8
extern const UByteArray AMI_ROM_HOLE_FILE_GUID_4; //05CA0200-0FC1-11DC-9011-00173153EBA8
extern const UByteArray AMI_ROM_HOLE_FILE_GUID_5; //05CA0201-0FC1-11DC-9011-00173153EBA8
extern const UByteArray AMI_ROM_HOLE_FILE_GUID_6; //05CA0202-0FC1-11DC-9011-00173153EBA8
extern const UByteArray AMI_ROM_HOLE_FILE_GUID_7; //05CA0203-0FC1-11DC-9011-00173153EBA8
extern const UByteArray AMI_ROM_HOLE_FILE_GUID_8; //05CA0204-0FC1-11DC-9011-00173153EBA8
extern const UByteArray AMI_ROM_HOLE_FILE_GUID_9; //05CA0205-0FC1-11DC-9011-00173153EBA8
extern const UByteArray AMI_ROM_HOLE_FILE_GUID_10; //05CA0206-0FC1-11DC-9011-00173153EBA8
extern const UByteArray AMI_ROM_HOLE_FILE_GUID_11; //05CA0207-0FC1-11DC-9011-00173153EBA8
extern const UByteArray AMI_ROM_HOLE_FILE_GUID_12; //05CA0208-0FC1-11DC-9011-00173153EBA8
extern const UByteArray AMI_ROM_HOLE_FILE_GUID_13; //05CA0209-0FC1-11DC-9011-00173153EBA8
extern const UByteArray AMI_ROM_HOLE_FILE_GUID_14; //05CA020A-0FC1-11DC-9011-00173153EBA8
extern const UByteArray AMI_ROM_HOLE_FILE_GUID_15; //05CA020B-0FC1-11DC-9011-00173153EBA8

// Restore previous packing rules
#pragma pack(pop)

#endif // FFS_H
