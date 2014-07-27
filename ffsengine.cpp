/* ffsengine.cpp

Copyright (c) 2014, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHWARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*/

#include <math.h>

#include "ffsengine.h"
#include "types.h"
#include "treemodel.h"
#include "descriptor.h"
#include "ffs.h"
#include "gbe.h"
#include "me.h"
#include "Tiano/EfiTianoCompress.h"
#include "Tiano/EfiTianoDecompress.h"
#include "LZMA/LzmaCompress.h"
#include "LZMA/LzmaDecompress.h"

#ifdef _CONSOLE
#include <iostream>
#endif

QString errorMessage(UINT8 errorCode)
{
    QString msg;
    switch (errorCode)
    {
    case ERR_SUCCESS:
        msg = QObject::tr("Success");
        break;
    case ERR_NOT_IMPLEMENTED:
        msg = QObject::tr("Not implemented");
        break;
    case ERR_INVALID_PARAMETER:
        msg = QObject::tr("Function called with invalid parameter");
        break;
    case ERR_BUFFER_TOO_SMALL:
        msg = QObject::tr("Buffer too small");
        break;
    case ERR_OUT_OF_RESOURCES:
        msg = QObject::tr("Out of resources");
        break;
    case ERR_OUT_OF_MEMORY:
        msg = QObject::tr("Out of memory");
        break;
    case ERR_FILE_OPEN:
        msg = QObject::tr("File can't be opened");
        break;
    case ERR_FILE_READ:
        msg = QObject::tr("File can't be read");
        break;
    case ERR_FILE_WRITE:
        msg = QObject::tr("File can't be written");
        break;
    case ERR_ITEM_NOT_FOUND:
        msg = QObject::tr("Item not found");
        break;
    case ERR_UNKNOWN_ITEM_TYPE:
        msg = QObject::tr("Unknown item type");
        break;
    case ERR_INVALID_FLASH_DESCRIPTOR:
        msg = QObject::tr("Invalid flash descriptor");
        break;
    case ERR_INVALID_REGION:
        msg = QObject::tr("Invalid region");
        break;
    case ERR_EMPTY_REGION:
        msg = QObject::tr("Empty region");
        break;
    case ERR_BIOS_REGION_NOT_FOUND:
        msg = QObject::tr("BIOS region not found");
        break;
    case ERR_VOLUMES_NOT_FOUND:
        msg = QObject::tr("UEFI volumes not found");
        break;
    case ERR_INVALID_VOLUME:
        msg = QObject::tr("Invalid UEFI volume");
        break;
    case ERR_VOLUME_REVISION_NOT_SUPPORTED:
        msg = QObject::tr("Volume revision not supported");
        break;
    case ERR_VOLUME_GROW_FAILED:
        msg = QObject::tr("Volume grow failed");
        break;
    case ERR_UNKNOWN_FFS:
        msg = QObject::tr("Unknown file system");
        break;
    case ERR_INVALID_FILE:
        msg = QObject::tr("Invalid file");
        break;
    case ERR_INVALID_SECTION:
        msg = QObject::tr("Invalid section");
        break;
    case ERR_UNKNOWN_SECTION:
        msg = QObject::tr("Unknown section");
        break;
    case ERR_STANDARD_COMPRESSION_FAILED:
        msg = QObject::tr("Standard compression failed");
        break;
    case ERR_CUSTOMIZED_COMPRESSION_FAILED:
        msg = QObject::tr("Customized compression failed");
        break;
    case ERR_STANDARD_DECOMPRESSION_FAILED:
        msg = QObject::tr("Standard decompression failed");
        break;
    case ERR_CUSTOMIZED_DECOMPRESSION_FAILED:
        msg = QObject::tr("Customized compression failed");
        break;
    case ERR_UNKNOWN_COMPRESSION_ALGORITHM:
        msg = QObject::tr("Unknown compression method");
        break;
    case ERR_UNKNOWN_EXTRACT_MODE:
        msg = QObject::tr("Unknown extract mode");
        break;
    case ERR_UNKNOWN_INSERT_MODE:
        msg = QObject::tr("Unknown insert mode");
        break;
    case ERR_UNKNOWN_IMAGE_TYPE:
        msg = QObject::tr("Unknown executable image type");
        break;
    case ERR_UNKNOWN_PE_OPTIONAL_HEADER_TYPE:
        msg = QObject::tr("Unknown PE optional header type");
        break;
    case ERR_UNKNOWN_RELOCATION_TYPE:
        msg = QObject::tr("Unknown relocation type");
        break;
    case ERR_GENERIC_CALL_NOT_SUPPORTED:
        msg = QObject::tr("Generic call of this function not supported");
        break;
    case ERR_VOLUME_BASE_NOT_FOUND:
        msg = QObject::tr("Volume base address not found");
        break;
    case ERR_PEI_CORE_ENTRY_POINT_NOT_FOUND:
        msg = QObject::tr("PEI core entry point not found");
        break;
    case ERR_COMPLEX_BLOCK_MAP:
        msg = QObject::tr("Block map structure too complex for correct analysis");
        break;
    case ERR_DIR_ALREADY_EXIST:
        msg = QObject::tr("Directory already exists");
        break;
    case ERR_DIR_CREATE:
        msg = QObject::tr("Directory can't be created");
        break;
    case ERR_UNKNOWN_PATCH_TYPE:
        msg = QObject::tr("Unknown patch type");
        break;
    case ERR_PATCH_OFFSET_OUT_OF_BOUNDS:
        msg = QObject::tr("Patch offset out of bounds");
        break;
    case ERR_INVALID_SYMBOL:
        msg = QObject::tr("Invalid symbol");
        break;
    case ERR_NOTHING_TO_PATCH:
        msg = QObject::tr("Nothing to patch");
        break;
    default:
        msg = QObject::tr("Unknown error %1").arg(errorCode);
        break;
    }

    return msg;
}

FfsEngine::FfsEngine(QObject *parent)
    : QObject(parent)
{
    model = new TreeModel();
    oldPeiCoreEntryPoint = 0;
    newPeiCoreEntryPoint = 0;
}

FfsEngine::~FfsEngine(void)
{
    delete model;
}

TreeModel* FfsEngine::treeModel() const
{
    return model;
}

void FfsEngine::msg(const QString & message, const QModelIndex & index)
{
#ifndef _CONSOLE
    messageItems.enqueue(MessageListItem(message, NULL, 0, index));
#else
    std::cout << message.toLatin1().constData() << std::endl;
#endif
}

#ifndef _CONSOLE
QQueue<MessageListItem> FfsEngine::messages() const
{
    return messageItems;
}

void FfsEngine::clearMessages()
{
    messageItems.clear();
}
#endif

bool FfsEngine::hasIntersection(const UINT32 begin1, const UINT32 end1, const UINT32 begin2, const UINT32 end2)
{
    if (begin1 < begin2 && begin2 < end1)
        return true;
    if (begin1 < end2 && end2 < end1)
        return true;
    if (begin2 < begin1 && begin1 < end2)
        return true;
    if (begin2 < end1 && end1 < end2)
        return true;
    return false;
}

// Firmware image parsing
UINT8 FfsEngine::parseImageFile(const QByteArray & buffer)
{
    oldPeiCoreEntryPoint = 0;
    newPeiCoreEntryPoint = 0;
    UINT32 capsuleHeaderSize = 0;
    FLASH_DESCRIPTOR_HEADER* descriptorHeader = NULL;
    QModelIndex index;
    QByteArray flashImage;

    // Check buffer size to be more then or equal to size of EFI_CAPSULE_HEADER
    if ((UINT32)buffer.size() <= sizeof(EFI_CAPSULE_HEADER))
    {
        msg(tr("parseImageFile: Image file is smaller then minimum size of %1 bytes").arg(sizeof(EFI_CAPSULE_HEADER)));
        return ERR_INVALID_PARAMETER;
    }

    // Check buffer for being normal EFI capsule header
    if (buffer.startsWith(EFI_CAPSULE_GUID)) {
        // Get info
        EFI_CAPSULE_HEADER* capsuleHeader = (EFI_CAPSULE_HEADER*)buffer.constData();
        capsuleHeaderSize = capsuleHeader->HeaderSize;
        QByteArray header = buffer.left(capsuleHeaderSize);
        QByteArray body = buffer.right(buffer.size() - capsuleHeaderSize);
        QString name = tr("UEFI capsule");
        QString info = tr("Header size: %1\nFlags: %2\nImage size: %3")
            .arg(capsuleHeader->HeaderSize, 8, 16, QChar('0'))
            .arg(capsuleHeader->Flags, 8, 16, QChar('0'))
            .arg(capsuleHeader->CapsuleImageSize, 8, 16, QChar('0'));
        // Add tree item
        index = model->addItem(Types::Capsule, Subtypes::UefiCapsule, COMPRESSION_ALGORITHM_NONE, name, "", info, header, body);
    }

    // Check buffer for being extended Aptio capsule header
    else if (buffer.startsWith(APTIO_CAPSULE_GUID)) {
        // Get info
        APTIO_CAPSULE_HEADER* aptioCapsuleHeader = (APTIO_CAPSULE_HEADER*)buffer.constData();
        capsuleHeaderSize = aptioCapsuleHeader->RomImageOffset;
        QByteArray header = buffer.left(capsuleHeaderSize);
        QByteArray body = buffer.right(buffer.size() - capsuleHeaderSize);
        QString name = tr("AMI Aptio capsule");
        QString info = tr("Header size: %1\nFlags: %2\nImage size: %3")
            .arg(aptioCapsuleHeader->RomImageOffset, 4, 16, QChar('0'))
            .arg(aptioCapsuleHeader->CapsuleHeader.Flags, 8, 16, QChar('0'))
            .arg(aptioCapsuleHeader->CapsuleHeader.CapsuleImageSize - aptioCapsuleHeader->RomImageOffset, 8, 16, QChar('0'));
        //!TODO: more info about Aptio capsule
        // Add tree item
        index = model->addItem(Types::Capsule, Subtypes::AptioCapsule, COMPRESSION_ALGORITHM_NONE, name, "", info, header, body);
    }

    // Skip capsule header to have flash chip image
    flashImage = buffer.right(buffer.size() - capsuleHeaderSize);

    // Check for Intel flash descriptor presence
    descriptorHeader = (FLASH_DESCRIPTOR_HEADER*)flashImage.constData();

    // Check descriptor signature
    UINT8 result;
    if (descriptorHeader->Signature == FLASH_DESCRIPTOR_SIGNATURE) {
        // Parse as Intel image
        QModelIndex imageIndex;
        result = parseIntelImage(flashImage, imageIndex, index);
        if (result != ERR_INVALID_FLASH_DESCRIPTOR)
            return result;
    }

    // Get info
    QString name = tr("BIOS image");
    QString info = tr("Size: %1")
        .arg(flashImage.size(), 8, 16, QChar('0'));

    // Add tree item
    index = model->addItem(Types::Image, Subtypes::BiosImage, COMPRESSION_ALGORITHM_NONE, name, "", info, QByteArray(), flashImage, QByteArray(), index);
    return parseBios(flashImage, index);
}

UINT8 FfsEngine::parseIntelImage(const QByteArray & intelImage, QModelIndex & index, const QModelIndex & parent)
{
    FLASH_DESCRIPTOR_MAP*               descriptorMap;
    FLASH_DESCRIPTOR_REGION_SECTION*    regionSection;
    FLASH_DESCRIPTOR_MASTER_SECTION*    masterSection;

    // Store the beginning of descriptor as descriptor base address
    UINT8* descriptor = (UINT8*)intelImage.constData();
    UINT32 descriptorBegin = 0;
    UINT32 descriptorEnd = FLASH_DESCRIPTOR_SIZE;

    // Check for buffer size to be greater or equal to descriptor region size
    if (intelImage.size() < FLASH_DESCRIPTOR_SIZE) {
        msg(tr("parseIntelImage: Input file is smaller then minimum descriptor size of %1 bytes").arg(FLASH_DESCRIPTOR_SIZE));
        return ERR_INVALID_FLASH_DESCRIPTOR;
    }

    // Parse descriptor map
    descriptorMap = (FLASH_DESCRIPTOR_MAP*)(descriptor + sizeof(FLASH_DESCRIPTOR_HEADER));
    regionSection = (FLASH_DESCRIPTOR_REGION_SECTION*)calculateAddress8(descriptor, descriptorMap->RegionBase);
    masterSection = (FLASH_DESCRIPTOR_MASTER_SECTION*)calculateAddress8(descriptor, descriptorMap->MasterBase);

    // GbE region
    QByteArray gbe;
    UINT32 gbeBegin = 0;
    UINT32 gbeEnd = 0;
    if (regionSection->GbeLimit) {
        gbeBegin = calculateRegionOffset(regionSection->GbeBase);
        gbeEnd = calculateRegionSize(regionSection->GbeBase, regionSection->GbeLimit);
        gbe = intelImage.mid(gbeBegin, gbeEnd);
        gbeEnd += gbeBegin;
    }
    // ME region
    QByteArray me;
    UINT32 meBegin = 0;
    UINT32 meEnd = 0;
    if (regionSection->MeLimit) {
        meBegin = calculateRegionOffset(regionSection->MeBase);
        meEnd = calculateRegionSize(regionSection->MeBase, regionSection->MeLimit);
        me = intelImage.mid(meBegin, meEnd);
        meEnd += meBegin;
    }
    // PDR region
    QByteArray pdr;
    UINT32 pdrBegin = 0;
    UINT32 pdrEnd = 0;
    if (regionSection->PdrLimit) {
        pdrBegin = calculateRegionOffset(regionSection->PdrBase);
        pdrEnd = calculateRegionSize(regionSection->PdrBase, regionSection->PdrLimit);
        pdr = intelImage.mid(pdrBegin, pdrEnd);
        pdrEnd += pdrBegin;
    }
    // BIOS region
    QByteArray bios;
    UINT32 biosBegin = 0;
    UINT32 biosEnd = 0;
    if (regionSection->BiosLimit) {
        biosBegin = calculateRegionOffset(regionSection->BiosBase);
        biosEnd = calculateRegionSize(regionSection->BiosBase, regionSection->BiosLimit);

        // Check for Gigabyte specific descriptor map
        if (biosEnd - biosBegin == intelImage.size()) {
            if (!meEnd) {
                msg(tr("parseIntelImage: can determine BIOS region start from Gigabyte-specific descriptor"));
                return ERR_INVALID_FLASH_DESCRIPTOR;
            }
            biosBegin = meEnd;
        }

        bios = intelImage.mid(biosBegin, biosEnd);
        biosEnd += biosBegin;
    }
    else {
        msg(tr("parseIntelImage: descriptor parsing failed, BIOS region not found in descriptor"));
        return ERR_INVALID_FLASH_DESCRIPTOR;
    }

    // Check for intersections between regions
    if (hasIntersection(descriptorBegin, descriptorEnd, gbeBegin, gbeEnd)) {
        msg(tr("parseIntelImage: descriptor parsing failed, descriptor region has intersection with GbE region"));
        return ERR_INVALID_FLASH_DESCRIPTOR;
    }
    if (hasIntersection(descriptorBegin, descriptorEnd, meBegin, meEnd)) {
        msg(tr("parseIntelImage: descriptor parsing failed, descriptor region has intersection with ME region"));
        return ERR_INVALID_FLASH_DESCRIPTOR;
    }
    if (hasIntersection(descriptorBegin, descriptorEnd, biosBegin, biosEnd)) {
        msg(tr("parseIntelImage: descriptor parsing failed, descriptor region has intersection with BIOS region"));
        return ERR_INVALID_FLASH_DESCRIPTOR;
    }
    if (hasIntersection(descriptorBegin, descriptorEnd, pdrBegin, pdrEnd)) {
        msg(tr("parseIntelImage: descriptor parsing failed, descriptor region has intersection with PDR region"));
        return ERR_INVALID_FLASH_DESCRIPTOR;
    }
    if (hasIntersection(gbeBegin, gbeEnd, meBegin, meEnd)) {
        msg(tr("parseIntelImage: descriptor parsing failed, GbE region has intersection with ME region"));
        return ERR_INVALID_FLASH_DESCRIPTOR;
    }
    if (hasIntersection(gbeBegin, gbeEnd, biosBegin, biosEnd)) {
        msg(tr("parseIntelImage: descriptor parsing failed, GbE region has intersection with BIOS region"));
        return ERR_INVALID_FLASH_DESCRIPTOR;
    }
    if (hasIntersection(gbeBegin, gbeEnd, pdrBegin, pdrEnd)) {
        msg(tr("parseIntelImage: descriptor parsing failed, GbE region has intersection with PDR region"));
        return ERR_INVALID_FLASH_DESCRIPTOR;
    }
    if (hasIntersection(meBegin, meEnd, biosBegin, biosEnd)) {
        msg(tr("parseIntelImage: descriptor parsing failed, ME region has intersection with BIOS region"));
        return ERR_INVALID_FLASH_DESCRIPTOR;
    }
    if (hasIntersection(meBegin, meEnd, pdrBegin, pdrEnd)) {
        msg(tr("parseIntelImage: descriptor parsing failed, ME region has intersection with PDR region"));
        return ERR_INVALID_FLASH_DESCRIPTOR;
    }
    if (hasIntersection(biosBegin, biosEnd, pdrBegin, pdrEnd)) {
        msg(tr("parseIntelImage: descriptor parsing failed, BIOS region has intersection with PDR region"));
        return ERR_INVALID_FLASH_DESCRIPTOR;
    }

    // Region map is consistent
    QByteArray body;
    QString    name;
    QString    info;

    // Intel image
    name = tr("Intel image");
    info = tr("Size: %1\nFlash chips: %2\nRegions: %3\nMasters: %4\nPCH straps: %5\nPROC straps: %6\nICC table entries: %7")
        .arg(intelImage.size(), 8, 16, QChar('0'))
        .arg(descriptorMap->NumberOfFlashChips + 1) //
        .arg(descriptorMap->NumberOfRegions + 1)    // Zero-based numbers in storage
        .arg(descriptorMap->NumberOfMasters + 1)    //
        .arg(descriptorMap->NumberOfPchStraps)
        .arg(descriptorMap->NumberOfProcStraps)
        .arg(descriptorMap->NumberOfIccTableEntries);

    // Add Intel image tree item
    index = model->addItem(Types::Image, Subtypes::IntelImage, COMPRESSION_ALGORITHM_NONE, name, "", info, QByteArray(), intelImage, QByteArray(), parent);

    // Descriptor
    // Get descriptor info
    body = intelImage.left(FLASH_DESCRIPTOR_SIZE);
    name = tr("Descriptor region");
    info = tr("Size: %1").arg(FLASH_DESCRIPTOR_SIZE, 4, 16, QChar('0'));

    // Check regions presence once again
    QVector<UINT32> offsets;
    if (regionSection->GbeLimit) {
        offsets.append(gbeBegin);
        info += tr("\nGbE region offset:  %1").arg(gbeBegin, 8, 16, QChar('0'));
    }
    if (regionSection->MeLimit) {
        offsets.append(meBegin);
        info += tr("\nME region offset:   %1").arg(meBegin, 8, 16, QChar('0'));
    }
    if (regionSection->BiosLimit) {
        offsets.append(biosBegin);
        info += tr("\nBIOS region offset: %1").arg(biosBegin, 8, 16, QChar('0'));
    }
    if (regionSection->PdrLimit) {
        offsets.append(pdrBegin);
        info += tr("\nPDR region offset:  %1").arg(pdrBegin, 8, 16, QChar('0'));
    }

    // Region access settings
    info += tr("\nRegion access settings:");
    info += tr("\nBIOS:%1%2  ME:%3%4  GbE:%5%6")
        .arg(masterSection->BiosRead, 2, 16, QChar('0'))
        .arg(masterSection->BiosWrite, 2, 16, QChar('0'))
        .arg(masterSection->MeRead, 2, 16, QChar('0'))
        .arg(masterSection->MeWrite, 2, 16, QChar('0'))
        .arg(masterSection->GbeRead, 2, 16, QChar('0'))
        .arg(masterSection->GbeWrite, 2, 16, QChar('0'));

    // BIOS access table
    info += tr("\nBIOS access table:");
    info += tr("\n      Read  Write");
    info += tr("\nDesc  %1  %2")
        .arg(masterSection->BiosRead  & FLASH_DESCRIPTOR_REGION_ACCESS_DESC ? "Yes " : "No  ")
        .arg(masterSection->BiosWrite & FLASH_DESCRIPTOR_REGION_ACCESS_DESC ? "Yes " : "No  ");
    info += tr("\nBIOS  Yes   Yes");
    info += tr("\nME    %1  %2")
        .arg(masterSection->BiosRead  & FLASH_DESCRIPTOR_REGION_ACCESS_ME ? "Yes " : "No  ")
        .arg(masterSection->BiosWrite & FLASH_DESCRIPTOR_REGION_ACCESS_ME ? "Yes " : "No  ");
    info += tr("\nGbE   %1  %2")
        .arg(masterSection->BiosRead  & FLASH_DESCRIPTOR_REGION_ACCESS_GBE ? "Yes " : "No  ")
        .arg(masterSection->BiosWrite & FLASH_DESCRIPTOR_REGION_ACCESS_GBE ? "Yes " : "No  ");
    info += tr("\nPDR   %1  %2")
        .arg(masterSection->BiosRead  & FLASH_DESCRIPTOR_REGION_ACCESS_PDR ? "Yes " : "No  ")
        .arg(masterSection->BiosWrite & FLASH_DESCRIPTOR_REGION_ACCESS_PDR ? "Yes " : "No  ");

    // VSCC table

    // Add descriptor tree item
    model->addItem(Types::Region, Subtypes::DescriptorRegion, COMPRESSION_ALGORITHM_NONE, name, "", info, QByteArray(), body, QByteArray(), index);

    // Sort regions in ascending order
    qSort(offsets);

    // Parse regions
    UINT8 result = 0;
    for (int i = 0; i < offsets.count(); i++) {
        // Parse GbE region
        if (offsets.at(i) == gbeBegin) {
            QModelIndex gbeIndex;
            result = parseGbeRegion(gbe, gbeIndex, index);
        }
        // Parse ME region
        else if (offsets.at(i) == meBegin) {
            QModelIndex meIndex;
            result = parseMeRegion(me, meIndex, index);
        }
        // Parse BIOS region
        else if (offsets.at(i) == biosBegin) {
            QModelIndex biosIndex;
            result = parseBiosRegion(bios, biosIndex, index);
        }
        // Parse PDR region
        else if (offsets.at(i) == pdrBegin) {
            QModelIndex pdrIndex;
            result = parsePdrRegion(pdr, pdrIndex, index);
        }
        if (result)
            return result;
    }

    return ERR_SUCCESS;
}

