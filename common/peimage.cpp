/* peimage.cpp

Copyright (c) 2015, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#include "peimage.h"

UString machineTypeToUString(UINT16 machineType)
{
    switch (machineType) {
    case EFI_IMAGE_FILE_MACHINE_AMD64:       return UString("x86-64");
    case EFI_IMAGE_FILE_MACHINE_ARM:         return UString("ARM");
    case EFI_IMAGE_FILE_MACHINE_ARMNT:       return UString("ARMv7");
    case EFI_IMAGE_FILE_MACHINE_APPLE_ARM:   return UString("Apple ARM");
    case EFI_IMAGE_FILE_MACHINE_AARCH64:     return UString("AArch64");
    case EFI_IMAGE_FILE_MACHINE_EBC:         return UString("EBC");
    case EFI_IMAGE_FILE_MACHINE_I386:        return UString("x86");
    case EFI_IMAGE_FILE_MACHINE_IA64:        return UString("IA64");
    case EFI_IMAGE_FILE_MACHINE_POWERPC:     return UString("PowerPC");
    case EFI_IMAGE_FILE_MACHINE_POWERPCFP:   return UString("PowerPC FP");
    case EFI_IMAGE_FILE_MACHINE_THUMB:       return UString("ARM Thumb");
    case EFI_IMAGE_FILE_MACHINE_RISCV32:     return UString("RISC-V 32-bit");
    case EFI_IMAGE_FILE_MACHINE_RISCV64:     return UString("RISC-V 64-bit");
    case EFI_IMAGE_FILE_MACHINE_RISCV128:    return UString("RISC-V 128-bit");
    default:                                 return usprintf("Unknown (%04Xh)", machineType);
    }
}