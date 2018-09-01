/* uefireplace.h
Copyright (c) 2017, mxxxc. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*/

#ifndef __UEFIREPLACE_H__
#define __UEFIREPLACE_H__

#include <iterator>
#include <set>

#include "../common/basetypes.h"
#include "../common/ustring.h"
#include "../common/filesystem.h"
#include "../common/ffs.h"
#include "../common/ffsparser.h"
#include "../common/ffsops.h"
#include "../common/ffsbuilder.h"
#include "../common/utility.h"

class UEFIReplace
{
public:
    UEFIReplace();
    ~UEFIReplace();

    USTATUS replace(const UString & inPath, const EFI_GUID & guid, const UINT8 sectionType, const UString & contentPath, const UString & outPath, bool replaceAsIs, bool replaceOnce);

private:
    USTATUS replaceInFile(const UModelIndex & index, const EFI_GUID & guid, const UINT8 sectionType, const UByteArray & contents, const UINT8 mode, bool replaceOnce);
    TreeModel* model;
    FfsParser* ffsParser;
    FfsBuilder* ffsBuilder;
    FfsOperations* ffsOps;
};

#endif
