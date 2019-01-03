/* LZMA Compress Header

  Copyright (c) 2014, Nikolaj Schlej. All rights reserved.
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHWARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  */

#ifndef LZMACOMPRESS_H
#define LZMACOMPRESS_H

#include "SDK/C/Types.h"
#include "../basetypes.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DEFAULT_LZMA_DICTIONARY_SIZE 0x800000
#define _LZMA_SIZE_OPT

    USTATUS
        EFIAPI
        LzmaCompress (
            const UINT8  *Source,
            UINT32       SourceSize,
            UINT8        *Destination,
            UINT32       *DestinationSize,
            UINT32        DictionarySize
        );

#ifdef __cplusplus
}
#endif

#endif // LZMACOMPRESS_H
