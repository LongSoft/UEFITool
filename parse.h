/* parse.h

Copyright (c) 2013, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHWARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*/

#ifndef __PARSE_H__
#define __PARSE_H__

#include <stdio.h>
#include <stdlib.h>
#include "basetypes.h"
#include "ffs.h"
#include "descriptor.h"
//#include "Tiano/TianoCompress.h"
//#include "Tiano/TianoDecompress.h"
//#include "LZMA/LzmaCompress.h"
//#include "LZMA/LzmaDecompress.h"

UINT8 parse_buffer(UINT8* buffer, size_t size);

#endif