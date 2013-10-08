/*++

Copyright (c) 2004 - 2006, Intel Corporation. All rights reserved.<BR>
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

#ifndef _EFITIANODECOMPRESS_H_
#define _EFITIANODECOMPRESS_H_
#include <string.h>
#include <stdlib.h>

#include "../basetypes.h"

#ifdef __cplusplus
extern "C" {
#endif

UINT32
EFIAPI
EfiGetInfo (
  VOID                    *Source,
  UINT32                  SrcSize,
  UINT32                  *DstSize,
  UINT32                  *ScratchSize
  )
/*++

Routine Description:

  The implementation is same as that  of EFI_DECOMPRESS_PROTOCOL.GetInfo().

Arguments:

  This        - The protocol instance pointer
  Source      - The source buffer containing the compressed data.
  SrcSize     - The size of source buffer
  DstSize     - The size of destination buffer.
  ScratchSize - The size of scratch buffer.

Returns:

  EFI_SUCCESS           - The size of destination buffer and the size of scratch buffer are successull retrieved.
  EFI_INVALID_PARAMETER - The source data is corrupted

--*/
;

UINT32
EFIAPI
EfiDecompress (
  VOID                    *Source,
  UINT32                  SrcSize,
  VOID                    *Destination,
  UINT32                  DstSize,
  VOID                    *Scratch,
  UINT32                  ScratchSize
  )
/*++

Routine Description:

  The implementation is same as that of EFI_DECOMPRESS_PROTOCOL.Decompress().

Arguments:

  This        - The protocol instance pointer
  Source      - The source buffer containing the compressed data.
  SrcSize     - The size of source buffer
  Destination - The destination buffer to store the decompressed data
  DstSize     - The size of destination buffer.
  Scratch     - The buffer used internally by the decompress routine. This  buffer is needed to store intermediate data.
  ScratchSize - The size of scratch buffer.

Returns:

  EFI_SUCCESS           - Decompression is successfull
  EFI_INVALID_PARAMETER - The source data is corrupted

--*/
;

UINT32
EFIAPI
TianoGetInfo (
  VOID                          *Source,
  UINT32                        SrcSize,
  UINT32                        *DstSize,
  UINT32                        *ScratchSize
  )
/*++

Routine Description:

  The implementation is same as that of EFI_TIANO_DECOMPRESS_PROTOCOL.GetInfo().

Arguments:

  This        - The protocol instance pointer
  Source      - The source buffer containing the compressed data.
  SrcSize     - The size of source buffer
  DstSize     - The size of destination buffer.
  ScratchSize - The size of scratch buffer.

Returns:

  EFI_SUCCESS           - The size of destination buffer and the size of scratch buffer are successull retrieved.
  EFI_INVALID_PARAMETER - The source data is corrupted

--*/
;

UINT32
EFIAPI
TianoDecompress (
  VOID                          *Source,
  UINT32                        SrcSize,
  VOID                          *Destination,
  UINT32                        DstSize,
  VOID                          *Scratch,
  UINT32                        ScratchSize
  )
/*++

Routine Description:

  The implementation is same as that  of EFI_TIANO_DECOMPRESS_PROTOCOL.Decompress().

Arguments:

  This        - The protocol instance pointer
  Source      - The source buffer containing the compressed data.
  SrcSize     - The size of source buffer
  Destination - The destination buffer to store the decompressed data
  DstSize     - The size of destination buffer.
  Scratch     - The buffer used internally by the decompress routine. This  buffer is needed to store intermediate data.
  ScratchSize - The size of scratch buffer.

Returns:

  EFI_SUCCESS           - Decompression is successfull
  EFI_INVALID_PARAMETER - The source data is corrupted

--*/
;

typedef
UINT32
(*GETINFO_FUNCTION) (
  VOID    *Source,
  UINT32  SrcSize,
  UINT32  *DstSize,
  UINT32  *ScratchSize
  );

typedef
UINT32
(*DECOMPRESS_FUNCTION) (
  VOID    *Source,
  UINT32  SrcSize,
  VOID    *Destination,
  UINT32  DstSize,
  VOID    *Scratch,
  UINT32  ScratchSize
  );

#ifdef __cplusplus
}
#endif
#endif
