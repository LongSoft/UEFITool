/* types.cpp

Copyright (c) 2015, Nikolaj Schlej. All rights reserved.
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
    case Subtypes::DescriptorRegion:
        return QObject::tr("Descriptor");
    case Subtypes::GbeRegion:
        return QObject::tr("GbE");
    case Subtypes::MeRegion:
        return QObject::tr("ME");
    case Subtypes::BiosRegion:
        return QObject::tr("BIOS");
    case Subtypes::PdrRegion:
        return QObject::tr("PDR");
    case Subtypes::EcRegion:
        return QObject::tr("EC");
    default:
        return QObject::tr("Unknown");
    };
}

QString itemTypeToQString(const UINT8 type)
{
    switch (type) {
    case Types::Root:
        return QObject::tr("Root");
    case Types::Image:
        return QObject::tr("Image");
    case Types::Capsule:
        return QObject::tr("Capsule");
    case Types::Region:
        return QObject::tr("Region");
    case Types::Volume:
        return QObject::tr("Volume");
    case Types::Padding:
        return QObject::tr("Padding");
    case Types::File:
        return QObject::tr("File");
    case Types::Section:
        return QObject::tr("Section");
    case Types::FreeSpace:
        return QObject::tr("Free space");
    default:
        return QObject::tr("Unknown");
    }
}

QString itemSubtypeToQString(const UINT8 type, const UINT8 subtype)
{
    switch (type) {
    case Types::Root:
    case Types::Image:
        if (subtype == Subtypes::IntelImage)
            return QObject::tr("Intel");
        else if (subtype == Subtypes::UefiImage)
            return QObject::tr("UEFI");
        else
            return QObject::tr("Unknown subtype");
    case Types::Padding:
        if (subtype == Subtypes::ZeroPadding)
            return QObject::tr("Empty (0x00)");
        else if (subtype == Subtypes::OnePadding)
            return QObject::tr("Empty (0xFF)");
        else if (subtype == Subtypes::DataPadding)
            return QObject::tr("Non-empty");
        else
            return QObject::tr("Unknown subtype");
    case Types::Volume:
        if (subtype == Subtypes::UnknownVolume)
            return QObject::tr("Unknown");
        else if (subtype == Subtypes::Ffs2Volume)
            return QObject::tr("FFSv2");
        else if (subtype == Subtypes::Ffs3Volume)
            return QObject::tr("FFSv3");
        else
            return QObject::tr("Unknown subtype");
    case Types::Capsule:
        if (subtype == Subtypes::AptioSignedCapsule)
            return QObject::tr("Aptio signed");
        else if (subtype == Subtypes::AptioUnsignedCapsule)
            return QObject::tr("Aptio unsigned");
        else if (subtype == Subtypes::UefiCapsule)
            return QObject::tr("UEFI 2.0");
        else if (subtype == Subtypes::ToshibaCapsule)
            return QObject::tr("Toshiba");
        else
            return QObject::tr("Unknown subtype");
    case Types::Region:
        return regionTypeToQString(subtype);
    case Types::File:
        return fileTypeToQString(subtype);
    case Types::Section:
        return sectionTypeToQString(subtype);
    case Types::FreeSpace:
        return QString();
    default:
        return QObject::tr("Unknown subtype");
    }
}

QString compressionTypeToQString(const UINT8 algorithm)
{
    switch (algorithm) {
    case COMPRESSION_ALGORITHM_NONE:
        return QObject::tr("None");
    case COMPRESSION_ALGORITHM_EFI11:
        return QObject::tr("EFI 1.1");
    case COMPRESSION_ALGORITHM_TIANO:
        return QObject::tr("Tiano");
    case COMPRESSION_ALGORITHM_LZMA:
        return QObject::tr("LZMA");
    case COMPRESSION_ALGORITHM_IMLZMA:
        return QObject::tr("Intel modified LZMA");
    default:
        return QObject::tr("Unknown");
    }
}

QString actionTypeToQString(const UINT8 action)
{
    switch (action) {
    case Actions::NoAction:
        return "";
    case Actions::Create:
        return QObject::tr("Create");
    case Actions::Insert:
        return QObject::tr("Insert");
    case Actions::Replace:
        return QObject::tr("Replace");
    case Actions::Remove:
        return QObject::tr("Remove");
    case Actions::Rebuild:
        return QObject::tr("Rebuild");
    case Actions::Rebase:
        return QObject::tr("Rebase");
	case Actions::DoNotRebuild:
		return QObject::tr("Do not rebuild");
    default:
        return QObject::tr("Unknown");
    }
}
