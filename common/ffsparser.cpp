/* ffsparser.cpp

Copyright (c) 2018, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHWARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*/

// A workaround for compilers not supporting c++11 and c11
// for using PRIX64.
#define __STDC_FORMAT_MACROS

#include "ffsparser.h"

#include <map>
#include <algorithm>
#include <inttypes.h>

#include "descriptor.h"
#include "ffs.h"
#include "gbe.h"
#include "me.h"
#include "fit.h"
#include "nvram.h"
#include "utility.h"
#include "peimage.h"
#include "parsingdata.h"
#include "types.h"
#include "utility.h"

#include "nvramparser.h"
#include "meparser.h"

#ifndef QT_CORE_LIB
namespace Qt {
    enum GlobalColor {
        red = 7,
        green = 8,
        cyan = 10,
        yellow = 12,
    };
}
#endif

// Region info
struct REGION_INFO {
    UINT32 offset;
    UINT32 length;
    UINT8  type;
    UByteArray data;
    friend bool operator< (const REGION_INFO & lhs, const REGION_INFO & rhs){ return lhs.offset < rhs.offset; }
};

// BPDT partition info
struct BPDT_PARTITION_INFO {
    BPDT_ENTRY ptEntry;
    UINT8 type;
    UModelIndex index;
    friend bool operator< (const BPDT_PARTITION_INFO & lhs, const BPDT_PARTITION_INFO & rhs){ return lhs.ptEntry.Offset < rhs.ptEntry.Offset; }
};

// CPD partition info
struct CPD_PARTITION_INFO {
    CPD_ENTRY ptEntry;
    UINT8 type;
    UModelIndex index;
    friend bool operator< (const CPD_PARTITION_INFO & lhs, const CPD_PARTITION_INFO & rhs){ return lhs.ptEntry.Offset.Offset < rhs.ptEntry.Offset.Offset; }
};

// Constructor
FfsParser::FfsParser(TreeModel* treeModel) : model(treeModel),
imageBase(0), addressDiff(0x100000000ULL),
bgAcmFound(false), bgKeyManifestFound(false), bgBootPolicyFound(false), bgProtectedRegionsBase(0) {
    nvramParser = new NvramParser(treeModel, this);
    meParser = new MeParser(treeModel, this);
}

// Destructor
FfsParser::~FfsParser() {
    delete nvramParser;
    delete meParser;
}

// Obtain parser messages
std::vector<std::pair<UString, UModelIndex> > FfsParser::getMessages() const {
    std::vector<std::pair<UString, UModelIndex> > meVector = meParser->getMessages();
    std::vector<std::pair<UString, UModelIndex> > nvramVector = nvramParser->getMessages();
    std::vector<std::pair<UString, UModelIndex> > resultVector = messagesVector;
    resultVector.insert(resultVector.end(), meVector.begin(), meVector.end());
    resultVector.insert(resultVector.end(), nvramVector.begin(), nvramVector.end());
    return resultVector;
}

// Firmware image parsing functions
USTATUS FfsParser::parse(const UByteArray & buffer)
{
    UModelIndex root;

    // Reset global parser state
    openedImage = buffer;
    imageBase = 0;
    addressDiff = 0x100000000ULL;
    bgAcmFound = false;
    bgKeyManifestFound = false;
    bgBootPolicyFound = false;
    bgProtectedRegionsBase = 0;
    lastVtf = UModelIndex();
    fitTable.clear();
    securityInfo = "";
    bgAcmFound = false;
    bgKeyManifestFound = false;
    bgBootPolicyFound = false;
    bgKmHash = UByteArray();
    bgBpHash = UByteArray();
    bgBpDigest = UByteArray();
    bgProtectedRanges.clear();
    bgDxeCoreIndex = UModelIndex();

    // Parse input buffer
    USTATUS result = performFirstPass(buffer, root);
    if (result == U_SUCCESS) {
        if (lastVtf.isValid()) {
            result = performSecondPass(root);
        }
        else {
            msg(usprintf("%s: not a single Volume Top File is found, the image may be corrupted", __FUNCTION__));
        }
    }

    addInfoRecursive(root);
    return result;
}

USTATUS FfsParser::performFirstPass(const UByteArray & buffer, UModelIndex & index)
{
    // Sanity check
    if (buffer.isEmpty()) {
        return EFI_INVALID_PARAMETER;
    }

    USTATUS result;

    // Try parsing as UEFI Capsule
    result = parseCapsule(buffer, 0, UModelIndex(), index);;
    if (result != U_ITEM_NOT_FOUND) {
        return result;
    }

    // Try parsing as Intel image
    result = parseIntelImage(buffer, 0, UModelIndex(), index);
    if (result != U_ITEM_NOT_FOUND) {
        return result;
    }

    // Parse as generic image
    return parseGenericImage(buffer, 0, UModelIndex(), index);
}

USTATUS FfsParser::parseGenericImage(const UByteArray & buffer, const UINT32 localOffset, const UModelIndex & parent, UModelIndex & index)
{
    // Parse as generic UEFI image
    UString name("UEFI image");
    UString info = usprintf("Full size: %Xh (%u)", buffer.size(), buffer.size());

    // Add tree item
    index = model->addItem(localOffset, Types::Image, Subtypes::UefiImage, name, UString(), info, UByteArray(), buffer, UByteArray(), Fixed, parent);

    // Parse the image as raw area
    bgProtectedRegionsBase = imageBase = model->base(parent) + localOffset;
    return parseRawArea(index);
}

USTATUS FfsParser::parseCapsule(const UByteArray & capsule, const UINT32 localOffset, const UModelIndex & parent, UModelIndex & index)
{
    // Check buffer size to be more than or equal to size of EFI_CAPSULE_HEADER
    if ((UINT32)capsule.size() < sizeof(EFI_CAPSULE_HEADER)) {
        return U_ITEM_NOT_FOUND;
    }

    UINT32 capsuleHeaderSize = 0;
    // Check buffer for being normal EFI capsule header
    if (capsule.startsWith(EFI_CAPSULE_GUID)
        || capsule.startsWith(EFI_FMP_CAPSULE_GUID)
        || capsule.startsWith(INTEL_CAPSULE_GUID)
        || capsule.startsWith(LENOVO_CAPSULE_GUID)
        || capsule.startsWith(LENOVO2_CAPSULE_GUID)) {
        // Get info
        const EFI_CAPSULE_HEADER* capsuleHeader = (const EFI_CAPSULE_HEADER*)capsule.constData();

        // Check sanity of HeaderSize and CapsuleImageSize values
        if (capsuleHeader->HeaderSize == 0 || capsuleHeader->HeaderSize > (UINT32)capsule.size()
            || capsuleHeader->HeaderSize > capsuleHeader->CapsuleImageSize) {
            msg(usprintf("%s: UEFI capsule header size of %Xh (%u) bytes is invalid", __FUNCTION__,
                         capsuleHeader->HeaderSize,
                         capsuleHeader->HeaderSize));
            return U_INVALID_CAPSULE;
        }
        if (capsuleHeader->CapsuleImageSize > (UINT32)capsule.size()) {
            msg(usprintf("%s: UEFI capsule image size of %Xh (%u) bytes is invalid", __FUNCTION__,
                         capsuleHeader->CapsuleImageSize,
                         capsuleHeader->CapsuleImageSize));
            return U_INVALID_CAPSULE;
        }

        capsuleHeaderSize = capsuleHeader->HeaderSize;
        UByteArray header = capsule.left(capsuleHeaderSize);
        UByteArray body = capsule.mid(capsuleHeaderSize);
        UString name("UEFI capsule");
        UString info = UString("Capsule GUID: ") + guidToUString(capsuleHeader->CapsuleGuid, false) +
        usprintf("\nFull size: %Xh (%u)\nHeader size: %Xh (%u)\nImage size: %Xh (%u)\nFlags: %08Xh",
                 capsule.size(), capsule.size(),
                 capsuleHeaderSize, capsuleHeaderSize,
                 capsuleHeader->CapsuleImageSize - capsuleHeaderSize, capsuleHeader->CapsuleImageSize - capsuleHeaderSize,
                 capsuleHeader->Flags);

        // Add tree item
        index = model->addItem(localOffset, Types::Capsule, Subtypes::UefiCapsule, name, UString(), info, header, body, UByteArray(), Fixed, parent);
    }
    // Check buffer for being Toshiba capsule header
    else if (capsule.startsWith(TOSHIBA_CAPSULE_GUID)) {
        // Get info
        const TOSHIBA_CAPSULE_HEADER* capsuleHeader = (const TOSHIBA_CAPSULE_HEADER*)capsule.constData();

        // Check sanity of HeaderSize and FullSize values
        if (capsuleHeader->HeaderSize == 0 || capsuleHeader->HeaderSize > (UINT32)capsule.size()
            || capsuleHeader->HeaderSize > capsuleHeader->FullSize) {
            msg(usprintf("%s: Toshiba capsule header size of %Xh (%u) bytes is invalid", __FUNCTION__,
                         capsuleHeader->HeaderSize, capsuleHeader->HeaderSize));
            return U_INVALID_CAPSULE;
        }
        if (capsuleHeader->FullSize > (UINT32)capsule.size()) {
            msg(usprintf("%s: Toshiba capsule full size of %Xh (%u) bytes is invalid", __FUNCTION__,
                         capsuleHeader->FullSize, capsuleHeader->FullSize));
            return U_INVALID_CAPSULE;
        }

        capsuleHeaderSize = capsuleHeader->HeaderSize;
        UByteArray header = capsule.left(capsuleHeaderSize);
        UByteArray body = capsule.mid(capsuleHeaderSize);
        UString name("Toshiba capsule");
        UString info = UString("Capsule GUID: ") + guidToUString(capsuleHeader->CapsuleGuid, false) +
        usprintf("\nFull size: %Xh (%u)\nHeader size: %Xh (%u)\nImage size: %Xh (%u)\nFlags: %08Xh",
                 capsule.size(), capsule.size(),
                 capsuleHeaderSize, capsuleHeaderSize,
                 capsuleHeader->FullSize - capsuleHeaderSize, capsuleHeader->FullSize - capsuleHeaderSize,
                 capsuleHeader->Flags);

        // Add tree item
        index = model->addItem(localOffset, Types::Capsule, Subtypes::ToshibaCapsule, name, UString(), info, header, body, UByteArray(), Fixed, parent);
    }
    // Check buffer for being extended Aptio capsule header
    else if (capsule.startsWith(APTIO_SIGNED_CAPSULE_GUID)
             || capsule.startsWith(APTIO_UNSIGNED_CAPSULE_GUID)) {
        bool signedCapsule = capsule.startsWith(APTIO_SIGNED_CAPSULE_GUID);

        if ((UINT32)capsule.size() <= sizeof(APTIO_CAPSULE_HEADER)) {
            msg(usprintf("%s: AMI capsule image file is smaller than minimum size of 20h (32) bytes", __FUNCTION__));
            return U_INVALID_CAPSULE;
        }

        // Get info
        const APTIO_CAPSULE_HEADER* capsuleHeader = (const APTIO_CAPSULE_HEADER*)capsule.constData();

        // Check sanity of RomImageOffset and CapsuleImageSize values
        if (capsuleHeader->RomImageOffset == 0 || capsuleHeader->RomImageOffset > (UINT32)capsule.size()
            || capsuleHeader->RomImageOffset > capsuleHeader->CapsuleHeader.CapsuleImageSize) {
            msg(usprintf("%s: AMI capsule image offset of %Xh (%u) bytes is invalid", __FUNCTION__,
                         capsuleHeader->RomImageOffset, capsuleHeader->RomImageOffset));
            return U_INVALID_CAPSULE;
        }
        if (capsuleHeader->CapsuleHeader.CapsuleImageSize > (UINT32)capsule.size()) {
            msg(usprintf("%s: AMI capsule image size of %Xh (%u) bytes is invalid", __FUNCTION__,
                         capsuleHeader->CapsuleHeader.CapsuleImageSize,
                         capsuleHeader->CapsuleHeader.CapsuleImageSize));
            return U_INVALID_CAPSULE;
        }

        capsuleHeaderSize = capsuleHeader->RomImageOffset;
        UByteArray header = capsule.left(capsuleHeaderSize);
        UByteArray body = capsule.mid(capsuleHeaderSize);
        UString name("AMI Aptio capsule");
        UString info = UString("Capsule GUID: ") + guidToUString(capsuleHeader->CapsuleHeader.CapsuleGuid, false) +
        usprintf("\nFull size: %Xh (%u)\nHeader size: %Xh (%u)\nImage size: %Xh (%u)\nFlags: %08Xh",
                 capsule.size(), capsule.size(),
                 capsuleHeaderSize, capsuleHeaderSize,
                 capsuleHeader->CapsuleHeader.CapsuleImageSize - capsuleHeaderSize, capsuleHeader->CapsuleHeader.CapsuleImageSize - capsuleHeaderSize,
                 capsuleHeader->CapsuleHeader.Flags);

        // Add tree item
        index = model->addItem(localOffset, Types::Capsule, signedCapsule ? Subtypes::AptioSignedCapsule : Subtypes::AptioUnsignedCapsule, name, UString(), info, header, body, UByteArray(), Fixed, parent);

        // Show message about possible Aptio signature break
        if (signedCapsule) {
            msg(usprintf("%s: Aptio capsule signature may become invalid after image modifications", __FUNCTION__), index);
        }
    }

    // Capsule present
    if (capsuleHeaderSize > 0) {
        UByteArray image = capsule.mid(capsuleHeaderSize);
        UModelIndex imageIndex;

        // Try parsing as Intel image
        USTATUS result = parseIntelImage(image, capsuleHeaderSize, index, imageIndex);
        if (result != U_ITEM_NOT_FOUND) {
            return result;
        }

        // Parse as generic image
        return parseGenericImage(image, capsuleHeaderSize, index, imageIndex);
    }

    return U_ITEM_NOT_FOUND;
}

USTATUS FfsParser::parseIntelImage(const UByteArray & intelImage, const UINT32 localOffset, const UModelIndex & parent, UModelIndex & index)
{
    // Check for buffer size to be greater or equal to descriptor region size
    if (intelImage.size() < FLASH_DESCRIPTOR_SIZE) {
        msg(usprintf("%s: input file is smaller than minimum descriptor size of %Xh (%u) bytes", __FUNCTION__, FLASH_DESCRIPTOR_SIZE, FLASH_DESCRIPTOR_SIZE));
        return U_ITEM_NOT_FOUND;
    }

    // Store the beginning of descriptor as descriptor base address
    const FLASH_DESCRIPTOR_HEADER* descriptor = (const FLASH_DESCRIPTOR_HEADER*)intelImage.constData();

    // Check descriptor signature
    if (descriptor->Signature != FLASH_DESCRIPTOR_SIGNATURE) {
        return U_ITEM_NOT_FOUND;
    }

    // Parse descriptor map
    const FLASH_DESCRIPTOR_MAP* descriptorMap = (const FLASH_DESCRIPTOR_MAP*)((UINT8*)descriptor + sizeof(FLASH_DESCRIPTOR_HEADER));
    const FLASH_DESCRIPTOR_UPPER_MAP* upperMap = (const FLASH_DESCRIPTOR_UPPER_MAP*)((UINT8*)descriptor + FLASH_DESCRIPTOR_UPPER_MAP_BASE);

    // Check sanity of base values
    if (descriptorMap->MasterBase > FLASH_DESCRIPTOR_MAX_BASE
        || descriptorMap->MasterBase == descriptorMap->RegionBase
        || descriptorMap->MasterBase == descriptorMap->ComponentBase) {
        msg(usprintf("%s: invalid descriptor master base %02Xh", __FUNCTION__, descriptorMap->MasterBase));
        return U_INVALID_FLASH_DESCRIPTOR;
    }
    if (descriptorMap->RegionBase > FLASH_DESCRIPTOR_MAX_BASE
        || descriptorMap->RegionBase == descriptorMap->ComponentBase) {
        msg(usprintf("%s: invalid descriptor region base %02Xh", __FUNCTION__, descriptorMap->RegionBase));
        return U_INVALID_FLASH_DESCRIPTOR;
    }
    if (descriptorMap->ComponentBase > FLASH_DESCRIPTOR_MAX_BASE) {
        msg(usprintf("%s: invalid descriptor component base %02Xh", __FUNCTION__, descriptorMap->ComponentBase));
        return U_INVALID_FLASH_DESCRIPTOR;
    }

    const FLASH_DESCRIPTOR_REGION_SECTION* regionSection = (const FLASH_DESCRIPTOR_REGION_SECTION*)calculateAddress8((UINT8*)descriptor, descriptorMap->RegionBase);
    const FLASH_DESCRIPTOR_COMPONENT_SECTION* componentSection = (const FLASH_DESCRIPTOR_COMPONENT_SECTION*)calculateAddress8((UINT8*)descriptor, descriptorMap->ComponentBase);

    UINT8 descriptorVersion = 2;
    // Check descriptor version by getting hardcoded value of FlashParameters.ReadClockFrequency
    if (componentSection->FlashParameters.ReadClockFrequency == FLASH_FREQUENCY_20MHZ)
        descriptorVersion = 1;

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
    if (regionSection->BiosLimit) {
        REGION_INFO bios;
        bios.type = Subtypes::BiosRegion;
        bios.offset = calculateRegionOffset(regionSection->BiosBase);
        bios.length = calculateRegionSize(regionSection->BiosBase, regionSection->BiosLimit);

        // Check for Gigabyte specific descriptor map
        if (bios.length == (UINT32)intelImage.size()) {
            if (!me.offset) {
                msg(usprintf("%s: can't determine BIOS region start from Gigabyte-specific descriptor", __FUNCTION__));
                return U_INVALID_FLASH_DESCRIPTOR;
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
        msg(usprintf("%s: descriptor parsing failed, BIOS region not found in descriptor", __FUNCTION__));
        return U_INVALID_FLASH_DESCRIPTOR;
    }

    // Add all other regions
    for (UINT8 i = Subtypes::GbeRegion; i <= Subtypes::PttRegion; i++) {
        if (descriptorVersion == 1 && i == Subtypes::MicrocodeRegion)
            break; // Do not parse Microcode and other following regions for legacy descriptors

        const UINT16* RegionBase = ((const UINT16*)regionSection) + 2 * i;
        const UINT16* RegionLimit = ((const UINT16*)regionSection) + 2 * i + 1;
        if (*RegionLimit && !(*RegionBase == 0xFFFF && *RegionLimit == 0xFFFF)) {
            REGION_INFO region;
            region.type = i;
            region.offset = calculateRegionOffset(*RegionBase);
            region.length = calculateRegionSize(*RegionBase, *RegionLimit);
            if (region.length != 0) {
                region.data = intelImage.mid(region.offset, region.length);
                regions.push_back(region);
            }
        }
    }

    // Sort regions in ascending order
    std::sort(regions.begin(), regions.end());

    // Check for intersections and paddings between regions
    REGION_INFO region;
    // Check intersection with the descriptor
    if (regions.front().offset < FLASH_DESCRIPTOR_SIZE) {
        msg(usprintf("%s: ", __FUNCTION__) + itemSubtypeToUString(Types::Region, regions.front().type)
            + UString(" region has intersection with flash descriptor"),
            index);
        return U_INVALID_FLASH_DESCRIPTOR;
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
        if ((UINT64)regions[i].offset + (UINT64)regions[i].length > (UINT64)intelImage.size()) {
            msg(usprintf("%s: ", __FUNCTION__) + itemSubtypeToUString(Types::Region, regions[i].type)
                + UString(" region is located outside of the opened image. If your system uses dual-chip storage, please append another part to the opened image"),
                index);
            return U_TRUNCATED_IMAGE;
        }

        // Check for intersection with previous region
        if (regions[i].offset < previousRegionEnd) {
            msg(usprintf("%s: ", __FUNCTION__) + itemSubtypeToUString(Types::Region, regions[i].type)
                + UString(" region has intersection with ") + itemSubtypeToUString(Types::Region, regions[i - 1].type) +UString(" region"),
                index);
            return U_INVALID_FLASH_DESCRIPTOR;
        }
        // Check for padding between current and previous regions
        else if (regions[i].offset > previousRegionEnd) {
            region.offset = previousRegionEnd;
            region.length = regions[i].offset - previousRegionEnd;
            region.data = intelImage.mid(region.offset, region.length);
            region.type = getPaddingType(region.data);
            std::vector<REGION_INFO>::iterator iter = regions.begin();
            std::advance(iter, i);
            regions.insert(iter, region);
        }
    }
    // Check for padding after the last region
    if ((UINT64)regions.back().offset + (UINT64)regions.back().length < (UINT64)intelImage.size()) {
        region.offset = regions.back().offset + regions.back().length;
        region.length = intelImage.size() - region.offset;
        region.data = intelImage.mid(region.offset, region.length);
        region.type = getPaddingType(region.data);
        regions.push_back(region);
    }

    // Region map is consistent

    // Intel image
    UString name("Intel image");
    UString info = usprintf("Full size: %Xh (%u)\nFlash chips: %u\nRegions: %u\nMasters: %u\nPCH straps: %u\nPROC straps: %u",
        intelImage.size(), intelImage.size(),
        descriptorMap->NumberOfFlashChips + 1, //
        descriptorMap->NumberOfRegions + 1,    // Zero-based numbers in storage
        descriptorMap->NumberOfMasters + 1,    //
        descriptorMap->NumberOfPchStraps,
        descriptorMap->NumberOfProcStraps);

    // Set image base
    imageBase = model->base(parent) + localOffset;

    // Add Intel image tree item
    index = model->addItem(localOffset, Types::Image, Subtypes::IntelImage, name, UString(), info, UByteArray(), intelImage, UByteArray(), Fixed, parent);

    // Descriptor
    // Get descriptor info
    UByteArray body = intelImage.left(FLASH_DESCRIPTOR_SIZE);
    name = UString("Descriptor region");
    info = usprintf("ReservedVector:\n%02X %02X %02X %02X %02X %02X %02X %02X\n"
                    "%02X %02X %02X %02X %02X %02X %02X %02X\nFull size: %Xh (%u)",
                    descriptor->ReservedVector[0],  descriptor->ReservedVector[1],  descriptor->ReservedVector[2],  descriptor->ReservedVector[3],
                    descriptor->ReservedVector[4],  descriptor->ReservedVector[5],  descriptor->ReservedVector[6],  descriptor->ReservedVector[7],
                    descriptor->ReservedVector[8],  descriptor->ReservedVector[9],  descriptor->ReservedVector[10], descriptor->ReservedVector[11],
                    descriptor->ReservedVector[12], descriptor->ReservedVector[13], descriptor->ReservedVector[14], descriptor->ReservedVector[15],
                    FLASH_DESCRIPTOR_SIZE, FLASH_DESCRIPTOR_SIZE);

    // Add offsets of actual regions
    for (size_t i = 0; i < regions.size(); i++) {
        if (regions[i].type != Subtypes::ZeroPadding && regions[i].type != Subtypes::OnePadding && regions[i].type != Subtypes::DataPadding)
            info += UString("\n") + itemSubtypeToUString(Types::Region, regions[i].type)
            + usprintf(" region offset: %Xh", regions[i].offset + localOffset);
    }

    // Region access settings
    if (descriptorVersion == 1) {
        const FLASH_DESCRIPTOR_MASTER_SECTION* masterSection = (const FLASH_DESCRIPTOR_MASTER_SECTION*)calculateAddress8((UINT8*)descriptor, descriptorMap->MasterBase);
        info += UString("\nRegion access settings:");
        info += usprintf("\nBIOS: %02Xh %02Xh ME: %02Xh %02Xh\nGbE:  %02Xh %02Xh",
            masterSection->BiosRead,
            masterSection->BiosWrite,
            masterSection->MeRead,
            masterSection->MeWrite,
            masterSection->GbeRead,
            masterSection->GbeWrite);

        // BIOS access table
        info  += UString("\nBIOS access table:")
               + UString("\n      Read  Write")
              + usprintf("\nDesc  %s  %s",  masterSection->BiosRead  & FLASH_DESCRIPTOR_REGION_ACCESS_DESC ? "Yes " : "No  ",
                                            masterSection->BiosWrite & FLASH_DESCRIPTOR_REGION_ACCESS_DESC ? "Yes " : "No  ");
        info  += UString("\nBIOS  Yes   Yes")
              + usprintf("\nME    %s  %s",  masterSection->BiosRead  & FLASH_DESCRIPTOR_REGION_ACCESS_ME   ? "Yes " : "No  ",
                                            masterSection->BiosWrite & FLASH_DESCRIPTOR_REGION_ACCESS_ME   ? "Yes " : "No  ");
        info += usprintf("\nGbE   %s  %s",  masterSection->BiosRead  & FLASH_DESCRIPTOR_REGION_ACCESS_GBE  ? "Yes " : "No  ",
                                            masterSection->BiosWrite & FLASH_DESCRIPTOR_REGION_ACCESS_GBE  ? "Yes " : "No  ");
        info += usprintf("\nPDR   %s  %s",  masterSection->BiosRead  & FLASH_DESCRIPTOR_REGION_ACCESS_PDR  ? "Yes " : "No  ",
                                            masterSection->BiosWrite & FLASH_DESCRIPTOR_REGION_ACCESS_PDR  ? "Yes " : "No  ");
    }
    else if (descriptorVersion == 2) {
        const FLASH_DESCRIPTOR_MASTER_SECTION_V2* masterSection = (const FLASH_DESCRIPTOR_MASTER_SECTION_V2*)calculateAddress8((UINT8*)descriptor, descriptorMap->MasterBase);
        info += UString("\nRegion access settings:");
        info += usprintf("\nBIOS: %03Xh %03Xh ME: %03Xh %03Xh\nGbE:  %03Xh %03Xh EC: %03Xh %03Xh",
            masterSection->BiosRead,
            masterSection->BiosWrite,
            masterSection->MeRead,
            masterSection->MeWrite,
            masterSection->GbeRead,
            masterSection->GbeWrite,
            masterSection->EcRead,
            masterSection->EcWrite);

        // BIOS access table
        info  += UString("\nBIOS access table:")
               + UString("\n      Read  Write")
              + usprintf("\nDesc  %s  %s",
            masterSection->BiosRead  & FLASH_DESCRIPTOR_REGION_ACCESS_DESC ? "Yes " : "No  ",
            masterSection->BiosWrite & FLASH_DESCRIPTOR_REGION_ACCESS_DESC ? "Yes " : "No  ");
        info  += UString("\nBIOS  Yes   Yes")
              + usprintf("\nME    %s  %s",
            masterSection->BiosRead  & FLASH_DESCRIPTOR_REGION_ACCESS_ME ? "Yes " : "No  ",
            masterSection->BiosWrite & FLASH_DESCRIPTOR_REGION_ACCESS_ME ? "Yes " : "No  ");
        info += usprintf("\nGbE   %s  %s",
            masterSection->BiosRead  & FLASH_DESCRIPTOR_REGION_ACCESS_GBE ? "Yes " : "No  ",
            masterSection->BiosWrite & FLASH_DESCRIPTOR_REGION_ACCESS_GBE ? "Yes " : "No  ");
        info += usprintf("\nPDR   %s  %s",
            masterSection->BiosRead  & FLASH_DESCRIPTOR_REGION_ACCESS_PDR ? "Yes " : "No  ",
            masterSection->BiosWrite & FLASH_DESCRIPTOR_REGION_ACCESS_PDR ? "Yes " : "No  ");
        info += usprintf("\nEC    %s  %s",
            masterSection->BiosRead  & FLASH_DESCRIPTOR_REGION_ACCESS_EC ? "Yes " : "No  ",
            masterSection->BiosWrite & FLASH_DESCRIPTOR_REGION_ACCESS_EC ? "Yes " : "No  ");

        // Prepend descriptor version if present
        if (descriptorMap->DescriptorVersion != FLASH_DESCRIPTOR_VERSION_INVALID) {
            const FLASH_DESCRIPTOR_VERSION* version = (const FLASH_DESCRIPTOR_VERSION*)&descriptorMap->DescriptorVersion;
            UString versionStr = usprintf("Flash descriptor version: %d.%d", version->Major, version->Minor);
            if (version->Major != FLASH_DESCRIPTOR_VERSION_MAJOR || version->Minor != FLASH_DESCRIPTOR_VERSION_MINOR) {
                versionStr += ", unknown";
                msg(usprintf("%s: unknown flash descriptor version %d.%d", __FUNCTION__, version->Major, version->Minor));
            }
            info = versionStr + "\n" + info;
        }
    }

    // VSCC table
    const VSCC_TABLE_ENTRY* vsccTableEntry = (const VSCC_TABLE_ENTRY*)((UINT8*)descriptor + ((UINT16)upperMap->VsccTableBase << 4));
    info += UString("\nFlash chips in VSCC table:");
    UINT8 vsscTableSize = upperMap->VsccTableSize * sizeof(UINT32) / sizeof(VSCC_TABLE_ENTRY);
    for (UINT8 i = 0; i < vsscTableSize; i++) {
        UString jedecId = jedecIdToUString(vsccTableEntry->VendorId, vsccTableEntry->DeviceId0, vsccTableEntry->DeviceId1);
        info += usprintf("\n%02X%02X%02X (", vsccTableEntry->VendorId, vsccTableEntry->DeviceId0, vsccTableEntry->DeviceId1)
            + jedecId
            + UString(")");
        if (jedecId == UString("Unknown")) {
            msg(usprintf("%s: SPI flash with unknown JEDEC ID %02X%02X%02X found in VSCC table", __FUNCTION__,
                         vsccTableEntry->VendorId, vsccTableEntry->DeviceId0, vsccTableEntry->DeviceId1), index);
        }
        vsccTableEntry++;
    }

    // Add descriptor tree item
    UModelIndex regionIndex = model->addItem(localOffset, Types::Region, Subtypes::DescriptorRegion, name, UString(), info, UByteArray(), body, UByteArray(), Fixed, index);


    // Parse regions
    USTATUS result = U_SUCCESS;
    USTATUS parseResult = U_SUCCESS;
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
        case Subtypes::DevExp1Region:
            result = parseDevExp1Region(region.data, region.offset, index, regionIndex);
            break;
        case Subtypes::Bios2Region:
        case Subtypes::MicrocodeRegion:
        case Subtypes::EcRegion:
        case Subtypes::DevExp2Region:
        case Subtypes::IeRegion:
        case Subtypes::Tgbe1Region:
        case Subtypes::Tgbe2Region:
        case Subtypes::Reserved1Region:
        case Subtypes::Reserved2Region:
        case Subtypes::PttRegion:
            result = parseGenericRegion(region.type, region.data, region.offset, index, regionIndex);
            break;
        case Subtypes::ZeroPadding:
        case Subtypes::OnePadding:
        case Subtypes::DataPadding: {
            // Add padding between regions
            UByteArray padding = intelImage.mid(region.offset, region.length);

            // Get info
            name = UString("Padding");
            info = usprintf("Full size: %Xh (%u)",
                padding.size(), padding.size());

            // Add tree item
            regionIndex = model->addItem(region.offset, Types::Padding, getPaddingType(padding), name, UString(), info, UByteArray(), padding, UByteArray(), Fixed, index);
            result = U_SUCCESS;
            } break;
        default:
            msg(usprintf("%s: region of unknown type found", __FUNCTION__), index);
            result = U_INVALID_FLASH_DESCRIPTOR;
        }
        // Store the first failed result as a final result
        if (!parseResult && result) {
            parseResult = result;
        }
    }

    return parseResult;
}

