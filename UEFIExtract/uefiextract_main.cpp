/* uefiextract_main.cpp

Copyright (c) 2016, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/
#include <QCoreApplication>
#include <QFileInfo>
#include <iostream>

#include <../common/ustring.h>
#include "../common/ffsparser.h"
#include "../common/ffsreport.h"
#include "../common/fitparser.h"
#include "ffsdumper.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    a.setOrganizationName("CodeRush");
    a.setOrganizationDomain("coderush.me");
    a.setApplicationName("UEFIExtract");

    if (a.arguments().length() > 32) {
        std::cout << "Too many arguments" << std::endl;
        return 1;
    }

    if (a.arguments().length() > 1) {
        // Check that input file exists
        UString path = a.arguments().at(1);
        QFileInfo fileInfo(path);
        if (!fileInfo.exists())
            return U_FILE_OPEN;

        // Open the input file
        QFile inputFile;
        inputFile.setFileName(path);
        if (!inputFile.open(QFile::ReadOnly))
            return U_FILE_OPEN;
        
        // Read and close the file
        UByteArray buffer = inputFile.readAll();
        inputFile.close();

        // Create model and ffsParser
        TreeModel model;
        FfsParser ffsParser(&model);
        // Parse input buffer
        USTATUS result = ffsParser.parse(buffer);
        if (result)
            return result;

        // Show ffsParser's messages
        std::vector<std::pair<UString, UModelIndex> > messages = ffsParser.getMessages();
        for (size_t i = 0; i < messages.size(); i++) {
            std::cout << messages[i].first.toLatin1().constData() << std::endl;
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
                    std::cout << fitMessages[i].first.toLatin1().constData() << std::endl;
                }

                // Show FIT table
                std::vector<std::vector<UString> > fitTable = fitParser.getFitTable();
                if (fitTable.size()) {
                    std::cout << "-------------------------------------------------------------------" << std::endl;
                    std::cout << "     Address     |   Size   | Ver  |           Type           | CS " << std::endl;
                    std::cout << "-------------------------------------------------------------------" << std::endl;
                    for (size_t i = 0; i < fitTable.size(); i++) {
                        std::cout << fitTable[i][0].toLatin1().constData() << " | "
                            << fitTable[i][1].toLatin1().constData() << " | "
                            << fitTable[i][2].toLatin1().constData() << " | "
                            << fitTable[i][3].toLatin1().constData() << " | "
                            << fitTable[i][4].toLatin1().constData() << std::endl;
                    }
                }
            }
        }
        
        // Create ffsReport
        FfsReport ffsReport(&model);
        std::vector<UString> report = ffsReport.generate();
        if (report.size()) {
            QFile file;
            file.setFileName(fileInfo.fileName().append(".report.txt"));
            if (file.open(QFile::Text | QFile::WriteOnly)) {
                for (size_t i = 0; i < report.size(); i++) {
                    file.write(report[i].toLatin1().append('\n'));
                }
                file.close();
            }
        }
                
        // Create ffsDumper
        FfsDumper ffsDumper(&model);

        // Dump all non-leaf elements
        if (a.arguments().length() == 2) {
            return (ffsDumper.dump(model.index(0, 0), fileInfo.fileName().append(".dump")) != U_SUCCESS);
        }
        else if (a.arguments().length() == 3 && a.arguments().at(2) == UString("all")) { // Dump everything
            return (ffsDumper.dump(model.index(0, 0), fileInfo.fileName().append(".dump"), true) != U_SUCCESS);
        }
        else if (a.arguments().length() == 3 && a.arguments().at(2) == UString("none")) { // Skip dumping
            return 0;
        }
        else { // Dump specific files
            UINT32 returned = 0;
            for (int i = 2; i < a.arguments().length(); i++) {
                result = ffsDumper.dump(model.index(0, 0), fileInfo.fileName().append(".dump"), true, a.arguments().at(i));
                if (result)
                    returned |= (1 << (i - 1));
            }
            return returned;
        }

        return 0;
    }
    else { // Show version and usage information
        std::cout << "UEFIExtract 0.12.0" << std::endl << std::endl
            << "Usage: UEFIExtract imagefile      - generate report and dump only leaf tree items into .dump folder" << std::endl
            << "       UEFIExtract imagefile all  - generate report and dump all tree items" << std::endl
            << "       UEFIExtract imagefile none - only generate report, no dump needed" << std::endl
            << "       UIFIExtract imagefile GUID_1 GUID_2 ... GUID_31 - dump only FFS file(s) with specific GUID(s)" << std::endl
            << "Return value is a bit mask where 0 at position N means that file with GUID_N was found and unpacked, 1 otherwise" << std::endl;
        return 1;
    }
}
