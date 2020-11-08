

#include "bgmanifestparser.h"
#include "bootguard.h"

BGIBBManifestParser::BGIBBManifestParser(FfsParser* parser)
{
    manifestType = IBBM;
    ffsParser = parser;
}


BGIBBManifestParserIcelake::BGIBBManifestParserIcelake(FfsParser* parser)
{
    manifestType = IBBM;
    ffsParser = parser;
}

BGManifestParser::BGManifestParser()
{
    manifestType = UNKNOWN;
    manifestVersion = 0;

}

MANIFEST_TYPE BGManifestParser::GetManifestType()
{
    return manifestType;
}

BGKeyManifestParser::BGKeyManifestParser()
{
    manifestType = KEYM;
}

BGKeyManifestParserIcelake::BGKeyManifestParserIcelake()
{
    manifestType = KEYM;
}

USTATUS BGKeyManifestParser::ParseManifest(const QByteArray &keyManifest, const UINT32 localOffset, const QModelIndex &parent, QString &info, UINT32 &realSize, UString &securityInfo, TreeModel* model, UByteArray bgKmHash)
{
    U_UNUSED_PARAMETER(realSize);
       if ((UINT32)keyManifest.size() < localOffset + sizeof(BG_KEY_MANIFEST)) {
           return U_INVALID_BG_KEY_MANIFEST;
       }

       const BG_KEY_MANIFEST* header = (const BG_KEY_MANIFEST*)(keyManifest.constData() + localOffset);
       if (header->Tag != BG_KEY_MANIFEST_TAG) {
           return U_INVALID_BG_KEY_MANIFEST;
       }

       // Valid KM found
       info = usprintf("LocalOffset: %08Xh, KM Version: %02Xh, KM SVN: %02Xh, KM ID: %02Xh",
                       localOffset,
                       header->KmVersion,
                       header->KmSvn,
                       header->KmId
                       );

       // Add KM header info
       securityInfo += usprintf(
                                 "Intel BootGuard Key manifest found at base %Xh\n"
                                 "Tag: __KEYM__ Version: %02Xh KmVersion: %02Xh KmSvn: %02Xh KmId: %02Xh",
                                 model->base(parent) + localOffset,
                                 header->Version,
                                 header->KmVersion,
                                 header->KmSvn,
                                 header->KmId
                                 );

       // Add hash of Key Manifest PubKey, this hash will be written to FPFs
       UINT8 hash[SHA256_DIGEST_SIZE];
       sha256(&header->KeyManifestSignature.PubKey.Modulus, sizeof(header->KeyManifestSignature.PubKey.Modulus), hash);
       securityInfo += UString("\n\nKey Manifest RSA Public Key Hash:\n");
       for (UINT8 i = 0; i < sizeof(hash); i++) {
           securityInfo += usprintf("%02X", hash[i]);
       }

       // Add BpKeyHash
       securityInfo += UString("\n\nBoot Policy RSA Public Key Hash:\n");
       for (UINT8 i = 0; i < sizeof(header->BpKeyHash.HashBuffer); i++) {
           securityInfo += usprintf("%02X", header->BpKeyHash.HashBuffer[i]);
       }
       bgKmHash = UByteArray((const char*)header->BpKeyHash.HashBuffer, sizeof(header->BpKeyHash.HashBuffer));

       // Add Key Manifest PubKey
       securityInfo += usprintf("\n\nKey Manifest RSA Public Key (Exponent: %Xh):",
                                 header->KeyManifestSignature.PubKey.Exponent);
       for (UINT16 i = 0; i < sizeof(header->KeyManifestSignature.PubKey.Modulus); i++) {
           if (i % 32 == 0)
               securityInfo += UString("\n");
           securityInfo += usprintf("%02X", header->KeyManifestSignature.PubKey.Modulus[i]);
       }
       // Add Key Manifest Signature
       securityInfo += UString("\n\nKey Manifest RSA Signature:");
       for (UINT16 i = 0; i < sizeof(header->KeyManifestSignature.Signature.Signature); i++) {
           if (i % 32 == 0)
               securityInfo += UString("\n");
           securityInfo += usprintf("%02X", header->KeyManifestSignature.Signature.Signature[i]);
       }
       securityInfo += UString("\n------------------------------------------------------------------------\n\n");

       return U_SUCCESS;
}


