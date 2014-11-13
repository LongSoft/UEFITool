#ifndef COMMON_H
#define COMMON_H

#include <QString>
#include "../peimage.h"
#include "../ffs.h"

#if defined(_WIN32) || defined(_WIN64)  
  #define strcasecmp _stricmp
#endif

/* RETURN CODES */
#define ERR_DIR_NOT_EXIST                   0xEB
#define ERR_FILE_NOT_FOUND                  0xEC
#define ERR_FILE_EXISTS                     0xED
#define ERR_REPLACE                         0xEE
#define ERR_RELOCATION                      0xEF
#define ERR_ERROR                           0xF0

/* DSDT STUFF */
#define DSDT_HEADER "DSDT"
#define DSDT_HEADER_SZ 4
#define UNPATCHABLE_SECTION ".ROM"
#define DYNAMIC_BASE        0x40
#define MAX_DSDT    0x3FFFF

/* COMPRESS RUN */
#define RUN_AS_IS           0
#define RUN_DELETE          1
#define RUN_DEL_OZM_NREQ    2

/* KEXT CONVERSION */
#define MIN_KEXT_ID 0xA
#define MAX_KEXT_ID 0xF

#define ALIGN16(Value) (((Value)+15) & ~15)
#define ALIGN32(Value) (((Value)+31) & ~31)

const static QString ozmDefaultsFilename = "OzmosisDefaults.plist";
static const QString DSDTFilename =  "DSDT.aml";
const static QString kextGUID = "DADE100%1-1B31-4FE4-8557-26FCEFC78275";

/* PE IMAGE */
///
/// @attention
/// EFI_IMAGE_HEADERS64 is for use ONLY by tools.
///
typedef struct {
    UINT32                      Signature;
    EFI_IMAGE_FILE_HEADER       FileHeader;
    EFI_IMAGE_OPTIONAL_HEADER64 OptionalHeader;
} EFI_IMAGE_NT_HEADERS64;

#define EFI_IMAGE_SIZEOF_NT_OPTIONAL64_HEADER sizeof (EFI_IMAGE_NT_HEADERS64)

typedef struct {
    UINT16 offset: 12;
    UINT16 type:4;
} RELOC_ENTRY;

#define EFI_IMAGE_SIZEOF_RELOC_ENTRY sizeof (RELOC_ENTRY)

#define SRC_NOT_SET    0
#define SRC_KEXT       1
#define SRC_BINARY     2
#define SRC_EFI        3

/* FILE ENTRIES */
struct sectionEntry {
    QString name;
    QString GUID;
    QString srcName;
    UINT8 srcType;
    UINT8 sectionType;
    BOOLEAN required;
};

#define OZMFFS_SIZE 17

static const sectionEntry OzmFfs[] = {
    // Filesystem
    {"PartitionDxe","1FA1F39E-FEFF-4AAE-BD7B-38A070A3B609", "partitiondxe.efi", SRC_EFI, EFI_SECTION_PE32, TRUE},
    {"EnhancedFat","961578FE-B6B7-44C3-AF35-6BC705CD2B1F", "enhancedfat.efi", SRC_EFI, EFI_SECTION_PE32, TRUE},
    {"HfsPlus","4CF484CD-135F-4FDC-BAFB-1AA104B48D36", "hfsplus.efi", SRC_EFI, EFI_SECTION_PE32, TRUE},
    {"ExtFs","B34E5765-2E04-4DAF-867F-7F40BE6FC33D", "extfs.efi", SRC_EFI, EFI_SECTION_PE32, FALSE},
    // Bootloader
    {"Ozmosis","AAE65279-0761-41D1-BA13-4A3C1383603F", "ozmosis.efi", SRC_EFI, EFI_SECTION_PE32, TRUE},
    // EFI / EDK Shell
    {"HermitShellX64","C57AD6B7-0515-40A8-9D21-551652854E37", "hermitshellx64.efi", SRC_EFI, EFI_SECTION_PE32, FALSE},
    // Defaults
    {"OzmosisDefaults","99F2839C-57C3-411E-ABC3-ADE5267D960D", "Defaults.plist", SRC_BINARY, EFI_SECTION_RAW, TRUE},
    // Theme
    {"OzmosisTheme","AC255206-DCF9-4837-8353-72BBBC0AC849", "Theme.bin", SRC_BINARY, EFI_SECTION_RAW, TRUE},
    // Kernel Extensions
    {"SmcEmulatorKext","DADE1001-1B31-4FE4-8557-26FCEFC78275", "FakeSMC.kext", SRC_KEXT, EFI_SECTION_RAW, TRUE},
    {"DisablerKext","DADE1002-1B31-4FE4-8557-26FCEFC78275", "Disabler.kext", SRC_KEXT, EFI_SECTION_RAW, FALSE},
    {"InjectorKext","DADE1003-1B31-4FE4-8557-26FCEFC78275", "Injector.kext", SRC_KEXT, EFI_SECTION_RAW, FALSE},
    {"PostbootMounter","DADE1004-1B31-4FE4-8557-26FCEFC78275", "PostbootMounter.kext", SRC_KEXT, EFI_SECTION_RAW, FALSE},
    {"PostbootSymbols","DADE1005-1B31-4FE4-8557-26FCEFC78275", "PostbootSymbols.kext", SRC_KEXT, EFI_SECTION_RAW, FALSE},
    {"CpuSensorsKext","DADE1006-1B31-4FE4-8557-26FCEFC78275", "CPUSensors.kext", SRC_KEXT, EFI_SECTION_RAW, FALSE},
    {"LpcSensorsKext","DADE1007-1B31-4FE4-8557-26FCEFC78275", "LPCSensors.kext", SRC_KEXT, EFI_SECTION_RAW, FALSE},
    {"GpuSensorsKext","DADE1008-1B31-4FE4-8557-26FCEFC78275", "GPUSensors.kext", SRC_KEXT, EFI_SECTION_RAW, FALSE},
    {"VoodooHdaKext", "DADE1009-1B31-4FE4-8557-26FCEFC78275", "VoodooHDA.kext", SRC_KEXT, EFI_SECTION_RAW, FALSE}
};

