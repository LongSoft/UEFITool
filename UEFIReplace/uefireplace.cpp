/* uefireplace.cpp

Copyright (c) 2017, mxxxc. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#include "uefireplace.h"

#include <cstring>

UEFIReplace::UEFIReplace()
{
    model = new TreeModel();
    ffsParser = new FfsParser(model);
    ffsBuilder = new FfsBuilder(model, ffsParser);
    ffsOps = new FfsOperations(model);
}

UEFIReplace::~UEFIReplace()
{
    delete ffsOps;
    delete ffsBuilder;
    delete ffsParser;
    delete model;
}

USTATUS UEFIReplace::replace(const UString & inPath, const EFI_GUID & guid, const UINT8 sectionType, const UString & contentPath, const UString & outPath, bool replaceAsIs, bool replaceOnce)
{
    UByteArray buffer;
    USTATUS result;

    result = readFileIntoArray(inPath, buffer);
    if (result)
        return result;

    result = ffsParser->parse(buffer);
    if (result)
        return result;

    UByteArray contents;
    result = readFileIntoArray(contentPath, contents);
    if (result)
        return result;

    result = replaceInFile(model->index(0, 0), guid, sectionType, contents,
        replaceAsIs ? REPLACE_MODE_AS_IS : REPLACE_MODE_BODY, replaceOnce);
    if (result)
        return result;

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

USTATUS UEFIReplace::replaceInFile(const UModelIndex & index, const EFI_GUID & guid, const UINT8 sectionType, const UByteArray & newData, const UINT8 mode, bool replaceOnce)
{
    if (!model || !index.isValid())
        return U_INVALID_PARAMETER;
    bool patched = false;
    if (model->subtype(index) == sectionType) {
        UModelIndex fileIndex = index;
        if (model->type(index) != Types::File)
            fileIndex = model->findParentOfType(index, Types::File);
        UByteArray fileGuid = model->header(fileIndex).left(sizeof(EFI_GUID));

        bool guidMatch = fileGuid.size() == sizeof(EFI_GUID) &&
            !std::memcmp((const EFI_GUID*)fileGuid.constData(), &guid, sizeof(EFI_GUID));
        if (!guidMatch && sectionType == EFI_SECTION_FREEFORM_SUBTYPE_GUID) {
            UByteArray subGuid = model->header(index).mid(sizeof(UINT32), sizeof(EFI_GUID));
            guidMatch = subGuid.size() == sizeof(EFI_GUID) &&
                !std::memcmp((const EFI_GUID*)subGuid.constData(), &guid, sizeof(EFI_GUID));;
        }
        if (guidMatch && model->action(index) != Actions::Replace) {
            UByteArray newDataCopy = newData;
            USTATUS result = ffsOps->replace(index, newDataCopy, mode);
            if (replaceOnce || (result != U_SUCCESS && result != U_NOTHING_TO_PATCH))
                return result;
            patched = result == U_SUCCESS;
        }
    }

    if (model->rowCount(index) > 0) {
        for (int i = 0; i < model->rowCount(index); i++) {
            USTATUS result = replaceInFile(index.child(i, 0), guid, sectionType, newData, mode, replaceOnce);
            if (result == U_SUCCESS) {
                patched = true;
                if (replaceOnce)
                    break;
            } else if (result != U_NOTHING_TO_PATCH) {
                return result;
            }
        }
    }

    return patched ? U_SUCCESS : U_NOTHING_TO_PATCH;
}