UINT8 FfsEngine::parseGbeRegion(const QByteArray & gbe, QModelIndex & index, const QModelIndex & parent, const UINT8 mode)
{
    if (gbe.isEmpty())
        return ERR_EMPTY_REGION;

    // Get info
    QString      name = tr("GbE region");
    GBE_MAC*     mac = (GBE_MAC*)gbe.constData();
    GBE_VERSION* version = (GBE_VERSION*)(gbe.constData() + GBE_VERSION_OFFSET);
    QString      info = tr("Size: %1\nMAC: %2:%3:%4:%5:%6:%7\nVersion: %8.%9")
        .arg(gbe.size(), 8, 16, QChar('0'))
        .arg(mac->vendor[0], 2, 16, QChar('0'))
        .arg(mac->vendor[1], 2, 16, QChar('0'))
        .arg(mac->vendor[2], 2, 16, QChar('0'))
        .arg(mac->device[0], 2, 16, QChar('0'))
        .arg(mac->device[1], 2, 16, QChar('0'))
        .arg(mac->device[2], 2, 16, QChar('0'))
        .arg(version->major)
        .arg(version->minor);

    // Add tree item
    index = model->addItem(Types::Region, Subtypes::GbeRegion, COMPRESSION_ALGORITHM_NONE, name, "", info, QByteArray(), gbe, QByteArray(), parent, mode);

    return ERR_SUCCESS;
}

UINT8 FfsEngine::parseMeRegion(const QByteArray & me, QModelIndex & index, const QModelIndex & parent, const UINT8 mode)
{
    if (me.isEmpty())
        return ERR_EMPTY_REGION;

    // Get info
    QString name = tr("ME region");
    QString info = tr("Size: %1").
        arg(me.size(), 8, 16, QChar('0'));

    // Search for new signature
    INT32 versionOffset = me.indexOf(ME_VERSION_SIGNATURE2);
    bool versionFound = true;
    if (versionOffset < 0){ // New signature not found
        // Search for old signature
        versionOffset = me.indexOf(ME_VERSION_SIGNATURE);
        if (versionOffset < 0){
            info += tr("\nVersion: unknown");
            versionFound = false;
        }
    }

    // Add version information
    if (versionFound) {
        ME_VERSION* version = (ME_VERSION*)(me.constData() + versionOffset);
        info += tr("\nVersion: %1.%2.%3.%4")
            .arg(version->major)
            .arg(version->minor)
            .arg(version->bugfix)
            .arg(version->build);
    }

    // Add tree item
    index = model->addItem(Types::Region, Subtypes::MeRegion, COMPRESSION_ALGORITHM_NONE, name, "", info, QByteArray(), me, QByteArray(), parent, mode);

    if (!versionFound)
        msg(tr("parseRegion: ME region version is unknown, it can be damaged"), index);

    return ERR_SUCCESS;
}

UINT8 FfsEngine::parsePdrRegion(const QByteArray & pdr, QModelIndex & index, const QModelIndex & parent, const UINT8 mode)
{
    if (pdr.isEmpty())
        return ERR_EMPTY_REGION;

    // Get info
    QString name = tr("PDR region");
    QString info = tr("Size: %1").
        arg(pdr.size(), 8, 16, QChar('0'));

    // Add tree item
    index = model->addItem(Types::Region, Subtypes::PdrRegion, COMPRESSION_ALGORITHM_NONE, name, "", info, QByteArray(), pdr, QByteArray(), parent, mode);

    // Parse PDR region as BIOS space
    UINT8 result = parseBios(pdr, index);
    if (result && result != ERR_VOLUMES_NOT_FOUND)
        return result;

    return ERR_SUCCESS;
}

UINT8 FfsEngine::parseBiosRegion(const QByteArray & bios, QModelIndex & index, const QModelIndex & parent, const UINT8 mode)
{
    if (bios.isEmpty())
        return ERR_EMPTY_REGION;

    // Get info
    QString name = tr("BIOS region");
    QString info = tr("Size: %1").
        arg(bios.size(), 8, 16, QChar('0'));

    // Add tree item
    index = model->addItem(Types::Region, Subtypes::BiosRegion, COMPRESSION_ALGORITHM_NONE, name, "", info, QByteArray(), bios, QByteArray(), parent, mode);

    return parseBios(bios, index);
}

UINT8 FfsEngine::parseBios(const QByteArray & bios, const QModelIndex & parent)
{
    // Search for first volume
    UINT32 prevVolumeOffset;
    UINT8 result;

    result = findNextVolume(bios, 0, prevVolumeOffset);
    if (result)
        return result;

    // First volume is not at the beginning of BIOS space
    QString name;
    QString info;
    if (prevVolumeOffset > 0) {
        // Get info
        QByteArray padding = bios.left(prevVolumeOffset);
        name = tr("Padding");
        info = tr("Size: %1")
            .arg(padding.size(), 8, 16, QChar('0'));
        // Add tree item
        model->addItem(Types::Padding, 0, COMPRESSION_ALGORITHM_NONE, name, "", info, QByteArray(), padding, QByteArray(), parent);
    }

    // Search for and parse all volumes
    UINT32 volumeOffset = prevVolumeOffset;
    UINT32 prevVolumeSize = 0;
    UINT32 volumeSize = 0;

    while (true)
    {
        bool msgAlignmentBitsSet = false;
        bool msgUnaligned = false;
        bool msgUnknownRevision = false;

        // Padding between volumes
        if (volumeOffset > prevVolumeOffset + prevVolumeSize) {
            UINT32 paddingSize = volumeOffset - prevVolumeOffset - prevVolumeSize;
            QByteArray padding = bios.mid(prevVolumeOffset + prevVolumeSize, paddingSize);
            // Get info
            name = tr("Padding");
            info = tr("Size: %1")
                .arg(padding.size(), 8, 16, QChar('0'));
            // Add tree item
            model->addItem(Types::Padding, 0, COMPRESSION_ALGORITHM_NONE, name, "", info, QByteArray(), padding, QByteArray(), parent);
        }

        // Get volume size
        result = getVolumeSize(bios, volumeOffset, volumeSize);
        if (result)
            return result;

        //Check that volume is fully present in input
        if (volumeOffset + volumeSize > (UINT32)bios.size()) {
            msg(tr("parseBios: One of volumes inside overlaps the end of data"), parent);
            return ERR_INVALID_VOLUME;
        }

        // Check volume revision and alignment
        EFI_FIRMWARE_VOLUME_HEADER* volumeHeader = (EFI_FIRMWARE_VOLUME_HEADER*)(bios.constData() + volumeOffset);
        UINT32 alignment;
        if (volumeHeader->Revision == 1) {
            // Acquire alignment capability bit
            bool alignmentCap = volumeHeader->Attributes & EFI_FVB_ALIGNMENT_CAP;
            if (!alignmentCap) {
                if (volumeHeader->Attributes & 0xFFFF0000)
                    msgAlignmentBitsSet = true;
            }
        }
        else if (volumeHeader->Revision == 2) {
            // Acquire alignment
            alignment = (UINT32)pow(2.0, (int)(volumeHeader->Attributes & EFI_FVB2_ALIGNMENT) >> 16);

            // Check alignment
            if (volumeOffset % alignment)
                msgUnaligned = true;
        }
        else
            msgUnknownRevision = true;

        // Parse volume
        QModelIndex index;
        UINT8 result = parseVolume(bios.mid(volumeOffset, volumeSize), index, parent);
        if (result)
            msg(tr("parseBios: Volume parsing failed with error %1").arg(result), parent);

        // Show messages
        if (msgAlignmentBitsSet)
            msg("parseBios: Alignment bits set on volume without alignment capability", index);
        if (msgUnaligned)
            msg(tr("parseBios: Unaligned revision 2 volume"), index);
        if (msgUnknownRevision)
            msg(tr("parseBios: Unknown volume revision %1").arg(volumeHeader->Revision), index);

        // Go to next volume
        prevVolumeOffset = volumeOffset;
        prevVolumeSize = volumeSize;

        result = findNextVolume(bios, volumeOffset + prevVolumeSize, volumeOffset);
        if (result) {
            UINT32 endPaddingSize = bios.size() - prevVolumeOffset - prevVolumeSize;
            // Padding at the end of BIOS space
            if (endPaddingSize > 0) {
                QByteArray padding = bios.right(endPaddingSize);
                // Get info
                name = tr("Padding");
                info = tr("Size: %2")
                    .arg(padding.size(), 8, 16, QChar('0'));
                // Add tree item
                model->addItem(Types::Padding, 0, COMPRESSION_ALGORITHM_NONE, name, "", info, QByteArray(), padding, QByteArray(), parent);
            }
            break;
        }
    }

    return ERR_SUCCESS;
}

UINT8 FfsEngine::findNextVolume(const QByteArray & bios, UINT32 volumeOffset, UINT32 & nextVolumeOffset)
{
    int nextIndex = bios.indexOf(EFI_FV_SIGNATURE, volumeOffset);
    if (nextIndex < EFI_FV_SIGNATURE_OFFSET) {
        return ERR_VOLUMES_NOT_FOUND;
    }

    nextVolumeOffset = nextIndex - EFI_FV_SIGNATURE_OFFSET;
    return ERR_SUCCESS;
}

UINT8 FfsEngine::getVolumeSize(const QByteArray & bios, UINT32 volumeOffset, UINT32 & volumeSize)
{
    // Populate volume header
    EFI_FIRMWARE_VOLUME_HEADER* volumeHeader = (EFI_FIRMWARE_VOLUME_HEADER*)(bios.constData() + volumeOffset);

    // Check volume signature
    if (QByteArray((const char*)&volumeHeader->Signature, sizeof(volumeHeader->Signature)) != EFI_FV_SIGNATURE)
        return ERR_INVALID_VOLUME;

    // Calculate volume size using BlockMap
    EFI_FV_BLOCK_MAP_ENTRY* entry = (EFI_FV_BLOCK_MAP_ENTRY*)(bios.constData() + volumeOffset + sizeof(EFI_FIRMWARE_VOLUME_HEADER));
    UINT32 bmVolumeSize = 0;
    while (entry->NumBlocks != 0 && entry->Length != 0) {
        if ((void*)entry > bios.constData() + bios.size())
            return ERR_INVALID_VOLUME;

        bmVolumeSize += entry->NumBlocks * entry->Length;
        entry += 1;
    }

    // Check calculated and stored volume sizes to be the same
    if (volumeHeader->FvLength != bmVolumeSize) {
        // Use smaller value as volume size
        volumeSize = volumeHeader->FvLength < bmVolumeSize ? volumeHeader->FvLength : bmVolumeSize;
    }
    else
        volumeSize = bmVolumeSize;

    return ERR_SUCCESS;
}

