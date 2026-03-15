#include <catch_amalgamated.hpp>
#include "descriptor.h"

TEST_CASE("Smoke test: descriptor compiles and links", "[descriptor][smoke]") {
    REQUIRE(calculateRegionOffset(0) == 0);
}

TEST_CASE("calculateRegionOffset", "[descriptor]") {
    REQUIRE(calculateRegionOffset(0) == 0);
    REQUIRE(calculateRegionOffset(1) == 0x1000);
    REQUIRE(calculateRegionOffset(0x10) == 0x10000);
}

TEST_CASE("calculateRegionSize", "[descriptor]") {
    SECTION("limit zero means absent") {
        REQUIRE(calculateRegionSize(0, 0) == 0);
        REQUIRE(calculateRegionSize(5, 0) == 0);
    }
    SECTION("single page") {
        REQUIRE(calculateRegionSize(1, 1) == 0x1000);
    }
    SECTION("multi page") {
        REQUIRE(calculateRegionSize(0, 1) == 0x2000);
    }
    SECTION("large region") {
        REQUIRE(calculateRegionSize(0, 0xFFFF) == (UINT32)0x10000 * 0x1000);
    }
}

TEST_CASE("jedecIdToUString", "[descriptor][jedec]") {
    SECTION("known chip: Winbond W25Q128") {
        REQUIRE(jedecIdToUString(0xEF, 0x40, 0x18) == UString("Winbond W25Q128"));
    }
    SECTION("known chip: Macronix MX25L128") {
        REQUIRE(jedecIdToUString(0xC2, 0x20, 0x18) == UString("Macronix MX25L128"));
    }
    SECTION("unknown chip") {
        UString result = jedecIdToUString(0x00, 0x00, 0x00);
        REQUIRE(result == usprintf("Unknown %08Xh", 0));
    }
}

TEST_CASE("calculateAddress8", "[descriptor]") {
    UINT8 base[1] = {0};
    const UINT8* baseAddr = base;
    SECTION("zero offset") {
        REQUIRE(calculateAddress8(baseAddr, 0) == baseAddr);
    }
    SECTION("multiplies by 0x10") {
        REQUIRE(calculateAddress8(baseAddr, 1) == baseAddr + 0x10);
        REQUIRE(calculateAddress8(baseAddr, 0xFF) == baseAddr + 0xFF0);
    }
}

TEST_CASE("calculateAddress16", "[descriptor]") {
    UINT8 base[1] = {0};
    const UINT8* baseAddr = base;
    SECTION("zero offset") {
        REQUIRE(calculateAddress16(baseAddr, 0) == baseAddr);
    }
    SECTION("multiplies by 0x1000") {
        REQUIRE(calculateAddress16(baseAddr, 1) == baseAddr + 0x1000);
    }
}
