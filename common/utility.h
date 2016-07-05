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

#include "ustring.h"
#include "basetypes.h"
#include "parsingdata.h"

// Returns either new parsing data instance or obtains it from index
PARSING_DATA parsingDataFromUModelIndex(const UModelIndex & index);

// Converts parsing data to byte array
UByteArray parsingDataToUByteArray(const PARSING_DATA & pdata);

// Converts error code to UString
extern UString errorCodeToUString(UINT8 errorCode);

// Decompression routine
extern USTATUS decompress(const UByteArray & compressed, UINT8 & algorithm, UByteArray & decompressed, UByteArray & efiDecompressed);

// Compression routine
//USTATUS compress(const UByteArray & decompressed, UByteArray & compressed, const UINT8 & algorithm);

// CRC32 calculation routine
extern UINT32 crc32(UINT32 initial, const UINT8* buffer, const UINT32 length);

// 8bit checksum calculation routine
extern UINT8 calculateChecksum8(const UINT8* buffer, UINT32 bufferSize);

// 16bit checksum calculation routine
extern UINT16 calculateChecksum16(const UINT16* buffer, UINT32 bufferSize);

#endif // UTILITY_H
