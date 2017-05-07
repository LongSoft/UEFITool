/* uefireplace.cpp

Copyright (c) 2017, mxxxc. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#include "uefireplace.h"

UEFIReplace::UEFIReplace(QObject *parent) :
    QObject(parent)
{
    ffsEngine = new FfsEngine(this);
    model = ffsEngine->treeModel();
}

UEFIReplace::~UEFIReplace()
{
    delete ffsEngine;
}

UINT8 UEFIReplace::replace(QString inPath, const QByteArray & guid, const UINT8 sectionType, const QString contentPath)
{
    QFileInfo fileInfo = QFileInfo(inPath);
    if (!fileInfo.exists())
        return ERR_FILE_OPEN;

    fileInfo = QFileInfo(contentPath);
    if (!fileInfo.exists())
        return ERR_FILE_OPEN;

    QFile inputFile;
    inputFile.setFileName(inPath);

    if (!inputFile.open(QFile::ReadOnly))
        return ERR_FILE_READ;

    QByteArray buffer = inputFile.readAll();
    inputFile.close();

    UINT8 result = ffsEngine->parseImageFile(buffer);
    if (result)
        return result;

    QFile contentFile;
    contentFile.setFileName(contentPath);

    if (!contentFile.open(QFile::ReadOnly))
        return ERR_FILE_READ;

    QByteArray contents = contentFile.readAll();
    contentFile.close();

    result = replaceInFile(model->index(0, 0), guid, sectionType, contents);
    if (result)
        return result;

    QByteArray reconstructed;
    result = ffsEngine->reconstructImageFile(reconstructed);
    if (result)
        return result;
    if (reconstructed == buffer)
        return ERR_NOTHING_TO_PATCH;

    QFile outputFile;
    outputFile.setFileName(inPath.append(".patched"));
    if (!outputFile.open(QFile::WriteOnly))
        return ERR_FILE_WRITE;

    outputFile.resize(0);
    outputFile.write(reconstructed);
    outputFile.close();

    return ERR_SUCCESS;
}

UINT8 UEFIReplace::replaceInFile(const QModelIndex & index, const QByteArray & guid, const UINT8 sectionType, const QByteArray & newData)
{
    if (!model || !index.isValid())
        return ERR_INVALID_PARAMETER;
    if (model->subtype(index) == sectionType) {
        QModelIndex fileIndex = model->findParentOfType(index, Types::File);
        QByteArray fileGuid = model->header(fileIndex).left(sizeof(EFI_GUID));
        if (fileGuid == guid) {
            return ffsEngine->replace(index, newData, REPLACE_MODE_BODY);
        }
    }

    bool patched = false;
    if (model->rowCount(index) > 0) {
        for (int i = 0; i < model->rowCount(index); i++) {
            UINT8 result = replaceInFile(index.child(i, 0), guid, sectionType, newData);
            if (!result) {
                patched = true;
                break;
            } else if (result != ERR_NOTHING_TO_PATCH)
                return result;
        }
    }

    return patched ? ERR_SUCCESS : ERR_NOTHING_TO_PATCH;
}
