/* ffsengine.cpp

Copyright (c) 2013, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHWARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*/

#include <math.h>

#include "ffsengine.h"
#include "treeitem.h"
#include "treemodel.h"
#include "descriptor.h"
#include "ffs.h"
#include "gbe.h"
#include "me.h"
#include "Tiano/EfiTianoCompress.h"
#include "Tiano/EfiTianoDecompress.h"
#include "LZMA/LzmaCompress.h"
#include "LZMA/LzmaDecompress.h"

FfsEngine::FfsEngine(QObject *parent)
    : QObject(parent)
{
    model = new TreeModel();
}

FfsEngine::~FfsEngine(void)
{
    delete model;
}

TreeModel* FfsEngine::treeModel() const
{
    return model;
}

void FfsEngine::msg(const QString & message, const QModelIndex index)
{
    messageItems.enqueue(MessageListItem(message, NULL, 0, index));
}

QQueue<MessageListItem> FfsEngine::messages() const
{
    return messageItems;
}

void FfsEngine::clearMessages()
{
    messageItems.clear();
}

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
UINT8 FfsEngine::parseInputFile(const QByteArray & buffer)
{
    UINT32 capsuleHeaderSize = 0;
    FLASH_DESCRIPTOR_HEADER* descriptorHeader = NULL;
    QModelIndex index;
    QByteArray flashImage;

    // Check buffer size to be more then or equal to sizeof(EFI_CAPSULE_HEADER)
    if ((UINT32) buffer.size() <= sizeof(EFI_CAPSULE_HEADER))
    {
        msg(tr("parseInputFile: Input file is smaller then minimum size of %1 bytes").arg(sizeof(EFI_CAPSULE_HEADER)));
        return ERR_INVALID_PARAMETER;
    }

    // Check buffer for being normal EFI capsule header
    if (buffer.startsWith(EFI_CAPSULE_GUID)) {
        // Get info
        EFI_CAPSULE_HEADER* capsuleHeader = (EFI_CAPSULE_HEADER*) buffer.constData();
        capsuleHeaderSize = capsuleHeader->HeaderSize;
        QByteArray header = buffer.left(capsuleHeaderSize);
        QByteArray body   = buffer.right(buffer.size() - capsuleHeaderSize);
        QString name = tr("UEFI capsule");
        QString info = tr("Header size: %1\nFlags: %2\nImage size: %3")
            .arg(capsuleHeader->HeaderSize, 8, 16, QChar('0'))
            .arg(capsuleHeader->Flags, 8, 16, QChar('0'))
            .arg(capsuleHeader->CapsuleImageSize, 8, 16, QChar('0'));
        // Add tree item
        index = model->addItem(Capsule, UefiCapsule, COMPRESSION_ALGORITHM_NONE, name, "", info, header, body);
    }

    // Check buffer for being extended Aptio capsule header
    else if (buffer.startsWith(APTIO_CAPSULE_GUID)) {
        // Get info
        APTIO_CAPSULE_HEADER* aptioCapsuleHeader = (APTIO_CAPSULE_HEADER*) buffer.constData();
        capsuleHeaderSize = aptioCapsuleHeader->RomImageOffset;
        QByteArray header = buffer.left(capsuleHeaderSize);
        QByteArray body   = buffer.right(buffer.size() - capsuleHeaderSize);
        QString name = tr("AMI Aptio capsule");
        QString info = tr("Header size: %1\nFlags: %2\nImage size: %3")
            .arg(aptioCapsuleHeader->RomImageOffset, 4, 16, QChar('0'))
            .arg(aptioCapsuleHeader->CapsuleHeader.Flags, 8, 16, QChar('0'))
            .arg(aptioCapsuleHeader->CapsuleHeader.CapsuleImageSize - aptioCapsuleHeader->RomImageOffset, 8, 16, QChar('0'));
        //!TODO: more info about Aptio capsule
        // Add tree item
        index = model->addItem(Capsule, AptioCapsule, COMPRESSION_ALGORITHM_NONE, name, "", info, header, body);
    }

    // Skip capsule header to have flash chip image
    flashImage = buffer.right(buffer.size() - capsuleHeaderSize);

    // Check for Intel flash descriptor presence
    descriptorHeader = (FLASH_DESCRIPTOR_HEADER*) flashImage.constData();

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
    index = model->addItem(Image, BiosImage, COMPRESSION_ALGORITHM_NONE, name, "", info, QByteArray(), flashImage, QByteArray(), index);
    return parseBios(flashImage, index);
}

