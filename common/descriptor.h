/* descriptor.h

Copyright (c) 2016, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHWARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*/

#ifndef DESCRIPTOR_H
#define DESCRIPTOR_H

#include "basetypes.h"
#include "ustring.h"
#include "ubytearray.h"

// Make sure we use right packing rules
#pragma pack(push,1)

// Flash descriptor header
typedef struct FLASH_DESCRIPTOR_HEADER_ {
    UINT8  ReservedVector[16];     // Reserved for ARM ResetVector, 0xFFs on x86/x86-64 machines
    UINT32 Signature;              // 0x0FF0A55A
} FLASH_DESCRIPTOR_HEADER;

// Flash descriptor signature
#define FLASH_DESCRIPTOR_SIGNATURE 0x0FF0A55A

// Descriptor region size
#define FLASH_DESCRIPTOR_SIZE      0x1000

// Maximum base value in descriptor map
#define FLASH_DESCRIPTOR_MAX_BASE  0xE0

// Descriptor version was reserved in older firmware
#define FLASH_DESCRIPTOR_VERSION_INVALID 0xFFFFFFFF
// The only known version found in Coffee Lake
#define FLASH_DESCRIPTOR_VERSION_MAJOR   1
#define FLASH_DESCRIPTOR_VERSION_MINOR   0

// Descriptor version present in Coffee Lake and newer
typedef struct _FLASH_DESCRIPTOR_VERSION {
    UINT32 Reserved : 14;
    UINT32 Minor : 7;
    UINT32 Major : 11;
} FLASH_DESCRIPTOR_VERSION;

// Descriptor map
// Base fields are storing bits [11:4] of actual base addresses, all other bits are 0
typedef struct FLASH_DESCRIPTOR_MAP_ {
    // FLMAP0
    UINT32 ComponentBase : 8;
    UINT32 NumberOfFlashChips : 2;      // Zero-based number of flash chips installed on board
    UINT32 : 6;
    UINT32 RegionBase : 8;
    UINT32 NumberOfRegions : 3;         // Reserved in v2 descriptor
    UINT32 : 5;
    // FLMAP 1
    UINT32 MasterBase : 8;
    UINT32 NumberOfMasters : 2;         // Zero-based number of flash masters
    UINT32 : 6;
    UINT32 PchStrapsBase : 8;
    UINT32 NumberOfPchStraps : 8;       // One-based number of UINT32s to read as PCH straps, min=0, max=255 (1 Kb)
    // FLMAP 2
    UINT32 ProcStrapsBase : 8;
    UINT32 NumberOfProcStraps : 8;      // One-based number of UINT32s to read as processor straps, min=0, max=255 (1 Kb)
    UINT32 : 16;
    // FLMAP 3
    UINT32 DescriptorVersion;           // Reserved prior to Coffee Lake
} FLASH_DESCRIPTOR_MAP;

// Component section structure
// Flash parameters DWORD structure
typedef struct FLASH_PARAMETERS_ {
    UINT16 : 16;
    UINT8 : 1;
    UINT8 ReadClockFrequency : 3; // Hardcoded value of 20 Mhz (000b) in v1 descriptors
    UINT8 : 4;
    UINT8 : 8;
} FLASH_PARAMETERS;

typedef struct FLASH_DESCRIPTOR_COMPONENT_SECTION_ {
    FLASH_PARAMETERS FlashParameters;      // Bit field with SPI flash parameters, changes almost every CPU generation, so will remain mostly undefined for now
    UINT8            InvalidInstruction0;  // Instructions for SPI chip, that must not be executed, like FLASH ERASE
    UINT8            InvalidInstruction1;  //
    UINT8            InvalidInstruction2;  //
    UINT8            InvalidInstruction3;  //
    UINT16           PartitionBoundary;    // Upper 16 bit of partition boundary address. Default is 0x0000, which makes the boundary to be 0x00001000
    UINT16 : 16;
} FLASH_DESCRIPTOR_COMPONENT_SECTION;

