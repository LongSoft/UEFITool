/* uefiextract_main.cpp

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
#include "uefiextract.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    a.setOrganizationName("CodeRush");
    a.setOrganizationDomain("coderush.me");
    a.setApplicationName("UEFIExtract");

    UEFIExtract w;
    UINT8 result = ERR_SUCCESS;
    if (a.arguments().length() > 1) {
        result = w.extractAll(a.arguments().at(1));
        switch (result) {
        case ERR_DIR_ALREADY_EXIST:
            std::cout << "Dump directory already exist, please remove it" << std::endl;
            break;
        case ERR_DIR_CREATE:
            std::cout << "Can't create directory" << std::endl;
            break;
        case ERR_FILE_OPEN:
            std::cout << "Can't create file" << std::endl;
            break;
        }
    }
    else {
        result = ERR_INVALID_PARAMETER;
        std::cout << "UEFIExtract 0.1.0" << std::endl << std::endl << 
            "Usage: uefiextract imagefile\n" << std::endl;
    }
        
    return result;
}