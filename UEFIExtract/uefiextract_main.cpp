/* uefiextract_main.cpp
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
#include <cstring>
#include <cstdlib>

#include "../version.h"
#include "../common/basetypes.h"
#include "../common/ustring.h"
#include "../common/filesystem.h"
#include "../common/ffsparser.h"
#include "../common/ffsreport.h"
#include "../common/guiddatabase.h"
#include "ffsdumper.h"
#include "uefidump.h"

enum ReadType {
    READ_INPUT,
    READ_OUTPUT,
    READ_MODE,
    READ_SECTION
};

int main(int argc, char *argv[])
{
    initGuidDatabase("guids.csv");

    if (argc > 1) {
        // Check that input file exists
        USTATUS result;
        UByteArray buffer;
        UString path = getAbsPath(argv[1]);
        result = readFileIntoBuffer(path, buffer);
        if (result)
            return result;

        // Hack to support legacy UEFIDump mode.
        if (argc == 3 && !std::strcmp(argv[2], "unpack")) {
            UEFIDumper uefidumper;
            return (uefidumper.dump(buffer, UString(argv[1])) != U_SUCCESS);
        }

        // Create model and ffsParser
        TreeModel model;
        FfsParser ffsParser(&model);
        // Parse input buffer
        result = ffsParser.parse(buffer);
        if (result)
            return result;

        // Show ffsParser's messages
        std::vector<std::pair<UString, UModelIndex> > messages = ffsParser.getMessages();
        for (size_t i = 0; i < messages.size(); i++) {
            std::cout << messages[i].first.toLocal8Bit() << std::endl;
        }

        // Get last VTF
        std::vector<std::pair<std::vector<UString>, UModelIndex > > fitTable = ffsParser.getFitTable();
        if (fitTable.size()) {
            std::cout << "---------------------------------------------------------------------------" << std::endl;
            std::cout << "     Address      |   Size    |  Ver  | CS  |          Type / Info          " << std::endl;
            std::cout << "---------------------------------------------------------------------------" << std::endl;
            for (size_t i = 0; i < fitTable.size(); i++) {
                std::cout << fitTable[i].first[0].toLocal8Bit() << " | "
                    << fitTable[i].first[1].toLocal8Bit() << " | "
                    << fitTable[i].first[2].toLocal8Bit() << " | "
                    << fitTable[i].first[3].toLocal8Bit() << " | "
                    << fitTable[i].first[4].toLocal8Bit() << " | "
                    << fitTable[i].first[5].toLocal8Bit() << std::endl;
            }
        }


        // Create ffsDumper
        FfsDumper ffsDumper(&model);

        // Dump only leaf elements, no report
        if (argc == 3 && !std::strcmp(argv[2], "dump")) {
            return (ffsDumper.dump(model.index(0, 0), path + UString(".dump")) != U_SUCCESS);
        }
        else if (argc > 3 ||
            (argc == 3 && std::strcmp(argv[2], "all") != 0 && std::strcmp(argv[2], "report") != 0)) { // Dump specific files, without report
            std::vector<UString> inputs, outputs;
            std::vector<FfsDumper::DumpMode> modes;
            std::vector<UINT8> sectionTypes;
            ReadType readType = READ_INPUT;
            for (int i = 2; i < argc; i++) {
                const char *arg = argv[i];
                if (!std::strcmp(arg, "-i")) {
                    readType = READ_INPUT;
                    continue;
                } else if (!std::strcmp(arg, "-o")) {
                    readType = READ_OUTPUT;
                    continue;
                } else if (!std::strcmp(arg, "-m")) {
                    readType = READ_MODE;
                    continue;
                } else if (!std::strcmp(arg, "-t")) {
                    readType = READ_SECTION;
                    continue;
                }

                if (readType == READ_INPUT) {
                    inputs.push_back(arg);
                } else if (readType == READ_OUTPUT) {
                    outputs.push_back(getAbsPath(arg));
                } else if (readType == READ_MODE) {
                    if (!std::strcmp(arg, "all"))
                        modes.push_back(FfsDumper::DUMP_ALL);
                    else if (!std::strcmp(arg, "body"))
                        modes.push_back(FfsDumper::DUMP_BODY);
                    else if (!std::strcmp(arg, "header"))
                        modes.push_back(FfsDumper::DUMP_HEADER);
                    else if (!std::strcmp(arg, "info"))
                        modes.push_back(FfsDumper::DUMP_INFO);
                    else if (!std::strcmp(arg, "file"))
                        modes.push_back(FfsDumper::DUMP_FILE);
                    else
                        return U_INVALID_PARAMETER;
                } else if (readType == READ_SECTION) {
                    char *converted = const_cast<char *>(arg);
                    UINT8 sectionType = (UINT8)std::strtol(arg, &converted, 16);
                    if (converted == arg)
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
                UString outPath = outputs.empty() ? path + UString(".dump") : outputs[i];
                FfsDumper::DumpMode mode = modes.empty() ? FfsDumper::DUMP_ALL : modes[i];
                UINT8 type = sectionTypes.empty() ? FfsDumper::IgnoreSectionType : sectionTypes[i];
                result = ffsDumper.dump(model.index(0, 0), outPath, mode, type, inputs[i]);
                if (result) {
                    std::cout << "Guid " << inputs[i].toLocal8Bit() << " failed with " << result << " code!" << std::endl;
                    lastError = result;
                }
            }

            return lastError;
        }

        // Create ffsReport
        FfsReport ffsReport(&model);
        std::vector<UString> report = ffsReport.generate();
        if (report.size()) {
            std::ofstream file;
            file.open((path + UString(".report.txt")).toLocal8Bit());
            for (size_t i = 0; i < report.size(); i++)
                file << report[i].toLocal8Bit() << '\n';
        }

        // Dump all non-leaf elements, with report, default
        if (argc == 2) {
            return (ffsDumper.dump(model.index(0, 0), path + UString(".dump")) != U_SUCCESS);
        }
        else if (argc == 3 && !std::strcmp(argv[2], "all")) { // Dump every element with report
            return (ffsDumper.dump(model.index(0, 0), path + UString(".dump"), FfsDumper::DUMP_ALL) != U_SUCCESS);
        }
        else if (argc == 3 && !std::strcmp(argv[2], "report")) { // Skip dumping
            return 0;
        }
    }
    // If parameters are different, show version and usage information
    std::cout << "UEFIExtract " PROGRAM_VERSION << std::endl << std::endl
        << "Usage: UEFIExtract imagefile        - generate report and dump only leaf tree items into .dump folder." << std::endl
        << "       UEFIExtract imagefile all    - generate report and dump all tree items." << std::endl
        << "       UEFIExtract imagefile unpack - generate report and dump all tree items in one dir." << std::endl
        << "       UEFIExtract imagefile dump   - only generate dump, no report needed." << std::endl
        << "       UEFIExtract imagefile report - only generate report, no dump needed." << std::endl
        << "       UEFIExtract imagefile GUID_1 ... [ -o FILE_1 ... ] [ -m MODE_1 ... ] [ -t TYPE_1 ... ] -" << std::endl
        << "         Dump only FFS file(s) with specific GUID(s), without report." << std::endl
        << "         Type is section type or FF to ignore. Mode is one of: all, body, header, info, file." << std::endl
        << "Return value is a bit mask where 0 at position N means that file with GUID_N was found and unpacked, 1 otherwise." << std::endl;
    return 1;
}
