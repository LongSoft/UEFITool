/* meparser.cpp

Copyright (c) 2016, Nikolaj Schlej. All rights reserved.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#include <inttypes.h>
#include <map>

#include "meparser.h"
#include "parsingdata.h"
#include "utility.h"

#ifdef U_ENABLE_ME_PARSING_SUPPORT

UString meBpdtEntryTypeToUString(UINT16 type) {
    switch (type) {
        case 0:    return UString("OEM SMIP");
        case 1:    return UString("CSE RBE");
        case 2:    return UString("CSE BUP");
        case 3:    return UString("uCode");
        case 4:    return UString("IBB");
        case 5:    return UString("S-BPDT");
        case 6:    return UString("OBB");
        case 7:    return UString("CSE Main");
        case 8:    return UString("ISH");
        case 9:    return UString("CSE IDLM");
        case 10:   return UString("IFP Override");
        case 11:   return UString("Debug Tokens");
        case 12:   return UString("USF Phy Config");
        case 13:   return UString("USF GPP LUN ID");
        case 14:   return UString("PMC");
        case 15:   return UString("iUnit");
        case 16:   return UString("NVM Config");
        case 17:   return UString("UEP");
        case 18:   return UString("WLAN uCode");
        case 19:   return UString("LOCL Sprites");
        case 20:   return UString("OEM Key Manifest");
        case 21:   return UString("Defaults/FITC.cfg");
        case 22:   return UString("PAVP");
        case 23:   return UString("TCSS FW IOM");
        case 24:   return UString("TCSS FW PHY");
        case 25:   return UString("TCSS TBT");
        default:   return usprintf("Unknown %u", type);
    }
}

UString meExtensionTypeToUstring(UINT32 type) {
    switch (type) {
        case 0:    return UString("System Info");
        case 1:    return UString("Init Script");
        case 2:    return UString("Feature Permissions");
        case 3:    return UString("Partition Info");
        case 4:    return UString("Shared Lib Attributes");
        case 5:    return UString("Process Attributes");
        case 6:    return UString("Thread Attributes");
        case 7:    return UString("Device Type");
        case 8:    return UString("MMIO Range");
        case 9:    return UString("Spec File Producer");
        case 10:   return UString("Module Attributes");
        case 11:   return UString("Locked Ranges");
        case 12:   return UString("Client System Info");
        case 13:   return UString("User Info");
        case 14:   return UString("Key Manifest");
        case 15:   return UString("Signed Package Info");
        case 16:   return UString("Anto-cloning SKU ID");
        case 18:   return UString("Intel IMR Info");
        case 20:   return UString("RCIP Info");
        case 21:   return UString("Secure Token");
        case 22:   return UString("IFWI Partition Manifest");
        default:   return usprintf("Unknown %u", type);
    }
}

struct ME_FPT_PARTITION_INFO {
    ME_FPT_ENTRY ptEntry;
    UINT8 type;
    UModelIndex index;
    friend bool operator< (const ME_FPT_PARTITION_INFO & lhs, const ME_FPT_PARTITION_INFO & rhs){ return lhs.ptEntry.Offset < rhs.ptEntry.Offset; }
};

struct ME_BPDT_PARTITION_INFO {
    ME_BPDT_ENTRY ptEntry;
    UINT8 type;
    UModelIndex index;
    friend bool operator< (const ME_BPDT_PARTITION_INFO & lhs, const ME_BPDT_PARTITION_INFO & rhs){ return lhs.ptEntry.Offset < rhs.ptEntry.Offset; }
};

struct ME_CPD_PARTITION_INFO {
    ME_BPDT_CPD_ENTRY ptEntry;
    UINT8 type;
    UModelIndex index;
    friend bool operator< (const ME_CPD_PARTITION_INFO & lhs, const ME_CPD_PARTITION_INFO & rhs){ return lhs.ptEntry.Offset.Offset < rhs.ptEntry.Offset.Offset; }
};

USTATUS MeParser::parseMeRegionBody(const UModelIndex & index)
{
    // Sanity check
    if (!index.isValid())
        return U_INVALID_PARAMETER;

    // Obtain ME region
    UByteArray meRegion = model->body(index);

    // Check region size
    if ((UINT32)meRegion.size() < ME_ROM_BYPASS_VECTOR_SIZE + sizeof(UINT32)) {
        msg(usprintf("%s: ME region too small to fit ROM bypass vector", __FUNCTION__), index);
        return U_INVALID_ME_PARTITION_TABLE;
    }

    // Check ME signature to determine it's version
    // ME v11 and older layout
    if (meRegion.left(sizeof(UINT32)) == ME_FPT_HEADER_SIGNATURE || meRegion.mid(ME_ROM_BYPASS_VECTOR_SIZE, sizeof(UINT32)) == ME_FPT_HEADER_SIGNATURE) {
        UModelIndex ptIndex;
        return parseFptRegion(meRegion, index, ptIndex);
    }

    // CannonLake 1.6+ layout (IFWI)
    // Check region size
    if ((UINT32)meRegion.size() < sizeof(ME_IFWI_LAYOUT_HEADER)) {
        msg(usprintf("%s: ME region too small to fit IFWI layout header", __FUNCTION__), index);
        return U_INVALID_ME_PARTITION_TABLE;
    }

    const ME_IFWI_LAYOUT_HEADER* regionHeader = (const ME_IFWI_LAYOUT_HEADER*)meRegion.constData();
    // Check region size
    if ((UINT32)meRegion.size() < regionHeader->DataPartitionOffset + sizeof(UINT32)) {
        msg(usprintf("%s: ME region too small to fit IFWI layout header", __FUNCTION__), index);
        return U_INVALID_ME_PARTITION_TABLE;
    }
    // Data partition always points to FPT header
    if (meRegion.mid(regionHeader->DataPartitionOffset, sizeof(UINT32)) == ME_FPT_HEADER_SIGNATURE) {
        UModelIndex ptIndex;
        return parseIfwiRegion(meRegion, index, ptIndex);
    }

    // Something else entirely
    msg(usprintf("%s: unknown ME region format", __FUNCTION__), index);
    return U_INVALID_ME_PARTITION_TABLE;
}

USTATUS MeParser::parseIfwiRegion(const UByteArray & region, const UModelIndex & parent, UModelIndex & index)
{
    // Add header
    UByteArray header = region.left(sizeof(ME_IFWI_LAYOUT_HEADER));
    const ME_IFWI_LAYOUT_HEADER* ifwiHeader = (const ME_IFWI_LAYOUT_HEADER*)region.constData();

    UString name = UString("IFWI header");
    UString info = usprintf("Full size: %Xh (%u)\n"
                            "Data  partition offset: %Xh\nData  partition length: %Xh\n"
                            "Boot1 partition offset: %Xh\nBoot1 partition length: %Xh\n"
                            "Boot2 partition offset: %Xh\nBoot2 partition length: %Xh\n"
                            "Boot3 partition offset: %Xh\nBoot3 partition length: %Xh",
                            header.size(), header.size(),
                            ifwiHeader->DataPartitionOffset, ifwiHeader->DataPartitionSize,
                            ifwiHeader->Boot1Offset, ifwiHeader->Boot1Size,
                            ifwiHeader->Boot2Offset, ifwiHeader->Boot2Size,
                            ifwiHeader->Boot3Offset, ifwiHeader->Boot3Size);
    // Add tree item
    index = model->addItem(0, Types::IfwiHeader, 0, name, UString(), info, UByteArray(), header, UByteArray(), Fixed, parent);

    // TODO: this requires better parsing using a similar approach as in other things: get all, sort, check for paddings/intersections

    // Add padding after header
    if (ifwiHeader->DataPartitionOffset > sizeof(ME_IFWI_LAYOUT_HEADER)) {
        UByteArray padding = region.mid(sizeof(ME_IFWI_LAYOUT_HEADER), ifwiHeader->DataPartitionOffset - sizeof(ME_IFWI_LAYOUT_HEADER));
        name = UString("Padding");
        info = usprintf("Full size: %Xh (%u)", padding.size(), padding.size());
        // Add tree item
        model->addItem(sizeof(ME_IFWI_LAYOUT_HEADER), Types::Padding, getPaddingType(padding), name, UString(), info, UByteArray(), padding, UByteArray(), Fixed, parent);
    }

    // Add data partition
    UByteArray dataPartition = region.mid(ifwiHeader->DataPartitionOffset, ifwiHeader->DataPartitionSize);
    name = UString("Data partition");
    info = usprintf("Full size: %Xh (%u)", dataPartition.size(), dataPartition.size());
    UModelIndex dataPartitionIndex = model->addItem(ifwiHeader->DataPartitionOffset, Types::IfwiPartition, Subtypes::DataIfwiPartition, name, UString(), info, UByteArray(), dataPartition, UByteArray(), Fixed, parent);
    UModelIndex dataPartitionFptRegionIndex;
    parseFptRegion(dataPartition, dataPartitionIndex, dataPartitionFptRegionIndex);

    // Add padding after data partition
    if (ifwiHeader->Boot1Offset > ifwiHeader->DataPartitionOffset + ifwiHeader->DataPartitionSize) {
        UByteArray padding = region.mid(ifwiHeader->DataPartitionOffset + ifwiHeader->DataPartitionSize, ifwiHeader->Boot1Offset - ifwiHeader->DataPartitionOffset + ifwiHeader->DataPartitionSize);
        name = UString("Padding");
        info = usprintf("Full size: %Xh (%u)", padding.size(), padding.size());
        // Add tree item
        model->addItem(ifwiHeader->DataPartitionOffset + ifwiHeader->DataPartitionSize, Types::Padding, getPaddingType(padding), name, UString(), info, UByteArray(), padding, UByteArray(), Fixed, parent);
    }
    else if (ifwiHeader->Boot1Offset < ifwiHeader->DataPartitionOffset + ifwiHeader->DataPartitionSize) {
        msg(usprintf("%s: invalid Boot1 partition offset", __FUNCTION__), parent);
        return U_INVALID_ME_PARTITION_TABLE;
    }

    // Add Boot1 partition
    UByteArray bpdt1Partition = region.mid(ifwiHeader->Boot1Offset, ifwiHeader->Boot1Size);
    name = UString("Boot1 partition");
    info = usprintf("Full size: %Xh (%u)", bpdt1Partition.size(), bpdt1Partition.size());
    UModelIndex bpdt1PartitionIndex = model->addItem(ifwiHeader->Boot1Offset, Types::IfwiPartition, Subtypes::BootIfwiPartition, name, UString(), info, UByteArray(), bpdt1Partition, UByteArray(), Fixed, parent);
    UModelIndex bpdt1PartitionBpdtRegionIndex;
    parseBpdtRegion(bpdt1Partition, bpdt1PartitionIndex, bpdt1PartitionBpdtRegionIndex);

    // Add padding after Boot1 partition
    if (ifwiHeader->Boot2Offset > ifwiHeader->Boot1Offset + ifwiHeader->Boot1Size) {
        UByteArray padding = region.mid(ifwiHeader->Boot1Offset + ifwiHeader->Boot1Size, ifwiHeader->Boot2Offset - ifwiHeader->Boot1Offset + ifwiHeader->Boot1Size);
        name = UString("Padding");
        info = usprintf("Full size: %Xh (%u)", padding.size(), padding.size());
        // Add tree item
        model->addItem(ifwiHeader->Boot1Offset + ifwiHeader->Boot1Size, Types::Padding, getPaddingType(padding), name, UString(), info, UByteArray(), padding, UByteArray(), Fixed, parent);
    }
    else if (ifwiHeader->Boot2Offset < ifwiHeader->Boot1Offset + ifwiHeader->Boot1Size) {
        msg(usprintf("%s: invalid Boot2 partition offset", __FUNCTION__), parent);
        return U_INVALID_ME_PARTITION_TABLE;
    }

    // Add Boot2 partition
    UByteArray bpdt2Partition = region.mid(ifwiHeader->Boot2Offset, ifwiHeader->Boot2Size);
    name = UString("Boot2 partition");
    info = usprintf("Full size: %Xh (%u)", bpdt2Partition.size(), bpdt2Partition.size());
    UModelIndex bpdt2PartitionIndex = model->addItem(ifwiHeader->Boot2Offset, Types::IfwiPartition, Subtypes::BootIfwiPartition, name, UString(), info, UByteArray(), bpdt2Partition, UByteArray(), Fixed, parent);
    UModelIndex bpdt2PartitionBpdtRegionIndex;
    parseBpdtRegion(bpdt2Partition, bpdt2PartitionIndex, bpdt2PartitionBpdtRegionIndex);

    // TODO: add Boot3 if needed
    // Add padding at the end
    if ((UINT32)region.size() > ifwiHeader->Boot2Offset + ifwiHeader->Boot2Size) {
        UByteArray padding = region.mid(ifwiHeader->Boot2Offset + ifwiHeader->Boot2Size, (UINT32)region.size() - ifwiHeader->Boot2Offset + ifwiHeader->Boot2Size);
        name = UString("Padding");
        info = usprintf("Full size: %Xh (%u)", padding.size(), padding.size());
        // Add tree item
        model->addItem(ifwiHeader->Boot2Offset + ifwiHeader->Boot2Size, Types::Padding, getPaddingType(padding), name, UString(), info, UByteArray(), padding, UByteArray(), Fixed, parent);
    }
    else if ((UINT32)region.size() < ifwiHeader->Boot2Offset + ifwiHeader->Boot2Size) {
        msg(usprintf("%s: Boot2 partition is located outside of the region", __FUNCTION__), parent);
        return U_INVALID_ME_PARTITION_TABLE;
    }

    return U_SUCCESS;
}

USTATUS MeParser::parseFptRegion(const UByteArray & region, const UModelIndex & parent, UModelIndex & index)
{
    // Check region size
    if ((UINT32)region.size() < sizeof(ME_FPT_HEADER)) {
        msg(usprintf("%s: region too small to fit FPT header", __FUNCTION__), parent);
        return U_INVALID_ME_PARTITION_TABLE;
    }
    
    // Populate partition table header
    const ME_FPT_HEADER* ptHeader = (const ME_FPT_HEADER*)region.constData();
    UINT32 romBypassVectorSize = 0;
    if (region.left(sizeof(UINT32)) != ME_FPT_HEADER_SIGNATURE) {
        // Adjust the header to skip ROM bypass vector
        romBypassVectorSize = ME_ROM_BYPASS_VECTOR_SIZE;
        ptHeader = (const ME_FPT_HEADER*)(region.constData() + romBypassVectorSize);
    }
    
    // Check region size again
    UINT32 ptBodySize = ptHeader->NumEntries * sizeof(ME_FPT_ENTRY);
    UINT32 ptSize = romBypassVectorSize + sizeof(ME_FPT_HEADER) + ptBodySize;
    if ((UINT32)region.size() < ptSize) {
        msg(usprintf("%s: ME region too small to fit partition table", __FUNCTION__), parent);
        return U_INVALID_ME_PARTITION_TABLE;
    }
    
    // Recalculate checksum
    UByteArray tempHeader = UByteArray((const char*)ptHeader, sizeof(ME_FPT_HEADER));
    ME_FPT_HEADER* tempPtHeader = (ME_FPT_HEADER*)tempHeader.data();
    tempPtHeader->Checksum = 0;
    UINT8 calculated = calculateChecksum8((const UINT8*)tempPtHeader, sizeof(ME_FPT_HEADER));
    bool msgInvalidPtHeaderChecksum = (calculated != ptHeader->Checksum);
    
    // Get info
    UByteArray header = region.left(romBypassVectorSize + sizeof(ME_FPT_HEADER));
    UByteArray body = region.mid(header.size(), ptBodySize);
    
    UString name = UString("FPT partition table");
    UString info = usprintf("Full size: %Xh (%u)\nHeader size: %Xh (%u)\nBody size: %Xh (%u)\nNumber of entries: %u\nHeader version: %02Xh\nEntry version: %02Xh\n"
                            "Header length: %02Xh\nTicks to add: %04Xh\nTokens to add: %04Xh\nUMA size: %Xh\nFlash layout: %Xh\nFITC version: %u.%u.%u.%u\nChecksum: %02Xh, ",
                            ptSize, ptSize,
                            header.size(), header.size(),
                            ptBodySize, ptBodySize,
                            ptHeader->NumEntries,
                            ptHeader->HeaderVersion,
                            ptHeader->EntryVersion,
                            ptHeader->HeaderLength,
                            ptHeader->TicksToAdd,
                            ptHeader->TokensToAdd,
                            ptHeader->UmaSize,
                            ptHeader->FlashLayout,
                            ptHeader->FitcMajor, ptHeader->FitcMinor, ptHeader->FitcHotfix, ptHeader->FitcBuild,
                            ptHeader->Checksum) + (ptHeader->Checksum == calculated ? UString("valid") : usprintf("invalid, should be %02Xh", calculated));
    
    // Add tree item
    index = model->addItem(0, Types::FptStore, 0, name, UString(), info, header, body, UByteArray(), Fixed, parent);
    
    // Show messages
    if (msgInvalidPtHeaderChecksum) {
        msg(usprintf("%s: FPT partition table header checksum is invalid", __FUNCTION__), index);
    }

    // Add partition table entries
    std::vector<ME_FPT_PARTITION_INFO> partitions;
    UINT32 offset = header.size();
    const ME_FPT_ENTRY* firstPtEntry = (const ME_FPT_ENTRY*)(region.constData() + offset);
    for (UINT8 i = 0; i < ptHeader->NumEntries; i++) {
        // Populate entry header
        const ME_FPT_ENTRY* ptEntry = firstPtEntry + i;
        
        // Get info
        name = usprintf("%c%c%c%c", ptEntry->PartitionName[0], ptEntry->PartitionName[1], ptEntry->PartitionName[2], ptEntry->PartitionName[3]);
        info = usprintf("Full size: %Xh (%u)\nPartition offset: %Xh\nPartition length: %Xh\nPartition type: %02Xh",
                        sizeof(ME_FPT_ENTRY), sizeof(ME_FPT_ENTRY),
                        ptEntry->Offset,
                        ptEntry->Length,
                        ptEntry->PartitionType);
        
        // Add tree item
        const UINT8 type = (ptEntry->Offset != 0 && ptEntry->Offset != 0xFFFFFFFF && ptEntry->Length != 0 && ptEntry->EntryValid != 0xFF) ? Subtypes::ValidFptEntry : Subtypes::InvalidFptEntry;
        UModelIndex entryIndex = model->addItem(offset, Types::FptEntry, type, name, UString(), info, UByteArray(), UByteArray((const char*)ptEntry, sizeof(ME_FPT_ENTRY)), UByteArray(), Fixed, index);
        
        // Adjust offset
        offset += sizeof(ME_FPT_ENTRY);
        
        // Add valid partitions
        if (type == Subtypes::ValidFptEntry) { // Skip absent and invalid partitions
            // Add to partitions vector
            ME_FPT_PARTITION_INFO partition;
            partition.type = Types::FptPartition;
            partition.ptEntry = *ptEntry;
            partition.index = entryIndex;
            partitions.push_back(partition);
        }
    }

make_partition_table_consistent:
    // Sort partitions by offset
    std::sort(partitions.begin(), partitions.end());
    
    // Check for intersections and paddings between partitions
    ME_FPT_PARTITION_INFO padding;
    
    // Check intersection with the partition table header
    if (partitions.front().ptEntry.Offset < ptSize) {
        msg(usprintf("%s: ME partition has intersection with ME partition table, skipped", __FUNCTION__),
            partitions.front().index);
        partitions.erase(partitions.begin());
        goto make_partition_table_consistent;
    }
    // Check for padding between partition table and the first partition
    else if (partitions.front().ptEntry.Offset > ptSize) {
        padding.ptEntry.Offset = ptSize;
        padding.ptEntry.Length = partitions.front().ptEntry.Offset - ptSize;
        padding.type = Types::Padding;
        partitions.insert(partitions.begin(), padding);
    }
    // Check for intersections/paddings between partitions
    for (size_t i = 1; i < partitions.size(); i++) {
        UINT32 previousPartitionEnd = partitions[i - 1].ptEntry.Offset + partitions[i - 1].ptEntry.Length;
        
        // Check that current region is fully present in the image
        if ((UINT32)partitions[i].ptEntry.Offset + (UINT32)partitions[i].ptEntry.Length > (UINT32)region.size()) {
            if ((UINT32)partitions[i].ptEntry.Offset >= (UINT32)region.size()) {
                msg(usprintf("%s: FPT partition is located outside of the opened image, skipped", __FUNCTION__), partitions[i].index);
                partitions.erase(partitions.begin() + i);
                goto make_partition_table_consistent;
            }
            else {
                msg(usprintf("%s: FPT partition can't fit into the region, truncated", __FUNCTION__), partitions[i].index);
                partitions[i].ptEntry.Length = (UINT32)region.size() - (UINT32)partitions[i].ptEntry.Offset;
            }
        }

        // Check for intersection with previous partition
        if (partitions[i].ptEntry.Offset < previousPartitionEnd) {
            // Check if current partition is located inside previous one
            if (partitions[i].ptEntry.Offset + partitions[i].ptEntry.Length <= previousPartitionEnd) {
                msg(usprintf("%s: FPT partition is located inside another FPT partition, skipped", __FUNCTION__),
                    partitions[i].index);
                partitions.erase(partitions.begin() + i);
                goto make_partition_table_consistent;
            }
            else {
                msg(usprintf("%s: FPT partition intersects with prevous one, skipped", __FUNCTION__),
                    partitions[i].index);
                partitions.erase(partitions.begin() + i);
                goto make_partition_table_consistent;
            }
        }
        
        // Check for padding between current and previous partitions
        else if (partitions[i].ptEntry.Offset > previousPartitionEnd) {
            padding.ptEntry.Offset = previousPartitionEnd;
            padding.ptEntry.Length = partitions[i].ptEntry.Offset - previousPartitionEnd;
            padding.type = Types::Padding;
            std::vector<ME_FPT_PARTITION_INFO>::iterator iter = partitions.begin();
            std::advance(iter, i);
            partitions.insert(iter, padding);
        }
    }
    // Check for padding after the last region
    if ((UINT32)partitions.back().ptEntry.Offset + (UINT32)partitions.back().ptEntry.Length < (UINT32)region.size()) {
        padding.ptEntry.Offset = partitions.back().ptEntry.Offset + partitions.back().ptEntry.Length;
        padding.ptEntry.Length = region.size() - padding.ptEntry.Offset;
        padding.type = Types::Padding;
        partitions.push_back(padding);
    }
    
    // Partition map is consistent
    for (size_t i = 0; i < partitions.size(); i++) {
        UByteArray partition = region.mid(partitions[i].ptEntry.Offset, partitions[i].ptEntry.Length);
        if (partitions[i].type == Types::FptPartition) {
            UModelIndex partitionIndex;
            // Get info
            name = usprintf("%c%c%c%c", partitions[i].ptEntry.PartitionName[0], partitions[i].ptEntry.PartitionName[1], partitions[i].ptEntry.PartitionName[2], partitions[i].ptEntry.PartitionName[3]);
            info = usprintf("Full size: %Xh (%u)\nPartition type: %02Xh\n",
                partition.size(), partition.size(),
                partitions[i].ptEntry.PartitionType);

            // Add tree item
            UINT8 type = Subtypes::CodeFptPartition + partitions[i].ptEntry.PartitionType;
            partitionIndex = model->addItem(partitions[i].ptEntry.Offset, Types::FptPartition, type, name, UString(), info, UByteArray(), partition, UByteArray(), Fixed, parent);
            if (type == Subtypes::CodeFptPartition && partition.left(sizeof(UINT32)) == ME_CPD_SIGNATURE) {
                // Parse conde partition contents
                UModelIndex ptIndex;
                parseCodePartitionDirectory(partition, partitions[i].ptEntry.Offset, partitionIndex, ptIndex);
            }
        }
        else if (partitions[i].type == Types::Padding) {
            // Get info
            name = UString("Padding");
            info = usprintf("Full size: %Xh (%u)", partition.size(), partition.size());
            
            // Add tree item
            model->addItem(partitions[i].ptEntry.Offset, Types::Padding, getPaddingType(partition), name, UString(), info, UByteArray(), partition, UByteArray(), Fixed, parent);
        }
    }
    
    return U_SUCCESS;
}

USTATUS MeParser::parseBpdtRegion(const UByteArray & region, const UModelIndex & parent, UModelIndex & index)
{
    // Check region size
    if ((UINT32)region.size() < sizeof(ME_BPDT_HEADER)) {
        msg(usprintf("%s: BPDT region too small to fit partition table header", __FUNCTION__), parent);
        return U_INVALID_ME_PARTITION_TABLE;
    }
    
    // Populate partition table header
    const ME_BPDT_HEADER* ptHeader = (const ME_BPDT_HEADER*)region.constData();
    
    // Check region size again
    UINT32 ptBodySize = ptHeader->NumEntries * sizeof(ME_BPDT_ENTRY);
    UINT32 ptSize = sizeof(ME_BPDT_HEADER) + ptBodySize;
    if ((UINT32)region.size() < ptSize) {
        msg(usprintf("%s: BPDT region too small to fit BPDT partition table", __FUNCTION__), parent);
        return U_INVALID_ME_PARTITION_TABLE;
    }
    
    // Get info
    UByteArray header = region.left(sizeof(ME_BPDT_HEADER));
    UByteArray body = region.mid(sizeof(ME_BPDT_HEADER), ptBodySize);
    
    UString name = UString("BPDT partition table");
    UString info = usprintf("Full size: %Xh (%u)\nHeader size: %Xh (%u)\nBody size: %Xh (%u)\nNumber of entries: %u\nVersion: %Xh\n"
                            "IFWI version: %Xh\nFITC version: %u.%u.%u.%u",
                            ptSize, ptSize,
                            header.size(), header.size(),
                            ptBodySize, ptBodySize,
                            ptHeader->NumEntries,
                            ptHeader->Version,
                            ptHeader->IfwiVersion,
                            ptHeader->FitcMajor, ptHeader->FitcMinor, ptHeader->FitcHotfix, ptHeader->FitcBuild);

    // Add tree item
    index = model->addItem(0, Types::BpdtStore, 0, name, UString(), info, header, body, UByteArray(), Fixed, parent);
    
    // Adjust offset
    UINT32 offset = sizeof(ME_FPT_HEADER);
    
    // Add partition table entries
    std::vector<ME_BPDT_PARTITION_INFO> partitions;
    const ME_BPDT_ENTRY* firstPtEntry = (const ME_BPDT_ENTRY*)(region.constData() + sizeof(ME_BPDT_HEADER));
    for (UINT8 i = 0; i < ptHeader->NumEntries; i++) {
        // Populate entry header
        const ME_BPDT_ENTRY* ptEntry = firstPtEntry + i;
        
        // Get info
        name = meBpdtEntryTypeToUString(ptEntry->Type);
        info = usprintf("Full size: %Xh (%u)\nType: %Xh\nPartition offset: %Xh\nPartition length: %Xh",
                        sizeof(ME_BPDT_ENTRY), sizeof(ME_BPDT_ENTRY),
                        ptEntry->Type,
                        ptEntry->Offset,
                        ptEntry->Length) +
                        UString("\nSplit sub-partition first part: ") + (ptEntry->SplitSubPartitionFirstPart ? "Yes" : "No") +
                        UString("\nSplit sub-partition second part: ") + (ptEntry->SplitSubPartitionSecondPart ? "Yes" : "No") +
                        UString("\nCode sub-partition: ") + (ptEntry->CodeSubPartition ? "Yes" : "No") +
                        UString("\nUMA cachable: ") + (ptEntry->UmaCachable ? "Yes" : "No");
        
        // Add tree item
        UModelIndex entryIndex = model->addItem(offset, Types::BpdtEntry, 0, name, UString(), info, UByteArray(), UByteArray((const char*)ptEntry, sizeof(ME_BPDT_ENTRY)), UByteArray(), Fixed, index);
        
        // Adjust offset
        offset += sizeof(ME_BPDT_ENTRY);
        
        if (ptEntry->Offset != 0 && ptEntry->Offset != 0xFFFFFFFF && ptEntry->Length != 0) {
            // Add to partitions vector
            ME_BPDT_PARTITION_INFO partition;
            partition.type = Types::FptPartition;
            partition.ptEntry = *ptEntry;
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
        model->addItem(ptSize, Types::Padding, getPaddingType(partition), name, UString(), info, UByteArray(), partition, UByteArray(), Fixed, parent);
        
        return U_SUCCESS;
    }

make_partition_table_consistent:
    // Sort partitions by offset
    std::sort(partitions.begin(), partitions.end());

    // Check for intersections and paddings between partitions
    ME_BPDT_PARTITION_INFO padding;

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
        padding.ptEntry.Length = partitions.front().ptEntry.Offset - padding.ptEntry.Offset;
        padding.type = Types::Padding;
        partitions.insert(partitions.begin(), padding);
    }
    // Check for intersections/paddings between partitions
    for (size_t i = 1; i < partitions.size(); i++) {
        UINT32 previousPartitionEnd = partitions[i - 1].ptEntry.Offset + partitions[i - 1].ptEntry.Length;

        // Check that partition is fully present in the image
        if ((UINT64)partitions[i].ptEntry.Offset + (UINT64)partitions[i].ptEntry.Length > (UINT64)region.size()) {
            if ((UINT64)partitions[i].ptEntry.Offset >= (UINT64)region.size()) {
                msg(usprintf("%s: BPDT partition is located outside of the opened image, skipped", __FUNCTION__), partitions[i].index);
                partitions.erase(partitions.begin() + i);
                goto make_partition_table_consistent;
            }
            else {
                msg(usprintf("%s: BPDT partition can't fit into it's region, truncated", __FUNCTION__), partitions[i].index);
                partitions[i].ptEntry.Length = (UINT32)region.size() - (UINT32)partitions[i].ptEntry.Offset;
            }
        }

        // Check for intersection with previous partition
        if (partitions[i].ptEntry.Offset < previousPartitionEnd) {
            // Check if current partition is located inside previous one
            if (partitions[i].ptEntry.Offset + partitions[i].ptEntry.Length <= previousPartitionEnd) {
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
            padding.ptEntry.Length = partitions[i].ptEntry.Offset - previousPartitionEnd;
            padding.type = Types::Padding;
            std::vector<ME_BPDT_PARTITION_INFO>::iterator iter = partitions.begin();
            std::advance(iter, i);
            partitions.insert(iter, padding);
        }
    }

    // Check for padding after the last region
    if ((UINT64)partitions.back().ptEntry.Offset + (UINT64)partitions.back().ptEntry.Length < (UINT64)region.size()) {
        padding.ptEntry.Offset = partitions.back().ptEntry.Offset + partitions.back().ptEntry.Length;
        padding.ptEntry.Length = region.size() - padding.ptEntry.Offset;
        padding.type = Types::Padding;
        partitions.push_back(padding);
    }

    // Partition map is consistent
    for (size_t i = 0; i < partitions.size(); i++) {
        if (partitions[i].type == Types::FptPartition) {
            // Get info
            UString name = meBpdtEntryTypeToUString(partitions[i].ptEntry.Type);
            UByteArray partition = region.mid(partitions[i].ptEntry.Offset, partitions[i].ptEntry.Length);
            UByteArray signature = partition.left(sizeof(UINT32));

            UString info = usprintf("Full size: %Xh (%u)\nType: %Xh",
                partition.size(), partition.size(),
                partitions[i].ptEntry.Type) +
                UString("\nSplit sub-partition first part: ") + (partitions[i].ptEntry.SplitSubPartitionFirstPart ? "Yes" : "No") +
                UString("\nSplit sub-partition second part: ") + (partitions[i].ptEntry.SplitSubPartitionSecondPart ? "Yes" : "No") +
                UString("\nCode sub-partition: ") + (partitions[i].ptEntry.CodeSubPartition ? "Yes" : "No") +
                UString("\nUMA cachable: ") + (partitions[i].ptEntry.UmaCachable ? "Yes" : "No");

            if (signature == ME_CPD_SIGNATURE) {
                const ME_CPD_HEADER* cpdHeader = (const ME_CPD_HEADER*)partition.constData();
                name = usprintf("%c%c%c%c", cpdHeader->ShortName[0], cpdHeader->ShortName[1], cpdHeader->ShortName[2], cpdHeader->ShortName[3]);
                UString text = meBpdtEntryTypeToUString(partitions[i].ptEntry.Type);

                // Add tree item
                UModelIndex ptIndex = model->addItem(partitions[i].ptEntry.Offset, Types::BpdtPartition, 0, name, text, info, UByteArray(), partition, UByteArray(), Fixed, parent);

                // Parse contents
                UModelIndex cpdIndex;
                parseCodePartitionDirectory(partition, partitions[i].ptEntry.Offset, ptIndex, cpdIndex);
            }
            else {
                // Add tree item
                model->addItem(partitions[i].ptEntry.Offset, Types::BpdtEntry, 0, name, UString(), info, UByteArray(), partition, UByteArray(), Fixed, parent);
            }
        }
        else if (partitions[i].type == Types::Padding) {
            UByteArray partition = region.mid(partitions[i].ptEntry.Offset, partitions[i].ptEntry.Length);
            
            // Get info
            name = UString("Padding");
            info = usprintf("Full size: %Xh (%u)",
                            partition.size(), partition.size());
            
            // Add tree item
            model->addItem(partitions[i].ptEntry.Offset, Types::Padding, getPaddingType(partition), name, UString(), info, UByteArray(), partition, UByteArray(), Fixed, parent);
        }
    }
    
    return U_SUCCESS;
}


USTATUS MeParser::parseCodePartitionDirectory(const UByteArray & directory, const UINT32 localOffset, const UModelIndex & parent, UModelIndex & index)
{
    // Check directory size
    if ((UINT32)directory.size() < sizeof(ME_CPD_HEADER)) {
        msg(usprintf("%s: CPD too small to fit partition table header", __FUNCTION__), parent);
        return U_INVALID_ME_PARTITION_TABLE;
    }

    // Populate partition table header
    const ME_CPD_HEADER* cpdHeader = (const ME_CPD_HEADER*)directory.constData();

    // Check directory size again
    UINT32 ptBodySize = cpdHeader->NumEntries * sizeof(ME_BPDT_CPD_ENTRY);
    UINT32 ptSize = sizeof(ME_CPD_HEADER) + ptBodySize;
    if ((UINT32)directory.size() < ptSize) {
        msg(usprintf("%s: CPD too small to fit partition table", __FUNCTION__), parent);
        return U_INVALID_ME_PARTITION_TABLE;
    }

    // Get info
    UByteArray header = directory.left(sizeof(ME_CPD_HEADER));
    UByteArray body = directory.mid(sizeof(ME_CPD_HEADER));
    UString name = usprintf("CPD partition table");
    UString info = usprintf("Full size: %Xh (%u)\nHeader size: %Xh (%u)\nBody size: %Xh (%u)\nNumber of entries: %u\n"
                            "Header version: %u\nEntry version: %u\nHeader checksum: %02Xh",
                            directory.size(), directory.size(),
                            header.size(), header.size(),
                            body.size(), body.size(),
                            cpdHeader->NumEntries,
                            cpdHeader->HeaderVersion,
                            cpdHeader->EntryVersion,
                            cpdHeader->HeaderChecksum);

    // Add tree item
    index = model->addItem(localOffset, Types::CpdStore, 0, name, UString(), info, header, body, UByteArray(), Fixed, parent);

    // Add partition table entries
    std::vector<ME_CPD_PARTITION_INFO> partitions;
    UINT32 offset = sizeof(ME_CPD_HEADER);
    const ME_BPDT_CPD_ENTRY* firstCpdEntry = (const ME_BPDT_CPD_ENTRY*)(body.constData());
    for (UINT32 i = 0; i < cpdHeader->NumEntries; i++) {
        // Populate entry header
        const ME_BPDT_CPD_ENTRY* cpdEntry = firstCpdEntry + i;
        UByteArray entry((const char*)cpdEntry, sizeof(ME_BPDT_CPD_ENTRY));

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
        offset += sizeof(ME_BPDT_CPD_ENTRY);

        if (cpdEntry->Offset.Offset != 0 && cpdEntry->Length != 0) {
            // Add to partitions vector
            ME_CPD_PARTITION_INFO partition;
            partition.type = Types::CpdPartition;
            partition.ptEntry = *cpdEntry;
            partition.index = entryIndex;
            partitions.push_back(partition);
        }
    }

    // Add padding if there's no partions to add
    if (partitions.size() == 0) {
        UByteArray partition = directory.mid(ptSize);

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
        UByteArray partition = directory.mid(partitions[i].ptEntry.Offset.Offset, partitions[i].ptEntry.Length);
        while (offset < (UINT32)partition.size()) {
            const ME_CPD_EXTENTION_HEADER* extHeader = (const ME_CPD_EXTENTION_HEADER*) (partition.constData() + offset);
            if (extHeader->Length <= ((UINT32)partition.size() - offset)) {
                if (extHeader->Type == 10) { //TODO: replace with defines
                    const ME_CPD_EXT_MODULE_ATTRIBUTES* attrHeader = (const ME_CPD_EXT_MODULE_ATTRIBUTES*)(partition.constData() + offset);
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
   ME_CPD_PARTITION_INFO padding;

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
       if ((UINT64)partitions[i].ptEntry.Offset.Offset + (UINT64)partitions[i].ptEntry.Length > (UINT64)directory.size()) {
           if ((UINT64)partitions[i].ptEntry.Offset.Offset >= (UINT64)directory.size()) {
               msg(usprintf("%s: CPD partition is located outside of the opened image, skipped", __FUNCTION__), partitions[i].index);
               partitions.erase(partitions.begin() + i);
               goto make_partition_table_consistent;
           }
           else {
               msg(usprintf("%s: CPD partition can't fit into it's region, truncated", __FUNCTION__), partitions[i].index);
               partitions[i].ptEntry.Length = (UINT32)directory.size() - (UINT32)partitions[i].ptEntry.Offset.Offset;
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
               msg(usprintf("%s: CPD partition intersects with prevous one, skipped", __FUNCTION__),
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
           std::vector<ME_CPD_PARTITION_INFO>::iterator iter = partitions.begin();
           std::advance(iter, i);
           partitions.insert(iter, padding);
       }
   }
   // Check for padding after the last region
   if ((UINT64)partitions.back().ptEntry.Offset.Offset + (UINT64)partitions.back().ptEntry.Length < (UINT64)directory.size()) {
       padding.ptEntry.Offset.Offset = partitions.back().ptEntry.Offset.Offset + partitions.back().ptEntry.Length;
       padding.ptEntry.Length = (UINT32)directory.size() - padding.ptEntry.Offset.Offset;
       padding.type = Types::Padding;
       partitions.push_back(padding);
   }

    // Partition map is consistent
    for (size_t i = 0; i < partitions.size(); i++) {
        if (partitions[i].type == Types::CpdPartition) {
            UByteArray partition = directory.mid(partitions[i].ptEntry.Offset.Offset, partitions[i].ptEntry.Length);

            // Get info
            name = usprintf("%c%c%c%c%c%c%c%c%c%c%c%c",
                            partitions[i].ptEntry.EntryName[0], partitions[i].ptEntry.EntryName[1], partitions[i].ptEntry.EntryName[2], partitions[i].ptEntry.EntryName[3],
                            partitions[i].ptEntry.EntryName[4], partitions[i].ptEntry.EntryName[5], partitions[i].ptEntry.EntryName[6], partitions[i].ptEntry.EntryName[7],
                            partitions[i].ptEntry.EntryName[8], partitions[i].ptEntry.EntryName[9], partitions[i].ptEntry.EntryName[10], partitions[i].ptEntry.EntryName[11]);

            // It's a manifest
            if (name.contains(".man")) {
                if (!partitions[i].ptEntry.Offset.HuffmanCompressed
                  && partitions[i].ptEntry.Length >= sizeof(ME_CPD_MANIFEST_HEADER)) {
                    const ME_CPD_MANIFEST_HEADER* manifestHeader = (const ME_CPD_MANIFEST_HEADER*) partition.constData();
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
                        parseExtensionsArea(partitionIndex);
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
                parseExtensionsArea(partitionIndex);
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
                parseExtensionsArea(partitionIndex);
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

                model->addItem(localOffset + partitions[i].ptEntry.Offset.Offset, Types::CpdPartition, Subtypes::CodeCpdPartition, name, UString(), info, UByteArray(), partition, UByteArray(), Fixed, parent);
            }
        }
        else if (partitions[i].type == Types::Padding) {
            UByteArray partition = directory.mid(partitions[i].ptEntry.Offset.Offset, partitions[i].ptEntry.Length);

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

USTATUS MeParser::parseExtensionsArea(const UModelIndex & index)
{
    if (!index.isValid()) {
        return U_INVALID_PARAMETER;
    }

    UByteArray body = model->body(index);
    UINT32 offset = 0;
    while (offset < (UINT32)body.size()) {
        const ME_CPD_EXTENTION_HEADER* extHeader = (const ME_CPD_EXTENTION_HEADER*) (body.constData() + offset);
        if (extHeader->Length <= ((UINT32)body.size() - offset)) {
            UByteArray partition = body.mid(offset, extHeader->Length);

            UString name = meExtensionTypeToUstring(extHeader->Type);
            UString info = usprintf("Full size: %Xh (%u)\nType: %Xh", partition.size(), partition.size(), extHeader->Type);

            // Parse Signed Package Info a bit further
            bool parsed = false;
            if (extHeader->Type == 15) {
                UByteArray header = partition.left(sizeof(ME_CPD_EXT_SIGNED_PACKAGE_INFO));
                UByteArray data = partition.mid(header.size());

                const ME_CPD_EXT_SIGNED_PACKAGE_INFO* infoHeader = (const ME_CPD_EXT_SIGNED_PACKAGE_INFO*)header.constData();

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
                UModelIndex infoIndex = model->addItem(offset, Types::CpdExtension, 0, name, UString(), info, header, data, UByteArray(), Fixed, index);
                parseSignedPackageInfoData(infoIndex);
                parsed = true;
            }

            // Parse IFWI Partition Manifest a bit further
            else if (extHeader->Type == 22) {
                const ME_CPD_EXT_IFWI_PARTITION_MANIFEST* attrHeader = (const ME_CPD_EXT_IFWI_PARTITION_MANIFEST*)partition.constData();

                // This hash is stored reversed, because why the hell not
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
                model->addItem(offset, Types::CpdExtension, 0, name, UString(), info, UByteArray(), partition, UByteArray(), Fixed, index);
                parsed = true;
            }

            // Parse Module Attributes a bit further
            else if (extHeader->Type == 10) {
                const ME_CPD_EXT_MODULE_ATTRIBUTES* attrHeader = (const ME_CPD_EXT_MODULE_ATTRIBUTES*)partition.constData();

                // This hash is stored reversed, because why the hell not
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
                model->addItem(offset, Types::CpdExtension, 0, name, UString(), info, UByteArray(), partition, UByteArray(), Fixed, index);
                parsed = true;
            }

            if (!parsed) {
                // Add tree item, if needed
                model->addItem(offset, Types::CpdExtension, 0, name, UString(), info, UByteArray(), partition, UByteArray(), Fixed, index);
            }

            offset += extHeader->Length;
        }
        else break;
        // TODO: add padding at the end
    }

    return U_SUCCESS;
}

USTATUS MeParser::parseSignedPackageInfoData(const UModelIndex & index)
{
    if (!index.isValid()) {
        return U_INVALID_PARAMETER;
    }

    UByteArray body = model->body(index);
    UINT32 offset = 0;
    while (offset < (UINT32)body.size()) {
        const ME_CPD_EXT_SIGNED_PACKAGE_INFO_MODULES* moduleHeader = (const ME_CPD_EXT_SIGNED_PACKAGE_INFO_MODULES*)(body.constData() + offset);
        if (sizeof(ME_CPD_EXT_SIGNED_PACKAGE_INFO_MODULES) <= ((UINT32)body.size() - offset)) {
            UByteArray module((const char*)moduleHeader,sizeof(ME_CPD_EXT_SIGNED_PACKAGE_INFO_MODULES));

            UString name = usprintf("%c%c%c%c%c%c%c%c%c%c%c%c",
                                    moduleHeader->Name[0], moduleHeader->Name[1], moduleHeader->Name[2], moduleHeader->Name[3],
                                    moduleHeader->Name[4], moduleHeader->Name[5], moduleHeader->Name[6], moduleHeader->Name[7],
                                    moduleHeader->Name[8], moduleHeader->Name[9], moduleHeader->Name[10],moduleHeader->Name[11]);

            // This hash is stored reversed, because why the hell not
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

#endif // U_ENABLE_ME_PARSING_SUPPORT

