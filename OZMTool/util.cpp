/* util.cpp

Copyright (c) 2014, tuxuser. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#include <QtEndian>
#include <QDirIterator>
#include <QDateTime>
#include <QUuid>
#include <plist/Plist.hpp>
#include "ffs/kextconvert.h"
#include "dsdt2bios/Dsdt2Bios.h"
#include "../ffs.h"
#include "util.h"

/* General stuff */

UINT8 fileOpen(QString path, QByteArray & buf)
{
    QFileInfo fileInfo(path);

    if (!fileInfo.exists())
        return ERR_FILE_NOT_FOUND;

    QFile inputFile;
    inputFile.setFileName(path);

    if (!inputFile.open(QFile::ReadOnly))
        return ERR_FILE_OPEN;

    buf.clear();

    buf.append(inputFile.readAll());
    inputFile.close();

    return ERR_SUCCESS;
}

UINT8 fileWrite(QString path, QByteArray & buf)
{
    QFileInfo fileInfo(path);

    if (fileInfo.exists())
        printf("Warning: File already exists! Overwriting it...\n");

    QFile writeFile;
    writeFile.setFileName(path);

    if (!writeFile.open(QFile::WriteOnly))
        return ERR_FILE_OPEN;

    if(writeFile.write(buf) != buf.size())
        return ERR_FILE_WRITE;

    writeFile.close();

    return ERR_SUCCESS;
}

BOOLEAN fileExists(QString path)
{
    QFileInfo fileInfo = QFileInfo(path);

    return fileInfo.exists();
}

UINT8 dirCreate(QString path)
{
    QDir dir;
    if (dir.cd(path))
        return ERR_DIR_ALREADY_EXIST;

    if (!dir.mkpath(path))
        return ERR_DIR_CREATE;

    return ERR_SUCCESS;
}

BOOLEAN dirExists(QString path)
{
    QDir dir(path);

    return dir.exists();
}

QString pathConcatenate(QString path, QString filename)
{
    return QDir(path).filePath(filename);
}

UINT32 getDateTime()
{
    QDateTime dateTime = QDateTime::currentDateTime();
    return dateTime.toTime_t();
}

UINT16 getUInt16(QByteArray & buf, UINT32 start, bool fromBE)
{
    UINT16 tmp = 0;

    tmp = (tmp << 8) + buf.at(start+0);
    tmp = (tmp << 8) + buf.at(start+1);

    if(fromBE)
        return qFromBigEndian(tmp);
    else
        return tmp;
}

UINT32 getUInt32(QByteArray & buf, UINT32 start, bool fromBE)
{
    UINT32 tmp = 0;

    tmp = (tmp << 8) + buf.at(start+0);
    tmp = (tmp << 8) + buf.at(start+1);
    tmp = (tmp << 8) + buf.at(start+2);
    tmp = (tmp << 8) + buf.at(start+3);

    if(fromBE)
        return qFromBigEndian(tmp);
    else
        return tmp;
}

/* Specific stuff */

UINT8 getGUIDfromFile(QByteArray object, QString & name)
{
    QByteArray header;
    EFI_GUID* guid;
    header = object.left(sizeof(EFI_GUID));
    guid = (EFI_GUID*)(header.constData());

    // Get info
    name = guidToQString(*guid);
    return ERR_SUCCESS;
}

UINT8 plistReadExecName(QByteArray plist, QString & name)
{
    static const std::string execIdentifier = "CFBundleExecutable";

    QString plistExec;

    std::map<std::string, boost::any> dict;
    Plist::readPlist(plist.data(), plist.size(), dict);

    if(dict.count(execIdentifier) > 0)
        plistExec = boost::any_cast<const std::string&>(dict.find(execIdentifier)->second).c_str();

    if(plistExec.isEmpty()) {
        printf("ERROR: CFBundleName in plist is blank. Aborting!\n");
        return ERR_ERROR;
    }

    name = plistExec;

    return ERR_SUCCESS;
}

UINT8 plistReadBundlenameAndVersion(QByteArray plist, QString & name, QString & version)
{
    static const std::string nameIdentifier = "CFBundleName";
    static const std::string versionIdentifier = "CFBundleShortVersionString";

    QString plistName;
    QString plistVersion;

    std::map<std::string, boost::any> dict;
    Plist::readPlist(plist.data(), plist.size(), dict);

    if(dict.count(nameIdentifier) > 0)
        plistName = boost::any_cast<const std::string&>(dict.find(nameIdentifier)->second).c_str();

    if(dict.count(versionIdentifier) > 0)
        plistVersion = boost::any_cast<const std::string&>(dict.find(versionIdentifier)->second).c_str();

    if(plistName.isEmpty()) {
        printf("ERROR: CFBundleName in plist is blank. Aborting!\n");
        return ERR_ERROR;
    }

    name = plistName;
    version = plistVersion;

    return ERR_SUCCESS;
}

