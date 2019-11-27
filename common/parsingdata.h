/* parsingdata.h

Copyright (c) 2015, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHWARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Parsing data is an information needed for each level of image reconstruction
routines without the need of backward traversal

*/

#ifndef PARSINGDATA_H
#define PARSINGDATA_H

#include "basetypes.h"

typedef struct VOLUME_PARSING_DATA_ {
    EFI_GUID extendedHeaderGuid;
    UINT32   alignment;
    UINT32   usedSpace;
    BOOLEAN  hasValidUsedSpace;
    UINT8    ffsVersion;
    UINT8    emptyByte;
    UINT8    revision;
    BOOLEAN  hasExtendedHeader;
    BOOLEAN  hasAppleCrc32;
    BOOLEAN  isWeakAligned;
} VOLUME_PARSING_DATA;

typedef struct FILE_PARSING_DATA_ {
    UINT8    emptyByte;
    EFI_GUID guid;
} FILE_PARSING_DATA;

typedef struct GUIDED_SECTION_PARSING_DATA_ {
    EFI_GUID guid;
    UINT32   dictionarySize;
} GUIDED_SECTION_PARSING_DATA;

typedef struct FREEFORM_GUIDED_SECTION_PARSING_DATA_ {
    EFI_GUID guid;
} FREEFORM_GUIDED_SECTION_PARSING_DATA;

typedef struct COMPRESSED_SECTION_PARSING_DATA_ {
    UINT32 uncompressedSize;
    UINT8  compressionType;
    UINT8  algorithm;
    UINT32 dictionarySize;
} COMPRESSED_SECTION_PARSING_DATA;

typedef struct TE_IMAGE_SECTION_PARSING_DATA_ {
    UINT32 originalImageBase;
    UINT32 adjustedImageBase;
    UINT8  imageBaseType;
} TE_IMAGE_SECTION_PARSING_DATA;

typedef struct NVAR_ENTRY_PARSING_DATA_ {
    UINT8   emptyByte;
    BOOLEAN isValid;
    UINT32  next;
} NVAR_ENTRY_PARSING_DATA;

#endif // PARSINGDATA_H
