/* nvramparser.cpp
 
 Copyright (c) 2016, Nikolaj Schlej. All rights reserved.
 
 This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php.
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 */

#ifdef U_ENABLE_NVRAM_PARSING_SUPPORT
#include <map>

#include "nvramparser.h"
#include "parsingdata.h"
#include "ustring.h"
#include "utility.h"
#include "nvram.h"
#include "ffs.h"
#include "intel_microcode.h"

#include "umemstream.h"
#include "kaitai/kaitaistream.h"
#include "generated/ami_nvar.h"
#include "generated/edk2_vss.h"

USTATUS NvramParser::parseNvarStore(const UModelIndex & index)
{
    // Sanity check
    if (!index.isValid())
        return U_INVALID_PARAMETER;

    UByteArray nvar = model->body(index);

    // Nothing to parse in an empty store
    if (nvar.isEmpty())
        return U_SUCCESS;

    try {
        const UINT32 localOffset = (UINT32)model->header(index).size();
        umemstream is(nvar.constData(), nvar.size());
        kaitai::kstream ks(&is);
        ami_nvar_t parsed(&ks);

        UINT16 guidsInStore = 0;
        UINT32 currentEntryIndex = 0;
        for (const auto & entry : *parsed.entries()) {
            UINT8 subtype = Subtypes::FullNvarEntry;
            UString name;
            UString text;
            UString info;
            UString guid;
            UByteArray header;
            UByteArray body;
            UByteArray tail;

            // This is a terminating entry, needs special processing
            if (entry->_is_null_signature_rest()) {
                UINT32 guidAreaSize = guidsInStore * sizeof(EFI_GUID);
                UINT32 unparsedSize = (UINT32)nvar.size() - entry->offset() - guidAreaSize;

                // Check if the data left is a free space or a padding
                UByteArray padding = nvar.mid(entry->offset(), unparsedSize);

                // Get info
                UString info = usprintf("Full size: %Xh (%u)", (UINT32)padding.size(), (UINT32)padding.size());

                if ((UINT32)padding.count(0xFF) == unparsedSize) { // Free space
                    // Add tree item
                    model->addItem(localOffset + entry->offset(), Types::FreeSpace, 0, UString("Free space"), UString(), info, UByteArray(), padding, UByteArray(), Fixed, index);
                }
                else {
                    // Nothing is parsed yet, but the file is not empty
                    if (entry->offset() == 0) {
                        msg(usprintf("%s: file can't be parsed as NVAR variable store", __FUNCTION__), index);
                        return U_SUCCESS;
                    }

                    // Add tree item
                    model->addItem(localOffset + entry->offset(), Types::Padding, getPaddingType(padding), UString("Padding"), UString(), info, UByteArray(), padding, UByteArray(), Fixed, index);
                }

                // Add GUID store area
                UByteArray guidArea = nvar.right(guidAreaSize);
                // Get info
                name = UString("GUID store");
                info = usprintf("Full size: %Xh (%u)\nGUIDs in store: %u",
                                (UINT32)guidArea.size(), (UINT32)guidArea.size(),
                                guidsInStore);
                // Add tree item
                model->addItem((UINT32)(localOffset + entry->offset() + padding.size()), Types::NvarGuidStore, 0, name, UString(), info, UByteArray(), guidArea, UByteArray(), Fixed, index);

                return U_SUCCESS;
            }

            // This is a normal entry
            const auto entry_body = entry->body();

            // Set default next to predefined last value
            NVAR_ENTRY_PARSING_DATA pdata = {};
            pdata.emptyByte = 0xFF;
            pdata.next = 0xFFFFFF;
            pdata.isValid = TRUE;

            // Check for invalid entry
            if (!entry->attributes()->valid()) {
                subtype = Subtypes::InvalidNvarEntry;
                name = UString("Invalid");
                pdata.isValid = FALSE;
                goto processing_done;
            }

            // Check for link entry
            if (entry->next() != 0xFFFFFF) {
                subtype = Subtypes::LinkNvarEntry;
                pdata.next = (UINT32)entry->next();
            }

            // Check for data-only entry (nameless and GUIDless entry or link)
            if (entry->attributes()->data_only()) {
                // Search backwards for a previous entry with a link to this variable
                UModelIndex prevEntryIndex;
                if (currentEntryIndex > 0) {
                    for (UINT32 i = currentEntryIndex - 1; i > 0; i--) {
                        const auto & previousEntry = parsed.entries()->at(i);

                        if (previousEntry == entry)
                            break;

                        if ((UINT32)previousEntry->next() + (UINT32)previousEntry->offset() == (UINT32)entry->offset()) { // Previous link is present and valid
                            prevEntryIndex = index.model()->index(i, 0, index);
                            // Make sure that we are linking to a valid entry
                            NVAR_ENTRY_PARSING_DATA pd = readUnaligned((NVAR_ENTRY_PARSING_DATA*)model->parsingData(prevEntryIndex).constData());
                            if (!pd.isValid) {
                                prevEntryIndex = UModelIndex();
                            }
                            break;
                        }
                    }
                }
                // Check if the link is valid
                if (prevEntryIndex.isValid()) {
                    // Use the name and text of the previous entry
                    name = model->name(prevEntryIndex);
                    text = model->text(prevEntryIndex);

                    if (entry->next() == 0xFFFFFF)
                        subtype = Subtypes::DataNvarEntry;
                }
                else {
                    subtype = Subtypes::InvalidLinkNvarEntry;
                    name = UString("InvalidLink");
                    pdata.isValid = FALSE;
                }
                goto processing_done;
            }

            // Obtain text
            if (!entry_body->_is_null_ascii_name()) {
                text = entry_body->ascii_name().c_str();
            }
            else if (!entry_body->_is_null_ucs2_name()) {
                UByteArray temp;
                for (const auto & ch : *entry_body->ucs2_name()->ucs2_chars()) {
                    temp += UByteArray((const char*)&ch, sizeof(ch));
                }
                text = uFromUcs2(temp.constData());
            }

            // Obtain GUID
            if (!entry_body->_is_null_guid()) { // GUID is stored in the entry itself
                const EFI_GUID g = readUnaligned((EFI_GUID*)entry_body->guid().c_str());
                name = guidToUString(g);
                guid = guidToUString(g, false);
            }
            else { // GUID is stored in GUID store at the end of the NVAR store
                // Grow the GUID store if needed
                if (guidsInStore < entry_body->guid_index() + 1)
                    guidsInStore = entry_body->guid_index() + 1;

                // The list begins at the end of the store and goes backwards
                const EFI_GUID g = readUnaligned((EFI_GUID*)(nvar.constData() + nvar.size()) - (entry_body->guid_index() + 1));
                name = guidToUString(g);
                guid = guidToUString(g, false);
            }

processing_done:
            // This feels hacky, but I haven't found a way to ask Kaitai for raw bytes
            header = nvar.mid(entry->offset(), sizeof(NVAR_ENTRY_HEADER) + entry_body->data_start_offset());
            body = nvar.mid(entry->offset() + sizeof(NVAR_ENTRY_HEADER) + entry_body->data_start_offset(), entry_body->data_size());
            tail = nvar.mid(entry->end_offset() - entry_body->extended_header_size(), entry_body->extended_header_size());

            // Add GUID info for valid entries
            if (!guid.isEmpty())
                info += UString("Variable GUID: ") + guid + "\n";

            // Add GUID index information
            if (!entry_body->_is_null_guid_index())
                info += usprintf("GUID index: %u\n", entry_body->guid_index());

            // Add header, body and extended data info
            info += usprintf("Full size: %Xh (%u)\nHeader size: %Xh (%u)\nBody size: %Xh (%u)\nTail size: %Xh (%u)",
                             entry->size(), entry->size(),
                             (UINT32)header.size(), (UINT32)header.size(),
                             (UINT32)body.size(), (UINT32)body.size(),
                             (UINT32)tail.size(), (UINT32)tail.size());

            // Add attributes info
            const NVAR_ENTRY_HEADER entryHeader = readUnaligned((NVAR_ENTRY_HEADER*)header.constData());
            info += usprintf("\nAttributes: %02Xh", entryHeader.Attributes);

            // Translate attributes to text
            if (entryHeader.Attributes != 0x00 && entryHeader.Attributes != 0xFF)
                info += UString(" (") + nvarAttributesToUString(entryHeader.Attributes) + UString(")");

            // Add next node info
            if (entry->next() != 0xFFFFFF)
                info += usprintf("\nNext node at offset: %Xh", localOffset + entry->offset() + (UINT32)entry->next());

            // Add extended header info
            if (entry_body->extended_header_size() > 0) {
                info += usprintf("\nExtended header size: %Xh (%u)",
                                 entry_body->extended_header_size(), entry_body->extended_header_size());

                const UINT8 extendedAttributes = *tail.constData();
                info += usprintf("\nExtended attributes: %02Xh (", extendedAttributes) + nvarExtendedAttributesToUString(extendedAttributes) + UString(")");

                // Add checksum
                if (!entry_body->_is_null_extended_header_checksum()) {
                    UINT8 calculatedChecksum = 0;
                    UByteArray wholeBody = body + tail;

                    // Include entry body
                    UINT8* start = (UINT8*)wholeBody.constData();
                    for (UINT8* p = start; p < start + wholeBody.size(); p++) {
                        calculatedChecksum += *p;
                    }
                    // Include entry size and flags
                    start = (UINT8*)&entryHeader.Size;
                    for (UINT8*p = start; p < start + sizeof(UINT16); p++) {
                        calculatedChecksum += *p;
                    }
                    // Include entry attributes
                    calculatedChecksum += entryHeader.Attributes;
                    info += usprintf("\nChecksum: %02Xh, ", entry_body->extended_header_checksum())
                     + (calculatedChecksum ? usprintf(", invalid, should be %02Xh", 0x100 - calculatedChecksum) : UString(", valid"));
                }

                // Add timestamp
                if (!entry_body->_is_null_extended_header_timestamp())
                    info += usprintf("\nTimestamp: %" PRIX64 "h", entry_body->extended_header_timestamp());

                // Add hash
                if (!entry_body->_is_null_extended_header_hash()) {
                    UByteArray hash = UByteArray(entry_body->extended_header_hash().c_str(), entry_body->extended_header_hash().size());
                    info += UString("\nHash: ") + UString(hash.toHex().constData());
                }
            }

            // Add tree item
            UModelIndex varIndex = model->addItem(localOffset + entry->offset(), Types::NvarEntry, subtype, name, text, info, header, body, tail, Fixed, index);
            currentEntryIndex++;

            // Set parsing data
            model->setParsingData(varIndex, UByteArray((const char*)&pdata, sizeof(pdata)));

            // Try parsing the entry data as NVAR storage if it begins with NVAR signature
            if ((subtype == Subtypes::DataNvarEntry || subtype == Subtypes::FullNvarEntry)
                && body.size() >= 4 && readUnaligned((const UINT32*)body.constData()) == NVRAM_NVAR_ENTRY_SIGNATURE)
                (void)parseNvarStore(varIndex);
        }
    }
    catch (...) {
        msg(usprintf("%s: unable to parse AMI NVAR storage", __FUNCTION__), index);
        return U_INVALID_STORE;
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
    const UINT32 localOffset = (UINT32)model->header(index).size();
    
    // Get item data
    UByteArray volumeBody = model->body(index);
    const UINT32 volumeBodySize = (UINT32)volumeBody.size();

    // Iterate over all bytes inside the volume body, trying to parse every next byte offset by one of the known parsers
    UByteArray padding;
    for (UINT32 offset = 0;
         offset < volumeBodySize;
         offset++) {
        bool storeFound = false;
        // Try parsing as VSS store
        try {
            UByteArray vss = volumeBody.mid(offset);
            umemstream is(vss.constData(), vss.size());
            kaitai::kstream ks(&is);
            edk2_vss_t parsed(&ks);

            // VSS store at current offset parsed correctly
            msg(usprintf("%s: VSS store found at offset: %Xh, paddingSize: %Xh", __FUNCTION__, localOffset + offset, (UINT32)padding.size()), index);

            storeFound = true;
            padding.clear();

            offset += parsed.size() - 1;
        } catch (...) {
           // Parsing failed try something else
        }

        //TODO: all other kinds of stores
        // if (!storeFound && ...)

        // This byte had not been parsed as anything
        if (!storeFound)
            padding += volumeBody.at(offset);
    }

    return U_SUCCESS;
}
#endif // U_ENABLE_NVRAM_PARSING_SUPPORT
