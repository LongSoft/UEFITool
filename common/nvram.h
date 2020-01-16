/* nvram.h

Copyright (c) 2016, Nikolaj Schlej. All rights reserved.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#ifndef NVRAM_H
#define NVRAM_H

#include "basetypes.h"
#include "ubytearray.h"
#include "ustring.h"

// Make sure we use right packing rules
#pragma pack(push, 1)

//
// NVAR store and entry
//

// CEF5B9A3-476D-497F-9FDC-E98143E0422C
const UByteArray NVRAM_NVAR_STORE_FILE_GUID
("\xA3\xB9\xF5\xCE\x6D\x47\x7F\x49\x9F\xDC\xE9\x81\x43\xE0\x42\x2C", 16);

// 9221315B-30BB-46B5-813E-1B1BF4712BD3
const UByteArray NVRAM_NVAR_EXTERNAL_DEFAULTS_FILE_GUID
("\x5B\x31\x21\x92\xBB\x30\xB5\x46\x81\x3E\x1B\x1B\xF4\x71\x2B\xD3", 16);

// 77D3DC50-D42B-4916-AC80-8F469035D150
const UByteArray NVRAM_NVAR_PEI_EXTERNAL_DEFAULTS_FILE_GUID
("\x50\xDC\xD3\x77\x2B\xD4\x16\x49\xAC\x80\x8F\x46\x90\x35\xD1\x50", 16);

// AF516361-B4C5-436E-A7E3-A149A31B1461
const UByteArray NVRAM_NVAR_BB_DEFAULTS_FILE_GUID
("\x61\x63\x51\xAF\xC5\xB4\x6E\x43\xA7\xE3\xA1\x49\xA3\x1B\x14\x61", 16);

extern UString nvarAttributesToUString(const UINT8 attributes);
extern UString nvarExtendedAttributesToUString(const UINT8 attributes);
extern UString efiTimeToUString(const EFI_TIME & time);

typedef struct NVAR_ENTRY_HEADER_ {
    UINT32 Signature;      // NVAR
    UINT16 Size;           // Size of the entry including header
    UINT32 Next : 24;      // Offset to the next entry in a list, or empty if the latest in the list
    UINT32 Attributes : 8; // Attributes
} NVAR_ENTRY_HEADER;

// NVAR signature
#define NVRAM_NVAR_ENTRY_SIGNATURE         0x5241564E

// Attributes
#define NVRAM_NVAR_ENTRY_RUNTIME           0x01
#define NVRAM_NVAR_ENTRY_ASCII_NAME        0x02
#define NVRAM_NVAR_ENTRY_GUID              0x04
#define NVRAM_NVAR_ENTRY_DATA_ONLY         0x08
#define NVRAM_NVAR_ENTRY_EXT_HEADER        0x10
#define NVRAM_NVAR_ENTRY_HW_ERROR_RECORD   0x20
#define NVRAM_NVAR_ENTRY_AUTH_WRITE        0x40
#define NVRAM_NVAR_ENTRY_VALID             0x80

// Extended attributes
#define NVRAM_NVAR_ENTRY_EXT_CHECKSUM      0x01
#define NVRAM_NVAR_ENTRY_EXT_AUTH_WRITE    0x10
#define NVRAM_NVAR_ENTRY_EXT_TIME_BASED    0x20
#define NVRAM_NVAR_ENTRY_EXT_UNKNOWN_MASK  0xCE

//
// TianoCore VSS store and variables
//

// FFF12B8D-7696-4C8B-A985-2747075B4F50
const UByteArray NVRAM_MAIN_STORE_VOLUME_GUID
("\x8D\x2B\xF1\xFF\x96\x76\x8B\x4C\xA9\x85\x27\x47\x07\x5B\x4F\x50", 16);

// 00504624-8A59-4EEB-BD0F-6B36E96128E0
const UByteArray NVRAM_ADDITIONAL_STORE_VOLUME_GUID
("\x24\x46\x50\x00\x59\x8A\xEB\x4E\xBD\x0F\x6B\x36\xE9\x61\x28\xE0", 16);

#define NVRAM_VSS_STORE_SIGNATURE            0x53535624 // $VSS
#define NVRAM_APPLE_SVS_STORE_SIGNATURE      0x53565324 // $SVS
#define NVRAM_APPLE_NSS_STORE_SIGNATURE      0x53534E24 // $NSS
#define NVRAM_APPLE_FSYS_STORE_SIGNATURE     0x73797346 // Fsys
#define NVRAM_APPLE_GAID_STORE_SIGNATURE     0x64696147 // Gaid
#define NVRAM_VSS_VARIABLE_START_ID          0x55AA

// Variable store header flags
#define NVRAM_VSS_VARIABLE_STORE_FORMATTED  0x5a
#define NVRAM_VSS_VARIABLE_STORE_HEALTHY    0xfe

// Variable store status
#define NVRAM_VSS_VARIABLE_STORE_STATUS_RAW     0
#define NVRAM_VSS_VARIABLE_STORE_STATUS_VALID   1
#define NVRAM_VSS_VARIABLE_STORE_STATUS_INVALID 2
#define NVRAM_VSS_VARIABLE_STORE_STATUS_UNKNOWN 3

// Variable store header
typedef struct VSS_VARIABLE_STORE_HEADER_ {
    UINT32  Signature; // $VSS signature
    UINT32  Size;      // Size of variable store, including store header
    UINT8   Format;    // Store format state
    UINT8   State;     // Store health state
    UINT16  Unknown;   // Used in Apple $SVS varstores
    UINT32  : 32;
} VSS_VARIABLE_STORE_HEADER;

// Normal variable header
typedef struct VSS_VARIABLE_HEADER_ {
    UINT16    StartId;    // Variable start marker AA55
    UINT8     State;      // Variable state
    UINT8     Reserved;
    UINT32    Attributes; // Variable attributes
    UINT32    NameSize;   // Size of variable name, stored as null-terminated UCS2 string
    UINT32    DataSize;   // Size of variable data without header and name
    EFI_GUID  VendorGuid; // Variable vendor GUID
} VSS_VARIABLE_HEADER;

// Intel variable header
typedef struct VSS_INTEL_VARIABLE_HEADER_ {
    UINT16    StartId;    // Variable start marker AA55
    UINT8     State;      // Variable state
    UINT8     Reserved;
    UINT32    Attributes; // Variable attributes
    UINT32    TotalSize;  // Size of variable including header
    EFI_GUID  VendorGuid; // Variable vendor GUID
} VSS_INTEL_VARIABLE_HEADER;

// Apple variation of normal variable header, with one new field
typedef struct VSS_APPLE_VARIABLE_HEADER_ {
    UINT16    StartId;    // Variable start marker AA55
    UINT8     State;      // Variable state
    UINT8     Reserved;
    UINT32    Attributes; // Variable attributes
    UINT32    NameSize;   // Size of variable name, stored as null-terminated UCS2 string
    UINT32    DataSize;   // Size of variable data without header and name
    EFI_GUID  VendorGuid; // Variable vendor GUID
    UINT32    DataCrc32;  // CRC32 of the data
} VSS_APPLE_VARIABLE_HEADER;

// Authenticated variable header, used for SecureBoot vars
typedef struct VSS_AUTH_VARIABLE_HEADER_ {
    UINT16    StartId;          // Variable start marker AA55
    UINT8     State;            // Variable state 
    UINT8     Reserved;
    UINT32    Attributes;       // Variable attributes
    UINT64    MonotonicCounter; // Monotonic counter against replay attack
    EFI_TIME  Timestamp;        // Time stamp against replay attack
    UINT32    PubKeyIndex;      // Index in PubKey database
    UINT32    NameSize;         // Size of variable name, stored as null-terminated UCS2 string
    UINT32    DataSize;         // Size of variable data without header and name
    EFI_GUID  VendorGuid;       // Variable vendor GUID
} VSS_AUTH_VARIABLE_HEADER;

// VSS variable states
#define NVRAM_VSS_VARIABLE_IN_DELETED_TRANSITION     0xfe  // Variable is in obsolete transistion
#define NVRAM_VSS_VARIABLE_DELETED                   0xfd  // Variable is obsolete
#define NVRAM_VSS_VARIABLE_HEADER_VALID              0x7f  // Variable has valid header
#define NVRAM_VSS_VARIABLE_ADDED                     0x3f  // Variable has been completely added
#define NVRAM_VSS_INTEL_VARIABLE_VALID               0xfc  // Intel special variable valid
#define NVRAM_VSS_INTEL_VARIABLE_INVALID             0xf8  // Intel special variable invalid 

// VSS variable attributes
#define NVRAM_VSS_VARIABLE_NON_VOLATILE                          0x00000001
#define NVRAM_VSS_VARIABLE_BOOTSERVICE_ACCESS                    0x00000002
#define NVRAM_VSS_VARIABLE_RUNTIME_ACCESS                        0x00000004
#define NVRAM_VSS_VARIABLE_HARDWARE_ERROR_RECORD                 0x00000008
#define NVRAM_VSS_VARIABLE_AUTHENTICATED_WRITE_ACCESS            0x00000010
#define NVRAM_VSS_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS 0x00000020
#define NVRAM_VSS_VARIABLE_APPEND_WRITE                          0x00000040
#define NVRAM_VSS_VARIABLE_APPLE_DATA_CHECKSUM                   0x80000000
#define NVRAM_VSS_VARIABLE_UNKNOWN_MASK                          0x7FFFFF80

extern UString vssAttributesToUString(const UINT32 attributes);

//
// VSS2 variables
//

// aaf32c78-947b-439a-a180-2e144ec37792
#define NVRAM_VSS2_AUTH_VAR_KEY_DATABASE_GUID_PART1 0xaaf32c78
const UByteArray NVRAM_VSS2_AUTH_VAR_KEY_DATABASE_GUID
("\x78\x2C\xF3\xAA\x7B\x94\x9A\x43\xA1\x80\x2E\x14\x4E\xC3\x77\x92");

#define NVRAM_VSS2_STORE_GUID_PART1 0xddcf3617
const UByteArray NVRAM_VSS2_STORE_GUID
("\x17\x36\xCF\xDD\x75\x32\x64\x41\x98\xB6\xFE\x85\x70\x7F\xFE\x7D");

const UByteArray NVRAM_FDC_STORE_GUID
("\x16\x36\xCF\xDD\x75\x32\x64\x41\x98\xB6\xFE\x85\x70\x7F\xFE\x7D");

// Variable store header
typedef struct VSS2_VARIABLE_STORE_HEADER_ {
    EFI_GUID Signature; // VSS2 Store Guid
    UINT32   Size;      // Size of variable store, including store header
    UINT8    Format;    // Store format state
    UINT8    State;     // Store health state
    UINT16   Unknown;
    UINT32   : 32;
} VSS2_VARIABLE_STORE_HEADER;

// VSS2 entries are 4-bytes aligned in VSS2 stores

//
// _FDC region
//

#define NVRAM_FDC_VOLUME_SIGNATURE 0x4344465F

typedef struct FDC_VOLUME_HEADER_ {
    UINT32 Signature; //_FDC signature
    UINT32 Size;      // Size of the whole region
    //EFI_FIRMWARE_VOLUME_HEADER VolumeHeader;
    //EFI_FV_BLOCK_MAP_ENTRY FvBlockMap[2];
    //VSS_VARIABLE_STORE_HEADER VssHeader;
} FDC_VOLUME_HEADER;

//
// FTW block
//
#define EFI_FAULT_TOLERANT_WORKING_BLOCK_VALID   0x1
#define EFI_FAULT_TOLERANT_WORKING_BLOCK_INVALID 0x2

// 9E58292B-7C68-497D-0ACE6500FD9F1B95
const UByteArray EDKII_WORKING_BLOCK_SIGNATURE_GUID
("\x2B\x29\x58\x9E\x68\x7C\x7D\x49\x0A\xCE\x65\x00\xFD\x9F\x1B\x95", 16);

// 9E58292B-7C68-497D-A0CE6500FD9F1B95
const UByteArray VSS2_WORKING_BLOCK_SIGNATURE_GUID
("\x2B\x29\x58\x9E\x68\x7C\x7D\x49\xA0\xCE\x65\x00\xFD\x9F\x1B\x95", 16);

#define NVRAM_MAIN_STORE_VOLUME_GUID_DATA1       0xFFF12B8D
#define EDKII_WORKING_BLOCK_SIGNATURE_GUID_DATA1 0x9E58292B

typedef struct EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER32_ {
    EFI_GUID  Signature; // NVRAM_MAIN_STORE_VOLUME_GUID
    UINT32    Crc; // Crc32 of the header with empty Crc and State fields
    UINT8     State;
    UINT8     Reserved[3];
    UINT32    WriteQueueSize; // Size of the FTW block without the header
    //UINT8   WriteQueue[WriteQueueSize];
} EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER32;

typedef struct EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER64_ {
    EFI_GUID  Signature; // NVRAM_MAIN_STORE_VOLUME_GUID or EDKII_WORKING_BLOCK_SIGNATURE_GUID
    UINT32    Crc; // Crc32 of the header with empty Crc and State fields
    UINT8     State;
    UINT8     Reserved[3];
    UINT64    WriteQueueSize; // Size of the FTW block without the header
    //UINT8   WriteQueue[WriteQueueSize];
} EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER64;

//
// Apple Fsys store
//

typedef struct APPLE_FSYS_STORE_HEADER_ {
    UINT32  Signature;  // Fsys or Gaid signature
    UINT8   Unknown0;   // Still unknown
    UINT32  Unknown1;   // Still unknown
    UINT16  Size;       // Size of variable store
} APPLE_FSYS_STORE_HEADER;

// Apple Fsys entry format
// UINT8 NameLength;
// CHAR8 Name[];
// UINT16 DataLength;
// UINT8 Data[]
// Store ends with a chunk named "EOF" without data
// All free bytes in store are zeroed
// Has CRC32 of the whole store without checksum field at the end

//
// EVSA store and entries
//

#define NVRAM_EVSA_STORE_SIGNATURE 0x41535645

#define NVRAM_EVSA_ENTRY_TYPE_STORE        0xEC
#define NVRAM_EVSA_ENTRY_TYPE_GUID1        0xED
#define NVRAM_EVSA_ENTRY_TYPE_GUID2        0xE1
#define NVRAM_EVSA_ENTRY_TYPE_NAME1        0xEE
#define NVRAM_EVSA_ENTRY_TYPE_NAME2        0xE2
#define NVRAM_EVSA_ENTRY_TYPE_DATA1        0xEF
#define NVRAM_EVSA_ENTRY_TYPE_DATA2        0xE3
#define NVRAM_EVSA_ENTRY_TYPE_DATA_INVALID 0x83

typedef struct EVSA_ENTRY_HEADER_ {
    UINT8  Type;
    UINT8  Checksum;
    UINT16 Size;
} EVSA_ENTRY_HEADER;

typedef struct EVSA_STORE_ENTRY_ {
    EVSA_ENTRY_HEADER Header;
    UINT32 Signature; // EVSA signature
    UINT32 Attributes;
    UINT32 StoreSize;
    UINT32 : 32;
} EVSA_STORE_ENTRY;

typedef struct EVSA_GUID_ENTRY_ {
    EVSA_ENTRY_HEADER Header;
    UINT16 GuidId;
    //EFI_GUID Guid;
} EVSA_GUID_ENTRY;

typedef struct EVSA_NAME_ENTRY_ {
    EVSA_ENTRY_HEADER Header;
    UINT16 VarId;
    //CHAR16 Name[];
} EVSA_NAME_ENTRY;

typedef struct EVSA_DATA_ENTRY_ {
    EVSA_ENTRY_HEADER Header;
    UINT16 GuidId;
    UINT16 VarId;
    UINT32 Attributes;
    //UINT8 Data[];
} EVSA_DATA_ENTRY;

// VSS variable attributes
#define NVRAM_EVSA_DATA_NON_VOLATILE                          0x00000001
#define NVRAM_EVSA_DATA_BOOTSERVICE_ACCESS                    0x00000002
#define NVRAM_EVSA_DATA_RUNTIME_ACCESS                        0x00000004
#define NVRAM_EVSA_DATA_HARDWARE_ERROR_RECORD                 0x00000008
#define NVRAM_EVSA_DATA_AUTHENTICATED_WRITE_ACCESS            0x00000010
#define NVRAM_EVSA_DATA_TIME_BASED_AUTHENTICATED_WRITE_ACCESS 0x00000020
#define NVRAM_EVSA_DATA_APPEND_WRITE                          0x00000040
#define NVRAM_EVSA_DATA_EXTENDED_HEADER                       0x10000000
#define NVRAM_EVSA_DATA_UNKNOWN_MASK                          0xEFFFFF80

typedef struct EVSA_DATA_ENTRY_EXTENDED_ {
    EVSA_ENTRY_HEADER Header;
    UINT16 GuidId;
    UINT16 VarId;
    UINT32 Attributes;
    UINT32 DataSize;
    //UINT8 Data[];
} EVSA_DATA_ENTRY_EXTENDED;

extern UString evsaAttributesToUString(const UINT32 attributes);

//
// Phoenix SCT Flash Map
//

#define NVRAM_PHOENIX_FLASH_MAP_SIGNATURE_PART1  0x414C465F
#define NVRAM_PHOENIX_FLASH_MAP_SIGNATURE_LENGTH 10

// _FLASH_MAP
const UByteArray NVRAM_PHOENIX_FLASH_MAP_SIGNATURE
("\x5F\x46\x4C\x41\x53\x48\x5F\x4D\x41\x50", 10);

typedef struct PHOENIX_FLASH_MAP_HEADER_ {
    UINT8  Signature[10]; // _FLASH_MAP signature
    UINT16 NumEntries;    // Number of entries in the map
    UINT32 : 32;          // Reserved field
} PHOENIX_FLASH_MAP_HEADER;

typedef struct PHOENIX_FLASH_MAP_ENTRY_ {
    EFI_GUID Guid;
    UINT16 DataType;
    UINT16 EntryType;
    UINT64 PhysicalAddress;
    UINT32 Size;
    UINT32 Offset;
} PHOENIX_FLASH_MAP_ENTRY;

#define NVRAM_PHOENIX_FLASH_MAP_ENTRY_TYPE_VOLUME     0x0000
#define NVRAM_PHOENIX_FLASH_MAP_ENTRY_TYPE_DATA_BLOCK 0x0001

extern UString flashMapGuidToUString(const EFI_GUID & guid);

// B091E7D2-05A0-4198-94F0-74B7B8C55459
const UByteArray NVRAM_PHOENIX_FLASH_MAP_VOLUME_HEADER
("\xD2\xE7\x91\xB0\xA0\x05\x98\x41\x94\xF0\x74\xB7\xB8\xC5\x54\x59", 16);

// FD3F690E-B4B0-4D68-89DB-19A1A3318F90
const UByteArray NVRAM_PHOENIX_FLASH_MAP_MICROCODES_GUID
("\x0E\x69\x3F\xFD\xB0\xB4\x68\x4D\x89\xDB\x19\xA1\xA3\x31\x8F\x90", 16);

// 46310243-7B03-4132-BE44-2243FACA7CDD
const UByteArray NVRAM_PHOENIX_FLASH_MAP_CMDB_GUID
("\x43\x02\x31\x46\x03\x7B\x32\x41\xBE\x44\x22\x43\xFA\xCA\x7C\xDD", 16);

// 1B2C4952-D778-4B64-BDA1-15A36F5FA545
const UByteArray NVRAM_PHOENIX_FLASH_MAP_PUBKEY1_GUID
("\x52\x49\x2C\x1B\x78\xD7\x64\x4B\xBD\xA1\x15\xA3\x6F\x5F\xA5\x45", 16);

// 127C1C4E-9135-46E3-B006-F9808B0559A5
const UByteArray NVRAM_PHOENIX_FLASH_MAP_MARKER1_GUID
("\x4E\x1C\x7C\x12\x35\x91\xE3\x46\xB0\x06\xF9\x80\x8B\x05\x59\xA5", 16);

// 7CE75114-8272-45AF-B536-761BD38852CE
const UByteArray NVRAM_PHOENIX_FLASH_MAP_PUBKEY2_GUID
("\x14\x51\xE7\x7C\x72\x82\xAF\x45\xB5\x36\x76\x1B\xD3\x88\x52\xCE", 16);

// 071A3DBE-CFF4-4B73-83F0-598C13DCFDD5
const UByteArray NVRAM_PHOENIX_FLASH_MAP_MARKER2_GUID
("\xBE\x3D\x1A\x07\xF4\xCF\x73\x4B\x83\xF0\x59\x8C\x13\xDC\xFD\xD5", 16);

// FACFB110-7BFD-4EFB-873E-88B6B23B97EA
const UByteArray NVRAM_PHOENIX_FLASH_MAP_EVSA1_GUID
("\x10\xB1\xCF\xFA\xFD\x7B\xFB\x4E\x87\x3E\x88\xB6\xB2\x3B\x97\xEA", 16);

// E68DC11A-A5F4-4AC3-AA2E-29E298BFF645
const UByteArray NVRAM_PHOENIX_FLASH_MAP_EVSA2_GUID
("\x1A\xC1\x8D\xE6\xF4\xA5\xC3\x4A\xAA\x2E\x29\xE2\x98\xBF\xF6\x45", 16);

// 4B3828AE-0ACE-45B6-8CDB-DAFC28BBF8C5
const UByteArray NVRAM_PHOENIX_FLASH_MAP_EVSA3_GUID
("\xAE\x28\x38\x4B\xCE\x0A\xB6\x45\x8C\xDB\xDA\xFC\x28\xBB\xF8\xC5", 16);

// C22E6B8A-8159-49A3-B353-E84B79DF19C0
const UByteArray NVRAM_PHOENIX_FLASH_MAP_EVSA4_GUID
("\x8A\x6B\x2E\xC2\x59\x81\xA3\x49\xB3\x53\xE8\x4B\x79\xDF\x19\xC0", 16);

// B6B5FAB9-75C4-4AAE-8314-7FFFA7156EAA
const UByteArray NVRAM_PHOENIX_FLASH_MAP_EVSA5_GUID
("\xB9\xFA\xB5\xB6\xC4\x75\xAE\x4A\x83\x14\x7F\xFF\xA7\x15\x6E\xAA", 16);

// 919B9699-8DD0-4376-AA0B-0E54CCA47D8F
const UByteArray NVRAM_PHOENIX_FLASH_MAP_EVSA6_GUID
("\x99\x96\x9B\x91\xD0\x8D\x76\x43\xAA\x0B\x0E\x54\xCC\xA4\x7D\x8F", 16);

// 58A90A52-929F-44F8-AC35-A7E1AB18AC91
const UByteArray NVRAM_PHOENIX_FLASH_MAP_EVSA7_GUID
("\x52\x0A\xA9\x58\x9F\x92\xF8\x44\xAC\x35\xA7\xE1\xAB\x18\xAC\x91", 16);

// 8CB71915-531F-4AF5-82BF-A09140817BAA
const UByteArray NVRAM_PHOENIX_FLASH_MAP_SELF_GUID
("\x15\x19\xB7\x8C\x1F\x53\xF5\x4A\x82\xBF\xA0\x91\x40\x81\x7B\xAA", 16);

//
// SLIC pubkey and marker
//

typedef struct OEM_ACTIVATION_PUBKEY_ {
    UINT32 Type;         // 0
    UINT32 Size;         // 0x9C
    UINT8  KeyType;
    UINT8  Version;
    UINT16 Reserved;
    UINT32 Algorithm;
    UINT32 Magic;        // RSA1 signature
    UINT32 BitLength;
    UINT32 Exponent;
    UINT8  Modulus[128];
} OEM_ACTIVATION_PUBKEY;

#define OEM_ACTIVATION_PUBKEY_TYPE  0x00000000
#define OEM_ACTIVATION_PUBKEY_MAGIC 0x31415352 // RSA1

typedef struct OEM_ACTIVATION_MARKER_ {
    UINT32 Type;         // 1
    UINT32 Size;         // 0xB6
    UINT32 Version;
    UINT8  OemId[6];
    UINT8  OemTableId[8];
    UINT64 WindowsFlag;  // WINDOWS signature
    UINT32 SlicVersion;
    UINT8  Reserved[16];
    UINT8  Signature[128];
} OEM_ACTIVATION_MARKER;

#define OEM_ACTIVATION_MARKER_TYPE               0x00000001
#define OEM_ACTIVATION_MARKER_WINDOWS_FLAG_PART1 0x444E4957
#define OEM_ACTIVATION_MARKER_WINDOWS_FLAG       0x2053574F444E4957UL
#define OEM_ACTIVATION_MARKER_RESERVED_BYTE      0x00

//
// Phoenix CMDB, no londer used, requires no parsing
//

typedef struct PHOENIX_CMDB_HEADER_ {
    UINT32 Signature;  // CMDB signature
    UINT32 HeaderSize; // Size of this header
    UINT32 TotalSize;  // Total size of header and chunks, without strings
    // UINT8 StartChunk[3];
    // UINT8 StringChunk[5][x];
    // C_STR Strings[2*x + 1];
} PHOENIX_CMDB_HEADER;

#define NVRAM_PHOENIX_CMDB_HEADER_SIGNATURE 0x42444D43
#define NVRAM_PHOENIX_CMDB_SIZE 0x100;

// Restore previous packing rules
#pragma pack(pop)

#endif // NVRAM_H