// Region section
// All base and limit register are storing upper part of actual UINT32 base and limit
// If limit is zero - region is not present
typedef struct FLASH_DESCRIPTOR_REGION_SECTION_ {
    UINT16 DescriptorBase;             // Descriptor
    UINT16 DescriptorLimit;            //
    UINT16 BiosBase;                   // BIOS
    UINT16 BiosLimit;                  //
    UINT16 MeBase;                     // Management Engine
    UINT16 MeLimit;                    //
    UINT16 GbeBase;                    // Gigabit Ethernet
    UINT16 GbeLimit;                   //
    UINT16 PdrBase;                    // Platform Data
    UINT16 PdrLimit;                   //
    UINT16 DevExp1Base;                // Device Expansion 1
    UINT16 DevExp1Limit;               //
    UINT16 Bios2Base;                  // Secondary BIOS
    UINT16 Bios2Limit;                 //
    UINT16 MicrocodeBase;              // CPU microcode
    UINT16 MicrocodeLimit;             //
    UINT16 EcBase;                     // Embedded Controller
    UINT16 EcLimit;                    //
    UINT16 DevExp2Base;                // Device Expansion 2
    UINT16 DevExp2Limit;               //
    UINT16 IeBase;                     // Innovation Engine
    UINT16 IeLimit;                    //
    UINT16 Tgbe1Base;                  // 10 Gigabit Ethernet 1
    UINT16 Tgbe1Limit;                 //
    UINT16 Tgbe2Base;                  // 10 Gigabit Ethernet 2
    UINT16 Tgbe2Limit;                 //
    UINT16 Reserved1Base;              // Reserved 1
    UINT16 Reserved1Limit;             //
    UINT16 Reserved2Base;              // Reserved 2
    UINT16 Reserved2Limit;             //
    UINT16 PttBase;                    // Platform Trust Technology
    UINT16 PttLimit;                   //
} FLASH_DESCRIPTOR_REGION_SECTION;

// Master section
typedef struct FLASH_DESCRIPTOR_MASTER_SECTION_ {
    UINT16 BiosId;
    UINT8 BiosRead;
    UINT8 BiosWrite;
    UINT16 MeId;
    UINT8 MeRead;
    UINT8 MeWrite;
    UINT16 GbeId;
    UINT8 GbeRead;
    UINT8 GbeWrite;
} FLASH_DESCRIPTOR_MASTER_SECTION;

// Master section v2 (Skylake+)
typedef struct FLASH_DESCRIPTOR_MASTER_SECTION_V2_ {
    UINT32 : 8;
    UINT32 BiosRead : 12;
    UINT32 BiosWrite : 12;
    UINT32 : 8;
    UINT32 MeRead : 12;
    UINT32 MeWrite : 12;
    UINT32 : 8;
    UINT32 GbeRead : 12;
    UINT32 GbeWrite : 12;
    UINT32 : 32;
    UINT32 : 8;
    UINT32 EcRead : 12;
    UINT32 EcWrite : 12;
} FLASH_DESCRIPTOR_MASTER_SECTION_V2;

// Region access bits in master section
#define FLASH_DESCRIPTOR_REGION_ACCESS_DESC 0x01
#define FLASH_DESCRIPTOR_REGION_ACCESS_BIOS 0x02
#define FLASH_DESCRIPTOR_REGION_ACCESS_ME   0x04
#define FLASH_DESCRIPTOR_REGION_ACCESS_GBE  0x08
#define FLASH_DESCRIPTOR_REGION_ACCESS_PDR  0x10
#define FLASH_DESCRIPTOR_REGION_ACCESS_EC   0x20

// Base address of descriptor upper map
#define FLASH_DESCRIPTOR_UPPER_MAP_BASE 0x0EFC

// Descriptor upper map structure
typedef struct FLASH_DESCRIPTOR_UPPER_MAP_ {
    UINT8 VsccTableBase; // Base address of VSCC Table for ME, bits [11:4]
    UINT8 VsccTableSize; // Counted in UINT32s
    UINT16 ReservedZero; // Still unknown, zero in all descriptors I have seen
} FLASH_DESCRIPTOR_UPPER_MAP;

// VSCC table entry structure
typedef struct VSCC_TABLE_ENTRY_ {
    UINT8   VendorId;          // JEDEC VendorID byte
    UINT8   DeviceId0;         // JEDEC DeviceID first byte
    UINT8   DeviceId1;         // JEDEC DeviceID second byte
    UINT8   ReservedZero;      // Reserved, must be zero
    UINT32  VsccRegisterValue; // VSCC register value
} VSCC_TABLE_ENTRY;

// Base address and size of OEM section
#define FLASH_DESCRIPTOR_OEM_SECTION_BASE 0x0F00
#define FLASH_DESCRIPTOR_OEM_SECTION_SIZE 0x100


