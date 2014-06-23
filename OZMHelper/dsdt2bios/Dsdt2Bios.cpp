//
//  DragDropView.m
//  AmiInfoBoard
//
//  Created by Frédéric Geoffroy on 14/04/2014.
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
#include <capstone/capstone.h>

#include "Dsdt2Bios.h"
#include "PeImage.h"

#define debug TRUE
static csh handle;

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
                if ((op->mem.disp > 0x900) && (op->mem.disp < 0xf000) ) r = op->mem.disp;
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
    struct platform platforms =
    {
        .arch = CS_ARCH_X86,
                .mode = CS_MODE_64,
                .code = (UINT8 *)X86_CODE64,
                .size = CodeSize - 1,
                .comment = "X86 64 (Intel syntax)"
    };
    
    cs_insn *insn;
    int i;
    
    cs_err err = cs_open(platforms.arch, platforms.mode, &handle);
    if (err) {
        printf("\n\n\n\n\n\n\n\nFailed on cs_open() with error returned: %u\n", err);
        return ERR_ERROR;
    }

    if (platforms.opt_type)
        cs_option(handle, platforms.opt_type, platforms.opt_value);

    cs_option(handle, CS_OPT_DETAIL, CS_OPT_ON);

    size_t count = cs_disasm_ex(handle, platforms.code, platforms.size, address, 0, &insn);
    if (count)
    {
        size_t j;
        for (j = 0; j < count; j++)
        {
            if ( insn_detail(handle, platforms.mode, &insn[j]) != 0)
            {
                unsigned short *adr = (unsigned short *)&X86_CODE64[insn[j].address+3];
                *adr += size;
                if ( debug ) printf("%s\t%s \t-> \t[0x%x]\n", insn[j].mnemonic, insn[j].op_str,*adr);
                ret = ERR_SUCCESS;
            }
        }
        // free memory allocated by cs_disasm_ex()
        cs_free(insn, count);
    }
    else
    {
        printf("\n\n\n\n\n\n\n\nERROR: Failed to disasm given code!\n");
        return ERR_ERROR;
    }
    cs_close(&handle);
    return ret;
}

