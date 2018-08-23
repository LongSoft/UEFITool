/* fssops.cpp

Copyright (c) 2015, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#include "ffsops.h"
#include "ffs.h"
#include "utility.h"
#include "nvramparser.h"

USTATUS FfsOperations::extract(const UModelIndex & index, UString & name, UByteArray & extracted, const UINT8 mode)
{
    // Sanity check
    if (!index.isValid())
        return U_INVALID_PARAMETER;

    // Default name
    name = uniqueItemName(index);

    // Get extracted data
    if (mode == EXTRACT_MODE_AS_IS) {
        // Extract as is, with header body and tail
        extracted.clear();
        extracted.append(model->header(index));
        extracted.append(model->body(index));
        extracted.append(model->tail(index));
    }
    else if (mode == EXTRACT_MODE_BODY) {
        name += QObject::tr("_body");
        // Extract without header and tail
        extracted.clear();
        extracted.append(model->body(index));
    }
    else if (mode == EXTRACT_MODE_BODY_UNCOMPRESSED) {
        name += QObject::tr("_body_unc");
        // Extract without header and tail, uncompressed
        extracted.clear();
        // There is no need to redo decompression, we can use child items
        for (int i = 0; i < model->rowCount(index); i++) {
             UModelIndex childIndex = index.child(i, 0);
             // Ensure 4-byte alignment of current section
             extracted.append(UByteArray('\x00', ALIGN4((UINT32)extracted.size()) - (UINT32)extracted.size()));
             // Add current section header, body and tail
             extracted.append(model->header(childIndex));
             extracted.append(model->body(childIndex));
             extracted.append(model->tail(childIndex));
        }
    }
    else
        return U_UNKNOWN_EXTRACT_MODE;

    return U_SUCCESS;
}

USTATUS FfsOperations::replace(const UModelIndex & index, UByteArray & data, const UINT8 mode)
{
    if (!index.isValid() || !index.parent().isValid())
        return U_INVALID_PARAMETER;

    USTATUS result;
    UByteArray empty = "";
    FfsParser parser(model);
    UINT32 localOffset = model->offset(index) + model->header(index).size();
    UModelIndex index_out;

    if(mode == REPLACE_MODE_BODY)
        data = model->header(index) + data;

    if (model->type(index) == Types::Region) {
        UINT8 type = model->subtype(index);
        switch (type) {
        case Subtypes::BiosRegion:
            result = parser.parseBiosRegion(data, localOffset, index, index_out, CREATE_MODE_AFTER);
            break;
        case Subtypes::MeRegion:
            result = parser.parseMeRegion(data, localOffset, index, index_out, CREATE_MODE_AFTER);
            break;
        case Subtypes::GbeRegion:
            result = parser.parseGbeRegion(data, localOffset, index, index_out, CREATE_MODE_AFTER);
            break;
        case Subtypes::PdrRegion:
            result = parser.parsePdrRegion(data, localOffset, index, index_out, CREATE_MODE_AFTER);
            break;
        default:
            return U_NOT_IMPLEMENTED;
        }

        if (result && result != U_VOLUMES_NOT_FOUND && result != U_INVALID_VOLUME)
            return result;
    }
    else if (model->type(index) == Types::Padding) {
        // Get info
        QString name = usprintf("Padding");
        QString info = usprintf("Full size: %Xh (%u)", data.size(), data.size());
        // Add tree item
        //!TODO UModelIndex fileIndex = model->addItem(Types::Padding, getPaddingType(body), COMPRESSION_ALGORITHM_NONE, name, "", info, UByteArray(), body, index, mode);
    }
    else if (model->type(index) == Types::Volume) {
        result = parser.parseVolumeHeader(data, localOffset, index, index_out, CREATE_MODE_AFTER);
        if (result)
            return result;

        result = parser.parseVolumeBody(index_out);
        if (result)
            return result;

    }
    else if (model->type(index) == Types::File) {
        result = parser.parseFileHeader(data, localOffset, index, index_out, CREATE_MODE_AFTER);
        if (result && result != U_VOLUMES_NOT_FOUND  && result != U_INVALID_VOLUME)
            return result;

        result = parser.parseFileBody(index_out);
        if (result && result != U_VOLUMES_NOT_FOUND  && result != U_INVALID_VOLUME)
            return result;
    }
    else if (model->type(index) == Types::Section) {
        result = parser.parseSectionHeader(data, localOffset, index, index_out, true, CREATE_MODE_AFTER);
        if (result && result != U_VOLUMES_NOT_FOUND && result != U_INVALID_VOLUME)
            return result;

        result = parser.parseSectionBody(index_out);
        if(result && result != U_VOLUMES_NOT_FOUND  && result != U_INVALID_VOLUME)
            return result;
    }
    else if(model->type(index) == Types::EvsaStore || model->type(index) == Types::CmdbStore ||
            model->type(index) == Types::FdcStore || model->type(index) == Types::FlashMapStore ||
            model->type(index) == Types::FsysStore || model->type(index) == Types::FtwStore ||
            model->type(index) == Types::Vss2Store || model->type(index) == Types::VssStore) {
        if(data.size() != model->header(index).size() + model->body(index).size())
            return U_INVALID_STORE_SIZE;

        NvramParser nvramParser(model, &parser);

        result = nvramParser.parseStoreHeader(data, localOffset, index, index_out, CREATE_MODE_AFTER);
        if (result && result != U_VOLUMES_NOT_FOUND && result != U_INVALID_VOLUME)
            return result;

        UINT8 type = model->type(index);
        switch (type) {
        case Types::FdcStore:      nvramParser.parseFdcStoreBody(index_out); break;
        case Types::VssStore:      nvramParser.parseVssStoreBody(index_out, 0); break;
        case Types::Vss2Store:     nvramParser.parseVssStoreBody(index_out, 4); break;
        case Types::FsysStore:     nvramParser.parseFsysStoreBody(index_out); break;
        case Types::EvsaStore:     nvramParser.parseEvsaStoreBody(index_out); break;
        case Types::FlashMapStore: nvramParser.parseFlashMapBody(index_out); break;
        }
    }
    else
        return U_NOT_IMPLEMENTED;

    // Set remove action to replaced item
    model->setAction(index, Actions::Remove);
    model->setAction(index_out, Actions::Replace);

    // Rebuild parent, if it has no action now
    UModelIndex parent = index.parent();
    if (parent.isValid() && model->type(parent) != Types::Root
        && model->action(parent) == Actions::NoAction)
       rebuild(parent);

    return U_SUCCESS;
}

USTATUS FfsOperations::remove(const UModelIndex & index)
{
    // Sanity check
    if (!index.isValid())
        return U_INVALID_PARAMETER;

    // Set remove action
    model->setAction(index, Actions::Remove);

    return U_SUCCESS;
}

USTATUS FfsOperations::rebuild(const UModelIndex & index)
{
    // Sanity check
    if (!index.isValid())
        return U_INVALID_PARAMETER;

    // On insert action, set insert action for children
    //if (action == Actions::Insert)
    //    for (int i = 0; i < item->childCount(); i++)
    //        setAction(index.child(i, 0), Actions::Insert);

    // Set rebuild action
    model->setAction(index, Actions::Rebuild);

    // Rebuild parent, if it has no action now
    UModelIndex parent = index.parent();
    if (parent.isValid() && model->type(parent) != Types::Root
        && model->action(parent) == Actions::NoAction)
       rebuild(parent);
    
    return U_SUCCESS;
}
