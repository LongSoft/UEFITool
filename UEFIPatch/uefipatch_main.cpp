/* uefipatch_main.cpp

Copyright (c) 2018, LongSoft. All rights reserved.
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
#include "uefipatch.h"

int main(int argc, char *argv[])
{
    UEFIPatch w;
    USTATUS result = U_SUCCESS;
    UINT32 argumentsCount = argc;

    if (argumentsCount < 2) {
        std::cout << "UEFIPatch " PROGRAM_VERSION " - UEFI image file patching utility" << std::endl << std::endl <<
            "Usage: UEFIPatch image_file [patches.txt] [-o output]" << std::endl << std::endl <<
            "Patches will be read from patches.txt file by default\n";
        return U_SUCCESS;
    }

    UString inputPath = argv[1];
    UString patches = "patches.txt";
    UString outputPath = inputPath + ".patched";
    for (UINT32 i = 2; i < argumentsCount; i++) {
        if ((!std::strcmp(argv[i], "-o") || !std::strcmp(argv[i], "--output")) && i + 1 < argumentsCount) {
            outputPath = argv[i+1];
            i++;
        } else if (patches == UString("patches.txt")) {
            patches = argv[i];
        } else {
            result = U_INVALID_PARAMETER;
        }
    }

    if (result == U_SUCCESS) {
        result = w.patchFromFile(inputPath, patches, outputPath);
    }

    switch (result) {
    case U_SUCCESS:
        std::cout << "Image patched" << std::endl;
        break;
    case U_INVALID_PARAMETER:
        std::cout << "Function called with invalid parameter" << std::endl;
        break;
    case U_NOTHING_TO_PATCH:
        std::cout << "No patches can be applied to input file" << std::endl;
        break;
    case U_UNKNOWN_PATCH_TYPE:
        std::cout << "Unknown patch type" << std::endl;
        break;
    case U_PATCH_OFFSET_OUT_OF_BOUNDS:
        std::cout << "Patch offset out of bounds" << std::endl;
        break;
    case U_INVALID_SYMBOL:
        std::cout << "Pattern format mismatch" << std::endl;
        break;
    case U_INVALID_FILE:
        std::cout << patches.toLocal8Bit() << " file not found or can't be read" << std::endl;
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
