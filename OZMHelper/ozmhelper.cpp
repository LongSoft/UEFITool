/* ozmhelper.cpp

Copyright (c) 2014, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#include "ozmhelper.h"

static const QString DSDTFilename =  "DSDT.aml";
static const QString AmiBoardFilename = "AmiBoardInfo";

OZMHelper::OZMHelper(QObject *parent) :
    QObject(parent)
{
    wrapper = new Wrapper();
    ozmFFS.append("DisablerKext"); //optional
    ozmFFS.append("EnhancedFat"); // optional
    ozmFFS.append("ExtFs"); // optional
    ozmFFS.append("HermitShellX64"); // optional
    ozmFFS.append("HfsPlus");
    ozmFFS.append("InjectorKext"); // optional
    ozmFFS.append("Ozmosis");
    ozmFFS.append("OzmosisTheme"); // optional, needed for Pos1-key bootmenu
    ozmFFS.append("OzmosisDefaults");
    ozmFFS.append("PartitionDxe"); // optional - for nonEFI/MBR/Windows needed
    ozmFFS.append("SmcEmulatorKext");
}

OZMHelper::~OZMHelper()
{
    delete wrapper;
}

UINT8 OZMHelper::DSDTExtract(QString input, QString output)
{
    UINT8 ret;
    QString outputFile;
    QByteArray buf, dsdtbuf;

    ret = wrapper->dirCreate(output);
    if (ret == ERR_DIR_CREATE) {
        return ret;
    }

    ret = wrapper->fileOpen(input, buf);
    if (ret) {
        return ret;
    }

    ret = wrapper->parseBIOSFile(buf);
    if (ret) {
        return ret;
    }

    buf.clear();

    ret = wrapper->dumpSectionByName(AmiBoardFilename, buf, EXTRACT_MODE_BODY);
    if (ret) {
        return ret;
    }

    ret = wrapper->getDSDTfromAMI(buf, dsdtbuf);
    if (ret) {
        return ret;
    }

    outputFile = wrapper->pathConcatenate(output, AmiBoardFilename);

    ret = wrapper->fileWrite(outputFile, buf);
    if (ret) {
        return ret;
    }

    outputFile = wrapper->pathConcatenate(output, DSDTFilename);

    ret = wrapper->fileWrite(outputFile, dsdtbuf);
    if (ret) {
        return ret;
    }

    buf.clear();
    dsdtbuf.clear();

    return ERR_SUCCESS;
}

UINT8 OZMHelper::OZMExtract(QString input, QString output)
{
    int i;
    UINT8 ret;
    QString outputFile;
    QByteArray buf;

    ret = wrapper->fileOpen(input, buf);
    if (ret) {
        return ret;
    }

    ret = wrapper->parseBIOSFile(buf);
    if (ret) {
        return ret;
    }

    buf.clear();

    ret = wrapper->dirCreate(output);
    if (ret == ERR_DIR_CREATE) {
        return ret;
    }

    for(i=0; i<ozmFFS.size(); i++) {
        /* ToDo: RATHER LOOK FOR GUID ? */
        ret = wrapper->dumpSectionByName(ozmFFS.at(i), buf, EXTRACT_MODE_AS_IS);
        if (ret == ERR_ITEM_NOT_FOUND)
            continue;
        if (ret) {
            return ret;
        }

        outputFile = wrapper->pathConcatenate(output,(ozmFFS.at(i)+".ffs"));

        ret = wrapper->fileWrite(outputFile, buf);
        if (ret) {
            return ret;
        }

        buf.clear();
    }

    return ERR_SUCCESS;
}

UINT8 OZMHelper::OZMCreate(QString input, QString output, QString inputFFS, QString inputKext, QString inputDSDT)
{
    UINT8 ret;
    QByteArray buf;
    QString outputFile;

    if (!wrapper->dirExists(inputFFS)) {
        return ERR_ITEM_NOT_FOUND;
    }

    if (!inputKext.isEmpty() && !wrapper->dirExists(inputKext)) {
        return ERR_ITEM_NOT_FOUND;
    }

    if (!wrapper->fileExists(inputDSDT)) {
        return ERR_ITEM_NOT_FOUND;
    }
    /* ToDo: validate DSDT */

    ret = wrapper->fileOpen(input, buf);
    if (ret) {
        return ret;
    }

    ret = wrapper->parseBIOSFile(buf);
    if (ret) {
        return ret;
    }

    buf.clear();

    ret = wrapper->dirCreate(output);
    if (ret == ERR_DIR_CREATE) {
        return ret;
    }

    /* ToDo: Implement this */
    buf.append("OZM");

    outputFile = wrapper->pathConcatenate(output,wrapper->getDateTime() + ".OZM");

    ret = wrapper->fileWrite(outputFile, buf);
    if (ret) {
        return ret;
    }

    return ERR_SUCCESS;
}

