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
        Rebase,
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
        SysFStore,
        EvsaStore,
        PhoenixFlashMapStore,
        InsydeFlashDeviceMapStore,
        DellDvarStore,
        CmdbStore,
        NvarGuidStore,
        NvarEntry,
        VssEntry,
        SysFEntry,
        EvsaEntry,
        PhoenixFlashMapEntry,
        InsydeFlashDeviceMapEntry,
        DellDvarEntry,
        Microcode,
        SlicData,
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
        CpdSpiEntry,
        StartupApDataEntry,
        DirectoryTable,
        DirectoryTableEntry,
    };
}

namespace Subtypes {
    enum ImageSubtypes{
        IntelImage = 90,
        UefiImage,
        AmdImage,
    };

    enum CapsuleSubtypes {
        AptioSignedCapsule = 100,
        AptioUnsignedCapsule,
        UefiCapsule,
        ToshibaCapsule,
    };

    enum VolumeSubtypes {
        UnknownVolume = 110,
        Ffs2Volume,
        Ffs3Volume,
        NvramVolume,
        MicrocodeVolume,
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
        PttRegion,
        PspL1DirectoryRegion,
        PspL2DirectoryRegion,
        PspDirectoryFile,
    };

    enum PaddingSubtypes {
        ZeroPadding = 120,
        OnePadding,
        DataPadding,
    };

    enum NvarEntrySubtypes {
        InvalidNvarEntry = 130,
        InvalidLinkNvarEntry,
        LinkNvarEntry,
        DataNvarEntry,
        FullNvarEntry,
    };

    enum VssEntrySubtypes {
        InvalidVssEntry = 140,
        StandardVssEntry,
        AppleVssEntry,
        AuthVssEntry,
        IntelVssEntry,
    };

    enum SysFEntrySubtypes {
        InvalidSysFEntry = 150,
        NormalSysFEntry,
    };
    
    enum DirectorySubtypes {
        PSPDirectory = 155,
        ComboDirectory,
        BiosDirectory,
        ISHDirectory,
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
        DataFlashMapEntry,
        UnknownFlashMapEntry,
    };

    enum DvarEntrySubtypes {
        InvalidDvarEntry = 180,
        NamespaceGuidDvarEntry,
        NameIdDvarEntry,
        UnknownDvarEntry
    };

    enum MicrocodeSubtypes {
        IntelMicrocode = 190,
        AmdMicrocode,
    };

    enum SlicDataSubtypes {
        PubkeySlicData = 200,
        MarkerSlicData,
    };

    // ME-specific
    enum IfwiPartitionSubtypes {
        DataIfwiPartition = 210,
        BootIfwiPartition,
    };

    enum FptEntrySubtypes {
        ValidFptEntry = 220,
        InvalidFptEntry,
    };

    enum FptPartitionSubtypes {
        CodeFptPartition = 230,
        DataFptPartition,
        GlutFptPartition,
    };

    enum CpdPartitionSubtypes {
        ManifestCpdPartition = 240,
        MetadataCpdPartition,
        KeyCpdPartition,
        CodeCpdPartition,
    };

    enum StartupApDataEntrySubtypes {
        x86128kStartupApDataEntry = 250,
    };
}

// *ToUString conversion routines
extern UString actionTypeToUString(const UINT8 action);
extern UString itemTypeToUString(const UINT8 type);
extern UString itemSubtypeToUString(const UINT8 type, const UINT8 subtype);
extern UString compressionTypeToUString(const UINT8 algorithm);
extern UString regionTypeToUString(const UINT8 type);
extern UString fitEntryTypeToUString(const UINT8 type);
extern UString hashTypeToUString(const UINT16 digest_agorithm_id);
extern UString insydeFlashDeviceMapEntryTypeGuidToUString(const EFI_GUID & guid);

#endif // TYPES_H
