/* fitparser.cpp
 
 Copyright (c) 2022, Nikolaj Schlej. All rights reserved.
 This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 */
#include "fitparser.h"

#ifdef U_ENABLE_FIT_PARSING_SUPPORT

#include "intel_fit.h"
#include "ffs.h"
#include "parsingdata.h"
#include "types.h"
#include "utility.h"
#include "digest/sha2.h"

#include <sstream>
#include "kaitai/kaitaistream.h"
#include "generated/intel_acbp_v1.h"
#include "generated/intel_acbp_v2.h"
#include "generated/intel_keym_v1.h"
#include "generated/intel_keym_v2.h"
#include "generated/intel_acm.h"

// TODO: put into separate H/CPP when we start using Kaitai for other parsers
// TODO: this implementation is certainly not a valid replacement to std::stringstream
// TODO: because it only supports getting through the buffer once
// TODO: however, we already do it this way, so it's enough for practical purposes of this file
class membuf : public std::streambuf {
public:
    membuf(const char *p, size_t l) {
        setg((char*)p, (char*)p, (char*)p + l);
    }

    pos_type seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode which = std::ios_base::in) override
    {
        (void)which;
        if (dir == std::ios_base::cur)
            gbump((int)off);
        else if (dir == std::ios_base::end)
            setg(eback(), egptr() + off, egptr());
        else if (dir == std::ios_base::beg)
            setg(eback(), eback() + off, egptr());
        return gptr() - eback();
    }

    pos_type seekpos(pos_type sp, std::ios_base::openmode which) override
    {
        return seekoff(sp - pos_type(off_type(0)), std::ios_base::beg, which);
    }
};

class memstream : public std::istream {
public:
  memstream(const char *p, size_t l) : std::istream(&buffer_),
    buffer_(p, l) {
    rdbuf(&buffer_);
  }

private:
  membuf buffer_;
};

USTATUS FitParser::parseFit(const UModelIndex & index)
{
    // Reset parser state
    fitTable.clear();
    securityInfo = "";
    bgAcmFound = false;
    bgKeyManifestFound = false;
    bgBootPolicyFound = false;
    bgKmHash = UByteArray();
    bgBpHashSha256 = UByteArray();
    bgBpHashSha384 = UByteArray();
    
    // Check sanity
    if (!index.isValid()) {
        return U_INVALID_PARAMETER;
    }
    
    // Search for FIT
    UModelIndex fitIndex;
    UINT32 fitOffset;
    findFitRecursive(index, fitIndex, fitOffset);
    
    // FIT not found
    if (!fitIndex.isValid()) {
        // Nothing to parse further
        return U_SUCCESS;
    }
    // Explicitly set the item containing FIT as fixed
    model->setFixed(fitIndex, true);
    
    // Special case of FIT header
    UByteArray fitBody = model->body(fitIndex);
    // This is safe, as we checked the size in findFitRecursive already
    const INTEL_FIT_ENTRY* fitHeader = (const INTEL_FIT_ENTRY*)(fitBody.constData() + fitOffset);
    
    // Sanity check
    UINT32 fitSize = fitHeader->Size * sizeof(INTEL_FIT_ENTRY);
    if ((UINT32)fitBody.size() - fitOffset < fitSize) {
        msg(usprintf("%s: not enough space to contain the whole FIT table", __FUNCTION__), fitIndex);
        return U_INVALID_FIT;
    }
    
    // Check FIT checksum, if present
    if (fitHeader->ChecksumValid) {
        // Calculate FIT entry checksum
        UByteArray tempFIT = model->body(fitIndex).mid(fitOffset, fitSize);
        INTEL_FIT_ENTRY* tempFitHeader = (INTEL_FIT_ENTRY*)tempFIT.data();
        tempFitHeader->Checksum = 0;
        UINT8 calculated = calculateChecksum8((const UINT8*)tempFitHeader, fitSize);
        if (calculated != fitHeader->Checksum) {
            msg(usprintf("%s: invalid FIT table checksum %02Xh, should be %02Xh", __FUNCTION__, fitHeader->Checksum, calculated), fitIndex);
        }
    }
    
    // Check fit header type
    if (fitHeader->Type != INTEL_FIT_TYPE_HEADER) {
        msg(usprintf("%s: invalid FIT header type", __FUNCTION__), fitIndex);
        return U_INVALID_FIT;
    }
    
    // Add FIT header
    std::vector<UString> currentStrings;
    currentStrings.push_back(UString("_FIT_            "));
    currentStrings.push_back(usprintf("%08Xh", fitSize));
    currentStrings.push_back(usprintf("%04Xh", fitHeader->Version));
    currentStrings.push_back(usprintf("%02Xh", fitHeader->Checksum));
    currentStrings.push_back(fitEntryTypeToUString(fitHeader->Type));
    currentStrings.push_back(UString()); // Empty info for FIT header
    fitTable.push_back(std::pair<std::vector<UString>, UModelIndex>(currentStrings, fitIndex));
    
    // Process all other entries
    UModelIndex acmIndex;
    UModelIndex kmIndex;
    UModelIndex bpIndex;
    for (UINT32 i = 1; i < fitHeader->Size; i++) {
        currentStrings.clear();
        UString info;
        UModelIndex itemIndex;
        const INTEL_FIT_ENTRY* currentEntry = fitHeader + i;
        UINT32 currentEntrySize = currentEntry->Size;
        
        // Check sanity
        if (currentEntry->Type == INTEL_FIT_TYPE_HEADER) {
            msg(usprintf("%s: second FIT header found, the table is damaged", __FUNCTION__), fitIndex);
            return U_INVALID_FIT;
        }
        
        // Special case of version 0 entries for TXT and TPM policies
        if ((currentEntry->Type == INTEL_FIT_TYPE_TXT_POLICY || currentEntry->Type == INTEL_FIT_TYPE_TPM_POLICY)
            && currentEntry->Version == 0) {
            const INTEL_FIT_INDEX_IO_ADDRESS* policy = (const INTEL_FIT_INDEX_IO_ADDRESS*)currentEntry;
            info += usprintf("Index: %04Xh, BitPosition: %02Xh, AccessWidth: %02Xh, DataRegAddr: %04Xh, IndexRegAddr: %04Xh",
                             policy->Index,
                             policy->BitPosition,
                             policy->AccessWidthInBytes,
                             policy->DataRegisterAddress,
                             policy->IndexRegisterAddress);
        }
        else if (currentEntry->Address > ffsParser->addressDiff && currentEntry->Address < 0xFFFFFFFFUL) { // Only elements in the image need to be parsed
            UINT32 currentEntryBase = (UINT32)(currentEntry->Address - ffsParser->addressDiff);
            itemIndex = model->findByBase(currentEntryBase);
            if (itemIndex.isValid()) {
                UByteArray item = model->header(itemIndex) + model->body(itemIndex) + model->tail(itemIndex);
                UINT32 localOffset = currentEntryBase - model->base(itemIndex);
                
                switch (currentEntry->Type) {
                    case INTEL_FIT_TYPE_MICROCODE:
                        (void)parseFitEntryMicrocode(item, localOffset, itemIndex, info, currentEntrySize);
                        break;
                        
                    case INTEL_FIT_TYPE_STARTUP_AC_MODULE:
                        (void)parseFitEntryAcm(item, localOffset, itemIndex, info, currentEntrySize);
                        acmIndex = itemIndex;
                        break;
                        
                    case INTEL_FIT_TYPE_BOOT_GUARD_KEY_MANIFEST:
                        (void)parseFitEntryBootGuardKeyManifest(item, localOffset, itemIndex, info, currentEntrySize);
                        kmIndex = itemIndex;
                        break;
                        
                    case INTEL_FIT_TYPE_BOOT_GUARD_BOOT_POLICY:
                        (void)parseFitEntryBootGuardBootPolicy(item, localOffset, itemIndex, info, currentEntrySize);
                        bpIndex = itemIndex;
                        break;
                        
                    default:
                        // Do nothing
                        break;
                }
            }
            else {
                msg(usprintf("%s: FIT entry #%u not found in the image", __FUNCTION__, i), fitIndex);
            }
        }
        
        // Explicitly set the item referenced by FIT as fixed
        if (itemIndex.isValid()) {
            model->setFixed(itemIndex, true);
        }
        
        // Add entry to fitTable
        currentStrings.push_back(usprintf("%016" PRIX64 "h", currentEntry->Address));
        currentStrings.push_back(usprintf("%08Xh", currentEntrySize));
        currentStrings.push_back(usprintf("%04Xh", currentEntry->Version));
        currentStrings.push_back(usprintf("%02Xh", currentEntry->Checksum));
        currentStrings.push_back(fitEntryTypeToUString(currentEntry->Type));
        currentStrings.push_back(info);
        fitTable.push_back(std::pair<std::vector<UString>, UModelIndex>(currentStrings, itemIndex));
    }
    
    // Perform validation of BootGuard components
    if (bgAcmFound) {
        if (!bgKeyManifestFound) {
            msg(usprintf("%s: startup ACM found, but KeyManifest is not", __FUNCTION__), acmIndex);
        }
        else if (!bgBootPolicyFound) {
            msg(usprintf("%s: startup ACM and Key Manifest found, Boot Policy is not", __FUNCTION__), kmIndex);
        }
        else {
            // Check key hashes
            if (!bgKmHash.isEmpty()
                && !(bgKmHash == bgBpHashSha256 || bgKmHash == bgBpHashSha384)) {
                msg(usprintf("%s: Boot Policy key hash stored in Key Manifest differs from the hash of the public key stored in Boot Policy", __FUNCTION__), bpIndex);
                return U_SUCCESS;
            }
        }
    }

    return U_SUCCESS;
}