#define DELETABLEFFS_SIZE 10

static const sectionEntry deletableFfs[] = {
  {"TcpDxe","B1625D3C-9D2D-4E0D-B864-8A763EE4EC50", "tcpdxe.efi", SRC_EFI, EFI_SECTION_PE32, FALSE},
  {"Dhcp4Dxe","8DD9176E-EE87-4F0E-8A84-3F998311F930", "dhcp4dxe.efi", SRC_EFI, EFI_SECTION_PE32, FALSE},
  {"Ip4ConfigDxe","8F9296EF-2880-4659-B857-915A8901BDC8", "ip4configdxe.efi", SRC_EFI, EFI_SECTION_PE32, FALSE},
  {"Ip4Dxe","8F92960F-2880-4659-B857-915A8901BDC8", "ip4dxe.efi", SRC_EFI, EFI_SECTION_PE32, FALSE},
  {"Mtftp4Dxe","61AFA223-8AC8-4440-9AB5-762B1BF05156", "mtftp4dxe.efi", SRC_EFI, EFI_SECTION_PE32, FALSE},
  {"Udp4Dxe","10EE5462-B207-4A4F-ABD8-CB522ECAA3A4", "udp4dxe.efi", SRC_EFI, EFI_SECTION_PE32, FALSE},
  {"Dhcp6Dxe","8DD9176D-EE87-4F0E-8A84-3F998311F930", "dhcp4dxe.efi", SRC_EFI, EFI_SECTION_PE32, FALSE},
  {"Ip6Dxe","8F92960E-2880-4659-B857-915A8901BDC8", "ip6dxe.efi", SRC_EFI, EFI_SECTION_PE32, FALSE},
  {"Mtftp6Dxe","61AFA251-8AC8-4440-9AB5-762B1BF05156", "mtftp6dxe.efi", SRC_EFI, EFI_SECTION_PE32, FALSE},
  {"Udp6Dxe","10EE54AE-B207-4A4F-ABD8-CB522ECAA3A4", "udp6dxe.efi", SRC_EFI, EFI_SECTION_PE32, FALSE}
};

static const sectionEntry coreDxeSection = {
    "CORE_DXE","5AE3F37E-4EAE-41AE-8240-35465B5E81EB", "coredxe.efi", SRC_EFI, EFI_SECTION_PE32, TRUE
};

static const sectionEntry amiBoardSection = {
    "AmiBoardInfo","9F3A0016-AE55-4288-829D-D22FD344C347", "amiboardinfo.efi", SRC_EFI, EFI_SECTION_PE32, TRUE
};

static const sectionEntry filesystemSection = {
    "FileSystem","93022F8C-1F09-47EF-BBB2-5814FF609DF5", "filesystem.efi", SRC_EFI, EFI_SECTION_PE32, FALSE
};

#endif // COMMON_H