UINT8  FfsEngine::parseVolume(const QByteArray & volume, QModelIndex & index, const QModelIndex & parent, const UINT8 mode)
{
    bool msgUnknownFS = false;
    bool msgSizeMismach = false;
    bool msgInvalidChecksum = false;

    // Populate volume header
    EFI_FIRMWARE_VOLUME_HEADER* volumeHeader = (EFI_FIRMWARE_VOLUME_HEADER*)(volume.constData());

    // Calculate volume header size
    UINT32 headerSize;
    if (volumeHeader->Revision > 1 && volumeHeader->ExtHeaderOffset) {
        EFI_FIRMWARE_VOLUME_EXT_HEADER* extendedHeader = (EFI_FIRMWARE_VOLUME_EXT_HEADER*)(volume.constData() + volumeHeader->ExtHeaderOffset);
        headerSize = volumeHeader->ExtHeaderOffset + extendedHeader->ExtHeaderSize;
    }
    else
        headerSize = volumeHeader->HeaderLength;

    // Sanity check after some new crazy MSI images
    headerSize = ALIGN8(headerSize);

    // Check for volume structure to be known
    // Default volume subtype is "normal"
    UINT8 subtype = Subtypes::NormalVolume;
    // FFS GUID v1
    if (QByteArray((const char*)&volumeHeader->FileSystemGuid, sizeof(EFI_GUID)) == EFI_FIRMWARE_FILE_SYSTEM_GUID) {
        // Code can be added here
    }
    // Apple Boot Volume FFS GUID
    else if (QByteArray((const char*)&volumeHeader->FileSystemGuid, sizeof(EFI_GUID)) == EFI_APPLE_BOOT_VOLUME_FILE_SYSTEM_GUID) {
        // Code can be added here
    }
    // Apple Boot Volume FFS GUID
    else if (QByteArray((const char*)&volumeHeader->FileSystemGuid, sizeof(EFI_GUID)) == EFI_APPLE_BOOT_VOLUME_FILE_SYSTEM2_GUID) {
        // Code can be added here
    }
    // FFS GUID v2
    else if (QByteArray((const char*)&volumeHeader->FileSystemGuid, sizeof(EFI_GUID)) == EFI_FIRMWARE_FILE_SYSTEM2_GUID) {
        // Code can be added here
    }
    // NVRAM volume
    else if (QByteArray((const char*)volumeHeader + headerSize, EFI_FIRMWARE_VOLUME_NVRAM_SIGNATURE.length()) == EFI_FIRMWARE_VOLUME_NVRAM_SIGNATURE) {
        subtype = Subtypes::NvramVolume;
    }
    // Other GUID
    else {
        msgUnknownFS = true;
        subtype = Subtypes::UnknownVolume;
    }

    // Check attributes
    // Determine value of empty byte
    char empty = volumeHeader->Attributes & EFI_FVB_ERASE_POLARITY ? '\xFF' : '\x00';

    // Get volume size
    UINT8 result;
    UINT32 volumeSize;

    result = getVolumeSize(volume, 0, volumeSize);
    if (result)
        return result;

    // Check reported size
    if (volumeSize != volumeHeader->FvLength)
        msgSizeMismach = true;
    // Trust header size
    else
        volumeSize = volumeHeader->FvLength;

    // Check header checksum by recalculating it
    if (subtype == Subtypes::NormalVolume && calculateChecksum16((UINT16*)volumeHeader, volumeHeader->HeaderLength))
        msgInvalidChecksum = true;

    // Get info
    QString name = guidToQString(volumeHeader->FileSystemGuid);
    QString info = tr("FileSystem GUID: %1\nSize: %2\nRevision: %3\nAttributes: %4\nErase polarity: %5\nHeader size: %6")
        .arg(guidToQString(volumeHeader->FileSystemGuid))
        .arg(volumeSize, 8, 16, QChar('0'))
        .arg(volumeHeader->Revision)
        .arg(volumeHeader->Attributes, 8, 16, QChar('0'))
        .arg(empty ? "1" : "0")
        .arg(headerSize, 4, 16, QChar('0'));
    // Extended header present
    if (volumeHeader->Revision > 1 && volumeHeader->ExtHeaderOffset) {
        EFI_FIRMWARE_VOLUME_EXT_HEADER* extendedHeader = (EFI_FIRMWARE_VOLUME_EXT_HEADER*)(volume.constData() + volumeHeader->ExtHeaderOffset);
        info += tr("\nExtended header size: %1\nVolume name: %2")
            .arg(extendedHeader->ExtHeaderSize, 8, 16, QChar('0'))
            .arg(guidToQString(extendedHeader->FvName));
    }

    // Add tree item
    QByteArray  header = volume.left(headerSize);
    QByteArray  body = volume.mid(headerSize, volumeSize - headerSize);
    index = model->addItem(Types::Volume, subtype, COMPRESSION_ALGORITHM_NONE, name, "", info, header, body, QByteArray(), parent, mode);

    // Show messages
    if (msgUnknownFS)
        msg(tr("parseVolume: Unknown file system %1").arg(guidToQString(volumeHeader->FileSystemGuid)), index);
    if (msgSizeMismach)
        msg(tr("parseVolume: Volume size stored in header %1 differs from calculated size %2")
        .arg(volumeHeader->FvLength, 8, 16, QChar('0'))
        .arg(volumeSize, 8, 16, QChar('0')), index);
    if (msgInvalidChecksum)
        msg(tr("parseVolume: Volume header checksum is invalid"), index);

    // Do not parse the contents of volumes other then normal
    if (subtype != Subtypes::NormalVolume)
        return ERR_SUCCESS;

    // Search for and parse all files
    UINT32 fileOffset = headerSize;
    UINT32 fileSize;
    QQueue<QByteArray> files;

    while (fileOffset < volumeSize) {
        bool msgUnalignedFile = false;
        bool msgDuplicateGuid = false;

        result = getFileSize(volume, fileOffset, fileSize);
        if (result)
            return result;

        // Check file size to be at least size of EFI_FFS_FILE_HEADER
        if (fileSize < sizeof(EFI_FFS_FILE_HEADER)) {
            msg(tr("parseVolume: Volume has FFS file with invalid size"), index);
            return ERR_INVALID_FILE;
        }

        QByteArray file = volume.mid(fileOffset, fileSize);
        QByteArray header = file.left(sizeof(EFI_FFS_FILE_HEADER));

        // If we are at empty space in the end of volume
        if (header.count(empty) == header.size())
            break; // Exit from loop

        // Check file alignment
        EFI_FFS_FILE_HEADER* fileHeader = (EFI_FFS_FILE_HEADER*)header.constData();
        UINT8 alignmentPower = ffsAlignmentTable[(fileHeader->Attributes & FFS_ATTRIB_DATA_ALIGNMENT) >> 3];
        UINT32 alignment = (UINT32)pow(2.0, alignmentPower);
        if ((fileOffset + sizeof(EFI_FFS_FILE_HEADER)) % alignment)
            msgUnalignedFile = true;

        // Check file GUID
        if (fileHeader->Type != EFI_FV_FILETYPE_PAD && files.indexOf(header.left(sizeof(EFI_GUID))) != -1)
            msgDuplicateGuid = true;

        // Add file GUID to queue
        files.enqueue(header.left(sizeof(EFI_GUID)));

        // Parse file
        QModelIndex fileIndex;
        result = parseFile(file, fileIndex, empty == '\xFF' ? ERASE_POLARITY_TRUE : ERASE_POLARITY_FALSE, index);
        if (result && result != ERR_VOLUMES_NOT_FOUND)
            msg(tr("parseVolume: FFS file parsing failed with error %1").arg(result), index);

        // Show messages
        if (msgUnalignedFile)
            msg(tr("parseVolume: Unaligned file %1").arg(guidToQString(fileHeader->Name)), fileIndex);
        if (msgDuplicateGuid)
            msg(tr("parseVolume: File with duplicate GUID %1").arg(guidToQString(fileHeader->Name)), fileIndex);

        // Move to next file
        fileOffset += fileSize;
        fileOffset = ALIGN8(fileOffset);
    }

    return ERR_SUCCESS;
}

UINT8 FfsEngine::getFileSize(const QByteArray & volume, const UINT32 fileOffset, UINT32 & fileSize)
{
    EFI_FFS_FILE_HEADER* fileHeader = (EFI_FFS_FILE_HEADER*)(volume.constData() + fileOffset);
    fileSize = uint24ToUint32(fileHeader->Size);
    return ERR_SUCCESS;
}

UINT8 FfsEngine::parseFile(const QByteArray & file, QModelIndex & index, const UINT8 erasePolarity, const QModelIndex & parent, const UINT8 mode)
{
    bool msgInvalidDataChecksum = false;
    bool msgInvalidTailValue = false;
    bool msgInvalidType = false;

    // Populate file header
    EFI_FFS_FILE_HEADER* fileHeader = (EFI_FFS_FILE_HEADER*)file.constData();

    // Check file state
    // Construct empty byte for this file
    char empty = erasePolarity ? '\xFF' : '\x00';

    // Check header checksum
    QByteArray header = file.left(sizeof(EFI_FFS_FILE_HEADER));
    QByteArray tempHeader = header;
    EFI_FFS_FILE_HEADER* tempFileHeader = (EFI_FFS_FILE_HEADER*)(tempHeader.data());
    tempFileHeader->IntegrityCheck.Checksum.Header = 0;
    tempFileHeader->IntegrityCheck.Checksum.File = 0;
    UINT8 calculated = calculateChecksum8((UINT8*)tempFileHeader, sizeof(EFI_FFS_FILE_HEADER) - 1);
    if (fileHeader->IntegrityCheck.Checksum.Header != calculated)
    {
        msg(tr("parseFile: %1, stored header checksum %2 differs from calculated %3")
            .arg(guidToQString(fileHeader->Name))
            .arg(fileHeader->IntegrityCheck.Checksum.Header, 2, 16, QChar('0'))
            .arg(calculated, 2, 16, QChar('0')), parent);
    }

    // Check data checksum
    // Data checksum must be calculated
    if (fileHeader->Attributes & FFS_ATTRIB_CHECKSUM) {
        UINT32 bufferSize = file.size() - sizeof(EFI_FFS_FILE_HEADER);
        // Exclude file tail from data checksum calculation
        if (fileHeader->Attributes & FFS_ATTRIB_TAIL_PRESENT)
            bufferSize -= sizeof(UINT16);
        calculated = calculateChecksum8((UINT8*)(file.constData() + sizeof(EFI_FFS_FILE_HEADER)), bufferSize);
        if (fileHeader->IntegrityCheck.Checksum.File != calculated)
            msgInvalidDataChecksum = true;
    }
    // Data checksum must be one of predefined values
    else if (fileHeader->IntegrityCheck.Checksum.File != FFS_FIXED_CHECKSUM && fileHeader->IntegrityCheck.Checksum.File != FFS_FIXED_CHECKSUM2)
        msgInvalidDataChecksum = true;

    // Get file body
    QByteArray body = file.right(file.size() - sizeof(EFI_FFS_FILE_HEADER));
    // Check for file tail presence
    QByteArray tail;
    if (fileHeader->Attributes & FFS_ATTRIB_TAIL_PRESENT)
    {
        //Check file tail;
        tail = body.right(sizeof(UINT16));
        UINT16 tailValue = *(UINT16*)tail.constData();
        if (fileHeader->IntegrityCheck.TailReference != (UINT16)~tailValue)
            msgInvalidTailValue = true;

        // Remove tail from file body
        body = body.left(body.size() - sizeof(UINT16));
    }

    // Parse current file by default
    bool parseCurrentFile = true;
    bool parseAsBios = false;

    // Check file type
    switch (fileHeader->Type)
    {
    case EFI_FV_FILETYPE_ALL:
        parseAsBios = true;
        break;
    case EFI_FV_FILETYPE_RAW:
        parseAsBios = true;
        break;
    case EFI_FV_FILETYPE_FREEFORM:
        break;
    case EFI_FV_FILETYPE_SECURITY_CORE:
        // Set parent volume type to BootVolume
        model->setSubtype(parent, Subtypes::BootVolume);
        break;
    case EFI_FV_FILETYPE_PEI_CORE:
        // Set parent volume type to BootVolume
        model->setSubtype(parent, Subtypes::BootVolume);
        break;
    case EFI_FV_FILETYPE_DXE_CORE:
        break;
    case EFI_FV_FILETYPE_PEIM:
        break;
    case EFI_FV_FILETYPE_DRIVER:
        break;
    case EFI_FV_FILETYPE_COMBINED_PEIM_DRIVER:
        break;
    case EFI_FV_FILETYPE_APPLICATION:
        break;
    case EFI_FV_FILETYPE_SMM:
        break;
    case EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE:
        break;
    case EFI_FV_FILETYPE_COMBINED_SMM_DXE:
        break;
    case EFI_FV_FILETYPE_SMM_CORE:
        break;
    case EFI_FV_FILETYPE_PAD:
        parseCurrentFile = false;
        break;
    default:
        msgInvalidType = true;
        parseCurrentFile = false;
    };

    // Check for empty file
    if (body.count(empty) == body.size()) {
        // No need to parse empty files
        parseCurrentFile = false;
    }

    // Get info
    QString name;
    QString info;
    if (fileHeader->Type != EFI_FV_FILETYPE_PAD)
        name = guidToQString(fileHeader->Name);
    else
        name = tr("Padding");
    info = tr("Name: %1\nType: %2\nAttributes: %3\nSize: %4\nState: %5")
        .arg(guidToQString(fileHeader->Name))
        .arg(fileHeader->Type, 2, 16, QChar('0'))
        .arg(fileHeader->Attributes, 2, 16, QChar('0'))
        .arg(uint24ToUint32(fileHeader->Size), 6, 16, QChar('0'))
        .arg(fileHeader->State, 2, 16, QChar('0'));

    // Add tree item
    index = model->addItem(Types::File, fileHeader->Type, COMPRESSION_ALGORITHM_NONE, name, "", info, header, body, tail, parent, mode);

    // Show messages
    if (msgInvalidDataChecksum)
        msg(tr("parseFile: Invalid data checksum"), index);
    if (msgInvalidTailValue)
        msg(tr("parseFile: Invalid tail value"), index);
    if (msgInvalidType)
        msg(tr("parseFile: Unknown file type %1").arg(fileHeader->Type, 2, 16, QChar('0')), index);

    if (!parseCurrentFile)
        return ERR_SUCCESS;

    // Parse file as BIOS space
    UINT8 result;
    if (parseAsBios) {
        result = parseBios(body, index);
        if (result && result != ERR_VOLUMES_NOT_FOUND)
            msg(tr("parseFile: Parsing file as BIOS failed with error %1").arg(result), index);
        return result;
    }

    // Parse sections
    result = parseSections(body, index);
    if (result)
        return result;

    return ERR_SUCCESS;
}

UINT8 FfsEngine::getSectionSize(const QByteArray & file, const UINT32 sectionOffset, UINT32 & sectionSize)
{
    EFI_COMMON_SECTION_HEADER* sectionHeader = (EFI_COMMON_SECTION_HEADER*)(file.constData() + sectionOffset);
    sectionSize = uint24ToUint32(sectionHeader->Size);
    return ERR_SUCCESS;
}

UINT8 FfsEngine::parseSections(const QByteArray & body, const QModelIndex & parent)
{
    // Search for and parse all sections
    UINT32 sectionOffset = 0;
    UINT32 sectionSize;
    UINT32 bodySize = body.size();
    UINT8  result;

    while (true) {
        // Get section size
        result = getSectionSize(body, sectionOffset, sectionSize);
        if (result)
            return result;

        // Parse section
        QModelIndex sectionIndex;
        result = parseSection(body.mid(sectionOffset, sectionSize), sectionIndex, parent);
        if (result)
            return result;

        // Move to next section
        sectionOffset += sectionSize;
        sectionOffset = ALIGN4(sectionOffset);

        // Exit from loop if no sections left
        if (sectionOffset >= bodySize)
            break;
    }

    return ERR_SUCCESS;
}

