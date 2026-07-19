/* uefiedit.cpp

Copyright (c) 2026, LongSoft. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#include "uefiedit.h"
#include <iostream>
#include <fstream>
#include <cstring>

UEFIEdit::UEFIEdit()
{
    model = new TreeModel();
    ffsParser = new FfsParser(model);
    ffsOps = new FfsOperations(model);
    ffsBuilder = new FfsBuilder(model);
    initDone = false;
}

UEFIEdit::~UEFIEdit()
{
    delete ffsBuilder;
    delete ffsOps;
    delete ffsParser;
    delete model;
}

USTATUS UEFIEdit::init(const UString & imagePath)
{
    if (false == readFileIntoBuffer(imagePath, originalBuffer))
        return U_FILE_OPEN;

    USTATUS result = ffsParser->parse(originalBuffer);
    if (result)
        return result;

    initDone = true;
    return U_SUCCESS;
}

USTATUS UEFIEdit::save(const UString & outputPath)
{
    if (!initDone)
        return U_INVALID_PARAMETER;

    UModelIndex root = model->index(0, 0);
    if (!root.isValid())
        return U_INVALID_PARAMETER;

    ffsBuilder->clearMessages();
    UByteArray image;
    USTATUS result = ffsBuilder->build(root, image);
    if (result) {
        std::cerr << "Build failed: " << errorCodeToUString(result).toLocal8Bit() << std::endl;
        for (auto &m : ffsBuilder->getMessages())
            std::cerr << "  BUILDER: " << m.first.toLocal8Bit() << std::endl;
        return result;
    }

    std::ofstream out(outputPath.toLocal8Bit(), std::ios::binary);
    if (!out) {
        std::cerr << "Cannot open " << outputPath.toLocal8Bit() << " for writing" << std::endl;
        return U_FILE_WRITE;
    }
    out.write(image.constData(), image.size());
    out.close();
    return U_SUCCESS;
}

// Parse a GUID string of the form "XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX"
// (case-insensitive, dashes optional) into an EFI_GUID.
static bool parseGuidString(const UString & s, EFI_GUID & guid)
{
    // Make a normalized copy: lowercase, no dashes, must be 32 hex chars
    std::string normalized;
    const char *p = s.toLocal8Bit();
    for (; *p; ++p) {
        char c = *p;
        if (c == '-' || c == ' ' || c == '{' || c == '}')
            continue;
        if (c >= 'A' && c <= 'F')
            c = (char)(c - 'A' + 'a');
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')))
            return false;
        normalized += c;
    }
    if (normalized.size() != 32)
        return false;

    auto hexByte = [](const char *h) -> UINT8 {
        auto v = [](char c) -> int {
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'a' && c <= 'f') return c - 'a' + 10;
            return 0;
        };
        return (UINT8)((v(h[0]) << 4) | v(h[1]));
    };
    auto hexWord = [&](const char *h) -> UINT16 {
        return (UINT16)((hexByte(h) << 8) | hexByte(h + 2));
    };
    auto hexDword = [&](const char *h) -> UINT32 {
        return (UINT32)(((UINT32)hexWord(h) << 16) | hexWord(h + 4));
    };

    const char *h = normalized.c_str();
    guid.Data1 = hexDword(h);       h += 8;
    guid.Data2 = hexWord(h);        h += 4;
    guid.Data3 = hexWord(h);        h += 4;
    for (int i = 0; i < 8; ++i) {
        guid.Data4[i] = hexByte(h);
        h += 2;
    }
    return true;
}

UModelIndex UEFIEdit::findItemByGuidRecursive(const UModelIndex & parent, const EFI_GUID & guid)
{
    if (!parent.isValid())
        return UModelIndex();

    // Files: GUID is the first field of the FFS file header
    if (model->type(parent) == Types::File && !model->hasEmptyHeader(parent)) {
        UByteArray hdr = model->header(parent);
        if ((UINT32)hdr.size() >= sizeof(EFI_GUID)) {
            const EFI_GUID *itemGuid = (const EFI_GUID *)hdr.constData();
            if (std::memcmp(itemGuid, &guid, sizeof(EFI_GUID)) == 0)
                return parent;
        }
    }

    // Volumes: the displayed name comes from the extended header's FvName GUID
    // (stored in VOLUME_PARSING_DATA.extendedHeaderGuid), not from FileSystemGuid
    // which is the FFS version GUID (e.g. 8C8CE578 for FFSv2).
    if (model->type(parent) == Types::Volume && !model->hasEmptyParsingData(parent)) {
        UByteArray pdata = model->parsingData(parent);
        if ((UINT32)pdata.size() >= sizeof(VOLUME_PARSING_DATA)) {
            const VOLUME_PARSING_DATA *vpd = (const VOLUME_PARSING_DATA *)pdata.constData();
            if (vpd->hasExtendedHeader && std::memcmp(&vpd->extendedHeaderGuid, &guid, sizeof(EFI_GUID)) == 0)
                return parent;
        }
    }

    // GUID-defined sections and freeform GUID sections: the GUID is in parsingData
    if (model->type(parent) == Types::Section && !model->hasEmptyParsingData(parent)) {
        UByteArray pdata = model->parsingData(parent);
        if ((UINT32)pdata.size() >= sizeof(EFI_GUID)) {
            const EFI_GUID *itemGuid = (const EFI_GUID *)pdata.constData();
            if (std::memcmp(itemGuid, &guid, sizeof(EFI_GUID)) == 0)
                return parent;
        }
    }

    for (int i = 0; i < model->rowCount(parent); ++i) {
        UModelIndex found = findItemByGuidRecursive(model->index(i, 0, parent), guid);
        if (found.isValid())
            return found;
    }
    return UModelIndex();
}

UModelIndex UEFIEdit::findItemByGuid(const UString & guidStr)
{
    EFI_GUID guid;
    if (!parseGuidString(guidStr, guid)) {
        std::cerr << "Invalid GUID: " << guidStr.toLocal8Bit() << std::endl;
        return UModelIndex();
    }
    return findItemByGuidRecursive(model->index(0, 0), guid);
}

UModelIndex UEFIEdit::findFileByGuid(const UString & guidStr)
{
    UModelIndex idx = findItemByGuid(guidStr);
    if (!idx.isValid()) {
        std::cerr << "Item with GUID " << guidStr.toLocal8Bit() << " not found" << std::endl;
    }
    return idx;
}

static void dumpTreeRecursive(TreeModel * model, const UModelIndex & parent, int depth, int row)
{
    if (!parent.isValid())
        return;
    for (int i = 0; i < depth; ++i) std::cerr << "  ";
    const char *typeName = "?";
    UINT8 t = model->type(parent);
    UINT8 st = model->subtype(parent);
    if (t == Types::Capsule) typeName = "Capsule";
    else if (t == Types::Image) typeName = "Image";
    else if (t == Types::Region) typeName = "Region";
    else if (t == Types::Padding) typeName = "Padding";
    else if (t == Types::Volume) typeName = "Volume";
    else if (t == Types::File) typeName = "File";
    else if (t == Types::Section) typeName = "Section";
    else if (t == Types::FreeSpace) typeName = "FreeSpace";
    std::cerr << "[" << row << "] type=" << (int)t << " (" << typeName << ") subtype=" << (int)st
              << " name='" << model->name(parent).toLocal8Bit() << "'"
              << " action=" << (int)model->action(parent)
              << " hdr=" << model->headerSize(parent)
              << " body=" << model->bodySize(parent)
              << " tail=" << model->tailSize(parent)
              << " children=" << model->rowCount(parent)
              << std::endl;
    for (int i = 0; i < model->rowCount(parent); ++i)
        dumpTreeRecursive(model, model->index(i, 0, parent), depth + 1, i);
}

void UEFIEdit::dumpTree()
{
    dumpTreeRecursive(model, model->index(0, 0), 0, 0);
}

// Read the data file and split into header+body depending on the target.
// For File insertion: parse EFI_FFS_FILE_HEADER (with large-file detection).
// For Section insertion: parse EFI_COMMON_SECTION_HEADER (with section2 detection).
static bool readAndSplit(const UString & path, bool isFile, const UModelIndex & parentVolume,
                         TreeModel * model, UINT8 & outType, UINT8 & outSubtype,
                         UByteArray & header, UByteArray & body)
{
    UByteArray data;
    if (!readFileIntoBuffer(path, data))
        return false;

    if (isFile) {
        if ((UINT32)data.size() < sizeof(EFI_FFS_FILE_HEADER))
            return false;
        const EFI_FFS_FILE_HEADER *fh = (const EFI_FFS_FILE_HEADER *)data.constData();
        UINT32 headerSize = sizeof(EFI_FFS_FILE_HEADER);
        if (fh->Attributes & FFS_ATTRIB_LARGE_FILE) {
            UINT8 ffsVersion = 2, revision = 2;
            if (parentVolume.isValid() && !model->hasEmptyParsingData(parentVolume)) {
                VOLUME_PARSING_DATA pdata = *(const VOLUME_PARSING_DATA *)model->parsingData(parentVolume).constData();
                ffsVersion = pdata.ffsVersion;
                revision = pdata.revision;
            }
            if (ffsVersion == 2 && revision == 2)
                headerSize = sizeof(EFI_FFS_FILE_HEADER2_LENOVO);
            else if (ffsVersion == 3)
                headerSize = sizeof(EFI_FFS_FILE_HEADER2);
        }
        outType = Types::File;
        outSubtype = fh->Type;
        header = UByteArray(data.constData(), headerSize);
        body = UByteArray(data.constData() + headerSize, data.size() - headerSize);
        return true;
    }
    else {
        if ((UINT32)data.size() < sizeof(EFI_COMMON_SECTION_HEADER))
            return false;
        const EFI_COMMON_SECTION_HEADER *sh = (const EFI_COMMON_SECTION_HEADER *)data.constData();
        UINT32 headerSize = sizeof(EFI_COMMON_SECTION_HEADER);
        UINT8 ffsVersion = 2;
        if (parentVolume.isValid() && !model->hasEmptyParsingData(parentVolume)) {
            VOLUME_PARSING_DATA pdata = *(const VOLUME_PARSING_DATA *)model->parsingData(parentVolume).constData();
            ffsVersion = pdata.ffsVersion;
        }
        if (ffsVersion == 3 && uint24ToUint32(sh->Size) == EFI_SECTION2_IS_USED) {
            if ((UINT32)data.size() < sizeof(EFI_COMMON_SECTION_HEADER2))
                return false;
            const EFI_COMMON_SECTION_HEADER2 *sh2 = (const EFI_COMMON_SECTION_HEADER2 *)data.constData();
            headerSize = sizeof(EFI_COMMON_SECTION_HEADER2);
            UINT32 fullSize = sh2->ExtendedSize;
            if (fullSize > (UINT32)data.size())
                fullSize = (UINT32)data.size();
            body = UByteArray(data.constData() + headerSize, fullSize - headerSize);
        }
        else {
            UINT32 fullSize = uint24ToUint32(sh->Size);
            if (fullSize > (UINT32)data.size())
                fullSize = (UINT32)data.size();
            body = UByteArray(data.constData() + headerSize, fullSize - headerSize);
        }
        outType = Types::Section;
        outSubtype = sh->Type;
        header = UByteArray(data.constData(), headerSize);
        return true;
    }
}

USTATUS UEFIEdit::insert(const UString & guidStr, const UINT8 mode, const UString & dataPath)
{
    if (!initDone)
        return U_INVALID_PARAMETER;

    UModelIndex index = findFileByGuid(guidStr);
    if (!index.isValid())
        return U_ITEM_NOT_FOUND;

    // Determine the parent for action marking and the type of the new item.
    // For PREPEND, the new item is added as a child of the selected item.
    // For BEFORE/AFTER, the new item is added as a sibling of the selected item.
    UModelIndex parentIndex;      // The logical parent (container) of the new item
    UModelIndex refItem;          // The reference item for addItem (see TreeModel::addItem)
    UINT8 createMode;
    if (mode == CREATE_MODE_PREPEND) {
        parentIndex = index;
        refItem = index;
        createMode = CREATE_MODE_PREPEND;
    }
    else if (mode == CREATE_MODE_BEFORE || mode == CREATE_MODE_AFTER) {
        parentIndex = index.parent();
        refItem = index;  // addItem uses the ref item's parent as the container for BEFORE/AFTER
        createMode = mode;
    }
    else {
        return U_INVALID_PARAMETER;
    }
    if (!parentIndex.isValid())
        return U_INVALID_PARAMETER;

    UINT8 parentType = model->type(parentIndex);
    bool insertFile = false;
    if (parentType == Types::Volume)
        insertFile = true;
    else if (parentType == Types::File)
        insertFile = false;
    else if (parentType == Types::Section) {
        UINT8 parentSubtype = model->subtype(parentIndex);
        if (parentSubtype != EFI_SECTION_COMPRESSION
            && parentSubtype != EFI_SECTION_GUID_DEFINED
            && parentSubtype != EFI_SECTION_DISPOSABLE) {
            std::cerr << "Can only insert into encapsulation sections" << std::endl;
            return U_INVALID_PARAMETER;
        }
        insertFile = false;
    }
    else {
        std::cerr << "Cannot insert into type " << (int)parentType << std::endl;
        return U_INVALID_PARAMETER;
    }

    UModelIndex parentVolume = (parentType == Types::Volume) ? parentIndex : model->findParentOfType(parentIndex, Types::Volume);
    UINT8 newType, newSubtype;
    UByteArray header, body;
    if (!readAndSplit(dataPath, insertFile, parentVolume, model, newType, newSubtype, header, body)) {
        std::cerr << "Failed to read/parse " << dataPath.toLocal8Bit() << std::endl;
        return U_INVALID_PARAMETER;
    }

    UString name;
    if (newType == Types::File) {
        const EFI_FFS_FILE_HEADER *fh = (const EFI_FFS_FILE_HEADER *)header.constData();
        name = guidToUString(fh->Name);
    }
    else {
        name = sectionTypeToUString(newSubtype) + UString(" section");
    }

    UModelIndex newIndex = model->addItem(model->offset(index), newType, newSubtype,
                                          name, UString(), UString(),
                                          header, body, UByteArray(),
                                          Movable, refItem, createMode);
    if (!newIndex.isValid()) {
        std::cerr << "Failed to add item to the tree" << std::endl;
        return U_INVALID_PARAMETER;
    }

    if (newType == Types::File) {
        const EFI_FFS_FILE_HEADER *fh = (const EFI_FFS_FILE_HEADER *)header.constData();
        FILE_PARSING_DATA pdata = {};
        pdata.emptyByte = (fh->State & EFI_FILE_ERASE_POLARITY) ? 0xFF : 0x00;
        pdata.guid = fh->Name;
        model->setParsingData(newIndex, UByteArray((const char *)&pdata, sizeof(pdata)));
    }

    model->setAction(newIndex, Actions::Insert);
    // Mark ancestors for rebuild so changes propagate to the root.
    for (UModelIndex p = parentIndex; p.isValid() && model->type(p) != Types::Root; p = p.parent()) {
        if (model->action(p) == Actions::NoAction)
            model->setAction(p, Actions::Rebuild);
    }
    UModelIndex root = model->index(0, 0);
    if (root.isValid() && model->action(root) == Actions::NoAction)
        model->setAction(root, Actions::Rebuild);

    std::cerr << "Inserted " << (insertFile ? "file" : "section") << " '" << name.toLocal8Bit()
              << "' (type=" << (int)newType << " subtype=" << (int)newSubtype << ")" << std::endl;
    return U_SUCCESS;
}

USTATUS UEFIEdit::remove(const UString & guidStr)
{
    if (!initDone)
        return U_INVALID_PARAMETER;

    UModelIndex index = findFileByGuid(guidStr);
    if (!index.isValid())
        return U_ITEM_NOT_FOUND;

    USTATUS result = ffsOps->remove(index);
    if (result)
        return result;
    // Mark all ancestors for rebuild so the removal propagates to the root.
    for (UModelIndex p = index.parent(); p.isValid() && model->type(p) != Types::Root; p = p.parent()) {
        if (model->action(p) == Actions::NoAction)
            model->setAction(p, Actions::Rebuild);
    }
    UModelIndex root = model->index(0, 0);
    if (root.isValid() && model->action(root) == Actions::NoAction)
        model->setAction(root, Actions::Rebuild);
    return U_SUCCESS;
}

USTATUS UEFIEdit::replace(const UString & guidStr, const UINT8 mode, const UString & dataPath)
{
    if (!initDone)
        return U_INVALID_PARAMETER;

    UModelIndex index = findFileByGuid(guidStr);
    if (!index.isValid())
        return U_ITEM_NOT_FOUND;

    UByteArray data;
    if (!readFileIntoBuffer(dataPath, data))
        return U_FILE_OPEN;

    USTATUS result = ffsOps->replace(index, data, mode);
    if (result)
        return result;
    // Mark all ancestors for rebuild so the replacement propagates to the root.
    for (UModelIndex p = index.parent(); p.isValid() && model->type(p) != Types::Root; p = p.parent()) {
        if (model->action(p) == Actions::NoAction)
            model->setAction(p, Actions::Rebuild);
    }
    UModelIndex root = model->index(0, 0);
    if (root.isValid() && model->action(root) == Actions::NoAction)
        model->setAction(root, Actions::Rebuild);
    return U_SUCCESS;
}

USTATUS UEFIEdit::rebuild(const UString & guidStr)
{
    if (!initDone)
        return U_INVALID_PARAMETER;

    UModelIndex index = findFileByGuid(guidStr);
    if (!index.isValid())
        return U_ITEM_NOT_FOUND;

    USTATUS result = ffsOps->rebuild(index);
    if (result)
        return result;
    UModelIndex root = model->index(0, 0);
    if (root.isValid() && model->action(root) == Actions::NoAction)
        model->setAction(root, Actions::Rebuild);
    return U_SUCCESS;
}