// AMD signatures
#define AMD_EMBEDDED_FIRMWARE_SIGNATURE             0x55AA55AA
#define AMD_PSP_DIRECTORY_HEADER_SIGNATURE          0x50535024  // "$PSP"
#define AMD_PSPL2_DIRECTORY_HEADER_SIGNATURE        0x324C5024  // "$PL2"
#define AMD_BIOS_HEADER_SIGNATURE                   0x44484224  // "$BHD"
#define AMD_BHDL2_HEADER_SIGNATURE                  0x324C4224  // "$BL2"
#define AMD_PSP_COMBO_DIRECTORY_HEADER_SIGNATURE    0x50535032  // "2PSP"
#define AMD_PSP_BHD2_DIRECTORY_HEADER_SIGNATURE     0x44484232  // "2BHD"

#define AMD_EFS_GEN1                                0xFFFFFFFFUL

#define AMD_EMBEDDED_FIRMWARE_OFFSET                0x20000

#define AMD_INVALID_SIZE                            0xFFFFFFFFUL

/* An address can be relative to the image/file start but it can also be the address when
 * the image is mapped at 0xff000000. Used to ensure that we only attempt to read within
 * the limits of the file. */
#define SPI_ROM_BASE                                0xFF000000UL
#define FILE_REL_MASK                               (~SPI_ROM_BASE)

typedef enum AMD_ADDR_MODE_ {
    AMD_ADDR_PHYSICAL = 0,	    // Physical address
    AMD_ADDR_REL_BIOS,	        // Relative to beginning of image
    AMD_ADDR_REL_TABLE,	        // Relative to table
    AMD_ADDR_REL_SLOT,	        // Relative to table entry
} AMD_ADDR_MODE;

typedef enum AMD_FW_TYPE_ {
    AMD_FW_PSP_PUBKEY = 0x00,
    AMD_FW_PSP_BOOTLOADER = 0x01,
    AMD_FW_PSP_SECURED_OS = 0x02,
    AMD_FW_PSP_RECOVERY = 0x03,
    AMD_FW_PSP_NVRAM = 0x04,
    AMD_FW_RTM_PUBKEY = 0x05,
    AMD_FW_BIOS_RTM = 0x06,
    AMD_FW_PSP_SMU_FIRMWARE = 0x08,
    AMD_FW_PSP_SECURED_DEBUG = 0x09,
    AMD_FW_ABL_PUBKEY = 0x0a,
    AMD_PSP_FUSE_CHAIN = 0x0b,
    AMD_FW_PSP_TRUSTLETS = 0x0c,
    AMD_FW_PSP_TRUSTLETKEY = 0x0d,
    AMD_FW_AGESA_RESUME = 0x10,
    AMD_FW_PSP_SMU_FIRMWARE2 = 0x12,
    AMD_DEBUG_UNLOCK = 0x13,
    AMD_PSP_MCLF_TRUSTLETS = 0x14, // ???
    AMD_FW_PSP_TEEIPKEY = 0x15,
    AMD_SEV_DRIVER = 0x1a,
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
    AMD_FET_BACKUP = 0x46,
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
    AMD_FW_TOS_WL_BIN = 0x9f,
    AMD_FW_UMSMU = 0xa2,
    AMD_FW_S3IMG = 0xa0,
    AMD_FW_USBDP = 0xa4,
    AMD_FW_USBSS = 0xa5,
    AMD_FW_USB4 = 0xa6,
} AMD_FW_TYPE;

typedef enum AMD_BIOS_TYPE_ {
    AMD_BIOS_SIG = 0x07,
    AMD_BIOS_APCB = 0x60,
    AMD_BIOS_APOB = 0x61,
    AMD_BIOS_BIN = 0x62,
    AMD_BIOS_APOB_NV = 0x63,
    AMD_BIOS_PMUI = 0x64,
    AMD_BIOS_PMUD = 0x65,
    AMD_BIOS_UCODE = 0x66,
    AMD_BIOS_FHP_DRIVER = 0x67,
    AMD_BIOS_APCB_BK = 0x68,
    AMD_BIOS_EARLY_VGA = 0x69,
    AMD_BIOS_MP2_CFG = 0x6a,
    AMD_BIOS_PSP_SHARED_MEM = 0x6b,
    AMD_BIOS_L2_PTR = 0x70,
} AMD_BIOS_TYPE;

