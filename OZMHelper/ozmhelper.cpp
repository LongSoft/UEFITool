/* ozmhelper.cpp

Copyright (c) 2014, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#include <QDirIterator>
#include "../ffs.h"
#include "ffsutil.h"
#include "ozmhelper.h"

static const QString DSDTFilename =  "DSDT.aml";

#define requiredFfsCount 6

static const sectionEntry requiredFfs[] = {
  {"PartitionDxe","1FA1F39E-FEFF-4AAE-BD7B-38A070A3B609",TRUE},
  {"EnhancedFat","961578FE-B6B7-44C3-AF35-6BC705CD2B1F",TRUE},
  {"HfsPlus","4CF484CD-135F-4FDC-BAFB-1AA104B48D36",TRUE},
  {"Ozmosis","AAE65279-0761-41D1-BA13-4A3C1383603F",TRUE},
  {"OzmosisDefaults","99F2839C-57C3-411E-ABC3-ADE5267D960D",TRUE},
  {"SmcEmulatorKext","DADE1001-1B31-4FE4-8557-26FCEFC78275",TRUE}
};

#define optionalFfsCount 5

static const sectionEntry optionalFfs[] = {
  {"ExtFs","B34E5765-2E04-4DAF-867F-7F40BE6FC33D", FALSE},
  {"HermitShellX64","C57AD6B7-0515-40A8-9D21-551652854E37", FALSE},
  {"OzmosisTheme","AC255206-DCF9-4837-8353-72BBBC0AC849", FALSE},
  {"DisablerKext","DADE1002-1B31-4FE4-8557-26FCEFC78275", FALSE},
  {"InjectorKext","DADE1003-1B31-4FE4-8557-26FCEFC78275", FALSE}
};

static const sectionEntry amiBoardSection = {
  "AmiBoardInfo","9F3A0016-AE55-4288-829D-D22FD344C347", TRUE
};

static QList<sectionEntry> OzmFfs;

OZMHelper::OZMHelper(QObject *parent) :
    QObject(parent)
{
    for(int i = 0; i < requiredFfsCount; i++) {
        OzmFfs.append(requiredFfs[i]);
    }
    for(int i = 0; i < optionalFfsCount; i++) {
        OzmFfs.append(optionalFfs[i]);
    }
}

OZMHelper::~OZMHelper()
{
}

UINT8 OZMHelper::DSDTExtract(QString input, QString output)
{
    UINT8 ret;
    QString outputFile;
    QByteArray buf, dsdtbuf;

    FFSUtil *fu = new FFSUtil();

    ret = dirCreate(output);
    if (ret == ERR_DIR_CREATE) {
        printf("ERROR: Creating dir failed!\n");
        return ret;
    }

    ret = fileOpen(input, buf);
    if (ret) {
        printf("ERROR: Opening BIOS failed!\n");
        return ret;
    }

    ret = fu->parseBIOSFile(buf);
    if (ret) {
        printf("ERROR: Parsing BIOS failed!\n");
        return ret;
    }

    buf.clear();

    ret = fu->dumpSectionByGUID(amiBoardSection.GUID, EFI_SECTION_PE32, buf, EXTRACT_MODE_BODY);
    if (ret) {
        printf("ERROR: Dumping AmiBoardInfo failed!\n");
        return ret;
    }

    ret = getDSDTfromAMI(buf, dsdtbuf);
    if (ret) {
        printf("ERROR: Extracting DSDT from AmiBoardInfo failed!\n");
        return ret;
    }

    outputFile = pathConcatenate(output, amiBoardSection.name + ".bin");

    ret = fileWrite(outputFile, buf);
    if (ret) {
        printf("ERROR: Writing AmiBoardInfo.bin to '%s' failed!\n", qPrintable(outputFile));
        return ret;
    }

    outputFile = pathConcatenate(output, DSDTFilename);

    ret = fileWrite(outputFile, dsdtbuf);
    if (ret) {
        printf("ERROR: Writing DSDT.aml to '%s' failed!\n", qPrintable(outputFile));
        return ret;
    }

    buf.clear();
    dsdtbuf.clear();

    return ERR_SUCCESS;
}

UINT8 OZMHelper::OZMUpdate(QString input, QString recentBios, QString output)
{
    input = "shut";
    recentBios = "your";
    output = "warnings";
    return ERR_NOT_IMPLEMENTED;
}

UINT8 OZMHelper::OZMExtract(QString input, QString output)
{
    int i;
    UINT8 ret;
    QString outputFile;
    QByteArray buf;

    FFSUtil *fu = new FFSUtil();

    ret = fileOpen(input, buf);
    if (ret) {
        return ret;
    }

    ret = fu->parseBIOSFile(buf);
    if (ret) {
        return ret;
    }

    buf.clear();

    ret = dirCreate(output);
    if (ret == ERR_DIR_CREATE) {
        return ret;
    }

    for(i=0; i<OzmFfs.size(); i++) {
        ret = fu->dumpFileByGUID(OzmFfs.at(i).GUID, buf, EXTRACT_MODE_AS_IS);
        if (ret == ERR_ITEM_NOT_FOUND)
            continue;
        if (ret) {
            return ret;
        }

        outputFile = pathConcatenate(output,(OzmFfs.at(i).name+".ffs"));

        ret = fileWrite(outputFile, buf);
        if (ret) {
            return ret;
        }

        buf.clear();
    }

    return ERR_SUCCESS;
}

UINT8 OZMHelper::OZMCreate(QString input, QString output, QString inputFFS, QString inputKext, QString inputDSDT)
{
    int i;
    UINT8 ret;
    QString guid, outputFilePath;
    QByteArray bios, dsdt, ffs, out;
    QByteArray amiboard, patchedAmiboard;
    QFileInfo biosfile(input);
    QModelIndex amiFileIdx, amiSectionIdx, volumeIdxCount, currIdx;

    QDirIterator diFFS(inputFFS);
    QList<kextEntry> kextList;

    BOOLEAN insertDSDT = FALSE;
    BOOLEAN insertKexts = FALSE;

    FFSUtil *fu = new FFSUtil();
    const static QModelIndex rootIndex = fu->getRootIndex();

    if (!dirExists(inputFFS)) {
        return ERR_ITEM_NOT_FOUND;
    }

    if (!inputKext.isEmpty() && dirExists(inputKext))
        insertKexts = TRUE;

    if (!inputDSDT.isEmpty() && fileExists(inputDSDT)) {
        insertDSDT = TRUE;
    }

    ret = dirCreate(output);
    if (ret == ERR_DIR_CREATE) {
        return ret;
    }

    ret = fileOpen(input, bios);
    if (ret) {
        return ret;
    }

    ret = fu->parseBIOSFile(bios);
    if (ret) {
        return ret;
    }

    /* Needed here to know correct volume image where everything goes */
    ret = fu->findFileByGUID(rootIndex, amiBoardSection.GUID, amiFileIdx);
    if(ret)
        return ERR_ITEM_NOT_FOUND;

    ret = fu->findSectionByIndex(amiFileIdx, EFI_SECTION_PE32, amiSectionIdx);
    if(ret)
        return ERR_ITEM_NOT_FOUND;

    fu->getLastSibling(amiFileIdx, volumeIdxCount);

    /* ToDo: Implement this */
    if(insertDSDT) {
        ret = fileOpen(inputDSDT, dsdt);
        if (ret) {
            return ret;
        }

        if(!dsdt.startsWith("DSDT")) {
            printf("ERROR: Input DSDT doesn't contain valid header!\n");
            return ERR_INVALID_FILE;
        }

        ret = fu->dumpSectionByGUID(amiBoardSection.GUID, EFI_SECTION_PE32,
                                                amiboard, EXTRACT_MODE_BODY);
        if(ret)
            return ret;

        ret = dsdt2bios(amiboard, dsdt, patchedAmiboard);
        if(ret)
            return ret;

       ret = fu->replace(amiSectionIdx, patchedAmiboard, REPLACE_MODE_BODY);
       if(ret)
           return ERR_REPLACE;
    }

    while (diFFS.hasNext()) {
        ffs.clear();
        guid = "";
        currIdx = rootIndex; // reset to 0,0

        ret = fileOpen(diFFS.filePath(), ffs);
        if (ret)
            return ret;

        /* verify input file, guid is read without verification */

        ret = getGUIDfromFile(ffs, guid);
        if (ret)
            return ret;

        ret = fu->findFileByGUID(rootIndex, guid, currIdx);
        if (ret) {
            /* Not found, insert at end of volume */
            ret = fu->insert(volumeIdxCount, ffs, CREATE_MODE_AFTER);
            if(ret)
                return ret;
        }
        else {
           /* Found, replace at known index */
           printf("Warning: File already present -> Replacing!\n");
           ret = fu->replace(currIdx, ffs, REPLACE_MODE_AS_IS); // as-is for whole File
           if(ret)
               return ret;
        }
    }

    if(insertKexts) {
        ffs.clear();
        guid = "";
        currIdx = rootIndex; // reset to 0,0

        ret = parseKextDirectory(inputKext, kextList);
        if (ret)
            return ret;

        for(i=0; i<kextList.size(); i++) {
            ret = convertKexts(kextList.at(i), ffs);
            if (ret)
                return ret;

            /* No need to verify, convertKexts returned fine */

            ret = getGUIDfromFile(ffs, guid);
            if (ret)
                return ret;

            ret = fu->findFileByGUID(rootIndex, guid, currIdx);
            if (ret) {
                /* Not found, insert at end of volume and increment Idx */
                ret = fu->insert(volumeIdxCount, ffs, CREATE_MODE_AFTER);
                if(ret)
                    return ret;
            }
            else {
               /* Found, replace at known index */
               printf("Warning: File already present -> Replacing!\n");
               ret = fu->replace(currIdx, ffs, REPLACE_MODE_AS_IS); // as-is for whole File
               if(ret)
                   return ret;
            }
        }
    }

    /* Congratz, we got that far :D */
    out.clear();
    ret = fu->reconstructImageFile(out);
    if(ret) {
        printf("ERROR: Image exploded.. please provide fewer files!\n");
        return ret;
    }

    outputFilePath = pathConcatenate(output,biosfile.fileName() + ".OZM");

    ret = fileWrite(outputFilePath, out);
    if (ret) {
        return ret;
    }

    return ERR_SUCCESS;
}

