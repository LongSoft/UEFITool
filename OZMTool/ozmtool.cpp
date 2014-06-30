/* ozmtool.cpp

Copyright (c) 2014, tuxuser. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#include <QDirIterator>
#include "dsdt2bios/Dsdt2Bios.h"
#include "../ffs.h"
#include "ffsutil.h"
#include "ozmtool.h"
#include "common.h"

static QList<sectionEntry> deleteFfs;
static QList<sectionEntry> compressFfs;
static QList<sectionEntry> OzmFfs;

OZMTool::OZMTool(QObject *parent) :
    QObject(parent)
{
    /* how to store those initially */
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
}

OZMTool::~OZMTool()
{
}

UINT8 OZMTool::DSDTExtract(QString inputfile, QString outputdir)
{
    UINT8 ret;
    QString outputFile;
    QByteArray buf, dsdtbuf;
    UINT32 start, size;

    FFSUtil *fu = new FFSUtil();
    Dsdt2Bios d2b;

    ret = dirCreate(outputdir);
    if (ret == ERR_DIR_CREATE) {
        printf("ERROR: Creating output dir failed!\n");
        return ret;
    }

    ret = fileOpen(inputfile, buf);
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

    printf("* Dumping AmiBoardInfo from BIOS...\n");
    ret = fu->dumpSectionByGUID(amiBoardSection.GUID, EFI_SECTION_PE32, buf, EXTRACT_MODE_BODY);
    if (ret) {
        printf("ERROR: Dumping AmiBoardInfo failed!\n");
        return ret;
    }

    printf("* Extracting DSDT from AmiBoardInfo...\n");
    ret = d2b.getDSDTFromAmi(buf, start, size);
    if(ret) {
        printf("ERROR: Extracting DSDT from AmiBoardInfo failed!\n");
        return ret;
    }

    outputFile = pathConcatenate(outputdir, amiBoardSection.name + ".bin");

    printf("* Writing DSDT and AmiBoardInfo to files...\n");
    ret = fileWrite(outputFile, buf);
    if (ret) {
        printf("ERROR: Writing AmiBoardInfo.bin to '%s' failed!\n", qPrintable(outputFile));
        return ret;
    }

    outputFile = pathConcatenate(outputdir, DSDTFilename);

    /* Safety - sometimes it needs 1 byte more, different from header description! */
    dsdtbuf = buf.mid(start, size + 1);

    ret = fileWrite(outputFile, dsdtbuf);
    if (ret) {
        printf("ERROR: Writing DSDT.aml to '%s' failed!\n", qPrintable(outputFile));
        return ret;
    }

    buf.clear();

    return ERR_SUCCESS;
}

UINT8 OZMTool::DSDTInject(QString inputfile, QString dsdtfile, QString outputfile)
{
    UINT8 ret;
    QByteArray buf, dsdtbuf, out;

    FFSUtil *fu = new FFSUtil();

    printf("Patching BIOS with supplied DSDT...\n");

    ret = fileOpen(dsdtfile, dsdtbuf);
    if (ret) {
        printf("ERROR: Opening DSDT failed!\n");
        return ret;
    }

    ret = fileOpen(inputfile, buf);
    if (ret) {
        printf("ERROR: Opening BIOS failed!\n");
        return ret;
    }

    ret = fu->parseBIOSFile(buf);
    if (ret) {
        printf("ERROR: Parsing BIOS failed!\n");
        return ret;
    }

    ret = fu->injectDSDT(dsdtbuf);
    if (ret) {
        printf("ERROR: Injecting DSDT failed!\n");
        return ret;
    }

    printf("* Reconstructing the BIOS image...\n");
    ret = fu->reconstructImageFile(out);
    if (ret) {
        printf("ERROR: Reconstructing the BIOS image failed!\n");
        return ERR_ERROR;
    }

    printf("* Writing patched BIOS to '%s'...\n", qPrintable(outputfile));
    ret = fileWrite(outputfile, out);
    if (ret) {
        printf("ERROR: Writing patched BIOS to '%s' failed!\n", qPrintable(outputfile));
        return ret;
    }

    buf.clear();

    return ERR_SUCCESS;
}