USTATUS FfsParser::parseGbeRegion(const UByteArray & gbe, const UINT32 localOffset, const UModelIndex & parent, UModelIndex & index)
{
    // Check sanity
    if (gbe.isEmpty())
        return U_EMPTY_REGION;
    if ((UINT32)gbe.size() < GBE_VERSION_OFFSET + sizeof(GBE_VERSION))
        return U_INVALID_REGION;

    // Get info
    UString name("GbE region");
    const GBE_MAC_ADDRESS* mac = (const GBE_MAC_ADDRESS*)gbe.constData();
    const GBE_VERSION* version = (const GBE_VERSION*)(gbe.constData() + GBE_VERSION_OFFSET);
    UString info = usprintf("Full size: %Xh (%u)\nMAC: %02X:%02X:%02X:%02X:%02X:%02X\nVersion: %u.%u",
        gbe.size(), gbe.size(),
        mac->vendor[0], mac->vendor[1], mac->vendor[2],
        mac->device[0], mac->device[1], mac->device[2],
        version->major,
        version->minor);

    // Add tree item
    index = model->addItem(localOffset, Types::Region, Subtypes::GbeRegion, name, UString(), info, UByteArray(), gbe, UByteArray(), Fixed, parent);

    return U_SUCCESS;
}

USTATUS FfsParser::parseMeRegion(const UByteArray & me, const UINT32 localOffset, const UModelIndex & parent, UModelIndex & index)
{
    // Check sanity
    if (me.isEmpty())
        return U_EMPTY_REGION;

    // Get info
    UString name("ME region");
    UString info = usprintf("Full size: %Xh (%u)", me.size(), me.size());

    // Parse region
    bool versionFound = true;
    bool emptyRegion = false;
    // Check for empty region
    if (me.size() == me.count('\xFF') || me.size() == me.count('\x00')) {
        // Further parsing not needed
        emptyRegion = true;
        info += ("\nState: empty");
    }
    else {
        // Search for new signature
        INT32 versionOffset = me.indexOf(ME_VERSION_SIGNATURE2);
        if (versionOffset < 0){ // New signature not found
            // Search for old signature
            versionOffset = me.indexOf(ME_VERSION_SIGNATURE);
            if (versionOffset < 0){
                info += ("\nVersion: unknown");
                versionFound = false;
            }
        }

        // Check sanity
        if ((UINT32)me.size() < (UINT32)versionOffset + sizeof(ME_VERSION))
            return U_INVALID_REGION;

        // Add version information
        if (versionFound) {
            const ME_VERSION* version = (const ME_VERSION*)(me.constData() + versionOffset);
            info += usprintf("\nVersion: %u.%u.%u.%u",
                version->Major,
                version->Minor,
                version->Bugfix,
                version->Build);
        }
    }

    // Add tree item
    index = model->addItem(localOffset, Types::Region, Subtypes::MeRegion, name, UString(), info, UByteArray(), me, UByteArray(), Fixed, parent);

    // Show messages
    if (emptyRegion) {
        msg(usprintf("%s: ME region is empty", __FUNCTION__), index);
    }
    else if (!versionFound) {
        msg(usprintf("%s: ME version is unknown, it can be damaged", __FUNCTION__), index);
    }
    else {
        meParser->parseMeRegionBody(index);
    }

    return U_SUCCESS;
}

USTATUS FfsParser::parsePdrRegion(const UByteArray & pdr, const UINT32 localOffset, const UModelIndex & parent, UModelIndex & index)
{
    // Check sanity
    if (pdr.isEmpty())
        return U_EMPTY_REGION;

    // Get info
    UString name("PDR region");
    UString info = usprintf("Full size: %Xh (%u)", pdr.size(), pdr.size());

    // Add tree item
    index = model->addItem(localOffset, Types::Region, Subtypes::PdrRegion, name, UString(), info, UByteArray(), pdr, UByteArray(), Fixed, parent);

    // Parse PDR region as BIOS space
    USTATUS result = parseRawArea(index);
    if (result && result != U_VOLUMES_NOT_FOUND && result != U_INVALID_VOLUME && result != U_STORES_NOT_FOUND)
        return result;

    return U_SUCCESS;
}

USTATUS FfsParser::parseDevExp1Region(const UByteArray & devExp1, const UINT32 localOffset, const UModelIndex & parent, UModelIndex & index)
{
    // Check sanity
    if (devExp1.isEmpty())
        return U_EMPTY_REGION;

    // Get info
    UString name("DevExp1 region");
    UString info = usprintf("Full size: %Xh (%u)", devExp1.size(), devExp1.size());

    bool emptyRegion = false;
    // Check for empty region
    if (devExp1.size() == devExp1.count('\xFF') || devExp1.size() == devExp1.count('\x00')) {
        // Further parsing not needed
        emptyRegion = true;
        info += ("\nState: empty");
    }

    // Add tree item
    index = model->addItem(localOffset, Types::Region, Subtypes::DevExp1Region, name, UString(), info, UByteArray(), devExp1, UByteArray(), Fixed, parent);

    if (!emptyRegion) {
        meParser->parseMeRegionBody(index);
    }
    return U_SUCCESS;
}

USTATUS FfsParser::parseGenericRegion(const UINT8 subtype, const UByteArray & region, const UINT32 localOffset, const UModelIndex & parent, UModelIndex & index)
{
    // Check sanity
    if (region.isEmpty())
        return U_EMPTY_REGION;

    // Get info
    UString name = itemSubtypeToUString(Types::Region, subtype) + UString(" region");
    UString info = usprintf("Full size: %Xh (%u)", region.size(), region.size());

    // Add tree item
    index = model->addItem(localOffset, Types::Region, subtype, name, UString(), info, UByteArray(), region, UByteArray(), Fixed, parent);

    return U_SUCCESS;
}

USTATUS FfsParser::parseBiosRegion(const UByteArray & bios, const UINT32 localOffset, const UModelIndex & parent, UModelIndex & index)
{
    // Sanity check
    if (bios.isEmpty())
        return U_EMPTY_REGION;

    // Get info
    UString name("BIOS region");
    UString info = usprintf("Full size: %Xh (%u)", bios.size(), bios.size());

    // Add tree item
    index = model->addItem(localOffset, Types::Region, Subtypes::BiosRegion, name, UString(), info, UByteArray(), bios, UByteArray(), Fixed, parent);

    return parseRawArea(index);
}

USTATUS FfsParser::parseRawArea(const UModelIndex & index)
{
    // Sanity check
    if (!index.isValid())
        return U_INVALID_PARAMETER;

    // Get item data
    UByteArray data = model->body(index);
    UINT32 headerSize = model->header(index).size();

    USTATUS result;
    UString name;
    UString info;

    // Search for the first item
    UINT8  prevItemType = 0;
    UINT32 prevItemOffset = 0;
    UINT32 prevItemSize = 0;
    UINT32 prevItemAltSize = 0;

    result = findNextRawAreaItem(index, 0, prevItemType, prevItemOffset, prevItemSize, prevItemAltSize);
    if (result) {
        // No need to parse further
        return U_SUCCESS;
    }

    // Set base of protected regions to be the first volume
    if (model->type(index) == Types::Region
        && model->subtype(index) == Subtypes::BiosRegion) {
        bgProtectedRegionsBase = (UINT64)model->base(index) + prevItemOffset;
    }

    // First item is not at the beginning of this raw area
    if (prevItemOffset > 0) {
        // Get info
        UByteArray padding = data.left(prevItemOffset);
        name = UString("Padding");
        info = usprintf("Full size: %Xh (%u)", padding.size(), padding.size());

        // Add tree item
        model->addItem(headerSize, Types::Padding, getPaddingType(padding), name, UString(), info, UByteArray(), padding, UByteArray(), Fixed, index);
    }

    // Search for and parse all items
    UINT8  itemType = prevItemType;
    UINT32 itemOffset = prevItemOffset;
    UINT32 itemSize = prevItemSize;
    UINT32 itemAltSize = prevItemAltSize;

    while (!result) {
        // Padding between items
        if (itemOffset > prevItemOffset + prevItemSize) {
            UINT32 paddingOffset = prevItemOffset + prevItemSize;
            UINT32 paddingSize = itemOffset - paddingOffset;
            UByteArray padding = data.mid(paddingOffset, paddingSize);

            // Get info
            name = UString("Padding");
            info = usprintf("Full size: %Xh (%u)", padding.size(), padding.size());

            // Add tree item
            model->addItem(headerSize + paddingOffset, Types::Padding, getPaddingType(padding), name, UString(), info, UByteArray(), padding, UByteArray(), Fixed, index);
        }

        // Check that item is fully present in input
        if (itemSize > (UINT32)data.size() || itemOffset + itemSize > (UINT32)data.size()) {
            // Mark the rest as padding and finish parsing
            UByteArray padding = data.mid(itemOffset);

            // Get info
            name = UString("Padding");
            info = usprintf("Full size: %Xh (%u)", padding.size(), padding.size());

            // Add tree item
            UModelIndex paddingIndex = model->addItem(headerSize + itemOffset, Types::Padding, getPaddingType(padding), name, UString(), info, UByteArray(), padding, UByteArray(), Fixed, index);
            msg(usprintf("%s: one of volumes inside overlaps the end of data", __FUNCTION__), paddingIndex);

            // Update variables
            prevItemOffset = itemOffset;
            prevItemSize = padding.size();
            break;
        }

        // Parse current volume's header
        if (itemType == Types::Volume) {
            UModelIndex volumeIndex;
            UByteArray volume = data.mid(itemOffset, itemSize);
            result = parseVolumeHeader(volume, headerSize + itemOffset, index, volumeIndex);
            if (result) {
                msg(usprintf("%s: volume header parsing failed with error ", __FUNCTION__) + errorCodeToUString(result), index);
            } else {
                // Show messages
                if (itemSize != itemAltSize)
                    msg(usprintf("%s: volume size stored in header %Xh differs from calculated using block map %Xh", __FUNCTION__,
                    itemSize, itemAltSize),
                    volumeIndex);
            }
        }
        else if (itemType == Types::Microcode) {
            UModelIndex microcodeIndex;
            UByteArray microcode = data.mid(itemOffset, itemSize);
            result = parseIntelMicrocodeHeader(microcode, headerSize + itemOffset, index, microcodeIndex);
            if (result) {
                msg(usprintf("%s: microcode header parsing failed with error ", __FUNCTION__) + errorCodeToUString(result), index);
            }
        }
        else if (itemType == Types::BpdtStore) {
            UByteArray bpdtStore = data.mid(itemOffset, itemSize);

            // Get info
            name = UString("BPDT region");
            info = usprintf("Full size: %Xh (%u)", bpdtStore.size(), bpdtStore.size());

            // Add tree item
            UModelIndex bpdtIndex = model->addItem(headerSize + itemOffset, Types::BpdtStore, 0, name, UString(), info, UByteArray(), bpdtStore, UByteArray(), Fixed, index);

            // Parse BPDT region
            UModelIndex bpdtPtIndex;
            result = parseBpdtRegion(bpdtStore, 0, 0, bpdtIndex, bpdtPtIndex);
            if (result) {
                msg(usprintf("%s: BPDT store parsing failed with error ", __FUNCTION__) + errorCodeToUString(result), index);
            }
        }
        else {
            return U_UNKNOWN_ITEM_TYPE;
        }

        // Go to next item
        prevItemOffset = itemOffset;
        prevItemSize = itemSize;
        prevItemType = itemType;
        result = findNextRawAreaItem(index, itemOffset + prevItemSize, itemType, itemOffset, itemSize, itemAltSize);

        // Silence value not used after assignment warning
        (void)prevItemType;
    }

    // Padding at the end of RAW area
    itemOffset = prevItemOffset + prevItemSize;
    if ((UINT32)data.size() > itemOffset) {
        UByteArray padding = data.mid(itemOffset);

        // Get info
        name = UString("Padding");
        info = usprintf("Full size: %Xh (%u)", padding.size(), padding.size());

        // Add tree item
        model->addItem(headerSize + itemOffset, Types::Padding, getPaddingType(padding), name, UString(), info, UByteArray(), padding, UByteArray(), Fixed, index);
    }

    // Parse bodies
    for (int i = 0; i < model->rowCount(index); i++) {
        UModelIndex current = index.child(i, 0);
        switch (model->type(current)) {
        case Types::Volume:
            parseVolumeBody(current);
            break;
        case Types::Microcode:
            // Parsing already done
            break;
        case Types::BpdtStore:
            // Parsing already done
            break;
        case Types::BpdtPartition:
            // Parsing already done
            break;
        case Types::Padding:
            // No parsing required
            break;
        default:
            return U_UNKNOWN_ITEM_TYPE;
        }
    }

    return U_SUCCESS;
}

USTATUS FfsParser::parseVolumeHeader(const UByteArray & volume, const UINT32 localOffset, const UModelIndex & parent, UModelIndex & index)
{
    // Sanity check
    if (volume.isEmpty())
        return U_INVALID_PARAMETER;

    // Check that there is space for the volume header
        if ((UINT32)volume.size() < sizeof(EFI_FIRMWARE_VOLUME_HEADER)) {
        msg(usprintf("%s: input volume size %Xh (%u) is smaller than volume header size 40h (64)", __FUNCTION__, volume.size(), volume.size()));
        return U_INVALID_VOLUME;
    }

    // Populate volume header
    const EFI_FIRMWARE_VOLUME_HEADER* volumeHeader = (const EFI_FIRMWARE_VOLUME_HEADER*)(volume.constData());

    // Check sanity of HeaderLength value
    if ((UINT32)ALIGN8(volumeHeader->HeaderLength) > (UINT32)volume.size()) {
        msg(usprintf("%s: volume header overlaps the end of data", __FUNCTION__));
        return U_INVALID_VOLUME;
    }
    // Check sanity of ExtHeaderOffset value
    if (volumeHeader->Revision > 1 && volumeHeader->ExtHeaderOffset
        && (UINT32)ALIGN8(volumeHeader->ExtHeaderOffset + sizeof(EFI_FIRMWARE_VOLUME_EXT_HEADER)) > (UINT32)volume.size()) {
        msg(usprintf("%s: extended volume header overlaps the end of data", __FUNCTION__));
        return U_INVALID_VOLUME;
    }

    // Calculate volume header size
    UINT32 headerSize;
    EFI_GUID extendedHeaderGuid = {0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0 }};
    bool hasExtendedHeader = false;
    if (volumeHeader->Revision > 1 && volumeHeader->ExtHeaderOffset) {
        hasExtendedHeader = true;
        const EFI_FIRMWARE_VOLUME_EXT_HEADER* extendedHeader = (const EFI_FIRMWARE_VOLUME_EXT_HEADER*)(volume.constData() + volumeHeader->ExtHeaderOffset);
        headerSize = volumeHeader->ExtHeaderOffset + extendedHeader->ExtHeaderSize;
        extendedHeaderGuid = extendedHeader->FvName;
    }
    else {
        headerSize = volumeHeader->HeaderLength;
    }

    // Extended header end can be unaligned
    headerSize = ALIGN8(headerSize);

    // Check for volume structure to be known
    bool isUnknown = true;
    bool isNvramVolume = false;
    bool isMicrocodeVolume = false;
    UINT8 ffsVersion = 0;

    // Check for FFS v2 volume
    UByteArray guid = UByteArray((const char*)&volumeHeader->FileSystemGuid, sizeof(EFI_GUID));
    if (std::find(FFSv2Volumes.begin(), FFSv2Volumes.end(), guid) != FFSv2Volumes.end()) {
        isUnknown = false;
        ffsVersion = 2;
    }
    // Check for FFS v3 volume
    else if (std::find(FFSv3Volumes.begin(), FFSv3Volumes.end(), guid) != FFSv3Volumes.end()) {
        isUnknown = false;
        ffsVersion = 3;
    }
    // Check for VSS NVRAM volume
    else if (guid == NVRAM_MAIN_STORE_VOLUME_GUID || guid == NVRAM_ADDITIONAL_STORE_VOLUME_GUID) {
        isUnknown = false;
        isNvramVolume = true;
    }
    // Check for Microcode volume
    else if (guid == EFI_APPLE_MICROCODE_VOLUME_GUID) {
        isUnknown = false;
        isMicrocodeVolume = true;
        headerSize = EFI_APPLE_MICROCODE_VOLUME_HEADER_SIZE;
    }

    // Check volume revision and alignment
    bool msgAlignmentBitsSet = false;
    bool msgUnaligned = false;
    bool msgUnknownRevision = false;
    UINT32 alignment = 0x10000; // Default volume alignment is 64K
    if (volumeHeader->Revision == 1) {
        // Acquire alignment capability bit
        bool alignmentCap = (volumeHeader->Attributes & EFI_FVB_ALIGNMENT_CAP) != 0;
        if (!alignmentCap) {
            if (volumeHeader->Attributes & 0xFFFF0000)
                msgAlignmentBitsSet = true;
        }
        // Do not check for volume alignment on revision 1 volumes
        // No one gives a single damn about setting it correctly
    }
    else if (volumeHeader->Revision == 2) {
        // Acquire alignment
        alignment = (UINT32)(1UL << ((volumeHeader->Attributes & EFI_FVB2_ALIGNMENT) >> 16));
        // Check alignment
        if (!isUnknown && !model->compressed(parent) && ((model->base(parent) + localOffset - imageBase) % alignment))
            msgUnaligned = true;
    }
    else {
        msgUnknownRevision = true;
    }

    // Check attributes
    // Determine value of empty byte
    UINT8 emptyByte = volumeHeader->Attributes & EFI_FVB_ERASE_POLARITY ? '\xFF' : '\x00';

    // Check for AppleCRC32 and UsedSpace in ZeroVector
    bool hasAppleCrc32 = false;
    UINT32 volumeSize = volume.size();
    UINT32 appleCrc32 = *(UINT32*)(volume.constData() + 8);
    UINT32 usedSpace = *(UINT32*)(volume.constData() + 12);
    if (appleCrc32 != 0) {
        // Calculate CRC32 of the volume body
        UINT32 crc = (UINT32)crc32(0, (const UINT8*)(volume.constData() + volumeHeader->HeaderLength), volumeSize - volumeHeader->HeaderLength);
        if (crc == appleCrc32) {
            hasAppleCrc32 = true;
        }
    }

    // Check header checksum by recalculating it
    bool msgInvalidChecksum = false;
    UByteArray tempHeader((const char*)volumeHeader, volumeHeader->HeaderLength);
    ((EFI_FIRMWARE_VOLUME_HEADER*)tempHeader.data())->Checksum = 0;
    UINT16 calculated = calculateChecksum16((const UINT16*)tempHeader.constData(), volumeHeader->HeaderLength);
    if (volumeHeader->Checksum != calculated)
        msgInvalidChecksum = true;

    // Get info
    UByteArray header = volume.left(headerSize);
    UByteArray body = volume.mid(headerSize);
    UString name = guidToUString(volumeHeader->FileSystemGuid);
    UString info = usprintf("ZeroVector:\n%02X %02X %02X %02X %02X %02X %02X %02X\n"
        "%02X %02X %02X %02X %02X %02X %02X %02X\nSignature: _FVH\nFileSystem GUID: ",
        volumeHeader->ZeroVector[0], volumeHeader->ZeroVector[1], volumeHeader->ZeroVector[2], volumeHeader->ZeroVector[3],
        volumeHeader->ZeroVector[4], volumeHeader->ZeroVector[5], volumeHeader->ZeroVector[6], volumeHeader->ZeroVector[7],
        volumeHeader->ZeroVector[8], volumeHeader->ZeroVector[9], volumeHeader->ZeroVector[10], volumeHeader->ZeroVector[11],
        volumeHeader->ZeroVector[12], volumeHeader->ZeroVector[13], volumeHeader->ZeroVector[14], volumeHeader->ZeroVector[15])
        + guidToUString(volumeHeader->FileSystemGuid, false) \
        + usprintf("\nFull size: %Xh (%u)\nHeader size: %Xh (%u)\nBody size: %Xh (%u)\nRevision: %u\nAttributes: %08Xh\nErase polarity: %u\nChecksum: %04Xh",
        volumeSize, volumeSize,
        headerSize, headerSize,
        volumeSize - headerSize, volumeSize - headerSize,
        volumeHeader->Revision,
        volumeHeader->Attributes,
        (emptyByte ? 1 : 0),
        volumeHeader->Checksum) +
        (msgInvalidChecksum ? usprintf(", invalid, should be %04Xh", calculated) : UString(", valid"));

    // Extended header present
    if (volumeHeader->Revision > 1 && volumeHeader->ExtHeaderOffset) {
        const EFI_FIRMWARE_VOLUME_EXT_HEADER* extendedHeader = (const EFI_FIRMWARE_VOLUME_EXT_HEADER*)(volume.constData() + volumeHeader->ExtHeaderOffset);
        info += usprintf("\nExtended header size: %Xh (%u)\nVolume GUID: ",
            extendedHeader->ExtHeaderSize, extendedHeader->ExtHeaderSize) + guidToUString(extendedHeader->FvName, false);
        name = guidToUString(extendedHeader->FvName); // Replace FFS GUID with volume GUID
    }

    // Add text
    UString text;
    if (hasAppleCrc32)
        text += UString("AppleCRC32 ");

    // Add tree item
    UINT8 subtype = Subtypes::UnknownVolume;
    if (!isUnknown) {
        if (ffsVersion == 2)
            subtype = Subtypes::Ffs2Volume;
        else if (ffsVersion == 3)
            subtype = Subtypes::Ffs3Volume;
        else if (isNvramVolume)
            subtype = Subtypes::NvramVolume;
        else if (isMicrocodeVolume)
            subtype = Subtypes::MicrocodeVolume;
    }
    index = model->addItem(localOffset, Types::Volume, subtype, name, text, info, header, body, UByteArray(), Movable, parent);

    // Set parsing data for created volume
    VOLUME_PARSING_DATA pdata;
    pdata.emptyByte = emptyByte;
    pdata.ffsVersion = ffsVersion;
    pdata.hasExtendedHeader = hasExtendedHeader ? TRUE : FALSE;
    pdata.extendedHeaderGuid = extendedHeaderGuid;
    pdata.alignment = alignment;
    pdata.revision = volumeHeader->Revision;
    pdata.hasAppleCrc32 = hasAppleCrc32;
    pdata.hasValidUsedSpace = FALSE; // Will be updated later, if needed
    pdata.usedSpace = usedSpace;
    pdata.isWeakAligned = (volumeHeader->Revision > 1 && (volumeHeader->Attributes & EFI_FVB2_WEAK_ALIGNMENT));
    model->setParsingData(index, UByteArray((const char*)&pdata, sizeof(pdata)));

    // Show messages
    if (isUnknown)
        msg(usprintf("%s: unknown file system ", __FUNCTION__) + guidToUString(volumeHeader->FileSystemGuid), index);
    if (msgInvalidChecksum)
        msg(usprintf("%s: volume header checksum is invalid", __FUNCTION__), index);
    if (msgAlignmentBitsSet)
        msg(usprintf("%s: alignment bits set on volume without alignment capability", __FUNCTION__), index);
    if (msgUnaligned)
        msg(usprintf("%s: unaligned volume", __FUNCTION__), index);
    if (msgUnknownRevision)
        msg(usprintf("%s: unknown volume revision %u", __FUNCTION__, volumeHeader->Revision), index);

    return U_SUCCESS;
}

BOOLEAN FfsParser::microcodeHeaderValid(const INTEL_MICROCODE_HEADER* ucodeHeader)
{
    // Check main reserved bytes to be zero
    bool reservedBytesValid = true;
    for (UINT32 i = 0; i < sizeof(ucodeHeader->Reserved); i++) {
        if (ucodeHeader->Reserved[i] != 0x00) {
            reservedBytesValid = false;
            break;
        }
    }
    if (!reservedBytesValid) {
        return FALSE;
    }

    // Check CpuFlags reserved bytes to be zero
    for (UINT32 i = 0; i < sizeof(ucodeHeader->ProcessorFlagsReserved); i++) {
        if (ucodeHeader->ProcessorFlagsReserved[i] != 0x00) {
            reservedBytesValid = false;
            break;
        }
    }
    if (!reservedBytesValid) {
        return FALSE;
    }

    // Check data size to be multiple of 4 and less than 0x1000000
    if (ucodeHeader->DataSize % 4 != 0 ||
        ucodeHeader->DataSize > 0xFFFFFF) {
        return FALSE;
    }

    // Check TotalSize to be greater or equal than DataSize and less than 0x1000000
    if (ucodeHeader->TotalSize < ucodeHeader->DataSize ||
        ucodeHeader->TotalSize > 0xFFFFFF) {
        return FALSE;
    }

    // Check date to be sane
    // Check day to be in 0x01-0x09, 0x10-0x19, 0x20-0x29, 0x30-0x31
    if (ucodeHeader->DateDay < 0x01 ||
        (ucodeHeader->DateDay > 0x09 && ucodeHeader->DateDay < 0x10) ||
        (ucodeHeader->DateDay > 0x19 && ucodeHeader->DateDay < 0x20) ||
        (ucodeHeader->DateDay > 0x29 && ucodeHeader->DateDay < 0x30) ||
        ucodeHeader->DateDay > 0x31) {
        return FALSE;
    }
    // Check month to be in 0x01-0x09, 0x10-0x12
    if (ucodeHeader->DateMonth < 0x01 ||
        (ucodeHeader->DateMonth > 0x09 && ucodeHeader->DateMonth < 0x10) ||
        ucodeHeader->DateMonth > 0x12) {
        return FALSE;
    }
    // Check year to be in 0x1990-0x1999, 0x2000-0x2009, 0x2010-0x2019, 0x2020-0x2029, 0x2030-0x2030, 0x2040-0x2049
    if (ucodeHeader->DateYear < 0x1990 ||
        (ucodeHeader->DateYear > 0x1999 && ucodeHeader->DateYear < 0x2000) ||
        (ucodeHeader->DateYear > 0x2009 && ucodeHeader->DateYear < 0x2010) ||
        (ucodeHeader->DateYear > 0x2019 && ucodeHeader->DateYear < 0x2020) ||
        (ucodeHeader->DateYear > 0x2029 && ucodeHeader->DateYear < 0x2030) ||
        (ucodeHeader->DateYear > 0x2039 && ucodeHeader->DateYear < 0x2040) ||
        ucodeHeader->DateYear > 0x2049) {
        return FALSE;
    }
    // Check HeaderVersion to be 1.
    if (ucodeHeader->HeaderVersion != 1) {
        return FALSE;
    }
    // Check LoaderRevision to be 1.
    if (ucodeHeader->LoaderRevision != 1) {
        return FALSE;
    }

    return TRUE;
}

USTATUS FfsParser::findNextRawAreaItem(const UModelIndex & index, const UINT32 localOffset, UINT8 & nextItemType, UINT32 & nextItemOffset, UINT32 & nextItemSize, UINT32 & nextItemAlternativeSize)
{
    UByteArray data = model->body(index);
    UINT32 dataSize = data.size();

    if (dataSize < sizeof(UINT32))
        return U_STORES_NOT_FOUND;

    UINT32 offset = localOffset;
    for (; offset < dataSize - sizeof(UINT32); offset++) {
        const UINT32* currentPos = (const UINT32*)(data.constData() + offset);
        const UINT32 restSize = dataSize - offset;
        if (readUnaligned(currentPos) == INTEL_MICROCODE_HEADER_VERSION_1) {// Intel microcode
            // Check data size
            if (restSize < sizeof(INTEL_MICROCODE_HEADER)) {
                continue;
            }

            // Check microcode header candidate
            const INTEL_MICROCODE_HEADER* ucodeHeader = (const INTEL_MICROCODE_HEADER*)currentPos;
            if (FALSE == microcodeHeaderValid(ucodeHeader)) {
                continue;
            }

            // Check size candidate
            if (ucodeHeader->TotalSize == 0)
                continue;

            // All checks passed, microcode found
            nextItemType = Types::Microcode;
            nextItemSize = ucodeHeader->TotalSize;
            nextItemAlternativeSize = ucodeHeader->TotalSize;
            nextItemOffset = offset;
            break;
        }
        else if (readUnaligned(currentPos) == EFI_FV_SIGNATURE) {
            if (offset < EFI_FV_SIGNATURE_OFFSET)
                continue;

            const EFI_FIRMWARE_VOLUME_HEADER* volumeHeader = (const EFI_FIRMWARE_VOLUME_HEADER*)(data.constData() + offset - EFI_FV_SIGNATURE_OFFSET);
            if (volumeHeader->FvLength < sizeof(EFI_FIRMWARE_VOLUME_HEADER) + 2 * sizeof(EFI_FV_BLOCK_MAP_ENTRY) || volumeHeader->FvLength >= 0xFFFFFFFFUL) {
                continue;
            }
            if (volumeHeader->Revision != 1 && volumeHeader->Revision != 2) {
                continue;
            }

            // Calculate alternative volume size using it's BlockMap
            nextItemAlternativeSize = 0;
            const EFI_FV_BLOCK_MAP_ENTRY* entry = (const EFI_FV_BLOCK_MAP_ENTRY*)(data.constData() + offset - EFI_FV_SIGNATURE_OFFSET + sizeof(EFI_FIRMWARE_VOLUME_HEADER));
            while (entry->NumBlocks != 0 && entry->Length != 0) {
                if ((void*)entry >= data.constData() + data.size()) {
                   continue;
                }

                nextItemAlternativeSize += entry->NumBlocks * entry->Length;
                entry += 1;
            }

            // All checks passed, volume found
            nextItemType = Types::Volume;
            nextItemSize = (UINT32)volumeHeader->FvLength;
            nextItemOffset = offset - EFI_FV_SIGNATURE_OFFSET;
            break;
        }
        else if (readUnaligned(currentPos) == BPDT_GREEN_SIGNATURE || readUnaligned(currentPos) == BPDT_YELLOW_SIGNATURE) {
            // Check data size
            if (restSize < sizeof(BPDT_HEADER))
                continue;

            const BPDT_HEADER *bpdtHeader = (const BPDT_HEADER *)currentPos;
            // Check version
            if (bpdtHeader->HeaderVersion != BPDT_HEADER_VERSION_1) // IFWI 2.0 only for now
                continue;

            UINT32 ptBodySize = bpdtHeader->NumEntries * sizeof(BPDT_ENTRY);
            UINT32 ptSize = sizeof(BPDT_HEADER) + ptBodySize;
            // Check data size again
            if (restSize < ptSize)
                continue;

            UINT32 sizeCandidate = 0;
            // Parse partition table
            const BPDT_ENTRY* firstPtEntry = (const BPDT_ENTRY*)((const UINT8*)bpdtHeader + sizeof(BPDT_HEADER));
            for (UINT16 i = 0; i < bpdtHeader->NumEntries; i++) {
                // Populate entry header
                const BPDT_ENTRY* ptEntry = firstPtEntry + i;
                // Check that entry is present in the image
                if (ptEntry->Offset != 0
                    && ptEntry->Offset != 0xFFFFFFFF
                    && ptEntry->Size != 0
                    && sizeCandidate < ptEntry->Offset + ptEntry->Size) {
                        sizeCandidate = ptEntry->Offset + ptEntry->Size;
                }
            }

            // Check size candidate
            if (sizeCandidate == 0)
                continue;

             // All checks passed, BPDT found
            nextItemType = Types::BpdtStore;
            nextItemSize = sizeCandidate;
            nextItemAlternativeSize = sizeCandidate;
            nextItemOffset = offset;
            break;
        }
    }

    // No more stores found
    if (offset >= dataSize - sizeof(UINT32)) {
        return U_STORES_NOT_FOUND;
    }

    return U_SUCCESS;
}

