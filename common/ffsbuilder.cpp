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

FfsBuilder::FfsBuilder(const TreeModel* treeModel)
    : model(treeModel)
{
}

FfsBuilder::~FfsBuilder()
{
}

void FfsBuilder::msg(const QString & message, const QModelIndex & index)
{
    messagesVector.push_back(std::pair<QString, QModelIndex>(message, index));
}

std::vector<std::pair<QString, QModelIndex> > FfsBuilder::getMessages() const
{
    return messagesVector;
}

void FfsBuilder::clearMessages()
{
    messagesVector.clear();
}

STATUS FfsBuilder::erase(const QModelIndex & index, QByteArray & erased)
{
    // Sanity check
    if (!index.isValid())
        return ERR_INVALID_PARAMETER;

    PARSING_DATA pdata = parsingDataFromQModelIndex(index);
    erased.fill(pdata.emptyByte);
    return ERR_SUCCESS;
}

STATUS FfsBuilder::build(const QModelIndex & root, QByteArray & image)
{
    // Sanity check
    if (!root.isValid())
        return ERR_INVALID_PARAMETER;

    if (model->type(root) == Types::Capsule) {
        return buildCapsule(root, image);
    }
    else if (model->type(root) == Types::Image) {
        if (model->subtype(root) == Subtypes::IntelImage) {
            return buildIntelImage(root, image);
        }
        else if (model->subtype(root) == Subtypes::IntelImage) {
            return buildRawArea(root, image);
        }
    }

    return ERR_NOT_IMPLEMENTED;
}

STATUS FfsBuilder::buildCapsule(const QModelIndex & index, QByteArray & capsule)
{
    // Sanity check
    if (!index.isValid())
        return ERR_INVALID_PARAMETER;

    // No action required
    if (model->action(index) == Actions::NoAction) {
        // Use original item data
        capsule = model->header(index).append(model->body(index));
        return ERR_SUCCESS;
    }

    // Rebuild or Replace
    else if (model->action(index) == Actions::Rebuild 
        || model->action(index) == Actions::Replace) {
        if (model->rowCount(index)) {
            // Clear the supplied QByteArray
            capsule.clear();

            // Right now there is only one capsule image element supported
            if (model->rowCount(index) != 1) {
                msg(QObject::tr("buildCapsule: building of capsules with %1 elements are not supported, original item data is used").arg(model->rowCount(index)), index);
                // Use original item data
                capsule = model->header(index).append(model->body(index));
                return ERR_SUCCESS;
            }
            
            // Build image
            QModelIndex imageIndex = index.child(0, 0);
            QByteArray imageData;
            
            // Check image type
            if (model->type(imageIndex) == Types::Image) {
                STATUS result;
                if (model->subtype(imageIndex) == Subtypes::IntelImage) {
                    result = buildIntelImage(imageIndex, imageData);
                }
                else if (model->subtype(imageIndex) == Subtypes::UefiImage) {
                    result = buildRawArea(imageIndex, imageData);
                }
                else {
                    msg(QObject::tr("buildCapsule: unexpected item of subtype %1 can't be processed, original item data is used").arg(model->subtype(imageIndex)), imageIndex);
                    capsule.append(model->header(imageIndex)).append(model->body(imageIndex));
                }
                
                // Check build result
                if (result) {
                    msg(QObject::tr("buildCapsule: building of \"%1\" failed with error \"%2\", original item data is used").arg(model->name(imageIndex)).arg(errorCodeToQString(result)), imageIndex);
                    capsule.append(model->header(imageIndex)).append(model->body(imageIndex));
                }
                else
                    capsule.append(imageData);
            }
            else {
                msg(QObject::tr("buildCapsule: unexpected item of type %1 can't be processed, original item data is used").arg(model->type(imageIndex)), imageIndex);
                capsule.append(model->header(imageIndex)).append(model->body(imageIndex));
            }
            
            // Check size of reconstructed capsule, it must remain the same
            UINT32 newSize = capsule.size();
            UINT32 oldSize = model->body(index).size();
            if (newSize > oldSize) {
                msg(QObject::tr("buildCapsule: new capsule body size %1h (%2) is bigger than the original %3h (%4)")
                    .hexarg(newSize).arg(newSize).hexarg(oldSize).arg(oldSize),index);
                return ERR_INVALID_PARAMETER;
            }
            else if (newSize < oldSize) {
                msg(QObject::tr("buildCapsule: new capsule body size %1h (%2) is smaller than the original %3h (%4)")
                    .hexarg(newSize).arg(newSize).hexarg(oldSize).arg(oldSize), index);
                return ERR_INVALID_PARAMETER;
            }
        }
        else
            capsule = model->body(index);

        // Build successful, append header
        capsule = model->header(index).append(capsule);
        return ERR_SUCCESS;
    }

    msg(QObject::tr("buildCapsule: unexpected action \"%1\"").arg(actionTypeToQString(model->action(index))), index);
    return ERR_NOT_IMPLEMENTED;
}

