/* utility.h

Copyright (c) 2016, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#ifndef UTILITY_H
#define UTILITY_H

#include "basetypes.h"
#include "ustring.h"
#include "treemodel.h"
#include "parsingdata.h"

// Returns either new parsing data instance or obtains it from index
PARSING_DATA parsingDataFromUModelIndex(const UModelIndex & index);

// Converts parsing data to byte array
UByteArray parsingDataToUByteArray(const PARSING_DATA & pdata);

// Returns unique name string based for tree item
UString uniqueItemName(const UModelIndex & index);

// Converts error code to UString
UString errorCodeToUString(UINT8 errorCode);

// Decompression routine
USTATUS decompress(const UByteArray & compressed, UINT8 & algorithm, UByteArray & decompressed, UByteArray & efiDecompressed);

// Compression routine
//USTATUS compress(const UByteArray & decompressed, UByteArray & compressed, const UINT8 & algorithm);

// CRC32 calculation routine
UINT32 crc32(UINT32 initial, const UINT8* buffer, const UINT32 length);

// 8bit checksum calculation routine
UINT8 calculateChecksum8(const UINT8* buffer, UINT32 bufferSize);

// 16bit checksum calculation routine
UINT16 calculateChecksum16(const UINT16* buffer, UINT32 bufferSize);

// Return padding type from it's contents
UINT8 getPaddingType(const UByteArray & padding);

#endif // UTILITY_H
