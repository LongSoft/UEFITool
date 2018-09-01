/* uefireplace_main.cpp

Copyright (c) 2017, mxxxc. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <cstring>

#include "../version.h"
#include "uefireplace.h"

int main(int argc, char *argv[])
{
    UEFIReplace r;
    USTATUS result = U_SUCCESS;

    if (argc < 5) {
        std::cout << "UEFIReplace " PROGRAM_VERSION " - UEFI image file replacement utility" << std::endl << std::endl <<
            "Usage: UEFIReplace image_file guid section_type contents_file [-o output] [-all] [-asis]" << std::endl;
        return U_SUCCESS;
    }

    EFI_GUID guid;
    UString uuid = UString(argv[2]);
    char *converted = const_cast<char *>(argv[3]);
    UINT8 sectionType = (UINT8)std::strtol(argv[3], &converted, 16);
    if (converted == argv[3] || !ustringToGuid(uuid, guid)) {
        result = U_INVALID_PARAMETER;
    } else {
        UString output = UString(argv[1]) + ".patched";
        bool replaceOnce = true;
        bool replaceAsIs = false;
        for (int i = 5, sz = argc; i < sz; i++) {
            if ((!std::strcmp(argv[i], "-o") || !std::strcmp(argv[i], "--output")) && i + 1 < sz) {
                output = argv[i+1];
                i++;
            } else if (!std::strcmp(argv[i], "-all")) {
                replaceOnce = false;
            } else if (!std::strcmp(argv[i], "-asis")) {
                replaceAsIs = true;
            } else {
                result = U_INVALID_PARAMETER;
            }
        }
        if (result == U_SUCCESS) {
            result = r.replace(argv[1], guid, sectionType, argv[4], output, replaceAsIs, replaceOnce);
        }
    }

    switch (result) {
    case U_SUCCESS:
        std::cout << "File replaced" << std::endl;
        break;
    case U_INVALID_PARAMETER:
        std::cout << "Function called with invalid parameter" << std::endl;
        break;
    case U_INVALID_FILE:
        std::cout << "Invalid/corrupted file specified" << std::endl;
        break;
    case U_INVALID_SECTION:
        std::cout << "Invalid/corrupted section specified" << std::endl;
        break;
    case U_NOTHING_TO_PATCH:
        std::cout << "No replacements can be applied to input file" << std::endl;
        break;
    case U_NOT_IMPLEMENTED:
        std::cout << "Can't replace body of this section type" << std::endl;
        break;
    case U_FILE_OPEN:
        std::cout << "Input file not found" << std::endl;
        break;
    case U_FILE_READ:
        std::cout << "Input file can't be read" << std::endl;
        break;
    case U_FILE_WRITE:
        std::cout << "Output file can't be written" << std::endl;
        break;
    default:
        std::cout << "Error " << result << std::endl;
    }

    return result;
}