USTATUS FfsParser::parseVolumeNonUefiData(const UByteArray & data, const UINT32 localOffset, const UModelIndex & index)
{
    // Sanity check
    if (!index.isValid())
        return U_INVALID_PARAMETER;

    // Get info
    UString info = usprintf("Full size: %Xh (%u)", data.size(), data.size());

    // Add padding tree item
    UModelIndex paddingIndex = model->addItem(localOffset, Types::Padding, Subtypes::DataPadding, UString("Non-UEFI data"), UString(), info, UByteArray(), data, UByteArray(), Fixed, index);
    msg(usprintf("%s: non-UEFI data found in volume's free space", __FUNCTION__), paddingIndex);

    // Parse contents as RAW area
    return parseRawArea(paddingIndex);
}

USTATUS FfsParser::parseVolumeBody(const UModelIndex & index)
{
    // Sanity check
    if (!index.isValid())
        return U_INVALID_PARAMETER;

    // Get volume header size and body
    UByteArray volumeBody = model->body(index);
    UINT32 volumeHeaderSize = model->header(index).size();

    // Parse VSS NVRAM volumes with a dedicated function
    if (model->subtype(index) == Subtypes::NvramVolume)
        return nvramParser->parseNvramVolumeBody(index);

    // Parse Microcode volume with a dedicated function
    if (model->subtype(index) == Subtypes::MicrocodeVolume)
        return parseMicrocodeVolumeBody(index);

    // Get required values from parsing data
    UINT8 emptyByte = 0xFF;
    UINT8 ffsVersion = 2;
    UINT32 usedSpace = 0;
    if (model->hasEmptyParsingData(index) == false) {
        UByteArray data = model->parsingData(index);
        const VOLUME_PARSING_DATA* pdata = (const VOLUME_PARSING_DATA*)data.constData();
        emptyByte = pdata->emptyByte;
        ffsVersion = pdata->ffsVersion;
        usedSpace = pdata->usedSpace;
    }

    // Check for unknown FFS version
    if (ffsVersion != 2 && ffsVersion != 3)
        return U_SUCCESS;

    // Search for and parse all files
    UINT32 volumeBodySize = volumeBody.size();
    UINT32 fileOffset = 0;

    while (fileOffset < volumeBodySize) {
        UINT32 fileSize = getFileSize(volumeBody, fileOffset, ffsVersion);

        if (fileSize == 0) {
            msg(usprintf("%s: file header parsing failed with invalid size", __FUNCTION__), index);
            return U_INVALID_PARAMETER;
        }

        // Check that we are at the empty space
        UByteArray header = volumeBody.mid(fileOffset, (int)std::min(sizeof(EFI_FFS_FILE_HEADER), (size_t)volumeBodySize - fileOffset));
        if (header.count(emptyByte) == header.size()) { //Empty space
            // Check volume usedSpace entry to be valid
            if (usedSpace > 0 && usedSpace == fileOffset + volumeHeaderSize) {
                if (model->hasEmptyParsingData(index) == false) {
                    UByteArray data = model->parsingData(index);
                    VOLUME_PARSING_DATA* pdata = (VOLUME_PARSING_DATA*)data.data();
                    pdata->hasValidUsedSpace = TRUE;
                    model->setParsingData(index, data);
                    model->setText(index, model->text(index) + "UsedSpace ");
                }
            }

            // Check free space to be actually free
            UByteArray freeSpace = volumeBody.mid(fileOffset);
            if (freeSpace.count(emptyByte) != freeSpace.size()) {
                // Search for the first non-empty byte
                UINT32 i;
                UINT32 size = freeSpace.size();
                const UINT8* current = (UINT8*)freeSpace.constData();
                for (i = 0; i < size; i++) {
                    if (*current++ != emptyByte)
                        break;
                }

                // Align found index to file alignment
                // It must be possible because minimum 16 bytes of empty were found before
                if (i != ALIGN8(i)) {
                    i = ALIGN8(i) - 8;
                }

                // Add all bytes before as free space
                if (i > 0) {
                    UByteArray free = freeSpace.left(i);

                    // Get info
                    UString info = usprintf("Full size: %Xh (%u)", free.size(), free.size());

                    // Add free space item
                    model->addItem(volumeHeaderSize + fileOffset, Types::FreeSpace, 0, UString("Volume free space"), UString(), info, UByteArray(), free, UByteArray(), Movable, index);
                }

                // Parse non-UEFI data
                parseVolumeNonUefiData(freeSpace.mid(i), volumeHeaderSize + fileOffset + i, index);
            }
            else {
                // Get info
                UString info = usprintf("Full size: %Xh (%u)", freeSpace.size(), freeSpace.size());

                // Add free space item
                model->addItem(volumeHeaderSize + fileOffset, Types::FreeSpace, 0, UString("Volume free space"), UString(), info, UByteArray(), freeSpace, UByteArray(), Movable, index);
            }
            break; // Exit from parsing loop
        }

        // We aren't at the end of empty space
        // Check that the remaining space can still have a file in it
        if (volumeBodySize - fileOffset < sizeof(EFI_FFS_FILE_HEADER) || // Remaining space is smaller than the smallest possible file
            volumeBodySize - fileOffset < fileSize) { // Remaining space is smaller than non-empty file size
            // Parse non-UEFI data
            parseVolumeNonUefiData(volumeBody.mid(fileOffset), volumeHeaderSize + fileOffset, index);
            break; // Exit from parsing loop
        }

        // Parse current file's header
        UModelIndex fileIndex;
        USTATUS result = parseFileHeader(volumeBody.mid(fileOffset, fileSize), volumeHeaderSize + fileOffset, index, fileIndex);
        if (result) {
            msg(usprintf("%s: file header parsing failed with error ", __FUNCTION__) + errorCodeToUString(result), index);
        }

        // Move to next file
        fileOffset += fileSize;
        fileOffset = ALIGN8(fileOffset);
    }

    // Check for duplicate GUIDs
    for (int i = 0; i < model->rowCount(index); i++) {
        UModelIndex current = index.child(i, 0);
        // Skip non-file entries and pad files
        if (model->type(current) != Types::File || model->subtype(current) == EFI_FV_FILETYPE_PAD)
            continue;

        // Get current file GUID
        UByteArray currentGuid(model->header(current).constData(), sizeof(EFI_GUID));

        // Check files after current for having an equal GUID
        for (int j = i + 1; j < model->rowCount(index); j++) {
            UModelIndex another = index.child(j, 0);

            // Skip non-file entries
            if (model->type(another) != Types::File)
                continue;

            // Get another file GUID
            UByteArray anotherGuid(model->header(another).constData(), sizeof(EFI_GUID));

            // Check GUIDs for being equal
            if (currentGuid == anotherGuid) {
                msg(usprintf("%s: file with duplicate GUID ", __FUNCTION__) + guidToUString(readUnaligned((EFI_GUID*)(anotherGuid.data()))), another);
            }
        }
    }

    //Parse bodies
    for (int i = 0; i < model->rowCount(index); i++) {
        UModelIndex current = index.child(i, 0);
        switch (model->type(current)) {
        case Types::File:
            parseFileBody(current);
            break;
        case Types::Padding:
        case Types::FreeSpace:
            // No parsing required
            break;
        default:
            return U_UNKNOWN_ITEM_TYPE;
        }
    }

    return U_SUCCESS;
}

UINT32 FfsParser::getFileSize(const UByteArray & volume, const UINT32 fileOffset, const UINT8 ffsVersion)
{
    if ((UINT32)volume.size() < fileOffset + sizeof(EFI_FFS_FILE_HEADER)) {
        return 0;
    }

    const EFI_FFS_FILE_HEADER* fileHeader = (const EFI_FFS_FILE_HEADER*)(volume.constData() + fileOffset);

    if (ffsVersion == 2) {
        return uint24ToUint32(fileHeader->Size);
    }
    else if (ffsVersion == 3) {
        if (fileHeader->Attributes & FFS_ATTRIB_LARGE_FILE) {
            if ((UINT32)volume.size() < fileOffset + sizeof(EFI_FFS_FILE_HEADER2)) {
                return 0;
            }

            const EFI_FFS_FILE_HEADER2* fileHeader2 = (const EFI_FFS_FILE_HEADER2*)(volume.constData() + fileOffset);
            return (UINT32) fileHeader2->ExtendedSize;
        }

        return uint24ToUint32(fileHeader->Size);
    }

    return 0;
}

USTATUS FfsParser::parseFileHeader(const UByteArray & file, const UINT32 localOffset, const UModelIndex & parent, UModelIndex & index)
{
    // Sanity check
    if (file.isEmpty()) {
        return U_INVALID_PARAMETER;
    }
    if ((UINT32)file.size() < sizeof(EFI_FFS_FILE_HEADER)) {
        return U_INVALID_FILE;
    }

    // Obtain required information from parent volume
    UINT8 ffsVersion = 2;
    bool isWeakAligned = false;
    UINT32 volumeAlignment = 0xFFFFFFFF;
    UINT8 volumeRevision = 2;
    UModelIndex parentVolumeIndex = model->type(parent) == Types::Volume ? parent : model->findParentOfType(parent, Types::Volume);
    if (parentVolumeIndex.isValid() && model->hasEmptyParsingData(parentVolumeIndex) == false) {
        UByteArray data = model->parsingData(parentVolumeIndex);
        const VOLUME_PARSING_DATA* pdata = (const VOLUME_PARSING_DATA*)data.constData();
        ffsVersion = pdata->ffsVersion;
        volumeAlignment = pdata->alignment;
        volumeRevision = pdata->revision;
        isWeakAligned = pdata->isWeakAligned;
    }

    // Get file header
    UByteArray header = file.left(sizeof(EFI_FFS_FILE_HEADER));
    EFI_FFS_FILE_HEADER* tempFileHeader = (EFI_FFS_FILE_HEADER*)header.data();
    if (ffsVersion == 3 && (tempFileHeader->Attributes & FFS_ATTRIB_LARGE_FILE)) {
        if ((UINT32)file.size() < sizeof(EFI_FFS_FILE_HEADER2))
            return U_INVALID_FILE;
        header = file.left(sizeof(EFI_FFS_FILE_HEADER2));
    }
    const EFI_FFS_FILE_HEADER* fileHeader = (const EFI_FFS_FILE_HEADER*)header.constData();

    // Check file alignment
    bool msgUnalignedFile = false;
    UINT8 alignmentPower = ffsAlignmentTable[(fileHeader->Attributes & FFS_ATTRIB_DATA_ALIGNMENT) >> 3];
    if (volumeRevision > 1 && (fileHeader->Attributes & FFS_ATTRIB_DATA_ALIGNMENT2)) {
        alignmentPower = ffsAlignment2Table[(fileHeader->Attributes & FFS_ATTRIB_DATA_ALIGNMENT) >> 3];
    }

    UINT32 alignment = (UINT32)(1UL << alignmentPower);
    if ((localOffset + header.size()) % alignment) {
        msgUnalignedFile = true;
    }

    // Check file alignment agains volume alignment
    bool msgFileAlignmentIsGreaterThanVolumeAlignment = false;
    if (!isWeakAligned && volumeAlignment < alignment) {
        msgFileAlignmentIsGreaterThanVolumeAlignment = true;
    }

    // Get file body
    UByteArray body = file.mid(header.size());

    // Check for file tail presence
    UByteArray tail;
    bool msgInvalidTailValue = false;
    if (volumeRevision == 1 && (fileHeader->Attributes & FFS_ATTRIB_TAIL_PRESENT)) {
        //Check file tail;
        UINT16 tailValue = *(UINT16*)body.right(sizeof(UINT16)).constData();
        if (fileHeader->IntegrityCheck.TailReference != (UINT16)~tailValue)
            msgInvalidTailValue = true;

        // Get tail and remove it from file body
        tail = body.right(sizeof(UINT16));
        body = body.left(body.size() - sizeof(UINT16));
    }

    // Check header checksum
    UINT8 calculatedHeader = 0x100 - (calculateSum8((const UINT8*)header.constData(), header.size()) - fileHeader->IntegrityCheck.Checksum.Header - fileHeader->IntegrityCheck.Checksum.File - fileHeader->State);
    bool msgInvalidHeaderChecksum = false;
    if (fileHeader->IntegrityCheck.Checksum.Header != calculatedHeader) {
        msgInvalidHeaderChecksum = true;
    }

    // Check data checksum
    // Data checksum must be calculated
    bool msgInvalidDataChecksum = false;
    UINT8 calculatedData = 0;
    if (fileHeader->Attributes & FFS_ATTRIB_CHECKSUM) {
        calculatedData = calculateChecksum8((const UINT8*)body.constData(), body.size());
    }
    // Data checksum must be one of predefined values
    else if (volumeRevision == 1) {
        calculatedData = FFS_FIXED_CHECKSUM;
    }
    else {
        calculatedData = FFS_FIXED_CHECKSUM2;
    }

    if (fileHeader->IntegrityCheck.Checksum.File != calculatedData) {
        msgInvalidDataChecksum = true;
    }

    // Check file type
    bool msgUnknownType = false;
    if (fileHeader->Type > EFI_FV_FILETYPE_MM_CORE_STANDALONE && fileHeader->Type != EFI_FV_FILETYPE_PAD) {
        msgUnknownType = true;
    };

    // Get info
    UString name;
    UString info;
    if (fileHeader->Type != EFI_FV_FILETYPE_PAD) {
        name = guidToUString(fileHeader->Name);
    } else {
        name = UString("Pad-file");
    }

    info = UString("File GUID: ") + guidToUString(fileHeader->Name, false) +
        usprintf("\nType: %02Xh\nAttributes: %02Xh\nFull size: %Xh (%u)\nHeader size: %Xh (%u)\nBody size: %Xh (%u)\nTail size: %Xh (%u)\nState: %02Xh",
        fileHeader->Type,
        fileHeader->Attributes,
        header.size() + body.size() + tail.size(), header.size() + body.size() + tail.size(),
        header.size(), header.size(),
        body.size(), body.size(),
        tail.size(), tail.size(),
        fileHeader->State) +
        usprintf("\nHeader checksum: %02Xh", fileHeader->IntegrityCheck.Checksum.Header) + (msgInvalidHeaderChecksum ? usprintf(", invalid, should be %02Xh", calculatedHeader) : UString(", valid")) +
        usprintf("\nData checksum: %02Xh", fileHeader->IntegrityCheck.Checksum.File) + (msgInvalidDataChecksum ? usprintf(", invalid, should be %02Xh", calculatedData) : UString(", valid"));

    UString text;
    bool isVtf = false;
    bool isDxeCore = false;
    // Check if the file is a Volume Top File
    UByteArray fileGuid = UByteArray((const char*)&fileHeader->Name, sizeof(EFI_GUID));
    if (fileGuid == EFI_FFS_VOLUME_TOP_FILE_GUID) {
        // Mark it as the last VTF
        // This information will later be used to determine memory addresses of uncompressed image elements
        // Because the last byte of the last VFT is mapped to 0xFFFFFFFF physical memory address
        isVtf = true;
        text = UString("Volume Top File");
    }
    // Check if the file is the first DXE Core
    else if (fileGuid == EFI_DXE_CORE_GUID || fileGuid == AMI_CORE_DXE_GUID) {
        // Mark is as first DXE core
        // This information may be used to determine DXE volume offset for old AMI or post-IBB protected ranges
        isDxeCore = true;
    }

    // Construct fixed state
    ItemFixedState fixed = (ItemFixedState)((fileHeader->Attributes & FFS_ATTRIB_FIXED) != 0);

    // Add tree item
    index = model->addItem(localOffset, Types::File, fileHeader->Type, name, text, info, header, body, tail, fixed, parent);

    // Set parsing data for created file
    FILE_PARSING_DATA pdata;
    pdata.emptyByte = (fileHeader->State & EFI_FILE_ERASE_POLARITY) ? 0xFF : 0x00;
    pdata.guid = fileHeader->Name;
    model->setParsingData(index, UByteArray((const char*)&pdata, sizeof(pdata)));

    // Override lastVtf index, if needed
    if (isVtf) {
        lastVtf = index;
    }

    // Override first DXE core index, if needed
    if (isDxeCore && !bgDxeCoreIndex.isValid()) {
        bgDxeCoreIndex = index;
    }

    // Show messages
    if (msgUnalignedFile)
        msg(usprintf("%s: unaligned file", __FUNCTION__), index);
    if (msgFileAlignmentIsGreaterThanVolumeAlignment)
        msg(usprintf("%s: file alignment %Xh is greater than parent volume alignment %Xh", __FUNCTION__, alignment, volumeAlignment), index);
    if (msgInvalidHeaderChecksum)
        msg(usprintf("%s: invalid header checksum %02Xh, should be %02Xh", __FUNCTION__, fileHeader->IntegrityCheck.Checksum.Header, calculatedHeader), index);
    if (msgInvalidDataChecksum)
        msg(usprintf("%s: invalid data checksum %02Xh, should be %02Xh", __FUNCTION__, fileHeader->IntegrityCheck.Checksum.File, calculatedData), index);
    if (msgInvalidTailValue)
        msg(usprintf("%s: invalid tail value %04Xh", __FUNCTION__, *(const UINT16*)tail.constData()), index);
    if (msgUnknownType)
        msg(usprintf("%s: unknown file type %02Xh", __FUNCTION__, fileHeader->Type), index);

    return U_SUCCESS;
}

UINT32 FfsParser::getSectionSize(const UByteArray & file, const UINT32 sectionOffset, const UINT8 ffsVersion)
{
    if ((UINT32)file.size() < sectionOffset + sizeof(EFI_COMMON_SECTION_HEADER)) {
        return 0;
    }
    const EFI_COMMON_SECTION_HEADER* sectionHeader = (const EFI_COMMON_SECTION_HEADER*)(file.constData() + sectionOffset);

    if (ffsVersion == 2) {
        return uint24ToUint32(sectionHeader->Size);
    }
    else if (ffsVersion == 3) {
        UINT32 size = uint24ToUint32(sectionHeader->Size);
        if (size == EFI_SECTION2_IS_USED) {
            if ((UINT32)file.size() < sectionOffset + sizeof(EFI_COMMON_SECTION_HEADER2)) {
                return 0;
            }
            const EFI_COMMON_SECTION_HEADER2* sectionHeader2 = (const EFI_COMMON_SECTION_HEADER2*)(file.constData() + sectionOffset);
            return sectionHeader2->ExtendedSize;
        }

        return size;
    }

    return 0;
}

USTATUS FfsParser::parseFileBody(const UModelIndex & index)
{
    // Sanity check
    if (!index.isValid())
        return U_INVALID_PARAMETER;

    // Do not parse non-file bodies
    if (model->type(index) != Types::File)
        return U_SUCCESS;

    // Parse pad-file body
    if (model->subtype(index) == EFI_FV_FILETYPE_PAD)
        return parsePadFileBody(index);

    // Parse raw files as raw areas
    if (model->subtype(index) == EFI_FV_FILETYPE_RAW || model->subtype(index) == EFI_FV_FILETYPE_ALL) {
        UByteArray fileGuid = UByteArray(model->header(index).constData(), sizeof(EFI_GUID));

        // Parse NVAR store
        if (fileGuid == NVRAM_NVAR_STORE_FILE_GUID) {
            model->setText(index, UString("NVAR store"));
            return nvramParser->parseNvarStore(index);
        }
        else if (fileGuid == NVRAM_NVAR_PEI_EXTERNAL_DEFAULTS_FILE_GUID) {
            model->setText(index, UString("NVRAM external defaults"));
            return nvramParser->parseNvarStore(index);
        }
        else if (fileGuid == NVRAM_NVAR_BB_DEFAULTS_FILE_GUID) {
            model->setText(index, UString("NVAR bb defaults"));
            return nvramParser->parseNvarStore(index);
        }
        // Parse vendor hash file
        else if (fileGuid == BG_VENDOR_HASH_FILE_GUID_PHOENIX) {
            return parseVendorHashFile(fileGuid, index);
        }

        return parseRawArea(index);
    }

    // Parse sections
    return parseSections(model->body(index), index, true);
}

USTATUS FfsParser::parsePadFileBody(const UModelIndex & index)
{
    // Sanity check
    if (!index.isValid())
        return U_INVALID_PARAMETER;

    // Check if all bytes of the file are empty
    UByteArray body = model->body(index);

    // Obtain required information from parent file
    UINT8 emptyByte = 0xFF;
    UModelIndex parentFileIndex = model->findParentOfType(index, Types::File);
    if (parentFileIndex.isValid() && model->hasEmptyParsingData(parentFileIndex) == false) {
        UByteArray data = model->parsingData(index);
        const FILE_PARSING_DATA* pdata = (const FILE_PARSING_DATA*)data.constData();
        emptyByte = pdata->emptyByte;
    }

    // Check if the while PAD file is empty
    if (body.size() == body.count(emptyByte))
        return U_SUCCESS;

    // Search for the first non-empty byte
    UINT32 nonEmptyByteOffset;
    UINT32 size = body.size();
    const UINT8* current = (const UINT8*)body.constData();
    for (nonEmptyByteOffset = 0; nonEmptyByteOffset < size; nonEmptyByteOffset++) {
        if (*current++ != emptyByte)
            break;
    }

    // Add all bytes before as free space...
    UINT32 headerSize = model->header(index).size();
    if (nonEmptyByteOffset >= 8) {
        // Align free space to 8 bytes boundary
        if (nonEmptyByteOffset != ALIGN8(nonEmptyByteOffset))
            nonEmptyByteOffset = ALIGN8(nonEmptyByteOffset) - 8;

        UByteArray free = body.left(nonEmptyByteOffset);

        // Get info
        UString info = usprintf("Full size: %Xh (%u)", free.size(), free.size());

        // Add tree item
        model->addItem(headerSize, Types::FreeSpace, 0, UString("Free space"), UString(), info, UByteArray(), free, UByteArray(), Movable, index);
    }
    else {
        nonEmptyByteOffset = 0;
    }

    // ... and all bytes after as a padding
    UByteArray padding = body.mid(nonEmptyByteOffset);

    // Get info
    UString info = usprintf("Full size: %Xh (%u)", padding.size(), padding.size());

    // Add tree item
    UModelIndex dataIndex = model->addItem(headerSize + nonEmptyByteOffset, Types::Padding, Subtypes::DataPadding, UString("Non-UEFI data"), UString(), info, UByteArray(), padding, UByteArray(), Fixed, index);

    // Show message
    msg(usprintf("%s: non-UEFI data found in pad-file", __FUNCTION__), dataIndex);

    // Rename the file
    model->setName(index, UString("Non-empty pad-file"));

    // Parse contents as RAW area
    return parseRawArea(dataIndex);
}

USTATUS FfsParser::parseSections(const UByteArray & sections, const UModelIndex & index, const bool insertIntoTree)
{
    // Sanity check
    if (!index.isValid())
        return U_INVALID_PARAMETER;

    // Search for and parse all sections
    UINT32 bodySize = sections.size();
    UINT32 headerSize = model->header(index).size();
    UINT32 sectionOffset = 0;
    USTATUS result = U_SUCCESS;

    // Obtain required information from parent volume
    UINT8 ffsVersion = 2;
    UModelIndex parentVolumeIndex = model->findParentOfType(index, Types::Volume);
    if (parentVolumeIndex.isValid() && model->hasEmptyParsingData(parentVolumeIndex) == false) {
        UByteArray data = model->parsingData(parentVolumeIndex);
        const VOLUME_PARSING_DATA* pdata = (const VOLUME_PARSING_DATA*)data.constData();
        ffsVersion = pdata->ffsVersion;
    }

    while (sectionOffset < bodySize) {
        // Get section size
        UINT32 sectionSize = getSectionSize(sections, sectionOffset, ffsVersion);

        // Check section size
        if (sectionSize < sizeof(EFI_COMMON_SECTION_HEADER) || sectionSize > (bodySize - sectionOffset)) {
            // Final parsing
            if (insertIntoTree) {
                // Add padding to fill the rest of sections
                UByteArray padding = sections.mid(sectionOffset);

                // Get info
                UString info = usprintf("Full size: %Xh (%u)", padding.size(), padding.size());

                // Add tree item
                UModelIndex dataIndex = model->addItem(headerSize + sectionOffset, Types::Padding, Subtypes::DataPadding, UString("Non-UEFI data"), UString(), info, UByteArray(), padding, UByteArray(), Fixed, index);

                // Show message
                msg(usprintf("%s: non-UEFI data found in sections area", __FUNCTION__), dataIndex);

                // Exit from parsing loop
                break;
            }
            // Preparsing
            else {
                return U_INVALID_SECTION;
            }
        }

        // Parse section header
        UModelIndex sectionIndex;
        result = parseSectionHeader(sections.mid(sectionOffset, sectionSize), headerSize + sectionOffset, index, sectionIndex, insertIntoTree);
        if (result) {
            if (insertIntoTree)
                msg(UString("parseSections: section header parsing failed with error ") + errorCodeToUString(result), index);
            else
                return U_INVALID_SECTION;
        }

        // Move to next section
        sectionOffset += sectionSize;
        sectionOffset = ALIGN4(sectionOffset);
    }

    // Parse bodies, will be skipped if insertIntoTree is not required
    for (int i = 0; i < model->rowCount(index); i++) {
        UModelIndex current = index.child(i, 0);
        switch (model->type(current)) {
        case Types::Section:
            parseSectionBody(current);
            break;
        case Types::Padding:
            // No parsing required
            break;
        default:
            return U_UNKNOWN_ITEM_TYPE;
        }
    }

    return U_SUCCESS;
}

USTATUS FfsParser::parseSectionHeader(const UByteArray & section, const UINT32 localOffset, const UModelIndex & parent, UModelIndex & index, const bool insertIntoTree)
{
    // Check sanity
    if ((UINT32)section.size() < sizeof(EFI_COMMON_SECTION_HEADER)) {
        return U_INVALID_SECTION;
    }

    const EFI_COMMON_SECTION_HEADER* sectionHeader = (const EFI_COMMON_SECTION_HEADER*)(section.constData());
    switch (sectionHeader->Type) {
    // Special
    case EFI_SECTION_COMPRESSION:           return parseCompressedSectionHeader(section, localOffset, parent, index, insertIntoTree);
    case EFI_SECTION_GUID_DEFINED:          return parseGuidedSectionHeader(section, localOffset, parent, index, insertIntoTree);
    case EFI_SECTION_FREEFORM_SUBTYPE_GUID: return parseFreeformGuidedSectionHeader(section, localOffset, parent, index, insertIntoTree);
    case EFI_SECTION_VERSION:               return parseVersionSectionHeader(section, localOffset, parent, index, insertIntoTree);
    case PHOENIX_SECTION_POSTCODE:
    case INSYDE_SECTION_POSTCODE:           return parsePostcodeSectionHeader(section, localOffset, parent, index, insertIntoTree);
    // Common
    case EFI_SECTION_DISPOSABLE:
    case EFI_SECTION_DXE_DEPEX:
    case EFI_SECTION_PEI_DEPEX:
    case EFI_SECTION_MM_DEPEX:
    case EFI_SECTION_PE32:
    case EFI_SECTION_PIC:
    case EFI_SECTION_TE:
    case EFI_SECTION_COMPATIBILITY16:
    case EFI_SECTION_USER_INTERFACE:
    case EFI_SECTION_FIRMWARE_VOLUME_IMAGE:
    case EFI_SECTION_RAW:                   return parseCommonSectionHeader(section, localOffset, parent, index, insertIntoTree);
    // Unknown
    default:
        USTATUS result = parseCommonSectionHeader(section, localOffset, parent, index, insertIntoTree);
        msg(usprintf("%s: section with unknown type %02Xh", __FUNCTION__, sectionHeader->Type), index);
        return result;
    }
}

USTATUS FfsParser::parseCommonSectionHeader(const UByteArray & section, const UINT32 localOffset, const UModelIndex & parent, UModelIndex & index, const bool insertIntoTree)
{
    // Check sanity
    if ((UINT32)section.size() < sizeof(EFI_COMMON_SECTION_HEADER)) {
        return U_INVALID_SECTION;
    }

    // Obtain required information from parent volume
    UINT8 ffsVersion = 2;
    UModelIndex parentVolumeIndex = model->findParentOfType(parent, Types::Volume);
    if (parentVolumeIndex.isValid() && model->hasEmptyParsingData(parentVolumeIndex) == false) {
        UByteArray data = model->parsingData(parentVolumeIndex);
        const VOLUME_PARSING_DATA* pdata = (const VOLUME_PARSING_DATA*)data.constData();
        ffsVersion = pdata->ffsVersion;
    }

    // Obtain header fields
    UINT32 headerSize;
    UINT8  type;
    const EFI_COMMON_SECTION_HEADER_APPLE* appleHeader = (const EFI_COMMON_SECTION_HEADER_APPLE*)(section.constData());
    if ((UINT32)section.size() >= sizeof(EFI_COMMON_SECTION_HEADER_APPLE) && appleHeader->Reserved == EFI_SECTION_APPLE_USED) {
        headerSize = sizeof(EFI_COMMON_SECTION_HEADER_APPLE);
        type = appleHeader->Type;
    }
    else {
        const EFI_COMMON_SECTION_HEADER* sectionHeader = (const EFI_COMMON_SECTION_HEADER*)(section.constData());
        headerSize = sizeof(EFI_COMMON_SECTION_HEADER);
        if (ffsVersion == 3 && uint24ToUint32(sectionHeader->Size) == EFI_SECTION2_IS_USED)
            headerSize = sizeof(EFI_COMMON_SECTION_HEADER2);
        type = sectionHeader->Type;
    }

    // Check sanity again
    if ((UINT32)section.size() < headerSize) {
        return U_INVALID_SECTION;
    }

    UByteArray header = section.left(headerSize);
    UByteArray body = section.mid(headerSize);

    // Get info
    UString name = sectionTypeToUString(type) + UString(" section");
    UString info = usprintf("Type: %02Xh\nFull size: %Xh (%u)\nHeader size: %Xh (%u)\nBody size: %Xh (%u)",
        type,
        section.size(), section.size(),
        headerSize, headerSize,
        body.size(), body.size());

    // Add tree item
    if (insertIntoTree) {
        index = model->addItem(localOffset, Types::Section, type, name, UString(), info, header, body, UByteArray(), Movable, parent);
    }

    return U_SUCCESS;
}