UINT8 OZMHelper::FFSConvert(QString input, QString output)
{
    UINT8 ret;
    QList<kextEntry> toConvert;
    QString filename;
    QByteArray out;

    if (!dirExists(input)) {
        return ERR_DIR_NOT_EXIST;
    }

    ret = dirCreate(output);
    if (ret == ERR_DIR_CREATE) {
        return ret;
    }

    ret = parseKextDirectory(input, toConvert);
    if (ret) {
        return ret;
    }

    for(int i=0; i < toConvert.size(); i++) {
        out.clear();

        ret = convertKexts(toConvert.at(i), out);
        if(ret)
            return ERR_ERROR;

        fileWrite(toConvert.at(i).filename, out);
        if(ret) {
            printf("ERROR: Saving '%s'\n", qPrintable(filename));
            return ERR_ERROR;
        }
    }

    return ERR_SUCCESS;
}

UINT8 OZMHelper::DSDT2Bios(QString input, QString inputDSDT, QString output)
{
    UINT8 ret;
    QString outputFile;
    QByteArray amiboardinfo;
    QByteArray dsdt;
    QByteArray out;

    if (!fileExists(input)) {
        return ERR_ITEM_NOT_FOUND;
    }

    if (!fileExists(inputDSDT)) {
        return ERR_ITEM_NOT_FOUND;
    }

    if (fileExists(output)) {
        printf("WARNING: Output file already exists! Overwriting it!\n");
    }

    ret = fileOpen(input, amiboardinfo);
    if (ret) {
        return ret;
    }

    ret = fileOpen(inputDSDT, dsdt);
    if (ret) {
        return ret;
    }

    ret = dsdt2bios(amiboardinfo, dsdt, out);
    if (ret) {
        return ret;
    }

    ret = fileWrite(output, out);
    if (ret) {
        return ret;
    }

    return ERR_SUCCESS;
}
