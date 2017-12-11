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

const QVector<QByteArray> FFSv2Volumes =
    QVector<QByteArray>()
    << EFI_FIRMWARE_FILE_SYSTEM_GUID
    << EFI_FIRMWARE_FILE_SYSTEM2_GUID
    << EFI_APPLE_BOOT_VOLUME_FILE_SYSTEM_GUID
    << EFI_APPLE_BOOT_VOLUME_FILE_SYSTEM2_GUID
    << EFI_INTEL_FILE_SYSTEM_GUID
    << EFI_INTEL_FILE_SYSTEM2_GUID
    << EFI_SONY_FILE_SYSTEM_GUID;

const QVector<QByteArray> FFSv3Volumes =
    QVector<QByteArray>()
    << EFI_FIRMWARE_FILE_SYSTEM3_GUID;

const UINT8 ffsAlignmentTable[] =
{ 0, 4, 7, 9, 10, 12, 15, 16 };

const UINT8 ffsAlignment2Table[] =
{ 17, 18, 19, 20, 21, 22, 23, 24 };

UINT8 calculateSum8(const UINT8* buffer, UINT32 bufferSize)
{
    if (!buffer)
        return 0;

    UINT8 counter = 0;

    while (bufferSize--)
        counter += buffer[bufferSize];

    return counter;
}

UINT8 calculateChecksum8(const UINT8* buffer, UINT32 bufferSize)
{
    if (!buffer)
        return 0;

    return (UINT8)0x100 - calculateSum8(buffer, bufferSize);
}

UINT16 calculateChecksum16(const UINT16* buffer, UINT32 bufferSize)
{
    if (!buffer)
        return 0;

    UINT16 counter = 0;
    UINT32 index = 0;

    bufferSize /= sizeof(UINT16);

    for (; index < bufferSize; index++) {
        counter = (UINT16)(counter + buffer[index]);
    }

    return (UINT16)0x10000 - counter;
}

VOID uint32ToUint24(UINT32 size, UINT8* ffsSize)
{
    ffsSize[2] = (UINT8)((size) >> 16);
    ffsSize[1] = (UINT8)((size) >> 8);
    ffsSize[0] = (UINT8)((size));
}

UINT32 uint24ToUint32(const UINT8* ffsSize)
{
    return (ffsSize[2] << 16) +
        (ffsSize[1] << 8) +
        ffsSize[0];
}

