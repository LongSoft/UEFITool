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
  {"OzmosisDefaults","99F2839C-57C3-411E-ABC3-ADE5267D960D",TRUE},// not required, but small
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

#define compressFfsCount 1

static const sectionEntry compressFfs[] = {
  {"CORE_DXE","5AE3F37E-4EAE-41AE-8240-35465B5E81EB", TRUE}
};

#define deletableFfsCount 11

static const sectionEntry deletableFfs[] = {
  {"FileSystem","93022F8C-1F09-47EF-BBB2-5814FF609DF5", FALSE},
  {"TcpDxe","B1625D3C-9D2D-4E0D-B864-8A763EE4EC50",FALSE},
  {"Dhcp4Dxe","8DD9176E-EE87-4F0E-8A84-3F998311F930",FALSE},
  {"Ip4ConfigDxe","8F9296EF-2880-4659-B857-915A8901BDC8",FALSE},
  {"Ip4Dxe","8F92960F-2880-4659-B857-915A8901BDC8",FALSE},
  {"Mtftp4Dxe","61AFA223-8AC8-4440-9AB5-762B1BF05156",FALSE},
  {"Udp4Dxe","10EE5462-B207-4A4F-ABD8-CB522ECAA3A4",FALSE},
  {"Dhcp6Dxe","8DD9176D-EE87-4F0E-8A84-3F998311F930",FALSE},
  {"Ip6Dxe","8F92960E-2880-4659-B857-915A8901BDC8",FALSE},
  {"Mtftp6Dxe","61AFA251-8AC8-4440-9AB5-762B1BF05156",FALSE},
  {"Udp6Dxe","10EE54AE-B207-4A4F-ABD8-CB522ECAA3A4",FALSE}
};

static const sectionEntry amiBoardSection = {
  "AmiBoardInfo","9F3A0016-AE55-4288-829D-D22FD344C347", TRUE
};

#define MAX_RUNS 4

#define RUN_AS_IS           0
#define RUN_COMPRESS        1
#define RUN_DEL_OZM_NREQ    2
#define RUN_DELETE          3

static QList<sectionEntry> occupyFfs;
static QList<sectionEntry> OzmFfs;

OZMHelper::OZMHelper(QObject *parent) :
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
        occupyFfs.append(compressFfs[i]);
    }
    for(int i = 0; i < deletableFfsCount; i++) {
        occupyFfs.append(deletableFfs[i]);
    }
}

OZMHelper::~OZMHelper()
{
}

UINT8 OZMHelper::DSDTExtract(QString inputfile, QString outputdir)
{
    UINT8 ret;
    QString outputFile;
    QByteArray buf, dsdtbuf;

    FFSUtil *fu = new FFSUtil();

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

    outputFile = pathConcatenate(outputdir, amiBoardSection.name + ".bin");

    ret = fileWrite(outputFile, buf);
    if (ret) {
        printf("ERROR: Writing AmiBoardInfo.bin to '%s' failed!\n", qPrintable(outputFile));
        return ret;
    }

    outputFile = pathConcatenate(outputdir, DSDTFilename);

    ret = fileWrite(outputFile, dsdtbuf);
    if (ret) {
        printf("ERROR: Writing DSDT.aml to '%s' failed!\n", qPrintable(outputFile));
        return ret;
    }

    buf.clear();
    dsdtbuf.clear();

    return ERR_SUCCESS;
}

UINT8 OZMHelper::OZMUpdate(QString inputfile, QString recentBios, QString outputfile)
{
    UINT8 ret;
    QByteArray oldBIOS;
    QByteArray newBIOS;

    FFSUtil *oFU = new FFSUtil();
    FFSUtil *nFU = new FFSUtil();

    ret = fileOpen(inputfile, oldBIOS);
    if (ret) {
        printf("ERROR: Opening '%s' failed!\n", qPrintable(inputfile));
        return ret;
    }

    ret = oFU->parseBIOSFile(oldBIOS);
    if (ret) {
        printf("ERROR: Parsing old BIOS failed!\n");
        return ret;
    }

    ret = fileOpen(recentBios, newBIOS);
    if (ret) {
        printf("ERROR: Opening '%s' failed!\n", qPrintable(recentBios));
        return ret;
    }

    ret = nFU->parseBIOSFile(newBIOS);
    if (ret) {
        printf("ERROR: Parsing old BIOS failed!\n");
        return ret;
    }

    printf("Function not implemented yet... Sorry!\n");
    return ERR_NOT_IMPLEMENTED;
}

UINT8 OZMHelper::OZMExtract(QString inputfile, QString outputdir)
{
    int i;
    UINT8 ret;
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

    return ERR_SUCCESS;
}

