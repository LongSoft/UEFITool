/* ffsutils.cpp

Copyright (c) 2019, LongSoft. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#include "ffsutils.h"
#include "utility.h"
#include "ffs.h"

namespace FfsUtils {

USTATUS findFileRecursive(TreeModel *model, const UModelIndex index, const UString & hexPattern, const UINT8 mode, std::set<std::pair<UModelIndex, UModelIndex> > & files)
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
        findFileRecursive(model, index.child(i, index.column()), hexPattern, mode, files);
    }

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
            UModelIndex ffs = model->findParentOfType(index, Types::File);
            if (model->type(index) == Types::Section && model->subtype(index) == EFI_SECTION_FREEFORM_SUBTYPE_GUID)
                files.insert(std::pair<UModelIndex, UModelIndex>(ffs, index));
            else
                files.insert(std::pair<UModelIndex, UModelIndex>(ffs, UModelIndex()));
        }
        else {
            files.insert(std::pair<UModelIndex, UModelIndex>(index, UModelIndex()));
        }

    }

    return U_SUCCESS;
}

};
