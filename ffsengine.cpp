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
    rootItem = new TreeItem(TreeItem::Root);
    rootItem->setName("Name");
    rootItem->setTypeName("Type");
    rootItem->setSubtypeName("Subtype");
    rootItem->setText("Text");
    treeModel = new TreeModel(rootItem);
}

FfsEngine::~FfsEngine(void)
{
    delete treeModel;
    delete rootItem;
}

TreeModel* FfsEngine::model() const
{
    return treeModel;
}

QString FfsEngine::message() const
{
    return text;
}

void FfsEngine::msg(const QString & message)
{
    text.append(message).append("\n");
}

QByteArray FfsEngine::header(const QModelIndex& index) const
{
    if (!index.isValid())
        return QByteArray();

    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    return item->header();
}

QByteArray FfsEngine::body(const QModelIndex& index) const
{
    if (!index.isValid())
        return QByteArray();

    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    return item->body();
}

bool FfsEngine::hasEmptyHeader(const QModelIndex& index) const
{
    if (!index.isValid())
        return true;

    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());

    return item->hasEmptyHeader();
}

bool FfsEngine::hasEmptyBody(const QModelIndex& index) const
{
    if (!index.isValid())
        return true;

    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());

    return item->hasEmptyBody();
}

bool FfsEngine::setTreeItemName(const QString &data, const QModelIndex &index)
{
    if(!index.isValid())
        return false;

    return treeModel->setItemName(data, index);
}

bool FfsEngine::setTreeItemText(const QString &data, const QModelIndex &index)
{
    if(!index.isValid())
        return false;

    return treeModel->setItemText(data, index);
}

QModelIndex FfsEngine::findParentOfType(UINT8 type, const QModelIndex& index) const
{
    if(!index.isValid())
        return QModelIndex();

    TreeItem *item;
    QModelIndex parent = index;

    for(item = static_cast<TreeItem*>(parent.internalPointer()); 
        item != NULL && item != rootItem && item->type() != type;
        item = static_cast<TreeItem*>(parent.internalPointer()))
        parent = parent.parent();
    if (item != NULL && item != rootItem)
        return parent;

    return QModelIndex();
}

bool FfsEngine::isOfType(UINT8 type, const QModelIndex & index) const
{
    if (!index.isValid())
        return false;

    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    return (item->type() == type);
}

bool FfsEngine::isOfSubtype(UINT8 subtype, const QModelIndex & index) const
{
    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    return (item->subtype() == subtype);
}

// Firmware image parsing
UINT8  FfsEngine::parseInputFile(const QByteArray & buffer)
{
    UINT32                   capsuleHeaderSize = 0;
    FLASH_DESCRIPTOR_HEADER* descriptorHeader = NULL;
    QByteArray               flashImage;
    QByteArray               bios;
    QModelIndex              index;
    QString                  name;
    QString                  info;

    // Check buffer size to be more or equal then sizeof(EFI_CAPSULE_HEADER)
    if (buffer.size() <= sizeof(EFI_CAPSULE_HEADER))
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
        name = tr("UEFI capsule");
        info = tr("Header size: %1\nFlags: %2\nImage size: %3")
            .arg(capsuleHeader->HeaderSize, 8, 16, QChar('0'))
            .arg(capsuleHeader->Flags, 8, 16, QChar('0'))
            .arg(capsuleHeader->CapsuleImageSize, 8, 16, QChar('0'));
        // Add tree item
        index = treeModel->addItem(TreeItem::Capsule, TreeItem::UefiCapsule, COMPRESSION_ALGORITHM_NONE, name, "", info, header, body);
    }

    // Check buffer for being extended Aptio capsule header
    else if (buffer.startsWith(APTIO_CAPSULE_GUID)) {
        // Get info
        APTIO_CAPSULE_HEADER* aptioCapsuleHeader = (APTIO_CAPSULE_HEADER*) buffer.constData();
        capsuleHeaderSize = aptioCapsuleHeader->RomImageOffset;
        QByteArray header = buffer.left(capsuleHeaderSize);
        QByteArray body   = buffer.right(buffer.size() - capsuleHeaderSize);
        name = tr("AMI Aptio capsule");
        info = tr("Header size: %1\nFlags: %2\nImage size: %3")
            .arg(aptioCapsuleHeader->RomImageOffset, 4, 16, QChar('0'))
            .arg(aptioCapsuleHeader->CapsuleHeader.Flags, 8, 16, QChar('0'))
            .arg(aptioCapsuleHeader->CapsuleHeader.CapsuleImageSize - aptioCapsuleHeader->RomImageOffset, 8, 16, QChar('0'));
        //!TODO: more info about Aptio capsule
        // Add tree item
        index = treeModel->addItem(TreeItem::Capsule, TreeItem::AptioCapsule, COMPRESSION_ALGORITHM_NONE, name, "", info, header, body);
    }

    // Skip capsule header to have flash chip image
    flashImage = buffer.right(buffer.size() - capsuleHeaderSize);

    // Check buffer for being Intel flash descriptor
    descriptorHeader = (FLASH_DESCRIPTOR_HEADER*) flashImage.constData();
    // Check descriptor signature
    if (descriptorHeader->Signature == FLASH_DESCRIPTOR_SIGNATURE) {
        FLASH_DESCRIPTOR_MAP*               descriptorMap;
        FLASH_DESCRIPTOR_REGION_SECTION*    regionSection;

        // Store the beginning of descriptor as descriptor base address
        UINT8* descriptor  = (UINT8*) flashImage.constData();

        // Check for buffer size to be greater or equal to descriptor region size
        if (flashImage.size() < FLASH_DESCRIPTOR_SIZE) {
            msg(tr("parseInputFile: Input file is smaller then minimum descriptor size of 4096 bytes"));
            return ERR_INVALID_FLASH_DESCRIPTOR;
        }

        // Parse descriptor map
        descriptorMap = (FLASH_DESCRIPTOR_MAP*) (descriptor + sizeof(FLASH_DESCRIPTOR_HEADER));
        regionSection = (FLASH_DESCRIPTOR_REGION_SECTION*) calculateAddress8(descriptor, descriptorMap->RegionBase);

        // Get info
        QByteArray header = flashImage.left(sizeof(FLASH_DESCRIPTOR_HEADER));
        QByteArray body   = flashImage.mid(sizeof(FLASH_DESCRIPTOR_HEADER), FLASH_DESCRIPTOR_SIZE - sizeof(FLASH_DESCRIPTOR_HEADER));
        name = tr("Descriptor");
        info = tr("Flash chips: %1\nRegions: %2\nMasters: %3\nPCH straps:%4\nPROC straps: %5\nICC table entries: %6")
            .arg(descriptorMap->NumberOfFlashChips + 1) //
            .arg(descriptorMap->NumberOfRegions + 1)    // Zero-based numbers in storage
            .arg(descriptorMap->NumberOfMasters + 1)    //
            .arg(descriptorMap->NumberOfPchStraps)
            .arg(descriptorMap->NumberOfProcStraps)
            .arg(descriptorMap->NumberOfIccTableEntries);
        //!TODO: more info about descriptor
        // Add tree item
        index = treeModel->addItem(TreeItem::Descriptor, 0, COMPRESSION_ALGORITHM_NONE, name, "", info, header, body);

        // Parse regions
        QModelIndex regionIndex;
        // Parse non-BIOS regions
        parseRegion(flashImage, TreeItem::GbeRegion,  regionSection->GbeBase,  regionSection->GbeLimit,  index, regionIndex);
        parseRegion(flashImage, TreeItem::MeRegion,   regionSection->MeBase,   regionSection->MeLimit,   index, regionIndex);
        parseRegion(flashImage, TreeItem::PdrRegion,  regionSection->PdrBase,  regionSection->PdrLimit,  index, regionIndex);
        // Parse BIOS region
        UINT8 result = parseRegion(flashImage, TreeItem::BiosRegion, regionSection->BiosBase, regionSection->BiosLimit, index, regionIndex);
        // Exit if no BIOS region found
        if (result != ERR_SUCCESS) {
            msg(tr("parseInputFile: BIOS region not found"));
            return ERR_BIOS_REGION_NOT_FOUND;
        }

        index = regionIndex;
        bios = QByteArray((const char*)(descriptor + calculateRegionOffset(regionSection->BiosBase)), calculateRegionSize(regionSection->BiosBase, regionSection->BiosLimit));
    }
    else {
        bios = buffer;
    }

    // We are in the beginning of BIOS space, where firmware volumes are
    // Parse BIOS space

    return parseBios(bios, index);
}

