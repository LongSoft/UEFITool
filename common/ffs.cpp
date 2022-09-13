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
#include "ubytearray.h"

//
// GUIDs mentioned in by ffs.h
//
// Standard FMP capsule GUID
extern const UByteArray EFI_FMP_CAPSULE_GUID // 6DCBD5ED-E82D-4C44-BDA1-7194199AD92A
("\xED\xD5\xCB\x6D\x2D\xE8\x44\x4C\xBD\xA1\x71\x94\x19\x9A\xD9\x2A", 16);
// Standard EFI capsule GUID
extern const UByteArray EFI_CAPSULE_GUID // 3B6686BD-0D76-4030-B70E-B5519E2FC5A0
("\xBD\x86\x66\x3B\x76\x0D\x30\x40\xB7\x0E\xB5\x51\x9E\x2F\xC5\xA0", 16);
// Intel capsule GUID
extern const UByteArray INTEL_CAPSULE_GUID // 539182B9-ABB5-4391-B69A-E3A943F72FCC
("\xB9\x82\x91\x53\xB5\xAB\x91\x43\xB6\x9A\xE3\xA9\x43\xF7\x2F\xCC", 16);
// Lenovo capsule GUID
extern const UByteArray LENOVO_CAPSULE_GUID // E20BAFD3-9914-4F4F-9537-3129E090EB3C
("\xD3\xAF\x0B\xE2\x14\x99\x4F\x4F\x95\x37\x31\x29\xE0\x90\xEB\x3C", 16);
// Another Lenovo capsule GUID
extern const UByteArray LENOVO2_CAPSULE_GUID // 25B5FE76-8243-4A5C-A9BD-7EE3246198B5
("\x76\xFE\xB5\x25\x43\x82\x5C\x4A\xA9\xBD\x7E\xE3\x24\x61\x98\xB5", 16);
// Toshiba capsule GUID
extern const UByteArray TOSHIBA_CAPSULE_GUID // 3BE07062-1D51-45D2-832B-F093257ED461
("\x62\x70\xE0\x3B\x51\x1D\xD2\x45\x83\x2B\xF0\x93\x25\x7E\xD4\x61", 16);
// AMI Aptio signed extended capsule GUID
extern const UByteArray APTIO_SIGNED_CAPSULE_GUID // 4A3CA68B-7723-48FB-803D-578CC1FEC44D
("\x8B\xA6\x3C\x4A\x23\x77\xFB\x48\x80\x3D\x57\x8C\xC1\xFE\xC4\x4D", 16);
// AMI Aptio unsigned extended capsule GUID
extern const UByteArray APTIO_UNSIGNED_CAPSULE_GUID // 14EEBB90-890A-43DB-AED1-5D3C4588A418
("\x90\xBB\xEE\x14\x0A\x89\xDB\x43\xAE\xD1\x5D\x3C\x45\x88\xA4\x18", 16);
// Standard file system GUIDs
extern const UByteArray EFI_FIRMWARE_FILE_SYSTEM_GUID // 7A9354D9-0468-444A-81CE-0BF617D890DF
("\xD9\x54\x93\x7A\x68\x04\x4A\x44\x81\xCE\x0B\xF6\x17\xD8\x90\xDF", 16);
extern const UByteArray EFI_FIRMWARE_FILE_SYSTEM2_GUID // 8C8CE578-8A3D-4F1C-9935-896185C32DD3
("\x78\xE5\x8C\x8C\x3D\x8A\x1C\x4F\x99\x35\x89\x61\x85\xC3\x2D\xD3", 16);
extern const UByteArray EFI_FIRMWARE_FILE_SYSTEM3_GUID // 5473C07A-3DCB-4DCA-BD6F-1E9689E7349A
("\x7A\xC0\x73\x54\xCB\x3D\xCA\x4D\xBD\x6F\x1E\x96\x89\xE7\x34\x9A", 16);
// Vendor-specific file system GUIDs
extern const UByteArray EFI_APPLE_IMMUTABLE_FV_GUID // 04ADEEAD-61FF-4D31-B6BA-64F8BF901F5A
("\xAD\xEE\xAD\x04\xFF\x61\x31\x4D\xB6\xBA\x64\xF8\xBF\x90\x1F\x5A", 16);
extern const UByteArray EFI_APPLE_AUTHENTICATION_FV_GUID // BD001B8C-6A71-487B-A14F-0C2A2DCF7A5D
("\x8C\x1B\x00\xBD\x71\x6A\x7B\x48\xA1\x4F\x0C\x2A\x2D\xCF\x7A\x5D", 16);
extern const UByteArray EFI_APPLE_MICROCODE_VOLUME_GUID // 153D2197-29BD-44DC-AC59-887F70E41A6B
("\x97\x21\x3D\x15\xBD\x29\xDC\x44\xAC\x59\x88\x7F\x70\xE4\x1A\x6B", 16);
extern const UByteArray EFI_INTEL_FILE_SYSTEM_GUID // AD3FFFFF-D28B-44C4-9F13-9EA98A97F9F0
("\xFF\xFF\x3F\xAD\x8B\xD2\xC4\x44\x9F\x13\x9E\xA9\x8A\x97\xF9\xF0", 16);
extern const UByteArray EFI_INTEL_FILE_SYSTEM2_GUID // D6A1CD70-4B33-4994-A6EA-375F2CCC5437
("\x70\xCD\xA1\xD6\x33\x4B\x94\x49\xA6\xEA\x37\x5F\x2C\xCC\x54\x37", 16);
extern const UByteArray EFI_SONY_FILE_SYSTEM_GUID // 4F494156-AED6-4D64-A537-B8A5557BCEEC
("\x56\x41\x49\x4F\xD6\xAE\x64\x4D\xA5\x37\xB8\xA5\x55\x7B\xCE\xEC", 16);
// PEI apriori file
extern const UByteArray EFI_PEI_APRIORI_FILE_GUID // 1B45CC0A-156A-428A-AF62-49864DA0E6E6
("\x0A\xCC\x45\x1B\x6A\x15\x8A\x42\xAF\x62\x49\x86\x4D\xA0\xE6\xE6", 16);
// DXE apriori file
extern const UByteArray EFI_DXE_APRIORI_FILE_GUID // FC510EE7-FFDC-11D4-BD41-0080C73C8881
("\xE7\x0E\x51\xFC\xDC\xFF\xD4\x11\xBD\x41\x00\x80\xC7\x3C\x88\x81", 16);
// Volume top file
extern const UByteArray EFI_FFS_VOLUME_TOP_FILE_GUID // 1BA0062E-C779-4582-8566-336AE8F78F09
("\x2E\x06\xA0\x1B\x79\xC7\x82\x45\x85\x66\x33\x6A\xE8\xF7\x8F\x09", 16);
// Padding file GUID
extern const UByteArray EFI_FFS_PAD_FILE_GUID // E4536585-7909-4A60-B5C6-ECDEA6EBFB5
("\x85\x65\x53\xE4\x09\x79\x60\x4A\xB5\xC6\xEC\xDE\xA6\xEB\xFB\x54", 16);
// AMI DXE core file
extern const UByteArray AMI_CORE_DXE_GUID // 5AE3F37E-4EAE-41AE-8240-35465B5E81EB
("\x7E\xF3\xE3\x5A\xAE\x4E\xAE\x41\x82\x40\x35\x46\x5B\x5E\x81\xEB", 16);
// EDK2 DXE core file
extern const UByteArray EFI_DXE_CORE_GUID // D6A2CB7F-6A18-4E2F-B43B-9920A733700A
("\x7F\xCB\xA2\xD6\x18\x6A\x2F\x4E\xB4\x3B\x99\x20\xA7\x33\x70\x0A", 16);
// GUIDs of GUID-defined sections
extern const UByteArray EFI_GUIDED_SECTION_CRC32 // FC1BCDB0-7D31-49AA-936A-A4600D9DD083
("\xB0\xCD\x1B\xFC\x31\x7D\xAA\x49\x93\x6A\xA4\x60\x0D\x9D\xD0\x83", 16);
extern const UByteArray EFI_GUIDED_SECTION_TIANO // A31280AD-481E-41B6-95E8-127F4C984779
("\xAD\x80\x12\xA3\x1E\x48\xB6\x41\x95\xE8\x12\x7F\x4C\x98\x47\x79", 16);
extern const UByteArray EFI_GUIDED_SECTION_LZMA // EE4E5898-3914-4259-9D6E-DC7BD79403CF
("\x98\x58\x4E\xEE\x14\x39\x59\x42\x9D\x6E\xDC\x7B\xD7\x94\x03\xCF", 16);
extern const UByteArray EFI_GUIDED_SECTION_LZMA_HP // 0ED85E23-F253-413F-A03C-901987B04397
("\x23\x5E\xD8\x0E\x53\xF2\x3F\x41\xA0\x3C\x90\x19\x87\xB0\x43\x97", 16);
extern const UByteArray EFI_GUIDED_SECTION_LZMAF86 // D42AE6BD-1352-4BFB-909A-CA72A6EAE889
("\xBD\xE6\x2A\xD4\x52\x13\xFB\x4B\x90\x9A\xCA\x72\xA6\xEA\xE8\x89", 16);
extern const UByteArray EFI_GUIDED_SECTION_GZIP // 1D301FE9-BE79-4353-91C2-D23BC959AE0C
("\xE9\x1F\x30\x1D\x79\xBE\x53\x43\x91\xC2\xD2\x3B\xC9\x59\xAE\x0C", 16);
extern const UByteArray EFI_FIRMWARE_CONTENTS_SIGNED_GUID // 0F9D89E8-9259-4F76-A5AF-0C89E34023DF
("\xE8\x89\x9D\x0F\x59\x92\x76\x4F\xA5\xAF\x0C\x89\xE3\x40\x23\xDF", 16);
extern const UByteArray EFI_CERT_TYPE_RSA2048_SHA256_GUID // A7717414-C616-4977-9420-844712A735BF
 ("\x14\x74\x71\xA7\x16\xC6\x77\x49\x94\x20\x84\x47\x12\xA7\x35\xBF");
