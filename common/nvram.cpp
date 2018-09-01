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

