/* ffsparser_amd.cpp

 Copyright (c) 2025 Patrick Rudolph. All rights reserved.
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

#include "amd_descriptor.h"
#include "utility.h"

// Converts addresses to be relative to the start of the file (ROM addressing)
USTATUS FfsParser::amdRelativeOffset(const UModelIndex& parent, const UINT64 addr, const UINT64 mode, UINT64 & outaddr)
{
    switch (mode) {
    /* Since this utility operates on the BIOS file, physical address is converted
       relative to the start of the BIOS file. */
    case AMD_ADDR_PHYSICAL:
        if (addr < SPI_ROM_BASE || addr > (SPI_ROM_BASE + FILE_REL_MASK)) {
            msg(usprintf("%s: Invalid address(%lx) or mode(%lx)\n", __FUNCTION__, addr, mode));
        }
        outaddr = addr & FILE_REL_MASK;
        return U_SUCCESS;
    case AMD_ADDR_REL_BIOS:
        {
            UModelIndex parentImageIndex = (model->type(parent) == Types::Image) ? parent : model->findParentOfType(parent, Types::Image);
            UByteArray parentImage = model->header(parentImageIndex) + model->body(parentImageIndex) + model->tail(parentImageIndex);
            if (addr >= parentImage.size()) {
                msg(usprintf("%s: Invalid address(%lx) or mode(%lx)\n", __FUNCTION__, addr, mode));
                return U_INVALID_PARAMETER;
            }
            outaddr = addr;
            return U_SUCCESS;
        }
    case AMD_ADDR_REL_TAB:
        outaddr = addr + model->base(parent);
        return U_SUCCESS;
    default:
        msg(usprintf("Unsupported mode %lu\n", mode));
        return U_INVALID_PARAMETER;
    }
}

// Convert the ID to known file names
UString FfsParser::pspFileName(const UINT8 type, const UINT8 subtype)
{
#define ENUM_TO_STRING(x) case x: return usprintf("PSP file %02x - %02x (%s)", type, subtype, #x);
    switch (type) {
        ENUM_TO_STRING(AMD_FW_PSP_PUBKEY)
        ENUM_TO_STRING(AMD_FW_PSP_BOOTLOADER)
        ENUM_TO_STRING(AMD_FW_PSP_SECURED_OS)
        ENUM_TO_STRING(AMD_FW_PSP_RECOVERY)
        ENUM_TO_STRING(AMD_FW_PSP_NVRAM)
        ENUM_TO_STRING(AMD_FW_RTM_PUBKEY)
        ENUM_TO_STRING(AMD_FW_PSP_SMU_FIRMWARE)
        ENUM_TO_STRING(AMD_FW_PSP_SECURED_DEBUG)
        ENUM_TO_STRING(AMD_FW_PSP_TRUSTLETS)
        ENUM_TO_STRING(AMD_FW_PSP_TRUSTLETKEY)
        ENUM_TO_STRING(AMD_FW_PSP_SMU_FIRMWARE2)
        ENUM_TO_STRING(AMD_DEBUG_UNLOCK)
        ENUM_TO_STRING(AMD_FW_PSP_TEEIPKEY)
        ENUM_TO_STRING(AMD_BOOT_DRIVER)
        ENUM_TO_STRING(AMD_SOC_DRIVER)
        ENUM_TO_STRING(AMD_DEBUG_DRIVER)
        ENUM_TO_STRING(AMD_INTERFACE_DRIVER)
        ENUM_TO_STRING(AMD_HW_IPCFG)
        ENUM_TO_STRING(AMD_WRAPPED_IKEK)
        ENUM_TO_STRING(AMD_TOKEN_UNLOCK)
        ENUM_TO_STRING(AMD_SEC_GASKET)
        ENUM_TO_STRING(AMD_MP2_FW)
        ENUM_TO_STRING(AMD_DRIVER_ENTRIES)
        ENUM_TO_STRING(AMD_FW_KVM_IMAGE)
        ENUM_TO_STRING(AMD_FW_MP5)
        ENUM_TO_STRING(AMD_S0I3_DRIVER)
        ENUM_TO_STRING(AMD_FW_ABL_PUBKEY)
        ENUM_TO_STRING(AMD_PSP_FUSE_CHAIN)
        ENUM_TO_STRING(AMD_FW_BIOS_TABLE)
        ENUM_TO_STRING(AMD_FW_RECOVERYAB_A)
        ENUM_TO_STRING(AMD_FW_RECOVERYAB_B)

        // BIOS types
        ENUM_TO_STRING(AMD_BIOS_SIG)
        ENUM_TO_STRING(AMD_BIOS_APCB)
        ENUM_TO_STRING(AMD_BIOS_APOB)
        ENUM_TO_STRING(AMD_BIOS_BIN)
        ENUM_TO_STRING(AMD_BIOS_APOB_NV)
        ENUM_TO_STRING(AMD_BIOS_PMUI)
        ENUM_TO_STRING(AMD_BIOS_PMUD)
        ENUM_TO_STRING(AMD_BIOS_UCODE)
        ENUM_TO_STRING(AMD_BIOS_APCB_BK)
        ENUM_TO_STRING(AMD_BIOS_EARLY_VGA)
        ENUM_TO_STRING(AMD_BIOS_MP2_CFG)
        ENUM_TO_STRING(AMD_BIOS_PSP_SHARED_MEM)
        ENUM_TO_STRING(AMD_BIOS_L2_PTR)
        ENUM_TO_STRING(AMD_BIOS_INVALID)
        default:
           break;
    }
#undef ENUM_TO_STRING
    return usprintf("PSP file %02x - %02x", type, subtype);
}

