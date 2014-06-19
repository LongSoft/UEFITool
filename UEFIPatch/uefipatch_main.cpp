/* uefipatch_main.cpp

Copyright (c) 2014, Nikolaj Schlej. All rights reserved.
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
    a.setApplicationName("UEFIExtract");

    UEFIPatch w;
    UINT8 result = ERR_SUCCESS;
    UINT32 argumentsCount = a.arguments().length();
    
    
    if (argumentsCount == 2) {
        result = w.patchFromFile(a.arguments().at(1));
    }
    else if (argumentsCount == 5) {
        result = w.patch(a.arguments().at(1), a.arguments().at(2), a.arguments().at(3), a.arguments().at(4));
    }
    else 
        result = ERR_INVALID_PARAMETER;

    switch (result) {
    case ERR_INVALID_PARAMETER:
        std::cout << "UEFIPatch 0.1.0 - UEFI image file patching utility" << std::endl << std::endl <<
            "Usage: UEFIPatch image_file [ffs_file_guid search_pattern replace_pattern]" << std::endl << std::endl <<
            "image_file       - full or relative path to UEFI image file" << std::endl <<
            "ffs_file_guid    - GUID of FFS file to be patched" << std::endl <<
            "search_pattern   - pattern to search" << std::endl <<
            "replace_pattern  - pattern to replace" << std::endl << std::endl <<
            "If only image_file parameter is specified, patches will be read from patches.txt file";
        break;
    case ERR_SUCCESS:
        std::cout << "Image patched" << std::endl;
        break;
    case ERR_ITEM_NOT_FOUND:
        std::cout << "FFS file or search pattern not found in input file" << std::endl;
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