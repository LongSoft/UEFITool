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

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <capstone/capstone.h>

#include "PeImage.h"

#define debug 1

static csh handle;

struct platform
{
	cs_arch arch;
	cs_mode mode;
	unsigned char *code;
	size_t size;
	char *comment;
	cs_opt_type opt_type;
	cs_opt_value opt_value;
};

static UINT64 insn_detail(csh ud, cs_mode mode, cs_insn *ins)
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

static int Disass(unsigned char *X86_CODE64, int CodeSize, int size)
{
    uint64_t address = 0;
    int ret = 0;
	struct platform platforms[] =
    {
		{
			.arch = CS_ARCH_X86,
			.mode = CS_MODE_64,
			.code = (unsigned char *)X86_CODE64,
			.size = CodeSize - 1,
			.comment = "X86 64 (Intel syntax)"
		},
	};
    
	cs_insn *insn;
	int i;
    
	for (i = 0; i < sizeof(platforms)/sizeof(platforms[0]); i++)
    {
		cs_err err = cs_open(platforms[i].arch, platforms[i].mode, &handle);
		if (err) {
            printf("\n\n\n\n\n\n\n\nFailed on cs_open() with error returned: %u\n", err);
            return 0;
		}
        
		if (platforms[i].opt_type)
			cs_option(handle, platforms[i].opt_type, platforms[i].opt_value);
        
		cs_option(handle, CS_OPT_DETAIL, CS_OPT_ON);
        
		size_t count = cs_disasm_ex(handle, platforms[i].code, platforms[i].size, address, 0, &insn);
		if (count)
        {
			size_t j;
			for (j = 0; j < count; j++)
            {
				if ( insn_detail(handle, platforms[i].mode, &insn[j]) != 0)
                {
                    unsigned short *adr = (unsigned short *)&X86_CODE64[insn[j].address+3];
                    *adr += size;
                    if ( debug ) printf("%s\t%s \t-> \t[0x%x]\n", insn[j].mnemonic, insn[j].op_str,*adr);
                    ret = 1;
                }
            }
            // free memory allocated by cs_disasm_ex()
			cs_free(insn, count);
		}
        else
        {
            printf("\n\n\n\n\n\n\n\nERROR: Failed to disasm given code!\n");
            return 0;
		}
		cs_close(&handle);
	}
    return ret;
}

unsigned int Read_AmiBoardInfo(const char *FileName, unsigned char *d,unsigned long *len, unsigned short *Old_Dsdt_Size, unsigned short *Old_Dsdt_Ofs, int Extract)
{
    int fd_amiboard, fd_out;
    EFI_IMAGE_DOS_HEADER *HeaderDOS;
    
    fd_amiboard = open(FileName, O_RDWR | O_NONBLOCK);
    if (fd_amiboard < 0)
    {
        printf("\n\n\n\n\n\n\n\nFile %s does not exist\n",FileName);
        return 0;
    }
    //Get size of AmiBoardInfo in Header
    *len = read(fd_amiboard, d, 0xFFFF);
    close(fd_amiboard);
    
    HeaderDOS = (EFI_IMAGE_DOS_HEADER *)&d[0];
    
    if (HeaderDOS->e_magic != 0x5A4D )
    {
        if (!((d[0] =='D') && (d[1] =='S') && (d[2] =='D') && (d[3] =='T')))
        {
            printf("\n\n\n\n\n\n\n\nFile %s has bad header\n",FileName);
            return 0;
        }
        else
            return 2;
    }
    
    for ( *Old_Dsdt_Ofs = 0; *Old_Dsdt_Ofs < *len ; *Old_Dsdt_Ofs+=1)
    {
        if ((d[*Old_Dsdt_Ofs] =='D') && (d[*Old_Dsdt_Ofs+1] =='S') && (d[*Old_Dsdt_Ofs+2] =='D') && (d[*Old_Dsdt_Ofs+3] =='T'))
        {
            *Old_Dsdt_Size = (d[*Old_Dsdt_Ofs+5] << 8) + d[*Old_Dsdt_Ofs+4];
            break;
        }
    }
    
    if ( Extract )
    {
        char *homeDir = getenv("HOME");
        char FileOutput[256];
        strcpy(FileOutput,homeDir);
        strcat(FileOutput,"/Desktop/DSDT-Original.aml");
        remove(FileOutput);
        fd_out = open(FileOutput, O_CREAT | O_RDWR | O_NONBLOCK, 0666);
        write(fd_out,&d[*Old_Dsdt_Ofs],*Old_Dsdt_Size);
        close(fd_out);
        //printf("DSDT-Original.aml has been successfully created\n\n");
        if ( Extract == 1 )
        {
            printf("\n\nDSDT-Original.aml and AmiBoardInfo.bin\n");
            printf("HAVE BEEN SUCCESSFULLY CREATED ON YOUR DESKTOP\n\n");
        }
        else
        {
            printf("\n\n\n\n\n\n\nDSDT-Original.aml\n\n");
            printf("HAS BEEN SUCCESSFULLY CREATED ON YOUR DESKTOP\n\n");
        }
    }
    
    return 1;
}

