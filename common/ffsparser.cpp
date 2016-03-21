/* ffsparser.cpp

Copyright (c) 2016, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHWARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*/

#include "ffsparser.h"

#include <cmath>
#include <algorithm>

// Region info structure definition
struct REGION_INFO {
    UINT32 offset;
    UINT32 length;
    UINT8  type;
    QByteArray data;
    friend bool operator< (const REGION_INFO & lhs, const REGION_INFO & rhs){ return lhs.offset < rhs.offset; }
};

FfsParser::FfsParser(TreeModel* treeModel)
    : model(treeModel), capsuleOffsetFixup(0)
{
}

FfsParser::~FfsParser()
{
}

void FfsParser::msg(const QString & message, const QModelIndex & index)
{
    messagesVector.push_back(std::pair<QString, QModelIndex>(message, index));
}

std::vector<std::pair<QString, QModelIndex> > FfsParser::getMessages() const
{
    return messagesVector;
}

void FfsParser::clearMessages()
{
    messagesVector.clear();
}

// Firmware image parsing functions
STATUS FfsParser::parse(const QByteArray & buffer) 
{
    QModelIndex root;
    STATUS result = performFirstPass(buffer, root);
    addOffsetsRecursive(root);
    if (result)
        return result;

    if (lastVtf.isValid()) {
        result = performSecondPass(root);
    }
    else {
        msg(QObject::tr("parse: not a single Volume Top File is found, the image may be corrupted"));
    }

    return result;
}

STATUS FfsParser::performFirstPass(const QByteArray & buffer, QModelIndex & index)
{
    // Reset capsule offset fixup value
    capsuleOffsetFixup = 0;

    // Check buffer size to be more than or equal to size of EFI_CAPSULE_HEADER
    if ((UINT32)buffer.size() <= sizeof(EFI_CAPSULE_HEADER)) {
        msg(QObject::tr("performFirstPass: image file is smaller than minimum size of %1h (%2) bytes").hexarg(sizeof(EFI_CAPSULE_HEADER)).arg(sizeof(EFI_CAPSULE_HEADER)));
        return ERR_INVALID_PARAMETER;
    }

    UINT32 capsuleHeaderSize = 0;
    // Check buffer for being normal EFI capsule header
    if (buffer.startsWith(EFI_CAPSULE_GUID)
        || buffer.startsWith(INTEL_CAPSULE_GUID)
        || buffer.startsWith(LENOVO_CAPSULE_GUID)
        || buffer.startsWith(LENOVO2_CAPSULE_GUID)) {
        // Get info
        const EFI_CAPSULE_HEADER* capsuleHeader = (const EFI_CAPSULE_HEADER*)buffer.constData();

        // Check sanity of HeaderSize and CapsuleImageSize values
        if (capsuleHeader->HeaderSize == 0 || capsuleHeader->HeaderSize > (UINT32)buffer.size() || capsuleHeader->HeaderSize > capsuleHeader->CapsuleImageSize) {
            msg(QObject::tr("performFirstPass: UEFI capsule header size of %1h (%2) bytes is invalid")
                .hexarg(capsuleHeader->HeaderSize).arg(capsuleHeader->HeaderSize));
            return ERR_INVALID_CAPSULE;
        }
        if (capsuleHeader->CapsuleImageSize == 0 || capsuleHeader->CapsuleImageSize > (UINT32)buffer.size()) {
            msg(QObject::tr("performFirstPass: UEFI capsule image size of %1h (%2) bytes is invalid")
                .hexarg(capsuleHeader->CapsuleImageSize).arg(capsuleHeader->CapsuleImageSize));
            return ERR_INVALID_CAPSULE;
        }

        capsuleHeaderSize = capsuleHeader->HeaderSize;
        QByteArray header = buffer.left(capsuleHeaderSize);
        QByteArray body = buffer.mid(capsuleHeaderSize);
        QString name = QObject::tr("UEFI capsule");
        QString info = QObject::tr("Capsule GUID: %1\nFull size: %2h (%3)\nHeader size: %4h (%5)\nImage size: %6h (%7)\nFlags: %8h")
            .arg(guidToQString(capsuleHeader->CapsuleGuid))
            .hexarg(buffer.size()).arg(buffer.size())
            .hexarg(capsuleHeaderSize).arg(capsuleHeaderSize)
            .hexarg(capsuleHeader->CapsuleImageSize - capsuleHeaderSize).arg(capsuleHeader->CapsuleImageSize - capsuleHeaderSize)
            .hexarg2(capsuleHeader->Flags, 8);

        // Set capsule offset fixup for correct volume allignment warnings
        capsuleOffsetFixup = capsuleHeaderSize;

        // Add tree item
        index = model->addItem(Types::Capsule, Subtypes::UefiCapsule, name, QString(), info, header, body, true);
    }
    // Check buffer for being Toshiba capsule header
    else if (buffer.startsWith(TOSHIBA_CAPSULE_GUID)) {
        // Get info
        const TOSHIBA_CAPSULE_HEADER* capsuleHeader = (const TOSHIBA_CAPSULE_HEADER*)buffer.constData();

        // Check sanity of HeaderSize and FullSize values
        if (capsuleHeader->HeaderSize == 0 || capsuleHeader->HeaderSize > (UINT32)buffer.size() || capsuleHeader->HeaderSize > capsuleHeader->FullSize) {
            msg(QObject::tr("performFirstPass: Toshiba capsule header size of %1h (%2) bytes is invalid")
                .hexarg(capsuleHeader->HeaderSize).arg(capsuleHeader->HeaderSize));
            return ERR_INVALID_CAPSULE;
        }
        if (capsuleHeader->FullSize == 0 || capsuleHeader->FullSize > (UINT32)buffer.size()) {
            msg(QObject::tr("performFirstPass: Toshiba capsule full size of %1h (%2) bytes is invalid")
                .hexarg(capsuleHeader->FullSize).arg(capsuleHeader->FullSize));
            return ERR_INVALID_CAPSULE;
        }

        capsuleHeaderSize = capsuleHeader->HeaderSize;
        QByteArray header = buffer.left(capsuleHeaderSize);
        QByteArray body = buffer.right(buffer.size() - capsuleHeaderSize);
        QString name = QObject::tr("Toshiba capsule");
        QString info = QObject::tr("Capsule GUID: %1\nFull size: %2h (%3)\nHeader size: %4h (%5)\nImage size: %6h (%7)\nFlags: %8h")
            .arg(guidToQString(capsuleHeader->CapsuleGuid))
            .hexarg(buffer.size()).arg(buffer.size())
            .hexarg(capsuleHeaderSize).arg(capsuleHeaderSize)
            .hexarg(capsuleHeader->FullSize - capsuleHeaderSize).arg(capsuleHeader->FullSize - capsuleHeaderSize)
            .hexarg2(capsuleHeader->Flags, 8);

        // Set capsule offset fixup for correct volume allignment warnings
        capsuleOffsetFixup = capsuleHeaderSize;

        // Add tree item
        index = model->addItem(Types::Capsule, Subtypes::ToshibaCapsule, name, QString(), info, header, body, true);
    }
    // Check buffer for being extended Aptio capsule header
    else if (buffer.startsWith(APTIO_SIGNED_CAPSULE_GUID) || buffer.startsWith(APTIO_UNSIGNED_CAPSULE_GUID)) {
        bool signedCapsule = buffer.startsWith(APTIO_SIGNED_CAPSULE_GUID);

        if ((UINT32)buffer.size() <= sizeof(APTIO_CAPSULE_HEADER)) {
            msg(QObject::tr("performFirstPass: AMI capsule image file is smaller than minimum size of %1h (%2) bytes").hexarg(sizeof(APTIO_CAPSULE_HEADER)).arg(sizeof(APTIO_CAPSULE_HEADER)));
            return ERR_INVALID_PARAMETER;
        }

        // Get info
        const APTIO_CAPSULE_HEADER* capsuleHeader = (const APTIO_CAPSULE_HEADER*)buffer.constData();

        // Check sanity of RomImageOffset and CapsuleImageSize values
        if (capsuleHeader->RomImageOffset == 0 || capsuleHeader->RomImageOffset > (UINT32)buffer.size() || capsuleHeader->RomImageOffset > capsuleHeader->CapsuleHeader.CapsuleImageSize) {
            msg(QObject::tr("performFirstPass: AMI capsule image offset of %1h (%2) bytes is invalid").hexarg(capsuleHeader->RomImageOffset).arg(capsuleHeader->RomImageOffset));
            return ERR_INVALID_CAPSULE;
        }
        if (capsuleHeader->CapsuleHeader.CapsuleImageSize == 0 || capsuleHeader->CapsuleHeader.CapsuleImageSize > (UINT32)buffer.size()) {
            msg(QObject::tr("performFirstPass: AMI capsule image size of %1h (%2) bytes is invalid").hexarg(capsuleHeader->CapsuleHeader.CapsuleImageSize).arg(capsuleHeader->CapsuleHeader.CapsuleImageSize));
            return ERR_INVALID_CAPSULE;
        }

        capsuleHeaderSize = capsuleHeader->RomImageOffset;
        QByteArray header = buffer.left(capsuleHeaderSize);
        QByteArray body = buffer.mid(capsuleHeaderSize);
        QString name = QObject::tr("AMI Aptio capsule");
        QString info = QObject::tr("Capsule GUID: %1\nFull size: %2h (%3)\nHeader size: %4h (%5)\nImage size: %6h (%7)\nFlags: %8h")
            .arg(guidToQString(capsuleHeader->CapsuleHeader.CapsuleGuid))
            .hexarg(buffer.size()).arg(buffer.size())
            .hexarg(capsuleHeaderSize).arg(capsuleHeaderSize)
            .hexarg(capsuleHeader->CapsuleHeader.CapsuleImageSize - capsuleHeaderSize).arg(capsuleHeader->CapsuleHeader.CapsuleImageSize - capsuleHeaderSize)
            .hexarg2(capsuleHeader->CapsuleHeader.Flags, 8);

        // Set capsule offset fixup for correct volume allignment warnings
        capsuleOffsetFixup = capsuleHeaderSize;

        // Add tree item
        index = model->addItem(Types::Capsule, signedCapsule ? Subtypes::AptioSignedCapsule : Subtypes::AptioUnsignedCapsule, name, QString(), info, header, body, true);

        // Show message about possible Aptio signature break
        if (signedCapsule) {
            msg(QObject::tr("performFirstPass: Aptio capsule signature may become invalid after image modifications"), index);
        }
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
        result = parseIntelImage(flashImage, capsuleHeaderSize, index, imageIndex);
        if (result != ERR_INVALID_FLASH_DESCRIPTOR) {
            if (!index.isValid())
                index = imageIndex;
            return result;
        }
    }

    // Get info
    QString name = QObject::tr("UEFI image");
    QString info = QObject::tr("Full size: %1h (%2)").hexarg(flashImage.size()).arg(flashImage.size());

    // Construct parsing data
    PARSING_DATA pdata = parsingDataFromQModelIndex(index);
    pdata.offset = capsuleHeaderSize;

    // Add tree item
    QModelIndex biosIndex = model->addItem(Types::Image, Subtypes::UefiImage, name, QString(), info, QByteArray(), flashImage, TRUE, parsingDataToQByteArray(pdata), index);

    // Parse the image
    result = parseRawArea(flashImage, biosIndex);
    if (!index.isValid())
        index = biosIndex;
    return result;
}

