/* nvram.cpp
 Copyright (c) 2016, Nikolaj Schlej. All rights reserved.
 
 This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php.
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 */

#include "nvram.h"
#include "ubytearray.h"

//
// GUIDs mentioned in by nvram.h
//
extern const UByteArray NVRAM_NVAR_STORE_FILE_GUID // CEF5B9A3-476D-497F-9FDC-E98143E0422C
("\xA3\xB9\xF5\xCE\x6D\x47\x7F\x49\x9F\xDC\xE9\x81\x43\xE0\x42\x2C", 16);
extern const UByteArray NVRAM_NVAR_EXTERNAL_DEFAULTS_FILE_GUID // 9221315B-30BB-46B5-813E-1B1BF4712BD3
("\x5B\x31\x21\x92\xBB\x30\xB5\x46\x81\x3E\x1B\x1B\xF4\x71\x2B\xD3", 16);
extern const UByteArray NVRAM_NVAR_PEI_EXTERNAL_DEFAULTS_FILE_GUID // 77D3DC50-D42B-4916-AC80-8F469035D150
("\x50\xDC\xD3\x77\x2B\xD4\x16\x49\xAC\x80\x8F\x46\x90\x35\xD1\x50", 16);
extern const UByteArray NVRAM_NVAR_BB_DEFAULTS_FILE_GUID // AF516361-B4C5-436E-A7E3-A149A31B1461
("\x61\x63\x51\xAF\xC5\xB4\x6E\x43\xA7\xE3\xA1\x49\xA3\x1B\x14\x61", 16);
extern const UByteArray NVRAM_MAIN_STORE_VOLUME_GUID // FFF12B8D-7696-4C8B-A985-2747075B4F50
("\x8D\x2B\xF1\xFF\x96\x76\x8B\x4C\xA9\x85\x27\x47\x07\x5B\x4F\x50", 16);
extern const UByteArray NVRAM_ADDITIONAL_STORE_VOLUME_GUID // 00504624-8A59-4EEB-BD0F-6B36E96128E0
("\x24\x46\x50\x00\x59\x8A\xEB\x4E\xBD\x0F\x6B\x36\xE9\x61\x28\xE0", 16);
extern const UByteArray NVRAM_VSS2_AUTH_VAR_KEY_DATABASE_GUID // AAF32C78-947B-439A-A180-2E144EC37792
("\x78\x2C\xF3\xAA\x7B\x94\x9A\x43\xA1\x80\x2E\x14\x4E\xC3\x77\x92");
extern const UByteArray NVRAM_VSS2_STORE_GUID // DDCF3617-3275-4164-98B6-FE85707FFE7D
("\x17\x36\xCF\xDD\x75\x32\x64\x41\x98\xB6\xFE\x85\x70\x7F\xFE\x7D");
extern const UByteArray NVRAM_FDC_STORE_GUID // DDCF3616-3275-4164-98B6-FE85707FFE7D
("\x16\x36\xCF\xDD\x75\x32\x64\x41\x98\xB6\xFE\x85\x70\x7F\xFE\x7D");
extern const UByteArray EDKII_WORKING_BLOCK_SIGNATURE_GUID // 9E58292B-7C68-497D-0ACE6500FD9F1B95
("\x2B\x29\x58\x9E\x68\x7C\x7D\x49\x0A\xCE\x65\x00\xFD\x9F\x1B\x95", 16);
extern const UByteArray VSS2_WORKING_BLOCK_SIGNATURE_GUID // 9E58292B-7C68-497D-A0CE6500FD9F1B95
("\x2B\x29\x58\x9E\x68\x7C\x7D\x49\xA0\xCE\x65\x00\xFD\x9F\x1B\x95", 16);
extern const UByteArray NVRAM_PHOENIX_FLASH_MAP_VOLUME_HEADER // B091E7D2-05A0-4198-94F0-74B7B8C55459
("\xD2\xE7\x91\xB0\xA0\x05\x98\x41\x94\xF0\x74\xB7\xB8\xC5\x54\x59", 16);
extern const UByteArray NVRAM_PHOENIX_FLASH_MAP_MICROCODES_GUID // FD3F690E-B4B0-4D68-89DB-19A1A3318F90
("\x0E\x69\x3F\xFD\xB0\xB4\x68\x4D\x89\xDB\x19\xA1\xA3\x31\x8F\x90", 16);
extern const UByteArray NVRAM_PHOENIX_FLASH_MAP_CMDB_GUID // 46310243-7B03-4132-BE44-2243FACA7CDD
("\x43\x02\x31\x46\x03\x7B\x32\x41\xBE\x44\x22\x43\xFA\xCA\x7C\xDD", 16);
extern const UByteArray NVRAM_PHOENIX_FLASH_MAP_PUBKEY1_GUID // 1B2C4952-D778-4B64-BDA1-15A36F5FA545
("\x52\x49\x2C\x1B\x78\xD7\x64\x4B\xBD\xA1\x15\xA3\x6F\x5F\xA5\x45", 16);
extern const UByteArray NVRAM_PHOENIX_FLASH_MAP_MARKER1_GUID // 127C1C4E-9135-46E3-B006-F9808B0559A5
("\x4E\x1C\x7C\x12\x35\x91\xE3\x46\xB0\x06\xF9\x80\x8B\x05\x59\xA5", 16);
extern const UByteArray NVRAM_PHOENIX_FLASH_MAP_PUBKEY2_GUID // 7CE75114-8272-45AF-B536-761BD38852CE
("\x14\x51\xE7\x7C\x72\x82\xAF\x45\xB5\x36\x76\x1B\xD3\x88\x52\xCE", 16);
extern const UByteArray NVRAM_PHOENIX_FLASH_MAP_MARKER2_GUID // 071A3DBE-CFF4-4B73-83F0-598C13DCFDD5
("\xBE\x3D\x1A\x07\xF4\xCF\x73\x4B\x83\xF0\x59\x8C\x13\xDC\xFD\xD5", 16);
extern const UByteArray NVRAM_PHOENIX_FLASH_MAP_EVSA1_GUID // FACFB110-7BFD-4EFB-873E-88B6B23B97EA
("\x10\xB1\xCF\xFA\xFD\x7B\xFB\x4E\x87\x3E\x88\xB6\xB2\x3B\x97\xEA", 16);
extern const UByteArray NVRAM_PHOENIX_FLASH_MAP_EVSA2_GUID // E68DC11A-A5F4-4AC3-AA2E-29E298BFF645
("\x1A\xC1\x8D\xE6\xF4\xA5\xC3\x4A\xAA\x2E\x29\xE2\x98\xBF\xF6\x45", 16);
extern const UByteArray NVRAM_PHOENIX_FLASH_MAP_EVSA3_GUID // 4B3828AE-0ACE-45B6-8CDB-DAFC28BBF8C5
("\xAE\x28\x38\x4B\xCE\x0A\xB6\x45\x8C\xDB\xDA\xFC\x28\xBB\xF8\xC5", 16);
extern const UByteArray NVRAM_PHOENIX_FLASH_MAP_EVSA4_GUID // C22E6B8A-8159-49A3-B353-E84B79DF19C0
("\x8A\x6B\x2E\xC2\x59\x81\xA3\x49\xB3\x53\xE8\x4B\x79\xDF\x19\xC0", 16);
extern const UByteArray NVRAM_PHOENIX_FLASH_MAP_EVSA5_GUID // B6B5FAB9-75C4-4AAE-8314-7FFFA7156EAA
("\xB9\xFA\xB5\xB6\xC4\x75\xAE\x4A\x83\x14\x7F\xFF\xA7\x15\x6E\xAA", 16);
extern const UByteArray NVRAM_PHOENIX_FLASH_MAP_EVSA6_GUID // 919B9699-8DD0-4376-AA0B-0E54CCA47D8F
("\x99\x96\x9B\x91\xD0\x8D\x76\x43\xAA\x0B\x0E\x54\xCC\xA4\x7D\x8F", 16);
extern const UByteArray NVRAM_PHOENIX_FLASH_MAP_EVSA7_GUID // 58A90A52-929F-44F8-AC35-A7E1AB18AC91
("\x52\x0A\xA9\x58\x9F\x92\xF8\x44\xAC\x35\xA7\xE1\xAB\x18\xAC\x91", 16);
extern const UByteArray NVRAM_PHOENIX_FLASH_MAP_SELF_GUID // 8CB71915-531F-4AF5-82BF-A09140817BAA
("\x15\x19\xB7\x8C\x1F\x53\xF5\x4A\x82\xBF\xA0\x91\x40\x81\x7B\xAA", 16);