UINT8 FfsEngine::parseSection(const QByteArray & section, QModelIndex & index, const QModelIndex & parent, const UINT8 mode)
{
    EFI_COMMON_SECTION_HEADER* sectionHeader = (EFI_COMMON_SECTION_HEADER*)(section.constData());
    UINT32 sectionSize = uint24ToUint32(sectionHeader->Size);
    QString name = sectionTypeToQString(sectionHeader->Type) + tr(" section");
    QString info;
    QByteArray header;
    QByteArray body;
    UINT32 headerSize;
    UINT8 result;

    switch (sectionHeader->Type) {
        // Encapsulated sections
    case EFI_SECTION_COMPRESSION:
    {
        bool parseCurrentSection = true;
        QByteArray decompressed;
        UINT8 algorithm;
        EFI_COMPRESSION_SECTION* compressedSectionHeader = (EFI_COMPRESSION_SECTION*)sectionHeader;
        header = section.left(sizeof(EFI_COMPRESSION_SECTION));
        body = section.mid(sizeof(EFI_COMPRESSION_SECTION), sectionSize - sizeof(EFI_COMPRESSION_SECTION));
        algorithm = COMPRESSION_ALGORITHM_UNKNOWN;
        // Decompress section
        result = decompress(body, compressedSectionHeader->CompressionType, decompressed, &algorithm);
        if (result)
            parseCurrentSection = false;

        // Get info
        info = tr("Type: %1\nSize: %2\nCompression type: %3\nDecompressed size: %4")
            .arg(sectionHeader->Type, 2, 16, QChar('0'))
            .arg(body.size(), 6, 16, QChar('0'))
            .arg(compressionTypeToQString(algorithm))
            .arg(compressedSectionHeader->UncompressedLength, 8, 16, QChar('0'));

        // Add tree item
        index = model->addItem(Types::Section, sectionHeader->Type, algorithm, name, "", info, header, body, QByteArray(), parent, mode);

        // Show message
        if (!parseCurrentSection)
            msg(tr("parseSection: Decompression failed with error %1").arg(result), index);
        else { // Parse decompressed data
            result = parseSections(decompressed, index);
            if (result)
                return result;
        }
    }
        break;
    case EFI_SECTION_GUID_DEFINED:
    {
        bool parseCurrentSection = true;
        bool msgUnknownGuid = false;
        bool msgInvalidCrc = false;
        bool msgUnknownAuth = false;

        EFI_GUID_DEFINED_SECTION* guidDefinedSectionHeader;
        header = section.left(sizeof(EFI_GUID_DEFINED_SECTION));
        guidDefinedSectionHeader = (EFI_GUID_DEFINED_SECTION*)(header.constData());
        header = section.left(guidDefinedSectionHeader->DataOffset);
        guidDefinedSectionHeader = (EFI_GUID_DEFINED_SECTION*)(header.constData());
        body = section.mid(guidDefinedSectionHeader->DataOffset, sectionSize - guidDefinedSectionHeader->DataOffset);
        QByteArray decompressed = body;

        // Get info
        name = guidToQString(guidDefinedSectionHeader->SectionDefinitionGuid);
        info = tr("GUID: %1\nType: %2\nSize: %3\nData offset: %4\nAttributes: %5")
            .arg(name)
            .arg(sectionHeader->Type, 2, 16, QChar('0'))
            .arg(body.size(), 6, 16, QChar('0'))
            .arg(guidDefinedSectionHeader->DataOffset, 4, 16, QChar('0'))
            .arg(guidDefinedSectionHeader->Attributes, 4, 16, QChar('0'));

        UINT8 algorithm = COMPRESSION_ALGORITHM_NONE;
        // Check if section requires processing
        if (guidDefinedSectionHeader->Attributes & EFI_GUIDED_SECTION_PROCESSING_REQUIRED) {
            // Tiano compressed section
            if (QByteArray((const char*)&guidDefinedSectionHeader->SectionDefinitionGuid, sizeof(EFI_GUID)) == EFI_GUIDED_SECTION_TIANO) {
                algorithm = COMPRESSION_ALGORITHM_UNKNOWN;
                info += tr("\nCompression type: Tiano");
                result = decompress(body, EFI_STANDARD_COMPRESSION, decompressed, &algorithm);
                if (result)
                    parseCurrentSection = false;
            }
            // LZMA compressed section
            else if (QByteArray((const char*)&guidDefinedSectionHeader->SectionDefinitionGuid, sizeof(EFI_GUID)) == EFI_GUIDED_SECTION_LZMA) {
                algorithm = COMPRESSION_ALGORITHM_UNKNOWN;
                info += tr("\nCompression type: LZMA");
                result = decompress(body, EFI_CUSTOMIZED_COMPRESSION, decompressed, &algorithm);
                if (result)
                    parseCurrentSection = false;
            }
            // Unknown GUIDed section
            else {
                msgUnknownGuid = true;
                parseCurrentSection = false;
            }
        }
        // Check if section requires checksum calculation
        else if (guidDefinedSectionHeader->Attributes & EFI_GUIDED_SECTION_AUTH_STATUS_VALID)
        {
            // CRC32 section
            if (QByteArray((const char*)&guidDefinedSectionHeader->SectionDefinitionGuid, sizeof(EFI_GUID)) == EFI_GUIDED_SECTION_CRC32) {
                info += tr("\nChecksum type: CRC32");
                // Calculate CRC32 of section data
                UINT32 crc = crc32(0, NULL, 0);
                crc = crc32(crc, (const UINT8*)body.constData(), body.size());
                // Check stored CRC32
                if (crc == *(UINT32*)(header.constData() + sizeof(EFI_GUID_DEFINED_SECTION))) {
                    info += tr("\nChecksum: valid");
                }
                else {
                    info += tr("\nChecksum: invalid");
                    msgInvalidCrc = true;
                }
            }
            else
                msgUnknownAuth = true;
        }

        // Add tree item
        index = model->addItem(Types::Section, sectionHeader->Type, algorithm, name, "", info, header, body, QByteArray(), parent, mode);

        // Show messages
        if (msgUnknownGuid)
            msg(tr("parseSection: GUID defined section with unknown processing method"), index);
        if (msgUnknownAuth)
            msg(tr("parseSection: GUID defined section with unknown authentication method"), index);
        if (msgInvalidCrc)
            msg(tr("parseSection: GUID defined section with invalid CRC32"), index);

        if (!parseCurrentSection) {
            msg(tr("parseSection: GUID defined section can not be processed"), index);
        }
        else { // Parse decompressed data
            result = parseSections(decompressed, index);
            if (result)
                return result;
        }
    }
        break;
    case EFI_SECTION_DISPOSABLE:
    {
        header = section.left(sizeof(EFI_DISPOSABLE_SECTION));
        body = section.mid(sizeof(EFI_DISPOSABLE_SECTION), sectionSize - sizeof(EFI_DISPOSABLE_SECTION));

        // Get info
        info = tr("parseSection: %1\nSize: %2")
            .arg(sectionHeader->Type, 2, 16, QChar('0'))
            .arg(body.size(), 6, 16, QChar('0'));

        // Add tree item
        index = model->addItem(Types::Section, sectionHeader->Type, COMPRESSION_ALGORITHM_NONE, name, "", info, header, body, QByteArray(), parent, mode);

        // Parse section body
        result = parseSections(body, index);
        if (result)
            return result;
    }
        break;
        // Leaf sections
    case EFI_SECTION_PE32:
    case EFI_SECTION_TE:
    case EFI_SECTION_PIC:
    case EFI_SECTION_DXE_DEPEX:
    case EFI_SECTION_PEI_DEPEX:
    case EFI_SECTION_SMM_DEPEX:
    case EFI_SECTION_COMPATIBILITY16: {
        headerSize = sizeOfSectionHeader(sectionHeader);
        header = section.left(headerSize);
        body = section.mid(headerSize, sectionSize - headerSize);

        // Get info
        info = tr("Type: %1\nSize: %2")
            .arg(sectionHeader->Type, 2, 16, QChar('0'))
            .arg(body.size(), 6, 16, QChar('0'));

        // Add tree item
        index = model->addItem(Types::Section, sectionHeader->Type, COMPRESSION_ALGORITHM_NONE, name, "", info, header, body, QByteArray(), parent, mode);

        // Special case of PEI Core
        if ((sectionHeader->Type == EFI_SECTION_PE32 || sectionHeader->Type == EFI_SECTION_TE) && model->subtype(parent) == EFI_FV_FILETYPE_PEI_CORE) {
            result = getEntryPoint(model->body(index), oldPeiCoreEntryPoint);
            if (result)
                msg(tr("parseSection: Can't get entry point of image file"), index);
        }
    }
        break;
    case EFI_SECTION_FREEFORM_SUBTYPE_GUID: {
        header = section.left(sizeof(EFI_FREEFORM_SUBTYPE_GUID_SECTION));
        body = section.mid(sizeof(EFI_FREEFORM_SUBTYPE_GUID_SECTION), sectionSize - sizeof(EFI_FREEFORM_SUBTYPE_GUID_SECTION));

        EFI_FREEFORM_SUBTYPE_GUID_SECTION* fsgHeader = (EFI_FREEFORM_SUBTYPE_GUID_SECTION*)sectionHeader;
        // Get info
        info = tr("Type: %1\nSize: %2\nSubtype GUID: %3")
            .arg(fsgHeader->Type, 2, 16, QChar('0'))
            .arg(body.size(), 6, 16, QChar('0'))
            .arg(guidToQString(fsgHeader->SubTypeGuid));

        // Add tree item
        index = model->addItem(Types::Section, sectionHeader->Type, COMPRESSION_ALGORITHM_NONE, name, "", info, header, body, QByteArray(), parent, mode);
    }
        break;
    case EFI_SECTION_VERSION: {
        header = section.left(sizeof(EFI_VERSION_SECTION));
        body = section.mid(sizeof(EFI_VERSION_SECTION), sectionSize - sizeof(EFI_VERSION_SECTION));

        EFI_VERSION_SECTION* versionHeader = (EFI_VERSION_SECTION*)sectionHeader;

        // Get info
        info = tr("Type: %1\nSize: %2\nBuild number: %3\nVersion string: %4")
            .arg(versionHeader->Type, 2, 16, QChar('0'))
            .arg(body.size(), 6, 16, QChar('0'))
            .arg(versionHeader->BuildNumber, 4, 16, QChar('0'))
            .arg(QString::fromUtf16((const ushort*)body.constData()));

        // Add tree item
        index = model->addItem(Types::Section, sectionHeader->Type, COMPRESSION_ALGORITHM_NONE, name, "", info, header, body, QByteArray(), parent, mode);
    }
        break;
    case EFI_SECTION_USER_INTERFACE: {
        header = section.left(sizeof(EFI_USER_INTERFACE_SECTION));
        body = section.mid(sizeof(EFI_USER_INTERFACE_SECTION), sectionSize - sizeof(EFI_USER_INTERFACE_SECTION));
        QString text = QString::fromUtf16((const ushort*)body.constData());

        // Get info
        info = tr("Type: %1\nSize: %2\nText: %3")
            .arg(sectionHeader->Type, 2, 16, QChar('0'))
            .arg(body.size(), 6, 16, QChar('0'))
            .arg(text);

        // Add tree item
        index = model->addItem(Types::Section, sectionHeader->Type, COMPRESSION_ALGORITHM_NONE, name, "", info, header, body, QByteArray(), parent, mode);

        // Rename parent file
        model->setTextString(model->findParentOfType(parent, Types::File), text);
    }
        break;
    case EFI_SECTION_FIRMWARE_VOLUME_IMAGE: {
        header = section.left(sizeof(EFI_FIRMWARE_VOLUME_IMAGE_SECTION));
        body = section.mid(sizeof(EFI_FIRMWARE_VOLUME_IMAGE_SECTION), sectionSize - sizeof(EFI_FIRMWARE_VOLUME_IMAGE_SECTION));

        // Get info
        info = tr("Type: %1\nSize: %2")
            .arg(sectionHeader->Type, 2, 16, QChar('0'))
            .arg(body.size(), 6, 16, QChar('0'));

        // Add tree item
        index = model->addItem(Types::Section, sectionHeader->Type, COMPRESSION_ALGORITHM_NONE, name, "", info, header, body, QByteArray(), parent, mode);

        // Parse section body as BIOS space
        result = parseBios(body, index);
        if (result && result != ERR_VOLUMES_NOT_FOUND) {
            msg(tr("parseSection: Parsing firmware volume image section as BIOS failed with error %1").arg(result), index);
            return result;
        }
    }
        break;
    case EFI_SECTION_RAW: {
        header = section.left(sizeof(EFI_RAW_SECTION));
        body = section.mid(sizeof(EFI_RAW_SECTION), sectionSize - sizeof(EFI_RAW_SECTION));

        // Get info
        info = tr("Type: %1\nSize: %2")
            .arg(sectionHeader->Type, 2, 16, QChar('0'))
            .arg(body.size(), 6, 16, QChar('0'));

        // Add tree item
        index = model->addItem(Types::Section, sectionHeader->Type, COMPRESSION_ALGORITHM_NONE, name, "", info, header, body, QByteArray(), parent, mode);

        // Parse section body as BIOS space
        result = parseBios(body, index);
        if (result && result != ERR_VOLUMES_NOT_FOUND) {
            msg(tr("parseSection: Parsing raw section as BIOS failed with error %1").arg(result), index);
            return result;
        }
    }
        break;
    default:
        header = section.left(sizeof(EFI_COMMON_SECTION_HEADER));
        body = section.mid(sizeof(EFI_COMMON_SECTION_HEADER), sectionSize - sizeof(EFI_COMMON_SECTION_HEADER));
        // Get info
        info = tr("Type: %1\nSize: %2")
            .arg(sectionHeader->Type, 2, 16, QChar('0'))
            .arg(body.size(), 6, 16, QChar('0'));

        // Add tree item
        index = model->addItem(Types::Section, sectionHeader->Type, COMPRESSION_ALGORITHM_NONE, name, "", info, header, body, QByteArray(), parent, mode);
        msg(tr("parseSection: Section with unknown type %1").arg(sectionHeader->Type, 2, 16, QChar('0')), index);
    }
    return ERR_SUCCESS;
}

// Operations on tree items
UINT8 FfsEngine::create(const QModelIndex & index, const UINT8 type, const QByteArray & header, const QByteArray & body, const UINT8 mode, const UINT8 action, const UINT8 algorithm)
{
    QByteArray created;
    UINT8 result;
    QModelIndex fileIndex;

    if (!index.isValid() || !index.parent().isValid())
        return ERR_INVALID_PARAMETER;

    QModelIndex parent;
    if (mode == CREATE_MODE_BEFORE || mode == CREATE_MODE_AFTER)
        parent = index.parent();
    else
        parent = index;

    // Create item
    if (type == Types::Region) {
        UINT8 subtype = model->subtype(index);
        switch (subtype) {
        case Subtypes::BiosRegion:
            result = parseBiosRegion(body, fileIndex, index, mode);
            break;
        case Subtypes::MeRegion:
            result = parseMeRegion(body, fileIndex, index, mode);
            break;
        case Subtypes::GbeRegion:
            result = parseGbeRegion(body, fileIndex, index, mode);
            break;
        case Subtypes::PdrRegion:
            result = parsePdrRegion(body, fileIndex, index, mode);
            break;
        default:
            return ERR_NOT_IMPLEMENTED;
        }

        if (result && result != ERR_VOLUMES_NOT_FOUND)
            return result;

        // Set action
        model->setAction(fileIndex, action);
    }
    else if (type == Types::File) {
        if (model->type(parent) != Types::Volume)
            return ERR_INVALID_FILE;

        EFI_FIRMWARE_VOLUME_HEADER* volumeHeader = (EFI_FIRMWARE_VOLUME_HEADER*)model->header(parent).constData();
        UINT8 revision = volumeHeader->Revision;
        bool erasePolarity = volumeHeader->Attributes & EFI_FVB_ERASE_POLARITY;

        if (header.size() != sizeof(EFI_FFS_FILE_HEADER))
            return ERR_INVALID_FILE;

        QByteArray newHeader = header;
        EFI_FFS_FILE_HEADER* fileHeader = (EFI_FFS_FILE_HEADER*)newHeader.data();

        // Correct file size
        UINT8 tailSize = fileHeader->Attributes & FFS_ATTRIB_TAIL_PRESENT ? sizeof(UINT16) : 0;
        uint32ToUint24(sizeof(EFI_FFS_FILE_HEADER) + body.size() + tailSize, fileHeader->Size);

        // Recalculate header checksum
        fileHeader->IntegrityCheck.Checksum.Header = 0;
        fileHeader->IntegrityCheck.Checksum.File = 0;
        fileHeader->IntegrityCheck.Checksum.Header = calculateChecksum8((UINT8*)fileHeader, sizeof(EFI_FFS_FILE_HEADER) - 1);

        // Recalculate data checksum, if needed
        if (fileHeader->Attributes & FFS_ATTRIB_CHECKSUM)
            fileHeader->IntegrityCheck.Checksum.File = calculateChecksum8((UINT8*)body.constData(), body.size());
        else if (revision == 1)
            fileHeader->IntegrityCheck.Checksum.File = FFS_FIXED_CHECKSUM;
        else
            fileHeader->IntegrityCheck.Checksum.File = FFS_FIXED_CHECKSUM2;

        // Append body
        created.append(body);

        // Append tail, if needed
        if (tailSize) {
            UINT8 ht = ~fileHeader->IntegrityCheck.Checksum.Header;
            UINT8 ft = ~fileHeader->IntegrityCheck.Checksum.File;
            created.append(ht).append(ft);
        }

        // Set file state
        UINT8 state = EFI_FILE_DATA_VALID | EFI_FILE_HEADER_VALID | EFI_FILE_HEADER_CONSTRUCTION;
        if (erasePolarity)
            state = ~state;
        fileHeader->State = state;

        // Prepend header
        created.prepend(newHeader);

        // Parse file
        result = parseFile(created, fileIndex, erasePolarity ? ERASE_POLARITY_TRUE : ERASE_POLARITY_FALSE, index, mode);
        if (result && result != ERR_VOLUMES_NOT_FOUND)
            return result;

        // Set action
        model->setAction(fileIndex, action);

        // Rebase all PEI-files that follow
        rebasePeiFiles(fileIndex);
    }
    else if (type == Types::Section) {
        if (model->type(parent) != Types::File && model->type(parent) != Types::Section)
            return ERR_INVALID_SECTION;

        if (header.size() < (int) sizeof(EFI_COMMON_SECTION_HEADER))
            return ERR_INVALID_SECTION;

        QByteArray newHeader = header;
        EFI_COMMON_SECTION_HEADER* commonHeader = (EFI_COMMON_SECTION_HEADER*)newHeader.data();

        switch (commonHeader->Type)
        {
        case EFI_SECTION_COMPRESSION: {
            EFI_COMPRESSION_SECTION* sectionHeader = (EFI_COMPRESSION_SECTION*)newHeader.data();
            // Correct uncompressed size
            sectionHeader->UncompressedLength = body.size();

            // Set compression type
            if (algorithm == COMPRESSION_ALGORITHM_NONE)
                sectionHeader->CompressionType = EFI_NOT_COMPRESSED;
            else if (algorithm == COMPRESSION_ALGORITHM_EFI11 || algorithm == COMPRESSION_ALGORITHM_TIANO)
                sectionHeader->CompressionType = EFI_STANDARD_COMPRESSION;
            else if (algorithm == COMPRESSION_ALGORITHM_LZMA || algorithm == COMPRESSION_ALGORITHM_IMLZMA)
                sectionHeader->CompressionType = EFI_CUSTOMIZED_COMPRESSION;
            else
                return ERR_UNKNOWN_COMPRESSION_ALGORITHM;

            // Compress body
            QByteArray compressed;
            result = compress(body, algorithm, compressed);
            if (result)
                return result;

            // Correct section size
            uint32ToUint24(header.size() + compressed.size(), commonHeader->Size);

            // Append header and body
            created.append(newHeader).append(compressed);

            // Parse section
            QModelIndex sectionIndex;
            result = parseSection(created, sectionIndex, index, mode);
            if (result && result != ERR_VOLUMES_NOT_FOUND)
                return result;

            // Set create action
            model->setAction(sectionIndex, action);

            // Find parent file for rebase
            fileIndex = model->findParentOfType(parent, Types::File);
        }
            break;
        case EFI_SECTION_GUID_DEFINED:{
            // Compress body
            QByteArray compressed;
            result = compress(body, algorithm, compressed);
            if (result)
                return result;

            // Correct section size
            uint32ToUint24(header.size() + compressed.size(), commonHeader->Size);

            // Append header and body
            created.append(newHeader).append(compressed);

            // Parse section
            QModelIndex sectionIndex;
            result = parseSection(created, sectionIndex, index, mode);
            if (result && result != ERR_VOLUMES_NOT_FOUND)
                return result;

            // Set create action
            model->setAction(sectionIndex, action);

            // Find parent file for rebase
            fileIndex = model->findParentOfType(parent, Types::File);
        }
            break;
        default:
            // Correct section size
            uint32ToUint24(header.size() + body.size(), commonHeader->Size);

            // Append header and body
            created.append(newHeader).append(body);

            // Parse section
            QModelIndex sectionIndex;
            result = parseSection(created, sectionIndex, index, mode);
            if (result && result != ERR_VOLUMES_NOT_FOUND)
                return result;

            // Set create action
            model->setAction(sectionIndex, action);

            // Find parent file for rebase
            fileIndex = model->findParentOfType(parent, Types::File);
        }

        // Rebase all PEI-files that follow
        rebasePeiFiles(fileIndex);
    }
    else
        return ERR_NOT_IMPLEMENTED;

    return ERR_SUCCESS;
}

void FfsEngine::rebasePeiFiles(const QModelIndex & index)
{
    // Rebase all PE32 and TE sections in PEI-files after modified file
    for (int i = index.row(); i < model->rowCount(index.parent()); i++) {
        // PEI-file
        QModelIndex currentFileIndex = index.parent().child(i, 0);
        if (model->subtype(currentFileIndex) == EFI_FV_FILETYPE_PEI_CORE ||
            model->subtype(currentFileIndex) == EFI_FV_FILETYPE_PEIM ||
            model->subtype(currentFileIndex) == EFI_FV_FILETYPE_COMBINED_PEIM_DRIVER) {
            for (int j = 0; j < model->rowCount(currentFileIndex); j++) {
                // Section in that file
                QModelIndex currentSectionIndex = currentFileIndex.child(j, 0);
                // If section stores PE32 or TE image
                if (model->subtype(currentSectionIndex) == EFI_SECTION_PE32 || model->subtype(currentSectionIndex) == EFI_SECTION_TE)
                    // Set rebase action
                    model->setAction(currentSectionIndex, Actions::Rebase);
            }
        }
    }
}

UINT8 FfsEngine::insert(const QModelIndex & index, const QByteArray & object, const UINT8 mode)
{
    if (!index.isValid() || !index.parent().isValid())
        return ERR_INVALID_PARAMETER;

    QModelIndex parent;
    if (mode == CREATE_MODE_BEFORE || mode == CREATE_MODE_AFTER)
        parent = index.parent();
    else
        parent = index;

    // Determine type of item to insert
    UINT8 type;
    UINT32 headerSize;
    if (model->type(parent) == Types::Volume) {
        type = Types::File;
        headerSize = sizeof(EFI_FFS_FILE_HEADER);
    }
    else if (model->type(parent) == Types::File) {
        type = Types::Section;
        EFI_COMMON_SECTION_HEADER* commonHeader = (EFI_COMMON_SECTION_HEADER*)object.constData();
        headerSize = sizeOfSectionHeader(commonHeader);
    }
    else if (model->type(parent) == Types::Section) {
        type = Types::Section;
        EFI_COMMON_SECTION_HEADER* commonHeader = (EFI_COMMON_SECTION_HEADER*)object.constData();
        headerSize = sizeOfSectionHeader(commonHeader);
    }
    else
        return ERR_NOT_IMPLEMENTED;

    return create(index, type, object.left(headerSize), object.right(object.size() - headerSize), mode, Actions::Insert);
}