// Returns U_SUCCESS when the table is known, fully within the specified UByteArray
// and the checksum is valid
USTATUS FfsParser::isValidTable(const UByteArray & amdImage, const UINT32 offset)
{
    if (offset % 16) {
        msg(usprintf("%s: Invalid offset specified: %x", __FUNCTION__, offset));
        return U_INVALID_PARAMETER;
    }

    if ((offset + sizeof(UINT32)) > amdImage.size()) {
        msg(usprintf("%s: Offset outside of image: %x", __FUNCTION__, offset));
        return U_BUFFER_TOO_SMALL;
    }

    const UINT32 *cookie = (const UINT32 *)(amdImage.constData() + offset);
    UINT32 headerSize = 0;
    switch (*cookie) {
        case AMD_PSP_COMBO_DIRECTORY_HEADER_SIGNATURE:
        case AMD_PSP_BHD2_DIRECTORY_HEADER_SIGNATURE:
            headerSize = sizeof(AMD_PSP_COMBO_DIRECTORY_HEADER);
            break;
        case AMD_PSP_DIRECTORY_HEADER_SIGNATURE:
        case AMD_PSPL2_DIRECTORY_HEADER_SIGNATURE:
            headerSize = sizeof(AMD_PSP_DIRECTORY_HEADER);
            break;
        case AMD_BIOS_HEADER_SIGNATURE:
        case AMD_BHDL2_HEADER_SIGNATURE:
            headerSize = sizeof(AMD_BIOS_DIRECTORY_HEADER);
            break;
        default:
            msg(usprintf("%s: Unknown cookie 0x%x", __FUNCTION__, *cookie));
            return U_UNKNOWN_ITEM_TYPE;
    }

    // Full header is part of image?
    if ((offset + headerSize) > amdImage.size()) {
        msg(usprintf("%s: Table header at 0x%X not within image", __FUNCTION__, offset));
        return U_BUFFER_TOO_SMALL;
    }

    // Fill in table specific details
    UINT32 checksumStart;
    UINT32 checksumSize;
    UINT32 size;
    UINT32 checksum;
    switch (*cookie) {
        case AMD_PSP_COMBO_DIRECTORY_HEADER_SIGNATURE:
        case AMD_PSP_BHD2_DIRECTORY_HEADER_SIGNATURE:
        {
               const AMD_PSP_COMBO_DIRECTORY_HEADER* hdr = (const AMD_PSP_COMBO_DIRECTORY_HEADER*)(cookie);
            size = hdr->Num_Entries * sizeof(AMD_PSP_COMBO_ENTRY) + sizeof(AMD_PSP_COMBO_DIRECTORY_HEADER);
            checksum = hdr->Checksum;

            checksumStart = offset + 8; // Start after checksum field
            checksumSize = size - 8; // Subtract cookie and checksum field
            break;
        }
        case AMD_PSP_DIRECTORY_HEADER_SIGNATURE:
        case AMD_PSPL2_DIRECTORY_HEADER_SIGNATURE:
        {
            const AMD_PSP_DIRECTORY_HEADER* hdr = (const AMD_PSP_DIRECTORY_HEADER*)(cookie);
            size = hdr->Num_Entries * sizeof(AMD_PSP_DIRECTORY_ENTRY) + sizeof(AMD_PSP_DIRECTORY_HEADER);
            checksum = hdr->Checksum;

            checksumStart = offset + 8; // Start after checksum field
            checksumSize = size - 8; // Subtract cookie and checksum field
            break;
        }
        case AMD_BIOS_HEADER_SIGNATURE:
        case AMD_BHDL2_HEADER_SIGNATURE:
        {
            const AMD_BIOS_DIRECTORY_HEADER* hdr = (const AMD_BIOS_DIRECTORY_HEADER*)(cookie);
            size = hdr->Num_Entries * sizeof(AMD_BIOS_DIRECTORY_ENTRY) + sizeof(AMD_BIOS_DIRECTORY_HEADER);
            checksum = hdr->Checksum;

            checksumStart = offset + 8; // Start after checksum field
            checksumSize = size - 8; // Subtract cookie and checksum field
            break;
        }
        default:
            return U_UNKNOWN_ITEM_TYPE;
    }

    // Full table is part of image?
    if ((offset + size) > amdImage.size()) {
        msg(usprintf("%s: Table at 0x%X not within image", __FUNCTION__, offset));
        return U_BUFFER_TOO_SMALL;
    }

    // Validate table checksum
    const UINT32 calcChecksum = fletcher32(amdImage.mid(checksumStart, checksumSize));
    if (calcChecksum != checksum) {
        msg(usprintf("%s: Header at offset %x checksum mismatch, expected %x found %x", __FUNCTION__, offset, checksum, calcChecksum));
        return U_INVALID_IMAGE;
    }

    return U_SUCCESS;
}

// Returns U_SUCCESS when the table could be extracted from the image
USTATUS FfsParser::extractTable(const UByteArray & amdImage, const UINT32 offset, UByteArray & table)
{
    USTATUS result;

    result = isValidTable(amdImage, offset);
    if (result != U_SUCCESS) {
        return result;
    }

    const UINT32 *cookie = (const UINT32 *)(amdImage.constData() + offset);
    UINT32 size;
    switch (*cookie) {
        case AMD_PSP_COMBO_DIRECTORY_HEADER_SIGNATURE:
        case AMD_PSP_BHD2_DIRECTORY_HEADER_SIGNATURE:
        {
            const AMD_PSP_COMBO_DIRECTORY_HEADER* hdr = (const AMD_PSP_COMBO_DIRECTORY_HEADER*)(cookie);
            size = hdr->Num_Entries * sizeof(AMD_PSP_COMBO_ENTRY) + sizeof(AMD_PSP_COMBO_DIRECTORY_HEADER);
            break;
        }
        case AMD_PSP_DIRECTORY_HEADER_SIGNATURE:
        case AMD_PSPL2_DIRECTORY_HEADER_SIGNATURE:
        {
            const AMD_PSP_DIRECTORY_HEADER* hdr = (const AMD_PSP_DIRECTORY_HEADER*)(cookie);
            size = hdr->Num_Entries * sizeof(AMD_PSP_DIRECTORY_ENTRY) + sizeof(AMD_PSP_DIRECTORY_HEADER);
            break;
        }
        case AMD_BIOS_HEADER_SIGNATURE:
        case AMD_BHDL2_HEADER_SIGNATURE:
        {
            const AMD_BIOS_DIRECTORY_HEADER* hdr = (const AMD_BIOS_DIRECTORY_HEADER*)(cookie);
            size = hdr->Num_Entries * sizeof(AMD_BIOS_DIRECTORY_ENTRY) + sizeof(AMD_BIOS_DIRECTORY_HEADER);
            break;
        }
        default:
            return U_UNKNOWN_ITEM_TYPE;
    }

    table = amdImage.mid(offset, size);
    return U_SUCCESS;
}

