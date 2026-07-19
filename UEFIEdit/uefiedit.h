/* uefiedit.h

Copyright (c) 2026, LongSoft. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#ifndef UEFIEDIT_H
#define UEFIEDIT_H

#include "../common/basetypes.h"
#include "../common/ustring.h"
#include "../common/ubytearray.h"
#include "../common/umemstream.h"
#include "../common/filesystem.h"
#include "../common/treemodel.h"
#include "../common/parsingdata.h"
#include "../common/ffsparser.h"
#include "../common/ffsops.h"
#include "../common/ffsbuilder.h"
#include "../common/ffs.h"
#include "../common/utility.h"

class UEFIEdit
{
public:
    explicit UEFIEdit();
    ~UEFIEdit();

    USTATUS init(const UString & imagePath);
    USTATUS save(const UString & outputPath);

    // Insert a file or section. The dataPath file is parsed:
    //   - if the target is a Volume, the data must be a FFS file (.ffs)
    //   - if the target is a File or encapsulation Section, the data must be a section (.sct)
    // mode is one of CREATE_MODE_PREPEND / CREATE_MODE_BEFORE / CREATE_MODE_AFTER.
    // guidStr selects the target item by FFS file GUID (case-insensitive, with or without dashes).
    USTATUS insert(const UString & guidStr, const UINT8 mode, const UString & dataPath);

    // Remove the item identified by guidStr (FFS file GUID) or by hex path "0:1:2..."
    USTATUS remove(const UString & guidStr);

    // Replace body or full item (as-is) for the item identified by guidStr.
    USTATUS replace(const UString & guidStr, const UINT8 mode, const UString & dataPath);

    // Mark an item (and its ancestors) for rebuild.
    USTATUS rebuild(const UString & guidStr);

    // Print the tree to stderr for debugging.
    void dumpTree();

private:
    TreeModel* model;
    FfsParser* ffsParser;
    FfsOperations* ffsOps;
    FfsBuilder* ffsBuilder;
    UByteArray originalBuffer;
    bool initDone;

    UModelIndex findItemByGuid(const UString & guidStr);
    UModelIndex findItemByGuidRecursive(const UModelIndex & parent, const EFI_GUID & guid);
    UModelIndex findFileByGuid(const UString & guidStr);
};

#endif // UEFIEDIT_H