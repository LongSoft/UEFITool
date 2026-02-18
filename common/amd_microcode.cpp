#include "amd_microcode.h"
#include "basetypes.h"

UINT32 getDataSizeMicrocodeAmd(const AMD_MICROCODE_HEADER *ucodeHeader) {
    if (ucodeHeader->LoaderID >= 0x8005) {
        return ((ucodeHeader->InitializationFlag << 8) | ucodeHeader->DataSize) * 0x10;
    }

    return ucodeHeader->DataSize;
}

UINT32 getSizeMicrocodeAmd(const AMD_MICROCODE_HEADER *ucodeHeader) {
    UINT32 microcodeDataLen = getDataSizeMicrocodeAmd(ucodeHeader);
    UINT32 cpuIdByte = (getCpuIdMicrocodeAmd(ucodeHeader) >> 16) & 0xFF;
    UINT32 microcodeLen = 0;

    if (microcodeDataLen == 0x20) {
        microcodeLen = 0x3C0;
    } else if (microcodeDataLen == 0x10) {
        microcodeLen = 0x200;
    } else if (microcodeDataLen) {
        microcodeLen = microcodeDataLen;
    } else if (cpuIdByte == 0x50) {
        microcodeLen = 0x620;
    } else if (cpuIdByte == 0x58) {
        microcodeLen = 0x567;
    } else if (cpuIdByte == 0x60 || cpuIdByte == 0x61 || cpuIdByte == 0x63 || cpuIdByte == 0x66 || cpuIdByte <= 0x67) {
        microcodeLen = 0xA20;
    } else if (cpuIdByte == 0x68 || cpuIdByte == 0x69) {
        microcodeLen = 0x980;
    } else if (cpuIdByte == 0x70 || cpuIdByte == 0x73) {
        microcodeLen = 0xD60;
    } else if (cpuIdByte >= 0x80 && cpuIdByte <= 0x83 || cpuIdByte >= 0x85 && cpuIdByte <= 0x8A) {
        microcodeLen = 0xC80;
    } else if (cpuIdByte >= 0xA0 && cpuIdByte <= 0xA7 || cpuIdByte == 0xAA) {
        microcodeLen = 0x15C0;
    } else if (cpuIdByte == 0xB4) {
        microcodeLen = 0x3820;
    }

    return microcodeLen;
}

UINT32 getCpuIdMicrocodeAmd(const AMD_MICROCODE_HEADER *ucodeHeader) {
    return (((ucodeHeader->ProcessorSignature >> 8) & 0xFF) << 16) | (0x0f << 8) |
           (ucodeHeader->ProcessorSignature & 0xff);
}

UINT16 getYearMicrocodeAmd(const AMD_MICROCODE_HEADER *ucodeHeader) {
    if (getCpuIdMicrocodeAmd(ucodeHeader) == 0x00800F11 && ucodeHeader->UpdateRevision == 0x8001105 &&
        ucodeHeader->DateYear == 0x2016) {
        return 0x2017;
    }

    return ucodeHeader->DateYear;
}

UINT8 getMonthMicrocodeAmd(const AMD_MICROCODE_HEADER *ucodeHeader) {
    if (getCpuIdMicrocodeAmd(ucodeHeader) == 0x00300F10 && ucodeHeader->UpdateRevision == 0x3000027 &&
        ucodeHeader->DateMonth == 0x13) {
        return 0x12;
    } else if (getCpuIdMicrocodeAmd(ucodeHeader) == 0x00730F01 && ucodeHeader->UpdateRevision == 0x7030106 &&
               ucodeHeader->DateMonth == 0x09) {
        return 0x02;
    }

    return ucodeHeader->DateMonth;
}

UINT8 getDayMicrocodeAmd(const AMD_MICROCODE_HEADER *ucodeHeader) {
    if (getCpuIdMicrocodeAmd(ucodeHeader) == 0x00730F01 && ucodeHeader->UpdateRevision == 0x7030106 &&
        ucodeHeader->DateDay == 0x02) {
        return 0x09;
    }

    return ucodeHeader->DateDay;
}