extern const UByteArray NVRAM_PHOENIX_FLASH_MAP_SIGNATURE
("\x5F\x46\x4C\x41\x53\x48\x5F\x4D\x41\x50", 10);

UString nvarAttributesToUString(const UINT8 attributes)
{
    if (attributes == 0x00 || attributes == 0xFF)
        return UString();
    
    UString str;
    if (attributes & NVRAM_NVAR_ENTRY_RUNTIME)         str += UString(", Runtime");
    if (attributes & NVRAM_NVAR_ENTRY_ASCII_NAME)      str += UString(", AsciiName");
    if (attributes & NVRAM_NVAR_ENTRY_GUID)            str += UString(", Guid");
    if (attributes & NVRAM_NVAR_ENTRY_DATA_ONLY)       str += UString(", DataOnly");
    if (attributes & NVRAM_NVAR_ENTRY_EXT_HEADER)      str += UString(", ExtHeader");
    if (attributes & NVRAM_NVAR_ENTRY_HW_ERROR_RECORD) str += UString(", HwErrorRecord");
    if (attributes & NVRAM_NVAR_ENTRY_AUTH_WRITE)      str += UString(", AuthWrite");
    if (attributes & NVRAM_NVAR_ENTRY_VALID)           str += UString(", Valid");
    
    str.remove(0, 2); // Remove first comma and space
    return str;
}

