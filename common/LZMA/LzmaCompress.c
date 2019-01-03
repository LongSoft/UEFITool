/* LZMA Compress Implementation

Copyright (c) 2012, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHWARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#include "LzmaCompress.h"
#include "SDK/C/7zVersion.h"
#include "SDK/C/LzmaEnc.h"

#include <stdlib.h>

#define LZMA_HEADER_SIZE (LZMA_PROPS_SIZE + 8)

static void * AllocForLzma(void *p, size_t size) { (void)p; return malloc(size); }
static void FreeForLzma(void *p, void *address) { (void)p; free(address); }
static ISzAlloc SzAllocForLzma = { &AllocForLzma, &FreeForLzma };

SRes OnProgress(void *p, UInt64 inSize, UInt64 outSize)
{
    (void)p; (void)inSize; (void)outSize;
    return SZ_OK;
}

static ICompressProgress g_ProgressCallback = { &OnProgress };

STATIC
UINT64
EFIAPI
RShiftU64 (
    UINT64 Operand,
    UINT32 Count
)
{
    return Operand >> Count;
}

VOID
SetEncodedSizeOfBuf(
    UINT64 EncodedSize,
    UINT8* EncodedData
)
{
    INT32   Index;

    EncodedData[LZMA_PROPS_SIZE] = EncodedSize & 0xFF;
    for (Index = LZMA_PROPS_SIZE + 1; Index <= LZMA_PROPS_SIZE + 7; Index++)
    {
        EncodedSize = RShiftU64(EncodedSize, 8);
        EncodedData[Index] = EncodedSize & 0xFF;
    }
}

USTATUS
EFIAPI
LzmaCompress (
    CONST UINT8  *Source,
    UINT32       SourceSize,
    UINT8        *Destination,
    UINT32       *DestinationSize,
    UINT32       DictionarySize
)
{
    SRes              LzmaResult;
    CLzmaEncProps     props;
    SizeT propsSize = LZMA_PROPS_SIZE;
    SizeT destLen = SourceSize + SourceSize / 3 + 128;

    if (*DestinationSize < (UINT32)destLen)
    {
        *DestinationSize = (UINT32)destLen;
        return EFI_BUFFER_TOO_SMALL;
    }

    LzmaEncProps_Init(&props);
    props.dictSize = DictionarySize;
    props.level = 9;
    props.fb = 273;

    LzmaResult = LzmaEncode(
        (Byte*)((UINT8*)Destination + LZMA_HEADER_SIZE),
        &destLen,
        Source,
        (SizeT)SourceSize,
        &props,
        (UINT8*)Destination,
        &propsSize,
        props.writeEndMark,
        &g_ProgressCallback,
        &SzAllocForLzma,
        &SzAllocForLzma);

    *DestinationSize = (UINT32)(destLen + LZMA_HEADER_SIZE);

    SetEncodedSizeOfBuf(SourceSize, Destination);

    if (LzmaResult == SZ_OK) {
        return EFI_SUCCESS;
    }
    else {
        return EFI_INVALID_PARAMETER;
    }
}
