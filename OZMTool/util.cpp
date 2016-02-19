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

UINT8 convertBinary(QString input, QString guid, QString sectionName, QByteArray & out)
{
    UINT8 ret;
    QByteArray plist;

    ret = fileOpen(input, plist);
    if(ret) {
        printf("ERROR: Open failed: %s\n", qPrintable(input));
        return ERR_ERROR;
    }

    ret = freeformCreate(plist, guid, sectionName, out);
    if(ret) {
        printf("ERROR: KEXT2FFS failed on '%s'\n", qPrintable(ozmDefaultsFilename));
        return ERR_ERROR;
    }

    return ERR_SUCCESS;
}

UINT8 convertKext(QString input, QString guid, QString basename, QByteArray & out)
{
    UINT8 ret;
    UINT8 nullterminator = 0;

    QString sectionName;
    QString bundleVersion, execName;
    QDir dir;

    QFileInfo binaryPath;
    QFileInfo plistPath;

    QByteArray plistbuf;
    QByteArray binarybuf;
    QByteArray toConvertBinary;

    // Check all folders in input-dir

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
    } else {
        sectionName.sprintf("%s.Rev-%s",qPrintable(basename), qPrintable(bundleVersion));
    }

    toConvertBinary.append(plistbuf);
    toConvertBinary.append(nullterminator);
    toConvertBinary.append(binarybuf);

    ret = freeformCreate(toConvertBinary, guid, sectionName, out);
    if(ret) {
        printf("ERROR: KEXT2FFS failed on '%s'\n", qPrintable(sectionName));
        return ERR_ERROR;
    }

    return ERR_SUCCESS;
}

UINT8 fileCreate(QByteArray fileBody, QString fileGuid, UINT8 fileType, UINT8 fileAttributes, UINT8 revision, UINT8 erasePolarity, QByteArray & fileOut)
{
    QUuid uuid = QUuid(fileGuid);
    EFI_FFS_FILE_HEADER *ffsHeader = new EFI_FFS_FILE_HEADER(); // = {0};

    if(uuid.isNull()) {
        printf("ERROR: Invalid GUID supplied!\n");
        return ERR_ERROR;
    }

    switch(fileType) {
    case EFI_FV_FILETYPE_RAW:
    case EFI_FV_FILETYPE_FREEFORM:
    case EFI_FV_FILETYPE_SECURITY_CORE:
    case EFI_FV_FILETYPE_PEI_CORE:
    case EFI_FV_FILETYPE_DXE_CORE:
    case EFI_FV_FILETYPE_PEIM:
    case EFI_FV_FILETYPE_DRIVER:
    case EFI_FV_FILETYPE_COMBINED_PEIM_DRIVER:
    case EFI_FV_FILETYPE_APPLICATION:
    case EFI_FV_FILETYPE_SMM:
    case EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE:
    case EFI_FV_FILETYPE_COMBINED_SMM_DXE:
    case EFI_FV_FILETYPE_SMM_CORE:
        ffsHeader->Attributes = fileAttributes;
        ffsHeader->State = EFI_FILE_HEADER_CONSTRUCTION | EFI_FILE_HEADER_VALID | EFI_FILE_DATA_VALID;
        ffsHeader->Type = fileType;
        uint32ToUint24(sizeof(EFI_FFS_FILE_HEADER)+fileBody.size(), ffsHeader->Size);
        memcpy(&ffsHeader->Name, &uuid.data1, sizeof(EFI_GUID));
        break;
    default:
        printf("Info: Unsupported fileType supplied!\n");
        return ERR_ERROR;
    }

    ffsHeader->Attributes |= (erasePolarity == ERASE_POLARITY_TRUE) ? '\xFF' : '\x00';
    if (erasePolarity == ERASE_POLARITY_TRUE)
        ffsHeader->State = ~ffsHeader->State;


    // Calculate header checksum
    ffsHeader->IntegrityCheck.Checksum.Header = 0;
    ffsHeader->IntegrityCheck.Checksum.File = 0;
    ffsHeader->IntegrityCheck.Checksum.Header = calculateChecksum8((UINT8*)ffsHeader, sizeof(EFI_FFS_FILE_HEADER)-1);

    // Set data checksum
    if (ffsHeader->Attributes & FFS_ATTRIB_CHECKSUM)
        ffsHeader->IntegrityCheck.Checksum.File = calculateChecksum8((UINT8*)fileBody.constData(), fileBody.size());
    else if (revision == 1)
        ffsHeader->IntegrityCheck.Checksum.File = FFS_FIXED_CHECKSUM;
    else
        ffsHeader->IntegrityCheck.Checksum.File = FFS_FIXED_CHECKSUM2;

    fileOut.append((const char*)ffsHeader, sizeof(EFI_FFS_FILE_HEADER));
    fileOut.append(fileBody);

    return ERR_SUCCESS;
}

