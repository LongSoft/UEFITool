/* ffsparser.h

Copyright (c) 2016, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHWARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*/

#ifndef FFSPARSER_H
#define FFSPARSER_H

#include <vector>

#include "ustring.h"
#include "ubytearray.h"
#include "basetypes.h"
#include "treemodel.h"
#include "utility.h"
#include "peimage.h"
#include "parsingdata.h"
#include "types.h"
#include "treemodel.h"
#include "descriptor.h"
#include "ffs.h"
#include "gbe.h"
#include "me.h"
#include "fit.h"
#include "nvram.h"

class TreeModel;

class FfsParser
{
public:
    // Default constructor and destructor
    FfsParser(TreeModel* treeModel) : model(treeModel), capsuleOffsetFixup(0) {}
    ~FfsParser() {}

    // Returns messages 
    std::vector<std::pair<UString, UModelIndex> > getMessages() const { return messagesVector; }
    // Clears messages
    void clearMessages() { messagesVector.clear(); }

    // Firmware image parsing
    USTATUS parse(const UByteArray &buffer);
    
    // Obtain parsed FIT table
    std::vector<std::vector<UString> > getFitTable() const { return fitTable; }

private:
    TreeModel *model;
    std::vector<std::pair<UString, UModelIndex> > messagesVector;
    void msg(const UString message, const UModelIndex index = UModelIndex()) {
        messagesVector.push_back(std::pair<UString, UModelIndex>(message, index));
    };

    UModelIndex lastVtf;
    UINT32 capsuleOffsetFixup;
    std::vector<std::vector<UString> > fitTable;

    // First pass
    USTATUS performFirstPass(const UByteArray & imageFile, UModelIndex & index);

    USTATUS parseRawArea(const UModelIndex & index);
    USTATUS parseVolumeHeader(const UByteArray & volume, const UINT32 parentOffset, const UModelIndex & parent, UModelIndex & index);
    USTATUS parseVolumeBody(const UModelIndex & index);
    USTATUS parseFileHeader(const UByteArray & file, const UINT32 parentOffset, const UModelIndex & parent, UModelIndex & index);
    USTATUS parseFileBody(const UModelIndex & index);
    USTATUS parseSectionHeader(const UByteArray & section, const UINT32 parentOffset, const UModelIndex & parent, UModelIndex & index, const bool preparse = false);
    USTATUS parseSectionBody(const UModelIndex & index);

    USTATUS parseIntelImage(const UByteArray & intelImage, const UINT32 parentOffset, const UModelIndex & parent, UModelIndex & root);
    USTATUS parseGbeRegion(const UByteArray & gbe, const UINT32 parentOffset, const UModelIndex & parent, UModelIndex & index);
    USTATUS parseMeRegion(const UByteArray & me, const UINT32 parentOffset, const UModelIndex & parent, UModelIndex & index);
    USTATUS parseBiosRegion(const UByteArray & bios, const UINT32 parentOffset, const UModelIndex & parent, UModelIndex & index);
    USTATUS parsePdrRegion(const UByteArray & pdr, const UINT32 parentOffset, const UModelIndex & parent, UModelIndex & index);
    USTATUS parseGeneralRegion(const UINT8 subtype, const UByteArray & region, const UINT32 parentOffset, const UModelIndex & parent, UModelIndex & index);

    USTATUS parsePadFileBody(const UModelIndex & index);
    USTATUS parseVolumeNonUefiData(const UByteArray & data, const UINT32 parentOffset, const UModelIndex & index);

    USTATUS parseSections(const UByteArray & sections, const UModelIndex & index, const bool preparse = false);
    USTATUS parseCommonSectionHeader(const UByteArray & section, const UINT32 parentOffset, const UModelIndex & parent, UModelIndex & index, const bool preparse);
    USTATUS parseCompressedSectionHeader(const UByteArray & section, const UINT32 parentOffset, const UModelIndex & parent, UModelIndex & index, const bool preparse);
    USTATUS parseGuidedSectionHeader(const UByteArray & section, const UINT32 parentOffset, const UModelIndex & parent, UModelIndex & index, const bool preparse);
    USTATUS parseFreeformGuidedSectionHeader(const UByteArray & section, const UINT32 parentOffset, const UModelIndex & parent, UModelIndex & index, const bool preparse);
    USTATUS parseVersionSectionHeader(const UByteArray & section, const UINT32 parentOffset, const UModelIndex & parent, UModelIndex & index, const bool preparse);
    USTATUS parsePostcodeSectionHeader(const UByteArray & section, const UINT32 parentOffset, const UModelIndex & parent, UModelIndex & index, const bool preparse);

