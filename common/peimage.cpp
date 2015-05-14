/* peimage.cpp

Copyright (c) 2015, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#include <QObject>

#include "peimage.h"

QString machineTypeToQString(UINT16 machineType)
{
    switch (machineType) {
    case EFI_IMAGE_FILE_MACHINE_AMD64:     return QObject::tr("x86-64");
    case EFI_IMAGE_FILE_MACHINE_ARM:       return QObject::tr("ARM");
    case EFI_IMAGE_FILE_MACHINE_ARMNT:     return QObject::tr("ARMv7");
    case EFI_IMAGE_FILE_MACHINE_ARM64:     return QObject::tr("ARM64");
    case EFI_IMAGE_FILE_MACHINE_EBC:       return QObject::tr("EBC");
    case EFI_IMAGE_FILE_MACHINE_I386:      return QObject::tr("x86");
    case EFI_IMAGE_FILE_MACHINE_IA64:      return QObject::tr("IA64");
    case EFI_IMAGE_FILE_MACHINE_POWERPC:   return QObject::tr("PowerPC");
    case EFI_IMAGE_FILE_MACHINE_POWERPCFP: return QObject::tr("PowerPC FP");
    case EFI_IMAGE_FILE_MACHINE_THUMB:     return QObject::tr("Thumb");
    default:                               return QObject::tr("Unknown %1h").hexarg2(machineType, 4);
    }
}