#ifndef COMMON_H
#define COMMON_H

#include <QString>
#include "../peimage.h"

#define ERR_DIR_NOT_EXIST                   0xEB
#define ERR_FILE_NOT_FOUND                  0xEC
#define ERR_FILE_EXISTS                     0xED
#define ERR_REPLACE                         0xEE
#define ERR_RELOCATION                      0xEF
#define ERR_ERROR                           0xF0

#define DSDT_HEADER "DSDT"
#define DSDT_HEADER_SZ 4

#define UNPATCHABLE_SECTION ".ROM"

#define RUN_AS_IS           0
#define RUN_DELETE          1
#define RUN_DEL_OZM_NREQ    2

#define MIN_KEXT_ID 0x9
#define MAX_KEXT_ID 0xF

#define ALIGN16(Value) (((Value)+15) & ~15)
#define ALIGN32(Value) (((Value)+31) & ~31)

const static QString ozmDefaultsFilename = "OzmosisDefaults.plist";
static const QString DSDTFilename =  "DSDT.aml";
const static QString kextGUID = "DADE100%1-1B31-4FE4-8557-26FCEFC78275";

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

struct sectionEntry {
    QString name;
    QString GUID;
    BOOLEAN required;
};

#define requiredFfsCount 6

static const sectionEntry requiredFfs[] = {
  {"PartitionDxe","1FA1F39E-FEFF-4AAE-BD7B-38A070A3B609",TRUE},
  {"EnhancedFat","961578FE-B6B7-44C3-AF35-6BC705CD2B1F",TRUE},
  {"HfsPlus","4CF484CD-135F-4FDC-BAFB-1AA104B48D36",TRUE},
  {"Ozmosis","AAE65279-0761-41D1-BA13-4A3C1383603F",TRUE},
  {"OzmosisDefaults","99F2839C-57C3-411E-ABC3-ADE5267D960D",TRUE},// not required, but small
  {"SmcEmulatorKext","DADE1001-1B31-4FE4-8557-26FCEFC78275",TRUE}
};

#define optionalFfsCount 10

static const sectionEntry optionalFfs[] = {
  {"ExtFs","B34E5765-2E04-4DAF-867F-7F40BE6FC33D", FALSE},
  {"HermitShellX64","C57AD6B7-0515-40A8-9D21-551652854E37", FALSE},
  {"OzmosisTheme","AC255206-DCF9-4837-8353-72BBBC0AC849", FALSE},
  {"DisablerKext","DADE1002-1B31-4FE4-8557-26FCEFC78275", FALSE},
  {"InjectorKext","DADE1003-1B31-4FE4-8557-26FCEFC78275", FALSE},
  {"PostbootMounter","DADE1004-1B31-4FE4-8557-26FCEFC78275", FALSE},
  {"PostbootSymbols","DADE1005-1B31-4FE4-8557-26FCEFC78275", FALSE},
  {"CpuSensorsKext","DADE1006-1B31-4FE4-8557-26FCEFC78275", FALSE},
  {"LpcSensorsKext","DADE1007-1B31-4FE4-8557-26FCEFC78275", FALSE},
  {"GpuSensorsKext","DADE1008-1B31-4FE4-8557-26FCEFC78275", FALSE}
};

#define deletableFfsCount 10

static const sectionEntry deletableFfs[] = {
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

static const sectionEntry coreDxeSection = {
    "CORE_DXE","5AE3F37E-4EAE-41AE-8240-35465B5E81EB", TRUE
};

static const sectionEntry amiBoardSection = {
    "AmiBoardInfo","9F3A0016-AE55-4288-829D-D22FD344C347", TRUE
};

static const sectionEntry filesystemSection = {
    "FileSystem","93022F8C-1F09-47EF-BBB2-5814FF609DF5", FALSE
};

static const sectionEntry ozmosisDefaults = {
    "OzmosisDefaults","99F2839C-57C3-411E-ABC3-ADE5267D960D", TRUE
};

#endif // COMMON_H