USTATUS FfsParser::parseCompressedSectionHeader(const UByteArray & section, const UINT32 localOffset, const UModelIndex & parent, UModelIndex & index, const bool insertIntoTree)
{
    // Check sanity
    if ((UINT32)section.size() < sizeof(EFI_COMMON_SECTION_HEADER))
        return U_INVALID_SECTION;

    // Obtain required information from parent volume
    UINT8 ffsVersion = 2;
    UModelIndex parentVolumeIndex = model->findParentOfType(parent, Types::Volume);
    if (parentVolumeIndex.isValid() && model->hasEmptyParsingData(parentVolumeIndex) == false) {
        UByteArray data = model->parsingData(parentVolumeIndex);
        const VOLUME_PARSING_DATA* pdata = (const VOLUME_PARSING_DATA*)data.constData();
        ffsVersion = pdata->ffsVersion;
    }

    // Obtain header fields
    UINT32 headerSize;
    UINT8 compressionType;
    UINT32 uncompressedLength;
    const EFI_COMMON_SECTION_HEADER* sectionHeader = (const EFI_COMMON_SECTION_HEADER*)(section.constData());
    const EFI_COMMON_SECTION_HEADER2* section2Header = (const EFI_COMMON_SECTION_HEADER2*)(section.constData());
    const EFI_COMMON_SECTION_HEADER_APPLE* appleHeader = (const EFI_COMMON_SECTION_HEADER_APPLE*)(section.constData());

    if ((UINT32)section.size() >= sizeof(EFI_COMMON_SECTION_HEADER_APPLE) && appleHeader->Reserved == EFI_SECTION_APPLE_USED) { // Check for apple section
        const EFI_COMPRESSION_SECTION_APPLE* appleSectionHeader = (const EFI_COMPRESSION_SECTION_APPLE*)(appleHeader + 1);
        headerSize = sizeof(EFI_COMMON_SECTION_HEADER_APPLE) + sizeof(EFI_COMPRESSION_SECTION_APPLE);
        compressionType = (UINT8)appleSectionHeader->CompressionType;
        uncompressedLength = appleSectionHeader->UncompressedLength;
    }
    else if (ffsVersion == 3 && uint24ToUint32(sectionHeader->Size) == EFI_SECTION2_IS_USED) { // Check for extended header section
        const EFI_COMPRESSION_SECTION* compressedSectionHeader = (const EFI_COMPRESSION_SECTION*)(section2Header + 1);
        if ((UINT32)section.size() < sizeof(EFI_COMMON_SECTION_HEADER2) + sizeof(EFI_COMPRESSION_SECTION))
            return U_INVALID_SECTION;
        headerSize = sizeof(EFI_COMMON_SECTION_HEADER2) + sizeof(EFI_COMPRESSION_SECTION);
        compressionType = compressedSectionHeader->CompressionType;
        uncompressedLength = compressedSectionHeader->UncompressedLength;
    }
    else { // Normal section
        const EFI_COMPRESSION_SECTION* compressedSectionHeader = (const EFI_COMPRESSION_SECTION*)(sectionHeader + 1);
        headerSize = sizeof(EFI_COMMON_SECTION_HEADER) + sizeof(EFI_COMPRESSION_SECTION);
        compressionType = compressedSectionHeader->CompressionType;
        uncompressedLength = compressedSectionHeader->UncompressedLength;
    }

    // Check sanity again
    if ((UINT32)section.size() < headerSize) {
        return U_INVALID_SECTION;
    }

    UByteArray header = section.left(headerSize);
    UByteArray body = section.mid(headerSize);

    // Get info
    UString name = sectionTypeToUString(sectionHeader->Type) + UString(" section");
    UString info = usprintf("Type: %02Xh\nFull size: %Xh (%u)\nHeader size: %Xh (%u)\nBody size: %Xh (%u)\nCompression type: %02Xh\nDecompressed size: %Xh (%u)",
        sectionHeader->Type,
        section.size(), section.size(),
        headerSize, headerSize,
        body.size(), body.size(),
        compressionType,
        uncompressedLength, uncompressedLength);

    // Add tree item
    if (insertIntoTree) {
        index = model->addItem(localOffset, Types::Section, sectionHeader->Type, name, UString(), info, header, body, UByteArray(), Movable, parent);

        // Set section parsing data
        COMPRESSED_SECTION_PARSING_DATA pdata;
        pdata.compressionType = compressionType;
        pdata.uncompressedSize = uncompressedLength;
        model->setParsingData(index, UByteArray((const char*)&pdata, sizeof(pdata)));
    }

    return U_SUCCESS;
}

USTATUS FfsParser::parseGuidedSectionHeader(const UByteArray & section, const UINT32 localOffset, const UModelIndex & parent, UModelIndex & index, const bool insertIntoTree)
{
    // Check sanity
    if ((UINT32)section.size() < sizeof(EFI_COMMON_SECTION_HEADER))
        return U_INVALID_SECTION;

    // Obtain required information from parent volume
    UINT8 ffsVersion = 2;
    UModelIndex parentVolumeIndex = model->findParentOfType(parent, Types::Volume);
    if (parentVolumeIndex.isValid() && model->hasEmptyParsingData(parentVolumeIndex) == false) {
        UByteArray data = model->parsingData(parentVolumeIndex);
        const VOLUME_PARSING_DATA* pdata = (const VOLUME_PARSING_DATA*)data.constData();
        ffsVersion = pdata->ffsVersion;
    }

    // Obtain header fields
    UINT32 headerSize;
    EFI_GUID guid;
    UINT16 dataOffset;
    UINT16 attributes;
    const EFI_COMMON_SECTION_HEADER* sectionHeader = (const EFI_COMMON_SECTION_HEADER*)(section.constData());
    const EFI_COMMON_SECTION_HEADER2* section2Header = (const EFI_COMMON_SECTION_HEADER2*)(section.constData());
    const EFI_COMMON_SECTION_HEADER_APPLE* appleHeader = (const EFI_COMMON_SECTION_HEADER_APPLE*)(section.constData());

    if ((UINT32)section.size() >= sizeof(EFI_COMMON_SECTION_HEADER_APPLE) && appleHeader->Reserved == EFI_SECTION_APPLE_USED) { // Check for apple section
        const EFI_GUID_DEFINED_SECTION_APPLE* appleSectionHeader = (const EFI_GUID_DEFINED_SECTION_APPLE*)(appleHeader + 1);
        headerSize = sizeof(EFI_COMMON_SECTION_HEADER_APPLE) + sizeof(EFI_GUID_DEFINED_SECTION_APPLE);
        if ((UINT32)section.size() < headerSize)
            return U_INVALID_SECTION;
        guid = appleSectionHeader->SectionDefinitionGuid;
        dataOffset = appleSectionHeader->DataOffset;
        attributes = appleSectionHeader->Attributes;
    }
    else if (ffsVersion == 3 && uint24ToUint32(sectionHeader->Size) == EFI_SECTION2_IS_USED) { // Check for extended header section
        const EFI_GUID_DEFINED_SECTION* guidDefinedSectionHeader = (const EFI_GUID_DEFINED_SECTION*)(section2Header + 1);
        if ((UINT32)section.size() < sizeof(EFI_COMMON_SECTION_HEADER2) + sizeof(EFI_GUID_DEFINED_SECTION))
            return U_INVALID_SECTION;
        headerSize = sizeof(EFI_COMMON_SECTION_HEADER2) + sizeof(EFI_GUID_DEFINED_SECTION);
        guid = guidDefinedSectionHeader->SectionDefinitionGuid;
        dataOffset = guidDefinedSectionHeader->DataOffset;
        attributes = guidDefinedSectionHeader->Attributes;
    }
    else { // Normal section
        const EFI_GUID_DEFINED_SECTION* guidDefinedSectionHeader = (const EFI_GUID_DEFINED_SECTION*)(sectionHeader + 1);
        headerSize = sizeof(EFI_COMMON_SECTION_HEADER) + sizeof(EFI_GUID_DEFINED_SECTION);
        guid = guidDefinedSectionHeader->SectionDefinitionGuid;
        dataOffset = guidDefinedSectionHeader->DataOffset;
        attributes = guidDefinedSectionHeader->Attributes;
    }
    // Check sanity again
    if ((UINT32)section.size() < headerSize)
        return U_INVALID_SECTION;

    // Check for special GUIDed sections
    UString additionalInfo;
    UByteArray baGuid((const char*)&guid, sizeof(EFI_GUID));
    bool msgSignedSectionFound = false;
    bool msgNoAuthStatusAttribute = false;
    bool msgNoProcessingRequiredAttributeCompressed = false;
    bool msgNoProcessingRequiredAttributeSigned = false;
    bool msgInvalidCrc = false;
    bool msgUnknownCertType = false;
    bool msgUnknownCertSubtype = false;
    bool msgProcessingRequiredAttributeOnUnknownGuidedSection = false;
    if (baGuid == EFI_GUIDED_SECTION_CRC32) {
        if ((attributes & EFI_GUIDED_SECTION_AUTH_STATUS_VALID) == 0) { // Check that AuthStatusValid attribute is set on compressed GUIDed sections
            msgNoAuthStatusAttribute = true;
        }

        if ((UINT32)section.size() < headerSize + sizeof(UINT32))
            return U_INVALID_SECTION;

        UINT32 crc = *(UINT32*)(section.constData() + headerSize);
        additionalInfo += UString("\nChecksum type: CRC32");
        // Calculate CRC32 of section data
        UINT32 calculated = (UINT32)crc32(0, (const UINT8*)section.constData() + dataOffset, section.size() - dataOffset);
        if (crc == calculated) {
            additionalInfo += usprintf("\nChecksum: %08Xh, valid", crc);
        }
        else {
            additionalInfo += usprintf("\nChecksum: %08Xh, invalid, should be %08Xh", crc, calculated);
            msgInvalidCrc = true;
        }
        // No need to change dataOffset here
    }
    else if (baGuid == EFI_GUIDED_SECTION_LZMA || baGuid == EFI_GUIDED_SECTION_LZMAF86 || baGuid == EFI_GUIDED_SECTION_TIANO || baGuid == EFI_GUIDED_SECTION_GZIP) {
        if ((attributes & EFI_GUIDED_SECTION_PROCESSING_REQUIRED) == 0) { // Check that ProcessingRequired attribute is set on compressed GUIDed sections
            msgNoProcessingRequiredAttributeCompressed = true;
        }
        // No need to change dataOffset here
    }
    else if (baGuid == EFI_CERT_TYPE_RSA2048_SHA256_GUID) {
        if ((attributes & EFI_GUIDED_SECTION_PROCESSING_REQUIRED) == 0) { // Check that ProcessingRequired attribute is set on signed GUIDed sections
            msgNoProcessingRequiredAttributeSigned = true;
        }

        // Get certificate type and length
        if ((UINT32)section.size() < headerSize + sizeof(EFI_CERT_BLOCK_RSA2048_SHA256))
            return U_INVALID_SECTION;

        // Adjust dataOffset
        dataOffset += sizeof(EFI_CERT_BLOCK_RSA2048_SHA256);
        additionalInfo += UString("\nCertificate type: RSA2048/SHA256");
        msgSignedSectionFound = true;
    }
    else if (baGuid == EFI_FIRMWARE_CONTENTS_SIGNED_GUID) {
        if ((attributes & EFI_GUIDED_SECTION_PROCESSING_REQUIRED) == 0) { // Check that ProcessingRequired attribute is set on signed GUIDed sections
            msgNoProcessingRequiredAttributeSigned = true;
        }

        // Get certificate type and length
        if ((UINT32)section.size() < headerSize + sizeof(WIN_CERTIFICATE))
            return U_INVALID_SECTION;

        const WIN_CERTIFICATE* winCertificate = (const WIN_CERTIFICATE*)(section.constData() + headerSize);
        UINT32 certLength = winCertificate->Length;
        UINT16 certType = winCertificate->CertificateType;

        // Adjust dataOffset
        dataOffset += certLength;

        // Check section size once again
        if ((UINT32)section.size() < dataOffset)
            return U_INVALID_SECTION;

        // Check certificate type
        if (certType == WIN_CERT_TYPE_EFI_GUID) {
            additionalInfo += UString("\nCertificate type: UEFI");

            // Get certificate GUID
            const WIN_CERTIFICATE_UEFI_GUID* winCertificateUefiGuid = (const WIN_CERTIFICATE_UEFI_GUID*)(section.constData() + headerSize);
            UByteArray certTypeGuid((const char*)&winCertificateUefiGuid->CertType, sizeof(EFI_GUID));

            if (certTypeGuid == EFI_CERT_TYPE_RSA2048_SHA256_GUID) {
                additionalInfo += UString("\nCertificate subtype: RSA2048/SHA256");
            }
            else {
                additionalInfo += UString("\nCertificate subtype: unknown, GUID ") + guidToUString(winCertificateUefiGuid->CertType);
                msgUnknownCertSubtype = true;
            }
        }
        else {
            additionalInfo += usprintf("\nCertificate type: unknown (%04Xh)", certType);
            msgUnknownCertType = true;
        }
        msgSignedSectionFound = true;
    }
    // Check that ProcessingRequired attribute is not set on GUIDed sections with unknown GUID
    else if ((attributes & EFI_GUIDED_SECTION_PROCESSING_REQUIRED) == EFI_GUIDED_SECTION_PROCESSING_REQUIRED) {
        msgProcessingRequiredAttributeOnUnknownGuidedSection = true;
    }

    UByteArray header = section.left(dataOffset);
    UByteArray body = section.mid(dataOffset);

    // Get info
    UString name = guidToUString(guid);
    UString info = UString("Section GUID: ") + guidToUString(guid, false) +
        usprintf("\nType: %02Xh\nFull size: %Xh (%u)\nHeader size: %Xh (%u)\nBody size: %Xh (%u)\nData offset: %Xh\nAttributes: %04Xh",
        sectionHeader->Type,
        section.size(), section.size(),
        header.size(), header.size(),
        body.size(), body.size(),
        dataOffset,
        attributes);

    // Append additional info
    info += additionalInfo;

    // Add tree item
    if (insertIntoTree) {
        index = model->addItem(localOffset, Types::Section, sectionHeader->Type, name, UString(), info, header, body, UByteArray(), Movable, parent);

        // Set parsing data
        GUIDED_SECTION_PARSING_DATA pdata;
        pdata.guid = guid;
        model->setParsingData(index, UByteArray((const char*)&pdata, sizeof(pdata)));

        // Show messages
        if (msgSignedSectionFound)
            msg(usprintf("%s: section signature may become invalid after any modification", __FUNCTION__), index);
        if (msgNoAuthStatusAttribute)
            msg(usprintf("%s: CRC32 GUIDed section without AuthStatusValid attribute", __FUNCTION__), index);
        if (msgNoProcessingRequiredAttributeCompressed)
            msg(usprintf("%s: compressed GUIDed section without ProcessingRequired attribute", __FUNCTION__), index);
        if (msgNoProcessingRequiredAttributeSigned)
            msg(usprintf("%s: signed GUIDed section without ProcessingRequired attribute", __FUNCTION__), index);
        if (msgInvalidCrc)
            msg(usprintf("%s: GUID defined section with invalid CRC32", __FUNCTION__), index);
        if (msgUnknownCertType)
            msg(usprintf("%s: signed GUIDed section with unknown type", __FUNCTION__), index);
        if (msgUnknownCertSubtype)
            msg(usprintf("%s: signed GUIDed section with unknown subtype", __FUNCTION__), index);
        if (msgProcessingRequiredAttributeOnUnknownGuidedSection)
            msg(usprintf("%s: processing required bit set for GUIDed section with unknown GUID", __FUNCTION__), index);
    }

    return U_SUCCESS;
}

USTATUS FfsParser::parseFreeformGuidedSectionHeader(const UByteArray & section, const UINT32 localOffset, const UModelIndex & parent, UModelIndex & index, const bool insertIntoTree)
{
    // Check sanity
    if ((UINT32)section.size() < sizeof(EFI_COMMON_SECTION_HEADER))
        return U_INVALID_SECTION;

    // Obtain required information from parent volume
    UINT8 ffsVersion = 2;
    UModelIndex parentVolumeIndex = model->findParentOfType(parent, Types::Volume);
    if (parentVolumeIndex.isValid() && model->hasEmptyParsingData(parentVolumeIndex) == false) {
        UByteArray data = model->parsingData(parentVolumeIndex);
        const VOLUME_PARSING_DATA* pdata = (const VOLUME_PARSING_DATA*)data.constData();
        ffsVersion = pdata->ffsVersion;
    }

    // Obtain header fields
    UINT32 headerSize;
    EFI_GUID guid;
    UINT8 type;
    const EFI_COMMON_SECTION_HEADER* sectionHeader = (const EFI_COMMON_SECTION_HEADER*)(section.constData());
    const EFI_COMMON_SECTION_HEADER2* section2Header = (const EFI_COMMON_SECTION_HEADER2*)(section.constData());
    const EFI_COMMON_SECTION_HEADER_APPLE* appleHeader = (const EFI_COMMON_SECTION_HEADER_APPLE*)(section.constData());

    if ((UINT32)section.size() >= sizeof(EFI_COMMON_SECTION_HEADER_APPLE) && appleHeader->Reserved == EFI_SECTION_APPLE_USED) { // Check for apple section
        const EFI_FREEFORM_SUBTYPE_GUID_SECTION* appleSectionHeader = (const EFI_FREEFORM_SUBTYPE_GUID_SECTION*)(appleHeader + 1);
        headerSize = sizeof(EFI_COMMON_SECTION_HEADER_APPLE) + sizeof(EFI_FREEFORM_SUBTYPE_GUID_SECTION);
        guid = appleSectionHeader->SubTypeGuid;
        type = appleHeader->Type;
    }
    else if (ffsVersion == 3 && uint24ToUint32(sectionHeader->Size) == EFI_SECTION2_IS_USED) { // Check for extended header section
        const EFI_FREEFORM_SUBTYPE_GUID_SECTION* fsgSectionHeader = (const EFI_FREEFORM_SUBTYPE_GUID_SECTION*)(section2Header + 1);
        if ((UINT32)section.size() < sizeof(EFI_COMMON_SECTION_HEADER2) + sizeof(EFI_FREEFORM_SUBTYPE_GUID_SECTION))
            return U_INVALID_SECTION;
        headerSize = sizeof(EFI_COMMON_SECTION_HEADER2) + sizeof(EFI_FREEFORM_SUBTYPE_GUID_SECTION);
        guid = fsgSectionHeader->SubTypeGuid;
        type = section2Header->Type;
    }
    else { // Normal section
        const EFI_FREEFORM_SUBTYPE_GUID_SECTION* fsgSectionHeader = (const EFI_FREEFORM_SUBTYPE_GUID_SECTION*)(sectionHeader + 1);
        headerSize = sizeof(EFI_COMMON_SECTION_HEADER) + sizeof(EFI_FREEFORM_SUBTYPE_GUID_SECTION);
        guid = fsgSectionHeader->SubTypeGuid;
        type = sectionHeader->Type;
    }

    // Check sanity again
    if ((UINT32)section.size() < headerSize)
        return U_INVALID_SECTION;

    UByteArray header = section.left(headerSize);
    UByteArray body = section.mid(headerSize);

    // Get info
    UString name = sectionTypeToUString(type) + (" section");
    UString info = usprintf("Type: %02Xh\nFull size: %Xh (%u)\nHeader size: %Xh (%u)\nBody size: %Xh (%u)\nSubtype GUID: ",
        type,
        section.size(), section.size(),
        header.size(), header.size(),
        body.size(), body.size())
        + guidToUString(guid, false);

    // Add tree item
    if (insertIntoTree) {
        index = model->addItem(localOffset, Types::Section, type, name, UString(), info, header, body, UByteArray(), Movable, parent);

        // Set parsing data
        FREEFORM_GUIDED_SECTION_PARSING_DATA pdata;
        pdata.guid = guid;
        model->setParsingData(index, UByteArray((const char*)&pdata, sizeof(pdata)));

        // Rename section
        model->setName(index, guidToUString(guid));
    }

    return U_SUCCESS;
}

USTATUS FfsParser::parseVersionSectionHeader(const UByteArray & section, const UINT32 localOffset, const UModelIndex & parent, UModelIndex & index, const bool insertIntoTree)
{
    // Check sanity
    if ((UINT32)section.size() < sizeof(EFI_COMMON_SECTION_HEADER))
        return U_INVALID_SECTION;

    // Obtain required information from parent volume
    UINT8 ffsVersion = 2;
    UModelIndex parentVolumeIndex = model->findParentOfType(parent, Types::Volume);
    if (parentVolumeIndex.isValid() && model->hasEmptyParsingData(parentVolumeIndex) == false) {
        UByteArray data = model->parsingData(parentVolumeIndex);
        const VOLUME_PARSING_DATA* pdata = (const VOLUME_PARSING_DATA*)data.constData();
        ffsVersion = pdata->ffsVersion;
    }

    // Obtain header fields
    UINT32 headerSize;
    UINT16 buildNumber;
    UINT8 type;
    const EFI_COMMON_SECTION_HEADER* sectionHeader = (const EFI_COMMON_SECTION_HEADER*)(section.constData());
    const EFI_COMMON_SECTION_HEADER2* section2Header = (const EFI_COMMON_SECTION_HEADER2*)(section.constData());
    const EFI_COMMON_SECTION_HEADER_APPLE* appleHeader = (const EFI_COMMON_SECTION_HEADER_APPLE*)(section.constData());

    if ((UINT32)section.size() >= sizeof(EFI_COMMON_SECTION_HEADER_APPLE) && appleHeader->Reserved == EFI_SECTION_APPLE_USED) { // Check for apple section
        const EFI_VERSION_SECTION* versionHeader = (const EFI_VERSION_SECTION*)(appleHeader + 1);
        headerSize = sizeof(EFI_COMMON_SECTION_HEADER_APPLE) + sizeof(EFI_VERSION_SECTION);
        buildNumber = versionHeader->BuildNumber;
        type = appleHeader->Type;
    }
    else if (ffsVersion == 3 && uint24ToUint32(sectionHeader->Size) == EFI_SECTION2_IS_USED) { // Check for extended header section
        const EFI_VERSION_SECTION* versionHeader = (const EFI_VERSION_SECTION*)(section2Header + 1);
        headerSize = sizeof(EFI_COMMON_SECTION_HEADER2) + sizeof(EFI_VERSION_SECTION);
        buildNumber = versionHeader->BuildNumber;
        type = section2Header->Type;
    }
    else { // Normal section
        const EFI_VERSION_SECTION* versionHeader = (const EFI_VERSION_SECTION*)(sectionHeader + 1);
        headerSize = sizeof(EFI_COMMON_SECTION_HEADER) + sizeof(EFI_VERSION_SECTION);
        buildNumber = versionHeader->BuildNumber;
        type = sectionHeader->Type;
    }

    // Check sanity again
    if ((UINT32)section.size() < headerSize)
        return U_INVALID_SECTION;

    UByteArray header = section.left(headerSize);
    UByteArray body = section.mid(headerSize);

    // Get info
    UString name = sectionTypeToUString(type) + (" section");
    UString info = usprintf("Type: %02Xh\nFull size: %Xh (%u)\nHeader size: %Xh (%u)\nBody size: %Xh (%u)\nBuild number: %u",
        type,
        section.size(), section.size(),
        header.size(), header.size(),
        body.size(), body.size(),
        buildNumber);

    // Add tree item
    if (insertIntoTree) {
        index = model->addItem(localOffset, Types::Section, type, name, UString(), info, header, body, UByteArray(), Movable, parent);
    }

    return U_SUCCESS;
}

USTATUS FfsParser::parsePostcodeSectionHeader(const UByteArray & section, const UINT32 localOffset, const UModelIndex & parent, UModelIndex & index, const bool insertIntoTree)
{
    // Check sanity
    if ((UINT32)section.size() < sizeof(EFI_COMMON_SECTION_HEADER))
        return U_INVALID_SECTION;

    // Obtain required information from parent volume
    UINT8 ffsVersion = 2;
    UModelIndex parentVolumeIndex = model->findParentOfType(parent, Types::Volume);
    if (parentVolumeIndex.isValid() && model->hasEmptyParsingData(parentVolumeIndex) == false) {
        UByteArray data = model->parsingData(parentVolumeIndex);
        const VOLUME_PARSING_DATA* pdata = (const VOLUME_PARSING_DATA*)data.constData();
        ffsVersion = pdata->ffsVersion;
    }

    // Obtain header fields
    UINT32 headerSize;
    UINT32 postCode;
    UINT8 type;
    const EFI_COMMON_SECTION_HEADER* sectionHeader = (const EFI_COMMON_SECTION_HEADER*)(section.constData());
    const EFI_COMMON_SECTION_HEADER2* section2Header = (const EFI_COMMON_SECTION_HEADER2*)(section.constData());
    const EFI_COMMON_SECTION_HEADER_APPLE* appleHeader = (const EFI_COMMON_SECTION_HEADER_APPLE*)(section.constData());

    if ((UINT32)section.size() >= sizeof(EFI_COMMON_SECTION_HEADER_APPLE) && appleHeader->Reserved == EFI_SECTION_APPLE_USED) { // Check for apple section
        const POSTCODE_SECTION* postcodeHeader = (const POSTCODE_SECTION*)(appleHeader + 1);
        headerSize = sizeof(EFI_COMMON_SECTION_HEADER_APPLE) + sizeof(POSTCODE_SECTION);
        postCode = postcodeHeader->Postcode;
        type = appleHeader->Type;
    }
    else if (ffsVersion == 3 && uint24ToUint32(sectionHeader->Size) == EFI_SECTION2_IS_USED) { // Check for extended header section
        const POSTCODE_SECTION* postcodeHeader = (const POSTCODE_SECTION*)(section2Header + 1);
        headerSize = sizeof(EFI_COMMON_SECTION_HEADER2) + sizeof(POSTCODE_SECTION);
        postCode = postcodeHeader->Postcode;
        type = section2Header->Type;
    }
    else { // Normal section
        const POSTCODE_SECTION* postcodeHeader = (const POSTCODE_SECTION*)(sectionHeader + 1);
        headerSize = sizeof(EFI_COMMON_SECTION_HEADER) + sizeof(POSTCODE_SECTION);
        postCode = postcodeHeader->Postcode;
        type = sectionHeader->Type;
    }

    // Check sanity again
    if ((UINT32)section.size() < headerSize)
        return U_INVALID_SECTION;

    UByteArray header = section.left(headerSize);
    UByteArray body = section.mid(headerSize);

    // Get info
    UString name = sectionTypeToUString(type) + (" section");
    UString info = usprintf("Type: %02Xh\nFull size: %Xh (%u)\nHeader size: %Xh (%u)\nBody size: %Xh (%u)\nPostcode: %Xh",
        type,
        section.size(), section.size(),
        header.size(), header.size(),
        body.size(), body.size(),
        postCode);

    // Add tree item
    if (insertIntoTree) {
        index = model->addItem(localOffset, Types::Section, sectionHeader->Type, name, UString(), info, header, body, UByteArray(), Movable, parent);
    }

    return U_SUCCESS;
}

USTATUS FfsParser::parseSectionBody(const UModelIndex & index)
{
    // Sanity check
    if (!index.isValid())
        return U_INVALID_PARAMETER;
    UByteArray header = model->header(index);
    if ((UINT32)header.size() < sizeof(EFI_COMMON_SECTION_HEADER))
        return U_INVALID_SECTION;

    const EFI_COMMON_SECTION_HEADER* sectionHeader = (const EFI_COMMON_SECTION_HEADER*)(header.constData());

    switch (sectionHeader->Type) {
    // Encapsulation
    case EFI_SECTION_COMPRESSION:           return parseCompressedSectionBody(index);
    case EFI_SECTION_GUID_DEFINED:          return parseGuidedSectionBody(index);
    case EFI_SECTION_DISPOSABLE:            return parseSections(model->body(index), index, true);
    // Leaf
    case EFI_SECTION_FREEFORM_SUBTYPE_GUID: return parseRawArea(index);
    case EFI_SECTION_VERSION:               return parseVersionSectionBody(index);
    case EFI_SECTION_DXE_DEPEX:
    case EFI_SECTION_PEI_DEPEX:
    case EFI_SECTION_MM_DEPEX:              return parseDepexSectionBody(index);
    case EFI_SECTION_TE:                    return parseTeImageSectionBody(index);
    case EFI_SECTION_PE32:
    case EFI_SECTION_PIC:                   return parsePeImageSectionBody(index);
    case EFI_SECTION_USER_INTERFACE:        return parseUiSectionBody(index);
    case EFI_SECTION_FIRMWARE_VOLUME_IMAGE: return parseRawArea(index);
    case EFI_SECTION_RAW:                   return parseRawSectionBody(index);
    // No parsing needed
    case EFI_SECTION_COMPATIBILITY16:
    case PHOENIX_SECTION_POSTCODE:
    case INSYDE_SECTION_POSTCODE:
    default:
        return U_SUCCESS;
    }
}

USTATUS FfsParser::parseCompressedSectionBody(const UModelIndex & index)
{
    // Sanity check
    if (!index.isValid())
        return U_INVALID_PARAMETER;

    // Obtain required information from parsing data
    UINT8 compressionType = EFI_NOT_COMPRESSED;
    UINT32 uncompressedSize = model->body(index).size();
    if (model->hasEmptyParsingData(index) == false) {
        UByteArray data = model->parsingData(index);
        const COMPRESSED_SECTION_PARSING_DATA* pdata = (const COMPRESSED_SECTION_PARSING_DATA*)data.constData();
        compressionType = readUnaligned(pdata).compressionType;
        uncompressedSize = readUnaligned(pdata).uncompressedSize;
    }

    // Decompress section
    UINT8 algorithm = COMPRESSION_ALGORITHM_NONE;
    UINT32 dictionarySize = 0;
    UByteArray decompressed;
    UByteArray efiDecompressed;
    USTATUS result = decompress(model->body(index), compressionType, algorithm, dictionarySize, decompressed, efiDecompressed);
    if (result) {
        msg(UString("parseCompressedSectionBody: decompression failed with error ") + errorCodeToUString(result), index);
        return U_SUCCESS;
    }

    // Check reported uncompressed size
    if (uncompressedSize != (UINT32)decompressed.size()) {
        msg(usprintf("parseCompressedSectionBody: decompressed size stored in header %Xh (%u) differs from actual %Xh (%u)",
            uncompressedSize, uncompressedSize,
            decompressed.size(), decompressed.size()),
            index);
        model->addInfo(index, usprintf("\nActual decompressed size: %Xh (%u)", decompressed.size(), decompressed.size()));
    }

    // Check for undecided compression algorithm, this is a special case
    if (algorithm == COMPRESSION_ALGORITHM_UNDECIDED) {
        // Try preparse of sections decompressed with Tiano algorithm
        if (U_SUCCESS == parseSections(decompressed, index, false)) {
            algorithm = COMPRESSION_ALGORITHM_TIANO;
        }
        // Try preparse of sections decompressed with EFI 1.1 algorithm
        else if (U_SUCCESS == parseSections(efiDecompressed, index, false)) {
            algorithm = COMPRESSION_ALGORITHM_EFI11;
            decompressed = efiDecompressed;
        }
        else {
            msg(UString("parseCompressedSectionBody: can't guess the correct decompression algorithm, both preparse steps are failed"), index);
        }
    }

    // Add info
    model->addInfo(index, UString("\nCompression algorithm: ") + compressionTypeToUString(algorithm));
    if (algorithm == COMPRESSION_ALGORITHM_LZMA || algorithm == COMPRESSION_ALGORITHM_LZMA_INTEL_LEGACY) {
        model->addInfo(index, usprintf("\nLZMA dictionary size: %Xh", dictionarySize));
    }

    // Update parsing data
    COMPRESSED_SECTION_PARSING_DATA pdata;
    pdata.algorithm = algorithm;
    pdata.dictionarySize = dictionarySize;
    pdata.compressionType = compressionType;
    pdata.uncompressedSize = uncompressedSize;
    model->setParsingData(index, UByteArray((const char*)&pdata, sizeof(pdata)));

    if (algorithm != COMPRESSION_ALGORITHM_NONE)
        model->setCompressed(index, true);

    // Parse decompressed data
    return parseSections(decompressed, index, true);
}

