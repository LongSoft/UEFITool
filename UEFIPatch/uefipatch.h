/* uefipatch.h

Copyright (c) 2018, LongSoft. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#ifndef __UEFIPATCH_H__
#define __UEFIPATCH_H__

#include <iterator>
#include <set>
#include <vector>

#include "../common/basetypes.h"
#include "../common/ustring.h"
#include "../common/filesystem.h"
#include "../common/ffs.h"
#include "../common/ffsparser.h"
#include "../common/ffsops.h"
#include "../common/ffsbuilder.h"
#include "../common/utility.h"

struct PatchData {
    UINT8 type;
    UINT32 offset;
    UByteArray hexFindPattern;
    UByteArray hexReplacePattern;
};

class UEFIPatch
{
public:
    explicit UEFIPatch();
    ~UEFIPatch();

    USTATUS patchFromFile(const UString & inPath, const UString & patches, const UString & outPath);
private:
    USTATUS patchFile(const UModelIndex & index, const UByteArray & fileGuid, const UINT8 sectionType, const std::vector<PatchData> & patches);
    TreeModel* model;
    FfsParser* ffsParser;
    FfsBuilder* ffsBuilder;
    FfsOperations* ffsOps;
};

#endif
