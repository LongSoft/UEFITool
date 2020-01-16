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
        VssStore,
        Vss2Store,
        FtwStore,
        FdcStore,
        FsysStore,
        EvsaStore,
        FlashMapStore,
        CmdbStore,
        NvarEntry,
        VssEntry,
        FsysEntry,
        EvsaEntry,
        FlashMapEntry,
        Microcode,
        SlicData,
        // ME-specific
        IfwiHeader,
        IfwiPartition,
        FptStore,
        FptEntry,
        FptPartition,
        BpdtStore,
        BpdtEntry,
        BpdtPartition,
        CpdStore,
        CpdEntry,
        CpdPartition,
        CpdExtension,
        CpdSpiEntry
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
        NvramVolume,
        MicrocodeVolume
    };

    enum RegionSubtypes {
        DescriptorRegion = 0,
        BiosRegion,
        MeRegion,
        GbeRegion,
        PdrRegion,
        DevExp1Region,
        Bios2Region,
        MicrocodeRegion,
        EcRegion,
        DevExp2Region,
        IeRegion,
        Tgbe1Region,
        Tgbe2Region,
        Reserved1Region,
        Reserved2Region,
        PttRegion
    };

    enum PaddingSubtypes {
        ZeroPadding = 120,
        OnePadding,
        DataPadding
    };

    enum NvarEntrySubtypes {
        InvalidNvarEntry = 130,
        InvalidLinkNvarEntry,
        LinkNvarEntry,
        DataNvarEntry,
        FullNvarEntry
    };

    enum VssEntrySubtypes {
        InvalidVssEntry = 140,
        StandardVssEntry,
        AppleVssEntry,
        AuthVssEntry,
        IntelVssEntry
    };

    enum FsysEntrySubtypes {
        InvalidFsysEntry = 150,
        NormalFsysEntry
    };
    
    enum EvsaEntrySubtypes {
        InvalidEvsaEntry = 160,
        UnknownEvsaEntry,
        GuidEvsaEntry,
        NameEvsaEntry,
        DataEvsaEntry,
    };

    enum FlashMapEntrySubtypes {
        VolumeFlashMapEntry = 170,
        DataFlashMapEntry
    };

    enum MicrocodeSubtypes {
        IntelMicrocode = 180,
        AmdMicrocode
    };

    enum SlicDataSubtypes {
        PubkeySlicData = 190,
        MarkerSlicData
    };

    // ME-specific
    enum IfwiPartitionSubtypes {
        DataIfwiPartition = 200,
        BootIfwiPartition
    };

    enum FptEntrySubtypes {
        ValidFptEntry = 210,
        InvalidFptEntry
    };

    enum FptPartitionSubtypes {
        CodeFptPartition = 220,
        DataFptPartition,
        GlutFptPartition
    };
	
    enum CpdPartitionSubtypes {
        ManifestCpdPartition = 230,
        MetadataCpdPartition,
        KeyCpdPartition,
        CodeCpdPartition
    };
}

// *ToUString conversion routines
extern UString actionTypeToUString(const UINT8 action);
extern UString itemTypeToUString(const UINT8 type);
extern UString itemSubtypeToUString(const UINT8 type, const UINT8 subtype);
extern UString compressionTypeToUString(const UINT8 algorithm);
extern UString regionTypeToUString(const UINT8 type);
extern UString fitEntryTypeToUString(const UINT8 type);

#endif // TYPES_H