unsigned int Read_Dsdt(const char *FileName, unsigned char *d, unsigned long len, unsigned short Old_Dsdt_Size, unsigned short Old_Dsdt_Ofs, unsigned short *reloc_padding)
{
    int fd_dsdt, fd_out, i, j;
    unsigned long dsdt_len;
    short size, padding;
    unsigned char *dsdt;
    unsigned short New_Dsdt_Size;
    
    
    EFI_IMAGE_DOS_HEADER *HeaderDOS;
    EFI_IMAGE_NT_HEADERS64 *HeaderNT;
    EFI_IMAGE_SECTION_HEADER *Section;

    
    dsdt = malloc(0x10000);
    
    
    fd_dsdt = open(FileName, O_RDWR | O_NONBLOCK);
    
    if (fd_dsdt < 0)
    {
        printf("\n\n\n\n\n\n\n\nFile %s does not exist\n",FileName);
        free(dsdt);
        return 0;
    }
    //Read DSDT into buffer
    dsdt_len = read(fd_dsdt, dsdt, 0xFFFF);
    close(fd_dsdt);
    
    if (!((dsdt[0] =='D') && (dsdt[1] =='S') && (dsdt[2] =='D') && (dsdt[3] =='T')))
    {
        printf("\n\n\n\n\n\n\n\nFile %s has bad header\n",FileName);
        free(dsdt);
        return 0;
    }
    
    
    //Check is not in good place but it's faster to me to put here ;)
    for (i = 0 ; i < len ; i++)
    {
        if (((d[i+0] =='.') && (d[i+1] =='R') && (d[i+2] =='O') && (d[i+3] =='M')))
        {
            printf("\n\n\n\n\n\n\n\nFile has .ROM section, it can't be patched\n");
            free(dsdt);
            return 0;
        }
    }
        
    New_Dsdt_Size = (dsdt[5] << 8) + dsdt[4];
    
    size = New_Dsdt_Size - Old_Dsdt_Size;
    padding = (0x10-(len+size))&0x0f;
    size += padding + *reloc_padding;
    
    if ((len+size) > 0xFFFF)
    {
        printf("\n\n\n\n\n\n\n\nFinal size > 0xFFFF not tested aborting\n");
        free(dsdt);
        return 0;
    }
    
    memcpy(&d[Old_Dsdt_Ofs+Old_Dsdt_Size+size],&d[Old_Dsdt_Ofs+Old_Dsdt_Size],len - Old_Dsdt_Ofs - Old_Dsdt_Size);

    memset(&d[Old_Dsdt_Ofs],0x00,New_Dsdt_Size + padding + *reloc_padding);
    
    
    memcpy(&d[Old_Dsdt_Ofs],dsdt,New_Dsdt_Size);
    
    
    HeaderDOS = (EFI_IMAGE_DOS_HEADER *)&d[0];
    HeaderNT = (EFI_IMAGE_NT_HEADERS64 *)&d[HeaderDOS->e_lfanew];
    
    if ( debug ) printf("Patching header\n");
    if ( debug ) printf("---------------\n\n");
    if ( debug ) printf("SizeOfInitializedData       \t0x%x",HeaderNT->OptionalHeader.SizeOfInitializedData);
    HeaderNT->OptionalHeader.SizeOfInitializedData += size;
    if ( debug ) printf("\t -> \t0x%x\n",HeaderNT->OptionalHeader.SizeOfInitializedData);
    if ( debug ) printf("SizeOfImage                 \t0x%x",HeaderNT->OptionalHeader.SizeOfImage);
    HeaderNT->OptionalHeader.SizeOfImage += size;
    if ( debug ) printf("\t -> \t0x%x\n",HeaderNT->OptionalHeader.SizeOfImage);
    
    for ( i = 0; i < EFI_IMAGE_NUMBER_OF_DIRECTORY_ENTRIES ;i++)
    {
        if ( HeaderNT->OptionalHeader.DataDirectory[i].VirtualAddress != 0 )
        {
            if ( debug ) printf("DataDirectory               \t0x%x",HeaderNT->OptionalHeader.DataDirectory[i].VirtualAddress);
            HeaderNT->OptionalHeader.DataDirectory[i].VirtualAddress += size;
            if ( debug ) printf("\t -> \t0x%x\n\n",HeaderNT->OptionalHeader.DataDirectory[i].VirtualAddress);
        }
    }
    
    
    Section = (EFI_IMAGE_SECTION_HEADER *)&d[HeaderDOS->e_lfanew+sizeof(EFI_IMAGE_NT_HEADERS64)];
    if ( debug ) printf("Patching sections\n");
    if ( debug ) printf("-----------------\n\n");
    unsigned int Found = 0;
    

    for ( i = 0 ; i < HeaderNT->FileHeader.NumberOfSections; i++)
    {
        if ( !strcmp((char *)&Section[i].Name, ".data" ) ) Found = 1;
        if ( Found )
        {
            if ( !strcmp((char *)&Section[i].Name, ".data" ) )
            {
    
                if ( debug ) printf("Name                         \t%s\t -> \t %s\n",Section[i].Name,Section[i].Name);
                if ( debug ) printf("PhysicalAddress             \t0x%x",Section[i].Misc.PhysicalAddress);
                Section[i].Misc.PhysicalAddress += size;
                if ( debug ) printf("\t -> \t0x%x\n",Section[i].Misc.PhysicalAddress);
                if ( debug ) printf("SizeOfRawData               \t0x%x",Section[i].SizeOfRawData);
                Section[i].SizeOfRawData += size;
                if ( debug ) printf("\t -> \t0x%x\n\n",Section[i].SizeOfRawData);
            }
            else
            {
                if (!strcmp((char *)&Section[i].Name,"")) strcpy((char *)&Section[i].Name,".empty");
                if ( debug ) printf("Name                        \t%s\t -> \t%s\n",Section[i].Name,Section[i].Name);
                if ( debug ) printf("VirtualAddress              \t0x%x",Section[i].VirtualAddress);
                Section[i].VirtualAddress += size;
                if ( debug ) printf("\t -> \t0x%x\n",Section[i].VirtualAddress);
                if ( debug ) printf("PointerToRawData            \t0x%x",Section[i].PointerToRawData);
                Section[i].PointerToRawData += size;
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
                        p = (UINT32 *)(&d[Section[i].PointerToRawData]) + Offset;
                        Offset = p->SizeOfBlock / sizeof(UINT32);
                        sizeSection += p->SizeOfBlock;
                        s = (UINT16 *)p + 4;
                        
                        index = 0;
                        if ( debug ) printf("Virtual base address           \t0x%04x",p->VirtualAddress);
                        OldAdr = p->VirtualAddress;
                        if (p->VirtualAddress != 0 ) p->VirtualAddress =(len + size) & 0xf000;
                        
                        if ( debug ) printf("\t -> \t0x%04x\n",p->VirtualAddress);

                        for ( j = 0; j <  ( p->SizeOfBlock - 8 ); j+=2)
                        {
                            if (*s != 0)
                            {
                                if ( debug ) printf("Table index %i                \t0x%04x",index++, OldAdr + (*s & 0xfff));
                                if (p->VirtualAddress != 0 ) *s = 0xa000 + ((*s + size ) & 0xfff);
                                if ( debug ) printf("\t -> \t0x%04x\n",p->VirtualAddress + (*s & 0xfff));
                            }
                            if (p->VirtualAddress != 0 )OldOfs = *s & 0xfff;
                            s++;
                            if (p->VirtualAddress != 0 )
                            {
                                if (j < ( p->SizeOfBlock - 8 - 4) )
                                {
                                    if ( OldOfs > ((*s +size) & 0xfff))
                                    {
                                        *reloc_padding = ( 0x10 + (0x1000 - OldOfs)) & 0xff0 ;
                                        if ( debug ) printf(" error %04X \n",*reloc_padding);
                                        goto error; //sorry it's not à beautifull end of prog ;)
                                    }
                                }
                            }
                        }

                    }while (sizeSection < Section[i].Misc.VirtualSize );
                }
            }
        }
    }
   
    char *homeDir = getenv("HOME");
    char FileOutput[256];
    strcpy(FileOutput,homeDir);
    strcat(FileOutput,"/Desktop/AmiBoardInfo.bin");
    remove(FileOutput);
    if ( debug ) printf("Patching adr in code\n");
    if ( debug ) printf("--------------------\n\n");
    if ( Disass(&d[HeaderNT->OptionalHeader.BaseOfCode],HeaderNT->OptionalHeader.SizeOfCode, size) )
    {
        fd_out = open(FileOutput, O_CREAT | O_RDWR | O_NONBLOCK, 0666 );
        write(fd_out,d,len+size);
        close(fd_out);
        //printf("\nDSDT-Original.aml has been successfully created\n\n");
        //printf("AmiBoardInfo.bin has been successfully created\n\n";
    }
    else
        printf("\n\n\n\n\n\n\n\nCode not patched, AmiBoardInfo.bin has not been created\n\n");
  
error:
    free(dsdt);
    
    return 1;
}