USTATUS FfsParser::parseGuidedSectionBody(const UModelIndex & index)
{
    // Sanity check
    if (!index.isValid())
        return U_INVALID_PARAMETER;

    // Obtain required information from parsing data
    EFI_GUID guid = { 0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0 }};
    if (model->hasEmptyParsingData(index) == false) {
        UByteArray data = model->parsingData(index);
        const GUIDED_SECTION_PARSING_DATA* pdata = (const GUIDED_SECTION_PARSING_DATA*)data.constData();
        guid = readUnaligned(pdata).guid;
    }

    // Check if section requires processing
    UByteArray processed = model->body(index);
    UByteArray efiDecompressed;
    UString info;
    bool parseCurrentSection = true;
    UINT8 algorithm = COMPRESSION_ALGORITHM_NONE;
    UINT32 dictionarySize = 0;
    UByteArray baGuid = UByteArray((const char*)&guid, sizeof(EFI_GUID));
    // Tiano compressed section
    if (baGuid == EFI_GUIDED_SECTION_TIANO) {
        USTATUS result = decompress(model->body(index), EFI_STANDARD_COMPRESSION, algorithm, dictionarySize, processed, efiDecompressed);
        if (result) {
            msg(usprintf("%s: decompression failed with error ", __FUNCTION__) + errorCodeToUString(result), index);
            return U_SUCCESS;
        }

        // Check for undecided compression algorithm, this is a special case
        if (algorithm == COMPRESSION_ALGORITHM_UNDECIDED) {
            // Try preparse of sections decompressed with Tiano algorithm
            if (U_SUCCESS == parseSections(processed, index, false)) {
                algorithm = COMPRESSION_ALGORITHM_TIANO;
            }
            // Try preparse of sections decompressed with EFI 1.1 algorithm
            else if (U_SUCCESS == parseSections(efiDecompressed, index, false)) {
                algorithm = COMPRESSION_ALGORITHM_EFI11;
                processed = efiDecompressed;
            }
            else {
                msg(usprintf("%s: can't guess the correct decompression algorithm, both preparse steps are failed", __FUNCTION__), index);
                parseCurrentSection = false;
            }
        }

        info += UString("\nCompression algorithm: ") + compressionTypeToUString(algorithm);
        info += usprintf("\nDecompressed size: %Xh (%u)", processed.size(), processed.size());
    }
    // LZMA compressed section
    else if (baGuid == EFI_GUIDED_SECTION_LZMA) {
        USTATUS result = decompress(model->body(index), EFI_CUSTOMIZED_COMPRESSION, algorithm, dictionarySize, processed, efiDecompressed);
        if (result) {
            msg(usprintf("%s: decompression failed with error ", __FUNCTION__) + errorCodeToUString(result), index);
            return U_SUCCESS;
        }

        if (algorithm == COMPRESSION_ALGORITHM_LZMA) {
            info += UString("\nCompression algorithm: LZMA");
            info += usprintf("\nDecompressed size: %Xh (%u)", processed.size(), processed.size());
            info += usprintf("\nLZMA dictionary size: %Xh", dictionarySize);
        }
        else {
            info += UString("\nCompression algorithm: unknown");
            parseCurrentSection = false;
        }
    }
    // LZMAF86 compressed section
    else if (baGuid == EFI_GUIDED_SECTION_LZMAF86) {
        USTATUS result = decompress(model->body(index), EFI_CUSTOMIZED_COMPRESSION_LZMAF86, algorithm, dictionarySize, processed, efiDecompressed);
        if (result) {
            msg(usprintf("%s: decompression failed with error ", __FUNCTION__) + errorCodeToUString(result), index);
            return U_SUCCESS;
        }

        if (algorithm == COMPRESSION_ALGORITHM_LZMAF86) {
            info += UString("\nCompression algorithm: LZMAF86");
            info += usprintf("\nDecompressed size: %Xh (%u)", processed.size(), processed.size());
            info += usprintf("\nLZMA dictionary size: %Xh", dictionarySize);
        }
        else {
            info += UString("\nCompression algorithm: unknown");
            parseCurrentSection = false;
        }
    }
    // GZip compressed section
    else if (baGuid == EFI_GUIDED_SECTION_GZIP) {
        USTATUS result = gzipDecompress(model->body(index), processed);
        if (result) {
            msg(usprintf("%s: decompression failed with error ", __FUNCTION__) + errorCodeToUString(result), index);
            return U_SUCCESS;
        }

        info += UString("\nCompression algorithm: GZip");
        info += usprintf("\nDecompressed size: %Xh (%u)", processed.size(), processed.size());
    }

    // Add info
    model->addInfo(index, info);

    // Update data
    if (algorithm != COMPRESSION_ALGORITHM_NONE)
        model->setCompressed(index, true);

    // Set parsing data
    GUIDED_SECTION_PARSING_DATA pdata;
    pdata.dictionarySize = dictionarySize;
    model->setParsingData(index, UByteArray((const char*)&pdata, sizeof(pdata)));

    if (!parseCurrentSection) {
        msg(usprintf("%s: GUID defined section can not be processed", __FUNCTION__), index);
        return U_SUCCESS;
    }

    return parseSections(processed, index, true);
}

USTATUS FfsParser::parseVersionSectionBody(const UModelIndex & index)
{
    // Sanity check
    if (!index.isValid())
        return U_INVALID_PARAMETER;

    // Add info
    model->addInfo(index, UString("\nVersion string: ") + UString::fromUtf16((const CHAR16*)model->body(index).constData()));

    return U_SUCCESS;
}

USTATUS FfsParser::parseDepexSectionBody(const UModelIndex & index)
{
    // Sanity check
    if (!index.isValid())
        return U_INVALID_PARAMETER;

    UByteArray body = model->body(index);
    UString parsed;

    // Check data to be present
    if (body.size() < 2) { // 2 is a minimal sane value, i.e TRUE + END
        msg(usprintf("%s: DEPEX section too short", __FUNCTION__), index);
        return U_DEPEX_PARSE_FAILED;
    }

    const EFI_GUID * guid;
    const UINT8* current = (const UINT8*)body.constData();

    // Special cases of first opcode
    switch (*current) {
    case EFI_DEP_BEFORE:
        if (body.size() != 2 * EFI_DEP_OPCODE_SIZE + sizeof(EFI_GUID)) {
            msg(usprintf("%s: DEPEX section too long for a section starting with BEFORE opcode", __FUNCTION__), index);
            return U_SUCCESS;
        }
        guid = (const EFI_GUID*)(current + EFI_DEP_OPCODE_SIZE);
        parsed += UString("\nBEFORE ") + guidToUString(readUnaligned(guid));
        current += EFI_DEP_OPCODE_SIZE + sizeof(EFI_GUID);
        if (*current != EFI_DEP_END){
            msg(usprintf("%s: DEPEX section ends with non-END opcode", __FUNCTION__), index);
            return U_SUCCESS;
        }
        return U_SUCCESS;
    case EFI_DEP_AFTER:
        if (body.size() != 2 * EFI_DEP_OPCODE_SIZE + sizeof(EFI_GUID)){
            msg(usprintf("%s: DEPEX section too long for a section starting with AFTER opcode", __FUNCTION__), index);
            return U_SUCCESS;
        }
        guid = (const EFI_GUID*)(current + EFI_DEP_OPCODE_SIZE);
        parsed += UString("\nAFTER ") + guidToUString(readUnaligned(guid));
        current += EFI_DEP_OPCODE_SIZE + sizeof(EFI_GUID);
        if (*current != EFI_DEP_END) {
            msg(usprintf("%s: DEPEX section ends with non-END opcode", __FUNCTION__), index);
            return U_SUCCESS;
        }
        return U_SUCCESS;
    case EFI_DEP_SOR:
        if (body.size() <= 2 * EFI_DEP_OPCODE_SIZE) {
            msg(usprintf("%s: DEPEX section too short for a section starting with SOR opcode", __FUNCTION__), index);
            return U_SUCCESS;
        }
        parsed += UString("\nSOR");
        current += EFI_DEP_OPCODE_SIZE;
        break;
    }

    // Parse the rest of depex
    while (current - (const UINT8*)body.constData() < body.size()) {
        switch (*current) {
        case EFI_DEP_BEFORE: {
            msg(usprintf("%s: misplaced BEFORE opcode", __FUNCTION__), index);
            return U_SUCCESS;
        }
        case EFI_DEP_AFTER: {
            msg(usprintf("%s: misplaced AFTER opcode", __FUNCTION__), index);
            return U_SUCCESS;
        }
        case EFI_DEP_SOR: {
            msg(usprintf("%s: misplaced SOR opcode", __FUNCTION__), index);
            return U_SUCCESS;
        }
        case EFI_DEP_PUSH:
            // Check that the rest of depex has correct size
            if ((UINT32)body.size() - (UINT32)(current - (const UINT8*)body.constData()) <= EFI_DEP_OPCODE_SIZE + sizeof(EFI_GUID)) {
                parsed.clear();
                msg(usprintf("%s: remains of DEPEX section too short for PUSH opcode", __FUNCTION__), index);
                return U_SUCCESS;
            }
            guid = (const EFI_GUID*)(current + EFI_DEP_OPCODE_SIZE);
            parsed += UString("\nPUSH ") + guidToUString(readUnaligned(guid));
            current += EFI_DEP_OPCODE_SIZE + sizeof(EFI_GUID);
            break;
        case EFI_DEP_AND:
            parsed += UString("\nAND");
            current += EFI_DEP_OPCODE_SIZE;
            break;
        case EFI_DEP_OR:
            parsed += UString("\nOR");
            current += EFI_DEP_OPCODE_SIZE;
            break;
        case EFI_DEP_NOT:
            parsed += UString("\nNOT");
            current += EFI_DEP_OPCODE_SIZE;
            break;
        case EFI_DEP_TRUE:
            parsed += UString("\nTRUE");
            current += EFI_DEP_OPCODE_SIZE;
            break;
        case EFI_DEP_FALSE:
            parsed += UString("\nFALSE");
            current += EFI_DEP_OPCODE_SIZE;
            break;
        case EFI_DEP_END:
            parsed += UString("\nEND");
            current += EFI_DEP_OPCODE_SIZE;
            // Check that END is the last opcode
            if (current - (const UINT8*)body.constData() < body.size()) {
                parsed.clear();
                msg(usprintf("%s: DEPEX section ends with non-END opcode", __FUNCTION__), index);
            }
            break;
        default:
            msg(usprintf("%s: unknown opcode %02Xh", __FUNCTION__, *current), index);
            return U_SUCCESS;
            break;
        }
    }

    // Add info
    model->addInfo(index, UString("\nParsed expression:") + parsed);

    return U_SUCCESS;
}

USTATUS FfsParser::parseUiSectionBody(const UModelIndex & index)
{
    // Sanity check
    if (!index.isValid())
        return U_INVALID_PARAMETER;

    UString text = UString::fromUtf16((const CHAR16*)model->body(index).constData());

    // Add info
    model->addInfo(index, UString("\nText: ") + text);

    // Rename parent file
    model->setText(model->findParentOfType(index, Types::File), text);

    return U_SUCCESS;
}

USTATUS FfsParser::parseAprioriRawSection(const UByteArray & body, UString & parsed)
{
    // Sanity check
    if (body.size() % sizeof(EFI_GUID)) {
        msg(usprintf("%s: apriori file has size is not a multiple of 16", __FUNCTION__));
    }
    parsed.clear();
    UINT32 count = body.size() / sizeof(EFI_GUID);
    if (count > 0) {
        for (UINT32 i = 0; i < count; i++) {
            const EFI_GUID* guid = (const EFI_GUID*)body.constData() + i;
            parsed += UString("\n") + guidToUString(readUnaligned(guid));
        }
    }

    return U_SUCCESS;
}

USTATUS FfsParser::parseRawSectionBody(const UModelIndex & index)
{
    // Sanity check
    if (!index.isValid())
        return U_INVALID_PARAMETER;

    // Check for apriori file
    UModelIndex parentFile = model->findParentOfType(index, Types::File);
    if (!parentFile.isValid())
        return U_INVALID_FILE; //TODO: better return code

    // Get parent file parsing data
    UByteArray parentFileGuid(model->header(parentFile).constData(), sizeof(EFI_GUID));
    if (parentFileGuid == EFI_PEI_APRIORI_FILE_GUID) { // PEI apriori file
        // Set parent file text
        model->setText(parentFile, UString("PEI apriori file"));
        // Parse apriori file list
        UString str;
        USTATUS result = parseAprioriRawSection(model->body(index), str);
        if (!result && !str.isEmpty())
            model->addInfo(index, UString("\nFile list:") + str);
        return result;
    }
    else if (parentFileGuid == EFI_DXE_APRIORI_FILE_GUID) { // DXE apriori file
        // Rename parent file
        model->setText(parentFile, UString("DXE apriori file"));
        // Parse apriori file list
        UString str;
        USTATUS result = parseAprioriRawSection(model->body(index), str);
        if (!result && !str.isEmpty())
            model->addInfo(index, UString("\nFile list:") + str);
        return result;
    }
    else if (parentFileGuid == NVRAM_NVAR_EXTERNAL_DEFAULTS_FILE_GUID) { // AMI NVRAM external defaults
        // Rename parent file
        model->setText(parentFile, UString("NVRAM external defaults"));
        // Parse NVAR area
        return nvramParser->parseNvarStore(index);
    }
    else if (parentFileGuid == BG_VENDOR_HASH_FILE_GUID_AMI) { // AMI vendor hash file
        // Parse AMI vendor hash file
        return parseVendorHashFile(parentFileGuid, index);
    }

    // Parse as raw area
    return parseRawArea(index);
}


USTATUS FfsParser::parsePeImageSectionBody(const UModelIndex & index)
{
    // Sanity check
    if (!index.isValid())
        return U_INVALID_PARAMETER;

    // Get section body
    UByteArray body = model->body(index);
    if ((UINT32)body.size() < sizeof(EFI_IMAGE_DOS_HEADER)) {
        msg(usprintf("%s: section body size is smaller than DOS header size", __FUNCTION__), index);
        return U_SUCCESS;
    }

    UString info;
    const EFI_IMAGE_DOS_HEADER* dosHeader = (const EFI_IMAGE_DOS_HEADER*)body.constData();
    if (dosHeader->e_magic != EFI_IMAGE_DOS_SIGNATURE) {
        info += usprintf("\nDOS signature: %04Xh, invalid", dosHeader->e_magic);
        msg(usprintf("%s: PE32 image with invalid DOS signature", __FUNCTION__), index);
        model->addInfo(index, info);
        return U_SUCCESS;
    }

    const EFI_IMAGE_PE_HEADER* peHeader = (EFI_IMAGE_PE_HEADER*)(body.constData() + dosHeader->e_lfanew);
    if (body.size() < (UINT8*)peHeader - (UINT8*)dosHeader) {
        info += UString("\nDOS header: invalid");
        msg(usprintf("%s: PE32 image with invalid DOS header", __FUNCTION__), index);
        model->addInfo(index, info);
        return U_SUCCESS;
    }

    if (peHeader->Signature != EFI_IMAGE_PE_SIGNATURE) {
        info += usprintf("\nPE signature: %08Xh, invalid", peHeader->Signature);
        msg(usprintf("%s: PE32 image with invalid PE signature", __FUNCTION__), index);
        model->addInfo(index, info);
        return U_SUCCESS;
    }

    const EFI_IMAGE_FILE_HEADER* imageFileHeader = (const EFI_IMAGE_FILE_HEADER*)(peHeader + 1);
    if (body.size() < (UINT8*)imageFileHeader - (UINT8*)dosHeader) {
        info += UString("\nPE header: invalid");
        msg(usprintf("%s: PE32 image with invalid PE header", __FUNCTION__), index);
        model->addInfo(index, info);
        return U_SUCCESS;
    }

    info += usprintf("\nDOS signature: %04Xh\nPE signature: %08Xh",
        dosHeader->e_magic,
        peHeader->Signature) +
        UString("\nMachine type: ") + machineTypeToUString(imageFileHeader->Machine) +
        usprintf("\nNumber of sections: %u\nCharacteristics: %04Xh",
        imageFileHeader->NumberOfSections,
        imageFileHeader->Characteristics);

    EFI_IMAGE_OPTIONAL_HEADER_POINTERS_UNION optionalHeader;
    optionalHeader.H32 = (const EFI_IMAGE_OPTIONAL_HEADER32*)(imageFileHeader + 1);
    if (body.size() < (UINT8*)optionalHeader.H32 - (UINT8*)dosHeader) {
        info += UString("\nPE optional header: invalid");
        msg(usprintf("%s: PE32 image with invalid PE optional header", __FUNCTION__), index);
        model->addInfo(index, info);
        return U_SUCCESS;
    }

    if (optionalHeader.H32->Magic == EFI_IMAGE_PE_OPTIONAL_HDR32_MAGIC) {
        info += usprintf("\nOptional header signature: %04Xh\nSubsystem: %04Xh\nAddress of entry point: %Xh\nBase of code: %Xh\nImage base: %Xh",
            optionalHeader.H32->Magic,
            optionalHeader.H32->Subsystem,
            optionalHeader.H32->AddressOfEntryPoint,
            optionalHeader.H32->BaseOfCode,
            optionalHeader.H32->ImageBase);
    }
    else if (optionalHeader.H32->Magic == EFI_IMAGE_PE_OPTIONAL_HDR64_MAGIC) {
        info += usprintf("\nOptional header signature: %04Xh\nSubsystem: %04Xh\nAddress of entry point: %Xh\nBase of code: %Xh\nImage base: %" PRIX64 "h",
            optionalHeader.H64->Magic,
            optionalHeader.H64->Subsystem,
            optionalHeader.H64->AddressOfEntryPoint,
            optionalHeader.H64->BaseOfCode,
            optionalHeader.H64->ImageBase);
    }
    else {
        info += usprintf("\nOptional header signature: %04Xh, unknown", optionalHeader.H32->Magic);
        msg(usprintf("%s: PE32 image with invalid optional PE header signature", __FUNCTION__), index);
    }

    model->addInfo(index, info);
    return U_SUCCESS;
}


USTATUS FfsParser::parseTeImageSectionBody(const UModelIndex & index)
{
    // Check sanity
    if (!index.isValid())
        return U_INVALID_PARAMETER;

    // Get section body
    UByteArray body = model->body(index);
    if ((UINT32)body.size() < sizeof(EFI_IMAGE_TE_HEADER)) {
        msg(usprintf("%s: section body size is smaller than TE header size", __FUNCTION__), index);
        return U_SUCCESS;
    }

    UString info;
    const EFI_IMAGE_TE_HEADER* teHeader = (const EFI_IMAGE_TE_HEADER*)body.constData();
    if (teHeader->Signature != EFI_IMAGE_TE_SIGNATURE) {
        info += usprintf("\nSignature: %04Xh, invalid", teHeader->Signature);
        msg(usprintf("%s: TE image with invalid TE signature", __FUNCTION__), index);
    }
    else {
        info += usprintf("\nSignature: %04Xh", teHeader->Signature) +
            UString("\nMachine type: ") + machineTypeToUString(teHeader->Machine) +
            usprintf("\nNumber of sections: %u\nSubsystem: %02Xh\nStripped size: %Xh (%u)\n"
            "Base of code: %Xh\nAddress of entry point: %Xh\nImage base: %" PRIX64 "h\nAdjusted image base: %" PRIX64 "h",
            teHeader->NumberOfSections,
            teHeader->Subsystem,
            teHeader->StrippedSize, teHeader->StrippedSize,
            teHeader->BaseOfCode,
            teHeader->AddressOfEntryPoint,
            teHeader->ImageBase,
            teHeader->ImageBase + teHeader->StrippedSize - sizeof(EFI_IMAGE_TE_HEADER));
    }

    // Update parsing data
    TE_IMAGE_SECTION_PARSING_DATA pdata;
    pdata.imageBaseType = EFI_IMAGE_TE_BASE_OTHER; // Will be determined later
    pdata.originalImageBase = (UINT32)teHeader->ImageBase;
    pdata.adjustedImageBase = (UINT32)(teHeader->ImageBase + teHeader->StrippedSize - sizeof(EFI_IMAGE_TE_HEADER));
    model->setParsingData(index, UByteArray((const char*)&pdata, sizeof(pdata)));

    // Add TE info
    model->addInfo(index, info);

    return U_SUCCESS;
}


USTATUS FfsParser::performSecondPass(const UModelIndex & index)
{
    // Sanity check
    if (!index.isValid() || !lastVtf.isValid())
        return U_INVALID_PARAMETER;

    // Check for compressed lastVtf
    if (model->compressed(lastVtf)) {
        msg(usprintf("%s: the last VTF appears inside compressed item, the image may be damaged", __FUNCTION__), lastVtf);
        return U_SUCCESS;
    }

    // Calculate address difference
    const UINT32 vtfSize = model->header(lastVtf).size() + model->body(lastVtf).size() + model->tail(lastVtf).size();
    addressDiff = 0xFFFFFFFFULL - model->base(lastVtf) - vtfSize + 1;

    // Parse reset vector data
    parseResetVectorData();

    // Find and parse FIT
    parseFit(index);

    // Check protected ranges
    checkProtectedRanges(index);

    // Check TE files to have original or adjusted base
    checkTeImageBase(index);

    return U_SUCCESS;
}

USTATUS FfsParser::parseResetVectorData()
{
    // Sanity check
    if (!lastVtf.isValid())
        return U_SUCCESS;

    // Check VTF to have enough space at the end to fit Reset Vector Data
    UByteArray vtf = model->header(lastVtf) + model->body(lastVtf) + model->tail(lastVtf);
    if ((UINT32)vtf.size() < sizeof(X86_RESET_VECTOR_DATA))
        return U_SUCCESS;

    const X86_RESET_VECTOR_DATA* resetVectorData = (const X86_RESET_VECTOR_DATA*)(vtf.constData() + vtf.size() - sizeof(X86_RESET_VECTOR_DATA));

    // Add info
    UString info = usprintf("\nAP entry vector: %02X %02X %02X %02X %02X %02X %02X %02X\n"
                            "Reset vector: %02X %02X %02X %02X %02X %02X %02X %02X\n"
                            "PEI core entry point: %08Xh\n"
                            "AP startup segment: %08X\n"
                            "BootFV base address: %08X\n",
        resetVectorData->ApEntryVector[0], resetVectorData->ApEntryVector[1], resetVectorData->ApEntryVector[2], resetVectorData->ApEntryVector[3],
        resetVectorData->ApEntryVector[4], resetVectorData->ApEntryVector[5], resetVectorData->ApEntryVector[6], resetVectorData->ApEntryVector[7],
        resetVectorData->ResetVector[0], resetVectorData->ResetVector[1], resetVectorData->ResetVector[2], resetVectorData->ResetVector[3],
        resetVectorData->ResetVector[4], resetVectorData->ResetVector[5], resetVectorData->ResetVector[6], resetVectorData->ResetVector[7],
        resetVectorData->PeiCoreEntryPoint,
        resetVectorData->ApStartupSegment,
        resetVectorData->BootFvBaseAddress);

    model->addInfo(lastVtf, info);
    return U_SUCCESS;
}

USTATUS FfsParser::checkTeImageBase(const UModelIndex & index)
{
    // Sanity check
    if (!index.isValid())
        return U_SUCCESS;

    // Determine relocation type of uncompressed TE image sections
    if (model->compressed(index) == false
        && model->type(index) == Types::Section
        && model->subtype(index) == EFI_SECTION_TE) {
        // Obtain required values from parsing data
        UINT32 originalImageBase = 0;
        UINT32 adjustedImageBase = 0;
        UINT8  imageBaseType = EFI_IMAGE_TE_BASE_OTHER;
        if (model->hasEmptyParsingData(index) == false) {
            UByteArray data = model->parsingData(index);
            const TE_IMAGE_SECTION_PARSING_DATA* pdata = (const TE_IMAGE_SECTION_PARSING_DATA*)data.constData();
            originalImageBase = readUnaligned(pdata).originalImageBase;
            adjustedImageBase = readUnaligned(pdata).adjustedImageBase;
        }

        if (originalImageBase != 0 || adjustedImageBase != 0) {
            // Check data memory address to be equal to either OriginalImageBase or AdjustedImageBase
            UINT64 address = addressDiff + model->base(index);
            UINT32 base = (UINT32)address + model->header(index).size();

            if (originalImageBase == base) {
                imageBaseType = EFI_IMAGE_TE_BASE_ORIGINAL;
            }
            else if (adjustedImageBase == base) {
                imageBaseType = EFI_IMAGE_TE_BASE_ADJUSTED;
            }
            else {
                // Check for one-bit difference
                UINT32 xored = base ^ originalImageBase; // XOR result can't be zero
                if ((xored & (xored - 1)) == 0) { // Check that XOR result is a power of 2, i.e. has exactly one bit set
                    imageBaseType = EFI_IMAGE_TE_BASE_ORIGINAL;
                }
                else { // The same check for adjustedImageBase
                    xored = base ^ adjustedImageBase;
                    if ((xored & (xored - 1)) == 0) {
                        imageBaseType = EFI_IMAGE_TE_BASE_ADJUSTED;
                    }
                }
            }

            // Show message if imageBaseType is still unknown
            if (imageBaseType == EFI_IMAGE_TE_BASE_OTHER) {
                msg(usprintf("%s: TE image base is neither zero, nor original, nor adjusted, nor top-swapped", __FUNCTION__), index);
            }

            // Update parsing data
            TE_IMAGE_SECTION_PARSING_DATA pdata;
            pdata.imageBaseType = imageBaseType;
            pdata.originalImageBase = originalImageBase;
            pdata.adjustedImageBase = adjustedImageBase;
            model->setParsingData(index, UByteArray((const char*)&pdata, sizeof(pdata)));
        }
    }

    // Process child items
    for (int i = 0; i < model->rowCount(index); i++) {
        checkTeImageBase(index.child(i, 0));
    }

    return U_SUCCESS;
}

USTATUS FfsParser::addInfoRecursive(const UModelIndex & index)
{
    // Sanity check
    if (!index.isValid())
        return U_INVALID_PARAMETER;

    // Add offset
    model->addInfo(index, usprintf("Offset: %Xh\n", model->offset(index)), false);

    // Add current base if the element is not compressed
    // or it's compressed, but it's parent isn't
    if ((!model->compressed(index)) || (index.parent().isValid() && !model->compressed(index.parent()))) {
        // Add physical address of the whole item or it's header and data portions separately
        UINT64 address = addressDiff + model->base(index);
        if (address <= 0xFFFFFFFFUL) {
            UINT32 headerSize = model->header(index).size();
            if (headerSize) {
                model->addInfo(index, usprintf("Data address: %08Xh\n", address + headerSize),false);
                model->addInfo(index, usprintf("Header address: %08Xh\n", address), false);
            }
            else {
                model->addInfo(index, usprintf("Address: %08Xh\n", address), false);
            }
        }
        // Add base
        model->addInfo(index, usprintf("Base: %Xh\n", model->base(index)), false);
    }
    model->addInfo(index, usprintf("Fixed: %s\n", model->fixed(index) ? "Yes" : "No"), false);

    // Process child items
    for (int i = 0; i < model->rowCount(index); i++) {
        addInfoRecursive(index.child(i, 0));
    }

    return U_SUCCESS;
}