USTATUS FfsParser::parseISH(const UByteArray & fileImage, const UModelIndex & parent, UModelIndex & index)
{
    USTATUS result;

    // Parse ISH table
    const AMD_ISH_DIRECTORY_TABLE* ishTable = (const AMD_ISH_DIRECTORY_TABLE*)fileImage.constData();
    UINTN length = sizeof(AMD_ISH_DIRECTORY_TABLE);

    if (fileImage.size() < length) {
        return U_BUFFER_TOO_SMALL;
    }

    // Checksum starts right after checksum field
    UByteArray data = fileImage.mid(4, length - 4);
    const UINT32 Checksum = fletcher32(data);
    if (ishTable->Checksum != Checksum) {
        msg(usprintf("%s: ISH table checksum mismatch, expected %x found %x", __FUNCTION__, ishTable->Checksum, Checksum));
        return U_INVALID_IMAGE;
    }

    data = fileImage.mid(0, length);

    // ISH directory
    UString name("ISH directory");
    UString info = usprintf("Full size    : %Xh (%u)\nPl2_location : %Xh (%u)\nBoot_Priority: %Xh (%u)\nSlot_Max_Size: %Xh (%u)\nPsp_Id       : %Xh (%u)\n",
                            (UINT32)length, (UINT32)length,
                            ishTable->Pl2_location, ishTable->Pl2_location,
                            ishTable->Boot_Priority, ishTable->Boot_Priority,
                            ishTable->Slot_Max_Size, ishTable->Slot_Max_Size,
                            ishTable->Psp_Id, ishTable->Psp_Id);

    // Add ISH directory image tree item
    index = model->addItem(0, Types::DirectoryTable, Subtypes::ISHDirectory, name, UString(), info, UByteArray(), data, UByteArray(), Fixed, parent);
    const UModelIndex ishIndex = index;

    // Add PSPL2 directory tree item
    result = parsePSPDir(ishTable->Pl2_location, ishIndex, index);
    if (result != U_SUCCESS) {
        msg(usprintf("%s: Failed to parse PSPL2 pointed to by ISH", __FUNCTION__));
        return result;
    }

    return U_SUCCESS;
}

USTATUS FfsParser::findByRange(const UINT32 base, const UINT32 size, const UModelIndex & index, UModelIndex & found)
{
    // Sort by inserting
    for (int i = 0; i < model->rowCount(index); i++) {
        UModelIndex current = index.model()->index(i, 0, index);
        UINT32 currentSize = (model->header(current).size() + model->body(current).size() + model->tail(current).size());

        // Non overlapping case
        if ((model->base(current) + currentSize) < base || model->base(current) > (base + size))
            continue;
        // Expect range to start at base or earlier
        if (model->base(current) > base)
            continue;
        // Existing region is too small
        if (currentSize < size)
            continue;
        found = current;

        findByRange(base, size, current, found);

        return U_SUCCESS;
    }
    return U_ITEM_NOT_FOUND;
}

USTATUS FfsParser::insertRegion(UINT32 imageOffset, const UINT32 size, const UString name, UINT8 type, UINT8 subType, const UModelIndex & parent, UModelIndex & index)
{
    UString parentName = (model->type(parent) != Types::Image) ? model->name(parent): UString();
    USTATUS result;

    UModelIndex parentImageIndex = (model->type(parent) != Types::Image) ? model->findParentOfType(parent, Types::Image) : parent;
    UByteArray parentImage = model->header(parentImageIndex) + model->body(parentImageIndex) + model->tail(parentImageIndex);
    UModelIndex containerIndex = parentImageIndex;

    msg(usprintf("%s: inserting %x-%x: ", __FUNCTION__, imageOffset, imageOffset + size) + name);

    UModelIndex findIndex;
    result = findByRange(imageOffset, size, parentImageIndex, findIndex);
    if (result == U_SUCCESS && findIndex.isValid()) {
        msg(usprintf("\n%s: findByRange returned model type %x:%x at offset %x, size %x", __FUNCTION__, model->type(findIndex), model->subtype(findIndex), model->base(findIndex), model->body(findIndex).size()));
        msg(usprintf("%s: was looking for model type %x:%x at offset %x, size %x\n", __FUNCTION__, type, subType, imageOffset, size));
    }
    if (result == U_SUCCESS && findIndex.isValid() && model->type(findIndex) == type && model->subtype(findIndex) == subType &&
        model->base(findIndex) == imageOffset && (model->header(findIndex).size() + model->body(findIndex).size() + model->tail(findIndex).size()) == size) {
        UString info;
        info += UString("Parent       : ") + parentName + UString("\n");
        info += usprintf("ParentOffset : %Xh\n", model->base(parent));

        model->addInfo(findIndex, info);

        msg(usprintf("%s: Skipping already added model type %x:%x at offset %x", __FUNCTION__, type, subType, imageOffset));
        return U_SUCCESS;
    } else if (result == U_SUCCESS && findIndex.isValid() && model->type(findIndex) != type && model->base(findIndex) <= imageOffset &&
               (model->header(findIndex).size() + model->body(findIndex).size() + model->tail(findIndex).size()) > size) {
        containerIndex = findIndex;
    }

    UString info = usprintf("Full size    : %Xh (%u)\nOffset       : %Xh (%u)\n",
                            size, size, imageOffset, imageOffset);

    if (model->type(parent) != Types::Image) {
        info += UString("Parent       : ") + parentName + UString("\n");
        info += usprintf("ParentOffset : %Xh\n", model->base(parent));
    }

    // Sort by inserting
    for (int i = 0; i < model->rowCount(containerIndex); i++) {
        UModelIndex current = index.model()->index(i, 0, containerIndex);

        if (model->base(current) > imageOffset) {
            // Add directory file tree item
            index = model->addItem(imageOffset - model->base(containerIndex), type, subType, name, parentName, info, UByteArray(), parentImage.mid(imageOffset, size), UByteArray(), Fixed, current, CREATE_MODE_BEFORE);
            return U_SUCCESS;
        }
    }

    // Add directory file tree item
    index = model->addItem(imageOffset - model->base(containerIndex), type, subType, name, parentName, info, UByteArray(), parentImage.mid(imageOffset, size), UByteArray(), Fixed, containerIndex);
    return U_SUCCESS;
}