UINT8 FfsEngine::parseRegion(const QByteArray & flashImage, UINT8 regionSubtype, const UINT16 regionBase, const UINT16 regionLimit, const QModelIndex & parent, QModelIndex & regionIndex)
{
    // Check for empty region or flash image
    if (!regionLimit || flashImage.size() <= 0)
        return ERR_EMPTY_REGION;

    // Storing flash image size to unsigned variable, because it can't be negative now and all other values are unsigned  
    UINT32 flashImageSize = (UINT32) flashImage.size(); 

    // Calculate region offset and size
    UINT32 regionOffset = calculateRegionOffset(regionBase);
    UINT32 regionSize   = calculateRegionSize(regionBase, regionLimit);

    // Populate descriptor map 
    FLASH_DESCRIPTOR_MAP* descriptor_map = (FLASH_DESCRIPTOR_MAP*) (flashImage.constData() + sizeof(FLASH_DESCRIPTOR_HEADER));

    // Determine presence of 2 flash chips
    bool twoChips = descriptor_map->NumberOfFlashChips;

    // Construct region name
    QString regionName = regionTypeToQString(regionSubtype);

    // Check region base to be in buffer
    if (regionOffset >= flashImageSize)
    {
        msg(tr("parseRegion: %1 region stored in descriptor not found").arg(regionName));
        if (twoChips) 
            msg(tr("Two flash chips installed, so it could be in another flash chip\n"
            "Make a dump from another flash chip and open it to view information about %1 region").arg(regionName));
        else
            msg(tr("One flash chip installed, so it is an error caused by damaged or incomplete dump"));
        msg(tr("Absence of %1 region assumed").arg(regionName));
        return ERR_INVALID_REGION;
    }

    // Check region to be fully present in buffer
    else if (regionOffset + regionSize > flashImageSize)
    {
        msg(tr("parseRegion: %1 region stored in descriptor overlaps the end of opened file").arg(regionName));
        if (twoChips) 
            msg(tr("Two flash chips installed, so it could be in another flash chip\n"
            "Make a dump from another flash chip and open it to view information about %1 region").arg(regionName));
        else
            msg(tr("One flash chip installed, so it is an error caused by damaged or incomplete dump"));
        msg(tr("Absence of %1 region assumed\n").arg(regionName));
        return ERR_INVALID_REGION;
    }

    // Get info
    QByteArray body = flashImage.mid(regionOffset, regionSize);
    QString    name;
    QString    info;
    GBE_MAC* gbeMac;   
    GBE_VERSION* gbeVersion;
    ME_VERSION* meVersion;
    INT32 meVersionOffset;
    info = tr("Size: %1")
        .arg(body.size(), 8, 16, QChar('0'));
    switch (regionSubtype) {
    case TreeItem::GbeRegion:
        name = tr("GbE region");
        gbeMac = (GBE_MAC*) body.constData();
        gbeVersion = (GBE_VERSION*) (body.constData() + GBE_VERSION_OFFSET);
        info += tr("\nMAC: %1:%2:%3:%4:%5:%6\nVersion: %7.%8")
            .arg(gbeMac->vendor[0], 2, 16, QChar('0'))
            .arg(gbeMac->vendor[1], 2, 16, QChar('0'))
            .arg(gbeMac->vendor[2], 2, 16, QChar('0'))
            .arg(gbeMac->device[0], 2, 16, QChar('0'))
            .arg(gbeMac->device[1], 2, 16, QChar('0'))
            .arg(gbeMac->device[2], 2, 16, QChar('0'))
            .arg(gbeVersion->major)
            .arg(gbeVersion->minor);
        break;
    case TreeItem::MeRegion:
        name = tr("ME region");
        meVersionOffset = body.indexOf(ME_VERSION_SIGNATURE);
        if (meVersionOffset < 0){
            info += tr("\nVersion: unknown");
            msg(tr("parseRegion: ME region version is unknown, it can be damaged"));
        }
        else {
            meVersion = (ME_VERSION*) (body.constData() + meVersionOffset);
            info += tr("\nVersion: %1.%2.%3.%4")
                .arg(meVersion->major)
                .arg(meVersion->minor)
                .arg(meVersion->bugfix)
                .arg(meVersion->build);
        }
        break;
    case TreeItem::BiosRegion:
        name = tr("BIOS region");
        break;
    case TreeItem::PdrRegion:
        name = tr("PDR region");
        break;
    default:
        name = tr("Unknown region");
        msg(tr("insertInTree: Unknown region"));
        break;
    }

    // Add tree item
    regionIndex = treeModel->addItem(TreeItem::Region, regionSubtype, COMPRESSION_ALGORITHM_NONE, name, "", info, QByteArray(), body, parent);

    return ERR_SUCCESS;
}

