/* uefidump.h

Copyright (c) 2016, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#ifndef UEFIDUMP_H
#define UEFIDUMP_H

#include <string>

#include "../common/basetypes.h"
#include "../common/ustring.h"
#include "../common/treemodel.h"
#include "../common/ffsparser.h"
#include "../common/ffsreport.h"
#include "../common/fitparser.h"


class UEFIDumper
{
public:
    explicit UEFIDumper() : model(), ffsParser(&model), ffsReport(&model), fitParser(&model), currentBuffer(), initialized(false), dumped(false) {}
    ~UEFIDumper() {}

    USTATUS dump(const UByteArray & buffer, const std::wstring & path, const std::wstring & guid = std::wstring());

private:
    USTATUS recursiveDump(const UModelIndex & root, const std::wstring & path, const std::wstring & guid);
    std::wstring guidToWstring(const EFI_GUID & guid);
    bool createFullPath(const std::wstring & path);

    TreeModel model;
    FfsParser ffsParser;
    FfsReport ffsReport;
    FitParser fitParser;
    
    UByteArray currentBuffer;
    bool initialized;
    bool dumped;
};

#endif
