/* types.cpp

Copyright (c) 2016, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHWARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*/

#include <QObject>
#include <QString>
#include "types.h"
#include "ffs.h"

QString regionTypeToQString(const UINT8 type)
{
    switch (type)
    {
    case Subtypes::DescriptorRegion:  return QObject::tr("Descriptor");
    case Subtypes::BiosRegion:        return QObject::tr("BIOS");
    case Subtypes::MeRegion:          return QObject::tr("ME");
    case Subtypes::GbeRegion:         return QObject::tr("GbE");
    case Subtypes::PdrRegion:         return QObject::tr("PDR");
    case Subtypes::Reserved1Region:   return QObject::tr("Reserved1");
    case Subtypes::Reserved2Region:   return QObject::tr("Reserved2");
    case Subtypes::Reserved3Region:   return QObject::tr("Reserved3");
    case Subtypes::EcRegion:          return QObject::tr("EC");
    case Subtypes::Reserved4Region:   return QObject::tr("Reserved4");
    };

    return  QObject::tr("Unknown");
}

QString itemTypeToQString(const UINT8 type)
{
    switch (type) {
    case Types::Root:          return QObject::tr("Root");
    case Types::Image:         return QObject::tr("Image");
    case Types::Capsule:       return QObject::tr("Capsule");
    case Types::Region:        return QObject::tr("Region");
    case Types::Volume:        return QObject::tr("Volume");
    case Types::Padding:       return QObject::tr("Padding");
    case Types::File:          return QObject::tr("File");
    case Types::Section:       return QObject::tr("Section");
    case Types::FreeSpace:     return QObject::tr("Free space");
    case Types::VssStore:      return QObject::tr("VSS store");
    case Types::FtwStore:      return QObject::tr("FTW store");
    case Types::FdcStore:      return QObject::tr("FDC store");
    case Types::FsysStore:     return QObject::tr("Fsys store");
    case Types::EvsaStore:     return QObject::tr("EVSA store");
    case Types::CmdbStore:     return QObject::tr("CMDB store");
    case Types::FlashMapStore: return QObject::tr("FlashMap store");
    case Types::NvarEntry:     return QObject::tr("NVAR entry");
    case Types::VssEntry:      return QObject::tr("VSS entry");
    case Types::FsysEntry:     return QObject::tr("Fsys entry");
    case Types::EvsaEntry:     return QObject::tr("EVSA entry");
    case Types::FlashMapEntry: return QObject::tr("FlashMap entry");
    case Types::Microcode:     return QObject::tr("Microcode");
    case Types::SlicData:      return QObject::tr("SLIC data");
    }

    return  QObject::tr("Unknown");
}

