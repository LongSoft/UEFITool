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

UINT8 UEFIReplace::replace(const QString & inPath, const QByteArray & guid, const UINT8 sectionType, const QString & contentPath, const QString & outPath, bool replaceAsIs, bool replaceOnce)
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

    result = replaceInFile(model->index(0, 0), guid, sectionType, contents,
        replaceAsIs ? REPLACE_MODE_AS_IS : REPLACE_MODE_BODY, replaceOnce);
    if (result)
        return result;

    QByteArray reconstructed;
    result = ffsEngine->reconstructImageFile(reconstructed);
    if (result)
        return result;
    if (reconstructed == buffer)
        return ERR_NOTHING_TO_PATCH;

    QFile outputFile;
    outputFile.setFileName(outPath);
    if (!outputFile.open(QFile::WriteOnly))
        return ERR_FILE_WRITE;

    outputFile.resize(0);
    outputFile.write(reconstructed);
    outputFile.close();

    return ERR_SUCCESS;
}

UINT8 UEFIReplace::replaceInFile(const QModelIndex & index, const QByteArray & guid, const UINT8 sectionType, const QByteArray & newData, const UINT8 mode, bool replaceOnce)
{
    if (!model || !index.isValid())
        return ERR_INVALID_PARAMETER;
    bool patched = false;
    if (model->subtype(index) == sectionType) {
        QModelIndex fileIndex = index;
        if (model->type(index) != Types::File)
            fileIndex = model->findParentOfType(index, Types::File);
        QByteArray fileGuid = model->header(fileIndex).left(sizeof(EFI_GUID));

        bool guidMatch = fileGuid == guid;
        if (!guidMatch && sectionType == EFI_SECTION_FREEFORM_SUBTYPE_GUID) {
            QByteArray subGuid = model->header(index).mid(sizeof(UINT32), sizeof(EFI_GUID));
            guidMatch = subGuid == guid;
        }
        if (guidMatch && model->action(index) != Actions::Replace) {
            UINT8 result = ffsEngine->replace(index, newData, mode);
            if (replaceOnce || (result != ERR_SUCCESS && result != ERR_NOTHING_TO_PATCH))
                return result;
            patched = result == ERR_SUCCESS;
        }
    }

    if (model->rowCount(index) > 0) {
        for (int i = 0; i < model->rowCount(index); i++) {
            UINT8 result = replaceInFile(index.child(i, 0), guid, sectionType, newData, mode, replaceOnce);
            if (result == ERR_SUCCESS) {
                patched = true;
                if (replaceOnce)
                    break;
            } else if (result != ERR_NOTHING_TO_PATCH) {
                return result;
            }
        }
    }

    return patched ? ERR_SUCCESS : ERR_NOTHING_TO_PATCH;
}