    USTATUS parseCompressedSectionBody(const UModelIndex & index);
    USTATUS parseGuidedSectionBody(const UModelIndex & index);
    USTATUS parseVersionSectionBody(const UModelIndex & index);
    USTATUS parseDepexSectionBody(const UModelIndex & index);
    USTATUS parseUiSectionBody(const UModelIndex & index);
    USTATUS parseRawSectionBody(const UModelIndex & index);
    USTATUS parsePeImageSectionBody(const UModelIndex & index);
    USTATUS parseTeImageSectionBody(const UModelIndex & index);

    UINT8  getPaddingType(const UByteArray & padding);
    USTATUS parseAprioriRawSection(const UByteArray & body, UString & parsed);
    USTATUS findNextVolume(const UModelIndex & index, const UByteArray & bios, const UINT32 parentOffset, const UINT32 volumeOffset, UINT32 & nextVolumeOffset);
    USTATUS getVolumeSize(const UByteArray & bios, const UINT32 volumeOffset, UINT32 & volumeSize, UINT32 & bmVolumeSize);
    UINT32 getFileSize(const UByteArray & volume, const UINT32 fileOffset, const UINT8 ffsVersion);
    UINT32 getSectionSize(const UByteArray & file, const UINT32 sectionOffset, const UINT8 ffsVersion);

    // NVRAM parsing
    USTATUS parseNvramVolumeBody(const UModelIndex & index);
    USTATUS findNextStore(const UModelIndex & index, const UByteArray & volume, const UINT32 parentOffset, const UINT32 storeOffset, UINT32 & nextStoreOffset);
    USTATUS getStoreSize(const UByteArray & data, const UINT32 storeOffset, UINT32 & storeSize);
    USTATUS parseStoreHeader(const UByteArray & store, const UINT32 parentOffset, const UModelIndex & parent, UModelIndex & index);
    
    USTATUS parseNvarStore(const UModelIndex & index);
    USTATUS parseVssStoreHeader(const UByteArray & store, const UINT32 parentOffset, const UModelIndex & parent, UModelIndex & index);
    USTATUS parseFtwStoreHeader(const UByteArray & store, const UINT32 parentOffset, const UModelIndex & parent, UModelIndex & index);
    USTATUS parseFdcStoreHeader(const UByteArray & store, const UINT32 parentOffset, const UModelIndex & parent, UModelIndex & index);
    USTATUS parseFsysStoreHeader(const UByteArray & store, const UINT32 parentOffset, const UModelIndex & parent, UModelIndex & index);
    USTATUS parseEvsaStoreHeader(const UByteArray & store, const UINT32 parentOffset, const UModelIndex & parent, UModelIndex & index);
    USTATUS parseFlashMapStoreHeader(const UByteArray & store, const UINT32 parentOffset, const UModelIndex & parent, UModelIndex & index);
    USTATUS parseCmdbStoreHeader(const UByteArray & store, const UINT32 parentOffset, const UModelIndex & parent, UModelIndex & index);
    USTATUS parseSlicPubkeyHeader(const UByteArray & store, const UINT32 parentOffset, const UModelIndex & parent, UModelIndex & index);
    USTATUS parseSlicMarkerHeader(const UByteArray & store, const UINT32 parentOffset, const UModelIndex & parent, UModelIndex & index);
    USTATUS parseIntelMicrocodeHeader(const UByteArray & store, const UINT32 parentOffset, const UModelIndex & parent, UModelIndex & index);
    
    USTATUS parseVssStoreBody(const UModelIndex & index);
    USTATUS parseFsysStoreBody(const UModelIndex & index);
    USTATUS parseEvsaStoreBody(const UModelIndex & index);
    USTATUS parseFlashMapBody(const UModelIndex & index);

    // Second pass
    USTATUS performSecondPass(const UModelIndex & index);
    USTATUS addOffsetsRecursive(const UModelIndex & index);
    USTATUS addMemoryAddressesRecursive(const UModelIndex & index, const UINT32 diff);
    USTATUS addFixedAndCompressedRecursive(const UModelIndex & index);
    USTATUS parseFit(const UModelIndex & index, const UINT32 diff);
    USTATUS findFitRecursive(const UModelIndex & index, const UINT32 diff, UModelIndex & found, UINT32 & fitOffset);
};

#endif // FFSPARSER_H
