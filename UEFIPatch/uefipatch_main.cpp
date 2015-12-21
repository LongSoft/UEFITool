/* uefipatch_main.cpp

Copyright (c) 2015, Nikolaj Schlej. All rights reserved.
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
#include "uefipatch.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    a.setOrganizationName("CodeRush");
    a.setOrganizationDomain("coderush.me");
    a.setApplicationName("UEFIPatch");

    UEFIPatch w;
    UINT8 result = ERR_SUCCESS;
    UINT32 argumentsCount = a.arguments().length();
    
    if (argumentsCount == 2) {
        result = w.patchFromFile(a.arguments().at(1));
    }
    else {
        std::cout << "UEFIPatch 0.3.9 - UEFI image file patching utility" << std::endl << std::endl <<
            "Usage: UEFIPatch image_file" << std::endl << std::endl <<
            "Patches will be read from patches.txt file\n";
        return ERR_SUCCESS;
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
        std::cout << "patches.txt file not found or can't be read" << std::endl;
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
