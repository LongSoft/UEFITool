/* uefifind.cpp

Copyright (c) 2016, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#include "uefifind.h"
#include <fstream>
#include <set>


UEFIFind::UEFIFind()
{
    model = new TreeModel();
    ffsParser = new FfsParser(model);
    initDone = false;
}

UEFIFind::~UEFIFind()
{
    delete ffsParser;
    delete model;
    model = NULL;
}

USTATUS UEFIFind::init(const UString & path)
{
    UByteArray buffer;
    if (false == readFileIntoBuffer(path, buffer))
        return U_FILE_OPEN;

    USTATUS result = ffsParser->parse(buffer);
    if (result)
        return result;

    initDone = true;
    return U_SUCCESS;
}

USTATUS UEFIFind::findFileRecursive(const UModelIndex index, const UString & hexPattern, const UINT8 mode, std::set<std::pair<UModelIndex, UModelIndex> > & files)
{
    if (!index.isValid())
        return U_SUCCESS;

    if (hexPattern.isEmpty())
        return U_INVALID_PARAMETER;

    const char *hexPatternRaw = hexPattern.toLocal8Bit();
    std::vector<UINT8> pattern, patternMask;
    if (!makePattern(hexPatternRaw, pattern, patternMask))
        return U_INVALID_PARAMETER;

    // Check for "all substrings" pattern
    size_t count = 0;
    for (size_t i = 0; i < patternMask.size(); i++)
        if (patternMask[i] == 0)
            count++;
    if (count == patternMask.size())
        return U_SUCCESS;

    bool hasChildren = (model->rowCount(index) > 0);
    for (int i = 0; i < model->rowCount(index); i++) {
        findFileRecursive(index.model()->index(i, index.column(), index), hexPattern, mode, files);
    }

    // TODO: handle a case where an item has both compressed and uncompressed bodies
    UByteArray data;
    if (hasChildren) {
        if (mode == SEARCH_MODE_HEADER)
            data = model->header(index);
        else if (mode == SEARCH_MODE_ALL)
            data = model->header(index) + model->body(index);
    }
    else {
        if (mode == SEARCH_MODE_HEADER)
            data = model->header(index);
        else if (mode == SEARCH_MODE_BODY)
            data = model->body(index);
        else
            data = model->header(index) + model->body(index);
    }

    const UINT8 *rawData = reinterpret_cast<const UINT8 *>(data.constData());
    INTN offset = findPattern(pattern.data(), patternMask.data(), pattern.size(), rawData, data.size(), 0);

    // For patterns that cross header|body boundary, skip patterns entirely located in body, since
    // children search above has already found them.
    if (hasChildren && mode == SEARCH_MODE_ALL && offset >= model->header(index).size()) {
        offset = -1;
    }

    if (offset >= 0) {
        if (model->type(index) != Types::File) {
            UModelIndex parentFile = model->findParentOfType(index, Types::File);
            if (model->type(index) == Types::Section && model->subtype(index) == EFI_SECTION_FREEFORM_SUBTYPE_GUID)
                files.insert(std::pair<UModelIndex, UModelIndex>(parentFile, index));
            else
                files.insert(std::pair<UModelIndex, UModelIndex>(parentFile, UModelIndex()));
        }
        else {
            files.insert(std::pair<UModelIndex, UModelIndex>(index, UModelIndex()));
        }
    }

    return U_SUCCESS;
}

USTATUS UEFIFind::find(const UINT8 mode, const bool count, const UString & hexPattern, UString & result)
{
    UModelIndex root = model->index(0, 0);
    std::set<std::pair<UModelIndex, UModelIndex> > files;

    result.clear();

    USTATUS returned = findFileRecursive(root, hexPattern, mode, files);
    if (returned)
        return returned;
    
    if (count) {
        if (!files.empty())
            result += usprintf("%lu\n", files.size());
        return U_SUCCESS;
    }

    for (std::set<std::pair<UModelIndex, UModelIndex> >::const_iterator citer = files.begin(); citer != files.end(); ++citer) {
        UByteArray data(16, '\x00');
        std::pair<UModelIndex, UModelIndex> indexes = *citer;
        if (!model->hasEmptyHeader(indexes.first))
            data = model->header(indexes.first).left(16);
        result += guidToUString(readUnaligned((const EFI_GUID*)data.constData()));

        // Special case of freeform subtype GUID files
        if (indexes.second.isValid() && model->subtype(indexes.second) == EFI_SECTION_FREEFORM_SUBTYPE_GUID) {
            data = model->header(indexes.second);
            result += UString(" ") + (guidToUString(readUnaligned((const EFI_GUID*)(data.constData() + sizeof(EFI_COMMON_SECTION_HEADER)))));
        }
        
        result += UString("\n");
    }
    return U_SUCCESS;
}
