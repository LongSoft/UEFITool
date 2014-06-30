/* ffsutil.cpp

Copyright (c) 2014, tuxuser. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#include "ffsutil.h"
#include "util.h"
#include "common.h"

FFSUtil::FFSUtil(void)
{
    ffsEngine = new FfsEngine();
}

FFSUtil::~FFSUtil(void)
{
    delete ffsEngine;
}

UINT8 FFSUtil::insert(QModelIndex & index, QByteArray & object, UINT8 mode) {
    return ffsEngine->insert(index, object, mode);
}

UINT8 FFSUtil::replace(QModelIndex & index, QByteArray & object, UINT8 mode) {
    return ffsEngine->replace(index, object, mode);
}

UINT8 FFSUtil::remove(QModelIndex & index) {
    return ffsEngine->remove(index);
}

UINT8 FFSUtil::compress(QByteArray & data, UINT8 algorithm, QByteArray & compressedData) {
    return ffsEngine->compress(data, algorithm, compressedData);
}

UINT8 FFSUtil::decompress(QByteArray & compressed, UINT8 compressionType, QByteArray & decompressedData) {
    return ffsEngine->decompress(compressed, compressionType, decompressedData);
}

UINT8 FFSUtil::reconstructImageFile(QByteArray & out) {
    return ffsEngine->reconstructImageFile(out);
}

UINT8 FFSUtil::getRootIndex(QModelIndex &result) {
    result = ffsEngine->treeModel()->index(0,0);
    return ERR_SUCCESS;
}

UINT8 FFSUtil::findFileByGUID(const QModelIndex index, const QString guid, QModelIndex & result)
{
    UINT8 ret;

    if (!index.isValid()) {
        return ERR_INVALID_SECTION;
    }

    if(!ffsEngine->treeModel()->nameString(index).compare(guid)) {
        result = index;
        return ERR_SUCCESS;
    }

    for (int i = 0; i < ffsEngine->treeModel()->rowCount(index); i++) {
        QModelIndex childIndex = index.child(i, 0);
        ret = findFileByGUID(childIndex, guid, result);
        if(result.isValid() && (ret == ERR_SUCCESS))
            return ERR_SUCCESS;
    }
    return ERR_ITEM_NOT_FOUND;
}

UINT8 FFSUtil::findSectionByIndex(const QModelIndex index, UINT8 type, QModelIndex & result)
{
    UINT8 ret;

    if (!index.isValid()) {
        return ERR_INVALID_SECTION;
    }

    if(ffsEngine->treeModel()->subtype(index) == type) {
        result = index;
        return ERR_SUCCESS;
    }

    for (int i = 0; i < ffsEngine->treeModel()->rowCount(index); i++) {
        QModelIndex childIndex = index.child(i, 0);
        ret = findSectionByIndex(childIndex, type, result);
        if(result.isValid() && (ret == ERR_SUCCESS))
            return ERR_SUCCESS;
    }
    return ERR_ITEM_NOT_FOUND;
}

UINT8 FFSUtil::dumpFileByGUID(QString guid, QByteArray & buf, UINT8 mode)
{
    UINT8 ret;
    QModelIndex result;
    QModelIndex rootIndex = ffsEngine->treeModel()->index(0, 0);

    ret = findFileByGUID(rootIndex, guid, result);
    if(ret)
        return ERR_ITEM_NOT_FOUND;

    ret = ffsEngine->extract(result, buf, mode);
    if(ret)
        return ERR_ERROR;

    return ERR_SUCCESS;
}

UINT8 FFSUtil::dumpSectionByGUID(QString guid, UINT8 type, QByteArray & buf, UINT8 mode)
{
    UINT8 ret;
    QModelIndex resultFile;
    QModelIndex resultSection;
    QModelIndex rootIndex = ffsEngine->treeModel()->index(0, 0);

    ret = findFileByGUID(rootIndex, guid, resultFile);
    if(ret)
        return ERR_ITEM_NOT_FOUND;

    ret = findSectionByIndex(resultFile, type, resultSection);
    if(ret)
        return ERR_ITEM_NOT_FOUND;

    ret = ffsEngine->extract(resultSection, buf, mode);
    if(ret)
        return ERR_ERROR;

    return ERR_SUCCESS;
}

UINT8 FFSUtil::getNameByGUID(QString guid, QString & name)
{
    UINT8 ret;
    QModelIndex result;
    QModelIndex rootIndex = ffsEngine->treeModel()->index(0, 0);

    ret = findFileByGUID(rootIndex, guid, result);
    if(ret)
        return ERR_ITEM_NOT_FOUND;

    name = ffsEngine->treeModel()->textString(result);

    return ERR_SUCCESS;
}

UINT8 FFSUtil::getLastVolumeIndex(QModelIndex & result)
{
    UINT8 ret;
    QModelIndex amiFileIdx, volumeIdxCount;
    QModelIndex rootIdx;

    getRootIndex(rootIdx);

    ret = findFileByGUID(rootIdx, amiBoardSection.GUID, amiFileIdx);
    if(ret) {
        printf("ERROR: '%s' [%s] couldn't be found!\n", qPrintable(amiBoardSection.name), qPrintable(amiBoardSection.GUID));
        return ERR_ITEM_NOT_FOUND;
    }

    getLastSibling(amiFileIdx, volumeIdxCount);

    result = volumeIdxCount;

    return ERR_SUCCESS;
}

UINT8 FFSUtil::getAmiBoardPE32Index(QModelIndex & result)
{
    UINT8 ret;
    QModelIndex amiFileIdx, amiSectionIdx, rootIdx;

    getRootIndex(rootIdx);

    ret = findFileByGUID(rootIdx, amiBoardSection.GUID, amiFileIdx);
    if(ret) {
        printf("ERROR: '%s' [%s] couldn't be found!\n", qPrintable(amiBoardSection.name), qPrintable(amiBoardSection.GUID));
        return ERR_ITEM_NOT_FOUND;
    }

    ret = findSectionByIndex(amiFileIdx, EFI_SECTION_PE32, amiSectionIdx);
    if(ret) {
        printf("ERROR: PE32 Section of GUID %s couldn't be found!\n",qPrintable(amiBoardSection.GUID));
        return ERR_ITEM_NOT_FOUND;
    }

    result = amiSectionIdx;

    return ERR_SUCCESS;
}

UINT8 FFSUtil::getLastSibling(QModelIndex index, QModelIndex & result)
{
    int lastRow, column;

    column = 0;
    lastRow = ffsEngine->treeModel()->rowCount(index.parent());
    lastRow--;

    result = index.sibling(lastRow, column);
    return ERR_SUCCESS;
}

UINT8 FFSUtil::parseBIOSFile(QByteArray & buf)
{
    UINT8 ret = ffsEngine->parseImageFile(buf);
    if (ret)
        return ret;

    return ERR_SUCCESS;
}

UINT8 FFSUtil::injectDSDT(QByteArray dsdt)
{
    UINT8 ret;
    QModelIndex amiSectionIdx;
    QByteArray amiboard, patchedAmiboard;

    if(!dsdt.startsWith("DSDT")) {
        printf("ERROR: Input DSDT doesn't contain valid header!\n");
        return ERR_INVALID_FILE;
    }

    ret = getAmiBoardPE32Index(amiSectionIdx);
    if (ret) {
        printf("ERROR: Failed to get AmiBoardInfo PE32 Section!\n");
        return ret;
    }

    ret = dumpSectionByGUID(amiBoardSection.GUID, EFI_SECTION_PE32,
                                amiboard, EXTRACT_MODE_BODY);
    if(ret){
        printf("ERROR: Failed to dump '%s' [%s] from BIOS!\n", qPrintable(amiBoardSection.name), qPrintable(amiBoardSection.GUID));
        return ret;
    }

    printf("* Dumped AmiBoardInfo from BIOS\n");

    ret = dsdt2bios(amiboard, dsdt, patchedAmiboard);
    if(ret) {
        printf("ERROR: Failed to inject DSDT into AmiBoardInfo!\n");
        return ret;
    }

    printf("* Injected new DSDT into AmiBoardInfo\n");

    ret = replace(amiSectionIdx, patchedAmiboard, REPLACE_MODE_BODY);
    if(ret) {
        printf("ERROR: Failed to replace '%s' [%s]\n", qPrintable(amiBoardSection.name), qPrintable(amiBoardSection.GUID));
        return ERR_REPLACE;
    }

    printf("* Replaced AmiBoardInfo in BIOS with patched one\n");

    return ERR_SUCCESS;
}

UINT8 FFSUtil::injectFile(QByteArray file) {
    UINT8 ret;
    QString guid;
    QModelIndex rootIdx, volIdx, currIdx;

    getRootIndex(rootIdx);

    ret = getLastVolumeIndex(volIdx);
    if(ret) {
        printf("ERROR: Failed to get Volume Index!\n");
        return ERR_ERROR;
    }

    ret = getGUIDfromFile(file, guid);
    if (ret){
        printf("ERROR: Getting GUID from file failed!\n");
        return ret;
    }

    ret = findFileByGUID(rootIdx, guid, currIdx);
    if (ret) {
        printf("* File [%s] not existant, inserting at the end of volume\n", qPrintable(guid));
        ret = insert(volIdx, file, CREATE_MODE_AFTER);
        if(ret) {
            printf("ERROR: Injection failed!\n");
            return ret;
        }
    }
    else {
        /* Found, replace at known index */
        printf("* File [%s] is already present -> Replacing it!\n", qPrintable(guid));
        ret = replace(currIdx, file, REPLACE_MODE_AS_IS); // as-is for whole File
        if(ret) {
            printf("ERROR: Replacing failed!\n");
            return ret;
        }
    }
    return ERR_SUCCESS;
}

