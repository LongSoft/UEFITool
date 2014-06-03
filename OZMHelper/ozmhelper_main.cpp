/* ozmhelper_main.cpp

Copyright (c) 2014, Nikolaj Schlej. All rights reserved.
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

#include "ozmhelper.h"

QString version = "v0.1";
QString appname = "OZMHelper";

void usageDsdt2Bios()
{
    printf("dsdt2bios command\n"
            " usage:\n"
            "\t%s --dsdt2bios -i AmiBoardInfo.bin -d DSDT.aml -o outputdir\n\n"
            " parameters:\n" \
            "\t-i, --input\t\tInput file (AmiBoardInfo)\n"
            "\t-d, --dsdt\t\tDSDT.aml file\n"
            "\t-o, --out [dir]\t\tOutput directory\n"
            "\t-h, --help\t\tPrint this\n\n",qPrintable(appname));
}

void usageFfsConvert()
{
    printf("ffsconvert command\n"
            " usage:\n"
            "\t%s --ffsconvert -o outputdir -i ffsdir\n\n"
            " parameters:\n"
            "\t-i, --input\t\tInput directory\n"
            "\t-o, --out [dir]\t\tOutput directory\n"
            "\t-h, --help\t\tPrint this\n\n",qPrintable(appname));
}

void usageOzmCreate()
{
    printf("ozmcreate command\n" \
            " usage:\n"
            "\t%s --ozmcreate -k kextdir -f ffsdir -d DSDT.aml -o outputdir -i BIOS.ROM\n\n"
            " parameters:\n"
            "\t-k, --kext [dir]\t\tKEXT directory\n"
            "\t-f, --ffs [dir]\t\tFFS directory (OZM files)\n"
            "\t-d, --dsdt [file]\t\tDSDT.aml file\n"
            "\t-i, --input\t\tInput file\n"
            "\t-o, --out [dir]\t\tOutput directory\n"
            "\t-h, --help\t\tPrint this\n\n",qPrintable(appname));
}

void usageOzmExtract()
{
    printf("ozmextract command\n"
            " usage:\n"
            "\t%s --ozmextract -o outputdir -i OZM.ROM\n\n"
            " parameters:\n"
            "\t-i, --input\t\tInput file\n"
            "\t-o, --out [dir]\t\tOutput directory\n"
            "\t-h, --help\t\tPrint this\n\n",qPrintable(appname));
}

void usageDSDTExtract()
{
    printf("dsdtextract command\n"
            " usage:\n"
            "\t%s --dsdtextract -o outputdir -i OZMBIOS_or_BIOS.ROM\n\n"
            " parameters:\n"
            "\t-i, --input\t\tInput file\n"
            "\t-o, --out [dir]\t\tOutput directory\n"
            "\t-h, --help\t\tPrint this\n\n",qPrintable(appname));
}

void usageGeneral()
{
    printf("Usage:\n" \
            "\t%s [COMMAND] [PARAMETERS...]\n\n"
            "Available commands:\n"
            "\t--dsdtextract\t\tExtracts DSDT from BIOS\n"
            "\t--ozmextract\t\tExtracts Ozmosis files (ffs) from BIOS\n"
            "\t--ozmcreate\t\tPatches Original BIOS with Ozmosis\n"
            "\t--ffsconvert\t\tConverts kext-directories to FFS\n"
            "\t--dsdt2bios\t\tInjects (bigger) DSDT into AmiBoardInfo\n"
            "\t--help, -h\t\tPrint this\n\n",qPrintable(appname));
}

void versionInfo()
{
    printf("%s - %s\n",qPrintable(appname), qPrintable(version));
}

void usageAll()
{
    usageGeneral();
    usageDSDTExtract();
    usageOzmExtract();
    usageOzmCreate();
    usageFfsConvert();
    usageDsdt2Bios();
}

int main(int argc, char *argv[])
{
    bool help = false;
    bool dsdtextract = false;
    bool ozmextract = false;
    bool ozmcreate = false;
    bool ffsconvert = false;
    bool dsdt2bios = false;
    QString inputpath = "";
    QString output = "";
    QString ffsdir = "";
    QString kextdir = "";
    QString dsdtfile = "";

    QCoreApplication a(argc, argv);
    a.setOrganizationName("CodeRush");
    a.setOrganizationDomain("coderush.me");
    a.setApplicationName("OZMHelper");

    OZMHelper w;
    UINT8 result = ERR_SUCCESS;


    if (argc == 1) {
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

        if (strcasecmp(argv[0], "--ffsconvert") == 0) {
            ffsconvert = true;
            argc --;
            argv ++;
            continue;
        }

        if (strcasecmp(argv[0], "--dsdt2bios") == 0) {
            ffsconvert = true;
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
                       "Input file/dir is missing for -i option\n");
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
                       "Output directory is missing for -o option\n");
                goto fail;
            }
            output = argv[1];
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

    int cmds = dsdtextract + ozmextract + ozmcreate + ffsconvert;

    if (help) {
        if (cmds > 1)
            usageAll();
        else if (dsdtextract)
            usageDSDTExtract();
        else if (ozmextract)
            usageOzmExtract();
        else if (ozmcreate)
            usageOzmCreate();
        else if (ffsconvert)
            usageFfsConvert();
        else if (dsdt2bios)
            usageDsdt2Bios();
        else
            usageAll();

        return ERR_SUCCESS;
    }

    if (cmds == 0) {
        printf("ERROR: No command supplied!\n");
        return ERR_GENERIC_CALL_NOT_SUPPORTED;
    }
    else if (cmds > 1) {
        printf("ERROR: More than one command supplied, only one is allowed!\n");
        return ERR_GENERIC_CALL_NOT_SUPPORTED;
    }

    if (inputpath.isEmpty()) {
        printf("ERROR: No input file/dir specified!\n");
        return ERR_GENERIC_CALL_NOT_SUPPORTED;
    }

    if (output.isEmpty()) {
        printf("ERROR: No output dir specified!\n");
        return ERR_GENERIC_CALL_NOT_SUPPORTED;
    }

    if(ozmcreate && kextdir.isEmpty())
        printf("Warning: No KEXT-dir given! Injecting only Ozmosis files!\n");

    if (ozmcreate && (ffsdir.isEmpty()||dsdtfile.isEmpty())) {
        printf("ERROR: No FFS directory or DSDT.aml file supplied!\n");
        return ERR_GENERIC_CALL_NOT_SUPPORTED;
    }

    if (dsdt2bios && dsdtfile.isEmpty()) {
        printf("ERROR: No DSDT.aml file supplied!\n");
        return ERR_GENERIC_CALL_NOT_SUPPORTED;
    }

    if (dsdtextract)
        result = w.DSDTExtract(inputpath, output);
    else if (ozmextract)
        result = w.OZMExtract(inputpath, output);
    else if (ozmcreate)
        result = w.OZMCreate(inputpath, output, ffsdir, kextdir, dsdtfile);
    else if (ffsconvert)
        result = w.FFSConvert(inputpath, output);
    else if (dsdt2bios)
        result = w.DSDT2Bios(inputpath, dsdtfile, output);

    printf("Program exited %s!\n", result ? "with errors" : "successfully");
    if(result)
        printf("Status: %i\n", result);

    return result;
}
