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
#include "ffsparser.h"
#include "sha256.h"

#ifdef U_ENABLE_ME_PARSING_SUPPORT
class MeParser 
{
public:
    // Default constructor and destructor
    MeParser(TreeModel* treeModel, FfsParser* parser) : model(treeModel), ffsParser(parser) {}
    ~MeParser() {}

    // Returns messages 
    std::vector<std::pair<UString, UModelIndex> > getMessages() const { return messagesVector; }
    // Clears messages
    void clearMessages() { messagesVector.clear(); }

    // ME parsing
    USTATUS parseMeRegionBody(const UModelIndex & index);
private:
    TreeModel *model;
    FfsParser *ffsParser;
    std::vector<std::pair<UString, UModelIndex> > messagesVector;

    void msg(const UString message, const UModelIndex index = UModelIndex()) {
        messagesVector.push_back(std::pair<UString, UModelIndex>(message, index));
    }

    USTATUS parseFptRegion(const UByteArray & region, const UModelIndex & parent, UModelIndex & index);
    USTATUS parseIfwi16Region(const UByteArray & region, const UModelIndex & parent, UModelIndex & index);
    USTATUS parseIfwi17Region(const UByteArray & region, const UModelIndex & parent, UModelIndex & index);
};
#else
class MeParser 
{
public:
    // Default constructor and destructor
    MeParser(TreeModel* treeModel, FfsParser* parser) { U_UNUSED_PARAMETER(treeModel); U_UNUSED_PARAMETER(parser); }
    ~MeParser() {}

    // Returns messages 
    std::vector<std::pair<UString, UModelIndex> > getMessages() const { return std::vector<std::pair<UString, UModelIndex> >(); }
    // Clears messages
    void clearMessages() {}

    // ME parsing
    USTATUS parseMeRegionBody(const UModelIndex & index) { U_UNUSED_PARAMETER(index); return U_SUCCESS; }
};
#endif // U_ENABLE_ME_PARSING_SUPPORT
#endif // MEPARSER_H
