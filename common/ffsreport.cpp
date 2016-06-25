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

std::vector<QString> FfsReport::generate()
{
    std::vector<QString> report;
    
    // Check model pointer
    if (!model) {
        report.push_back(QObject::tr("ERROR: Invalid model pointer provided."));
        return report;
    }
    
    // Check root index to be valid
    QModelIndex root = model->index(0,0);
    if (!root.isValid()) {
        report.push_back(QObject::tr("ERROR: Model root index is invalid."));
        return report;
    }

    // Generate report recursive
    report.push_back(QObject::tr("      Type       |         Subtype          |   Size   |  CRC32   |   Name "));
    STATUS result = generateRecursive(report, root);
    if (result) {
        report.push_back(QObject::tr("ERROR: generateRecursive returned %1.")
        .arg(errorCodeToQString(result)));
    }
        
    return report;
}

STATUS FfsReport::generateRecursive(std::vector<QString> & report, QModelIndex index, UINT32 level)
{
    if (!index.isValid())
        return ERR_SUCCESS; //Nothing to report for invalid index
    
    // Calculate item CRC32
    QByteArray data = model->header(index) + model->body(index) + model->tail(index);
    UINT32 crc = crc32(0, (const UINT8*)data.constData(), data.size());

    // Information on current item
    QString text = model->text(index);
    report.push_back(QObject::tr("%1 | %2 | %3 | %4 | %5 %6 %7")
        .arg(itemTypeToQString(model->type(index)), 16, QChar(' '))
        .arg(itemSubtypeToQString(model->type(index), model->subtype(index)), 24, QChar(' '))
        .hexarg2(data.size(), 8)
        .hexarg2(crc, 8)
        .arg(' ', level, QChar('-'))
        .arg(model->name(index))
        .arg(text.isEmpty() ? "" : text.prepend("| "))
        );
    
    // Information on child items
    for (int i = 0; i < model->rowCount(index); i++) {
        generateRecursive(report, index.child(i,0), level + 1);
    }
    
    return ERR_SUCCESS;
}
