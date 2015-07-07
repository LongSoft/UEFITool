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
#include "types.h"
#include "treemodel.h"


FitParser::FitParser(TreeModel* treeModel, QObject *parent)
    : QObject(parent), model(treeModel)
{
}

FitParser::~FitParser()
{
}

STATUS FitParser::parse(const QModelIndex & index, const QModelIndex & lastVtfIndex)
{
    // Check sanity
    if (!index.isValid() || !lastVtfIndex.isValid())
        return EFI_INVALID_PARAMETER;

    // Store lastVtfIndex
    lastVtf = lastVtfIndex;

    // Search for FIT
    QModelIndex fitIndex;
    UINT32 fitOffset;
    STATUS result = findFitRecursive(index, fitIndex, fitOffset);
    if (result)
        return result;

    // FIT not found
    if (!fitIndex.isValid())
        return ERR_SUCCESS;
    
    // Get parsing data for the current item
    PARSING_DATA pdata = parsingDataFromQModelIndex(fitIndex);

    // Explicitly set the item as fixed
    pdata.fixed = TRUE;

    // Set modified parsing data
    model->setParsingData(fitIndex, parsingDataToQByteArray(pdata));

    // Add all FIT entries into QVector
    const FIT_ENTRY* fitHeader = (const FIT_ENTRY*)(model->body(fitIndex).constData() + fitOffset);

    // Special case of FIT header
    QString remark;

    // Check FIT checksum, if present
    if (fitHeader->Type & 0x80) {
        // Calculate FIT entry checksum
        UINT32 fitSize = (fitHeader->Size & 0xFFFFFF) << 4;
        UINT8 calculated = calculateChecksum8((const UINT8*)fitHeader, fitSize);
        if (calculated) {
            remark.append(tr("Invalid FIT table checksum, ").hexarg2(calculated, 2));
        }
    }

    // Check fit header version and type
    if (fitHeader->Version != FIT_HEADER_VERSION) {
        remark.append(tr("Invalid FIT header version, "));
    }
    if ((fitHeader->Type & 0x7F) != FIT_TYPE_HEADER) {
        remark.append(tr("Invalid FIT header type, "));
    }

    // Remove the last ", " from remark string, if needed
    if (!remark.isEmpty())
        remark = remark.left(remark.length() - 2);

    // Add FIT header to fitEntries vector
    fitEntries.append(QPair<FIT_ENTRY, QString>(*fitHeader, remark));

    // Process all other entries
    for (UINT32 i = 1; i < fitHeader->Size; i++) {
        remark.clear();
        const FIT_ENTRY* currentEntry = fitHeader + i;

        // Check entry type
        switch (currentEntry->Type & 0x7F) {
        case FIT_TYPE_HEADER:
            remark.append(tr("Second FIT header found, the table is damaged"));
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
            remark.append(tr("Modified image may not work"));
            break;
        }

        fitEntries.append(QPair<FIT_ENTRY, QString>(*currentEntry, remark));
    }

    return ERR_SUCCESS;
}

STATUS FitParser::findFitRecursive(const QModelIndex & index, QModelIndex & found, UINT32 & fitOffset)
{
    // Sanity check
    if (!index.isValid())
        return EFI_SUCCESS;

    // Process child items
    for (int i = 0; i < model->rowCount(index); i++) {
        findFitRecursive(index.child(i, 0), found, fitOffset);
        if (found.isValid())
            return EFI_SUCCESS;
    }

    // Get parsing data for the current item
    PARSING_DATA pdata = parsingDataFromQModelIndex(index);

    // Check item's address to be in required range
    INT32 offset = model->body(index).indexOf(FIT_SIGNATURE);
    // Check for FIT signature in item's body
    if (offset >= 0) {
        // FIT candidate found, calculate it's physical address
        UINT32 fitAddress = pdata.address + model->header(index).size() + (UINT32)offset;
            
        // Check FIT address to be in the last VTF
        QByteArray lastVtfBody = model->body(lastVtf);
        if (*(const UINT32*)(lastVtfBody.constData() + lastVtfBody.size() - FIT_POINTER_OFFSET) == fitAddress) {
            found = index;
            fitOffset = offset;
            return ERR_SUCCESS;
        }
    }

    return ERR_SUCCESS;
}