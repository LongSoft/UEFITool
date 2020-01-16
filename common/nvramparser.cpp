/* nvramparser.cpp

Copyright (c) 2016, Nikolaj Schlej. All rights reserved.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

//TODO: relax fixed restrictions once NVRAM builder is ready

// A workaround for compilers not supporting c++11 and c11
// for using PRIX64.
#define __STDC_FORMAT_MACROS

#include <inttypes.h>
#include <map>

#include "nvramparser.h"
#include "parsingdata.h"
#include "utility.h"
#include "nvram.h"
#include "ffs.h"
#include "fit.h"

#ifdef U_ENABLE_NVRAM_PARSING_SUPPORT
USTATUS NvramParser::parseNvarStore(const UModelIndex & index)
{
    // Sanity check
    if (!index.isValid())
        return U_INVALID_PARAMETER;

    // Obtain required information from parent file
    UINT8 emptyByte = 0xFF;
    UModelIndex parentFileIndex = model->findParentOfType(index, Types::File);
    if (parentFileIndex.isValid() && model->hasEmptyParsingData(parentFileIndex) == false) {
        UByteArray data = model->parsingData(parentFileIndex);
        const FILE_PARSING_DATA* pdata = (const FILE_PARSING_DATA*)data.constData();
        emptyByte = readUnaligned(pdata).emptyByte;
    }

    // Get local offset
    UINT32 localOffset = model->header(index).size();

    // Get item data
    const UByteArray data = model->body(index);

    // Parse all entries
    UINT32 offset = 0;
    UINT32 guidsInStore = 0;
    while (1) {
        bool msgUnknownExtDataFormat = false;
        bool msgExtHeaderTooLong = false;
        bool msgExtDataTooShort = false;

        bool isInvalid = false;
        bool isInvalidLink = false;
        bool hasExtendedHeader = false;
        bool hasChecksum = false;
        bool hasTimestamp = false;
        bool hasHash = false;
        bool hasGuidIndex = false;

        UINT32 guidIndex = 0;
        UINT8  storedChecksum = 0;
        UINT8  calculatedChecksum = 0;
        UINT32 extendedHeaderSize = 0;
        UINT8  extendedAttributes = 0;
        UINT64 timestamp = 0;
        UByteArray hash;

        UINT8 subtype = Subtypes::FullNvarEntry;
        UString name;
        UString guid;
        UString text;
        UByteArray header;
        UByteArray body;
        UByteArray tail;

        UINT32 guidAreaSize = guidsInStore * sizeof(EFI_GUID);
        UINT32 unparsedSize = (UINT32)data.size() - offset - guidAreaSize;

        // Get entry header
        const NVAR_ENTRY_HEADER* entryHeader = (const NVAR_ENTRY_HEADER*)(data.constData() + offset);

        // Check header size and signature
        if (unparsedSize < sizeof(NVAR_ENTRY_HEADER) ||
            entryHeader->Signature != NVRAM_NVAR_ENTRY_SIGNATURE ||
            unparsedSize < entryHeader->Size) {
            // Check if the data left is a free space or a padding
            UByteArray padding = data.mid(offset, unparsedSize);

            // Get info
            UString info = usprintf("Full size: %Xh (%u)", padding.size(), padding.size());

            if ((UINT32)padding.count(emptyByte) == unparsedSize) { // Free space
                // Add tree item
                model->addItem(localOffset + offset, Types::FreeSpace, 0, UString("Free space"), UString(), info, UByteArray(), padding, UByteArray(), Fixed, index);
            }
            else {
                // Nothing is parsed yet, but the file is not empty 
                if (!offset) {
                    msg(usprintf("%s: file can't be parsed as NVAR variables store", __FUNCTION__), index);
                    return U_SUCCESS;
                }

                // Add tree item
                model->addItem(localOffset + offset, Types::Padding, getPaddingType(padding), UString("Padding"), UString(), info, UByteArray(), padding, UByteArray(), Fixed, index);
            }

            // Add GUID store area
            UByteArray guidArea = data.right(guidAreaSize);
            // Get info
            name = UString("GUID store");
            info = usprintf("Full size: %Xh (%u)\nGUIDs in store: %u",
                guidArea.size(), guidArea.size(),
                guidsInStore);
            // Add tree item
            model->addItem(localOffset + offset + padding.size(), Types::Padding, getPaddingType(guidArea), name, UString(), info, UByteArray(), guidArea, UByteArray(), Fixed, index);

            return U_SUCCESS;
        }

        // Contruct generic header and body
        header = data.mid(offset, sizeof(NVAR_ENTRY_HEADER));
        body = data.mid(offset + sizeof(NVAR_ENTRY_HEADER), entryHeader->Size - sizeof(NVAR_ENTRY_HEADER));

        UINT32 lastVariableFlag = emptyByte ? 0xFFFFFF : 0;

        // Set default next to predefined last value
        NVAR_ENTRY_PARSING_DATA pdata;
        pdata.emptyByte = emptyByte;
        pdata.next = lastVariableFlag;

        // Entry is marked as invalid
        if ((entryHeader->Attributes & NVRAM_NVAR_ENTRY_VALID) == 0) { // Valid attribute is not set
            isInvalid = true;
            // Do not parse further
            goto parsing_done;
        }

        // Add next node information to parsing data
        if (entryHeader->Next != lastVariableFlag) {
            subtype = Subtypes::LinkNvarEntry;
            pdata.next = entryHeader->Next;
        }

        // Entry with extended header
        if (entryHeader->Attributes & NVRAM_NVAR_ENTRY_EXT_HEADER) {
            hasExtendedHeader = true;
            msgUnknownExtDataFormat = true;

            extendedHeaderSize = readUnaligned((UINT16*)(body.constData() + body.size() - sizeof(UINT16)));
            if (extendedHeaderSize > (UINT32)body.size()) {
                msgExtHeaderTooLong = true;
                isInvalid = true;
                // Do not parse further
                goto parsing_done;
            }

            extendedAttributes = *(UINT8*)(body.constData() + body.size() - extendedHeaderSize);

            // Variable with checksum
            if (extendedAttributes & NVRAM_NVAR_ENTRY_EXT_CHECKSUM) {
                // Get stored checksum
                storedChecksum = *(UINT8*)(body.constData() + body.size() - sizeof(UINT16) - sizeof(UINT8));

                // Recalculate checksum for the variable
                calculatedChecksum = 0;
                // Include entry data
                UINT8* start = (UINT8*)(entryHeader + 1);
                for (UINT8* p = start; p < start + entryHeader->Size - sizeof(NVAR_ENTRY_HEADER); p++) {
                    calculatedChecksum += *p;
                }
                // Include entry size and flags
                start = (UINT8*)&entryHeader->Size;
                for (UINT8*p = start; p < start + sizeof(UINT16); p++) {
                    calculatedChecksum += *p;
                }
                // Include entry attributes
                calculatedChecksum += entryHeader->Attributes;

                hasChecksum = true;
                msgUnknownExtDataFormat = false;
            }

            tail = body.mid(body.size() - extendedHeaderSize);
            body = body.left(body.size() - extendedHeaderSize);

            // Entry with authenticated write (for SecureBoot)
            if (entryHeader->Attributes & NVRAM_NVAR_ENTRY_AUTH_WRITE) {
                if ((entryHeader->Attributes & NVRAM_NVAR_ENTRY_DATA_ONLY)) {// Data only auth. variables has no hash
                    if ((UINT32)tail.size() < sizeof(UINT64)) {
                        msgExtDataTooShort = true;
                        isInvalid = true;
                        // Do not parse further
                        goto parsing_done;
                    }

                    timestamp = readUnaligned(tail.constData() + sizeof(UINT8));
                    hasTimestamp = true;
                    msgUnknownExtDataFormat = false;
                }
                else { // Full or link variable have hash
                    if ((UINT32)tail.size() < sizeof(UINT64) + SHA256_HASH_SIZE) {
                        msgExtDataTooShort = true;
                        isInvalid = true;
                        // Do not parse further
                        goto parsing_done;
                    }

                    timestamp = readUnaligned((UINT64*)(tail.constData() + sizeof(UINT8)));
                    hash = tail.mid(sizeof(UINT64) + sizeof(UINT8), SHA256_HASH_SIZE);
                    hasTimestamp = true;
                    hasHash = true;
                    msgUnknownExtDataFormat = false;
                }
            }
        }

        // Entry is data-only (nameless and GUIDless entry or link)
        if (entryHeader->Attributes & NVRAM_NVAR_ENTRY_DATA_ONLY) { // Data-only attribute is set
            isInvalidLink = true;
            UModelIndex nvarIndex;
            // Search previously added entries for a link to this variable
            // WARNING: O(n^2), may be very slow
            for (int i = model->rowCount(index) - 1; i >= 0; i--) {
                nvarIndex = index.child(i, 0);
                if (model->hasEmptyParsingData(nvarIndex) == false) {
                    UByteArray nvarData = model->parsingData(nvarIndex);
                    const NVAR_ENTRY_PARSING_DATA nvarPdata = readUnaligned((const NVAR_ENTRY_PARSING_DATA*)nvarData.constData());
                    if (nvarPdata.isValid && nvarPdata.next + model->offset(nvarIndex) - localOffset == offset) { // Previous link is present and valid
                        isInvalidLink = false;
                        break;
                    }
                }
            }
            // Check if the link is valid
            if (!isInvalidLink) {
                // Use the name and text of the previous link
                name = model->name(nvarIndex);
                text = model->text(nvarIndex);

                if (entryHeader->Next == lastVariableFlag)
                    subtype = Subtypes::DataNvarEntry;
            }

            // Do not parse further
            goto parsing_done;
        }

        // Get entry name
        {
            UINT32 nameOffset = (entryHeader->Attributes & NVRAM_NVAR_ENTRY_GUID) ? sizeof(EFI_GUID) : sizeof(UINT8); // GUID can be stored with the variable or in a separate store, so there will only be an index of it
            CHAR8* namePtr = (CHAR8*)(entryHeader + 1) + nameOffset;
            UINT32 nameSize = 0;
            if (entryHeader->Attributes & NVRAM_NVAR_ENTRY_ASCII_NAME) { // Name is stored as ASCII string of CHAR8s
                text = UString(namePtr);
                nameSize = text.length() + 1;
            }
            else { // Name is stored as UCS2 string of CHAR16s
                text = UString::fromUtf16((CHAR16*)namePtr);
                nameSize = (text.length() + 1) * 2;
            }

            // Get entry GUID
            if (entryHeader->Attributes & NVRAM_NVAR_ENTRY_GUID) { // GUID is strored in the variable itself
                name = guidToUString(readUnaligned((EFI_GUID*)(entryHeader + 1)));
                guid = guidToUString(readUnaligned((EFI_GUID*)(entryHeader + 1)), false);
            }
            // GUID is stored in GUID list at the end of the store
            else {
                guidIndex = *(UINT8*)(entryHeader + 1);
                if (guidsInStore < guidIndex + 1)
                    guidsInStore = guidIndex + 1;

                // The list begins at the end of the store and goes backwards
                const EFI_GUID* guidPtr = (const EFI_GUID*)(data.constData() + data.size()) - 1 - guidIndex;
                name = guidToUString(readUnaligned(guidPtr));
                guid = guidToUString(readUnaligned(guidPtr), false);
                hasGuidIndex = true;
            }

            // Include name and GUID into the header and remove them from body
            header = data.mid(offset, sizeof(NVAR_ENTRY_HEADER) + nameOffset + nameSize);
            body = body.mid(nameOffset + nameSize);
        }
    parsing_done:
        UString info;

        // Rename invalid entries according to their types
        pdata.isValid = TRUE;
        if (isInvalid) {
            name = UString("Invalid");
            subtype = Subtypes::InvalidNvarEntry;
            pdata.isValid = FALSE;
        }
        else if (isInvalidLink) {
            name = UString("Invalid link");
            subtype = Subtypes::InvalidLinkNvarEntry;
            pdata.isValid = FALSE;
        }
        else // Add GUID info for valid entries
            info += UString("Variable GUID: ") + guid + UString("\n");

        // Add GUID index information
        if (hasGuidIndex)
            info += usprintf("GUID index: %u\n", guidIndex);

        // Add header, body and extended data info
        info += usprintf("Full size: %Xh (%u)\nHeader size: %Xh (%u)\nBody size: %Xh (%u)",
            entryHeader->Size, entryHeader->Size,
            header.size(), header.size(),
            body.size(), body.size());

        // Add attributes info
        info += usprintf("\nAttributes: %02Xh", entryHeader->Attributes);
        // Translate attributes to text
        if (entryHeader->Attributes && entryHeader->Attributes != 0xFF)
            info += UString(" (") + nvarAttributesToUString(entryHeader->Attributes) + UString(")");

        // Add next node info
        if (!isInvalid && entryHeader->Next != lastVariableFlag)
            info += usprintf("\nNext node at offset: %Xh", localOffset + offset + entryHeader->Next);

        // Add extended header info
        if (hasExtendedHeader) {
            info += usprintf("\nExtended header size: %Xh (%u)\nExtended attributes: %Xh (",
                extendedHeaderSize, extendedHeaderSize,
                extendedAttributes) + nvarExtendedAttributesToUString(extendedAttributes) + UString(")");

            // Add checksum
            if (hasChecksum)
                info += usprintf("\nChecksum: %02Xh", storedChecksum) +
                (calculatedChecksum ? usprintf(", invalid, should be %02Xh", 0x100 - calculatedChecksum) : UString(", valid"));

            // Add timestamp
            if (hasTimestamp)
                info += usprintf("\nTimestamp: %" PRIX64 "h", timestamp);

            // Add hash
            if (hasHash)
                info += UString("\nHash: ") + UString(hash.toHex().constData());
        }

        // Add tree item
        UModelIndex varIndex = model->addItem(localOffset + offset, Types::NvarEntry, subtype, name, text, info, header, body, tail, Fixed, index);

        // Set parsing data for created entry
        model->setParsingData(varIndex, UByteArray((const char*)&pdata, sizeof(pdata)));

        // Show messages
        if (msgUnknownExtDataFormat) msg(usprintf("%s: unknown extended data format", __FUNCTION__), varIndex);
        if (msgExtHeaderTooLong)     msg(usprintf("%s: extended header size (%Xh) is greater than body size (%Xh)", __FUNCTION__,
            extendedHeaderSize, body.size()), varIndex);
        if (msgExtDataTooShort)      msg(usprintf("%s: extended header size (%Xh) is too small for timestamp and hash", __FUNCTION__,
            tail.size()), varIndex);

        // Try parsing the entry data as NVAR storage if it begins with NVAR signature
        if ((subtype == Subtypes::DataNvarEntry || subtype == Subtypes::FullNvarEntry)
            && body.size() >= 4 && readUnaligned((const UINT32*)body.constData()) == NVRAM_NVAR_ENTRY_SIGNATURE)
            parseNvarStore(varIndex);

        // Move to next exntry
        offset += entryHeader->Size;
    }

    return U_SUCCESS;
}

USTATUS NvramParser::parseNvramVolumeBody(const UModelIndex & index)
{
    // Sanity check
    if (!index.isValid())
        return U_INVALID_PARAMETER;

    // Obtain required fields from parsing data
    UINT8 emptyByte = 0xFF;
    if (model->hasEmptyParsingData(index) == false) {
        UByteArray data = model->parsingData(index);
        const VOLUME_PARSING_DATA* pdata = (const VOLUME_PARSING_DATA*)data.constData();
        emptyByte = pdata->emptyByte;
    }

    // Get local offset
    UINT32 localOffset = model->header(index).size();

    // Get item data
    UByteArray data = model->body(index);

    // Search for first store
    USTATUS result;
    UINT32 prevStoreOffset;
    result = findNextStore(index, data, localOffset, 0, prevStoreOffset);
    if (result)
        return result;

    // First store is not at the beginning of volume body
    UString name;
    UString info;
    if (prevStoreOffset > 0) {
        // Get info
        UByteArray padding = data.left(prevStoreOffset);
        name = UString("Padding");
        info = usprintf("Full size: %Xh (%u)", padding.size(), padding.size());

        // Add tree item
        model->addItem(localOffset, Types::Padding, getPaddingType(padding), name, UString(), info, UByteArray(), padding, UByteArray(), Fixed, index);
    }

    // Search for and parse all stores
    UINT32 storeOffset = prevStoreOffset;
    UINT32 prevStoreSize = 0;

    while (!result) {
        // Padding between stores
        if (storeOffset > prevStoreOffset + prevStoreSize) {
            UINT32 paddingOffset = prevStoreOffset + prevStoreSize;
            UINT32 paddingSize = storeOffset - paddingOffset;
            UByteArray padding = data.mid(paddingOffset, paddingSize);

            // Get info
            name = UString("Padding");
            info = usprintf("Full size: %Xh (%u)", padding.size(), padding.size());

            // Add tree item
            model->addItem(localOffset + paddingOffset, Types::Padding, getPaddingType(padding), name, UString(), info, UByteArray(), padding, UByteArray(), Fixed, index);
        }

        // Get store size
        UINT32 storeSize = 0;
        result = getStoreSize(data, storeOffset, storeSize);
        if (result) {
            msg(usprintf("%s: getStoreSize failed with error ", __FUNCTION__) + errorCodeToUString(result), index);
            return result;
        }

        // Check that current store is fully present in input
        if (storeSize > (UINT32)data.size() || storeOffset + storeSize > (UINT32)data.size()) {
            // Mark the rest as padding and finish parsing
            UByteArray padding = data.mid(storeOffset);

            // Get info
            name = UString("Padding");
            info = usprintf("Full size: %Xh (%u)", padding.size(), padding.size());

            // Add tree item
            UModelIndex paddingIndex = model->addItem(localOffset + storeOffset, Types::Padding, getPaddingType(padding), name, UString(), info, UByteArray(), padding, UByteArray(), Fixed, index);
            msg(usprintf("%s: one of stores inside overlaps the end of data", __FUNCTION__), paddingIndex);

            // Update variables
            prevStoreOffset = storeOffset;
            prevStoreSize = padding.size();
            break;
        }

        // Parse current store header
        UModelIndex storeIndex;
        UByteArray store = data.mid(storeOffset, storeSize);
        result = parseStoreHeader(store, localOffset + storeOffset, index, storeIndex);
        if (result)
            msg(usprintf("%s: store header parsing failed with error ", __FUNCTION__) + errorCodeToUString(result), index);

        // Go to next store
        prevStoreOffset = storeOffset;
        prevStoreSize = storeSize;
        result = findNextStore(index, data, localOffset, storeOffset + prevStoreSize, storeOffset);
    }

    // Padding/free space at the end
    storeOffset = prevStoreOffset + prevStoreSize;
    if ((UINT32)data.size() > storeOffset) {
        UByteArray padding = data.mid(storeOffset);
        // Add info
        info = usprintf("Full size: %Xh (%u)", padding.size(), padding.size());

        if (padding.count(emptyByte) == padding.size()) { // Free space
            // Add tree item
            model->addItem(localOffset + storeOffset, Types::FreeSpace, 0, UString("Free space"), UString(), info, UByteArray(), padding, UByteArray(), Fixed, index);
        }
        else {
            // Nothing is parsed yet, but the file is not empty 
            if (!storeOffset) {
                msg(usprintf("%s: can't be parsed as NVRAM volume", __FUNCTION__), index);
                return U_SUCCESS;
            }

            // Add tree item
            model->addItem(localOffset + storeOffset, Types::Padding, getPaddingType(padding), UString("Padding"), UString(), info, UByteArray(), padding, UByteArray(), Fixed, index);
        }
    }

    // Parse bodies
    for (int i = 0; i < model->rowCount(index); i++) {
        UModelIndex current = index.child(i, 0);
        switch (model->type(current)) {
        case Types::FdcStore:
            parseFdcStoreBody(current);
            break;
        case Types::VssStore:
            parseVssStoreBody(current, 0);
            break;
        case Types::Vss2Store:
            parseVssStoreBody(current, 4);
            break;
        case Types::FsysStore:
            parseFsysStoreBody(current);
            break;
        case Types::EvsaStore:
            parseEvsaStoreBody(current);
            break;
        case Types::FlashMapStore:
            parseFlashMapBody(current);
            break;
        default:
            // Ignore unknown!
            break;
        }
    }

    return U_SUCCESS;
}

USTATUS NvramParser::findNextStore(const UModelIndex & index, const UByteArray & volume, const UINT32 localOffset, const UINT32 storeOffset, UINT32 & nextStoreOffset)
{
    UINT32 dataSize = volume.size();

    if (dataSize < sizeof(UINT32))
        return U_STORES_NOT_FOUND;

	// TODO: add checks for restSize
    UINT32 offset = storeOffset;
    for (; offset < dataSize - sizeof(UINT32); offset++) {
        const UINT32* currentPos = (const UINT32*)(volume.constData() + offset);
        if (*currentPos == NVRAM_VSS_STORE_SIGNATURE || *currentPos == NVRAM_APPLE_SVS_STORE_SIGNATURE || *currentPos == NVRAM_APPLE_NSS_STORE_SIGNATURE) { // $VSS, $SVS or $NSS signatures found, perform checks
            const VSS_VARIABLE_STORE_HEADER* vssHeader = (const VSS_VARIABLE_STORE_HEADER*)currentPos;
            if (vssHeader->Format != NVRAM_VSS_VARIABLE_STORE_FORMATTED) {
                msg(usprintf("%s: VSS store candidate at offset %Xh skipped, has invalid format %02Xh", __FUNCTION__, localOffset + offset, vssHeader->Format), index);
                continue;
            }
            if (vssHeader->Size == 0 || vssHeader->Size == 0xFFFFFFFF) {
                msg(usprintf("%s: VSS store candidate at offset %Xh skipped, has invalid size %Xh", __FUNCTION__, localOffset + offset, vssHeader->Size), index);
                continue;
            }
            // All checks passed, store found
            break;
        }
        else if (*currentPos == NVRAM_VSS2_AUTH_VAR_KEY_DATABASE_GUID_PART1 || *currentPos == NVRAM_VSS2_STORE_GUID_PART1) { // VSS2 store signatures found, perform checks
            UByteArray guid = UByteArray(volume.constData() + offset, sizeof(EFI_GUID));
            if (guid != NVRAM_VSS2_AUTH_VAR_KEY_DATABASE_GUID && guid != NVRAM_VSS2_STORE_GUID) // Check the whole signature
                continue;

            const VSS2_VARIABLE_STORE_HEADER* vssHeader = (const VSS2_VARIABLE_STORE_HEADER*)currentPos;
            if (vssHeader->Format != NVRAM_VSS_VARIABLE_STORE_FORMATTED) {
                msg(usprintf("%s: VSS2 store candidate at offset %Xh skipped, has invalid format %02Xh", __FUNCTION__, localOffset + offset, vssHeader->Format), index);
                continue;
            }
            if (vssHeader->Size == 0 || vssHeader->Size == 0xFFFFFFFF) {
                msg(usprintf("%s: VSS2 store candidate at offset %Xh skipped, has invalid size %Xh", __FUNCTION__, localOffset + offset, vssHeader->Size), index);
                continue;
            }
            // All checks passed, store found
            break;
        }
        else if (*currentPos == NVRAM_FDC_VOLUME_SIGNATURE) { // FDC signature found
            const FDC_VOLUME_HEADER* fdcHeader = (const FDC_VOLUME_HEADER*)currentPos;
            if (fdcHeader->Size == 0 || fdcHeader->Size == 0xFFFFFFFF) {
                msg(usprintf("%s: FDC store candidate at offset %Xh skipped, has invalid size %Xh", __FUNCTION__, localOffset + offset, fdcHeader->Size), index);
                continue;
            }
            // All checks passed, store found
            break;
        }
        else if (*currentPos == NVRAM_APPLE_FSYS_STORE_SIGNATURE || *currentPos == NVRAM_APPLE_GAID_STORE_SIGNATURE) { // Fsys or Gaid signature found
            const APPLE_FSYS_STORE_HEADER* fsysHeader = (const APPLE_FSYS_STORE_HEADER*)currentPos;
            if (fsysHeader->Size == 0 || fsysHeader->Size == 0xFFFF) {
                msg(usprintf("%s: Fsys store candidate at offset %Xh skipped, has invalid size %Xh", __FUNCTION__, localOffset + offset, fsysHeader->Size), index);
                continue;
            }
            // All checks passed, store found
            break;
        }
        else if (*currentPos == NVRAM_EVSA_STORE_SIGNATURE) { //EVSA signature found
            if (offset < sizeof(UINT32))
                continue;

            const EVSA_STORE_ENTRY* evsaHeader = (const EVSA_STORE_ENTRY*)(currentPos - 1);
            if (evsaHeader->Header.Type != NVRAM_EVSA_ENTRY_TYPE_STORE) {
                msg(usprintf("%s: EVSA store candidate at offset %Xh skipped, has invalid type %02Xh", __FUNCTION__, localOffset + offset - 4, evsaHeader->Header.Type), index);
                continue;
            }
            if (evsaHeader->StoreSize == 0 || evsaHeader->StoreSize == 0xFFFFFFFF) {
                msg(usprintf("%s: EVSA store candidate at offset %Xh skipped, has invalid size %Xh", __FUNCTION__, localOffset + offset, evsaHeader->StoreSize), index);
                continue;
            }
            // All checks passed, store found
            offset -= sizeof(UINT32);
            break;
        }
        else if (*currentPos == NVRAM_MAIN_STORE_VOLUME_GUID_DATA1 || *currentPos == EDKII_WORKING_BLOCK_SIGNATURE_GUID_DATA1) { // Possible FTW block signature found
            UByteArray guid = UByteArray(volume.constData() + offset, sizeof(EFI_GUID));
            if (guid != NVRAM_MAIN_STORE_VOLUME_GUID && guid != EDKII_WORKING_BLOCK_SIGNATURE_GUID && guid != VSS2_WORKING_BLOCK_SIGNATURE_GUID) // Check the whole signature
                continue;

            // Detect header variant based on WriteQueueSize
            const EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER32* ftwHeader = (const EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER32*)currentPos;
            if (ftwHeader->WriteQueueSize % 0x10 == 0x04) { // Header with 32 bit WriteQueueSize
                if (ftwHeader->WriteQueueSize == 0 || ftwHeader->WriteQueueSize == 0xFFFFFFFF) {
                    msg(usprintf("%s: FTW block candidate at offset %Xh skipped, has invalid body size %Xh", __FUNCTION__, localOffset + offset, ftwHeader->WriteQueueSize), index);
                    continue;
                }
            }
            else if (ftwHeader->WriteQueueSize % 0x10 == 0x00) { // Header with 64 bit WriteQueueSize
                const EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER64* ftw64Header = (const EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER64*)currentPos;
                if (ftw64Header->WriteQueueSize == 0 || ftw64Header->WriteQueueSize >= 0xFFFFFFFF) {
                    msg(usprintf("%s: FTW block candidate at offset %Xh skipped, has invalid body size %Xh", __FUNCTION__, localOffset + offset, ftw64Header->WriteQueueSize), index);
                    continue;
                }
            }
            else // Unknown header
                continue;

            // All checks passed, store found
            break;
        }
        else if (*currentPos == NVRAM_PHOENIX_FLASH_MAP_SIGNATURE_PART1) {// Phoenix SCT flash map
            UByteArray signature = UByteArray(volume.constData() + offset, NVRAM_PHOENIX_FLASH_MAP_SIGNATURE_LENGTH);
            if (signature != NVRAM_PHOENIX_FLASH_MAP_SIGNATURE) // Check the whole signature
                continue;

            // All checks passed, store found
            break;
        }
        else if (*currentPos == NVRAM_PHOENIX_CMDB_HEADER_SIGNATURE) { // Phoenix SCT CMDB store
            const PHOENIX_CMDB_HEADER* cmdbHeader = (const PHOENIX_CMDB_HEADER*)currentPos;

            // Check size
            if (cmdbHeader->HeaderSize != sizeof(PHOENIX_CMDB_HEADER))
                continue;

            // All checks passed, store found
            break;
        }
        else if (*currentPos == INTEL_MICROCODE_HEADER_VERSION_1) {// Intel microcode
            const INTEL_MICROCODE_HEADER* ucodeHeader = (const INTEL_MICROCODE_HEADER*)currentPos;
            
            // TotalSize is greater then DataSize and is multiple of 1024
            if (FALSE == ffsParser->microcodeHeaderValid(ucodeHeader)) {
                continue;
            }

            // All checks passed, store found
            break;
        }
        else if (*currentPos == OEM_ACTIVATION_PUBKEY_MAGIC) { // SLIC pubkey
            if (offset < 4 * sizeof(UINT32))
                continue;

            const OEM_ACTIVATION_PUBKEY* pubkeyHeader = (const OEM_ACTIVATION_PUBKEY*)(currentPos - 4);
            // Check type
            if (pubkeyHeader->Type != OEM_ACTIVATION_PUBKEY_TYPE)
                continue;

            // All checks passed, store found
            offset -= 4 * sizeof(UINT32);
            break;
        }
        else if (*currentPos == OEM_ACTIVATION_MARKER_WINDOWS_FLAG_PART1) { // SLIC marker
            if (offset >= dataSize - sizeof(UINT64) ||
                *(const UINT64*)currentPos != OEM_ACTIVATION_MARKER_WINDOWS_FLAG ||
                offset < 26) // Check full windows flag and structure size
                continue;

            const OEM_ACTIVATION_MARKER* markerHeader = (const OEM_ACTIVATION_MARKER*)(volume.constData() + offset - 26);
            // Check reserved bytes
            bool reservedBytesValid = true;
            for (UINT32 i = 0; i < sizeof(markerHeader->Reserved); i++)
                if (markerHeader->Reserved[i] != OEM_ACTIVATION_MARKER_RESERVED_BYTE) {
                    reservedBytesValid = false;
                    break;
                }
            if (!reservedBytesValid)
                continue;

            // All checks passed, store found
            offset -= 26;
            break;
        }
    }
    // No more stores found
    if (offset >= dataSize - sizeof(UINT32))
        return U_STORES_NOT_FOUND;

    nextStoreOffset = offset;

    return U_SUCCESS;
}

USTATUS NvramParser::getStoreSize(const UByteArray & data, const UINT32 storeOffset, UINT32 & storeSize)
{
    const UINT32* signature = (const UINT32*)(data.constData() + storeOffset);
    if (*signature == NVRAM_VSS_STORE_SIGNATURE || *signature == NVRAM_APPLE_SVS_STORE_SIGNATURE || *signature == NVRAM_APPLE_NSS_STORE_SIGNATURE) {
        const VSS_VARIABLE_STORE_HEADER* vssHeader = (const VSS_VARIABLE_STORE_HEADER*)signature;
        storeSize = vssHeader->Size;
    }
    else if (*signature == NVRAM_VSS2_AUTH_VAR_KEY_DATABASE_GUID_PART1 || *signature == NVRAM_VSS2_STORE_GUID_PART1) {
        const VSS2_VARIABLE_STORE_HEADER* vssHeader = (const VSS2_VARIABLE_STORE_HEADER*)signature;
        storeSize = vssHeader->Size;
    }
    else if (*signature == NVRAM_FDC_VOLUME_SIGNATURE) {
        const FDC_VOLUME_HEADER* fdcHeader = (const FDC_VOLUME_HEADER*)signature;
        storeSize = fdcHeader->Size;
    }
    else if (*signature == NVRAM_APPLE_FSYS_STORE_SIGNATURE || *signature == NVRAM_APPLE_GAID_STORE_SIGNATURE) {
        const APPLE_FSYS_STORE_HEADER* fsysHeader = (const APPLE_FSYS_STORE_HEADER*)signature;
        storeSize = fsysHeader->Size;
    }
    else if (*(signature + 1) == NVRAM_EVSA_STORE_SIGNATURE) {
        const EVSA_STORE_ENTRY* evsaHeader = (const EVSA_STORE_ENTRY*)signature;
        storeSize = evsaHeader->StoreSize;
    }
    else if (*signature == NVRAM_MAIN_STORE_VOLUME_GUID_DATA1 || *signature == EDKII_WORKING_BLOCK_SIGNATURE_GUID_DATA1) {
        const EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER32* ftwHeader = (const EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER32*)signature;
        if (ftwHeader->WriteQueueSize % 0x10 == 0x04) { // Header with 32 bit WriteQueueSize
            storeSize = sizeof(EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER32) + ftwHeader->WriteQueueSize;
        }
        else { //  Header with 64 bit WriteQueueSize
            const EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER64* ftw64Header = (const EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER64*)signature;
            storeSize = sizeof(EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER64) + (UINT32)ftw64Header->WriteQueueSize;
        }
    }
    else if (*signature == NVRAM_PHOENIX_FLASH_MAP_SIGNATURE_PART1) { // Phoenix SCT flash map
        const PHOENIX_FLASH_MAP_HEADER* flashMapHeader = (const PHOENIX_FLASH_MAP_HEADER*)signature;
        storeSize = sizeof(PHOENIX_FLASH_MAP_HEADER) + sizeof(PHOENIX_FLASH_MAP_ENTRY) * flashMapHeader->NumEntries;
    }
    else if (*signature == NVRAM_PHOENIX_CMDB_HEADER_SIGNATURE) { // Phoenix SCT CMDB store
        storeSize = NVRAM_PHOENIX_CMDB_SIZE; // It's a predefined max size, no need to calculate
    }
    else if (*(signature + 4) == OEM_ACTIVATION_PUBKEY_MAGIC) { // SLIC pubkey
        const OEM_ACTIVATION_PUBKEY* pubkeyHeader = (const OEM_ACTIVATION_PUBKEY*)signature;
        storeSize = pubkeyHeader->Size;
    }
    else if (*(const UINT64*)(data.constData() + storeOffset + 26) == OEM_ACTIVATION_MARKER_WINDOWS_FLAG) { // SLIC marker
        const OEM_ACTIVATION_MARKER* markerHeader = (const OEM_ACTIVATION_MARKER*)signature;
        storeSize = markerHeader->Size;
    }
    else if (*signature == INTEL_MICROCODE_HEADER_VERSION_1) { // Intel microcode, must be checked after SLIC marker because of the same *signature values
        const INTEL_MICROCODE_HEADER* ucodeHeader = (const INTEL_MICROCODE_HEADER*)signature;
        storeSize = ucodeHeader->TotalSize;
    } else {
        return U_INVALID_PARAMETER; // Unreachable
    }
    return U_SUCCESS;
}

USTATUS NvramParser::parseVssStoreHeader(const UByteArray & store, const UINT32 localOffset, const bool sizeOverride, const UModelIndex & parent, UModelIndex & index)
{
    const UINT32 dataSize = (const UINT32)store.size();

    // Check store size
    if (dataSize < sizeof(VSS_VARIABLE_STORE_HEADER)) {
        msg(usprintf("%s: volume body is too small even for VSS store header", __FUNCTION__), parent);
        return U_SUCCESS;
    }

    // Get VSS store header
    const VSS_VARIABLE_STORE_HEADER* vssStoreHeader = (const VSS_VARIABLE_STORE_HEADER*)store.constData();

    // Check for size override
    UINT32 storeSize = vssStoreHeader->Size;
    if (sizeOverride) {
        storeSize = dataSize;
    }

    // Check store size
    if (dataSize < storeSize) {
        msg(usprintf("%s: VSS store size %Xh (%u) is greater than volume body size %Xh (%u)", __FUNCTION__,
            storeSize, storeSize,
            dataSize, dataSize), parent);
        return U_SUCCESS;
    }

    // Construct header and body
    UByteArray header = store.left(sizeof(VSS_VARIABLE_STORE_HEADER));
    UByteArray body = store.mid(sizeof(VSS_VARIABLE_STORE_HEADER), storeSize - sizeof(VSS_VARIABLE_STORE_HEADER));

    // Add info
    UString name;
    if (vssStoreHeader->Signature == NVRAM_APPLE_SVS_STORE_SIGNATURE) {
        name = UString("SVS store");
    }
    else if (vssStoreHeader->Signature == NVRAM_APPLE_NSS_STORE_SIGNATURE) {
        name = UString("NSS store");
    }
    else {
        name = UString("VSS store");
    }
    
    UString info = usprintf("Signature: %Xh\nFull size: %Xh (%u)\nHeader size: %Xh (%u)\nBody size: %Xh (%u)\nFormat: %02Xh\nState: %02Xh\nUnknown: %04Xh",
        vssStoreHeader->Signature,
        storeSize, storeSize,
        header.size(), header.size(),
        body.size(), body.size(),
        vssStoreHeader->Format,
        vssStoreHeader->State,
        vssStoreHeader->Unknown);

    // Add tree item
    index = model->addItem(localOffset, Types::VssStore, 0, name, UString(), info, header, body, UByteArray(), Fixed, parent);

    return U_SUCCESS;
}

USTATUS NvramParser::parseVss2StoreHeader(const UByteArray & store, const UINT32 localOffset, const bool sizeOverride, const UModelIndex & parent, UModelIndex & index)
{
    const UINT32 dataSize = (const UINT32)store.size();

    // Check store size
    if (dataSize < sizeof(VSS2_VARIABLE_STORE_HEADER)) {
        msg(usprintf("%s: volume body is too small even for VSS2 store header", __FUNCTION__), parent);
        return U_SUCCESS;
    }

    // Get VSS2 store header
    const VSS2_VARIABLE_STORE_HEADER* vssStoreHeader = (const VSS2_VARIABLE_STORE_HEADER*)store.constData();

    // Check for size override
    UINT32 storeSize = vssStoreHeader->Size;
    if (sizeOverride) {
        storeSize = dataSize;
    }

    // Check store size
    if (dataSize < storeSize) {
        msg(usprintf("%s: VSS2 store size %Xh (%u) is greater than volume body size %Xh (%u)", __FUNCTION__,
            storeSize, storeSize,
            dataSize, dataSize), parent);
        return U_SUCCESS;
    }

    // Construct header and body
    UByteArray header = store.left(sizeof(VSS2_VARIABLE_STORE_HEADER));
    UByteArray body = store.mid(sizeof(VSS2_VARIABLE_STORE_HEADER), storeSize - sizeof(VSS2_VARIABLE_STORE_HEADER));

    // Add info
    UString name = UString("VSS2 store");
    UString info = UString("Signature: ") + guidToUString(vssStoreHeader->Signature, false) +
        usprintf("\nFull size: %Xh (%u)\nHeader size: %Xh (%u)\nBody size: %Xh (%u)\nFormat: %02Xh\nState: %02Xh\nUnknown: %04Xh",
            storeSize, storeSize,
            header.size(), header.size(),
            body.size(), body.size(),
            vssStoreHeader->Format,
            vssStoreHeader->State,
            vssStoreHeader->Unknown);

    // Add tree item
    index = model->addItem(localOffset, Types::Vss2Store, 0, name, UString(), info, header, body, UByteArray(), Fixed, parent);

    return U_SUCCESS;
}

USTATUS NvramParser::parseFtwStoreHeader(const UByteArray & store, const UINT32 localOffset, const UModelIndex & parent, UModelIndex & index)
{
    const UINT32 dataSize = (const UINT32)store.size();

    // Check store size
    if (dataSize < sizeof(EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER64)) {
        msg(usprintf("%s: volume body is too small even for FTW store header", __FUNCTION__), parent);
        return U_SUCCESS;
    }

    // Obtain required information from parent volume
    UINT8 emptyByte = 0xFF;
    UModelIndex parentVolumeIndex = model->findParentOfType(parent, Types::Volume);
    if (parentVolumeIndex.isValid() && model->hasEmptyParsingData(parentVolumeIndex) == false) {
        UByteArray data = model->parsingData(parentVolumeIndex);
        const VOLUME_PARSING_DATA* pdata = (const VOLUME_PARSING_DATA*)data.constData();
        emptyByte = pdata->emptyByte;
    }

    // Get FTW block headers
    const EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER32* ftw32BlockHeader = (const EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER32*)store.constData();
    const EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER64* ftw64BlockHeader = (const EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER64*)store.constData();

    // Check store size
    UINT32 ftwBlockSize;
    bool has32bitHeader;
    if (ftw32BlockHeader->WriteQueueSize % 0x10 == 0x04) { // Header with 32 bit WriteQueueSize
        ftwBlockSize = sizeof(EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER32) + ftw32BlockHeader->WriteQueueSize;
        has32bitHeader = true;
    }
    else { // Header with 64 bit WriteQueueSize
        ftwBlockSize = sizeof(EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER64) + (UINT32)ftw64BlockHeader->WriteQueueSize;
        has32bitHeader = false;
    }
    if (dataSize < ftwBlockSize) {
        msg(usprintf("%s: FTW store size %Xh (%u) is greater than volume body size %Xh (%u)", __FUNCTION__,
            ftwBlockSize, ftwBlockSize,
            dataSize, dataSize), parent);
        return U_SUCCESS;
    }

    // Construct header and body
    UINT32 headerSize = has32bitHeader ? sizeof(EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER32) : sizeof(EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER64);
    UByteArray header = store.left(headerSize);
    UByteArray body = store.mid(headerSize, ftwBlockSize - headerSize);

    // Check block header checksum
    UByteArray crcHeader = header;
    EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER32* crcFtwBlockHeader = (EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER32*)header.data();
    crcFtwBlockHeader->Crc = emptyByte ? 0xFFFFFFFF : 0;
    crcFtwBlockHeader->State = emptyByte ? 0xFF : 0;
    UINT32 calculatedCrc = (UINT32)crc32(0, (const UINT8*)crcFtwBlockHeader, headerSize);

    // Add info
    UString name("FTW store");
    UString info = UString("Signature: ") + guidToUString(ftw32BlockHeader->Signature, false) +
        usprintf("\nFull size: %Xh (%u)\nHeader size: %Xh (%u)\nBody size: %Xh (%u)\nState: %02Xh\nHeader CRC32: %08Xh",
            ftwBlockSize, ftwBlockSize,
            headerSize, headerSize,
            body.size(), body.size(),
            ftw32BlockHeader->State,
            ftw32BlockHeader->Crc) +
            (ftw32BlockHeader->Crc != calculatedCrc ? usprintf(", invalid, should be %08Xh", calculatedCrc) : UString(", valid"));

    // Add tree item
    index = model->addItem(localOffset, Types::FtwStore, 0, name, UString(), info, header, body, UByteArray(), Fixed, parent);

    return U_SUCCESS;
}

USTATUS NvramParser::parseFdcStoreHeader(const UByteArray & store, const UINT32 localOffset, const UModelIndex & parent, UModelIndex & index)
{
    const UINT32 dataSize = (const UINT32)store.size();

    // Check store size
    if (dataSize < sizeof(FDC_VOLUME_HEADER)) {
        msg(usprintf("%s: volume body is too small even for FDC store header", __FUNCTION__), parent);
        return U_SUCCESS;
    }

    // Get Fdc store header
    const FDC_VOLUME_HEADER* fdcStoreHeader = (const FDC_VOLUME_HEADER*)store.constData();

    // Check store size
    if (dataSize < fdcStoreHeader->Size) {
        msg(usprintf("%s: FDC store size %Xh (%u) is greater than volume body size %Xh (%u)", __FUNCTION__,
            fdcStoreHeader->Size, fdcStoreHeader->Size,
            dataSize, dataSize), parent);
        return U_SUCCESS;
    }

    // Construct header and body
    UByteArray header = store.left(sizeof(FDC_VOLUME_HEADER));
    UByteArray body = store.mid(sizeof(FDC_VOLUME_HEADER), fdcStoreHeader->Size - sizeof(FDC_VOLUME_HEADER));

    // Add info
    UString name("FDC store");
    UString info = usprintf("Signature: _FDC\nFull size: %Xh (%u)\nHeader size: %Xh (%u)\nBody size: %Xh (%u)",
        fdcStoreHeader->Size, fdcStoreHeader->Size,
        header.size(), header.size(),
        body.size(), body.size());

    // Add tree item
    index = model->addItem(localOffset, Types::FdcStore, 0, name, UString(), info, header, body, UByteArray(), Fixed, parent);

    return U_SUCCESS;
}

USTATUS NvramParser::parseFsysStoreHeader(const UByteArray & store, const UINT32 localOffset, const UModelIndex & parent, UModelIndex & index)
{
    const UINT32 dataSize = (const UINT32)store.size();

    // Check store size
    if (dataSize < sizeof(APPLE_FSYS_STORE_HEADER)) {
        msg(usprintf("%s: volume body is too small even for Fsys store header", __FUNCTION__), parent);
        return U_SUCCESS;
    }

    // Get Fsys store header
    const APPLE_FSYS_STORE_HEADER* fsysStoreHeader = (const APPLE_FSYS_STORE_HEADER*)store.constData();

    // Check store size
    if (dataSize < fsysStoreHeader->Size) {
        msg(usprintf("%s: Fsys store size %Xh (%u) is greater than volume body size %Xh (%u)", __FUNCTION__,
            fsysStoreHeader->Size, fsysStoreHeader->Size,
            dataSize, dataSize), parent);
        return U_SUCCESS;
    }

    // Construct header and body
    UByteArray header = store.left(sizeof(APPLE_FSYS_STORE_HEADER));
    UByteArray body = store.mid(sizeof(APPLE_FSYS_STORE_HEADER), fsysStoreHeader->Size - sizeof(APPLE_FSYS_STORE_HEADER) - sizeof(UINT32));

    // Check store checksum
    UINT32 storedCrc = *(UINT32*)store.right(sizeof(UINT32)).constData();
    UINT32 calculatedCrc = (UINT32)crc32(0, (const UINT8*)store.constData(), (const UINT32)store.size() - sizeof(UINT32));

    // Add info
    bool isGaidStore = (fsysStoreHeader->Signature == NVRAM_APPLE_GAID_STORE_SIGNATURE);
    UString name = isGaidStore ? UString("Gaid store") : UString("Fsys store");
    UString info = usprintf("Signature: %s\nFull size: %Xh (%u)\nHeader size: %Xh (%u)\nBody size: %Xh (%u)\nUnknown0: %02Xh\nUnknown1: %08Xh\nCRC32: %08Xh",
        isGaidStore ? "Gaid" : "Fsys",
        fsysStoreHeader->Size, fsysStoreHeader->Size,
        header.size(), header.size(),
        body.size(), body.size(),
        fsysStoreHeader->Unknown0,
        fsysStoreHeader->Unknown1)
        + (storedCrc != calculatedCrc ? usprintf(", invalid, should be %08Xh", calculatedCrc) : UString(", valid"));

    // Add tree item
    index = model->addItem(localOffset, Types::FsysStore, 0, name, UString(), info, header, body, UByteArray(), Fixed, parent);

    return U_SUCCESS;
}

USTATUS NvramParser::parseEvsaStoreHeader(const UByteArray & store, const UINT32 localOffset, const UModelIndex & parent, UModelIndex & index)
{
    const UINT32 dataSize = (const UINT32)store.size();

    // Check dataSize
    if (dataSize < sizeof(EVSA_STORE_ENTRY)) {
        msg(usprintf("%s: volume body is too small even for EVSA store header", __FUNCTION__), parent);
        return U_SUCCESS;
    }

    // Get EVSA store header
    const EVSA_STORE_ENTRY* evsaStoreHeader = (const EVSA_STORE_ENTRY*)store.constData();

    // Check store size
    if (dataSize < evsaStoreHeader->StoreSize) {
        msg(usprintf("%s: EVSA store size %Xh (%u) is greater than volume body size %Xh (%u)", __FUNCTION__,
            evsaStoreHeader->StoreSize, evsaStoreHeader->StoreSize,
            dataSize, dataSize), parent);
        return U_SUCCESS;
    }

    // Construct header and body
    UByteArray header = store.left(evsaStoreHeader->Header.Size);
    UByteArray body = store.mid(evsaStoreHeader->Header.Size, evsaStoreHeader->StoreSize - evsaStoreHeader->Header.Size);

    // Recalculate checksum
    UINT8 calculated = calculateChecksum8(((const UINT8*)evsaStoreHeader) + 2, evsaStoreHeader->Header.Size - 2);

    // Add info
    UString name("EVSA store");
    UString info = usprintf("Signature: EVSA\nFull size: %Xh (%u)\nHeader size: %Xh (%u)\nBody size: %Xh (%u)\nType: %02Xh\nAttributes: %08Xh\nChecksum: %02Xh",
        evsaStoreHeader->StoreSize, evsaStoreHeader->StoreSize,
        header.size(), header.size(),
        body.size(), body.size(),
        evsaStoreHeader->Header.Type,
        evsaStoreHeader->Attributes,
        evsaStoreHeader->Header.Checksum) +
        (evsaStoreHeader->Header.Checksum != calculated ? usprintf("%, invalid, should be %02Xh", calculated) : UString(", valid"));

    // Add tree item
    index = model->addItem(localOffset, Types::EvsaStore, 0, name, UString(), info, header, body, UByteArray(), Fixed, parent);

    return U_SUCCESS;
}

USTATUS NvramParser::parseFlashMapStoreHeader(const UByteArray & store, const UINT32 localOffset, const UModelIndex & parent, UModelIndex & index)
{
    const UINT32 dataSize = (const UINT32)store.size();

    // Check data size
    if (dataSize < sizeof(PHOENIX_FLASH_MAP_HEADER)) {
        msg(usprintf("%s: volume body is too small even for FlashMap block header", __FUNCTION__), parent);
        return U_SUCCESS;
    }

    // Get FlashMap block header
    const PHOENIX_FLASH_MAP_HEADER* flashMapHeader = (const PHOENIX_FLASH_MAP_HEADER*)store.constData();

    // Check store size
    UINT32 flashMapSize = sizeof(PHOENIX_FLASH_MAP_HEADER) + flashMapHeader->NumEntries * sizeof(PHOENIX_FLASH_MAP_ENTRY);
    if (dataSize < flashMapSize) {
        msg(usprintf("%s: FlashMap block size %Xh (%u) is greater than volume body size %Xh (%u)", __FUNCTION__,
            flashMapSize, flashMapSize,
            dataSize, dataSize), parent);
        return U_SUCCESS;
    }

    // Construct header and body
    UByteArray header = store.left(sizeof(PHOENIX_FLASH_MAP_HEADER));
    UByteArray body = store.mid(sizeof(PHOENIX_FLASH_MAP_HEADER), flashMapSize - sizeof(PHOENIX_FLASH_MAP_HEADER));

    // Add info
    UString name("Phoenix SCT flash map");
    UString info = usprintf("Signature: _FLASH_MAP\nFull size: %Xh (%u)\nHeader size: %Xh (%u)\nBody size: %Xh (%u)\nNumber of entries: %u",
        flashMapSize, flashMapSize,
        header.size(), header.size(),
        body.size(), body.size(),
        flashMapHeader->NumEntries);

    // Add tree item
    index = model->addItem(localOffset, Types::FlashMapStore, 0, name, UString(), info, header, body, UByteArray(), Fixed, parent);

    return U_SUCCESS;
}

USTATUS NvramParser::parseCmdbStoreHeader(const UByteArray & store, const UINT32 localOffset, const UModelIndex & parent, UModelIndex & index)
{
    const UINT32 dataSize = (const UINT32)store.size();

    // Check store size
    if (dataSize < sizeof(PHOENIX_CMDB_HEADER)) {
        msg(usprintf("%s: volume body is too small even for CMDB store header", __FUNCTION__), parent);
        return U_SUCCESS;
    }

    UINT32 cmdbSize = NVRAM_PHOENIX_CMDB_SIZE;
    if (dataSize < cmdbSize) {
        msg(usprintf("%s: CMDB store size %Xh (%u) is greater than volume body size %Xh (%u)", __FUNCTION__,
            cmdbSize, cmdbSize,
            dataSize, dataSize), parent);
        return U_SUCCESS;
    }

    // Get store header
    const PHOENIX_CMDB_HEADER* cmdbHeader = (const PHOENIX_CMDB_HEADER*)store.constData();

    // Construct header and body
    UByteArray header = store.left(cmdbHeader->TotalSize);
    UByteArray body = store.mid(cmdbHeader->TotalSize, cmdbSize - cmdbHeader->TotalSize);

    // Add info
    UString name("CMDB store");
    UString info = usprintf("Signature: CMDB\nFull size: %Xh (%u)\nHeader size: %Xh (%u)\nBody size: %Xh (%u)",
        cmdbSize, cmdbSize,
        header.size(), header.size(),
        body.size(), body.size());

    // Add tree item
    index = model->addItem(localOffset, Types::CmdbStore, 0, name, UString(), info, header, body, UByteArray(), Fixed, parent);

    return U_SUCCESS;
}

USTATUS NvramParser::parseSlicPubkeyHeader(const UByteArray & store, const UINT32 localOffset, const UModelIndex & parent, UModelIndex & index)
{
    const UINT32 dataSize = (const UINT32)store.size();

    // Check data size
    if (dataSize < sizeof(OEM_ACTIVATION_PUBKEY)) {
        msg(usprintf("%s: volume body is too small even for SLIC pubkey header", __FUNCTION__), parent);
        return U_SUCCESS;
    }

    // Get SLIC pubkey header
    const OEM_ACTIVATION_PUBKEY* pubkeyHeader = (const OEM_ACTIVATION_PUBKEY*)store.constData();

    // Check store size
    if (dataSize < pubkeyHeader->Size) {
        msg(usprintf("%s: SLIC pubkey size %Xh (%u) is greater than volume body size %Xh (%u)", __FUNCTION__,
            pubkeyHeader->Size, pubkeyHeader->Size,
            dataSize, dataSize), parent);
        return U_SUCCESS;
    }

    // Construct header and body
    UByteArray header = store.left(sizeof(OEM_ACTIVATION_PUBKEY));

    // Add info
    UString name("SLIC pubkey");
    UString info = usprintf("Type: 0h\nFull size: %Xh (%u)\nHeader size: %Xh (%u)\nBody size: 0h (0)\n"
        "Key type: %02Xh\nVersion: %02Xh\nAlgorithm: %08Xh\nMagic: RSA1\nBit length: %08Xh\nExponent: %08Xh",
        pubkeyHeader->Size, pubkeyHeader->Size,
        header.size(), header.size(),
        pubkeyHeader->KeyType,
        pubkeyHeader->Version,
        pubkeyHeader->Algorithm,
        pubkeyHeader->BitLength,
        pubkeyHeader->Exponent);

    // Add tree item
    index = model->addItem(localOffset, Types::SlicData, Subtypes::PubkeySlicData, name, UString(), info, header, UByteArray(), UByteArray(), Fixed, parent);

    return U_SUCCESS;
}

USTATUS NvramParser::parseSlicMarkerHeader(const UByteArray & store, const UINT32 localOffset, const UModelIndex & parent, UModelIndex & index)
{
    const UINT32 dataSize = (const UINT32)store.size();

    // Check data size
    if (dataSize < sizeof(OEM_ACTIVATION_MARKER)) {
        msg(usprintf("%s: volume body is too small even for SLIC marker header", __FUNCTION__), parent);
        return U_SUCCESS;
    }

    // Get SLIC marker header
    const OEM_ACTIVATION_MARKER* markerHeader = (const OEM_ACTIVATION_MARKER*)store.constData();

    // Check store size
    if (dataSize < markerHeader->Size) {
        msg(usprintf("%s: SLIC marker size %Xh (%u) is greater than volume body size %Xh (%u)", __FUNCTION__,
            markerHeader->Size, markerHeader->Size,
            dataSize, dataSize), parent);
        return U_SUCCESS;
    }

    // Construct header and body
    UByteArray header = store.left(sizeof(OEM_ACTIVATION_MARKER));

    // Add info
    UString name("SLIC marker");
    UString info = usprintf("Type: 1h\nFull size: %Xh (%u)\nHeader size: %Xh (%u)\nBody size: 0h (0)\n"
        "Version: %08Xh\nOEM ID: %s\nOEM table ID: %s\nWindows flag: WINDOWS\nSLIC version: %08Xh",
        markerHeader->Size, markerHeader->Size,
        header.size(), header.size(),
        markerHeader->Version,
        (const char*)UString((const char*)&(markerHeader->OemId)).left(6).toLocal8Bit(),
        (const char*)UString((const char*)&(markerHeader->OemTableId)).left(8).toLocal8Bit(),
        markerHeader->SlicVersion);


    // Add tree item
    index = model->addItem(localOffset, Types::SlicData, Subtypes::MarkerSlicData, name, UString(), info, header, UByteArray(), UByteArray(), Fixed, parent);

    return U_SUCCESS;
}

USTATUS NvramParser::parseStoreHeader(const UByteArray & store, const UINT32 localOffset, const UModelIndex & parent, UModelIndex & index)
{
    const UINT32 dataSize = (const UINT32)store.size();
    const UINT32* signature = (const UINT32*)store.constData();
    // Check store size
    if (dataSize < sizeof(UINT32)) {
        msg(usprintf("%s: volume body is too small even for a store signature", __FUNCTION__), parent);
        return U_SUCCESS;
    }

    // Check signature and run parser function needed
    // VSS/SVS/NSS store
    if (*signature == NVRAM_VSS_STORE_SIGNATURE || *signature == NVRAM_APPLE_SVS_STORE_SIGNATURE || *signature == NVRAM_APPLE_NSS_STORE_SIGNATURE)
        return parseVssStoreHeader(store, localOffset, false, parent, index);
    // VSS2 store
    if (*signature == NVRAM_VSS2_AUTH_VAR_KEY_DATABASE_GUID_PART1 || *signature == NVRAM_VSS2_STORE_GUID_PART1)
        return parseVss2StoreHeader(store, localOffset, false, parent, index);
    // FTW store
    else if (*signature == NVRAM_MAIN_STORE_VOLUME_GUID_DATA1 || *signature == EDKII_WORKING_BLOCK_SIGNATURE_GUID_DATA1)
        return parseFtwStoreHeader(store, localOffset, parent, index);
    // FDC store
    else if (*signature == NVRAM_FDC_VOLUME_SIGNATURE)
        return parseFdcStoreHeader(store, localOffset, parent, index);
    // Apple Fsys/Gaid store
    else if (*signature == NVRAM_APPLE_FSYS_STORE_SIGNATURE || *signature == NVRAM_APPLE_GAID_STORE_SIGNATURE)
        return parseFsysStoreHeader(store, localOffset, parent, index);
    // EVSA store
    else if (dataSize >= 2 * sizeof(UINT32) && *(signature + 1) == NVRAM_EVSA_STORE_SIGNATURE)
        return parseEvsaStoreHeader(store, localOffset, parent, index);
    // Phoenix SCT flash map
    else if (*signature == NVRAM_PHOENIX_FLASH_MAP_SIGNATURE_PART1)
        return parseFlashMapStoreHeader(store, localOffset, parent, index);
    // Phoenix CMDB store
    else if (*signature == NVRAM_PHOENIX_CMDB_HEADER_SIGNATURE)
        return parseCmdbStoreHeader(store, localOffset, parent, index);
    // SLIC pubkey
    else if (dataSize >= 5 * sizeof(UINT32) && *(signature + 4) == OEM_ACTIVATION_PUBKEY_MAGIC)
        return parseSlicPubkeyHeader(store, localOffset, parent, index);
    // SLIC marker
    else if (dataSize >= 34 && *(const UINT64*)(store.constData() + 26) == OEM_ACTIVATION_MARKER_WINDOWS_FLAG)
        return parseSlicMarkerHeader(store, localOffset, parent, index);
    // Intel microcode
    // Must be checked after SLIC marker because of the same *signature values
    else if (*signature == INTEL_MICROCODE_HEADER_VERSION_1)
        return ffsParser->parseIntelMicrocodeHeader(store, localOffset, parent, index);

    msg(usprintf("parseStoreHeader: don't know how to parse a header with signature %08Xh", *signature), parent);
    return U_SUCCESS;
}

USTATUS NvramParser::parseFdcStoreBody(const UModelIndex & index)
{
    // Sanity check
    if (!index.isValid())
        return U_INVALID_PARAMETER;

    // Get item data
    const UByteArray data = model->body(index);

    // Get local offset
    UINT32 localOffset = model->header(index).size();

    // The body is a firmware volume with either a VSS or VSS2 store
    UModelIndex volumeIndex;
    USTATUS status = ffsParser->parseVolumeHeader(data, localOffset, index, volumeIndex);
    if (status || !volumeIndex.isValid()) {
        msg(usprintf("%s: store can't be parsed as FDC store", __FUNCTION__), index);
        return U_SUCCESS;
    }

    // Determine if it's a VSS or VSS2 store inside
    UByteArray store = model->body(volumeIndex);
    if ((UINT32)store.size() >= sizeof(UINT32) && *(const UINT32*)store.constData() == NVRAM_VSS_STORE_SIGNATURE) {
        UModelIndex vssIndex;
        status = parseVssStoreHeader(store, localOffset + model->header(volumeIndex).size(), true, volumeIndex, vssIndex);
        if (status)
            return status;
        return parseVssStoreBody(vssIndex, 0);
    }
    else if ((UINT32)store.size() >= sizeof(EFI_GUID) && store.left(sizeof(EFI_GUID)) == NVRAM_FDC_STORE_GUID) {
        UModelIndex vss2Index;
        status = parseVss2StoreHeader(store, localOffset + model->header(volumeIndex).size(), true, volumeIndex, vss2Index);
        if (status)
            return status;
        return parseVssStoreBody(vss2Index, 0);
    }
    else {
        msg(usprintf("%s: internal volume can't be parsed as VSS/VSS2 store", __FUNCTION__), index);
        return U_SUCCESS;
    }

}

USTATUS NvramParser::parseVssStoreBody(const UModelIndex & index, UINT8 alignment)
{
    // Sanity check
    if (!index.isValid())
        return U_INVALID_PARAMETER;

    // Obtain required information from parent volume
    UINT8 emptyByte = 0xFF;
    UModelIndex parentVolumeIndex = model->findParentOfType(index, Types::Volume);
    if (parentVolumeIndex.isValid() && model->hasEmptyParsingData(parentVolumeIndex) == false) {
        UByteArray data = model->parsingData(parentVolumeIndex);
        const VOLUME_PARSING_DATA* pdata = (const VOLUME_PARSING_DATA*)data.constData();
        emptyByte = pdata->emptyByte;
    }

    // Get local offset
    UINT32 localOffset = model->header(index).size();

    // Get item data
    const UByteArray data = model->body(index);

    // Check that the is enough space for variable header
    const UINT32 dataSize = (UINT32)data.size();
    if (dataSize < sizeof(VSS_VARIABLE_HEADER)) {
        msg(usprintf("%s: store body is too small even for VSS variable header", __FUNCTION__), index);
        return U_SUCCESS;
    }

    UINT32 offset = 0;

    // Parse all variables
    while (1) {
        bool isInvalid = true;
        bool isAuthenticated = false;
        bool isAppleCrc32 = false;
        bool isIntelSpecial = false;

        UINT32 storedCrc32 = 0;
        UINT32 calculatedCrc32 = 0;
        UINT64 monotonicCounter = 0;
        EFI_TIME timestamp = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
        UINT32 pubKeyIndex = 0;

        UINT8 subtype = 0;
        UString name;
        UString text;
        EFI_GUID* variableGuid = NULL;
        CHAR16*   variableName = (CHAR16*)L"";
        UByteArray header;
        UByteArray body;

        UINT32 unparsedSize = dataSize - offset;

        // Get variable header
        const VSS_VARIABLE_HEADER* variableHeader = (const VSS_VARIABLE_HEADER*)(data.constData() + offset);

        // Check variable header to fit in still unparsed data
        UINT32 variableSize = 0;
        if (unparsedSize >= sizeof(VSS_VARIABLE_HEADER)
            && variableHeader->StartId == NVRAM_VSS_VARIABLE_START_ID) {
            // Apple VSS variable with CRC32 of the data  
            if (variableHeader->Attributes & NVRAM_VSS_VARIABLE_APPLE_DATA_CHECKSUM) {
                isAppleCrc32 = true;
                if (unparsedSize < sizeof(VSS_APPLE_VARIABLE_HEADER)) {
                    variableSize = 0;
                }
                else {
                    const VSS_APPLE_VARIABLE_HEADER* appleVariableHeader = (const VSS_APPLE_VARIABLE_HEADER*)variableHeader;
                    variableSize = sizeof(VSS_APPLE_VARIABLE_HEADER) + appleVariableHeader->NameSize + appleVariableHeader->DataSize;
                    variableGuid = (EFI_GUID*)&appleVariableHeader->VendorGuid;
                    variableName = (CHAR16*)(appleVariableHeader + 1);

                    header = data.mid(offset, sizeof(VSS_APPLE_VARIABLE_HEADER) + appleVariableHeader->NameSize);
                    body = data.mid(offset + header.size(), appleVariableHeader->DataSize);

                    // Calculate CRC32 of the variable data
                    storedCrc32 = appleVariableHeader->DataCrc32;
                    calculatedCrc32 = (UINT32)crc32(0, (const UINT8*)body.constData(), body.size());
                }
            }

            // Authenticated variable
            else if ((variableHeader->Attributes & NVRAM_VSS_VARIABLE_AUTHENTICATED_WRITE_ACCESS)
                || (variableHeader->Attributes & NVRAM_VSS_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS)
                || (variableHeader->Attributes & NVRAM_VSS_VARIABLE_APPEND_WRITE)
                || (variableHeader->NameSize == 0 && variableHeader->DataSize == 0)) { // If both NameSize and DataSize are zeros, it's auth variable with zero montonic counter
                isAuthenticated = true;
                if (unparsedSize < sizeof(VSS_AUTH_VARIABLE_HEADER)) {
                    variableSize = 0;
                }
                else {
                    const VSS_AUTH_VARIABLE_HEADER* authVariableHeader = (const VSS_AUTH_VARIABLE_HEADER*)variableHeader;
                    variableSize = sizeof(VSS_AUTH_VARIABLE_HEADER) + authVariableHeader->NameSize + authVariableHeader->DataSize;
                    variableGuid = (EFI_GUID*)&authVariableHeader->VendorGuid;
                    variableName = (CHAR16*)(authVariableHeader + 1);

                    header = data.mid(offset, sizeof(VSS_AUTH_VARIABLE_HEADER) + authVariableHeader->NameSize);
                    body = data.mid(offset + header.size(), authVariableHeader->DataSize);

                    monotonicCounter = authVariableHeader->MonotonicCounter;
                    timestamp = authVariableHeader->Timestamp;
                    pubKeyIndex = authVariableHeader->PubKeyIndex;
                }
            }

            // Intel special variable
            else if (variableHeader->State == NVRAM_VSS_INTEL_VARIABLE_VALID
                || variableHeader->State == NVRAM_VSS_INTEL_VARIABLE_INVALID) {
                isIntelSpecial = true;
                const VSS_INTEL_VARIABLE_HEADER* intelVariableHeader = (const VSS_INTEL_VARIABLE_HEADER*)variableHeader;
                variableSize = intelVariableHeader->TotalSize;
                variableGuid = (EFI_GUID*)&intelVariableHeader->VendorGuid;
                variableName = (CHAR16*)(intelVariableHeader + 1);

                UINT32 i = 0;
                while (variableName[i] != 0) ++i;

                i = sizeof(VSS_INTEL_VARIABLE_HEADER) + 2 * (i + 1);
                i = i < variableSize ? i : variableSize;

                header = data.mid(offset, i);
                body = data.mid(offset + header.size(), variableSize - i);
            }

            // Normal VSS variable
            else {
                variableSize = sizeof(VSS_VARIABLE_HEADER) + variableHeader->NameSize + variableHeader->DataSize;
                variableGuid = (EFI_GUID*)&variableHeader->VendorGuid;
                variableName = (CHAR16*)(variableHeader + 1);

                header = data.mid(offset, sizeof(VSS_VARIABLE_HEADER) + variableHeader->NameSize);
                body = data.mid(offset + header.size(), variableHeader->DataSize);
            }

            // Check variable state
            if (variableHeader->State == NVRAM_VSS_INTEL_VARIABLE_VALID
                || variableHeader->State == NVRAM_VSS_VARIABLE_ADDED
                || variableHeader->State == NVRAM_VSS_VARIABLE_HEADER_VALID) {
                isInvalid = false;
            }

            // Check variable size
            if (variableSize > unparsedSize) {
                variableSize = 0;
            }
        }

        // Can't parse further, add the last element and break the loop
        if (!variableSize) {
            // Check if the data left is a free space or a padding
            UByteArray padding = data.mid(offset, unparsedSize);
            // Get info
            UString info = usprintf("Full size: %Xh (%u)", padding.size(), padding.size());

            if (padding.count(emptyByte) == padding.size()) { // Free space
                // Add tree item
                model->addItem(localOffset + offset, Types::FreeSpace, 0, UString("Free space"), UString(), info, UByteArray(), padding, UByteArray(), Fixed, index);
            }
            else { // Padding
                // Nothing is parsed yet, but the store is not empty 
                if (!offset) {
                    msg(usprintf("%s: store can't be parsed as VSS store", __FUNCTION__), index);
                    return U_SUCCESS;
                }

                // Add tree item
                model->addItem(localOffset + offset, Types::Padding, getPaddingType(padding), UString("Padding"), UString(), info, UByteArray(), padding, UByteArray(), Fixed, index);
            }

            return U_SUCCESS;
        }

        UString info;

        // Rename invalid variables
        if (isInvalid || !variableGuid) {
            isInvalid = true;
            name = UString("Invalid");
        }
        else { // Add GUID and text for valid variables
            name = guidToUString(readUnaligned(variableGuid));
            info += UString("Variable GUID: ") + guidToUString(readUnaligned(variableGuid), false) + UString("\n");
            text = UString::fromUtf16(variableName);
        }

        // Add info
        info += usprintf("Full size: %Xh (%u)\nHeader size %Xh (%u)\nBody size: %Xh (%u)\nState: %02Xh\nReserved: %02Xh\nAttributes: %08Xh (",
            variableSize, variableSize,
            header.size(), header.size(),
            body.size(), body.size(),
            variableHeader->State,
            variableHeader->Reserved,
            variableHeader->Attributes) + vssAttributesToUString(variableHeader->Attributes) + UString(")");

        // Set subtype and add related info
        if (isInvalid)
            subtype = Subtypes::InvalidVssEntry;
        else if (isAuthenticated) {
            subtype = Subtypes::AuthVssEntry;
            info += usprintf("\nMonotonic counter: %" PRIX64 "h\nTimestamp: ", monotonicCounter) + efiTimeToUString(timestamp)
                + usprintf("\nPubKey index: %u", pubKeyIndex);
        }
        else if (isAppleCrc32) {
            subtype = Subtypes::AppleVssEntry;
            info += usprintf("\nData checksum: %08Xh", storedCrc32) +
                (storedCrc32 != calculatedCrc32 ? usprintf(", invalid, should be %08Xh", calculatedCrc32) : UString(", valid"));
        }
        else if (isIntelSpecial) {
            subtype = Subtypes::IntelVssEntry;
        }
        else {
            subtype = Subtypes::StandardVssEntry;
        }

        // Add tree item
        model->addItem(localOffset + offset, Types::VssEntry, subtype, name, text, info, header, body, UByteArray(), Fixed, index);

        // Apply alignment, if needed
        if (alignment) {
            variableSize = ((variableSize + alignment - 1) & (~(alignment - 1)));
        }

        // Move to next variable
        offset += variableSize;
    }

    return U_SUCCESS;
}

USTATUS NvramParser::parseFsysStoreBody(const UModelIndex & index)
{
    // Sanity check
    if (!index.isValid())
        return U_INVALID_PARAMETER;

    // Get local offset
    UINT32 localOffset = model->header(index).size();

    // Get item data
    const UByteArray data = model->body(index);

    // Check that the is enough space for variable header
    const UINT32 storeDataSize = (UINT32)data.size();
    UINT32 offset = 0;

    // Parse all variables
    while (1) {
        UINT32 unparsedSize = storeDataSize - offset;
        UINT32 variableSize = 0;

        // Get nameSize and name of the variable
        UINT8 nameSize = *(UINT8*)(data.constData() + offset);
        bool valid = !(nameSize & 0x80); // Last bit is a validity bit, 0 means valid
        nameSize &= 0x7F;

        // Check sanity
        if (unparsedSize >= nameSize + sizeof(UINT8)) {
            variableSize = nameSize + sizeof(UINT8);
        }

        UByteArray name;
        if (variableSize) {
            name = data.mid(offset + sizeof(UINT8), nameSize);
            // Check for EOF variable
            if (nameSize == 3 && name[0] == 'E' && name[1] == 'O' && name[2] == 'F') {
                // There is no data afterward, add EOF variable and free space and return
                UByteArray header = data.mid(offset, sizeof(UINT8) + nameSize);
                UString info = usprintf("Full size: %Xh (%u)", header.size(), header.size());

                // Add EOF tree item
                model->addItem(localOffset + offset, Types::FsysEntry, Subtypes::NormalFsysEntry, UString("EOF"), UString(), info, header, UByteArray(), UByteArray(), Fixed, index);

                // Add free space
                offset += header.size();
                UByteArray body = data.mid(offset);
                info = usprintf("Full size: %Xh (%u)", body.size(), body.size());

                // Add free space tree item
                model->addItem(localOffset + offset, Types::FreeSpace, 0, UString("Free space"), UString(), info, UByteArray(), body, UByteArray(), Fixed, index);

                return U_SUCCESS;
            }
        }

        // Get dataSize and data of the variable
        const UINT16 dataSize = *(UINT16*)(data.constData() + offset + sizeof(UINT8) + nameSize);
        if (unparsedSize >= sizeof(UINT8) + nameSize + sizeof(UINT16) + dataSize) {
            variableSize = sizeof(UINT8) + nameSize + sizeof(UINT16) + dataSize;
        }
        else {
            // Last variable is bad, add the rest as padding and return
            UByteArray body = data.mid(offset);
            UString info = usprintf("Full size: %Xh (%u)", body.size(), body.size());

            // Add padding tree item
            model->addItem(localOffset + offset, Types::Padding, getPaddingType(body), UString("Padding"), UString(), info, UByteArray(), body, UByteArray(), Fixed, index);

            // Show message
            msg(usprintf("%s: next variable appears too big, added as padding", __FUNCTION__), index);

            return U_SUCCESS;
        }

        // Construct header and body
        UByteArray header = data.mid(offset, sizeof(UINT8) + nameSize + sizeof(UINT16));
        UByteArray body = data.mid(offset + sizeof(UINT8) + nameSize + sizeof(UINT16), dataSize);

        // Add info
        UString info = usprintf("Full size: %Xh (%u)\nHeader size %Xh (%u)\nBody size: %Xh (%u)",
            variableSize, variableSize,
            header.size(), header.size(),
            body.size(), body.size());

        // Add tree item
        model->addItem(localOffset + offset, Types::FsysEntry, valid ? Subtypes::NormalFsysEntry : Subtypes::InvalidFsysEntry, UString(name.constData()), UString(), info, header, body, UByteArray(), Fixed, index);

        // Move to next variable
        offset += variableSize;
    }

    return U_SUCCESS;
}

USTATUS NvramParser::parseEvsaStoreBody(const UModelIndex & index)
{
    // Sanity check
    if (!index.isValid())
        return U_INVALID_PARAMETER;

    // Obtain required information from parent volume
    UINT8 emptyByte = 0xFF;
    UModelIndex parentVolumeIndex = model->findParentOfType(index, Types::Volume);
    if (parentVolumeIndex.isValid() && model->hasEmptyParsingData(parentVolumeIndex) == false) {
        UByteArray data = model->parsingData(parentVolumeIndex);
        const VOLUME_PARSING_DATA* pdata = (const VOLUME_PARSING_DATA*)data.constData();
        emptyByte = pdata->emptyByte;
    }

    // Get local offset
    UINT32 localOffset = model->header(index).size();

    // Get item data
    const UByteArray data = model->body(index);

    // Check that the is enough space for entry header
    const UINT32 storeDataSize = (UINT32)data.size();
    UINT32 offset = 0;

    std::map<UINT16, EFI_GUID> guidMap;
    std::map<UINT16, UString> nameMap;

    // Parse all entries
    UINT32 unparsedSize = storeDataSize;
    while (unparsedSize) {
        UINT32 variableSize = 0;
        UString name;
        UString info;
        UByteArray header;
        UByteArray body;
        UINT8 subtype;
        UINT8 calculated;

        const EVSA_ENTRY_HEADER* entryHeader = (const EVSA_ENTRY_HEADER*)(data.constData() + offset);

        // Check entry size
        variableSize = sizeof(EVSA_ENTRY_HEADER);
        if (unparsedSize < variableSize || unparsedSize < entryHeader->Size) {
            body = data.mid(offset);
            info = usprintf("Full size: %Xh (%u)", body.size(), body.size());

            if (body.count(emptyByte) == body.size()) { // Free space
                // Add free space tree item
                model->addItem(localOffset + offset, Types::FreeSpace, 0, UString("Free space"), UString(), info, UByteArray(), body, UByteArray(), Fixed, index);
            }
            else {
                // Add padding tree item
                UModelIndex itemIndex = model->addItem(localOffset + offset, Types::Padding, getPaddingType(body), UString("Padding"), UString(), info, UByteArray(), body, UByteArray(), Fixed, index);

                // Show message
                msg(usprintf("%s: variable parsing failed, the rest of unparsed store added as padding", __FUNCTION__), itemIndex);
            }
            break;
        }
        variableSize = entryHeader->Size;

        // Recalculate entry checksum
        calculated = calculateChecksum8(((const UINT8*)entryHeader) + 2, entryHeader->Size - 2);

        // GUID entry
        if (entryHeader->Type == NVRAM_EVSA_ENTRY_TYPE_GUID1 ||
            entryHeader->Type == NVRAM_EVSA_ENTRY_TYPE_GUID2) {
            const EVSA_GUID_ENTRY* guidHeader = (const EVSA_GUID_ENTRY*)entryHeader;
            header = data.mid(offset, sizeof(EVSA_GUID_ENTRY));
            body = data.mid(offset + sizeof(EVSA_GUID_ENTRY), guidHeader->Header.Size - sizeof(EVSA_GUID_ENTRY));
            EFI_GUID guid = *(EFI_GUID*)body.constData();
            name = guidToUString(guid);
            info = UString("GUID: ") + guidToUString(guid, false) + usprintf("\nFull size: %Xh (%u)\nHeader size %Xh (%u)\nBody size: %Xh (%u)\nType: %02Xh\nChecksum: %02Xh",
                variableSize, variableSize,
                header.size(), header.size(),
                body.size(), body.size(),
                guidHeader->Header.Type,
                guidHeader->Header.Checksum)
                + (guidHeader->Header.Checksum != calculated ? usprintf(", invalid, should be %02Xh", calculated) : UString(", valid"))
                + usprintf("\nGuidId: %04Xh", guidHeader->GuidId);
            subtype = Subtypes::GuidEvsaEntry;
            guidMap.insert(std::pair<UINT16, EFI_GUID>(guidHeader->GuidId, guid));
        }
        // Name entry
        else if (entryHeader->Type == NVRAM_EVSA_ENTRY_TYPE_NAME1 ||
            entryHeader->Type == NVRAM_EVSA_ENTRY_TYPE_NAME2) {
            const EVSA_NAME_ENTRY* nameHeader = (const EVSA_NAME_ENTRY*)entryHeader;
            header = data.mid(offset, sizeof(EVSA_NAME_ENTRY));
            body = data.mid(offset + sizeof(EVSA_NAME_ENTRY), nameHeader->Header.Size - sizeof(EVSA_NAME_ENTRY));
            name = UString::fromUtf16((const CHAR16*)body.constData());
            info = UString("Name: ") + name + usprintf("\nFull size: %Xh (%u)\nHeader size %Xh (%u)\nBody size: %Xh (%u)\nType: %02Xh\nChecksum: %02Xh",
                variableSize, variableSize,
                header.size(), header.size(),
                body.size(), body.size(),
                nameHeader->Header.Type,
                nameHeader->Header.Checksum)
                + (nameHeader->Header.Checksum != calculated ? usprintf(", invalid, should be %02Xh", calculated) : UString(", valid"))
                + usprintf("\nVarId: %04Xh", nameHeader->VarId);
            subtype = Subtypes::NameEvsaEntry;
            nameMap.insert(std::pair<UINT16, UString>(nameHeader->VarId, name));
        }
        // Data entry
        else if (entryHeader->Type == NVRAM_EVSA_ENTRY_TYPE_DATA1 ||
            entryHeader->Type == NVRAM_EVSA_ENTRY_TYPE_DATA2 ||
            entryHeader->Type == NVRAM_EVSA_ENTRY_TYPE_DATA_INVALID) {
            const EVSA_DATA_ENTRY* dataHeader = (const EVSA_DATA_ENTRY*)entryHeader;
            // Check for extended header
            UINT32 headerSize = sizeof(EVSA_DATA_ENTRY);
            UINT32 dataSize = dataHeader->Header.Size - sizeof(EVSA_DATA_ENTRY);
            if (dataHeader->Attributes & NVRAM_EVSA_DATA_EXTENDED_HEADER) {
                const EVSA_DATA_ENTRY_EXTENDED* dataHeaderExtended = (const EVSA_DATA_ENTRY_EXTENDED*)entryHeader;
                headerSize = sizeof(EVSA_DATA_ENTRY_EXTENDED);
                dataSize = dataHeaderExtended->DataSize;
                variableSize = headerSize + dataSize;
            }

            header = data.mid(offset, headerSize);
            body = data.mid(offset + headerSize, dataSize);
            name = UString("Data");
            info = usprintf("Full size: %Xh (%u)\nHeader size %Xh (%u)\nBody size: %Xh (%u)\nType: %02Xh\nChecksum: %02Xh",
                variableSize, variableSize,
                headerSize, headerSize,
                dataSize, dataSize,
                dataHeader->Header.Type,
                dataHeader->Header.Checksum)
                + (dataHeader->Header.Checksum != calculated ? usprintf(", invalid, should be %02Xh", calculated) : UString(", valid"))
                + usprintf("\nVarId: %04Xh\nGuidId: %04Xh\nAttributes: %08Xh (",
                    dataHeader->VarId,
                    dataHeader->GuidId,
                    dataHeader->Attributes)
                + evsaAttributesToUString(dataHeader->Attributes) + UString(")");
            subtype = Subtypes::DataEvsaEntry;
        }
        // Unknown entry or free space
        else {
            body = data.mid(offset);
            info = usprintf("Full size: %Xh (%u)", body.size(), body.size());

            if (body.count(emptyByte) == body.size()) { // Free space
                // Add free space tree item
                model->addItem(localOffset + offset, Types::FreeSpace, 0, UString("Free space"), UString(), info, UByteArray(), body, UByteArray(), Fixed, index);
            }
            else {
                // Add padding tree item
                UModelIndex itemIndex = model->addItem(localOffset + offset, Types::Padding, getPaddingType(body), UString("Padding"), UString(), info, UByteArray(), body, UByteArray(), Fixed, index);

                // Show message
                msg(usprintf("%s: unknown variable of type %02Xh found at offset %Xh, the rest of unparsed store added as padding", __FUNCTION__, entryHeader->Type, offset), itemIndex);
            }
            break;
        }

        // Add tree item
        model->addItem(localOffset + offset, Types::EvsaEntry, subtype, name, UString(), info, header, body, UByteArray(), Fixed, index);

        // Move to next variable
        offset += variableSize;
        unparsedSize = storeDataSize - offset;
    }

    // Reparse all data variables to detect invalid ones and assign name and test to valid ones
    for (int i = 0; i < model->rowCount(index); i++) {
        UModelIndex current = index.child(i, 0);
        if (model->subtype(current) == Subtypes::DataEvsaEntry) {
            UByteArray header = model->header(current);
            const EVSA_DATA_ENTRY* dataHeader = (const EVSA_DATA_ENTRY*)header.constData();
            UString guid;
            if (guidMap.count(dataHeader->GuidId))
                guid = guidToUString(guidMap[dataHeader->GuidId], false);
            UString name;
            if (nameMap.count(dataHeader->VarId))
                name = nameMap[dataHeader->VarId];

            // Check for variable validity
            if (guid.isEmpty() && name.isEmpty()) { // Both name and guid aren't found
                model->setSubtype(current, Subtypes::InvalidEvsaEntry);
                model->setName(current, UString("Invalid"));
                msg(usprintf("%s: data variable with invalid GuidId and invalid VarId", __FUNCTION__), current);
            }
            else if (guid.isEmpty()) { // Guid not found
                model->setSubtype(current, Subtypes::InvalidEvsaEntry);
                model->setName(current, UString("Invalid"));
                msg(usprintf("%s: data variable with invalid GuidId", __FUNCTION__), current);
            }
            else if (name.isEmpty()) { // Name not found
                model->setSubtype(current, Subtypes::InvalidEvsaEntry);
                model->setName(current, UString("Invalid"));
                msg(usprintf("%s: data variable with invalid VarId", __FUNCTION__), current);
            }
            else { // Variable is OK, rename it
                if (dataHeader->Header.Type == NVRAM_EVSA_ENTRY_TYPE_DATA_INVALID) {
                    model->setSubtype(current, Subtypes::InvalidEvsaEntry);
                    model->setName(current, UString("Invalid"));
                }
                else {
                    model->setName(current, guid);
                }
                model->setText(current, name);
                model->addInfo(current, UString("GUID: ") + guid + UString("\nName: ") + name + UString("\n"), false);
            }
        }
    }

    return U_SUCCESS;
}


USTATUS NvramParser::parseFlashMapBody(const UModelIndex & index)
{
    // Sanity check
    if (!index.isValid())
        return U_INVALID_PARAMETER;

    // Get parsing data for the current item
    UINT32 localOffset = model->header(index).size();
    const UByteArray data = model->body(index);


    const UINT32 dataSize = (UINT32)data.size();
    UINT32 offset = 0;
    UINT32 unparsedSize = dataSize;
    // Parse all entries
    while (unparsedSize) {
        const PHOENIX_FLASH_MAP_ENTRY* entryHeader = (const PHOENIX_FLASH_MAP_ENTRY*)(data.constData() + offset);

        // Check entry size
        if (unparsedSize < sizeof(PHOENIX_FLASH_MAP_ENTRY)) {
            // Last variable is bad, add the rest as padding and return
            UByteArray body = data.mid(offset);
            UString info = usprintf("Full size: %Xh (%u)", body.size(), body.size());

            // Add padding tree item
            model->addItem(localOffset + offset, Types::Padding, getPaddingType(body), UString("Padding"), UString(), info, UByteArray(), body, UByteArray(), Fixed, index);

            // Show message
            if (unparsedSize < entryHeader->Size)
                msg(usprintf("%s: next entry appears too big, added as padding", __FUNCTION__), index);

            break;
        }

        UString name = guidToUString(entryHeader->Guid);

        // Construct header
        UByteArray header = data.mid(offset, sizeof(PHOENIX_FLASH_MAP_ENTRY));

        // Add info
        UString info = UString("Entry GUID: ") + guidToUString(entryHeader->Guid, false) +
            usprintf("\nFull size: 24h (36)\nHeader size: 24h (36)\nBody size: 0h (0)\n"
                "Entry type: %04Xh\nData type: %04Xh\nMemory address: %08Xh\nSize: %08Xh\nOffset: %08Xh",
                entryHeader->EntryType,
                entryHeader->DataType,
                entryHeader->PhysicalAddress,
                entryHeader->Size,
                entryHeader->Offset);

        // Determine subtype
        UINT8 subtype = 0;
        switch (entryHeader->DataType) {
        case NVRAM_PHOENIX_FLASH_MAP_ENTRY_TYPE_VOLUME:
            subtype = Subtypes::VolumeFlashMapEntry;
            break;
        case NVRAM_PHOENIX_FLASH_MAP_ENTRY_TYPE_DATA_BLOCK:
            subtype = Subtypes::DataFlashMapEntry;
            break;
        }

        // Add tree item
        model->addItem(localOffset + offset, Types::FlashMapEntry, subtype, name, flashMapGuidToUString(entryHeader->Guid), info, header, UByteArray(), UByteArray(), Fixed, index);

        // Move to next variable
        offset += sizeof(PHOENIX_FLASH_MAP_ENTRY);
        unparsedSize = dataSize - offset;
    }

    return U_SUCCESS;
}
#endif // U_ENABLE_NVRAM_PARSING_SUPPORT
