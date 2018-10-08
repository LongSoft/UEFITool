/* fssops.h

Copyright (c) 2015, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#ifndef FFSOPS_H
#define FFSOPS_H

#include <vector>

#include "basetypes.h"
#include "ubytearray.h"
#include "ustring.h"
#include "treemodel.h"
#include "ffsparser.h"

class FfsOperations
{
public:

    FfsOperations(TreeModel * treeModel) : model(treeModel) {}
    ~FfsOperations() {};

    std::vector<std::pair<UString, UModelIndex> > getMessages() const { return messagesVector; }
    void clearMessages() { messagesVector.clear(); }

    USTATUS extract(const UModelIndex & index, UString & name, UByteArray & extracted, const UINT8 mode);
    USTATUS replace(const UModelIndex & index, UByteArray & data, const UINT8 mode);
    
    USTATUS remove(const UModelIndex & index);
    USTATUS rebuild(const UModelIndex & index);

private:
    TreeModel * model;

    std::vector<std::pair<UString, UModelIndex> > messagesVector;

    void msg(const UString & message, const UModelIndex &index = UModelIndex()) {
        messagesVector.push_back(std::pair<UString, UModelIndex>(message, index));
    }
};

#endif // FFSOPS_H
