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

UINT8 UEFIPatch::patchFromFile(const QString & path, const QString & patches, const QString & outputPath)
{
    QFileInfo patchInfo = QFileInfo(patches);

    if (!patchInfo.exists())
        return ERR_INVALID_FILE;

    QFile file;
    file.setFileName(patches);

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

    UINT8 counter = 0;
    while (!file.atEnd()) {
        QByteArray line = file.readLine().trimmed();
        // Use sharp sign as commentary
        if (line.count() == 0 || line[0] == '#')
            continue;

        QList<QByteArray> list = line.split(' ');
        if (list.count() < 3)
            continue;
        
        QUuid uuid = QUuid(list.at(0));
        QByteArray guid = QByteArray::fromRawData((const char*)&uuid.data1, sizeof(EFI_GUID));
        bool converted;
        UINT8 sectionType = (UINT8)list.at(1).toUShort(&converted, 16);
        if (!converted)
            return ERR_INVALID_PARAMETER;
                
        QVector<PatchData> patches;

        for (int i = 2; i < list.count(); i++) {
            QList<QByteArray> patchList = list.at(i).split(':');
            PatchData patch;
            patch.type = *(UINT8*)patchList.at(0).constData();
            if (patch.type == PATCH_TYPE_PATTERN) {
                patch.offset = 0xFFFFFFFF;
                patch.hexFindPattern = patchList.at(1);
                patch.hexReplacePattern = patchList.at(2);
                patches.append(patch);
            }
            else if (patch.type == PATCH_TYPE_OFFSET) {
                patch.offset = patchList.at(1).toUInt(NULL, 16);
                patch.hexReplacePattern = patchList.at(2);
                patches.append(patch);
            }
            else {
                // Ignore unknown patch type
                continue;
            }
        }
        result = patchFile(model->index(0, 0), guid, sectionType, patches);
        if (result && result != ERR_NOTHING_TO_PATCH)
            return result;
        counter++;
    }
    
    QByteArray reconstructed;
    result = ffsEngine->reconstructImageFile(reconstructed);
    if (result)
        return result;
    if (reconstructed == buffer)
        return ERR_NOTHING_TO_PATCH;
    
    QFile outputFile;
    outputFile.setFileName(outputPath);
    if (!outputFile.open(QFile::WriteOnly))
        return ERR_FILE_WRITE;

    outputFile.resize(0);
    outputFile.write(reconstructed);
    outputFile.close();

    return ERR_SUCCESS;
}

UINT8 UEFIPatch::patchFromArg(const QString & path, const QString & patch, const QString & outputPath)
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

    if (patch != NULL && !patch.isEmpty()) {
        QByteArray line = patch.toUtf8();

        QList<QByteArray> list = line.split(' ');
        if (list.count() < 3)
            return ERR_INVALID_PARAMETER;

        QUuid uuid = QUuid(list.at(0));
        QByteArray guid = QByteArray::fromRawData((const char*)&uuid.data1, sizeof(EFI_GUID));
        bool converted;
        UINT8 sectionType = (UINT8)list.at(1).toUShort(&converted, 16);
        if (!converted)
            return ERR_INVALID_PARAMETER;

        QVector<PatchData> patches;

        for (int i = 2; i < list.count(); i++) {
            QList<QByteArray> patchList = list.at(i).split(':');
            PatchData patch;
            patch.type = *(UINT8*)patchList.at(0).constData();
            if (patch.type == PATCH_TYPE_PATTERN) {
                patch.offset = 0xFFFFFFFF;
                patch.hexFindPattern = patchList.at(1);
                patch.hexReplacePattern = patchList.at(2);
                patches.append(patch);
            }
            else if (patch.type == PATCH_TYPE_OFFSET) {
                patch.offset = patchList.at(1).toUInt(NULL, 16);
                patch.hexReplacePattern = patchList.at(2);
                patches.append(patch);
            }
            else {
                // Ignore unknown patch type
                continue;
            }
        }
        result = patchFile(model->index(0, 0), guid, sectionType, patches);
        if (result && result != ERR_NOTHING_TO_PATCH)
            return result;
    }

    QByteArray reconstructed;
    result = ffsEngine->reconstructImageFile(reconstructed);
    if (result)
        return result;
    if (reconstructed == buffer)
        return ERR_NOTHING_TO_PATCH;

    QFile outputFile;
    outputFile.setFileName(outputPath);
    if (!outputFile.open(QFile::WriteOnly))
        return ERR_FILE_WRITE;

    outputFile.resize(0);
    outputFile.write(reconstructed);
    outputFile.close();

    return ERR_SUCCESS;
}

UINT8 UEFIPatch::patchFile(const QModelIndex & index, const QByteArray & fileGuid, const UINT8 sectionType, const QVector<PatchData> & patches)
{
  
    if (!model || !index.isValid())
        return ERR_INVALID_PARAMETER;
    if (model->type(index) == Types::Section && model->subtype(index) == sectionType) {
        QModelIndex fileIndex = model->findParentOfType(index, Types::File);
        if (model->type(fileIndex) == Types::File &&
            model->header(fileIndex).left(sizeof(EFI_GUID)) == fileGuid)
        {
            return ffsEngine->patch(index, patches);
        }
    }

    if (model->rowCount(index) > 0) {
        for (int i = 0; i < model->rowCount(index); i++) {
            UINT8 result = patchFile(index.child(i, 0), fileGuid, sectionType, patches);
            if (!result)
                break;
            else if (result != ERR_NOTHING_TO_PATCH)
                return result;
        }
    }

    return ERR_NOTHING_TO_PATCH;
}