// Embedded firmware descriptor
typedef struct AMD_EMBEDDED_FIRMWARE_ {
    UINT32 Signature;           // 0x55AA55AA
    UINT32 IMC_Firmware;        // Pointer to IMC blob
    UINT32 GEC_Firmware;        // Pointer to GEC blob
    UINT32 xHCI_Firmware;       // Pointer to xHCI blob
    UINT32 PSP_Directory;       // Use NewPSP_Directory when 0 or AMD_INVALID_SIZE
    UINT32 NewPSP_Directory;    // Could be upper 32-bit of PSP_Directory
    UINT32 BIOS0_Entry;         // Unused?
    UINT32 BIOS1_Entry;         // Used by EFS1.0
    // Might be a BIOS directory or Combo directory table
    UINT32 BIOS2_Entry;         // Unused?
    UINT32 EFS_Generation;      // only used after RAVEN/PICASSO
    // EFS 1.0
    //  PLATFORM_CARRIZO      15h (60-6fh)  10220B00h
    //  PLATFORM_STONEYRIDGE  15h (60-6fh)  10220B00h
    //  PLATFORM_RAVEN        17h (00-0fh)  BC0A0000h
    //  PLATFORM_PICASSO      17h (10-2fh)  BC0A0000h
    // EFS 2.0
    //  PLATFORM_RENOIR       17h (10-1fh)  BC0C0000h
    //  PLATFORM_LUCIENNE     17h (60-6fh)  BC0C0000h
    //  PLATFORM_CEZANNE      19h (50-5fh)  BC0C0140h
    //  PLATFORM_MENDOCINO    17h (A0-Afh)  BC0D0900h
    //  PLATFORM_PHOENIX      19h (70-7fh)  BC0D0400h
    //  PLATFORM_GLINDA       17h
    //  PLATFORM_GENOA        19h           BC0C0111h
    //  PLATFORM_FAEGAN       19h           BC0E1000h
    UINT32 BIOS3_Entry;         // only used when not using A/B recovery
    // Might be a BIOS directory or Combo directory table
    UINT32 BackupPSP_Directory;
    UINT32 Promontory_Firmware;
    UINT32 Reserved_1[6];
} AMD_EMBEDDED_FIRMWARE;

#define AMD_ADDITIONAL_INFO_V0_DEF \
    UINT32               \
        DirSize     : 10,\
        SpiBlockSize: 4, \
        BaseAddress : 15,\
        AddrMode    : 2, \
        Version     : 1

#define AMD_ADDITIONAL_INFO_V1_DEF \
    UINT32                \
        DirSize      : 16,\
        SpiBlockSize : 4, \
        DirHeaderSize: 4, \
        AddrMode     : 2, \
        Reserved     : 5, \
        Version      : 1

#define AMD_ADDITIONAL_INFO_DEF \
union {                             \
    struct {                        \
        AMD_ADDITIONAL_INFO_V0_DEF; \
    };                              \
    struct {                        \
        AMD_ADDITIONAL_INFO_V1_DEF; \
    } v1;                           \
}

typedef struct AMD_ADDITIONAL_INFO_ {
    union {
        UINT32 raw;
        struct {
            AMD_ADDITIONAL_INFO_DEF;
        };
    };
} AMD_ADDITIONAL_INFO;

#define AMD_COMMON_HEADER_DEF \
    UINT32 Cookie;   \
    UINT32 Checksum; \
    UINT32 NumEntries

// Common part of PSP/BIOS/Combo headers
typedef struct AMD_COMMON_HEADER_ {
    AMD_COMMON_HEADER_DEF;
} AMD_COMMON_HEADER;

#define AMD_PSPBIOS_COMMON_HEADER_DEF \
    AMD_COMMON_HEADER_DEF;                  \
    union {                                 \
        AMD_ADDITIONAL_INFO AdditionalInfo; \
        struct {                            \
            AMD_ADDITIONAL_INFO_DEF;        \
        };                                  \
    }

// Common part of PSP/BIOS headers
typedef struct AMD_PSPBIOS_COMMON_HEADER_ {
    AMD_PSPBIOS_COMMON_HEADER_DEF;
} AMD_PSPBIOS_COMMON_HEADER;

// PSP directory header
typedef struct AMD_PSP_DIRECTORY_HEADER_ {
    AMD_PSPBIOS_COMMON_HEADER_DEF;  // cookie = 0x50535024
} AMD_PSP_DIRECTORY_HEADER;

#define AMD_ADDRESS_ADDRESSMODE_DEF \
    UINT64               \
        Address     : 62,\
        AddrMode    : 2

typedef struct AMD_ADDRESS_ADDRESSMODE_ {
    union {
        UINT64 raw;
        struct {
            AMD_ADDRESS_ADDRESSMODE_DEF;
        };
    };
} AMD_ADDRESS_ADDRESSMODE;

#define AMD_PSP_DIRECTORY_ENTRY_FLAGS_DEF \
    UINT16              \
        RomId       : 2,\
        Writable    : 1,\
        Instance    : 4,\
        Reserved    : 9

