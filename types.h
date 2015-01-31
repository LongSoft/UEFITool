/* types.h

Copyright (c) 2015, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#ifndef __TYPES_H__
#define __TYPES_H__

#include "basetypes.h"

#pragma pack(push, 1)

// Actions
namespace Actions
{
    enum ActionTypes {
        NoAction = 50,
        Create,
        Insert,
        Replace,
        Remove,
        Rebuild,
        Rebase
    };
}

// Types
namespace Types {
    enum ItemTypes {
        Root = 60,
        Capsule,
        Image,
        Region,
        Padding,
        Volume,
        File,
        Section
    };
}

// Capsule attributes
typedef struct _CAPSULE_ATTRIBUTES {
    UINT32 Type     : 7;
    UINT32 Signed   : 1;
    UINT32 Reserved : 24;
} CAPSULE_ATTRIBUTES;
#define ATTR_CAPSULE_TYPE_UEFI20 0
#define ATTR_CAPSULE_TYPE_APTIO  1

typedef struct _IMAGE_ATTRIBUTES {
    UINT32 IntelDescriptor : 1;
    UINT32 Reserved        : 31;
} IMAGE_ATTRIBUTES;
#define ATTR_IMAGE_TYPE_UEFI       0
#define ATTR_IMAGE_TYPE_DESCRIPTOR 1

typedef struct _REGION_ATTRIBUTES {
    UINT32 Type     : 7;
    UINT32 Empty    : 1;
    UINT32 Reserved : 24;
} REGION_ATTRIBUTES;

#define ATTR_REGION_TYPE_DESCRIPTOR 0
#define ATTR_REGION_TYPE_GBE        1
#define ATTR_REGION_TYPE_ME         2
#define ATTR_REGION_TYPE_BIOS       3
#define ATTR_REGION_TYPE_PDR        4

typedef struct _VOLUME_ATTRIBUTES {
    UINT32 Unknown       : 1;
    UINT32 VtfPresent    : 1;
    UINT32 ZeroVectorCrc : 1;
    UINT32 FsVersion     : 5;
    UINT32 Reserved      : 24;
} VOLUME_ATTRIBUTES;

typedef struct _PADDING_ATTRIBUTES {
    UINT32 Empty         : 1;
    UINT32 ErasePolarity : 1;
    UINT32 Reserved      : 30;
} PADDING_ATTRIBUTES;
#define ATTR_PADDING_DATA       0
#define ATTR_PADDING_ZERO_EMPTY 1
#define ATTR_PADDING_ONE_EMPTY  3

#pragma pack(pop)

// *ToQString conversion routines
extern QString actionTypeToQString(const UINT8 action);
extern QString itemTypeToQString(const UINT8 type);
extern QString itemAttributesToQString(const UINT8 type, const UINT8 attributes);
extern QString compressionTypeToQString(const UINT8 algorithm);
extern QString regionTypeToQString(const UINT8 type);
#endif