USTATUS FfsParser::insertDirectoryFile(const UINT32 imageOffset, const UINT32 size, const UString name, const UModelIndex & parent, UModelIndex & index)
{
    /*
     * Directories can reference that same BIOS region multiple
     * times, due to A/B partition support or because multiple SoC
     * versions are supported in one ROM.
     * Check if the requested file already exists and do not create
     * a new region.
     */
    return insertRegion(imageOffset, size, name, Types::Region, Subtypes::PspDirectoryFile, parent, index);
}

USTATUS FfsParser::parseComboDir(const UINT32 offset, const UModelIndex & parent, UModelIndex & index)
{
    UINTN level = 0;
    USTATUS result;

    UModelIndex parentImageIndex = (model->type(parent) == Types::Image) ? parent : model->findParentOfType(parent, Types::Image);
    UByteArray parentImage = model->header(parentImageIndex) + model->body(parentImageIndex) + model->tail(parentImageIndex);
    UString parentName = (model->type(parent) != Types::Image) ? model->name(parent) : "";

    UByteArray tableImage;
    // extractTable also validates the input
    result = extractTable(parentImage, offset, tableImage);
    if (result != U_SUCCESS) {
        return result;
    }

    const AMD_PSP_COMBO_DIRECTORY_HEADER* hdr = (const AMD_PSP_COMBO_DIRECTORY_HEADER*)(tableImage.data());

    // Check PSP signature
    switch (hdr->Cookie) {
        case AMD_PSP_COMBO_DIRECTORY_HEADER_SIGNATURE:
            level = 1;
            break;
        case AMD_PSP_BHD2_DIRECTORY_HEADER_SIGNATURE:
            level = 2;
            break;
        default:
            msg(usprintf("%s: Combo header has invalid cookie 0x%x", __FUNCTION__, hdr->Cookie));
            return U_INVALID_IMAGE;
    }

    // PSP combo directory table
    UString name = usprintf("PSP %s directory table", (level == 1) ? "COMBO" : "BHD2");
    UString info = usprintf("Full size    : %Xh (%u)\nOffset       : %Xh (%u)\nEntry count  : %u\n",
                            (UINT32)tableImage.size(), (UINT32)tableImage.size(),
                            (UINT32)offset, (UINT32)offset,
                            hdr->Num_Entries);
    if (parentName != "") {
        name += "(" + parentName + ")";
    }

    // Add PSP combo directory table
    result = insertRegion(offset, tableImage.size(), name, Types::DirectoryTable, Subtypes::ComboDirectory, parent, index);
    if (result != U_SUCCESS) {
        return result;
    }
    const UModelIndex pspHeaderIndex = index;

    const AMD_PSP_COMBO_ENTRY* Entry = (const AMD_PSP_COMBO_ENTRY*)(hdr + 1);
    for (UINTN i = 0; i < hdr->Num_Entries; i++) {
        // Fill directory table
        REGION_INFO psp_entry;
        psp_entry.type = Entry[i].Id;
        psp_entry.offset = sizeof(AMD_PSP_COMBO_DIRECTORY_HEADER) + i * sizeof(AMD_PSP_COMBO_ENTRY);
        psp_entry.length = sizeof(AMD_PSP_COMBO_ENTRY);
        psp_entry.data = tableImage.mid(psp_entry.offset, psp_entry.length);

        UString entryName = name  + " entry";

        // PSP table entry
        info = usprintf("Full size    : %Xh (%u)\nOffset       : %Xh (%u)\nID Select    : %Xh\nID           : %Xh\nLvl2_Address : %llXh\n",
                        (UINT32)psp_entry.length, (UINT32)psp_entry.length,
                        (UINT32)psp_entry.offset, (UINT32)psp_entry.offset,
                        Entry[i].Id_Sel, Entry[i].Id, Entry[i].Lvl2_Address);

        // Add PSP table entry image tree item
        index = model->addItem(psp_entry.offset, Types::DirectoryTableEntry, Subtypes::ComboDirectory, entryName, UString(), info, UByteArray(), psp_entry.data, UByteArray(), Fixed, pspHeaderIndex);
        UModelIndex pspEntryIndex = index;

        if (isSupportedCookie(parentImage, Entry[i].Lvl2_Address) == U_SUCCESS) {
            result = parsePSPDir(Entry[i].Lvl2_Address, pspEntryIndex, index);
        }

        if (result != U_SUCCESS) {
            return result;
        }
    }
    return U_SUCCESS;
}

USTATUS FfsParser::decompressBios(UModelIndex& parent)
{
    USTATUS result;

    UByteArray parentImage = model->header(parent) + model->body(parent) + model->tail(parent);
    if (parentImage.size() < 256)  {
        return U_BUFFER_TOO_SMALL;
    }
    parentImage = parentImage.mid(256, parentImage.size() - 256);
    UByteArray output;

    result = zlibDecompress(parentImage, output);
    if (result) {
        msg(usprintf("%s: decompression failed with error ", __FUNCTION__) + errorCodeToUString(result), parent);
        return U_SUCCESS;
    }

    model->setUncompressedData(parent, output);
    model->setCompressed(parent, true);

    return U_SUCCESS;
}

