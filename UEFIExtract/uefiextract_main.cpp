/* uefiextract_main.cpp
Copyright (c) 2018, LongSoft. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*/
#include <QCoreApplication>
#include <QString>
#include <QFileInfo>

#include <iostream>

#include "../version.h"
#include "../common/ffsparser.h"
#include "../common/ffsreport.h"
#include "ffsdumper.h"

enum ReadType {
    READ_INPUT,
    READ_OUTPUT,
    READ_MODE,
    READ_SECTION
};

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    a.setOrganizationName("LongSoft");
    a.setOrganizationDomain("longsoft.me");
    a.setApplicationName("UEFIExtract");

    if (a.arguments().length() > 1) {
        // Check that input file exists
        QString path = a.arguments().at(1);
        QFileInfo fileInfo(path);
        if (!fileInfo.exists())
            return U_FILE_OPEN;

        // Open the input file
        QFile inputFile;
        inputFile.setFileName(path);
        if (!inputFile.open(QFile::ReadOnly))
            return U_FILE_OPEN;

        // Read and close the file
        QByteArray buffer = inputFile.readAll();
        inputFile.close();

        // Create model and ffsParser
        TreeModel model;
        FfsParser ffsParser(&model);
        // Parse input buffer
        USTATUS result = ffsParser.parse(buffer);
        if (result)
            return result;

        // Show ffsParser's messages
        std::vector<std::pair<QString, QModelIndex> > messages = ffsParser.getMessages();
        for (size_t i = 0; i < messages.size(); i++) {
            std::cout << messages[i].first.toLatin1().constData() << std::endl;
        }

        // Get last VTF
        std::vector<std::pair<std::vector<QString>, QModelIndex > > fitTable = ffsParser.getFitTable();
        if (fitTable.size()) {
            std::cout << "---------------------------------------------------------------------------" << std::endl;
            std::cout << "     Address     |   Size    |  Ver  | CS  |          Type / Info          " << std::endl;
            std::cout << "---------------------------------------------------------------------------" << std::endl;
            for (size_t i = 0; i < fitTable.size(); i++) {
                std::cout << fitTable[i].first[0].toLatin1().constData() << " | "
                    << fitTable[i].first[1].toLatin1().constData() << " | "
                    << fitTable[i].first[2].toLatin1().constData() << " | "
                    << fitTable[i].first[3].toLatin1().constData() << " | "
                    << fitTable[i].first[4].toLatin1().constData() << " | "
                    << fitTable[i].first[5].toLatin1().constData() << std::endl;
            }
        }


        // Create ffsDumper
        FfsDumper ffsDumper(&model);

        // Dump only leaf elements, no report
        if (a.arguments().length() == 3 && a.arguments().at(2) == QString("dump")) {
            return (ffsDumper.dump(model.index(0, 0), fileInfo.fileName().append(".dump")) != U_SUCCESS);
        }
        else if (a.arguments().length() > 3 ||
            (a.arguments().length() == 3 && a.arguments().at(2) != QString("all") && a.arguments().at(2) != QString("report"))) { // Dump specific files, without report
            std::vector<QString> inputs, outputs;
            std::vector<FfsDumper::DumpMode> modes;
            std::vector<UINT8> sectionTypes;
            ReadType readType = READ_INPUT;
            for (int i = 2; i < a.arguments().length(); i++) {
                QString arg = a.arguments().at(i);
                if (arg == QString("-i")) {
                    readType = READ_INPUT;
                    continue;
                } else if (arg == QString("-o")) {
                    readType = READ_OUTPUT;
                    continue;
                } else if (arg == QString("-m")) {
                    readType = READ_MODE;
                    continue;
                } else if (arg == QString("-t")) {
                    readType = READ_SECTION;
                    continue;
                }

                if (readType == READ_INPUT) {
                    inputs.push_back(arg);
                } else if (readType == READ_OUTPUT) {
                    outputs.push_back(arg);
                } else if (readType == READ_MODE) {
                    if (arg == QString("all"))
                        modes.push_back(FfsDumper::DUMP_ALL);
                    else if (arg == QString("body"))
                        modes.push_back(FfsDumper::DUMP_BODY);
                    else if (arg == QString("header"))
                        modes.push_back(FfsDumper::DUMP_HEADER);
                    else if (arg == QString("info"))
                        modes.push_back(FfsDumper::DUMP_INFO);
                    else
                        return U_INVALID_PARAMETER;
                } else if (readType == READ_SECTION) {
                    bool converted;
                    UINT8 sectionType = (UINT8)arg.toUShort(&converted, 16);
                    if (!converted)
                        return U_INVALID_PARAMETER;
                    sectionTypes.push_back(sectionType);
                }
            }
            if (inputs.empty() || (!outputs.empty() && inputs.size() != outputs.size()) ||
                (!modes.empty() && inputs.size() != modes.size()) ||
                (!sectionTypes.empty() && inputs.size() != sectionTypes.size()))
                return U_INVALID_PARAMETER;

            USTATUS lastError = U_SUCCESS;
            for (size_t i = 0; i < inputs.size(); i++) {
                QString outPath = outputs.empty() ? fileInfo.fileName().append(".dump") : outputs[i];
                FfsDumper::DumpMode mode = modes.empty() ? FfsDumper::DUMP_ALL : modes[i];
                UINT8 type = sectionTypes.empty() ? FfsDumper::IgnoreSectionType : sectionTypes[i];
                result = ffsDumper.dump(model.index(0, 0), outPath, mode, type, inputs[i]);
                if (result) {
                    std::cout << "Guid " << inputs[i].toStdString() << " failed with " << result << " code!" << std::endl;
                    lastError = result;
                }
            }

            return lastError;
        }

        // Create ffsReport
        FfsReport ffsReport(&model);
        std::vector<QString> report = ffsReport.generate();
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

        // Dump all non-leaf elements, with report, default
        if (a.arguments().length() == 2) {
            return (ffsDumper.dump(model.index(0, 0), fileInfo.fileName().append(".dump")) != U_SUCCESS);
        }
        else if (a.arguments().length() == 3 && a.arguments().at(2) == QString("all")) { // Dump every elementm with report
            return (ffsDumper.dump(model.index(0, 0), fileInfo.fileName().append(".dump"), FfsDumper::DUMP_ALL) != U_SUCCESS);
        }
        else if (a.arguments().length() == 3 && a.arguments().at(2) == QString("report")) { // Skip dumping
            return 0;
        }
    }
    // If parameters are different, show version and usage information
    std::cout << "UEFIExtract " PROGRAM_VERSION << std::endl << std::endl
        << "Usage: UEFIExtract imagefile        - generate report and dump only leaf tree items into .dump folder." << std::endl
        << "       UEFIExtract imagefile all    - generate report and dump all tree items." << std::endl
        << "       UEFIExtract imagefile dump   - only generate dump, no report needed." << std::endl
        << "       UEFIExtract imagefile report - only generate report, no dump needed." << std::endl
        << "       UEFIExtract imagefile GUID_1 ... [ -o FILE_1 ... ] [ -m MODE_1 ... ] [ -t TYPE_1 ... ] -" << std::endl
        << "         Dump only FFS file(s) with specific GUID(s), without report." << std::endl
        << "         Type is section type or FF to ignore. Mode is one of: all, body, header, info." << std::endl
        << "Return value is a bit mask where 0 at position N means that file with GUID_N was found and unpacked, 1 otherwise." << std::endl;
    return 1;
}
