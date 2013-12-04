/* LZMA Decompress Implementation

Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHWARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#include "LzmaDecompress.h"
#include "SDK/C/Types.h"
#include "SDK/C/7zVersion.h"

#include <stdlib.h>

UINT64
	EFIAPI
	LShiftU64 (
	UINT64                    Operand,
	UINT32                     Count
	)
{
	return Operand << Count;
}

static void * AllocForLzma(void *p, size_t size) { return malloc(size); }
static void FreeForLzma(void *p, void *address) { free(address); }
static ISzAlloc SzAllocForLzma = { &AllocForLzma, &FreeForLzma };

/*
Get the size of the uncompressed buffer by parsing EncodeData header.

@param EncodedData  Pointer to the compressed data.

@return The size of the uncompressed buffer.
*/
UINT64
	GetDecodedSizeOfBuf(
	UINT8 *EncodedData
	)
{
	UINT64 DecodedSize;
	INT32   Index;

	// Parse header 
	DecodedSize = 0;
	for (Index = LZMA_PROPS_SIZE + 7; Index >= LZMA_PROPS_SIZE; Index--)
		DecodedSize = LShiftU64(DecodedSize, 8) + EncodedData[Index];

	return DecodedSize;
}

//
// LZMA functions and data as defined local LzmaDecompressLibInternal.h
//

/*
Given a Lzma compressed source buffer, this function retrieves the size of 
the uncompressed buffer and the size of the scratch buffer required 
to decompress the compressed source buffer.

Retrieves the size of the uncompressed buffer and the temporary scratch buffer 
required to decompress the buffer specified by Source and SourceSize.
The size of the uncompressed buffer is returned DestinationSize, 
the size of the scratch buffer is returned ScratchSize, and RETURN_SUCCESS is returned.
This function does not have scratch buffer available to perform a thorough 
checking of the validity of the source data. It just retrieves the "Original Size"
field from the LZMA_HEADER_SIZE beginning bytes of the source data and output it as DestinationSize.
And ScratchSize is specific to the decompression implementation.

If SourceSize is less than LZMA_HEADER_SIZE, then ASSERT().

@param  Source          The source buffer containing the compressed data.
@param  SourceSize      The size, bytes, of the source buffer.
@param  DestinationSize A pointer to the size, bytes, of the uncompressed buffer
that will be generated when the compressed buffer specified
by Source and SourceSize is decompressed.

@retval  EFI_SUCCESS The size of the uncompressed data was returned 
DestinationSize and the size of the scratch 
buffer was returned ScratchSize.

*/
INT32
	EFIAPI
	LzmaGetInfo (
	CONST VOID  *Source,
	UINT32      SourceSize,
	UINT32      *DestinationSize
	)
{
	UInt64  DecodedSize;

	ASSERT(SourceSize >= LZMA_HEADER_SIZE);

	DecodedSize = GetDecodedSizeOfBuf((UINT8*)Source);

	*DestinationSize = (UINT32)DecodedSize;
	return ERR_SUCCESS;
}

/*
Decompresses a Lzma compressed source buffer.

Extracts decompressed data to its original form.
If the compressed source data specified by Source is successfully decompressed 
into Destination, then RETURN_SUCCESS is returned.  If the compressed source data 
specified by Source is not a valid compressed data format,
then RETURN_INVALID_PARAMETER is returned.

@param  Source      The source buffer containing the compressed data.
@param  SourceSize  The size of source buffer.
@param  Destination The destination buffer to store the decompressed data

@retval  EFI_SUCCESS Decompression completed successfully, and 
the uncompressed buffer is returned Destination.
@retval  EFI_INVALID_PARAMETER 
The source buffer specified by Source is corrupted 
(not a valid compressed format).
*/
INT32
	EFIAPI
	LzmaDecompress (
	CONST VOID  *Source,
	UINT32       SourceSize,
	VOID    *Destination
	)
{
	SRes              LzmaResult;
	ELzmaStatus       Status;
	SizeT             DecodedBufSize;
	SizeT             EncodedDataSize;

	DecodedBufSize = (SizeT)GetDecodedSizeOfBuf((UINT8*)Source);
	EncodedDataSize = (SizeT) (SourceSize - LZMA_HEADER_SIZE);

	LzmaResult = LzmaDecode(
		(Byte*) Destination,
		&DecodedBufSize,
		(Byte*)((UINT8*)Source + LZMA_HEADER_SIZE),
		&EncodedDataSize,
		(CONST Byte*) Source,
		LZMA_PROPS_SIZE,
		LZMA_FINISH_END,
		&Status,
		&SzAllocForLzma
		);

	if (LzmaResult == SZ_OK) {
		return ERR_SUCCESS;
	} else {
		return ERR_INVALID_PARAMETER;
	}
}

