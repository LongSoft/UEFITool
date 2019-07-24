/* fssreport.cpp

Copyright (c) 2016, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#include "ffsreport.h"
#include "ffs.h"
#include "utility.h"

std::vector<UString> FfsReport::generate()
{
    std::vector<UString> report;
    
    // Check model pointer
    if (!model) {
        report.push_back(usprintf("%s: invalid model pointer provided", __FUNCTION__));
        return report;
    }

    // Check root index to be valid
    UModelIndex root = model->index(0,0);
    if (!root.isValid()) {
        report.push_back(usprintf("%s: model root index is invalid", __FUNCTION__));
        return report;
    }

    // Generate report recursive
    report.push_back(UString("      Type       |        Subtype        |   Base   |   Size   |  CRC32   |   Name "));
    USTATUS result = generateRecursive(report, root);
    if (result) {
        report.push_back(usprintf("%s: generateRecursive returned ", __FUNCTION__) + errorCodeToUString(result));
    }

    return report;
}

USTATUS FfsReport::generateRecursive(std::vector<UString> & report, const UModelIndex & index, const UINT32 level)
{
    if (!index.isValid())
        return U_SUCCESS; // Nothing to report for invalid index
    
    // Calculate item CRC32
    UByteArray data = model->header(index) + model->body(index) + model->tail(index);
    UINT32 crc = (UINT32)crc32(0, (const UINT8*)data.constData(), data.size());

    // Information on current item
    UString text = model->text(index);
    UString offset = "|   N/A    ";
    if ((!model->compressed(index)) || (index.parent().isValid() && !model->compressed(index.parent()))) {
        offset = usprintf("| %08X ", model->base(index));
    }

    report.push_back(
        UString(" ") + itemTypeToUString(model->type(index)).leftJustified(16) 
        + UString("| ") + itemSubtypeToUString(model->type(index), model->subtype(index)).leftJustified(22)
        + offset
        + usprintf("| %08X | %08X | ", data.size(), crc) 
        + urepeated('-', level) + UString(" ") + model->name(index) + (text.isEmpty() ? UString() : UString(" | ") + text)
        );

    // Information on child items
    for (int i = 0; i < model->rowCount(index); i++) {
        generateRecursive(report, index.child(i,0), level + 1);
    }

    return U_SUCCESS;
}