UINT8 FfsEngine::parseIntelImage(const QByteArray & flashImage, QModelIndex & index, const QModelIndex & parent)
{
    FLASH_DESCRIPTOR_MAP*               descriptorMap;
    FLASH_DESCRIPTOR_REGION_SECTION*    regionSection;
    //FLASH_DESCRIPTOR_COMPONENT_SECTION* componentSection;

    // Store the beginning of descriptor as descriptor base address
    UINT8* descriptor      = (UINT8*) flashImage.constData();
    UINT32 descriptorBegin = 0;
    UINT32 descriptorEnd   = FLASH_DESCRIPTOR_SIZE;

    // Check for buffer size to be greater or equal to descriptor region size
    if (flashImage.size() < FLASH_DESCRIPTOR_SIZE) {
        msg(tr("parseInputFile: Input file is smaller then minimum descriptor size of %1 bytes").arg(FLASH_DESCRIPTOR_SIZE));
        return ERR_INVALID_FLASH_DESCRIPTOR;
    }

    // Parse descriptor map
    descriptorMap    = (FLASH_DESCRIPTOR_MAP*) (descriptor + sizeof(FLASH_DESCRIPTOR_HEADER));
    regionSection    = (FLASH_DESCRIPTOR_REGION_SECTION*) calculateAddress8(descriptor, descriptorMap->RegionBase);
    //componentSection = (FLASH_DESCRIPTOR_COMPONENT_SECTION*) calculateAddress8(descriptor, descriptorMap->ComponentBase);

    // GbE region
    QByteArray gbe;
    UINT32 gbeBegin = 0;
    UINT32 gbeEnd   = 0;
    if (regionSection->GbeLimit) {
        gbeBegin = calculateRegionOffset(regionSection->GbeBase);
        gbeEnd   = calculateRegionSize(regionSection->GbeBase, regionSection->GbeLimit);
        gbe = flashImage.mid(gbeBegin, gbeEnd);
        gbeEnd += gbeBegin;
    }
    // ME region
    QByteArray me;
    UINT32 meBegin = 0;
    UINT32 meEnd   = 0;
    if (regionSection->MeLimit) {
        meBegin = calculateRegionOffset(regionSection->MeBase);
        meEnd   = calculateRegionSize(regionSection->MeBase, regionSection->MeLimit);
        me = flashImage.mid(meBegin, meEnd);
        meEnd += meBegin;
    }
    // PDR region
    QByteArray pdr;
    UINT32 pdrBegin = 0;
    UINT32 pdrEnd   = 0;
    if (regionSection->PdrLimit) {
        pdrBegin = calculateRegionOffset(regionSection->PdrBase);
        pdrEnd   = calculateRegionSize(regionSection->PdrBase, regionSection->PdrLimit);
        pdr = flashImage.mid(pdrBegin, pdrEnd);
        pdrEnd += pdrBegin;
    }
    // BIOS region
    QByteArray bios;
    UINT32 biosBegin = 0;
    UINT32 biosEnd   = 0;
    if (regionSection->BiosLimit) {
        biosBegin = calculateRegionOffset(regionSection->BiosBase);
        biosEnd   = calculateRegionSize(regionSection->BiosBase, regionSection->BiosLimit);
        bios = flashImage.mid(biosBegin, biosEnd);
        biosEnd += biosBegin;
    }
    else {
        msg(tr("parseInputFile: descriptor parsing failed, BIOS region not found in descriptor"));
        return ERR_INVALID_FLASH_DESCRIPTOR;
    }

    // Check for intersections between regions
    if (hasIntersection(descriptorBegin, descriptorEnd, gbeBegin, gbeEnd)) {
        msg(tr("parseInputFile: descriptor parsing failed, descriptor region has intersection with GbE region"));
        return ERR_INVALID_FLASH_DESCRIPTOR;
    }
    if (hasIntersection(descriptorBegin, descriptorEnd, meBegin, meEnd)) {
        msg(tr("parseInputFile: descriptor parsing failed, descriptor region has intersection with ME region"));
        return ERR_INVALID_FLASH_DESCRIPTOR;
    }
    if (hasIntersection(descriptorBegin, descriptorEnd, biosBegin, biosEnd)) {
        msg(tr("parseInputFile: descriptor parsing failed, descriptor region has intersection with BIOS region"));
        return ERR_INVALID_FLASH_DESCRIPTOR;
    }
    if (hasIntersection(descriptorBegin, descriptorEnd, pdrBegin, pdrEnd)) {
        msg(tr("parseInputFile: descriptor parsing failed, descriptor region has intersection with PDR region"));
        return ERR_INVALID_FLASH_DESCRIPTOR;
    }
    if (hasIntersection(gbeBegin, gbeEnd, meBegin, meEnd)) {
        msg(tr("parseInputFile: descriptor parsing failed, GbE region has intersection with ME region"));
        return ERR_INVALID_FLASH_DESCRIPTOR;
    }
    if (hasIntersection(gbeBegin, gbeEnd, biosBegin, biosEnd)) {
        msg(tr("parseInputFile: descriptor parsing failed, GbE region has intersection with BIOS region"));
        return ERR_INVALID_FLASH_DESCRIPTOR;
    }
    if (hasIntersection(gbeBegin, gbeEnd, pdrBegin, pdrEnd)) {
        msg(tr("parseInputFile: descriptor parsing failed, GbE region has intersection with PDR region"));
        return ERR_INVALID_FLASH_DESCRIPTOR;
    }
    if (hasIntersection(meBegin, meEnd, biosBegin, biosEnd)) {
        msg(tr("parseInputFile: descriptor parsing failed, ME region has intersection with BIOS region"));
        return ERR_INVALID_FLASH_DESCRIPTOR;
    }
    if (hasIntersection(meBegin, meEnd, pdrBegin, pdrEnd)) {
        msg(tr("parseInputFile: descriptor parsing failed, ME region has intersection with PDR region"));
        return ERR_INVALID_FLASH_DESCRIPTOR;
    }
    if (hasIntersection(biosBegin, biosEnd, pdrBegin, pdrEnd)) {
        msg(tr("parseInputFile: descriptor parsing failed, BIOS region has intersection with PDR region"));
        return ERR_INVALID_FLASH_DESCRIPTOR;
    }

    // Region map is consistent
    QByteArray body;
    QString    name;
    QString    info;

    // Intel image
    name = tr("Intel image");
    info = tr("Size: %1\nFlash chips: %2\nRegions: %3\nMasters: %4\nPCH straps: %5\nPROC straps: %6\nICC table entries: %7")
        .arg(flashImage.size(), 8, 16, QChar('0'))
        .arg(descriptorMap->NumberOfFlashChips + 1) //
        .arg(descriptorMap->NumberOfRegions + 1)    // Zero-based numbers in storage
        .arg(descriptorMap->NumberOfMasters + 1)    //
        .arg(descriptorMap->NumberOfPchStraps)
        .arg(descriptorMap->NumberOfProcStraps)
        .arg(descriptorMap->NumberOfIccTableEntries);

    // Add Intel image tree item
    index = model->addItem(Image, IntelImage, COMPRESSION_ALGORITHM_NONE, name, "", info, QByteArray(), flashImage, QByteArray(), parent);

    // Descriptor
    // Get descriptor info
    body = flashImage.left(FLASH_DESCRIPTOR_SIZE);
    name = tr("Descriptor region");
    info = tr("Size: %1").arg(FLASH_DESCRIPTOR_SIZE, 4, 16, QChar('0'));

    // Check regions presence once again
    QVector<UINT32> offsets;
    if (regionSection->GbeLimit) {
        offsets.append(gbeBegin);
        info += tr("\nGbE region offset: %1").arg(gbeBegin, 8, 16, QChar('0'));
    }
    if (regionSection->MeLimit) {
        offsets.append(meBegin);
        info += tr("\nME region offset: %1").arg(meBegin, 8, 16, QChar('0'));
    }
    if (regionSection->BiosLimit) {
        offsets.append(biosBegin);
        info += tr("\nBIOS region offset: %1").arg(biosBegin, 8, 16, QChar('0'));
    }
    if (regionSection->PdrLimit) {
        offsets.append(pdrBegin);
        info += tr("\nPDR region offset: %1").arg(pdrBegin, 8, 16, QChar('0'));
    }

    // Add descriptor tree item
    model->addItem(Region, DescriptorRegion, COMPRESSION_ALGORITHM_NONE, name, "", info, QByteArray(), body, QByteArray(), index);

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

UINT8 FfsEngine::parseGbeRegion(const QByteArray & gbe, QModelIndex & index, const QModelIndex & parent)
{
    if (gbe.isEmpty())
        return ERR_EMPTY_REGION;

    // Get info
    QString      name = tr("GbE region");
    GBE_MAC*     mac = (GBE_MAC*) gbe.constData();
    GBE_VERSION* version = (GBE_VERSION*) (gbe.constData() + GBE_VERSION_OFFSET);
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
    index = model->addItem( Region,  GbeRegion, COMPRESSION_ALGORITHM_NONE, name, "", info, QByteArray(), gbe, QByteArray(), parent);

    return ERR_SUCCESS;
}

UINT8 FfsEngine::parseMeRegion(const QByteArray & me, QModelIndex & index, const QModelIndex & parent)
{
    if (me.isEmpty())
        return ERR_EMPTY_REGION;

    // Get info
    QString name = tr("ME region");
    QString info = tr("Size: %1").
        arg(me.size(), 8, 16, QChar('0'));

    ME_VERSION* version;
    INT32 versionOffset = me.indexOf(ME_VERSION_SIGNATURE);
    if (versionOffset < 0){
        info += tr("\nVersion: unknown");
        msg(tr("parseRegion: ME region version is unknown, it can be damaged"), parent);
    }
    else {
        version = (ME_VERSION*) (me.constData() + versionOffset);
        info += tr("\nVersion: %1.%2.%3.%4")
            .arg(version->major)
            .arg(version->minor)
            .arg(version->bugfix)
            .arg(version->build);
    }

    // Add tree item
    index = model->addItem( Region,  MeRegion, COMPRESSION_ALGORITHM_NONE, name, "", info, QByteArray(), me, QByteArray(), parent);

    return ERR_SUCCESS;
}

UINT8 FfsEngine::parsePdrRegion(const QByteArray & pdr, QModelIndex & index, const QModelIndex & parent)
{
    if (pdr.isEmpty())
        return ERR_EMPTY_REGION;

    // Get info
    QString name = tr("PDR region");
    QString info = tr("Size: %1").
        arg(pdr.size(), 8, 16, QChar('0'));

    // Add tree item
    index = model->addItem( Region,  PdrRegion, COMPRESSION_ALGORITHM_NONE, name, "", info, QByteArray(), pdr, QByteArray(), parent);

    return ERR_SUCCESS;
}

UINT8 FfsEngine::parseBiosRegion(const QByteArray & bios, QModelIndex & index, const QModelIndex & parent)
{
    if (bios.isEmpty())
        return ERR_EMPTY_REGION;

    // Get info
    QString name = tr("BIOS region");
    QString info = tr("Size: %1").
        arg(bios.size(), 8, 16, QChar('0'));

    // Add tree item
    index = model->addItem( Region,  BiosRegion, COMPRESSION_ALGORITHM_NONE, name, "", info, QByteArray(), bios, QByteArray(), parent);

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
        model->addItem( Padding, 0, COMPRESSION_ALGORITHM_NONE, name, "", info, QByteArray(), padding, QByteArray(), parent);
    }

    // Search for and parse all volumes
    UINT32 volumeOffset = prevVolumeOffset;
    UINT32 prevVolumeSize = 0;
    UINT32 volumeSize = 0;

    while(true)
    {
        // Padding between volumes
        if (volumeOffset > prevVolumeOffset + prevVolumeSize) {
            UINT32 paddingSize = volumeOffset - prevVolumeOffset - prevVolumeSize;
            QByteArray padding = bios.mid(prevVolumeOffset + prevVolumeSize, paddingSize);
            // Get info
            name = tr("Padding");
            info = tr("Size: %1")
                .arg(padding.size(), 8, 16, QChar('0'));
            // Add tree item
            model->addItem( Padding, 0, COMPRESSION_ALGORITHM_NONE, name, "", info, QByteArray(), padding, QByteArray(), parent);
        }

        // Get volume size
        result = getVolumeSize(bios, volumeOffset, volumeSize);
        if (result)
            return result;

        //Check that volume is fully present in input
        if (volumeOffset + volumeSize > (UINT32) bios.size()) {
            msg(tr("parseBios: Volume overlaps the end of input buffer"), parent);
            return ERR_INVALID_VOLUME;
        }

        // Check volume revision and alignment
        EFI_FIRMWARE_VOLUME_HEADER* volumeHeader = (EFI_FIRMWARE_VOLUME_HEADER*) (bios.constData() + volumeOffset);
        UINT32 alignment;
        if (volumeHeader->Revision == 1) {
            // Acquire alignment bits
            bool alignmentCap    = volumeHeader->Attributes & EFI_FVB_ALIGNMENT_CAP;
            bool alignment2      = volumeHeader->Attributes & EFI_FVB_ALIGNMENT_2;
            bool alignment4      = volumeHeader->Attributes & EFI_FVB_ALIGNMENT_4;
            bool alignment8      = volumeHeader->Attributes & EFI_FVB_ALIGNMENT_8;
            bool alignment16     = volumeHeader->Attributes & EFI_FVB_ALIGNMENT_16;
            bool alignment32     = volumeHeader->Attributes & EFI_FVB_ALIGNMENT_32;
            bool alignment64     = volumeHeader->Attributes & EFI_FVB_ALIGNMENT_64;
            bool alignment128    = volumeHeader->Attributes & EFI_FVB_ALIGNMENT_128;
            bool alignment256    = volumeHeader->Attributes & EFI_FVB_ALIGNMENT_256;
            bool alignment512    = volumeHeader->Attributes & EFI_FVB_ALIGNMENT_512;
            bool alignment1k     = volumeHeader->Attributes & EFI_FVB_ALIGNMENT_1K;
            bool alignment2k     = volumeHeader->Attributes & EFI_FVB_ALIGNMENT_2K;
            bool alignment4k     = volumeHeader->Attributes & EFI_FVB_ALIGNMENT_4K;
            bool alignment8k     = volumeHeader->Attributes & EFI_FVB_ALIGNMENT_8K;
            bool alignment16k    = volumeHeader->Attributes & EFI_FVB_ALIGNMENT_16K;
            bool alignment32k    = volumeHeader->Attributes & EFI_FVB_ALIGNMENT_32K;
            bool alignment64k    = volumeHeader->Attributes & EFI_FVB_ALIGNMENT_64K;

            // Check alignment setup
            if (!alignmentCap &&
                (  alignment2   || alignment4   || alignment8   || alignment16
                || alignment32  || alignment64  || alignment128 || alignment256
                || alignment512 || alignment1k  || alignment2k  || alignment4k
                || alignment8k  || alignment16k || alignment32k || alignment64k))
                msg("parseBios: Incompatible revision 1 volume alignment setup", parent);

            // Assume that smaller alignment value consumes greater
            //!TODO: refactor this code
            alignment = 0x01;
            if (alignment2)
                alignment = 0x02;
            else if (alignment4)
                alignment = 0x04;
            else if (alignment8)
                alignment = 0x08;
            else if (alignment16)
                alignment = 0x10;
            else if (alignment32)
                alignment = 0x20;
            else if (alignment64)
                alignment = 0x40;
            else if (alignment128)
                alignment = 0x80;
            else if (alignment256)
                alignment = 0x100;
            else if (alignment512)
                alignment = 0x200;
            else if (alignment1k)
                alignment = 0x400;
            else if (alignment2k)
                alignment = 0x800;
            else if (alignment4k)
                alignment = 0x1000;
            else if (alignment8k)
                alignment = 0x2000;
            else if (alignment16k)
                alignment = 0x4000;
            else if (alignment32k)
                alignment = 0x8000;
            else if (alignment64k)
                alignment = 0x10000;

            // Check alignment
            if (volumeOffset % alignment) {
                msg(tr("parseBios: Unaligned revision 1 volume"), parent);
            }
        }
        else if (volumeHeader->Revision == 2) {
            // Acquire alignment
            alignment = (UINT32) pow(2.0, (int) (volumeHeader->Attributes & EFI_FVB2_ALIGNMENT) >> 16);

            // Check alignment
            if (volumeOffset % alignment) {
                msg(tr("parseBios: Unaligned revision 2 volume"), parent);
            }
        }
        else
            msg(tr("parseBios: Unknown volume revision (%1)").arg(volumeHeader->Revision), parent);

        // Parse volume
        QModelIndex index;
        UINT8 result = parseVolume(bios.mid(volumeOffset, volumeSize), index, parent);
        if (result)
            msg(tr("parseBios: Volume parsing failed (%1)").arg(result), parent);

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
                model->addItem( Padding, 0, COMPRESSION_ALGORITHM_NONE, name, "", info, QByteArray(), padding, QByteArray(), parent);
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

    nextVolumeOffset  = nextIndex - EFI_FV_SIGNATURE_OFFSET;
    return ERR_SUCCESS;
}

UINT8 FfsEngine::getVolumeSize(const QByteArray & bios, UINT32 volumeOffset, UINT32 & volumeSize)
{
    // Populate volume header
    EFI_FIRMWARE_VOLUME_HEADER* volumeHeader = (EFI_FIRMWARE_VOLUME_HEADER*) (bios.constData() + volumeOffset);

    // Check volume signature
    if (QByteArray((const char*) &volumeHeader->Signature, sizeof(volumeHeader->Signature)) != EFI_FV_SIGNATURE)
        return ERR_INVALID_VOLUME;

    // Use BlockMap to determine volume size
    EFI_FV_BLOCK_MAP_ENTRY* entry = (EFI_FV_BLOCK_MAP_ENTRY*) (bios.constData() + volumeOffset + sizeof(EFI_FIRMWARE_VOLUME_HEADER));
    volumeSize = 0;
    while(entry->NumBlocks != 0 && entry->Length != 0) {
        if ((void*) entry > bios.constData() + bios.size())
            return ERR_INVALID_VOLUME;

        volumeSize += entry->NumBlocks * entry->Length;
        entry += 1;
    }

    return ERR_SUCCESS;
}

UINT8  FfsEngine::parseVolume(const QByteArray & volume, QModelIndex & index, const QModelIndex & parent, const UINT8 mode)
{
    // Populate volume header
    EFI_FIRMWARE_VOLUME_HEADER* volumeHeader = (EFI_FIRMWARE_VOLUME_HEADER*) (volume.constData());

    // Check filesystem GUID to be known
    // Do not parse volume with unknown FFS, because parsing will fail
    bool parseCurrentVolume = true;
    // FFS GUID v1
    if (QByteArray((const char*) &volumeHeader->FileSystemGuid, sizeof(EFI_GUID)) == EFI_FIRMWARE_FILE_SYSTEM_GUID) {
        // Code can be added here
    }
    // FFS GUID v2
    else if (QByteArray((const char*) &volumeHeader->FileSystemGuid, sizeof(EFI_GUID)) == EFI_FIRMWARE_FILE_SYSTEM2_GUID) {
        // Code can be added here
    }
    // Other GUID
    else {
        msg(tr("parseBios: Unknown file system (%1)").arg(guidToQString(volumeHeader->FileSystemGuid)), parent);
        parseCurrentVolume = false;
    }

    // Check attributes
    // Determine value of empty byte
    char empty = volumeHeader->Attributes & EFI_FVB_ERASE_POLARITY ? '\xFF' : '\x00';

    // Check header checksum by recalculating it
    if (calculateChecksum16((UINT16*) volumeHeader, volumeHeader->HeaderLength)) {
        msg(tr("parseBios: Volume header checksum is invalid"), parent);
    }

    // Check for presence of extended header, only if header revision is greater then 1
    UINT32 headerSize;
    if (volumeHeader->Revision > 1 && volumeHeader->ExtHeaderOffset) {
        EFI_FIRMWARE_VOLUME_EXT_HEADER* extendedHeader = (EFI_FIRMWARE_VOLUME_EXT_HEADER*) ((UINT8*) volumeHeader + volumeHeader->ExtHeaderOffset);
        headerSize = volumeHeader->ExtHeaderOffset + extendedHeader->ExtHeaderSize;
    } else {
        headerSize = volumeHeader->HeaderLength;
    }

    // Get volume size
    UINT8 result;
    UINT32 volumeSize;

    result = getVolumeSize(volume, 0, volumeSize);
    if (result)
        return result;

    // Check reported size
    if (volumeSize != volumeHeader->FvLength) {
        msg(tr("%1: volume size stored in header %2 differs from calculated size %3")
            .arg(guidToQString(volumeHeader->FileSystemGuid))
            .arg(volumeHeader->FvLength, 8, 16, QChar('0'))
            .arg(volumeSize, 8, 16, QChar('0')), parent);
    }

    // Get info
    QString name = guidToQString(volumeHeader->FileSystemGuid);
    QString info = tr("Size: %1\nRevision: %2\nAttributes: %3\nHeader size: %4")
        .arg(volumeSize, 8, 16, QChar('0'))
        .arg(volumeHeader->Revision)
        .arg(volumeHeader->Attributes, 8, 16, QChar('0'))
        .arg(volumeHeader->HeaderLength, 4, 16, QChar('0'));

    // Add tree item
    QByteArray  header = volume.left(headerSize);
    QByteArray  body   = volume.mid(headerSize, volumeSize - headerSize);
    index  = model->addItem( Volume, 0, COMPRESSION_ALGORITHM_NONE, name, "", info, header, body, QByteArray(), parent, mode);

    // Do not parse volumes with unknown FS
    if (!parseCurrentVolume)
        return ERR_SUCCESS;

    // Search for and parse all files
    UINT32 fileOffset = headerSize;
    UINT32 fileSize;
    QQueue<QByteArray> files;

    while (true) {
        result = getFileSize(volume, fileOffset, fileSize);
        if (result)
            return result;

        // Check file size to be at least sizeof(EFI_FFS_FILE_HEADER)
        if (fileSize < sizeof(EFI_FFS_FILE_HEADER)) {
            msg(tr("parseVolume: File with invalid size"), index);
            return ERR_INVALID_FILE;
        }

        QByteArray file = volume.mid(fileOffset, fileSize);
        QByteArray header = file.left(sizeof(EFI_FFS_FILE_HEADER));

        // If we are at empty space in the end of volume
        if (header.count(empty) == header.size())
            break; // Exit from loop

        // Check file alignment
        EFI_FFS_FILE_HEADER* fileHeader = (EFI_FFS_FILE_HEADER*) header.constData();
        UINT8 alignmentPower = ffsAlignmentTable[(fileHeader->Attributes & FFS_ATTRIB_DATA_ALIGNMENT) >> 3];
        UINT32 alignment = (UINT32) pow(2.0, alignmentPower);
        if ((fileOffset + sizeof(EFI_FFS_FILE_HEADER)) % alignment) {
            msg(tr("parseVolume: %1, unaligned file").arg(guidToQString(fileHeader->Name)), index);
        }

        // Check file GUID
        if (fileHeader->Type != EFI_FV_FILETYPE_PAD && files.indexOf(header.left(sizeof(EFI_GUID))) != -1)
            msg(tr("%1: file with duplicate GUID").arg(guidToQString(fileHeader->Name)), index);

        // Add file GUID to queue
        files.enqueue(header.left(sizeof(EFI_GUID)));

        // Parse file
        QModelIndex fileIndex;
        result = parseFile(file, fileIndex, empty == '\xFF' ? ERASE_POLARITY_TRUE : ERASE_POLARITY_FALSE, index);
        if (result)
            msg(tr("parseVolume: Parse FFS file failed (%1)").arg(result), index);

        // Move to next file
        fileOffset += fileSize;
        fileOffset = ALIGN8(fileOffset);

        // Exit from loop if no files left
        if (fileOffset >= (UINT32) volume.size())
            break;
    }

    return ERR_SUCCESS;
}

UINT8 FfsEngine::getFileSize(const QByteArray & volume, const UINT32 fileOffset, UINT32 & fileSize)
{
    EFI_FFS_FILE_HEADER* fileHeader = (EFI_FFS_FILE_HEADER*) (volume.constData() + fileOffset);
    fileSize = uint24ToUint32(fileHeader->Size);
    return ERR_SUCCESS;
}

UINT8 FfsEngine::parseFile(const QByteArray & file, QModelIndex & index, const UINT8 erasePolarity, const QModelIndex & parent, const UINT8 mode)
{
    // Populate file header
    EFI_FFS_FILE_HEADER* fileHeader = (EFI_FFS_FILE_HEADER*) file.constData();

    // Check file state
    // Determine file erase polarity
    bool fileErasePolarity = fileHeader->State | EFI_FILE_ERASE_POLARITY;

    // Check file erase polarity to be the same as parent erase polarity
    if (erasePolarity != ERASE_POLARITY_UNKNOWN && (bool) erasePolarity != fileErasePolarity) {
        msg(tr("parseFile: %1, erase polarity differs from parent erase polarity"), parent);
    }

    // Construct empty byte for this file
    char empty = fileErasePolarity ? '\xFF' : '\x00';

    // Check header checksum
    QByteArray header = file.left(sizeof(EFI_FFS_FILE_HEADER));
    QByteArray tempHeader = header;
    EFI_FFS_FILE_HEADER* tempFileHeader = (EFI_FFS_FILE_HEADER*) (tempHeader.data());
    tempFileHeader->IntegrityCheck.Checksum.Header = 0;
    tempFileHeader->IntegrityCheck.Checksum.File = 0;
    UINT8 calculated = calculateChecksum8((UINT8*) tempFileHeader, sizeof(EFI_FFS_FILE_HEADER) - 1);
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
        if(fileHeader->Attributes & FFS_ATTRIB_TAIL_PRESENT)
            bufferSize -= sizeof(UINT16);
        calculated = calculateChecksum8((UINT8*)(file.constData() + sizeof(EFI_FFS_FILE_HEADER)), bufferSize);
        if (fileHeader->IntegrityCheck.Checksum.File != calculated) {
            msg(tr("parseFile: %1, stored data checksum %2 differs from calculated %3")
                .arg(guidToQString(fileHeader->Name))
                .arg(fileHeader->IntegrityCheck.Checksum.File, 2, 16, QChar('0'))
                .arg(calculated, 2, 16, QChar('0')), parent);
        }
    }
    // Data checksum must be one of predefined values
    else {
        if (fileHeader->IntegrityCheck.Checksum.File != FFS_FIXED_CHECKSUM && fileHeader->IntegrityCheck.Checksum.File != FFS_FIXED_CHECKSUM2) {
            msg(tr("parseVolume: %1, stored data checksum %2 differs from standard value")
                .arg(guidToQString(fileHeader->Name))
                .arg(fileHeader->IntegrityCheck.Checksum.File, 2, 16, QChar('0')), parent);
        }
    }

    // Get file body
    QByteArray body = file.right(file.size() - sizeof(EFI_FFS_FILE_HEADER));
    // Check for file tail presence
    QByteArray tail;
    if (fileHeader->Attributes & FFS_ATTRIB_TAIL_PRESENT)
    {
        //Check file tail;
        tail = body.right(sizeof(UINT16));
        UINT16 tailValue = *(UINT16*) tail.constData();
        if (fileHeader->IntegrityCheck.TailReference != (UINT16)~tailValue)
            msg(tr("parseFile: %1, bitwise not of tail value %2 differs from %3 stored in file header")
            .arg(guidToQString(fileHeader->Name))
            .arg(~tailValue, 4, 16, QChar('0'))
            .arg(fileHeader->IntegrityCheck.TailReference, 4, 16, QChar('0')), parent);

        // Remove tail from file body
        body = body.left(body.size() - sizeof(UINT16));
    }

    // Parse current file by default
    bool parseCurrentFile = true;
    bool parseAsBios = false;

    // Check file type
    //!TODO: add more file specific checks
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
        break;
    case EFI_FV_FILETYPE_PEI_CORE:
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
        parseCurrentFile = false;
        msg(tr("parseFile: Unknown file type (%1)").arg(fileHeader->Type, 2, 16, QChar('0')), parent);
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
    info = tr("Type: %1\nAttributes: %2\nSize: %3\nState: %4")
        .arg(fileHeader->Type, 2, 16, QChar('0'))
        .arg(fileHeader->Attributes, 2, 16, QChar('0'))
        .arg(uint24ToUint32(fileHeader->Size), 6, 16, QChar('0'))
        .arg(fileHeader->State, 2, 16, QChar('0'));

    // Add tree item
    index = model->addItem( File, fileHeader->Type, COMPRESSION_ALGORITHM_NONE, name, "", info, header, body, tail, parent, mode);

    if (!parseCurrentFile)
        return ERR_SUCCESS;

    // Parse file as BIOS space
    UINT8 result;
    if (parseAsBios) {
        result = parseBios(body, index);
        if (result && result != ERR_VOLUMES_NOT_FOUND)
            msg(tr("parseFile: Parse file as BIOS failed (%1)").arg(result), index);
        return ERR_SUCCESS;
    }

    // Parse sections
    result = parseSections(body, index);
    if (result)
        return result;

    return ERR_SUCCESS;
}

