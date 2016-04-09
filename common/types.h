/* types.h

Copyright (c) 2016, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#ifndef TYPES_H
#define TYPES_H

#include "basetypes.h"

// Actions
namespace Actions
{
    enum ActionTypes {
        NoAction = 50,
        Erase,
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
        Section,
        FreeSpace,
        NvramStoreVss,
        NvramStoreFdc,
        NvramStoreFsys,
        NvramStoreEvsa,
        NvramFtwBlock,
        NvramVariableNvar,
        NvramVariableVss,
        NvramVariableFsys,
        NvramEntryEvsa
    };
}

namespace Subtypes {
    enum ImageSubtypes{
        IntelImage = 90,
        UefiImage
    };

    enum CapsuleSubtypes {
        AptioSignedCapsule = 100,
        AptioUnsignedCapsule,
        UefiCapsule,
        ToshibaCapsule
    };

    enum VolumeSubtypes {
        UnknownVolume = 110,
        Ffs2Volume,
        Ffs3Volume,
        NvramVolume
    };

    enum RegionSubtypes {
        DescriptorRegion = 0,
        BiosRegion,
        MeRegion,
        GbeRegion,
        PdrRegion,
        Reserved1Region,
        Reserved2Region,
        Reserved3Region,
        EcRegion,
        Reserved4Region
    };

    enum PaddingSubtypes {
        ZeroPadding = 120,
        OnePadding,
        DataPadding
    };

    enum NvarVariableSubtypes {
        InvalidNvarVariable = 130,
        InvalidLinkNvarVariable,
        LinkNvarVariable,
        DataNvarVariable,
        FullNvarVariable
    };

    enum VssVariableSubtypes {
        InvalidVssVariable = 140,
        StandardVssVariable,
        Crc32VssVariable,
        AuthVssVariable
    };

    enum EvsaVariableSubtypes {
        InvalidEvsaEntry = 150,
        UnknownEvsaEntry,
        GuidEvsaEntry,
        NameEvsaEntry,
        DataEvsaEntry,
    };
};

// *ToQString conversion routines
extern QString actionTypeToQString(const UINT8 action);
extern QString itemTypeToQString(const UINT8 type);
extern QString itemSubtypeToQString(const UINT8 type, const UINT8 subtype);
extern QString compressionTypeToQString(const UINT8 algorithm);
extern QString regionTypeToQString(const UINT8 type);

#endif // TYPES_H
