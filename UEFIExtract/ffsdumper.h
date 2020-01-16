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

#include <set>

#include "../common/basetypes.h"
#include "../common/ustring.h"
#include "../common/treemodel.h"
#include "../common/ffs.h"
#include "../common/filesystem.h"
#include "../common/utility.h"

class FfsDumper
{
public:
    enum DumpMode {
        DUMP_CURRENT,
        DUMP_ALL,
        DUMP_BODY,
        DUMP_HEADER,
        DUMP_INFO,
        DUMP_FILE
    };

    static const UINT8 IgnoreSectionType = 0xFF;

    explicit FfsDumper(TreeModel * treeModel) : model(treeModel), dumped(false), 
        counterHeader(0), counterBody(0), counterRaw(0), counterInfo(0) {}
    ~FfsDumper() {};

    USTATUS dump(const UModelIndex & root, const UString & path, const DumpMode dumpMode = DUMP_CURRENT, const UINT8 sectionType = IgnoreSectionType, const UString & guid = UString());

private:
    USTATUS recursiveDump(const UModelIndex & root, const UString & path, const DumpMode dumpMode, const UINT8 sectionType, const UString & guid);
    TreeModel* model;
    UString currentPath;
    bool dumped;
    int counterHeader, counterBody, counterRaw, counterInfo;
    std::set<UModelIndex> fileList;
};
#endif // FFSDUMPER_H
