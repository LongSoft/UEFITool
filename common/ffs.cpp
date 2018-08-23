/* ffs.cpp

Copyright (c) 2015, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHWARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*/

#include "ffs.h"
#include "guiddatabase.h"

// This is a workaround for the lack of static std::vector initializer before C++11
const UByteArray FFSv2VolumesInt[] = {
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
const std::vector<UByteArray> FFSv2Volumes(FFSv2VolumesInt, FFSv2VolumesInt + FFSv2VolumesIntSize);
// Luckily, FFSv3Volumes now only has 1 element
const std::vector<UByteArray> FFSv3Volumes(1, EFI_FIRMWARE_FILE_SYSTEM3_GUID);

const UINT8 ffsAlignmentTable[] =
{ 0, 4, 7, 9, 10, 12, 15, 16 };

const UINT8 ffsAlignment2Table[] =
{ 17, 18, 19, 20, 21, 22, 23, 24 };

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

UString guidToUString(const EFI_GUID & guid, bool convertToString)
{
    if (convertToString) {
        UString readableName = guidDatabaseLookup(guid);
        if (!readableName.isEmpty())
            return readableName;
    }

    return usprintf("%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
        guid.Data1,
        guid.Data2,
        guid.Data3,
        guid.Data4[0],
        guid.Data4[1],
        guid.Data4[2],
        guid.Data4[3],
        guid.Data4[4],
        guid.Data4[5],
        guid.Data4[6],
        guid.Data4[7]);
}

UString fileTypeToUString(const UINT8 type)
{
    switch (type)
    {
    case EFI_FV_FILETYPE_RAW:                   return UString("Raw");
    case EFI_FV_FILETYPE_FREEFORM:              return UString("Freeform");
    case EFI_FV_FILETYPE_SECURITY_CORE:         return UString("SEC core");
    case EFI_FV_FILETYPE_PEI_CORE:              return UString("PEI core");
    case EFI_FV_FILETYPE_DXE_CORE:              return UString("DXE core");
    case EFI_FV_FILETYPE_PEIM:                  return UString("PEI module");
    case EFI_FV_FILETYPE_DRIVER:                return UString("DXE driver");
    case EFI_FV_FILETYPE_COMBINED_PEIM_DRIVER:  return UString("Combined PEI/DXE");
    case EFI_FV_FILETYPE_APPLICATION:           return UString("Application");
    case EFI_FV_FILETYPE_MM:                    return UString("SMM module");
    case EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE: return UString("Volume image");
    case EFI_FV_FILETYPE_COMBINED_MM_DXE:       return UString("Combined SMM/DXE");
    case EFI_FV_FILETYPE_MM_CORE:               return UString("SMM core");
    case EFI_FV_FILETYPE_MM_STANDALONE:         return UString("MM standalone module");
    case EFI_FV_FILETYPE_MM_CORE_STANDALONE:    return UString("MM standalone core");
    case EFI_FV_FILETYPE_PAD:                   return UString("Pad");
    default:                                    return UString("Unknown");
    };
}

UString sectionTypeToUString(const UINT8 type)
{
    switch (type)
    {
    case EFI_SECTION_COMPRESSION:               return UString("Compressed");
    case EFI_SECTION_GUID_DEFINED:              return UString("GUID defined");
    case EFI_SECTION_DISPOSABLE:                return UString("Disposable");
    case EFI_SECTION_PE32:                      return UString("PE32 image");
    case EFI_SECTION_PIC:                       return UString("PIC image");
    case EFI_SECTION_TE:                        return UString("TE image");
    case EFI_SECTION_DXE_DEPEX:                 return UString("DXE dependency");
    case EFI_SECTION_VERSION:                   return UString("Version");
    case EFI_SECTION_USER_INTERFACE:            return UString("UI");
    case EFI_SECTION_COMPATIBILITY16:           return UString("16-bit image");
    case EFI_SECTION_FIRMWARE_VOLUME_IMAGE:     return UString("Volume image");
    case EFI_SECTION_FREEFORM_SUBTYPE_GUID:     return UString("Freeform subtype GUID");
    case EFI_SECTION_RAW:                       return UString("Raw");
    case EFI_SECTION_PEI_DEPEX:                 return UString("PEI dependency");
    case EFI_SECTION_MM_DEPEX:                  return UString("MM dependency");
    case INSYDE_SECTION_POSTCODE:               return UString("Insyde postcode");
    case PHOENIX_SECTION_POSTCODE:              return UString("Phoenix postcode");
    default:                                    return UString("Unknown");
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
            if (UByteArray((const char*)&gdsHeader->SectionDefinitionGuid, sizeof(EFI_GUID)) == EFI_FIRMWARE_CONTENTS_SIGNED_GUID) {
                const WIN_CERTIFICATE* certificateHeader = (const WIN_CERTIFICATE*)(gdsHeader + 1);
                return gdsHeader->DataOffset + certificateHeader->Length;
            }
            return gdsHeader->DataOffset;
        }
        else {
            const EFI_GUID_DEFINED_SECTION2* gdsHeader = (const EFI_GUID_DEFINED_SECTION2*)header;
            if (UByteArray((const char*)&gdsHeader->SectionDefinitionGuid, sizeof(EFI_GUID)) == EFI_FIRMWARE_CONTENTS_SIGNED_GUID) {
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
    case EFI_SECTION_MM_DEPEX:              return extended ? sizeof(EFI_SMM_DEPEX_SECTION2) : sizeof(EFI_SMM_DEPEX_SECTION);
    case INSYDE_SECTION_POSTCODE:           return extended ? sizeof(POSTCODE_SECTION2) : sizeof(POSTCODE_SECTION);
    case PHOENIX_SECTION_POSTCODE:          return extended ? sizeof(POSTCODE_SECTION2) : sizeof(POSTCODE_SECTION);
    default:                                return extended ? sizeof(EFI_COMMON_SECTION_HEADER2) : sizeof(EFI_COMMON_SECTION_HEADER);
    }
}