UINT8 sectionCreate(QByteArray body, UINT8 sectionType, QByteArray & sectionOut)
{
    UINT8 alignment;
    QByteArray fileBody, header;
    EFI_COMPRESSION_SECTION *compressedSection = new EFI_COMPRESSION_SECTION();
    EFI_COMMON_SECTION_HEADER *sectionHeader   = new EFI_COMMON_SECTION_HEADER();


    FfsEngine *fe = new FfsEngine();

    switch (sectionType) {
    case EFI_SECTION_COMPRESSION:
        fe->compress(body, COMPRESSION_ALGORITHM_TIANO, fileBody);
        alignment = fileBody.size() % 4;
        if (alignment) {
            fileBody.append(QByteArray(4 - alignment, '\x00'));
        }

        compressedSection->CompressionType = COMPRESSION_ALGORITHM_TIANO;
        compressedSection->Type = EFI_SECTION_COMPRESSION;
        compressedSection->UncompressedLength = body.size(); //fileBody ???
        uint32ToUint24(sizeof(EFI_COMPRESSION_SECTION) + fileBody.size(), compressedSection->Size); //body.size ???

        header.append((const char *) compressedSection, sizeof(EFI_COMPRESSION_SECTION));
        break;
    case EFI_SECTION_PE32:
    case EFI_SECTION_PIC:
    case EFI_SECTION_TE:
    case EFI_SECTION_DXE_DEPEX:
    case EFI_SECTION_PEI_DEPEX:
    case EFI_SECTION_SMM_DEPEX:
    case EFI_SECTION_COMPATIBILITY16:
    case EFI_SECTION_USER_INTERFACE:
    case EFI_SECTION_RAW:
        fileBody.append(body);
        alignment = fileBody.size() % 4;
        if (alignment) {
            fileBody.append(QByteArray(4 - alignment, '\x00'));
        }

        sectionHeader->Type = sectionType;
        uint32ToUint24(sizeof(EFI_COMMON_SECTION_HEADER) + fileBody.size(), sectionHeader->Size);

        header.append((const char *) sectionHeader, sizeof(EFI_COMMON_SECTION_HEADER));
        break;
    default:
        printf("ERROR: Invalid sectionType supplied!\n");
        return ERR_ERROR;
    }

    sectionOut.append(header);
    sectionOut.append(fileBody);

    return ERR_SUCCESS;
}

UINT8 freeformCreate(QByteArray binary, QString guid, QString sectionName, QByteArray & fileOut)
{
    UINT8 ret;
    QByteArray rawSection, userSection, ffs, body;

    ret = sectionCreate(binary, EFI_SECTION_PE32, rawSection);
    if (ret) {
        printf("ERROR: Failed to create PE32 Section!\n");
        return ERR_ERROR;
    }

    // somehow this doesn't create the correct UTF string
    // ret = sectionCreate(QByteArray((const char*)sectionName.utf16()), EFI_SECTION_USER_INTERFACE, userSection);
    // use workaround
    QByteArray ba;
    ba.append( (const char*) sectionName.utf16(), sectionName.size() * 2 );
    ret = sectionCreate(ba, EFI_SECTION_USER_INTERFACE, userSection);
    if (ret) {
        printf("ERROR: Failed to create User Interface Section!\n");
        return ERR_ERROR;
    }

    body.append(rawSection);
    body.append(userSection);

    ret = fileCreate(body, guid, EFI_FV_FILETYPE_FREEFORM, 0, 0, ERASE_POLARITY_FALSE, ffs);
    if (ret) {
        printf("ERROR: Failed to create FFS Header!\n");
        return ERR_ERROR;
    }

    fileOut = ffs;

    return 0;
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
    if (offset < 0) {
        printf("ERROR: DSDT wasn't found in AmiBoardInfo");
        return ERR_FILE_NOT_FOUND;
    }

    size = getUInt32(amiboardbuf, offset+DSDT_HEADER_SZ, TRUE);

    if (size > (UINT32)(amiboardbuf.size() - offset)) {
        printf("ERROR: Read invalid size from DSDT. Aborting!\n");
        return ERR_INVALID_PARAMETER;
    }

    out.append(amiboardbuf.mid(offset, size + 1));

    return ERR_SUCCESS;
}

