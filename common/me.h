/* me.h

Copyright (c) 2015, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHWARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*/

#ifndef ME_H
#define ME_H

#include "basetypes.h"

// Make sure we use right packing rules
#pragma pack(push, 1)

const UByteArray ME_VERSION_SIGNATURE("\x24\x4D\x41\x4E", 4);  //$MAN
const UByteArray ME_VERSION_SIGNATURE2("\x24\x4D\x4E\x32", 4); //$MN2

typedef struct ME_VERSION_ {
    UINT32 Signature;
    UINT32 Reserved;
    UINT16 Major;
    UINT16 Minor;
    UINT16 Bugfix;
    UINT16 Build;
} ME_VERSION;

// Restore previous packing rules
#pragma pack(pop)

#endif // ME_H