void FitParser::findFitRecursive(const UModelIndex & index, UModelIndex & found, UINT32 & fitOffset)
{
    // Sanity check
    if (!index.isValid()) {
        return;
    }
    
    // Process child items
    for (int i = 0; i < model->rowCount(index); i++) {
        findFitRecursive(index.model()->index(i, 0, index), found, fitOffset);
        
        if (found.isValid()) {
            // Found it, no need to process further
            return;
        }
    }
    
    // Check for all FIT signatures in item body
    UByteArray lastVtfBody = model->body(ffsParser->lastVtf);
    UINT64 fitSignatureValue = INTEL_FIT_SIGNATURE;
    UByteArray fitSignature((const char*)&fitSignatureValue, sizeof(fitSignatureValue));
    UINT32 storedFitAddress = *(const UINT32*)(lastVtfBody.constData() + lastVtfBody.size() - INTEL_FIT_POINTER_OFFSET);
    for (INT32 offset = (INT32)model->body(index).indexOf(fitSignature);
         offset >= 0;
         offset = (INT32)model->body(index).indexOf(fitSignature, offset + 1)) {
        // FIT candidate found, calculate its physical address
        UINT32 fitAddress = (UINT32)(model->base(index) + (UINT32)ffsParser->addressDiff + model->header(index).size() + (UINT32)offset);
        
        // Check FIT address to be stored in the last VTF
        if (fitAddress == storedFitAddress) {
            // Valid FIT table must have at least two entries
            if ((UINT32)model->body(index).size() < offset + 2*sizeof(INTEL_FIT_ENTRY)) {
                msg(usprintf("%s: FIT table candidate found, too small to contain real FIT", __FUNCTION__), index);
            }
            else {
                // Real FIT found
                found = index;
                fitOffset = offset;
                msg(usprintf("%s: real FIT table found at physical address %08Xh", __FUNCTION__, fitAddress), found);
                break;
            }
        }
        else if (model->rowCount(index) == 0) { // Show messages only to leaf items
            msg(usprintf("%s: FIT table candidate found, but not referenced from the last VTF", __FUNCTION__), index);
        }
    }
}

USTATUS FitParser::parseFitEntryMicrocode(const UByteArray & microcode, const UINT32 localOffset, const UModelIndex & parent, UString & info, UINT32 &realSize)
{
    U_UNUSED_PARAMETER(parent);
    if ((UINT32)microcode.size() - localOffset < sizeof(INTEL_MICROCODE_HEADER)) {
        return U_INVALID_MICROCODE;
    }
    
    const INTEL_MICROCODE_HEADER* ucodeHeader = (const INTEL_MICROCODE_HEADER*)(microcode.constData() + localOffset);
    if (!ffsParser->microcodeHeaderValid(ucodeHeader)) {
        return U_INVALID_MICROCODE;
    }
    
    if ((UINT32)microcode.size() - localOffset < ucodeHeader->TotalSize) {
        return U_INVALID_MICROCODE;
    }
    
    // Valid microcode found
    info = usprintf("CpuSignature: %08Xh, Revision: %08Xh, Date: %02X.%02X.%04X",
                    ucodeHeader->ProcessorSignature,
                    ucodeHeader->UpdateRevision,
                    ucodeHeader->DateDay,
                    ucodeHeader->DateMonth,
                    ucodeHeader->DateYear);
    realSize = ucodeHeader->TotalSize;
    
    return U_SUCCESS;
}