UINT8 OZMTool::OZMUpdate(QString inputfile, QString recentBios, QString outputfile, int aggressivity)
{
    int i;
    UINT8 ret;
    QString guid;
    QByteArray oldBIOS;
    QByteArray newBIOS;
    QByteArray ffsbuf;
    QByteArray out;
    QModelIndex volumeIdx, currIdx;

    FFSUtil *oFU = new FFSUtil();
    FFSUtil *nFU = new FFSUtil();

    ret = fileOpen(inputfile, oldBIOS);
    if (ret) {
        printf("ERROR: Opening '%s' failed!\n", qPrintable(inputfile));
        return ret;
    }

    ret = fileOpen(recentBios, newBIOS);
    if (ret) {
        printf("ERROR: Opening '%s' failed!\n", qPrintable(recentBios));
        return ret;
    }

    ret = checkAggressivityLevel(aggressivity);
    if(ret) {
        printf("Warning: Resetting aggressivity level to 'None' !");
        aggressivity = 0;
    }

    ret = oFU->parseBIOSFile(oldBIOS);
    if (ret) {
        printf("ERROR: Parsing old BIOS failed!\n");
        return ret;
    }

    ret = nFU->parseBIOSFile(newBIOS);
    if (ret) {
        printf("ERROR: Parsing old BIOS failed!\n");
        return ret;
    }

    ret = nFU->getLastVolumeIndex(volumeIdx);
    if (ret) {
        printf("ERROR: Failed to get Volume Index!\n");
        return ret;
    }

    for(i = 0; i < OzmFfs.size(); i++){
        ffsbuf.clear();

        ret = oFU->dumpFileByGUID(OzmFfs.at(i).GUID, ffsbuf, EXTRACT_MODE_AS_IS);
        if(ret && OzmFfs.at(i).required) {
            printf("ERROR: Required file '%s' [%s] not found!\n", qPrintable(OzmFfs.at(i).name), qPrintable(OzmFfs.at(i).GUID));
            return ERR_FILE_NOT_FOUND;
        }
        else if(ret)
            continue;

        printf("Injecting '%s' [%s] from old into new BIOS...\n", qPrintable(OzmFfs.at(i).name), qPrintable(OzmFfs.at(i).GUID));

        ret = nFU->injectFile(ffsbuf);
        if (ret) {
            printf("ERROR: Injection failed!\n");
            return ERR_ERROR;
        }
    }

    for(i = MIN_KEXT_ID; i <= MAX_KEXT_ID; i++) {
        ffsbuf.clear();

        guid = kextGUID.arg(i, 0, 16);

        ret = oFU->dumpFileByGUID(guid, ffsbuf, EXTRACT_MODE_AS_IS);
        if(ret)
            continue; // Not found

        printf("Injecting custom ffs [%s] from old into new BIOS...\n", qPrintable(guid));

        ret = nFU->injectFile(ffsbuf);
        if (ret) {
            printf("ERROR: Injection failed!\n");
            return ERR_ERROR;
        }
    }

    ret = nFU->runFreeSomeSpace(aggressivity);
    if (ret) {
        printf("ERROR: Freeing space failed!\n");
        return ERR_ERROR;
    }

    out.clear();
    printf("Reconstructing final image...\n");
    ret = nFU->reconstructImageFile(out);
    if(ret) {
        printf("ERROR: Image exploded...\n");
        return ERR_VOLUME_GROW_FAILED;
    }

    printf("* Image built successfully!\n");

    ret = fileWrite(outputfile, out);
    if (ret) {
        printf("ERROR: Writing updated BIOS to '%s' failed!\n", qPrintable(outputfile));
        return ret;
    }
    printf("Bios successfully saved to '%s'\n",qPrintable(outputfile));

    return ERR_SUCCESS;
}

