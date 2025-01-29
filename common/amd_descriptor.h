/* amd_descriptor.h

Copyright (c) 2025 Patrick Rudolph. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHWARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*/

#ifndef AMD_DESCRIPTOR_H
#define AMD_DESCRIPTOR_H

#include "basetypes.h"
#include "ustring.h"
#include "ubytearray.h"

// Make sure we use right packing rules
#pragma pack(push,1)

typedef enum AMD_ADDR_MODE_ {
	AMD_ADDR_PHYSICAL = 0,	/* Physical address */
	AMD_ADDR_REL_BIOS,	/* Relative to beginning of image */
	AMD_ADDR_REL_TAB,	/* Relative to table */
	AMD_ADDR_REL_SLOT,	/* Relative to slot */
} AMD_ADDR_MODE;

/* An address can be relative to the image/file start but it can also be the address when
 * the image is mapped at 0xff000000. Used to ensure that we only attempt to read within
 * the limits of the file. */
#define SPI_ROM_BASE 0xff000000
#define FILE_REL_MASK 0xffffff

// Embedded firmware descriptor
typedef struct AMD_EMBEDDED_FIRMWARE_ {
    UINT32 Signature;             // 0x55aa55aa
    UINT32 IMC_Entry;             // Pointer to IMC blob
    UINT32 GEC_Entry;             // Pointer to GEC blob
    UINT32 xHCI_Entry;            // Pointer to xHCI blob
    UINT32 PSP_Directory;         // Use New_PSP_Directory when 0xffffffff
    UINT32 New_PSP_Directory;     // Could be upper 32-bit of PSP_Directory
    UINT32 BIOS0_Entry;           // Unused?
    UINT32 BIOS1_Entry;           // Used by EFS1.0
      // Might be a BIOS directory or Combo directory table
    UINT32 BIOS2_Entry;           // Unused?
    UINT32 Efs_Generation;        // only used after RAVEN/PICASSO
      // EFS 1.0
      //  PLATFORM_CARRIZO      15h (60-6fh)
      //  PLATFORM_STONEYRIDGE  15h (60-6fh)
      //  PLATFORM_RAVEN        17h (00-0fh)
      //  PLATFORM_PICASSO      17h (10-2fh)
      // EFS 2.0
      //  PLATFORM_RENOIR       17h (10-1fh)
      //  PLATFORM_LUCIENNE     17h (60-6fh)
      //  PLATFORM_CEZANNE      19h (50-5fh)
      //  PLATFORM_MENDOCINO    17h (A0-Afh)
      //  PLATFORM_PHOENIX      19h (70-7fh)
      //  PLATFORM_GLINDA       17h
      //  PLATFORM_GENOA        19h
    UINT32 BIOS3_Entry;         // only used when not using A/B recovery
      // Might be a BIOS directory or Combo directory table
    UINT32 Reserved_0;
    UINT32 Promontory_FW_PTR;
    UINT32 Reserved_1[6];
} AMD_EMBEDDED_FIRMWARE;

#define AMD_EFS_GEN1 0xFFFFFFFFUL

// PSP directory header
typedef struct AMD_PSP_DIRECTORY_HEADER_ {
    UINT32 Cookie;              // 0x50535024
    UINT32 Checksum;
    UINT32 Num_Entries;
    UINT32 Additional_Info_Fields;
} AMD_PSP_DIRECTORY_HEADER;

typedef struct AMD_PSP_DIRECTORY_ENTRY_ {
    UINT8  Type;
    UINT8  SubProg;
    UINT16 Flags;
    UINT32 Size;
    UINT64 Address_AddressMode;
} AMD_PSP_DIRECTORY_ENTRY;

// PSP combo directory header
typedef struct AMD_PSP_COMBO_DIRECTORY_HEADER_ {
    UINT32 Cookie;              // 0x50535032
    UINT32 Checksum;
    UINT32 Num_Entries;
    UINT32 Lookup;
    UINT64 Reserved[2];
} AMD_PSP_COMBO_DIRECTORY_HEADER;

typedef struct AMD_PSP_COMBO_ENTRY_ {
    UINT32 Id_Sel;
    UINT32 Id;
    UINT64 Lvl2_Address;
} AMD_PSP_COMBO_ENTRY;