UString nvarExtendedAttributesToUString(const UINT8 attributes)
{
    UString str;
    if (attributes & NVRAM_NVAR_ENTRY_EXT_CHECKSUM)        str += UString(", Checksum");
    if (attributes & NVRAM_NVAR_ENTRY_EXT_AUTH_WRITE)      str += UString(", AuthWrite");
    if (attributes & NVRAM_NVAR_ENTRY_EXT_TIME_BASED)      str += UString(", TimeBasedAuthWrite");
    if (attributes & NVRAM_NVAR_ENTRY_EXT_UNKNOWN_MASK)    str += UString(", Unknown");
    
    str.remove(0, 2); // Remove first comma and space
    return str;
}

extern UString vssAttributesToUString(const UINT32 attributes)
{
    UString str;
    if (attributes & NVRAM_VSS_VARIABLE_NON_VOLATILE)                          str += UString(", NonVolatile");
    if (attributes & NVRAM_VSS_VARIABLE_BOOTSERVICE_ACCESS)                    str += UString(", BootService");
    if (attributes & NVRAM_VSS_VARIABLE_RUNTIME_ACCESS)                        str += UString(", Runtime");
    if (attributes & NVRAM_VSS_VARIABLE_HARDWARE_ERROR_RECORD)                 str += UString(", HwErrorRecord");
    if (attributes & NVRAM_VSS_VARIABLE_AUTHENTICATED_WRITE_ACCESS)            str += UString(", AuthWrite");
    if (attributes & NVRAM_VSS_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS) str += UString(", TimeBasedAuthWrite");
    if (attributes & NVRAM_VSS_VARIABLE_APPEND_WRITE)                          str += UString(", AppendWrite");
    if (attributes & NVRAM_VSS_VARIABLE_APPLE_DATA_CHECKSUM)                   str += UString(", AppleChecksum");
    if (attributes & NVRAM_VSS_VARIABLE_UNKNOWN_MASK)                          str += UString(", Unknown");
    
    str.remove(0, 2); // Remove first comma and space
    return str;
}

