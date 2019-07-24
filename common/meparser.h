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
#include "sha256.h"

#ifdef U_ENABLE_ME_PARSING_SUPPORT

// FPT
#define ME_ROM_BYPASS_VECTOR_SIZE 0x10

const UByteArray ME_FPT_HEADER_SIGNATURE("\x24\x46\x50\x54", 4); //$FPT

typedef struct ME_FPT_HEADER_ {
    UINT32 Signature;
    UINT32 NumEntries;
    UINT8  HeaderVersion;
    UINT8  EntryVersion;
    UINT8  HeaderLength;
    UINT8  Checksum;      // One bit for Redundant before IFWI
    UINT16 TicksToAdd;
    UINT16 TokensToAdd;
    UINT32 UmaSize;       // Flags in SPS
    UINT32 FlashLayout;   // Crc32 before IFWI
    UINT16 FitcMajor;
    UINT16 FitcMinor;
    UINT16 FitcHotfix;
    UINT16 FitcBuild;
} ME_FPT_HEADER;

typedef struct ME_FPT_ENTRY_{
    CHAR8  PartitionName[4];
    UINT8  Reserved1;
    UINT32 Offset;
    UINT32 Length;
    UINT8  Reserved2[12];
    UINT32 PartitionType : 7;
    UINT32 CopyToDramCache : 1;
    UINT32 Reserved3 : 7;
    UINT32 BuiltWithLength1 : 1;
    UINT32 BuiltWithLength2 : 1;
    UINT32 Reserved4 : 7;
    UINT32 EntryValid : 8;
} ME_FPT_ENTRY;


// IFWI
typedef struct ME_IFWI_LAYOUT_HEADER_ {
    UINT8  RomBypassVector[16];
    UINT32 DataPartitionOffset;
    UINT32 DataPartitionSize;
    UINT32 Boot1Offset;
    UINT32 Boot1Size;
    UINT32 Boot2Offset;
    UINT32 Boot2Size;
    UINT32 Boot3Offset;
    UINT32 Boot3Size;
} ME_IFWI_LAYOUT_HEADER;


// BPDT
const UByteArray ME_BPDT_GREEN_SIGNATURE("\xAA\x55\x00\x00", 4); //0x000055AA
const UByteArray ME_BPDT_YELLOW_SIGNATURE("\xAA\x55\xAA\x00", 4); //0x00AA55AA

typedef struct ME_BPDT_HEADER_ {
    UINT32 Signature;
    UINT16 NumEntries;
    UINT16 Version;
    UINT32 Checksum;
    UINT32 IfwiVersion;
    UINT16 FitcMajor;
    UINT16 FitcMinor;
    UINT16 FitcHotfix;
    UINT16 FitcBuild;
} ME_BPDT_HEADER ;

typedef struct ME_BPDT_ENTRY_ {
    UINT32 Type : 16;
    UINT32 SplitSubPartitionFirstPart : 1;
    UINT32 SplitSubPartitionSecondPart : 1;
    UINT32 CodeSubPartition : 1;
    UINT32 UmaCachable : 1;
    UINT32 Reserved: 12;
    UINT32 Offset;
    UINT32 Length;
} ME_BPDT_ENTRY;

// CPD
const UByteArray ME_CPD_SIGNATURE("\x24\x43\x50\x44", 4); //$CPD

typedef struct ME_CPD_HEADER_ {
    UINT32 Signature;
    UINT32 NumEntries;
    UINT8  HeaderVersion;
    UINT8  EntryVersion;
    UINT8  HeaderLength;
    UINT8  HeaderChecksum;
    UINT8  ShortName[4];
} ME_CPD_HEADER;

typedef struct ME_BPDT_CPD_ENTRY_ {
    UINT8  EntryName[12];
    struct {
        UINT32 Offset : 25;
        UINT32 HuffmanCompressed : 1;
        UINT32 Reserved : 6;
    } Offset;
    UINT32 Length;
    UINT32 Reserved;
} ME_BPDT_CPD_ENTRY;

typedef struct ME_CPD_MANIFEST_HEADER_ {
    UINT32 HeaderType;
    UINT32 HeaderLength;
    UINT32 HeaderVersion;
    UINT32 Flags;
    UINT32 Vendor;
    UINT32 Date;
    UINT32 Size;
    UINT32 HeaderId;
    UINT32 Reserved1;
    UINT16 VersionMajor;
    UINT16 VersionMinor;
    UINT16 VersionBugfix;
    UINT16 VersionBuild;
    UINT32 SecurityVersion;
    UINT8  Reserved2[8];
    UINT8  Reserved3[64];
    UINT32 ModulusSize;
    UINT32 ExponentSize;
    //manifest_rsa_key_t public_key;
    //manifest_signature_t signature;
} ME_CPD_MANIFEST_HEADER;