STATUS FfsParser::parseIntelImage(const QByteArray & intelImage, const UINT32 parentOffset, const QModelIndex & parent, QModelIndex & index)
{
    // Sanity check
    if (intelImage.isEmpty())
        return EFI_INVALID_PARAMETER;

    // Get parent's parsing data
    PARSING_DATA pdata = parsingDataFromQModelIndex(parent);

    // Store the beginning of descriptor as descriptor base address
    const UINT8* descriptor = (const UINT8*)intelImage.constData();

    // Check for buffer size to be greater or equal to descriptor region size
    if (intelImage.size() < FLASH_DESCRIPTOR_SIZE) {
        msg(QObject::tr("parseIntelImage: input file is smaller than minimum descriptor size of %1h (%2) bytes").hexarg(FLASH_DESCRIPTOR_SIZE).arg(FLASH_DESCRIPTOR_SIZE));
        return ERR_INVALID_FLASH_DESCRIPTOR;
    }

    // Parse descriptor map
    const FLASH_DESCRIPTOR_MAP* descriptorMap = (const FLASH_DESCRIPTOR_MAP*)(descriptor + sizeof(FLASH_DESCRIPTOR_HEADER));
    const FLASH_DESCRIPTOR_UPPER_MAP* upperMap = (const FLASH_DESCRIPTOR_UPPER_MAP*)(descriptor + FLASH_DESCRIPTOR_UPPER_MAP_BASE);

    // Check sanity of base values
    if (descriptorMap->MasterBase > FLASH_DESCRIPTOR_MAX_BASE
        || descriptorMap->MasterBase == descriptorMap->RegionBase
        || descriptorMap->MasterBase == descriptorMap->ComponentBase) {
        msg(QObject::tr("parseIntelImage: invalid descriptor master base %1h").hexarg2(descriptorMap->MasterBase, 2));
        return ERR_INVALID_FLASH_DESCRIPTOR;
    }
    if (descriptorMap->RegionBase > FLASH_DESCRIPTOR_MAX_BASE
        || descriptorMap->RegionBase == descriptorMap->ComponentBase) {
        msg(QObject::tr("parseIntelImage: invalid descriptor region base %1h").hexarg2(descriptorMap->RegionBase, 2));
        return ERR_INVALID_FLASH_DESCRIPTOR;
    }
    if (descriptorMap->ComponentBase > FLASH_DESCRIPTOR_MAX_BASE) {
        msg(QObject::tr("parseIntelImage: invalid descriptor component base %1h").hexarg2(descriptorMap->ComponentBase, 2));
        return ERR_INVALID_FLASH_DESCRIPTOR;
    }

    const FLASH_DESCRIPTOR_REGION_SECTION* regionSection = (const FLASH_DESCRIPTOR_REGION_SECTION*)calculateAddress8(descriptor, descriptorMap->RegionBase);
    const FLASH_DESCRIPTOR_COMPONENT_SECTION* componentSection = (const FLASH_DESCRIPTOR_COMPONENT_SECTION*)calculateAddress8(descriptor, descriptorMap->ComponentBase);

    // Check descriptor version by getting hardcoded value of FlashParameters.ReadClockFrequency
    UINT8 descriptorVersion = 0;
    if (componentSection->FlashParameters.ReadClockFrequency == FLASH_FREQUENCY_20MHZ)      // Old descriptor
        descriptorVersion = 1;
    else if (componentSection->FlashParameters.ReadClockFrequency == FLASH_FREQUENCY_17MHZ) // Skylake+ descriptor
        descriptorVersion = 2;
    else {
        msg(QObject::tr("parseIntelImage: unknown descriptor version with ReadClockFrequency %1h").hexarg(componentSection->FlashParameters.ReadClockFrequency));
        return ERR_INVALID_FLASH_DESCRIPTOR;
    }

    // Regions
    std::vector<REGION_INFO> regions;

    // ME region
    REGION_INFO me;
    me.type = Subtypes::MeRegion;
    me.offset = 0;
    me.length = 0;
    if (regionSection->MeLimit) {
        me.offset = calculateRegionOffset(regionSection->MeBase);
        me.length = calculateRegionSize(regionSection->MeBase, regionSection->MeLimit);
        me.data = intelImage.mid(me.offset, me.length);
        regions.push_back(me);
    }

    // BIOS region
    REGION_INFO bios;
    bios.type = Subtypes::BiosRegion;
    bios.offset = 0;
    bios.length = 0;
    if (regionSection->BiosLimit) {
        bios.offset = calculateRegionOffset(regionSection->BiosBase);
        bios.length = calculateRegionSize(regionSection->BiosBase, regionSection->BiosLimit);

        // Check for Gigabyte specific descriptor map
        if (bios.length == (UINT32)intelImage.size()) {
            if (!me.offset) {
                msg(QObject::tr("parseIntelImage: can't determine BIOS region start from Gigabyte-specific descriptor"));
                return ERR_INVALID_FLASH_DESCRIPTOR;
            }
            // Use ME region end as BIOS region offset
            bios.offset = me.offset + me.length;
            bios.length = (UINT32)intelImage.size() - bios.offset;
            bios.data = intelImage.mid(bios.offset, bios.length);
        }
        // Normal descriptor map
        else {
            bios.data = intelImage.mid(bios.offset, bios.length);
        }

        regions.push_back(bios);
    }
    else {
        msg(QObject::tr("parseIntelImage: descriptor parsing failed, BIOS region not found in descriptor"));
        return ERR_INVALID_FLASH_DESCRIPTOR;
    }

    // GbE region
    REGION_INFO gbe;
    gbe.type = Subtypes::GbeRegion;
    gbe.offset = 0;
    gbe.length = 0;
    if (regionSection->GbeLimit) {
        gbe.offset = calculateRegionOffset(regionSection->GbeBase);
        gbe.length = calculateRegionSize(regionSection->GbeBase, regionSection->GbeLimit);
        gbe.data = intelImage.mid(gbe.offset, gbe.length);
        regions.push_back(gbe);
    }

    // PDR region
    REGION_INFO pdr;
    pdr.type = Subtypes::PdrRegion;
    pdr.offset = 0;
    pdr.length = 0;
    if (regionSection->PdrLimit) {
        pdr.offset = calculateRegionOffset(regionSection->PdrBase);
        pdr.length = calculateRegionSize(regionSection->PdrBase, regionSection->PdrLimit);
        pdr.data = intelImage.mid(pdr.offset, pdr.length);
        regions.push_back(pdr);
    }

    // Reserved1 region
    REGION_INFO reserved1;
    reserved1.type = Subtypes::Reserved1Region;
    reserved1.offset = 0;
    reserved1.length = 0;
    if (regionSection->Reserved1Limit && regionSection->Reserved1Base != 0xFFFF && regionSection->Reserved1Limit != 0xFFFF) {
        reserved1.offset = calculateRegionOffset(regionSection->Reserved1Base);
        reserved1.length = calculateRegionSize(regionSection->Reserved1Base, regionSection->Reserved1Limit);
        reserved1.data = intelImage.mid(reserved1.offset, reserved1.length);
        regions.push_back(reserved1);
    }

    // Reserved2 region
    REGION_INFO reserved2;
    reserved2.type = Subtypes::Reserved2Region;
    reserved2.offset = 0;
    reserved2.length = 0;
    if (regionSection->Reserved2Limit && regionSection->Reserved2Base != 0xFFFF && regionSection->Reserved2Limit != 0xFFFF) {
        reserved2.offset = calculateRegionOffset(regionSection->Reserved2Base);
        reserved2.length = calculateRegionSize(regionSection->Reserved2Base, regionSection->Reserved2Limit);
        reserved2.data = intelImage.mid(reserved2.offset, reserved2.length);
        regions.push_back(reserved2);
    }

    // Reserved3 region
    REGION_INFO reserved3;
    reserved3.type = Subtypes::Reserved3Region;
    reserved3.offset = 0;
    reserved3.length = 0;

    // EC region
    REGION_INFO ec;
    ec.type = Subtypes::EcRegion;
    ec.offset = 0;
    ec.length = 0;

    // Reserved4 region
    REGION_INFO reserved4;
    reserved3.type = Subtypes::Reserved4Region;
    reserved4.offset = 0;
    reserved4.length = 0;

    // Check for EC and reserved region 4 only for v2 descriptor
    if (descriptorVersion == 2) {
        if (regionSection->Reserved3Limit) {
            reserved3.offset = calculateRegionOffset(regionSection->Reserved3Base);
            reserved3.length = calculateRegionSize(regionSection->Reserved3Base, regionSection->Reserved3Limit);
            reserved3.data = intelImage.mid(reserved3.offset, reserved3.length);
            regions.push_back(reserved3);
        }

        if (regionSection->EcLimit) {
            ec.offset = calculateRegionOffset(regionSection->EcBase);
            ec.length = calculateRegionSize(regionSection->EcBase, regionSection->EcLimit);
            ec.data = intelImage.mid(ec.offset, ec.length);
            regions.push_back(ec);
        }
    
        if (regionSection->Reserved4Limit) {
            reserved4.offset = calculateRegionOffset(regionSection->Reserved4Base);
            reserved4.length = calculateRegionSize(regionSection->Reserved4Base, regionSection->Reserved4Limit);
            reserved4.data = intelImage.mid(reserved4.offset, reserved4.length);
            regions.push_back(reserved4);
        }
    }

    // Sort regions in ascending order
    std::sort(regions.begin(), regions.end());

    // Check for intersections and paddings between regions
    REGION_INFO region;
    // Check intersection with the descriptor
    if (regions.front().offset < FLASH_DESCRIPTOR_SIZE) {
        msg(QObject::tr("parseIntelImage: %1 region has intersection with flash descriptor").arg(itemSubtypeToQString(Types::Region, regions.front().type)), index);
        return ERR_INVALID_FLASH_DESCRIPTOR;
    }
    // Check for padding between descriptor and the first region 
    else if (regions.front().offset > FLASH_DESCRIPTOR_SIZE) {
        region.offset = FLASH_DESCRIPTOR_SIZE;
        region.length = regions.front().offset - FLASH_DESCRIPTOR_SIZE;
        region.data = intelImage.mid(region.offset, region.length);
        region.type = getPaddingType(region.data);
        regions.insert(regions.begin(), region);
    }
    // Check for intersections/paddings between regions
    for (size_t i = 1; i < regions.size(); i++) {
        UINT32 previousRegionEnd = regions[i-1].offset + regions[i-1].length;
        // Check that current region is fully present in the image
        if (regions[i].offset + regions[i].length > (UINT32)intelImage.size()) {
            msg(QObject::tr("parseIntelImage: %1 region is located outside of opened image, if your system uses dual-chip storage, please append another part to the opened image")
                .arg(itemSubtypeToQString(Types::Region, regions[i].type)), index);
            return ERR_TRUNCATED_IMAGE;
        }

        // Check for intersection with previous region
        if (regions[i].offset < previousRegionEnd) {
            msg(QObject::tr("parseIntelImage: %1 region has intersection with %2 region")
                .arg(itemSubtypeToQString(Types::Region, regions[i].type))
                .arg(itemSubtypeToQString(Types::Region, regions[i-1].type)), index);
            return ERR_INVALID_FLASH_DESCRIPTOR;
        }
        // Check for padding between current and previous regions
        else if (regions[i].offset > previousRegionEnd) {
            region.offset = previousRegionEnd;
            region.length = regions[i].offset - previousRegionEnd;
            region.data = intelImage.mid(region.offset, region.length);
            region.type = getPaddingType(region.data);
            std::vector<REGION_INFO>::iterator iter = regions.begin();
            std::advance(iter, i - 1);
            regions.insert(iter, region);
        }
    }
    // Check for padding after the last region
    if (regions.back().offset + regions.back().length < (UINT32)intelImage.size()) {
        region.offset = regions.back().offset + regions.back().length;
        region.length = intelImage.size() - region.offset;
        region.data = intelImage.mid(region.offset, region.length);
        region.type = getPaddingType(region.data);
        regions.push_back(region);
    }
    
    // Region map is consistent

    // Intel image
    QString name = QObject::tr("Intel image");
    QString info = QObject::tr("Full size: %1h (%2)\nFlash chips: %3\nRegions: %4\nMasters: %5\nPCH straps: %6\nPROC straps: %7")
        .hexarg(intelImage.size()).arg(intelImage.size())
        .arg(descriptorMap->NumberOfFlashChips + 1) //
        .arg(descriptorMap->NumberOfRegions + 1)    // Zero-based numbers in storage
        .arg(descriptorMap->NumberOfMasters + 1)    //
        .arg(descriptorMap->NumberOfPchStraps)
        .arg(descriptorMap->NumberOfProcStraps);

    // Construct parsing data
    pdata.offset = parentOffset;

    // Add Intel image tree item
    index = model->addItem(Types::Image, Subtypes::IntelImage, name, QString(), info, QByteArray(), intelImage, TRUE, parsingDataToQByteArray(pdata), parent);

    // Descriptor
    // Get descriptor info
    QByteArray body = intelImage.left(FLASH_DESCRIPTOR_SIZE);
    name = QObject::tr("Descriptor region");
    info = QObject::tr("Full size: %1h (%2)").hexarg(FLASH_DESCRIPTOR_SIZE).arg(FLASH_DESCRIPTOR_SIZE);
    
    // Add offsets of actual regions
    for (size_t i = 0; i < regions.size(); i++) {
        if (regions[i].type != Subtypes::ZeroPadding && regions[i].type != Subtypes::OnePadding && regions[i].type != Subtypes::DataPadding)
            info += QObject::tr("\n%1 region offset: %2h").arg(itemSubtypeToQString(Types::Region, regions[i].type)).hexarg(regions[i].offset + parentOffset);
    }

    // Region access settings
    if (descriptorVersion == 1) {
        const FLASH_DESCRIPTOR_MASTER_SECTION* masterSection = (const FLASH_DESCRIPTOR_MASTER_SECTION*)calculateAddress8(descriptor, descriptorMap->MasterBase);
        info += QObject::tr("\nRegion access settings:");
        info += QObject::tr("\nBIOS: %1h %2h ME: %3h %4h\nGbE:  %5h %6h")
            .hexarg2(masterSection->BiosRead, 2)
            .hexarg2(masterSection->BiosWrite, 2)
            .hexarg2(masterSection->MeRead, 2)
            .hexarg2(masterSection->MeWrite, 2)
            .hexarg2(masterSection->GbeRead, 2)
            .hexarg2(masterSection->GbeWrite, 2);

        // BIOS access table
        info += QObject::tr("\nBIOS access table:");
        info += QObject::tr("\n      Read  Write");
        info += QObject::tr("\nDesc  %1  %2")
            .arg(masterSection->BiosRead  & FLASH_DESCRIPTOR_REGION_ACCESS_DESC ? "Yes " : "No  ")
            .arg(masterSection->BiosWrite & FLASH_DESCRIPTOR_REGION_ACCESS_DESC ? "Yes " : "No  ");
        info += QObject::tr("\nBIOS  Yes   Yes");
        info += QObject::tr("\nME    %1  %2")
            .arg(masterSection->BiosRead  & FLASH_DESCRIPTOR_REGION_ACCESS_ME ? "Yes " : "No  ")
            .arg(masterSection->BiosWrite & FLASH_DESCRIPTOR_REGION_ACCESS_ME ? "Yes " : "No  ");
        info += QObject::tr("\nGbE   %1  %2")
            .arg(masterSection->BiosRead  & FLASH_DESCRIPTOR_REGION_ACCESS_GBE ? "Yes " : "No  ")
            .arg(masterSection->BiosWrite & FLASH_DESCRIPTOR_REGION_ACCESS_GBE ? "Yes " : "No  ");
        info += QObject::tr("\nPDR   %1  %2")
            .arg(masterSection->BiosRead  & FLASH_DESCRIPTOR_REGION_ACCESS_PDR ? "Yes " : "No  ")
            .arg(masterSection->BiosWrite & FLASH_DESCRIPTOR_REGION_ACCESS_PDR ? "Yes " : "No  ");
    }
    else if (descriptorVersion == 2) {
        const FLASH_DESCRIPTOR_MASTER_SECTION_V2* masterSection = (const FLASH_DESCRIPTOR_MASTER_SECTION_V2*)calculateAddress8(descriptor, descriptorMap->MasterBase);
        info += QObject::tr("\nRegion access settings:");
        info += QObject::tr("\nBIOS: %1h %2h ME: %3h %4h\nGbE:  %5h %6h EC: %7h %8h")
            .hexarg2(masterSection->BiosRead, 3)
            .hexarg2(masterSection->BiosWrite, 3)
            .hexarg2(masterSection->MeRead, 3)
            .hexarg2(masterSection->MeWrite, 3)
            .hexarg2(masterSection->GbeRead, 3)
            .hexarg2(masterSection->GbeWrite, 3)
            .hexarg2(masterSection->EcRead, 3)
            .hexarg2(masterSection->EcWrite, 3);

        // BIOS access table
        info += QObject::tr("\nBIOS access table:");
        info += QObject::tr("\n      Read  Write");
        info += QObject::tr("\nDesc  %1  %2")
            .arg(masterSection->BiosRead  & FLASH_DESCRIPTOR_REGION_ACCESS_DESC ? "Yes " : "No  ")
            .arg(masterSection->BiosWrite & FLASH_DESCRIPTOR_REGION_ACCESS_DESC ? "Yes " : "No  ");
        info += QObject::tr("\nBIOS  Yes   Yes");
        info += QObject::tr("\nME    %1  %2")
            .arg(masterSection->BiosRead  & FLASH_DESCRIPTOR_REGION_ACCESS_ME ? "Yes " : "No  ")
            .arg(masterSection->BiosWrite & FLASH_DESCRIPTOR_REGION_ACCESS_ME ? "Yes " : "No  ");
        info += QObject::tr("\nGbE   %1  %2")
            .arg(masterSection->BiosRead  & FLASH_DESCRIPTOR_REGION_ACCESS_GBE ? "Yes " : "No  ")
            .arg(masterSection->BiosWrite & FLASH_DESCRIPTOR_REGION_ACCESS_GBE ? "Yes " : "No  ");
        info += QObject::tr("\nPDR   %1  %2")
            .arg(masterSection->BiosRead  & FLASH_DESCRIPTOR_REGION_ACCESS_PDR ? "Yes " : "No  ")
            .arg(masterSection->BiosWrite & FLASH_DESCRIPTOR_REGION_ACCESS_PDR ? "Yes " : "No  ");
        info += QObject::tr("\nEC    %1  %2")
            .arg(masterSection->BiosRead  & FLASH_DESCRIPTOR_REGION_ACCESS_EC ? "Yes " : "No  ")
            .arg(masterSection->BiosWrite & FLASH_DESCRIPTOR_REGION_ACCESS_EC ? "Yes " : "No  ");
    }

    // VSCC table
    const VSCC_TABLE_ENTRY* vsccTableEntry = (const VSCC_TABLE_ENTRY*)(descriptor + ((UINT16)upperMap->VsccTableBase << 4));
    info += QObject::tr("\nFlash chips in VSCC table:");
    UINT8 vsscTableSize = upperMap->VsccTableSize * sizeof(UINT32) / sizeof(VSCC_TABLE_ENTRY);
    for (int i = 0; i < vsscTableSize; i++) {
        info += QObject::tr("\n%1%2%3h")
            .hexarg2(vsccTableEntry->VendorId, 2)
            .hexarg2(vsccTableEntry->DeviceId0, 2)
            .hexarg2(vsccTableEntry->DeviceId1, 2);
        vsccTableEntry++;
    }

    // Add descriptor tree item
    QModelIndex regionIndex = model->addItem(Types::Region, Subtypes::DescriptorRegion, name, QString(), info, QByteArray(), body, TRUE, parsingDataToQByteArray(pdata), index);
    
    // Parse regions
    UINT8 result = ERR_SUCCESS;
    UINT8 parseResult = ERR_SUCCESS;
    for (size_t i = 0; i < regions.size(); i++) {
        region = regions[i];
        switch (region.type) {
        case Subtypes::BiosRegion:
            result = parseBiosRegion(region.data, region.offset, index, regionIndex);
            break;
        case Subtypes::MeRegion:
            result = parseMeRegion(region.data, region.offset, index, regionIndex);
            break;
        case Subtypes::GbeRegion:
            result = parseGbeRegion(region.data, region.offset, index, regionIndex);
            break;
        case Subtypes::PdrRegion:
            result = parsePdrRegion(region.data, region.offset, index, regionIndex);
            break;
        case Subtypes::Reserved1Region:
        case Subtypes::Reserved2Region:
        case Subtypes::Reserved3Region:
        case Subtypes::EcRegion:
        case Subtypes::Reserved4Region:
            result = parseGeneralRegion(region.type, region.data, region.offset, index, regionIndex);
            break;
        case Subtypes::ZeroPadding:
        case Subtypes::OnePadding:
        case Subtypes::DataPadding: {
            // Add padding between regions
            QByteArray padding = intelImage.mid(region.offset, region.length);

            // Get parent's parsing data
            PARSING_DATA pdata = parsingDataFromQModelIndex(index);

            // Get info
            name = QObject::tr("Padding");
            info = QObject::tr("Full size: %1h (%2)")
                .hexarg(padding.size()).arg(padding.size());

            // Construct parsing data
            pdata.offset = parentOffset + region.offset;

            // Add tree item
            regionIndex = model->addItem(Types::Padding, getPaddingType(padding), name, QString(), info, QByteArray(), padding, TRUE, parsingDataToQByteArray(pdata), index);
            result = ERR_SUCCESS;
            } break;
        default:
            msg(QObject::tr("parseIntelImage: region of unknown type found"), index);
            result = ERR_INVALID_FLASH_DESCRIPTOR;
        }
        // Store the first failed result as a final result
        if (!parseResult && result)
            parseResult = result;
    }

    return parseResult;
}

