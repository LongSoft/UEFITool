/* EfiTianoDecompress.h

Copyright (c) 2015, Nikolaj Schlej. All rights reserved.<BR>
Copyright (c) 2004 - 2008, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

Decompress.h

Abstract:

Header file for decompression routine.
Providing both EFI and Tiano decompress algorithms.

--*/

#ifndef EFITIANODECOMPRESS_H
#define EFITIANODECOMPRESS_H
#include <string.h>
#include <stdlib.h>

#include "../basetypes.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct EFI_TIANO_HEADER_ {
    UINT32 CompSize;
    UINT32 OrigSize;
} EFI_TIANO_HEADER;

/*++

Routine Description:

The implementation is same as that of EFI_DECOMPRESS_PROTOCOL.GetInfo().

Arguments:

Source      - The source buffer containing the compressed data.
SrcSize     - The size of source buffer
DstSize     - The size of destination buffer.
ScratchSize - The size of scratch buffer.

Returns:

EFI_SUCCESS           - The size of destination buffer and the size of scratch buffer are successfully retrieved.
EFI_INVALID_PARAMETER - The source data is corrupted

--*/
EFI_STATUS
EFIAPI
EfiTianoGetInfo (
    IN      CONST VOID *Source,
    IN      UINT32      SrcSize,
    OUT     UINT32      *DstSize,
    OUT     UINT32      *ScratchSize
    );


/*++

Routine Description:

The implementation is same as that of EFI_DECOMPRESS_PROTOCOL.Decompress().

Arguments:

Source      - The source buffer containing the compressed data.
SrcSize     - The size of source buffer
Destination - The destination buffer to store the decompressed data
DstSize     - The size of destination buffer.
Scratch     - The buffer used internally by the decompress routine. This  buffer is needed to store intermediate data.
ScratchSize - The size of scratch buffer.

Returns:

EFI_SUCCESS           - Decompression is successful
EFI_INVALID_PARAMETER - The source data is corrupted

--*/
EFI_STATUS
EFIAPI
EfiDecompress(
    IN      CONST VOID *Source,
    IN      UINT32     SrcSize,
    IN OUT  VOID       *Destination,
    IN      UINT32     DstSize,
    IN OUT  VOID       *Scratch,
    IN      UINT32     ScratchSize
);

/*++

Routine Description:

The implementation is same as that  of EFI_TIANO_DECOMPRESS_PROTOCOL.Decompress().

Arguments:

Source      - The source buffer containing the compressed data.
SrcSize     - The size of source buffer
Destination - The destination buffer to store the decompressed data
DstSize     - The size of destination buffer.
Scratch     - The buffer used internally by the decompress routine. This  buffer is needed to store intermediate data.
ScratchSize - The size of scratch buffer.

Returns:

EFI_SUCCESS           - Decompression is successful
EFI_INVALID_PARAMETER - The source data is corrupted

--*/
EFI_STATUS
EFIAPI
TianoDecompress(
    IN      CONST VOID *Source,
    IN      UINT32     SrcSize,
    IN OUT  VOID       *Destination,
    IN      UINT32     DstSize,
    IN OUT  VOID       *Scratch,
    IN      UINT32     ScratchSize
    );

#ifdef __cplusplus
}
#endif
#endif // EFITIANODECOMPRESS_H
