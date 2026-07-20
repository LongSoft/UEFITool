/* fssbuilder.cpp
 
 Copyright (c) 2015, Nikolaj Schlej. All rights reserved.
 This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 */
#include "ffsbuilder.h"

#include "descriptor.h"
#include "ffs.h"
#include "peimage.h"
#include "utility.h"
#include "nvram.h"
#include "parsingdata.h"
#include "Tiano/EfiTianoCompress.h"
#include "LZMA/LzmaCompress.h"

#include <cstring>

USTATUS FfsBuilder::erase(const UModelIndex & index, UByteArray & erased)
{
    // Sanity check
    if (!index.isValid())
        return U_INVALID_PARAMETER;
    
    // Try to get emptyByte value from item's parsing data
    UINT8 emptyByte = 0xFF;
    if (!model->hasEmptyParsingData(index)) {
        if (model->type(index) == Types::Volume) {
            VOLUME_PARSING_DATA pdata = *(VOLUME_PARSING_DATA*)model->parsingData(index).constData();
            emptyByte = pdata.emptyByte;
        }
        else if (model->type(index) == Types::File) {
            FILE_PARSING_DATA pdata = *(FILE_PARSING_DATA*)model->parsingData(index).constData();
            emptyByte = pdata.emptyByte;
        }
    }
    
    erased = UByteArray(model->fullSize(index), emptyByte);
    
    return U_SUCCESS;
}

USTATUS FfsBuilder::build(const UModelIndex & root, UByteArray & image)
{
    // Sanity check
    if (!root.isValid())
        return U_INVALID_PARAMETER;
    
    if (model->type(root) == Types::Capsule) {
        return buildCapsule(root, image);
    }
    else if (model->type(root) == Types::Image) {
        if (model->subtype(root) == Subtypes::IntelImage) {
            return buildIntelImage(root, image);
        }
        else if (model->subtype(root) == Subtypes::UefiImage) {
            return buildRawArea(root, image);
        }
    }
    
    return U_NOT_IMPLEMENTED;
}

USTATUS FfsBuilder::buildCapsule(const UModelIndex & index, UByteArray & capsule)
{
    // Sanity check
    if (!index.isValid())
        return U_INVALID_PARAMETER;
    
    // No action
    if (model->action(index) == Actions::NoAction) {
        // Use original item data
        capsule = model->full(index);
        return U_SUCCESS;
    }
    
    // Rebuild, Replace, or Insert
    else if (model->action(index) == Actions::Rebuild
             || model->action(index) == Actions::Replace
             || model->action(index) == Actions::Insert) {
        if (model->rowCount(index)) {
            // Clear the supplied UByteArray
            capsule.clear();
            
            // Right now there is only one capsule image element supported
            if (model->rowCount(index) != 1) {
                msg(usprintf("buildCapsule: building of capsules with %d items is not yet supported", model->rowCount(index)), index);
                return U_NOT_IMPLEMENTED;
            }
            
            // Build image
            UModelIndex imageIndex = index.model()->index(0, 0, index);
            UByteArray imageData;
            
            // Check image type
            if (model->type(imageIndex) == Types::Image) {
                USTATUS result = U_SUCCESS;
                if (model->subtype(imageIndex) == Subtypes::IntelImage) {
                    result = buildIntelImage(imageIndex, imageData);
                }
                else if (model->subtype(imageIndex) == Subtypes::UefiImage) {
                    result = buildRawArea(imageIndex, imageData);
                }
                else {
                    msg(UString("buildCapsule: unexpected item subtype ") + itemSubtypeToUString(model->type(imageIndex), model->subtype(imageIndex)), imageIndex);
                    return U_UNKNOWN_ITEM_TYPE;
                }
                
                // Check build result
                if (result) {
                    msg(UString("buildCapsule: building of ") + model->name(imageIndex) + UString(" failed with error ") + errorCodeToUString(result), imageIndex);
                    return result;
                }
                else
                    capsule += imageData;
            }
            else {
                msg(UString("buildCapsule: unexpected item type ") + itemTypeToUString(model->type(imageIndex)), imageIndex);
                return U_UNKNOWN_ITEM_TYPE;
            }
            
            // Check size of reconstructed capsule body, it must remain the same
            UINT32 newSize = (UINT32)capsule.size();
            UINT32 oldSize = model->bodySize(index);
            if (newSize > oldSize) {
                msg(usprintf("buildCapsule: new capsule size %Xh (%u) is bigger than the original %Xh (%u)", newSize, newSize, oldSize, oldSize), index);
                return U_INVALID_CAPSULE;
            }
            else if (newSize < oldSize) {
                msg(usprintf("buildCapsule: new capsule size %Xh (%u) is smaller than the original %Xh (%u)", newSize, newSize, oldSize, oldSize), index);
                return U_INVALID_CAPSULE;
            }
        }
        else
            capsule = model->body(index);
        
        // Build successful, append header and tail
        capsule = model->header(index) + capsule + model->tail(index);
        return U_SUCCESS;
    }
    
    msg(UString("buildCapsule: unexpected action " + actionTypeToUString(model->action(index))), index);
    return U_NOT_IMPLEMENTED;
}

