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

#include <map>
#include <cmath>
#include <algorithm>
#include <inttypes.h>

// Region info structure definition
struct REGION_INFO {
    UINT32 offset;
    UINT32 length;
    UINT8  type;
    UByteArray data;
    friend bool operator< (const REGION_INFO & lhs, const REGION_INFO & rhs){ return lhs.offset < rhs.offset; }
};

// Firmware image parsing functions
USTATUS FfsParser::parse(const UByteArray & buffer) 
{
    UModelIndex root;
    USTATUS result = performFirstPass(buffer, root);
    addOffsetsRecursive(root);
    if (result)
        return result;

    if (lastVtf.isValid()) 
        result = performSecondPass(root);
    else 
        msg(("parse: not a single Volume Top File is found, the image may be corrupted"));

    return result;
}

USTATUS FfsParser::performFirstPass(const UByteArray & buffer, UModelIndex & index)
{
    // Reset capsule offset fixup value
    capsuleOffsetFixup = 0;

    // Check buffer size to be more than or equal to size of EFI_CAPSULE_HEADER
    if ((UINT32)buffer.size() <= sizeof(EFI_CAPSULE_HEADER)) {
        msg(UString("performFirstPass: image file is smaller than minimum size of 1Ch (28) bytes"));
        return U_INVALID_PARAMETER;
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
            msg(usprintf("performFirstPass: UEFI capsule header size of %Xh (%u) bytes is invalid",
                capsuleHeader->HeaderSize,
                capsuleHeader->HeaderSize));
            return U_INVALID_CAPSULE;
        }
        if (capsuleHeader->CapsuleImageSize == 0 || capsuleHeader->CapsuleImageSize > (UINT32)buffer.size()) {
            msg(usprintf("performFirstPass: UEFI capsule image size of %Xh (%u) bytes is invalid",
                capsuleHeader->CapsuleImageSize,
                capsuleHeader->CapsuleImageSize));
            return U_INVALID_CAPSULE;
        }

        capsuleHeaderSize = capsuleHeader->HeaderSize;
        UByteArray header = buffer.left(capsuleHeaderSize);
        UByteArray body = buffer.mid(capsuleHeaderSize);
        UString name("UEFI capsule");
        UString info = UString("Capsule GUID: ") + guidToUString(capsuleHeader->CapsuleGuid) + 
            usprintf("\nFull size: %Xh (%u)\nHeader size: %Xh (%u)\nImage size: %Xh (%u)\nFlags: %08Xh",
            buffer.size(), buffer.size(),
            capsuleHeaderSize, capsuleHeaderSize,
            capsuleHeader->CapsuleImageSize - capsuleHeaderSize, capsuleHeader->CapsuleImageSize - capsuleHeaderSize,
            capsuleHeader->Flags);

        // Set capsule offset fixup for correct volume allignment warnings
        capsuleOffsetFixup = capsuleHeaderSize;

        // Add tree item
        index = model->addItem(Types::Capsule, Subtypes::UefiCapsule, name, UString(), info, header, body, UByteArray(), true);
    }
    // Check buffer for being Toshiba capsule header
    else if (buffer.startsWith(TOSHIBA_CAPSULE_GUID)) {
        // Get info
        const TOSHIBA_CAPSULE_HEADER* capsuleHeader = (const TOSHIBA_CAPSULE_HEADER*)buffer.constData();

        // Check sanity of HeaderSize and FullSize values
        if (capsuleHeader->HeaderSize == 0 || capsuleHeader->HeaderSize > (UINT32)buffer.size() || capsuleHeader->HeaderSize > capsuleHeader->FullSize) {
            msg(usprintf("performFirstPass: Toshiba capsule header size of %Xh (%u) bytes is invalid",
                capsuleHeader->HeaderSize, capsuleHeader->HeaderSize));
            return U_INVALID_CAPSULE;
        }
        if (capsuleHeader->FullSize == 0 || capsuleHeader->FullSize > (UINT32)buffer.size()) {
            msg(usprintf("performFirstPass: Toshiba capsule full size of %Xh (%u) bytes is invalid",
                capsuleHeader->FullSize, capsuleHeader->FullSize));
            return U_INVALID_CAPSULE;
        }

        capsuleHeaderSize = capsuleHeader->HeaderSize;
        UByteArray header = buffer.left(capsuleHeaderSize);
        UByteArray body = buffer.mid(capsuleHeaderSize);
        UString name("Toshiba capsule");
        UString info = UString("Capsule GUID: ") + guidToUString(capsuleHeader->CapsuleGuid) + 
            usprintf("\nFull size: %Xh (%u)\nHeader size: %Xh (%u)\nImage size: %Xh (%u)\nFlags: %08Xh",
            buffer.size(), buffer.size(),
            capsuleHeaderSize, capsuleHeaderSize,
            capsuleHeader->FullSize - capsuleHeaderSize, capsuleHeader->FullSize - capsuleHeaderSize,
            capsuleHeader->Flags);

        // Set capsule offset fixup for correct volume allignment warnings
        capsuleOffsetFixup = capsuleHeaderSize;

        // Add tree item
        index = model->addItem(Types::Capsule, Subtypes::ToshibaCapsule, name, UString(), info, header, body, UByteArray(), true);
    }
    // Check buffer for being extended Aptio capsule header
    else if (buffer.startsWith(APTIO_SIGNED_CAPSULE_GUID) || buffer.startsWith(APTIO_UNSIGNED_CAPSULE_GUID)) {
        bool signedCapsule = buffer.startsWith(APTIO_SIGNED_CAPSULE_GUID);

        if ((UINT32)buffer.size() <= sizeof(APTIO_CAPSULE_HEADER)) {
            msg(UString("performFirstPass: AMI capsule image file is smaller than minimum size of 20h (32) bytes"));
            return U_INVALID_PARAMETER;
        }

        // Get info
        const APTIO_CAPSULE_HEADER* capsuleHeader = (const APTIO_CAPSULE_HEADER*)buffer.constData();

        // Check sanity of RomImageOffset and CapsuleImageSize values
        if (capsuleHeader->RomImageOffset == 0 || capsuleHeader->RomImageOffset > (UINT32)buffer.size() || capsuleHeader->RomImageOffset > capsuleHeader->CapsuleHeader.CapsuleImageSize) {
            msg(usprintf("performFirstPass: AMI capsule image offset of %Xh (%u) bytes is invalid", 
                capsuleHeader->RomImageOffset, capsuleHeader->RomImageOffset));
            return U_INVALID_CAPSULE;
        }
        if (capsuleHeader->CapsuleHeader.CapsuleImageSize == 0 || capsuleHeader->CapsuleHeader.CapsuleImageSize > (UINT32)buffer.size()) {
            msg(usprintf("performFirstPass: AMI capsule image size of %Xh (%u) bytes is invalid", 
                capsuleHeader->CapsuleHeader.CapsuleImageSize, 
                capsuleHeader->CapsuleHeader.CapsuleImageSize));
            return U_INVALID_CAPSULE;
        }

        capsuleHeaderSize = capsuleHeader->RomImageOffset;
        UByteArray header = buffer.left(capsuleHeaderSize);
        UByteArray body = buffer.mid(capsuleHeaderSize);
        UString name("AMI Aptio capsule");
        UString info = UString("Capsule GUID: ") + guidToUString(capsuleHeader->CapsuleHeader.CapsuleGuid) +
            usprintf("\nFull size: %Xh (%u)\nHeader size: %Xh (%u)\nImage size: %Xh (%u)\nFlags: %08Xh",
            buffer.size(), buffer.size(),
            capsuleHeaderSize, capsuleHeaderSize,
            capsuleHeader->CapsuleHeader.CapsuleImageSize - capsuleHeaderSize, capsuleHeader->CapsuleHeader.CapsuleImageSize - capsuleHeaderSize,
            capsuleHeader->CapsuleHeader.Flags);

        // Set capsule offset fixup for correct volume allignment warnings
        capsuleOffsetFixup = capsuleHeaderSize;

        // Add tree item
        index = model->addItem(Types::Capsule, signedCapsule ? Subtypes::AptioSignedCapsule : Subtypes::AptioUnsignedCapsule, name, UString(), info, header, body, UByteArray(), true);

        // Show message about possible Aptio signature break
        if (signedCapsule) {
            msg(UString("performFirstPass: Aptio capsule signature may become invalid after image modifications"), index);
        }
    }

    // Skip capsule header to have flash chip image
    UByteArray flashImage = buffer.mid(capsuleHeaderSize);

    // Check for Intel flash descriptor presence
    const FLASH_DESCRIPTOR_HEADER* descriptorHeader = (const FLASH_DESCRIPTOR_HEADER*)flashImage.constData();

    // Check descriptor signature
    USTATUS result;
    if (descriptorHeader->Signature == FLASH_DESCRIPTOR_SIGNATURE) {
        // Parse as Intel image
        UModelIndex imageIndex;
        result = parseIntelImage(flashImage, capsuleHeaderSize, index, imageIndex);
        if (result != U_INVALID_FLASH_DESCRIPTOR) {
            if (!index.isValid())
                index = imageIndex;
            return result;
        }
    }

    // Get info
    UString name("UEFI image");
    UString info = usprintf("Full size: %Xh (%u)", flashImage.size(), flashImage.size());

    // Construct parsing data
    PARSING_DATA pdata = parsingDataFromUModelIndex(index);
    pdata.offset = capsuleHeaderSize;

    // Add tree item
    UModelIndex biosIndex = model->addItem(Types::Image, Subtypes::UefiImage, name, UString(), info, UByteArray(), flashImage, UByteArray(), true, parsingDataToUByteArray(pdata), index);

    // Parse the image
    result = parseRawArea(biosIndex);
    if (!index.isValid())
        index = biosIndex;
    return result;
}

USTATUS FfsParser::parseIntelImage(const UByteArray & intelImage, const UINT32 parentOffset, const UModelIndex & parent, UModelIndex & index)
{
    // Sanity check
    if (intelImage.isEmpty())
        return EFI_INVALID_PARAMETER;

    // Get parent's parsing data
    PARSING_DATA pdata = parsingDataFromUModelIndex(parent);

    // Store the beginning of descriptor as descriptor base address
    const UINT8* descriptor = (const UINT8*)intelImage.constData();

    // Check for buffer size to be greater or equal to descriptor region size
    if (intelImage.size() < FLASH_DESCRIPTOR_SIZE) {
        msg(usprintf("parseIntelImage: input file is smaller than minimum descriptor size of %Xh (%u) bytes", FLASH_DESCRIPTOR_SIZE, FLASH_DESCRIPTOR_SIZE));
        return U_INVALID_FLASH_DESCRIPTOR;
    }

    // Parse descriptor map
    const FLASH_DESCRIPTOR_MAP* descriptorMap = (const FLASH_DESCRIPTOR_MAP*)(descriptor + sizeof(FLASH_DESCRIPTOR_HEADER));
    const FLASH_DESCRIPTOR_UPPER_MAP* upperMap = (const FLASH_DESCRIPTOR_UPPER_MAP*)(descriptor + FLASH_DESCRIPTOR_UPPER_MAP_BASE);

    // Check sanity of base values
    if (descriptorMap->MasterBase > FLASH_DESCRIPTOR_MAX_BASE
        || descriptorMap->MasterBase == descriptorMap->RegionBase
        || descriptorMap->MasterBase == descriptorMap->ComponentBase) {
        msg(usprintf("parseIntelImage: invalid descriptor master base %02Xh", descriptorMap->MasterBase));
        return U_INVALID_FLASH_DESCRIPTOR;
    }
    if (descriptorMap->RegionBase > FLASH_DESCRIPTOR_MAX_BASE
        || descriptorMap->RegionBase == descriptorMap->ComponentBase) {
        msg(usprintf("parseIntelImage: invalid descriptor region base %02Xh", descriptorMap->RegionBase));
        return U_INVALID_FLASH_DESCRIPTOR;
    }
    if (descriptorMap->ComponentBase > FLASH_DESCRIPTOR_MAX_BASE) {
        msg(usprintf("parseIntelImage: invalid descriptor component base %02Xh", descriptorMap->ComponentBase));
        return U_INVALID_FLASH_DESCRIPTOR;
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
        msg(usprintf("parseIntelImage: unknown descriptor version with ReadClockFrequency %02Xh", componentSection->FlashParameters.ReadClockFrequency));
        return U_INVALID_FLASH_DESCRIPTOR;
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
                msg(("parseIntelImage: can't determine BIOS region start from Gigabyte-specific descriptor"));
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
        msg(("parseIntelImage: descriptor parsing failed, BIOS region not found in descriptor"));
        return U_INVALID_FLASH_DESCRIPTOR;
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
        msg(UString("parseIntelImage: ") + itemSubtypeToUString(Types::Region, regions.front().type) 
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
        if (regions[i].offset + regions[i].length > (UINT32)intelImage.size()) {
            msg(UString("parseIntelImage: ") + itemSubtypeToUString(Types::Region, regions[i].type) 
                + UString(" region is located outside of opened image, if your system uses dual-chip storage, please append another part to the opened image"),
                index);
            return U_TRUNCATED_IMAGE;
        }

        // Check for intersection with previous region
        if (regions[i].offset < previousRegionEnd) {
            msg(UString("parseIntelImage: ") + itemSubtypeToUString(Types::Region, regions[i].type) 
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
    UString name("Intel image");
    UString info = usprintf("Full size: %Xh (%u)\nFlash chips: %u\nRegions: %u\nMasters: %u\nPCH straps: %u\nPROC straps: %u",
        intelImage.size(), intelImage.size(),
        descriptorMap->NumberOfFlashChips + 1, //
        descriptorMap->NumberOfRegions + 1,    // Zero-based numbers in storage
        descriptorMap->NumberOfMasters + 1,    //
        descriptorMap->NumberOfPchStraps,
        descriptorMap->NumberOfProcStraps);

    // Construct parsing data
    pdata.offset = parentOffset;

    // Add Intel image tree item
    index = model->addItem(Types::Image, Subtypes::IntelImage, name, UString(), info, UByteArray(), intelImage, UByteArray(), true, parsingDataToUByteArray(pdata), parent);

    // Descriptor
    // Get descriptor info
    UByteArray body = intelImage.left(FLASH_DESCRIPTOR_SIZE);
    name = UString("Descriptor region");
    info = usprintf("Full size: %Xh (%u)", FLASH_DESCRIPTOR_SIZE, FLASH_DESCRIPTOR_SIZE);
    
    // Add offsets of actual regions
    for (size_t i = 0; i < regions.size(); i++) {
        if (regions[i].type != Subtypes::ZeroPadding && regions[i].type != Subtypes::OnePadding && regions[i].type != Subtypes::DataPadding)
            info += UString("\n") + itemSubtypeToUString(Types::Region, regions[i].type)
            + usprintf(" region offset: %Xh", regions[i].offset + parentOffset);
    }

    // Region access settings
    if (descriptorVersion == 1) {
        const FLASH_DESCRIPTOR_MASTER_SECTION* masterSection = (const FLASH_DESCRIPTOR_MASTER_SECTION*)calculateAddress8(descriptor, descriptorMap->MasterBase);
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
        const FLASH_DESCRIPTOR_MASTER_SECTION_V2* masterSection = (const FLASH_DESCRIPTOR_MASTER_SECTION_V2*)calculateAddress8(descriptor, descriptorMap->MasterBase);
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
    }

    // VSCC table
    const VSCC_TABLE_ENTRY* vsccTableEntry = (const VSCC_TABLE_ENTRY*)(descriptor + ((UINT16)upperMap->VsccTableBase << 4));
    info += UString("\nFlash chips in VSCC table:");
    UINT8 vsscTableSize = upperMap->VsccTableSize * sizeof(UINT32) / sizeof(VSCC_TABLE_ENTRY);
    for (int i = 0; i < vsscTableSize; i++) {
        info += usprintf("\n%02X%02X%02Xh",
            vsccTableEntry->VendorId, vsccTableEntry->DeviceId0, vsccTableEntry->DeviceId1);
        vsccTableEntry++;
    }

    // Add descriptor tree item
    UModelIndex regionIndex = model->addItem(Types::Region, Subtypes::DescriptorRegion, name, UString(), info, UByteArray(), body, UByteArray(), true, parsingDataToUByteArray(pdata), index);
    
    // Parse regions
    UINT8 result = U_SUCCESS;
    UINT8 parseResult = U_SUCCESS;
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
            UByteArray padding = intelImage.mid(region.offset, region.length);

            // Get parent's parsing data
            PARSING_DATA pdata = parsingDataFromUModelIndex(index);

            // Get info
            name = UString("Padding");
            info = usprintf("Full size: %Xh (%u)",
                padding.size(), padding.size());

            // Construct parsing data
            pdata.offset = parentOffset + region.offset;

            // Add tree item
            regionIndex = model->addItem(Types::Padding, getPaddingType(padding), name, UString(), info, UByteArray(), padding, UByteArray(), true, parsingDataToUByteArray(pdata), index);
            result = U_SUCCESS;
            } break;
        default:
            msg(("parseIntelImage: region of unknown type found"), index);
            result = U_INVALID_FLASH_DESCRIPTOR;
        }
        // Store the first failed result as a final result
        if (!parseResult && result)
            parseResult = result;
    }

    return parseResult;
}

USTATUS FfsParser::parseGbeRegion(const UByteArray & gbe, const UINT32 parentOffset, const UModelIndex & parent, UModelIndex & index)
{
    // Check sanity
    if (gbe.isEmpty())
        return U_EMPTY_REGION;
    if ((UINT32)gbe.size() < GBE_VERSION_OFFSET + sizeof(GBE_VERSION))
        return U_INVALID_REGION;

    // Get parent's parsing data
    PARSING_DATA pdata = parsingDataFromUModelIndex(parent);

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

    // Construct parsing data
    pdata.offset += parentOffset;

    // Add tree item
    index = model->addItem(Types::Region, Subtypes::GbeRegion, name, UString(), info, UByteArray(), gbe, UByteArray(), true, parsingDataToUByteArray(pdata), parent);

    return U_SUCCESS;
}

USTATUS FfsParser::parseMeRegion(const UByteArray & me, const UINT32 parentOffset, const UModelIndex & parent, UModelIndex & index)
{
    // Check sanity
    if (me.isEmpty())
        return U_EMPTY_REGION;

    // Get parent's parsing data
    PARSING_DATA pdata = parsingDataFromUModelIndex(parent);

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
                version->major,
                version->minor,
                version->bugfix,
                version->build);
        }
    }

    // Construct parsing data
    pdata.offset += parentOffset;

    // Add tree item
    index = model->addItem(Types::Region, Subtypes::MeRegion, name, UString(), info, UByteArray(), me, UByteArray(), true, parsingDataToUByteArray(pdata), parent);

    // Show messages
    if (emptyRegion) {
        msg(UString("parseMeRegion: ME region is empty"), index);
    }
    else if (!versionFound) {
        msg(UString("parseMeRegion: ME version is unknown, it can be damaged"), index);
    }

    return U_SUCCESS;
}

USTATUS FfsParser::parsePdrRegion(const UByteArray & pdr, const UINT32 parentOffset, const UModelIndex & parent, UModelIndex & index)
{
    // Check sanity
    if (pdr.isEmpty())
        return U_EMPTY_REGION;

    // Get parent's parsing data
    PARSING_DATA pdata = parsingDataFromUModelIndex(parent);

    // Get info
    UString name("PDR region");
    UString info = usprintf("Full size: %Xh (%u)", pdr.size(), pdr.size());

    // Construct parsing data
    pdata.offset += parentOffset;

    // Add tree item
    index = model->addItem(Types::Region, Subtypes::PdrRegion, name, UString(), info, UByteArray(), pdr, UByteArray(), true, parsingDataToUByteArray(pdata), parent);

    // Parse PDR region as BIOS space
    UINT8 result = parseRawArea(index);
    if (result && result != U_VOLUMES_NOT_FOUND && result != U_INVALID_VOLUME)
        return result;

    return U_SUCCESS;
}