UINT8 FFSUtil::runFreeSomeSpace(int aggressivity) {
    int i;
    UINT8 ret;
    QModelIndex rootIdx, currIdx;

    static QList<sectionEntry> deleteFfs;
    static QList<sectionEntry> compressFfs;
    static QList<sectionEntry> OzmFfs;

    /* Just temporary... */
    for(int i = 0; i < requiredFfsCount; i++) {
        OzmFfs.append(requiredFfs[i]);
    }
    for(int i = 0; i < optionalFfsCount; i++) {
        OzmFfs.append(optionalFfs[i]);
    }
    for(int i = 0; i < compressFfsCount; i++) {
        compressFfs.append(compressedFfs[i]);
    }
    for(int i = 0; i < deletableFfsCount; i++) {
        deleteFfs.append(deletableFfs[i]);
    }

    getRootIndex(rootIdx);

    switch(aggressivity) {
    case RUN_DELETE:
        printf("Deleting network BIOS stuff (PXE) to save space...\n");
        for(i = 0; i<deleteFfs.size(); i++){
            ret = findFileByGUID(rootIdx,deleteFfs.at(i).GUID,currIdx);
            if(ret)
                continue;
            ret = remove(currIdx);
            if(ret)
                printf("Warning: Removing entry '%s' [%s] failed!\n", qPrintable(deleteFfs.at(i).name), qPrintable(deleteFfs.at(i).GUID));
            else
                printf("* Removed '%s' [%s] succesfully!\n", qPrintable(deleteFfs.at(i).name), qPrintable(deleteFfs.at(i).GUID));
        }
    case RUN_DEL_OZM_NREQ:
        printf("Deleting non-essential Ozmosis files to save space...\n");
        for(i = 0; i<OzmFfs.size(); i++){
            if(!OzmFfs.at(i).required){
                ret = findFileByGUID(rootIdx,OzmFfs.at(i).GUID,currIdx);
                if(ret)
                    continue;
                ret = remove(currIdx);
                if(ret)
                    printf("Warning: Removing entry '%s' [%s] failed!\n", qPrintable(OzmFfs.at(i).name), qPrintable(OzmFfs.at(i).GUID));
                else
                    printf("* Removed '%s' [%s] succesfully!\n", qPrintable(OzmFfs.at(i).name), qPrintable(OzmFfs.at(i).GUID));
            }
        }
    case RUN_COMPRESS:
        /*
        printf("Compressing some files to save space...\n");
        for(i = 0; i<compressFfs.size(); i++) {
            compressed.clear();
            ffs.clear();
            ret = fu->findFileByGUID(rootIndex, compressFfs.at(i).GUID, currIdx);
            if(ret)
                continue;
            ret = fu->dumpFileByGUID(compressFfs.at(i).GUID, ffs, EXTRACT_MODE_AS_IS);
            if(ret) {
                printf("Warning: Failed to get '%s' [%s] for compression!\n", qPrintable(compressFfs.at(i).name), qPrintable(compressFfs.at(i).GUID));
                continue;
            }
            printf("* Trying to compress '%s' [%s]\n", qPrintable(compressFfs.at(i).name), qPrintable(compressFfs.at(i).GUID));
            ret = fu->compress(ffs, COMPRESSION_ALGORITHM_TIANO, compressed);
            if(ret) {
                printf("Warning: Compressing '%s' [%s] failed!\n", qPrintable(compressFfs.at(i).name), qPrintable(compressFfs.at(i).GUID));
                continue;
            }
            ret = fu->replace(currIdx, compressed, REPLACE_MODE_AS_IS);
            if(ret) {
                printf("Warning: Injecting compressed '%s' [%s] failed!\n", qPrintable(compressFfs.at(i).name), qPrintable(compressFfs.at(i).GUID));
                continue;
            }
            printf("* File was injected compressed successfully!\n");
        }
        */
    case RUN_AS_IS:
        break;
    default:
        printf("No aggressivity level for freeing space supplied, doing nothing..\n");
        break;
    }
    return ERR_SUCCESS;
}