typedef enum AMD_BIOS_TYPE_ {
	AMD_BIOS_SIG = 0x07,
	AMD_BIOS_APCB = 0x60,
	AMD_BIOS_APOB = 0x61,
	AMD_BIOS_BIN = 0x62,
	AMD_BIOS_APOB_NV = 0x63,
	AMD_BIOS_PMUI = 0x64,
	AMD_BIOS_PMUD = 0x65,
	AMD_BIOS_UCODE = 0x66,
	AMD_BIOS_APCB_BK = 0x68,
	AMD_BIOS_EARLY_VGA = 0x69,
	AMD_BIOS_MP2_CFG = 0x6a,
	AMD_BIOS_PSP_SHARED_MEM = 0x6b,
	AMD_BIOS_L2_PTR =  0x70,
	AMD_BIOS_INVALID,
	AMD_BIOS_SKIP
} AMD_BIOS_TYPE;

typedef enum AMD_FW_TYPE_ {
	AMD_FW_PSP_PUBKEY = 0x00,
	AMD_FW_PSP_BOOTLOADER = 0x01,
	AMD_FW_PSP_SECURED_OS = 0x02,
	AMD_FW_PSP_RECOVERY = 0x03,
	AMD_FW_PSP_NVRAM = 0x04,
	AMD_FW_RTM_PUBKEY = 0x05,
	AMD_FW_PSP_SMU_FIRMWARE = 0x08,
	AMD_FW_PSP_SECURED_DEBUG = 0x09,
	AMD_FW_ABL_PUBKEY = 0x0a,
	AMD_PSP_FUSE_CHAIN = 0x0b,
	AMD_FW_PSP_TRUSTLETS = 0x0c,
	AMD_FW_PSP_TRUSTLETKEY = 0x0d,
	AMD_FW_PSP_SMU_FIRMWARE2 = 0x12,
	AMD_DEBUG_UNLOCK = 0x13,
	AMD_FW_PSP_TEEIPKEY = 0x15,
	AMD_BOOT_DRIVER = 0x1b,
	AMD_SOC_DRIVER = 0x1c,
	AMD_DEBUG_DRIVER = 0x1d,
	AMD_INTERFACE_DRIVER = 0x1f,
	AMD_HW_IPCFG = 0x20,
	AMD_WRAPPED_IKEK = 0x21,
	AMD_TOKEN_UNLOCK = 0x22,
	AMD_SEC_GASKET = 0x24,
	AMD_MP2_FW = 0x25,
	AMD_DRIVER_ENTRIES = 0x28,
	AMD_FW_KVM_IMAGE = 0x29,
	AMD_FW_MP5 = 0x2a,
	AMD_S0I3_DRIVER = 0x2d,
	AMD_ABL0 = 0x30,
	AMD_ABL1 = 0x31,
	AMD_ABL2 = 0x32,
	AMD_ABL3 = 0x33,
	AMD_ABL4 = 0x34,
	AMD_ABL5 = 0x35,
	AMD_ABL6 = 0x36,
	AMD_ABL7 = 0x37,
	AMD_SEV_DATA = 0x38,
	AMD_SEV_CODE = 0x39,
	AMD_FW_PSP_WHITELIST = 0x3a,
	AMD_VBIOS_BTLOADER = 0x3c,
	AMD_FW_L2_PTR = 0x40,
	AMD_FW_DXIO = 0x42,
	AMD_FW_USB_PHY = 0x44,
	AMD_FW_TOS_SEC_POLICY = 0x45,
	AMD_FW_DRTM_TA = 0x47,
	AMD_FW_RECOVERYAB_A = 0x48,
	AMD_FW_RECOVERYAB_B = 0x4A,
	AMD_FW_BIOS_TABLE = 0x49,
	AMD_FW_KEYDB_BL = 0x50,
	AMD_FW_KEYDB_TOS = 0x51,
	AMD_FW_PSP_VERSTAGE = 0x52,
	AMD_FW_VERSTAGE_SIG = 0x53,
	AMD_RPMC_NVRAM = 0x54,
	AMD_FW_SPL = 0x55,
	AMD_FW_DMCU_ERAM = 0x58,
	AMD_FW_DMCU_ISR = 0x59,
	AMD_FW_MSMU = 0x5a,
	AMD_FW_SPIROM_CFG = 0x5c,
	AMD_FW_MPIO = 0x5d,
	AMD_FW_TPMLITE = 0x5f, /* family 17h & 19h */
	AMD_FW_PSP_SMUSCS = 0x5f, /* family 15h & 16h */
	AMD_FW_DMCUB = 0x71,
	AMD_FW_PSP_BOOTLOADER_AB = 0x73,
	AMD_RIB = 0x76,
	AMD_FW_AMF_SRAM = 0x85,
	AMD_FW_AMF_DRAM = 0x86,
	AMD_FW_MFD_MPM = 0x87,
	AMD_FW_AMF_WLAN = 0x88,
	AMD_FW_AMF_MFD = 0x89,
	AMD_FW_MPDMA_TF = 0x8c,
	AMD_TA_IKEK = 0x8d,
	AMD_FW_MPCCX = 0x90,
	AMD_FW_GMI3_PHY = 0x91,
	AMD_FW_MPDMA_PM = 0x92,
	AMD_FW_LSDMA = 0x94,
	AMD_FW_C20_MP = 0x95,
	AMD_FW_FCFG_TABLE = 0x98,
	AMD_FW_MINIMSMU = 0x9a,
	AMD_FW_GFXIMU_0 = 0x9b,
	AMD_FW_GFXIMU_1 = 0x9c,
	AMD_FW_GFXIMU_2 = 0x9d,
	AMD_FW_SRAM_FW_EXT = 0x9d,
	AMD_FW_UMSMU = 0xa2,
	AMD_FW_S3IMG = 0xa0,
	AMD_FW_USBDP = 0xa4,
	AMD_FW_USBSS = 0xa5,
	AMD_FW_USB4 = 0xa6,
	AMD_FW_IMC = 0x200,	/* Large enough to be larger than the top BHD entry type. */
	AMD_FW_GEC,
	AMD_FW_XHCI,
	AMD_FW_INVALID,		/* Real last one to detect the last entry in table. */
	AMD_FW_SKIP		/* This is for non-applicable options. */
} AMD_FW_TYPE;