STATUS FfsParser::parseGbeRegion(const QByteArray & gbe, const UINT32 parentOffset, const QModelIndex & parent, QModelIndex & index)
{
    // Check sanity
    if (gbe.isEmpty())
        return ERR_EMPTY_REGION;
    if ((UINT32)gbe.size() < GBE_VERSION_OFFSET + sizeof(GBE_VERSION))
        return ERR_INVALID_REGION;

    // Get parent's parsing data
    PARSING_DATA pdata = parsingDataFromQModelIndex(parent);

    // Get info
    QString name = QObject::tr("GbE region");
    const GBE_MAC_ADDRESS* mac = (const GBE_MAC_ADDRESS*)gbe.constData();
    const GBE_VERSION* version = (const GBE_VERSION*)(gbe.constData() + GBE_VERSION_OFFSET);
    QString info = QObject::tr("Full size: %1h (%2)\nMAC: %3:%4:%5:%6:%7:%8\nVersion: %9.%10")
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
    pdata.offset += parentOffset;

    // Add tree item
    index = model->addItem(Types::Region, Subtypes::GbeRegion, name, QString(), info, QByteArray(), gbe, TRUE, parsingDataToQByteArray(pdata), parent);

    return ERR_SUCCESS;
}

STATUS FfsParser::parseMeRegion(const QByteArray & me, const UINT32 parentOffset, const QModelIndex & parent, QModelIndex & index)
{
    // Check sanity
    if (me.isEmpty())
        return ERR_EMPTY_REGION;

    // Get parent's parsing data
    PARSING_DATA pdata = parsingDataFromQModelIndex(parent);

    // Get info
    QString name = QObject::tr("ME region");
    QString info = QObject::tr("Full size: %1h (%2)").
        hexarg(me.size()).arg(me.size());

    // Parse region
    bool versionFound = true;
    bool emptyRegion = false;
    // Check for empty region
    if (me.count() == me.count('\xFF') || me.count() == me.count('\x00')) {
        // Further parsing not needed
        emptyRegion = true;
        info += QObject::tr("\nState: empty");
    }
    else {
        // Search for new signature
        INT32 versionOffset = me.indexOf(ME_VERSION_SIGNATURE2);
        if (versionOffset < 0){ // New signature not found
            // Search for old signature
            versionOffset = me.indexOf(ME_VERSION_SIGNATURE);
            if (versionOffset < 0){
                info += QObject::tr("\nVersion: unknown");
                versionFound = false;
            }
        }

        // Check sanity
        if ((UINT32)me.size() < (UINT32)versionOffset + sizeof(ME_VERSION))
            return ERR_INVALID_REGION;

        // Add version information
        if (versionFound) {
            const ME_VERSION* version = (const ME_VERSION*)(me.constData() + versionOffset);
            info += QObject::tr("\nVersion: %1.%2.%3.%4")
                .arg(version->major)
                .arg(version->minor)
                .arg(version->bugfix)
                .arg(version->build);
        }
    }

    // Construct parsing data
    pdata.offset += parentOffset;

    // Add tree item
    index = model->addItem(Types::Region, Subtypes::MeRegion, name, QString(), info, QByteArray(), me, TRUE, parsingDataToQByteArray(pdata), parent);

    // Show messages
    if (emptyRegion) {
        msg(QObject::tr("parseMeRegion: ME region is empty"), index);
    }
    else if (!versionFound) {
        msg(QObject::tr("parseMeRegion: ME version is unknown, it can be damaged"), index);
    }

    return ERR_SUCCESS;
}

STATUS FfsParser::parsePdrRegion(const QByteArray & pdr, const UINT32 parentOffset, const QModelIndex & parent, QModelIndex & index)
{
    // Check sanity
    if (pdr.isEmpty())
        return ERR_EMPTY_REGION;

    // Get parent's parsing data
    PARSING_DATA pdata = parsingDataFromQModelIndex(parent);

    // Get info
    QString name = QObject::tr("PDR region");
    QString info = QObject::tr("Full size: %1h (%2)").
        hexarg(pdr.size()).arg(pdr.size());

    // Construct parsing data
    pdata.offset += parentOffset;

    // Add tree item
    index = model->addItem(Types::Region, Subtypes::PdrRegion, name, QString(), info, QByteArray(), pdr, TRUE, parsingDataToQByteArray(pdata), parent);

    // Parse PDR region as BIOS space
    UINT8 result = parseRawArea(pdr, index);
    if (result && result != ERR_VOLUMES_NOT_FOUND && result != ERR_INVALID_VOLUME)
        return result;

    return ERR_SUCCESS;
}

STATUS FfsParser::parseGeneralRegion(const UINT8 subtype, const QByteArray & region, const UINT32 parentOffset, const QModelIndex & parent, QModelIndex & index)
{
    // Check sanity
    if (region.isEmpty())
        return ERR_EMPTY_REGION;

    // Get parent's parsing data
    PARSING_DATA pdata = parsingDataFromQModelIndex(parent);

    // Get info
    QString name = QObject::tr("%1 region").arg(itemSubtypeToQString(Types::Region, subtype));
    QString info = QObject::tr("Full size: %1h (%2)").
        hexarg(region.size()).arg(region.size());

    // Construct parsing data
    pdata.offset += parentOffset;

    // Add tree item
    index = model->addItem(Types::Region, subtype, name, QString(), info, QByteArray(), region, TRUE, parsingDataToQByteArray(pdata), parent);

    return ERR_SUCCESS;
}

STATUS FfsParser::parseBiosRegion(const QByteArray & bios, const UINT32 parentOffset, const QModelIndex & parent, QModelIndex & index)
{
    // Sanity check
    if (bios.isEmpty())
        return ERR_EMPTY_REGION;

    // Get parent's parsing data
    PARSING_DATA pdata = parsingDataFromQModelIndex(parent);

    // Get info
    QString name = QObject::tr("BIOS region");
    QString info = QObject::tr("Full size: %1h (%2)").
        hexarg(bios.size()).arg(bios.size());

    // Construct parsing data
    pdata.offset += parentOffset;

    // Add tree item
    index = model->addItem(Types::Region, Subtypes::BiosRegion, name, QString(), info, QByteArray(), bios, TRUE, parsingDataToQByteArray(pdata), parent);

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
    PARSING_DATA pdata = parsingDataFromQModelIndex(index);
    UINT32 headerSize = model->header(index).size();
    UINT32 offset = pdata.offset + headerSize;

    // Search for first volume
    STATUS result;
    UINT32 prevVolumeOffset;

    result = findNextVolume(index, data, offset, 0, prevVolumeOffset);
    if (result)
        return result;

    // First volume is not at the beginning of BIOS space
    QString name;
    QString info;
    if (prevVolumeOffset > 0) {
        // Get info
        QByteArray padding = data.left(prevVolumeOffset);
        name = QObject::tr("Padding");
        info = QObject::tr("Full size: %1h (%2)")
            .hexarg(padding.size()).arg(padding.size());

        // Construct parsing data
        pdata.offset = offset;

        // Add tree item
        model->addItem(Types::Padding, getPaddingType(padding), name, QString(), info, QByteArray(), padding, TRUE, parsingDataToQByteArray(pdata), index);
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
            name = QObject::tr("Padding");
            info = QObject::tr("Full size: %1h (%2)")
                .hexarg(padding.size()).arg(padding.size());

            // Construct parsing data
            pdata.offset = offset + paddingOffset;

            // Add tree item
            model->addItem(Types::Padding, getPaddingType(padding), name, QString(), info, QByteArray(), padding, TRUE, parsingDataToQByteArray(pdata), index);
        }

        // Get volume size
        UINT32 volumeSize = 0;
        UINT32 bmVolumeSize = 0;
        result = getVolumeSize(data, volumeOffset, volumeSize, bmVolumeSize);
        if (result) {
            msg(QObject::tr("parseRawArea: getVolumeSize failed with error \"%1\"").arg(errorCodeToQString(result)), index);
            return result;
        }
        
        // Check that volume is fully present in input
        if (volumeSize > (UINT32)data.size() || volumeOffset + volumeSize > (UINT32)data.size()) {
            msg(QObject::tr("parseRawArea: one of volumes inside overlaps the end of data"), index);
            return ERR_INVALID_VOLUME;
        }
        
        QByteArray volume = data.mid(volumeOffset, volumeSize);
        if (volumeSize > (UINT32)volume.size()) {
            // Mark the rest as padding and finish the parsing
            QByteArray padding = data.right(volume.size());

            // Get info
            name = QObject::tr("Padding");
            info = QObject::tr("Full size: %1h (%2)")
                .hexarg(padding.size()).arg(padding.size());

            // Construct parsing data
            pdata.offset = offset + volumeOffset;

            // Add tree item
            QModelIndex paddingIndex = model->addItem(Types::Padding, getPaddingType(padding), name, QString(), info, QByteArray(), padding, TRUE, parsingDataToQByteArray(pdata), index);
            msg(QObject::tr("parseRawArea: one of volumes inside overlaps the end of data"), paddingIndex);

            // Update variables
            prevVolumeOffset = volumeOffset;
            prevVolumeSize = padding.size();
            break;
        }

        // Parse current volume's header
        QModelIndex volumeIndex;
        result = parseVolumeHeader(volume, model->header(index).size() + volumeOffset, index, volumeIndex);
        if (result)
            msg(QObject::tr("parseRawArea: volume header parsing failed with error \"%1\"").arg(errorCodeToQString(result)), index);
        else {
            // Show messages
            if (volumeSize != bmVolumeSize)
                msg(QObject::tr("parseBiosBody: volume size stored in header %1h (%2) differs from calculated using block map %3h (%4)")
                .hexarg(volumeSize).arg(volumeSize)
                .hexarg(bmVolumeSize).arg(bmVolumeSize),
                volumeIndex);
        }

        // Go to next volume
        prevVolumeOffset = volumeOffset;
        prevVolumeSize = volumeSize;
        result = findNextVolume(index, data, offset, volumeOffset + prevVolumeSize, volumeOffset);
    }

    // Padding at the end of BIOS space
    volumeOffset = prevVolumeOffset + prevVolumeSize;
    if ((UINT32)data.size() > volumeOffset) {
        QByteArray padding = data.mid(volumeOffset);

        // Get info
        name = QObject::tr("Padding");
        info = QObject::tr("Full size: %1h (%2)")
            .hexarg(padding.size()).arg(padding.size());

        // Construct parsing data
        pdata.offset = offset + headerSize + volumeOffset;

        // Add tree item
        model->addItem(Types::Padding, getPaddingType(padding), name, QString(), info, QByteArray(), padding, TRUE, parsingDataToQByteArray(pdata), index);
    }

    // Parse bodies
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
    PARSING_DATA pdata = parsingDataFromQModelIndex(parent);

    // Check that there is space for the volume header
        if ((UINT32)volume.size() < sizeof(EFI_FIRMWARE_VOLUME_HEADER)) {
        msg(QObject::tr("parseVolumeHeader: input volume size %1h (%2) is smaller than volume header size 40h (64)").hexarg(volume.size()).arg(volume.size()));
        return ERR_INVALID_VOLUME;
    }

    // Populate volume header
    const EFI_FIRMWARE_VOLUME_HEADER* volumeHeader = (const EFI_FIRMWARE_VOLUME_HEADER*)(volume.constData());

    // Check sanity of HeaderLength value
    if ((UINT32)ALIGN8(volumeHeader->HeaderLength) > (UINT32)volume.size()) {
        msg(QObject::tr("parseVolumeHeader: volume header overlaps the end of data"));
        return ERR_INVALID_VOLUME;
    }
    // Check sanity of ExtHeaderOffset value
    if (volumeHeader->Revision > 1 && volumeHeader->ExtHeaderOffset
        && (UINT32)ALIGN8(volumeHeader->ExtHeaderOffset + sizeof(EFI_FIRMWARE_VOLUME_EXT_HEADER)) > (UINT32)volume.size()) {
        msg(QObject::tr("parseVolumeHeader: extended volume header overlaps the end of data"));
        return ERR_INVALID_VOLUME;
    }

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
    QByteArray guid = QByteArray((const char*)volumeHeader->FileSystemGuid.Data, sizeof(EFI_GUID));
    if (std::find(FFSv2Volumes.begin(), FFSv2Volumes.end(), guid) != FFSv2Volumes.end()) {
        isUnknown = false;
        ffsVersion = 2;
    }

    // Check for FFS v3 volume
    if (std::find(FFSv3Volumes.begin(), FFSv3Volumes.end(), guid) != FFSv3Volumes.end()) {
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
        if (!isUnknown && !model->compressed(parent) && ((pdata.offset + parentOffset - capsuleOffsetFixup) % alignment))
            msgUnaligned = true;
    }
    else
        msgUnknownRevision = true;

    // Check attributes
    // Determine value of empty byte
    UINT8 emptyByte = volumeHeader->Attributes & EFI_FVB_ERASE_POLARITY ? '\xFF' : '\x00';

    // Check for AppleCRC32 and AppleFreeSpaceOffset in ZeroVector
    bool hasAppleCrc32 = false;
    bool hasAppleFSO = false;
    UINT32 volumeSize = volume.size();
    UINT32 appleCrc32 = *(UINT32*)(volume.constData() + 8);
    UINT32 appleFSO = *(UINT32*)(volume.constData() + 12);
    if (appleCrc32 != 0) {
        // Calculate CRC32 of the volume body
        UINT32 crc = crc32(0, (const UINT8*)(volume.constData() + volumeHeader->HeaderLength), volumeSize - volumeHeader->HeaderLength);
        if (crc == appleCrc32) {
            hasAppleCrc32 = true;
        }

        // Check if FreeSpaceOffset is non-zero
        if (appleFSO != 0) {
            hasAppleFSO = true;
        }
    }

    // Check header checksum by recalculating it
    bool msgInvalidChecksum = false;
    QByteArray tempHeader((const char*)volumeHeader, volumeHeader->HeaderLength);
    ((EFI_FIRMWARE_VOLUME_HEADER*)tempHeader.data())->Checksum = 0;
    UINT16 calculated = calculateChecksum16((const UINT16*)tempHeader.constData(), volumeHeader->HeaderLength);
    if (volumeHeader->Checksum != calculated)
        msgInvalidChecksum = true;

    // Get info
    QByteArray header = volume.left(headerSize);
    QByteArray body = volume.mid(headerSize);
    QString name = guidToQString(volumeHeader->FileSystemGuid);
    QString info = QObject::tr("ZeroVector:\n%1 %2 %3 %4 %5 %6 %7 %8\n%9 %10 %11 %12 %13 %14 %15 %16\nFileSystem GUID: %17\nFull size: %18h (%19)\n"
        "Header size: %20h (%21)\nBody size: %22h (%23)\nRevision: %24\nAttributes: %25h\nErase polarity: %26\nChecksum: %27h, %28")
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
        .arg(emptyByte ? "1" : "0")
        .hexarg2(volumeHeader->Checksum, 4)
        .arg(msgInvalidChecksum ? QObject::tr("invalid, should be %1h").hexarg2(calculated, 4) : QObject::tr("valid"));

    // Extended header present
    if (volumeHeader->Revision > 1 && volumeHeader->ExtHeaderOffset) {
        const EFI_FIRMWARE_VOLUME_EXT_HEADER* extendedHeader = (const EFI_FIRMWARE_VOLUME_EXT_HEADER*)(volume.constData() + volumeHeader->ExtHeaderOffset);
        info += QObject::tr("\nExtended header size: %1h (%2)\nVolume GUID: %3")
            .hexarg(extendedHeader->ExtHeaderSize).arg(extendedHeader->ExtHeaderSize)
            .arg(guidToQString(extendedHeader->FvName));
    }

    // Construct parsing data
    pdata.offset += parentOffset;
    pdata.emptyByte = emptyByte;
    pdata.ffsVersion = ffsVersion;
    pdata.volume.hasExtendedHeader = hasExtendedHeader ? TRUE : FALSE;
    pdata.volume.extendedHeaderGuid = extendedHeaderGuid;
    pdata.volume.alignment = alignment;
    pdata.volume.revision = volumeHeader->Revision;
    pdata.volume.hasAppleCrc32 = hasAppleCrc32;
    pdata.volume.hasAppleFSO = hasAppleFSO;
    pdata.volume.isWeakAligned = (volumeHeader->Revision > 1 && (volumeHeader->Attributes & EFI_FVB2_WEAK_ALIGNMENT));

    // Add text
    QString text;
    if (hasAppleCrc32)
        text += QObject::tr("AppleCRC32 ");
    if (hasAppleFSO)
        text += QObject::tr("AppleFSO ");

    // Add tree item
    UINT8 subtype = Subtypes::UnknownVolume;
    if (!isUnknown) {
        if (ffsVersion == 2)
            subtype = Subtypes::Ffs2Volume;
        else if (ffsVersion == 3)
            subtype = Subtypes::Ffs3Volume;
    }
    index = model->addItem(Types::Volume, subtype, name, text, info, header, body, TRUE, parsingDataToQByteArray(pdata), parent);

    // Show messages
    if (isUnknown)
        msg(QObject::tr("parseVolumeHeader: unknown file system %1").arg(guidToQString(volumeHeader->FileSystemGuid)), index);
    if (msgInvalidChecksum)
        msg(QObject::tr("parseVolumeHeader: volume header checksum is invalid"), index);
    if (msgAlignmentBitsSet)
        msg(QObject::tr("parseVolumeHeader: alignment bits set on volume without alignment capability"), index);
    if (msgUnaligned)
        msg(QObject::tr("parseVolumeHeader: unaligned volume"), index);
    if (msgUnknownRevision)
        msg(QObject::tr("parseVolumeHeader: unknown volume revision %1").arg(volumeHeader->Revision), index);

    return ERR_SUCCESS;
}

