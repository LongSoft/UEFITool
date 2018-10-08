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
        extracted += model->header(index);
        extracted += model->body(index);
        extracted += model->tail(index);
    }
    else if (mode == EXTRACT_MODE_BODY) {
        name += UString("_body");
        // Extract without header and tail
        extracted.clear();
        extracted += model->body(index);
    }
    else if (mode == EXTRACT_MODE_BODY_UNCOMPRESSED) {
        name += UString("_body_unc");
        // Extract without header and tail, uncompressed
        extracted.clear();
        // There is no need to redo decompression, we can use child items
        for (int i = 0; i < model->rowCount(index); i++) {
             UModelIndex childIndex = index.child(i, 0);
             // Ensure 4-byte alignment of current section
             extracted += UByteArray(ALIGN4((UINT32)extracted.size()) - (UINT32)extracted.size(), '\x00');
             // Add current section header, body and tail
             extracted += model->header(childIndex);
             extracted += model->body(childIndex);
             extracted += model->tail(childIndex);
        }
    }
    else
        return U_UNKNOWN_EXTRACT_MODE;

    return U_SUCCESS;
}

USTATUS FfsOperations::replace(const UModelIndex & index, UByteArray & data, const UINT8 mode)
{
    U_UNUSED_PARAMETER(data);

    // Sanity check
    if (!index.isValid())
        return U_INVALID_PARAMETER;

    if (mode == REPLACE_MODE_AS_IS) {
        return U_NOT_IMPLEMENTED;
    }
    else if (mode == REPLACE_MODE_BODY) {
        return U_NOT_IMPLEMENTED;
    }
    
     return U_UNKNOWN_REPLACE_MODE;
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
