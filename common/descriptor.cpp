/* descriptor.cpp

Copyright (c) 2015, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHWARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*/

#include "descriptor.h"

// Calculate address of data structure addressed by descriptor address format
// 8 bit base or limit
const UINT8* calculateAddress8(const UINT8* baseAddress, const UINT8 baseOrLimit)
{
    return baseAddress + baseOrLimit * 0x10;
}

// 16 bit base or limit
const UINT8* calculateAddress16(const UINT8* baseAddress, const UINT16 baseOrLimit)
{
    return baseAddress + baseOrLimit * 0x1000;
}

// Calculate offset of region using its base
UINT32 calculateRegionOffset(const UINT16 base)
{
    return base * 0x1000;
}

//Calculate size of region using its base and limit
UINT32 calculateRegionSize(const UINT16 base, const UINT16 limit)
{
    if (limit)
        return (limit + 1 - base) * 0x1000;
    return 0;
}

// Return human-readable chip name for given JEDEC ID
UString jedecIdToUString(UINT8 vendorId, UINT8 deviceId0, UINT8 deviceId1)
{
    UINT32 jedecId = (UINT32)deviceId1 + ((UINT32)deviceId0 << 8) + ((UINT32)vendorId << 16);
    switch (jedecId) {
    // Winbond
    case 0xEF3013: return UString("Winbond W25X40");
    case 0xEF3014: return UString("Winbond W25X80");
    case 0xEF3015: return UString("Winbond W25X16");
    case 0xEF3016: return UString("Winbond W25X32");
    case 0xEF3017: return UString("Winbond W25X64");
    case 0xEF4013: return UString("Winbond W25Q40");
    case 0xEF4014: return UString("Winbond W25Q80");
    case 0xEF4015: return UString("Winbond W25Q16");
    case 0xEF4016: return UString("Winbond W25Q32");
    case 0xEF4017: return UString("Winbond W25Q64");
    case 0xEF4018: return UString("Winbond W25Q128");
    case 0xEF4019: return UString("Winbond W25Q256");
    case 0xEF6015: return UString("Winbond W25Q16 1.8v");
    case 0xEF6016: return UString("Winbond W25Q32 1.8v");
    case 0xEF6017: return UString("Winbond W25Q64 1.8v");
    case 0xEF6018: return UString("Winbond W25Q128 1.8v");

    // Macronix
    case 0xC22013: return UString("Macronix MX25L40");
    case 0xC22014: return UString("Macronix MX25L80");
    case 0xC22015: 
    case 0xC22415:
    case 0xC22515: return UString("Macronix MX25L16");
    case 0xC22016:
    case 0xC22535: return UString("Macronix MX25U16");
    case 0xC22536: return UString("Macronix MX25U32");
    case 0xC22537: return UString("Macronix MX25U64");
    case 0xC22538: return UString("Macronix MX25U128");
    case 0xC22539: return UString("Macronix MX25U256");
    case 0xC25E16: return UString("Macronix MX25L32");
    case 0xC22017: 
    case 0xC29517: return UString("Macronix MX25L64");
    case 0xC22018: return UString("Macronix MX25L128");
    case 0xC22019: return UString("Macronix MX25L256");
    
    // Micron
    case 0x202014: return UString("Micron M25P80");
    case 0x202015: return UString("Micron M25P16");
    case 0x202016: return UString("Micron M25P32");
    case 0x202017: return UString("Micron M25P64");
    case 0x202018: return UString("Micron M25P128");
    case 0x204011: return UString("Micron M45PE10");
    case 0x204012: return UString("Micron M45PE20");
    case 0x204013: return UString("Micron M45PE40");
    case 0x204014: return UString("Micron M45PE80");
    case 0x204015: return UString("Micron M45PE16");
    case 0x207114: return UString("Micron M25PX80");
    case 0x207115: return UString("Micron M25PX16");
    case 0x207116: return UString("Micron M25PX32");
    case 0x207117: return UString("Micron M25PX64");
    case 0x208011: return UString("Micron M25PE10");
    case 0x208012: return UString("Micron M25PE20");
    case 0x208013: return UString("Micron M25PE40");
    case 0x208014: return UString("Micron M25PE80");
    case 0x208015: return UString("Micron M25PE16");
    case 0x20BA15: return UString("Micron N25Q016");
    case 0x20BA16: return UString("Micron N25Q032");
    case 0x20BA17: return UString("Micron N25Q064");
    case 0x20BA18: return UString("Micron N25Q128");
    case 0x20BA19: return UString("Micron N25Q256");
    case 0x20BA20: return UString("Micron N25Q512");
    case 0x20BA21: return UString("Micron N25Q00A");
    case 0x20BB15: return UString("Micron N25Q016 1.8v");
    case 0x20BB16: return UString("Micron N25Q032 1.8v");
    case 0x20BB17: return UString("Micron N25Q064 1.8v");
    case 0x20BB18: return UString("Micron MT25Q128 1.8v");
    case 0x20BB19: return UString("Micron MT25Q256 1.8v");
    case 0x20BB20: return UString("Micron MT25Q512 1.8v");

    // Atmel
    case 0x1F4500: return UString("Atmel AT26DF081");
    case 0x1F4501: return UString("Atmel AT26DF081A");
    case 0x1F4502: return UString("Atmel AT25DF081");
    case 0x1F4600: return UString("Atmel AT26DF161");
    case 0x1F4601: return UString("Atmel AT26DF161A");
    case 0x1F4602: return UString("Atmel AT25DF161");
    case 0x1F8600: return UString("Atmel AT25DQ161");
    case 0x1F4700: return UString("Atmel AT25DF321");
    case 0x1F4701: return UString("Atmel AT25DF321A");
    case 0x1F4800: return UString("Atmel AT25DF641");
    case 0x1F8800: return UString("Atmel AT25DQ641");

    // Microchip
    case 0xBF2541: return UString("Microchip SST25VF016B");
    case 0xBF254A: return UString("Microchip SST25VF032B");
    case 0xBF258D: return UString("Microchip SST25VF040B");
    case 0xBF258E: return UString("Microchip SST25VF080B");
    case 0xBF254B: return UString("Microchip SST25VF064C");

    // EON
    case 0x1C3013: return UString("EON EN25Q40");
    case 0x1C3014: return UString("EON EN25Q80");
    case 0x1C3015: return UString("EON EN25Q16");
    case 0x1C3016: return UString("EON EN25Q32");
    case 0x1C3017: return UString("EON EN25Q64");
    case 0x1C3018: return UString("EON EN25Q128");
    case 0x1C3114: return UString("EON EN25F80");
    case 0x1C3115: return UString("EON EN25F16");
    case 0x1C3116: return UString("EON EN25F32");
    case 0x1C3117: return UString("EON EN25F64");
    case 0x1C7015: return UString("EON EN25QH16");
    case 0x1C7016: return UString("EON EN25QH32");
    case 0x1C7017: return UString("EON EN25QH64");
    case 0x1C7018: return UString("EON EN25QH128");
    case 0x1C7019: return UString("EON EN25QH256");

    // GigaDevice
    case 0xC84014: return UString("GigaDevice GD25x80");
    case 0xC84015: return UString("GigaDevice GD25x16");
    case 0xC84016: return UString("GigaDevice GD25x32");
    case 0xC84017: return UString("GigaDevice GD25x64");
    case 0xC84018: return UString("GigaDevice GD25x128");
    case 0xC86017: return UString("GigaDevice GD25Lx64");
    case 0xC86018: return UString("GigaDevice GD25Lx128");

    // Fidelix
    case 0xF83215: return UString("Fidelix FM25Q16");
    case 0xF83216: return UString("Fidelix FM25Q32");
    case 0xF83217: return UString("Fidelix FM25Q64");
    case 0xF83218: return UString("Fidelix FM25Q128");

    // Spansion
    case 0x014015: return UString("Spansion S25FL116K");
    case 0x014016: return UString("Spansion S25FL132K");
    case 0x014017: return UString("Spansion S25FL164K");
        
    // Amic
    case 0x373015: return UString("Amic A25L016");
    case 0x373016: return UString("Amic A25L032");
    case 0x374016: return UString("Amic A25L032A");

    // PMC
    case 0x7F9D13: return UString("PMC Pm25LV080B");
    case 0x7F9D14: return UString("PMC Pm25LV016B");
    case 0x7F9D44: return UString("PMC Pm25LQ080C");
    case 0x7F9D45: return UString("PMC Pm25LQ016C");
    case 0x7F9D46: return UString("PMC Pm25LQ032C");

    // ISSI
    case 0x9D6017: return UString("ISSI Ix25LP064");
    case 0x9D6018: return UString("ISSI Ix25LP128");
    case 0x9D7018: return UString("ISSI Ix25WP128");
    }

    return UString("Unknown");
}
