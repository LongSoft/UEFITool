/* ffsutil.cpp

Copyright (c) 2014, tuxuser. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#include "../ffs.h"
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

UINT8 FFSUtil::extract(QModelIndex & index, QByteArray & extracted, UINT8 mode) {
    return ffsEngine->extract(index, extracted, mode);
}

UINT8 FFSUtil::compress(QByteArray & data, UINT8 algorithm, QByteArray & compressedData) {
    return ffsEngine->compress(data, algorithm, compressedData);
}

UINT8 FFSUtil::decompress(QByteArray & compressed, UINT8 compressionType, QByteArray & decompressedData, UINT8 *algorithm) {
    return ffsEngine->decompress(compressed, compressionType, decompressedData, algorithm);
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

    if(!ffsEngine->treeModel()->name(index).compare(guid)) {
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

    name = ffsEngine->treeModel()->text(result);

    return ERR_SUCCESS;
}

UINT8 FFSUtil::getLastVolumeIndex(QModelIndex & result)
{
    UINT8 ret;
    QModelIndex amiFileIdx, volumeIdxCount;
    QModelIndex rootIdx;

    getRootIndex(rootIdx);
	
    for(int i=0; i < AMIBOARD_SIZE; i++){
	ret = findFileByGUID(rootIdx, amiBoardSection[i].GUID, amiFileIdx);
	if(ret) {
           printf("ERROR: '%s' [%s] couldn't be found!\n", qPrintable(amiBoardSection[i].name), qPrintable(amiBoardSection[i].GUID));
    	} else
	   break;
    }
    if(ret)
	return ERR_ITEM_NOT_FOUND;

    getLastSibling(amiFileIdx, volumeIdxCount);
    result = volumeIdxCount;
    return ERR_SUCCESS;
}

UINT8 FFSUtil::getAmiBoardPE32Index(QModelIndex & result)
{
    UINT8 ret;
    QModelIndex amiFileIdx, amiSectionIdx, rootIdx;

    getRootIndex(rootIdx);

    for(int i=0; i < AMIBOARD_SIZE; i++){
        ret = findFileByGUID(rootIdx, amiBoardSection[i].GUID, amiFileIdx);
        if(ret) {
           printf("ERROR: '%s' [%s] couldn't be found!\n", qPrintable(amiBoardSection[i].name), qPrintable(amiBoardSection[i].GUID));
        } else
           break;
    }
    if(ret)
        return ERR_ITEM_NOT_FOUND;

    ret = findSectionByIndex(amiFileIdx, EFI_SECTION_PE32, amiSectionIdx);
    if(ret) {
        printf("ERROR: PE32 Section of AmiBoardInfo couldn't be found!\n");
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

   ret = extract(amiSectionIdx, amiboard, EXTRACT_MODE_BODY);
   if(ret){
        printf("ERROR: Failed to dump AmiBoardInfo from BIOS!\n");
        return ret;
    }

    printf("* Dumped AmiBoardInfo from BIOS...\n");
    printf("* Injecting DSDT into AmiBoardInfo...\n");

    ret = injectDSDTintoAmiboardInfo(amiboard, dsdt, patchedAmiboard);
    if (ret){
        printf("ERROR: Failed to patch DSDT into AmiBoardInfo!\n");
        return ret;
    }

    printf("* Injected new DSDT into AmiBoardInfo\n");

    ret = replace(amiSectionIdx, patchedAmiboard, REPLACE_MODE_BODY);
    if(ret) {
        printf("ERROR: Failed to replace AmiBoardInfo\n");
        return ERR_REPLACE;
    }

    printf("* Replaced AmiBoardInfo in BIOS with patched one\n");

    return ERR_SUCCESS;
}

UINT8 FFSUtil::injectFile(QByteArray file)
{
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
        printf("* File not existant, inserting at the end of volume\n");
        ret = insert(volIdx, file, CREATE_MODE_AFTER);
        if(ret) {
            printf("ERROR: Injection failed!\n");
            return ret;
        }
    }
    else {
        /* Found, replace at known index */
        printf("* File is already present -> Replacing it!\n");
        ret = replace(currIdx, file, REPLACE_MODE_AS_IS); // as-is for whole File
        if(ret) {
            printf("ERROR: Replacing failed!\n");
            return ret;
        }
    }
    return ERR_SUCCESS;
}

UINT8 FFSUtil::deleteFilesystemFfs()
{
    UINT8 ret;
    QModelIndex rootIdx, currIdx;

    getRootIndex(rootIdx);

    printf("Deleting non required Filesystem FFS...\n");

    ret = findFileByGUID(rootIdx, filesystemSection.GUID, currIdx);
    if(ret) {
        printf("Warning: '%s' [%s] was not found!\n", qPrintable(filesystemSection.name), qPrintable(filesystemSection.GUID));
        return ret;
    }

    ret = remove(currIdx);
    if(ret) {
        printf("Warning: Removing entry '%s' [%s] failed!\n", qPrintable(filesystemSection.name), qPrintable(filesystemSection.GUID));
        return ret;
    }

    printf("* Removed '%s' [%s] succesfully!\n", qPrintable(filesystemSection.name), qPrintable(filesystemSection.GUID));
    return ERR_SUCCESS;
}


