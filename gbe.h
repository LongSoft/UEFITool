/* gbe.h

Copyright (c) 2014, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHWARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*/

#ifndef __GBE_H__
#define __GBE_H__

#include "basetypes.h"

// Make sure we use right packing rules
#pragma pack(push,1)

typedef struct {
    UINT8 vendor[3];
    UINT8 device[3];
} GBE_MAC;

#define GBE_VERSION_OFFSET 10

typedef struct {
    UINT8 id:    4;
    UINT8 minor: 4;
    UINT8 major;
} GBE_VERSION;

// Restore previous packing rules
#pragma pack(pop)
#endif