UINT8 plistWriteNewBasename(QByteArray plist, QString newName, QByteArray & out)
{
    std::vector<char> data;
    static const std::string nameIdentifier = "CFBundleName";

    QString plistName;

    std::map<std::string, boost::any> dict;
    Plist::readPlist(plist.data(), plist.size(), dict);

    if(dict.count(nameIdentifier) > 0)
        plistName = boost::any_cast<const std::string&>(dict.find(nameIdentifier)->second).c_str();

    if(plistName.isEmpty()) {
        printf("ERROR: CFBundleName in plist is blank, so cannot be modified. Aborting!\n");
        return ERR_ERROR;
    }

    // Assign new value for CFBundleName
    dict[nameIdentifier] = std::string(newName.toUtf8().constData());

    Plist::writePlistXML(data, dict);

    out.clear();
    out.append(data.data(), data.size());

    return ERR_SUCCESS;
}

UINT8 checkAggressivityLevel(int aggressivity) {
    QString level;

    switch(aggressivity) {
    case RUN_AS_IS:
        level = "Do nothing - Inject as-is";
        break;
    case RUN_COMPRESS:
         level = "Compress CORE_DXE";
         break;
    case RUN_DELETE:
         level = "Delete network stuff from BIOS";
         break;
    case RUN_DEL_OZM_NREQ:
         level = "Delete non-required Ozmosis files";
         break;
    default:
        printf("Warning: Invalid aggressivity level set!\n");
        return ERR_ERROR;
    }

    printf("Info: Aggressivity level set to '%s'...\n", qPrintable(level));
    return ERR_SUCCESS;
}

UINT8 convertOzmPlist(QString input, QByteArray & out)
{
    UINT8 ret;
    QByteArray plist;

    KextConvert *kext = new KextConvert();

    ret = fileOpen(input, plist);
    if(ret) {
        printf("ERROR: Open failed: %s\n", qPrintable(input));
        return ERR_ERROR;
    }

    ret = kext->createFFS(ozmSectionName, ozmPlistGUID, plist, out);
    if(ret) {
        printf("ERROR: KEXT2FFS failed on '%s'\n", qPrintable(ozmDefaultsFilename));
        return ERR_ERROR;
    }

    return ERR_SUCCESS;
}

UINT8 convertKext(QString input, int kextIndex, QByteArray & out)
{
    UINT8 ret;
    UINT8 nullterminator = 0;

    QString sectionName, guid;
    QString bundleName, bundleVersion, execName;
    QDir dir;

    QFileInfo binaryPath;
    QFileInfo plistPath;

    QByteArray plistbuf;
    QByteArray binarybuf;
    QByteArray toConvertBinary;

    KextConvert *kext = new KextConvert();

    // Check all folders in input-dir

    if(kextIndex > 0xF) {
        printf("ERROR: Invalid kextIndex '%i' supplied!\n", kextIndex);
        return ERR_ERROR;
    }

    dir.setPath(input);
    dir = dir.filePath("Contents");
    plistPath.setFile(dir,"Info.plist");
    dir = dir.filePath("MacOS");

    if (!dir.exists()) {
        printf("ERROR: Kext-dir invalid: */Contents/MacOS/ missing!\n");
        return ERR_ERROR;
    }

    if (!plistPath.exists()) {
        printf("ERROR: Kext-dir invalid: */Contents/Info.plist missing!\n");
        return ERR_ERROR;
    }

    ret = fileOpen(plistPath.filePath(), plistbuf);
    if(ret) {
        printf("ERROR: Opening '%s' failed!\n", qPrintable(plistPath.filePath()));
        return ret;
    }

    ret = plistReadExecName(plistbuf, execName);
    if(ret) {
        printf("ERROR: Failed to get executableName Info.plist\n");
        return ret;
    }

    binaryPath.setFile(dir, execName);

    if (!binaryPath.exists()) {
        printf("ERROR: Kext-dir invalid: */Contents/MacOS/%s missing!\n",
               qPrintable(execName));
        return ERR_ERROR;
    }

    ret = plistReadBundlenameAndVersion(plistbuf, bundleName, bundleVersion);
    if (ret) {
        printf("ERROR: Failed to get bundleName and/or version from Info.plist\n");
        return ret;
    }

    ret = fileOpen(binaryPath.filePath(), binarybuf);
    if (ret) {
        printf("ERROR: Opening '%s' failed!\n", qPrintable(binaryPath.filePath()));
        return ERR_ERROR;
    }

    if(bundleVersion.isEmpty())
        bundleVersion = "?";

    sectionName.sprintf("%s-%s",qPrintable(bundleName), qPrintable(bundleVersion));

    guid = kextGUID.arg(kextIndex, 0, 16);

    toConvertBinary.append(plistbuf);
    toConvertBinary.append(nullterminator);
    toConvertBinary.append(binarybuf);

//    ret = kext->createFFS(sectionName, guid, toConvertBinary, out);
    ret = customFFScreate(toConvertBinary, guid, sectionName, out);
    if(ret) {
        printf("ERROR: KEXT2FFS failed on '%s'\n", qPrintable(sectionName));
        return ERR_ERROR;
    }

    return ERR_SUCCESS;
}

