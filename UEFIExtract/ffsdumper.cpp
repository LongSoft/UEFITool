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

#include <fstream>

USTATUS FfsDumper::dump(const UModelIndex & root, const UString & path, const DumpMode dumpMode, const UINT8 sectionType, const UString & guid)
{
    dumped = false;
    counterHeader = counterBody = counterRaw = counterInfo = 0;

    if (changeDirectory(path))
        return U_DIR_ALREADY_EXIST;

    UINT8 result = recursiveDump(root, path, dumpMode, sectionType, guid);
    if (result)
        return result;
    else if (!dumped)
        return U_ITEM_NOT_FOUND;
    return U_SUCCESS;
}

USTATUS FfsDumper::recursiveDump(const UModelIndex & index, const UString & path, const DumpMode dumpMode, const UINT8 sectionType, const UString & guid)
{
    if (!index.isValid())
        return U_INVALID_PARAMETER;

    if (guid.isEmpty() ||
        (model->subtype(index) == EFI_SECTION_FREEFORM_SUBTYPE_GUID &&
        guidToUString(*(const EFI_GUID*)(model->header(index).constData() + sizeof(EFI_COMMON_SECTION_HEADER))) == guid) ||
        guidToUString(*(const EFI_GUID*)model->header(index).constData()) == guid ||
        guidToUString(*(const EFI_GUID*)model->header(model->findParentOfType(index, Types::File)).constData()) == guid) {

        if (!changeDirectory(path) && !makeDirectory(path))
            return U_DIR_CREATE;

        counterHeader = counterBody = counterRaw = counterInfo = 0;

        std::ofstream file;
        if (dumpMode == DUMP_ALL || model->rowCount(index) == 0)  { // Dump if leaf item or dumpAll is true
            if (dumpMode == DUMP_ALL || dumpMode == DUMP_CURRENT || dumpMode == DUMP_HEADER) {
                if (!model->header(index).isEmpty() && (sectionType == IgnoreSectionType || model->subtype(index) == sectionType)) {
                    UString filename;
                    if (counterHeader == 0)
                        filename = usprintf("%s/header.bin", (const char *)path.toLocal8Bit());
                    else
                        filename = usprintf("%s/header_%d.bin", (const char *)path.toLocal8Bit(), counterHeader);
                    counterHeader++;
                    file.open((const char *)filename.toLocal8Bit(), std::ofstream::binary);
                    const UByteArray &data = model->header(index);
                    file.write(data.constData(), data.size());
                    file.close();
                }
            }

            if (dumpMode == DUMP_ALL || dumpMode == DUMP_CURRENT || dumpMode == DUMP_BODY) {
                if (!model->body(index).isEmpty() && (sectionType == IgnoreSectionType || model->subtype(index) == sectionType)) {
                    UString filename;
                    if (counterBody == 0)
                        filename = usprintf("%s/body.bin", (const char *)path.toLocal8Bit());
                    else
                        filename = usprintf("%s/body_%d.bin", (const char *)path.toLocal8Bit(), counterBody);
                    counterBody++;
                    file.open((const char *)filename.toLocal8Bit(), std::ofstream::binary);
                    const UByteArray &data = model->body(index);
                    file.write(data.constData(), data.size());
                    file.close();
                }
            }

            if (dumpMode == DUMP_FILE && (sectionType == IgnoreSectionType || model->subtype(index) == sectionType)) {
                UModelIndex fileIndex = model->findParentOfType(index, Types::File);
                if (!fileIndex.isValid())
                    fileIndex = index;
                UString filename;
                if (counterRaw == 0)
                    filename = usprintf("%s/file.ffs", (const char *)path.toLocal8Bit());
                else
                    filename = usprintf("%s/file_%d.bin", (const char *)path.toLocal8Bit(), counterRaw);
                counterRaw++;
                file.open((const char *)filename.toLocal8Bit(), std::ofstream::binary);
                const UByteArray &headerData = model->header(index);
                const UByteArray &bodyData = model->body(index);
                const UByteArray &tailData = model->tail(index);
                file.write(headerData.constData(), headerData.size());
                file.write(bodyData.constData(), bodyData.size());
                file.write(tailData.constData(), tailData.size());
                file.close();
            }
        }

        // Always dump info unless explicitly prohibited
        if ((dumpMode == DUMP_ALL || dumpMode == DUMP_CURRENT || dumpMode == DUMP_INFO)
            && (sectionType == IgnoreSectionType || model->subtype(index) == sectionType)) {
            UString info = usprintf("Type: %s\nSubtype: %s\n%s%s\n",
                (const char *)itemTypeToUString(model->type(index)).toLocal8Bit(),
                (const char *)itemSubtypeToUString(model->type(index), model->subtype(index)).toLocal8Bit(),
                (const char *)(model->text(index).isEmpty() ? UString("") :
                    usprintf("Text: %s\n", (const char *)model->text(index).toLocal8Bit())).toLocal8Bit(),
                (const char *)model->info(index).toLocal8Bit());
            UString filename;
            if (counterInfo == 0)
                filename = usprintf("%s/info.txt", (const char *)path.toLocal8Bit());
            else
                filename = usprintf("%s/info_%d.txt", (const char *)path.toLocal8Bit(), counterInfo);
            counterInfo++;
            file.open((const char *)filename.toLocal8Bit());
            file << (const char *)info.toLocal8Bit();
            file.close();
        }

        dumped = true;
    }

    UINT8 result;
    for (int i = 0; i < model->rowCount(index); i++) {
        UModelIndex childIndex = index.child(i, 0);
        bool useText = FALSE;
        if (model->type(childIndex) != Types::Volume)
            useText = !model->text(childIndex).isEmpty();

        UString childPath = path;
        if (dumpMode == DUMP_ALL || dumpMode == DUMP_CURRENT)
            childPath = usprintf("%s/%d %s", (const char *)path.toLocal8Bit(), i,
                (const char *)(useText ? model->text(childIndex) : model->name(childIndex)).toLocal8Bit());
        result = recursiveDump(childIndex, childPath, dumpMode, sectionType, guid);
        if (result)
            return result;
    }

    return U_SUCCESS;
}
