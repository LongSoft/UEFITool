/* ffs.h 

Copyright (c) 2013, Nikolaj Schlej. All rights reserved.
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
#include <QObject>
#include "basetypes.h"

// C++ functions
// GUID to QString routine
extern QString guidToQString(const EFI_GUID guid);
// File type to QString routine
extern QString fileTypeToQString(const UINT8 type);
// Section type to QString routine
extern QString sectionTypeToQString(const UINT8 type);

#ifdef __cplusplus
extern "C" {
#endif
// Make sure we use right packing rules
#pragma pack(push,1)

//*****************************************************************************
// EFI Capsule
//*****************************************************************************
// Capsule header
typedef struct {
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
const QByteArray EFI_CAPSULE_GUID("\xBD\x86\x66\x3B\x76\x0D\x30\x40\xB7\x0E\xB5\x51\x9E\x2F\xC5\xA0", 16);

// AMI Aptio extended capsule header
typedef struct {
    EFI_CAPSULE_HEADER    CapsuleHeader;           
    UINT16                RomImageOffset;	// offset in bytes from the beginning of the capsule header to the start of
                                            // the capsule volume
    //!TODO: Enable certificate and rom layout reading
	//UINT16                RomLayoutOffset;	// offset to the table of the module descriptors in the capsule's volume
                                                // that are included in the signature calculation
    //FW_CERTIFICATE      FWCert;           
    //ROM_AREA			  RomAreaMap[1];
} APTIO_CAPSULE_HEADER;

// AMI Aptio extended capsule GUID
const QByteArray APTIO_CAPSULE_GUID("\x8B\xA6\x3C\x4A\x23\x77\xFB\x48\x80\x3D\x57\x8C\xC1\xFE\xC4\x4D", 16); 

//*****************************************************************************
// EFI Firmware Volume
//*****************************************************************************
// Firmware block map entry
// FvBlockMap ends with an entry {0x00000000, 0x00000000}
typedef struct {
  UINT32  NumBlocks;
  UINT32  BlockLength;
} EFI_FV_BLOCK_MAP_ENTRY;

// Volume header
typedef struct {
	UINT8                  ZeroVector[16];
	EFI_GUID               FileSystemGuid;
	UINT64                 FvLength;
	UINT32                 Signature;
	UINT32                 Attributes;
	UINT16                 HeaderLength;
	UINT16                 Checksum;
    UINT16                 ExtHeaderOffset;  //Reserved in Revision 1
    UINT8                  Reserved[1];
	UINT8                  Revision;
	//EFI_FV_BLOCK_MAP_ENTRY FvBlockMap[1];	
} EFI_FIRMWARE_VOLUME_HEADER;

// Filesystem GUIDs
const QByteArray EFI_FIRMWARE_FILE_SYSTEM_GUID
("\xD9\x54\x93\x7A\x68\x04\x4A\x44\x81\xCE\x0B\xF6\x17\xD8\x90\xDF", 16); 
const QByteArray EFI_FIRMWARE_FILE_SYSTEM2_GUID
("\x78\xE5\x8C\x8C\x3D\x8A\x1C\x4F\x99\x35\x89\x61\x85\xC3\x2D\xD3", 16);

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
typedef struct {
  EFI_GUID          FvName;
  UINT32            ExtHeaderSize;
} EFI_FIRMWARE_VOLUME_EXT_HEADER;

// Extended header entry
// The extended header entries follow each other and are 
// terminated by ExtHeaderType EFI_FV_EXT_TYPE_END
#define EFI_FV_EXT_TYPE_END        0x00
typedef struct {
  UINT16  ExtEntrySize;
  UINT16  ExtEntryType;
} EFI_FIRMWARE_VOLUME_EXT_ENTRY;

// GUID that maps OEM file types to GUIDs
#define EFI_FV_EXT_TYPE_OEM_TYPE   0x01
typedef struct {
  EFI_FIRMWARE_VOLUME_EXT_ENTRY    Header;
  UINT32                           TypeMask;
  //EFI_GUID                         Types[1];
} EFI_FIRMWARE_VOLUME_EXT_HEADER_OEM_TYPE;

#define EFI_FV_EXT_TYPE_GUID_TYPE  0x02
typedef struct {
 EFI_FIRMWARE_VOLUME_EXT_ENTRY Header;
 EFI_GUID FormatType;
 //UINT8 Data[];
} EFI_FIRMWARE_VOLUME_EXT_ENTRY_GUID_TYPE;


// Volume header 16bit checksum calculation routine
extern UINT16 calculateChecksum16(UINT8* buffer, UINT32 bufferSize);

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
typedef struct {
	EFI_GUID                Name;
    EFI_FFS_INTEGRITY_CHECK IntegrityCheck;
	UINT8                   Type;
	UINT8                   Attributes;
	UINT8                   Size[3];
	UINT8                   State;
} EFI_FFS_FILE_HEADER;

// Large file header
//typedef struct {
//	EFI_GUID                Name;
//  EFI_FFS_INTEGRITY_CHECK IntegrityCheck;
//	UINT8                   Type;
//	UINT8                   Attributes;
//	UINT8                   Size[3];
//	UINT8                   State;
//  UINT12                  ExtendedSize;
//} EFI_FFS_FILE_HEADER2;

// Standart data checksum, used if FFS_ATTRIB_CHECKSUM is clear
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
// Revision 1
#define FFS_ATTRIB_TAIL_PRESENT       0x01
#define FFS_ATTRIB_RECOVERY           0x02
//#define FFS_ATTRIB_HEADER_EXTENSION   0x04 //Must be set to zero in Revision 1
#define FFS_ATTRIB_DATA_ALIGNMENT     0x38
#define FFS_ATTRIB_CHECKSUM           0x40
// Revision 2
#define FFS_ATTRIB_LARGE_FILE         0x01
#define FFS_ATTRIB_FIXED              0x04

// FFS alignment table
extern const UINT8 ffsAlignmentTable[];

// File states
#define EFI_FILE_HEADER_CONSTRUCTION    0x01
#define EFI_FILE_HEADER_VALID           0x02
#define EFI_FILE_DATA_VALID             0x04
#define EFI_FILE_MARKED_FOR_UPDATE      0x08
#define EFI_FILE_DELETED                0x10
#define EFI_FILE_HEADER_INVALID         0x20

// Volume top file
const QByteArray EFI_FFS_VOLUME_TOP_FILE_GUID 
("\x2E\x06\xA0\x1B\x79\xC7\x82\x45\x85\x66\x33\x6A\xE8\xF7\x8F\x09", 16);

// FFS size convertion routines
extern VOID uint32ToUint24(UINT32 size, UINT8* ffsSize);
extern UINT32 uint24ToUint32(UINT8* ffsSize);
// FFS file 8bit checksum calculation routine
extern UINT8 calculateChecksum8(UINT8* buffer, UINT32 bufferSize);

//*****************************************************************************
// EFI FFS File Section
//*****************************************************************************
// Common section header
typedef struct {
    UINT8    Size[3];
    UINT8    Type;
} EFI_COMMON_SECTION_HEADER;

// Large file common section header
//typedef struct {
//    UINT8    Size[3];    //Must be 0xFFFFFF for this header to be used
//    UINT8    Type;      
//    UINT32 ExtendedSize;
//} EFI_COMMON_SECTION_HEADER2;

// File section types
#define EFI_SECTION_ALL 0x00 // Imposible attribute for file in the FS

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

// Compression section
typedef struct {
    UINT8    Size[3];
    UINT8    Type;
    UINT32   UncompressedLength;
    UINT8    CompressionType;
} EFI_COMPRESSION_SECTION;

// Compression types
#define EFI_NOT_COMPRESSED          0x00 
#define EFI_STANDARD_COMPRESSION    0x01 //Tiano for Revision 2, EFI for Revision 1
#define EFI_CUSTOMIZED_COMPRESSION  0x02 //LZMA for Revision 2 Tiano for Revision 1

//GUID defined section
typedef struct {
    UINT8    Size[3];
    UINT8    Type;
    EFI_GUID SectionDefinitionGuid;
    UINT16   DataOffset;
    UINT16   Attributes;
} EFI_GUID_DEFINED_SECTION;

// Attributes for GUID defined section
#define EFI_GUIDED_SECTION_PROCESSING_REQUIRED  0x01
#define EFI_GUIDED_SECTION_AUTH_STATUS_VALID    0x02

// Version section
typedef struct {
    UINT8    Size[3];
    UINT8    Type;
    UINT16   BuildNumber;
    //CHAR16   VersionString[1];
} EFI_VERSION_SECTION;

// Freeform subtype GUID section
typedef struct {
    UINT8    Size[3];
    UINT8    Type;
    EFI_GUID SubTypeGuid;
} EFI_FREEFORM_SUBTYPE_GUID_SECTION;

// User interface section
typedef struct {
    UINT8    Size[3];
    UINT8    Type;
    //CHAR16   FileNameString[1];
} EFI_USER_INTERFACE_SECTION;

// Other sections
typedef EFI_COMMON_SECTION_HEADER EFI_DISPOSABLE_SECTION;
typedef EFI_COMMON_SECTION_HEADER EFI_RAW_SECTION;
typedef EFI_COMMON_SECTION_HEADER EFI_DXE_DEPEX_SECTION;
typedef EFI_COMMON_SECTION_HEADER EFI_PEI_DEPEX_SECTION;
typedef EFI_COMMON_SECTION_HEADER EFI_SMM_DEPEX_SECTION;
typedef EFI_COMMON_SECTION_HEADER EFI_PE32_SECTION;
typedef EFI_COMMON_SECTION_HEADER EFI_PIC_SECTION;
typedef EFI_COMMON_SECTION_HEADER EFI_TE_SECTION;
typedef EFI_COMMON_SECTION_HEADER EFI_COMPATIBILITY16_SECTION;
typedef EFI_COMMON_SECTION_HEADER EFI_FIRMWARE_VOLUME_IMAGE_SECTION;

//Section routines
extern UINT32 sizeOfSectionHeaderOfType(const UINT8 type);

// Restore previous packing rules
#pragma pack(pop)

#ifdef __cplusplus
}
#endif
#endif