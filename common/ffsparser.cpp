/* ffsparser.cpp

Copyright (c) 2015, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHWARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*/

#include <math.h>

#include "ffsparser.h"
#include "types.h"
#include "treemodel.h"
#include "descriptor.h"
#include "ffs.h"
#include "gbe.h"
#include "me.h"

FfsParser::FfsParser(TreeModel* treeModel, QObject *parent)
    : QObject(parent), model(treeModel)
{
}

FfsParser::~FfsParser()
{
}

void FfsParser::msg(const QString & message, const QModelIndex & index)
{
    messagesVector.push_back(QPair<QString, QModelIndex>(message, index));
}

QVector<QPair<QString, QModelIndex> > FfsParser::getMessages() const
{
    return messagesVector;
}

void FfsParser::clearMessages()
{
    messagesVector.clear();
}

BOOLEAN FfsParser::hasIntersection(const UINT32 begin1, const UINT32 end1, const UINT32 begin2, const UINT32 end2)
{
    if (begin1 < begin2 && begin2 < end1)
        return TRUE;
    if (begin1 < end2 && end2 < end1)
        return TRUE;
    if (begin2 < begin1 && begin1 < end2)
        return TRUE;
    if (begin2 < end1 && end1 < end2)
        return TRUE;
    return FALSE;
}

// Firmware image parsing functions
STATUS FfsParser::parseImageFile(const QByteArray & buffer, const QModelIndex & root)
{
    // Check buffer size to be more then or equal to size of EFI_CAPSULE_HEADER
    if ((UINT32)buffer.size() <= sizeof(EFI_CAPSULE_HEADER)) {
        msg(tr("parseImageFile: image file is smaller then minimum size of %1h (%2) bytes").hexarg(sizeof(EFI_CAPSULE_HEADER)).arg(sizeof(EFI_CAPSULE_HEADER)));
        return ERR_INVALID_PARAMETER;
    }

    // Check buffer for being normal EFI capsule header
    UINT32 capsuleHeaderSize = 0;
    QModelIndex index;
    if (buffer.startsWith(EFI_CAPSULE_GUID)) {
        // Get info
        const EFI_CAPSULE_HEADER* capsuleHeader = (const EFI_CAPSULE_HEADER*)buffer.constData();
        capsuleHeaderSize = capsuleHeader->HeaderSize;
        QByteArray header = buffer.left(capsuleHeaderSize);
        QByteArray body = buffer.mid(capsuleHeaderSize);
        QString name = tr("UEFI capsule");
        QString info = tr("Offset: 0h\nCapsule GUID: %1\nFull size: %2h (%3)\nHeader size: %4h (%5)\nImage size: %6h (%7)\nFlags: %8h")
            .arg(guidToQString(capsuleHeader->CapsuleGuid))
            .hexarg(buffer.size()).arg(buffer.size())
            .hexarg(capsuleHeader->HeaderSize).arg(capsuleHeader->HeaderSize)
            .hexarg(capsuleHeader->CapsuleImageSize).arg(capsuleHeader->CapsuleImageSize)
            .hexarg2(capsuleHeader->Flags, 8);

        // Construct parsing data
        PARSING_DATA pdata = parsingDataFromQByteArray(QModelIndex());
        pdata.fixed = TRUE;

        // Add tree item
        index = model->addItem(Types::Capsule, Subtypes::UefiCapsule, name, QString(), info, header, body, parsingDataToQByteArray(pdata), root);
    }
    // Check buffer for being extended Aptio signed capsule header
    else if (buffer.startsWith(APTIO_SIGNED_CAPSULE_GUID) || buffer.startsWith(APTIO_UNSIGNED_CAPSULE_GUID)) {
        bool signedCapsule = buffer.startsWith(APTIO_SIGNED_CAPSULE_GUID);
        // Get info
        const APTIO_CAPSULE_HEADER* capsuleHeader = (const APTIO_CAPSULE_HEADER*)buffer.constData();
        capsuleHeaderSize = capsuleHeader->RomImageOffset;
        QByteArray header = buffer.left(capsuleHeaderSize);
        QByteArray body = buffer.mid(capsuleHeaderSize);
        QString name = tr("AMI Aptio capsule");
        QString info = tr("Offset: 0h\nCapsule GUID: %1\nFull size: %2h (%3)\nHeader size: %4h (%5)\nImage size: %6h (%7)\nFlags: %8h")
            .arg(guidToQString(capsuleHeader->CapsuleHeader.CapsuleGuid))
            .hexarg(buffer.size()).arg(buffer.size())
            .hexarg(capsuleHeaderSize).arg(capsuleHeaderSize)
            .hexarg(capsuleHeader->CapsuleHeader.CapsuleImageSize - capsuleHeaderSize).arg(capsuleHeader->CapsuleHeader.CapsuleImageSize - capsuleHeaderSize)
            .hexarg2(capsuleHeader->CapsuleHeader.Flags, 8);

        // Construct parsing data
        PARSING_DATA pdata = parsingDataFromQByteArray(QModelIndex());
        pdata.fixed = TRUE;

        // Add tree item
        index = model->addItem(Types::Capsule, signedCapsule ? Subtypes::AptioSignedCapsule : Subtypes::AptioUnsignedCapsule, name, QString(), info, header, body, parsingDataToQByteArray(pdata), root);

        // Show message about possible Aptio signature break
        if (signedCapsule) {
            msg(tr("parseImageFile: Aptio capsule signature may become invalid after image modifications"), index);
        }
    }
    // Other cases
    else {
        index = root;
    }

    // Skip capsule header to have flash chip image
    QByteArray flashImage = buffer.mid(capsuleHeaderSize);

    // Check for Intel flash descriptor presence
    const FLASH_DESCRIPTOR_HEADER* descriptorHeader = (const FLASH_DESCRIPTOR_HEADER*)flashImage.constData();

    // Check descriptor signature
    STATUS result;
    if (descriptorHeader->Signature == FLASH_DESCRIPTOR_SIGNATURE) {
        // Parse as Intel image
        QModelIndex imageIndex;
        result = parseIntelImage(flashImage, index, imageIndex);
        if (result != ERR_INVALID_FLASH_DESCRIPTOR)
            return result;
    }

    // Get info
    QString name = tr("UEFI image");
    QString info = tr("Offset: %1h\nFull size: %2h (%3)")
        .hexarg(capsuleHeaderSize).hexarg(flashImage.size()).arg(flashImage.size());

    // Construct parsing data
    PARSING_DATA pdata = parsingDataFromQByteArray(index);
    pdata.fixed = TRUE;
    pdata.offset = capsuleHeaderSize;

    // Add tree item
    QModelIndex biosIndex = model->addItem(Types::Image, Subtypes::UefiImage, name, QString(), info, QByteArray(), flashImage, parsingDataToQByteArray(pdata), index);

    // Parse the image
    result = parseRawArea(flashImage, biosIndex);
    if (result)
        return result;

    // Check if the last VTF is found
    if (!lastVtf.isValid()) {
        msg(tr("parseImageFile: not a single Volume Top File is found, physical memory addresses can't be calculated"), biosIndex);
    }
    else {
        return addMemoryAddressesInfo(biosIndex);
    }

    return ERR_SUCCESS;
}

