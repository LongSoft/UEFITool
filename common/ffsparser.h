/* ffsparser.h

Copyright (c) 2017, LongSoft. All rights reserved.
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

#include "basetypes.h"
#include "ustring.h"
#include "ubytearray.h"
#include "treemodel.h"
#include "bootguard.h"
#include "fit.h"

typedef struct BG_PROTECTED_RANGE_ {
    UINT32     Offset;
    UINT32     Size;
    UINT8      Type;
    UByteArray Hash;
} BG_PROTECTED_RANGE;

#define BG_PROTECTED_RANGE_INTEL_BOOT_GUARD_IBB      0x01
#define BG_PROTECTED_RANGE_INTEL_BOOT_GUARD_POST_IBB 0x02
#define BG_PROTECTED_RANGE_VENDOR_HASH_PHOENIX       0x03
#define BG_PROTECTED_RANGE_VENDOR_HASH_AMI_OLD       0x04
#define BG_PROTECTED_RANGE_VENDOR_HASH_AMI_NEW       0x05
#define BG_PROTECTED_RANGE_VENDOR_HASH_MICROSOFT     0x06

class NvramParser;
class MeParser;

class FfsParser
{
public:
    // Constructor and destructor
    FfsParser(TreeModel* treeModel);
    ~FfsParser();

    // Obtain parser messages 
    std::vector<std::pair<UString, UModelIndex> > getMessages() const;
    // Clear messages
    void clearMessages() { messagesVector.clear(); }

    // Parse firmware image
    USTATUS parse(const UByteArray &buffer);
    
    // Obtain parsed FIT table
    std::vector<std::pair<std::vector<UString>, UModelIndex> > getFitTable() const { return fitTable; }

    // Obtain Security Info
    UString getSecurityInfo() const { return securityInfo; }

    // Obtain offset/address difference
    UINT64 getAddressDiff() { return addressDiff; }

private:
    TreeModel *model;
    std::vector<std::pair<UString, UModelIndex> > messagesVector;
    void msg(const UString & message, const UModelIndex & index = UModelIndex()) {
        messagesVector.push_back(std::pair<UString, UModelIndex>(message, index));
    };

    NvramParser* nvramParser;
    MeParser* meParser;
 
    UByteArray openedImage;
    UModelIndex lastVtf;
    UINT32 imageBase;
    UINT64 addressDiff;
    std::vector<std::pair<std::vector<UString>, UModelIndex> > fitTable;
    
    UString securityInfo;
    bool bgAcmFound;
    bool bgKeyManifestFound;
    bool bgBootPolicyFound;
    UByteArray bgKmHash;
    UByteArray bgBpHash;
    UByteArray bgBpDigest;
    std::vector<BG_PROTECTED_RANGE> bgProtectedRanges;
    UINT64 bgProtectedRegionsBase;
    UModelIndex bgDxeCoreIndex;

    // First pass
    USTATUS performFirstPass(const UByteArray & imageFile, UModelIndex & index);

    USTATUS parseCapsule(const UByteArray & capsule, const UINT32 localOffset, const UModelIndex & parent, UModelIndex & index);
    USTATUS parseIntelImage(const UByteArray & intelImage, const UINT32 localOffset, const UModelIndex & parent, UModelIndex & index);
    USTATUS parseGenericImage(const UByteArray & intelImage, const UINT32 localOffset, const UModelIndex & parent, UModelIndex & index);

    USTATUS parseBpdtRegion(const UByteArray & region, const UINT32 localOffset, const UINT32 sbpdtOffsetFixup, const UModelIndex & parent, UModelIndex & index);
    USTATUS parseCpdRegion(const UByteArray & region, const UINT32 localOffset, const UModelIndex & parent, UModelIndex & index);
    USTATUS parseCpdExtensionsArea(const UModelIndex & index);
    USTATUS parseSignedPackageInfoData(const UModelIndex & index);
    
    USTATUS parseRawArea(const UModelIndex & index);
    USTATUS parseVolumeHeader(const UByteArray & volume, const UINT32 localOffset, const UModelIndex & parent, UModelIndex & index);
    USTATUS parseVolumeBody(const UModelIndex & index);
    USTATUS parseMicrocodeVolumeBody(const UModelIndex & index);
    USTATUS parseFileHeader(const UByteArray & file, const UINT32 localOffset, const UModelIndex & parent, UModelIndex & index);
    USTATUS parseFileBody(const UModelIndex & index);
    USTATUS parseSectionHeader(const UByteArray & section, const UINT32 localOffset, const UModelIndex & parent, UModelIndex & index, const bool insertIntoTree);
    USTATUS parseSectionBody(const UModelIndex & index);

    USTATUS parseGbeRegion(const UByteArray & gbe, const UINT32 localOffset, const UModelIndex & parent, UModelIndex & index);
    USTATUS parseMeRegion(const UByteArray & me, const UINT32 localOffset, const UModelIndex & parent, UModelIndex & index);
    USTATUS parseBiosRegion(const UByteArray & bios, const UINT32 localOffset, const UModelIndex & parent, UModelIndex & index);
    USTATUS parsePdrRegion(const UByteArray & pdr, const UINT32 localOffset, const UModelIndex & parent, UModelIndex & index);
    USTATUS parseDevExp1Region(const UByteArray & devExp1, const UINT32 localOffset, const UModelIndex & parent, UModelIndex & index);
    USTATUS parseGenericRegion(const UINT8 subtype, const UByteArray & region, const UINT32 localOffset, const UModelIndex & parent, UModelIndex & index);

    USTATUS parsePadFileBody(const UModelIndex & index);
    USTATUS parseVolumeNonUefiData(const UByteArray & data, const UINT32 localOffset, const UModelIndex & index);

    USTATUS parseSections(const UByteArray & sections, const UModelIndex & index, const bool insertIntoTree);
    USTATUS parseCommonSectionHeader(const UByteArray & section, const UINT32 localOffset, const UModelIndex & parent, UModelIndex & index, const bool insertIntoTree);
    USTATUS parseCompressedSectionHeader(const UByteArray & section, const UINT32 localOffset, const UModelIndex & parent, UModelIndex & index, const bool insertIntoTree);
    USTATUS parseGuidedSectionHeader(const UByteArray & section, const UINT32 localOffset, const UModelIndex & parent, UModelIndex & index, const bool insertIntoTree);
    USTATUS parseFreeformGuidedSectionHeader(const UByteArray & section, const UINT32 localOffset, const UModelIndex & parent, UModelIndex & index, const bool insertIntoTree);
    USTATUS parseVersionSectionHeader(const UByteArray & section, const UINT32 localOffset, const UModelIndex & parent, UModelIndex & index, const bool insertIntoTree);
    USTATUS parsePostcodeSectionHeader(const UByteArray & section, const UINT32 localOffset, const UModelIndex & parent, UModelIndex & index, const bool insertIntoTree);

    USTATUS parseCompressedSectionBody(const UModelIndex & index);
    USTATUS parseGuidedSectionBody(const UModelIndex & index);
    USTATUS parseVersionSectionBody(const UModelIndex & index);
    USTATUS parseDepexSectionBody(const UModelIndex & index);
    USTATUS parseUiSectionBody(const UModelIndex & index);
    USTATUS parseRawSectionBody(const UModelIndex & index);
    USTATUS parsePeImageSectionBody(const UModelIndex & index);
    USTATUS parseTeImageSectionBody(const UModelIndex & index);

    USTATUS parseAprioriRawSection(const UByteArray & body, UString & parsed);
    USTATUS findNextRawAreaItem(const UModelIndex & index, const UINT32 localOffset, UINT8 & nextItemType, UINT32 & nextItemOffset, UINT32 & nextItemSize, UINT32 & nextItemAlternativeSize);
    UINT32  getFileSize(const UByteArray & volume, const UINT32 fileOffset, const UINT8 ffsVersion);
    UINT32  getSectionSize(const UByteArray & file, const UINT32 sectionOffset, const UINT8 ffsVersion);
    
    USTATUS parseIntelMicrocodeHeader(const UByteArray & store, const UINT32 localOffset, const UModelIndex & parent, UModelIndex & index);
    BOOLEAN microcodeHeaderValid(const INTEL_MICROCODE_HEADER* ucodeHeader);

    // Second pass
    USTATUS performSecondPass(const UModelIndex & index);
    USTATUS addInfoRecursive(const UModelIndex & index);
    USTATUS checkTeImageBase(const UModelIndex & index);
    USTATUS checkProtectedRanges(const UModelIndex & index);
    USTATUS markProtectedRangeRecursive(const UModelIndex & index, const BG_PROTECTED_RANGE & range);

    USTATUS parseResetVectorData();
    USTATUS parseFit(const UModelIndex & index);
    USTATUS parseVendorHashFile(const UByteArray & fileGuid, const UModelIndex & index);


#ifdef U_ENABLE_FIT_PARSING_SUPPORT
    void findFitRecursive(const UModelIndex & index, UModelIndex & found, UINT32 & fitOffset);

    // FIT entries
    USTATUS parseFitEntryMicrocode(const UByteArray & microcode, const UINT32 localOffset, const UModelIndex & parent, UString & info, UINT32 &realSize);
    USTATUS parseFitEntryAcm(const UByteArray & acm, const UINT32 localOffset, const UModelIndex & parent, UString & info, UINT32 &realSize);
    USTATUS parseFitEntryBootGuardKeyManifest(const UByteArray & keyManifest, const UINT32 localOffset, const UModelIndex & parent, UString & info, UINT32 &realSize);
    USTATUS parseFitEntryBootGuardBootPolicy(const UByteArray & bootPolicy, const UINT32 localOffset, const UModelIndex & parent, UString & info, UINT32 &realSize);
    USTATUS findNextBootGuardBootPolicyElement(const UByteArray & bootPolicy, const UINT32 elementOffset, UINT32 & nextElementOffset, UINT32 & nextElementSize);
#endif

#ifdef U_ENABLE_NVRAM_PARSING_SUPPORT
    friend class NvramParser; // Make FFS parsing routines accessible to NvramParser
#endif
    
#ifdef U_ENABLE_ME_PARSING_SUPPORT
    friend class MeParser; // Make FFS parsing routines accessible to MeParser
#endif
};

#endif // FFSPARSER_H
