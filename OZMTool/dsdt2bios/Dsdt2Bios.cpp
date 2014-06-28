//
//  DragDropView.m
//  AmiInfoBoard
//
//  Created by Frédéric Geoffroy on 14/04/2014.
//  Ported to QT & edited by tuxuser
//  Copyright (c) 2014 FredWst. All rights reserved.
//

//***************************************************
//  main.c
//  Dsdt2Bios
//
//  Created by FredWst on 09/04/2014.
//
// **************************************************
//
// Dsdt2Bios using capstone lib engine to patch code.
//
// http://www.capstone-engine.org
//
//***************************************************

#include <QtEndian>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <capstone.h>

#include "Dsdt2Bios.h"
#include "PeImage.h"

#define DEBUG 0

#if DEBUG
    #define dprintf(...) \
            do { printf(__VA_ARGS__); } while (0)
#else
    #define dprintf(...)
#endif

UINT64 Dsdt2Bios::insn_detail(csh ud, cs_mode mode, cs_insn *ins)
{
    int i;
    UINT64 r = 0;
    cs_x86 *x86 = &(ins->detail->x86);
    
    for (i = 0; i < x86->op_count; i++)
    {
        cs_x86_op *op = &(x86->operands[i]);
        switch((int)op->type)
        {
        case X86_OP_MEM:
            if ((op->mem.disp > 0x900) && (op->mem.disp < 0xf000) )
                r = op->mem.disp;
            break;
        default:
            break;
        }
    }
    return r;
}

UINT8 Dsdt2Bios::Disass(UINT8 *X86_CODE64, INT32 CodeSize, INT32 size)
{
    UINT8 ret = ERR_ERROR;
    UINT64 address = 0;
    
    cs_insn *insn;
    static csh handle;

    struct platform x64_platform =
    {
        .arch = CS_ARCH_X86,
                .mode = CS_MODE_64,
                .code = (unsigned char *)X86_CODE64,
                .size = CodeSize - 1,
                .comment = "X86 64 (Intel syntax)"
    };

    cs_err err = cs_open(x64_platform.arch, x64_platform.mode, &handle);
    if (err) {
        dprintf("\nFailed on cs_open() with error returned: %u\n", err);
        return ERR_ERROR;
    }

    if (x64_platform.opt_type)
        cs_option(handle, x64_platform.opt_type, x64_platform.opt_value);

    cs_option(handle, CS_OPT_DETAIL, CS_OPT_ON);

    size_t count = cs_disasm_ex(handle, x64_platform.code, x64_platform.size, address, 0, &insn);
    if (!count) {
        dprintf("\nERROR: Failed to disasm given code!\n");
        cs_close(&handle);
        return ERR_ERROR;
    }

    size_t j;
    for (j = 0; j < count; j++)
    {
        if ( insn_detail(handle, x64_platform.mode, &insn[j]) == 0)
            continue;

        unsigned short *adr = (unsigned short *)&X86_CODE64[insn[j].address+3];
        *adr += size;
        dprintf("%s\t%s \t-> \t[0x%x]\n", insn[j].mnemonic, insn[j].op_str,*adr);
        ret = ERR_SUCCESS;
    }
    // free memory allocated by cs_disasm_ex()
    cs_free(insn, count);
    cs_close(&handle);
    return ret;
}

UINT8 Dsdt2Bios::getDSDTFromAmi(QByteArray amiboard, UINT32 &DSDTOffset, UINT32 &DSDTSize)
{
    INT32  ret;
    UINT32 offset;
    UINT32 size = 0;
    EFI_IMAGE_DOS_HEADER *HeaderDOS;
    
    if(!amiboard.size()) {
        printf("ERROR: AmiBoardInfo is empty. Aborting!\n");
        return ERR_FILE_READ;
    }
    
    HeaderDOS = (EFI_IMAGE_DOS_HEADER *)amiboard.constData();
    
    if (HeaderDOS->e_magic != 0x5A4D ) {
        printf("Error: Invalid file, not AmiBoardInfo. Aborting!\n");
        return ERR_INVALID_FILE;
    }
    
    ret = amiboard.indexOf(DSDT_HEADER);
    if(ret < 0) {
        printf("ERROR: DSDT wasn't found in AmiBoardInfo");
        return ERR_FILE_NOT_FOUND;
    }

    offset = ret;

    size = (size << 8) + amiboard.at(offset+4);
    size = (size << 8) + amiboard.at(offset+5);
    size = (size << 8) + amiboard.at(offset+6);
    size = (size << 8) + amiboard.at(offset+7);
    size = qFromBigEndian(size);

    if(size > (amiboard.size()-offset)) {
        printf("ERROR: Read invalid size from DSDT. Aborting!\n");
        return ERR_INVALID_PARAMETER;
    }

    DSDTSize = size;
    DSDTOffset = offset;

    return ERR_SUCCESS;
}