UINT8 FfsEngine::parseBios(const QByteArray & bios, const QModelIndex & parent)
{
    // Search for first volume
    UINT32 prevVolumeOffset;
    UINT8 result;
    result = findNextVolume(bios, 0, prevVolumeOffset);
    if (result == ERR_VOLUMES_NOT_FOUND) {
        return result;
    }

    // First volume is not at the beginning of BIOS space
    QString name;
    QString info;
    if (prevVolumeOffset > 0) {
        // Get info
        QByteArray padding = bios.left(prevVolumeOffset);
        name = tr("Padding");
        info = tr("Size: %2")
            .arg(padding.size(), 8, 16, QChar('0'));
        // Add tree item
        treeModel->addItem(TreeItem::Padding, 0, COMPRESSION_ALGORITHM_NONE, name, "", info, QByteArray(), padding, parent);
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
            info = tr("Size: %2")
                .arg(padding.size(), 8, 16, QChar('0'));
            // Add tree item
            treeModel->addItem(TreeItem::Padding, 0, COMPRESSION_ALGORITHM_NONE, name, "", info, QByteArray(), padding, parent);
        } 

        // Get volume size
        result = getVolumeSize(bios, volumeOffset, volumeSize);
        if (result)
            return result;

        //Check that volume is fully present in input
        if (volumeOffset + volumeSize > (UINT32) bios.size()) {
            msg(tr("parseBios: Volume overlaps the end of input buffer"));
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
                msg("parseBios: Incompatible revision 1 volume alignment setup");

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
                msg(tr("parseBios: Unaligned revision 1 volume"));
            }
        }
        else if (volumeHeader->Revision == 2) {
            // Acquire alignment
            alignment = pow(2, (volumeHeader->Attributes & EFI_FVB2_ALIGNMENT) >> 16);

            // Check alignment
            if (volumeOffset % alignment) {
                msg(tr("parseBios: Unaligned revision 2 volume"));
            }
        }
        else
            msg(tr("parseBios: Unknown volume revision (%1)").arg(volumeHeader->Revision));

        // Parse volume
        UINT8 result = parseVolume(bios.mid(volumeOffset, volumeSize), parent);
        if (result)
            msg(tr("parseBios: Volume parsing failed (%1)").arg(result));

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
                treeModel->addItem(TreeItem::Padding, 0, COMPRESSION_ALGORITHM_NONE, name, "", info, QByteArray(), padding, parent);
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

    volumeSize = volumeHeader->FvLength;
    return ERR_SUCCESS;
}

