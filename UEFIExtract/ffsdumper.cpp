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
    fileList.clear();

    if (changeDirectory(path))
        return U_DIR_ALREADY_EXIST;

    currentPath = path;

    USTATUS result = recursiveDump(root, path, dumpMode, sectionType, guid);
    if (result) {
        return result;
    } else if (!dumped) {
        removeDirectory(path);
        return U_ITEM_NOT_FOUND;
    }

    return U_SUCCESS;
}

USTATUS FfsDumper::recursiveDump(const UModelIndex & index, const UString & path, const DumpMode dumpMode, const UINT8 sectionType, const UString & guid)
{
    if (!index.isValid())
        return U_INVALID_PARAMETER;

    if (guid.isEmpty() ||
        (model->subtype(index) == EFI_SECTION_FREEFORM_SUBTYPE_GUID &&
            guidToUString(readUnaligned((const EFI_GUID*)(model->header(index).constData() + sizeof(EFI_COMMON_SECTION_HEADER)))) == guid) ||
        guidToUString(readUnaligned((const EFI_GUID*)model->header(index).constData())) == guid ||
        guidToUString(readUnaligned((const EFI_GUID*)model->header(model->findParentOfType(index, Types::File)).constData())) == guid) {

        if (!changeDirectory(path) && !makeDirectory(path))
            return U_DIR_CREATE;

        if (currentPath != path) {
            counterHeader = counterBody = counterRaw = counterInfo = 0;
            currentPath = path;
        }

        if (fileList.count(index) == 0
            && (dumpMode == DUMP_ALL || model->rowCount(index) == 0)
            && (sectionType == IgnoreSectionType || model->subtype(index) == sectionType)) {

            if ((dumpMode == DUMP_ALL || dumpMode == DUMP_CURRENT || dumpMode == DUMP_HEADER)
                && !model->header(index).isEmpty()) {
                fileList.insert(index);

                UString filename;
                if (counterHeader == 0)
                    filename = usprintf("%s/header.bin", path.toLocal8Bit());
                else
                    filename = usprintf("%s/header_%d.bin", path.toLocal8Bit(), counterHeader);
                counterHeader++;

                std::ofstream file(filename.toLocal8Bit(), std::ofstream::binary);
                if (!file)
                    return U_FILE_OPEN;

                const UByteArray &data = model->header(index);
                file.write(data.constData(), data.size());

                dumped = true;
            }

            if ((dumpMode == DUMP_ALL || dumpMode == DUMP_CURRENT || dumpMode == DUMP_BODY)
                && !model->body(index).isEmpty()) {
                fileList.insert(index);
                UString filename;
                if (counterBody == 0)
                    filename = usprintf("%s/body.bin", path.toLocal8Bit());
                else
                    filename = usprintf("%s/body_%d.bin", path.toLocal8Bit(), counterBody);
                counterBody++;

                std::ofstream file(filename.toLocal8Bit(), std::ofstream::binary);
                if (!file)
                    return U_FILE_OPEN;

                const UByteArray &data = model->body(index);
                file.write(data.constData(), data.size());

                dumped = true;
            }

            if (dumpMode == DUMP_FILE) {
                UModelIndex fileIndex = index;
                if (model->type(fileIndex) != Types::File) {
                    fileIndex = model->findParentOfType(index, Types::File);
                    if (!fileIndex.isValid())
                        fileIndex = index;
                }

                // We may select parent file during ffs extraction.
                if (fileList.count(fileIndex) == 0) {
                    fileList.insert(fileIndex);

                    UString filename;
                    if (counterRaw == 0)
                        filename = usprintf("%s/file.ffs", path.toLocal8Bit());
                    else
                        filename = usprintf("%s/file_%d.ffs", path.toLocal8Bit(), counterRaw);
                    counterRaw++;

                    std::ofstream file(filename.toLocal8Bit(), std::ofstream::binary);
                    if (!file)
                        return U_FILE_OPEN;

                    const UByteArray &headerData = model->header(fileIndex);
                    const UByteArray &bodyData = model->body(fileIndex);
                    const UByteArray &tailData = model->tail(fileIndex);

                    file.write(headerData.constData(), headerData.size());
                    file.write(bodyData.constData(), bodyData.size());
                    file.write(tailData.constData(), tailData.size());

                    dumped = true;
                }
            }
        }

        // Always dump info unless explicitly prohibited
        if ((dumpMode == DUMP_ALL || dumpMode == DUMP_CURRENT || dumpMode == DUMP_INFO)
            && (sectionType == IgnoreSectionType || model->subtype(index) == sectionType)) {
            UString info = usprintf("Type: %s\nSubtype: %s\n%s%s\n",
                itemTypeToUString(model->type(index)).toLocal8Bit(),
                itemSubtypeToUString(model->type(index), model->subtype(index)).toLocal8Bit(),
                (model->text(index).isEmpty() ? UString("") :
                    usprintf("Text: %s\n", model->text(index).toLocal8Bit())).toLocal8Bit(),
                model->info(index).toLocal8Bit());

            UString filename;
            if (counterInfo == 0)
                filename = usprintf("%s/info.txt", path.toLocal8Bit());
            else
                filename = usprintf("%s/info_%d.txt", path.toLocal8Bit(), counterInfo);
            counterInfo++;

            std::ofstream file(filename.toLocal8Bit());
            if (!file)
                return U_FILE_OPEN;

            file << info.toLocal8Bit();

            dumped = true;
        }
    }

    USTATUS result;

    for (int i = 0; i < model->rowCount(index); i++) {
        UModelIndex childIndex = index.child(i, 0);
        bool useText = FALSE;
        if (model->type(childIndex) != Types::Volume)
            useText = !model->text(childIndex).isEmpty();

        UString childPath = path;
        if (dumpMode == DUMP_ALL || dumpMode == DUMP_CURRENT) {
            if (!changeDirectory(path) && !makeDirectory(path))
                return U_DIR_CREATE;

            childPath = usprintf("%s/%d %s", path.toLocal8Bit(), i,
                (useText ? model->text(childIndex) : model->name(childIndex)).toLocal8Bit());
        }
        result = recursiveDump(childIndex, childPath, dumpMode, sectionType, guid);
        if (result)
            return result;
    }

    return U_SUCCESS;
}