UINT8 FfsEngine::getSectionSize(const QByteArray & file, const UINT32 sectionOffset, UINT32 & sectionSize)
{
    EFI_COMMON_SECTION_HEADER* sectionHeader = (EFI_COMMON_SECTION_HEADER*) (file.constData() + sectionOffset);
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
    EFI_COMMON_SECTION_HEADER* sectionHeader = (EFI_COMMON_SECTION_HEADER*) (section.constData());
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
            EFI_COMPRESSION_SECTION* compressedSectionHeader = (EFI_COMPRESSION_SECTION*) sectionHeader;
            header = section.left(sizeof(EFI_COMPRESSION_SECTION));
            body  = section.mid(sizeof(EFI_COMPRESSION_SECTION), sectionSize - sizeof(EFI_COMPRESSION_SECTION));
            algorithm = COMPRESSION_ALGORITHM_UNKNOWN;
            // Decompress section
            result = decompress(body, compressedSectionHeader->CompressionType, decompressed, &algorithm);
            if (result) {
                msg(tr("parseSection: Section decompression failed (%1)").arg(result), parent);
                parseCurrentSection = false;
            }

            // Get info
            info = tr("Type: %1\nSize: %2\nCompression type: %3\nDecompressed size: %4")
                .arg(sectionHeader->Type, 2, 16, QChar('0'))
                .arg(body.size(), 8, 16, QChar('0'))
                .arg(compressionTypeToQString(algorithm))
                .arg(compressedSectionHeader->UncompressedLength, 8, 16, QChar('0'));

            // Add tree item
            index  = model->addItem( Section, sectionHeader->Type, algorithm, name, "", info, header, body, QByteArray(), parent, mode);

            // Parse decompressed data
            if (parseCurrentSection) {
                result = parseSections(decompressed, index);
                if (result)
                    return result;
            }
        }
        break;
    case EFI_SECTION_GUID_DEFINED:
        {
            bool parseCurrentSection = true;
            EFI_GUID_DEFINED_SECTION* guidDefinedSectionHeader;
            header = section.left(sizeof(EFI_GUID_DEFINED_SECTION));
            guidDefinedSectionHeader = (EFI_GUID_DEFINED_SECTION*) (header.constData());
            header = section.left(guidDefinedSectionHeader->DataOffset);
            guidDefinedSectionHeader = (EFI_GUID_DEFINED_SECTION*) (header.constData());
            body   = section.mid(guidDefinedSectionHeader->DataOffset, sectionSize - guidDefinedSectionHeader->DataOffset);
            QByteArray decompressed = body;
            UINT8 algorithm = COMPRESSION_ALGORITHM_NONE;
            // Check if section requires processing
            if (guidDefinedSectionHeader->Attributes & EFI_GUIDED_SECTION_PROCESSING_REQUIRED) {
                // Try to decompress section body using both known compression algorithms
                algorithm = COMPRESSION_ALGORITHM_UNKNOWN;
                // Tiano
                result = decompress(body, EFI_STANDARD_COMPRESSION, decompressed, &algorithm);
                if (result) {
                    result = decompress(body, EFI_CUSTOMIZED_COMPRESSION, decompressed, &algorithm);
                    if (result) {
                        msg(tr("parseSection: GUID defined section can not be decompressed (%1)").arg(result), parent);
                        parseCurrentSection = false;
                    }
                }
            }

            // Get info
            name = guidToQString(guidDefinedSectionHeader->SectionDefinitionGuid);
            info = tr("Type: %1\nSize: %2\nData offset: %3\nAttributes: %4\nCompression type: %5")
                .arg(sectionHeader->Type, 2, 16, QChar('0'))
                .arg(body.size(), 8, 16, QChar('0'))
                .arg(guidDefinedSectionHeader->DataOffset, 4, 16, QChar('0'))
                .arg(guidDefinedSectionHeader->Attributes, 4, 16, QChar('0'))
                .arg(compressionTypeToQString(algorithm));

            // Add tree item
            index  = model->addItem( Section, sectionHeader->Type, algorithm, name, "", info, header, body, QByteArray(), parent, mode);

            // Parse decompressed data
            if (parseCurrentSection) {
                result = parseSections(decompressed, index);
                if (result)
                    return result;
            }
        }
        break;
    case EFI_SECTION_DISPOSABLE:
        {
            header = section.left(sizeof(EFI_DISPOSABLE_SECTION));
            body   = section.mid(sizeof(EFI_DISPOSABLE_SECTION), sectionSize - sizeof(EFI_DISPOSABLE_SECTION));

            // Get info
            info = tr("parseSection: %1\nSize: %2")
                .arg(sectionHeader->Type, 2, 16, QChar('0'))
                .arg(body.size(), 8, 16, QChar('0'));

            // Add tree item
            index  = model->addItem( Section, sectionHeader->Type, COMPRESSION_ALGORITHM_NONE, name, "", info, header, body, QByteArray(), parent, mode);

            // Parse section body
            result = parseSections(body, index);
            if (result)
                return result;
        }
        break;

    // Leaf sections
    case EFI_SECTION_PE32:
    case EFI_SECTION_PIC:
    case EFI_SECTION_TE:
    case EFI_SECTION_VERSION:
    case EFI_SECTION_FREEFORM_SUBTYPE_GUID:
    case EFI_SECTION_DXE_DEPEX:
    case EFI_SECTION_PEI_DEPEX:
    case EFI_SECTION_SMM_DEPEX:
    case EFI_SECTION_COMPATIBILITY16:
        headerSize = sizeOfSectionHeaderOfType(sectionHeader->Type);
        header     = section.left(headerSize);
        body       = section.mid(headerSize, sectionSize - headerSize);

        // Get info
        info = tr("Type: %1\nSize: %2")
            .arg(sectionHeader->Type, 2, 16, QChar('0'))
            .arg(body.size(), 8, 16, QChar('0'));

        // Add tree item
        index = model->addItem( Section, sectionHeader->Type, COMPRESSION_ALGORITHM_NONE, name, "", info, header, body, QByteArray(), parent, mode);
        break;
    case EFI_SECTION_USER_INTERFACE:
        {
            header = section.left(sizeof(EFI_USER_INTERFACE_SECTION));
            body   = section.mid(sizeof(EFI_USER_INTERFACE_SECTION), sectionSize - sizeof(EFI_USER_INTERFACE_SECTION));

            // Get info
            info = tr("Type: %1\nSize: %2")
                .arg(sectionHeader->Type, 2, 16, QChar('0'))
                .arg(body.size(), 8, 16, QChar('0'));

            // Add tree item
            index = model->addItem( Section, sectionHeader->Type, COMPRESSION_ALGORITHM_NONE, name, "", info, header, body, QByteArray(), parent, mode);

            // Rename parent file
            QString text = QString::fromUtf16((const ushort*)body.constData());
            model->setTextString(model->findParentOfType(parent,  File), text);
        }
        break;
    case EFI_SECTION_FIRMWARE_VOLUME_IMAGE:
        header = section.left(sizeof(EFI_FIRMWARE_VOLUME_IMAGE_SECTION));
        body   = section.mid(sizeof(EFI_FIRMWARE_VOLUME_IMAGE_SECTION), sectionSize - sizeof(EFI_FIRMWARE_VOLUME_IMAGE_SECTION));

        // Get info
        info = tr("Type: %1\nSize: %2")
            .arg(sectionHeader->Type, 2, 16, QChar('0'))
            .arg(body.size(), 8, 16, QChar('0'));

        // Add tree item
        index = model->addItem( Section, sectionHeader->Type, COMPRESSION_ALGORITHM_NONE, name, "", info, header, body, QByteArray(), parent, mode);

        // Parse section body as BIOS space
        result = parseBios(body, index);
        if (result && result != ERR_VOLUMES_NOT_FOUND) {
            msg(tr("parseSection: Firmware volume image can not be parsed as BIOS (%1)").arg(result), index);
            return result;
        }
        break;
    case EFI_SECTION_RAW:
        header = section.left(sizeof(EFI_RAW_SECTION));
        body   = section.mid(sizeof(EFI_RAW_SECTION), sectionSize - sizeof(EFI_RAW_SECTION));

        // Get info
        info = tr("Type: %1\nSize: %2")
            .arg(sectionHeader->Type, 2, 16, QChar('0'))
            .arg(body.size(), 8, 16, QChar('0'));

        // Add tree item
        index  = model->addItem( Section, sectionHeader->Type, COMPRESSION_ALGORITHM_NONE, name, "", info, header, body, QByteArray(), parent, mode);

        // Parse section body as BIOS space
        result = parseBios(body, index);
        if (result && result != ERR_VOLUMES_NOT_FOUND) {
            msg(tr("parseSection: Raw section can not be parsed as BIOS (%1)").arg(result), index);
            return result;
        }
        break;
    default:
        header = section.left(sizeof(EFI_COMMON_SECTION_HEADER));
        body   = section.mid(sizeof(EFI_COMMON_SECTION_HEADER), sectionSize - sizeof(EFI_COMMON_SECTION_HEADER));
        // Get info
        info = tr("Type: %1\nSize: %2")
            .arg(sectionHeader->Type, 2, 16, QChar('0'))
            .arg(body.size(), 8, 16, QChar('0'));

        // Add tree item
        index  = model->addItem( Section, sectionHeader->Type, COMPRESSION_ALGORITHM_NONE, name, "", info, header, body, QByteArray(), parent, mode);
        msg(tr("parseSection: Section with unknown type (%1)").arg(sectionHeader->Type, 2, 16, QChar('0')), index);
    }
    return ERR_SUCCESS;
}

