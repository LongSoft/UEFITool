/* ffs.cpp

Copyright (c) 2015, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHWARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*/

#include <cstdio>

#include "ffs.h"
#include "guiddatabase.h"

// This is a workaround for the lack of static std::vector initializer before C++11
const UByteArray FFSv2VolumesInt[] = {
    EFI_FIRMWARE_FILE_SYSTEM_GUID,
    EFI_FIRMWARE_FILE_SYSTEM2_GUID,
    EFI_APPLE_AUTHENTICATION_FV_GUID,
    EFI_APPLE_IMMUTABLE_FV_GUID,
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
    ffsSize[2] = (UINT8)((size) >> 16U);
    ffsSize[1] = (UINT8)((size) >> 8U);
    ffsSize[0] = (UINT8)((size));
}

UINT32 uint24ToUint32(const UINT8* ffsSize)
{
    return (UINT32) ffsSize[0]
    + ((UINT32) ffsSize[1] << 8U)
    + ((UINT32) ffsSize[2] << 16U);
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


bool ustringToGuid(const UString & str, EFI_GUID & guid)
{
    unsigned long p0;
    unsigned p1, p2, p3, p4, p5, p6, p7, p8, p9, p10;

    int err = std::sscanf(str.toLocal8Bit(), "%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
                          &p0, &p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, &p9, &p10);
    if (err == 0)
        return false;

    guid.Data1 = (UINT32)p0;
    guid.Data2 = (UINT16)p1;
    guid.Data3 = (UINT16)p2;
    guid.Data4[0] = (UINT8)p3;
    guid.Data4[1] = (UINT8)p4;
    guid.Data4[2] = (UINT8)p5;
    guid.Data4[3] = (UINT8)p6;
    guid.Data4[4] = (UINT8)p7;
    guid.Data4[5] = (UINT8)p8;
    guid.Data4[6] = (UINT8)p9;
    guid.Data4[7] = (UINT8)p10;

    return true;
}

UString fileTypeToUString(const UINT8 type)
{
    switch (type) {
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
        default:                                    return usprintf("Unknown %u", type);
    };
}

UString sectionTypeToUString(const UINT8 type)
{
    switch (type) {
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
        default:                                    return usprintf("Unknown %u", type);
    }
}

UString bpdtEntryTypeToUString(const UINT16 type)
{
    switch (type) {
        case BPDT_ENTRY_TYPE_OEM_SMIP:           return UString("OEM SMIP");
        case BPDT_ENTRY_TYPE_OEM_RBE:            return UString("CSE RBE");
        case BPDT_ENTRY_TYPE_CSE_BUP:            return UString("CSE BUP");
        case BPDT_ENTRY_TYPE_UCODE:              return UString("uCode");
        case BPDT_ENTRY_TYPE_IBB:                return UString("IBB");
        case BPDT_ENTRY_TYPE_SBPDT:              return UString("S-BPDT");
        case BPDT_ENTRY_TYPE_OBB:                return UString("OBB");
        case BPDT_ENTRY_TYPE_CSE_MAIN:           return UString("CSE Main");
        case BPDT_ENTRY_TYPE_ISH:                return UString("ISH");
        case BPDT_ENTRY_TYPE_CSE_IDLM:           return UString("CSE IDLM");
        case BPDT_ENTRY_TYPE_IFP_OVERRIDE:       return UString("IFP Override");
        case BPDT_ENTRY_TYPE_DEBUG_TOKENS:       return UString("Debug Tokens");
        case BPDT_ENTRY_TYPE_USF_PHY_CONFIG:     return UString("USF Phy Config");
        case BPDT_ENTRY_TYPE_USB_GPP_LUN_ID:     return UString("USF GPP LUN ID");
        case BPDT_ENTRY_TYPE_PMC:                return UString("PMC");
        case BPDT_ENTRY_TYPE_IUNIT:              return UString("iUnit");
        case BPDT_ENTRY_TYPE_NVM_CONFIG:         return UString("NVM Config");
        case BPDT_ENTRY_TYPE_UEP:                return UString("UEP");
        case BPDT_ENTRY_TYPE_WLAN_UCODE:         return UString("WLAN uCode");
        case BPDT_ENTRY_TYPE_LOCL_SPRITES:       return UString("LOCL Sprites");
        case BPDT_ENTRY_TYPE_OEM_KEY_MANIFEST:   return UString("OEM Key Manifest");
        case BPDT_ENTRY_TYPE_DEFAULTS:           return UString("Defaults");
        case BPDT_ENTRY_TYPE_PAVP:               return UString("PAVP");
        case BPDT_ENTRY_TYPE_TCSS_FW_IOM:        return UString("TCSS FW IOM");
        case BPDT_ENTRY_TYPE_TCSS_FW_PHY:        return UString("TCSS FW PHY");
        case BPDT_ENTRY_TYPE_TBT:                return UString("TCSS TBT");
        default:                                 return usprintf("Unknown %u", type);
    }
}

UString cpdExtensionTypeToUstring(const UINT32 type)
{
    switch (type) {
        case CPD_EXT_TYPE_SYSTEM_INFO:               return UString("System Info");
        case CPD_EXT_TYPE_INIT_SCRIPT:               return UString("Init Script");
        case CPD_EXT_TYPE_FEATURE_PERMISSIONS:       return UString("Feature Permissions");
        case CPD_EXT_TYPE_PARTITION_INFO:            return UString("Partition Info");
        case CPD_EXT_TYPE_SHARED_LIB_ATTRIBUTES:     return UString("Shared Lib Attributes");
        case CPD_EXT_TYPE_PROCESS_ATTRIBUTES:        return UString("Process Attributes");
        case CPD_EXT_TYPE_THREAD_ATTRIBUTES:         return UString("Thread Attributes");
        case CPD_EXT_TYPE_DEVICE_TYPE:               return UString("Device Type");
        case CPD_EXT_TYPE_MMIO_RANGE:                return UString("MMIO Range");
        case CPD_EXT_TYPE_SPEC_FILE_PRODUCER:        return UString("Spec File Producer");
        case CPD_EXT_TYPE_MODULE_ATTRIBUTES:         return UString("Module Attributes");
        case CPD_EXT_TYPE_LOCKED_RANGES:             return UString("Locked Ranges");
        case CPD_EXT_TYPE_CLIENT_SYSTEM_INFO:        return UString("Client System Info");
        case CPD_EXT_TYPE_USER_INFO:                 return UString("User Info");
        case CPD_EXT_TYPE_KEY_MANIFEST:              return UString("Key Manifest");
        case CPD_EXT_TYPE_SIGNED_PACKAGE_INFO:       return UString("Signed Package Info");
        case CPD_EXT_TYPE_ANTI_CLONING_SKU_ID:       return UString("Anti-cloning SKU ID");
        case CPD_EXT_TYPE_CAVS:                      return UString("cAVS");
        case CPD_EXT_TYPE_IMR_INFO:                  return UString("IMR Info");
        case CPD_EXT_TYPE_RCIP_INFO:                 return UString("RCIP Info");
        case CPD_EXT_TYPE_BOOT_POLICY:               return UString("Boot Policy");
        case CPD_EXT_TYPE_SECURE_TOKEN:              return UString("Secure Token");
        case CPD_EXT_TYPE_IFWI_PARTITION_MANIFEST:   return UString("IFWI Partition Manifest");
        case CPD_EXT_TYPE_FD_HASH:                   return UString("FD Hash");
        case CPD_EXT_TYPE_IOM_METADATA:              return UString("IOM Metadata");
        case CPD_EXT_TYPE_MGP_METADATA:              return UString("MGP Metadata");
        case CPD_EXT_TYPE_TBT_METADATA:              return UString("TBT Metadata");
        default:                                     return usprintf("Unknown %u", type);
    }
}