USTATUS FfsParser::parseGeneralRegion(const UINT8 subtype, const UByteArray & region, const UINT32 parentOffset, const UModelIndex & parent, UModelIndex & index)
{
    // Check sanity
    if (region.isEmpty())
        return U_EMPTY_REGION;

    // Get parent's parsing data
    PARSING_DATA pdata = parsingDataFromUModelIndex(parent);

    // Get info
    UString name = itemSubtypeToUString(Types::Region, subtype) + UString(" region");
    UString info = usprintf("Full size: %Xh (%u)", region.size(), region.size());

    // Construct parsing data
    pdata.offset += parentOffset;

    // Add tree item
    index = model->addItem(Types::Region, subtype, name, UString(), info, UByteArray(), region, UByteArray(), true, parsingDataToUByteArray(pdata), parent);

    return U_SUCCESS;
}

USTATUS FfsParser::parseBiosRegion(const UByteArray & bios, const UINT32 parentOffset, const UModelIndex & parent, UModelIndex & index)
{
    // Sanity check
    if (bios.isEmpty())
        return U_EMPTY_REGION;

    // Get parent's parsing data
    PARSING_DATA pdata = parsingDataFromUModelIndex(parent);

    // Get info
    UString name("BIOS region");
    UString info = usprintf("Full size: %Xh (%u)", bios.size(), bios.size());

    // Construct parsing data
    pdata.offset += parentOffset;

    // Add tree item
    index = model->addItem(Types::Region, Subtypes::BiosRegion, name, UString(), info, UByteArray(), bios, UByteArray(), true, parsingDataToUByteArray(pdata), parent);

    return parseRawArea(index);
}

UINT8 FfsParser::getPaddingType(const UByteArray & padding)
{
    if (padding.count('\x00') == padding.size())
        return Subtypes::ZeroPadding;
    if (padding.count('\xFF') == padding.size())
        return Subtypes::OnePadding;
    return Subtypes::DataPadding;
}

