/* ffsdumper.cpp

Copyright (c) 2018, LongSoft. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#include "uefidump.h"
#include "../common/ffs.h"
#include "../common/utility.h"
#include "../common/filesystem.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>

USTATUS UEFIDumper::dump(const UByteArray & buffer, const UString & inPath, const UString & guid)
{
    UString path = UString(inPath) + UString(".dump");
    UString reportPath = UString(inPath) + UString(".report.txt");

    if (initialized) {
        // Check if called with a different buffer as before
        if (buffer != currentBuffer) {
            // Reinitalize if so
            initialized = false;
        }
    }

    if (!initialized) {
        // Fill currentBuffer
        currentBuffer = buffer;

        // Parse FFS structure
        USTATUS result = ffsParser.parse(buffer);
        if (result)
            return result;
        // Show ffsParser messages
        std::vector<std::pair<UString, UModelIndex> > messages = ffsParser.getMessages();
        for (size_t i = 0; i < messages.size(); i++) {
            std::cout << messages[i].first << std::endl;
        }

        // Show FIT table
        std::vector<std::pair<std::vector<UString>, UModelIndex > > fitTable = ffsParser.getFitTable();
        if (fitTable.size()) {
            std::cout << "-------------------------------------------------------------------------" << std::endl;
            std::cout << "     Address       |   Size    |  Ver  | CS  |         Type / Info         " << std::endl;
            std::cout << "-------------------------------------------------------------------------" << std::endl;
            for (size_t i = 0; i < fitTable.size(); i++) {
                std::cout << fitTable[i].first[0].toLocal8Bit() << " | "
                    << fitTable[i].first[1].toLocal8Bit() << " | "
                    << fitTable[i].first[2].toLocal8Bit() << " | "
                    << fitTable[i].first[3].toLocal8Bit() << " | "
                    << fitTable[i].first[4].toLocal8Bit() << " | "
                    << fitTable[i].first[5].toLocal8Bit() << std::endl;
            }
        }

        // Create ffsReport
        FfsReport ffsReport(&model);
        std::vector<UString> report = ffsReport.generate();
        if (report.size()) {
            std::ofstream ofs;
            ofs.open(reportPath, std::ofstream::out);
            for (size_t i = 0; i < report.size(); i++) {
                ofs << report[i].toLocal8Bit() << std::endl;
            }
            ofs.close();
        }
        
        initialized = true;
    }
    
    // Check for dump directory existence
    if (isExistOnFs(path))
        return U_DIR_ALREADY_EXIST;

    // Create dump directory and cd to it
    if (!makeDirectory(path))
        return U_DIR_CREATE;

    if (!changeDirectory(path))
        return U_DIR_CHANGE;
    
    dumped = false;
    USTATUS result = recursiveDump(model.index(0,0));
    if (result)
        return result;
    else if (!dumped)
        return U_ITEM_NOT_FOUND;

    return U_SUCCESS;
}

USTATUS UEFIDumper::recursiveDump(const UModelIndex & index)
{
    if (!index.isValid())
        return U_INVALID_PARAMETER;

    //UByteArray itemHeader = model.header(index);
    //UByteArray fileHeader = model.header(model.findParentOfType(index, Types::File));

    //if (guid.length() == 0 ||
    //    (itemHeader.size() >= sizeof (EFI_GUID) && guidToUString(*(const EFI_GUID*)itemHeader.constData()) == guid) ||
    //    (fileHeader.size() >= sizeof(EFI_GUID) && guidToUString(*(const EFI_GUID*)fileHeader.constData()) == guid)) {

        // Construct file name
        UString orgName = uniqueItemName(index);
        UString name = orgName;
        bool nameFound = false;
        for (int i = 1; i < 1000; ++i) {
            if (!isExistOnFs(name + UString("_info.txt"))) {
                nameFound = true;
                break;
            }
            name = orgName + UString("_") + usprintf("%03d", i);
        }

        if (!nameFound)
            return U_INVALID_PARAMETER; //TODO: replace with proper errorCode

        // Add header and body only for leaf sections
        if (model.rowCount(index) == 0) {
            // Header
            UByteArray data = model.header(index);
            if (!data.isEmpty()) {
                std::ofstream file;
                UString str = name + UString("_header.bin");
                file.open(str.toLocal8Bit(), std::ios::out | std::ios::binary);
                file.write(data.constData(), data.size());
                file.close();
            }

            // Body
            data = model.body(index);
            if (!data.isEmpty()) {
                std::ofstream file;
                UString str = name + UString("_body.bin");
                file.open(str.toLocal8Bit(), std::ios::out | std::ios::binary);
                file.write(data.constData(), data.size());
                file.close();
            }
        }
        // Info
        UString info = "Type: " + itemTypeToUString(model.type(index)) + "\n" +
            "Subtype: " + itemSubtypeToUString(model.type(index), model.subtype(index)) + "\n";
        if (model.text(index).length() > 0)
            info += "Text: " + model.text(index) + "\n";
        info += model.info(index) + "\n";

        std::ofstream file;
        UString str = name + UString("_info.txt");
        file.open(str.toLocal8Bit(), std::ios::out);
        file.write(info.toLocal8Bit(), info.length());
        file.close();

        dumped = true;
    //}
    
    // Process child items
    USTATUS result;
    for (int i = 0; i < model.rowCount(index); i++) {
        result = recursiveDump(index.child(i, 0));
        if (result)
            return result;
    }

    return U_SUCCESS;
}