UINT8 injectDSDTintoAmiboardInfo(QByteArray ami, QByteArray dsdtbuf, QByteArray & out)
{
    int i;
    INT32 offset, diffDSDT;

    BOOLEAN hasDotROM = FALSE;
    BOOLEAN needsCodePatching = TRUE;
    UINT32 relocStart, relocSize;
    int physEntries, logicalEntries;
    UINT32 index;
    UINT32 dataLeft;
    UINT32 baseRelocAddr;

    UINT32 oldDSDTsize, newDSDTsize, sectionsStart, alignDiffDSDT;
    EFI_IMAGE_DOS_HEADER *HeaderDOS;
    EFI_IMAGE_NT_HEADERS64 *HeaderNT;
    EFI_IMAGE_SECTION_HEADER *Section;
    EFI_IMAGE_BASE_RELOCATION *BASE_RELOCATION;
    RELOC_ENTRY *RELOCATION_ENTRIES;

    const static char *DATA_SECTION  = ".data";
    const static char *EMPTY_SECTION = ".empty";
    const static char *RELOC_SECTION = ".reloc";

    static unsigned char *amiboardbuf = (unsigned char *) ami.constData();

    HeaderDOS = (EFI_IMAGE_DOS_HEADER *) amiboardbuf;

    if (HeaderDOS->e_magic != EFI_IMAGE_DOS_SIGNATURE) {
        printf("Error: Invalid file, not AmiBoardInfo. Aborting!\n");
        return ERR_INVALID_FILE;
    }

    offset = ami.indexOf(DSDT_HEADER);
    if (offset < 0) {
        printf("ERROR: DSDT wasn't found in AmiBoardInfo");
        return ERR_FILE_NOT_FOUND;
    }

    if (ami.indexOf(UNPATCHABLE_SECTION) > 0) {
        hasDotROM = TRUE;
    }

    oldDSDTsize = getUInt32(ami, offset+DSDT_HEADER_SZ, TRUE);

    if(oldDSDTsize > (UINT32) (sizeof(amiboardbuf) - offset)) {
        printf("ERROR: Read invalid size from DSDT. Aborting!\n");
        return ERR_INVALID_PARAMETER;
    }

    newDSDTsize = dsdtbuf.size();
    diffDSDT = newDSDTsize - oldDSDTsize;

    if(diffDSDT <= 0) {
        printf("Info: New DSDT is not larger than old one, no need to patch anything :)\n");
        UINT32 padbytes = (diffDSDT * (-1)); // negative val -> positive
        out.append(ami.left(offset)); // Start of PE32
        out.append(dsdtbuf); // new DSDT
        out.append(QByteArray(padbytes, '\x00')); // padding to match old DSDT location
        out.append(ami.mid(offset + oldDSDTsize)); // rest of PE32
        return ERR_SUCCESS;
    }

    HeaderNT = (EFI_IMAGE_NT_HEADERS64 *) & amiboardbuf[HeaderDOS->e_lfanew];
    sectionsStart = HeaderDOS->e_lfanew + sizeof(EFI_IMAGE_NT_HEADERS64);
    Section = (EFI_IMAGE_SECTION_HEADER *) & amiboardbuf[sectionsStart];

    relocStart = HeaderNT->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress;
    relocSize  = HeaderNT->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;

    if((HeaderNT->OptionalHeader.DllCharacteristics & DYNAMIC_BASE) && hasDotROM) {
        needsCodePatching = FALSE;
        printf("Info: PE32 has DYNAMIC_BASE set -> no Code Patching required...\n");
    } else if (hasDotROM) {
        printf("ERROR: PE32 has .ROM but not DYNAMIC_BASE set -> Unpatchable atm..\n");
        return ERR_ERROR;
    }

    alignDiffDSDT = ALIGN32(diffDSDT);

    printf(" * Patching header...\n");
    printf("\tSizeOfInitialzedData: %X --> %X\n",
           HeaderNT->OptionalHeader.SizeOfInitializedData,
           HeaderNT->OptionalHeader.SizeOfInitializedData += alignDiffDSDT);
    printf("\tSizeOfImage: %X --> %X\n",
           HeaderNT->OptionalHeader.SizeOfImage,
           HeaderNT->OptionalHeader.SizeOfImage += alignDiffDSDT);

    printf(" * Patching directory entries...\n");
    for ( i = 0; i < EFI_IMAGE_NUMBER_OF_DIRECTORY_ENTRIES ;i++) {

        if(HeaderNT->OptionalHeader.DataDirectory[i].VirtualAddress == 0)
            continue;

        printf(" - DataDirectory %02X:\n", i);
        printf("\tVirtualAddress: %X --> %X\n",
               HeaderNT->OptionalHeader.DataDirectory[i].VirtualAddress,
               HeaderNT->OptionalHeader.DataDirectory[i].VirtualAddress += alignDiffDSDT);
    }

    printf(" * Patching sections...\n");
    for (i = 0 ; i < HeaderNT->FileHeader.NumberOfSections; i++) {

        if (!strcmp((char *) & Section[i].Name, "")) // Give it a clear name
             strcpy((char *) & Section[i].Name, EMPTY_SECTION);

        printf(" - Section: %s\n", Section[i].Name);

        if(!strcmp((char *) & Section[i].Name, DATA_SECTION)) {
            /* DSDT blob starts in .data section */
            printf("\tPhysicalAddress: %X --> %X\n",
                   Section[i].Misc.PhysicalAddress,
                   Section[i].Misc.PhysicalAddress += alignDiffDSDT);
            printf("\tSizeOfRawData: %X --> %X\n",
                   Section[i].SizeOfRawData,
                   Section[i].SizeOfRawData += alignDiffDSDT);
        }
        else if(!strcmp((char *) & Section[i].Name, EMPTY_SECTION)) {
            /* .empty section is after .data -> needs patching */
            printf("\tVirtualAddress: %X --> %X\n",
                   Section[i].VirtualAddress,
                   Section[i].VirtualAddress += alignDiffDSDT);
            printf("\tPointerToRawData: %X --> %X\n",
                   Section[i].PointerToRawData,
                   Section[i].PointerToRawData += alignDiffDSDT);
        }
        else if(!strcmp((char *) & Section[i].Name, RELOC_SECTION)) {
            /* .reloc section is after .data -> needs patching */
            printf("\tVirtualAddress: %X --> %X\n",
                   Section[i].VirtualAddress,
                   Section[i].VirtualAddress += alignDiffDSDT);
            printf("\tPointerToRawData: %X --> %X\n",
                   Section[i].PointerToRawData,
                   Section[i].PointerToRawData += alignDiffDSDT);
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
            BASE_RELOCATION = (EFI_IMAGE_BASE_RELOCATION *) & amiboardbuf[baseRelocAddr];
            physEntries = (BASE_RELOCATION->SizeOfBlock - EFI_IMAGE_SIZEOF_BASE_RELOCATION) / EFI_IMAGE_SIZEOF_RELOC_ENTRY;
            logicalEntries = physEntries - 1; // physEntries needed to calc next Base Relocation Table offset
            RELOCATION_ENTRIES = (RELOC_ENTRY*) & amiboardbuf[baseRelocAddr+EFI_IMAGE_SIZEOF_BASE_RELOCATION];

            baseRelocAddr += EFI_IMAGE_SIZEOF_BASE_RELOCATION + (physEntries * EFI_IMAGE_SIZEOF_RELOC_ENTRY);
            dataLeft -= (physEntries * EFI_IMAGE_SIZEOF_RELOC_ENTRY) + EFI_IMAGE_SIZEOF_BASE_RELOCATION;


            printf(" - Relocation Table %X:\n", index);
            index++;

            if(BASE_RELOCATION->VirtualAddress < (UINT32)offset) {
                printf("\tNothing to do here - VirtualAddress < DSDTOffset (%X < %X)\n",
                                BASE_RELOCATION->VirtualAddress, offset);
                continue;
            }

            //Testing first relocation entry should be good..
            UINT32 shiftBy = ((UINT32)RELOCATION_ENTRIES[0].offset + alignDiffDSDT) & 0xF000;

            printf(" - VirtualAddress: %X --> %X\n",
                    BASE_RELOCATION->VirtualAddress,
                    BASE_RELOCATION->VirtualAddress += shiftBy);

            for(int j=0; j<logicalEntries; j++) {
                printf(" - Relocation: %X\n", j);
                printf("\tOffset: %X --> %X\n",
                       RELOCATION_ENTRIES[j].offset,
                       RELOCATION_ENTRIES[j].offset += alignDiffDSDT);
            }
        }
    }


    if (needsCodePatching) {
        printf(" * Patching addresses in code\n");
        const static UINT32 MAX_INSTRUCTIONS = 1000;
        _DInst decomposed[MAX_INSTRUCTIONS];
        _DecodedInst disassembled[MAX_INSTRUCTIONS];
        _DecodeResult res, res2;
        _CodeInfo ci = {0, 0, 0, 0, Decode64Bits, 0};
        ci.codeOffset = HeaderNT->OptionalHeader.BaseOfCode;
        ci.codeLen = HeaderNT->OptionalHeader.SizeOfCode;
        ci.code = (const unsigned char*)&amiboardbuf[ci.codeOffset];
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

        for (int i = 0; i < (int) decodedInstructionsCount; i++) {

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
                   *patchValue += alignDiffDSDT);
            patchCount++;
        }

        if(patchCount < 1){
            printf("ERROR: Something went wrong, didn't patch anything...\n");
            return ERR_ERROR;
        }

        printf("Patched %i instructions\n", patchCount);
    }

    /* Copy data till DSDT */
    out.append((const char*) amiboardbuf, offset);
    // Copy new DSDT
    out.append(dsdtbuf, newDSDTsize);
    // Pad the file
    out.append(QByteArray((alignDiffDSDT - diffDSDT), '\x00'));
    // Copy the rest
    out.append(ami.mid(offset + oldDSDTsize));

    return ERR_SUCCESS;
}
