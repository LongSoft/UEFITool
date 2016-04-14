/* nvram.cpp
Copyright (c) 2016, Nikolaj Schlej. All rights reserved.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#include <QObject>
#include "nvram.h"

QString nvarAttributesToQString(const UINT8 attributes)
{
    if (attributes == 0x00 || attributes == 0xFF) 
        return QString();
    
    QString str;

    if (attributes & NVRAM_NVAR_VARIABLE_ATTRIB_RUNTIME) 
        str += QObject::tr(", Runtime");
    if (attributes & NVRAM_NVAR_VARIABLE_ATTRIB_ASCII_NAME) 
        str += QObject::tr(", AsciiName");
    if (attributes & NVRAM_NVAR_VARIABLE_ATTRIB_GUID) 
        str += QObject::tr(", Guid");
    if (attributes & NVRAM_NVAR_VARIABLE_ATTRIB_DATA_ONLY) 
        str += QObject::tr(", DataOnly");
    if (attributes & NVRAM_NVAR_VARIABLE_ATTRIB_EXT_HEADER) 
        str += QObject::tr(", ExtHeader");
    if (attributes & NVRAM_NVAR_VARIABLE_ATTRIB_HW_ERROR_RECORD) 
        str += QObject::tr(", HwErrorRecord");
    if (attributes & NVRAM_NVAR_VARIABLE_ATTRIB_AUTH_WRITE) 
        str += QObject::tr(", AuthWrite");
    if (attributes & NVRAM_NVAR_VARIABLE_ATTRIB_VALID) 
        str += QObject::tr(", Valid");
    
    return str.mid(2); // Remove first comma and space
}

QString efiTimeToQString(const EFI_TIME & time)
{
    return QObject::tr("%1-%2-%3T%4:%5:%6.%7")
        .arg(time.Year, 4, 10, QLatin1Char('0'))
        .arg(time.Month, 2, 10, QLatin1Char('0'))
        .arg(time.Day, 2, 10, QLatin1Char('0'))
        .arg(time.Hour, 2, 10, QLatin1Char('0'))
        .arg(time.Minute, 2, 10, QLatin1Char('0'))
        .arg(time.Second, 2, 10, QLatin1Char('0'))
        .arg(time.Nanosecond);
}

QString flashMapGuidToQString(const EFI_GUID & guid)
{
    const QByteArray baGuid((const char*)&guid, sizeof(EFI_GUID));
    if (baGuid == NVRAM_PHOENIX_FLASH_MAP_VOLUME_HEADER)
        return QObject::tr("Volume header");
    if (baGuid == NVRAM_PHOENIX_FLASH_MAP_MICROCODES_GUID)
        return QObject::tr("Microcodes");
    if (baGuid == NVRAM_PHOENIX_FLASH_MAP_CMDB_GUID)
        return QObject::tr("CMDB");
    if (baGuid == NVRAM_PHOENIX_FLASH_MAP_PUBKEY1_GUID || baGuid == NVRAM_PHOENIX_FLASH_MAP_PUBKEY2_GUID)
        return QObject::tr("SLIC pubkey");
    if (baGuid == NVRAM_PHOENIX_FLASH_MAP_MARKER1_GUID || baGuid == NVRAM_PHOENIX_FLASH_MAP_MARKER2_GUID)
        return QObject::tr("SLIC marker");
    if (baGuid == NVRAM_PHOENIX_FLASH_MAP_EVSA1_GUID || 
        baGuid == NVRAM_PHOENIX_FLASH_MAP_EVSA2_GUID ||
        baGuid == NVRAM_PHOENIX_FLASH_MAP_EVSA3_GUID || 
        baGuid == NVRAM_PHOENIX_FLASH_MAP_EVSA4_GUID || 
        baGuid == NVRAM_PHOENIX_FLASH_MAP_EVSA5_GUID ||
        baGuid == NVRAM_PHOENIX_FLASH_MAP_EVSA6_GUID ||
        baGuid == NVRAM_PHOENIX_FLASH_MAP_EVSA7_GUID)
        return QObject::tr("EVSA store");
    if (baGuid == NVRAM_PHOENIX_FLASH_MAP_SELF_GUID)
        return QObject::tr("Flash map");
    return QString();
}

