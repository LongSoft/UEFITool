/** @file

Copyright (c) 1999 - 2014, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  FirmwareVolumeBufferLib.h
  
Abstract:

  EFI Firmware Volume routines which work on a Fv image in buffers.

**/

#ifndef FirmwareVolumeBuffer_h_INCLUDED
#define FirmwareVolumeBuffer_h_INCLUDED

#include "Common/UefiBaseTypes.h"
#include "Common/PiFirmwareFile.h"
#include "Common/PiFirmwareVolume.h"

EFI_STATUS
FvBufAddFile (
  IN OUT VOID *Fv,
  IN VOID *File
  );

EFI_STATUS
FvBufAddFileWithExtend (
  IN OUT VOID **Fv,
  IN VOID *File
  );

EFI_STATUS
FvBufAddVtfFile (
  IN OUT VOID *Fv,
  IN VOID *File
  );

EFI_STATUS
FvBufChecksumFile (
  IN OUT VOID *FfsFile
  );

EFI_STATUS
FvBufChecksumHeader (
  IN OUT VOID *Fv
  );

EFI_STATUS
FvBufClearAllFiles (
  IN OUT VOID *Fv
  );

VOID
FvBufCompact3ByteSize (
  OUT VOID* SizeDest,
  IN UINT32 Size
  );

EFI_STATUS
FvBufCountSections (
  IN VOID* FfsFile,
  IN UINTN* Count
  );

EFI_STATUS
FvBufDuplicate (
  IN VOID *SourceFv,
  IN OUT VOID **DestinationFv
  );

UINT32
FvBufExpand3ByteSize (
  IN VOID* Size
  );

UINT32
FvBufGetFfsFileSize (
  IN EFI_FFS_FILE_HEADER *Ffs
  );

UINT32
FvBufGetFfsHeaderSize (
  IN EFI_FFS_FILE_HEADER *Ffs
  );

EFI_STATUS
FvBufExtend (
  IN VOID **Fv,
  IN UINTN Size
  );

EFI_STATUS
FvBufFindFileByName (
  IN VOID *Fv,
  IN EFI_GUID *Name,
  OUT VOID **File
  );

EFI_STATUS
FvBufFindFileByType (
  IN VOID *Fv,
  IN EFI_FV_FILETYPE Type,
  OUT VOID **File
  );

EFI_STATUS
FvBufFindNextFile (
  IN VOID *Fv,
  IN OUT UINTN *Key,
  OUT VOID **File
  );

EFI_STATUS
FvBufFindNextSection (
  IN VOID *SectionsStart,
  IN UINTN TotalSectionsSize,
  IN OUT UINTN *Key,
  OUT VOID **Section
  );

EFI_STATUS
FvBufFindSectionByType (
  IN VOID *FfsFile,
  IN UINT8 Type,
  OUT VOID **Section
  );

EFI_STATUS
FvBufGetFileRawData (
  IN  VOID*     FfsFile,
  OUT VOID**    RawData,
  OUT UINTN*    RawDataSize
  );

EFI_STATUS
FvBufGetSize (
  IN VOID *Fv,
  OUT UINTN *Size
  );

EFI_STATUS
FvBufPackageFreeformRawFile (
  IN EFI_GUID*  Filename,
  IN VOID*      RawData,
  IN UINTN      RawDataSize,
  OUT VOID**    FfsFile
  );

EFI_STATUS
FvBufRemoveFile (
  IN OUT VOID *Fv,
  IN EFI_GUID *Name
  );

EFI_STATUS
FvBufUnifyBlockSizes (
  IN OUT VOID *Fv,
  IN UINTN BlockSize
  );

EFI_STATUS
FvBufShrinkWrap (
  IN VOID *Fv
  );

#endif // #ifndef FirmwareVolumeBuffer_h_INCLUDED