USTATUS FitParser::parseFitEntryAcm(const UByteArray & acm, const UINT32 localOffset, const UModelIndex & parent, UString & info, UINT32 &realSize)
{
    try {
        memstream is(acm.constData(), acm.size());
        is.seekg(localOffset, is.beg);
        kaitai::kstream ks(&is);
        intel_acm_t parsed(&ks);
        intel_acm_t::header_t* header = parsed.header();
        
        realSize = header->module_size();
        
        // Check header version to be of a known value
        if (header->header_version() != intel_acm_t::KNOWN_HEADER_VERSION_V0_0
            && header->header_version() != intel_acm_t::KNOWN_HEADER_VERSION_V3_0) {
            msg(usprintf("%s: Intel ACM with unknown header version %08Xh found", __FUNCTION__, header->header_version()), parent);
        }
        
        // Valid ACM found
        info = usprintf("LocalOffset: %08Xh, EntryPoint: %08Xh, ACM SVN: %04Xh, Date: %02X.%02X.%04X",
                        localOffset,
                        header->entry_point(),
                        header->acm_svn(),
                        header->date_day(),
                        header->date_month(),
                        header->date_year());
        
        // Populate ACM info
        UString acmInfo;
        if (header->module_subtype() == intel_acm_t::MODULE_SUBTYPE_TXT) {
            acmInfo = "TXT ACM ";
        }
        else if(header->module_subtype() == intel_acm_t::MODULE_SUBTYPE_STARTUP) {
            acmInfo = "Startup ACM ";
        }
        else if (header->module_subtype() == intel_acm_t::MODULE_SUBTYPE_BOOT_GUARD) {
            acmInfo = "BootGuard ACM ";
        }
        else {
            acmInfo = usprintf("Unknown ACM (%04Xh)", header->module_subtype());
            msg(usprintf("%s: Intel ACM with unknown subtype %04Xh found", __FUNCTION__, header->module_subtype()), parent);
        }
        
        acmInfo += usprintf("found at base %Xh\n"
                            "ModuleType: %04Xh\n"
                            "ModuleSubtype: %04Xh\n"
                            "HeaderSize: %08Xh\n"
                            "HeaderVersion: %08Xh\n"
                            "ChipsetId: %04Xh\n"
                            "Flags: %04Xh\n"
                            "ModuleVendor: %04Xh\n"
                            "Date: %02X.%02X.%04X\n"
                            "ModuleSize: %08Xh\n"
                            "AcmSvn: %04Xh\n"
                            "SeSvn: %04Xh\n"
                            "CodeControlFlags: %08Xh\n"
                            "ErrorEntryPoint: %08Xh\n"
                            "GdtMax: %08Xh\n"
                            "GdtBase: %08Xh\n"
                            "SegmentSel: %08Xh\n"
                            "EntryPoint: %08Xh\n"
                            "KeySize: %08Xh\n"
                            "ScratchSpaceSize: %08Xh\n",
                            model->base(parent) + localOffset,
                            header->module_type(),
                            header->module_subtype(),
                            header->header_size() * (UINT32)sizeof(UINT32),
                            header->header_version(),
                            header->chipset_id(),
                            header->flags(),
                            header->module_vendor(),
                            header->date_day(), header->date_month(), header->date_year(),
                            header->module_size() * (UINT32)sizeof(UINT32),
                            header->acm_svn(),
                            header->se_svn(),
                            header->code_control_flags(),
                            header->error_entry_point(),
                            header->gdt_max(),
                            header->gdt_base(),
                            header->segment_sel(),
                            header->entry_point(),
                            header->key_size() * (UINT32)sizeof(UINT32),
                            header->scratch_space_size() * (UINT32)sizeof(UINT32));
        
        // Add RsaPublicKey
        if (header->_is_null_rsa_exponent() == false) {
            acmInfo += usprintf("ACM RSA Public Key (Exponent: %Xh):", header->rsa_exponent());
        }
        else {
            acmInfo += usprintf("ACM RSA Public Key (Exponent: %Xh):", INTEL_ACM_HARDCODED_RSA_EXPONENT);
        }
        for (UINT32 i = 0; i < header->rsa_public_key().size(); i++) {
            if (i % 32 == 0) acmInfo += "\n";
            acmInfo += usprintf("%02X", (UINT8)header->rsa_public_key().at(i));
        }
        acmInfo += "\n";
        
        // Add RsaSignature
        acmInfo += UString("ACM RSA Signature:");
        for (UINT32 i = 0; i < header->rsa_signature().size(); i++) {
            if (i % 32 == 0) acmInfo +="\n";
            acmInfo += usprintf("%02X", (UINT8)header->rsa_signature().at(i));
        }
        acmInfo += "\n";
        
        securityInfo += acmInfo + "\n";
        bgAcmFound = true;
        return U_SUCCESS;
    }
    catch (...) {
        msg(usprintf("%s: unable to parse ACM", __FUNCTION__), parent);
        return U_INVALID_ACM;
    }
}

