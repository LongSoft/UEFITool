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

//typedef struct CAPSULE_PARSING_DATA_ {
//} CAPSULE_PARSING_DATA;

//typedef struct IMAGE_PARSING_DATA_ {
//} IMAGE_PARSING_DATA;

//typedef struct PADDING_PARSING_DATA_ {
//} PADDING_PARSING_DATA;

typedef struct VOLUME_PARSING_DATA_ {
    EFI_GUID extendedHeaderGuid;
    UINT32   alignment;
    UINT8    revision;
    BOOLEAN  hasExtendedHeader;
    BOOLEAN  hasAppleCrc32;
    BOOLEAN  hasAppleFSO;
    BOOLEAN  isWeakAligned;
} VOLUME_PARSING_DATA;

//typedef struct FREE_SPACE_PARSING_DATA_ {
//} FREE_SPACE_PARSING_DATA;

typedef struct FILE_PARSING_DATA_ {
    union {
        UINT8   tailArray[2];
        UINT16  tail;
    };
    BOOLEAN hasTail;
    UINT8 format;
} FILE_PARSING_DATA;

#define RAW_FILE_FORMAT_UNKNOWN      0
#define RAW_FILE_FORMAT_NVAR_STORE   1

typedef struct COMPRESSED_SECTION_PARSING_DATA_ {
    UINT32 uncompressedSize;
    UINT8  compressionType;
    UINT8  algorithm;
} COMPRESSED_SECTION_PARSING_DATA;

typedef struct GUIDED_SECTION_PARSING_DATA_ {
    EFI_GUID guid;
} GUIDED_SECTION_PARSING_DATA;

typedef struct FREEFORM_GUIDED_SECTION_PARSING_DATA_ {
    EFI_GUID guid;
} FREEFORM_GUIDED_SECTION_PARSING_DATA;

typedef struct TE_IMAGE_SECTION_PARSING_DATA_ {
    UINT32  imageBase;
    UINT32  adjustedImageBase;
    UINT8   revision;
} TE_IMAGE_SECTION_PARSING_DATA;

typedef struct SECTION_PARSING_DATA_ {
    union {
        COMPRESSED_SECTION_PARSING_DATA      compressed;
        GUIDED_SECTION_PARSING_DATA          guidDefined;
        FREEFORM_GUIDED_SECTION_PARSING_DATA freeformSubtypeGuid;
        TE_IMAGE_SECTION_PARSING_DATA        teImage;
    };
} SECTION_PARSING_DATA;

typedef struct NVAR_ENTRY_PARSING_DATA_ {
    BOOLEAN isValid;
    UINT32 next;
} NVAR_ENTRY_PARSING_DATA;

typedef struct PARSING_DATA_ {
    UINT8   emptyByte;
    UINT8   ffsVersion;
    UINT32  offset;
    UINT32  address;
    union {
        VOLUME_PARSING_DATA       volume;
        FILE_PARSING_DATA         file;
        SECTION_PARSING_DATA      section;
        NVAR_ENTRY_PARSING_DATA   nvar;
    };
} PARSING_DATA;

#endif // NVRAM_H
