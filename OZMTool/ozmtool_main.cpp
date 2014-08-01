/* ozmtool_main.cpp

Copyright (c) 2014, tuxuser. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/
#include <QCoreApplication>
#include <QString>
#include <QStringList>
#include <iostream>
#include <string.h>
#include <stdio.h>
#include "version.h"

#include "ozmtool.h"

QString appname = "OZMTool";

void usageDsdt2Bios()
{
    printf("dsdt2bios command\n"
            " usage:\n"
            "\t%s --dsdt2bios -i AmiBoardInfo.bin -d DSDT.aml -o patchedAmiBoardInfo.bin\n\n"
            " parameters:\n" \
            "\t-i, --input [file]\t\tInput file (AmiBoardInfo)\n"
            "\t-d, --dsdt [file]\t\tDSDT.aml file\n"
            "\t-o, --out [file]\t\tOutput file (patched AmiBoardInfo)\n"
            "\t-h, --help\t\tPrint this\n\n",qPrintable(appname));
}

void usageKext2Ffs()
{
    printf("kext2ffs command\n"
            " usage:\n"
            "\t%s --kext2ffs -o outputdir -i kextsdir\n\n"
            " parameters:\n"
            "\t-i, --input [dir]\t\tInput kexts directory\n"
            "\t-o, --out [dir]\t\tOutput ffs directory\n"
            "\t-h, --help\t\tPrint this\n\n",qPrintable(appname));
}

void usageOzmUpdate()
{
    printf("ozmupdate command\n" \
            " usage:\n"
            "\t%s --ozmupdate -a 1 -o BIOS_RECENT.OZM -i BIOS.OZM -r BIOS.CLEAN\n\n"
            " parameters:\n"
            "\t-i, --input [file]\t\tInput \"old\" Ozmosis BIOSFile\n"
            "\t-r, --recent [file]\t\tInput \"recent\" clean BIOSFile\n"
            "\t-a, --aggressivity\t\tAggressivity level (see README)\n"
            "\t-cr, --compressdxe\t\tCompress CORE_DXE\n"
            "\t-o, --out [file]\t\tOutput BIOSFile\n"
            "\t-h, --help\t\tPrint this\n\n",qPrintable(appname));
}

void usageOzmCreate()
{
    printf("ozmcreate command\n" \
            " usage:\n"
            "\t%s --ozmcreate -a 1 -cr -ck -k kextdir -f ffsdir -d DSDT.aml -o outputfile -i BIOS.ROM\n\n"
            " parameters:\n"
            "\t-f, --ffs [dir]\t\tFFS directory (extracted OZM files)\n"
            "\t-d, --dsdt [file]\t\t (optional) DSDT.aml file\n"
            "\t-k, --kext [dir]\t\t (optional) KEXT directory\n"
            "\t-e, --efi [dir]\t\t (optional) EFI directory\n"
            "\t-i, --input [file]\t\tInput CLEAN Bios\n"
            "\t-a, --aggressivity\t\t (optional) Aggressivity level (see README)\n"
            "\t-cr,--compressdxe\t\t (optional) Compress CORE_DXE\n"
            "\t-ck,--compresskexts\t\t (optional) Compress converted Kexts\n"
            "\t-o, --out [file]\t\tOutput OZM Bios\n"
            "\t-h, --help\t\tPrint this\n\n",qPrintable(appname));
}

void usageOzmExtract()
{
    printf("ozmextract command\n"
            " usage:\n"
            "\t%s --ozmextract -o outputdir -i OZM.ROM\n\n"
            " parameters:\n"
            "\t-i, --input [file]\t\tInput stock OZM Bios\n"
            "\t-o, --out [dir]\t\tOutput directory\n"
            "\t-h, --help\t\tPrint this\n\n",qPrintable(appname));
}

void usageDSDTExtract()
{
    printf("dsdtextract command\n"
            " usage:\n"
            "\t%s --dsdtextract -o outputdir -i OZMBIOS_or_BIOS.ROM\n\n"
            " parameters:\n"
            "\t-i, --input [file]\t\tBIOS file\n"
            "\t-o, --out [dir]\t\tOutput directory\n"
            "\t-h, --help\t\tPrint this\n\n",qPrintable(appname));
}

void usageDSDTInject()
{
    printf("dsdtinject command\n"
            " usage:\n"
            "\t%s --dsdtinject -i BIOS.ROM -d DSDT.aml -o outputfile\n\n"
            " parameters:\n"
            "\t-i, --input [file]\t\tBIOS file\n"
            "\n-d, --dsdt [file]\t\tDSDT.aml\n"
            "\t-o, --out [dir]\t\tOutput directory\n"
            "\t-h, --help\t\tPrint this\n\n",qPrintable(appname));
}

void usageGeneral()
{
    printf("Usage:\n" \
            "\t%s [COMMAND] [PARAMETERS...]\n\n"
            "Available commands:\n"
            "\t--dsdtextract\t\tExtracts DSDT from BIOS\n"
            "\t--dsdtinject\t\tInjects DSDT into BIOS\n"
            "\t--ozmupdate\t\tUpdates clean BIOS with files from old OZM-flavoured one\n"
            "\t--ozmextract\t\tExtracts Ozmosis files (ffs) from BIOS\n"
            "\t--ozmcreate\t\tPatches Original BIOS with Ozmosis\n"
            "\t--kext2ffs\t\tConverts kext-directories to FFS\n"
            "\t--dsdt2bios\t\tInjects (bigger) DSDT into AmiBoardInfo\n"
            "\t--help, -h\t\tPrint this\n\n",qPrintable(appname));
}

void versionInfo()
{
    printf("%s - %s\n",qPrintable(appname), GIT_VERSION);
}

void usageAll()
{
    usageGeneral();
    usageDSDTExtract();
    usageDSDTInject();
    usageOzmUpdate();
    usageOzmExtract();
    usageOzmCreate();
    usageKext2Ffs();
    usageDsdt2Bios();
}

int main(int argc, char *argv[])
{
    bool help = false;
    bool dsdtextract = false;
    bool dsdtinject = false;
    bool ozmupdate = false;
    bool ozmextract = false;
    bool ozmcreate = false;
    bool kext2ffs = false;
    bool dsdt2bios = false;
    bool compressdxe = false;
    bool compresskexts = false;
    QString inputpath = "";
    QString output = "";
    QString ffsdir = "";
    QString kextdir = "";
    QString efidir = "";
    QString dsdtfile = "";
    QString recent = "";
    int aggressivity = 0;

    QCoreApplication a(argc, argv);
    a.setOrganizationName("tuxuser");
    a.setOrganizationDomain("tuxuser.org");
    a.setApplicationName("OZMTool");

    OZMTool w;
    UINT8 result = ERR_SUCCESS;


    if (argc == 1) {
        versionInfo();
        usageGeneral();
        printf("ERROR: No options supplied!\n");
        return ERR_GENERIC_CALL_NOT_SUPPORTED;
    }

    //
    // Parse command line
    //
    argc --;
    argv ++;


    if ((strcasecmp(argv[0], "-h") == 0) || (strcasecmp(argv[0], "--help") == 0)) {
        usageAll();
        return ERR_SUCCESS;
    }

    if (strcasecmp(argv[0], "--version") == 0) {
        versionInfo();
        return ERR_SUCCESS;
    }

    while (argc > 0) {

        if (strcasecmp(argv[0], "--dsdtextract") == 0) {
            dsdtextract = true;
            argc --;
            argv ++;
            continue;
        }

        if (strcasecmp(argv[0], "--dsdtinject") == 0) {
            dsdtinject = true;
            argc --;
            argv ++;
            continue;
        }

        if (strcasecmp(argv[0], "--ozmupdate") == 0) {
            ozmupdate = true;
            argc --;
            argv ++;
            continue;
        }

        if (strcasecmp(argv[0], "--ozmextract") == 0) {
            ozmextract = true;
            argc --;
            argv ++;
            continue;
        }

        if (strcasecmp(argv[0], "--ozmcreate") == 0) {
            ozmcreate = true;
            argc --;
            argv ++;
            continue;
        }

        if (strcasecmp(argv[0], "--kext2ffs") == 0) {
            kext2ffs = true;
            argc --;
            argv ++;
            continue;
        }

        if (strcasecmp(argv[0], "--dsdt2bios") == 0) {
            dsdt2bios = true;
            argc --;
            argv ++;
            continue;
        }

        if ((strcasecmp(argv[0], "-cr") == 0) || (strcasecmp(argv[0], "--compressdxe") == 0)) {
            compressdxe = true;
            argc --;
            argv ++;
            continue;
        }

        if ((strcasecmp(argv[0], "-ck") == 0) || (strcasecmp(argv[0], "--compresskexts") == 0)) {
            compresskexts = true;
            argc --;
            argv ++;
            continue;
        }

        if ((strcasecmp(argv[0], "-f") == 0) || (strcasecmp(argv[0], "--ffs") == 0)) {
            if (argv[1] == NULL || argv[1][0] == '-') {
                printf("Invalid option value\n"
                       "Input FFS directory is missing for -f option\n");
                goto fail;
            }
            ffsdir = argv[1];
            argc -= 2;
            argv += 2;
            continue;
        }

        if ((strcasecmp(argv[0], "-k") == 0) || (strcasecmp(argv[0], "--kext") == 0)) {
            if (argv[1] == NULL || argv[1][0] == '-') {
                printf("Invalid option value\n"
                       "Input KEXT directory is missing for -k option\n");
                goto fail;
            }
            kextdir = argv[1];
            argc -= 2;
            argv += 2;
            continue;
        }

        if ((strcasecmp(argv[0], "-e") == 0) || (strcasecmp(argv[0], "--efi") == 0)) {
            if (argv[1] == NULL || argv[1][0] == '-') {
                printf("Invalid option value\n"
                       "Input KEXT directory is missing for -k option\n");
                goto fail;
            }
            efidir = argv[1];
            argc -= 2;
            argv += 2;
            continue;
        }

        if ((strcasecmp(argv[0], "-d") == 0) || (strcasecmp(argv[0], "--dsdt") == 0)) {
            if (argv[1] == NULL || argv[1][0] == '-') {
                printf("Invalid option value\n"
                       "Input DSDT.aml file is missing for -d option\n");
                goto fail;
            }
            dsdtfile = argv[1];
            argc -= 2;
            argv += 2;
            continue;
        }

        if ((strcasecmp(argv[0], "-i") == 0) || (strcasecmp(argv[0], "--input") == 0)) {
            if (argv[1] == NULL || argv[1][0] == '-') {
                printf("Invalid option value\n"
                       "Input file/directory is missing for -i option\n");
                goto fail;
            }
            inputpath = argv[1];
            argc -= 2;
            argv += 2;
            continue;
        }

        if ((strcasecmp(argv[0], "-o") == 0) || (strcasecmp(argv[0], "--out") == 0)) {
            if (argv[1] == NULL || argv[1][0] == '-') {
                printf("Invalid option value\n"
                       "Output file/directory is missing for -o option\n");
                goto fail;
            }
            output = argv[1];
            argc -= 2;
            argv += 2;
            continue;
        }

        if ((strcasecmp(argv[0], "-r") == 0) || (strcasecmp(argv[0], "--recent") == 0)) {
            if (argv[1] == NULL || argv[1][0] == '-') {
                printf("Invalid option value\n"
                       "Input file is missing for -r option\n");
                goto fail;
            }
            recent = argv[1];
            argc -= 2;
            argv += 2;
            continue;
        }

        if ((strcasecmp(argv[0], "-a") == 0) || (strcasecmp(argv[0], "--aggressivity") == 0)) {
            if (argv[1] == NULL || argv[1][0] == '-') {
                printf("Invalid option value\n"
                       "Value is missing for -a option\n");
                goto fail;
            }
            aggressivity = atoi(argv[1]);
            argc -= 2;
            argv += 2;
            continue;
        }

        if ((strcasecmp(argv[0], "-h") == 0) || (strcasecmp(argv[0], "--help") == 0)) {
            help = true;
            argc --;
            argv ++;
            continue;
        }

        printf("Unknown option: %s\n", argv[0]);

fail:
        printf("Exiting!\n");
        return ERR_GENERIC_CALL_NOT_SUPPORTED;
    }

    int cmds = dsdtextract + dsdtinject + ozmextract + ozmupdate + ozmcreate + kext2ffs + dsdt2bios;

    versionInfo();

    if (help) {
        if (cmds > 1)
            usageAll();
        else if (dsdtextract)
            usageDSDTExtract();
        else if (dsdtinject)
            usageDSDTInject();
        else if (ozmupdate)
            usageOzmUpdate();
        else if (ozmextract)
            usageOzmExtract();
        else if (ozmcreate)
            usageOzmCreate();
        else if (kext2ffs)
            usageKext2Ffs();
        else if (dsdt2bios)
            usageDsdt2Bios();
        else
            usageAll();

        return ERR_SUCCESS;
    }

    if (cmds == 0) {
        usageGeneral();
        printf("ERROR: No command supplied!\n");
        return ERR_GENERIC_CALL_NOT_SUPPORTED;
    }
    else if (cmds > 1) {
        usageGeneral();
        printf("ERROR: More than one command supplied, only one is allowed!\n");
        return ERR_GENERIC_CALL_NOT_SUPPORTED;
    }

    if (inputpath.isEmpty()) {
        printf("ERROR: No input file/dir specified!\n");
        return ERR_GENERIC_CALL_NOT_SUPPORTED;
    }

    if (output.isEmpty()) {
        printf("ERROR: No output file/dir specified!\n");
        return ERR_GENERIC_CALL_NOT_SUPPORTED;
    }

    if (ozmcreate && ffsdir.isEmpty()) {
        printf("ERROR: No FFS directory file supplied!\n");
        return ERR_GENERIC_CALL_NOT_SUPPORTED;
    }

    if (ozmupdate && recent.isEmpty()) {
        printf("ERROR: No \"recent/clean\" BIOS file supplied!\n");
        return ERR_GENERIC_CALL_NOT_SUPPORTED;
    }

    if ((dsdt2bios || dsdtinject) && dsdtfile.isEmpty()) {
        printf("ERROR: No DSDT.aml file supplied!\n");
        return ERR_GENERIC_CALL_NOT_SUPPORTED;
    }

    if (dsdtextract)
        result = w.DSDTExtract(inputpath, output);
    else if (dsdtinject)
        result = w.DSDTInject(inputpath, dsdtfile, output);
    else if (ozmupdate)
        result = w.OZMUpdate(inputpath, recent, output, aggressivity, compressdxe);
    else if (ozmextract)
        result = w.OZMExtract(inputpath, output);
    else if (ozmcreate)
        result = w.OZMCreate(inputpath, output, ffsdir, kextdir, efidir, dsdtfile, aggressivity, compressdxe, compresskexts);
    else if (kext2ffs)
        result = w.Kext2Ffs(inputpath, output);
    else if (dsdt2bios)
        result = w.DSDT2Bios(inputpath, dsdtfile, output);


    if(result) {
        printf("! Program exited with errors !\n");
        printf("\nStatus code: %i\n", result);
    }

    return result;
}