UINT8 OZMTool::OZMExtract(QString inputfile, QString outputdir)
{
    int i;
    UINT8 ret;
    QString guid, name;
    QString outputFile;
    QByteArray buf;

    FFSUtil *fu = new FFSUtil();

    ret = fileOpen(inputfile, buf);
    if (ret) {
        printf("ERROR: Opening '%s' failed!\n", qPrintable(inputfile));
        return ret;
    }

    ret = fu->parseBIOSFile(buf);
    if (ret) {
        printf("ERROR: Parsing BIOS failed!\n");
        return ret;
    }

    buf.clear();

    ret = dirCreate(outputdir);
    if (ret == ERR_DIR_CREATE) {
        printf("ERROR: Creating output directory failed!\n");
        return ret;
    }

    for(i=0; i<OzmFfs.size(); i++) {
        ret = fu->dumpFileByGUID(OzmFfs.at(i).GUID, buf, EXTRACT_MODE_AS_IS);
        if (ret == ERR_ITEM_NOT_FOUND) {
            printf("Warning: File '%s' [%s] wasn't found!\n", qPrintable(OzmFfs.at(i).name), qPrintable(OzmFfs.at(i).GUID));
            continue;
        }
        if (ret) {
            printf("ERROR: Dumping '%s' [%s] failed!\n", qPrintable(OzmFfs.at(i).name), qPrintable(OzmFfs.at(i).GUID));
            return ret;
        }

        outputFile = pathConcatenate(outputdir,(OzmFfs.at(i).name+".ffs"));

        ret = fileWrite(outputFile, buf);
        if (ret) {
            printf("ERROR: Saving '%s' failed!\n", qPrintable(outputFile));
            return ret;
        }

        printf("* '%s' [%s] extracted & saved\n", qPrintable(OzmFfs.at(i).name), qPrintable(OzmFfs.at(i).GUID));
        buf.clear();
    }

    printf("* Checking for optional Kexts...\n");
    for(i=MIN_KEXT_ID; i<= MAX_KEXT_ID; i++) {
        buf.clear();

        guid = kextGUID.arg(i, 0, 16);

        ret = fu->dumpFileByGUID(guid, buf, EXTRACT_MODE_AS_IS);
        if(ret)
            continue;

        ret = fu->getNameByGUID(guid, name);
        if(ret) {
            printf("ERROR: Failed to get text of section [%s]\n", qPrintable(guid));
            continue;
        }

        outputFile = pathConcatenate(outputdir,(name+".ffs"));

        ret = fileWrite(outputFile, buf);
        if (ret) {
            printf("ERROR: Saving '%s' failed!\n", qPrintable(outputFile));
            return ret;
        }

        printf("* '%s' [%s] extracted & saved\n", qPrintable(name), qPrintable(guid));
    }

    return ERR_SUCCESS;
}

