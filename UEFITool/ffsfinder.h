/* fssfinder.h

Copyright (c) 2015, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#ifndef FFSFINDER_H
#define FFSFINDER_H

#include <vector>
#include <QRegExp>

#include "../common/ubytearray.h"
#include "../common/ustring.h"
#include "../common/basetypes.h"
#include "../common/treemodel.h"

class FfsFinder
{
public:
    FfsFinder(const TreeModel * treeModel) : model(treeModel) {}
    ~FfsFinder() {}

    std::vector<std::pair<UString, UModelIndex> > getMessages() const { return messagesVector; }
    void clearMessages() { messagesVector.clear(); }

    USTATUS findHexPattern(const UModelIndex & index, const UByteArray & hexPattern, const UINT8 mode);
    USTATUS findGuidPattern(const UModelIndex & index, const UByteArray & guidPattern, const UINT8 mode);
    USTATUS findTextPattern(const UModelIndex & index, const UString & pattern, const UINT8 mode, const bool unicode, const Qt::CaseSensitivity caseSensitive);

private:
    const TreeModel* model;
    std::vector<std::pair<UString, UModelIndex> > messagesVector;

    void msg(const UString & message, const UModelIndex &index = UModelIndex()) {
        messagesVector.push_back(std::pair<UString, UModelIndex>(message, index));
    }
};

#endif // FFSFINDER_H