USTATUS FitParser::parseFitEntryBootGuardKeyManifest(const UByteArray & keyManifest, const UINT32 localOffset, const UModelIndex & parent, UString & info, UINT32 &realSize)
{
    U_UNUSED_PARAMETER(realSize);
    
    // v1
    try {
        memstream is(keyManifest.constData(), keyManifest.size());
        is.seekg(localOffset, is.beg);
        kaitai::kstream ks(&is);
        intel_keym_v1_t parsed(&ks);
        
        // Valid KM found
        info = usprintf("LocalOffset: %08Xh, Version: %02Xh, KM Version: %02Xh, KM SVN: %02Xh",
                        localOffset,
                        parsed.version(),
                        parsed.km_version(),
                        parsed.km_svn());
        
        // Populate KM info
        UString kmInfo
        = usprintf("Intel BootGuard Key manifest found at base %Xh\n"
                   "Tag: '__KEYM__'\n"
                   "Version: %02Xh\n"
                   "KmVersion: %02Xh\n"
                   "KmSvn: %02Xh\n"
                   "KmId: %02Xh\n",
                   model->base(parent) + localOffset,
                   parsed.version(),
                   parsed.km_version(),
                   parsed.km_svn(),
                   parsed.km_id());
        
        // Add KM hash
        kmInfo += UString("KM Hash (") + hashTypeToUString(parsed.km_hash()->hash_algorithm_id()) + "): ";
        for (UINT16 j = 0; j < parsed.km_hash()->len_hash(); j++) {
            kmInfo += usprintf("%02X", (UINT8) parsed.km_hash()->hash().data()[j]);
        }
        kmInfo += "\n";
        
        // Add Key Signature
        const intel_keym_v1_t::key_signature_t* key_signature = parsed.key_signature();
        kmInfo += usprintf("Key Manifest Key Signature:\n"
                           "Version: %02Xh\n"
                           "KeyId: %04Xh\n"
                           "SigScheme: %04Xh\n",
                           key_signature->version(),
                           key_signature->key_id(),
                           key_signature->sig_scheme());
                           
        // Add PubKey
        kmInfo += usprintf("Key Manifest Public Key (Exponent: %Xh): ", key_signature->public_key()->exponent());
        for (UINT16 i = 0; i < (UINT16)key_signature->public_key()->modulus().length(); i++) {
            if (i % 32 == 0) kmInfo += UString("\n");
            kmInfo += usprintf("%02X", (UINT8)key_signature->public_key()->modulus().at(i));
        }
        kmInfo += "\n";
        
        // Add Signature
        kmInfo += UString("Key Manifest Signature: ");
        for (UINT16 i = 0; i < (UINT16)key_signature->signature()->signature().length(); i++) {
            if (i % 32 == 0) kmInfo += UString("\n");
            kmInfo += usprintf("%02X", (UINT8)key_signature->signature()->signature().at(i));
        }
        kmInfo += "\n";
                
        securityInfo += kmInfo + "\n";
        bgKeyManifestFound = true;
        return U_SUCCESS;
    }
    catch (...) {
        // Do nothing here, will try parsing as v2 next
    }
    
    // v2
    try {
        memstream is(keyManifest.constData(), keyManifest.size());
        is.seekg(localOffset, is.beg);
        kaitai::kstream ks(&is);
        intel_keym_v2_t parsed(&ks);
        intel_keym_v2_t::header_t* header = parsed.header();
        
        // Valid KM found
        info = usprintf("LocalOffset: %08Xh, Version: %02Xh, KM Version: %02Xh, KM SVN: %02Xh",
                        localOffset,
                        header->version(),
                        parsed.km_version(),
                        parsed.km_svn());
        
        // Populate KM info
        UString kmInfo
        = usprintf("Intel BootGuard Key manifest found at base %Xh\n"
                   "Tag: '__KEYM__'\n"
                   "Version: %02Xh\n"
                   "KmVersion: %02Xh\n"
                   "KmSvn: %02Xh\n"
                   "KmId: %02Xh\n"
                   "KeySignatureOffset: %04Xh\n"
                   "FPFHashAlgorithmId: %04Xh\n"
                   "HashCount: %04Xh\n",
                   model->base(parent) + localOffset,
                   header->version(),
                   parsed.km_version(),
                   parsed.km_svn(),
                   parsed.km_id(),
                   parsed.key_signature_offset(),
                   parsed.fpf_hash_algorithm_id(),
                   parsed.num_km_hashes());
        
        // Add KM hashes
        if (parsed.num_km_hashes() == 0) {
            kmInfo += UString("KM Hashes: N/A\n");
            msg(usprintf("%s: Key Manifest without KM hashes", __FUNCTION__), parent);
        }
        else {
            kmInfo += UString("KM Hashes:\n");
            for (UINT16 i = 0; i < parsed.num_km_hashes(); i++) {
                intel_keym_v2_t::km_hash_t* current_km_hash = parsed.km_hashes()->at(i);
                
                // Add KM hash
                kmInfo += usprintf("UsageFlags: %016" PRIX64 "h, ", current_km_hash->usage_flags()) + hashTypeToUString(current_km_hash->hash_algorithm_id()) + ": ";
                for (UINT16 j = 0; j < current_km_hash->len_hash(); j++) {
                    kmInfo += usprintf("%02X", (UINT8)current_km_hash->hash().data()[j]);
                }
                kmInfo += "\n";
                
                if (current_km_hash->usage_flags() == intel_keym_v2_t::KM_USAGE_FLAGS_BOOT_POLICY_MANIFEST) {
                    bgKmHash = UByteArray((const char*)current_km_hash->hash().data(), current_km_hash->hash().size());
                }
            }
        }
        
        // Add Key Signature
        const intel_keym_v2_t::key_signature_t* key_signature = parsed.key_signature();
        kmInfo += usprintf("Key Manifest Key Signature:\n"
                           "Version: %02Xh\n"
                           "KeyId: %04Xh\n"
                           "SigScheme: %04Xh\n",
                           key_signature->version(),
                           key_signature->key_id(),
                           key_signature->sig_scheme());
                           
        // Add PubKey
        kmInfo += usprintf("Key Manifest Public Key (Exponent: %Xh): ", key_signature->public_key()->exponent());
        for (UINT16 i = 0; i < (UINT16)key_signature->public_key()->modulus().length(); i++) {
            if (i % 32 == 0) kmInfo += UString("\n");
            kmInfo += usprintf("%02X", (UINT8)key_signature->public_key()->modulus().at(i));
        }
        kmInfo += "\n";
        
        // Add Signature
        kmInfo += UString("Key Manifest Signature: ");
        for (UINT16 i = 0; i < (UINT16)key_signature->signature()->signature().length(); i++) {
            if (i % 32 == 0) kmInfo += UString("\n");
            kmInfo += usprintf("%02X", (UINT8)key_signature->signature()->signature().at(i));
        }
        kmInfo += "\n";
        
        securityInfo += kmInfo + "\n";
        bgKeyManifestFound = true;
        return U_SUCCESS;
    }
    catch (...) {
        msg(usprintf("%s: unable to parse Key Manifest", __FUNCTION__), parent);
        return U_INVALID_BOOT_GUARD_KEY_MANIFEST;
    }
}