UINT8 OZMTool::OZMCreate(QString inputfile, QString outputfile, QString inputFFSdir, QString inputKextdir, QString inputDSDTfile, int aggressivity)
{
    int i, kextId;
    UINT8 ret;
    QByteArray bios, dsdt, ffs, out;
    QModelIndex volumeIdxCount;

    BOOLEAN insertDSDT = FALSE;
    BOOLEAN insertKexts = FALSE;

    FFSUtil *fu = new FFSUtil();

    if (!dirExists(inputFFSdir)) {
        printf("ERROR: FFS directory '%s' couldn't be found!\n", qPrintable(inputFFSdir));
        return ERR_ITEM_NOT_FOUND;
    }

    if (!inputKextdir.isEmpty() && dirExists(inputKextdir))
        insertKexts = TRUE;
    else
        printf("Warning: No KEXT-dir given! Injecting only Ozmosis files!\n");

    if (!inputDSDTfile.isEmpty() && fileExists(inputDSDTfile))
        insertDSDT = TRUE;
    else
        printf("Warning: No DSDT file given! Will leave DSDT as-is!\n");

    ret = checkAggressivityLevel(aggressivity);
    if(ret) {
        printf("Warning: Resetting aggressivity level to 'None' !");
        aggressivity = 0;
    }

    ret = fileOpen(inputfile, bios);
    if (ret) {
        printf("ERROR: Opening '%s' failed!\n", qPrintable(inputfile));
        return ret;
    }

    ret = fu->parseBIOSFile(bios);
    if (ret) {
        printf("ERROR: Parsing BIOS failed!\n");
        return ret;
    }

    ret = fu->getLastVolumeIndex(volumeIdxCount);
    if (ret) {
        printf("ERROR: Failed to get last Volume Index!\n");
        return ret;
    }

    if(insertDSDT) {
        printf("Inserting supplied DSDT into image...\n");

        ret = fileOpen(inputDSDTfile, dsdt);
        if (ret) {
            printf("ERROR: Opening DSDT '%s' failed!\n", qPrintable(inputDSDTfile));
            return ret;
        }

        ret = fu->injectDSDT(dsdt);
        if (ret) {
            printf("ERROR: Failed to inject DSDT!\n");
            return ret;
        }
    }

    printf("Injecting FFS into BIOS...\n");
    QDirIterator diFFS(inputFFSdir);
    QFileInfo currFFS;

    while (diFFS.hasNext()) {
        ffs.clear();
        currFFS = diFFS.next();

        if(!currFFS.fileName().endsWith(".ffs"))
            continue;

        ret = fileOpen(currFFS.filePath(), ffs);
        if (ret) {
            printf("ERROR: Opening '%s' failed!\n", qPrintable(diFFS.filePath()));
            return ret;
        }

        ret = fu->injectFile(ffs);
        if (ret) {
            printf("ERROR: Injection of file '%s' failed!\n", qPrintable(currFFS.fileName()));
            return ERR_ERROR;
        }
    }

    QDirIterator diKext(inputKextdir);
    QFileInfo currKext;

    if(insertKexts) {

        printf("Converting Kext & injecting into BIOS...\n");

        kextId = MIN_KEXT_ID;

        while (diKext.hasNext()) {
            ffs.clear();
            currKext = diKext.next();

            if(!currKext.fileName().compare("FakeSMC.kext")){
                ret = convertKext(currKext.filePath(), 1, ffs);
            } else if(!currKext.fileName().compare("Disabler.kext")){
                ret = convertKext(currKext.filePath(), 2, ffs);
            } else if(!currKext.fileName().compare("Injector.kext")){
                ret = convertKext(currKext.filePath(), 3, ffs);
            } else if(!currKext.fileName().compare(ozmDefaultsFilename)){
                ret = convertOzmPlist(currKext.filePath(), ffs);
            } else if(currKext.fileName().endsWith(".kext")){
                ret = convertKext(currKext.filePath(), kextId, out);
                kextId++;
            } else
                continue;

            printf("* Current file '%s'...\n", qPrintable(currKext.fileName()));

            if(ret) {
                printf("ERROR: Converting '%s' to FFS failed!\n", qPrintable(currKext.fileName()));
                return ERR_ERROR;
            }

            fu->injectFile(ffs);
            if (ret) {
                printf("ERROR: Injection of file '%s' failed!\n", qPrintable(currKext.fileName()));
                return ret;
            }
        }
    }

    ret = fu->runFreeSomeSpace(aggressivity);
    if (ret) {
        printf("ERROR: Freeing space failed!\n");
        return ERR_ERROR;
    }

    out.clear();
    printf("Reconstructing final image...\n");
    ret = fu->reconstructImageFile(out);
    if(ret) {
        printf("ERROR: Image exploded...\n");
        return ERR_VOLUME_GROW_FAILED;
    }

    printf("* Image built successfully!\n");

    ret = fileWrite(outputfile, out);
    if (ret) {
        printf("ERROR: Writing patched BIOS to '%s' failed!\n", qPrintable(outputfile));
        return ret;
    }
    printf("Bios successfully saved to '%s'\n\n",qPrintable(outputfile));

    printf("Starting verification... if you see any unusual warnings/errors -> DONT USE THE IMAGE!\n");
    printf("NOTE: You are using this application on your own risk anyway..\n");
    printf("NOTE: 'parseInputFile: descriptor parsing failed, descriptor region has intersection with BIOS region' can be ignored..\n\n");

    FFSUtil *verifyFU = new FFSUtil();
    ret = verifyFU->parseBIOSFile(out);
    if(ret) {
        printf("ERROR: Parsing BIOS failed!\n");
        return ret;
    }

    for(i=0; i<OzmFfs.size(); i++) {
        ffs.clear();
        if(OzmFfs.at(i).required) {
            ret = verifyFU->dumpFileByGUID(OzmFfs.at(i).GUID, ffs, EXTRACT_MODE_AS_IS);
            if(ret)
                printf("ERROR: Failed to dump '%s' [%s] !\n",qPrintable(OzmFfs.at(i).name), qPrintable(OzmFfs.at(i).GUID));
        }
    }

    return ERR_SUCCESS;
}

