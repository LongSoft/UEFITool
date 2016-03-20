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

#ifndef __PARSINGDATA_H__
#define __PARSINGDATA_H__

#include "basetypes.h"

//typedef struct _CAPSULE_PARSING_DATA {
//} CAPSULE_PARSING_DATA;

//typedef struct _IMAGE_PARSING_DATA {
//} IMAGE_PARSING_DATA;

//typedef struct _PADDING_PARSING_DATA {
//} PADDING_PARSING_DATA;

typedef struct _VOLUME_PARSING_DATA {
    EFI_GUID extendedHeaderGuid;
    UINT32   alignment;
    UINT8    revision;
    BOOLEAN  hasExtendedHeader;
    BOOLEAN  hasAppleCrc32;
    BOOLEAN  hasAppleFSO;
    BOOLEAN  isWeakAligned;
} VOLUME_PARSING_DATA;

//typedef struct _FREE_SPACE_PARSING_DATA {
//} FREE_SPACE_PARSING_DATA;

typedef struct _FILE_PARSING_DATA {
    union {
        UINT8   tailArray[2];
        UINT16  tail;
    };
    BOOLEAN hasTail;
    UINT8 format;
} FILE_PARSING_DATA;

#define RAW_FILE_FORMAT_UNKNOWN      0
#define RAW_FILE_FORMAT_NVAR_STORAGE 1

typedef struct _COMPRESSED_SECTION_PARSING_DATA {
    UINT32 uncompressedSize;
    UINT8  compressionType;
    UINT8  algorithm;
} COMPRESSED_SECTION_PARSING_DATA;

typedef struct _GUIDED_SECTION_PARSING_DATA {
    EFI_GUID guid;
} GUIDED_SECTION_PARSING_DATA;

typedef struct _FREEFORM_GUIDED_SECTION_PARSING_DATA {
    EFI_GUID guid;
} FREEFORM_GUIDED_SECTION_PARSING_DATA;

typedef struct _TE_IMAGE_SECTION_PARSING_DATA {
    UINT32  imageBase;
    UINT32  adjustedImageBase;
    UINT8   revision;
} TE_IMAGE_SECTION_PARSING_DATA;

typedef struct _SECTION_PARSING_DATA {
    union {
        COMPRESSED_SECTION_PARSING_DATA      compressed;
        GUIDED_SECTION_PARSING_DATA          guidDefined;
        FREEFORM_GUIDED_SECTION_PARSING_DATA freeformSubtypeGuid;
        TE_IMAGE_SECTION_PARSING_DATA        teImage;
    };
} SECTION_PARSING_DATA;

typedef struct _NVRAM_NVAR_PARSING_DATA {
    UINT32 next;
    UINT8  attributes;
    UINT8  extendedAttributes;
    UINT64 timestamp;
    UINT8  hash[0x20]; //SHA256
} NVRAM_NVAR_PARSING_DATA;

typedef struct _NVRAM_PARSING_DATA {
    //union {
        NVRAM_NVAR_PARSING_DATA      nvar;
    //};
} NVRAM_PARSING_DATA;


typedef struct _PARSING_DATA {
    UINT8   emptyByte;
    UINT8   ffsVersion;
    UINT32  offset;
    UINT32  address;
    union {
        //CAPSULE_PARSING_DATA capsule;
        //IMAGE_PARSING_DATA image;
        //PADDING_PARSING_DATA padding;
        VOLUME_PARSING_DATA  volume;
        //FREE_SPACE_PARSING_DATA freeSpace;
        FILE_PARSING_DATA    file;
        SECTION_PARSING_DATA section;
        NVRAM_PARSING_DATA   nvram;
    };
} PARSING_DATA;

#endif