UINT8 OZMHelper::OZMCreate(QString inputfile, QString outputfile, QString inputFFSdir, QString inputKextdir, QString inputDSDTfile)
{
    int i, run = 0;
    UINT8 ret;
    QString guid;
    QByteArray bios, dsdt, ffs, out;
    QByteArray amiboard, patchedAmiboard;
    QModelIndex amiFileIdx, amiSectionIdx, volumeIdxCount, currIdx, rootIndex;

    BOOLEAN insertDSDT = FALSE;
    BOOLEAN insertKexts = FALSE;

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

    do {
        //cleanup
        bios.clear();
        dsdt.clear();
        ffs.clear();
        out.clear();
        amiboard.clear();
        patchedAmiboard.clear();

        FFSUtil *fu = new FFSUtil();

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

        rootIndex = fu->getRootIndex();

        /* Needed here to know correct volume image where everything goes */
        ret = fu->findFileByGUID(rootIndex, amiBoardSection.GUID, amiFileIdx);
        if(ret) {
            printf("ERROR: '%s' [%s] couldn't be found!\n", qPrintable(amiBoardSection.name), qPrintable(amiBoardSection.GUID));
            return ERR_ITEM_NOT_FOUND;
        }

        ret = fu->findSectionByIndex(amiFileIdx, EFI_SECTION_PE32, amiSectionIdx);
        if(ret) {
            printf("ERROR: PE32 Section of GUID %s couldn't be found!\n",qPrintable(amiBoardSection.GUID));
            return ERR_ITEM_NOT_FOUND;
        }

        fu->getLastSibling(amiFileIdx, volumeIdxCount); // We want Sibling of file, not section!

        if(insertDSDT) {
            printf("Inserting supplied DSDT into image...\n");

            ret = fileOpen(inputDSDTfile, dsdt);
            if (ret) {
                printf("ERROR: Opening DSDT '%s' failed!\n", qPrintable(inputDSDTfile));
                return ret;
            }

            if(!dsdt.startsWith("DSDT")) {
                printf("ERROR: Input DSDT doesn't contain valid header!\n");
                return ERR_INVALID_FILE;
            }

            ret = fu->dumpSectionByGUID(amiBoardSection.GUID, EFI_SECTION_PE32,
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

            ret = fu->replace(amiSectionIdx, patchedAmiboard, REPLACE_MODE_BODY);
            if(ret) {
                printf("ERROR: Failed to replace '%s' [%s]\n", qPrintable(amiBoardSection.name), qPrintable(amiBoardSection.GUID));
                return ERR_REPLACE;
            }

            printf("* Replaced AmiBoardInfo in BIOS with patched one\n");
        }

        printf("Injecting FFS into BIOS...\n");
        QDirIterator diFFS(inputFFSdir);
        QFileInfo currFFS;

        while (diFFS.hasNext()) {
            ffs.clear();
            guid = "";
            currIdx = rootIndex; // reset to 0,0
            currFFS = diFFS.next();

            if(!currFFS.fileName().compare(".")  ||
                    !currFFS.fileName().compare("..") ||
                    currFFS.fileName().isEmpty())
                continue;

            ret = fileOpen(currFFS.filePath(), ffs);
            if (ret) {
                printf("ERROR: Opening '%s' failed!\n", qPrintable(diFFS.filePath()));
                return ret;
            }

            /* ToDo: verify input file, guid is read without verification */

            ret = getGUIDfromFile(ffs, guid);
            if (ret){
                printf("ERROR: Getting GUID from file failed!\n");
                return ret;
            }

            ret = fu->findFileByGUID(rootIndex, guid, currIdx);
            if (ret) {
                printf(" * File '%s' [%s] not existant, inserting at the end of volume\n", qPrintable(diFFS.fileName()), qPrintable(guid));
                ret = fu->insert(volumeIdxCount, ffs, CREATE_MODE_AFTER);
                if(ret) {
                    printf("ERROR: Injection failed!\n");
                    return ret;
                }
            }
            else {
                /* Found, replace at known index */
                printf(" * File '%s' [%s] is already present -> Replacing it!\n", qPrintable(diFFS.fileName()), qPrintable(guid));
                ret = fu->replace(currIdx, ffs, REPLACE_MODE_AS_IS); // as-is for whole File
                if(ret) {
                    printf("ERROR: Replacing failed!\n");
                    return ret;
                }
            }
        }

        if(insertKexts) {

            printf("Converting Kext & injecting into BIOS...\n");
            QList<kextEntry> kextList;

            ffs.clear();
            guid = "";
            currIdx = rootIndex; // reset to 0,0

            ret = parseKextDirectory(inputKextdir, kextList);
            if (ret) {
                printf("ERROR: Parsing supplied Kext directory failed!\n");
                return ret;
            }

            for(i=0; i<kextList.size(); i++) {
                printf("* Attempting to convert '%s'..\n", qPrintable(kextList.at(i).basename));
                ret = convertKexts(kextList.at(i), ffs);
                if (ret) {
                    printf("ERROR: Conversion failed!\n");
                    return ret;
                }

                /* No need to verify, convertKexts returned fine */

                ret = getGUIDfromFile(ffs, guid);
                if (ret) {
                    printf("ERROR: Getting GUID failed!\n");
                    return ret;
                }

                ret = fu->findFileByGUID(rootIndex, guid, currIdx);
                if (ret) {
                    printf(" * File '%s' [%s] not existant, inserting at the end of volume\n", qPrintable(kextList.at(i).basename), qPrintable(guid));
                    ret = fu->insert(volumeIdxCount, ffs, CREATE_MODE_AFTER);
                    if(ret) {
                        printf("ERROR: Injection failed!\n");
                        return ret;
                    }
                }
                else {
                    /* Found, replace at known index */
                    printf(" * File '%s' [%s] is already present -> Replacing it!\n", qPrintable(kextList.at(i).basename), qPrintable(guid));
                    ret = fu->replace(currIdx, ffs, REPLACE_MODE_AS_IS); // as-is for whole File
                    if(ret) {
                        printf("ERROR: Replacing failed!\n");
                        return ret;
                    }
                }
            }
        }

        BOOLEAN req = FALSE;
        switch(run){
        case RUN_DELETE:
            printf("Deleting network BIOS stuff (PXE) to save space...\n");
            for(i = 0; i<occupyFfs.size(); i++){
                req = occupyFfs.at(i).required;
                if(!req){
                    ret = fu->findFileByGUID(rootIndex,occupyFfs.at(i).GUID,currIdx);
                    if(ret)
                        continue;
                    ret = fu->remove(currIdx);
                    if(ret)
                        printf("Warning: Removing entry '%s' [%s] failed!\n", qPrintable(occupyFfs.at(i).name), qPrintable(occupyFfs.at(i).GUID));
                    else
                        printf(" * Removed '%s' [%s] succesfully!\n", qPrintable(occupyFfs.at(i).name), qPrintable(occupyFfs.at(i).GUID));
                }
            }
        case RUN_DEL_OZM_NREQ:
            printf("Deleting non-essential Ozmosis files to save space...\n");
            for(i = 0; i<OzmFfs.size(); i++){
                req = OzmFfs.at(i).required;
                if(!req){
                    ret = fu->findFileByGUID(rootIndex,OzmFfs.at(i).GUID,currIdx);
                    if(ret)
                        continue;
                    ret = fu->remove(currIdx);
                    if(ret)
                        printf("Warning: Removing entry '%s' [%s] failed!\n", qPrintable(OzmFfs.at(i).name), qPrintable(OzmFfs.at(i).GUID));
                    else
                        printf(" * Removed '%s' [%s] succesfully!\n", qPrintable(OzmFfs.at(i).name), qPrintable(OzmFfs.at(i).GUID));
                }
            }
        case RUN_COMPRESS:
            printf("Compressing some files to save space...\n");
            printf("Warning: Sorry... not implemented yet...\n");
        case RUN_AS_IS:
            break;
        }

        out.clear();
        printf("Reconstructing final image...\n");
        ret = fu->reconstructImageFile(out);
        if(ret)
            printf("ERROR: Image exploded...\n");

        run++;

        if((run < MAX_RUNS) && ret) {
            printf("\n\n*** Re-trying the process ***\n");
            continue;
        } else if(run >= MAX_RUNS && ret)
            return ret;

        printf(" * Image built successfully!\n");
        break;

    } while(true);

    ret = fileWrite(outputfile, out);
    if (ret) {
        printf("ERROR: Writing patched BIOS to '%s' failed!\n", qPrintable(outputfile));
        return ret;
    }
    printf("Bios successfully saved to '%s'\n",qPrintable(outputfile));

    return ERR_SUCCESS;
}

UINT8 OZMHelper::FFSConvert(QString inputdir, QString outputdir)
{
    UINT8 ret;
    QList<kextEntry> toConvert;
    QString filepath;
    QByteArray out;

    if (!dirExists(inputdir)) {
        printf("ERROR: Input directory '%s' doesn't exist!\n", qPrintable(inputdir));
        return ERR_DIR_NOT_EXIST;
    }

    ret = dirCreate(outputdir);
    if (ret == ERR_DIR_CREATE) {
        printf("ERROR: Creating output directory failed!\n");
        return ret;
    }

    ret = parseKextDirectory(inputdir, toConvert);
    if (ret) {
        printf("ERROR: Parsing supplied Kext directory failed!\n");
        return ret;
    }

    for(int i=0; i < toConvert.size(); i++) {
        out.clear();

        printf("* Attempting to convert '%s'..\n", qPrintable(toConvert.at(i).basename));
        ret = convertKexts(toConvert.at(i), out);
        if(ret) {
            printf("ERROR: Conversion failed!\n");
            return ERR_ERROR;
        }

        filepath = pathConcatenate(outputdir, toConvert.at(i).filename);

        fileWrite(filepath, out);
        if(ret) {
            printf("ERROR: Saving '%s' failed!\n", qPrintable(filepath));
            return ERR_ERROR;
        }

        printf("* Success!\n");
    }

    return ERR_SUCCESS;
}

UINT8 OZMHelper::DSDT2Bios(QString inputfile, QString inputDSDTfile, QString outputfile)
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
