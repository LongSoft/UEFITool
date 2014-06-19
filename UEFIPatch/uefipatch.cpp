/* uefipatch.cpp

Copyright (c) 2014, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#include "uefipatch.h"

UEFIPatch::UEFIPatch(QObject *parent) :
    QObject(parent)
{
    ffsEngine = new FfsEngine(this);
    model = ffsEngine->treeModel();
}

UEFIPatch::~UEFIPatch()
{
    delete ffsEngine;
}

UINT8 UEFIPatch::patchFromFile(QString path)
{
    QFileInfo patchInfo = QFileInfo("patches.txt");

    if (!patchInfo.exists())
        return ERR_INVALID_FILE;

    QFile file;
    file.setFileName("patches.txt");

    if (!file.open(QFile::ReadOnly | QFile::Text))
        return ERR_INVALID_FILE;
    
    QFileInfo fileInfo = QFileInfo(path);

    if (!fileInfo.exists())
        return ERR_FILE_OPEN;

    QFile inputFile;
    inputFile.setFileName(path);

    if (!inputFile.open(QFile::ReadOnly))
        return ERR_FILE_READ;

    QByteArray buffer = inputFile.readAll();
    inputFile.close();

    UINT8 result = ffsEngine->parseImageFile(buffer);
    if (result)
        return result;

    while (!file.atEnd()) {
        QByteArray line = file.readLine();
        // Use sharp sign as commentary
        if (line.count() == 0 || line[0] == '#')
            continue;
        
        QList<QByteArray> list = line.split(' ');
        if (list.count() < 3)
            continue;
        
        QUuid uuid = QUuid(list.at(0));
        QByteArray guid = QByteArray::fromRawData((const char*)&uuid.data1, sizeof(EFI_GUID));
        result = patchFile(model->index(0, 0), guid, QByteArray::fromHex(list.at(1)), QByteArray::fromHex(list.at(2)));
        if (result)
            return result;
    }
    
    QByteArray reconstructed;
    result = ffsEngine->reconstructImageFile(reconstructed);
    if (result)
        return result;

    if (reconstructed == buffer) {
        return ERR_ITEM_NOT_FOUND;
    }

    QFile outputFile;
    outputFile.setFileName(path.append(".patched"));
    if (!outputFile.open(QFile::WriteOnly))
        return ERR_FILE_WRITE;

    outputFile.resize(0);
    outputFile.write(reconstructed);
    outputFile.close();

    return ERR_SUCCESS;
}

UINT8 UEFIPatch::patch(QString path, QString fileGuid, QString findPattern, QString replacePattern)
{
    QFileInfo fileInfo = QFileInfo(path);

    if (!fileInfo.exists())
        return ERR_FILE_OPEN;

    QFile inputFile;
    inputFile.setFileName(path);

    if (!inputFile.open(QFile::ReadOnly))
        return ERR_FILE_READ;

    QByteArray buffer = inputFile.readAll();
    inputFile.close();

    UINT8 result = ffsEngine->parseImageFile(buffer);
    if (result)
        return result;


    QUuid uuid = QUuid(fileGuid);
    QByteArray guid = QByteArray::fromRawData((const char*)&uuid.data1, sizeof(EFI_GUID));
    result = patchFile(model->index(0, 0), guid, QByteArray::fromHex(findPattern.toLatin1()), QByteArray::fromHex(replacePattern.toLatin1()));
    if (result)
        return result;

    QByteArray reconstructed;
    result = ffsEngine->reconstructImageFile(reconstructed);
    if (result)
        return result;

    if (reconstructed == buffer) {
        return ERR_ITEM_NOT_FOUND;
    }

    QFile outputFile;
    outputFile.setFileName(path.append(".patched"));
    if (!outputFile.open(QFile::WriteOnly))
        return ERR_FILE_WRITE;

    outputFile.resize(0);
    outputFile.write(reconstructed);
    outputFile.close();

    return ERR_SUCCESS;
}

UINT8 UEFIPatch::patchFile(const QModelIndex & index, const QByteArray & fileGuid, const QByteArray & findPattern, const QByteArray & replacePattern)
{
    if (!model || !index.isValid())
        return ERR_INVALID_PARAMETER;

    if (model->type(index) == Types::File && model->header(index).left(sizeof(EFI_GUID)) == fileGuid) {
        return ffsEngine->patch(index, findPattern, replacePattern, PATCH_MODE_BODY);
    }

    int childCount = model->rowCount(index);
    if (childCount > 0) {
        UINT8 result;
        for (int i = 0; i < childCount; i++) {
            result = patchFile(index.child(i, 0), fileGuid, findPattern, replacePattern);
            if (result)
                return result;
        }
    }
    
    return ERR_SUCCESS;
}