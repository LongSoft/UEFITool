/* me.h

Copyright (c) 2014, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHWARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*/

#ifndef __ME_H__
#define __ME_H__

#include "basetypes.h"

// Make sure we use right packing rules
#pragma pack(push,1)

const QByteArray ME_VERSION_SIGNATURE("\x24\x4D\x4E\x32", 4);

typedef struct {
    UINT32 signature;
    UINT32 reserved; // Unknown for me
    UINT16 major;
    UINT16 minor;
    UINT16 bugfix;
    UINT16 build;
} ME_VERSION;

// Restore previous packing rules
#pragma pack(pop)
#endif