UINT8 FfsEngine::replace(const QModelIndex & index, const QByteArray & object, const UINT8 mode)
{
    if (!index.isValid())
        return ERR_INVALID_PARAMETER;

    // Determine type of item to replace
    UINT32 headerSize;
    UINT8 result;
    if (model->type(index) == Types::Region) {
        if (mode == REPLACE_MODE_AS_IS)
            result = create(index, Types::Region, QByteArray(), object, CREATE_MODE_AFTER, Actions::Replace);
        else
            return ERR_NOT_IMPLEMENTED;
    }
    else if (model->type(index) == Types::File) {
        if (mode == REPLACE_MODE_AS_IS) {
            headerSize = sizeof(EFI_FFS_FILE_HEADER);
            result = create(index, Types::File, object.left(headerSize), object.right(object.size() - headerSize), CREATE_MODE_AFTER, Actions::Replace);
        }
        else if (mode == REPLACE_MODE_BODY)
            result = create(index, Types::File, model->header(index), object, CREATE_MODE_AFTER, Actions::Replace);
        else
            return ERR_NOT_IMPLEMENTED;
    }
    else if (model->type(index) == Types::Section) {
        if (mode == REPLACE_MODE_AS_IS) {
            EFI_COMMON_SECTION_HEADER* commonHeader = (EFI_COMMON_SECTION_HEADER*)object.constData();
            headerSize = sizeOfSectionHeader(commonHeader);
            result = create(index, Types::Section, object.left(headerSize), object.right(object.size() - headerSize), CREATE_MODE_AFTER, Actions::Replace);
        }
        else if (mode == REPLACE_MODE_BODY) {
            result = create(index, Types::Section, model->header(index), object, CREATE_MODE_AFTER, Actions::Replace, model->compression(index));
        }
        else
            return ERR_NOT_IMPLEMENTED;
    }
    else
        return ERR_NOT_IMPLEMENTED;

    // Check create result
    if (result)
        return result;

    // Set remove action to replaced item
    model->setAction(index, Actions::Remove);

    return ERR_SUCCESS;
}

UINT8 FfsEngine::extract(const QModelIndex & index, QByteArray & extracted, const UINT8 mode)
{
    if (!index.isValid())
        return ERR_INVALID_PARAMETER;

    if (mode == EXTRACT_MODE_AS_IS) {
        // Extract as is, with header, body and tail
        extracted.clear();
        extracted.append(model->header(index));
        extracted.append(model->body(index));
        extracted.append(model->tail(index));
    }
    else if (mode == EXTRACT_MODE_BODY) {
        // Extract without header and tail
        extracted.clear();
        // Special case of compressed bodies
        if (model->type(index) == Types::Section) {
            QByteArray decompressed;
            UINT8 result;
            if (model->subtype(index) == EFI_SECTION_COMPRESSION) {
                EFI_COMPRESSION_SECTION* compressedHeader = (EFI_COMPRESSION_SECTION*)model->header(index).constData();
                result = decompress(model->body(index), compressedHeader->CompressionType, decompressed);
                if (result)
                    return result;
                extracted.append(decompressed);
                return ERR_SUCCESS;
            }
            else if (model->subtype(index) == EFI_SECTION_GUID_DEFINED) {
                QByteArray decompressed;
                // Check if section requires processing
                EFI_GUID_DEFINED_SECTION* guidDefinedSectionHeader = (EFI_GUID_DEFINED_SECTION*)model->header(index).constData();
                if (guidDefinedSectionHeader->Attributes & EFI_GUIDED_SECTION_PROCESSING_REQUIRED) {
                    // Try to decompress section body using both known compression algorithms
                    result = decompress(model->body(index), EFI_STANDARD_COMPRESSION, decompressed);
                    if (result) {
                        result = decompress(model->body(index), EFI_CUSTOMIZED_COMPRESSION, decompressed);
                        if (result)
                            return result;
                    }
                    extracted.append(decompressed);
                    return ERR_SUCCESS;
                }
            }
        }

        extracted.append(model->body(index));
    }
    else
        return ERR_UNKNOWN_EXTRACT_MODE;

    return ERR_SUCCESS;
}

UINT8 FfsEngine::remove(const QModelIndex & index)
{
    if (!index.isValid())
        return ERR_INVALID_PARAMETER;

    // Set action for the item
    model->setAction(index, Actions::Remove);

    QModelIndex fileIndex;

    if (model->type(index) == Types::Volume && model->rowCount(index) > 0)
        fileIndex = index.child(0, 0);
    else if (model->type(index) == Types::File)
        fileIndex = index;
    else if (model->type(index) == Types::Section)
        fileIndex = model->findParentOfType(index, Types::File);
    else
        return ERR_SUCCESS;

    // Rebase all PEI-files that follow
    rebasePeiFiles(fileIndex);

    return ERR_SUCCESS;
}

UINT8 FfsEngine::rebuild(const QModelIndex & index)
{
    if (!index.isValid())
        return ERR_INVALID_PARAMETER;

    // Set action for the item
    model->setAction(index, Actions::Rebuild);

    QModelIndex fileIndex;

    if (model->type(index) == Types::Volume && model->rowCount(index) > 0)
        fileIndex = index.child(0, 0);
    else if (model->type(index) == Types::File)
        fileIndex = index;
    else if (model->type(index) == Types::Section)
        fileIndex = model->findParentOfType(index, Types::File);
    else
        return ERR_SUCCESS;

    // Rebase all PEI-files that follow
    rebasePeiFiles(fileIndex);

    return ERR_SUCCESS;
}

// Compression routines
UINT8 FfsEngine::decompress(const QByteArray & compressedData, const UINT8 compressionType, QByteArray & decompressedData, UINT8 * algorithm)
{
    UINT8* data;
    UINT32 dataSize;
    UINT8* decompressed;
    UINT32 decompressedSize = 0;
    UINT8* scratch;
    UINT32 scratchSize = 0;
    EFI_TIANO_HEADER* header;

    switch (compressionType)
    {
    case EFI_NOT_COMPRESSED:
        decompressedData = compressedData;
        if (algorithm)
            *algorithm = COMPRESSION_ALGORITHM_NONE;
        return ERR_SUCCESS;
    case EFI_STANDARD_COMPRESSION:
        // Get buffer sizes
        data = (UINT8*)compressedData.constData();
        dataSize = compressedData.size();

        // Check header to be valid
        header = (EFI_TIANO_HEADER*)data;
        if (header->CompSize + sizeof(EFI_TIANO_HEADER) != dataSize)
            return ERR_STANDARD_DECOMPRESSION_FAILED;

        // Get info function is the same for both algorithms
        if (ERR_SUCCESS != EfiTianoGetInfo(data, dataSize, &decompressedSize, &scratchSize))
            return ERR_STANDARD_DECOMPRESSION_FAILED;

        // Allocate memory
        decompressed = new UINT8[decompressedSize];
        scratch = new UINT8[scratchSize];

        // Decompress section data
        //TODO: separate EFI1.1 from Tiano another way
        // Try Tiano decompression first
        if (ERR_SUCCESS != TianoDecompress(data, dataSize, decompressed, decompressedSize, scratch, scratchSize)) {
            // Not Tiano, try EFI 1.1
            if (ERR_SUCCESS != EfiDecompress(data, dataSize, decompressed, decompressedSize, scratch, scratchSize)) {
                if (algorithm)
                    *algorithm = COMPRESSION_ALGORITHM_UNKNOWN;
                delete[] decompressed;
                delete[] scratch;
                return ERR_STANDARD_DECOMPRESSION_FAILED;
            }
            else if (algorithm)
                *algorithm = COMPRESSION_ALGORITHM_EFI11;
        }
        else if (algorithm)
            *algorithm = COMPRESSION_ALGORITHM_TIANO;

        decompressedData = QByteArray((const char*)decompressed, decompressedSize);

        delete[] decompressed;
        delete[] scratch;
        return ERR_SUCCESS;
    case EFI_CUSTOMIZED_COMPRESSION:
        // Get buffer sizes
        data = (UINT8*)compressedData.constData();
        dataSize = compressedData.size();

        // Get info
        if (ERR_SUCCESS != LzmaGetInfo(data, dataSize, &decompressedSize))
            return ERR_CUSTOMIZED_DECOMPRESSION_FAILED;

        // Allocate memory
        decompressed = new UINT8[decompressedSize];

        // Decompress section data
        if (ERR_SUCCESS != LzmaDecompress(data, dataSize, decompressed)) {
            // Intel modified LZMA workaround
            EFI_COMMON_SECTION_HEADER* shittySectionHeader;
            UINT32 shittySectionSize;
            // Shitty compressed section with a section header between COMPRESSED_SECTION_HEADER and LZMA_HEADER
            // We must determine section header size by checking it's type before we can unpack that non-standard compressed section
            shittySectionHeader = (EFI_COMMON_SECTION_HEADER*)data;
            shittySectionSize = sizeOfSectionHeader(shittySectionHeader);

            // Decompress section data once again
            data += shittySectionSize;

            // Get info again
            if (ERR_SUCCESS != LzmaGetInfo(data, dataSize, &decompressedSize)) {
                delete[] decompressed;
                return ERR_CUSTOMIZED_DECOMPRESSION_FAILED;
            }

            // Decompress section data again
            if (ERR_SUCCESS != LzmaDecompress(data, dataSize, decompressed)) {
                if (algorithm)
                    *algorithm = COMPRESSION_ALGORITHM_UNKNOWN;
                delete[] decompressed;
                return ERR_CUSTOMIZED_DECOMPRESSION_FAILED;
            }
            else {
                if (algorithm)
                    *algorithm = COMPRESSION_ALGORITHM_IMLZMA;
                decompressedData = QByteArray((const char*)decompressed, decompressedSize);
            }
        }
        else {
            if (algorithm)
                *algorithm = COMPRESSION_ALGORITHM_LZMA;
            decompressedData = QByteArray((const char*)decompressed, decompressedSize);
        }

        delete[] decompressed;
        return ERR_SUCCESS;
    default:
        msg(tr("decompress: Unknown compression type (%1)").arg(compressionType));
        if (algorithm)
            *algorithm = COMPRESSION_ALGORITHM_UNKNOWN;
        return ERR_UNKNOWN_COMPRESSION_ALGORITHM;
    }
}

UINT8 FfsEngine::compress(const QByteArray & data, const UINT8 algorithm, QByteArray & compressedData)
{
    UINT8* compressed;

    switch (algorithm) {
    case COMPRESSION_ALGORITHM_NONE:
    {
        compressedData = data;
        return ERR_SUCCESS;
    }
        break;
    case COMPRESSION_ALGORITHM_EFI11:
    {
        UINT64 compressedSize = 0;
        if (EfiCompress(data.constData(), data.size(), NULL, &compressedSize) != ERR_BUFFER_TOO_SMALL)
            return ERR_STANDARD_COMPRESSION_FAILED;
        compressed = new UINT8[compressedSize];
        if (EfiCompress(data.constData(), data.size(), compressed, &compressedSize) != ERR_SUCCESS) {
            delete[] compressed;
            return ERR_STANDARD_COMPRESSION_FAILED;
        }
        compressedData = QByteArray((const char*)compressed, compressedSize);
        delete[] compressed;
        return ERR_SUCCESS;
    }
        break;
    case COMPRESSION_ALGORITHM_TIANO:
    {
        UINT64 compressedSize = 0;
        if (TianoCompress(data.constData(), data.size(), NULL, &compressedSize) != ERR_BUFFER_TOO_SMALL)
            return ERR_STANDARD_COMPRESSION_FAILED;
        compressed = new UINT8[compressedSize];
        if (TianoCompress(data.constData(), data.size(), compressed, &compressedSize) != ERR_SUCCESS) {
            delete[] compressed;
            return ERR_STANDARD_COMPRESSION_FAILED;
        }
        compressedData = QByteArray((const char*)compressed, compressedSize);
        delete[] compressed;
        return ERR_SUCCESS;
    }
        break;
    case COMPRESSION_ALGORITHM_LZMA:
    {
        UINT32 compressedSize = 0;
        if (LzmaCompress((const UINT8*)data.constData(), data.size(), NULL, &compressedSize) != ERR_BUFFER_TOO_SMALL)
            return ERR_CUSTOMIZED_COMPRESSION_FAILED;
        compressed = new UINT8[compressedSize];
        if (LzmaCompress((const UINT8*)data.constData(), data.size(), compressed, &compressedSize) != ERR_SUCCESS) {
            delete[] compressed;
            return ERR_CUSTOMIZED_COMPRESSION_FAILED;
        }
        compressedData = QByteArray((const char*)compressed, compressedSize);
        delete[] compressed;
        return ERR_SUCCESS;
    }
        break;
    case COMPRESSION_ALGORITHM_IMLZMA:
    {
        UINT32 compressedSize = 0;
        QByteArray header = data.left(sizeof(EFI_COMMON_SECTION_HEADER));
        EFI_COMMON_SECTION_HEADER* sectionHeader = (EFI_COMMON_SECTION_HEADER*)header.constData();
        UINT32 headerSize = sizeOfSectionHeader(sectionHeader);
        header = data.left(headerSize);
        QByteArray newData = data.mid(headerSize);
        if (LzmaCompress((UINT8*)newData.constData(), newData.size(), NULL, &compressedSize) != ERR_BUFFER_TOO_SMALL)
            return ERR_CUSTOMIZED_COMPRESSION_FAILED;
        compressed = new UINT8[compressedSize];
        if (LzmaCompress((UINT8*)newData.constData(), newData.size(), compressed, &compressedSize) != ERR_SUCCESS) {
            delete[] compressed;
            return ERR_CUSTOMIZED_COMPRESSION_FAILED;
        }
        compressedData = header.append(QByteArray((const char*)compressed, compressedSize));
        delete[] compressed;
        return ERR_SUCCESS;
    }
        break;
    default:
        msg(tr("compress: Unknown compression algorithm (%1)").arg(algorithm));
        return ERR_UNKNOWN_COMPRESSION_ALGORITHM;
    }
}

// Construction routines
UINT8 FfsEngine::constructPadFile(const QByteArray &guid, const UINT32 size, const UINT8 revision, const UINT8 erasePolarity, QByteArray & pad)
{
    if (size < sizeof(EFI_FFS_FILE_HEADER) || erasePolarity == ERASE_POLARITY_UNKNOWN)
        return ERR_INVALID_PARAMETER;

    pad = QByteArray(size - guid.size(), erasePolarity == ERASE_POLARITY_TRUE ? '\xFF' : '\x00');
    pad.prepend(guid);
    EFI_FFS_FILE_HEADER* header = (EFI_FFS_FILE_HEADER*)pad.data();
    uint32ToUint24(size, header->Size);
    header->Attributes = 0x00;
    header->Type = EFI_FV_FILETYPE_PAD;
    header->State = EFI_FILE_HEADER_CONSTRUCTION | EFI_FILE_HEADER_VALID | EFI_FILE_DATA_VALID;
    // Invert state bits if erase polarity is true
    if (erasePolarity == ERASE_POLARITY_TRUE)
        header->State = ~header->State;

    // Calculate header checksum
    header->IntegrityCheck.Checksum.Header = 0;
    header->IntegrityCheck.Checksum.File = 0;
    header->IntegrityCheck.Checksum.Header = calculateChecksum8((UINT8*)header, sizeof(EFI_FFS_FILE_HEADER) - 1);

    // Set data checksum
    if (revision == 1)
        header->IntegrityCheck.Checksum.File = FFS_FIXED_CHECKSUM;
    else
        header->IntegrityCheck.Checksum.File = FFS_FIXED_CHECKSUM2;

    return ERR_SUCCESS;
}

UINT8 FfsEngine::reconstructIntelImage(const QModelIndex& index, QByteArray& reconstructed)
{
    if (!index.isValid())
        return ERR_SUCCESS;

    UINT8 result;

    // No action
    if (model->action(index) == Actions::NoAction) {
        reconstructed = model->header(index).append(model->body(index)).append(model->tail(index));
        return ERR_SUCCESS;
    }

    // Other supported actions
    else if (model->action(index) == Actions::Rebuild) {
        reconstructed.clear();
        // First child will always be descriptor for this type of image
        QByteArray descriptor;
        result = reconstructRegion(index.child(0, 0), descriptor);
        if (result)
            return result;
        reconstructed.append(descriptor);

        FLASH_DESCRIPTOR_MAP* descriptorMap = (FLASH_DESCRIPTOR_MAP*)(descriptor.constData() + sizeof(FLASH_DESCRIPTOR_HEADER));
        FLASH_DESCRIPTOR_REGION_SECTION* regionSection = (FLASH_DESCRIPTOR_REGION_SECTION*)calculateAddress8((UINT8*)descriptor.constData(), descriptorMap->RegionBase);
        QByteArray gbe;
        UINT32 gbeBegin = calculateRegionOffset(regionSection->GbeBase);
        UINT32 gbeEnd = gbeBegin + calculateRegionSize(regionSection->GbeBase, regionSection->GbeLimit);
        QByteArray me;
        UINT32 meBegin = calculateRegionOffset(regionSection->MeBase);
        UINT32 meEnd = meBegin + calculateRegionSize(regionSection->MeBase, regionSection->MeLimit);
        QByteArray bios;
        UINT32 biosBegin = calculateRegionOffset(regionSection->BiosBase);
        UINT32 biosEnd = biosBegin + calculateRegionSize(regionSection->BiosBase, regionSection->BiosLimit);
        QByteArray pdr;
        UINT32 pdrBegin = calculateRegionOffset(regionSection->PdrBase);
        UINT32 pdrEnd = pdrBegin + calculateRegionSize(regionSection->PdrBase, regionSection->PdrLimit);

        UINT32 offset = descriptor.size();
        // Reconstruct other regions
        char empty = '\xFF';
        for (int i = 1; i < model->rowCount(index); i++) {
            QByteArray region;
            result = reconstructRegion(index.child(i, 0), region);
            if (result)
                return result;

            switch (model->subtype(index.child(i, 0)))
            {
            case Subtypes::GbeRegion:
                gbe = region;
                if (gbeBegin > offset)
                    reconstructed.append(QByteArray(gbeBegin - offset, empty));
                reconstructed.append(gbe);
                offset = gbeEnd;
                break;
            case Subtypes::MeRegion:
                me = region;
                if (meBegin > offset)
                    reconstructed.append(QByteArray(meBegin - offset, empty));
                reconstructed.append(me);
                offset = meEnd;
                break;
            case Subtypes::BiosRegion:
                bios = region;
                if (biosBegin > offset)
                    reconstructed.append(QByteArray(biosBegin - offset, empty));
                reconstructed.append(bios);
                offset = biosEnd;
                break;
            case Subtypes::PdrRegion:
                pdr = region;
                if (pdrBegin > offset)
                    reconstructed.append(QByteArray(pdrBegin - offset, empty));
                reconstructed.append(pdr);
                offset = pdrEnd;
                break;
            default:
                msg(tr("reconstructIntelImage: unknown region type found"), index);
                return ERR_INVALID_REGION;
            }
        }
        if ((UINT32)model->body(index).size() > offset)
            reconstructed.append(QByteArray((UINT32)model->body(index).size() - offset, empty));

        // Check size of reconstructed image, it must be same
        if (reconstructed.size() > model->body(index).size()) {
            msg(tr("reconstructIntelImage: reconstructed body %1 is bigger then original %2")
                .arg(reconstructed.size(), 8, 16, QChar('0'))
                .arg(model->body(index).size(), 8, 16, QChar('0')), index);
            return ERR_INVALID_PARAMETER;
        }
        else if (reconstructed.size() < model->body(index).size()) {
            msg(tr("reconstructIntelImage: reconstructed body %1 is smaller then original %2")
                .arg(reconstructed.size(), 8, 16, QChar('0'))
                .arg(model->body(index).size(), 8, 16, QChar('0')), index);
            return ERR_INVALID_PARAMETER;
        }

        // Reconstruction successful
        return ERR_SUCCESS;
    }

    // All other actions are not supported
    return ERR_NOT_IMPLEMENTED;
}

