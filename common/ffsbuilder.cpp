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

    erased = UByteArray(model->header(index).size() + model->body(index).size() + model->tail(index).size(), emptyByte);

    return U_SUCCESS;
}

USTATUS FfsBuilder::build(const UModelIndex & index, UByteArray & reconstructed)
{
    if (!index.isValid())
        return U_SUCCESS;

    USTATUS result;

    switch (model->type(index)) {
    case Types::Image:
        if (model->subtype(index) == Subtypes::IntelImage) {
            result = buildIntelImage(index, reconstructed);
            if (result)
                return result;
        }
        else {
            //Other images types can be reconstructed like regions
            result = buildRegion(index, reconstructed);
            if (result)
                return result;
        }
        break;

    case Types::Capsule:
        result = buildCapsule(index, reconstructed);
        if (result)
            return result;
        break;

    case Types::Region:
        result = buildRegion(index, reconstructed);
        if (result)
            return result;
        break;

    case Types::Padding:
        result = buildPadding(index, reconstructed);
        if (result)
            return result;
        break;

    case Types::Volume:
        // Nvram rebuild support
        if(model->subtype(index) == Subtypes::NvramVolume)
            result = buildNvramVolume(index, reconstructed);
        else
            result = buildVolume(index, reconstructed);

        if (result)
            return result;
        break;

    case Types::Section:
        result = buildSection(index, 0, reconstructed);
        if (result)
            return result;
        break;
    default:
        msg(usprintf("build: unknown item type %1").arg(model->type(index)), index);
        return U_UNKNOWN_ITEM_TYPE;
    }

    return U_SUCCESS;
}