typedef struct ME_CPD_EXTENTION_HEADER_ {
   UINT32 Type;
   UINT32 Length;
} ME_CPD_EXTENTION_HEADER;

typedef struct ME_CPD_EXT_SIGNED_PACKAGE_INFO_MODULES_ {
    UINT8  Name[12];
    UINT8  Type;
    UINT8  HashAlgorithm;
    UINT16 HashSize;
    UINT32 MetadataSize;
    UINT8  MetadataHash[32];
} ME_CPD_EXT_SIGNED_PACKAGE_INFO_MODULES;

typedef struct ME_CPD_EXT_SIGNED_PACKAGE_INFO_ {
    UINT32 ExtensionType;
    UINT32 ExtensionLength;
    UINT8  PackageName[4];
    UINT32 Vcn;
    UINT8  UsageBitmap[16];
    UINT32 Svn;
    UINT8  Reserved[16];
    // ME_EXT_SIGNED_PACKAGE_INFO_MODULES Modules[];
} ME_CPD_EXT_SIGNED_PACKAGE_INFO;

typedef struct ME_CPD_EXT_MODULE_ATTRIBUTES_ {
    UINT32 ExtensionType;
    UINT32 ExtensionLength;
    UINT8  CompressionType;
    UINT8  Reserved[3];
    UINT32 UncompressedSize;
    UINT32 CompressedSize;
    UINT32 GlobalModuleId;
    UINT8  ImageHash[32];
} ME_CPD_EXT_MODULE_ATTRIBUTES;

typedef struct ME_CPD_EXT_IFWI_PARTITION_MANIFEST_ {
    UINT32 ExtensionType;
    UINT32 ExtensionLength;
    UINT8  PartitionName[4];
    UINT32 CompletePartitionLength;
    UINT16 PartitionVersionMinor;
    UINT16 PartitionVersionMajor;
    UINT32 DataFormatVersion;
    UINT32 InstanceId;
    UINT32 SupportMultipleInstances : 1;
    UINT32 SupportApiVersionBasedUpdate : 1;
    UINT32 ActionOnUpdate : 2;
    UINT32 ObeyFullUpdateRules : 1;
    UINT32 IfrEnableOnly : 1;
    UINT32 AllowCrossPointUpdate : 1;
    UINT32 AllowCrossHotfixUpdate : 1;
    UINT32 PartialUpdateOnly : 1;
    UINT32 ReservedFlags : 23;
    UINT32 HashAlgorithm : 8;
    UINT32 HashSize : 24;
    UINT8  CompletePartitionHash[32];
    UINT8  Reserved[20];
} ME_CPD_EXT_IFWI_PARTITION_MANIFEST;

#define ME_MODULE_COMPRESSION_TYPE_UNCOMPRESSED 0
#define ME_MODULE_COMPRESSION_TYPE_HUFFMAN 1
#define ME_MODULE_COMPRESSION_TYPE_LZMA 2

#define ME_MANIFEST_HEADER_ID 0x324E4D24 //$MN2


class MeParser 
{
public:
    // Default constructor and destructor
    MeParser(TreeModel* treeModel) : model(treeModel) {}
    ~MeParser() {}

    // Returns messages 
    std::vector<std::pair<UString, UModelIndex> > getMessages() const { return messagesVector; }
    // Clears messages
    void clearMessages() { messagesVector.clear(); }

    // ME parsing
    USTATUS parseMeRegionBody(const UModelIndex & index);
    
private:
    TreeModel *model;
    std::vector<std::pair<UString, UModelIndex> > messagesVector;

    void msg(const UString message, const UModelIndex index = UModelIndex()) {
        messagesVector.push_back(std::pair<UString, UModelIndex>(message, index));
    }

    USTATUS parseFptRegion(const UByteArray & region, const UModelIndex & parent, UModelIndex & index);
    USTATUS parseIfwiRegion(const UByteArray & region, const UModelIndex & parent, UModelIndex & index);
    USTATUS parseBpdtRegion(const UByteArray & region, const UModelIndex & parent, UModelIndex & index);

    USTATUS parseCodePartitionDirectory(const UByteArray & directory, const UINT32 localOffset, const UModelIndex & parent, UModelIndex & index);
    USTATUS parseExtensionsArea(const UModelIndex & index);
    USTATUS parseSignedPackageInfoData(const UModelIndex & index);
};
#else
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
#endif // U_ENABLE_ME_PARSING_SUPPORT
#endif // MEPARSER_H
