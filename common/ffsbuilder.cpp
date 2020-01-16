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

    erased = UByteArray(model->header(index).size() + model->body(index).size() + model->tail(index).size(), emptyByte);

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
                    capsule += imageData;
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
                intelImage += model->header(currentRegion) + model->body(currentRegion) + model->tail(currentRegion);
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

USTATUS FfsBuilder::buildRawArea(const UModelIndex & index, UByteArray & rawArea)
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
                rawArea += currentData;
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
    U_UNUSED_PARAMETER(index);
    U_UNUSED_PARAMETER(volume);

    return U_NOT_IMPLEMENTED;
}

USTATUS FfsBuilder::buildPadFile(const UModelIndex & index, UByteArray & padFile)
{
    U_UNUSED_PARAMETER(index);
    U_UNUSED_PARAMETER(padFile);

    return U_NOT_IMPLEMENTED;
}

USTATUS FfsBuilder::buildFile(const UModelIndex & index, UByteArray & file)
{
    U_UNUSED_PARAMETER(index);
    U_UNUSED_PARAMETER(file);

    return U_NOT_IMPLEMENTED;
}

USTATUS FfsBuilder::buildSection(const UModelIndex & index, UByteArray & section)
{
    U_UNUSED_PARAMETER(index);
    U_UNUSED_PARAMETER(section);

    return U_NOT_IMPLEMENTED;
}