extern const UByteArray EFI_HASH_ALGORITHM_SHA256_GUID // 51AA59DE-FDF2-4EA3-BC63-875FB7842EE9
("\xde\x59\xAA\x51\xF2\xFD\xA3\x4E\xBC\x63\x87\x5F\xB7\x84\x2E\xE9");

// Protected range files
extern const UByteArray PROTECTED_RANGE_VENDOR_HASH_FILE_GUID_PHOENIX // 389CC6F2-1EA8-467B-AB8A-78E769AE2A15
("\xF2\xC6\x9C\x38\xA8\x1E\x7B\x46\xAB\x8A\x78\xE7\x69\xAE\x2A\x15", 16);
extern const UByteArray PROTECTED_RANGE_VENDOR_HASH_FILE_GUID_AMI // CBC91F44-A4BC-4A5B-8696-703451D0B053
("\x44\x1F\xC9\xCB\xBC\xA4\x5B\x4A\x86\x96\x70\x34\x51\xD0\xB0\x53", 16);

// AMI ROM Hole files
extern const UByteArray AMI_ROM_HOLE_FILE_GUID_0 //05CA01FC-0FC1-11DC-9011-00173153EBA8
("\xFC\x01\xCA\x05\xC1\x0F\xDC\x11\x90\x11\x00\x17\x31\x53\xEB\xA8", 16);
extern const UByteArray AMI_ROM_HOLE_FILE_GUID_1 //05CA01FD-0FC1-11DC-9011-00173153EBA8
("\xFD\x01\xCA\x05\xC1\x0F\xDC\x11\x90\x11\x00\x17\x31\x53\xEB\xA8", 16);
extern const UByteArray AMI_ROM_HOLE_FILE_GUID_2 //05CA01FE-0FC1-11DC-9011-00173153EBA8
("\xFE\x01\xCA\x05\xC1\x0F\xDC\x11\x90\x11\x00\x17\x31\x53\xEB\xA8", 16);
extern const UByteArray AMI_ROM_HOLE_FILE_GUID_3 //05CA01FF-0FC1-11DC-9011-00173153EBA8
("\xFF\x01\xCA\x05\xC1\x0F\xDC\x11\x90\x11\x00\x17\x31\x53\xEB\xA8", 16);
extern const UByteArray AMI_ROM_HOLE_FILE_GUID_4 //05CA0200-0FC1-11DC-9011-00173153EBA8
("\x00\x02\xCA\x05\xC1\x0F\xDC\x11\x90\x11\x00\x17\x31\x53\xEB\xA8", 16);
extern const UByteArray AMI_ROM_HOLE_FILE_GUID_5 //05CA0201-0FC1-11DC-9011-00173153EBA8
("\x01\x02\xCA\x05\xC1\x0F\xDC\x11\x90\x11\x00\x17\x31\x53\xEB\xA8", 16);
extern const UByteArray AMI_ROM_HOLE_FILE_GUID_6 //05CA0202-0FC1-11DC-9011-00173153EBA8
("\x02\x02\xCA\x05\xC1\x0F\xDC\x11\x90\x11\x00\x17\x31\x53\xEB\xA8", 16);
extern const UByteArray AMI_ROM_HOLE_FILE_GUID_7 //05CA0203-0FC1-11DC-9011-00173153EBA8
("\x03\x02\xCA\x05\xC1\x0F\xDC\x11\x90\x11\x00\x17\x31\x53\xEB\xA8", 16);
extern const UByteArray AMI_ROM_HOLE_FILE_GUID_8 //05CA0204-0FC1-11DC-9011-00173153EBA8
("\x04\x02\xCA\x05\xC1\x0F\xDC\x11\x90\x11\x00\x17\x31\x53\xEB\xA8", 16);
extern const UByteArray AMI_ROM_HOLE_FILE_GUID_9 //05CA0205-0FC1-11DC-9011-00173153EBA8
("\x05\x02\xCA\x05\xC1\x0F\xDC\x11\x90\x11\x00\x17\x31\x53\xEB\xA8", 16);
extern const UByteArray AMI_ROM_HOLE_FILE_GUID_10 //05CA0206-0FC1-11DC-9011-00173153EBA8
("\x06\x02\xCA\x05\xC1\x0F\xDC\x11\x90\x11\x00\x17\x31\x53\xEB\xA8", 16);
extern const UByteArray AMI_ROM_HOLE_FILE_GUID_11 //05CA0207-0FC1-11DC-9011-00173153EBA8
("\x07\x02\xCA\x05\xC1\x0F\xDC\x11\x90\x11\x00\x17\x31\x53\xEB\xA8", 16);
extern const UByteArray AMI_ROM_HOLE_FILE_GUID_12 //05CA0208-0FC1-11DC-9011-00173153EBA8
("\x08\x02\xCA\x05\xC1\x0F\xDC\x11\x90\x11\x00\x17\x31\x53\xEB\xA8", 16);
extern const UByteArray AMI_ROM_HOLE_FILE_GUID_13 //05CA0209-0FC1-11DC-9011-00173153EBA8
("\x09\x02\xCA\x05\xC1\x0F\xDC\x11\x90\x11\x00\x17\x31\x53\xEB\xA8", 16);
extern const UByteArray AMI_ROM_HOLE_FILE_GUID_14 //05CA020A-0FC1-11DC-9011-00173153EBA8
("\x0A\x02\xCA\x05\xC1\x0F\xDC\x11\x90\x11\x00\x17\x31\x53\xEB\xA8", 16);
extern const UByteArray AMI_ROM_HOLE_FILE_GUID_15 //05CA020B-0FC1-11DC-9011-00173153EBA8
("\x0B\x02\xCA\x05\xC1\x0F\xDC\x11\x90\x11\x00\x17\x31\x53\xEB\xA8", 16);