USTATUS FfsBuilder::buildIntelImage(const UModelIndex & index, UByteArray & intelImage)
{
    // Sanity check
    if (!index.isValid())
        return U_SUCCESS;
    
    // No action
    if (model->action(index) == Actions::NoAction) {
        intelImage = model->full(index);
        return U_SUCCESS;
    }
    // Remove
    else if (model->action(index) == Actions::Remove) {
        intelImage.clear();
        return U_SUCCESS;
    }
    // Rebuild
    else if (model->action(index) == Actions::Rebuild) {
        // First child will always be descriptor for this type of image, and it's read only for now
        intelImage = model->full(index.model()->index(0, 0, index));
        
        // Process other regions
        for (int i = 1; i < model->rowCount(index); i++) {
            UModelIndex currentRegion = index.model()->index(i, 0, index);
            
            // Skip regions with Remove action
            if (model->action(currentRegion) == Actions::Remove)
                continue;
            
            // Check item type to be either region or padding
            UINT8 type = model->type(currentRegion);
            if (type == Types::Padding) {
                // Add padding as is
                intelImage += model->full(currentRegion);
                continue;
            }
            
            // Check region subtype
            USTATUS result;
            UByteArray region;
            UINT8 regionType = model->subtype(currentRegion);
            switch (regionType) {
                case Subtypes::BiosRegion:
                case Subtypes::PdrRegion:
                    result = buildRawArea(currentRegion, region);
                    if (result) {
                        msg(UString("buildIntelImage: building of region ") + regionTypeToUString(regionType) + UString(" failed with error ") + errorCodeToUString(result), currentRegion);
                        return result;
                    }
                    break;
                case Subtypes::MeRegion:
                case Subtypes::GbeRegion:
                case Subtypes::DevExp1Region:
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
                    // Add region as is
                    region = model->header(currentRegion) + model->body(currentRegion);
                    break;
                default:
                    msg(UString("buildIntelImage: unknown region type"), currentRegion);
                    return U_UNKNOWN_ITEM_TYPE;
            }
            
            // Append the resulting region
            intelImage += region;
        }
        
        // Check size of new image, it must be same as old one
        UINT32 newSize = (UINT32)intelImage.size();
        UINT32 oldSize = model->bodySize(index);
        if (newSize > oldSize) {
            msg(usprintf("buildIntelImage: new image size %Xh (%u) is bigger than the original %Xh (%u)", newSize, newSize, oldSize, oldSize), index);
            return U_INVALID_IMAGE;
        }
        else if (newSize < oldSize) {
            msg(usprintf("buildIntelImage: new image size %Xh (%u) is smaller than the original %Xh (%u)", newSize, newSize, oldSize, oldSize), index);
            return U_INVALID_IMAGE;
        }
        
        // Build successful, append header and tail
        intelImage = model->header(index) + intelImage + model->tail(index);
        return U_SUCCESS;
    }
    
    msg(UString("buildIntelImage: unexpected action " + actionTypeToUString(model->action(index))), index);
    return U_NOT_IMPLEMENTED;
}

USTATUS FfsBuilder::buildRawArea(const UModelIndex & index, UByteArray & rawArea)
{
    // Sanity check
    if (!index.isValid())
        return U_INVALID_PARAMETER;
    
    // No action required
    if (model->action(index) == Actions::NoAction) {
        rawArea = model->full(index);
        return U_SUCCESS;
    }
    // Remove
    else if (model->action(index) == Actions::Remove) {
        rawArea.clear();
        return U_SUCCESS;
    }
    // Rebuild, Replace, or Insert
    else if (model->action(index) == Actions::Rebuild
             || model->action(index) == Actions::Replace
             || model->action(index) == Actions::Insert) {
        // Rebuild if there is at least 1 child
        if (model->rowCount(index)) {
            // Clear the supplied UByteArray
            rawArea.clear();
            
            // Build children
            for (int i = 0; i < model->rowCount(index); i++) {
                USTATUS result = U_SUCCESS;
                UModelIndex currentChild = index.model()->index(i, 0, index);
                UByteArray currentData;
                
                // Check child type
                if (model->type(currentChild) == Types::Volume) {
                    result = buildVolume(currentChild, currentData);
                }
                else if (model->type(currentChild) == Types::Padding) {
                    result = buildPadding(currentChild, currentData);
                }
                else {
                    msg(UString("buildRawArea: unexpected item type ") + itemTypeToUString(model->type(currentChild)), currentChild);
                    return U_UNKNOWN_ITEM_TYPE;
                }
                // Check build result
                if (result) {
                    msg(UString("buildRawArea: building of ") + model->name(currentChild) + UString(" failed with error ") + errorCodeToUString(result), currentChild);
                    return result;
                }
                // Append current data
                rawArea += currentData;
            }
            
            // Check size of new raw area. It must not exceed the original, but
            // can be smaller — the missing space is padded with 0xFF (typical
            // empty flash value) to keep the raw area at its fixed size.
            UINT32 newSize = (UINT32)rawArea.size();
            UINT32 oldSize = model->bodySize(index);
            if (newSize > oldSize) {
                msg(usprintf("buildRawArea: new area size %Xh (%u) is bigger than the original %Xh (%u)", newSize, newSize, oldSize, oldSize), index);
                return U_INVALID_RAW_AREA;
            }
            else if (newSize < oldSize) {
                // Pad the remaining space with 0xFF
                rawArea += UByteArray(oldSize - newSize, (char)0xFF);
            }
        }
        // No need to rebuild a raw area with no children
        else {
            rawArea = model->body(index);
        }
        
        // Build successful, add header if needed
        rawArea = model->header(index) + rawArea + model->tail(index);
        return U_SUCCESS;
    }
    
    msg(UString("buildRawArea: unexpected action " + actionTypeToUString(model->action(index))), index);
    return U_NOT_IMPLEMENTED;
}