USTATUS FfsParser::parseBiosDir(const UINT32 imageOffset, const UModelIndex & parent, UModelIndex & index)
{
    UINTN level = 0;
    USTATUS result;

    UModelIndex parentImageIndex = (model->type(parent) == Types::Image) ? parent : model->findParentOfType(parent, Types::Image);
    UByteArray parentImage = model->header(parentImageIndex) + model->body(parentImageIndex) + model->tail(parentImageIndex);
    UString parentName = (model->type(parent) != Types::Image) ? model->name(parent) : "";

    UByteArray tableImage;
    // extractTable also validates the input
    result = extractTable(parentImage, imageOffset, tableImage);
    if (result != U_SUCCESS) {
        msg(usprintf("%s: Failed to extract table", __FUNCTION__));
        return result;
    }

    const AMD_BIOS_DIRECTORY_HEADER* hdr = (const AMD_BIOS_DIRECTORY_HEADER*)(tableImage.data());

    // Check Bios directory signature
    switch (hdr->Cookie) {
        case AMD_BIOS_HEADER_SIGNATURE:
            level = 1;
            break;
        case AMD_BHDL2_HEADER_SIGNATURE:
            level = 2;
            break;
        default:
            msg(usprintf("%s: Bios header has invalid cookie 0x%x", __FUNCTION__, hdr->Cookie));
            return U_INVALID_IMAGE;
    }

    // Bios directory table
    UString name = usprintf("Bios %s directory table", (level == 1) ? "COMBO" : "BHD2");
    UString info = usprintf("Full size    : %Xh (%u)\nOffset       : %Xh (%u)\nEntry count  : %u\n",
                            (UINT32)tableImage.size(), (UINT32)tableImage.size(),
                            (UINT32)imageOffset, (UINT32)imageOffset,
                            hdr->Num_Entries);
    if (parentName != "") {
        name += "(" + parentName + ")";
    }

    // Add Bios directory table
    result = insertRegion(imageOffset, tableImage.size(), name, Types::DirectoryTable, Subtypes::BiosDirectory, parent, index);
    if (result != U_SUCCESS) {
        return result;
    }

    const UModelIndex biosHeaderIndex = index;

    const AMD_BIOS_DIRECTORY_ENTRY* Entry = (const AMD_BIOS_DIRECTORY_ENTRY*)(hdr + 1);
    for (UINTN i = 0; i < hdr->Num_Entries; i++) {
        const UINT64 addr = Entry[i].Address_AddressMode & 0x3fffffffffffffffULL;
        const UINT64 mode = Entry[i].Address_AddressMode >> 62;
        const UINT32 size = Entry[i].Size;

        // Fill directory table
        REGION_INFO bios_entry;
        bios_entry.type = Entry[i].Type;
        bios_entry.offset = sizeof(AMD_BIOS_DIRECTORY_HEADER) + i * sizeof(AMD_BIOS_DIRECTORY_ENTRY);
        bios_entry.length = sizeof(AMD_BIOS_DIRECTORY_ENTRY);
        bios_entry.data = tableImage.mid(bios_entry.offset, bios_entry.length);
        const UString fileName = pspFileName(Entry[i].Type, Entry[i].RegionType);

        // Bios table entry
        info = usprintf("Full size    : %Xh (%u)\nOffset       : %Xh (%u)\nType         : %Xh\nRegionType   : %Xh\nFlags        : %Xh\nAddress      : %llXh\nAddressMode  : %Xh\nDestination  : %llXh\n",
                        (UINT32)bios_entry.length, (UINT32)bios_entry.length,
                        (UINT32)bios_entry.offset, (UINT32)bios_entry.offset,
                        Entry[i].Type, Entry[i].RegionType, Entry[i].Flags, addr, mode, Entry[i].Destination);

        // Add Bios table entry image tree item
        index = model->addItem(bios_entry.offset, Types::DirectoryTableEntry, Subtypes::BiosDirectory, fileName, UString(), info, UByteArray(), bios_entry.data, UByteArray(), Fixed, biosHeaderIndex);

        // Look for files based on directory table
        UINT64 entry_offset = 0;
        result = amdRelativeOffset(biosHeaderIndex, addr, mode, entry_offset);
        if (result != U_SUCCESS) {
            msg(usprintf("%s: amdRelativeOffset failed for file ", __FUNCTION__) + name);
            continue;
        }
        if (size == 0x00000000 || size == 0xffffffff) {
            msg(usprintf("%s: Skipping BIOS file %d with no size", __FUNCTION__, i));
            continue;
        }

        // BIOS file entry
        info = usprintf("Full size    : %Xh (%u)\nOffset       : %Xh (%u)\nType         : %Xh\nRegionType   : %Xh\nFlags        : %Xh\n Reset Image : %s\n Copy Image  : %s\n ReadOnly    : %s\n Compressed  : %s",
                        (UINT32)size, (UINT32)size,
                        (UINT32)entry_offset, (UINT32)entry_offset,
                        Entry[i].Type, Entry[i].RegionType, Entry[i].Flags,
                        (Entry[i].Flags & 1) ? "true" : "false",
                        (Entry[i].Flags & 2) ? "true" : "false",
                        (Entry[i].Flags & 4) ? "true" : "false",
                        (Entry[i].Flags & 8) ? "true" : "false"
                        );

        UModelIndex findIndex = model->findByBase(entry_offset - model->base(parentImageIndex));
        if (findIndex.isValid() && model->type(findIndex) == Types::Region && model->subtype(findIndex) == Subtypes::BiosRegion) {
            msg(usprintf("%s: Skipping already added BIOS file", __FUNCTION__) + fileName);
            continue;
        }

        result = insertDirectoryFile((UINT32)entry_offset, size, fileName, biosHeaderIndex, index);
        if (result != U_SUCCESS) {
            msg(usprintf("%s: Failed to create directory file", __FUNCTION__) + fileName);
        }
        const UModelIndex biosFileIndex = index;

        switch (Entry[i].Type) {
            case AMD_BIOS_L2_PTR:
                if (level == 1) {
                    result = parseBiosDir(entry_offset, parentImageIndex, index);
                }
                break;
            case AMD_BIOS_BIN:

                if (Entry[i].Flags & 8) {
                    result = decompressBios(index);
                    parseGenericImage(model->uncompressedData(biosFileIndex), 0, biosFileIndex, index);
                } else {
                    parseGenericImage(model->body(biosFileIndex), 0, biosFileIndex, index);
                }

                break;
        }
        if (result != U_SUCCESS) {
            msg(usprintf("%s: Failed to parse file", __FUNCTION__) + fileName);
        }
    }
    return U_SUCCESS;
}

