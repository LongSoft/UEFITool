/* ffsdumper.cpp

Copyright (c) 2015, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#include "ffsdumper.h"

FfsDumper::FfsDumper(TreeModel* treeModel)
    : model(treeModel), dumped(false)
{
}

FfsDumper::~FfsDumper()
{
}

USTATUS FfsDumper::dump(const UModelIndex & root, const UString & path, const bool dumpAll, const UString & guid)
{
    dumped = false;
    UINT8 result = recursiveDump(root, path, dumpAll, guid);
    if (result)
        return result;
    else if (!dumped)
        return U_ITEM_NOT_FOUND;
    return U_SUCCESS;
}

USTATUS FfsDumper::recursiveDump(const UModelIndex & index, const UString & path, const bool dumpAll, const UString & guid)
{
    if (!index.isValid())
        return U_INVALID_PARAMETER;

    QDir dir;
    if (guid.isEmpty() ||
        guidToUString(*(const EFI_GUID*)model->header(index).constData()) == guid ||
        guidToUString(*(const EFI_GUID*)model->header(model->findParentOfType(index, Types::File)).constData()) == guid) {

        if (dir.cd(path))
            return U_DIR_ALREADY_EXIST;

        if (!dir.mkpath(path))
            return U_DIR_CREATE;

        QFile file;
        if (dumpAll || model->rowCount(index) == 0)  { // Dump if leaf item or dumpAll is true
            if (!model->header(index).isEmpty()) {
                file.setFileName(path + UString("/header.bin"));
                if (!file.open(QFile::WriteOnly))
                    return U_FILE_OPEN;

                file.write(model->header(index));
                file.close();
            }

            if (!model->body(index).isEmpty()) {
                file.setFileName(path + UString("/body.bin"));
                if (!file.open(QFile::WriteOnly))
                    return U_FILE_OPEN;

                file.write(model->body(index));
                file.close();
            }
        }
        
        // Always dump info
        UString info = UString("Type: ") + itemTypeToUString(model->type(index)) + UString("\n")
            + UString("Subtype: ") + itemSubtypeToUString(model->type(index), model->subtype(index)) + UString("\n")
            + (model->text(index).isEmpty() ? UString("") : UString("Text: ") + model->text(index) + UString("\n"))
            + model->info(index) + UString("\n");
        file.setFileName(path + UString("/info.txt"));
        if (!file.open(QFile::Text | QFile::WriteOnly))
            return U_FILE_OPEN;

        file.write(info.toLatin1());
        file.close();
        dumped = true;
    }

    UINT8 result;
    for (int i = 0; i < model->rowCount(index); i++) {
        UModelIndex childIndex = index.child(i, 0);
        bool useText = FALSE;
        if (model->type(childIndex) != Types::Volume)
            useText = !model->text(childIndex).isEmpty();

        UString childPath = path + usprintf("/%u ", i) + (useText ? model->text(childIndex) : model->name(childIndex));
        result = recursiveDump(childIndex, childPath, dumpAll, guid);
        if (result)
            return result;
    }

    return U_SUCCESS;
}