QString guidToQString(const EFI_GUID& guid)
{
    QByteArray baGuid = QByteArray::fromRawData((const char*)guid.Data, sizeof(EFI_GUID));
    const UINT32 i32 = *(const UINT32*)baGuid.left(4).constData();
    const UINT16 i16_0 = *(const UINT16*)baGuid.mid(4, 2).constData();
    const UINT16 i16_1 = *(const UINT16*)baGuid.mid(6, 2).constData();
    const UINT8 i8_0 = *(const UINT8*)baGuid.mid(8, 1).constData();
    const UINT8 i8_1 = *(const UINT8*)baGuid.mid(9, 1).constData();
    const UINT8 i8_2 = *(const UINT8*)baGuid.mid(10, 1).constData();
    const UINT8 i8_3 = *(const UINT8*)baGuid.mid(11, 1).constData();
    const UINT8 i8_4 = *(const UINT8*)baGuid.mid(12, 1).constData();
    const UINT8 i8_5 = *(const UINT8*)baGuid.mid(13, 1).constData();
    const UINT8 i8_6 = *(const UINT8*)baGuid.mid(14, 1).constData();
    const UINT8 i8_7 = *(const UINT8*)baGuid.mid(15, 1).constData();

    return QString("%1-%2-%3-%4%5-%6%7%8%9%10%11")
        .arg(i32, 8, 16, QChar('0'))
        .arg(i16_0, 4, 16, QChar('0'))
        .arg(i16_1, 4, 16, QChar('0'))
        .arg(i8_0, 2, 16, QChar('0'))
        .arg(i8_1, 2, 16, QChar('0'))
        .arg(i8_2, 2, 16, QChar('0'))
        .arg(i8_3, 2, 16, QChar('0'))
        .arg(i8_4, 2, 16, QChar('0'))
        .arg(i8_5, 2, 16, QChar('0'))
        .arg(i8_6, 2, 16, QChar('0'))
        .arg(i8_7, 2, 16, QChar('0')).toUpper();
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
	case EFI_FV_FILETYPE_SMM_STANDALONE:        return QObject::tr("SMM standalone");
	case EFI_FV_FILETYPE_SMM_CORE_STANDALONE:   return QObject::tr("SMM core standalone");
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
    case EFI_SECTION_USER_INTERFACE:            return QObject::tr("User interface");
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

UINT32 sizeOfSectionHeader(const EFI_COMMON_SECTION_HEADER* header)
{
    if (!header)
        return 0;

    bool extended = false;
    if (uint24ToUint32(header->Size) == EFI_SECTION2_IS_USED) {
        extended = true;
    }

    switch (header->Type)
    {
    case EFI_SECTION_GUID_DEFINED: {
        if (!extended) {
            const EFI_GUID_DEFINED_SECTION* gdsHeader = (const EFI_GUID_DEFINED_SECTION*)header;
            if (QByteArray((const char*)&gdsHeader->SectionDefinitionGuid, sizeof(EFI_GUID)) == EFI_FIRMWARE_CONTENTS_SIGNED_GUID) {
                const WIN_CERTIFICATE* certificateHeader = (const WIN_CERTIFICATE*)(gdsHeader + 1);
                return gdsHeader->DataOffset + certificateHeader->Length;
            }
            return gdsHeader->DataOffset;
        }
        else {
            const EFI_GUID_DEFINED_SECTION2* gdsHeader = (const EFI_GUID_DEFINED_SECTION2*)header;
            if (QByteArray((const char*)&gdsHeader->SectionDefinitionGuid, sizeof(EFI_GUID)) == EFI_FIRMWARE_CONTENTS_SIGNED_GUID) {
                const WIN_CERTIFICATE* certificateHeader = (const WIN_CERTIFICATE*)(gdsHeader + 1);
                return gdsHeader->DataOffset + certificateHeader->Length;
            }
            return gdsHeader->DataOffset;
        }
    }
    case EFI_SECTION_COMPRESSION:           return extended ? sizeof(EFI_COMPRESSION_SECTION2) : sizeof(EFI_COMPRESSION_SECTION);
    case EFI_SECTION_DISPOSABLE:            return extended ? sizeof(EFI_DISPOSABLE_SECTION2) : sizeof(EFI_DISPOSABLE_SECTION);
    case EFI_SECTION_PE32:                  return extended ? sizeof(EFI_PE32_SECTION2) : sizeof(EFI_PE32_SECTION);
    case EFI_SECTION_PIC:                   return extended ? sizeof(EFI_PIC_SECTION2) : sizeof(EFI_PIC_SECTION);
    case EFI_SECTION_TE:                    return extended ? sizeof(EFI_TE_SECTION2) : sizeof(EFI_TE_SECTION);
    case EFI_SECTION_DXE_DEPEX:             return extended ? sizeof(EFI_DXE_DEPEX_SECTION2) : sizeof(EFI_DXE_DEPEX_SECTION);
    case EFI_SECTION_VERSION:               return extended ? sizeof(EFI_VERSION_SECTION2) : sizeof(EFI_VERSION_SECTION);
    case EFI_SECTION_USER_INTERFACE:        return extended ? sizeof(EFI_USER_INTERFACE_SECTION2) : sizeof(EFI_USER_INTERFACE_SECTION);
    case EFI_SECTION_COMPATIBILITY16:       return extended ? sizeof(EFI_COMPATIBILITY16_SECTION2) : sizeof(EFI_COMPATIBILITY16_SECTION);
    case EFI_SECTION_FIRMWARE_VOLUME_IMAGE: return extended ? sizeof(EFI_FIRMWARE_VOLUME_IMAGE_SECTION2) : sizeof(EFI_FIRMWARE_VOLUME_IMAGE_SECTION);
    case EFI_SECTION_FREEFORM_SUBTYPE_GUID: return extended ? sizeof(EFI_FREEFORM_SUBTYPE_GUID_SECTION2) : sizeof(EFI_FREEFORM_SUBTYPE_GUID_SECTION);
    case EFI_SECTION_RAW:                   return extended ? sizeof(EFI_RAW_SECTION2) : sizeof(EFI_RAW_SECTION);
    case EFI_SECTION_PEI_DEPEX:             return extended ? sizeof(EFI_PEI_DEPEX_SECTION2) : sizeof(EFI_PEI_DEPEX_SECTION);
    case EFI_SECTION_SMM_DEPEX:             return extended ? sizeof(EFI_SMM_DEPEX_SECTION2) : sizeof(EFI_SMM_DEPEX_SECTION);
    case INSYDE_SECTION_POSTCODE:           return extended ? sizeof(POSTCODE_SECTION2) : sizeof(POSTCODE_SECTION);
    case SCT_SECTION_POSTCODE:              return extended ? sizeof(POSTCODE_SECTION2) : sizeof(POSTCODE_SECTION);
    default:                                return extended ? sizeof(EFI_COMMON_SECTION_HEADER2) : sizeof(EFI_COMMON_SECTION_HEADER);
    }
}