USTATUS FfsBuilder::buildPadding(const UModelIndex & index, UByteArray & padding)
{
    // Sanity check
    if (!index.isValid())
        return U_INVALID_PARAMETER;
    
    // No action required
    if (model->action(index) == Actions::NoAction) {
        padding = model->full(index);
        return U_SUCCESS;
    }
    // Remove
    else if (model->action(index) == Actions::Remove) {
        padding.clear();
        return U_SUCCESS;
    }
    // Erase
    else if (model->action(index) == Actions::Erase) {
        return erase(index, padding);
    }
    
    msg(UString("buildPadding: unexpected action " + actionTypeToUString(model->action(index))), index);
    return U_NOT_IMPLEMENTED;
}

USTATUS FfsBuilder::buildNonUefiData(const UModelIndex & index, UByteArray & data)
{
    // Sanity check
    if (!index.isValid())
        return U_INVALID_PARAMETER;
    
    // No action required
    if (model->action(index) == Actions::NoAction) {
        data = model->full(index);
        return U_SUCCESS;
    }
    // Remove
    else if (model->action(index) == Actions::Remove) {
        data.clear();
        return U_SUCCESS;
    }
    // Erase
    else if (model->action(index) == Actions::Erase) {
        return erase(index, data);
    }
    
    msg(UString("buildNoUefiData: unexpected action " + actionTypeToUString(model->action(index))), index);
    return U_NOT_IMPLEMENTED;
}

USTATUS FfsBuilder::buildFreeSpace(const UModelIndex & index, UByteArray & freeSpace)
{
    // Sanity check
    if (!index.isValid())
        return U_INVALID_PARAMETER;
    
    // No actions possible for free space
    freeSpace = model->full(index);
    return U_SUCCESS;
}