USTATUS FfsParser::checkProtectedRanges(const UModelIndex & index)
{
    // Sanity check
    if (!index.isValid())
        return U_INVALID_PARAMETER;

    // Calculate digest for BG-protected ranges
    UByteArray protectedParts;
    bool bgProtectedRangeFound = false;
    try {
        for (UINT32 i = 0; i < (UINT32)bgProtectedRanges.size(); i++) {
            if (bgProtectedRanges[i].Type == BG_PROTECTED_RANGE_INTEL_BOOT_GUARD_IBB && bgProtectedRanges[i].Size > 0) {
                bgProtectedRangeFound = true;
                if ((UINT64)bgProtectedRanges[i].Offset >= addressDiff) {
                    bgProtectedRanges[i].Offset -= (UINT32)addressDiff;
                } else {
                    // TODO: Explore this.
                    msg(usprintf("%s: Suspicious BG protection offset", __FUNCTION__), index);
                }
                protectedParts += openedImage.mid(bgProtectedRanges[i].Offset, bgProtectedRanges[i].Size);
                markProtectedRangeRecursive(index, bgProtectedRanges[i]);
            }
        }
    } catch (...) {
        bgProtectedRangeFound = false;
    }

    if (bgProtectedRangeFound) {
        UByteArray digest(SHA256_DIGEST_SIZE, '\x00');
        sha256(protectedParts.constData(), protectedParts.size(), digest.data());

        if (digest != bgBpDigest) {
            msg(usprintf("%s: BG-protected ranges hash mismatch, opened image may refuse to boot", __FUNCTION__), index);
        }
    }
    else if (bgBootPolicyFound) {
        msg(usprintf("%s: BootPolicy doesn't define any BG-protected ranges", __FUNCTION__), index);
    }

    // Calculate digests for vendor-protected ranges
    for (UINT32 i = 0; i < (UINT32)bgProtectedRanges.size(); i++) {
        if (bgProtectedRanges[i].Type == BG_PROTECTED_RANGE_VENDOR_HASH_AMI_OLD
            && bgProtectedRanges[i].Size != 0 && bgProtectedRanges[i].Size != 0xFFFFFFFF) {
            if (!bgDxeCoreIndex.isValid()) {
                msg(usprintf("%s: can't determine DXE volume offset, old AMI protected range hash can't be checked", __FUNCTION__), index);
            }
            else {
                // Offset will be determined as the offset of root volume with first DXE core
                UModelIndex dxeRootVolumeIndex = model->findLastParentOfType(bgDxeCoreIndex, Types::Volume);
                if (!dxeRootVolumeIndex.isValid()) {
                    msg(usprintf("%s: can't determine DXE volume offset, old AMI protected range hash can't be checked", __FUNCTION__), index);
                }
                else {
                    bgProtectedRanges[i].Offset = model->base(dxeRootVolumeIndex);
                    protectedParts = openedImage.mid(bgProtectedRanges[i].Offset, bgProtectedRanges[i].Size);

                    UByteArray digest(SHA256_DIGEST_SIZE, '\x00');
                    sha256(protectedParts.constData(), protectedParts.size(), digest.data());

                    if (digest != bgProtectedRanges[i].Hash) {
                        msg(usprintf("%s: old AMI protected range [%Xh:%Xh] hash mismatch, opened image may refuse to boot", __FUNCTION__,
                            bgProtectedRanges[i].Offset, bgProtectedRanges[i].Offset + bgProtectedRanges[i].Size),
                            model->findByBase(bgProtectedRanges[i].Offset));
                    }

                    markProtectedRangeRecursive(index, bgProtectedRanges[i]);
                }
            }
        }
        else if (bgProtectedRanges[i].Type == BG_PROTECTED_RANGE_INTEL_BOOT_GUARD_POST_IBB) {
            if (!bgDxeCoreIndex.isValid()) {
                msg(usprintf("%s: can't determine DXE volume offset, post-IBB protected range hash can't be checked", __FUNCTION__), index);
            }
            else {
                // Offset will be determined as the offset of root volume with first DXE core
                UModelIndex dxeRootVolumeIndex = model->findLastParentOfType(bgDxeCoreIndex, Types::Volume);
                if (!dxeRootVolumeIndex.isValid()) {
                    msg(usprintf("%s: can't determine DXE volume offset, post-IBB protected range hash can't be checked", __FUNCTION__), index);
                }
                else
                {
                    bgProtectedRanges[i].Offset = model->base(dxeRootVolumeIndex);
                    bgProtectedRanges[i].Size = model->header(dxeRootVolumeIndex).size() + model->body(dxeRootVolumeIndex).size() + model->tail(dxeRootVolumeIndex).size();
                    protectedParts = openedImage.mid(bgProtectedRanges[i].Offset, bgProtectedRanges[i].Size);

                    UByteArray digest(SHA256_DIGEST_SIZE, '\x00');
                    sha256(protectedParts.constData(), protectedParts.size(), digest.data());

                    if (digest != bgProtectedRanges[i].Hash) {
                        msg(usprintf("%s: post-IBB protected range [%Xh:%Xh] hash mismatch, opened image may refuse to boot", __FUNCTION__,
                            bgProtectedRanges[i].Offset, bgProtectedRanges[i].Offset + bgProtectedRanges[i].Size),
                            model->findByBase(bgProtectedRanges[i].Offset));
                    }

                    markProtectedRangeRecursive(index, bgProtectedRanges[i]);
                }
            }
        }
        else if (bgProtectedRanges[i].Type == BG_PROTECTED_RANGE_VENDOR_HASH_AMI_NEW
            && bgProtectedRanges[i].Size != 0 && bgProtectedRanges[i].Size != 0xFFFFFFFF
            && bgProtectedRanges[i].Offset != 0 && bgProtectedRanges[i].Offset != 0xFFFFFFFF) {

            bgProtectedRanges[i].Offset -= (UINT32)addressDiff;
            protectedParts = openedImage.mid(bgProtectedRanges[i].Offset, bgProtectedRanges[i].Size);

            UByteArray digest(SHA256_DIGEST_SIZE, '\x00');
            sha256(protectedParts.constData(), protectedParts.size(), digest.data());

            if (digest != bgProtectedRanges[i].Hash) {
                msg(usprintf("%s: AMI protected range [%Xh:%Xh] hash mismatch, opened image may refuse to boot", __FUNCTION__,
                    bgProtectedRanges[i].Offset, bgProtectedRanges[i].Offset + bgProtectedRanges[i].Size),
                    model->findByBase(bgProtectedRanges[i].Offset));
            }

            markProtectedRangeRecursive(index, bgProtectedRanges[i]);
        }
        else if (bgProtectedRanges[i].Type == BG_PROTECTED_RANGE_VENDOR_HASH_PHOENIX
            && bgProtectedRanges[i].Size != 0 && bgProtectedRanges[i].Size != 0xFFFFFFFF
            && bgProtectedRanges[i].Offset != 0xFFFFFFFF) {
            bgProtectedRanges[i].Offset += (UINT32)bgProtectedRegionsBase;
            protectedParts = openedImage.mid(bgProtectedRanges[i].Offset, bgProtectedRanges[i].Size);

            UByteArray digest(SHA256_DIGEST_SIZE, '\x00');
            sha256(protectedParts.constData(), protectedParts.size(), digest.data());

            if (digest != bgProtectedRanges[i].Hash) {
                msg(usprintf("%s: Phoenix protected range [%Xh:%Xh] hash mismatch, opened image may refuse to boot", __FUNCTION__,
                    bgProtectedRanges[i].Offset, bgProtectedRanges[i].Offset + bgProtectedRanges[i].Size),
                    model->findByBase(bgProtectedRanges[i].Offset));
            }

            markProtectedRangeRecursive(index, bgProtectedRanges[i]);
        }
        else if (bgProtectedRanges[i].Type == BG_PROTECTED_RANGE_VENDOR_HASH_MICROSOFT
            && bgProtectedRanges[i].Size != 0 && bgProtectedRanges[i].Size != 0xFFFFFFFF
            && bgProtectedRanges[i].Offset != 0 && bgProtectedRanges[i].Offset != 0xFFFFFFFF) {
            bgProtectedRanges[i].Offset -= (UINT32)addressDiff;
            protectedParts = openedImage.mid(bgProtectedRanges[i].Offset, bgProtectedRanges[i].Size);

            UByteArray digest(SHA256_DIGEST_SIZE, '\x00');
            sha256(protectedParts.constData(), protectedParts.size(), digest.data());

            if (digest != bgProtectedRanges[i].Hash) {
                msg(usprintf("%s: Microsoft protected range [%Xh:%Xh] hash mismatch, opened image may refuse to boot", __FUNCTION__,
                    bgProtectedRanges[i].Offset, bgProtectedRanges[i].Offset + bgProtectedRanges[i].Size),
                    model->findByBase(bgProtectedRanges[i].Offset));
            }

            markProtectedRangeRecursive(index, bgProtectedRanges[i]);
        }
    }

    return U_SUCCESS;
}

USTATUS FfsParser::markProtectedRangeRecursive(const UModelIndex & index, const BG_PROTECTED_RANGE & range)
{
    if (!index.isValid())
        return U_SUCCESS;

    // Mark compressed items
    UModelIndex parentIndex = model->parent(index);
    if (parentIndex.isValid() && model->compressed(index) && model->compressed(parentIndex)) {
        model->setMarking(index, model->marking(parentIndex));
    }
    // Mark normal items
    else {
        UINT32 currentOffset = model->base(index);
        UINT32 currentSize = model->header(index).size() + model->body(index).size() + model->tail(index).size();

        if (std::min(currentOffset + currentSize, range.Offset + range.Size) > std::max(currentOffset, range.Offset)) {
            if (range.Offset <= currentOffset && currentOffset + currentSize <= range.Offset + range.Size) { // Mark as fully in range
                if (range.Type == BG_PROTECTED_RANGE_INTEL_BOOT_GUARD_IBB) {
                    model->setMarking(index, Qt::red);
                }
                else {
                    model->setMarking(index, Qt::cyan);
                }
            }
            else { // Mark as partially in range
                model->setMarking(index, Qt::yellow);
            }
        }
    }

    for (int i = 0; i < model->rowCount(index); i++) {
        markProtectedRangeRecursive(index.child(i, 0), range);
    }

    return U_SUCCESS;
}

USTATUS FfsParser::parseVendorHashFile(const UByteArray & fileGuid, const UModelIndex & index)
{
    if (!index.isValid())
        return EFI_INVALID_PARAMETER;

    if (fileGuid == BG_VENDOR_HASH_FILE_GUID_PHOENIX) {
        const UByteArray &body = model->body(index);
        UINT32 size = (UINT32)body.size();

        // File too small to have even a signature
        if (size < sizeof(BG_VENDOR_HASH_FILE_SIGNATURE_PHOENIX)) {
            msg(usprintf("%s: unknown or corrupted Phoenix hash file found", __FUNCTION__), index);
            model->setText(index, UString("Phoenix hash file"));
            return U_INVALID_FILE;
        }

        const BG_VENDOR_HASH_FILE_HEADER_PHOENIX* header = (const BG_VENDOR_HASH_FILE_HEADER_PHOENIX*)body.constData();
        if (header->Signature == BG_VENDOR_HASH_FILE_SIGNATURE_PHOENIX) {
            if (size < sizeof(BG_VENDOR_HASH_FILE_HEADER_PHOENIX) ||
                size < sizeof(BG_VENDOR_HASH_FILE_HEADER_PHOENIX) + header->NumEntries * sizeof(BG_VENDOR_HASH_FILE_ENTRY)) {
                msg(usprintf("%s: unknown or corrupted Phoenix hash file found", __FUNCTION__), index);
                model->setText(index, UString("Phoenix hash file"));
                return U_INVALID_FILE;
            }

            if (header->NumEntries > 0) {
                bool protectedRangesFound = false;
                for (UINT32 i = 0; i < header->NumEntries; i++) {
                    protectedRangesFound = true;
                    const BG_VENDOR_HASH_FILE_ENTRY* entry = (const BG_VENDOR_HASH_FILE_ENTRY*)(header + 1) + i;

                    BG_PROTECTED_RANGE range;
                    range.Offset = entry->Offset;
                    range.Size = entry->Size;
                    range.Hash = UByteArray((const char*)entry->Hash, sizeof(entry->Hash));
                    range.Type = BG_PROTECTED_RANGE_VENDOR_HASH_PHOENIX;
                    bgProtectedRanges.push_back(range);
                }

                if (protectedRangesFound) {
                    securityInfo += usprintf("Phoenix hash file found at base %Xh\nProtected ranges:", model->base(index));
                    for (UINT32 i = 0; i < header->NumEntries; i++) {
                        const BG_VENDOR_HASH_FILE_ENTRY* entry = (const BG_VENDOR_HASH_FILE_ENTRY*)(header + 1) + i;
                        securityInfo += usprintf("\nRelativeOffset: %08Xh Size: %Xh\nHash: ", entry->Offset, entry->Size);
                        for (UINT8 j = 0; j < sizeof(entry->Hash); j++) {
                            securityInfo += usprintf("%02X", entry->Hash[j]);
                        }
                    }
                    securityInfo += UString("\n------------------------------------------------------------------------\n\n");
                }

                msg(usprintf("%s: Phoenix hash file found", __FUNCTION__), index);
            }
            else {
                msg(usprintf("%s: empty Phoenix hash file found", __FUNCTION__), index);
            }

            model->setText(index, UString("Phoenix hash file"));
        }
    }
    else if (fileGuid == BG_VENDOR_HASH_FILE_GUID_AMI) {
        UModelIndex fileIndex = model->parent(index);
        const UByteArray &body = model->body(index);
        UINT32 size = (UINT32)body.size();
        if (size != (UINT32)body.count('\xFF')) {
            if (size == sizeof(BG_VENDOR_HASH_FILE_HEADER_AMI_NEW)) {
                bool protectedRangesFound = false;
                UINT32 NumEntries = (UINT32)body.size() / sizeof(BG_VENDOR_HASH_FILE_ENTRY);
                for (UINT32 i = 0; i < NumEntries; i++) {
                    protectedRangesFound = true;
                    const BG_VENDOR_HASH_FILE_ENTRY* entry = (const BG_VENDOR_HASH_FILE_ENTRY*)(body.constData()) + i;
                    BG_PROTECTED_RANGE range;
                    range.Offset = entry->Offset;
                    range.Size = entry->Size;
                    range.Hash = UByteArray((const char*)entry->Hash, sizeof(entry->Hash));
                    range.Type = BG_PROTECTED_RANGE_VENDOR_HASH_AMI_NEW;
                    bgProtectedRanges.push_back(range);
                }

                if (protectedRangesFound) {
                    securityInfo += usprintf("New AMI hash file found at base %Xh\nProtected ranges:", model->base(fileIndex));
                    for (UINT32 i = 0; i < NumEntries; i++) {
                        const BG_VENDOR_HASH_FILE_ENTRY* entry = (const BG_VENDOR_HASH_FILE_ENTRY*)(body.constData()) + i;
                        securityInfo += usprintf("\nAddress: %08Xh Size: %Xh\nHash: ", entry->Offset, entry->Size);
                        for (UINT8 j = 0; j < sizeof(entry->Hash); j++) {
                            securityInfo += usprintf("%02X", entry->Hash[j]);
                        }
                    }
                    securityInfo += UString("\n------------------------------------------------------------------------\n\n");
                }

                msg(usprintf("%s: new AMI hash file found", __FUNCTION__), fileIndex);
            }
            else if (size == sizeof(BG_VENDOR_HASH_FILE_HEADER_AMI_OLD)) {
                securityInfo += usprintf("Old AMI hash file found at base %Xh\nProtected range:", model->base(fileIndex));
                const BG_VENDOR_HASH_FILE_HEADER_AMI_OLD* entry = (const BG_VENDOR_HASH_FILE_HEADER_AMI_OLD*)(body.constData());
                securityInfo += usprintf("\nSize: %Xh\nHash: ", entry->Size);
                for (UINT8 i = 0; i < sizeof(entry->Hash); i++) {
                    securityInfo += usprintf("%02X", entry->Hash[i]);
                }
                securityInfo += UString("\n------------------------------------------------------------------------\n\n");

                BG_PROTECTED_RANGE range;
                range.Offset = 0;
                range.Size = entry->Size;
                range.Hash = UByteArray((const char*)entry->Hash, sizeof(entry->Hash));
                range.Type = BG_PROTECTED_RANGE_VENDOR_HASH_AMI_OLD;
                bgProtectedRanges.push_back(range);

                msg(usprintf("%s: old AMI hash file found", __FUNCTION__), fileIndex);
            }
            else {
                msg(usprintf("%s: unknown or corrupted AMI hash file found", __FUNCTION__), index);
            }
        }
        else {
            msg(usprintf("%s: empty AMI hash file found", __FUNCTION__), fileIndex);
        }

        model->setText(fileIndex, UString("AMI hash file"));
    }

    return U_SUCCESS;
}

#ifndef U_ENABLE_FIT_PARSING_SUPPORT
USTATUS FfsParser::parseFit(const UModelIndex & index)
{
    U_UNUSED_PARAMETER(index);
    return U_SUCCESS;
}

#else
USTATUS FfsParser::parseFit(const UModelIndex & index)
{
    // Check sanity
    if (!index.isValid())
        return EFI_INVALID_PARAMETER;

    // Search for FIT
    UModelIndex fitIndex;
    UINT32 fitOffset;
    findFitRecursive(index, fitIndex, fitOffset);

    // FIT not found
    if (!fitIndex.isValid())
        return U_SUCCESS;

    // Explicitly set the item containing FIT as fixed
    model->setFixed(fitIndex, true);

    // Special case of FIT header
    UByteArray fitBody = model->body(fitIndex);
    const FIT_ENTRY* fitHeader = (const FIT_ENTRY*)(fitBody.constData() + fitOffset);

    // Check FIT checksum, if present
    UINT32 fitSize = fitHeader->Size * sizeof(FIT_ENTRY);
    if (fitHeader->CsFlag) {
        // Calculate FIT entry checksum
        UByteArray tempFIT = model->body(fitIndex).mid(fitOffset, fitSize);
        FIT_ENTRY* tempFitHeader = (FIT_ENTRY*)tempFIT.data();
        tempFitHeader->Checksum = 0;
        UINT8 calculated = calculateChecksum8((const UINT8*)tempFitHeader, fitSize);
        if (calculated != fitHeader->Checksum) {
            msg(usprintf("%s: invalid FIT table checksum %02Xh, should be %02Xh", __FUNCTION__, fitHeader->Checksum, calculated), fitIndex);
        }
    }

    // Check fit header type
    if (fitHeader->Type != FIT_TYPE_HEADER) {
        msg(UString("Invalid FIT header type"), fitIndex);
        return U_INVALID_FIT;
    }

    // Add FIT header
    std::vector<UString> currentStrings;
    currentStrings.push_back(UString("_FIT_            "));
    currentStrings.push_back(usprintf("%08Xh", fitSize));
    currentStrings.push_back(usprintf("%04Xh", fitHeader->Version));
    currentStrings.push_back(usprintf("%02Xh", fitHeader->Checksum));
    currentStrings.push_back(fitEntryTypeToUString(fitHeader->Type));
    currentStrings.push_back(UString()); // Empty info for FIT header
    fitTable.push_back(std::pair<std::vector<UString>, UModelIndex>(currentStrings, fitIndex));

    // Process all other entries
    UModelIndex acmIndex;
    UModelIndex kmIndex;
    UModelIndex bpIndex;
    for (UINT32 i = 1; i < fitHeader->Size; i++) {
        currentStrings.clear();
        UString info;
        UModelIndex itemIndex;
        const FIT_ENTRY* currentEntry = fitHeader + i;
        UINT32 currentEntrySize = currentEntry->Size;

        // Check sanity
        if (currentEntry->Type == FIT_TYPE_HEADER) {
            msg(usprintf("%s: second FIT header found, the table is damaged", __FUNCTION__), fitIndex);
            return U_INVALID_FIT;
        }

        // Special case of version 0 entries
        if (currentEntry->Version == 0) {
            const FIT_ENTRY_VERSION_0_CONFIG_POLICY* policy = (const FIT_ENTRY_VERSION_0_CONFIG_POLICY*)currentEntry;
            info += usprintf("Index: %04Xh BitPosition: %02Xh AccessWidth: %02Xh DataRegAddr: %04Xh IndexRegAddr: %04Xh",
                policy->Index,
                policy->BitPosition,
                policy->AccessWidth,
                policy->DataRegisterAddress,
                policy->IndexRegisterAddress);
        }
        else if (currentEntry->Address > addressDiff && currentEntry->Address < 0xFFFFFFFFUL) { // Only elements in the image need to be parsed
            UINT32 currentEntryBase = (UINT32)(currentEntry->Address - addressDiff);
            itemIndex = model->findByBase(currentEntryBase);
            if (itemIndex.isValid()) {
                USTATUS status = U_INVALID_FIT;
                UByteArray item = model->header(itemIndex) + model->body(itemIndex) + model->tail(itemIndex);
                UINT32 localOffset = currentEntryBase - model->base(itemIndex);

                switch (currentEntry->Type) {
                case FIT_TYPE_MICROCODE:
                    status = parseFitEntryMicrocode(item, localOffset, itemIndex, info, currentEntrySize);
                    break;

                case FIT_TYPE_BIOS_AC_MODULE:
                    status = parseFitEntryAcm(item, localOffset, itemIndex, info, currentEntrySize);
                    acmIndex = itemIndex;
                    break;

                case FIT_TYPE_AC_KEY_MANIFEST:
                    status = parseFitEntryBootGuardKeyManifest(item, localOffset, itemIndex, info, currentEntrySize);
                    kmIndex = itemIndex;
                    break;

                case FIT_TYPE_AC_BOOT_POLICY:
                    status = parseFitEntryBootGuardBootPolicy(item, localOffset, itemIndex, info, currentEntrySize);
                    bpIndex = itemIndex;
                    break;

                default:
                    // Do nothing
                    status = U_SUCCESS;
                    break;
                }

                if (status != U_SUCCESS)
                    itemIndex = UModelIndex();
            }
            else {
                msg(usprintf("%s: FIT entry #%d not found in the image", __FUNCTION__, i), fitIndex);
            }
        }

        if (itemIndex.isValid()) {
            // Explicitly set the item referenced by FIT as fixed
            // TODO: lift this restriction after FIT builder is ready
            model->setFixed(itemIndex, true);
        }

        // Add entry to fitTable
        currentStrings.push_back(usprintf("%016" PRIX64 "h", currentEntry->Address));
        currentStrings.push_back(usprintf("%08Xh", currentEntrySize, currentEntrySize));
        currentStrings.push_back(usprintf("%04Xh", currentEntry->Version));
        currentStrings.push_back(usprintf("%02Xh", currentEntry->Checksum));
        currentStrings.push_back(fitEntryTypeToUString(currentEntry->Type));
        currentStrings.push_back(info);
        fitTable.push_back(std::pair<std::vector<UString>, UModelIndex>(currentStrings, itemIndex));
    }

    // Perform validation of BootGuard stuff
    if (bgAcmFound) {
        if (!bgKeyManifestFound) {
            msg(usprintf("%s: ACM found, but KeyManifest isn't", __FUNCTION__), acmIndex);
        }
        else if (!bgBootPolicyFound) {
            msg(usprintf("%s: ACM and KeyManifest found, BootPolicy isn't", __FUNCTION__), kmIndex);
        }
        else {
            // Check key hashes
            if (!bgKmHash.isEmpty() && bgBpHash.isEmpty() && bgKmHash != bgBpHash) {
                msg(usprintf("%s: BootPolicy key hash stored in KeyManifest differs from the hash of public key stored in BootPolicy", __FUNCTION__), bpIndex);
                return U_SUCCESS;
            }
        }
    }

    return U_SUCCESS;
}

void FfsParser::findFitRecursive(const UModelIndex & index, UModelIndex & found, UINT32 & fitOffset)
{
    // Sanity check
    if (!index.isValid()) {
        return;
    }

    // Process child items
    for (int i = 0; i < model->rowCount(index); i++) {
        findFitRecursive(index.child(i, 0), found, fitOffset);
        if (found.isValid())
            return;
    }

    // Check for all FIT signatures in item's body
    UByteArray lastVtfBody = model->body(lastVtf);
    UINT32 storedFitAddress = *(const UINT32*)(lastVtfBody.constData() + lastVtfBody.size() - FIT_POINTER_OFFSET);
    for (INT32 offset = model->body(index).indexOf(FIT_SIGNATURE);
        offset >= 0;
        offset = model->body(index).indexOf(FIT_SIGNATURE, offset + 1)) {
        // FIT candidate found, calculate it's physical address
        UINT32 fitAddress = model->base(index) + (UINT32)addressDiff + model->header(index).size() + (UINT32)offset;

        // Check FIT address to be stored in the last VTF
        if (fitAddress == storedFitAddress) {
            found = index;
            fitOffset = offset;
            msg(usprintf("%s: real FIT table found at physical address %08Xh", __FUNCTION__, fitAddress), found);
            break;
        }
        else if (model->rowCount(index) == 0) // Show messages only to leaf items
            msg(usprintf("%s: FIT table candidate found, but not referenced from the last VTF", __FUNCTION__), index);
    }
}

USTATUS FfsParser::parseFitEntryMicrocode(const UByteArray & microcode, const UINT32 localOffset, const UModelIndex & parent, UString & info, UINT32 &realSize)
{
    U_UNUSED_PARAMETER(parent);
    if ((UINT32)microcode.size() - localOffset < sizeof(INTEL_MICROCODE_HEADER)) {
        return U_INVALID_MICROCODE;
    }

    const INTEL_MICROCODE_HEADER* ucodeHeader = (const INTEL_MICROCODE_HEADER*)(microcode.constData() + localOffset);
    if (!microcodeHeaderValid(ucodeHeader)) {
        return U_INVALID_MICROCODE;
    }

    if ((UINT32)microcode.size() - localOffset < ucodeHeader->TotalSize) {
        return U_INVALID_MICROCODE;
    }

    // Valid microcode found
    info = usprintf("CpuSignature: %08Xh, Revision: %08Xh, Date: %02X.%02X.%04X",
                    ucodeHeader->ProcessorSignature,
                    ucodeHeader->UpdateRevision,
                    ucodeHeader->DateDay,
                    ucodeHeader->DateMonth,
                    ucodeHeader->DateYear);
    realSize = ucodeHeader->TotalSize;
    return U_SUCCESS;
}

USTATUS FfsParser::parseFitEntryAcm(const UByteArray & acm, const UINT32 localOffset, const UModelIndex & parent, UString & info, UINT32 &realSize)
{
    if ((UINT32)acm.size() < localOffset + sizeof(INTEL_ACM_HEADER)) {
        return U_INVALID_ACM;
    }

    const INTEL_ACM_HEADER* header = (const INTEL_ACM_HEADER*)(acm.constData() + localOffset);
    if (header->ModuleType != INTEL_ACM_MODULE_TYPE || header->ModuleVendor != INTEL_ACM_MODULE_VENDOR) {
        return U_INVALID_ACM;
    }

    UINT32 acmSize = header->ModuleSize * sizeof(UINT32);
    if ((UINT32)acm.size() < localOffset + acmSize) {
        return U_INVALID_ACM;
    }

    // Valid ACM found
    info = usprintf("LocalOffset: %08Xh, EntryPoint: %08Xh, ACM SVN: %04Xh, Date: %02X.%02X.%04X",
                    localOffset,
                    header->EntryPoint,
                    header->AcmSvn,
                    header->DateDay,
                    header->DateMonth,
                    header->DateYear
                    );
    realSize = acmSize;

    // Add ACM header info
    UString acmInfo;
    acmInfo += usprintf(" found at base %Xh\n"
                        "ModuleType: %04Xh         ModuleSubtype: %04Xh     HeaderLength: %08Xh\n"
                        "HeaderVersion: %08Xh  ChipsetId:  %04Xh        Flags: %04Xh\n"
                        "ModuleVendor: %04Xh       Date: %02X.%02X.%04X         ModuleSize: %08Xh\n"
                        "EntryPoint: %08Xh     AcmSvn: %04Xh            Unknown1: %08Xh\n"
                        "Unknown2: %08Xh       GdtBase: %08Xh       GdtMax: %08Xh\n"
                        "SegSel: %08Xh         KeySize: %08Xh       Unknown3: %08Xh",
                        model->base(parent) + localOffset,
                        header->ModuleType,
                        header->ModuleSubtype,
                        header->ModuleSize * sizeof(UINT32),
                        header->HeaderVersion,
                        header->ChipsetId,
                        header->Flags,
                        header->ModuleVendor,
                        header->DateDay, header->DateMonth, header->DateYear,
                        header->ModuleSize * sizeof(UINT32),
                        header->EntryPoint,
                        header->AcmSvn,
                        header->Unknown1,
                        header->Unknown2,
                        header->GdtBase,
                        header->GdtMax,
                        header->SegmentSel,
                        header->KeySize * sizeof(UINT32),
                        header->Unknown4 * sizeof(UINT32)
                        );
    // Add PubKey
    acmInfo += usprintf("\n\nACM RSA Public Key (Exponent: %Xh):", header->RsaPubExp);
    for (UINT16 i = 0; i < sizeof(header->RsaPubKey); i++) {
        if (i % 32 == 0)
            acmInfo += UString("\n");
        acmInfo += usprintf("%02X", header->RsaPubKey[i]);
    }
    // Add RsaSig
    acmInfo += UString("\n\nACM RSA Signature:");
    for (UINT16 i = 0; i < sizeof(header->RsaSig); i++) {
        if (i % 32 == 0)
            acmInfo += UString("\n");
        acmInfo += usprintf("%02X", header->RsaSig[i]);
    }
    acmInfo += UString("\n------------------------------------------------------------------------\n\n");

    if(header->ModuleSubtype == INTEL_ACM_MODULE_SUBTYPE_TXT_ACM)
        securityInfo += "TXT ACM" + acmInfo;
    else if(header->ModuleSubtype == INTEL_ACM_MODULE_SUBTYPE_S_ACM)
        securityInfo += "S-ACM" + acmInfo;
    else if (header->ModuleSubtype == INTEL_ACM_MODULE_SUBTYPE_BOOTGUARD)
        securityInfo += "BootGuard ACM" + acmInfo;
    else
        securityInfo += "Intel ACM" + acmInfo;

    bgAcmFound = true;
    return U_SUCCESS;
}

USTATUS FfsParser::parseFitEntryBootGuardKeyManifest(const UByteArray & keyManifest, const UINT32 localOffset, const UModelIndex & parent, UString & info, UINT32 &realSize)
{
    U_UNUSED_PARAMETER(realSize);
    if ((UINT32)keyManifest.size() < localOffset + sizeof(BG_KEY_MANIFEST)) {
        return U_INVALID_BG_KEY_MANIFEST;
    }

    const BG_KEY_MANIFEST* header = (const BG_KEY_MANIFEST*)(keyManifest.constData() + localOffset);
    if (header->Tag != BG_KEY_MANIFEST_TAG) {
        return U_INVALID_BG_KEY_MANIFEST;
    }

    // Valid KM found
    info = usprintf("LocalOffset: %08Xh, KM Version: %02Xh, KM SVN: %02Xh, KM ID: %02Xh",
                    localOffset,
                    header->KmVersion,
                    header->KmSvn,
                    header->KmId
                    );

    // Add KM header info
    securityInfo += usprintf(
                              "Intel BootGuard Key manifest found at base %Xh\n"
                              "Tag: __KEYM__ Version: %02Xh KmVersion: %02Xh KmSvn: %02Xh KmId: %02Xh",
                              model->base(parent) + localOffset,
                              header->Version,
                              header->KmVersion,
                              header->KmSvn,
                              header->KmId
                              );

    // Add hash of Key Manifest PubKey, this hash will be written to FPFs
    UINT8 hash[SHA256_DIGEST_SIZE];
    sha256(&header->KeyManifestSignature.PubKey.Modulus, sizeof(header->KeyManifestSignature.PubKey.Modulus), hash);
    securityInfo += UString("\n\nKey Manifest RSA Public Key Hash:\n");
    for (UINT8 i = 0; i < sizeof(hash); i++) {
        securityInfo += usprintf("%02X", hash[i]);
    }

    // Add BpKeyHash
    securityInfo += UString("\n\nBoot Policy RSA Public Key Hash:\n");
    for (UINT8 i = 0; i < sizeof(header->BpKeyHash.HashBuffer); i++) {
        securityInfo += usprintf("%02X", header->BpKeyHash.HashBuffer[i]);
    }
    bgKmHash = UByteArray((const char*)header->BpKeyHash.HashBuffer, sizeof(header->BpKeyHash.HashBuffer));

    // Add Key Manifest PubKey
    securityInfo += usprintf("\n\nKey Manifest RSA Public Key (Exponent: %Xh):",
                              header->KeyManifestSignature.PubKey.Exponent);
    for (UINT16 i = 0; i < sizeof(header->KeyManifestSignature.PubKey.Modulus); i++) {
        if (i % 32 == 0)
            securityInfo += UString("\n");
        securityInfo += usprintf("%02X", header->KeyManifestSignature.PubKey.Modulus[i]);
    }
    // Add Key Manifest Signature
    securityInfo += UString("\n\nKey Manifest RSA Signature:");
    for (UINT16 i = 0; i < sizeof(header->KeyManifestSignature.Signature.Signature); i++) {
        if (i % 32 == 0)
            securityInfo += UString("\n");
        securityInfo += usprintf("%02X", header->KeyManifestSignature.Signature.Signature[i]);
    }
    securityInfo += UString("\n------------------------------------------------------------------------\n\n");
    bgKeyManifestFound = true;
    return U_SUCCESS;
}