UINT8 Dsdt2Bios::getFromAmiBoardInfo(QByteArray amiboard, UINT32 & DSDTOffset, UINT32 & DSDTSize)
{
    INT32  ret;
    UINT32 offset;
    UINT32 size = 0;
    EFI_IMAGE_DOS_HEADER *HeaderDOS;
    
    if(!amiboard.size()) {
        printf("ERROR: AmiBoardInfo is empty. Aborting!\n");
        return ERR_FILE_READ;
    }
/*
    else if(amiboard.size() > 0xFFFF) {
        printf("ERROR: AmiBoardInfo exceeds maximal size of %i(0x%X). Aborting!\n", 0xFFFF, 0xFFFF);
        return ERR_FILE_READ;
    }
*/
    
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

UINT8 Dsdt2Bios::injectIntoAmiBoardInfo(QByteArray amiboard, QByteArray dsdt, UINT32 DSDTOffsetOld, UINT32 DSDTSizeOld, QByteArray & out, BOOLEAN firstRun, UINT16 relocPadding)
{
    int i, j;
    UINT8 ret;
    INT32 DSDTSizeDiff, padding;
    UINT32 DSDTSizeNew = 0;
    UINT32 DSDTLen, AmiLen;
    QByteArray amiToEdit;
    
    
    EFI_IMAGE_DOS_HEADER *HeaderDOS;
    EFI_IMAGE_NT_HEADERS64 *HeaderNT;
    EFI_IMAGE_SECTION_HEADER *Section;

    amiToEdit = amiboard;
    
    DSDTLen = dsdt.size(); //physical size
    AmiLen = amiToEdit.size();
    
    if(!dsdt.startsWith(DSDT_HEADER)) {
        printf("ERROR: DSDT has invalid header. Aborting!\n");
        return ERR_INVALID_FILE;
    }
    
    if(dsdt.indexOf(UNPATCHABLE_SECTION) > 0) {
        printf("ERROR: AmiBoardInfo contains '.ROM' section => unpatchable atm. Aborting!\n");
        return ERR_INVALID_SECTION;
    }

    DSDTSizeNew = (DSDTSizeNew << 8) + dsdt.at(4);
    DSDTSizeNew = (DSDTSizeNew << 8) + dsdt.at(5);
    DSDTSizeNew = (DSDTSizeNew << 8) + dsdt.at(6);
    DSDTSizeNew = (DSDTSizeNew << 8) + dsdt.at(7);
    DSDTSizeNew = qFromBigEndian(DSDTSizeNew);
    
    if(DSDTSizeNew != DSDTLen) {
        printf("ERROR: Size of DSDT differs from passed data to in-code define. Aborting!\n");
        return ERR_ERROR;
    }

    DSDTSizeDiff = DSDTSizeNew - DSDTSizeOld;
    padding = (0x10-(AmiLen+DSDTSizeDiff))&0x0f;
    DSDTSizeDiff += padding + relocPadding;

/*
    if ((AmiLen+DSDTSizeDiff) > 0xFFFF)
    {
        printf("ERROR: Final size exceeds limit of %i (0x%X). Aborting!\n", 0xFFFF, 0xFFFF);
        return ERR_BUFFER_TOO_SMALL;
    }
*/
    
    amiToEdit = amiToEdit.replace(DSDTOffsetOld+DSDTSizeOld+DSDTSizeDiff, AmiLen-DSDTOffsetOld-DSDTSizeOld, amiToEdit.constData()+DSDTOffsetOld+DSDTSizeOld);
    memset(amiToEdit.mid(DSDTOffsetOld).data_ptr(), 0, DSDTSizeNew+padding+relocPadding);
    amiToEdit = amiToEdit.replace(DSDTOffsetOld, DSDTSizeNew, dsdt);
    
    HeaderDOS = (EFI_IMAGE_DOS_HEADER *)amiToEdit.data_ptr();
    HeaderNT = (EFI_IMAGE_NT_HEADERS64 *)amiToEdit.mid(HeaderDOS->e_lfanew).data_ptr();
    
    if ( debug ) printf("Patching header\n");
    if ( debug ) printf("---------------\n\n");
    if ( debug ) printf("SizeOfInitializedData       \t0x%x",HeaderNT->OptionalHeader.SizeOfInitializedData);
    HeaderNT->OptionalHeader.SizeOfInitializedData += DSDTSizeDiff;
    if ( debug ) printf("\t -> \t0x%x\n",HeaderNT->OptionalHeader.SizeOfInitializedData);
    if ( debug ) printf("SizeOfImage                 \t0x%x",HeaderNT->OptionalHeader.SizeOfImage);
    HeaderNT->OptionalHeader.SizeOfImage += DSDTSizeDiff;
    if ( debug ) printf("\t -> \t0x%x\n",HeaderNT->OptionalHeader.SizeOfImage);
    
    for ( i = 0; i < EFI_IMAGE_NUMBER_OF_DIRECTORY_ENTRIES ;i++)
    {
        if ( HeaderNT->OptionalHeader.DataDirectory[i].VirtualAddress != 0 )
        {
            if ( debug ) printf("DataDirectory               \t0x%x",HeaderNT->OptionalHeader.DataDirectory[i].VirtualAddress);
            HeaderNT->OptionalHeader.DataDirectory[i].VirtualAddress += DSDTSizeDiff;
            if ( debug ) printf("\t -> \t0x%x\n\n",HeaderNT->OptionalHeader.DataDirectory[i].VirtualAddress);
        }
    }
    
    
    Section = (EFI_IMAGE_SECTION_HEADER *)amiToEdit.mid(HeaderDOS->e_lfanew+sizeof(EFI_IMAGE_NT_HEADERS64)).data_ptr();//[HeaderDOS->e_lfanew+sizeof(EFI_IMAGE_NT_HEADERS64)];
    if ( debug ) printf("Patching sections\n");
    if ( debug ) printf("-----------------\n\n");

    UINT32 Found = 0;

    for ( i = 0 ; i < HeaderNT->FileHeader.NumberOfSections; i++)
    {
        if ( !strcmp((char *)&Section[i].Name, ".data" ) ) Found = 1;
        if ( Found )
        {
            if ( !strcmp((char *)&Section[i].Name, ".data" ) )
            {
    
                if ( debug ) printf("Name                         \t%s\t -> \t %s\n",Section[i].Name,Section[i].Name);
                if ( debug ) printf("PhysicalAddress             \t0x%x",Section[i].Misc.PhysicalAddress);
                Section[i].Misc.PhysicalAddress += DSDTSizeDiff;
                if ( debug ) printf("\t -> \t0x%x\n",Section[i].Misc.PhysicalAddress);
                if ( debug ) printf("SizeOfRawData               \t0x%x",Section[i].SizeOfRawData);
                Section[i].SizeOfRawData += DSDTSizeDiff;
                if ( debug ) printf("\t -> \t0x%x\n\n",Section[i].SizeOfRawData);
            }
            else
            {
                if (!strcmp((char *)&Section[i].Name,"")) strcpy((char *)&Section[i].Name,".empty");
                if ( debug ) printf("Name                        \t%s\t -> \t%s\n",Section[i].Name,Section[i].Name);
                if ( debug ) printf("VirtualAddress              \t0x%x",Section[i].VirtualAddress);
                Section[i].VirtualAddress += DSDTSizeDiff;
                if ( debug ) printf("\t -> \t0x%x\n",Section[i].VirtualAddress);
                if ( debug ) printf("PointerToRawData            \t0x%x",Section[i].PointerToRawData);
                Section[i].PointerToRawData += DSDTSizeDiff;
                if ( debug ) printf("\t -> \t0x%x\n\n",Section[i].PointerToRawData);
                
                if ( !strcmp((char *)&Section[i].Name, ".reloc" ) )
                {
                    
                    if ( debug ) printf("Patching relocations\n");
                    if ( debug ) printf("--------------------\n\n");
                    
    
                    EFI_IMAGE_BASE_RELOCATION *p;
                    UINT16 *s;
                    UINT32 Offset = 0;
                    UINT32 sizeSection = 0;
                    UINT32 index = 0;
                    UINT32 OldAdr = 0;
                    UINT32 OldOfs = 0;
                    do
                    {
                        p = (EFI_IMAGE_BASE_RELOCATION *)(amiToEdit.mid(Section[i].PointerToRawData).data_ptr()) + Offset;
                        Offset = p->SizeOfBlock / sizeof(UINT32);
                        sizeSection += p->SizeOfBlock;
                        s = (UINT16 *)p + 4;
                        
                        index = 0;
                        if ( debug ) printf("Virtual base address           \t0x%04x",p->VirtualAddress);
                        OldAdr = p->VirtualAddress;
                        if (p->VirtualAddress != 0 ) p->VirtualAddress =(AmiLen + DSDTSizeDiff) & 0xf000;
                        
                        if ( debug ) printf("\t -> \t0x%04x\n",p->VirtualAddress);

                        for ( j = 0; j <  ( p->SizeOfBlock - 8 ); j+=2)
                        {
                            if (*s != 0)
                            {
                                if ( debug ) printf("Table index %i                \t0x%04x",index++, OldAdr + (*s & 0xfff));
                                if (p->VirtualAddress != 0 ) *s = 0xa000 + ((*s + DSDTSizeDiff ) & 0xfff);
                                if ( debug ) printf("\t -> \t0x%04x\n",p->VirtualAddress + (*s & 0xfff));
                            }
                            if (p->VirtualAddress != 0 )OldOfs = *s & 0xfff;
                            s++;
                            if (p->VirtualAddress != 0 )
                            {
                                if (j < ( p->SizeOfBlock - 8 - 4) )
                                {
                                    if ( OldOfs > ((*s +DSDTSizeDiff) & 0xfff))
                                    {
                                        relocPadding = ( 0x10 + (0x1000 - OldOfs)) & 0xff0 ;
                                        if(firstRun) {
                                            printf("Failed on 1st run.. retrying with padding of %x!\n",relocPadding);
                                            ret = injectIntoAmiBoardInfo(amiboard, dsdt, DSDTOffsetOld, DSDTSizeOld, out, FALSE, relocPadding); //Recalling THIS function
                                        }
                                        else
                                            printf("ERROR: Sorry, failed on second patching run. Aborting!\n");

                                        return ret;
                                    }
                                }
                            }
                        }

                    }while (sizeSection < Section[i].Misc.VirtualSize );
                }
            }
        }
    }

    if ( debug ) printf("Patching adr in code\n");
    if ( debug ) printf("--------------------\n\n");
    if (!Disass((UINT8 *)amiToEdit.mid(HeaderNT->OptionalHeader.BaseOfCode).data_ptr(),HeaderNT->OptionalHeader.SizeOfCode, DSDTSizeDiff))
    {
        printf("Successfully patched AmiBoardInfo to new offset :) All credits to FredWst!\n");
        out.clear();
        out.append(amiToEdit, AmiLen+DSDTSizeDiff);
    }
    else {
        printf("AmiBoardInfo Code not patched :( All credits to noob tuxuser, who fucked up!\n\n");
        return ERR_ERROR;
    }

    return ERR_SUCCESS;
}