QString itemSubtypeToQString(const UINT8 type, const UINT8 subtype)
{
    switch (type) {
    case Types::Root:
    case Types::FreeSpace:
    case Types::VssStore:
    case Types::FdcStore:
    case Types::FsysStore:
    case Types::EvsaStore:
    case Types::FtwStore:
    case Types::FlashMapStore:
    case Types::CmdbStore:
    case Types::FsysEntry:
    case Types::SlicData:                                  return QString();
    case Types::Image:
        if (subtype == Subtypes::IntelImage)               return QObject::tr("Intel");
        if (subtype == Subtypes::UefiImage)                return QObject::tr("UEFI");
        break;
    case Types::Padding:
        if (subtype == Subtypes::ZeroPadding)              return QObject::tr("Empty (0x00)");
        if (subtype == Subtypes::OnePadding)               return QObject::tr("Empty (0xFF)");
        if (subtype == Subtypes::DataPadding)              return QObject::tr("Non-empty");
        break;
    case Types::Volume: 
        if (subtype == Subtypes::UnknownVolume)            return QObject::tr("Unknown");
        if (subtype == Subtypes::Ffs2Volume)               return QObject::tr("FFSv2");
        if (subtype == Subtypes::Ffs3Volume)               return QObject::tr("FFSv3");
        if (subtype == Subtypes::NvramVolume)              return QObject::tr("NVRAM");
        break;
    case Types::Capsule: 
        if (subtype == Subtypes::AptioSignedCapsule)       return QObject::tr("Aptio signed");
        if (subtype == Subtypes::AptioUnsignedCapsule)     return QObject::tr("Aptio unsigned");
        if (subtype == Subtypes::UefiCapsule)              return QObject::tr("UEFI 2.0");
        if (subtype == Subtypes::ToshibaCapsule)           return QObject::tr("Toshiba");
        break;
    case Types::Region:                                    return regionTypeToQString(subtype);
    case Types::File:                                      return fileTypeToQString(subtype);
    case Types::Section:                                   return sectionTypeToQString(subtype);
    case Types::NvarEntry:
        if (subtype == Subtypes::InvalidNvarEntry)         return QObject::tr("Invalid");
        if (subtype == Subtypes::InvalidLinkNvarEntry)     return QObject::tr("Invalid link");
        if (subtype == Subtypes::LinkNvarEntry)            return QObject::tr("Link");
        if (subtype == Subtypes::DataNvarEntry)            return QObject::tr("Data");
        if (subtype == Subtypes::FullNvarEntry)            return QObject::tr("Full");
        break;
    case Types::VssEntry:
        if (subtype == Subtypes::InvalidVssEntry)          return QObject::tr("Invalid");
        if (subtype == Subtypes::StandardVssEntry)         return QObject::tr("Standard");
        if (subtype == Subtypes::AppleVssEntry)            return QObject::tr("Apple");
        if (subtype == Subtypes::AuthVssEntry)             return QObject::tr("Auth");
        break;
    case Types::EvsaEntry:
        if (subtype == Subtypes::InvalidEvsaEntry)         return QObject::tr("Invalid");
        if (subtype == Subtypes::UnknownEvsaEntry)         return QObject::tr("Unknown");
        if (subtype == Subtypes::GuidEvsaEntry)            return QObject::tr("GUID");
        if (subtype == Subtypes::NameEvsaEntry)            return QObject::tr("Name");
        if (subtype == Subtypes::DataEvsaEntry)            return QObject::tr("Data");
        break;
    case Types::FlashMapEntry:
        if (subtype == Subtypes::VolumeFlashMapEntry)      return QObject::tr("Volume");
        if (subtype == Subtypes::DataFlashMapEntry)        return QObject::tr("Data");
        break;
    case Types::Microcode:
        if (subtype == Subtypes::IntelMicrocode)           return QObject::tr("Intel");
        if (subtype == Subtypes::AmdMicrocode)             return QObject::tr("AMD");
        break;
    }

    return  QObject::tr("Unknown");
}

QString compressionTypeToQString(const UINT8 algorithm)
{
    switch (algorithm) {
    case COMPRESSION_ALGORITHM_NONE:         return QObject::tr("None");
    case COMPRESSION_ALGORITHM_EFI11:        return QObject::tr("EFI 1.1");
    case COMPRESSION_ALGORITHM_TIANO:        return QObject::tr("Tiano");
    case COMPRESSION_ALGORITHM_UNDECIDED:    return QObject::tr("Undecided Tiano/EFI 1.1");
    case COMPRESSION_ALGORITHM_LZMA:         return QObject::tr("LZMA");
    case COMPRESSION_ALGORITHM_IMLZMA:       return QObject::tr("Intel modified LZMA");
    }

    return  QObject::tr("Unknown");
}

QString actionTypeToQString(const UINT8 action)
{
    switch (action) {
    case Actions::NoAction:      return QString();
    case Actions::Create:        return QObject::tr("Create");
    case Actions::Insert:        return QObject::tr("Insert");
    case Actions::Replace:       return QObject::tr("Replace");
    case Actions::Remove:        return QObject::tr("Remove");
    case Actions::Rebuild:       return QObject::tr("Rebuild");
    case Actions::Rebase:        return QObject::tr("Rebase");
    }

    return  QObject::tr("Unknown");
}