/* gbe.h

Copyright (c) 2015, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHWARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*/

#ifndef GBE_H
#define GBE_H

#include "basetypes.h"

// Make sure we use right packing rules
#pragma pack(push, 1)

typedef struct GBE_MAC_ADDRESS_ {
    UINT8 vendor[3];
    UINT8 device[3];
} GBE_MAC_ADDRESS;

#define GBE_VERSION_OFFSET 10

typedef struct GBE_VERSION_ {
    UINT8 id    : 4;
    UINT8 minor : 4;
    UINT8 major;
} GBE_VERSION;

// Restore previous packing rules
#pragma pack(pop)

#endif // GBE_H
