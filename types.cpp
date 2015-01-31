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
    case ATTR_REGION_TYPE_DESCRIPTOR:
        return QObject::tr("Descriptor");
    case ATTR_REGION_TYPE_GBE:
        return QObject::tr("GbE");
    case ATTR_REGION_TYPE_ME:
        return QObject::tr("ME/TXE");
    case ATTR_REGION_TYPE_BIOS:
        return QObject::tr("BIOS");
    case ATTR_REGION_TYPE_PDR:
        return QObject::tr("PDR");
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
    default:
        return QObject::tr("Unknown");
    }
}

QString itemAttributesToQString(const UINT8 type, const UINT8 attributes)
{
    switch (type) {
    case Types::Root:
    case Types::Image:
        if (attributes == ATTR_IMAGE_TYPE_DESCRIPTOR)
            return QObject::tr("Intel");
        else if (attributes == ATTR_IMAGE_TYPE_UEFI)
            return QObject::tr("UEFI");
        else
            return QObject::tr("Unknown");
    case Types::Padding:
        if (attributes == ATTR_PADDING_ZERO_EMPTY)
            return QObject::tr("Empty (0x00)");
        else if (attributes == ATTR_PADDING_ONE_EMPTY)
            return QObject::tr("Empty (0xFF)");
        else if (attributes == ATTR_PADDING_DATA)
            return QObject::tr("Non-empty");
        else
            return QObject::tr("Unknown");
    case Types::Volume: {
        QString string;
        VOLUME_ATTRIBUTES* volumeAttr = (VOLUME_ATTRIBUTES*)&attributes;
        if (volumeAttr->ZeroVectorCrc)
            string += QObject::tr("ZVCRC ");

        if (volumeAttr->VtfPresent)
            string += QObject::tr("Boot ");

        if (volumeAttr->Unknown) {
            string += QObject::tr("Unknown");
            return string;
        }

        if (volumeAttr->FsVersion == 2 || volumeAttr->FsVersion == 3)
            string += QObject::tr("FFSv%1").arg(volumeAttr->FsVersion);
        else
            return QObject::tr("Unknown FFS version");

        return string;
    }
    case Types::Capsule: {
        QString string;
        CAPSULE_ATTRIBUTES* capsuleAttr = (CAPSULE_ATTRIBUTES*)&attributes;
        if (capsuleAttr->Type == ATTR_CAPSULE_TYPE_APTIO)
            string += QObject::tr("Aptio ");
        else if (capsuleAttr->Type == ATTR_CAPSULE_TYPE_UEFI20)
            string += QObject::tr("UEFI 2.0 ");
        else 
            return QObject::tr("Unknown type");

        if (capsuleAttr->Signed)
            string += QObject::tr("signed");
        else
            string += QObject::tr("unsigned");

        return string;
    }
    case Types::Region:
        return regionTypeToQString(attributes);
    case Types::File:
        return fileTypeToQString(attributes);
    case Types::Section:
        return sectionTypeToQString(attributes);
    default:
        return QObject::tr("Unknown");
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
    default:
        return QObject::tr("Unknown");
    }
}