USTATUS FfsBuilder::buildRegion(const UModelIndex& index, UByteArray& reconstructed, bool includeHeader)
{
    if (!index.isValid())
        return U_SUCCESS;

    USTATUS result;

    // No action
    if (model->action(index) == Actions::NoAction) {
        reconstructed = model->header(index).append(model->body(index));
        return U_SUCCESS;
    }
    else if (model->action(index) == Actions::Remove) {
        reconstructed.clear();
        return U_SUCCESS;
    }
    else if (model->action(index) == Actions::Rebuild ||
        model->action(index) == Actions::Replace) {
        if (model->rowCount(index)) {
            reconstructed.clear();
            // Reconstruct children
            for (int i = 0; i < model->rowCount(index); i++) {
                UByteArray child;
                result = build(index.child(i, 0), child);
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
            msg("buildRegion: reconstructed region size is bigger then original ",
                index);
            return U_INVALID_PARAMETER;
        }
        else if (reconstructed.size() < model->body(index).size()) {
            msg("buildRegion: reconstructed region size is smaller then original ",
                index);
            return U_INVALID_PARAMETER;
        }

        // Reconstruction successful
        if (includeHeader)
            reconstructed = model->header(index).append(reconstructed);
        return U_SUCCESS;
    }

    // All other actions are not supported
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
        capsule = model->header(index) + model->body(index) + model->tail(index);
        return U_SUCCESS;
    }

    // Rebuild or Replace
    else if (model->action(index) == Actions::Rebuild 
        || model->action(index) == Actions::Replace) {
        if (model->rowCount(index)) {
            // Clear the supplied UByteArray
            capsule.clear();

            // Right now there is only one capsule image element supported
            if (model->rowCount(index) != 1) {
                msg(usprintf("buildCapsule: building of capsules with %d items is not yet supported", model->rowCount(index)), index);
                return U_NOT_IMPLEMENTED;
            }
            
            // Build image
            UModelIndex imageIndex = index.child(0, 0);
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
                    capsule.append(imageData);
            }
            else {
                msg(UString("buildCapsule: unexpected item type ") + itemTypeToUString(model->type(imageIndex)), imageIndex);
                return U_UNKNOWN_ITEM_TYPE;
            }
            
            // Check size of reconstructed capsule body, it must remain the same
            UINT32 newSize = capsule.size();
            UINT32 oldSize = model->body(index).size();
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
        intelImage = model->header(index) + model->body(index) + model->tail(index);
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
        intelImage = model->header(index.child(0, 0)) + model->body(index.child(0, 0)) + model->tail(index.child(0, 0));
        
        // Process other regions
        for (int i = 1; i < model->rowCount(index); i++) {
            UModelIndex currentRegion = index.child(i, 0);

            // Skip regions with Remove action
            if (model->action(currentRegion) == Actions::Remove)
                continue;

            // Check item type to be either region or padding
            UINT8 type = model->type(currentRegion);
            if (type == Types::Padding) {
                // Add padding as is
                intelImage.append(model->header(currentRegion) + model->body(currentRegion) + model->tail(currentRegion));
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
                region = model->header(currentRegion).append(model->body(currentRegion));
                break;
            default:
                msg(UString("buildIntelImage: unknown region type"), currentRegion);
                return U_UNKNOWN_ITEM_TYPE;
            }

            // Append the resulting region
            intelImage.append(region);
        }
        
        // Check size of new image, it must be same as old one
        UINT32 newSize = intelImage.size();
        UINT32 oldSize = model->body(index).size();
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

USTATUS FfsBuilder::buildRawArea(const UModelIndex & index, UByteArray & rawArea, bool includeHeader)
{
    // Sanity check
    if (!index.isValid())
        return U_INVALID_PARAMETER;

    // No action required
    if (model->action(index) == Actions::NoAction) {
        rawArea = model->header(index) + model->body(index) + model->tail(index);
        return U_SUCCESS;
    }
    // Remove
    else if (model->action(index) == Actions::Remove) {
        rawArea.clear();
        return U_SUCCESS;
    }
    // Rebuild or Replace
    else if (model->action(index) == Actions::Rebuild 
        || model->action(index) == Actions::Replace) {
        // Rebuild if there is at least 1 child
        if (model->rowCount(index)) {
            // Clear the supplied UByteArray
            rawArea.clear();

            // Build children
            for (int i = 0; i < model->rowCount(index); i++) {
                USTATUS result = U_SUCCESS;
                UModelIndex currentChild = index.child(i, 0);
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
                rawArea.append(currentData);
            }

            // Check size of new raw area, it must be same as original one
            UINT32 newSize = rawArea.size();
            UINT32 oldSize = model->body(index).size();
            if (newSize > oldSize) {
                msg(usprintf("buildRawArea: new area size %Xh (%u) is bigger than the original %Xh (%u)", newSize, newSize, oldSize, oldSize), index);
                return U_INVALID_RAW_AREA;
            }
            else if (newSize < oldSize) {
                msg(usprintf("buildRawArea: new area size %Xh (%u) is smaller than the original %Xh (%u)", newSize, newSize, oldSize, oldSize), index);
                return U_INVALID_RAW_AREA;
            }
        }
        // No need to rebuild a raw area with no children
        else {
            rawArea = model->body(index);
        }

        // Build successful, add header if needed
        if(includeHeader)
            rawArea = model->header(index) + rawArea + model->tail(index);
        else
            rawArea = rawArea + model->tail(index);

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
        padding = model->header(index) + model->body(index) + model->tail(index);
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
        data = model->header(index) + model->body(index) + model->tail(index);
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

    // TODO: rebuild properly

    msg(UString("buildNoUefiData: unexpected action " + actionTypeToUString(model->action(index))), index);
    return U_NOT_IMPLEMENTED;
}

USTATUS FfsBuilder::buildFreeSpace(const UModelIndex & index, UByteArray & freeSpace)
{
    // Sanity check
    if (!index.isValid())
        return U_INVALID_PARAMETER;

    // No actions possible for free space
    freeSpace = model->header(index) + model->body(index) + model->tail(index);
    return U_SUCCESS;
}

USTATUS FfsBuilder::buildVolume(const UModelIndex & index, UByteArray & volume)
{
    if (!index.isValid())
        return U_SUCCESS;

    USTATUS result;

    // No action
    if (model->action(index) == Actions::NoAction) {
        volume = model->header(index).append(model->body(index));
        return U_SUCCESS;
    }
    else if (model->action(index) == Actions::Remove) {
        volume.clear();
        return U_SUCCESS;
    }
    else if (model->action(index) == Actions::Replace ||
             model->action(index) == Actions::Rebuild) {
        UByteArray header = model->header(index);
        UByteArray body = model->body(index);
        EFI_FIRMWARE_VOLUME_HEADER* volumeHeader = (EFI_FIRMWARE_VOLUME_HEADER*)header.data();

        // Check sanity of HeaderLength
        if (volumeHeader->HeaderLength > header.size()) {
            msg(UString("buildVolume: invalid volume header length, reconstruction is not possible"), index);
            return U_INVALID_VOLUME;
        }

        // Recalculate volume header checksum
        volumeHeader->Checksum = 0;
        volumeHeader->Checksum = calculateChecksum16((const UINT16*)volumeHeader, volumeHeader->HeaderLength);

        // Get volume size
        UINT32 volumeSize = header.size() + body.size();

        // Reconstruct volume body
        UINT32 freeSpaceOffset = 0;
        if (model->rowCount(index)) {
            volume.clear();
            UINT8 polarity = volumeHeader->Attributes & EFI_FVB_ERASE_POLARITY ? ERASE_POLARITY_TRUE : ERASE_POLARITY_FALSE;
            char empty = volumeHeader->Attributes & EFI_FVB_ERASE_POLARITY ? '\xFF' : '\x00';

            // Calculate volume base for volume
            UINT32 volumeBase;
            UByteArray file;
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
                for (UModelIndex parentIndex = index.parent(); model->type(parentIndex) != Types::Root; parentIndex = parentIndex.parent()) {
                    UByteArray data = model->parsingData(parentIndex);
                    const COMPRESSED_SECTION_PARSING_DATA* pdata = (const COMPRESSED_SECTION_PARSING_DATA*)data.constData();

                    if (pdata->algorithm != COMPRESSION_ALGORITHM_NONE) {
                        // No rebase needed for compressed PEI files
                        baseFound = true;
                        volumeBase = 0;
                        break;
                    }
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
                        UModelIndex peiFile = index.child(i, 0);
                        UINT32 sectionOffset = sizeof(EFI_FFS_FILE_HEADER);
                        // BUGBUG: this parsing is bad and doesn't support large files, but it needs to be performed only for very old images with uncompressed DXE volumes, so whatever
                        // Search for PE32 or TE section
                        for (int j = 0; j < model->rowCount(peiFile); j++) {
                            if (model->subtype(peiFile.child(j, 0)) == EFI_SECTION_PE32 ||
                                    model->subtype(peiFile.child(j, 0)) == EFI_SECTION_TE) {
                                UModelIndex image = peiFile.child(j, 0);
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
            UByteArray padFileGuid = EFI_FFS_PAD_FILE_GUID;
            UByteArray vtf;
            UModelIndex vtfIndex;
            UINT32 nonUefiDataOffset = 0;
            UByteArray nonUefiData;
            for (int i = 0; i < model->rowCount(index); i++) {
                // Inside a volume can be files, free space or padding with non-UEFI data
                if (model->type(index.child(i, 0)) == Types::File) { // Next item is a file

                    // Align to 8 byte boundary
                    UINT32 alignment = offset % 8;
                    if (alignment) {
                        alignment = 8 - alignment;
                        offset += alignment;
                        volume.append(UByteArray(alignment, empty));
                    }

                    // Calculate file base
                    UINT32 fileBase = volumeBase ? volumeBase + header.size() + offset : 0;

                    // Reconstruct file
                    result = buildFile(index.child(i, 0), volumeHeader->Revision, polarity, fileBase, file);
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

                        // Parse non-empty pad file
                        if (model->rowCount(index.child(i, 0))) {
                            //TODO: handle it
                            continue;
                        }
                        // Skip empty pad-file
                        else
                            continue;
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
                        UByteArray pad;
                        result = buildPadFile(padFileGuid, size, volumeHeader->Revision, polarity, pad);
                        if (result)
                            return result;
                        // Append constructed pad file to volume body
                        volume.append(pad);
                        offset += size;
                    }

                    // Append current file to new volume body
                    volume.append(file);

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
                msg(usprintf("buildVolume: both VTF and non-UEFI data found in the volume, reconstruction is not possible"), index);
                return U_INVALID_VOLUME;
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
                    msg(usprintf("buildVolume: wrong size of the Volume Top File"), index);
                    return U_INVALID_FILE;
                }
                // Insert pad file to fill the gap
                if (vtfOffset > offset) {
                    // Determine pad file size
                    UINT32 size = vtfOffset - offset;
                    // Construct pad file
                    UByteArray pad;
                    result = buildPadFile(padFileGuid, size, volumeHeader->Revision, polarity, pad);
                    if (result)
                        return result;
                    // Append constructed pad file to volume body
                    volume.append(pad);
                }
                // No more space left in volume
                else if (offset > vtfOffset) {
                    msg(usprintf("buildVolume: no space left to insert VTF, need %xh (%d) byte(s) more",
                        offset - vtfOffset, offset - vtfOffset), index);
                    return U_INVALID_VOLUME;
                }

                // Calculate VTF base
                UINT32 vtfBase = volumeBase ? volumeBase + vtfOffset : 0;

                // Reconstruct VTF again
                result = buildFile(vtfIndex, volumeHeader->Revision, polarity, vtfBase, vtf);
                if (result)
                    return result;

                // Patch VTF
                if (!parser->peiCoreEntryPoint) {
                    msg("patchVtf: PEI Core entry point can't be determined. VTF can't be patched.", index);
                    return U_PEI_CORE_ENTRY_POINT_NOT_FOUND;
                }
                if (parser->newPeiCoreEntryPoint && parser->peiCoreEntryPoint != parser->newPeiCoreEntryPoint) {
                    // Replace last occurrence of oldPeiCoreEntryPoint with newPeiCoreEntryPoint
                    QByteArray old((char*)&parser->peiCoreEntryPoint, sizeof(parser->peiCoreEntryPoint));
                    int i = vtf.lastIndexOf(old);
                    if (i == -1)
                        msg("patchVtf: PEI Core entry point can't be found in VTF. VTF not patched.", index);
                    else {
                        UINT32* data = (UINT32*)(vtf.data() + i);
                        *data = parser->newPeiCoreEntryPoint;
                    }
                }

                // Append VTF
                volume.append(vtf);
            }
            else if (!nonUefiData.isEmpty()) { //Non-UEFI data found
                // No space left
                if (offset > nonUefiDataOffset) {
                    msg(usprintf("buildVolume: no space left to insert non-UEFI data, need %xh (%d) byte(s) more",
                        offset - nonUefiDataOffset, offset - nonUefiDataOffset), index);
                    return U_INVALID_VOLUME;
                }
                // Append additional free space
                else if (nonUefiDataOffset > offset) {
                    volume.append(UByteArray(nonUefiDataOffset - offset, empty));
                }

                // Append VTF
                volume.append(nonUefiData);
            }
            else {
                // Fill the rest of volume space with empty char
                if (body.size() > volume.size()) {
                    // Fill volume end with empty char
                    volume.append(UByteArray(body.size() - volume.size(), empty));
                }
                else if (body.size() < volume.size()) {
                    // Check if volume can be grown
                    // Root volume can't be grown
                    UINT8 parentType = model->type(index.parent());
                    if (parentType != Types::File && parentType != Types::Section) {
                        msg("buildVolume: root volume can't be grown", index);
                        return U_INVALID_VOLUME;
                    }

                    // Grow volume to fit new body
                    UINT32 newSize = header.size() + volume.size();
                    result = growVolume(header, volumeSize, newSize);
                    if (result)
                        return result;

                    // Fill volume end with empty char
                    volume.append(UByteArray(newSize - header.size() - volume.size(), empty));
                    volumeSize = newSize;
                }
            }
        }
        // Use current volume body
        else {
            volume = model->body(index);

            // BUGBUG: volume size may change during this operation for volumes withour files in them
            // but such volumes are fairly rare
        }

        // Check new volume size
        if ((UINT32)(header.size() + volume.size()) != volumeSize) {
            msg("buildVolume: volume size can't be changed", index);
            return U_INVALID_VOLUME;
        }

        // Reconstruction successful
        volume = header.append(volume);

        // Recalculate CRC32 in ZeroVector, if needed
        if (model->text(index).contains("AppleCRC32 ")) {
            // Get current CRC32 value from volume header
            const UINT32 currentCrc = *(const UINT32*)(volume.constData() + 8);
            // Calculate new value
            UINT32 crc = crc32(0, (const UINT8*)volume.constData() + volumeHeader->HeaderLength, volume.size() - volumeHeader->HeaderLength);
            // Update the value
            if (currentCrc != crc) {
                *(UINT32*)(volume.data() + 8) = crc;

                // Recalculate header checksum
                volumeHeader = (EFI_FIRMWARE_VOLUME_HEADER*)volume.data();
                volumeHeader->Checksum = 0;
                volumeHeader->Checksum = calculateChecksum16((const UINT16*)volumeHeader, volumeHeader->HeaderLength);
            }
        }

        // Store new free space offset, if needed
        if (model->text(index).contains("AppleFSO ")) {
            // Get current CRC32 value from volume header
            const UINT32 currentFso = *(const UINT32*)(volume.constData() + 12);
            // Update the value
            if (freeSpaceOffset != 0 && currentFso != freeSpaceOffset) {
                *(UINT32*)(volume.data() + 12) = freeSpaceOffset;

                // Recalculate header checksum
                volumeHeader = (EFI_FIRMWARE_VOLUME_HEADER*)volume.data();
                volumeHeader->Checksum = 0;
                volumeHeader->Checksum = calculateChecksum16((const UINT16*)volumeHeader, volumeHeader->HeaderLength);
            }
        }

        return U_SUCCESS;
    }

    return U_NOT_IMPLEMENTED;
}

USTATUS FfsBuilder::buildNvramVolume(const UModelIndex & index, UByteArray & volume)
{
    if (!index.isValid())
        return U_SUCCESS;

    USTATUS result;

    // No action
    if (model->action(index) == Actions::NoAction) {
        volume = model->header(index).append(model->body(index));
        return U_SUCCESS;
    }
    else if (model->action(index) == Actions::Remove) {
        volume.clear();
        return U_SUCCESS;
    }
    else if (model->action(index) == Actions::Replace ||
             model->action(index) == Actions::Rebuild) {
        UByteArray header = model->header(index);
        UByteArray body = model->body(index);
        EFI_FIRMWARE_VOLUME_HEADER* volumeHeader = (EFI_FIRMWARE_VOLUME_HEADER*)header.data();

        // Check sanity of HeaderLength
        if (volumeHeader->HeaderLength > header.size()) {
            msg(UString("buildNvramVolume: invalid volume header length, reconstruction is not possible"), index);
            return U_INVALID_VOLUME;
        }

        // Recalculate volume header checksum
        volumeHeader->Checksum = 0;
        volumeHeader->Checksum = calculateChecksum16((const UINT16*)volumeHeader, volumeHeader->HeaderLength);

        volume.clear();

        for (int i = 0; i < model->rowCount(index); i++) {
            UModelIndex currentIndex = index.child(i, 0);
            UByteArray store;

            result = buildNvramStore(currentIndex, store);
            if(result)
                return result;

            // Element reconstruct success
            volume.append(store);
        }

        // Volume reconstruct success
        volume = header.append(volume);


        return U_SUCCESS;

    }

    return U_NOT_IMPLEMENTED;
}

USTATUS FfsBuilder::buildNvramStore(const UModelIndex & index, UByteArray & store)
{
    UByteArray header = model->header(index);
    UByteArray body  = model->body(index);
    UINT8 type = model->type(index);

    if(model->action(index) == Actions::Remove) {
        header.clear();
        body.clear();
    }
    else if(model->action(index) == Actions::Replace ||
            model->action(index) == Actions::Rebuild) {
        if(type == Types::FdcStore) {
            // Recalculate store header
            EFI_FIRMWARE_VOLUME_HEADER* volumeHeader = (EFI_FIRMWARE_VOLUME_HEADER*)header.data();
            volumeHeader->Checksum = 0;
            volumeHeader->Checksum = calculateChecksum16((const UINT16*)volumeHeader, volumeHeader->HeaderLength);

            // Rebuild VSS or VSS2 volume inside
            UByteArray vssStore;
            buildNvramStore(index.child(0, 0), vssStore);
            body = vssStore;
        }
        else if(type == Types::VssStore || type == Types::Vss2Store) {
            if(model->rowCount(index)) {
                body.clear();
                for (int i = 0; i < model->rowCount(index); i++) {
                    UModelIndex currentIndex = index.child(i, 0);
                    UByteArray currentHeader = model->header(currentIndex);
                    UByteArray currentBody = model->body(currentIndex);
                    UINT8 currentAction = model->action(currentIndex);

                    if(currentAction == Actions::Remove) {
                        currentHeader.clear();
                        currentBody.clear();
                    }
                    else if(currentAction == Actions::Rebuild ||
                            currentAction == Actions::Replace) {
                        // Recalculate all Apple variables crc's
                        if(model->subtype(currentIndex) == Subtypes::AppleVssEntry) {
                            VSS_APPLE_VARIABLE_HEADER* appleVariableHeader = (VSS_APPLE_VARIABLE_HEADER*)currentHeader.data();
                            appleVariableHeader->DataCrc32 = crc32(0, (const UINT8*)currentBody.constData(), currentBody.size());
                            //appleVariableHeader->DataSize = currentBody.size();
                        }
                    }
                    body.append(currentHeader.append(currentBody));
                }
            }
        }
        else if(type == Types::FsysStore) {
             UByteArray store = header + body;

            // Recalculate store checksum
            UINT32 calculatedCrc = crc32(0, (const UINT8*)store.constData(), (const UINT32)store.size() - sizeof(UINT32));
            // Write new checksum
            body.replace((const UINT32)body.size() - sizeof(UINT32), sizeof(UINT32), (const char*)calculatedCrc, sizeof(UINT32));
        }
        else if(type == Types::EvsaStore) {
            UByteArray store = header + body;

            // Recalculate header checksum
            const EVSA_STORE_ENTRY* evsaStoreHeader = (const EVSA_STORE_ENTRY*)store.constData();
            UINT8 storeCrc = calculateChecksum8(((const UINT8*)evsaStoreHeader) + 2, evsaStoreHeader->Header.Size - 2);
            // Write new checksum
            EVSA_ENTRY_HEADER* evsaEntryHeader = (EVSA_ENTRY_HEADER*)header.data();
            evsaEntryHeader->Checksum = storeCrc;

            // Recalculate all crc's
            if(model->rowCount(index)) {
                body.clear();
                for (int i = 0; i < model->rowCount(index); i++) {
                    UModelIndex currentIndex = index.child(i, 0);
                    UByteArray currentHeader = model->header(currentIndex);
                    UByteArray currentBody = model->body(currentIndex);
                    UINT8 currentSubtype = model->subtype(currentIndex);
                    UINT8 currentAction = model->action(currentIndex);

                    if(currentAction == Actions::Remove) {
                        currentHeader.clear();
                        currentBody.clear();
                    }
                    else if(currentAction == Actions::Rebuild ||
                            currentAction == Actions::Replace) {

                        if(currentSubtype == Subtypes::DataEvsaEntry ||
                                currentSubtype == Subtypes::GuidEvsaEntry ||
                                currentSubtype == Subtypes::NameEvsaEntry) {
                            UByteArray currentStore = currentHeader + currentBody;

                            // Recalculate header checksum
                            const EVSA_STORE_ENTRY* evsaStoreHeader = (const EVSA_STORE_ENTRY*)currentStore.constData();
                            UINT8 entryCrc = calculateChecksum8(((const UINT8*)evsaStoreHeader) + 2, evsaStoreHeader->Header.Size - 2);
                            // Write new checksum
                            EVSA_ENTRY_HEADER* evsaEntryHeader = (EVSA_ENTRY_HEADER*)currentHeader.data();
                            evsaEntryHeader->Checksum = entryCrc;
                        }
                    }

                    body.append(currentHeader.append(currentBody));
                }
            }
        }
        else if(type == Types::FtwStore) {
            // Recalculate block header checksum
            EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER32* crcFtwBlockHeader = (EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER32*)header.data();
            crcFtwBlockHeader->Crc = crc32(0, (const UINT8*)crcFtwBlockHeader, header.size());
        }
    }

    // Rebuild end
    store.clear();
    store = header.append(body);

    return U_SUCCESS;
}

USTATUS FfsBuilder::buildPadFile(const UByteArray & guid, const UINT32 size, const UINT8 revision, const UINT8 erasePolarity, UByteArray & pad)
{
    if (size < sizeof(EFI_FFS_FILE_HEADER) || erasePolarity == ERASE_POLARITY_UNKNOWN)
        return U_INVALID_PARAMETER;

    if (size >= 0xFFFFFF) // TODO: large file support
        return U_INVALID_PARAMETER;

    pad = UByteArray(size - guid.size(), erasePolarity == ERASE_POLARITY_TRUE ? '\xFF' : '\x00');
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

    return U_SUCCESS;
}

USTATUS FfsBuilder::buildFile(const UModelIndex & index, const UINT8 revision, const UINT8 erasePolarity, const UINT32 base, UByteArray & reconstructed)
{
    if (!index.isValid())
        return U_SUCCESS;

    USTATUS result;

    // No action
    if (model->action(index) == Actions::NoAction) {
        reconstructed = model->header(index).append(model->body(index));
        const EFI_FFS_FILE_HEADER* fileHeader = (const EFI_FFS_FILE_HEADER*)model->header(index).constData();
        // Append tail, if needed
        if (fileHeader->Attributes & FFS_ATTRIB_TAIL_PRESENT) {
            UINT8 ht = ~fileHeader->IntegrityCheck.Checksum.Header;
            UINT8 ft = ~fileHeader->IntegrityCheck.Checksum.File;
            reconstructed.append(ht).append(ft);
        }
        return U_SUCCESS;
    }
    else if (model->action(index) == Actions::Remove) {
        reconstructed.clear();
        return U_SUCCESS;
    }
    else if (model->action(index) == Actions::Insert ||
             model->action(index) == Actions::Replace ||
             model->action(index) == Actions::Rebuild) {
        UByteArray header = model->header(index);
        EFI_FFS_FILE_HEADER* fileHeader = (EFI_FFS_FILE_HEADER*)header.data();

        // Check erase polarity
        if (erasePolarity == ERASE_POLARITY_UNKNOWN) {
            msg("buildFile: unknown erase polarity", index);
            return U_INVALID_PARAMETER;
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
            msg("buildFile: file is HEADER_INVALID state, and will be removed from reconstructed image", index);
            return U_SUCCESS;
        }
        else if (state & EFI_FILE_DELETED) {
            // File marked to have been deleted form and must be deleted
            // Do not add anything to queue
            msg("buildFile: file is in DELETED state, and will be removed from reconstructed image", index);
            return U_SUCCESS;
        }
        else if (state & EFI_FILE_MARKED_FOR_UPDATE) {
            // File is marked for update, the mark must be removed
            msg("buildFile: file's MARKED_FOR_UPDATE state cleared", index);
        }
        else if (state & EFI_FILE_DATA_VALID) {
            // File is in good condition, reconstruct it
        }
        else if (state & EFI_FILE_HEADER_VALID) {
            // Header is valid, but data is not, so file must be deleted
            msg("buildFile: file is in HEADER_VALID (but not in DATA_VALID) state, and will be removed from reconstructed image", index);
            return U_SUCCESS;
        }
        else if (state & EFI_FILE_HEADER_CONSTRUCTION) {
            // Header construction not finished, so file must be deleted
            msg("buildFile: file is in HEADER_CONSTRUCTION (but not in DATA_VALID) state, and will be removed from reconstructed image", index);
            return U_SUCCESS;
        }

        // Reconstruct file body
        if (model->rowCount(index)) {
            reconstructed.clear();
            // Construct new file body
            // File contains raw data, must be parsed as region without header
            if (model->subtype(index) == EFI_FV_FILETYPE_ALL || model->subtype(index) == EFI_FV_FILETYPE_RAW) {
                result = buildRawArea(index, reconstructed, false);
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
                        reconstructed.append(UByteArray(alignment, '\x00'));
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
                    UByteArray section;
                    result = buildSection(index.child(i, 0), sectionBase, section);
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
                msg("buildFile: resulting file size is too big", index);
                return U_INVALID_FILE;
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
        return U_SUCCESS;
    }

    // All other actions are not supported
    return U_NOT_IMPLEMENTED;
}


USTATUS FfsBuilder::buildSection(const UModelIndex & index, const UINT32 base, UByteArray & reconstructed)
{
    if (!index.isValid())
        return U_SUCCESS;

    USTATUS result;

    // No action
    if (model->action(index) == Actions::NoAction) {
        reconstructed = model->header(index).append(model->body(index));
        return U_SUCCESS;
    }
    else if (model->action(index) == Actions::Remove) {
        reconstructed.clear();
        return U_SUCCESS;
    }
    else if (model->action(index) == Actions::Insert ||
             model->action(index) == Actions::Replace ||
             model->action(index) == Actions::Rebuild ||
             model->action(index) == Actions::Rebase) {
        UByteArray header = model->header(index);
        EFI_COMMON_SECTION_HEADER* commonHeader = (EFI_COMMON_SECTION_HEADER*)header.data();
        bool extended = false;
        UByteArray data = model->parsingData(index);
        const COMPRESSED_SECTION_PARSING_DATA* compress_data = (const COMPRESSED_SECTION_PARSING_DATA*)data.constData();

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
                    reconstructed.append(UByteArray(alignment, '\x00'));
                }

                // Reconstruct subsections
                UByteArray section;
                result = build(index.child(i, 0), section);
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
                // Compress new section body
                UByteArray compressed;
                result = compress(reconstructed, compress_data->algorithm, compressed);
                if (result)
                    return result;
                // Correct compression type
                compessionHeader->CompressionType = compress_data->compressionType;

                // Replace new section body
                reconstructed = compressed;
            }
            else if (model->subtype(index) == EFI_SECTION_GUID_DEFINED) {
                EFI_GUID_DEFINED_SECTION* guidDefinedHeader = (EFI_GUID_DEFINED_SECTION*)header.data();
                // Compress new section body
                UByteArray compressed;
                result = compress(reconstructed, compress_data->algorithm, compressed);
                if (result)
                    return result;
                // Check for authentication status valid attribute
                if (guidDefinedHeader->Attributes & EFI_GUIDED_SECTION_AUTH_STATUS_VALID) {
                    // CRC32 section
                    if (UByteArray((const char*)&guidDefinedHeader->SectionDefinitionGuid, sizeof(EFI_GUID)) == EFI_GUIDED_SECTION_CRC32) {
                        // Check header size
                        if ((UINT32)header.size() != sizeof(EFI_GUID_DEFINED_SECTION) + sizeof(UINT32)) {
                            msg("buildSection: invalid CRC32 section size", index);
                            return U_INVALID_SECTION;
                        }
                        // Calculate CRC32 of section data
                        UINT32 crc = crc32(0, (const UINT8*)compressed.constData(), compressed.size());
                        // Store new CRC32
                        *(UINT32*)(header.data() + sizeof(EFI_GUID_DEFINED_SECTION)) = crc;
                    }
                    else {
                        msg(usprintf("buildSection: GUID defined section authentication info can become invalid")
                            .arg(guidToUString(guidDefinedHeader->SectionDefinitionGuid)), index);
                    }
                }
                // Check for Intel signed section
                if (guidDefinedHeader->Attributes & EFI_GUIDED_SECTION_PROCESSING_REQUIRED
                        && UByteArray((const char*)&guidDefinedHeader->SectionDefinitionGuid, sizeof(EFI_GUID)) == EFI_FIRMWARE_CONTENTS_SIGNED_GUID) {
                    msg(usprintf("buildSection: GUID defined section signature can become invalid")
                        .arg(guidToUString(guidDefinedHeader->SectionDefinitionGuid)), index);
                }
                // Replace new section body
                reconstructed = compressed;
            }
            else if (compress_data->algorithm != COMPRESSION_ALGORITHM_NONE) {
                msg(usprintf("buildSection: incorrectly required compression for section of type %1")
                    .arg(model->subtype(index)), index);
                return U_INVALID_SECTION;
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
                result = rebase(reconstructed, base - teFixup + header.size());
                if (result) {
                    msg("buildSection: executable section rebase failed", index);
                    return result;
                }

                // Special case of PEI Core rebase
                if (model->subtype(index.parent()) == EFI_FV_FILETYPE_PEI_CORE) {
                   result = getEntryPoint(reconstructed, parser->newPeiCoreEntryPoint);
                   if (result)
                        msg("buildSection: can't get entry point of PEI core", index);
                }
            }
        }

        // Reconstruction successful
        reconstructed = header.append(reconstructed);

        return U_SUCCESS;
    }

    // All other actions are not supported
    return U_NOT_IMPLEMENTED;
}