UString evsaAttributesToUString(const UINT32 attributes)
{
    UString str;
    if (attributes & NVRAM_EVSA_DATA_NON_VOLATILE)                          str += UString(", NonVolatile");
    if (attributes & NVRAM_EVSA_DATA_BOOTSERVICE_ACCESS)                    str += UString(", BootService");
    if (attributes & NVRAM_EVSA_DATA_RUNTIME_ACCESS)                        str += UString(", Runtime");
    if (attributes & NVRAM_EVSA_DATA_HARDWARE_ERROR_RECORD)                 str += UString(", HwErrorRecord");
    if (attributes & NVRAM_EVSA_DATA_AUTHENTICATED_WRITE_ACCESS)            str += UString(", AuthWrite");
    if (attributes & NVRAM_EVSA_DATA_TIME_BASED_AUTHENTICATED_WRITE_ACCESS) str += UString(", TimeBasedAuthWrite");
    if (attributes & NVRAM_EVSA_DATA_APPEND_WRITE)                          str += UString(", AppendWrite");
    if (attributes & NVRAM_EVSA_DATA_EXTENDED_HEADER)                       str += UString(", ExtendedHeader");
    if (attributes & NVRAM_EVSA_DATA_UNKNOWN_MASK)                          str += UString(", Unknown");
    
    str.remove(0, 2); // Remove first comma and space
    return str;
}

UString efiTimeToUString(const EFI_TIME & time)
{
    return usprintf("%04u-%02u-%02uT%02u:%02u:%02u.%u",
                    time.Year,
                    time.Month,
                    time.Day,
                    time.Hour,
                    time.Minute,
                    time.Second,
                    time.Nanosecond);
}

UString flashMapGuidToUString(const EFI_GUID & guid)
{
    const UByteArray baGuid((const char*)&guid, sizeof(EFI_GUID));
    if (baGuid == NVRAM_PHOENIX_FLASH_MAP_VOLUME_HEADER)        return UString("Volume header");
    if (baGuid == NVRAM_PHOENIX_FLASH_MAP_MICROCODES_GUID)      return UString("Microcodes");
    if (baGuid == NVRAM_PHOENIX_FLASH_MAP_CMDB_GUID)            return UString("CMDB");
    if (baGuid == NVRAM_PHOENIX_FLASH_MAP_PUBKEY1_GUID
        || baGuid == NVRAM_PHOENIX_FLASH_MAP_PUBKEY2_GUID)      return UString("SLIC pubkey");
    if (baGuid == NVRAM_PHOENIX_FLASH_MAP_MARKER1_GUID
        || baGuid == NVRAM_PHOENIX_FLASH_MAP_MARKER2_GUID)      return UString("SLIC marker");
    if (baGuid == NVRAM_PHOENIX_FLASH_MAP_EVSA1_GUID
        || baGuid == NVRAM_PHOENIX_FLASH_MAP_EVSA2_GUID
        || baGuid == NVRAM_PHOENIX_FLASH_MAP_EVSA3_GUID
        || baGuid == NVRAM_PHOENIX_FLASH_MAP_EVSA4_GUID
        || baGuid == NVRAM_PHOENIX_FLASH_MAP_EVSA5_GUID
        || baGuid == NVRAM_PHOENIX_FLASH_MAP_EVSA6_GUID
        || baGuid == NVRAM_PHOENIX_FLASH_MAP_EVSA7_GUID)        return UString("EVSA store");
    if (baGuid == NVRAM_PHOENIX_FLASH_MAP_SELF_GUID)            return UString("Flash map");
    return UString("Unknown");
}

