/* uefifind_main.cpp

Copyright (c) 2015, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/
#include <QCoreApplication>
#include <iostream>
#include "uefifind.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    a.setOrganizationName("CodeRush");
    a.setOrganizationDomain("coderush.me");
    a.setApplicationName("UEFIFind");

    UEFIFind w;
    UINT8 result;

    if (a.arguments().length() == 5) {
        result = w.init(a.arguments().at(4));
        if (result)
            return result;

        // Get search mode
        UINT8 mode;
        if (a.arguments().at(1) == QString("header"))
            mode = SEARCH_MODE_HEADER;
        else if (a.arguments().at(1) == QString("body"))
            mode = SEARCH_MODE_BODY;
        else if (a.arguments().at(1) == QString("all"))
            mode = SEARCH_MODE_ALL;
        else
            return ERR_INVALID_PARAMETER;

        // Get result type
        bool count;
        if (a.arguments().at(2) == QString("list"))
            count = false;
        else if (a.arguments().at(2) == QString("count"))
            count = true;
        else
            return ERR_INVALID_PARAMETER;

        // Go find the supplied pattern
        QString found;
        result = w.find(mode, count, a.arguments().at(3), found);
        if (result)
            return result;

        // Nothing was found
        if (found.isEmpty())
            return ERR_ITEM_NOT_FOUND;

        // Print result
        std::cout << found.toStdString();
        return ERR_SUCCESS;
    }
    else {
        std::cout << "UEFIFind 0.3.4" << std::endl << std::endl <<
            "Usage: uefifind {header | body | all} {list | count} pattern imagefile\n";
        return ERR_INVALID_PARAMETER;
    }
}