UINT8 FFSUtil::compressDXE()
{
    UINT8 ret;
    QModelIndex rootIdx, currIdx;
    QByteArray buf, compressed;

    getRootIndex(rootIdx);

    printf("Compressing CORE_DXE to save space...\n");

    ret = findFileByGUID(rootIdx, coreDxeSection.GUID, currIdx);
    if(ret)
        return ret;

    ret = dumpFileByGUID(coreDxeSection.GUID, buf, EXTRACT_MODE_AS_IS);
    if (ret) {
        printf("ERROR: Failed to dump '%s' [%s] !\n", qPrintable(coreDxeSection.name), qPrintable(coreDxeSection.GUID));
        return ret;
    }

    ret = compressFFS(buf, compressed);
    if (ret) {
        printf("ERROR: Failed to re/compress '%s'' [%s] !\n", qPrintable(coreDxeSection.name), qPrintable(coreDxeSection.GUID));
        return ret;
    }

    ret = replace(currIdx, compressed, REPLACE_MODE_AS_IS);
    if(ret) {
        printf("Warning: Injecting compressed '%s' [%s] failed!\n", qPrintable(coreDxeSection.name), qPrintable(coreDxeSection.GUID));
        return ret;
    }
    printf("* File was injected compressed successfully!\n");
    return ERR_SUCCESS;
}


UINT8 FFSUtil::compressFFS(QByteArray ffs, QByteArray & out)
{
    UINT8 ret;
    QByteArray body, compressedBody;
    EFI_COMPRESSION_SECTION *compressionHeader = new EFI_COMPRESSION_SECTION();//{0};

    unsigned char *buf = (unsigned char*) ffs.data();

    // Split ffs into ffs-header and body
    EFI_FFS_FILE_HEADER* ffsHeader = (EFI_FFS_FILE_HEADER*)buf;
    EFI_COMMON_SECTION_HEADER* sectionHeader = (EFI_COMMON_SECTION_HEADER*)&buf[sizeof(EFI_FFS_FILE_HEADER)];
    UINT8 algo;

    if(sectionHeader->Type == EFI_SECTION_COMPRESSION) {
        compressionHeader = (EFI_COMPRESSION_SECTION*) sectionHeader;

        if(compressionHeader->CompressionType == EFI_STANDARD_COMPRESSION ||
           compressionHeader->CompressionType == EFI_CUSTOMIZED_COMPRESSION) {
            printf("Info: File seems already compressed!\n");
            out = ffs;
            return ERR_SUCCESS;
        }
        compressedBody = ffs.mid(sizeof(EFI_FFS_FILE_HEADER)+sizeof(EFI_COMPRESSION_SECTION));
        ret = decompress(compressedBody, compressionHeader->CompressionType, body, &algo);
        if (ret) {
            printf("ERROR: Decompression failed!\n");
            return ERR_ERROR;
        }
        compressedBody.clear();
    }
    else {
        body = ffs.mid(sizeof(EFI_FFS_FILE_HEADER));
    }

    ret = ffsEngine->compress(body, COMPRESSION_ALGORITHM_TIANO, compressedBody);
    if (ret) {
        printf("ERROR: Compression failed!\n");
        return ERR_ERROR;
    }

    // Assign infos to compression header
    compressionHeader->Type = EFI_SECTION_COMPRESSION;
    compressionHeader->CompressionType = EFI_STANDARD_COMPRESSION;
    uint32ToUint24(sizeof(EFI_COMPRESSION_SECTION) + compressedBody.size(), compressionHeader->Size);
    compressionHeader->UncompressedLength = body.size();

    out.append((const char*)compressionHeader, sizeof(EFI_COMPRESSION_SECTION));
    out.append(compressedBody);

    // Set new size and checksums in ffs header
    uint32ToUint24(sizeof(EFI_FFS_FILE_HEADER)+out.size(), ffsHeader->Size);
    ffsHeader->IntegrityCheck.Checksum.Header = 0;
    ffsHeader->IntegrityCheck.Checksum.File = 0;
    ffsHeader->IntegrityCheck.Checksum.Header = calculateChecksum8((UINT8*)ffsHeader, sizeof(EFI_FFS_FILE_HEADER)-1);
    ffsHeader->IntegrityCheck.Checksum.File = FFS_FIXED_CHECKSUM2;

    out.prepend((const char *)ffsHeader, sizeof(EFI_FFS_FILE_HEADER));

    return ERR_SUCCESS;
}


UINT8 FFSUtil::runFreeSomeSpace(int aggressivity)
{
    int i;
    UINT8 ret;
    QModelIndex rootIdx, currIdx;

    getRootIndex(rootIdx);

    switch(aggressivity) {
    case RUN_DEL_OZM_NREQ:
        printf("Deleting non-essential Ozmosis files to save space...\n");
        for(i = 0; i<OZMFFS_SIZE; i++){
            if(!OzmFfs[i].required){
                ret = findFileByGUID(rootIdx,OzmFfs[i].GUID,currIdx);
                if(ret)
                    continue;
                ret = remove(currIdx);
                if(ret)
                    printf("Warning: Removing entry '%s' [%s] failed!\n", qPrintable(OzmFfs[i].name), qPrintable(OzmFfs[i].GUID));
                else
                    printf("* Removed '%s' [%s] succesfully!\n", qPrintable(OzmFfs[i].name), qPrintable(OzmFfs[i].GUID));
            }
        }
    case RUN_DELETE:
        printf("Deleting network BIOS stuff (PXE) to save space...\n");
        for(i = 0; i<DELETABLEFFS_SIZE; i++){
            ret = findFileByGUID(rootIdx,deletableFfs[i].GUID,currIdx);
            if(ret)
                continue;
            ret = remove(currIdx);
            if(ret)
                printf("Warning: Removing entry '%s' [%s] failed!\n", qPrintable(deletableFfs[i].name), qPrintable(deletableFfs[i].GUID));
            else
                printf("* Removed '%s' [%s] succesfully!\n", qPrintable(deletableFfs[i].name), qPrintable(deletableFfs[i].GUID));
        }
    case RUN_AS_IS:
        break;
    default:
        printf("No aggressivity level for freeing space supplied, doing nothing..\n");
        break;
    }
    return ERR_SUCCESS;
}
