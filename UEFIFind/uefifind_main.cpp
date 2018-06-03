/* uefifind_main.cpp

Copyright (c) 2018, LongSoft. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/
#include <QCoreApplication>
#include <iostream>

#include "../version.h"
#include "uefifind.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    a.setOrganizationName("LongSoft");
    a.setOrganizationDomain("longsoft.me");
    a.setApplicationName("UEFIFind");

    UEFIFind w;
    UINT8 result;

    if (a.arguments().length() == 5) {
        QString inputArg = a.arguments().at(1);
        QString modeArg = a.arguments().at(2);
        QString subModeArg = a.arguments().at(3);
        QString patternArg = a.arguments().at(4);

        // Get search mode
        UINT8 mode;
        if (modeArg == QString("header"))
            mode = SEARCH_MODE_HEADER;
        else if (modeArg == QString("body"))
            mode = SEARCH_MODE_BODY;
        else if (modeArg == QString("all"))
            mode = SEARCH_MODE_ALL;
        else
            return U_INVALID_PARAMETER;

        // Get result type
        bool count;
        if (subModeArg == QString("list"))
            count = false;
        else if (subModeArg == QString("count"))
            count = true;
        else
            return U_INVALID_PARAMETER;

        // Parse input file
        result = w.init(inputArg);
        if (result)
            return result;

        // Go find the supplied pattern
        QString found;
        result = w.find(mode, count, patternArg, found);
        if (result)
            return result;

        // Nothing is found
        if (found.isEmpty())
            return U_ITEM_NOT_FOUND;

        // Print result
        std::cout << found.toStdString();
        return U_SUCCESS;
    }
    else if (a.arguments().length() == 4) {
        QString inputArg = a.arguments().at(1);
        QString modeArg = a.arguments().at(2);
        QString patternArg = a.arguments().at(3);

        // Get search mode
        if (modeArg != QString("file"))
            return U_INVALID_PARAMETER;

        // Open patterns file
        QFileInfo fileInfo(patternArg);
        if (!fileInfo.exists())
            return U_FILE_OPEN;

        QFile patternsFile;
        patternsFile.setFileName(patternArg);
        if (!patternsFile.open(QFile::ReadOnly))
            return U_FILE_OPEN;

        // Parse input file
        result = w.init(inputArg);
        if (result)
            return result;

        // Perform searches
        bool somethingFound = false;
        while (!patternsFile.atEnd()) {
            QByteArray line = patternsFile.readLine();
            // Use sharp symbol as commentary
            if (line.count() == 0 || line[0] == '#')
                continue;

            // Split the read line
            QList<QByteArray> list = line.split(' ');
            if (list.count() < 3) {
                std::cout << line.constData() << "skipped, too few arguments" << std::endl << std::endl;
                continue;
            }
            // Get search mode
            UINT8 mode;
            if (list.at(0) == QString("header"))
                mode = SEARCH_MODE_HEADER;
            else if (list.at(0) == QString("body"))
                mode = SEARCH_MODE_BODY;
            else if (list.at(0) == QString("all"))
                mode = SEARCH_MODE_ALL;
            else {
                std::cout << line.constData() << "skipped, invalid search mode" << std::endl << std::endl;
                continue;
            }

            // Get result type
            bool count;
            if (list.at(1) == QString("list"))
                count = false;
            else if (list.at(1) == QString("count"))
                count = true;
            else {
                std::cout << line.constData() << "skipped, invalid result type" << std::endl << std::endl;
                continue;
            }

            // Go find the supplied pattern
            QString found;
            result = w.find(mode, count, list.at(2), found);
            if (result) {
                std::cout << line.constData() << "skipped, find failed with error " << (UINT32)result << std::endl << std::endl;
                continue;
            }

            if (found.isEmpty()) {
                // Nothing is found
                std::cout << line.constData() << "nothing found" << std::endl << std::endl;
            }
            else {
                // Print result
                std::cout << line.constData() << found.toStdString() << std::endl;
                somethingFound = true;
            }
        }

        // Nothing is found
        if (!somethingFound)
            return U_ITEM_NOT_FOUND;

        return U_SUCCESS;
    }
    else {
        std::cout << "UEFIFind " PROGRAM_VERSION << std::endl << std::endl <<
            "Usage: UEFIFind imagefile {header | body | all} {list | count} pattern" << std::endl <<
            "    or UEFIFind imagefile file patternsfile" << std::endl;
        return U_INVALID_PARAMETER;
    }
}