UINT8 OZMTool::FFSConvert(QString inputdir, QString outputdir)
{
    UINT8 ret;
    int kextId;
    QString filepath;
    QByteArray out, compressedOut;

    FFSUtil *fu = new FFSUtil();

    if (!dirExists(inputdir)) {
        printf("ERROR: Input directory '%s' doesn't exist!\n", qPrintable(inputdir));
        return ERR_DIR_NOT_EXIST;
    }

    ret = dirCreate(outputdir);
    if (ret == ERR_DIR_CREATE) {
        printf("ERROR: Creating output directory failed!\n");
        return ret;
    }

    QDirIterator diKext(inputdir);
    QFileInfo currKext;

    printf("Converting Kexts...\n");

    kextId = MIN_KEXT_ID;

    while (diKext.hasNext()) {
        out.clear();
        compressedOut.clear();
        currKext = diKext.next();

        if(!currKext.fileName().compare("FakeSMC.kext")){
            ret = convertKext(currKext.filePath(), 1, out);
        } else if(!currKext.fileName().compare("Disabler.kext")){
            ret = convertKext(currKext.filePath(), 2, out);
        } else if(!currKext.fileName().compare("Injector.kext")){
            ret = convertKext(currKext.filePath(), 3, out);
        } else if(!currKext.fileName().compare(ozmDefaultsFilename)){
            ret = convertOzmPlist(currKext.filePath(), out);
        } else if(currKext.fileName().endsWith(".kext")){
            ret = convertKext(currKext.filePath(), kextId, out);
            kextId++;
        } else
            continue;

        printf("* Current file '%s'...\n", qPrintable(currKext.fileName()));

        if(ret) {
            printf("ERROR: Converting '%s' to FFS failed!\n", qPrintable(currKext.fileName()));
            return ERR_ERROR;
        }

        printf("* Compressing it...\n");
        ret = fu->compress(out, COMPRESSION_ALGORITHM_EFI11, compressedOut);
        if(ret) {
            printf("ERROR: Compression of FFS failed!\n");
            return ERR_ERROR;
        }

        filepath = pathConcatenate(outputdir, currKext.fileName() + ".ffs");
        fileWrite(filepath, out);
        if(ret) {
            printf("ERROR: Saving '%s' failed!\n", qPrintable(filepath));
            return ERR_ERROR;
        }

        filepath = pathConcatenate(outputdir, currKext.fileName() + ".ffs.compressed");
        fileWrite(filepath, compressedOut);
        if(ret) {
            printf("ERROR: Saving '%s' failed!\n", qPrintable(filepath));
            return ERR_ERROR;
        }

    }

    return ERR_SUCCESS;
}

UINT8 OZMTool::DSDT2Bios(QString inputfile, QString inputDSDTfile, QString outputfile)
{
    UINT8 ret;
    QByteArray amiboardinfo;
    QByteArray dsdt;
    QByteArray out;

    if (fileExists(outputfile)) {
        printf("ERROR: Output file already exists!\n");
        return ERR_FILE_EXISTS;
    }

    ret = fileOpen(inputfile, amiboardinfo);
    if (ret) {
        printf("ERROR: Opening '%s' failed!\n", qPrintable(inputfile));
        return ret;
    }

    ret = fileOpen(inputDSDTfile, dsdt);
    if (ret) {
        printf("ERROR: Opening '%s' failed!\n", qPrintable(inputDSDTfile));
        return ret;
    }

    printf("Attempting to patch DSDT into AmiBoardInfo\n");

    ret = dsdt2bios(amiboardinfo, dsdt, out);
    if (ret) {
        printf("ERROR: Failed to inject DSDT into AmiBoardInfo!\n");
        return ret;
    }

    ret = fileWrite(outputfile, out);
    if (ret) {
        printf("ERROR: Saving to '%s' failed!\n", qPrintable(outputfile));
        return ret;
    }

    printf("* Success!\n");

    return ERR_SUCCESS;
}
