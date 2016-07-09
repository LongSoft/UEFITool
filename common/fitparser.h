/* fitparser.h

Copyright (c) 2015, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHWARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*/

#ifndef FITPARSER_H
#define FITPARSER_H

#include <vector>

#include "ustring.h"
#include "ubytearray.h"
#include "treemodel.h"
#include "utility.h"
#include "parsingdata.h"
#include "fit.h"
#include "types.h"
#include "treemodel.h"

class TreeModel;

class FitParser
{
public:
    // Default constructor and destructor
    FitParser(TreeModel* treeModel) : model(treeModel) {}
    ~FitParser() {}

    // Returns messages
    std::vector<std::pair<UString, UModelIndex> > getMessages() const { return messagesVector; };
    // Clears messages
    void clearMessages() { messagesVector.clear(); }

    USTATUS parse(const UModelIndex & index, const UModelIndex & lastVtf);
    std::vector<std::vector<UString> > getFitTable() const { return fitTable; }

private:
    TreeModel *model;
    std::vector<std::pair<UString, UModelIndex> > messagesVector;
    UModelIndex lastVtf;
    std::vector<std::vector<UString> > fitTable;
    
    USTATUS findFitRecursive(const UModelIndex & index, UModelIndex & found, UINT32 & fitOffset);
    UString fitEntryTypeToUString(UINT8 type);

    // Message helper
    void msg(const UString & message, const UModelIndex &index = UModelIndex()) {
        messagesVector.push_back(std::pair<UString, UModelIndex>(message, index));
    }
};

#endif // FITPARSER_H