STATUS FfsParser::findNextVolume(const QModelIndex & index, const QByteArray & bios, const UINT32 parentOffset, const UINT32 volumeOffset, UINT32 & nextVolumeOffset)
{
    int nextIndex = bios.indexOf(EFI_FV_SIGNATURE, volumeOffset);
    if (nextIndex < EFI_FV_SIGNATURE_OFFSET)
        return ERR_VOLUMES_NOT_FOUND;

    // Check volume header to be sane
    for (; nextIndex > 0; nextIndex = bios.indexOf(EFI_FV_SIGNATURE, nextIndex + 1)) {
        const EFI_FIRMWARE_VOLUME_HEADER* volumeHeader = (const EFI_FIRMWARE_VOLUME_HEADER*)(bios.constData() + nextIndex - EFI_FV_SIGNATURE_OFFSET);
        if (volumeHeader->FvLength < sizeof(EFI_FIRMWARE_VOLUME_HEADER) + 2 * sizeof(EFI_FV_BLOCK_MAP_ENTRY) || volumeHeader->FvLength >= 0xFFFFFFFFUL) {
            msg(QObject::tr("findNextVolume: volume candidate at offset %1h skipped, has invalid FvLength %2h").hexarg(parentOffset + (nextIndex - EFI_FV_SIGNATURE_OFFSET)).hexarg2(volumeHeader->FvLength, 16), index);
            continue;
        }
        if (volumeHeader->Reserved != 0xFF && volumeHeader->Reserved != 0x00) {
            msg(QObject::tr("findNextVolume: volume candidate at offset %1h skipped, has invalid Reserved byte value %2").hexarg(parentOffset + (nextIndex - EFI_FV_SIGNATURE_OFFSET)).hexarg2(volumeHeader->Reserved, 2), index);
            continue;
        }
        if (volumeHeader->Revision != 1 && volumeHeader->Revision != 2) {
            msg(QObject::tr("findNextVolume: volume candidate at offset %1h skipped, has invalid Revision byte value %2").hexarg(parentOffset + (nextIndex - EFI_FV_SIGNATURE_OFFSET)).hexarg2(volumeHeader->Revision, 2), index);
            continue;
        }
        // All checks passed, volume found
        break;
    }
    // No additional volumes found
    if (nextIndex < EFI_FV_SIGNATURE_OFFSET)
        return ERR_VOLUMES_NOT_FOUND;

    nextVolumeOffset = nextIndex - EFI_FV_SIGNATURE_OFFSET;
    return ERR_SUCCESS;
}

STATUS FfsParser::getVolumeSize(const QByteArray & bios, UINT32 volumeOffset, UINT32 & volumeSize, UINT32 & bmVolumeSize)
{
    // Check that there is space for the volume header and at least two block map entries.
    if ((UINT32)bios.size() < volumeOffset + sizeof(EFI_FIRMWARE_VOLUME_HEADER) + 2 * sizeof(EFI_FV_BLOCK_MAP_ENTRY))
        return ERR_INVALID_VOLUME;

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

    if (volumeSize == 0)
        return ERR_INVALID_VOLUME;

    return ERR_SUCCESS;
}

