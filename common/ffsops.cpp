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

USTATUS FfsOperations::extract(const UModelIndex & index, UString & name, UByteArray & extracted, const UINT8 mode)
{
    // Sanity check
    if (!index.isValid())
        return U_INVALID_PARAMETER;

    // Get data from parsing data
    PARSING_DATA pdata = parsingDataFromUModelIndex(index);

    // Construct a name for extracted data
    UString itemName = model->name(index);
    UString itemText = model->text(index);

    // Default name
    name = itemName.replace(' ', '_').replace('/', '_').replace('-', '_');

    switch (model->type(index)) {
    case Types::Volume:        if (pdata.volume.hasExtendedHeader) name = guidToUString(pdata.volume.extendedHeaderGuid).replace('-', '_'); break;
    case Types::NvarEntry:
    case Types::VssEntry:
    case Types::FsysEntry:
    case Types::EvsaEntry:
    case Types::FlashMapEntry:
    case Types::File:          name = itemText.isEmpty() ? itemName : itemText.replace(' ', '_').replace('-', '_'); break;
    case Types::Section: {
        // Get parent file name
        UModelIndex fileIndex = model->findParentOfType(index, Types::File);
        UString fileText = model->text(fileIndex);
        name = fileText.isEmpty() ? model->name(fileIndex) : fileText.replace(' ', '_').replace('-', '_');
        // Append section subtype name
        name += '_' + itemName.replace(' ', '_');
        } break;
    }

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

USTATUS FfsOperations::replace(const UModelIndex & index, const UString & data, const UINT8 mode)
{
    // Sanity check
    if (!index.isValid())
        return U_INVALID_PARAMETER;

    // Get data from parsing data
    //PARSING_DATA pdata = parsingDataFromQModelIndex(index);

    if (mode == REPLACE_MODE_AS_IS) {
        return U_NOT_IMPLEMENTED;
    }
    else if (mode == REPLACE_MODE_BODY) {
        return U_NOT_IMPLEMENTED;
    }
    else 
        return U_UNKNOWN_REPLACE_MODE;
    
    return U_NOT_IMPLEMENTED;
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
