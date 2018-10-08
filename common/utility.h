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

#include <vector>

#include "basetypes.h"
#include "ustring.h"
#include "treemodel.h"
#include "parsingdata.h"

// Returns unique name for tree item
UString uniqueItemName(const UModelIndex & index);

// Converts error code to UString
UString errorCodeToUString(USTATUS errorCode);

// Decompression routine
USTATUS decompress(const UByteArray & compressed, const UINT8 compressionType, UINT8 & algorithm, UByteArray & decompressed, UByteArray & efiDecompressed);

// Compression routine
//USTATUS compress(const UByteArray & decompressed, UByteArray & compressed, const UINT8 & algorithm);

// CRC32 calculation routine
UINT32 crc32(UINT32 initial, const UINT8* buffer, const UINT32 length);

// 8bit sum calculation routine
UINT8 calculateSum8(const UINT8* buffer, UINT32 bufferSize);

// 8bit checksum calculation routine
UINT8 calculateChecksum8(const UINT8* buffer, UINT32 bufferSize);

// 16bit checksum calculation routine
UINT16 calculateChecksum16(const UINT16* buffer, UINT32 bufferSize);

// Return padding type from it's contents
UINT8 getPaddingType(const UByteArray & padding);

// Make pattern from a hexstring with an assumption of . being any char
BOOLEAN makePattern(const CHAR8 *textPattern, std::vector<UINT8> &pattern, std::vector<UINT8> &patternMask);

// Find pattern in a binary blob
INTN findPattern(const UINT8 *pattern, const UINT8 *patternMask, UINTN patternSize,
    const UINT8 *data, UINTN dataSize, UINTN dataOff);

// Safely dereferences misaligned pointers
template <typename T>
inline T readMisaligned(const T *v) {
	T tmp;
	memcpy(&tmp, v, sizeof(T));
	return tmp;
}

#endif // UTILITY_H