USTATUS FfsParser::findNextBootGuardBootPolicyElement(const UByteArray & bootPolicy, const UINT32 elementOffset, UINT32 & nextElementOffset, UINT32 & nextElementSize)
{
    UINT32 dataSize = bootPolicy.size();
    if (dataSize < sizeof(UINT64)) {
        return U_ELEMENTS_NOT_FOUND;
    }

    UINT32 offset = elementOffset;
    for (; offset < dataSize - sizeof(UINT64); offset++) {
        const UINT64* currentPos = (const UINT64*)(bootPolicy.constData() + offset);
        if (*currentPos == BG_BOOT_POLICY_MANIFEST_IBB_ELEMENT_TAG && offset + sizeof(BG_IBB_ELEMENT) < dataSize) {
            const BG_IBB_ELEMENT* header = (const BG_IBB_ELEMENT*)currentPos;
            // Check that all segments are present
            if (offset + sizeof(BG_IBB_ELEMENT) + sizeof(BG_IBB_SEGMENT_ELEMENT) * header->IbbSegCount < dataSize) {
                nextElementOffset = offset;
                nextElementSize = sizeof(BG_IBB_ELEMENT) + sizeof(BG_IBB_SEGMENT_ELEMENT) * header->IbbSegCount;
                return U_SUCCESS;
            }
        }
        else if (*currentPos == BG_BOOT_POLICY_MANIFEST_PLATFORM_MANUFACTURER_ELEMENT_TAG && offset + sizeof(BG_PLATFORM_MANUFACTURER_ELEMENT) < dataSize) {
            const BG_PLATFORM_MANUFACTURER_ELEMENT* header = (const BG_PLATFORM_MANUFACTURER_ELEMENT*)currentPos;
            // Check that data is present
            if (offset + sizeof(BG_PLATFORM_MANUFACTURER_ELEMENT) + header->DataSize < dataSize) {
                nextElementOffset = offset;
                nextElementSize = sizeof(BG_PLATFORM_MANUFACTURER_ELEMENT) + header->DataSize;
                return U_SUCCESS;
            }
        }
        else if (*currentPos == BG_BOOT_POLICY_MANIFEST_SIGNATURE_ELEMENT_TAG && offset + sizeof(BG_BOOT_POLICY_MANIFEST_SIGNATURE_ELEMENT) < dataSize) {
            nextElementOffset = offset;
            nextElementSize = sizeof(BG_BOOT_POLICY_MANIFEST_SIGNATURE_ELEMENT);
            return U_SUCCESS;
        }
    }

    return U_ELEMENTS_NOT_FOUND;
}

USTATUS FfsParser::parseFitEntryBootGuardBootPolicy(const UByteArray & bootPolicy, const UINT32 localOffset, const UModelIndex & parent, UString & info, UINT32 &realSize)
{
    U_UNUSED_PARAMETER(realSize);
    if ((UINT32)bootPolicy.size() < localOffset + sizeof(BG_BOOT_POLICY_MANIFEST_HEADER)) {
        return U_INVALID_BG_BOOT_POLICY;
    }

    const BG_BOOT_POLICY_MANIFEST_HEADER* header = (const BG_BOOT_POLICY_MANIFEST_HEADER*)(bootPolicy.constData() + localOffset);
    if (header->Tag != BG_BOOT_POLICY_MANIFEST_HEADER_TAG) {
        return U_INVALID_BG_BOOT_POLICY;
    }

    UINT32 bmSize = sizeof(BG_BOOT_POLICY_MANIFEST_HEADER);
    if ((UINT32)bootPolicy.size() < localOffset + bmSize) {
        return U_INVALID_BG_BOOT_POLICY;
    }

    // Valid BPM found
    info = usprintf("LocalOffset: %08Xh, BP SVN: %02Xh, ACM SVN: %02Xh",
                    localOffset,
                    header->BPSVN,
                    header->ACMSVN
                    );

    // Add BP header info
    securityInfo += usprintf(
                              "Intel BootGuard Boot Policy Manifest found at base %Xh\n"
                              "Tag: __ACBP__ Version: %02Xh HeaderVersion: %02Xh\n"
                              "PMBPMVersion: %02Xh PBSVN: %02Xh ACMSVN: %02Xh NEMDataStack: %04Xh\n",
                              model->base(parent) + localOffset,
                              header->Version,
                              header->HeaderVersion,
                              header->PMBPMVersion,
                              header->BPSVN,
                              header->ACMSVN,
                              header->NEMDataSize
                              );

    // Iterate over elements to get them all
    UINT32 elementOffset = 0;
    UINT32 elementSize = 0;
    USTATUS status = findNextBootGuardBootPolicyElement(bootPolicy, localOffset + sizeof(BG_BOOT_POLICY_MANIFEST_HEADER), elementOffset, elementSize);
    while (status == U_SUCCESS) {
        const UINT64* currentPos = (const UINT64*)(bootPolicy.constData() + elementOffset);
        if (*currentPos == BG_BOOT_POLICY_MANIFEST_IBB_ELEMENT_TAG) {
            const BG_IBB_ELEMENT* elementHeader = (const BG_IBB_ELEMENT*)currentPos;
            // Valid IBB element found
            securityInfo += usprintf(
                                      "\nInitial Boot Block Element found at base %Xh\n"
                                      "Tag: __IBBS__       Version: %02Xh         Unknown: %02Xh\n"
                                      "Flags: %08Xh    IbbMchBar: %08Xh VtdBar: %08Xh\n"
                                      "PmrlBase: %08Xh PmrlLimit: %08Xh  EntryPoint: %08Xh",
                                      model->base(parent) + localOffset + elementOffset,
                                      elementHeader->Version,
                                      elementHeader->Unknown,
                                      elementHeader->Flags,
                                      elementHeader->IbbMchBar,
                                      elementHeader->VtdBar,
                                      elementHeader->PmrlBase,
                                      elementHeader->PmrlLimit,
                                      elementHeader->EntryPoint
                                      );

            // Add PostIbbHash
            securityInfo += UString("\n\nPost IBB Hash:\n");
            for (UINT8 i = 0; i < sizeof(elementHeader->IbbHash.HashBuffer); i++) {
                securityInfo += usprintf("%02X", elementHeader->IbbHash.HashBuffer[i]);
            }

            // Check for non-empry PostIbbHash
            UByteArray postIbbHash((const char*)elementHeader->IbbHash.HashBuffer, sizeof(elementHeader->IbbHash.HashBuffer));
            if (postIbbHash.count('\x00') != postIbbHash.size() && postIbbHash.count('\xFF') != postIbbHash.size()) {
                BG_PROTECTED_RANGE range;
                range.Type = BG_PROTECTED_RANGE_INTEL_BOOT_GUARD_POST_IBB;
                range.Hash = postIbbHash;
                bgProtectedRanges.push_back(range);
            }

            // Add Digest
            bgBpDigest = UByteArray((const char*)elementHeader->Digest.HashBuffer, sizeof(elementHeader->Digest.HashBuffer));
            securityInfo += UString("\n\nIBB Digest:\n");
            for (UINT8 i = 0; i < (UINT8)bgBpDigest.size(); i++) {
                securityInfo += usprintf("%02X", (UINT8)bgBpDigest.at(i));
            }

            // Add all IBB segments
            securityInfo += UString("\n\nIBB Segments:\n");
            const BG_IBB_SEGMENT_ELEMENT* segments = (const BG_IBB_SEGMENT_ELEMENT*)(elementHeader + 1);
            for (UINT8 i = 0; i < elementHeader->IbbSegCount; i++) {
                securityInfo += usprintf("Flags: %04Xh Address: %08Xh Size: %08Xh\n",
                                          segments[i].Flags, segments[i].Base, segments[i].Size);
                if (segments[i].Flags == BG_IBB_SEGMENT_FLAG_IBB) {
                    BG_PROTECTED_RANGE range;
                    range.Offset = segments[i].Base;
                    range.Size = segments[i].Size;
                    range.Type = BG_PROTECTED_RANGE_INTEL_BOOT_GUARD_IBB;
                    bgProtectedRanges.push_back(range);
                }
            }
        }
        else if (*currentPos == BG_BOOT_POLICY_MANIFEST_PLATFORM_MANUFACTURER_ELEMENT_TAG) {
            const BG_PLATFORM_MANUFACTURER_ELEMENT* elementHeader = (const BG_PLATFORM_MANUFACTURER_ELEMENT*)currentPos;
            securityInfo += usprintf(
                                      "\nPlatform Manufacturer Data Element found at base %Xh\n"
                                      "Tag: __PMDA__ Version: %02Xh DataSize: %02Xh",
                                      model->base(parent) + localOffset + elementOffset,
                                      elementHeader->Version,
                                      elementHeader->DataSize
                                      );
            // Check for Microsoft PMDA hash data
            const BG_MICROSOFT_PMDA_HEADER* pmdaHeader = (const BG_MICROSOFT_PMDA_HEADER*)(elementHeader + 1);
            if (pmdaHeader->Version == BG_MICROSOFT_PMDA_VERSION
                && elementHeader->DataSize == sizeof(BG_MICROSOFT_PMDA_HEADER) + sizeof(BG_MICROSOFT_PMDA_ENTRY)*pmdaHeader->NumEntries) {
                // Add entries
                securityInfo += UString("\nMicrosoft PMDA-based protected ranges:\n");
                const BG_MICROSOFT_PMDA_ENTRY* entries = (const BG_MICROSOFT_PMDA_ENTRY*)(pmdaHeader + 1);
                for (UINT32 i = 0; i < pmdaHeader->NumEntries; i++) {

                    securityInfo += usprintf("Address: %08Xh Size: %08Xh\n", entries[i].Address, entries[i].Size);
                    securityInfo += UString("Hash: ");
                    for (UINT8 j = 0; j < sizeof(entries[i].Hash); j++) {
                        securityInfo += usprintf("%02X", entries[i].Hash[j]);
                    }
                    securityInfo += UString("\n");

                    BG_PROTECTED_RANGE range;
                    range.Offset = entries[i].Address;
                    range.Size = entries[i].Size;
                    range.Hash = UByteArray((const char*)entries[i].Hash, sizeof(entries[i].Hash));
                    range.Type = BG_PROTECTED_RANGE_VENDOR_HASH_MICROSOFT;
                    bgProtectedRanges.push_back(range);
                }
            }
            else {
                // Add raw data
                const UINT8* data = (const UINT8*)(elementHeader + 1);
                for (UINT16 i = 0; i < elementHeader->DataSize; i++) {
                    if (i % 32 == 0)
                        securityInfo += UString("\n");
                    securityInfo += usprintf("%02X", data[i]);
                }
                securityInfo += UString("\n");
            }
        }
        else if (*currentPos == BG_BOOT_POLICY_MANIFEST_SIGNATURE_ELEMENT_TAG) {
            const BG_BOOT_POLICY_MANIFEST_SIGNATURE_ELEMENT* elementHeader = (const BG_BOOT_POLICY_MANIFEST_SIGNATURE_ELEMENT*)currentPos;
            securityInfo += usprintf(
                                      "\nBoot Policy Signature Element found at base %Xh\n"
                                      "Tag: __PMSG__ Version: %02Xh",
                                      model->base(parent) + localOffset + elementOffset,
                                      elementHeader->Version
                                      );

            // Add PubKey
            securityInfo += usprintf("\n\nBoot Policy RSA Public Key (Exponent: %Xh):", elementHeader->KeySignature.PubKey.Exponent);
            for (UINT16 i = 0; i < sizeof(elementHeader->KeySignature.PubKey.Modulus); i++) {
                if (i % 32 == 0)
                    securityInfo += UString("\n");
                securityInfo += usprintf("%02X", elementHeader->KeySignature.PubKey.Modulus[i]);
            }

            // Calculate and add PubKey hash
            UINT8 hash[SHA256_DIGEST_SIZE];
            sha256(&elementHeader->KeySignature.PubKey.Modulus, sizeof(elementHeader->KeySignature.PubKey.Modulus), hash);
            securityInfo += UString("\n\nBoot Policy RSA Public Key Hash:");
            for (UINT8 i = 0; i < sizeof(hash); i++) {
                if (i % 32 == 0)
                    securityInfo += UString("\n");
                securityInfo += usprintf("%02X", hash[i]);
            }
            bgBpHash = UByteArray((const char*)hash, sizeof(hash));

            // Add Signature
            securityInfo += UString("\n\nBoot Policy RSA Signature:");
            for (UINT16 i = 0; i < sizeof(elementHeader->KeySignature.Signature.Signature); i++) {
                if (i % 32 == 0)
                    securityInfo += UString("\n");
                securityInfo += usprintf("%02X", elementHeader->KeySignature.Signature.Signature[i]);
            }
        }
        status = findNextBootGuardBootPolicyElement(bootPolicy, elementOffset + elementSize, elementOffset, elementSize);
    }

    securityInfo += UString("\n------------------------------------------------------------------------\n\n");
    bgBootPolicyFound = true;
    return U_SUCCESS;
}
#endif

USTATUS FfsParser::parseMicrocodeVolumeBody(const UModelIndex & index)
{
    const UINT32 headerSize = (UINT32)model->header(index).size();
    const UINT32 bodySize = (UINT32)model->body(index).size();
    UINT32 offset = 0;
    USTATUS result = U_SUCCESS;

    while(true) {
        // Parse current microcode
        UModelIndex currentMicrocode;
        UByteArray ucode = model->body(index).mid(offset);

        // Check for empty area
        if (ucode.size() == ucode.count('\xFF') || ucode.size() == ucode.count('\x00')) {
            result = U_INVALID_MICROCODE;
        }
        else {
            result = parseIntelMicrocodeHeader(ucode, headerSize + offset, index, currentMicrocode);
        }

        // Add the rest as padding
        if (result) {
            if (offset < bodySize) {
                // Get info
                UString name = UString("Padding");
                UString info = usprintf("Full size: %Xh (%u)", ucode.size(), ucode.size());

                // Add tree item
                model->addItem(headerSize + offset, Types::Padding, getPaddingType(ucode), name, UString(), info, UByteArray(), ucode, UByteArray(), Fixed, index);
            }
            return U_SUCCESS;
        }

        // Get to next candidate
        offset += model->header(currentMicrocode).size() + model->body(currentMicrocode).size() + model->tail(currentMicrocode).size();
        if (offset >= bodySize)
            break;
    }
    return U_SUCCESS;
}

USTATUS FfsParser::parseIntelMicrocodeHeader(const UByteArray & microcode, const UINT32 localOffset, const UModelIndex & parent, UModelIndex & index)
{
    // We have enough data to fit the header
    if ((UINT32)microcode.size() <  sizeof(INTEL_MICROCODE_HEADER)) {
        return U_INVALID_MICROCODE;
    }

    const INTEL_MICROCODE_HEADER* ucodeHeader = (const INTEL_MICROCODE_HEADER*)microcode.constData();

    if (!microcodeHeaderValid(ucodeHeader)) {
        return U_INVALID_MICROCODE;
    }

    // We have enough data to fit the whole TotalSize
    if ((UINT32)microcode.size() < ucodeHeader->TotalSize) {
        return U_INVALID_MICROCODE;
    }

    // Valid microcode found
    UINT32 dataSize = ucodeHeader->DataSize;
    if (dataSize == 0) {
        dataSize = INTEL_MICROCODE_REAL_DATA_SIZE_ON_ZERO;
    }

    // Cross check DataSize and TotalSize
    if (ucodeHeader->TotalSize < sizeof(INTEL_MICROCODE_HEADER) + dataSize) {
        return U_INVALID_MICROCODE;
    }

    // Recalculate the whole microcode checksum
    UByteArray tempMicrocode = microcode;
    INTEL_MICROCODE_HEADER* tempUcodeHeader = (INTEL_MICROCODE_HEADER*)(tempMicrocode.data());
    tempUcodeHeader->Checksum = 0;
    UINT32 calculated = calculateChecksum32((const UINT32*)tempMicrocode.constData(), tempUcodeHeader->TotalSize);
    bool msgInvalidChecksum = (ucodeHeader->Checksum != calculated);

    // Construct header, body and tail
    UByteArray header = microcode.left(sizeof(INTEL_MICROCODE_HEADER));
    UByteArray body = microcode.mid(sizeof(INTEL_MICROCODE_HEADER), dataSize);
    UByteArray tail;

    // Check if the tail is present
    if (ucodeHeader->TotalSize > sizeof(INTEL_MICROCODE_HEADER) + dataSize) {
        tail = microcode.mid(sizeof(INTEL_MICROCODE_HEADER) + dataSize, ucodeHeader->TotalSize - (sizeof(INTEL_MICROCODE_HEADER) + dataSize));
    }

    // Check if we have extended header in the tail
    UString extendedHeaderInfo;
    bool msgUnknownOrDamagedMicrocodeTail = false;
    if ((UINT32)tail.size() >= sizeof(INTEL_MICROCODE_EXTENDED_HEADER)) {
        const INTEL_MICROCODE_EXTENDED_HEADER* extendedHeader = (const INTEL_MICROCODE_EXTENDED_HEADER*)tail.constData();

        // Reserved bytes are all zeroes
        bool extendedReservedBytesValid = true;
        for (UINT8 i = 0; i < sizeof(extendedHeader->Reserved); i++) {
            if (extendedHeader->Reserved[i] != 0x00) {
                extendedReservedBytesValid = false;
                break;
            }
        }

        // We have more than 0 entries and they are all in the tail
        if (extendedReservedBytesValid
            && extendedHeader->EntryCount > 0
            && (UINT32)tail.size() == sizeof(INTEL_MICROCODE_EXTENDED_HEADER) + extendedHeader->EntryCount * sizeof(INTEL_MICROCODE_EXTENDED_HEADER_ENTRY)) {
            // Recalculate extended header checksum
            INTEL_MICROCODE_EXTENDED_HEADER* tempExtendedHeader = (INTEL_MICROCODE_EXTENDED_HEADER*)(tempMicrocode.data() + sizeof(INTEL_MICROCODE_HEADER) + dataSize);
            tempExtendedHeader->Checksum = 0;
            UINT32 extendedCalculated = calculateChecksum32((const UINT32*)tempExtendedHeader, sizeof(INTEL_MICROCODE_EXTENDED_HEADER) + extendedHeader->EntryCount * sizeof(INTEL_MICROCODE_EXTENDED_HEADER_ENTRY));

            extendedHeaderInfo = usprintf("\nExtended header entries: %u\nExtended header checksum: %08Xh, ",
                                          extendedHeader->EntryCount,
                                          extendedHeader->Checksum)
                               + (extendedHeader->Checksum == extendedCalculated ? UString("valid") : usprintf("invalid, should be %08Xh", extendedCalculated));

            const INTEL_MICROCODE_EXTENDED_HEADER_ENTRY* firstEntry = (const INTEL_MICROCODE_EXTENDED_HEADER_ENTRY*)(extendedHeader + 1);
            for (UINT8 i = 0; i < extendedHeader->EntryCount; i++) {
                const INTEL_MICROCODE_EXTENDED_HEADER_ENTRY* entry = (const INTEL_MICROCODE_EXTENDED_HEADER_ENTRY*)(firstEntry + i);

                // Recalculate checksum after patching
                tempUcodeHeader->Checksum = 0;
                tempUcodeHeader->ProcessorFlags = entry->ProcessorFlags;
                tempUcodeHeader->ProcessorSignature = entry->ProcessorSignature;
                UINT32 entryCalculated = calculateChecksum32((const UINT32*)tempMicrocode.constData(), sizeof(INTEL_MICROCODE_HEADER) + dataSize);

                extendedHeaderInfo += usprintf("\nCPU signature #%u: %08Xh\nCPU flags #%u: %02Xh\nChecksum #%u: %08Xh, ",
                                               i + 1, entry->ProcessorSignature,
                                               i + 1, entry->ProcessorFlags,
                                               i + 1, entry->Checksum)
                                    + (entry->Checksum == entryCalculated ? UString("valid") : usprintf("invalid, should be %08Xh", entryCalculated));
            }
        }
        else {
            msgUnknownOrDamagedMicrocodeTail = true;
        }
    }
    else if (tail.size() != 0) {
        msgUnknownOrDamagedMicrocodeTail = true;
    }

    // Get microcode binary
    UByteArray microcodeBinary = microcode.left(ucodeHeader->TotalSize);

    // Add info
    UString name("Intel microcode");
    UString info = usprintf("Full size: %Xh (%u)\nHeader size: 0h (0u)\nBody size: %Xh (%u)\nTail size: 0h (0u)\n"
                            "Date: %02X.%02X.%04x\nCPU signature: %08Xh\nRevision: %08Xh\nCPU flags: %02Xh\nChecksum: %08Xh, ",
                            microcodeBinary.size(), microcodeBinary.size(),
                            microcodeBinary.size(), microcodeBinary.size(),
                            ucodeHeader->DateDay,
                            ucodeHeader->DateMonth,
                            ucodeHeader->DateYear,
                            ucodeHeader->ProcessorSignature,
                            ucodeHeader->UpdateRevision,
                            ucodeHeader->ProcessorFlags,
                            ucodeHeader->Checksum)
                 + (ucodeHeader->Checksum == calculated ? UString("valid") : usprintf("invalid, should be %08Xh", calculated))
                 + extendedHeaderInfo;

    // Add tree item
    index = model->addItem(localOffset, Types::Microcode, Subtypes::IntelMicrocode, name, UString(), info, UByteArray(), microcodeBinary, UByteArray(), Fixed, parent);
    if (msgInvalidChecksum)
        msg(usprintf("%s: invalid microcode checksum %08Xh, should be %08Xh", __FUNCTION__, ucodeHeader->Checksum, calculated), index);
    if (msgUnknownOrDamagedMicrocodeTail)
        msg(usprintf("%s: extended header of size %Xh (%u) found, but it's damaged or has unknown format", __FUNCTION__, tail.size(), tail.size()), index);

    // No need to parse the body further for now
    return U_SUCCESS;
}

USTATUS FfsParser::parseBpdtRegion(const UByteArray & region, const UINT32 localOffset, const UINT32 sbpdtOffsetFixup, const UModelIndex & parent, UModelIndex & index)
{
    UINT32 regionSize = (UINT32)region.size();

    // Check region size
    if (regionSize < sizeof(BPDT_HEADER)) {
        msg(usprintf("%s: BPDT region too small to fit BPDT partition table header", __FUNCTION__), parent);
        return U_INVALID_ME_PARTITION_TABLE;
    }

    // Populate partition table header
    const BPDT_HEADER* ptHeader = (const BPDT_HEADER*)(region.constData());

    // Check region size again
    UINT32 ptBodySize = ptHeader->NumEntries * sizeof(BPDT_ENTRY);
    UINT32 ptSize = sizeof(BPDT_HEADER) + ptBodySize;
    if (regionSize < ptSize) {
        msg(usprintf("%s: BPDT region too small to fit BPDT partition table", __FUNCTION__), parent);
        return U_INVALID_ME_PARTITION_TABLE;
    }

    // Get info
    UByteArray header = region.left(sizeof(BPDT_HEADER));
    UByteArray body = region.mid(sizeof(BPDT_HEADER), ptBodySize);

    UString name = UString("BPDT partition table");
    UString info = usprintf("Full size: %Xh (%u)\nHeader size: %Xh (%u)\nBody size: %Xh (%u)\nNumber of entries: %u\nVersion: %2Xh\n"
                            "IFWI version: %Xh\nFITC version: %u.%u.%u.%u",
                            ptSize, ptSize,
                            header.size(), header.size(),
                            ptBodySize, ptBodySize,
                            ptHeader->NumEntries,
                            ptHeader->HeaderVersion,
                            ptHeader->IfwiVersion,
                            ptHeader->FitcMajor, ptHeader->FitcMinor, ptHeader->FitcHotfix, ptHeader->FitcBuild);

    // Add tree item
    index = model->addItem(localOffset, Types::BpdtStore, 0, name, UString(), info, header, body, UByteArray(), Fixed, parent);

    // Adjust offset
    UINT32 offset = sizeof(BPDT_HEADER);

    // Add partition table entries
    std::vector<BPDT_PARTITION_INFO> partitions;
    const BPDT_ENTRY* firstPtEntry = (const BPDT_ENTRY*)((const UINT8*)ptHeader + sizeof(BPDT_HEADER));
    for (UINT16 i = 0; i < ptHeader->NumEntries; i++) {
        // Populate entry header
        const BPDT_ENTRY* ptEntry = firstPtEntry + i;

        // Get info
        name = bpdtEntryTypeToUString(ptEntry->Type);
        info = usprintf("Full size: %Xh (%u)\nType: %Xh\nPartition offset: %Xh\nPartition length: %Xh",
                        sizeof(BPDT_ENTRY), sizeof(BPDT_ENTRY),
                        ptEntry->Type,
                        ptEntry->Offset,
                        ptEntry->Size) +
        UString("\nSplit sub-partition first part: ") + (ptEntry->SplitSubPartitionFirstPart ? "Yes" : "No") +
        UString("\nSplit sub-partition second part: ") + (ptEntry->SplitSubPartitionSecondPart ? "Yes" : "No") +
        UString("\nCode sub-partition: ") + (ptEntry->CodeSubPartition ? "Yes" : "No") +
        UString("\nUMA cachable: ") + (ptEntry->UmaCachable ? "Yes" : "No");

        // Add tree item
        UModelIndex entryIndex = model->addItem(localOffset + offset, Types::BpdtEntry, 0, name, UString(), info, UByteArray(), UByteArray((const char*)ptEntry, sizeof(BPDT_ENTRY)), UByteArray(), Fixed, index);

        // Adjust offset
        offset += sizeof(BPDT_ENTRY);

        if (ptEntry->Offset != 0 && ptEntry->Offset != 0xFFFFFFFF && ptEntry->Size != 0) {
            // Add to partitions vector
            BPDT_PARTITION_INFO partition;
            partition.type = Types::BpdtPartition;
            partition.ptEntry = *ptEntry;
            partition.ptEntry.Offset -= sbpdtOffsetFixup;
            partition.index = entryIndex;
            partitions.push_back(partition);
        }
    }

    // Add padding if there's no partions to add
    if (partitions.size() == 0) {
        UByteArray partition = region.mid(ptSize);

        // Get info
        name = UString("Padding");
        info = usprintf("Full size: %Xh (%u)",
                        partition.size(), partition.size());

        // Add tree item
        model->addItem(localOffset + ptSize, Types::Padding, getPaddingType(partition), name, UString(), info, UByteArray(), partition, UByteArray(), Fixed, parent);
        return U_SUCCESS;
    }

make_partition_table_consistent:
    // Sort partitions by offset
    std::sort(partitions.begin(), partitions.end());

    // Check for intersections and paddings between partitions
    BPDT_PARTITION_INFO padding;

    // Check intersection with the partition table header
    if (partitions.front().ptEntry.Offset < ptSize) {
        msg(usprintf("%s: BPDT partition has intersection with BPDT partition table, skipped", __FUNCTION__),
            partitions.front().index);
        partitions.erase(partitions.begin());
        goto make_partition_table_consistent;
    }
    // Check for padding between partition table and the first partition
    else if (partitions.front().ptEntry.Offset > ptSize) {
        padding.ptEntry.Offset = ptSize;
        padding.ptEntry.Size = partitions.front().ptEntry.Offset - padding.ptEntry.Offset;
        padding.type = Types::Padding;
        partitions.insert(partitions.begin(), padding);
    }
    // Check for intersections/paddings between partitions
    for (size_t i = 1; i < partitions.size(); i++) {
        UINT32 previousPartitionEnd = partitions[i - 1].ptEntry.Offset + partitions[i - 1].ptEntry.Size;

        // Check that partition is fully present in the image
        if ((UINT64)partitions[i].ptEntry.Offset + (UINT64)partitions[i].ptEntry.Size > regionSize) {
            if ((UINT64)partitions[i].ptEntry.Offset >= (UINT64)region.size()) {
                msg(usprintf("%s: BPDT partition is located outside of the opened image, skipped", __FUNCTION__), partitions[i].index);
                partitions.erase(partitions.begin() + i);
                goto make_partition_table_consistent;
            }
            else {
                msg(usprintf("%s: BPDT partition can't fit into it's region, truncated", __FUNCTION__), partitions[i].index);
                partitions[i].ptEntry.Size = regionSize - (UINT32)partitions[i].ptEntry.Offset;
            }
        }

        // Check for intersection with previous partition
        if (partitions[i].ptEntry.Offset < previousPartitionEnd) {
            // Check if current partition is located inside previous one
            if (partitions[i].ptEntry.Offset + partitions[i].ptEntry.Size <= previousPartitionEnd) {
                msg(usprintf("%s: BPDT partition is located inside another BPDT partition, skipped", __FUNCTION__),
                    partitions[i].index);
                partitions.erase(partitions.begin() + i);
                goto make_partition_table_consistent;
            }
            else {
                msg(usprintf("%s: BPDT partition intersects with prevous one, skipped", __FUNCTION__),
                    partitions[i].index);
                partitions.erase(partitions.begin() + i);
                goto make_partition_table_consistent;
            }
        }

        // Check for padding between current and previous partitions
        else if (partitions[i].ptEntry.Offset > previousPartitionEnd) {
            padding.ptEntry.Offset = previousPartitionEnd;
            padding.ptEntry.Size = partitions[i].ptEntry.Offset - previousPartitionEnd;
            padding.type = Types::Padding;
            std::vector<BPDT_PARTITION_INFO>::iterator iter = partitions.begin();
            std::advance(iter, i);
            partitions.insert(iter, padding);
        }
    }

    // Partition map is consistent
    for (size_t i = 0; i < partitions.size(); i++) {
        if (partitions[i].type == Types::BpdtPartition) {
            // Get info
            UString name = bpdtEntryTypeToUString(partitions[i].ptEntry.Type);
            UByteArray partition = region.mid(partitions[i].ptEntry.Offset, partitions[i].ptEntry.Size);
            UByteArray signature = partition.left(sizeof(UINT32));

            UString info = usprintf("Full size: %Xh (%u)\nType: %Xh",
                                    partition.size(), partition.size(),
                                    partitions[i].ptEntry.Type) +
            UString("\nSplit sub-partition first part: ") + (partitions[i].ptEntry.SplitSubPartitionFirstPart ? "Yes" : "No") +
            UString("\nSplit sub-partition second part: ") + (partitions[i].ptEntry.SplitSubPartitionSecondPart ? "Yes" : "No") +
            UString("\nCode sub-partition: ") + (partitions[i].ptEntry.CodeSubPartition ? "Yes" : "No") +
            UString("\nUMA cachable: ") + (partitions[i].ptEntry.UmaCachable ? "Yes" : "No");

            UString text = bpdtEntryTypeToUString(partitions[i].ptEntry.Type);

            // Add tree item
            UModelIndex partitionIndex = model->addItem(localOffset + partitions[i].ptEntry.Offset, Types::BpdtPartition, 0, name, text, info, UByteArray(), partition, UByteArray(), Fixed, parent);

            // Special case of S-BPDT
            if (partitions[i].ptEntry.Type == BPDT_ENTRY_TYPE_SBPDT) {
                UModelIndex sbpdtIndex;
                parseBpdtRegion(partition, 0, partitions[i].ptEntry.Offset, partitionIndex, sbpdtIndex); // Third parameter is a fixup for S-BPDT offset entries, because they are calculated from the start of BIOS region
            }

            // Parse code partitions
            if (readUnaligned((const UINT32*)partition.constData()) == CPD_SIGNATURE) {
                // Parse code partition contents
                UModelIndex cpdIndex;
                parseCpdRegion(partition, localOffset, partitionIndex, cpdIndex);
            }

            if (partitions[i].ptEntry.Type > BPDT_LAST_KNOWN_ENTRY_TYPE) {
                msg(usprintf("%s: BPDT entry of unknown type found", __FUNCTION__), partitionIndex);
            }
        }
        else if (partitions[i].type == Types::Padding) {
            UByteArray partition = region.mid(partitions[i].ptEntry.Offset, partitions[i].ptEntry.Size);

            // Get info
            name = UString("Padding");
            info = usprintf("Full size: %Xh (%u)",
                            partition.size(), partition.size());

            // Add tree item
            model->addItem(localOffset + partitions[i].ptEntry.Offset, Types::Padding, getPaddingType(partition), name, UString(), info, UByteArray(), partition, UByteArray(), Fixed, parent);
        }
    }

    // Add padding after the last region
    if ((UINT64)partitions.back().ptEntry.Offset + (UINT64)partitions.back().ptEntry.Size < regionSize) {
        UByteArray partition = region.mid(partitions.back().ptEntry.Offset + partitions.back().ptEntry.Size, regionSize - padding.ptEntry.Offset);

        // Get info
        name = UString("Padding");
        info = usprintf("Full size: %Xh (%u)",
                        partition.size(), partition.size());

        // Add tree item
        model->addItem(localOffset + partitions.back().ptEntry.Offset + partitions.back().ptEntry.Size, Types::Padding, getPaddingType(partition), name, UString(), info, UByteArray(), partition, UByteArray(), Fixed, parent);
    }

    return U_SUCCESS;
}