STATUS FfsParser::parseIntelImage(const QByteArray & intelImage, const QModelIndex & parent, QModelIndex & index)
{
    // Sanity check
    if (intelImage.isEmpty())
        return EFI_INVALID_PARAMETER;

    // Get parent's parsing data
    PARSING_DATA pdata = parsingDataFromQByteArray(parent);

    // Store the beginning of descriptor as descriptor base address
    const UINT8* descriptor = (const UINT8*)intelImage.constData();
    UINT32 descriptorBegin = 0;
    UINT32 descriptorEnd = FLASH_DESCRIPTOR_SIZE;

    // Check for buffer size to be greater or equal to descriptor region size
    if (intelImage.size() < FLASH_DESCRIPTOR_SIZE) {
        msg(tr("parseIntelImage: input file is smaller then minimum descriptor size of %1h (%2) bytes").hexarg(FLASH_DESCRIPTOR_SIZE).arg(FLASH_DESCRIPTOR_SIZE));
        return ERR_INVALID_FLASH_DESCRIPTOR;
    }

    // Parse descriptor map
    const FLASH_DESCRIPTOR_MAP* descriptorMap = (const FLASH_DESCRIPTOR_MAP*)(descriptor + sizeof(FLASH_DESCRIPTOR_HEADER));
    const FLASH_DESCRIPTOR_UPPER_MAP*  upperMap = (const FLASH_DESCRIPTOR_UPPER_MAP*)(descriptor + FLASH_DESCRIPTOR_UPPER_MAP_BASE);
    const FLASH_DESCRIPTOR_REGION_SECTION* regionSection = (const FLASH_DESCRIPTOR_REGION_SECTION*)calculateAddress8(descriptor, descriptorMap->RegionBase);
    const FLASH_DESCRIPTOR_MASTER_SECTION* masterSection = (const FLASH_DESCRIPTOR_MASTER_SECTION*)calculateAddress8(descriptor, descriptorMap->MasterBase);

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
        if (biosEnd - biosBegin == (UINT32)intelImage.size()) {
            if (!meEnd) {
                msg(tr("parseIntelImage: can't determine BIOS region start from Gigabyte-specific descriptor"));
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

    // Intel image
    QString name = tr("Intel image");
    QString info = tr("Full size: %1h (%2)\nFlash chips: %3\nRegions: %4\nMasters: %5\nPCH straps: %6\nPROC straps: %7\nICC table entries: %8")
        .hexarg(intelImage.size()).arg(intelImage.size())
        .arg(descriptorMap->NumberOfFlashChips + 1) //
        .arg(descriptorMap->NumberOfRegions + 1)    // Zero-based numbers in storage
        .arg(descriptorMap->NumberOfMasters + 1)    //
        .arg(descriptorMap->NumberOfPchStraps)
        .arg(descriptorMap->NumberOfProcStraps)
        .arg(descriptorMap->NumberOfIccTableEntries);

    // Construct parsing data
    pdata.fixed = TRUE;
    if (pdata.isOnFlash) info.prepend(tr("Offset: %1h\n").hexarg(pdata.offset));

    // Add Intel image tree item
    index = model->addItem(Types::Image, Subtypes::IntelImage, name, QString(), info, QByteArray(), intelImage, parsingDataToQByteArray(pdata), parent);

    // Descriptor
    // Get descriptor info
    QByteArray body = intelImage.left(FLASH_DESCRIPTOR_SIZE);
    name = tr("Descriptor region");
    info = tr("Full size: %1h (%2)").hexarg(FLASH_DESCRIPTOR_SIZE).arg(FLASH_DESCRIPTOR_SIZE);
    if (pdata.isOnFlash) info.prepend(tr("Offset: %1h\n").hexarg(pdata.offset));

    // Check regions presence once again
    QVector<UINT32> offsets;
    if (regionSection->GbeLimit) {
        offsets.append(gbeBegin);
        info += tr("\nGbE region offset:  %1h").hexarg(gbeBegin);
    }
    if (regionSection->MeLimit) {
        offsets.append(meBegin);
        info += tr("\nME region offset:   %1h").hexarg(meBegin);
    }
    if (regionSection->BiosLimit) {
        offsets.append(biosBegin);
        info += tr("\nBIOS region offset: %1h").hexarg(biosBegin);
    }
    if (regionSection->PdrLimit) {
        offsets.append(pdrBegin);
        info += tr("\nPDR region offset:  %1h").hexarg(pdrBegin);
    }

    // Region access settings
    info += tr("\nRegion access settings:");
    info += tr("\nBIOS:%1%2h ME:%3%4h GbE:%5%6h")
        .hexarg2(masterSection->BiosRead, 2)
        .hexarg2(masterSection->BiosWrite, 2)
        .hexarg2(masterSection->MeRead, 2)
        .hexarg2(masterSection->MeWrite, 2)
        .hexarg2(masterSection->GbeRead, 2)
        .hexarg2(masterSection->GbeWrite, 2);

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
    const VSCC_TABLE_ENTRY* vsccTableEntry = (const VSCC_TABLE_ENTRY*)(descriptor + ((UINT16)upperMap->VsccTableBase << 4));
    info += tr("\nFlash chips in VSCC table:");
    UINT8 vsscTableSize = upperMap->VsccTableSize * sizeof(UINT32) / sizeof(VSCC_TABLE_ENTRY);
    for (int i = 0; i < vsscTableSize; i++) {
        info += tr("\n%1%2%3h")
            .hexarg2(vsccTableEntry->VendorId, 2)
            .hexarg2(vsccTableEntry->DeviceId0, 2)
            .hexarg2(vsccTableEntry->DeviceId1, 2);
        vsccTableEntry++;
    }

    // Add descriptor tree item
    model->addItem(Types::Region, Subtypes::DescriptorRegion, name, QString(), info, QByteArray(), body, parsingDataToQByteArray(pdata), index);

    // Sort regions in ascending order
    qSort(offsets);

    // Parse regions
    UINT8 result = 0;
    for (int i = 0; i < offsets.count(); i++) {
        // Parse GbE region
        if (offsets.at(i) == gbeBegin) {
            QModelIndex gbeIndex;
            result = parseGbeRegion(gbe, gbeBegin, index, gbeIndex);
        }
        // Parse ME region
        else if (offsets.at(i) == meBegin) {
            QModelIndex meIndex;
            result = parseMeRegion(me, meBegin, index, meIndex);
        }
        // Parse BIOS region
        else if (offsets.at(i) == biosBegin) {
            QModelIndex biosIndex;
            result = parseBiosRegion(bios, biosBegin, index, biosIndex);
        }
        // Parse PDR region
        else if (offsets.at(i) == pdrBegin) {
            QModelIndex pdrIndex;
            result = parsePdrRegion(pdr, pdrBegin, index, pdrIndex);
        }
        if (result)
            return result;
    }

    // Check if the last VTF is found
    if (!lastVtf.isValid()) {
        msg(tr("parseIntelImage: not a single Volume Top File is found, physical memory addresses can't be calculated"), index);
    }
    else {
        return addMemoryAddressesInfo(index);
    }

    return ERR_SUCCESS;
}

STATUS FfsParser::parseGbeRegion(const QByteArray & gbe, const UINT32 parentOffset, const QModelIndex & parent, QModelIndex & index)
{
    // Check sanity
    if (gbe.isEmpty())
        return ERR_EMPTY_REGION;

    // Get parent's parsing data
    PARSING_DATA pdata = parsingDataFromQByteArray(parent);

    // Get info
    QString name = tr("GbE region");
    const GBE_MAC_ADDRESS* mac = (const GBE_MAC_ADDRESS*)gbe.constData();
    const GBE_VERSION* version = (const GBE_VERSION*)(gbe.constData() + GBE_VERSION_OFFSET);
    QString info = tr("Full size: %1h (%2)\nMAC: %3:%4:%5:%6:%7:%8\nVersion: %9.%10")
        .hexarg(gbe.size()).arg(gbe.size())
        .hexarg2(mac->vendor[0], 2)
        .hexarg2(mac->vendor[1], 2)
        .hexarg2(mac->vendor[2], 2)
        .hexarg2(mac->device[0], 2)
        .hexarg2(mac->device[1], 2)
        .hexarg2(mac->device[2], 2)
        .arg(version->major)
        .arg(version->minor);

    // Construct parsing data
    pdata.fixed = TRUE;
    pdata.offset += parentOffset;
    if (pdata.isOnFlash) info.prepend(tr("Offset: %1h\n").hexarg(pdata.offset));

    // Add tree item
    index = model->addItem(Types::Region, Subtypes::GbeRegion, name, QString(), info, QByteArray(), gbe, parsingDataToQByteArray(pdata), parent);

    return ERR_SUCCESS;
}

STATUS FfsParser::parseMeRegion(const QByteArray & me, const UINT32 parentOffset, const QModelIndex & parent, QModelIndex & index)
{
    // Check sanity
    if (me.isEmpty())
        return ERR_EMPTY_REGION;

    // Get parent's parsing data
    PARSING_DATA pdata = parsingDataFromQByteArray(parent);

    // Get info
    QString name = tr("ME region");
    QString info = tr("Full size: %1h (%2)").
        hexarg(me.size()).arg(me.size());

    // Parse region
    bool versionFound = true;
    bool emptyRegion = false;
    // Check for empty region
    if (me.count() == me.count('\xFF') || me.count() == me.count('\x00')) {
        // Further parsing not needed
        emptyRegion = true;
        info += tr("\nState: empty");
    }
    else {
        // Search for new signature
        INT32 versionOffset = me.indexOf(ME_VERSION_SIGNATURE2);
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
            const ME_VERSION* version = (const ME_VERSION*)(me.constData() + versionOffset);
            info += tr("\nVersion: %1.%2.%3.%4")
                .arg(version->major)
                .arg(version->minor)
                .arg(version->bugfix)
                .arg(version->build);
        }
    }

    // Construct parsing data
    pdata.fixed = TRUE;
    pdata.offset += parentOffset;
    if (pdata.isOnFlash) info.prepend(tr("Offset: %1h\n").hexarg(pdata.offset));

    // Add tree item
    index = model->addItem(Types::Region, Subtypes::MeRegion, name, QString(), info, QByteArray(), me, parsingDataToQByteArray(pdata), parent);

    // Show messages
    if (emptyRegion) {
        msg(tr("parseMeRegion: ME region is empty"), index);
    }
    else if (!versionFound) {
        msg(tr("parseMeRegion: ME version is unknown, it can be damaged"), index);
    }

    return ERR_SUCCESS;
}

STATUS FfsParser::parsePdrRegion(const QByteArray & pdr, const UINT32 parentOffset, const QModelIndex & parent, QModelIndex & index)
{
    // Check sanity
    if (pdr.isEmpty())
        return ERR_EMPTY_REGION;

    // Get parent's parsing data
    PARSING_DATA pdata = parsingDataFromQByteArray(parent);

    // Get info
    QString name = tr("PDR region");
    QString info = tr("Full size: %1h (%2)").
        hexarg(pdr.size()).arg(pdr.size());

    // Construct parsing data
    pdata.fixed = TRUE;
    pdata.offset += parentOffset;
    if (pdata.isOnFlash) info.prepend(tr("Offset: %1h\n").hexarg(pdata.offset));

    // Add tree item
    index = model->addItem(Types::Region, Subtypes::PdrRegion, name, QString(), info, QByteArray(), pdr, parsingDataToQByteArray(pdata), parent);

    // Parse PDR region as BIOS space
    UINT8 result = parseRawArea(pdr, index);
    if (result && result != ERR_VOLUMES_NOT_FOUND && result != ERR_INVALID_VOLUME)
        return result;

    return ERR_SUCCESS;
}

STATUS FfsParser::parseBiosRegion(const QByteArray & bios, const UINT32 parentOffset, const QModelIndex & parent, QModelIndex & index)
{
    // Sanity check
    if (bios.isEmpty())
        return ERR_EMPTY_REGION;

    // Get parent's parsing data
    PARSING_DATA pdata = parsingDataFromQByteArray(parent);

    // Get info
    QString name = tr("BIOS region");
    QString info = tr("Full size: %1h (%2)").
        hexarg(bios.size()).arg(bios.size());

    // Construct parsing data
    pdata.fixed = TRUE;
    pdata.offset += parentOffset;
    if (pdata.isOnFlash) info.prepend(tr("Offset: %1h\n").hexarg(pdata.offset));

    // Add tree item
    index = model->addItem(Types::Region, Subtypes::BiosRegion, name, QString(), info, QByteArray(), bios, parsingDataToQByteArray(pdata), parent);

    return parseRawArea(bios, index);
}

UINT8 FfsParser::getPaddingType(const QByteArray & padding)
{
    if (padding.count('\x00') == padding.count())
        return Subtypes::ZeroPadding;
    if (padding.count('\xFF') == padding.count())
        return Subtypes::OnePadding;
    return Subtypes::DataPadding;
}

STATUS FfsParser::parseRawArea(const QByteArray & data, const QModelIndex & index)
{
    // Sanity check
    if (!index.isValid())
        return ERR_INVALID_PARAMETER;

    // Get parsing data
    PARSING_DATA pdata = parsingDataFromQByteArray(index);
    UINT32 offset = pdata.offset;
    UINT32 headerSize = model->header(index).size();

    // Search for first volume
    STATUS result;
    UINT32 prevVolumeOffset;

    result = findNextVolume(data, 0, prevVolumeOffset);
    if (result)
        return result;

    // First volume is not at the beginning of BIOS space
    QString name;
    QString info;
    if (prevVolumeOffset > 0) {
        // Get info
        QByteArray padding = data.left(prevVolumeOffset);
        name = tr("Padding");
        info = tr("Full size: %1h (%2)")
            .hexarg(padding.size()).arg(padding.size());

        // Construct parsing data
        pdata.fixed = TRUE;
        pdata.offset = offset + headerSize;
        if (pdata.isOnFlash) info.prepend(tr("Offset: %1h\n").hexarg(pdata.offset));

        // Add tree item
        model->addItem(Types::Padding, getPaddingType(padding), name, QString(), info, QByteArray(), padding, parsingDataToQByteArray(pdata), index);
    }

    // Search for and parse all volumes
    UINT32 volumeOffset = prevVolumeOffset;
    UINT32 prevVolumeSize = 0;

    while (!result)
    {
        // Padding between volumes
        if (volumeOffset > prevVolumeOffset + prevVolumeSize) {
            UINT32 paddingOffset = prevVolumeOffset + prevVolumeSize;
            UINT32 paddingSize = volumeOffset - paddingOffset;
            QByteArray padding = data.mid(paddingOffset, paddingSize);

            // Get info
            name = tr("Padding");
            info = tr("Full size: %1h (%2)")
                .hexarg(padding.size()).arg(padding.size());

            // Construct parsing data
            pdata.fixed = TRUE;
            pdata.offset = offset + headerSize + paddingOffset;
            if (pdata.isOnFlash) info.prepend(tr("Offset: %1h\n").hexarg(pdata.offset));

            // Add tree item
            model->addItem(Types::Padding, getPaddingType(padding), name, QString(), info, QByteArray(), padding, parsingDataToQByteArray(pdata), index);
        }

        // Get volume size
        UINT32 volumeSize = 0;
        UINT32 bmVolumeSize = 0;
        result = getVolumeSize(data, volumeOffset, volumeSize, bmVolumeSize);
        if (result)
            return result;

        // Check that volume is fully present in input
        QByteArray volume = data.mid(volumeOffset, volumeSize);
        if (volumeSize > (UINT32)volume.size()) {
            // Mark the rest as padding and finish the parsing
            QByteArray padding = data.right(volume.size());

            // Get info
            name = tr("Padding");
            info = tr("Full size: %1h (%2)")
                .hexarg(padding.size()).arg(padding.size());

            // Construct parsing data
            pdata.fixed = TRUE;
            pdata.offset = offset + headerSize + volumeOffset;
            if (pdata.isOnFlash) info.prepend(tr("Offset: %1h\n").hexarg(pdata.offset));

            // Add tree item
            QModelIndex paddingIndex = model->addItem(Types::Padding, getPaddingType(padding), name, QString(), info, QByteArray(), padding, parsingDataToQByteArray(pdata), index);
            msg(tr("parseRawArea: one of volumes inside overlaps the end of data"), paddingIndex);

            // Update variables
            prevVolumeOffset = volumeOffset;
            prevVolumeSize = padding.size();
            break;
        }

        // Parse current volume's header
        QModelIndex volumeIndex;
        result = parseVolumeHeader(volume, model->header(index).size() + volumeOffset, index, volumeIndex);
        if (result)
            msg(tr("parseRawArea: volume header parsing failed with error \"%1\"").arg(errorCodeToQString(result)), index);
        else {
            // Show messages
            if (volumeSize != bmVolumeSize)
                msg(tr("parseBiosBody: volume size stored in header %1h (%2) differs from calculated using block map %3h (%4)")
                .hexarg(volumeSize).arg(volumeSize)
                .hexarg(bmVolumeSize).arg(bmVolumeSize),
                volumeIndex);
        }

        // Go to next volume
        prevVolumeOffset = volumeOffset;
        prevVolumeSize = volumeSize;
        result = findNextVolume(data, volumeOffset + prevVolumeSize, volumeOffset);
    }

    // Padding at the end of BIOS space
    volumeOffset = prevVolumeOffset + prevVolumeSize;
    if ((UINT32)data.size() > volumeOffset) {
        QByteArray padding = data.mid(volumeOffset);

        // Get info
        name = tr("Padding");
        info = tr("Full size: %1h (%2)")
            .hexarg(padding.size()).arg(padding.size());

        // Construct parsing data
        pdata.fixed = TRUE;
        pdata.offset = offset + headerSize + volumeOffset;
        if (pdata.isOnFlash) info.prepend(tr("Offset: %1h\n").hexarg(pdata.offset));

        // Add tree item
        model->addItem(Types::Padding, getPaddingType(padding), name, QString(), info, QByteArray(), padding, parsingDataToQByteArray(pdata), index);
    }

    //Parse bodies
    for (int i = 0; i < model->rowCount(index); i++) {
        QModelIndex current = index.child(i, 0);
        switch (model->type(current)) {
        case Types::Volume:
            parseVolumeBody(current);
            break;
        case Types::Padding:
            // No parsing required
            break;
        default:
            return ERR_UNKNOWN_ITEM_TYPE;
        }
    }

    return ERR_SUCCESS;
}

STATUS FfsParser::parseVolumeHeader(const QByteArray & volume, const UINT32 parentOffset, const QModelIndex & parent, QModelIndex & index)
{
    // Sanity check
    if (volume.isEmpty())
        return ERR_INVALID_PARAMETER;

    // Get parent's parsing data
    PARSING_DATA pdata = parsingDataFromQByteArray(parent);

    // Populate volume header
    const EFI_FIRMWARE_VOLUME_HEADER* volumeHeader = (const EFI_FIRMWARE_VOLUME_HEADER*)(volume.constData());

    // Calculate volume header size
    UINT32 headerSize;
    EFI_GUID extendedHeaderGuid = {{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }};
    bool hasExtendedHeader = false;
    if (volumeHeader->Revision > 1 && volumeHeader->ExtHeaderOffset) {
        hasExtendedHeader = true;
        const EFI_FIRMWARE_VOLUME_EXT_HEADER* extendedHeader = (const EFI_FIRMWARE_VOLUME_EXT_HEADER*)(volume.constData() + volumeHeader->ExtHeaderOffset);
        headerSize = volumeHeader->ExtHeaderOffset + extendedHeader->ExtHeaderSize;
        extendedHeaderGuid = extendedHeader->FvName;
    }
    else
        headerSize = volumeHeader->HeaderLength;

    // Extended header end can be unaligned
    headerSize = ALIGN8(headerSize);

    // Check for volume structure to be known
    bool isUnknown = true;
    UINT8 ffsVersion = 0;

    // Check for FFS v2 volume
    if (FFSv2Volumes.contains(QByteArray::fromRawData((const char*)volumeHeader->FileSystemGuid.Data, sizeof(EFI_GUID)))) {
        isUnknown = false;
        ffsVersion = 2;
    }

    // Check for FFS v3 volume
    if (FFSv3Volumes.contains(QByteArray::fromRawData((const char*)volumeHeader->FileSystemGuid.Data, sizeof(EFI_GUID)))) {
        isUnknown = false;
        ffsVersion = 3;
    }

    // Check volume revision and alignment
    bool msgAlignmentBitsSet = false;
    bool msgUnaligned = false;
    bool msgUnknownRevision = false;
    UINT32 alignment = 65536; // Default volume alignment is 64K
    if (volumeHeader->Revision == 1) {
        // Acquire alignment capability bit
        bool alignmentCap = volumeHeader->Attributes & EFI_FVB_ALIGNMENT_CAP;
        if (!alignmentCap) {
            if ((volumeHeader->Attributes & 0xFFFF0000))
                msgAlignmentBitsSet = true;
        }
        // Do not check for volume alignment on revision 1 volumes
        // No one gives a single crap about setting it correctly
        /*else {
            if (volumeHeader->Attributes & EFI_FVB_ALIGNMENT_2) alignment = 2;
            if (volumeHeader->Attributes & EFI_FVB_ALIGNMENT_4) alignment = 4;
            if (volumeHeader->Attributes & EFI_FVB_ALIGNMENT_8) alignment = 8;
            if (volumeHeader->Attributes & EFI_FVB_ALIGNMENT_16) alignment = 16;
            if (volumeHeader->Attributes & EFI_FVB_ALIGNMENT_32) alignment = 32;
            if (volumeHeader->Attributes & EFI_FVB_ALIGNMENT_64) alignment = 64;
            if (volumeHeader->Attributes & EFI_FVB_ALIGNMENT_128) alignment = 128;
            if (volumeHeader->Attributes & EFI_FVB_ALIGNMENT_256) alignment = 256;
            if (volumeHeader->Attributes & EFI_FVB_ALIGNMENT_512) alignment = 512;
            if (volumeHeader->Attributes & EFI_FVB_ALIGNMENT_1K) alignment = 1024;
            if (volumeHeader->Attributes & EFI_FVB_ALIGNMENT_2K) alignment = 2048;
            if (volumeHeader->Attributes & EFI_FVB_ALIGNMENT_4K) alignment = 4096;
            if (volumeHeader->Attributes & EFI_FVB_ALIGNMENT_8K) alignment = 8192;
            if (volumeHeader->Attributes & EFI_FVB_ALIGNMENT_16K) alignment = 16384;
            if (volumeHeader->Attributes & EFI_FVB_ALIGNMENT_32K) alignment = 32768;
            if (volumeHeader->Attributes & EFI_FVB_ALIGNMENT_64K) alignment = 65536;
        }*/
    }
    else if (volumeHeader->Revision == 2) {
        // Acquire alignment
        alignment = (UINT32)pow(2.0, (int)(volumeHeader->Attributes & EFI_FVB2_ALIGNMENT) >> 16);
        // Check alignment
        if (!isUnknown && pdata.isOnFlash && ((pdata.offset + parentOffset) % alignment))
            msgUnaligned = true;
    }
    else
        msgUnknownRevision = true;

    // Check attributes
    // Determine value of empty byte
    UINT8 emptyByte = volumeHeader->Attributes & EFI_FVB_ERASE_POLARITY ? '\xFF' : '\x00';

    // Check for Apple CRC32 in ZeroVector
    bool hasZeroVectorCRC32 = false;
    UINT32 volumeSize = volume.size();
    UINT32 crc32FromZeroVector = *(UINT32*)(volume.constData() + 8);
    if (crc32FromZeroVector != 0) {
        // Calculate CRC32 of the volume body
        UINT32 crc = crc32(0, (const UINT8*)(volume.constData() + volumeHeader->HeaderLength), volumeSize - volumeHeader->HeaderLength);
        if (crc == crc32FromZeroVector) {
            hasZeroVectorCRC32 = true;
        }
    }

    // Check header checksum by recalculating it
    bool msgInvalidChecksum = false;
    if (calculateChecksum16((const UINT16*)volumeHeader, volumeHeader->HeaderLength))
        msgInvalidChecksum = true;

    // Get info
    QByteArray header = volume.left(headerSize);
    QByteArray body = volume.mid(headerSize);
    QString name = guidToQString(volumeHeader->FileSystemGuid);
    QString info = tr("ZeroVector:\n%1 %2 %3 %4 %5 %6 %7 %8\n%9 %10 %11 %12 %13 %14 %15 %16\nFileSystem GUID: %17\nFull size: %18h (%19)\n"
        "Header size: %20h (%21)\nBody size: %22h (%23)\nRevision: %24\nAttributes: %25h\nErase polarity: %26")
        .hexarg2(volumeHeader->ZeroVector[0], 2).hexarg2(volumeHeader->ZeroVector[1], 2).hexarg2(volumeHeader->ZeroVector[2], 2).hexarg2(volumeHeader->ZeroVector[3], 2)
        .hexarg2(volumeHeader->ZeroVector[4], 2).hexarg2(volumeHeader->ZeroVector[5], 2).hexarg2(volumeHeader->ZeroVector[6], 2).hexarg2(volumeHeader->ZeroVector[7], 2)
        .hexarg2(volumeHeader->ZeroVector[8], 2).hexarg2(volumeHeader->ZeroVector[9], 2).hexarg2(volumeHeader->ZeroVector[10], 2).hexarg2(volumeHeader->ZeroVector[11], 2)
        .hexarg2(volumeHeader->ZeroVector[12], 2).hexarg2(volumeHeader->ZeroVector[13], 2).hexarg2(volumeHeader->ZeroVector[14], 2).hexarg2(volumeHeader->ZeroVector[15], 2)
        .arg(guidToQString(volumeHeader->FileSystemGuid))
        .hexarg(volumeSize).arg(volumeSize)
        .hexarg(headerSize).arg(headerSize)
        .hexarg(volumeSize - headerSize).arg(volumeSize - headerSize)
        .arg(volumeHeader->Revision)
        .hexarg2(volumeHeader->Attributes, 8)
        .arg(emptyByte ? "1" : "0");

    // Apple CRC32 volume
    if (hasZeroVectorCRC32) {
        info += tr("\nCRC32 in ZeroVector: valid");
    }

    // Extended header present
    if (volumeHeader->Revision > 1 && volumeHeader->ExtHeaderOffset) {
        const EFI_FIRMWARE_VOLUME_EXT_HEADER* extendedHeader = (const EFI_FIRMWARE_VOLUME_EXT_HEADER*)(volume.constData() + volumeHeader->ExtHeaderOffset);
        info += tr("\nExtended header size: %1h (%2)\nVolume GUID: %3")
            .hexarg(extendedHeader->ExtHeaderSize).arg(extendedHeader->ExtHeaderSize)
            .arg(guidToQString(extendedHeader->FvName));
    }

    // Construct parsing data
    pdata.fixed = TRUE;
    pdata.offset += parentOffset;
    pdata.emptyByte = emptyByte;
    pdata.ffsVersion = ffsVersion;
    pdata.volume.hasExtendedHeader = hasExtendedHeader ? TRUE : FALSE;
    pdata.volume.extendedHeaderGuid = extendedHeaderGuid;
    pdata.volume.alignment = alignment;
    pdata.volume.revision = volumeHeader->Revision;
    pdata.volume.hasZeroVectorCRC32 = hasZeroVectorCRC32;
    pdata.volume.isWeakAligned = (volumeHeader->Revision > 1 && (volumeHeader->Attributes & EFI_FVB2_WEAK_ALIGNMENT));
    if (pdata.isOnFlash) info.prepend(tr("Offset: %1h\n").hexarg(pdata.offset));

    // Add text
    QString text;
    if (hasZeroVectorCRC32)
        text += tr("ZeroVectorCRC32 ");

    // Add tree item
    UINT8 subtype = Subtypes::UnknownVolume;
    if (!isUnknown) {
        if (ffsVersion == 2)
            subtype = Subtypes::Ffs2Volume;
        else if (ffsVersion == 3)
            subtype = Subtypes::Ffs3Volume;
    }
    index = model->addItem(Types::Volume, subtype, name, text, info, header, body, parsingDataToQByteArray(pdata), parent);

    // Show messages
    if (isUnknown)
        msg(tr("parseVolumeHeader: unknown file system %1").arg(guidToQString(volumeHeader->FileSystemGuid)), index);
    if (msgInvalidChecksum)
        msg(tr("parseVolumeHeader: volume header checksum is invalid"), index);
    if (msgAlignmentBitsSet)
        msg(tr("parseVolumeHeader: alignment bits set on volume without alignment capability"), index);
    if (msgUnaligned)
        msg(tr("parseVolumeHeader: unaligned volume"), index);
    if (msgUnknownRevision)
        msg(tr("parseVolumeHeader: unknown volume revision %1").arg(volumeHeader->Revision), index);

    return ERR_SUCCESS;
}

STATUS FfsParser::findNextVolume(const QByteArray & bios, UINT32 volumeOffset, UINT32 & nextVolumeOffset)
{
    int nextIndex = bios.indexOf(EFI_FV_SIGNATURE, volumeOffset);
    if (nextIndex < EFI_FV_SIGNATURE_OFFSET)
        return ERR_VOLUMES_NOT_FOUND;

    nextVolumeOffset = nextIndex - EFI_FV_SIGNATURE_OFFSET;
    return ERR_SUCCESS;
}

STATUS FfsParser::getVolumeSize(const QByteArray & bios, UINT32 volumeOffset, UINT32 & volumeSize, UINT32 & bmVolumeSize)
{
    // Populate volume header
    const EFI_FIRMWARE_VOLUME_HEADER* volumeHeader = (const EFI_FIRMWARE_VOLUME_HEADER*)(bios.constData() + volumeOffset);

    // Check volume signature
    if (QByteArray((const char*)&volumeHeader->Signature, sizeof(volumeHeader->Signature)) != EFI_FV_SIGNATURE)
        return ERR_INVALID_VOLUME;

    // Calculate volume size using BlockMap
    const EFI_FV_BLOCK_MAP_ENTRY* entry = (const EFI_FV_BLOCK_MAP_ENTRY*)(bios.constData() + volumeOffset + sizeof(EFI_FIRMWARE_VOLUME_HEADER));
    UINT32 calcVolumeSize = 0;
    while (entry->NumBlocks != 0 && entry->Length != 0) {
        if ((void*)entry > bios.constData() + bios.size())
            return ERR_INVALID_VOLUME;

        calcVolumeSize += entry->NumBlocks * entry->Length;
        entry += 1;
    }

    volumeSize = volumeHeader->FvLength;
    bmVolumeSize = calcVolumeSize;
    return ERR_SUCCESS;
}

STATUS FfsParser::parseVolumeBody(const QModelIndex & index)
{
    // Sanity check
    if (!index.isValid())
        return ERR_INVALID_PARAMETER;

    // Get volume header size and body
    QByteArray volumeBody = model->body(index);
    UINT32 volumeHeaderSize = model->header(index).size();

    // Get parsing data
    PARSING_DATA pdata = parsingDataFromQByteArray(index);
    UINT32 offset = pdata.offset;

    if (pdata.ffsVersion != 2 && pdata.ffsVersion != 3) // Don't parse unknown volumes
        return ERR_SUCCESS;

    // Search for and parse all files
    UINT32 volumeBodySize = volumeBody.size();
    UINT32 fileOffset = 0;
    
    while (fileOffset < volumeBodySize) {
        UINT32 fileSize = getFileSize(volumeBody, fileOffset, pdata.ffsVersion);
        // Check file size 
        if (fileSize < sizeof(EFI_FFS_FILE_HEADER) || fileSize > volumeBodySize - fileOffset) {
            // Check that we are at the empty space
            QByteArray header = volumeBody.mid(fileOffset, sizeof(EFI_FFS_FILE_HEADER));
            if (header.count(pdata.emptyByte) == header.size()) { //Empty space
                // Check free space to be actually free
                QByteArray freeSpace = volumeBody.mid(fileOffset);
                if (freeSpace.count(pdata.emptyByte) != freeSpace.count()) {
                    // Search for the first non-empty byte
                    UINT32 i;
                    UINT32 size = freeSpace.size();
                    const UINT8* current = (UINT8*)freeSpace.constData();
                    for (i = 0; i < size; i++) {
                        if (*current++ != pdata.emptyByte)
                            break;
                    }

                    // Align found index to file alignment
                    // It must be possible because minimum 16 bytes of empty were found before
                    if (i != ALIGN8(i))
                        i = ALIGN8(i) - 8;

                    // Construct parsing data
                    pdata.fixed = FALSE; // Free space is not fixed
                    pdata.offset = offset + volumeHeaderSize + fileOffset;

                    // Add all bytes before as free space...
                    if (i > 0) {
                        QByteArray free = freeSpace.left(i);

                        // Get info
                        QString info = tr("Full size: %1h (%2)").hexarg(free.size()).arg(free.size());
                        if (pdata.isOnFlash) info.prepend(tr("Offset: %1h\n").hexarg(pdata.offset));

                        // Add free space item
                        model->addItem(Types::FreeSpace, 0, tr("Volume free space"), "", info, QByteArray(), free, parsingDataToQByteArray(pdata), index);
                    }
                    // ... and all bytes after as a padding
                    pdata.fixed = TRUE; // Non-UEFI data is fixed
                    pdata.offset += i;
                    QByteArray padding = freeSpace.mid(i);

                    // Get info
                    QString info = tr("Full size: %1h (%2)").hexarg(padding.size()).arg(padding.size());
                    if (pdata.isOnFlash) info.prepend(tr("Offset: %1h\n").hexarg(pdata.offset));

                    // Add padding tree item
                    QModelIndex dataIndex = model->addItem(Types::Padding, Subtypes::DataPadding, tr("Non-UEFI data"), "", info, QByteArray(), padding, parsingDataToQByteArray(pdata), index);
                    msg(tr("parseVolumeBody: non-UEFI data found in volume's free space"), dataIndex);
                }
                else {
                    // Construct parsing data
                    pdata.fixed = FALSE; // Free space is not fixed
                    pdata.offset = offset + volumeHeaderSize + fileOffset;

                    // Get info
                    QString info = tr("Full size: %1h (%2)").hexarg(freeSpace.size()).arg(freeSpace.size());
                    if (pdata.isOnFlash) info.prepend(tr("Offset: %1h\n").hexarg(pdata.offset));

                    // Add free space item
                    model->addItem(Types::FreeSpace, 0, tr("Volume free space"), "", info, QByteArray(), freeSpace, parsingDataToQByteArray(pdata), index);
                }
                break; // Exit from parsing loop
            }
            else { //File space
                // Add padding to the end of the volume
                pdata.fixed = TRUE; // Non-UEFI data is fixed
                pdata.offset = offset + volumeHeaderSize + fileOffset;
                QByteArray padding = volumeBody.mid(fileOffset);

                // Get info
                QString info = tr("Full size: %1h (%2)").hexarg(padding.size()).arg(padding.size());
                if (pdata.isOnFlash) info.prepend(tr("Offset: %1h\n").hexarg(pdata.offset));

                // Add padding tree item
                QModelIndex dataIndex = model->addItem(Types::Padding, Subtypes::DataPadding, tr("Non-UEFI data"), "", info, QByteArray(), padding, parsingDataToQByteArray(pdata), index);

                // Show message
                msg(tr("parseVolumeBody: non-UEFI data found inside volume's file space"), dataIndex);

                break; // Exit from parsing loop
            }
        }

        // Get file header
        QByteArray file = volumeBody.mid(fileOffset, fileSize);
        QByteArray header = file.left(sizeof(EFI_FFS_FILE_HEADER));
        const EFI_FFS_FILE_HEADER* fileHeader = (const EFI_FFS_FILE_HEADER*)header.constData();
        if (pdata.ffsVersion == 3 && (fileHeader->Attributes & FFS_ATTRIB_LARGE_FILE)) {
            header = file.left(sizeof(EFI_FFS_FILE_HEADER2));
        }

        //Parse current file's header
        QModelIndex fileIndex;
        STATUS result = parseFileHeader(file, volumeHeaderSize + fileOffset, index, fileIndex);
        if (result)
            msg(tr("parseVolumeBody: file header parsing failed with error \"%1\"").arg(errorCodeToQString(result)), index);

        // Move to next file
        fileOffset += fileSize;
        fileOffset = ALIGN8(fileOffset);
    }

    // Check for duplicate GUIDs
    for (int i = 0; i < model->rowCount(index); i++) {
        QModelIndex current = index.child(i, 0);
        // Skip non-file entries and pad files
        if (model->type(current) != Types::File || model->subtype(current) == EFI_FV_FILETYPE_PAD)
            continue;
        QByteArray currentGuid = model->header(current).left(sizeof(EFI_GUID));
        // Check files after current for having the same GUID
        for (int j = i + 1; j < model->rowCount(index); j++) {
            QModelIndex another = index.child(j, 0);
            // Skip non-file entries
            if (model->type(another) != Types::File)
                continue;
            // Check GUIDs for being same
            QByteArray anotherGuid = model->header(another).left(sizeof(EFI_GUID));
            if (currentGuid == anotherGuid) {
                msg(tr("parseVolumeBody: file with duplicate GUID %1").arg(guidToQString(*(const EFI_GUID*)anotherGuid.constData())), another);
            }
        }
    }

    //Parse bodies
    for (int i = 0; i < model->rowCount(index); i++) {
        QModelIndex current = index.child(i, 0);
        switch (model->type(current)) {
        case Types::File:
            parseFileBody(current);
            break;
        case Types::Padding:
        case Types::FreeSpace:
            // No parsing required
            break;
        default:
            return ERR_UNKNOWN_ITEM_TYPE;
        }
    }

    return ERR_SUCCESS;
}

UINT32 FfsParser::getFileSize(const QByteArray & volume, const UINT32 fileOffset, const UINT8 ffsVersion)
{
    if (ffsVersion == 2) {
        const EFI_FFS_FILE_HEADER* fileHeader = (const EFI_FFS_FILE_HEADER*)(volume.constData() + fileOffset);
        return uint24ToUint32(fileHeader->Size);
    }
    else if (ffsVersion == 3) {
        const EFI_FFS_FILE_HEADER2* fileHeader = (const EFI_FFS_FILE_HEADER2*)(volume.constData() + fileOffset);
        if (fileHeader->Attributes & FFS_ATTRIB_LARGE_FILE)
            return fileHeader->ExtendedSize;
        else
            return uint24ToUint32(fileHeader->Size);
    }
    else
        return 0;
}

STATUS FfsParser::parseFileHeader(const QByteArray & file, const UINT32 parentOffset, const QModelIndex & parent, QModelIndex & index)
{
    // Sanity check
    if (file.isEmpty())
        return ERR_INVALID_PARAMETER;

    // Get parent's parsing data
    PARSING_DATA pdata = parsingDataFromQByteArray(parent);

    // Get file header
    QByteArray header = file.left(sizeof(EFI_FFS_FILE_HEADER));
    const EFI_FFS_FILE_HEADER* fileHeader = (const EFI_FFS_FILE_HEADER*)header.constData();
    if (pdata.ffsVersion == 3 && (fileHeader->Attributes & FFS_ATTRIB_LARGE_FILE)) {
        header = file.left(sizeof(EFI_FFS_FILE_HEADER2));
    }

    // Check file alignment
    bool msgUnalignedFile = false;
    UINT8 alignmentPower = ffsAlignmentTable[(fileHeader->Attributes & FFS_ATTRIB_DATA_ALIGNMENT) >> 3];
    UINT32 alignment = (UINT32)pow(2.0, alignmentPower);
    if ((parentOffset + header.size()) % alignment)
        msgUnalignedFile = true;

    // Check file alignment agains volume alignment
    bool msgFileAlignmentIsGreaterThenVolumes = false;
    if (!pdata.volume.isWeakAligned && pdata.volume.alignment < alignment)
        msgFileAlignmentIsGreaterThenVolumes = true;

    // Check header checksum
    QByteArray tempHeader = header;
    EFI_FFS_FILE_HEADER* tempFileHeader = (EFI_FFS_FILE_HEADER*)(tempHeader.data());
    tempFileHeader->IntegrityCheck.Checksum.Header = 0;
    tempFileHeader->IntegrityCheck.Checksum.File = 0;
    UINT8 calculated = calculateChecksum8((const UINT8*)tempFileHeader, header.size() - 1);
    bool msgInvalidHeaderChecksum = false;
    if (fileHeader->IntegrityCheck.Checksum.Header != calculated)
        msgInvalidHeaderChecksum = true;

    // Check data checksum
    // Data checksum must be calculated
    bool msgInvalidDataChecksum = false;
    if (fileHeader->Attributes & FFS_ATTRIB_CHECKSUM) {
        UINT32 bufferSize = file.size() - header.size();
        // Exclude file tail from data checksum calculation
        if (pdata.volume.revision == 1 && (fileHeader->Attributes & FFS_ATTRIB_TAIL_PRESENT))
            bufferSize -= sizeof(UINT16);
        calculated = calculateChecksum8((const UINT8*)(file.constData() + header.size()), bufferSize);
        if (fileHeader->IntegrityCheck.Checksum.File != calculated)
            msgInvalidDataChecksum = true;
    }
    // Data checksum must be one of predefined values
    else if (pdata.volume.revision == 1 && fileHeader->IntegrityCheck.Checksum.File != FFS_FIXED_CHECKSUM)
        msgInvalidDataChecksum = true;
    else if (pdata.volume.revision == 2 && fileHeader->IntegrityCheck.Checksum.File != FFS_FIXED_CHECKSUM2)
        msgInvalidDataChecksum = true;

    // Check file type
    bool msgUnknownType = false;
    if (fileHeader->Type > EFI_FV_FILETYPE_SMM_CORE && fileHeader->Type != EFI_FV_FILETYPE_PAD) {
        msgUnknownType = true;
    };

    // Get file body
    QByteArray body = file.mid(header.size());

    // Check for file tail presence
    UINT16 tail = 0;
    bool msgInvalidTailValue = false;
    bool hasTail = false;
    if (pdata.volume.revision == 1 && (fileHeader->Attributes & FFS_ATTRIB_TAIL_PRESENT))
    {
        hasTail = true;

        //Check file tail;
        tail = *(UINT16*)body.right(sizeof(UINT16)).constData();
        if (fileHeader->IntegrityCheck.TailReference != (UINT16)~tail)
            msgInvalidTailValue = true;

        // Remove tail from file body
        body = body.left(body.size() - sizeof(UINT16));
    }

    // Get info
    QString name;
    QString info;
    if (fileHeader->Type != EFI_FV_FILETYPE_PAD)
        name = guidToQString(fileHeader->Name);
    else
        name = tr("Pad-file");

    info = tr("File GUID: %1\nType: %2h\nAttributes: %3h\nFull size: %4h (%5)\nHeader size: %6h (%7)\nBody size: %8h (%9)\nState: %10h")
        .arg(guidToQString(fileHeader->Name))
        .hexarg2(fileHeader->Type, 2)
        .hexarg2(fileHeader->Attributes, 2)
        .hexarg(header.size() + body.size()).arg(header.size() + body.size())
        .hexarg(header.size()).arg(header.size())
        .hexarg(body.size()).arg(body.size())
        .hexarg2(fileHeader->State, 2);

    // Check if the file is a Volume Top File
    QString text;
    bool isVtf = false;
    if (EFI_FFS_VOLUME_TOP_FILE_GUID == header.left(sizeof(EFI_GUID))) {
        // Mark it as the last VTF
        // This information will later be used to determine memory addresses of uncompressed image elements
        // Because the last byte of the last VFT is mapped to 0xFFFFFFFF physical memory address 
        isVtf = true;
        text = tr("Volume Top File");
    }

    // Construct parsing data
    pdata.fixed = fileHeader->Attributes & FFS_ATTRIB_FIXED;
    pdata.offset += parentOffset;
    pdata.file.hasTail = hasTail ? TRUE : FALSE;
    pdata.file.tail = tail;
    if (pdata.isOnFlash) info.prepend(tr("Offset: %1h\n").hexarg(pdata.offset));

    // Add tree item
    index = model->addItem(Types::File, fileHeader->Type, name, text, info, header, body, parsingDataToQByteArray(pdata), parent);

    // Overwrite lastVtf, if needed
    if (isVtf) {
        lastVtf = index;
    }

    // Show messages
    if (msgUnalignedFile)
        msg(tr("parseFileHeader: unaligned file"), index);
    if (msgFileAlignmentIsGreaterThenVolumes)
        msg(tr("parseFileHeader: file alignment %1h is greater than parent volume alignment %2h").hexarg(alignment).hexarg(pdata.volume.alignment), index);
    if (msgInvalidHeaderChecksum)
        msg(tr("parseFileHeader: invalid header checksum"), index);
    if (msgInvalidDataChecksum)
        msg(tr("parseFileHeader: invalid data checksum"), index);
    if (msgInvalidTailValue)
        msg(tr("parseFileHeader: invalid tail value"), index);
    if (msgUnknownType)
        msg(tr("parseFileHeader: unknown file type %1h").hexarg2(fileHeader->Type, 2), index);

    return ERR_SUCCESS;
}

UINT32 FfsParser::getSectionSize(const QByteArray & file, const UINT32 sectionOffset, const UINT8 ffsVersion)
{
    if (ffsVersion == 2) {
        const EFI_COMMON_SECTION_HEADER* sectionHeader = (const EFI_COMMON_SECTION_HEADER*)(file.constData() + sectionOffset);
        return uint24ToUint32(sectionHeader->Size);
    }
    else if (ffsVersion == 3) {
        const EFI_COMMON_SECTION_HEADER2* sectionHeader = (const EFI_COMMON_SECTION_HEADER2*)(file.constData() + sectionOffset);
        UINT32 size = uint24ToUint32(sectionHeader->Size);
        if (size == EFI_SECTION2_IS_USED)
            return sectionHeader->ExtendedSize;
        else
            return size;
    }
    else
        return 0;
}

STATUS FfsParser::parseFileBody(const QModelIndex & index)
{
    // Sanity check
    if (!index.isValid())
        return ERR_INVALID_PARAMETER;

    // Do not parse non-file bodies
    if (model->type(index) != Types::File)
        return ERR_SUCCESS;

    // Parse pad-file body
    if (model->subtype(index) == EFI_FV_FILETYPE_PAD)
        return parsePadFileBody(index);

    // Parse raw files as raw areas
    if (model->subtype(index) == EFI_FV_FILETYPE_RAW || model->subtype(index) == EFI_FV_FILETYPE_ALL)
        return parseRawArea(model->body(index), index);

    // Parse sections
    return parseSections(model->body(index), index);
}

STATUS FfsParser::parsePadFileBody(const QModelIndex & index)
{
    // Sanity check
    if (!index.isValid())
        return ERR_INVALID_PARAMETER;

    // Get data from parsing data
    PARSING_DATA pdata = parsingDataFromQByteArray(index);

    // Check if all bytes of the file are empty
    QByteArray body = model->body(index);
    if (body.size() == body.count(pdata.emptyByte))
        return ERR_SUCCESS;

    // Search for the first non-empty byte
    UINT32 i;
    UINT32 size = body.size();
    const UINT8* current = (const UINT8*)body.constData();
    for (i = 0; i < size; i++) {
        if (*current++ != pdata.emptyByte)
            break;
    }

    // Add all bytes before as free space...
    if (i >= 8) {
        // Align free space to 8 bytes boundary
        if (i != ALIGN8(i))
            i = ALIGN8(i) - 8;

        QByteArray free = body.left(i);

        // Get info
        QString info = tr("Full size: %1h (%2)").hexarg(free.size()).arg(free.size());

        // Constuct parsing data
        pdata.offset += model->header(index).size();
        if (pdata.isOnFlash) info.prepend(tr("Offset: %1h\n").hexarg(pdata.offset));

        // Add tree item
        model->addItem(Types::FreeSpace, 0, tr("Free space"), QString(), info, QByteArray(), free, parsingDataToQByteArray(pdata), index);
    }
    else 
        i = 0;

    // ... and all bytes after as a padding
    QByteArray padding = body.mid(i);

    // Get info
    QString info = tr("Full size: %1h (%2)").hexarg(padding.size()).arg(padding.size());

    // Constuct parsing data
    pdata.offset += i;
    pdata.fixed = TRUE; // This data is fixed
    if (pdata.isOnFlash) info.prepend(tr("Offset: %1h\n").hexarg(pdata.offset));

    // Add tree item
    QModelIndex dataIndex = model->addItem(Types::Padding, Subtypes::DataPadding, tr("Non-UEFI data"), "", info, QByteArray(), padding, parsingDataToQByteArray(pdata), index);

    // Show message
    msg(tr("parsePadFileBody: non-UEFI data found in pad-file"), dataIndex);

    // Rename the file
    model->setName(index, tr("Non-empty pad-file"));

    return ERR_SUCCESS;
}

STATUS FfsParser::parseSections(QByteArray sections, const QModelIndex & index)
{
    // Sanity check
    if (!index.isValid())
        return ERR_INVALID_PARAMETER;

    // Get data from parsing data
    PARSING_DATA pdata = parsingDataFromQByteArray(index);

    // Search for and parse all sections
    UINT32 bodySize = sections.size();
    UINT32 headerSize = model->header(index).size();
    UINT32 sectionOffset = 0;

    while (sectionOffset < bodySize) {
        // Get section size
        UINT32 sectionSize = getSectionSize(sections, sectionOffset, pdata.ffsVersion);

        // Check section size
        if (sectionSize < sizeof(EFI_COMMON_SECTION_HEADER) || sectionSize > (bodySize - sectionOffset)) {
            // Add padding to fill the rest of sections
            QByteArray padding = sections.mid(sectionOffset);
            // Get info
            QString info = tr("Full size: %1h (%2)").hexarg(padding.size()).arg(padding.size());

            // Constuct parsing data
            pdata.fixed = TRUE; // Non-UEFI data is fixed
            pdata.offset += headerSize + sectionOffset;
            if (pdata.isOnFlash) info.prepend(tr("Offset: %1h\n").hexarg(pdata.offset));

            // Add tree item
            QModelIndex dataIndex = model->addItem(Types::Padding, Subtypes::DataPadding, tr("Non-UEFI data"), "", info, QByteArray(), padding, parsingDataToQByteArray(pdata), index);

            // Show message
            msg(tr("parseSections: non-UEFI data found in sections area"), dataIndex);

            break; // Exit from parsing loop
        }

        // Parse section header
        QModelIndex sectionIndex;
        STATUS result = parseSectionHeader(sections.mid(sectionOffset, sectionSize), headerSize + sectionOffset, index, sectionIndex);
        if (result)
            msg(tr("parseSections: section header parsing failed with error \"%1\"").arg(errorCodeToQString(result)), index);

        // Move to next section
        sectionOffset += sectionSize;
        sectionOffset = ALIGN4(sectionOffset);
    }

    //Parse bodies
    for (int i = 0; i < model->rowCount(index); i++) {
        QModelIndex current = index.child(i, 0);
        switch (model->type(current)) {
        case Types::Section:
            parseSectionBody(current);
            break;
        case Types::Padding:
            // No parsing required
            break;
        default:
            return ERR_UNKNOWN_ITEM_TYPE;
        }
    }
    
    return ERR_SUCCESS;
}

STATUS FfsParser::parseSectionHeader(const QByteArray & section, const UINT32 parentOffset, const QModelIndex & parent, QModelIndex & index)
{
    const EFI_COMMON_SECTION_HEADER* sectionHeader = (const EFI_COMMON_SECTION_HEADER*)(section.constData());

    switch (sectionHeader->Type) {
    // Special
    case EFI_SECTION_COMPRESSION:           return parseCompressedSectionHeader(section, parentOffset, parent, index);
    case EFI_SECTION_GUID_DEFINED:          return parseGuidedSectionHeader(section, parentOffset, parent, index);
    case EFI_SECTION_FREEFORM_SUBTYPE_GUID: return parseFreeformGuidedSectionHeader(section, parentOffset, parent, index);
    case EFI_SECTION_VERSION:               return parseVersionSectionHeader(section, parentOffset, parent, index);
    case SCT_SECTION_POSTCODE:
    case INSYDE_SECTION_POSTCODE:           return parsePostcodeSectionHeader(section, parentOffset, parent, index);
    // Common
    case EFI_SECTION_DISPOSABLE:
    case EFI_SECTION_DXE_DEPEX:
    case EFI_SECTION_PEI_DEPEX:
    case EFI_SECTION_SMM_DEPEX:
    case EFI_SECTION_PE32:
    case EFI_SECTION_PIC:
    case EFI_SECTION_TE:
    case EFI_SECTION_COMPATIBILITY16:
    case EFI_SECTION_USER_INTERFACE:
    case EFI_SECTION_FIRMWARE_VOLUME_IMAGE:
    case EFI_SECTION_RAW:                   return parseCommonSectionHeader(section, parentOffset, parent, index);
    // Unknown 
    default: 
        STATUS result = parseCommonSectionHeader(section, parentOffset, parent, index);
        msg(tr("parseSectionHeader: section with unknown type %1h").hexarg2(sectionHeader->Type, 2), index);
        return result;
    }
}

STATUS FfsParser::parseCommonSectionHeader(const QByteArray & section, const UINT32 parentOffset, const QModelIndex & parent, QModelIndex & index)
{
    // Get data from parent's parsing data
    PARSING_DATA pdata = parsingDataFromQByteArray(parent);

    // Obtain header fields
    const EFI_COMMON_SECTION_HEADER* sectionHeader = (const EFI_COMMON_SECTION_HEADER*)(section.constData());
    UINT32 headerSize = sizeof(EFI_COMMON_SECTION_HEADER);
    if (pdata.ffsVersion == 3 && uint24ToUint32(sectionHeader->Size) == EFI_SECTION2_IS_USED)
        headerSize = sizeof(EFI_COMMON_SECTION_HEADER2);

    QByteArray header = section.left(headerSize);
    QByteArray body = section.mid(headerSize);

    // Get info
    QString name = sectionTypeToQString(sectionHeader->Type) + tr(" section");
    QString info = tr("Type: %1h\nFull size: %2h (%3)\nHeader size: %4h (%5)\nBody size: %6h (%7)")
        .hexarg2(sectionHeader->Type, 2)
        .hexarg(section.size()).arg(section.size())
        .hexarg(headerSize).arg(headerSize)
        .hexarg(body.size()).arg(body.size());

    // Construct parsing data
    pdata.offset += parentOffset;
    if (pdata.isOnFlash) info.prepend(tr("Offset: %1h\n").hexarg(pdata.offset));

    // Add tree item
    index = model->addItem(Types::Section, sectionHeader->Type, name, QString(), info, header, body, parsingDataToQByteArray(pdata), parent);

    return ERR_SUCCESS;
}

STATUS FfsParser::parseCompressedSectionHeader(const QByteArray & section, const UINT32 parentOffset, const QModelIndex & parent, QModelIndex & index)
{
    // Get data from parent's parsing data
    PARSING_DATA pdata = parsingDataFromQByteArray(parent);

    // Obtain header fields
    const EFI_COMMON_SECTION_HEADER* sectionHeader = (const EFI_COMMON_SECTION_HEADER*)(section.constData());
    const EFI_COMPRESSION_SECTION* compressedSectionHeader = (const EFI_COMPRESSION_SECTION*)sectionHeader;
    UINT32 headerSize = sizeof(EFI_COMPRESSION_SECTION);
    UINT8 compressionType = compressedSectionHeader->CompressionType;
    UINT32 uncompressedLength = compressedSectionHeader->UncompressedLength;
    if (pdata.ffsVersion == 3 && uint24ToUint32(sectionHeader->Size) == EFI_SECTION2_IS_USED) {
        const EFI_COMPRESSION_SECTION2* compressedSectionHeader2 = (const EFI_COMPRESSION_SECTION2*)sectionHeader;
        headerSize = sizeof(EFI_COMPRESSION_SECTION2);
        compressionType = compressedSectionHeader2->CompressionType;
        uncompressedLength = compressedSectionHeader->UncompressedLength;
    }

    QByteArray header = section.left(headerSize);
    QByteArray body = section.mid(headerSize);

    // Get info
    QString name = sectionTypeToQString(sectionHeader->Type) + tr(" section");
    QString info = tr("Type: %1h\nFull size: %2h (%3)\nHeader size: %4h (%5)\nBody size: %6h (%7)\nCompression type: %8h\nDecompressed size: %9h (%10)")
        .hexarg2(sectionHeader->Type, 2)
        .hexarg(section.size()).arg(section.size())
        .hexarg(headerSize).arg(headerSize)
        .hexarg(body.size()).arg(body.size())
        .hexarg2(compressionType, 2)
        .hexarg(uncompressedLength).arg(uncompressedLength);

    // Construct parsing data
    pdata.offset += parentOffset;
    pdata.section.compressed.compressionType = compressionType;
    pdata.section.compressed.uncompressedSize = uncompressedLength;
    if (pdata.isOnFlash) info.prepend(tr("Offset: %1h\n").hexarg(pdata.offset));

    // Add tree item
    index = model->addItem(Types::Section, sectionHeader->Type, name, QString(), info, header, body, parsingDataToQByteArray(pdata), parent);

    return ERR_SUCCESS;
}

STATUS FfsParser::parseGuidedSectionHeader(const QByteArray & section, const UINT32 parentOffset, const QModelIndex & parent, QModelIndex & index)
{
    // Get data from parent's parsing data
    PARSING_DATA pdata = parsingDataFromQByteArray(parent);

    // Obtain header fields
    const EFI_COMMON_SECTION_HEADER* sectionHeader = (const EFI_COMMON_SECTION_HEADER*)(section.constData());
    const EFI_GUID_DEFINED_SECTION* guidDefinedSectionHeader = guidDefinedSectionHeader = (const EFI_GUID_DEFINED_SECTION*)sectionHeader;
    EFI_GUID guid = guidDefinedSectionHeader->SectionDefinitionGuid;
    UINT16 dataOffset = guidDefinedSectionHeader->DataOffset;
    UINT16 attributes = guidDefinedSectionHeader->Attributes;
    if (pdata.ffsVersion == 3 && uint24ToUint32(sectionHeader->Size) == EFI_SECTION2_IS_USED) {
        const EFI_GUID_DEFINED_SECTION2* guidDefinedSectionHeader2 = (const EFI_GUID_DEFINED_SECTION2*)sectionHeader;
        guid = guidDefinedSectionHeader2->SectionDefinitionGuid;
        dataOffset = guidDefinedSectionHeader2->DataOffset;
        attributes = guidDefinedSectionHeader2->Attributes;
    }
    
    QByteArray header = section.left(dataOffset);
    QByteArray body = section.mid(dataOffset);

    // Get info
    QString name = guidToQString(guid);
    QString info = tr("Section GUID: %1\nType: %2h\nFull size: %3h (%4)\nHeader size: %5h (%6)\nBody size: %7h (%8)\nData offset: %9h\nAttributes: %10h")
        .arg(name)
        .hexarg2(sectionHeader->Type, 2)
        .hexarg(section.size()).arg(section.size())
        .hexarg(header.size()).arg(header.size())
        .hexarg(body.size()).arg(body.size())
        .hexarg(dataOffset)
        .hexarg2(attributes, 4);

    // Construct parsing data
    pdata.offset += parentOffset;
    pdata.section.guidDefined.attributes = attributes;
    pdata.section.guidDefined.guid = guid;
    if (pdata.isOnFlash) info.prepend(tr("Offset: %1h\n").hexarg(pdata.offset));

    // Add tree item
    index = model->addItem(Types::Section, sectionHeader->Type, name, QString(), info, header, body, parsingDataToQByteArray(pdata), parent);

    return ERR_SUCCESS;
}

STATUS FfsParser::parseFreeformGuidedSectionHeader(const QByteArray & section, const UINT32 parentOffset, const QModelIndex & parent, QModelIndex & index)
{
    // Get data from parent's parsing data
    PARSING_DATA pdata = parsingDataFromQByteArray(parent);

    // Obtain header fields
    const EFI_COMMON_SECTION_HEADER* sectionHeader = (const EFI_COMMON_SECTION_HEADER*)(section.constData());
    const EFI_FREEFORM_SUBTYPE_GUID_SECTION* fsgHeader = (const EFI_FREEFORM_SUBTYPE_GUID_SECTION*)sectionHeader;
    UINT32 headerSize = sizeof(EFI_FREEFORM_SUBTYPE_GUID_SECTION);
    EFI_GUID guid = fsgHeader->SubTypeGuid;
    if (pdata.ffsVersion == 3 && uint24ToUint32(sectionHeader->Size) == EFI_SECTION2_IS_USED) {
        const EFI_FREEFORM_SUBTYPE_GUID_SECTION2* fsgHeader2 = (const EFI_FREEFORM_SUBTYPE_GUID_SECTION2*)sectionHeader;
        headerSize = sizeof(EFI_FREEFORM_SUBTYPE_GUID_SECTION2);
        guid = fsgHeader2->SubTypeGuid;
    }

    QByteArray header = section.left(headerSize);
    QByteArray body = section.mid(headerSize);

    // Check for signed section
    bool msgSigned = false;
    bool msgUnknownSignature = false;
    bool msgUnknownUefiGuidSignature = false;
    QString signInfo;
    if (QByteArray((const char*)&guid, sizeof(EFI_GUID)) == EFI_FIRMWARE_CONTENTS_SIGNED_GUID) {
        msgSigned = true;
        const WIN_CERTIFICATE* certificateHeader = (const WIN_CERTIFICATE*)model->body(index).constData();
        if (certificateHeader->CertificateType == WIN_CERT_TYPE_EFI_GUID) {
            signInfo += tr("\nSignature type: UEFI");
            const WIN_CERTIFICATE_UEFI_GUID* guidCertificateHeader = (const WIN_CERTIFICATE_UEFI_GUID*)certificateHeader;
            if (QByteArray((const char*)&guidCertificateHeader->CertType, sizeof(EFI_GUID)) == EFI_CERT_TYPE_RSA2048_SHA256_GUID) {
                signInfo += tr("\nSignature subtype: RSA2048/SHA256");
            }
            else if (QByteArray((const char*)&guidCertificateHeader->CertType, sizeof(EFI_GUID)) == EFI_CERT_TYPE_PKCS7_GUID) {
                signInfo += tr("\nSignature subtype: PCKS7");
            }
            else {
                signInfo += tr("\nSignature subtype: unknown");
                msgUnknownUefiGuidSignature = true;
            }
        }
        else if (certificateHeader->CertificateType == WIN_CERT_TYPE_PKCS_SIGNED_DATA) {
            signInfo += tr("\nSignature type: PCKS7");
        }
        else {
            signInfo += tr("\nSignature type: unknown");
            msgUnknownSignature = true;
        }

        // Add additional to the header
        header.append(body.left(certificateHeader->Length));
        // Get new body
        body = body.mid(certificateHeader->Length);
    }

    // Get info
    QString name = sectionTypeToQString(sectionHeader->Type) + tr(" section");
    QString info = tr("Type: %1h\nFull size: %2h (%3)\nHeader size: %4h (%5)\nBody size: %6h (%7)\nSubtype GUID: %8")
        .hexarg2(fsgHeader->Type, 2)
        .hexarg(section.size()).arg(section.size())
        .hexarg(header.size()).arg(header.size())
        .hexarg(body.size()).arg(body.size())
        .arg(guidToQString(guid));
    if (!signInfo.isEmpty()) info.append(signInfo);

    // Construct parsing data
    pdata.offset += parentOffset;
    pdata.section.freeformSubtypeGuid.guid = guid;
    if (pdata.isOnFlash) info.prepend(tr("Offset: %1h\n").hexarg(pdata.offset));

    // Add tree item
    index = model->addItem(Types::Section, sectionHeader->Type, name, QString(), info, header, body, parsingDataToQByteArray(pdata), parent);

    // Show messages
    if (msgSigned)
        msg(tr("parseSection: signature may become invalid after any modification"), index);
    if (msgUnknownUefiGuidSignature)
        msg(tr("parseSection: GUID defined section with unknown signature subtype"), index);
    if (msgUnknownSignature)
        msg(tr("parseSection: GUID defined section with unknown signature type"), index);

    // Rename section
    model->setName(index, guidToQString(guid));

    return ERR_SUCCESS;
}

STATUS FfsParser::parseVersionSectionHeader(const QByteArray & section, const UINT32 parentOffset, const QModelIndex & parent, QModelIndex & index)
{
    // Get data from parent's parsing data
    PARSING_DATA pdata = parsingDataFromQByteArray(parent);

    // Obtain header fields
    const EFI_COMMON_SECTION_HEADER* sectionHeader = (const EFI_COMMON_SECTION_HEADER*)(section.constData());
    const EFI_VERSION_SECTION* versionHeader = (const EFI_VERSION_SECTION*)sectionHeader;
    UINT32 headerSize = sizeof(EFI_VERSION_SECTION);
    UINT16 buildNumber = versionHeader->BuildNumber;
    if (pdata.ffsVersion == 3 && uint24ToUint32(sectionHeader->Size) == EFI_SECTION2_IS_USED) {
        const EFI_VERSION_SECTION2* versionHeader2 = (const EFI_VERSION_SECTION2*)sectionHeader;
        headerSize = sizeof(EFI_VERSION_SECTION2);
        buildNumber = versionHeader2->BuildNumber;
    }

    QByteArray header = section.left(headerSize);
    QByteArray body = section.mid(headerSize);
    
    // Get info
    QString name = sectionTypeToQString(sectionHeader->Type) + tr(" section");
    QString info = tr("Type: %1h\nFull size: %2h (%3)\nHeader size: %4h (%5)\nBody size: %6h (%7)\nBuild number: %8")
        .hexarg2(versionHeader->Type, 2)
        .hexarg(section.size()).arg(section.size())
        .hexarg(header.size()).arg(header.size())
        .hexarg(body.size()).arg(body.size())
        .arg(buildNumber);

    // Construct parsing data
    pdata.offset += parentOffset;
    if (pdata.isOnFlash) info.prepend(tr("Offset: %1h\n").hexarg(pdata.offset));

    // Add tree item
    index = model->addItem(Types::Section, sectionHeader->Type, name, QString(), info, header, body, parsingDataToQByteArray(pdata), parent);
    
    return ERR_SUCCESS;
}

STATUS FfsParser::parsePostcodeSectionHeader(const QByteArray & section, const UINT32 parentOffset, const QModelIndex & parent, QModelIndex & index)
{
    // Get data from parent's parsing data
    PARSING_DATA pdata = parsingDataFromQByteArray(parent);

    // Obtain header fields
    const EFI_COMMON_SECTION_HEADER* sectionHeader = (const EFI_COMMON_SECTION_HEADER*)(section.constData());
    const POSTCODE_SECTION* postcodeHeader = (const POSTCODE_SECTION*)sectionHeader;
    UINT32 headerSize = sizeof(POSTCODE_SECTION);
    UINT32 postCode = postcodeHeader->Postcode;
    if (pdata.ffsVersion == 3 && uint24ToUint32(sectionHeader->Size) == EFI_SECTION2_IS_USED) {
        const POSTCODE_SECTION2* postcodeHeader2 = (const POSTCODE_SECTION2*)sectionHeader;
        headerSize = sizeof(POSTCODE_SECTION2);
        postCode = postcodeHeader2->Postcode;
    }

    QByteArray header = section.left(headerSize);
    QByteArray body = section.mid(headerSize);

    // Get info
    QString name = sectionTypeToQString(sectionHeader->Type) + tr(" section");
    QString info = tr("Type: %1h\nFull size: %2h (%3)\nHeader size: %4h (%5)\nBody size: %6h (%7)\nPostcode: %8h\n")
        .hexarg2(postcodeHeader->Type, 2)
        .hexarg(section.size()).arg(section.size())
        .hexarg(header.size()).arg(header.size())
        .hexarg(body.size()).arg(body.size())
        .hexarg(postCode);

    // Construct parsing data
    pdata.offset += parentOffset;
    if (pdata.isOnFlash) info.prepend(tr("Offset: %1h\n").hexarg(pdata.offset));

    // Add tree item
    index = model->addItem(Types::Section, sectionHeader->Type, name, QString(), info, header, body, parsingDataToQByteArray(pdata), parent);

    return ERR_SUCCESS;
}


STATUS FfsParser::parseSectionBody(const QModelIndex & index)
{
    // Sanity check
    if (!index.isValid())
        return ERR_INVALID_PARAMETER;
    
    const EFI_COMMON_SECTION_HEADER* sectionHeader = (const EFI_COMMON_SECTION_HEADER*)(model->header(index).constData());

    switch (sectionHeader->Type) {
    // Encapsulation
    case EFI_SECTION_COMPRESSION:           return parseCompressedSectionBody(index);
    case EFI_SECTION_GUID_DEFINED:          return parseGuidedSectionBody(index);
    case EFI_SECTION_DISPOSABLE:            return parseSections(model->body(index), index);
    // Leaf
    case EFI_SECTION_FREEFORM_SUBTYPE_GUID: return parseRawArea(model->body(index), index);
    case EFI_SECTION_VERSION:               return parseVersionSectionBody(index);
    case EFI_SECTION_DXE_DEPEX:
    case EFI_SECTION_PEI_DEPEX:
    case EFI_SECTION_SMM_DEPEX:             return parseDepexSectionBody(index);
    case EFI_SECTION_TE:                    return parseTeImageSectionBody(index);
    case EFI_SECTION_PE32:
    case EFI_SECTION_PIC:                   return parsePeImageSectionBody(index);
    case EFI_SECTION_USER_INTERFACE:        return parseUiSectionBody(index);
    case EFI_SECTION_FIRMWARE_VOLUME_IMAGE: return parseRawArea(model->body(index), index);
    case EFI_SECTION_RAW:                   return parseRawSectionBody(index);
    // No parsing needed
    case EFI_SECTION_COMPATIBILITY16:
    case SCT_SECTION_POSTCODE:
    case INSYDE_SECTION_POSTCODE:
    default:
        return ERR_SUCCESS;
    }
}

STATUS FfsParser::parseCompressedSectionBody(const QModelIndex & index)
{
    // Sanity check
    if (!index.isValid())
        return ERR_INVALID_PARAMETER;

    // Get data from parsing data
    PARSING_DATA pdata = parsingDataFromQByteArray(index);
    UINT8 algorithm = pdata.section.compressed.compressionType;

    // Decompress section
    QByteArray decompressed;
    STATUS result = decompress(model->body(index), algorithm, decompressed);
    if (result) {
        msg(tr("parseCompressedSectionBody: decompression failed with error \"%1\"").arg(errorCodeToQString(result)), index);
        return ERR_SUCCESS;
    }
    
    // Check reported uncompressed size
    if (pdata.section.compressed.uncompressedSize != (UINT32)decompressed.size()) {
        msg(tr("parseCompressedSectionBody: decompressed size stored in header %1h (%2) differs from actual %3h (%4)")
            .hexarg(pdata.section.compressed.uncompressedSize)
            .arg(pdata.section.compressed.uncompressedSize)
            .hexarg(decompressed.size())
            .arg(decompressed.size()), index);
        model->addInfo(index, tr("\nActual decompressed size: %1h (%2)").hexarg(decompressed.size()).arg(decompressed.size()));
    }

    // Add info
    model->addInfo(index, tr("\nCompression algorithm: %1").arg(compressionTypeToQString(algorithm)));

    // Update parsing data
    pdata.isOnFlash = (algorithm == COMPRESSION_ALGORITHM_NONE); // Data is not on flash unless not compressed
    pdata.section.compressed.algorithm = algorithm;
    model->setParsingData(index, parsingDataToQByteArray(pdata));

    // Parse decompressed data
    return parseSections(decompressed, index);
}

STATUS FfsParser::parseGuidedSectionBody(const QModelIndex & index)
{
    // Sanity check
    if (!index.isValid())
        return ERR_INVALID_PARAMETER;

    // Get data from parsing data
    PARSING_DATA pdata = parsingDataFromQByteArray(index);
    UINT32 attributes = pdata.section.guidDefined.attributes;
    EFI_GUID guid = pdata.section.guidDefined.guid;

    // Check if section requires processing
    QByteArray processed = model->body(index);
    QString info;
    bool parseCurrentSection = true;
    UINT8 algorithm = COMPRESSION_ALGORITHM_NONE;
    if (attributes & EFI_GUIDED_SECTION_PROCESSING_REQUIRED) {
        // Tiano compressed section
        if (QByteArray((const char*)&guid, sizeof(EFI_GUID)) == EFI_GUIDED_SECTION_TIANO) {
            algorithm = EFI_STANDARD_COMPRESSION;
            STATUS result = decompress(model->body(index), algorithm, processed);
            if (result) {
                parseCurrentSection = false;
                msg(tr("parseGuidedSectionBody: decompression failed with error \"%1\"").arg(errorCodeToQString(result)), index);
                return ERR_SUCCESS;
            }

            if (algorithm == COMPRESSION_ALGORITHM_TIANO) {
                info += tr("\nCompression algorithm: Tiano");
                info += tr("\nDecompressed size: %1h (%2)").hexarg(processed.length()).arg(processed.length());
            }
            else if (algorithm == COMPRESSION_ALGORITHM_EFI11) {
                info += tr("\nCompression algorithm: EFI 1.1");
                info += tr("\nDecompressed size: %1h (%2)").hexarg(processed.length()).arg(processed.length());
            }
            else
                info += tr("\nCompression type: unknown");
        }
        // LZMA compressed section
        else if (QByteArray((const char*)&guid, sizeof(EFI_GUID)) == EFI_GUIDED_SECTION_LZMA) {
            algorithm = EFI_CUSTOMIZED_COMPRESSION;
            STATUS result = decompress(model->body(index), algorithm, processed);
            if (result) {
                parseCurrentSection = false;
                msg(tr("parseGuidedSectionBody: decompression failed with error \"%1\"").arg(errorCodeToQString(result)), index);
                return ERR_SUCCESS;
            }

            if (algorithm == COMPRESSION_ALGORITHM_LZMA) {
                info += tr("\nCompression algorithm: LZMA");
                info += tr("\nDecompressed size: %1h (%2)").hexarg(processed.length()).arg(processed.length());
            }
            else
                info += tr("\nCompression algorithm: unknown");
        }
        // Unknown GUIDed section
        else {
            parseCurrentSection = false;
            msg(tr("parseGuidedSectionBody: GUID defined section with unknown processing method"), index);
        }
    }

    // Check if section requires checksum calculation
    if (attributes & EFI_GUIDED_SECTION_AUTH_STATUS_VALID)
    {
        // CRC32 section
        if (QByteArray((const char*)&guid, sizeof(EFI_GUID)) == EFI_GUIDED_SECTION_CRC32) {
            info += tr("\nChecksum type: CRC32");
            // Calculate CRC32 of section data
            QByteArray body = model->body(index);
            UINT32 crc = crc32(0, (const UINT8*)body.constData(), body.size());
            // Check stored CRC32
            if (crc == *(const UINT32*)(model->header(index).constData() + sizeof(EFI_GUID_DEFINED_SECTION))) {
                info += tr("\nChecksum: valid");
            }
            else {
                info += tr("\nChecksum: invalid");
                msg(tr("parseGuidedSectionBody: GUID defined section with invalid CRC32"), index);
            }
        }
        else
            msg(tr("parseGuidedSectionBody: GUID defined section with unknown authentication method"), index);
    }

    // Add info
    model->addInfo(index, info);

    // Update parsing data
    pdata.isOnFlash = (algorithm == COMPRESSION_ALGORITHM_NONE); // Data is not on flash unless not compressed
    model->setParsingData(index, parsingDataToQByteArray(pdata));

    if (!parseCurrentSection) {
        msg(tr("parseGuidedSectionBody: GUID defined section can not be processed"), index);
        return ERR_SUCCESS;
    }

    return parseSections(processed, index);
}

STATUS FfsParser::parseVersionSectionBody(const QModelIndex & index)
{
    // Sanity check
    if (!index.isValid())
        return ERR_INVALID_PARAMETER;

    // Add info
    model->addInfo(index, tr("\nVersion string: %1").arg(QString::fromUtf16((const ushort*)model->body(index).constData())));

    return ERR_SUCCESS;
}

STATUS FfsParser::parseDepexSectionBody(const QModelIndex & index)
{
    // Sanity check
    if (!index.isValid())
        return ERR_INVALID_PARAMETER;

    QByteArray body = model->body(index);
    QString parsed;

    // Check data to be present
    if (!body.size())
        return ERR_INVALID_PARAMETER;

    const EFI_GUID * guid;
    const UINT8* current = (const UINT8*)body.constData();

    // Special cases of first opcode
    switch (*current) {
    case EFI_DEP_BEFORE:
        if (body.size() != 2 * EFI_DEP_OPCODE_SIZE + sizeof(EFI_GUID)) {
            msg(tr("parseDepexSectionBody: DEPEX section too long for a section starting with BEFORE opcode"), index);
            return ERR_SUCCESS;
        }
        guid = (const EFI_GUID*)(current + EFI_DEP_OPCODE_SIZE);
        parsed += tr("\nBEFORE %1").arg(guidToQString(*guid));
        current += EFI_DEP_OPCODE_SIZE + sizeof(EFI_GUID);
        if (*current != EFI_DEP_END){
            msg(tr("parseDepexSectionBody: DEPEX section ends with non-END opcode"), index);
            return ERR_SUCCESS;
        }
        return ERR_SUCCESS;
    case EFI_DEP_AFTER:
        if (body.size() != 2 * EFI_DEP_OPCODE_SIZE + sizeof(EFI_GUID)){
            msg(tr("parseDepexSectionBody: DEPEX section too long for a section starting with AFTER opcode"), index);
            return ERR_SUCCESS;
        }
        guid = (const EFI_GUID*)(current + EFI_DEP_OPCODE_SIZE);
        parsed += tr("\nAFTER %1").arg(guidToQString(*guid));
        current += EFI_DEP_OPCODE_SIZE + sizeof(EFI_GUID);
        if (*current != EFI_DEP_END) {
            msg(tr("parseDepexSectionBody: DEPEX section ends with non-END opcode"), index);
            return ERR_SUCCESS;
        }
        return ERR_SUCCESS;
    case EFI_DEP_SOR:
        if (body.size() <= 2 * EFI_DEP_OPCODE_SIZE) {
            msg(tr("parseDepexSectionBody: DEPEX section too short for a section starting with SOR opcode"), index);
            return ERR_SUCCESS;
        }
        parsed += tr("\nSOR");
        current += EFI_DEP_OPCODE_SIZE;
        break;
    }

    // Parse the rest of depex 
    while (current - (const UINT8*)body.constData() < body.size()) {
        switch (*current) {
        case EFI_DEP_BEFORE: {
            msg(tr("parseDepexSectionBody: misplaced BEFORE opcode"), index);
            return ERR_SUCCESS;
        }
        case EFI_DEP_AFTER: {
            msg(tr("parseDepexSectionBody: misplaced AFTER opcode"), index);
            return ERR_SUCCESS;
        }
        case EFI_DEP_SOR: {
            msg(tr("parseDepexSectionBody: misplaced SOR opcode"), index);
            return ERR_SUCCESS;
        }
        case EFI_DEP_PUSH:
            // Check that the rest of depex has correct size
            if ((UINT32)body.size() - (UINT32)(current - (const UINT8*)body.constData()) <= EFI_DEP_OPCODE_SIZE + sizeof(EFI_GUID)) {
                parsed.clear();
                msg(tr("parseDepexSectionBody: remains of DEPEX section too short for PUSH opcode"), index);
                return ERR_SUCCESS;
            }
            guid = (const EFI_GUID*)(current + EFI_DEP_OPCODE_SIZE);
            parsed += tr("\nPUSH %1").arg(guidToQString(*guid));
            current += EFI_DEP_OPCODE_SIZE + sizeof(EFI_GUID);
            break;
        case EFI_DEP_AND:
            parsed += tr("\nAND");
            current += EFI_DEP_OPCODE_SIZE;
            break;
        case EFI_DEP_OR:
            parsed += tr("\nOR");
            current += EFI_DEP_OPCODE_SIZE;
            break;
        case EFI_DEP_NOT:
            parsed += tr("\nNOT");
            current += EFI_DEP_OPCODE_SIZE;
            break;
        case EFI_DEP_TRUE:
            parsed += tr("\nTRUE");
            current += EFI_DEP_OPCODE_SIZE;
            break;
        case EFI_DEP_FALSE:
            parsed += tr("\nFALSE");
            current += EFI_DEP_OPCODE_SIZE;
            break;
        case EFI_DEP_END:
            parsed += tr("\nEND");
            current += EFI_DEP_OPCODE_SIZE;
            // Check that END is the last opcode
            if (current - (const UINT8*)body.constData() < body.size()) {
                parsed.clear();
                msg(tr("parseDepexSectionBody: DEPEX section ends with non-END opcode"), index);
            }
            break;
        default:
            msg(tr("parseDepexSectionBody: unknown opcode"), index);
            return ERR_SUCCESS;
            break;
        }
    }
    
    // Add info
    model->addInfo(index, tr("\nParsed expression:%1").arg(parsed));

    return ERR_SUCCESS;
}

STATUS FfsParser::parseUiSectionBody(const QModelIndex & index)
{
    // Sanity check
    if (!index.isValid())
        return ERR_INVALID_PARAMETER;

    QString text = QString::fromUtf16((const ushort*)model->body(index).constData());

    // Add info
    model->addInfo(index, tr("\nText: %1").arg(text));

    // Rename parent file
    model->setText(model->findParentOfType(index, Types::File), text);

    return ERR_SUCCESS;
}

STATUS FfsParser::parseAprioriRawSection(const QByteArray & body, QString & parsed)
{
    parsed.clear();

    UINT32 count = body.size() / sizeof(EFI_GUID);
    if (count > 0) {
        for (UINT32 i = 0; i < count; i++) {
            const EFI_GUID* guid = (const EFI_GUID*)body.constData() + i;
            parsed += tr("\n%1").arg(guidToQString(*guid));
        }
    }

    return ERR_SUCCESS;
}

STATUS FfsParser::parseRawSectionBody(const QModelIndex & index)
{
    // Sanity check
    if (!index.isValid())
        return ERR_INVALID_PARAMETER;

    // Check for apriori file
    QModelIndex parentFile = model->findParentOfType(index, Types::File);
    QByteArray parentFileGuid = model->header(parentFile).left(sizeof(EFI_GUID));
    if (parentFileGuid == EFI_PEI_APRIORI_FILE_GUID) { // PEI apriori file
        // Parse apriori file list
        QString str;
        STATUS result = parseAprioriRawSection(model->body(index), str);
        if (!result && !str.isEmpty())
            model->addInfo(index, tr("\nFile list:%1").arg(str));

        // Set parent file text
        model->setText(parentFile, tr("PEI apriori file"));

        return ERR_SUCCESS;
    }
    else if (parentFileGuid == EFI_DXE_APRIORI_FILE_GUID) { // DXE apriori file
        // Parse apriori file list
        QString str;
        STATUS result = parseAprioriRawSection(model->body(index), str);
        if (!result && !str.isEmpty())
            model->addInfo(index, tr("\nFile list:%1").arg(str));

        // Set parent file text
        model->setText(parentFile, tr("DXE apriori file"));

        return ERR_SUCCESS;
    }

    // Parse as raw area
    return parseRawArea(model->body(index), index);
}


STATUS FfsParser::parsePeImageSectionBody(const QModelIndex & index)
{
    // Sanity check
    if (!index.isValid())
        return ERR_INVALID_PARAMETER;

    // Get section body
    QByteArray body = model->body(index);

    // Get PE info
    QByteArray info;

    const EFI_IMAGE_DOS_HEADER* dosHeader = (const EFI_IMAGE_DOS_HEADER*)body.constData();
    if (dosHeader->e_magic != EFI_IMAGE_DOS_SIGNATURE) {
        info += tr("\nDOS signature: %1h, invalid").hexarg2(dosHeader->e_magic, 4);
        msg(tr("parsePeImageSectionBody: PE32 image with invalid DOS signature"), index);
    }
    else {
        const EFI_IMAGE_PE_HEADER* peHeader = (EFI_IMAGE_PE_HEADER*)(body.constData() + dosHeader->e_lfanew);
        if (peHeader->Signature != EFI_IMAGE_PE_SIGNATURE) {
            info += tr("\nPE signature: %1h, invalid").hexarg2(peHeader->Signature, 8);
            msg(tr("parsePeImageSectionBody: PE32 image with invalid PE signature"), index);
        }
        else {
            const EFI_IMAGE_FILE_HEADER* imageFileHeader = (const EFI_IMAGE_FILE_HEADER*)(peHeader + 1);
            info += tr("\nDOS signature: %1h\nPE signature: %2h\nMachine type: %3\nNumber of sections: %4\nCharacteristics: %5h")
                .hexarg2(dosHeader->e_magic, 4)
                .hexarg2(peHeader->Signature, 8)
                .arg(machineTypeToQString(imageFileHeader->Machine))
                .arg(imageFileHeader->NumberOfSections)
                .hexarg2(imageFileHeader->Characteristics, 4);

            EFI_IMAGE_OPTIONAL_HEADER_POINTERS_UNION optionalHeader;
            optionalHeader.H32 = (const EFI_IMAGE_OPTIONAL_HEADER32*)(imageFileHeader + 1);
            if (optionalHeader.H32->Magic == EFI_IMAGE_PE_OPTIONAL_HDR32_MAGIC) {
                info += tr("\nOptional header signature: %1h\nSubsystem: %2h\nAddress of entryPoint: %3h\nBase of code: %4h\nImage base: %5h")
                    .hexarg2(optionalHeader.H32->Magic, 4)
                    .hexarg2(optionalHeader.H32->Subsystem, 4)
                    .hexarg(optionalHeader.H32->AddressOfEntryPoint)
                    .hexarg(optionalHeader.H32->BaseOfCode)
                    .hexarg(optionalHeader.H32->ImageBase);
            }
            else if (optionalHeader.H32->Magic == EFI_IMAGE_PE_OPTIONAL_HDR64_MAGIC) {
                info += tr("\nOptional header signature: %1h\nSubsystem: %2h\nAddress of entryPoint: %3h\nBase of code: %4h\nImage base: %5h")
                    .hexarg2(optionalHeader.H64->Magic, 4)
                    .hexarg2(optionalHeader.H64->Subsystem, 4)
                    .hexarg(optionalHeader.H64->AddressOfEntryPoint)
                    .hexarg(optionalHeader.H64->BaseOfCode)
                    .hexarg(optionalHeader.H64->ImageBase);
            }
            else {
                info += tr("\nOptional header signature: %1h, unknown").hexarg2(optionalHeader.H64->Magic, 4);
                msg(tr("parsePeImageSectionBody: PE32 image with invalid optional PE header signature"), index);
            }
        }
    }

    // Add PE info
    model->addInfo(index, info);

    return ERR_SUCCESS;
}


STATUS FfsParser::parseTeImageSectionBody(const QModelIndex & index)
{
    // Sanity check
    if (!index.isValid())
        return ERR_INVALID_PARAMETER;

    // Get section body
    QByteArray body = model->body(index);

    // Get TE info
    QByteArray info;
    const EFI_IMAGE_TE_HEADER* teHeader = (const EFI_IMAGE_TE_HEADER*)body.constData();
    if (teHeader->Signature != EFI_IMAGE_TE_SIGNATURE) {
        info += tr("\nSignature: %1h, invalid").hexarg2(teHeader->Signature, 4);
        msg(tr("parseTeImageSectionBody: TE image with invalid TE signature"), index);
    }
    else {
        info += tr("\nSignature: %1h\nMachine type: %2\nNumber of sections: %3\nSubsystem: %4h\nStripped size: %5h (%6)\nBase of code: %7h\nAddress of entry point: %8h\nImage base: %9h\nAdjusted image base: %10h")
            .hexarg2(teHeader->Signature, 4)
            .arg(machineTypeToQString(teHeader->Machine))
            .arg(teHeader->NumberOfSections)
            .hexarg2(teHeader->Subsystem, 2)
            .hexarg(teHeader->StrippedSize).arg(teHeader->StrippedSize)
            .hexarg(teHeader->BaseOfCode)
            .hexarg(teHeader->AddressOfEntryPoint)
            .hexarg(teHeader->ImageBase)
            .hexarg(teHeader->ImageBase + teHeader->StrippedSize - sizeof(EFI_IMAGE_TE_HEADER));
    }

    // Get data from parsing data
    PARSING_DATA pdata = parsingDataFromQByteArray(index);
    pdata.section.teImage.imageBase = teHeader->ImageBase;
    pdata.section.teImage.adjustedImageBase = teHeader->ImageBase + teHeader->StrippedSize - sizeof(EFI_IMAGE_TE_HEADER);
    
    // Update parsing data
    model->setParsingData(index, parsingDataToQByteArray(pdata));

    // Add TE info
    model->addInfo(index, info);

    return ERR_SUCCESS;
}


STATUS FfsParser::addMemoryAddressesInfo(const QModelIndex & index)
{
    // Sanity check
    if (!index.isValid() || !lastVtf.isValid())
        return ERR_INVALID_PARAMETER;

    // Get parsing data for the last VTF
    PARSING_DATA pdata = parsingDataFromQByteArray(lastVtf);
    if (!pdata.isOnFlash) {
        msg(tr("addPhysicalAddressInfo: the last VTF appears inside compressed item, the image may be damaged"), lastVtf);
        return ERR_SUCCESS;
    }

    // Calculate address difference
    const UINT32 vtfSize = model->header(lastVtf).size() + model->body(lastVtf).size() + (pdata.file.hasTail ? sizeof(UINT16) : 0);
    const UINT32 diff = 0xFFFFFFFF - pdata.offset - vtfSize + 1;

    // Apply address information to index and all it's child items
    return addMemoryAddressesRecursive(index, diff);
}

STATUS FfsParser::addMemoryAddressesRecursive(const QModelIndex & index, const UINT32 diff)
{
    // Sanity check
    if (!index.isValid())
        return ERR_SUCCESS;

    // Get parsing data for the current item
    PARSING_DATA pdata = parsingDataFromQByteArray(index);

    // Set address value for non-compressed data
    if (pdata.isOnFlash) {
        // Check address sanity
        if ((const UINT64)diff + pdata.offset <= 0xFFFFFFFF)  {
            // Update info
            pdata.address = diff + pdata.offset;
            UINT32 headerSize = model->header(index).size();
            if (headerSize) {
                model->addInfo(index, tr("\nHeader memory address: %1h").hexarg2(pdata.address, 8));
                model->addInfo(index, tr("\nData memory address: %1h").hexarg2(pdata.address + headerSize, 8));
            }
            else {
                model->addInfo(index, tr("\nMemory address: %1h").hexarg2(pdata.address, 8));
            }

            // Special case of uncompressed TE image sections
            if (model->type(index) == Types::Section && model->subtype(index) == EFI_SECTION_TE && pdata.isOnFlash) {
                // Check data memory address to be equal to either ImageBase or AdjustedImageBase
                if (pdata.section.teImage.imageBase == pdata.address + headerSize) {
                    pdata.section.teImage.revision = 1;
                    model->addInfo(index, tr("\nTE image format revision: %1").arg(pdata.section.teImage.revision));
                }
                else if (pdata.section.teImage.adjustedImageBase == pdata.address + headerSize) {
                    pdata.section.teImage.revision = 2;
                    model->addInfo(index, tr("\nTE image format revision: %1").arg(pdata.section.teImage.revision));
                }
                else {
                    msg(tr("addMemoryAddressesRecursive: image base is nether original nor adjusted, the image is either damaged or a part of backup PEI volume"), index);
                    pdata.section.teImage.revision = 0;
                }
            }

            // Set modified parsing data
            model->setParsingData(index, parsingDataToQByteArray(pdata));
        }
    }

    // Process child items
    for (int i = 0; i < model->rowCount(index); i++) {
        addMemoryAddressesRecursive(index.child(i, 0), diff);
    }

    return ERR_SUCCESS;
}