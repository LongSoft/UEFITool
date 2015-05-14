/* fssops.cpp

Copyright (c) 2015, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#include "ffsops.h"

FfsOperations::FfsOperations(const TreeModel* treeModel, QObject *parent)
    : QObject(parent), model(treeModel)
{
}

FfsOperations::~FfsOperations()
{
}

void FfsOperations::msg(const QString & message, const QModelIndex & index)
{
    messagesVector.push_back(QPair<QString, QModelIndex>(message, index));
}

QVector<QPair<QString, QModelIndex> > FfsOperations::getMessages() const
{
    return messagesVector;
}

void FfsOperations::clearMessages()
{
    messagesVector.clear();
}

STATUS FfsOperations::extract(const QModelIndex & index, QString & name, QByteArray & extracted, const UINT8 mode)
{
    // Sanity check
    if (!index.isValid())
        return ERR_INVALID_PARAMETER;

    // Get data from parsing data
    PARSING_DATA pdata = parsingDataFromQByteArray(index);

    // Construct a name for extracted data
    QString itemName = model->name(index);
    QString itemText = model->text(index);
    switch (model->type(index)) {
    case Types::Volume: {
        if (pdata.volume.hasExtendedHeader)
            name = guidToQString(pdata.volume.extendedHeaderGuid);
        else
            name = itemName;
    } break;
    case Types::File: {
        name = itemText.isEmpty() ? itemName : itemText.replace(' ', '_');
    } break;
    case Types::Section: {
        // Get parent file name
        QModelIndex fileIndex = model->findParentOfType(index, Types::File);
        QString fileText = model->text(fileIndex);
        name = fileText.isEmpty() ? model->name(fileIndex) : fileText.replace(' ', '_');
        // Append section subtype name
        name += QChar('_') + itemName.replace(' ', '_');
    } break;

    case Types::Capsule:
    case Types::Image:
    case Types::Region:
    case Types::Padding:
    default:
        name = itemName.replace(' ', '_').replace('/', '_');
    }

    // Get extracted data
    if (mode == EXTRACT_MODE_AS_IS) {
        // Extract as is, with header body and tail
        extracted.clear();
        extracted.append(model->header(index));
        extracted.append(model->body(index));
        // Handle file tail
        if (model->type(index) == Types::File) {
            if (pdata.file.hasTail)
                extracted.append(pdata.file.tail);
        }
    }
    else if (mode == EXTRACT_MODE_BODY) {
        name += tr("_body");
        // Extract without header and tail
        extracted.clear();
        extracted.append(model->body(index));
    }
    else
        return ERR_UNKNOWN_EXTRACT_MODE;

    return ERR_SUCCESS;
}

