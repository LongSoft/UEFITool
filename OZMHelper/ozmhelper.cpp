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

static const QStringList ozmFFS{
    "DisablerKext", // optional
    "EnhancedFat", // optional
    "ExtFs", // optional
    "HermitShellX64", // optional
    "HfsPlus",
    "InjectorKext", // optional
    "Ozmosis",
    "OzmosisTheme", // optional, needed for Pos1-key bootmenu
    "OzmosisDefaults",
    "PartitionDxe", // optional - for nonEFI/MBR/Windows needed
    "SmcEmulatorKext"
};

static const QString DSDTFilename =  "DSDT.aml";
static const QString AmiBoardFilename = "AmiBoardInfo";

OZMHelper::OZMHelper(QObject *parent) :
    QObject(parent)
{
    wrapper = new Wrapper();
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
    UINT8 ret;
    QString ozmdefaults;
    QStringList kextList;
    QByteArray plist;
    QByteArray binary;
    QByteArray out;

    QString kextExtension = ".kext";
    QString ozmDefaultsFilename = "OzmosisDefaults.plist";
    QString ozmGUID = "99F2839C-57C3-411E-ABC3-ADE5267D960D";

    if (!wrapper->dirExists(input)) {
        return ERR_ITEM_NOT_FOUND;
    }

    if(wrapper->fileExists(wrapper->pathConcatenate(input, ozmDefaultsFilename))) {
        printf("Found %s\n", qPrintable(ozmDefaultsFilename));
        ozmdefaults = wrapper->pathConcatenate(input, ozmDefaultsFilename);
    }

    if(wrapper->getFolderListByExt(kextList, input, kextExtension)) {
        return ERR_INVALID_PARAMETER;
    }

    if(!ozmdefaults.isEmpty()) {
        binary.clear();
        ret = wrapper->fileOpen(ozmdefaults, binary);
        if(ret) {
            return ret;
        }
        ret = wrapper->kext2ffs(ozmDefaultsFilename, ozmGUID, NULL, binary, out);
        if(ret) {
            return ret;
        }
        ret = wrapper->fileWrite("OzmosisDefaults.ffs", out);
        if(ret) {
            return ret;
        }
        printf("%s written successfully!", "OzmosisDefaults.ffs");
    }

    printf("Validating %i kext folder/s...\n", kextList.size());

    for(int i=0; i < kextList.size(); i++) {
        printf("%s\t\t\t\t",qPrintable(kextList.at(i)));
        if(!wrapper->isValidKextDir(kextList.at(i))) {
            printf("Invalid!\n");
            return ERR_ITEM_NOT_FOUND;
        }
        else
            printf("OK\n");
    }

    ret = wrapper->dirCreate(output);
    if (ret == ERR_DIR_CREATE) {
        return ret;
    }

    /* ToDo: Implement this */
    // Convert Kext to FFS
    for(int i=0; i < kextList.size(); i++) {
        ret = wrapper->fileOpen(wrapper->pathConcatenate(kextList.at(i),"Contents/MacOS/GenericUSBXHCI"), binary);
        if(ret)
                printf("Open failed: %s\n",wrapper->pathConcatenate(kextList.at(i),"Contents/MacOS/GenericUSBXHCI").toLocal8Bit().constData());
        ret = wrapper->fileOpen(wrapper->pathConcatenate(kextList.at(i),"Contents/Info.plist"), plist);
        if(ret)
                printf("Open failed: %s\n",wrapper->pathConcatenate(kextList.at(i),"Contents/Info.plist").toLocal8Bit().constData());

        ret = wrapper->kext2ffs("GenericUSBXHCI","DADE1006-1B31-4FE4-8557-26FCEFC78275", plist, binary, out);

        wrapper->fileWrite("GenericUSBXHCI.ffs",out);
    }

    return ERR_SUCCESS;
}