UINT8 customFFScreate(QByteArray body, QString guid, QString sectionName, QByteArray & out)
{
    QByteArray bufSectionName;
    QByteArray fileBody, header;

    /* FFS PE32 Section */
    header.fill(0, sizeof(EFI_COMMON_SECTION_HEADER));
    EFI_COMMON_SECTION_HEADER* PE32SectionHeader = (EFI_COMMON_SECTION_HEADER*)header.data();

    uint32ToUint24(sizeof(EFI_COMMON_SECTION_HEADER)+body.size(), PE32SectionHeader->Size);
    PE32SectionHeader->Type = EFI_SECTION_PE32;

    fileBody.append(header, sizeof(EFI_COMMON_SECTION_HEADER));
    fileBody.append(body);

    /* FFS User Interface */
    header.clear();
    header.fill(0, sizeof(EFI_USER_INTERFACE_SECTION));
    EFI_USER_INTERFACE_SECTION* UserInterfaceSection = (EFI_USER_INTERFACE_SECTION*)header.data();

    /* Convert sectionName to unicode data */
    bufSectionName.append((const char*) (sectionName.utf16()), sectionName.size() * 2);

    uint32ToUint24(sizeof(EFI_USER_INTERFACE_SECTION)+bufSectionName.size(), UserInterfaceSection->Size);
    UserInterfaceSection->Type = EFI_SECTION_USER_INTERFACE;

    fileBody.append(header, sizeof(EFI_USER_INTERFACE_SECTION));
    fileBody.append(bufSectionName);

    /* FFS File */
    const static UINT8 revision = 0;
    const static UINT8 erasePolarity = 1;
    const static UINT32 size = fileBody.size();

    QUuid uuid = QUuid(guid);

    header.clear();
    header.fill(0, sizeof(EFI_FFS_FILE_HEADER));
    EFI_FFS_FILE_HEADER* fileHeader = (EFI_FFS_FILE_HEADER*)header.data();

    uint32ToUint24(sizeof(EFI_FFS_FILE_HEADER)+size, fileHeader->Size);
    fileHeader->Attributes = 0x00;
    fileHeader->Attributes |= (erasePolarity == ERASE_POLARITY_TRUE) ? '\xFF' : '\x00';
    fileHeader->Type = EFI_FV_FILETYPE_FREEFORM;
    fileHeader->State = EFI_FILE_HEADER_CONSTRUCTION | EFI_FILE_HEADER_VALID | EFI_FILE_DATA_VALID;
    // Invert state bits if erase polarity is true
    if (erasePolarity == ERASE_POLARITY_TRUE)
        fileHeader->State = ~fileHeader->State;

    memcpy(&fileHeader->Name, &uuid.data1, sizeof(EFI_GUID));

    // Calculate header checksum
    fileHeader->IntegrityCheck.Checksum.Header = 0;
    fileHeader->IntegrityCheck.Checksum.File = 0;
    fileHeader->IntegrityCheck.Checksum.Header = calculateChecksum8((UINT8*)fileHeader, sizeof(EFI_FFS_FILE_HEADER)-1);

    // Set data checksum
    if (fileHeader->Attributes & FFS_ATTRIB_CHECKSUM)
        fileHeader->IntegrityCheck.Checksum.File = calculateChecksum8((UINT8*)fileBody.constData(), fileBody.size());
    else if (revision == 1)
        fileHeader->IntegrityCheck.Checksum.File = FFS_FIXED_CHECKSUM;
    else
        fileHeader->IntegrityCheck.Checksum.File = FFS_FIXED_CHECKSUM2;

    out.clear();
    out.append(header, sizeof(EFI_FFS_FILE_HEADER));
    out.append(fileBody);

    return ERR_SUCCESS;
}
