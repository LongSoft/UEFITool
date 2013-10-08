/* treeitemtypes.h

Copyright (c) 2013, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

*/

#ifndef __TREEITEMTYPES_H__
#define __TREEITEMTYPES_H__

// TreeItem types
enum ItemTypes {
    RootItem,
    CapsuleItem,
    DescriptorItem,
    RegionItem,
    PaddingItem,
    VolumeItem,
    FileItem,
    SectionItem
};

// Capsule subtypes
enum CapsuleSubtypes {
    AptioCapsule,
    UefiCapsule
};

// Region subtypes
enum RegionSubtypes {
    GbeRegion,
    MeRegion,
    BiosRegion,
    PdrRegion
};

// File subtypes
enum FileSubtypes {
    RawFile,
    FreeformFile,
    SecurityCoreFile,
    PeiCoreFile,
    DxeCoreFile,
    PeiModuleFile,
    DxeDriverFile,
    CombinedPeiDxeFile,
    ApplicationFile,
    SmmModuleFile,
    VolumeImageFile,
    CombinedSmmDxeFile,
    SmmCoreFile,
    PadFile = 0xF0
};

enum SectionSubtypes {
    CompressionSection = 0x01,
    GuidDefinedSection,
    DisposableSection,
    Pe32ImageSection = 0x10,
    PicImageSection,
    TeImageSection,
    DxeDepexSection,
    VersionSection,
    UserInterfaceSection,
    Compatibility16Section,
    VolumeImageSection,
    FreeformSubtypeGuidSection,
    RawSection,
    PeiDepexSection = 0x1B,
    SmmDepexSection
};


#endif