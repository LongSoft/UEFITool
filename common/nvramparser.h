/* nvramparser.h

Copyright (c) 2016, Nikolaj Schlej. All rights reserved.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#ifndef NVRAMPARSER_H
#define NVRAMPARSER_H

#include <vector>

#include "basetypes.h"
#include "ustring.h"
#include "ubytearray.h"
#include "treemodel.h"
#include "ffsparser.h"

#ifdef U_ENABLE_NVRAM_PARSING_SUPPORT
class NvramParser
{
public:
    // Default constructor and destructor
    NvramParser(TreeModel* treeModel, FfsParser* parser) : model(treeModel), ffsParser(parser) {}
    ~NvramParser() {}

    // Returns messages
    std::vector<std::pair<UString, UModelIndex> > getMessages() const { return messagesVector; }
    // Clears messages
    void clearMessages() { messagesVector.clear(); }

    // NVRAM parsing
    USTATUS parseNvramVolumeBody(const UModelIndex & index);
    USTATUS parseNvarStore(const UModelIndex & index);
    
private:
    TreeModel *model;
    FfsParser *ffsParser;
    std::vector<std::pair<UString, UModelIndex> > messagesVector;
    void msg(const UString & message, const UModelIndex & index = UModelIndex()) {
        messagesVector.push_back(std::pair<UString, UModelIndex>(message, index));
    };
};
#else
class NvramParser
{
public:
    // Default constructor and destructor
    NvramParser(TreeModel* treeModel, FfsParser* parser) { U_UNUSED_PARAMETER(treeModel); U_UNUSED_PARAMETER(parser); }
    ~NvramParser() {}

    // Returns messages
    std::vector<std::pair<UString, UModelIndex> > getMessages() const { return std::vector<std::pair<UString, UModelIndex> >(); }
    // Clears messages
    void clearMessages() {}

    // NVRAM parsing
    USTATUS parseNvramVolumeBody(const UModelIndex &) { return U_SUCCESS; }
    USTATUS parseNvarStore(const UModelIndex &)  { return U_SUCCESS; }
};
#endif // U_ENABLE_NVRAM_PARSING_SUPPORT
#endif // NVRAMPARSER_H