USTATUS FitParser::parseFitEntryBootGuardBootPolicy(const UByteArray & bootPolicy, const UINT32 localOffset, const UModelIndex & parent, UString & info, UINT32 &realSize)
{
    U_UNUSED_PARAMETER(realSize);
    
    // v1
    try {
        memstream is(bootPolicy.constData(), bootPolicy.size());
        is.seekg(localOffset, is.beg);
        kaitai::kstream ks(&is);
        intel_acbp_v1_t parsed(&ks);
        
        // Valid BPM found
        info = usprintf("LocalOffset: %08Xh, Version: %02Xh, BP SVN: %02Xh, ACM SVN: %02Xh",
                        localOffset,
                        parsed.version(),
                        parsed.bp_svn(),
                        parsed.acm_svn());
        
        UString bpInfo
        = usprintf("Intel BootGuard Boot Policy Manifest found at base %Xh\n"
                   "StructureId: '__ACBP__'\n"
                   "Version: %02Xh\n"
                   "BPMRevision: %02Xh\n"
                   "BPSVN: %02Xh\n"
                   "ACMSVN: %02Xh\n"
                   "NEMDataSize: %04Xh\n",
                   model->base(parent) + localOffset,
                   parsed.version(),
                   parsed.bpm_revision(),
                   parsed.bp_svn(),
                   parsed.acm_svn(),
                   parsed.nem_data_size());
        
        bpInfo += UString("Boot Policy Elements:\n");
        const std::vector<intel_acbp_v1_t::acbp_element_t*>* elements = parsed.elements();
        for (intel_acbp_v1_t::acbp_element_t* element : *elements) {
            const intel_acbp_v1_t::common_header_t* element_header = element->header();
            
            UINT64 structure_id = element_header->structure_id();
            const char* structure_id_bytes = (const char*)&structure_id;
            
            bpInfo += usprintf("StructureId: '%c%c%c%c%c%c%c%c'\n"
                               "Version: %02Xh\n",
                               structure_id_bytes[0],
                               structure_id_bytes[1],
                               structure_id_bytes[2],
                               structure_id_bytes[3],
                               structure_id_bytes[4],
                               structure_id_bytes[5],
                               structure_id_bytes[6],
                               structure_id_bytes[7],
                               element_header->version());
            
            // IBBS
            if (element->_is_null_ibbs_body() == false) {
                const intel_acbp_v1_t::ibbs_body_t* ibbs_body = element->ibbs_body();
                
                // Valid IBBS element found
                bpInfo += usprintf("Flags: %08Xh\n"
                                   "MchBar: %016" PRIX64 "h\n"
                                   "VtdBar: %016" PRIX64 "h\n"
                                   "DmaProtectionBase0: %08Xh\n"
                                   "DmaProtectionLimit0: %08Xh\n"
                                   "DmaProtectionBase1: %016" PRIX64 "h\n"
                                   "DmaProtectionLimit1: %016" PRIX64 "h\n"
                                   "IbbEntryPoint: %08Xh\n"
                                   "IbbSegmentsCount: %02Xh\n",
                                   ibbs_body->flags(),
                                   ibbs_body->mch_bar(),
                                   ibbs_body->vtd_bar(),
                                   ibbs_body->dma_protection_base0(),
                                   ibbs_body->dma_protection_limit0(),
                                   ibbs_body->dma_protection_base1(),
                                   ibbs_body->dma_protection_limit1(),
                                   ibbs_body->ibb_entry_point(),
                                   ibbs_body->num_ibb_segments());
                
                // Check for non-empty PostIbbHash
                if (ibbs_body->post_ibb_hash()->len_hash() == 0) {
                    bpInfo += UString("PostIBB Hash: N/A\n");
                }
                else {
                    // Add postIbbHash protected range
                    UByteArray postIbbHash(ibbs_body->post_ibb_hash()->hash().data(), ibbs_body->post_ibb_hash()->len_hash());
                    if (postIbbHash.count('\x00') != postIbbHash.size()
                        && postIbbHash.count('\xFF') != postIbbHash.size()) {
                        PROTECTED_RANGE range = {};
                        range.Type = PROTECTED_RANGE_INTEL_BOOT_GUARD_POST_IBB;
                        range.AlgorithmId = ibbs_body->post_ibb_hash()->hash_algorithm_id();
                        range.Hash = postIbbHash;
                        ffsParser->protectedRanges.push_back(range);
                    }
                    
                    // Add PostIbbHash
                    bpInfo += UString("PostIBB Hash (") + hashTypeToUString(ibbs_body->post_ibb_hash()->hash_algorithm_id()) + "): ";
                    for (UINT16 i = 0; i < ibbs_body->post_ibb_hash()->len_hash(); i++) {
                        bpInfo += usprintf("%02X", (UINT8)ibbs_body->post_ibb_hash()->hash().data()[i]);
                    }
                    bpInfo += "\n";
                }
                
                // Add IbbHash
                bpInfo += UString("IBB Hash (") + hashTypeToUString(ibbs_body->ibb_hash()->hash_algorithm_id()) + "): ";
                for (UINT16 j = 0; j < ibbs_body->ibb_hash()->len_hash(); j++) {
                    bpInfo += usprintf("%02X", (UINT8)ibbs_body->ibb_hash()->hash().data()[j]);
                }
                bpInfo += "\n";
                
                // Check for non-empty IbbSegments
                if (ibbs_body->num_ibb_segments() == 0) {
                    bpInfo += UString("IBB Segments: N/A\n");
                    msg(usprintf("%s: Boot Policy without IBB segments", __FUNCTION__), parent);
                }
                else {
                    bpInfo += UString("IBB Segments:\n");
                    for (UINT8 i = 0; i < ibbs_body->num_ibb_segments(); i++) {
                        const intel_acbp_v1_t::ibb_segment_t* current_segment = ibbs_body->ibb_segments()->at(i);
                        
                        bpInfo += usprintf("Flags: %04Xh, Address: %08Xh, Size: %08Xh\n",
                                           current_segment->flags(),
                                           current_segment->base(),
                                           current_segment->size());
                        
                        if (current_segment->flags() == intel_acbp_v1_t::IBB_SEGMENT_TYPE_IBB) {
                            PROTECTED_RANGE range = {};
                            range.Offset = current_segment->base();
                            range.Size = current_segment->size();
                            range.AlgorithmId = TCG_HASH_ALGORITHM_ID_SHA256;
                            range.Type = PROTECTED_RANGE_INTEL_BOOT_GUARD_IBB;
                            ffsParser->protectedRanges.push_back(range);
                        }
                    }
                }
            }
            // PMDA
            else if (element->_is_null_pmda_body() == false) {
                intel_acbp_v1_t::pmda_body_t* pmda_body = element->pmda_body();
                
                // Valid Microsoft PMDA element found
                bpInfo += usprintf("TotalSize: %04Xh\n"
                                   "Version: %08Xh\n"
                                   "NumEntries: %08Xh\n",
                                   pmda_body->total_size(),
                                   pmda_body->version(),
                                   pmda_body->num_entries());
                if (pmda_body->num_entries() == 0) {
                    bpInfo += UString("PMDA Entries: N/A\n");
                }
                else {
                    bpInfo += UString("PMDA Entries:\n");
                    // v1 entries
                    if (pmda_body->_is_null_entries_v1() == false) {
                        for (UINT32 i = 0; i < pmda_body->num_entries(); i++) {
                            const intel_acbp_v1_t::pmda_entry_v1_t* current_element = pmda_body->entries_v1()->at(i);
                            
                            // Add element
                            bpInfo += usprintf("Address: %08Xh, Size: %08Xh\n",
                                               current_element->base(),
                                               current_element->size());
                            
                            // Add hash
                            bpInfo += "SHA256: ";
                            for (UINT16 j = 0; j < (UINT16)current_element->hash().size(); j++) {
                                bpInfo += usprintf("%02X", (UINT8)current_element->hash().data()[j]);
                            }
                            bpInfo += "\n";
                            
                            // Add protected range
                            PROTECTED_RANGE range = {};
                            range.Offset = current_element->base();
                            range.Size = current_element->size();
                            range.Type = PROTECTED_RANGE_VENDOR_HASH_MICROSOFT_PMDA;
                            range.AlgorithmId = TCG_HASH_ALGORITHM_ID_SHA256;
                            range.Hash = UByteArray(current_element->hash().data(), current_element->hash().size());
                            ffsParser->protectedRanges.push_back(range);
                        }
                    }
                    // v2 entries
                    else if (pmda_body->_is_null_entries_v2() == false) {
                        for (UINT32 i = 0; i < pmda_body->num_entries(); i++) {
                            const intel_acbp_v1_t::pmda_entry_v2_t* current_element = pmda_body->entries_v2()->at(i);
                            
                            // Add element
                            bpInfo += usprintf("Address: %08Xh, Size: %08Xh\n",
                                               current_element->base(),
                                               current_element->size());
                            
                            // Add hash
                            bpInfo += hashTypeToUString(current_element->hash()->hash_algorithm_id()) + ": ";
                            for (UINT16 j = 0; j < (UINT16)current_element->hash()->hash().size(); j++) {
                                bpInfo += usprintf("%02X", (UINT8)current_element->hash()->hash().data()[j]);
                            }
                            bpInfo += "\n";
                            
                            // Add protected range
                            PROTECTED_RANGE range = {};
                            range.Offset = current_element->base();
                            range.Size = current_element->size();
                            range.Type = PROTECTED_RANGE_VENDOR_HASH_MICROSOFT_PMDA;
                            range.AlgorithmId = current_element->hash()->hash_algorithm_id();
                            range.Hash = UByteArray(current_element->hash()->hash().data(), current_element->hash()->hash().size());
                            ffsParser->protectedRanges.push_back(range);
                        }
                    }
                }
            }
            // PMSG
            else if (element->_is_null_pmsg_body() == false) {
                const intel_acbp_v1_t::pmsg_body_t* key_signature = element->pmsg_body();
                bpInfo += usprintf("Boot Policy Key Signature:\n"
                                   "Version: %02Xh\n"
                                   "KeyId: %04Xh\n"
                                   "SigScheme: %04Xh\n",
                                   key_signature->version(),
                                   key_signature->key_id(),
                                   key_signature->sig_scheme());
                                   
                // Add PubKey
                bpInfo += usprintf("Boot Policy Public Key (Exponent: %Xh): ", key_signature->public_key()->exponent());
                for (UINT16 i = 0; i < (UINT16)key_signature->public_key()->modulus().length(); i++) {
                    if (i % 32 == 0) bpInfo += UString("\n");
                    bpInfo += usprintf("%02X", (UINT8)key_signature->public_key()->modulus().at(i));
                }
                bpInfo += "\n";
                
                // Calculate and add PubKey hashes
                UINT8 hash[SHA384_HASH_SIZE];
                // SHA256
                sha256(key_signature->public_key()->modulus().data(), key_signature->public_key()->modulus().length() , hash);
                bpInfo += UString("Boot Policy Public Key Hash (SHA256): ");
                for (UINT8 i = 0; i < SHA256_HASH_SIZE; i++) {
                    bpInfo += usprintf("%02X", hash[i]);
                }
                bpInfo += "\n";
                bgBpHashSha256 = UByteArray((const char*)hash, SHA256_HASH_SIZE);
                // SHA384
                sha384(key_signature->public_key()->modulus().data(), key_signature->public_key()->modulus().length() , hash);
                bpInfo += UString("Boot Policy Public Key Hash (SHA384): ");
                for (UINT8 i = 0; i < SHA384_HASH_SIZE; i++) {
                    bpInfo += usprintf("%02X", hash[i]);
                }
                bpInfo += "\n";
                bgBpHashSha384 = UByteArray((const char*)hash, SHA384_HASH_SIZE);
                
                // Add Signature
                bpInfo += UString("Boot Policy Signature: ");
                for (UINT16 i = 0; i < (UINT16)key_signature->signature()->signature().length(); i++) {
                    if (i % 32 == 0) bpInfo += UString("\n");
                    bpInfo += usprintf("%02X", (UINT8)key_signature->signature()->signature().at(i));
                }
                bpInfo += "\n";
            }
        }
        
        securityInfo += bpInfo + "\n";
        bgBootPolicyFound = true;
        return U_SUCCESS;
    }
    catch (...) {
        // Do nothing here, will try parsing as v2 next
    }
    
    // v2
    try {
        memstream is(bootPolicy.constData(), bootPolicy.size());
        is.seekg(localOffset, is.beg);
        kaitai::kstream ks(&is);
        intel_acbp_v2_t parsed(&ks); // This already verified the version to be >= 0x20
        // Valid BPM found
        info = usprintf("LocalOffset: %08Xh, Version: %02Xh, BP SVN: %02Xh, ACM SVN: %02Xh",
                        localOffset,
                        parsed.version(),
                        parsed.bp_svn(),
                        parsed.acm_svn());
        
        // Add BP header and body info
        UString bpInfo
        = usprintf("Intel BootGuard Boot Policy Manifest found at base %Xh\n"
                   "StructureId: '__ACBP__'\n"
                   "Version: %02Xh\n"
                   "HeaderSpecific: %02Xh\n"
                   "TotalSize: %04Xh\n"
                   "KeySignatureOffset: %04Xh\n"
                   "BPMRevision: %02Xh\n"
                   "BPSVN: %02Xh\n"
                   "ACMSVN: %02Xh\n"
                   "NEMDataSize: %04Xh\n",
                   model->base(parent) + localOffset,
                   parsed.version(),
                   parsed.header_specific(),
                   parsed.total_size(),
                   parsed.key_signature_offset(),
                   parsed.bpm_revision(),
                   parsed.bp_svn(),
                   parsed.acm_svn(),
                   parsed.nem_data_size());
        
        bpInfo += UString("Boot Policy Elements:\n");
        const std::vector<intel_acbp_v2_t::acbp_element_t*>* elements = parsed.elements();
        for (intel_acbp_v2_t::acbp_element_t* element : *elements) {
            const intel_acbp_v2_t::header_t* element_header = element->header();
            
            UINT64 structure_id = element_header->structure_id();
            const char* structure_id_bytes = (const char*)&structure_id;
            
            bpInfo += usprintf("StructureId: '%c%c%c%c%c%c%c%c'\n"
                               "Version: %02Xh\n"
                               "HeaderSpecific: %02Xh\n"
                               "TotalSize: %04Xh\n",
                               structure_id_bytes[0],
                               structure_id_bytes[1],
                               structure_id_bytes[2],
                               structure_id_bytes[3],
                               structure_id_bytes[4],
                               structure_id_bytes[5],
                               structure_id_bytes[6],
                               structure_id_bytes[7],
                               element_header->version(),
                               element_header->header_specific(),
                               element_header->total_size());
            
            // IBBS
            if (element->_is_null_ibbs_body() == false) {
                const intel_acbp_v2_t::ibbs_body_t* ibbs_body = element->ibbs_body();
                
                // Valid IBBS element found
                bpInfo += usprintf("SetNumber: %02Xh\n"
                                   "PBETValue: %02Xh\n"
                                   "Flags: %08Xh\n"
                                   "MchBar: %016" PRIX64 "h\n"
                                   "VtdBar: %016" PRIX64 "h\n"
                                   "DmaProtectionBase0: %08Xh\n"
                                   "DmaProtectionLimit0: %08Xh\n"
                                   "DmaProtectionBase1: %016" PRIX64 "h\n"
                                   "DmaProtectionLimit1: %016" PRIX64 "h\n"
                                   "IbbEntryPoint: %08Xh\n"
                                   "IbbDigestsSize: %02Xh\n"
                                   "IbbDigestsCount: %02Xh\n"
                                   "IbbSegmentsCount: %02Xh\n",
                                   ibbs_body->set_number(),
                                   ibbs_body->pbet_value(),
                                   ibbs_body->flags(),
                                   ibbs_body->mch_bar(),
                                   ibbs_body->vtd_bar(),
                                   ibbs_body->dma_protection_base0(),
                                   ibbs_body->dma_protection_limit0(),
                                   ibbs_body->dma_protection_base1(),
                                   ibbs_body->dma_protection_limit1(),
                                   ibbs_body->ibb_entry_point(),
                                   ibbs_body->ibb_digests_size(),
                                   ibbs_body->num_ibb_digests(),
                                   ibbs_body->num_ibb_segments());
                
                // Check for non-empty PostIbbHash
                if (ibbs_body->post_ibb_digest()->len_hash() == 0) {
                    bpInfo += UString("PostIBB Hash: N/A\n");
                }
                else {
                    // Add postIbbHash protected range
                    UByteArray postIbbHash(ibbs_body->post_ibb_digest()->hash().data(), ibbs_body->post_ibb_digest()->len_hash());
                    if (postIbbHash.count('\x00') != postIbbHash.size()
                        && postIbbHash.count('\xFF') != postIbbHash.size()) {
                        PROTECTED_RANGE range = {};
                        range.Type = PROTECTED_RANGE_INTEL_BOOT_GUARD_POST_IBB;
                        range.AlgorithmId = ibbs_body->post_ibb_digest()->hash_algorithm_id();
                        range.Hash = postIbbHash;
                        ffsParser->protectedRanges.push_back(range);
                    }
                    
                    // Add PostIbbDigest
                    bpInfo += UString("PostIBB Hash (") + hashTypeToUString(ibbs_body->post_ibb_digest()->hash_algorithm_id()) + "): ";
                    for (UINT16 i = 0; i < ibbs_body->post_ibb_digest()->len_hash(); i++) {
                        bpInfo += usprintf("%02X", (UINT8)ibbs_body->post_ibb_digest()->hash().data()[i]);
                    }
                    bpInfo += "\n";
                }
                
                // Check for non-empty ObbHash
                if (ibbs_body->obb_digest() == 0) {
                    bpInfo += UString("OBB Hash: N/A\n");
                }
                else {
                    // Add ObbHash
                    bpInfo += UString("OBB Hash (") + hashTypeToUString(ibbs_body->obb_digest()->hash_algorithm_id()) + "): ";
                    for (UINT16 i = 0; i < ibbs_body->obb_digest()->len_hash(); i++) {
                        bpInfo += usprintf("%02X", (UINT8)ibbs_body->obb_digest()->hash().data()[i]);
                    }
                    bpInfo += "\n";
                    
                    // Add ObbHash protected range
                    UByteArray obbHash(ibbs_body->obb_digest()->hash().data(), ibbs_body->obb_digest()->len_hash());
                    if (obbHash.count('\x00') != obbHash.size()
                        && obbHash.count('\xFF') != obbHash.size()) {
                        PROTECTED_RANGE range = {};
                        range.Type = PROTECTED_RANGE_INTEL_BOOT_GUARD_OBB;
                        range.AlgorithmId = ibbs_body->obb_digest()->hash_algorithm_id();
                        range.Hash = obbHash;
                        ffsParser->protectedRanges.push_back(range);
                    }
                }
                
                // Check for non-empty IbbDigests
                if (ibbs_body->num_ibb_digests() == 0) {
                    bpInfo += UString("IBB Hashes: N/A\n");
                    msg(usprintf("%s: Boot Policy without IBB digests", __FUNCTION__), parent);
                }
                else {
                    bpInfo += UString("IBB Hashes:\n");
                    for (UINT16 i = 0; i < ibbs_body->num_ibb_digests(); i++) {
                        const intel_acbp_v2_t::hash_t* current_hash = ibbs_body->ibb_digests()->at(i);
                        bpInfo += hashTypeToUString(current_hash->hash_algorithm_id()) + ": ";
                        for (UINT16 j = 0; j < current_hash->len_hash(); j++) {
                            bpInfo += usprintf("%02X", (UINT8)current_hash->hash().data()[j]);
                        }
                        bpInfo += "\n";
                    }
                }
                
                // Check for non-empty IbbSegments
                if (ibbs_body->num_ibb_segments() == 0) {
                    bpInfo += UString("IBB Segments: N/A\n");
                    msg(usprintf("%s: Boot Policy without IBB segments", __FUNCTION__), parent);
                }
                else {
                    bpInfo += UString("IBB Segments:\n");
                    for (UINT8 i = 0; i < ibbs_body->num_ibb_segments(); i++) {
                        const intel_acbp_v2_t::ibb_segment_t* current_segment = ibbs_body->ibb_segments()->at(i);
                        
                        bpInfo += usprintf("Flags: %04Xh, Address: %08Xh, Size: %08Xh\n",
                                           current_segment->flags(),
                                           current_segment->base(),
                                           current_segment->size());
                        
                        if (current_segment->flags() == intel_acbp_v2_t::IBB_SEGMENT_TYPE_IBB) {
                            PROTECTED_RANGE range = {};
                            range.Offset = current_segment->base();
                            range.Size =current_segment->size();
                            range.Type = PROTECTED_RANGE_INTEL_BOOT_GUARD_IBB;
                            range.AlgorithmId = TCG_HASH_ALGORITHM_ID_SHA256;
                            ffsParser->protectedRanges.push_back(range);
                        }
                    }
                }
            }
            // PMDA
            else if (element->_is_null_pmda_body() == false) {
                const intel_acbp_v2_t::pmda_body_t* pmda_body = element->pmda_body();
                
                // Valid Microsoft PMDA element found
                bpInfo += usprintf("TotalSize: %04Xh\n"
                                   "Version: %08Xh\n"
                                   "NumEntries: %08Xh\n",
                                   pmda_body->total_size(),
                                   pmda_body->version(),
                                   pmda_body->num_entries());
                
                if (pmda_body->num_entries() == 0) {
                    bpInfo += UString("PMDA Entries: N/A\n");
                }
                else {
                    bpInfo += UString("PMDA Entries:\n");
                    for (UINT32 i = 0; i < pmda_body->num_entries(); i++) {
                        const intel_acbp_v2_t::pmda_entry_v3_t* current_entry = pmda_body->entries()->at(i);
                        
                        UINT64 entry_id = current_entry->entry_id();
                        const char* entry_id_bytes = (const char*)&entry_id;
                        
                        // Add element
                        bpInfo += usprintf("EntryId: '%c%c%c%c', Version: %04Xh, Address: %08Xh, Size: %08Xh\n",
                                           entry_id_bytes[0],
                                           entry_id_bytes[1],
                                           entry_id_bytes[2],
                                           entry_id_bytes[3],
                                           current_entry->version(),
                                           current_entry->base(),
                                           current_entry->size());
                        
                        // Add hash
                        bpInfo += hashTypeToUString(current_entry->hash()->hash_algorithm_id()) + ": ";
                        for (UINT16 j = 0; j < current_entry->hash()->len_hash(); j++) {
                            bpInfo += usprintf("%02X", (UINT8)current_entry->hash()->hash().data()[j]);
                        }
                        bpInfo += "\n";
                        
                        // Add protected range
                        PROTECTED_RANGE range = {};
                        range.Offset = current_entry->base();
                        range.Size = current_entry->size();
                        range.Type = PROTECTED_RANGE_VENDOR_HASH_MICROSOFT_PMDA;
                        range.AlgorithmId = current_entry->hash()->hash_algorithm_id();
                        range.Hash = UByteArray(current_entry->hash()->hash().data(), current_entry->hash()->hash().size());
                        ffsParser->protectedRanges.push_back(range);
                    }
                }
            }
            bpInfo += "\n";
        }
        
        // Add Key Signature
        const intel_acbp_v2_t::key_signature_t* key_signature = parsed.key_signature();
        bpInfo += usprintf("Boot Policy Key Signature:\n"
                           "Version: %02Xh\n"
                           "KeyId: %04Xh\n"
                           "SigScheme: %04Xh\n",
                           key_signature->version(),
                           key_signature->key_id(),
                           key_signature->sig_scheme());
                           
        // Add PubKey
        bpInfo += usprintf("Boot Policy Public Key (Exponent: %Xh): ", key_signature->public_key()->exponent());
        for (UINT16 i = 0; i < (UINT16)key_signature->public_key()->modulus().length(); i++) {
            if (i % 32 == 0) bpInfo += UString("\n");
            bpInfo += usprintf("%02X", (UINT8)key_signature->public_key()->modulus().at(i));
        }
        bpInfo += "\n";
        
        // Calculate and add PubKey hashes
        UINT8 hash[SHA384_HASH_SIZE];
        // SHA256
        sha256(key_signature->public_key()->modulus().data(), key_signature->public_key()->modulus().length() , hash);
        bpInfo += UString("Boot Policy Public Key Hash (SHA256): ");
        for (UINT8 i = 0; i < SHA256_HASH_SIZE; i++) {
            bpInfo += usprintf("%02X", hash[i]);
        }
        bpInfo += "\n";
        bgBpHashSha256 = UByteArray((const char*)hash, SHA256_HASH_SIZE);
        // SHA384
        sha384(key_signature->public_key()->modulus().data(), key_signature->public_key()->modulus().length() , hash);
        bpInfo += UString("Boot Policy Public Key Hash (SHA384): ");
        for (UINT8 i = 0; i < SHA384_HASH_SIZE; i++) {
            bpInfo += usprintf("%02X", hash[i]);
        }
        bpInfo += "\n";
        bgBpHashSha384 = UByteArray((const char*)hash, SHA384_HASH_SIZE);
        
        // Add Signature
        bpInfo += UString("Boot Policy Signature: ");
        for (UINT16 i = 0; i < (UINT16)key_signature->signature()->signature().length(); i++) {
            if (i % 32 == 0) bpInfo += UString("\n");
            bpInfo += usprintf("%02X", (UINT8)key_signature->signature()->signature().at(i));
        }
        bpInfo += "\n";
        
        securityInfo += bpInfo + "\n";
        bgBootPolicyFound = true;
        return U_SUCCESS;
    }
    catch (...) {
        msg(usprintf("%s: unable to parse Boot Policy", __FUNCTION__), parent);
        return U_INVALID_BOOT_GUARD_BOOT_POLICY;
    }
}
#endif // U_ENABLE_ME_PARSING_SUPPORT
