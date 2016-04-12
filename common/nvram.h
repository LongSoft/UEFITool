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

#include <QByteArray>
#include <QString>
#include "basetypes.h"

//
// NVAR store
//

// CEF5B9A3-476D-497F-9FDC-E98143E0422C
const QByteArray NVRAM_NVAR_STORE_FILE_GUID
("\xA3\xB9\xF5\xCE\x6D\x47\x7F\x49\x9F\xDC\xE9\x81\x43\xE0\x42\x2C", 16);

// 9221315B-30BB-46B5-813E-1B1BF4712BD3
const QByteArray NVRAM_NVAR_EXTERNAL_DEFAULTS_FILE_GUID
("\x5B\x31\x21\x92\xBB\x30\xB5\x46\x81\x3E\x1B\x1B\xF4\x71\x2B\xD3", 16);

extern QString nvarAttributesToQString(const UINT8 attributes);

extern QString efiTimeToQString(const EFI_TIME & time);

// Make sure we use right packing rules
#pragma pack(push, 1)

// Variable header
typedef struct NVAR_VARIABLE_HEADER_ {
    UINT32 Signature;      // NVAR
    UINT16 Size;           // Size of the variable including header
    UINT32 Next : 24;      // Offset to the next variable in a list, or empty if latest in the list
    UINT32 Attributes : 8; // Attributes
} NVAR_VARIABLE_HEADER;

// NVAR signature
#define NVRAM_NVAR_VARIABLE_SIGNATURE         0x5241564E

// Attributes
#define NVRAM_NVAR_VARIABLE_ATTRIB_RUNTIME         0x01
#define NVRAM_NVAR_VARIABLE_ATTRIB_ASCII_NAME      0x02
#define NVRAM_NVAR_VARIABLE_ATTRIB_GUID            0x04
#define NVRAM_NVAR_VARIABLE_ATTRIB_DATA_ONLY       0x08
#define NVRAM_NVAR_VARIABLE_ATTRIB_EXT_HEADER      0x10
#define NVRAM_NVAR_VARIABLE_ATTRIB_HW_ERROR_RECORD 0x20 
#define NVRAM_NVAR_VARIABLE_ATTRIB_AUTH_WRITE      0x40
#define NVRAM_NVAR_VARIABLE_ATTRIB_VALID           0x80

// Extended attributes
#define NVRAM_NVAR_VARIABLE_EXT_ATTRIB_CHECKSUM    0x01
#define NVRAM_NVAR_VARIABLE_EXT_ATTRIB_AUTH_WRITE  0x10
#define NVRAM_NVAR_VARIABLE_EXT_ATTRIB_TIME_BASED  0x20



//
// TianoCore VSS and it's variations
//

// FFF12B8D-7696-4C8B-A985-2747075B4F50
const QByteArray NVRAM_MAIN_STORE_VOLUME_GUID
("\x8D\x2B\xF1\xFF\x96\x76\x8B\x4C\xA9\x85\x27\x47\x07\x5B\x4F\x50", 16);

// 00504624-8A59-4EEB-BD0F-6B36E96128E0
const QByteArray NVRAM_ADDITIONAL_STORE_VOLUME_GUID
("\x24\x46\x50\x00\x59\x8A\xEB\x4E\xBD\x0F\x6B\x36\xE9\x61\x28\xE0", 16);

#define NVRAM_VSS_STORE_SIGNATURE            0x53535624 // $VSS
#define NVRAM_APPLE_SVS_STORE_SIGNATURE      0x53565324 // $SVS
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
    UINT8     : 8;
    UINT32    Attributes; // Variable attributes
    UINT32    NameSize;   // Size of variable name, null-terminated UCS2 string
    UINT32    DataSize;   // Size of variable data without header and name
    EFI_GUID  VendorGuid; // Variable vendor GUID
} VSS_VARIABLE_HEADER;

// Apple variation of normal variable header, with one new field
typedef struct VSS_APPLE_VARIABLE_HEADER_ {
    UINT16    StartId;    // Variable start marker AA55
    UINT8     State;      // Variable state 
    UINT8     : 8;
    UINT32    Attributes; // Variable attributes
    UINT32    NameSize;   // Size of variable name, null-terminated UCS2 string
    UINT32    DataSize;   // Size of variable data without header and name
    EFI_GUID  VendorGuid; // Variable vendor GUID
    UINT32    DataCrc32;  // CRC32 of the data
} VSS_APPLE_VARIABLE_HEADER;

// Authenticated variable header, used for SecureBoot vars
typedef struct VSS_AUTH_VARIABLE_HEADER_ {
    UINT16    StartId;          // Variable start marker AA55
    UINT8     State;            // Variable state 
    UINT8     : 8;
    UINT32    Attributes;       // Variable attributes
    UINT64    MonotonicCounter; // Monotonic counter against replay attack
    EFI_TIME  Timestamp;        // Time stamp against replay attack
    UINT32    PubKeyIndex;      // Index in PubKey database
    UINT32    NameSize;         // Size of variable name, null-terminated UCS2 string
    UINT32    DataSize;         // Size of variable data without header and name
    EFI_GUID  VendorGuid;       // Variable vendor GUID
} VSS_AUTH_VARIABLE_HEADER;