USTATUS BGKeyManifestParserIcelake::ParseManifest(const QByteArray &keyManifest, const UINT32 localOffset, const QModelIndex &parent, QString &info, UINT32 &realSize, UString &securityInfo, TreeModel* model, UByteArray bgKmHash)
{
       U_UNUSED_PARAMETER(realSize);
        if ((UINT32)keyManifest.size() < localOffset + sizeof(BG_KEY_MANIFEST2)) {
            return U_INVALID_BG_KEY_MANIFEST;
        }

        const BG_KEY_MANIFEST2* header = (const BG_KEY_MANIFEST2*)(keyManifest.constData() + localOffset);
        if (header->Tag != BG_KEY_MANIFEST_TAG) {
            return U_INVALID_BG_KEY_MANIFEST;
        }

        // Valid KM found
        info = usprintf("LocalOffset: %08Xh, KM Version: %02Xh, KM SVN: %02Xh, KM ID: %02Xh",
                        localOffset,
                        header->KmVersion,
                        header->KmSvn,
                        header->KmId
                        );

        // Add KM header info
        securityInfo += usprintf(
                                  "Intel BootGuard Key manifest found at base %Xh\n"
                                  "Tag: __KEYM__ Version: %02Xh KmVersion: %02Xh KmSvn: %02Xh KmId: %02Xh TotalKeys: %02Xh SignatureOffset: %04Xh",
                                  model->base(parent) + localOffset,
                                  header->Version,
                                  header->KmVersion,
                                  header->KmSvn,
                                  header->KmId,
                                  header->TotalKeys,
                                  header->RSAEntryOffset
                                  );

        // Add hash of Key Manifest PubKey, this hash will be written to FPFs
        UINT8 hash[SHA256_DIGEST_SIZE];
        sha256(&header->KeyManifestSignature.PubKey.Modulus, sizeof(header->KeyManifestSignature.PubKey.Modulus), hash);
        securityInfo += UString("\n\nKey Manifest RSA Public Key Hash:\n");
        for (UINT8 i = 0; i < sizeof(hash); i++) {
            securityInfo += usprintf("%02X", hash[i]);
        }

        // Add BpKeyHash
        securityInfo += UString("\n\nBoot Policy RSA Public Key Hash:\n");
        for (UINT8 i = 0; i < sizeof(header->BpKeyHash.HashBuffer); i++) {
            securityInfo += usprintf("%02X", header->BpKeyHash.HashBuffer[i]);
        }
        bgKmHash = UByteArray((const char*)header->BpKeyHash.HashBuffer, sizeof(header->BpKeyHash.HashBuffer));

        // Add Key Manifest PubKey
        securityInfo += usprintf("\n\nKey Manifest RSA Public Key (Exponent: %Xh):",
                                  header->KeyManifestSignature.PubKey.Exponent);
        for (UINT16 i = 0; i < sizeof(header->KeyManifestSignature.PubKey.Modulus); i++) {
            if (i % 32 == 0)
                securityInfo += UString("\n");
            securityInfo += usprintf("%02X", header->KeyManifestSignature.PubKey.Modulus[i]);
        }
        // Add Key Manifest Signature
        securityInfo += UString("\n\nKey Manifest RSA Signature:");
        for (UINT16 i = 0; i < sizeof(header->KeyManifestSignature.Signature.Signature); i++) {
            if (i % 32 == 0)
                securityInfo += UString("\n");
            securityInfo += usprintf("%02X", header->KeyManifestSignature.Signature.Signature[i]);
        }
        securityInfo += UString("\n------------------------------------------------------------------------\n\n");

        return U_SUCCESS;

}