USTATUS FfsParser::parsePSPDir(const UINT32 offset, const UModelIndex & parent, UModelIndex & index)
{
    UINTN level;
    USTATUS result;

    UModelIndex parentImageIndex = (model->type(parent) == Types::Image) ? parent : model->findParentOfType(parent, Types::Image);
    UByteArray parentImage = model->header(parentImageIndex) + model->body(parentImageIndex) + model->tail(parentImageIndex);
    UString parentName = (model->type(parent) != Types::Image) ? model->name(parent) : "";

    UByteArray tableImage;
    // extractTable also validates the input
    result = extractTable(parentImage, offset, tableImage);
    if (result != U_SUCCESS) {
        return result;
    }

    const AMD_PSP_DIRECTORY_HEADER* hdr = (const AMD_PSP_DIRECTORY_HEADER*)(tableImage.constData());

    // Check PSP signature
    switch (hdr->Cookie) {
        case AMD_PSP_DIRECTORY_HEADER_SIGNATURE:
            level = 1;
            break;
        case AMD_PSPL2_DIRECTORY_HEADER_SIGNATURE:
            level = 2;
            break;
        default:
            msg(usprintf("%s: PSP header has invalid cookie 0x%x", __FUNCTION__, hdr->Cookie));
            return U_INVALID_IMAGE;
    }

    // Update REGION_INFO
    REGION_INFO psp_directory;
    if (level == 1)
        psp_directory.type = Subtypes::PspL1DirectoryRegion;
    else
        psp_directory.type = Subtypes::PspL2DirectoryRegion;
    psp_directory.offset = offset;
    psp_directory.length = (hdr->Additional_Info_Fields & 0x3ff) << 12;

    // Validate image length
    if ((psp_directory.offset + psp_directory.length) > parentImage.size()) {
        return U_INVALID_IMAGE;
    }
    psp_directory.data = parentImage.mid(psp_directory.offset, psp_directory.length);

    // PSP directory
    UString name = usprintf("PSPL%d directory", level);
    if (parentName != "") {
        name += " (" + parentName + ")";
    }
    UString info = usprintf("Full size     : %Xh (%u)\nOffset        : %Xh (%u)\nPSP Table Size: %Xh (%u)",
                            (UINT32)psp_directory.length, (UINT32)psp_directory.length,
                            (UINT32)psp_directory.offset, (UINT32)psp_directory.offset,
                            (UINT32)tableImage.size(), (UINT32)tableImage.size());

    // Add PSP directory region
    result = insertRegion(offset, (hdr->Additional_Info_Fields & 0x3ff) << 12, name, Types::Region, psp_directory.type, parent, index);
    if (result != U_SUCCESS) {
        return result;
    }
    const UModelIndex pspRegionIndex = index;

    // PSP directory table
    name = UString("PSP directory table");
    info = usprintf("Full size    : %Xh (%u)\nOffset       : %Xh (%u)\nEntry count  : %u\n",
                   (UINT32)tableImage.size(), (UINT32)tableImage.size(),
                   (UINT32)offset, (UINT32)offset,
                   hdr->Num_Entries);

    // Add PSP directory table
    index = model->addItem(0, Types::DirectoryTable, Subtypes::PSPDirectory, name, UString(), info, UByteArray(), tableImage, UByteArray(), Fixed, pspRegionIndex);
    UModelIndex pspHeaderIndex = index;

    const AMD_PSP_DIRECTORY_ENTRY* Entry = (const AMD_PSP_DIRECTORY_ENTRY*)(hdr + 1);
    for (UINTN i = 0; i < hdr->Num_Entries; i++) {
        const UINT8 type = Entry[i].Type;
        const UINT8 subtype = Entry[i].SubProg;
        const UINT16 flags = Entry[i].Flags;
        const UINT64 addr = Entry[i].Address_AddressMode & 0x3fffffffffffffffULL;
        const UINT64 mode = Entry[i].Address_AddressMode >> 62;
        UINT32 size = Entry[i].Size;
        const UString fileName = pspFileName(type, subtype);

        msg(usprintf("%s: Entry%d: ", __FUNCTION__, i) + fileName);

        // Fill directory table

        REGION_INFO psp_entry;
        psp_entry.type = type;
        psp_entry.offset = sizeof(AMD_PSP_DIRECTORY_HEADER) + i * sizeof(AMD_PSP_DIRECTORY_ENTRY);
        psp_entry.length = sizeof(AMD_PSP_DIRECTORY_ENTRY);
        psp_entry.data = psp_directory.data.mid(psp_entry.offset, psp_entry.length);

        // PSP table entry
        UString info = usprintf("Full size    : %Xh (%u)\nOffset       : %Xh (%u)\nFileType     : %Xh\nSubProg      : %Xh\nFlags        : %Xh\nFile Address : %llxh\nAddressMode  : %xh\nFile Size    : %xh (%u)\n",
                                (UINT32)psp_entry.length, (UINT32)psp_entry.length,
                                (UINT32)psp_entry.offset, (UINT32)psp_entry.offset,
                                type, subtype, flags, addr, mode, size, size);

        // Add PSP table entry image tree item
        index = model->addItem(psp_entry.offset, Types::DirectoryTableEntry, Subtypes::PSPDirectory, fileName, UString(), info, UByteArray(), psp_entry.data, UByteArray(), Fixed, pspHeaderIndex);

        // Look for files based on directory table
        UINT64 entry_offset = 0;
        result = amdRelativeOffset(pspHeaderIndex, addr, mode, entry_offset);
        if (result != U_SUCCESS) {
            msg(usprintf("%s: amdRelativeOffset failed for file ", __FUNCTION__) + fileName);
            continue;
        }

        // Some firmwares are broken and set size 0 for ISH directory table
        switch (type) {
            case AMD_FW_RECOVERYAB_A:
            case AMD_FW_RECOVERYAB_B:
            size = 4096;
        }

        if (size == 0x00000000 || size == 0xffffffff) {
            msg(usprintf("%s: Skipping PSP file with no size ", __FUNCTION__) + fileName);
            continue;
        }

        // PSP file entry
        info = usprintf("Full size    : %Xh (%u)\nOffset       : %Xh (%u)\nFileType     : %Xh\nSubProg      : %Xh\nFlags        : %Xh",
                        (UINT32)size, (UINT32)size,
                        (UINT32)entry_offset, (UINT32)entry_offset,
                        type, subtype, flags);

        // Add PSP file tree item
        UModelIndex parentIndex = model->findByBase(entry_offset);
        if (!parentIndex.isValid()) {
            parentIndex = model->findParentOfType(pspRegionIndex, Types::Image);
        }

        result = insertDirectoryFile((UINT32)entry_offset, size, fileName, pspRegionIndex, index);
        if (result != U_SUCCESS) {
            msg(usprintf("%s: Failed to create directory file", __FUNCTION__) + fileName);
        }
        const UModelIndex pspFileIndex = index;

        switch (type) {
            case AMD_FW_RECOVERYAB_A:
            case AMD_FW_RECOVERYAB_B:
                if (level == 1) {
                    // Can be a PSPL2 table or ISH directory
                    result = isValidTable(parentImage, model->base(pspFileIndex));
                    if (result == U_SUCCESS) {
                        result = parsePSPDir(model->base(pspFileIndex), pspFileIndex, index);
                    } else {
                        result = parseISH(model->body(pspFileIndex), pspFileIndex, index);
                    }
                }
                break;
            case AMD_FW_BIOS_TABLE:
                //if (level == 2) {
                    result = parseBiosDir(entry_offset, pspFileIndex, index);
               // }
                break;
        }
        if (result != U_SUCCESS) {
            msg(usprintf("%s: Failed to parse file ", __FUNCTION__) + fileName);
        }
    }

    return U_SUCCESS;
}

