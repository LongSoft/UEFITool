/* fitparser.cpp

Copyright (c) 2015, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHWARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*/
#include "fitparser.h"

USTATUS FitParser::parse(const UModelIndex & index, const UModelIndex & lastVtfIndex)
{
    // Check sanity
    if (!index.isValid() || !lastVtfIndex.isValid())
        return EFI_INVALID_PARAMETER;

    // Store lastVtfIndex
    lastVtf = lastVtfIndex;

    // Search for FIT
    UModelIndex fitIndex;
    UINT32 fitOffset;
    USTATUS result = findFitRecursive(index, fitIndex, fitOffset);
    if (result)
        return result;

    // FIT not found
    if (!fitIndex.isValid())
        return U_SUCCESS;
    
    // Explicitly set the item as fixed
    model->setFixed(index, true);

    // Special case of FIT header
    const FIT_ENTRY* fitHeader = (const FIT_ENTRY*)(model->body(fitIndex).constData() + fitOffset);
    
    // Check FIT checksum, if present
    UINT32 fitSize = (fitHeader->Size & 0xFFFFFF) << 4;
    if (fitHeader->Type & 0x80) {
        // Calculate FIT entry checksum
        UByteArray tempFIT = model->body(fitIndex).mid(fitOffset, fitSize);
        FIT_ENTRY* tempFitHeader = (FIT_ENTRY*)tempFIT.data();
        tempFitHeader->Checksum = 0;
        UINT8 calculated = calculateChecksum8((const UINT8*)tempFitHeader, fitSize);
        if (calculated != fitHeader->Checksum) {
            msg(usprintf("Invalid FIT table checksum %02Xh, should be %02Xh", fitHeader->Checksum, calculated), fitIndex);
        }
    }

    // Check fit header type
    if ((fitHeader->Type & 0x7F) != FIT_TYPE_HEADER) {
        msg(("Invalid FIT header type"), fitIndex);
    }

    // Add FIT header to fitTable
    std::vector<UString> currentStrings;
    currentStrings.push_back(UString("_FIT_           "));
    currentStrings.push_back(usprintf("%08X", fitSize));
    currentStrings.push_back(usprintf("%04X", fitHeader->Version));
    currentStrings.push_back(fitEntryTypeToUString(fitHeader->Type));
    currentStrings.push_back(usprintf("%02X", fitHeader->Checksum));
    fitTable.push_back(currentStrings);

    // Process all other entries
    bool msgModifiedImageMayNotWork = false;
    for (UINT32 i = 1; i < fitHeader->Size; i++) {
        currentStrings.clear();
        const FIT_ENTRY* currentEntry = fitHeader + i;

        // Check entry type
        switch (currentEntry->Type & 0x7F) {
        case FIT_TYPE_HEADER:
            msg(UString("Second FIT header found, the table is damaged"), fitIndex);
            break;

        case FIT_TYPE_EMPTY:
        case FIT_TYPE_MICROCODE:
            break;

        case FIT_TYPE_BIOS_AC_MODULE:
        case FIT_TYPE_BIOS_INIT_MODULE:
        case FIT_TYPE_TPM_POLICY:
        case FIT_TYPE_BIOS_POLICY_DATA:
        case FIT_TYPE_TXT_CONF_POLICY:
        case FIT_TYPE_AC_KEY_MANIFEST:
        case FIT_TYPE_AC_BOOT_POLICY:
        default:
            msgModifiedImageMayNotWork = true;
            break;
        }

        // Add entry to fitTable
        currentStrings.push_back(usprintf("%016X",currentEntry->Address));
        currentStrings.push_back(usprintf("%08X", currentEntry->Size));
        currentStrings.push_back(usprintf("%04X", currentEntry->Version));
        currentStrings.push_back(fitEntryTypeToUString(currentEntry->Type));
        currentStrings.push_back(usprintf("%02X", currentEntry->Checksum));
        fitTable.push_back(currentStrings);
    }

    if (msgModifiedImageMayNotWork)
        msg(("Opened image may not work after any modification"));

    return U_SUCCESS;
}

UString FitParser::fitEntryTypeToUString(UINT8 type)
{
    switch (type & 0x7F) {
    case FIT_TYPE_HEADER:           return ("Header                  ");
    case FIT_TYPE_MICROCODE:        return ("Microcode               ");
    case FIT_TYPE_BIOS_AC_MODULE:   return ("BIOS ACM                ");
    case FIT_TYPE_BIOS_INIT_MODULE: return ("BIOS Init               ");
    case FIT_TYPE_TPM_POLICY:       return ("TPM Policy              ");
    case FIT_TYPE_BIOS_POLICY_DATA: return ("BIOS Policy Data        ");
    case FIT_TYPE_TXT_CONF_POLICY:  return ("TXT Configuration Policy");
    case FIT_TYPE_AC_KEY_MANIFEST:  return ("BootGuard Key Manifest  ");
    case FIT_TYPE_AC_BOOT_POLICY:   return ("BootGuard Boot Policy   ");
    case FIT_TYPE_EMPTY:            return ("Empty                   ");
    default:                        return ("Unknown                 ");
    }
}

USTATUS FitParser::findFitRecursive(const UModelIndex & index, UModelIndex & found, UINT32 & fitOffset)
{
    // Sanity check
    if (!index.isValid())
        return U_SUCCESS;

    // Process child items
    for (int i = 0; i < model->rowCount(index); i++) {
        findFitRecursive(index.child(i, 0), found, fitOffset);
        if (found.isValid())
            return U_SUCCESS;
    }

    // Get parsing data for the current item
    PARSING_DATA pdata = parsingDataFromUModelIndex(index);

    // Check for all FIT signatures in item's body
    for (INT32 offset = model->body(index).indexOf(FIT_SIGNATURE); 
         offset >= 0; 
         offset = model->body(index).indexOf(FIT_SIGNATURE, offset + 1)) {
        // FIT candidate found, calculate it's physical address
        UINT32 fitAddress = pdata.address + model->header(index).size() + (UINT32)offset;
            
        // Check FIT address to be in the last VTF
        UByteArray lastVtfBody = model->body(lastVtf);
        if (*(const UINT32*)(lastVtfBody.constData() + lastVtfBody.size() - FIT_POINTER_OFFSET) == fitAddress) {
            found = index;
            fitOffset = offset;
            msg(usprintf("Real FIT table found at physical address %Xh", fitAddress), found);
            return U_SUCCESS;
        }
        else if (model->rowCount(index) == 0) // Show messages only to leaf items
            msg(("FIT table candidate found, but not referenced from the last VTF"), index);
    }

    return U_SUCCESS;
}