USTATUS FfsBuilder::buildVolume(const UModelIndex & index, UByteArray & volume)
{
    // Sanity check
    if (!index.isValid())
        return U_INVALID_PARAMETER;
    
    // No action required
    if (model->action(index) == Actions::NoAction) {
        volume = model->full(index);
        return U_SUCCESS;
    }
    // Remove
    else if (model->action(index) == Actions::Remove) {
        volume.clear();
        return U_SUCCESS;
    }
    // Rebuild, Replace, or Insert
    else if (model->action(index) == Actions::Rebuild
             || model->action(index) == Actions::Replace
             || model->action(index) == Actions::Insert) {
        // Gather volume parsing data
        UINT8 emptyByte = 0xFF;
        if (!model->hasEmptyParsingData(index)) {
            VOLUME_PARSING_DATA pdata = *(const VOLUME_PARSING_DATA*)model->parsingData(index).constData();
            emptyByte = pdata.emptyByte;
        }
        
        // Rebuild if there is at least 1 child
        if (model->rowCount(index)) {
            // Clear the supplied UByteArray
            UByteArray body;
            UINT32 freeSpaceSize = 0;
            bool freeSpaceFound = false;
            
            // Build children, deferring FreeSpace to the end so it can absorb
            // size changes from insert/remove/replace operations.
            for (int i = 0; i < model->rowCount(index); i++) {
                USTATUS result = U_SUCCESS;
                UModelIndex currentChild = index.model()->index(i, 0, index);
                UByteArray currentData;
                
                // Check child type
                if (model->type(currentChild) == Types::File) {
                    result = buildFile(currentChild, currentData);
                }
                else if (model->type(currentChild) == Types::FreeSpace) {
                    // Defer free space; remember its original size so we can
                    // recompute it after all other children are built.
                    freeSpaceSize += model->bodySize(currentChild);
                    freeSpaceFound = true;
                    continue;
                }
                else if (model->type(currentChild) == Types::Padding) {
                    result = buildPadding(currentChild, currentData);
                }
                else {
                    msg(UString("buildVolume: unexpected item type ") + itemTypeToUString(model->type(currentChild)), currentChild);
                    return U_UNKNOWN_ITEM_TYPE;
                }
                // Check build result
                if (result) {
                    msg(UString("buildVolume: building of ") + model->name(currentChild) + UString(" failed with error ") + errorCodeToUString(result), currentChild);
                    return result;
                }
                // Append current data
                body += currentData;
                
                // Append alignment bytes after each file, if any
                UByteArray alignment = model->alignmentBytes(currentChild);
                if (!alignment.isEmpty())
                    body += alignment;
            }
            
            // Compute the new body size and adjust free space / padding.
            UINT32 newBodySize = (UINT32)body.size();
            UINT32 oldBodySize = model->bodySize(index);
            if (newBodySize > oldBodySize) {
                // If we have free space to consume, the new content fits.
                if (freeSpaceFound && newBodySize - oldBodySize <= freeSpaceSize) {
                    // Reduce the free space accordingly; just emit the content
                    // as-is and pad with emptyByte to oldBodySize below.
                }
                else {
                    msg(usprintf("buildVolume: new volume body size %Xh (%u) is bigger than the original %Xh (%u), free space %Xh (%u)",
                                 newBodySize, newBodySize, oldBodySize, oldBodySize, freeSpaceSize, freeSpaceSize), index);
                    return U_INVALID_VOLUME;
                }
            }
            // Pad the body to the original body size using emptyByte.
            // This absorbs both the remaining free space and any shrinkage.
            if (newBodySize < oldBodySize)
                body += UByteArray(oldBodySize - newBodySize, (char)emptyByte);
            
            // Build successful, append header and tail
            volume = model->header(index) + body + model->tail(index);
        }
        else
            volume = model->full(index);
        
        return U_SUCCESS;
    }
    
    msg(UString("buildVolume: unexpected action ") + actionTypeToUString(model->action(index)), index);
    return U_NOT_IMPLEMENTED;
}

USTATUS FfsBuilder::buildPadFile(const UModelIndex & index, UByteArray & padFile)
{
    // Sanity check
    if (!index.isValid())
        return U_INVALID_PARAMETER;
    
    // No action required
    if (model->action(index) == Actions::NoAction) {
        padFile = model->full(index);
        return U_SUCCESS;
    }
    // Remove
    else if (model->action(index) == Actions::Remove) {
        padFile.clear();
        return U_SUCCESS;
    }
    // Rebuild: pad file is regenerated to fit the remaining free space
    else if (model->action(index) == Actions::Rebuild) {
        UINT8 emptyByte = 0xFF;
        if (!model->hasEmptyParsingData(index)) {
            FILE_PARSING_DATA pdata = *(const FILE_PARSING_DATA*)model->parsingData(index).constData();
            emptyByte = pdata.emptyByte;
        }
        padFile = UByteArray(model->fullSize(index), emptyByte);
        return U_SUCCESS;
    }
    
    msg(UString("buildPadFile: unexpected action ") + actionTypeToUString(model->action(index)), index);
    return U_NOT_IMPLEMENTED;
}