// Returns U_SUCCESS when a supported cookie is found at the specified offset
USTATUS FfsParser::isSupportedCookie(const UByteArray & amdImage, const UINT32 offset)
{
    if (offset % 16) {
        return U_INVALID_PARAMETER;
    }

    if ((offset + sizeof(UINT32)) > amdImage.size()) {
        return U_BUFFER_TOO_SMALL;
    }

    const UINT32 *cookie = (const UINT32 *)(amdImage.constData() + offset);

    switch (*cookie) {
        case AMD_PSP_DIRECTORY_HEADER_SIGNATURE:
        case AMD_PSPL2_DIRECTORY_HEADER_SIGNATURE:
        case AMD_PSP_COMBO_DIRECTORY_HEADER_SIGNATURE:
        case AMD_PSP_BHD2_DIRECTORY_HEADER_SIGNATURE:
        case AMD_BIOS_HEADER_SIGNATURE:
        case AMD_BHDL2_HEADER_SIGNATURE:
            return U_SUCCESS;
    }
    return U_UNKNOWN_ITEM_TYPE;
}

// Decodes any supported PSP table found at the specified offset
USTATUS FfsParser::decodePSPTableAny(const UByteArray & amdImage, const UINT32 offset, const UModelIndex & parent, UModelIndex & index)
{
    USTATUS result;

    if (offset % 16) {
        return U_INVALID_PARAMETER;
    }

    result = isValidTable(amdImage, offset);
    if (result != U_SUCCESS) {
        return result;
    }

    const UINT32 *cookie = (const UINT32 *)(amdImage.constData() + offset);
    switch (*cookie) {
        case AMD_PSP_DIRECTORY_HEADER_SIGNATURE:
        case AMD_PSPL2_DIRECTORY_HEADER_SIGNATURE:
            result = parsePSPDir(offset, parent, index);
            break;
        case AMD_PSP_COMBO_DIRECTORY_HEADER_SIGNATURE:
        case AMD_PSP_BHD2_DIRECTORY_HEADER_SIGNATURE:
            result = parseComboDir(offset, parent, index);
            break;
        case AMD_BIOS_HEADER_SIGNATURE:
        case AMD_BHDL2_HEADER_SIGNATURE:
            result = parseBiosDir(offset, parent, index);
            break;
        default:
            result = U_UNKNOWN_ITEM_TYPE;
    }
    return result;
}

USTATUS FfsParser::parseEFTable(const UByteArray & amdImage, const UINT32 efOffset, const UModelIndex & parent, UModelIndex & index)
{
    USTATUS result;
    UString name("Embedded Firmware Table");
    if (efOffset + sizeof(AMD_EMBEDDED_FIRMWARE) > amdImage.size()) {
        return U_INVALID_PARAMETER;
    }
    AMD_EMBEDDED_FIRMWARE* ef_descriptor = (AMD_EMBEDDED_FIRMWARE*)(amdImage.constData() + efOffset);

    msg(usprintf("%s: IMC_Entry 0x%x", __FUNCTION__, ef_descriptor->IMC_Entry));
    msg(usprintf("%s: GEC_Entry 0x%x", __FUNCTION__, ef_descriptor->GEC_Entry));
    msg(usprintf("%s: xHCI_Entry 0x%x", __FUNCTION__, ef_descriptor->xHCI_Entry));
    msg(usprintf("%s: Efs_Generation 0x%x", __FUNCTION__, ef_descriptor->Efs_Generation));
    msg(usprintf("%s: PSP_Directory 0x%x", __FUNCTION__, ef_descriptor->PSP_Directory));
    msg(usprintf("%s: New_PSP_Directory 0x%x", __FUNCTION__, ef_descriptor->New_PSP_Directory));
    msg(usprintf("%s: BIOS0_Entry 0x%x", __FUNCTION__, ef_descriptor->BIOS0_Entry));
    msg(usprintf("%s: BIOS1_Entry 0x%x", __FUNCTION__, ef_descriptor->BIOS1_Entry));
    msg(usprintf("%s: BIOS2_Entry 0x%x", __FUNCTION__, ef_descriptor->BIOS2_Entry));
    msg(usprintf("%s: BIOS3_Entry 0x%x", __FUNCTION__, ef_descriptor->BIOS3_Entry));

    // The specification between SoCs changed a lot, and at this point the
    // SoC/PSP ID isn't known. Attempt to decode all tables without assuming
    // to find a specific type
    if (isSupportedCookie(amdImage, ef_descriptor->PSP_Directory) == U_SUCCESS) {
        result = decodePSPTableAny(amdImage, ef_descriptor->PSP_Directory, parent, index);
        if (result != U_SUCCESS) {
            return result;
        }
    }
    if (isSupportedCookie(amdImage, ef_descriptor->New_PSP_Directory) == U_SUCCESS) {
        result = decodePSPTableAny(amdImage, ef_descriptor->New_PSP_Directory, parent, index);
        if (result != U_SUCCESS) {
            return result;
        }
    }
    if (isSupportedCookie(amdImage, ef_descriptor->BIOS0_Entry) == U_SUCCESS) {
        result = decodePSPTableAny(amdImage, ef_descriptor->BIOS0_Entry, parent, index);
        if (result != U_SUCCESS) {
            return result;
        }
    }
    if (isSupportedCookie(amdImage, ef_descriptor->BIOS1_Entry) == U_SUCCESS) {
        result = decodePSPTableAny(amdImage, ef_descriptor->BIOS1_Entry, parent, index);
        if (result != U_SUCCESS) {
            return result;
        }
    }
    if (isSupportedCookie(amdImage, ef_descriptor->BIOS2_Entry) == U_SUCCESS) {
        result = decodePSPTableAny(amdImage, ef_descriptor->BIOS2_Entry, parent, index);
        if (result != U_SUCCESS) {
            return result;
        }
    }
    if (isSupportedCookie(amdImage, ef_descriptor->BIOS3_Entry) == U_SUCCESS) {
        result = decodePSPTableAny(amdImage, ef_descriptor->BIOS3_Entry, parent, index);
        if (result != U_SUCCESS) {
            return result;
        }
    }
    return U_SUCCESS;
}