USTATUS FfsParser::parseRawArea(const UModelIndex & index)
{
    // Sanity check
    if (!index.isValid())
        return U_INVALID_PARAMETER;

    // Get parsing data
    PARSING_DATA pdata = parsingDataFromUModelIndex(index);
    UINT32 headerSize = model->header(index).size();
    UINT32 offset = pdata.offset + headerSize;

    // Get item data
    UByteArray data = model->body(index);

    // Search for first volume
    USTATUS result;
    UINT32 prevVolumeOffset;

    result = findNextVolume(index, data, offset, 0, prevVolumeOffset);
    if (result)
        return result;

    // First volume is not at the beginning of RAW area
    UString name;
    UString info;
    if (prevVolumeOffset > 0) {
        // Get info
        UByteArray padding = data.left(prevVolumeOffset);
        name = UString("Padding");
        info = usprintf("Full size: %Xh (%u)", padding.size(), padding.size());

        // Construct parsing data
        pdata.offset = offset;

        // Add tree item
        model->addItem(Types::Padding, getPaddingType(padding), name, UString(), info, UByteArray(), padding, UByteArray(), true, parsingDataToUByteArray(pdata), index);
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
            UByteArray padding = data.mid(paddingOffset, paddingSize);

            // Get info
            name = UString("Padding");
            info = usprintf("Full size: %Xh (%u)", padding.size(), padding.size());

            // Construct parsing data
            pdata.offset = offset + paddingOffset;

            // Add tree item
            model->addItem(Types::Padding, getPaddingType(padding), name, UString(), info, UByteArray(), padding, UByteArray(), true, parsingDataToUByteArray(pdata), index);
        }

        // Get volume size
        UINT32 volumeSize = 0;
        UINT32 bmVolumeSize = 0;
        result = getVolumeSize(data, volumeOffset, volumeSize, bmVolumeSize);
        if (result) {
            msg(UString("parseRawArea: getVolumeSize failed with error ") + errorCodeToUString(result), index);
            return result;
        }
        
        // Check that volume is fully present in input
        if (volumeSize > (UINT32)data.size() || volumeOffset + volumeSize > (UINT32)data.size()) {
            msg(UString("parseRawArea: one of volumes inside overlaps the end of data"), index);
            return U_INVALID_VOLUME;
        }
        
        UByteArray volume = data.mid(volumeOffset, volumeSize);
        if (volumeSize > (UINT32)volume.size()) {
            // Mark the rest as padding and finish the parsing
            UByteArray padding = data.right(volume.size());

            // Get info
            name = UString("Padding");
            info = usprintf("Full size: %Xh (%u)", padding.size(), padding.size());

            // Construct parsing data
            pdata.offset = offset + volumeOffset;

            // Add tree item
            UModelIndex paddingIndex = model->addItem(Types::Padding, getPaddingType(padding), name, UString(), info, UByteArray(), padding, UByteArray(), true, parsingDataToUByteArray(pdata), index);
            msg(UString("parseRawArea: one of volumes inside overlaps the end of data"), paddingIndex);

            // Update variables
            prevVolumeOffset = volumeOffset;
            prevVolumeSize = padding.size();
            break;
        }

        // Parse current volume's header
        UModelIndex volumeIndex;
        result = parseVolumeHeader(volume, headerSize + volumeOffset, index, volumeIndex);
        if (result)
            msg(UString("parseRawArea: volume header parsing failed with error ") + errorCodeToUString(result), index);
        else {
            // Show messages
            if (volumeSize != bmVolumeSize)
                msg(usprintf("parseRawArea: volume size stored in header %Xh (%u) differs from calculated using block map %Xh (%u)",
                volumeSize, volumeSize,
                bmVolumeSize, bmVolumeSize),
                volumeIndex);
        }

        // Go to next volume
        prevVolumeOffset = volumeOffset;
        prevVolumeSize = volumeSize;
        result = findNextVolume(index, data, offset, volumeOffset + prevVolumeSize, volumeOffset);
    }

    // Padding at the end of RAW area
    volumeOffset = prevVolumeOffset + prevVolumeSize;
    if ((UINT32)data.size() > volumeOffset) {
        UByteArray padding = data.mid(volumeOffset);

        // Get info
        name = UString("Padding");
        info = usprintf("Full size: %Xh (%u)", padding.size(), padding.size());

        // Construct parsing data
        pdata.offset = offset + headerSize + volumeOffset;

        // Add tree item
        model->addItem(Types::Padding, getPaddingType(padding), name, UString(), info, UByteArray(), padding, UByteArray(), true, parsingDataToUByteArray(pdata), index);
    }

    // Parse bodies
    for (int i = 0; i < model->rowCount(index); i++) {
        UModelIndex current = index.child(i, 0);
        switch (model->type(current)) {
        case Types::Volume:
            parseVolumeBody(current);
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

USTATUS FfsParser::parseVolumeHeader(const UByteArray & volume, const UINT32 parentOffset, const UModelIndex & parent, UModelIndex & index)
{
    // Sanity check
    if (volume.isEmpty())
        return U_INVALID_PARAMETER;

    // Get parent's parsing data
    PARSING_DATA pdata = parsingDataFromUModelIndex(parent);

    // Check that there is space for the volume header
        if ((UINT32)volume.size() < sizeof(EFI_FIRMWARE_VOLUME_HEADER)) {
        msg(usprintf("parseVolumeHeader: input volume size %Xh (%u) is smaller than volume header size 40h (64)", volume.size(), volume.size()));
        return U_INVALID_VOLUME;
    }

    // Populate volume header
    const EFI_FIRMWARE_VOLUME_HEADER* volumeHeader = (const EFI_FIRMWARE_VOLUME_HEADER*)(volume.constData());

    // Check sanity of HeaderLength value
    if ((UINT32)ALIGN8(volumeHeader->HeaderLength) > (UINT32)volume.size()) {
        msg(UString("parseVolumeHeader: volume header overlaps the end of data"));
        return U_INVALID_VOLUME;
    }
    // Check sanity of ExtHeaderOffset value
    if (volumeHeader->Revision > 1 && volumeHeader->ExtHeaderOffset
        && (UINT32)ALIGN8(volumeHeader->ExtHeaderOffset + sizeof(EFI_FIRMWARE_VOLUME_EXT_HEADER)) > (UINT32)volume.size()) {
        msg(UString("parseVolumeHeader: extended volume header overlaps the end of data"));
        return U_INVALID_VOLUME;
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
    bool isNvramVolume = false;
    UINT8 ffsVersion = 0;

    // Check for FFS v2 volume
    UByteArray guid = UByteArray((const char*)volumeHeader->FileSystemGuid.Data, sizeof(EFI_GUID));
    if (std::find(FFSv2Volumes.begin(), FFSv2Volumes.end(), guid) != FFSv2Volumes.end()) {
        isUnknown = false;
        ffsVersion = 2;
    }

    // Check for FFS v3 volume
    if (std::find(FFSv3Volumes.begin(), FFSv3Volumes.end(), guid) != FFSv3Volumes.end()) {
        isUnknown = false;
        ffsVersion = 3;
    }

    // Check for VSS NVRAM volume
    if (guid == NVRAM_MAIN_STORE_VOLUME_GUID || guid == NVRAM_ADDITIONAL_STORE_VOLUME_GUID) {
        isUnknown = false;
        isNvramVolume = true;
    }

    // Check volume revision and alignment
    bool msgAlignmentBitsSet = false;
    bool msgUnaligned = false;
    bool msgUnknownRevision = false;
    UINT32 alignment = 65536; // Default volume alignment is 64K
    if (volumeHeader->Revision == 1) {
        // Acquire alignment capability bit
        bool alignmentCap = (volumeHeader->Attributes & EFI_FVB_ALIGNMENT_CAP) != 0;
        if (!alignmentCap) {
            if (volumeHeader->Attributes & 0xFFFF0000)
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
    UByteArray tempHeader((const char*)volumeHeader, volumeHeader->HeaderLength);
    ((EFI_FIRMWARE_VOLUME_HEADER*)tempHeader.data())->Checksum = 0;
    UINT16 calculated = calculateChecksum16((const UINT16*)tempHeader.constData(), volumeHeader->HeaderLength);
    if (volumeHeader->Checksum != calculated)
        msgInvalidChecksum = true;

    // Get info
    UByteArray header = volume.left(headerSize);
    UByteArray body = volume.mid(headerSize);
    UString name = guidToUString(volumeHeader->FileSystemGuid);
    UString info = usprintf("Signature: _FVH\nZeroVector:\n%02X %02X %02X %02X %02X %02X %02X %02X\n"
        "%02X %02X %02X %02X %02X %02X %02X %02X\nFileSystem GUID: ", 
        volumeHeader->ZeroVector[0], volumeHeader->ZeroVector[1], volumeHeader->ZeroVector[2], volumeHeader->ZeroVector[3],
        volumeHeader->ZeroVector[4], volumeHeader->ZeroVector[5], volumeHeader->ZeroVector[6], volumeHeader->ZeroVector[7],
        volumeHeader->ZeroVector[8], volumeHeader->ZeroVector[9], volumeHeader->ZeroVector[10], volumeHeader->ZeroVector[11],
        volumeHeader->ZeroVector[12], volumeHeader->ZeroVector[13], volumeHeader->ZeroVector[14], volumeHeader->ZeroVector[15])
        + guidToUString(volumeHeader->FileSystemGuid) \
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
            extendedHeader->ExtHeaderSize, extendedHeader->ExtHeaderSize) + guidToUString(extendedHeader->FvName);
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
    UString text;
    if (hasAppleCrc32)
        text += UString("AppleCRC32 ");
    if (hasAppleFSO)
        text += UString("AppleFSO ");

    // Add tree item
    UINT8 subtype = Subtypes::UnknownVolume;
    if (!isUnknown) {
        if (ffsVersion == 2)
            subtype = Subtypes::Ffs2Volume;
        else if (ffsVersion == 3)
            subtype = Subtypes::Ffs3Volume;
        else if (isNvramVolume)
            subtype = Subtypes::NvramVolume;
    }
    index = model->addItem(Types::Volume, subtype, name, text, info, header, body, UByteArray(), true, parsingDataToUByteArray(pdata), parent);

    // Show messages
    if (isUnknown)
        msg(UString("parseVolumeHeader: unknown file system ") + guidToUString(volumeHeader->FileSystemGuid), index);
    if (msgInvalidChecksum)
        msg(UString("parseVolumeHeader: volume header checksum is invalid"), index);
    if (msgAlignmentBitsSet)
        msg(UString("parseVolumeHeader: alignment bits set on volume without alignment capability"), index);
    if (msgUnaligned)
        msg(UString("parseVolumeHeader: unaligned volume"), index);
    if (msgUnknownRevision)
        msg(usprintf("parseVolumeHeader: unknown volume revision %u", volumeHeader->Revision), index);

    return U_SUCCESS;
}

USTATUS FfsParser::findNextVolume(const UModelIndex & index, const UByteArray & bios, const UINT32 parentOffset, const UINT32 volumeOffset, UINT32 & nextVolumeOffset)
{
    int nextIndex = bios.indexOf(EFI_FV_SIGNATURE, volumeOffset);
    if (nextIndex < EFI_FV_SIGNATURE_OFFSET)
        return U_VOLUMES_NOT_FOUND;

    // Check volume header to be sane
    for (; nextIndex > 0; nextIndex = bios.indexOf(EFI_FV_SIGNATURE, nextIndex + 1)) {
        const EFI_FIRMWARE_VOLUME_HEADER* volumeHeader = (const EFI_FIRMWARE_VOLUME_HEADER*)(bios.constData() + nextIndex - EFI_FV_SIGNATURE_OFFSET);
        if (volumeHeader->FvLength < sizeof(EFI_FIRMWARE_VOLUME_HEADER) + 2 * sizeof(EFI_FV_BLOCK_MAP_ENTRY) || volumeHeader->FvLength >= 0xFFFFFFFFUL) {
            msg(usprintf("findNextVolume: volume candidate at offset %Xh skipped, has invalid FvLength %" PRIX64 "h", 
                parentOffset + (nextIndex - EFI_FV_SIGNATURE_OFFSET), 
                volumeHeader->FvLength), index);
            continue;
        }
        if (volumeHeader->Reserved != 0xFF && volumeHeader->Reserved != 0x00) {
            msg(usprintf("findNextVolume: volume candidate at offset %Xh skipped, has invalid Reserved byte value %02Xh", 
                parentOffset + (nextIndex - EFI_FV_SIGNATURE_OFFSET),
                volumeHeader->Reserved), index);
            continue;
        }
        if (volumeHeader->Revision != 1 && volumeHeader->Revision != 2) {
            msg(usprintf("findNextVolume: volume candidate at offset %Xh skipped, has invalid Revision byte value %02Xh", 
                parentOffset + (nextIndex - EFI_FV_SIGNATURE_OFFSET)
                ,volumeHeader->Revision), index);
            continue;
        }
        // All checks passed, volume found
        break;
    }
    // No more volumes found
    if (nextIndex < EFI_FV_SIGNATURE_OFFSET)
        return U_VOLUMES_NOT_FOUND;

    nextVolumeOffset = nextIndex - EFI_FV_SIGNATURE_OFFSET;
    return U_SUCCESS;
}

USTATUS FfsParser::getVolumeSize(const UByteArray & bios, UINT32 volumeOffset, UINT32 & volumeSize, UINT32 & bmVolumeSize)
{
    // Check that there is space for the volume header and at least two block map entries.
    if ((UINT32)bios.size() < volumeOffset + sizeof(EFI_FIRMWARE_VOLUME_HEADER) + 2 * sizeof(EFI_FV_BLOCK_MAP_ENTRY))
        return U_INVALID_VOLUME;

    // Populate volume header
    const EFI_FIRMWARE_VOLUME_HEADER* volumeHeader = (const EFI_FIRMWARE_VOLUME_HEADER*)(bios.constData() + volumeOffset);

    // Check volume signature
    if (UByteArray((const char*)&volumeHeader->Signature, sizeof(volumeHeader->Signature)) != EFI_FV_SIGNATURE)
        return U_INVALID_VOLUME;

    // Calculate volume size using BlockMap
    const EFI_FV_BLOCK_MAP_ENTRY* entry = (const EFI_FV_BLOCK_MAP_ENTRY*)(bios.constData() + volumeOffset + sizeof(EFI_FIRMWARE_VOLUME_HEADER));
    UINT32 calcVolumeSize = 0;
    while (entry->NumBlocks != 0 && entry->Length != 0) {
        if ((void*)entry > bios.constData() + bios.size())
            return U_INVALID_VOLUME;

        calcVolumeSize += entry->NumBlocks * entry->Length;
        entry += 1;
    }

    volumeSize = (UINT32)volumeHeader->FvLength;
    bmVolumeSize = calcVolumeSize;

    if (volumeSize == 0)
        return U_INVALID_VOLUME;

    return U_SUCCESS;
}

USTATUS FfsParser::parseVolumeNonUefiData(const UByteArray & data, const UINT32 parentOffset, const UModelIndex & index)
{
    // Sanity check
    if (!index.isValid())
        return U_INVALID_PARAMETER;

    // Get parsing data
    PARSING_DATA pdata = parsingDataFromUModelIndex(index);

    // Modify it
    pdata.offset += parentOffset;

    // Search for VTF GUID backwards in received data
    UByteArray padding = data;
    UByteArray vtf;
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
    UString info = usprintf("Full size: %Xh (%u)", padding.size(), padding.size());

    // Add padding tree item
    UModelIndex paddingIndex = model->addItem(Types::Padding, Subtypes::DataPadding, UString("Non-UEFI data"), "", info, UByteArray(), padding, UByteArray(), true, parsingDataToUByteArray(pdata), index);
    msg(UString("parseVolumeNonUefiData: non-UEFI data found in volume's free space"), paddingIndex);

    if (vtfIndex >= 0) {
        // Get VTF file header
        UByteArray header = vtf.left(sizeof(EFI_FFS_FILE_HEADER));
        const EFI_FFS_FILE_HEADER* fileHeader = (const EFI_FFS_FILE_HEADER*)header.constData();
        if (pdata.ffsVersion == 3 && (fileHeader->Attributes & FFS_ATTRIB_LARGE_FILE)) {
            header = vtf.left(sizeof(EFI_FFS_FILE_HEADER2));
        }

        //Parse VTF file header
        UModelIndex fileIndex;
        USTATUS result = parseFileHeader(vtf, parentOffset + vtfIndex, index, fileIndex);
        if (result) {
            msg(UString("parseVolumeNonUefiData: VTF file header parsing failed with error ") + errorCodeToUString(result), index);
            
            // Add the rest as non-UEFI data too
            pdata.offset += vtfIndex;
            // Get info
            UString info = usprintf("Full size: %Xh (%u)", vtf.size(), vtf.size());

            // Add padding tree item
            UModelIndex paddingIndex = model->addItem(Types::Padding, Subtypes::DataPadding, UString("Non-UEFI data"), "", info, UByteArray(), vtf, UByteArray(), true, parsingDataToUByteArray(pdata), index);
            msg(("parseVolumeNonUefiData: non-UEFI data found in volume's free space"), paddingIndex);
        }
    }

    return U_SUCCESS;
}

USTATUS FfsParser::parseVolumeBody(const UModelIndex & index)
{
    // Sanity check
    if (!index.isValid())
        return U_INVALID_PARAMETER;

    // Get volume header size and body
    UByteArray volumeBody = model->body(index);
    UINT32 volumeHeaderSize = model->header(index).size();

    // Get parsing data
    PARSING_DATA pdata = parsingDataFromUModelIndex(index);
    UINT32 offset = pdata.offset;

    // Parse VSS NVRAM volumes with a dedicated function
    if (model->subtype(index) == Subtypes::NvramVolume)
        return parseNvramVolumeBody(index);

    if (pdata.ffsVersion != 2 && pdata.ffsVersion != 3) // Don't parse unknown volumes
        return U_SUCCESS;

    // Search for and parse all files
    UINT32 volumeBodySize = volumeBody.size();
    UINT32 fileOffset = 0;
    
    while (fileOffset < volumeBodySize) {
        UINT32 fileSize = getFileSize(volumeBody, fileOffset, pdata.ffsVersion);
        // Check file size 
        if (fileSize < sizeof(EFI_FFS_FILE_HEADER) || fileSize > volumeBodySize - fileOffset) {
            // Check that we are at the empty space
            UByteArray header = volumeBody.mid(fileOffset, sizeof(EFI_FFS_FILE_HEADER));
            if (header.count(pdata.emptyByte) == header.size()) { //Empty space
                // Check free space to be actually free
                UByteArray freeSpace = volumeBody.mid(fileOffset);
                if (freeSpace.count(pdata.emptyByte) != freeSpace.size()) {
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
                        UByteArray free = freeSpace.left(i);

                        // Get info
                        UString info = usprintf("Full size: %Xh (%u)", free.size(), free.size());

                        // Add free space item
                        model->addItem(Types::FreeSpace, 0, UString("Volume free space"), "", info, UByteArray(), free, UByteArray(), false, parsingDataToUByteArray(pdata), index);
                    }

                    // Parse non-UEFI data 
                    parseVolumeNonUefiData(freeSpace.mid(i), volumeHeaderSize + fileOffset + i, index);
                }
                else {
                    // Construct parsing data
                    pdata.offset = offset + volumeHeaderSize + fileOffset;

                    // Get info
                    UString info = usprintf("Full size: %Xh (%u)", freeSpace.size(), freeSpace.size());

                    // Add free space item
                    model->addItem(Types::FreeSpace, 0, UString("Volume free space"), "", info, UByteArray(), freeSpace, UByteArray(), false, parsingDataToUByteArray(pdata), index);
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
        UByteArray file = volumeBody.mid(fileOffset, fileSize);
        UByteArray header = file.left(sizeof(EFI_FFS_FILE_HEADER));
        const EFI_FFS_FILE_HEADER* fileHeader = (const EFI_FFS_FILE_HEADER*)header.constData();
        if (pdata.ffsVersion == 3 && (fileHeader->Attributes & FFS_ATTRIB_LARGE_FILE)) {
            header = file.left(sizeof(EFI_FFS_FILE_HEADER2));
        }

        //Parse current file's header
        UModelIndex fileIndex;
        USTATUS result = parseFileHeader(file, volumeHeaderSize + fileOffset, index, fileIndex);
        if (result)
            msg(UString("parseVolumeBody: file header parsing failed with error ") + errorCodeToUString(result), index);

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
        
        // Get current file parsing data
        PARSING_DATA currentPdata = parsingDataFromUModelIndex(current);
        UByteArray currentGuid((const char*)&currentPdata.file.guid, sizeof(EFI_GUID));

        // Check files after current for having an equal GUID
        for (int j = i + 1; j < model->rowCount(index); j++) {
            UModelIndex another = index.child(j, 0);

            // Skip non-file entries
            if (model->type(another) != Types::File)
                continue;
            
            // Get another file parsing data
            PARSING_DATA anotherPdata = parsingDataFromUModelIndex(another);
            UByteArray anotherGuid((const char*)&anotherPdata.file.guid, sizeof(EFI_GUID));

            // Check GUIDs for being equal
            if (currentGuid == anotherGuid) {
                msg(UString("parseVolumeBody: file with duplicate GUID ") + guidToUString(anotherPdata.file.guid), another);
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

USTATUS FfsParser::parseFileHeader(const UByteArray & file, const UINT32 parentOffset, const UModelIndex & parent, UModelIndex & index)
{
    // Sanity check
    if (file.isEmpty())
        return U_INVALID_PARAMETER;

    if ((UINT32)file.size() < sizeof(EFI_FFS_FILE_HEADER))
        return U_INVALID_FILE;

    // Get parent's parsing data
    PARSING_DATA pdata = parsingDataFromUModelIndex(parent);

    // Get file header
    UByteArray header = file.left(sizeof(EFI_FFS_FILE_HEADER));
    const EFI_FFS_FILE_HEADER* fileHeader = (const EFI_FFS_FILE_HEADER*)header.constData();
    if (pdata.ffsVersion == 3 && (fileHeader->Attributes & FFS_ATTRIB_LARGE_FILE)) {
        if ((UINT32)file.size() < sizeof(EFI_FFS_FILE_HEADER2))
            return U_INVALID_FILE;
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
    UByteArray tempHeader = header;
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
    UByteArray body = file.mid(header.size());

    // Check for file tail presence
    UByteArray tail;
    bool msgInvalidTailValue = false;
    if (pdata.volume.revision == 1 && (fileHeader->Attributes & FFS_ATTRIB_TAIL_PRESENT))
    {
        //Check file tail;
        UINT16 tailValue = *(UINT16*)body.right(sizeof(UINT16)).constData();
        if (fileHeader->IntegrityCheck.TailReference != (UINT16)~tailValue)
            msgInvalidTailValue = true;

        // Get tail and remove it from file body
        tail = body.right(sizeof(UINT16));
        body = body.left(body.size() - sizeof(UINT16));
    }

    // Get info
    UString name;
    UString info;
    if (fileHeader->Type != EFI_FV_FILETYPE_PAD)
        name = guidToUString(fileHeader->Name);
    else
        name = UString("Pad-file");

    info = UString("File GUID: ") + guidToUString(fileHeader->Name) +
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

    // Add file GUID to parsing data
    pdata.file.guid = fileHeader->Name;

    UString text;
    bool isVtf = false;
    // Check if the file is a Volume Top File
    if (UByteArray((const char*)&fileHeader->Name, sizeof(EFI_GUID)) == EFI_FFS_VOLUME_TOP_FILE_GUID) {
        // Mark it as the last VTF
        // This information will later be used to determine memory addresses of uncompressed image elements
        // Because the last byte of the last VFT is mapped to 0xFFFFFFFF physical memory address 
        isVtf = true;
        text = UString("Volume Top File");
    }

    // Construct parsing data
    bool fixed = (fileHeader->Attributes & FFS_ATTRIB_FIXED) != 0;
    pdata.offset += parentOffset;
    

    // Add tree item
    index = model->addItem(Types::File, fileHeader->Type, name, text, info, header, body, tail, fixed, parsingDataToUByteArray(pdata), parent);

    // Overwrite lastVtf, if needed
    if (isVtf) {
        lastVtf = index;
    }

    // Show messages
    if (msgUnalignedFile)
        msg(UString("parseFileHeader: unaligned file"), index);
    if (msgFileAlignmentIsGreaterThanVolumes)
        msg(usprintf("parseFileHeader: file alignment %Xh is greater than parent volume alignment %Xh", alignment, pdata.volume.alignment), index);
    if (msgInvalidHeaderChecksum)
        msg(UString("parseFileHeader: invalid header checksum"), index);
    if (msgInvalidDataChecksum)
        msg(UString("parseFileHeader: invalid data checksum"), index);
    if (msgInvalidTailValue)
        msg(UString("parseFileHeader: invalid tail value"), index);
    if (msgUnknownType)
        msg(usprintf("parseFileHeader: unknown file type %02Xh", fileHeader->Type), index);

    return U_SUCCESS;
}

UINT32 FfsParser::getSectionSize(const UByteArray & file, const UINT32 sectionOffset, const UINT8 ffsVersion)
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
        // Get data from parsing data
        PARSING_DATA pdata = parsingDataFromUModelIndex(index);

        // Parse NVAR store
        if (UByteArray((const char*)&pdata.file.guid, sizeof(EFI_GUID)) == NVRAM_NVAR_STORE_FILE_GUID)
            return parseNvarStore(index);

        return parseRawArea(index);
    }

    // Parse sections
    return parseSections(model->body(index), index);
}

USTATUS FfsParser::parsePadFileBody(const UModelIndex & index)
{
    // Sanity check
    if (!index.isValid())
        return U_INVALID_PARAMETER;

    // Get data from parsing data
    PARSING_DATA pdata = parsingDataFromUModelIndex(index);

    // Check if all bytes of the file are empty
    UByteArray body = model->body(index);
    if (body.size() == body.count(pdata.emptyByte))
        return U_SUCCESS;

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

        UByteArray free = body.left(i);

        // Get info
        UString info = usprintf("Full size: %Xh (%u)", free.size(), free.size());

        // Constuct parsing data
        pdata.offset += model->header(index).size();

        // Add tree item
        model->addItem(Types::FreeSpace, 0, UString("Free space"), UString(), info, UByteArray(), free, UByteArray(), false, parsingDataToUByteArray(pdata), index);
    }
    else 
        i = 0;

    // ... and all bytes after as a padding
    UByteArray padding = body.mid(i);

    // Get info
    UString info = usprintf("Full size: %Xh (%u)", padding.size(), padding.size());

    // Constuct parsing data
    pdata.offset += i;

    // Add tree item
    UModelIndex dataIndex = model->addItem(Types::Padding, Subtypes::DataPadding, UString("Non-UEFI data"), "", info, UByteArray(), padding, UByteArray(), true, parsingDataToUByteArray(pdata), index);

    // Show message
    msg(UString("parsePadFileBody: non-UEFI data found in pad-file"), dataIndex);

    // Rename the file
    model->setName(index, UString("Non-empty pad-file"));

    return U_SUCCESS;
}

USTATUS FfsParser::parseSections(const UByteArray & sections, const UModelIndex & index, const bool preparse)
{
    // Sanity check
    if (!index.isValid())
        return U_INVALID_PARAMETER;

    // Get data from parsing data
    PARSING_DATA pdata = parsingDataFromUModelIndex(index);

    // Search for and parse all sections
    UINT32 bodySize = sections.size();
    UINT32 headerSize = model->header(index).size();
    UINT32 sectionOffset = 0;

    USTATUS result = U_SUCCESS;
    while (sectionOffset < bodySize) {
        // Get section size
        UINT32 sectionSize = getSectionSize(sections, sectionOffset, pdata.ffsVersion);

        // Check section size
        if (sectionSize < sizeof(EFI_COMMON_SECTION_HEADER) || sectionSize > (bodySize - sectionOffset)) {
            // Add padding to fill the rest of sections
            UByteArray padding = sections.mid(sectionOffset);
            // Get info
            UString info = usprintf("Full size: %Xh (%u)", padding.size(), padding.size());

            // Constuct parsing data
            pdata.offset += headerSize + sectionOffset;

            // Final parsing
            if (!preparse) {
                // Add tree item
                UModelIndex dataIndex = model->addItem(Types::Padding, Subtypes::DataPadding, UString("Non-UEFI data"), "", info, UByteArray(), padding, UByteArray(), true, parsingDataToUByteArray(pdata), index);

                // Show message
                msg(UString("parseSections: non-UEFI data found in sections area"), dataIndex);
            }
            // Preparsing
            else {
                return U_INVALID_SECTION;
            }
            break; // Exit from parsing loop
        }

        // Parse section header
        UModelIndex sectionIndex;
        result = parseSectionHeader(sections.mid(sectionOffset, sectionSize), headerSize + sectionOffset, index, sectionIndex, preparse);
        if (result) {
            if (!preparse)
                msg(UString("parseSections: section header parsing failed with error ") + errorCodeToUString(result), index);
            else
                return U_INVALID_SECTION;
        }
        // Move to next section
        sectionOffset += sectionSize;
        sectionOffset = ALIGN4(sectionOffset);
    }

    //Parse bodies, will be skipped on preparse phase
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

USTATUS FfsParser::parseSectionHeader(const UByteArray & section, const UINT32 parentOffset, const UModelIndex & parent, UModelIndex & index, const bool preparse)
{
    // Check sanity
    if ((UINT32)section.size() < sizeof(EFI_COMMON_SECTION_HEADER))
        return U_INVALID_SECTION;

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
        USTATUS result = parseCommonSectionHeader(section, parentOffset, parent, index, preparse);
        msg(usprintf("parseSectionHeader: section with unknown type %02Xh", sectionHeader->Type), index);
        return result;
    }
}

USTATUS FfsParser::parseCommonSectionHeader(const UByteArray & section, const UINT32 parentOffset, const UModelIndex & parent, UModelIndex & index, const bool preparse)
{
    // Check sanity
    if ((UINT32)section.size() < sizeof(EFI_COMMON_SECTION_HEADER))
        return U_INVALID_SECTION;
    
    // Get data from parent's parsing data
    PARSING_DATA pdata = parsingDataFromUModelIndex(parent);

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
        if (pdata.ffsVersion == 3 && uint24ToUint32(sectionHeader->Size) == EFI_SECTION2_IS_USED)
            headerSize = sizeof(EFI_COMMON_SECTION_HEADER2);
        type = sectionHeader->Type;
    }

    // Check sanity again
    if ((UINT32)section.size() < headerSize)
        return U_INVALID_SECTION;
    
    UByteArray header = section.left(headerSize);
    UByteArray body = section.mid(headerSize);

    // Get info
    UString name = sectionTypeToUString(type) + UString(" section");
    UString info = usprintf("Type: %02Xh\nFull size: %Xh (%u)\nHeader size: %Xh (%u)\nBody size: %Xh (%u)",
        type,
        section.size(), section.size(),
        headerSize, headerSize,
        body.size(), body.size());

    // Construct parsing data
    pdata.offset += parentOffset;

    // Add tree item
    if (!preparse) {
        index = model->addItem(Types::Section, type, name, UString(), info, header, body, UByteArray(), true, parsingDataToUByteArray(pdata), parent);
    } 
    return U_SUCCESS;
}

USTATUS FfsParser::parseCompressedSectionHeader(const UByteArray & section, const UINT32 parentOffset, const UModelIndex & parent, UModelIndex & index, const bool preparse)
{
    // Check sanity
    if ((UINT32)section.size() < sizeof(EFI_COMMON_SECTION_HEADER))
        return U_INVALID_SECTION;

    // Get data from parent's parsing data
    PARSING_DATA pdata = parsingDataFromUModelIndex(parent);

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
    else if (pdata.ffsVersion == 3 && uint24ToUint32(sectionHeader->Size) == EFI_SECTION2_IS_USED) { // Check for extended header section
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
    if ((UINT32)section.size() < headerSize)
        return U_INVALID_SECTION;
    
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

    // Construct parsing data
    pdata.offset += parentOffset;
    pdata.section.compressed.compressionType = compressionType;
    pdata.section.compressed.uncompressedSize = uncompressedLength;

    // Add tree item
    if (!preparse) {
        index = model->addItem(Types::Section, sectionHeader->Type, name, UString(), info, header, body, UByteArray(), true, parsingDataToUByteArray(pdata), parent);
    }
    return U_SUCCESS;
}

USTATUS FfsParser::parseGuidedSectionHeader(const UByteArray & section, const UINT32 parentOffset, const UModelIndex & parent, UModelIndex & index, const bool preparse)
{
    // Check sanity
    if ((UINT32)section.size() < sizeof(EFI_COMMON_SECTION_HEADER))
        return U_INVALID_SECTION;

    // Get data from parent's parsing data
    PARSING_DATA pdata = parsingDataFromUModelIndex(parent);

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
    else if (pdata.ffsVersion == 3 && uint24ToUint32(sectionHeader->Size) == EFI_SECTION2_IS_USED) { // Check for extended header section
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
    if (baGuid == EFI_GUIDED_SECTION_CRC32) {
        if ((attributes & EFI_GUIDED_SECTION_AUTH_STATUS_VALID) == 0) { // Check that AuthStatusValid attribute is set on compressed GUIDed sections
            msgNoAuthStatusAttribute = true;
        }

        if ((UINT32)section.size() < headerSize + sizeof(UINT32))
            return U_INVALID_SECTION;

        UINT32 crc = *(UINT32*)(section.constData() + headerSize);
        additionalInfo += UString("\nChecksum type: CRC32");
        // Calculate CRC32 of section data
        UINT32 calculated = crc32(0, (const UINT8*)section.constData() + dataOffset, section.size() - dataOffset);
        if (crc == calculated) {
            additionalInfo += usprintf("\nChecksum: %08Xh, valid", crc);
        }
        else {
            additionalInfo += usprintf("\nChecksum: %08Xh, invalid, should be %08Xh", crc, calculated);
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

    UByteArray header = section.left(dataOffset);
    UByteArray body = section.mid(dataOffset);

    // Get info
    UString name = guidToUString(guid);
    UString info = UString("Section GUID: ") + name +
        usprintf("\nType: %02Xh\nFull size: %Xh (%u)\nHeader size: %Xh (%u)\nBody size: %Xh (%u)\nData offset: %Xh\nAttributes: %04Xh",
        sectionHeader->Type, 
        section.size(), section.size(),
        header.size(), header.size(),
        body.size(), body.size(),
        dataOffset,
        attributes);

    // Append additional info
    info += additionalInfo;

    // Construct parsing data
    pdata.offset += parentOffset;
    pdata.section.guidDefined.guid = guid;

    // Add tree item
    if (!preparse) {
        index = model->addItem(Types::Section, sectionHeader->Type, name, UString(), info, header, body, UByteArray(), false, parsingDataToUByteArray(pdata), parent);

        // Show messages
        if (msgSignedSectionFound)
            msg(UString("parseGuidedSectionHeader: section signature may become invalid after any modification"), index);
        if (msgNoAuthStatusAttribute)
            msg(UString("parseGuidedSectionHeader: CRC32 GUIDed section without AuthStatusValid attribute"), index);
        if (msgNoProcessingRequiredAttributeCompressed)
            msg(UString("parseGuidedSectionHeader: compressed GUIDed section without ProcessingRequired attribute"), index);
        if (msgNoProcessingRequiredAttributeSigned)
            msg(UString("parseGuidedSectionHeader: signed GUIDed section without ProcessingRequired attribute"), index);
        if (msgInvalidCrc)
            msg(UString("parseGuidedSectionHeader: GUID defined section with invalid CRC32"), index);
        if (msgUnknownCertType)
            msg(UString("parseGuidedSectionHeader: signed GUIDed section with unknown type"), index);
        if (msgUnknownCertSubtype)
            msg(UString("parseGuidedSectionHeader: signed GUIDed section with unknown subtype"), index);
    }

    return U_SUCCESS;
}

USTATUS FfsParser::parseFreeformGuidedSectionHeader(const UByteArray & section, const UINT32 parentOffset, const UModelIndex & parent, UModelIndex & index, const bool preparse)
{
    // Check sanity
    if ((UINT32)section.size() < sizeof(EFI_COMMON_SECTION_HEADER))
        return U_INVALID_SECTION;

    // Get data from parent's parsing data
    PARSING_DATA pdata = parsingDataFromUModelIndex(parent);

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
    else if (pdata.ffsVersion == 3 && uint24ToUint32(sectionHeader->Size) == EFI_SECTION2_IS_USED) { // Check for extended header section
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
        + guidToUString(guid);

    // Construct parsing data
    pdata.offset += parentOffset;
    pdata.section.freeformSubtypeGuid.guid = guid;

    // Add tree item
    if (!preparse) {
        index = model->addItem(Types::Section, type, name, UString(), info, header, body, UByteArray(), false, parsingDataToUByteArray(pdata), parent);

        // Rename section
        model->setName(index, guidToUString(guid));
    }
    return U_SUCCESS;
}

USTATUS FfsParser::parseVersionSectionHeader(const UByteArray & section, const UINT32 parentOffset, const UModelIndex & parent, UModelIndex & index, const bool preparse)
{
    // Check sanity
    if ((UINT32)section.size() < sizeof(EFI_COMMON_SECTION_HEADER))
        return U_INVALID_SECTION;

    // Get data from parent's parsing data
    PARSING_DATA pdata = parsingDataFromUModelIndex(parent);

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
    else if (pdata.ffsVersion == 3 && uint24ToUint32(sectionHeader->Size) == EFI_SECTION2_IS_USED) { // Check for extended header section
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

    // Construct parsing data
    pdata.offset += parentOffset;

    // Add tree item
    if (!preparse) {
        index = model->addItem(Types::Section, type, name, UString(), info, header, body, UByteArray(), false, parsingDataToUByteArray(pdata), parent);
    }
    return U_SUCCESS;
}

USTATUS FfsParser::parsePostcodeSectionHeader(const UByteArray & section, const UINT32 parentOffset, const UModelIndex & parent, UModelIndex & index, const bool preparse)
{
    // Check sanity
    if ((UINT32)section.size() < sizeof(EFI_COMMON_SECTION_HEADER))
        return U_INVALID_SECTION;

    // Get data from parent's parsing data
    PARSING_DATA pdata = parsingDataFromUModelIndex(parent);

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
    else if (pdata.ffsVersion == 3 && uint24ToUint32(sectionHeader->Size) == EFI_SECTION2_IS_USED) { // Check for extended header section
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

    // Construct parsing data
    pdata.offset += parentOffset;

    // Add tree item
    if (!preparse) {
        index = model->addItem(Types::Section, sectionHeader->Type, name, UString(), info, header, body, UByteArray(), false, parsingDataToUByteArray(pdata), parent);
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
    case EFI_SECTION_DISPOSABLE:            return parseSections(model->body(index), index);
    // Leaf
    case EFI_SECTION_FREEFORM_SUBTYPE_GUID: return parseRawArea(index);
    case EFI_SECTION_VERSION:               return parseVersionSectionBody(index);
    case EFI_SECTION_DXE_DEPEX:
    case EFI_SECTION_PEI_DEPEX:
    case EFI_SECTION_SMM_DEPEX:             return parseDepexSectionBody(index);
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

    // Get data from parsing data
    PARSING_DATA pdata = parsingDataFromUModelIndex(index);
    UINT8 algorithm = pdata.section.compressed.compressionType;

    // Decompress section
    UByteArray decompressed;
    UByteArray efiDecompressed;
    USTATUS result = decompress(model->body(index), algorithm, decompressed, efiDecompressed);
    if (result) {
        msg(UString("parseCompressedSectionBody: decompression failed with error ") + errorCodeToUString(result), index);
        return U_SUCCESS;
    }
    
    // Check reported uncompressed size
    if (pdata.section.compressed.uncompressedSize != (UINT32)decompressed.size()) {
        msg(usprintf("parseCompressedSectionBody: decompressed size stored in header %Xh (%u) differs from actual %Xh (%u)",
            pdata.section.compressed.uncompressedSize,
            pdata.section.compressed.uncompressedSize,
            decompressed.size(),
            decompressed.size()), index);
        model->addInfo(index, usprintf("\nActual decompressed size: %Xh (%u)", decompressed.size(), decompressed.size()));
    }

    // Check for undecided compression algorithm, this is a special case
    if (algorithm == COMPRESSION_ALGORITHM_UNDECIDED) {
        // Try preparse of sections decompressed with Tiano algorithm
        if (U_SUCCESS == parseSections(decompressed, index, true)) {
            algorithm = COMPRESSION_ALGORITHM_TIANO;
        }
        // Try preparse of sections decompressed with EFI 1.1 algorithm
        else if (U_SUCCESS == parseSections(efiDecompressed, index, true)) {
            algorithm = COMPRESSION_ALGORITHM_EFI11;
            decompressed = efiDecompressed;
        }
        else {
            msg(UString("parseCompressedSectionBody: can't guess the correct decompression algorithm, both preparse steps are failed"), index);
        }
    }

    // Add info
    model->addInfo(index, UString("\nCompression algorithm: ") + compressionTypeToUString(algorithm));

    // Update data
    pdata.section.compressed.algorithm = algorithm;
    if (algorithm != COMPRESSION_ALGORITHM_NONE)
        model->setCompressed(index, true);
    model->setParsingData(index, parsingDataToUByteArray(pdata));
    
    // Parse decompressed data
    return parseSections(decompressed, index);
}

USTATUS FfsParser::parseGuidedSectionBody(const UModelIndex & index)
{
    // Sanity check
    if (!index.isValid())
        return U_INVALID_PARAMETER;

    // Get data from parsing data
    PARSING_DATA pdata = parsingDataFromUModelIndex(index);
    EFI_GUID guid = pdata.section.guidDefined.guid;

    // Check if section requires processing
    UByteArray processed = model->body(index);
    UByteArray efiDecompressed;
    UString info;
    bool parseCurrentSection = true;
    UINT8 algorithm = COMPRESSION_ALGORITHM_NONE;
    // Tiano compressed section
    if (UByteArray((const char*)&guid, sizeof(EFI_GUID)) == EFI_GUIDED_SECTION_TIANO) {
        algorithm = EFI_STANDARD_COMPRESSION;
        USTATUS result = decompress(model->body(index), algorithm, processed, efiDecompressed);
        if (result) {
            parseCurrentSection = false;
            msg(UString("parseGuidedSectionBody: decompression failed with error ") + errorCodeToUString(result), index);
            return U_SUCCESS;
        }

        // Check for undecided compression algorithm, this is a special case
        if (algorithm == COMPRESSION_ALGORITHM_UNDECIDED) {
            // Try preparse of sections decompressed with Tiano algorithm
            if (U_SUCCESS == parseSections(processed, index, true)) {
                algorithm = COMPRESSION_ALGORITHM_TIANO;
            }
            // Try preparse of sections decompressed with EFI 1.1 algorithm
            else if (U_SUCCESS == parseSections(efiDecompressed, index, true)) {
                algorithm = COMPRESSION_ALGORITHM_EFI11;
                processed = efiDecompressed;
            }
            else {
                msg(UString("parseGuidedSectionBody: can't guess the correct decompression algorithm, both preparse steps are failed"), index);
				parseCurrentSection = false;
            }
        }
        
        info += UString("\nCompression algorithm: ") + compressionTypeToUString(algorithm);
        info += usprintf("\nDecompressed size: %Xh (%u)", processed.size(), processed.size());
    }
    // LZMA compressed section
    else if (UByteArray((const char*)&guid, sizeof(EFI_GUID)) == EFI_GUIDED_SECTION_LZMA) {
        algorithm = EFI_CUSTOMIZED_COMPRESSION;
        USTATUS result = decompress(model->body(index), algorithm, processed, efiDecompressed);
        if (result) {
            parseCurrentSection = false;
            msg(UString("parseGuidedSectionBody: decompression failed with error ") + errorCodeToUString(result), index);
            return U_SUCCESS;
        }

        if (algorithm == COMPRESSION_ALGORITHM_LZMA) {
            info += UString("\nCompression algorithm: LZMA");
            info += usprintf("\nDecompressed size: %Xh (%u)", processed.size(), processed.size());
        }
        else {
            info += UString("\nCompression algorithm: unknown");
			parseCurrentSection = false;
		}
    }

    // Add info
    model->addInfo(index, info);

    // Update data
    if (algorithm != COMPRESSION_ALGORITHM_NONE)
        model->setCompressed(index, true);
    model->setParsingData(index, parsingDataToUByteArray(pdata));

    if (!parseCurrentSection) {
        msg(UString("parseGuidedSectionBody: GUID defined section can not be processed"), index);
        return U_SUCCESS;
    }

    return parseSections(processed, index);
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
        msg(UString("parseDepexSectionBody: DEPEX section too short"), index);
        return U_DEPEX_PARSE_FAILED;
    }

    const EFI_GUID * guid;
    const UINT8* current = (const UINT8*)body.constData();

    // Special cases of first opcode
    switch (*current) {
    case EFI_DEP_BEFORE:
        if (body.size() != 2 * EFI_DEP_OPCODE_SIZE + sizeof(EFI_GUID)) {
            msg(UString("parseDepexSectionBody: DEPEX section too long for a section starting with BEFORE opcode"), index);
            return U_SUCCESS;
        }
        guid = (const EFI_GUID*)(current + EFI_DEP_OPCODE_SIZE);
        parsed += UString("\nBEFORE ") + guidToUString(*guid);
        current += EFI_DEP_OPCODE_SIZE + sizeof(EFI_GUID);
        if (*current != EFI_DEP_END){
            msg(UString("parseDepexSectionBody: DEPEX section ends with non-END opcode"), index);
            return U_SUCCESS;
        }
        return U_SUCCESS;
    case EFI_DEP_AFTER:
        if (body.size() != 2 * EFI_DEP_OPCODE_SIZE + sizeof(EFI_GUID)){
            msg(UString("parseDepexSectionBody: DEPEX section too long for a section starting with AFTER opcode"), index);
            return U_SUCCESS;
        }
        guid = (const EFI_GUID*)(current + EFI_DEP_OPCODE_SIZE);
        parsed += UString("\nAFTER ") + guidToUString(*guid);
        current += EFI_DEP_OPCODE_SIZE + sizeof(EFI_GUID);
        if (*current != EFI_DEP_END) {
            msg(UString("parseDepexSectionBody: DEPEX section ends with non-END opcode"), index);
            return U_SUCCESS;
        }
        return U_SUCCESS;
    case EFI_DEP_SOR:
        if (body.size() <= 2 * EFI_DEP_OPCODE_SIZE) {
            msg(UString("parseDepexSectionBody: DEPEX section too short for a section starting with SOR opcode"), index);
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
            msg(UString("parseDepexSectionBody: misplaced BEFORE opcode"), index);
            return U_SUCCESS;
        }
        case EFI_DEP_AFTER: {
            msg(UString("parseDepexSectionBody: misplaced AFTER opcode"), index);
            return U_SUCCESS;
        }
        case EFI_DEP_SOR: {
            msg(UString("parseDepexSectionBody: misplaced SOR opcode"), index);
            return U_SUCCESS;
        }
        case EFI_DEP_PUSH:
            // Check that the rest of depex has correct size
            if ((UINT32)body.size() - (UINT32)(current - (const UINT8*)body.constData()) <= EFI_DEP_OPCODE_SIZE + sizeof(EFI_GUID)) {
                parsed.clear();
                msg(UString("parseDepexSectionBody: remains of DEPEX section too short for PUSH opcode"), index);
                return U_SUCCESS;
            }
            guid = (const EFI_GUID*)(current + EFI_DEP_OPCODE_SIZE);
            parsed += UString("\nPUSH ") + guidToUString(*guid);
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
                msg(UString("parseDepexSectionBody: DEPEX section ends with non-END opcode"), index);
            }
            break;
        default:
            msg(UString("parseDepexSectionBody: unknown opcode"), index);
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
        msg(UString("parseAprioriRawSection: apriori file has size is not a multiple of 16"));
    }
    parsed.clear();
    UINT32 count = body.size() / sizeof(EFI_GUID);
    if (count > 0) {
        for (UINT32 i = 0; i < count; i++) {
            const EFI_GUID* guid = (const EFI_GUID*)body.constData() + i;
            parsed += UString("\n") + guidToUString(*guid);
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

    // Get parent file parsing data
    PARSING_DATA pdata = parsingDataFromUModelIndex(parentFile);
    UByteArray parentFileGuid((const char*)&pdata.file.guid, sizeof(EFI_GUID));

    if (parentFileGuid == EFI_PEI_APRIORI_FILE_GUID) { // PEI apriori file
        // Parse apriori file list
        UString str;
        USTATUS result = parseAprioriRawSection(model->body(index), str);
        if (!result && !str.isEmpty())
            model->addInfo(index, UString("\nFile list:") + str);

        // Set parent file text
        model->setText(parentFile, UString("PEI apriori file"));

        return U_SUCCESS;
    }
    else if (parentFileGuid == EFI_DXE_APRIORI_FILE_GUID) { // DXE apriori file
        // Parse apriori file list
        UString str;
        USTATUS result = parseAprioriRawSection(model->body(index), str);
        if (!result && !str.isEmpty())
            model->addInfo(index, UString("\nFile list:") + str);

        // Set parent file text
        model->setText(parentFile, UString("DXE apriori file"));

        return U_SUCCESS;
    }
    else if (parentFileGuid == NVRAM_NVAR_EXTERNAL_DEFAULTS_FILE_GUID) {
        // Parse NVAR area
        parseNvarStore(index);

        // Set parent file text
        model->setText(parentFile, UString("NVRAM external defaults"));
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
        msg(UString("parsePeImageSectionBody: section body size is smaller than DOS header size"), index);
        return U_SUCCESS;
    }

    UString info;
    const EFI_IMAGE_DOS_HEADER* dosHeader = (const EFI_IMAGE_DOS_HEADER*)body.constData();
    if (dosHeader->e_magic != EFI_IMAGE_DOS_SIGNATURE) {
        info += usprintf("\nDOS signature: %04Xh, invalid", dosHeader->e_magic);
        msg(UString("parsePeImageSectionBody: PE32 image with invalid DOS signature"), index);
        model->addInfo(index, info);
        return U_SUCCESS;
    }

    const EFI_IMAGE_PE_HEADER* peHeader = (EFI_IMAGE_PE_HEADER*)(body.constData() + dosHeader->e_lfanew);
    if (body.size() < (UINT8*)peHeader - (UINT8*)dosHeader) {
        info += UString("\nDOS header: invalid");
        msg(UString("parsePeImageSectionBody: PE32 image with invalid DOS header"), index);
        model->addInfo(index, info);
        return U_SUCCESS;
    }

    if (peHeader->Signature != EFI_IMAGE_PE_SIGNATURE) {
        info += usprintf("\nPE signature: %08Xh, invalid", peHeader->Signature);
        msg(UString("parsePeImageSectionBody: PE32 image with invalid PE signature"), index);
        model->addInfo(index, info);
        return U_SUCCESS;
    }

    const EFI_IMAGE_FILE_HEADER* imageFileHeader = (const EFI_IMAGE_FILE_HEADER*)(peHeader + 1);
    if (body.size() < (UINT8*)imageFileHeader - (UINT8*)dosHeader) {
        info += UString("\nPE header: invalid");
        msg(UString("parsePeImageSectionBody: PE32 image with invalid PE header"), index);
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
        msg(UString("parsePeImageSectionBody: PE32 image with invalid PE optional header"), index);
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
        msg(UString("parsePeImageSectionBody: PE32 image with invalid optional PE header signature"), index);
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
        msg(("parsePeImageSectionBody: section body size is smaller than TE header size"), index);
        return U_SUCCESS;
    }

    UString info;
    const EFI_IMAGE_TE_HEADER* teHeader = (const EFI_IMAGE_TE_HEADER*)body.constData();
    if (teHeader->Signature != EFI_IMAGE_TE_SIGNATURE) {
        info += usprintf("\nSignature: %04Xh, invalid", teHeader->Signature);
        msg(UString("parseTeImageSectionBody: TE image with invalid TE signature"), index);
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

    // Get data from parsing data
    PARSING_DATA pdata = parsingDataFromUModelIndex(index);
    pdata.section.teImage.imageBase = (UINT32)teHeader->ImageBase;
    pdata.section.teImage.adjustedImageBase = (UINT32)(teHeader->ImageBase + teHeader->StrippedSize - sizeof(EFI_IMAGE_TE_HEADER));
    
    // Update parsing data
    model->setParsingData(index, parsingDataToUByteArray(pdata));

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
        msg(UString("performSecondPass: the last VTF appears inside compressed item, the image may be damaged"), lastVtf);
        return U_SUCCESS;
    }

    // Get parsing data for the last VTF
    PARSING_DATA pdata = parsingDataFromUModelIndex(lastVtf);

    // Calculate address difference
    const UINT32 vtfSize = model->header(lastVtf).size() + model->body(lastVtf).size() + model->tail(lastVtf).size();
    const UINT32 diff = 0xFFFFFFFFUL - pdata.offset - vtfSize + 1;

    // Apply address information to index and all it's child items
    addMemoryAddressesRecursive(index, diff);

    return U_SUCCESS;
}

USTATUS FfsParser::addMemoryAddressesRecursive(const UModelIndex & index, const UINT32 diff)
{
    // Sanity check
    if (!index.isValid())
        return U_SUCCESS;
    
    // Set address value for non-compressed data
    if (!model->compressed(index)) {
        // Get parsing data for the current item
        PARSING_DATA pdata = parsingDataFromUModelIndex(index);

        // Check address sanity
        if ((const UINT64)diff + pdata.offset <= 0xFFFFFFFFUL)  {
            // Update info
            pdata.address = diff + pdata.offset;
            UINT32 headerSize = model->header(index).size();
            if (headerSize) {
                model->addInfo(index, usprintf("\nHeader memory address: %08Xh", pdata.address));
                model->addInfo(index, usprintf("\nData memory address: %08Xh", pdata.address + headerSize));
            }
            else {
                model->addInfo(index, usprintf("\nMemory address: %08Xh", pdata.address));
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
                    msg(UString("addMemoryAddressesRecursive: image base is neither original nor adjusted, it's likely a part of backup PEI volume or DXE volume, but can also be damaged"), index);
                    pdata.section.teImage.revision = 0;
                }
            }

            // Set modified parsing data
            model->setParsingData(index, parsingDataToUByteArray(pdata));
        }
    }

    // Process child items
    for (int i = 0; i < model->rowCount(index); i++) {
        addMemoryAddressesRecursive(index.child(i, 0), diff);
    }

    return U_SUCCESS;
}

USTATUS FfsParser::addOffsetsRecursive(const UModelIndex & index)
{
    // Sanity check
    if (!index.isValid())
        return U_INVALID_PARAMETER;
    
    // Get parsing data for the current item
    PARSING_DATA pdata = parsingDataFromUModelIndex(index);

    // Add current offset if the element is not compressed
    // or it's compressed, but it's parent isn't
    if ((!model->compressed(index)) || (index.parent().isValid() && !model->compressed(index.parent()))) {
        model->addInfo(index, usprintf("Offset: %Xh\n", pdata.offset), false);
    }

    //TODO: show FIT file fixed attribute correctly
    model->addInfo(index, usprintf("\nCompressed: %s", model->compressed(index) ? "Yes" : "No"));
    model->addInfo(index, usprintf("\nFixed: %s", model->fixed(index) ? "Yes" : "No"));

    // Process child items
    for (int i = 0; i < model->rowCount(index); i++) {
        addOffsetsRecursive(index.child(i, 0));
    }

    return U_SUCCESS;
}

USTATUS FfsParser::parseNvarStore(const UModelIndex & index)
{
    // Sanity check
    if (!index.isValid())
        return U_INVALID_PARAMETER;

    // Get parsing data for the current item
    PARSING_DATA pdata = parsingDataFromUModelIndex(index);
    UINT32 parentOffset = pdata.offset + model->header(index).size();

    // Get item data
    const UByteArray data = model->body(index);

    // Rename parent file
    model->setText(model->findParentOfType(index, Types::File), UString("NVAR store"));

    UINT32 offset = 0;
    UINT32 guidsInStore = 0;
    const UINT8 emptyByte = pdata.emptyByte;
    // Parse all entries
    while (1) {
        bool msgUnknownExtDataFormat = false;
        bool msgExtHeaderTooLong = false;
        bool msgExtDataTooShort = false;

        bool isInvalid = false;
        bool isInvalidLink = false;
        bool isDataOnly = false;
        bool hasExtendedHeader = false;
        bool hasChecksum = false;
        bool hasTimestampAndHash = false;
        bool hasGuidIndex = false;

        UINT32 guidIndex = 0;
        UINT8  storedChecksum = 0;
        UINT8  calculatedChecksum = 0;
        UINT32 extendedHeaderSize = 0;
        UINT8  extendedAttributes = 0;
        UINT64 timestamp = 0;
        UByteArray hash;

        UINT8 subtype = Subtypes::FullNvarEntry;
        UString name;
        UString text;
        UByteArray header;
        UByteArray body;
        UByteArray tail;
        
        UINT32 guidAreaSize = guidsInStore * sizeof(EFI_GUID);
        UINT32 unparsedSize = (UINT32)data.size() - offset - guidAreaSize;

        // Get entry header
        const NVAR_ENTRY_HEADER* entryHeader = (const NVAR_ENTRY_HEADER*)(data.constData() + offset);
        
        // Check header size and signature
        if (unparsedSize < sizeof(NVAR_ENTRY_HEADER) ||
            entryHeader->Signature != NVRAM_NVAR_ENTRY_SIGNATURE ||
            unparsedSize < entryHeader->Size) {

            // Check if the data left is a free space or a padding
            UByteArray padding = data.mid(offset, unparsedSize);
            UINT8 type;
            
            if ((UINT32)padding.count(emptyByte) == unparsedSize) {
                // It's a free space
                name = ("Free space");
                type = Types::FreeSpace;
                subtype = 0;
            }
            else {
                // Nothing is parsed yet, but the file is not empty 
                if (!offset) {
                    msg(UString("parseNvarStore: file can't be parsed as NVAR variables store"), index);
                    return U_SUCCESS;
                }

                // It's a padding
                name = UString("Padding");
                type = Types::Padding;
                subtype = getPaddingType(padding);
            }
            // Get info
            UString info = usprintf("Full size: %Xh (%u)", padding.size(), padding.size());
            // Construct parsing data
            pdata.offset = parentOffset + offset;
            // Add tree item
            model->addItem(type, subtype, name, UString(), info, UByteArray(), padding, UByteArray(), false, parsingDataToUByteArray(pdata), index);

            // Add GUID store area
            UByteArray guidArea = data.right(guidAreaSize);
            // Get info
            name = UString("GUID store area");
            info = usprintf("Full size: %Xh (%u)\nGUIDs in store: %u",
                guidArea.size(), guidArea.size(),
                guidsInStore);
            // Construct parsing data
            pdata.offset = parentOffset + offset + padding.size();
            // Add tree item
            model->addItem(Types::Padding, getPaddingType(guidArea), name, UString(), info, UByteArray(), guidArea, UByteArray(), false, parsingDataToUByteArray(pdata), index);

            return U_SUCCESS;
        }
        
        // Contruct generic header and body
        header = data.mid(offset, sizeof(NVAR_ENTRY_HEADER));
        body = data.mid(offset + sizeof(NVAR_ENTRY_HEADER), entryHeader->Size - sizeof(NVAR_ENTRY_HEADER));

        UINT32 lastVariableFlag = pdata.emptyByte ? 0xFFFFFF : 0;
        
        // Set default next to predefined last value
        pdata.nvar.next = lastVariableFlag;

        // Entry is marked as invalid
        if ((entryHeader->Attributes & NVRAM_NVAR_ENTRY_VALID) == 0) { // Valid attribute is not set
            isInvalid = true;
            // Do not parse further
            goto parsing_done;
        }

        // Add next node information to parsing data
        if (entryHeader->Next != lastVariableFlag) {
            subtype = Subtypes::LinkNvarEntry;
            pdata.nvar.next = entryHeader->Next;
        }
        
        // Entry with extended header
        if (entryHeader->Attributes & NVRAM_NVAR_ENTRY_EXT_HEADER) {
            hasExtendedHeader = true;
            msgUnknownExtDataFormat = true;

            extendedHeaderSize = *(UINT16*)(body.constData() + body.size() - sizeof(UINT16));
            if (extendedHeaderSize > (UINT32)body.size()) {
                msgExtHeaderTooLong = true;
                isInvalid = true;
                // Do not parse further
                goto parsing_done;
            }

            extendedAttributes = *(UINT8*)(body.constData() + body.size() - extendedHeaderSize);

            // Variable with checksum
            if (extendedAttributes & NVRAM_NVAR_ENTRY_EXT_CHECKSUM) {
                // Get stored checksum
                storedChecksum = *(UINT8*)(body.constData() + body.size() - sizeof(UINT16) - sizeof(UINT8));

                // Recalculate checksum for the variable
                calculatedChecksum = 0;
                // Include entry data
                UINT8* start = (UINT8*)(entryHeader + 1);
                for (UINT8* p = start; p < start + entryHeader->Size - sizeof(NVAR_ENTRY_HEADER); p++) {
                    calculatedChecksum += *p;
                }
                // Include entry size and flags
                start = (UINT8*)&entryHeader->Size;
                for (UINT8*p = start; p < start + sizeof(UINT16); p++) {
                    calculatedChecksum += *p;
                }
                // Include entry attributes
                calculatedChecksum += entryHeader->Attributes;
                
                hasChecksum = true;
                msgUnknownExtDataFormat = false;
            }

            tail = body.mid(body.size() - extendedHeaderSize);
            body = body.left(body.size() - extendedHeaderSize);

            // Entry with authenticated write (for SecureBoot)
            if (entryHeader->Attributes & NVRAM_NVAR_ENTRY_AUTH_WRITE) {
                if ((UINT32)tail.size() < sizeof(UINT64) + SHA256_HASH_SIZE) {
                    msgExtDataTooShort = true;
                    isInvalid = true;
                    // Do not parse further
                    goto parsing_done;
                }

                timestamp = *(UINT64*)(tail.constData() + sizeof(UINT8));
                hash = tail.mid(sizeof(UINT64) + sizeof(UINT8), SHA256_HASH_SIZE);
                hasTimestampAndHash = true;
                msgUnknownExtDataFormat = false;
            }
        }

        // Entry is data-only (nameless and GUIDless entry or link)
        if (entryHeader->Attributes & NVRAM_NVAR_ENTRY_DATA_ONLY) { // Data-only attribute is set
            isInvalidLink = true;
            UModelIndex nvarIndex;
            // Search prevously added entries for a link to this variable //TODO:replace with linked lists
            for (int i = 0; i < model->rowCount(index); i++) {
                nvarIndex = index.child(i, 0);
                PARSING_DATA nvarPdata = parsingDataFromUModelIndex(nvarIndex);
                if (nvarPdata.nvar.isValid && nvarPdata.nvar.next + nvarPdata.offset - parentOffset == offset) { // Previous link is present and valid
                    isInvalidLink = false;
                    break;
                }
            }
            // Check if the link is valid
            if (!isInvalidLink) {
                // Use the name and text of the previous link
                name = model->name(nvarIndex);
                text = model->text(nvarIndex);

                if (entryHeader->Next == lastVariableFlag)
                    subtype = Subtypes::DataNvarEntry;
            }

            isDataOnly = true;
            // Do not parse further
            goto parsing_done;
        }

        // Get entry name
        {
            UINT32 nameOffset = (entryHeader->Attributes & NVRAM_NVAR_ENTRY_GUID) ? sizeof(EFI_GUID) : sizeof(UINT8); // GUID can be stored with the variable or in a separate store, so there will only be an index of it
            CHAR8* namePtr = (CHAR8*)(entryHeader + 1) + nameOffset;
            UINT32 nameSize = 0;
            if (entryHeader->Attributes & NVRAM_NVAR_ENTRY_ASCII_NAME) { // Name is stored as ASCII string of CHAR8s
                text = UString(namePtr);
                nameSize = text.length() + 1;
            }
            else { // Name is stored as UCS2 string of CHAR16s
                text = UString::fromUtf16((CHAR16*)namePtr);
                nameSize = (text.length() + 1) * 2;
            }

            // Get entry GUID
            if (entryHeader->Attributes & NVRAM_NVAR_ENTRY_GUID) { // GUID is strored in the variable itself
                name = guidToUString(*(EFI_GUID*)(entryHeader + 1));
            }
            // GUID is stored in GUID list at the end of the store
            else {
                guidIndex = *(UINT8*)(entryHeader + 1);
                if (guidsInStore < guidIndex + 1)
                    guidsInStore = guidIndex + 1;

                // The list begins at the end of the store and goes backwards
                const EFI_GUID* guidPtr = (const EFI_GUID*)(data.constData() + data.size()) - 1 - guidIndex;
                name = guidToUString(*guidPtr);
                hasGuidIndex = true;
            }

            // Include name and GUID into the header and remove them from body
            header = data.mid(offset, sizeof(NVAR_ENTRY_HEADER) + nameOffset + nameSize);
            body = body.mid(nameOffset + nameSize);
        }
parsing_done:
        UString info;

        // Rename invalid entries according to their types
        pdata.nvar.isValid = TRUE;
        if (isInvalid) {
            name = UString("Invalid");
            subtype = Subtypes::InvalidNvarEntry;
            pdata.nvar.isValid = FALSE;
        }
        else if (isInvalidLink) {
            name = UString("Invalid link");
            subtype = Subtypes::InvalidLinkNvarEntry;
            pdata.nvar.isValid = FALSE;
        }
        else // Add GUID info for valid entries
            info += UString("Variable GUID: ") + name + UString("\n");
        
        // Add GUID index information
        if (hasGuidIndex)
            info += usprintf("GUID index: %u\n", guidIndex);

        // Add header, body and extended data info
        info += usprintf("Full size: %Xh (%u)\nHeader size: %Xh (%u)\nBody size: %Xh (%u)",
            entryHeader->Size, entryHeader->Size,
            header.size(), header.size(),
            body.size(), body.size());
        
        // Add attributes info
        info += usprintf("\nAttributes: %02Xh", entryHeader->Attributes);
        // Translate attributes to text
        if (entryHeader->Attributes && entryHeader->Attributes != 0xFF)
            info += UString(" (") + nvarAttributesToUString(entryHeader->Attributes) + UString(")");
        
        // Add next node info
        if (!isInvalid && entryHeader->Next != lastVariableFlag)
            info += usprintf("\nNext node at offset: %Xh", parentOffset + offset + entryHeader->Next);

        // Add extended header info
        if (hasExtendedHeader) {
            info += usprintf("\nExtended header size: %Xh (%u)\nExtended attributes: %Xh (",
                extendedHeaderSize, extendedHeaderSize,
                extendedAttributes) + nvarExtendedAttributesToUString(extendedAttributes) + UString(")");

            // Checksum
            if (hasChecksum)
                info += usprintf("\nChecksum: %02Xh", storedChecksum) +
                    (calculatedChecksum ? usprintf(", invalid, should be %02Xh", 0x100 - calculatedChecksum) : UString(", valid"));
            // Authentication data
            if (hasTimestampAndHash) {
                info += usprintf("\nTimestamp: %" PRIX64 "h\nHash: ",
                    timestamp) + UString(hash.toHex().constData());
            }
        }
        
        // Add correct offset to parsing data
        pdata.offset = parentOffset + offset;

        // Add tree item
        UModelIndex varIndex = model->addItem(Types::NvarEntry, subtype, name, text, info, header, body, tail, false, parsingDataToUByteArray(pdata), index);

        // Show messages
        if (msgUnknownExtDataFormat) msg(UString("parseNvarStore: unknown extended data format"), varIndex);
        if (msgExtHeaderTooLong)     msg(usprintf("parseNvarStore: extended header size (%Xh) is greater than body size (%Xh)",
                                         extendedHeaderSize, body.size()), varIndex);
        if (msgExtDataTooShort)      msg(usprintf("parseNvarStore: extended header size (%Xh) is too small for timestamp and hash",
                                         tail.size()), varIndex);

        // Try parsing the entry data as NVAR storage if it begins with NVAR signature
        if ((subtype == Subtypes::DataNvarEntry || subtype == Subtypes::FullNvarEntry) 
            && *(const UINT32*)body.constData() == NVRAM_NVAR_ENTRY_SIGNATURE)
            parseNvarStore(varIndex);
        
        // Move to next exntry
        offset += entryHeader->Size;
    }
    
    return U_SUCCESS;
}

USTATUS FfsParser::parseNvramVolumeBody(const UModelIndex & index)
{
    // Sanity check
    if (!index.isValid())
        return U_INVALID_PARAMETER;

    // Get parsing data
    PARSING_DATA pdata = parsingDataFromUModelIndex(index);
    UINT32 parentOffset = pdata.offset + model->header(index).size();

    // Get item data
    UByteArray data = model->body(index);

    // Search for first store
    USTATUS result;
    UINT32 prevStoreOffset;
    result = findNextStore(index, data, parentOffset, 0, prevStoreOffset);
    if (result)
        return result;

    // First store is not at the beginning of volume body
    UString name;
    UString info;
    if (prevStoreOffset > 0) {
        // Get info
        UByteArray padding = data.left(prevStoreOffset);
        name = UString("Padding");
        info = usprintf("Full size: %Xh (%u)", padding.size(), padding.size());

        // Construct parsing data
        pdata.offset = parentOffset;

        // Add tree item
        model->addItem(Types::Padding, getPaddingType(padding), name, UString(), info, UByteArray(), padding, UByteArray(), true, parsingDataToUByteArray(pdata), index);
    }

    // Search for and parse all stores
    UINT32 storeOffset = prevStoreOffset;
    UINT32 prevStoreSize = 0;

    while (!result)
    {
        // Padding between stores
        if (storeOffset > prevStoreOffset + prevStoreSize) {
            UINT32 paddingOffset = prevStoreOffset + prevStoreSize;
            UINT32 paddingSize = storeOffset - paddingOffset;
            UByteArray padding = data.mid(paddingOffset, paddingSize);

            // Get info
            name = UString("Padding");
            info = usprintf("Full size: %Xh (%u)", padding.size(), padding.size());

            // Construct parsing data
            pdata.offset = parentOffset + paddingOffset;

            // Add tree item
            model->addItem(Types::Padding, getPaddingType(padding), name, UString(), info, UByteArray(), padding, UByteArray(), true, parsingDataToUByteArray(pdata), index);
        }

        // Get store size
        UINT32 storeSize = 0;
        result = getStoreSize(data, storeOffset, storeSize);
        if (result) {
            msg(UString("parseNvramVolumeBody: getStoreSize failed with error ") + errorCodeToUString(result), index);
            return result;
        }

        // Check that current store is fully present in input
        if (storeSize > (UINT32)data.size() || storeOffset + storeSize > (UINT32)data.size()) {
            // Mark the rest as padding and finish parsing
            UByteArray padding = data.mid(storeOffset);

            // Get info
            name = UString("Padding");
            info = usprintf("Full size: %Xh (%u)", padding.size(), padding.size());

            // Construct parsing data
            pdata.offset = parentOffset + storeOffset;

            // Add tree item
            UModelIndex paddingIndex = model->addItem(Types::Padding, getPaddingType(padding), name, UString(), info, UByteArray(), padding, UByteArray(), true, parsingDataToUByteArray(pdata), index);
            msg(UString("parseNvramVolumeBody: one of stores inside overlaps the end of data"), paddingIndex);

            // Update variables
            prevStoreOffset = storeOffset;
            prevStoreSize = padding.size();
            break;
        }

        UByteArray store = data.mid(storeOffset, storeSize);
        // Parse current store header
        UModelIndex storeIndex;
        result = parseStoreHeader(store, parentOffset + storeOffset, index, storeIndex);
        if (result)
            msg(UString("parseNvramVolumeBody: store header parsing failed with error ") + errorCodeToUString(result), index);

        // Go to next store
        prevStoreOffset = storeOffset;
        prevStoreSize = storeSize;
        result = findNextStore(index, data, parentOffset, storeOffset + prevStoreSize, storeOffset);
    }

    // Padding/free space at the end
    storeOffset = prevStoreOffset + prevStoreSize;
    if ((UINT32)data.size() > storeOffset) {
        UByteArray padding = data.mid(storeOffset);
        UINT8 type;
        UINT8 subtype;
        if (padding.count(pdata.emptyByte) == padding.size()) {
            // It's a free space
            name = UString("Free space");
            type = Types::FreeSpace;
            subtype = 0;
        }
        else {
            // Nothing is parsed yet, but the file is not empty 
            if (!storeOffset) {
                msg(UString("parseNvramVolumeBody: can't be parsed as NVRAM volume"), index);
                return U_SUCCESS;
            }

            // It's a padding
            name = UString("Padding");
            type = Types::Padding;
            subtype = getPaddingType(padding);
        }

        // Add info
        info = usprintf("Full size: %Xh (%u)", padding.size(), padding.size());

        // Construct parsing data
        pdata.offset = parentOffset + storeOffset;
        
        // Add tree item
        model->addItem(type, subtype, name, UString(), info, UByteArray(), padding, UByteArray(), true, parsingDataToUByteArray(pdata), index);
    }

    // Parse bodies
    for (int i = 0; i < model->rowCount(index); i++) {
        UModelIndex current = index.child(i, 0);
        switch (model->type(current)) {
        case Types::VssStore:
        case Types::FdcStore:       parseVssStoreBody(current);  break;
        case Types::FsysStore:      parseFsysStoreBody(current); break;
        case Types::EvsaStore:      parseEvsaStoreBody(current); break;
        case Types::FlashMapStore:  parseFlashMapBody(current);  break;
        }
    }
    
    return U_SUCCESS;
}

USTATUS FfsParser::findNextStore(const UModelIndex & index, const UByteArray & volume, const UINT32 parentOffset, const UINT32 storeOffset, UINT32 & nextStoreOffset)
{
    UINT32 dataSize = volume.size();

    if (dataSize < sizeof(UINT32))
        return U_STORES_NOT_FOUND;

    UINT32 offset = storeOffset;
    for (; offset < dataSize - sizeof(UINT32); offset++) {
        const UINT32* currentPos = (const UINT32*)(volume.constData() + offset);
        if (*currentPos == NVRAM_VSS_STORE_SIGNATURE || *currentPos == NVRAM_APPLE_SVS_STORE_SIGNATURE) { //$VSS or $SVS signatures found, perform checks
            const VSS_VARIABLE_STORE_HEADER* vssHeader = (const VSS_VARIABLE_STORE_HEADER*)currentPos;
            if (vssHeader->Format != NVRAM_VSS_VARIABLE_STORE_FORMATTED) {
                msg(usprintf("findNextStore: VSS store candidate at offset %Xh skipped, has invalid format %02Xh", parentOffset + offset, vssHeader->Format), index);
                continue;
            }
            if (vssHeader->Size == 0 || vssHeader->Size == 0xFFFFFFFF) {
                msg(usprintf("findNextStore: VSS store candidate at offset %Xh skipped, has invalid size %Xh", parentOffset + offset, vssHeader->Size), index);
                continue;
            }
            // All checks passed, store found
            break;
        }
        else if (*currentPos == NVRAM_FDC_VOLUME_SIGNATURE) { //FDC signature found
            const FDC_VOLUME_HEADER* fdcHeader = (const FDC_VOLUME_HEADER*)currentPos;
            if (fdcHeader->Size == 0 || fdcHeader->Size == 0xFFFFFFFF) {
                msg(usprintf("findNextStore: FDC store candidate at offset %Xh skipped, has invalid size %Xh", parentOffset + offset, fdcHeader->Size), index);
                continue;
            }
            // All checks passed, store found
            break;
        }
        else if (*currentPos == NVRAM_APPLE_FSYS_STORE_SIGNATURE || *currentPos == NVRAM_APPLE_GAID_STORE_SIGNATURE) { //Fsys or Gaid signature found
            const APPLE_FSYS_STORE_HEADER* fsysHeader = (const APPLE_FSYS_STORE_HEADER*)currentPos;
            if (fsysHeader->Size == 0 || fsysHeader->Size == 0xFFFF) {
                msg(usprintf("findNextStore: Fsys store candidate at offset %Xh skipped, has invalid size %Xh", parentOffset + offset, fsysHeader->Size), index);
                continue;
            }
            // All checks passed, store found
            break;
        }
        else if (*currentPos == NVRAM_EVSA_STORE_SIGNATURE) { //EVSA signature found
            if (offset < sizeof(UINT32))
                continue;

            const EVSA_STORE_ENTRY* evsaHeader = (const EVSA_STORE_ENTRY*)(currentPos - 1);
            if (evsaHeader->Header.Type != NVRAM_EVSA_ENTRY_TYPE_STORE) {
                msg(usprintf("findNextStore: EVSA store candidate at offset %Xh skipped, has invalid type %02Xh", parentOffset + offset - 4, evsaHeader->Header.Type), index);
                continue;
            }
            if (evsaHeader->StoreSize == 0 || evsaHeader->StoreSize == 0xFFFFFFFF) {
                msg(usprintf("findNextStore: EVSA store candidate at offset %Xh skipped, has invalid size %Xh", parentOffset + offset, evsaHeader->StoreSize), index);
                continue;
            }
            // All checks passed, store found
            offset -= sizeof(UINT32);
            break;
        }
        else if (*currentPos == NVRAM_MAIN_STORE_VOLUME_GUID_DATA1 || *currentPos == EDKII_WORKING_BLOCK_SIGNATURE_GUID_DATA1) { //Possible FTW block signature found
            UByteArray guid = UByteArray(volume.constData() + offset, sizeof(EFI_GUID));
            if (guid != NVRAM_MAIN_STORE_VOLUME_GUID && guid != EDKII_WORKING_BLOCK_SIGNATURE_GUID) // Check the whole signature
                continue;

            // Detect header variant based on WriteQueueSize
            const EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER32* ftwHeader = (const EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER32*)currentPos;
            if (ftwHeader->WriteQueueSize % 0x10 == 0x04) { // Header with 32 bit WriteQueueSize
                if (ftwHeader->WriteQueueSize == 0 || ftwHeader->WriteQueueSize == 0xFFFFFFFF) {
                    msg(usprintf("findNextStore: FTW block candidate at offset %Xh skipped, has invalid body size %Xh", parentOffset + offset, ftwHeader->WriteQueueSize), index);
                    continue;
                }
            }
            else if (ftwHeader->WriteQueueSize % 0x10 == 0x00) { // Header with 64 bit WriteQueueSize
                const EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER64* ftw64Header = (const EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER64*)currentPos;
                if (ftw64Header->WriteQueueSize == 0 || ftw64Header->WriteQueueSize >= 0xFFFFFFFF) {
                    msg(usprintf("findNextStore: FTW block candidate at offset %Xh skipped, has invalid body size %Xh", parentOffset + offset, ftw64Header->WriteQueueSize), index);
                    continue;
                }
            }
            else // Unknown header
                continue;

            // All checks passed, store found
            break;
        }
        else if (*currentPos == NVRAM_PHOENIX_FLASH_MAP_SIGNATURE_PART1) {// Phoenix SCT flash map
            UByteArray signature = UByteArray(volume.constData() + offset, NVRAM_PHOENIX_FLASH_MAP_SIGNATURE_LENGTH);
            if (signature != NVRAM_PHOENIX_FLASH_MAP_SIGNATURE) // Check the whole signature
                continue;

            // All checks passed, store found
            break;
        }
        else if (*currentPos == NVRAM_PHOENIX_CMDB_HEADER_SIGNATURE) { // Phoenix SCT CMDB store
            const PHOENIX_CMDB_HEADER* cmdbHeader = (const PHOENIX_CMDB_HEADER*)currentPos;

            // Check size
            if (cmdbHeader->HeaderSize != sizeof(PHOENIX_CMDB_HEADER))
                continue;

            // All checks passed, store found
            break;
        }
        else if (*currentPos == INTEL_MICROCODE_HEADER_VERSION) {// Intel microcode
            if (!INTEL_MICROCODE_HEADER_SIZES_VALID(currentPos)) // Check header sizes
                continue;

            // Check reserved bytes
            const INTEL_MICROCODE_HEADER* ucodeHeader = (const INTEL_MICROCODE_HEADER*)currentPos;
            bool reservedBytesValid = true;
            for (UINT32 i = 0; i < sizeof(ucodeHeader->Reserved); i++)
                if (ucodeHeader->Reserved[i] != INTEL_MICROCODE_HEADER_RESERVED_BYTE) {
                    reservedBytesValid = false;
                    break;
                }
            if (!reservedBytesValid)
                continue;

            // All checks passed, store found
            break;
        }
        else if (*currentPos == OEM_ACTIVATION_PUBKEY_MAGIC) { // SLIC pubkey
            if (offset < 4 * sizeof(UINT32))
                continue;

            const OEM_ACTIVATION_PUBKEY* pubkeyHeader = (const OEM_ACTIVATION_PUBKEY*)(currentPos - 4);
            // Check type
            if (pubkeyHeader->Type != OEM_ACTIVATION_PUBKEY_TYPE)
                continue;

            // All checks passed, store found
            offset -= 4 * sizeof(UINT32);
            break;
        }
        else if (*currentPos == OEM_ACTIVATION_MARKER_WINDOWS_FLAG_PART1) { // SLIC marker
            if (offset >= dataSize - sizeof(UINT64) || 
                *(const UINT64*)currentPos != OEM_ACTIVATION_MARKER_WINDOWS_FLAG ||
                offset < 26) // Check full windows flag and structure size
                continue;
            
            const OEM_ACTIVATION_MARKER* markerHeader = (const OEM_ACTIVATION_MARKER*)(volume.constData() + offset - 26);
            // Check reserved bytes
            bool reservedBytesValid = true;
            for (UINT32 i = 0; i < sizeof(markerHeader->Reserved); i++)
                if (markerHeader->Reserved[i] != OEM_ACTIVATION_MARKER_RESERVED_BYTE) {
                    reservedBytesValid = false;
                    break;
                }
            if (!reservedBytesValid)
                continue;

            // All checks passed, store found
            offset -= 26;
            break;
        }
    }
    // No more stores found
    if (offset >= dataSize - sizeof(UINT32))
        return U_STORES_NOT_FOUND;

    nextStoreOffset = offset;

    return U_SUCCESS;
}

USTATUS FfsParser::getStoreSize(const UByteArray & data, const UINT32 storeOffset, UINT32 & storeSize)
{
    const UINT32* signature = (const UINT32*)(data.constData() + storeOffset);
    if (*signature == NVRAM_VSS_STORE_SIGNATURE || *signature == NVRAM_APPLE_SVS_STORE_SIGNATURE) {
        const VSS_VARIABLE_STORE_HEADER* vssHeader = (const VSS_VARIABLE_STORE_HEADER*)signature;
        storeSize = vssHeader->Size;
    }
    else if (*signature == NVRAM_FDC_VOLUME_SIGNATURE) {
        const FDC_VOLUME_HEADER* fdcHeader = (const FDC_VOLUME_HEADER*)signature;
        storeSize = fdcHeader->Size;
    }
    else if (*signature == NVRAM_APPLE_FSYS_STORE_SIGNATURE || *signature == NVRAM_APPLE_GAID_STORE_SIGNATURE) {
        const APPLE_FSYS_STORE_HEADER* fsysHeader = (const APPLE_FSYS_STORE_HEADER*)signature;
        storeSize = fsysHeader->Size;
    }
    else if (*(signature + 1) == NVRAM_EVSA_STORE_SIGNATURE) {
        const EVSA_STORE_ENTRY* evsaHeader = (const EVSA_STORE_ENTRY*)signature;
        storeSize = evsaHeader->StoreSize;
    }
    else if (*signature == NVRAM_MAIN_STORE_VOLUME_GUID_DATA1 || *signature == EDKII_WORKING_BLOCK_SIGNATURE_GUID_DATA1) {
        const EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER32* ftwHeader = (const EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER32*)signature;
        if (ftwHeader->WriteQueueSize % 0x10 == 0x04) { // Header with 32 bit WriteQueueSize
            storeSize = sizeof(EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER32) + ftwHeader->WriteQueueSize;
        }
        else { //  Header with 64 bit WriteQueueSize
            const EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER64* ftw64Header = (const EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER64*)signature;
            storeSize = sizeof(EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER64) + (UINT32)ftw64Header->WriteQueueSize;
        }
    }
    else if (*signature == NVRAM_PHOENIX_FLASH_MAP_SIGNATURE_PART1) { // Phoenix SCT flash map
        const PHOENIX_FLASH_MAP_HEADER* flashMapHeader = (const PHOENIX_FLASH_MAP_HEADER*)signature;
        storeSize = sizeof(PHOENIX_FLASH_MAP_HEADER) + sizeof(PHOENIX_FLASH_MAP_ENTRY) * flashMapHeader->NumEntries;
    }
    else if (*signature == NVRAM_PHOENIX_CMDB_HEADER_SIGNATURE) { // Phoenix SCT CMDB store
        storeSize = NVRAM_PHOENIX_CMDB_SIZE; // It's a predefined max size, no need to calculate
    }
    else if (*(signature + 4) == OEM_ACTIVATION_PUBKEY_MAGIC) { // SLIC pubkey
        const OEM_ACTIVATION_PUBKEY* pubkeyHeader = (const OEM_ACTIVATION_PUBKEY*)signature;
        storeSize = pubkeyHeader->Size;
    }
    else if (*(const UINT64*)(data.constData() + storeOffset + 26) == OEM_ACTIVATION_MARKER_WINDOWS_FLAG) { // SLIC marker
        const OEM_ACTIVATION_MARKER* markerHeader = (const OEM_ACTIVATION_MARKER*)signature;
        storeSize = markerHeader->Size;
    }
    else if (*signature == INTEL_MICROCODE_HEADER_VERSION) { // Intel microcode, must be checked after SLIC marker because of the same *signature values
        const INTEL_MICROCODE_HEADER* ucodeHeader = (const INTEL_MICROCODE_HEADER*)signature;
        storeSize = ucodeHeader->TotalSize;
    }
    return U_SUCCESS;
}

USTATUS FfsParser::parseVssStoreHeader(const UByteArray & store, const UINT32 parentOffset, const UModelIndex & parent, UModelIndex & index)
{
    const UINT32 dataSize = (const UINT32)store.size();
    
    // Check store size
    if (dataSize < sizeof(VSS_VARIABLE_STORE_HEADER)) {
        msg(UString("parseVssStoreHeader: volume body is too small even for VSS store header"), parent);
        return U_SUCCESS;
    }

    // Get VSS store header
    const VSS_VARIABLE_STORE_HEADER* vssStoreHeader = (const VSS_VARIABLE_STORE_HEADER*)store.constData();

    // Check store size
    if (dataSize < vssStoreHeader->Size) {
        msg(usprintf("parseVssStoreHeader: VSS store size %Xh (%u) is greater than volume body size %Xh (%u)",
            vssStoreHeader->Size, vssStoreHeader->Size,
            dataSize, dataSize), parent);
        return U_SUCCESS;
    }

    // Get parsing data
    PARSING_DATA pdata = parsingDataFromUModelIndex(parent);

    // Construct header and body
    UByteArray header = store.left(sizeof(VSS_VARIABLE_STORE_HEADER));
    UByteArray body = store.mid(sizeof(VSS_VARIABLE_STORE_HEADER), vssStoreHeader->Size - sizeof(VSS_VARIABLE_STORE_HEADER));

    // Add info
    bool isSvsStore = (vssStoreHeader->Signature == NVRAM_APPLE_SVS_STORE_SIGNATURE);
    UString name = isSvsStore ? UString("SVS store") : UString("VSS store");
    UString info = usprintf("Signature: %s\nFull size: %Xh (%u)\nHeader size: %Xh (%u)\nBody size: %Xh (%u)\nFormat: %02Xh\nState: %02Xh\nUnknown: %04Xh",
        isSvsStore ? "$SVS" : "$VSS",
        vssStoreHeader->Size, vssStoreHeader->Size,
        header.size(), header.size(),
        body.size(), body.size(),
        vssStoreHeader->Format,
        vssStoreHeader->State, 
        vssStoreHeader->Unknown);

    // Add correct offset
    pdata.offset = parentOffset;

    // Add tree item
    index = model->addItem(Types::VssStore, 0, name, UString(), info, header, body, UByteArray(), true, parsingDataToUByteArray(pdata), parent);

    return U_SUCCESS;
}

USTATUS FfsParser::parseFtwStoreHeader(const UByteArray & store, const UINT32 parentOffset, const UModelIndex & parent, UModelIndex & index)
{
    const UINT32 dataSize = (const UINT32)store.size();

    // Check store size
    if (dataSize < sizeof(EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER64)) {
        msg(UString("parseFtwStoreHeader: volume body is too small even for FTW store header"), parent);
        return U_SUCCESS;
    }

    // Get FTW block headers
    const EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER32* ftw32BlockHeader = (const EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER32*)store.constData();
    const EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER64* ftw64BlockHeader = (const EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER64*)store.constData();

    // Check store size
    UINT32 ftwBlockSize;
    bool has32bitHeader;
    if (ftw32BlockHeader->WriteQueueSize % 0x10 == 0x04) { // Header with 32 bit WriteQueueSize
        ftwBlockSize = sizeof(EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER32) + ftw32BlockHeader->WriteQueueSize;
        has32bitHeader = true;
    }
    else { // Header with 64 bit WriteQueueSize
        ftwBlockSize = sizeof(EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER64) + (UINT32)ftw64BlockHeader->WriteQueueSize;
        has32bitHeader = false;
    }
    if (dataSize < ftwBlockSize) {
        msg(usprintf("parseFtwStoreHeader: FTW store size %Xh (%u) is greater than volume body size %Xh (%u)",
            ftwBlockSize, ftwBlockSize,
            dataSize, dataSize), parent);
        return U_SUCCESS;
    }

    // Get parsing data
    PARSING_DATA pdata = parsingDataFromUModelIndex(parent);

    // Construct header and body
    UINT32 headerSize = has32bitHeader ? sizeof(EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER32) : sizeof(EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER64);
    UByteArray header = store.left(headerSize);
    UByteArray body = store.mid(headerSize, ftwBlockSize - headerSize);

    // Check block header checksum
    UByteArray crcHeader = header;
    EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER32* crcFtwBlockHeader = (EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER32*)header.data();
    crcFtwBlockHeader->Crc = pdata.emptyByte ? 0xFFFFFFFF : 0;
    crcFtwBlockHeader->State = pdata.emptyByte ? 0xFF : 0;
    UINT32 calculatedCrc = crc32(0, (const UINT8*)crcFtwBlockHeader, headerSize);

    // Add info
    UString name("FTW store");
    UString info = UString("Signature: ") + guidToUString(ftw32BlockHeader->Signature) +
        usprintf("\nFull size: %Xh (%u)\nHeader size: %Xh (%u)\nBody size: %Xh (%u)\nState: %02Xh\nHeader CRC32: %08Xh",
        ftwBlockSize, ftwBlockSize,
        headerSize, headerSize,
        body.size(), body.size(),
        ftw32BlockHeader->State,
        ftw32BlockHeader->Crc) +
        (ftw32BlockHeader->Crc != calculatedCrc ? usprintf(", invalid, should be %08Xh", calculatedCrc) : UString(", valid"));

    // Add correct offset
    pdata.offset = parentOffset;

    // Add tree item
    index = model->addItem(Types::FtwStore, 0, name, UString(), info, header, body, UByteArray(), true, parsingDataToUByteArray(pdata), parent);

    return U_SUCCESS;
}

USTATUS FfsParser::parseFdcStoreHeader(const UByteArray & store, const UINT32 parentOffset, const UModelIndex & parent, UModelIndex & index)
{
    const UINT32 dataSize = (const UINT32)store.size();

    // Check store size
    if (dataSize < sizeof(FDC_VOLUME_HEADER)) {
        msg(UString("parseFdcStoreHeader: volume body is too small even for FDC store header"), parent);
        return U_SUCCESS;
    }

    // Get Fdc store header
    const FDC_VOLUME_HEADER* fdcStoreHeader = (const FDC_VOLUME_HEADER*)store.constData();

    // Check store size
    if (dataSize < fdcStoreHeader->Size) {
        msg(usprintf("parseFdcStoreHeader: FDC store size %Xh (%u) is greater than volume body size %Xh (%u)",
            fdcStoreHeader->Size, fdcStoreHeader->Size,
            dataSize, dataSize), parent);
        return U_SUCCESS;
    }

    // Determine internal volume header size
    const EFI_FIRMWARE_VOLUME_HEADER* volumeHeader = (const EFI_FIRMWARE_VOLUME_HEADER*)(fdcStoreHeader + 1);
    UINT32 headerSize;
    if (volumeHeader->Revision > 1 && volumeHeader->ExtHeaderOffset) {
        const EFI_FIRMWARE_VOLUME_EXT_HEADER* extendedHeader = (const EFI_FIRMWARE_VOLUME_EXT_HEADER*)((const UINT8*)volumeHeader + volumeHeader->ExtHeaderOffset);
        headerSize = volumeHeader->ExtHeaderOffset + extendedHeader->ExtHeaderSize;
    }
    else
        headerSize = volumeHeader->HeaderLength;

    // Extended header end can be unaligned
    headerSize = ALIGN8(headerSize);

    // Add VSS store header
    headerSize += sizeof(VSS_VARIABLE_STORE_HEADER);

    // Add FDC header 
    headerSize += sizeof(FDC_VOLUME_HEADER);

    // Check sanity of combined header size
    if (dataSize < headerSize) {
        msg(usprintf("parseFdcStoreHeader: FDC store header size %Xh (%u) is greater than volume body size %Xh (%u)",
            fdcStoreHeader->Size,fdcStoreHeader->Size,
            dataSize, dataSize), parent);
        return U_SUCCESS;
    }

    // Get parsing data
    PARSING_DATA pdata = parsingDataFromUModelIndex(parent);

    // Construct header and body
    UByteArray header = store.left(headerSize);
    UByteArray body = store.mid(headerSize, fdcStoreHeader->Size - headerSize);

    // Add info
    UString name("FDC store");
    UString info = usprintf("Signature: _FDC\nFull size: %Xh (%u)\nHeader size: %Xh (%u)\nBody size: %Xh (%u)",
        fdcStoreHeader->Size, fdcStoreHeader->Size,
        header.size(), header.size(),
        body.size(), body.size());

    // TODO: add internal headers info

    // Add correct offset
    pdata.offset = parentOffset;

    // Add tree item
    index = model->addItem(Types::FdcStore, 0, name, UString(), info, header, body, UByteArray(), true, parsingDataToUByteArray(pdata), parent);

    return U_SUCCESS;
}

USTATUS FfsParser::parseFsysStoreHeader(const UByteArray & store, const UINT32 parentOffset, const UModelIndex & parent, UModelIndex & index)
{
    const UINT32 dataSize = (const UINT32)store.size();

    // Check store size
    if (dataSize < sizeof(APPLE_FSYS_STORE_HEADER)) {
        msg(UString("parseFsysStoreHeader: volume body is too small even for Fsys store header"), parent);
        return U_SUCCESS;
    }

    // Get Fsys store header
    const APPLE_FSYS_STORE_HEADER* fsysStoreHeader = (const APPLE_FSYS_STORE_HEADER*)store.constData();

    // Check store size
    if (dataSize < fsysStoreHeader->Size) {
        msg(usprintf("parseFsysStoreHeader: Fsys store size %Xh (%u) is greater than volume body size %Xh (%u)",
            fsysStoreHeader->Size, fsysStoreHeader->Size,
            dataSize, dataSize), parent);
        return U_SUCCESS;
    }

    // Get parsing data
    PARSING_DATA pdata = parsingDataFromUModelIndex(parent);

    // Construct header and body
    UByteArray header = store.left(sizeof(APPLE_FSYS_STORE_HEADER));
    UByteArray body = store.mid(sizeof(APPLE_FSYS_STORE_HEADER), fsysStoreHeader->Size - sizeof(APPLE_FSYS_STORE_HEADER) - sizeof(UINT32));

    // Check store checksum
    UINT32 storedCrc = *(UINT32*)store.right(sizeof(UINT32)).constData();
    UINT32 calculatedCrc = crc32(0, (const UINT8*)store.constData(), (const UINT32)store.size() - sizeof(UINT32));

    // Add info
    bool isGaidStore = (fsysStoreHeader->Signature == NVRAM_APPLE_GAID_STORE_SIGNATURE);
    UString name = isGaidStore ? UString("Gaid store") : UString("Fsys store");
    UString info = usprintf("Signature: %s\nFull size: %Xh (%u)\nHeader size: %Xh (%u)\nBody size: %Xh (%u)\nUnknown0: %02Xh\nUnknown1: %08Xh\nCRC32: %08Xh",
        isGaidStore ? "Gaid" : "Fsys",
        fsysStoreHeader->Size, fsysStoreHeader->Size,
        header.size(), header.size(),
        body.size(), body.size(),
        fsysStoreHeader->Unknown0, 
        fsysStoreHeader->Unknown1)
        + (storedCrc != calculatedCrc ? usprintf(", invalid, should be %08Xh", calculatedCrc) : UString(", valid"));

    // Add correct offset
    pdata.offset = parentOffset;

    // Add tree item
    index = model->addItem(Types::FsysStore, 0, name, UString(), info, header, body, UByteArray(), true, parsingDataToUByteArray(pdata), parent);

    return U_SUCCESS;
}

USTATUS FfsParser::parseEvsaStoreHeader(const UByteArray & store, const UINT32 parentOffset, const UModelIndex & parent, UModelIndex & index)
{
    const UINT32 dataSize = (const UINT32)store.size();

    // Check dataSize
    if (dataSize < sizeof(EVSA_STORE_ENTRY)) {
        msg(UString("parseEvsaStoreHeader: volume body is too small even for EVSA store header"), parent);
        return U_SUCCESS;
    }

    // Get EVSA store header
    const EVSA_STORE_ENTRY* evsaStoreHeader = (const EVSA_STORE_ENTRY*)store.constData();

    // Check store size
    if (dataSize < evsaStoreHeader->StoreSize) {
        msg(usprintf("parseEvsaStoreHeader: EVSA store size %Xh (%u) is greater than volume body size %Xh (%u)",
            evsaStoreHeader->StoreSize, evsaStoreHeader->StoreSize,
            dataSize, dataSize), parent);
        return U_SUCCESS;
    }

    // Get parsing data
    PARSING_DATA pdata = parsingDataFromUModelIndex(parent);

    // Construct header and body
    UByteArray header = store.left(evsaStoreHeader->Header.Size);
    UByteArray body = store.mid(evsaStoreHeader->Header.Size, evsaStoreHeader->StoreSize - evsaStoreHeader->Header.Size);

    // Recalculate checksum
    UINT8 calculated = calculateChecksum8(((const UINT8*)evsaStoreHeader) + 2, evsaStoreHeader->Header.Size - 2);

    // Add info
    UString name("EVSA store");
    UString info = usprintf("Signature: EVSA\nFull size: %Xh (%u)\nHeader size: %Xh (%u)\nBody size: %Xh (%u)\nType: %02Xh\nAttributes: %08Xh\nChecksum: %02Xh",
        evsaStoreHeader->StoreSize, evsaStoreHeader->StoreSize,
        header.size(), header.size(),
        body.size(), body.size(),
        evsaStoreHeader->Header.Type,
        evsaStoreHeader->Attributes,
        evsaStoreHeader->Header.Checksum) +
        (evsaStoreHeader->Header.Checksum != calculated ? usprintf("%, invalid, should be %02Xh", calculated) : UString(", valid"));

    // Add correct offset
    pdata.offset = parentOffset;

    // Add tree item
    index = model->addItem(Types::EvsaStore, 0, name, UString(), info, header, body, UByteArray(), true, parsingDataToUByteArray(pdata), parent);

    return U_SUCCESS;
}

USTATUS FfsParser::parseFlashMapStoreHeader(const UByteArray & store, const UINT32 parentOffset, const UModelIndex & parent, UModelIndex & index)
{
    const UINT32 dataSize = (const UINT32)store.size();

    // Check data size
    if (dataSize < sizeof(PHOENIX_FLASH_MAP_HEADER)) {
        msg(UString("parseFlashMapStoreHeader: volume body is too small even for FlashMap block header"), parent);
        return U_SUCCESS;
    }

    // Get FlashMap block header
    const PHOENIX_FLASH_MAP_HEADER* flashMapHeader = (const PHOENIX_FLASH_MAP_HEADER*)store.constData();

    // Check store size
    UINT32 flashMapSize = sizeof(PHOENIX_FLASH_MAP_HEADER) + flashMapHeader->NumEntries * sizeof(PHOENIX_FLASH_MAP_ENTRY);
    if (dataSize < flashMapSize) {
        msg(usprintf("parseFlashMapStoreHeader: FlashMap block size %Xh (%u) is greater than volume body size %Xh (%u)",
            flashMapSize, flashMapSize,
            dataSize, dataSize), parent);
        return U_SUCCESS;
    }

    // Get parsing data
    PARSING_DATA pdata = parsingDataFromUModelIndex(parent);

    // Construct header and body
    UByteArray header = store.left(sizeof(PHOENIX_FLASH_MAP_HEADER));
    UByteArray body = store.mid(sizeof(PHOENIX_FLASH_MAP_HEADER), flashMapSize - sizeof(PHOENIX_FLASH_MAP_HEADER));

    // Add info
    UString name("Phoenix SCT flash map");
    UString info = usprintf("Signature: _FLASH_MAP\nFull size: %Xh (%u)\nHeader size: %Xh (%u)\nBody size: %Xh (%u)\nNumber of entries: %u",
        flashMapSize, flashMapSize,
        header.size(), header.size(),
        body.size(), body.size(),
        flashMapHeader->NumEntries);

    // Add correct offset
    pdata.offset = parentOffset;

    // Add tree item
    index = model->addItem(Types::FlashMapStore, 0, name, UString(), info, header, body, UByteArray(), true, parsingDataToUByteArray(pdata), parent);

    return U_SUCCESS;
}

USTATUS FfsParser::parseCmdbStoreHeader(const UByteArray & store, const UINT32 parentOffset, const UModelIndex & parent, UModelIndex & index)
{
    const UINT32 dataSize = (const UINT32)store.size();
    
    // Check store size
    if (dataSize < sizeof(PHOENIX_CMDB_HEADER)) {
        msg(UString("parseCmdbStoreHeader: volume body is too small even for CMDB store header"), parent);
        return U_SUCCESS;
    }

    UINT32 cmdbSize = NVRAM_PHOENIX_CMDB_SIZE;
    if (dataSize < cmdbSize) {
        msg(usprintf("parseCmdbStoreHeader: CMDB store size %Xh (%u) is greater than volume body size %Xh (%u)",
            cmdbSize, cmdbSize,
            dataSize, dataSize), parent);
        return U_SUCCESS;
    }

    // Get store header
    const PHOENIX_CMDB_HEADER* cmdbHeader = (const PHOENIX_CMDB_HEADER*)store.constData();

    // Get parsing data
    PARSING_DATA pdata = parsingDataFromUModelIndex(parent);

    // Construct header and body
    UByteArray header = store.left(cmdbHeader->TotalSize);
    UByteArray body = store.mid(cmdbHeader->TotalSize, cmdbSize - cmdbHeader->TotalSize);

    // Add info
    UString name("CMDB store");
    UString info = usprintf("Signature: CMDB\nFull size: %Xh (%u)\nHeader size: %Xh (%u)\nBody size: %Xh (%u)",
        cmdbSize, cmdbSize,
        header.size(), header.size(),
        body.size(), body.size());

    // Add correct offset
    pdata.offset = parentOffset;

    // Add tree item
    index = model->addItem(Types::CmdbStore, 0, name, UString(), info, header, body, UByteArray(), true, parsingDataToUByteArray(pdata), parent);

    return U_SUCCESS;
}

USTATUS FfsParser::parseSlicPubkeyHeader(const UByteArray & store, const UINT32 parentOffset, const UModelIndex & parent, UModelIndex & index)
{
    const UINT32 dataSize = (const UINT32)store.size();

    // Check data size
    if (dataSize < sizeof(OEM_ACTIVATION_PUBKEY)) {
        msg(UString("parseSlicPubkeyHeader: volume body is too small even for SLIC pubkey header"), parent);
        return U_SUCCESS;
    }

    // Get SLIC pubkey header
    const OEM_ACTIVATION_PUBKEY* pubkeyHeader = (const OEM_ACTIVATION_PUBKEY*)store.constData();

    // Check store size
    if (dataSize < pubkeyHeader->Size) {
        msg(usprintf("parseSlicPubkeyHeader: SLIC pubkey size %Xh (%u) is greater than volume body size %Xh (%u)",
            pubkeyHeader->Size, pubkeyHeader->Size,
            dataSize, dataSize), parent);
        return U_SUCCESS;
    }

    // Get parsing data
    PARSING_DATA pdata = parsingDataFromUModelIndex(parent);

    // Construct header and body
    UByteArray header = store.left(sizeof(OEM_ACTIVATION_PUBKEY));

    // Add info
    UString name("SLIC pubkey");
    UString info = usprintf("Type: 0h\nFull size: %Xh (%u)\nHeader size: %Xh (%u)\nBody size: 0h (0)\n"
        "Key type: %02Xh\nVersion: %02Xh\nAlgorithm: %08Xh\nMagic: RSA1\nBit length: %08Xh\nExponent: %08Xh",
        pubkeyHeader->Size, pubkeyHeader->Size,
        header.size(), header.size(),
        pubkeyHeader->KeyType,
        pubkeyHeader->Version,
        pubkeyHeader->Algorithm,
        pubkeyHeader->BitLength,
        pubkeyHeader->Exponent);

    // Add correct offset
    pdata.offset = parentOffset;

    // Add tree item
    index = model->addItem(Types::SlicData, Subtypes::PubkeySlicData, name, UString(), info, header, UByteArray(), UByteArray(), true, parsingDataToUByteArray(pdata), parent);

    return U_SUCCESS;
}

USTATUS FfsParser::parseSlicMarkerHeader(const UByteArray & store, const UINT32 parentOffset, const UModelIndex & parent, UModelIndex & index)
{
    const UINT32 dataSize = (const UINT32)store.size();

    // Check data size
    if (dataSize < sizeof(OEM_ACTIVATION_MARKER)) {
        msg(UString("parseSlicMarkerHeader: volume body is too small even for SLIC marker header"), parent);
        return U_SUCCESS;
    }

    // Get SLIC marker header
    const OEM_ACTIVATION_MARKER* markerHeader = (const OEM_ACTIVATION_MARKER*)store.constData();

    // Check store size
    if (dataSize < markerHeader->Size) {
        msg(usprintf("parseSlicMarkerHeader: SLIC marker size %Xh (%u) is greater than volume body size %Xh (%u)",
            markerHeader->Size, markerHeader->Size,
            dataSize, dataSize), parent);
        return U_SUCCESS;
    }

    // Get parsing data
    PARSING_DATA pdata = parsingDataFromUModelIndex(parent);

    // Construct header and body
    UByteArray header = store.left(sizeof(OEM_ACTIVATION_MARKER));

    // Add info
    UString name("SLIC marker");
    UString info = usprintf("Type: 1h\nFull size: %Xh (%u)\nHeader size: %Xh (%u)\nBody size: 0h (0)\n"
        "Version: %08Xh\nOEM ID: %s\nOEM table ID: %s\nWindows flag: WINDOWS\nSLIC version: %08Xh",
        markerHeader->Size, markerHeader->Size,
        header.size(), header.size(),
        markerHeader->Version, 
        (const char*)UString((const char*)&(markerHeader->OemId)).left(6).toLocal8Bit(),
        (const char*)UString((const char*)&(markerHeader->OemTableId)).left(8).toLocal8Bit(),
        markerHeader->SlicVersion);

    // Add correct offset
    pdata.offset = parentOffset;

    // Add tree item
    index = model->addItem(Types::SlicData, Subtypes::MarkerSlicData, name, UString(), info, header, UByteArray(), UByteArray(), true, parsingDataToUByteArray(pdata), parent);

    return U_SUCCESS;
}

USTATUS FfsParser::parseIntelMicrocodeHeader(const UByteArray & store, const UINT32 parentOffset, const UModelIndex & parent, UModelIndex & index)
{
    const UINT32 dataSize = (const UINT32)store.size();

    // Check data size
    if (dataSize < sizeof(INTEL_MICROCODE_HEADER)) {
        msg(UString("parseIntelMicrocodeHeader: volume body is too small even for Intel microcode header"), parent);
        return U_SUCCESS;
    }

    // Get Intel microcode header
    const INTEL_MICROCODE_HEADER* ucodeHeader = (const INTEL_MICROCODE_HEADER*)store.constData();

    // Check store size
    if (dataSize < ucodeHeader->TotalSize) {
        msg(usprintf("parseIntelMicrocodeHeader: Intel microcode size %Xh (%u) is greater than volume body size %Xh (%u)",
            ucodeHeader->TotalSize, ucodeHeader->TotalSize,
            dataSize, dataSize), parent);
        return U_SUCCESS;
    }

    // Get parsing data
    PARSING_DATA pdata = parsingDataFromUModelIndex(parent);

    // Construct header and body
    UByteArray header = store.left(sizeof(INTEL_MICROCODE_HEADER));
    UByteArray body = store.mid(sizeof(INTEL_MICROCODE_HEADER), ucodeHeader->DataSize);

    //TODO: recalculate checksum

    // Add info
    UString name("Intel microcode");
    UString info = usprintf("Revision: 1h\nFull size: %Xh (%u)\nHeader size: %Xh (%u)\nBody size: %Xh (%u)\n"
        "Date: %08Xh\nCPU signature: %08Xh\nChecksum: %08Xh\nLoader revision: %08Xh\nCPU flags: %08Xh",
        ucodeHeader->TotalSize, ucodeHeader->TotalSize,
        header.size(), header.size(),
        body.size(), body.size(),
        ucodeHeader->Date, 
        ucodeHeader->CpuSignature, 
        ucodeHeader->Checksum, 
        ucodeHeader->LoaderRevision, 
        ucodeHeader->CpuFlags);

    // Add correct offset
    pdata.offset = parentOffset;

    // Add tree item
    index = model->addItem(Types::Microcode, Subtypes::IntelMicrocode, name, UString(), info, header, body, UByteArray(), true, parsingDataToUByteArray(pdata), parent);

    return U_SUCCESS;
}

USTATUS FfsParser::parseStoreHeader(const UByteArray & store, const UINT32 parentOffset, const UModelIndex & parent, UModelIndex & index)
{
    const UINT32 dataSize = (const UINT32)store.size();
    const UINT32* signature = (const UINT32*)store.constData();
    // Check store size
    if (dataSize < sizeof(UINT32)) {
        msg(UString("parseStoreHeader: volume body is too small even for store signature"), parent);
        return U_SUCCESS;
    }

    // Check signature and run parser function needed
    // VSS/SVS store
    if (*signature == NVRAM_VSS_STORE_SIGNATURE || *signature == NVRAM_APPLE_SVS_STORE_SIGNATURE) 
        return parseVssStoreHeader(store, parentOffset, parent, index);
    // FTW store
    else if (*signature == NVRAM_MAIN_STORE_VOLUME_GUID_DATA1 || *signature == EDKII_WORKING_BLOCK_SIGNATURE_GUID_DATA1) 
        return parseFtwStoreHeader(store, parentOffset, parent, index);
    // FDC store
    else if (*signature == NVRAM_FDC_VOLUME_SIGNATURE) 
        return parseFdcStoreHeader(store, parentOffset, parent, index);
    // Apple Fsys/Gaid store
    else if (*signature == NVRAM_APPLE_FSYS_STORE_SIGNATURE || *signature == NVRAM_APPLE_GAID_STORE_SIGNATURE) 
        return parseFsysStoreHeader(store, parentOffset, parent, index);
    // EVSA store
    else if (*(signature + 1) == NVRAM_EVSA_STORE_SIGNATURE) 
        return parseEvsaStoreHeader(store, parentOffset, parent, index);
    // Phoenix SCT flash map
    else if (*signature == NVRAM_PHOENIX_FLASH_MAP_SIGNATURE_PART1) 
        return parseFlashMapStoreHeader(store, parentOffset, parent, index);
    // Phoenix CMDB store
    else if (*signature == NVRAM_PHOENIX_CMDB_HEADER_SIGNATURE) 
        return parseCmdbStoreHeader(store, parentOffset, parent, index);
    // SLIC pubkey
    else if (*(signature + 4) == OEM_ACTIVATION_PUBKEY_MAGIC)
        return parseSlicPubkeyHeader(store, parentOffset, parent, index);
    // SLIC marker
    else if (*(const UINT64*)(store.constData() + 26) == OEM_ACTIVATION_MARKER_WINDOWS_FLAG)
        return parseSlicMarkerHeader(store, parentOffset, parent, index);
    // Intel microcode
    // Must be checked after SLIC marker because of the same *signature values
    else if (*signature == INTEL_MICROCODE_HEADER_VERSION)
        return parseIntelMicrocodeHeader(store, parentOffset, parent, index);

    msg(usprintf("parseStoreHeader: don't know how to parse a header with signature %08Xh", *signature), parent);
    return U_SUCCESS;
}

USTATUS FfsParser::parseVssStoreBody(const UModelIndex & index)
{
    // Sanity check
    if (!index.isValid())
        return U_INVALID_PARAMETER;

    // Get parsing data for the current item
    PARSING_DATA pdata = parsingDataFromUModelIndex(index);
    UINT32 parentOffset = pdata.offset + model->header(index).size();
    const UByteArray data = model->body(index);

    // Check that the is enough space for variable header
    const UINT32 dataSize = (UINT32)data.size();
    if (dataSize < sizeof(VSS_VARIABLE_HEADER)) {
        msg(UString("parseVssStoreBody: store body is too small even for VSS variable header"), index);
        return U_SUCCESS;
    }
    
    UINT32 offset = 0;

    // Parse all variables
    while (1) {
        bool isInvalid = false;
        bool isAuthenticated = false;
        bool isAppleCrc32 = false;
        
        UINT32 storedCrc32 = 0;
        UINT32 calculatedCrc32 = 0;
        UINT64 monotonicCounter = 0;
        EFI_TIME timestamp = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
        UINT32 pubKeyIndex = 0;

        UINT8 subtype = 0;
        UString name;
        UString text;
        EFI_GUID* variableGuid;
        CHAR16*   variableName;
        UByteArray header;
        UByteArray body;

        UINT32 unparsedSize = dataSize - offset;

        // Get variable header
        const VSS_VARIABLE_HEADER* variableHeader = (const VSS_VARIABLE_HEADER*)(data.constData() + offset);

        // Check variable header to fit in still unparsed data
        UINT32 variableSize = 0;
        if (unparsedSize >= sizeof(VSS_VARIABLE_HEADER) 
            && variableHeader->StartId == NVRAM_VSS_VARIABLE_START_ID) {

            // Apple VSS variable with CRC32 of the data  
            if (variableHeader->Attributes & NVRAM_VSS_VARIABLE_APPLE_DATA_CHECKSUM) {
                isAppleCrc32 = true;
                if (unparsedSize < sizeof(VSS_APPLE_VARIABLE_HEADER)) {
                    variableSize = 0;
                }
                else {
                    const VSS_APPLE_VARIABLE_HEADER* appleVariableHeader = (const VSS_APPLE_VARIABLE_HEADER*)variableHeader;
                    variableSize = sizeof(VSS_APPLE_VARIABLE_HEADER) + appleVariableHeader->NameSize + appleVariableHeader->DataSize;
                    variableGuid = (EFI_GUID*)&appleVariableHeader->VendorGuid;
                    variableName = (CHAR16*)(appleVariableHeader + 1);

                    header = data.mid(offset, sizeof(VSS_APPLE_VARIABLE_HEADER) + appleVariableHeader->NameSize);
                    body = data.mid(offset + header.size(), appleVariableHeader->DataSize);

                    // Calculate CRC32 of the variable data
                    storedCrc32 = appleVariableHeader->DataCrc32;
                    calculatedCrc32 = crc32(0, (const UINT8*)body.constData(), body.size());
                }
            }

            // Authenticated variable
            else if ((variableHeader->Attributes & NVRAM_VSS_VARIABLE_AUTHENTICATED_WRITE_ACCESS)
                || (variableHeader->Attributes & NVRAM_VSS_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS)
                || (variableHeader->Attributes & NVRAM_VSS_VARIABLE_APPEND_WRITE)
                || (variableHeader->NameSize == 0 && variableHeader->DataSize == 0)) { // If both NameSize and DataSize are zeros, it's auth variable with zero montonic counter
                isAuthenticated = true;
                if (unparsedSize < sizeof(VSS_AUTH_VARIABLE_HEADER)) {
                    variableSize = 0;
                }
                else {
                    const VSS_AUTH_VARIABLE_HEADER* authVariableHeader = (const VSS_AUTH_VARIABLE_HEADER*)variableHeader;
                    variableSize = sizeof(VSS_AUTH_VARIABLE_HEADER) + authVariableHeader->NameSize + authVariableHeader->DataSize;
                    variableGuid = (EFI_GUID*)&authVariableHeader->VendorGuid;
                    variableName = (CHAR16*)(authVariableHeader + 1);

                    header = data.mid(offset, sizeof(VSS_AUTH_VARIABLE_HEADER) + authVariableHeader->NameSize);
                    body = data.mid(offset + header.size(), authVariableHeader->DataSize);

                    monotonicCounter = authVariableHeader->MonotonicCounter;
                    timestamp = authVariableHeader->Timestamp;
                    pubKeyIndex = authVariableHeader->PubKeyIndex;
                }
            }
            
            // Normal VSS variable
            if (!isAuthenticated && !isAppleCrc32) {
                variableSize = sizeof(VSS_VARIABLE_HEADER) + variableHeader->NameSize + variableHeader->DataSize;
                variableGuid = (EFI_GUID*)&variableHeader->VendorGuid;
                variableName = (CHAR16*)(variableHeader + 1);

                header = data.mid(offset, sizeof(VSS_VARIABLE_HEADER) + variableHeader->NameSize);
                body = data.mid(offset + header.size(), variableHeader->DataSize);
            }

            // There is also a case of authenticated Apple variables, but I haven't seen one yet

            // Check variable state
            if (variableHeader->State != NVRAM_VSS_VARIABLE_ADDED && variableHeader->State != NVRAM_VSS_VARIABLE_HEADER_VALID) {
                isInvalid = true;
            }
        }

        // Can't parse further, add the last element and break the loop
        if (!variableSize) {
            // Check if the data left is a free space or a padding
            UByteArray padding = data.mid(offset, unparsedSize);
            UINT8 type;

            if (padding.count(pdata.emptyByte) == padding.size()) {
                // It's a free space
                name = UString("Free space");
                type = Types::FreeSpace;
                subtype = 0;
            }
            else {
                // Nothing is parsed yet, but the store is not empty 
                if (!offset) {
                    msg(UString("parseVssStoreBody: store can't be parsed as VSS store"), index);
                    return U_SUCCESS;
                }

                // It's a padding
                name = UString("Padding");
                type = Types::Padding;
                subtype = getPaddingType(padding);
            }

            // Get info
            UString info = usprintf("Full size: %Xh (%u)", padding.size(), padding.size());
            
            // Construct parsing data
            pdata.offset = parentOffset + offset;
            
            // Add tree item
            model->addItem(type, subtype, name, UString(), info, UByteArray(), padding, UByteArray(), false, parsingDataToUByteArray(pdata), index);

            return U_SUCCESS;
        }

        UString info;
        
        // Rename invalid variables
        if (isInvalid) {
            name = UString("Invalid");
        }
        else { // Add GUID and text for valid variables
            name = guidToUString(*variableGuid);
            info += UString("Variable GUID: ") + name + UString("\n");
            text = UString::fromUtf16(variableName);
        }

        // Add info
        info += usprintf("Full size: %Xh (%u)\nHeader size %Xh (%u)\nBody size: %Xh (%u)\nState: %02Xh\nAttributes: %08Xh (",
            variableSize, variableSize,
            header.size(), header.size(),
            body.size(), body.size(), 
            variableHeader->State,
            variableHeader->Attributes) + vssAttributesToUString(variableHeader->Attributes) + UString(")");

        // Set subtype and add related info
        if (isInvalid)
            subtype = Subtypes::InvalidVssEntry;
        else if (isAuthenticated) {
            subtype = Subtypes::AuthVssEntry;
            info += usprintf("\nMonotonic counter: %" PRIX64 "h\nTimestamp: ", monotonicCounter) + efiTimeToUString(timestamp)
                + usprintf("\nPubKey index: %u", pubKeyIndex);
        }
        else if (isAppleCrc32) {
            subtype = Subtypes::AppleVssEntry;
            info += usprintf("\nData checksum: %08Xh", storedCrc32) +
                (storedCrc32 != calculatedCrc32 ? usprintf(", invalid, should be %08Xh", calculatedCrc32) : UString(", valid"));
        }
        else
            subtype = Subtypes::StandardVssEntry;

        // Add correct offset to parsing data
        pdata.offset = parentOffset + offset;

        // Add tree item
        model->addItem(Types::VssEntry, subtype, name, text, info, header, body, UByteArray(), false, parsingDataToUByteArray(pdata), index);

        // Move to next variable
        offset += variableSize;
    }

    return U_SUCCESS;
}

USTATUS FfsParser::parseFsysStoreBody(const UModelIndex & index)
{
    // Sanity check
    if (!index.isValid())
        return U_INVALID_PARAMETER;

    // Get parsing data for the current item
    PARSING_DATA pdata = parsingDataFromUModelIndex(index);
    UINT32 parentOffset = pdata.offset + model->header(index).size();
    const UByteArray data = model->body(index);

    // Check that the is enough space for variable header
    const UINT32 dataSize = (UINT32)data.size();
    UINT32 offset = 0;

    // Parse all variables
    while (1) {
        UINT32 unparsedSize = dataSize - offset;
        UINT32 variableSize = 0;

        // Get nameSize and name of the variable
        const UINT8 nameSize = *(UINT8*)(data.constData() + offset);
        // Check sanity
        if (unparsedSize >= nameSize + sizeof(UINT8)) {
            variableSize = nameSize + sizeof(UINT8);
        }

        UByteArray name;
        if (variableSize) {
            name = data.mid(offset + sizeof(UINT8), nameSize);
            // Check for EOF variable
            if (nameSize == 3 && name[0] == 'E' && name[1] == 'O' && name[2] == 'F') {
                // There is no data afterward, add EOF variable and free space and return
                UByteArray header = data.mid(offset, sizeof(UINT8) + nameSize);
                UString info = usprintf("Full size: %Xh (%u)", header.size(), header.size());
                
                // Add correct offset to parsing data
                pdata.offset = parentOffset + offset;
                
                // Add EOF tree item
                model->addItem(Types::FsysEntry, 0, UString("EOF"), UString(), info, header, UByteArray(), UByteArray(), false, parsingDataToUByteArray(pdata), index);

                // Add free space
                offset += header.size();
                unparsedSize = dataSize - offset;
                UByteArray body = data.mid(offset);
                info = usprintf("Full size: %Xh (%u)", body.size(), body.size());

                // Add correct offset to parsing data
                pdata.offset = parentOffset + offset;

                // Add free space tree item
                model->addItem(Types::FreeSpace, 0, UString("Free space"), UString(), info, UByteArray(), body, UByteArray(), false, parsingDataToUByteArray(pdata), index);

                return U_SUCCESS;
            }
        }

        // Get dataSize and data of the variable
        const UINT16 dataSize = *(UINT16*)(data.constData() + offset + sizeof(UINT8) + nameSize);
        if (unparsedSize >= sizeof(UINT8) + nameSize + sizeof(UINT16) + dataSize) {
            variableSize = sizeof(UINT8) + nameSize + sizeof(UINT16) + dataSize;
        }
        else {
            // Last variable is bad, add the rest as padding and return
            UByteArray body = data.mid(offset);
            UString info = usprintf("Full size: %Xh (%u)", body.size(), body.size());

            // Add correct offset to parsing data
            pdata.offset = parentOffset + offset;

            // Add free space tree item
            model->addItem(Types::Padding, getPaddingType(body), UString("Padding"), UString(), info, UByteArray(), body, UByteArray(), false, parsingDataToUByteArray(pdata), index);
            
            // Show message
            msg(UString("parseFsysStoreBody: next variable appears too big, added as padding"), index);

            return U_SUCCESS;
        }

        // Construct header and body
        UByteArray header = data.mid(offset, sizeof(UINT8) + nameSize + sizeof(UINT16));
        UByteArray body = data.mid(offset + sizeof(UINT8) + nameSize + sizeof(UINT16), dataSize);

        // Add info
        UString info = usprintf("Full size: %Xh (%u)\nHeader size %Xh (%u)\nBody size: %Xh (%u)",
            variableSize, variableSize,
            header.size(), header.size(),
            body.size(), body.size());

        // Add correct offset to parsing data
        pdata.offset = parentOffset + offset;

        // Add tree item
        model->addItem(Types::FsysEntry, 0, UString(name.constData()), UString(), info, header, body, UByteArray(), false, parsingDataToUByteArray(pdata), index);

        // Move to next variable
        offset += variableSize;
    }
    
    return U_SUCCESS;
}

USTATUS FfsParser::parseEvsaStoreBody(const UModelIndex & index)
{
    // Sanity check
    if (!index.isValid())
        return U_INVALID_PARAMETER;

    // Get parsing data for the current item
    PARSING_DATA pdata = parsingDataFromUModelIndex(index);
    UINT32 parentOffset = pdata.offset + model->header(index).size();
    const UByteArray data = model->body(index);

    // Check that the is enough space for entry header
    const UINT32 dataSize = (UINT32)data.size();
    UINT32 offset = 0;

    std::map<UINT16, EFI_GUID> guidMap;
    std::map<UINT16, UString> nameMap;

    // Parse all entries
    UINT32 unparsedSize = dataSize;
    while (unparsedSize) {
        UINT32 variableSize = 0;
        UString name;
        UString info;
        UByteArray header;
        UByteArray body;
        UINT8 subtype;
        UINT8 calculated;

        const EVSA_ENTRY_HEADER* entryHeader = (const EVSA_ENTRY_HEADER*)(data.constData() + offset);

        // Check entry size
        variableSize = sizeof(EVSA_ENTRY_HEADER);
        if (unparsedSize < variableSize || unparsedSize < entryHeader->Size) {
            UByteArray body = data.mid(offset);
            UString info = usprintf("Full size: %Xh (%u)", body.size(), body.size());

            // Checke type
            UString name("Free space");
            UINT8 type = Types::FreeSpace;
            UINT8 subtype = 0;
            if (getPaddingType(body) == Subtypes::DataPadding) {
                name = UString("Padding");
                type = Types::Padding;
                subtype = Subtypes::DataPadding;
            }

            // Add correct offset to parsing data
            pdata.offset = parentOffset + offset;

            // Add free space tree item
            UModelIndex itemIndex = model->addItem(type, subtype, name, UString(), info, UByteArray(), body, UByteArray(), false, parsingDataToUByteArray(pdata), index);
            
            // Show message
            if (type == Types::Padding)
                msg(UString("parseEvsaStoreBody: variable parsing failed, rest of unparsed store added as padding"), itemIndex);
            
            break;
        }
        variableSize = entryHeader->Size;

        // Recalculate entry checksum
        calculated = calculateChecksum8(((const UINT8*)entryHeader) + 2, entryHeader->Size - 2);

        // GUID entry
        if (entryHeader->Type == NVRAM_EVSA_ENTRY_TYPE_GUID1 || 
            entryHeader->Type == NVRAM_EVSA_ENTRY_TYPE_GUID2) {
            const EVSA_GUID_ENTRY* guidHeader = (const EVSA_GUID_ENTRY*)entryHeader;
            header = data.mid(offset, sizeof(EVSA_GUID_ENTRY));
            body = data.mid(offset + sizeof(EVSA_GUID_ENTRY), guidHeader->Header.Size - sizeof(EVSA_GUID_ENTRY));
            EFI_GUID guid = *(EFI_GUID*)body.constData();
            name = guidToUString(guid);
            info = UString("GUID: ") + name + usprintf("\nFull size: %Xh (%u)\nHeader size %Xh (%u)\nBody size: %Xh (%u)\nType: %02Xh\nChecksum: %02Xh",
                variableSize, variableSize,
                header.size(), header.size(),
                body.size(), body.size(),
                guidHeader->Header.Type,
                guidHeader->Header.Checksum)
                + (guidHeader->Header.Checksum != calculated ? usprintf(", invalid, should be %02Xh", calculated) : UString(", valid"))
                + usprintf("\nGuidId: %04Xh", guidHeader->GuidId);
            subtype = Subtypes::GuidEvsaEntry;
            guidMap.insert(std::pair<UINT16, EFI_GUID>(guidHeader->GuidId, guid));
        }
        // Name entry
        else if (entryHeader->Type == NVRAM_EVSA_ENTRY_TYPE_NAME1 ||
            entryHeader->Type == NVRAM_EVSA_ENTRY_TYPE_NAME2) {
            const EVSA_NAME_ENTRY* nameHeader = (const EVSA_NAME_ENTRY*)entryHeader;
            header = data.mid(offset, sizeof(EVSA_NAME_ENTRY));
            body = data.mid(offset + sizeof(EVSA_NAME_ENTRY), nameHeader->Header.Size - sizeof(EVSA_NAME_ENTRY));
            name = UString::fromUtf16((const CHAR16*)body.constData());
            info = UString("GUID: ") + name + usprintf("\nFull size: %Xh (%u)\nHeader size %Xh (%u)\nBody size: %Xh (%u)\nType: %02Xh\nChecksum: %02Xh",
                variableSize, variableSize,
                header.size(), header.size(),
                body.size(), body.size(),
                nameHeader->Header.Type,
                nameHeader->Header.Checksum) 
                + (nameHeader->Header.Checksum != calculated ? usprintf(", invalid, should be %02Xh", calculated) : UString(", valid"))
                + usprintf("\nVarId: %04Xh", nameHeader->VarId);
            subtype = Subtypes::NameEvsaEntry;
            nameMap.insert(std::pair<UINT16, UString>(nameHeader->VarId, name));
        }
        // Data entry
        else if (entryHeader->Type == NVRAM_EVSA_ENTRY_TYPE_DATA1 ||
            entryHeader->Type == NVRAM_EVSA_ENTRY_TYPE_DATA2 ||
            entryHeader->Type == NVRAM_EVSA_ENTRY_TYPE_DATA_INVALID) {
            const EVSA_DATA_ENTRY* dataHeader = (const EVSA_DATA_ENTRY*)entryHeader;
            // Check for extended header
            UINT32 headerSize = sizeof(EVSA_DATA_ENTRY);
            UINT32 dataSize = dataHeader->Header.Size - sizeof(EVSA_DATA_ENTRY);
            if (dataHeader->Attributes & NVRAM_EVSA_DATA_EXTENDED_HEADER) {
                const EVSA_DATA_ENTRY_EXTENDED* dataHeaderExtended = (const EVSA_DATA_ENTRY_EXTENDED*)entryHeader;
                headerSize = sizeof(EVSA_DATA_ENTRY_EXTENDED);
                dataSize = dataHeaderExtended->DataSize;
                variableSize = headerSize + dataSize;
            }

            header = data.mid(offset, headerSize);
            body = data.mid(offset + headerSize, dataSize);
            name = UString("Data");
            info = usprintf("Full size: %Xh (%u)\nHeader size %Xh (%u)\nBody size: %Xh (%u)\nType: %02Xh\nChecksum: %02Xh",
                variableSize, variableSize,
                headerSize, headerSize,
                dataSize, dataSize,
                dataHeader->Header.Type,
                dataHeader->Header.Checksum)
                + (dataHeader->Header.Checksum != calculated ? usprintf(", invalid, should be %02Xh", calculated) : UString(", valid"))
                + usprintf("\nVarId: %04Xh\nGuidId: %04Xh\nAttributes: %08Xh (",
                dataHeader->VarId,
                dataHeader->GuidId,
                dataHeader->Attributes) 
                + evsaAttributesToUString(dataHeader->Attributes) + UString(")");
            subtype = Subtypes::DataEvsaEntry;
        }
        // Unknown entry or free space
        else {
            UByteArray body = data.mid(offset);
            UString info = usprintf("Full size: %Xh (%u)", body.size(), body.size());

            // Check type
            UString name("Free space");
            UINT8 type = Types::FreeSpace;
            UINT8 subtype = 0;
            if (getPaddingType(body) == Subtypes::DataPadding) {
                name = UString("Padding");
                type = Types::Padding;
                subtype = Subtypes::DataPadding;
            }

            // Add correct offset to parsing data
            pdata.offset = parentOffset + offset;

            // Add free space tree item
            UModelIndex itemIndex = model->addItem(type, subtype, name, UString(), info, UByteArray(), body, UByteArray(), false, parsingDataToUByteArray(pdata), index);

            // Show message
            if (type == Types::Padding)
                msg(usprintf("parseEvsaStoreBody: unknown variable of type %02Xh found at offset %Xh, the rest of unparsed store added as padding",entryHeader->Type, offset), itemIndex);
            break;
        }

        // Add correct offset to parsing data
        pdata.offset = parentOffset + offset;

        // Add tree item
        model->addItem(Types::EvsaEntry, subtype, name, UString(), info, header, body, UByteArray(), false, parsingDataToUByteArray(pdata), index);

        // Move to next variable
        offset += variableSize;
        unparsedSize = dataSize - offset;
    }

    // Reparse all data variables to detect invalid ones and assign name and test to valid ones
    for (int i = 0; i < model->rowCount(index); i++) {
        UModelIndex current = index.child(i, 0);
        if (model->subtype(current) == Subtypes::DataEvsaEntry) {
            UByteArray header = model->header(current);
            const EVSA_DATA_ENTRY* dataHeader = (const EVSA_DATA_ENTRY*)header.constData();
            UString guid;
            if (guidMap.count(dataHeader->GuidId)) 
                guid = guidToUString(guidMap[dataHeader->GuidId]);
            UString name;
            if (nameMap.count(dataHeader->VarId))
                name = nameMap[dataHeader->VarId];
            
            // Check for variable validity
            if (guid.isEmpty() && name.isEmpty()) { // Both name and guid aren't found
                model->setSubtype(current, Subtypes::InvalidEvsaEntry);
                model->setName(current, UString("Invalid"));
                msg(UString("parseEvsaStoreBody: data variable with invalid GuidId and invalid VarId"), current);
            }
            else if (guid.isEmpty()) { // Guid not found
                model->setSubtype(current, Subtypes::InvalidEvsaEntry);
                model->setName(current, UString("Invalid"));
                msg(UString("parseEvsaStoreBody: data variable with invalid GuidId"), current);
            }
            else if (name.isEmpty()) { // Name not found
                model->setSubtype(current, Subtypes::InvalidEvsaEntry);
                model->setName(current, UString("Invalid"));
                msg(UString("parseEvsaStoreBody: data variable with invalid VarId"), current);
            }
            else { // Variable is OK, rename it
                if (dataHeader->Header.Type == NVRAM_EVSA_ENTRY_TYPE_DATA_INVALID) {
                    model->setSubtype(current, Subtypes::InvalidEvsaEntry);
                    model->setName(current, UString("Invalid"));
                }
                else {
                    model->setName(current, guid);
                }
                model->setText(current, name);
                model->addInfo(current, UString("GUID: ") + guid + UString("\nName: ") + name + UString("\n"), false);
            }
        }
    }

    return U_SUCCESS;
}


USTATUS FfsParser::parseFlashMapBody(const UModelIndex & index)
{
    // Sanity check
    if (!index.isValid())
        return U_INVALID_PARAMETER;

    // Get parsing data for the current item
    PARSING_DATA pdata = parsingDataFromUModelIndex(index);
    UINT32 parentOffset = pdata.offset + model->header(index).size();
    const UByteArray data = model->body(index);

    
    const UINT32 dataSize = (UINT32)data.size();
    UINT32 offset = 0;
    UINT32 unparsedSize = dataSize;
    // Parse all entries
    while (unparsedSize) {
        const PHOENIX_FLASH_MAP_ENTRY* entryHeader = (const PHOENIX_FLASH_MAP_ENTRY*)(data.constData() + offset);

        // Check entry size
        if (unparsedSize < sizeof(PHOENIX_FLASH_MAP_ENTRY)) {
            // Last variable is bad, add the rest as padding and return
            UByteArray body = data.mid(offset);
            UString info = usprintf("Full size: %Xh (%u)", body.size(), body.size());

            // Add correct offset to parsing data
            pdata.offset = parentOffset + offset;

            // Add free space tree item
            model->addItem(Types::Padding, getPaddingType(body), UString("Padding"), UString(), info, UByteArray(), body, UByteArray(), false, parsingDataToUByteArray(pdata), index);

            // Show message
            if (unparsedSize < entryHeader->Size)
                msg(UString("parseFlashMapBody: next entry appears too big, added as padding"), index);

            break;
        }

        UString name = guidToUString(entryHeader->Guid);
        
        // Construct header
        UByteArray header = data.mid(offset, sizeof(PHOENIX_FLASH_MAP_ENTRY));

        // Add info
        UString info = UString("Entry GUID: ") + name + usprintf("\nFull size: 24h (36)\nHeader size: 24h (36)\nBody size: 0h (0)\n"
            "Entry type: %04Xh\nData type: %04Xh\nMemory address: %08Xh\nSize: %08Xh\nOffset: %08Xh",
            entryHeader->EntryType,
            entryHeader->DataType, 
            entryHeader->PhysicalAddress, 
            entryHeader->Size, 
            entryHeader->Offset);

        // Add correct offset to parsing data
        pdata.offset = parentOffset + offset;

        // Determine subtype
        UINT8 subtype = 0;
        switch (entryHeader->DataType) {
        case NVRAM_PHOENIX_FLASH_MAP_ENTRY_TYPE_VOLUME:
            subtype = Subtypes::VolumeFlashMapEntry;
            break;
        case NVRAM_PHOENIX_FLASH_MAP_ENTRY_TYPE_DATA_BLOCK:
            subtype = Subtypes::DataFlashMapEntry;
            break;
        }

        // Add tree item
        model->addItem(Types::FlashMapEntry, subtype, name, flashMapGuidToUString(entryHeader->Guid), info, header, UByteArray(), UByteArray(), true, parsingDataToUByteArray(pdata), index);

        // Move to next variable
        offset += sizeof(PHOENIX_FLASH_MAP_ENTRY);
        unparsedSize = dataSize - offset;
    }

    return U_SUCCESS;
}