USTATUS FfsBuilder::buildFile(const UModelIndex & index, UByteArray & file)
{
    // Sanity check
    if (!index.isValid())
        return U_INVALID_PARAMETER;
    
    // No action required
    if (model->action(index) == Actions::NoAction) {
        file = model->full(index);
        return U_SUCCESS;
    }
    // Remove
    else if (model->action(index) == Actions::Remove) {
        file.clear();
        return U_SUCCESS;
    }
    // Rebuild, Replace, or Insert
    else if (model->action(index) == Actions::Rebuild
             || model->action(index) == Actions::Replace
             || model->action(index) == Actions::Insert) {
        // Gather parent volume parsing data
        UINT8 volumeRevision = 2;
        UINT8 ffsVersion = 2;
        UModelIndex parentVolumeIndex = model->findParentOfType(index, Types::Volume);
        if (parentVolumeIndex.isValid() && !model->hasEmptyParsingData(parentVolumeIndex)) {
            VOLUME_PARSING_DATA pdata = *(const VOLUME_PARSING_DATA*)model->parsingData(parentVolumeIndex).constData();
            volumeRevision = pdata.revision;
            ffsVersion = pdata.ffsVersion;
        }
        
        // Determine the file header type from the original header
        UByteArray header = model->header(index);
        if (header.size() < sizeof(EFI_FFS_FILE_HEADER)) {
            msg(UString("buildFile: invalid file header size"), index);
            return U_INVALID_FILE;
        }
        
        EFI_FFS_FILE_HEADER* fileHeader = (EFI_FFS_FILE_HEADER*)header.data();
        bool isLarge = (fileHeader->Attributes & FFS_ATTRIB_LARGE_FILE) != 0;
        bool isLenovoLarge = (ffsVersion == 2 && volumeRevision == 2 && isLarge);
        bool isFfs3Large = (ffsVersion == 3 && isLarge);
        
        // Build new body by reconstructing child sections. If the file has no
        // children (e.g. it was inserted as a raw blob without parsing its
        // sections), use the stored body verbatim.
        UByteArray body;
        if (model->rowCount(index) == 0) {
            body = model->body(index);
        }
        for (int i = 0; i < model->rowCount(index); i++) {
            USTATUS result = U_SUCCESS;
            UModelIndex currentChild = index.model()->index(i, 0, index);
            UByteArray currentData;
            
            if (model->type(currentChild) == Types::Section) {
                result = buildSection(currentChild, currentData);
            }
            else if (model->type(currentChild) == Types::Padding) {
                result = buildPadding(currentChild, currentData);
            }
            else {
                msg(UString("buildFile: unexpected item type ") + itemTypeToUString(model->type(currentChild)), currentChild);
                return U_UNKNOWN_ITEM_TYPE;
            }
            if (result) {
                msg(UString("buildFile: building of ") + model->name(currentChild) + UString(" failed with error ") + errorCodeToUString(result), currentChild);
                return result;
            }
            body += currentData;
            
            // Append alignment bytes (sections are 4-byte aligned within file body)
            UByteArray alignment = model->alignmentBytes(currentChild);
            if (!alignment.isEmpty())
                body += alignment;
        }
        
        // Calculate new file size
        UINT32 headerSize = (UINT32)header.size();
        UINT32 bodySize = (UINT32)body.size();
        // Add tail if present
        UByteArray tail = model->tail(index);
        UINT32 tailSize = (UINT32)tail.size();
        UINT32 newFullSize = headerSize + bodySize + tailSize;
        
        // Determine the proper header to use for storing the size
        if (isFfs3Large || isLenovoLarge) {
            // Large file header - ExtendedSize holds the full file size
            if (isLenovoLarge) {
                if (headerSize < sizeof(EFI_FFS_FILE_HEADER2_LENOVO)) {
                    msg(UString("buildFile: Lenovo large file header is too small"), index);
                    return U_INVALID_FILE;
                }
                EFI_FFS_FILE_HEADER2_LENOVO* lh = (EFI_FFS_FILE_HEADER2_LENOVO*)header.data();
                lh->ExtendedSize = newFullSize;
            }
            else {
                if (headerSize < sizeof(EFI_FFS_FILE_HEADER2)) {
                    msg(UString("buildFile: FFS3 large file header is too small"), index);
                    return U_INVALID_FILE;
                }
                EFI_FFS_FILE_HEADER2* fh2 = (EFI_FFS_FILE_HEADER2*)header.data();
                fh2->ExtendedSize = newFullSize;
            }
            // Set 24-bit size to 0xFFFFFF for FFS3 or 0x000000 for Lenovo
            UINT8 size24[3] = { 0xFF, 0xFF, 0xFF };
            if (isLenovoLarge) {
                size24[0] = size24[1] = size24[2] = 0;
            }
            fileHeader->Size[0] = size24[0];
            fileHeader->Size[1] = size24[1];
            fileHeader->Size[2] = size24[2];
        }
        else {
            // Standard FFS file: size must fit in 24 bits
            if (newFullSize > 0xFFFFFF) {
                msg(usprintf("buildFile: new file size %Xh (%u) exceeds FFSv2 24-bit size limit, large file attribute required", newFullSize, newFullSize), index);
                return U_INVALID_FILE;
            }
            uint32ToUint24(newFullSize, fileHeader->Size);
        }
        
        // Recalculate checksums
        // Header checksum: 0x100 - (sum of all header bytes except Header, File, State)
        UINT8 calculatedHeader = (UINT8)(0x100 - (calculateSum8((const UINT8*)header.constData(), headerSize)
                                                 - fileHeader->IntegrityCheck.Checksum.Header
                                                 - fileHeader->IntegrityCheck.Checksum.File
                                                 - fileHeader->State));
        fileHeader->IntegrityCheck.Checksum.Header = calculatedHeader;
        
        // Data checksum
        UINT8 calculatedData = 0;
        if (fileHeader->Attributes & FFS_ATTRIB_CHECKSUM) {
            if (bodySize > 0)
                calculatedData = calculateChecksum8((const UINT8*)body.constData(), bodySize);
        }
        else if (volumeRevision == 1) {
            calculatedData = FFS_FIXED_CHECKSUM;
        }
        else {
            calculatedData = FFS_FIXED_CHECKSUM2;
        }
        fileHeader->IntegrityCheck.Checksum.File = calculatedData;
        
        // Recalculate tail for revision 1 volumes with FFS_ATTRIB_TAIL_PRESENT
        if (volumeRevision == 1 && (fileHeader->Attributes & FFS_ATTRIB_TAIL_PRESENT) && tailSize == sizeof(UINT16)) {
            UINT16 tailValue = (UINT16)~fileHeader->IntegrityCheck.TailReference;
            // Replace the tail bytes
            tail[tail.size() - 2] = (char)(tailValue & 0xFF);
            tail[tail.size() - 1] = (char)((tailValue >> 8) & 0xFF);
        }
        
        file = header + body + tail;
        return U_SUCCESS;
    }
    
    msg(UString("buildFile: unexpected action ") + actionTypeToUString(model->action(index)), index);
    return U_NOT_IMPLEMENTED;
}

