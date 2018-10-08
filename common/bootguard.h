/* bootguard.h

Copyright (c) 2017, LongSoft. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#ifndef BOOTGUARD_H
#define BOOTGUARD_H

#include "basetypes.h"
#include "sha256.h"

#pragma pack(push, 1)

const UByteArray BG_VENDOR_HASH_FILE_GUID_PHOENIX // 389CC6F2-1EA8-467B-AB8A-78E769AE2A15
("\xF2\xC6\x9C\x38\xA8\x1E\x7B\x46\xAB\x8A\x78\xE7\x69\xAE\x2A\x15", 16);

#define BG_VENDOR_HASH_FILE_SIGNATURE_PHOENIX (*(UINT64 *)"$HASHTBL")

const UByteArray BG_VENDOR_HASH_FILE_GUID_AMI // CBC91F44-A4BC-4A5B-8696-703451D0B053
("\x44\x1F\xC9\xCB\xBC\xA4\x5B\x4A\x86\x96\x70\x34\x51\xD0\xB0\x53", 16);

typedef struct BG_VENDOR_HASH_FILE_ENTRY
{
    UINT8  Hash[SHA256_DIGEST_SIZE];
    UINT32 Offset;
    UINT32 Size;
} BG_VENDOR_HASH_FILE_ENTRY;

typedef struct BG_VENDOR_HASH_FILE_HEADER_PHOENIX_
{
    UINT64 Signature;
    UINT32 NumEntries;
    //BG_VENDOR_HASH_FILE_ENTRY Entries[];
} BG_VENDOR_HASH_FILE_HEADER_PHOENIX;

typedef struct BG_VENDOR_HASH_FILE_HEADER_AMI_NEW_
{
    BG_VENDOR_HASH_FILE_ENTRY Entries[2];
} BG_VENDOR_HASH_FILE_HEADER_AMI_NEW;

typedef struct BG_VENDOR_HASH_FILE_HEADER_AMI_OLD_
{
    UINT8  Hash[SHA256_DIGEST_SIZE];
    UINT32 Size;
    // Offset is derived from flash map, will be detected as root volume with DXE core
} BG_VENDOR_HASH_FILE_HEADER_AMI_OLD;

typedef struct BG_MICROSOFT_PMDA_HEADER_
{
    UINT32 Version;
    UINT32 NumEntries;
} BG_MICROSOFT_PMDA_HEADER;

#define BG_MICROSOFT_PMDA_VERSION 0x00000001

typedef struct BG_MICROSOFT_PMDA_ENTRY_
{
    UINT32 Address;
    UINT32 Size;
    UINT8  Hash[SHA256_DIGEST_SIZE];
} BG_MICROSOFT_PMDA_ENTRY;

//
// Intel ACM
//

#define INTEL_ACM_MODULE_TYPE               0x2
#define INTEL_ACM_MODULE_SUBTYPE_TXT_ACM    0x0
#define INTEL_ACM_MODULE_SUBTYPE_S_ACM      0x1
#define INTEL_ACM_MODULE_SUBTYPE_BOOTGUARD  0x3
#define INTEL_ACM_MODULE_VENDOR             0x8086

typedef struct INTEL_ACM_HEADER_ {
    UINT16 ModuleType;
    UINT16 ModuleSubtype;
    UINT32 HeaderType;
    UINT32 HeaderVersion;
    UINT16 ChipsetId;
    UINT16 Flags;
    UINT32 ModuleVendor;
    UINT8  DateDay;
    UINT8  DateMonth;
    UINT16 DateYear;
    UINT32 ModuleSize;
    UINT16 AcmSvn;
    UINT16 : 16;
    UINT32 Unknown1;
    UINT32 Unknown2;
    UINT32 GdtMax;
    UINT32 GdtBase;
    UINT32 SegmentSel;
    UINT32 EntryPoint;
    UINT8  Unknown3[64];
    UINT32 KeySize;
    UINT32 Unknown4;
    UINT8  RsaPubKey[256];
    UINT32 RsaPubExp;
    UINT8  RsaSig[256];
} INTEL_ACM_HEADER;

//
// Intel BootGuard Key Manifest
//
#define BG_BOOT_POLICY_MANIFEST_HEADER_TAG  (*(UINT64 *)"__ACBP__")
typedef struct BG_BOOT_POLICY_MANIFEST_HEADER_ {
    UINT64 Tag;
    UINT8  Version;
    UINT8  HeaderVersion;
    UINT8  PMBPMVersion;
    UINT8  BPSVN;
    UINT8  ACMSVN;
    UINT8  : 8;
    UINT16 NEMDataSize;
} BG_BOOT_POLICY_MANIFEST_HEADER;

typedef struct SHA256_HASH_ {
    UINT16 HashAlgorithmId;
    UINT16 Size;
    UINT8  HashBuffer[32];
} SHA256_HASH;

typedef struct RSA_PUBLIC_KEY_ {
    UINT8  Version;
    UINT16 KeySize;
    UINT32 Exponent;
    UINT8  Modulus[256];
} RSA_PUBLIC_KEY;

typedef struct RSA_SIGNATURE_ {
    UINT8  Version;
    UINT16 KeySize;
    UINT16 HashId;
    UINT8  Signature[256];
} RSA_SIGNATURE;

typedef struct KEY_SIGNATURE_ {
    UINT8          Version;
    UINT16         KeyId;
    RSA_PUBLIC_KEY PubKey;
    UINT16         SigScheme;
    RSA_SIGNATURE  Signature;
} BG_KEY_SIGNATURE;

#define BG_IBB_SEGMENT_FLAG_IBB      0x0
#define BG_IBB_SEGMENT_FLAG_NON_IBB  0x1
typedef struct BG_IBB_SEGMENT_ELEMENT_ {
UINT16: 16;
    UINT16 Flags;
    UINT32 Base;
    UINT32 Size;
} BG_IBB_SEGMENT_ELEMENT;

#define BG_BOOT_POLICY_MANIFEST_IBB_ELEMENT_TAG  (*(UINT64 *)"__IBBS__")
#define BG_IBB_FLAG_AUTHORITY_MEASURE            0x4

typedef struct BG_IBB_ELEMENT_ {
    UINT64                 Tag;
    UINT8                  Version;
    UINT16                 : 16;
    UINT8                  Unknown;
    UINT32                 Flags;
    UINT64                 IbbMchBar;
    UINT64                 VtdBar;
    UINT32                 PmrlBase;
    UINT32                 PmrlLimit;
    UINT64                 Unknown3;
    UINT64                 Unknown4;
    SHA256_HASH            IbbHash;
    UINT32                 EntryPoint;
    SHA256_HASH            Digest;
    UINT8                  IbbSegCount;
    // BG_IBB_SEGMENT_ELEMENT IbbSegment[];
} BG_IBB_ELEMENT;

#define BG_BOOT_POLICY_MANIFEST_PLATFORM_MANUFACTURER_ELEMENT_TAG  (*(UINT64 *)"__PMDA__")
typedef struct BG_PLATFORM_MANUFACTURER_ELEMENT_ {
    UINT64 Tag;
    UINT8  Version;
    UINT16 DataSize;
} BG_PLATFORM_MANUFACTURER_ELEMENT;

#define BG_BOOT_POLICY_MANIFEST_SIGNATURE_ELEMENT_TAG  (*(UINT64 *)"__PMSG__")
typedef struct BG_BOOT_POLICY_MANIFEST_SIGNATURE_ELEMENT_ {
    UINT64               Tag;
    UINT8                Version;
    BG_KEY_SIGNATURE     KeySignature;
} BG_BOOT_POLICY_MANIFEST_SIGNATURE_ELEMENT;

#define BG_KEY_MANIFEST_TAG  (*(UINT64 *)"__KEYM__")
typedef struct BG_KEY_MANIFEST_ {
    UINT64               Tag;
    UINT8                Version;
    UINT8                KmVersion;
    UINT8                KmSvn;
    UINT8                KmId;
    SHA256_HASH          BpKeyHash;
    BG_KEY_SIGNATURE     KeyManifestSignature;
} BG_KEY_MANIFEST;

#pragma pack(pop)

#endif // BOOTGUARD_H