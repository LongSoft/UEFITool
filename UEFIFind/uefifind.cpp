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

UINT8 UEFIFind::find(const UINT8 mode, const bool count, const QString & hexPattern, QString & result)
{
    QModelIndex root = model->index(0, 0);
    QSet<QModelIndex> files;

    result.clear();

    UINT8 returned = findFileRecursive(root, hexPattern, mode, files);
    if (returned)
        return returned;
    
    if (count) {
        if (files.count())
            result.append(QString("%1\n").arg(files.count()));
        return ERR_SUCCESS;
    }

    QModelIndex index;
    Q_FOREACH(index, files) {
        QByteArray data = model->header(index).left(16);

        UINT32 u32   = *(UINT32*)data.constData();
        UINT16 u16_1 = *(UINT16*)(data.constData() + 4);
        UINT16 u16_2 = *(UINT16*)(data.constData() + 6);
        UINT8  u8_1  = *(UINT8*)(data.constData()  + 8);
        UINT8  u8_2  = *(UINT8*)(data.constData()  + 9);
        UINT8  u8_3  = *(UINT8*)(data.constData()  + 10);
        UINT8  u8_4  = *(UINT8*)(data.constData()  + 11);
        UINT8  u8_5  = *(UINT8*)(data.constData()  + 12);
        UINT8  u8_6  = *(UINT8*)(data.constData()  + 13);
        UINT8  u8_7  = *(UINT8*)(data.constData()  + 14);
        UINT8  u8_8  = *(UINT8*)(data.constData()  + 15);

        QString guid = QString("%1-%2-%3-%4%5-%6%7%8%9%10%11\n").hexarg(u32, 8).hexarg(u16_1, 4).hexarg(u16_2, 4).hexarg(u8_1, 2).hexarg(u8_2, 2)
            .hexarg(u8_3, 2).hexarg(u8_4, 2).hexarg(u8_5, 2).hexarg(u8_6, 2).hexarg(u8_7, 2).hexarg(u8_8, 2);

        result.append(guid);
    }
    return ERR_SUCCESS;
}

UINT8 UEFIFind::findFileRecursive(const QModelIndex index, const QString & hexPattern, const UINT8 mode, QSet<QModelIndex> & files)
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
            data.append(model->header(index)).append(model->body(index)).append(model->tail(index));
    }

    QString hexBody = QString(data.toHex());
    QRegExp regexp = QRegExp(QString(hexPattern), Qt::CaseInsensitive);
    INT32 offset = regexp.indexIn(hexBody);
    while (offset >= 0) {
        if (offset % 2 == 0) {
            if (model->type(index) != Types::File) {
                QModelIndex ffs = model->findParentOfType(index, Types::File);
                files.insert(ffs);
            }
            else
                files.insert(index);

            break;
        }
        offset = regexp.indexIn(hexBody, offset + 1);
    }

    return ERR_SUCCESS;
}