/*
 * Creates the OSI Fletcher checksum. See 8473-1, Appendix C, section C.3.
 * The checksum field of the passed PDU does not need to be reset to zero.
 *
 * The "Fletcher Checksum" was proposed in a paper by John G. Fletcher of
 * Lawrence Livermore Labs.  The Fletcher Checksum was proposed as an
 * alternative to cyclical redundancy checks because it provides error-
 * detection properties similar to cyclical redundancy checks but at the
 * cost of a simple summation technique.  Its characteristics were first
 * published in IEEE Transactions on Communications in January 1982.  One
 * version has been adopted by ISO for use in the class-4 transport layer
 * of the network protocol.
 *
 * This program expects:
 *    stdin:    The input file to compute a checksum for.  The input file
 *              not be longer than 256 bytes.
 *    stdout:   Copied from the input file with the Fletcher's Checksum
 *              inserted 8 bytes after the beginning of the file.
 *    stderr:   Used to print out error messages.
 */
UINT32 FfsParser::fletcher32(const UByteArray &Image)
{
	UINT32 c0;
	UINT32 c1;
	UINT32 checksum;
	INTN index;
	const UINT16 *pptr = (const UINT16 *)Image.constData();

	INTN length = Image.size() / 2;

	c0 = 0xFFFF;
	c1 = 0xFFFF;

	while (length) {
		index = length >= 359 ? 359 : length;
		length -= index;
		do {
			c0 += *(pptr++);
			c1 += c0;
		} while (--index);
		c0 = (c0 & 0xFFFF) + (c0 >> 16);
		c1 = (c1 & 0xFFFF) + (c1 >> 16);
	}

	/* Sums[0,1] mod 64K + overflow */
	c0 = (c0 & 0xFFFF) + (c0 >> 16);
	c1 = (c1 & 0xFFFF) + (c1 >> 16);
	checksum = (c1 << 16) | c0;

	return checksum;
}

USTATUS FfsParser::parseAMDImage(const UByteArray & amdImage, const UINT32 localOffset, const UModelIndex & parent, UModelIndex & index)
{
    AMD_EMBEDDED_FIRMWARE* ef_descriptor = NULL;
    USTATUS result;
    UINT32 ProbeOffset;

    // Probe all possible locations for the header
    for (ProbeOffset = AMD_EMBEDDED_FIRMWARE_OFFSET; (ProbeOffset + sizeof(AMD_EMBEDDED_FIRMWARE)) < amdImage.size(); ProbeOffset += 0x80000) {
        // Store the beginning of descriptor as descriptor base address
        ef_descriptor = (AMD_EMBEDDED_FIRMWARE*)(amdImage.constData() + ProbeOffset);

        // Check descriptor signature
        if (ef_descriptor->Signature == AMD_EMBEDDED_FIRMWARE_SIGNATURE) {
            msg(usprintf("%s: Embedded firmware table found at offset 0x%x", __FUNCTION__, ProbeOffset));
            // The Embedded Firmware table has no checksum, thus test if there's a valid pointer
            if (isSupportedCookie(amdImage, ef_descriptor->PSP_Directory) != U_SUCCESS &&
                isSupportedCookie(amdImage, ef_descriptor->New_PSP_Directory) != U_SUCCESS &&
                isSupportedCookie(amdImage, ef_descriptor->BIOS0_Entry) != U_SUCCESS &&
                isSupportedCookie(amdImage, ef_descriptor->BIOS1_Entry) != U_SUCCESS &&
                isSupportedCookie(amdImage, ef_descriptor->BIOS2_Entry) != U_SUCCESS &&
                isSupportedCookie(amdImage, ef_descriptor->BIOS3_Entry) != U_SUCCESS) {
                ef_descriptor = NULL;
                continue;
            }
            break;
        }
    }
    if (ef_descriptor == NULL) {
        msg(usprintf("%s: Embedded firmware table not found", __FUNCTION__));
        return U_ITEM_NOT_FOUND;
    }

    // AMD image
    UString name("AMD image");
    UString info = usprintf("Full size: %Xh (%u)\n",
                            (UINT32)amdImage.size(), (UINT32)amdImage.size());

    // Set image base
    imageBase = model->base(parent) + localOffset;

    // Add AMD image tree item
    index = model->addItem(localOffset, Types::Image, Subtypes::AmdImage, name, UString(), info, UByteArray(), amdImage, UByteArray(), Fixed, parent);
    UModelIndex parentIndex = index;

    result = parseEFTable(amdImage, ProbeOffset, parentIndex, index);
    if (result != U_SUCCESS) {
        return result;
    }
#if 0
    // Sort regions in ascending order
    std::sort(regions.begin(), regions.end());

    // Add offsets of actual regions
    for (size_t i = 0; i < regions.size(); i++) {
        if (regions[i].type != Subtypes::ZeroPadding && regions[i].type != Subtypes::OnePadding && regions[i].type != Subtypes::DataPadding)
            info += "\n" + itemSubtypeToUString(Types::Region, regions[i].type)
            + usprintf(" region offset: %Xh", regions[i].offset + localOffset);
    }
#endif

    return result;
}