USTATUS FfsParser::parseCpdRegion(const UByteArray & region, const UINT32 localOffset, const UModelIndex & parent, UModelIndex & index)
{
    // Check directory size
    if ((UINT32)region.size() < sizeof(CPD_REV1_HEADER)) {
        msg(usprintf("%s: CPD too small to fit rev1 partition table header", __FUNCTION__), parent);
        return U_INVALID_ME_PARTITION_TABLE;
    }

    // Populate partition table header
    const CPD_REV1_HEADER* cpdHeader = (const CPD_REV1_HEADER*)region.constData();

    // Check header version to be known
    UINT32 ptHeaderSize = 0;
    if (cpdHeader->HeaderVersion == 2) {
        if ((UINT32)region.size() < sizeof(CPD_REV2_HEADER)) {
            msg(usprintf("%s: CPD too small to fit rev2 partition table header", __FUNCTION__), parent);
            return U_INVALID_ME_PARTITION_TABLE;
        }

        ptHeaderSize = sizeof(CPD_REV2_HEADER);
    }
    else if (cpdHeader->HeaderVersion == 1) {
        ptHeaderSize = sizeof(CPD_REV1_HEADER);
    }

    // Check directory size again
    UINT32 ptBodySize = cpdHeader->NumEntries * sizeof(CPD_ENTRY);
    UINT32 ptSize = ptHeaderSize + ptBodySize;
    if ((UINT32)region.size() < ptSize) {
        msg(usprintf("%s: CPD too small to fit the whole partition table", __FUNCTION__), parent);
        return U_INVALID_ME_PARTITION_TABLE;
    }

    // Get info
    UByteArray header = region.left(ptHeaderSize);
    UByteArray body = region.mid(ptHeaderSize);
    UString name = usprintf("CPD partition table");
    UString info = usprintf("Full size: %Xh (%u)\nHeader size: %Xh (%u)\nBody size: %Xh (%u)\nNumber of entries: %u\n"
                            "Header version: %02X\nEntry version: %02X",
                            region.size(), region.size(),
                            header.size(), header.size(),
                            body.size(), body.size(),
                            cpdHeader->NumEntries,
                            cpdHeader->HeaderVersion,
                            cpdHeader->EntryVersion);

    // Add tree item
    index = model->addItem(localOffset, Types::CpdStore, 0, name, UString(), info, header, body, UByteArray(), Fixed, parent);

    // Add partition table entries
    std::vector<CPD_PARTITION_INFO> partitions;
    UINT32 offset = ptHeaderSize;
    const CPD_ENTRY* firstCpdEntry = (const CPD_ENTRY*)(body.constData());
    for (UINT32 i = 0; i < cpdHeader->NumEntries; i++) {
        // Populate entry header
        const CPD_ENTRY* cpdEntry = firstCpdEntry + i;
        UByteArray entry((const char*)cpdEntry, sizeof(CPD_ENTRY));

        // Get info
        name = usprintf("%c%c%c%c%c%c%c%c%c%c%c%c",
                        cpdEntry->EntryName[0], cpdEntry->EntryName[1], cpdEntry->EntryName[2], cpdEntry->EntryName[3],
                        cpdEntry->EntryName[4], cpdEntry->EntryName[5], cpdEntry->EntryName[6], cpdEntry->EntryName[7],
                        cpdEntry->EntryName[8], cpdEntry->EntryName[9], cpdEntry->EntryName[10], cpdEntry->EntryName[11]);
        info = usprintf("Full size: %Xh (%u)\nEntry offset: %Xh\nEntry length: %Xh\nHuffman compressed: ",
                        entry.size(), entry.size(),
                        cpdEntry->Offset.Offset,
                        cpdEntry->Length)
        + (cpdEntry->Offset.HuffmanCompressed ? "Yes" : "No");

        // Add tree item
        UModelIndex entryIndex = model->addItem(offset, Types::CpdEntry, 0, name, UString(), info, UByteArray(), entry, UByteArray(), Fixed, index);

        // Adjust offset
        offset += sizeof(CPD_ENTRY);

        if (cpdEntry->Offset.Offset != 0 && cpdEntry->Length != 0) {
            // Add to partitions vector
            CPD_PARTITION_INFO partition;
            partition.type = Types::CpdPartition;
            partition.ptEntry = *cpdEntry;
            partition.index = entryIndex;
            partitions.push_back(partition);
        }
    }

    // Add padding if there's no partions to add
    if (partitions.size() == 0) {
        UByteArray partition = region.mid(ptSize);

        // Get info
        name = UString("Padding");
        info = usprintf("Full size: %Xh (%u)",
                        partition.size(), partition.size());

        // Add tree item
        model->addItem(localOffset + ptSize, Types::Padding, getPaddingType(partition), name, UString(), info, UByteArray(), partition, UByteArray(), Fixed, parent);

        return U_SUCCESS;
    }

    // Sort partitions by offset
    std::sort(partitions.begin(), partitions.end());

    // Because lenghts for all Huffmann-compressed partitions mean nothing at all, we need to split all partitions into 2 classes:
    // 1. CPD manifest (should be the first)
    // 2. Metadata entries (should begin right after partition manifest and end before any code partition)
    UINT32 i = 1;
    while (i < partitions.size()) {
        name = usprintf("%c%c%c%c%c%c%c%c%c%c%c%c",
                        partitions[i].ptEntry.EntryName[0], partitions[i].ptEntry.EntryName[1], partitions[i].ptEntry.EntryName[2],  partitions[i].ptEntry.EntryName[3],
                        partitions[i].ptEntry.EntryName[4], partitions[i].ptEntry.EntryName[5], partitions[i].ptEntry.EntryName[6],  partitions[i].ptEntry.EntryName[7],
                        partitions[i].ptEntry.EntryName[8], partitions[i].ptEntry.EntryName[9], partitions[i].ptEntry.EntryName[10], partitions[i].ptEntry.EntryName[11]);

        // Check if the current entry is metadata entry
        if (!name.contains(".met")) {
            // No need to parse further, all metadata partitions are parsed
            break;
        }

        // Parse into data block, find Module Attributes extension, and get compressed size from there
        UINT32 offset = 0;
        UINT32 length = 0xFFFFFFFF; // Special guardian value
        UByteArray partition = region.mid(partitions[i].ptEntry.Offset.Offset, partitions[i].ptEntry.Length);
        while (offset < (UINT32)partition.size()) {
            const CPD_EXTENTION_HEADER* extHeader = (const CPD_EXTENTION_HEADER*) (partition.constData() + offset);
            if (extHeader->Length <= ((UINT32)partition.size() - offset)) {
                if (extHeader->Type == CPD_EXT_TYPE_MODULE_ATTRIBUTES) {
                    const CPD_EXT_MODULE_ATTRIBUTES* attrHeader = (const CPD_EXT_MODULE_ATTRIBUTES*)(partition.constData() + offset);
                    length = attrHeader->CompressedSize;
                }
                offset += extHeader->Length;
            }
            else break;
        }

        // Search down for corresponding code partition
        // Construct it's name by replacing last 4 non-zero butes of the name with zeros
        UINT32 j = 0;
        for (UINT32 k = 11; k > 0 && j < 4; k--) {
            if (name[k] != '\x00') {
                name[k] = '\x00';
                j++;
            }
        }

        // Search
        j = i + 1;
        while (j < partitions.size()) {
            if (name == usprintf("%c%c%c%c%c%c%c%c%c%c%c%c",
                                 partitions[j].ptEntry.EntryName[0], partitions[j].ptEntry.EntryName[1], partitions[j].ptEntry.EntryName[2],  partitions[j].ptEntry.EntryName[3],
                                 partitions[j].ptEntry.EntryName[4], partitions[j].ptEntry.EntryName[5], partitions[j].ptEntry.EntryName[6],  partitions[j].ptEntry.EntryName[7],
                                 partitions[j].ptEntry.EntryName[8], partitions[j].ptEntry.EntryName[9], partitions[j].ptEntry.EntryName[10], partitions[j].ptEntry.EntryName[11])) {
                // Found it, update it's Length if needed
                if (partitions[j].ptEntry.Offset.HuffmanCompressed) {
                    partitions[j].ptEntry.Length = length;
                }
                else if (length != 0xFFFFFFFF && partitions[j].ptEntry.Length != length) {
                    msg(usprintf("%s: partition size mismatch between partition table (%Xh) and partition metadata (%Xh)", __FUNCTION__,
                                 partitions[j].ptEntry.Length, length), partitions[j].index);
                    partitions[j].ptEntry.Length = length; // Believe metadata
                }
                // No need to search further
                break;
            }
            // Check the next partition
            j++;
        }
        // Check the next partition
        i++;
    }

make_partition_table_consistent:
    // Sort partitions by offset
    std::sort(partitions.begin(), partitions.end());

    // Check for intersections and paddings between partitions
    CPD_PARTITION_INFO padding;

    // Check intersection with the partition table header
    if (partitions.front().ptEntry.Offset.Offset < ptSize) {
        msg(usprintf("%s: CPD partition has intersection with CPD partition table, skipped", __FUNCTION__),
            partitions.front().index);
        partitions.erase(partitions.begin());
        goto make_partition_table_consistent;
    }
    // Check for padding between partition table and the first partition
    else if (partitions.front().ptEntry.Offset.Offset > ptSize) {
        padding.ptEntry.Offset.Offset = ptSize;
        padding.ptEntry.Length = partitions.front().ptEntry.Offset.Offset - padding.ptEntry.Offset.Offset;
        padding.type = Types::Padding;
        partitions.insert(partitions.begin(), padding);
    }
    // Check for intersections/paddings between partitions
    for (size_t i = 1; i < partitions.size(); i++) {
        UINT32 previousPartitionEnd = partitions[i - 1].ptEntry.Offset.Offset + partitions[i - 1].ptEntry.Length;

        // Check that current region is fully present in the image
        if ((UINT64)partitions[i].ptEntry.Offset.Offset + (UINT64)partitions[i].ptEntry.Length > (UINT64)region.size()) {
            if ((UINT64)partitions[i].ptEntry.Offset.Offset >= (UINT64)region.size()) {
                msg(usprintf("%s: CPD partition is located outside of the opened image, skipped", __FUNCTION__), partitions[i].index);
                partitions.erase(partitions.begin() + i);
                goto make_partition_table_consistent;
            }
            else {
                msg(usprintf("%s: CPD partition can't fit into it's region, truncated", __FUNCTION__), partitions[i].index);
                partitions[i].ptEntry.Length = (UINT32)region.size() - (UINT32)partitions[i].ptEntry.Offset.Offset;
            }
        }

        // Check for intersection with previous partition
        if (partitions[i].ptEntry.Offset.Offset < previousPartitionEnd) {
            // Check if current partition is located inside previous one
            if (partitions[i].ptEntry.Offset.Offset + partitions[i].ptEntry.Length <= previousPartitionEnd) {
                msg(usprintf("%s: CPD partition is located inside another CPD partition, skipped", __FUNCTION__),
                    partitions[i].index);
                partitions.erase(partitions.begin() + i);
                goto make_partition_table_consistent;
            }
            else {
                msg(usprintf("%s: CPD partition intersects with previous one, skipped", __FUNCTION__),
                    partitions[i].index);
                partitions.erase(partitions.begin() + i);
                goto make_partition_table_consistent;
            }
        }
        // Check for padding between current and previous partitions
        else if (partitions[i].ptEntry.Offset.Offset > previousPartitionEnd) {
            padding.ptEntry.Offset.Offset = previousPartitionEnd;
            padding.ptEntry.Length = partitions[i].ptEntry.Offset.Offset - previousPartitionEnd;
            padding.type = Types::Padding;
            std::vector<CPD_PARTITION_INFO>::iterator iter = partitions.begin();
            std::advance(iter, i);
            partitions.insert(iter, padding);
        }
    }
    // Check for padding after the last region
    if ((UINT64)partitions.back().ptEntry.Offset.Offset + (UINT64)partitions.back().ptEntry.Length < (UINT64)region.size()) {
        padding.ptEntry.Offset.Offset = partitions.back().ptEntry.Offset.Offset + partitions.back().ptEntry.Length;
        padding.ptEntry.Length = (UINT32)region.size() - padding.ptEntry.Offset.Offset;
        padding.type = Types::Padding;
        partitions.push_back(padding);
    }

    // Partition map is consistent
    for (size_t i = 0; i < partitions.size(); i++) {
        if (partitions[i].type == Types::CpdPartition) {
            UByteArray partition = region.mid(partitions[i].ptEntry.Offset.Offset, partitions[i].ptEntry.Length);

            // Get info
            name = usprintf("%c%c%c%c%c%c%c%c%c%c%c%c",
                            partitions[i].ptEntry.EntryName[0], partitions[i].ptEntry.EntryName[1], partitions[i].ptEntry.EntryName[2], partitions[i].ptEntry.EntryName[3],
                            partitions[i].ptEntry.EntryName[4], partitions[i].ptEntry.EntryName[5], partitions[i].ptEntry.EntryName[6], partitions[i].ptEntry.EntryName[7],
                            partitions[i].ptEntry.EntryName[8], partitions[i].ptEntry.EntryName[9], partitions[i].ptEntry.EntryName[10], partitions[i].ptEntry.EntryName[11]);

            // It's a manifest
            if (name.contains(".man")) {
                if (!partitions[i].ptEntry.Offset.HuffmanCompressed
                    && partitions[i].ptEntry.Length >= sizeof(CPD_MANIFEST_HEADER)) {
                    const CPD_MANIFEST_HEADER* manifestHeader = (const CPD_MANIFEST_HEADER*) partition.constData();
                    if (manifestHeader->HeaderId == ME_MANIFEST_HEADER_ID) {
                        UByteArray header = partition.left(manifestHeader->HeaderLength * sizeof(UINT32));
                        UByteArray body = partition.mid(header.size());

                        info += usprintf(
                                         "\nHeader type: %u\nHeader length: %Xh (%u)\nHeader version: %Xh\nFlags: %08Xh\nVendor: %Xh\n"
                                         "Date: %Xh\nSize: %Xh (%u)\nVersion: %u.%u.%u.%u\nSecurity version number: %u\nModulus size: %Xh (%u)\nExponent size: %Xh (%u)",
                                         manifestHeader->HeaderType,
                                         manifestHeader->HeaderLength * sizeof(UINT32), manifestHeader->HeaderLength * sizeof(UINT32),
                                         manifestHeader->HeaderVersion,
                                         manifestHeader->Flags,
                                         manifestHeader->Vendor,
                                         manifestHeader->Date,
                                         manifestHeader->Size * sizeof(UINT32), manifestHeader->Size * sizeof(UINT32),
                                         manifestHeader->VersionMajor, manifestHeader->VersionMinor, manifestHeader->VersionBugfix, manifestHeader->VersionBuild,
                                         manifestHeader->SecurityVersion,
                                         manifestHeader->ModulusSize * sizeof(UINT32), manifestHeader->ModulusSize * sizeof(UINT32),
                                         manifestHeader->ExponentSize * sizeof(UINT32), manifestHeader->ExponentSize * sizeof(UINT32));

                        // Add tree item
                        UModelIndex partitionIndex = model->addItem(localOffset + partitions[i].ptEntry.Offset.Offset, Types::CpdPartition, Subtypes::ManifestCpdPartition, name, UString(), info, header, body, UByteArray(), Fixed, parent);

                        // Parse data as extensions area
                        parseCpdExtensionsArea(partitionIndex);
                    }
                }
            }
            // It's a metadata
            else if (name.contains(".met")) {
                info = usprintf("Full size: %Xh (%u)\nEntry offset: %Xh\nEntry length: %Xh\nHuffman compressed: ",
                                partition.size(), partition.size(),
                                partitions[i].ptEntry.Offset.Offset,
                                partitions[i].ptEntry.Length)
                + (partitions[i].ptEntry.Offset.HuffmanCompressed ? "Yes" : "No");

                // Calculate SHA256 hash over the metadata and add it to it's info
                UByteArray hash(SHA256_DIGEST_SIZE, '\x00');
                sha256(partition.constData(), partition.size(), hash.data());
                info += UString("\nMetadata hash: ") + UString(hash.toHex().constData());

                // Add three item
                UModelIndex partitionIndex = model->addItem(localOffset + partitions[i].ptEntry.Offset.Offset, Types::CpdPartition,  Subtypes::MetadataCpdPartition, name, UString(), info, UByteArray(), partition, UByteArray(), Fixed, parent);

                // Parse data as extensions area
                parseCpdExtensionsArea(partitionIndex);
            }
            // It's a key
            else if (name.contains(".key")) {
                info = usprintf("Full size: %Xh (%u)\nEntry offset: %Xh\nEntry length: %Xh\nHuffman compressed: ",
                                partition.size(), partition.size(),
                                partitions[i].ptEntry.Offset.Offset,
                                partitions[i].ptEntry.Length)
                + (partitions[i].ptEntry.Offset.HuffmanCompressed ? "Yes" : "No");

                // Calculate SHA256 hash over the key and add it to it's info
                UByteArray hash(SHA256_DIGEST_SIZE, '\x00');
                sha256(partition.constData(), partition.size(), hash.data());
                info += UString("\nHash: ") + UString(hash.toHex().constData());

                // Add three item
                UModelIndex partitionIndex = model->addItem(localOffset + partitions[i].ptEntry.Offset.Offset, Types::CpdPartition,  Subtypes::KeyCpdPartition, name, UString(), info, UByteArray(), partition, UByteArray(), Fixed, parent);

                // Parse data as extensions area
                parseCpdExtensionsArea(partitionIndex);
            }
            // It's a code
            else {
                info = usprintf("Full size: %Xh (%u)\nEntry offset: %Xh\nEntry length: %Xh\nHuffman compressed: ",
                                partition.size(), partition.size(),
                                partitions[i].ptEntry.Offset.Offset,
                                partitions[i].ptEntry.Length)
                + (partitions[i].ptEntry.Offset.HuffmanCompressed ? "Yes" : "No");

                // Calculate SHA256 hash over the code and add it to it's info
                UByteArray hash(SHA256_DIGEST_SIZE, '\x00');
                sha256(partition.constData(), partition.size(), hash.data());
                info += UString("\nHash: ") + UString(hash.toHex().constData());

                UModelIndex codeIndex = model->addItem(localOffset + partitions[i].ptEntry.Offset.Offset, Types::CpdPartition, Subtypes::CodeCpdPartition, name, UString(), info, UByteArray(), partition, UByteArray(), Fixed, parent);
                parseRawArea(codeIndex);
            }
        }
        else if (partitions[i].type == Types::Padding) {
            UByteArray partition = region.mid(partitions[i].ptEntry.Offset.Offset, partitions[i].ptEntry.Length);

            // Get info
            name = UString("Padding");
            info = usprintf("Full size: %Xh (%u)", partition.size(), partition.size());

            // Add tree item
            model->addItem(localOffset + partitions[i].ptEntry.Offset.Offset, Types::Padding, getPaddingType(partition), name, UString(), info, UByteArray(), partition, UByteArray(), Fixed, parent);
        }
        else {
            msg(usprintf("%s: CPD partition of unknown type found", __FUNCTION__), parent);
            return U_INVALID_ME_PARTITION_TABLE;
        }
    }

    return U_SUCCESS;
}

USTATUS FfsParser::parseCpdExtensionsArea(const UModelIndex & index)
{
    if (!index.isValid()) {
        return U_INVALID_PARAMETER;
    }

    UByteArray body = model->body(index);
    UINT32 offset = 0;
    while (offset < (UINT32)body.size()) {
        const CPD_EXTENTION_HEADER* extHeader = (const CPD_EXTENTION_HEADER*) (body.constData() + offset);
        if (extHeader->Length <= ((UINT32)body.size() - offset)) {
            UByteArray partition = body.mid(offset, extHeader->Length);

            UString name = cpdExtensionTypeToUstring(extHeader->Type);
            UString info = usprintf("Full size: %Xh (%u)\nType: %Xh", partition.size(), partition.size(), extHeader->Type);

            // Parse Signed Package Info a bit further
            UModelIndex extIndex;
            if (extHeader->Type == CPD_EXT_TYPE_SIGNED_PACKAGE_INFO) {
                UByteArray header = partition.left(sizeof(CPD_EXT_SIGNED_PACKAGE_INFO));
                UByteArray data = partition.mid(header.size());

                const CPD_EXT_SIGNED_PACKAGE_INFO* infoHeader = (const CPD_EXT_SIGNED_PACKAGE_INFO*)header.constData();

                info = usprintf("Full size: %Xh (%u)\nHeader size: %Xh (%u)\nBody size: %Xh (%u)\nType: %Xh\n"
                                "Package name: %c%c%c%c\nVersion control number: %Xh\nSecurity version number: %Xh\n"
                                "Usage bitmap: %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
                                partition.size(), partition.size(),
                                header.size(), header.size(),
                                body.size(), body.size(),
                                infoHeader->ExtensionType,
                                infoHeader->PackageName[0], infoHeader->PackageName[1], infoHeader->PackageName[2], infoHeader->PackageName[3],
                                infoHeader->Vcn,
                                infoHeader->Svn,
                                infoHeader->UsageBitmap[0],  infoHeader->UsageBitmap[1],  infoHeader->UsageBitmap[2],  infoHeader->UsageBitmap[3],
                                infoHeader->UsageBitmap[4],  infoHeader->UsageBitmap[5],  infoHeader->UsageBitmap[6],  infoHeader->UsageBitmap[7],
                                infoHeader->UsageBitmap[8],  infoHeader->UsageBitmap[9],  infoHeader->UsageBitmap[10], infoHeader->UsageBitmap[11],
                                infoHeader->UsageBitmap[12], infoHeader->UsageBitmap[13], infoHeader->UsageBitmap[14], infoHeader->UsageBitmap[15]);

                // Add tree item
                extIndex = model->addItem(offset, Types::CpdExtension, 0, name, UString(), info, header, data, UByteArray(), Fixed, index);
                parseSignedPackageInfoData(extIndex);
            }
            // Parse IFWI Partition Manifest a bit further
            else if (extHeader->Type == CPD_EXT_TYPE_IFWI_PARTITION_MANIFEST) {
                const CPD_EXT_IFWI_PARTITION_MANIFEST* attrHeader = (const CPD_EXT_IFWI_PARTITION_MANIFEST*)partition.constData();

                // This hash is stored reversed
                // Need to reverse it back to normal
                UByteArray hash((const char*)&attrHeader->CompletePartitionHash, sizeof(attrHeader->CompletePartitionHash));
                std::reverse(hash.begin(), hash.end());

                info = usprintf("Full size: %Xh (%u)\nType: %Xh\n"
                                "Partition name: %c%c%c%c\nPartition length: %Xh\nPartition version major: %Xh\nPartition version minor: %Xh\n"
                                "Data format version: %Xh\nInstance ID: %Xh\nHash algorithm: %Xh\nHash size: %Xh\nAction on update: %Xh",
                                partition.size(), partition.size(),
                                attrHeader->ExtensionType,
                                attrHeader->PartitionName[0], attrHeader->PartitionName[1], attrHeader->PartitionName[2], attrHeader->PartitionName[3],
                                attrHeader->CompletePartitionLength,
                                attrHeader->PartitionVersionMajor, attrHeader->PartitionVersionMinor,
                                attrHeader->DataFormatVersion,
                                attrHeader->InstanceId,
                                attrHeader->HashAlgorithm,
                                attrHeader->HashSize,
                                attrHeader->ActionOnUpdate)
                + UString("\nSupport multiple instances: ") + (attrHeader->SupportMultipleInstances ? "Yes" : "No")
                + UString("\nSupport API version based update: ") + (attrHeader->SupportApiVersionBasedUpdate ? "Yes" : "No")
                + UString("\nObey full update rules: ") + (attrHeader->ObeyFullUpdateRules ? "Yes" : "No")
                + UString("\nIFR enable only: ") + (attrHeader->IfrEnableOnly ? "Yes" : "No")
                + UString("\nAllow cross point update: ") + (attrHeader->AllowCrossPointUpdate ? "Yes" : "No")
                + UString("\nAllow cross hotfix update: ") + (attrHeader->AllowCrossHotfixUpdate ? "Yes" : "No")
                + UString("\nPartial update only: ") + (attrHeader->PartialUpdateOnly ? "Yes" : "No")
                + UString("\nPartition hash: ") +  UString(hash.toHex().constData());

                // Add tree item
                extIndex = model->addItem(offset, Types::CpdExtension, 0, name, UString(), info, UByteArray(), partition, UByteArray(), Fixed, index);
            }
            // Parse Module Attributes a bit further
            else if (extHeader->Type == CPD_EXT_TYPE_MODULE_ATTRIBUTES) {
                const CPD_EXT_MODULE_ATTRIBUTES* attrHeader = (const CPD_EXT_MODULE_ATTRIBUTES*)partition.constData();

                // This hash is stored reversed
                // Need to reverse it back to normal
                UByteArray hash((const char*)&attrHeader->ImageHash, sizeof(attrHeader->ImageHash));
                std::reverse(hash.begin(), hash.end());

                info = usprintf("Full size: %Xh (%u)\nType: %Xh\n"
                                "Compression type: %Xh\nUncompressed size: %Xh (%u)\nCompressed size: %Xh (%u)\nGlobal module ID: %Xh\nImage hash: ",
                                partition.size(), partition.size(),
                                attrHeader->ExtensionType,
                                attrHeader->CompressionType,
                                attrHeader->UncompressedSize, attrHeader->UncompressedSize,
                                attrHeader->CompressedSize, attrHeader->CompressedSize,
                                attrHeader->GlobalModuleId) + UString(hash.toHex().constData());

                // Add tree item
                extIndex = model->addItem(offset, Types::CpdExtension, 0, name, UString(), info, UByteArray(), partition, UByteArray(), Fixed, index);
            }
            // Parse everything else
            else {
                // Add tree item, if needed
                extIndex = model->addItem(offset, Types::CpdExtension, 0, name, UString(), info, UByteArray(), partition, UByteArray(), Fixed, index);
            }

            if (extHeader->Type > CPD_LAST_KNOWN_EXT_TYPE) {
                msg(usprintf("%s: CPD extention of unknown type found", __FUNCTION__), extIndex);
            }

            offset += extHeader->Length;
        }
        else break;
        // TODO: add padding at the end
    }

    return U_SUCCESS;
}

USTATUS FfsParser::parseSignedPackageInfoData(const UModelIndex & index)
{
    if (!index.isValid()) {
        return U_INVALID_PARAMETER;
    }

    UByteArray body = model->body(index);
    UINT32 offset = 0;
    while (offset < (UINT32)body.size()) {
        const CPD_EXT_SIGNED_PACKAGE_INFO_MODULE* moduleHeader = (const CPD_EXT_SIGNED_PACKAGE_INFO_MODULE*)(body.constData() + offset);
        if (sizeof(CPD_EXT_SIGNED_PACKAGE_INFO_MODULE) <= ((UINT32)body.size() - offset)) {
            UByteArray module((const char*)moduleHeader, sizeof(CPD_EXT_SIGNED_PACKAGE_INFO_MODULE));

            UString name = usprintf("%c%c%c%c%c%c%c%c%c%c%c%c",
                                    moduleHeader->Name[0], moduleHeader->Name[1], moduleHeader->Name[2], moduleHeader->Name[3],
                                    moduleHeader->Name[4], moduleHeader->Name[5], moduleHeader->Name[6], moduleHeader->Name[7],
                                    moduleHeader->Name[8], moduleHeader->Name[9], moduleHeader->Name[10],moduleHeader->Name[11]);

            // This hash is stored reversed
            // Need to reverse it back to normal
            UByteArray hash((const char*)&moduleHeader->MetadataHash, sizeof(moduleHeader->MetadataHash));
            std::reverse(hash.begin(), hash.end());

            UString info = usprintf("Full size: %X (%u)\nType: %Xh\nHash algorithm: %Xh\nHash size: %Xh (%u)\nMetadata size: %Xh (%u)\nMetadata hash: ",
                                    module.size(), module.size(),
                                    moduleHeader->Type,
                                    moduleHeader->HashAlgorithm,
                                    moduleHeader->HashSize, moduleHeader->HashSize,
                                    moduleHeader->MetadataSize, moduleHeader->MetadataSize) + UString(hash.toHex().constData());
            // Add tree otem
            model->addItem(offset, Types::CpdSpiEntry, 0, name, UString(), info, UByteArray(), module, UByteArray(), Fixed, index);

            offset += module.size();
        }
        else break;
        // TODO: add padding at the end
    }

    return U_SUCCESS;
}
