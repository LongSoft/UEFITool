/* uefiedit_main.cpp

Copyright (c) 2026, LongSoft. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#include <iostream>
#include <cstring>
#include <string>

#include "../version.h"
#include "../common/basetypes.h"
#include "../common/ustring.h"
#include "../common/filesystem.h"
#include "../common/guiddatabase.h"
#include "uefiedit.h"

static void print_usage()
{
    std::cout << "UEFIEdit " PROGRAM_VERSION << std::endl
        << "Console utility for inserting/removing/replacing FFS files and sections in UEFI images." << std::endl
        << "Usage:" << std::endl
        << "  UEFIEdit {-h | --help | -v | --version}" << std::endl
        << "  UEFIEdit imagefile dump                              - print parsed tree" << std::endl
        << "  UEFIEdit imagefile save output.bin                   - save (rebuild) the current image" << std::endl
        << "  UEFIEdit imagefile insert GUID file.ffs              - insert FFS file into the volume containing GUID" << std::endl
        << "  UEFIEdit imagefile insert-before GUID file.ffs       - insert FFS file before the file with GUID" << std::endl
        << "  UEFIEdit imagefile insert-after GUID file.ffs        - insert FFS file after the file with GUID" << std::endl
        << "  UEFIEdit imagefile remove GUID                       - mark the file with GUID for removal" << std::endl
        << "  UEFIEdit imagefile replace GUID file.ffs             - replace the file with GUID (full, as-is)" << std::endl
        << "  UEFIEdit imagefile replace-body GUID file.bin        - replace the body of the file with GUID" << std::endl
        << "  UEFIEdit imagefile rebuild GUID                      - mark the file with GUID for rebuild" << std::endl
        << std::endl
        << "Multiple edit commands can be chained before \"save\". GUID format: XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX" << std::endl
        << "Exit code is 0 on success, non-zero USTATUS code on failure." << std::endl;
}

int main(int argc, char *argv[])
{
    initGuidDatabase("guids.csv");

    if (argc <= 1) {
        print_usage();
        return 0;
    }

    UString arg1 = argv[1];
    if (arg1 == UString("-h") || arg1 == UString("--help")) {
        print_usage();
        return 0;
    }
    if (arg1 == UString("-v") || arg1 == UString("--version")) {
        std::cout << PROGRAM_VERSION << std::endl;
        return 0;
    }

    if (argc < 3) {
        print_usage();
        return U_INVALID_PARAMETER;
    }

    UString imagePath = getAbsPath(argv[1]);
    UEFIEdit editor;
    USTATUS result = editor.init(imagePath);
    if (result) {
        std::cerr << "init failed: " << errorCodeToUString(result).toLocal8Bit() << std::endl;
        return (int)result;
    }

    // Process commands starting from argv[2]. Each command consumes a fixed number
    // of arguments. The loop allows chaining multiple edits in a single invocation.
    int i = 2;
    while (i < argc) {
        UString cmd = argv[i];

        if (cmd == UString("dump")) {
            editor.dumpTree();
            i += 1;
            continue;
        }
        if (cmd == UString("save") && i + 1 < argc) {
            UString outPath = getAbsPath(argv[i + 1]);
            result = editor.save(outPath);
            if (result) {
                std::cerr << "save failed: " << errorCodeToUString(result).toLocal8Bit() << std::endl;
                return (int)result;
            }
            std::cerr << "Saved to " << outPath.toLocal8Bit() << std::endl;
            i += 2;
            continue;
        }
        if (cmd == UString("insert") && i + 2 < argc) {
            result = editor.insert(UString(argv[i + 1]), CREATE_MODE_PREPEND, UString(argv[i + 2]));
            if (result) return (int)result;
            i += 3;
            continue;
        }
        if (cmd == UString("insert-before") && i + 2 < argc) {
            result = editor.insert(UString(argv[i + 1]), CREATE_MODE_BEFORE, UString(argv[i + 2]));
            if (result) return (int)result;
            i += 3;
            continue;
        }
        if (cmd == UString("insert-after") && i + 2 < argc) {
            result = editor.insert(UString(argv[i + 1]), CREATE_MODE_AFTER, UString(argv[i + 2]));
            if (result) return (int)result;
            i += 3;
            continue;
        }
        if (cmd == UString("remove") && i + 1 < argc) {
            result = editor.remove(UString(argv[i + 1]));
            if (result) return (int)result;
            i += 2;
            continue;
        }
        if (cmd == UString("replace") && i + 2 < argc) {
            result = editor.replace(UString(argv[i + 1]), REPLACE_MODE_AS_IS, UString(argv[i + 2]));
            if (result) return (int)result;
            i += 3;
            continue;
        }
        if (cmd == UString("replace-body") && i + 2 < argc) {
            result = editor.replace(UString(argv[i + 1]), REPLACE_MODE_BODY, UString(argv[i + 2]));
            if (result) return (int)result;
            i += 3;
            continue;
        }
        if (cmd == UString("rebuild") && i + 1 < argc) {
            result = editor.rebuild(UString(argv[i + 1]));
            if (result) return (int)result;
            i += 2;
            continue;
        }

        std::cerr << "Unknown or incomplete command: " << cmd.toLocal8Bit() << std::endl;
        print_usage();
        return U_INVALID_PARAMETER;
    }

    return 0;
}