UINT8 FfsEngine::reconstructRegion(const QModelIndex& index, QByteArray& reconstructed)
{
    if (!index.isValid())
        return ERR_SUCCESS;

    UINT8 result;

    // No action
    if (model->action(index) == Actions::NoAction) {
        reconstructed = model->header(index).append(model->body(index)).append(model->tail(index));
        return ERR_SUCCESS;
    }
    else if (model->action(index) == Actions::Remove) {
        reconstructed.clear();
        return ERR_SUCCESS;
    }
    else if (model->action(index) == Actions::Rebuild ||
        model->action(index) == Actions::Replace) {
        if (model->rowCount(index)) {
            reconstructed.clear();
            // Reconstruct children
            for (int i = 0; i < model->rowCount(index); i++) {
                QByteArray child;
                result = reconstruct(index.child(i, 0), child);
                if (result)
                    return result;
                reconstructed.append(child);
            }
        }
        // Use stored item body
        else
            reconstructed = model->body(index);

        // Check size of reconstructed region, it must be same
        if (reconstructed.size() > model->body(index).size()) {
            msg(tr("reconstructRegion: reconstructed region (%1) is bigger then original (%2)")
                .arg(reconstructed.size(), 8, 16, QChar('0'))
                .arg(model->body(index).size(), 8, 16, QChar('0')), index);
            return ERR_INVALID_PARAMETER;
        }
        else if (reconstructed.size() < model->body(index).size()) {
            msg(tr("reconstructRegion: reconstructed region (%1) is smaller then original (%2)")
                .arg(reconstructed.size(), 8, 16, QChar('0'))
                .arg(model->body(index).size(), 8, 16, QChar('0')), index);
            return ERR_INVALID_PARAMETER;
        }

        // Reconstruction successful
        reconstructed = model->header(index).append(reconstructed);
        return ERR_SUCCESS;
    }

    // All other actions are not supported
    return ERR_NOT_IMPLEMENTED;
}

UINT8 FfsEngine::reconstructVolume(const QModelIndex & index, QByteArray & reconstructed)
{
    if (!index.isValid())
        return ERR_SUCCESS;

    UINT8 result;

    // No action
    if (model->action(index) == Actions::NoAction) {
        reconstructed = model->header(index).append(model->body(index)).append(model->tail(index));
        return ERR_SUCCESS;
    }
    else if (model->action(index) == Actions::Remove) {
        reconstructed.clear();
        EFI_FIRMWARE_VOLUME_HEADER* volumeHeader = (EFI_FIRMWARE_VOLUME_HEADER*)model->header(index).constData();
        char empty = volumeHeader->Attributes & EFI_FVB_ERASE_POLARITY ? '\xFF' : '\x00';
        reconstructed.fill(empty, model->header(index).size() + model->body(index).size() + model->tail(index).size());
        return ERR_SUCCESS;
    }
    else if (model->action(index) == Actions::Rebuild) {
        //!TODO: add check for weak aligned volume
        QByteArray header = model->header(index);
        EFI_FIRMWARE_VOLUME_HEADER* volumeHeader = (EFI_FIRMWARE_VOLUME_HEADER*)header.data();

        // Recalculate volume header checksum
        volumeHeader->Checksum = 0;
        volumeHeader->Checksum = calculateChecksum16((UINT16*)volumeHeader, volumeHeader->HeaderLength);

        // Get volume size
        UINT32 volumeSize;
        result = getVolumeSize(header, 0, volumeSize);
        if (result)
            return result;

        // Reconstruct volume body
        if (model->rowCount(index)) {
            reconstructed.clear();
            UINT8 polarity = volumeHeader->Attributes & EFI_FVB_ERASE_POLARITY ? ERASE_POLARITY_TRUE : ERASE_POLARITY_FALSE;
            char empty = volumeHeader->Attributes & EFI_FVB_ERASE_POLARITY ? '\xFF' : '\x00';

            // Calculate volume base for volume
            UINT32 volumeBase;
            QByteArray file;
            bool baseFound = false;

            // Search for VTF
            for (int i = 0; i < model->rowCount(index); i++) {
                file = model->header(index.child(i, 0));
                // VTF found
                if (file.left(sizeof(EFI_GUID)) == EFI_FFS_VOLUME_TOP_FILE_GUID) {
                    baseFound = true;
                    volumeBase = (UINT32)(0x100000000 - volumeSize);
                    break;
                }
            }

            // Determine if volume is inside compressed item
            if (!baseFound) {
                // Iterate up to the root, checking for compression type to be other then none
                for (QModelIndex parentIndex = index.parent(); model->type(parentIndex) != Types::Root; parentIndex = parentIndex.parent())
                    if (model->compression(parentIndex) != COMPRESSION_ALGORITHM_NONE) {
                    // No rebase needed for compressed PEI files
                    baseFound = true;
                    volumeBase = 0;
                    break;
                    }
            }

            // Find volume base address using first PEI file in it
            if (!baseFound) {
                // Search for first PEI-file and use it as base source
                UINT32 fileOffset = header.size();
                for (int i = 0; i < model->rowCount(index); i++) {
                    if ((model->subtype(index.child(i, 0)) == EFI_FV_FILETYPE_PEI_CORE ||
                        model->subtype(index.child(i, 0)) == EFI_FV_FILETYPE_PEIM ||
                        model->subtype(index.child(i, 0)) == EFI_FV_FILETYPE_COMBINED_PEIM_DRIVER)){
                        QModelIndex peiFile = index.child(i, 0);
                        UINT32 sectionOffset = sizeof(EFI_FFS_FILE_HEADER);
                        // Search for PE32 or TE section
                        for (int j = 0; j < model->rowCount(peiFile); j++) {
                            if (model->subtype(peiFile.child(j, 0)) == EFI_SECTION_PE32 ||
                                model->subtype(peiFile.child(j, 0)) == EFI_SECTION_TE) {
                                QModelIndex image = peiFile.child(j, 0);
                                // Check for correct action
                                if (model->action(image) == Actions::Remove || model->action(image) == Actions::Insert)
                                    continue;
                                // Calculate relative base address
                                UINT32 relbase = fileOffset + sectionOffset + model->header(image).size();
                                // Calculate offset of image relative to file base
                                UINT32 imagebase;
                                result = getBase(model->body(image), imagebase);
                                if (!result) {
                                    // Calculate volume base
                                    volumeBase = imagebase - relbase;
                                    baseFound = true;
                                    goto out;
                                }
                            }
                            sectionOffset += model->header(peiFile.child(j, 0)).size() + model->body(peiFile.child(j, 0)).size();
                            sectionOffset = ALIGN4(sectionOffset);
                        }
                    }
                    fileOffset += model->header(index.child(i, 0)).size() + model->body(index.child(i, 0)).size() + model->tail(index.child(i, 0)).size();
                    fileOffset = ALIGN8(fileOffset);
                }
            }
        out:
            // Do not set volume base
            if (!baseFound)
                volumeBase = 0;

            // Reconstruct files in volume
            UINT32 offset = 0;
            QByteArray padFileGuid = EFI_FFS_PAD_FILE_GUID;
            QByteArray vtf;
            QModelIndex vtfIndex;
            for (int i = 0; i < model->rowCount(index); i++) {
                // Align to 8 byte boundary
                UINT32 alignment = offset % 8;
                if (alignment) {
                    alignment = 8 - alignment;
                    offset += alignment;
                    reconstructed.append(QByteArray(alignment, empty));
                }

                // Calculate file base
                UINT32 fileBase = volumeBase ? volumeBase + header.size() + offset : 0;

                // Reconstruct file
                result = reconstructFile(index.child(i, 0), volumeHeader->Revision, polarity, fileBase, file);
                if (result)
                    return result;

                // Empty file
                if (file.isEmpty())
                    continue;

                EFI_FFS_FILE_HEADER* fileHeader = (EFI_FFS_FILE_HEADER*)file.data();

                // Pad file
                if (fileHeader->Type == EFI_FV_FILETYPE_PAD) {
                    padFileGuid = file.left(sizeof(EFI_GUID));
                    continue;
                }

                // Volume Top File
                if (file.left(sizeof(EFI_GUID)) == EFI_FFS_VOLUME_TOP_FILE_GUID) {
                    vtf = file;
                    vtfIndex = index.child(i, 0);
                    continue;
                }

                // Normal file
                // Ensure correct alignment
                UINT8 alignmentPower;
                UINT32 alignmentBase;
                alignmentPower = ffsAlignmentTable[(fileHeader->Attributes & FFS_ATTRIB_DATA_ALIGNMENT) >> 3];
                alignment = (UINT32)pow(2.0, alignmentPower);
                alignmentBase = header.size() + offset + sizeof(EFI_FFS_FILE_HEADER);
                if (alignmentBase % alignment) {
                    // File will be unaligned if added as is, so we must add pad file before it
                    // Determine pad file size
                    UINT32 size = alignment - (alignmentBase % alignment);
                    // Required padding is smaller then minimal pad file size
                    while (size < sizeof(EFI_FFS_FILE_HEADER)) {
                        size += alignment;
                    }
                    // Construct pad file
                    QByteArray pad;
                    result = constructPadFile(padFileGuid, size, volumeHeader->Revision, polarity, pad);
                    if (result)
                        return result;
                    // Append constructed pad file to volume body
                    reconstructed.append(pad);
                    offset += size;
                }

                // Append current file to new volume body
                reconstructed.append(file);

                // Change current file offset
                offset += file.size();
            }

            // Insert VTF to it's correct place
            if (!vtf.isEmpty()) {
                // Determine correct VTF offset
                UINT32 vtfOffset = volumeSize - header.size() - vtf.size();

                if (vtfOffset % 8) {
                    msg(tr("reconstructVolume: %1: Wrong size of Volume Top File")
                        .arg(guidToQString(volumeHeader->FileSystemGuid)), index);
                    return ERR_INVALID_FILE;
                }
                // Insert pad file to fill the gap
                if (vtfOffset > offset) {
                    // Determine pad file size
                    UINT32 size = vtfOffset - offset;
                    // Construct pad file
                    QByteArray pad;
                    result = constructPadFile(padFileGuid, size, volumeHeader->Revision, polarity, pad);
                    if (result)
                        return result;
                    // Append constructed pad file to volume body
                    reconstructed.append(pad);
                }
                // No more space left in volume
                else if (vtfOffset < offset) {
                    msg(tr("reconstructVolume: %1: volume has no free space left").arg(guidToQString(volumeHeader->FileSystemGuid)), index);
                    return ERR_INVALID_VOLUME;
                }

                // Calculate VTF base
                UINT32 vtfBase = volumeBase ? volumeBase + vtfOffset : 0;

                // Reconstruct VTF again
                result = reconstructFile(vtfIndex, volumeHeader->Revision, polarity, vtfBase, vtf);
                if (result)
                    return result;

                // Patch PEI core entry point in VTF
                result = patchVtf(vtf);
                if (result)
                    return result;

                // Append VTF
                reconstructed.append(vtf);
            }
            else {
                // Fill the rest of volume space with empty char
                UINT32 volumeBodySize = volumeSize - header.size();
                if (volumeBodySize > (UINT32)reconstructed.size()) {
                    // Fill volume end with empty char
                    reconstructed.append(QByteArray(volumeBodySize - reconstructed.size(), empty));
                }
                else if (volumeBodySize < (UINT32)reconstructed.size()) {
                    // Check if volume can be grown
                    // Root volume can't be grown yet
                    UINT8 parentType = model->type(index.parent());
                    if (parentType != Types::File && parentType != Types::Section) {
                        msg(tr("reconstructVolume: %1: root volume can't be grown").arg(guidToQString(volumeHeader->FileSystemGuid)), index);
                        return ERR_INVALID_VOLUME;
                    }

                    // Grow volume to fit new body
                    UINT32 newSize = header.size() + reconstructed.size();
                    result = growVolume(header, volumeSize, newSize);
                    if (result)
                        return result;

                    // Fill volume end with empty char
                    reconstructed.append(QByteArray(newSize - header.size() - reconstructed.size(), empty));
                    volumeSize = newSize;
                }
            }

            // Check new volume size
            if ((UINT32)(header.size() + reconstructed.size()) > volumeSize)
            {
                msg(tr("reconstructVolume: volume grow failed"), index);
                return ERR_INVALID_VOLUME;
            }
        }
        // Use current volume body
        else
            reconstructed = model->body(index);

        // Reconstruction successful
        reconstructed = header.append(reconstructed);
        return ERR_SUCCESS;
    }

    // All other actions are not supported
    return ERR_NOT_IMPLEMENTED;
}

UINT8 FfsEngine::reconstructFile(const QModelIndex& index, const UINT8 revision, const UINT8 erasePolarity, const UINT32 base, QByteArray& reconstructed)
{
    if (!index.isValid())
        return ERR_SUCCESS;

    UINT8 result;

    // No action
    if (model->action(index) == Actions::NoAction) {
        reconstructed = model->header(index).append(model->body(index)).append(model->tail(index));
        return ERR_SUCCESS;
    }
    else if (model->action(index) == Actions::Remove) {
        reconstructed.clear();
        return ERR_SUCCESS;
    }
    else if (model->action(index) == Actions::Insert ||
        model->action(index) == Actions::Replace ||
        model->action(index) == Actions::Rebuild) {
        QByteArray header = model->header(index);
        EFI_FFS_FILE_HEADER* fileHeader = (EFI_FFS_FILE_HEADER*)header.data();

        // Check erase polarity
        if (erasePolarity == ERASE_POLARITY_UNKNOWN) {
            msg(tr("reconstructFile: %1, unknown erase polarity").arg(guidToQString(fileHeader->Name)), index);
            return ERR_INVALID_PARAMETER;
        }

        // Check file state
        // Invert it first if erase polarity is true
        UINT8 state = fileHeader->State;
        if (erasePolarity == ERASE_POLARITY_TRUE)
            state = ~state;

        // Order of this checks must be preserved
        // Check file to have valid state, or delete it otherwise
        if (state & EFI_FILE_HEADER_INVALID) {
            // File marked to have invalid header and must be deleted
            // Do not add anything to queue
            msg(tr("reconstructFile: %1, file is HEADER_INVALID state, and will be removed from reconstructed image")
                .arg(guidToQString(fileHeader->Name)), index);
            return ERR_SUCCESS;
        }
        else if (state & EFI_FILE_DELETED) {
            // File marked to have been deleted form and must be deleted
            // Do not add anything to queue
            msg(tr("reconstructFile: %1, file is in DELETED state, and will be removed from reconstructed image")
                .arg(guidToQString(fileHeader->Name)), index);
            return ERR_SUCCESS;
        }
        else if (state & EFI_FILE_MARKED_FOR_UPDATE) {
            // File is marked for update, the mark must be removed
            msg(tr("reconstructFile: %1, file MARKED_FOR_UPDATE state cleared")
                .arg(guidToQString(fileHeader->Name)), index);
        }
        else if (state & EFI_FILE_DATA_VALID) {
            // File is in good condition, reconstruct it
        }
        else if (state & EFI_FILE_HEADER_VALID) {
            // Header is valid, but data is not, so file must be deleted
            msg(tr("reconstructFile: %1, file is in HEADER_VALID (but not in DATA_VALID) state, and will be removed from reconstructed image")
                .arg(guidToQString(fileHeader->Name)), index);
            return ERR_SUCCESS;
        }
        else if (state & EFI_FILE_HEADER_CONSTRUCTION) {
            // Header construction not finished, so file must be deleted
            msg(tr("reconstructFile: %1, file is in HEADER_CONSTRUCTION (but not in DATA_VALID) state, and will be removed from reconstructed image")
                .arg(guidToQString(fileHeader->Name)), index);
            return ERR_SUCCESS;
        }

        // Reconstruct file body
        if (model->rowCount(index)) {
            reconstructed.clear();
            // Construct new file body
            // File contains raw data, must be parsed as region
            if (model->subtype(index) == EFI_FV_FILETYPE_ALL || model->subtype(index) == EFI_FV_FILETYPE_RAW) {
                result = reconstructRegion(index, reconstructed);
                if (result)
                    return result;
            }
            // File contains sections
            else {
                UINT32 offset = 0;

                for (int i = 0; i < model->rowCount(index); i++) {
                    // Align to 4 byte boundary
                    UINT8 alignment = offset % 4;
                    if (alignment) {
                        alignment = 4 - alignment;
                        offset += alignment;
                        reconstructed.append(QByteArray(alignment, '\x00'));
                    }

                    // Calculate section base
                    UINT32 sectionBase = base ? base + sizeof(EFI_FFS_FILE_HEADER) + offset : 0;

                    // Reconstruct section
                    QByteArray section;
                    result = reconstructSection(index.child(i, 0), sectionBase, section);
                    if (result)
                        return result;

                    // Check for empty section
                    if (section.isEmpty())
                        continue;

                    // Append current section to new file body
                    reconstructed.append(section);

                    // Change current file offset
                    offset += section.size();
                }
            }

            // Correct file size
            UINT8 tailSize = (fileHeader->Attributes & FFS_ATTRIB_TAIL_PRESENT) ? sizeof(UINT16) : 0;

            uint32ToUint24(sizeof(EFI_FFS_FILE_HEADER) + reconstructed.size() + tailSize, fileHeader->Size);

            // Recalculate header checksum
            fileHeader->IntegrityCheck.Checksum.Header = 0;
            fileHeader->IntegrityCheck.Checksum.File = 0;
            fileHeader->IntegrityCheck.Checksum.Header = calculateChecksum8((UINT8*)fileHeader, sizeof(EFI_FFS_FILE_HEADER) - 1);
        }
        // Use current file body
        else
            reconstructed = model->body(index);

        // Recalculate data checksum, if needed
        if (fileHeader->Attributes & FFS_ATTRIB_CHECKSUM) {
            fileHeader->IntegrityCheck.Checksum.File = calculateChecksum8((UINT8*)reconstructed.constData(), reconstructed.size());
        }
        else if (revision == 1)
            fileHeader->IntegrityCheck.Checksum.File = FFS_FIXED_CHECKSUM;
        else
            fileHeader->IntegrityCheck.Checksum.File = FFS_FIXED_CHECKSUM2;

        // Append tail, if needed
        if (fileHeader->Attributes & FFS_ATTRIB_TAIL_PRESENT) {
            UINT8 ht = ~fileHeader->IntegrityCheck.Checksum.Header;
            UINT8 ft = ~fileHeader->IntegrityCheck.Checksum.File;
            reconstructed.append(ht).append(ft);
        }
        // Set file state
        state = EFI_FILE_DATA_VALID | EFI_FILE_HEADER_VALID | EFI_FILE_HEADER_CONSTRUCTION;
        if (erasePolarity == ERASE_POLARITY_TRUE)
            state = ~state;
        fileHeader->State = state;

        // Reconstruction successful
        reconstructed = header.append(reconstructed);
        return ERR_SUCCESS;
    }

    // All other actions are not supported
    return ERR_NOT_IMPLEMENTED;
}