#define AMD_MAX_PSP_ENTRIES 0xff

typedef struct AMD_ISH_DIRECTORY_TABLE_ {
	UINT32 Checksum;
	UINT32 Boot_Priority;
	UINT32 Update_Retry_Count;
	UINT8  Glitch_Retry_Count;
	UINT8  Glitch_Higherbits_Reserved[3];
	UINT32 Pl2_location;
	UINT32 Psp_Id;
	UINT32 Slot_Max_Size;
	UINT32 Reserved;
} AMD_ISH_DIRECTORY_TABLE;

// BIOS directory header
typedef struct AMD_BIOS_DIRECTORY_HEADER_ {
    UINT32 Cookie;              // 0x44484224
    UINT32 Checksum;
    UINT32 Num_Entries;
    UINT32 Additional_Info_Fields;
} AMD_BIOS_DIRECTORY_HEADER;

typedef struct AMD_BIOS_DIRECTORY_ENTRY_ {
    UINT8  Type;
    UINT8  RegionType;
    UINT16 Flags;
    UINT32 Size;
    UINT64 Address_AddressMode;
    UINT64 Destination;
} AMD_BIOS_DIRECTORY_ENTRY;
#define AMD_MAX_BIOS_ENTRIES 0x2f

// AMD signatures
#define AMD_EMBEDDED_FIRMWARE_SIGNATURE 0x55aa55aa
#define AMD_PSP_DIRECTORY_HEADER_SIGNATURE 0x50535024
#define AMD_PSPL2_DIRECTORY_HEADER_SIGNATURE 0x324c5024
#define AMD_BIOS_HEADER_SIGNATURE 0x44484224
#define AMD_BHDL2_HEADER_SIGNATURE 0x324c4224
#define AMD_PSP_COMBO_DIRECTORY_HEADER_SIGNATURE 0x50535032
#define AMD_PSP_BHD2_DIRECTORY_HEADER_SIGNATURE 0x44484232

#define AMD_EMBEDDED_FIRMWARE_OFFSET 0x20000

#endif // AMD_DESCRIPTOR_H
