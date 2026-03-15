#include <cstring>
#include <catch_amalgamated.hpp>
#include "ffs.h"

TEST_CASE("Smoke test: ffs compiles and links", "[ffs][smoke]") {
    UINT8 buf[3] = {0, 0, 0};
    REQUIRE(uint24ToUint32(buf) == 0);
}

TEST_CASE("uint24ToUint32", "[ffs][size]") {
    SECTION("zero") {
        UINT8 buf[] = {0x00, 0x00, 0x00};
        REQUIRE(uint24ToUint32(buf) == 0);
    }
    SECTION("maximum 24-bit value") {
        UINT8 buf[] = {0xFF, 0xFF, 0xFF};
        REQUIRE(uint24ToUint32(buf) == 0x00FFFFFF);
    }
    SECTION("LSB only") {
        UINT8 buf[] = {0x01, 0x00, 0x00};
        REQUIRE(uint24ToUint32(buf) == 1);
    }
    SECTION("middle byte") {
        UINT8 buf[] = {0x00, 0x01, 0x00};
        REQUIRE(uint24ToUint32(buf) == 256);
    }
    SECTION("MSB only") {
        UINT8 buf[] = {0x00, 0x00, 0x01};
        REQUIRE(uint24ToUint32(buf) == 65536);
    }
}

TEST_CASE("uint32ToUint24", "[ffs][size]") {
    SECTION("round-trip") {
        for (UINT32 val : {0u, 1u, 255u, 256u, 65536u, 0x00FFFFFFu}) {
            UINT8 buf[3];
            uint32ToUint24(val, buf);
            REQUIRE(uint24ToUint32(buf) == val);
        }
    }
    SECTION("truncates upper byte") {
        UINT8 buf[3];
        uint32ToUint24(0x01000000, buf);
        REQUIRE(uint24ToUint32(buf) == 0);
    }
}

TEST_CASE("guidToUString", "[ffs][guid]") {
    SECTION("zero GUID formats correctly") {
        EFI_GUID zero = {0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0}};
        UString result = guidToUString(zero, false);
        REQUIRE(result == UString("00000000-0000-0000-0000-000000000000"));
    }
    SECTION("non-zero GUID formats correctly") {
        EFI_GUID guid = {0x8C8CE578, 0x8A3D, 0x4F1C, {0x99, 0x35, 0x89, 0x61, 0x85, 0xC3, 0x2D, 0xD3}};
        UString result = guidToUString(guid, false);
        REQUIRE(result == UString("8C8CE578-8A3D-4F1C-9935-896185C32DD3"));
    }
}

TEST_CASE("ustringToGuid", "[ffs][guid]") {
    SECTION("valid GUID round-trips") {
        EFI_GUID original = {0xDEADBEEF, 0xCAFE, 0xBABE, {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF}};
        UString str = guidToUString(original, false);
        EFI_GUID parsed;
        REQUIRE(ustringToGuid(str, parsed));
        REQUIRE(memcmp(&original, &parsed, sizeof(EFI_GUID)) == 0);
    }
    SECTION("invalid string") {
        EFI_GUID guid;
        REQUIRE_FALSE(ustringToGuid(UString("not-a-guid"), guid));
    }
    // Note: empty string is not rejected by the current implementation
    // (sscanf returns EOF, not 0, so the check passes). This is a known
    // upstream quirk, not tested here.
}

TEST_CASE("fileTypeToUString", "[ffs][types]") {
    REQUIRE(fileTypeToUString(EFI_FV_FILETYPE_RAW) == UString("Raw"));
    REQUIRE(fileTypeToUString(EFI_FV_FILETYPE_DXE_CORE) == UString("DXE core"));
    REQUIRE(fileTypeToUString(EFI_FV_FILETYPE_PAD) == UString("Pad"));
    // Unknown
    UString unknown = fileTypeToUString(0xFE);
    REQUIRE(unknown == usprintf("Unknown %02Xh", 0xFE));
}

TEST_CASE("sectionTypeToUString", "[ffs][types]") {
    REQUIRE(sectionTypeToUString(EFI_SECTION_COMPRESSION) == UString("Compressed"));
    REQUIRE(sectionTypeToUString(EFI_SECTION_PE32) == UString("PE32 image"));
    REQUIRE(sectionTypeToUString(EFI_SECTION_RAW) == UString("Raw"));
    REQUIRE(sectionTypeToUString(PHOENIX_SECTION_POSTCODE) == UString("Phoenix postcode"));
    REQUIRE(sectionTypeToUString(INSYDE_SECTION_POSTCODE) == UString("Insyde postcode"));
}
