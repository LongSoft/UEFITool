/* ffsdumper.h

Copyright (c) 2015, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#ifndef FFSDUMPER_H
#define FFSDUMPER_H


#include <QDir>
#include "../common/ubytearray.h"
#include "../common/ustring.h"
#include "../common/umodelindex.h"
#include "../common/basetypes.h"
#include "../common/treemodel.h"
#include "../common/ffs.h"

class FfsDumper
{
public:
    explicit FfsDumper(TreeModel * treeModel);
    ~FfsDumper();

    USTATUS dump(const UModelIndex & root, const UString & path, const bool dumpAll = false, const UString & guid = UString());

private:
    USTATUS recursiveDump(const UModelIndex & root, const UString & path, const bool dumpAll, const UString & guid);
    TreeModel* model;
    bool dumped;
};

#endif // FFSDUMPER_H
