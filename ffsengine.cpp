/* ffsengine.cpp

Copyright (c) 2015, Nikolaj Schlej. All rights reserved.
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
#include "LZMA/x86Convert.h"

#ifdef _CONSOLE
#include <iostream>
#endif

QString errorMessage(UINT8 errorCode)
{
    switch (errorCode) {
    case ERR_SUCCESS:                         return QObject::tr("Success");
    case ERR_NOT_IMPLEMENTED:                 return QObject::tr("Not implemented");
    case ERR_INVALID_PARAMETER:               return QObject::tr("Function called with invalid parameter");
    case ERR_BUFFER_TOO_SMALL:                return QObject::tr("Buffer too small");
    case ERR_OUT_OF_RESOURCES:                return QObject::tr("Out of resources");
    case ERR_OUT_OF_MEMORY:                   return QObject::tr("Out of memory");
    case ERR_FILE_OPEN:                       return QObject::tr("File can't be opened");
    case ERR_FILE_READ:                       return QObject::tr("File can't be read");
    case ERR_FILE_WRITE:                      return QObject::tr("File can't be written");
    case ERR_ITEM_NOT_FOUND:                  return QObject::tr("Item not found");
    case ERR_UNKNOWN_ITEM_TYPE:               return QObject::tr("Unknown item type");
    case ERR_INVALID_FLASH_DESCRIPTOR:        return QObject::tr("Invalid flash descriptor");
    case ERR_INVALID_REGION:                  return QObject::tr("Invalid region");
    case ERR_EMPTY_REGION:                    return QObject::tr("Empty region");
    case ERR_BIOS_REGION_NOT_FOUND:           return QObject::tr("BIOS region not found");
    case ERR_VOLUMES_NOT_FOUND:               return QObject::tr("UEFI volumes not found");
    case ERR_INVALID_VOLUME:                  return QObject::tr("Invalid UEFI volume");
    case ERR_VOLUME_REVISION_NOT_SUPPORTED:   return QObject::tr("Volume revision not supported");
    case ERR_VOLUME_GROW_FAILED:              return QObject::tr("Volume grow failed");
    case ERR_UNKNOWN_FFS:                     return QObject::tr("Unknown file system");
    case ERR_INVALID_FILE:                    return QObject::tr("Invalid file");
    case ERR_INVALID_SECTION:                 return QObject::tr("Invalid section");
    case ERR_UNKNOWN_SECTION:                 return QObject::tr("Unknown section");
    case ERR_STANDARD_COMPRESSION_FAILED:     return QObject::tr("Standard compression failed");
    case ERR_CUSTOMIZED_COMPRESSION_FAILED:   return QObject::tr("Customized compression failed");
    case ERR_STANDARD_DECOMPRESSION_FAILED:   return QObject::tr("Standard decompression failed");
    case ERR_CUSTOMIZED_DECOMPRESSION_FAILED: return QObject::tr("Customized compression failed");
    case ERR_UNKNOWN_COMPRESSION_ALGORITHM:   return QObject::tr("Unknown compression method");
    case ERR_UNKNOWN_EXTRACT_MODE:            return QObject::tr("Unknown extract mode");
    case ERR_UNKNOWN_INSERT_MODE:             return QObject::tr("Unknown insert mode");
    case ERR_UNKNOWN_IMAGE_TYPE:              return QObject::tr("Unknown executable image type");
    case ERR_UNKNOWN_PE_OPTIONAL_HEADER_TYPE: return QObject::tr("Unknown PE optional header type");
    case ERR_UNKNOWN_RELOCATION_TYPE:         return QObject::tr("Unknown relocation type");
    case ERR_GENERIC_CALL_NOT_SUPPORTED:      return QObject::tr("Generic call not supported");
    case ERR_VOLUME_BASE_NOT_FOUND:           return QObject::tr("Volume base address not found");
    case ERR_PEI_CORE_ENTRY_POINT_NOT_FOUND:  return QObject::tr("PEI core entry point not found");
    case ERR_COMPLEX_BLOCK_MAP:               return QObject::tr("Block map structure too complex for correct analysis");
    case ERR_DIR_ALREADY_EXIST:               return QObject::tr("Directory already exists");
    case ERR_DIR_CREATE:                      return QObject::tr("Directory can't be created");
    case ERR_UNKNOWN_PATCH_TYPE:              return QObject::tr("Unknown patch type");
    case ERR_PATCH_OFFSET_OUT_OF_BOUNDS:      return QObject::tr("Patch offset out of bounds");
    case ERR_INVALID_SYMBOL:                  return QObject::tr("Invalid symbol");
    case ERR_NOTHING_TO_PATCH:                return QObject::tr("Nothing to patch");
    case ERR_DEPEX_PARSE_FAILED:              return QObject::tr("Dependency expression parsing failed");
    case ERR_TRUNCATED_IMAGE:                 return QObject::tr("Image is truncated");
    case ERR_BAD_RELOCATION_ENTRY:            return QObject::tr("Bad image relocation entry");
    default:                                  return QObject::tr("Unknown error %1").arg(errorCode);
    }
}

FfsEngine::FfsEngine(QObject *parent)
    : QObject(parent)
{
    model = new TreeModel();
    oldPeiCoreEntryPoint = 0;
    newPeiCoreEntryPoint = 0;
    dumped = false;
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
#ifndef _DISABLE_ENGINE_MESSAGES
#ifndef _CONSOLE
    messageItems.enqueue(MessageListItem(message, NULL, 0, index));
#else
    (void) index;
    std::cout << message.toLatin1().constData() << std::endl;
#endif
#else
    (void)message;
    (void)index;
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

    // Check buffer size to be more then or equal to size of EFI_CAPSULE_HEADER
    if ((UINT32)buffer.size() <= sizeof(EFI_CAPSULE_HEADER)) {
        msg(tr("parseImageFile: image file is smaller then minimum size of %1h (%2) bytes").hexarg(sizeof(EFI_CAPSULE_HEADER)).arg(sizeof(EFI_CAPSULE_HEADER)));
        return ERR_INVALID_PARAMETER;
    }

    // Check buffer for being normal EFI capsule header
    UINT32 capsuleHeaderSize = 0;
    QModelIndex index;
    if (buffer.startsWith(EFI_CAPSULE_GUID)
        || buffer.startsWith(INTEL_CAPSULE_GUID)) {
        // Get info
        const EFI_CAPSULE_HEADER* capsuleHeader = (const EFI_CAPSULE_HEADER*)buffer.constData();
        capsuleHeaderSize = capsuleHeader->HeaderSize;
        QByteArray header = buffer.left(capsuleHeaderSize);
        QByteArray body = buffer.right(buffer.size() - capsuleHeaderSize);
        QString name = tr("UEFI capsule");
        QString info = tr("Capsule GUID: %1\nFull size: %2h (%3)\nHeader size: %4h (%5)\nImage size: %6h (%7)\nFlags: %8h")
            .arg(guidToQString(capsuleHeader->CapsuleGuid))
            .hexarg(buffer.size()).arg(buffer.size())
            .hexarg(capsuleHeader->HeaderSize).arg(capsuleHeader->HeaderSize)
            .hexarg(capsuleHeader->CapsuleImageSize).arg(capsuleHeader->CapsuleImageSize)
            .hexarg2(capsuleHeader->Flags, 8);

        // Add tree item
        index = model->addItem(Types::Capsule, Subtypes::UefiCapsule, COMPRESSION_ALGORITHM_NONE, name, "", info, header, body);
    }
    // Check buffer for being Toshiba capsule header
    else if (buffer.startsWith(TOSHIBA_CAPSULE_GUID)) {
        // Get info
        const TOSHIBA_CAPSULE_HEADER* capsuleHeader = (const TOSHIBA_CAPSULE_HEADER*)buffer.constData();
        capsuleHeaderSize = capsuleHeader->HeaderSize;
        QByteArray header = buffer.left(capsuleHeaderSize);
        QByteArray body = buffer.right(buffer.size() - capsuleHeaderSize);
        QString name = tr("UEFI capsule");
        QString info = tr("Capsule GUID: %1\nFull size: %2h (%3)\nHeader size: %4h (%5)\nImage size: %6h (%7)\nFlags: %8h")
            .arg(guidToQString(capsuleHeader->CapsuleGuid))
            .hexarg(buffer.size()).arg(buffer.size())
            .hexarg(capsuleHeader->HeaderSize).arg(capsuleHeader->HeaderSize)
            .hexarg(capsuleHeader->FullSize - capsuleHeader->HeaderSize).arg(capsuleHeader->FullSize - capsuleHeader->HeaderSize)
            .hexarg2(capsuleHeader->Flags, 8);

        // Add tree item
        index = model->addItem(Types::Capsule, Subtypes::ToshibaCapsule, COMPRESSION_ALGORITHM_NONE, name, "", info, header, body);
    }
    // Check buffer for being extended Aptio signed capsule header
    else if (buffer.startsWith(APTIO_SIGNED_CAPSULE_GUID) || buffer.startsWith(APTIO_UNSIGNED_CAPSULE_GUID)) {
        // Get info
        bool signedCapsule = buffer.startsWith(APTIO_SIGNED_CAPSULE_GUID);

        const APTIO_CAPSULE_HEADER* capsuleHeader = (const APTIO_CAPSULE_HEADER*)buffer.constData();
        capsuleHeaderSize = capsuleHeader->RomImageOffset;
        QByteArray header = buffer.left(capsuleHeaderSize);
        QByteArray body = buffer.right(buffer.size() - capsuleHeaderSize);
        QString name = tr("AMI Aptio capsule");
        QString info = tr("Capsule GUID: %1\nFull size: %2h (%3)\nHeader size: %4h (%5)\nImage size: %6h (%7)\nFlags: %8h")
            .arg(guidToQString(capsuleHeader->CapsuleHeader.CapsuleGuid))
            .hexarg(buffer.size()).arg(buffer.size())
            .hexarg(capsuleHeaderSize).arg(capsuleHeaderSize)
            .hexarg(capsuleHeader->CapsuleHeader.CapsuleImageSize - capsuleHeaderSize).arg(capsuleHeader->CapsuleHeader.CapsuleImageSize - capsuleHeaderSize)
            .hexarg2(capsuleHeader->CapsuleHeader.Flags, 8);

        //!TODO: more info about Aptio capsule

        // Add tree item
        index = model->addItem(Types::Capsule, signedCapsule ? Subtypes::AptioSignedCapsule : Subtypes::AptioUnsignedCapsule, COMPRESSION_ALGORITHM_NONE, name, "", info, header, body);

        // Show message about possible Aptio signature break
        if (signedCapsule) {
            msg(tr("parseImageFile: Aptio capsule signature may become invalid after image modifications"), index);
        }
    }

    // Skip capsule header to have flash chip image
    QByteArray flashImage = buffer.right(buffer.size() - capsuleHeaderSize);

    // Check for Intel flash descriptor presence
    const FLASH_DESCRIPTOR_HEADER* descriptorHeader = (const FLASH_DESCRIPTOR_HEADER*)flashImage.constData();

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
    QString name = tr("UEFI image");
    QString info = tr("Full size: %1h (%2)")
        .hexarg(flashImage.size()).arg(flashImage.size());

    // Add tree item
    index = model->addItem(Types::Image, Subtypes::UefiImage, COMPRESSION_ALGORITHM_NONE, name, "", info, QByteArray(), flashImage, index);
    return parseBios(flashImage, index);
}

UINT8 FfsEngine::parseIntelImage(const QByteArray & intelImage, QModelIndex & index, const QModelIndex & parent)
{
    // Sanity check
    if (intelImage.isEmpty())
        return EFI_INVALID_PARAMETER;

    // Store the beginning of descriptor as descriptor base address
    const UINT8* descriptor = (const UINT8*)intelImage.constData();
    UINT32 descriptorBegin = 0;
    UINT32 descriptorEnd = FLASH_DESCRIPTOR_SIZE;

    // Check for buffer size to be greater or equal to descriptor region size
    if (intelImage.size() < FLASH_DESCRIPTOR_SIZE) {
        msg(tr("parseIntelImage: input file is smaller than minimum descriptor size of 1000h (4096) bytes"));
        return ERR_INVALID_FLASH_DESCRIPTOR;
    }

    // Parse descriptor map
    const FLASH_DESCRIPTOR_MAP* descriptorMap = (const FLASH_DESCRIPTOR_MAP*)(descriptor + sizeof(FLASH_DESCRIPTOR_HEADER));
    const FLASH_DESCRIPTOR_UPPER_MAP*  upperMap = (const FLASH_DESCRIPTOR_UPPER_MAP*)(descriptor + FLASH_DESCRIPTOR_UPPER_MAP_BASE);

    // Check sanity of base values
    if (descriptorMap->MasterBase > FLASH_DESCRIPTOR_MAX_BASE
        || descriptorMap->MasterBase == descriptorMap->RegionBase
        || descriptorMap->MasterBase == descriptorMap->ComponentBase) {
        msg(tr("parseIntelImage: invalid descriptor master base %1h").hexarg2(descriptorMap->MasterBase, 2));
        return ERR_INVALID_FLASH_DESCRIPTOR;
    }
    if (descriptorMap->RegionBase > FLASH_DESCRIPTOR_MAX_BASE
        || descriptorMap->RegionBase == descriptorMap->ComponentBase) {
        msg(tr("parseIntelImage: invalid descriptor region base %1h").hexarg2(descriptorMap->RegionBase, 2));
        return ERR_INVALID_FLASH_DESCRIPTOR;
    }
    if (descriptorMap->ComponentBase > FLASH_DESCRIPTOR_MAX_BASE) {
        msg(tr("parseIntelImage: invalid descriptor component base %1h").hexarg2(descriptorMap->ComponentBase, 2));
        return ERR_INVALID_FLASH_DESCRIPTOR;
    }

    const FLASH_DESCRIPTOR_REGION_SECTION* regionSection = (const FLASH_DESCRIPTOR_REGION_SECTION*)calculateAddress8(descriptor, descriptorMap->RegionBase);
    const FLASH_DESCRIPTOR_COMPONENT_SECTION* componentSection = (const FLASH_DESCRIPTOR_COMPONENT_SECTION*)calculateAddress8(descriptor, descriptorMap->ComponentBase);

    // Check for legacy descriptor version by getting hardcoded value of FlashParameters.ReadClockFrequency
    UINT8 descriptorVersion = 2; // Skylake+ descriptor
    if (componentSection->FlashParameters.ReadClockFrequency == FLASH_FREQUENCY_20MHZ)  // Legacy descriptor
        descriptorVersion = 1;

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
            bios = intelImage.mid(biosBegin, biosEnd);
            // biosEnd will point to the end of the image file
            // it may be wrong, but it's pretty hard to detect a padding after BIOS region
            // with malformed descriptor
        }
        // Normal descriptor map
        else {
            bios = intelImage.mid(biosBegin, biosEnd);
            // Calculate biosEnd
            biosEnd += biosBegin;
        }
    }
    else {
        msg(tr("parseIntelImage: descriptor parsing failed, BIOS region not found in descriptor"));
        return ERR_INVALID_FLASH_DESCRIPTOR;
    }
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
    // EC region
    QByteArray ec;
    UINT32 ecBegin = 0;
    UINT32 ecEnd = 0;
    if (descriptorVersion == 2) {
        if (regionSection->EcLimit) {
            ecBegin = calculateRegionOffset(regionSection->EcBase);
            ecEnd = calculateRegionSize(regionSection->EcBase, regionSection->EcLimit);
            ec = intelImage.mid(ecBegin, ecEnd);
            ecEnd += ecBegin;
        }
    }

    // Check for intersections between regions
    // Descriptor
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
    if (descriptorVersion == 2 && hasIntersection(descriptorBegin, descriptorEnd, ecBegin, ecEnd)) {
        msg(tr("parseIntelImage: descriptor parsing failed, descriptor region has intersection with EC region"));
        return ERR_INVALID_FLASH_DESCRIPTOR;
    }
    // GbE
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
    if (descriptorVersion == 2 && hasIntersection(gbeBegin, gbeEnd, ecBegin, ecEnd)) {
        msg(tr("parseIntelImage: descriptor parsing failed, GbE region has intersection with EC region"));
        return ERR_INVALID_FLASH_DESCRIPTOR;
    }
    // ME
    if (hasIntersection(meBegin, meEnd, biosBegin, biosEnd)) {
        msg(tr("parseIntelImage: descriptor parsing failed, ME region has intersection with BIOS region"));
        return ERR_INVALID_FLASH_DESCRIPTOR;
    }
    if (hasIntersection(meBegin, meEnd, pdrBegin, pdrEnd)) {
        msg(tr("parseIntelImage: descriptor parsing failed, ME region has intersection with PDR region"));
        return ERR_INVALID_FLASH_DESCRIPTOR;
    }
    if (descriptorVersion == 2 && hasIntersection(meBegin, meEnd, ecBegin, ecEnd)) {
        msg(tr("parseIntelImage: descriptor parsing failed, ME region has intersection with EC region"));
        return ERR_INVALID_FLASH_DESCRIPTOR;
    }
    // BIOS
    if (hasIntersection(biosBegin, biosEnd, pdrBegin, pdrEnd)) {
        msg(tr("parseIntelImage: descriptor parsing failed, BIOS region has intersection with PDR region"));
        return ERR_INVALID_FLASH_DESCRIPTOR;
    }
    if (descriptorVersion == 2 && hasIntersection(biosBegin, biosEnd, ecBegin, ecEnd)) {
        msg(tr("parseIntelImage: descriptor parsing failed, BIOS region has intersection with EC region"));
        return ERR_INVALID_FLASH_DESCRIPTOR;
    }
    // PDR
    if (descriptorVersion == 2 && hasIntersection(pdrBegin, pdrEnd, ecBegin, ecEnd)) {
        msg(tr("parseIntelImage: descriptor parsing failed, PDR region has intersection with EC region"));
        return ERR_INVALID_FLASH_DESCRIPTOR;
    }

    // Region map is consistent

    // Intel image
    QString name = tr("Intel image");
    QString info = tr("Full size: %1h (%2)\nFlash chips: %3\nMasters: %4\nPCH straps: %5\nCPU straps: %6\n")
        .hexarg(intelImage.size()).arg(intelImage.size())
        .arg(descriptorMap->NumberOfFlashChips + 1)
        .arg(descriptorMap->NumberOfMasters + 1)
        .arg(descriptorMap->NumberOfPchStraps)
        .arg(descriptorMap->NumberOfProcStraps);

    // Add Intel image tree item
    index = model->addItem(Types::Image, Subtypes::IntelImage, COMPRESSION_ALGORITHM_NONE, name, "", info, QByteArray(), intelImage, parent);

    // Descriptor
    // Get descriptor info
    QByteArray body = intelImage.left(FLASH_DESCRIPTOR_SIZE);
    name = tr("Descriptor region");
    info = tr("Full size: %1h (%2)").hexarg(FLASH_DESCRIPTOR_SIZE).arg(FLASH_DESCRIPTOR_SIZE);

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
    if (descriptorVersion == 2 && regionSection->EcLimit) {
        offsets.append(ecBegin);
        info += tr("\nEC region offset:  %1h").hexarg(ecBegin);
    }

    // Region access settings
    if (descriptorVersion == 1) {
        const FLASH_DESCRIPTOR_MASTER_SECTION* masterSection = (const FLASH_DESCRIPTOR_MASTER_SECTION*)calculateAddress8(descriptor, descriptorMap->MasterBase);
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
    }
    else if (descriptorVersion == 2) {
        const FLASH_DESCRIPTOR_MASTER_SECTION_V2* masterSection = (const FLASH_DESCRIPTOR_MASTER_SECTION_V2*)calculateAddress8(descriptor, descriptorMap->MasterBase);
        info += tr("\nRegion access settings:");
        info += tr("\nBIOS: %1h %2h ME: %3h %4h\nGbE:  %5h %6h EC: %7h %8h")
            .hexarg2(masterSection->BiosRead, 3)
            .hexarg2(masterSection->BiosWrite, 3)
            .hexarg2(masterSection->MeRead, 3)
            .hexarg2(masterSection->MeWrite, 3)
            .hexarg2(masterSection->GbeRead, 3)
            .hexarg2(masterSection->GbeWrite, 3)
            .hexarg2(masterSection->EcRead, 3)
            .hexarg2(masterSection->EcWrite, 3);

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
        info += tr("\nEC    %1  %2")
            .arg(masterSection->BiosRead  & FLASH_DESCRIPTOR_REGION_ACCESS_EC ? "Yes " : "No  ")
            .arg(masterSection->BiosWrite & FLASH_DESCRIPTOR_REGION_ACCESS_EC ? "Yes " : "No  ");

        // Prepend descriptor version if present
        if (descriptorMap->DescriptorVersion != FLASH_DESCRIPTOR_VERSION_INVALID) {
            const FLASH_DESCRIPTOR_VERSION* version = (const FLASH_DESCRIPTOR_VERSION*)&descriptorMap->DescriptorVersion;
            QString versionStr = tr("Flash descriptor version: %1.%2").arg(version->Major).arg(version->Minor);
            if (version->Major != FLASH_DESCRIPTOR_VERSION_MAJOR || version->Minor != FLASH_DESCRIPTOR_VERSION_MINOR) {
                versionStr += tr(", unknown");
                msg(tr("parseIntelImage: unknown flash descriptor version %1.%2").arg(version->Major).arg(version->Minor));
            }
            info = versionStr + "\n" + info;
        }
    }

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
    model->addItem(Types::Region, Subtypes::DescriptorRegion, COMPRESSION_ALGORITHM_NONE, name, "", info, QByteArray(), body, index);

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
        // Parse EC region
        else if (descriptorVersion == 2 && offsets.at(i) == ecBegin) {
            QModelIndex ecIndex;
            result = parseEcRegion(ec, ecIndex, index);
        }
        if (result)
            return result;
    }

    // Add the data after the last region as padding
    UINT32 IntelDataEnd = 0;
    UINT32 LastRegionOffset = offsets.last();
    if (LastRegionOffset == gbeBegin)
        IntelDataEnd = gbeEnd;
    else if (LastRegionOffset == meBegin)
        IntelDataEnd = meEnd;
    else if (LastRegionOffset == biosBegin)
        IntelDataEnd = biosEnd;
    else if (LastRegionOffset == pdrBegin)
        IntelDataEnd = pdrEnd;
    else if (descriptorVersion == 2 && LastRegionOffset == ecBegin)
        IntelDataEnd = ecEnd;

    if (IntelDataEnd > (UINT32)intelImage.size()) { // Image file is truncated
        msg(tr("parseIntelImage: image size %1 (%2) is smaller than the end of last region %3 (%4), may be damaged")
            .hexarg(intelImage.size()).arg(intelImage.size())
            .hexarg(IntelDataEnd).arg(IntelDataEnd), index);
        return ERR_TRUNCATED_IMAGE;
    }
    else if (IntelDataEnd < (UINT32)intelImage.size()) { // Insert padding
        QByteArray padding = intelImage.mid(IntelDataEnd);
        // Get info
        name = tr("Padding");
        info = tr("Full size: %1h (%2)")
            .hexarg(padding.size()).arg(padding.size());
        // Add tree item
        model->addItem(Types::Padding, getPaddingType(padding), COMPRESSION_ALGORITHM_NONE, name, "", info, QByteArray(), padding, index);
    }

    return ERR_SUCCESS;
}

UINT8 FfsEngine::parseGbeRegion(const QByteArray & gbe, QModelIndex & index, const QModelIndex & parent, const UINT8 mode)
{
    // Check sanity
    if (gbe.isEmpty())
        return ERR_EMPTY_REGION;

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

    // Add tree item
    index = model->addItem(Types::Region, Subtypes::GbeRegion, COMPRESSION_ALGORITHM_NONE, name, "", info, QByteArray(), gbe, parent, mode);

    return ERR_SUCCESS;
}

UINT8 FfsEngine::parseMeRegion(const QByteArray & me, QModelIndex & index, const QModelIndex & parent, const UINT8 mode)
{
    // Check sanity
    if (me.isEmpty())
        return ERR_EMPTY_REGION;

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

    // Add tree item
    index = model->addItem(Types::Region, Subtypes::MeRegion, COMPRESSION_ALGORITHM_NONE, name, "", info, QByteArray(), me, parent, mode);

    // Show messages
    if (emptyRegion) {
        msg(tr("parseRegion: ME region is empty"), index);
    }
    else if (!versionFound) {
        msg(tr("parseRegion: ME region version is unknown, it can be damaged"), index);
    }

    return ERR_SUCCESS;
}

UINT8 FfsEngine::parsePdrRegion(const QByteArray & pdr, QModelIndex & index, const QModelIndex & parent, const UINT8 mode)
{
    // Check sanity
    if (pdr.isEmpty())
        return ERR_EMPTY_REGION;

    // Get info
    QString name = tr("PDR region");
    QString info = tr("Full size: %1h (%2)").
        hexarg(pdr.size()).arg(pdr.size());

    // Add tree item
    index = model->addItem(Types::Region, Subtypes::PdrRegion, COMPRESSION_ALGORITHM_NONE, name, "", info, QByteArray(), pdr, parent, mode);

    // Parse PDR region as BIOS space
    UINT8 result = parseBios(pdr, index);
    if (result && result != ERR_VOLUMES_NOT_FOUND && result != ERR_INVALID_VOLUME)
        return result;

    return ERR_SUCCESS;
}

UINT8 FfsEngine::parseEcRegion(const QByteArray & ec, QModelIndex & index, const QModelIndex & parent, const UINT8 mode)
{
    // Check sanity
    if (ec.isEmpty())
        return ERR_EMPTY_REGION;

    // Get info
    QString name = tr("EC region");
    QString info = tr("Full size: %1h (%2)").
        hexarg(ec.size()).arg(ec.size());

    // Add tree item
    index = model->addItem(Types::Region, Subtypes::EcRegion, COMPRESSION_ALGORITHM_NONE, name, "", info, QByteArray(), ec, parent, mode);

    return ERR_SUCCESS;
}

UINT8 FfsEngine::parseBiosRegion(const QByteArray & bios, QModelIndex & index, const QModelIndex & parent, const UINT8 mode)
{
    if (bios.isEmpty())
        return ERR_EMPTY_REGION;

    // Get info
    QString name = tr("BIOS region");
    QString info = tr("Full size: %1h (%2)").
        hexarg(bios.size()).arg(bios.size());

    // Add tree item
    index = model->addItem(Types::Region, Subtypes::BiosRegion, COMPRESSION_ALGORITHM_NONE, name, "", info, QByteArray(), bios, parent, mode);

    return parseBios(bios, index);
}

UINT32 FfsEngine::getPaddingType(const QByteArray & padding)
{
    if (padding.count('\x00') == padding.count())
        return Subtypes::ZeroPadding;
    if (padding.count('\xFF') == padding.count())
        return Subtypes::OnePadding;
    return Subtypes::DataPadding;
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
        info = tr("Full size: %1h (%2)")
            .hexarg(padding.size()).arg(padding.size());

        // Add tree item
        model->addItem(Types::Padding, getPaddingType(padding), COMPRESSION_ALGORITHM_NONE, name, "", info, QByteArray(), padding, parent);
    }

    // Search for and parse all volumes
    UINT32 volumeOffset = prevVolumeOffset;
    UINT32 prevVolumeSize = 0;
    UINT32 volumeSize = 0;
    UINT32 bmVolumeSize = 0;

    while (true)
    {
        bool msgAlignmentBitsSet = false;
        bool msgUnaligned = false;
        bool msgUnknownRevision = false;
        bool msgSizeMismach = false;

        // Padding between volumes
        if (volumeOffset > prevVolumeOffset + prevVolumeSize) {
            UINT32 paddingSize = volumeOffset - prevVolumeOffset - prevVolumeSize;
            QByteArray padding = bios.mid(prevVolumeOffset + prevVolumeSize, paddingSize);
            // Get info
            name = tr("Padding");
            info = tr("Full size: %1h (%2)")
                .hexarg(padding.size()).arg(padding.size());
            // Add tree item
            model->addItem(Types::Padding, getPaddingType(padding), COMPRESSION_ALGORITHM_NONE, name, "", info, QByteArray(), padding, parent);
        }

        // Get volume size
        result = getVolumeSize(bios, volumeOffset, volumeSize, bmVolumeSize);
        if (result) {
            msg(tr("parseBios: getVolumeSize failed with error \"%1\"").arg(errorMessage(result)), parent);
            return result;
        }

        // Check that volume is fully present in input
        if (volumeSize > (UINT32)bios.size() || volumeOffset + volumeSize > (UINT32)bios.size()) {
            msg(tr("parseBios: one of volumes inside overlaps the end of data"), parent);
            return ERR_INVALID_VOLUME;
        }

        // Check reported size against a size calculated using block map
        if (volumeSize != bmVolumeSize)
            msgSizeMismach = true;

        // Check volume revision and alignment
        const EFI_FIRMWARE_VOLUME_HEADER* volumeHeader = (const EFI_FIRMWARE_VOLUME_HEADER*)(bios.constData() + volumeOffset);
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
            alignment = (UINT32)(1UL << ((volumeHeader->Attributes & EFI_FVB2_ALIGNMENT) >> 16));

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
            msg(tr("parseBios: volume parsing failed with error \"%1\"").arg(errorMessage(result)), parent);

        // Show messages
        if (msgAlignmentBitsSet)
            msg("parseBios: alignment bits set on volume without alignment capability", index);
        if (msgUnaligned)
            msg(tr("parseBios: unaligned revision 2 volume"), index);
        if (msgUnknownRevision)
            msg(tr("parseBios: unknown volume revision %1").arg(volumeHeader->Revision), index);
        if (msgSizeMismach)
            msg(tr("parseBios: volume size stored in header %1h differs from calculated using block map %2h")
            .hexarg(volumeSize).arg(bmVolumeSize),
            index);

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
                info = tr("Full size: %1h (%2)")
                    .hexarg(padding.size()).arg(padding.size());
                // Add tree item
                model->addItem(Types::Padding, getPaddingType(padding), COMPRESSION_ALGORITHM_NONE, name, "", info, QByteArray(), padding, parent);
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

UINT8 FfsEngine::getVolumeSize(const QByteArray & bios, UINT32 volumeOffset, UINT32 & volumeSize, UINT32 & bmVolumeSize)
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

UINT8  FfsEngine::parseVolume(const QByteArray & volume, QModelIndex & index, const QModelIndex & parent, const UINT8 mode)
{
    // Check that there is space for the volume header
    if ((UINT32)volume.size() < sizeof(EFI_FIRMWARE_VOLUME_HEADER)) {
        msg(tr("parseVolume: input volume size %1h (%2) is smaller than volume header size 40h (64)").hexarg(volume.size()).arg(volume.size()));
        return ERR_INVALID_VOLUME;
    }

    // Populate volume header
    const EFI_FIRMWARE_VOLUME_HEADER* volumeHeader = (const EFI_FIRMWARE_VOLUME_HEADER*)(volume.constData());

    // Check sanity of HeaderLength value
    if (ALIGN8(volumeHeader->HeaderLength) > volume.size()) {
        msg(tr("parseVolume: volume header overlaps the end of data"));
        return ERR_INVALID_VOLUME;
    }

    // Check sanity of ExtHeaderOffset value
    if (volumeHeader->ExtHeaderOffset > 0
        && (UINT32)volume.size() < ALIGN8(volumeHeader->ExtHeaderOffset + sizeof(EFI_FIRMWARE_VOLUME_EXT_HEADER))) {
        msg(tr("parseVolume: extended volume header overlaps the end of data"));
        return ERR_INVALID_VOLUME;
    }

    // Calculate volume header size
    UINT32 headerSize;
    if (volumeHeader->Revision > 1 && volumeHeader->ExtHeaderOffset) {
        const EFI_FIRMWARE_VOLUME_EXT_HEADER* extendedHeader = (const EFI_FIRMWARE_VOLUME_EXT_HEADER*)(volume.constData() + volumeHeader->ExtHeaderOffset);
        headerSize = volumeHeader->ExtHeaderOffset + extendedHeader->ExtHeaderSize;
    }
    else
        headerSize = volumeHeader->HeaderLength;

    // Sanity check after some crazy MSI images
    headerSize = ALIGN8(headerSize);

    // Check for FFS v2/v3 volume
	UINT8 subtype = Subtypes::UnknownVolume;
    if (FFSv2Volumes.contains(QByteArray::fromRawData((const char*)volumeHeader->FileSystemGuid.Data, sizeof(EFI_GUID)))){
		subtype = Subtypes::Ffs2Volume;
	}
	else if (FFSv3Volumes.contains(QByteArray::fromRawData((const char*)volumeHeader->FileSystemGuid.Data, sizeof(EFI_GUID)))) {
        subtype = Subtypes::Ffs3Volume;
    }

    // Check attributes
    // Determine value of empty byte
    char empty = volumeHeader->Attributes & EFI_FVB_ERASE_POLARITY ? '\xFF' : '\x00';

    // Get volume size
    UINT32 volumeSize;
    UINT32 bmVolumeSize;

    UINT8 result = getVolumeSize(volume, 0, volumeSize, bmVolumeSize);
    if (result)
        return result;

    // Check for Apple CRC32 in ZeroVector
    bool volumeHasZVCRC = false;
    bool volumeHasZVFSO = false;
    UINT32 crc32FromZeroVector = *(UINT32*)(volume.constData() + 8);
    UINT32 freeSpaceOffsetFromZeroVector = *(UINT32*)(volume.constData() + 12);
    if (crc32FromZeroVector != 0) {
        // Calculate CRC32 of the volume body
        UINT32 crc = crc32(0, (const UINT8*)(volume.constData() + volumeHeader->HeaderLength), volumeSize - volumeHeader->HeaderLength);
        if (crc == crc32FromZeroVector) {
            volumeHasZVCRC = true;
        }

        // Check for free space size in zero vector
        if (freeSpaceOffsetFromZeroVector != 0) {
            volumeHasZVFSO = true;
        }
    }

    // Check header checksum by recalculating it
    bool msgInvalidChecksum = false;
    if (calculateChecksum16((const UINT16*)volumeHeader, volumeHeader->HeaderLength))
        msgInvalidChecksum = true;

    // Get info
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
        .arg(empty ? "1" : "0");

    // Extended header present
    if (volumeHeader->Revision > 1 && volumeHeader->ExtHeaderOffset) {
        const EFI_FIRMWARE_VOLUME_EXT_HEADER* extendedHeader = (const EFI_FIRMWARE_VOLUME_EXT_HEADER*)(volume.constData() + volumeHeader->ExtHeaderOffset);
        info += tr("\nExtended header size: %1h (%2)\nVolume GUID: %3")
            .hexarg(extendedHeader->ExtHeaderSize).arg(extendedHeader->ExtHeaderSize)
            .arg(guidToQString(extendedHeader->FvName));
    }

    // Add text
    QString text;
    if (volumeHasZVCRC)
        text += tr("AppleCRC32 ");
    if (volumeHasZVFSO)
        text += tr("AppleFSO ");

    // Add tree item
    QByteArray  header = volume.left(headerSize);
    QByteArray  body = volume.mid(headerSize, volumeSize - headerSize);
    index = model->addItem(Types::Volume, subtype, COMPRESSION_ALGORITHM_NONE, name, text, info, header, body, parent, mode);

    // Show messages
    if (subtype == Subtypes::UnknownVolume) {
        msg(tr("parseVolume: unknown file system %1").arg(guidToQString(volumeHeader->FileSystemGuid)), index);
        // Do not parse unknown volumes
        return ERR_SUCCESS;
    }
    if (msgInvalidChecksum) {
        msg(tr("parseVolume: volume header checksum is invalid"), index);
    }

    // Search for and parse all files
    UINT32 fileOffset = headerSize;
    UINT32 fileSize;
    QQueue<QByteArray> files;

    while (fileOffset < volumeSize) {
        bool msgUnalignedFile = false;
        bool msgDuplicateGuid = false;

        // Check if it's possibly the latest file in the volume
        if (volumeSize - fileOffset < sizeof(EFI_FFS_FILE_HEADER)) {
            // No files are possible after this point
            // All the rest is either free space or non-UEFI data
            QByteArray rest = volume.right(volumeSize - fileOffset);
            if (rest.count(empty) == rest.size()) { // It's a free space
                model->addItem(Types::FreeSpace, 0, COMPRESSION_ALGORITHM_NONE, tr("Volume free space"), "", tr("Full size: %1h (%2)").hexarg(rest.size()).arg(rest.size()), QByteArray(), rest, index);
            }
            else { //It's non-UEFI data
                QModelIndex dataIndex = model->addItem(Types::Padding, Subtypes::DataPadding, COMPRESSION_ALGORITHM_NONE, tr("Non-UEFI data"), "", tr("Full size: %1h (%2)").hexarg(rest.size()).arg(rest.size()), QByteArray(), rest, index);
                msg(tr("parseVolume: non-UEFI data found in volume's free space"), dataIndex);
            }
            // Exit from loop
            break;
        }

        QByteArray tempFile = volume.mid(fileOffset, sizeof(EFI_FFS_FILE_HEADER));
        const EFI_FFS_FILE_HEADER* tempFileHeader = (const EFI_FFS_FILE_HEADER*)tempFile.constData();
        UINT32 fileHeaderSize = sizeof(EFI_FFS_FILE_HEADER);
        fileSize = uint24ToUint32(tempFileHeader->Size);
        if (volumeHeader->Revision > 1 && (tempFileHeader->Attributes & FFS_ATTRIB_LARGE_FILE)) {
            // Check if it's possibly the latest file in the volume
            if (volumeSize - fileOffset < sizeof(EFI_FFS_FILE_HEADER2)) {
                // No files are possible after this point
                // All the rest is either free space or non-UEFI data
                QByteArray rest = volume.right(volumeSize - fileOffset);
                if (rest.count(empty) == rest.size()) { // It's a free space
                    model->addItem(Types::FreeSpace, 0, COMPRESSION_ALGORITHM_NONE, tr("Volume free space"), "", tr("Full size: %1h (%2)").hexarg(rest.size()).arg(rest.size()), QByteArray(), rest, index);
                }
                else { //It's non-UEFI data
                    QModelIndex dataIndex = model->addItem(Types::Padding, Subtypes::DataPadding, COMPRESSION_ALGORITHM_NONE, tr("Non-UEFI data"), "", tr("Full size: %1h (%2)").hexarg(rest.size()).arg(rest.size()), QByteArray(), rest, index);
                    msg(tr("parseVolume: non-UEFI data found in volume's free space"), dataIndex);
                }
                // Exit from loop
                break;
            }

            fileHeaderSize = sizeof(EFI_FFS_FILE_HEADER2);
            tempFile = volume.mid(fileOffset, sizeof(EFI_FFS_FILE_HEADER2));
            const EFI_FFS_FILE_HEADER2* tempFileHeader2 = (const EFI_FFS_FILE_HEADER2*)tempFile.constData();
            fileSize = (UINT32)tempFileHeader2->ExtendedSize;
        }

        // Check file size to be at least size of the header
        if (fileSize < fileHeaderSize) {
            msg(tr("parseVolume: volume has FFS file with invalid size"), index);
            return ERR_INVALID_FILE;
        }
       
        QByteArray file = volume.mid(fileOffset, fileSize);
        QByteArray header = file.left(fileHeaderSize);
        
        // If we are at empty space in the end of volume
        if (header.count(empty) == header.size()) {
            // Check free space to be actually free
            QByteArray freeSpace = volume.mid(fileOffset);
            if (freeSpace.count(empty) != freeSpace.count()) {
                // Search for the first non-empty byte
                UINT32 i;
                UINT32 size = freeSpace.size();
                const CHAR8* current = freeSpace.constData();
                for (i = 0; i < size; i++) {
                    if (*current++ != empty)
                        break;
                }

                // Align found index to file alignment
                // It must be possible because minimum 16 bytes of empty were found before
                if (i != ALIGN8(i))
                    i = ALIGN8(i) - 8;

                // Add all bytes before as free space...
                if (i > 0) {
                    QByteArray free = freeSpace.left(i);
                    model->addItem(Types::FreeSpace, 0, COMPRESSION_ALGORITHM_NONE, tr("Volume free space"), "", tr("Full size: %1h (%2)").hexarg(free.size()).arg(free.size()), QByteArray(), free, index);
                }
                // ... and all bytes after as a padding
                QByteArray padding = freeSpace.mid(i);
                QModelIndex dataIndex = model->addItem(Types::Padding, Subtypes::DataPadding, COMPRESSION_ALGORITHM_NONE, tr("Non-UEFI data"), "", tr("Full size: %1h (%2)").hexarg(padding.size()).arg(padding.size()), QByteArray(), padding, index);
                msg(tr("parseVolume: non-UEFI data found in volume's free space"), dataIndex);
            }
            else {
                // Add free space element
                model->addItem(Types::FreeSpace, 0, COMPRESSION_ALGORITHM_NONE, tr("Volume free space"), "", tr("Full size: %1h (%2)").hexarg(freeSpace.size()).arg(freeSpace.size()), QByteArray(), freeSpace, index);
            }
            break; // Exit from loop
        }

        // Check file alignment
        const EFI_FFS_FILE_HEADER* fileHeader = (const EFI_FFS_FILE_HEADER*)header.constData();
        UINT8 alignmentPower = ffsAlignmentTable[(fileHeader->Attributes & FFS_ATTRIB_DATA_ALIGNMENT) >> 3];
        if (volumeHeader->Revision > 1 && (fileHeader->Attributes & FFS_ATTRIB_DATA_ALIGNMENT2))
            alignmentPower = ffsAlignment2Table[(fileHeader->Attributes & FFS_ATTRIB_DATA_ALIGNMENT) >> 3];

        UINT32 alignment = (UINT32)(1UL << alignmentPower);
        if ((fileOffset + fileHeaderSize) % alignment)
            msgUnalignedFile = true;

        // Check file GUID
        if (fileHeader->Type != EFI_FV_FILETYPE_PAD && files.indexOf(header.left(sizeof(EFI_GUID))) != -1)
            msgDuplicateGuid = true;

        // Add file GUID to queue
        files.enqueue(header.left(sizeof(EFI_GUID)));

        // Parse file
        QModelIndex fileIndex;
        result = parseFile(file, fileIndex, volumeHeader->Revision, empty == '\xFF' ? ERASE_POLARITY_TRUE : ERASE_POLARITY_FALSE, index);
        if (result && result != ERR_VOLUMES_NOT_FOUND && result != ERR_INVALID_VOLUME)
            msg(tr("parseVolume: FFS file parsing failed with error \"%1\"").arg(errorMessage(result)), index);

        // Show messages
        if (msgUnalignedFile)
            msg(tr("parseVolume: unaligned file %1").arg(guidToQString(fileHeader->Name)), fileIndex);
        if (msgDuplicateGuid)
            msg(tr("parseVolume: file with duplicate GUID %1").arg(guidToQString(fileHeader->Name)), fileIndex);

        // Move to next file
        fileOffset += fileSize;
        fileOffset = ALIGN8(fileOffset);
    }

    return ERR_SUCCESS;
}

UINT8 FfsEngine::parseFile(const QByteArray & file, QModelIndex & index, const UINT8 revision, const UINT8 erasePolarity, const QModelIndex & parent, const UINT8 mode)
{
    bool msgInvalidHeaderChecksum = false;
    bool msgInvalidDataChecksum = false;
    bool msgInvalidTailValue = false;
    bool msgInvalidType = false;

    // Populate file header
    if ((UINT32)file.size() < sizeof(EFI_FFS_FILE_HEADER))
        return ERR_INVALID_FILE;
    const EFI_FFS_FILE_HEADER* fileHeader = (const EFI_FFS_FILE_HEADER*)file.constData();

    // Construct empty byte for this file
    char empty = erasePolarity ? '\xFF' : '\x00';

    // Get file header
    QByteArray header = file.left(sizeof(EFI_FFS_FILE_HEADER));
    if (revision > 1 && (fileHeader->Attributes & FFS_ATTRIB_LARGE_FILE)) {
        if ((UINT32)file.size() < sizeof(EFI_FFS_FILE_HEADER2))
            return ERR_INVALID_FILE;
        header = file.left(sizeof(EFI_FFS_FILE_HEADER2));
    }

    // Check header checksum
    UINT8 calculatedHeader = 0x100 - (calculateSum8((const UINT8*)header.constData(), header.size()) - fileHeader->IntegrityCheck.Checksum.Header - fileHeader->IntegrityCheck.Checksum.File - fileHeader->State);
    if (fileHeader->IntegrityCheck.Checksum.Header != calculatedHeader)
        msgInvalidHeaderChecksum = true;

    // Get file body
    QByteArray body = file.mid(header.size());

    // Check for file tail presence
    QByteArray tail;
    if (revision == 1 && fileHeader->Attributes & FFS_ATTRIB_TAIL_PRESENT) {
        //Check file tail;
        tail = body.right(sizeof(UINT16));
        UINT16 tailValue = *(UINT16*)tail.constData();
        if (fileHeader->IntegrityCheck.TailReference != (UINT16)~tailValue)
            msgInvalidTailValue = true;

        // Remove tail from file body
        body = body.left(body.size() - sizeof(UINT16));
    }

    // Check data checksum
    // Data checksum must be calculated
    UINT8 calculatedData = 0;
    if (fileHeader->Attributes & FFS_ATTRIB_CHECKSUM) {
        calculatedData = calculateChecksum8((const UINT8*)body.constData(), body.size());
    }
    // Data checksum must be one of predefined values
    else if (revision == 1) {
        calculatedData = FFS_FIXED_CHECKSUM;
    }
    else {
        calculatedData = FFS_FIXED_CHECKSUM2;
    }

    if (fileHeader->IntegrityCheck.Checksum.File != calculatedData)
        msgInvalidDataChecksum = true;

    // Parse current file by default
    bool parseCurrentFile = false;
    bool parseAsBios = false;

    // Check file type
    switch (fileHeader->Type) {
    case EFI_FV_FILETYPE_ALL:
    case EFI_FV_FILETYPE_RAW:
        parseAsBios = true;
    case EFI_FV_FILETYPE_FREEFORM:
    case EFI_FV_FILETYPE_SECURITY_CORE:
    case EFI_FV_FILETYPE_PEI_CORE:
    case EFI_FV_FILETYPE_DXE_CORE:
    case EFI_FV_FILETYPE_PEIM:
    case EFI_FV_FILETYPE_DRIVER:
    case EFI_FV_FILETYPE_COMBINED_PEIM_DRIVER:
    case EFI_FV_FILETYPE_APPLICATION:
    case EFI_FV_FILETYPE_SMM:
    case EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE:
    case EFI_FV_FILETYPE_COMBINED_SMM_DXE:
    case EFI_FV_FILETYPE_SMM_CORE:
	case EFI_FV_FILETYPE_SMM_STANDALONE:
    case EFI_FV_FILETYPE_SMM_CORE_STANDALONE:
    case EFI_FV_FILETYPE_PAD:
        parseCurrentFile = true;
        break;
    default:
        msgInvalidType = true;
    };

    // Check for empty file
    bool parseAsNonEmptyPadFile = false;
    if (body.count(empty) == body.size()) {
        // No need to parse empty files
        parseCurrentFile = false;
    }
    // Check for non-empty pad file
    else if (fileHeader->Type == EFI_FV_FILETYPE_PAD) {
        parseAsNonEmptyPadFile = true;
    }

    // Get info
    QString name;
    QString info;
    if (fileHeader->Type != EFI_FV_FILETYPE_PAD)
        name = guidToQString(fileHeader->Name);
    else
        name = parseAsNonEmptyPadFile ? tr("Non-empty pad-file") : tr("Pad-file");

    info = tr("File GUID: %1\nType: %2h\nAttributes: %3h\nFull size: %4h (%5)\nHeader size: %6h (%7)\nBody size: %8h (%9)\nState: %10h\nHeader checksum: %11h\nData checksum: %12h")
        .arg(guidToQString(fileHeader->Name))
        .hexarg2(fileHeader->Type, 2)
        .hexarg2(fileHeader->Attributes, 2)
        .hexarg(header.size() + body.size() + tail.size()).arg(header.size() + body.size() + tail.size())
        .hexarg(header.size()).arg(header.size())
        .hexarg(body.size()).arg(body.size())
        .hexarg2(fileHeader->State, 2)
        .hexarg2(fileHeader->IntegrityCheck.Checksum.Header, 2)
        .hexarg2(fileHeader->IntegrityCheck.Checksum.File, 2);

    // Add tree item
    index = model->addItem(Types::File, fileHeader->Type, COMPRESSION_ALGORITHM_NONE, name, "", info, header, body, parent, mode);

    // Show messages
    if (msgInvalidHeaderChecksum)
        msg(tr("parseFile: invalid header checksum %1h, should be %2h").hexarg2(fileHeader->IntegrityCheck.Checksum.Header, 2).hexarg2(calculatedHeader, 2), index);
    if (msgInvalidDataChecksum)
        msg(tr("parseFile: invalid data checksum %1h, should be %2h").hexarg2(fileHeader->IntegrityCheck.Checksum.File, 2).hexarg2(calculatedData, 2), index);
    if (msgInvalidTailValue)
        msg(tr("parseFile: invalid tail value %1h").hexarg(*(UINT16*)tail.data()), index);
    if (msgInvalidType)
        msg(tr("parseFile: unknown file type %1h").arg(fileHeader->Type, 2), index);

    // No parsing needed
    if (!parseCurrentFile)
        return ERR_SUCCESS;

    // Parse non-empty pad file
    if (parseAsNonEmptyPadFile) {
        // Search for the first non-empty byte
        UINT32 i;
        UINT32 size = body.size();
        const CHAR8* current = body.constData();
        for (i = 0; i < size; i++) {
            if (*current++ != empty)
                break;
        }
        // Add all bytes before as free space...
        if (i > 0) {
            QByteArray free = body.left(i);
            model->addItem(Types::FreeSpace, 0, COMPRESSION_ALGORITHM_NONE, tr("Free space"), "", tr("Full size: %1h (%2)").hexarg(free.size()).arg(free.size()), QByteArray(), free, index, mode);
        }
        // ... and all bytes after as a padding
        QByteArray padding = body.mid(i);
        QModelIndex dataIndex = model->addItem(Types::Padding, Subtypes::DataPadding, COMPRESSION_ALGORITHM_NONE, tr("Non-UEFI data"), "", tr("Full size: %1h (%2)").hexarg(padding.size()).arg(padding.size()), QByteArray(), padding, index, mode);

        // Show message
        msg(tr("parseFile: non-empty pad-file contents will be destroyed after volume modifications"), dataIndex);

        return ERR_SUCCESS;
    }

    // Parse file as BIOS space
    UINT8 result;
    if (parseAsBios) {
        result = parseBios(body, index);
        if (result && result != ERR_VOLUMES_NOT_FOUND && result != ERR_INVALID_VOLUME)
            msg(tr("parseFile: parsing file as BIOS failed with error \"%1\"").arg(errorMessage(result)), index);
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
    if ((UINT32)file.size() < sectionOffset + sizeof(EFI_COMMON_SECTION_HEADER))
        return ERR_INVALID_FILE;
	
    const EFI_COMMON_SECTION_HEADER* sectionHeader = (const EFI_COMMON_SECTION_HEADER*)(file.constData() + sectionOffset);
    sectionSize = uint24ToUint32(sectionHeader->Size);
    // This may introduce a very rare error with a non-extended section of size equal to 0xFFFFFF
	if (sectionSize != 0xFFFFFF)
        return ERR_SUCCESS;
 
    if ((UINT32)file.size() < sectionOffset + sizeof(EFI_COMMON_SECTION_HEADER2))
        return ERR_INVALID_FILE;

    const EFI_COMMON_SECTION_HEADER2* sectionHeader2 = (const EFI_COMMON_SECTION_HEADER2*)(file.constData() + sectionOffset);
    sectionSize = sectionHeader2->ExtendedSize;	  
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
        // Exit from loop if no sections left
        if (sectionSize == 0)
            break;

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

void FfsEngine::parseAprioriRawSection(const QByteArray & body, QString & parsed)
{
    parsed.clear();

    UINT32 count = body.size() / sizeof(EFI_GUID);
    if (count > 0) {
        for (UINT32 i = 0; i < count; i++) {
            const EFI_GUID* guid = (const EFI_GUID*)body.constData() + i;
            parsed += tr("\n%1").arg(guidToQString(*guid));
        }
    }
}

UINT8 FfsEngine::parseDepexSection(const QByteArray & body, QString & parsed)
{
    parsed.clear();
    // Check data to be present
    if (!body.size())
        return ERR_INVALID_PARAMETER;

    const EFI_GUID * guid;
    const UINT8* current = (const UINT8*)body.constData();

    // Special cases of first opcode
    switch (*current) {
    case EFI_DEP_BEFORE:
        if (body.size() != 2*EFI_DEP_OPCODE_SIZE + sizeof(EFI_GUID))
            return ERR_DEPEX_PARSE_FAILED;
        guid = (const EFI_GUID*)(current + EFI_DEP_OPCODE_SIZE);
        parsed += tr("\nBEFORE %1").arg(guidToQString(*guid));
        current += EFI_DEP_OPCODE_SIZE + sizeof(EFI_GUID);
        if (*current != EFI_DEP_END)
            return ERR_DEPEX_PARSE_FAILED;
        return ERR_SUCCESS;
    case EFI_DEP_AFTER:
        if (body.size() != 2 * EFI_DEP_OPCODE_SIZE + sizeof(EFI_GUID))
            return ERR_DEPEX_PARSE_FAILED;
        guid = (const EFI_GUID*)(current + EFI_DEP_OPCODE_SIZE);
        parsed += tr("\nAFTER %1").arg(guidToQString(*guid));
        current += EFI_DEP_OPCODE_SIZE + sizeof(EFI_GUID);
        if (*current != EFI_DEP_END)
            return ERR_DEPEX_PARSE_FAILED;
        return ERR_SUCCESS;
    case EFI_DEP_SOR:
        if (body.size() <= 2 * EFI_DEP_OPCODE_SIZE) {
            return ERR_DEPEX_PARSE_FAILED;
        }
        parsed += tr("\nSOR");
        current += EFI_DEP_OPCODE_SIZE;
        break;
    }

    // Parse the rest of depex
    while (current - (const UINT8*)body.constData() < body.size()) {
        switch (*current) {
        case EFI_DEP_BEFORE:
        case EFI_DEP_AFTER:
        case EFI_DEP_SOR:
            return ERR_DEPEX_PARSE_FAILED;
        case EFI_DEP_PUSH:
            // Check that the rest of depex has correct size
            if ((UINT32)body.size() - (UINT32)(current - (const UINT8*)body.constData()) <= EFI_DEP_OPCODE_SIZE + sizeof(EFI_GUID)) {
                parsed.clear();
                return ERR_DEPEX_PARSE_FAILED;
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
                return ERR_DEPEX_PARSE_FAILED;
            }
            break;
        default:
            return ERR_DEPEX_PARSE_FAILED;
            break;
        }
    }

    return ERR_SUCCESS;
}

UINT8 x86Convert(QByteArray & input, int mode) {
    unsigned char* source = (unsigned char*)input.data();
    UINT32 sourceSize = input.size();

    UINT32 state;
    x86_Convert_Init(state);
    UINT32 converted = x86_Convert(source, sourceSize, 0, &state, mode);

    const UINT8 x86LookAhead = 4;
    if (converted + x86LookAhead != sourceSize) {
        return ERR_INVALID_VOLUME;
    }

    return ERR_SUCCESS;
}

UINT8 FfsEngine::parseSection(const QByteArray & section, QModelIndex & index, const QModelIndex & parent, const UINT8 mode)
{
    const EFI_COMMON_SECTION_HEADER* sectionHeader = (const EFI_COMMON_SECTION_HEADER*)(section.constData());
    QString name = sectionTypeToQString(sectionHeader->Type) + tr(" section");
    QString info;
    QByteArray header;
    QByteArray body;
    UINT32 headerSize = sizeOfSectionHeader(sectionHeader);
    UINT8 result;

    switch (sectionHeader->Type) {
    // Encapsulated sections
    case EFI_SECTION_COMPRESSION:
    {
        bool parseCurrentSection = true;
        QByteArray decompressed;
        UINT8 algorithm;
        const EFI_COMPRESSION_SECTION* compressedSectionHeader = (const EFI_COMPRESSION_SECTION*)sectionHeader;
        header = section.left(headerSize);
        body = section.mid(headerSize);
        algorithm = COMPRESSION_ALGORITHM_UNKNOWN;
        // Decompress section
        result = decompress(body, compressedSectionHeader->CompressionType, decompressed, &algorithm);
        if (result)
            parseCurrentSection = false;

        // Get info
        info = tr("Type: %1h\nFull size: %2h (%3)\nHeader size: %4h (%5)\nBody size: %6h (%7)\nCompression type: %8\nDecompressed size: %9h (%10)")
            .hexarg2(sectionHeader->Type, 2)
            .hexarg(section.size()).arg(section.size())
            .hexarg(header.size()).arg(header.size())
            .hexarg(body.size()).arg(body.size())
            .arg(compressionTypeToQString(algorithm))
            .hexarg(compressedSectionHeader->UncompressedLength).arg(compressedSectionHeader->UncompressedLength);

        UINT32 dictionarySize = DEFAULT_LZMA_DICTIONARY_SIZE;
        if (algorithm == COMPRESSION_ALGORITHM_LZMA) {
            // Dictionary size is stored in bytes 1-4 of LZMA-compressed data
            dictionarySize = *(UINT32*)(body.constData() + 1);
            info += tr("\nLZMA dictionary size: %1h").hexarg(dictionarySize);
        }

        // Add tree item
        index = model->addItem(Types::Section, sectionHeader->Type, algorithm, name, "", info, header, body, parent, mode);
        model->setDictionarySize(index, dictionarySize);

        // Show message
        if (!parseCurrentSection)
            msg(tr("parseSection: decompression failed with error \"%1\"").arg(errorMessage(result)), index);
        else { // Parse decompressed data
            result = parseSections(decompressed, index);
            if (result)
                return result;
        }
    } break;

    case EFI_SECTION_GUID_DEFINED:
    {
        bool parseCurrentSection = true;
        bool msgUnknownGuid = false;
        bool msgInvalidCrc = false;
        bool msgUnknownAuth = false;
        bool msgSigned = false;
        bool msgInvalidSignatureLength = false;
        bool msgUnknownSignature = false;
        bool msgUnknownUefiGuidSignature = false;

        header = section.left(headerSize);
        body = section.mid(headerSize);
 
        const EFI_GUID_DEFINED_SECTION* guidDefinedSectionHeader = (const EFI_GUID_DEFINED_SECTION*)(header.constData());
        QByteArray processed = body;

        // Get info
        name = guidToQString(guidDefinedSectionHeader->SectionDefinitionGuid);
        info = tr("Section GUID: %1\nType: %2h\nFull size: %3h (%4)\nHeader size: %5h (%6)\nBody size: %7h (%8)\nData offset: %9h\nAttributes: %10h")
            .arg(name)
            .hexarg2(sectionHeader->Type, 2)
            .hexarg(section.size()).arg(section.size())
            .hexarg(header.size()).arg(header.size())
            .hexarg(body.size()).arg(body.size())
            .hexarg(guidDefinedSectionHeader->DataOffset)
            .hexarg2(guidDefinedSectionHeader->Attributes, 4);

        UINT8 algorithm = COMPRESSION_ALGORITHM_NONE;
        UINT32 dictionarySize = DEFAULT_LZMA_DICTIONARY_SIZE;

        // Check if section requires processing
        QByteArray parsedGuid = QByteArray((const char*)&guidDefinedSectionHeader->SectionDefinitionGuid, sizeof(EFI_GUID));
        if (guidDefinedSectionHeader->Attributes & EFI_GUIDED_SECTION_PROCESSING_REQUIRED) {
            // Tiano compressed section
            if (parsedGuid == EFI_GUIDED_SECTION_TIANO) {
                algorithm = COMPRESSION_ALGORITHM_UNKNOWN;

                result = decompress(body, EFI_STANDARD_COMPRESSION, processed, &algorithm);
                if (result)
                    parseCurrentSection = false;

                if (algorithm == COMPRESSION_ALGORITHM_TIANO) {
                    info += tr("\nCompression type: Tiano");
                    info += tr("\nDecompressed size: %1h (%2)").hexarg(processed.length()).arg(processed.length());
                }
                else if (algorithm == COMPRESSION_ALGORITHM_EFI11) {
                    info += tr("\nCompression type: EFI 1.1");
                    info += tr("\nDecompressed size: %1h (%2)").hexarg(processed.length()).arg(processed.length());
                }
                else
                    info += tr("\nCompression type: unknown");
            }
            // LZMA compressed section
            else if (parsedGuid == EFI_GUIDED_SECTION_LZMA || parsedGuid == EFI_GUIDED_SECTION_LZMAF86) {
                algorithm = COMPRESSION_ALGORITHM_UNKNOWN;

                result = decompress(body, EFI_CUSTOMIZED_COMPRESSION, processed, &algorithm);
                if (result)
                    parseCurrentSection = false;
                if (parsedGuid == EFI_GUIDED_SECTION_LZMAF86) {
                    if (x86Convert(processed, 0) != ERR_SUCCESS) {
                        msg(tr("parseSection: unable to convert LZMAF86 compressed data"));
                    }
                }

                if (algorithm == COMPRESSION_ALGORITHM_LZMA) {
                    info += tr("\nCompression type: LZMA");
                    info += tr("\nDecompressed size: %1h (%2)").hexarg(processed.length()).arg(processed.length());

                    // Dictionary size is stored in bytes 1-4 of LZMA-compressed data
                    dictionarySize = *(UINT32*)(body.constData() + 1);
                    info += tr("\nLZMA dictionary size: %1h").hexarg(dictionarySize);
                }
                else
                    info += tr("\nCompression type: unknown");
            }
            // Signed section
            else if (parsedGuid == EFI_FIRMWARE_CONTENTS_SIGNED_GUID) {
                msgSigned = true;
                const WIN_CERTIFICATE* certificateHeader = (const WIN_CERTIFICATE*)body.constData();
                if ((UINT32)body.size() < sizeof(WIN_CERTIFICATE)) {
                    info += tr("\nSignature type: invalid, wrong length");
                    msgInvalidSignatureLength = true;
                    parseCurrentSection = false;
                }
                else if (certificateHeader->CertificateType == WIN_CERT_TYPE_EFI_GUID) {
                    info += tr("\nSignature type: UEFI");
                    const WIN_CERTIFICATE_UEFI_GUID* guidCertificateHeader = (const WIN_CERTIFICATE_UEFI_GUID*)certificateHeader;
                    if (QByteArray((const char*)&guidCertificateHeader->CertType, sizeof(EFI_GUID)) == EFI_CERT_TYPE_RSA2048_SHA256_GUID) {
                        info += tr("\nSignature subtype: RSA2048/SHA256");
                        // TODO: show signature info in Information panel
                    }
                    else if (QByteArray((const char*)&guidCertificateHeader->CertType, sizeof(EFI_GUID)) == EFI_CERT_TYPE_PKCS7_GUID) {
                        info += tr("\nSignature subtype: PCKS7");
                        // TODO: show signature info in Information panel
                    }
                    else {
                        info += tr("\nSignature subtype: unknown");
                        msgUnknownUefiGuidSignature = true;
                    }
                }
                else if (certificateHeader->CertificateType == WIN_CERT_TYPE_PKCS_SIGNED_DATA) {
                    info += tr("\nSignature type: PKCS7");
                    // TODO: show signature info in Information panel
                }
                else {
                    info += tr("\nSignature type: unknown");
                    msgUnknownSignature = true;
                }

                if ((UINT32)body.size() < certificateHeader->Length) {
                    info += tr("\nSignature type: invalid, wrong length");
                    msgInvalidSignatureLength = true;
                    parseCurrentSection = false;
                }
                else {
                    // Add additional data to the header
                    header.append(body.left(certificateHeader->Length));
                    // Get new body
                    processed = body = body.mid(certificateHeader->Length);
                }
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
            if (parsedGuid == EFI_GUIDED_SECTION_CRC32) {
                info += tr("\nChecksum type: CRC32");
                // Calculate CRC32 of section data
                UINT32 crc = crc32(0, (const UINT8*)body.constData(), body.size());
                // Check stored CRC32
                if (crc == *(const UINT32*)(header.constData() + sizeof(EFI_GUID_DEFINED_SECTION))) {
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
        index = model->addItem(Types::Section, sectionHeader->Type, algorithm, name, "", info, header, body, parent, mode);
        model->setDictionarySize(index, dictionarySize);

        // Show messages
        if (msgUnknownGuid)
            msg(tr("parseSection: GUID defined section with unknown processing method"), index);
        if (msgUnknownAuth)
            msg(tr("parseSection: GUID defined section with unknown authentication method"), index);
        if (msgInvalidCrc)
            msg(tr("parseSection: GUID defined section with invalid CRC32"), index);
        if (msgSigned)
            msg(tr("parseSection: signature may become invalid after any modification"), index);
        if (msgUnknownUefiGuidSignature)
            msg(tr("parseSection: GUID defined section with unknown signature subtype"), index);
        if (msgInvalidSignatureLength)
            msg(tr("parseSection: GUID defined section with invalid signature length"), index);
        if (msgUnknownSignature)
            msg(tr("parseSection: GUID defined section with unknown signature type"), index);

        if (!parseCurrentSection) {
            msg(tr("parseSection: GUID defined section can not be processed"), index);
        }
        else { // Parse processed data
            result = parseSections(processed, index);
            if (result)
                return result;
        }
    } break;

    case EFI_SECTION_DISPOSABLE:
    {
        header = section.left(headerSize);
        body = section.mid(headerSize);

        // Get info
        info = tr("Type: %1h\nFull size: %2h (%3)\nHeader size: %4h (%5)\nBody size: %6h (%7)")
            .hexarg2(sectionHeader->Type, 2)
            .hexarg(section.size()).arg(section.size())
            .hexarg(header.size()).arg(header.size())
            .hexarg(body.size()).arg(body.size());

        // Add tree item
        index = model->addItem(Types::Section, sectionHeader->Type, COMPRESSION_ALGORITHM_NONE, name, "", info, header, body, parent, mode);

        // Parse section body
        result = parseSections(body, index);
        if (result)
            return result;
    } break;

    // Leaf sections
    case EFI_SECTION_DXE_DEPEX:
    case EFI_SECTION_PEI_DEPEX:
    case EFI_SECTION_SMM_DEPEX: {
        bool msgDepexParseFailed = false;
        header = section.left(headerSize);
        body = section.mid(headerSize);

        // Get info
        info = tr("Type: %1h\nFull size: %2h (%3)\nHeader size: %4h (%5)\nBody size: %6h (%7)")
            .hexarg2(sectionHeader->Type, 2)
            .hexarg(section.size()).arg(section.size())
            .hexarg(header.size()).arg(header.size())
            .hexarg(body.size()).arg(body.size());

        // Parse dependency expression
        QString str;
        result = parseDepexSection(body, str);
        if (result)
            msgDepexParseFailed = true;
        else if (str.count())
            info += tr("\nParsed expression:%1").arg(str);

        // Add tree item
        index = model->addItem(Types::Section, sectionHeader->Type, COMPRESSION_ALGORITHM_NONE, name, "", info, header, body, parent, mode);

        // Show messages
        if (msgDepexParseFailed)
            msg(tr("parseSection: dependency expression parsing failed"), index);
    } break;

    case EFI_SECTION_TE: {
        header = section.left(headerSize);
        body = section.mid(headerSize);

        // Get standard info
        info = tr("Type: %1h\nFull size: %2h (%3)\nHeader size: %4h (%5)\nBody size: %6h (%7)")
            .hexarg2(sectionHeader->Type, 2)
            .hexarg(section.size()).arg(section.size())
            .hexarg(header.size()).arg(header.size())
            .hexarg(body.size()).arg(body.size());

        // Get TE info
        bool msgInvalidSignature = false;
        const EFI_IMAGE_TE_HEADER* teHeader = (const EFI_IMAGE_TE_HEADER*)body.constData();
        
        // Most EFI images today include teFixup in ImageBase value,
        // which doesn't follow the UEFI spec, but is so popular that
        // only a few images out of thousands are different
        UINT32 teFixup = 0; //teHeader->StrippedSize - sizeof(EFI_IMAGE_TE_HEADER);
        if (teHeader->Signature != EFI_IMAGE_TE_SIGNATURE) {
            info += tr("\nSignature: %1h, invalid").hexarg2(teHeader->Signature, 4);
            msgInvalidSignature = true;
        }
        else {
            info += tr("\nSignature: %1h\nMachine type: %2\nNumber of sections: %3\nSubsystem: %4h\nStrippedSize: %5h (%6)\nBaseOfCode: %7h\nRelativeEntryPoint: %8h\nImageBase: %9h\nEntryPoint: %10h")
                .hexarg2(teHeader->Signature, 4)
                .arg(machineTypeToQString(teHeader->Machine))
                .arg(teHeader->NumberOfSections)
                .hexarg2(teHeader->Subsystem, 2)
                .hexarg(teHeader->StrippedSize).arg(teHeader->StrippedSize)
                .hexarg(teHeader->BaseOfCode)
                .hexarg(teHeader->AddressOfEntryPoint)
                .hexarg(teHeader->ImageBase)
                .hexarg(teHeader->ImageBase + teHeader->AddressOfEntryPoint - teFixup);
        }
        // Add tree item
        index = model->addItem(Types::Section, sectionHeader->Type, COMPRESSION_ALGORITHM_NONE, name, "", info, header, body, parent, mode);

        // Show messages
        if (msgInvalidSignature) {
            msg("parseSection: TE image with invalid TE signature", index);
        }

        // Special case of PEI Core
        QModelIndex core = model->findParentOfType(index, Types::File);
        if (core.isValid() && model->subtype(core) == EFI_FV_FILETYPE_PEI_CORE
            && oldPeiCoreEntryPoint == 0) {
            result = getEntryPoint(model->body(index), oldPeiCoreEntryPoint);
            if (result)
                msg(tr("parseSection: can't get original PEI core entry point"), index);
        }
    } break;

    case EFI_SECTION_PE32:
    case EFI_SECTION_PIC: {
        header = section.left(headerSize);
        body = section.mid(headerSize);

        // Get standard info
        info = tr("Type: %1h\nFull size: %2h (%3)\nHeader size: %4h (%5)\nBody size: %6h (%7)")
            .hexarg2(sectionHeader->Type, 2)
            .hexarg(section.size()).arg(section.size())
            .hexarg(header.size()).arg(header.size())
            .hexarg(body.size()).arg(body.size());

        // Get PE info
        bool msgInvalidDosSignature = false;
        bool msgInvalidDosHeader = false;
        bool msgInvalidPeSignature = false;
        bool msgUnknownOptionalHeaderSignature = false;

        const EFI_IMAGE_DOS_HEADER* dosHeader = (const EFI_IMAGE_DOS_HEADER*)body.constData();
        if (dosHeader->e_magic != EFI_IMAGE_DOS_SIGNATURE) {
            info += tr("\nDOS signature: %1h, invalid").hexarg2(dosHeader->e_magic, 4);
            msgInvalidDosSignature = true;
        }
        else {
            const EFI_IMAGE_PE_HEADER* peHeader = (EFI_IMAGE_PE_HEADER*)(body.constData() + dosHeader->e_lfanew);
            if ((UINT32)body.size() < dosHeader->e_lfanew + sizeof(EFI_IMAGE_PE_HEADER)) {
                info += tr("\nDOS lfanew: %1h, invalid").hexarg2(dosHeader->e_lfanew, 8);
                msgInvalidDosHeader = true;
            }
            else if (peHeader->Signature != EFI_IMAGE_PE_SIGNATURE) {
                info += tr("\nPE signature: %1h, invalid").hexarg2(peHeader->Signature, 8);
                msgInvalidPeSignature = true;
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
                    info += tr("\nOptional header signature: %1h\nSubsystem: %2h\nRelativeEntryPoint: %3h\nBaseOfCode: %4h\nImageBase: %5h\nEntryPoint: %6h")
                        .hexarg2(optionalHeader.H32->Magic, 4)
                        .hexarg2(optionalHeader.H32->Subsystem, 4)
                        .hexarg(optionalHeader.H32->AddressOfEntryPoint)
                        .hexarg(optionalHeader.H32->BaseOfCode)
                        .hexarg(optionalHeader.H32->ImageBase)
                        .hexarg(optionalHeader.H32->ImageBase + optionalHeader.H32->AddressOfEntryPoint);
                }
                else if (optionalHeader.H32->Magic == EFI_IMAGE_PE_OPTIONAL_HDR64_MAGIC) {
                    info += tr("\nOptional header signature: %1h\nSubsystem: %2h\nRelativeEntryPoint: %3h\nBaseOfCode: %4h\nImageBase: %5h\nEntryPoint: %6h")
                        .hexarg2(optionalHeader.H64->Magic, 4)
                        .hexarg2(optionalHeader.H64->Subsystem, 4)
                        .hexarg(optionalHeader.H64->AddressOfEntryPoint)
                        .hexarg(optionalHeader.H64->BaseOfCode)
                        .hexarg(optionalHeader.H64->ImageBase)
                        .hexarg(optionalHeader.H64->ImageBase + optionalHeader.H64->AddressOfEntryPoint);
                }
                else {
                    info += tr("\nOptional header signature: %1h, unknown").hexarg2(optionalHeader.H64->Magic, 4);
                    msgUnknownOptionalHeaderSignature = true;
                }
            }
        }

        // Add tree item
        index = model->addItem(Types::Section, sectionHeader->Type, COMPRESSION_ALGORITHM_NONE, name, "", info, header, body, parent, mode);

        // Show messages
        if (msgInvalidDosSignature) {
            msg("parseSection: PE32 image with invalid DOS signature", index);
        }
        if (msgInvalidDosHeader) {
            msg("parseSection: PE32 image with invalid DOS header", index);
        }
        if (msgInvalidPeSignature) {
            msg("parseSection: PE32 image with invalid PE signature", index);
        }
        if (msgUnknownOptionalHeaderSignature) {
            msg("parseSection: PE32 image with unknown optional header signature", index);
        }

        // Special case of PEI Core
        QModelIndex core = model->findParentOfType(index, Types::File);
        if (core.isValid() && model->subtype(core) == EFI_FV_FILETYPE_PEI_CORE
            && oldPeiCoreEntryPoint == 0) {
            result = getEntryPoint(model->body(index), oldPeiCoreEntryPoint);
            if (result)
                msg(tr("parseSection: can't get original PEI core entry point"), index);
        }
    } break;

    case EFI_SECTION_COMPATIBILITY16: {
        header = section.left(headerSize);
        body = section.mid(headerSize);

        // Get info
        info = tr("Type: %1h\nFull size: %2h (%3)\nHeader size: %4h (%5)\nBody size: %6h (%7)")
            .hexarg2(sectionHeader->Type, 2)
            .hexarg(section.size()).arg(section.size())
            .hexarg(header.size()).arg(header.size())
            .hexarg(body.size()).arg(body.size());

        // Add tree item
        index = model->addItem(Types::Section, sectionHeader->Type, COMPRESSION_ALGORITHM_NONE, name, "", info, header, body, parent, mode);
    } break;

    case EFI_SECTION_FREEFORM_SUBTYPE_GUID: {
        header = section.left(headerSize);
        body = section.mid(headerSize);

        const EFI_FREEFORM_SUBTYPE_GUID_SECTION* fsgHeader = (const EFI_FREEFORM_SUBTYPE_GUID_SECTION*)sectionHeader;
        // Get info
        info = tr("Type: %1h\nFull size: %2h (%3)\nHeader size: %4h (%5)\nBody size: %6h (%7)\nSubtype GUID: %8")
            .hexarg2(fsgHeader->Type, 2)
            .hexarg(section.size()).arg(section.size())
            .hexarg(header.size()).arg(header.size())
            .hexarg(body.size()).arg(body.size())
            .arg(guidToQString(fsgHeader->SubTypeGuid));

        // Add tree item
        index = model->addItem(Types::Section, sectionHeader->Type, COMPRESSION_ALGORITHM_NONE, name, "", info, header, body, parent, mode);

        // Rename section
        model->setName(index, guidToQString(fsgHeader->SubTypeGuid));
    } break;

    case EFI_SECTION_VERSION: {
        header = section.left(headerSize);
        body = section.mid(headerSize);

        const EFI_VERSION_SECTION* versionHeader = (const EFI_VERSION_SECTION*)sectionHeader;

        // Get info
        info = tr("Type: %1h\nFull size: %2h (%3)\nHeader size: %4h (%5)\nBody size: %6h (%7)\nBuild number: %8\nVersion string: %9")
            .hexarg2(versionHeader->Type, 2)
            .hexarg(section.size()).arg(section.size())
            .hexarg(header.size()).arg(header.size())
            .hexarg(body.size()).arg(body.size())
            .arg(versionHeader->BuildNumber)
            .arg(QString::fromUtf16((const ushort*)body.constData()));

        // Add tree item
        index = model->addItem(Types::Section, sectionHeader->Type, COMPRESSION_ALGORITHM_NONE, name, "", info, header, body, parent, mode);
    } break;

    case EFI_SECTION_USER_INTERFACE: {
        header = section.left(headerSize);
        body = section.mid(headerSize);
        QString text = QString::fromUtf16((const ushort*)body.constData());

        // Get info
        info = tr("Type: %1h\nFull size: %2h (%3)\nHeader size: %4h (%5)\nBody size: %6h (%7)\nText: %8")
            .hexarg2(sectionHeader->Type, 2)
            .hexarg(section.size()).arg(section.size())
            .hexarg(header.size()).arg(header.size())
            .hexarg(body.size()).arg(body.size())
            .arg(text);

        // Add tree item
        index = model->addItem(Types::Section, sectionHeader->Type, COMPRESSION_ALGORITHM_NONE, name, "", info, header, body, parent, mode);

        // Rename parent file
        model->setText(model->findParentOfType(parent, Types::File), text);
    } break;

    case EFI_SECTION_FIRMWARE_VOLUME_IMAGE: {
        header = section.left(headerSize);
        body = section.mid(headerSize);

        // Get info
        info = tr("Type: %1h\nFull size: %2h (%3)\nHeader size: %4h (%5)\nBody size: %6h (%7)")
            .hexarg2(sectionHeader->Type, 2)
            .hexarg(section.size()).arg(section.size())
            .hexarg(header.size()).arg(header.size())
            .hexarg(body.size()).arg(body.size());

        // Add tree item
        index = model->addItem(Types::Section, sectionHeader->Type, COMPRESSION_ALGORITHM_NONE, name, "", info, header, body, parent, mode);

        // Parse section body as BIOS space
        result = parseBios(body, index);
        if (result && result != ERR_VOLUMES_NOT_FOUND && result != ERR_INVALID_VOLUME) {
            msg(tr("parseSection: parsing firmware volume image section as BIOS failed with error \"%1\"").arg(errorMessage(result)), index);
            return result;
        }
    } break;

    case EFI_SECTION_RAW: {
        bool parsed = false;
        header = section.left(headerSize);
        body = section.mid(headerSize);

        // Get info
        info = tr("Type: %1h\nFull size: %2h (%3)\nHeader size: %4h (%5)\nBody size: %6h (%7)")
            .hexarg2(sectionHeader->Type, 2)
            .hexarg(section.size()).arg(section.size())
            .hexarg(header.size()).arg(header.size())
            .hexarg(body.size()).arg(body.size());

        // Check for apriori file
        QModelIndex parentFile = model->findParentOfType(parent, Types::File);
        QByteArray parentFileGuid = model->header(parentFile).left(sizeof(EFI_GUID));
        if (parentFileGuid == EFI_PEI_APRIORI_FILE_GUID) {
            // Mark file as parsed
            parsed = true;

            // Parse apriori file list
            QString str;
            parseAprioriRawSection(body, str);
            if (str.count())
                info += tr("\nFile list:%1").arg(str);

            // Set parent file text
            model->setText(parentFile, tr("PEI apriori file"));
        }
        else if (parentFileGuid == EFI_DXE_APRIORI_FILE_GUID) {
            // Mark file as parsed
            parsed = true;

            // Parse apriori file list
            QString str;
            parseAprioriRawSection(body, str);
            if (str.count())
                info += tr("\nFile list:%1").arg(str);

            // Set parent file text
            model->setText(parentFile, tr("DXE apriori file"));
        }

        // Add tree item
        index = model->addItem(Types::Section, sectionHeader->Type, COMPRESSION_ALGORITHM_NONE, name, "", info, header, body, parent, mode);

        // Parse section body as BIOS space
        if (!parsed) {
            result = parseBios(body, index);
            if (result && result != ERR_VOLUMES_NOT_FOUND && result != ERR_INVALID_VOLUME) {
                msg(tr("parseSection: parsing raw section as BIOS failed with error \"%1\"").arg(errorMessage(result)), index);
                return result;
            }
        }
    } break;

    case SCT_SECTION_POSTCODE:
    case INSYDE_SECTION_POSTCODE: {
        header = section.left(headerSize);
        body = section.mid(headerSize);

        const POSTCODE_SECTION* postcodeHeader = (const POSTCODE_SECTION*)sectionHeader;

        // Get info
        info = tr("Type: %1h\nFull size: %2h (%3)\nHeader size: %4h (%5)\nBody size: %6h (%7)\nPostcode: %8h")
            .hexarg2(postcodeHeader->Type, 2)
            .hexarg(section.size()).arg(section.size())
            .hexarg(header.size()).arg(header.size())
            .hexarg(body.size()).arg(body.size())
            .hexarg(postcodeHeader->Postcode);

        // Add tree item
        index = model->addItem(Types::Section, sectionHeader->Type, COMPRESSION_ALGORITHM_NONE, name, "", info, header, body, parent, mode);
    } break;

    default:
        header = section.left(headerSize);
        body = section.mid(headerSize);
        // Get info
        info = tr("Type: %1h\nFull size: %2h (%3)\nHeader size: %4h (%5)\nBody size: %6h (%7)")
            .hexarg2(sectionHeader->Type, 2)
            .hexarg(section.size()).arg(section.size())
            .hexarg(header.size()).arg(header.size())
            .hexarg(body.size()).arg(body.size());

        // Add tree item
        index = model->addItem(Types::Section, sectionHeader->Type, COMPRESSION_ALGORITHM_NONE, name, "", info, header, body, parent, mode);
        msg(tr("parseSection: section with unknown type %1h").hexarg2(sectionHeader->Type, 2), index);
    }
    return ERR_SUCCESS;
}

// Operations on tree items
UINT8 FfsEngine::create(const QModelIndex & index, const UINT8 type, const QByteArray & header, const QByteArray & body, const UINT8 mode, const UINT8 action, const UINT8 algorithm)
{
    QByteArray created;
    UINT8 result;
    QModelIndex fileIndex;
    UINT32 defaultDictionarySize = DEFAULT_LZMA_DICTIONARY_SIZE;

    if (!index.isValid() || !index.parent().isValid())
        return ERR_INVALID_PARAMETER;

    QModelIndex parent;
    if (mode == CREATE_MODE_BEFORE || mode == CREATE_MODE_AFTER)
        parent = index.parent();
    else
        parent = index;

    // Create item
    if (type == Types::Region) {
        UINT8 type = model->subtype(index);
        switch (type) {
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

        if (result && result != ERR_VOLUMES_NOT_FOUND && result != ERR_INVALID_VOLUME)
            return result;

        // Set action
        model->setAction(fileIndex, action);
    }
    else if (type == Types::Padding) {
        // Get info
        QString name = tr("Padding");
        QString info = tr("Full size: %1h (%2)")
            .hexarg(body.size()).arg(body.size());

        // Add tree item
        QModelIndex fileIndex = model->addItem(Types::Padding, getPaddingType(body), COMPRESSION_ALGORITHM_NONE, name, "", info, QByteArray(), body, index, mode);

        // Set action
        model->setAction(fileIndex, action);
    }
    else if (type == Types::Volume) {
        QByteArray volume;
        if (header.isEmpty()) // Whole volume
            volume.append(body);
        else { // Body only
            volume.append(header).append(body);
            INT32 sizeDiff = model->body(index).size() - body.size();
            if (sizeDiff > 0) {
                const EFI_FIRMWARE_VOLUME_HEADER* volumeHeader = (const EFI_FIRMWARE_VOLUME_HEADER*)model->header(index).constData();
                bool erasePolarity = volumeHeader->Attributes & EFI_FVB_ERASE_POLARITY;
                volume.append(QByteArray(sizeDiff, erasePolarity ? '\xFF' : '\x00'));
            }
        }
        result = parseVolume(volume, fileIndex, index, mode);
        if (result)
           return result;

        // Set action
        model->setAction(fileIndex, action);
    }
    else if (type == Types::File) {
        if (model->type(parent) != Types::Volume)
            return ERR_INVALID_FILE;

        const EFI_FIRMWARE_VOLUME_HEADER* volumeHeader = (const EFI_FIRMWARE_VOLUME_HEADER*)model->header(parent).constData();
        UINT8 revision = volumeHeader->Revision;
        bool erasePolarity = volumeHeader->Attributes & EFI_FVB_ERASE_POLARITY;

        if (header.size() != sizeof(EFI_FFS_FILE_HEADER))
            return ERR_INVALID_FILE;

        QByteArray newObject = header + body;
        EFI_FFS_FILE_HEADER* fileHeader = (EFI_FFS_FILE_HEADER*)newObject.data();

        // Determine correct file header size
        bool largeFile = false;
        UINT32 headerSize = sizeof(EFI_FFS_FILE_HEADER);
        if (revision > 1 && (fileHeader->Attributes & FFS_ATTRIB_LARGE_FILE)) {
            largeFile = true;
            headerSize = sizeof(EFI_FFS_FILE_HEADER2);
        }

        QByteArray newHeader = newObject.left(headerSize);
        QByteArray newBody = newObject.mid(headerSize);

        // Check if the file has a tail
        UINT8 tailSize = (revision == 1 && (fileHeader->Attributes & FFS_ATTRIB_TAIL_PRESENT)) ? sizeof(UINT16) : 0;
        if (tailSize) {
            // Remove the tail, it will then be added back for revision 1 volumes
            newBody = newBody.left(newBody.size() - tailSize);
        }

        // Correct file size
        if (!largeFile) {
            if (newBody.size() >= 0xFFFFFF) {
                return ERR_INVALID_FILE;
            }

            uint32ToUint24(headerSize + newBody.size() + tailSize, fileHeader->Size);
        }
        else {
            uint32ToUint24(0xFFFFFF, fileHeader->Size);
            EFI_FFS_FILE_HEADER2* fileHeader2 = (EFI_FFS_FILE_HEADER2*)newHeader.data();
            fileHeader2->ExtendedSize = headerSize + newBody.size() + tailSize;
        }

        // Set file state
        UINT8 state = EFI_FILE_DATA_VALID | EFI_FILE_HEADER_VALID | EFI_FILE_HEADER_CONSTRUCTION;
        if (erasePolarity)
            state = ~state;
        fileHeader->State = state;

        // Recalculate header checksum
        fileHeader->IntegrityCheck.Checksum.Header = 0;
        fileHeader->IntegrityCheck.Checksum.File = 0;
        fileHeader->IntegrityCheck.Checksum.Header = 0x100 - (calculateSum8((const UINT8*)newHeader.constData(), headerSize) - fileHeader->State);

        // Recalculate data checksum, if needed
        if (fileHeader->Attributes & FFS_ATTRIB_CHECKSUM)
            fileHeader->IntegrityCheck.Checksum.File = calculateChecksum8((const UINT8*)newBody.constData(), newBody.size());
        else if (revision == 1)
            fileHeader->IntegrityCheck.Checksum.File = FFS_FIXED_CHECKSUM;
        else
            fileHeader->IntegrityCheck.Checksum.File = FFS_FIXED_CHECKSUM2;

        // Append new body
        created.append(newBody);

        // Append tail, if needed
        if (revision == 1 && tailSize) {
            UINT8 ht = ~fileHeader->IntegrityCheck.Checksum.Header;
            UINT8 ft = ~fileHeader->IntegrityCheck.Checksum.File;
            created.append(ht).append(ft);
        }

        // Prepend header
        created.prepend(newHeader);

        // Parse file
        result = parseFile(created, fileIndex, revision, erasePolarity ? ERASE_POLARITY_TRUE : ERASE_POLARITY_FALSE, index, mode);
        if (result && result != ERR_VOLUMES_NOT_FOUND  && result != ERR_INVALID_VOLUME)
            return result;

        // Set action
        model->setAction(fileIndex, action);

        // Rebase all PEI-files that follow
        rebasePeiFiles(fileIndex);
    }
    else if (type == Types::Section) {
        if (model->type(parent) != Types::File && model->type(parent) != Types::Section)
            return ERR_INVALID_SECTION;

        if ((UINT32)header.size() < sizeof(EFI_COMMON_SECTION_HEADER))
            return ERR_INVALID_SECTION;

        QByteArray newHeader = header;
        EFI_COMMON_SECTION_HEADER* commonHeader = (EFI_COMMON_SECTION_HEADER*)newHeader.data();

        if (uint24ToUint32(commonHeader->Size) == EFI_SECTION2_IS_USED) {
            msg(tr("create: creation of large sections not supported yet"), index);
            return ERR_NOT_IMPLEMENTED;
        }

        switch (commonHeader->Type)
        {
        case EFI_SECTION_COMPRESSION: {
            EFI_COMPRESSION_SECTION* sectionHeader = (EFI_COMPRESSION_SECTION*)newHeader.data();
            // Correct uncompressed size
            sectionHeader->UncompressedLength = body.size();

            // Set compression type
            if (algorithm == COMPRESSION_ALGORITHM_NONE) {
                sectionHeader->CompressionType = EFI_NOT_COMPRESSED;
            }
            else if (algorithm == COMPRESSION_ALGORITHM_EFI11 || algorithm == COMPRESSION_ALGORITHM_TIANO) {
                sectionHeader->CompressionType = EFI_STANDARD_COMPRESSION;
            }
            else if (algorithm == COMPRESSION_ALGORITHM_LZMA || algorithm == COMPRESSION_ALGORITHM_IMLZMA) {
                sectionHeader->CompressionType = EFI_CUSTOMIZED_COMPRESSION;
            }
            else
                return ERR_UNKNOWN_COMPRESSION_ALGORITHM;

            // Compress body
            QByteArray compressed;
            result = compress(body, algorithm, defaultDictionarySize, compressed);
            if (result)
                return result;

            // Correct section size
            uint32ToUint24(header.size() + compressed.size(), commonHeader->Size);

            // Append header and body
            created.append(newHeader).append(compressed);

            // Parse section
            QModelIndex sectionIndex;
            result = parseSection(created, sectionIndex, index, mode);
            if (result && result != ERR_VOLUMES_NOT_FOUND && result != ERR_INVALID_VOLUME)
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
            result = compress(body, algorithm, defaultDictionarySize, compressed);
            if (result)
                return result;

            // Correct section size
            uint32ToUint24(header.size() + compressed.size(), commonHeader->Size);

            // Append header and body
            created.append(newHeader).append(compressed);

            // Parse section
            QModelIndex sectionIndex;
            result = parseSection(created, sectionIndex, index, mode);
            if (result && result != ERR_VOLUMES_NOT_FOUND && result != ERR_INVALID_VOLUME)
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
            if (result && result != ERR_VOLUMES_NOT_FOUND && result != ERR_INVALID_VOLUME)
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
                    if (model->action(currentSectionIndex) != Actions::Remove)
                        model->setAction(currentSectionIndex, Actions::Rebase);
            }
        }
    }

    // Rebase VTF in subsequent volumes.
    QModelIndex parent = index.parent();
    while (parent.isValid() && model->type(parent) != Types::Volume)
        parent = parent.parent();
    if (parent.isValid()) {
        QModelIndex volumeContainer = parent.parent();
        // Iterate over volumes starting from the one after.
        for (int i = parent.row() + 1; i < model->rowCount(volumeContainer); i++) {
            QModelIndex currentVolumeIndex = volumeContainer.child(i, 0);
            // Iterate over files within each volume after the current one.
            for (int j = 0; j < model->rowCount(currentVolumeIndex); j++) {
                QModelIndex currentFileIndex = currentVolumeIndex.child(j, 0);
                if (model->header(currentFileIndex).left(sizeof(EFI_GUID)) == EFI_FFS_VOLUME_TOP_FILE_GUID) {
                    for (int k = 0; k < model->rowCount(currentFileIndex); k++) {
                        QModelIndex currentSectionIndex = currentFileIndex.child(k, 0);
                        // If section stores PE32 or TE image
                        if (model->subtype(currentSectionIndex) == EFI_SECTION_PE32 || model->subtype(currentSectionIndex) == EFI_SECTION_TE)
                            // Set rebase action
                            if (model->action(currentSectionIndex) != Actions::Remove)
                                model->setAction(currentSectionIndex, Actions::Rebase);
                    }
                }
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
        const EFI_COMMON_SECTION_HEADER* commonHeader = (const EFI_COMMON_SECTION_HEADER*)object.constData();
        headerSize = sizeOfSectionHeader(commonHeader);
    }
    else if (model->type(parent) == Types::Section) {
        type = Types::Section;
        const EFI_COMMON_SECTION_HEADER* commonHeader = (const EFI_COMMON_SECTION_HEADER*)object.constData();
        headerSize = sizeOfSectionHeader(commonHeader);
    }
    else
        return ERR_NOT_IMPLEMENTED;

    if ((UINT32)object.size() < headerSize)
        return ERR_BUFFER_TOO_SMALL;

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
    else if (model->type(index) == Types::Padding) {
        if (mode == REPLACE_MODE_AS_IS)
            result = create(index, Types::Padding, QByteArray(), object, CREATE_MODE_AFTER, Actions::Replace);
        else
            return ERR_NOT_IMPLEMENTED;
    }
    else if (model->type(index) == Types::Volume) {
        if (mode == REPLACE_MODE_AS_IS) {
            result = create(index, Types::Volume, QByteArray(), object, CREATE_MODE_AFTER, Actions::Replace);
        }
        else if (mode == REPLACE_MODE_BODY) {
            result = create(index, Types::Volume, model->header(index), object, CREATE_MODE_AFTER, Actions::Replace);
        }
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
            const EFI_COMMON_SECTION_HEADER* commonHeader = (const EFI_COMMON_SECTION_HEADER*)object.constData();
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
        // Extract as is, with header and body
        extracted.clear();
        extracted.append(model->header(index));
        extracted.append(model->body(index));
        if (model->type(index) == Types::File) {
            UINT8 revision = 2;
            QModelIndex parent = model->parent(index);
            if (parent.isValid() && model->type(parent) == Types::Volume) {
                const EFI_FIRMWARE_VOLUME_HEADER* volumeHeader = (const EFI_FIRMWARE_VOLUME_HEADER*)model->header(parent).constData();
                revision = volumeHeader->Revision;
            }

            const EFI_FFS_FILE_HEADER* fileHeader = (const EFI_FFS_FILE_HEADER*)model->header(index).constData();
            if (revision == 1 && fileHeader->Attributes & FFS_ATTRIB_TAIL_PRESENT) {
                UINT8 ht = ~fileHeader->IntegrityCheck.Checksum.Header;
                UINT8 ft = ~fileHeader->IntegrityCheck.Checksum.File;
                extracted.append(ht).append(ft);
            }
        }
    }
    else if (mode == EXTRACT_MODE_BODY) {
        // Extract without header and tail
        extracted.clear();
        // Special case of compressed bodies
        if (model->type(index) == Types::Section) {
            QByteArray decompressed;
            UINT8 result;
            if (model->subtype(index) == EFI_SECTION_COMPRESSION) {
                const EFI_COMPRESSION_SECTION* compressedHeader = (const EFI_COMPRESSION_SECTION*)model->header(index).constData();
                result = decompress(model->body(index), compressedHeader->CompressionType, decompressed);
                if (result)
                    return result;
                extracted.append(decompressed);
                return ERR_SUCCESS;
            }
            else if (model->subtype(index) == EFI_SECTION_GUID_DEFINED) {
                QByteArray decompressed;
                // Check if section requires processing
                const EFI_GUID_DEFINED_SECTION* guidDefinedSectionHeader = (const EFI_GUID_DEFINED_SECTION*)model->header(index).constData();
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

UINT8 FfsEngine::doNotRebuild(const QModelIndex & index)
{
    if (!index.isValid())
        return ERR_INVALID_PARAMETER;

    // Set action for the item
    model->setAction(index, Actions::DoNotRebuild);

    return ERR_SUCCESS;
}

// Compression routines
UINT8 FfsEngine::decompress(const QByteArray & compressedData, const UINT8 compressionType, QByteArray & decompressedData, UINT8 * algorithm)
{
    const UINT8* data;
    UINT32 dataSize;
    UINT8* decompressed;
    UINT32 decompressedSize = 0;
    UINT8* scratch;
    UINT32 scratchSize = 0;
    const EFI_TIANO_HEADER* header;

    switch (compressionType)
    {
    case EFI_NOT_COMPRESSED:
        decompressedData = compressedData;
        if (algorithm)
            *algorithm = COMPRESSION_ALGORITHM_NONE;
        return ERR_SUCCESS;
    case EFI_STANDARD_COMPRESSION:
        // Get buffer sizes
        data = (UINT8*)compressedData.data();
        dataSize = compressedData.size();

        // Check header to be valid
        header = (const EFI_TIANO_HEADER*)data;
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

        if (decompressedSize > INT32_MAX) {
            delete[] decompressed;
            delete[] scratch;
            return ERR_STANDARD_DECOMPRESSION_FAILED;
        }

        decompressedData = QByteArray((const char*)decompressed, (int)decompressedSize);

        delete[] decompressed;
        delete[] scratch;
        return ERR_SUCCESS;
    case EFI_CUSTOMIZED_COMPRESSION:
        // Get buffer sizes
        data = (const UINT8*)compressedData.constData();
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
            if (ERR_SUCCESS != LzmaDecompress(data, dataSize, decompressed)
                || decompressedSize > INT32_MAX) {
                if (algorithm)
                    *algorithm = COMPRESSION_ALGORITHM_UNKNOWN;
                delete[] decompressed;
                return ERR_CUSTOMIZED_DECOMPRESSION_FAILED;
            }
            else {
                if (algorithm)
                    *algorithm = COMPRESSION_ALGORITHM_IMLZMA;
                decompressedData = QByteArray((const char*)decompressed, (int)decompressedSize);
            }
        }
        else {
            if (decompressedSize > INT32_MAX) {
                delete[] decompressed;
                return ERR_CUSTOMIZED_DECOMPRESSION_FAILED;
            }
            if (algorithm)
                *algorithm = COMPRESSION_ALGORITHM_LZMA;
            decompressedData = QByteArray((const char*)decompressed, (int)decompressedSize);
        }

        delete[] decompressed;
        return ERR_SUCCESS;
    default:
        msg(tr("decompress: unknown compression type %1").arg(compressionType));
        if (algorithm)
            *algorithm = COMPRESSION_ALGORITHM_UNKNOWN;
        return ERR_UNKNOWN_COMPRESSION_ALGORITHM;
    }
}

UINT8 FfsEngine::compress(const QByteArray & data, const UINT8 algorithm, const UINT32 dictionarySize, QByteArray & compressedData)
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
        // Try legacy function first
        UINT32 compressedSize = 0;
        if (EfiCompressLegacy(data.constData(), data.size(), NULL, &compressedSize) != ERR_BUFFER_TOO_SMALL)
            return ERR_STANDARD_COMPRESSION_FAILED;
        compressed = new UINT8[compressedSize];
        if (EfiCompressLegacy(data.constData(), data.size(), compressed, &compressedSize) != ERR_SUCCESS) {
            delete[] compressed;
            return ERR_STANDARD_COMPRESSION_FAILED;
        }
        compressedData = QByteArray((const char*)compressed, compressedSize);

        // Check that compressed data can be decompressed normally
        QByteArray decompressed;
        if (decompress(compressedData, EFI_STANDARD_COMPRESSION, decompressed, NULL) == ERR_SUCCESS
            && decompressed == data) {
            delete[] compressed;
            return ERR_SUCCESS;
        }
        delete[] compressed;

        // Legacy function failed, use current one
        compressedSize = 0;
        if (EfiCompress(data.constData(), data.size(), NULL, &compressedSize) != ERR_BUFFER_TOO_SMALL)
            return ERR_STANDARD_COMPRESSION_FAILED;
        compressed = new UINT8[compressedSize];
        if (EfiCompress(data.constData(), data.size(), compressed, &compressedSize) != ERR_SUCCESS) {
            delete[] compressed;
            return ERR_STANDARD_COMPRESSION_FAILED;
        }
        compressedData = QByteArray((const char*)compressed, compressedSize);

        // New functions will be trusted here, because another check will reduce performance
        delete[] compressed;
        return ERR_SUCCESS;
    }
        break;
    case COMPRESSION_ALGORITHM_TIANO:
    {
        // Try legacy function first
        UINT32 compressedSize = 0;
        if (TianoCompressLegacy(data.constData(), data.size(), NULL, &compressedSize) != ERR_BUFFER_TOO_SMALL)
            return ERR_STANDARD_COMPRESSION_FAILED;
        compressed = new UINT8[compressedSize];
        if (TianoCompressLegacy(data.constData(), data.size(), compressed, &compressedSize) != ERR_SUCCESS) {
            delete[] compressed;
            return ERR_STANDARD_COMPRESSION_FAILED;
        }
        compressedData = QByteArray((const char*)compressed, compressedSize);

        // Check that compressed data can be decompressed normally
        QByteArray decompressed;
        if (decompress(compressedData, EFI_STANDARD_COMPRESSION, decompressed, NULL) == ERR_SUCCESS
            && decompressed == data) {
            delete[] compressed;
            return ERR_SUCCESS;
        }
        delete[] compressed;

        // Legacy function failed, use current one
        compressedSize = 0;
        if (TianoCompress(data.constData(), data.size(), NULL, &compressedSize) != ERR_BUFFER_TOO_SMALL)
            return ERR_STANDARD_COMPRESSION_FAILED;
        compressed = new UINT8[compressedSize];
        if (TianoCompress(data.constData(), data.size(), compressed, &compressedSize) != ERR_SUCCESS) {
            delete[] compressed;
            return ERR_STANDARD_COMPRESSION_FAILED;
        }
        compressedData = QByteArray((const char*)compressed, compressedSize);

        // New functions will be trusted here, because another check will reduce performance
        delete[] compressed;
        return ERR_SUCCESS;
    }
        break;
    case COMPRESSION_ALGORITHM_LZMA:
    {
        UINT32 compressedSize = 0;
        if (LzmaCompress((const UINT8*)data.constData(), data.size(), NULL, &compressedSize, dictionarySize) != ERR_BUFFER_TOO_SMALL)
            return ERR_CUSTOMIZED_COMPRESSION_FAILED;
        compressed = new UINT8[compressedSize];
        if (LzmaCompress((const UINT8*)data.constData(), data.size(), compressed, &compressedSize, dictionarySize) != ERR_SUCCESS) {
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
        const EFI_COMMON_SECTION_HEADER* sectionHeader = (const EFI_COMMON_SECTION_HEADER*)header.constData();
        UINT32 headerSize = sizeOfSectionHeader(sectionHeader);
        header = data.left(headerSize);
        QByteArray newData = data.mid(headerSize);
        if (LzmaCompress((const UINT8*)newData.constData(), newData.size(), NULL, &compressedSize, dictionarySize) != ERR_BUFFER_TOO_SMALL)
            return ERR_CUSTOMIZED_COMPRESSION_FAILED;
        compressed = new UINT8[compressedSize];
        if (LzmaCompress((const UINT8*)newData.constData(), newData.size(), compressed, &compressedSize, dictionarySize) != ERR_SUCCESS) {
            delete[] compressed;
            return ERR_CUSTOMIZED_COMPRESSION_FAILED;
        }
        compressedData = header.append(QByteArray((const char*)compressed, compressedSize));
        delete[] compressed;
        return ERR_SUCCESS;
    }
        break;
    default:
        msg(tr("compress: unknown compression algorithm %1").arg(algorithm));
        return ERR_UNKNOWN_COMPRESSION_ALGORITHM;
    }
}

// Construction routines
UINT8 FfsEngine::constructPadFile(const QByteArray &guid, const UINT32 size, const UINT8 revision, const UINT8 erasePolarity, QByteArray & pad)
{
    if (size < sizeof(EFI_FFS_FILE_HEADER) || erasePolarity == ERASE_POLARITY_UNKNOWN)
        return ERR_INVALID_PARAMETER;

    if (size >= 0xFFFFFF) // TODO: large file support
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
    header->IntegrityCheck.Checksum.Header = calculateChecksum8((const UINT8*)header, sizeof(EFI_FFS_FILE_HEADER) - 1);

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
    if (model->action(index) == Actions::NoAction || model->action(index) == Actions::DoNotRebuild) {
        reconstructed = model->header(index).append(model->body(index));
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

        // Check descriptor size
        if ((UINT32)descriptor.size() < FLASH_DESCRIPTOR_SIZE) {
            msg(tr("reconstructIntelImage: descriptor is smaller than minimum size of 1000h (4096) bytes"));
            return ERR_INVALID_FLASH_DESCRIPTOR;
        }

        const FLASH_DESCRIPTOR_MAP* descriptorMap = (const FLASH_DESCRIPTOR_MAP*)(descriptor.constData() + sizeof(FLASH_DESCRIPTOR_HEADER));
        // Check sanity of base values
        if (descriptorMap->MasterBase > FLASH_DESCRIPTOR_MAX_BASE
            || descriptorMap->MasterBase == descriptorMap->RegionBase
            || descriptorMap->MasterBase == descriptorMap->ComponentBase) {
            msg(tr("reconstructIntelImage: invalid descriptor master base %1h").hexarg2(descriptorMap->MasterBase, 2));
            return ERR_INVALID_FLASH_DESCRIPTOR;
        }
        if (descriptorMap->RegionBase > FLASH_DESCRIPTOR_MAX_BASE
            || descriptorMap->RegionBase == descriptorMap->ComponentBase) {
            msg(tr("reconstructIntelImage: invalid descriptor region base %1h").hexarg2(descriptorMap->RegionBase, 2));
            return ERR_INVALID_FLASH_DESCRIPTOR;
        }
        if (descriptorMap->ComponentBase > FLASH_DESCRIPTOR_MAX_BASE) {
            msg(tr("reconstructIntelImage: invalid descriptor component base %1h").hexarg2(descriptorMap->ComponentBase, 2));
            return ERR_INVALID_FLASH_DESCRIPTOR;
        }


        const FLASH_DESCRIPTOR_REGION_SECTION* regionSection = (const FLASH_DESCRIPTOR_REGION_SECTION*)calculateAddress8((const UINT8*)descriptor.constData(), descriptorMap->RegionBase);
        QByteArray gbe;
        UINT32 gbeBegin = calculateRegionOffset(regionSection->GbeBase);
        UINT32 gbeEnd = gbeBegin + calculateRegionSize(regionSection->GbeBase, regionSection->GbeLimit);

        QByteArray me;
        UINT32 meBegin = calculateRegionOffset(regionSection->MeBase);
        UINT32 meEnd = meBegin + calculateRegionSize(regionSection->MeBase, regionSection->MeLimit);

        QByteArray bios;
        UINT32 biosBegin = calculateRegionOffset(regionSection->BiosBase);
        UINT32 biosEnd = calculateRegionSize(regionSection->BiosBase, regionSection->BiosLimit);
        // Gigabyte descriptor map
        if (biosEnd - biosBegin == (UINT32)(model->header(index).size() + model->body(index).size())) {
            biosBegin = meEnd;
            biosEnd = model->header(index).size() + model->body(index).size();
        }
        // Normal descriptor map
        else
            biosEnd += biosBegin;

        QByteArray pdr;
        UINT32 pdrBegin = calculateRegionOffset(regionSection->PdrBase);
        UINT32 pdrEnd = pdrBegin + calculateRegionSize(regionSection->PdrBase, regionSection->PdrLimit);

        QByteArray ec;
        UINT32 ecBegin = 0;
        UINT32 ecEnd = 0;

        // Check for legacy descriptor version by getting hardcoded value of FlashParameters.ReadClockFrequency
        UINT8 descriptorVersion = 2; // Skylake+ descriptor
        const FLASH_DESCRIPTOR_COMPONENT_SECTION* componentSection = (const FLASH_DESCRIPTOR_COMPONENT_SECTION*)calculateAddress8((const UINT8*)descriptor.constData(), descriptorMap->ComponentBase);
        if (componentSection->FlashParameters.ReadClockFrequency == FLASH_FREQUENCY_20MHZ) // Legacy descriptor
            descriptorVersion = 1;

        if (descriptorVersion == 2 && descriptorMap->DescriptorVersion != FLASH_DESCRIPTOR_VERSION_INVALID) {
            // Warn about incompatible version descriptor in case it is different.
            const FLASH_DESCRIPTOR_VERSION *version = (const FLASH_DESCRIPTOR_VERSION *)&descriptorMap->DescriptorVersion;
            if (version->Major != FLASH_DESCRIPTOR_VERSION_MAJOR || version->Minor != FLASH_DESCRIPTOR_VERSION_MINOR)
                msg(tr("reconstructIntelImage: discovered unexpected %1.%2 descriptor version, trying to continue...")
                    .arg(version->Major).arg(version->Minor));
        }

        if (descriptorVersion == 2) {
            ecBegin = calculateRegionOffset(regionSection->EcBase);
            ecEnd = ecBegin + calculateRegionSize(regionSection->EcBase, regionSection->EcLimit);
        }

        UINT32 offset = descriptor.size();
        // Reconstruct other regions
        char empty = '\xFF';
        for (int i = 1; i < model->rowCount(index); i++) {
            QByteArray region;

            // Padding after the end of all Intel regions
            if (model->type(index.child(i, 0)) == Types::Padding) {
                region = model->body(index.child(i, 0));
                reconstructed.append(region);
                offset += region.size();
                continue;
            }

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
            case Subtypes::EcRegion:
                if (descriptorVersion == 1) {
                    msg(tr("reconstructIntelImage: incompatible region type found"), index);
                    return ERR_INVALID_REGION;
                }
                ec = region;
                if (ecBegin > offset)
                    reconstructed.append(QByteArray(ecBegin - offset, empty));
                reconstructed.append(ec);
                offset = ecEnd;
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
            msg(tr("reconstructIntelImage: reconstructed body size %1h (%2) is bigger then original %3h (%4) ")
                .hexarg(reconstructed.size()).arg(reconstructed.size())
                .hexarg(model->body(index).size()).arg(model->body(index).size()),
                index);
            return ERR_INVALID_PARAMETER;
        }
        else if (reconstructed.size() < model->body(index).size()) {
            msg(tr("reconstructIntelImage: reconstructed body size %1h (%2) is smaller then original %3h (%4) ")
                .hexarg(reconstructed.size()).arg(reconstructed.size())
                .hexarg(model->body(index).size()).arg(model->body(index).size()),
                index);
            return ERR_INVALID_PARAMETER;
        }

        // Reconstruction successful
        return ERR_SUCCESS;
    }

    // All other actions are not supported
    return ERR_NOT_IMPLEMENTED;
}

UINT8 FfsEngine::reconstructRegion(const QModelIndex& index, QByteArray& reconstructed, bool includeHeader)
{
    if (!index.isValid())
        return ERR_SUCCESS;

    UINT8 result;

    // No action
    if (model->action(index) == Actions::NoAction || model->action(index) == Actions::DoNotRebuild) {
        reconstructed = model->header(index).append(model->body(index));
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
            msg(tr("reconstructRegion: reconstructed region size %1h (%2) is bigger then original %3h (%4)")
                .hexarg(reconstructed.size()).arg(reconstructed.size())
                .hexarg(model->body(index).size()).arg(model->body(index).size()),
                index);
            return ERR_INVALID_PARAMETER;
        }
        else if (reconstructed.size() < model->body(index).size()) {
            msg(tr("reconstructRegion: reconstructed region size %1h (%2) is smaller then original %3h (%4)")
                .hexarg(reconstructed.size()).arg(reconstructed.size())
                .hexarg(model->body(index).size()).arg(model->body(index).size()),
                index);
            return ERR_INVALID_PARAMETER;
        }

        // Reconstruction successful
        if (includeHeader)
            reconstructed = model->header(index).append(reconstructed);
        return ERR_SUCCESS;
    }

    // All other actions are not supported
    return ERR_NOT_IMPLEMENTED;
}

UINT8 FfsEngine::reconstructPadding(const QModelIndex& index, QByteArray& reconstructed)
{
    if (!index.isValid())
        return ERR_SUCCESS;

    // No action
    if (model->action(index) == Actions::NoAction || model->action(index) == Actions::DoNotRebuild) {
        reconstructed = model->body(index);
        return ERR_SUCCESS;
    }
    else if (model->action(index) == Actions::Remove) {
        reconstructed.clear();
        return ERR_SUCCESS;
    }
    else if (model->action(index) == Actions::Rebuild ||
        model->action(index) == Actions::Replace) {
        // Use stored item body
        reconstructed = model->body(index);

        // Check size of reconstructed region, it must be same
        if (reconstructed.size() > model->body(index).size()) {
            msg(tr("reconstructPadding: reconstructed padding size %1h (%2) is bigger then original %3h (%4)")
                .hexarg(reconstructed.size()).arg(reconstructed.size())
                .hexarg(model->body(index).size()).arg(model->body(index).size()),
                index);
            return ERR_INVALID_PARAMETER;
        }
        else if (reconstructed.size() < model->body(index).size()) {
            msg(tr("reconstructPadding: reconstructed padding size %1h (%2) is smaller then original %3h (%4)")
                .hexarg(reconstructed.size()).arg(reconstructed.size())
                .hexarg(model->body(index).size()).arg(model->body(index).size()),
                index);
            return ERR_INVALID_PARAMETER;
        }

        // Reconstruction successful
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
    if (model->action(index) == Actions::NoAction || model->action(index) == Actions::DoNotRebuild) {
        reconstructed = model->header(index).append(model->body(index));
        return ERR_SUCCESS;
    }
    else if (model->action(index) == Actions::Remove) {
        reconstructed.clear();
        return ERR_SUCCESS;
    }
    else if (model->action(index) == Actions::Replace ||
        model->action(index) == Actions::Rebuild) {
        QByteArray header = model->header(index);
        QByteArray body = model->body(index);
        EFI_FIRMWARE_VOLUME_HEADER* volumeHeader = (EFI_FIRMWARE_VOLUME_HEADER*)header.data();

        // Check sanity of HeaderLength
        if (volumeHeader->HeaderLength > header.size()) {
            msg(tr("reconstructVolume: invalid volume header length, reconstruction is not possible"), index);
            return ERR_INVALID_VOLUME;
        }

        // Recalculate volume header checksum
        volumeHeader->Checksum = 0;
        volumeHeader->Checksum = calculateChecksum16((const UINT16*)volumeHeader, volumeHeader->HeaderLength);

        // Get volume size
        UINT32 volumeSize = header.size() + body.size();

        // Reconstruct volume body
        UINT32 freeSpaceOffset = 0;
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
                        // BUGBUG: this parsing is bad and doesn't support large files, but it needs to be performed only for very old images with uncompressed DXE volumes, so whatever
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
                                UINT32 imagebase = 0;
                                result = getBase(model->body(image), imagebase); // imagebase passed by reference
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
                    fileOffset += model->header(index.child(i, 0)).size() + model->body(index.child(i, 0)).size();
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
            UINT32 nonUefiDataOffset = 0;
            QByteArray nonUefiData;
            for (int i = 0; i < model->rowCount(index); i++) {
                // Inside a volume can be files, free space or padding with non-UEFI data
                if (model->type(index.child(i, 0)) == Types::File) { // Next item is a file

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
                    UINT32 fileHeaderSize = sizeof(EFI_FFS_FILE_HEADER);
                    if (volumeHeader->Revision > 1 && (fileHeader->Attributes & FFS_ATTRIB_LARGE_FILE))
                        fileHeaderSize = sizeof(EFI_FFS_FILE_HEADER2);

                    // Pad file
                    if (fileHeader->Type == EFI_FV_FILETYPE_PAD) {
                        padFileGuid = file.left(sizeof(EFI_GUID));

                        if (model->action(index.child(i, 0)) == Actions::DoNotRebuild) {
                            // User asked not to touch this file, do nothing here
                        }
                        // Parse non-empty pad file
                        else if (model->rowCount(index.child(i, 0))) {
                            // TODO: handle this special case
                            continue;
                        }
                        // Skip empty pad-file
                        else {
                            continue;
                        }
                    }

                    // Volume Top File
                    if (file.left(sizeof(EFI_GUID)) == EFI_FFS_VOLUME_TOP_FILE_GUID) {
                        vtf = file;
                        vtfIndex = index.child(i, 0);
                        continue;
                    }

                    // Normal file
                    // Ensure correct alignment
                    UINT8 alignmentPower = ffsAlignmentTable[(fileHeader->Attributes & FFS_ATTRIB_DATA_ALIGNMENT) >> 3];
                    alignment = (UINT32)(1UL <<alignmentPower);
                    UINT32 alignmentBase = header.size() + offset + fileHeaderSize;
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
                else if (model->type(index.child(i, 0)) == Types::FreeSpace) { //Next item is a free space
                    // Some data are located beyond free space
                    if (offset + (UINT32)model->body(index.child(i, 0)).size() < (UINT32)model->body(index).size()) {
                        // Get non-UEFI data and it's offset
                        nonUefiData = model->body(index.child(i + 1, 0));
                        nonUefiDataOffset = body.size() - nonUefiData.size();
                        break;
                    }
                }
            }

            // Check volume sanity
            if (!vtf.isEmpty() && !nonUefiData.isEmpty()) {
                msg(tr("reconstructVolume: both VTF and non-UEFI data found in the volume, reconstruction is not possible"), index);
                return ERR_INVALID_VOLUME;
            }

            // Check for free space offset in ZeroVector
            if (model->text(index).contains("AppleFSO ")) {
                // Align current offset to 8 byte boundary
                UINT32 alignment = offset % 8;
                freeSpaceOffset = model->header(index).size() + offset;
                if (alignment) {
                    alignment = 8 - alignment;
                    freeSpaceOffset += alignment;
                }
            }

            // Insert VTF or non-UEFI data to it's correct place
            if (!vtf.isEmpty()) { // VTF found
                // Determine correct VTF offset
                UINT32 vtfOffset = model->body(index).size() - vtf.size();

                if (vtfOffset % 8) {
                    msg(tr("reconstructVolume: wrong size of the Volume Top File"), index);
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
                else if (offset > vtfOffset) {
                    msg(tr("reconstructVolume: no space left to insert VTF, need %1h (%2) byte(s) more")
                        .hexarg(offset - vtfOffset).arg(offset - vtfOffset), index);
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
            else if (!nonUefiData.isEmpty()) { //Non-UEFI data found
                // No space left
                if (offset > nonUefiDataOffset) {
                    msg(tr("reconstructVolume: no space left to insert non-UEFI data, need %1h (%2) byte(s) more")
                        .hexarg(offset - nonUefiDataOffset).arg(offset - nonUefiDataOffset), index);
                    return ERR_INVALID_VOLUME;
                }
                // Append additional free space
                else if (nonUefiDataOffset > offset) {
                    reconstructed.append(QByteArray(nonUefiDataOffset - offset, empty));
                }

                // Append VTF
                reconstructed.append(nonUefiData);
            }
            else {
                // Fill the rest of volume space with empty char
                if (body.size() > reconstructed.size()) {
                    // Fill volume end with empty char
                    reconstructed.append(QByteArray(body.size() - reconstructed.size(), empty));
                }
                else if (body.size() < reconstructed.size()) {
                    // Check if volume can be grown
                    // Root volume can't be grown
                    UINT8 parentType = model->type(index.parent());
                    if (parentType != Types::File && parentType != Types::Section) {
                        msg(tr("reconstructVolume: root volume can't be grown"), index);
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
        }
        // Use current volume body
        else {
            reconstructed = model->body(index);

            // BUGBUG: volume size may change during this operation for volumes withour files in them
            // but such volumes are fairly rare
        }

        // Check new volume size
        if ((UINT32)(header.size() + reconstructed.size()) != volumeSize) {
            msg(tr("reconstructVolume: volume size can't be changed"), index);
            return ERR_INVALID_VOLUME;
        }

        // Reconstruction successful
        reconstructed = header.append(reconstructed);

        // Recalculate CRC32 in ZeroVector, if needed
        if (model->text(index).contains("AppleCRC32 ")) {
            // Get current CRC32 value from volume header
            const UINT32 currentCrc = *(const UINT32*)(reconstructed.constData() + 8);
            // Calculate new value
            UINT32 crc = crc32(0, (const UINT8*)reconstructed.constData() + volumeHeader->HeaderLength, reconstructed.size() - volumeHeader->HeaderLength);
            // Update the value
            if (currentCrc != crc) {
                *(UINT32*)(reconstructed.data() + 8) = crc;

                // Recalculate header checksum
                volumeHeader = (EFI_FIRMWARE_VOLUME_HEADER*)reconstructed.data();
                volumeHeader->Checksum = 0;
                volumeHeader->Checksum = calculateChecksum16((const UINT16*)volumeHeader, volumeHeader->HeaderLength);
            }
        }

        // Store new free space offset, if needed
        if (model->text(index).contains("AppleFSO ")) {
            // Get current CRC32 value from volume header
            const UINT32 currentFso = *(const UINT32*)(reconstructed.constData() + 12);
            // Update the value
            if (freeSpaceOffset != 0 && currentFso != freeSpaceOffset) {
                *(UINT32*)(reconstructed.data() + 12) = freeSpaceOffset;

                // Recalculate header checksum
                volumeHeader = (EFI_FIRMWARE_VOLUME_HEADER*)reconstructed.data();
                volumeHeader->Checksum = 0;
                volumeHeader->Checksum = calculateChecksum16((const UINT16*)volumeHeader, volumeHeader->HeaderLength);
            }
        }

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
    if (model->action(index) == Actions::NoAction || model->action(index) == Actions::DoNotRebuild) {
        reconstructed = model->header(index).append(model->body(index));
        const EFI_FFS_FILE_HEADER* fileHeader = (const EFI_FFS_FILE_HEADER*)model->header(index).constData();
        // Append tail, if needed
        if (fileHeader->Attributes & FFS_ATTRIB_TAIL_PRESENT) {
            UINT8 ht = ~fileHeader->IntegrityCheck.Checksum.Header;
            UINT8 ft = ~fileHeader->IntegrityCheck.Checksum.File;
            reconstructed.append(ht).append(ft);
        }
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
            msg(tr("reconstructFile: unknown erase polarity"), index);
            return ERR_INVALID_PARAMETER;
        }

        // Check file state
        // Check top reserved bit of file state to determine it's original erase polarity
        UINT8 state = fileHeader->State;
        if (state & EFI_FILE_ERASE_POLARITY)
            state = ~state;

        // Order of this checks must be preserved
        // Check file to have valid state, or delete it otherwise
        if (state & EFI_FILE_HEADER_INVALID) {
            // File marked to have invalid header and must be deleted
            // Do not add anything to queue
            msg(tr("reconstructFile: file is HEADER_INVALID state, and will be removed from reconstructed image"), index);
            return ERR_SUCCESS;
        }
        else if (state & EFI_FILE_DELETED) {
            // File marked to have been deleted form and must be deleted
            // Do not add anything to queue
            msg(tr("reconstructFile: file is in DELETED state, and will be removed from reconstructed image"), index);
            return ERR_SUCCESS;
        }
        else if (state & EFI_FILE_MARKED_FOR_UPDATE) {
            // File is marked for update, the mark must be removed
            msg(tr("reconstructFile: file's MARKED_FOR_UPDATE state cleared"), index);
        }
        else if (state & EFI_FILE_DATA_VALID) {
            // File is in good condition, reconstruct it
        }
        else if (state & EFI_FILE_HEADER_VALID) {
            // Header is valid, but data is not, so file must be deleted
            msg(tr("reconstructFile: file is in HEADER_VALID (but not in DATA_VALID) state, and will be removed from reconstructed image"), index);
            return ERR_SUCCESS;
        }
        else if (state & EFI_FILE_HEADER_CONSTRUCTION) {
            // Header construction not finished, so file must be deleted
            msg(tr("reconstructFile: file is in HEADER_CONSTRUCTION (but not in DATA_VALID) state, and will be removed from reconstructed image"), index);
            return ERR_SUCCESS;
        }

        // Reconstruct file body
        if (model->rowCount(index)) {
            reconstructed.clear();
            // Construct new file body
            // File contains raw data, must be parsed as region without header
            if (model->subtype(index) == EFI_FV_FILETYPE_ALL || model->subtype(index) == EFI_FV_FILETYPE_RAW) {
                result = reconstructRegion(index, reconstructed, false);
                if (result)
                    return result;
            }
            // File contains sections
            else {
                UINT32 offset = 0;
                UINT32 headerSize = sizeof(EFI_FFS_FILE_HEADER);
                if (revision > 1 && (fileHeader->Attributes & FFS_ATTRIB_LARGE_FILE)) {
                    headerSize = sizeof(EFI_FFS_FILE_HEADER2);
                }

                for (int i = 0; i < model->rowCount(index); i++) {
                    // Align to 4 byte boundary
                    UINT8 alignment = offset % 4;
                    if (alignment) {
                        alignment = 4 - alignment;
                        offset += alignment;
                        reconstructed.append(QByteArray(alignment, '\x00'));
                    }

                    // Calculate section base
                    UINT32 sectionBase = base ? base + headerSize + offset : 0;
                    UINT8 alignmentPower = ffsAlignmentTable[(fileHeader->Attributes & FFS_ATTRIB_DATA_ALIGNMENT) >> 3];
                    UINT32 fileAlignment = (UINT32)(1UL << alignmentPower);
                    UINT32 alignmentBase = base + headerSize;
                    if (alignmentBase % fileAlignment) {
                        // File will be unaligned if added as is, so we must add pad file before it
                        // Determine pad file size
                        UINT32 size = fileAlignment - (alignmentBase % fileAlignment);
                        // Required padding is smaller then minimal pad file size
                        while (size < sizeof(EFI_FFS_FILE_HEADER)) {
                            size += fileAlignment;
                        }
                        // Adjust file base to incorporate pad file that will be added to align it
                        sectionBase += size;
                    }
                    
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
        }
        // Use current file body
        else
            reconstructed = model->body(index);

        // Correct file size
        UINT8 tailSize = (revision == 1 && (fileHeader->Attributes & FFS_ATTRIB_TAIL_PRESENT)) ? sizeof(UINT16) : 0;
        if (revision > 1 && (fileHeader->Attributes & FFS_ATTRIB_LARGE_FILE)) {
            uint32ToUint24(EFI_SECTION2_IS_USED, fileHeader->Size);
            EFI_FFS_FILE_HEADER2* fileHeader2 = (EFI_FFS_FILE_HEADER2*)fileHeader;
            fileHeader2->ExtendedSize = sizeof(EFI_FFS_FILE_HEADER2) + reconstructed.size() + tailSize;
        }
        else {
            if (sizeof(EFI_FFS_FILE_HEADER) + reconstructed.size() + tailSize > 0xFFFFFF) {
                msg(tr("reconstructFile: resulting file size is too big"), index);
                return ERR_INVALID_FILE;
            }
            uint32ToUint24(sizeof(EFI_FFS_FILE_HEADER) + reconstructed.size() + tailSize, fileHeader->Size);
        }

        // Recalculate header checksum
        fileHeader->IntegrityCheck.Checksum.Header = 0;
        fileHeader->IntegrityCheck.Checksum.File = 0;
        fileHeader->IntegrityCheck.Checksum.Header = 0x100 - (calculateSum8((const UINT8*)header.constData(), header.size()) - fileHeader->State);

        // Recalculate data checksum, if needed
        if (fileHeader->Attributes & FFS_ATTRIB_CHECKSUM) {
            fileHeader->IntegrityCheck.Checksum.File = calculateChecksum8((const UINT8*)reconstructed.constData(), reconstructed.size());
        }
        else if (revision == 1)
            fileHeader->IntegrityCheck.Checksum.File = FFS_FIXED_CHECKSUM;
        else
            fileHeader->IntegrityCheck.Checksum.File = FFS_FIXED_CHECKSUM2;

        // Append tail, if needed
        if (revision == 1 && fileHeader->Attributes & FFS_ATTRIB_TAIL_PRESENT) {
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
    if (model->action(index) == Actions::NoAction || model->action(index) == Actions::DoNotRebuild) {
        reconstructed = model->header(index).append(model->body(index));
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
        bool extended = false;
        if (uint24ToUint32(commonHeader->Size) == 0xFFFFFF) {
            extended = true;
        }

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

                // Correct compression type
                if (model->compression(index) == COMPRESSION_ALGORITHM_NONE) {
                    compessionHeader->CompressionType = EFI_NOT_COMPRESSED;
                }
                else if (model->compression(index) == COMPRESSION_ALGORITHM_EFI11 || model->compression(index) == COMPRESSION_ALGORITHM_TIANO) {
                    compessionHeader->CompressionType = EFI_STANDARD_COMPRESSION;
                }
                else if (model->compression(index) == COMPRESSION_ALGORITHM_LZMA) {
                    compessionHeader->CompressionType = EFI_CUSTOMIZED_COMPRESSION;
                }
                else if (model->compression(index) == COMPRESSION_ALGORITHM_IMLZMA) {
                    compessionHeader->CompressionType = EFI_CUSTOMIZED_COMPRESSION;
                }
                else
                    return ERR_UNKNOWN_COMPRESSION_ALGORITHM;

                // Compress new section body
                QByteArray compressed;
                result = compress(reconstructed, model->compression(index), model->dictionarySize(index), compressed);
                if (result)
                    return result;

                // Replace new section body
                reconstructed = compressed;
            }
            else if (model->subtype(index) == EFI_SECTION_GUID_DEFINED) {
                EFI_GUID_DEFINED_SECTION* guidDefinedHeader = (EFI_GUID_DEFINED_SECTION*)header.data();
                // Convert x86
                if (QByteArray((const char*)&guidDefinedHeader->SectionDefinitionGuid, sizeof(EFI_GUID)) == EFI_GUIDED_SECTION_LZMAF86) {
                    result = x86Convert(reconstructed, 1);
                    if (result)
                        return result;
                }
                // Compress new section body
                QByteArray compressed;
                result = compress(reconstructed, model->compression(index), model->dictionarySize(index), compressed);
                if (result)
                    return result;
                // Check for authentication status valid attribute
                if (guidDefinedHeader->Attributes & EFI_GUIDED_SECTION_AUTH_STATUS_VALID) {
                    // CRC32 section
                    if (QByteArray((const char*)&guidDefinedHeader->SectionDefinitionGuid, sizeof(EFI_GUID)) == EFI_GUIDED_SECTION_CRC32) {
                        // Check header size
                        if ((UINT32)header.size() != sizeof(EFI_GUID_DEFINED_SECTION) + sizeof(UINT32)) {
                            msg(tr("reconstructSection: invalid CRC32 section size %1h (%2)")
                                .hexarg(header.size()).arg(header.size()), index);
                            return ERR_INVALID_SECTION;
                        }
                        // Calculate CRC32 of section data
                        UINT32 crc = crc32(0, (const UINT8*)compressed.constData(), compressed.size());
                        // Store new CRC32
                        *(UINT32*)(header.data() + sizeof(EFI_GUID_DEFINED_SECTION)) = crc;
                    }
                    else {
                        msg(tr("reconstructSection: GUID defined section authentication info can become invalid")
                            .arg(guidToQString(guidDefinedHeader->SectionDefinitionGuid)), index);
                    }
                }
                // Check for Intel signed section
                if (guidDefinedHeader->Attributes & EFI_GUIDED_SECTION_PROCESSING_REQUIRED
                    && QByteArray((const char*)&guidDefinedHeader->SectionDefinitionGuid, sizeof(EFI_GUID)) == EFI_FIRMWARE_CONTENTS_SIGNED_GUID) {
                    msg(tr("reconstructSection: GUID defined section signature can become invalid")
                        .arg(guidToQString(guidDefinedHeader->SectionDefinitionGuid)), index);
                }
                // Replace new section body
                reconstructed = compressed;
            }
            else if (model->compression(index) != COMPRESSION_ALGORITHM_NONE) {
                msg(tr("reconstructSection: incorrectly required compression for section of type %1")
                    .arg(model->subtype(index)), index);
                return ERR_INVALID_SECTION;
            }
        }
        // Leaf section
        else {
            reconstructed = model->body(index);
        }

        // Correct section size
        if (extended) {
            EFI_COMMON_SECTION_HEADER2 * extHeader = (EFI_COMMON_SECTION_HEADER2*)commonHeader;
            extHeader->ExtendedSize = header.size() + reconstructed.size();
            uint32ToUint24(0xFFFFFF, commonHeader->Size);
        }
        else {
            uint32ToUint24(header.size() + reconstructed.size(), commonHeader->Size);
        }

        // Rebase PE32 or TE image, if needed
        if ((model->subtype(index) == EFI_SECTION_PE32 || model->subtype(index) == EFI_SECTION_TE) &&
            (model->subtype(index.parent()) == EFI_FV_FILETYPE_PEI_CORE ||
                model->subtype(index.parent()) == EFI_FV_FILETYPE_PEIM ||
                model->subtype(index.parent()) == EFI_FV_FILETYPE_COMBINED_PEIM_DRIVER)) {
            UINT16 teFixup = 0;

            // Most EFI images today include teFixup in ImageBase value,
            // which doesn't follow the UEFI spec, but is so popular that
            // only a few images out of thousands are different

            // There are some heuristics possible here to detect if an entry point is calculated correctly
            // or needs a proper fixup, but new_engine already have them and it's better to work on proper
            // builder for it than trying to fix this mess

            //if (model->subtype(index) == EFI_SECTION_TE) {
            //  const EFI_IMAGE_TE_HEADER* teHeader = (const EFI_IMAGE_TE_HEADER*)model->body(index).constData();
            //  teFixup = teHeader->StrippedSize - sizeof(EFI_IMAGE_TE_HEADER);
            //

            if (base) {
                result = rebase(reconstructed, base - teFixup + header.size(), index);
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
        result = reconstructPadding(index, reconstructed);
        if (result)
            return result;
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
        msg(tr("reconstruct: unknown item type %1").arg(model->type(index)), index);
        return ERR_UNKNOWN_ITEM_TYPE;
    }

    return ERR_SUCCESS;
}

UINT8 FfsEngine::growVolume(QByteArray & header, const UINT32 size, UINT32 & newSize)
{
    // Check sanity
    if ((UINT32)header.size() < sizeof(EFI_FIRMWARE_VOLUME_HEADER))
        return ERR_INVALID_VOLUME;

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
    volumeHeader->Checksum = calculateChecksum16((const UINT16*)volumeHeader, volumeHeader->HeaderLength);

    return ERR_SUCCESS;
}

UINT8 FfsEngine::reconstructImageFile(QByteArray & reconstructed)
{
    return reconstruct(model->index(0, 0), reconstructed);
}

// Search routines
UINT8 FfsEngine::findHexPattern(const QModelIndex & index, const QByteArray & hexPattern, const UINT8 mode)
{
    if (!index.isValid())
        return ERR_SUCCESS;

    if (hexPattern.isEmpty())
        return ERR_INVALID_PARAMETER;

    // Check for "all substrings" pattern
    if (hexPattern.count('.') == hexPattern.length())
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
            data.append(model->header(index));
        else if (mode == SEARCH_MODE_BODY)
            data.append(model->body(index));
        else
            data.append(model->header(index)).append(model->body(index));
    }

    QString hexBody = QString(data.toHex());
    QRegExp regexp = QRegExp(QString(hexPattern), Qt::CaseInsensitive);
    INT32 offset = regexp.indexIn(hexBody);
    while (offset >= 0) {
        if (offset % 2 == 0) {
            msg(tr("Hex pattern \"%1\" found as \"%2\" in %3 at %4-offset %5h")
                .arg(QString(hexPattern))
                .arg(hexBody.mid(offset, hexPattern.length()).toUpper())
                .arg(model->name(index))
                .arg(mode == SEARCH_MODE_BODY ? tr("body") : tr("header"))
                .hexarg(offset / 2),
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
            data.append(model->header(index));
        else if (mode == SEARCH_MODE_BODY)
            data.append(model->body(index));
        else
            data.append(model->header(index)).append(model->body(index));
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
            msg(tr("GUID pattern \"%1\" found as \"%2\" in %3 at %4-offset %5h")
                .arg(QString(guidPattern))
                .arg(hexBody.mid(offset, hexPattern.length()).toUpper())
                .arg(model->name(index))
                .arg(mode == SEARCH_MODE_BODY ? tr("body") : tr("header"))
                .hexarg(offset / 2),
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
        msg(tr("%1 text \"%2\" found in %3 at offset %4h")
            .arg(unicode ? "Unicode" : "ASCII")
            .arg(pattern)
            .arg(model->name(index))
            .hexarg(unicode ? offset * 2 : offset),
            index);
    }

    return ERR_SUCCESS;
}

UINT8 FfsEngine::rebase(QByteArray &executable, const UINT32 base, const QModelIndex & index)
{
    UINT32 delta;       // Difference between old and new base addresses
    UINT32 relocOffset; // Offset of relocation region
    UINT32 relocSize;   // Size of relocation region
    UINT32 teFixup = 0; // Bytes removed form PE header for TE images

    // Copy input data to local storage
    QByteArray file = executable;

    // Populate DOS header
    if ((UINT32)file.size() < sizeof(EFI_IMAGE_DOS_HEADER))
        return ERR_INVALID_FILE;
    EFI_IMAGE_DOS_HEADER* dosHeader = (EFI_IMAGE_DOS_HEADER*)file.data();

    // Check signature
    if (dosHeader->e_magic == EFI_IMAGE_DOS_SIGNATURE){
        UINT32 offset = dosHeader->e_lfanew;
        if ((UINT32)file.size() < offset + sizeof(EFI_IMAGE_PE_HEADER))
            return ERR_UNKNOWN_IMAGE_TYPE;
        EFI_IMAGE_PE_HEADER* peHeader = (EFI_IMAGE_PE_HEADER*)(file.data() + offset);
        if (peHeader->Signature != EFI_IMAGE_PE_SIGNATURE)
            return ERR_UNKNOWN_IMAGE_TYPE;
        offset += sizeof(EFI_IMAGE_PE_HEADER);
        // Skip file header
        offset += sizeof(EFI_IMAGE_FILE_HEADER);
        // Check optional header magic
        if ((UINT32)file.size() < offset + sizeof(UINT16))
            return ERR_UNKNOWN_IMAGE_TYPE;
        UINT16 magic = *(UINT16*)(file.data() + offset);
        if (magic == EFI_IMAGE_PE_OPTIONAL_HDR32_MAGIC) {
            if ((UINT32)file.size() < offset + sizeof(EFI_IMAGE_OPTIONAL_HEADER32))
                return ERR_UNKNOWN_PE_OPTIONAL_HEADER_TYPE;
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
            if ((UINT32)file.size() < offset + sizeof(EFI_IMAGE_OPTIONAL_HEADER64))
                return ERR_UNKNOWN_PE_OPTIONAL_HEADER_TYPE;
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
        if ((UINT32)file.size() < sizeof(EFI_IMAGE_TE_HEADER))
            return ERR_INVALID_FILE;
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

        // Warn the user about possible outcome of incorrect rebase of TE image
        msg(tr("rebase: can't determine if TE image base is adjusted or not, rebased TE image may stop working"), index);
    }
    else
        return ERR_UNKNOWN_IMAGE_TYPE;

    // No relocations
    if (relocOffset == 0) {
        // No need to fix relocations
        executable = file;
        return ERR_SUCCESS;
    }

    EFI_IMAGE_BASE_RELOCATION *RelocBase;
    EFI_IMAGE_BASE_RELOCATION *RelocBaseEnd;
    UINT16                    *Reloc;
    UINT16                    *RelocEnd;
    UINT16                    *F16;
    UINT32                    *F32;
    UINT64                    *F64;

    // Run the whole relocation block
    RelocBase = (EFI_IMAGE_BASE_RELOCATION*)(file.data() + relocOffset - teFixup);
    RelocBaseEnd = (EFI_IMAGE_BASE_RELOCATION*)(file.data() + relocOffset - teFixup + relocSize);

    while (RelocBase < RelocBaseEnd) {
        Reloc = (UINT16*)((UINT8*)RelocBase + sizeof(EFI_IMAGE_BASE_RELOCATION));
        RelocEnd = (UINT16*)((UINT8*)RelocBase + RelocBase->SizeOfBlock);

        // Run this relocation record
        while (Reloc < RelocEnd) {
            if (*Reloc == 0x0000) {
                // Skip last emtpy reloc entry
                Reloc += 1;
                continue;
            }

            UINT32 RelocLocation = RelocBase->VirtualAddress - teFixup + (*Reloc & 0x0FFF);
            if ((UINT32)file.size() < RelocLocation)
                return ERR_BAD_RELOCATION_ENTRY;
            UINT8* data = (UINT8*)(file.data() + RelocLocation);
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
        msg(tr("patchVtf: PEI Core entry point can't be determined. VTF can't be patched."));
        return ERR_PEI_CORE_ENTRY_POINT_NOT_FOUND;
    }

    if (!newPeiCoreEntryPoint || oldPeiCoreEntryPoint == newPeiCoreEntryPoint)
        // No need to patch anything
        return ERR_SUCCESS;

    // Replace last occurrence of oldPeiCoreEntryPoint with newPeiCoreEntryPoint
    QByteArray old((char*)&oldPeiCoreEntryPoint, sizeof(oldPeiCoreEntryPoint));
    int i = vtf.lastIndexOf(old);
    if (i == -1) {
        msg(tr("patchVtf: PEI Core entry point can't be found in VTF. VTF not patched."));
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
    if ((UINT32)file.size() < sizeof(EFI_IMAGE_DOS_HEADER))
        return ERR_INVALID_FILE;
    const EFI_IMAGE_DOS_HEADER* dosHeader = (const EFI_IMAGE_DOS_HEADER*)file.constData();

    // Check signature
    if (dosHeader->e_magic == EFI_IMAGE_DOS_SIGNATURE){
        UINT32 offset = dosHeader->e_lfanew;
        if ((UINT32)file.size() < offset + sizeof(EFI_IMAGE_PE_HEADER))
            return ERR_UNKNOWN_IMAGE_TYPE;
        const EFI_IMAGE_PE_HEADER* peHeader = (const EFI_IMAGE_PE_HEADER*)(file.constData() + offset);
        if (peHeader->Signature != EFI_IMAGE_PE_SIGNATURE)
            return ERR_UNKNOWN_IMAGE_TYPE;
        offset += sizeof(EFI_IMAGE_PE_HEADER);

        // Skip file header
        offset += sizeof(EFI_IMAGE_FILE_HEADER);

        // Check optional header magic
        const UINT16 magic = *(const UINT16*)(file.constData() + offset);
        if (magic == EFI_IMAGE_PE_OPTIONAL_HDR32_MAGIC) {
            if ((UINT32)file.size() < offset + sizeof(EFI_IMAGE_OPTIONAL_HEADER32))
                return ERR_UNKNOWN_PE_OPTIONAL_HEADER_TYPE;
            const EFI_IMAGE_OPTIONAL_HEADER32* optHeader = (const EFI_IMAGE_OPTIONAL_HEADER32*)(file.constData() + offset);
            entryPoint = optHeader->ImageBase + optHeader->AddressOfEntryPoint;
        }
        else if (magic == EFI_IMAGE_PE_OPTIONAL_HDR64_MAGIC) {
            if ((UINT32)file.size() < offset + sizeof(EFI_IMAGE_OPTIONAL_HEADER64))
                return ERR_UNKNOWN_PE_OPTIONAL_HEADER_TYPE;
            const EFI_IMAGE_OPTIONAL_HEADER64* optHeader = (const EFI_IMAGE_OPTIONAL_HEADER64*)(file.constData() + offset);
            entryPoint = optHeader->ImageBase + optHeader->AddressOfEntryPoint;
        }
        else
            return ERR_UNKNOWN_PE_OPTIONAL_HEADER_TYPE;
    }
    else if (dosHeader->e_magic == EFI_IMAGE_TE_SIGNATURE){
        // Populate TE header
        if ((UINT32)file.size() < sizeof(EFI_IMAGE_TE_HEADER))
            return ERR_INVALID_FILE;
        const EFI_IMAGE_TE_HEADER* teHeader = (const EFI_IMAGE_TE_HEADER*)file.constData();
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
    if ((UINT32)file.size() < sizeof(EFI_IMAGE_DOS_HEADER))
        return ERR_INVALID_FILE;
    const EFI_IMAGE_DOS_HEADER* dosHeader = (const EFI_IMAGE_DOS_HEADER*)file.constData();

    // Check signature
    if (dosHeader->e_magic == EFI_IMAGE_DOS_SIGNATURE){
        UINT32 offset = dosHeader->e_lfanew;
        if ((UINT32)file.size() < offset + sizeof(EFI_IMAGE_PE_HEADER))
            return ERR_UNKNOWN_IMAGE_TYPE;
        const EFI_IMAGE_PE_HEADER* peHeader = (const EFI_IMAGE_PE_HEADER*)(file.constData() + offset);
        if (peHeader->Signature != EFI_IMAGE_PE_SIGNATURE)
            return ERR_UNKNOWN_IMAGE_TYPE;
        offset += sizeof(EFI_IMAGE_PE_HEADER);

        // Skip file header
        offset += sizeof(EFI_IMAGE_FILE_HEADER);

        // Check optional header magic
        const UINT16 magic = *(const UINT16*)(file.constData() + offset);
        if (magic == EFI_IMAGE_PE_OPTIONAL_HDR32_MAGIC) {
            if ((UINT32)file.size() < offset + sizeof(EFI_IMAGE_OPTIONAL_HEADER32))
                return ERR_UNKNOWN_PE_OPTIONAL_HEADER_TYPE;
            const EFI_IMAGE_OPTIONAL_HEADER32* optHeader = (const EFI_IMAGE_OPTIONAL_HEADER32*)(file.constData() + offset);
            base = optHeader->ImageBase;
        }
        else if (magic == EFI_IMAGE_PE_OPTIONAL_HDR64_MAGIC) {
            if ((UINT32)file.size() < offset + sizeof(EFI_IMAGE_OPTIONAL_HEADER64))
                return ERR_UNKNOWN_PE_OPTIONAL_HEADER_TYPE;
            const EFI_IMAGE_OPTIONAL_HEADER64* optHeader = (const EFI_IMAGE_OPTIONAL_HEADER64*)(file.constData() + offset);
            base = optHeader->ImageBase;
        }
        else
            return ERR_UNKNOWN_PE_OPTIONAL_HEADER_TYPE;
    }
    else if (dosHeader->e_magic == EFI_IMAGE_TE_SIGNATURE){
        // Populate TE header
        if ((UINT32)file.size() < sizeof(EFI_IMAGE_TE_HEADER))
            return ERR_INVALID_FILE;
        const EFI_IMAGE_TE_HEADER* teHeader = (const EFI_IMAGE_TE_HEADER*)file.constData();
        //!TODO: add handling
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

UINT8 FfsEngine::dump(const QModelIndex & index, const QString & path, const QString & guid)
{
    dumped = false;
    UINT8 result = recursiveDump(index, path, guid);
    if (result)
        return result;
    else if (!dumped)
        return ERR_ITEM_NOT_FOUND;
    return ERR_SUCCESS;
}

UINT8 FfsEngine::recursiveDump(const QModelIndex & index, const QString & path, const QString & guid)
{
    if (!index.isValid())
        return ERR_INVALID_PARAMETER;

    QDir dir;
    if (guid.isEmpty() ||
        guidToQString(*(const EFI_GUID*)model->header(index).constData()) == guid ||
        guidToQString(*(const EFI_GUID*)model->header(model->findParentOfType(index, Types::File)).constData()) == guid) {

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
            .arg(itemTypeToQString(model->type(index)))
            .arg(itemSubtypeToQString(model->type(index), model->subtype(index)))
            .arg(model->text(index).isEmpty() ? "" : tr("Text: %1\n").arg(model->text(index)))
            .arg(model->info(index));
        file.setFileName(tr("%1/info.txt").arg(path));
        if (!file.open(QFile::Text | QFile::WriteOnly))
            return ERR_FILE_OPEN;
        file.write(info.toLatin1());
        file.close();
        dumped = true;
    }

    UINT8 result;
    for (int i = 0; i < model->rowCount(index); i++) {
        QModelIndex childIndex = index.child(i, 0);
        QString childPath = QString("%1/%2 %3").arg(path).arg(i).arg(model->text(childIndex).isEmpty() ? model->name(childIndex) : model->text(childIndex));
        result = recursiveDump(childIndex, childPath, guid);
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
    msg(tr("patch: replaced %1 bytes at offset %2h %3 -> %4")
        .arg(replacePattern.length())
        .hexarg(offset)
        .arg(QString(data.mid(offset, replacePattern.length()).toHex()).toUpper())
        .arg(QString(replacePattern.toHex()).toUpper()));
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

