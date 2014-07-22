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
#include <qtplist/PListParser.h>
#include <distorm.h>
#include "../ffs.h"
#include "../peimage.h"
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
    static const QString execIdentifier = "CFBundleExecutable";
    QString plistExec;

    QVariantMap parsed = PListParser::parsePList(plist).toMap();

    if (parsed.contains(execIdentifier))
        plistExec = parsed.value(execIdentifier).toString();

    if(plistExec.isEmpty()) {
        printf("ERROR: '%s'' in plist is blank. Aborting!\n", qPrintable(execIdentifier));
        return ERR_ERROR;
    }

    name = plistExec;

    return ERR_SUCCESS;
}

UINT8 plistReadBundleVersion(QByteArray plist, QString & version)
{
    static const QString versionIdentifier = "CFBundleShortVersionString";
    QString plistVersion;

    QVariantMap parsed = PListParser::parsePList(plist).toMap();

    if (parsed.contains(versionIdentifier))
        plistVersion = parsed.value(versionIdentifier).toString();

    if(plistVersion.isEmpty()) {
        version = "?";
        return ERR_ERROR;
    }

    version = plistVersion;

    return ERR_SUCCESS;
}

UINT8 checkAggressivityLevel(int aggressivity) {
    QString level;

    switch(aggressivity) {
    case RUN_AS_IS:
        level = "Do nothing - Inject as-is";
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

    ret = fileOpen(input, plist);
    if(ret) {
        printf("ERROR: Open failed: %s\n", qPrintable(input));
        return ERR_ERROR;
    }

    ret = ffsCreate(plist, ozmosisDefaults.GUID, ozmosisDefaults.name, out);
    if(ret) {
        printf("ERROR: KEXT2FFS failed on '%s'\n", qPrintable(ozmDefaultsFilename));
        return ERR_ERROR;
    }

    return ERR_SUCCESS;
}

UINT8 convertKext(QString input, int kextIndex, QString basename, QByteArray & out)
{
    UINT8 ret;
    UINT8 nullterminator = 0;

    QString sectionName, guid;
    QString bundleVersion, execName;
    QDir dir;

    QFileInfo binaryPath;
    QFileInfo plistPath;

    QByteArray plistbuf;
    QByteArray binarybuf;
    QByteArray toConvertBinary;

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

    ret = fileOpen(binaryPath.filePath(), binarybuf);
    if (ret) {
        printf("ERROR: Opening '%s' failed!\n", qPrintable(binaryPath.filePath()));
        return ERR_ERROR;
    }

    ret = plistReadBundleVersion(plistbuf, bundleVersion);
    if (ret) {
        printf("Info: Unable to get version string...\n");
        sectionName = basename;
    }
    else
        sectionName.sprintf("%s.Rev-%s",qPrintable(basename), qPrintable(bundleVersion));

    guid = kextGUID.arg(kextIndex, 1, 16).toUpper();

    toConvertBinary.append(plistbuf);
    toConvertBinary.append(nullterminator);
    toConvertBinary.append(binarybuf);

    ret = ffsCreate(toConvertBinary, guid, sectionName, out);
    if(ret) {
        printf("ERROR: KEXT2FFS failed on '%s'\n", qPrintable(sectionName));
        return ERR_ERROR;
    }

    return ERR_SUCCESS;
}

UINT8 ffsCreate(QByteArray body, QString guid, QString sectionName, QByteArray & out)
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

    /* Align for next section */
    UINT8 alignment = fileBody.size() % 4;
    if (alignment) {
        alignment = 4 - alignment;
        fileBody.append(QByteArray(alignment, '\x00'));
    }

    fileBody.append(header, sizeof(EFI_USER_INTERFACE_SECTION));
    fileBody.append(bufSectionName);

    /* FFS File */
    const static UINT8 revision = 0;
    const static UINT8 erasePolarity = 0;
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

UINT8 extractDSDTfromAmiboardInfo(QByteArray amiboardbuf, QByteArray & out)
{
    INT32 offset;
    UINT32 size = 0;
    EFI_IMAGE_DOS_HEADER *HeaderDOS;

    HeaderDOS = (EFI_IMAGE_DOS_HEADER *)amiboardbuf.data();

    if (HeaderDOS->e_magic != EFI_IMAGE_DOS_SIGNATURE) {
        printf("Error: Invalid file, not AmiBoardInfo. Aborting!\n");
        return ERR_INVALID_FILE;
    }

    offset = amiboardbuf.indexOf(DSDT_HEADER);
    if(offset < 0) {
        printf("ERROR: DSDT wasn't found in AmiBoardInfo");
        return ERR_FILE_NOT_FOUND;
    }

    size = getUInt32(amiboardbuf, offset+DSDT_HEADER_SZ, TRUE);

    if(size > (UINT32)(amiboardbuf.size()-offset)) {
        printf("ERROR: Read invalid size from DSDT. Aborting!\n");
        return ERR_INVALID_PARAMETER;
    }

    out.append(amiboardbuf.mid(offset, size + 1));

    return ERR_SUCCESS;
}

UINT8 injectDSDTintoAmiboardInfo(QByteArray amiboardbuf, QByteArray dsdtbuf, QByteArray & out)
{
    int i;
    INT32 offset, diffDSDT;

    UINT32 relocStart, relocSize;
    int physEntries, logicalEntries;
    UINT32 index;
    UINT32 dataLeft;
    UINT32 baseRelocAddr;

    UINT32 oldDSDTsize, newDSDTsize, sectionsStart, alignment;
    EFI_IMAGE_DOS_HEADER *HeaderDOS;
    EFI_IMAGE_NT_HEADERS64 *HeaderNT;
    EFI_IMAGE_SECTION_HEADER *Section;
    EFI_IMAGE_BASE_RELOCATION *BASE_RELOCATION;
    RELOC_ENTRY *RELOCATION_ENTRIES;

    const static char *DATA_SECTION = ".data";
    const static char *EMPTY_SECTION = ".empty";
    const static char *RELOC_SECTION = ".reloc";

    HeaderDOS = (EFI_IMAGE_DOS_HEADER *)amiboardbuf.data();

    if (HeaderDOS->e_magic != EFI_IMAGE_DOS_SIGNATURE) {
        printf("Error: Invalid file, not AmiBoardInfo. Aborting!\n");
        return ERR_INVALID_FILE;
    }

    offset = amiboardbuf.indexOf(DSDT_HEADER);
    if(offset < 0) {
        printf("ERROR: DSDT wasn't found in AmiBoardInfo");
        return ERR_FILE_NOT_FOUND;
    }

    oldDSDTsize = getUInt32(amiboardbuf, offset+DSDT_HEADER_SZ, TRUE);

    if(oldDSDTsize > (UINT32)(amiboardbuf.size()-offset)) {
        printf("ERROR: Read invalid size from DSDT. Aborting!\n");
        return ERR_INVALID_PARAMETER;
    }

    if(amiboardbuf.indexOf(UNPATCHABLE_SECTION) > 0) {
        printf("ERROR: AmiBoardInfo contains '.ROM' section => unpatchable atm!\n");
        return ERR_ERROR;
    }

    newDSDTsize = dsdtbuf.size();
    diffDSDT = newDSDTsize - oldDSDTsize;

    if(diffDSDT <= 0) {
        printf("Info: New DSDT is not larger than old one, no need to patch anything :)\n");
        UINT32 padbytes = (diffDSDT * (-1)); // negative val -> positive
        out.append(amiboardbuf.left(offset)); // Start of PE32
        out.append(dsdtbuf); // new DSDT
        out.append(QByteArray(padbytes, '\x00')); // padding to match old DSDT location
        out.append(amiboardbuf.mid(offset+oldDSDTsize)); // rest of PE32
        return ERR_SUCCESS;
    }

    HeaderNT = (EFI_IMAGE_NT_HEADERS64 *)amiboardbuf.mid(HeaderDOS->e_lfanew).constData();
    sectionsStart = HeaderDOS->e_lfanew+sizeof(EFI_IMAGE_NT_HEADERS64);
    Section = (EFI_IMAGE_SECTION_HEADER *)amiboardbuf.mid(sectionsStart).constData();

    relocStart = HeaderNT->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress;
    relocSize = HeaderNT->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;

    alignment = ALIGN32(diffDSDT);
    printf(" * Patching header...\n");
    printf("\tSizeOfInitialzedData: %X --> %X\n",
           HeaderNT->OptionalHeader.SizeOfInitializedData,
           HeaderNT->OptionalHeader.SizeOfInitializedData += alignment);
    printf("\tSizeOfImage: %X --> %X\n",
           HeaderNT->OptionalHeader.SizeOfImage,
           HeaderNT->OptionalHeader.SizeOfImage += alignment);

    printf(" * Patching directory entries...\n");
    for ( i = 0; i < EFI_IMAGE_NUMBER_OF_DIRECTORY_ENTRIES ;i++) {

        if(HeaderNT->OptionalHeader.DataDirectory[i].VirtualAddress == 0)
            continue;

        printf(" - DataDirectory %02X:\n", i);
        printf("\tVirtualAddress: %X --> %X\n",
               HeaderNT->OptionalHeader.DataDirectory[i].VirtualAddress,
               HeaderNT->OptionalHeader.DataDirectory[i].VirtualAddress += alignment);
    }

    printf(" * Patching sections...\n");
    for (i = 0 ; i < HeaderNT->FileHeader.NumberOfSections; i++) {

        if (!strcmp((char *)&Section[i].Name, "")) // Give it a clear name
            strcpy((char *)&Section[i].Name, EMPTY_SECTION);

        printf(" - Section: %s\n", Section[i].Name);

        if(!strcmp((char *)&Section[i].Name, DATA_SECTION)) {
            /* DSDT blob starts in .data section */
            printf("\tSizeOfRawData: %X --> %X\n",
                   Section[i].SizeOfRawData,
                   Section[i].SizeOfRawData += alignment);
        }
        else if(!strcmp((char *)&Section[i].Name, EMPTY_SECTION)) {
            /* .empty section is after .data -> needs patching */
            printf("\tVirtualAddress: %X --> %X\n",
                   Section[i].VirtualAddress,
                   Section[i].VirtualAddress += alignment);
            printf("\tPointerToRawData: %X --> %X\n",
                   Section[i].PointerToRawData,
                   Section[i].PointerToRawData += alignment);
        }
        else if(!strcmp((char *)&Section[i].Name, RELOC_SECTION)) {
            /* .reloc section is after .data -> needs patching */
            printf("\tVirtualAddress: %X --> %X\n",
                   Section[i].VirtualAddress,
                   Section[i].VirtualAddress += alignment);
            printf("\tPointerToRawData: %X --> %X\n",
                   Section[i].PointerToRawData,
                   Section[i].PointerToRawData += alignment);
        }
        else
            printf("\tNothing to do here...\n");
    }

    if(relocStart > 0) {
        printf(" * Patching actual relocations...\n");
        index = 0;
        dataLeft = relocSize;
        baseRelocAddr = relocStart;
        while(dataLeft > 0) {
            BASE_RELOCATION = (EFI_IMAGE_BASE_RELOCATION*) amiboardbuf.mid(baseRelocAddr, EFI_IMAGE_SIZEOF_BASE_RELOCATION).constData();
            physEntries = (BASE_RELOCATION->SizeOfBlock - EFI_IMAGE_SIZEOF_BASE_RELOCATION) / EFI_IMAGE_SIZEOF_RELOC_ENTRY;
            logicalEntries = physEntries - 1; // physEntries needed to calc next Base Relocation Table offset
            RELOCATION_ENTRIES = (RELOC_ENTRY*)amiboardbuf.mid(baseRelocAddr+EFI_IMAGE_SIZEOF_BASE_RELOCATION, logicalEntries * EFI_IMAGE_SIZEOF_RELOC_ENTRY).constData();

            baseRelocAddr += EFI_IMAGE_SIZEOF_BASE_RELOCATION + (physEntries * EFI_IMAGE_SIZEOF_RELOC_ENTRY);
            dataLeft -= (physEntries * EFI_IMAGE_SIZEOF_RELOC_ENTRY) + EFI_IMAGE_SIZEOF_BASE_RELOCATION;
            index++;

            printf(" - Relocation Table %X:\n", index);

            if(BASE_RELOCATION->VirtualAddress < (UINT32)offset) {
                printf("\tNothing to do here - VirtualAddress < DSDTOffset (%X < %X)\n",
                                BASE_RELOCATION->VirtualAddress, offset);
                continue;
            }

            for(int j=0; j<logicalEntries; j++) {
                printf(" - Relocation: %X\n", j);
                printf("\tOffset: %X --> %X\n",
                       RELOCATION_ENTRIES[j].offset,
                       RELOCATION_ENTRIES[j].offset += alignment);
            }
        }
    }

    printf(" * Patching addresses in code\n");
    const static UINT32 MAX_INSTRUCTIONS = 1000;
    _DInst decomposed[MAX_INSTRUCTIONS];
    _DecodedInst disassembled[MAX_INSTRUCTIONS];
    _DecodeResult res, res2;
    _CodeInfo ci = {0};
    ci.codeOffset = HeaderNT->OptionalHeader.BaseOfCode;
    ci.codeLen = HeaderNT->OptionalHeader.SizeOfCode;
    ci.code = (const unsigned char*)amiboardbuf.mid(ci.codeOffset,ci.codeLen).constData();
    ci.dt = Decode64Bits;

    UINT32 decomposedInstructionsCount = 0;
    UINT32 decodedInstructionsCount = 0;
    UINT32 patchCount = 0;

    /* Actual disassembly */
    res = distorm_decode(ci.codeOffset,
                   ci.code,
                   ci.codeLen,
                   Decode64Bits,
                   disassembled,
                   MAX_INSTRUCTIONS,
                   &decomposedInstructionsCount);

    /* Decompose for human-readable output */
    res2 = distorm_decompose(&ci,
                    decomposed,
                    MAX_INSTRUCTIONS,
                    &decodedInstructionsCount);

    if(decodedInstructionsCount != decomposedInstructionsCount) {
        printf("ERROR: decompose / decode mismatch! Aborting!\n");
        return ERR_ERROR;
    }

    for (int i = 0; i < decodedInstructionsCount; i++) {

        if((decomposed[i].disp < (UINT64)offset)||decomposed[i].disp > (MAX_DSDT & 0xFF000))
            continue;

        UINT32 patchOffset = (decomposed[i].addr-ci.codeOffset)+3;
        UINT32 *patchValue = (UINT32*)&ci.code[patchOffset];

        printf("offset: %08X: %s%s%s ",
               patchOffset,
               (char*)disassembled[i].mnemonic.p,
               disassembled[i].operands.length != 0 ? " " : "",
               (char*)disassembled[i].operands.p);
        printf("[%x] --> [%x]\n",
               *patchValue,
               *patchValue += alignment);
        patchCount++;
    }

    if(patchCount < 1){
        printf("ERROR: Something went wrong, didn't patch anything...\n");
        return ERR_ERROR;
    }

    printf("Patched %i instructions\n", patchCount);

    amiboardbuf.replace(ci.codeOffset,ci.codeLen, (const char*)ci.code);
    /* ToDo: Clean up the following mess ? Maybe.. */
    /* ToDo: Stuff new RELOCATION Section + patched .DATA Section in outputfile */

    /* Copy unmodified DOSHeader, modded NTHeader & modded SectionHeader */
    out.append(amiboardbuf.constData(), HeaderDOS->e_lfanew);
    out.append((const char*)HeaderNT, sizeof(EFI_IMAGE_NT_HEADERS64));
    out.append((const char*)Section, sizeof(EFI_IMAGE_SECTION_HEADER)*HeaderNT->FileHeader.NumberOfSections);
    /* Copy rest of data till DSDT */
    out.append(amiboardbuf.mid(out.size()).constData(), (offset-out.size()));
    // Copy new DSDT
    out.append(dsdtbuf.constData(), newDSDTsize);
    // Pad the file
    out.append(QByteArray((alignment-diffDSDT), '\x00'));
    // Copy the rest
    out.append(amiboardbuf.mid(offset+oldDSDTsize).constData(), (amiboardbuf.size()-(offset+oldDSDTsize)));

    return ERR_SUCCESS;
}
