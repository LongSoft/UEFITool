/* descriptor.cpp

Copyright (c) 2013, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHWARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*/

#include <QObject>
#include "descriptor.h"

// Calculate address of data structure addressed by descriptor address format
// 8 bit base or limit
UINT8* calculateAddress8(UINT8* baseAddress, const UINT8 baseOrLimit)
{
    return baseAddress + baseOrLimit * 0x10;
}

// 16 bit base or limit
UINT8* calculateAddress16(UINT8* baseAddress, const UINT16 baseOrLimit)
{
    return baseAddress + baseOrLimit * 0x1000;
}

// Calculate offset of region using its base
UINT32 calculateRegionOffset(const UINT16 base)
{
    return base * 0x1000;
}

//Calculate size of region using its base and limit
UINT32 calculateRegionSize(const UINT16 base, const UINT16 limit)
{
    if (limit)
        return (limit + 1 - base) * 0x1000;
    return 0;
}