UINT8 OZMHelper::FFSConvert(QString input, QString output)
{
    UINT8 nullterminator = 0;
    UINT8 ret;
    UINT32 GUIDindex;
    UINT32 GUIDindexCount;
    QByteArray plist;
    QByteArray plistBinary;
    QByteArray binary;
    QByteArray inputBinary;
    QByteArray out;
    QList<kextEntry> toConvert;
    kextEntry mKextEntry;
    QString basename;
    QString sectionName;
    QString filename;
    QDir dir;

    QFileInfo binaryPath;
    QFileInfo plistPath;
    QDirIterator di(input);

    const static QString kextExtension = ".kext";
    const static QString ozmDefaultsFilename = "OzmosisDefaults.plist";
    const static QString ozmPlistGUID = "99F2839C-57C3-411E-ABC3-ADE5267D960D";
    const static QString ozmPlistPath = wrapper->pathConcatenate(input, ozmDefaultsFilename);
    const static QString kextGUID = "DADE100%1-1B31-4FE4-8557-26FCEFC78275"; // %1 placeholder

    GUIDindexCount = 6;

    if (!wrapper->dirExists(input)) {
        return ERR_DIR_NOT_EXIST;
    }

    ret = wrapper->dirCreate(output);
    if (ret == ERR_DIR_CREATE) {
        return ret;
    }

    if(wrapper->fileExists(ozmPlistPath)) {
        printf("Found %s\n", qPrintable(ozmDefaultsFilename));

        mKextEntry.basename = "OzmosisDefaults";
        mKextEntry.binaryPath = ozmPlistPath; // yes, Plist handled as binary
        mKextEntry.plistPath = "";
        mKextEntry.GUID = ozmPlistGUID;

        toConvert.append(mKextEntry);
    }

    // Check all folders in input-dir
    while (di.hasNext()) {

        if(!di.next().endsWith(kextExtension))
            continue;

        basename = di.fileName().left(di.fileName().size()-kextExtension.size());

        dir.setPath(di.filePath());
        dir = dir.filePath("Contents");
        plistPath.setFile(dir,"Info.plist");
        dir = dir.filePath("MacOS");
        binaryPath.setFile(dir, basename);

        if (!dir.exists()) {
            printf("ERROR: Kext-dir invalid: %s/Contents/MacOS/ missing!\n", qPrintable(di.fileName()));
            return STATUS_ERROR;
        }

        if (!plistPath.exists()) {
            printf("ERROR: Kext-dir invalid: %s/Contents/Info.plist missing!\n", qPrintable(di.fileName()));
            return STATUS_ERROR;
        }

        if (!binaryPath.exists()) {
            printf("ERROR: Kext-dir invalid: %s/Contents/MacOS/%s missing!\n",
                                                    qPrintable(di.fileName()), qPrintable(basename));
            return STATUS_ERROR;
        }

        if(GUIDindexCount > 0xF) {
            printf("ERROR: Reached maximum Kext-Count! Ignoring the rest...\n");
            break;
        }

        if(!di.fileName().compare("FakeSMC.kext"))
            GUIDindex = 1;
        else if(!di.fileName().compare("Disabler.kext"))
            GUIDindex = 2;
        else if(!di.fileName().compare("Injector.kext"))
            GUIDindex = 3;
        else
            GUIDindex = GUIDindexCount++;

        mKextEntry.basename = basename;
        mKextEntry.binaryPath = binaryPath.filePath();
        mKextEntry.plistPath = plistPath.filePath();
        mKextEntry.GUID = kextGUID.arg(GUIDindex, 0, 16);
        toConvert.append(mKextEntry);
    }


    // Convert Kext to FFS
    for(int i=0; i < toConvert.size(); i++) {
        inputBinary.clear();

        if(toConvert.at(i).GUID.compare(ozmPlistGUID)) { // If NOT Ozmosis => check plist path
            ret = wrapper->fileOpen(toConvert.at(i).plistPath, plist);
            if(ret) {
                printf("ERROR: Open failed: %s\n", qPrintable(toConvert.at(i).plistPath));
                return STATUS_ERROR;
            }

            ret = wrapper->getInfoFromPlist(plist, sectionName, plistBinary);
            if(ret) {
                printf("ERROR: Failed to get values/convert Info.plist\n");
                return STATUS_ERROR;
            }

            inputBinary.append(plistBinary);
            inputBinary.append(nullterminator);
        }
        else {
            sectionName = toConvert.at(i).basename;
        }

        ret = wrapper->fileOpen(toConvert.at(i).binaryPath, binary);
        if(ret) {
            printf("ERROR: Open failed: %s\n", qPrintable(toConvert.at(i).binaryPath));
            return STATUS_ERROR;
        }
        inputBinary.append(binary);

        out.clear();
        ret = wrapper->kext2ffs(sectionName, toConvert.at(i).GUID, inputBinary, out);
        if(ret) {
            printf("ERROR: KEXT2FFS failed on '%s'\n", qPrintable(toConvert.at(i).basename));
            return STATUS_ERROR;
        }

        filename = QString("%1.ffs").arg(toConvert.at(i).basename);

        wrapper->fileWrite(filename, out);
        if(ret) {
            printf("ERROR: Saving '%s'\n", qPrintable(filename));
            return STATUS_ERROR;
        }
    }

    return ERR_SUCCESS;
}
