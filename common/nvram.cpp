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

QString variableAttributesToQstring(UINT8 attributes)
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
    
    return str.mid(2); // Remove the first comma and space
}