USTATUS BGIBBManifestParser::ParseManifest(const QByteArray &bootPolicy, const UINT32 localOffset, const QModelIndex &parent, QString &info, UINT32 &realSize, UString &securityInfo, TreeModel* model, UByteArray bgKmHash)
{
    U_UNUSED_PARAMETER(realSize);
        if ((UINT32)bootPolicy.size() < localOffset + sizeof(BG_BOOT_POLICY_MANIFEST_HEADER)) {
            return U_INVALID_BG_BOOT_POLICY;
        }

        const BG_BOOT_POLICY_MANIFEST_HEADER* header = (const BG_BOOT_POLICY_MANIFEST_HEADER*)(bootPolicy.constData() + localOffset);
        if (header->Tag != BG_BOOT_POLICY_MANIFEST_HEADER_TAG) {
            return U_INVALID_BG_BOOT_POLICY;
        }

        UINT32 bmSize = sizeof(BG_BOOT_POLICY_MANIFEST_HEADER);
        if ((UINT32)bootPolicy.size() < localOffset + bmSize) {
            return U_INVALID_BG_BOOT_POLICY;
        }

        // Valid BPM found
        info = usprintf("LocalOffset: %08Xh, BP SVN: %02Xh, ACM SVN: %02Xh",
                        localOffset,
                        header->BPSVN,
                        header->ACMSVN
                        );

        // Add BP header info
        securityInfo += usprintf(
                                  "Intel BootGuard Boot Policy Manifest found at base %Xh\n"
                                  "Tag: __ACBP__ Version: %02Xh HeaderVersion: %02Xh\n"
                                  "PMBPMVersion: %02Xh PBSVN: %02Xh ACMSVN: %02Xh NEMDataStack: %04Xh\n",
                                  model->base(parent) + localOffset,
                                  header->Version,
                                  header->HeaderVersion,
                                  header->PMBPMVersion,
                                  header->BPSVN,
                                  header->ACMSVN,
                                  header->NEMDataSize
                                  );

        // Iterate over elements to get them all
        UINT32 elementOffset = 0;
        UINT32 elementSize = 0;
        USTATUS status = this->ffsParser->findNextBootGuardBootPolicyElement(bootPolicy, localOffset + sizeof(BG_BOOT_POLICY_MANIFEST_HEADER), elementOffset, elementSize);
        while (status == U_SUCCESS) {
            const UINT64* currentPos = (const UINT64*)(bootPolicy.constData() + elementOffset);
            if (*currentPos == BG_BOOT_POLICY_MANIFEST_IBB_ELEMENT_TAG) {
                const BG_IBB_ELEMENT* elementHeader = (const BG_IBB_ELEMENT*)currentPos;
                // Valid IBB element found
                securityInfo += usprintf(
                                          "\nInitial Boot Block Element found at base %Xh\n"
                                          "Tag: __IBBS__       Version: %02Xh         Unknown: %02Xh\n"
                                          "Flags: %08Xh    IbbMchBar: %08Xh VtdBar: %08Xh\n"
                                          "PmrlBase: %08Xh PmrlLimit: %08Xh  EntryPoint: %08Xh",
                                          model->base(parent) + localOffset + elementOffset,
                                          elementHeader->Version,
                                          elementHeader->Unknown,
                                          elementHeader->Flags,
                                          elementHeader->IbbMchBar,
                                          elementHeader->VtdBar,
                                          elementHeader->PmrlBase,
                                          elementHeader->PmrlLimit,
                                          elementHeader->EntryPoint
                                          );

                // Add PostIbbHash
                securityInfo += UString("\n\nPost IBB Hash:\n");
                for (UINT8 i = 0; i < sizeof(elementHeader->IbbHash.HashBuffer); i++) {
                    securityInfo += usprintf("%02X", elementHeader->IbbHash.HashBuffer[i]);
                }

                // Check for non-empry PostIbbHash
                UByteArray postIbbHash((const char*)elementHeader->IbbHash.HashBuffer, sizeof(elementHeader->IbbHash.HashBuffer));
                if (postIbbHash.count('\x00') != postIbbHash.size() && postIbbHash.count('\xFF') != postIbbHash.size()) {
                    BG_PROTECTED_RANGE range;
                    range.Type = BG_PROTECTED_RANGE_INTEL_BOOT_GUARD_POST_IBB;
                    range.Hash = postIbbHash;
                    this->ffsParser->bgProtectedRanges.push_back(range);
                }

                // Add Digest
                this->ffsParser->bgBpDigest = UByteArray((const char*)elementHeader->Digest.HashBuffer, sizeof(elementHeader->Digest.HashBuffer));
                securityInfo += UString("\n\nIBB Digest:\n");
                for (UINT8 i = 0; i < (UINT8)(this->ffsParser->bgBpDigest.size()); i++) {
                    securityInfo += usprintf("%02X", (UINT8)(this->ffsParser->bgBpDigest).at(i));
                }

                // Add all IBB segments
                securityInfo += UString("\n\nIBB Segments:\n");
                const BG_IBB_SEGMENT_ELEMENT* segments = (const BG_IBB_SEGMENT_ELEMENT*)(elementHeader + 1);
                for (UINT8 i = 0; i < elementHeader->IbbSegCount; i++) {
                    securityInfo += usprintf("Flags: %04Xh Address: %08Xh Size: %08Xh\n",
                                              segments[i].Flags, segments[i].Base, segments[i].Size);
                    if (segments[i].Flags == BG_IBB_SEGMENT_FLAG_IBB) {
                        BG_PROTECTED_RANGE range;
                        range.Offset = segments[i].Base;
                        range.Size = segments[i].Size;
                        range.Type = BG_PROTECTED_RANGE_INTEL_BOOT_GUARD_IBB;
                        this->ffsParser->bgProtectedRanges.push_back(range);
                    }
                }
            }
            else if (*currentPos == BG_BOOT_POLICY_MANIFEST_PLATFORM_MANUFACTURER_ELEMENT_TAG) {
                const BG_PLATFORM_MANUFACTURER_ELEMENT* elementHeader = (const BG_PLATFORM_MANUFACTURER_ELEMENT*)currentPos;
                securityInfo += usprintf(
                                          "\nPlatform Manufacturer Data Element found at base %Xh\n"
                                          "Tag: __PMDA__ Version: %02Xh DataSize: %02Xh",
                                          model->base(parent) + localOffset + elementOffset,
                                          elementHeader->Version,
                                          elementHeader->DataSize
                                          );
                // Check for Microsoft PMDA hash data
                const BG_MICROSOFT_PMDA_HEADER* pmdaHeader = (const BG_MICROSOFT_PMDA_HEADER*)(elementHeader + 1);
                if (pmdaHeader->Version == BG_MICROSOFT_PMDA_VERSION
                    && elementHeader->DataSize == sizeof(BG_MICROSOFT_PMDA_HEADER) + sizeof(BG_MICROSOFT_PMDA_ENTRY)*pmdaHeader->NumEntries) {
                    // Add entries
                    securityInfo += UString("\nMicrosoft PMDA-based protected ranges:\n");
                    const BG_MICROSOFT_PMDA_ENTRY* entries = (const BG_MICROSOFT_PMDA_ENTRY*)(pmdaHeader + 1);
                    for (UINT32 i = 0; i < pmdaHeader->NumEntries; i++) {

                        securityInfo += usprintf("Address: %08Xh Size: %08Xh\n", entries[i].Address, entries[i].Size);
                        securityInfo += UString("Hash: ");
                        for (UINT8 j = 0; j < sizeof(entries[i].Hash); j++) {
                            securityInfo += usprintf("%02X", entries[i].Hash[j]);
                        }
                        securityInfo += UString("\n");

                        BG_PROTECTED_RANGE range;
                        range.Offset = entries[i].Address;
                        range.Size = entries[i].Size;
                        range.Hash = UByteArray((const char*)entries[i].Hash, sizeof(entries[i].Hash));
                        range.Type = BG_PROTECTED_RANGE_VENDOR_HASH_MICROSOFT;
                        this->ffsParser->bgProtectedRanges.push_back(range);
                    }
                }
                else {
                    // Add raw data
                    const UINT8* data = (const UINT8*)(elementHeader + 1);
                    for (UINT16 i = 0; i < elementHeader->DataSize; i++) {
                        if (i % 32 == 0)
                            securityInfo += UString("\n");
                        securityInfo += usprintf("%02X", data[i]);
                    }
                    securityInfo += UString("\n");
                }
            }
            else if (*currentPos == BG_BOOT_POLICY_MANIFEST_SIGNATURE_ELEMENT_TAG) {
                const BG_BOOT_POLICY_MANIFEST_SIGNATURE_ELEMENT* elementHeader = (const BG_BOOT_POLICY_MANIFEST_SIGNATURE_ELEMENT*)currentPos;
                securityInfo += usprintf(
                                          "\nBoot Policy Signature Element found at base %Xh\n"
                                          "Tag: __PMSG__ Version: %02Xh",
                                          model->base(parent) + localOffset + elementOffset,
                                          elementHeader->Version
                                          );

                // Add PubKey
                securityInfo += usprintf("\n\nBoot Policy RSA Public Key (Exponent: %Xh):", elementHeader->KeySignature.PubKey.Exponent);
                for (UINT16 i = 0; i < sizeof(elementHeader->KeySignature.PubKey.Modulus); i++) {
                    if (i % 32 == 0)
                        securityInfo += UString("\n");
                    securityInfo += usprintf("%02X", elementHeader->KeySignature.PubKey.Modulus[i]);
                }

                // Calculate and add PubKey hash
                UINT8 hash[SHA256_DIGEST_SIZE];
                sha256(&elementHeader->KeySignature.PubKey.Modulus, sizeof(elementHeader->KeySignature.PubKey.Modulus), hash);
                securityInfo += UString("\n\nBoot Policy RSA Public Key Hash:");
                for (UINT8 i = 0; i < sizeof(hash); i++) {
                    if (i % 32 == 0)
                        securityInfo += UString("\n");
                    securityInfo += usprintf("%02X", hash[i]);
                }
                this->ffsParser->bgBpHash = UByteArray((const char*)hash, sizeof(hash));

                // Add Signature
                securityInfo += UString("\n\nBoot Policy RSA Signature:");
                for (UINT16 i = 0; i < sizeof(elementHeader->KeySignature.Signature.Signature); i++) {
                    if (i % 32 == 0)
                        securityInfo += UString("\n");
                    securityInfo += usprintf("%02X", elementHeader->KeySignature.Signature.Signature[i]);
                }
            }
            status = this->ffsParser->findNextBootGuardBootPolicyElement(bootPolicy, elementOffset + elementSize, elementOffset, elementSize);
        }

        securityInfo += UString("\n------------------------------------------------------------------------\n\n");
        this->ffsParser->bgBootPolicyFound = true;
        return U_SUCCESS;
}

