/* meparser.cpp

Copyright (c) 2019, Nikolaj Schlej. All rights reserved.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#include <inttypes.h>
#include <map>

#include "ffs.h"
#include "me.h"
#include "meparser.h"
#include "parsingdata.h"
#include "utility.h"

#ifdef U_ENABLE_ME_PARSING_SUPPORT

struct FPT_PARTITION_INFO {
    FPT_HEADER_ENTRY ptEntry;
    UINT8 type;
    UModelIndex index;
    friend bool operator< (const FPT_PARTITION_INFO & lhs, const FPT_PARTITION_INFO & rhs){ return lhs.ptEntry.Offset < rhs.ptEntry.Offset; }
};

struct IFWI_PARTITION_INFO {
    IFWI_HEADER_ENTRY ptEntry;
    UINT8 type;
    UINT8 subtype;
    friend bool operator< (const IFWI_PARTITION_INFO & lhs, const IFWI_PARTITION_INFO & rhs){ return lhs.ptEntry.Offset < rhs.ptEntry.Offset; }
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
    if (meRegion.left(sizeof(UINT32)) == FPT_HEADER_SIGNATURE || meRegion.mid(ME_ROM_BYPASS_VECTOR_SIZE, sizeof(UINT32)) == FPT_HEADER_SIGNATURE) {
        UModelIndex ptIndex;
        return parseFptRegion(meRegion, index, ptIndex);
    }

    // IFWI 1.6
    // Check region size
    if ((UINT32)meRegion.size() < sizeof(IFWI_16_LAYOUT_HEADER)) {
        msg(usprintf("%s: ME region too small to fit IFWI 1.6 layout header", __FUNCTION__), index);
        return U_INVALID_ME_PARTITION_TABLE;
    }

    const IFWI_16_LAYOUT_HEADER* ifwi16Header = (const IFWI_16_LAYOUT_HEADER*)meRegion.constData();
    // Check region size
    if ((UINT32)meRegion.size() < ifwi16Header->DataPartition.Offset + sizeof(UINT32)) {
        msg(usprintf("%s: ME region too small to fit IFWI 1.6 data partition", __FUNCTION__), index);
        return U_INVALID_ME_PARTITION_TABLE;
    }
    // Data partition always points to FPT header
    if (meRegion.mid(ifwi16Header->DataPartition.Offset, sizeof(UINT32)) == FPT_HEADER_SIGNATURE) {
        UModelIndex ptIndex;
        return parseIfwi16Region(meRegion, index, ptIndex);
    }

    // IFWI 1.7
    if ((UINT32)meRegion.size() < sizeof(IFWI_17_LAYOUT_HEADER)) {
        msg(usprintf("%s: ME region too small to fit IFWI 1.7 layout header", __FUNCTION__), index);
        return U_INVALID_ME_PARTITION_TABLE;
    }
    
    const IFWI_17_LAYOUT_HEADER* ifwi17Header = (const IFWI_17_LAYOUT_HEADER*)meRegion.constData();
    // Check region size
    if ((UINT32)meRegion.size() < ifwi17Header->DataPartition.Offset + sizeof(UINT32)) {
        msg(usprintf("%s: ME region too small to fit IFWI 1.7 data partition", __FUNCTION__), index);
        return U_INVALID_ME_PARTITION_TABLE;
    }
    // Data partition always points to FPT header
    if (meRegion.mid(ifwi17Header->DataPartition.Offset, sizeof(UINT32)) == FPT_HEADER_SIGNATURE) {
        UModelIndex ptIndex;
        return parseIfwi17Region(meRegion, index, ptIndex);
    }

    // Something else entirely
    msg(usprintf("%s: unknown ME region format", __FUNCTION__), index);
    return U_INVALID_ME_PARTITION_TABLE;
}

USTATUS MeParser::parseFptRegion(const UByteArray & region, const UModelIndex & parent, UModelIndex & index)
{
    // Check region size
    if ((UINT32)region.size() < sizeof(FPT_HEADER)) {
        msg(usprintf("%s: region too small to fit FPT header", __FUNCTION__), parent);
        return U_INVALID_ME_PARTITION_TABLE;
    }
    
    // Populate partition table header
    const FPT_HEADER* ptHeader = (const FPT_HEADER*)region.constData();
    UINT32 romBypassVectorSize = 0;
    if (region.left(sizeof(UINT32)) != FPT_HEADER_SIGNATURE) {
        // Adjust the header to skip ROM bypass vector
        romBypassVectorSize = ME_ROM_BYPASS_VECTOR_SIZE;
        ptHeader = (const FPT_HEADER*)(region.constData() + romBypassVectorSize);
    }
    
    // Check region size again
    UINT32 ptBodySize = ptHeader->NumEntries * sizeof(FPT_HEADER_ENTRY);
    UINT32 ptSize = romBypassVectorSize + sizeof(FPT_HEADER) + ptBodySize;
    if ((UINT32)region.size() < ptSize) {
        msg(usprintf("%s: ME region too small to fit partition table", __FUNCTION__), parent);
        return U_INVALID_ME_PARTITION_TABLE;
    }
    
    // Recalculate checksum
    UByteArray tempHeader = UByteArray((const char*)ptHeader, sizeof(FPT_HEADER));
    FPT_HEADER* tempPtHeader = (FPT_HEADER*)tempHeader.data();
    tempPtHeader->Checksum = 0;
    UINT8 calculated = calculateChecksum8((const UINT8*)tempPtHeader, sizeof(FPT_HEADER));
    bool msgInvalidPtHeaderChecksum = (calculated != ptHeader->Checksum);
    
    // Get info
    UByteArray header = region.left(romBypassVectorSize + sizeof(FPT_HEADER));
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
    std::vector<FPT_PARTITION_INFO> partitions;
    UINT32 offset = header.size();
    const FPT_HEADER_ENTRY* firstPtEntry = (const FPT_HEADER_ENTRY*)(region.constData() + offset);
    for (UINT8 i = 0; i < ptHeader->NumEntries; i++) {
        // Populate entry header
        const FPT_HEADER_ENTRY* ptEntry = firstPtEntry + i;
        
        // Get info
        name = usprintf("%c%c%c%c", ptEntry->Name[0], ptEntry->Name[1], ptEntry->Name[2], ptEntry->Name[3]);
        info = usprintf("Full size: %Xh (%u)\nPartition offset: %Xh\nPartition length: %Xh\nPartition type: %02Xh",
                        sizeof(FPT_HEADER_ENTRY), sizeof(FPT_HEADER_ENTRY),
                        ptEntry->Offset,
                        ptEntry->Size,
                        ptEntry->Type);
        
        // Add tree item
        const UINT8 type = (ptEntry->Offset != 0 && ptEntry->Offset != 0xFFFFFFFF && ptEntry->Size != 0 && ptEntry->EntryValid != 0xFF) ? Subtypes::ValidFptEntry : Subtypes::InvalidFptEntry;
        UModelIndex entryIndex = model->addItem(offset, Types::FptEntry, type, name, UString(), info, UByteArray(), UByteArray((const char*)ptEntry, sizeof(FPT_HEADER_ENTRY)), UByteArray(), Fixed, index);
        
        // Adjust offset
        offset += sizeof(FPT_HEADER_ENTRY);
        
        // Add valid partitions
        if (type == Subtypes::ValidFptEntry) { // Skip absent and invalid partitions
            // Add to partitions vector
            FPT_PARTITION_INFO partition;
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
    FPT_PARTITION_INFO padding;
    
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
        padding.ptEntry.Size = partitions.front().ptEntry.Offset - ptSize;
        padding.type = Types::Padding;
        partitions.insert(partitions.begin(), padding);
    }
    // Check for intersections/paddings between partitions
    for (size_t i = 1; i < partitions.size(); i++) {
        UINT32 previousPartitionEnd = partitions[i - 1].ptEntry.Offset + partitions[i - 1].ptEntry.Size;
        
        // Check that current region is fully present in the image
        if ((UINT32)partitions[i].ptEntry.Offset + (UINT32)partitions[i].ptEntry.Size > (UINT32)region.size()) {
            if ((UINT32)partitions[i].ptEntry.Offset >= (UINT32)region.size()) {
                msg(usprintf("%s: FPT partition is located outside of the opened image, skipped", __FUNCTION__), partitions[i].index);
                partitions.erase(partitions.begin() + i);
                goto make_partition_table_consistent;
            }
            else {
                msg(usprintf("%s: FPT partition can't fit into the region, truncated", __FUNCTION__), partitions[i].index);
                partitions[i].ptEntry.Size = (UINT32)region.size() - (UINT32)partitions[i].ptEntry.Offset;
            }
        }

        // Check for intersection with previous partition
        if (partitions[i].ptEntry.Offset < previousPartitionEnd) {
            // Check if current partition is located inside previous one
            if (partitions[i].ptEntry.Offset + partitions[i].ptEntry.Size <= previousPartitionEnd) {
                msg(usprintf("%s: FPT partition is located inside another FPT partition, skipped", __FUNCTION__),
                    partitions[i].index);
                partitions.erase(partitions.begin() + i);
                goto make_partition_table_consistent;
            }
            else {
                msg(usprintf("%s: FPT partition intersects with previous one, skipped", __FUNCTION__),
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
            std::vector<FPT_PARTITION_INFO>::iterator iter = partitions.begin();
            std::advance(iter, i);
            partitions.insert(iter, padding);
        }
    }
    // Check for padding after the last region
    if ((UINT32)partitions.back().ptEntry.Offset + (UINT32)partitions.back().ptEntry.Size < (UINT32)region.size()) {
        padding.ptEntry.Offset = partitions.back().ptEntry.Offset + partitions.back().ptEntry.Size;
        padding.ptEntry.Size = region.size() - padding.ptEntry.Offset;
        padding.type = Types::Padding;
        partitions.push_back(padding);
    }
    
    // Partition map is consistent
    for (size_t i = 0; i < partitions.size(); i++) {
        UByteArray partition = region.mid(partitions[i].ptEntry.Offset, partitions[i].ptEntry.Size);
        if (partitions[i].type == Types::FptPartition) {
            UModelIndex partitionIndex;
            // Get info
            name = usprintf("%c%c%c%c", partitions[i].ptEntry.Name[0], partitions[i].ptEntry.Name[1], partitions[i].ptEntry.Name[2], partitions[i].ptEntry.Name[3]);
            info = usprintf("Full size: %Xh (%u)\nPartition type: %02Xh\n",
                partition.size(), partition.size(),
                partitions[i].ptEntry.Type);

            // Add tree item
            UINT8 type = Subtypes::CodeFptPartition + partitions[i].ptEntry.Type;
            partitionIndex = model->addItem(partitions[i].ptEntry.Offset, Types::FptPartition, type, name, UString(), info, UByteArray(), partition, UByteArray(), Fixed, parent);
            if (type == Subtypes::CodeFptPartition && partition.size() >= (int) sizeof(UINT32) && readUnaligned((const UINT32*)partition.constData()) == CPD_SIGNATURE) {
                // Parse code partition contents
                UModelIndex cpdIndex;
                ffsParser->parseCpdRegion(partition, partitions[i].ptEntry.Offset, partitionIndex, cpdIndex);
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

USTATUS MeParser::parseIfwi16Region(const UByteArray & region, const UModelIndex & parent, UModelIndex & index)
{
    // Check region size again
    if ((UINT32)region.size() < sizeof(IFWI_16_LAYOUT_HEADER)) {
        msg(usprintf("%s: ME region too small to fit IFWI 1.6 layout header", __FUNCTION__), parent);
        return U_INVALID_ME_PARTITION_TABLE;
    }
    
    const IFWI_16_LAYOUT_HEADER* ifwiHeader = (const IFWI_16_LAYOUT_HEADER*)region.constData();
    
    // Add header
    UINT32 ptSize = sizeof(IFWI_16_LAYOUT_HEADER);
    UByteArray header = region.left(ptSize);
    
    UString name = UString("IFWI 1.6 header");
    UString info = usprintf("Full size: %Xh (%u)\n"
                            "Data  partition offset: %Xh\nData  partition size:   %Xh\n"
                            "Boot1 partition offset: %Xh\nBoot1 partition size:   %Xh\n"
                            "Boot2 partition offset: %Xh\nBoot2 partition size:   %Xh\n"
                            "Boot3 partition offset: %Xh\nBoot3 partition size:   %Xh\n"
                            "Boot4 partition offset: %Xh\nBoot4 partition size:   %Xh\n"
                            "Boot5 partition offset: %Xh\nBoot5 partition size:   %Xh\n"
                            "Checksum: %Xh",
                            header.size(), header.size(),
                            ifwiHeader->DataPartition.Offset, ifwiHeader->DataPartition.Size,
                            ifwiHeader->BootPartition[0].Offset, ifwiHeader->BootPartition[0].Size,
                            ifwiHeader->BootPartition[1].Offset, ifwiHeader->BootPartition[1].Size,
                            ifwiHeader->BootPartition[2].Offset, ifwiHeader->BootPartition[2].Size,
                            ifwiHeader->BootPartition[3].Offset, ifwiHeader->BootPartition[3].Size,
                            ifwiHeader->BootPartition[4].Offset, ifwiHeader->BootPartition[4].Size,
                            ifwiHeader->Checksum);
    // Add tree item
    index = model->addItem(0, Types::IfwiHeader, 0, name, UString(), info, UByteArray(), header, UByteArray(), Fixed, parent);
    
    std::vector<IFWI_PARTITION_INFO> partitions;
    // Add data partition
    {
        IFWI_PARTITION_INFO partition;
        partition.type = Types::IfwiPartition;
        partition.subtype = Subtypes::DataIfwiPartition;
        partition.ptEntry = ifwiHeader->DataPartition;
        partitions.push_back(partition);
    }
    // Add boot partitions
    for (UINT8 i = 0 ; i < 4; i++) {
        if (ifwiHeader->BootPartition[i].Offset != 0 && ifwiHeader->BootPartition[i].Offset != 0xFFFFFFFF) {
            IFWI_PARTITION_INFO partition;
            partition.type = Types::IfwiPartition;
            partition.subtype = Subtypes::BootIfwiPartition;
            partition.ptEntry = ifwiHeader->BootPartition[i];
            partitions.push_back(partition);
        }
    }

make_partition_table_consistent:
    // Sort partitions by offset
    std::sort(partitions.begin(), partitions.end());
    
    // Check for intersections and paddings between partitions
    IFWI_PARTITION_INFO padding;
    
    // Check intersection with the partition table header
    if (partitions.front().ptEntry.Offset < ptSize) {
        msg(usprintf("%s: IFWI partition has intersection with IFWI layout header, skipped", __FUNCTION__), index);
        partitions.erase(partitions.begin());
        goto make_partition_table_consistent;
    }
    // Check for padding between partition table and the first partition
    else if (partitions.front().ptEntry.Offset > ptSize) {
        padding.ptEntry.Offset = ptSize;
        padding.ptEntry.Size = partitions.front().ptEntry.Offset - ptSize;
        padding.type = Types::Padding;
        partitions.insert(partitions.begin(), padding);
    }
    // Check for intersections/paddings between partitions
    for (size_t i = 1; i < partitions.size(); i++) {
        UINT32 previousPartitionEnd = partitions[i - 1].ptEntry.Offset + partitions[i - 1].ptEntry.Size;
        
        // Check that current region is fully present in the image
        if ((UINT32)partitions[i].ptEntry.Offset + (UINT32)partitions[i].ptEntry.Size > (UINT32)region.size()) {
            if ((UINT32)partitions[i].ptEntry.Offset >= (UINT32)region.size()) {
                msg(usprintf("%s: IFWI partition is located outside of the opened image, skipped", __FUNCTION__), index);
                partitions.erase(partitions.begin() + i);
                goto make_partition_table_consistent;
            }
            else {
                msg(usprintf("%s: IFWI partition can't fit into the region, truncated", __FUNCTION__), index);
                partitions[i].ptEntry.Size = (UINT32)region.size() - (UINT32)partitions[i].ptEntry.Offset;
            }
        }
        
        // Check for intersection with previous partition
        if (partitions[i].ptEntry.Offset < previousPartitionEnd) {
            // Check if current partition is located inside previous one
            if (partitions[i].ptEntry.Offset + partitions[i].ptEntry.Size <= previousPartitionEnd) {
                msg(usprintf("%s: IFWI partition is located inside another IFWI partition, skipped", __FUNCTION__), index);
                partitions.erase(partitions.begin() + i);
                goto make_partition_table_consistent;
            }
            else {
                msg(usprintf("%s: IFWI partition intersects with previous one, skipped", __FUNCTION__), index);
                partitions.erase(partitions.begin() + i);
                goto make_partition_table_consistent;
            }
        }
        
        // Check for padding between current and previous partitions
        else if (partitions[i].ptEntry.Offset > previousPartitionEnd) {
            padding.ptEntry.Offset = previousPartitionEnd;
            padding.ptEntry.Size = partitions[i].ptEntry.Offset - previousPartitionEnd;
            padding.type = Types::Padding;
            std::vector<IFWI_PARTITION_INFO>::iterator iter = partitions.begin();
            std::advance(iter, i);
            partitions.insert(iter, padding);
        }
    }
    // Check for padding after the last region
    if ((UINT32)partitions.back().ptEntry.Offset + (UINT32)partitions.back().ptEntry.Size < (UINT32)region.size()) {
        padding.ptEntry.Offset = partitions.back().ptEntry.Offset + partitions.back().ptEntry.Size;
        padding.ptEntry.Size = region.size() - padding.ptEntry.Offset;
        padding.type = Types::Padding;
        partitions.push_back(padding);
    }
    
    // Partition map is consistent
    for (size_t i = 0; i < partitions.size(); i++) {
        UByteArray partition = region.mid(partitions[i].ptEntry.Offset, partitions[i].ptEntry.Size);
        if (partitions[i].type == Types::IfwiPartition) {
            UModelIndex partitionIndex;
            if (partitions[i].subtype == Subtypes::DataIfwiPartition) {
                name = "Data partition";
            }
            else if (partitions[i].subtype == Subtypes::BootIfwiPartition) {
                name = "Boot partition";
            }
            
            // Get info
            info = usprintf("Full size: %Xh (%u)\n",
                            partition.size(), partition.size());
            
            // Add tree item
            partitionIndex = model->addItem(partitions[i].ptEntry.Offset, partitions[i].type, partitions[i].subtype, name, UString(), info, UByteArray(), partition, UByteArray(), Fixed, parent);
            
            // Parse partition further
            if (partitions[i].subtype == Subtypes::DataIfwiPartition) {
                UModelIndex dataPartitionFptRegionIndex;
                parseFptRegion(partition, partitionIndex, dataPartitionFptRegionIndex);
            }
            else if (partitions[i].subtype == Subtypes::BootIfwiPartition) {
                // Parse code partition contents
                UModelIndex bootPartitionBpdtRegionIndex;
                ffsParser->parseBpdtRegion(partition, 0, 0, partitionIndex, bootPartitionBpdtRegionIndex);
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

USTATUS MeParser::parseIfwi17Region(const UByteArray & region, const UModelIndex & parent, UModelIndex & index)
{
    // Check region size again
    if ((UINT32)region.size() < sizeof(IFWI_17_LAYOUT_HEADER)) {
        msg(usprintf("%s: ME region too small to fit IFWI 1.7 layout header", __FUNCTION__), parent);
        return U_INVALID_ME_PARTITION_TABLE;
    }
    
    const IFWI_17_LAYOUT_HEADER* ifwiHeader = (const IFWI_17_LAYOUT_HEADER*)region.constData();
    // TODO: add check for HeaderSize to be 0x40
    
    // Add header
    UINT32 ptSize = sizeof(IFWI_17_LAYOUT_HEADER);
    UByteArray header = region.left(ptSize);
    
    UString name = UString("IFWI 1.7 header");
    UString info = usprintf("Full size: %Xh (%u)\n"
                            "Flags: %02Xh\n"
                            "Reserved: %02Xh\n"
                            "Checksum: %Xh\n"
                            "Data  partition offset: %Xh\nData  partition size:   %Xh\n"
                            "Boot1 partition offset: %Xh\nBoot1 partition size:   %Xh\n"
                            "Boot2 partition offset: %Xh\nBoot2 partition size:   %Xh\n"
                            "Boot3 partition offset: %Xh\nBoot3 partition size:   %Xh\n"
                            "Boot4 partition offset: %Xh\nBoot4 partition size:   %Xh\n"
                            "Boot5 partition offset: %Xh\nBoot5 partition size:   %Xh\n"
                            "Temp page offset:       %Xh\nTemp page size:         %Xh\n",
                            header.size(), header.size(),
                            ifwiHeader->Flags,
                            ifwiHeader->Reserved,
                            ifwiHeader->Checksum,
                            ifwiHeader->DataPartition.Offset, ifwiHeader->DataPartition.Size,
                            ifwiHeader->BootPartition[0].Offset, ifwiHeader->BootPartition[0].Size,
                            ifwiHeader->BootPartition[1].Offset, ifwiHeader->BootPartition[1].Size,
                            ifwiHeader->BootPartition[2].Offset, ifwiHeader->BootPartition[2].Size,
                            ifwiHeader->BootPartition[3].Offset, ifwiHeader->BootPartition[3].Size,
                            ifwiHeader->BootPartition[4].Offset, ifwiHeader->BootPartition[4].Size,
                            ifwiHeader->TempPage.Offset, ifwiHeader->TempPage.Size);
    // Add tree item
    index = model->addItem(0, Types::IfwiHeader, 0, name, UString(), info, UByteArray(), header, UByteArray(), Fixed, parent);
    
    std::vector<IFWI_PARTITION_INFO> partitions;
    // Add data partition
    {
        IFWI_PARTITION_INFO partition;
        partition.type = Types::IfwiPartition;
        partition.subtype = Subtypes::DataIfwiPartition;
        partition.ptEntry = ifwiHeader->DataPartition;
        partitions.push_back(partition);
    }
    // Add boot partitions
    for (UINT8 i = 0 ; i < 4; i++) {
        if (ifwiHeader->BootPartition[i].Offset != 0 && ifwiHeader->BootPartition[i].Offset != 0xFFFFFFFF) {
            IFWI_PARTITION_INFO partition;
            partition.type = Types::IfwiPartition;
            partition.subtype = Subtypes::BootIfwiPartition;
            partition.ptEntry = ifwiHeader->BootPartition[i];
            partitions.push_back(partition);
        }
    }
    // Add temp page
    if (ifwiHeader->TempPage.Offset != 0 && ifwiHeader->TempPage.Offset != 0xFFFFFFFF) {
        IFWI_PARTITION_INFO partition;
        partition.type = Types::IfwiPartition;
        partition.subtype = Subtypes::DataPadding;
        partition.ptEntry = ifwiHeader->TempPage;
        partitions.push_back(partition);
    }
    
make_partition_table_consistent:
    // Sort partitions by offset
    std::sort(partitions.begin(), partitions.end());
    
    // Check for intersections and paddings between partitions
    IFWI_PARTITION_INFO padding;
    
    // Check intersection with the partition table header
    if (partitions.front().ptEntry.Offset < ptSize) {
        msg(usprintf("%s: IFWI partition has intersection with IFWI layout header, skipped", __FUNCTION__), index);
        partitions.erase(partitions.begin());
        goto make_partition_table_consistent;
    }
    // Check for padding between partition table and the first partition
    else if (partitions.front().ptEntry.Offset > ptSize) {
        padding.ptEntry.Offset = ptSize;
        padding.ptEntry.Size = partitions.front().ptEntry.Offset - ptSize;
        padding.type = Types::Padding;
        partitions.insert(partitions.begin(), padding);
    }
    // Check for intersections/paddings between partitions
    for (size_t i = 1; i < partitions.size(); i++) {
        UINT32 previousPartitionEnd = partitions[i - 1].ptEntry.Offset + partitions[i - 1].ptEntry.Size;
        
        // Check that current region is fully present in the image
        if ((UINT32)partitions[i].ptEntry.Offset + (UINT32)partitions[i].ptEntry.Size > (UINT32)region.size()) {
            if ((UINT32)partitions[i].ptEntry.Offset >= (UINT32)region.size()) {
                msg(usprintf("%s: IFWI partition is located outside of the opened image, skipped", __FUNCTION__), index);
                partitions.erase(partitions.begin() + i);
                goto make_partition_table_consistent;
            }
            else {
                msg(usprintf("%s: IFWI partition can't fit into the region, truncated", __FUNCTION__), index);
                partitions[i].ptEntry.Size = (UINT32)region.size() - (UINT32)partitions[i].ptEntry.Offset;
            }
        }
        
        // Check for intersection with previous partition
        if (partitions[i].ptEntry.Offset < previousPartitionEnd) {
            // Check if current partition is located inside previous one
            if (partitions[i].ptEntry.Offset + partitions[i].ptEntry.Size <= previousPartitionEnd) {
                msg(usprintf("%s: IFWI partition is located inside another IFWI partition, skipped", __FUNCTION__), index);
                partitions.erase(partitions.begin() + i);
                goto make_partition_table_consistent;
            }
            else {
                msg(usprintf("%s: IFWI partition intersects with previous one, skipped", __FUNCTION__), index);
                partitions.erase(partitions.begin() + i);
                goto make_partition_table_consistent;
            }
        }
        
        // Check for padding between current and previous partitions
        else if (partitions[i].ptEntry.Offset > previousPartitionEnd) {
            padding.ptEntry.Offset = previousPartitionEnd;
            padding.ptEntry.Size = partitions[i].ptEntry.Offset - previousPartitionEnd;
            padding.type = Types::Padding;
            std::vector<IFWI_PARTITION_INFO>::iterator iter = partitions.begin();
            std::advance(iter, i);
            partitions.insert(iter, padding);
        }
    }
    // Check for padding after the last region
    if ((UINT32)partitions.back().ptEntry.Offset + (UINT32)partitions.back().ptEntry.Size < (UINT32)region.size()) {
        padding.ptEntry.Offset = partitions.back().ptEntry.Offset + partitions.back().ptEntry.Size;
        padding.ptEntry.Size = region.size() - padding.ptEntry.Offset;
        padding.type = Types::Padding;
        partitions.push_back(padding);
    }
    
    // Partition map is consistent
    for (size_t i = 0; i < partitions.size(); i++) {
        UByteArray partition = region.mid(partitions[i].ptEntry.Offset, partitions[i].ptEntry.Size);
        if (partitions[i].type == Types::IfwiPartition) {
            UModelIndex partitionIndex;
            if (partitions[i].subtype == Subtypes::DataIfwiPartition) {
                name = "Data partition";
                
            }
            else if (partitions[i].subtype == Subtypes::BootIfwiPartition){
                name = "Boot partition";
            }
            else {
                name = "Temp page";
            }
            
            // Get info
            info = usprintf("Full size: %Xh (%u)\n",
                            partition.size(), partition.size());
            
            // Add tree item
            partitionIndex = model->addItem(partitions[i].ptEntry.Offset, partitions[i].type, partitions[i].subtype, name, UString(), info, UByteArray(), partition, UByteArray(), Fixed, parent);
            
            // Parse partition further
            if (partitions[i].subtype == Subtypes::DataIfwiPartition) {
                UModelIndex dataPartitionFptRegionIndex;
                parseFptRegion(partition, partitionIndex, dataPartitionFptRegionIndex);
            }
            else if (partitions[i].subtype == Subtypes::BootIfwiPartition) {
                // Parse code partition contents
                UModelIndex bootPartitionBpdtRegionIndex;
                ffsParser->parseBpdtRegion(partition, 0, 0, partitionIndex, bootPartitionBpdtRegionIndex);
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

#endif // U_ENABLE_ME_PARSING_SUPPORT

