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

USTATUS FfsBuilder::erase(const UModelIndex & index, UByteArray & erased)
{
    // Sanity check
    if (!index.isValid())
        return U_INVALID_PARAMETER;

    return U_NOT_IMPLEMENTED;
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

    // No action required
    if (model->action(index) == Actions::NoAction) {
        // Use original item data
        capsule = model->header(index).append(model->body(index));
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
                //msg(UString("buildCapsule: building of capsules with %1 elements are not supported, original item data is used").arg(model->rowCount(index)), index);
                // Use original item data
                capsule = model->header(index).append(model->body(index));
                return U_SUCCESS;
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
                    //msg(UString("buildCapsule: unexpected item of subtype %1 can't be processed, original item data is used").arg(model->subtype(imageIndex)), imageIndex);
                    capsule.append(model->header(imageIndex)).append(model->body(imageIndex));
                }
                
                // Check build result
                if (result) {
                    //msg(UString("buildCapsule: building of \"%1\" failed with error \"%2\", original item data is used").arg(model->name(imageIndex)).arg(errorCodeToUString(result)), imageIndex);
                    capsule.append(model->header(imageIndex)).append(model->body(imageIndex));
                }
                else
                    capsule.append(imageData);
            }
            else {
                //msg(UString("buildCapsule: unexpected item of type %1 can't be processed, original item data is used").arg(model->type(imageIndex)), imageIndex);
                capsule.append(model->header(imageIndex)).append(model->body(imageIndex));
            }
            
            // Check size of reconstructed capsule, it must remain the same
            UINT32 newSize = capsule.size();
            UINT32 oldSize = model->body(index).size();
            if (newSize > oldSize) {
                //msg(UString("buildCapsule: new capsule body size %1h (%2) is bigger than the original %3h (%4)")
                //    .hexarg(newSize).arg(newSize).hexarg(oldSize).arg(oldSize),index);
                return U_INVALID_PARAMETER;
            }
            else if (newSize < oldSize) {
                //msg(UString("buildCapsule: new capsule body size %1h (%2) is smaller than the original %3h (%4)")
                //    .hexarg(newSize).arg(newSize).hexarg(oldSize).arg(oldSize), index);
                return U_INVALID_PARAMETER;
            }
        }
        else
            capsule = model->body(index);

        // Build successful, append header
        capsule = model->header(index).append(capsule);
        return U_SUCCESS;
    }

    //msg(UString("buildCapsule: unexpected action \"%1\"").arg(actionTypeToQString(model->action(index))), index);
    return U_NOT_IMPLEMENTED;
}

USTATUS FfsBuilder::buildIntelImage(const UModelIndex & index, UByteArray & intelImage)
{
    // Sanity check
    if (!index.isValid())
        return U_SUCCESS;
    
    // No action
    if (model->action(index) == Actions::NoAction) {
        intelImage = model->header(index).append(model->body(index));
        return U_SUCCESS;
    }

    // Rebuild
    else if (model->action(index) == Actions::Rebuild) {
        intelImage.clear();

        // First child will always be descriptor for this type of image, and it's read only
        intelImage.append(model->header(index.child(0, 0)).append(model->body(index.child(0, 0))));
        
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
                intelImage.append(model->header(currentRegion).append(model->body(currentRegion)));
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
                    //msg(UString("buildIntelImage: building of %1 region failed with error \"%2\", original item data is used").arg(regionTypeToQString(regionType)).arg(errorCodeToQString(result)), currentRegion);
                    region = model->header(currentRegion).append(model->body(currentRegion));
                }
                break;
            case Subtypes::GbeRegion:
            case Subtypes::MeRegion:
            case Subtypes::EcRegion:
            case Subtypes::Reserved1Region:
            case Subtypes::Reserved2Region:
            case Subtypes::Reserved3Region:
            case Subtypes::Reserved4Region:
                // Add region as is
                region = model->header(currentRegion).append(model->body(currentRegion));
                break;
            default:
                msg(UString("buildIntelImage: don't know how to build region of unknown type"), index);
                return U_UNKNOWN_ITEM_TYPE;
            }

            // Append the resulting region
            intelImage.append(region);
        }
        
        // Check size of new image, it must be same as old one
        UINT32 newSize = intelImage.size();
        UINT32 oldSize = model->body(index).size();
        if (newSize > oldSize) {
            //msg(UString("buildIntelImage: new image size %1h (%2) is bigger than the original %3h (%4)")
            //    .hexarg(newSize).arg(newSize).hexarg(oldSize).arg(oldSize), index);
            return U_INVALID_PARAMETER;
        }
        else if (newSize < oldSize) {
            //msg(UString("buildIntelImage: new image size %1h (%2) is smaller than the original %3h (%4)")
            //    .hexarg(newSize).arg(newSize).hexarg(oldSize).arg(oldSize), index);
            return U_INVALID_PARAMETER;
        }

        // Reconstruction successful
        return U_SUCCESS;
    }

    //msg(UString("buildIntelImage: unexpected action \"%1\"").arg(actionTypeToQString(model->action(index))), index);
    return U_NOT_IMPLEMENTED;
}