typedef struct AMD_PSP_DIRECTORY_ENTRY_FLAGS_ {
    union {
        UINT16 raw;
        struct {
            AMD_PSP_DIRECTORY_ENTRY_FLAGS_DEF;
        };
    };
} AMD_PSP_DIRECTORY_ENTRY_FLAGS;

typedef struct AMD_PSP_DIRECTORY_ENTRY_ {
    UINT8  Type;
    UINT8  SubProgram;
    union {
        AMD_PSP_DIRECTORY_ENTRY_FLAGS Flags;
        struct {
            AMD_PSP_DIRECTORY_ENTRY_FLAGS_DEF;
        };
    };
    UINT32 Size;
    union {
        AMD_ADDRESS_ADDRESSMODE AddressMode;
        struct {
            AMD_ADDRESS_ADDRESSMODE_DEF;
        };
    };
} AMD_PSP_DIRECTORY_ENTRY;

// PSP combo directory header
typedef struct AMD_PSP_COMBO_DIRECTORY_HEADER_ {
    AMD_COMMON_HEADER_DEF;  // cookie = 0x50535032
    UINT32 Lookup; // 0 - by PSP Id, 1 - by Family Id
    UINT64 Reserved[2];
} AMD_PSP_COMBO_DIRECTORY_HEADER;

typedef struct AMD_PSP_COMBO_ENTRY_ {
    UINT32 IdSel; // 0 - Id is PSP Id, 1 - Id is Family Id
    UINT32 Id;
    UINT32 L2Address;
    UINT32 Reserved; // ???
} AMD_PSP_COMBO_ENTRY;

// BIOS directory header
typedef struct AMD_BIOS_DIRECTORY_HEADER_ {
    AMD_PSPBIOS_COMMON_HEADER_DEF;  // cookie = 0x44484224
} AMD_BIOS_DIRECTORY_HEADER;

#define AMD_BIOS_DIRECTORY_ENTRY_FLAGS_DEF \
    UINT16              \
        ResetImage  : 1,\
        CopyImage   : 1,\
        ReadOnly    : 1,\
        Compressed  : 1,\
        Instance    : 4,\
        SubProgram  : 3,\
        RomId       : 2,\
        Writable    : 1,\
        Reserved    : 2

typedef struct AMD_BIOS_DIRECTORY_ENTRY_FLAGS_ {
    union {
        UINT16 raw;
        struct {
            AMD_BIOS_DIRECTORY_ENTRY_FLAGS_DEF;
        };
    };
} AMD_BIOS_DIRECTORY_ENTRY_FLAGS;

typedef struct AMD_BIOS_DIRECTORY_ENTRY_ {
    UINT8  Type;
    UINT8  RegionType;
    union {
        AMD_BIOS_DIRECTORY_ENTRY_FLAGS Flags;
        struct {
            AMD_BIOS_DIRECTORY_ENTRY_FLAGS_DEF;
        };
    };
    UINT32 Size;
    union {
        AMD_ADDRESS_ADDRESSMODE AddressMode;
        struct {
            AMD_ADDRESS_ADDRESSMODE_DEF;
        };
    };
    UINT64 Destination;
} AMD_BIOS_DIRECTORY_ENTRY;

typedef struct AMD_ISH_DIRECTORY_TABLE_ {
    UINT32 Checksum;
    UINT32 BootPriority; // 0xFFFFFFFF: A/B, 1: B/A
    UINT32 UpdateRetryCount;
    UINT8  GlitchRetryCount;
    UINT8  Reserved_1[3];
    UINT32 L2Address;
    UINT32 PspId;
    UINT32 SlotMaxSize;
    UINT32 Reserved_2;
} AMD_ISH_DIRECTORY_TABLE;

// Restore previous packing rules
#pragma pack(pop)

// Calculate address of data structure addressed by descriptor address format
// 8 bit base or limit
extern const UINT8* calculateAddress8(const UINT8* baseAddress, const UINT8 baseOrLimit);
// 16 bit base or limit
extern const UINT8* calculateAddress16(const UINT8* baseAddress, const UINT16 baseOrLimit);

// Calculate offset of region using it's base
extern UINT32 calculateRegionOffset(const UINT16 base);
// Calculate size of region using it's base and limit
extern UINT32 calculateRegionSize(const UINT16 base, const UINT16 limit);

// Return human-readable chip name for given JEDEC ID
extern UString jedecIdToUString(UINT8 vendorId, UINT8 deviceId0, UINT8 deviceId1);

#endif // DESCRIPTOR_H
