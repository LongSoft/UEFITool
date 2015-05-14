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

STATUS FfsBuilder::build(const QModelIndex & root, QByteArray & image)
{
    return ERR_NOT_IMPLEMENTED;
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

            // Reconstruct children
            for (int i = 0; i < model->rowCount(index); i++) {
                QModelIndex currentChild = index.child(i, 0);
                QByteArray currentData;
                // Check child type
                if (model->type(currentChild) == Types::Image) {
                    result = buildImage(currentChild, currentData);
                    if (!result) { 
                        capsule.append(currentData);
                    }
                    else {
                        msg(tr("buildCapsule: building of \"%1\" failed with error \"%2\", original item data used").arg(model->name(currentChild)).arg(errorCodeToQString(result)), currentChild);
                        capsule.append(model->header(currentChild)).append(model->body(currentChild));
                    }
                }
                else {
                    msg(tr("buildCapsule: unexpected child item of type \"%1\" can't be processed, original item data used").arg(itemTypeToQString(model->type(currentChild))), currentChild);
                    capsule.append(model->header(currentChild)).append(model->body(currentChild));
                }
            }

            // Check size of reconstructed capsule, it must remain the same
            if (capsule.size() > model->body(index).size()) {
                msg(tr("buildCapsule: new capsule size %1h (%2) is bigger than the original %3h (%4)")
                    .hexarg(capsule.size()).arg(capsule.size())
                    .hexarg(model->body(index).size()).arg(model->body(index).size()),
                    index);
                return ERR_INVALID_PARAMETER;
            }
            else if (capsule.size() < model->body(index).size()) {
                msg(tr("buildCapsule: new capsule size %1h (%2) is smaller than the original %3h (%4)")
                    .hexarg(capsule.size()).arg(capsule.size())
                    .hexarg(model->body(index).size()).arg(model->body(index).size()),
                    index);
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

STATUS FfsBuilder::buildImage(const QModelIndex & index, QByteArray & intelImage)
{
    return ERR_NOT_IMPLEMENTED;
}

STATUS FfsBuilder::buildRawArea(const QModelIndex & index, QByteArray & rawArea)
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

            // Reconstruct children
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

            // Check size of reconstructed raw area, it must remain the same
            if (rawArea.size() > model->body(index).size()) {
                msg(tr("buildRawArea: new raw area size %1h (%2) is bigger than the original %3h (%4)")
                    .hexarg(rawArea.size()).arg(rawArea.size())
                    .hexarg(model->body(index).size()).arg(model->body(index).size()),
                    index);
                return ERR_INVALID_PARAMETER;
            }
            else if (rawArea.size() < model->body(index).size()) {
                msg(tr("buildRawArea: new raw area size %1h (%2) is smaller than the original %3h (%4)")
                    .hexarg(rawArea.size()).arg(rawArea.size())
                    .hexarg(model->body(index).size()).arg(model->body(index).size()),
                    index);
                return ERR_INVALID_PARAMETER;
            }
        }
        else
            rawArea = model->body(index);

        // Build successful, append header
        rawArea = model->header(index).append(rawArea);
        return ERR_SUCCESS;
    }

    msg(tr("buildRawArea: unexpected action \"%1\"").arg(actionTypeToQString(model->action(index))), index);
    return ERR_NOT_IMPLEMENTED;
}

STATUS FfsBuilder::buildVolume(const QModelIndex & index, QByteArray & volume)
{
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
        padding.fill('\xFF', model->header(index).size() + model->body(index).size());
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
        data.fill('\xFF', model->header(index).size() + model->body(index).size());
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

