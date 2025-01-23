/* ffsparser_intel.cpp

 Copyright (c) 2018, Nikolaj Schlej. All rights reserved.
 This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php

 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHWARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 */

#include "ffsparser.h"

#include <map>
#include <algorithm>
#include <iostream>

#include "descriptor.h"
#include "intel_descriptor.h"
#include "ffs.h"
#include "gbe.h"
#include "me.h"
#include "intel_fit.h"
#include "nvram.h"
#include "peimage.h"
#include "parsingdata.h"
#include "types.h"
#include "utility.h"

#include "nvramparser.h"
#include "meparser.h"
#include "fitparser.h"

#include "digest/sha1.h"
#include "digest/sha2.h"
#include "digest/sm3.h"

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
        if ((UINT32)intelImage.size() < me.offset + me.length) {
            msg(usprintf("%s: ", __FUNCTION__)
                + itemSubtypeToUString(Types::Region, me.type)
                + UString(" region is located outside of the opened image. If your system uses dual-chip storage, please append another part to the opened image"),
                index);
            return U_TRUNCATED_IMAGE;
        }
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
        }

        if ((UINT32)intelImage.size() < bios.offset + bios.length) {
            msg(usprintf("%s: ", __FUNCTION__)
                + itemSubtypeToUString(Types::Region, bios.type)
                + UString(" region is located outside of the opened image. If your system uses dual-chip storage, please append another part to the opened image"),
                index);
            return U_TRUNCATED_IMAGE;
        }
        bios.data = intelImage.mid(bios.offset, bios.length);
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
                if ((UINT32)intelImage.size() < region.offset + region.length) {
                    msg(usprintf("%s: ", __FUNCTION__)
                        + itemSubtypeToUString(Types::Region, region.type)
                        + UString(" region is located outside of the opened image. If your system uses dual-chip storage, please append another part to the opened image"),
                        index);
                    return U_TRUNCATED_IMAGE;
                }
                region.data = intelImage.mid(region.offset, region.length);
                regions.push_back(region);
            }
        }
    }

    // Regions can not be empty here
    if (regions.empty()) {
        msg(usprintf("%s: descriptor parsing failed, no regions found", __FUNCTION__));
        return U_INVALID_FLASH_DESCRIPTOR;
    }

    // Sort regions in ascending order
    std::sort(regions.begin(), regions.end());

    // Check for intersections and paddings between regions
    REGION_INFO region;
    // Check intersection with the descriptor
    if (regions.front().offset < FLASH_DESCRIPTOR_SIZE) {
        msg(usprintf("%s: ", __FUNCTION__)
            + itemSubtypeToUString(Types::Region, regions.front().type)
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
        // Check for intersection with previous region
        if (regions[i].offset < previousRegionEnd) {
            msg(usprintf("%s: ", __FUNCTION__)
                + itemSubtypeToUString(Types::Region, regions[i].type)
                + UString(" region has intersection with ") + itemSubtypeToUString(Types::Region, regions[i - 1].type)
                + UString(" region"),
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
        region.length = (UINT32)(intelImage.size() - region.offset);
        region.data = intelImage.mid(region.offset, region.length);
        region.type = getPaddingType(region.data);
        regions.push_back(region);
    }

    // Region map is consistent

    // Intel image
    UString name("Intel image");
    UString info = usprintf("Full size: %Xh (%u)\nFlash chips: %u\nRegions: %u\nMasters: %u\nPCH straps: %u\nPROC straps: %u",
                            (UINT32)intelImage.size(), (UINT32)intelImage.size(),
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
            info += "\n" + itemSubtypeToUString(Types::Region, regions[i].type)
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
        if (jedecId.startsWith("Unknown")) {
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
                                (UINT32)padding.size(), (UINT32)padding.size());

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
                            (UINT32)gbe.size(), (UINT32)gbe.size(),
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
    UString info = usprintf("Full size: %Xh (%u)", (UINT32)me.size(), (UINT32)me.size());

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
        UINT32 sig2Value = ME_VERSION_SIGNATURE2;
        UByteArray sig2((const char*)&sig2Value, sizeof(sig2Value));
        INT32 versionOffset = (INT32)me.indexOf(sig2);
        if (versionOffset < 0) { // New signature not found
            // Search for old signature
            UINT32 sigValue = ME_VERSION_SIGNATURE;
            UByteArray sig((const char*)&sigValue, sizeof(sigValue));
            versionOffset = (INT32)me.indexOf(sig);
            if (versionOffset < 0) {
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
    UString info = usprintf("Full size: %Xh (%u)", (UINT32)pdr.size(), (UINT32)pdr.size());

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
    UString info = usprintf("Full size: %Xh (%u)", (UINT32)devExp1.size(), (UINT32)devExp1.size());

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
