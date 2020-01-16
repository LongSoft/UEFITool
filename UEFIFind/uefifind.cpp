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
#include "../common/ffsutils.h"

#include <fstream>

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
    USTATUS result;
    UByteArray buffer;
    result = readFileIntoBuffer(path, buffer);
    if (result)
        return result;

    result = ffsParser->parse(buffer);
    if (result)
        return result;

    initDone = true;
    return U_SUCCESS;
}

USTATUS UEFIFind::find(const UINT8 mode, const bool count, const UString & hexPattern, UString & result)
{
    UModelIndex root = model->index(0, 0);
    std::set<std::pair<UModelIndex, UModelIndex> > files;

    result.clear();

    USTATUS returned = FfsUtils::findFileRecursive(model, root, hexPattern, mode, files);
    if (returned)
        return returned;
    
    if (count) {
        if (!files.empty())
            result += usprintf("%d\n", files.size());
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
