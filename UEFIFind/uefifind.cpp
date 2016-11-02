/* uefifind.cpp

Copyright (c) 2016, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#include "uefifind.h"

UEFIFind::UEFIFind()
{
    model = new TreeModel();
    ffsParser = new FfsParser(model);
    initDone = false;
}

UEFIFind::~UEFIFind()
{
    delete ffsParser;
    delete model;
    model = NULL;
}

USTATUS UEFIFind::init(const QString & path)
{
    USTATUS result;
    
    fileInfo = QFileInfo(path);

    if (!fileInfo.exists())
        return U_FILE_OPEN;

    QFile inputFile;
    inputFile.setFileName(path);

    if (!inputFile.open(QFile::ReadOnly))
        return U_FILE_OPEN;

    QByteArray buffer = inputFile.readAll();
    inputFile.close();

    result = ffsParser->parse(buffer);
    if (result)
        return result;

    initDone = true;
    return U_SUCCESS;
}

USTATUS UEFIFind::find(const UINT8 mode, const bool count, const QString & hexPattern, QString & result)
{
    QModelIndex root = model->index(0, 0);
    std::set<std::pair<QModelIndex, QModelIndex> > files;

    result.clear();

    USTATUS returned = findFileRecursive(root, hexPattern, mode, files);
    if (returned)
        return returned;
    
    if (count) {
        if (!files.empty())
            result.append(QString("%1\n").arg(files.size()));
        return U_SUCCESS;
    }

    for (std::set<std::pair<QModelIndex, QModelIndex> >::const_iterator citer = files.begin(); citer != files.end(); ++citer) {
        QByteArray data(16, '\x00');
        std::pair<QModelIndex, QModelIndex> indexes = *citer;
        if (!model->hasEmptyHeader(indexes.first))
            data = model->header(indexes.first).left(16);
        result.append(guidToUString(*(const EFI_GUID*)data.constData()));

        // Special case of freeform subtype GUID files
        if (indexes.second.isValid() && model->subtype(indexes.second) == EFI_SECTION_FREEFORM_SUBTYPE_GUID) {
            data = model->header(indexes.second).left(sizeof(EFI_FREEFORM_SUBTYPE_GUID_SECTION));
            result.append(" ").append(guidToUString(*(const EFI_GUID*)(data.constData() + sizeof(EFI_COMMON_SECTION_HEADER))));
        }
        
        result.append("\n");
    }
    return U_SUCCESS;
}

USTATUS UEFIFind::findFileRecursive(const QModelIndex index, const QString & hexPattern, const UINT8 mode, std::set<std::pair<QModelIndex, QModelIndex> > & files)
{
    if (!index.isValid())
        return U_SUCCESS;

    if (hexPattern.isEmpty())
        return U_INVALID_PARAMETER;

    // Check for "all substrings" pattern
    if (hexPattern.count('.') == hexPattern.length())
        return U_SUCCESS;

    bool hasChildren = (model->rowCount(index) > 0);
    for (int i = 0; i < model->rowCount(index); i++) {
        findFileRecursive(index.child(i, index.column()), hexPattern, mode, files);
    }

    QByteArray data;
    if (hasChildren) {
        if (mode == SEARCH_MODE_HEADER || mode == SEARCH_MODE_ALL)
            data.append(model->header(index));
    }
    else {
        if (mode == SEARCH_MODE_HEADER)
            data.append(model->header(index));
        else if (mode == SEARCH_MODE_BODY)
            data.append(model->body(index));
        else
            data.append(model->header(index)).append(model->body(index));
    }

    QString hexBody = QString(data.toHex());
    QRegExp regexp = QRegExp(QString(hexPattern), Qt::CaseInsensitive);
    INT32 offset = regexp.indexIn(hexBody);
    while (offset >= 0) {
        if (offset % 2 == 0) {
            if (model->type(index) != Types::File) {
                QModelIndex ffs = model->findParentOfType(index, Types::File);
                if (model->type(index) == Types::Section && model->subtype(index) == EFI_SECTION_FREEFORM_SUBTYPE_GUID)
                    files.insert(std::pair<QModelIndex, QModelIndex>(ffs, index));
                else
                    files.insert(std::pair<QModelIndex, QModelIndex>(ffs, QModelIndex()));
            }
            else
                files.insert(std::pair<QModelIndex, QModelIndex>(index, QModelIndex()));

            break;
        }
        offset = regexp.indexIn(hexBody, offset + 1);
    }

    return U_SUCCESS;
}