// Operations on tree items
UINT8 FfsEngine::create(const QModelIndex & index, const UINT8 type, const QByteArray & header, const QByteArray & body, const UINT8 mode, const UINT8 action, const UINT8 algorithm)
{
    QByteArray created;
    UINT8 result;

    if (!index.isValid() || !index.parent().isValid())
        return ERR_INVALID_PARAMETER;

    QModelIndex parent;
    if (mode == CREATE_MODE_BEFORE || mode == CREATE_MODE_AFTER)
        parent = index.parent();
    else
        parent = index;

    // Create item
    if (type ==  File) {
        if (model->type(parent) !=  Volume)
            return ERR_INVALID_FILE;

        EFI_FIRMWARE_VOLUME_HEADER* volumeHeader = (EFI_FIRMWARE_VOLUME_HEADER*) model->header(parent).constData();
        UINT8 revision = volumeHeader->Revision;
        bool erasePolarity = volumeHeader->Attributes & EFI_FVB_ERASE_POLARITY;

        if (header.size() != sizeof(EFI_FFS_FILE_HEADER))
            return ERR_INVALID_FILE;

        QByteArray newHeader = header;
        EFI_FFS_FILE_HEADER* fileHeader = (EFI_FFS_FILE_HEADER*) newHeader.data();

        // Correct file size
        UINT8 tailSize = fileHeader->Attributes & FFS_ATTRIB_TAIL_PRESENT ? sizeof(UINT16) : 0;
        uint32ToUint24(sizeof(EFI_FFS_FILE_HEADER) + body.size() + tailSize, fileHeader->Size);

        // Recalculate header checksum
        fileHeader->IntegrityCheck.Checksum.Header = 0;
        fileHeader->IntegrityCheck.Checksum.File = 0;
        fileHeader->IntegrityCheck.Checksum.Header = calculateChecksum8((UINT8*) fileHeader, sizeof(EFI_FFS_FILE_HEADER) - 1);

        // Recalculate data checksum, if needed
        if (fileHeader->Attributes & FFS_ATTRIB_CHECKSUM)
            fileHeader->IntegrityCheck.Checksum.File = calculateChecksum8((UINT8*) body.constData(), body.size());
        else if (revision == 1)
            fileHeader->IntegrityCheck.Checksum.File = FFS_FIXED_CHECKSUM;
        else
            fileHeader->IntegrityCheck.Checksum.File = FFS_FIXED_CHECKSUM2;

        // Append body
        created.append(body);

        // Append tail, if needed
        if (tailSize)
            created.append(~fileHeader->IntegrityCheck.TailReference);

        // Set file state
        UINT8 state = EFI_FILE_DATA_VALID | EFI_FILE_HEADER_VALID | EFI_FILE_HEADER_CONSTRUCTION;
        if (erasePolarity)
            state = ~state;
        fileHeader->State = state;

        // Prepend header
        created.prepend(newHeader);

        // Parse file
        QModelIndex fileIndex;
        result = parseFile(created, fileIndex, erasePolarity ? ERASE_POLARITY_TRUE : ERASE_POLARITY_FALSE, index, mode);
        if (result)
            return result;

        // Set action
        model->setAction(fileIndex, action);
    }
    else if (type ==  Section) {
        if (model->type(parent) !=  File && model->type(parent) !=  Section)
            return ERR_INVALID_SECTION;

        if (header.size() < (int) sizeof(EFI_COMMON_SECTION_HEADER))
            return ERR_INVALID_SECTION;

        QByteArray newHeader = header;
        EFI_COMMON_SECTION_HEADER* commonHeader = (EFI_COMMON_SECTION_HEADER*) newHeader.data();

        switch (commonHeader->Type)
        {
        case EFI_SECTION_COMPRESSION: {
                EFI_COMPRESSION_SECTION* sectionHeader = (EFI_COMPRESSION_SECTION*) newHeader.data();
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
                if (result)
                    return result;

                // Set create action
                model->setAction(sectionIndex, action);
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
                if (result)
                    return result;

                // Set create action
                model->setAction(sectionIndex, action);
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
            if (result)
                return result;

            // Set create action
            model->setAction(sectionIndex, action);
        }
    }
    else
        return ERR_NOT_IMPLEMENTED;

    return ERR_SUCCESS;
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
    if (model->type(parent) ==  Volume) {
        type =  File;
        headerSize = sizeof(EFI_FFS_FILE_HEADER);
    }
    else if (model->type(parent) ==  File) {
        type =  Section;
        EFI_COMMON_SECTION_HEADER* commonHeader = (EFI_COMMON_SECTION_HEADER*) object.constData();
        headerSize = sizeOfSectionHeaderOfType(commonHeader->Type);
    }
    else
        return ERR_NOT_IMPLEMENTED;

    return create(index, type, object.left(headerSize), object.right(object.size() - headerSize), mode,  Insert);
}

UINT8 FfsEngine::replace(const QModelIndex & index, const QByteArray & object, const UINT8 mode)
{
    if (!index.isValid())
        return ERR_INVALID_PARAMETER;

    // Determine type of item to replace
    UINT32 headerSize;
    UINT8 result;
    if (model->type(index) ==  File) {
        if (mode == REPLACE_MODE_AS_IS) {
            headerSize = sizeof(EFI_FFS_FILE_HEADER);
            result = create(index,  File, object.left(headerSize), object.right(object.size() - headerSize), CREATE_MODE_AFTER,  Replace);
        }
        else if (mode == REPLACE_MODE_BODY)
            result = create(index,  File, model->header(index), object, CREATE_MODE_AFTER,  Replace);
        else
            return ERR_NOT_IMPLEMENTED;
    }
    else if (model->type(index) ==  Section) {
        if (mode == REPLACE_MODE_AS_IS) {
            EFI_COMMON_SECTION_HEADER* commonHeader = (EFI_COMMON_SECTION_HEADER*) object.constData();
            headerSize = sizeOfSectionHeaderOfType(commonHeader->Type);
            result = create(index,  Section, object.left(headerSize), object.right(object.size() - headerSize), CREATE_MODE_AFTER,  Replace);
        }
        else if (mode == REPLACE_MODE_BODY) {
            result = create(index,  Section, model->header(index), object, CREATE_MODE_AFTER,  Replace, model->compression(index));
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
    model->setAction(index,  Remove);

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
        if (model->type(index) ==  Section) {
            QByteArray decompressed;
            UINT8 result;
            if (model->subtype(index) == EFI_SECTION_COMPRESSION) {
                EFI_COMPRESSION_SECTION* compressedHeader = (EFI_COMPRESSION_SECTION*) model->header(index).constData();
                result = decompress(model->body(index), compressedHeader->CompressionType, decompressed);
                if (result)
                    return result;
                extracted.append(decompressed);
                return ERR_SUCCESS;
            }
            else if (model->subtype(index) == EFI_SECTION_GUID_DEFINED) {
                QByteArray decompressed;
                // Check if section requires processing
                EFI_GUID_DEFINED_SECTION* guidDefinedSectionHeader = (EFI_GUID_DEFINED_SECTION*) model->header(index).constData();
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
    model->setAction(index,  Remove);

    return ERR_SUCCESS;
}

UINT8 FfsEngine::rebuild(const QModelIndex & index)
{
    if (!index.isValid())
        return ERR_INVALID_PARAMETER;

    // Set action for the item
    model->setAction(index,  Rebuild);

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
        data = (UINT8*) compressedData.constData();
        dataSize = compressedData.size();

        // Check header to be valid
        header = (EFI_TIANO_HEADER*) data;
        if (header->CompSize + sizeof(EFI_TIANO_HEADER) != dataSize)
            return ERR_STANDARD_DECOMPRESSION_FAILED;

        // Get info function is the same for both algorithms
        if (ERR_SUCCESS != EfiTianoGetInfo(data, dataSize, &decompressedSize, &scratchSize))
            return ERR_STANDARD_DECOMPRESSION_FAILED;

        // Allocate memory
        decompressed = new UINT8[decompressedSize];
        scratch = new UINT8[scratchSize];

        // Decompress section data
        // Try Tiano decompression first
        if (ERR_SUCCESS != TianoDecompress(data, dataSize, decompressed, decompressedSize, scratch, scratchSize)) {
            // Not Tiano, try EFI 1.1
            if (ERR_SUCCESS != EfiDecompress(data, dataSize, decompressed, decompressedSize, scratch, scratchSize)) {
                if (algorithm)
                    *algorithm = COMPRESSION_ALGORITHM_UNKNOWN;
                return ERR_STANDARD_DECOMPRESSION_FAILED;
            }
            else if (algorithm)
                *algorithm = COMPRESSION_ALGORITHM_EFI11;
        }
        else if (algorithm)
            *algorithm = COMPRESSION_ALGORITHM_TIANO;

        decompressedData = QByteArray((const char*) decompressed, decompressedSize);

        // Free allocated memory
        delete[] decompressed;
        delete[] scratch;

        return ERR_SUCCESS;
    case EFI_CUSTOMIZED_COMPRESSION:
        // Get buffer sizes
        data = (UINT8*) compressedData.constData();
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
            shittySectionHeader = (EFI_COMMON_SECTION_HEADER*) data;
            shittySectionSize = sizeOfSectionHeaderOfType(shittySectionHeader->Type);

            // Decompress section data once again
            data += shittySectionSize;

            // Get info again
            if (ERR_SUCCESS != LzmaGetInfo(data, dataSize, &decompressedSize))
                return ERR_CUSTOMIZED_DECOMPRESSION_FAILED;

            // Decompress section data again
            if (ERR_SUCCESS != LzmaDecompress(data, dataSize, decompressed)) {
                if (algorithm)
                    *algorithm = COMPRESSION_ALGORITHM_UNKNOWN;
                return ERR_CUSTOMIZED_DECOMPRESSION_FAILED;
            }
            else {
                if (algorithm)
                    *algorithm = COMPRESSION_ALGORITHM_IMLZMA;
                decompressedData = QByteArray((const char*) decompressed, decompressedSize);
            }
        }
        else {
            if (algorithm)
                *algorithm = COMPRESSION_ALGORITHM_LZMA;
            decompressedData = QByteArray((const char*) decompressed, decompressedSize);
        }

        // Free memory
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
    UINT32 compressedSize = 0;

    switch (algorithm) {
    case COMPRESSION_ALGORITHM_NONE:
        {
            compressedData = data;
            return ERR_SUCCESS;
        }
        break;
    case COMPRESSION_ALGORITHM_EFI11:
        {
            if (EfiCompress((UINT8*) data.constData(), data.size(), NULL, &compressedSize) != ERR_BUFFER_TOO_SMALL)
                return ERR_STANDARD_COMPRESSION_FAILED;
            compressed = new UINT8[compressedSize];
            if (EfiCompress((UINT8*) data.constData(), data.size(), compressed, &compressedSize) != ERR_SUCCESS)
                return ERR_STANDARD_COMPRESSION_FAILED;
            compressedData = QByteArray((const char*) compressed, compressedSize);
            delete[] compressed;
            return ERR_SUCCESS;
        }
        break;
    case COMPRESSION_ALGORITHM_TIANO:
        {
            if (TianoCompress((UINT8*) data.constData(), data.size(), NULL, &compressedSize) != ERR_BUFFER_TOO_SMALL)
                return ERR_STANDARD_COMPRESSION_FAILED;
            compressed = new UINT8[compressedSize];
            if (TianoCompress((UINT8*) data.constData(), data.size(), compressed, &compressedSize) != ERR_SUCCESS)
                return ERR_STANDARD_COMPRESSION_FAILED;
            compressedData = QByteArray((const char*) compressed, compressedSize);
            delete[] compressed;
            return ERR_SUCCESS;
        }
        break;
    case COMPRESSION_ALGORITHM_LZMA:
        {
            if (LzmaCompress((const UINT8*) data.constData(), data.size(), NULL, &compressedSize) != ERR_BUFFER_TOO_SMALL)
                return ERR_CUSTOMIZED_COMPRESSION_FAILED;
            compressed = new UINT8[compressedSize];
            if (LzmaCompress((const UINT8*) data.constData(), data.size(), compressed, &compressedSize) != ERR_SUCCESS)
                return ERR_CUSTOMIZED_COMPRESSION_FAILED;
            compressedData = QByteArray((const char*) compressed, compressedSize);
            delete[] compressed;
            return ERR_SUCCESS;
        }
        break;
    case COMPRESSION_ALGORITHM_IMLZMA:
        {
            QByteArray header = data.left(sizeof(EFI_COMMON_SECTION_HEADER));
            EFI_COMMON_SECTION_HEADER* sectionHeader = (EFI_COMMON_SECTION_HEADER*) header.constData();
            UINT32 headerSize = sizeOfSectionHeaderOfType(sectionHeader->Type);
            header = data.left(headerSize);
            QByteArray newData = data.mid(headerSize);
            if (LzmaCompress((UINT8*) newData.constData(), newData.size(), NULL, &compressedSize) != ERR_BUFFER_TOO_SMALL)
                return ERR_CUSTOMIZED_COMPRESSION_FAILED;
            compressed = new UINT8[compressedSize];
            if (LzmaCompress((UINT8*) newData.constData(), newData.size(), compressed, &compressedSize) != ERR_SUCCESS)
                return ERR_CUSTOMIZED_COMPRESSION_FAILED;
            compressedData = header.append(QByteArray((const char*) compressed, compressedSize));
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
UINT8 FfsEngine::reconstructImage(QByteArray & reconstructed)
{
    QQueue<QByteArray> queue;
    UINT8 result = reconstruct(model->index(0,0), queue);
    if (result)
        return result;
    reconstructed.clear();
    while (!queue.isEmpty())
        reconstructed.append(queue.dequeue());
    return ERR_SUCCESS;
}

UINT8 FfsEngine::constructPadFile(const UINT32 size, const UINT8 revision, const UINT8 erasePolarity, QByteArray & pad)
{
    if (size < sizeof(EFI_FFS_FILE_HEADER) || erasePolarity == ERASE_POLARITY_UNKNOWN)
        return ERR_INVALID_PARAMETER;

    pad = QByteArray(size, erasePolarity == ERASE_POLARITY_TRUE ? '\xFF' : '\x00');
    EFI_FFS_FILE_HEADER* header = (EFI_FFS_FILE_HEADER*) pad.data();
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
    header->IntegrityCheck.Checksum.Header = calculateChecksum8((UINT8*) header, sizeof(EFI_FFS_FILE_HEADER) - 1);

    // Set data checksum
    if (revision == 1)
        header->IntegrityCheck.Checksum.File = FFS_FIXED_CHECKSUM;
    else
        header->IntegrityCheck.Checksum.File = FFS_FIXED_CHECKSUM2;

    return ERR_SUCCESS;
}

UINT8 FfsEngine::reconstruct(const QModelIndex & index, QQueue<QByteArray> & queue, const UINT8 revision, const UINT8 erasePolarity, const UINT32 base)
{
    if (!index.isValid())
        return ERR_SUCCESS;

    QByteArray reconstructed;
    UINT8 result;

    // No action is needed, just return header + body + tail
    if (!base && model->action(index) == NoAction) {
        reconstructed = model->header(index).append(model->body(index)).append(model->tail(index));
        queue.enqueue(reconstructed);
        return ERR_SUCCESS;
    }
    // Remove item
    if (model->action(index) ==  Remove) {
        // Volume can be removed by replacing all it's contents with empty bytes
        if (model->type(index) ==  Volume) {
            EFI_FIRMWARE_VOLUME_HEADER* volumeHeader = (EFI_FIRMWARE_VOLUME_HEADER*) model->header(index).constData();
            char empty = volumeHeader->Attributes & EFI_FVB_ERASE_POLARITY ? '\xFF' : '\x00';
            reconstructed.fill(empty, model->header(index).size() + model->body(index).size() + model->tail(index).size());
            queue.enqueue(reconstructed);
            return ERR_SUCCESS;
        }
        // File can be removed
        if (model->type(index) ==  File)
            // Add nothing to queue
                return ERR_SUCCESS;
        // Section can be removed
        else if (model->type(index) ==  Section)
            // Add nothing to queue
            return ERR_SUCCESS;
        // Other item types can't be removed
        else
            return ERR_NOT_IMPLEMENTED;
    }
    // Reconstruct item and it's children recursive
    if (base || model->action(index) == Create
          || model->action(index) == Insert
          || model->action(index) == Replace
          || model->action(index) == Rebuild) {
        QQueue<QByteArray> childrenQueue;

        switch (model->type(index)) {
        case  Image:
            if (model->subtype(index) ==  IntelImage) {
                // Reconstruct Intel image
                // First child will always be descriptor for this type of image
                result = reconstruct(index.child(0, index.column()), childrenQueue);
                if (result)
                    return result;
                QByteArray descriptor = childrenQueue.dequeue();
                reconstructed.append(descriptor);

                FLASH_DESCRIPTOR_MAP* descriptorMap = (FLASH_DESCRIPTOR_MAP*) (descriptor.constData() + sizeof(FLASH_DESCRIPTOR_HEADER));
                FLASH_DESCRIPTOR_REGION_SECTION* regionSection = (FLASH_DESCRIPTOR_REGION_SECTION*) calculateAddress8((UINT8*)descriptor.constData(), descriptorMap->RegionBase);
                QByteArray gbe;
                UINT32 gbeBegin = calculateRegionOffset(regionSection->GbeBase);
                UINT32 gbeEnd   = gbeBegin + calculateRegionSize(regionSection->GbeBase, regionSection->GbeLimit);
                QByteArray me;
                UINT32 meBegin = calculateRegionOffset(regionSection->MeBase);
                UINT32 meEnd   = meBegin + calculateRegionSize(regionSection->MeBase, regionSection->MeLimit);
                QByteArray bios;
                UINT32 biosBegin = calculateRegionOffset(regionSection->BiosBase);
                UINT32 biosEnd   = biosBegin + calculateRegionSize(regionSection->BiosBase, regionSection->BiosLimit);
                QByteArray pdr;
                UINT32 pdrBegin = calculateRegionOffset(regionSection->PdrBase);
                UINT32 pdrEnd   = pdrBegin + calculateRegionSize(regionSection->PdrBase, regionSection->PdrLimit);

                UINT32 offset = descriptor.size();
                // Reconstruct other regions
                char empty = '\xFF'; //!TODO: determine empty char using one of reserved descriptor fields
                for (int i = 1; i < model->rowCount(index); i++) {
                    result = reconstruct(index.child(i, index.column()), childrenQueue);
                    if (result)
                        return result;
                    switch(model->subtype(model->index(i, 0, index)))
                    {
                    case  GbeRegion:
                        gbe = childrenQueue.dequeue();
                        if (gbeBegin > offset)
                            reconstructed.append(QByteArray(gbeBegin - offset, empty));
                        reconstructed.append(gbe);
                        offset = gbeEnd;
                        break;
                    case  MeRegion:
                        me = childrenQueue.dequeue();
                        if (meBegin > offset)
                            reconstructed.append(QByteArray(meBegin - offset, empty));
                        reconstructed.append(me);
                        offset = meEnd;
                        break;
                    case  BiosRegion:
                        bios = childrenQueue.dequeue();
                        if (biosBegin > offset)
                            reconstructed.append(QByteArray(biosBegin - offset, empty));
                        reconstructed.append(bios);
                        offset = biosEnd;
                        break;
                    case  PdrRegion:
                        pdr = childrenQueue.dequeue();
                        if (pdrBegin > offset)
                            reconstructed.append(QByteArray(pdrBegin - offset, empty));
                        reconstructed.append(pdr);
                        offset = pdrEnd;
                        break;
                    default:
                        msg(tr("reconstruct: unknown region type found while reconstructing Intel image"), index);
                        return ERR_INVALID_REGION;
                    }
                }
                if ((UINT32)model->body(index).size() > offset)
                    reconstructed.append(QByteArray((UINT32)model->body(index).size() - offset, empty));

                // Check size of reconstructed image, it must be same
                if (reconstructed.size() > model->body(index).size()) {
                    msg(tr("reconstruct: reconstructed body %1 is bigger then original %2")
                        .arg(reconstructed.size(), 8, 16, QChar('0'))
                        .arg(model->body(index).size(), 8, 16, QChar('0')), index);
                    return ERR_INVALID_PARAMETER;
                }
                else if (reconstructed.size() < model->body(index).size()) {
                    msg(tr("reconstruct: reconstructed body %1 is smaller then original %2")
                        .arg(reconstructed.size(), 8, 16, QChar('0'))
                        .arg(model->body(index).size(), 8, 16, QChar('0')), index);
                    return ERR_INVALID_PARAMETER;
                }

                // Enqueue reconstructed item
                queue.enqueue(model->header(index).append(reconstructed));
                return ERR_SUCCESS;
            }
            // BIOS Image must be treated like region
        case  Capsule:
            if (model->subtype(index) ==  AptioCapsule)
                msg(tr("reconstruct: Aptio capsule checksum and signature can now become invalid"), index);
        case  Region:
            {
                // Reconstruct item body
                if (model->rowCount(index)) {
                    // Reconstruct item children
                    for (int i = 0; i < model->rowCount(index); i++) {
                        result = reconstruct(index.child(i, index.column()), childrenQueue);
                        if (result)
                            return result;
                    }

                    // No additional checks needed
                    while (!childrenQueue.isEmpty())
                        reconstructed.append(childrenQueue.dequeue());
                }
                // Use stored item body
                else
                    reconstructed = model->body(index);

                // Check size of reconstructed image, it must be same
                if (model->type(index) !=  Root) {
                    if (reconstructed.size() > model->body(index).size()) {
                        msg(tr("reconstructed: reconstructed body %1 is bigger then original %2")
                            .arg(reconstructed.size(), 8, 16, QChar('0'))
                            .arg(model->body(index).size(), 8, 16, QChar('0')), index);
                        return ERR_INVALID_PARAMETER;
                    }
                    else if (reconstructed.size() < model->body(index).size()) {
                        msg(tr("reconstructed: reconstructed body %1 is smaller then original %2")
                            .arg(reconstructed.size(), 8, 16, QChar('0'))
                            .arg(model->body(index).size(), 8, 16, QChar('0')), index);
                        return ERR_INVALID_PARAMETER;
                    }
                }

                // Enqueue reconstructed item
                queue.enqueue(model->header(index).append(reconstructed));
            }
            break;

        case  Volume:
            {
                //!TODO: add check for weak aligned volume
                QByteArray header = model->header(index);
                EFI_FIRMWARE_VOLUME_HEADER* volumeHeader = (EFI_FIRMWARE_VOLUME_HEADER*) header.data();

                // Recalculate volume header checksum
                volumeHeader->Checksum = 0;
                volumeHeader->Checksum = calculateChecksum16((UINT16*) volumeHeader, volumeHeader->HeaderLength);

                // Get volume size
                UINT32 volumeSize;
                result = getVolumeSize(header, 0, volumeSize);
                if (result)
                    return result;

                // Reconstruct volume body
                if (model->rowCount(index)) {
                    UINT8 polarity = volumeHeader->Attributes & EFI_FVB_ERASE_POLARITY ? ERASE_POLARITY_TRUE : ERASE_POLARITY_FALSE;
                    char empty = volumeHeader->Attributes & EFI_FVB_ERASE_POLARITY ? '\xFF' : '\x00';
                    bool bootVolume = false;

                    // Check for boot volume
                    for (int i = 0; i < model->rowCount(index); i++) {
                        QByteArray hdr = model->header(index.child(i, index.column()));
                        EFI_FFS_FILE_HEADER* fileHeader = (EFI_FFS_FILE_HEADER*) hdr.data();
                        if (fileHeader->Type == EFI_FV_FILETYPE_PEI_CORE) {
                            bootVolume = true;
                            break;
                        }
                    }

                    // Reconstruct files in volume
                    UINT32 offset = 0;
                    QByteArray vtf;
                    QModelIndex vtfIndex;
                    QByteArray amiBeforeVtf;
                    QModelIndex amiBeforeVtfIndex;
                    for (int i = 0; i < model->rowCount(index); i++) {
                        // Align to 8 byte boundary
                        UINT32 alignment = offset % 8;
                        if (alignment) {
                            alignment = 8 - alignment;
                            offset += alignment;
                            reconstructed.append(QByteArray(alignment, empty));
                        }
						
                        // Calculate file base
                        UINT32 newbase = 0;
                        if (bootVolume) {
                            newbase = (UINT32) 0x100000000 - volumeSize + header.size() + offset;
						}

						// Reconstruct file
                        result = reconstruct(index.child(i, index.column()), childrenQueue, volumeHeader->Revision, polarity, newbase);
                        if (result)
                            return result;

						// Check for empty queue
						if (childrenQueue.isEmpty())
							continue;
                        
						// Get file from queue
                        QByteArray file = childrenQueue.dequeue();
                        EFI_FFS_FILE_HEADER* fileHeader = (EFI_FFS_FILE_HEADER*) file.data();

                        // Pad file
                        if (fileHeader->Type == EFI_FV_FILETYPE_PAD)
                            continue;

						// AMI file before VTF
                        if (file.left(sizeof(EFI_GUID)) == EFI_AMI_FFS_FILE_BEFORE_VTF_GUID) {
                            amiBeforeVtf = file;
                            amiBeforeVtfIndex = index.child(i, index.column());
                            continue;
                        }

                        // Volume Top File
                        if (file.left(sizeof(EFI_GUID)) == EFI_FFS_VOLUME_TOP_FILE_GUID) {
                            vtf = file;
                            vtfIndex = index.child(i, index.column());
                            continue;
                        }
						                        
                        // Normal file
						// Ensure correct alignment
                        UINT8 alignmentPower;
                        UINT32 alignmentBase;
                        alignmentPower = ffsAlignmentTable[(fileHeader->Attributes & FFS_ATTRIB_DATA_ALIGNMENT) >> 3];
                        alignment = (UINT32) pow(2.0, alignmentPower);
                        alignmentBase = header.size() + offset + sizeof(EFI_FFS_FILE_HEADER);
                        if (alignmentBase % alignment) {
                            // File will be unaligned if added as is, so we must add pad file before it
                            // Determine pad file size
                            UINT32 size = alignment - (alignmentBase % alignment);
                            // Required padding is smaler then minimal pad file size
                            while (size < sizeof(EFI_FFS_FILE_HEADER)) {
                                size += alignment;
                            }
                            // Construct pad file
                            QByteArray pad;
                            result = constructPadFile(size, revision, polarity, pad);
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

                    // Insert AMI file before VTF to it's correct place
                    if (!amiBeforeVtf.isEmpty()) {
                        // Determine correct offset
                        UINT32 amiOffset = volumeSize - header.size() - amiBeforeVtf.size() - EFI_AMI_FFS_FILE_BEFORE_VTF_OFFSET;

                        // Insert pad file to fill the gap
                        if (amiOffset > offset) {
                            // Determine pad file size
                            UINT32 size = amiOffset - offset;
                            // Construct pad file
                            QByteArray pad;
                            result = constructPadFile(size, revision, polarity, pad);
                            if (result)
                                return result;
                            // Append constructed pad file to volume body
                            reconstructed.append(pad);
                            offset = amiOffset;
                        }
                        if (amiOffset < offset) {
                            msg(tr("reconstruct: %1: volume has no free space left").arg(guidToQString(volumeHeader->FileSystemGuid)), index);
                            return ERR_INVALID_VOLUME;
                        }

                        // Calculate file base
                        UINT32 newbase = 0;
                        if (bootVolume) {
                            newbase = (UINT32) 0x100000000 - volumeSize + header.size() + amiOffset;
                        }

                        // Reconstruct file again
                        result = reconstruct(amiBeforeVtfIndex, childrenQueue, volumeHeader->Revision, polarity, newbase);
                        if (result)
                            return result;

                        // Get file from queue
                        amiBeforeVtf = childrenQueue.dequeue();

                        // Append AMI file before VTF
                        reconstructed.append(amiBeforeVtf);

						// Change current file offset
                        offset += amiBeforeVtf.size();

                        // Align to 8 byte boundary
                        UINT32 alignment = offset % 8;
                        if (alignment) {
                            alignment = 8 - alignment;
                            offset += alignment;
                            reconstructed.append(QByteArray(alignment, empty));
                        }
                    }

                    // Insert VTF to it's correct place
                    if (!vtf.isEmpty()) {
                        // Determine correct VTF offset
                        UINT32 vtfOffset = volumeSize - header.size() - vtf.size();

                        if (vtfOffset % 8) {
                            msg(tr("reconstruct: %1: Wrong size of Volume Top File")
                                .arg(guidToQString(volumeHeader->FileSystemGuid)), index);
                            return ERR_INVALID_FILE;
                        }
                        // Insert pad file to fill the gap
                        if (vtfOffset > offset) {
                            // Determine pad file size
                            UINT32 size = vtfOffset - offset;
                            // Construct pad file
                            QByteArray pad;
                            result = constructPadFile(size, revision, polarity, pad);
                            if (result)
                                return result;
                            // Append constructed pad file to volume body
                            reconstructed.append(pad);
                            offset = vtfOffset;
                        }
                        // No more space left in volume
                        else if (vtfOffset < offset) {
                            msg(tr("reconstruct: %1: volume has no free space left").arg(guidToQString(volumeHeader->FileSystemGuid)), index);
                            return ERR_INVALID_VOLUME;
                        }

                        // Calculate file base
                        UINT32 newbase = 0;
                        if (bootVolume) {
                            newbase = (UINT32) 0x100000000 - volumeSize + header.size() + vtfOffset;
                        }

                        // Reconstruct file
                        result = reconstruct(vtfIndex, childrenQueue, volumeHeader->Revision, polarity, newbase);
                        if (result)
                            return result;

                        // Get file from queue
                        vtf = childrenQueue.dequeue();

						// Append VTF
						reconstructed.append(vtf);
                    }
					else {
						// Fill the rest of volume space with empty char
						UINT32 volumeBodySize = volumeSize - header.size();
						if (volumeBodySize > (UINT32) reconstructed.size()) {
							// Fill volume end with empty char
							reconstructed.append(QByteArray(volumeBodySize - reconstructed.size(), empty));
						}
						else if (volumeBodySize < (UINT32) reconstructed.size()) {
							// Check if volume can be grown
							// Root volume can't be grown yet
							UINT8 parentType = model->type(index.parent());
							if(parentType != File && parentType != Section) {
								msg(tr("reconstruct: %1: can't grow root volume").arg(guidToQString(volumeHeader->FileSystemGuid)), index);
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
                        msg(tr("reconstruct: Volume grow failed"));
                        return ERR_INVALID_VOLUME;
                    }
                }
                // Use current volume body
                else
                    reconstructed = model->body(index);

                // Enqueue reconstructed item
                queue.enqueue(header.append(reconstructed));
            }
            break;

        case  File:
            {
                QByteArray header = model->header(index);
                EFI_FFS_FILE_HEADER* fileHeader = (EFI_FFS_FILE_HEADER*) header.data();

                // Check erase polarity
                if (erasePolarity == ERASE_POLARITY_UNKNOWN) {
                    msg(tr("reconstruct: %1, unknown erase polarity").arg(guidToQString(fileHeader->Name)), index);
                    return ERR_INVALID_PARAMETER;
                }

                // Construct empty char for this file
                char empty = (erasePolarity == ERASE_POLARITY_TRUE ? '\xFF' : '\x00');

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
                    msg(tr("reconstruct: %1, file is HEADER_INVALID state, and will be removed from reconstructed image")
                        .arg(guidToQString(fileHeader->Name)), index);
                    return ERR_SUCCESS;
                }
                else if (state & EFI_FILE_DELETED) {
                    // File marked to have been deleted form and must be deleted
                    // Do not add anything to queue
                    msg(tr("reconstruct: %1, file is in DELETED state, and will be removed from reconstructed image")
                        .arg(guidToQString(fileHeader->Name)), index);
                    return ERR_SUCCESS;
                }
                else if (state & EFI_FILE_MARKED_FOR_UPDATE) {
                    // File is marked for update, the mark must be removed
                    msg(tr("reconstruct: %1, file MARKED_FOR_UPDATE state cleared")
                        .arg(guidToQString(fileHeader->Name)), index);
                }
                else if (state & EFI_FILE_DATA_VALID) {
                    // File is in good condition, reconstruct it
                }
                else if (state & EFI_FILE_HEADER_VALID) {
                    // Header is valid, but data is not, so file must be deleted
                    msg(tr("reconstruct: %1, file is in HEADER_VALID (but not in DATA_VALID) state, and will be removed from reconstructed image")
                        .arg(guidToQString(fileHeader->Name)), index);
                    return ERR_SUCCESS;
                }
                else if (state & EFI_FILE_HEADER_CONSTRUCTION) {
                    // Header construction not finished, so file must be deleted
                    msg(tr("reconstruct: %1, file is in HEADER_CONSTRUCTION (but not in DATA_VALID) state, and will be removed from reconstructed image")
                        .arg(guidToQString(fileHeader->Name)), index);
                    return ERR_SUCCESS;
                }

                // Reconstruct file body
                if (model->rowCount(index)) {
                    // Construct new file body
                    UINT32 offset = 0;
                    for (int i = 0; i < model->rowCount(index); i++) {
                        // Align to 4 byte boundary
                        UINT8 alignment = offset % 4;
                        if (alignment) {
                            alignment = 4 - alignment;
                            offset += alignment;
                            reconstructed.append(QByteArray(alignment, empty));
                        }

                        // Correct base, if needed
                        UINT32 newbase = 0;
                        if (base) {
							newbase = base + header.size() + offset;
                        }

                        // Reconstruct section
                        result = reconstruct(index.child(i, index.column()), childrenQueue, revision, empty, newbase);
                        if (result)
                            return result;

                        // Check for empty queue
                        if (childrenQueue.isEmpty())
                            continue;

                        // Get section from queue
                        QByteArray section = childrenQueue.dequeue();

                        // Append current section to new file body
                        reconstructed.append(section);

                        // Change current file offset
                        offset += section.size();
                    }

                    // Correct file size
                    UINT8 tailSize = model->hasEmptyTail(index) ? 0 : sizeof(UINT16);

                    uint32ToUint24(sizeof(EFI_FFS_FILE_HEADER) + reconstructed.size() + tailSize, fileHeader->Size);

                    // Recalculate header checksum
                    fileHeader->IntegrityCheck.Checksum.Header = 0;
                    fileHeader->IntegrityCheck.Checksum.File = 0;
                    fileHeader->IntegrityCheck.Checksum.Header = calculateChecksum8((UINT8*) fileHeader, sizeof(EFI_FFS_FILE_HEADER) - 1);

                }
                // Use current file body
                else
                    reconstructed = model->body(index);

                // Recalculate data checksum, if needed
                if (fileHeader->Attributes & FFS_ATTRIB_CHECKSUM) {
                    fileHeader->IntegrityCheck.Checksum.File = calculateChecksum8((UINT8*) reconstructed.constData(), reconstructed.size());
                }
                else if (revision == 1)
                    fileHeader->IntegrityCheck.Checksum.File = FFS_FIXED_CHECKSUM;
                else
                    fileHeader->IntegrityCheck.Checksum.File = FFS_FIXED_CHECKSUM2;

                // Append tail, if needed
                if (!model->hasEmptyTail(index))
                    reconstructed.append(~fileHeader->IntegrityCheck.TailReference);

                // Set file state
                state = EFI_FILE_DATA_VALID | EFI_FILE_HEADER_VALID | EFI_FILE_HEADER_CONSTRUCTION;
                if (erasePolarity == ERASE_POLARITY_TRUE)
                    state = ~state;
                fileHeader->State = state;

                // Enqueue reconstructed item
                queue.enqueue(header.append(reconstructed));
            }
            break;

        case  Section:
            {
                QByteArray header = model->header(index);
                EFI_COMMON_SECTION_HEADER* commonHeader = (EFI_COMMON_SECTION_HEADER*) header.data();

                // Section with children
                if (model->rowCount(index)) {
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

                        // No rebase needed for subsections
                        /*// Correct base, if needed
                        UINT32 newbase = 0;
                        if (base) {
                            newbase += offset;
                        }*/

                        // Reconstruct subsections
                        result = reconstruct(index.child(i, index.column()), childrenQueue/*, revision, erasePolarity, newbase*/);
                        if (result)
                            return result;

                        // Check for empty queue
                        if (childrenQueue.isEmpty())
                            continue;

                        // Get section from queue
                        QByteArray section = childrenQueue.dequeue();

                        // Append current subsection to new section body
                        reconstructed.append(section);

                        // Change current file offset
                        offset += section.size();
                    }

                    // Only this 2 sections can have compressed body
                    if (model->subtype(index) == EFI_SECTION_COMPRESSION) {
                        EFI_COMPRESSION_SECTION* compessionHeader = (EFI_COMPRESSION_SECTION*) header.data();
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
                        EFI_GUID_DEFINED_SECTION* guidDefinedHeader = (EFI_GUID_DEFINED_SECTION*) header.data();
                        // Compress new section body
                        QByteArray compressed;
                        result = compress(reconstructed, model->compression(index), compressed);
                        if (result)
                            return result;
                        // Check for auth status valid attribute
                        if (guidDefinedHeader->Attributes & EFI_GUIDED_SECTION_AUTH_STATUS_VALID) {
                            msg(tr("reconstruct: %1: GUID defined section signature can now become invalid")
                                .arg(guidToQString(guidDefinedHeader->SectionDefinitionGuid)));
                        }
                        // Replace new section body
                        reconstructed = compressed;
                    }
                    else if (model->compression(index) != COMPRESSION_ALGORITHM_NONE) {
                        msg(tr("reconstruct: incorrectly required compression for section of type %1")
                            .arg(model->subtype(index)));
                        return ERR_INVALID_SECTION;
                    }

                    // Correct section size
                    uint32ToUint24(header.size() + reconstructed.size(), commonHeader->Size);
                }
                // Leaf section
                else
                    reconstructed = model->body(index);

				// Rebase executable section, if needed
				if (base && (model->subtype(index) == EFI_SECTION_PE32 || model->subtype(index) == EFI_SECTION_TE)) {
					result = rebase(reconstructed, base + header.size());
					if (result) {
						msg(tr("reconstruct: can't rebase executable"));
						return result;
					}
				}

                // Enqueue reconstructed item
                queue.enqueue(header.append(reconstructed));
            }
            break;

        default:
            msg(tr("reconstruct: Unknown item type (%1)").arg(model->type(index)));
            return ERR_UNKNOWN_ITEM_TYPE;
        }

        return ERR_SUCCESS;
    }

    return ERR_NOT_IMPLEMENTED;
}

UINT8 FfsEngine::growVolume(QByteArray & header, const UINT32 size, UINT32 & newSize)
{
    // Adjust new size to be representable by current FvBlockMap
    EFI_FIRMWARE_VOLUME_HEADER* volumeHeader = (EFI_FIRMWARE_VOLUME_HEADER*) header.data();
    EFI_FV_BLOCK_MAP_ENTRY* blockMap = (EFI_FV_BLOCK_MAP_ENTRY*) (header.data() + sizeof(EFI_FIRMWARE_VOLUME_HEADER));

    // Get block map size
    UINT32 extHeaderOffset = volumeHeader->Revision == 2 ? volumeHeader->ExtHeaderOffset : 0;
    UINT32 blockMapSize = header.size() - extHeaderOffset - sizeof(EFI_FIRMWARE_VOLUME_HEADER);
    if (blockMapSize % sizeof(EFI_FV_BLOCK_MAP_ENTRY))
        return ERR_INVALID_VOLUME;
    UINT32 blockMapCount = blockMapSize / sizeof(EFI_FV_BLOCK_MAP_ENTRY);

    // Check blockMap validity
    if (blockMap[blockMapCount-1].NumBlocks != 0 || blockMap[blockMapCount-1].Length != 0)
        return ERR_INVALID_VOLUME;

    // Calculate new size
    if (newSize <= size)
        return ERR_INVALID_PARAMETER;
    newSize += blockMap->Length - newSize % blockMap->Length;

    // Recalculate number of blocks
    blockMap->NumBlocks = newSize / blockMap->Length;

    // Set new volume size
    volumeHeader->FvLength = 0;
    for(UINT8 i = 0; i < blockMapCount; i++) {
        volumeHeader->FvLength += blockMap[i].NumBlocks * blockMap[i].Length;
    }

    // Recalculate volume header checksum
    volumeHeader->Checksum = 0;
    volumeHeader->Checksum = calculateChecksum16((UINT16*) volumeHeader, volumeHeader->HeaderLength);

    return ERR_SUCCESS;
}

// Search routines
UINT8 FfsEngine::findHexPattern(const QByteArray & pattern, const UINT8 mode)
{
    return findHexPatternIn(model->index(0,0), pattern, mode);
}

UINT8 FfsEngine::findHexPatternIn(const QModelIndex & index, const QByteArray & pattern, const UINT8 mode)
{
    if (pattern.isEmpty())
        return ERR_INVALID_PARAMETER;

    if (!index.isValid())
        return ERR_SUCCESS;

    bool hasChildren = (model->rowCount(index) > 0);
    for (int i = 0; i < model->rowCount(index); i++) {
        findHexPatternIn(index.child(i, index.column()), pattern, mode);
    }

    QByteArray data;
    if (hasChildren) {
        if(mode != SEARCH_MODE_BODY)
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

    int offset = -1;
    while ((offset = data.indexOf(pattern, offset + 1)) >= 0) {
        msg(tr("Hex pattern \"%1\" found in %2 at offset %3")
            .arg(QString(pattern.toHex()))
            .arg(model->nameString(index))
            .arg(offset, 8, 16, QChar('0')),
            index);
    }

    return ERR_SUCCESS;
}

UINT8 FfsEngine::findTextPattern(const QString & pattern, const bool unicode, const Qt::CaseSensitivity caseSensitive)
{
    return findTextPatternIn(model->index(0,0), pattern, unicode, caseSensitive);
}

UINT8 FfsEngine::findTextPatternIn(const QModelIndex & index, const QString & pattern, const bool unicode, const Qt::CaseSensitivity caseSensitive)
{
    if (pattern.isEmpty())
        return ERR_INVALID_PARAMETER;

    if (!index.isValid())
        return ERR_SUCCESS;

    bool hasChildren = (model->rowCount(index) > 0);
    for (int i = 0; i < model->rowCount(index); i++) {
        findTextPatternIn(index.child(i, index.column()), pattern, unicode, caseSensitive);
    }

    if (hasChildren)
        return ERR_SUCCESS;

    QString data;
    if (unicode)
        data = QString::fromUtf16((const ushort*) model->body(index).data(), model->body(index).length()/2);
    else
        data = QString::fromLatin1((const char*) model->body(index).data(), model->body(index).length());

    int offset = -1;
    while ((offset = data.indexOf(pattern, offset + 1, caseSensitive)) >= 0) {
        msg(tr("%1 text pattern \"%2\" found in %3 at offset %4")
            .arg(unicode ? "Unicode" : "ASCII")
            .arg(pattern)
            .arg(model->nameString(index))
            .arg(unicode ? offset*2 : offset, 8, 16, QChar('0')),
            index);
    }

    return ERR_SUCCESS;
}

UINT8 FfsEngine::rebase(QByteArray &executable, const UINT32 base)
{
    // Relocation data
    UINT32 delta; // Delta between old and new bases
    UINT32 relocOffset; // Offset of relocation region
    UINT32 relocSize;   // Size of relocation region
    UINT32 teFixup = 0;
    // Copy input data to local storage
    QByteArray file = executable;

    // Populate DOS header
    EFI_IMAGE_DOS_HEADER* dosHeader = (EFI_IMAGE_DOS_HEADER*) file.data();

    // Check signature
    if (dosHeader->e_magic == EFI_IMAGE_DOS_SIGNATURE){
        UINT32 offset = dosHeader->e_lfanew;
        EFI_IMAGE_PE_HEADER* peHeader = (EFI_IMAGE_PE_HEADER*) (file.data() + offset);
        if (peHeader->Signature != EFI_IMAGE_PE_SIGNATURE)
            return ERR_UNKNOWN_IMAGE_TYPE;
        offset += sizeof(EFI_IMAGE_PE_HEADER);
        // Skip file header
        offset += sizeof(EFI_IMAGE_FILE_HEADER);
        // Check optional header magic
        UINT16 magic = *(UINT16*) (file.data() + offset);
        if (magic == EFI_IMAGE_PE_OPTIONAL_HDR32_MAGIC) {
            EFI_IMAGE_OPTIONAL_HEADER32* optHeader = (EFI_IMAGE_OPTIONAL_HEADER32*) (file.data() + offset);
            // Get relocation data
            delta = base - optHeader->ImageBase;
			if (!delta)
				// No need to rebase
				return ERR_SUCCESS;
            relocOffset = optHeader->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress;
            relocSize   = optHeader->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;
            // Set new base
            optHeader->ImageBase = base;
        }
        else if (magic == EFI_IMAGE_PE_OPTIONAL_HDR32_MAGIC) {
            EFI_IMAGE_OPTIONAL_HEADER64* optHeader = (EFI_IMAGE_OPTIONAL_HEADER64*) (file.data() + offset);
            // Get relocation data
            delta = base - optHeader->ImageBase;
			if (!delta)
				// No need to rebase
				return ERR_SUCCESS;
            relocOffset = optHeader->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress;
            relocSize   = optHeader->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;
            // Set new base
            optHeader->ImageBase = base;
            }
        else
            return ERR_UNKNOWN_PE_OPTIONAL_HEADER_TYPE;
    }
    else if (dosHeader->e_magic == EFI_IMAGE_TE_SIGNATURE){
        // Populate TE header
        EFI_IMAGE_TE_HEADER* teHeader = (EFI_IMAGE_TE_HEADER*) file.data();
        // Get relocation data
        delta = base - teHeader->ImageBase;
		if (!delta)
			// No need to rebase
			return ERR_SUCCESS;
        relocOffset = teHeader->DataDirectory[EFI_IMAGE_TE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress ;
        teFixup     = teHeader->StrippedSize - sizeof(EFI_IMAGE_TE_HEADER);
        relocSize   = teHeader->DataDirectory[EFI_IMAGE_TE_DIRECTORY_ENTRY_BASERELOC].Size;
        // Set new base
        teHeader->ImageBase = base;
    }
    else
        return ERR_UNKNOWN_IMAGE_TYPE;
	
	// No relocations
	if (relocOffset == 0)
		// No need to rebase
		return ERR_SUCCESS;

    // Locate relocation section using data directory
    EFI_IMAGE_BASE_RELOCATION   *RelocBase;
    EFI_IMAGE_BASE_RELOCATION   *RelocBaseEnd;
    UINT16                      *Reloc;
    UINT16                      *RelocEnd;
    UINT16                      *F16;
    UINT32                      *F32;
    UINT64                      *F64;

    // Run the whole relocation block
    RelocBase = (EFI_IMAGE_BASE_RELOCATION*) (file.data() + relocOffset - teFixup);
    RelocBaseEnd = (EFI_IMAGE_BASE_RELOCATION*) (file.data() + relocOffset - teFixup + relocSize);

    while (RelocBase < RelocBaseEnd) {
        Reloc = (UINT16*) ((UINT8*) RelocBase + sizeof(EFI_IMAGE_BASE_RELOCATION));
        RelocEnd = (UINT16*) ((UINT8*) RelocBase + RelocBase->SizeOfBlock);

        // Run this relocation record
        while (Reloc < RelocEnd) {
            UINT8* data = (UINT8*) (file.data() + RelocBase->VirtualAddress - teFixup + (*Reloc & 0x0FFF));
            switch ((*Reloc) >> 12) {
            case EFI_IMAGE_REL_BASED_ABSOLUTE:
                // Do nothing
                break;

            case EFI_IMAGE_REL_BASED_HIGH:
                // Add second 16 bits of delta
                F16  = (UINT16*) data;
                *F16 = (UINT16)(*F16 + (UINT16)(((UINT32) delta) >> 16));
                break;

            case EFI_IMAGE_REL_BASED_LOW:
                // Add first 16 bits of delta
                F16  = (UINT16*) data;
                *F16 = (UINT16) (*F16 + (UINT16) delta);
                break;

            case EFI_IMAGE_REL_BASED_HIGHLOW:
                // Add first 32 bits of delta
                F32  = (UINT32*) data;
                *F32 = *F32 + (UINT32) delta;
                break;

            case EFI_IMAGE_REL_BASED_DIR64:
                // Add all 64 bits of delta
                F64  = (UINT64*) data;
                *F64 = *F64 + (UINT64) delta;
                break;

            default:
                return ERR_UNKNOWN_RELOCATION_TYPE;
            }

            // Next reloc record
            Reloc += 1;
        }

        // Next reloc block
        RelocBase = (EFI_IMAGE_BASE_RELOCATION*)RelocEnd;
    }

    executable = file;
    return ERR_SUCCESS;
}