UINT8 Dsdt2Bios::injectDSDTIntoAmi(QByteArray ami, QByteArray dsdt, UINT32 DSDTOffsetOld, UINT32 DSDTSizeOld, QByteArray & out, UINT32 & relocPadding)
{
    int i, j;
    UINT32 DSDTLen, amiLen;
    INT32 diffSize, padding;
    UINT32 DSDTSizeNew;
    bool foundDataSection = false;

    QByteArray amiBufNew;
    QByteArray padData;
    
    EFI_IMAGE_DOS_HEADER *HeaderDOS;
    EFI_IMAGE_NT_HEADERS64 *HeaderNT;
    EFI_IMAGE_SECTION_HEADER *Section;
    
    DSDTLen = dsdt.size(); //physical size
    amiLen = ami.size();
    
    if(!dsdt.startsWith(DSDT_HEADER)) {
        printf("ERROR: DSDT has invalid header. Aborting!\n");
        return ERR_INVALID_FILE;
    }
    
    if(ami.indexOf(UNPATCHABLE_SECTION) > 0) {
        printf("ERROR: AmiBoardInfo contains '.ROM' section => unpatchable atm. Aborting!\n");
        return ERR_INVALID_SECTION;
    }

    DSDTSizeNew = 0;
    DSDTSizeNew = (DSDTSizeNew << 8) + dsdt.at(4);
    DSDTSizeNew = (DSDTSizeNew << 8) + dsdt.at(5);
    DSDTSizeNew = (DSDTSizeNew << 8) + dsdt.at(6);
    DSDTSizeNew = (DSDTSizeNew << 8) + dsdt.at(7);
    DSDTSizeNew = qFromBigEndian(DSDTSizeNew);


    diffSize = DSDTSizeNew - DSDTSizeOld;
    padding = 0x10-(amiLen+diffSize)&0x0f;
    diffSize += padding + relocPadding;
    
    // Copying data *till* DSDT
    amiBufNew.append(ami.constData(),DSDTOffsetOld);
    // Copy new DSDT
    amiBufNew.append(dsdt.constData(), DSDTLen);
    // Pad the file
    padData.fill(0, padding+relocPadding);
    amiBufNew.append(padData);
    // Copy the rest
    amiBufNew.append(ami.mid(DSDTOffsetOld+DSDTSizeOld).constData(),(amiLen-DSDTOffsetOld-DSDTSizeOld));
    
    HeaderDOS = (EFI_IMAGE_DOS_HEADER *)amiBufNew.constData();
    HeaderNT = (EFI_IMAGE_NT_HEADERS64 *)amiBufNew.mid(HeaderDOS->e_lfanew).constData();
    
    dprintf("Patching header\n");
    dprintf("---------------\n\n");
    dprintf("SizeOfInitializedData       \t0x%x",HeaderNT->OptionalHeader.SizeOfInitializedData);
    HeaderNT->OptionalHeader.SizeOfInitializedData += diffSize;
    dprintf("\t -> \t0x%x\n",HeaderNT->OptionalHeader.SizeOfInitializedData);
    dprintf("SizeOfImage                 \t0x%x",HeaderNT->OptionalHeader.SizeOfImage);
    HeaderNT->OptionalHeader.SizeOfImage += diffSize;
    dprintf("\t -> \t0x%x\n",HeaderNT->OptionalHeader.SizeOfImage);
    
    for ( i = 0; i < EFI_IMAGE_NUMBER_OF_DIRECTORY_ENTRIES ;i++) {
        if ( HeaderNT->OptionalHeader.DataDirectory[i].VirtualAddress != 0 ) {
            dprintf("DataDirectory               \t0x%x",HeaderNT->OptionalHeader.DataDirectory[i].VirtualAddress);
            HeaderNT->OptionalHeader.DataDirectory[i].VirtualAddress += diffSize;
            dprintf("\t -> \t0x%x\n\n",HeaderNT->OptionalHeader.DataDirectory[i].VirtualAddress);
        }
    }
    
    
    Section = (EFI_IMAGE_SECTION_HEADER *)amiBufNew.mid(HeaderDOS->e_lfanew+sizeof(EFI_IMAGE_NT_HEADERS64)).constData();//[HeaderDOS->e_lfanew+sizeof(EFI_IMAGE_NT_HEADERS64)];
    dprintf("Patching sections\n");
    dprintf("-----------------\n\n");

    for ( i = 0 ; i < HeaderNT->FileHeader.NumberOfSections; i++)
    {
        if ( !strcmp((char *)&Section[i].Name, ".data" ) )
        {
            foundDataSection = TRUE;
            dprintf("Name                         \t%s\t -> \t %s\n",Section[i].Name,Section[i].Name);
            dprintf("PhysicalAddress             \t0x%x",Section[i].Misc.PhysicalAddress);
            Section[i].Misc.PhysicalAddress += diffSize;
            dprintf("\t -> \t0x%x\n",Section[i].Misc.PhysicalAddress);
            dprintf("SizeOfRawData               \t0x%x",Section[i].SizeOfRawData);
            Section[i].SizeOfRawData += diffSize;
            dprintf("\t -> \t0x%x\n\n",Section[i].SizeOfRawData);
        }
        else if (foundDataSection)
        {
            if (!strcmp((char *)&Section[i].Name,""))
                strcpy((char *)&Section[i].Name,".empty");
            dprintf("Name                        \t%s\t -> \t%s\n",Section[i].Name,Section[i].Name);
            dprintf("VirtualAddress              \t0x%x",Section[i].VirtualAddress);
            Section[i].VirtualAddress += diffSize;
            dprintf("\t -> \t0x%x\n",Section[i].VirtualAddress);
            dprintf("PointerToRawData            \t0x%x",Section[i].PointerToRawData);
            Section[i].PointerToRawData += diffSize;
            dprintf("\t -> \t0x%x\n\n",Section[i].PointerToRawData);

            if ( !strcmp((char *)&Section[i].Name, ".reloc" ) )
            {
                dprintf("Patching relocations\n");
                dprintf("--------------------\n\n");

                EFI_IMAGE_BASE_RELOCATION *p;
                UINT16 *s;
                UINT32 Offset = 0;
                UINT32 sizeSection = 0;
                UINT32 index = 0;
                UINT32 OldAdr = 0;
                UINT32 OldOfs = 0;
                do
                {
                    p = (EFI_IMAGE_BASE_RELOCATION *)((UINT32 *)(amiBufNew.mid(Section[i].PointerToRawData).constData()) + Offset);
                    Offset = p->SizeOfBlock / sizeof(UINT32);
                    sizeSection += p->SizeOfBlock;
                    s = (UINT16 *)p + 4;

                    index = 0;
                    dprintf("Virtual base address           \t0x%04x",p->VirtualAddress);
                    OldAdr = p->VirtualAddress;
                    if (p->VirtualAddress != 0 )
                        p->VirtualAddress =(amiLen + diffSize) & 0xf000;

                    dprintf("\t -> \t0x%04x\n",p->VirtualAddress);

                    for ( j = 0; j <  ( p->SizeOfBlock - 8 ); j+=2)
                    {
                        if (*s != 0)
                        {
                            dprintf("Table index %i                \t0x%04x",index++, OldAdr + (*s & 0xfff));
                            if (p->VirtualAddress != 0 )
                                *s = 0xa000 + ((*s + diffSize ) & 0xfff);
                            dprintf("\t -> \t0x%04x\n",p->VirtualAddress + (*s & 0xfff));
                        }

                        if (p->VirtualAddress != 0 )
                            OldOfs = *s & 0xfff;

                        s++;

                        if ((p->VirtualAddress != 0)&&(OldOfs > ((*s +diffSize) & 0xfff))&&(j < ( p->SizeOfBlock - 8 - 4)))
                        {
                            relocPadding = ( 0x10 + (0x1000 - OldOfs)) & 0xff0 ;
                            dprintf("ERROR: errcode %x patching relocations!\n",relocPadding);
                            return ERR_RELOCATION;
                        }
                    }

                }while (sizeSection < Section[i].Misc.VirtualSize );
            }
        }
    }

    dprintf("Patching adr in code\n");
    dprintf("--------------------\n\n");
    if (!Disass((UINT8 *)amiBufNew.mid(HeaderNT->OptionalHeader.BaseOfCode).constData(),HeaderNT->OptionalHeader.SizeOfCode, diffSize))
    {
        printf("* Successfully patched AmiBoardInfo to new offset :) All credits to FredWst!\n");
        out.clear();
        out.append(amiBufNew.constData(), amiLen+diffSize);
    }
    else {
        printf("AmiBoardInfo Code not patched :( Blame noob tuxuser, who fucked up!\n\n");
        return ERR_ERROR;
    }

    return ERR_SUCCESS;
}