STATUS FfsParser::parseVolumeNonUefiData(const QByteArray & data, const UINT32 parentOffset, const QModelIndex & index)
{
    // Sanity check
    if (!index.isValid())
        return ERR_INVALID_PARAMETER;

    // Get parsing data
    PARSING_DATA pdata = parsingDataFromQModelIndex(index);

    // Modify it
    pdata.offset += parentOffset;

    // Search for VTF GUID backwards in received data
    QByteArray padding = data;
    QByteArray vtf;
    INT32 vtfIndex = data.lastIndexOf(EFI_FFS_VOLUME_TOP_FILE_GUID);
    if (vtfIndex >= 0) { // VTF candidate found inside non-UEFI data
        padding = data.left(vtfIndex);
        vtf = data.mid(vtfIndex);
        const EFI_FFS_FILE_HEADER* fileHeader = (const EFI_FFS_FILE_HEADER*)vtf.constData();
        if ((UINT32)vtf.size() < sizeof(EFI_FFS_FILE_HEADER) // VTF candidate is too small to be a real VTF in FFSv1/v2 volume
            || (pdata.ffsVersion == 3
                && (fileHeader->Attributes & FFS_ATTRIB_LARGE_FILE)
                && (UINT32)vtf.size() < sizeof(EFI_FFS_FILE_HEADER2))) { // VTF candidate is too small to be a real VTF in FFSv3 volume
            vtfIndex = -1;
            padding = data;
            vtf.clear();
        }
    }

    // Add non-UEFI data first
    // Get info
    QString info = QObject::tr("Full size: %1h (%2)").hexarg(padding.size()).arg(padding.size());

    // Add padding tree item
    QModelIndex paddingIndex = model->addItem(Types::Padding, Subtypes::DataPadding, QObject::tr("Non-UEFI data"), "", info, QByteArray(), padding, TRUE, parsingDataToQByteArray(pdata), index);
    msg(QObject::tr("parseVolumeNonUefiData: non-UEFI data found in volume's free space"), paddingIndex);

    if (vtfIndex >= 0) {
        // Get VTF file header
        QByteArray header = vtf.left(sizeof(EFI_FFS_FILE_HEADER));
        const EFI_FFS_FILE_HEADER* fileHeader = (const EFI_FFS_FILE_HEADER*)header.constData();
        if (pdata.ffsVersion == 3 && (fileHeader->Attributes & FFS_ATTRIB_LARGE_FILE)) {
            header = vtf.left(sizeof(EFI_FFS_FILE_HEADER2));
        }

        //Parse VTF file header
        QModelIndex fileIndex;
        STATUS result = parseFileHeader(vtf, parentOffset + vtfIndex, index, fileIndex);
        if (result) {
            msg(QObject::tr("parseVolumeNonUefiData: VTF file header parsing failed with error \"%1\"").arg(errorCodeToQString(result)), index);
            
            // Add the rest as non-UEFI data too
            pdata.offset += vtfIndex;
            // Get info
            QString info = QObject::tr("Full size: %1h (%2)").hexarg(vtf.size()).arg(vtf.size());

            // Add padding tree item
            QModelIndex paddingIndex = model->addItem(Types::Padding, Subtypes::DataPadding, QObject::tr("Non-UEFI data"), "", info, QByteArray(), vtf, TRUE, parsingDataToQByteArray(pdata), index);
            msg(QObject::tr("parseVolumeNonUefiData: non-UEFI data found in volume's free space"), paddingIndex);
        }
    }

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
    PARSING_DATA pdata = parsingDataFromQModelIndex(index);
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
                    pdata.offset = offset + volumeHeaderSize + fileOffset;

                    // Add all bytes before as free space
                    if (i > 0) {
                        QByteArray free = freeSpace.left(i);

                        // Get info
                        QString info = QObject::tr("Full size: %1h (%2)").hexarg(free.size()).arg(free.size());

                        // Add free space item
                        model->addItem(Types::FreeSpace, 0, QObject::tr("Volume free space"), "", info, QByteArray(), free, FALSE, parsingDataToQByteArray(pdata), index);
                    }

                    // Parse non-UEFI data 
                    parseVolumeNonUefiData(freeSpace.mid(i), volumeHeaderSize + fileOffset + i, index);
                }
                else {
                    // Construct parsing data
                    pdata.offset = offset + volumeHeaderSize + fileOffset;

                    // Get info
                    QString info = QObject::tr("Full size: %1h (%2)").hexarg(freeSpace.size()).arg(freeSpace.size());

                    // Add free space item
                    model->addItem(Types::FreeSpace, 0, QObject::tr("Volume free space"), "", info, QByteArray(), freeSpace, FALSE, parsingDataToQByteArray(pdata), index);
                }
                break; // Exit from parsing loop
            }
            else { //File space
                // Parse non-UEFI data 
                parseVolumeNonUefiData(volumeBody.mid(fileOffset), volumeHeaderSize + fileOffset, index);
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
            msg(QObject::tr("parseVolumeBody: file header parsing failed with error \"%1\"").arg(errorCodeToQString(result)), index);

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
        // Check files after current for having an equal GUID
        for (int j = i + 1; j < model->rowCount(index); j++) {
            QModelIndex another = index.child(j, 0);
            // Skip non-file entries
            if (model->type(another) != Types::File)
                continue;
            // Check GUIDs for being equal
            QByteArray anotherGuid = model->header(another).left(sizeof(EFI_GUID));
            if (currentGuid == anotherGuid) {
                msg(QObject::tr("parseVolumeBody: file with duplicate GUID %1").arg(guidToQString(*(const EFI_GUID*)anotherGuid.constData())), another);
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
        if ((UINT32)volume.size() < fileOffset + sizeof(EFI_FFS_FILE_HEADER))
            return 0;
        const EFI_FFS_FILE_HEADER* fileHeader = (const EFI_FFS_FILE_HEADER*)(volume.constData() + fileOffset);
        return uint24ToUint32(fileHeader->Size);
    }
    else if (ffsVersion == 3) {
        if ((UINT32)volume.size() < fileOffset + sizeof(EFI_FFS_FILE_HEADER2))
            return 0;
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

    if ((UINT32)file.size() < sizeof(EFI_FFS_FILE_HEADER))
        return ERR_INVALID_FILE;

    // Get parent's parsing data
    PARSING_DATA pdata = parsingDataFromQModelIndex(parent);

    // Get file header
    QByteArray header = file.left(sizeof(EFI_FFS_FILE_HEADER));
    const EFI_FFS_FILE_HEADER* fileHeader = (const EFI_FFS_FILE_HEADER*)header.constData();
    if (pdata.ffsVersion == 3 && (fileHeader->Attributes & FFS_ATTRIB_LARGE_FILE)) {
        if ((UINT32)file.size() < sizeof(EFI_FFS_FILE_HEADER2))
            return ERR_INVALID_FILE;
        header = file.left(sizeof(EFI_FFS_FILE_HEADER2));
    }

    // Check file alignment
    bool msgUnalignedFile = false;
    UINT8 alignmentPower = ffsAlignmentTable[(fileHeader->Attributes & FFS_ATTRIB_DATA_ALIGNMENT) >> 3];
    UINT32 alignment = (UINT32)pow(2.0, alignmentPower);
    if ((parentOffset + header.size()) % alignment)
        msgUnalignedFile = true;

    // Check file alignment agains volume alignment
    bool msgFileAlignmentIsGreaterThanVolumes = false;
    if (!pdata.volume.isWeakAligned && pdata.volume.alignment < alignment)
        msgFileAlignmentIsGreaterThanVolumes = true;

    // Check header checksum
    QByteArray tempHeader = header;
    EFI_FFS_FILE_HEADER* tempFileHeader = (EFI_FFS_FILE_HEADER*)(tempHeader.data());
    tempFileHeader->IntegrityCheck.Checksum.Header = 0;
    tempFileHeader->IntegrityCheck.Checksum.File = 0;
    UINT8 calculatedHeader = calculateChecksum8((const UINT8*)tempFileHeader, header.size() - 1);
    bool msgInvalidHeaderChecksum = false;
    if (fileHeader->IntegrityCheck.Checksum.Header != calculatedHeader)
        msgInvalidHeaderChecksum = true;

    // Check data checksum
    // Data checksum must be calculated
    bool msgInvalidDataChecksum = false;
    UINT8 calculatedData = 0;
    if (fileHeader->Attributes & FFS_ATTRIB_CHECKSUM) {
        UINT32 bufferSize = file.size() - header.size();
        // Exclude file tail from data checksum calculation
        if (pdata.volume.revision == 1 && (fileHeader->Attributes & FFS_ATTRIB_TAIL_PRESENT))
            bufferSize -= sizeof(UINT16);
        calculatedData = calculateChecksum8((const UINT8*)(file.constData() + header.size()), bufferSize);
        if (fileHeader->IntegrityCheck.Checksum.File != calculatedData)
            msgInvalidDataChecksum = true;
    }
    // Data checksum must be one of predefined values
    else if (pdata.volume.revision == 1 && fileHeader->IntegrityCheck.Checksum.File != FFS_FIXED_CHECKSUM) {
        calculatedData = FFS_FIXED_CHECKSUM;
        msgInvalidDataChecksum = true;
    }
    else if (pdata.volume.revision == 2 && fileHeader->IntegrityCheck.Checksum.File != FFS_FIXED_CHECKSUM2) {
        calculatedData = FFS_FIXED_CHECKSUM2;
        msgInvalidDataChecksum = true;
    }

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
        name = QObject::tr("Pad-file");

    info = QObject::tr("File GUID: %1\nType: %2h\nAttributes: %3h\nFull size: %4h (%5)\nHeader size: %6h (%7)\nBody size: %8h (%9)\nState: %10h\nHeader checksum: %11h, %12\nData checksum: %13h, %14")
        .arg(guidToQString(fileHeader->Name))
        .hexarg2(fileHeader->Type, 2)
        .hexarg2(fileHeader->Attributes, 2)
        .hexarg(header.size() + body.size()).arg(header.size() + body.size())
        .hexarg(header.size()).arg(header.size())
        .hexarg(body.size()).arg(body.size())
        .hexarg2(fileHeader->State, 2)
        .hexarg2(fileHeader->IntegrityCheck.Checksum.Header, 2)
        .arg(msgInvalidHeaderChecksum ? QObject::tr("invalid, should be %1h").hexarg2(calculatedHeader, 2) : QObject::tr("valid"))
        .hexarg2(fileHeader->IntegrityCheck.Checksum.File, 2)
        .arg(msgInvalidDataChecksum ? QObject::tr("invalid, should be %1h").hexarg2(calculatedData, 2) : QObject::tr("valid"));

    // Set raw file format to unknown by default
    pdata.file.format = RAW_FILE_FORMAT_UNKNOWN;

    QString text;
    bool isVtf = false;
    QByteArray guid = header.left(sizeof(EFI_GUID));
    // Check if the file is a Volume Top File
    if (guid == EFI_FFS_VOLUME_TOP_FILE_GUID) {
        // Mark it as the last VTF
        // This information will later be used to determine memory addresses of uncompressed image elements
        // Because the last byte of the last VFT is mapped to 0xFFFFFFFF physical memory address 
        isVtf = true;
        text = QObject::tr("Volume Top File");
    }
    // Check if the file is NVRAM storage with NVAR format
    else if (guid == NVRAM_NVAR_STORAGE_FILE_GUID || guid == NVRAM_NVAR_EXTERNAL_DEFAULTS_FILE_GUID) {
        // Mark the file as NVAR storage
        pdata.file.format = RAW_FILE_FORMAT_NVAR_STORAGE;
    }

    // Construct parsing data
    bool fixed = fileHeader->Attributes & FFS_ATTRIB_FIXED;
    pdata.offset += parentOffset;
    pdata.file.hasTail = hasTail ? TRUE : FALSE;
    pdata.file.tail = tail;

    // Add tree item
    index = model->addItem(Types::File, fileHeader->Type, name, text, info, header, body, fixed, parsingDataToQByteArray(pdata), parent);

    // Overwrite lastVtf, if needed
    if (isVtf) {
        lastVtf = index;
    }

    // Show messages
    if (msgUnalignedFile)
        msg(QObject::tr("parseFileHeader: unaligned file"), index);
    if (msgFileAlignmentIsGreaterThanVolumes)
        msg(QObject::tr("parseFileHeader: file alignment %1h is greater than parent volume alignment %2h").hexarg(alignment).hexarg(pdata.volume.alignment), index);
    if (msgInvalidHeaderChecksum)
        msg(QObject::tr("parseFileHeader: invalid header checksum"), index);
    if (msgInvalidDataChecksum)
        msg(QObject::tr("parseFileHeader: invalid data checksum"), index);
    if (msgInvalidTailValue)
        msg(QObject::tr("parseFileHeader: invalid tail value"), index);
    if (msgUnknownType)
        msg(QObject::tr("parseFileHeader: unknown file type %1h").hexarg2(fileHeader->Type, 2), index);

    return ERR_SUCCESS;
}

UINT32 FfsParser::getSectionSize(const QByteArray & file, const UINT32 sectionOffset, const UINT8 ffsVersion)
{
    if (ffsVersion == 2) {
        if ((UINT32)file.size() < sectionOffset + sizeof(EFI_COMMON_SECTION_HEADER))
            return 0;
        const EFI_COMMON_SECTION_HEADER* sectionHeader = (const EFI_COMMON_SECTION_HEADER*)(file.constData() + sectionOffset);
        return uint24ToUint32(sectionHeader->Size);
    }
    else if (ffsVersion == 3) {
        if ((UINT32)file.size() < sectionOffset + sizeof(EFI_COMMON_SECTION_HEADER2))
            return 0;
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
    if (model->subtype(index) == EFI_FV_FILETYPE_RAW || model->subtype(index) == EFI_FV_FILETYPE_ALL) {
        // Get data from parsing data
        PARSING_DATA pdata = parsingDataFromQModelIndex(index);

        // Parse NVAR storage
        if (pdata.file.format == RAW_FILE_FORMAT_NVAR_STORAGE)
            return parseNvarStorage(model->body(index), index);

        return parseRawArea(model->body(index), index);
    }

    // Parse sections
    return parseSections(model->body(index), index);
}

STATUS FfsParser::parsePadFileBody(const QModelIndex & index)
{
    // Sanity check
    if (!index.isValid())
        return ERR_INVALID_PARAMETER;

    // Get data from parsing data
    PARSING_DATA pdata = parsingDataFromQModelIndex(index);

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
        QString info = QObject::tr("Full size: %1h (%2)").hexarg(free.size()).arg(free.size());

        // Constuct parsing data
        pdata.offset += model->header(index).size();

        // Add tree item
        model->addItem(Types::FreeSpace, 0, QObject::tr("Free space"), QString(), info, QByteArray(), free, FALSE, parsingDataToQByteArray(pdata), index);
    }
    else 
        i = 0;

    // ... and all bytes after as a padding
    QByteArray padding = body.mid(i);

    // Get info
    QString info = QObject::tr("Full size: %1h (%2)").hexarg(padding.size()).arg(padding.size());

    // Constuct parsing data
    pdata.offset += i;

    // Add tree item
    QModelIndex dataIndex = model->addItem(Types::Padding, Subtypes::DataPadding, QObject::tr("Non-UEFI data"), "", info, QByteArray(), padding, TRUE, parsingDataToQByteArray(pdata), index);

    // Show message
    msg(QObject::tr("parsePadFileBody: non-UEFI data found in pad-file"), dataIndex);

    // Rename the file
    model->setName(index, QObject::tr("Non-empty pad-file"));

    return ERR_SUCCESS;
}

STATUS FfsParser::parseSections(const QByteArray & sections, const QModelIndex & index, const bool preparse)
{
    // Sanity check
    if (!index.isValid())
        return ERR_INVALID_PARAMETER;

    // Get data from parsing data
    PARSING_DATA pdata = parsingDataFromQModelIndex(index);

    // Search for and parse all sections
    UINT32 bodySize = sections.size();
    UINT32 headerSize = model->header(index).size();
    UINT32 sectionOffset = 0;

    STATUS result = ERR_SUCCESS;
    while (sectionOffset < bodySize) {
        // Get section size
        UINT32 sectionSize = getSectionSize(sections, sectionOffset, pdata.ffsVersion);

        // Check section size
        if (sectionSize < sizeof(EFI_COMMON_SECTION_HEADER) || sectionSize > (bodySize - sectionOffset)) {
            // Add padding to fill the rest of sections
            QByteArray padding = sections.mid(sectionOffset);
            // Get info
            QString info = QObject::tr("Full size: %1h (%2)").hexarg(padding.size()).arg(padding.size());

            // Constuct parsing data
            pdata.offset += headerSize + sectionOffset;

            // Final parsing
            if (!preparse) {
                // Add tree item
                QModelIndex dataIndex = model->addItem(Types::Padding, Subtypes::DataPadding, QObject::tr("Non-UEFI data"), "", info, QByteArray(), padding, TRUE, parsingDataToQByteArray(pdata), index);

                // Show message
                msg(QObject::tr("parseSections: non-UEFI data found in sections area"), dataIndex);
            }
            // Preparsing
            else {
                return ERR_INVALID_SECTION;
            }
            break; // Exit from parsing loop
        }

        // Parse section header
        QModelIndex sectionIndex;
        result = parseSectionHeader(sections.mid(sectionOffset, sectionSize), headerSize + sectionOffset, index, sectionIndex, preparse);
        if (result) {
            if (!preparse)
                msg(QObject::tr("parseSections: section header parsing failed with error \"%1\"").arg(errorCodeToQString(result)), index);
            else
                return ERR_INVALID_SECTION;
        }
        // Move to next section
        sectionOffset += sectionSize;
        sectionOffset = ALIGN4(sectionOffset);
    }

    //Parse bodies, will be skipped on preparse phase
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

STATUS FfsParser::parseSectionHeader(const QByteArray & section, const UINT32 parentOffset, const QModelIndex & parent, QModelIndex & index, const bool preparse)
{
    // Check sanity
    if ((UINT32)section.size() < sizeof(EFI_COMMON_SECTION_HEADER))
        return ERR_INVALID_SECTION;

    const EFI_COMMON_SECTION_HEADER* sectionHeader = (const EFI_COMMON_SECTION_HEADER*)(section.constData());
    switch (sectionHeader->Type) {
    // Special
    case EFI_SECTION_COMPRESSION:           return parseCompressedSectionHeader(section, parentOffset, parent, index, preparse);
    case EFI_SECTION_GUID_DEFINED:          return parseGuidedSectionHeader(section, parentOffset, parent, index, preparse);
    case EFI_SECTION_FREEFORM_SUBTYPE_GUID: return parseFreeformGuidedSectionHeader(section, parentOffset, parent, index, preparse);
    case EFI_SECTION_VERSION:               return parseVersionSectionHeader(section, parentOffset, parent, index, preparse);
    case PHOENIX_SECTION_POSTCODE:
    case INSYDE_SECTION_POSTCODE:           return parsePostcodeSectionHeader(section, parentOffset, parent, index, preparse);
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
    case EFI_SECTION_RAW:                   return parseCommonSectionHeader(section, parentOffset, parent, index, preparse);
    // Unknown
    default: 
        STATUS result = parseCommonSectionHeader(section, parentOffset, parent, index, preparse);
        msg(QObject::tr("parseSectionHeader: section with unknown type %1h").hexarg2(sectionHeader->Type, 2), index);
        return result;
    }
}

STATUS FfsParser::parseCommonSectionHeader(const QByteArray & section, const UINT32 parentOffset, const QModelIndex & parent, QModelIndex & index, const bool preparse)
{
    // Check sanity
    if ((UINT32)section.size() < sizeof(EFI_COMMON_SECTION_HEADER))
        return ERR_INVALID_SECTION;

    // Get data from parent's parsing data
    PARSING_DATA pdata = parsingDataFromQModelIndex(parent);

    // Obtain header fields
    const EFI_COMMON_SECTION_HEADER* sectionHeader = (const EFI_COMMON_SECTION_HEADER*)(section.constData());
    UINT32 headerSize = sizeof(EFI_COMMON_SECTION_HEADER);
    if (pdata.ffsVersion == 3 && uint24ToUint32(sectionHeader->Size) == EFI_SECTION2_IS_USED)
        headerSize = sizeof(EFI_COMMON_SECTION_HEADER2);

    QByteArray header = section.left(headerSize);
    QByteArray body = section.mid(headerSize);

    // Get info
    QString name = sectionTypeToQString(sectionHeader->Type) + QObject::tr(" section");
    QString info = QObject::tr("Type: %1h\nFull size: %2h (%3)\nHeader size: %4h (%5)\nBody size: %6h (%7)")
        .hexarg2(sectionHeader->Type, 2)
        .hexarg(section.size()).arg(section.size())
        .hexarg(headerSize).arg(headerSize)
        .hexarg(body.size()).arg(body.size());

    // Construct parsing data
    pdata.offset += parentOffset;

    // Add tree item
    if (!preparse) {
        index = model->addItem(Types::Section, sectionHeader->Type, name, QString(), info, header, body, FALSE, parsingDataToQByteArray(pdata), parent);
    } 
    return ERR_SUCCESS;
}

STATUS FfsParser::parseCompressedSectionHeader(const QByteArray & section, const UINT32 parentOffset, const QModelIndex & parent, QModelIndex & index, const bool preparse)
{
    // Check sanity
    if ((UINT32)section.size() < sizeof(EFI_COMPRESSION_SECTION))
        return ERR_INVALID_SECTION;

    // Get data from parent's parsing data
    PARSING_DATA pdata = parsingDataFromQModelIndex(parent);

    // Obtain header fields
    const EFI_COMMON_SECTION_HEADER* sectionHeader = (const EFI_COMMON_SECTION_HEADER*)(section.constData());
    const EFI_COMPRESSION_SECTION* compressedSectionHeader = (const EFI_COMPRESSION_SECTION*)sectionHeader;
    UINT32 headerSize = sizeof(EFI_COMPRESSION_SECTION);
    UINT8 compressionType = compressedSectionHeader->CompressionType;
    UINT32 uncompressedLength = compressedSectionHeader->UncompressedLength;
    if (pdata.ffsVersion == 3 && uint24ToUint32(sectionHeader->Size) == EFI_SECTION2_IS_USED) {
        if ((UINT32)section.size() < sizeof(EFI_COMPRESSION_SECTION2))
            return ERR_INVALID_SECTION;
        const EFI_COMPRESSION_SECTION2* compressedSectionHeader2 = (const EFI_COMPRESSION_SECTION2*)sectionHeader;
        headerSize = sizeof(EFI_COMPRESSION_SECTION2);
        compressionType = compressedSectionHeader2->CompressionType;
        uncompressedLength = compressedSectionHeader->UncompressedLength;
    }

    QByteArray header = section.left(headerSize);
    QByteArray body = section.mid(headerSize);

    // Get info
    QString name = sectionTypeToQString(sectionHeader->Type) + QObject::tr(" section");
    QString info = QObject::tr("Type: %1h\nFull size: %2h (%3)\nHeader size: %4h (%5)\nBody size: %6h (%7)\nCompression type: %8h\nDecompressed size: %9h (%10)")
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

    // Add tree item
    if (!preparse) {
        index = model->addItem(Types::Section, sectionHeader->Type, name, QString(), info, header, body, FALSE, parsingDataToQByteArray(pdata), parent);
    }
    return ERR_SUCCESS;
}

STATUS FfsParser::parseGuidedSectionHeader(const QByteArray & section, const UINT32 parentOffset, const QModelIndex & parent, QModelIndex & index, const bool preparse)
{
    // Check sanity
    if ((UINT32)section.size() < sizeof(EFI_GUID_DEFINED_SECTION))
        return ERR_INVALID_SECTION;

    // Get data from parent's parsing data
    PARSING_DATA pdata = parsingDataFromQModelIndex(parent);

    // Obtain header fields
    const EFI_COMMON_SECTION_HEADER* sectionHeader = (const EFI_COMMON_SECTION_HEADER*)(section.constData());
    const EFI_GUID_DEFINED_SECTION* guidDefinedSectionHeader = (const EFI_GUID_DEFINED_SECTION*)sectionHeader;
    EFI_GUID guid = guidDefinedSectionHeader->SectionDefinitionGuid;
    UINT16 dataOffset = guidDefinedSectionHeader->DataOffset;
    UINT16 attributes = guidDefinedSectionHeader->Attributes;
    UINT32 nextHeaderOffset = sizeof(EFI_GUID_DEFINED_SECTION);
    if (pdata.ffsVersion == 3 && uint24ToUint32(sectionHeader->Size) == EFI_SECTION2_IS_USED) {
        if ((UINT32)section.size() < sizeof(EFI_GUID_DEFINED_SECTION2))
            return ERR_INVALID_SECTION;
        const EFI_GUID_DEFINED_SECTION2* guidDefinedSectionHeader2 = (const EFI_GUID_DEFINED_SECTION2*)sectionHeader;
        guid = guidDefinedSectionHeader2->SectionDefinitionGuid;
        dataOffset = guidDefinedSectionHeader2->DataOffset;
        attributes = guidDefinedSectionHeader2->Attributes;
        nextHeaderOffset = sizeof(EFI_GUID_DEFINED_SECTION2);
    }

    // Check for special GUIDed sections
    QByteArray additionalInfo;
    QByteArray baGuid((const char*)&guid, sizeof(EFI_GUID));
    bool msgSignedSectionFound = false;
    bool msgNoAuthStatusAttribute = false;
    bool msgNoProcessingRequiredAttributeCompressed = false;
    bool msgNoProcessingRequiredAttributeSigned = false;
    bool msgInvalidCrc = false;
    bool msgUnknownCertType = false;
    bool msgUnknownCertSubtype = false;
    if (baGuid == EFI_GUIDED_SECTION_CRC32) {
        if ((attributes & EFI_GUIDED_SECTION_AUTH_STATUS_VALID) == 0) { // Check that AuthStatusValid attribute is set on compressed GUIDed sections
            msgNoAuthStatusAttribute = true;
        }

        if ((UINT32)section.size() < nextHeaderOffset + sizeof(UINT32))
            return ERR_INVALID_SECTION;

        UINT32 crc = *(UINT32*)(section.constData() + nextHeaderOffset);
        additionalInfo += QObject::tr("\nChecksum type: CRC32");
        // Calculate CRC32 of section data
        UINT32 calculated = crc32(0, (const UINT8*)section.constData() + dataOffset, section.size() - dataOffset);
        if (crc == calculated) {
            additionalInfo += QObject::tr("\nChecksum: %1h, valid").hexarg2(crc, 8);
        }
        else {
            additionalInfo += QObject::tr("\nChecksum: %1h, invalid, should be %2h").hexarg2(crc, 8).hexarg2(calculated, 8);
            msgInvalidCrc = true;
        }
        // No need to change dataOffset here
    }
    else if (baGuid == EFI_GUIDED_SECTION_LZMA || baGuid == EFI_GUIDED_SECTION_TIANO) {
        if ((attributes & EFI_GUIDED_SECTION_PROCESSING_REQUIRED) == 0) { // Check that ProcessingRequired attribute is set on compressed GUIDed sections
            msgNoProcessingRequiredAttributeCompressed = true;
        }
        // No need to change dataOffset here
    }
    else if (baGuid == EFI_FIRMWARE_CONTENTS_SIGNED_GUID) {
        if ((attributes & EFI_GUIDED_SECTION_PROCESSING_REQUIRED) == 0) { // Check that ProcessingRequired attribute is set on signed GUIDed sections
            msgNoProcessingRequiredAttributeSigned = true;
        }

        // Get certificate type and length
        if ((UINT32)section.size() < nextHeaderOffset + sizeof(WIN_CERTIFICATE))
            return ERR_INVALID_SECTION;

        const WIN_CERTIFICATE* winCertificate = (const WIN_CERTIFICATE*)(section.constData() + nextHeaderOffset);
        UINT32 certLength = winCertificate->Length;
        UINT16 certType = winCertificate->CertificateType;

        // Adjust dataOffset
        dataOffset += certLength;

        // Check section size once again
        if ((UINT32)section.size() < dataOffset)
            return ERR_INVALID_SECTION;

        // Check certificate type
        if (certType == WIN_CERT_TYPE_EFI_GUID) {
            additionalInfo += QObject::tr("\nCertificate type: UEFI");

            // Get certificate GUID
            const WIN_CERTIFICATE_UEFI_GUID* winCertificateUefiGuid = (const WIN_CERTIFICATE_UEFI_GUID*)(section.constData() + nextHeaderOffset);
            QByteArray certTypeGuid((const char*)&winCertificateUefiGuid->CertType, sizeof(EFI_GUID));

            if (certTypeGuid == EFI_CERT_TYPE_RSA2048_SHA256_GUID) {
                additionalInfo += QObject::tr("\nCertificate subtype: RSA2048/SHA256");
            }
            else {
                additionalInfo += QObject::tr("\nCertificate subtype: unknown, GUID %1").arg(guidToQString(winCertificateUefiGuid->CertType));
                msgUnknownCertSubtype = true;
            }
        }
        else {
            additionalInfo += QObject::tr("\nCertificate type: unknown (%1h)").hexarg2(certType, 4);
            msgUnknownCertType = true;
        }
        msgSignedSectionFound = true;
    }

    QByteArray header = section.left(dataOffset);
    QByteArray body = section.mid(dataOffset);

    // Get info
    QString name = guidToQString(guid);
    QString info = QObject::tr("Section GUID: %1\nType: %2h\nFull size: %3h (%4)\nHeader size: %5h (%6)\nBody size: %7h (%8)\nData offset: %9h\nAttributes: %10h")
        .arg(name)
        .hexarg2(sectionHeader->Type, 2)
        .hexarg(section.size()).arg(section.size())
        .hexarg(header.size()).arg(header.size())
        .hexarg(body.size()).arg(body.size())
        .hexarg(dataOffset)
        .hexarg2(attributes, 4);

    // Append additional info
    info.append(additionalInfo);

    // Construct parsing data
    pdata.offset += parentOffset;
    pdata.section.guidDefined.guid = guid;

    // Add tree item
    if (!preparse) {
        index = model->addItem(Types::Section, sectionHeader->Type, name, QString(), info, header, body, FALSE, parsingDataToQByteArray(pdata), parent);

        // Show messages
        if (msgSignedSectionFound)
            msg(QObject::tr("parseGuidedSectionHeader: section signature may become invalid after any modification"), index);
        if (msgNoAuthStatusAttribute)
            msg(QObject::tr("parseGuidedSectionHeader: CRC32 GUIDed section without AuthStatusValid attribute"), index);
        if (msgNoProcessingRequiredAttributeCompressed)
            msg(QObject::tr("parseGuidedSectionHeader: compressed GUIDed section without ProcessingRequired attribute"), index);
        if (msgNoProcessingRequiredAttributeSigned)
            msg(QObject::tr("parseGuidedSectionHeader: signed GUIDed section without ProcessingRequired attribute"), index);
        if (msgInvalidCrc)
            msg(QObject::tr("parseGuidedSectionHeader: GUID defined section with invalid CRC32"), index);
        if (msgUnknownCertType)
            msg(QObject::tr("parseGuidedSectionHeader: signed GUIDed section with unknown type"), index);
        if (msgUnknownCertSubtype)
            msg(QObject::tr("parseGuidedSectionHeader: signed GUIDed section with unknown subtype"), index);
    }

    return ERR_SUCCESS;
}

STATUS FfsParser::parseFreeformGuidedSectionHeader(const QByteArray & section, const UINT32 parentOffset, const QModelIndex & parent, QModelIndex & index, const bool preparse)
{
    // Check sanity
    if ((UINT32)section.size() < sizeof(EFI_FREEFORM_SUBTYPE_GUID_SECTION))
        return ERR_INVALID_SECTION;

    // Get data from parent's parsing data
    PARSING_DATA pdata = parsingDataFromQModelIndex(parent);

    // Obtain header fields
    const EFI_COMMON_SECTION_HEADER* sectionHeader = (const EFI_COMMON_SECTION_HEADER*)(section.constData());
    const EFI_FREEFORM_SUBTYPE_GUID_SECTION* fsgHeader = (const EFI_FREEFORM_SUBTYPE_GUID_SECTION*)sectionHeader;
    UINT32 headerSize = sizeof(EFI_FREEFORM_SUBTYPE_GUID_SECTION);
    EFI_GUID guid = fsgHeader->SubTypeGuid;
    if (pdata.ffsVersion == 3 && uint24ToUint32(sectionHeader->Size) == EFI_SECTION2_IS_USED) {
        if ((UINT32)section.size() < sizeof(EFI_FREEFORM_SUBTYPE_GUID_SECTION2))
            return ERR_INVALID_SECTION;
        const EFI_FREEFORM_SUBTYPE_GUID_SECTION2* fsgHeader2 = (const EFI_FREEFORM_SUBTYPE_GUID_SECTION2*)sectionHeader;
        headerSize = sizeof(EFI_FREEFORM_SUBTYPE_GUID_SECTION2);
        guid = fsgHeader2->SubTypeGuid;
    }

    QByteArray header = section.left(headerSize);
    QByteArray body = section.mid(headerSize);

    // Get info
    QString name = sectionTypeToQString(sectionHeader->Type) + QObject::tr(" section");
    QString info = QObject::tr("Type: %1h\nFull size: %2h (%3)\nHeader size: %4h (%5)\nBody size: %6h (%7)\nSubtype GUID: %8")
        .hexarg2(fsgHeader->Type, 2)
        .hexarg(section.size()).arg(section.size())
        .hexarg(header.size()).arg(header.size())
        .hexarg(body.size()).arg(body.size())
        .arg(guidToQString(guid));

    // Construct parsing data
    pdata.offset += parentOffset;
    pdata.section.freeformSubtypeGuid.guid = guid;

    // Add tree item
    if (!preparse) {
        index = model->addItem(Types::Section, sectionHeader->Type, name, QString(), info, header, body, FALSE, parsingDataToQByteArray(pdata), parent);

        // Rename section
        model->setName(index, guidToQString(guid));
    }
    return ERR_SUCCESS;
}

STATUS FfsParser::parseVersionSectionHeader(const QByteArray & section, const UINT32 parentOffset, const QModelIndex & parent, QModelIndex & index, const bool preparse)
{
    // Check sanity
    if ((UINT32)section.size() < sizeof(EFI_VERSION_SECTION))
        return ERR_INVALID_SECTION;

    // Get data from parent's parsing data
    PARSING_DATA pdata = parsingDataFromQModelIndex(parent);

    // Obtain header fields
    const EFI_COMMON_SECTION_HEADER* sectionHeader = (const EFI_COMMON_SECTION_HEADER*)(section.constData());
    const EFI_VERSION_SECTION* versionHeader = (const EFI_VERSION_SECTION*)sectionHeader;
    UINT32 headerSize = sizeof(EFI_VERSION_SECTION);
    UINT16 buildNumber = versionHeader->BuildNumber;
    if (pdata.ffsVersion == 3 && uint24ToUint32(sectionHeader->Size) == EFI_SECTION2_IS_USED) {
        if ((UINT32)section.size() < sizeof(EFI_VERSION_SECTION2))
            return ERR_INVALID_SECTION;
        const EFI_VERSION_SECTION2* versionHeader2 = (const EFI_VERSION_SECTION2*)sectionHeader;
        headerSize = sizeof(EFI_VERSION_SECTION2);
        buildNumber = versionHeader2->BuildNumber;
    }

    QByteArray header = section.left(headerSize);
    QByteArray body = section.mid(headerSize);
    
    // Get info
    QString name = sectionTypeToQString(sectionHeader->Type) + QObject::tr(" section");
    QString info = QObject::tr("Type: %1h\nFull size: %2h (%3)\nHeader size: %4h (%5)\nBody size: %6h (%7)\nBuild number: %8")
        .hexarg2(versionHeader->Type, 2)
        .hexarg(section.size()).arg(section.size())
        .hexarg(header.size()).arg(header.size())
        .hexarg(body.size()).arg(body.size())
        .arg(buildNumber);

    // Construct parsing data
    pdata.offset += parentOffset;

    // Add tree item
    if (!preparse) {
        index = model->addItem(Types::Section, sectionHeader->Type, name, QString(), info, header, body, FALSE, parsingDataToQByteArray(pdata), parent);
    }
    return ERR_SUCCESS;
}

STATUS FfsParser::parsePostcodeSectionHeader(const QByteArray & section, const UINT32 parentOffset, const QModelIndex & parent, QModelIndex & index, const bool preparse)
{
    // Check sanity
    if ((UINT32)section.size() < sizeof(POSTCODE_SECTION))
        return ERR_INVALID_SECTION;

    // Get data from parent's parsing data
    PARSING_DATA pdata = parsingDataFromQModelIndex(parent);

    // Obtain header fields
    const EFI_COMMON_SECTION_HEADER* sectionHeader = (const EFI_COMMON_SECTION_HEADER*)(section.constData());
    const POSTCODE_SECTION* postcodeHeader = (const POSTCODE_SECTION*)sectionHeader;
    UINT32 headerSize = sizeof(POSTCODE_SECTION);
    UINT32 postCode = postcodeHeader->Postcode;
    if (pdata.ffsVersion == 3 && uint24ToUint32(sectionHeader->Size) == EFI_SECTION2_IS_USED) {
        if ((UINT32)section.size() < sizeof(POSTCODE_SECTION2))
            return ERR_INVALID_SECTION;
        const POSTCODE_SECTION2* postcodeHeader2 = (const POSTCODE_SECTION2*)sectionHeader;
        headerSize = sizeof(POSTCODE_SECTION2);
        postCode = postcodeHeader2->Postcode;
    }

    QByteArray header = section.left(headerSize);
    QByteArray body = section.mid(headerSize);

    // Get info
    QString name = sectionTypeToQString(sectionHeader->Type) + QObject::tr(" section");
    QString info = QObject::tr("Type: %1h\nFull size: %2h (%3)\nHeader size: %4h (%5)\nBody size: %6h (%7)\nPostcode: %8h")
        .hexarg2(postcodeHeader->Type, 2)
        .hexarg(section.size()).arg(section.size())
        .hexarg(header.size()).arg(header.size())
        .hexarg(body.size()).arg(body.size())
        .hexarg(postCode);

    // Construct parsing data
    pdata.offset += parentOffset;

    // Add tree item
    if (!preparse) {
        index = model->addItem(Types::Section, sectionHeader->Type, name, QString(), info, header, body, FALSE, parsingDataToQByteArray(pdata), parent);
    }
    return ERR_SUCCESS;
}


STATUS FfsParser::parseSectionBody(const QModelIndex & index)
{
    // Sanity check
    if (!index.isValid())
        return ERR_INVALID_PARAMETER;
    QByteArray header = model->header(index);
    if ((UINT32)header.size() < sizeof(EFI_COMMON_SECTION_HEADER))
        return ERR_INVALID_SECTION;
    
    const EFI_COMMON_SECTION_HEADER* sectionHeader = (const EFI_COMMON_SECTION_HEADER*)(header.constData());

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
    case PHOENIX_SECTION_POSTCODE:
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
    PARSING_DATA pdata = parsingDataFromQModelIndex(index);
    UINT8 algorithm = pdata.section.compressed.compressionType;

    // Decompress section
    QByteArray decompressed;
    QByteArray efiDecompressed;
    STATUS result = decompress(model->body(index), algorithm, decompressed, efiDecompressed);
    if (result) {
        msg(QObject::tr("parseCompressedSectionBody: decompression failed with error \"%1\"").arg(errorCodeToQString(result)), index);
        return ERR_SUCCESS;
    }
    
    // Check reported uncompressed size
    if (pdata.section.compressed.uncompressedSize != (UINT32)decompressed.size()) {
        msg(QObject::tr("parseCompressedSectionBody: decompressed size stored in header %1h (%2) differs from actual %3h (%4)")
            .hexarg(pdata.section.compressed.uncompressedSize)
            .arg(pdata.section.compressed.uncompressedSize)
            .hexarg(decompressed.size())
            .arg(decompressed.size()), index);
        model->addInfo(index, QObject::tr("\nActual decompressed size: %1h (%2)").hexarg(decompressed.size()).arg(decompressed.size()));
    }

    // Check for undecided compression algorithm, this is a special case
    if (algorithm == COMPRESSION_ALGORITHM_UNDECIDED) {
        // Try preparse of sections decompressed with Tiano algorithm
        if (ERR_SUCCESS == parseSections(decompressed, index, true)) {
            algorithm = COMPRESSION_ALGORITHM_TIANO;
        }
        // Try preparse of sections decompressed with EFI 1.1 algorithm
        else if (ERR_SUCCESS == parseSections(efiDecompressed, index, true)) {
            algorithm = COMPRESSION_ALGORITHM_EFI11;
            decompressed = efiDecompressed;
        }
        else {
            msg(QObject::tr("parseCompressedSectionBody: can't guess the correct decompression algorithm, both preparse steps are failed"), index);
        }
    }

    // Add info
    model->addInfo(index, QObject::tr("\nCompression algorithm: %1").arg(compressionTypeToQString(algorithm)));

    // Update data
    pdata.section.compressed.algorithm = algorithm;
    if (algorithm != COMPRESSION_ALGORITHM_NONE)
        model->setCompressed(index, true);
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
    PARSING_DATA pdata = parsingDataFromQModelIndex(index);
    EFI_GUID guid = pdata.section.guidDefined.guid;

    // Check if section requires processing
    QByteArray processed = model->body(index);
    QByteArray efiDecompressed;
    QString info;
    bool parseCurrentSection = true;
    UINT8 algorithm = COMPRESSION_ALGORITHM_NONE;
    // Tiano compressed section
    if (QByteArray((const char*)&guid, sizeof(EFI_GUID)) == EFI_GUIDED_SECTION_TIANO) {
        algorithm = EFI_STANDARD_COMPRESSION;
        STATUS result = decompress(model->body(index), algorithm, processed, efiDecompressed);
        if (result) {
            parseCurrentSection = false;
            msg(QObject::tr("parseGuidedSectionBody: decompression failed with error \"%1\"").arg(errorCodeToQString(result)), index);
            return ERR_SUCCESS;
        }

        // Check for undecided compression algorithm, this is a special case
        if (algorithm == COMPRESSION_ALGORITHM_UNDECIDED) {
            // Try preparse of sections decompressed with Tiano algorithm
            if (ERR_SUCCESS == parseSections(processed, index, true)) {
                algorithm = COMPRESSION_ALGORITHM_TIANO;
            }
            // Try preparse of sections decompressed with EFI 1.1 algorithm
            else if (ERR_SUCCESS == parseSections(efiDecompressed, index, true)) {
                algorithm = COMPRESSION_ALGORITHM_EFI11;
                processed = efiDecompressed;
            }
            else {
                msg(QObject::tr("parseGuidedSectionBody: can't guess the correct decompression algorithm, both preparse steps are failed"), index);
            }
        }
        
        info += QObject::tr("\nCompression algorithm: %1").arg(compressionTypeToQString(algorithm));
        info += QObject::tr("\nDecompressed size: %1h (%2)").hexarg(processed.length()).arg(processed.length());
    }
    // LZMA compressed section
    else if (QByteArray((const char*)&guid, sizeof(EFI_GUID)) == EFI_GUIDED_SECTION_LZMA) {
        algorithm = EFI_CUSTOMIZED_COMPRESSION;
        STATUS result = decompress(model->body(index), algorithm, processed, efiDecompressed);
        if (result) {
            parseCurrentSection = false;
            msg(QObject::tr("parseGuidedSectionBody: decompression failed with error \"%1\"").arg(errorCodeToQString(result)), index);
            return ERR_SUCCESS;
        }

        if (algorithm == COMPRESSION_ALGORITHM_LZMA) {
            info += QObject::tr("\nCompression algorithm: LZMA");
            info += QObject::tr("\nDecompressed size: %1h (%2)").hexarg(processed.length()).arg(processed.length());
        }
        else
            info += QObject::tr("\nCompression algorithm: unknown");
    }

    // Add info
    model->addInfo(index, info);

    // Update data
    if (algorithm != COMPRESSION_ALGORITHM_NONE)
        model->setCompressed(index, true);
    model->setParsingData(index, parsingDataToQByteArray(pdata));

    if (!parseCurrentSection) {
        msg(QObject::tr("parseGuidedSectionBody: GUID defined section can not be processed"), index);
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
    model->addInfo(index, QObject::tr("\nVersion string: %1").arg(QString::fromUtf16((const ushort*)model->body(index).constData())));

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
    if (body.size() < 2) { // 2 is a minimal sane value, i.e TRUE + END
        msg(QObject::tr("parseDepexSectionBody: DEPEX section too short"), index);
        return ERR_DEPEX_PARSE_FAILED;
    }

    const EFI_GUID * guid;
    const UINT8* current = (const UINT8*)body.constData();

    // Special cases of first opcode
    switch (*current) {
    case EFI_DEP_BEFORE:
        if (body.size() != 2 * EFI_DEP_OPCODE_SIZE + sizeof(EFI_GUID)) {
            msg(QObject::tr("parseDepexSectionBody: DEPEX section too long for a section starting with BEFORE opcode"), index);
            return ERR_SUCCESS;
        }
        guid = (const EFI_GUID*)(current + EFI_DEP_OPCODE_SIZE);
        parsed += QObject::tr("\nBEFORE %1").arg(guidToQString(*guid));
        current += EFI_DEP_OPCODE_SIZE + sizeof(EFI_GUID);
        if (*current != EFI_DEP_END){
            msg(QObject::tr("parseDepexSectionBody: DEPEX section ends with non-END opcode"), index);
            return ERR_SUCCESS;
        }
        return ERR_SUCCESS;
    case EFI_DEP_AFTER:
        if (body.size() != 2 * EFI_DEP_OPCODE_SIZE + sizeof(EFI_GUID)){
            msg(QObject::tr("parseDepexSectionBody: DEPEX section too long for a section starting with AFTER opcode"), index);
            return ERR_SUCCESS;
        }
        guid = (const EFI_GUID*)(current + EFI_DEP_OPCODE_SIZE);
        parsed += QObject::tr("\nAFTER %1").arg(guidToQString(*guid));
        current += EFI_DEP_OPCODE_SIZE + sizeof(EFI_GUID);
        if (*current != EFI_DEP_END) {
            msg(QObject::tr("parseDepexSectionBody: DEPEX section ends with non-END opcode"), index);
            return ERR_SUCCESS;
        }
        return ERR_SUCCESS;
    case EFI_DEP_SOR:
        if (body.size() <= 2 * EFI_DEP_OPCODE_SIZE) {
            msg(QObject::tr("parseDepexSectionBody: DEPEX section too short for a section starting with SOR opcode"), index);
            return ERR_SUCCESS;
        }
        parsed += QObject::tr("\nSOR");
        current += EFI_DEP_OPCODE_SIZE;
        break;
    }

    // Parse the rest of depex 
    while (current - (const UINT8*)body.constData() < body.size()) {
        switch (*current) {
        case EFI_DEP_BEFORE: {
            msg(QObject::tr("parseDepexSectionBody: misplaced BEFORE opcode"), index);
            return ERR_SUCCESS;
        }
        case EFI_DEP_AFTER: {
            msg(QObject::tr("parseDepexSectionBody: misplaced AFTER opcode"), index);
            return ERR_SUCCESS;
        }
        case EFI_DEP_SOR: {
            msg(QObject::tr("parseDepexSectionBody: misplaced SOR opcode"), index);
            return ERR_SUCCESS;
        }
        case EFI_DEP_PUSH:
            // Check that the rest of depex has correct size
            if ((UINT32)body.size() - (UINT32)(current - (const UINT8*)body.constData()) <= EFI_DEP_OPCODE_SIZE + sizeof(EFI_GUID)) {
                parsed.clear();
                msg(QObject::tr("parseDepexSectionBody: remains of DEPEX section too short for PUSH opcode"), index);
                return ERR_SUCCESS;
            }
            guid = (const EFI_GUID*)(current + EFI_DEP_OPCODE_SIZE);
            parsed += QObject::tr("\nPUSH %1").arg(guidToQString(*guid));
            current += EFI_DEP_OPCODE_SIZE + sizeof(EFI_GUID);
            break;
        case EFI_DEP_AND:
            parsed += QObject::tr("\nAND");
            current += EFI_DEP_OPCODE_SIZE;
            break;
        case EFI_DEP_OR:
            parsed += QObject::tr("\nOR");
            current += EFI_DEP_OPCODE_SIZE;
            break;
        case EFI_DEP_NOT:
            parsed += QObject::tr("\nNOT");
            current += EFI_DEP_OPCODE_SIZE;
            break;
        case EFI_DEP_TRUE:
            parsed += QObject::tr("\nTRUE");
            current += EFI_DEP_OPCODE_SIZE;
            break;
        case EFI_DEP_FALSE:
            parsed += QObject::tr("\nFALSE");
            current += EFI_DEP_OPCODE_SIZE;
            break;
        case EFI_DEP_END:
            parsed += QObject::tr("\nEND");
            current += EFI_DEP_OPCODE_SIZE;
            // Check that END is the last opcode
            if (current - (const UINT8*)body.constData() < body.size()) {
                parsed.clear();
                msg(QObject::tr("parseDepexSectionBody: DEPEX section ends with non-END opcode"), index);
            }
            break;
        default:
            msg(QObject::tr("parseDepexSectionBody: unknown opcode"), index);
            return ERR_SUCCESS;
            break;
        }
    }
    
    // Add info
    model->addInfo(index, QObject::tr("\nParsed expression:%1").arg(parsed));

    return ERR_SUCCESS;
}

STATUS FfsParser::parseUiSectionBody(const QModelIndex & index)
{
    // Sanity check
    if (!index.isValid())
        return ERR_INVALID_PARAMETER;

    QString text = QString::fromUtf16((const ushort*)model->body(index).constData());

    // Add info
    model->addInfo(index, QObject::tr("\nText: %1").arg(text));

    // Rename parent file
    model->setText(model->findParentOfType(index, Types::File), text);

    return ERR_SUCCESS;
}

STATUS FfsParser::parseAprioriRawSection(const QByteArray & body, QString & parsed)
{
    // Sanity check
    if (body.size() % sizeof(EFI_GUID)) {
        msg(QObject::tr("parseAprioriRawSection: apriori file has size is not a multiple of 16"));
    }
    parsed.clear();
    UINT32 count = body.size() / sizeof(EFI_GUID);
    if (count > 0) {
        for (UINT32 i = 0; i < count; i++) {
            const EFI_GUID* guid = (const EFI_GUID*)body.constData() + i;
            parsed += QObject::tr("\n%1").arg(guidToQString(*guid));
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
            model->addInfo(index, QObject::tr("\nFile list:%1").arg(str));

        // Set parent file text
        model->setText(parentFile, QObject::tr("PEI apriori file"));

        return ERR_SUCCESS;
    }
    else if (parentFileGuid == EFI_DXE_APRIORI_FILE_GUID) { // DXE apriori file
        // Parse apriori file list
        QString str;
        STATUS result = parseAprioriRawSection(model->body(index), str);
        if (!result && !str.isEmpty())
            model->addInfo(index, QObject::tr("\nFile list:%1").arg(str));

        // Set parent file text
        model->setText(parentFile, QObject::tr("DXE apriori file"));

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
    if ((UINT32)body.size() < sizeof(EFI_IMAGE_DOS_HEADER)) {
        msg(QObject::tr("parsePeImageSectionBody: section body size is smaller than DOS header size"), index);
        return ERR_SUCCESS;
    }

    QByteArray info;
    const EFI_IMAGE_DOS_HEADER* dosHeader = (const EFI_IMAGE_DOS_HEADER*)body.constData();
    if (dosHeader->e_magic != EFI_IMAGE_DOS_SIGNATURE) {
        info += QObject::tr("\nDOS signature: %1h, invalid").hexarg2(dosHeader->e_magic, 4);
        msg(QObject::tr("parsePeImageSectionBody: PE32 image with invalid DOS signature"), index);
        model->addInfo(index, info);
        return ERR_SUCCESS;
    }

    const EFI_IMAGE_PE_HEADER* peHeader = (EFI_IMAGE_PE_HEADER*)(body.constData() + dosHeader->e_lfanew);
    if (body.size() < (UINT8*)peHeader - (UINT8*)dosHeader) {
        info += QObject::tr("\nDOS header: invalid");
        msg(QObject::tr("parsePeImageSectionBody: PE32 image with invalid DOS header"), index);
        model->addInfo(index, info);
        return ERR_SUCCESS;
    }

    if (peHeader->Signature != EFI_IMAGE_PE_SIGNATURE) {
        info += QObject::tr("\nPE signature: %1h, invalid").hexarg2(peHeader->Signature, 8);
        msg(QObject::tr("parsePeImageSectionBody: PE32 image with invalid PE signature"), index);
        model->addInfo(index, info);
        return ERR_SUCCESS;
    }

    const EFI_IMAGE_FILE_HEADER* imageFileHeader = (const EFI_IMAGE_FILE_HEADER*)(peHeader + 1);
    if (body.size() < (UINT8*)imageFileHeader - (UINT8*)dosHeader) {
        info += QObject::tr("\nPE header: invalid");
        msg(QObject::tr("parsePeImageSectionBody: PE32 image with invalid PE header"), index);
        model->addInfo(index, info);
        return ERR_SUCCESS;
    }

    info += QObject::tr("\nDOS signature: %1h\nPE signature: %2h\nMachine type: %3\nNumber of sections: %4\nCharacteristics: %5h")
        .hexarg2(dosHeader->e_magic, 4)
        .hexarg2(peHeader->Signature, 8)
        .arg(machineTypeToQString(imageFileHeader->Machine))
        .arg(imageFileHeader->NumberOfSections)
        .hexarg2(imageFileHeader->Characteristics, 4);

    EFI_IMAGE_OPTIONAL_HEADER_POINTERS_UNION optionalHeader;
    optionalHeader.H32 = (const EFI_IMAGE_OPTIONAL_HEADER32*)(imageFileHeader + 1);
    if (body.size() < (UINT8*)optionalHeader.H32 - (UINT8*)dosHeader) {
        info += QObject::tr("\nPE optional header: invalid");
        msg(QObject::tr("parsePeImageSectionBody: PE32 image with invalid PE optional header"), index);
        model->addInfo(index, info);
        return ERR_SUCCESS;
    }

    if (optionalHeader.H32->Magic == EFI_IMAGE_PE_OPTIONAL_HDR32_MAGIC) {
        info += QObject::tr("\nOptional header signature: %1h\nSubsystem: %2h\nAddress of entry point: %3h\nBase of code: %4h\nImage base: %5h")
            .hexarg2(optionalHeader.H32->Magic, 4)
            .hexarg2(optionalHeader.H32->Subsystem, 4)
            .hexarg(optionalHeader.H32->AddressOfEntryPoint)
            .hexarg(optionalHeader.H32->BaseOfCode)
            .hexarg(optionalHeader.H32->ImageBase);
    }
    else if (optionalHeader.H32->Magic == EFI_IMAGE_PE_OPTIONAL_HDR64_MAGIC) {
        info += QObject::tr("\nOptional header signature: %1h\nSubsystem: %2h\nAddress of entry point: %3h\nBase of code: %4h\nImage base: %5h")
            .hexarg2(optionalHeader.H64->Magic, 4)
            .hexarg2(optionalHeader.H64->Subsystem, 4)
            .hexarg(optionalHeader.H64->AddressOfEntryPoint)
            .hexarg(optionalHeader.H64->BaseOfCode)
            .hexarg(optionalHeader.H64->ImageBase);
    }
    else {
        info += QObject::tr("\nOptional header signature: %1h, unknown").hexarg2(optionalHeader.H32->Magic, 4);
        msg(QObject::tr("parsePeImageSectionBody: PE32 image with invalid optional PE header signature"), index);
    }

    model->addInfo(index, info);
    return ERR_SUCCESS;
}


STATUS FfsParser::parseTeImageSectionBody(const QModelIndex & index)
{
    // Check sanity
    if (!index.isValid())
        return ERR_INVALID_PARAMETER;

    // Get section body
    QByteArray body = model->body(index);
    if ((UINT32)body.size() < sizeof(EFI_IMAGE_TE_HEADER)) {
        msg(QObject::tr("parsePeImageSectionBody: section body size is smaller than TE header size"), index);
        return ERR_SUCCESS;
    }

    QByteArray info;
    const EFI_IMAGE_TE_HEADER* teHeader = (const EFI_IMAGE_TE_HEADER*)body.constData();
    if (teHeader->Signature != EFI_IMAGE_TE_SIGNATURE) {
        info += QObject::tr("\nSignature: %1h, invalid").hexarg2(teHeader->Signature, 4);
        msg(QObject::tr("parseTeImageSectionBody: TE image with invalid TE signature"), index);
    }
    else {
        info += QObject::tr("\nSignature: %1h\nMachine type: %2\nNumber of sections: %3\nSubsystem: %4h\nStripped size: %5h (%6)\nBase of code: %7h\nAddress of entry point: %8h\nImage base: %9h\nAdjusted image base: %10h")
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
    PARSING_DATA pdata = parsingDataFromQModelIndex(index);
    pdata.section.teImage.imageBase = teHeader->ImageBase;
    pdata.section.teImage.adjustedImageBase = teHeader->ImageBase + teHeader->StrippedSize - sizeof(EFI_IMAGE_TE_HEADER);
    
    // Update parsing data
    model->setParsingData(index, parsingDataToQByteArray(pdata));

    // Add TE info
    model->addInfo(index, info);

    return ERR_SUCCESS;
}


STATUS FfsParser::performSecondPass(const QModelIndex & index)
{
    // Sanity check
    if (!index.isValid() || !lastVtf.isValid())
        return ERR_INVALID_PARAMETER;

    // Check for compressed lastVtf
    if (model->compressed(lastVtf)) {
        msg(QObject::tr("performSecondPass: the last VTF appears inside compressed item, the image may be damaged"), lastVtf);
        return ERR_SUCCESS;
    }

    // Get parsing data for the last VTF
    PARSING_DATA pdata = parsingDataFromQModelIndex(lastVtf);

    // Calculate address difference
    const UINT32 vtfSize = model->header(lastVtf).size() + model->body(lastVtf).size() + (pdata.file.hasTail ? sizeof(UINT16) : 0);
    const UINT32 diff = 0xFFFFFFFFUL - pdata.offset - vtfSize + 1;

    // Apply address information to index and all it's child items
    addMemoryAddressesRecursive(index, diff);

    return ERR_SUCCESS;
}

STATUS FfsParser::addMemoryAddressesRecursive(const QModelIndex & index, const UINT32 diff)
{
    // Sanity check
    if (!index.isValid())
        return ERR_SUCCESS;
    
    // Set address value for non-compressed data
    if (!model->compressed(index)) {
        // Get parsing data for the current item
        PARSING_DATA pdata = parsingDataFromQModelIndex(index);

        // Check address sanity
        if ((const UINT64)diff + pdata.offset <= 0xFFFFFFFFUL)  {
            // Update info
            pdata.address = diff + pdata.offset;
            UINT32 headerSize = model->header(index).size();
            if (headerSize) {
                model->addInfo(index, QObject::tr("\nHeader memory address: %1h").hexarg2(pdata.address, 8));
                model->addInfo(index, QObject::tr("\nData memory address: %1h").hexarg2(pdata.address + headerSize, 8));
            }
            else {
                model->addInfo(index, QObject::tr("\nMemory address: %1h").hexarg2(pdata.address, 8));
            }

            // Special case of uncompressed TE image sections
            if (model->type(index) == Types::Section && model->subtype(index) == EFI_SECTION_TE) {
                // Check data memory address to be equal to either ImageBase or AdjustedImageBase
                if (pdata.section.teImage.imageBase == pdata.address + headerSize) {
                    pdata.section.teImage.revision = 1;
                }
                else if (pdata.section.teImage.adjustedImageBase == pdata.address + headerSize) {
                    pdata.section.teImage.revision = 2;
                }
                else {
                    msg(QObject::tr("addMemoryAddressesRecursive: image base is nether original nor adjusted, it's likely a part of backup PEI volume or DXE volume, but can also be damaged"), index);
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

STATUS FfsParser::addOffsetsRecursive(const QModelIndex & index)
{
    // Sanity check
    if (!index.isValid())
        return ERR_INVALID_PARAMETER;
    
    // Get parsing data for the current item
    PARSING_DATA pdata = parsingDataFromQModelIndex(index);

    // Add current offset if the element is not compressed
    // or it's compressed, but it's parent isn't
    if ((!model->compressed(index)) || (index.parent().isValid() && !model->compressed(index.parent()))) {
        model->addInfo(index, QObject::tr("Offset: %1h\n").hexarg(pdata.offset), false);
    }

    //TODO: show FIT file fixed attribute correctly
    model->addInfo(index, QObject::tr("\nCompressed: %1").arg(model->compressed(index) ? QObject::tr("Yes") : QObject::tr("No")));
    model->addInfo(index, QObject::tr("\nFixed: %1").arg(model->fixed(index) ? QObject::tr("Yes") : QObject::tr("No")));

    // Process child items
    for (int i = 0; i < model->rowCount(index); i++) {
        addOffsetsRecursive(index.child(i, 0));
    }

    return ERR_SUCCESS;
}

STATUS FfsParser::parseNvarStorage(const QByteArray & data, const QModelIndex & index)
{
    // Sanity check
    if (!index.isValid())
        return ERR_INVALID_PARAMETER;

    // Get parsing data for the current item
    PARSING_DATA pdata = parsingDataFromQModelIndex(index);
    UINT32 parentOffset = pdata.offset + model->header(index).size();

    // Rename parent file
    model->setText(model->findParentOfType(index, Types::File), QObject::tr("NVAR storage"));

    UINT32 offset = 0;
    UINT32 guidsInStorage = 0;
    
    // Parse all variables
    while (1) {
        bool msgUnknownExtDataFormat = false;
        bool msgExtHeaderTooLong = false;
        bool msgExtDataTooShort = false;

        bool isInvalid = false;
        bool isDataOnly = false;
        bool hasExtendedHeader = false;
        bool hasChecksum = false;
        bool hasTimestampAndHash = false;
        bool hasGuidIndex = false;

        UINT32 guidIndex = 0;
        UINT8  storedChecksum = 0;
        UINT8  calculatedChecksum = 0;
        UINT16 extendedHeaderSize = 0;
        UINT8  extendedAttributes = 0;
        UINT64 timestamp = 0;
        QByteArray hash;

        UINT8 subtype = Subtypes::FullNvar;
        QString name;
        QString text;
        QByteArray header;
        QByteArray body;
        QByteArray extendedData;



        UINT32 guidAreaSize = guidsInStorage * sizeof(EFI_GUID);
        UINT32 unparsedSize = (UINT32)data.size() - offset - guidAreaSize;

        // Get variable header
        const NVAR_VARIABLE_HEADER* variableHeader = (const NVAR_VARIABLE_HEADER*)(data.constData() + offset);
        
        // Check variable header
        if (unparsedSize < sizeof(NVAR_VARIABLE_HEADER) ||
            variableHeader->Signature != NVRAM_NVAR_VARIABLE_SIGNATURE ||
            unparsedSize < variableHeader->Size) {

            // Check if the data left is a free space or a padding
            QByteArray padding = data.mid(offset, unparsedSize);
            UINT8 type;
            
            if (padding.count(pdata.emptyByte) == padding.size()) {
                // It's a free space
                name = QObject::tr("Free space");
                type = Types::FreeSpace;
                subtype = 0;
            }
            else {
                // Nothing is parsed yet, but the file is not empty 
                if (!offset) {
                    msg(QObject::tr("parseNvarStorage: file can't be parsed as NVAR variables storage"), index);
                    return ERR_INVALID_FILE;
                }

                // It's a padding
                name = QObject::tr("Padding");
                type = Types::Padding;
                subtype = getPaddingType(padding);
            }
            // Get info
            QString info = QObject::tr("Full size: %1h (%2)")
                .hexarg(padding.size()).arg(padding.size());
            // Construct parsing data
            pdata.offset = parentOffset + offset;
            // Add tree item
            model->addItem(type, subtype, name, QString(), info, QByteArray(), padding, FALSE, parsingDataToQByteArray(pdata), index);

            // Add GUID storage area
            QByteArray guidArea = data.right(guidAreaSize);
            // Get info
            name = QObject::tr("GUID storage area");
            info = QObject::tr("Full size: %1h (%2)\nGUIDs in storage: %3")
                .hexarg(guidArea.size()).arg(guidArea.size())
                .arg(guidsInStorage);
            // Construct parsing data
            pdata.offset = parentOffset + offset + padding.size();
            // Add tree item
            model->addItem(Types::Padding, getPaddingType(guidArea), name, QString(), info, QByteArray(), guidArea, FALSE, parsingDataToQByteArray(pdata), index);

            return ERR_SUCCESS;
        }
        
        // Contruct generic header and body
        header = data.mid(offset, sizeof(NVAR_VARIABLE_HEADER));
        body = data.mid(offset + sizeof(NVAR_VARIABLE_HEADER), variableHeader->Size - sizeof(NVAR_VARIABLE_HEADER));

        UINT32 lastVariableFlag = pdata.emptyByte ? 0xFFFFFF : 0;
        
        // Set default next to predefined last value
        pdata.nvram.nvar.next = lastVariableFlag;

        // Variable is marked as invalid
        if ((variableHeader->Attributes & NVRAM_NVAR_VARIABLE_ATTRIB_VALID) == 0) { // Valid attribute is not set
            isInvalid = true;
            // Do not parse further
            goto parsing_done;
        }

        // Add next node information to parsing data
        if (variableHeader->Next != lastVariableFlag) {
            subtype = Subtypes::LinkNvar;
            pdata.nvram.nvar.next = variableHeader->Next;
        }
        
        // Variable with extended header
        if (variableHeader->Attributes & NVRAM_NVAR_VARIABLE_ATTRIB_EXT_HEADER) {
            hasExtendedHeader = true;
            msgUnknownExtDataFormat = true;

            extendedHeaderSize = *(UINT16*)(body.constData() + body.size() - sizeof(UINT16));
            if (extendedHeaderSize > body.size()) {
                msgExtHeaderTooLong = true;
                isInvalid = true;
                // Do not parse further
                goto parsing_done;
            }

            extendedAttributes = *(UINT8*)(body.constData() + body.size() - extendedHeaderSize);

            // Variable with checksum
            if (extendedAttributes & NVRAM_NVAR_VARIABLE_EXT_ATTRIB_CHECKSUM) {
                // Get stored checksum
                storedChecksum = *(UINT8*)(body.constData() + body.size() - sizeof(UINT16) - sizeof(UINT8));

                // Recalculate checksum for the variable
                calculatedChecksum = 0;
                // Include variable data
                UINT8* start = (UINT8*)(variableHeader + 1);
                for (UINT8* p = start; p < start + variableHeader->Size - sizeof(NVAR_VARIABLE_HEADER); p++) {
                    calculatedChecksum += *p;
                }
                // Include variable size and flags
                start = (UINT8*)&variableHeader->Size;
                for (UINT8*p = start; p < start + sizeof(UINT16); p++) {
                    calculatedChecksum += *p;
                }
                // Include variable attributes
                calculatedChecksum += variableHeader->Attributes;
                
                hasChecksum = true;
                msgUnknownExtDataFormat = false;
            }

            extendedData = body.mid(body.size() - extendedHeaderSize + sizeof(UINT8), extendedHeaderSize - sizeof(UINT16) - sizeof(UINT8) - (hasChecksum ? 1 : 0));
            body = body.left(body.size() - extendedHeaderSize);

            // Variable with authenticated write (for SecureBoot)
            if (variableHeader->Attributes & NVRAM_NVAR_VARIABLE_ATTRIB_AUTH_WRITE) {
                if (extendedData.size() < 40) {
                    msgExtDataTooShort = true;
                    isInvalid = true;
                    // Do not parse further
                    goto parsing_done;
                }

                timestamp = *(UINT64*)(extendedData.constData());
                hash = extendedData.mid(sizeof(UINT64), 0x20); //Length of SHA256 hash
                hasTimestampAndHash = true;
                msgUnknownExtDataFormat = false;
            }
        }

        // Variable is data-only (nameless and GUIDless link)
        if (variableHeader->Attributes & NVRAM_NVAR_VARIABLE_ATTRIB_DATA_ONLY) { // Data-only attribute is set
            isInvalid = true;
            QModelIndex nvarIndex;
            // Search prevously added variable for a link to this variable
            for (int i = 0; i < model->rowCount(index); i++) {
                nvarIndex = index.child(i, 0);
                PARSING_DATA nvarPdata = parsingDataFromQModelIndex(nvarIndex);
                if (nvarPdata.nvram.nvar.next + nvarPdata.offset - parentOffset == offset) { // Previous link is present and valid
                    isInvalid = false;
                    break;
                }
            }
            // Check if the link is valid
            if (!isInvalid) {
                // Use the name and text of the previous link
                name = model->name(nvarIndex);
                text = model->text(nvarIndex);

                if (variableHeader->Next == lastVariableFlag)
                    subtype = Subtypes::DataNvar;
            }

            isDataOnly = true;
            // Do not parse further
            goto parsing_done;
        }

        // Get variable name
        {
            UINT32 nameOffset = (variableHeader->Attributes & NVRAM_NVAR_VARIABLE_ATTRIB_GUID) ? sizeof(EFI_GUID) : 1; // GUID can be stored with the variable or in a separate storage, so there will only be an index of it
            CHAR8* namePtr = (CHAR8*)(variableHeader + 1) + nameOffset;
            UINT32 nameSize = 0;
            if (variableHeader->Attributes & NVRAM_NVAR_VARIABLE_ATTRIB_ASCII_NAME) { // Name is stored as ASCII string of CHAR8s
                text = QString(namePtr);
                nameSize = text.length() + 1;
            }
            else { // Name is stored as UCS2 string of CHAR16s
                text = QString::fromUtf16((CHAR16*)namePtr);
                nameSize = (text.length() + 1) * 2;
            }

            // Get variable GUID
            if (variableHeader->Attributes & NVRAM_NVAR_VARIABLE_ATTRIB_GUID) { // GUID is strored in the variable itself
                name = guidToQString(*(EFI_GUID*)(variableHeader + 1));
            }
            // GUID is stored in GUID list at the end of the storage
            else {
                guidIndex = *(UINT8*)(variableHeader + 1);
                if (guidsInStorage < guidIndex + 1)
                    guidsInStorage = guidIndex + 1;

                // The list begins at the end of the storage and goes backwards
                const EFI_GUID* guidPtr = (const EFI_GUID*)(data.constData() + data.size()) - 1 - guidIndex;
                name = guidToQString(*guidPtr);
                hasGuidIndex = true;
            }

            // Include variable name and GUID into the header and remove them from body
            header = data.mid(offset, sizeof(NVAR_VARIABLE_HEADER) + nameOffset + nameSize);
            body = body.mid(nameOffset + nameSize);
        }
parsing_done:
        QString info;
        // Rename invalid variables according to their types
        if (isInvalid) {
            if (variableHeader->Next != lastVariableFlag) {
                name = QObject::tr("Invalid link");
                subtype = Subtypes::InvalidLinkNvar;
            }
            else {
                name = QObject::tr("Invalid");
                subtype = Subtypes::InvalidNvar;
            }
        }
        else // Add GUID info for valid variables
            info += QObject::tr("Variable GUID: %1\n").arg(name);
        
        // Add GUID index information
        if (hasGuidIndex)
            info += QObject::tr("GUID index: %1\n").arg(guidIndex);

        // Add header, body and extended data info
        info += QObject::tr("Full size: %1h (%2)\nHeader size %3h (%4)\nBody size: %5h (%6)")
            .hexarg(variableHeader->Size).arg(variableHeader->Size)
            .hexarg(header.size()).arg(header.size())
            .hexarg(body.size()).arg(body.size());
        
        // Add attributes info
        info += QObject::tr("\nAttributes: %1h").hexarg2(variableHeader->Attributes, 2);
        // Translate attributes to text
        if (variableHeader->Attributes) 
            info += QObject::tr("\nAttributes as text: %1").arg(variableAttributesToQstring(variableHeader->Attributes));
        pdata.nvram.nvar.attributes = variableHeader->Attributes;

        // Add next node info
        if (!isInvalid && variableHeader->Next != lastVariableFlag)
            info += QObject::tr("\nNext node at offset: %1h").hexarg(parentOffset + offset + variableHeader->Next);

        // Add extended header info
        if (hasExtendedHeader) {
            info += QObject::tr("\nExtended header size: %1h (%2)\nExtended attributes: %3h")
                .hexarg(extendedHeaderSize).arg(extendedHeaderSize)
                .hexarg2(extendedAttributes, 2);
            pdata.nvram.nvar.extendedAttributes = extendedAttributes;
            // Checksum
            if (hasChecksum)
                info += QObject::tr("\nChecksum: %1h%2").hexarg2(storedChecksum, 2)
                    .arg(calculatedChecksum ? QObject::tr(", invalid, should be %1h").hexarg2(0x100 - calculatedChecksum, 2) : QObject::tr(", valid"));
            // Extended data
            if (!extendedData.isEmpty())
                info += QObject::tr("\nExtended data size: %1h (%2)")
                    .hexarg(extendedData.size()).arg(extendedData.size());
            // Authentication data
            if (hasTimestampAndHash) {
                info += QObject::tr("\nTimestamp: %1h\nHash: %2")
                    .hexarg2(timestamp, 16).arg(QString(hash.toHex()));

                pdata.nvram.nvar.timestamp = timestamp;
                memcpy(pdata.nvram.nvar.hash, hash.constData(), 0x20);
            }
        }
        
        // Add correct offset to parsing data
        pdata.offset = parentOffset + offset;

        // Add tree item
        QModelIndex varIndex = model->addItem(Types::NvramVariableNvar, subtype, name, text, info, header, body, FALSE, parsingDataToQByteArray(pdata), index);

        // Show messages
        if (msgUnknownExtDataFormat)
            msg(QObject::tr("parseNvarStorage: unknown extended data format"), varIndex);
        if (msgExtHeaderTooLong)
            msg(QObject::tr("parseNvarStorage: extended header size (%1h) is greater than body size (%2h)")
            .hexarg(extendedHeaderSize).hexarg(body.size()), varIndex);
        if (msgExtDataTooShort)
            msg(QObject::tr("parseNvarStorage: extended data size (%1h) is smaller than required for timestamp and hash (0x28)")
            .hexarg(extendedData.size()), varIndex);

        // Check variable name to be in the list of nesting variables
        if (text.toLatin1() == QString("StdDefaults") || text.toLatin1() == QString("MfgDefaults")) {
            STATUS result = parseNvarStorage(body, varIndex);
            if (result)
                msg(QObject::tr("parseNvarStorage: parsing of nested NVAR storage failed with error \"%1\"").arg(errorCodeToQString(result)), varIndex);
        }

        // Move to next variable
        offset += variableHeader->Size;
    }
    
    return ERR_SUCCESS;
}
