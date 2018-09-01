/* uefipatch.cpp

Copyright (c) 2018, LongSoft. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#include "uefipatch.h"

UEFIPatch::UEFIPatch()
{
    model = new TreeModel();
    ffsParser = new FfsParser(model);
    ffsBuilder = new FfsBuilder(model, ffsParser);
    ffsOps = new FfsOperations(model);
}

UEFIPatch::~UEFIPatch()
{
    delete ffsOps;
    delete ffsBuilder;
    delete ffsParser;
    delete model;
}

USTATUS UEFIPatch::patchFromFile(const UString & inPath, const UString & patches, const UString & outPath)
{
    if (!isExistOnFs(patches))
        return U_INVALID_FILE;

    std::ifstream patchesFile(patches.toLocal8Bit());
    if (!patchesFile)
        return U_INVALID_FILE;
    
    UByteArray buffer;
    USTATUS result = readFileIntoArray(inPath, buffer);
    if (result)
        return result;

    result = ffsParser->parse(buffer);
    if (result)
        return result;

    UINT8 counter = 0;
    while (!patchesFile.eof()) {
        std::string line;
        std::getline(patchesFile, line);
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

        if (list.size() < 3)
            continue;

        EFI_GUID guid;
        const char *sectionTypeStr = list[1].toLocal8Bit();
        char *converted = const_cast<char *>(sectionTypeStr);
        UINT8 sectionType = (UINT8)std::strtol(sectionTypeStr, &converted, 16);
        if (converted == sectionTypeStr || !ustringToGuid(list[0], guid))
            return U_INVALID_PARAMETER;
                
        std::vector<PatchData> patches;

        for (size_t i = 2; i < list.size(); i++) {
            std::vector<UByteArray> patchList;
            std::string patchStr = list.at(i).toLocal8Bit();
            std::string::size_type prev = 0, curr = 0;
            while ((curr = line.find(':', curr)) != std::string::npos) {
                std::string substring( line.substr(prev, curr-prev) );
                patchList.push_back(UByteArray(substring));
                prev = ++curr;
            }
            patchList.push_back(UByteArray(line.substr(prev, curr-prev)));

            PatchData patch;
            patch.type = *(UINT8*)patchList.at(0).constData();
            if (patch.type == PATCH_TYPE_PATTERN) {
                patch.offset = 0xFFFFFFFF;
                patch.hexFindPattern = patchList.at(1);
                patch.hexReplacePattern = patchList.at(2);
                patches.push_back(patch);
            }
            else if (patch.type == PATCH_TYPE_OFFSET) {
                patch.offset = patchList.at(1).toUInt(NULL, 16);
                patch.hexReplacePattern = patchList.at(2);
                patches.push_back(patch);
            }
            else {
                // Ignore unknown patch type
                continue;
            }
        }
        result = patchFile(model->index(0, 0), UByteArray((const char*)&guid, sizeof(EFI_GUID)), sectionType, patches);
        if (result && result != U_NOTHING_TO_PATCH)
            return result;
        counter++;
    }
    
    UByteArray reconstructed;
    result = ffsBuilder->build(model->index(0,0), reconstructed);
    if (result)
        return result;
    if (reconstructed == buffer)
        return U_NOTHING_TO_PATCH;

    std::ofstream outputFile;
    outputFile.open(outPath.toLocal8Bit(), std::ios::out | std::ios::binary);
    outputFile.write(reconstructed.constData(), reconstructed.size());
    outputFile.close();

    return U_SUCCESS;
}

USTATUS UEFIPatch::patchFile(const UModelIndex & index, const UByteArray & fileGuid, const UINT8 sectionType, const std::vector<PatchData> & patches)
{
    if (!model || !index.isValid())
        return U_INVALID_PARAMETER;
    if (model->type(index) == Types::Section && model->subtype(index) == sectionType) {
        UModelIndex fileIndex = model->findParentOfType(index, Types::File);
        if (model->type(fileIndex) == Types::File &&
            model->header(fileIndex).left(sizeof(EFI_GUID)) == fileGuid)
        {
            // return ffsOps->patch(index, patches);
            #warning "ffsOps->patch is not implemented!"
            return U_NOTHING_TO_PATCH;
        }
    }

    if (model->rowCount(index) > 0) {
        for (int i = 0; i < model->rowCount(index); i++) {
            USTATUS result = patchFile(index.child(i, 0), fileGuid, sectionType, patches);
            if (!result)
                break;
            else if (result != U_NOTHING_TO_PATCH)
                return result;
        }
    }

    return U_NOTHING_TO_PATCH;
}
