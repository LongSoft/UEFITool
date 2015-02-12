/* uefifind.cpp

Copyright (c) 2014, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#include "uefifind.h"

UEFIFind::UEFIFind(QObject *parent) :
    QObject(parent)
{
    ffsEngine = new FfsEngine(this);
    model = ffsEngine->treeModel();
    initDone = false;
}

UEFIFind::~UEFIFind()
{
    model = NULL;
    delete ffsEngine;
}

UINT8 UEFIFind::init(const QString & path)
{
    UINT8 result;
    
    fileInfo = QFileInfo(path);

    if (!fileInfo.exists())
        return ERR_FILE_OPEN;

    QFile inputFile;
    inputFile.setFileName(path);

    if (!inputFile.open(QFile::ReadOnly))
        return ERR_FILE_OPEN;

    QByteArray buffer = inputFile.readAll();
    inputFile.close();

    result = ffsEngine->parseImageFile(buffer);
    if (result)
        return result;

    initDone = true;
    return ERR_SUCCESS;
}

QString UEFIFind::guidToQString(const UINT8* guid)
{
    const UINT32 u32 = *(const UINT32*)guid;
    const UINT16 u16_1 = *(const UINT16*)(guid + 4);
    const UINT16 u16_2 = *(const UINT16*)(guid + 6);
    const UINT8  u8_1 = *(const UINT8*)(guid + 8);
    const UINT8  u8_2 = *(const UINT8*)(guid + 9);
    const UINT8  u8_3 = *(const UINT8*)(guid + 10);
    const UINT8  u8_4 = *(const UINT8*)(guid + 11);
    const UINT8  u8_5 = *(const UINT8*)(guid + 12);
    const UINT8  u8_6 = *(const UINT8*)(guid + 13);
    const UINT8  u8_7 = *(const UINT8*)(guid + 14);
    const UINT8  u8_8 = *(const UINT8*)(guid + 15);

    return QString("%1-%2-%3-%4%5-%6%7%8%9%10%11").hexarg2(u32, 8).hexarg2(u16_1, 4).hexarg2(u16_2, 4).hexarg2(u8_1, 2).hexarg2(u8_2, 2)
        .hexarg2(u8_3, 2).hexarg2(u8_4, 2).hexarg2(u8_5, 2).hexarg2(u8_6, 2).hexarg2(u8_7, 2).hexarg2(u8_8, 2);
}

UINT8 UEFIFind::find(const UINT8 mode, const bool count, const QString & hexPattern, QString & result)
{
    QModelIndex root = model->index(0, 0);
    QSet<QPair<QModelIndex, QModelIndex> > files;

    result.clear();

    UINT8 returned = findFileRecursive(root, hexPattern, mode, files);
    if (returned)
        return returned;
    
    if (count) {
        if (files.count())
            result.append(QString("%1\n").arg(files.count()));
        return ERR_SUCCESS;
    }

    QPair<QModelIndex, QModelIndex> indexes;
    Q_FOREACH(indexes, files) {
        QByteArray data = model->header(indexes.first).left(16);
        result.append(guidToQString((const UINT8*)data.constData()));

        // Special case of freeform subtype GUID files
        if (indexes.second.isValid() && model->subtype(indexes.second) == EFI_SECTION_FREEFORM_SUBTYPE_GUID) {
            data = model->header(indexes.second).left(sizeof(EFI_FREEFORM_SUBTYPE_GUID_SECTION));
            result.append(" ").append(guidToQString((const UINT8*)data.constData() + sizeof(EFI_COMMON_SECTION_HEADER)));
        }
        
        result.append("\n");

    }
    return ERR_SUCCESS;
}

UINT8 UEFIFind::findFileRecursive(const QModelIndex index, const QString & hexPattern, const UINT8 mode, QSet<QPair<QModelIndex, QModelIndex> > & files)
{
    if (!index.isValid())
        return ERR_SUCCESS;

    if (hexPattern.isEmpty())
        return ERR_INVALID_PARAMETER;

    // Check for "all substrings" pattern
    if (hexPattern.count('.') == hexPattern.length())
        return ERR_SUCCESS;

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
                    files.insert(QPair<QModelIndex, QModelIndex>(ffs, index));
                else
                    files.insert(QPair<QModelIndex, QModelIndex>(ffs, QModelIndex()));
            }
            else
                files.insert(QPair<QModelIndex, QModelIndex>(index, QModelIndex()));

            break;
        }
        offset = regexp.indexIn(hexBody, offset + 1);
    }

    return ERR_SUCCESS;
}