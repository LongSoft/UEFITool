/* meparser.h

Copyright (c) 2016, Nikolaj Schlej. All rights reserved.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#ifndef MEPARSER_H
#define MEPARSER_H

#include <vector>

#include "basetypes.h"
#include "ustring.h"
#include "ubytearray.h"
#include "treemodel.h"
#include "me.h"

// TODO: implement ME region parser

class MeParser 
{
public:
    // Default constructor and destructor
    MeParser(TreeModel* treeModel) { U_UNUSED_PARAMETER(treeModel); }
    ~MeParser() {}

    // Returns messages 
    std::vector<std::pair<UString, UModelIndex> > getMessages() const { return std::vector<std::pair<UString, UModelIndex> >(); }
    // Clears messages
    void clearMessages() {}

    // ME parsing
    USTATUS parseMeRegionBody(const UModelIndex & index) { U_UNUSED_PARAMETER(index); return U_SUCCESS; }
};
#endif // MEPARSER_H
