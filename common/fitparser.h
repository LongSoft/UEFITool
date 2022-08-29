/* fitparser.h

Copyright (c) 2022, Nikolaj Schlej. All rights reserved.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#ifndef FITPARSER_H
#define FITPARSER_H

#include <vector>

#include "basetypes.h"
#include "ustring.h"
#include "ubytearray.h"
#include "treemodel.h"
#include "intel_fit.h"
#include "intel_microcode.h"
#include "ffsparser.h"

class FfsParser;

#ifdef U_ENABLE_FIT_PARSING_SUPPORT
class FitParser
{
public:
    // Default constructor and destructor
    FitParser(TreeModel* treeModel, FfsParser* parser) : model(treeModel), ffsParser(parser),
        bgAcmFound(false), bgKeyManifestFound(false), bgBootPolicyFound(false) {}
    ~FitParser() {}

    // Returns messages
    std::vector<std::pair<UString, UModelIndex> > getMessages() const { return messagesVector; }
    // Clears messages
    void clearMessages() { messagesVector.clear(); }

    // Obtain parsed FIT table
    std::vector<std::pair<std::vector<UString>, UModelIndex> > getFitTable() const { return fitTable; }
    
    // Obtain security info
    UString getSecurityInfo() const { return securityInfo; }
    
    // FIT parsing
    USTATUS parseFit(const UModelIndex & index);
        
private:
    TreeModel *model;
    FfsParser *ffsParser;
    std::vector<std::pair<UString, UModelIndex> > messagesVector;
    
    std::vector<std::pair<std::vector<UString>, UModelIndex> > fitTable;
    bool bgAcmFound;
    bool bgKeyManifestFound;
    bool bgBootPolicyFound;
    UByteArray bgKmHash;
    UByteArray bgBpHashSha256;
    UByteArray bgBpHashSha384;
    UString securityInfo;
    
    void msg(const UString message, const UModelIndex index = UModelIndex()) {
        messagesVector.push_back(std::pair<UString, UModelIndex>(message, index));
    }
    
    void findFitRecursive(const UModelIndex & index, UModelIndex & found, UINT32 & fitOffset);
    USTATUS parseFitEntryMicrocode(const UByteArray & microcode, const UINT32 localOffset, const UModelIndex & parent, UString & info, UINT32 &realSize);
    USTATUS parseFitEntryAcm(const UByteArray & acm, const UINT32 localOffset, const UModelIndex & parent, UString & info, UINT32 &realSize);
    USTATUS parseFitEntryBootGuardKeyManifest(const UByteArray & keyManifest, const UINT32 localOffset, const UModelIndex & parent, UString & info, UINT32 &realSize);
    USTATUS parseFitEntryBootGuardBootPolicy(const UByteArray & bootPolicy, const UINT32 localOffset, const UModelIndex & parent, UString & info, UINT32 &realSize);
};
#else // U_ENABLE_FIT_PARSING_SUPPORT
class FitParser
{
public:
    // Default constructor and destructor
    FitParser(TreeModel* treeModel, FfsParser* parser) { U_UNUSED_PARAMETER(treeModel); U_UNUSED_PARAMETER(parser); }
    ~FitParser() {}

    // Returns messages
    std::vector<std::pair<UString, UModelIndex> > getMessages() const { return std::vector<std::pair<UString, UModelIndex> >(); }
    // Clears messages
    void clearMessages() {}

    // Obtain parsed FIT table
    std::vector<std::pair<std::vector<UString>, UModelIndex> > getFitTable() const { return std::vector<std::pair<std::vector<UString>, UModelIndex> >(); }
    
    // Obtain security info
    UString getSecurityInfo() const { return UString(); }
    
    // FIT parsing
    USTATUS parseFit(const UModelIndex & index) { U_UNUSED_PARAMETER(index); return U_SUCCESS; }
};
#endif // U_ENABLE_FIT_PARSING_SUPPORT
#endif // FITPARSER_H
