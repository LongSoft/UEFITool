/* ffsdumper.cpp

Copyright (c) 2015, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#include <windows.h> 

#include "uefidump.h"
#include "../common/ffs.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>

USTATUS UEFIDumper::dump(const UByteArray & buffer, const std::wstring & inPath, const std::wstring & guid)
{
    //TODO: rework to support relative paths
    std::wstring path = std::wstring(inPath).append(L".dump2");
    path = L"\\\\?\\" + path;
    std::wstring reportPath = std::wstring(inPath).append(L".report2.txt");
    reportPath = L"\\\\?\\" + reportPath;
    std::wcout << L"Dump path: " << path << std::endl;
    std::wcout << L"Report path: " << reportPath << std::endl;


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

        // Get last VTF
        UModelIndex lastVtf = ffsParser.getLastVtf();
        if (lastVtf.isValid()) {
            // Create fitParser
            FitParser fitParser(&model);
            // Find and parse FIT table
            result = fitParser.parse(model.index(0, 0), lastVtf);
            if (U_SUCCESS == result) {
                // Show fitParser's messages
                std::vector<std::pair<UString, UModelIndex> > fitMessages = fitParser.getMessages();
                for (size_t i = 0; i < fitMessages.size(); i++) {
                    std::cout << (const char*)fitMessages[i].first.toLocal8Bit() << std::endl;
                }

                // Show FIT table
                std::vector<std::vector<UString> > fitTable = fitParser.getFitTable();
                if (fitTable.size()) {
                    std::cout << "-------------------------------------------------------------------" << std::endl;
                    std::cout << "     Address     |   Size   | Ver  |           Type           | CS " << std::endl;
                    std::cout << "-------------------------------------------------------------------" << std::endl;
                    for (size_t i = 0; i < fitTable.size(); i++) {
                        std::cout << (const char*)fitTable[i][0].toLocal8Bit() << " | "
                            << (const char*)fitTable[i][1].toLocal8Bit() << " | "
                            << (const char*)fitTable[i][2].toLocal8Bit() << " | "
                            << (const char*)fitTable[i][3].toLocal8Bit() << " | "
                            << (const char*)fitTable[i][4].toLocal8Bit() << std::endl;
                    }
                }
            }
        }

        // Create ffsReport
        FfsReport ffsReport(&model);
        std::vector<UString> report = ffsReport.generate();
        if (report.size()) {
            std::ofstream ofs;
            ofs.open(reportPath, std::ofstream::out);
            for (size_t i = 0; i < report.size(); i++) {
                ofs << (const char*)report[i].toLocal8Bit() << std::endl;
            }
            ofs.close();
        }
        
        initialized = true;
    }
    
    dumped = false;
    UINT8 result = recursiveDump(model.index(0,0), path, guid);
    if (result)
        return result;
    else if (!dumped)
        return U_ITEM_NOT_FOUND;

    return U_SUCCESS;
}

std::wstring UEFIDumper::guidToWstring(const EFI_GUID & guid)
{
    std::wstringstream ws;

    ws << std::hex << std::uppercase << std::setfill(L'0');
    ws << std::setw(8) << *(const UINT32*)&guid.Data[0] << L"-";
    ws << std::setw(4) << *(const UINT16*)&guid.Data[4] << L"-";
    ws << std::setw(4) << *(const UINT16*)&guid.Data[6] << L"-";
    ws << std::setw(2) << guid.Data[8];
    ws << std::setw(2) << guid.Data[9] << L"-";
    ws << std::setw(2) << guid.Data[10];
    ws << std::setw(2) << guid.Data[11];
    ws << std::setw(2) << guid.Data[12];
    ws << std::setw(2) << guid.Data[13];
    ws << std::setw(2) << guid.Data[14];
    ws << std::setw(2) << guid.Data[15];

    return ws.str();
}

bool UEFIDumper::createFullPath(const std::wstring & path) {

    // Break the path into parent\current, assuming the path is already full and converted into Windows native "\\?\" format
    size_t pos = path.find_last_of(L'\\');

    // Slash is not found, it's a bug
    if (pos == path.npos)
        return FALSE;
    
    std::wstring parent = path.substr(0, pos);
    std::wstring current = path.substr(pos + 1);

    // Check if first exist, if so, create last and return true
    UINT32 attributes = GetFileAttributesW(parent.c_str());
    if ((attributes != INVALID_FILE_ATTRIBUTES) && (attributes & FILE_ATTRIBUTE_DIRECTORY)) {
        // first is already exist, just create last
        return CreateDirectoryW(path.c_str(), NULL) != 0;
    }

    // Perform recursive call
    if (createFullPath(parent))
        return CreateDirectoryW(path.c_str(), NULL) != 0;

    return FALSE;
}

USTATUS UEFIDumper::recursiveDump(const UModelIndex & index, const std::wstring & path, const std::wstring & guid)
{
    if (!index.isValid())
        return U_INVALID_PARAMETER;

    UByteArray itemHeader = model.header(index);
    UByteArray fileHeader = model.header(model.findParentOfType(index, Types::File));

    if (guid.length() == 0 ||
        (itemHeader.size() >= sizeof (EFI_GUID) && guidToWstring(*(const EFI_GUID*)itemHeader.constData()) == guid) ||
        (fileHeader.size() >= sizeof(EFI_GUID) && guidToWstring(*(const EFI_GUID*)fileHeader.constData()) == guid)) {

        if (SetCurrentDirectoryW(path.c_str()))
            return U_DIR_ALREADY_EXIST;

        if (!createFullPath(path))
            return U_DIR_CREATE;

        //if (model.rowCount(index) == 0) {
            // Header
            UByteArray data = model.header(index);
            if (!data.isEmpty()) {
                std::ofstream file;
                std::wstring name = path + std::wstring(L"\\header.bin");
                file.open(name, std::ios::out | std::ios::binary);
                file.write(data.constData(), data.size());
                file.close();
            }

            // Body
            data = model.body(index);
            if (!data.isEmpty()) {
                std::ofstream file;
                std::wstring name = path + std::wstring(L"\\body.bin");
                file.open(name, std::ios::out | std::ios::binary);
                file.write(data.constData(), data.size());
                file.close();
            }
        //}
        // Info
        UString info = "Type: " + itemTypeToUString(model.type(index)) + "\n" +
            "Subtype: " + itemSubtypeToUString(model.type(index), model.subtype(index)) + "\n";
        if (model.text(index).length() > 0)
            info += "Text: " + model.text(index) + "\n";
        info += model.info(index) + "\n";

        std::ofstream file;
        std::wstring name = path + std::wstring(L"\\info.txt");
        file.open(name, std::ios::out);
        file.write((const char*)info, info.length());
        file.close();

        dumped = true;
    }

    UINT8 result;
    for (int i = 0; i < model.rowCount(index); i++) {
        UModelIndex childIndex = index.child(i, 0);
        bool useText = false;
        if (model.type(childIndex) != Types::Volume)
            useText = (model.text(childIndex).length() > 0);
                
        UString name = useText ? (const char *)model.text(childIndex) : (const char *)model.name(childIndex);
        std::string sName = std::string((const char*)name, name.length());
        std::wstring childPath = path + std::wstring(L"\\") + std::to_wstring(i) + std::wstring(L" ") + std::wstring(sName.begin(), sName.end());
        
        // Workaround for paths with dot at the end, just add remove it
        if (childPath.at(childPath.length() - 1) == L'.')
            childPath.pop_back();

        result = recursiveDump(childIndex, childPath, guid);
        if (result)
            return result;
    }

    return U_SUCCESS;
}