UINT8  FfsEngine::parseVolume(const QByteArray & volume, const QModelIndex & parent, const UINT8 mode)
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
        msg(tr("parseBios: Unknown file system (%1)").arg(guidToQString(volumeHeader->FileSystemGuid)));
        parseCurrentVolume = false;
    }

    // Check attributes
    // Determine value of empty byte
    char empty = volumeHeader->Attributes & EFI_FVB_ERASE_POLARITY ? '\xFF' : '\x00';

    // Check header checksum by recalculating it
    if (!calculateChecksum16((UINT8*) volumeHeader, volumeHeader->HeaderLength)) {
        msg(tr("parseBios: Volume header checksum is invalid"));
    }

    // Check for presence of extended header, only if header revision is greater then 1
    UINT32 headerSize;
    if (volumeHeader->Revision > 1 && volumeHeader->ExtHeaderOffset) {
        EFI_FIRMWARE_VOLUME_EXT_HEADER* extendedHeader = (EFI_FIRMWARE_VOLUME_EXT_HEADER*) ((UINT8*) volumeHeader + volumeHeader->ExtHeaderOffset);
        headerSize = volumeHeader->ExtHeaderOffset + extendedHeader->ExtHeaderSize;
    } else {
        headerSize = volumeHeader->HeaderLength;
    }

    // Get info
    QString name = guidToQString(volumeHeader->FileSystemGuid);
    QString info = tr("Size: %1\nRevision: %2\nAttributes: %3\nHeader size: %4")
        .arg(volumeHeader->FvLength, 8, 16, QChar('0'))
        .arg(volumeHeader->Revision)
        .arg(volumeHeader->Attributes, 8, 16, QChar('0'))
        .arg(volumeHeader->HeaderLength, 4, 16, QChar('0'));

    // Add tree item
    QByteArray  header = volume.left(headerSize);
    QByteArray  body   = volume.mid(headerSize, volumeHeader->FvLength - headerSize);
    QModelIndex index  = treeModel->addItem(TreeItem::Volume, 0, COMPRESSION_ALGORITHM_NONE, name, "", info, header, body, parent, mode);

    // Do not parse volumes with unknown FS
    if (!parseCurrentVolume)
        return ERR_SUCCESS;

    // Search for and parse all files
    UINT32 fileOffset = headerSize;
    UINT32 fileSize;
    UINT8 result;
    QQueue<QByteArray> files;

    while (true) {
        result = getFileSize(volume, fileOffset, fileSize);
        if (result)
            return result;

        // Check file size to be at least sizeof(EFI_FFS_FILE_HEADER)
        if (fileSize < sizeof(EFI_FFS_FILE_HEADER)) {
            msg(tr("parseVolume: File with invalid size"));
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
        UINT32 alignment = pow(2, alignmentPower);
        if ((fileOffset + sizeof(EFI_FFS_FILE_HEADER)) % alignment) {
            msg(tr("parseVolume: %1, unaligned file").arg(guidToQString(fileHeader->Name)));
        }

        // Check file GUID
        if (fileHeader->Type != EFI_FV_FILETYPE_PAD && files.indexOf(header.left(sizeof(EFI_GUID))) != -1)
            msg(tr("%1: file with duplicate GUID").arg(guidToQString(fileHeader->Name)));

        // Add file GUID to queue
        files.enqueue(header.left(sizeof(EFI_GUID)));

        // Parse file 
        result = parseFile(file, volumeHeader->Revision, empty, index);
        if (result)
            msg(tr("parseVolume: Parse FFS file failed (%1)").arg(result));

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

UINT8 FfsEngine::parseFile(const QByteArray & file, UINT8 revision, const char empty, const QModelIndex & parent, const UINT8 mode)
{
    EFI_FFS_FILE_HEADER* fileHeader = (EFI_FFS_FILE_HEADER*) file.constData();

    // Check header checksum
    QByteArray header = file.left(sizeof(EFI_FFS_FILE_HEADER));
    QByteArray tempHeader = header;
    EFI_FFS_FILE_HEADER* tempFileHeader = (EFI_FFS_FILE_HEADER*) (tempHeader.data());
    tempFileHeader->IntegrityCheck.Checksum.Header = 0;
    tempFileHeader->IntegrityCheck.Checksum.File = 0;
    UINT8 calculated = calculateChecksum8((UINT8*) tempFileHeader, sizeof(EFI_FFS_FILE_HEADER) - 1);
    if (fileHeader->IntegrityCheck.Checksum.Header != calculated)
    {
        msg(tr("parseVolume: %1, stored header checksum %2 differs from calculated %3")
            .arg(guidToQString(fileHeader->Name))
            .arg(fileHeader->IntegrityCheck.Checksum.Header, 2, 16, QChar('0'))
            .arg(calculated, 2, 16, QChar('0')));
    }

    // Check data checksum
    // Data checksum must be calculated
    if (fileHeader->Attributes & FFS_ATTRIB_CHECKSUM) {
        UINT32 bufferSize = file.size() - sizeof(EFI_FFS_FILE_HEADER);
        // Exclude file tail from data checksum calculation
        if(revision == 1 && fileHeader->Attributes & FFS_ATTRIB_TAIL_PRESENT)
            bufferSize -= sizeof(UINT16);
        calculated = calculateChecksum8((UINT8*)(file.constData() + sizeof(EFI_FFS_FILE_HEADER)), bufferSize);
        if (fileHeader->IntegrityCheck.Checksum.File != calculated) {
            msg(tr("parseVolume: %1, stored data checksum %2 differs from calculated %3")
                .arg(guidToQString(fileHeader->Name))
                .arg(fileHeader->IntegrityCheck.Checksum.File, 2, 16, QChar('0'))
                .arg(calculated, 2, 16, QChar('0')));
        }
    }
    // Data checksum must be one of predefined values
    else {
        if (fileHeader->IntegrityCheck.Checksum.File != FFS_FIXED_CHECKSUM && fileHeader->IntegrityCheck.Checksum.File != FFS_FIXED_CHECKSUM2) {
            msg(tr("parseVolume: %1, stored data checksum %2 differs from standard value")
                .arg(guidToQString(fileHeader->Name))
                .arg(fileHeader->IntegrityCheck.Checksum.File, 2, 16, QChar('0')));
        }
    }

    // Get file body
    QByteArray body = file.right(file.size() - sizeof(EFI_FFS_FILE_HEADER));
    UINT32 fileSize = (UINT32) file.size(); 
    // For files in Revision 1 volumes, check for file tail presence
    if (revision == 1 && fileHeader->Attributes & FFS_ATTRIB_TAIL_PRESENT)
    {
        //Check file tail;
        UINT16* tail = (UINT16*) body.right(sizeof(UINT16)).constData();
        if (!fileHeader->IntegrityCheck.TailReference == *tail)
            msg(tr("parseVolume: %1, file tail value %2 is not a bitwise not of %3 stored in file header")
            .arg(guidToQString(fileHeader->Name))
            .arg(*tail, 4, 16, QChar('0'))
            .arg(fileHeader->IntegrityCheck.TailReference, 4, 16, QChar('0')));

        // Remove tail from file body
        body = body.left(body.size() - sizeof(UINT16));
        fileSize -= sizeof(UINT16);
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
        msg(tr("parseVolume: Unknown file type (%1)").arg(fileHeader->Type, 2, 16, QChar('0')));
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
    QModelIndex index = treeModel->addItem(TreeItem::File, fileHeader->Type, COMPRESSION_ALGORITHM_NONE, name, "", info, header, body, parent, mode);

    if (!parseCurrentFile)
        return ERR_SUCCESS;

    // Parse file as BIOS space
    UINT8 result;
    if (parseAsBios) {
        result = parseBios(body, index);
        if (result && result != ERR_VOLUMES_NOT_FOUND)
            msg(tr("parseVolume: Parse file as BIOS failed (%1)").arg(result));
        return ERR_SUCCESS;
    }

    // Parse sections
    result = parseSections(body, revision, empty, index);
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

UINT8 FfsEngine::parseSections(const QByteArray & body, const UINT8 revision, const char empty, const QModelIndex & parent)
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
            result = parseSection(body.mid(sectionOffset, sectionSize), revision, empty, parent);
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

UINT8 FfsEngine::parseSection(const QByteArray & section, const UINT8 revision, 
                               const char empty, const QModelIndex & parent, const UINT8 mode)
{
    EFI_COMMON_SECTION_HEADER* sectionHeader = (EFI_COMMON_SECTION_HEADER*) (section.constData());
    UINT32 sectionSize = uint24ToUint32(sectionHeader->Size);
    QString name = sectionTypeToQString(sectionHeader->Type) + tr(" section");
    QString info; 
    QByteArray header;
    QByteArray body;
    UINT32 headerSize;
    QModelIndex index;
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
                msg(tr("parseFile: Section decompression failed (%1)").arg(result));
                parseCurrentSection = false;
            }

            // Get info
            info = tr("Type: %1\nSize: %2\nCompression type: %3\nDecompressed size: %4")
                .arg(sectionHeader->Type, 2, 16, QChar('0'))
                .arg(body.size(), 8, 16, QChar('0'))
                .arg(compressionTypeToQString(algorithm))
                .arg(compressedSectionHeader->UncompressedLength, 8, 16, QChar('0'));

            // Add tree item
            index  = treeModel->addItem(TreeItem::Section, sectionHeader->Type, algorithm, name, "", info, header, body, parent, mode);
            
            // Parse decompressed data
            if (parseCurrentSection) {
                result = parseSections(decompressed, revision, empty, index);
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
                        msg(tr("parseFile: GUID defined section can not be decompressed (%1)").arg(result));
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
            index  = treeModel->addItem(TreeItem::Section, sectionHeader->Type, algorithm, name, "", info, header, body, parent, mode);

            // Parse decompressed data
            if (parseCurrentSection) {
                result = parseSections(decompressed, revision, empty, index);
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
            info = tr("Type: %1\nSize: %2")
                .arg(sectionHeader->Type, 2, 16, QChar('0'))
                .arg(body.size(), 8, 16, QChar('0'));

            // Add tree item
            index  = treeModel->addItem(TreeItem::Section, sectionHeader->Type, COMPRESSION_ALGORITHM_NONE, name, "", info, header, body, parent, mode);

            // Parse section body
            result = parseSections(body, revision, empty, index);
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
        index = treeModel->addItem(TreeItem::Section, sectionHeader->Type, COMPRESSION_ALGORITHM_NONE, name, "", info, header, body, parent, mode);
        break;
    case EFI_SECTION_USER_INTERFACE:
        header = section.left(sizeof(EFI_USER_INTERFACE_SECTION));
        body   = section.mid(sizeof(EFI_USER_INTERFACE_SECTION), sectionSize - sizeof(EFI_USER_INTERFACE_SECTION));

        // Get info
        info = tr("Type: %1\nSize: %2")
            .arg(sectionHeader->Type, 2, 16, QChar('0'))
            .arg(body.size(), 8, 16, QChar('0'));

        // Add tree item
        index = treeModel->addItem(TreeItem::Section, sectionHeader->Type, COMPRESSION_ALGORITHM_NONE, name, "", info, header, body, parent, mode);

        // Rename parent file
        {
            QString text = QString::fromUtf16((const ushort*)body.constData());
            setTreeItemText(text, findParentOfType(TreeItem::File, parent));
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
        index = treeModel->addItem(TreeItem::Section, sectionHeader->Type, COMPRESSION_ALGORITHM_NONE, name, "", info, header, body, parent, mode);

        // Parse section body as BIOS space
        result = parseBios(body, index);
        if (result && result != ERR_VOLUMES_NOT_FOUND) {
            msg(tr("parseFile: Firmware volume image can not be parsed (%1)").arg(result));
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
        index  = treeModel->addItem(TreeItem::Section, sectionHeader->Type, COMPRESSION_ALGORITHM_NONE, name, "", info, header, body, parent, mode);

        // Parse section body as BIOS space
        result = parseBios(body, index);
        if (result && result != ERR_VOLUMES_NOT_FOUND) {
            msg(tr("parseFile: Raw section can not be parsed as BIOS (%1)").arg(result));
            return result;
        }
        break;
    default:
        msg(tr("parseFile: Section with unknown type (%1)").arg(sectionHeader->Type, 2, 16, QChar('0')));
        header = section.left(sizeof(EFI_COMMON_SECTION_HEADER));
        body   = section.mid(sizeof(EFI_COMMON_SECTION_HEADER), sectionSize - sizeof(EFI_COMMON_SECTION_HEADER));
        // Get info
        info = tr("Type: %1\nSize: %2")
            .arg(sectionHeader->Type, 2, 16, QChar('0'))
            .arg(body.size(), 8, 16, QChar('0'));

        // Add tree item
        index  = treeModel->addItem(TreeItem::Section, sectionHeader->Type, COMPRESSION_ALGORITHM_NONE, name, "", info, header, body, parent, mode);
        return ERR_UNKNOWN_SECTION;
    }
    return ERR_SUCCESS;
}

// Operations on tree items
UINT8 FfsEngine::insert(const QModelIndex & index, const QByteArray & object, const UINT8 type, const UINT8 mode)
{
    if (!index.isValid())
        return ERR_INVALID_PARAMETER;
    
    // Only files and sections can now be inserted
    if (type == TreeItem::File) {
        QModelIndex parent;
        if (mode == INSERT_MODE_BEFORE || mode == INSERT_MODE_AFTER)
            parent = index.parent();
        else
            parent = index;
        
        // Parent type must be volume
        TreeItem * parentItem = static_cast<TreeItem*>(parent.internalPointer());
        if (parentItem->type() != TreeItem::Volume) {
            msg(tr("insert: file can't be inserted into something that is not volume"));
            return ERR_INVALID_VOLUME;
        }

        EFI_FIRMWARE_VOLUME_HEADER* header = (EFI_FIRMWARE_VOLUME_HEADER*) parentItem->header().constData();

        // Parse file
        UINT8 result = parseFile(object, header->Revision, header->Attributes & EFI_FVB_ERASE_POLARITY ? '\xFF' : '\x00', index, mode);
        if (result)
            return result;
        
        // Set reconstruct action for all it's parents
        for (;parent.isValid(); parent = parent.parent())
            treeModel->setItemAction(TreeItem::Reconstruct, parent);

    }
    else if (type == TreeItem::Section) {
        QModelIndex parent;
        if (mode == INSERT_MODE_BEFORE || mode == INSERT_MODE_AFTER)
            parent = index.parent();
        else
            parent = index;
        
        // Parent type must be file or encapsulation section
        TreeItem * parentItem = static_cast<TreeItem*>(parent.internalPointer());
        if (parentItem->type() == TreeItem::File || (parentItem->type() == TreeItem::Section && 
            (parentItem->subtype() == EFI_SECTION_COMPRESSION || 
             parentItem->subtype() == EFI_SECTION_GUID_DEFINED || 
             parentItem->subtype() == EFI_SECTION_DISPOSABLE))) {
                QModelIndex volumeIndex = findParentOfType(TreeItem::Volume, parent);
                if (!volumeIndex.isValid()) {
                    msg(tr("insert: Parent volume not found"));
                    return ERR_INVALID_VOLUME;
                }

                TreeItem * volumeItem = static_cast<TreeItem*>(volumeIndex.internalPointer());
                EFI_FIRMWARE_VOLUME_HEADER* header = (EFI_FIRMWARE_VOLUME_HEADER*) volumeItem->header().constData();

                // Parse section
                UINT8 result = parseSection(object, header->Revision, header->Attributes & EFI_FVB_ERASE_POLARITY ? '\xFF' : '\x00', index, mode);
                if (result)
                    return result;

                // Set reconstruct action for all parents
                for (;parent.isValid(); parent = parent.parent())
                    treeModel->setItemAction(TreeItem::Reconstruct, parent);
        }
        else {
            msg(tr("insert: section can't be inserted into something that is not file or encapsulation section"));
            return ERR_INVALID_FILE;
        }
    }
    else 
        return ERR_NOT_IMPLEMENTED;
    
    return ERR_SUCCESS;
}

UINT8 FfsEngine::remove(const QModelIndex & index)
{
    if (!index.isValid())
        return ERR_INVALID_PARAMETER;

    // Set action for the item
    treeModel->setItemAction(TreeItem::Remove, index);

    // Set reconstruct action for all it's parents
    for (QModelIndex parent = index.parent(); parent.isValid(); parent = parent.parent())
        treeModel->setItemAction(TreeItem::Reconstruct, parent);

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
        if (ERR_SUCCESS != EfiDecompress(data, dataSize, decompressed, decompressedSize, scratch, scratchSize)) {
            if (ERR_SUCCESS != TianoDecompress(data, dataSize, decompressed, decompressedSize, scratch, scratchSize)) {
                if (algorithm)
                    *algorithm = COMPRESSION_ALGORITHM_UNKNOWN;
                return ERR_STANDARD_DECOMPRESSION_FAILED;
            }
            else if (algorithm)
                *algorithm = COMPRESSION_ALGORITHM_TIANO;
        }
        else if (algorithm)
            *algorithm = COMPRESSION_ALGORITHM_EFI11;

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
            if (LzmaCompress((UINT8*) data.constData(), data.size(), NULL, &compressedSize) != ERR_BUFFER_TOO_SMALL)
                return ERR_CUSTOMIZED_COMPRESSION_FAILED;
            compressed = new UINT8[compressedSize];
            if (LzmaCompress((UINT8*) data.constData(), data.size(), compressed, &compressedSize) != ERR_SUCCESS)
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
    reconstructed = QByteArray();
    rootItem->setAction(TreeItem::Reconstruct);
    UINT8 result = reconstruct(rootItem, queue);
    if (result)
        return result;

    while (!queue.isEmpty())
        reconstructed.append(queue.dequeue());
    rootItem->setAction(TreeItem::NoAction);
    return ERR_SUCCESS;
}

UINT8 FfsEngine::constructPadFile(const UINT32 size, const UINT8 revision, const char empty, QByteArray & pad)
{
    if (size < sizeof(EFI_FFS_FILE_HEADER))
        return ERR_INVALID_PARAMETER;

    pad = QByteArray(size, empty);
    EFI_FFS_FILE_HEADER* header = (EFI_FFS_FILE_HEADER*) pad.data();
    uint32ToUint24(size, header->Size);
    header->Attributes = 0x00;
    header->Type = EFI_FV_FILETYPE_PAD;
    header->State = 0xF8;
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

UINT8 FfsEngine::reconstruct(TreeItem* item, QQueue<QByteArray> & queue, const UINT8 revision, char empty)
{
    if (!item)
        return ERR_SUCCESS;

    QByteArray reconstructed;
    UINT8 result;

    // No action is needed, just return header + body
    if (item->action() == TreeItem::NoAction) {
        reconstructed = item->header().append(item->body());
        // One special case: file with tail
        if (revision == 1 && item->type() == TreeItem::File) {
            EFI_FFS_FILE_HEADER* fileHeader = (EFI_FFS_FILE_HEADER*) item->header().constData();
            if (fileHeader->Attributes & FFS_ATTRIB_TAIL_PRESENT) {
                // Append file tail
                reconstructed.append(!fileHeader->IntegrityCheck.TailReference);
            }
        }

        queue.enqueue(reconstructed);
        return ERR_SUCCESS;
    }
    // Remove item
    else if (item->action() == TreeItem::Remove) {
        // Root item can't be removed
        if (item == rootItem)
            return ERR_INVALID_PARAMETER;
        // Volume can be removed by replacing all it's contents with empty bytes
        if (item->type() == TreeItem::Volume) {
            EFI_FIRMWARE_VOLUME_HEADER* volumeHeader = (EFI_FIRMWARE_VOLUME_HEADER*) item->header().constData();
            empty = volumeHeader->Attributes & EFI_FVB_ERASE_POLARITY ? '\xFF' : '\x00';
            reconstructed.fill(empty, item->header().size() + item->body().size());
            queue.enqueue(reconstructed);
            return ERR_SUCCESS;
        }
        // File can be removed
        if (item->type() == TreeItem::File)
            // Add nothing to queue
                return ERR_SUCCESS;
        // Section can be removed
        else if (item->type() == TreeItem::Section)
            // Add nothing to queue
            return ERR_SUCCESS;
        // Other item types can't be removed
        else
            return ERR_NOT_IMPLEMENTED;
    }
    // Reconstruct item and it's children recursive
    else if (item->action() == TreeItem::Reconstruct) {
        QQueue<QByteArray> childrenQueue;

        switch (item->type()) {
        case TreeItem::Capsule:
            if (item->subtype() == TreeItem::AptioCapsule)
                msg(tr("reconstruct: Aptio extended header checksum and signature are now invalid"));
        case TreeItem::Root:
        case TreeItem::Descriptor:
        case TreeItem::Region:
        case TreeItem::Padding:
            // Reconstruct item body
            if (item->childCount()) {
                // Reconstruct item children
                for (int i = 0; i < item->childCount(); i++) {
                    result = reconstruct(item->child(i), childrenQueue);
                    if (result)
                        return result;
                }

                // No additional checks needed
                while (!childrenQueue.isEmpty())
                    reconstructed.append(childrenQueue.dequeue());
            }
            // Use stored item body
            else
                reconstructed = item->body();

            // Enqueue reconstructed item
            queue.enqueue(item->header().append(reconstructed));  
            break;

        case TreeItem::Volume: 
            {
                //!TODO: add check for weak aligned volumes
                QByteArray header = item->header();
                EFI_FIRMWARE_VOLUME_HEADER* volumeHeader = (EFI_FIRMWARE_VOLUME_HEADER*) header.constData();
                empty = volumeHeader->Attributes & EFI_FVB_ERASE_POLARITY ? '\xFF' : '\x00';
                // Reconstruct volume body
                if (item->childCount()) {
                    // Reconstruct files in volume
                    for (int i = 0; i < item->childCount(); i++) {
                        // Reconstruct files
                        result = reconstruct(item->child(i), childrenQueue, volumeHeader->Revision, empty);
                        if (result)
                            return result;
                    }

                    // Remove all pad files, which will be recreated later
                    foreach(const QByteArray & child, childrenQueue)
                    {
                        EFI_FFS_FILE_HEADER* fileHeader = (EFI_FFS_FILE_HEADER*) child.constData();
                        if (fileHeader->Type == EFI_FV_FILETYPE_PAD)
                            childrenQueue.removeAll(child);
                    }

                    // Construct new volume body
                    UINT32 offset = 0;
                    while (!childrenQueue.isEmpty())
                    {
                        // Align to 8 byte boundary
                        UINT32 alignment = offset % 8;
                        if (alignment) {
                            alignment = 8 - alignment;
                            offset += alignment;
                            reconstructed.append(QByteArray(alignment, empty));
                        }

                        // Get file from queue
                        QByteArray file = childrenQueue.dequeue();
                        EFI_FFS_FILE_HEADER* fileHeader = (EFI_FFS_FILE_HEADER*) file.data(); 

                        // Check alignment
                        UINT8 alignmentPower;
                        UINT32 base;
                        alignmentPower = ffsAlignmentTable[(fileHeader->Attributes & FFS_ATTRIB_DATA_ALIGNMENT) >> 3];
                        alignment = pow(2, alignmentPower);
                        base = header.size() + offset + sizeof(EFI_FFS_FILE_HEADER);
                        if (base % alignment) {
                            // File will be unaligned if added as is, so we must add pad file before it
                            // Determine pad file size
                            UINT32 size = alignment - (base % alignment);
                            // Required padding is smaler then minimal pad file size
                            while (size < sizeof(EFI_FFS_FILE_HEADER)) {
                                size += alignment;
                            }
                            // Construct pad file
                            QByteArray pad;
                            result = constructPadFile(size, revision, empty, pad);
                            if (result)
                                return result;
                            // Append constructed pad file to volume body
                            reconstructed.append(pad);
                            offset += size;
                        }

                        // If this is a last file in volume
                        if (childrenQueue.isEmpty())
                        {
                            // Last file of the volume can be Volume Top File
                            if (file.left(sizeof(EFI_GUID)) == EFI_FFS_VOLUME_TOP_FILE_GUID) {
                                // Determine correct VTF offset
                                UINT32 vtfOffset = volumeHeader->FvLength - header.size() - file.size();
                                if (offset % 8) {
                                    msg(tr("reconstruct: %1: Wrong size of Volume Top File")
                                        .arg(guidToQString(volumeHeader->FileSystemGuid)));
                                    return ERR_INVALID_FILE;
                                }
                                // Insert pad file to fill the gap
                                if (vtfOffset > offset) {
                                    // Determine pad file size
                                    UINT32 size = vtfOffset - offset;
                                    // Construct pad file
                                    QByteArray pad;
                                    result = constructPadFile(size, revision, empty, pad);
                                    if (result)
                                        return result;
                                    // Append constructed pad file to volume body
                                    reconstructed.append(pad);
                                    offset = vtfOffset;
                                    // Ensure that no more files will be in this volume
                                    childrenQueue.clear();
                                }
                                // No more space left in volume
                                else if (vtfOffset < offset) {
                                    //!TODO: attempt volume grow
                                    msg(tr("reconstruct: %1: can't insert VTF, need additional %2 bytes")
                                        .arg(guidToQString(volumeHeader->FileSystemGuid))
                                        .arg(offset - vtfOffset, 8, 16, QChar('0')));
                                    return ERR_INVALID_VOLUME;
                                }
                            }
                            // Fill all following bytes with empty char
                            else {
                                INT32 size = volumeHeader->FvLength - header.size() - offset - file.size();
                                // Append fill
                                if (size > 0)
                                    file.append(QByteArray(size, empty));
                            }
                        }

                        // Append current file to new volume body
                        reconstructed.append(file);
                        // Change current file offset
                        offset += file.size();
                    }

                    // Check new body size
                    if (header.size() + reconstructed.size() > volumeHeader->FvLength)
                    {
                        //!TODO: attemt volume grow
                        msg(tr("reconstruct: Volume grow operation is not yet implemented"));
                        return ERR_NOT_IMPLEMENTED;
                        /*// Volumes can be children of RootItem, CapsuleItem, RegionItem, FileItem and SectionItem

                        UINT32 sizeToGrow = 0;
                        UINT8 parentType = item->parent()->type();
                        //!TODO: refactor this code to make it work
                        // First 3 kind of volumes can be grown only if they have padding after them
                        if (parentType == RootItem || parentType == CapsuleItem || parentType == RegionItem) {
                        // Find next item 
                        for (int i = 0; i < item->parent()->childCount(); i++)
                        if (item == item->parent()->child(i) && item->parent()->child(i+1) != NULL) {
                        TreeItem* pad = item->parent()->child(i+1);
                        // Check if that item is padding
                        if (pad->type() == PaddingItem) {
                        // All it's space can be used for volume growing
                        sizeToGrow = pad->body().size();
                        }
                        }
                        }

                        // Second 2 kind can just be grown up to UIN32_MAX in size
                        if (parentType == TreeItem::File || parentType == TreeItem::Section) {
                        sizeToGrow = UINT32_MAX - header.size() - reconstructed.size();
                        }    

                        // Volume is a child of some other item, this is a bug
                        else {
                        msg(tr("reconstructTreeItem: %1: volume is a child of incompatible item")
                        .arg(guidToQString(volumeHeader->FileSystemGuid)));
                        return ERR_INVALID_VOLUME;
                        }

                        if (sizeToGrow == 0 || (header.size() + reconstructed.size() - volumeHeader->FvLength) > sizeToGrow) {
                        msg(tr("reconstruct: %1: volume can not be grown")
                        .arg(guidToQString(volumeHeader->FileSystemGuid)));
                        return ERR_VOLUME_GROW_FAILED;
                        }

                        // Adjust new size to be representable by current FvBlockMap
                        // We assume that all current volumes have only one meaningful FvBlockMap entry
                        EFI_FV_BLOCK_MAP_ENTRY* blockMap = (EFI_FV_BLOCK_MAP_ENTRY*) (header.data() + sizeof(EFI_FIRMWARE_VOLUME_HEADER));

                        // Calculate new size
                        UINT32 size = header.size() + reconstructed.size();
                        sizeToGrow = blockMap->Length - size % blockMap->Length;

                        // Recalculate number of blocks
                        blockMap->NumBlocks += sizeToGrow / blockMap->Length + 1;

                        // Set new volume size
                        //!NOTE: this is dangerous and must be checked before adding volume to items tree
                        volumeHeader->FvLength = 0;
                        while (blockMap->NumBlocks != 0 || blockMap->Length != 0)
                        volumeHeader->FvLength += blockMap->NumBlocks * blockMap->Length;

                        // Recalculate volume header checksum
                        volumeHeader->Checksum = 0;
                        volumeHeader->Checksum = calculateChecksum16((UINT8*) volumeHeader, volumeHeader->HeaderLength);*/
                    }
                }
                // Use current volume body
                else 
                    reconstructed = item->body();

                // Enqueue reconstructed item
                queue.enqueue(header.append(reconstructed));    
            }
            break;

        case TreeItem::File: 
            {
                QByteArray header = item->header();
                EFI_FFS_FILE_HEADER* fileHeader = (EFI_FFS_FILE_HEADER*) header.data(); 

                // Reconstruct file body
                if (item->childCount()) {
                    for (int i = 0; i < item->childCount(); i++) {
                        // Reconstruct sections
                        result = reconstruct(item->child(i), childrenQueue, revision, empty);
                        if (result)
                            return result;
                    }

                    // Construct new file body
                    UINT32 offset = 0;
                    while (!childrenQueue.isEmpty())
                    {
                        // Align to 4 byte boundary
                        UINT8 alignment = offset % 4;
                        if (alignment) {
                            alignment = 4 - alignment;
                            offset += alignment;
                            reconstructed.append(QByteArray(alignment, empty));
                        }

                        // Get section from queue
                        QByteArray section = childrenQueue.dequeue();

                        // Append current section to new file body
                        reconstructed.append(section);
                        // Change current file offset
                        offset += section.size();
                    }

                    // Correct file size
                    UINT8 tailSize = 0;
                    if(revision == 1 && (fileHeader->Attributes & FFS_ATTRIB_TAIL_PRESENT))
                        tailSize = sizeof(UINT16);

                    uint32ToUint24(sizeof(EFI_FFS_FILE_HEADER) + reconstructed.size() + tailSize, fileHeader->Size);

                    // Recalculate header checksum
                    fileHeader->IntegrityCheck.Checksum.Header = 0;
                    fileHeader->IntegrityCheck.Checksum.File = 0;
                    fileHeader->IntegrityCheck.Checksum.Header = calculateChecksum8((UINT8*) fileHeader, sizeof(EFI_FFS_FILE_HEADER) - 1);

                    // Recalculate data checksum, if needed
                    if (fileHeader->Attributes & FFS_ATTRIB_CHECKSUM) {
                        fileHeader->IntegrityCheck.Checksum.File = calculateChecksum8((UINT8*) reconstructed.constData(), reconstructed.size());
                    }
                    else if (revision == 1)
                        fileHeader->IntegrityCheck.Checksum.File = FFS_FIXED_CHECKSUM;
                    else 
                        fileHeader->IntegrityCheck.Checksum.File = FFS_FIXED_CHECKSUM2;

                    // Append tail, if needed
                    if (tailSize)
                        reconstructed.append(!fileHeader->IntegrityCheck.TailReference);
                }
                // Use current file body
                else
                    reconstructed = item->body();

                // Enqueue reconstructed item
                queue.enqueue(header.append(reconstructed));  
            }
            break;

        case TreeItem::Section:
            {
                QByteArray header = item->header();
                EFI_COMMON_SECTION_HEADER* commonHeader = (EFI_COMMON_SECTION_HEADER*) header.data();

                // Section with children
                if (item->childCount()) {
                    // Reconstruct section body
                    for (int i = 0; i < item->childCount(); i++) {
                        // Reconstruct subsections
                        result = reconstruct(item->child(i), childrenQueue, revision, empty);
                        if (result)
                            return result;
                    }

                    // Construct new section body
                    UINT32 offset = 0;
                    while (!childrenQueue.isEmpty())
                    {
                        // Align to 4 byte boundary
                        UINT8 alignment = offset % 4;
                        if (alignment) {
                            alignment = 4 - alignment;
                            offset += alignment;
                            reconstructed.append(QByteArray(alignment, empty));
                        }

                        // Get section from queue
                        QByteArray section = childrenQueue.dequeue();

                        // Append current subsection to new section body
                        reconstructed.append(section);
                        // Change current file offset
                        offset += section.size();
                    }

                    // Only this 2 sections can have compressed body
                    if (item->subtype() == EFI_SECTION_COMPRESSION) {
                        EFI_COMPRESSION_SECTION* compessionHeader = (EFI_COMPRESSION_SECTION*) header.data();
                        // Set new uncompressed size
                        compessionHeader->UncompressedLength = reconstructed.size();
                        // Compress new section body
                        QByteArray compressed;
                        result = compress(reconstructed, item->compression(), compressed);
                        if (result)
                            return result;
                        // Correct compression type
                        if (item->compression() == COMPRESSION_ALGORITHM_NONE)
                            compessionHeader->CompressionType = EFI_NOT_COMPRESSED;
                        else if (item->compression() == COMPRESSION_ALGORITHM_LZMA || item->compression() == COMPRESSION_ALGORITHM_IMLZMA)
                            compessionHeader->CompressionType = EFI_CUSTOMIZED_COMPRESSION;
                        else
                            compessionHeader->CompressionType = EFI_STANDARD_COMPRESSION;
                        // Replace new section body
                        reconstructed = compressed;
                    }
                    else if (item->subtype() == EFI_SECTION_GUID_DEFINED) {
                        EFI_GUID_DEFINED_SECTION* guidDefinedHeader = (EFI_GUID_DEFINED_SECTION*) header.data();
                        // Compress new section body
                        QByteArray compressed;
                        result = compress(reconstructed, item->compression(), compressed);
                        if (result)
                            return result;
                        // Check for auth status valid attribute
                        if (guidDefinedHeader->Attributes & EFI_GUIDED_SECTION_AUTH_STATUS_VALID)
                        {
                            msg(tr("reconstruct: %1: GUID defined section signature can now become invalid")
                                .arg(guidToQString(guidDefinedHeader->SectionDefinitionGuid)));
                        }
                        // Replace new section body
                        reconstructed = compressed;
                    }
                    else if (item->compression() != COMPRESSION_ALGORITHM_NONE) {
                        msg(tr("reconstruct: compression required for section of type %1")
                            .arg(item->subtype()));
                        return ERR_INVALID_SECTION;
                    }

                    // Correct section size
                    uint32ToUint24(header.size() + reconstructed.size(), commonHeader->Size);
                }
                // Leaf section
                else 
                    reconstructed = item->body();

                // Enqueue reconstructed item
                queue.enqueue(header.append(reconstructed));
            }
            break;

        default:
            msg(tr("reconstruct: Unknown item type (%1)").arg(item->type()));
            return ERR_UNKNOWN_ITEM_TYPE;
        }

        return ERR_SUCCESS;
    }

    //!TODO: implement other actions
    return ERR_NOT_IMPLEMENTED;
}



// Will be refactored later
QByteArray FfsEngine::decompressFile(const QModelIndex& index) const
{
    if (!index.isValid())
        return QByteArray();

    // Check index item to be FFS file
    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    if(item->type() != TreeItem::File)
        return QByteArray();

    QByteArray file;
    UINT32 offset = 0;
    // Construct new item body
    for (int i = 0; i < item->childCount(); i++) {
        // If section is not compressed, add it to new body as is
        TreeItem* sectionItem = item->child(i);
        if (sectionItem->subtype() != EFI_SECTION_COMPRESSION) {
            QByteArray section = sectionItem->header().append(sectionItem->body());
            UINT32 align = ALIGN4(offset) - offset;
            file.append(QByteArray(align, '\x00')).append(section);
            offset += align + section.size();
        }
        else {
            // Construct new section body by adding all child sections to this new section
            QByteArray section;
            UINT32 subOffset = 0;
            for (int j = 0; j < sectionItem->childCount(); j++)
            {
                TreeItem* subSectionItem = sectionItem->child(j);
                QByteArray subSection = subSectionItem->header().append(subSectionItem->body());
                UINT32 align = ALIGN4(subOffset) - subOffset;
                section.append(QByteArray(align, '\x00')).append(subSection);
                subOffset += align + subSection.size();
            }
            // Add newly constructed section to file body

            EFI_COMPRESSION_SECTION sectionHeader;
            sectionHeader.Type = EFI_SECTION_COMPRESSION;
            sectionHeader.CompressionType = EFI_NOT_COMPRESSED;
            sectionHeader.UncompressedLength = section.size();
            uint32ToUint24(section.size() + sizeof(EFI_COMPRESSION_SECTION), sectionHeader.Size);
            UINT32 align = ALIGN4(offset) - offset;
            file.append(QByteArray(align, '\x00'))
                .append(QByteArray((const char*) &sectionHeader, sizeof(EFI_COMPRESSION_SECTION)))
                .append(section);
            offset += align + section.size();
        }
    }

    QByteArray header = item->header();
    EFI_FFS_FILE_HEADER* fileHeader = (EFI_FFS_FILE_HEADER*) header.data();

    // Correct file data checksum, if needed
    if (fileHeader->Attributes & FFS_ATTRIB_CHECKSUM) {
        UINT32 bufferSize = file.size() - sizeof(EFI_FFS_FILE_HEADER);
        fileHeader->IntegrityCheck.Checksum.File = calculateChecksum8((UINT8*)(file.data() + sizeof(EFI_FFS_FILE_HEADER)), bufferSize);
    }

    // Add file tail, if needed
    if(fileHeader->Attributes & FFS_ATTRIB_TAIL_PRESENT)
        file.append(!fileHeader->IntegrityCheck.TailReference);

    return header.append(file);
}

bool FfsEngine::isCompressedFile(const QModelIndex& index) const
{
    if (!index.isValid())
        return false;

    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    if(item->type() != TreeItem::File)
        return false;

    for (int i = 0; i < item->childCount(); i++) {
        if (item->child(i)->subtype() == EFI_SECTION_COMPRESSION)
            return true;
    }

    return false;
}