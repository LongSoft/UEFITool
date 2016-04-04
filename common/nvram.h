/* nvram.h

Copyright (c) 2016, Nikolaj Schlej. All rights reserved.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#ifndef __NVRAM_H__
#define __NVRAM_H__

#include <QByteArray>
#include <QString>
#include "basetypes.h"

//
// Let's start with NVAR storage, as the most difficult one
//

// CEF5B9A3-476D-497F-9FDC-E98143E0422C
const QByteArray NVRAM_NVAR_STORAGE_FILE_GUID
("\xA3\xB9\xF5\xCE\x6D\x47\x7F\x49\x9F\xDC\xE9\x81\x43\xE0\x42\x2C", 16);

// 9221315B-30BB-46B5-813E-1B1BF4712BD3
const QByteArray NVRAM_NVAR_EXTERNAL_DEFAULTS_FILE_GUID
("\x5B\x31\x21\x92\xBB\x30\xB5\x46\x81\x3E\x1B\x1B\xF4\x71\x2B\xD3", 16);

extern QString nvarAttributesToQString(const UINT8 attributes);

extern QString efiTimeToQString(const EFI_TIME & time);

// Make sure we use right packing rules
#pragma pack(push,1)

// Variable header
typedef struct _NVAR_VARIABLE_HEADER {
    UINT32 Signature;      // NVAR signature
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
// Next format is TianoCore VSS and it's variations
//

// FFF12B8D-7696-4C8B-A985-2747075B4F50
const QByteArray NVRAM_VSS_STORAGE_VOLUME_GUID
("\x8D\x2B\xF1\xFF\x96\x76\x8B\x4C\xA9\x85\x27\x47\x07\x5B\x4F\x50", 16);

#define NVRAM_VSS_STORE_SIGNATURE            0x53535624 // $VSS
#define NVRAM_APPLE_SVS_STORE_SIGNATURE      0x53565324 // $SVS
#define NVRAM_APPLE_FSYS_STORE_SIGNATURE     0x73797346 // Fsys
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
typedef struct _VSS_VARIABLE_STORE_HEADER {
    UINT32  Signature; // $VSS signature
    UINT32  Size;      // Size of variable storage, including storage header
    UINT8   Format;    // Storage format state
    UINT8   State;     // Storage health state 
    UINT16  Unknown;   // Used in Apple $SVS varstores
    UINT32  : 32;
} VSS_VARIABLE_STORE_HEADER;

// Apple Fsys store header
typedef struct _APPLE_FSYS_STORE_HEADER {
    UINT32  Signature;  // Fsys signature
    UINT8   Unknown[5]; // Still unknown
    UINT16  Size;       // Size of variable storage
} APPLE_FSYS_STORE_HEADER;

// Apple Fsys variable format
// UINT8 NameLength;
// CHAR8 Name[];
// UINT16 DataLength;
// UINT8 Data[]
// Storage ends with a chunk named "EOF" without data
// All free bytes in storage are zeroed
// Has CRC32 of the whole store without checksum field at the end

// Normal variable header
typedef struct _VSS_VARIABLE_HEADER {
    UINT16    StartId;    // Variable start marker AA55
    UINT8     State;      // Variable state 
    UINT8     : 8;
    UINT32    Attributes; // Variable attributes
    UINT32    NameSize;   // Size of variable name, null-terminated UCS2 string
    UINT32    DataSize;   // Size of variable data without header and name
    EFI_GUID  VendorGuid; // Variable vendor GUID
} VSS_VARIABLE_HEADER;

// Apple variation of normal variable header, with one new field
typedef struct _VSS_APPLE_VARIABLE_HEADER {
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
typedef struct _VSS_AUTH_VARIABLE_HEADER {
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

// FDC region can be found in some VSS volumes
// It has another VSS volume inside
// _FDC header structure
typedef struct _FDC_VOLUME_HEADER {
    UINT32 Signature;
    UINT32 Size;
    //EFI_FIRMWARE_VOLUME_HEADER VolumeHeader;
    //EFI_FV_BLOCK_MAP_ENTRY FvBlockMap[2];
    //VSS_VARIABLE_STORE_HEADER VssHeader;
} FDC_VOLUME_HEADER;

#define NVRAM_FDC_VOLUME_SIGNATURE 0x4344465F

// Restore previous packing rules
#pragma pack(pop)

#endif