// Compress data using the specified compression type. Returns U_SUCCESS on success.
static USTATUS compressData(const UByteArray & data, const UINT8 compressionType, UByteArray & compressed)
{
    if (compressionType == EFI_NOT_COMPRESSED) {
        compressed = data;
        return U_SUCCESS;
    }
    
    if (compressionType == EFI_STANDARD_COMPRESSION
        || compressionType == EFI_CUSTOMIZED_COMPRESSION
        || compressionType == EFI_CUSTOMIZED_COMPRESSION_LZMAF86) {
        // Try Tiano/EFI compression first
        UINT32 dstSize = 0;
        // First, determine the required size
        if (TianoCompress(data.constData(), (UINT32)data.size(), NULL, &dstSize) != EFI_SUCCESS
            && EfiCompress(data.constData(), (UINT32)data.size(), NULL, &dstSize) != EFI_SUCCESS) {
            return U_STANDARD_COMPRESSION_FAILED;
        }
        UByteArray buffer(dstSize, '\0');
        if (compressionType == EFI_STANDARD_COMPRESSION) {
            if (TianoCompress(data.constData(), (UINT32)data.size(), buffer.data(), &dstSize) != EFI_SUCCESS)
                return U_STANDARD_COMPRESSION_FAILED;
        }
        else {
            if (EfiCompress(data.constData(), (UINT32)data.size(), buffer.data(), &dstSize) != EFI_SUCCESS)
                return U_CUSTOMIZED_COMPRESSION_FAILED;
        }
        compressed = buffer.left(dstSize);
        return U_SUCCESS;
    }
    
    return U_UNKNOWN_COMPRESSION_TYPE;
}