UINT8 FfsEngine::reconstructSection(const QModelIndex& index, const UINT32 base, QByteArray& reconstructed)
{
    if (!index.isValid())
        return ERR_SUCCESS;

    UINT8 result;

    // No action
    if (model->action(index) == Actions::NoAction) {
        reconstructed = model->header(index).append(model->body(index)).append(model->tail(index));
        return ERR_SUCCESS;
    }
    else if (model->action(index) == Actions::Remove) {
        reconstructed.clear();
        return ERR_SUCCESS;
    }
    else if (model->action(index) == Actions::Insert ||
        model->action(index) == Actions::Replace ||
        model->action(index) == Actions::Rebuild ||
        model->action(index) == Actions::Rebase) {
        QByteArray header = model->header(index);
        EFI_COMMON_SECTION_HEADER* commonHeader = (EFI_COMMON_SECTION_HEADER*)header.data();

        // Reconstruct section with children
        if (model->rowCount(index)) {
            reconstructed.clear();
            // Construct new section body
            UINT32 offset = 0;

            // Reconstruct section body
            for (int i = 0; i < model->rowCount(index); i++) {
                // Align to 4 byte boundary
                UINT8 alignment = offset % 4;
                if (alignment) {
                    alignment = 4 - alignment;
                    offset += alignment;
                    reconstructed.append(QByteArray(alignment, '\x00'));
                }

                // Reconstruct subsections
                QByteArray section;
                result = reconstruct(index.child(i, 0), section);
                if (result)
                    return result;

                // Check for empty queue
                if (section.isEmpty())
                    continue;

                // Append current subsection to new section body
                reconstructed.append(section);

                // Change current file offset
                offset += section.size();
            }

            // Only this 2 sections can have compressed body
            if (model->subtype(index) == EFI_SECTION_COMPRESSION) {
                EFI_COMPRESSION_SECTION* compessionHeader = (EFI_COMPRESSION_SECTION*)header.data();
                // Set new uncompressed size
                compessionHeader->UncompressedLength = reconstructed.size();
                // Compress new section body
                QByteArray compressed;
                result = compress(reconstructed, model->compression(index), compressed);
                if (result)
                    return result;
                // Correct compression type
                if (model->compression(index) == COMPRESSION_ALGORITHM_NONE)
                    compessionHeader->CompressionType = EFI_NOT_COMPRESSED;
                else if (model->compression(index) == COMPRESSION_ALGORITHM_LZMA || model->compression(index) == COMPRESSION_ALGORITHM_IMLZMA)
                    compessionHeader->CompressionType = EFI_CUSTOMIZED_COMPRESSION;
                else if (model->compression(index) == COMPRESSION_ALGORITHM_EFI11 || model->compression(index) == COMPRESSION_ALGORITHM_TIANO)
                    compessionHeader->CompressionType = EFI_STANDARD_COMPRESSION;
                else
                    return ERR_UNKNOWN_COMPRESSION_ALGORITHM;

                // Replace new section body
                reconstructed = compressed;
            }
            else if (model->subtype(index) == EFI_SECTION_GUID_DEFINED) {
                EFI_GUID_DEFINED_SECTION* guidDefinedHeader = (EFI_GUID_DEFINED_SECTION*)header.data();
                // Compress new section body
                QByteArray compressed;
                result = compress(reconstructed, model->compression(index), compressed);
                if (result)
                    return result;
                // Check for authentication status valid attribute
                if (guidDefinedHeader->Attributes & EFI_GUIDED_SECTION_AUTH_STATUS_VALID) {
                    // CRC32 section
                    if (QByteArray((const char*)&guidDefinedHeader->SectionDefinitionGuid, sizeof(EFI_GUID)) == EFI_GUIDED_SECTION_CRC32) {
                        // Calculate CRC32 of section data
                        UINT32 crc = crc32(0, NULL, 0);
                        crc = crc32(crc, (const UINT8*)compressed.constData(), compressed.size());
                        // Store new CRC32
                        *(UINT32*)(header.data() + sizeof(EFI_GUID_DEFINED_SECTION)) = crc;
                    }
                    else {
                        msg(tr("reconstructSection: %1: GUID defined section authentication info can become invalid")
                            .arg(guidToQString(guidDefinedHeader->SectionDefinitionGuid)), index);
                    }
                }
                // Replace new section body
                reconstructed = compressed;
            }
            else if (model->compression(index) != COMPRESSION_ALGORITHM_NONE) {
                msg(tr("reconstructSection: incorrectly required compression for section of type %1")
                    .arg(model->subtype(index)), index);
                return ERR_INVALID_SECTION;
            }

            // Correct section size
            uint32ToUint24(header.size() + reconstructed.size(), commonHeader->Size);
        }
        // Leaf section
        else
            reconstructed = model->body(index);

        // Rebase PE32 or TE image, if needed
        if ((model->subtype(index) == EFI_SECTION_PE32 || model->subtype(index) == EFI_SECTION_TE) &&
            (model->subtype(index.parent()) == EFI_FV_FILETYPE_PEI_CORE ||
            model->subtype(index.parent()) == EFI_FV_FILETYPE_PEIM ||
            model->subtype(index.parent()) == EFI_FV_FILETYPE_COMBINED_PEIM_DRIVER)) {
            if (base) {
                result = rebase(reconstructed, base + header.size());
                if (result) {
                    msg(tr("reconstructSection: executable section rebase failed"), index);
                    return result;
                }

                // Special case of PEI Core rebase
                if (model->subtype(index.parent()) == EFI_FV_FILETYPE_PEI_CORE) {
                    result = getEntryPoint(reconstructed, newPeiCoreEntryPoint);
                    if (result)
                        msg(tr("reconstructSection: can't get entry point of PEI core"), index);
                }
            }
        }

        // Reconstruction successful
        reconstructed = header.append(reconstructed);

        return ERR_SUCCESS;
    }

    // All other actions are not supported
    return ERR_NOT_IMPLEMENTED;
}

UINT8 FfsEngine::reconstruct(const QModelIndex &index, QByteArray& reconstructed)
{
    if (!index.isValid())
        return ERR_SUCCESS;

    UINT8 result;

    switch (model->type(index)) {
    case Types::Image:
        if (model->subtype(index) == Subtypes::IntelImage) {
            result = reconstructIntelImage(index, reconstructed);
            if (result)
                return result;
        }
        else {
            //Other images types can be reconstructed like regions
            result = reconstructRegion(index, reconstructed);
            if (result)
                return result;
        }
        break;

    case Types::Capsule:
        if (model->subtype(index) == Subtypes::AptioCapsule)
            msg(tr("reconstruct: Aptio capsule checksum and signature can now become invalid"), index);
        // Capsules can be reconstructed like regions
        result = reconstructRegion(index, reconstructed);
        if (result)
            return result;
        break;

    case Types::Region:
        result = reconstructRegion(index, reconstructed);
        if (result)
            return result;
        break;

    case Types::Padding:
        // No reconstruction needed
        reconstructed = model->header(index).append(model->body(index)).append(model->tail(index));
        return ERR_SUCCESS;
        break;

    case Types::Volume:
        result = reconstructVolume(index, reconstructed);
        if (result)
            return result;
        break;

    case Types::File: //Must not be called that way
        msg(tr("reconstruct: call of generic function is not supported for files").arg(model->type(index)), index);
        return ERR_GENERIC_CALL_NOT_SUPPORTED;
        break;

    case Types::Section:
        result = reconstructSection(index, 0, reconstructed);
        if (result)
            return result;
        break;
    default:
        msg(tr("reconstruct: unknown item type (%1)").arg(model->type(index)), index);
        return ERR_UNKNOWN_ITEM_TYPE;
    }

    return ERR_SUCCESS;
}

UINT8 FfsEngine::growVolume(QByteArray & header, const UINT32 size, UINT32 & newSize)
{
    // Adjust new size to be representable by current FvBlockMap
    EFI_FIRMWARE_VOLUME_HEADER* volumeHeader = (EFI_FIRMWARE_VOLUME_HEADER*)header.data();
    EFI_FV_BLOCK_MAP_ENTRY* blockMap = (EFI_FV_BLOCK_MAP_ENTRY*)(header.data() + sizeof(EFI_FIRMWARE_VOLUME_HEADER));

    // Get block map size
    UINT32 blockMapSize = volumeHeader->HeaderLength - sizeof(EFI_FIRMWARE_VOLUME_HEADER);
    if (blockMapSize % sizeof(EFI_FV_BLOCK_MAP_ENTRY))
        return ERR_INVALID_VOLUME;
    UINT32 blockMapCount = blockMapSize / sizeof(EFI_FV_BLOCK_MAP_ENTRY);

    // Check blockMap validity
    if (blockMap[blockMapCount - 1].NumBlocks != 0 || blockMap[blockMapCount - 1].Length != 0)
        return ERR_INVALID_VOLUME;

    // Case of complex blockMap
    if (blockMapCount > 2)
        return ERR_COMPLEX_BLOCK_MAP;

    // Calculate new size
    if (newSize <= size)
        return ERR_INVALID_PARAMETER;

    newSize += blockMap[0].Length - newSize % blockMap[0].Length;

    // Recalculate number of blocks
    blockMap[0].NumBlocks = newSize / blockMap[0].Length;

    // Set new volume size
    volumeHeader->FvLength = 0;
    for (UINT8 i = 0; i < blockMapCount; i++) {
        volumeHeader->FvLength += blockMap[i].NumBlocks * blockMap[i].Length;
    }

    // Recalculate volume header checksum
    volumeHeader->Checksum = 0;
    volumeHeader->Checksum = calculateChecksum16((UINT16*)volumeHeader, volumeHeader->HeaderLength);

    return ERR_SUCCESS;
}

UINT8 FfsEngine::reconstructImageFile(QByteArray & reconstructed)
{
    return reconstruct(model->index(0, 0), reconstructed);
}

// Search routines
UINT8 FfsEngine::findHexPattern(const QModelIndex & index, const QByteArray & hexPattern, const UINT8 mode)
{
    if (hexPattern.isEmpty())
        return ERR_INVALID_PARAMETER;

    if (!index.isValid())
        return ERR_SUCCESS;

    bool hasChildren = (model->rowCount(index) > 0);
    for (int i = 0; i < model->rowCount(index); i++) {
        findHexPattern(index.child(i, index.column()), hexPattern, mode);
    }

    QByteArray data;
    if (hasChildren) {
        if (mode != SEARCH_MODE_BODY)
            data = model->header(index);
    }
    else {
        if (mode == SEARCH_MODE_HEADER)
            data.append(model->header(index)).append(model->tail(index));
        else if (mode == SEARCH_MODE_BODY)
            data.append(model->body(index));
        else
            data.append(model->header(index)).append(model->body(index)).append(model->tail(index));
    }

    // Check for "all substrings" pattern
    if (hexPattern.count('.') == hexPattern.length())
        return ERR_SUCCESS;

    QString hexBody = QString(data.toHex());
    QRegExp regexp = QRegExp(QString(hexPattern), Qt::CaseInsensitive);
    INT32 offset = regexp.indexIn(hexBody);
    while (offset >= 0) {
        if (offset % 2 == 0) {
            msg(tr("Hex pattern \"%1\" found as \"%2\" in %3 at %4-offset %5")
                .arg(QString(hexPattern))
                .arg(hexBody.mid(offset, hexPattern.length()))
                .arg(model->nameString(index))
                .arg(mode == SEARCH_MODE_BODY ? tr("body") : tr("header"))
                .arg(offset / 2, 8, 16, QChar('0')),
                index);
        }
        offset = regexp.indexIn(hexBody, offset + 1);
    }

    return ERR_SUCCESS;
}

UINT8 FfsEngine::findGuidPattern(const QModelIndex & index, const QByteArray & guidPattern, const UINT8 mode)
{
    if (guidPattern.isEmpty())
        return ERR_INVALID_PARAMETER;

    if (!index.isValid())
        return ERR_SUCCESS;

    bool hasChildren = (model->rowCount(index) > 0);
    for (int i = 0; i < model->rowCount(index); i++) {
        findGuidPattern(index.child(i, index.column()), guidPattern, mode);
    }

    QByteArray data;
    if (hasChildren) {
        if (mode != SEARCH_MODE_BODY)
            data = model->header(index);
    }
    else {
        if (mode == SEARCH_MODE_HEADER)
            data.append(model->header(index)).append(model->tail(index));
        else if (mode == SEARCH_MODE_BODY)
            data.append(model->body(index));
        else
            data.append(model->header(index)).append(model->body(index)).append(model->tail(index));
    }

    QString hexBody = QString(data.toHex());
    QList<QByteArray> list = guidPattern.split('-');
    if (list.count() != 5)
        return ERR_INVALID_PARAMETER;

    QByteArray hexPattern;
    // Reverse first GUID block
    hexPattern.append(list.at(0).mid(6, 2));
    hexPattern.append(list.at(0).mid(4, 2));
    hexPattern.append(list.at(0).mid(2, 2));
    hexPattern.append(list.at(0).mid(0, 2));
    // Reverse second GUID block
    hexPattern.append(list.at(1).mid(2, 2));
    hexPattern.append(list.at(1).mid(0, 2));
    // Reverse third GUID block
    hexPattern.append(list.at(2).mid(2, 2));
    hexPattern.append(list.at(2).mid(0, 2));
    // Append fourth and fifth GUID blocks as is
    hexPattern.append(list.at(3)).append(list.at(4));

    // Check for "all substrings" pattern
    if (hexPattern.count('.') == hexPattern.length())
        return ERR_SUCCESS;

    QRegExp regexp = QRegExp(QString(hexPattern), Qt::CaseInsensitive);
    INT32 offset = regexp.indexIn(hexBody);
    while (offset >= 0) {
        if (offset % 2 == 0) {
            msg(tr("GUID pattern \"%1\" found as \"%2\" in %3 at %4-offset %5")
                .arg(QString(guidPattern))
                .arg(hexBody.mid(offset, hexPattern.length()))
                .arg(model->nameString(index))
                .arg(mode == SEARCH_MODE_BODY ? tr("body") : tr("header"))
                .arg(offset / 2, 8, 16, QChar('0')),
                index);
        }
        offset = regexp.indexIn(hexBody, offset + 1);
    }

    return ERR_SUCCESS;
}

UINT8 FfsEngine::findTextPattern(const QModelIndex & index, const QString & pattern, const bool unicode, const Qt::CaseSensitivity caseSensitive)
{
    if (pattern.isEmpty())
        return ERR_INVALID_PARAMETER;

    if (!index.isValid())
        return ERR_SUCCESS;

    bool hasChildren = (model->rowCount(index) > 0);
    for (int i = 0; i < model->rowCount(index); i++) {
        findTextPattern(index.child(i, index.column()), pattern, unicode, caseSensitive);
    }

    if (hasChildren)
        return ERR_SUCCESS;

    QString data;
    if (unicode)
        data = QString::fromUtf16((const ushort*)model->body(index).data(), model->body(index).length() / 2);
    else
        data = QString::fromLatin1((const char*)model->body(index).data(), model->body(index).length());

    int offset = -1;
    while ((offset = data.indexOf(pattern, offset + 1, caseSensitive)) >= 0) {
        msg(tr("%1 text \"%2\" found in %3 at offset %4")
            .arg(unicode ? "Unicode" : "ASCII")
            .arg(pattern)
            .arg(model->nameString(index))
            .arg(unicode ? offset * 2 : offset, 8, 16, QChar('0')),
            index);
    }

    return ERR_SUCCESS;
}

