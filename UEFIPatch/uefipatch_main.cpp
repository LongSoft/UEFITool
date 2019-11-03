/* uefipatch_main.cpp

Copyright (c) 2018, LongSoft. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/
#include <QCoreApplication>
#include <QString>
#include <QStringList>
#include <iostream>
#include <string>

#include "../version.h"
#include "uefipatch.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    a.setOrganizationName("LongSoft");
    a.setOrganizationDomain("longsoft.me");
    a.setApplicationName("UEFIPatch");

    UEFIPatch w;
    UINT8 result = ERR_SUCCESS;
    const QStringList &args = a.arguments();
    UINT32 argumentsCount = args.length();

    if (argumentsCount < 2) {
        std::cout << "UEFIPatch " PROGRAM_VERSION " - UEFI image file patching utility" << std::endl << std::endl <<
            "Usage: UEFIPatch image_file [patches.txt] [-o output]" << std::endl <<
            "Usage: UEFIPatch image_file [-p \"Guid SectionType Patch\"] [-o output]" << std::endl << std::endl <<
            "Patches will be read from patches.txt file by default\n";
        return ERR_SUCCESS;
    }

    QString inputPath = a.arguments().at(1);
    QString patches = "patches.txt";
    QString outputPath = inputPath + ".patched";
    int patchFrom = PATCH_FROM_FILE;
    for (UINT32 i = 2; i < argumentsCount; i++) {
        if ((args.at(i) == "-p" || args.at(i) == "--patch") && i + 1 < argumentsCount) {
            patchFrom = PATCH_FROM_ARG;
            patches = args.at(i+1);
            i++;
        } else if ((args.at(i) == "-o" || args.at(i) == "--output") && i + 1 < argumentsCount) {
            outputPath = args.at(i+1);
            i++;
        } else if (patches == "patches.txt") {
            patches = args.at(i);
        } else {
            result = ERR_INVALID_PARAMETER;
        }
    }

    if (result == ERR_SUCCESS) {
        if (patchFrom == PATCH_FROM_FILE) {
            result = w.patchFromFile(inputPath, patches, outputPath);
        } else if (patchFrom == PATCH_FROM_ARG) {
            result = w.patchFromArg(inputPath, patches, outputPath);
        }
    }

    switch (result) {
    case ERR_SUCCESS:
        std::cout << "Image patched" << std::endl;
        break;
    case ERR_INVALID_PARAMETER:
        std::cout << "Function called with invalid parameter" << std::endl;
        break;
    case ERR_NOTHING_TO_PATCH:
        std::cout << "No patches can be applied to input file" << std::endl;
        break;
    case ERR_UNKNOWN_PATCH_TYPE:
        std::cout << "Unknown patch type" << std::endl;
        break;
    case ERR_PATCH_OFFSET_OUT_OF_BOUNDS:
        std::cout << "Patch offset out of bounds" << std::endl;
        break;
    case ERR_INVALID_SYMBOL:
        std::cout << "Pattern format mismatch" << std::endl;
        break;
    case ERR_INVALID_FILE:
        std::cout << patches.toStdString() << " file not found or can't be read" << std::endl;
        break;
    case ERR_FILE_OPEN:
        std::cout << "Input file not found" << std::endl;
        break;
    case ERR_FILE_READ:
        std::cout << "Input file can't be read" << std::endl;
        break;
    case ERR_FILE_WRITE:
        std::cout << "Output file can't be written" << std::endl;
        break;
    default:
        std::cout << "Error " << result << std::endl;
    }

    return result;
}