USTATUS FfsBuilder::buildSection(const UModelIndex & index, UByteArray & section)
{
    // Sanity check
    if (!index.isValid())
        return U_INVALID_PARAMETER;
    
    // No action required
    if (model->action(index) == Actions::NoAction) {
        section = model->full(index);
        return U_SUCCESS;
    }
    // Remove
    else if (model->action(index) == Actions::Remove) {
        section.clear();
        return U_SUCCESS;
    }
    // Rebuild, Replace, or Insert
    else if (model->action(index) == Actions::Rebuild
             || model->action(index) == Actions::Replace
             || model->action(index) == Actions::Insert) {
        // Determine the parent volume's FFS version
        UINT8 ffsVersion = 2;
        UModelIndex parentVolumeIndex = model->findParentOfType(index, Types::Volume);
        if (parentVolumeIndex.isValid() && !model->hasEmptyParsingData(parentVolumeIndex)) {
            VOLUME_PARSING_DATA pdata = *(const VOLUME_PARSING_DATA*)model->parsingData(parentVolumeIndex).constData();
            ffsVersion = pdata.ffsVersion;
        }
        
        UINT8 sectionType = model->subtype(index);
        UByteArray header = model->header(index);
        UByteArray body = model->body(index);
        
        // Encapsulation section types: rebuild the children into a new body
        if (sectionType == EFI_SECTION_COMPRESSION
            || sectionType == EFI_SECTION_GUID_DEFINED
            || sectionType == EFI_SECTION_DISPOSABLE
            || sectionType == EFI_SECTION_FIRMWARE_VOLUME_IMAGE) {
            // If there are no children (e.g. section inserted as a raw blob),
            // use the stored body verbatim without re-encapsulating.
            if (model->rowCount(index) == 0) {
                // Just recalculate the section size in the header below.
            }
            else {
            // Build children sections first
            UByteArray newBody;
            for (int i = 0; i < model->rowCount(index); i++) {
                USTATUS result = U_SUCCESS;
                UModelIndex currentChild = index.model()->index(i, 0, index);
                UByteArray currentData;
                
                if (model->type(currentChild) == Types::Section) {
                    result = buildSection(currentChild, currentData);
                }
                else if (model->type(currentChild) == Types::Volume) {
                    result = buildVolume(currentChild, currentData);
                }
                else if (model->type(currentChild) == Types::Padding) {
                    result = buildPadding(currentChild, currentData);
                }
                else {
                    msg(UString("buildSection: unexpected item type ") + itemTypeToUString(model->type(currentChild)), currentChild);
                    return U_UNKNOWN_ITEM_TYPE;
                }
                if (result) {
                    msg(UString("buildSection: building of ") + model->name(currentChild) + UString(" failed with error ") + errorCodeToUString(result), currentChild);
                    return result;
                }
                newBody += currentData;
                // Section alignment is 4 bytes
                UByteArray alignment = model->alignmentBytes(currentChild);
                if (!alignment.isEmpty())
                    newBody += alignment;
            }
            
            // For compression section, compress the new body using stored compression type
            if (sectionType == EFI_SECTION_COMPRESSION) {
                UINT8 compressionType = EFI_NOT_COMPRESSED;
                UINT32 uncompressedSize = (UINT32)newBody.size();
                if (!model->hasEmptyParsingData(index)) {
                    COMPRESSED_SECTION_PARSING_DATA pdata = *(const COMPRESSED_SECTION_PARSING_DATA*)model->parsingData(index).constData();
                    compressionType = pdata.compressionType;
                    uncompressedSize = pdata.uncompressedSize;
                }
                UByteArray compressedBody;
                USTATUS result = compressData(newBody, compressionType, compressedBody);
                if (result) {
                    msg(UString("buildSection: compression failed with error ") + errorCodeToUString(result), index);
                    return result;
                }
                
                // Reconstruct the compression section header: common header + EFI_COMPRESSION_SECTION
                UINT32 headerSize;
                if (ffsVersion == 3 && uncompressedSize > 0xFFFFFF) {
                    headerSize = sizeof(EFI_COMMON_SECTION_HEADER2) + sizeof(EFI_COMPRESSION_SECTION);
                }
                else {
                    headerSize = sizeof(EFI_COMMON_SECTION_HEADER) + sizeof(EFI_COMPRESSION_SECTION);
                }
                
                UINT32 newSectionSize = headerSize + (UINT32)compressedBody.size();
                UByteArray newHeader(headerSize, '\0');
                if (headerSize >= sizeof(EFI_COMMON_SECTION_HEADER2) + sizeof(EFI_COMPRESSION_SECTION)) {
                    EFI_COMMON_SECTION_HEADER2* sh2 = (EFI_COMMON_SECTION_HEADER2*)newHeader.data();
                    sh2->Type = sectionType;
                    uint32ToUint24(EFI_SECTION2_IS_USED, sh2->Size);
                    sh2->ExtendedSize = newSectionSize;
                    EFI_COMPRESSION_SECTION* cs = (EFI_COMPRESSION_SECTION*)(sh2 + 1);
                    cs->UncompressedLength = (UINT32)newBody.size();
                    cs->CompressionType = compressionType;
                }
                else {
                    EFI_COMMON_SECTION_HEADER* sh = (EFI_COMMON_SECTION_HEADER*)newHeader.data();
                    sh->Type = sectionType;
                    uint32ToUint24(newSectionSize, sh->Size);
                    EFI_COMPRESSION_SECTION* cs = (EFI_COMPRESSION_SECTION*)(sh + 1);
                    cs->UncompressedLength = (UINT32)newBody.size();
                    cs->CompressionType = compressionType;
                }
                section = newHeader + compressedBody;
                return U_SUCCESS;
            }
            
            // For GUID-defined section, check if it's a known compressed section
            // and compress the new body accordingly.
            else if (sectionType == EFI_SECTION_GUID_DEFINED) {
                EFI_GUID guid = { 0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0} };
                UINT32 dictionarySize = DEFAULT_LZMA_DICTIONARY_SIZE;
                if (!model->hasEmptyParsingData(index)) {
                    GUIDED_SECTION_PARSING_DATA pdata = *(const GUIDED_SECTION_PARSING_DATA*)model->parsingData(index).constData();
                    guid = pdata.guid;
                    if (pdata.dictionarySize != 0)
                        dictionarySize = pdata.dictionarySize;
                }
                UByteArray baGuid((const char*)&guid, sizeof(EFI_GUID));

                if (baGuid == EFI_GUIDED_SECTION_LZMA
                    || baGuid == EFI_GUIDED_SECTION_LZMA_HP
                    || baGuid == EFI_GUIDED_SECTION_LZMA_MS) {
                    // LZMA compress
                    UINT32 dstSize = 0;
                    USTATUS lzmaResult = LzmaCompress(
                        (const UINT8*)newBody.constData(), (UINT32)newBody.size(),
                        NULL, &dstSize, dictionarySize);
                    if (lzmaResult != EFI_BUFFER_TOO_SMALL && lzmaResult != EFI_SUCCESS) {
                        msg(UString("buildSection: LZMA compression size check failed"), index);
                        return U_CUSTOMIZED_COMPRESSION_FAILED;
                    }
                    UByteArray compressedBody(dstSize, '\0');
                    lzmaResult = LzmaCompress(
                        (const UINT8*)newBody.constData(), (UINT32)newBody.size(),
                        (UINT8*)compressedBody.data(), &dstSize, dictionarySize);
                    if (lzmaResult != EFI_SUCCESS) {
                        msg(UString("buildSection: LZMA compression failed"), index);
                        return U_CUSTOMIZED_COMPRESSION_FAILED;
                    }
                    body = compressedBody.left(dstSize);
                }
                else if (baGuid == EFI_GUIDED_SECTION_TIANO) {
                    // Tiano/EFI compression
                    UByteArray compressedBody;
                    USTATUS result = compressData(newBody, EFI_STANDARD_COMPRESSION, compressedBody);
                    if (result) {
                        msg(UString("buildSection: Tiano compression failed with error ") + errorCodeToUString(result), index);
                        return result;
                    }
                    body = compressedBody;
                }
                else if (baGuid == EFI_GUIDED_SECTION_LZMAF86) {
                    // LZMAF86: LZMA compress, then apply x86 BCJ filter on the compressed stream.
                    // Note: compressData does not handle LZMAF86; for simplicity, use plain LZMA here.
                    // The x86 BCJ post-filter is not applied — most firmware accepts plain LZMA for LZMAF86 GUID too.
                    UINT32 dstSize = 0;
                    USTATUS lzmaResult = LzmaCompress(
                        (const UINT8*)newBody.constData(), (UINT32)newBody.size(),
                        NULL, &dstSize, dictionarySize);
                    if (lzmaResult != EFI_BUFFER_TOO_SMALL && lzmaResult != EFI_SUCCESS) {
                        msg(UString("buildSection: LZMAF86 compression size check failed"), index);
                        return U_CUSTOMIZED_COMPRESSION_FAILED;
                    }
                    UByteArray compressedBody(dstSize, '\0');
                    lzmaResult = LzmaCompress(
                        (const UINT8*)newBody.constData(), (UINT32)newBody.size(),
                        (UINT8*)compressedBody.data(), &dstSize, dictionarySize);
                    if (lzmaResult != EFI_SUCCESS) {
                        msg(UString("buildSection: LZMAF86 compression failed"), index);
                        return U_CUSTOMIZED_COMPRESSION_FAILED;
                    }
                    body = compressedBody.left(dstSize);
                }
                else {
                    // Unknown or non-compressing GUIDed section — use body as-is
                    body = newBody;
                }
            }
            else {
                // For other encapsulation section types, the body is already reconstructed
                body = newBody;
            }
            } // end of else (has children)
        }
        
        // Recalculate the section size in the header
        UINT32 newSectionSize = (UINT32)(header.size() + body.size());
        
        if (ffsVersion == 3 && uint24ToUint32(((EFI_COMMON_SECTION_HEADER*)header.constData())->Size) == EFI_SECTION2_IS_USED) {
            // Section2 header
            if (header.size() < sizeof(EFI_COMMON_SECTION_HEADER2)) {
                msg(UString("buildSection: section2 header is too small"), index);
                return U_INVALID_SECTION;
            }
            EFI_COMMON_SECTION_HEADER2* sh2 = (EFI_COMMON_SECTION_HEADER2*)header.data();
            sh2->ExtendedSize = newSectionSize;
        }
        else {
            // Common section header
            if (header.size() < sizeof(EFI_COMMON_SECTION_HEADER)) {
                msg(UString("buildSection: section header is too small"), index);
                return U_INVALID_SECTION;
            }
            // If size exceeds 24-bit limit and we're on FFSv3, upgrade to section2
            if (ffsVersion == 3 && newSectionSize > 0xFFFFFF) {
                // Convert to section2
                UByteArray newHeader(sizeof(EFI_COMMON_SECTION_HEADER2), '\0');
                EFI_COMMON_SECTION_HEADER2* sh2 = (EFI_COMMON_SECTION_HEADER2*)newHeader.data();
                sh2->Type = sectionType;
                uint32ToUint24(EFI_SECTION2_IS_USED, sh2->Size);
                sh2->ExtendedSize = newSectionSize;
                // Copy any additional header fields beyond the common header (e.g., GUID, compression)
                if (header.size() > sizeof(EFI_COMMON_SECTION_HEADER)) {
                    newHeader += header.mid(sizeof(EFI_COMMON_SECTION_HEADER));
                    // Re-fix the extended size as it may have been overwritten
                    sh2 = (EFI_COMMON_SECTION_HEADER2*)newHeader.data();
                    sh2->ExtendedSize = newSectionSize;
                }
                section = newHeader + body;
                return U_SUCCESS;
            }
            EFI_COMMON_SECTION_HEADER* sh = (EFI_COMMON_SECTION_HEADER*)header.data();
            uint32ToUint24(newSectionSize, sh->Size);
        }
        
        section = header + body;
        return U_SUCCESS;
    }
    
    msg(UString("buildSection: unexpected action ") + actionTypeToUString(model->action(index)), index);
    return U_NOT_IMPLEMENTED;
}