STATUS FfsBuilder::buildIntelImage(const QModelIndex & index, QByteArray & intelImage)
{
    // Sanity check
    if (!index.isValid())
        return ERR_SUCCESS;
    
    // No action
    if (model->action(index) == Actions::NoAction) {
        intelImage = model->header(index).append(model->body(index));
        return ERR_SUCCESS;
    }

    // Rebuild
    else if (model->action(index) == Actions::Rebuild) {
        intelImage.clear();

        // First child will always be descriptor for this type of image, and it's read only
        intelImage.append(model->header(index.child(0, 0)).append(model->body(index.child(0, 0))));
        
        // Process other regions
        for (int i = 1; i < model->rowCount(index); i++) {
            QModelIndex currentRegion = index.child(i, 0);

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
            STATUS result;
            QByteArray region;
            UINT8 regionType = model->subtype(currentRegion);
            switch (regionType) {
            case Subtypes::BiosRegion:
            case Subtypes::PdrRegion:
                result = buildRawArea(currentRegion, region);
                if (result) {
                    msg(QObject::tr("buildIntelImage: building of %1 region failed with error \"%2\", original item data is used").arg(regionTypeToQString(regionType)).arg(errorCodeToQString(result)), currentRegion);
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
                msg(QObject::tr("buildIntelImage: don't know how to build region of unknown type"), index);
                return ERR_UNKNOWN_ITEM_TYPE;
            }

            // Append the resulting region
            intelImage.append(region);
        }
        
        // Check size of new image, it must be same as old one
        UINT32 newSize = intelImage.size();
        UINT32 oldSize = model->body(index).size();
        if (newSize > oldSize) {
            msg(QObject::tr("buildIntelImage: new image size %1h (%2) is bigger than the original %3h (%4)")
                .hexarg(newSize).arg(newSize).hexarg(oldSize).arg(oldSize), index);
            return ERR_INVALID_PARAMETER;
        }
        else if (newSize < oldSize) {
            msg(QObject::tr("buildIntelImage: new image size %1h (%2) is smaller than the original %3h (%4)")
                .hexarg(newSize).arg(newSize).hexarg(oldSize).arg(oldSize), index);
            return ERR_INVALID_PARAMETER;
        }

        // Reconstruction successful
        return ERR_SUCCESS;
    }

    msg(QObject::tr("buildIntelImage: unexpected action \"%1\"").arg(actionTypeToQString(model->action(index))), index);
    return ERR_NOT_IMPLEMENTED;
}

STATUS FfsBuilder::buildRawArea(const QModelIndex & index, QByteArray & rawArea, bool addHeader)
{
    // Sanity check
    if (!index.isValid())
        return ERR_INVALID_PARAMETER;

    // No action required
    if (model->action(index) == Actions::NoAction) {
        rawArea = model->header(index).append(model->body(index));
        return ERR_SUCCESS;
    }

    // Rebuild or Replace
    else if (model->action(index) == Actions::Rebuild 
        || model->action(index) == Actions::Replace) {
        if (model->rowCount(index)) {
            // Clear the supplied QByteArray
            rawArea.clear();

            // Build children
            for (int i = 0; i < model->rowCount(index); i++) {
                STATUS result = ERR_SUCCESS;
                QModelIndex currentChild = index.child(i, 0);
                QByteArray currentData;
                // Check child type
                if (model->type(currentChild) == Types::Volume) {
                    result = buildVolume(currentChild, currentData);
                }
                else if (model->type(currentChild) == Types::Padding) {
                    result = buildPadding(currentChild, currentData);
                }
                else {
                    msg(QObject::tr("buildRawArea: unexpected item of type %1 can't be processed, original item data is used").arg(model->type(currentChild)), currentChild);
                    currentData = model->header(currentChild).append(model->body(currentChild));
                }
                // Check build result
                if (result) {
                    msg(QObject::tr("buildRawArea: building of %1 failed with error \"%2\", original item data is used").arg(model->name(currentChild)).arg(errorCodeToQString(result)), currentChild);
                    currentData = model->header(currentChild).append(model->body(currentChild));
                }
                // Append current data
                rawArea.append(currentData);
            }

            // Check size of new raw area, it must be same as original one
            UINT32 newSize = rawArea.size();
            UINT32 oldSize = model->body(index).size();
            if (newSize > oldSize) {
                msg(QObject::tr("buildRawArea: new area size %1h (%2) is bigger than the original %3h (%4)")
                    .hexarg(newSize).arg(newSize).hexarg(oldSize).arg(oldSize), index);
                return ERR_INVALID_PARAMETER;
            }
            else if (newSize < oldSize) {
                msg(QObject::tr("buildRawArea: new area size %1h (%2) is smaller than the original %3h (%4)")
                    .hexarg(newSize).arg(newSize).hexarg(oldSize).arg(oldSize), index);
                return ERR_INVALID_PARAMETER;
            }
        }
        else
            rawArea = model->body(index);

        // Build successful, add header if needed
        if (addHeader)
            rawArea = model->header(index).append(rawArea);
        return ERR_SUCCESS;
    }

    msg(QObject::tr("buildRawArea: unexpected action \"%1\"").arg(actionTypeToQString(model->action(index))), index);
    return ERR_NOT_IMPLEMENTED;
}

STATUS FfsBuilder::buildPadding(const QModelIndex & index, QByteArray & padding)
{
    // Sanity check
    if (!index.isValid())
        return ERR_INVALID_PARAMETER;

    // No action required
    if (model->action(index) == Actions::NoAction) {
        padding = model->header(index).append(model->body(index));
        return ERR_SUCCESS;
    }

    // Erase
    else if (model->action(index) == Actions::Erase) {
        padding = model->header(index).append(model->body(index));
        if(erase(index, padding))
            msg(QObject::tr("buildPadding: erase failed, original item data is used"), index);
        return ERR_SUCCESS;
    }

    msg(QObject::tr("buildPadding: unexpected action \"%1\"").arg(actionTypeToQString(model->action(index))), index);
    return ERR_NOT_IMPLEMENTED;
}

STATUS FfsBuilder::buildNonUefiData(const QModelIndex & index, QByteArray & data)
{
    // Sanity check
    if (!index.isValid())
        return ERR_INVALID_PARAMETER;

    // No action required
    if (model->action(index) == Actions::NoAction) {
        data = model->header(index).append(model->body(index));
        return ERR_SUCCESS;
    }

    // Erase
    else if (model->action(index) == Actions::Erase) {
        data = model->header(index).append(model->body(index));
        if (erase(index, data))
            msg(QObject::tr("buildNonUefiData: erase failed, original item data is used"), index);
        return ERR_SUCCESS;
    }

    msg(QObject::tr("buildNonUefiData: unexpected action \"%1\"").arg(actionTypeToQString(model->action(index))), index);
    return ERR_NOT_IMPLEMENTED;
}

STATUS FfsBuilder::buildFreeSpace(const QModelIndex & index, QByteArray & freeSpace)
{
    // Sanity check
    if (!index.isValid())
        return ERR_INVALID_PARAMETER;

    // No action required
    if (model->action(index) == Actions::NoAction) {
        freeSpace = model->header(index).append(model->body(index));
        return ERR_SUCCESS;
    }

    msg(QObject::tr("buildFreeSpace: unexpected action \"%1\"").arg(actionTypeToQString(model->action(index))), index);
    return ERR_NOT_IMPLEMENTED;
}

STATUS FfsBuilder::buildVolume(const QModelIndex & index, QByteArray & volume)
{
    return ERR_NOT_IMPLEMENTED;
}

STATUS FfsBuilder::buildPadFile(const QModelIndex & index, QByteArray & padFile)
{
    return ERR_NOT_IMPLEMENTED;
}

STATUS FfsBuilder::buildFile(const QModelIndex & index, QByteArray & file)
{
    return ERR_NOT_IMPLEMENTED;
}

STATUS FfsBuilder::buildSection(const QModelIndex & index, QByteArray & section)
{
    return ERR_NOT_IMPLEMENTED;
}


