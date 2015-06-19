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

FfsBuilder::FfsBuilder(const TreeModel* treeModel, QObject *parent)
    : QObject(parent), model(treeModel)
{
}

FfsBuilder::~FfsBuilder()
{
}

void FfsBuilder::msg(const QString & message, const QModelIndex & index)
{
    messagesVector.push_back(QPair<QString, QModelIndex>(message, index));
}

QVector<QPair<QString, QModelIndex> > FfsBuilder::getMessages() const
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

    PARSING_DATA pdata = parsingDataFromQByteArray(index);
    erased.fill(pdata.emptyByte);
    return ERR_SUCCESS;
}

STATUS FfsBuilder::buildCapsule(const QModelIndex & index, QByteArray & capsule)
{
    // Sanity check
    if (!index.isValid())
        return ERR_INVALID_PARAMETER;

    STATUS result;

    // No action required
    if (model->action(index) == Actions::NoAction) {
        capsule = model->header(index).append(model->body(index));
        return ERR_SUCCESS;
    }

    // Rebuild or Replace
    else if (model->action(index) == Actions::Rebuild 
        || model->action(index) == Actions::Replace) {
        if (model->rowCount(index)) {
            // Clear the supplied QByteArray
            capsule.clear();

            // Build children
            for (int i = 0; i < model->rowCount(index); i++) {
                QModelIndex currentChild = index.child(i, 0);
                QByteArray currentData;
                // Check child type
                if (model->type(currentChild) == Types::Image) {
                    if (model->subtype(currentChild) == Subtypes::IntelImage)
                        result = buildIntelImage(currentChild, currentData);
                    else
                        result = buildRawArea(currentChild, currentData);

                    // Check build result
                    if (result) {
                        msg(tr("buildCapsule: building of \"%1\" failed with error \"%2\", original item data used").arg(model->name(currentChild)).arg(errorCodeToQString(result)), currentChild);
                        capsule.append(model->header(currentChild)).append(model->body(currentChild));
                    }
                    else
                        capsule.append(currentData);
                }
                else {
                    msg(tr("buildCapsule: unexpected child item of type \"%1\" can't be processed, original item data used").arg(itemTypeToQString(model->type(currentChild))), currentChild);
                    capsule.append(model->header(currentChild)).append(model->body(currentChild));
                }
            }

            // Check size of reconstructed capsule, it must remain the same
            UINT32 newSize = capsule.size();
            UINT32 oldSize = model->body(index).size();
            if (newSize > oldSize) {
                msg(tr("buildCapsule: new capsule size %1h (%2) is bigger than the original %3h (%4)")
                    .hexarg(newSize).arg(newSize).hexarg(oldSize).arg(oldSize),index);
                return ERR_INVALID_PARAMETER;
            }
            else if (newSize < oldSize) {
                msg(tr("buildCapsule: new capsule size %1h (%2) is smaller than the original %3h (%4)")
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

    msg(tr("buildCapsule: unexpected action \"%1\"").arg(actionTypeToQString(model->action(index))), index);
    return ERR_NOT_IMPLEMENTED;
}

STATUS FfsBuilder::buildIntelImage(const QModelIndex & index, QByteArray & intelImage)
{
    if (!index.isValid())
        return ERR_SUCCESS;

    UINT8 result;

    // No action
    if (model->action(index) == Actions::NoAction) {
        intelImage = model->header(index).append(model->body(index));
        return ERR_SUCCESS;
    }

    // Other supported actions
    else if (model->action(index) == Actions::Rebuild) {
        intelImage.clear();
        // First child will always be descriptor for this type of image
        QByteArray descriptor;
        result = buildRegion(index.child(0, 0), descriptor);
        if (result)
            return result;
        intelImage.append(descriptor);

        const FLASH_DESCRIPTOR_MAP* descriptorMap = (const FLASH_DESCRIPTOR_MAP*)(descriptor.constData() + sizeof(FLASH_DESCRIPTOR_HEADER));
        const FLASH_DESCRIPTOR_REGION_SECTION* regionSection = (const FLASH_DESCRIPTOR_REGION_SECTION*)calculateAddress8((const UINT8*)descriptor.constData(), descriptorMap->RegionBase);
        QByteArray gbe;
        UINT32 gbeBegin = calculateRegionOffset(regionSection->GbeBase);
        UINT32 gbeEnd = gbeBegin + calculateRegionSize(regionSection->GbeBase, regionSection->GbeLimit);
        QByteArray me;
        UINT32 meBegin = calculateRegionOffset(regionSection->MeBase);
        UINT32 meEnd = meBegin + calculateRegionSize(regionSection->MeBase, regionSection->MeLimit);
        QByteArray bios;
        UINT32 biosBegin = calculateRegionOffset(regionSection->BiosBase);
        UINT32 biosEnd = biosBegin + calculateRegionSize(regionSection->BiosBase, regionSection->BiosLimit);
        QByteArray pdr;
        UINT32 pdrBegin = calculateRegionOffset(regionSection->PdrBase);
        UINT32 pdrEnd = pdrBegin + calculateRegionSize(regionSection->PdrBase, regionSection->PdrLimit);

        UINT32 offset = descriptor.size();
        // Reconstruct other regions
        char empty = '\xFF';
        for (int i = 1; i < model->rowCount(index); i++) {
            QByteArray region;
            result = buildRegion(index.child(i, 0), region);
            if (result)
                return result;

            UINT8 type = model->subtype(index.child(i, 0));
            switch (type)
            {
            case Subtypes::GbeRegion:
                gbe = region;
                if (gbeBegin > offset)
                    intelImage.append(QByteArray(gbeBegin - offset, empty));
                intelImage.append(gbe);
                offset = gbeEnd;
                break;
            case Subtypes::MeRegion:
                me = region;
                if (meBegin > offset)
                    intelImage.append(QByteArray(meBegin - offset, empty));
                intelImage.append(me);
                offset = meEnd;
                break;
            case Subtypes::BiosRegion:
                bios = region;
                if (biosBegin > offset)
                    intelImage.append(QByteArray(biosBegin - offset, empty));
                intelImage.append(bios);
                offset = biosEnd;
                break;
            case Subtypes::PdrRegion:
                pdr = region;
                if (pdrBegin > offset)
                    intelImage.append(QByteArray(pdrBegin - offset, empty));
                intelImage.append(pdr);
                offset = pdrEnd;
                break;
            default:
                msg(tr("buildIntelImage: unknown region type found"), index);
                return ERR_INVALID_REGION;
            }
        }
        if ((UINT32)model->body(index).size() > offset)
            intelImage.append(QByteArray((UINT32)model->body(index).size() - offset, empty));

        // Check size of new image, it must be same as old one
        UINT32 newSize = intelImage.size();
        UINT32 oldSize = model->body(index).size();
        if (newSize > oldSize) {
            msg(tr("buildIntelImage: new image size %1h (%2) is bigger than the original %3h (%4)")
                .hexarg(newSize).arg(newSize).hexarg(oldSize).arg(oldSize), index);
            return ERR_INVALID_PARAMETER;
        }
        else if (newSize < oldSize) {
            msg(tr("buildIntelImage: new image size %1h (%2) is smaller than the original %3h (%4)")
                .hexarg(newSize).arg(newSize).hexarg(oldSize).arg(oldSize), index);
            return ERR_INVALID_PARAMETER;
        }

        // Reconstruction successful
        return ERR_SUCCESS;
    }

    msg(tr("buildIntelImage: unexpected action \"%1\"").arg(actionTypeToQString(model->action(index))), index);
    return ERR_NOT_IMPLEMENTED;
}

STATUS FfsBuilder::buildRegion(const QModelIndex & index, QByteArray & region)
{
    if (!index.isValid())
        return ERR_SUCCESS;

    UINT8 result;

    // No action required
    if (model->action(index) == Actions::NoAction) {
        region = model->header(index).append(model->body(index));
        return ERR_SUCCESS;
    }

    // Erase
    else if (model->action(index) == Actions::Erase) {
        region = model->header(index).append(model->body(index));
        if (erase(index, region))
            msg(tr("buildRegion: erase failed, original item data used"), index);
        return ERR_SUCCESS;
    }

    // Rebuild or replace
    else if (model->action(index) == Actions::Rebuild ||
        model->action(index) == Actions::Replace) {
        if (model->rowCount(index)) {
            region.clear();
            // Build children
            for (int i = 0; i < model->rowCount(index); i++) {
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
                    msg(tr("buildRegion: unexpected child item of type \"%1\" can't be processed, original item data used").arg(itemTypeToQString(model->type(currentChild))), currentChild);
                    result = ERR_SUCCESS;
                    currentData = model->header(currentChild).append(model->body(currentChild));
                }
                // Check build result
                if (result) {
                    msg(tr("buildRegion: building of \"%1\" failed with error \"%2\", original item data used").arg(model->name(currentChild)).arg(errorCodeToQString(result)), currentChild);
                    currentData = model->header(currentChild).append(model->body(currentChild));
                }
                // Append current data
                region.append(currentData);
            }
        }
        else
            region = model->body(index);

        // Check size of new region, it must be same as original one
        UINT32 newSize = region.size();
        UINT32 oldSize = model->body(index).size();
        if (newSize > oldSize) {
            msg(tr("buildRegion: new region size %1h (%2) is bigger than the original %3h (%4)")
                .hexarg(newSize).arg(newSize).hexarg(oldSize).arg(oldSize), index);
            return ERR_INVALID_PARAMETER;
        }
        else if (newSize < oldSize) {
            msg(tr("buildRegion: new region size %1h (%2) is smaller than the original %3h (%4)")
                .hexarg(newSize).arg(newSize).hexarg(oldSize).arg(oldSize), index);
            return ERR_INVALID_PARAMETER;
        }

        // Build successful
        region = model->header(index).append(region);
        return ERR_SUCCESS;
    }

    msg(tr("buildRegion: unexpected action \"%1\"").arg(actionTypeToQString(model->action(index))), index);
    return ERR_NOT_IMPLEMENTED;
}

STATUS FfsBuilder::buildRawArea(const QModelIndex & index, QByteArray & rawArea, bool addHeader)
{
    // Sanity check
    if (!index.isValid())
        return ERR_INVALID_PARAMETER;

    STATUS result;

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
                    msg(tr("buildRawArea: unexpected child item of type \"%1\" can't be processed, original item data used").arg(itemTypeToQString(model->type(currentChild))), currentChild);
                    result = ERR_SUCCESS;
                    currentData = model->header(currentChild).append(model->body(currentChild));
                }
                // Check build result
                if (result) {
                    msg(tr("buildRawArea: building of \"%1\" failed with error \"%2\", original item data used").arg(model->name(currentChild)).arg(errorCodeToQString(result)), currentChild);
                    currentData = model->header(currentChild).append(model->body(currentChild));
                }
                // Append current data
                rawArea.append(currentData);
            }

            // Check size of new raw area, it must be same as original one
            UINT32 newSize = rawArea.size();
            UINT32 oldSize = model->body(index).size();
            if (newSize > oldSize) {
                msg(tr("buildRawArea: new area size %1h (%2) is bigger than the original %3h (%4)")
                    .hexarg(newSize).arg(newSize).hexarg(oldSize).arg(oldSize), index);
                return ERR_INVALID_PARAMETER;
            }
            else if (newSize < oldSize) {
                msg(tr("buildRawArea: new area size %1h (%2) is smaller than the original %3h (%4)")
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

    msg(tr("buildRawArea: unexpected action \"%1\"").arg(actionTypeToQString(model->action(index))), index);
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
            msg(tr("buildPadding: erase failed, original item data used"), index);
        return ERR_SUCCESS;
    }

    msg(tr("buildPadding: unexpected action \"%1\"").arg(actionTypeToQString(model->action(index))), index);
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
            msg(tr("buildNonUefiData: erase failed, original item data used"), index);
        return ERR_SUCCESS;
    }

    msg(tr("buildNonUefiData: unexpected action \"%1\"").arg(actionTypeToQString(model->action(index))), index);
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

    msg(tr("buildFreeSpace: unexpected action \"%1\"").arg(actionTypeToQString(model->action(index))), index);
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

STATUS FfsBuilder::build(const QModelIndex & root, QByteArray & image)
{
    return ERR_NOT_IMPLEMENTED;
}