// VSS variable states
#define NVRAM_VSS_VARIABLE_IN_DELETED_TRANSITION     0xfe  // Variable is in obsolete transistion
#define NVRAM_VSS_VARIABLE_DELETED                   0xfd  // Variable is obsolete
#define NVRAM_VSS_VARIABLE_HEADER_VALID              0x7f  // Variable has valid header
#define NVRAM_VSS_VARIABLE_ADDED                     0x3f  // Variable has been completely added
#define NVRAM_VSS_IS_VARIABLE_STATE(_c, _Mask)  (BOOLEAN) (((~_c) & (~_Mask)) != 0)

// VSS variable attributes
#define NVRAM_VSS_VARIABLE_NON_VOLATILE                          0x00000001
#define NVRAM_VSS_VARIABLE_BOOTSERVICE_ACCESS                    0x00000002
#define NVRAM_VSS_VARIABLE_RUNTIME_ACCESS                        0x00000004
#define NVRAM_VSS_VARIABLE_HARDWARE_ERROR_RECORD                 0x00000008
#define NVRAM_VSS_VARIABLE_AUTHENTICATED_WRITE_ACCESS            0x00000010
#define NVRAM_VSS_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS 0x00000020
#define NVRAM_VSS_VARIABLE_APPEND_WRITE                          0x00000040
#define NVRAM_VSS_VARIABLE_APPLE_DATA_CHECKSUM                   0x80000000

// FDC region can be found in Insyde VSS volumes
// It has another VSS volume inside
// _FDC header structure
#define NVRAM_FDC_VOLUME_SIGNATURE 0x4344465F

typedef struct FDC_VOLUME_HEADER_ {
    UINT32 Signature; //_FDC
    UINT32 Size;      // Size of the whole region
    //EFI_FIRMWARE_VOLUME_HEADER VolumeHeader;
    //EFI_FV_BLOCK_MAP_ENTRY FvBlockMap[2];
    //VSS_VARIABLE_STORE_HEADER VssHeader;
} FDC_VOLUME_HEADER;

// FTW block
// EFI Fault tolerant working block header
#define EFI_FAULT_TOLERANT_WORKING_BLOCK_VALID   0x1
#define EFI_FAULT_TOLERANT_WORKING_BLOCK_INVALID 0x2

// 9E58292B-7C68-497D-0ACE6500FD9F1B95
const QByteArray EDKII_WORKING_BLOCK_SIGNATURE_GUID
("\x2B\x29\x58\x9E\x68\x7C\x7D\x49\x0A\xCE\x65\x00\xFD\x9F\x1B\x95", 16);

#define NVRAM_MAIN_STORE_VOLUME_GUID_DATA1   0xFFF12B8D
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
    EFI_GUID  Signature; // NVRAM_MAIN_STORE_VOLUME_GUID
    UINT32    Crc; // Crc32 of the header with empty Crc and State fields
    UINT8     State;
    UINT8     Reserved[3];
    UINT64    WriteQueueSize; // Size of the FTW block without the header
    //UINT8   WriteQueue[WriteQueueSize];
} EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER64;

//
// Apple Fsys
//

typedef struct APPLE_FSYS_STORE_HEADER_ {
    UINT32  Signature;  // Fsys signature
    UINT8   Unknown[5]; // Still unknown
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
// EVSA
//

#define NVRAM_EVSA_STORE_SIGNATURE 0x41535645

#define NVRAM_EVSA_ENTRY_TYPE_STORE 0xEC
#define NVRAM_EVSA_ENTRY_TYPE_GUID1 0xED
#define NVRAM_EVSA_ENTRY_TYPE_GUID2 0xE1
#define NVRAM_EVSA_ENTRY_TYPE_NAME1 0xEE
#define NVRAM_EVSA_ENTRY_TYPE_NAME2 0xE2
#define NVRAM_EVSA_ENTRY_TYPE_DATA1 0xEF
#define NVRAM_EVSA_ENTRY_TYPE_DATA2 0xE3
#define NVRAM_EVSA_ENTRY_TYPE_DATA3 0x83

typedef struct EVSA_ENTRY_HEADER_ {
    UINT8  Type;
    UINT8  Checksum;
    UINT16 Size;
} EVSA_ENTRY_HEADER;

typedef struct EVSA_STORE_ENTRY_ {
    EVSA_ENTRY_HEADER Header;
    UINT32 Signature; // EVSA
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


//
// Phoenix SCT Flash Map
//

#define NVRAM_PHOENIX_FLASH_MAP_SIGNATURE_PART1 0x414C465F
#define NVRAM_PHOENIX_FLASH_MAP_SIGNATURE_LENGTH 10

// _FLASH_MAP
const QByteArray NVRAM_PHOENIX_FLASH_MAP_SIGNATURE
("\x5F\x46\x4C\x41\x53\x48\x5F\x4D\x41\x50", 10);

typedef struct PHOENIX_FLASH_MAP_HEADER_ {
    UINT8  Signature[10]; // _FLASH_MAP signature
    UINT16 NumEntries;    // Number of entries in the map
    UINT32 : 32;          // Reserved field
} PHOENIX_FLASH_MAP_HEADER;

typedef struct PHOENIX_FLASH_MAP_ENTRY_ {
    EFI_GUID Guid;
    UINT32 Type;
    UINT64 PhysicalAddress;
    UINT32 Size;
    UINT32 Offset;
} PHOENIX_FLASH_MAP_ENTRY;





// Restore previous packing rules
#pragma pack(pop)

#endif // NVRAM_H
