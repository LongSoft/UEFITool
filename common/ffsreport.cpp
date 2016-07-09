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

std::vector<UString> FfsReport::generate()
{
    std::vector<UString> report;
    
    // Check model pointer
    if (!model) {
        report.push_back(UString("ERROR: Invalid model pointer provided"));
        return report;
    }
    
    // Check root index to be valid
    UModelIndex root = model->index(0,0);
    if (!root.isValid()) {
        report.push_back(UString("ERROR: Model root index is invalid"));
        return report;
    }

    // Generate report recursive
    report.push_back(UString("      Type       |        Subtype        |   Size   |  CRC32   |   Name "));
    USTATUS result = generateRecursive(report, root);
    if (result) {
        report.push_back(UString("ERROR: generateRecursive returned ") + errorCodeToUString(result));
    }
        
    return report;
}

USTATUS FfsReport::generateRecursive(std::vector<UString> & report, UModelIndex index, UINT32 level)
{
    if (!index.isValid())
        return U_SUCCESS; //Nothing to report for invalid index
    
    // Calculate item CRC32
    UByteArray data = model->header(index) + model->body(index) + model->tail(index);
    UINT32 crc = crc32(0, (const UINT8*)data.constData(), data.size());

    // Information on current item
    UString text = model->text(index);
    report.push_back(
        UString(" ") + itemTypeToUString(model->type(index)).leftJustified(16) 
        + UString("| ") + itemSubtypeToUString(model->type(index), model->subtype(index)).leftJustified(22)
        + usprintf("| %08X | %08X | ", data.size(), crc) 
        + UString('-', level) + UString(" ") + model->name(index) + (text.isEmpty() ? UString("") : UString(" | ") + text)
        );
    
    // Information on child items
    for (int i = 0; i < model->rowCount(index); i++) {
        generateRecursive(report, index.child(i,0), level + 1);
    }
    
    return U_SUCCESS;
}
