/* uefireplace_main.cpp

Copyright (c) 2017, mxxxc. All rights reserved.
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

#include "../version.h"
#include "uefireplace.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    a.setOrganizationName("LongSoft");
    a.setOrganizationDomain("longsoft.me");
    a.setApplicationName("UEFIReplace");

    UEFIReplace r;
    UINT8 result = ERR_SUCCESS;
    QStringList args = a.arguments();

    if (args.length() < 5) {
        std::cout << "UEFIReplace " PROGRAM_VERSION " - UEFI image file replacement utility" << std::endl << std::endl <<
            "Usage: UEFIReplace image_file guid section_type contents_file [-o output] [-all] [-asis]" << std::endl;
        return ERR_SUCCESS;
    }

    QUuid uuid = QUuid(args.at(2));
    QByteArray guid = QByteArray::fromRawData((const char*)&uuid.data1, sizeof(EFI_GUID));
    bool converted;
    UINT8 sectionType = (UINT8)args.at(3).toUShort(&converted, 16);
    if (!converted) {
        result = ERR_INVALID_PARAMETER;
    } else {
        QString output = args.at(1) + ".patched";
        bool replaceOnce = true;
        bool replaceAsIs = false;
        for (int i = 5, sz = args.size(); i < sz; i++) {
            if ((args.at(i) == "-o" || args.at(i) == "--output") && i + 1 < sz) {
                output = args.at(i+1);
                i++;
            } else if (args.at(i) == "-all") {
                replaceOnce = false;
            } else if (args.at(i) == "-asis") {
                replaceAsIs = true;
            } else {
                result = ERR_INVALID_PARAMETER;
            }
        }
        if (result == ERR_SUCCESS) {
            result = r.replace(args.at(1), guid, sectionType, args.at(4), output, replaceAsIs, replaceOnce);
        }
    }

    switch (result) {
    case ERR_SUCCESS:
        std::cout << "File replaced" << std::endl;
        break;
    case ERR_INVALID_PARAMETER:
        std::cout << "Function called with invalid parameter" << std::endl;
        break;
    case ERR_INVALID_FILE:
        std::cout << "Invalid/corrupted file specified" << std::endl;
        break;
    case ERR_INVALID_SECTION:
        std::cout << "Invalid/corrupted section specified" << std::endl;
        break;
    case ERR_NOTHING_TO_PATCH:
        std::cout << "No replacements can be applied to input file" << std::endl;
        break;
    case ERR_NOT_IMPLEMENTED:
        std::cout << "Can't replace body of this section type" << std::endl;
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
