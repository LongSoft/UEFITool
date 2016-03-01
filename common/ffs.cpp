/* ffs.cpp

Copyright (c) 2015, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHWARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*/

#include <QObject>
#include "ffs.h"

// This is a workaround for the lack of static std::vector initializer before C++11
const QByteArray FFSv2VolumesInt[] = {
    EFI_FIRMWARE_FILE_SYSTEM_GUID,
    EFI_FIRMWARE_FILE_SYSTEM2_GUID,
    EFI_APPLE_BOOT_VOLUME_FILE_SYSTEM_GUID,
    EFI_APPLE_BOOT_VOLUME_FILE_SYSTEM2_GUID,
    EFI_INTEL_FILE_SYSTEM_GUID,
    EFI_INTEL_FILE_SYSTEM2_GUID,
    EFI_SONY_FILE_SYSTEM_GUID
};
// This number must be updated if the array above is grown
#define FFSv2VolumesIntSize 7
const std::vector<QByteArray> FFSv2Volumes(FFSv2VolumesInt, FFSv2VolumesInt + FFSv2VolumesIntSize);
// Luckily, FFSv3Volumes now only has 1 element
const std::vector<QByteArray> FFSv3Volumes(1, EFI_FIRMWARE_FILE_SYSTEM3_GUID);

const UINT8 ffsAlignmentTable[] =
{ 0, 4, 7, 9, 10, 12, 15, 16 };

VOID uint32ToUint24(UINT32 size, UINT8* ffsSize)
{
    ffsSize[2] = (UINT8)((size) >> 16);
    ffsSize[1] = (UINT8)((size) >> 8);
    ffsSize[0] = (UINT8)((size));
}

UINT32 uint24ToUint32(const UINT8* ffsSize)
{
    return *(UINT32*)ffsSize & 0x00FFFFFF;
}

QString guidToQString(const EFI_GUID & guid)
{
    return QString("%1-%2-%3-%4%5-%6%7%8%9%10%11")
        .arg(*(const UINT32*)&guid.Data[0], 8, 16, QChar('0'))
        .arg(*(const UINT16*)&guid.Data[4], 4, 16, QChar('0'))
        .arg(*(const UINT16*)&guid.Data[6], 4, 16, QChar('0'))
        .arg(guid.Data[8],  2, 16, QChar('0'))
        .arg(guid.Data[9],  2, 16, QChar('0'))
        .arg(guid.Data[10], 2, 16, QChar('0'))
        .arg(guid.Data[11], 2, 16, QChar('0'))
        .arg(guid.Data[12], 2, 16, QChar('0'))
        .arg(guid.Data[13], 2, 16, QChar('0'))
        .arg(guid.Data[14], 2, 16, QChar('0'))
        .arg(guid.Data[15], 2, 16, QChar('0')).toUpper();
}

QString fileTypeToQString(const UINT8 type)
{
    switch (type)
    {
    case EFI_FV_FILETYPE_RAW:                   return QObject::tr("Raw");
    case EFI_FV_FILETYPE_FREEFORM:              return QObject::tr("Freeform");
    case EFI_FV_FILETYPE_SECURITY_CORE:         return QObject::tr("SEC core");
    case EFI_FV_FILETYPE_PEI_CORE:              return QObject::tr("PEI core");
    case EFI_FV_FILETYPE_DXE_CORE:              return QObject::tr("DXE core");
    case EFI_FV_FILETYPE_PEIM:                  return QObject::tr("PEI module");
    case EFI_FV_FILETYPE_DRIVER:                return QObject::tr("DXE driver");
    case EFI_FV_FILETYPE_COMBINED_PEIM_DRIVER:  return QObject::tr("Combined PEI/DXE");
    case EFI_FV_FILETYPE_APPLICATION:           return QObject::tr("Application");
    case EFI_FV_FILETYPE_SMM:                   return QObject::tr("SMM module");
    case EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE: return QObject::tr("Volume image");
    case EFI_FV_FILETYPE_COMBINED_SMM_DXE:      return QObject::tr("Combined SMM/DXE");
    case EFI_FV_FILETYPE_SMM_CORE:              return QObject::tr("SMM core");
    case EFI_FV_FILETYPE_PAD:                   return QObject::tr("Pad");
    default:                                    return QObject::tr("Unknown");
    };
}

QString sectionTypeToQString(const UINT8 type)
{
    switch (type)
    {
    case EFI_SECTION_COMPRESSION:               return QObject::tr("Compressed");
    case EFI_SECTION_GUID_DEFINED:              return QObject::tr("GUID defined");
    case EFI_SECTION_DISPOSABLE:                return QObject::tr("Disposable");
    case EFI_SECTION_PE32:                      return QObject::tr("PE32 image");
    case EFI_SECTION_PIC:                       return QObject::tr("PIC image");
    case EFI_SECTION_TE:                        return QObject::tr("TE image");
    case EFI_SECTION_DXE_DEPEX:                 return QObject::tr("DXE dependency");
    case EFI_SECTION_VERSION:                   return QObject::tr("Version");
    case EFI_SECTION_USER_INTERFACE:            return QObject::tr("UI");
    case EFI_SECTION_COMPATIBILITY16:           return QObject::tr("16-bit image");
    case EFI_SECTION_FIRMWARE_VOLUME_IMAGE:     return QObject::tr("Volume image");
    case EFI_SECTION_FREEFORM_SUBTYPE_GUID:     return QObject::tr("Freeform subtype GUID");
    case EFI_SECTION_RAW:                       return QObject::tr("Raw");
    case EFI_SECTION_PEI_DEPEX:                 return QObject::tr("PEI dependency");
    case EFI_SECTION_SMM_DEPEX:                 return QObject::tr("SMM dependency");
    case INSYDE_SECTION_POSTCODE:               return QObject::tr("Insyde postcode");
    case SCT_SECTION_POSTCODE:                  return QObject::tr("SCT postcode");
    default:                                    return QObject::tr("Unknown");
    }
}