const std::vector<UByteArray> FFSv2Volumes({
    EFI_FIRMWARE_FILE_SYSTEM_GUID,
    EFI_FIRMWARE_FILE_SYSTEM2_GUID,
    EFI_APPLE_AUTHENTICATION_FV_GUID,
    EFI_APPLE_IMMUTABLE_FV_GUID,
    EFI_INTEL_FILE_SYSTEM_GUID,
    EFI_INTEL_FILE_SYSTEM2_GUID,
    EFI_SONY_FILE_SYSTEM_GUID
});

const std::vector<UByteArray> FFSv3Volumes({EFI_FIRMWARE_FILE_SYSTEM3_GUID});

const UINT8 ffsAlignmentTable[] =
{ 0, 4, 7, 9, 10, 12, 15, 16 };

const UINT8 ffsAlignment2Table[] =
{ 17, 18, 19, 20, 21, 22, 23, 24 };

extern const UByteArray RECOVERY_STARTUP_AP_DATA_X86_128K // jmp far F000:FFD0, EAD0FF00F0
("\xEA\xD0\xFF\x00\xF0\x00\x00\x00\x00\x00\x00\x00\x00\x00\x27\x2D", RECOVERY_STARTUP_AP_DATA_X86_SIZE);

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
    };
    return usprintf("Unknown %02Xh", type);
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
    }
    return usprintf("Unknown %02Xh", type);
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
        case BPDT_ENTRY_TYPE_USF_GPP_LUN_ID:     return UString("USF GPP LUN ID");
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
        case BPDT_ENTRY_TYPE_USB_PHY:            return UString("USB PHY");
        case BPDT_ENTRY_TYPE_PCHC:               return UString("PCHC");
        case BPDT_ENTRY_TYPE_SAMF:               return UString("SAMF");
        case BPDT_ENTRY_TYPE_PPHY:               return UString("PPHY");
    }
    return usprintf("Unknown %04Xh", type);
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
        case CPD_EXT_TYPE_GMF_CERTIFICATE:           return UString("Golden Measurement File Certificate");
        case CPD_EXT_TYPE_GMF_BODY:                  return UString("Golden Measurement File Body");
        case CPD_EXT_TYPE_KEY_MANIFEST_EXT:          return UString("Extended Key Manifest");
        case CPD_EXT_TYPE_SIGNED_PACKAGE_INFO_EXT:   return UString("Extended Signed Package Info");
        case CPD_EXT_TYPE_SPS_PLATFORM_ID:           return UString("SPS Platform ID");
    }
    return usprintf("Unknown %08Xh", type);
}
