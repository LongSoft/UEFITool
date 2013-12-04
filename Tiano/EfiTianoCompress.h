/* EFI/Tiano Compress Header

Copyright (c) 2004 - 2008, Intel Corporation. All rights reserved.
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
																						  
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHWARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  TianoCompress.h

Abstract:

  Header file for compression routine.
  
*/

#ifndef _EFITIANOCOMPRESS_H_
#define _EFITIANOCOMPRESS_H_

#include <string.h>
#include <stdlib.h>

#include "../basetypes.h"

#ifdef __cplusplus
extern "C" {
#endif

/*++

Routine Description:

  Tiano compression routine.

Arguments:

  SrcBuffer   - The buffer storing the source data
  SrcSize     - The size of source data
  DstBuffer   - The buffer to store the compressed data
  DstSize     - On input, the size of DstBuffer; On output,
				the size of the actual compressed data.

Returns:

  EFI_BUFFER_TOO_SMALL  - The DstBuffer is too small. this case,
						  DstSize contains the size needed.
  EFI_SUCCESS           - Compression is successful.
  EFI_OUT_OF_RESOURCES  - No resource to complete function.
  EFI_INVALID_PARAMETER - Parameter supplied is wrong.

--*/
INT32
TianoCompress (
  UINT8   *SrcBuffer,
  UINT32  SrcSize,
  UINT8   *DstBuffer,
  UINT32  *DstSize
  )
;

/*++

Routine Description:

  EFI 1.1 compression routine.

Arguments:

  SrcBuffer   - The buffer storing the source data
  SrcSize     - The size of source data
  DstBuffer   - The buffer to store the compressed data
  DstSize     - On input, the size of DstBuffer; On output,
				the size of the actual compressed data.

Returns:

  EFI_BUFFER_TOO_SMALL  - The DstBuffer is too small. this case,
						  DstSize contains the size needed.
  EFI_SUCCESS           - Compression is successful.
  EFI_OUT_OF_RESOURCES  - No resource to complete function.
  EFI_INVALID_PARAMETER - Parameter supplied is wrong.

--*/
INT32
EfiCompress (
  UINT8   *SrcBuffer,
  UINT32  SrcSize,
  UINT8   *DstBuffer,
  UINT32  *DstSize
  )
;

#ifdef __cplusplus
}
#endif
#endif
