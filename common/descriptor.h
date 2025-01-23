/* descriptor.h

Copyright (c) 2016, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHWARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*/

#ifndef DESCRIPTOR_H
#define DESCRIPTOR_H

#include "basetypes.h"
#include "ustring.h"

// Calculate address of data structure addressed by descriptor address format
// 8 bit base or limit
extern const UINT8* calculateAddress8(const UINT8* baseAddress, const UINT8 baseOrLimit);
// 16 bit base or limit
extern const UINT8* calculateAddress16(const UINT8* baseAddress, const UINT16 baseOrLimit);

// Calculate offset of region using it's base
extern UINT32 calculateRegionOffset(const UINT16 base);
// Calculate size of region using it's base and limit
extern UINT32 calculateRegionSize(const UINT16 base, const UINT16 limit);

// Return human-readable chip name for given JEDEC ID
extern UString jedecIdToUString(UINT8 vendorId, UINT8 deviceId0, UINT8 deviceId1);

#endif // DESCRIPTOR_H