USTATUS BGIBBManifestParserIcelake::ParseManifest(const QByteArray &bootPolicy, const UINT32 localOffset, const QModelIndex &parent, QString &info, UINT32 &realSize, UString &securityInfo, TreeModel* model, UByteArray bgKmHash)
{
    U_UNUSED_PARAMETER(realSize);
       if ((UINT32)bootPolicy.size() < localOffset + sizeof(BG_BOOT_POLICY_MANIFEST_HEADER2)) {
           return U_INVALID_BG_BOOT_POLICY;
       }

       const BG_BOOT_POLICY_MANIFEST_HEADER2* header = (const BG_BOOT_POLICY_MANIFEST_HEADER2*)(bootPolicy.constData() + localOffset);
       if (header->Tag != BG_BOOT_POLICY_MANIFEST_HEADER_TAG) {
           return U_INVALID_BG_BOOT_POLICY;
       }

       UINT32 bmSize = sizeof(BG_BOOT_POLICY_MANIFEST_HEADER2);
       if ((UINT32)bootPolicy.size() < localOffset + bmSize) {
           return U_INVALID_BG_BOOT_POLICY;
       }

       // Valid BPM found
       info = usprintf("LocalOffset: %08Xh, BP SVN: %02Xh, ACM SVN: %02Xh",
                       localOffset,
                       header->BPSVN,
                       header->ACMSVN
                       );

       // Add BP header info
       securityInfo += usprintf(
                                 "Intel BootGuard Boot Policy Manifest found at base %Xh\n"
                                 "Tag: __ACBP__ Version: %02Xh HeaderVersion: %02Xh\n"
                                 "PMBPMVersion: %02Xh PBSVN: %02Xh ACMSVN: %02Xh SignatureOffset: %04Xh NEMDataStack: %04Xh\n",
                                 model->base(parent) + localOffset,
                                 header->Version,
                                 header->HeaderVersion,
                                 header->PMBPMVersion,
                                 header->BPSVN,
                                 header->ACMSVN,
                                 header->RSAEntryOffset,
                                 header->NEMDataSize
                                 );

       // Iterate over elements to get them all
       UINT32 elementOffset = 0;
       UINT32 elementSize = 0;
       USTATUS status = this->ffsParser->findNextBootGuardBootPolicyElement(bootPolicy, localOffset + sizeof(BG_BOOT_POLICY_MANIFEST_HEADER2), elementOffset, elementSize);
       while (status == U_SUCCESS) {
           const UINT64* currentPos = (const UINT64*)(bootPolicy.constData() + elementOffset);
           if (*currentPos == BG_BOOT_POLICY_MANIFEST_IBB_ELEMENT_TAG)
           {
               const BG_IBB_ELEMENT2* elementHeader = (const BG_IBB_ELEMENT2*)currentPos;
               // Valid IBB element found
               securityInfo += usprintf(
                                         "\nInitial Boot Block Element found at base %Xh\n"
                                         "Tag: __IBBS__  Version: %02Xh  ProfileTimer: %02Xh  ElementSize: %04Xh\n"
                                         "Flags: %08Xh    IbbMchBar: %08Xh VtdBar: %08Xh\n"
                                         "PmrlBase: %08Xh PmrlLimit: %08Xh  EntryPoint: %08Xh",
                                         model->base(parent) + localOffset + elementOffset,
                                         elementHeader->Version,
                                         elementHeader->PolicyTimerVal,
                                         elementHeader->ElementSize,
                                         elementHeader->Flags,
                                         elementHeader->IbbMchBar,
                                         elementHeader->VtdBar,
                                         elementHeader->PmrlBase,
                                         elementHeader->PmrlLimit,
                                         elementHeader->EntryPoint
                                         );

               // Add PostIbbHash
               //post Ibb hash location not exists

               // Add Digest
                securityInfo += UString("\n\nIBB Digests:\n");
                char* currSHA = (char*)(elementHeader->SHA_HASHList);
               for (int i=0; i < elementHeader->NumOfDigests; i++)
               {
                   if (((HASH_HEADER*)currSHA)->HashAlgorithmId == 0xB)
                       securityInfo += usprintf("\n%02X- SHA256 Hash:\n", i+1);
                   else if (((HASH_HEADER*)currSHA)->HashAlgorithmId == 0xC)
                       securityInfo += usprintf("\n%02X- SHA384 Hash:\n", i+1);
                   else if (((HASH_HEADER*)currSHA)->HashAlgorithmId == 0x4)
                       securityInfo += usprintf("\n%02X- SHA1 Hash:\n", i+1);
                   else
                       securityInfo += usprintf("\n%02X- Unknown Hash:\n", i+1);

                   UINT16 currSHASize = ((HASH_HEADER*)currSHA)->Size;
                   this->ffsParser->bgBpDigest= UByteArray((const char*)(currSHA+sizeof(HASH_HEADER)), currSHASize);
                   for (UINT8 i = 0; i < (UINT8)this->ffsParser->bgBpDigest.size(); i++)
                       securityInfo += usprintf("%02X", (UINT8)this->ffsParser->bgBpDigest.at(i));

                   currSHA = currSHA + sizeof(HASH_HEADER) + currSHASize;
               }


               // Add all IBB segments
               securityInfo += UString("\n\nIBB Segments:\n");
               const BG_IBB_SEGMENT_ELEMENT* segments = (const BG_IBB_SEGMENT_ELEMENT*)((char*)elementHeader + sizeof(BG_IBB_ELEMENT2) + elementHeader->SizeOfDigests +3);
               UINT8 IbbSegCount = *((char*)segments -1);
               for (UINT8 i = 0; i < IbbSegCount; i++) {
                   securityInfo += usprintf("Flags: %04Xh Address: %08Xh Size: %08Xh\n",
                                             segments[i].Flags, segments[i].Base, segments[i].Size);
                   if (segments[i].Flags == BG_IBB_SEGMENT_FLAG_IBB) {
                       BG_PROTECTED_RANGE range;
                       range.Offset = segments[i].Base;
                       range.Size = segments[i].Size;
                       range.Type = BG_PROTECTED_RANGE_INTEL_BOOT_GUARD_IBB;
                       this->ffsParser->bgProtectedRanges.push_back(range);
                   }
               }
           }
           else if (*currentPos == BG_BOOT_POLICY_MANIFEST_PLATFORM_MANUFACTURER_ELEMENT_TAG) {
               const BG_PLATFORM_MANUFACTURER_ELEMENT* elementHeader = (const BG_PLATFORM_MANUFACTURER_ELEMENT*)currentPos;
               securityInfo += usprintf(
                                         "\nPlatform Manufacturer Data Element found at base %Xh\n"
                                         "Tag: __PMDA__ Version: %02Xh DataSize: %02Xh",
                                         model->base(parent) + localOffset + elementOffset,
                                         elementHeader->Version,
                                         elementHeader->DataSize
                                         );
               // Check for Microsoft PMDA hash data
               const BG_MICROSOFT_PMDA_HEADER* pmdaHeader = (const BG_MICROSOFT_PMDA_HEADER*)(elementHeader + 1);
               if (pmdaHeader->Version == BG_MICROSOFT_PMDA_VERSION
                   && elementHeader->DataSize == sizeof(BG_MICROSOFT_PMDA_HEADER) + sizeof(BG_MICROSOFT_PMDA_ENTRY)*pmdaHeader->NumEntries) {
                   // Add entries
                   securityInfo += UString("\nMicrosoft PMDA-based protected ranges:\n");
                   const BG_MICROSOFT_PMDA_ENTRY* entries = (const BG_MICROSOFT_PMDA_ENTRY*)(pmdaHeader + 1);
                   for (UINT32 i = 0; i < pmdaHeader->NumEntries; i++) {

                       securityInfo += usprintf("Address: %08Xh Size: %08Xh\n", entries[i].Address, entries[i].Size);
                       securityInfo += UString("Hash: ");
                       for (UINT8 j = 0; j < sizeof(entries[i].Hash); j++) {
                           securityInfo += usprintf("%02X", entries[i].Hash[j]);
                       }
                       securityInfo += UString("\n");

                       BG_PROTECTED_RANGE range;
                       range.Offset = entries[i].Address;
                       range.Size = entries[i].Size;
                       range.Hash = UByteArray((const char*)entries[i].Hash, sizeof(entries[i].Hash));
                       range.Type = BG_PROTECTED_RANGE_VENDOR_HASH_MICROSOFT;
                       this->ffsParser->bgProtectedRanges.push_back(range);
                   }
               }
               else {
                   // Add raw data
                   const UINT8* data = (const UINT8*)(elementHeader + 1);
                   for (UINT16 i = 0; i < elementHeader->DataSize; i++) {
                       if (i % 32 == 0)
                           securityInfo += UString("\n");
                       securityInfo += usprintf("%02X", data[i]);
                   }
                   securityInfo += UString("\n");
               }
           }
           else if (*currentPos == BG_BOOT_POLICY_MANIFEST_SIGNATURE_ELEMENT_TAG) {
               const BG_BOOT_POLICY_MANIFEST_SIGNATURE_ELEMENT2* elementHeader = (const BG_BOOT_POLICY_MANIFEST_SIGNATURE_ELEMENT2*)currentPos;
               securityInfo += usprintf(
                                         "\nBoot Policy Signature Element found at base %Xh\n"
                                         "Tag: __PMSG__ Version: %02Xh",
                                         model->base(parent) + localOffset + elementOffset,
                                         elementHeader->Version
                                         );

               // Add PubKey
               securityInfo += usprintf("\n\nBoot Policy RSA Public Key (Exponent: %Xh):", elementHeader->KeySignature.PubKey.Exponent);
               for (UINT16 i = 0; i < sizeof(elementHeader->KeySignature.PubKey.Modulus); i++) {
                   if (i % 32 == 0)
                       securityInfo += UString("\n");
                   securityInfo += usprintf("%02X", elementHeader->KeySignature.PubKey.Modulus[i]);
               }

               // Calculate and add PubKey hash
               UINT8 hash[SHA256_DIGEST_SIZE];
               sha256(&elementHeader->KeySignature.PubKey.Modulus, sizeof(elementHeader->KeySignature.PubKey.Modulus), hash);
               securityInfo += UString("\n\nBoot Policy RSA Public Key Hash:");
               for (UINT8 i = 0; i < sizeof(hash); i++) {
                   if (i % 32 == 0)
                       securityInfo += UString("\n");
                   securityInfo += usprintf("%02X", hash[i]);
               }
               this->ffsParser->bgBpHash = UByteArray((const char*)hash, sizeof(hash));

               // Add Signature
               securityInfo += UString("\n\nBoot Policy RSA Signature:");
               for (UINT16 i = 0; i < sizeof(elementHeader->KeySignature.Signature.Signature); i++) {
                   if (i % 32 == 0)
                       securityInfo += UString("\n");
                   securityInfo += usprintf("%02X", elementHeader->KeySignature.Signature.Signature[i]);
               }
           }
           status = this->ffsParser->findNextBootGuardBootPolicyElement(bootPolicy, elementOffset + elementSize, elementOffset, elementSize);
       }

       securityInfo += UString("\n------------------------------------------------------------------------\n\n");
       this->ffsParser->bgBootPolicyFound = true;
       return U_SUCCESS;
}