USTATUS FfsBuilder::buildRawArea(const UModelIndex & index, UByteArray & rawArea, bool addHeader)
{
    // Sanity check
    if (!index.isValid())
        return U_INVALID_PARAMETER;

    // No action required
    if (model->action(index) == Actions::NoAction) {
        rawArea = model->header(index).append(model->body(index));
        return U_SUCCESS;
    }

    // Rebuild or Replace
    else if (model->action(index) == Actions::Rebuild 
        || model->action(index) == Actions::Replace) {
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
                    //msg(UString("buildRawArea: unexpected item of type %1 can't be processed, original item data is used").arg(model->type(currentChild)), currentChild);
                    currentData = model->header(currentChild).append(model->body(currentChild));
                }
                // Check build result
                if (result) {
                    //msg(UString("buildRawArea: building of %1 failed with error \"%2\", original item data is used").arg(model->name(currentChild)).arg(errorCodeToQString(result)), currentChild);
                    currentData = model->header(currentChild).append(model->body(currentChild));
                }
                // Append current data
                rawArea.append(currentData);
            }

            // Check size of new raw area, it must be same as original one
            UINT32 newSize = rawArea.size();
            UINT32 oldSize = model->body(index).size();
            if (newSize > oldSize) {
                //msg(UString("buildRawArea: new area size %1h (%2) is bigger than the original %3h (%4)")
                //    .hexarg(newSize).arg(newSize).hexarg(oldSize).arg(oldSize), index);
                return U_INVALID_PARAMETER;
            }
            else if (newSize < oldSize) {
                //msg(UString("buildRawArea: new area size %1h (%2) is smaller than the original %3h (%4)")
                //    .hexarg(newSize).arg(newSize).hexarg(oldSize).arg(oldSize), index);
                return U_INVALID_PARAMETER;
            }
        }
        else
            rawArea = model->body(index);

        // Build successful, add header if needed
        if (addHeader)
            rawArea = model->header(index).append(rawArea);
        return U_SUCCESS;
    }

    //msg(UString("buildRawArea: unexpected action \"%1\"").arg(actionTypeToQString(model->action(index))), index);
    return U_NOT_IMPLEMENTED;
}

USTATUS FfsBuilder::buildPadding(const UModelIndex & index, UByteArray & padding)
{
    // Sanity check
    if (!index.isValid())
        return U_INVALID_PARAMETER;

    // No action required
    if (model->action(index) == Actions::NoAction) {
        padding = model->header(index).append(model->body(index));
        return U_SUCCESS;
    }

    // Erase
    else if (model->action(index) == Actions::Erase) {
        padding = model->header(index).append(model->body(index));
        if(erase(index, padding))
            msg(UString("buildPadding: erase failed, original item data is used"), index);
        return U_SUCCESS;
    }

    //msg(UString("buildPadding: unexpected action \"%1\"").arg(actionTypeToUString(model->action(index))), index);
    return U_NOT_IMPLEMENTED;
}

USTATUS FfsBuilder::buildNonUefiData(const UModelIndex & index, UByteArray & data)
{
    // Sanity check
    if (!index.isValid())
        return U_INVALID_PARAMETER;

    // No action required
    if (model->action(index) == Actions::NoAction) {
        data = model->header(index).append(model->body(index));
        return U_SUCCESS;
    }

    // Erase
    else if (model->action(index) == Actions::Erase) {
        data = model->header(index).append(model->body(index));
        if (erase(index, data))
            msg(UString("buildNonUefiData: erase failed, original item data is used"), index);
        return U_SUCCESS;
    }

    //msg(UString("buildNonUefiData: unexpected action \"%1\"").arg(actionTypeToUString(model->action(index))), index);
    return U_NOT_IMPLEMENTED;
}

USTATUS FfsBuilder::buildFreeSpace(const UModelIndex & index, UByteArray & freeSpace)
{
    // Sanity check
    if (!index.isValid())
        return U_INVALID_PARAMETER;

    // No action required
    if (model->action(index) == Actions::NoAction) {
        freeSpace = model->header(index).append(model->body(index));
        return U_SUCCESS;
    }

    //msg(UString("buildFreeSpace: unexpected action \"%1\"").arg(actionTypeToQString(model->action(index))), index);
    return U_NOT_IMPLEMENTED;
}

USTATUS FfsBuilder::buildVolume(const UModelIndex & index, UByteArray & volume)
{
    return U_NOT_IMPLEMENTED;
}

USTATUS FfsBuilder::buildPadFile(const UModelIndex & index, UByteArray & padFile)
{
    return U_NOT_IMPLEMENTED;
}

USTATUS FfsBuilder::buildFile(const UModelIndex & index, UByteArray & file)
{
    return U_NOT_IMPLEMENTED;
}

USTATUS FfsBuilder::buildSection(const UModelIndex & index, UByteArray & section)
{
    return U_NOT_IMPLEMENTED;
}