UINT8 FfsEngine::rebase(QByteArray &executable, const UINT32 base)
{
    UINT32 delta;       // Difference between old and new base addresses
    UINT32 relocOffset; // Offset of relocation region
    UINT32 relocSize;   // Size of relocation region
    UINT32 teFixup = 0; // Bytes removed form PE header for TE images

    // Copy input data to local storage
    QByteArray file = executable;

    // Populate DOS header
    EFI_IMAGE_DOS_HEADER* dosHeader = (EFI_IMAGE_DOS_HEADER*)file.data();

    // Check signature
    if (dosHeader->e_magic == EFI_IMAGE_DOS_SIGNATURE){
        UINT32 offset = dosHeader->e_lfanew;
        EFI_IMAGE_PE_HEADER* peHeader = (EFI_IMAGE_PE_HEADER*)(file.data() + offset);
        if (peHeader->Signature != EFI_IMAGE_PE_SIGNATURE)
            return ERR_UNKNOWN_IMAGE_TYPE;
        offset += sizeof(EFI_IMAGE_PE_HEADER);
        // Skip file header
        offset += sizeof(EFI_IMAGE_FILE_HEADER);
        // Check optional header magic
        UINT16 magic = *(UINT16*)(file.data() + offset);
        if (magic == EFI_IMAGE_PE_OPTIONAL_HDR32_MAGIC) {
            EFI_IMAGE_OPTIONAL_HEADER32* optHeader = (EFI_IMAGE_OPTIONAL_HEADER32*)(file.data() + offset);
            delta = base - optHeader->ImageBase;
            if (!delta)
                // No need to rebase
                return ERR_SUCCESS;
            relocOffset = optHeader->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress;
            relocSize = optHeader->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;
            // Set new base
            optHeader->ImageBase = base;
        }
        else if (magic == EFI_IMAGE_PE_OPTIONAL_HDR64_MAGIC) {
            EFI_IMAGE_OPTIONAL_HEADER64* optHeader = (EFI_IMAGE_OPTIONAL_HEADER64*)(file.data() + offset);
            delta = base - optHeader->ImageBase;
            if (!delta)
                // No need to rebase
                return ERR_SUCCESS;
            relocOffset = optHeader->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress;
            relocSize = optHeader->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;
            // Set new base
            optHeader->ImageBase = base;
        }
        else
            return ERR_UNKNOWN_PE_OPTIONAL_HEADER_TYPE;
    }
    else if (dosHeader->e_magic == EFI_IMAGE_TE_SIGNATURE){
        // Populate TE header
        EFI_IMAGE_TE_HEADER* teHeader = (EFI_IMAGE_TE_HEADER*)file.data();
        delta = base - teHeader->ImageBase;
        if (!delta)
            // No need to rebase
            return ERR_SUCCESS;
        relocOffset = teHeader->DataDirectory[EFI_IMAGE_TE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress;
        teFixup = teHeader->StrippedSize - sizeof(EFI_IMAGE_TE_HEADER);
        relocSize = teHeader->DataDirectory[EFI_IMAGE_TE_DIRECTORY_ENTRY_BASERELOC].Size;
        // Set new base
        teHeader->ImageBase = base;
    }
    else
        return ERR_UNKNOWN_IMAGE_TYPE;

    // No relocations
    if (relocOffset == 0) {
        // No need to fix relocations
        executable = file;
        return ERR_SUCCESS;
    }

    EFI_IMAGE_BASE_RELOCATION   *RelocBase;
    EFI_IMAGE_BASE_RELOCATION   *RelocBaseEnd;
    UINT16                      *Reloc;
    UINT16                      *RelocEnd;
    UINT16                      *F16;
    UINT32                      *F32;
    UINT64                      *F64;

    // Run the whole relocation block
    RelocBase = (EFI_IMAGE_BASE_RELOCATION*)(file.data() + relocOffset - teFixup);
    RelocBaseEnd = (EFI_IMAGE_BASE_RELOCATION*)(file.data() + relocOffset - teFixup + relocSize);

    while (RelocBase < RelocBaseEnd) {
        Reloc = (UINT16*)((UINT8*)RelocBase + sizeof(EFI_IMAGE_BASE_RELOCATION));
        RelocEnd = (UINT16*)((UINT8*)RelocBase + RelocBase->SizeOfBlock);

        // Run this relocation record
        while (Reloc < RelocEnd) {
            UINT8* data = (UINT8*)(file.data() + RelocBase->VirtualAddress - teFixup + (*Reloc & 0x0FFF));
            switch ((*Reloc) >> 12) {
            case EFI_IMAGE_REL_BASED_ABSOLUTE:
                // Do nothing
                break;

            case EFI_IMAGE_REL_BASED_HIGH:
                // Add second 16 bits of delta
                F16 = (UINT16*)data;
                *F16 = (UINT16)(*F16 + (UINT16)(((UINT32)delta) >> 16));
                break;

            case EFI_IMAGE_REL_BASED_LOW:
                // Add first 16 bits of delta
                F16 = (UINT16*)data;
                *F16 = (UINT16)(*F16 + (UINT16)delta);
                break;

            case EFI_IMAGE_REL_BASED_HIGHLOW:
                // Add first 32 bits of delta
                F32 = (UINT32*)data;
                *F32 = *F32 + (UINT32)delta;
                break;

            case EFI_IMAGE_REL_BASED_DIR64:
                // Add all 64 bits of delta
                F64 = (UINT64*)data;
                *F64 = *F64 + (UINT64)delta;
                break;

            default:
                return ERR_UNKNOWN_RELOCATION_TYPE;
            }

            // Next relocation record
            Reloc += 1;
        }

        // Next relocation block
        RelocBase = (EFI_IMAGE_BASE_RELOCATION*)RelocEnd;
    }

    executable = file;
    return ERR_SUCCESS;
}

UINT8 FfsEngine::patchVtf(QByteArray &vtf)
{
    if (!oldPeiCoreEntryPoint) {
        msg(tr("PEI Core entry point can't be determined. VTF can't be patched."));
        return ERR_PEI_CORE_ENTRY_POINT_NOT_FOUND;
    }

    if (!newPeiCoreEntryPoint || oldPeiCoreEntryPoint == newPeiCoreEntryPoint)
        // No need to patch anything
        return ERR_SUCCESS;

    // Replace last occurrence of oldPeiCoreEntryPoint with newPeiCoreEntryPoint
    QByteArray old((char*)&oldPeiCoreEntryPoint, sizeof(oldPeiCoreEntryPoint));
    int i = vtf.lastIndexOf(old);
    if (i == -1) {
        msg(tr("PEI Core entry point can't be found in VTF. VTF not patched."));
        return ERR_SUCCESS;
    }
    UINT32* data = (UINT32*)(vtf.data() + i);
    *data = newPeiCoreEntryPoint;

    return ERR_SUCCESS;
}

UINT8 FfsEngine::getEntryPoint(const QByteArray &file, UINT32& entryPoint)
{
    if (file.isEmpty())
        return ERR_INVALID_FILE;

    // Populate DOS header
    EFI_IMAGE_DOS_HEADER* dosHeader = (EFI_IMAGE_DOS_HEADER*)file.data();

    // Check signature
    if (dosHeader->e_magic == EFI_IMAGE_DOS_SIGNATURE){
        UINT32 offset = dosHeader->e_lfanew;
        EFI_IMAGE_PE_HEADER* peHeader = (EFI_IMAGE_PE_HEADER*)(file.data() + offset);
        if (peHeader->Signature != EFI_IMAGE_PE_SIGNATURE)
            return ERR_UNKNOWN_IMAGE_TYPE;
        offset += sizeof(EFI_IMAGE_PE_HEADER);

        // Skip file header
        offset += sizeof(EFI_IMAGE_FILE_HEADER);

        // Check optional header magic
        UINT16 magic = *(UINT16*)(file.data() + offset);
        if (magic == EFI_IMAGE_PE_OPTIONAL_HDR32_MAGIC) {
            EFI_IMAGE_OPTIONAL_HEADER32* optHeader = (EFI_IMAGE_OPTIONAL_HEADER32*)(file.data() + offset);
            entryPoint = optHeader->ImageBase + optHeader->AddressOfEntryPoint;
        }
        else if (magic == EFI_IMAGE_PE_OPTIONAL_HDR64_MAGIC) {
            EFI_IMAGE_OPTIONAL_HEADER64* optHeader = (EFI_IMAGE_OPTIONAL_HEADER64*)(file.data() + offset);
            entryPoint = optHeader->ImageBase + optHeader->AddressOfEntryPoint;
        }
        else
            return ERR_UNKNOWN_PE_OPTIONAL_HEADER_TYPE;
    }
    else if (dosHeader->e_magic == EFI_IMAGE_TE_SIGNATURE){
        // Populate TE header
        EFI_IMAGE_TE_HEADER* teHeader = (EFI_IMAGE_TE_HEADER*)file.data();
        UINT32 teFixup = teHeader->StrippedSize - sizeof(EFI_IMAGE_TE_HEADER);
        entryPoint = teHeader->ImageBase + teHeader->AddressOfEntryPoint - teFixup;
    }
    return ERR_SUCCESS;
}

UINT8 FfsEngine::getBase(const QByteArray& file, UINT32& base)
{
    if (file.isEmpty())
        return ERR_INVALID_FILE;

    // Populate DOS header
    EFI_IMAGE_DOS_HEADER* dosHeader = (EFI_IMAGE_DOS_HEADER*)file.data();

    // Check signature
    if (dosHeader->e_magic == EFI_IMAGE_DOS_SIGNATURE){
        UINT32 offset = dosHeader->e_lfanew;
        EFI_IMAGE_PE_HEADER* peHeader = (EFI_IMAGE_PE_HEADER*)(file.data() + offset);
        if (peHeader->Signature != EFI_IMAGE_PE_SIGNATURE)
            return ERR_UNKNOWN_IMAGE_TYPE;
        offset += sizeof(EFI_IMAGE_PE_HEADER);

        // Skip file header
        offset += sizeof(EFI_IMAGE_FILE_HEADER);

        // Check optional header magic
        UINT16 magic = *(UINT16*)(file.data() + offset);
        if (magic == EFI_IMAGE_PE_OPTIONAL_HDR32_MAGIC) {
            EFI_IMAGE_OPTIONAL_HEADER32* optHeader = (EFI_IMAGE_OPTIONAL_HEADER32*)(file.data() + offset);
            base = optHeader->ImageBase;
        }
        else if (magic == EFI_IMAGE_PE_OPTIONAL_HDR64_MAGIC) {
            EFI_IMAGE_OPTIONAL_HEADER64* optHeader = (EFI_IMAGE_OPTIONAL_HEADER64*)(file.data() + offset);
            base = optHeader->ImageBase;
        }
        else
            return ERR_UNKNOWN_PE_OPTIONAL_HEADER_TYPE;
    }
    else if (dosHeader->e_magic == EFI_IMAGE_TE_SIGNATURE){
        // Populate TE header
        EFI_IMAGE_TE_HEADER* teHeader = (EFI_IMAGE_TE_HEADER*)file.data();
        base = teHeader->ImageBase;
    }

    return ERR_SUCCESS;
}

UINT32 FfsEngine::crc32(UINT32 initial, const UINT8* buffer, UINT32 length)
{
    static const UINT32 crcTable[256] = {
        0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F, 0xE963A535,
        0x9E6495A3, 0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988, 0x09B64C2B, 0x7EB17CBD,
        0xE7B82D07, 0x90BF1D91, 0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE, 0x1ADAD47D,
        0x6DDDE4EB, 0xF4D4B551, 0x83D385C7, 0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC,
        0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5, 0x3B6E20C8, 0x4C69105E, 0xD56041E4,
        0xA2677172, 0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B, 0x35B5A8FA, 0x42B2986C,
        0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59, 0x26D930AC,
        0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
        0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924, 0x2F6F7C87, 0x58684C11, 0xC1611DAB,
        0xB6662D3D, 0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F,
        0x9FBFE4A5, 0xE8B8D433, 0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB,
        0x086D3D2D, 0x91646C97, 0xE6635C01, 0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
        0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457, 0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA,
        0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65, 0x4DB26158, 0x3AB551CE,
        0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB, 0x4369E96A,
        0x346ED9FC, 0xAD678846, 0xDA60B8D0, 0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
        0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409,
        0xCE61E49F, 0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81,
        0xB7BD5C3B, 0xC0BA6CAD, 0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A, 0xEAD54739,
        0x9DD277AF, 0x04DB2615, 0x73DC1683, 0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8,
        0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1, 0xF00F9344, 0x8708A3D2, 0x1E01F268,
        0x6906C2FE, 0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7, 0xFED41B76, 0x89D32BE0,
        0x10DA7A5A, 0x67DD4ACC, 0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5, 0xD6D6A3E8,
        0xA1D1937E, 0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
        0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55, 0x316E8EEF,
        0x4669BE79, 0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236, 0xCC0C7795, 0xBB0B4703,
        0x220216B9, 0x5505262F, 0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7,
        0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D, 0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A,
        0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713, 0x95BF4A82, 0xE2B87A14, 0x7BB12BAE,
        0x0CB61B38, 0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21, 0x86D3D2D4, 0xF1D4E242,
        0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777, 0x88085AE6,
        0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
        0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2, 0xA7672661, 0xD06016F7, 0x4969474D,
        0x3E6E77DB, 0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5,
        0x47B2CF7F, 0x30B5FFE9, 0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605,
        0xCDD70693, 0x54DE5729, 0x23D967BF, 0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
        0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D };
    UINT32 crc32;
    UINT32 i;

    // Accumulate crc32 for buffer
    crc32 = initial ^ 0xFFFFFFFF;
    for (i = 0; i < length; i++) {
        crc32 = (crc32 >> 8) ^ crcTable[(crc32 ^ buffer[i]) & 0xFF];
    }

    return(crc32 ^ 0xFFFFFFFF);
}

UINT8 FfsEngine::dump(const QModelIndex & index, const QString path)
{
    if (!index.isValid())
        return ERR_INVALID_PARAMETER;

    QDir dir;
    if (dir.cd(path))
        return ERR_DIR_ALREADY_EXIST;

    if (!dir.mkpath(path))
        return ERR_DIR_CREATE;

    QFile file;
    if (!model->header(index).isEmpty()) {
        file.setFileName(tr("%1/header.bin").arg(path));
        if (!file.open(QFile::WriteOnly))
            return ERR_FILE_OPEN;
        file.write(model->header(index));
        file.close();
    }

    if (!model->body(index).isEmpty()) {
        file.setFileName(tr("%1/body.bin").arg(path));
        if (!file.open(QFile::WriteOnly))
            return ERR_FILE_OPEN;
        file.write(model->body(index));
        file.close();
    }

    QString info = tr("Type: %1\nSubtype: %2\n%3%4")
        .arg(model->typeString(index))
        .arg(model->subtypeString(index))
        .arg(model->textString(index).isEmpty() ? "" : tr("Text: %1\n").arg(model->textString(index)))
        .arg(model->info(index));
    file.setFileName(tr("%1/info.txt").arg(path));
    if (!file.open(QFile::Text | QFile::WriteOnly))
        return ERR_FILE_OPEN;
    file.write(info.toLatin1());
    file.close();

    UINT8 result;
    for (int i = 0; i < model->rowCount(index); i++) {
        QModelIndex childIndex = index.child(i, 0);
        QString childPath = tr("%1/%2 %3").arg(path).arg(i).arg(model->textString(childIndex).isEmpty() ? model->nameString(childIndex) : model->textString(childIndex));
        result = dump(childIndex, childPath);
        if (result)
            return result;
    }

    return ERR_SUCCESS;
}

UINT8 FfsEngine::patch(const QModelIndex & index, const QVector<PatchData> & patches)
{
    if (!index.isValid() || patches.isEmpty() || model->rowCount(index))
        return ERR_INVALID_PARAMETER;

    // Skip removed items
    if (model->action(index) == Actions::Remove)
        return ERR_NOTHING_TO_PATCH;

    UINT8 result;

    // Apply patches to item's body
    QByteArray body = model->body(index);
    PatchData current;
    Q_FOREACH(current, patches)
    {
        if (current.type == PATCH_TYPE_OFFSET) {
            result = patchViaOffset(body, current.offset, current.hexReplacePattern);
            if (result)
                return result;
        }
        else if (current.type == PATCH_TYPE_PATTERN) {
            result = patchViaPattern(body, current.hexFindPattern, current.hexReplacePattern);
            if (result)
                return result;
        }
        else
            return ERR_UNKNOWN_PATCH_TYPE;
    }

    if (body != model->body(index)) {
        QByteArray patched = model->header(index);
        patched.append(body);
        return replace(index, patched, REPLACE_MODE_AS_IS);
    }

    return ERR_NOTHING_TO_PATCH;
}

UINT8 FfsEngine::patchViaOffset(QByteArray & data, const UINT32 offset, const QByteArray & hexReplacePattern)
{
    QByteArray body = data;

    // Skip patterns with odd length
    if (hexReplacePattern.length() % 2 > 0)
        return ERR_INVALID_PARAMETER;

    // Check offset bounds
    if (offset > (UINT32)(body.length() - hexReplacePattern.length() / 2))
        return ERR_PATCH_OFFSET_OUT_OF_BOUNDS;

    // Parse replace pattern
    QByteArray replacePattern;
    bool converted;
    for (int i = 0; i < hexReplacePattern.length() / 2; i++) {
        QByteArray hex = hexReplacePattern.mid(2 * i, 2);
        UINT8 value = 0;

        if (!hex.contains('.')) { // Normal byte pattern
            value = (UINT8)hex.toUShort(&converted, 16);
            if (!converted)
                return ERR_INVALID_SYMBOL;
        }
        else { // Placeholder byte pattern
            if (hex[0] == '.' && hex[1] == '.') { // Full byte placeholder
                value = body.at(offset + i);
            }
            else if (hex[0] == '.') {// Upper byte part placeholder
                hex[0] = '0';
                value = (UINT8)(body.at(offset + i) & 0xF0);
                value += (UINT8)hex.toUShort(&converted, 16);
                if (!converted)
                    return ERR_INVALID_SYMBOL;
            }
            else if (hex[1] == '.') { // Lower byte part placeholder
                hex[1] = '0';
                value = (UINT8)(body.at(offset + i) & 0x0F);
                value += (UINT8)hex.toUShort(&converted, 16);
                if (!converted)
                    return ERR_INVALID_SYMBOL;
            }
            else
                return ERR_INVALID_SYMBOL;
        }

        // Append calculated value to real pattern
        replacePattern.append(value);
    }

    body.replace(offset, replacePattern.length(), replacePattern);
    msg(tr("patch: replaced %1 bytes at offset 0x%2 %3 -> %4")
        .arg(replacePattern.length())
        .arg(offset, 8, 16, QChar('0'))
        .arg(QString(data.mid(offset, replacePattern.length()).toHex()))
        .arg(QString(replacePattern.toHex())));
    data = body;
    return ERR_SUCCESS;
}

UINT8 FfsEngine::patchViaPattern(QByteArray & data, const QByteArray & hexFindPattern, const QByteArray & hexReplacePattern)
{
    QByteArray body = data;

    // Skip patterns with odd length
    if (hexFindPattern.length() % 2 > 0 || hexReplacePattern.length() % 2 > 0)
        return ERR_INVALID_PARAMETER;

    // Convert file body to hex;
    QString hexBody = QString(body.toHex());
    QRegExp regexp = QRegExp(QString(hexFindPattern), Qt::CaseInsensitive);
    INT32 offset = regexp.indexIn(hexBody);
    while (offset >= 0) {
        if (offset % 2 == 0) {
            UINT8 result = patchViaOffset(body, offset / 2, hexReplacePattern);
            if (result)
                return result;
        }
        offset = regexp.indexIn(hexBody, offset + 1);
    }

    data = body;
    return ERR_SUCCESS;
}