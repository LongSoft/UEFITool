/* uefifind_main.cpp

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

#include "../version.h"
#include "../common/guiddatabase.h"
#include "uefifind.h"

int main(int argc, char *argv[])
{
    UEFIFind w;
    USTATUS result;

    initGuidDatabase("guids.csv");

    if (argc == 5) {
        UString inputArg = argv[1];
        UString modeArg = argv[2];
        UString subModeArg = argv[3];
        UString patternArg = argv[4];

        // Get search mode
        UINT8 mode;
        if (modeArg == UString("header"))
            mode = SEARCH_MODE_HEADER;
        else if (modeArg == UString("body"))
            mode = SEARCH_MODE_BODY;
        else if (modeArg == UString("all"))
            mode = SEARCH_MODE_ALL;
        else
            return U_INVALID_PARAMETER;

        // Get result type
        bool count;
        if (subModeArg == UString("list"))
            count = false;
        else if (subModeArg == UString("count"))
            count = true;
        else
            return U_INVALID_PARAMETER;

        // Parse input file
        result = w.init(inputArg);
        if (result)
            return result;

        // Go find the supplied pattern
        UString found;
        result = w.find(mode, count, patternArg, found);
        if (result)
            return result;

        // Nothing is found
        if (found.isEmpty())
            return U_ITEM_NOT_FOUND;

        // Print result
        std::cout << found.toLocal8Bit();
        return U_SUCCESS;
    }
    else if (argc == 4) {
        UString inputArg = argv[1];
        UString modeArg = argv[2];
        UString patternArg = argv[3];

        // Get search mode
        if (modeArg != UString("file"))
            return U_INVALID_PARAMETER;

        // Open patterns file
        if (!isExistOnFs(patternArg))
            return U_FILE_OPEN;

        std::ifstream patternsFile(patternArg.toLocal8Bit());
        if (!patternsFile)
            return U_FILE_OPEN;

        // Parse input file
        result = w.init(inputArg);
        if (result)
            return result;

        // Perform searches
        bool somethingFound = false;
        while (!patternsFile.eof()) {
            std::string line;
            std::getline(patternsFile, line);
            // Use sharp symbol as commentary
            if (line.size() == 0 || line[0] == '#')
                continue;

            // Split the read line
            std::vector<UString> list;
            std::string::size_type prev = 0, curr = 0;
            while ((curr = line.find(' ', curr)) != std::string::npos) {
                std::string substring( line.substr(prev, curr-prev) );
                list.push_back(UString(substring.c_str()));
                prev = ++curr;
            }
            list.push_back(UString(line.substr(prev, curr-prev).c_str()));

            if (list.size() < 3) {
                std::cout << line << std::endl << "skipped, too few arguments" << std::endl << std::endl;
                continue;
            }
            // Get search mode
            UINT8 mode;
            if (list.at(0) == UString("header"))
                mode = SEARCH_MODE_HEADER;
            else if (list.at(0) == UString("body"))
                mode = SEARCH_MODE_BODY;
            else if (list.at(0) == UString("all"))
                mode = SEARCH_MODE_ALL;
            else {
                std::cout << line << std::endl << "skipped, invalid search mode" << std::endl << std::endl;
                continue;
            }

            // Get result type
            bool count;
            if (list.at(1) == UString("list"))
                count = false;
            else if (list.at(1) == UString("count"))
                count = true;
            else {
                std::cout << line << std::endl << "skipped, invalid result type" << std::endl << std::endl;
                continue;
            }

            // Go find the supplied pattern
            UString found;
            result = w.find(mode, count, list.at(2), found);
            if (result) {
                std::cout << line << std::endl << "skipped, find failed with error " << (UINT32)result << std::endl << std::endl;
                continue;
            }

            if (found.isEmpty()) {
                // Nothing is found
                std::cout << line << std::endl << "nothing found" << std::endl << std::endl;
            }
            else {
                // Print result
                std::cout << line << std::endl << found.toLocal8Bit() << std::endl;
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
