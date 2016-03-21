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

extern QString variableAttributesToQstring(UINT8 attributes);

extern std::vector<const CHAR8*> nestingVariableNames;

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

// Restore previous packing rules
#pragma pack(pop)

#endif
